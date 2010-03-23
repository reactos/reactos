/******************************************************************************
 *                          Process Manager Functions                         *
 ******************************************************************************/
$if (_NTDDK_)
extern NTKERNELAPI PEPROCESS PsInitialSystemProcess;
$endif

$if (_WDMDDK_)
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

#endif
$endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)

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
$endif

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
$endif

#endif

$if (_NTDDK_)
#if (NTDDI_VERSION >= NTDDI_WINXP)

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

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

#if (NTDDI_VERSION >= NTDDI_WS03)
NTKERNELAPI
HANDLE
NTAPI
PsGetThreadProcessId(
  IN PETHREAD Thread);
#endif

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

#endif

#if (NTDDI_VERSION >= NTDDI_VISTASP1)
NTKERNELAPI
NTSTATUS
NTAPI
PsSetCreateProcessNotifyRoutineEx(
  IN PCREATE_PROCESS_NOTIFY_ROUTINE_EX NotifyRoutine,
  IN BOOLEAN Remove);
#endif
$endif

$if (_WDMDDK_)
NTKERNELAPI
NTSTATUS
NTAPI
PsWrapApcWow64Thread(
  IN OUT PVOID *ApcContext,
  IN OUT PVOID *ApcRoutine);
$end

