/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    slitest.c

Abstract:

    Test module for NLS API SetLocaleInfo.

    NOTE: This code was simply hacked together quickly in order to
          test the different code modules of the NLS component.
          This is NOT meant to be a formal regression test.

Revision History:

    07-14-93    JulieB    Created.

--*/



//
//  Include Files.
//

#include "nlstest.h"




//
//  Constant Declarations.
//

#define  BUFSIZE         100                // buffer size in wide chars
#define  LCTYPE_INVALID  0x0000002          // invalid LCTYPE




//
//  Global Variables.
//

LCID Locale;

WCHAR lpLCData[BUFSIZE];
WCHAR pTemp[BUFSIZE];
WCHAR pSList[BUFSIZE];
WCHAR pSTimeFormat[BUFSIZE];
WCHAR pSTime[BUFSIZE];
WCHAR pITime[BUFSIZE];
WCHAR pSShortDate[BUFSIZE];
WCHAR pSDate[BUFSIZE];




//
//  Forward Declarations.
//

BOOL
InitSetLocInfo();

int
SLI_BadParamCheck();

int
SLI_NormalCase();

int
SLI_Ansi();





////////////////////////////////////////////////////////////////////////////
//
//  TestSetLocaleInfo
//
//  Test routine for SetLocaleInfoW API.
//
//  07-14-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int TestSetLocaleInfo()
{
    int ErrCount = 0;             // error count


    //
    //  Print out what's being done.
    //
    printf("\n\nTESTING SetLocaleInfoW...\n\n");

    //
    //  Initialize global variables.
    //
    if (!InitSetLocInfo())
    {
        printf("\nABORTED TestSetLocaleInfo: Could not Initialize.\n");
        return (1);
    }

    //
    //  Test bad parameters.
    //
    ErrCount += SLI_BadParamCheck();

    //
    //  Test normal cases.
    //
    ErrCount += SLI_NormalCase();

    //
    //  Test Ansi version.
    //
    ErrCount += SLI_Ansi();

    //
    //  Print out result.
    //
    printf("\nSetLocaleInfoW:  ERRORS = %d\n", ErrCount);

    //
    //  Return total number of errors found.
    //
    return (ErrCount);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitSetLocInfo
//
//  This routine initializes the global variables.  If no errors were
//  encountered, then it returns TRUE.  Otherwise, it returns FALSE.
//
//  07-14-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL InitSetLocInfo()
{
    //
    //  Make a Locale.
    //
    Locale = MAKELCID(0x0409, 0);

    //
    //  Save the SLIST value to be restored later.
    //
    if ( !GetLocaleInfoW( Locale,
                          LOCALE_SLIST,
                          pSList,
                          BUFSIZE ) )
    {
        printf("ERROR: Initialization SLIST - error = %d\n", GetLastError());
        return (FALSE);
    }

    if ( !GetLocaleInfoW( Locale,
                          LOCALE_STIMEFORMAT,
                          pSTimeFormat,
                          BUFSIZE ) )
    {
        printf("ERROR: Initialization STIMEFORMAT - error = %d\n", GetLastError());
        return (FALSE);
    }

    if ( !GetLocaleInfoW( Locale,
                          LOCALE_STIME,
                          pSTime,
                          BUFSIZE ) )
    {
        printf("ERROR: Initialization STIME - error = %d\n", GetLastError());
        return (FALSE);
    }

    if ( !GetLocaleInfoW( Locale,
                          LOCALE_ITIME,
                          pITime,
                          BUFSIZE ) )
    {
        printf("ERROR: Initialization ITIME - error = %d\n", GetLastError());
        return (FALSE);
    }

    if ( !GetLocaleInfoW( Locale,
                          LOCALE_SSHORTDATE,
                          pSShortDate,
                          BUFSIZE ) )
    {
        printf("ERROR: Initialization SSHORTDATE - error = %d\n", GetLastError());
        return (FALSE);
    }

    if ( !GetLocaleInfoW( Locale,
                          LOCALE_SDATE,
                          pSDate,
                          BUFSIZE ) )
    {
        printf("ERROR: Initialization SDATE - error = %d\n", GetLastError());
        return (FALSE);
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  SLI_BadParamCheck
//
//  This routine passes in bad parameters to the API routines and checks to
//  be sure they are handled properly.  The number of errors encountered
//  is returned to the caller.
//
//  07-14-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int SLI_BadParamCheck()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


    //
    //  Bad Locale.
    //

    //  Variation 1  -  Bad Locale
    rc = SetLocaleInfoW( (LCID)333,
                         LOCALE_SLIST,
                         L"," );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "Bad Locale",
                         &NumErrors );


    //
    //  Null Pointers.
    //

    //  Variation 1  -  lpLCData = NULL
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SLIST,
                         NULL );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "lpLCData NULL",
                         &NumErrors );


    //
    //  Zero or Invalid Type.
    //

    //  Variation 1  -  LCType = invalid
    rc = SetLocaleInfoW( Locale,
                         LCTYPE_INVALID,
                         L"," );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "LCType invalid",
                         &NumErrors );

    //  Variation 2  -  LCType = 0
    rc = SetLocaleInfoW( Locale,
                         0,
                         L"," );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "LCType zero",
                         &NumErrors );

    //  Variation 1  -  Use CP ACP, LCType = invalid
    rc = SetLocaleInfoW( Locale,
                         LOCALE_USE_CP_ACP | LCTYPE_INVALID,
                         L"," );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "Use CP ACP, LCType invalid",
                         &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  SLI_NormalCase
