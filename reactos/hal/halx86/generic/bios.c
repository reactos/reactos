/*
 * PROJECT:         ReactOS Hardware Abstraction Layer (HAL)
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            halx86/generic/bios.c
 * PURPOSE:         BIOS Access Routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

ULONG HalpSavedPfn;
HARDWARE_PTE HalpSavedPte;
ULONG HalpGpfHandler;
ULONG HalpBopHandler;
ULONG HalpSavedEsp0;
USHORT HalpSavedIopmBase;
PUSHORT HalpSavedIoMap;
USHORT HalpSavedIoMapData[32][2];
ULONG HalpSavedIoMapEntries;

#define GetPdeAddress(x) (PHARDWARE_PTE)(((((ULONG_PTR)(x)) >> 22) << 2) + 0xC0300000)
#define GetPteAddress(x) (PHARDWARE_PTE)(((((ULONG_PTR)(x)) >> 12) << 2) + 0xC0000000)

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
HalpStoreAndClearIopm(VOID)
{
    ULONG i, j;
    PUSHORT Entry = HalpSavedIoMap;

    //
    // Loop the I/O Map
    //
    for (i = j = 0; i < (IOPM_SIZE) / 2; i++)
    {
        //
        // Check for non-FFFF entry
        //
        if (*Entry != 0xFFFF)
        {
            //
            // Save it
            //
            ASSERT(j < 32);
            HalpSavedIoMapData[j][0] = i;
            HalpSavedIoMapData[j][1] = *Entry;
        }

        //
        // Clear it
        //
        *Entry++ = 0;
    }

    //
    // Terminate it
    //
    while (i++ < (IOPM_FULL_SIZE / 2)) *Entry++ = 0xFFFF;

    //
    // Return the entries we saved
    //
    HalpSavedIoMapEntries = j;
}

VOID
NTAPI
HalpRestoreIopm(VOID)
{
    ULONG i = HalpSavedIoMapEntries;

    //
    // Set default state
    //
    RtlFillMemory(HalpSavedIoMap, 0xFF, IOPM_FULL_SIZE);

    //
    // Restore the backed up copy, and initialize it
    //
    while (i--) HalpSavedIoMap[HalpSavedIoMapData[i][0]] = HalpSavedIoMapData[i][1];
}

VOID
NTAPI
HalpMapRealModeMemory(VOID)
{
    PHARDWARE_PTE Pte, V86Pte;
    ULONG i;
    
    /* Get the page table directory for the lowest meg of memory */
    Pte = GetPdeAddress(0);
    HalpSavedPfn = Pte->PageFrameNumber;
    HalpSavedPte = *Pte;
    
    /* Map it to the HAL reserved region and make it valid */ 
    Pte->Valid = 1;
    Pte->Write = 1;
    Pte->Owner = 1;
    Pte->PageFrameNumber = (GetPdeAddress(0xFFC00000))->PageFrameNumber;
    
    /* Flush the TLB by resetting CR3 */
    __writecr3(__readcr3());
    
    /* Now loop the first meg of memory */
    for (i = 0; i < 0x100000; i += PAGE_SIZE)
    {
        /* Identity map it */
        Pte = GetPteAddress((PVOID)i);
        Pte->PageFrameNumber = i >> PAGE_SHIFT;
        Pte->Valid = 1;
        Pte->Write = 1;
        Pte->Owner = 1;
    }
    
    /* Now get the entry for our real mode V86 code and the target */
    Pte = GetPteAddress(0x20000);
    V86Pte = GetPteAddress(&HalpRealModeStart);
    do
    {
        /* Map the physical address into our real-mode region */
        Pte->PageFrameNumber = V86Pte->PageFrameNumber;
        
        /* Keep going until we've reached the end of our region */
        Pte++;
        V86Pte++;
    } while (V86Pte <= GetPteAddress(&HalpRealModeEnd));
    
    /* Flush the TLB by resetting CR3 */
    __writecr3(__readcr3());
}

VOID
NTAPI
HalpSwitchToRealModeTrapHandlers(VOID)
{
    ULONG Handler;

    /* Save the current Invalid Opcode and General Protection Fault Handlers */
    HalpGpfHandler = ((((PKIPCR)KeGetPcr())->IDT[13].ExtendedOffset << 16) &
                       0xFFFF0000) |
        (((PKIPCR)KeGetPcr())->IDT[13].Offset & 0xFFFF);
    HalpBopHandler = ((((PKIPCR)KeGetPcr())->IDT[6].ExtendedOffset << 16) &
                       0xFFFF0000) |
        (((PKIPCR)KeGetPcr())->IDT[6].Offset & 0xFFFF);
    
    /* Now set our own GPF handler to handle exceptions while in real mode */
    Handler = (ULONG_PTR)HalpTrap0D;
    ((PKIPCR)KeGetPcr())->IDT[13].ExtendedOffset =
        (USHORT)((Handler >> 16) & 0xFFFF);
    ((PKIPCR)KeGetPcr())->IDT[13].Offset = (USHORT)Handler;
    
    /* And our own invalid opcode handler to detect the BOP to get us out */
    Handler = (ULONG_PTR)HalpTrap06;
    ((PKIPCR)KeGetPcr())->IDT[6].ExtendedOffset =
        (USHORT)((Handler >> 16) & 0xFFFF);
    ((PKIPCR)KeGetPcr())->IDT[6].Offset = (USHORT)Handler;
}

