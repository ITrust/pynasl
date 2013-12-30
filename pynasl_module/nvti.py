import conf
from pynasl_ import *
import hashlib
import os.path
import log

##
## following var are used while conf was not load from a configuration file
##

class nvti(object):

    def __init__(self, plugin_name):
        if log.LOG_LEVEL == 1:
            log.log_info('load nvti for %s' % plugin_name)
        self.plugin_name = plugin_name
        self.plugin_dir = conf.global_conf["plugins_folder"]
        self.nvti_dir = conf.global_conf["cache_folder"]
        self.hash = hashlib.md5()
        self.check()

    def create_md5(self, plugin_data, md5_path, md5_sum=None):
        if md5_sum == None:
            self.hash.update(plugin_data)
            md5_sum = self.hash.hexdigest()
        md5_file = open(md5_path, "w")
        md5_file.write(md5_sum)
        md5_file.close()

    def check(self):
        full_name = self.plugin_dir + '/' + self.plugin_name
        self.nvti_path = self.nvti_dir + '/' + self.plugin_name + '.nvti'
        md5_path = self.nvti_dir + '/' + self.plugin_name + '.md5'

        if not os.path.isfile(full_name):
            raise ValueError("Plugin '%s' can't be found." % full_name )
        plugin = open(full_name)
    
        ##
        ## if nvti file not exist it's create, like the md5 file
        ##
        if not os.path.isfile(self.nvti_path):
            if log.LOG_LEVEL == 2:
                log.log_info('nvti for %s does not exist, pynasl create it' % self.plugin_name)
            create_nvti(full_name, self.nvti_path)
            self.create_md5(plugin.read(), md5_path) 
            return

        ##
        ## if md5 file not exist nvti file is regenerate and md5 file is create
        ##
        try:
            md5_file = open(md5_path)
        except IOError:
            if LOG_LEVEL == 2:
                log_info("nvti's md5 for %s does not exist, pyansl create it and recreate nvti file" % self.plugin_name)
            create_nvti(full_name, self.nvti_path)
            self.create_md5(plugin.read(), md5_path)
            md5_file.close()
            return

        old_md5_sum = md5_file.read()
        md5_file.close()
        plugin_data = plugin.read()
        self.hash.update(plugin_data)
        new_md5_sum = self.hash.hexdigest()
        plugin.close()

        ##
        ## if md5sum of the md5 file is different of md5sum of the plugin nvti en md5 file are regerate
        ##
        if new_md5_sum != old_md5_sum:
            if log.LOG_LEVEL == 2:
                log.log_info('Plugin %s was changed, pynasl recreate nvti file' % self.plugin_name)
            create_nvti(full_name, self.nvti_path)
            self.create_md5(plugin_data, md5_path, new_md5_sum)

    def get_nvti_info(self):
        if hasattr(self, 'nvti_info'):
            return self.nvti_info
        self.nvti_info = info_from_nvti(self.nvti_dir + '/' + self.plugin_name)
        self.nvti_info['name'] = self.plugin_name
        if self.nvti_info['dep']:
            self.nvti_info['dep'] = self.nvti_info['dep'].split(', ')
        return self.nvti_info
