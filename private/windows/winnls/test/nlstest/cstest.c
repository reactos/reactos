/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    cstest.c

Abstract:

    Test module for NLS API CompareString.

    NOTE: This code was simply hacked together quickly in order to
          test the different code modules of the NLS component.
          This is NOT meant to be a formal regression test.

Revision History:

    06-14-91    JulieB    Created.

--*/



//
//  Include Files.
//

#include "nlstest.h"




//
//  Constant Declarations.
//

#define  BUFSIZE           100              // buffer size in wide chars

#define  CS_INVALID_FLAGS  ((DWORD)(~(NORM_IGNORECASE |                     \
                                      NORM_IGNORENONSPACE |                 \
                                      NORM_IGNORESYMBOLS |                  \
                                      NORM_IGNOREKANATYPE |                 \
                                      NORM_IGNOREWIDTH |                    \
                                      NORM_STOP_ON_NULL)))




//
//  Global Variables.
//

LCID Locale;

#define wCompStr1  L"This Is A String"
#define wCompStr2  L"This Is A String Longer"
#define wCompStr3  L"THIS IS A STRing"
#define wCompStr4  L"This Is$ A String"
#define wCompStr5  L"This Is A Different String"

//  Sharp S
#define wCmpSharpS1     L"t\x00dft"
#define wCmpSharpS2     L"tSSt"
#define wCmpSharpS3     L"tSt"
#define wCmpSharpS4     L"tt"
#define wCmpSharpS5     L"tS"
#define wCmpSharpS6     L"\x00dft"

//  A-acute, E-acute
#define wCmpPre         L"\x00c1\x00c9"
#define wCmpPreLow      L"\x00e1\x00e9"
#define wCmpComp        L"\x0041\x0301\x0045\x0301"
#define wCmpComp2       L"\x0041\x0301\x0045\x0301\x0301"

//  A, E  -  Expansion
#define wCmpExp         L"\x00c6"
#define wCmpExp2        L"ae"

//  Unsortable character in string
#define wCmpUnsort      L"A\xffff\x0301\x0045\x0301"

//  Diacritics and Symbols
#define wCmpDiac1       L"\x00e4.ext"
#define wCmpDiac2       L"a\x00e4.ext"

//  Diacritics Only
#define wCmpDiac3       L"\x00e4"
#define wCmpDiac4       L"a\x00e4"

//  Nonspace
#define wCmpNS1         L"\x0301\x00a2\x0045"
#define wCmpNS2         L"\x00a2\x0045"
#define wCmpNS3         L"\x0301-E"
#define wCmpNS4         L"-E"


//  French Diacritic Sorting
#define wCmpFrench1     L"cot\x00e9"
#define wCmpFrench2     L"c\x00f4te"
#define wCmpFrench3     L"c\x00f4t\x00e9"

//  Danish Compression Sorting
#define wCmpAEMacronLg  L"\x01e2"
#define wCmpAEMacronSm  L"\x01e3"
#define wCmpAELg        L"\x00c6"
#define wCmpAESm        L"\x00e6"




//
//  Forward Declarations.
//

BOOL
InitCompStr();

int
CS_BadParamCheck();

int
CS_NormalCase();

int
CS_Ansi();

void
CheckReturnCompStr(
    int CurrentReturn,
    int ExpectedReturn,
    LPSTR pErrString,
    int *pNumErrors);

void
CompareSortkeyStrings(
    LPBYTE pSort1,
    LPBYTE pSort2,
    int ExpectedReturn,
    LPSTR pErrString,
    int *pNumErrors);

UINT
GetCPFromLocale(
    LCID Locale);

void
CompareStringTester(
    LCID Locale,
    DWORD dwFlags,
    LPWSTR pString1,
    int Count1,
    LPWSTR pString2,
    int Count2,
    int ExpectedReturn,
    LPSTR pErrString,
    BOOL TestAVersion,
    int *pNumErrors);




