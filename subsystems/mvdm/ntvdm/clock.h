/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/clock.h
 * PURPOSE:         Clock for VDM
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _CLOCK_H_
#define _CLOCK_H_

/* DEFINITIONS ****************************************************************/

#define HARDWARE_TIMER_ENABLED (1 << 0)
#define HARDWARE_TIMER_ONESHOT (1 << 1)
#define HARDWARE_TIMER_PRECISE (1 << 2)

/* Converts a frequency in Hertz into a duration in nanoseconds */
#define HZ_TO_NS(Freq) (ULONGLONG)(1000000000ULL / (ULONGLONG)(Freq))

typedef VOID (FASTCALL *PHARDWARE_TIMER_PROC)(ULONGLONG ElapsedTime);

typedef struct _HARDWARE_TIMER
{
    LIST_ENTRY Link;
    ULONG Flags;
    LONG EnableCount;
    ULONGLONG Delay;
    LARGE_INTEGER LastTick;
    PHARDWARE_TIMER_PROC Callback;
} HARDWARE_TIMER, *PHARDWARE_TIMER;

/* FUNCTIONS ******************************************************************/

PHARDWARE_TIMER CreateHardwareTimer
(
    ULONG Flags,
    ULONGLONG Delay, /* nanoseconds */
    PHARDWARE_TIMER_PROC Callback
);
VOID EnableHardwareTimer(PHARDWARE_TIMER Timer);
VOID DisableHardwareTimer(PHARDWARE_TIMER Timer);
VOID SetHardwareTimerDelay(PHARDWARE_TIMER Timer, ULONGLONG NewDelay);
VOID DestroyHardwareTimer(PHARDWARE_TIMER Timer);

ULONGLONG GetCycleCount(VOID);
ULONGLONG GetCycleSpeed(VOID);

VOID ClockUpdate(VOID);
BOOLEAN ClockInitialize(VOID);

#endif /* _CLOCK_H_ */
