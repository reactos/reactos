/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    mbtest.c

Abstract:

    Test module for NLS API MultiByteToWideChar.

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

#define  BUFSIZE           50               // buffer size in chars
#define  MB_INVALID_FLAGS  ((DWORD)(~(MB_PRECOMPOSED | MB_COMPOSITE)))




//
//  Global Variables.
//

#define mbMBStr   "This Is A String."

BYTE mbMBStr2[] = "This Is A String.";      // this could get overwritten

#define wcMBStr   L"This Is A String."

WCHAR wcMBDest[BUFSIZE];




//
//  Forward Declarations.
//

int
MB_BadParamCheck();

int
MB_NormalCase();

int
MB_TestFlags();

int
MB_TestDBCS();





////////////////////////////////////////////////////////////////////////////
//
//  TestMBToWC
//
//  Test routine for MultiByteToWideChar API.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int TestMBToWC()
{
    int ErrCount = 0;             // error count


    //
    //  Print out what's being done.
    //
    printf("\n\nTESTING MultiByteToWideChar...\n\n");

    //
    //  Test bad parameters.
    //
    ErrCount += MB_BadParamCheck();

    //
    //  Test normal cases.
    //
    ErrCount += MB_NormalCase();

    //
    //  Test flags.
    //
    ErrCount += MB_TestFlags();

    //
    //  Test DBCS.
    //
    ErrCount += MB_TestDBCS();

    //
    //  Print out result.
    //
    printf("\nMultiByteToWideChar:  ERRORS = %d\n", ErrCount);

    //
    //  Return total number of errors found.
    //
    return (ErrCount);
}


////////////////////////////////////////////////////////////////////////////
//
//  MB_BadParamCheck
//
//  This routine passes in bad parameters to the API routine and checks to
//  be sure they are handled properly.  The number of errors encountered
//  is returned to the caller.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int MB_BadParamCheck()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


    //
    //  Null Pointers and Equal Pointers.
    //

    //  Variation 1  -  lpMultiByteStr = NULL
    rc = MultiByteToWideChar( 1252,
                              MB_PRECOMPOSED,
                              NULL,
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "lpMultiByteStr NULL",
                         &NumErrors );

    //  Variation 2  -  lpWideCharStr = NULL
    rc = MultiByteToWideChar( 1252,
                              MB_PRECOMPOSED,
                              mbMBStr,
                              -1,
                              NULL,
                              BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "lpWideCharStr NULL",
                         &NumErrors );

    //  Variation 3  -  equal pointers
    rc = MultiByteToWideChar( 1252,
                              0,
                              mbMBStr2,
                              -1,
                              (LPWSTR)mbMBStr2,
                              sizeof(mbMBStr2) / sizeof(WCHAR) );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "equal pointers",
                         &NumErrors );


    //
    //  Negative or Zero Lengths.
    //

    //  Variation 1  -  cchMultiByte = 0
    rc = MultiByteToWideChar( 1252,
                              MB_PRECOMPOSED,
                              mbMBStr,
                              0,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "cchMultiByte zero",
                         &NumErrors );

    //  Variation 2  -  cchWideChar = negative
    rc = MultiByteToWideChar( 1252,
                              MB_PRECOMPOSED,
                              mbMBStr,
                              -1,
                              wcMBDest,
                              -1 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "cchWideChar zero",
                         &NumErrors );


    //
    //  Invalid Code Page.
    //

    //  Variation 1  -  CodePage = invalid
    rc = MultiByteToWideChar( 8,
                              MB_PRECOMPOSED,
                              mbMBStr,
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "CodePage invalid",
                         &NumErrors );


    //  Variation 2  -  CodePage = invalid
    rc = MultiByteToWideChar( 65002,
                              MB_PRECOMPOSED,
                              mbMBStr,
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "CodePage invalid 2",
                         &NumErrors );


    //
    //  Zero or Invalid Flag Values.
    //

    //  Variation 1  -  wFlags = 0
    rc = MultiByteToWideChar( 1252,
                              0,
                              mbMBStr,
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       wcMBStr,
                       "wFlags zero",
                       &NumErrors );

    //  Variation 2  -  wFlags = invalid
    rc = MultiByteToWideChar( 1252,
                              MB_INVALID_FLAGS,
                              mbMBStr,
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "wFlags invalid",
                         &NumErrors );

    //  Variation 3  -  precomp | comp
    rc = MultiByteToWideChar( 1252,
                              MB_PRECOMPOSED | MB_COMPOSITE,
                              mbMBStr,
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "precomp | comp",
                         &NumErrors );


    //
    //  Buffer Too Small.
    //

    //  Variation 1  -  cchWideChar = too small
    rc = MultiByteToWideChar( 1252,
                              MB_PRECOMPOSED,
                              mbMBStr,
                              -1,
                              wcMBDest,
                              2 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INSUFFICIENT_BUFFER,
                         "cchWideChar too small",
                         &NumErrors );



    //
    //  No Unicode Translation - using MB_ERR_INVALID_CHARS flag.
    //

