/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/ntdll/main/dllmain.c
 * PURPOSE:         
 * PROGRAMMER:      
 */

#include <ddk/ntddk.h>
#include <stdarg.h>
#include <stdio.h>
#include <ntdll/ntdll.h>
#include <windows.h>

BOOL WINAPI DllMainCRTStartup(HINSTANCE hinstDll,
			      DWORD fdwReason,
			      LPVOID fImpLoad)
{
  return TRUE;
}

/* EOF */
