/* $Id: dllmain.c,v 1.1 2000/06/18 10:57:42 ea Exp $
 * 
 * ReactOS MSVCRT.DLL Compatibility Library
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
