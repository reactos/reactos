/* $Id: dllmain.c,v 1.3 2001/06/17 20:20:21 ekohl Exp $
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
   DPRINT("ADVAPI32: DllMain() called\n");

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

   DPRINT1("ADVAPI32: DllMain() done\n");

   return TRUE;
}

/* EOF */
