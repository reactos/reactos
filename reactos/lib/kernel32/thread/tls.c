/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/thread/tls.c
 * PURPOSE:         Thread functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *			Tls functions are modified from WINE
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <kernel32/thread.h>
#include <ddk/ntddk.h>
#include <string.h>

/* FUNCTIONS *****************************************************************/

DWORD STDCALL TlsAlloc(VOID)
{
   DWORD dwTlsIndex = GetTeb()->dwTlsIndex;
   void	**TlsData = GetTeb()->TlsData;
	
   if (dwTlsIndex < (sizeof(TlsData) / sizeof(TlsData[0])))
     {
	TlsData[dwTlsIndex] = NULL;
	return (dwTlsIndex++);
     }
   return (0xFFFFFFFFUL);
}

WINBOOL	STDCALL TlsFree(DWORD dwTlsIndex)
{
   return (TRUE);
}

LPVOID STDCALL TlsGetVlue(DWORD dwTlsIndex)
{
   void	**TlsData = GetTeb()->TlsData;
	
   if (dwTlsIndex < (sizeof(TlsData) / sizeof(TlsData[0])))
     {
	SetLastError(NO_ERROR);
	return (TlsData[dwTlsIndex]);
     }
   SetLastError(1);
   return (NULL);
}

WINBOOL	STDCALL TlsSetValue(DWORD dwTlsIndex, LPVOID lpTlsValue)
{
   void	**TlsData = GetTeb()->TlsData;
	
   if (dwTlsIndex < (sizeof(TlsData) / sizeof(TlsData[0])))
     {
	TlsData[dwTlsIndex] = lpTlsValue;
	return (TRUE);
     }
   return (FALSE);
}
