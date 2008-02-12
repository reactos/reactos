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

/* GLOBALS ********************************************************************/

BOOLEAN KeIsArmV6;
ULONG KeFixedTbEntries;
ULONG KeNumberProcessIds;
ULONG KeNumberTbEntries;

VOID HalSweepDcache(VOID);
VOID HalSweepIcache(VOID);

#define __ARMV6__ KeIsArmV6

//
// METAFIXME: We need to stop using 1MB Section Entry TTEs!
//

/* FUNCTIONS ******************************************************************/

VOID
DebugService(IN ULONG ServiceType,
             IN PCHAR Buffer,
             IN ULONG Length,
             IN ULONG Component,
             IN ULONG Level)
{
    //
    // ARM Bring-up Hack
    //
    void arm_kprintf(const char *fmt, ...);
    arm_kprintf("%s", Buffer);
}

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
    // Hack for 1MB Section Entries
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
    TranslationTable->Pte[(ULONG)Virtual >> TTB_SHIFT] = Pte;
    
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
        TranslationTable->Pte[(ULONG)Virtual >> TTB_SHIFT].AsUlong = 0;
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
KiInitializeSystem(IN ULONG Magic,
                   IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    ARM_PTE Pte;
    PKPCR Pcr;
    DPRINT1("-----------------------------------------------------\n");
    DPRINT1("ReactOS-ARM "KERNEL_VERSION_STR" (Build "KERNEL_VERSION_BUILD_STR")\n");
    DPRINT1("Command Line: %s\n", LoaderBlock->LoadOptions);
    DPRINT1("ARC Paths: %s %s %s %s\n", LoaderBlock->ArcBootDeviceName,
            LoaderBlock->NtHalPathName,
            LoaderBlock->ArcHalDeviceName,
            LoaderBlock->NtBootPathName);

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
    // Build the KIPCR pte
    //
    Pte.L1.Section.Type = SectionPte;
    Pte.L1.Section.Buffered = FALSE;
    Pte.L1.Section.Cached = FALSE;
    Pte.L1.Section.Reserved = 1; // ARM926EJ-S manual recommends setting to 1
    Pte.L1.Section.Domain = Domain0;
    Pte.L1.Section.Access = SupervisorAccess;
    Pte.L1.Section.BaseAddress = LoaderBlock->u.Arm.PcrPage;
    Pte.L1.Section.Ignored = Pte.L1.Section.Ignored1 = 0;
    
    //
    // Map it into kernel address space by locking it into the TLB
    //
    KeFillFixedEntryTb(Pte, (PVOID)KIPCR, PCR_ENTRY);

    //
    // Now map the PCR into user address space as well (read-only)
    //
    Pte.L1.Section.Access = SharedAccess;
    KeFillFixedEntryTb(Pte, (PVOID)USPCR, PCR_ENTRY + 1);
    
    //
    // Now we should be able to use the PCR...
    //
    Pcr = (PKPCR)KeGetPcr();
    
    //
    // Set the cache policy (HACK)
    //
    Pcr->CachePolicy = 0;
    Pcr->AlignedCachePolicy = 0;
    
    //
    // Copy cache information from the loader block
    //
    Pcr->FirstLevelDcacheSize = LoaderBlock->u.Arm.FirstLevelDcacheSize;
    Pcr->SecondLevelDcacheSize = LoaderBlock->u.Arm.SecondLevelDcacheSize;
    Pcr->FirstLevelIcacheSize = LoaderBlock->u.Arm.FirstLevelIcacheSize;
    Pcr->SecondLevelIcacheSize = LoaderBlock->u.Arm.SecondLevelIcacheSize;
    Pcr->FirstLevelDcacheFillSize = LoaderBlock->u.Arm.FirstLevelDcacheFillSize;
    Pcr->SecondLevelDcacheFillSize = LoaderBlock->u.Arm.SecondLevelDcacheFillSize;
    Pcr->FirstLevelIcacheFillSize = LoaderBlock->u.Arm.FirstLevelIcacheFillSize;
    Pcr->SecondLevelIcacheFillSize = LoaderBlock->u.Arm.SecondLevelIcacheFillSize;

    //
    // Set global d-cache fill and alignment values
    //
    if (Pcr->SecondLevelDcacheSize)
    {
        //
        // Use the first level
        //
        Pcr->DcacheFillSize = Pcr->SecondLevelDcacheSize;
    }
    else
    {
        //
        // Use the second level
        //
        Pcr->DcacheFillSize = Pcr->SecondLevelDcacheSize;
    }
    
    //
    // Set the alignment
    //
    Pcr->DcacheAlignment = Pcr->DcacheFillSize - 1;
    
    //
    // Set global i-cache fill and alignment values
    //
    if (Pcr->SecondLevelIcacheSize)
    {
        //
        // Use the first level
        //
        Pcr->IcacheFillSize = Pcr->SecondLevelIcacheSize;
    }
    else
    {
        //
        // Use the second level
        //
        Pcr->IcacheFillSize = Pcr->SecondLevelIcacheSize;
    }
    
    //
    // Set the alignment
    //
    Pcr->IcacheAlignment = Pcr->IcacheFillSize - 1;
    
    //
    // Now sweep caches
    //
    HalSweepIcache();
    HalSweepDcache();
    while (TRUE);
}
