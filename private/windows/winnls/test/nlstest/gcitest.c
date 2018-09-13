/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    gcitest.c

Abstract:

    Test module for NLS API GetCalendarInfo.

    NOTE: This code was simply hacked together quickly in order to
          test the different code modules of the NLS component.
          This is NOT meant to be a formal regression test.

Revision History:

    03-10-98    JulieB    Created.

--*/



//
//  Include Files.
//

#include "nlstest.h"




//
//  Constant Declarations.
//

#define  BUFSIZE             50             // buffer size in wide chars
#define  CALTYPE_INVALID     0x0000100      // invalid CALTYPE
#define  LANG_INVALID        0x0417         // invalid lang id

#define  S_ITWODIGITYEARMAX  L"2029"
#define  I_ITWODIGITYEARMAX  2029




//
//  Global Variables.
//

LCID Locale;

WCHAR lpCalData[BUFSIZE];
BYTE  lpCalDataA[BUFSIZE];


//
//  pCalInfoFlag and pCalString must have the same number of entries.
//
CALTYPE pCalInfoFlag[] =
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

#define NUM_CAL_FLAGS  ( sizeof(pCalInfoFlag) / sizeof(CALTYPE) )


LPWSTR pCalString[] =
{
    L"1",
    L"Gregorian Calendar",
    L"2029",
    L"0",
    L"",
    L"M/d/yyyy",
    L"dddd, MMMM dd, yyyy",
    L"MMMM, yyyy",
    L"Monday",
    L"January",
    L""
};


LPWSTR pCalStringJapan[] =
{
    L"3",
    L"\x548c\x66a6",
    L"99",
    L"1989",
    L"\x5e73\x6210",
    L"gg y/M/d",
    L"gg y'\x5e74'M'\x6708'd'\x65e5'",
    L"gg y'\x5e74'M'\x6708'",
    L"\x6708\x66dc\x65e5",
    L"1\x6708",
    L""
};


LPWSTR pCalStringHebrew[] =
{
    L"8",
    L"\x05dc\x05d5\x05d7\x00a0\x05e9\x05e0\x05d4\x00a0\x05e2\x05d1\x05e8\x05d9",
    L"5790",
    L"0",
    L"",
    L"dd/MMM/yy",
    L"dd MMMM yyyy",
    L"MMMM yyyy",
    L"\x05d9\x05d5\x05dd\x00a0\x05e9\x05e0\x05d9",
    L"\x05ea\x05e9\x05e8\x05d9",
    L"\x05d0\x05dc\x05d5\x05dc"
};


//
//  pCalIntFlag and pCalInt must have the same number of entries.
//
CALTYPE pCalIntFlag[] =
{
    CAL_ICALINTVALUE,
    CAL_ITWODIGITYEARMAX,
    CAL_IYEAROFFSETRANGE,
};

#define NUM_CAL_INT_FLAGS  ( sizeof(pCalIntFlag) / sizeof(CALTYPE) )


int pCalInt[] =
{
    1,
    2029,
    0
};


int pCalIntJapan[] =
{
    3,
    99,
    1989
};




//
//  Forward Declarations.
//

BOOL
InitGetCalInfo();

int
GCI_BadParamCheck();

int
GCI_NormalCase();

int
GCI_Ansi();





