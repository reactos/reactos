/* $Id: dllmain.c,v 1.11 2000/03/22 18:35:47 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/dllmain.c
 * PURPOSE:         Initialization 
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <ddk/ntddk.h>
#include <ntdll/csr.h>
#include <windows.h>
#include <wchar.h>

#define NDEBUG
#include <kernel32/kernel32.h>

WINBOOL STDCALL DllMain (HANDLE hInst,
			 ULONG ul_reason_for_call,
			 LPVOID lpReserved);



BOOL WINAPI DllMainCRTStartup(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
   return(DllMain(hDll,dwReason,lpReserved));
}

WINBOOL STDCALL DllMain(HANDLE hInst,
			ULONG ul_reason_for_call,
			LPVOID lpReserved)
{
   DPRINT("DllMain(hInst %x, ul_reason_for_call %d)\n",
	  hInst, ul_reason_for_call);

   switch (ul_reason_for_call)
     {
      case DLL_PROCESS_ATTACH:
	  {
	     NTSTATUS Status;
	     DPRINT("DLL_PROCESS_ATTACH\n");
	     break;
	  }
      case DLL_PROCESS_DETACH:
	  {
	     DPRINT("DLL_PROCESS_DETACH\n");
	     HeapDestroy(NtCurrentPeb()->ProcessHeap);
	     break;
	  }
      default:
	break;
    }
   return TRUE;
}

/* EOF */
