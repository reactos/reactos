#include <windows.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/string.h>


int _putenv(const char *val)
{
  char buffer[1024];
  char *epos;

  strcpy(buffer,val);
  epos = strchr(buffer, '=');
  if ( epos == NULL )
	return -1;

  *epos = 0;

  return SetEnvironmentVariableA(buffer,epos+1);
}

int _wputenv(const wchar_t *val)
{
  wchar_t buffer[1024];
  wchar_t *epos;

  wcscpy(buffer,val);
  epos = wcschr(buffer, L'=');
  if ( epos == NULL )
	return -1;

  *epos = 0;

  return SetEnvironmentVariableW(buffer,epos+1);
}
