/* $Id: userenv.c,v 1.2 2004/03/13 20:49:07 ekohl Exp $ 
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/userenv/userenv.c
 * PURPOSE:         DLL initialization code
 * PROGRAMMER:      Eric Kohl
 */

#include <ntos.h>
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
