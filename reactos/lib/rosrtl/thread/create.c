/* $Id: create.c,v 1.1 2003/04/29 02:17:01 hyperion Exp $
*/
/*
*/

#include <stdarg.h>
#include <ddk/ntddk.h>
#include <rosrtl/thread.h>

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
 IN PTHREAD_START_ROUTINE StartAddress,
 OUT PHANDLE ThreadHandle OPTIONAL,
 OUT PCLIENT_ID ClientId OPTIONAL,
 IN ULONG ParameterCount,
 IN ULONG_PTR * Parameters
)
{
 USER_STACK usUserStack;
 OBJECT_ATTRIBUTES oaThreadAttribs;
 /* FIXME: read the defaults from the executable image */
 ULONG_PTR nStackReserve = 0x100000;
 /* FIXME: when we finally have exception handling, make this PAGE_SIZE */
 ULONG_PTR nStackCommit = 0x100000;
 ULONG_PTR nSize = 0;
 PVOID pStackLowest = NULL;
 ULONG nDummy;
 CONTEXT ctxInitialContext;
 NTSTATUS nErrCode;
 HANDLE hThread;
 CLIENT_ID cidClientId;

 if(ThreadHandle == NULL) ThreadHandle = &hThread;
 if(ClientId == NULL) ClientId = &cidClientId;
 
 if(StackReserve == NULL) StackReserve = &nStackReserve;
 else ROUNDUP(*StackReserve, PAGE_SIZE);

 if(StackCommit == NULL) StackCommit = &nStackCommit;
 else ROUNDUP(*StackCommit, PAGE_SIZE);

#if 0
 /* the stack commit size must be equal to or less than the reserve size */
 if(*StackCommit > *StackReserve) *StackCommit = *StackReserve;
#else
 /* FIXME: no SEH, no guard pages */
 *StackCommit = *StackReserve;
#endif

 usUserStack.FixedStackBase = NULL;
 usUserStack.FixedStackLimit =  NULL;
 usUserStack.ExpandableStackBase = NULL;
 usUserStack.ExpandableStackLimit = NULL; 
 usUserStack.ExpandableStackBottom = NULL;

 /* FIXME: this code assumes a stack growing downwards */
 /* fixed stack */
 if(*StackCommit == *StackReserve)
 {
  usUserStack.FixedStackLimit = NULL;

  /* allocate the stack */
  nErrCode = NtAllocateVirtualMemory
  (
   ProcessHandle,
   &(usUserStack.FixedStackLimit),
   StackZeroBits,
   StackReserve,
   MEM_RESERVE | MEM_COMMIT,
   PAGE_READWRITE
  );

  /* failure */
  if(!NT_SUCCESS(nErrCode)) goto l_Fail;

  /* store the highest (first) address of the stack */
  usUserStack.FixedStackBase =
   (PUCHAR)(usUserStack.FixedStackLimit) + *StackReserve;

  *StackCommit = *StackReserve;
 }
 /* expandable stack */
 else
 {
  ULONG_PTR nGuardSize = PAGE_SIZE;
  PVOID pGuardBase;

  DPRINT("Expandable stack\n");

  usUserStack.FixedStackLimit = NULL;
  usUserStack.FixedStackBase = NULL;
  usUserStack.ExpandableStackBottom = NULL;

  /* reserve the stack */
  nErrCode = NtAllocateVirtualMemory
  (
   ProcessHandle,
   &(usUserStack.ExpandableStackBottom),
   StackZeroBits,
   StackReserve,
   MEM_RESERVE,
   PAGE_READWRITE
  );

  /* failure */
  if(!NT_SUCCESS(nErrCode)) goto l_Fail;

  DPRINT("Reserved %08X bytes\n", *StackReserve);

  /* expandable stack base - the highest address of the stack */
  usUserStack.ExpandableStackBase =
   (PUCHAR)(usUserStack.ExpandableStackBottom) + *StackReserve;

  /* expandable stack limit - the lowest committed address of the stack */
  usUserStack.ExpandableStackLimit =
   (PUCHAR)(usUserStack.ExpandableStackBase) - *StackCommit;

  DPRINT("Stack base   %p\n", usUserStack.ExpandableStackBase);
  DPRINT("Stack limit  %p\n", usUserStack.ExpandableStackLimit);
  DPRINT("Stack bottom %p\n", usUserStack.ExpandableStackBottom);

  /* commit as much stack as requested */
  nErrCode = NtAllocateVirtualMemory
  (
   ProcessHandle,
   &(usUserStack.ExpandableStackLimit),
   0,
   StackCommit,
   MEM_COMMIT,
   PAGE_READWRITE
  );

  /* failure */
  if(!NT_SUCCESS(nErrCode)) goto l_Fail;

  assert((*StackReserve - *StackCommit) >= PAGE_SIZE);
  assert((*StackReserve - *StackCommit) % PAGE_SIZE == 0);

  pGuardBase = (PUCHAR)(usUserStack.ExpandableStackLimit) - PAGE_SIZE;

  DPRINT("Guard base %p\n", usUserStack.ExpandableStackBase);

  /* set up the guard page */
  nErrCode = NtAllocateVirtualMemory
  (
   ProcessHandle,
   &pGuardBase,
   0,
   &nGuardSize,
   MEM_COMMIT,
   PAGE_READWRITE | PAGE_GUARD
  );

  /* failure */
  if(!NT_SUCCESS(nErrCode)) goto l_Fail;

  DPRINT("Guard base %p\n", usUserStack.ExpandableStackBase);
 }

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
 if(!NT_SUCCESS(nErrCode)) goto l_Cleanup;

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
 if(!NT_SUCCESS(nErrCode)) goto l_Cleanup;

 /* success */
 return STATUS_SUCCESS;

 /* deallocate the stack */
l_Cleanup:
 if(usUserStack.FixedStackLimit)
  pStackLowest = usUserStack.FixedStackLimit;
 else if(usUserStack.ExpandableStackBottom)
  pStackLowest = usUserStack.ExpandableStackBottom;

 /* free the stack, if it was allocated */
 if(pStackLowest != NULL)
  NtFreeVirtualMemory(ProcessHandle, &pStackLowest, &nSize, MEM_RELEASE);

 /* failure */
l_Fail:
 assert(!NT_SUCCESS(nErrCode));
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
 IN PTHREAD_START_ROUTINE StartAddress,
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
