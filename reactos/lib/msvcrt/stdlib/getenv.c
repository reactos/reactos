#include <windows.h>
#include <msvcrt/stdlib.h>

#define NDEBUG
#include <msvcrt/msvcrtdbg.h>


char *getenv(const char *name)
{
	char *buffer = (char*)0xffffffff;
	int len = GetEnvironmentVariableA(name,buffer,0) + 1;
	DPRINT("getenv(%s)\n", name);
	buffer = (char *)malloc(len);
	DPRINT("getenv('%s') %d %x\n", name, len, buffer);
	if (buffer == NULL || GetEnvironmentVariableA(name,buffer,len) == 0 )
	{
		free(buffer);
		return NULL;
	}
	return buffer;
}

wchar_t *_wgetenv(const wchar_t *name)
{
	wchar_t *buffer = (wchar_t*)0xffffffff;
	int len = GetEnvironmentVariableW(name, buffer,0) + 1;
	DPRINT("_wgetenv(%S)\n", name);
	buffer = (wchar_t *)malloc(len * sizeof(wchar_t));
	if (buffer == NULL || GetEnvironmentVariableW(name,buffer,len) == 0)
	{
		free(buffer);
		return NULL;
	}
	return buffer;
}
