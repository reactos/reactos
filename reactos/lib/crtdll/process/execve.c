/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */

#include <process.h>

int _execve(const char *path,const  char * const argv[], char * const envp[])
{
  return _spawnve(P_OVERLAY, path, argv, envp);
}
