/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
//#include <libc/stubs.h>
//#include <unistd.h>
#include <process.h>

extern char * const *_environ;

int execv(const char *path, const char * const *argv)
{
  return spawnve(P_OVERLAY, path, argv, _environ);
}
