/* $Id: create.c,v 1.6 2004/03/02 17:16:04 navaraf Exp $
*/
/*
*/

#include <stdarg.h>
#define NTOS_MODE_USER
#include <ntos.h>

#define NDEBUG
#include <ntdll/ntdll.h>

#include <rosrtl/thread.h>

NTSTATUS STDCALL
RtlRosCreateUserThread
(
 IN HANDLE ProcessHandle,
 IN POBJECT_ATTRIBUTES ObjectAttributes,
 IN BOOLEAN CreateSuspended,
 IN LONG StackZeroBits,
 IN OUT PULONG StackReserve OPTIONAL,
 IN OUT PULONG StackCommit OPTIONAL,
 IN PVOID StartAddress,
 OUT PHANDLE ThreadHandle OPTIONAL,
 OUT PCLIENT_ID ClientId OPTIONAL,
 IN ULONG ParameterCount,
 IN ULONG_PTR * Parameters
)
{
 USER_STACK usUserStack;
 CONTEXT ctxInitialContext;
 NTSTATUS nErrCode;
 HANDLE hThread;
 CLIENT_ID cidClientId;

 if(ThreadHandle == NULL) ThreadHandle = &hThread;
 if(ClientId == NULL) ClientId = &cidClientId;

 /* allocate the stack for the thread */
 nErrCode = RtlRosCreateStack
 (
  ProcessHandle,
  &usUserStack,
  StackZeroBits,
  StackReserve,
  StackCommit
 );
 
 /* failure */
 if(!NT_SUCCESS(nErrCode)) goto l_Fail;

 /* initialize the registers and stack for the thread */
 nErrCode = RtlRosInitializeContext
 (
  ProcessHandle,
  &ctxInitialContext,
  StartAddress,
  &usUserStack,
  ParameterCount,
  Parameters
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode)) goto l_Fail;

 /* create the thread object */
 nErrCode = NtCreateThread
 (
  ThreadHandle,
  THREAD_ALL_ACCESS,
  ObjectAttributes,
  ProcessHandle,
  ClientId,
  &ctxInitialContext,
  &usUserStack,
  CreateSuspended
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode)) goto l_Fail;

 /* success */
 return STATUS_SUCCESS;

 /* failure */
l_Fail:
 ASSERT(!NT_SUCCESS(nErrCode));

 /* deallocate the stack */
 RtlRosDeleteStack(ProcessHandle, &usUserStack);
 
 return nErrCode;
}

NTSTATUS CDECL
RtlRosCreateUserThreadVa
(
 IN HANDLE ProcessHandle,
 IN POBJECT_ATTRIBUTES ObjectAttributes,
 IN BOOLEAN CreateSuspended,
 IN LONG StackZeroBits,
 IN OUT PULONG StackReserve OPTIONAL,
 IN OUT PULONG StackCommit OPTIONAL,
 IN PVOID StartAddress,
 OUT PHANDLE ThreadHandle OPTIONAL,
 OUT PCLIENT_ID ClientId OPTIONAL,
 IN ULONG ParameterCount,
 ...
)
{
 va_list vaArgs;
 NTSTATUS nErrCode;
 
 va_start(vaArgs, ParameterCount);
 
 /*
  FIXME: this code makes several non-portable assumptions:
   - all parameters are passed on the stack
   - the stack is a contiguous array of cells as large as an ULONG_PTR
   - the stack grows downwards

  This happens to work on the Intel x86, but is likely to bomb horribly on most
  other platforms
 */
 nErrCode = RtlRosCreateUserThread
 (
  ProcessHandle,
  ObjectAttributes,
  CreateSuspended,
  StackZeroBits,
  StackReserve,
  StackCommit,
  StartAddress,
  ThreadHandle,
  ClientId,
  ParameterCount,
  (ULONG_PTR *)vaArgs
 );

 va_end(vaArgs);
 
 return nErrCode;
}

/* EOF */
