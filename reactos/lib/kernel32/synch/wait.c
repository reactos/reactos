/* $Id: wait.c,v 1.18 2002/09/08 10:22:45 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/synch/wait.c
 * PURPOSE:         Wait functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <kernel32/error.h>
#include <windows.h>
#include <wchar.h>

#define NDEBUG
#include <kernel32/kernel32.h>

/* FUNCTIONS ****************************************************************/

DWORD STDCALL
WaitForSingleObject(HANDLE hHandle,
		    DWORD dwMilliseconds)
{
   return WaitForSingleObjectEx(hHandle,
				dwMilliseconds,
				FALSE);
}


DWORD STDCALL
WaitForSingleObjectEx(HANDLE hHandle,
                      DWORD  dwMilliseconds,
                      BOOL   bAlertable)
{
   NTSTATUS  errCode;
   PLARGE_INTEGER TimePtr;
   LARGE_INTEGER Time;

   if (dwMilliseconds == INFINITE)
     {
	TimePtr = NULL;
     }
   else
     {
        Time.QuadPart = -10000 * dwMilliseconds;
	TimePtr = &Time;
     }

   errCode = NtWaitForSingleObject(hHandle,
				   (BOOLEAN) bAlertable,
				   TimePtr);
   if (errCode == STATUS_TIMEOUT)
     {
         return WAIT_TIMEOUT;
     }
   else if ((errCode == WAIT_OBJECT_0) ||
	    (errCode == WAIT_ABANDONED_0))
     {
	return(errCode);
     }

   SetLastErrorByStatus (errCode);

   return(WAIT_FAILED);
}


DWORD STDCALL
WaitForMultipleObjects(DWORD nCount,
                       CONST HANDLE *lpHandles,
                       BOOL  bWaitAll,
                       DWORD dwMilliseconds)
{
   return WaitForMultipleObjectsEx(nCount,
				   lpHandles,
				   bWaitAll ? WaitAll : WaitAny,
				   dwMilliseconds,
				   FALSE);
}


DWORD STDCALL
WaitForMultipleObjectsEx(DWORD nCount,
                         CONST HANDLE *lpHandles,
                         BOOL  bWaitAll,
                         DWORD dwMilliseconds,
                         BOOL  bAlertable)
{
   NTSTATUS  errCode;
   LARGE_INTEGER Time;
   PLARGE_INTEGER TimePtr;

   DPRINT("nCount %lu\n", nCount);

   if (dwMilliseconds == INFINITE)
     {
        TimePtr = NULL;
     }
   else
     {
        Time.QuadPart = -10000 * dwMilliseconds;
        TimePtr = &Time;
     }

   errCode = NtWaitForMultipleObjects (nCount,
                                       (PHANDLE)lpHandles,
                                       (CINT)bWaitAll,
                                       (BOOLEAN)bAlertable,
                                       TimePtr);

   if (errCode == STATUS_TIMEOUT)
     {
         return WAIT_TIMEOUT;
     }
   else if (((errCode >= WAIT_OBJECT_0) &&
             (errCode <= WAIT_OBJECT_0 + nCount - 1)) ||
            ((errCode >= WAIT_ABANDONED_0) &&
             (errCode <= WAIT_ABANDONED_0 + nCount - 1)))
     {
        return(errCode);
     }

   DPRINT("errCode %lx\n", errCode);
   SetLastErrorByStatus (errCode);

   return(WAIT_FAILED);
}


BOOL STDCALL
SignalObjectAndWait(HANDLE hObjectToSignal,
		    HANDLE hObjectToWaitOn,
		    DWORD dwMilliseconds,
		    BOOL bAlertable)
{
   UNIMPLEMENTED
}

/* EOF */
