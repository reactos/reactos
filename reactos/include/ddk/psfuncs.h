
PACCESS_TOKEN PsReferenceEffectiveToken(PETHREAD Thread,
					PTOKEN_TYPE TokenType,
					PUCHAR b,
					PSECURITY_IMPERSONATION_LEVEL Level);

NTSTATUS PsOpenTokenOfProcess(HANDLE ProcessHandle,
			      PACCESS_TOKEN* Token);

HANDLE PsGetCurrentProcessId(VOID);
HANDLE PsGetCurrentThreadId(VOID);

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
NTSTATUS PsCreateSystemThread(PHANDLE ThreadHandle,
			      ACCESS_MASK DesiredAccess,
			      POBJECT_ATTRIBUTES ObjectAttributes,
			      HANDLE ProcessHandle,
			      PCLIENT_ID ClientId,
			      PKSTART_ROUTINE StartRoutine,
                              PVOID StartContext);
NTSTATUS PsTerminateSystemThread(NTSTATUS ExitStatus);
ULONG PsSuspendThread(PETHREAD Thread,
		      PNTSTATUS WaitStatus,
		      UCHAR Alertable,
		      ULONG WaitMode);
ULONG PsResumeThread(PETHREAD Thread,
		     PNTSTATUS WaitStatus);
PETHREAD PsGetCurrentThread(VOID);
struct _EPROCESS* PsGetCurrentProcess(VOID);
PACCESS_TOKEN PsReferenceImpersonationToken(PETHREAD Thread,
					    PULONG Unknown1,
					    PULONG Unknown2,
					    SECURITY_IMPERSONATION_LEVEL* 
					    Level);
PACCESS_TOKEN PsReferencePrimaryToken(PEPROCESS Process);
NTSTATUS PsAssignImpersonationToken(PETHREAD Thread,
				    HANDLE TokenHandle);

VOID PsImpersonateClient(PETHREAD Thread,
			 PACCESS_TOKEN Token,
			 UCHAR b,
			 UCHAR c,
			 SECURITY_IMPERSONATION_LEVEL Level);
