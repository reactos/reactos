/* $Id$
 */
#ifndef _INCLUDE_DDK_PSFUNCS_H
#define _INCLUDE_DDK_PSFUNCS_H

NTSTATUS STDCALL
PsAssignImpersonationToken (IN struct _ETHREAD* Thread,
			    IN HANDLE TokenHandle);

/*
 * Warning: Even though it returns HANDLE, it's not a real HANDLE but really a
 * ULONG ProcessId! (Skywing)
 */
HANDLE STDCALL
PsGetCurrentProcessId (VOID);

HANDLE STDCALL
PsGetCurrentThreadId (VOID);

/*
 * FUNCTION: Creates a thread which executes in kernel mode
 * ARGUMENTS:
 *       ThreadHandle (OUT) = Caller supplied storage for the returned thread 
 *                            handle
 *       DesiredAccess = Requested access to the thread
 *       ObjectAttributes = Object attributes (optional)
 *       ProcessHandle = Handle of process thread will run in
 *                       NULL to use system process
 *       ClientId (OUT) = Caller supplied storage for the returned client id
 *                        of the thread (optional)
 *       StartRoutine = Entry point for the thread
 *       StartContext = Argument supplied to the thread when it begins
 *                     execution
 * RETURNS: Success or failure status
 */
NTSTATUS STDCALL
PsCreateSystemThread (PHANDLE ThreadHandle,
		      ACCESS_MASK DesiredAccess,
		      POBJECT_ATTRIBUTES ObjectAttributes,
		      HANDLE ProcessHandle,
		      PCLIENT_ID ClientId,
		      PKSTART_ROUTINE StartRoutine,
		      void *StartContext);

NTSTATUS STDCALL
PsTerminateSystemThread (NTSTATUS ExitStatus);

NTSTATUS STDCALL
PsCreateSystemProcess (PHANDLE ProcessHandle,
		       ACCESS_MASK DesiredAccess,
		       POBJECT_ATTRIBUTES ObjectAttributes);

NTSTATUS STDCALL PsCreateWin32Process(struct _EPROCESS* Process);
NTSTATUS STDCALL PsCreateWin32Thread(struct _ETHREAD* Thread);

VOID
STDCALL PsChargePoolQuota(
    IN PEPROCESS Process,
    IN POOL_TYPE PoolType,
    IN ULONG_PTR Amount
    );

NTSTATUS
STDCALL PsChargeProcessNonPagedPoolQuota (
   	IN PEPROCESS Process,
    IN ULONG_PTR Amount
	);

NTSTATUS
STDCALL PsChargeProcessPagedPoolQuota (
   	IN PEPROCESS Process,
    IN ULONG_PTR Amount
	);

NTSTATUS
STDCALL PsChargeProcessPoolQuota(
    IN PEPROCESS Process,
    IN POOL_TYPE PoolType,
    IN ULONG_PTR Amount
    );

VOID
STDCALL PsDereferenceImpersonationToken(
    IN PACCESS_TOKEN ImpersonationToken
    );

VOID
STDCALL PsDereferencePrimaryToken(
    IN PACCESS_TOKEN PrimaryToken
    );

BOOLEAN
STDCALL PsDisableImpersonation(
    IN struct _ETHREAD* Thread,
    IN PSE_IMPERSONATION_STATE ImpersonationState
    );

ULONG
STDCALL PsGetCurrentProcessSessionId (
    VOID
	);

KPROCESSOR_MODE
STDCALL PsGetCurrentThreadPreviousMode (
    VOID
	);


PVOID
STDCALL PsGetCurrentThreadStackBase (
    VOID
	);

PVOID
STDCALL PsGetCurrentThreadStackLimit (
    VOID
	);

PVOID
STDCALL PsGetJobLock(
    PEJOB	Job
	);

PVOID
STDCALL PsGetJobSessionId(
    PEJOB	Job
	);

ULONG
STDCALL PsGetJobUIRestrictionsClass(
   	PEJOB	Job
	);

