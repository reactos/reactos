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
 * @implemented
 */
DWORD
WINAPI
FlsAlloc(PFLS_CALLBACK_FUNCTION lpCallback)
{
    DWORD dwFlsIndex;
    PPEB Peb = NtCurrentPeb();
    PVOID *ppFlsSlots;

    RtlAcquirePebLock();

    ppFlsSlots = NtCurrentTeb()->FlsData;

    if (!Peb->FlsCallback &&
        !(Peb->FlsCallback = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY,
                                             FLS_MAXIMUM_AVAILABLE * sizeof(PVOID))))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        dwFlsIndex = FLS_OUT_OF_INDEXES;
    }
    else
    {
        dwFlsIndex = RtlFindClearBitsAndSet(Peb->FlsBitmap, 1, 1);
        if (dwFlsIndex != FLS_OUT_OF_INDEXES)
        {
            if (!ppFlsSlots &&
                !(ppFlsSlots = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY,
                                               (FLS_MAXIMUM_AVAILABLE + 2) * sizeof(PVOID))))
            {
                RtlClearBits(Peb->FlsBitmap, dwFlsIndex, 1);
                dwFlsIndex = FLS_OUT_OF_INDEXES;
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            }
            else
            {
                if (!NtCurrentTeb()->FlsData)
                    NtCurrentTeb()->FlsData = ppFlsSlots;

                if (lpCallback)
                    DPRINT1("FlsAlloc: Got lpCallback 0x%p, UNIMPLEMENTED!", lpCallback);

                ppFlsSlots[dwFlsIndex + 2] = NULL; /* clear the value */
                Peb->FlsCallback[dwFlsIndex] = lpCallback;
            }
        }
        else
        {
            SetLastError(ERROR_NO_MORE_ITEMS);
        }
    }
    RtlReleasePebLock();
    return dwFlsIndex;
}


/*
 * @implemented
 */
BOOL
WINAPI
FlsFree(DWORD dwFlsIndex)
{
    BOOL ret;
    PPEB Peb = NtCurrentPeb();
    PVOID *ppFlsSlots;

    if (dwFlsIndex >= FLS_MAXIMUM_AVAILABLE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    RtlAcquirePebLock();

    ppFlsSlots = NtCurrentTeb()->FlsData;
    ret = RtlAreBitsSet(Peb->FlsBitmap, dwFlsIndex, 1);
    if (ret)
    {
        RtlClearBits(Peb->FlsBitmap, dwFlsIndex, 1);
        /* FIXME: call Fls callback */
        /* FIXME: add equivalent of ThreadZeroTlsCell here */
        if (ppFlsSlots)
            ppFlsSlots[dwFlsIndex + 2] = NULL;
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }
    RtlReleasePebLock();
    return ret;
}


/*
 * @implemented
 */
PVOID
WINAPI
FlsGetValue(DWORD dwFlsIndex)
{
    PVOID *ppFlsSlots;

    ppFlsSlots = NtCurrentTeb()->FlsData;
    if (!dwFlsIndex || dwFlsIndex >= FLS_MAXIMUM_AVAILABLE || !ppFlsSlots)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    SetLastError(ERROR_SUCCESS);
    return ppFlsSlots[dwFlsIndex + 2];
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

    if (!dwFlsIndex || dwFlsIndex >= FLS_MAXIMUM_AVAILABLE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (!NtCurrentTeb()->FlsData &&
        !(NtCurrentTeb()->FlsData = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY,
                                                    (FLS_MAXIMUM_AVAILABLE + 2) * sizeof(PVOID))))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    ppFlsSlots = NtCurrentTeb()->FlsData;
    ppFlsSlots[dwFlsIndex + 2] = lpFlsData;
    return TRUE;
}

/* EOF */
