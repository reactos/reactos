/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS System Libraries
 * FILE:        dll/win32/kernel32/client/fiber.c
 * PURPOSE:     Fiber Implementation
 * PROGRAMMERS:
 *              Alex Ionescu (alex@relsoft.net)
 *              KJK::Hyperion <noog@libero.it>
 */
#include <k32.h>

#define NDEBUG
#include <debug.h>

#ifdef _M_IX86
C_ASSERT(FIELD_OFFSET(FIBER, ExceptionList) == 0x04);
C_ASSERT(FIELD_OFFSET(FIBER, StackBase) == 0x08);
C_ASSERT(FIELD_OFFSET(FIBER, StackLimit) == 0x0C);
C_ASSERT(FIELD_OFFSET(FIBER, DeallocationStack) == 0x10);
C_ASSERT(FIELD_OFFSET(FIBER, FiberContext) == 0x14);
C_ASSERT(FIELD_OFFSET(FIBER, GuaranteedStackBytes) == 0x2E0);
C_ASSERT(FIELD_OFFSET(FIBER, FlsData) == 0x2E4);
C_ASSERT(FIELD_OFFSET(FIBER, ActivationContextStackPointer) == 0x2E8);
#endif // _M_IX86

/* PRIVATE FUNCTIONS **********************************************************/

