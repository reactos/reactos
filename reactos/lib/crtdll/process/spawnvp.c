/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <crtdll/process.h>
#include <crtdll/stdlib.h>

int _spawnvp(int nMode, const char* szPath, char* const* szaArgv)
{
  return spawnvpe(nMode, szPath, (char * const *)szaArgv, _environ);
}
