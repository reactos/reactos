/* $Id: dllmain.c,v 1.2 2000/04/25 23:22:53 ea Exp $
 * 
 * ReactOS FMIFS.DLL
 */
#include <windows.h>
BOOLEAN
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
	return (TRUE);
}
/* EOF */