LONGLONG
STDCALL PsGetProcessCreateTimeQuadPart(
    PEPROCESS	Process
	);

PVOID
STDCALL PsGetProcessDebugPort(
    PEPROCESS	Process
	);


BOOLEAN
STDCALL PsGetProcessExitProcessCalled(
    PEPROCESS	Process
	);

NTSTATUS
STDCALL PsGetProcessExitStatus(
	PEPROCESS Process
	);

HANDLE
STDCALL PsGetProcessId(
   	PEPROCESS	Process
	);

LPSTR
STDCALL PsGetProcessImageFileName(
    PEPROCESS	Process
	);

HANDLE
STDCALL PsGetProcessInheritedFromUniqueProcessId(
    	PEPROCESS	Process
	);

PEJOB
STDCALL PsGetProcessJob(
	PEPROCESS Process
	);

PPEB
STDCALL PsGetProcessPeb(
    PEPROCESS	Process
	);

ULONG
STDCALL PsGetProcessPriorityClass(
    PEPROCESS	Process
	);

PVOID
STDCALL PsGetProcessSectionBaseAddress(
    PEPROCESS	Process
	);

PVOID
STDCALL PsGetProcessSecurityPort(
	PEPROCESS Process
	);

HANDLE
STDCALL PsGetProcessSessionId(
    PEPROCESS	Process
	);

PVOID
STDCALL PsGetProcessWin32Process(
	PEPROCESS Process
	);

PVOID
STDCALL PsGetProcessWin32WindowStation(
    PEPROCESS	Process
	);

ULONG
STDCALL PsGetThreadFreezeCount(
	struct _ETHREAD* Thread
	);

BOOLEAN
STDCALL PsGetThreadHardErrorsAreDisabled(
    struct _ETHREAD*	Thread
	);

HANDLE
STDCALL PsGetThreadId(
    struct _ETHREAD*	Thread
	);

PEPROCESS
STDCALL PsGetThreadProcess(
    struct _ETHREAD*	Thread
	);

HANDLE
STDCALL PsGetThreadProcessId(
    struct _ETHREAD*	Thread
	);

HANDLE
STDCALL PsGetThreadSessionId(
    struct _ETHREAD*	Thread
	);

PTEB
STDCALL PsGetThreadTeb(
    struct _ETHREAD*	Thread
	);

PVOID
STDCALL PsGetThreadWin32Thread(
    struct _ETHREAD*	Thread
	);

BOOLEAN
STDCALL PsIsProcessBeingDebugged(
    PEPROCESS	Process
	);

                         
BOOLEAN                             
STDCALL PsIsSystemThread(                   
    struct _ETHREAD* Thread                 
     );

BOOLEAN
STDCALL PsIsThreadImpersonating(
    struct _ETHREAD*	Thread
	);

NTSTATUS
STDCALL PsRemoveCreateThreadNotifyRoutine (
    IN PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine
    );

NTSTATUS
STDCALL PsRemoveLoadImageNotifyRoutine(
    IN PLOAD_IMAGE_NOTIFY_ROUTINE NotifyRoutine
    );

VOID
STDCALL PsRestoreImpersonation(
	IN struct _ETHREAD*   	 Thread,
	IN PSE_IMPERSONATION_STATE  	ImpersonationState
     ); 

VOID
STDCALL PsReturnPoolQuota(
    IN PEPROCESS Process,
    IN POOL_TYPE PoolType,
    IN ULONG_PTR Amount
    );

VOID
STDCALL PsReturnProcessNonPagedPoolQuota(
    IN PEPROCESS Process,
    IN ULONG_PTR Amount
    );

VOID
STDCALL PsReturnProcessPagedPoolQuota(
    IN PEPROCESS Process,
    IN ULONG_PTR Amount
    );

VOID 
STDCALL PsRevertToSelf(
	VOID
	); 

