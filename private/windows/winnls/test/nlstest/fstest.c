/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    fstest.c

Abstract:

    Test module for NLS API FoldString.

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

#define  BUFSIZE           50
#define  FS_INVALID_FLAGS  ((DWORD)(~(MAP_FOLDCZONE | MAP_PRECOMPOSED |          \
                                      MAP_COMPOSITE | MAP_FOLDDIGITS)))




//
//  Global Variables.
//

#define FoldSrc1                  L"This Is A String"

#define FoldSrc2                  L"This Is$ A Str,ing"


WCHAR FoldDest[BUFSIZE];


#define wcMultiComp               L"\x0065\x0301\x0300"

#define wcCompDigitCZone          L"\x0065\x0301\x0300\x00b2\xfe64"
#define wcFoldCompDigitCZone      L"\x00e9\x0300\x0032\x003c"

#define wcPrecompDigitCZone       L"\x00e9\x0300\x00b2\xfe64"
#define wcFoldPreDigitCZone       L"\x0065\x0301\x0300\x0032\x003c"




//
//  Forward Declarations.
//

BOOL
InitFoldStr();

int
FS_BadParamCheck();

int
FS_NormalCase();

int
FS_Ansi();





////////////////////////////////////////////////////////////////////////////
//
//  TestFoldString
//
//  Test routine for FoldStringW API.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int TestFoldString()
{
    int ErrCount = 0;             // error count


    //
    //  Print out what's being done.
    //
    printf("\n\nTESTING FoldStringW...\n\n");

    //
    //  Initialize global variables.
    //
    if (!InitFoldStr())
    {
        printf("\nABORTED TestFoldString: Could not Initialize.\n");
        return (1);
    }

    //
    //  Test bad parameters.
    //
    ErrCount += FS_BadParamCheck();

    //
    //  Test normal cases.
    //
    ErrCount += FS_NormalCase();

    //
    //  Test Ansi version.
    //
    ErrCount += FS_Ansi();

    //
    //  Print out result.
    //
    printf("\nFoldStringW:  ERRORS = %d\n", ErrCount);

    //
    //  Return total number of errors found.
    //
    return (ErrCount);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitFoldStr
//
//  This routine initializes the global variables.  If no errors were
//  encountered, then it returns TRUE.  Otherwise, it returns FALSE.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL InitFoldStr()
{
    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  FS_BadParamCheck
//
//  This routine passes in bad parameters to the API routines and checks to
//  be sure they are handled properly.  The number of errors encountered
//  is returned to the caller.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int FS_BadParamCheck()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


    //
    //  Null Pointers.
    //

    //  Variation 1  -  lpSrcStr = NULL
    rc = FoldStringW( MAP_FOLDDIGITS,
                      NULL,
                      -1,
                      FoldDest,
                      BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "lpSrcStr NULL",
                         &NumErrors );

    //  Variation 2  -  lpDestStr = NULL
    rc = FoldStringW( MAP_FOLDDIGITS,
                      FoldSrc1,
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
    rc = FoldStringW( MAP_FOLDDIGITS,
                      FoldSrc1,
                      0,
                      FoldDest,
                      BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "cbSrc = 0",
                         &NumErrors );

    //  Variation 2  -  cbDest < 0
    rc = FoldStringW( MAP_FOLDDIGITS,
                      FoldSrc1,
                      -1,
                      FoldDest,
                      -1 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "cbDest < 0",
                         &NumErrors );


    //
    //  Zero or Invalid Flag Values.
    //

    //  Variation 1  -  dwMapFlags = invalid
    rc = FoldStringW( FS_INVALID_FLAGS,
                      FoldSrc1,
                      -1,
                      FoldDest,
                      BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "dwMapFlags invalid",
                         &NumErrors );

    //  Variation 2  -  dwMapFlags = 0
    rc = FoldStringW( 0,
                      FoldSrc1,
                      -1,
                      FoldDest,
                      BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "dwMapFlags zero",
                         &NumErrors );

    //  Variation 3  -  illegal combo comp
    rc = FoldStringW( MAP_PRECOMPOSED | MAP_COMPOSITE,
                      FoldSrc1,
                      -1,
                      FoldDest,
                      BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "illegal combo comp",
                         &NumErrors );

    //  Variation 4  -  illegal combo ligatures
    rc = FoldStringW( MAP_EXPAND_LIGATURES | MAP_COMPOSITE,
                      FoldSrc1,
                      -1,
                      FoldDest,
                      BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "illegal combo ligatures and comp",
                         &NumErrors );

    //  Variation 5  -  illegal combo ligatures
    rc = FoldStringW( MAP_EXPAND_LIGATURES | MAP_PRECOMPOSED,
                      FoldSrc1,
                      -1,
                      FoldDest,
                      BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "illegal combo ligatures and precomp",
                         &NumErrors );


    //
    //  Same Buffer Check.
    //

    //  Variation 1  -  same buffer
    FoldDest[0] = 0;
    rc = FoldStringW( MAP_FOLDDIGITS,
                      FoldDest,
                      -1,
                      FoldDest,
                      BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "same buffer",
                         &NumErrors );


    //
    //  Insufficient Buffer Check.
    //

    //  Variation 1  -  insufficient buffer
    FoldDest[0] = 0;
    rc = FoldStringW( MAP_EXPAND_LIGATURES,
                      L"\x00e6",
                      -1,
                      FoldDest,
                      2 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INSUFFICIENT_BUFFER,
                         "insufficient buffer",
                         &NumErrors );

    FoldDest[0] = 0;
    rc = FoldStringW( MAP_EXPAND_LIGATURES,
                      L"\x00e6",
                      1,
                      FoldDest,
                      1 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INSUFFICIENT_BUFFER,
                         "insufficient buffer 2",
                         &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  FS_NormalCase
//
//  This routine tests the normal cases of the API routine.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int FS_NormalCase()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


#ifdef PERF

  DbgBreakPoint();

#endif


    //
    //  cbDest = 0.
    //

    //  Variation 1  -  cbSrc = -1
    rc = FoldStringW( MAP_PRECOMPOSED,
                      FoldSrc1,
                      -1,
                      FoldDest,
                      0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       FoldSrc1,
                       "cbDest (0) cbSrc (-1)",
                       &NumErrors );

    //  Variation 2  -  cbSrc = value
    rc = FoldStringW( MAP_PRECOMPOSED,
                      FoldSrc1,
                      WC_STRING_LEN_NULL(FoldSrc1),
                      FoldDest,
                      0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       FoldSrc1,
                       "cbDest (0) cbSrc (value)",
                       &NumErrors );

    //  Variation 3  -  lpDestStr = NULL
    rc = FoldStringW( MAP_PRECOMPOSED,
                      FoldSrc1,
                      -1,
                      NULL,
                      0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       FoldSrc1,
                       "cbDest (0) lpDestStr NULL",
                       &NumErrors );


    //
    //  cbSrc.
    //

    //  Variation 1  -  cbSrc = -1
    rc = FoldStringW( MAP_PRECOMPOSED,
                      FoldSrc1,
                      -1,
                      FoldDest,
                      BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       FoldDest,
                       FoldSrc1,
                       "cbSrc (-1)",
                       &NumErrors );

    //  Variation 2  -  cbSrc = value
    rc = FoldStringW( MAP_PRECOMPOSED,
                      FoldSrc1,
                      WC_STRING_LEN(FoldSrc1),
                      FoldDest,
                      BUFSIZE );
    CheckReturnValidW( rc,
                       WC_STRING_LEN(FoldSrc1),
                       FoldDest,
                       FoldSrc1,
                       "cbSrc (value)",
                       &NumErrors );

    //  Variation 3  -  cbSrc = -1, no DestStr
    rc = FoldStringW( MAP_PRECOMPOSED,
                      FoldSrc1,
                      -1,
                      NULL,
                      0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       FoldSrc1,
                       "cbSrc (-1), no DestStr",
                       &NumErrors );

    //  Variation 4  -  cbSrc = value, no DestStr
    rc = FoldStringW( MAP_PRECOMPOSED,
                      FoldSrc1,
                      WC_STRING_LEN(FoldSrc1),
                      NULL,
                      0 );
    CheckReturnValidW( rc,
                       WC_STRING_LEN(FoldSrc1),
                       NULL,
                       FoldSrc1,
                       "cbSrc (value), no DestStr",
                       &NumErrors );


    //
    //  MAP_PRECOMPOSED Flag.
    //

    //  Variation 1  -  precomposed
    rc = FoldStringW( MAP_PRECOMPOSED,
                      FoldSrc2,
                      -1,
                      FoldDest,
                      BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       FoldDest,
                       FoldSrc2,
                       "precomposed",
                       &NumErrors );

    //  Variation 2  -  precomposed
    rc = FoldStringW( MAP_PRECOMPOSED,
                      L"\x006e\x0303",
                      -1,
                      FoldDest,
                      BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       FoldDest,
                       L"\x00f1",
                       "precomposed (n tilde)",
                       &NumErrors );

    //  Variation 3  -  precomposed
    rc = FoldStringW( MAP_PRECOMPOSED,
                      L"\x006e\x0303",
                      -1,
                      NULL,
                      0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       L"\x00f1",
                       "precomposed (n tilde), no DestStr",
                       &NumErrors );

    //  Variation 4  -  precomposed
    rc = FoldStringW( MAP_PRECOMPOSED,
                      L"\x0062\x0303",
                      -1,
                      FoldDest,
                      BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       FoldDest,
                       L"\x0062\x0303",
                       "precomposed (b tilde)",
                       &NumErrors );

    //  Variation 5  -  precomposed
    rc = FoldStringW( MAP_PRECOMPOSED,
                      L"\x0062\x0303",
                      -1,
                      NULL,
                      0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       L"\x0062\x0303",
                       "precomposed (b tilde), no DestStr",
                       &NumErrors );


    //
    //  MAP_COMPOSITE Flag.
    //

    //  Variation 1  -  composite
    rc = FoldStringW( MAP_COMPOSITE,
                      FoldSrc2,
                      -1,
                      FoldDest,
                      BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       FoldDest,
                       FoldSrc2,
                       "composite",
                       &NumErrors );

    //  Variation 2  -  composite
    rc = FoldStringW( MAP_COMPOSITE,
                      L"\x00f1",
                      -1,
                      FoldDest,
                      BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       FoldDest,
                       L"\x006e\x0303",
                       "composite (n tilde)",
                       &NumErrors );

    //  Variation 3  -  composite
    rc = FoldStringW( MAP_COMPOSITE,
                      L"\x00f1",
                      -1,
                      NULL,
                      0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       L"\x006e\x0303",
                       "composite (n tilde), no DestStr",
                       &NumErrors );

    //  Variation 4  -  composite
    rc = FoldStringW( MAP_COMPOSITE,
                      L"\x01c4",
                      -1,
                      FoldDest,
                      BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       FoldDest,
                       L"\x0044\x017d",
                       "composite (dz hacek)",
                       &NumErrors );

    //  Variation 5  -  composite
    rc = FoldStringW( MAP_COMPOSITE,
                      L"\x01c4",
                      -1,
                      NULL,
                      0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       L"\x0044\x017d",
                       "composite (dz hacek), no DestStr",
                       &NumErrors );

    //  Variation 6  -  composite
    rc = FoldStringW( MAP_COMPOSITE,
                      L"\x0062\x0303",
                      -1,
                      FoldDest,
                      BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       FoldDest,
                       L"\x0062\x0303",
                       "composite (b tilde)",
                       &NumErrors );

    //  Variation 7  -  composite
    rc = FoldStringW( MAP_COMPOSITE,
                      L"\x0062\x0303",
                      -1,
                      NULL,
                      0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       L"\x0062\x0303",
                       "composite (b tilde), no DestStr",
                       &NumErrors );


    //
    //  MAP_FOLDCZONE Flag.
    //

    //  Variation 1  -  fold compatibility zone
    rc = FoldStringW( MAP_FOLDCZONE,
                      FoldSrc2,
                      -1,
                      FoldDest,
                      BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       FoldDest,
                       FoldSrc2,
                       "fold czone",
                       &NumErrors );

    //  Variation 2  -  fold compatibility zone
    rc = FoldStringW( MAP_FOLDCZONE,
                      L"\x004a\xff24\xff22",
                      -1,
                      FoldDest,
                      BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       FoldDest,
                       L"\x004a\x0044\x0042",
                       "fold czone (JDB)",
                       &NumErrors );

    //  Variation 3  -  fold compatibility zone
    rc = FoldStringW( MAP_FOLDCZONE,
                      L"\x004a\xff24\xff22",
                      -1,
                      NULL,
                      0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       L"\x004a\x0044\x0042",
                       "fold czone (JDB), no DestStr",
                       &NumErrors );


    //
    //  MAP_FOLDDIGITS Flag.
    //

    //  Variation 1  -  fold digits
    rc = FoldStringW( MAP_FOLDDIGITS,
                      FoldSrc2,
                      -1,
                      FoldDest,
                      BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       FoldDest,
                       FoldSrc2,
                       "fold digits",
                       &NumErrors );

    //  Variation 2  -  fold digits
    rc = FoldStringW( MAP_FOLDDIGITS,
                      L"\x00b2\x00b3",
                      -1,
                      FoldDest,
                      BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       FoldDest,
                       L"\x0032\x0033",
                       "fold digits (23)",
                       &NumErrors );

    //  Variation 3  -  fold digits
    rc = FoldStringW( MAP_FOLDDIGITS,
                      L"\x00b2\x00b3",
                      -1,
                      NULL,
                      0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       L"\x0032\x0033",
                       "fold digits (23), no DestStr",
                       &NumErrors );



    //
    //  Check precomposed with multiple diacritics.
    //
    //  Variation 1  -  precomp, multi diacritics
    rc = FoldStringW( MAP_PRECOMPOSED,
                      wcMultiComp,
                      3,
                      FoldDest,
                      BUFSIZE );
    CheckReturnValidW( rc,
                       2,
                       FoldDest,
                       L"\x00e9\x0300",
                       "precomp, multi diacritics",
                       &NumErrors );

    //  Variation 2  -  precomp, czone
    rc = FoldStringW( MAP_PRECOMPOSED | MAP_FOLDCZONE,
                      wcCompDigitCZone,
                      -1,
                      FoldDest,
                      BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       FoldDest,
                       L"\x00e9\x0300\x00b2\x003c",
                       "precomp, czone",
                       &NumErrors );

    //  Variation 3  -  precomp, digits
    rc = FoldStringW( MAP_PRECOMPOSED | MAP_FOLDDIGITS,
                      wcCompDigitCZone,
                      -1,
                      FoldDest,
                      BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       FoldDest,
                       L"\x00e9\x0300\x0032\xfe64",
                       "precomp, digits",
                       &NumErrors );

    //  Variation 4  -  precomp, czone, digits
    rc = FoldStringW( MAP_PRECOMPOSED | MAP_FOLDCZONE | MAP_FOLDDIGITS,
                      wcCompDigitCZone,
                      -1,
                      FoldDest,
                      BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       FoldDest,
                       L"\x00e9\x0300\x0032\x003c",
                       "precomp, czone, digits",
                       &NumErrors );


    //
    //  Check composite.
    //

    //  Variation 1  -  comp, czone
    rc = FoldStringW( MAP_COMPOSITE | MAP_FOLDCZONE,
                      wcPrecompDigitCZone,
                      -1,
                      FoldDest,
                      BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       FoldDest,
                       L"\x0065\x0301\x0300\x00b2\x003c",
                       "comp, czone",
                       &NumErrors );

    //  Variation 2  -  comp, digits
    rc = FoldStringW( MAP_COMPOSITE | MAP_FOLDDIGITS,
                      wcPrecompDigitCZone,
                      -1,
                      FoldDest,
                      BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       FoldDest,
                       L"\x0065\x0301\x0300\x0032\xfe64",
                       "comp, digits",
                       &NumErrors );

    //  Variation 3  -  comp, czone, digits
    rc = FoldStringW( MAP_COMPOSITE | MAP_FOLDCZONE | MAP_FOLDDIGITS,
                      wcPrecompDigitCZone,
                      -1,
                      FoldDest,
                      BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       FoldDest,
                       L"\x0065\x0301\x0300\x0032\x003c",
                       "comp, czone, digits",
                       &NumErrors );


    //
    //  MAP_EXPAND_LIGATURES Flag.
    //

    //  Variation 1  -  expand ligatures
    rc = FoldStringW( MAP_EXPAND_LIGATURES,
                      L"abc",
                      -1,
                      FoldDest,
                      BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       FoldDest,
                       L"abc",
                       "expand ligatures 1",
                       &NumErrors );

    //  Variation 2  -  expand ligatures
    rc = FoldStringW( MAP_EXPAND_LIGATURES,
                      L"\x00e6",
                      -1,
                      FoldDest,
                      BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       FoldDest,
                       L"ae",
                       "expand ligatures 2",
                       &NumErrors );

    //  Variation 3  -  expand ligatures
    rc = FoldStringW( MAP_EXPAND_LIGATURES,
                      L"\x00e6",
                      -1,
                      FoldDest,
                      3 );
    CheckReturnValidW( rc,
                       -1,
                       FoldDest,
                       L"ae",
                       "expand ligatures 3",
                       &NumErrors );

    //  Variation 4  -  expand ligatures
    rc = FoldStringW( MAP_EXPAND_LIGATURES,
                      L"\x00e6",
                      1,
                      FoldDest,
                      2 );
    CheckReturnValidW( rc,
                       2,
                       FoldDest,
                       L"ae",
                       "expand ligatures 4",
                       &NumErrors );

    //  Variation 5  -  expand ligatures
    rc = FoldStringW( MAP_EXPAND_LIGATURES,
                      L"\x00e6",
                      1,
                      NULL,
                      0 );
    CheckReturnValidW( rc,
                       2,
                       NULL,
                       L"ae",
                       "expand ligatures 5 - no DestStr",
                       &NumErrors );

    //  Variation 6  -  expand ligatures
    rc = FoldStringW( MAP_EXPAND_LIGATURES | MAP_FOLDCZONE,
                      L"\x00e6\xff26",
                      -1,
                      FoldDest,
                      BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       FoldDest,
                       L"aeF",
                       "expand ligatures 6 - lig & czone",
                       &NumErrors );

    //  Variation 7  -  expand ligatures
    rc = FoldStringW( MAP_EXPAND_LIGATURES | MAP_FOLDCZONE,
                      L"\x00e6\xff26",
                      -1,
                      NULL,
                      0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       L"aeF",
                       "expand ligatures 7 - lig & czone - no DestStr",
                       &NumErrors );

    //  Variation 8  -  expand ligatures
    rc = FoldStringW( MAP_EXPAND_LIGATURES | MAP_FOLDDIGITS,
                      L"\x00e6\x0660",
                      -1,
                      FoldDest,
                      BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       FoldDest,
                       L"ae0",
                       "expand ligatures 8 - lig & digits",
                       &NumErrors );

    //  Variation 9  -  expand ligatures
    rc = FoldStringW( MAP_EXPAND_LIGATURES | MAP_FOLDDIGITS,
                      L"\x00e6\x0660",
                      -1,
                      NULL,
                      0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       L"ae0",
                       "expand ligatures 9 - lig & digits - no DestStr",
                       &NumErrors );

    //  Variation 10  -  expand ligatures
    rc = FoldStringW( MAP_EXPAND_LIGATURES | MAP_FOLDCZONE | MAP_FOLDDIGITS,
                      L"\x00e6\xff26\x0660",
                      -1,
                      FoldDest,
                      BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       FoldDest,
                       L"aeF0",
                       "expand ligatures 10 - lig, czone, & digits",
                       &NumErrors );

    //  Variation 11  -  expand ligatures
    rc = FoldStringW( MAP_EXPAND_LIGATURES | MAP_FOLDCZONE | MAP_FOLDDIGITS,
                      L"\x00e6\xff26\x0660",
                      -1,
                      NULL,
                      0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       L"aeF0",
                       "expand ligatures 11 - lig, czone, & digits - no DestStr",
                       &NumErrors );

    //  Variation 12  -  expand ligatures
    rc = FoldStringW( MAP_EXPAND_LIGATURES,
                      L"\xfb03",
                      -1,
                      FoldDest,
                      BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       FoldDest,
                       L"ffi",
                       "expand ligatures 12",
                       &NumErrors );

    //  Variation 13  -  expand ligatures
    rc = FoldStringW( MAP_EXPAND_LIGATURES,
                      L"\xfb03",
                      -1,
                      NULL,
                      0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       L"ffi",
                       "expand ligatures 13",
                       &NumErrors );



    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}

////////////////////////////////////////////////////////////////////////////
//
//  FS_Ansi
//
//  This routine tests the Ansi version of the API routine.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int FS_Ansi()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code
    BYTE pFoldDestA[BUFSIZE];     // ptr to buffer


    //
    //  FoldStringA.
    //

    //  Variation 1  -  cbSrc = -1
    rc = FoldStringA( MAP_PRECOMPOSED,
                      "AbCd",
                      -1,
                      pFoldDestA,
                      BUFSIZE );
    CheckReturnValidA( rc,
                       -1,
                       pFoldDestA,
                       "AbCd",
                       NULL,
                       "A version cbSrc (-1)",
                       &NumErrors );

    //  Variation 2  -  cbSrc = value
    rc = FoldStringA( MAP_PRECOMPOSED,
                      "AbCd",
                      4,
                      pFoldDestA,
                      BUFSIZE );
    CheckReturnValidA( rc,
                       4,
                       pFoldDestA,
                       "AbCd",
                       NULL,
                       "A version cbSrc (value)",
                       &NumErrors );

    //  Variation 3  -  cbSrc = -1, no DestStr
    rc = FoldStringA( MAP_PRECOMPOSED,
                      "AbCd",
                      -1,
                      NULL,
                      0 );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       "AbCd",
                       NULL,
                       "A version cbSrc (-1), no DestStr",
                       &NumErrors );

    //  Variation 4  -  cbSrc = value, no DestStr
    rc = FoldStringA( MAP_PRECOMPOSED,
                      "AbCd",
                      4,
                      NULL,
                      0 );
    CheckReturnValidA( rc,
                       4,
                       NULL,
                       "AbCd",
                       NULL,
                       "A version cbSrc (value), no DestStr",
                       &NumErrors );


    //
    //  MAP_EXPAND_LIGATURES Flag.
    //

    //  Variation 1  -  expand ligatures
    rc = FoldStringA( MAP_EXPAND_LIGATURES,
                      "abc",
                      -1,
                      pFoldDestA,
                      BUFSIZE );
    CheckReturnValidA( rc,
                       -1,
                       pFoldDestA,
                       "abc",
                       NULL,
                       "A expand ligatures 1",
                       &NumErrors );

    //  Variation 2  -  expand ligatures
    rc = FoldStringA( MAP_EXPAND_LIGATURES,
                      "\xe6",
                      -1,
                      pFoldDestA,
                      BUFSIZE );
    CheckReturnValidA( rc,
                       -1,
                       pFoldDestA,
                       "ae",
                       NULL,
                       "A expand ligatures 2",
                       &NumErrors );

    //  Variation 3  -  expand ligatures
    rc = FoldStringA( MAP_EXPAND_LIGATURES,
                      "\xe6",
                      -1,
                      pFoldDestA,
                      3 );
    CheckReturnValidA( rc,
                       -1,
                       pFoldDestA,
                       "ae",
                       NULL,
                       "A expand ligatures 3",
                       &NumErrors );

    //  Variation 4  -  expand ligatures
    rc = FoldStringA( MAP_EXPAND_LIGATURES,
                      "\xe6",
                      1,
                      pFoldDestA,
                      2 );
    CheckReturnValidA( rc,
                       2,
                       pFoldDestA,
                       "ae",
                       NULL,
                       "A expand ligatures 4",
                       &NumErrors );

    //  Variation 5  -  expand ligatures
    rc = FoldStringA( MAP_EXPAND_LIGATURES,
                      "\xe6",
                      1,
                      NULL,
                      0 );
    CheckReturnValidA( rc,
                       2,
                       NULL,
                       "ae",
                       NULL,
                       "A expand ligatures 5 - no DestStr",
                       &NumErrors );

    //  Variation 6  -  expand ligatures
    rc = FoldStringA( MAP_EXPAND_LIGATURES | MAP_FOLDCZONE,
                      "\xe6G",
                      -1,
                      pFoldDestA,
                      BUFSIZE );
    CheckReturnValidA( rc,
                       -1,
                       pFoldDestA,
                       "aeG",
                       NULL,
                       "A expand ligatures 6 - lig & czone",
                       &NumErrors );

    //  Variation 7  -  expand ligatures
    rc = FoldStringA( MAP_EXPAND_LIGATURES | MAP_FOLDCZONE,
                      "\xe6G",
                      -1,
                      NULL,
                      0 );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       "aeG",
                       NULL,
                       "A expand ligatures 7 - lig & czone - no DestStr",
                       &NumErrors );

    //  Variation 8  -  expand ligatures
    rc = FoldStringA( MAP_EXPAND_LIGATURES | MAP_FOLDDIGITS,
                      "\xe6\xb2",
                      -1,
                      pFoldDestA,
                      BUFSIZE );
    CheckReturnValidA( rc,
                       -1,
                       pFoldDestA,
                       "ae2",
                       NULL,
                       "A expand ligatures 8 - lig & digits",
                       &NumErrors );

    //  Variation 9  -  expand ligatures
    rc = FoldStringA( MAP_EXPAND_LIGATURES | MAP_FOLDDIGITS,
                      "\xe6\xb2",
                      -1,
                      NULL,
                      0 );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       "ae2",
                       NULL,
                       "A expand ligatures 9 - lig & digits - no DestStr",
                       &NumErrors );

    //  Variation 10  -  expand ligatures
    rc = FoldStringA( MAP_EXPAND_LIGATURES | MAP_FOLDCZONE | MAP_FOLDDIGITS,
                      "\xe6G\xb2",
                      -1,
                      pFoldDestA,
                      BUFSIZE );
    CheckReturnValidA( rc,
                       -1,
                       pFoldDestA,
                       "aeG2",
                       NULL,
                       "A expand ligatures 10 - lig, czone, & digits",
                       &NumErrors );

    //  Variation 11  -  expand ligatures
    rc = FoldStringA( MAP_EXPAND_LIGATURES | MAP_FOLDCZONE | MAP_FOLDDIGITS,
                      "\xe6G\xb2",
                      -1,
                      NULL,
                      0 );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       "aeG2",
                       NULL,
                       "A expand ligatures 11 - lig, czone, & digits - no DestStr",
                       &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}
