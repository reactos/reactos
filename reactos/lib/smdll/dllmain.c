/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS
 * FILE:            lib/smdll/dllmain.c
 * PURPOSE:         SM Helper Library
 */

#define NTOS_MODE_USER
#include <ntos.h>

BOOL STDCALL DllMain(HANDLE hinstDll, DWORD fdwReason, LPVOID fImpLoad)
{
  return TRUE;
}

/* EOF */
