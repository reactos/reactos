/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <process.h>
#include <errno.h>


int _spawnvpe(int mode, const char *path,const char *const argv[],const char *const envp[])
{

  char rpath[300];
  return spawnve(mode, rpath, argv, envp);

}
