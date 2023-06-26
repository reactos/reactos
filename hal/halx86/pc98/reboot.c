/*
 * PROJECT:     NEC PC-98 series HAL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Reboot routine
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>

/* PRIVATE FUNCTIONS *********************************************************/

#ifndef _MINIHAL_
static DECLSPEC_NORETURN
VOID
HalpFreezeSystem(VOID)
{
    /* Disable interrupts and halt the CPU */
    _disable();
    __halt();
    UNREACHABLE;
}
#endif

DECLSPEC_NORETURN
VOID
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
    UNREACHABLE;
}

/* PUBLIC FUNCTIONS **********************************************************/

#ifndef _MINIHAL_
VOID
NTAPI
HalReturnToFirmware(
    _In_ FIRMWARE_REENTRY Action)
{
    switch (Action)
    {
        /* All recognized actions */
        case HalHaltRoutine:
        case HalPowerDownRoutine:
            HalpFreezeSystem();

        case HalRestartRoutine:
        case HalRebootRoutine:
        {
            /* Acquire the display */
            InbvAcquireDisplayOwnership();

            /* Call the internal reboot function */
            HalpReboot();
        }

        /* Anything else */
        default:
        {
            /* Print message and break */
            DbgPrint("HalReturnToFirmware called!\n");
            DbgBreakPoint();
        }
    }
}
#endif // _MINIHAL_

/* EOF */
