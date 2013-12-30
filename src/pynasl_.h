#ifndef PYNASL_H__
# define PYNASL_H__


int	save_kb_new(struct arglist *globals, char *hostname, char *kbname);
int	save_kb_close(struct arglist *globals, char *hostname, char *kbname);

int	save_kb_write_str(struct arglist *globals, char *hostname, char *name, char *value);
int	save_kb_write_int(struct arglist *globals, char *hostname, char *name, int value);
void	parse_kb(char *line);

//kb is a global variable because we need to no reload all kb file for each plugin in the same instance of pynasl
#include "kb.h" 
extern struct kb_item  **kbs;

#endif
