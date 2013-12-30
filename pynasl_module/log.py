import conf
import logging

LOG_LEVEL = 0
logger = None

def init_log(level):
    global LOG_LEVEL
    global logger
    logger = logging.getLogger("Pynasl_logger")
    logger.setLevel(logging.INFO)
    try:
        ch = logging.FileHandler(conf.global_conf['logfile'])
    except KeyError:
        pass
    ch.setLevel(logging.INFO)
    formatter = logging.Formatter("%(asctime)s;%(levelname)s;%(message)s")
    ch.setFormatter(formatter)
    logger.addHandler(ch)

    try:
        if int(level) > 2 or int(level) < 0:
            print 'Bad level value, it must be between 0 and 2'
            LOG_LEVEL = 0
            return
    except ValueError:
        print 'Level must be an integer between 0 and 2'
        LOG_LEVEL = 0
        return
    LOG_LEVEL = int(level)
    
def log_info(msg):
    logger.info(msg)

