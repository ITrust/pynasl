#ifndef NVTI_H__
# define NVTI_H__

#include "nvticache.h"

PyObject * m_create_nvti(PyObject *self, PyObject *args);

PyObject * m_get_nvti_info(PyObject *self, PyObject *args);

PyObject * m_was_launched(PyObject *self, PyObject *args);

void	init_nvticache();

PyObject * m_init_nvticache(PyObject *self, PyObject *args);

nvti_t *my_nvticache_get (const nvticache_t * cache, const gchar * filename);

extern nvticache_t *g_nvticache;

#endif
