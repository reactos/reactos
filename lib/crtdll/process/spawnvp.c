/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <msvcrt/process.h>
#include <msvcrt/stdlib.h>

/*
 * @implemented
 */
int _spawnvp(int nMode, const char* szPath, char* const* szaArgv)
{
  return spawnvpe(nMode, szPath, (char * const *)szaArgv, _environ);
}
