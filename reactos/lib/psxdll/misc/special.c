/* $Id: special.c,v 1.2 1999/10/12 21:17:05 ea Exp $
 * 
 * reactos/subsys/psxdll/misc/special.c
 * 
 * ReactOS Operating System
 * - POSIX+ client side DLL (ReactOS POSIX+ Subsystem)
 */
#define NTOS_MODE_USER
#include <ntos.h>


/* Unknown */
typedef 
struct _INITIALIZATION_DATA
{
	DWORD	Unknown0;
	DWORD	Unknown1;

} INITIALIZATION_DATA, * PINITIALIZATION_DATA;


INITIALIZATION_DATA
InitializationData = { 0, 0 };


VOID
STDCALL
__PdxInitializeData (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	InitializationData.Unknown0 = Unknown0;	
	InitializationData.Unknown1 = Unknown1;	
	return;
}


LPWSTR
STDCALL
__PdxGetCmdLine (
	VOID
	)
{
	return NULL;
}


/* EOF */