VOID
WINAPI
BaseRundownFls(_In_ PVOID FlsData)
{
    /* No FLS support yet */
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
ConvertFiberToThread(VOID)
{
    PTEB Teb;
    PFIBER FiberData;
    DPRINT1("Converting Fiber to Thread\n");

    /* Check if the thread is already not a fiber */
    Teb = NtCurrentTeb();
    if (!Teb->HasFiberData)
    {
        /* Fail */
        SetLastError(ERROR_ALREADY_THREAD);
        return FALSE;
    }

    /* This thread won't run a fiber anymore */
    Teb->HasFiberData = FALSE;
    FiberData = Teb->NtTib.FiberData;
    Teb->NtTib.FiberData = NULL;

    /* Free the fiber */
    ASSERT(FiberData != NULL);
    RtlFreeHeap(GetProcessHeap(),
                0,
                FiberData);

    /* Success */
    return TRUE;
}

/*
 * @implemented
 */
LPVOID
WINAPI
ConvertThreadToFiberEx(_In_opt_ LPVOID lpParameter,
                       _In_ DWORD dwFlags)
{
    PTEB Teb;
    PFIBER Fiber;
    DPRINT1("Converting Thread to Fiber\n");

    /* Check for invalid flags */
    if (dwFlags & ~FIBER_FLAG_FLOAT_SWITCH)
    {
        /* Fail */
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    /* Are we already a fiber? */
    Teb = NtCurrentTeb();
    if (Teb->HasFiberData)
    {
        /* Fail */
        SetLastError(ERROR_ALREADY_FIBER);
        return NULL;
    }

    /* Allocate the fiber */
    Fiber = RtlAllocateHeap(RtlGetProcessHeap(),
                            0,
                            sizeof(FIBER));
    if (!Fiber)
    {
        /* Fail */
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    /* Copy some contextual data from the thread to the fiber */
    Fiber->FiberData = lpParameter;
    Fiber->ExceptionList = Teb->NtTib.ExceptionList;
    Fiber->StackBase = Teb->NtTib.StackBase;
    Fiber->StackLimit = Teb->NtTib.StackLimit;
    Fiber->DeallocationStack = Teb->DeallocationStack;
    Fiber->FlsData = Teb->FlsData;
    Fiber->GuaranteedStackBytes = Teb->GuaranteedStackBytes;
    Fiber->ActivationContextStackPointer = Teb->ActivationContextStackPointer;

    /* Save FPU State if requested, otherwise just the basic registers */
    Fiber->FiberContext.ContextFlags = (dwFlags & FIBER_FLAG_FLOAT_SWITCH) ?
                                       (CONTEXT_FULL | CONTEXT_FLOATING_POINT) :
                                       CONTEXT_FULL;

    /* Associate the fiber to the current thread */
    Teb->NtTib.FiberData = Fiber;
    Teb->HasFiberData = TRUE;

    /* Return opaque fiber data */
    return (LPVOID)Fiber;
}

/*
 * @implemented
 */
LPVOID
WINAPI
ConvertThreadToFiber(_In_opt_ LPVOID lpParameter)
{
    /* Call the newer function */
    return ConvertThreadToFiberEx(lpParameter,
                                  0);
}

/*
 * @implemented
 */
LPVOID
WINAPI
CreateFiber(_In_ SIZE_T dwStackSize,
            _In_ LPFIBER_START_ROUTINE lpStartAddress,
            _In_opt_ LPVOID lpParameter)
{
    /* Call the Newer Function */
    return CreateFiberEx(dwStackSize,
                         0,
                         0,
                         lpStartAddress,
                         lpParameter);
}

/*
 * @implemented
 */
LPVOID
WINAPI
CreateFiberEx(_In_ SIZE_T dwStackCommitSize,
              _In_ SIZE_T dwStackReserveSize,
              _In_ DWORD dwFlags,
              _In_ LPFIBER_START_ROUTINE lpStartAddress,
              _In_opt_ LPVOID lpParameter)
{
    PFIBER Fiber;
    NTSTATUS Status;
    INITIAL_TEB InitialTeb;
    PACTIVATION_CONTEXT_STACK ActivationContextStackPointer;
    DPRINT("Creating Fiber\n");

    /* Check for invalid flags */
    if (dwFlags & ~FIBER_FLAG_FLOAT_SWITCH)
    {
        /* Fail */
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    /* Allocate the Activation Context Stack */
    ActivationContextStackPointer = NULL;
    Status = RtlAllocateActivationContextStack(&ActivationContextStackPointer);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        BaseSetLastNTError(Status);
        return NULL;
    }

    /* Allocate the fiber */
    Fiber = RtlAllocateHeap(RtlGetProcessHeap(),
                            0,
                            sizeof(FIBER));
    if (!Fiber)
    {
        /* Free the activation context stack */
        RtlFreeActivationContextStack(ActivationContextStackPointer);

        /* Fail */
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    /* Create the stack for the fiber */
    Status = BaseCreateStack(NtCurrentProcess(),
                              dwStackCommitSize,
                              dwStackReserveSize,
                              &InitialTeb);
    if (!NT_SUCCESS(Status))
    {
        /* Free the fiber */
        RtlFreeHeap(GetProcessHeap(),
                    0,
                    Fiber);

        /* Free the activation context stack */
        RtlFreeActivationContextStack(ActivationContextStackPointer);

        /* Failure */
        BaseSetLastNTError(Status);
        return NULL;
    }

    /* Clear the context */
    RtlZeroMemory(&Fiber->FiberContext,
                  sizeof(CONTEXT));

    /* Copy the data into the fiber */
    Fiber->StackBase = InitialTeb.StackBase;
    Fiber->StackLimit = InitialTeb.StackLimit;
    Fiber->DeallocationStack = InitialTeb.AllocatedStackBase;
    Fiber->FiberData = lpParameter;
    Fiber->ExceptionList = EXCEPTION_CHAIN_END;
    Fiber->GuaranteedStackBytes = 0;
    Fiber->FlsData = NULL;
    Fiber->ActivationContextStackPointer = ActivationContextStackPointer;

    /* Save FPU State if requested, otherwise just the basic registers */
    Fiber->FiberContext.ContextFlags = (dwFlags & FIBER_FLAG_FLOAT_SWITCH) ?
                                       (CONTEXT_FULL | CONTEXT_FLOATING_POINT) :
                                       CONTEXT_FULL;

    /* Initialize the context for the fiber */
    BaseInitializeContext(&Fiber->FiberContext,
                          lpParameter,
                          lpStartAddress,
                          InitialTeb.StackBase,
                          2);

    /* Return the Fiber */
    return Fiber;
}

/*
 * @implemented
 */
VOID
WINAPI
DeleteFiber(_In_ LPVOID lpFiber)
{
    SIZE_T Size;
    PFIBER Fiber;
    PTEB Teb;

    /* Are we deleting ourselves? */
    Teb = NtCurrentTeb();
    Fiber = (PFIBER)lpFiber;
    if ((Teb->HasFiberData) &&
        (Teb->NtTib.FiberData == Fiber))
    {
        /* Just exit */
        ExitThread(1);
    }

    /* Not ourselves, de-allocate the stack */
    Size = 0 ;
    NtFreeVirtualMemory(NtCurrentProcess(),
                        &Fiber->DeallocationStack,
                        &Size,
                        MEM_RELEASE);

    /* Get rid of FLS */
    if (Fiber->FlsData) BaseRundownFls(Fiber->FlsData);

    /* Get rid of the activation context stack */
    RtlFreeActivationContextStack(Fiber->ActivationContextStackPointer);

    /* Free the fiber data */
    RtlFreeHeap(GetProcessHeap(),
                0,
                lpFiber);
}

/*
 * @implemented
 */
BOOL
WINAPI
IsThreadAFiber(VOID)
{
    /* Return flag in the TEB */
    return NtCurrentTeb()->HasFiberData;
}

/*
 * @unimplemented
 */
DWORD
WINAPI
FlsAlloc(PFLS_CALLBACK_FUNCTION lpCallback)
{
   (void)lpCallback;

   UNIMPLEMENTED;
   SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
   return FLS_OUT_OF_INDEXES;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
FlsFree(DWORD dwFlsIndex)
{
    (void)dwFlsIndex;

    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @implemented
 */
PVOID
WINAPI
FlsGetValue(DWORD dwFlsIndex)
{
    PVOID *ppFlsSlots;
    PVOID pRetVal;

    if(dwFlsIndex >= 128) goto l_InvalidParam;

    ppFlsSlots = NtCurrentTeb()->FlsData;

    if(ppFlsSlots == NULL) goto l_InvalidParam;

    SetLastError(0);
    pRetVal = ppFlsSlots[dwFlsIndex + 2];

    return pRetVal;

l_InvalidParam:
    SetLastError(ERROR_INVALID_PARAMETER);
    return NULL;
}


/*
 * @implemented
 */
BOOL
WINAPI
FlsSetValue(DWORD dwFlsIndex,
            PVOID lpFlsData)
{
    PVOID *ppFlsSlots;
    TEB *pTeb = NtCurrentTeb();

    if(dwFlsIndex >= 128) goto l_InvalidParam;

    ppFlsSlots = pTeb->FlsData;

    if (ppFlsSlots == NULL)
    {
        PEB *pPeb = pTeb->ProcessEnvironmentBlock;

        ppFlsSlots = RtlAllocateHeap(pPeb->ProcessHeap,
                                     HEAP_ZERO_MEMORY,
                                     (128 + 2) * sizeof(PVOID));
        if(ppFlsSlots == NULL) goto l_OutOfMemory;

        pTeb->FlsData = ppFlsSlots;

        RtlAcquirePebLock();

        /* TODO: initialization */

        RtlReleasePebLock();
    }

    ppFlsSlots[dwFlsIndex + 2] = lpFlsData;

    return TRUE;

l_OutOfMemory:
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    goto l_Fail;

l_InvalidParam:
    SetLastError(ERROR_INVALID_PARAMETER);

l_Fail:
    return FALSE;
}

/* EOF */
