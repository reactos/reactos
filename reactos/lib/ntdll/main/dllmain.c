/* $Id: dllmain.c,v 1.6 2000/01/18 12:04:45 ekohl Exp $
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


BOOL WINAPI DllMainCRTStartup(HINSTANCE hinstDll, 
			      DWORD fdwReason, 
			      LPVOID fImpLoad)
{
  return TRUE;
}

/* EOF */
