/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/process/dll.c
 * PURPOSE:     Dll support routines
 * PROGRAMER:   Ariadne
 * UPDATE HISTORY:
 *              04/03/99: Created
 */

#include <precomp.h>

/*
 * @implemented
 */
intptr_t _loaddll(char* name)
{
	return (intptr_t) LoadLibraryA(name);
}

/*
 * @implemented
 */
int _unloaddll(intptr_t handle)
{
	return FreeLibrary((HMODULE) handle);
}

/*
 * @implemented
 */
FARPROC _getdllprocaddr(intptr_t hModule, char* lpProcName, intptr_t iOrdinal)
{
	if (lpProcName != NULL)
		return GetProcAddress((HMODULE) hModule, lpProcName);
	else
		return GetProcAddress((HMODULE) hModule, (LPSTR)iOrdinal);
   	return (NULL);
}