////////////////////////////////////////////////////////////////////////////
//
//  TestGetCalendarInfo
//
//  Test routine for GetCalendarInfoW API.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int TestGetCalendarInfo()
{
    int ErrCount = 0;             // error count


    //
    //  Print out what's being done.
    //
    printf("\n\nTESTING GetCalendarInfoW...\n\n");

    //
    //  Initialize global variables.
    //
    if (!InitGetCalInfo())
    {
        printf("\nABORTED TestGetCalendarInfo: Could not Initialize.\n");
        return (1);
    }

    //
    //  Test bad parameters.
    //
    ErrCount += GCI_BadParamCheck();

    //
    //  Test normal cases.
    //
    ErrCount += GCI_NormalCase();

    //
    //  Test Ansi version.
    //
    ErrCount += GCI_Ansi();

    //
    //  Print out result.
    //
    printf("\nGetCalendarInfoW:  ERRORS = %d\n", ErrCount);

    //
    //  Return total number of errors found.
    //
    return (ErrCount);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitGetCalInfo
//
//  This routine initializes the global variables.  If no errors were
//  encountered, then it returns TRUE.  Otherwise, it returns FALSE.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL InitGetCalInfo()
{
    //
    //  Make a Locale.
    //
    Locale = MAKELCID(0x0409, 0);


    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  GCI_BadParamCheck
//
//  This routine passes in bad parameters to the API routines and checks to
//  be sure they are handled properly.  The number of errors encountered
//  is returned to the caller.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GCI_BadParamCheck()
{
    DWORD dwValue;                // value
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


    //
    //  Bad Locale.
    //

    //  Variation 1  -  Bad Locale
    rc = GetCalendarInfoW( (LCID)333,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           lpCalData,
                           BUFSIZE,
                           NULL );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "Bad Locale",
                         &NumErrors );

    //  Variation 2  -  Bad Locale
    rc = GetCalendarInfoW( LANG_INVALID,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           lpCalData,
                           BUFSIZE,
                           NULL );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "invalid locale - invalid",
                         &NumErrors );


    //
    //  Null Pointers.
    //

    //  Variation 1  -  lpCalData = NULL
    rc = GetCalendarInfoW( Locale,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           NULL,
                           BUFSIZE,
                           NULL );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "lpCalData NULL",
                         &NumErrors );


    //
    //  Bad Counts.
    //

    //  Variation 1  -  cchData < 0
    rc = GetCalendarInfoW( Locale,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           lpCalData,
                           -1,
                           NULL );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "cchData < 0",
                         &NumErrors );


    //
    //  Zero or Invalid Type.
    //

    //  Variation 1  -  CalType = invalid
    rc = GetCalendarInfoW( Locale,
                           CAL_GREGORIAN,
                           CALTYPE_INVALID,
                           lpCalData,
                           BUFSIZE,
                           NULL );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "CalType invalid",
                         &NumErrors );

    //  Variation 2  -  CalType = 0
    rc = GetCalendarInfoW( Locale,
                           CAL_GREGORIAN,
                           0,
                           lpCalData,
                           BUFSIZE,
                           NULL );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "CalType zero",
                         &NumErrors );

    //  Variation 3  -  Use CP ACP, CalType = invalid
    rc = GetCalendarInfoW( Locale,
                           CAL_GREGORIAN,
                           CAL_USE_CP_ACP | CALTYPE_INVALID,
                           lpCalData,
                           BUFSIZE,
                           NULL );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "Use CP ACP, CalType invalid",
                         &NumErrors );



    //
    //  Buffer Too Small.
    //

    //  Variation 1  -  cchData = too small
    rc = GetCalendarInfoW( Locale,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           lpCalData,
                           1,
                           NULL );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INSUFFICIENT_BUFFER,
                         "cchData too small",
                         &NumErrors );


    //
    //  RETURN_NUMBER flag.
    //

    //  Variation 1  -  invalid flags
    rc = GetCalendarInfoW( Locale,
                           CAL_GREGORIAN,
                           CAL_SCALNAME | CAL_RETURN_NUMBER,
                           NULL,
                           0,
                           &dwValue );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "invalid flags - RETURN_NUMBER",
                         &NumErrors );

    //  Variation 2  -  wrong buffer 1
    rc = GetCalendarInfoW( Locale,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX | CAL_RETURN_NUMBER,
                           lpCalData,
                           BUFSIZE,
                           NULL );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "wrong buffer 1 - RETURN_NUMBER",
                         &NumErrors );

    //  Variation 3  -  wrong buffer 2
    rc = GetCalendarInfoW( Locale,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX | CAL_RETURN_NUMBER,
                           lpCalData,
                           0,
                           &dwValue );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "wrong buffer 2 - RETURN_NUMBER",
                         &NumErrors );

    //  Variation 4  -  wrong buffer 3
    rc = GetCalendarInfoW( Locale,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX | CAL_RETURN_NUMBER,
                           NULL,
                           BUFSIZE,
                           &dwValue );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "wrong buffer 3 - RETURN_NUMBER",
                         &NumErrors );


    //
    //  Wrong Buffer.
    //

    //  Variation 1  -  wrong buffer 1
    rc = GetCalendarInfoW( Locale,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           lpCalData,
                           BUFSIZE,
                           &dwValue );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "wrong buffer 1",
                         &NumErrors );

    //  Variation 2  -  wrong buffer 2
    rc = GetCalendarInfoW( Locale,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           NULL,
                           0,
                           &dwValue );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "wrong buffer 2",
                         &NumErrors );

    //  Variation 3  -  wrong buffer 3
    rc = GetCalendarInfoW( Locale,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           lpCalData,
                           0,
                           &dwValue );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "wrong buffer 3",
                         &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  GCI_NormalCase
