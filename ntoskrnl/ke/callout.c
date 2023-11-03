/*
* PROJECT:     ReactOS Kernel
* LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
* PURPOSE:     Kernel stack expansion functions
* COPYRIGHT:   Copyright 2023 Timo Kreuzer <timo.kreuzer@reactos.org>
*/

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include <minwin/ntosifs.h>
#define NDEBUG
#include <debug.h>

/* This function is implemented in asm */
VOID
NTAPI
KiSwitchStackAndCallout(
    _In_opt_ PVOID Parameter,
    _In_ PEXPAND_STACK_CALLOUT Callout,
    _In_ PVOID Stack);

ULONG KiNumberOfCalloutStacks = 0;

/* FUNCTIONS *****************************************************************/

/*!
 * @brief Allocate a callout stack
 * 
 * @param StackType - Type of the stack to allocate
 * @param RecursionDepth - Recursion depth
 * @param Reserved - Reserved, must be 0
 * @param StackContext - Pointer to a variable that receives the stack context
 * 
 * @return STATUS_SUCCESS on success, STATUS_INSUFFICIENT_RESOURCES if the stack could not be allocated
 */
_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NTAPI
KeAllocateCalloutStackEx(
    _In_ _Strict_type_match_ KSTACK_TYPE StackType,
    _In_ UCHAR RecursionDepth,
    _In_ _Reserved_ SIZE_T Reserved,
    _Outptr_ PVOID *StackContext)
{
    BOOLEAN LargeStack = (StackType == ReserveStackLarge);
    SIZE_T StackSize = LargeStack ? KERNEL_LARGE_STACK_SIZE : KERNEL_STACK_SIZE;
    PVOID StackBase;
    PKSTACK_CONTROL StackControl;
    UCHAR Node = KeGetCurrentThread()->Process->IdealNode;

    /* Validate the StackType */
    if (StackType > MaximumReserveStacks)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Allocate a stack with Mm */
    StackBase = MmCreateKernelStackEx(LargeStack, StackSize, Node);
    if (StackBase == NULL)
    {
        DPRINT1("Failed to allocate callout stack\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // FIXME: Only commit as much as needed, but this function doesn't have a size parameter
    // Use KiAllocateStackSegment

    /* Get the stack control structure and initialize it */
    StackControl = (PKSTACK_CONTROL)StackBase - 1;
    StackControl->StackBase = (ULONG_PTR)StackBase;
    StackControl->ActualLimit = (ULONG_PTR)StackBase - StackSize;
    StackControl->StackExpansion = 1;

    /* Return the stack control as context */
    *StackContext = StackControl;

    KiNumberOfCalloutStacks++;

    return STATUS_SUCCESS;
}

/*
 * @brief Allocate a callout stack
 * 
 * @param LargeStack - TRUE if a large stack should be allocated, FALSE for a normal stack
 * 
 * @return Pointer to the stack context on success, NULL if the stack could not be allocated
 */
_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
PVOID
NTAPI
KeAllocateCalloutStack(
    _In_ BOOLEAN LargeStack)
{
    KSTACK_TYPE StackType = LargeStack ? ReserveStackLarge : ReserveStackNormal;
    NTSTATUS Status;
    PVOID StackContext;

    /* Forward to KeAllocateCalloutStackEx */
    Status = KeAllocateCalloutStackEx(StackType, 0, 0, &StackContext);
    if (!NT_SUCCESS(Status))
    {
        return NULL;
    }

    return StackContext;
}

/*
 * @brief Free a callout stack
 * 
 * @param Context - Pointer to the stack context
 */
_IRQL_requires_max_(APC_LEVEL)
VOID
NTAPI
KeFreeCalloutStack(
    _In_ PVOID Context)
{
    PKSTACK_CONTROL StackControl = (PKSTACK_CONTROL)Context;
    SIZE_T StackSize;
    BOOLEAN IsLargeStack;

    /* Check if the stack is a large stack */
    StackSize = StackControl->StackBase - StackControl->ActualLimit & ~1;
    IsLargeStack = StackSize > KERNEL_STACK_SIZE;

    /* Free the stack */
    MmDeleteKernelStack((PVOID)StackControl->StackBase, IsLargeStack);
    KiNumberOfCalloutStacks--;
}

/*!
 * @brief Free a callout stack
 * 
 * @param StackContext - Pointer to the stack context
 */
VOID
NTAPI
KiRemoveThreadCalloutStack(
    _In_ PKTHREAD Thread)
{
    PKSTACK_CONTROL StackControl;

    ASSERT(Thread != KeGetCurrentThread());

    ASSERT(Thread->CalloutActive);
    Thread->CalloutActive--;

    /* Unlink the callout stack */
    StackControl = ((PKSTACK_CONTROL)Thread->StackBase) - 1;
    Thread->StackBase = (PVOID)StackControl->Previous.StackBase;
    Thread->StackLimit = StackControl->Previous.StackLimit;
    Thread->KernelStack = (PVOID)StackControl->Previous.KernelStack;
    Thread->InitialStack = (PVOID)StackControl->Previous.InitialStack;

    /* Delete the callout stack */
    KeFreeCalloutStack(StackControl);
}

/*!
 * @brief Expand the kernel stack and call a function
 *
 * @param Callout - Callout function to call
 * @param Parameter - Parameter to pass to the callout function
 * @param Size - Size of the stack to allocate
 * @param Wait - TRUE if the callout should be called with interrupts enabled, FALSE if interrupts should be disabled
 * @param Context - Reserved, must be NULL
 *
 * @return STATUS_SUCCESS on success,
 *     STATUS_INVALID_PARAMETER if the size is too large,
 *     STATUS_INSUFFICIENT_RESOURCES if the stack could not be allocated
 */
_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
static
NTSTATUS
NTAPI
KiAllocateStackSegmentAndCallout(
    _In_ PEXPAND_STACK_CALLOUT Callout,
    _In_opt_ PVOID Parameter,
    _In_ SIZE_T Size,
    _In_ BOOLEAN Wait,
    _In_opt_ PVOID Context)
{
    PKTHREAD CurrentThread = KeGetCurrentThread();
    PKIPCR Pcr = (PKIPCR)KeGetPcr();
    PKSTACK_CONTROL StackControl;
    NTSTATUS Status;
    KSTACK_TYPE StackType;
    PVOID StackContext;
    SIZE_T StackCommit;
    BOOLEAN PreviousCalloutActive;

    UNREFERENCED_PARAMETER(Wait);
    UNREFERENCED_PARAMETER(Context);

    ASSERT(__readeflags() & EFLAGS_INTERRUPT_MASK);

    /* Check if the size is too large */
    if (Size > MAXIMUM_EXPANSION_SIZE)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Allow a maximum of 5 stacks to be allocated before bailing out */
    if (CurrentThread->CalloutActive >= 5)
    {
        return STATUS_STACK_OVERFLOW;
    }

    /* Check if we need a large stack */
    if ((Size > KERNEL_STACK_SIZE) || CurrentThread->LargeStack)
    {
        StackType = ReserveStackLarge;
        StackCommit = KERNEL_LARGE_STACK_COMMIT;
    }
    else
    {
        StackType = ReserveStackNormal;
        StackCommit = KERNEL_STACK_SIZE;
    }

    /* Allocate the stack */
    Status = KeAllocateCalloutStackEx(StackType, 0, 0, &StackContext);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to allocate callout stack\n");
        return Status;
    }

    /* Save previous CalloutActive and set it to TRUE */
    PreviousCalloutActive = CurrentThread->CalloutActive;
    CurrentThread->CalloutActive = TRUE;

    /* Get the stack control from the context */
    StackControl = (PKSTACK_CONTROL)StackContext;

    /* Link the previous stack */
    StackControl->Previous.StackBase = (ULONG_PTR)CurrentThread->StackBase;
    StackControl->Previous.StackLimit = CurrentThread->StackLimit;
    StackControl->Previous.KernelStack = (ULONG_PTR)CurrentThread->KernelStack;
    StackControl->Previous.InitialStack = (ULONG_PTR)CurrentThread->InitialStack;

    /* Disable interrupts */
    _disable();

    /* Set up the new stack in the thread */
    CurrentThread->StackBase = (PVOID)StackControl->StackBase;
    CurrentThread->StackLimit = StackControl->StackBase - StackCommit;
    CurrentThread->InitialStack = StackControl;
    CurrentThread->KernelStack = (PVOID)((ULONG_PTR)CurrentThread->InitialStack - sizeof(KTRAP_FRAME));
    ASSERT(((ULONG_PTR)CurrentThread->KernelStack & 0xF) == 0);

#if defined(_M_AMD64)
    /* Set up the new stack in the PRCB and TSS */
    Pcr->Prcb.RspBase = (ULONG_PTR)CurrentThread->InitialStack;
    Pcr->TssBase->Rsp0 = (ULONG_PTR)CurrentThread->InitialStack;
#elif defined(_M_IX86)
    Pcr->TSS->Esp0 = (ULONG_PTR)CurrentThread->InitialStack - sizeof(FX_SAVE_AREA);
#else
    UNIMPLEMENTED_DBGBREAK();
#endif

    /* Switch to the new stack and invoke the callout function */
    KiSwitchStackAndCallout(Parameter, Callout, CurrentThread->KernelStack);
 
    /* Restore the old stack */
    CurrentThread->StackBase = (PVOID)StackControl->Previous.StackBase;
    CurrentThread->StackLimit = StackControl->Previous.StackLimit;
    CurrentThread->KernelStack = (PVOID)StackControl->Previous.KernelStack;
    CurrentThread->InitialStack = (PVOID)StackControl->Previous.InitialStack;

#if defined(_M_AMD64)
    /* Restore the old stack in the PCR and TSS */
    Pcr->Prcb.RspBase = (ULONG_PTR)CurrentThread->InitialStack;
    Pcr->TssBase->Rsp0 = (ULONG_PTR)CurrentThread->InitialStack;
#elif defined(_M_IX86)
    Pcr->TSS->Esp0 = (ULONG_PTR)CurrentThread->InitialStack - sizeof(FX_SAVE_AREA);
#else
    UNIMPLEMENTED_DBGBREAK();
#endif

    /* Enable interrupts */
    _enable();

    /* Restore CalloutActive */
    CurrentThread->CalloutActive = PreviousCalloutActive;

    /* Free the stack */
    KeFreeCalloutStack(StackContext);

    return Status;
}

/*!
 * @brief Expand the kernel stack to the requested size and call a function
 *
 * @param Callout - Callout function to call
 * @param Parameter - Parameter to pass to the callout function
 * @param Size - Size of the stack to allocate
 * @param Wait - TRUE if the callout should be called with interrupts enabled, FALSE if interrupts should be disabled
 * @param Context - Reserved, must be NULL
 *
 * @return STATUS_SUCCESS on success,
 *     STATUS_INVALID_PARAMETER if the size is too large,
 *     STATUS_INSUFFICIENT_RESOURCES if the stack could not be allocated
 */
ULONG g_Count;
_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NTAPI
KeExpandKernelStackAndCalloutEx(
    _In_ PEXPAND_STACK_CALLOUT Callout,
    _In_opt_ PVOID Parameter,
    _In_ SIZE_T Size,
    _In_ BOOLEAN Wait,
    _In_opt_ PVOID Context)
{
    ULONG_PTR CurrentStackPointer;
    SIZE_T RemainingStack;
    NTSTATUS Status;

    /* Check if we already have enough space on the stack */
    CurrentStackPointer = KiGetStackPointer();
    RemainingStack = CurrentStackPointer - (ULONG_PTR)KeGetCurrentThread()->StackLimit;
    if (RemainingStack >= Size)
    {
        /* We have enough space, just call the callout */
        Callout(Parameter);
        return STATUS_SUCCESS;
    }

    /* Check if we have a large stack */
    if (KeGetCurrentThread()->LargeStack)
    {
        /* Try to grow it */
        Status = MmGrowKernelStackEx((PVOID)CurrentStackPointer, Size);
        if (NT_SUCCESS(Status))
        {
            /* We have enough space now, call the callout */
            Callout(Parameter);
            return STATUS_SUCCESS;
        }
    }

    /* No luck, we need to allocate a new stack segment */
    return KiAllocateStackSegmentAndCallout(Callout, Parameter, Size, Wait, Context);
}

/*
 * @brief Expand the kernel stack and call a function
 * 
 * @param Callout - Callout function to call
 * @param Parameter - Parameter to pass to the callout function
 * @param Size - Size of the stack to allocate
 * 
 * @return STATUS_SUCCESS on success,
 *     STATUS_INVALID_PARAMETER if the size is too large,
 *     STATUS_INSUFFICIENT_RESOURCES if the stack could not be allocated
 */
_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NTAPI
KeExpandKernelStackAndCallout(
    _In_ PEXPAND_STACK_CALLOUT Callout,
    _In_opt_ PVOID Parameter,
    _In_ SIZE_T Size)
{
    return KeExpandKernelStackAndCalloutEx(Callout, Parameter, Size, FALSE, NULL);
}
