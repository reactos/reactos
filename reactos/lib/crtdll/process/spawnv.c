/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <process.h>
#include <stdlib.h>


int _spawnv(int mode, const char *path,const char *const argv[])
{
  return _spawnve(mode, path, (char * const *)argv, _environ);
}
