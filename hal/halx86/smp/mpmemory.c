/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Source file for private memory functions specific to SMP
 * COPYRIGHT:  Copyright 2021 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#include <smp.h>
#include "smpp.h"
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

#ifdef _M_AMD64
typedef struct _APSTUB
{
    UINT64 StructAPCr0;
    UINT64 StructAPCr2;
    UINT64 StructAPCr3;
    UINT64 StructAPCr4;
    UINT64 StructAPSegCs;
    UINT64 StructAPSegSs;
    UINT64 StructAPSegDs;
    UINT64 StructAPSegEs;
    UINT64 StructAPSegGs;
    UINT64 StructAPSegFs;
    UINT64 StructAPRip;
} APSTUB, *PAPSTUB;

#elif _M_IX86
typedef struct _APSTUB
{
    UINT32 StructAPCr0;
    UINT32 StructAPCr2;
    UINT32 StructAPCr3;
    UINT32 StructAPCr4;
    UINT32 StructAPSegCs;
    UINT32 StructAPSegSs;
    UINT32 StructAPSegDs;
    UINT32 StructAPSegEs;
    UINT32 StructAPSegGs;
    UINT32 StructAPSegFs;
    UINT32 StructAPTr;
    UINT32 StructAPEip;
    UINT32 StructAPEsp;
    UINT32 StructAPEcx;
} APSTUB, *PAPSTUB;
#endif

/* AP spinup stub univeral definietions */
extern PVOID APEntry;
extern PVOID APEntryEnd;
extern PVOID APSpinup;
extern PVOID APSpinupEnd;
extern PVOID APFinal;
extern PVOID APFinalEnd;
extern UINT32 TempPageTableLoc;
extern UINT16 APJumpOffset;
extern UINT16 APFinalOffset;
extern KDESCRIPTOR APGDT;
extern KDESCRIPTOR APIDT;
extern APSTUB APProcessorStateStruct;
extern PHYSICAL_ADDRESS HalpLowStubPhysicalAddress;

/* FUNCTIONS *****************************************************************/

VOID
HalpInitializeAPStub(PVOID APStubLocation, 
                     KDESCRIPTOR FinalGdt, 
                     KDESCRIPTOR FinalIdt)
{
    DPRINT1("HalpInitializeAPStub: Writing APBootStub\n");
    PVOID APStubThirdPhaseLoc, APStubSecondPhaseLoc, APJumppLoc, APFinalLoc;
    PKDESCRIPTOR GdtLoc, IdtLoc;

    /* Get the locations used to copy over */
    APJumppLoc = (PUSHORT)((ULONG_PTR)APStubLocation + (ULONG_PTR)&APJumpOffset - (ULONG_PTR)&APEntry); 
    APFinalLoc = (PUSHORT)((ULONG_PTR)APStubLocation + ((ULONG_PTR)&APFinalOffset - (ULONG_PTR)&APSpinup) + ((ULONG_PTR)&APEntryEnd  - (ULONG_PTR)&APEntry)); 
    APStubSecondPhaseLoc = (PVOID)((ULONG_PTR)APStubLocation + ((ULONG_PTR)&APEntryEnd  - (ULONG_PTR)&APEntry));
    APStubThirdPhaseLoc = (PVOID)((ULONG_PTR)APStubLocation + ((ULONG_PTR)&APEntryEnd  - (ULONG_PTR)&APEntry) + ((ULONG_PTR)&APSpinupEnd - (ULONG_PTR)&APSpinup));

    /* Write the AP stub code */
    RtlCopyMemory(APStubLocation, &APEntry,  ((ULONG_PTR)&APEntryEnd - (ULONG_PTR)&APEntry));
    RtlCopyMemory(APStubSecondPhaseLoc, &APSpinup,  ((ULONG_PTR)&APSpinupEnd - (ULONG_PTR)&APSpinup));
    RtlCopyMemory(APStubThirdPhaseLoc, &APFinal,  ((ULONG_PTR)&APFinalEnd - (ULONG_PTR)&APFinal));
    
    /* Calculate and Write Locations for jumps */
    APJumpOffset = (UINT16)(((ULONG_PTR)APStubLocation * 4) + ((ULONG_PTR)&APEntryEnd  - (ULONG_PTR)&APEntry));
    APFinalOffset = (UINT16)(((ULONG_PTR)APStubLocation * 4) + ((ULONG_PTR)&APEntryEnd  - (ULONG_PTR)&APEntry) + ((ULONG_PTR)&APSpinupEnd - (ULONG_PTR)&APSpinup));
    RtlCopyMemory(APJumppLoc, &APJumpOffset,  sizeof(APJumpOffset));
    RtlCopyMemory(APFinalLoc, &APFinalOffset,  sizeof(APFinalOffset));

    /* Copy GDT Descriptor into area */
    GdtLoc = (PKDESCRIPTOR)((ULONG_PTR)APStubLocation + ((ULONG_PTR)&APGDT - (ULONG_PTR)&APSpinup) + (ULONG_PTR)&APEntryEnd  - (ULONG_PTR)&APEntry);
    GdtLoc->Limit = FinalGdt.Limit;
    GdtLoc->Base = FinalGdt.Base;
    DPRINT1("The GDT Limit.Base is %X.%X\n", GdtLoc->Limit, GdtLoc->Base);

    /* Copy IDT Descriptor into area */
    IdtLoc = (PKDESCRIPTOR)((ULONG_PTR)APStubLocation + ((ULONG_PTR)&APIDT - (ULONG_PTR)&APSpinup) + (ULONG_PTR)&APEntryEnd  - (ULONG_PTR)&APEntry);
    IdtLoc->Limit = FinalIdt.Limit;
    IdtLoc->Base = FinalIdt.Base;
    DPRINT1("The IDT Limit.Base is %X.%X\n", IdtLoc->Limit, IdtLoc->Base);
}

