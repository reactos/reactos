/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <process.h>

extern char *const *_environ;

int _execvp(const char *path,const char * const argv[])
{
  return _spawnvpe(P_OVERLAY,path,(const char *const*) argv,(const char *const*) _environ);
}
