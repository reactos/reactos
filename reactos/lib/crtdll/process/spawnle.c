#include <crtdll/process.h>
#include <crtdll/stdlib.h>
#include <crtdll/stdarg.h>


int _spawnle(int mode, const char *path, const char *szArgv0, ... /*, const char **envp */)
{
  char *szArg[100];
  char *a;
  char *ptr;
  int i = 1;
  va_list l = 0;
  szArg[0]=(char*)szArgv0;
  va_start(l,szArgv0);
  do {
  	a = (char *)va_arg(l,const char *);
	szArg[i++] = (char *)a;
  } while ( a != NULL && i < 100 );

  if(a != NULL)
  {
//    __set_errno(E2BIG);
    return -1;
  }

  ptr = (char *)va_arg(l,const char *);

  return _spawnve(mode, path, (char * const *)szArg, (char * const *)ptr);
}
