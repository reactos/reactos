/* $Id: dllmain.c,v 1.2 2002/09/07 15:12:43 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/secur32/lsa.c
 * PURPOSE:         Client-side LSA functions
 * UPDATE HISTORY:
 *                  Created 05/08/00
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#define NTOS_USER_MODE
#include <ntos.h>

/* GLOBALS *******************************************************************/

HANDLE Secur32Heap;

/* FUNCTIONS *****************************************************************/

WINBOOL STDCALL DllMain(HINSTANCE hInstance, ULONG Reason, PVOID Reserved)
{
   switch (Reason)
     {
      case DLL_PROCESS_ATTACH:
	Secur32Heap = RtlCreateHeap(0, NULL, 4096, 0, NULL, NULL);
	if (Secur32Heap == 0)
	  {
	     return(FALSE);
	  }
	break;
	
      case DLL_PROCESS_DETACH:
	if (!RtlDestroyHeap(Secur32Heap))
	  {
	     return(FALSE);
	  }
	break;
     }
   return(TRUE);
}
