/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <crtdll/process.h>
#include <crtdll/stdlib.h>
#include <stdarg.h>

int _spawnl(int nMode, const char* szPath, const char* szArgv0,...)
{
  va_list a = 0;
  va_start(a,szArgv0);
  return _spawnve(nMode, szPath, (char * const *)a, _environ);
}
