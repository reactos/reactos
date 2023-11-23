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
extern PVOID HalpAPEntry16End;
extern HALP_APIC_INFO_TABLE HalpApicInfoTable;

ULONG HalpStartedProcessorCount = 1;

#ifndef Add2Ptr
#define Add2Ptr(P,I) ((PVOID)((PUCHAR)(P) + (I)))
#endif
#ifndef PtrOffset
#define PtrOffset(B,O) ((ULONG)((ULONG_PTR)(O) - (ULONG_PTR)(B)))
#endif

// Windows uses PROCESSOR_START_BLOCK (offsets defined in ksamd64.inc)
typedef struct _AP_ENTRY_DATA
{
    PKPROCESSOR_STATE ProcessorState;
} AP_ENTRY_DATA, *PAP_ENTRY_DATA;

/* FUNCTIONS *****************************************************************/

static
ULONG
HalpSetupTemporaryMappings(
    _In_ PKPROCESSOR_STATE ProcessorState)
{
    PMMPXE RootPageTable = Add2Ptr(HalpLowStub, 1 * PAGE_SIZE);
    PMMPPE PageTableLvl3 = Add2Ptr(HalpLowStub, 2 * PAGE_SIZE);
    PMMPDE PageTableLvl2 = Add2Ptr(HalpLowStub, 3 * PAGE_SIZE);
    PHYSICAL_ADDRESS PhysicalAddress;
    ULONG SelfMapPxi;

    /* Copy current mappings */
    RtlCopyMemory(RootPageTable, MiAddressToPxe(NULL), PAGE_SIZE);

    /* Set up self-mapping PXE */
    SelfMapPxi = MiAddressToPxi(MiAddressToPxe(NULL));
    PhysicalAddress = MmGetPhysicalAddress(RootPageTable);
    RootPageTable[SelfMapPxi].u.Flush.PageFrameNumber = PhysicalAddress.QuadPart >> PAGE_SHIFT;

    /* Set up low PXE */
    PhysicalAddress = MmGetPhysicalAddress(PageTableLvl3);
    RootPageTable[0].u.Flush.PageFrameNumber = PhysicalAddress.QuadPart >> PAGE_SHIFT;
    RootPageTable[0].u.Flush.Valid = 1;
    RootPageTable[0].u.Flush.Write = 1;

    /* Set up low PPE */
    PhysicalAddress = MmGetPhysicalAddress(PageTableLvl2);
    PageTableLvl3[0].u.Flush.PageFrameNumber = PhysicalAddress.QuadPart >> PAGE_SHIFT;
    PageTableLvl3[0].u.Flush.Valid = 1;
    PageTableLvl3[0].u.Flush.Write = 1;

    /* Set up a large-page low PDE */
    PageTableLvl2[0].u.Flush.PageFrameNumber = 0;
    PageTableLvl2[0].u.Flush.Valid = 1;
    PageTableLvl2[0].u.Flush.Write = 1;
    PageTableLvl2[0].u.Flush.LargePage = 1;

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
    PAP_ENTRY_DATA APEntryData;
    ULONG InitialCr3;

    if (HalpStartedProcessorCount == HalpApicInfoTable.ProcessorCount)
        return FALSE;

    /* Clean up low stub from any previous data */
    RtlZeroMemory(HalpLowStub, HALP_LOW_STUB_SIZE_IN_PAGES * PAGE_SIZE);

    /* Initalize the temporary page table */
    InitialCr3 = HalpSetupTemporaryMappings(ProcessorState);

    /* Put the bootstrap code into low memory */
    SIZE_T APEntrySize = (ULONG_PTR)&HalpAPEntry16End - (ULONG_PTR)&HalpAPEntry16;
    ASSERT(APEntrySize <= PAGE_SIZE);
    RtlCopyMemory(HalpLowStub, &HalpAPEntry16, APEntrySize);

    /* Get a pointer to APEntryData */
    SIZE_T Offset = PtrOffset(&HalpAPEntry16, &HalpAPEntryData);
    APEntryData = Add2Ptr(HalpLowStub, Offset);

    /* Fill in the APEntryData structure */
    APEntryData->ProcessorState = ProcessorState;

    /* Start the processor */
    ApicStartApplicationProcessor(HalpStartedProcessorCount, HalpLowStubPhysicalAddress);

    HalpStartedProcessorCount++;

    return TRUE;
}
