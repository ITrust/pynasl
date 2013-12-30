#include <Python.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <pwd.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "network.h"
#include "system.h"
#include "hosts_gatherer.h"

#include "kb.h"
#include "nvti.h"
#include "nasl_tree.h"
#include "nasl.h"
#include "nasl_global_ctxt.h"

#include "nasl_lex_ctxt.h"
#include "nasl_init.h"
#include "nasl_scanner_glue.h"
#include "exec.h"

#include "pynasl_.h"
#include "nvti_pynasl.h"
#include "utils.h"
#include "config.h"
#include "plugutils.h"
#include "internal_com.h"
#include "socket.h"
#include "report.h"

struct	kb_item  **kbs;
int	g_socket_out;

static void
sighandler(__attribute__((unused))int s) {
  exit(0);
}

static void
handle_signals(void) {
  signal(SIGINT, sighandler);
  signal(SIGTERM, sighandler);
  signal(SIGPIPE, SIG_IGN);
}

static void
handle_kbs(struct arglist *script_infos, char *hostname) {

  struct kb_item *tkb = kb_item_get_pattern(kbs, "*");
  
  while (tkb != NULL) {
    switch (tkb->type) {
    case KB_TYPE_INT:
      save_kb_write_int(script_infos, hostname, tkb->name, tkb->v.v_int);
      break;
    case KB_TYPE_STR:
      save_kb_write_str(script_infos, hostname, tkb->name, tkb->v.v_str);
      break;
    default:
      break;
    }
    tkb = tkb->next;
  }
}

static struct arglist *
init_plug(struct arglist *prefs, nvti_t *nvti, char *hostname, struct in6_addr *ip)
{
  struct arglist	*script_infos;
  struct arglist	*hostinfos;
  struct arglist	*ports;
  struct arglist	*globals;

  script_infos = plug_create_from_nvti_and_prefs(nvti, prefs);
  arg_add_value (script_infos, "key", ARG_PTR, -1, kbs);

  hostinfos = emalloc (sizeof (*hostinfos));
  arg_add_value (hostinfos, "FQDN", ARG_STRING, strlen (hostname), hostname);
  arg_add_value (hostinfos, "NAME", ARG_STRING, strlen (hostname), hostname);
  arg_add_value (hostinfos, "IP", ARG_PTR, sizeof (struct in6_addr), ip);

  globals =  emalloc (sizeof (*globals));
  arg_add_value(globals, "global_socket", ARG_INT, -1, (void*) g_socket_out);
  arg_add_value(script_infos, "globals", ARG_ARGLIST, sizeof(*globals),  globals);
  
  ports = emalloc (sizeof (*ports));
  arg_add_value (hostinfos, "PORTS", ARG_ARGLIST, sizeof (struct arglist), ports);

  arg_add_value (script_infos, "HOSTNAME", ARG_ARGLIST, -1, hostinfos);
  return script_infos;
}

void parse_kb(char *line)
{
  int	ct_key = -1;
  int	ct_value = -1;
  int	ct_digit = -1;

  while (line[++ct_key] != ' ' && line[ct_key]);
  if (line[ct_key + 2] == ' ')
    ct_key += 2;
  if (line[ct_key] == ' ')
    ct_key++;
  while (line[ct_key + ++ct_value] != '=' && line[ct_key + ct_value]);
  line[ct_key + ct_value] = '\0';
  ct_value += ct_key + 1;
  while(line[ct_value + ++ct_digit] != '\n' && isdigit(line[ct_value + ct_digit]));
  if (line[ct_value + ct_digit] != '\n')
    kb_item_add_str(kbs, line + ct_key, line + ct_value);
  else
    kb_item_add_int(kbs, line + ct_key, atoi(line + ct_value));
}



static PyObject *
m_init_conf(PyObject *self, PyObject *args)
{
  gchar			*conf_filename = NULL;
  struct arglist	*p;
  PyObject		*dict;
  gchar			*include_dir;

  if (!PyArg_ParseTuple(args, "s", &conf_filename)) {
    return Py_False;
  }

  g_prefs = init_conf();
  if (load_conf(conf_filename, g_prefs, 1) == -1)
    {
      /* fprintf(stderr, "Failed to load config file: %s\n,", conf_filename); */
      return Py_False;
    }
  
  include_dir = conf_get_value("include_folders");
  add_nasl_inc_dir(include_dir);
 
  dict = PyDict_New();
  for (p = g_prefs ; p && p->name ; p = p->next)
    {
      PyDict_SetItem(dict, PyString_FromString(p->name), PyString_FromString(p->value));
    }

  return Py_BuildValue("N", dict);
}

