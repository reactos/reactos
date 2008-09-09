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
CCHAR KeNumberProcessors;
ULONG KeDcacheFlushCount;
ULONG KeActiveProcessors;
ULONG KeProcessorArchitecture;
ULONG KeProcessorLevel;
ULONG KeProcessorRevision;
ULONG KeFeatureBits;
ULONG KeLargestCacheLine = 32; // FIXME: It depends

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
KeFillFixedEntryTb(IN ARM_PTE Pte,
                   IN PVOID Virtual,
                   IN ULONG Index)
{
    ARM_LOCKDOWN_REGISTER LockdownRegister;
    ULONG OldVictimCount;
    volatile unsigned long Temp;
    PARM_TRANSLATION_TABLE TranslationTable;
    
    //
    // Fixed TB entries must be section entries
    //
    Virtual = (PVOID)((ULONG)Virtual & 0xFFF00000);
    
    //
    // On ARM, we can't set the index ourselves, so make sure that we are not
    // locking down more than 8 entries.
    //
    UNREFERENCED_PARAMETER(Index);
    KeFixedTbEntries++;
    ASSERT(KeFixedTbEntries <= 8);
    
    //
    // Flush the address
    //
    KiFlushSingleTb(TRUE, Virtual);
    
    //
    // Read lockdown register and set the preserve bit
    //
    LockdownRegister = KeArmLockdownRegisterGet();
    LockdownRegister.Preserve = TRUE;
    OldVictimCount = LockdownRegister.Victim;
    KeArmLockdownRegisterSet(LockdownRegister);
    
    //
    // Map the PTE for this virtual address
    //
    TranslationTable = (PVOID)KeArmTranslationTableRegisterGet().AsUlong;
    TranslationTable->Pte[(ULONG)Virtual >> PDE_SHIFT] = Pte;
    
    //
    // Now force a miss
    //
    Temp = *(PULONG)Virtual;
    
    //
    // Read lockdown register 
    //
    LockdownRegister = KeArmLockdownRegisterGet();
    if (LockdownRegister.Victim == 0)
    {
        //
        // This can only happen on QEMU or broken CPUs since there *has*
        // to have been at least a miss since the system started. For example,
        // QEMU doesn't support TLB lockdown.
        //
        // On these systems, we'll just keep the PTE mapped
        //
    }
    else
    {
        //
        // Clear the preserve bits
        //
        LockdownRegister.Preserve = FALSE;
        ASSERT(LockdownRegister.Victim == OldVictimCount + 1);
        KeArmLockdownRegisterSet(LockdownRegister);
        
        //
        // Clear the PTE
        //
        TranslationTable->Pte[(ULONG)Virtual >> PDE_SHIFT].AsUlong = 0;
    }
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
NTAPI
KiSaveProcessorControlState(OUT PKPROCESSOR_STATE ProcessorState)
{
    //
    // Save some critical stuff we use
    //
    ProcessorState->SpecialRegisters.ControlRegister = KeArmControlRegisterGet();
    ProcessorState->SpecialRegisters.LockdownRegister = KeArmLockdownRegisterGet();
    ProcessorState->SpecialRegisters.CacheRegister = KeArmCacheRegisterGet();
    ProcessorState->SpecialRegisters.StatusRegister = KeArmStatusRegisterGet();
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

BOOLEAN
NTAPI
KeDisableInterrupts(VOID)
{
    ARM_STATUS_REGISTER Flags;
    
    //
    // Get current interrupt state and disable interrupts
    //
    Flags = KeArmStatusRegisterGet();
    _disable();
    
    //
    // Return previous interrupt state
    //
    return Flags.IrqDisable;
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
KAFFINITY
NTAPI
KeQueryActiveProcessors(VOID)
{
    PAGED_CODE();
    
    //
    // Simply return the number of active processors
    //
    return KeActiveProcessors;
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

/*
 * @implemented
 */
NTSTATUS
NTAPI
KeSaveFloatingPointState(OUT PKFLOATING_SAVE Save)
{
    //
    // Nothing to do on ARM
    //
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
KeRestoreFloatingPointState(IN PKFLOATING_SAVE Save)
{
    //
    // Nothing to do on ARM
    //
    return STATUS_SUCCESS;
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
