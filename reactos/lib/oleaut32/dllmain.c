/* $Id: dllmain.c,v 1.1 2001/07/16 01:45:43 rex Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/oldaut32/misc/dllmain.c
 * PURPOSE:         Library main function
 * PROGRAMMER:      Rex Jolliff (rex@lvcablemodem.com)
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
  DPRINT("OLEAUT32: DllMain() called\n");

  switch (dwReason)
  {
  case DLL_PROCESS_ATTACH:
    break;

  case DLL_PROCESS_DETACH:
    break;
  }

  DPRINT("OLEAUT32: DllMain() done\n");

  return TRUE;
}


