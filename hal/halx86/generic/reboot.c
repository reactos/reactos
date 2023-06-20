/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/generic/reboot.c
 * PURPOSE:         Reboot functions
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include <hal.h>

/* PRIVATE FUNCTIONS *********************************************************/

static VOID
HalpWriteResetCommand(VOID)
{
    /* Generate RESET signal via keyboard controller */
    WRITE_PORT_UCHAR((PUCHAR)0x64, 0xFE);
};

DECLSPEC_NORETURN
VOID
HalpReboot(VOID)
{
    PHYSICAL_ADDRESS PhysicalAddress;
    UCHAR Data;
    PVOID ZeroPageMapping;

    /* Map the first physical page */
    PhysicalAddress.QuadPart = 0;
    ZeroPageMapping = HalpMapPhysicalMemory64(PhysicalAddress, 1);

    /* Enable warm reboot */
    ((PUSHORT)ZeroPageMapping)[0x239] = 0x1234;

    /* Lock CMOS Access (and disable interrupts) */
    HalpAcquireCmosSpinLock();

    /* Setup control register B */
    WRITE_PORT_UCHAR((PUCHAR)0x70, 0x0B);
    KeStallExecutionProcessor(1);

    /* Read periodic register and clear the interrupt enable */
    Data = READ_PORT_UCHAR((PUCHAR)0x71);
    WRITE_PORT_UCHAR((PUCHAR)0x71, Data & ~0x40);
    KeStallExecutionProcessor(1);

    /* Setup control register A */
    WRITE_PORT_UCHAR((PUCHAR)0x70, 0x0A);
    KeStallExecutionProcessor(1);

    /* Read divider rate and reset it */
    Data = READ_PORT_UCHAR((PUCHAR)0x71);
    WRITE_PORT_UCHAR((PUCHAR)0x71, (Data & ~0x9) | 0x06);
    KeStallExecutionProcessor(1);

    /* Reset neutral CMOS address */
    WRITE_PORT_UCHAR((PUCHAR)0x70, 0x15);
    KeStallExecutionProcessor(1);

    /* Flush write buffers and send the reset command */
    KeFlushWriteBuffer();
    HalpWriteResetCommand();

    /* Halt the CPU */
    __halt();
    UNREACHABLE;
}

/* PUBLIC FUNCTIONS **********************************************************/

#ifndef _MINIHAL_
/*
 * @implemented
 */
VOID
NTAPI
HalReturnToFirmware(
    _In_ FIRMWARE_REENTRY Action)
{
    /* Check what kind of action this is */
    switch (Action)
    {
        /* All recognized actions */
        case HalHaltRoutine:
        case HalPowerDownRoutine:
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
