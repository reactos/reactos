/* $Id: dllmain.c,v 1.7 2000/06/29 23:35:28 dwelch Exp $
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
