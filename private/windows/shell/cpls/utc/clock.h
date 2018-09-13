/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    clock.h

Abstract:

    This module contains the information for the clock in the Date/Time
    applet.

Revision History:

--*/



//
//  Messages.
//

//
//  CLM_UPDATETIME
//
//  wParam: CLF_GETTIME or CLF_SETTIME (senders perspective)
//  lParam: pointer to LPSYSTEMTIME called to force reflection of new time
//  return: not used
//
#define CLF_GETTIME          0
#define CLF_SETTIME          1
#define CLM_UPDATETIME       (WM_USER + 102)


//
//  CLM_TIMEHWND
//
//  wParam: CLF_GETPROVIDERHWND or CLF_SETPROVIDERHWND
//  lParam: HWND of timer window
//  return: not used
//
#define CLF_GETHWND          0
#define CLF_SETHWND          1
#define CLM_TIMEHWND         (WM_USER + 103)




//
//  Function Prototypes.
//

BOOL
ClockInit(
    HINSTANCE hInstance);

BOOL
CalendarInit(
    HINSTANCE hInstance);
