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

VOID
STDCALL PsChargePoolQuota(
    IN PEPROCESS Process,
    IN POOL_TYPE PoolType,
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

BOOLEAN                             
STDCALL PsIsSystemThread(                   
    struct _ETHREAD* Thread                 
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
STDCALL PsRevertToSelf(
	VOID
	); 

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

BOOLEAN STDCALL PsGetVersion (PULONG		MajorVersion	OPTIONAL,
			      PULONG		MinorVersion	OPTIONAL,
			      PULONG		BuildNumber	OPTIONAL,
			      PUNICODE_STRING	CSDVersion	OPTIONAL);

LARGE_INTEGER STDCALL PsGetProcessExitTime(VOID);
BOOLEAN STDCALL PsIsThreadTerminating(struct _ETHREAD* Thread);

NTSTATUS STDCALL
PsSetCreateProcessNotifyRoutine(IN PCREATE_PROCESS_NOTIFY_ROUTINE NotifyRoutine,
				IN BOOLEAN Remove);

NTSTATUS STDCALL
PsSetCreateThreadNotifyRoutine(IN PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine);

#endif

/* EOF */
