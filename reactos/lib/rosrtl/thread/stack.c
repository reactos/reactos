/* $Id: stack.c,v 1.1 2003/05/29 00:36:41 hyperion Exp $
*/
/*
*/

#include <ddk/ntddk.h>
#include <rosrtl/thread.h>

#define NDEBUG
#include <ntdll/ntdll.h>

NTSTATUS NTAPI RtlRosCreateStack
(
 IN HANDLE ProcessHandle,
 OUT PUSER_STACK UserStack,
 IN LONG StackZeroBits,
 IN OUT PULONG StackReserve OPTIONAL,
 IN OUT PULONG StackCommit OPTIONAL
)
{
 /* FIXME: read the defaults from the executable image */
 ULONG_PTR nStackReserve = 0x100000;
 /* FIXME: when we finally have exception handling, make this PAGE_SIZE */
 ULONG_PTR nStackCommit = 0x100000;
 PVOID pStackLowest;
 ULONG_PTR nSize = 0;
 NTSTATUS nErrCode;

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

 /* FIXME: this code assumes a stack growing downwards */
 /* fixed stack */
 if(*StackCommit == *StackReserve)
 {
  UserStack->ExpandableStackBase = NULL;
  UserStack->ExpandableStackLimit = NULL; 
  UserStack->ExpandableStackBottom = NULL;

  UserStack->FixedStackLimit = NULL;

  /* allocate the stack */
  nErrCode = NtAllocateVirtualMemory
  (
   ProcessHandle,
   &(UserStack->FixedStackLimit),
   StackZeroBits,
   StackReserve,
   MEM_RESERVE | MEM_COMMIT,
   PAGE_READWRITE
  );

  /* failure */
  if(!NT_SUCCESS(nErrCode)) goto l_Fail;

  /* store the highest (first) address of the stack */
  UserStack->FixedStackBase =
   (PUCHAR)(UserStack->FixedStackLimit) + *StackReserve;

  *StackCommit = *StackReserve;
 }
 /* expandable stack */
 else
 {
  ULONG_PTR nGuardSize = PAGE_SIZE;
  PVOID pGuardBase;

  DPRINT("Expandable stack\n");

  UserStack->FixedStackBase = NULL;
  UserStack->FixedStackLimit =  NULL;

  UserStack->ExpandableStackBottom = NULL;

  /* reserve the stack */
  nErrCode = NtAllocateVirtualMemory
  (
   ProcessHandle,
   &(UserStack->ExpandableStackBottom),
   StackZeroBits,
   StackReserve,
   MEM_RESERVE,
   PAGE_READWRITE
  );

  /* failure */
  if(!NT_SUCCESS(nErrCode)) goto l_Fail;

  DPRINT("Reserved %08X bytes\n", *StackReserve);

  /* expandable stack base - the highest address of the stack */
  UserStack->ExpandableStackBase =
   (PUCHAR)(UserStack->ExpandableStackBottom) + *StackReserve;

  /* expandable stack limit - the lowest committed address of the stack */
  UserStack->ExpandableStackLimit =
   (PUCHAR)(UserStack->ExpandableStackBase) - *StackCommit;

  DPRINT("Stack base   %p\n", UserStack->ExpandableStackBase);
  DPRINT("Stack limit  %p\n", UserStack->ExpandableStackLimit);
  DPRINT("Stack bottom %p\n", UserStack->ExpandableStackBottom);

  /* commit as much stack as requested */
  nErrCode = NtAllocateVirtualMemory
  (
   ProcessHandle,
   &(UserStack->ExpandableStackLimit),
   0,
   StackCommit,
   MEM_COMMIT,
   PAGE_READWRITE
  );

  /* failure */
  if(!NT_SUCCESS(nErrCode)) goto l_Cleanup;

  assert((*StackReserve - *StackCommit) >= PAGE_SIZE);
  assert((*StackReserve - *StackCommit) % PAGE_SIZE == 0);

  pGuardBase = (PUCHAR)(UserStack->ExpandableStackLimit) - PAGE_SIZE;

  DPRINT("Guard base %p\n", UserStack->ExpandableStackBase);

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

  DPRINT("Guard base %p\n", UserStack->ExpandableStackBase);
 }


 /* success */
 return STATUS_SUCCESS;

 /* deallocate the stack */
l_Cleanup:
 RtlRosDeleteStack(ProcessHandle, UserStack);

 /* failure */
l_Fail:
 assert(!NT_SUCCESS(nErrCode));
 return nErrCode;
}

NTSTATUS NTAPI RtlRosDeleteStack
(
 IN HANDLE ProcessHandle,
 IN PUSER_STACK UserStack
)
{
 PVOID pStackLowest = NULL;
 ULONG_PTR nSize;

 if(UserStack->FixedStackLimit)
  pStackLowest = UserStack->FixedStackLimit;
 else if(UserStack->ExpandableStackBottom)
  pStackLowest = UserStack->ExpandableStackBottom;

 /* free the stack, if it was allocated */
 if(pStackLowest != NULL)
  return NtFreeVirtualMemory(ProcessHandle, &pStackLowest, &nSize, MEM_RELEASE);

 return STATUS_SUCCESS;
}

/* EOF */
