/* $Id: dllmain.c,v 1.1 2001/07/06 02:47:17 rex Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/shell32/misc/dllmain.c
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
  DPRINT("SHELL32: DllMain() called\n");

  switch (dwReason)
  {
  case DLL_PROCESS_ATTACH:
    break;

  case DLL_PROCESS_DETACH:
    break;
  }

  DPRINT1("SHELL32: DllMain() done\n");

  return TRUE;
}


