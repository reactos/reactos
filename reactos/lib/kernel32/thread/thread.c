/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/thread/thread.c
 * PURPOSE:         Thread functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
			Tls functions are modified from WINE
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <windows.h>
#include <kernel32/thread.h>
#include <ddk/ntddk.h>
#include <string.h>


HANDLE
STDCALL
CreateThread(
	     LPSECURITY_ATTRIBUTES lpThreadAttributes,
	     DWORD dwStackSize,
	     LPTHREAD_START_ROUTINE lpStartAddress,
	     LPVOID lpParameter,
	     DWORD dwCreationFlags,
	     LPDWORD lpThreadId
	     )
{
	return CreateRemoteThread(NtCurrentProcess(),lpThreadAttributes,dwStackSize,
			lpStartAddress,lpParameter,dwCreationFlags,lpThreadId);
}




HANDLE
STDCALL
CreateRemoteThread(
		   HANDLE hProcess,
		   LPSECURITY_ATTRIBUTES lpThreadAttributes,
		   DWORD dwStackSize,
		   LPTHREAD_START_ROUTINE lpStartAddress,
		   LPVOID lpParameter,
		   DWORD dwCreationFlags,
		   LPDWORD lpThreadId
		   )
{	
	NTSTATUS errCode;
	HANDLE ThreadHandle;
	OBJECT_ATTRIBUTES ObjectAttributes;
	CLIENT_ID ClientId;
	CONTEXT ThreadContext;
	INITIAL_TEB InitialTeb;
	BOOLEAN CreateSuspended = FALSE;

	ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
	ObjectAttributes.RootDirectory = NULL;
	ObjectAttributes.ObjectName = NULL;
	ObjectAttributes.Attributes = 0;
	if ( lpThreadAttributes != NULL ) {
		if ( lpThreadAttributes->bInheritHandle ) 
			ObjectAttributes.Attributes = OBJ_INHERIT;
		ObjectAttributes.SecurityDescriptor = lpThreadAttributes->lpSecurityDescriptor;
	}
	ObjectAttributes.SecurityQualityOfService = NULL;

	if ( ( dwCreationFlags & CREATE_SUSPENDED ) == CREATE_SUSPENDED )
		CreateSuspended = TRUE;
	else
		CreateSuspended = FALSE;
	// fix context
	GetThreadContext(NtCurrentThread(),&ThreadContext);
	// fix teb [ stack context ] --> check the image file 

	errCode = NtCreateThread(
		&ThreadHandle,
		THREAD_ALL_ACCESS,
		&ObjectAttributes,
		hProcess,
		&ClientId,
		&ThreadContext,
		&InitialTeb,
		CreateSuspended
	);
	if ( lpThreadId != NULL )
		memcpy(lpThreadId, &ClientId.UniqueThread,sizeof(ULONG));

	return ThreadHandle;
}

NT_TEB *GetTeb(VOID)
{
	return NULL;
}

WINBOOL STDCALL
SwitchToThread(VOID )
{
	NTSTATUS errCode;
	errCode = NtYieldExecution();
	return TRUE;
}

DWORD
STDCALL
GetCurrentThreadId()
{

	return (DWORD)(GetTeb()->Cid).UniqueThread; 
}

VOID
STDCALL
ExitThread(
	    UINT uExitCode
	    ) 
{
	NTSTATUS errCode;	 

	errCode = NtTerminateThread(
		NtCurrentThread() ,
		uExitCode
	);
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
	}
	return;
}

WINBOOL
STDCALL
GetThreadTimes(
	       HANDLE hThread,
	       LPFILETIME lpCreationTime,
	       LPFILETIME lpExitTime,
	       LPFILETIME lpKernelTime,
	       LPFILETIME lpUserTime
	       )
{
	NTSTATUS errCode;
	KERNEL_USER_TIMES KernelUserTimes;
	ULONG ReturnLength;
	errCode = NtQueryInformationThread(hThread,ThreadTimes,&KernelUserTimes,sizeof(KERNEL_USER_TIMES),&ReturnLength);
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	memcpy(lpCreationTime, &KernelUserTimes.CreateTime, sizeof(FILETIME));
	memcpy(lpExitTime, &KernelUserTimes.ExitTime, sizeof(FILETIME));
	memcpy(lpKernelTime, &KernelUserTimes.KernelTime, sizeof(FILETIME));
	memcpy(lpUserTime, &KernelUserTimes.UserTime, sizeof(FILETIME));
	return TRUE;
	
}


