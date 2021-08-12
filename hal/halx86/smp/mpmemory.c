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
} APSTUB, *PAPSTUB;
#endif

/* AP spinup stub universal */
extern APSTUB APProcessorStateStruct;
extern PVOID APEntry;
extern PVOID APEntryEnd;
extern PVOID APSpinup;
extern PVOID APSpinupEnd;
extern UINT16 APJumpOffset;


/* FUNCTIONS *****************************************************************/

VOID
HalpInitializeAPStub(PVOID APStubLocation)
{
    PVOID APStubSecondPhaseLoc;
    PVOID APJumppLoc;
    /* Get the locations used to copy over */
    APJumppLoc = (PUSHORT)((ULONG_PTR)APStubLocation + (ULONG_PTR)&APJumpOffset - (ULONG_PTR)&APEntry); 
    APStubSecondPhaseLoc = (PVOID)((ULONG_PTR)APStubLocation + ((ULONG_PTR)&APEntryEnd  - (ULONG_PTR)&APEntry));
    APJumpOffset = (UINT16)(((ULONG_PTR)APStubLocation * 4) + ((ULONG_PTR)&APEntryEnd  - (ULONG_PTR)&APEntry));
    /* Copy over the bootstub for specific AP */
    RtlCopyMemory(APStubLocation, &APEntry,  ((ULONG_PTR)&APEntryEnd - (ULONG_PTR)&APEntry));
    RtlCopyMemory(APStubSecondPhaseLoc, &APSpinup,  ((ULONG_PTR)&APSpinupEnd - (ULONG_PTR)&APSpinup));
    RtlCopyMemory(APJumppLoc, &APJumpOffset,  sizeof(APJumpOffset));

    /* ProcessorState */
}

#ifdef _M_AMD64
VOID
HalpWriteProcessorState(PVOID APStubLocation, 
                        PKPROCESSOR_STATE ProcessorState)
{

}
#elif _M_IX86
VOID
HalpWriteProcessorState(PVOID APStubLocation, 
                        PKPROCESSOR_STATE ProcessorState)
{
    PVOID APProcessorStateLoc;
    APSTUB APStub;
    APProcessorStateLoc = (PUSHORT)(((ULONG_PTR)APStubLocation) + ((ULONG_PTR)&APProcessorStateStruct - (ULONG_PTR)&APSpinup) + ((ULONG_PTR)&APEntryEnd  - (ULONG_PTR)&APEntry));
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
    /* Copy over ProcessorState struct */
    RtlCopyMemory(APProcessorStateLoc, &APStub, sizeof(APSTUB));
}
#endif
