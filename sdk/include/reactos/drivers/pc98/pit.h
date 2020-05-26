/*
 * PROJECT:     NEC PC-98 series on-board hardware
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Intel 8253A PIT header file
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

#pragma once

#define TIMER_CHANNEL0_DATA_PORT    0x71
#define TIMER_CHANNEL1_DATA_PORT    0x73
#define TIMER_CHANNEL2_DATA_PORT    0x75
#define TIMER_CONTROL_PORT          0x77

/* Tick rate of PIT depends on system clock frequency */
#define TIMER_FREQUENCY_1    1996800 /* 8 MHz */
#define TIMER_FREQUENCY_2    2457600 /* 10 MHz, 5 MHz */

typedef enum _TIMER_OPERATING_MODES
{
    /* Interrupt On Terminal Count */
    PitOperatingMode0,

    /* Hardware Re-triggerable One-Shot */
    PitOperatingMode1,

    /* Rate Generator */
    PitOperatingMode2,

    /* Square Wave Generator */
    PitOperatingMode3,

    /* Software Triggered Strobe */
    PitOperatingMode4,

    /* Hardware Triggered Strobe */
    PitOperatingMode5
} TIMER_OPERATING_MODES;

typedef enum _TIMER_ACCESS_MODES
{
    PitAccessModeCounterLatch,
    PitAccessModeLow,
    PitAccessModeHigh,
    PitAccessModeLowHigh
} TIMER_ACCESS_MODES;

typedef enum _TIMER_CHANNELS
{
    /* IRQ 0 */
    PitChannel0,

    /* PC Speaker */
    PitChannel1,

    /* RS-232 chipset */
    PitChannel2,

    /* Execute multiple latch command */
    MultipleLatch
} TIMER_CHANNELS;

typedef union _TIMER_CONTROL_PORT_REGISTER
{
    struct
    {
        UCHAR BcdMode:1;
        UCHAR OperatingMode:3;
        UCHAR AccessMode:2;
        UCHAR Channel:2;
    };
    UCHAR Bits;
} TIMER_CONTROL_PORT_REGISTER, *PTIMER_CONTROL_PORT_REGISTER;

FORCEINLINE
ULONG
Read8253Timer(TIMER_CHANNELS TimerChannel)
{
    ULONG Count;

    WRITE_PORT_UCHAR((PUCHAR)TIMER_CONTROL_PORT, (TimerChannel << 6) | PitAccessModeCounterLatch);
    Count = READ_PORT_UCHAR((PUCHAR)(TIMER_CHANNEL0_DATA_PORT + TimerChannel * 2));
    Count |= READ_PORT_UCHAR((PUCHAR)(TIMER_CHANNEL0_DATA_PORT + TimerChannel * 2)) << 8;

    return Count;
}

FORCEINLINE
VOID
Write8253Timer(
    TIMER_CONTROL_PORT_REGISTER TimerControl,
    USHORT Count)
{
    WRITE_PORT_UCHAR((PUCHAR)TIMER_CONTROL_PORT, TimerControl.Bits);
    WRITE_PORT_UCHAR((PUCHAR)(TIMER_CHANNEL0_DATA_PORT + TimerControl.Channel * 2), Count & 0xFF);
    WRITE_PORT_UCHAR((PUCHAR)(TIMER_CHANNEL0_DATA_PORT + TimerControl.Channel * 2), (Count >> 8) & 0xFF);
}
