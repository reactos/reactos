/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrti.h>


int _execve(const char* szPath, char* const* szaArgv, char* const* szaEnv)
{
  return _spawnve(P_OVERLAY, szPath, szaArgv, szaEnv);
}
