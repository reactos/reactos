/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/kernel32/mem/timer.c
 * PURPOSE:              Implementing timer
 * PROGRAMMER:           
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>

/* FUNCTIONS *****************************************************************/



HANDLE CreateWaitableTimerW(
  LPSECURITY_ATTRIBUTES lpTimerAttributes,
  WINBOOL bManualReset,  
  LPWSTR lpTimerName 
)
{

	NTSTATUS errCode;
	HANDLE TimerHandle;
	OBJECT_ATTRIBUTES ObjectAttributes;
	UNICODE_STRING UnicodeName;

	ULONG TimerType;

	if ( bManualReset )
		TimerType = 1;
	else
		TimerType = 2;

   	RtlInitUnicodeString(&UnicodeName, lpTimerName);
   	InitializeObjectAttributes(&ObjectAttributes,
			      &UnicodeName,
			      0,
			      NULL,
			      NULL);
	//TIMER_ALL_ACCESS
	errCode = NtCreateTimer(
		&TimerHandle,
		0,
		&ObjectAttributes,
		TimerType
	);

	return TimerHandle;
}


HANDLE CreateWaitableTimerA(
  LPSECURITY_ATTRIBUTES lpTimerAttributes,
  WINBOOL bManualReset,  
  LPCSTR lpTimerName 
)
{
	WCHAR NameW[MAX_PATH];
	ULONG i = 0;

	while ((*lpTimerName)!=0 && i < MAX_PATH)
		{
		NameW[i] = *lpTimerName;
		lpTimerName++;
		i++;
		}
	NameW[i] = 0;
	return CreateWaitableTimerW(lpTimerAttributes,
		bManualReset,
		NameW);
}

HANDLE OpenWaitableTimerW(
  DWORD dwDesiredAccess,  
  WINBOOL bInheritHandle,    
  LPCWSTR lpTimerName     
)
{
	NTSTATUS errCode;

	HANDLE TimerHandle;
	OBJECT_ATTRIBUTES ObjectAttributes;
	UNICODE_STRING UnicodeName;
	ULONG Attributes = 0;

	if ( bInheritHandle )
		Attributes = OBJ_INHERIT;
	

   	RtlInitUnicodeString(&UnicodeName, lpTimerName);
   	InitializeObjectAttributes(&ObjectAttributes,
			      &UnicodeName,
			      Attributes,
			      NULL,
			      NULL);

	errCode = NtOpenTimer(
		&TimerHandle,
		dwDesiredAccess,
		&ObjectAttributes
    	);

	return TimerHandle;	
}

HANDLE OpenWaitableTimerA(
  DWORD dwDesiredAccess,  
  WINBOOL bInheritHandle,    
  LPCSTR lpTimerName     
)
{
	WCHAR NameW[MAX_PATH];
	ULONG i = 0;

	while ((*lpTimerName)!=0 && i < MAX_PATH)
		{
		NameW[i] = *lpTimerName;
		lpTimerName++;
		i++;
		}
	NameW[i] = 0;
	return OpenWaitableTimerW(dwDesiredAccess, bInheritHandle,(LPCWSTR) NameW);
}


WINBOOL SetWaitableTimer(
  HANDLE hTimer,         
  LARGE_INTEGER *pDueTime,          
  LONG lPeriod,             
  PTIMERAPCROUTINE pfnCompletionRoutine,  
  LPVOID lpArgToCompletionRoutine,      
  WINBOOL fResume                           
)
{
	NTSTATUS errCode;
	BOOLEAN pState;

	errCode = NtSetTimer(hTimer, pDueTime,
		 pfnCompletionRoutine,
		lpArgToCompletionRoutine, fResume, lPeriod, &pState);

	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	return TRUE;	
}

WINBOOL CancelWaitableTimer(HANDLE hTimer)
{
	NTSTATUS errCode;
	BOOLEAN CurrentState;

	errCode = NtCancelTimer(
		hTimer,
		&CurrentState
	);
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	return TRUE;	
	
}