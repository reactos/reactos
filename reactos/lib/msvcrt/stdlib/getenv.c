#include <windows.h>
#include <msvcrt/stdlib.h>


char *getenv(const char *name)
{
	char *buffer;
	buffer = (char *)malloc(MAX_PATH);
	buffer[0] = 0;
	if ( GetEnvironmentVariableA(name,buffer,MAX_PATH) == 0 )
		return NULL;
	return buffer;
}

wchar_t *_wgetenv(const wchar_t *name)
{
	wchar_t *buffer;
	buffer = (wchar_t *)malloc(MAX_PATH * sizeof(wchar_t));
	buffer[0] = 0;
	if ( GetEnvironmentVariableW(name,buffer,MAX_PATH) == 0 )
		return NULL;
	return buffer;
}
