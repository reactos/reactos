/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * FILE:        hal/halx86/smp/mp.c
 * PURPOSE:     Source file for private memory functions specific to SMP
 * PROGRAMMERS:  Copyright 2021 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#include <smp.h>
#include "smpp.h"
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* AP spinup stub specific */
#if 0
extern PVOID APEntry;
extern PVOID APEntryEnd;
extern PVOID APSpinup;
extern PVOID APSpinupEnd;
#endif

/* FUNCTIONS *****************************************************************/

VOID
HalpInitializeAPStub(PVOID APStubLocation)
{
    #if 0
    PVOID APStubSecondPhaseLoc;

    /* Copy over the bootstub for specific AP */
    APStubSecondPhaseLoc = (PVOID)((ULONG_PTR)APStubLocation + ((ULONG_PTR)&APEntryEnd  - (ULONG_PTR)&APEntry));
    RtlCopyMemory(APStubLocation, &APEntry,  ((ULONG_PTR)&APEntryEnd - (ULONG_PTR)&APEntry));
    RtlCopyMemory(APStubSecondPhaseLoc, &APSpinup,  ((ULONG_PTR)&APSpinupEnd - (ULONG_PTR)&APSpinup));
    #endif
}
