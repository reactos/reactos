/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details *///#include <libc/stubs.h>
//#include <unistd.h>
#include <process.h>
#include <stdlib.h>

int _execvp(const char *path,const char * const argv[])
{
  return _spawnvpe(P_OVERLAY, path, argv, _environ);
}
