/* $Id: dllmain.c,v 1.2 2000/09/05 22:59:58 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/misc/dllmain.c
 * PURPOSE:         Library main function
 * PROGRAMMER:      ???
 * UPDATE HISTORY:
 *                  Created ???
 */

#include <ddk/ntddk.h>
#include <windows.h>

//#define NDEBUG
#include <debug.h>


INT
STDCALL
DllMain(
	PVOID	hinstDll,
	ULONG	dwReason,
	PVOID	reserved
	)
{
DPRINT1("ADVAPI32: DllMain() called\n");
	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls (hinstDll);
			RegInitialize ();
			break;

		case DLL_PROCESS_DETACH:
			RegCleanup ();
			break;
	}

DPRINT1("ADVAPI32: DllMain() done\n");

	return TRUE;
}

/* EOF */
