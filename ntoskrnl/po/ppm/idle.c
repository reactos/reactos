/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Power Processor Management idle processor handling support
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
    /* FIXME */
    UNIMPLEMENTED;
    NOTHING;
}

/* EOF */
