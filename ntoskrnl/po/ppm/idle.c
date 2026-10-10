/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Processor Power Management idle processor handling support
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/

VOID
FASTCALL
PpmIdle(
    _In_ PPROCESSOR_POWER_STATE PowerState)
{
    /*
     * FIXME: Halting the processor the legacy way through HAL must be done
     * ONLY in uniprocessor systems. This routine must check if performance
     * throttling is supported and adjust the P states accordingly. The target
     * processor control region is either promoted or demoted to either of the
     * C states depending on how long it was idling, as determined by checking
     * if the kernel time exceeded the time limit.
     */
    HalProcessorIdle();
}

/* EOF */
