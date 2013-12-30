/*
** kbs.c for kbs in /home/talanor/projects/pynasl
** 
** Made by quentin poirier
** Login   <poirie_q@epitech.net>
** 
** Started on  Wed Oct  9 13:46:14 2013 quentin poirier
** Last update Mon Dec 23 16:55:21 2013 ampotos
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syslog.h>
#include <glib.h>
#include "system.h"
#include "hosts_gatherer.h"
#include "nasl.h"
#include "log.h"
#include "locks.h"
#include "plugutils.h"

/************************************************************

 * UTILS

 ***********************************************************/

/**
 * Determines if a process is alive - as reliably as we can
 */
int
process_alive (pid_t pid)
{
  int i, ret;
  if (pid == 0)
    return 0;

  for (i = 0, ret = 1; (i < 10) && (ret > 0); i++)
    ret = waitpid (pid, NULL, WNOHANG);

  return kill (pid, 0) == 0;
}

static FILE *log;

/************************************************************

 * LOGS

 ***********************************************************/

/*
 * write into the logfile
 * Nothing fancy here...
 */
void
log_write (const char *str, ...)
{
  va_list param;
  char disp[4096];
  char *tmp;

  va_start (param, str);
  vsnprintf (disp, sizeof (disp), str, param);
  va_end (param);

  tmp = disp;
  while ((tmp = (char *) strchr (tmp, '\n')) != NULL)
    tmp[0] = ' ';


  if (log != NULL)
    {
      char timestr[255];
      time_t t;

      t = time (NULL);
      tmp = ctime (&t);

      timestr[sizeof (timestr) - 1] = '\0';
      strncpy (timestr, tmp, sizeof (timestr) - 1);
      timestr[strlen (timestr) - 1] = '\0';
      fprintf (log, "[%s][%d] %s\n", timestr, getpid (), disp);
    }
  else
    syslog (LOG_NOTICE, "%s", disp);
}

/************************************************************

 * LOCKS

 ***********************************************************/

static char *
file_lock_name (char *name)
{
  char *ret;
  char *t;

  if (!name)
    return NULL;

  ret = emalloc (strlen (name) * 2 + 6);
  name = strdup (name);
  t = strrchr (name, '/');
  if (t)
    {
      t[0] = '\0';
      sprintf (ret, "%s/.%s.lck", name, t + 1);
      t[0] = '/';
    }
  else
    sprintf (ret, ".%s.lck", name);

  efree (&name);
  return ret;
}

int
file_unlock (name)
     char *name;
{
  char *lock = file_lock_name (name);
  int e = 0;

  e = unlink (lock);
  efree (&lock);
  return e;
}

int
file_lock (name)
     char *name;
{
  char *lock = file_lock_name (name);
  int fd = -1;
  char buf[20];
  fd = open (lock, O_RDWR | O_CREAT | O_EXCL, 0600);
  efree (&lock);
  if (fd < 0)
    return -1;

  bzero (buf, sizeof (buf));
  snprintf (buf, sizeof (buf), "%d", getpid ());
  if (write (fd, buf, strlen (buf)) < 0)
    {
      close (fd);
      return -1;
    }
  close (fd);
  return 0;
}

int
file_locked (name)
     char *name;
{
  char *lock = file_lock_name (name);
  char asc_pid[20];
  int pid;
  int ret = 0;
  int fd = open (lock, O_RDONLY);
  if (fd < 0)
    {
      efree (&lock);
      return 0;
    }


  /*
   * We check that the process which set the
   * lock is still alive
   */
  bzero (asc_pid, sizeof (asc_pid));
  if (read (fd, asc_pid, sizeof (asc_pid) - 1) < 0)
    {
      log_write ("Could not determine if the file %s is locked: Failed to read %s\n",
                 name, lock);
      efree (&lock);
      close (fd);
      return 0;
    }

  close (fd);
  pid = atoi (asc_pid);
  if (process_alive (pid))
    {
      log_write
        ("The file %s is locked by process %d. Delete %s if you think this is incorrect\n",
         name, pid, lock);
      ret = 1;
    }
  else
    file_unlock (name);

  efree (&lock);
  return ret;
}

/************************************************************

 * KBS

 *************************************************************/

/**
   FIXME : this lib is here for development purpose, it is only a c/c of some functions
   of openvas-scanner. It should be removed in a future version of the development.
   This file is not guaranteed to work with all versions of openvas..
   We must recode a library to load symbols from an ELF binary (check dlfcn from libC)
 */

char *
user_home (struct arglist *globals)
{
  char *user = arg_get_value (globals, "user");
  char *ret;

  if (!user)
    return NULL;

 /** @todo consider using glib functions */
  ret = emalloc (strlen (OPENVAS_USERS_DIR) + strlen (user) + 2);
  sprintf (ret, "%s/%s", OPENVAS_USERS_DIR, user);

  return ret;
}

/**
 * @brief Replaces slashes in name by underscores (in-place).
 *
 * @param name String in which slashes will be replaced by underscores.
 *
 * @return Pointer to the parameter name string.
 */
