/* $Id: dllmain.c,v 1.9 2002/09/07 15:12:39 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/ntdll/main/dllmain.c
 * PURPOSE:         
 * PROGRAMMER:      
 */

#include <windows.h>
#define NTOS_USER_MODE
#include <ntos.h>
#include <stdarg.h>
#include <stdio.h>

#define NDEBUG
#include <debug.h>

BOOL WINAPI DllMainCRTStartup(HINSTANCE hinstDll,
			      DWORD fdwReason,
			      LPVOID fImpLoad)
{
  return TRUE;
}

/* EOF */
