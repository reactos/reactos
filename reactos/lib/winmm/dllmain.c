/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Media API DLL
 * FILE:        dllmain.c
 * PURPOSE:     DLL entry
 * PROGRAMMERS: Steven Edwards (isolation@users.sourceforge.net)
 * REVISIONS:
 *   SAE 9-24-02 Created
 */

#include <ddk/ntddk.h>
#include <windows.h>

//#define NDEBUG
#include <debug.h>

INT STDCALL
DllMain(PVOID hinstDll,
	ULONG dwReason,
	PVOID reserved)
{
  DbgPrint("WINMM: DllMain() called\n");

  switch (dwReason)
  {
  case DLL_PROCESS_ATTACH:
    break;

  case DLL_PROCESS_DETACH:
    break;
  }

  DbgPrint("WINMM: DllMain() done\n");

  return TRUE;
}
