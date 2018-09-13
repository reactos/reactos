/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    mstest.c

Abstract:

    Test module for NLS API LCMapString.

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

#define  BUFSIZE  50
#define  LCMS_INVALID_FLAGS                                               \
               ((DWORD)(~( LCMAP_LOWERCASE    | LCMAP_UPPERCASE     |     \
                           LCMAP_SORTKEY      | LCMAP_BYTEREV       |     \
                           LCMAP_HIRAGANA     | LCMAP_KATAKANA      |     \
                           LCMAP_HALFWIDTH    | LCMAP_FULLWIDTH     |     \
                           LCMAP_TRADITIONAL_CHINESE                |     \
                           LCMAP_SIMPLIFIED_CHINESE                 |     \
                           NORM_IGNORECASE    | NORM_IGNORENONSPACE |     \
                           NORM_IGNORESYMBOLS | NORM_IGNOREKANATYPE |     \
                           NORM_IGNOREWIDTH )))




//
//  Global Variables.
//

LCID Locale;

#define MapSrc1              L"This Is A String"

#define MapSrc2              L"This Is$ A S-tr,in'g"

#define MapNoSymbols         L"ThisIsAString"


WCHAR MapDest[BUFSIZE];
BYTE  SortDest[BUFSIZE];
BYTE  MapDestA[BUFSIZE];


#define MapUpper             L"THIS IS A STRING"
#define MapLower             L"this is a string"
#define MapLower2            L"this is$ a str,ing"

//  Sharp S
#define wcSharpS             L"\x0074\x00df\x0074"

//  Nonspace
#define MapNS1               L"\x0074\x00e1\x0061\x0301"
#define MapNS2               L"\x0074\x0301\x00e1\x0061\x0301"
#define MapNoNS1             L"\x0074\x0061\x0061"
#define MapUpperNS           L"\x0054\x00c1\x0041\x0301"

//  Sortkey
#define SortPunctPre         L"\x00e1\x002d"          // a-acute, -
#define SortPunctComp        L"\x0061\x0301\x002d"    // a, acute, -

#define SortPunctExp1        L"\x00e6\x002d"          // ae, -
#define SortPunctExp2        L"\x0061\x0065\x002d"    // a, e, -

#define SortPunctUnsort      L"\x00e6\xffff\x002d"    // ae, unsort, -

#define SortSymbolPre        L"\x00e1\x002a"          // a-acute, *
#define SortSymbolComp       L"\x0061\x0301\x002a"    // a, acute, *

#define SortSymbolExp1       L"\x00e6\x002a"          // ae, *
#define SortSymbolExp2       L"\x0061\x0065\x002a"    // a, e, *

#define SortSymbolUnsort     L"\x00e6\xffff\x002a"    // ae, unsort, *




//
//  Forward Declarations.
//

BOOL
InitLCMapStr();

int
LCMS_BadParamCheck();

int
LCMS_NormalCase();

int
LCMS_Ansi();

void
CheckReturnValidSortKey(
    int CurrentReturn,
    int ExpectedReturn,
    LPBYTE pCurrentString,
    LPBYTE pExpectedString,
    LPBYTE pErrString,
    int *pNumErrors);





