/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/arm/kiinit.c
 * PURPOSE:         Implements the kernel entry point for ARM machines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

void arm_kprintf(const char *fmt, ...);

/* GLOBALS ********************************************************************/

BOOLEAN KeIsArmV6;
ULONG KeFixedTbEntries;
ULONG KeNumberProcessIds;
ULONG KeNumberTbEntries;

#define __ARMV6__ KeIsArmV6

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
    ULONG Temp;
    UNREFERENCED_PARAMETER(Pte);
    UNREFERENCED_PARAMETER(Index);
    
    //
    // On ARM, we can't set the index ourselves, so make sure that we are not
    // locking down more than 8 entries.
    //
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
    // Now force a miss
    //
    Temp = *(PULONG)Virtual;
    
    //
    // Read lockdown register and clear the preserve bit
    //
    LockdownRegister = KeArmLockdownRegisterGet();
    LockdownRegister.Preserve = FALSE;
    ASSERT(LockdownRegister.Victim == OldVictimCount + 1);
    KeArmLockdownRegisterSet(LockdownRegister);
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
KiInitializeSystem(IN ULONG Magic,
                   IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
	arm_kprintf("%s:%i\n", __func__, __LINE__);
    //
    // Detect ARM version (Architecture 6 is the ARMv5TE-J, go figure!)
    //
    KeIsArmV6 = KeArmIdCodeRegisterGet().Architecture == 7;
    
    //
    // Set the number of TLB entries and ASIDs
    //
    KeNumberTbEntries = 64;
    if (__ARMV6__)
    {
        //
        // 256 ASIDs on v6/v7
        //
        KeNumberProcessIds = 256;
    }
    else
    {
        //
        // The TLB is VIVT on v4/v5
        //
        KeNumberProcessIds = 0;
    }
    
    //
    // Flush the TLB
    //
    KeFlushTb();
    
    //
    //
    //

    while (TRUE);
}
