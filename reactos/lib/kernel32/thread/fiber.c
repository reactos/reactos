/* $Id: fiber.c,v 1.6 2003/07/10 18:50:51 chorns Exp $
 *
 * FILE: lib/kernel32/thread/fiber.c
 *
 * ReactOS Kernel32.dll
 *
 */
#include <k32.h>

#define NDEBUG
#include <kernel32/kernel32.h>

struct _FIBER                                           /* Field offsets:  */
{                                                       /* 32 bit   64 bit */
 /* this must be the first field */
 LPVOID Parameter;                                      /*   0x00     0x00 */

 struct _EXCEPTION_REGISTRATION_RECORD * ExceptionList; /*   0x04     0x08 */
 LPVOID StackBase;                                      /*   0x08     0x10 */
 LPVOID StackLimit;                                     /*   0x0C     0x18 */
 LPVOID DeallocationStack;                              /*   0x10     0x20 */
 ULONG_PTR Flags;                                       /*   0x14     0x28 */
#if defined(_M_IX86)
 /* control flow registers */
 DWORD Eip;                                             /*   0x18     ---- */
 DWORD Esp;                                             /*   0x1C     ---- */
 DWORD Ebp;                                             /*   0x20     ---- */

 /* general-purpose registers that must be preserved across calls */
 DWORD Ebx;                                             /*   0x24     ---- */
 DWORD Esi;                                             /*   0x28     ---- */
 DWORD Edi;                                             /*   0x2C     ---- */

 /* floating point save area (optional) */
 FLOATING_SAVE_AREA FloatSave;                          /*   0x30     ---- */
#else
#error Unspecified or unsupported architecture.
#endif
};

typedef struct _FIBER FIBER, * PFIBER;

__declspec(noreturn) void WINAPI FiberStartup(PVOID lpStartAddress);

/*
 * @implemented
 */
BOOL WINAPI ConvertFiberToThread(void)
{
 PTEB pTeb = NtCurrentTeb();

 /* the current thread isn't running a fiber: failure */
 if(!pTeb->IsFiber)
 {
  SetLastError(ERROR_INVALID_PARAMETER);
  return FALSE;
 }

 /* this thread won't run a fiber anymore */
 pTeb->IsFiber = FALSE;

 /* free the fiber */
 if(pTeb->Tib.Fib.FiberData != NULL)
  RtlFreeHeap(pTeb->Peb->ProcessHeap, 0, pTeb->Tib.Fib.FiberData);

 /* success */
}


/*
 * @implemented
 */
LPVOID WINAPI ConvertThreadToFiber(LPVOID lpParameter)
{
 return ConvertThreadToFiberEx(lpParameter, 0);
}


/*
 * @implemented
 */
LPVOID WINAPI ConvertThreadToFiberEx(LPVOID lpParameter, DWORD dwFlags)
{
 PTEB pTeb = NtCurrentTeb();
 PFIBER pfCurFiber;

 /* the current thread is already a fiber */
 if(pTeb->IsFiber && pTeb->Tib.Fib.FiberData) return pTeb->Tib.Fib.FiberData;

 /* allocate the fiber */
 pfCurFiber = (PFIBER)RtlAllocateHeap(pTeb->Peb->ProcessHeap, 0, sizeof(FIBER));

 /* failure */
 if(pfCurFiber == NULL)
 {
  SetLastError(ERROR_NOT_ENOUGH_MEMORY);
  return NULL;
 }

 pfCurFiber->Parameter = lpParameter;
 pfCurFiber->Flags = dwFlags;

 /* copy some contextual data from the thread to the fiber */
 pfCurFiber->ExceptionList = pTeb->Tib.ExceptionList;
 pfCurFiber->StackBase = pTeb->Tib.StackBase;
 pfCurFiber->StackLimit = pTeb->Tib.StackLimit;
 pfCurFiber->DeallocationStack = pTeb->DeallocationStack;

 /* associate the fiber to the current thread */
 pTeb->Tib.Fib.FiberData = pfCurFiber;
 pTeb->IsFiber = TRUE;

 /* success */
 return (LPVOID)pfCurFiber;
}


/*
 * @implemented
 */
LPVOID WINAPI CreateFiber
(
 SIZE_T dwStackSize,
 LPFIBER_START_ROUTINE lpStartAddress,
 LPVOID lpParameter
)
{
 return CreateFiberEx(dwStackSize, 0, 0, lpStartAddress, lpParameter);
}


/*
 * @implemented
 */
