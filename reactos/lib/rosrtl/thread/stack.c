/* $Id: stack.c,v 1.6 2004/03/02 17:16:04 navaraf Exp $
*/
/*
*/

#define NTOS_MODE_USER
#include <ntos.h>

#define NDEBUG
#include <ntdll/ntdll.h>

#include <rosrtl/thread.h>

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

  ASSERT((*StackReserve - *StackCommit) >= PAGE_SIZE);
  ASSERT((*StackReserve - *StackCommit) % PAGE_SIZE == 0);

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
 ASSERT(!NT_SUCCESS(nErrCode));
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
 IN PUSER_STACK UserStack,
 OUT PVOID * StackBase,
 OUT PVOID * StackLimit
)
{
 /* fixed-size stack */
 if(UserStack->FixedStackBase && UserStack->FixedStackLimit)
 {
  *StackBase = UserStack->FixedStackBase;
  *StackLimit = UserStack->FixedStackLimit;
 }
 /* expandable stack */
 else if(UserStack->ExpandableStackBase && UserStack->ExpandableStackLimit)
 {
  *StackBase = UserStack->ExpandableStackBase;
  *StackLimit = UserStack->ExpandableStackLimit;
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
