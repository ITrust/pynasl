#!/usr/bin/env python

import pynaslmod
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('-k', '--kb', default='', help='Specify a kb file to load and write')
parser.add_argument('-i', '--kb_in', default='', help='Specify a kb file to load')
parser.add_argument('-o', '--kb_out', default='', help='Specify a kb file to write')
parser.add_argument('-c', '--conf_file', default='', help='Specify a config file')
parser.add_argument('-v', '--verbose', action='count', default=0, help="Set level of verbosity (-v : print load, end of plugin, -vv : print more information)")
parser.add_argument('-p', '--plugins', default='', help="Plugins to exec (use ',' as separtor for multiple plugins)")
parser.add_argument('-f', '--file', default='', help="file who contain plugins list (one per line or use ',' as separator")
parser.add_argument('-t', '--target', default='', help="Target of the scan")

args = parser.parse_args()

if not args.target:
    print 'Target (-t) must be specified'
    exit (0)

kb_files = {}
if args.kb:
    kb_files['in'] = args.kb
    kb_files['out'] = args.kb
elif args.kb_in:
    kb_files['in'] = args.kb_in
elif args.kb_out:
    kb_files['out'] = args.kb_out

conf = args.conf_file
    
verbose = int(args.verbose)

if kb_files.has_key('in'):
    pynaslmod.init(conf, kb_src=kb_files['in'])
else:
    pynaslmod.init(conf)
    
if verbose >= 1:
    pynaslmod.init_log(verbose)
    
plugins = []
if args.plugins:
    plugins += [plugin.strip() for plugin in args.plugins.split(',') if plugin]
if args.file:
    f = open(args.file)
    data = f.read()
    f.close()
    for line in data.split('\n'):
        plugins += [plugin.strip() for plugin in line.split(',') if plugin]

if kb_files.has_key('out'):
    pynaslmod.exec_script(plugins, args.target, kb_file = kb_files['out'])
else:
    pynaslmod.exec_script(plugins, args.target)
