/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
//#include <libc/stubs.h>
//#include <unistd.h>
#include <process.h>

extern char *const *_environ;

int execvp(const char *path,const char * const argv[])
{
  return spawnvpe(P_OVERLAY, path, argv, _environ);
}
