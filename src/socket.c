#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <Python.h>
#include "socket.h"
#include "internal_com.h"
#include "report.h"

static int	g_csock = 0;

PyObject *
m_init_socket(PyObject *self, PyObject *args)
{
  int	sockets[2];

  g_socket_out = sockets[0];
  g_csock = sockets[1];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
    perror("socketpair");
    exit(errno);
  }

  g_socket_out = sockets[0];
  g_csock = sockets[1];

  return Py_True;
}

void	listen_kb_socket(report_t **report)
{
  int			sunsize;
  struct sockaddr_in	csun;
  char			*buf;
  int			ret;
  int			line_size;
  int			type;
  int			ack;
  int			bufsize;

  line_size = 0;
  ack = INTERNAL_COMM_MSG_TYPE_CTRL | INTERNAL_COMM_CTRL_ACK;
  bufsize = 1024;
  if ((buf = malloc(bufsize)) == NULL)
    {
      perror("malloc()");
      exit(1);
    }
  while ((ret = recv(g_csock, &type, sizeof(type), 0)) > 0)
    {
      if (type == (INTERNAL_COMM_MSG_TYPE_CTRL | INTERNAL_COMM_CTRL_FINISHED))
	{
	  send(g_csock, &ack, sizeof(ack), 0);
	  break;
	}
      if ((ret = recv(g_csock, &line_size, sizeof(line_size), 0)) <= 0)
	break;

      if (line_size >= bufsize)
	{
	  bufsize = line_size + 1;
	  buf = realloc(buf, bufsize);
	}
      *buf = '\0';
      if (line_size > 0)
	{
	  if ((ret = recv(g_csock, buf, line_size, 0)) <= 0)
	    break;

	  buf[ret] = '\0';
	}
      buf[line_size - 2] = '\0';
      get_report(report, buf);
      if (type & INTERNAL_COMM_MSG_TYPE_KB)
      	parse_kb(buf);
      send(g_csock, &ack, sizeof(ack), 0);
    }
}
