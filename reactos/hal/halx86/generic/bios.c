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

//
// PTE Data
//
ULONG HalpSavedPfn;
HARDWARE_PTE HalpSavedPte;

//
// IDT Data
//
PVOID HalpGpfHandler;
PVOID HalpBopHandler;

//
// TSS Data
//
ULONG HalpSavedEsp0;
USHORT HalpSavedTss;

//
// IOPM Data
//
USHORT HalpSavedIopmBase;
PUSHORT HalpSavedIoMap;
USHORT HalpSavedIoMapData[32][2];
ULONG HalpSavedIoMapEntries;

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
HalpBorrowTss(VOID)
{
    USHORT Tss;
    PKGDTENTRY TssGdt;
    ULONG_PTR TssLimit;
    PKTSS TssBase;

    //
    // Get the current TSS and its GDT entry
    //
    Tss = Ke386GetTr();
    TssGdt = &((PKIPCR)KeGetPcr())->GDT[Tss / sizeof(KGDTENTRY)];

    //
    // Get the KTSS limit and check if it has IOPM space
    //
    TssLimit = TssGdt->LimitLow | TssGdt->HighWord.Bits.LimitHi << 16;

    //
    // If the KTSS doesn't have enough space this is probably an NMI or DF
    //
    if (TssLimit > IOPM_SIZE)
    {
        //
        // We are good to go
        //
        HalpSavedTss = 0;
        return;
    }

    //
    // Get the "real" TSS
    //
    TssGdt = &((PKIPCR)KeGetPcr())->GDT[KGDT_TSS / sizeof(KGDTENTRY)];
    TssBase = (PKTSS)(ULONG_PTR)(TssGdt->BaseLow |
                                 TssGdt->HighWord.Bytes.BaseMid << 16 |
                                 TssGdt->HighWord.Bytes.BaseHi << 24);

    //
    // Switch to it
    //
    KeGetPcr()->TSS = TssBase;

    //
    // Set it up
    //
    TssGdt->HighWord.Bits.Type = I386_TSS;
    TssGdt->HighWord.Bits.Pres = 1;
    TssGdt->HighWord.Bits.Dpl = 0;
    
    //
    // Load new TSS and return old one
    //
    Ke386SetTr(KGDT_TSS);
    HalpSavedTss = Tss;
}

VOID
NTAPI
HalpReturnTss(VOID)
{
    PKGDTENTRY TssGdt;
    PKTSS TssBase;
    
    //
    // Get the original TSS
    //
    TssGdt = &((PKIPCR)KeGetPcr())->GDT[HalpSavedTss / sizeof(KGDTENTRY)];
    TssBase = (PKTSS)(ULONG_PTR)(TssGdt->BaseLow |
                                 TssGdt->HighWord.Bytes.BaseMid << 16 |
                                 TssGdt->HighWord.Bytes.BaseHi << 24);

    //
    // Switch to it
    //
    KeGetPcr()->TSS = TssBase;

    //
    // Set it up
    //
    TssGdt->HighWord.Bits.Type = I386_TSS;
    TssGdt->HighWord.Bits.Pres = 1;
    TssGdt->HighWord.Bits.Dpl = 0;

    //
    // Load old TSS
    //
    Ke386SetTr(HalpSavedTss);
}

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

    //
    // Get the page table directory for the lowest meg of memory
    //
    Pte = HalAddressToPde(0);
    HalpSavedPfn = Pte->PageFrameNumber;
    HalpSavedPte = *Pte;

    //
    // Map it to the HAL reserved region and make it valid
    //
    Pte->Valid = 1;
    Pte->Write = 1;
    Pte->Owner = 1;
    Pte->PageFrameNumber = (HalAddressToPde(0xFFC00000))->PageFrameNumber;

    //
    // Flush the TLB
    //
    HalpFlushTLB();

    //
    // Now loop the first meg of memory
    //
    for (i = 0; i < 0x100000; i += PAGE_SIZE)
    {
        //
        // Identity map it
        //
        Pte = HalAddressToPte(i);
        Pte->PageFrameNumber = i >> PAGE_SHIFT;
        Pte->Valid = 1;
        Pte->Write = 1;
        Pte->Owner = 1;
    }

    //
    // Now get the entry for our real mode V86 code and the target
    //
    Pte = HalAddressToPte(0x20000);
    V86Pte = HalAddressToPte(&HalpRealModeStart);
    do
    {
        //
        // Map the physical address into our real-mode region
        //
        Pte->PageFrameNumber = V86Pte->PageFrameNumber;
        
        //
        // Keep going until we've reached the end of our region
        //
        Pte++;
        V86Pte++;
    } while (V86Pte <= HalAddressToPte(&HalpRealModeEnd));

    //
    // Flush the TLB
    //
    HalpFlushTLB();
}

