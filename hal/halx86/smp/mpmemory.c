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

/* AP spinup stub specific */
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
}
