/*
 * PROJECT:     NEC PC-98 series HAL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Reboot routine
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>

#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

static VOID
DECLSPEC_NORETURN
NTAPI
HalpFreezeSystem(VOID)
{
    HaliHaltSystem();

    while (TRUE)
        NOTHING;
}

VOID
NTAPI
HalpReboot(VOID)
{
    /* Disable interrupts */
    _disable();

    /* Flush write buffers */
    KeFlushWriteBuffer();

    /* Send the reset command */
    WRITE_PORT_UCHAR((PUCHAR)PPI_IO_o_CONTROL, PPI_SHUTDOWN_0_ENABLE);
    WRITE_PORT_UCHAR((PUCHAR)PPI_IO_o_CONTROL, PPI_SHUTDOWN_1_ENABLE);
    WRITE_PORT_UCHAR((PUCHAR)CPU_IO_o_RESET, 0);

    /* Halt the CPU */
    __halt();
}

/* PUBLIC FUNCTIONS **********************************************************/

VOID
NTAPI
HalReturnToFirmware(
    _In_ FIRMWARE_REENTRY Action)
{
    switch (Action)
    {
        case HalPowerDownRoutine:
            HalpFreezeSystem();

        case HalHaltRoutine:
        case HalRebootRoutine:
#ifndef _MINIHAL_
            /* Acquire the display */
            InbvAcquireDisplayOwnership();
#endif

            /* Call the internal reboot function */
            HalpReboot();

        /* Anything else */
        default:
            /* Print message and break */
            DbgPrint("HalReturnToFirmware called!\n");
            DbgBreakPoint();
    }
}
