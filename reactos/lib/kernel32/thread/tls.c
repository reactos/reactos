/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/thread/tls.c
 * PURPOSE:         Thread functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  Tls functions are modified from WINE
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <windows.h>
#include <kernel32/thread.h>
#include <string.h>
#include <ntdll/rtl.h>

/* FUNCTIONS *****************************************************************/

DWORD STDCALL TlsAlloc(VOID)
{
   PULONG TlsBitmap = NtCurrentPeb()->TlsBitmapBits;
   ULONG i, j;
   
   RtlAcquirePebLock();
   
   for (i = 0; i < 2; i++)
     {	
	for (j = 0; j < 32; j++)
	  {
	     if ((TlsBitmap[i] & (1 << j)) == 0)
	       {
		  TlsBitmap[i] = TlsBitmap[i] | (1 << j);
		  RtlReleasePebLock();
		  return((i * 32) + j);
	       }
	  }
     }

   RtlReleasePebLock();
   return (0xFFFFFFFFUL);
}

WINBOOL	STDCALL TlsFree(DWORD dwTlsIndex)
{
   PULONG TlsBitmap = NtCurrentPeb()->TlsBitmapBits;
   
   RtlAcquirePebLock();
   TlsBitmap[dwTlsIndex / 32] =
     TlsBitmap[dwTlsIndex / 32] & ~(1 << (dwTlsIndex % 32));
   RtlReleasePebLock();
   return(TRUE);
}

LPVOID STDCALL TlsGetValue(DWORD dwTlsIndex)
{
   return(NtCurrentTeb()->TlsSlots[dwTlsIndex]);
}

WINBOOL	STDCALL TlsSetValue(DWORD dwTlsIndex, LPVOID lpTlsValue)
{
   NtCurrentTeb()->TlsSlots[dwTlsIndex] = lpTlsValue;
   return(TRUE);
}
