/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/ntdll/main/dllmain.c
 * PURPOSE:
 * PROGRAMMER:
 */

#define NDEBUG
#include <ntdll.h>

BOOL WINAPI DllMainCRTStartup(HINSTANCE hinstDll,
			      DWORD fdwReason,
			      LPVOID fImpLoad)
{
  return TRUE;
}

/* EOF */
