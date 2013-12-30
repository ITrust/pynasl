#ifndef REPORT_H_
#define REPORT_H_

#include <Python.h>

typedef struct report_s
{
  int	port;
  char	*msg;
  struct report_s *next;
} report_t;

void    get_report(report_t **report, const char *line);

PyObject * make_report(report_t *report);

#endif