VOID
STDCALL
PsRevertThreadToSelf(
	IN struct _ETHREAD* Thread
	);

VOID
STDCALL PsSetJobUIRestrictionsClass(
    PEJOB	Job,
    ULONG	UIRestrictionsClass	
	);

ULONG 
STDCALL PsSetLegoNotifyRoutine(   	
	PVOID LegoNotifyRoutine  	 
	);

VOID
STDCALL PsSetProcessPriorityClass(
    PEPROCESS	Process,
    ULONG	PriorityClass	
	);

VOID
STDCALL PsSetProcessSecurityPort(
    PEPROCESS	Process,
    PVOID	SecurityPort	
	);

VOID
STDCALL PsSetProcessWin32Process(
    PEPROCESS	Process,
    PVOID	Win32Process
	);

VOID
STDCALL PsSetProcessWin32WindowStation(
    PEPROCESS	Process,
    PVOID	WindowStation
	);

VOID
STDCALL PsSetThreadHardErrorsAreDisabled(
    struct _ETHREAD*	Thread,
    BOOLEAN	HardErrorsAreDisabled
	);

VOID
STDCALL PsSetThreadWin32Thread(
    struct _ETHREAD*	Thread,
    PVOID	Win32Thread
	);


VOID STDCALL
STDCALL PsEstablishWin32Callouts (PW32_PROCESS_CALLBACK W32ProcessCallback,
			  PW32_THREAD_CALLBACK W32ThreadCallback,
			  PVOID Param3,
			  PVOID Param4,
			  ULONG W32ThreadSize,
			  ULONG W32ProcessSize);

#define PsGetCurrentProcess() IoGetCurrentProcess()
#define PsGetCurrentThread() ((struct _ETHREAD*) (KeGetCurrentThread()))

PACCESS_TOKEN STDCALL
PsReferenceImpersonationToken (IN struct _ETHREAD *Thread,
			       OUT PBOOLEAN CopyOnOpen,
			       OUT PBOOLEAN EffectiveOnly,
			       OUT PSECURITY_IMPERSONATION_LEVEL ImpersonationLevel);

PACCESS_TOKEN STDCALL
PsReferencePrimaryToken (struct _EPROCESS* Process);

VOID STDCALL
PsImpersonateClient (IN struct _ETHREAD* Thread,
		     IN PACCESS_TOKEN Token,
		     IN BOOLEAN CopyOnOpen,
		     IN BOOLEAN EffectiveOnly,
		     IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel);

VOID STDCALL
PsRevertToSelf (VOID);

BOOLEAN STDCALL PsGetVersion (PULONG		MajorVersion	OPTIONAL,
			      PULONG		MinorVersion	OPTIONAL,
			      PULONG		BuildNumber	OPTIONAL,
			      PUNICODE_STRING	CSDVersion	OPTIONAL);

LARGE_INTEGER STDCALL PsGetProcessExitTime(VOID);
BOOLEAN STDCALL PsIsThreadTerminating(struct _ETHREAD* Thread);

NTSTATUS STDCALL PsLookupProcessByProcessId(IN HANDLE ProcessId,
					    OUT PEPROCESS *Process);

NTSTATUS STDCALL PsLookupProcessThreadByCid(IN PCLIENT_ID Cid,
					    OUT PEPROCESS *Process OPTIONAL,
					    OUT struct _ETHREAD **Thread);
					 /* OUT PETHREAD *Thread); */

NTSTATUS STDCALL PsLookupThreadByThreadId(IN PVOID ThreadId,
					  OUT struct _ETHREAD **Thread);
					/* OUT PETHREAD *Thread); */

NTSTATUS STDCALL
PsSetCreateProcessNotifyRoutine(IN PCREATE_PROCESS_NOTIFY_ROUTINE NotifyRoutine,
				IN BOOLEAN Remove);

NTSTATUS STDCALL
PsSetCreateThreadNotifyRoutine(IN PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine);

#endif

/* EOF */
