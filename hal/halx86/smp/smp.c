/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Core source file for SMP management
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

/* FUNCTIONS *****************************************************************/

VOID
HalpSetupProcessorsTable(
    _In_ UINT32 NTProcessorNumber)
{
    PKPRCB CurrentPrcb;

    /*
     * Link the Prcb of the current CPU to
     * the current CPUs entry in the global ProcessorIdentity
     */
    CurrentPrcb = KeGetCurrentPrcb();
    HalpProcessorIdentity[NTProcessorNumber].ProcessorPrcb = CurrentPrcb;
}
