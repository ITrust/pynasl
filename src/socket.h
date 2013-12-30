#ifndef SOCKET_H_
#define SOCKET_H_

#include <Python.h>
#include "report.h"

void		init_kb_socket_listen(char *host, int port);
int		init_client_soc(char *hostname, int port);
void		listen_kb_socket(report_t **report);
PyObject *	m_init_socket(PyObject *self, PyObject *args);

extern	int	g_socket_out;

#endif
