/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details *///#include <crtdll/stubs.h>
//#include <crtdll/unistd.h>
#include <crtdll/process.h>
#include <crtdll/stdlib.h>

int _execvp(const char* szPath, char* const* szaArgv)
{
  return _spawnvpe(P_OVERLAY, szPath, szaArgv, _environ);
}
