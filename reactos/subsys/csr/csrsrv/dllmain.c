/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csr/csrsrv/dllmain.c
 * PURPOSE:         DLL entry point
 */

/* INCLUDES ******************************************************************/

#include "srv.h"

#define NDEBUG
#include <debug.h>

HANDLE CsrSrvDllHandle = 0;

/* FUNCTIONS *****************************************************************/

BOOL STDCALL
DllMain(HANDLE hDll,
	DWORD dwReason,
	LPVOID lpReserved)
{
  if (DLL_PROCESS_ATTACH == dwReason)
    {
      CsrSrvDllHandle = hDll;
    }

  return TRUE;
}

/* EOF */
