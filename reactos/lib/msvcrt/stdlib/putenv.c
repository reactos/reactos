#include <msvcrti.h>

#define NDEBUG
#include <msvcrtdbg.h>


extern int BlockEnvToEnviron(); // defined in misc/dllmain.c

int _putenv(const char *val)
{
  char *buffer;
  char *epos;
  int res;

  DPRINT("_putenv('%s')\n", val);
  epos = strchr(val, '=');
  if ( epos == NULL )
	return -1;
  buffer = (char*)malloc(epos - val + 1);
  if (buffer == NULL)
	return -1;
  strncpy(buffer, val, epos - val);
  buffer[epos - val] = 0;
  res = SetEnvironmentVariableA(buffer,epos+1);
  free(buffer);
  if (BlockEnvToEnviron()) return 0;
  return  res;
}

int _wputenv(const wchar_t *val)
{
  wchar_t *buffer;
  wchar_t *epos;
  int res;

  DPRINT("_wputenv('%S')\n", val);
  epos = wcsrchr(val, L'=');
  if ( epos == NULL )
	return -1;
  buffer = (wchar_t*)malloc((epos - val + 1) * sizeof (wchar_t));
  if (buffer == NULL)
	return -1;
  wcsncpy(buffer, val, epos - val);
  buffer[epos - val] = 0;
  res = SetEnvironmentVariableW(buffer,epos+1);
  free(buffer);
  if (BlockEnvToEnviron() ) return 0;
  return  res;
}
