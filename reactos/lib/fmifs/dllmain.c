/* $Id: dllmain.c,v 1.1 1999/05/11 21:19:41 ea Exp $
 * 
 * ReactOS FMIFS.DLL
 */
#include <windows.h>
INT
__stdcall
DllMain(
	PVOID	hinstDll,
	ULONG	dwReason,
	PVOID	reserved
	)
{
	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			break;
	}
	return(1);
}
/* EOF */
