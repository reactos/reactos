/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <process.h>
#include <errno.h>


int spawnvpe(int mode, const char *path,const char *const argv[],const char *const envp[])
{

  char rpath[300];
  union {const char * const *cpcp; char **cpp; } u;
  u.cpcp = envp;
/*
  if (!__dosexec_find_on_path(path, u.cpp, rpath))
  {
    errno = ENOENT;
    return -1;
  }
  else
*/
    return spawnve(mode, rpath, argv, envp);

}