#if 0
    //  Variation 1  -  cp 875
    rc = MultiByteToWideChar( 875,
                              MB_ERR_INVALID_CHARS,
                              "fa\x30",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_NO_UNICODE_TRANSLATION,
                         "invalid char flag (875)",
                         &NumErrors );

    //  Variation 2  -  cp 875
    rc = MultiByteToWideChar( 875,
                              MB_ERR_INVALID_CHARS,
                              "\x30",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_NO_UNICODE_TRANSLATION,
                         "invalid char flag (875) 2",
                         &NumErrors );
#endif

    //  Variation 3  -  cp 857
    rc = MultiByteToWideChar( 857,
                              MB_ERR_INVALID_CHARS,
                              "fa\xd5",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_NO_UNICODE_TRANSLATION,
                         "invalid char flag (857)",
                         &NumErrors );

    //  Variation 4  -  cp 857
    rc = MultiByteToWideChar( 857,
                              MB_ERR_INVALID_CHARS,
                              "\xd5",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_NO_UNICODE_TRANSLATION,
                         "invalid char flag (857) 2",
                         &NumErrors );

    //  Variation 5  -  cp 857
    rc = MultiByteToWideChar( 857,
                              MB_ERR_INVALID_CHARS,
                              "fa\xd5t",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_NO_UNICODE_TRANSLATION,
                         "invalid char flag (857) 3",
                         &NumErrors );


    //  Variation 6  -  cp 857
    rc = MultiByteToWideChar( 857,
                              MB_ERR_INVALID_CHARS | MB_COMPOSITE,
                              "\xd5",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_NO_UNICODE_TRANSLATION,
                         "invalid char/composite flag (857)",
                         &NumErrors );


    //  Variation 7  -  cp 857
    rc = MultiByteToWideChar( 857,
                              MB_ERR_INVALID_CHARS | MB_COMPOSITE,
                              "fa\xd5t",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_NO_UNICODE_TRANSLATION,
                         "invalid char/composite flag (857) 2",
                         &NumErrors );


    //  Variation 8  -  cp 857
    rc = MultiByteToWideChar( 857,
                              MB_ERR_INVALID_CHARS | MB_COMPOSITE | MB_USEGLYPHCHARS,
                              "\xd5",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_NO_UNICODE_TRANSLATION,
                         "invalid char/composite/glyph flag (857)",
                         &NumErrors );


    //  Variation 9  -  cp 857
    rc = MultiByteToWideChar( 857,
                              MB_ERR_INVALID_CHARS | MB_COMPOSITE | MB_USEGLYPHCHARS,
                              "fa\xd5t",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_NO_UNICODE_TRANSLATION,
                         "invalid char/composite/glyph flag (857) 2",
                         &NumErrors );


    //  Variation 10  -  cp 857
    rc = MultiByteToWideChar( 857,
                              MB_ERR_INVALID_CHARS | MB_USEGLYPHCHARS,
                              "\xd5",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_NO_UNICODE_TRANSLATION,
                         "invalid char/glyph flag (857)",
                         &NumErrors );


    //  Variation 10  -  cp 857
    rc = MultiByteToWideChar( 857,
                              MB_ERR_INVALID_CHARS | MB_USEGLYPHCHARS,
                              "fa\xd5t",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_NO_UNICODE_TRANSLATION,
                         "invalid char/glyph flag (857) 2",
                         &NumErrors );


    //
    //  DBCS - No Unicode Translation - using MB_ERR_INVALID_CHARS flag.
    //

    //  Variation 1  -  cp 932
    rc = MultiByteToWideChar( 932,
                              MB_ERR_INVALID_CHARS,
                              "\x81\x30",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_NO_UNICODE_TRANSLATION,
                         "invalid char flag (932)",
                         &NumErrors );

    //  Variation 2  -  cp 932
    rc = MultiByteToWideChar( 932,
                              MB_ERR_INVALID_CHARS,
                              "\x81",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_NO_UNICODE_TRANSLATION,
                         "invalid char flag (932) 2",
                         &NumErrors );

    //  Variation 3  -  cp 932
    rc = MultiByteToWideChar( 932,
                              MB_ERR_INVALID_CHARS | MB_COMPOSITE,
                              "\x81\x30",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_NO_UNICODE_TRANSLATION,
                         "invalid char flag (932) 3",
                         &NumErrors );

    //  Variation 4  -  cp 932
    rc = MultiByteToWideChar( 932,
                              MB_ERR_INVALID_CHARS | MB_COMPOSITE,
                              "\x81",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_NO_UNICODE_TRANSLATION,
                         "invalid char/composite flag (932)",
                         &NumErrors );

    //  Variation 5  -  cp 932
    rc = MultiByteToWideChar( 932,
                              MB_ERR_INVALID_CHARS | MB_COMPOSITE | MB_USEGLYPHCHARS,
                              "\x81\x30",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_NO_UNICODE_TRANSLATION,
                         "invalid char/composite/glyph flag (932)",
                         &NumErrors );

    //  Variation 6  -  cp 932
    rc = MultiByteToWideChar( 932,
                              MB_ERR_INVALID_CHARS | MB_COMPOSITE | MB_USEGLYPHCHARS,
                              "\x81",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_NO_UNICODE_TRANSLATION,
                         "invalid char/composite/glyph flag (932) 2",
                         &NumErrors );

    //  Variation 7  -  cp 932
    rc = MultiByteToWideChar( 932,
                              MB_ERR_INVALID_CHARS | MB_USEGLYPHCHARS,
                              "\x81\x30",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_NO_UNICODE_TRANSLATION,
                         "invalid char/glyph flag (932)",
                         &NumErrors );

    //  Variation 8  -  cp 932
    rc = MultiByteToWideChar( 932,
                              MB_ERR_INVALID_CHARS | MB_USEGLYPHCHARS,
                              "\x81",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_NO_UNICODE_TRANSLATION,
                         "invalid char/glyph flag (932) 2",
                         &NumErrors );



    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  MB_NormalCase
