/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Rtl user thread functions
 * FILE:              lib/ntdll/rtl/thread.c
 * PROGRAMER:         Eric Kohl
 * REVISION HISTORY:
 *                    09/07/99: Created
 *                    09/10/99: Cleanup and full stack support.
 *                    25/04/03: Near rewrite. Made code more readable, replaced
 *                              INITIAL_TEB with USER_STACK, added support for
 *                              fixed-size stacks
 *                    28/04/03: Moved all code to a new statically linked
 *                              library (ROSRTL) so it can be shared with
 *                              kernel32.dll without exporting non-standard
 *                              functions from ntdll.dll
 */

/* INCLUDES *****************************************************************/

#define NTOS_MODE_USER
#include <ntos.h>

#define NDEBUG
#include <ntdll/ntdll.h>

/* FUNCTIONS ***************************************************************/

NTSTATUS STDCALL RtlCreateUserThread
(
 HANDLE ProcessHandle,
 PSECURITY_DESCRIPTOR SecurityDescriptor,
 BOOLEAN CreateSuspended,
 LONG StackZeroBits,
 PULONG StackReserve,
 PULONG StackCommit,
 PTHREAD_START_ROUTINE StartAddress,
 PVOID Parameter,
 PHANDLE ThreadHandle,
 PCLIENT_ID ClientId
)
{
 OBJECT_ATTRIBUTES oaThreadAttribs;
 
 InitializeObjectAttributes
 (
  &oaThreadAttribs,
  NULL,
  0,
  NULL,
  SecurityDescriptor
 );
 
 return RtlRosCreateUserThreadEx
 (
  ProcessHandle,
  &oaThreadAttribs,
  CreateSuspended,
  StackZeroBits,
  StackReserve,
  StackCommit,
  StartAddress,
  ThreadHandle,
  ClientId,
  1,
  (ULONG_PTR *)&Parameter
 );
}

NTSTATUS STDCALL RtlInitializeContext
(
 HANDLE ProcessHandle,
 PCONTEXT Context,
 PVOID Parameter,
 PTHREAD_START_ROUTINE StartAddress,
 PUSER_STACK UserStack
)
{
 return RtlRosInitializeContextEx
 (
  ProcessHandle,
  Context,
  StartAddress,
  UserStack,
  1,
  (ULONG_PTR *)&Parameter
 );
}

NTSTATUS STDCALL RtlFreeUserThreadStack
(
 HANDLE ProcessHandle,
 HANDLE ThreadHandle
)
{
 THREAD_BASIC_INFORMATION tbiInfo;
 NTSTATUS nErrCode;
 ULONG nDummy;
 ULONG nSize = 0;
 PVOID pStackBase;
 PTEB pTeb;

 /* query basic information about the thread */
 nErrCode = NtQueryInformationThread
 (
  ThreadHandle,
  ThreadBasicInformation,
  &tbiInfo,
  sizeof(tbiInfo),
  NULL
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode)) return nErrCode;
 if(tbiInfo.TebBaseAddress == NULL) return STATUS_ACCESS_VIOLATION;

 pTeb = (PTEB)tbiInfo.TebBaseAddress;

 /* read the base address of the stack to be deallocated */
 nErrCode = NtReadVirtualMemory
 (
  ProcessHandle,
  &pTeb->DeallocationStack,
  &pStackBase,
  sizeof(pStackBase),
  &nDummy
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode)) return nErrCode;
 if(pStackBase == NULL) return STATUS_ACCESS_VIOLATION;

 /* deallocate the stack */
 nErrCode = NtFreeVirtualMemory(ProcessHandle, pStackBase, &nSize, MEM_RELEASE);

 return nErrCode;
}

NTSTATUS STDCALL RtlExitUserThread(NTSTATUS nErrCode)
{
 return NtTerminateThread(NtCurrentThread(), nErrCode);
}

/* EOF */
