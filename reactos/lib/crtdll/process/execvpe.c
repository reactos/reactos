/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */


#include <process.h>

int _execvpe(const char *path,const char * const argv[],const char * const envp[])
{
  return _spawnvpe(P_OVERLAY, path, argv, envp);
}


