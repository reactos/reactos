/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    gtftest.c

Abstract:

    Test module for NLS API GetTimeFormat.

    NOTE: This code was simply hacked together quickly in order to
          test the different code modules of the NLS component.
          This is NOT meant to be a formal regression test.

Revision History:

    04-30-93    JulieB    Created.

--*/



//
//  Include Files.
//

#include "nlstest.h"




//
//  Constant Declarations.
//

#define  BUFSIZE                50          // buffer size in wide chars
#define  GTF_INVALID_FLAGS      ((DWORD)(~(LOCALE_NOUSEROVERRIDE |       \
                                           TIME_NOMINUTESORSECONDS |     \
                                           TIME_NOSECONDS |              \
                                           TIME_NOTIMEMARKER |           \
                                           TIME_FORCE24HOURFORMAT)))

#define  ENGLISH_US             L"3:45:25 AM"
#define  CZECH                  L"3:45:25"
#define  DANISH                 L"03.45.25"

#define  US_NOMINSEC            L"3 AM"
#define  US_NOSEC               L"3:45 AM"
#define  US_NOTM                L"3:45:25"
#define  US_NOTM_NOMINSEC       L"3"
#define  US_NOTM_NOSEC          L"3:45"

#define  CZECH_NOMINSEC         L"3"
#define  CZECH_NOSEC            L"3:45"
#define  CZECH_NOTM             L"3:45:25"
#define  CZECH_NOTM_NOMINSEC    L"3"
#define  CZECH_NOTM_NOSEC       L"3:45"


#define  US_24HR                L"3:45:25 AM"
#define  US_24HR_2              L"15:45:25 PM"
#define  CZECH_24HR             L"3:45:25"
#define  CZECH_24HR_2           L"15:45:25"




//
//  Global Variables.
//

LCID Locale;

SYSTEMTIME SysTime;
SYSTEMTIME SysTime2;

WCHAR lpTimeStr[BUFSIZE];


//
//  Time format buffers must be in line with the pAllLocales global
//  buffer.
//
LPWSTR pTimeAM[] =
{
    L"03:45:25",                                 //  0x0402
    L"\x4e0a\x5348 03:45:25",                    //  0x0404
    L"3:45:25",                                  //  0x0804
    L"3:45:25",                                  //  0x0c04
    L"AM 3:45:25",                               //  0x1004
    L"3:45:25",                                  //  0x0405
    L"03:45:25",                                 //  0x0406
    L"03:45:25",                                 //  0x0407
    L"03:45:25",                                 //  0x0807
    L"03:45:25",                                 //  0x0c07
    L"3:45:25 \x03c0\x03bc",                     //  0x0408
    L"3:45:25 AM",                               //  0x0409
    L"03:45:25",                                 //  0x0809
    L"3:45:25 AM",                               //  0x0c09
    L"3:45:25 AM",                               //  0x1009
    L"3:45:25 a.m.",                             //  0x1409
    L"03:45:25",                                 //  0x1809
    L"3:45:25",                                  //  0x040a
    L"03:45:25 a.m.",                            //  0x080a
    L"3:45:25",                                  //  0x0c0a
    L"3:45:25",                                  //  0x040b
    L"03:45:25",                                 //  0x040c
    L"3:45:25",                                  //  0x080c
    L"03:45:25",                                 //  0x0c0c
    L"03:45:25",                                 //  0x100c
    L"3:45:25",                                  //  0x040e
    L"03:45:25",                                 //  0x040f
    L"3.45.25",                                  //  0x0410
    L"03:45:25",                                 //  0x0810
    L"3:45:25",                                  //  0x0411
    L"\xc624\xc804 3:45:25",                     //  0x0412
    L"3:45:25",                                  //  0x0413
    L"3:45:25",                                  //  0x0813
    L"03:45:25",                                 //  0x0414
    L"03:45:25",                                 //  0x0814
    L"03:45:25",                                 //  0x0415
    L"03:45:25",                                 //  0x0416
    L"3:45:25",                                  //  0x0816
    L"03:45:25",                                 //  0x0418
    L"3:45:25",                                  //  0x0419
    L"3:45:25",                                  //  0x041a
    L"3:45:25",                                  //  0x041b
    L"03:45:25",                                 //  0x041d
    L"03:45:25",                                 //  0x041f
    L"3:45:25"                                   //  0x0424
};

