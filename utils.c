#include <string.h>
#include <stdlib.h>
#include <glib.h>

#include "utils.h"
#include "config.h"

gchar *
get_nvti_from_plug(gchar *plug_name)
{
  gchar	*nvti_path;
  char	*nvti_dir;
  int	ct;

  nvti_dir = conf_get_value("cache_folder");
  if ((nvti_path = malloc(sizeof(*nvti_path) * (strlen(nvti_dir) + strlen(plug_name) + 7))) == NULL)
    return NULL;
  strcpy(nvti_path, nvti_dir);
  ct = strlen(nvti_dir);
  if (nvti_dir[strlen(nvti_dir) - 1] != '/')
    {
      nvti_path[strlen(nvti_dir)] = '/';
      ct++;
    }
  strcpy(nvti_path + ct, plug_name);
  ct += strlen(plug_name);
  nvti_path[ct + 1] = '\0';
  return nvti_path;
}


