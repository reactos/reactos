/* $Id: dllmain.c,v 1.4 2002/08/15 16:11:01 dwelch Exp $
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

#define NDEBUG
#include <debug.h>


INT STDCALL
DllMain(PVOID hinstDll,
	ULONG dwReason,
	PVOID reserved)
{
   switch (dwReason)
     {
     case DLL_PROCESS_ATTACH:
	DisableThreadLibraryCalls(hinstDll);
	RegInitialize();
	break;

     case DLL_PROCESS_DETACH:
	RegCleanup();
	break;
     }

   return TRUE;
}

/* EOF */
