#include <windows.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/string.h>


extern int BlockEnvToEnviron(); // defined in misc/dllmain.c

int _putenv(const char *val)
{
  char buffer[1024];
  char *epos;
  int res;

  strcpy(buffer,val);
  epos = strchr(buffer, '=');
  if ( epos == NULL )
	return -1;

  *epos = 0;

  res = SetEnvironmentVariableA(buffer,epos+1);
  if (BlockEnvToEnviron() ) return 0;
  return  res;
}

int _wputenv(const wchar_t *val)
{
  wchar_t buffer[1024];
  wchar_t *epos;
  int res;

  wcscpy(buffer,val);
  epos = wcschr(buffer, L'=');
  if ( epos == NULL )
	return -1;

  *epos = 0;

  res = SetEnvironmentVariableW(buffer,epos+1);
  if (BlockEnvToEnviron() ) return 0;
  return  res;
}
