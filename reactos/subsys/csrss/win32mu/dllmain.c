/* $Id: $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/win32csr/dllmain.c
 * PURPOSE:         Initialization 
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include "csrplugin.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS *******************************************************************/

HINSTANCE DllHandle = (HINSTANCE) 0;

/* FUNCTIONS *****************************************************************/

BOOL STDCALL
DllMain(HANDLE hDll,
	DWORD dwReason,
	LPVOID lpReserved)
{
  if (DLL_PROCESS_ATTACH == dwReason)
    {
      DllHandle = hDll;
    }

  return TRUE;
}

/* EOF */