//
//  This routine tests the normal cases of the API routine.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int MB_NormalCase()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code
    BYTE Ch;                      // buffer for one character
    WCHAR CheckBuf;               // return buffer for one character
    LPWSTR pBogus = NULL;         // bogus, uninitialized pointer
    LCID Locale;                  // save the old locale


#ifdef PERF

  DbgBreakPoint();

#endif


    //
    //  cchWideChar.
    //

    //  Variation 1  -  cchWideChar = length of wcMBDest
    rc = MultiByteToWideChar( 1252,
                              MB_PRECOMPOSED,
                              mbMBStr,
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       wcMBStr,
                       "cchWideChar (length)",
                       &NumErrors );

    //  Variation 2  -  cchWideChar = 0
    wcMBDest[0] = 0x0005;
    rc = MultiByteToWideChar( 1252,
                              MB_PRECOMPOSED,
                              mbMBStr,
                              -1,
                              wcMBDest,
                              0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       wcMBStr,
                       "cchWideChar zero",
                       &NumErrors );

    //  Variation 3  -  cchWideChar = 0, wcMBDest = NULL
    rc = MultiByteToWideChar( 1252,
                              MB_PRECOMPOSED,
                              mbMBStr,
                              -1,
                              NULL,
                              0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       wcMBStr,
                       "cchWideChar (NULL ptr)",
                       &NumErrors );

    //  Variation 4  -  cchWideChar = 0, wcMBDest = uninitialized ptr
    rc = MultiByteToWideChar( 1252,
                              MB_PRECOMPOSED,
                              mbMBStr,
                              -1,
                              pBogus,
                              0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       wcMBStr,
                       "cchWideChar (bogus ptr)",
                       &NumErrors );


    //
    //  cchMultiByte.
    //

    //  Variation 1  -  cchMultiByte = length of mbMBStr
    rc = MultiByteToWideChar( 1252,
                              MB_PRECOMPOSED,
                              mbMBStr,
                              MB_STRING_LEN(mbMBStr),
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       WC_STRING_LEN(wcMBStr),
                       wcMBDest,
                       wcMBStr,
                       "cchMultiByte (length)",
                       &NumErrors );

    //  Variation 2  -  cchMultiByte = -1
    rc = MultiByteToWideChar( 1252,
                              MB_PRECOMPOSED,
                              mbMBStr,
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       wcMBStr,
                       "cchMultiByte (-1)",
                       &NumErrors );

    //  Variation 3  -  cchMultiByte = length of mbMBStr, no wcMBDest
    rc = MultiByteToWideChar( 1252,
                              MB_PRECOMPOSED,
                              mbMBStr,
                              MB_STRING_LEN(mbMBStr),
                              NULL,
                              0 );
    CheckReturnValidW( rc,
                       WC_STRING_LEN(wcMBStr),
                       NULL,
                       wcMBStr,
                       "cchMultiByte (length), no wcMBDest",
                       &NumErrors );

    //  Variation 4  -  cchMultiByte = -1, no wcMBDest
    rc = MultiByteToWideChar( 1252,
                              MB_PRECOMPOSED,
                              mbMBStr,
                              -1,
                              NULL,
                              0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       wcMBStr,
                       "cchMultiByte (-1), no wcMBDest",
                       &NumErrors );


    //
    //  CodePage.
    //

    //  Variation 1  -  CodePage = CP_ACP
    rc = MultiByteToWideChar( CP_ACP,
                              MB_PRECOMPOSED,
                              mbMBStr,
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       wcMBStr,
                       "CodePage CP_ACP",
                       &NumErrors );

    //  Variation 2  -  CodePage = CP_ACP, no wcMBDest
    rc = MultiByteToWideChar( CP_ACP,
                              MB_PRECOMPOSED,
                              mbMBStr,
                              -1,
                              NULL,
                              0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       wcMBStr,
                       "CodePage CP_ACP, no wcMBDest",
                       &NumErrors );

    //  Variation 3  -  CodePage = CP_OEMCP
    rc = MultiByteToWideChar( CP_OEMCP,
                              MB_PRECOMPOSED,
                              mbMBStr,
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       wcMBStr,
                       "CodePage CP_OEMCP",
                       &NumErrors );

    //  Variation 4  -  CodePage = CP_OEMCP, no wcMBDest
    rc = MultiByteToWideChar( CP_OEMCP,
                              MB_PRECOMPOSED,
                              mbMBStr,
                              -1,
                              NULL,
                              0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       wcMBStr,
                       "CodePage CP_OEMCP, no wcMBDest",
                       &NumErrors );

    //  Variation 5  -  CodePage = CP_MACCP
    rc = MultiByteToWideChar( CP_MACCP,
                              MB_PRECOMPOSED,
                              mbMBStr,
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       wcMBStr,
                       "CodePage CP_MACCP",
                       &NumErrors );

    //  Variation 6  -  CodePage = CP_MACCP, no wcMBDest
    rc = MultiByteToWideChar( CP_MACCP,
                              MB_PRECOMPOSED,
                              mbMBStr,
                              -1,
                              NULL,
                              0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       wcMBStr,
                       "CodePage CP_MACCP, no wcMBDest",
                       &NumErrors );


    //  Variation 7  -  CodePage = 437
    rc = MultiByteToWideChar( 437,
                              MB_PRECOMPOSED,
                              mbMBStr,
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       wcMBStr,
                       "CodePage 437",
                       &NumErrors );

    //  Variation 8  -  CodePage = 437, no wcMBDest
    rc = MultiByteToWideChar( 437,
                              MB_PRECOMPOSED,
                              mbMBStr,
                              -1,
                              NULL,
                              0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       wcMBStr,
                       "CodePage 437, no wcMBDest",
                       &NumErrors );

    //  Variation 9  -  CodePage = 850
    rc = MultiByteToWideChar( 850,
                              MB_PRECOMPOSED,
                              mbMBStr,
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       wcMBStr,
                       "CodePage 850",
                       &NumErrors );

    //  Variation 10  -  CodePage = 850, no wcMBDest
    rc = MultiByteToWideChar( 850,
                              MB_PRECOMPOSED,
                              mbMBStr,
                              -1,
                              NULL,
                              0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       wcMBStr,
                       "CodePage 850, no wcMBDest",
                       &NumErrors );

    //  Variation 11  -  CodePage = 10000
    rc = MultiByteToWideChar( 10000,
                              MB_PRECOMPOSED,
                              mbMBStr,
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       wcMBStr,
                       "CodePage 10000",
                       &NumErrors );

    //  Variation 12  -  CodePage = 10000, no wcMBDest
    rc = MultiByteToWideChar( 10000,
                              MB_PRECOMPOSED,
                              mbMBStr,
                              -1,
                              NULL,
                              0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       wcMBStr,
                       "CodePage 10000, no wcMBDest",
                       &NumErrors );

    //  Variation 13  -  CodePage = CP_THREAD_ACP
    rc = MultiByteToWideChar( CP_THREAD_ACP,
                              0,
                              mbMBStr,
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       wcMBStr,
                       "CodePage CP_THREAD_ACP",
                       &NumErrors );

    //  Variation 14  -  CodePage = CP_THREAD_ACP, no wcMBDest
    rc = MultiByteToWideChar( CP_THREAD_ACP,
                              0,
                              mbMBStr,
                              -1,
                              NULL,
                              0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       wcMBStr,
                       "CodePage CP_THREAD_ACP, no wcMBDest",
                       &NumErrors );

    //  Variation 15  -  CodePage = CP_THREAD_ACP
    Locale = GetThreadLocale();
    SetThreadLocale(0x00000405);                 // Czech - cp 1250
    rc = MultiByteToWideChar( CP_THREAD_ACP,
                              0,
                              "\x9d\x9f",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       L"\x0165\x017a",
                       "CodePage CP_THREAD_ACP - Czech cp 1250",
                       &NumErrors );
    SetThreadLocale(Locale);                     // US - cp 1252
    rc = MultiByteToWideChar( CP_THREAD_ACP,
                              0,
                              "\x9d\x9f",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       L"\x009d\x0178",
                       "CodePage CP_THREAD_ACP - US cp 1252",
                       &NumErrors );

    //  Variation 16  -  CodePage = CP_SYMBOL
    rc = MultiByteToWideChar( CP_SYMBOL,
                              0,
                              "\x20\x33\xca\x00",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       L"\xf020\xf033\xf0ca\x0000",
                       "CodePage CP_SYMBOL",
                       &NumErrors );

    //  Variation 17  -  CodePage = CP_SYMBOL, no wcMBDest
    rc = MultiByteToWideChar( CP_SYMBOL,
                              0,
                              "\x20\x33\xca\x00",
                              -1,
                              NULL,
                              0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       L"\xf020\xf033\xf0ca\x0000",
                       "CodePage CP_SYMBOL, no wcMBDest",
                       &NumErrors );

    //  Variation 18  -  CodePage = CP_SYMBOL
    rc = MultiByteToWideChar( CP_SYMBOL,
                              0,
                              "\x20\x33\xca\x00",
                              5,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       5,
                       wcMBDest,
                       L"\xf020\xf033\xf0ca\x0000",
                       "CodePage CP_SYMBOL, size",
                       &NumErrors );





    //
    //  UTF 7.
    //

    //  Variation 1  -  CodePage = CP_UTF7
    rc = MultiByteToWideChar( CP_UTF7,
                              0,
                              "\x2b\x55\x78\x64\x4f\x72\x41\x2d",  // +UxdOrA-
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       L"\x5317\x4eac",
                       "CodePage CP_UTF7 - 1",
                       &NumErrors );

    //  Variation 2  -  CodePage = CP_UTF7
    rc = MultiByteToWideChar( CP_UTF7,                // +Ti1XC2b4Xpc-
                              0,
                              "\x2b\x54\x69\x31\x58\x43\x32\x62\x34\x58\x70\x63\x2d",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       L"\x4e2d\x570b\x66f8\x5e97",
                       "CodePage CP_UTF7 - 2",
                       &NumErrors );

    //  Variation 3  -  CodePage = CP_UTF7
    rc = MultiByteToWideChar( CP_UTF7,
                              0,
                              "\x2b\x41\x44\x73\x2d",   // +ADs-
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       L"\x003b",
                       "CodePage CP_UTF7 - 3",
                       &NumErrors );

    //  Variation 4  -  CodePage = CP_UTF7
    rc = MultiByteToWideChar( CP_UTF7,                // +XrdxmVtXUXg-
                              0,
                              "\x2b\x58\x72\x64\x78\x6d\x56\x74\x58\x55\x58\x67\x2d",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       L"\x5eb7\x7199\x5b57\x5178",
                       "CodePage CP_UTF7 - 4",
                       &NumErrors );

    //  Variation 5  -  CodePage = CP_UTF7
    rc = MultiByteToWideChar( CP_UTF7,                // +TpVPXGBG-
                              0,
                              "\x2b\x54\x70\x56\x50\x58\x47\x42\x47\x2d",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       L"\x4e95\x4f5c\x6046",
                       "CodePage CP_UTF7 - 5",
                       &NumErrors );

    //  Variation 6  -  CodePage = CP_UTF7
    rc = MultiByteToWideChar( CP_UTF7,
                              0,
                              "\x2b\x41\x46\x38\x2d",   // +AF8-
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       L"\x005f",
                       "CodePage CP_UTF7 - 6",
                       &NumErrors );

    //  Variation 7  -  CodePage = CP_UTF7
    rc = MultiByteToWideChar( CP_UTF7,
                              0,
                              "\x2b\x41\x45\x41\x2d",   // +AEA-
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       L"\x0040",
                       "CodePage CP_UTF7 - 7",
                       &NumErrors );

    //  Variation 8  -  CodePage = CP_UTF7
    rc = MultiByteToWideChar( CP_UTF7,
                              0,
                              "\x2b\x2d",        // +-
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       L"\x002b",
                       "CodePage CP_UTF7 - 8",
                       &NumErrors );

    //  Variation 9  -  CodePage = CP_UTF7
    rc = MultiByteToWideChar( CP_UTF7,
                              0,
                              "\x41\x2b\x2d",    // A+-
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       L"\x0041\x002b",
                       "CodePage CP_UTF7 - 9",
                       &NumErrors );

    //  Variation 10  -  CodePage = CP_UTF7
    rc = MultiByteToWideChar( CP_UTF7,
                              0,
                              "\x2b\x41\x45\x41\x2d\x2b\x2d",  // +AEA-+-
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       L"\x0040\x002b",
                       "CodePage CP_UTF7 - 10",
                       &NumErrors );


    //
    //  UTF 8.
    //

    //  Variation 1  -  CodePage = CP_UTF8
    rc = MultiByteToWideChar( CP_UTF8,
                              0,
                              "\xe5\x8c\x97\xe4\xba\xac",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       L"\x5317\x4eac",
                       "CodePage CP_UTF8 - 1",
                       &NumErrors );

    //  Variation 2  -  CodePage = CP_UTF8
    rc = MultiByteToWideChar( CP_UTF8,
                              0,
                              "\xe4\xb8\xad\xe5\x9c\x8b\xe6\x9b\xb8\xe5\xba\x97",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       L"\x4e2d\x570b\x66f8\x5e97",
                       "CodePage CP_UTF8 - 2",
                       &NumErrors );

    //  Variation 3  -  CodePage = CP_UTF8
    rc = MultiByteToWideChar( CP_UTF8,
                              0,
                              "\x3b",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       L"\x003b",
                       "CodePage CP_UTF8 - 3",
                       &NumErrors );

    //  Variation 4  -  CodePage = CP_UTF8
    rc = MultiByteToWideChar( CP_UTF8,
                              0,
                              "\xe5\xba\xb7\xe7\x86\x99\xe5\xad\x97\xe5\x85\xb8",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       L"\x5eb7\x7199\x5b57\x5178",
                       "CodePage CP_UTF8 - 4",
                       &NumErrors );

    //  Variation 5  -  CodePage = CP_UTF8
    rc = MultiByteToWideChar( CP_UTF8,
                              0,
                              "\xe4\xba\x95\xe4\xbd\x9c\xe6\x81\x86",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       L"\x4e95\x4f5c\x6046",
                       "CodePage CP_UTF8 - 5",
                       &NumErrors );

    //  Variation 6  -  CodePage = CP_UTF8
    rc = MultiByteToWideChar( CP_UTF8,
                              0,
                              "\x5f",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       L"\x005f",
                       "CodePage CP_UTF8 - 6",
                       &NumErrors );

    //  Variation 7  -  CodePage = CP_UTF8
    rc = MultiByteToWideChar( CP_UTF8,
                              0,
                              "\x40",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       L"\x0040",
                       "CodePage CP_UTF8 - 7",
                       &NumErrors );

    //  Variation 8  -  CodePage = CP_UTF8
    rc = MultiByteToWideChar( CP_UTF8,
                              0,
                              "\x2b",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       L"\x002b",
                       "CodePage CP_UTF8 - 8",
                       &NumErrors );



    //
    //  Check 0xb3 by itself (to make sure not sign extended).
    //
    Ch = (BYTE)0xb3;
    rc = MultiByteToWideChar( 1252,
                              MB_PRECOMPOSED,
                              &Ch,
                              1,
                              &CheckBuf,
                              1 );
    CheckReturnValidW( rc,
                       1,
                       &CheckBuf,
                       L"\x00b3",
                       "Check Buffer",
                       &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  MB_TestFlags
//
//  This routine tests the different flags of the API routine.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int MB_TestFlags()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code
    BYTE Ch;                      // buffer for one character
    WCHAR CheckBuf;               // return buffer for one character


    //
    //  MB_PRECOMPOSED flag.
    //

    //  Variation 1  -  normal
    rc = MultiByteToWideChar( 1252,
                              MB_PRECOMPOSED,
                              mbMBStr,
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       wcMBStr,
                       "MB_PRECOMPOSED",
                       &NumErrors );

    //  Variation 2  -  normal, no wcMBDest
    rc = MultiByteToWideChar( 1252,
                              MB_PRECOMPOSED,
                              mbMBStr,
                              -1,
                              NULL,
                              0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       wcMBStr,
                       "MB_PRECOMPOSED, no wcMBDest",
                       &NumErrors );

    //  Variation 3  -  normal
    rc = MultiByteToWideChar( 1252,
                              MB_PRECOMPOSED,
                              "\x6e\x7e",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       L"\x006e\x007e",
                       "MB_PRECOMPOSED (n tilde)",
                       &NumErrors );

    //  Variation 4  -  normal, no wcMBDest
    rc = MultiByteToWideChar( 1252,
                              MB_PRECOMPOSED,
                              "\x6e\x7e",
                              -1,
                              NULL,
                              0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       L"\x006e\x007e",
                       "MB_PRECOMPOSED (n tilde), no wcMBDest",
                       &NumErrors );



    //
    //  MB_COMPOSITE flag.
    //

    //  Variation 1  -  normal
    rc = MultiByteToWideChar( 1252,
                              MB_COMPOSITE,
                              mbMBStr,
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       wcMBStr,
                       "MB_COMPOSITE",
                       &NumErrors );

    //  Variation 2  -  normal, no wcMBDest
    rc = MultiByteToWideChar( 1252,
                              MB_COMPOSITE,
                              mbMBStr,
                              -1,
                              NULL,
                              0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       wcMBStr,
                       "MB_COMPOSITE, no wcMBDest",
                       &NumErrors );

    //  Variation 3  -  normal
    rc = MultiByteToWideChar( 1252,
                              MB_COMPOSITE,
                              "\xfc\xf5",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       L"\x0075\x0308\x006f\x0303",
                       "MB_COMPOSITE (u diaeresis, o tilde)",
                       &NumErrors );

    //  Variation 4  -  normal, no wcMBDest
    rc = MultiByteToWideChar( 1252,
                              MB_COMPOSITE,
                              "\xfc\xf5",
                              -1,
                              NULL,
                              0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       L"\x0075\x0308\x006f\x0303",
                       "MB_COMPOSITE (u diaeresis, o tilde), no wcMBDest",
                       &NumErrors );



    //
    //  MB_USEGLYPHCHARS flag.
    //

    //  Variation 1  -  Use Glyph
    Ch = (BYTE)0x0D;
    CheckBuf = (WCHAR)0;
    rc = MultiByteToWideChar( 437,
                              MB_USEGLYPHCHARS,
                              &Ch,
                              1,
                              &CheckBuf,
                              1 );
    CheckReturnValidW( rc,
                       1,
                       &CheckBuf,
                       L"\x266a",
                       "Use Glyph",
                       &NumErrors );

    //  Variation 2  -  Do NOT Use Glyph
    Ch = (BYTE)0x0D;
    CheckBuf = (WCHAR)0;
    rc = MultiByteToWideChar( 437,
                              0,
                              &Ch,
                              1,
                              &CheckBuf,
                              1 );
    CheckReturnValidW( rc,
                       1,
                       &CheckBuf,
                       L"\x000d",
                       "Do NOT Use Glyph",
                       &NumErrors );

    //  Variation 3  -  Use Glyph, Composite
    Ch = (BYTE)0x0D;
    CheckBuf = (WCHAR)0;
    rc = MultiByteToWideChar( 437,
                              MB_USEGLYPHCHARS | MB_COMPOSITE,
                              &Ch,
                              1,
                              &CheckBuf,
                              1 );
    CheckReturnValidW( rc,
                       1,
                       &CheckBuf,
                       L"\x266a",
                       "Use Glyph, Composite",
                       &NumErrors );

    //  Variation 4  -  Do NOT Use Glyph, Composite
    Ch = (BYTE)0x0D;
    CheckBuf = (WCHAR)0;
    rc = MultiByteToWideChar( 437,
                              MB_COMPOSITE,
                              &Ch,
                              1,
                              &CheckBuf,
                              1 );
    CheckReturnValidW( rc,
                       1,
                       &CheckBuf,
                       L"\x000d",
                       "Do NOT Use Glyph, Composite",
                       &NumErrors );



    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  MB_TestDBCS
//
//  This routine tests the DBCS code of the API routine.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int MB_TestDBCS()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


    //
    //  No trail byte with lead byte.
    //

    //  Variation 1  -  1 char
    wcMBDest[0] = 0;
    rc = MultiByteToWideChar( 932,
                              0,
                              "\x81",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       1,
                       wcMBDest,
                       L"\x0000",
                       "no trail byte 1 char",
                       &NumErrors );

    //  Variation 2  -  1 char
    wcMBDest[0] = 0;
    rc = MultiByteToWideChar( 932,
                              0,
                              "\x81",
                              1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       1,
                       wcMBDest,
                       L"\x0000",
                       "no trail byte 1 char 2",
                       &NumErrors );

    //  Variation 3  -  2 chars
    wcMBDest[0] = 0;
    rc = MultiByteToWideChar( 932,
                              0,
                              "\xdf\x81",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       L"\xff9f",
                       "no trail byte 2 chars",
                       &NumErrors );

    //  Variation 4  -  2 chars
    wcMBDest[0] = 0;
    rc = MultiByteToWideChar( 932,
                              0,
                              "\xdf\x81",
                              2,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       L"\xff9f",
                       "no trail byte 2 chars 2",
                       &NumErrors );


    //
    //  Invalid trail byte with lead byte.
    //

    //  Variation 1  -  2 chars
    wcMBDest[0] = 0;
    rc = MultiByteToWideChar( 932,
                              0,
                              "\x81\x30",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       L"\x30fb",
                       "invalid trail byte 2 chars",
                       &NumErrors );

    //  Variation 2  -  3 chars
    wcMBDest[0] = 0;
    rc = MultiByteToWideChar( 932,
                              0,
                              "\x81\x30\xdf",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       L"\x30fb\xff9f",
                       "invalid trail byte 3 chars",
                       &NumErrors );


    //
    //  Valid Double byte string.
    //

    //  Variation 1  -  2 chars
    wcMBDest[0] = 0;
    rc = MultiByteToWideChar( 932,
                              0,
                              "\x81\x40",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       L"\x3000",
                       "valid trail byte 2 chars",
                       &NumErrors );

    //  Variation 2  -  3 chars
    wcMBDest[0] = 0;
    rc = MultiByteToWideChar( 932,
                              0,
                              "\x81\x40\xdf",
                              -1,
                              wcMBDest,
                              BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       wcMBDest,
                       L"\x3000\xff9f",
                       "valid trail byte 3 chars",
                       &NumErrors );



    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}
