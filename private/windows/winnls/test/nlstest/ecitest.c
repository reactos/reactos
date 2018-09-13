/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    ecitest.c

Abstract:

    Test module for NLS API EnumCalendarInfo.

    NOTE: This code was simply hacked together quickly in order to
          test the different code modules of the NLS component.
          This is NOT meant to be a formal regression test.

Revision History:

    08-02-93    JulieB    Created.

--*/



//
//  Include Files.
//

#include "nlstest.h"




//
//  Constant Declarations.
//

#define  ECI_INVALID_FLAG       0x00000100




//
//  Global Variables.
//

int CalendarCtr;


CALTYPE pCalFlag[] =
{
    CAL_ICALINTVALUE,
    CAL_SCALNAME,
    CAL_ITWODIGITYEARMAX,
    CAL_IYEAROFFSETRANGE,
    CAL_SERASTRING,
    CAL_SSHORTDATE,
    CAL_SLONGDATE,
    CAL_SYEARMONTH,
    CAL_SDAYNAME1,
    CAL_SMONTHNAME1,
    CAL_SMONTHNAME13
};
#define NUM_CAL_FLAGS  ( sizeof(pCalFlag) / sizeof(CALTYPE) )


//
//  pCalEnglish and pCalJapan_x must have the same number of entries
//  as pCalFlag.
//
int pCalEnglish[] =
{
    1,
    1,
    1,
    0,
    0,
    7,
    4,
    1,
    1,
    1,
    0
};

int pCalJapan_All[] =
{
    3,
    3,
    3,
    4,
    4,
    22,
    14,
    3,
    2,
    2,
    0
};

int pCalJapan_1[] =
{
    1,
    1,
    1,
    0,
    0,
    9,
    4,
    1,
    1,
    1,
    0
};

int pCalJapan_2[] =
{
    1,
    1,
    1,
    0,
    0,
    4,
    2,
    1,
    1,
    1,
    0
};

int pCalJapan_3[] =
{
    1,
    1,
    1,
    4,
    4,
    9,
    8,
    1,
    1,
    1,
    0
};




//
//  Forward Declarations.
//

BOOL
InitEnumCalendarInfo();

int
ECI_BadParamCheck();

int
ECI_NormalCase();

int
ECI_Ansi();

BOOL
CALLBACK
MyFuncCalendar(
    LPWSTR pStr);

BOOL
CALLBACK
MyFuncCalendarA(
    LPSTR pStr);

BOOL
CALLBACK
MyFuncCalendarEx(
    LPWSTR pStr,
    CALID CalId);

BOOL
CALLBACK
MyFuncCalendarExA(
    LPSTR pStr,
    CALID CalId);




//
//  Callback functions.
//

BOOL CALLBACK MyFuncCalendar(
    LPWSTR pStr)
{
    if (Verbose)
    {
        while (*pStr)
        {
            printf((*pStr > 0xff) ? "(0x%x)" : "%wc", *pStr);
            pStr++;
        }
        printf("\n");
    }

    CalendarCtr++;

    return (TRUE);
}


BOOL CALLBACK MyFuncCalendarA(
    LPSTR pStr)
{
    if (Verbose)
    {
        while (*pStr)
        {
            printf((*pStr > 0xff) ? "(0x%x)" : "%c", *pStr);
            pStr++;
        }
        printf("\n");
    }

    CalendarCtr++;

    return (TRUE);
}


//
//  Callback functions for EX version.
//

BOOL CALLBACK MyFuncCalendarEx(
    LPWSTR pStr,
    CALID CalId)
{
    if (Verbose)
    {
        printf("CalId = %d\n", CalId);

        while (*pStr)
        {
            printf((*pStr > 0xff) ? "(0x%x)" : "%wc", *pStr);
            pStr++;
        }
        printf("\n");
    }

    CalendarCtr++;

    return (TRUE);
}


