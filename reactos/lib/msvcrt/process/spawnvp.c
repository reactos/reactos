/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <msvcrti.h>


int _spawnvp(int nMode, const char* szPath, char* const* szaArgv)
{
  return _spawnvpe(nMode, szPath, (char * const *)szaArgv, _environ);
}
