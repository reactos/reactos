/* $Id: create.c,v 1.3 2003/06/01 14:59:02 chorns Exp $
*/
/*
*/

#include <stdarg.h>
#define NTOS_MODE_USER
#include <ntos.h>

#define NDEBUG
#include <ntdll/ntdll.h>

NTSTATUS STDCALL RtlRosCreateUserThreadEx
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
 OBJECT_ATTRIBUTES oaThreadAttribs;
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
 nErrCode = RtlRosInitializeContextEx
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
 assert(!NT_SUCCESS(nErrCode));

 /* deallocate the stack */
 RtlRosDeleteStack(ProcessHandle, &usUserStack);
 
 return nErrCode;
}

NTSTATUS CDECL RtlRosCreateUserThreadVa
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
 
 /* FIXME: this code assumes a stack growing downwards */
 nErrCode = RtlRosCreateUserThreadEx
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
