#!/usr/bin/python
import pynaslmod

pynaslmod.init("cfg")
pynaslmod.init_log('2')

# try:
#     pynasl.exec_script('plugins_1.nasl', '127.0.0.1')
# except:
#     print "fail ^^"
pynaslmod.exec_script(['nmap.nasl', 'plugins_2.nasl'], '127.0.0.1', kb_file='/tmp/un')
# nvti = pynasl.nvti('gb_secpod_ssl_ciphers_weak_report.nasl')
# ret = pynasl.exec_script('gb_secpod_ssl_ciphers_weak_report.nasl', '127.0.0.1')
# print ret

