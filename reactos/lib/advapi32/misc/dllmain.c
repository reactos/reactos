/* $Id: dllmain.c,v 1.5 2002/09/07 15:12:22 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/misc/dllmain.c
 * PURPOSE:         Library main function
 * PROGRAMMER:      ???
 * UPDATE HISTORY:
 *                  Created ???
 */

#include <advapi32.h>

#define NDEBUG
//#include <debug.h>

#if 0
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
#endif
/* EOF */
