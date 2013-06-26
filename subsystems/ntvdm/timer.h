/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            timer.h
 * PURPOSE:         Programmable Interval Timer emulation (header file)
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _TIMER_H_
#define _TIMER_H_

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

/* DEFINES ********************************************************************/

#define PIT_CHANNELS 3
#define PIT_BASE_FREQUENCY 1193182LL
#define PIT_DATA_PORT(x) (0x40 + (x))
#define PIT_COMMAND_PORT 0x43

enum
{
    PIT_MODE_INT_ON_TERMINAL_COUNT,
    PIT_MODE_HARDWARE_ONE_SHOT,
    PIT_MODE_RATE_GENERATOR,
    PIT_MODE_SQUARE_WAVE,
    PIT_MODE_SOFTWARE_STROBE,
    PIT_MODE_HARDWARE_STROBE
};

typedef struct _PIT_CHANNEL
{
    WORD ReloadValue;
    WORD CurrentValue;
    WORD LatchedValue;
    INT Mode;
    BOOLEAN Pulsed;
    BOOLEAN LatchSet;
    BOOLEAN InputFlipFlop;
    BOOLEAN OutputFlipFlop;
    BYTE AccessMode;
} PIT_CHANNEL, *PPIT_CHANNEL;

/* FUNCTIONS ******************************************************************/

VOID PitWriteCommand(BYTE Value);
BYTE PitReadData(BYTE Channel);
VOID PitWriteData(BYTE Channel, BYTE Value);
VOID PitDecrementCount();

#endif

/* EOF */

