/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <msvcrt/process.h>
#include <msvcrt/errno.h>


int _spawnvpe(int nMode, const char* szPath, char* const* szaArgv, char* const* szaEnv)
{

  return spawnve(nMode, szPath, szaArgv, szaEnv);

}
