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

#include <ntos.h>

#define NDEBUG
#include <ntdll/ntdll.h>

/* FUNCTIONS ***************************************************************/

/*
 @implemented
*/
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
 
 return RtlRosCreateUserThread
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

/*
 @implemented
*/
NTSTATUS STDCALL
RtlInitializeContext(
  IN HANDLE ProcessHandle,
  OUT PCONTEXT ThreadContext,
  IN PVOID ThreadStartParam  OPTIONAL,
  IN PTHREAD_START_ROUTINE ThreadStartAddress,
  IN PINITIAL_TEB InitialTeb)
{
 return RtlRosInitializeContext
 (
  ProcessHandle,
  ThreadContext,
  ThreadStartAddress,
  InitialTeb,
  1,
  (ULONG_PTR *)&ThreadStartParam
 );
}

/*
 @implemented
*/
NTSTATUS STDCALL RtlFreeUserThreadStack
(
 HANDLE ProcessHandle,
 HANDLE ThreadHandle
)
{
 return RtlRosFreeUserThreadStack(ProcessHandle, ThreadHandle);
}

/*
 @implemented
*/
NTSTATUS STDCALL RtlExitUserThread(NTSTATUS Status)
{
 RtlRosExitUserThread(Status);
}

/* EOF */
