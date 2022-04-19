/*
 * PROJECT:     Original Xbox onboard hardware
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     SMSC LPC47M157 (Super I/O) header file
 * COPYRIGHT:   Copyright 2020 Stanislav Motylkov (x86corez@gmail.com)
 */

#ifndef _SUPERIO_H_
#define _SUPERIO_H_

#pragma once

/*
 * Registers and definitions
 */
#define LPC_IO_BASE             0x2E

#define LPC_ENTER_CONFIG_KEY    0x55
#define LPC_EXIT_CONFIG_KEY     0xAA

#define LPC_DEVICE_FDD              0x0
#define LPC_DEVICE_PARALLEL_PORT    0x3
#define LPC_DEVICE_SERIAL_PORT_1    0x4
#define LPC_DEVICE_SERIAL_PORT_2    0x5
#define LPC_DEVICE_KEYBOARD         0x7
#define LPC_DEVICE_GAME_PORT        0x9
#define LPC_DEVICE_PME              0xA
#define LPC_DEVICE_MPU_401          0xB

#define LPC_CONFIG_DEVICE_NUMBER                0x07
#define LPC_CONFIG_DEVICE_ACTIVATE              0x30
#define LPC_CONFIG_DEVICE_BASE_ADDRESS_HIGH     0x60
#define LPC_CONFIG_DEVICE_BASE_ADDRESS_LOW      0x61
#define LPC_CONFIG_DEVICE_INTERRUPT_PRIMARY     0x70
#define LPC_CONFIG_DEVICE_INTERRUPT_SECONDARY   0x72
#define LPC_CONFIG_DEVICE_DMA_CHANNEL           0x74

/*
 * Functions
 */
FORCEINLINE
VOID
LpcEnterConfig(VOID)
{
    /* Enter Configuration */
    WRITE_PORT_UCHAR((PUCHAR)LPC_IO_BASE, LPC_ENTER_CONFIG_KEY);
}

FORCEINLINE
VOID
LpcExitConfig(VOID)
{
    /* Exit Configuration */
    WRITE_PORT_UCHAR((PUCHAR)LPC_IO_BASE, LPC_EXIT_CONFIG_KEY);
}

FORCEINLINE
UCHAR
LpcReadRegister(UCHAR Register)
{
    WRITE_PORT_UCHAR((PUCHAR)LPC_IO_BASE, Register);
    return READ_PORT_UCHAR((PUCHAR)(LPC_IO_BASE + 1));
}

FORCEINLINE
VOID
LpcWriteRegister(UCHAR Register, UCHAR Value)
{
    WRITE_PORT_UCHAR((PUCHAR)LPC_IO_BASE, Register);
    WRITE_PORT_UCHAR((PUCHAR)(LPC_IO_BASE + 1), Value);
}

#ifndef _BLDR_
FORCEINLINE
ULONG
LpcDetectSuperIO(VOID)
{
    LpcEnterConfig();

    LpcWriteRegister(LPC_CONFIG_DEVICE_NUMBER, LPC_DEVICE_SERIAL_PORT_1);

    if (READ_PORT_UCHAR((PUCHAR)(LPC_IO_BASE + 1)) != LPC_DEVICE_SERIAL_PORT_1)
        return 0;

    if (LpcReadRegister(LPC_CONFIG_DEVICE_ACTIVATE) > 1)
        return 0;

    LpcExitConfig();

    return LPC_IO_BASE;
}
#endif

FORCEINLINE
ULONG
LpcGetIoBase()
{
    ULONG Base = 0;

    // Read LSB
    Base = LpcReadRegister(LPC_CONFIG_DEVICE_BASE_ADDRESS_LOW);
    // Read MSB
    Base |= (LpcReadRegister(LPC_CONFIG_DEVICE_BASE_ADDRESS_HIGH) << 8);

    return Base;
}

#ifndef _BLDR_
FORCEINLINE
ULONG
LpcGetIoBaseMPU()
{
    ULONG Base = 0;

    // Read LSB
    Base = LpcReadRegister(LPC_CONFIG_DEVICE_BASE_ADDRESS_HIGH);
    // Read MSB
    Base |= (LpcReadRegister(LPC_CONFIG_DEVICE_BASE_ADDRESS_LOW) << 8);

    return Base;
}
#endif

FORCEINLINE
ULONG
LpcGetIrqPrimary()
{
    return LpcReadRegister(LPC_CONFIG_DEVICE_INTERRUPT_PRIMARY);
}

#ifndef _BLDR_
FORCEINLINE
ULONG
LpcGetIrqSecondary()
{
    return LpcReadRegister(LPC_CONFIG_DEVICE_INTERRUPT_SECONDARY);
}

FORCEINLINE
ULONG
LpcGetDmaChannel()
{
    return LpcReadRegister(LPC_CONFIG_DEVICE_DMA_CHANNEL);
}
#endif

#endif /* _SUPERIO_H_ */
