/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/hardware/pit.h
 * PURPOSE:         Programmable Interval Timer emulation -
 *                  i82C54/8254 compatible
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _PIT_H_
#define _PIT_H_

/* DEFINES ********************************************************************/

#define PIT_CHANNELS 3
#define PIT_BASE_FREQUENCY 1193182LL
#define PIT_DATA_PORT(x) (0x40 + (x))
#define PIT_COMMAND_PORT 0x43

#define WRITE_PIT_VALUE(PitChannel, Value)    \
    (PitChannel).Bcd ? BCD_TO_BINARY(Value) : (Value)

#define READ_PIT_VALUE(PitChannel, Value)     \
    (PitChannel).Bcd ? BINARY_TO_BCD(Value) : (Value)

typedef enum _PIT_MODE
{
    PIT_MODE_INT_ON_TERMINAL_COUNT,
    PIT_MODE_HARDWARE_ONE_SHOT,
    PIT_MODE_RATE_GENERATOR,
    PIT_MODE_SQUARE_WAVE,
    PIT_MODE_SOFTWARE_STROBE,
    PIT_MODE_HARDWARE_STROBE
} PIT_MODE, *PPIT_MODE;

typedef VOID (WINAPI *PIT_OUT_FUNCTION)(LPVOID Param, BOOLEAN State);

typedef struct _PIT_CHANNEL
{
    /* PIT Status fields */
    PIT_MODE Mode;
    BOOLEAN  Bcd;
    BYTE     ReadWriteMode; // 0 --> Counter Latch ; 1 --> LSB R/W ; 2 --> MSB R/W ; 3 --> LSB then MSB R/W

    /* For interleaved reading and writing in 2-byte RW mode */
    BYTE    ReadStatus;     // Same convention as ReadWriteMode
    BYTE    WriteStatus;    // Same convention as ReadWriteMode

    /* For reading the PIT status byte */
    BOOLEAN LatchStatusSet;
    BYTE    StatusLatch;

    /* Counting */
    BOOLEAN Gate;

    /**/WORD    CountRegister;/**/  // Our ReloadValue ???
    WORD OutputLatch;
    /*******************************/

    WORD    ReloadValue;    // Max value of the counter
    WORD    CurrentValue;   // Real value of the counter

    /* PIT Output */
    BOOLEAN Out;    // 0: Low ; 1: High
    /** HACK!! **/BOOLEAN FlipFlop;/** HACK!! **/
    LPVOID           OutParam;
    PIT_OUT_FUNCTION OutFunction;

} PIT_CHANNEL, *PPIT_CHANNEL;

/* FUNCTIONS ******************************************************************/

VOID PitSetOutFunction(BYTE Channel, LPVOID Param, PIT_OUT_FUNCTION OutFunction);
VOID PitSetGate(BYTE Channel, BOOLEAN State);
WORD PitGetReloadValue(BYTE Channel);

VOID PitInitialize(VOID);

#endif // _PIT_H_

/* EOF */
