#!/usr/bin/python

import conf
import pynasl_

def init(cfg, **kwargs):
    kb_src = ""
    if "kb_src" in kwargs:
        kb_src = kwargs["kb_src"]
    conf.init_conf(cfg)
    if kb_src:
        pynasl_.init_kb(kb_src)
    else:
        pynasl_.init_kb()
    pynasl_.init_socket()
    pynasl_.init_nvticache()

