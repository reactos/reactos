/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <msvcrti.h>


int _spawnl(int nMode, const char* szPath, const char* szArgv0,...)
{
  char *szArg[100];
  const char *a;
  int i = 1;
  va_list l = 0;
  szArg[0]=(char*)szArgv0;
  va_start(l,szArgv0);
  do {
  	a = va_arg(l,const char *);
	szArg[i++] = (char *)a;
  } while ( a != NULL && i < 100 );
  
  return _spawnve(nMode, szPath, szArg, _environ);
}
