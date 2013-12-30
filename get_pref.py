#!/usr/bin/python

import pynaslmod
import sys

if len(sys.argv) < 3:
    print "Usage : ./get_pref.py file_conf plugin1 plugin2 ..."
    exit(0)

pynaslmod.init_conf(sys.argv[1])
pynaslmod.init_nvticache()

for plugin in sys.argv[2:]:
    if not plugin.endswith('.nasl'):
        plugin += '.nasl'
    print plugin + ':'
    nvti = pynaslmod.nvti(plugin)
    info =  nvti.get_nvti_info()
    nvti_path = pynaslmod.conf.global_conf["cache_folder"] + '/' + plugin + '.nvti'
    f = open(nvti_path)
    data = f.read()
    f.close()
    try:
        data_pref = data.split('[NVT Prefs]')[1].split('\n')
        plugin_name = data.split('[NVT Prefs]')[0].split('Name=')[1].split('\n')[0]
    except IndexError:
        print '%s has no preference\n' % plugin
        continue
    for pref in data_pref[1:-1]:
        elem_pref = pref.split('=')[1].split(';')
        print '%s[%s]:%s' %(plugin_name, elem_pref[1], elem_pref[0])
        if len(elem_pref) >= 3:
            if elem_pref[1] == 'radio':
                print 'Possible value : %s.\n' % ', '.join([a[:-1] for a in elem_pref[2:]])[:-2]
            elif elem_pref[1] == 'checkbox':
                print 'Default value is %s\n' % elem_pref[2]
            else:
                print
