/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/process/dll.c
 * PURPOSE:     Dll support routines
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              04/03/99: Created
 */

#include <windows.h>
#include <msvcrt/process.h>


void* _loaddll(char* name)
{
	return LoadLibraryA(name);
}

int _unloaddll(void* handle)
{
	return FreeLibrary(handle);
}

FARPROC _getdllprocaddr(void* hModule, char* lpProcName, int iOrdinal)
{
	if (lpProcName != NULL) 
		return GetProcAddress(hModule, lpProcName);
	else
		return GetProcAddress(hModule, (LPSTR)iOrdinal);
   	return (NULL);
}
