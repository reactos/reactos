/*++ BUILD Version: 0002    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    jazzrtc.h

Abstract:

    This module is the header file that describes hardware structure
    for the realtime clock on the Jazz R3000 and R4000 system.

Author:

    David N. Cutler (davec) 3-May-1991

Revision History:

--*/

#ifndef _JAZZRTC_
#define _JAZZRTC_

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
#define RTC_BATTERY_BACKED_UP_RAM 14    // battery backed up RAM [0..49]

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

#endif // _JAZZRTC_
