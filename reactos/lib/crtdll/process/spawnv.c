/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <process.h>

extern char **_environ;

int spawnv(int mode, const char *path,const char *const argv[])
{
  return spawnve(mode, path, (char * const *)argv, _environ);
}
