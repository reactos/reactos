/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */


#include <process.h>

int execvpe(const char *path,const char * const argv[],const char * const envp[])
{
  return spawnvpe(P_OVERLAY, path, argv, envp);
}


int _execvpe(const char *path,const char * const argv[],const char * const envp[])
{
  return spawnvpe(P_OVERLAY, path, argv, envp);
}