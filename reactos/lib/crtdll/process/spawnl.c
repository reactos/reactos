/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <process.h>

extern char *_environ[];

int _spawnl(int mode, const char *path, const char *argv0, ...)
{
  return _spawnve(mode, path, (const char * const *)&argv0,(const char *const *) _environ);
}