//
//  This routine tests the normal cases of the API routine.
//
//  07-14-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int SLI_NormalCase()
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
    rc = SetLocaleInfoW( LOCALE_SYSTEM_DEFAULT,
                         LOCALE_SLIST,
                         L"::" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - system default locale",
                      &NumErrors );
    rc = GetLocaleInfoW( LOCALE_SYSTEM_DEFAULT,
                         LOCALE_SLIST,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"::",
                       "GET - system default locale",
                       &NumErrors );

    //  Variation 2  -  Current User Locale
    rc = SetLocaleInfoW( LOCALE_USER_DEFAULT,
                         LOCALE_SLIST,
                         L";;" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - current user locale",
                      &NumErrors );
    rc = GetLocaleInfoW( LOCALE_USER_DEFAULT,
                         LOCALE_SLIST,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L";;",
                       "GET - current user locale",
                       &NumErrors );

    //
    //  Use CP ACP.
    //

    //  Variation 1  -  Use CP ACP, System Default Locale
    rc = SetLocaleInfoW( LOCALE_SYSTEM_DEFAULT,
                         LOCALE_USE_CP_ACP | LOCALE_SLIST,
                         L".." );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - Use CP ACP, system default locale",
                      &NumErrors );
    rc = GetLocaleInfoW( LOCALE_SYSTEM_DEFAULT,
                         LOCALE_USE_CP_ACP | LOCALE_SLIST,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"..",
                       "GET - Use CP ACP, system default locale",
                       &NumErrors );

    //
    //  LCTYPE values.
    //

    //  Variation 1  -  SLIST
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SLIST,
                         L"::::" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - MAX SLIST",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SLIST,
                         L"''" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - SLIST",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_SLIST,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"''",
                       "GET - SLIST",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SLIST,
                         pSList );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - SLIST",
                      &NumErrors );


    //  Variation 2  -  IMEASURE
    rc = GetLocaleInfoW( Locale,
                         LOCALE_IMEASURE,
                         pTemp,
                         BUFSIZE );
    CheckReturnEqual( rc,
                      0,
                      "GET - current IMEASURE",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_IMEASURE,
                         L"2" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid IMEASURE",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_IMEASURE,
                         L"0" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - IMEASURE",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_IMEASURE,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"0",
                       "GET - IMEASURE",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_IMEASURE,
                         pTemp );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - IMEASURE",
                      &NumErrors );


    //  Variation 3  -  SDECIMAL
    rc = GetLocaleInfoW( Locale,
                         LOCALE_SDECIMAL,
                         pTemp,
                         BUFSIZE );
    CheckReturnEqual( rc,
                      0,
                      "GET - current SDECIMAL",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SDECIMAL,
                         L"[][]" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid SDECIMAL",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SDECIMAL,
                         L"6" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid SDECIMAL (6)",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SDECIMAL,
                         L"{" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - SDECIMAL",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_SDECIMAL,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"{",
                       "GET - SDECIMAL",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SDECIMAL,
                         pTemp );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - SDECIMAL",
                      &NumErrors );


    //  Variation 4  -  STHOUSAND
    rc = GetLocaleInfoW( Locale,
                         LOCALE_STHOUSAND,
                         pTemp,
                         BUFSIZE );
    CheckReturnEqual( rc,
                      0,
                      "GET - current STHOUSAND",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_STHOUSAND,
                         L"[][]" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid STHOUSAND",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_STHOUSAND,
                         L"6" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid STHOUSAND (6)",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_STHOUSAND,
                         L"{" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - STHOUSAND",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_STHOUSAND,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"{",
                       "GET - STHOUSAND",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_STHOUSAND,
                         pTemp );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - STHOUSAND",
                      &NumErrors );


    //  Variation 5  -  SGROUPING
    rc = GetLocaleInfoW( Locale,
                         LOCALE_SGROUPING,
                         pTemp,
                         BUFSIZE );
    CheckReturnEqual( rc,
                      0,
                      "GET - current SGROUPING",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SGROUPING,
                         L"3;" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid SGROUPING 1",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SGROUPING,
                         L"3;2;" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid SGROUPING 2",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SGROUPING,
                         L"10;0" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid SGROUPING 3",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SGROUPING,
                         L"3:0" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid SGROUPING 4",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SGROUPING,
                         L"5;0" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - SGROUPING",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_SGROUPING,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"5;0",
                       "GET - SGROUPING",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SGROUPING,
                         L"3;2;0" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - SGROUPING",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_SGROUPING,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"3;2;0",
                       "GET - SGROUPING",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SGROUPING,
                         pTemp );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - SGROUPING",
                      &NumErrors );


    //  Variation 6  -  IDIGITS
    rc = GetLocaleInfoW( Locale,
                         LOCALE_IDIGITS,
                         pTemp,
                         BUFSIZE );
    CheckReturnEqual( rc,
                      0,
                      "GET - current IDIGITS",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_IDIGITS,
                         L"a" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid IDIGITS",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_IDIGITS,
                         L"5" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - IDIGITS",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_IDIGITS,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"5",
                       "GET - IDIGITS",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_IDIGITS,
                         pTemp );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - IDIGITS",
                      &NumErrors );


    //  Variation 7  -  ILZERO
    rc = GetLocaleInfoW( Locale,
                         LOCALE_ILZERO,
                         pTemp,
                         BUFSIZE );
    CheckReturnEqual( rc,
                      0,
                      "GET - current ILZERO",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_ILZERO,
                         L"2" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid ILZERO",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_ILZERO,
                         L"1" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - ILZERO",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_ILZERO,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"1",
                       "GET - ILZERO",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_ILZERO,
                         pTemp );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - ILZERO",
                      &NumErrors );


    //  Variation 8  -  SCURRENCY
    rc = GetLocaleInfoW( Locale,
                         LOCALE_SCURRENCY,
                         pTemp,
                         BUFSIZE );
    CheckReturnEqual( rc,
                      0,
                      "GET - current SCURRENCY",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SCURRENCY,
                         L"[][][]" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid SCURRENCY",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SCURRENCY,
                         L"x5" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid SCURRENCY (x5)",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SCURRENCY,
                         L"%@%" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - SCURRENCY",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_SCURRENCY,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"%@%",
                       "GET - SCURRENCY",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SCURRENCY,
                         pTemp );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - SCURRENCY",
                      &NumErrors );


    //  Variation 9  -  SMONDECIMALSEP
    rc = GetLocaleInfoW( Locale,
                         LOCALE_SMONDECIMALSEP,
                         pTemp,
                         BUFSIZE );
    CheckReturnEqual( rc,
                      0,
                      "GET - current SMONDECIMALSEP",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SMONDECIMALSEP,
                         L"{}{}" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid SMONDECIMALSEP",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SMONDECIMALSEP,
                         L"6" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid SMONDECIMALSEP (6)",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SMONDECIMALSEP,
                         L"%" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - SMONDECIMALSEP",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_SMONDECIMALSEP,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"%",
                       "GET - SMONDECIMALSEP",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SMONDECIMALSEP,
                         pTemp );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - SMONDECIMALSEP",
                      &NumErrors );


    //  Variation 10  -  SMONTHOUSANDSEP
    rc = GetLocaleInfoW( Locale,
                         LOCALE_SMONTHOUSANDSEP,
                         pTemp,
                         BUFSIZE );
    CheckReturnEqual( rc,
                      0,
                      "GET - current SMONTHOUSANDSEP",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SMONTHOUSANDSEP,
                         L"{}{}" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid SMONTHOUSANDSEP",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SMONTHOUSANDSEP,
                         L"6" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid SMONTHOUSANDSEP (6)",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SMONTHOUSANDSEP,
                         L"%" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - SMONTHOUSANDSEP",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_SMONTHOUSANDSEP,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"%",
                       "GET - SMONTHOUSANDSEP",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SMONTHOUSANDSEP,
                         pTemp );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - SMONTHOUSANDSEP",
                      &NumErrors );


    //  Variation 11  -  SMONGROUPING
    rc = GetLocaleInfoW( Locale,
                         LOCALE_SMONGROUPING,
                         pTemp,
                         BUFSIZE );
    CheckReturnEqual( rc,
                      0,
                      "GET - current SMONGROUPING",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SMONGROUPING,
                         L"3;" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid SMONGROUPING 1",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SMONGROUPING,
                         L"3;2;" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid SMONGROUPING 2",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SMONGROUPING,
                         L"10;0" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid SMONGROUPING 3",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SMONGROUPING,
                         L"3:0" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid SMONGROUPING 4",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SMONGROUPING,
                         L"5;0" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - SMONGROUPING",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_SMONGROUPING,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"5;0",
                       "GET - SMONGROUPING",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SMONGROUPING,
                         L"3;2;0" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - SMONGROUPING",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_SMONGROUPING,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"3;2;0",
                       "GET - SMONGROUPING",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SMONGROUPING,
                         pTemp );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - SMONGROUPING",
                      &NumErrors );


    //  Variation 12  -  ICURRDIGITS
    rc = GetLocaleInfoW( Locale,
                         LOCALE_ICURRDIGITS,
                         pTemp,
                         BUFSIZE );
    CheckReturnEqual( rc,
                      0,
                      "GET - current ICURRDIGITS",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_ICURRDIGITS,
                         L"aa" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid ICURRDIGITS",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_ICURRDIGITS,
                         L"85" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - ICURRDIGITS",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_ICURRDIGITS,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"85",
                       "GET - ICURRDIGITS",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_ICURRDIGITS,
                         pTemp );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - ICURRDIGITS",
                      &NumErrors );


    //  Variation 13  -  ICURRENCY
    rc = GetLocaleInfoW( Locale,
                         LOCALE_ICURRENCY,
                         pTemp,
                         BUFSIZE );
    CheckReturnEqual( rc,
                      0,
                      "GET - current ICURRENCY",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_ICURRENCY,
                         L"4" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid ICURRENCY",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_ICURRENCY,
                         L"3" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - ICURRENCY",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_ICURRENCY,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"3",
                       "GET - ICURRENCY",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_ICURRENCY,
                         pTemp );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - ICURRENCY",
                      &NumErrors );


    //  Variation 14  -  INEGCURR
    rc = GetLocaleInfoW( Locale,
                         LOCALE_INEGCURR,
                         pTemp,
                         BUFSIZE );
    CheckReturnEqual( rc,
                      0,
                      "GET - current INEGCURR",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_INEGCURR,
                         L"16" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid INEGCURR",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_INEGCURR,
                         L"13" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - INEGCURR",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_INEGCURR,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"13",
                       "GET - INEGCURR",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_INEGCURR,
                         pTemp );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - INEGCURR",
                      &NumErrors );


    //  Variation 15  -  SPOSITIVESIGN
    rc = GetLocaleInfoW( Locale,
                         LOCALE_SPOSITIVESIGN,
                         pTemp,
                         BUFSIZE );
    CheckReturnEqual( rc,
                      0,
                      "GET - current SPOSITIVESIGN",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SPOSITIVESIGN,
                         L"{}{}{" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid SPOSITIVESIGN",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SPOSITIVESIGN,
                         L"x5" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid SPOSITIVESIGN (x5)",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SPOSITIVESIGN,
                         L"[]" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - SPOSITIVESIGN",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_SPOSITIVESIGN,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"[]",
                       "GET - SPOSITIVESIGN",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SPOSITIVESIGN,
                         pTemp );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - SPOSITIVESIGN",
                      &NumErrors );


    //  Variation 16  -  SNEGATIVESIGN
    rc = GetLocaleInfoW( Locale,
                         LOCALE_SNEGATIVESIGN,
                         pTemp,
                         BUFSIZE );
    CheckReturnEqual( rc,
                      0,
                      "GET - SNEGATIVESIGN",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SNEGATIVESIGN,
                         L"{}{}{" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid SNEGATIVESIGN",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SNEGATIVESIGN,
                         L"x5" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid SNEGATIVESIGN (x5)",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SNEGATIVESIGN,
                         L"[]" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - SNEGATIVESIGN",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_SNEGATIVESIGN,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"[]",
                       "GET - SNEGATIVESIGN",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SNEGATIVESIGN,
                         pTemp );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - SNEGATIVESIGN",
                      &NumErrors );


    //  Variation 17  -  STIMEFORMAT
    rc = SetLocaleInfoW( Locale,
                         LOCALE_STIMEFORMAT,
                         L"HHHHHmmmmmsssssHHHHHmmmmmsssssHHHHHmmmmmsssssHHHHHmmmmmsssssHHHHHmmmmmsssssHHHHH" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid STIMEFORMAT",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_STIMEFORMAT,
                         L"tt HH/mm/ss" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - STIMEFORMAT",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_STIMEFORMAT,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"tt HH/mm/ss",
                       "GET - STIMEFORMAT",
                       &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_STIME,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"/",
                       "GET - STIME from STIMEFORMAT",
                       &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_ITIME,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"1",
                       "GET - ITIME from STIMEFORMAT",
                       &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_ITLZERO,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"1",
                       "GET - ITLZERO from STIMEFORMAT",
                       &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_ITIMEMARKPOSN,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"1",
                       "GET - ITIMEMARKPOSN from STIMEFORMAT",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_STIMEFORMAT,
                         pSTimeFormat );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - STIMEFORMAT",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_STIME,
                         pSTime );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - STIME from STIMEFORMAT",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_ITIME,
                         pITime );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - ITIME from STIMEFORMAT",
                      &NumErrors );


    //  Variation 18  -  STIME
    rc = SetLocaleInfoW( Locale,
                         LOCALE_STIME,
                         L"{**}" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid STIME",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_STIME,
                         L"x5" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid STIME (x5)",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_STIMEFORMAT,
                         L"HH/mm/ss" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - STIMEFORMAT from STIME",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_STIME,
                         L"[]" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - STIME",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_STIME,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"[]",
                       "GET - STIME",
                       &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_STIMEFORMAT,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"HH[]mm[]ss",
                       "GET - STIMEFORMAT from STIME",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_STIME,
                         pSTime );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - STIME",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_STIMEFORMAT,
                         pSTimeFormat );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - STIMEFORMAT from STIME",
                      &NumErrors );


    //  Variation 18.1  -  STIME
    rc = SetLocaleInfoW( Locale,
                         LOCALE_STIMEFORMAT,
                         L"HH/mm/ss' ('hh' oclock)'" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - STIMEFORMAT from STIME",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_STIME,
                         L"[]" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - STIME",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_STIME,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"[]",
                       "GET - STIME",
                       &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_STIMEFORMAT,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"HH[]mm[]ss' ('hh' oclock)'",
                       "GET - STIMEFORMAT from STIME",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_STIME,
                         pSTime );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - STIME",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_STIMEFORMAT,
                         pSTimeFormat );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - STIMEFORMAT from STIME",
                      &NumErrors );


    //  Variation 19  -  ITIME
    rc = SetLocaleInfoW( Locale,
                         LOCALE_ITIME,
                         L"2" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid ITIME",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_STIMEFORMAT,
                         L"hh/mm/ss" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - STIMEFORMAT from ITIME",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_ITIME,
                         L"1" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - ITIME",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_ITIME,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"1",
                       "GET - ITIME",
                       &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_STIMEFORMAT,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"HH/mm/ss",
                       "GET - STIMEFORMAT from ITIME",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_ITIME,
                         pITime );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - ITIME",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_STIMEFORMAT,
                         pSTimeFormat );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - STIMEFORMAT from ITIME",
                      &NumErrors );


    //  Variation 20  -  S1159
    rc = GetLocaleInfoW( Locale,
                         LOCALE_S1159,
                         pTemp,
                         BUFSIZE );
    CheckReturnEqual( rc,
                      0,
                      "GET - current S1159",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_S1159,
                         L"123456789" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid S1159",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_S1159,
                         L"DAWN" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - S1159",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_S1159,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"DAWN",
                       "GET - S1159",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_S1159,
                         pTemp );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - S1159",
                      &NumErrors );


    //  Variation 21  -  S2359
    rc = GetLocaleInfoW( Locale,
                         LOCALE_S2359,
                         pTemp,
                         BUFSIZE );
    CheckReturnEqual( rc,
                      0,
                      "GET - S2359",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_S2359,
                         L"123456789" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid S2359",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_S2359,
                         L"DUSK" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - S2359",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_S2359,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"DUSK",
                       "GET - S2359",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_S2359,
                         pTemp );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - S2359",
                      &NumErrors );


    //  Variation 22  -  SSHORTDATE
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SSHORTDATE,
                         L"dddddMMMMMyyyyydddddMMMMMyyyyydddddMMMMMyyyyydddddMMMMMyyyyydddddMMMMMyyyyyddddd" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid SSHORTDATE",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SSHORTDATE,
                         L"yyyy:MM:dd" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - SSHORTDATE",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_SSHORTDATE,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"yyyy:MM:dd",
                       "GET - SSHORTDATE",
                       &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_SDATE,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L":",
                       "GET - SDATE from SSHORTDATE",
                       &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_IDATE,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"2",
                       "GET - IDATE from SSHORTDATE",
                       &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_IMONLZERO,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"1",
                       "GET - IMONLZERO from SSHORTDATE",
                       &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_IDAYLZERO,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"1",
                       "GET - IDAYLZERO from SSHORTDATE",
                       &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_ICENTURY,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"1",
                       "GET - ICENTURY from SSHORTDATE",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SSHORTDATE,
                         pSShortDate );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - SSHORTDATE",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SDATE,
                         pSDate );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - SDATE from SSHORTDATE",
                      &NumErrors );


    //  Variation 23  -  SDATE
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SDATE,
                         L"{}{}" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid SDATE",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SDATE,
                         L"6" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid SDATE (6)",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SSHORTDATE,
                         L"yy:MM:dd" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - SSHORTDATE from SDATE",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SDATE,
                         L"+" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - SDATE",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_SDATE,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"+",
                       "GET - SDATE",
                       &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_SSHORTDATE,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"yy+MM+dd",
                       "GET - SSHORTDATE from SDATE",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SDATE,
                         pSDate );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - SDATE",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SSHORTDATE,
                         pSShortDate );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - SSHORTDATE from SDATE",
                      &NumErrors );


    //  Variation 23.1  -  SDATE
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SSHORTDATE,
                         L"yy:MM:dd' ('ddd')'" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - SSHORTDATE from SDATE",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SDATE,
                         L"+" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - SDATE",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_SDATE,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"+",
                       "GET - SDATE",
                       &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_SSHORTDATE,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"yy+MM+dd' ('ddd')'",
                       "GET - SSHORTDATE from SDATE",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SDATE,
                         pSDate );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - SDATE",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SSHORTDATE,
                         pSShortDate );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - SSHORTDATE from SDATE",
                      &NumErrors );


    //  Variation 24  -  SLONGDATE
    rc = GetLocaleInfoW( Locale,
                         LOCALE_SLONGDATE,
                         pTemp,
                         BUFSIZE );
    CheckReturnEqual( rc,
                      0,
                      "GET - current SLONGDATE",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SLONGDATE,
                         L"dddddMMMMMyyyyydddddMMMMMyyyyydddddMMMMMyyyyydddddMMMMMyyyyydddddMMMMMyyyyyddddd" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid SLONGDATE",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SLONGDATE,
                         L"yy, MMMM dd, dddd" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - SLONGDATE",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_SLONGDATE,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"yy, MMMM dd, dddd",
                       "GET - SLONGDATE",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SLONGDATE,
                         pTemp );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - SLONGDATE",
                      &NumErrors );


    //  Variation 25  -  ICALENDARTYPE
    rc = GetLocaleInfoW( Locale,
                         LOCALE_ICALENDARTYPE,
                         pTemp,
                         BUFSIZE );
    CheckReturnEqual( rc,
                      0,
                      "GET - current ICALENDARTYPE",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_ICALENDARTYPE,
                         L"0" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid ICALENDARTYPE 0",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_ICALENDARTYPE,
                         L"8" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid ICALENDARTYPE 8",
                      &NumErrors );
    rc = SetLocaleInfoW( 0x0411,
                         LOCALE_ICALENDARTYPE,
                         L"3" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - ICALENDARTYPE",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_ICALENDARTYPE,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"3",
                       "GET - ICALENDARTYPE",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_ICALENDARTYPE,
                         pTemp );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - ICALENDARTYPE",
                      &NumErrors );


    //  Variation 26  -  IFIRSTDAYOFWEEK
    rc = GetLocaleInfoW( Locale,
                         LOCALE_IFIRSTDAYOFWEEK,
                         pTemp,
                         BUFSIZE );
    CheckReturnEqual( rc,
                      0,
                      "GET - current IFIRSTDAYOFWEEK",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_IFIRSTDAYOFWEEK,
                         L"7" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid IFIRSTDAYOFWEEK",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_IFIRSTDAYOFWEEK,
                         L"3" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - IFIRSTDAYOFWEEK",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_IFIRSTDAYOFWEEK,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"3",
                       "GET - IFIRSTDAYOFWEEK",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_IFIRSTDAYOFWEEK,
                         pTemp );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - IFIRSTDAYOFWEEK",
                      &NumErrors );


    //  Variation 27  -  IFIRSTWEEKOFYEAR
    rc = GetLocaleInfoW( Locale,
                         LOCALE_IFIRSTWEEKOFYEAR,
                         pTemp,
                         BUFSIZE );
    CheckReturnEqual( rc,
                      0,
                      "GET - current IFIRSTWEEKOFYEAR",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_IFIRSTWEEKOFYEAR,
                         L"3" );
    CheckReturnEqual( rc,
                      TRUE,
                      "SET - invalid IFIRSTWEEKOFYEAR",
                      &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_IFIRSTWEEKOFYEAR,
                         L"1" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - IFIRSTWEEKOFYEAR",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_IFIRSTWEEKOFYEAR,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"1",
                       "GET - IFIRSTWEEKOFYEAR",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_IFIRSTWEEKOFYEAR,
                         L"2" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - IFIRSTWEEKOFYEAR 2",
                      &NumErrors );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_IFIRSTWEEKOFYEAR,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"2",
                       "GET - IFIRSTWEEKOFYEAR 2",
                       &NumErrors );
    rc = SetLocaleInfoW( Locale,
                         LOCALE_IFIRSTWEEKOFYEAR,
                         pTemp );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - IFIRSTWEEKOFYEAR",
                      &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  SLI_Ansi
