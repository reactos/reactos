/* $Id: psfuncs.h,v 1.28 2004/03/14 18:10:41 ekohl Exp $
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

VOID STDCALL
PsEstablishWin32Callouts (PW32_PROCESS_CALLBACK W32ProcessCallback,
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

NTSTATUS STDCALL PsLookupProcessByProcessId(IN PVOID ProcessId,
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
