/*
 * PROJECT:     NEC PC-98 series HAL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Delay routines
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>

#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
HalpCalibrateStallExecution(VOID)
{
    /* FIXME */
    NOTHING;
}

/* PUBLIC FUNCTIONS **********************************************************/

#ifndef _MINIHAL_
VOID
NTAPI
KeStallExecutionProcessor(
    _In_ ULONG MicroSeconds)
{
    while (MicroSeconds--)
    {
        /* FIXME: Use stall factor */
        WRITE_PORT_UCHAR((PUCHAR)CPU_IO_o_ARTIC_DELAY, 0);
    }
}
#endif