LPVOID WINAPI CreateFiberEx
(
 SIZE_T dwStackCommitSize,
 SIZE_T dwStackReserveSize,
 DWORD dwFlags,
 LPFIBER_START_ROUTINE lpStartAddress,
 LPVOID lpParameter
)
{
 PFIBER pfCurFiber;
 NTSTATUS nErrCode;
 PSIZE_T pnStackReserve = NULL;
 PSIZE_T pnStackCommit = NULL;
 USER_STACK usFiberStack;
 CONTEXT ctxFiberContext;
 PCHAR pStackBase;
 PCHAR pStackLimit;
 PTEB pTeb = NtCurrentTeb();

 /* allocate the fiber */
 pfCurFiber = (PFIBER)RtlAllocateHeap(pTeb->Peb->ProcessHeap, 0, sizeof(FIBER));

 /* failure */
 if(pfCurFiber == NULL)
 {
  SetLastError(ERROR_NOT_ENOUGH_MEMORY);
  return NULL;
 }

 /* if the stack reserve or commit size weren't specified, use defaults */
 if(dwStackReserveSize > 0) pnStackReserve = &dwStackReserveSize;
 if(dwStackCommitSize > 0) pnStackCommit = &dwStackCommitSize;

 /* create the stack for the fiber */
 nErrCode = RtlRosCreateStack
 (
  NtCurrentProcess(),
  &usFiberStack,
  0,
  pnStackReserve,
  pnStackCommit
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode)) goto l_CleanupFiber;

 /* initialize the context for the fiber */
 nErrCode = RtlRosInitializeContextEx
 (
  NtCurrentProcess(),
  &ctxFiberContext,
  FiberStartup,
  &usFiberStack,
  1,
  (ULONG_PTR *)&lpStartAddress
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode)) goto l_CleanupStack;

 /* copy the data into the fiber */

 /* fixed-size stack */
 if(usFiberStack.FixedStackBase && usFiberStack.FixedStackLimit)
 {
  pfCurFiber->StackBase = usFiberStack.FixedStackBase;
  pfCurFiber->StackLimit = usFiberStack.FixedStackLimit;
  pfCurFiber->DeallocationStack = usFiberStack.FixedStackLimit;
 }
 /* expandable stack */
 else if
 (
  usFiberStack.ExpandableStackBase &&
  usFiberStack.ExpandableStackLimit &&
  usFiberStack.ExpandableStackBottom
 )
 {
  pfCurFiber->StackBase = usFiberStack.ExpandableStackBase;
  pfCurFiber->StackLimit = usFiberStack.ExpandableStackLimit;
  pfCurFiber->DeallocationStack = usFiberStack.ExpandableStackBottom;
 }
 /* bad initial stack */
 else goto l_CleanupStack;

 pfCurFiber->Parameter = lpParameter;
 pfCurFiber->Flags = dwFlags;
 pfCurFiber->ExceptionList = (struct _EXCEPTION_REGISTRATION_RECORD *)-1;

#if defined(_M_IX86)

 pfCurFiber->Eip = ctxFiberContext.Eip;
 pfCurFiber->Esp = ctxFiberContext.Esp;
 pfCurFiber->Ebp = ctxFiberContext.Ebp;
 pfCurFiber->Ebx = ctxFiberContext.Ebx;
 pfCurFiber->Esi = ctxFiberContext.Esi;
 pfCurFiber->Edi = ctxFiberContext.Edi;

 if(dwFlags & FIBER_FLAG_FLOAT_SWITCH)
  pfCurFiber->FloatSave = ctxFiberContext.FloatSave;
 
#else
#error Unspecified or unsupported architecture.
#endif
 
 return pfCurFiber;

l_CleanupStack:
 /* free the stack */
 RtlRosDeleteStack(NtCurrentProcess(), &usFiberStack);

l_CleanupFiber:
 /* free the fiber */
 RtlFreeHeap(pTeb->Peb->ProcessHeap, 0, pfCurFiber);

 /* failure */
 assert(!NT_SUCCESS(nErrCode));
 SetLastErrorByStatus(nErrCode);
 return NULL;
}


/*
 * @implemented
 */
void WINAPI DeleteFiber(LPVOID lpFiber)
{
 SIZE_T nSize = 0;
 PVOID pStackAllocBase = ((PFIBER)lpFiber)->DeallocationStack;
 PTEB pTeb = NtCurrentTeb();

 /* free the fiber */
 RtlFreeHeap(pTeb->Peb->ProcessHeap, 0, lpFiber);

 /* the fiber is deleting itself: let the system deallocate the stack */
 if(pTeb->Tib.Fib.FiberData == lpFiber) ExitThread(1);

 /* deallocate the stack */
 NtFreeVirtualMemory
 (
  NtCurrentProcess(),
  &pStackAllocBase,
  &nSize,
  MEM_RELEASE
 );
}


__declspec(noreturn) extern void WINAPI ThreadStartup
(
 LPTHREAD_START_ROUTINE lpStartAddress,
 LPVOID lpParameter
);


__declspec(noreturn) void WINAPI FiberStartup(PVOID lpStartAddress)
{
 /* FIXME? this should be pretty accurate */
 ThreadStartup(lpStartAddress, NtCurrentTeb()->Tib.Fib.FiberData);
}

/* EOF */