BOOL CALLBACK MyFuncCalendarExA(
    LPSTR pStr,
    CALID CalId)
{
    if (Verbose)
    {
        printf("CalId = %d\n", CalId);

        while (*pStr)
        {
            printf((*pStr > 0xff) ? "(0x%x)" : "%c", *pStr);
            pStr++;
        }
        printf("\n");
    }

    CalendarCtr++;

    return (TRUE);
}





////////////////////////////////////////////////////////////////////////////
//
//  TestEnumCalendarInfo
//
//  Test routine for EnumCalendarInfoW API.
//
//  08-02-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int TestEnumCalendarInfo()
{
    int ErrCount = 0;             // error count


    //
    //  Print out what's being done.
    //
    printf("\n\nTESTING EnumCalendarInfoW...\n\n");

    //
    //  Initialize global variables.
    //
    if (!InitEnumCalendarInfo())
    {
        printf("\nABORTED TestEnumCalendarInfo: Could not Initialize.\n");
        return (1);
    }

    //
    //  Test bad parameters.
    //
    ErrCount += ECI_BadParamCheck();

    //
    //  Test normal cases.
    //
    ErrCount += ECI_NormalCase();

    //
    //  Test Ansi version.
    //
    ErrCount += ECI_Ansi();

    //
    //  Print out result.
    //
    printf("\nEnumCalendarInfoW:  ERRORS = %d\n", ErrCount);

    //
    //  Return total number of errors found.
    //
    return (ErrCount);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitEnumCalendarInfo
//
//  This routine initializes the global variables.  If no errors were
//  encountered, then it returns TRUE.  Otherwise, it returns FALSE.
//
//  08-02-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL InitEnumCalendarInfo()
{
    //
    //  Initialize date counter.
    //
    CalendarCtr = 0;

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  ECI_BadParamCheck
//
//  This routine passes in bad parameters to the API routines and checks to
//  be sure they are handled properly.  The number of errors encountered
//  is returned to the caller.
//
//  08-02-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ECI_BadParamCheck()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


    //
    //  Bad Function.
    //

    //  Variation 1  -  bad function
    CalendarCtr = 0;
    rc = EnumCalendarInfoW( NULL,
                            0x0409,
                            ENUM_ALL_CALENDARS,
                            CAL_ICALINTVALUE );
    CheckReturnBadParamEnum( rc,
                             FALSE,
                             ERROR_INVALID_PARAMETER,
                             "function invalid",
                             &NumErrors,
                             CalendarCtr,
                             0 );

    CalendarCtr = 0;
    rc = EnumCalendarInfoExW( NULL,
                              0x0409,
                              ENUM_ALL_CALENDARS,
                              CAL_ICALINTVALUE );
    CheckReturnBadParamEnum( rc,
                             FALSE,
                             ERROR_INVALID_PARAMETER,
                             "Ex function invalid",
                             &NumErrors,
                             CalendarCtr,
                             0 );


    //
    //  Bad Locale.
    //

    //  Variation 1  -  bad locale
    CalendarCtr = 0;
    rc = EnumCalendarInfoW( MyFuncCalendar,
                            (LCID)333,
                            ENUM_ALL_CALENDARS,
                            CAL_ICALINTVALUE );
    CheckReturnBadParamEnum( rc,
                             FALSE,
                             ERROR_INVALID_PARAMETER,
                             "Locale invalid",
                             &NumErrors,
                             CalendarCtr,
                             0 );

    CalendarCtr = 0;
    rc = EnumCalendarInfoExW( MyFuncCalendarEx,
                              (LCID)333,
                              ENUM_ALL_CALENDARS,
                              CAL_ICALINTVALUE );
    CheckReturnBadParamEnum( rc,
                             FALSE,
                             ERROR_INVALID_PARAMETER,
                             "Ex Locale invalid",
                             &NumErrors,
                             CalendarCtr,
                             0 );


    //
    //  Invalid Flag.
    //

    //  Variation 1  -  dwFlags = invalid
    CalendarCtr = 0;
    rc = EnumCalendarInfoW( MyFuncCalendar,
                            0x0409,
                            ENUM_ALL_CALENDARS,
                            ECI_INVALID_FLAG );
    CheckReturnBadParamEnum( rc,
                             FALSE,
                             ERROR_INVALID_FLAGS,
                             "Flag invalid",
                             &NumErrors,
                             CalendarCtr,
                             0 );

    CalendarCtr = 0;
    rc = EnumCalendarInfoExW( MyFuncCalendarEx,
                              0x0409,
                              ENUM_ALL_CALENDARS,
                              ECI_INVALID_FLAG );
    CheckReturnBadParamEnum( rc,
                             FALSE,
                             ERROR_INVALID_FLAGS,
                             "Ex Flag invalid",
                             &NumErrors,
                             CalendarCtr,
                             0 );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  ECI_NormalCase
//
//  This routine tests the normal cases of the API routine.
//
//  08-02-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ECI_NormalCase()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code
    int ctr;                      // loop counter


    if (Verbose)
    {
        printf("\n----  W version  ----\n\n");
    }

    //
    //  Single calendar id - English.
    //

    //  Variation 1  -  iCalIntValue - English
    CalendarCtr = 0;
    rc = EnumCalendarInfoW( MyFuncCalendar,
                            0x0409,
                            0,
                            CAL_ICALINTVALUE );
    CheckReturnValidEnum( rc,
                          FALSE,
                          CalendarCtr,
                          0,
                          "iCalIntValue English (cal 0)",
                          &NumErrors );

    CalendarCtr = 0;
    rc = EnumCalendarInfoExW( MyFuncCalendarEx,
                              0x0409,
                              0,
                              CAL_ICALINTVALUE );
    CheckReturnValidEnum( rc,
                          FALSE,
                          CalendarCtr,
                          0,
                          "Ex iCalIntValue English (cal 0)",
                          &NumErrors );

    //  Variation 2  -  iCalIntValue - English
    CalendarCtr = 0;
    rc = EnumCalendarInfoW( MyFuncCalendar,
                            0x0409,
                            2,
                            CAL_ICALINTVALUE );
    CheckReturnValidEnum( rc,
                          FALSE,
                          CalendarCtr,
                          0,
                          "iCalIntValue English (cal 2)",
                          &NumErrors );

    CalendarCtr = 0;
    rc = EnumCalendarInfoExW( MyFuncCalendarEx,
                              0x0409,
                              2,
                              CAL_ICALINTVALUE );
    CheckReturnValidEnum( rc,
                          FALSE,
                          CalendarCtr,
                          0,
                          "Ex iCalIntValue English (cal 2)",
                          &NumErrors );

    //  Variation 3  -  Use CP ACP
    CalendarCtr = 0;
    rc = EnumCalendarInfoW( MyFuncCalendar,
                            0x0409,
                            2,
                            CAL_ICALINTVALUE | LOCALE_USE_CP_ACP );
    CheckReturnValidEnum( rc,
                          FALSE,
                          CalendarCtr,
                          0,
                          "Use CP ACP",
                          &NumErrors );

    CalendarCtr = 0;
    rc = EnumCalendarInfoExW( MyFuncCalendarEx,
                              0x0409,
                              2,
                              CAL_ICALINTVALUE | LOCALE_USE_CP_ACP );
    CheckReturnValidEnum( rc,
                          FALSE,
                          CalendarCtr,
                          0,
                          "Ex Use CP ACP",
                          &NumErrors );


    //
    //  CALTYPE values - English.
    //

    for (ctr = 0; ctr < NUM_CAL_FLAGS; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoW( MyFuncCalendar,
                                0x0409,
                                1,
                                pCalFlag[ctr] );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  pCalEnglish[ctr],
                                  "English (cal 1) Calendar Flag",
                                  pCalFlag[ctr],
                                  &NumErrors );
    }

    for (ctr = 0; ctr < NUM_CAL_FLAGS; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoExW( MyFuncCalendarEx,
                                  0x0409,
                                  1,
                                  pCalFlag[ctr] );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  pCalEnglish[ctr],
                                  "Ex English (cal 1) Calendar Flag",
                                  pCalFlag[ctr],
                                  &NumErrors );
    }



    //
    //  Single calendar id - Japan.
    //

    //  Variation 1  -  iCalIntValue - Japan
    CalendarCtr = 0;
    rc = EnumCalendarInfoW( MyFuncCalendar,
                            0x0411,
                            0,
                            CAL_ICALINTVALUE );
    CheckReturnValidEnum( rc,
                          FALSE,
                          CalendarCtr,
                          0,
                          "iCalIntValue Japan (cal 0)",
                          &NumErrors );

    CalendarCtr = 0;
    rc = EnumCalendarInfoExW( MyFuncCalendarEx,
                              0x0411,
                              0,
                              CAL_ICALINTVALUE );
    CheckReturnValidEnum( rc,
                          FALSE,
                          CalendarCtr,
                          0,
                          "Ex iCalIntValue Japan (cal 0)",
                          &NumErrors );



    //
    //  CALTYPE values - Japan.
    //

    for (ctr = 0; ctr < NUM_CAL_FLAGS; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoW( MyFuncCalendar,
                                0x0411,
                                1,
                                pCalFlag[ctr] );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  pCalJapan_1[ctr],
                                  "Japan (cal 1) Calendar Flag",
                                  pCalFlag[ctr],
                                  &NumErrors );
    }

    for (ctr = 0; ctr < NUM_CAL_FLAGS; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoExW( MyFuncCalendarEx,
                                  0x0411,
                                  1,
                                  pCalFlag[ctr] );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  pCalJapan_1[ctr],
                                  "Ex Japan (cal 1) Calendar Flag",
                                  pCalFlag[ctr],
                                  &NumErrors );
    }

    for (ctr = 0; ctr < NUM_CAL_FLAGS; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoW( MyFuncCalendar,
                                0x0411,
                                2,
                                pCalFlag[ctr] );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  pCalJapan_2[ctr],
                                  "Japan (cal 2) Calendar Flag",
                                  pCalFlag[ctr],
                                  &NumErrors );
    }

    for (ctr = 0; ctr < NUM_CAL_FLAGS; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoExW( MyFuncCalendarEx,
                                  0x0411,
                                  2,
                                  pCalFlag[ctr] );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  pCalJapan_2[ctr],
                                  "Ex Japan (cal 2) Calendar Flag",
                                  pCalFlag[ctr],
                                  &NumErrors );
    }

    for (ctr = 0; ctr < NUM_CAL_FLAGS; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoW( MyFuncCalendar,
                                0x0411,
                                3,
                                pCalFlag[ctr] );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  pCalJapan_3[ctr],
                                  "Japan (cal 3) Calendar Flag",
                                  pCalFlag[ctr],
                                  &NumErrors );
    }

    for (ctr = 0; ctr < NUM_CAL_FLAGS; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoExW( MyFuncCalendarEx,
                                  0x0411,
                                  3,
                                  pCalFlag[ctr] );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  pCalJapan_3[ctr],
                                  "Ex Japan (cal 3) Calendar Flag",
                                  pCalFlag[ctr],
                                  &NumErrors );
    }



    //
    //  English - Enumerate ALL Calendars.
    //

    for (ctr = 0; ctr < NUM_CAL_FLAGS; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoW( MyFuncCalendar,
                                0x0409,
                                ENUM_ALL_CALENDARS,
                                pCalFlag[ctr] );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  pCalEnglish[ctr],
                                  "English (all cal) Calendar Flag",
                                  pCalFlag[ctr],
                                  &NumErrors );
    }

    for (ctr = 0; ctr < NUM_CAL_FLAGS; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoExW( MyFuncCalendarEx,
                                  0x0409,
                                  ENUM_ALL_CALENDARS,
                                  pCalFlag[ctr] );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  pCalEnglish[ctr],
                                  "Ex English (all cal) Calendar Flag",
                                  pCalFlag[ctr],
                                  &NumErrors );
    }

    for (ctr = CAL_SDAYNAME1; ctr <= CAL_SMONTHNAME12; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoW( MyFuncCalendar,
                                0x0409,
                                ENUM_ALL_CALENDARS,
                                ctr );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  1,
                                  "English (all cal) Day/Month Calendar Flag",
                                  ctr,
                                  &NumErrors );
    }

    for (ctr = CAL_SDAYNAME1; ctr <= CAL_SMONTHNAME12; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoExW( MyFuncCalendarEx,
                                  0x0409,
                                  ENUM_ALL_CALENDARS,
                                  ctr );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  1,
                                  "Ex English (all cal) Day/Month Calendar Flag",
                                  ctr,
                                  &NumErrors );
    }

    for (ctr = CAL_SABBREVMONTHNAME1; ctr <= CAL_SABBREVMONTHNAME12; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoW( MyFuncCalendar,
                                0x0409,
                                ENUM_ALL_CALENDARS,
                                ctr );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  1,
                                  "English (all cal) Day/Month Calendar Flag",
                                  ctr,
                                  &NumErrors );
    }

    for (ctr = CAL_SABBREVMONTHNAME1; ctr <= CAL_SABBREVMONTHNAME12; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoExW( MyFuncCalendarEx,
                                  0x0409,
                                  ENUM_ALL_CALENDARS,
                                  ctr );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  1,
                                  "Ex English (all cal) Day/Month Calendar Flag",
                                  ctr,
                                  &NumErrors );
    }



    //
    //  Japan - Enumerate ALL Calendars.
    //

    for (ctr = 0; ctr < NUM_CAL_FLAGS; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoW( MyFuncCalendar,
                                0x0411,
                                ENUM_ALL_CALENDARS,
                                pCalFlag[ctr] );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  pCalJapan_All[ctr],
                                  "Japan (all cal) Calendar Flag",
                                  pCalFlag[ctr],
                                  &NumErrors );
    }

    for (ctr = 0; ctr < NUM_CAL_FLAGS; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoExW( MyFuncCalendarEx,
                                  0x0411,
                                  ENUM_ALL_CALENDARS,
                                  pCalFlag[ctr] );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  pCalJapan_All[ctr],
                                  "Ex Japan (all cal) Calendar Flag",
                                  pCalFlag[ctr],
                                  &NumErrors );
    }

    for (ctr = CAL_SDAYNAME1; ctr <= CAL_SMONTHNAME12; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoW( MyFuncCalendar,
                                0x0411,
                                ENUM_ALL_CALENDARS,
                                ctr );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  2,
                                  "Japan (all cal) Day/Month Calendar Flag",
                                  ctr,
                                  &NumErrors );
    }

    for (ctr = CAL_SDAYNAME1; ctr <= CAL_SMONTHNAME12; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoExW( MyFuncCalendarEx,
                                  0x0411,
                                  ENUM_ALL_CALENDARS,
                                  ctr );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  2,
                                  "Ex Japan (all cal) Day/Month Calendar Flag",
                                  ctr,
                                  &NumErrors );
    }

    for (ctr = CAL_SABBREVMONTHNAME1; ctr <= CAL_SABBREVMONTHNAME12; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoW( MyFuncCalendar,
                                0x0411,
                                ENUM_ALL_CALENDARS,
                                ctr );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  2,
                                  "Japan (all cal) Day/Month Calendar Flag",
                                  ctr,
                                  &NumErrors );
    }

    for (ctr = CAL_SABBREVMONTHNAME1; ctr <= CAL_SABBREVMONTHNAME12; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoExW( MyFuncCalendarEx,
                                  0x0411,
                                  ENUM_ALL_CALENDARS,
                                  ctr );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  2,
                                  "Ex Japan (all cal) Day/Month Calendar Flag",
                                  ctr,
                                  &NumErrors );
    }



    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  ECI_Ansi
