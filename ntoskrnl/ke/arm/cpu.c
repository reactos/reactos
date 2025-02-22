/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/ke/arm/cpu.c
 * PURPOSE:         Implements routines for ARM CPU support
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

ULONG KeFixedTbEntries;
ULONG KiDmaIoCoherency;
ULONG KeIcacheFlushCount = 0;
ULONG KeDcacheFlushCount;
ULONG KeLargestCacheLine = 64; // FIXME: It depends

/* FUNCTIONS ******************************************************************/

VOID
KiFlushSingleTb(IN BOOLEAN Invalid,
                IN PVOID Virtual)
{
    //
    // Just invalidate it
    //
    KeArmInvalidateTlbEntry(Virtual);
}

VOID
KeFlushTb(VOID)
{
    //
    // Flush the entire TLB
    //
    KeArmFlushTlb();
}

VOID
NTAPI
KeFlushCurrentTb(VOID)
{
    //
    // Rename?
    //
    KeFlushTb();
}

VOID
FASTCALL
KeZeroPages(IN PVOID Address,
            IN ULONG Size)
{
    /* Not using XMMI in this routine */
    RtlZeroMemory(Address, Size);
}

VOID
NTAPI
KiSaveProcessorControlState(OUT PKPROCESSOR_STATE ProcessorState)
{
    //
    // Save some critical stuff we use
    //
    __debugbreak();
#if 0
    ProcessorState->SpecialRegisters.ControlRegister = KeArmControlRegisterGet();
    ProcessorState->SpecialRegisters.LockdownRegister = KeArmLockdownRegisterGet();
    ProcessorState->SpecialRegisters.CacheRegister = KeArmCacheRegisterGet();
    ProcessorState->SpecialRegisters.StatusRegister = KeArmStatusRegisterGet();
#endif
}

VOID
NTAPI
KiRestoreProcessorControlState(PKPROCESSOR_STATE ProcessorState)
{
    __debugbreak();
#if 0
    KeArmControlRegisterSet(ProcessorState->SpecialRegisters.ControlRegister);
    KeArmLockdownRegisterSet(ProcessorState->SpecialRegisters.LockdownRegister);
    KeArmCacheRegisterSet(ProcessorState->SpecialRegisters.CacheRegister);
    KeArmStatusRegisterSet(ProcessorState->SpecialRegisters.StatusRegister);
#endif
}

BOOLEAN
NTAPI
KeInvalidateAllCaches(VOID)
{
    //
    // Invalidate D cache and I cache
    //
    KeArmInvalidateAllCaches();
    return TRUE;
}


/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
ULONG
NTAPI
KeGetRecommendedSharedDataAlignment(VOID)
{
    /* Return the global variable */
    return KeLargestCacheLine;
}

/*
 * @implemented
 */
VOID
NTAPI
KeFlushEntireTb(IN BOOLEAN Invalid,
                IN BOOLEAN AllProcessors)
{
    KIRQL OldIrql;

    //
    // Raise the IRQL for the TB Flush
    //
    OldIrql = KeRaiseIrqlToSynchLevel();

    //
    // Flush the TB for the Current CPU
    //
    KeFlushCurrentTb();

    //
    // Return to Original IRQL
    //
    KeLowerIrql(OldIrql);
}

/*
 * @implemented
 */
VOID
NTAPI
KeSetDmaIoCoherency(IN ULONG Coherency)
{
    //
    // Save the coherency globally
    //
    KiDmaIoCoherency = Coherency;
}

/*
 * @implemented
 */
VOID
__cdecl
KeSaveStateForHibernate(IN PKPROCESSOR_STATE State)
{
    //
    // Capture the context
    //
    RtlCaptureContext(&State->ContextFrame);

    //
    // Capture the control state
    //
    KiSaveProcessorControlState(State);
}

VOID
NTAPI
KeFlushIoBuffers(
    _In_ PMDL Mdl,
    _In_ BOOLEAN ReadOperation,
    _In_ BOOLEAN DmaOperation)
{
    DbgBreakPoint();
}

/* SYSTEM CALLS NOT VALID ON THIS CPU *****************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtVdmControl(IN ULONG ControlCode,
             IN PVOID ControlData)
{
    //
    // Does not exist on ARM
    //
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtSetLdtEntries(IN ULONG Selector1,
                IN LDT_ENTRY LdtEntry1,
                IN ULONG Selector2,
                IN LDT_ENTRY LdtEntry2)
{
    //
    // Does not exist on ARM
    //
    return STATUS_NOT_IMPLEMENTED;
}
