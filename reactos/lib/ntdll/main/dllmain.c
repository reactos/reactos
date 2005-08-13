/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/ntdll/main/dllmain.c
 * PURPOSE:
 * PROGRAMMER:
 */

#include <ntdll.h>
#define NDEBUG
#include <debug.h>

BOOL WINAPI DllMainCRTStartup(HINSTANCE hinstDll,
			      DWORD fdwReason,
			      LPVOID fImpLoad)
{
  return TRUE;
}

/* EOF */