//
//  This routine tests the normal cases of the API routine.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GCI_NormalCase()
{
    DWORD dwValue;                // value
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code
    int ctr;                      // loop counter


#ifdef PERF

  DbgBreakPoint();

#endif


    //
    //  Locales.
    //

    //  Variation 1  -  System Default Locale
    rc = GetCalendarInfoW( LOCALE_SYSTEM_DEFAULT,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           lpCalData,
                           BUFSIZE,
                           NULL );
    CheckReturnEqual( rc,
                      0,
                      "system default locale",
                      &NumErrors );

    //  Variation 2  -  Current User Locale
    rc = GetCalendarInfoW( LOCALE_USER_DEFAULT,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           lpCalData,
                           BUFSIZE,
                           NULL );
    CheckReturnEqual( rc,
                      0,
                      "current user locale",
                      &NumErrors );


    //
    //  Use CP ACP.
    //

    //  Variation 1  -  Use CP ACP, System Default Locale
    rc = GetCalendarInfoW( LOCALE_SYSTEM_DEFAULT,
                           CAL_GREGORIAN,
                           CAL_USE_CP_ACP | CAL_ITWODIGITYEARMAX,
                           lpCalData,
                           BUFSIZE,
                           NULL );
    CheckReturnEqual( rc,
                      0,
                      "Use CP ACP, system default locale",
                      &NumErrors );


    //
    //  cchData.
    //

    //  Variation 1  -  cchData = size of lpCalData buffer
    rc = GetCalendarInfoW( Locale,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           lpCalData,
                           BUFSIZE,
                           NULL );
    CheckReturnValidW( rc,
                       -1,
                       lpCalData,
                       S_ITWODIGITYEARMAX,
                       "cchData = bufsize",
                       &NumErrors );

    //  Variation 2  -  cchData = 0
    lpCalData[0] = 0x0000;
    rc = GetCalendarInfoW( Locale,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           lpCalData,
                           0,
                           NULL );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       S_ITWODIGITYEARMAX,
                       "cchData zero",
                       &NumErrors );

    //  Variation 3  -  cchData = 0, lpCalData = NULL
    rc = GetCalendarInfoW( Locale,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           NULL,
                           0,
                           NULL );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       S_ITWODIGITYEARMAX,
                       "cchData (NULL ptr)",
                       &NumErrors );


    //
    //  CALTYPE values.
    //

    for (ctr = 0; ctr < NUM_CAL_FLAGS; ctr++)
    {
        rc = GetCalendarInfoW( Locale,
                               CAL_GREGORIAN,
                               pCalInfoFlag[ctr],
                               lpCalData,
                               BUFSIZE,
                               NULL );
        CheckReturnValidLoopW( rc,
                               -1,
                               lpCalData,
                               pCalString[ctr],
                               "Calendar Flag",
                               pCalInfoFlag[ctr],
                               &NumErrors );
    }

    for (ctr = 0; ctr < NUM_CAL_FLAGS; ctr++)
    {
        rc = GetCalendarInfoW( 0x0411,
                               CAL_JAPAN,
                               pCalInfoFlag[ctr],
                               lpCalData,
                               BUFSIZE,
                               NULL );
        CheckReturnValidLoopW( rc,
                               -1,
                               lpCalData,
                               pCalStringJapan[ctr],
                               "Japan Calendar Flag",
                               pCalInfoFlag[ctr],
                               &NumErrors );
    }

    for (ctr = 0; ctr < NUM_CAL_FLAGS; ctr++)
    {
        rc = GetCalendarInfoW( 0x040d,
                               CAL_HEBREW,
                               pCalInfoFlag[ctr],
                               lpCalData,
                               BUFSIZE,
                               NULL );
        CheckReturnValidLoopW( rc,
                               -1,
                               lpCalData,
                               pCalStringHebrew[ctr],
                               "Hebrew Calendar Flag",
                               pCalInfoFlag[ctr],
                               &NumErrors );
    }


    //
    //  Language Neutral.
    //

    //  Variation 1  -  language neutral
    rc = GetCalendarInfoW( 0x0000,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           lpCalData,
                           BUFSIZE,
                           NULL );
    CheckReturnValidW( rc,
                       -1,
                       lpCalData,
                       S_ITWODIGITYEARMAX,
                       "language neutral",
                       &NumErrors );

    //  Variation 2  -  sys default
    rc = GetCalendarInfoW( 0x0400,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           lpCalData,
                           BUFSIZE,
                           NULL );
    CheckReturnValidW( rc,
                       -1,
                       lpCalData,
                       S_ITWODIGITYEARMAX,
                       "sys default",
                       &NumErrors );

    //  Variation 3  -  user default
    rc = GetCalendarInfoW( 0x0800,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           lpCalData,
                           BUFSIZE,
                           NULL );
    CheckReturnValidW( rc,
                       -1,
                       lpCalData,
                       S_ITWODIGITYEARMAX,
                       "user default",
                       &NumErrors );

    //  Variation 4  -  sub lang neutral US
    rc = GetCalendarInfoW( 0x0009,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           lpCalData,
                           BUFSIZE,
                           NULL );
    CheckReturnValidW( rc,
                       -1,
                       lpCalData,
                       S_ITWODIGITYEARMAX,
                       "sub lang neutral US",
                       &NumErrors );



    //
    //  Test Return Number flag.
    //
    //  Variation 1  -  RETURN_NUMBER
    rc = GetCalendarInfoW( Locale,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX | CAL_RETURN_NUMBER,
                           NULL,
                           0,
                           &dwValue );
    CheckReturnValidInt( rc,
                         2,
                         dwValue,
                         2029,
                         "Return_Number - two digit year max",
                         &NumErrors );


    //
    //  Try all INT flags with CAL_RETURN_NUMBER flag.
    //
    for (ctr = 0; ctr < NUM_CAL_INT_FLAGS; ctr++)
    {
        rc = GetCalendarInfoW( Locale,
                               CAL_GREGORIAN,
                               pCalIntFlag[ctr] | CAL_RETURN_NUMBER,
                               NULL,
                               0,
                               &dwValue );
        CheckReturnValidInt( rc,
                             2,
                             dwValue,
                             pCalInt[ctr],
                             "Calendar Int Flag",
                             &NumErrors );
    }

    for (ctr = 0; ctr < NUM_CAL_INT_FLAGS; ctr++)
    {
        rc = GetCalendarInfoW( 0x0411,
                               CAL_JAPAN,
                               pCalIntFlag[ctr] | CAL_RETURN_NUMBER,
                               NULL,
                               0,
                               &dwValue );
        CheckReturnValidInt( rc,
                             2,
                             dwValue,
                             pCalIntJapan[ctr],
                             "Japan Calendar Int Flag",
                             &NumErrors );
    }


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  GCI_Ansi
//
//  This routine tests the Ansi version of the API routine.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GCI_Ansi()
{
    DWORD dwValue;                // value
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code
    int ctr;                      // loop counter


    //
    //  GetCalendarInfoA.
    //

    //  Variation 1  -  ITWODIGITYEARMAX
    rc = GetCalendarInfoA( Locale,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           lpCalDataA,
                           BUFSIZE,
                           NULL );
    CheckReturnValidA( rc,
                       -1,
                       lpCalDataA,
                       "2029",
                       NULL,
                       "A version ITWODIGITYEARMAX",
                       &NumErrors );

    //  Variation 2  -  ITWODIGITYEARMAX
    rc = GetCalendarInfoA( Locale,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           NULL,
                           0,
                           NULL );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       "2029",
                       NULL,
                       "A version ITWODIGITYEARMAX, no Dest",
                       &NumErrors );

    //  Variation 3  -  ITWODIGITYEARMAX
    rc = GetCalendarInfoA( Locale,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           NULL,
                           BUFSIZE,
                           NULL );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "A version bad lpCalData",
                         &NumErrors );


    //
    //  Use CP ACP.
    //

    //  Variation 1  -  Use CP ACP, ITWODIGITYEARMAX
    rc = GetCalendarInfoA( Locale,
                           CAL_GREGORIAN,
                           CAL_USE_CP_ACP | CAL_ITWODIGITYEARMAX,
                           lpCalDataA,
                           BUFSIZE,
                           NULL );
    CheckReturnValidA( rc,
                       -1,
                       lpCalDataA,
                       "2029",
                       NULL,
                       "A version Use CP ACP, ITWODIGITYEARMAX",
                       &NumErrors );


    //
    //  Make sure the A and W versions set the same error value.
    //

    SetLastError( 0 );
    rc = GetCalendarInfoA( Locale,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           lpCalDataA,
                           -1,
                           NULL );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "A and W error returns same - A version",
                         &NumErrors );

    SetLastError( 0 );
    rc = GetCalendarInfoW( Locale,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           lpCalData,
                           -1,
                           NULL );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "A and W error returns same - W version",
                         &NumErrors );


    //
    //  Test Return Number flag.
    //
    //  Variation 1  -  RETURN_NUMBER
    rc = GetCalendarInfoA( Locale,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX | CAL_RETURN_NUMBER,
                           NULL,
                           0,
                           &dwValue );
    CheckReturnValidInt( rc,
                         4,
                         dwValue,
                         2029,
                         "A Version Return_Number - two digit year max",
                         &NumErrors );


    //
    //  Try all INT flags with CAL_RETURN_NUMBER flag.
    //
    for (ctr = 0; ctr < NUM_CAL_INT_FLAGS; ctr++)
    {
        rc = GetCalendarInfoA( Locale,
                               CAL_GREGORIAN,
                               pCalIntFlag[ctr] | CAL_RETURN_NUMBER,
                               NULL,
                               0,
                               &dwValue );
        CheckReturnValidInt( rc,
                             4,
                             dwValue,
                             pCalInt[ctr],
                             "A Version Calendar Int Flag",
                             &NumErrors );
    }


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}
