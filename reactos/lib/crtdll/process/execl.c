/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <process.h>

extern char *const *_environ;

int _execl(const char *path, const char *argv0, ...)
{
  return _spawnve(P_OVERLAY, path, (const char *const*)&argv0,(const char *const*) _environ);
}
