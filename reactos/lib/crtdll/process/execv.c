/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <process.h>

extern char * const *_environ;

int _execv(const char *path, const char * const *argv)
{
  return _spawnve(P_OVERLAY, path, argv,(const char *const*) _environ);
}
