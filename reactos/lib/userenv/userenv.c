/* $Id: userenv.c,v 1.1 2004/01/09 19:52:01 ekohl Exp $ 
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/userenv/userenv.c
 * PURPOSE:         DLL initialization code
 * PROGRAMMER:      Eric Kohl
 */

#include <windows.h>
#include <userenv.h>

#include "internal.h"


BOOL WINAPI
DllMain (HINSTANCE hinstDLL,
         DWORD fdwReason,
         LPVOID lpvReserved)
{
  if (fdwReason == DLL_PROCESS_ATTACH)
    {
    }
  else if (fdwReason == DLL_PROCESS_DETACH)
    {
    }

  return TRUE;
}
