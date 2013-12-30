#!/usr/bin/env python2.7
## conf.py for  in /home/eax/stage/pynasl
## 
## Made by kevin soules
## Login   <soules_k@epitech.net>
## 
## Started on  Fri Nov 29 16:38:44 2013 kevin soules
## Last update Thu Dec 19 17:14:20 2013 kevin soules
##

global_conf = None

from pynasl_ import *

def init_conf(path):
    global global_conf
    global_conf = init_conf_(path)
    if not global_conf:
        raise ValueError("Config '%s' can't be found." % path)
    return global_conf
