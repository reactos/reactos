/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details *///#include <crtdll/stubs.h>

#include <msvcrt/process.h>
#include <msvcrt/stdlib.h>

int _execvp(const char* szPath, char* const* szaArgv)
{
  return _spawnvpe(P_OVERLAY, szPath, szaArgv, _environ);
}
