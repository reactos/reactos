/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <process.h>

extern char **_environ;

int _spawnl(int mode, const char *path, const char *argv0, ...)
{
  return spawnve(mode, path, (char * const *)&argv0, _environ);
}