static PyObject *
m_init_kb(PyObject *self, PyObject *args)
{
  gchar		*kb_file_name = NULL;
  FILE		*kb_file;
  size_t	size;
  int		size_read;
  char		*line;

  if (!PyArg_ParseTuple(args, "|s", &kb_file_name)) {
    return Py_False;
  }
  /*
  ** If no kb_file is no specified we create a new kb file
  */
  kbs = kb_new();
  if (!kb_file_name)
    return Py_True;
  /*
  ** Else we load the kb file
  */
  size_read = 42;
  line = NULL;
  if ((kb_file = fopen(kb_file_name, "r")) == NULL)
    return Py_False;
  while (size_read > 0)
    {
      size_read = getline(&line, &size, kb_file);
      size = 0;
      if (size_read > 0)
	{
	  if (line[size_read - 1] == '\n')
	    line[size_read - 1] = '\0';	  
	  parse_kb(line);
	  free(line);
	  line = NULL;
	}
    }
  fclose(kb_file);
  return Py_True;
}


/**
 * launch_script(@script_path, @target)
 * @script_path is the nasl script path
 * @target is the host-target (ip or hostname)
 *
 * Both @target and @script_path must be non-NULL, non-empty values
 *
 * This function is a simplified version of the main function of openvas-nasl
 */

static PyObject *
m_launch_script(PyObject *self, PyObject *args) {
  gchar			*script_path;
  gchar			*target = "127.0.0.1";
  int			mode = 0;
  gchar			*nvti_path;
  nvti_t		*nvti;
  struct hg_globals	*hg_globals;
  char			hostname[1024];
  struct in6_addr	ip6;
  struct arglist	*script_infos;
  struct passwd		*pwd;
  char			launched[512];
  pid_t			pid;
  pid_t			ppid;
  int			ctrl_finish;
  int			status;
  struct rusage		rusage;
  report_t		*report = NULL;
  PyObject		*ret = NULL;
  gchar			*kb_out_name = NULL;

  if (!PyArg_ParseTuple(args, "ss|s", &script_path, &target, &kb_out_name)) {
    return NULL;
  }


  mode |= NASL_COMMAND_LINE;
  mode |= NASL_ALWAYS_SIGNED;

  openvas_SSL_init();

  if (geteuid() != 0) {
    fprintf(stderr, "Plugin execution must be ran as root\n");
    return Py_False;
  }
  handle_signals();

  if ((pwd = getpwuid(getuid())) == NULL) {
    fprintf(stderr, "getpwuid() error\n");
    return Py_False;
  }

  hg_globals = hg_init(target, 4);

  nvti = my_nvticache_get(g_nvticache, script_path);
  free(nvti_path);

  while (hg_next_host(hg_globals, &ip6, hostname, sizeof(hostname)) >= 0) {
    script_infos = init_plug(g_prefs, nvti, hostname, &ip6);
    arg_add_value(script_infos, "user", ARG_STRING, strlen(pwd->pw_name), pwd->pw_name);

    snprintf(launched, sizeof(launched), "Launched/%s", nvti_oid(nvti));
    kb_item_set_int(kbs, launched, 1);

    ctrl_finish = 1<<16 | 1;

    if ((pid = fork()) == 0)
      {
	ppid = getppid();
	if (exec_nasl_script(script_infos, script_path, mode) < 0) {
	  fprintf(stderr, "launch_script: Error on script execution\n");
	}
	send(g_socket_out, &ctrl_finish, 4, 0);
	recv(g_socket_out, &ctrl_finish, 4, 0);
	exit(0);
      }

    listen_kb_socket(&report);
    

    save_kb_new(script_infos, hostname, kb_out_name);
    handle_kbs(script_infos, hostname);
    save_kb_close(script_infos, hostname, kb_out_name);
 }
  hg_cleanup(hg_globals);
  nvti_free(nvti);
  return make_report(report);
}

static PyMethodDef m_methods[] = {
  {"launch_script", m_launch_script, METH_VARARGS, "launch script on target host"},
  {"create_nvti", m_create_nvti, METH_VARARGS, "create an nvti file for this plugin"},
  {"info_from_nvti", m_get_nvti_info, METH_VARARGS, "get plugin information form nvti file"},
  {"init_kb", m_init_kb, METH_VARARGS, "init kb in memory (new kb or load an existing kb file)"},
  {"init_conf_", m_init_conf, METH_VARARGS, "init conf in memory"},
  {"init_socket", m_init_socket, METH_VARARGS, "init socket in memory"},
  {"was_launched", m_was_launched, METH_VARARGS, "check in kb if the plugin with the oid was launched or not"},
  {"init_nvticache", m_init_nvticache, METH_VARARGS, "init nvticache in memory"},
  {NULL, NULL, 0, NULL}        /* Sentinel */
};

// FIXME : handle properly the init of the module
PyMODINIT_FUNC
initpynasl_(void) {
  PyObject *m;

  m = Py_InitModule("pynasl_", m_methods);
  if (m == NULL)
    return;
}
