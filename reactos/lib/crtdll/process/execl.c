/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <process.h>

extern char *const *_environ;

int execl(const char *path, const char *argv0, ...)
{
  return spawnve(P_OVERLAY, path, (char *const*)&argv0, _environ);
}
