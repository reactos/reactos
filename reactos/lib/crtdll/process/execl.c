/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <process.h>
#include <stdlib.h>

int _execl(const char *path, const char *argv0, ...)
{
  return _spawnve(P_OVERLAY, path, (char *const*)&argv0, _environ);
}
