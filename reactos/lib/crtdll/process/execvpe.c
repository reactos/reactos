/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */


#include <msvcrt/process.h>

/*
 * @implemented
 */
int _execvpe(const char* szPath, char* const* szaArgv, char* const* szaEnv)
{
  return _spawnvpe(P_OVERLAY, szPath, szaArgv, szaEnv);
}


