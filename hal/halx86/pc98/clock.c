/*
 * PROJECT:     NEC PC-98 series HAL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     PIT rollover tables
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>

/* GLOBALS *******************************************************************/

#define LPT_STATUS         0x42
#define SYSTEM_CLOCK_8_MHZ 0x20

ULONG PIT_FREQUENCY;
HALP_ROLLOVER HalpRolloverTable[15];

/* Accuracy is 97.5% */
static const HALP_ROLLOVER RolloverTable1[15] =
{
    {1996, 9996}, /* 1 ms */
    {3993, 19997},
    {5990, 29998},
    {7987, 39999},
    {9984, 50000},
    {11980, 59996},
    {13977, 69997},
    {15974, 79998},
    {17971, 89999},
    {19968, 100000},
    {21964, 109996},
    {23961, 119997},
    {25958, 129998},
    {27955, 139999},
    {29951, 149995} /* 15 ms */
};
static const HALP_ROLLOVER RolloverTable2[15] =
{
    {2463, 10022}, /* 1 ms */
    {4912, 19987},
    {7376, 30013},
    {9825, 39978},
    {12288, 50000},
    {14751, 60022},
    {17200, 69987},
    {19664, 80013},
    {22113, 89978},
    {24576, 100000},
    {27039, 110022},
    {29488, 119987},
    {31952, 130013},
    {34401, 139978},
    {36864, 150000} /* 15 ms */
};

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
HalpInitializeClockPc98(VOID)
{
    /* Obtain system clock value from the parallel port */
    if (__inbyte(LPT_STATUS) & SYSTEM_CLOCK_8_MHZ)
    {
        PIT_FREQUENCY = TIMER_FREQUENCY_1;
        RtlCopyMemory(HalpRolloverTable, RolloverTable1, sizeof(HALP_ROLLOVER) * 15);
    }
    else
    {
        PIT_FREQUENCY = TIMER_FREQUENCY_2;
        RtlCopyMemory(HalpRolloverTable, RolloverTable2, sizeof(HALP_ROLLOVER) * 15);
    }
}
