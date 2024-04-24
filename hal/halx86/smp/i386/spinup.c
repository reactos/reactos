/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     i386 Application Processor (AP) spinup setup
 * COPYRIGHT:   Copyright 2021 Victor Perevertkin <victor.perevertkin@reactos.org>
 *              Copyright 2021-2023 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#include <smp.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

extern PPROCESSOR_IDENTITY HalpProcessorIdentity;
extern PHYSICAL_ADDRESS HalpLowStubPhysicalAddress;
extern PVOID HalpLowStub;

// The data necessary for a boot (stored inside HalpLowStub)
extern PVOID HalpAPEntry16;
extern PVOID HalpAPEntryData;
extern PVOID HalpAPEntry32;
extern PVOID HalpAPEntry16End;
extern HALP_APIC_INFO_TABLE HalpApicInfoTable;

ULONG HalpStartedProcessorCount = 1;

#ifndef Add2Ptr
#define Add2Ptr(P,I) ((PVOID)((PUCHAR)(P) + (I)))
#endif
#ifndef PtrOffset
#define PtrOffset(B,O) ((ULONG)((ULONG_PTR)(O) - (ULONG_PTR)(B)))
#endif

typedef struct _AP_ENTRY_DATA
{
    UINT32 Jump32Offset;
    ULONG Jump32Segment;
    PVOID SelfPtr;
    ULONG PageTableRoot;
    PKPROCESSOR_STATE ProcessorState;
    KDESCRIPTOR Gdtr;
    KDESCRIPTOR Idtr;
} AP_ENTRY_DATA, *PAP_ENTRY_DATA;

/* FUNCTIONS *****************************************************************/

static
ULONG
HalpSetupTemporaryMappings(
    _In_ PKPROCESSOR_STATE ProcessorState)
{
    PMMPDE RootPageTable = Add2Ptr(HalpLowStub, PAGE_SIZE);
    PMMPDE LowMapPde = Add2Ptr(HalpLowStub, 2 * PAGE_SIZE);
    PMMPTE LowStubPte = MiAddressToPte(HalpLowStub);
    PHYSICAL_ADDRESS PhysicalAddress;
    ULONG StartPti;

    /* Copy current mappings */
    RtlCopyMemory(RootPageTable, MiAddressToPde(NULL), PAGE_SIZE);

    /* Set up low PDE */
    PhysicalAddress = MmGetPhysicalAddress(LowMapPde);
    RootPageTable[0].u.Hard.PageFrameNumber = PhysicalAddress.QuadPart >> PAGE_SHIFT;
    RootPageTable[0].u.Hard.Valid = 1;
    RootPageTable[0].u.Hard.Write = 1;

    /* Copy low stub PTEs */
    StartPti = MiAddressToPteOffset(HalpLowStubPhysicalAddress.QuadPart);
    ASSERT(StartPti + HALP_LOW_STUB_SIZE_IN_PAGES < 1024);
    for (ULONG i = 0; i < HALP_LOW_STUB_SIZE_IN_PAGES; i++)
    {
        LowMapPde[StartPti + i] = LowStubPte[i];
    }

    PhysicalAddress = MmGetPhysicalAddress(RootPageTable);
    ASSERT(PhysicalAddress.QuadPart < 0x100000000);
    return (ULONG)PhysicalAddress.QuadPart;
}

BOOLEAN
NTAPI
HalStartNextProcessor(
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock,
    _In_ PKPROCESSOR_STATE ProcessorState)
{
    if (HalpStartedProcessorCount == HalpApicInfoTable.ProcessorCount)
        return FALSE;

    // Initalize the temporary page table
    // TODO: clean it up after an AP boots successfully
    ULONG initialCr3 = HalpSetupTemporaryMappings(ProcessorState);
    if (!initialCr3)
        return FALSE;

    // Put the bootstrap code into low memory
    RtlCopyMemory(HalpLowStub, &HalpAPEntry16, (ULONG_PTR)&HalpAPEntry16End - (ULONG_PTR)&HalpAPEntry16);

    // Get a pointer to apEntryData
    PAP_ENTRY_DATA apEntryData = (PVOID)((ULONG_PTR)HalpLowStub + ((ULONG_PTR)&HalpAPEntryData - (ULONG_PTR)&HalpAPEntry16));

    *apEntryData = (AP_ENTRY_DATA){
        .Jump32Offset = (ULONG)&HalpAPEntry32,
        .Jump32Segment = (ULONG)ProcessorState->ContextFrame.SegCs,
        .SelfPtr = (PVOID)apEntryData,
        .PageTableRoot = initialCr3,
        .ProcessorState = ProcessorState,
        .Gdtr = ProcessorState->SpecialRegisters.Gdtr,
        .Idtr = ProcessorState->SpecialRegisters.Idtr,
    };

    ApicStartApplicationProcessor(HalpStartedProcessorCount, HalpLowStubPhysicalAddress);

    HalpStartedProcessorCount++;

    return TRUE;
}
