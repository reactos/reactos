#include <windows.h>
#include <process.h>

void *_loaddll (char *name)
{
	return LoadLibraryA(name);
}

int _unloaddll(void *handle)
{
	return FreeLibrary(handle);
}