/* $Id: timer.c,v 1.14 2004/01/23 17:17:10 ekohl Exp $
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
#include <kernel32/kernel32.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
HANDLE STDCALL
CreateWaitableTimerW(LPSECURITY_ATTRIBUTES lpTimerAttributes,
		     BOOL bManualReset,
		     LPWSTR lpTimerName)
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

   RtlInitUnicodeString(&UnicodeName, lpTimerName);
   InitializeObjectAttributes(&ObjectAttributes,
			      &UnicodeName,
			      0,
			      hBaseDir,
			      NULL);
   
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

	RtlInitAnsiString (&TimerName,
	                   (LPSTR)lpTimerName);
	RtlAnsiStringToUnicodeString (&TimerNameU,
	                              &TimerName,
	                              TRUE);

	TimerHandle = CreateWaitableTimerW (lpTimerAttributes,
	                                    bManualReset,
	                                    TimerNameU.Buffer);

	RtlFreeUnicodeString (&TimerNameU);

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
   ULONG Attributes = 0;

   if (bInheritHandle)
     {
	Attributes = OBJ_INHERIT;
     }

   RtlInitUnicodeString(&UnicodeName,
			lpTimerName);
   InitializeObjectAttributes(&ObjectAttributes,
			      &UnicodeName,
			      Attributes,
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