LPWSTR pTimePM[] =
{
    L"15:45:25",                                 //  0x0402
    L"\x4e0b\x5348 03:45:25",                    //  0x0404
    L"15:45:25",                                 //  0x0804
    L"15:45:25",                                 //  0x0c04
    L"PM 3:45:25",                               //  0x1004
    L"15:45:25",                                 //  0x0405
    L"15:45:25",                                 //  0x0406
    L"15:45:25",                                 //  0x0407
    L"15:45:25",                                 //  0x0807
    L"15:45:25",                                 //  0x0c07
    L"3:45:25 \x03bc\x03bc",                     //  0x0408
    L"3:45:25 PM",                               //  0x0409
    L"15:45:25",                                 //  0x0809
    L"3:45:25 PM",                               //  0x0c09
    L"3:45:25 PM",                               //  0x1009
    L"3:45:25 p.m.",                             //  0x1409
    L"15:45:25",                                 //  0x1809
    L"15:45:25",                                 //  0x040a
    L"03:45:25 p.m.",                            //  0x080a
    L"15:45:25",                                 //  0x0c0a
    L"15:45:25",                                 //  0x040b
    L"15:45:25",                                 //  0x040c
    L"15:45:25",                                 //  0x080c
    L"15:45:25",                                 //  0x0c0c
    L"15:45:25",                                 //  0x100c
    L"15:45:25",                                 //  0x040e
    L"15:45:25",                                 //  0x040f
    L"15.45.25",                                 //  0x0410
    L"15:45:25",                                 //  0x0810
    L"15:45:25",                                 //  0x0411
    L"\xc624\xd6c4 3:45:25",                     //  0x0412
    L"15:45:25",                                 //  0x0413
    L"15:45:25",                                 //  0x0813
    L"15:45:25",                                 //  0x0414
    L"15:45:25",                                 //  0x0814
    L"15:45:25",                                 //  0x0415
    L"15:45:25",                                 //  0x0416
    L"15:45:25",                                 //  0x0816
    L"15:45:25",                                 //  0x0418
    L"15:45:25",                                 //  0x0419
    L"15:45:25",                                 //  0x041a
    L"15:45:25",                                 //  0x041b
    L"15:45:25",                                 //  0x041d
    L"15:45:25",                                 //  0x041f
    L"15:45:25"                                  //  0x0424
};




//
//  Forward Declarations.
//

BOOL
InitGetTimeFormat();

int
GTF_BadParamCheck();

int
GTF_NormalCase();

int
GTF_Ansi();