#ifdef _M_AMD64
VOID
HalpWriteProcessorState(PVOID APStubLocation, 
                        PKPROCESSOR_STATE ProcessorState,
                        UINT32 LoaderBlock)
{
    UNIMPLEMENTED;
}

#elif _M_IX86
VOID
HalpWriteProcessorState(PVOID APStubLocation, 
                        PKPROCESSOR_STATE ProcessorState,
                        UINT32 LoaderBlock)
{
    APSTUB APStub;
    PVOID APProcessorStateLoc;
    APProcessorStateLoc = (PVOID)((ULONG_PTR)APStubLocation + ((ULONG_PTR)&APProcessorStateStruct - (ULONG_PTR)&APFinal) + ((ULONG_PTR)&APSpinupEnd - (ULONG_PTR)&APSpinup) + ((ULONG_PTR)&APEntryEnd  - (ULONG_PTR)&APEntry));
    DPRINT1("HalpWriteProcessorState: Writing ProcessorState into the AP BootStub\n");
    APStub.StructAPCr0 = ProcessorState->SpecialRegisters.Cr0;
    APStub.StructAPCr2 = ProcessorState->SpecialRegisters.Cr2;
    APStub.StructAPCr3 = ProcessorState->SpecialRegisters.Cr3;
    APStub.StructAPCr4 = ProcessorState->SpecialRegisters.Cr4;
    APStub.StructAPSegCs = ProcessorState->ContextFrame.SegCs;
    APStub.StructAPSegSs = ProcessorState->ContextFrame.SegSs;
    APStub.StructAPSegDs = ProcessorState->ContextFrame.SegDs;
    APStub.StructAPSegEs = ProcessorState->ContextFrame.SegDs;
    APStub.StructAPSegGs = ProcessorState->ContextFrame.SegGs;
    APStub.StructAPSegFs = ProcessorState->ContextFrame.SegFs;
    APStub.StructAPTr = ProcessorState->SpecialRegisters.Tr;
    APStub.StructAPEip = ProcessorState->ContextFrame.Eip;
    APStub.StructAPEsp = ProcessorState->ContextFrame.Esp;
    APStub.StructAPEcx = (ULONG_PTR)LoaderBlock;
    RtlCopyMemory(APProcessorStateLoc, &APStub, sizeof(APSTUB));
}
#endif
