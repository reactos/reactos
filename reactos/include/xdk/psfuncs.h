/******************************************************************************
 *                          Process Manager Functions                         *
 ******************************************************************************/
$if (_WDMDDK_)

NTKERNELAPI
NTSTATUS
NTAPI
PsWrapApcWow64Thread(
  _Inout_ PVOID *ApcContext,
  _Inout_ PVOID *ApcRoutine);

/*
 * PEPROCESS
 * PsGetCurrentProcess(VOID)
 */
#define PsGetCurrentProcess IoGetCurrentProcess

#if !defined(_PSGETCURRENTTHREAD_)
#define _PSGETCURRENTTHREAD_
_IRQL_requires_max_(DISPATCH_LEVEL)
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

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenProcess(
  _Out_ PHANDLE ProcessHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_opt_ PCLIENT_ID ClientId);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationProcess(
  _In_ HANDLE ProcessHandle,
  _In_ PROCESSINFOCLASS ProcessInformationClass,
  _Out_ PVOID ProcessInformation,
  _In_ ULONG ProcessInformationLength,
  _Out_opt_ PULONG ReturnLength);
$endif (_NTDDK_)
$if (_NTIFS_)

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
PsLookupProcessByProcessId(
  _In_ HANDLE ProcessId,
  _Outptr_ PEPROCESS *Process);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
PsLookupThreadByThreadId(
  _In_ HANDLE UniqueThreadId,
  _Outptr_ PETHREAD *Thread);
$endif (_NTIFS_)

#if (NTDDI_VERSION >= NTDDI_WIN2K)

$if (_WDMDDK_)
_IRQL_requires_max_(APC_LEVEL)
_Post_satisfies_(return <= 0)
_Must_inspect_result_
NTKERNELAPI
NTSTATUS
NTAPI
PsCreateSystemThread(
  _Out_ PHANDLE ThreadHandle,
  _In_ ULONG DesiredAccess,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_opt_ HANDLE ProcessHandle,
  _Out_opt_ PCLIENT_ID ClientId,
  _In_ PKSTART_ROUTINE StartRoutine,
  _In_opt_ _When_(return==0, __drv_aliasesMem) PVOID StartContext);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
PsTerminateSystemThread(
  _In_ NTSTATUS ExitStatus);

$endif (_WDMDDK_)
$if (_NTDDK_)

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
PsSetCreateProcessNotifyRoutine(
  _In_ PCREATE_PROCESS_NOTIFY_ROUTINE NotifyRoutine,
  _In_ BOOLEAN Remove);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
PsSetCreateThreadNotifyRoutine(
  _In_ PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
PsSetLoadImageNotifyRoutine(
  _In_ PLOAD_IMAGE_NOTIFY_ROUTINE NotifyRoutine);

NTKERNELAPI
HANDLE
NTAPI
PsGetCurrentProcessId(VOID);

_IRQL_requires_max_(DISPATCH_LEVEL)
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

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
PACCESS_TOKEN
NTAPI
PsReferenceImpersonationToken(
  _Inout_ PETHREAD Thread,
  _Out_ PBOOLEAN CopyOnOpen,
  _Out_ PBOOLEAN EffectiveOnly,
  _Out_ PSECURITY_IMPERSONATION_LEVEL ImpersonationLevel);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
LARGE_INTEGER
NTAPI
PsGetProcessExitTime(VOID);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
PsIsThreadTerminating(
  _In_ PETHREAD Thread);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
PsImpersonateClient(
  _Inout_ PETHREAD Thread,
  _In_opt_ PACCESS_TOKEN Token,
  _In_ BOOLEAN CopyOnOpen,
  _In_ BOOLEAN EffectiveOnly,
  _In_ SECURITY_IMPERSONATION_LEVEL ImpersonationLevel);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
PsDisableImpersonation(
  _Inout_ PETHREAD Thread,
  _Inout_ PSE_IMPERSONATION_STATE ImpersonationState);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
PsRestoreImpersonation(
  _Inout_ PETHREAD Thread,
  _In_ PSE_IMPERSONATION_STATE ImpersonationState);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
PsRevertToSelf(VOID);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
PsChargePoolQuota(
  _In_ PEPROCESS Process,
  _In_ POOL_TYPE PoolType,
  _In_ ULONG_PTR Amount);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
PsReturnPoolQuota(
  _In_ PEPROCESS Process,
  _In_ POOL_TYPE PoolType,
  _In_ ULONG_PTR Amount);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
PsAssignImpersonationToken(
  _In_ PETHREAD Thread,
  _In_opt_ HANDLE Token);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
HANDLE
NTAPI
PsReferencePrimaryToken(
  _Inout_ PEPROCESS Process);
$endif (_NTIFS_)
#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */
$if (_NTDDK_ || _NTIFS_)
#if (NTDDI_VERSION >= NTDDI_WINXP)
$endif (_NTDDK_ || _NTIFS_)

$if (_NTDDK_)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
HANDLE
NTAPI
PsGetProcessId(
  _In_ PEPROCESS Process);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
HANDLE
NTAPI
PsGetThreadId(
  _In_ PETHREAD Thread);

NTKERNELAPI
NTSTATUS
NTAPI
PsRemoveCreateThreadNotifyRoutine(
  _In_ PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
PsRemoveLoadImageNotifyRoutine(
  _In_ PLOAD_IMAGE_NOTIFY_ROUTINE NotifyRoutine);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
LONGLONG
NTAPI
PsGetProcessCreateTimeQuadPart(
  _In_ PEPROCESS Process);
$endif (_NTDDK_)
$if (_NTIFS_)

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
PsDereferencePrimaryToken(
  _In_ PACCESS_TOKEN PrimaryToken);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
PsDereferenceImpersonationToken(
  _In_ PACCESS_TOKEN ImpersonationToken);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
PsChargeProcessPoolQuota(
  _In_ PEPROCESS Process,
  _In_ POOL_TYPE PoolType,
  _In_ ULONG_PTR Amount);

NTKERNELAPI
BOOLEAN
NTAPI
PsIsSystemThread(
  _In_ PETHREAD Thread);
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
