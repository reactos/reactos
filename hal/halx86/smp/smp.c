/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Core source file for SMP management
 * COPYRIGHT:   Copyright 2021 Justin Miller <justinmiller100@gmail.com>
 *              Copyright 2021 Victor Perevertkin <victor.perevertkin@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#include <smp.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

extern PHYSICAL_ADDRESS HalpLowStubPhysicalAddress;
extern PVOID HalpLowStub;

// The data necessary for a boot (stored inside HalpLowStub)
extern PVOID APEntry16;
extern PVOID APEntry16End;
extern PVOID APEntry32;
extern PVOID APEntryJump32Offset;
extern PVOID APEntryJump32Segment;
extern PVOID TempPageTableAddr;
extern PVOID APEntryCpuState;

/* TODO: MaxAPCount should be assigned by a Multi APIC table */
ULONG MaxAPCount = 2;
ULONG StartedProcessorCount = 1;

typedef struct _AP_ENTRY_CPU_STATE
{
    PVOID SelfPtr;
    UINT32 Esp;
    UINT32 Eip;
    UINT32 Eflags;
    UINT32 SegCs;
    UINT32 SegDs;
    UINT32 SegEs;
    UINT32 SegSs;
    UINT32 SegFs;
    UINT32 SegGs;
    KSPECIAL_REGISTERS SpecialRegisters; 
} AP_ENTRY_CPU_STATE, *PAP_ENTRY_CPU_STATE;

/* FUNCTIONS *****************************************************************/


static
VOID
HalpMapAddressFlat(
    _Inout_ PMMPDE PageDirectory,
    _In_ PVOID VirtAddress,
    _In_ PVOID TargetVirtAddress)
{
    if (TargetVirtAddress == NULL)
        TargetVirtAddress = VirtAddress;

    PMMPDE currentPde;

    currentPde = &PageDirectory[MiAddressToPdeOffset(TargetVirtAddress)];

    // Allocate a Page Table if there is no one for this address
    if (currentPde->u.Long == 0)
    {
        PMMPTE pageTable = ExAllocatePoolZero(NonPagedPool, PAGE_SIZE, TAG_HAL);
        ASSERT(pageTable);

        currentPde->u.Hard.PageFrameNumber = MmGetPhysicalAddress(pageTable).QuadPart >> PAGE_SHIFT;
        currentPde->u.Hard.Valid = TRUE;
        currentPde->u.Hard.Write = TRUE;
    }
    
    // Map the Page Table so we can add our VirtAddress there (hack around I/O memory mapper for that)
    PHYSICAL_ADDRESS b = {.QuadPart = (ULONG_PTR)currentPde->u.Hard.PageFrameNumber << PAGE_SHIFT};

    PMMPTE pageTable = MmMapIoSpace(b, PAGE_SIZE, MmCached);

    PMMPTE currentPte = &pageTable[MiAddressToPteOffset(TargetVirtAddress)];
    currentPte->u.Hard.PageFrameNumber = MmGetPhysicalAddress(VirtAddress).QuadPart >> PAGE_SHIFT;
    currentPte->u.Hard.Valid = TRUE;
    currentPte->u.Hard.Write = TRUE;

    MmUnmapIoSpace(pageTable, PAGE_SIZE);

    DPRINT("Map %p -> %p, PDE %u PTE %u\n",
           TargetVirtAddress,
           (PVOID)MmGetPhysicalAddress(VirtAddress).LowPart,
           MiAddressToPdeOffset(TargetVirtAddress),
           MiAddressToPteOffset(TargetVirtAddress));
}

static
PHYSICAL_ADDRESS
HalpInitTempPageTable(
    _In_ PKPROCESSOR_STATE ProcessorState)
{
    PMMPDE pageDirectory = ExAllocatePoolZero(NonPagedPool, PAGE_SIZE, TAG_HAL);
    ASSERT(pageDirectory);

    // Map the low stub
    HalpMapAddressFlat(pageDirectory, HalpLowStub, (PVOID)(ULONG_PTR)HalpLowStubPhysicalAddress.QuadPart);
    HalpMapAddressFlat(pageDirectory, HalpLowStub, NULL);

    // Map 32bit mode entry point
    HalpMapAddressFlat(pageDirectory, &APEntry32, NULL);

    // Map GDT
    HalpMapAddressFlat(pageDirectory, (PVOID)ProcessorState->SpecialRegisters.Gdtr.Base, NULL);

    // Map IDT
    HalpMapAddressFlat(pageDirectory, (PVOID)ProcessorState->SpecialRegisters.Idtr.Base, NULL);

    // __debugbreak();

    return MmGetPhysicalAddress(pageDirectory);
}

BOOLEAN
NTAPI
HalStartNextProcessor(
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock,
    _In_ PKPROCESSOR_STATE ProcessorState)
{
    if (MaxAPCount > StartedProcessorCount)
    { 
        // Initalize the temporary page table
        // TODO: clean it up after an AP boots successfully
        ULONG_PTR initialCr3 = HalpInitTempPageTable(ProcessorState).QuadPart;

        // Put the bootstrap code into low memory
        RtlCopyMemory(HalpLowStub, &APEntry16,  ((ULONG_PTR)&APEntry16End - (ULONG_PTR)&APEntry16));

        // Set the data for 16bit entry code
        PUINT32 APEntryJump32OffsetPtr = (PUINT32)((ULONG_PTR)HalpLowStub + (ULONG_PTR)&APEntryJump32Offset - (ULONG_PTR)&APEntry16);
        PUINT32 APEntryJump32SegmentPtr = (PUINT32)((ULONG_PTR)HalpLowStub + (ULONG_PTR)&APEntryJump32Segment - (ULONG_PTR)&APEntry16);
        PUINT32 TempPageTableAddrPtr = (PUINT32)((ULONG_PTR)HalpLowStub + (ULONG_PTR)&TempPageTableAddr - (ULONG_PTR)&APEntry16);

        PVOID buf = &APEntry32;

        RtlCopyMemory(APEntryJump32OffsetPtr, &buf, sizeof(buf));
        RtlCopyMemory(APEntryJump32SegmentPtr, &ProcessorState->ContextFrame.SegCs, sizeof(UINT16));
        RtlCopyMemory(TempPageTableAddrPtr, &initialCr3, sizeof(initialCr3));

        // Write processor state stuff
        PAP_ENTRY_CPU_STATE apCpuState = (PVOID)((ULONG_PTR)HalpLowStub + ((ULONG_PTR)&APEntryCpuState - (ULONG_PTR)&APEntry16));
        *apCpuState = (AP_ENTRY_CPU_STATE){
            .SelfPtr = apCpuState,
            .Esp = ProcessorState->ContextFrame.Esp,
            .Eip = ProcessorState->ContextFrame.Eip,
            .Eflags = ProcessorState->ContextFrame.EFlags,
            .SegCs = ProcessorState->ContextFrame.SegCs,
            .SegDs = ProcessorState->ContextFrame.SegDs,
            .SegEs = ProcessorState->ContextFrame.SegEs,
            .SegSs = ProcessorState->ContextFrame.SegSs,
            .SegFs = ProcessorState->ContextFrame.SegFs,
            .SegGs = ProcessorState->ContextFrame.SegGs,
            .SpecialRegisters = ProcessorState->SpecialRegisters,
        };

        ApicStartApplicationProcessor(StartedProcessorCount, HalpLowStubPhysicalAddress);

        StartedProcessorCount++;

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

