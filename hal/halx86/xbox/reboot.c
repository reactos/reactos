/*
 * PROJECT:         Xbox HAL
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Xbox reboot functions
 * COPYRIGHT:       Copyright 2004 Lehner Franz (franz@caos.at)
 *                  Copyright 2019 Stanislav Motylkov (x86corez@gmail.com)
 *
 * REFERENCES:      https://xboxdevwiki.net/SMBus
 *                  https://github.com/XboxDev/cromwell/blob/master/drivers/pci/i2cio.c
 *                  https://github.com/torvalds/linux/blob/master/drivers/i2c/busses/i2c-amd756.c
 *                  https://github.com/xqemu/xqemu/blob/master/hw/xbox/smbus_xbox_smc.c
 */

/* INCLUDES ******************************************************************/

#include "halxbox.h"

#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
SMBusWriteByte(UCHAR Address, UCHAR Register, UCHAR Data)
{
    INT Retries = 50;

    /* Wait while bus is busy with any master traffic */
    while (READ_PORT_USHORT((PUSHORT)SMB_GLOBAL_STATUS) & 0x800)
    {
        NOTHING;
    }

    while (Retries--)
    {
        UCHAR b;

        WRITE_PORT_UCHAR((PUCHAR)SMB_HOST_ADDRESS, Address << 1);
        WRITE_PORT_UCHAR((PUCHAR)SMB_HOST_COMMAND, Register);

        WRITE_PORT_UCHAR((PUCHAR)SMB_HOST_DATA, Data);

        /* Clear down all preexisting errors */
        WRITE_PORT_USHORT((PUSHORT)SMB_GLOBAL_STATUS, READ_PORT_USHORT((PUSHORT)SMB_GLOBAL_STATUS));

        /* Let I2C SMBus know we're sending a single byte here */
        WRITE_PORT_UCHAR((PUCHAR)SMB_GLOBAL_ENABLE, 0x1A);

        b = 0;

        while (!(b & 0x36))
        {
            b = READ_PORT_UCHAR((PUCHAR)SMB_GLOBAL_STATUS);
        }

        if (b & 0x10)
        {
            return;
        }

        KeStallExecutionProcessor(1);
    }
}

VOID
DECLSPEC_NORETURN
NTAPI
HalpXboxPowerAction(IN UCHAR Action)
{
    SMBusWriteByte(SMB_DEVICE_SMC_PIC16LC, SMC_REG_POWER, Action);

    /* Halt the CPU */
    __halt();

    while (TRUE); /* 'noreturn' function */
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
        case HalPowerDownRoutine:
        {
            /* Call the internal power function */
            HalpXboxPowerAction(SMC_REG_POWER_SHUTDOWN);
        }
        case HalRestartRoutine:
        {
            HalpXboxPowerAction(SMC_REG_POWER_CYCLE);
        }
        case HalRebootRoutine:
        {
            HalpXboxPowerAction(SMC_REG_POWER_RESET);
        }
        /* Anything else */
        default:
        {
            /* Print message and break */
            DbgPrint("HalReturnToFirmware(%d) called!\n", Action);
            DbgBreakPoint();
        }
    }
}

/* EOF */