////////////////////////////////////////////////////////////////////////////
//
//  TestGetTimeFormat
//
//  Test routine for GetTimeFormatW API.
//
//  04-30-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int TestGetTimeFormat()
{
    int ErrCount = 0;             // error count


    //
    //  Print out what's being done.
    //
    printf("\n\nTESTING GetTimeFormatW...\n\n");

    //
    //  Initialize global variables.
    //
    if (!InitGetTimeFormat())
    {
        printf("\nABORTED TestGetTimeFormat: Could not Initialize.\n");
        return (1);
    }

    //
    //  Test bad parameters.
    //
    ErrCount += GTF_BadParamCheck();

    //
    //  Test normal cases.
    //
    ErrCount += GTF_NormalCase();

    //
    //  Test Ansi version.
    //
    ErrCount += GTF_Ansi();

    //
    //  Print out result.
    //
    printf("\nGetTimeFormatW:  ERRORS = %d\n", ErrCount);

    //
    //  Return total number of errors found.
    //
    return (ErrCount);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitGetTimeFormat
//
//  This routine initializes the global variables.  If no errors were
//  encountered, then it returns TRUE.  Otherwise, it returns FALSE.
//
//  04-30-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL InitGetTimeFormat()
{
    //
    //  Make a Locale.
    //
    Locale = MAKELCID(0x0409, 0);

    //
    //  Initialize the system time.
    //
    SysTime.wYear = 1993;
    SysTime.wMonth = 5;
    SysTime.wDayOfWeek = 6;
    SysTime.wDay = 1;
    SysTime.wHour = 3;
    SysTime.wMinute = 45;
    SysTime.wSecond = 25;
    SysTime.wMilliseconds = 13;

    SysTime2.wYear = 1993;
    SysTime2.wMonth = 5;
    SysTime2.wDayOfWeek = 6;
    SysTime2.wDay = 1;
    SysTime2.wHour = 15;
    SysTime2.wMinute = 45;
    SysTime2.wSecond = 25;
    SysTime2.wMilliseconds = 13;

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  GTF_BadParamCheck
//
//  This routine passes in bad parameters to the API routines and checks to
//  be sure they are handled properly.  The number of errors encountered
//  is returned to the caller.
//
//  04-30-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GTF_BadParamCheck()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code
    SYSTEMTIME MyTime;            // structure to hold custom time


    //
    //  Bad Locale.
    //

    //  Variation 1  -  Bad Locale
    rc = GetTimeFormatW( (LCID)333,
                          0,
                          NULL,
                          NULL,
                          lpTimeStr,
                          BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "Bad Locale",
                         &NumErrors );


    //
    //  Null Pointers.
    //

    //  Variation 1  -  lpTimeStr = NULL
    rc = GetTimeFormatW( Locale,
                         0,
                         NULL,
                         NULL,
                         NULL,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "lpTimeStr NULL",
                         &NumErrors );


    //
    //  Bad Count.
    //

    //  Variation 1  -  cchTime < 0
    rc = GetTimeFormatW( Locale,
                         0,
                         NULL,
                         NULL,
                         lpTimeStr,
                         -1 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "cchTime < 0",
                         &NumErrors );


    //
    //  Invalid Flag.
    //

    //  Variation 1  -  LCType = invalid
    rc = GetTimeFormatW( Locale,
                         GTF_INVALID_FLAGS,
                         NULL,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "Flag invalid",
                         &NumErrors );

    //  Variation 2  -  lpFormat and NoUserOverride
    rc = GetTimeFormatW( Locale,
                         LOCALE_NOUSEROVERRIDE,
                         NULL,
                         L"tt hh:mm:ss",
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "lpFormat and NoUserOverride",
                         &NumErrors );

    //  Variation 3  -  Use CP ACP, lpFormat and NoUserOverride
    rc = GetTimeFormatW( Locale,
                         LOCALE_USE_CP_ACP | LOCALE_NOUSEROVERRIDE,
                         NULL,
                         L"tt hh:mm:ss",
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "Use CP ACP, lpFormat and NoUserOverride",
                         &NumErrors );


    //
    //  Buffer Too Small.
    //

    //  Variation 1  -  cchTime = too small
    rc = GetTimeFormatW( Locale,
                         0,
                         NULL,
                         NULL,
                         lpTimeStr,
                         2 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INSUFFICIENT_BUFFER,
                         "cchTime too small",
                         &NumErrors );


    //
    //  Bad time passed in.
    //

    //  Variation 1  -  bad wHour
    MyTime.wYear = 1993;
    MyTime.wMonth = 5;
    MyTime.wDayOfWeek = 6;
    MyTime.wDay = 1;
    MyTime.wHour = 24;
    MyTime.wMinute = 45;
    MyTime.wSecond = 25;
    MyTime.wMilliseconds = 13;
    rc = GetTimeFormatW( Locale,
                         0,
                         &MyTime,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad wHour",
                         &NumErrors );

    //  Variation 2  -  bad wMinute
    MyTime.wYear = 1993;
    MyTime.wMonth = 5;
    MyTime.wDayOfWeek = 6;
    MyTime.wDay = 1;
    MyTime.wHour = 15;
    MyTime.wMinute = 60;
    MyTime.wSecond = 25;
    MyTime.wMilliseconds = 13;
    rc = GetTimeFormatW( Locale,
                         0,
                         &MyTime,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad wMinute",
                         &NumErrors );

    //  Variation 3  -  bad wSecond
    MyTime.wYear = 1993;
    MyTime.wMonth = 5;
    MyTime.wDayOfWeek = 6;
    MyTime.wDay = 1;
    MyTime.wHour = 15;
    MyTime.wMinute = 45;
    MyTime.wSecond = 60;
    MyTime.wMilliseconds = 13;
    rc = GetTimeFormatW( Locale,
                         0,
                         &MyTime,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad wSecond",
                         &NumErrors );

    //  Variation 4  -  bad wMilliseconds
    MyTime.wYear = 1993;
    MyTime.wMonth = 5;
    MyTime.wDayOfWeek = 6;
    MyTime.wDay = 1;
    MyTime.wHour = 15;
    MyTime.wMinute = 45;
    MyTime.wSecond = 25;
    MyTime.wMilliseconds = 1000;
    rc = GetTimeFormatW( Locale,
                         0,
                         &MyTime,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad wMilliseconds",
                         &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  GTF_NormalCase
//
//  This routine tests the normal cases of the API routine.
//
//  04-30-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GTF_NormalCase()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code
    SYSTEMTIME MyTime;            // structure to hold custom time
    int ctr;                      // loop counter


#ifdef PERF

  DbgBreakPoint();

#endif


    //
    //  Locales.
    //

    //  Variation 1  -  System Default Locale
    rc = GetTimeFormatW( LOCALE_SYSTEM_DEFAULT,
                         0,
                         NULL,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnEqual( rc,
                      0,
                      "system default locale",
                      &NumErrors );

    //  Variation 2  -  Current User Locale
    rc = GetTimeFormatW( LOCALE_USER_DEFAULT,
                         0,
                         NULL,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnEqual( rc,
                      0,
                      "current user locale",
                      &NumErrors );


    //
    //  Language Neutral.
    //

    //  Variation 1  -  neutral
    rc = GetTimeFormatW( 0x0000,
                         0,
                         &SysTime,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       ENGLISH_US,
                       "neutral locale",
                       &NumErrors );

    //  Variation 2  -  sys default
    rc = GetTimeFormatW( 0x0400,
                         0,
                         &SysTime,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       ENGLISH_US,
                       "sys default locale",
                       &NumErrors );

    //  Variation 3  -  user default
    rc = GetTimeFormatW( 0x0800,
                         0,
                         &SysTime,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       ENGLISH_US,
                       "user default locale",
                       &NumErrors );

    //  Variation 4  -  sub lang neutral US
    rc = GetTimeFormatW( 0x0009,
                         0,
                         &SysTime,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       ENGLISH_US,
                       "sub lang neutral US",
                       &NumErrors );

    //  Variation 5  -  sub lang neutral Czech
    rc = GetTimeFormatW( 0x0005,
                         0,
                         &SysTime,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       CZECH,
                       "sub lang neutral Czech",
                       &NumErrors );


    //
    //  Use CP ACP.
    //

    //  Variation 1  -  Use CP ACP, System Default Locale
    rc = GetTimeFormatW( LOCALE_SYSTEM_DEFAULT,
                         LOCALE_USE_CP_ACP,
                         NULL,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnEqual( rc,
                      0,
                      "Use CP ACP, system default locale",
                      &NumErrors );


    //
    //  cchTime.
    //

    //  Variation 1  -  cchTime = size of lpTimeStr buffer
    rc = GetTimeFormatW( Locale,
                         0,
                         &SysTime,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       ENGLISH_US,
                       "cchTime = bufsize",
                       &NumErrors );

    //  Variation 2  -  cchTime = 0
    lpTimeStr[0] = 0x0000;
    rc = GetTimeFormatW( Locale,
                         0,
                         &SysTime,
                         NULL,
                         lpTimeStr,
                         0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       ENGLISH_US,
                       "cchTime zero",
                       &NumErrors );

    //  Variation 3  -  cchTime = 0, lpTimeStr = NULL
    rc = GetTimeFormatW( Locale,
                         0,
                         &SysTime,
                         NULL,
                         NULL,
                         0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       ENGLISH_US,
                       "cchTime (NULL ptr)",
                       &NumErrors );


    //
    //  lpFormat.
    //

    //  Variation 1  -  AM/PM
    rc = GetTimeFormatW( 0x0409,
                         0,
                         &SysTime,
                         L"tt hh:mm:ss",
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       L"AM 03:45:25",
                       "lpFormat AM/PM (tt hh:mm:ss)",
                       &NumErrors );

    //  Variation 2  -  AM/PM
    rc = GetTimeFormatW( 0x0409,
                         TIME_NOTIMEMARKER,
                         &SysTime,
                         L"tt hh:mm:ss",
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       L"03:45:25",
                       "lpFormat NoTimeMarker (tt hh:mm:ss)",
                       &NumErrors );

    //  Variation 3  -  AM/PM
    rc = GetTimeFormatW( 0x0409,
                         TIME_NOTIMEMARKER | TIME_NOSECONDS,
                         &SysTime,
                         L"tt hh:mm:ss",
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       L"03:45",
                       "lpFormat NoTimeMarker, NoSeconds (tt hh:mm:ss)",
                       &NumErrors );

    //  Variation 4  -  AM/PM
    rc = GetTimeFormatW( 0x0409,
                         TIME_NOTIMEMARKER | TIME_NOMINUTESORSECONDS,
                         &SysTime,
                         L"tt hh:mm:ss",
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       L"03",
                       "lpFormat NoTimeMarker, NoMinutesOrSeconds (tt hh:mm:ss)",
                       &NumErrors );

    //  Variation 5  -  AM/PM
    rc = GetTimeFormatW( 0x0409,
                         TIME_NOMINUTESORSECONDS,
                         &SysTime,
                         L"tt hh:mm:ss",
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       L"AM 03",
                       "lpFormat NoMinutesOrSeconds (tt hh:mm:ss)",
                       &NumErrors );

    //  Variation 6  -  Extra h, m, s
    rc = GetTimeFormatW( 0x0409,
                         0,
                         &SysTime,
                         L"hhh:mmm:sss",
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       L"03:45:25",
                       "lpFormat (hhh:mmm:sss)",
                       &NumErrors );

    //  Variation 7  -  Extra H, m, s
    rc = GetTimeFormatW( 0x0409,
                         0,
                         &SysTime2,
                         L"HHH:mmm:sss",
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       L"15:45:25",
                       "lpFormat (HHH:mmm:sss)",
                       &NumErrors );

    //  Variation 8  -  h:m:s
    MyTime.wHour = 15;
    MyTime.wMinute = 4;
    MyTime.wSecond = 5;
    MyTime.wMilliseconds = 13;
    rc = GetTimeFormatW( 0x0409,
                         0,
                         &MyTime,
                         L"h:m:s",
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       L"3:4:5",
                       "lpFormat (h:m:s)",
                       &NumErrors );

    //  Variation 9  -  H:m:s
    MyTime.wHour = 15;
    MyTime.wMinute = 4;
    MyTime.wSecond = 5;
    MyTime.wMilliseconds = 13;
    rc = GetTimeFormatW( 0x0409,
                         0,
                         &MyTime,
                         L"H:m:s",
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       L"15:4:5",
                       "lpFormat (H:m:s)",
                       &NumErrors );

    //  Variation 10  -  single quote
    MyTime.wHour = 15;
    MyTime.wMinute = 4;
    MyTime.wSecond = 5;
    MyTime.wMilliseconds = 13;
    rc = GetTimeFormatW( 0x0409,
                         0,
                         &MyTime,
                         L"h 'oclock'",
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       L"3 oclock",
                       "lpFormat (h 'oclock')",
                       &NumErrors );

    //  Variation 11  -  single quote
    MyTime.wHour = 15;
    MyTime.wMinute = 4;
    MyTime.wSecond = 5;
    MyTime.wMilliseconds = 13;
    rc = GetTimeFormatW( 0x0409,
                         0,
                         &MyTime,
                         L"h 'o''clock' tt",
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       L"3 o'clock PM",
                       "lpFormat (h 'o''clock' tt)",
                       &NumErrors );


    //
    //  Flag values.
    //

    //  Variation 1  -  NOUSEROVERRIDE
    rc = GetTimeFormatW( Locale,
                         LOCALE_NOUSEROVERRIDE,
                         &SysTime,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       ENGLISH_US,
                       "NoUserOverride",
                       &NumErrors );

    //  Variation 2  -  US NOMINUTESORSECONDS
    rc = GetTimeFormatW( Locale,
                         TIME_NOMINUTESORSECONDS,
                         &SysTime,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       US_NOMINSEC,
                       "US NoMinutesOrSeconds",
                       &NumErrors );

    //  Variation 3  -  US NOSECONDS
    rc = GetTimeFormatW( Locale,
                         TIME_NOSECONDS,
                         &SysTime,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       US_NOSEC,
                       "US NoSeconds",
                       &NumErrors );

    //  Variation 4  -  US NOMINUTESORSECONDS and NOSECONDS
    rc = GetTimeFormatW( Locale,
                         TIME_NOMINUTESORSECONDS | TIME_NOSECONDS,
                         &SysTime,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       US_NOMINSEC,
                       "US NoMinutesOrSeconds, NoSeconds",
                       &NumErrors );

    //  Variation 5  -  US NOTIMEMARKER
    rc = GetTimeFormatW( Locale,
                         TIME_NOTIMEMARKER,
                         &SysTime,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       US_NOTM,
                       "US NoTimeMarker",
                       &NumErrors );

    //  Variation 6  -  US NOTIMEMARKER and NOMINUTESORSECONDS
    rc = GetTimeFormatW( Locale,
                         TIME_NOTIMEMARKER | TIME_NOMINUTESORSECONDS,
                         &SysTime,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       US_NOTM_NOMINSEC,
                       "US NoTimeMarker, NoMinutesOrSeconds",
                       &NumErrors );

    //  Variation 7  -  US NOTIMEMARKER and NOSECONDS
    rc = GetTimeFormatW( Locale,
                         TIME_NOTIMEMARKER | TIME_NOSECONDS,
                         &SysTime,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       US_NOTM_NOSEC,
                       "US NoTimeMarker, NoSeconds",
                       &NumErrors );

    //  Variation 8  -  US NOTIMEMARKER and NOMINUTESORSECONDS and NOSECONDS
    rc = GetTimeFormatW( Locale,
                         TIME_NOTIMEMARKER | TIME_NOMINUTESORSECONDS | TIME_NOSECONDS,
                         &SysTime,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       US_NOTM_NOMINSEC,
                       "US NoTimeMarker, NoMinutesOrSeconds, NoSeconds",
                       &NumErrors );

    //  Variation 9  -  CZECH NOMINUTESORSECONDS
    rc = GetTimeFormatW( 0x0405,
                         TIME_NOMINUTESORSECONDS,
                         &SysTime,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       CZECH_NOMINSEC,
                       "Czech NoMinutesOrSeconds",
                       &NumErrors );

    //  Variation 10 -  CZECH NOSECONDS
    rc = GetTimeFormatW( 0x0405,
                         TIME_NOSECONDS,
                         &SysTime,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       CZECH_NOSEC,
                       "Czech NoSeconds",
                       &NumErrors );

    //  Variation 11 -  CZECH NOMINUTESORSECONDS and NOSECONDS
    rc = GetTimeFormatW( 0x0405,
                         TIME_NOMINUTESORSECONDS | TIME_NOSECONDS,
                         &SysTime,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       CZECH_NOMINSEC,
                       "Czech NoMinutesOrSeconds, NoSeconds",
                       &NumErrors );

    //  Variation 12 -  CZECH NOTIMEMARKER
    rc = GetTimeFormatW( 0x0405,
                         TIME_NOTIMEMARKER,
                         &SysTime,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       CZECH_NOTM,
                       "Czech NoTimeMarker",
                       &NumErrors );

    //  Variation 13 -  CZECH NOTIMEMARKER and NOMINUTESORSECONDS
    rc = GetTimeFormatW( 0x0405,
                         TIME_NOTIMEMARKER | TIME_NOMINUTESORSECONDS,
                         &SysTime,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       CZECH_NOTM_NOMINSEC,
                       "Czech NoTimeMarker, NoMinutesOrSeconds",
                       &NumErrors );

    //  Variation 14 -  CZECH NOTIMEMARKER and NOSECONDS
    rc = GetTimeFormatW( 0x0405,
                         TIME_NOTIMEMARKER | TIME_NOSECONDS,
                         &SysTime,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       CZECH_NOTM_NOSEC,
                       "Czech NoTimeMarker, NoSeconds",
                       &NumErrors );

    //  Variation 15 -  CZECH NOTIMEMARKER and NOMINUTESORSECONDS and NOSECONDS
    rc = GetTimeFormatW( 0x0405,
                         TIME_NOTIMEMARKER | TIME_NOMINUTESORSECONDS | TIME_NOSECONDS,
                         &SysTime,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       CZECH_NOTM_NOMINSEC,
                       "Czech NoTimeMarker, NoMinutesOrSeconds, NoSeconds",
                       &NumErrors );


    //  Variation 16  -  US FORCE24HOURFORMAT
    rc = GetTimeFormatW( Locale,
                         TIME_FORCE24HOURFORMAT,
                         &SysTime,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       US_24HR,
                       "US Force24HourFormat 1",
                       &NumErrors );

    //  Variation 17  -  US FORCE24HOURFORMAT
    rc = GetTimeFormatW( Locale,
                         TIME_FORCE24HOURFORMAT,
                         &SysTime2,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       US_24HR_2,
                       "US Force24HourFormat 2",
                       &NumErrors );

    //  Variation 18 -  CZECH FORCE24HOURFORMAT
    rc = GetTimeFormatW( 0x0405,
                         TIME_FORCE24HOURFORMAT,
                         &SysTime,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       CZECH_24HR,
                       "Czech Force24HourFormat",
                       &NumErrors );

    //  Variation 19 -  CZECH FORCE24HOURFORMAT
    rc = GetTimeFormatW( 0x0405,
                         TIME_FORCE24HOURFORMAT,
                         &SysTime2,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpTimeStr,
                       CZECH_24HR_2,
                       "Czech Force24HourFormat 2",
                       &NumErrors );



    //
    //  Test all locales - 3:45:25 AM
    //

    for (ctr = 0; ctr < NumLocales; ctr++)
    {
        rc = GetTimeFormatW( pAllLocales[ctr],
                             0,
                             &SysTime,
                             NULL,
                             lpTimeStr,
                             BUFSIZE );
        CheckReturnValidLoopW( rc,
                               -1,
                               lpTimeStr,
                               pTimeAM[ctr],
                               "Time AM",
                               pAllLocales[ctr],
                               &NumErrors );
    }


    //
    //  Test all locales - 3:45:25 PM
    //

    for (ctr = 0; ctr < NumLocales; ctr++)
    {
        rc = GetTimeFormatW( pAllLocales[ctr],
                             0,
                             &SysTime2,
                             NULL,
                             lpTimeStr,
                             BUFSIZE );
        CheckReturnValidLoopW( rc,
                               -1,
                               lpTimeStr,
                               pTimePM[ctr],
                               "Time PM",
                               pAllLocales[ctr],
                               &NumErrors );
    }



    //
    //  System Time.
    //
    //  NOTE: For this test, must use a locale that has the same length
    //        for both the AM and PM, since I don't check if the test
    //        is run in the AM or PM.
    //

    //  Variation 1  -  Danish system time
    rc = GetTimeFormatW( 0x0406,
                         0,
                         NULL,
                         NULL,
                         lpTimeStr,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       DANISH,
                       "Danish System Time",
                       &NumErrors );



    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  GTF_Ansi
//
//  This routine tests the Ansi version of the API routine.
//
//  04-30-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GTF_Ansi()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code
    SYSTEMTIME MyTime;            // structure to hold custom time
    BYTE pTimeStrA[BUFSIZE];      // ptr to time string


    MyTime.wYear = 1993;
    MyTime.wMonth = 5;
    MyTime.wDayOfWeek = 6;
    MyTime.wDay = 1;
    MyTime.wHour = 15;
    MyTime.wMinute = 4;
    MyTime.wSecond = 5;
    MyTime.wMilliseconds = 13;


    //
    //  GetTimeFormatA
    //

    //  Variation 1  -  AM/PM
    rc = GetTimeFormatA( 0x0409,
                         0,
                         &MyTime,
                         "tt hh:mm:ss",
                         pTimeStrA,
                         BUFSIZE );
    CheckReturnValidA( rc,
                       -1,
                       pTimeStrA,
                       "PM 03:04:05",
                       NULL,
                       "A version (tt hh:mm:ss)",
                       &NumErrors );

    //  Variation 2  -  AM/PM (no dest)
    rc = GetTimeFormatA( 0x0409,
                         0,
                         &MyTime,
                         "tt hh:mm:ss",
                         NULL,
                         0 );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       "PM 03:04:05",
                       NULL,
                       "A version (tt hh:mm:ss), no Dest",
                       &NumErrors );


    //
    //  Use CP ACP.
    //

    //  Variation 1  -  Use CP ACP, AM/PM
    rc = GetTimeFormatA( 0x0409,
                         LOCALE_USE_CP_ACP,
                         &MyTime,
                         "tt hh:mm:ss",
                         pTimeStrA,
                         BUFSIZE );
    CheckReturnValidA( rc,
                       -1,
                       pTimeStrA,
                       "PM 03:04:05",
                       NULL,
                       "A version Use CP ACP (tt hh:mm:ss)",
                       &NumErrors );



    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}