//
//  This routine tests the Ansi version of the API routine.
//
//  07-14-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int SLI_Ansi()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


    //
    //  SetLocaleInfoA.
    //

    //  Variation 1  -  SList
    rc = SetLocaleInfoA( 0x0409,
                         LOCALE_SLIST,
                         "::" );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - system default locale",
                      &NumErrors );
    rc = GetLocaleInfoW( 0x0409,
                         LOCALE_SLIST,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"::",
                       "GET - system default locale",
                       &NumErrors );

    //  Variation 2  -  Use CP ACP, SList
    rc = SetLocaleInfoA( 0x0409,
                         LOCALE_USE_CP_ACP | LOCALE_SLIST,
                         ".." );
    CheckReturnEqual( rc,
                      FALSE,
                      "SET - Use CP ACP, system default locale",
                      &NumErrors );
    rc = GetLocaleInfoW( 0x0409,
                         LOCALE_USE_CP_ACP | LOCALE_SLIST,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"..",
                       "GET - Use CP ACP, system default locale",
                       &NumErrors );


    //
    //  Reset the slist value.
    //
    rc = SetLocaleInfoW( Locale,
                         LOCALE_SLIST,
                         pSList );
    CheckReturnEqual( rc,
                      FALSE,
                      "ReSET - SLIST",
                      &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}