//
//  This routine tests the normal cases of the API routine.
//
//  08-02-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ECI_Ansi()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code
    int ctr;                      // loop counter


    if (Verbose)
    {
        printf("\n----  A version  ----\n\n");
    }

    //
    //  Single calendar id - English.
    //

    //  Variation 1  -  iCalIntValue - English
    CalendarCtr = 0;
    rc = EnumCalendarInfoA( MyFuncCalendarA,
                            0x0409,
                            0,
                            CAL_ICALINTVALUE );
    CheckReturnValidEnum( rc,
                          FALSE,
                          CalendarCtr,
                          0,
                          "A version iCalIntValue English (cal 0)",
                          &NumErrors );

    CalendarCtr = 0;
    rc = EnumCalendarInfoExA( MyFuncCalendarExA,
                              0x0409,
                              0,
                              CAL_ICALINTVALUE );
    CheckReturnValidEnum( rc,
                          FALSE,
                          CalendarCtr,
                          0,
                          "Ex A version iCalIntValue English (cal 0)",
                          &NumErrors );

    //  Variation 2  -  iCalIntValue - English
    CalendarCtr = 0;
    rc = EnumCalendarInfoA( MyFuncCalendarA,
                            0x0409,
                            2,
                            CAL_ICALINTVALUE );
    CheckReturnValidEnum( rc,
                          FALSE,
                          CalendarCtr,
                          0,
                          "A version iCalIntValue English (cal 2)",
                          &NumErrors );

    CalendarCtr = 0;
    rc = EnumCalendarInfoExA( MyFuncCalendarExA,
                              0x0409,
                              2,
                              CAL_ICALINTVALUE );
    CheckReturnValidEnum( rc,
                          FALSE,
                          CalendarCtr,
                          0,
                          "Ex A version iCalIntValue English (cal 2)",
                          &NumErrors );

    //  Variation 3  -  Use CP ACP
    CalendarCtr = 0;
    rc = EnumCalendarInfoA( MyFuncCalendarA,
                            0x0409,
                            0,
                            CAL_ICALINTVALUE | LOCALE_USE_CP_ACP );
    CheckReturnValidEnum( rc,
                          FALSE,
                          CalendarCtr,
                          0,
                          "A version Use CP ACP",
                          &NumErrors );

    CalendarCtr = 0;
    rc = EnumCalendarInfoExA( MyFuncCalendarExA,
                              0x0409,
                              0,
                              CAL_ICALINTVALUE | LOCALE_USE_CP_ACP );
    CheckReturnValidEnum( rc,
                          FALSE,
                          CalendarCtr,
                          0,
                          "Ex A version Use CP ACP",
                          &NumErrors );


    //
    //  CALTYPE values - English.
    //

    for (ctr = 0; ctr < NUM_CAL_FLAGS; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoA( MyFuncCalendarA,
                                0x0409,
                                1,
                                pCalFlag[ctr] );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  pCalEnglish[ctr],
                                  "A version English (cal 1) Calendar Flag",
                                  pCalFlag[ctr],
                                  &NumErrors );
    }

    for (ctr = 0; ctr < NUM_CAL_FLAGS; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoExA( MyFuncCalendarExA,
                                  0x0409,
                                  1,
                                  pCalFlag[ctr] );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  pCalEnglish[ctr],
                                  "Ex A version English (cal 1) Calendar Flag",
                                  pCalFlag[ctr],
                                  &NumErrors );
    }



    //
    //  Single calendar id - Japan.
    //

    //  Variation 1  -  iCalIntValue - Japan
    CalendarCtr = 0;
    rc = EnumCalendarInfoA( MyFuncCalendarA,
                            0x0411,
                            0,
                            CAL_ICALINTVALUE );
    CheckReturnValidEnum( rc,
                          FALSE,
                          CalendarCtr,
                          0,
                          "A version iCalIntValue Japan (cal 0)",
                          &NumErrors );

    CalendarCtr = 0;
    rc = EnumCalendarInfoExA( MyFuncCalendarExA,
                              0x0411,
                              0,
                              CAL_ICALINTVALUE );
    CheckReturnValidEnum( rc,
                          FALSE,
                          CalendarCtr,
                          0,
                          "Ex A version iCalIntValue Japan (cal 0)",
                          &NumErrors );



    //
    //  CALTYPE values - Japan.
    //

    for (ctr = 0; ctr < NUM_CAL_FLAGS; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoA( MyFuncCalendarA,
                                0x0411,
                                1,
                                pCalFlag[ctr] );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  pCalJapan_1[ctr],
                                  "A version Japan (cal 1) Calendar Flag",
                                  pCalFlag[ctr],
                                  &NumErrors );
    }

    for (ctr = 0; ctr < NUM_CAL_FLAGS; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoExA( MyFuncCalendarExA,
                                  0x0411,
                                  1,
                                  pCalFlag[ctr] );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  pCalJapan_1[ctr],
                                  "Ex A version Japan (cal 1) Calendar Flag",
                                  pCalFlag[ctr],
                                  &NumErrors );
    }

    for (ctr = 0; ctr < NUM_CAL_FLAGS; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoA( MyFuncCalendarA,
                                0x0411,
                                2,
                                pCalFlag[ctr] );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  pCalJapan_2[ctr],
                                  "A version Japan (cal 2) Calendar Flag",
                                  pCalFlag[ctr],
                                  &NumErrors );
    }

    for (ctr = 0; ctr < NUM_CAL_FLAGS; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoExA( MyFuncCalendarExA,
                                  0x0411,
                                  2,
                                  pCalFlag[ctr] );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  pCalJapan_2[ctr],
                                  "Ex A version Japan (cal 2) Calendar Flag",
                                  pCalFlag[ctr],
                                  &NumErrors );
    }

    for (ctr = 0; ctr < NUM_CAL_FLAGS; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoA( MyFuncCalendarA,
                                0x0411,
                                3,
                                pCalFlag[ctr] );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  pCalJapan_3[ctr],
                                  "A version Japan (cal 3) Calendar Flag",
                                  pCalFlag[ctr],
                                  &NumErrors );
    }

    for (ctr = 0; ctr < NUM_CAL_FLAGS; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoExA( MyFuncCalendarExA,
                                  0x0411,
                                  3,
                                  pCalFlag[ctr] );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  pCalJapan_3[ctr],
                                  "Ex A version Japan (cal 3) Calendar Flag",
                                  pCalFlag[ctr],
                                  &NumErrors );
    }



    //
    //  English - Enumerate ALL Calendars.
    //

    for (ctr = 0; ctr < NUM_CAL_FLAGS; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoA( MyFuncCalendarA,
                                0x0409,
                                ENUM_ALL_CALENDARS,
                                pCalFlag[ctr] );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  pCalEnglish[ctr],
                                  "A version English (all cal) Calendar Flag",
                                  pCalFlag[ctr],
                                  &NumErrors );
    }

    for (ctr = 0; ctr < NUM_CAL_FLAGS; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoExA( MyFuncCalendarExA,
                                  0x0409,
                                  ENUM_ALL_CALENDARS,
                                  pCalFlag[ctr] );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  pCalEnglish[ctr],
                                  "Ex A version English (all cal) Calendar Flag",
                                  pCalFlag[ctr],
                                  &NumErrors );
    }

    for (ctr = CAL_SDAYNAME1; ctr <= CAL_SMONTHNAME12; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoA( MyFuncCalendarA,
                                0x0409,
                                ENUM_ALL_CALENDARS,
                                ctr );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  1,
                                  "A version English (all cal) Day/Month Calendar Flag",
                                  ctr,
                                  &NumErrors );
    }

    for (ctr = CAL_SDAYNAME1; ctr <= CAL_SMONTHNAME12; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoExA( MyFuncCalendarExA,
                                  0x0409,
                                  ENUM_ALL_CALENDARS,
                                  ctr );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  1,
                                  "Ex A version English (all cal) Day/Month Calendar Flag",
                                  ctr,
                                  &NumErrors );
    }

    for (ctr = CAL_SABBREVMONTHNAME1; ctr <= CAL_SABBREVMONTHNAME12; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoA( MyFuncCalendarA,
                                0x0409,
                                ENUM_ALL_CALENDARS,
                                ctr );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  1,
                                  "A version English (all cal) Day/Month Calendar Flag",
                                  ctr,
                                  &NumErrors );
    }

    for (ctr = CAL_SABBREVMONTHNAME1; ctr <= CAL_SABBREVMONTHNAME12; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoExA( MyFuncCalendarExA,
                                  0x0409,
                                  ENUM_ALL_CALENDARS,
                                  ctr );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  1,
                                  "Ex A version English (all cal) Day/Month Calendar Flag",
                                  ctr,
                                  &NumErrors );
    }



    //
    //  Japan - Enumerate ALL Calendars.
    //

    for (ctr = 0; ctr < NUM_CAL_FLAGS; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoA( MyFuncCalendarA,
                                0x0411,
                                ENUM_ALL_CALENDARS,
                                pCalFlag[ctr] );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  pCalJapan_All[ctr],
                                  "A version Japan (all cal) Calendar Flag",
                                  pCalFlag[ctr],
                                  &NumErrors );
    }

    for (ctr = 0; ctr < NUM_CAL_FLAGS; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoExA( MyFuncCalendarExA,
                                  0x0411,
                                  ENUM_ALL_CALENDARS,
                                  pCalFlag[ctr] );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  pCalJapan_All[ctr],
                                  "Ex A version Japan (all cal) Calendar Flag",
                                  pCalFlag[ctr],
                                  &NumErrors );
    }

    for (ctr = CAL_SDAYNAME1; ctr <= CAL_SMONTHNAME12; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoA( MyFuncCalendarA,
                                0x0411,
                                ENUM_ALL_CALENDARS,
                                ctr );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  2,
                                  "A version Japan (all cal) Day/Month Calendar Flag",
                                  ctr,
                                  &NumErrors );
    }

    for (ctr = CAL_SDAYNAME1; ctr <= CAL_SMONTHNAME12; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoExA( MyFuncCalendarExA,
                                  0x0411,
                                  ENUM_ALL_CALENDARS,
                                  ctr );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  2,
                                  "Ex A version Japan (all cal) Day/Month Calendar Flag",
                                  ctr,
                                  &NumErrors );
    }

    for (ctr = CAL_SABBREVMONTHNAME1; ctr <= CAL_SABBREVMONTHNAME12; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoA( MyFuncCalendarA,
                                0x0411,
                                ENUM_ALL_CALENDARS,
                                ctr );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  2,
                                  "A version Japan (all cal) Day/Month Calendar Flag",
                                  ctr,
                                  &NumErrors );
    }

    for (ctr = CAL_SABBREVMONTHNAME1; ctr <= CAL_SABBREVMONTHNAME12; ctr++)
    {
        CalendarCtr = 0;
        rc = EnumCalendarInfoExA( MyFuncCalendarExA,
                                  0x0411,
                                  ENUM_ALL_CALENDARS,
                                  ctr );
        CheckReturnValidEnumLoop( rc,
                                  TRUE,
                                  CalendarCtr,
                                  2,
                                  "Ex A version Japan (all cal) Day/Month Calendar Flag",
                                  ctr,
                                  &NumErrors );
    }



    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}
