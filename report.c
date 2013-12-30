#include "report.h"
#include <string.h>
#include <stdio.h>
#include <Python.h>

void	get_report(report_t **report, const char *line)
{
  char		*msg;
  int		ct_port;
  int		port;
  report_t	*new_report;

  if ((msg = strstr(line, "report")) == NULL)
    return;
  ct_port = (msg - line);
  while (--ct_port && line[ct_port - 1] != '/');
  port = atoi(line + ct_port);
  if ((new_report = malloc(sizeof(*new_report))) == NULL)
    return;
  new_report->port = port;
  new_report->msg = strdup(msg + 7);
  new_report->next = *report;
  *report = new_report;
}


PyObject * make_report(report_t *report)
{
  PyObject	*list = PyList_New(0);
  report_t	*to_free;

  while (report)
    {
      PyList_Append(list, Py_BuildValue("{siss}", "port", report->port, "report", report->
msg));
      to_free = report;
      report = report->next;
      free(to_free->msg);
      free(to_free);
    }
  return  list;
}
