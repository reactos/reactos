/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <crtdll/process.h>
#include <crtdll/stdlib.h>
#include <stdarg.h>

int _spawnl(int nMode, const char* szPath, const char* szArgv0,...)
{
  char *szArg[100];
  const char *a;
  int i = 0;
  va_list l = 0;
  va_start(l,szArgv0);
  do {
  	a = va_arg(l,const char *);
	szArg[i++] = (char *)a;
  } while ( a != NULL && i < 100 );
  
  return _spawnve(nMode, szPath, szArg, _environ);
}