////////////////////////////////////////////////////////////////////////////
//
//  TestCompareString
//
//  Test routine for CompareStringW API.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int TestCompareString()
{
    int ErrCount = 0;             // error count


    //
    //  Print out what's being done.
    //
    printf("\n\nTESTING CompareStringW...\n\n");

    //
    //  Initialize global variables.
    //
    if (!InitCompStr())
    {
        printf("\nABORTED TestCompareString: Could not Initialize.\n");
        return (1);
    }

    //
    //  Test bad parameters.
    //
    ErrCount += CS_BadParamCheck();

    //
    //  Test normal cases.
    //
    ErrCount += CS_NormalCase();

    //
    //  Test Ansi Version.
    //
    ErrCount += CS_Ansi();

    //
    //  Print out result.
    //
    printf("\nCompareStringW:  ERRORS = %d\n", ErrCount);

    //
    //  Return total number of errors found.
    //
    return (ErrCount);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitCompStr
//
//  This routine initializes the global variables.  If no errors were
//  encountered, then it returns TRUE.  Otherwise, it returns FALSE.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL InitCompStr()
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
//  CS_BadParamCheck
//
//  This routine passes in bad parameters to the API routines and checks to
//  be sure they are handled properly.  The number of errors encountered
//  is returned to the caller.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int CS_BadParamCheck()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


    //
    //  Bad Locale.
    //

    //  Variation 1  -  Bad Locale
    rc = CompareStringW( (LCID)333,
                         0,
                         wCompStr1,
                         -1,
                         wCompStr2,
                         -1 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "Bad Locale",
                         &NumErrors );


    //
    //  Null Pointers.
    //

    //  Variation 1  -  lpString1 = NULL
    rc = CompareStringW( Locale,
                         0,
                         NULL,
                         -1,
                         wCompStr2,
                         -1 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "lpString1 NULL",
                         &NumErrors );

    //  Variation 2  -  lpString2 = NULL
    rc = CompareStringW( Locale,
                         0,
                         wCompStr1,
                         -1,
                         NULL,
                         -1 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "lpString2 NULL",
                         &NumErrors );


    //
    //  Zero or Invalid Flag Values.
    //

    //  Variation 1  -  dwCmpFlags = invalid
    rc = CompareStringW( Locale,
                         CS_INVALID_FLAGS,
                         wCompStr1,
                         -1,
                         wCompStr2,
                         -1 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "dwCmpFlags invalid",
                         &NumErrors );

    //  Variation 2  -  dwCmpFlags = 0
    rc = CompareStringW( Locale,
                         0,
                         wCompStr1,
                         -1,
                         wCompStr2,
                         -1 );
    CheckReturnCompStr( rc,
                        1,
                        "dwCmpFlags zero",
                        &NumErrors );

    //  Variation 3  -  dwCmpFlags = Use CP ACP
    rc = CompareStringW( Locale,
                         LOCALE_USE_CP_ACP,
                         wCompStr1,
                         -1,
                         wCompStr2,
                         -1 );
    CheckReturnCompStr( rc,
                        1,
                        "Use CP ACP",
                        &NumErrors );


//
//  CompareStringA.
//

    //
    //  Bad Locale.
    //

    //  Variation 1  -  Bad Locale
    rc = CompareStringA( (LCID)333,
                         0,
                         "foo",
                         -1,
                         "foo",
                         -1 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "A version - Bad Locale",
                         &NumErrors );


    //
    //  Null Pointers.
    //

    //  Variation 1  -  lpString1 = NULL
    rc = CompareStringA( Locale,
                         0,
                         NULL,
                         -1,
                         "foo",
                         -1 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "A version - lpString1 NULL",
                         &NumErrors );

    //  Variation 2  -  lpString2 = NULL
    rc = CompareStringA( Locale,
                         0,
                         "foo",
                         -1,
                         NULL,
                         -1 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "A version - lpString2 NULL",
                         &NumErrors );


    //
    //  Zero or Invalid Flag Values.
    //

    //  Variation 1  -  dwCmpFlags = invalid
    rc = CompareStringA( Locale,
                         CS_INVALID_FLAGS,
                         "foo",
                         -1,
                         "foo",
                         -1 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "A version - dwCmpFlags invalid",
                         &NumErrors );

    //  Variation 2  -  dwCmpFlags = 0
    rc = CompareStringA( Locale,
                         0,
                         "foo",
                         -1,
                         "foo",
                         -1 );
    CheckReturnCompStr( rc,
                        2,
                        "A version - dwCmpFlags zero",
                        &NumErrors );

    //  Variation 3  -  dwCmpFlags = Use CP ACP
    rc = CompareStringA( Locale,
                         LOCALE_USE_CP_ACP,
                         "foo",
                         -1,
                         "foo",
                         -1 );
    CheckReturnCompStr( rc,
                        2,
                        "A version - Use CP ACP",
                        &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  CS_NormalCase
//
//  This routine tests the normal cases of the API routine.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int CS_NormalCase()
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
    CompareStringTester( LOCALE_SYSTEM_DEFAULT,
                         0,
                         wCompStr1,
                         -1,
                         wCompStr1,
                         -1,
                         2,
                         "System Default Locale",
                         TRUE,
                         &NumErrors );

    //  Variation 2  -  Current User Locale
    CompareStringTester( LOCALE_USER_DEFAULT,
                         0,
                         wCompStr1,
                         -1,
                         wCompStr1,
                         -1,
                         2,
                         "Current User Locale",
                         TRUE,
                         &NumErrors );


    //
    //  Equal Strings.
    //

    //  Variation 1  -  equal strings
    CompareStringTester( Locale,
                         0,
                         wCompStr1,
                         -1,
                         wCompStr1,
                         -1,
                         2,
                         "equal strings",
                         TRUE,
                         &NumErrors );

    //
    //  nCounts.
    //

    //  Variation 1  -  nCount1 = 0
    rc = CompareStringW( Locale,
                         0,
                         wCompStr1,
                         0,
                         wCompStr2,
                         -1 );
    CheckReturnCompStr( rc,
                         1,
                         "nCount1 = 0",
                         &NumErrors );

    //  Variation 2  -  nCount2 = 0
    rc = CompareStringW( Locale,
                         0,
                         wCompStr1,
                         -1,
                         wCompStr2,
                         0 );
    CheckReturnCompStr( rc,
                         3,
                         "nCount2 = 0",
                         &NumErrors );

    //  Variation 3  -  nCount1 and nCount2 = 0
    rc = CompareStringW( Locale,
                         0,
                         wCompStr1,
                         0,
                         wCompStr2,
                         0 );
    CheckReturnCompStr( rc,
                         2,
                         "nCount1 and nCount2 = 0",
                         &NumErrors );

    //  Variation 4  -  counts = -1
    CompareStringTester( Locale,
                         0,
                         wCompStr1,
                         -1,
                         wCompStr1,
                         -1,
                         2,
                         "counts (-1)",
                         TRUE,
                         &NumErrors );

    //  Variation 5  -  counts = value
    CompareStringTester( Locale,
                         0,
                         wCompStr1,
                         WC_STRING_LEN(wCompStr1),
                         wCompStr1,
                         WC_STRING_LEN(wCompStr1),
                         2,
                         "counts (value)",
                         TRUE,
                         &NumErrors );

    //  Variation 6  -  count1 bigger
    CompareStringTester( Locale,
                         0,
                         wCompStr1,
                         WC_STRING_LEN(wCompStr1),
                         wCompStr1,
                         WC_STRING_LEN(wCompStr1) - 2,
                         3,
                         "count1 bigger",
                         TRUE,
                         &NumErrors );

    //  Variation 7  -  count2 bigger
    CompareStringTester( Locale,
                         0,
                         wCompStr1,
                         WC_STRING_LEN(wCompStr1) - 2,
                         wCompStr1,
                         WC_STRING_LEN(wCompStr1),
                         1,
                         "count2 bigger",
                         TRUE,
                         &NumErrors );

    //  Variation 8  -  count1 ONE bigger
    CompareStringTester( Locale,
                         0,
                         wCompStr1,
                         WC_STRING_LEN(wCompStr1),
                         wCompStr1,
                         WC_STRING_LEN(wCompStr1) - 1,
                         3,
                         "count1 ONE bigger",
                         TRUE,
                         &NumErrors );

    //  Variation 9  -  count2 ONE bigger
    CompareStringTester( Locale,
                         0,
                         wCompStr1,
                         WC_STRING_LEN(wCompStr1) - 1,
                         wCompStr1,
                         WC_STRING_LEN(wCompStr1),
                         1,
                         "count2 ONE bigger",
                         TRUE,
                         &NumErrors );


    //
    //  Longer Strings.
    //

    //  Variation 1  -  string1 longer
    CompareStringTester( Locale,
                         0,
                         wCompStr2,
                         -1,
                         wCompStr1,
                         -1,
                         3,
                         "string1 longer",
                         TRUE,
                         &NumErrors );

    //  Variation 2  -  string2 longer
    CompareStringTester( Locale,
                         0,
                         wCompStr1,
                         -1,
                         wCompStr2,
                         -1,
                         1,
                         "string2 longer",
                         TRUE,
                         &NumErrors );


    //
    //  Flags.
    //

    //  Variation 0  -  Use CP ACP
    CompareStringTester( Locale,
                         LOCALE_USE_CP_ACP | NORM_IGNORECASE,
                         wCompStr1,
                         -1,
                         wCompStr3,
                         -1,
                         2,
                         "Use CP ACP",
                         TRUE,
                         &NumErrors );

    //  Variation 1  -  upper/lower case
    CompareStringTester( Locale,
                         NORM_IGNORECASE,
                         wCompStr1,
                         -1,
                         wCompStr3,
                         -1,
                         2,
                         "upper/lower case",
                         TRUE,
                         &NumErrors );

    //  Variation 2  -  symbols
    CompareStringTester( Locale,
                         NORM_IGNORESYMBOLS,
                         wCompStr1,
                         -1,
                         wCompStr4,
                         -1,
                         2,
                         "symbols",
                         TRUE,
                         &NumErrors );

    //  Variation 3  -  case and symbols
    CompareStringTester( Locale,
                         NORM_IGNORECASE | NORM_IGNORESYMBOLS,
                         wCompStr3,
                         -1,
                         wCompStr4,
                         -1,
                         2,
                         "case and symbols",
                         TRUE,
                         &NumErrors );

    //  Variation 4  -  different strings w/ case
    CompareStringTester( Locale,
                         NORM_IGNORECASE,
                         wCompStr4,
                         -1,
                         wCompStr5,
                         -1,
                         3,
                         "different case",
                         TRUE,
                         &NumErrors );

    //  Variation 5  -  ignore case, sharp S
    CompareStringTester( Locale,
                         NORM_IGNORECASE,
                         wCmpSharpS1,
                         -1,
                         wCmpSharpS2,
                         -1,
                         2,
                         "ignore case, sharp S",
                         TRUE,
                         &NumErrors );

    //  Variation 6  -  ignore case, sharp S diff
    CompareStringTester( Locale,
                         NORM_IGNORECASE,
                         wCmpSharpS1,
                         -1,
                         wCmpSharpS3,
                         -1,
                         1,
                         "ignore case, sharp S diff",
                         TRUE,
                         &NumErrors );

    //  Variation 7  -  ignore case, sharp S diff2
    CompareStringTester( Locale,
                         NORM_IGNORECASE,
                         wCmpSharpS1,
                         -1,
                         wCmpSharpS4,
                         -1,
                         1,
                         "ignore case, sharp S diff2",
                         TRUE,
                         &NumErrors );

    //  Variation 8  -  ignore case, sharp S diff3
    CompareStringTester( Locale,
                         NORM_IGNORECASE,
                         wCmpSharpS1,
                         -1,
                         wCmpSharpS5,
                         -1,
                         3,
                         "ignore case, sharp S diff3",
                         TRUE,
                         &NumErrors );

    //  Variation 9  -  stop on null
    rc = CompareStringW( Locale,
                         NORM_STOP_ON_NULL,
                         L"ab\x0000yy",
                         5,
                         L"ab\x0000zz",
                         5 );
    CheckReturnCompStr( rc,
                         2,
                         "stop on null - 1",
                         &NumErrors );

    //  Variation 10  -  stop on null
    rc = CompareStringW( Locale,
                         NORM_STOP_ON_NULL,
                         L"ab\x0000yy",
                         -1,
                         L"ab\x0000zz",
                         -1 );
    CheckReturnCompStr( rc,
                         2,
                         "stop on null - 2",
                         &NumErrors );

    //  Variation 11  -  stop on null
    rc = CompareStringW( Locale,
                         NORM_STOP_ON_NULL,
                         L"ab\x0000yy",
                         5,
                         L"ab\x0000zz",
                         -1 );
    CheckReturnCompStr( rc,
                         2,
                         "stop on null - 3",
                         &NumErrors );

    //  Variation 12  -  stop on null
    rc = CompareStringW( Locale,
                         NORM_STOP_ON_NULL,
                         L"ab\x0000yy",
                         -1,
                         L"ab\x0000zz",
                         5 );
    CheckReturnCompStr( rc,
                         2,
                         "stop on null - 4",
                         &NumErrors );

    //  Variation 13  -  stop on null
    rc = CompareStringW( Locale,
                         NORM_STOP_ON_NULL,
                         L"abyyy",
                         -1,
                         L"abyyy",
                         -1 );
    CheckReturnCompStr( rc,
                         2,
                         "stop on null - 5",
                         &NumErrors );

    //  Variation 14  -  stop on null
    rc = CompareStringW( Locale,
                         NORM_STOP_ON_NULL,
                         L"abyyy",
                         5,
                         L"abyyy",
                         5 );
    CheckReturnCompStr( rc,
                         2,
                         "stop on null - 6",
                         &NumErrors );

    //  Variation 15  -  stop on null
    rc = CompareStringW( Locale,
                         NORM_STOP_ON_NULL,
                         L"ab\x0000yy",
                         5,
                         L"abyyy",
                         5 );
    CheckReturnCompStr( rc,
                         1,
                         "stop on null - 7",
                         &NumErrors );




    //
    //  Different Locale
    //

    //  Variation 1  -  locale 040c
    CompareStringTester( MAKELCID(0x040c, 0),
                         0,
                         wCompStr1,
                         -1,
                         wCompStr1,
                         -1,
                         2,
                         "locale 040c - equal strings",
                         TRUE,
                         &NumErrors );

    //  Variation 2  -  locale 040c again
    CompareStringTester( MAKELCID(0x040c, 0),
                         0,
                         wCompStr1,
                         -1,
                         wCompStr1,
                         -1,
                         2,
                         "locale 040c again - equal strings",
                         TRUE,
                         &NumErrors );


    //
    //  Various Checks for Sorting.
    //

    // Variation 1 - Expansion first char
    CompareStringTester( Locale,
                         NORM_IGNORECASE,
                         wCmpSharpS6,
                         -1,
                         L"SST",
                         -1,
                         2,
                         "expansion 1st char",
                         TRUE,
                         &NumErrors );

    // Variation 2 - Comp vs. Precomp
    CompareStringTester( Locale,
                         0,
                         wCmpPre,
                         -1,
                         wCmpComp,
                         -1,
                         2,
                         "comp vs precomp",
                         FALSE,
                         &NumErrors );

    // Variation 3 - Comp vs. Precomp (ignore nonspace)
    CompareStringTester( Locale,
                         NORM_IGNORENONSPACE,
                         wCmpPre,
                         -1,
                         wCmpComp2,
                         -1,
                         2,
                         "comp vs precomp (ignore nonspace)",
                         FALSE,
                         &NumErrors );

    // Variation 4 - Comp vs. Precomp (ignore nonspace - diff)
    CompareStringTester( Locale,
                         NORM_IGNORENONSPACE,
                         wCmpPre,
                         -1,
                         wCmpComp2,
                         -1,
                         2,
                         "comp vs precomp (ignore nonspace - diff)",
                         FALSE,
                         &NumErrors );

    // Variation 5 - Comp vs. Precomp (diff)
    CompareStringTester( Locale,
                         0,
                         wCmpPre,
                         -1,
                         wCmpComp2,
                         -1,
                         1,
                         "comp vs precomp (diff)",
                         FALSE,
                         &NumErrors );

    // Variation 6 - Comp vs. Precomp (ignore case)
    CompareStringTester( Locale,
                         NORM_IGNORECASE,
                         wCmpPreLow,
                         -1,
                         wCmpComp,
                         -1,
                         2,
                         "comp vs precomp (ignore case)",
                         FALSE,
                         &NumErrors );

    // Variation 7 - Expansion (ignore case)
    CompareStringTester( Locale,
                         NORM_IGNORECASE,
                         wCmpExp,
                         -1,
                         wCmpExp2,
                         -1,
                         2,
                         "expansion (ignore case)",
                         TRUE,
                         &NumErrors );

    // Variation 8 - Expansion (ignore nonspace)
    CompareStringTester( Locale,
                         NORM_IGNORENONSPACE,
                         wCmpExp,
                         -1,
                         wCmpComp,
                         -1,
                         2,
                         "expansion (ignore nonspace)",
                         FALSE,
                         &NumErrors );

    // Variation 9 - Unsortable with Precomp
    CompareStringTester( Locale,
                         0,
                         wCmpUnsort,
                         -1,
                         wCmpComp,
                         -1,
                         2,
                         "unsortable with precomp",
                         FALSE,
                         &NumErrors );

    // Variation 10 - Extra symbols
    CompareStringTester( Locale,
                         NORM_IGNORESYMBOLS,
                         L"T*est",
                         -1,
                         L"Test*$@!",
                         -1,
                         2,
                         "extra symbols",
                         TRUE,
                         &NumErrors );

    // Variation 11 - Comp vs. Precomp (ignore case, nonspace)
    CompareStringTester( Locale,
                         NORM_IGNORECASE | NORM_IGNORENONSPACE,
                         wCmpPreLow,
                         -1,
                         L"AE",
                         -1,
                         2,
                         "comp vs precomp (ignore case, nonspace)",
                         TRUE,
                         &NumErrors );

    // Variation 12 - Comp vs. Precomp (ignore case, nonspace, symbol)
    CompareStringTester( Locale,
                         NORM_IGNORECASE | NORM_IGNORENONSPACE | NORM_IGNORESYMBOLS,
                         wCmpPreLow,
                         -1,
                         L"A$E",
                         -1,
                         2,
                         "comp vs precomp (ignore case, nonspace, symbol)",
                         TRUE,
                         &NumErrors );

    // Variation 13 - Expansion (1 to 3)
    CompareStringTester( Locale,
                         0,
                         L"ffi",
                         -1,
                         L"\xfb03",
                         -1,
                         2,
                         "expansion (1 to 3)",
                         FALSE,
                         &NumErrors );

    // Variation 14 - Expansion (1 to 3)
    CompareStringTester( Locale,
                         0,
                         L"\xfb03",
                         -1,
                         L"ffi",
                         -1,
                         2,
                         "expansion (1 to 3) 2",
                         FALSE,
                         &NumErrors );

    // Variation 15 - Expansion (1 to 3)
    CompareStringTester( Locale,
                         0,
                         L"\xfb03\x0061",
                         -1,
                         L"ffia",
                         -1,
                         2,
                         "expansion (1 to 3) 3",
                         FALSE,
                         &NumErrors );

    // Variation 16 - Expansion (1 to 3)
    CompareStringTester( Locale,
                         0,
                         L"ffia",
                         -1,
                         L"\xfb03\x0061",
                         -1,
                         2,
                         "expansion (1 to 3) 4",
                         FALSE,
                         &NumErrors );

    // Variation 17 - Expansion (1 to 3)
    CompareStringTester( Locale,
                         NORM_IGNORECASE,
                         L"ffiA",
                         -1,
                         L"\xfb03\x0061",
                         -1,
                         2,
                         "expansion (1 to 3) 5",
                         FALSE,
                         &NumErrors );

    // Variation 18 - Expansion (1 to 3)
    CompareStringTester( Locale,
                         NORM_IGNORECASE,
                         L"ffia",
                         -1,
                         L"\xfb03\x0041",
                         -1,
                         2,
                         "expansion (1 to 3) 6",
                         FALSE,
                         &NumErrors );


    //
    //  Symbol checks.
    //

    // Variation 1 - video vs vid_all
    CompareStringTester( Locale,
                         0,
                         L"video",
                         -1,
                         L"vid_all",
                         -1,
                         3,
                         "video vs vid_all",
                         TRUE,
                         &NumErrors );

    // Variation 2 - video vs vid_all (case)
    CompareStringTester( Locale,
                         NORM_IGNORECASE,
                         L"video",
                         -1,
                         L"vid_all",
                         -1,
                         3,
                         "video vs vid_all (case)",
                         TRUE,
                         &NumErrors );

    // Variation 3 - symbol first
    CompareStringTester( Locale,
                         0,
                         L"{Other}",
                         -1,
                         L"Novell",
                         -1,
                         1,
                         "symbol first",
                         TRUE,
                         &NumErrors );


    //
    //  Compression.
    //

    // Variation 1 - Spanish Castilian ch
    CompareStringTester( MAKELCID(0x040a, 0),
                         0,
                         L"ch",
                         -1,
                         L"cz",
                         -1,
                         3,
                         "Spanish Castilian ch vs cz",
                         TRUE,
                         &NumErrors );

    // Variation 2 - Spanish Castilian ll
    CompareStringTester( MAKELCID(0x040a, 0),
                         0,
                         L"ll",
                         -1,
                         L"lz",
                         -1,
                         3,
                         "Spanish Castilian ll vs lz",
                         TRUE,
                         &NumErrors );

    // Variation 5 - Spanish Modern ch   --  ch does NOT sort after cz
    CompareStringTester( MAKELCID(0x0c0a, 0),
                         0,
                         L"ch",
                         -1,
                         L"cz",
                         -1,
                         1,
                         "Spanish Modern ch vs cz",
                         TRUE,
                         &NumErrors );

    // Variation 6 - Spanish Modern ll
    CompareStringTester( MAKELCID(0x0c0a, 0),
                         0,
                         L"ll",
                         -1,
                         L"lz",
                         -1,
                         1,
                         "Spanish Modern ll vs lz",
                         TRUE,
                         &NumErrors );

    // Variation 7 - Hungarian ccs
    CompareStringTester( MAKELCID(0x040e, 0),
                         0,
                         L"ccs",
                         -1,
                         L"cscs",
                         -1,
                         2,
                         "Hungarian ccs",
                         TRUE,
                         &NumErrors );

    // Variation 8 - Hungarian ddz
    CompareStringTester( MAKELCID(0x040e, 0),
                         0,
                         L"ddz",
                         -1,
                         L"dzdz",
                         -1,
                         2,
                         "Hungarian ddz",
                         TRUE,
                         &NumErrors );

    // Variation 9 - Hungarian ddzs
    CompareStringTester( MAKELCID(0x040e, 0),
                         0,
                         L"ddzs",
                         -1,
                         L"dzsdzs",
                         -1,
                         2,
                         "Hungarian ddzs",
                         TRUE,
                         &NumErrors );

    // Variation 10 - Danish aa vs. zoo
    CompareStringTester( MAKELCID(0x0406, 0),
                         0,
                         L"aa",
                         -1,
                         L"zoo",
                         -1,
                         3,
                         "Danish aa and zoo",
                         TRUE,
                         &NumErrors );

    // Variation 11 - Danish a$$a vs. zoo
    CompareStringTester( MAKELCID(0x0406, 0),
                         0,
                         L"a$$a",
                         -1,
                         L"zoo",
                         -1,
                         1,
                         "Danish a$$a and zoo",
                         TRUE,
                         &NumErrors );

    // Variation 12 - Danish a$$a vs. zoo
    CompareStringTester( MAKELCID(0x0406, 0),
                         NORM_IGNORESYMBOLS,
                         L"a$$a",
                         -1,
                         L"zoo",
                         -1,
                         1,
                         "Danish a$$a and zoo (ignore symbols)",
                         TRUE,
                         &NumErrors );



    //
    //  Compression - case differences.
    //

    // Variation 1 - Spanish Ch and cz
    CompareStringTester( MAKELCID(0x040a, 0),
                         0,
                         L"Ch",
                         -1,
                         L"cz",
                         -1,
                         3,
                         "Spanish Ch and cz",
                         TRUE,
                         &NumErrors );

    // Variation 2 - Spanish Ch and ch
    CompareStringTester( MAKELCID(0x040a, 0),
                         0,
                         L"Ch",
                         -1,
                         L"ch",
                         -1,
                         3,
                         "Spanish Ch and ch",
                         TRUE,
                         &NumErrors );

    // Variation 3 - Spanish CH and ch
    CompareStringTester( MAKELCID(0x040a, 0),
                         0,
                         L"CH",
                         -1,
                         L"ch",
                         -1,
                         3,
                         "Spanish CH and ch",
                         TRUE,
                         &NumErrors );

    // Variation 4 - Spanish Ch and cH
    CompareStringTester( MAKELCID(0x040a, 0),
                         0,
                         L"Ch",
                         -1,
                         L"cH",
                         -1,
                         3,
                         "Spanish Ch and cH",
                         TRUE,
                         &NumErrors );

    // Variation 5 - Spanish ch and cH
    CompareStringTester( MAKELCID(0x040a, 0),
                         0,
                         L"ch",
                         -1,
                         L"cH",
                         -1,
                         3,
                         "Spanish ch and cH",
                         TRUE,
                         &NumErrors );

    // Variation 6 - Spanish CH and cH
    CompareStringTester( MAKELCID(0x040a, 0),
                         0,
                         L"CH",
                         -1,
                         L"cH",
                         -1,
                         3,
                         "Spanish CH and cH",
                         TRUE,
                         &NumErrors );

    // Variation 7 - Spanish ch and cZ
    CompareStringTester( MAKELCID(0x040a, 0),
                         0,
                         L"ch",
                         -1,
                         L"cZ",
                         -1,
                         3,
                         "Spanish ch and cZ",
                         TRUE,
                         &NumErrors );

    // Variation 8 - Spanish Castilian Ll and lz
    CompareStringTester( MAKELCID(0x040a, 0),
                         0,
                         L"Ll",
                         -1,
                         L"lz",
                         -1,
                         3,
                         "Spanish Castilian Ll vs lz",
                         TRUE,
                         &NumErrors );

    // Variation 9 - Hungarian ccs and csCs
    CompareStringTester( MAKELCID(0x040e, 0),
                         NORM_IGNORECASE,
                         L"ccs",
                         -1,
                         L"csCs",
                         -1,
                         2,
                         "Hungarian ccs and csCs",
                         TRUE,
                         &NumErrors );

    // Variation 10 - Hungarian ccs and Cscs
    CompareStringTester( MAKELCID(0x040e, 0),
                         NORM_IGNORECASE,
                         L"ccs",
                         -1,
                         L"Cscs",
                         -1,
                         2,
                         "Hungarian ccs and Cscs",
                         TRUE,
                         &NumErrors );

    // Variation 11 - Hungarian tty and tTy
    CompareStringTester( 0x040e,
                         0,
                         L"tty",
                         -1,
                         L"tTy",
                         -1,
                         1,
                         "Hungarian tty vs tTy",
                         TRUE,
                         &NumErrors );

    // Variation 12 - Hungarian tty and tTy
    CompareStringTester( 0x040e,
                         NORM_IGNORECASE,
                         L"tty",
                         -1,
                         L"tTy",
                         -1,
                         2,
                         "Hungarian tty vs tTy - ignore case",
                         TRUE,
                         &NumErrors );

    // Variation 13 - Danish aA vs. zoo
    CompareStringTester( MAKELCID(0x0406, 0),
                         0,
                         L"aA",
                         -1,
                         L"zoo",
                         -1,
                         1,
                         "Danish aA and zoo",
                         TRUE,
                         &NumErrors );


    //
    //  Ignore Case Compression
    //

    // Variation 1 - Spanish ch and cH
    CompareStringTester( MAKELCID(0x040a, 0),
                         NORM_IGNORECASE,
                         L"ch",
                         -1,
                         L"cH",
                         -1,
                         3,
                         "Spanish (case) ch and cH",
                         TRUE,
                         &NumErrors );

    // Variation 2 - Spanish ch and Ch
    CompareStringTester( MAKELCID(0x040a, 0),
                         NORM_IGNORECASE,
                         L"ch",
                         -1,
                         L"Ch",
                         -1,
                         2,
                         "Spanish (case) ch and Ch",
                         TRUE,
                         &NumErrors );

    // Variation 3 - Spanish ch and CH
    CompareStringTester( MAKELCID(0x040a, 0),
                         NORM_IGNORECASE,
                         L"ch",
                         -1,
                         L"CH",
                         -1,
                         2,
                         "Spanish (case) ch and CH",
                         TRUE,
                         &NumErrors );

    // Variation 4 - Spanish CH and cH
    CompareStringTester( MAKELCID(0x040a, 0),
                         NORM_IGNORECASE,
                         L"CH",
                         -1,
                         L"cH",
                         -1,
                         3,
                         "Spanish (case) CH and cH",
                         TRUE,
                         &NumErrors );

    // Variation 5 - Spanish CH and Ch
    CompareStringTester( MAKELCID(0x040a, 0),
                         NORM_IGNORECASE,
                         L"CH",
                         -1,
                         L"Ch",
                         -1,
                         2,
                         "Spanish (case) CH and Ch",
                         TRUE,
                         &NumErrors );

    // Variation 6 - Spanish CH and ch
    CompareStringTester( MAKELCID(0x040a, 0),
                         NORM_IGNORECASE,
                         L"CH",
                         -1,
                         L"ch",
                         -1,
                         2,
                         "Spanish (case) CH and ch",
                         TRUE,
                         &NumErrors );


    //
    //  Check ae macron for Danish.
    //

    // Variation 1 - Danish AE macron vs. AA
    CompareStringTester( MAKELCID(0x0406, 0),
                         0,
                         wCmpAEMacronLg,
                         -1,
                         L"AA",
                         -1,
                         1,
                         "Danish AE macron and AA",
                         FALSE,
                         &NumErrors );

    // Variation 2 - Danish ae macron vs. AA
    CompareStringTester( MAKELCID(0x0406, 0),
                         0,
                         wCmpAEMacronSm,
                         -1,
                         L"AA",
                         -1,
                         1,
                         "Danish ae macron and AA",
                         FALSE,
                         &NumErrors );

    // Variation 3 - Danish AE macron vs. aa
    CompareStringTester( MAKELCID(0x0406, 0),
                         0,
                         wCmpAEMacronLg,
                         -1,
                         L"aa",
                         -1,
                         1,
                         "Danish AE macron and aa",
                         FALSE,
                         &NumErrors );

    // Variation 4 - Danish ae macron vs. aa
    CompareStringTester( MAKELCID(0x0406, 0),
                         0,
                         wCmpAEMacronSm,
                         -1,
                         L"aa",
                         -1,
                         1,
                         "Danish ae macron and aa",
                         FALSE,
                         &NumErrors );

    // Variation 5 - Danish AE macron vs. Z
    CompareStringTester( MAKELCID(0x0406, 0),
                         0,
                         wCmpAEMacronLg,
                         -1,
                         L"Z",
                         -1,
                         3,
                         "Danish AE macron and Z",
                         FALSE,
                         &NumErrors );

    // Variation 6 - Danish ae macron vs. Z
    CompareStringTester( MAKELCID(0x0406, 0),
                         0,
                         wCmpAEMacronSm,
                         -1,
                         L"Z",
                         -1,
                         3,
                         "Danish ae macron and Z",
                         FALSE,
                         &NumErrors );


    //
    //  Check ae for Danish.
    //

    // Variation 1 - Danish AE vs. AA
    CompareStringTester( MAKELCID(0x0406, 0),
                         0,
                         wCmpAELg,
                         -1,
                         L"AA",
                         -1,
                         1,
                         "Danish AE and AA",
                         TRUE,
                         &NumErrors );

    // Variation 2 - Danish ae vs. AA
    CompareStringTester( MAKELCID(0x0406, 0),
                         0,
                         wCmpAESm,
                         -1,
                         L"AA",
                         -1,
                         1,
                         "Danish ae and AA",
                         TRUE,
                         &NumErrors );

    // Variation 3 - Danish AE vs. aa
    CompareStringTester( MAKELCID(0x0406, 0),
                         0,
                         wCmpAELg,
                         -1,
                         L"aa",
                         -1,
                         1,
                         "Danish AE and aa",
                         TRUE,
                         &NumErrors );

    // Variation 4 - Danish ae vs. aa
    CompareStringTester( MAKELCID(0x0406, 0),
                         0,
                         wCmpAESm,
                         -1,
                         L"aa",
                         -1,
                         1,
                         "Danish ae and aa",
                         TRUE,
                         &NumErrors );

    // Variation 5 - Danish AE vs. Z
    CompareStringTester( MAKELCID(0x0406, 0),
                         0,
                         wCmpAELg,
                         -1,
                         L"Z",
                         -1,
                         3,
                         "Danish AE and Z",
                         TRUE,
                         &NumErrors );

    // Variation 6 - Danish ae vs. Z
    CompareStringTester( MAKELCID(0x0406, 0),
                         0,
                         wCmpAESm,
                         -1,
                         L"Z",
                         -1,
                         3,
                         "Danish ae and Z",
                         TRUE,
                         &NumErrors );


    //
    //  Check compression for Danish.
    //

    // Variation 1 - Danish aa vs. B
    CompareStringTester( MAKELCID(0x0406, 0),
                         0,
                         L"aa",
                         -1,
                         L"B",
                         -1,
                         3,
                         "Danish aa and B",
                         TRUE,
                         &NumErrors );

    // Variation 2 - Danish B vs. aa
    CompareStringTester( MAKELCID(0x0406, 0),
                         0,
                         L"B",
                         -1,
                         L"aa",
                         -1,
                         1,
                         "Danish B and aa",
                         TRUE,
                         &NumErrors );

    // Variation 3 - Danish aa vs. z
    CompareStringTester( MAKELCID(0x0406, 0),
                         0,
                         L"aa",
                         -1,
                         L"z",
                         -1,
                         3,
                         "Danish aa and z",
                         TRUE,
                         &NumErrors );

    // Variation 4 - Danish üb vs. ya
    CompareStringTester( MAKELCID(0x0406, 0),
                         0,
                         L"üb",
                         -1,
                         L"ya",
                         -1,
                         3,
                         "Danish üb and ya",
                         TRUE,
                         &NumErrors );

    // Variation 5 - Danish  ya vs. üb
    CompareStringTester( MAKELCID(0x0406, 0),
                         0,
                         L"ya",
                         -1,
                         L"üb",
                         -1,
                         1,
                         "Danish ya and üb",
                         TRUE,
                         &NumErrors );


    //
    //  Numeric check.
    //

    //  Variation 1 - numeric check
    CompareStringTester( Locale,
                         0,
                         L"1.ext",
                         -1,
                         L"a.ext",
                         -1,
                         1,
                         "numeric check",
                         TRUE,
                         &NumErrors );


    //
    //  Check diacritics and symbols.
    //

    //  Variation 1
    CompareStringTester( Locale,
                         0,
                         L"a.ext",
                         -1,
                         wCmpDiac1,
                         -1,
                         1,
                         "diacritic check 1",
                         TRUE,
                         &NumErrors );

    //  Variation 2
    CompareStringTester( Locale,
                         0,
                         L"a.ext",
                         -1,
                         wCmpDiac2,
                         -1,
                         1,
                         "diacritic check 2",
                         TRUE,
                         &NumErrors );

    //  Variation 3
    CompareStringTester( Locale,
                         0,
                         wCmpDiac1,
                         -1,
                         wCmpDiac2,
                         -1,
                         1,
                         "diacritic check 3",
                         TRUE,
                         &NumErrors );

    //  Variation 4
    CompareStringTester( Locale,
                         0,
                         wCmpDiac2,
                         -1,
                         L"az.ext",
                         -1,
                         1,
                         "diacritic check 4",
                         TRUE,
                         &NumErrors );


    //
    //  Check diacritics only.
    //

    //  Variation 5
    CompareStringTester( Locale,
                         0,
                         L"a",
                         -1,
                         wCmpDiac3,
                         -1,
                         1,
                         "diacritic check 5",
                         TRUE,
                         &NumErrors );

    //  Variation 6
    CompareStringTester( Locale,
                         0,
                         L"a",
                         -1,
                         wCmpDiac4,
                         -1,
                         1,
                         "diacritic check 6",
                         TRUE,
                         &NumErrors );

    //  Variation 7
    CompareStringTester( Locale,
                         0,
                         wCmpDiac3,
                         -1,
                         wCmpDiac4,
                         -1,
                         1,
                         "diacritic check 7",
                         TRUE,
                         &NumErrors );

    //  Variation 8
    CompareStringTester( Locale,
                         0,
                         wCmpDiac4,
                         -1,
                         L"az",
                         -1,
                         1,
                         "diacritic check 8",
                         TRUE,
                         &NumErrors );


    //
    //  Check Punctuation.
    //

    //  Variation 1
    CompareStringTester( Locale,
                         0,
                         L"coop",
                         -1,
                         L"co-op",
                         -1,
                         1,
                         "punctuation check 1",
                         TRUE,
                         &NumErrors );

    //  Variation 2
    CompareStringTester( Locale,
                         0,
                         L"co-op",
                         -1,
                         L"cop",
                         -1,
                         1,
                         "punctuation check 2",
                         TRUE,
                         &NumErrors );

    //  Variation 3
    CompareStringTester( Locale,
                         0,
                         L"co-op",
                         -1,
                         L"coop",
                         -1,
                         3,
                         "punctuation check 3",
                         TRUE,
                         &NumErrors );

    //  Variation 4
    CompareStringTester( Locale,
                         0,
                         L"cop",
                         -1,
                         L"co-op",
                         -1,
                         3,
                         "punctuation check 4",
                         TRUE,
                         &NumErrors );

    //  Variation 5
    CompareStringTester( 0x0409,
                         1,
                         L"A-ANNA",
                         -1,
                         L"A'JEFF",
                         -1,
                         1,
                         "hyphen, apostrophe",
                         FALSE,
                         &NumErrors );

    //  Variation 6
    CompareStringTester( 0x0409,
                         1,
                         L"A'JEFF",
                         -1,
                         L"A-ANNA",
                         -1,
                         3,
                         "apostrophe, hyphen",
                         FALSE,
                         &NumErrors );



    //
    //  Check Punctuation - STRING SORT.
    //

    //  Variation 1
    CompareStringTester( Locale,
                         SORT_STRINGSORT,
                         L"coop",
                         -1,
                         L"co-op",
                         -1,
                         3,
                         "string sort punctuation check 1",
                         TRUE,
                         &NumErrors );

    //  Variation 2
    CompareStringTester( Locale,
                         SORT_STRINGSORT,
                         L"co-op",
                         -1,
                         L"cop",
                         -1,
                         1,
                         "string sort punctuation check 2",
                         TRUE,
                         &NumErrors );

    //  Variation 3
    CompareStringTester( Locale,
                         SORT_STRINGSORT,
                         L"co-op",
                         -1,
                         L"coop",
                         -1,
                         1,
                         "string sort punctuation check 3",
                         TRUE,
                         &NumErrors );

    //  Variation 4
    CompareStringTester( Locale,
                         SORT_STRINGSORT,
                         L"cop",
                         -1,
                         L"co-op",
                         -1,
                         3,
                         "string sort punctuation check 4",
                         TRUE,
                         &NumErrors );

    //  Variation 5
    CompareStringTester( Locale,
                         SORT_STRINGSORT,
                         L"cop",
                         -1,
                         L"coop",
                         -1,
                         3,
                         "string sort punctuation check 5",
                         TRUE,
                         &NumErrors );

    //  Variation 6
    CompareStringTester( Locale,
                         SORT_STRINGSORT,
                         L"cop",
                         3,
                         L"coop",
                         3,
                         3,
                         "string sort punctuation check 6",
                         TRUE,
                         &NumErrors );

    //  Variation 7
    CompareStringTester( Locale,
                         SORT_STRINGSORT,
                         L"a-coo",
                         -1,
                         L"a_coo",
                         -1,
                         1,
                         "string sort hyphen vs. unserscore 1",
                         TRUE,
                         &NumErrors );

    //  Variation 8
    CompareStringTester( Locale,
                         0,
                         L"a-coo",
                         -1,
                         L"a_coo",
                         -1,
                         3,
                         "string sort hyphen vs. unserscore 2",
                         TRUE,
                         &NumErrors );

    //  Variation 9
    CompareStringTester( Locale,
                         SORT_STRINGSORT,
                         L"a-coo",
                         5,
                         L"a_coo",
                         5,
                         1,
                         "string sort hyphen vs. unserscore 3",
                         TRUE,
                         &NumErrors );

    //  Variation 10
    CompareStringTester( Locale,
                         0,
                         L"a-coo",
                         5,
                         L"a_coo",
                         5,
                         3,
                         "string sort hyphen vs. unserscore 4",
                         TRUE,
                         &NumErrors );

    //  Variation 11
    CompareStringTester( Locale,
                         SORT_STRINGSORT | NORM_IGNORECASE,
                         L"AdministratorAdministrative User Accountdomain...",
                         0xb,
                         L"MpPwd223304MpPwd2233305MpPwd223306...",
                         0xb,
                         1,
                         "test for net stuff",
                         TRUE,
                         &NumErrors );


    //
    //  Check diacritic followed by nonspace - NORM_IGNORENONSPACE.
    //

    //  Variation 1
    CompareStringTester( Locale,
                         NORM_IGNORENONSPACE,
                         wCmpNS1,
                         -1,
                         wCmpNS2,
                         -1,
                         2,
                         "diacritic, ignorenonspace, symbol",
                         FALSE,
                         &NumErrors );

    //  Variation 2
    CompareStringTester( Locale,
                         NORM_IGNORENONSPACE,
                         wCmpNS3,
                         -1,
                         wCmpNS4,
                         -1,
                         2,
                         "diacritic, ignorenonspace, punct 1",
                         FALSE,
                         &NumErrors );

    //  Variation 3
    CompareStringTester( Locale,
                         NORM_IGNORENONSPACE,
                         wCmpNS4,
                         -1,
                         wCmpNS3,
                         -1,
                         2,
                         "diacritic, ignorenonspace, punct 2",
                         FALSE,
                         &NumErrors );



    //
    //  Check French vs. English diacritic sorting.
    //

    //  Variation 1
    CompareStringTester( Locale,
                         0,
                         wCmpFrench1,
                         -1,
                         wCmpFrench2,
                         -1,
                         1,
                         "English diacritic sort 1",
                         TRUE,
                         &NumErrors );

    //  Variation 2
    CompareStringTester( Locale,
                         0,
                         wCmpFrench2,
                         -1,
                         wCmpFrench3,
                         -1,
                         1,
                         "English diacritic sort 2",
                         TRUE,
                         &NumErrors );

    //  Variation 3
    CompareStringTester( Locale,
                         0,
                         wCmpFrench1,
                         -1,
                         wCmpFrench3,
                         -1,
                         1,
                         "English diacritic sort 3",
                         TRUE,
                         &NumErrors );

    //  Variation 4
    CompareStringTester( MAKELCID(0x040C, 0),
                         0,
                         wCmpFrench1,
                         -1,
                         wCmpFrench2,
                         -1,
                         3,
                         "French diacritic sort 1",
                         TRUE,
                         &NumErrors );

    //  Variation 5
    CompareStringTester( MAKELCID(0x040C, 0),
                         0,
                         wCmpFrench2,
                         -1,
                         wCmpFrench3,
                         -1,
                         1,
                         "French diacritic sort 2",
                         TRUE,
                         &NumErrors );

    //  Variation 6
    CompareStringTester( MAKELCID(0x040C, 0),
                         0,
                         wCmpFrench1,
                         -1,
                         wCmpFrench3,
                         -1,
                         1,
                         "French diacritic sort 3",
                         TRUE,
                         &NumErrors );



    //
    //  Diacritic tests.
    //

    //   Variation 1
    CompareStringTester( Locale,
                         0,
                         L"\x00c4\x00c4",
                         -1,
                         L"\x00c4\x0041\x0308",
                         -1,
                         2,
                         "Diacritic - A diaeresis",
                         FALSE,
                         &NumErrors );

    //   Variation 2
    CompareStringTester( Locale,
                         0,
                         L"\x00c4\x00c4",
                         2,
                         L"\x00c4\x0041\x0308",
                         3,
                         2,
                         "Diacritic - A diaeresis (size)",
                         FALSE,
                         &NumErrors );

    //   Variation 3
    CompareStringTester( Locale,
                         0,
                         L"\x00c4\x01e0",
                         -1,
                         L"\x0041\x0308\x0041\x0307\x0304",
                         -1,
                         2,
                         "Diacritic - A dot macron",
                         FALSE,
                         &NumErrors );

    //   Variation 4
    CompareStringTester( Locale,
                         0,
                         L"\x00c4\x01e0",
                         2,
                         L"\x0041\x0308\x0041\x0307\x0304",
                         5,
                         2,
                         "Diacritic - A dot macron (size)",
                         FALSE,
                         &NumErrors );




    //
    //  Kana and Width Flags.
    //

    //  Variation 1  -  ignore kana
    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNOREKANATYPE,
                         L"\x30b1\x30b2",
                         -1,
                         L"\x3051\x3052",
                         -1,
                         2,
                         "ignore kana (equal)",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNOREKANATYPE,
                         L"\x30b1\x30b2",
                         2,
                         L"\x3051\x3052",
                         2,
                         2,
                         "ignore kana (equal) (size)",
                         TRUE,
                         &NumErrors );

    //  Variation 2  -  ignore kana
    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNOREKANATYPE,
                         L"\x30b1\x30b2",
                         -1,
                         L"\x3051\x3051",
                         -1,
                         3,
                         "ignore kana (not equal)",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNOREKANATYPE,
                         L"\x30b1\x30b2",
                         2,
                         L"\x3051\x3051",
                         2,
                         3,
                         "ignore kana (not equal) (size)",
                         TRUE,
                         &NumErrors );

    //  Variation 3  -  ignore width
    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNOREWIDTH,
                         L"\x30b1\x30b2",
                         -1,
                         L"\xff79\x30b2",
                         -1,
                         2,
                         "ignore width (equal)",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNOREWIDTH,
                         L"\x30b1\x30b2",
                         2,
                         L"\xff79\x30b2",
                         2,
                         2,
                         "ignore width (equal) (size)",
                         TRUE,
                         &NumErrors );

    //  Variation 4  -  ignore width
    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNOREWIDTH,
                         L"\x30b1\x30b2",
                         -1,
                         L"\xff7a\x30b2",
                         -1,
                         1,
                         "ignore width (not equal)",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNOREWIDTH,
                         L"\x30b1\x30b2",
                         2,
                         L"\xff7a\x30b2",
                         2,
                         1,
                         "ignore width (not equal) (size)",
                         TRUE,
                         &NumErrors );

    //  Variation 5  -  ignore kana, width
    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNOREWIDTH | NORM_IGNOREKANATYPE,
                         L"\x30b1\x30b2\xff76",
                         -1,
                         L"\xff79\x3052\x304b",
                         -1,
                         2,
                         "ignore kana width (equal)",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNOREWIDTH | NORM_IGNOREKANATYPE,
                         L"\x30b1\x30b2\xff76",
                         2,
                         L"\xff79\x3052\x304b",
                         2,
                         2,
                         "ignore kana width (equal) (size)",
                         TRUE,
                         &NumErrors );

    //  Variation 6  -  ignore kana, width
    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNOREWIDTH | NORM_IGNOREKANATYPE,
                         L"\x30b1\x30b2\xff77",
                         -1,
                         L"\xff79\x3052\x304b",
                         -1,
                         3,
                         "ignore kana width (not equal)",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNOREWIDTH | NORM_IGNOREKANATYPE,
                         L"\x30b1\x30b2\xff77",
                         3,
                         L"\xff79\x3052\x304b",
                         3,
                         3,
                         "ignore kana width (not equal) (size)",
                         TRUE,
                         &NumErrors );



    //
    //  Japanese tests - REPEAT.
    //

    // Variation 1 - repeat
    CompareStringTester( MAKELCID(0x0411, 0),
                         0,
                         L"\x30f1\x30fd",
                         -1,
                         L"\x30f1\x30f1",
                         -1,
                         3,
                         "repeat same",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         0,
                         L"\x30f1\x30f1",
                         -1,
                         L"\x30f1\x30fd",
                         -1,
                         1,
                         "repeat same 2",
                         TRUE,
                         &NumErrors );

    // Variation 2 - repeat (1st char)
    CompareStringTester( MAKELCID(0x0411, 0),
                         0,
                         L"\x30fd\x30f1\x30fd",
                         -1,
                         L"\x30f1\x30f1",
                         -1,
                         3,
                         "repeat (1st char)",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         0,
                         L"\x30f1\x30f1",
                         -1,
                         L"\x30fd\x30f1\x30fd",
                         -1,
                         1,
                         "repeat (1st char) 2",
                         TRUE,
                         &NumErrors );

    // Variation 3 - repeat
    CompareStringTester( MAKELCID(0x0411, 0),
                         0,
                         L"\x30f1\x30fd",
                         2,
                         L"\x30f1\x30f1",
                         2,
                         3,
                         "repeat same (size)",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         0,
                         L"\x30f1\x30f1",
                         2,
                         L"\x30f1\x30fd",
                         2,
                         1,
                         "repeat same (size) 2",
                         TRUE,
                         &NumErrors );

    // Variation 4 - repeat (1st char)
    CompareStringTester( MAKELCID(0x0411, 0),
                         0,
                         L"\x30fd\x30f1\x30fd",
                         3,
                         L"\x30f1\x30f1",
                         2,
                         3,
                         "repeat same (1st char) (size)",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         0,
                         L"\x30f1\x30f1",
                         2,
                         L"\x30fd\x30f1\x30fd",
                         3,
                         1,
                         "repeat same (1st char) (size) 2",
                         TRUE,
                         &NumErrors );

    // Variation 5 - repeat (twice)
    CompareStringTester( MAKELCID(0x0411, 0),
                         0,
                         L"\x30f1\x30fd\x30fd",
                         -1,
                         L"\x30f1\x30f1\x30f1",
                         -1,
                         3,
                         "repeat (twice)",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         0,
                         L"\x30f1\x30f1\x30f1",
                         -1,
                         L"\x30f1\x30fd\x30fd",
                         -1,
                         1,
                         "repeat (twice) 2",
                         TRUE,
                         &NumErrors );

    // Variation 6 - repeat (unsortable)
    CompareStringTester( MAKELCID(0x0411, 0),
                         0,
                         L"\x30f1\x30fd\xfffe\x30fd",
                         -1,
                         L"\x30f1\x30f1\x30f1",
                         -1,
                         3,
                         "repeat (unsortable)",
                         FALSE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         0,
                         L"\x30f1\x30f1\x30f1",
                         -1,
                         L"\x30f1\x30fd\xfffe\x30fd",
                         -1,
                         1,
                         "repeat (unsortable) 2",
                         FALSE,
                         &NumErrors );

    // Variation 7 - repeat (unsortable)
    CompareStringTester( MAKELCID(0x0411, 0),
                         0,
                         L"\x30f1\x30fd\xfffe\x30fd",
                         4,
                         L"\x30f1\x30f1\x30f1",
                         3,
                         3,
                         "repeat (unsortable) (size)",
                         FALSE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         0,
                         L"\x30f1\x30f1\x30f1",
                         3,
                         L"\x30f1\x30fd\xfffe\x30fd",
                         4,
                         1,
                         "repeat (unsortable) (size) 2",
                         FALSE,
                         &NumErrors );



    //
    //  Japanese tests - REPEAT with NORM_IGNORENONSPACE.
    //

    // Variation 1 - repeat
    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORENONSPACE,
                         L"\x30ab\x30fe",
                         -1,
                         L"\x30ab\x30ab",
                         -1,
                         2,
                         "repeat same (ignore nonspace)",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORENONSPACE,
                         L"\x30ab\x30ab",
                         -1,
                         L"\x30ab\x30fe",
                         -1,
                         2,
                         "repeat same 2 (ignore nonspace)",
                         TRUE,
                         &NumErrors );

    // Variation 2 - repeat
    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORENONSPACE,
                         L"\x30ab\x30fe",
                         2,
                         L"\x30ab\x30ab",
                         2,
                         2,
                         "repeat same (size) (ignore nonspace)",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORENONSPACE,
                         L"\x30ab\x30ab",
                         2,
                         L"\x30ab\x30fe",
                         2,
                         2,
                         "repeat same (size) 2 (ignore nonspace)",
                         TRUE,
                         &NumErrors );

    // Variation 3 - repeat
    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORENONSPACE,
                         L"\xff76\x30fe",
                         -1,
                         L"\xff76\xff76",
                         -1,
                         2,
                         "repeat same half (ignore nonspace)",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORENONSPACE,
                         L"\xff76\xff76",
                         -1,
                         L"\xff76\x30fe",
                         -1,
                         2,
                         "repeat same 2 half (ignore nonspace)",
                         TRUE,
                         &NumErrors );

    // Variation 4 - repeat
    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORENONSPACE,
                         L"\xff76\x30fe",
                         2,
                         L"\xff76\xff76",
                         2,
                         2,
                         "repeat same (size) half (ignore nonspace)",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORENONSPACE,
                         L"\xff76\xff76",
                         2,
                         L"\xff76\x30fe",
                         2,
                         2,
                         "repeat same (size) 2 half (ignore nonspace)",
                         TRUE,
                         &NumErrors );

    // Variation 5 - repeat, vowel
    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORENONSPACE,
                         L"\x30b9\x30ba",
                         -1,
                         L"\x30b9\x30fe",
                         -1,
                         2,
                         "repeat vowel same (ignore nonspace)",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORENONSPACE,
                         L"\x30b9\x30fe",
                         -1,
                         L"\x30b9\x30ba",
                         -1,
                         2,
                         "repeat vowel same 2 (ignore nonspace)",
                         TRUE,
                         &NumErrors );

    // Variation 6 - repeat, vowel
    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORENONSPACE,
                         L"\x30b9\x30ba",
                         2,
                         L"\x30b9\x30fe",
                         2,
                         2,
                         "repeat vowel same (size) (ignore nonspace)",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORENONSPACE,
                         L"\x30b9\x30fe",
                         2,
                         L"\x30b9\x30ba",
                         2,
                         2,
                         "repeat vowel same (size) 2 (ignore nonspace)",
                         TRUE,
                         &NumErrors );





    //
    //  Japanese tests - CHO-ON.
    //

    // Variation 1 - cho-on
    CompareStringTester( MAKELCID(0x0411, 0),
                         0,
                         L"\x30f4\x30fc",
                         -1,
                         L"\x30f4\x30f4",
                         -1,
                         1,
                         "cho-on",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         0,
                         L"\x30f4\x30f4",
                         -1,
                         L"\x30f4\x30fc",
                         -1,
                         3,
                         "cho-on 2",
                         TRUE,
                         &NumErrors );

    // Variation 2 - cho-on (ignore nonspace)
    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORENONSPACE,
                         L"\x30f4\x30fc",
                         -1,
                         L"\x30f4\x30f4",
                         -1,
                         2,
                         "cho-on (ignore nonspace)",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORENONSPACE,
                         L"\x30f4\x30f4",
                         -1,
                         L"\x30f4\x30fc",
                         -1,
                         2,
                         "cho-on 2 (ignore nonspace)",
                         TRUE,
                         &NumErrors );

    // Variation 3 - cho-on, katakana N
    CompareStringTester( MAKELCID(0x0411, 0),
                         0,
                         L"\x30f3\x30fc",
                         -1,
                         L"\x30f3\x30f3",
                         -1,
                         3,
                         "cho-on, katakana N",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         0,
                         L"\x30f3\x30f3",
                         -1,
                         L"\x30f3\x30fc",
                         -1,
                         1,
                         "cho-on 2, katakana N",
                         TRUE,
                         &NumErrors );


    //
    //  Japanese tests - XJIS ordering.
    //
    CompareStringTester( MAKELCID(0x0411, SORT_JAPANESE_XJIS),
                         0,
                         L"\x337d",
                         -1,
                         L"\x337e",
                         -1,
                         3,
                         "XJIS order",
                         TRUE,
                         &NumErrors );


    //
    //  Chinese tests - BIG5 ordering.
    //
    CompareStringTester( MAKELCID(0x0404, SORT_CHINESE_BIG5),
                         0,
                         L"\x632f",
                         -1,
                         L"\x633e",
                         -1,
                         3,
                         "BIG5 order",
                         TRUE,
                         &NumErrors );


    //
    //  Korean tests - KSC ordering.
    //
    CompareStringTester( MAKELCID(0x0412, SORT_KOREAN_KSC),
                         0,
                         L"\x4e00",
                         -1,
                         L"\x4eba",
                         -1,
                         3,
                         "KSC order",
                         FALSE,
                         &NumErrors );


    //
    //  More Japanese Tests.
    //
    CompareStringTester( MAKELCID(0x0411, 0),
                         0,
                         L"\x3042\x309b",
                         -1,
                         L"\x3042\xff9e",
                         -1,
                         2,
                         "Japanese Test 1",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         0,
                         L"\x306f\x309c",
                         -1,
                         L"\x306f\xff9f",
                         -1,
                         2,
                         "Japanese Test 2",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORENONSPACE,
                         L"\xff76\xff9e\xff71",
                         -1,
                         L"\xff76\xff9e\xff70",
                         -1,
                         2,
                         "Japanese Test 3",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORENONSPACE,
                         L"\xff8a\xff9f\xff71",
                         -1,
                         L"\xff8a\xff9f\xff70",
                         -1,
                         2,
                         "Japanese Test 4",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORENONSPACE,
                         L"\xff71\xff71",
                         -1,
                         L"\xff71\x30fd",
                         -1,
                         2,
                         "Japanese Test 5",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORENONSPACE,
                         L"\xff71\xff71",
                         -1,
                         L"\xff71\x309d",
                         -1,
                         2,
                         "Japanese Test 6",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORENONSPACE,
                         L"\xff67\xff71",
                         -1,
                         L"\xff67\x309e",
                         -1,
                         2,
                         "Japanese Test 7",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORENONSPACE,
                         L"\xff76\xff76\xff9e",
                         -1,
                         L"\xff76\x30fe",
                         -1,
                         2,
                         "Japanese Test 8",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORENONSPACE,
                         L"\xff76\xff76\xff9e",
                         -1,
                         L"\xff76\x309e",
                         -1,
                         2,
                         "Japanese Test 9",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNOREWIDTH,
                         L"\xffe3",
                         -1,
                         L"\x007e",
                         -1,
                         3,
                         "Japanese Test 10",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNOREWIDTH,
                         L"\x2018",
                         -1,
                         L"\x0027",
                         -1,
                         3,
                         "Japanese Test 11",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNOREWIDTH,
                         L"\x2019",
                         -1,
                         L"\x0027",
                         -1,
                         3,
                         "Japanese Test 12",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNOREWIDTH,
                         L"\x201c",
                         -1,
                         L"\x0022",
                         -1,
                         3,
                         "Japanese Test 13",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNOREWIDTH,
                         L"\x201d",
                         -1,
                         L"\x0022",
                         -1,
                         3,
                         "Japanese Test 14",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNOREWIDTH,
                         L"\xffe5",
                         -1,
                         L"\x005c",
                         -1,
                         2,
                         "Japanese Test 15",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORESYMBOLS,
                         L"\xff70\x309b",
                         -1,
                         L"\xff70\xff9e",
                         -1,
                         2,
                         "Japanese Test 16",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORESYMBOLS,
                         L"\xff70\x309c",
                         -1,
                         L"\xff70\xff9f",
                         -1,
                         2,
                         "Japanese Test 17",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORESYMBOLS,
                         L"\x3000",
                         -1,
                         L"\x0020",
                         -1,
                         2,
                         "Japanese Test 18",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORESYMBOLS,
                         L"\x3001",
                         -1,
                         L"\xff64",
                         -1,
                         2,
                         "Japanese Test 19",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORESYMBOLS,
                         L"\x3002",
                         -1,
                         L"\xff61",
                         -1,
                         2,
                         "Japanese Test 20",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNOREWIDTH | NORM_IGNORENONSPACE,
                         L"\x30ac\x30a2",
                         -1,
                         L"\xff76\xff9e\xff70",
                         -1,
                         2,
                         "Japanese Test 21",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNOREWIDTH | NORM_IGNORENONSPACE | NORM_IGNOREKANATYPE,
                         L"\x30ac\x30a2",
                         -1,
                         L"\xff76\xff9e\xff70",
                         -1,
                         2,
                         "Japanese Test 22",
                         TRUE,
                         &NumErrors );


    //
    //  New Japanese Tests.
    //

    CompareStringTester( MAKELCID(0x0411, 0),
                         0,
                         L"\x30cf\x30fc\x30c8",
                         -1,
                         L"\x30cf\x30a2\x30c8",
                         -1,
                         3,
                         "Cho-On test - 1",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         0,
                         L"\x30cf\x30fc\x30c8",
                         -1,
                         L"\x30cf\x30a2\x30c9",
                         -1,
                         1,
                         "Cho-On & Handaku/Daku-On - 2",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         0,
                         L"\x3066\x3063",
                         -1,
                         L"\x3066\x3064",
                         -1,
                         1,
                         "BreathStop - 3",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         0,
                         L"\x3066\x3063\x3071",
                         -1,
                         L"\x3066\x3064\x3070",
                         -1,
                         3,
                         "BreathStop & Handaku/Daku-On - 4",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         0,
                         L"\x30a2\x30a2",
                         -1,
                         L"\xff71\x3042",
                         -1,
                         1,
                         "Halfwidth & Hiragana/Katakana - 5",
                         TRUE,
                         &NumErrors );

    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORESYMBOLS,
                         L"\x3001",
                         -1,
                         L"\xff64",
                         -1,
                         2,
                         "ignore kana test - 6",
                         TRUE,
                         &NumErrors );




    CompareStringTester( MAKELCID(0x0411, 0),
                         0,
                         L"\x3042\x309b",
                         -1,
                         L"\x3042\xff9e",
                         -1,
                         2,
                         "DakuOn (table) - 7",
                         TRUE,
                         &NumErrors );


    CompareStringTester( MAKELCID(0x0411, 0),
                         0,
                         L"\x3042\x309c",
                         -1,
                         L"\x3042\xff9f",
                         -1,
                         2,
                         "HanDakuOn (table) - 8",
                         TRUE,
                         &NumErrors );


    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORENONSPACE,
                         L"\x3041",
                         -1,
                         L"\x3042",
                         -1,
                         2,
                         "IgnoreNonSpace - 9",
                         TRUE,
                         &NumErrors );


    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORECASE | NORM_IGNOREWIDTH,
                         L"\xffe3",
                         -1,
                         L"\x007e",
                         -1,
                         3,
                         "IgnoreCaseWidth - 10",
                         TRUE,
                         &NumErrors );


    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORECASE | NORM_IGNOREWIDTH,
                         L"\xff5e",
                         -1,
                         L"\x007e",
                         -1,
                         2,
                         "IgnoreCaseWidth - 11",
                         TRUE,
                         &NumErrors );


    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORECASE,
                         L"\xff71\xff70",
                         -1,
                         L"\x30a2\x2015",
                         -1,
                         1,
                         "IgnoreCase - 12",
                         TRUE,
                         &NumErrors );


    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH,
                         L"\xff71\xff70",
                         -1,
                         L"\x30a2\x2015",
                         -1,
                         2,
                         "IgnoreKanaWidth - 13",
                         TRUE,
                         &NumErrors );


    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH,
                         L"\xff71\xff70",
                         -1,
                         L"\x30a2\x30fc",
                         -1,
                         2,
                         "IgnoreKanaWidth - 14",
                         TRUE,
                         &NumErrors );


    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORESYMBOLS,
                         L"\xff70\x309b",
                         -1,
                         L"\xff70\xff9e",
                         -1,
                         2,
                         "IgnoreSymbol (table) - 15",
                         TRUE,
                         &NumErrors );


    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORENONSPACE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH,
                         L"\x3041",
                         -1,
                         L"\x3042",
                         -1,
                         2,
                         "IgnoreWidthNonspaceKana - 16",
                         TRUE,
                         &NumErrors );


    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNOREWIDTH,
                         L"\xff67\xff70",
                         -1,
                         L"\x30a1\x30fc",
                         -1,
                         2,
                         "IgnoreWidth - 17",
                         TRUE,
                         &NumErrors );


    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH,
                         L"\xff67\xff70",
                         -1,
                         L"\x30a1\x30fc",
                         -1,
                         2,
                         "IgnoreKanaWidth - 18",
                         TRUE,
                         &NumErrors );


    CompareStringTester( MAKELCID(0x0411, 0),
                         0,
                         L"\x4e9c\x4e9c",
                         -1,
                         L"\x4e9c\x3005",
                         -1,
                         1,
                         "Ideograph, cho-on - 19",
                         TRUE,
                         &NumErrors );


    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORENONSPACE,
                         L"\x4e9c\x4e9c",
                         -1,
                         L"\x4e9c\x3005",
                         -1,
                         2,
                         "Ideograph, cho-on - 20",
                         TRUE,
                         &NumErrors );


    CompareStringTester( MAKELCID(0x0411, 0),
                         0,
                         L"\x30fb\x30fd",
                         -1,
                         L"\x30fb\x30fb",
                         -1,
                         1,
                         "Symbol, cho-on - 21",
                         TRUE,
                         &NumErrors );


    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORENONSPACE,
                         L"\x30fb\x30fd",
                         -1,
                         L"\x30fb\x30fb",
                         -1,
                         1,
                         "Symbol, cho-on - 22",
                         TRUE,
                         &NumErrors );


    CompareStringTester( MAKELCID(0x0411, 0),
                         NORM_IGNORENONSPACE,
                         L"\x308e\xff70",
                         -1,
                         L"\x308e\x30fc",
                         -1,
                         1,
                         "small cho-on (ignore nonspace) - 23",
                         TRUE,
                         &NumErrors );


    //
    //  Tests with extra diacritic on end of string.  Make sure it
    //  uses the first diacritic difference found rather than the
    //  "longer" string difference.
    //
    CompareStringTester( 0x0411,
                         0,
                         L"\x30cf\xff8a\xff9e",
                         -1,
                         L"\xff8a\xff9e\x30cf",
                         -1,
                         1,
                         "Different Compares",
                         FALSE,
                         &NumErrors );

    CompareStringTester( 0x0411,
                         NORM_IGNOREWIDTH,
                         L"\x30cf\xff8a\xff9e",
                         -1,
                         L"\xff8a\xff9e\x30cf",
                         -1,
                         1,
                         "Different Compares (ignore width)",
                         FALSE,
                         &NumErrors );

    CompareStringTester( 0x0411,
                         NORM_IGNORENONSPACE,
                         L"\x30cf\xff8a\xff9e",
                         -1,
                         L"\xff8a\xff9e\x30cf",
                         -1,
                         3,
                         "Different Compares (ignore nonspace)",
                         FALSE,
                         &NumErrors );

    CompareStringTester( 0x0411,
                         NORM_IGNORENONSPACE | NORM_IGNOREWIDTH,
                         L"\x30cf\xff8a\xff9e",
                         -1,
                         L"\xff8a\xff9e\x30cf",
                         -1,
                         2,
                         "Different Compares (ignore nonspace, width)",
                         FALSE,
                         &NumErrors );




    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  CS_Ansi