VOID
NTAPI
HalpSetupRealModeIoPermissionsAndTask(VOID)
{
    /* Save a copy of the I/O Map and delete it */
    HalpSavedIoMap = (PUSHORT)&(KeGetPcr()->TSS->IoMaps[0]);
    HalpStoreAndClearIopm();
    
    /* Save the IOPM and switch to the real-mode one */
    HalpSavedIopmBase = KeGetPcr()->TSS->IoMapBase;
    KeGetPcr()->TSS->IoMapBase = KiComputeIopmOffset(1);
    
    /* Save our stack pointer */
    HalpSavedEsp0 = KeGetPcr()->TSS->Esp0; 
}

VOID
NTAPI
HalpRestoreTrapHandlers(VOID)
{
    /* We're back, restore the handlers we over-wrote */
    ((PKIPCR)KeGetPcr())->IDT[13].ExtendedOffset =
    (USHORT)((HalpGpfHandler >> 16) & 0xFFFF);
    ((PKIPCR)KeGetPcr())->IDT[13].Offset = (USHORT)HalpGpfHandler;    
    ((PKIPCR)KeGetPcr())->IDT[6].ExtendedOffset =
        (USHORT)((HalpBopHandler >> 16) & 0xFFFF);
    ((PKIPCR)KeGetPcr())->IDT[6].Offset = (USHORT)HalpBopHandler;
}

VOID
NTAPI
HalpRestoreIoPermissionsAndTask(VOID)
{
    /* Restore the stack pointer */
    KeGetPcr()->TSS->Esp0 = HalpSavedEsp0;
    
    /* Restore the I/O Map */
    HalpRestoreIopm();
    
    /* Restore the IOPM */
    KeGetPcr()->TSS->IoMapBase = HalpSavedIopmBase;    
}

VOID
NTAPI
HalpUnmapRealModeMemory(VOID)
{
    ULONG i;
    PHARDWARE_PTE Pte;

    /* Loop the first meg of memory */
    for (i = 0; i < 0x100000; i += PAGE_SIZE)
    {
        /* Invalidate each PTE */
        Pte = GetPteAddress((PVOID)i);
        Pte->Valid = 0;
        Pte->Write = 0;
        Pte->Owner = 0;
        Pte->PageFrameNumber = 0;
    }
    
    /* Restore the PDE for the lowest megabyte of memory */
    Pte = GetPdeAddress(0);
    *Pte = HalpSavedPte;
    Pte->PageFrameNumber = HalpSavedPfn;
    
    /* Flush the TLB by resetting CR3 */
    __writecr3(__readcr3());
}

BOOLEAN
NTAPI
HalpBiosDisplayReset(VOID)
{
    ULONG Flags;
    PHARDWARE_PTE IdtPte;
    BOOLEAN RestoreWriteProtection = FALSE;

    /* Disable interrupts */
    Flags = __readeflags();
    _disable();

    /* Map memory available to the V8086 real-mode code */
    HalpMapRealModeMemory();

    /* 
     * On P5, the first 7 entries of the IDT are write protected to work around
     * the cmpxchg8b lock errata. Unprotect them here so we can set our custom
     * invalid op-code handler.
     */
    if (KeGetCurrentPrcb()->CpuType == 5)
    {
        /* Get the PTE and check if it is has been write protected yet  */
        IdtPte = GetPteAddress(((PKIPCR)KeGetPcr())->IDT);
        if (IdtPte->Write == 0)
        {
            /* Remove the protection and flush the TLB */
            IdtPte->Write = 1;
            __writecr3(__readcr3());
            RestoreWriteProtection = TRUE;
        }
    }

    /* Use special invalid opcode and GPF trap handlers */
    HalpSwitchToRealModeTrapHandlers();

    /* Configure the IOPM and TSS */
    HalpSetupRealModeIoPermissionsAndTask();

    /* Now jump to real mode */
    HalpBiosCall();

    /* Restore kernel trap handlers */
    HalpRestoreTrapHandlers();

    /* Check if we removed the write protection before */
    if (RestoreWriteProtection)
    {
        /* Get the PTE, restore the write protection and flush the TLB */
        IdtPte = GetPteAddress(((PKIPCR)KeGetPcr())->IDT);
        IdtPte->Write = 0;
        __writecr3(__readcr3());
    }
    
    /* Restore TSS and IOPM */
    HalpRestoreIoPermissionsAndTask();
    
    /* Restore low memory mapping */
    HalpUnmapRealModeMemory();

    /* Restore interrupts if they were previously enabled */
    __writeeflags(Flags);
    return TRUE;
}

/* EOF */