static char *
filter_odd_name (char *name)
{
  char *ret = name;
  while (name[0])
    {
      /* A host name should never contain any slash. But we never  know */
      if (name[0] == '/')
        name[0] = '_';
      name++;
    }
  return ret;
}

/**
 * Returns name of the directory which contains the sessions of the current
 * user (/path/to/var/lib/openvas/<username>/kbs/).
 *
 * @return Path to knowledge base directory for current user, has to be freed
 *         using g_free.
 */
static gchar *
kb_dirname (struct arglist *globals)
{
  return g_build_filename (user_home (globals), "kbs", NULL);
}

/**
 * @brief Returns file name where the kb for scan of a host can be saved/read
 * @brief from.
 *
 * From \<hostname\>, return /path/to/var/lib/openvas/\<username\>/kbs/\<hostname\> .
 */
static char *
kb_fname (struct arglist *globals, char *hostname)
{
  gchar *dir = kb_dirname (globals);
  char *ret;
  char *hn = strdup (hostname);

  hn = filter_odd_name (hn);

 /** @todo use glibs *build_path functions */
  ret = emalloc (strlen (dir) + strlen (hn) + 2);
  sprintf (ret, "%s/%s", dir, hn);
  g_free (dir);
  efree (&hn);
  return ret;
}

/**
 * Create a kb directory.
 * XXXXX does not check for the existence of a directory and does
 * not check any error
 */
static int
kb_mkdir (char *dir)
{
  char *t;
  int ret = 0;

  dir = estrdup (dir);
  t = strchr (dir + 1, '/');
  while (t)
    {
      t[0] = '\0';
      mkdir (dir, 0700);
      t[0] = '/';
      t = strchr (t + 1, '/');
    }

  if ((ret = mkdir (dir, 0700)) < 0)
    {
      if (errno != EEXIST)
        log_write ("mkdir(%s) failed : %s\n", dir, strerror (errno));
      efree (&dir);
      return ret;
    }
  efree (&dir);
  return ret;
}

// TODO : test the effect of this function

/**
 * @brief Initialize a new KB that will be saved.
 *
 * The indices of all the opened KB are in a hashlist in
 * globals, saved under the name "save_kb". This makes no sense
 * at this time, as the test of each host is done in a separate
 * process, but this allows us to regroup easily these in
 * the future.
 */
int
save_kb_new (struct arglist *globals, char *hostname, char* kbname)
{
  char *fname;
  char *dir;
  char *user = arg_get_value (globals, "user");
  int ret = 0;
  int f;
  int f_test;

  if (hostname == NULL)
    return -1;
  if (kbname == NULL)
    {
      dir = kb_dirname (globals);
      kb_mkdir (dir);
      efree (&dir);

      fname = kb_fname (globals, hostname);
    }
  else
    fname = strdup(kbname);
  if (file_locked (fname))
    {
      efree (&fname);
      return 0;
    }
  unlink (fname);               /* delete the previous kb */
  f = open (fname, O_CREAT | O_RDWR | O_EXCL, 0640);
  if (f < 0)
    {
      log_write ("user %s : Can not save KB for %s - %s", user, hostname,
                 strerror (errno));
      ret = -1;
      efree (&fname);
      return ret;
    }
  else
    {
      file_lock (fname);
      log_write ("user %s : new KB will be saved as %s", user, fname);
      if (arg_get_value (globals, "save_kb"))
        arg_set_value (globals, "save_kb", sizeof (gpointer),
                       GSIZE_TO_POINTER (f));
      else
        arg_add_value (globals, "save_kb", ARG_INT, sizeof (gpointer),
                       GSIZE_TO_POINTER (f));
    }
  return 0;
}

void
save_kb_close (struct arglist *globals, char *hostname, char* kb_file)
{
  int fd = GPOINTER_TO_SIZE (arg_get_value (globals, "save_kb"));
  char *fname = kb_fname (globals, hostname);
  if (kb_file)
    {
      efree(&fname);
      fname = strdup(kb_file);
    }
  if (fd > 0)
    close (fd);
  file_unlock (fname);
  efree (&fname);
}

/****
     SAVE_KB
 */

/*
 * mmap() tends to sometimes act weirdly
 */
static char *
map_file (int file)
{
  struct stat st;
  char *ret;
  int i = 0;
  int len;

  bzero (&st, sizeof (st));
  fstat (file, &st);
  len = (int) st.st_size;
  if (len == 0)
    return NULL;

  lseek (file, 0, SEEK_SET);
  ret = emalloc (len + 1);
  while (i < len)
    {
      int e = read (file, ret + i, len - i);
      if (e > 0)
        i += e;
      else
        {
          log_write ("read(%d, buf, %d) failed : %s\n", file, len,
                     strerror (errno));
          efree (&ret);
          lseek (file, len, SEEK_SET);
          return NULL;
        }
    }

  lseek (file, len, SEEK_SET);
  return ret;
}

