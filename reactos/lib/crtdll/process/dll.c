#include <windows.h>
#include <crtdll/process.h>

void *_loaddll (char *name)
{
	return LoadLibraryA(name);
}

int _unloaddll(void *handle)
{
	return FreeLibrary(handle);
}

FARPROC _getdllprocaddr(void *hModule,char * lpProcName, int iOrdinal)
{
   

	if ( lpProcName != NULL ) 
		return GetProcAddress(hModule, lpProcName);
	else
		return GetProcAddress(hModule, (LPSTR)iOrdinal);
   	return (NULL);
}
