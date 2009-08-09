/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/generic/reboot.c
 * PURPOSE:         Reboot functions
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Eric Kohl (ekohl@abo.rhein-zeitung.de)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

static VOID
NTAPI
HalpWriteResetCommand(VOID)
{
    /* Generate RESET signal via keyboard controller */
    WRITE_PORT_UCHAR((PUCHAR)0x64, 0xFE);
};

static VOID
NTAPI
HalpReboot(VOID)
{
    UCHAR Data;
    PVOID HalpZeroPageMapping;
    PHYSICAL_ADDRESS Null = {{0, 0}};

    /* Enable warm reboot */
    HalpZeroPageMapping = MmMapIoSpace(Null, PAGE_SIZE, MmNonCached);
    ((PUSHORT)HalpZeroPageMapping)[0x239] = 0x1234;

    /* FIXME: Lock CMOS Access */

    /* Disable interrupts */
    _disable();

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
    Ke386HaltProcessor();
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
VOID
NTAPI
HalReturnToFirmware(IN FIRMWARE_REENTRY Action)
{
    /* Check the kind of action this is */
    switch (Action)
    {
        /* All recognized actions */
        case HalHaltRoutine:
        case HalRebootRoutine:

            /* Call the internal reboot function */
            HalpReboot();

        /* Anything else */
        default:

            /* Print message and break */
            DbgPrint("HalReturnToFirmware called!\n");
            DbgBreakPoint();
    }
}

/* EOF */
