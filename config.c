/*
** config.c for  in /home/eax/stage/pynasl
** 
** Made by kevin soules
** Login   <soules_k@epitech.net>
** 
** Started on  Fri Nov 22 14:04:50 2013 kevin soules
** Last update Tue Dec 24 16:41:20 2013 ampotos
*/

// gcc config.c -o config -Iopenvas-libraries-6.0.1/{misc,nasl}/ -I/usr/include/glib-2.0/ -I/usr/lib/glib-2.0/include/ -lopenvas_misc -lopenvas_nasl

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arglists.h>
#include <system_internal.h>
#include <nasl.h>
#include "config.h"

typedef struct
{
  char *key;
  char *value;
} t_pref;

static t_pref pref_defaults[] = {
  {"plugins_folder", "/var/lib/openvas/plugins"},
  {"cache_folder", "/var/lib//cache/openvas"},
  {"include_folders", "/var/lib/openvas/plugins"},
  /* {"logfile", "/var/log/openvas/openvassd.messages"}, */
  {"log_whole_attack", "no"},
  {"log_plugins_name_at_load", "no"},
  {"dumpfile", "/var/log/openvas/openvassd.dump"},
  {"cgi_path", "/cgi-bin:/scripts"},
  {"port_range", "default"},
  {"checks_read_timeout", "5"},
  {"network_scan", "no"},
  {"non_simult_ports", "139, 445"},
  {"plugins_timeout", "320"},
  {"safe_checks", "yes"},
  {"use_mac_addr", "no"},
  {"kb_max_age", "864000"},
  {"unscanned_closed", "yes"},
  // Empty options must be "\0", not NULL, to match the behavior of
  // preferences_process.
  {"vhosts", "\0"},
  {"vhosts_ip", "\0"},
  {"reverse_lookup", "no"},
  {NULL, NULL}
};

struct arglist *g_prefs;

static int is_char_to_trim(char c)
{
  return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

static char	*my_trim(char *s)
{
  char		*start;
  char		*end;
  int		len;

  start = s - 1;
  len = strlen(s);
  end = s + len;

  while (is_char_to_trim(*(++start)));
  while (is_char_to_trim(*(--end)));
  *(end + 1) = '\0';
  return (start);
}

static int	parse_one_line(char *line, struct arglist *argl, int overwrite)
{
  char		*dup;
  char		*key;
  char		*value;

  while (*line == ' ' || *line == '\t')
    line++;
  dup = strdup(line);
  if (!dup)
    return (-1);

  key = dup;
  value = strchr(dup, '=');
  if (!value)
    return (-1);

  *value = '\0';
  value++;
  key = my_trim(key);
  value = my_trim(value);
  if (*key == '#')
    return (0);
  if (arg_get_value(argl, key) == NULL)
    arg_add_value(argl, key, ARG_STRING, sizeof(*value), value);
  else if (overwrite)
    arg_set_value(argl, key, ARG_STRING, value);
  return (1);
}

struct arglist	*init_conf()
{
  int			i;
  struct arglist	*argl;

  argl = emalloc (sizeof (*argl));
  i = 0;
  while (pref_defaults[i].key)
    {
      pref_defaults[i];
      arg_add_value(argl, pref_defaults[i].key, ARG_STRING,
		    sizeof(*pref_defaults[i].value), pref_defaults[i].value);
      i++;
    }
  return argl;
}

int		load_conf(char *filename, struct arglist *argl, int overwrite)
{
  FILE		*fd;
  char		*buf;
  ssize_t	size;
  ssize_t	ret;

  buf = NULL;
  ret = 42;

  if ((fd = fopen(filename, "r")) == NULL)
    {
      perror("fopen()");
      return (-1);
    }
  while (ret > 0)
    {
      ret = getline(&buf, &size, fd);
      size = 0;
      if (ret > 0)
	parse_one_line(buf, argl, overwrite);
      free(buf);
      buf = NULL;
    }
  fclose(fd);
  return (0);
}


char	*conf_get_value(char *key)
{
  return arg_get_value(g_prefs, key);
}
