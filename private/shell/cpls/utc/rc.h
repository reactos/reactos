/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    rc.h

Abstract:

    This module contains the resource ids for the Date/Time applet.

Revision History:

--*/



#ifdef RC_INVOKED
  #define MAPCTL_CLASSNAME        "CplWorldMapClass"
  #define CALENDAR_CLASS          "CalWndMain"
  #define CLOCK_CLASS             "ClockWndMain"
#else
  #define MAPCTL_CLASSNAME        TEXT("CplWorldMapClass")
  #define CALENDAR_CLASS          TEXT("CalWndMain")
  #define CLOCK_CLASS             TEXT("ClockWndMain")
#endif

#define DLG_DATETIME              1
#define DLG_TIMEZONE              2
#define DLG_DATETIMEWIDE          10

#define IDB_TIMEZONE              50

#define IDD_AUTOMAGIC             100
#define IDD_TIMEZONES             101
#define IDD_TIMEMAP               102
#define IDD_GROUPBOX1             103
#define IDD_GROUPBOX2             104

#define IDI_TIMEDATE              200

#define IDS_TIMEDATE              300
#define IDS_TIMEDATEINFO          301

#define IDS_WARNAUTOTIMECHANGE    302
#define IDS_WATC_CAPTION          303

#define IDS_CAPTION               304
#define IDS_NOTIMEERROR           305

#define IDS_ISRAELTIMEZONE        306
#define IDS_JERUSALEMTIMEZONE     307


//
//  The Order of HOUR, MINUTE, SECOND, MONTH, DAY, and YEAR
//  are critical.
//
#define DATETIME_STATIC			  -1
#define DATETIME                  700
#define DATETIME_HOUR             701
#define DATETIME_MINUTE           702
#define DATETIME_SECOND           703
#define DATETIME_MONTH            704
#define DATETIME_DAY              705
#define DATETIME_YEAR             706
#define DATETIME_TSEP1            707
#define DATETIME_TSEP2            708

#define DATETIME_TARROW           709
#define DATETIME_AMPM             710
#define DATETIME_CALENDAR         711
#define DATETIME_CLOCK            712
#define DATETIME_MONTHNAME        713

#define DATETIME_YARROW           714
#define DATETIME_TBORDER          715
#define DATETIME_CURTZ            716
