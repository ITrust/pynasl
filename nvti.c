#include <Python.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <libgen.h>

#include "system.h"

#include "nasl.h"

#include "nvticache.h"

#include "nvti_pynasl.h"
#include "pynasl_.h"
#include "nvticache.h"
#include "config.h"

nvticache_t *g_nvticache;

nvti_t *
my_nvticache_get (const nvticache_t * cache, const gchar * filename)
{
  nvti_t *n = NULL, *n2;
  gchar *src_file = g_build_filename (cache->src_path, filename, NULL);
  gchar *dummy = g_build_filename (cache->cache_path, filename, NULL);
  gchar *cache_file = g_strconcat (dummy, ".nvti", NULL);
  struct stat src_stat;
  struct stat cache_stat;

  g_free (dummy);

  if (src_file && cache_file && stat (src_file, &src_stat) >= 0
      && stat (cache_file, &cache_stat) >= 0
      && (cache_stat.st_mtime >= src_stat.st_mtime))
    {
      n = nvti_from_keyfile (cache_file);
    }

  if (src_file)
    g_free (src_file);
  if (cache_file)
    g_free (cache_file);

  if (!n) return NULL;
  
  n2 = nvtis_lookup (cache->nvtis, nvti_oid (n));
  if (n2)
    return nvti_clone (n2);
  else
    {
      n2 = nvti_clone (n);
      nvtis_add (cache->nvtis, n);
      return n2;
    }
}


PyObject *
m_init_nvticache(PyObject *self, PyObject *args)
{
  char  *nvti_path;
  char	*plugin_dir;

  nvti_path = conf_get_value("cache_folder");
  plugin_dir = conf_get_value("plugins_folder");

  g_nvticache = nvticache_new(nvti_path, plugin_dir);
  arg_add_value(g_prefs, "nvticache", ARG_PTR, -1, g_nvticache);
  return Py_True;
}

PyObject *
m_create_nvti(PyObject *self, PyObject *args)
{
  gchar			*script_path;
  gchar			*nvti_path;
  struct arglist	*script_data;
  nvti_t		*nvti;
  int			mode;

  if (!PyArg_ParseTuple(args, "ss", &script_path, &nvti_path))
    return Py_False;
  mode = NASL_EXEC_DESCR;
  mode |= NASL_ALWAYS_SIGNED;
  script_data = emalloc(sizeof(*script_data));

  add_nasl_inc_dir("");
  nvti = nvti_new ();

  arg_add_value (script_data, "NVTI", ARG_PTR, -1, nvti);
  arg_add_value (script_data, "preferences", ARG_ARGLIST, -1, NULL);

  if (exec_nasl_script(script_data, script_path, mode) < 0)
    {
      printf("create_nvti : Failed to load %s description\n", script_path);
      arg_free_all(script_data);
      return Py_False;
    }

  nvti_set_src (nvti, script_path);

  if (nvti_to_keyfile(nvti, nvti_path) != 0)
    {
      printf("create_nvti : Failed to create nvti for %s\n", script_path);
      arg_free_all(script_data);
      return Py_False;
    }
  arg_del_value(script_data, "NVTI");
  arg_free_all(script_data);
  return Py_True;
}

PyObject *
m_get_nvti_info(PyObject *self, PyObject *args)
{
  gchar		*nvti_path; 
  nvti_t	*nvti;
  gchar		*oid;
  PyObject	*dico;
  gchar		*dep;

  if (!PyArg_ParseTuple(args, "s", &nvti_path))
    return NULL;
  nvti = my_nvticache_get(g_nvticache, basename(nvti_path));
  oid = nvti_oid(nvti);
  dep = nvti_dependencies(nvti);
  dico = Py_BuildValue("{ssss}", "oid", oid, "dep", dep);
  return dico;
}

PyObject *
m_was_launched(PyObject *self, PyObject *args)
{
  gchar	*oid;

  if (!PyArg_ParseTuple(args, "s", &oid))
    return NULL;
  if (kb_item_get_int(kbs, oid) == 1)
    return Py_True;
  return Py_False;
}
