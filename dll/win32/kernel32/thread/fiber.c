/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS System Libraries
 * FILE:        lib/kernel32/thread/fiber.c
 * PURPOSE:     Fiber Implementation
 * PROGRAMMERS: 
 *              Alex Ionescu (alex@relsoft.net)
 *              KJK::Hyperion <noog@libero.it>
 */
#include <k32.h>

#define NDEBUG
#include <debug.h>

typedef struct _FIBER                                      /* Field offsets:  */
{                                                          /* 32 bit   64 bit */
    /* this must be the first field */
    LPVOID Parameter;                                      /*   0x00     0x00 */
    struct _EXCEPTION_REGISTRATION_RECORD * ExceptionList; /*   0x04     0x08 */
    LPVOID StackBase;                                      /*   0x08     0x10 */
    LPVOID StackLimit;                                     /*   0x0C     0x18 */
    LPVOID DeallocationStack;                              /*   0x10     0x20 */
    CONTEXT Context;                                       /*   0x14     0x28 */
    ULONG GuaranteedStackBytes;                            /*   0x2E0         */
    PVOID FlsData;                                         /*   0x2E4         */
    PVOID ActivationContextStack;                          /*   0x2E8         */
} FIBER, *PFIBER;

/*
 * @implemented
 */
BOOL
WINAPI
ConvertFiberToThread(VOID)
{
    PTEB pTeb = NtCurrentTeb();
    DPRINT1("Converting Fiber to Thread\n");

    /* the current thread isn't running a fiber: failure */
    if(!pTeb->HasFiberData)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* this thread won't run a fiber anymore */
    pTeb->HasFiberData = FALSE;

    /* free the fiber */
    if(pTeb->Tib.FiberData != NULL)
    {
        RtlFreeHeap(GetProcessHeap(), 0, pTeb->Tib.FiberData);
    }

    /* success */
    return TRUE;
}

/*
 * @implemented
 */
