/* $Id: dllmain.c,v 1.1 1999/09/12 21:54:16 ea Exp $
 * 
 * reactos/subsys/psxdll/dllmain.c
 * 
 * ReactOS Operating System
 * - POSIX+ client side DLL (ReactOS POSIX+ Subsystem)
 */
#include <windows.h>

BOOL
WINAPI
DllMain (
	HINSTANCE	hinstDll, 
	DWORD		fdwReason, 
	LPVOID		fImpLoad
	)
{
	/* FIXME: Connect to psxss.exe */
	return TRUE;
}


