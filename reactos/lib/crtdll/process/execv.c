/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <process.h>
#include <stdlib.h>

int _execv(const char *path, const char * const *argv)
{
  return _spawnve(P_OVERLAY, path, argv, _environ);
}
