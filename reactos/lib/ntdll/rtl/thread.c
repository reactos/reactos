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
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <napi/i386/segment.h>
#include <napi/teb.h>
#include <ntdll/rtl.h>

//#define NDEBUG
#include <ntdll/ntdll.h>


/* FUNCTIONS ***************************************************************/

NTSTATUS STDCALL RtlInitializeContextEx
(
 HANDLE ProcessHandle,
 PCONTEXT Context,
 PTHREAD_START_ROUTINE StartAddress,
 PUSER_STACK UserStack,
 ULONG ParameterCount,
 ULONG_PTR * Parameters
);

NTSTATUS STDCALL RtlCreateUserThreadEx
(
 HANDLE ProcessHandle,
 POBJECT_ATTRIBUTES ObjectAttributes,
 BOOLEAN CreateSuspended,
 LONG StackZeroBits,
 PULONG StackReserve,
 PULONG StackCommit,
 PTHREAD_START_ROUTINE StartAddress,
 PHANDLE ThreadHandle,
 PCLIENT_ID ClientId,
 ULONG ParameterCount,
 ULONG_PTR * Parameters
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

 ULONG i;

 DPRINT("blah\n");

 for(i = 0; i < ParameterCount; ++ i)
  DPRINT("parameter %lu = %p\n", i, Parameters[i]);

 DPRINT("doh\n");

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
  DPRINT("Fixed stack\n");

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

  DPRINT("Stack base %p\n", usUserStack.FixedStackBase);
  DPRINT("Stack limit %p\n", usUserStack.FixedStackLimit);
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
 nErrCode = RtlInitializeContextEx
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

NTSTATUS CDECL RtlCreateUserThreadVa
(
 HANDLE ProcessHandle,
 POBJECT_ATTRIBUTES ObjectAttributes,
 BOOLEAN CreateSuspended,
 LONG StackZeroBits,
 PULONG StackReserve,
 PULONG StackCommit,
 PTHREAD_START_ROUTINE StartAddress,
 PHANDLE ThreadHandle,
 PCLIENT_ID ClientId,
 ULONG ParameterCount,
 ...
)
{
 va_list vaArgs;
 NTSTATUS nErrCode;
 
 va_start(vaArgs, ParameterCount);
 
 /* FIXME: this code assumes a stack growing downwards */
 nErrCode = RtlCreateUserThreadEx
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
 
 return RtlCreateUserThreadEx
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

NTSTATUS STDCALL RtlInitializeContextEx
(
 HANDLE ProcessHandle,
 PCONTEXT Context,
 PTHREAD_START_ROUTINE StartAddress,
 PUSER_STACK UserStack,
 ULONG ParameterCount,
 ULONG_PTR * Parameters
)
{
 ULONG nDummy;
 PCHAR pStackBase;
 PCHAR pStackLimit;
 ULONG_PTR nRetAddr = 0xDEADBEEF;
 SIZE_T nParamsSize = ParameterCount * sizeof(ULONG_PTR);
 NTSTATUS nErrCode;
 
 if(UserStack->FixedStackBase != NULL && UserStack->FixedStackLimit != NULL)
 {
  pStackBase = UserStack->FixedStackBase;
  pStackLimit = UserStack->FixedStackLimit;
 }
 else if(UserStack->ExpandableStackBase != NULL)
 {
  pStackBase = UserStack->ExpandableStackBase;
  pStackLimit = UserStack->ExpandableStackLimit;
 }
 else
  return STATUS_BAD_INITIAL_STACK;

 if(pStackBase <= pStackLimit)
  return STATUS_BAD_INITIAL_STACK;

 /* too many parameters */
 if((nParamsSize + sizeof(ULONG_PTR)) > (SIZE_T)(pStackBase - pStackLimit))
  return STATUS_STACK_OVERFLOW;

#if defined(_M_IX86)
 memset(Context, 0, sizeof(CONTEXT));

 Context->ContextFlags = CONTEXT_FULL;

 Context->Eip = (ULONG_PTR)StartAddress;
 Context->SegGs = USER_DS;
 Context->SegFs = TEB_SELECTOR;
 Context->SegEs = USER_DS;
 Context->SegDs = USER_DS;
 Context->SegCs = USER_CS;
 Context->SegSs = USER_DS;
 Context->Esp = (ULONG_PTR)pStackBase - (nParamsSize + sizeof(ULONG_PTR));
 Context->EFlags = ((ULONG_PTR)1 << 1) | ((ULONG_PTR)1 << 9);

 DPRINT("Effective stack base %p\n", Context->Esp);
#else
#error Unsupported architecture
#endif

 /* write the parameters */
 nErrCode = NtWriteVirtualMemory
 (
  ProcessHandle,
  ((PUCHAR)pStackBase) - nParamsSize,
  Parameters,
  nParamsSize,
  &nDummy
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode)) return nErrCode;

 /* write the return address */
 return NtWriteVirtualMemory
 (
  ProcessHandle,
  ((PUCHAR)pStackBase) - (nParamsSize + sizeof(ULONG_PTR)),
  &nRetAddr,
  sizeof(ULONG_PTR),
  &nDummy
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
 return RtlInitializeContextEx
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