VOID
NTAPI
HalpSwitchToRealModeTrapHandlers(VOID)
{
    //
    // Save the current Invalid Opcode and General Protection Fault Handlers
    //
    HalpGpfHandler = KeQueryInterruptHandler(13);
    HalpBopHandler = KeQueryInterruptHandler(6);

    //
    // Now set our own GPF handler to handle exceptions while in real mode
    //
    KeRegisterInterruptHandler(13, HalpTrap0D);

    //
    // And our own invalid opcode handler to detect the BOP to get us out
    //
    KeRegisterInterruptHandler(6, HalpTrap06);
}

VOID
NTAPI
HalpSetupRealModeIoPermissionsAndTask(VOID)
{
    //
    // Switch to valid TSS
    //
    HalpBorrowTss();

    //
    // Save a copy of the I/O Map and delete it
    //
    HalpSavedIoMap = (PUSHORT)&(KeGetPcr()->TSS->IoMaps[0]);
    HalpStoreAndClearIopm();

    //
    // Save the IOPM and switch to the real-mode one
    //
    HalpSavedIopmBase = KeGetPcr()->TSS->IoMapBase;
    KeGetPcr()->TSS->IoMapBase = KiComputeIopmOffset(1);

    //
    // Save our stack pointer
    //
    HalpSavedEsp0 = KeGetPcr()->TSS->Esp0; 
}

VOID
NTAPI
HalpRestoreTrapHandlers(VOID)
{
    //
    // Keep dummy GPF handler in case we get an NMI during V8086
    //
    if (!HalpNMIInProgress)
    {
        //
        // Not an NMI -- put back the original handler
        //
        KeRegisterInterruptHandler(13, HalpGpfHandler);
    }

    //
    // Restore invalid opcode handler
    //
    KeRegisterInterruptHandler(6, HalpBopHandler);
}

VOID
NTAPI
HalpRestoreIoPermissionsAndTask(VOID)
{
    //
    // Restore the stack pointer
    //
    KeGetPcr()->TSS->Esp0 = HalpSavedEsp0;

    //
    // Restore the I/O Map
    //
    HalpRestoreIopm();

    //
    // Restore the IOPM
    //
    KeGetPcr()->TSS->IoMapBase = HalpSavedIopmBase;

    //
    // Restore the TSS
    //
    if (HalpSavedTss) HalpReturnTss();
}

VOID
NTAPI
HalpUnmapRealModeMemory(VOID)
{
    ULONG i;
    PHARDWARE_PTE Pte;

    //
    // Loop the first meg of memory
    //
    for (i = 0; i < 0x100000; i += PAGE_SIZE)
    {
        //
        // Invalidate each PTE
        //
        Pte = HalAddressToPte(i);
        Pte->Valid = 0;
        Pte->Write = 0;
        Pte->Owner = 0;
        Pte->PageFrameNumber = 0;
    }

    //
    // Restore the PDE for the lowest megabyte of memory
    //
    Pte = HalAddressToPde(0);
    *Pte = HalpSavedPte;
    Pte->PageFrameNumber = HalpSavedPfn;

    //
    // Flush the TLB
    //
    HalpFlushTLB();
}

BOOLEAN
NTAPI
HalpBiosDisplayReset(VOID)
{
    ULONG Flags;
    PHARDWARE_PTE IdtPte;
    BOOLEAN RestoreWriteProtection = FALSE;

    //
    // Disable interrupts
    //
    Flags = __readeflags();
    _disable();

    //
    // Map memory available to the V8086 real-mode code
    //
    HalpMapRealModeMemory();

    // 
    // On P5, the first 7 entries of the IDT are write protected to work around
    // the cmpxchg8b lock errata. Unprotect them here so we can set our custom
    // invalid op-code handler.
    //
    IdtPte = HalAddressToPte(((PKIPCR)KeGetPcr())->IDT);
    RestoreWriteProtection = IdtPte->Write;
    IdtPte->Write = 1;

    //
    // Use special invalid opcode and GPF trap handlers
    //
    HalpSwitchToRealModeTrapHandlers();

    //
    // Configure the IOPM and TSS
    //
    HalpSetupRealModeIoPermissionsAndTask();

    //
    // Now jump to real mode
    //
    HalpBiosCall();

    //
    // Restore kernel trap handlers
    //
    HalpRestoreTrapHandlers();

    //
    // Restore write permission
    //
    IdtPte->Write = RestoreWriteProtection;

    //
    // Restore TSS and IOPM
    //
    HalpRestoreIoPermissionsAndTask();
    
    //
    // Restore low memory mapping
    //
    HalpUnmapRealModeMemory();

    //
    // Restore interrupts if they were previously enabled
    //
    __writeeflags(Flags);
    return TRUE;
}

/* EOF */
