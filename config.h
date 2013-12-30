/*
** config.h for  in /home/eax/stage/pynasl
** 
** Made by kevin soules
** Login   <soules_k@epitech.net>
** 
** Started on  Fri Nov 22 14:04:28 2013 kevin soules
** Last update Fri Nov 29 15:27:18 2013 kevin soules
*/

#ifndef CONFIG_H_
# define CONFIG_H_

#include <arglists.h>

extern struct arglist *g_prefs;

struct arglist  *init_conf();
int	load_conf(char *filename, struct arglist *argl, int overwrite);
char    *conf_get_value(char *key);

#endif
