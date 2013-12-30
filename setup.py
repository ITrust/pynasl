#!/usr/bin/env python

from distutils.core import setup, Extension
import commands

def pkg_config(*packages, **kw):
    flag_map = {'-I': 'include_dirs', '-L': 'library_dirs', '-l': 'libraries'}
    for token in commands.getoutput("pkg-config --libs --cflags %s" % ' '.join(packages)).split():
        if flag_map.has_key(token[:2]):
            kw.setdefault(flag_map.get(token[:2]), []).append(token[2:])
        else:
            kw.setdefault('extra_link_args', []).append(token)
    for k, v in kw.iteritems():
        kw[k] = list(set(v))
    return kw

pynasl_ = Extension(
    'pynasl_',
    **pkg_config(
        'glib-2.0',
        libraries=['openvas_hg', 'openvas_misc', 'openvas_nasl', 'openvas_base'],
        sources=['pynasl_.c', 'kbs.c', 'nvti.c', 'utils.c', 'config.c', 'socket.c', 'report.c'],
        include_dirs=[
            './openvas-libraries-6.0.1/hg/',
            './openvas-libraries-6.0.1/nasl',
            './openvas-libraries-6.0.1/misc',
            './openvas-libraries-6.0.1/base',
            './scanner_include/'
            ],
        define_macros=[('OPENVAS_USERS_DIR', '\"/usr/local/var/lib/openvas/users\"')],
        )
    )

setup(name='pynasl_',
      version='1.0',
      description='pynasl_',
      author='Quentin Poirier',
      author_email='quentin.poirier@epitech.eu',
      ext_modules=[pynasl_])

setup(name='pynaslmod',
      version='1.0',
      description='pynasl module',
      author='Daniel Mercier',
      author_email='dmercier@itrust.fr',
      package_dir={'pynaslmod' : 'pynasl_module'},
      packages=['pynaslmod'])
