/******************************************************************************
 *                          Process Manager Functions                         *
 ******************************************************************************/
$if (_WDMDDK_)

NTKERNELAPI
NTSTATUS
NTAPI
PsWrapApcWow64Thread(
  IN OUT PVOID *ApcContext,
  IN OUT PVOID *ApcRoutine);

/*
 * PEPROCESS
 * PsGetCurrentProcess(VOID)
 */
#define PsGetCurrentProcess IoGetCurrentProcess

#if !defined(_PSGETCURRENTTHREAD_)
#define _PSGETCURRENTTHREAD_
FORCEINLINE
PETHREAD
NTAPI
PsGetCurrentThread(VOID)
{
  return (PETHREAD)KeGetCurrentThread();
}
#endif /* !_PSGETCURRENTTHREAD_ */

$endif (_WDMDDK_)
$if (_NTDDK_)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenProcess(
  OUT PHANDLE ProcessHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes,
  IN PCLIENT_ID ClientId OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationProcess(
  IN HANDLE ProcessHandle,
  IN PROCESSINFOCLASS ProcessInformationClass,
  OUT PVOID ProcessInformation OPTIONAL,
  IN ULONG ProcessInformationLength,
  OUT PULONG ReturnLength OPTIONAL);
$endif (_NTDDK_)
$if (_NTIFS_)

NTKERNELAPI
NTSTATUS
NTAPI
PsLookupProcessByProcessId(
  IN HANDLE ProcessId,
  OUT PEPROCESS *Process);

NTKERNELAPI
NTSTATUS
NTAPI
PsLookupThreadByThreadId(
  IN HANDLE UniqueThreadId,
  OUT PETHREAD *Thread);
$endif (_NTIFS_)

#if (NTDDI_VERSION >= NTDDI_WIN2K)

$if (_WDMDDK_)
NTKERNELAPI
NTSTATUS
NTAPI
PsCreateSystemThread(
  OUT PHANDLE ThreadHandle,
  IN ULONG DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
  IN HANDLE ProcessHandle OPTIONAL,
  OUT PCLIENT_ID ClientId OPTIONAL,
  IN PKSTART_ROUTINE StartRoutine,
  IN PVOID StartContext OPTIONAL);

NTKERNELAPI
NTSTATUS
NTAPI
PsTerminateSystemThread(
  IN NTSTATUS ExitStatus);

$endif (_WDMDDK_)
$if (_NTDDK_)

NTKERNELAPI
NTSTATUS
NTAPI
PsSetCreateProcessNotifyRoutine(
  IN PCREATE_PROCESS_NOTIFY_ROUTINE NotifyRoutine,
  IN BOOLEAN Remove);

NTKERNELAPI
NTSTATUS
NTAPI
PsSetCreateThreadNotifyRoutine(
  IN PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine);

NTKERNELAPI
NTSTATUS
NTAPI
PsSetLoadImageNotifyRoutine(
  IN PLOAD_IMAGE_NOTIFY_ROUTINE NotifyRoutine);

NTKERNELAPI
HANDLE
NTAPI
PsGetCurrentProcessId(VOID);

NTKERNELAPI
HANDLE
NTAPI
PsGetCurrentThreadId(VOID);

NTKERNELAPI
BOOLEAN
NTAPI
PsGetVersion(
  OUT PULONG MajorVersion OPTIONAL,
  OUT PULONG MinorVersion OPTIONAL,
  OUT PULONG BuildNumber OPTIONAL,
  OUT PUNICODE_STRING CSDVersion OPTIONAL);
$endif (_NTDDK_)
$if (_NTIFS_)

NTKERNELAPI
PACCESS_TOKEN
NTAPI
PsReferenceImpersonationToken(
  IN OUT PETHREAD Thread,
  OUT PBOOLEAN CopyOnOpen,
  OUT PBOOLEAN EffectiveOnly,
  OUT PSECURITY_IMPERSONATION_LEVEL ImpersonationLevel);

NTKERNELAPI
LARGE_INTEGER
NTAPI
PsGetProcessExitTime(VOID);

NTKERNELAPI
BOOLEAN
NTAPI
PsIsThreadTerminating(
  IN PETHREAD Thread);

NTKERNELAPI
NTSTATUS
NTAPI
PsImpersonateClient(
  IN OUT PETHREAD Thread,
  IN PACCESS_TOKEN Token,
  IN BOOLEAN CopyOnOpen,
  IN BOOLEAN EffectiveOnly,
  IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel);

NTKERNELAPI
BOOLEAN
NTAPI
PsDisableImpersonation(
  IN OUT PETHREAD Thread,
  IN OUT PSE_IMPERSONATION_STATE ImpersonationState);

NTKERNELAPI
VOID
NTAPI
PsRestoreImpersonation(
  IN PETHREAD Thread,
  IN PSE_IMPERSONATION_STATE ImpersonationState);

NTKERNELAPI
VOID
NTAPI
PsRevertToSelf(VOID);

NTKERNELAPI
VOID
NTAPI
PsChargePoolQuota(
  IN PEPROCESS Process,
  IN POOL_TYPE PoolType,
  IN ULONG_PTR Amount);

NTKERNELAPI
VOID
NTAPI
PsReturnPoolQuota(
  IN PEPROCESS Process,
  IN POOL_TYPE PoolType,
  IN ULONG_PTR Amount);

NTKERNELAPI
NTSTATUS
NTAPI
PsAssignImpersonationToken(
  IN PETHREAD Thread,
  IN HANDLE Token OPTIONAL);

NTKERNELAPI
HANDLE
NTAPI
PsReferencePrimaryToken(
  IN OUT PEPROCESS Process);
$endif (_NTIFS_)
#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */
$if (_NTDDK_ || _NTIFS_)
#if (NTDDI_VERSION >= NTDDI_WINXP)
$endif (_NTDDK_ || _NTIFS_)

$if (_NTDDK_)
NTKERNELAPI
HANDLE
NTAPI
PsGetProcessId(
  IN PEPROCESS Process);

NTKERNELAPI
HANDLE
NTAPI
PsGetThreadId(
  IN PETHREAD Thread);

NTKERNELAPI
NTSTATUS
NTAPI
PsRemoveCreateThreadNotifyRoutine(
  IN PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine);

NTKERNELAPI
NTSTATUS
NTAPI
PsRemoveLoadImageNotifyRoutine(
  IN PLOAD_IMAGE_NOTIFY_ROUTINE NotifyRoutine);

NTKERNELAPI
LONGLONG
NTAPI
PsGetProcessCreateTimeQuadPart(
  IN PEPROCESS Process);
$endif (_NTDDK_)
$if (_NTIFS_)

NTKERNELAPI
VOID
NTAPI
PsDereferencePrimaryToken(
  IN PACCESS_TOKEN PrimaryToken);

NTKERNELAPI
VOID
NTAPI
PsDereferenceImpersonationToken(
  IN PACCESS_TOKEN ImpersonationToken);

NTKERNELAPI
NTSTATUS
NTAPI
PsChargeProcessPoolQuota(
  IN PEPROCESS Process,
  IN POOL_TYPE PoolType,
  IN ULONG_PTR Amount);

NTKERNELAPI
BOOLEAN
NTAPI
PsIsSystemThread(
  IN PETHREAD Thread);
$endif (_NTIFS_)
$if (_NTDDK_ || _NTIFS_)
#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */
$endif (_NTDDK_ || _NTIFS_)

$if (_NTDDK_)
#if (NTDDI_VERSION >= NTDDI_WS03)
NTKERNELAPI
HANDLE
NTAPI
PsGetThreadProcessId(
  IN PETHREAD Thread);
#endif /* (NTDDI_VERSION >= NTDDI_WS03) */

#if (NTDDI_VERSION >= NTDDI_VISTA)

NTKERNELAPI
BOOLEAN
NTAPI
PsSetCurrentThreadPrefetching(
  IN BOOLEAN Prefetching);

NTKERNELAPI
BOOLEAN
NTAPI
PsIsCurrentThreadPrefetching(VOID);

#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

#if (NTDDI_VERSION >= NTDDI_VISTASP1)
NTKERNELAPI
NTSTATUS
NTAPI
PsSetCreateProcessNotifyRoutineEx(
  IN PCREATE_PROCESS_NOTIFY_ROUTINE_EX NotifyRoutine,
  IN BOOLEAN Remove);
#endif /* (NTDDI_VERSION >= NTDDI_VISTASP1) */
$endif (_NTDDK_)
