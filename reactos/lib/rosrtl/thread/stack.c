/* $Id: stack.c,v 1.6.18.1 2004/10/24 23:07:04 ion Exp $
*/
/*
*/
#include <windows.h>
#include <ndk/umtypes.h>
#include <ndk/zwfuncs.h>
#include <ndk/pstypes.h>
#include <reactos/wine/debug.h>
#include <reactos/helper.h>
#include "thread.h"

NTSTATUS NTAPI RtlRosCreateStack
(
 IN HANDLE ProcessHandle,
 OUT PINITIAL_TEB InitialTeb,
 IN LONG StackZeroBits,
 IN OUT PULONG StackReserve OPTIONAL,
 IN OUT PULONG StackCommit OPTIONAL
)
{
 /* FIXME: read the defaults from the executable image */
 ULONG_PTR nStackReserve = 0x100000;
 /* FIXME: when we finally have exception handling, make this PAGE_SIZE */
 ULONG_PTR nStackCommit = 0x100000;
 NTSTATUS nErrCode;

 if(StackReserve == NULL) StackReserve = &nStackReserve;
 else *StackReserve = ROUNDUP(*StackReserve, PAGE_SIZE);

 if(StackCommit == NULL) StackCommit = &nStackCommit;
 else *StackCommit = ROUNDUP(*StackCommit, PAGE_SIZE);

#if 0
 /* the stack commit size must be equal to or less than the reserve size */
 if(*StackCommit > *StackReserve) *StackCommit = *StackReserve;
#else
 /* FIXME: no SEH, no guard pages */
 *StackCommit = *StackReserve;
#endif

 /* FIXME: this code assumes a stack growing downwards */
 /* fixed stack */
 if(*StackCommit == *StackReserve)
 {
  InitialTeb->ExpandableStackBase = NULL;
  InitialTeb->ExpandableStackLimit = NULL; 
  InitialTeb->ExpandableStackBottom = NULL;

  InitialTeb->FixedStackLimit = NULL;

  /* allocate the stack */
  nErrCode = NtAllocateVirtualMemory
  (
   ProcessHandle,
   &(InitialTeb->FixedStackLimit),
   StackZeroBits,
   StackReserve,
   MEM_RESERVE | MEM_COMMIT,
   PAGE_READWRITE
  );

  /* failure */
  if(!NT_SUCCESS(nErrCode)) goto l_Fail;

  /* store the highest (first) address of the stack */
  InitialTeb->FixedStackBase =
   (PUCHAR)(InitialTeb->FixedStackLimit) + *StackReserve;

  *StackCommit = *StackReserve;
 }
 /* expandable stack */
 else
 {
  ULONG_PTR nGuardSize = PAGE_SIZE;
  PVOID pGuardBase;

  DPRINT("Expandable stack\n");

  InitialTeb->FixedStackBase = NULL;
  InitialTeb->FixedStackLimit =  NULL;

  InitialTeb->ExpandableStackBottom = NULL;

  /* reserve the stack */
  nErrCode = NtAllocateVirtualMemory
  (
   ProcessHandle,
   &(InitialTeb->ExpandableStackBottom),
   StackZeroBits,
   StackReserve,
   MEM_RESERVE,
   PAGE_READWRITE
  );

  /* failure */
  if(!NT_SUCCESS(nErrCode)) goto l_Fail;

  DPRINT("Reserved %08X bytes\n", *StackReserve);

  /* expandable stack base - the highest address of the stack */
  InitialTeb->ExpandableStackBase =
   (PUCHAR)(InitialTeb->ExpandableStackBottom) + *StackReserve;

  /* expandable stack limit - the lowest committed address of the stack */
  InitialTeb->ExpandableStackLimit =
   (PUCHAR)(InitialTeb->ExpandableStackBase) - *StackCommit;

  DPRINT("Stack base   %p\n", InitialTeb->ExpandableStackBase);
  DPRINT("Stack limit  %p\n", InitialTeb->ExpandableStackLimit);
  DPRINT("Stack bottom %p\n", InitialTeb->ExpandableStackBottom);

  /* commit as much stack as requested */
  nErrCode = NtAllocateVirtualMemory
  (
   ProcessHandle,
   &(InitialTeb->ExpandableStackLimit),
   0,
   StackCommit,
   MEM_COMMIT,
   PAGE_READWRITE
  );

  /* failure */
  if(!NT_SUCCESS(nErrCode)) goto l_Cleanup;

  ASSERT((*StackReserve - *StackCommit) >= PAGE_SIZE);
  ASSERT((*StackReserve - *StackCommit) % PAGE_SIZE == 0);

  pGuardBase = (PUCHAR)(InitialTeb->ExpandableStackLimit) - PAGE_SIZE;

  DPRINT("Guard base %p\n", InitialTeb->ExpandableStackBase);

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
  if(!NT_SUCCESS(nErrCode)) goto l_Cleanup;

  DPRINT("Guard base %p\n", InitialTeb->ExpandableStackBase);
 }


 /* success */
 return STATUS_SUCCESS;

 /* deallocate the stack */
l_Cleanup:
 RtlRosDeleteStack(ProcessHandle, InitialTeb);

 /* failure */
l_Fail:
 ASSERT(!NT_SUCCESS(nErrCode));
 return nErrCode;
}

