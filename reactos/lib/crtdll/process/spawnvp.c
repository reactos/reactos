/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <process.h>

extern char *_environ[];

int _spawnvp(int mode, const char *path,const char *const argv[])
{
  return _spawnvpe(mode, path, (const char * const *)argv,(const char * const *) _environ);
}
