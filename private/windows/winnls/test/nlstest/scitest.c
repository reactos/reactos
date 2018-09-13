/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    scitest.c

Abstract:

    Test module for NLS API SetCalendarInfo.

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

#define  BUFSIZE          100               // buffer size in wide chars
#define  CALTYPE_INVALID  0x0000002         // invalid CALTYPE




//
//  Global Variables.
//

LCID Locale;

WCHAR lpCalData[BUFSIZE];
WCHAR pTwoDigitYearMax[BUFSIZE];




//
//  Forward Declarations.
//

BOOL
InitSetCalInfo();

int
SCI_BadParamCheck();

int
SCI_NormalCase();

int
SCI_Ansi();





////////////////////////////////////////////////////////////////////////////
//
//  TestSetCalendarInfo
//
//  Test routine for SetCalendarInfoW API.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int TestSetCalendarInfo()
{
    int ErrCount = 0;             // error count


    //
    //  Print out what's being done.
    //
    printf("\n\nTESTING SetCalendarInfoW...\n\n");

    //
    //  Initialize global variables.
    //
    if (!InitSetCalInfo())
    {
        printf("\nABORTED TestSetCalendarInfo: Could not Initialize.\n");
        return (1);
    }

    //
    //  Test bad parameters.
    //
    ErrCount += SCI_BadParamCheck();

    //
    //  Test normal cases.
    //
    ErrCount += SCI_NormalCase();

    //
    //  Test Ansi version.
    //
    ErrCount += SCI_Ansi();

    //
    //  Print out result.
    //
    printf("\nSetCalendarInfoW:  ERRORS = %d\n", ErrCount);

    //
    //  Return total number of errors found.
    //
    return (ErrCount);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitSetCalInfo
//
//  This routine initializes the global variables.  If no errors were
//  encountered, then it returns TRUE.  Otherwise, it returns FALSE.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL InitSetCalInfo()
{
    //
    //  Make a Locale.
    //
    Locale = MAKELCID(0x0409, 0);

    //
    //  Save the ITWODIGITYEARMAX value to be restored later.
    //
    if (!GetCalendarInfoW( Locale,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           pTwoDigitYearMax,
                           BUFSIZE,
                           NULL ))
    {
        printf("ERROR: Initialization ITWODIGITYEARMAX - error = %d\n", GetLastError());
        return (FALSE);
    }


    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  SCI_BadParamCheck
//
//  This routine passes in bad parameters to the API routines and checks to
//  be sure they are handled properly.  The number of errors encountered
//  is returned to the caller.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int SCI_BadParamCheck()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


    //
    //  Bad Locale.
    //

    //  Variation 1  -  Bad Locale
    rc = SetCalendarInfoW( (LCID)333,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           L"2029" );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "Bad Locale",
                         &NumErrors );


    //
    //  Null Pointers.
    //

    //  Variation 1  -  lpCalData = NULL
    rc = SetCalendarInfoW( Locale,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           NULL );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "lpCalData NULL",
                         &NumErrors );


    //
    //  Zero or Invalid Type.
    //

    //  Variation 1  -  CalType = invalid
    rc = SetCalendarInfoW( Locale,
                           CAL_GREGORIAN,
                           CALTYPE_INVALID,
                           L"2029" );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "CalType invalid",
                         &NumErrors );

    //  Variation 2  -  CalType = 0
    rc = SetCalendarInfoW( Locale,
                           CAL_GREGORIAN,
                           0,
                           L"2029" );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "CalType zero",
                         &NumErrors );

    //  Variation 1  -  Use CP ACP, CalType = invalid
    rc = SetCalendarInfoW( Locale,
                           CAL_GREGORIAN,
                           CAL_USE_CP_ACP | CALTYPE_INVALID,
                           L"2029" );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "Use CP ACP, CalType invalid",
                         &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  SCI_NormalCase
//
//  This routine tests the normal cases of the API routine.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int SCI_NormalCase()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


#ifdef PERF

  DbgBreakPoint();

