/* $Id: userenv.c,v 1.3 2004/08/15 19:02:40 chorns Exp $ 
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/userenv/userenv.c
 * PURPOSE:         DLL initialization code
 * PROGRAMMER:      Eric Kohl
 */

#include "precomp.h"


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
