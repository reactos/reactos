/* $Id: psfuncs.h,v 1.14 2001/08/21 20:13:05 chorns Exp $
 */
#ifndef _INCLUDE_DDK_PSFUNCS_H
#define _INCLUDE_DDK_PSFUNCS_H

NTSTATUS STDCALL PsAssignImpersonationToken (struct _ETHREAD* Thread,
					     HANDLE TokenHandle);

HANDLE STDCALL PsGetCurrentProcessId(VOID);
HANDLE STDCALL PsGetCurrentThreadId(VOID);

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
NTSTATUS STDCALL PsCreateSystemThread(PHANDLE ThreadHandle,
				      ACCESS_MASK DesiredAccess,
				      POBJECT_ATTRIBUTES ObjectAttributes,
				      HANDLE ProcessHandle,
				      PCLIENT_ID ClientId,
				      PKSTART_ROUTINE StartRoutine,
				      PVOID StartContext);
NTSTATUS STDCALL PsTerminateSystemThread(NTSTATUS ExitStatus);

NTSTATUS STDCALL PsCreateSystemProcess(PHANDLE ProcessHandle,
				       ACCESS_MASK DesiredAccess,
				       POBJECT_ATTRIBUTES ObjectAttributes);

struct _ETHREAD* STDCALL PsGetCurrentThread(VOID);
struct _EPROCESS* STDCALL PsGetCurrentProcess(VOID);
PACCESS_TOKEN STDCALL PsReferenceImpersonationToken(struct _ETHREAD* Thread,
						    PULONG Unknown1,
						    PULONG Unknown2,
						    SECURITY_IMPERSONATION_LEVEL* 
						    Level);
PACCESS_TOKEN STDCALL PsReferencePrimaryToken(struct _EPROCESS* Process);
NTSTATUS STDCALL PsAssignImpersonationToken(struct _ETHREAD* Thread,
					    HANDLE TokenHandle);

VOID STDCALL PsImpersonateClient(struct _ETHREAD* Thread,
				 PACCESS_TOKEN Token,
				 UCHAR b,
				 UCHAR c,
				 SECURITY_IMPERSONATION_LEVEL Level);
VOID STDCALL PsRevertToSelf(struct _ETHREAD* Thread);

BOOLEAN STDCALL PsGetVersion (PULONG		MajorVersion	OPTIONAL,
			      PULONG		MinorVersion	OPTIONAL,
			      PULONG		BuildNumber	OPTIONAL,
			      PUNICODE_STRING	CSDVersion	OPTIONAL);

VOID STDCALL PsDispatchThread(ULONG NewThreadStatus);

#endif

/* EOF */