NTSTATUS NTAPI RtlRosDeleteStack
(
 IN HANDLE ProcessHandle,
 IN PINITIAL_TEB InitialTeb
)
{
 PVOID pStackLowest = NULL;
 ULONG_PTR nSize;

 if(InitialTeb->FixedStackLimit)
  pStackLowest = InitialTeb->FixedStackLimit;
 else if(InitialTeb->ExpandableStackBottom)
  pStackLowest = InitialTeb->ExpandableStackBottom;

 /* free the stack, if it was allocated */
 if(pStackLowest != NULL)
  return NtFreeVirtualMemory(ProcessHandle, &pStackLowest, &nSize, MEM_RELEASE);

 return STATUS_SUCCESS;
}

NTSTATUS NTAPI RtlRosFreeUserThreadStack
(
 IN HANDLE ProcessHandle,
 IN HANDLE ThreadHandle
)
{
 NTSTATUS nErrCode;
 ULONG nSize = 0;
 PVOID pStackBase;

 if(ThreadHandle == NtCurrentThread())
  pStackBase = NtCurrentTeb()->DeallocationStack;
 else
 {
  THREAD_BASIC_INFORMATION tbiInfo;
  ULONG nDummy;

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

  /* read the base address of the stack to be deallocated */
  nErrCode = NtReadVirtualMemory
  (
   ProcessHandle,
   &((PTEB)tbiInfo.TebBaseAddress)->DeallocationStack,
   &pStackBase,
   sizeof(pStackBase),
   &nDummy
  );
 
  /* failure */
  if(!NT_SUCCESS(nErrCode)) return nErrCode;
  if(pStackBase == NULL) return STATUS_ACCESS_VIOLATION;
 }

 /* deallocate the stack */
 nErrCode = NtFreeVirtualMemory(ProcessHandle, &pStackBase, &nSize, MEM_RELEASE);

 return nErrCode;
}

NTSTATUS NTAPI RtlpRosGetStackLimits
(
 IN PINITIAL_TEB InitialTeb,
 OUT PVOID * StackBase,
 OUT PVOID * StackLimit
)
{
 /* fixed-size stack */
 if(InitialTeb->FixedStackBase && InitialTeb->FixedStackLimit)
 {
  *StackBase = InitialTeb->FixedStackBase;
  *StackLimit = InitialTeb->FixedStackLimit;
 }
 /* expandable stack */
 else if(InitialTeb->ExpandableStackBase && InitialTeb->ExpandableStackLimit)
 {
  *StackBase = InitialTeb->ExpandableStackBase;
  *StackLimit = InitialTeb->ExpandableStackLimit;
 }
 /* can't determine the type of stack: failure */
 else
 {
  DPRINT("Invalid user-mode stack\n");
  return STATUS_BAD_INITIAL_STACK;
 }

 /* valid stack */
 return STATUS_SUCCESS;
}

/* EOF */
