/* $Id: tls.c,v 1.10 2003/01/15 21:24:36 chorns Exp $
 *
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

#include <k32.h>

#define NDEBUG
#include <kernel32/kernel32.h>


/* FUNCTIONS *****************************************************************/

DWORD STDCALL 
TlsAlloc(VOID)
{
   ULONG Index;

   RtlAcquirePebLock();
   Index = RtlFindClearBitsAndSet (NtCurrentPeb()->TlsBitmap, 1, 0);
   if (Index == (ULONG)-1)
     {
       SetLastErrorByStatus(STATUS_NO_MEMORY);
     }
   else
     {
       NtCurrentTeb()->TlsSlots[Index] = 0;
     }
   RtlReleasePebLock();
   
   return(Index);
}

WINBOOL STDCALL 
TlsFree(DWORD dwTlsIndex)
{
   if (dwTlsIndex >= TLS_MINIMUM_AVAILABLE)
     {
	SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
	return(FALSE);
     }

   RtlAcquirePebLock();
   if (RtlAreBitsSet(NtCurrentPeb()->TlsBitmap, dwTlsIndex, 1))
     {
	/*
	 * clear the tls cells (slots) in all threads
	 * of the current process
	 */
	NtSetInformationThread(NtCurrentThread(),
			       ThreadZeroTlsCell,
			       &dwTlsIndex,
			       sizeof(DWORD));
	RtlClearBits(NtCurrentPeb()->TlsBitmap,
		     dwTlsIndex,
		     1);
     }
   RtlReleasePebLock();

   return(TRUE);
}

LPVOID STDCALL 
TlsGetValue(DWORD dwTlsIndex)
{
   if (dwTlsIndex >= TLS_MINIMUM_AVAILABLE)
     {
	SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
	return(NULL);
     }
   return(NtCurrentTeb()->TlsSlots[dwTlsIndex]);
}

WINBOOL STDCALL 
TlsSetValue(DWORD dwTlsIndex, LPVOID lpTlsValue)
{
   if (dwTlsIndex >= TLS_MINIMUM_AVAILABLE)
     {
	SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
	return(FALSE);
     }
   NtCurrentTeb()->TlsSlots[dwTlsIndex] = lpTlsValue;
   return(TRUE);
}

/* EOF */
