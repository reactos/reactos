/* $Id: dllmain.c,v 1.3 2002/08/29 23:57:54 hyperion Exp $
 * 
 * ReactOS PSAPI.DLL
 */
#include <windows.h>
#include <ntdll/ldr.h>

BOOLEAN STDCALL DllMain
(
 PVOID hinstDll,
 ULONG dwReason,
 PVOID reserved
)
{
 if(dwReason == DLL_PROCESS_ATTACH)
  /* don't bother calling the entry point on thread startup - PSAPI.DLL doesn't
     store any per-thread data */
  LdrDisableThreadCalloutsForDll(hinstDll);

 return (TRUE);
}

/* EOF */
