/* $Id: dllmain.c,v 1.1 2001/11/28 23:35:16 ea Exp $
 * 
 * ReactOS PSAPI.DLL
 */
#include <windows.h>
BOOLEAN __stdcall DllMain(
	PVOID	hinstDll,
	ULONG	dwReason,
	PVOID	reserved)
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