static int
save_kb_rm_entry_value (struct arglist *globals, char *hostname, char *name,
                        char *value)
{
  char *buf;
  char *t;
  int fd;
  char *req;


  fd = GPOINTER_TO_SIZE (arg_get_value (globals, "save_kb"));
  if (fd <= 0)
    return -1;

  buf = map_file (fd);
  if (buf)
    {
      if (value)
        {
          req = emalloc (strlen (name) + strlen (value) + 2);
          sprintf (req, "%s=%s", name, value);
        }
      else
        req = estrdup (name);

      t = strstr (buf, req);
      if (t)
        {
          char *end;

          while (t[0] != '\n')
            {
              if (t == buf)
                break;
              else
                t--;
            }

          if (t[0] == '\n')
            t++;
          end = strchr (t, '\n');
          t[0] = '\0';
          if (end)
            {
              end[0] = '\0';
              end++;
            }

          if ((lseek (fd, 0, SEEK_SET)) < 0)
            {
              log_write ("lseek() failed - %s\n", strerror (errno));
            }

          if ((ftruncate (fd, 0)) < 0)
            {
              log_write ("ftruncate() failed - %s\n", strerror (errno));
            }


          if (write (fd, buf, strlen (buf)) < 0)
            {
              log_write ("write() failed - %s\n", strerror (errno));
            }

          if (end)
            {
              if ((write (fd, end, strlen (end))) < 0)
                log_write ("write() failed - %s\n", strerror (errno));
            }
        }
      efree (&buf);
      efree (&req);
      lseek (fd, 0, SEEK_END);
    }
  return 0;
}

static int
save_kb_rm_entry (struct arglist *globals, char *hostname, char *name)
{
  return save_kb_rm_entry_value (globals, hostname, name, NULL);
}

static int
save_kb_entry_present_already (struct arglist *globals, char *hostname,
                               char *name, char *value)
{
  char *buf;
  int fd;
  char *req;
  int ret;

  fd = GPOINTER_TO_SIZE (arg_get_value (globals, "save_kb"));
  if (fd <= 0)
    return -1;

  buf = map_file (fd);
  if (buf)
    {
      req = emalloc (strlen (name) + strlen (value) + 2);
      sprintf (req, "%s=%s", name, value);
      if (strstr (buf, req))
        ret = 1;
      else
        ret = 0;
      efree (&buf);
      efree (&req);
      return ret;
    }
  return -1;
}

int
save_kb_write (struct arglist *globals, char *hostname, char *name, char *value,
               int type)
{
  int fd;
  char *str;
  int e;
  struct timeval now;

  if (!globals || !hostname || !name || !value)
    return -1;

  fd = GPOINTER_TO_SIZE (arg_get_value (globals, "save_kb"));
  if (fd < 0)
    {
      fprintf (stderr, "user %s : Can not find KB fd for %s\n",
                 (char *) arg_get_value (globals, "user"), hostname);
      return -1;
    }

  /* Skip temporary KB entries */
  if (!strncmp (name, "/tmp/", 4) || !strncmp (name, "NIDS/", 5)
      || !strncmp (name, "Settings/", 9))
    return 0;

  /* Don't save sensitive information */
  if (strncmp (name, "Secret/", 7) == 0)
    return 0;

  /* Avoid duplicates for these families */
  if (!strncmp (name, "Success/", strlen ("Success/"))
      || !strncmp (name, "Launched/", strlen ("Launched/"))
      || !strncmp (name, "SentData/", strlen ("SentData/")))
    {
      save_kb_rm_entry (globals, hostname, name);
    }

  if (save_kb_entry_present_already (globals, hostname, name, value))
    {
      save_kb_rm_entry_value (globals, hostname, name, value);
    }

  str = emalloc (strlen (name) + strlen (value) + 25);
  gettimeofday (&now, NULL);
  sprintf (str, "%ld %d %s=%s\n", (long) now.tv_sec, type, name, value);

  /** @todo Fix a bug (most probably race condition). Although following write
   * call does return > 0, sometimes the content never reaches the file,
   * especially for big amount of data in value (e.g. big file contents) */
  e = write (fd, str, strlen (str));
  if (e < 0)
    {
      fprintf (stderr, "user %s : write kb error - %s\n",
                 (char *) arg_get_value (globals, "user"), strerror (errno));
    }
  efree (&str);
  return 0;
}

int
save_kb_write_str (struct arglist *globals, char *hostname, char *name,
                   char *value)
{
  char *newvalue = addslashes (value);
  int e;

  e = save_kb_write (globals, hostname, name, newvalue, ARG_STRING);
  efree (&newvalue);
  return e;
}


int
save_kb_write_int (struct arglist *globals, char *hostname, char *name,
                   int value)
{
  static char asc_value[25];
  int e;
  sprintf (asc_value, "%d", value);
  e = save_kb_write (globals, hostname, name, asc_value, ARG_INT);
  bzero (asc_value, sizeof (asc_value));
  return e;
}