////////////////////////////////////////////////////////////////////////////
//
//  TestLCMapString
//
//  Test routine for LCMapStringW API.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int TestLCMapString()
{
    int ErrCount = 0;             // error count


    //
    //  Print out what's being done.
    //
    printf("\n\nTESTING LCMapStringW...\n\n");

    //
    //  Initialize global variables.
    //
    if (!InitLCMapStr())
    {
        printf("\nABORTED TestLCMapString: Could not Initialize.\n");
        return (1);
    }

    //
    //  Test bad parameters.
    //
    ErrCount += LCMS_BadParamCheck();

    //
    //  Test normal cases.
    //
    ErrCount += LCMS_NormalCase();

    //
    //  Test Ansi version.
    //
    ErrCount += LCMS_Ansi();

    //
    //  Print out result.
    //
    printf("\nLCMapStringW:  ERRORS = %d\n", ErrCount);

    //
    //  Return total number of errors found.
    //
    return (ErrCount);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitLCMapStr
//
//  This routine initializes the global variables.  If no errors were
//  encountered, then it returns TRUE.  Otherwise, it returns FALSE.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL InitLCMapStr()
{
    int size = BUFSIZE;           // size of string


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
//  LCMS_BadParamCheck
//
//  This routine passes in bad parameters to the API routines and checks to
//  be sure they are handled properly.  The number of errors encountered
//  is returned to the caller.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int LCMS_BadParamCheck()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


    //
    //  Bad Locale.
    //

    //  Variation 1  -  Bad Locale
    rc = LCMapStringW( (LCID)333,
                       LCMAP_LOWERCASE,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "Bad Locale",
                         &NumErrors );


    //
    //  Null Pointers.
    //

    //  Variation 1  -  lpSrcStr = NULL
    rc = LCMapStringW( Locale,
                       LCMAP_LOWERCASE,
                       NULL,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "lpSrcStr NULL",
                         &NumErrors );

    //  Variation 2  -  lpDestStr = NULL
    rc = LCMapStringW( Locale,
                       LCMAP_LOWERCASE,
                       MapSrc1,
                       -1,
                       NULL,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "lpDestStr NULL",
                         &NumErrors );


    //
    //  Bad Counts.
    //

    //  Variation 1  -  cbSrc = 0
    rc = LCMapStringW( Locale,
                       LCMAP_LOWERCASE,
                       MapSrc1,
                       0,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "cbSrc = 0",
                         &NumErrors );

    //  Variation 2  -  cbDest < 0
    rc = LCMapStringW( Locale,
                       LCMAP_LOWERCASE,
                       MapSrc1,
                       -1,
                       MapDest,
                       -1 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "cbDest < 0",
                         &NumErrors );


    //
    //  Zero or Invalid Flag Values.
    //

    //  Variation 0  -  Use CP ACP
    rc = LCMapStringW( Locale,
                       LOCALE_USE_CP_ACP,
                       L"Th",
                       2,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "Use CP ACP",
                         &NumErrors );

    //  Variation 0.1  -  byterev, ignore case
    rc = LCMapStringW( Locale,
                       LCMAP_BYTEREV | NORM_IGNORECASE,
                       L"Th",
                       2,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "byterev, ignore case",
                         &NumErrors );

    //  Variation 1  -  dwMapFlags = invalid
    rc = LCMapStringW( Locale,
                       LCMS_INVALID_FLAGS,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "dwMapFlags invalid",
                         &NumErrors );

    //  Variation 2  -  dwMapFlags = 0
    rc = LCMapStringW( Locale,
                       0,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "dwMapFlags zero",
                         &NumErrors );

    //  Variation 3  -  illegal combo case
    rc = LCMapStringW( Locale,
                       LCMAP_LOWERCASE | LCMAP_UPPERCASE,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "illegal combo case",
                         &NumErrors );

    //  Variation 4  -  illegal combo sortkey
    rc = LCMapStringW( Locale,
                       LCMAP_LOWERCASE | LCMAP_SORTKEY,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "illegal combo sortkey",
                         &NumErrors );

    //  Variation 5  -  legal combo byterev
    rc = LCMapStringW( Locale,
                       LCMAP_LOWERCASE | LCMAP_BYTEREV,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       MapSrc1,
                       "legal combo byterev",
                       &NumErrors );

    //  Variation 6  -  illegal flag ignorecase
    rc = LCMapStringW( Locale,
                       NORM_IGNORECASE,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "illegal flag ignorecase",
                         &NumErrors );

    //  Variation 7  -  illegal combo kana
    rc = LCMapStringW( Locale,
                       LCMAP_HIRAGANA | LCMAP_KATAKANA,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "illegal combo kana",
                         &NumErrors );

    //  Variation 8  -  illegal combo width
    rc = LCMapStringW( Locale,
                       LCMAP_HALFWIDTH | LCMAP_FULLWIDTH,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "illegal combo width",
                         &NumErrors );

    //  Variation 9  -  illegal combo sortkey, hiragana
    rc = LCMapStringW( Locale,
                       LCMAP_SORTKEY | LCMAP_HIRAGANA,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "illegal combo sortkey, hiragana",
                         &NumErrors );

    //  Variation 10  -  illegal combo sortkey, katakana
    rc = LCMapStringW( Locale,
                       LCMAP_SORTKEY | LCMAP_KATAKANA,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "illegal combo sortkey, katakana",
                         &NumErrors );

    //  Variation 11  -  illegal combo sortkey, half width
    rc = LCMapStringW( Locale,
                       LCMAP_SORTKEY | LCMAP_HALFWIDTH,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "illegal combo sortkey, half width",
                         &NumErrors );

    //  Variation 12  -  illegal combo sortkey, full width
    rc = LCMapStringW( Locale,
                       LCMAP_SORTKEY | LCMAP_FULLWIDTH,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "illegal combo sortkey, full width",
                         &NumErrors );

    //  Variation 13  -  illegal combo kana, ignore symbols
    rc = LCMapStringW( Locale,
                       LCMAP_HIRAGANA | NORM_IGNORESYMBOLS,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "illegal combo kana, symbols",
                         &NumErrors );

    //  Variation 14  -  illegal combo width, ignore symbols
    rc = LCMapStringW( Locale,
                       LCMAP_HALFWIDTH | NORM_IGNORESYMBOLS,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "illegal combo width, symbols",
                         &NumErrors );

    //  Variation 15  -  illegal combo kana, ignore nonspace
    rc = LCMapStringW( Locale,
                       LCMAP_HIRAGANA | NORM_IGNORENONSPACE,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "illegal combo kana, nonspace",
                         &NumErrors );

    //  Variation 16  -  illegal combo width, ignore nonspace
    rc = LCMapStringW( Locale,
                       LCMAP_HALFWIDTH | NORM_IGNORENONSPACE,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "illegal combo width, nonspace",
                         &NumErrors );

    //  Variation 17  -  illegal combo kana, ignore kana
    rc = LCMapStringW( Locale,
                       LCMAP_HIRAGANA | NORM_IGNOREKANATYPE,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "illegal combo kana, ignore kana",
                         &NumErrors );

    //  Variation 18  -  illegal combo width, ignore kana
    rc = LCMapStringW( Locale,
                       LCMAP_HALFWIDTH | NORM_IGNOREKANATYPE,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "illegal combo width, ignore kana",
                         &NumErrors );

    //  Variation 19  -  illegal combo kana, ignore width
    rc = LCMapStringW( Locale,
                       LCMAP_HIRAGANA | NORM_IGNOREWIDTH,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "illegal combo kana, ignore width",
                         &NumErrors );

    //  Variation 20  -  illegal combo width, ignore width
    rc = LCMapStringW( Locale,
                       LCMAP_HALFWIDTH | NORM_IGNOREWIDTH,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "illegal combo width, ignore width",
                         &NumErrors );

    //  Variation 21  -  illegal combo linguistic
    rc = LCMapStringW( Locale,
                       LCMAP_LINGUISTIC_CASING,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "illegal combo linguistic",
                         &NumErrors );

    //  Variation 22  -  illegal combo linguistic, kana
    rc = LCMapStringW( Locale,
                       LCMAP_LINGUISTIC_CASING | LCMAP_HIRAGANA,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "illegal combo linguistic, kana",
                         &NumErrors );

    //  Variation 23  -  illegal combo linguistic, sortkey
    rc = LCMapStringW( Locale,
                       LCMAP_LINGUISTIC_CASING | LCMAP_SORTKEY,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "illegal combo linguistic, sortkey",
                         &NumErrors );

    //  Variation 24  -  illegal combo linguistic, norm
    rc = LCMapStringW( Locale,
                       LCMAP_LINGUISTIC_CASING | NORM_IGNORENONSPACE,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "illegal combo linguistic, norm",
                         &NumErrors );

    //  Variation 25  -  illegal combo traditional, kana
    rc = LCMapStringW( Locale,
                       LCMAP_TRADITIONAL_CHINESE | LCMAP_HIRAGANA,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "illegal combo traditional, kana",
                         &NumErrors );

    //  Variation 26  -  illegal combo traditional, width
    rc = LCMapStringW( Locale,
                       LCMAP_TRADITIONAL_CHINESE | LCMAP_HALFWIDTH,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "illegal combo traditional, width",
                         &NumErrors );

    //  Variation 27  -  illegal combo traditional, sortkey
    rc = LCMapStringW( Locale,
                       LCMAP_TRADITIONAL_CHINESE | LCMAP_SORTKEY,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "illegal combo traditional, sortkey",
                         &NumErrors );

    //  Variation 28  -  illegal combo simplified, kana
    rc = LCMapStringW( Locale,
                       LCMAP_SIMPLIFIED_CHINESE | LCMAP_HIRAGANA,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "illegal combo simplified, kana",
                         &NumErrors );

    //  Variation 29  -  illegal combo simplified, width
    rc = LCMapStringW( Locale,
                       LCMAP_SIMPLIFIED_CHINESE | LCMAP_HALFWIDTH,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "illegal combo simplified, width",
                         &NumErrors );

    //  Variation 30  -  illegal combo simplified, sortkey
    rc = LCMapStringW( Locale,
                       LCMAP_SIMPLIFIED_CHINESE | LCMAP_SORTKEY,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "illegal combo simplified, sortkey",
                         &NumErrors );





    //
    //  Check the error returns for A and W version.
    //

    //  Variation 1  -  NULL destination
    SetLastError(0);
    rc = LCMapStringW( 0x0409,
                       0,
                       L"xxx",
                       -1,
                       NULL,
                       100 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "A and W errors same - NULL Dest - W version",
                         &NumErrors );

    SetLastError(0);
    rc = LCMapStringA( 0x0409,
                       0,
                       "xxx",
                       -1,
                       NULL,
                       100 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "A and W errors same - NULL Dest - A version",
                         &NumErrors );


    //  Variation 2  -  Pointers equal
    SetLastError(0);
    rc = LCMapStringW( 0x0409,
                       0,
                       MapDest,
                       -1,
                       MapDest,
                       100 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "A and W errors same - Pointers Equal - W version",
                         &NumErrors );

    SetLastError(0);
    rc = LCMapStringA( 0x0409,
                       0,
                       MapDestA,
                       -1,
                       MapDestA,
                       100 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "A and W errors same - Pointers Equal - A version",
                         &NumErrors );


    //
    //  Check src = dest  -  W Version.
    //

    //  Variation 0
    SetLastError(0);
    MapDest[0] = L'x';
    MapDest[1] = 0;
    rc = LCMapStringW( 0x0409,
                       LCMAP_UPPERCASE | LCMAP_HIRAGANA,
                       MapDest,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "src = dest - uppercase and hiragana",
                         &NumErrors );

    //  Variation 1
    SetLastError(0);
    MapDest[0] = L'x';
    MapDest[1] = 0;
    rc = LCMapStringW( 0x0409,
                       LCMAP_LOWERCASE | LCMAP_HIRAGANA,
                       MapDest,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "src = dest - lowercase and hiragana",
                         &NumErrors );

    //  Variation 2
    SetLastError(0);
    MapDest[0] = L'x';
    MapDest[1] = 0;
    rc = LCMapStringW( 0x0409,
                       LCMAP_LOWERCASE | LCMAP_KATAKANA,
                       MapDest,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "src = dest - lowercase and katakana",
                         &NumErrors );

    //  Variation 3
    SetLastError(0);
    MapDest[0] = L'x';
    MapDest[1] = 0;
    rc = LCMapStringW( 0x0409,
                       LCMAP_LOWERCASE | LCMAP_HALFWIDTH,
                       MapDest,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "src = dest - lowercase and halfwidth",
                         &NumErrors );

    //  Variation 4
    SetLastError(0);
    MapDest[0] = L'x';
    MapDest[1] = 0;
    rc = LCMapStringW( 0x0409,
                       LCMAP_LOWERCASE | LCMAP_FULLWIDTH,
                       MapDest,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "src = dest - lowercase and fullwidth",
                         &NumErrors );

    //  Variation 5
    SetLastError(0);
    MapDest[0] = L'x';
    MapDest[1] = 0;
    rc = LCMapStringW( 0x0409,
                       LCMAP_LOWERCASE | LCMAP_FULLWIDTH | LCMAP_HIRAGANA,
                       MapDest,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "src = dest - lowercase and fullwidth and hiragana",
                         &NumErrors );

    //  Variation 6
    MapDest[0] = L'x';
    MapDest[1] = L'y';
    MapDest[2] = 0;
    rc = LCMapStringW( Locale,
                       LCMAP_UPPERCASE,
                       MapDest,
                       -1,
                       MapDest,
                       2 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INSUFFICIENT_BUFFER,
                         "src = dest - uppercase, size - buffer too small",
                         &NumErrors );


    //
    //  Check src = dest  -  A Version.
    //

    //  Variation 0
    SetLastError(0);
    MapDestA[0] = 'x';
    MapDestA[1] = 0;
    rc = LCMapStringA( 0x0409,
                       LCMAP_UPPERCASE | LCMAP_HIRAGANA,
                       MapDestA,
                       -1,
                       MapDestA,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "src = dest - uppercase and hiragana (A Version)",
                         &NumErrors );

    //  Variation 1
    SetLastError(0);
    MapDestA[0] = 'x';
    MapDestA[1] = 0;
    rc = LCMapStringA( 0x0409,
                       LCMAP_LOWERCASE | LCMAP_HIRAGANA,
                       MapDestA,
                       -1,
                       MapDestA,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "src = dest - lowercase and hiragana (A Version)",
                         &NumErrors );

    //  Variation 2
    SetLastError(0);
    MapDestA[0] = 'x';
    MapDestA[1] = 0;
    rc = LCMapStringA( 0x0409,
                       LCMAP_LOWERCASE | LCMAP_KATAKANA,
                       MapDestA,
                       -1,
                       MapDestA,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "src = dest - lowercase and katakana (A Version)",
                         &NumErrors );

    //  Variation 3
    SetLastError(0);
    MapDestA[0] = 'x';
    MapDestA[1] = 0;
    rc = LCMapStringA( 0x0409,
                       LCMAP_LOWERCASE | LCMAP_HALFWIDTH,
                       MapDestA,
                       -1,
                       MapDestA,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "src = dest - lowercase and halfwidth (A Version)",
                         &NumErrors );

    //  Variation 4
    SetLastError(0);
    MapDestA[0] = 'x';
    MapDestA[1] = 0;
    rc = LCMapStringA( 0x0409,
                       LCMAP_LOWERCASE | LCMAP_FULLWIDTH,
                       MapDestA,
                       -1,
                       MapDestA,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "src = dest - lowercase and fullwidth (A Version)",
                         &NumErrors );

    //  Variation 5
    SetLastError(0);
    MapDestA[0] = 'x';
    MapDestA[1] = 0;
    rc = LCMapStringA( 0x0409,
                       LCMAP_LOWERCASE | LCMAP_FULLWIDTH | LCMAP_HIRAGANA,
                       MapDestA,
                       -1,
                       MapDestA,
                       BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "src = dest - lowercase and fullwidth and hiragana (A Version)",
                         &NumErrors );

    //  Variation 6
    MapDestA[0] = 'x';
    MapDestA[1] = 'y';
    MapDestA[2] = 0;
    rc = LCMapStringA( Locale,
                       LCMAP_UPPERCASE,
                       MapDestA,
                       -1,
                       MapDestA,
                       2 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INSUFFICIENT_BUFFER,
                         "src = dest - uppercase, size - buffer too small (A Version)",
                         &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  LCMS_NormalCase
//
//  This routine tests the normal cases of the API routine.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int LCMS_NormalCase()
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
    rc = LCMapStringW( LOCALE_SYSTEM_DEFAULT,
                       LCMAP_LOWERCASE,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       MapLower,
                       "system default locale",
                       &NumErrors );

    //  Variation 2  -  Current User Locale
    rc = LCMapStringW( LOCALE_USER_DEFAULT,
                       LCMAP_LOWERCASE,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       MapLower,
                       "current user locale",
                       &NumErrors );


    //
    //  cbDest = 0.
    //

    //  Variation 1  -  cbSrc = -1
    rc = LCMapStringW( Locale,
                       LCMAP_LOWERCASE,
                       MapSrc1,
                       -1,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       MapLower,
                       "cbDest (0) cbSrc (-1)",
                       &NumErrors );

    //  Variation 2  -  cbSrc = value
    rc = LCMapStringW( Locale,
                       LCMAP_LOWERCASE,
                       MapSrc1,
                       WC_STRING_LEN_NULL(MapSrc1),
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       MapLower,
                       "cbDest (0) cbSrc (value)",
                       &NumErrors );

    //  Variation 3  -  lpDestStr = NULL
    rc = LCMapStringW( Locale,
                       LCMAP_LOWERCASE,
                       MapSrc1,
                       -1,
                       NULL,
                       0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       MapLower,
                       "cbDest (0) lpDestStr NULL",
                       &NumErrors );


    //
    //  cbSrc.
    //

    //  Variation 1  -  cbSrc = -1
    rc = LCMapStringW( Locale,
                       LCMAP_LOWERCASE,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       MapLower,
                       "cbSrc (-1)",
                       &NumErrors );

    //  Variation 2  -  cbSrc = value
    rc = LCMapStringW( Locale,
                       LCMAP_LOWERCASE,
                       MapSrc1,
                       WC_STRING_LEN(MapSrc1),
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       WC_STRING_LEN(MapSrc1),
                       MapDest,
                       MapLower,
                       "cbSrc (value)",
                       &NumErrors );

    //  Variation 3  -  cbSrc = -1, no DestStr
    rc = LCMapStringW( Locale,
                       LCMAP_LOWERCASE,
                       MapSrc1,
                       -1,
                       NULL,
                       0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       MapLower,
                       "cbSrc (-1), no DestStr",
                       &NumErrors );

    //  Variation 4  -  cbSrc = value, no DestStr
    rc = LCMapStringW( Locale,
                       LCMAP_LOWERCASE,
                       MapSrc1,
                       WC_STRING_LEN(MapSrc1),
                       NULL,
                       0 );
    CheckReturnValidW( rc,
                       WC_STRING_LEN(MapSrc1),
                       NULL,
                       MapLower,
                       "cbSrc (value), no DestStr",
                       &NumErrors );


    //
    //  LOCALE_USE_CP_ACP Flag.
    //

    //  Variation 1  -  Use CP ACP
    rc = LCMapStringW( Locale,
                       LOCALE_USE_CP_ACP | LCMAP_LOWERCASE,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       MapLower,
                       "Use CP ACP",
                       &NumErrors );


    //
    //  LCMAP_LOWERCASE Flag.
    //

    //  Variation 1  -  lowercase
    rc = LCMapStringW( Locale,
                       LCMAP_LOWERCASE,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       MapLower,
                       "lowercase",
                       &NumErrors );

    //  Variation 2  -  lowercase, no DestStr
    rc = LCMapStringW( Locale,
                       LCMAP_LOWERCASE,
                       MapSrc1,
                       -1,
                       NULL,
                       0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       MapLower,
                       "lowercase, no DestStr",
                       &NumErrors );


    //
    //  LCMAP_UPPERCASE Flag.
    //

    //  Variation 1  -  uppercase
    rc = LCMapStringW( Locale,
                       LCMAP_UPPERCASE,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       MapUpper,
                       "uppercase",
                       &NumErrors );

    //  Variation 2  -  uppercase, no DestStr
    rc = LCMapStringW( Locale,
                       LCMAP_UPPERCASE,
                       MapSrc1,
                       -1,
                       NULL,
                       0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       MapUpper,
                       "uppercase, no DestStr",
                       &NumErrors );

    //  Variation 3  -  uppercase, sharp S
    rc = LCMapStringW( Locale,
                       LCMAP_UPPERCASE,
                       wcSharpS,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\x0054\x00df\x0054",
                       "uppercase, sharp S",
                       &NumErrors );

    //  Variation 4  -  upper case nonspace
    rc = LCMapStringW( Locale,
                       LCMAP_UPPERCASE,
                       MapNS1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       MapUpperNS,
                       "uppercase, nonspace",
                       &NumErrors );

    //  Variation 5  -  upper case, no DestStr, nonspace
    rc = LCMapStringW( Locale,
                       LCMAP_UPPERCASE,
                       MapNS1,
                       -1,
                       NULL,
                       0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       MapUpperNS,
                       "uppercase, nonspace, no DestStr",
                       &NumErrors );

    //  Variation 6  -  uppercase Korean
    MapDest[0] = L'a';
    MapDest[1] = L'b';
    MapDest[2] = 0;
    rc = LCMapStringW( MAKELCID(MAKELANGID(LANG_KOREAN, SUBLANG_KOREAN), SORT_DEFAULT),
                       LCMAP_UPPERCASE,
                       MapDest,
                       3,
                       MapDest,
                       3 );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"AB",
                       "Korean uppercase",
                       &NumErrors );




    //
    //  LCMAP_LINGUISTIC_CASING Flag.
    //

    //  Variation 1  -  linguistic uppercase
    rc = LCMapStringW( Locale,
                       LCMAP_UPPERCASE | LCMAP_LINGUISTIC_CASING,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       MapUpper,
                       "linguistic uppercase",
                       &NumErrors );

    //  Variation 2  -  linguistic uppercase
    rc = LCMapStringW( Locale,
                       LCMAP_UPPERCASE | LCMAP_LINGUISTIC_CASING,
                       L"\xff41\xff5a\xff21",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\xff21\xff3a\xff21",
                       "linguistic uppercase (AZ)",
                       &NumErrors );

    //  Variation 3  -  linguistic uppercase
    rc = LCMapStringW( 0x041f,
                       LCMAP_UPPERCASE | LCMAP_LINGUISTIC_CASING,
                       L"\xff41\xff5a\x0069\x0131",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\xff21\xff3a\x0130\x0049",
                       "linguistic uppercase (Turkish)",
                       &NumErrors );

    //  Variation 4  -  linguistic lowercase
    rc = LCMapStringW( Locale,
                       LCMAP_LOWERCASE | LCMAP_LINGUISTIC_CASING,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       MapLower,
                       "linguistic lowercase",
                       &NumErrors );

    //  Variation 5  -  linguistic lowercase
    rc = LCMapStringW( Locale,
                       LCMAP_LOWERCASE | LCMAP_LINGUISTIC_CASING,
                       L"\xff21\xff3a\xff41",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\xff41\xff5a\xff41",
                       "linguistic lowercase (az)",
                       &NumErrors );

    //  Variation 6  -  linguistic lowercase
    rc = LCMapStringW( 0x041f,
                       LCMAP_LOWERCASE | LCMAP_LINGUISTIC_CASING,
                       L"\xff21\xff3a\x0049\x0130",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\xff41\xff5a\x0131\x0069",
                       "linguistic lowercase (Turkish)",
                       &NumErrors );


    //
    //  NORM_IGNORESYMBOLS Flag.
    //

    //  Variation 1  -  ignoresymbols
    rc = LCMapStringW( Locale,
                       NORM_IGNORESYMBOLS,
                       MapSrc2,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       MapNoSymbols,
                       "ignore symbols",
                       &NumErrors );

    //  Variation 2  -  ignoresymbols, no DestStr
    rc = LCMapStringW( Locale,
                       NORM_IGNORESYMBOLS,
                       MapSrc2,
                       -1,
                       NULL,
                       0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       MapNoSymbols,
                       "ignore symbols, no DestStr",
                       &NumErrors );


    //
    //  NORM_IGNORENONSPACE Flag.
    //

    //  Variation 1  -  ignore nonspace
    rc = LCMapStringW( Locale,
                       NORM_IGNORENONSPACE,
                       MapNS1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       MapNoNS1,
                       "ignore nonspace",
                       &NumErrors );

    //  Variation 2  -  ignore nonspace, no translation
    rc = LCMapStringW( Locale,
                       NORM_IGNORENONSPACE,
                       MapNS2,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       MapNoNS1,
                       "ignore nonspace, no trans",
                       &NumErrors );

    //  Variation 3  -  ignore nonspace, no DestStr
    rc = LCMapStringW( Locale,
                       NORM_IGNORENONSPACE,
                       MapNS1,
                       -1,
                       NULL,
                       0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       MapNoNS1,
                       "ignore nonspace, no DestStr",
                       &NumErrors );

    //  Variation 4  -  ignore nonspace, no translation, no DestStr
    rc = LCMapStringW( Locale,
                       NORM_IGNORENONSPACE,
                       MapNS2,
                       -1,
                       NULL,
                       0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       MapNoNS1,
                       "ignore nonspace, no trans, no DestStr",
                       &NumErrors );


    //
    //  LCMAP_SORTKEY Flag.
    //

    //  Variation 1  -  sortkey
    rc = LCMapStringW( Locale,
                       LCMAP_SORTKEY,
                       L"Th",
                       -1,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x0e\x99\x0e\x2c\x01\x01\x12\x01\x01",
                             "sortkey",
                             &NumErrors );

    //  Variation 2  -  sortkey, no DestStr
    rc = LCMapStringW( Locale,
                       LCMAP_SORTKEY,
                       L"Th",
                       -1,
                       NULL,
                       0 );
    CheckReturnValidSortKey( rc,
                             -1,
                             NULL,
                             "\x0e\x99\x0e\x2c\x01\x01\x12\x01\x01",
                             "sortkey, no DestStr",
                             &NumErrors );

    //  Variation 3  -  sortkey (case)
    rc = LCMapStringW( Locale,
                       LCMAP_SORTKEY | NORM_IGNORECASE,
                       L"Th",
                       -1,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x0e\x99\x0e\x2c\x01\x01\x01\x01",
                             "sortkey (case)",
                             &NumErrors );

    //  Variation 4  -  sortkey, no DestStr (case)
    rc = LCMapStringW( Locale,
                       LCMAP_SORTKEY | NORM_IGNORECASE,
                       L"Th",
                       -1,
                       NULL,
                       0 );
    CheckReturnValidSortKey( rc,
                             -1,
                             NULL,
                             "\x0e\x99\x0e\x2c\x01\x01\x01\x01",
                             "sortkey (case), no DestStr",
                             &NumErrors );

    //
    //  Check for punctuation position using nonspace chars.
    //  Make sure:
    //     a-acute, punct    is the same as
    //     a, acute, punct
    //

    //  Variation 5  -  sortkey (precomp punct position)
    rc = LCMapStringW( Locale,
                       LCMAP_SORTKEY,
                       SortPunctPre,
                       -1,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x0e\x02\x01\x0e\x01\x01\x01\x80\x0b\x06\x82",
                             "sortkey (precomp punct position)",
                             &NumErrors );

    //  Variation 6  -  sortkey (comp punct position)
    rc = LCMapStringW( Locale,
                       LCMAP_SORTKEY,
                       SortPunctComp,
                       -1,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x0e\x02\x01\x0e\x01\x01\x01\x80\x0b\x06\x82",
                             "sortkey (comp punct position)",
                             &NumErrors );

    //
    //  Check for punctuation position using expansion chars.
    //  Make sure:
    //     ae, punct    is the same as
    //     a, e, punct
    //
    //  Variation 7  -  sortkey (expansion 1 punct position)
    rc = LCMapStringW( Locale,
                       LCMAP_SORTKEY,
                       SortPunctExp1,
                       -1,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x0e\x02\x0e\x21\x01\x01\x01\x01\x80\x0f\x06\x82",
                             "sortkey (expansion 1 punct position)",
                             &NumErrors );

    //  Variation 8  -  sortkey (expansion 2 punct position)
    rc = LCMapStringW( Locale,
                       LCMAP_SORTKEY,
                       SortPunctExp2,
                       -1,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x0e\x02\x0e\x21\x01\x01\x01\x01\x80\x0f\x06\x82",
                             "sortkey (expansion 2 punct position)",
                             &NumErrors );

    //
    //  Check for punctuation position using unsortable chars.
    //  Make sure:
    //     ae, unsort, punct    is the same as
    //     a, e, punct
    //
    //  Variation 9  -  sortkey (unsortable punct position)
    rc = LCMapStringW( Locale,
                       LCMAP_SORTKEY,
                       SortPunctUnsort,
                       -1,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x0e\x02\x0e\x21\x01\x01\x01\x01\x80\x0f\x06\x82",
                             "sortkey (unsortable punct position)",
                             &NumErrors );

    //
    //  Check symbol and nonspace chars.
    //  Make sure:
    //     a-acute, symbol    is the same as
    //     a, acute, symbol
    //

    //  Variation 5  -  sortkey (precomp symbol)
    rc = LCMapStringW( Locale,
                       LCMAP_SORTKEY,
                       SortSymbolPre,
                       -1,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x0e\x02\x07\x2d\x01\x0e\x01\x01\x01",
                             "sortkey (precomp symbol)",
                             &NumErrors );

    //  Variation 6  -  sortkey (comp symbol)
    rc = LCMapStringW( Locale,
                       LCMAP_SORTKEY,
                       SortSymbolComp,
                       -1,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x0e\x02\x07\x2d\x01\x0e\x01\x01\x01",
                             "sortkey (comp symbol)",
                             &NumErrors );

    //
    //  Check symbol and expansion char.
    //  Make sure:
    //     ae, symbol    is the same as
    //     a, e, symbol
    //
    //  Variation 7  -  sortkey (expansion 1 symbol)
    rc = LCMapStringW( Locale,
                       LCMAP_SORTKEY,
                       SortSymbolExp1,
                       -1,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x0e\x02\x0e\x21\x07\x2d\x01\x01\x01\x01",
                             "sortkey (expansion 1 symbol)",
                             &NumErrors );

    //  Variation 8  -  sortkey (expansion 2 symbol)
    rc = LCMapStringW( Locale,
                       LCMAP_SORTKEY,
                       SortSymbolExp2,
                       -1,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x0e\x02\x0e\x21\x07\x2d\x01\x01\x01\x01",
                             "sortkey (expansion 2 symbol)",
                             &NumErrors );

    //
    //  Check symbol and unsortable char.
    //  Make sure:
    //     ae, unsort, symbol    is the same as
    //     a, e, symbol
    //
    //  Variation 9  -  sortkey (unsortable symbol)
    rc = LCMapStringW( Locale,
                       LCMAP_SORTKEY,
                       SortSymbolUnsort,
                       -1,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x0e\x02\x0e\x21\x07\x2d\x01\x01\x01\x01",
                             "sortkey (unsortable symbol)",
                             &NumErrors );


    //
    //  Check Hungarian compression.
    //  Make sure:    ccs is the same as cs+cs
    //
    //  Variation 10  -  sortkey (Hungarian compression)
    rc = LCMapStringW( MAKELCID(0x040e, 0),
                       LCMAP_SORTKEY,
                       L"ccsddzs",
                       -1,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x0e\x0e\x0e\x0e\x0e\x1e\x0e\x1e\x01\x01\x01\x01",
                             "sortkey (hungarian compression)",
                             &NumErrors );


    //
    //  Check expansion (1 to 3).
    //
    //  Variation 11  -  sortkey (expansion 1 to 3)
    rc = LCMapStringW( Locale,
                       LCMAP_SORTKEY,
                       L"\xfb03",
                       -1,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x0e\x23\x0e\x23\x0e\x32\x01\x01\x01\x01",
                             "sortkey (expansion 1 to 3)",
                             &NumErrors );

    //  Variation 12  -  sortkey (expansion 1 to 3)
    rc = LCMapStringW( Locale,
                       LCMAP_SORTKEY,
                       L"ffi",
                       -1,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x0e\x23\x0e\x23\x0e\x32\x01\x01\x01\x01",
                             "sortkey (expansion 1 to 3) 2",
                             &NumErrors );


    //
    //  Check WORD sort versus STRING sort.
    //
    //  Variation 1  -  sortkey (WORD SORT)
    rc = LCMapStringW( MAKELCID(0x040e, 0),
                       LCMAP_SORTKEY,
                       L"co-op",
                       -1,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x0e\x0a\x0e\x7c\x0e\x7c\x0e\x7e\x01\x01\x01\x01\x80\x0f\x06\x82",
                             "sortkey (WORD SORT)",
                             &NumErrors );

    //  Variation 2  -  sortkey (STRING SORT)
    rc = LCMapStringW( MAKELCID(0x040e, 0),
                       LCMAP_SORTKEY | SORT_STRINGSORT,
                       L"co-op",
                       -1,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x0e\x0a\x0e\x7c\x06\x82\x0e\x7c\x0e\x7e\x01\x01\x01\x01",
                             "sortkey (STRING SORT)",
                             &NumErrors );




    //
    //  LCMAP_BYTEREV Flag.
    //

    //  Variation 1  -  byterev
    rc = LCMapStringW( Locale,
                       LCMAP_BYTEREV,
                       L"Th",
                       2,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       2,
                       MapDest,
                       L"\x5400\x6800",
                       "byterev",
                       &NumErrors );

    //  Variation 2  -  byterev, no DestStr
    rc = LCMapStringW( Locale,
                       LCMAP_BYTEREV,
                       L"Th",
                       2,
                       NULL,
                       0 );
    CheckReturnValidW( rc,
                       2,
                       NULL,
                       L"\x5400\x6800",
                       "byterev, no DestStr",
                       &NumErrors );

    // Variation 3  -  sortkey, byterev
    rc = LCMapStringW( Locale,
                       LCMAP_BYTEREV | LCMAP_SORTKEY,
                       L"Th",
                       2,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             10,
                             SortDest,
                             "\x99\x0e\x2c\x0e\x01\x01\x01\x12\x00\x01",
                             "sortkey, byterev",
                             &NumErrors );

    // Variation 4  -  sortkey, byterev (-1)
    rc = LCMapStringW( Locale,
                       LCMAP_BYTEREV | LCMAP_SORTKEY,
                       L"Th",
                       -1,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             10,
                             SortDest,
                             "\x99\x0e\x2c\x0e\x01\x01\x01\x12\x00\x01",
                             "sortkey, byterev (-1)",
                             &NumErrors );

    //  Variation 5  -  byterev, ignore nonspace
    rc = LCMapStringW( Locale,
                       LCMAP_BYTEREV | NORM_IGNORENONSPACE,
                       L"T\x0300h",
                       3,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       2,
                       MapDest,
                       L"\x5400\x6800",
                       "byterev, ignore nonspace",
                       &NumErrors );

    //  Variation 6  -  byterev, ignore symbols
    rc = LCMapStringW( Locale,
                       LCMAP_BYTEREV | NORM_IGNORESYMBOLS,
                       L"T*h",
                       3,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       2,
                       MapDest,
                       L"\x5400\x6800",
                       "byterev, ignore symbols",
                       &NumErrors );

    //  Variation 7  -  byterev, ignore nonspace and symbols
    rc = LCMapStringW( Locale,
                       LCMAP_BYTEREV | NORM_IGNORENONSPACE | NORM_IGNORESYMBOLS,
                       L"*T\x0300h@",
                       5,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       2,
                       MapDest,
                       L"\x5400\x6800",
                       "byterev, ignore nonspace and symbols",
                       &NumErrors );

    //  Variation 8  -  byterev, no DestStr, cchDest 0
    rc = LCMapStringW( 0x0409,
                       LCMAP_BYTEREV,
                       L"Th",
                       2,
                       NULL,
                       0 );
    CheckReturnValidW( rc,
                       2,
                       NULL,
                       L"\x5400\x6800",
                       "byterev, no DestStr",
                       &NumErrors );

    //  Variation 9 -  sortkey | byterev, no DestStr, cchDest 0
    rc = LCMapStringW( 0x0409,
                       LCMAP_SORTKEY | LCMAP_BYTEREV,
                       L"Th",
                       2,
                       NULL,
                       0 );
    CheckReturnValidSortKey( rc,
                             10,
                             NULL,
                             "\x99\x0e\x2c\x0e\x01\x01\x01\x12\x00\x01",
                             "sortkey, byterev (-1), no DestStr",
                             &NumErrors );



    //
    //  LCMAP_HIRAGANA, LCMAP_KATAKANA, LCMAP_HALFWIDTH, LCMAP_FULLWIDTH Flags.
    //

    //  Variation 1  -  hiragana
    rc = LCMapStringW( Locale,
                       LCMAP_HIRAGANA,
                       L"\x30a1Th\x30aa",
                       4,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       4,
                       MapDest,
                       L"\x3041Th\x304a",
                       "hiragana",
                       &NumErrors );

    //  Variation 2  -  katakana
    rc = LCMapStringW( Locale,
                       LCMAP_KATAKANA,
                       L"\x3041Th\x304a",
                       4,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       4,
                       MapDest,
                       L"\x30a1Th\x30aa",
                       "katakana",
                       &NumErrors );

    //  Variation 3  -  halfwidth
    rc = LCMapStringW( Locale,
                       LCMAP_HALFWIDTH,
                       L"\x30a6Th\x3131",
                       4,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       4,
                       MapDest,
                       L"\xff73Th\xffa1",
                       "half width",
                       &NumErrors );

    //  Variation 4  -  fullwidth
    rc = LCMapStringW( Locale,
                       LCMAP_FULLWIDTH,
                       L"\xff61Th\xffca",
                       4,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       4,
                       MapDest,
                       L"\x3002\xff34\xff48\x3155",
                       "full width",
                       &NumErrors );

    //  Variation 5  -  hiragana, half width
    rc = LCMapStringW( Locale,
                       LCMAP_HIRAGANA | LCMAP_HALFWIDTH,
                       L"\x30a1Th\x30aa",
                       4,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       4,
                       MapDest,
                       L"\x3041Th\x304a",
                       "hiragana, half width",
                       &NumErrors );

    //  Variation 6  -  hiragana, full width
    rc = LCMapStringW( Locale,
                       LCMAP_HIRAGANA | LCMAP_FULLWIDTH,
                       L"\x30a1Th\x30aa",
                       4,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       4,
                       MapDest,
                       L"\x3041\xff34\xff48\x304a",
                       "hiragana, full width",
                       &NumErrors );

    //  Variation 7  -  katakana, half width
    rc = LCMapStringW( Locale,
                       LCMAP_KATAKANA | LCMAP_HALFWIDTH,
                       L"\x3041Th\x304a",
                       4,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       4,
                       MapDest,
                       L"\xff67Th\xff75",
                       "katakana, half width",
                       &NumErrors );

    //  Variation 8  -  katakana, full width
    rc = LCMapStringW( Locale,
                       LCMAP_KATAKANA | LCMAP_FULLWIDTH,
                       L"\x3041Th\x304a",
                       4,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       4,
                       MapDest,
                       L"\x30a1\xff34\xff48\x30aa",
                       "katakana, full width",
                       &NumErrors );

    //  Variation 9  -  byterev, hiragana, full width
    rc = LCMapStringW( Locale,
                       LCMAP_BYTEREV | LCMAP_HIRAGANA | LCMAP_FULLWIDTH,
                       L"\x30a1Th\x30aa",
                       4,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       4,
                       MapDest,
                       L"\x4130\x34ff\x48ff\x4a30",
                       "byterev, hiragana, full width",
                       &NumErrors );

    //  Variation 10 -  byterev, katakana, full width
    rc = LCMapStringW( Locale,
                       LCMAP_BYTEREV | LCMAP_KATAKANA | LCMAP_FULLWIDTH,
                       L"\x3041Th\x304a",
                       4,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       4,
                       MapDest,
                       L"\xa130\x34ff\x48ff\xaa30",
                       "byterev, katakana, full width",
                       &NumErrors );

    //  Variation 11  -  uppercase, katakana
    rc = LCMapStringW( Locale,
                       LCMAP_UPPERCASE | LCMAP_KATAKANA,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       MapUpper,
                       "uppercase, katakana",
                       &NumErrors );


    //  Variation 12 -  uppercase, half width
    rc = LCMapStringW( Locale,
                       LCMAP_UPPERCASE | LCMAP_HALFWIDTH,
                       MapSrc1,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       MapUpper,
                       "uppercase, half width",
                       &NumErrors );




    //
    //  LCMAP_HIRAGANA, LCMAP_KATAKANA, LCMAP_HALFWIDTH, LCMAP_FULLWIDTH Flags.
    //  cchDest == 0
    //

    //  Variation 1  -  hiragana
    rc = LCMapStringW( Locale,
                       LCMAP_HIRAGANA,
                       L"\x30a1Th\x30aa",
                       4,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       4,
                       NULL,
                       L"\x3041Th\x304a",
                       "hiragana (cchDest = 0)",
                       &NumErrors );

    //  Variation 2  -  katakana
    rc = LCMapStringW( Locale,
                       LCMAP_KATAKANA,
                       L"\x3041Th\x304a",
                       4,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       4,
                       NULL,
                       L"\x30a1Th\x30aa",
                       "katakana (cchDest = 0)",
                       &NumErrors );

    //  Variation 3  -  halfwidth
    rc = LCMapStringW( Locale,
                       LCMAP_HALFWIDTH,
                       L"\x30a6Th\x3131",
                       4,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       4,
                       NULL,
                       L"\xff73Th\xffa1",
                       "half width (cchDest = 0)",
                       &NumErrors );

    //  Variation 4  -  fullwidth
    rc = LCMapStringW( Locale,
                       LCMAP_FULLWIDTH,
                       L"\xff61Th\xffca",
                       4,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       4,
                       NULL,
                       L"\x3002\xff34\xff48\x3155",
                       "full width (cchDest = 0)",
                       &NumErrors );

    //  Variation 5  -  hiragana, half width
    rc = LCMapStringW( Locale,
                       LCMAP_HIRAGANA | LCMAP_HALFWIDTH,
                       L"\x30a1Th\x30aa",
                       4,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       4,
                       NULL,
                       L"\x3041Th\x304a",
                       "hiragana, half width (cchDest = 0)",
                       &NumErrors );

    //  Variation 6  -  hiragana, full width
    rc = LCMapStringW( Locale,
                       LCMAP_HIRAGANA | LCMAP_FULLWIDTH,
                       L"\x30a1Th\x30aa",
                       4,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       4,
                       NULL,
                       L"\x3041\xff34\xff48\x304a",
                       "hiragana, full width (cchDest = 0)",
                       &NumErrors );

    //  Variation 7  -  katakana, half width
    rc = LCMapStringW( Locale,
                       LCMAP_KATAKANA | LCMAP_HALFWIDTH,
                       L"\x3041Th\x304a",
                       4,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       4,
                       NULL,
                       L"\xff67Th\xff75",
                       "katakana, half width (cchDest = 0)",
                       &NumErrors );

    //  Variation 8  -  katakana, full width
    rc = LCMapStringW( Locale,
                       LCMAP_KATAKANA | LCMAP_FULLWIDTH,
                       L"\x3041Th\x304a",
                       4,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       4,
                       NULL,
                       L"\x30a1\xff34\xff48\x30aa",
                       "katakana, full width (cchDest = 0)",
                       &NumErrors );

    //  Variation 9  -  byterev, hiragana, full width
    rc = LCMapStringW( Locale,
                       LCMAP_BYTEREV | LCMAP_HIRAGANA | LCMAP_FULLWIDTH,
                       L"\x30a1Th\x30aa",
                       4,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       4,
                       NULL,
                       L"\x4130\x34ff\x48ff\x4a30",
                       "byterev, hiragana, full width (cchDest = 0)",
                       &NumErrors );

    //  Variation 10 -  byterev, katakana, full width
    rc = LCMapStringW( Locale,
                       LCMAP_BYTEREV | LCMAP_KATAKANA | LCMAP_FULLWIDTH,
                       L"\x3041Th\x304a",
                       4,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       4,
                       NULL,
                       L"\xa130\x34ff\x48ff\xaa30",
                       "byterev, katakana, full width (cchDest = 0)",
                       &NumErrors );

    //  Variation 11  -  uppercase, katakana
    rc = LCMapStringW( Locale,
                       LCMAP_UPPERCASE | LCMAP_KATAKANA,
                       MapSrc1,
                       -1,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       MapUpper,
                       "uppercase, katakana (cchDest = 0)",
                       &NumErrors );


    //  Variation 12 -  uppercase, half width
    rc = LCMapStringW( Locale,
                       LCMAP_UPPERCASE | LCMAP_HALFWIDTH,
                       MapSrc1,
                       -1,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       MapUpper,
                       "uppercase, half width (cchDest = 0)",
                       &NumErrors );



    //
    //  Kana and Width - precomposed forms.
    //

    //  Variation 1 -  half width
    rc = LCMapStringW( Locale,
                       LCMAP_HALFWIDTH,
                       L"\x30d0\x30d1",
                       2,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       4,
                       MapDest,
                       L"\xff8a\xff9e\xff8a\xff9f",
                       "half precomp",
                       &NumErrors );

    //  Variation 2 -  half width, hiragana
    rc = LCMapStringW( Locale,
                       LCMAP_HALFWIDTH | LCMAP_HIRAGANA,
                       L"\x30d0\x30d1",
                       2,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       2,
                       MapDest,
                       L"\x3070\x3071",
                       "half, hiragana precomp",
                       &NumErrors );

    //  Variation 3 -  half width, katakana
    rc = LCMapStringW( Locale,
                       LCMAP_HALFWIDTH | LCMAP_KATAKANA,
                       L"\x30d0\x30d1",
                       2,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       4,
                       MapDest,
                       L"\xff8a\xff9e\xff8a\xff9f",
                       "half, katakana precomp",
                       &NumErrors );

    //  Variation 4 -  case, half width, katakana
    rc = LCMapStringW( Locale,
                       LCMAP_HALFWIDTH | LCMAP_KATAKANA | LCMAP_LOWERCASE,
                       L"\xff2a\x30d0\xff2a\x30d1J",
                       5,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       7,
                       MapDest,
                       L"j\xff8a\xff9ej\xff8a\xff9fj",
                       "case, half, katakana precomp",
                       &NumErrors );

    //  Variation 5 -  full width
    rc = LCMapStringW( Locale,
                       LCMAP_FULLWIDTH,
                       L"\x30d0\x30d1",
                       2,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       2,
                       MapDest,
                       L"\x30d0\x30d1",
                       "full precomp",
                       &NumErrors );

    //  Variation 6 -  full width, hiragana
    rc = LCMapStringW( Locale,
                       LCMAP_FULLWIDTH | LCMAP_HIRAGANA,
                       L"\x30d0\x30d1",
                       2,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       2,
                       MapDest,
                       L"\x3070\x3071",
                       "full, hiragana precomp",
                       &NumErrors );

    //  Variation 7 -  full width, katakana
    rc = LCMapStringW( Locale,
                       LCMAP_FULLWIDTH | LCMAP_KATAKANA,
                       L"\x30d0\x30d1",
                       2,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       2,
                       MapDest,
                       L"\x30d0\x30d1",
                       "full, katakana precomp",
                       &NumErrors );



    //
    //  Kana and Width - precomposed forms.
    //  cchDest = 0
    //

    //  Variation 1 -  half width
    rc = LCMapStringW( Locale,
                       LCMAP_HALFWIDTH,
                       L"\x30d0\x30d1",
                       2,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       4,
                       NULL,
                       L"\xff8a\xff9e\xff8a\xff9f",
                       "half precomp (cchDest = 0)",
                       &NumErrors );

    //  Variation 2 -  half width, hiragana
    rc = LCMapStringW( Locale,
                       LCMAP_HALFWIDTH | LCMAP_HIRAGANA,
                       L"\x30d0\x30d1",
                       2,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       2,
                       NULL,
                       L"\x3070\x3071",
                       "half, hiragana precomp (cchDest = 0)",
                       &NumErrors );

    //  Variation 3 -  half width, katakana
    rc = LCMapStringW( Locale,
                       LCMAP_HALFWIDTH | LCMAP_KATAKANA,
                       L"\x30d0\x30d1",
                       2,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       4,
                       NULL,
                       L"\xff8a\xff9e\xff8a\xff9f",
                       "half, katakana precomp (cchDest = 0)",
                       &NumErrors );

    //  Variation 4 -  case, half width, katakana
    rc = LCMapStringW( Locale,
                       LCMAP_HALFWIDTH | LCMAP_KATAKANA | LCMAP_LOWERCASE,
                       L"\xff2a\x30d0\xff2a\x30d1J",
                       5,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       7,
                       NULL,
                       L"j\xff8a\xff9ej\xff8a\xff9fj",
                       "case, half, katakana precomp (cchDest = 0)",
                       &NumErrors );

    //  Variation 5 -  full width
    rc = LCMapStringW( Locale,
                       LCMAP_FULLWIDTH,
                       L"\x30d0\x30d1",
                       2,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       2,
                       NULL,
                       L"\x30d0\x30d1",
                       "full precomp",
                       &NumErrors );

    //  Variation 6 -  full width, hiragana
    rc = LCMapStringW( Locale,
                       LCMAP_FULLWIDTH | LCMAP_HIRAGANA,
                       L"\x30d0\x30d1",
                       2,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       2,
                       NULL,
                       L"\x3070\x3071",
                       "full, hiragana precomp (cchDest = 0)",
                       &NumErrors );

    //  Variation 7 -  full width, katakana
    rc = LCMapStringW( Locale,
                       LCMAP_FULLWIDTH | LCMAP_KATAKANA,
                       L"\x30d0\x30d1",
                       2,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       2,
                       NULL,
                       L"\x30d0\x30d1",
                       "full, katakana precomp (cchDest = 0)",
                       &NumErrors );



    //
    //  Kana and Width - composite forms.
    //

    //  Variation 1 -  half width
    rc = LCMapStringW( Locale,
                       LCMAP_HALFWIDTH,
                       L"\x30cf\x309b\x30cf\x309c",
                       4,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       4,
                       MapDest,
                       L"\xff8a\xff9e\xff8a\xff9f",
                       "half comp",
                       &NumErrors );

    //  Variation 2 -  half width, hiragana
    rc = LCMapStringW( Locale,
                       LCMAP_HALFWIDTH | LCMAP_HIRAGANA,
                       L"\x30cf\x309b\x30cf\x309c",
                       4,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       4,
                       MapDest,
                       L"\x306f\xff9e\x306f\xff9f",
                       "half, hiragana comp",
                       &NumErrors );

    //  Variation 3 -  half width, katakana
    rc = LCMapStringW( Locale,
                       LCMAP_HALFWIDTH | LCMAP_KATAKANA,
                       L"\x30cf\x309b\x30cf\x309c",
                       4,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       4,
                       MapDest,
                       L"\xff8a\xff9e\xff8a\xff9f",
                       "half, katakana comp",
                       &NumErrors );

    //  Variation 4 -  case, half width, katakana
    rc = LCMapStringW( Locale,
                       LCMAP_HALFWIDTH | LCMAP_KATAKANA | LCMAP_LOWERCASE,
                       L"\xff2a\x30cf\x309b\xff2a\x30cf\x309cJ",
                       7,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       7,
                       MapDest,
                       L"j\xff8a\xff9ej\xff8a\xff9fj",
                       "case, half, katakana comp",
                       &NumErrors );

    //  Variation 5 -  full width
    rc = LCMapStringW( Locale,
                       LCMAP_FULLWIDTH,
                       L"\x30cf\x309b\x30cf\x309c",
                       4,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       2,
                       MapDest,
                       L"\x30d0\x30d1",
                       "full comp",
                       &NumErrors );

    //  Variation 6 -  full width, hiragana
    rc = LCMapStringW( Locale,
                       LCMAP_FULLWIDTH | LCMAP_HIRAGANA,
                       L"\x30cf\x309b\x30cf\x309c",
                       4,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       2,
                       MapDest,
                       L"\x3070\x3071",
                       "full, hiragana comp",
                       &NumErrors );

    //  Variation 7 -  full width, katakana
    rc = LCMapStringW( Locale,
                       LCMAP_FULLWIDTH | LCMAP_KATAKANA,
                       L"\x30cf\x309b\x30cf\x309c",
                       4,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       2,
                       MapDest,
                       L"\x30d0\x30d1",
                       "full, katakana comp",
                       &NumErrors );

    //  Variation 8 -  case, full width, katakana
    rc = LCMapStringW( Locale,
                       LCMAP_FULLWIDTH | LCMAP_KATAKANA | LCMAP_LOWERCASE,
                       L"\xff2a\x30cf\x309bJ\x30cf\x309cJ",
                       7,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       5,
                       MapDest,
                       L"\xff4a\x30d0\xff4a\x30d1\xff4a",
                       "case, full, katakana comp",
                       &NumErrors );


    //
    //  Kana and Width - composite forms.
    //  cchDest == 0
    //

    //  Variation 1 -  half width
    rc = LCMapStringW( Locale,
                       LCMAP_HALFWIDTH,
                       L"\x30cf\x309b\x30cf\x309c",
                       4,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       4,
                       NULL,
                       L"\xff8a\xff9e\xff8a\xff9f",
                       "half comp (cchDest = 0)",
                       &NumErrors );

    //  Variation 2 -  half width, hiragana
    rc = LCMapStringW( Locale,
                       LCMAP_HALFWIDTH | LCMAP_HIRAGANA,
                       L"\x30cf\x309b\x30cf\x309c",
                       4,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       4,
                       NULL,
                       L"\x306f\xff9e\x306f\xff9f",
                       "half, hiragana comp (cchDest = 0)",
                       &NumErrors );

    //  Variation 3 -  half width, katakana
    rc = LCMapStringW( Locale,
                       LCMAP_HALFWIDTH | LCMAP_KATAKANA,
                       L"\x30cf\x309b\x30cf\x309c",
                       4,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       4,
                       NULL,
                       L"\xff8a\xff9e\xff8a\xff9f",
                       "half, katakana comp (cchDest = 0)",
                       &NumErrors );

    //  Variation 4 -  case, half width, katakana
    rc = LCMapStringW( Locale,
                       LCMAP_HALFWIDTH | LCMAP_KATAKANA | LCMAP_LOWERCASE,
                       L"\xff2a\x30cf\x309b\xff2a\x30cf\x309cJ",
                       7,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       7,
                       NULL,
                       L"j\xff8a\xff9ej\xff8a\xff9fj",
                       "case, half, katakana comp (cchDest = 0)",
                       &NumErrors );

    //  Variation 5 -  full width
    rc = LCMapStringW( Locale,
                       LCMAP_FULLWIDTH,
                       L"\x30cf\x309b\x30cf\x309c",
                       4,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       2,
                       NULL,
                       L"\x30d0\x30d1",
                       "full comp (cchDest = 0)",
                       &NumErrors );

    //  Variation 6 -  full width, hiragana
    rc = LCMapStringW( Locale,
                       LCMAP_FULLWIDTH | LCMAP_HIRAGANA,
                       L"\x30cf\x309b\x30cf\x309c",
                       4,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       2,
                       NULL,
                       L"\x3070\x3071",
                       "full, hiragana comp (cchDest = 0)",
                       &NumErrors );

    //  Variation 7 -  full width, katakana
    rc = LCMapStringW( Locale,
                       LCMAP_FULLWIDTH | LCMAP_KATAKANA,
                       L"\x30cf\x309b\x30cf\x309c",
                       4,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       2,
                       NULL,
                       L"\x30d0\x30d1",
                       "full, katakana comp (cchDest = 0)",
                       &NumErrors );

    //  Variation 8 -  case, full width, katakana
    rc = LCMapStringW( Locale,
                       LCMAP_FULLWIDTH | LCMAP_KATAKANA | LCMAP_LOWERCASE,
                       L"\xff2a\x30cf\x309bJ\x30cf\x309cJ",
                       7,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       5,
                       NULL,
                       L"\xff4a\x30d0\xff4a\x30d1\xff4a",
                       "case, full, katakana comp (cchDest = 0)",
                       &NumErrors );



    //
    //  Kana and Width - half width composite forms.
    //

    //  Variation 1 -  half width
    rc = LCMapStringW( Locale,
                       LCMAP_HALFWIDTH,
                       L"\xff8a\xff9e\xff8a\xff9f",
                       4,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       4,
                       MapDest,
                       L"\xff8a\xff9e\xff8a\xff9f",
                       "half comp (half)",
                       &NumErrors );

    //  Variation 2 -  half width, hiragana
    rc = LCMapStringW( Locale,
                       LCMAP_HALFWIDTH | LCMAP_HIRAGANA,
                       L"\xff8a\xff9e\xff8a\xff9f",
                       4,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       4,
                       MapDest,
                       L"\xff8a\xff9e\xff8a\xff9f",
                       "half, hiragana comp (half)",
                       &NumErrors );

    //  Variation 3 -  half width, katakana
    rc = LCMapStringW( Locale,
                       LCMAP_HALFWIDTH | LCMAP_KATAKANA,
                       L"\xff8a\xff9e\xff8a\xff9f",
                       4,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       4,
                       MapDest,
                       L"\xff8a\xff9e\xff8a\xff9f",
                       "half, katakana comp (half)",
                       &NumErrors );

    //  Variation 4 -  full width
    rc = LCMapStringW( Locale,
                       LCMAP_FULLWIDTH,
                       L"\xff8a\xff9e\xff8a\xff9f",
                       4,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       2,
                       MapDest,
                       L"\x30d0\x30d1",
                       "full comp (half)",
                       &NumErrors );

    //  Variation 5 -  full width, hiragana
    rc = LCMapStringW( Locale,
                       LCMAP_FULLWIDTH | LCMAP_HIRAGANA,
                       L"\xff8a\xff9e\xff8a\xff9f",
                       4,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       2,
                       MapDest,
                       L"\x3070\x3071",
                       "full, hiragana comp (half)",
                       &NumErrors );

    //  Variation 6 -  full width, katakana
    rc = LCMapStringW( Locale,
                       LCMAP_FULLWIDTH | LCMAP_KATAKANA,
                       L"\xff8a\xff9e\xff8a\xff9f",
                       4,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       2,
                       MapDest,
                       L"\x30d0\x30d1",
                       "full, katakana comp (half)",
                       &NumErrors );

    //  Variation 7 -  case, full width, katakana
    rc = LCMapStringW( Locale,
                       LCMAP_FULLWIDTH | LCMAP_KATAKANA | LCMAP_LOWERCASE,
                       L"\xff2a\xff8a\xff9eJ\xff8a\xff9fJ",
                       7,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       5,
                       MapDest,
                       L"\xff4a\x30d0\xff4a\x30d1\xff4a",
                       "case, full, katakana comp (half)",
                       &NumErrors );


    //
    //  Kana and Width - half width composite forms.
    //  cchDest == 0
    //

    //  Variation 1 -  half width
    rc = LCMapStringW( Locale,
                       LCMAP_HALFWIDTH,
                       L"\xff8a\xff9e\xff8a\xff9f",
                       4,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       4,
                       NULL,
                       L"\xff8a\xff9e\xff8a\xff9f",
                       "half comp (half) (cchDest = 0)",
                       &NumErrors );

    //  Variation 2 -  half width, hiragana
    rc = LCMapStringW( Locale,
                       LCMAP_HALFWIDTH | LCMAP_HIRAGANA,
                       L"\xff8a\xff9e\xff8a\xff9f",
                       4,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       4,
                       NULL,
                       L"\xff8a\xff9e\xff8a\xff9f",
                       "half, hiragana comp (half) (cchDest = 0)",
                       &NumErrors );

    //  Variation 3 -  half width, katakana
    rc = LCMapStringW( Locale,
                       LCMAP_HALFWIDTH | LCMAP_KATAKANA,
                       L"\xff8a\xff9e\xff8a\xff9f",
                       4,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       4,
                       NULL,
                       L"\xff8a\xff9e\xff8a\xff9f",
                       "half, katakana comp (half) (cchDest = 0)",
                       &NumErrors );

    //  Variation 4 -  full width
    rc = LCMapStringW( Locale,
                       LCMAP_FULLWIDTH,
                       L"\xff8a\xff9e\xff8a\xff9f",
                       4,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       2,
                       NULL,
                       L"\x30d0\x30d1",
                       "full comp (half) (cchDest = 0)",
                       &NumErrors );

    //  Variation 5 -  full width, hiragana
    rc = LCMapStringW( Locale,
                       LCMAP_FULLWIDTH | LCMAP_HIRAGANA,
                       L"\xff8a\xff9e\xff8a\xff9f",
                       4,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       2,
                       NULL,
                       L"\x3070\x3071",
                       "full, hiragana comp (half) (cchDest = 0)",
                       &NumErrors );

    //  Variation 6 -  full width, katakana
    rc = LCMapStringW( Locale,
                       LCMAP_FULLWIDTH | LCMAP_KATAKANA,
                       L"\xff8a\xff9e\xff8a\xff9f",
                       4,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       2,
                       NULL,
                       L"\x30d0\x30d1",
                       "full, katakana comp (half) (cchDest = 0)",
                       &NumErrors );

    //  Variation 7 -  case, full width, katakana
    rc = LCMapStringW( Locale,
                       LCMAP_FULLWIDTH | LCMAP_KATAKANA | LCMAP_LOWERCASE,
                       L"\xff2a\xff8a\xff9eJ\xff8a\xff9fJ",
                       7,
                       MapDest,
                       0 );
    CheckReturnValidW( rc,
                       5,
                       NULL,
                       L"\xff4a\x30d0\xff4a\x30d1\xff4a",
                       "case, full, katakana comp (half) (cchDest = 0)",
                       &NumErrors );



    //
    //  LCMAP_TRADITIONAL_CHINESE Flag.
    //

    //  Variation 1  -  map simplified to traditional
    rc = LCMapStringW( Locale,
                       LCMAP_TRADITIONAL_CHINESE,
                       L"\x4e07\x4e0e\x9f95\x9f9f",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\x842c\x8207\x9f95\x9f9c",
                       "traditional",
                       &NumErrors );

    //  Variation 2  -  map simplified to traditional
    rc = LCMapStringW( Locale,
                       LCMAP_TRADITIONAL_CHINESE | LCMAP_LOWERCASE,
                       L"\x4e07\x4e0e\x9f95\x9f9f",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\x842c\x8207\x9f95\x9f9c",
                       "traditional, lowercase",
                       &NumErrors );

    //  Variation 3  -  map simplified to traditional
    rc = LCMapStringW( Locale,
                       LCMAP_TRADITIONAL_CHINESE | LCMAP_LOWERCASE,
                       L"YYz\x4e07\x4e0e\x9f95\x9f9fYzY",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"yyz\x842c\x8207\x9f95\x9f9cyzy",
                       "traditional, lowercase 2",
                       &NumErrors );

    //  Variation 4  -  map simplified to traditional
    rc = LCMapStringW( Locale,
                       LCMAP_TRADITIONAL_CHINESE | LCMAP_UPPERCASE,
                       L"\x4e07\x4e0e\x9f95\x9f9f",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\x842c\x8207\x9f95\x9f9c",
                       "traditional, uppercase",
                       &NumErrors );

    //  Variation 5  -  map simplified to traditional
    rc = LCMapStringW( Locale,
                       LCMAP_TRADITIONAL_CHINESE | LCMAP_UPPERCASE,
                       L"Yyz\x4e07\x4e0e\x9f95\x9f9fyzY",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"YYZ\x842c\x8207\x9f95\x9f9cYZY",
                       "traditional, uppercase 2",
                       &NumErrors );



    //
    //  LCMAP_SIMPLIFIED_CHINESE Flag.
    //

    //  Variation 1  -  map traditional to simplified
    rc = LCMapStringW( Locale,
                       LCMAP_SIMPLIFIED_CHINESE,
                       L"\x4e1f\xfa26\x9038",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\x4e22\x90fd\x9038",
                       "simplified",
                       &NumErrors );

    //  Variation 2  -  map traditional to simplified
    rc = LCMapStringW( Locale,
                       LCMAP_SIMPLIFIED_CHINESE | LCMAP_LOWERCASE,
                       L"\x4e1f\xfa26\x9038",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\x4e22\x90fd\x9038",
                       "simplified, lowercase",
                       &NumErrors );

    //  Variation 3  -  map traditional to simplified
    rc = LCMapStringW( Locale,
                       LCMAP_SIMPLIFIED_CHINESE | LCMAP_LOWERCASE,
                       L"YYz\x4e1f\xfa26\x9038YzY",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"yyz\x4e22\x90fd\x9038yzy",
                       "simplified, lowercase 2",
                       &NumErrors );

    //  Variation 4  -  map traditional to simplified
    rc = LCMapStringW( Locale,
                       LCMAP_SIMPLIFIED_CHINESE | LCMAP_UPPERCASE,
                       L"\x4e1f\xfa26\x9038",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\x4e22\x90fd\x9038",
                       "simplified, uppercase",
                       &NumErrors );

    //  Variation 5  -  map traditional to simplified
    rc = LCMapStringW( Locale,
                       LCMAP_SIMPLIFIED_CHINESE | LCMAP_UPPERCASE,
                       L"Yyz\x4e1f\xfa26\x9038yzY",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"YYZ\x4e22\x90fd\x9038YZY",
                       "simplified, uppercase 2",
                       &NumErrors );



    //
    //  Japanese sortkey tests - CHO-ON.
    //

    //  Variation 1  -  cho-on
    rc = LCMapStringW( 0x0409,
                       LCMAP_SORTKEY,
                       L"\x30f6\x30fc",
                       2,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x22\x0d\x22\x05\x01\x01\x01\xc4\xc4\xff\x03\x05\x02\xc4\xc4\xff\xff\x01",
                             "cho-on",
                             &NumErrors );

    //  Variation 2  -  cho-on first char
    rc = LCMapStringW( 0x0409,
                       LCMAP_SORTKEY,
                       L"\x30fc\x30f6\x30fc",
                       3,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\xff\xff\x22\x0d\x22\x05\x01\x01\x01\xc4\xc4\xff\x03\x05\x02\xc4\xc4\xff\xff\x01",
                             "cho-on first char",
                             &NumErrors );

    //  Variation 3  -  cho-on first and second char
    rc = LCMapStringW( 0x0409,
                       LCMAP_SORTKEY,
                       L"\x30fc\x30fc\x30f6\x30fc",
                       4,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\xff\xff\xff\xff\x22\x0d\x22\x05\x01\x01\x01\xc4\xc4\xff\x03\x05\x02\xc4\xc4\xff\xff\x01",
                             "cho-on first and second char",
                             &NumErrors );


    //  Variation 4  -  cho-on with ignore nonspace flag
    rc = LCMapStringW( 0x0411,
                       LCMAP_SORTKEY | NORM_IGNORENONSPACE,
                       L"\xff76\xff9e\xff70",
                       -1,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x22\x0a\x22\x02\x01\x01\x01\xff\x02\xc4\xc4\xff\xc4\xc4\xff\x01",
                             "cho-on with ignore nonspace flag",
                             &NumErrors );

    //  Variation 5  -  cho-on without ignore nonspace flag
    rc = LCMapStringW( 0x0411,
                       LCMAP_SORTKEY,
                       L"\xff76\xff9e\xff70",
                       -1,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x22\x0a\x22\x02\x01\x03\x01\x01\xff\x03\x05\x02\xc4\xc4\xff\xc4\xc4\xff\x01",
                             "cho-on without ignore nonspace flag",
                             &NumErrors );

    //  Variation 6  -  cho-on with ignore nonspace flag
    rc = LCMapStringW( 0x0411,
                       LCMAP_SORTKEY | NORM_IGNORENONSPACE,
                       L"\xff76\xff9e\xff71",
                       -1,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x22\x0a\x22\x02\x01\x01\x01\xff\x02\xc4\xc4\xff\xc4\xc4\xff\x01",
                             "cho-on with ignore nonspace flag 2",
                             &NumErrors );

    //  Variation 7  -  cho-on without ignore nonspace flag
    rc = LCMapStringW( 0x0411,
                       LCMAP_SORTKEY,
                       L"\xff76\xff9e\xff71",
                       -1,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x22\x0a\x22\x02\x01\x03\x01\x01\xff\x02\xc4\xc4\xff\xc4\xc4\xff\x01",
                             "cho-on without ignore nonspace flag 2",
                             &NumErrors );



    //
    //  Japanese sortkey tests - REPEAT.
    //

    //  Variation 1  -  repeat
    rc = LCMapStringW( 0x0409,
                       LCMAP_SORTKEY,
                       L"\x30f6\x30fd",
                       2,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x22\x0d\x22\x0d\x01\x01\x01\xc4\xc4\xff\x03\x04\x02\xc4\xc4\xff\xff\x01",
                             "repeat",
                             &NumErrors );

    rc = LCMapStringW( 0x0409,
                       LCMAP_SORTKEY,
                       L"\x30f6\x30f6",
                       2,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x22\x0d\x22\x0d\x01\x01\x01\xc4\xc4\xff\x02\xc4\xc4\xff\xff\x01",
                             "repeat (actual)",
                             &NumErrors );


    //  Variation 2  -  repeat first char
    rc = LCMapStringW( 0x0409,
                       LCMAP_SORTKEY,
                       L"\x30fd\x30f6\x30fd",
                       3,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\xff\xff\x22\x0d\x22\x0d\x01\x01\x01\xc4\xc4\xff\x03\x04\x02\xc4\xc4\xff\xff\x01",
                             "repeat first char",
                             &NumErrors );

    //  Variation 3  -  repeat first and second char
    rc = LCMapStringW( 0x0409,
                       LCMAP_SORTKEY,
                       L"\x30fd\x30fd\x30f6\x30fd",
                       4,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\xff\xff\xff\xff\x22\x0d\x22\x0d\x01\x01\x01\xc4\xc4\xff\x03\x04\x02\xc4\xc4\xff\xff\x01",
                             "repeat first and second char",
                             &NumErrors );

    //  Variation 4  -  repeat (0x30fb)
    rc = LCMapStringW( 0x0409,
                       LCMAP_SORTKEY,
                       L"\x30fb\x30fd",
                       2,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x0a\x0e\x0a\x0e\x01\x01\x03\x01\x01",
                             "repeat (0x30fb)",
                             &NumErrors );

    //  Variation 5  -  repeat alone
    rc = LCMapStringW( 0x0409,
                       LCMAP_SORTKEY,
                       L"\x3094\x309d",
                       2,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x22\x04\x22\x04\x01\x03\x01\x01\xff\x03\x04\x02\xff\xff\x01",
                             "repeat alone",
                             &NumErrors );

    //  Variation 6  -  repeat, ignore kana
    rc = LCMapStringW( 0x0409,
                       LCMAP_SORTKEY | NORM_IGNOREKANATYPE,
                       L"\x3094\x309d",
                       2,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x22\x04\x22\x04\x01\x03\x01\x01\xff\x03\x04\x02\xc4\xc4\xff\xff\x01",
                             "repeat, ignore kana",
                             &NumErrors );

    //  Variation 7  -  repeat, ignore width
    rc = LCMapStringW( 0x0409,
                       LCMAP_SORTKEY | NORM_IGNOREWIDTH,
                       L"\x3094\x309d",
                       2,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x22\x04\x22\x04\x01\x03\x01\x01\xff\x03\x04\x02\xff\xc4\xc4\xff\x01",
                             "repeat, ignore width",
                             &NumErrors );

    //  Variation 8  -  repeat, ignore kana and width
    rc = LCMapStringW( 0x0409,
                       LCMAP_SORTKEY | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH,
                       L"\x3094\x309d",
                       2,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x22\x04\x22\x04\x01\x03\x01\x01\xff\x03\x04\x02\xc4\xc4\xff\xc4\xc4\xff\x01",
                             "repeat, ignore kana and width",
                             &NumErrors );



    //
    //  Test for upper case.
    //

    //  Variation 1  -  uppercase
    rc = LCMapStringW( 0x0409,
                       LCMAP_UPPERCASE,
                       L"\x00e7",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\x00c7",
                       "uppercase check",
                       &NumErrors );


    //
    //  More Japanese Tests.
    //
    rc = LCMapStringW( 0x0411,
                       LCMAP_FULLWIDTH,
                       L"\x0020",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\x3000",
                       "Japanese Test 1",
                       &NumErrors );

    rc = LCMapStringW( 0x0411,
                       LCMAP_FULLWIDTH,
                       L"\x0022",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\xff02",
                       "Japanese Test 2",
                       &NumErrors );

    rc = LCMapStringW( 0x0411,
                       LCMAP_FULLWIDTH,
                       L"\x0027",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\xff07",
                       "Japanese Test 3",
                       &NumErrors );

    rc = LCMapStringW( 0x0411,
                       LCMAP_FULLWIDTH,
                       L"\x005c",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\x005c",
                       "Japanese Test 4",
                       &NumErrors );

    rc = LCMapStringW( 0x0411,
                       LCMAP_FULLWIDTH,
                       L"\x007e",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\xff5e",
                       "Japanese Test 5",
                       &NumErrors );

    rc = LCMapStringW( 0x0411,
                       LCMAP_FULLWIDTH,
                       L"\x30fb",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\x30fb",
                       "Japanese Test 6",
                       &NumErrors );

    rc = LCMapStringW( 0x0411,
                       LCMAP_FULLWIDTH,
                       L"\x3046\xff9e",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\x3094",
                       "Japanese Test 7",
                       &NumErrors );

    rc = LCMapStringW( 0x0411,
                       LCMAP_FULLWIDTH,
                       L"\x3046\x309b",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\x3094",
                       "Japanese Test 8",
                       &NumErrors );

    rc = LCMapStringW( 0x0411,
                       LCMAP_FULLWIDTH,
                       L"\x304b\xff9e",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\x304c",
                       "Japanese Test 9",
                       &NumErrors );

    rc = LCMapStringW( 0x0411,
                       LCMAP_FULLWIDTH,
                       L"\x306f\xff9f",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\x3071",
                       "Japanese Test 10",
                       &NumErrors );

    rc = LCMapStringW( 0x0411,
                       LCMAP_HALFWIDTH,
                       L"\x3000",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\x0020",
                       "Japanese Test 11",
                       &NumErrors );

    rc = LCMapStringW( 0x0411,
                       LCMAP_HALFWIDTH,
                       L"\xffe3",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\x00af",
                       "Japanese Test 12",
                       &NumErrors );

    rc = LCMapStringW( 0x0411,
                       LCMAP_HALFWIDTH,
                       L"\x2015",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\x2015",
                       "Japanese Test 13",
                       &NumErrors );

    rc = LCMapStringW( 0x0411,
                       LCMAP_HALFWIDTH,
                       L"\xff3c",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\xff3c",
                       "Japanese Test 14",
                       &NumErrors );

    rc = LCMapStringW( 0x0411,
                       LCMAP_HALFWIDTH,
                       L"\xff5e",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\x007e",
                       "Japanese Test 15",
                       &NumErrors );

    rc = LCMapStringW( 0x0411,
                       LCMAP_HALFWIDTH,
                       L"\x2018",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"'",
                       "Japanese Test 16",
                       &NumErrors );

    rc = LCMapStringW( 0x0411,
                       LCMAP_HALFWIDTH,
                       L"\x2019",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"'",
                       "Japanese Test 17",
                       &NumErrors );

    rc = LCMapStringW( 0x0411,
                       LCMAP_HALFWIDTH,
                       L"\x201c",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\"",
                       "Japanese Test 18",
                       &NumErrors );

    rc = LCMapStringW( 0x0411,
                       LCMAP_HALFWIDTH,
                       L"\x201d",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\"",
                       "Japanese Test 19",
                       &NumErrors );

    rc = LCMapStringW( 0x0411,
                       LCMAP_HALFWIDTH,
                       L"\x30fb",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\xff65",
                       "Japanese Test 20",
                       &NumErrors );

    rc = LCMapStringW( 0x0411,
                       LCMAP_HALFWIDTH,
                       L"\xffe5",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\x00a5",
                       "Japanese Test 21",
                       &NumErrors );

    rc = LCMapStringW( 0x0411,
                       LCMAP_HIRAGANA | LCMAP_LOWERCASE,
                       L"\x30fb",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\x30fb",
                       "Japanese Test 22",
                       &NumErrors );

    rc = LCMapStringW( 0x0411,
                       LCMAP_HIRAGANA,
                       L"\x30f4",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\x3094",
                       "Japanese Test 23",
                       &NumErrors );

    rc = LCMapStringW( 0x0411,
                       LCMAP_HIRAGANA,
                       L"\x30f4\xff9e",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\x3094\xff9e",
                       "Japanese Test 24",
                       &NumErrors );

    rc = LCMapStringW( 0x0411,
                       LCMAP_UPPERCASE,
                       L"\x3046\xff9e",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\x3046\xff9e",
                       "Japanese Test 25",
                       &NumErrors );

    rc = LCMapStringW( 0x0411,
                       LCMAP_UPPERCASE,
                       L"\x304b\xff9e",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\x304b\xff9e",
                       "Japanese Test 26",
                       &NumErrors );

    rc = LCMapStringW( 0x0411,
                       LCMAP_UPPERCASE,
                       L"\x304b\x309b",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\x304b\x309b",
                       "Japanese Test 27",
                       &NumErrors );

    rc = LCMapStringW( 0x0411,
                       LCMAP_KATAKANA,
                       L"\x304f\x309b",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"\x30af\x309b",
                       "Japanese Test 28",
                       &NumErrors );

    rc = LCMapStringW( 0x0411,
                       LCMAP_KATAKANA | LCMAP_UPPERCASE,
                       L"a\x304f",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"A\x30af",
                       "Japanese Test 29",
                       &NumErrors );

    rc = LCMapStringW( 0x0411,
                       LCMAP_KATAKANA | LCMAP_LOWERCASE,
                       L"A\x304f",
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"a\x30af",
                       "Japanese Test 30",
                       &NumErrors );

    rc = LCMapStringW( 0x0411,
                       LCMAP_SORTKEY,
                       L"\x005c",
                       -1,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x0a\x05\x01\x01\x01\x01",
                             "Japanese 0x005c",
                             &NumErrors );


    //
    //  Ideograph LCMAP_SORTKEY Tests.
    //

    //  Variation 1  -  ideograph sortkey
    rc = LCMapStringW( 0x0411,
                       LCMAP_SORTKEY,
                       L"\x99d5",
                       -1,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x81\x22\x01\x01\x01\x01",
                             "ideograph sortkey Japanese",
                             &NumErrors );

    //  Variation 2  -  ideograph sortkey, no DestStr
    rc = LCMapStringW( 0x0411,
                       LCMAP_SORTKEY,
                       L"\x99d5",
                       -1,
                       NULL,
                       0 );
    CheckReturnValidSortKey( rc,
                             -1,
                             NULL,
                             "\x81\x22\x01\x01\x01\x01",
                             "ideograph sortkey Japanese, no DestStr",
                             &NumErrors );

    //  Variation 3  -  ideograph sortkey
    rc = LCMapStringW( 0x0412,
                       LCMAP_SORTKEY,
                       L"\x99d5",
                       -1,
                       (LPWSTR)SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x0e\x03\x01\x79\x01\x01\x01",
                             "ideograph sortkey Korean",
                             &NumErrors );

    //  Variation 4  -  ideograph sortkey, no DestStr
    rc = LCMapStringW( 0x0412,
                       LCMAP_SORTKEY,
                       L"\x99d5",
                       -1,
                       NULL,
                       0 );
    CheckReturnValidSortKey( rc,
                             -1,
                             NULL,
                             "\x80\x03\x01\x79\x01\x01\x01",
                             "ideograph sortkey Korean, no DestStr",
                             &NumErrors );


    //
    //  Src = Dest - W Version.
    //

    //  Variation 1  -  lowercase
    MapDest[0] = L'X';
    MapDest[1] = L'Y';
    MapDest[2] = 0;
    rc = LCMapStringW( Locale,
                       LCMAP_LOWERCASE,
                       MapDest,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"xy",
                       "src = dest - lowercase",
                       &NumErrors );

    //  Variation 2  -  uppercase
    MapDest[0] = L'x';
    MapDest[1] = L'y';
    MapDest[2] = 0;
    rc = LCMapStringW( Locale,
                       LCMAP_UPPERCASE,
                       MapDest,
                       -1,
                       MapDest,
                       BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       MapDest,
                       L"XY",
                       "src = dest - uppercase",
                       &NumErrors );

    //  Variation 3  -  uppercase
    MapDest[0] = L'x';
    MapDest[1] = L'y';
    MapDest[2] = 0;
    rc = LCMapStringW( Locale,
                       LCMAP_UPPERCASE,
                       MapDest,
                       2,
                       MapDest,
                       2 );
    CheckReturnValidW( rc,
                       2,
                       MapDest,
                       L"XY",
                       "src = dest - uppercase, size",
                       &NumErrors );


    //
    //  Src = Dest - A Version.
    //

    //  Variation 1  -  lowercase
    MapDestA[0] = 'X';
    MapDestA[1] = 'Y';
    MapDestA[2] = 0;
    rc = LCMapStringA( Locale,
                       LCMAP_LOWERCASE,
                       MapDestA,
                       -1,
                       MapDestA,
                       BUFSIZE );
    CheckReturnValidA( rc,
                       -1,
                       MapDestA,
                       "xy",
                       NULL,
                       "src = dest - lowercase (A Version)",
                       &NumErrors );

    //  Variation 2  -  uppercase
    MapDestA[0] = 'x';
    MapDestA[1] = 'y';
    MapDestA[2] = 0;
    rc = LCMapStringA( Locale,
                       LCMAP_UPPERCASE,
                       MapDestA,
                       -1,
                       MapDestA,
                       BUFSIZE );
    CheckReturnValidA( rc,
                       -1,
                       MapDestA,
                       "XY",
                       NULL,
                       "src = dest - uppercase (A Version)",
                       &NumErrors );

    //  Variation 3  -  uppercase
    MapDestA[0] = 'x';
    MapDestA[1] = 'y';
    MapDestA[2] = 0;
    rc = LCMapStringA( Locale,
                       LCMAP_UPPERCASE,
                       MapDestA,
                       2,
                       MapDestA,
                       2 );
    CheckReturnValidA( rc,
                       2,
                       MapDestA,
                       "XY",
                       NULL,
                       "src = dest - uppercase, size (A Version)",
                       &NumErrors );



    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  LCMS_Ansi
//
//  This routine tests the Ansi version of the API routine.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int LCMS_Ansi()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


    //
    //  LCMapStringA - USE CP ACP.
    //

    //  Variation 1  -  Use CP ACP
    rc = LCMapStringA( 0x0409,
                       LOCALE_USE_CP_ACP | LCMAP_LOWERCASE,
                       "ABCD",
                       -1,
                       MapDestA,
                       BUFSIZE );
    CheckReturnValidA( rc,
                       -1,
                       MapDestA,
                       "abcd",
                       NULL,
                       "A Version Use CP ACP",
                       &NumErrors );


    //
    //  LCMapStringA - LOWER case.
    //

    //  Variation 1  -  Lower case
    rc = LCMapStringA( 0x0409,
                       LCMAP_LOWERCASE,
                       "ABCD",
                       -1,
                       MapDestA,
                       BUFSIZE );
    CheckReturnValidA( rc,
                       -1,
                       MapDestA,
                       "abcd",
                       NULL,
                       "A version lower",
                       &NumErrors );

    //  Variation 2  -  Lower case
    rc = LCMapStringA( 0x0409,
                       LCMAP_LOWERCASE,
                       "ABCD",
                       -1,
                       NULL,
                       0 );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       "abcd",
                       NULL,
                       "A version lower (cchDest = 0)",
                       &NumErrors );

    //  Variation 3  -  Lower case
    rc = LCMapStringA( 0x0409,
                       LCMAP_LOWERCASE,
                       "ABCD",
                       4,
                       MapDestA,
                       BUFSIZE );
    CheckReturnValidA( rc,
                       4,
                       MapDestA,
                       "abcd",
                       NULL,
                       "A version lower size",
                       &NumErrors );

    //  Variation 4  -  Lower case
    rc = LCMapStringA( 0x0409,
                       LCMAP_LOWERCASE,
                       "ABCD",
                       4,
                       NULL,
                       0 );
    CheckReturnValidA( rc,
                       4,
                       NULL,
                       "abcd",
                       NULL,
                       "A version lower size (cchDest = 0)",
                       &NumErrors );


    //
    //  LCMapStringA - SORTKEY.
    //
    //  Variation 1  -  sortkey
    rc = LCMapStringA( Locale,
                       LCMAP_SORTKEY,
                       "Th",
                       -1,
                       SortDest,
                       BUFSIZE );
    CheckReturnValidSortKey( rc,
                             -1,
                             SortDest,
                             "\x0e\x99\x0e\x2c\x01\x01\x12\x01\x01",
                             "A version sortkey",
                             &NumErrors );

    //  Variation 2  -  sortkey, no DestStr
    rc = LCMapStringA( Locale,
                       LCMAP_SORTKEY,
                       "Th",
                       -1,
                       NULL,
                       0 );
    CheckReturnValidSortKey( rc,
                             -1,
                             NULL,
                             "\x0e\x99\x0e\x2c\x01\x01\x12\x01\x01",
                             "A version sortkey (cchDest = 0)",
                             &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  CheckReturnValidSortKey
//
//  Checks the return code from the valid LCMapString[A|W] call with the
//  LCMAP_SOTRKEY flag set.  It prints out the appropriate error if the
//  incorrect result is found.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

void CheckReturnValidSortKey(
    int CurrentReturn,
    int ExpectedReturn,
    LPBYTE pCurrentString,
    LPBYTE pExpectedString,
    LPBYTE pErrString,
    int *pNumErrors)
{
    int ctr;                 // loop counter


    if (ExpectedReturn == -1)
    {
        ExpectedReturn = MB_STRING_LEN_NULL(pExpectedString);
    }

    if ( (CurrentReturn != ExpectedReturn) ||
         ( (pCurrentString != NULL) &&
           (CompStringsA(pCurrentString, pExpectedString, CurrentReturn)) ) )
    {
        printf("ERROR: %s - \n", pErrString);
        printf("  Return = %d, Expected = %d\n", CurrentReturn, ExpectedReturn);

        if (pCurrentString != NULL)
        {
            printf("       ");
            for (ctr = 0; ctr < CurrentReturn; ctr++)
            {
                printf("%x ", pCurrentString[ctr]);
            }
            printf("\n");
        }

        (*pNumErrors)++;
    }
}
