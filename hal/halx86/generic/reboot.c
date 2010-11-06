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
#define NDEBUG
#include <debug.h>

#define GetPteAddress(x) (PHARDWARE_PTE)(((((ULONG_PTR)(x)) >> 12) << 2) + 0xC0000000)

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
HalpWriteResetCommand(VOID)
{
    /* Generate RESET signal via keyboard controller */
    WRITE_PORT_UCHAR((PUCHAR)0x64, 0xFE);
};

VOID
NTAPI
HalpReboot(VOID)
{
    UCHAR Data;
    PVOID ZeroPageMapping;
    PHARDWARE_PTE Pte;

    /* Get a PTE in the HAL reserved region */
    ZeroPageMapping = (PVOID)(0xFFC00000 + PAGE_SIZE);
    Pte = GetPteAddress(ZeroPageMapping);

    /* Make it valid and map it to the first physical page */
    Pte->Valid = 1;
    Pte->Write = 1;
    Pte->Owner = 1;
    Pte->PageFrameNumber = 0;

    /* Flush the TLB by resetting CR3 */
    __writecr3(__readcr3());

    /* Enable warm reboot */
    ((PUSHORT)ZeroPageMapping)[0x239] = 0x1234;

    /* Lock CMOS Access (and disable interrupts) */
    HalpAcquireSystemHardwareSpinLock();

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
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
VOID
NTAPI
HalReturnToFirmware(IN FIRMWARE_REENTRY Action)
{
    /* Check what kind of action this is */
    switch (Action)
    {
        /* All recognized actions */
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

/* EOF */
