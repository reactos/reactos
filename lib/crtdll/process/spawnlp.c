/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <msvcrt/process.h>
#include <msvcrt/stdlib.h>
#include <stdarg.h>

/*
 * @implemented
 */
int _spawnlp(int nMode, const char* szPath, const char* szArgv0, ...)
{
  char *szArg[100];
  const char *a;
  int i = 0;
  va_list l = 0;
  va_start(l,szArgv0);
  do {
  	a = (const char *)va_arg(l,const char *);
	szArg[i++] = (char *)a;
  } while ( a != NULL && i < 100 );
  return _spawnvpe(nMode, szPath,szArg, _environ);
}
