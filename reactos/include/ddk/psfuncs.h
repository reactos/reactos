/* $Id: psfuncs.h,v 1.8 2000/07/01 22:36:53 ekohl Exp $
 */
#ifndef _INCLUDE_DDK_PSFUNCS_H
#define _INCLUDE_DDK_PSFUNCS_H


NTSTATUS
STDCALL
PsAssignImpersonationToken (
	PETHREAD	Thread,
	HANDLE		TokenHandle
	);

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
NTSTATUS
STDCALL
PsCreateSystemThread (
	PHANDLE			ThreadHandle,
	ACCESS_MASK		DesiredAccess,
	POBJECT_ATTRIBUTES	ObjectAttributes,
	HANDLE			ProcessHandle,
	PCLIENT_ID		ClientId,
	PKSTART_ROUTINE		StartRoutine,
	PVOID			StartContext
	);

/*
 * PEPROCESS
 * PsGetCurrentProcess (
 *	VOID
 *	);
 */
#define PsGetCurrentProcess() \
	(IoGetCurrentProcess ())

HANDLE
STDCALL
PsGetCurrentProcessId (
	VOID
	);

/*
 * PETHREAD
 * PsGetCurrentThread (
 *	VOID
 *	);
*/
#define PsGetCurrentThread() \
	((PETHREAD)KeGetCurrentThread ())

HANDLE
STDCALL
PsGetCurrentThreadId (
	VOID
	);

BOOLEAN
STDCALL
PsGetVersion (
	PULONG		MajorVersion	OPTIONAL,
	PULONG		MinorVersion	OPTIONAL,
	PULONG		BuildNumber	OPTIONAL,
	PUNICODE_STRING	CSDVersion	OPTIONAL
	);

VOID
STDCALL
PsImpersonateClient (
	PETHREAD			Thread,
	PACCESS_TOKEN			Token,
	UCHAR				b,
	UCHAR				c,
	SECURITY_IMPERSONATION_LEVEL	Level
	);

PACCESS_TOKEN
STDCALL
PsReferenceImpersonationToken (
	PETHREAD			Thread,
	PULONG				Unknown1,
	PULONG				Unknown2,
	SECURITY_IMPERSONATION_LEVEL	* Level
	);

PACCESS_TOKEN
STDCALL
PsReferencePrimaryToken (
	PEPROCESS	Process
	);

#if 0
/* FIXME: This is the correct prototype */
VOID
STDCALL
PsRevertToSelf (
	VOID
	);
#endif

VOID
STDCALL
PsRevertToSelf (
	PETHREAD	Thread
	);

NTSTATUS
STDCALL
PsTerminateSystemThread (
	NTSTATUS	ExitStatus
	);

#endif

/* EOF */
