from pynasl_ import *
import conf
from nvti import nvti
import log
import inspect

executed_plugin = []
dep_check = []

class CircularDepError(Exception):
    def __init__(self, msg):
        self.msg = msg

    def __str__(self):
        return self.msg

def exec_script(plugins, target, **kwargs):
    global executed_plugin
    global dep_check

    kb_file = None
    nvti_info = None
    if "kb_file" in kwargs:
        kb_file = kwargs["kb_file"]
    if "nvti_info" in kwargs:
        nvti_info = kwargs["nvti_info"]
    nvti_dir = conf.global_conf["cache_folder"] # empty list
    ret = []
    if type(plugins) == str:
        plugins = [plugins]
    for plugin_name in plugins:
        if len(plugin_name) > 0:
            nvti_info = None
        if nvti_info == None:
            nvti_obj = nvti(plugin_name)
            nvti_info = nvti_obj.get_nvti_info()
        if log.LOG_LEVEL == 2:
            log.log_info('Check dependences for %s' % plugin_name)
        if nvti_info['dep']:
            for dep in nvti_info['dep'] :
                if dep in dep_check:
                    dep_check = []
                    raise CircularDepError('Plugin %s is in a infinite dependence loop' % dep)
                dep_check.append(dep)
                nvti_dep = nvti(dep)
                nvti_dep_info = nvti_dep.get_nvti_info()
                if not was_launched('Launched/' + nvti_dep_info['oid']):
                    ret_tmp = exec_script([dep], target, kb_file=kb_file, nvti_info=nvti_dep_info)
                    if ret_tmp == False:
                        return False
                    ret += ret_tmp
        if plugin_name not in executed_plugin:
            executed_plugin.append(plugin_name)
            if log.LOG_LEVEL >= 1:
                log.log_info('Start of execution of %s' % plugin_name)
            if kb_file == None:
                ret_new = launch_script(plugin_name, target)
            else:
                ret_new = launch_script(plugin_name, target, kb_file)
            if ret_new == False:
                return False
            if log.LOG_LEVEL >= 1:
                log.log_info('End of execution of %s' % plugin_name)
            for rapport in ret_new:
                rapport['plugin'] = plugin_name
            ret += ret_new
        if inspect.stack()[1][3] != 'exec_sript':
            dep_check = []
    for report in ret:
        report['report'] = report['report'].replace('\\n', '\n')
    return ret