LPVOID
WINAPI
ConvertThreadToFiberEx(LPVOID lpParameter, 
                       DWORD dwFlags)
{
    PTEB pTeb = NtCurrentTeb();
    PFIBER pfCurFiber;
    DPRINT1("Converting Thread to Fiber\n");

    /* the current thread is already a fiber */
    if(pTeb->HasFiberData && pTeb->Tib.FiberData) return pTeb->Tib.FiberData;

    /* allocate the fiber */
    pfCurFiber = (PFIBER)RtlAllocateHeap(GetProcessHeap(), 
                                         0,
                                         sizeof(FIBER));

    /* failure */
    if(pfCurFiber == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    /* copy some contextual data from the thread to the fiber */
    pfCurFiber->ExceptionList = pTeb->Tib.ExceptionList;
    pfCurFiber->StackBase = pTeb->Tib.StackBase;
    pfCurFiber->StackLimit = pTeb->Tib.StackLimit;
    pfCurFiber->DeallocationStack = pTeb->DeallocationStack;
    pfCurFiber->FlsData = pTeb->FlsData;
    pfCurFiber->GuaranteedStackBytes = pTeb->GuaranteedStackBytes;
    pfCurFiber->ActivationContextStack = pTeb->ActivationContextStackPointer;
    pfCurFiber->Context.ContextFlags = CONTEXT_FULL;
    
    /* Save FPU State if requsted */
    if (dwFlags & FIBER_FLAG_FLOAT_SWITCH)
    {
        pfCurFiber->Context.ContextFlags |= CONTEXT_FLOATING_POINT;
    }

    /* associate the fiber to the current thread */
    pTeb->Tib.FiberData = pfCurFiber;
    pTeb->HasFiberData = TRUE;

    /* success */
    return (LPVOID)pfCurFiber;
}

/*
 * @implemented
 */
LPVOID
WINAPI
ConvertThreadToFiber(LPVOID lpParameter)
{
    /* Call the newer function */
    return ConvertThreadToFiberEx(lpParameter, 0);
}

/*
 * @implemented
 */
LPVOID
WINAPI
CreateFiber(SIZE_T dwStackSize,
            LPFIBER_START_ROUTINE lpStartAddress,
            LPVOID lpParameter)
{
    /* Call the Newer Function */
    return CreateFiberEx(dwStackSize, 0, 0, lpStartAddress, lpParameter);
}

/*
 * @implemented
 */
LPVOID
WINAPI
CreateFiberEx(SIZE_T dwStackCommitSize,
              SIZE_T dwStackReserveSize,
              DWORD dwFlags,
              LPFIBER_START_ROUTINE lpStartAddress,
              LPVOID lpParameter)
{
    PFIBER pfCurFiber;
    NTSTATUS nErrCode;
    INITIAL_TEB usFiberInitialTeb;
    CONTEXT ctxFiberContext;
    PVOID ActivationContextStack = NULL;
    DPRINT1("Creating Fiber\n");

    #ifdef SXS_SUPPORT_ENABLED
    /* Allocate the Activation Context Stack */
    nErrCode = RtlAllocateActivationContextStack(&ActivationContextStack);
    #endif
    
    /* Allocate the fiber */
    pfCurFiber = (PFIBER)RtlAllocateHeap(GetProcessHeap(), 
                                         0,
                                         sizeof(FIBER));
    /* Failure */
    if(pfCurFiber == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    /* Create the stack for the fiber */
    nErrCode = BasepCreateStack(NtCurrentProcess(),
                                dwStackCommitSize,
                                dwStackReserveSize,
                                &usFiberInitialTeb);
    /* Failure */
    if(!NT_SUCCESS(nErrCode)) 
    {
        /* Free the fiber */
        RtlFreeHeap(GetProcessHeap(), 0, pfCurFiber);

        /* Failure */
        SetLastErrorByStatus(nErrCode);
        return NULL;
    }
    
    /* Clear the context */
    RtlZeroMemory(&pfCurFiber->Context, sizeof(CONTEXT));
    
    /* copy the data into the fiber */
    pfCurFiber->StackBase = usFiberInitialTeb.StackBase;
    pfCurFiber->StackLimit = usFiberInitialTeb.StackLimit;
    pfCurFiber->DeallocationStack = usFiberInitialTeb.AllocatedStackBase;
    pfCurFiber->Parameter = lpParameter;
    pfCurFiber->ExceptionList = (struct _EXCEPTION_REGISTRATION_RECORD *)-1;
    pfCurFiber->GuaranteedStackBytes = 0;
    pfCurFiber->FlsData = NULL;
    pfCurFiber->ActivationContextStack = ActivationContextStack;
    pfCurFiber->Context.ContextFlags = CONTEXT_FULL;
    
    /* Save FPU State if requsted */
    if (dwFlags & FIBER_FLAG_FLOAT_SWITCH)
    {
        pfCurFiber->Context.ContextFlags |= CONTEXT_FLOATING_POINT;
    }
    
    /* initialize the context for the fiber */
    BasepInitializeContext(&ctxFiberContext,
                           lpParameter,
                           lpStartAddress,
                           usFiberInitialTeb.StackBase,
                           2);
 
    /* Return the Fiber */ 
    return pfCurFiber;
}

/*
 * @implemented
 */
VOID
WINAPI
DeleteFiber(LPVOID lpFiber)
{
    SIZE_T nSize = 0;
    PVOID pStackAllocBase = ((PFIBER)lpFiber)->DeallocationStack;

    /* free the fiber */
    RtlFreeHeap(GetProcessHeap(), 0, lpFiber);

    /* the fiber is deleting itself: let the system deallocate the stack */
    if(NtCurrentTeb()->Tib.FiberData == lpFiber) ExitThread(1);

    /* deallocate the stack */
    NtFreeVirtualMemory(NtCurrentProcess(),
                        &pStackAllocBase,
                        &nSize,
                        MEM_RELEASE);
}

/*
 * @implemented
 */
BOOL
WINAPI
IsThreadAFiber(VOID)
{
    return NtCurrentTeb()->HasFiberData;
}


__declspec(noreturn)
VOID
WINAPI
BaseFiberStartup(VOID)
{
#ifdef _M_IX86
    PFIBER Fiber = GetFiberData();

    /* Call the Thread Startup Routine */
    DPRINT1("Starting Fiber\n");
    BaseThreadStartup((LPTHREAD_START_ROUTINE)Fiber->Context.Eax,
                      (LPVOID)Fiber->Context.Ebx);
#else
#warning Unknown architecture
    UNIMPLEMENTED;
    DbgBreakPoint();
#endif
}

/* EOF */
