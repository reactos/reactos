/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/generic/bios.c
 * PURPOSE:         BIOS Access Routines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

UCHAR HalpIopmSaveBuffer[0x2000];
ULONG HalpSavedPfn;
HARDWARE_PTE HalpSavedPte;
ULONG HalpGpfHandler;
ULONG HalpBopHandler;
USHORT HalpSavedIopmBase;
PUCHAR HalpSavedIoMap;
ULONG HalpSavedEsp0;

#define GetPdeAddress(x) (PHARDWARE_PTE)(((((ULONG_PTR)(x)) >> 22) << 2) + 0xC0300000)
#define GetPteAddress(x) (PHARDWARE_PTE)(((((ULONG_PTR)(x)) >> 12) << 2) + 0xC0000000)

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
HalpStoreAndClearIopm(IN PUCHAR IoMap)
{
    ULONG i;
    
    /* Backup the old I/O Map */
    RtlCopyMemory(HalpIopmSaveBuffer, IoMap, 0x2000);

    /* Erase the current one */
    for (i = 0; i < 0x2000; i++) IoMap[i] = 0;
    for (i = 0x2000; i < 0x2004; i++) IoMap[i] = 0xFF;
}

VOID
NTAPI
HalpRestoreIopm(IN PUCHAR IoMap)
{
    ULONG i;

    /* Restore the backed up copy, and initialize it */
    RtlCopyMemory(IoMap, HalpIopmSaveBuffer, 0x2000);
    for (i = 0x2000; i < 0x2004; i++) IoMap[i] = 0xFF;
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
    HalpSavedIoMap = (PUCHAR)&(KeGetPcr()->TSS->IoMaps[0]);
    HalpStoreAndClearIopm(HalpSavedIoMap);
    
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
    HalpRestoreIopm(HalpSavedIoMap);
    
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
    ULONG Flags = 0;

    /* Disable interrupts */
    Ke386SaveFlags(Flags);
    _disable();

    /* Map memory available to the V8086 real-mode code */
    HalpMapRealModeMemory();

    /* Use special invalid opcode and GPF trap handlers */
    HalpSwitchToRealModeTrapHandlers();

    /* Configure the IOPM and TSS */
    HalpSetupRealModeIoPermissionsAndTask();

    /* Now jump to real mode */
    HalpBiosCall();

    /* Restore kernel trap handlers */
    HalpRestoreTrapHandlers();
    
    /* Restore TSS and IOPM */
    HalpRestoreIoPermissionsAndTask();
    
    /* Restore low memory mapping */
    HalpUnmapRealModeMemory();

    /* Restore interrupts if they were previously enabled */
    Ke386RestoreFlags(Flags);
    return TRUE;
}

/* EOF */