WINBOOL
STDCALL GetThreadContext(
    HANDLE hThread,	
    LPCONTEXT lpContext 	
   )
{
	NTSTATUS errCode;
	errCode = NtGetContextThread(hThread,lpContext);
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	return TRUE;
}

WINBOOL
STDCALL
SetThreadContext(
    HANDLE hThread,	
    CONST CONTEXT *lpContext 
   )
{
	NTSTATUS errCode;

	errCode = NtSetContextThread(hThread,(void *)lpContext);
	if (!NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	return TRUE;
}



WINBOOL 
STDCALL
GetExitCodeThread(
    HANDLE hThread,	
    LPDWORD lpExitCode 
   )
{
	NTSTATUS errCode;
	THREAD_BASIC_INFORMATION ThreadBasic;
	ULONG DataWritten;
	errCode = NtQueryInformationThread(hThread,ThreadBasicInformation,&ThreadBasic,sizeof(THREAD_BASIC_INFORMATION),&DataWritten);
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	memcpy( lpExitCode ,&ThreadBasic.ExitStatus,sizeof(DWORD));
	return TRUE;
	
}


DWORD 
STDCALL
ResumeThread(
    HANDLE hThread 	
   )
{
	NTSTATUS errCode;
	ULONG PreviousResumeCount;

	errCode = NtResumeThread(hThread,&PreviousResumeCount );
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return  -1;
	}
	return PreviousResumeCount;
}

DWORD 
STDCALL
SuspendThread(
    HANDLE hThread 
   )
{
	NTSTATUS errCode;
	ULONG PreviousSuspendCount;

	errCode = NtSuspendThread(hThread,&PreviousSuspendCount );
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return  -1;
	}
	return PreviousSuspendCount;
}


DWORD
STDCALL
SetThreadAffinityMask(
		      HANDLE hThread,
		      DWORD dwThreadAffinityMask
		      )
{
	return 0;
}


WINBOOL
STDCALL
SetThreadPriority(
		  HANDLE hThread,
		  int nPriority
		  )
{
	NTSTATUS errCode;
	THREAD_BASIC_INFORMATION ThreadBasic;
	ULONG DataWritten;
	errCode = NtQueryInformationThread(hThread,ThreadBasicInformation,&ThreadBasic,sizeof(THREAD_BASIC_INFORMATION),&DataWritten);
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	ThreadBasic.BasePriority = nPriority;
	errCode = NtSetInformationThread(hThread,ThreadBasicInformation,&ThreadBasic,sizeof(THREAD_BASIC_INFORMATION));
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	return TRUE;
}


int
STDCALL
GetThreadPriority(
		  HANDLE hThread
		  )
{
	NTSTATUS errCode;
	THREAD_BASIC_INFORMATION ThreadBasic;
	ULONG DataWritten;
	errCode = NtQueryInformationThread(hThread,ThreadBasicInformation,&ThreadBasic,sizeof(THREAD_BASIC_INFORMATION),&DataWritten);
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return THREAD_PRIORITY_ERROR_RETURN;
	}
	return ThreadBasic.BasePriority;
}


/* (WIN32) Thread Local Storage ******************************************** */

DWORD	STDCALL
TlsAlloc(VOID)
{
	DWORD 	dwTlsIndex = GetTeb()->dwTlsIndex;
	

	void	**TlsData = GetTeb()->TlsData;
	
	
	if (dwTlsIndex < sizeof(TlsData) / sizeof(TlsData[0]))
	{
		TlsData[dwTlsIndex] = NULL;
		return (dwTlsIndex++);
	}
	return (0xFFFFFFFFUL);
}

WINBOOL	STDCALL
TlsFree(DWORD dwTlsIndex)
{
	
	return (TRUE);
}

LPVOID	STDCALL
TlsGetValue(DWORD dwTlsIndex)
{
	
	
	void	**TlsData = GetTeb()->TlsData;

	
	if (dwTlsIndex < sizeof(TlsData) / sizeof(TlsData[0]))
	{
	
		SetLastError(NO_ERROR);
		return (TlsData[dwTlsIndex]);
	}
	SetLastError(1);
	return (NULL);
}

WINBOOL	STDCALL
TlsSetValue(DWORD dwTlsIndex, LPVOID lpTlsValue)
{
	
	
	void	**TlsData = GetTeb()->TlsData;

	
	if (dwTlsIndex < sizeof(TlsData) / sizeof(TlsData[0]))
	{
		
		TlsData[dwTlsIndex] = lpTlsValue;
		return (TRUE);
	}
	return (FALSE);
}

/*************************************************************/