//
//  This routine tests the Ansi version of the API routine.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int CS_Ansi()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


    //
    //  CompareStringA is tested by CompareStringTester routine.
    //


    //
    //  CompareString
    //

    //  Variation 1  -  foo bar
    rc = CompareString( Locale,
                        0,
                        TEXT("foo"),
                        -1,
                        TEXT("bar"),
                        -1 );
    CheckReturnCompStr( rc,
                        3,
                        "neutral version (foo, bar)",
                        &NumErrors );

    //  Variation 2  -  foo bar
    rc = CompareString( Locale,
                        0,
                        TEXT("foo"),
                        3,
                        TEXT("bar"),
                        3 );
    CheckReturnCompStr( rc,
                        3,
                        "neutral version (foo, bar) size",
                        &NumErrors );



    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  CheckReturnCompStr
//
//  Checks the return code from the CompareString[A/W] call.  It prints out
//  the appropriate error if the incorrect result is found.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

void CheckReturnCompStr(
    int CurrentReturn,
    int ExpectedReturn,
    LPSTR pErrString,
    int *pNumErrors)
{
    if (CurrentReturn != ExpectedReturn)
    {
        printf("ERROR: %s - \n", pErrString);
        printf("  Return = %d, Expected = %d\n", CurrentReturn, ExpectedReturn);

        (*pNumErrors)++;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CompareSortkeyStrings
//
//  Checks that the result of the byte by byte compare of the sortkey strings
//  is the expected result.  It prints out the appropriate error if the
//  incorrect result is found.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

void CompareSortkeyStrings(
    LPBYTE pSort1,
    LPBYTE pSort2,
    int ExpectedReturn,
    LPSTR pErrString,
    int *pNumErrors)
{
    int CurrentReturn;            // current return value
    int ctr;                      // loop counter


    CurrentReturn = strcmp(pSort1, pSort2) + 2;
    if (CurrentReturn != ExpectedReturn)
    {
        printf("ERROR: SortKey - %s - \n", pErrString);
        printf("  Return = %d, Expected = %d\n", CurrentReturn, ExpectedReturn);

        printf("   SortKey1 = ");
        for (ctr = 0; pSort1[ctr]; ctr++)
        {
            printf("%x ", pSort1[ctr]);
        }
        printf("\n");

        printf("   SortKey2 = ");
        for (ctr = 0; pSort2[ctr]; ctr++)
        {
            printf("%x ", pSort2[ctr]);
        }
        printf("\n");

        (*pNumErrors)++;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  GetCPFromLocale
//
//  Gets the default code page for the given locale.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

UINT GetCPFromLocale(
    LCID Locale)
{
    WCHAR pBuf[BUFSIZE];
    LPWSTR pTmp;
    UINT CodePage;
    UINT Value;


    //
    //  Get the ACP.
    //
    if (!GetLocaleInfoW( Locale,
                         LOCALE_IDEFAULTANSICODEPAGE,
                         pBuf,
                         BUFSIZE ))
    {
        printf("FATAL ERROR: Could NOT get locale information for ACP.\n");
        return (0);
    }

    //
    //  Convert the string to an integer and return it.
    //
    pTmp = pBuf;
    for (CodePage = 0; *pTmp; pTmp++)
    {
        if ((Value = (UINT)(*pTmp - L'0')) > 9)
            break;
        CodePage = (UINT)(CodePage * 10 + Value);
    }
    return ( CodePage );
}


////////////////////////////////////////////////////////////////////////////
//
//  CompareStringTester
//
//  Call CompareStringW and CompareStringA and checks the return code.
//  It also calls LCMapStringW and LCMapStringA using the LCMAP_SORTKEY flag,
//  and then checks to be sure the byte by byte compare gives the same
//  result as CompareString.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

void CompareStringTester(
    LCID Locale,
    DWORD dwFlags,
    LPWSTR pString1,
    int Count1,
    LPWSTR pString2,
    int Count2,
    int ExpectedReturn,
    LPSTR pErrString,
    BOOL TestAVersion,
    int *pNumErrors)
{
    int rc;
    BYTE SortDest1[BUFSIZE];
    BYTE SortDest2[BUFSIZE];

    BYTE pString1A[BUFSIZE];
    BYTE pString2A[BUFSIZE];
    int Count1A;
    int Count2A;
    int Count1T = -1;
    int Count2T;


    //
    //  Call CompareStringW with the given counts.
    //
    rc = CompareStringW( Locale,
                         dwFlags,
                         pString1,
                         Count1,
                         pString2,
                         Count2 );
    CheckReturnCompStr( rc,
                        ExpectedReturn,
                        pErrString,
                        pNumErrors );


    //
    //  Call LCMapStringW with the given counts.
    //
    if ( (!LCMapStringW( Locale,
                         LCMAP_SORTKEY | dwFlags,
                         pString1,
                         Count1,
                         (LPWSTR)SortDest1,
                         BUFSIZE )) ||
         (!LCMapStringW( Locale,
                         LCMAP_SORTKEY | dwFlags,
                         pString2,
                         Count2,
                         (LPWSTR)SortDest2,
                         BUFSIZE )) )
    {
        printf("FATAL ERROR: Could NOT get SORTKEY value for string.\n");
        return;
    }

    CompareSortkeyStrings( SortDest1,
                           SortDest2,
                           ExpectedReturn,
                           pErrString,
                           pNumErrors );


    //
    //  See if we should make additional calls with the temp counts.
    //
    if ((Count1 < 0) || (Count2 < 0))
    {
        //
        //  Get the temp counts if either count is set to -1.  This is to make
        //  an additional call to CompareStringW with the actual count of the
        //  strings instead of -1.
        //
        Count1T = (Count1 < 0) ? wcslen(pString1) : Count1;
        Count2T = (Count2 < 0) ? wcslen(pString2) : Count2;


        //
        //  Call CompareStringW with the temp counts.
        //
        rc = CompareStringW( Locale,
                             dwFlags,
                             pString1,
                             Count1T,
                             pString2,
                             Count2T );
        CheckReturnCompStr( rc,
                            ExpectedReturn,
                            pErrString,
                            pNumErrors );


        //
        //  Call LCMapStringW with the temp counts.
        //
        if ( (!LCMapStringW( Locale,
                             LCMAP_SORTKEY | dwFlags,
                             pString1,
                             Count1T,
                             (LPWSTR)SortDest1,
                             BUFSIZE )) ||
             (!LCMapStringW( Locale,
                             LCMAP_SORTKEY | dwFlags,
                             pString2,
                             Count2T,
                             (LPWSTR)SortDest2,
                             BUFSIZE )) )
        {
            printf("FATAL ERROR: Could NOT get SORTKEY value for string.\n");
            return;
        }

        CompareSortkeyStrings( SortDest1,
                               SortDest2,
                               ExpectedReturn,
                               pErrString,
                               pNumErrors );
    }


    if (TestAVersion)
    {
        //
        //  Get the Ansi versions of the strings.
        //
        if ( ((Count1A = WideCharToMultiByte( GetCPFromLocale(Locale),
                                              0,
                                              pString1,
                                              Count1,
                                              pString1A,
                                              BUFSIZE,
                                              NULL,
                                              NULL )) == 0) ||
             ((Count2A = WideCharToMultiByte( GetCPFromLocale(Locale),
                                              0,
                                              pString2,
                                              Count2,
                                              pString2A,
                                              BUFSIZE,
                                              NULL,
                                              NULL )) == 0) )
        {
            printf("FATAL ERROR: Could NOT convert to MULTIBYTE string.\n");
            return;
        }


        //
        //  Call CompareStringA with the given counts.
        //
        rc = CompareStringA( Locale,
                             dwFlags,
                             pString1A,
                             (Count1 < 0) ? Count1 : Count1A,
                             pString2A,
                             (Count2 < 0) ? Count2 : Count2A );
        CheckReturnCompStr( rc,
                            ExpectedReturn,
                            pErrString,
                            pNumErrors );


        //
        //  Call LCMapStringA with the given counts.
        //
        if ( (!LCMapStringA( Locale,
                             LCMAP_SORTKEY | dwFlags,
                             pString1A,
                             (Count1 < 0) ? Count1 : Count1A,
                             (LPSTR)SortDest1,
                             BUFSIZE )) ||
             (!LCMapStringA( Locale,
                             LCMAP_SORTKEY | dwFlags,
                             pString2A,
                             (Count2 < 0) ? Count2 : Count2A,
                             (LPSTR)SortDest2,
                             BUFSIZE )) )
        {
            printf("FATAL ERROR: Could NOT get SORTKEY value for string.\n");
            return;
        }

        CompareSortkeyStrings( SortDest1,
                               SortDest2,
                               ExpectedReturn,
                               pErrString,
                               pNumErrors );

        //
        //  See if we should make additional calls with the temp counts.
        //
        if (Count1T != -1)
        {
            //
            //  Call CompareStringA with the temp counts.
            //
            rc = CompareStringA( Locale,
                                 dwFlags,
                                 pString1A,
                                 Count1A,
                                 pString2A,
                                 Count2A );
            CheckReturnCompStr( rc,
                                ExpectedReturn,
                                pErrString,
                                pNumErrors );


            //
            //  Call LCMapStringA with the given counts.
            //
            if ( (!LCMapStringA( Locale,
                                 LCMAP_SORTKEY | dwFlags,
                                 pString1A,
                                 Count1A,
                                 (LPSTR)SortDest1,
                                 BUFSIZE )) ||
                 (!LCMapStringA( Locale,
                                 LCMAP_SORTKEY | dwFlags,
                                 pString2A,
                                 Count2A,
                                 (LPSTR)SortDest2,
                                 BUFSIZE )) )
            {
                printf("FATAL ERROR: Could NOT get SORTKEY value for string.\n");
                return;
            }

            CompareSortkeyStrings( SortDest1,
                                   SortDest2,
                                   ExpectedReturn,
                                   pErrString,
                                   pNumErrors );
        }
    }
}
