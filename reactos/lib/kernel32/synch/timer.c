/* $Id: timer.c,v 1.17 2004/10/24 12:16:54 weiden Exp $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/kernel32/synch/timer.c
 * PURPOSE:              Implementing timer
 * PROGRAMMER:           
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"


/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
HANDLE STDCALL
CreateWaitableTimerW(LPSECURITY_ATTRIBUTES lpTimerAttributes,
		     BOOL bManualReset,
		     LPCWSTR lpTimerName)
{
   NTSTATUS Status;
   HANDLE TimerHandle;
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING UnicodeName;
   ULONG TimerType;
   
   if (bManualReset)
     TimerType = NotificationTimer;
   else
     TimerType = SynchronizationTimer;

   if (lpTimerName)
     {
       RtlInitUnicodeString(&UnicodeName, lpTimerName);
     }
   InitializeObjectAttributes(&ObjectAttributes,
			      (lpTimerName ? &UnicodeName : NULL),
			      0,
			      hBaseDir,
			      NULL);
   
   if (lpTimerAttributes != NULL)
     {
       ObjectAttributes.SecurityDescriptor = lpTimerAttributes->lpSecurityDescriptor;
       if(lpTimerAttributes->bInheritHandle)
         {
           ObjectAttributes.Attributes |= OBJ_INHERIT;
         }
     }
   
   Status = NtCreateTimer(&TimerHandle,
			  TIMER_ALL_ACCESS,
			  &ObjectAttributes,
			  TimerType);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return NULL;
     }

   return TimerHandle;
}


/*
 * @implemented
 */
HANDLE STDCALL
CreateWaitableTimerA(LPSECURITY_ATTRIBUTES lpTimerAttributes,
		     BOOL bManualReset,
		     LPCSTR lpTimerName)
{
	UNICODE_STRING TimerNameU;
	ANSI_STRING TimerName;
	HANDLE TimerHandle;

        if (lpTimerName != NULL)
          {
	    RtlInitAnsiString (&TimerName,
	                       (LPSTR)lpTimerName);
	    RtlAnsiStringToUnicodeString (&TimerNameU,
	                                  &TimerName,
	                                  TRUE);
          }

	TimerHandle = CreateWaitableTimerW (lpTimerAttributes,
	                                    bManualReset,
	                                    (lpTimerName ? TimerNameU.Buffer : NULL));

        if (lpTimerName != NULL)
          {
            RtlFreeUnicodeString (&TimerNameU);
          }

	return TimerHandle;
}


/*
 * @implemented
 */
HANDLE STDCALL
OpenWaitableTimerW(DWORD dwDesiredAccess,
		   BOOL bInheritHandle,
		   LPCWSTR lpTimerName)
{
   NTSTATUS Status;
   HANDLE TimerHandle;
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING UnicodeName;
   
   if (lpTimerName == NULL)
     {
	SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
	return NULL;
     }

   RtlInitUnicodeString(&UnicodeName,
			lpTimerName);
   InitializeObjectAttributes(&ObjectAttributes,
			      &UnicodeName,
			      (bInheritHandle ? OBJ_INHERIT : 0),
			      hBaseDir,
			      NULL);

   Status = NtOpenTimer(&TimerHandle,
			dwDesiredAccess,
			&ObjectAttributes);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return NULL;
     }

   return TimerHandle;
}


/*
 * @implemented
 */
HANDLE STDCALL
OpenWaitableTimerA(DWORD dwDesiredAccess,
		   BOOL bInheritHandle,
		   LPCSTR lpTimerName)
{
   UNICODE_STRING TimerNameU;
   ANSI_STRING TimerName;
   HANDLE TimerHandle;
	
   if (lpTimerName == NULL)
     {
        SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
        return NULL;
     }

   RtlInitAnsiString (&TimerName,
                     (LPSTR)lpTimerName);
   RtlAnsiStringToUnicodeString (&TimerNameU,
                                 &TimerName,
                                 TRUE);

   TimerHandle = OpenWaitableTimerW (dwDesiredAccess,
                                     bInheritHandle,
                                     TimerNameU.Buffer);

   RtlFreeUnicodeString (&TimerNameU);

   return TimerHandle;
}


/*
 * @implemented
 */
BOOL STDCALL
SetWaitableTimer(HANDLE hTimer,
		 const LARGE_INTEGER *pDueTime,
		 LONG lPeriod,
		 PTIMERAPCROUTINE pfnCompletionRoutine,
		 LPVOID lpArgToCompletionRoutine,
		 BOOL fResume)
{
   NTSTATUS Status;
   BOOLEAN pState;

   Status = NtSetTimer(hTimer,
		       (LARGE_INTEGER *)pDueTime,
		       pfnCompletionRoutine,
		       lpArgToCompletionRoutine,
		       fResume,
		       lPeriod,
		       &pState);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return FALSE;
     }
   return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
CancelWaitableTimer(HANDLE hTimer)
{
   NTSTATUS Status;
   BOOLEAN CurrentState;

   Status = NtCancelTimer(hTimer,
			  &CurrentState);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return FALSE;
     }
   return TRUE;
}

/* EOF */
