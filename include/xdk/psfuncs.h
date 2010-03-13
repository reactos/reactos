/******************************************************************************
 *                          Process Manager Functions                         *
 ******************************************************************************/

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
PsGetCurrentThread (
  VOID)
{
  return (PETHREAD)KeGetCurrentThread();
}

#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)

NTKERNELAPI
NTSTATUS
NTAPI
PsCreateSystemThread(
  OUT PHANDLE  ThreadHandle,
  IN ULONG  DesiredAccess,
  IN POBJECT_ATTRIBUTES  ObjectAttributes  OPTIONAL,
  IN HANDLE  ProcessHandle  OPTIONAL,
  OUT PCLIENT_ID  ClientId  OPTIONAL,
  IN PKSTART_ROUTINE  StartRoutine,
  IN PVOID  StartContext OPTIONAL);

NTKERNELAPI
NTSTATUS
NTAPI
PsTerminateSystemThread(
  IN NTSTATUS  ExitStatus);

#endif

