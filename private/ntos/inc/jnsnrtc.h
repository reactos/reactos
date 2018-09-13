/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1992  Digital Equipment Corporation

Module Name:

    jnsnrtc.h

Abstract:

    This module is the header file that describes hardware structure
    for the Toy clock in the 82C106 combo chip on Jensen

Author:

    David N. Cutler (davec) 3-May-1991
    Jeff McLeman  (mcleman) 4-Jun-1992

Revision History:

    13-Jul-1992 Jeff McLeman
     Add port offsets for use with VTI access routines.

    4-Jun-1992  Jeff McLeman
     Adapt module to Jensen specific

--*/

#ifndef _JNSNRTC_
#define _JNSNRTC_

//
// Define Realtime Clock register numbers.
//

#define RTC_SECOND 0                    // second of minute [0..59]
#define RTC_SECOND_ALARM 1              // seconds to alarm
#define RTC_MINUTE 2                    // minute of hour [0..59]
#define RTC_MINUTE_ALARM 3              // minutes to alarm
#define RTC_HOUR 4                      // hour of day [0..23]
#define RTC_HOUR_ALARM 5                // hours to alarm
#define RTC_DAY_OF_WEEK 6               // day of week [1..7]
#define RTC_DAY_OF_MONTH 7              // day of month [1..31]
#define RTC_MONTH 8                     // month of year [1..12]
#define RTC_YEAR 9                      // year [00..99]
#define RTC_CONTROL_REGISTERA 10        // control register A
#define RTC_CONTROL_REGISTERB 11        // control register B
#define RTC_CONTROL_REGISTERC 12        // control register C
#define RTC_CONTROL_REGISTERD 13        // control register D

//
// The RTC NVRAM byte addresses are 0x0e -- 0x4f.
//
// Addresses 0x0E -- 0x3F are reserved for use by VMS/OSF.
//

#define RTC_RAM_NT_FLAGS0 0x40          // NT firmware flag set #0
#define RTC_RAM_CONSOLE_SELECTION 0x4f  // VMS/OSF/NT boot selection


//
// Define port registers for access with the VTI access
// routines
//

#define RTC_APORT     0x170
#define RTC_DPORT     0x171


#ifndef _LANGUAGE_ASSEMBLY

//
// Define Control Register A structure.
//

typedef struct _RTC_CONTROL_REGISTER_A {
    UCHAR RateSelect : 4;
    UCHAR TimebaseDivisor : 3;
    UCHAR UpdateInProgress : 1;
} RTC_CONTROL_REGISTER_A, *PRTC_CONTROL_REGISTER_A;

//
// Define Control Register B structure.
//

typedef struct _RTC_CONTROL_REGISTER_B {
    UCHAR DayLightSavingsEnable : 1;
    UCHAR HoursFormat : 1;
    UCHAR DataMode : 1;
    UCHAR SquareWaveEnable : 1;
    UCHAR UpdateInterruptEnable : 1;
    UCHAR AlarmInterruptEnable : 1;
    UCHAR TimerInterruptEnable : 1;
    UCHAR SetTime : 1;
} RTC_CONTROL_REGISTER_B, *PRTC_CONTROL_REGISTER_B;

//
// Define Control Register C structure.
//

typedef struct _RTC_CONTROL_REGISTER_C {
    UCHAR Fill : 4;
    UCHAR UpdateInterruptFlag : 1;
    UCHAR AlarmInterruptFlag : 1;
    UCHAR TimeInterruptFlag : 1;
    UCHAR InterruptRequest : 1;
} RTC_CONTROL_REGISTER_C, *PRTC_CONTROL_REGISTER_C;

//
// Define Control Register D structure.
//

typedef struct _RTC_CONTROL_REGISTER_D {
    UCHAR Fill : 7;
    UCHAR ValidTime : 1;
} RTC_CONTROL_REGISTER_D, *PRTC_CONTROL_REGISTER_D;

//
// Definitions for NT area of NVRAM
//

typedef struct _RTC_RAM_NT_FLAGS_0 {
    UCHAR Fill : 7;
    UCHAR AutoRunECU : 1;
} RTC_RAM_NT_FLAGS_0, *PRTC_RAM_NT_FLAGS_0;


#endif //_LANGUAGE_ASSEMBLY

// Values for RTC_RAM_CONSOLE_SELECTION
#define RTC_RAM_CONSOLE_SELECTION_NT    1
#define RTC_RAM_CONSOLE_SELECTION_VMS   2
#define RTC_RAM_CONSOLE_SELECTION_OSF   3

//
// Define initialization values for Jensen interval timer
// There are four different rates that are used under NT
// (see page 9-8 of KN121 System Module Programmer's Reference)
//
//   .976562 ms
//  1.953125 ms
//  3.90625  ms
//  7.8125   ms
//
#define RTC_RATE_SELECT1    6
#define RTC_RATE_SELECT2    7
#define RTC_RATE_SELECT3    8
#define RTC_RATE_SELECT4    9

//
// note that rates 1-3 have some rounding error,
// since they are not expressible in even 100ns units
//

#define RTC_PERIOD_IN_CLUNKS1   9766
#define RTC_PERIOD_IN_CLUNKS2  19531
#define RTC_PERIOD_IN_CLUNKS3  39063
#define RTC_PERIOD_IN_CLUNKS4  78125

//
// Defaults
//
#define MINIMUM_INCREMENT RTC_PERIOD_IN_CLUNKS1
#define MAXIMUM_INCREMENT RTC_PERIOD_IN_CLUNKS4
#define MAXIMUM_RATE_SELECT RTC_RATE_SELECT4

#endif // _JNSNRTC_
