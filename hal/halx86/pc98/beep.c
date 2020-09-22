/*
 * PROJECT:     NEC PC-98 series HAL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Speaker support (beeping)
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

BOOLEAN
NTAPI
HalMakeBeep(
    _In_ ULONG Frequency)
{
    TIMER_CONTROL_PORT_REGISTER TimerControl;
    ULONG Divider;
    BOOLEAN Success = FALSE;

    HalpAcquireCmosSpinLock();

    __outbyte(PPI_IO_o_CONTROL, PPI_TIMER_1_UNGATE_TO_SPEAKER);

    if (Frequency)
    {
        Divider = PIT_FREQUENCY / Frequency;

        if (Divider <= 0x10000)
        {
            TimerControl.BcdMode = FALSE;
            TimerControl.OperatingMode = PitOperatingMode3;
            TimerControl.Channel = PitChannel1;
            TimerControl.AccessMode = PitAccessModeLowHigh;
            __outbyte(TIMER_CONTROL_PORT, TimerControl.Bits);
            __outbyte(TIMER_CHANNEL1_DATA_PORT, FIRSTBYTE(Divider));
            __outbyte(TIMER_CHANNEL1_DATA_PORT, SECONDBYTE(Divider));

            __outbyte(PPI_IO_o_CONTROL, PPI_TIMER_1_GATE_TO_SPEAKER);

            Success = TRUE;
        }
    }

    HalpReleaseCmosSpinLock();

    return Success;
}