#endif


    //
    //  Locales.
    //

    //  Variation 1  -  System Default Locale
    rc = SetCalendarInfoW( LOCALE_SYSTEM_DEFAULT,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           L"2035" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - system default locale",
                      &NumErrors );
    rc = GetCalendarInfoW( LOCALE_SYSTEM_DEFAULT,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           lpCalData,
                           BUFSIZE,
                           NULL );
    CheckReturnValidW( rc,
                       -1,
                       lpCalData,
                       L"2035",
                       "GET - system default locale",
                       &NumErrors );

    //  Variation 2  -  Current User Locale
    rc = SetCalendarInfoW( LOCALE_USER_DEFAULT,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           L"2040" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - current user locale",
                      &NumErrors );
    rc = GetCalendarInfoW( LOCALE_USER_DEFAULT,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           lpCalData,
                           BUFSIZE,
                           NULL );
    CheckReturnValidW( rc,
                       -1,
                       lpCalData,
                       L"2040",
                       "GET - current user locale",
                       &NumErrors );

    //
    //  Use CP ACP.
    //

    //  Variation 1  -  Use CP ACP, System Default Locale
    rc = SetCalendarInfoW( LOCALE_SYSTEM_DEFAULT,
                           CAL_GREGORIAN,
                           CAL_USE_CP_ACP | CAL_ITWODIGITYEARMAX,
                           L"2050" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - Use CP ACP, system default locale",
                      &NumErrors );
    rc = GetCalendarInfoW( LOCALE_SYSTEM_DEFAULT,
                           CAL_GREGORIAN,
                           CAL_USE_CP_ACP | CAL_ITWODIGITYEARMAX,
                           lpCalData,
                           BUFSIZE,
                           NULL );
    CheckReturnValidW( rc,
                       -1,
                       lpCalData,
                       L"2050",
                       "GET - Use CP ACP, system default locale",
                       &NumErrors );

    //
    //  CALTYPE values.
    //

    //  Variation 1  -  ITWODIGITYEARMAX
    rc = SetCalendarInfoW( Locale,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           L"10000" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - MAX ITWODIGITYEARMAX",
                      &NumErrors );
    rc = SetCalendarInfoW( Locale,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           L"98" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - MIN ITWODIGITYEARMAX",
                      &NumErrors );
    rc = SetCalendarInfoW( Locale,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           L"3030" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - ITWODIGITYEARMAX",
                      &NumErrors );
    rc = GetCalendarInfoW( Locale,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           lpCalData,
                           BUFSIZE,
                           NULL );
    CheckReturnValidW( rc,
                       -1,
                       lpCalData,
                       L"3030",
                       "GET - ITWODIGITYEARMAX",
                       &NumErrors );
    rc = SetCalendarInfoW( Locale,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           pTwoDigitYearMax );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - ITWODIGITYEARMAX",
                      &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  SCI_Ansi
//
//  This routine tests the Ansi version of the API routine.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int SCI_Ansi()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


    //
    //  SetCalendarInfoA.
    //

    //  Variation 1  -  ITWODIGITYEARMAX
    rc = SetCalendarInfoA( Locale,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           "4040" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - system default locale",
                      &NumErrors );
    rc = GetCalendarInfoW( Locale,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           lpCalData,
                           BUFSIZE,
                           NULL );
    CheckReturnValidW( rc,
                       -1,
                       lpCalData,
                       L"4040",
                       "GET - system default locale",
                       &NumErrors );

    //  Variation 2  -  Use CP ACP, ITWODIGITYEARMAX
    rc = SetCalendarInfoA( Locale,
                           CAL_GREGORIAN,
                           CAL_USE_CP_ACP | CAL_ITWODIGITYEARMAX,
                           "4141" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - Use CP ACP, system default locale",
                      &NumErrors );
    rc = GetCalendarInfoW( Locale,
                           CAL_GREGORIAN,
                           CAL_USE_CP_ACP | CAL_ITWODIGITYEARMAX,
                           lpCalData,
                           BUFSIZE,
                           NULL );
    CheckReturnValidW( rc,
                       -1,
                       lpCalData,
                       L"4141",
                       "GET - Use CP ACP, system default locale",
                       &NumErrors );


    //
    //  Reset the ITWODIGITYEARMAX value.
    //
    rc = SetCalendarInfoW( Locale,
                           CAL_GREGORIAN,
                           CAL_ITWODIGITYEARMAX,
                           pTwoDigitYearMax );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - ITWODIGITYEARMAX",
                      &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}
