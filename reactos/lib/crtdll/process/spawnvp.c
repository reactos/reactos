/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <process.h>
#include <stdlib.h>

int spawnvp(int mode, const char *path,const char *const argv[])
{
  return spawnvpe(mode, path, (char * const *)argv, _environ);
}
