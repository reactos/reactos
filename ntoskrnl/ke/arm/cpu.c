/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
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
        DPRINT1("TLB Lockdown Failure (%p). Running on QEMU?\n", Virtual);
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
