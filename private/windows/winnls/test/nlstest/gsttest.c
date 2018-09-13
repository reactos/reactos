/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    gsttest.c

Abstract:

    Test module for NLS API GetStringType.

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

#define  BUFSIZE             50
#define  GST_INVALID_FLAGS   ((DWORD)(~(CT_CTYPE1 | CT_CTYPE2 | CT_CTYPE3)))

#define  CT1_LOCASE_LETTER   L"\x0102\x0102"
#define  CT1_UPCASE_LETTER   L"\x0101\x0101"


#ifdef JDB

    // JDB - Fix to get around C compiler bug - it tries to translate
    //       from Unicode to Ansi
    #define  CT1_NUMBER          L"\x0084\x0084"

#else

    // JDB - Fix to get around C compiler bug.
    WCHAR CT1_NUMBER[] = {0x0084, 0x0084, 0x0000};

#endif


#define  CT1_PUNCTUATION     L"\x0010\x0010"

#define  CT2_LOCASE_LETTER   L"\x0001\x0001"
#define  CT2_UPCASE_LETTER   L"\x0001\x0001"
#define  CT2_NUMBER          L"\x0003\x0003"
#define  CT2_PUNCTUATION     L"\x000b\x000b"

#define  CT3_VALUE           L"\x8040\x8040"
#define  CT3_SYMBOL          L"\x0048\x0048"




//
//  Global Variables.
//

#define pGSTSrcLower    L"th"

#define pGSTSrcUpper    L"TH"

#define pGSTSrcNumber   L"12"

#define pGSTSrcPunct    L";?"


WORD  pCharType[BUFSIZE * 2];




//
//  Forward Declarations.
//

BOOL
InitGetStringType();

int
GST_BadParamCheck();

int
GST_NormalCase();

int
GST_Ansi();

void
CheckReturnGetStringType(
    int CurrentReturn,
    int ExpectedReturn,
    LPWSTR pCurrentString,
    LPWSTR pExpectedString,
    int ExpectedSize,
    LPSTR pErrString,
    int *pNumErrors);





////////////////////////////////////////////////////////////////////////////
//
//  TestGetStringType
//
//  Test routine for GetStringTypeW API.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int TestGetStringType()
{
    int ErrCount = 0;             // error count


    //
    //  Print out what's being done.
    //
    printf("\n\nTESTING GetStringTypeW...\n\n");

    //
    //  Initialize global variables.
    //
    if (!InitGetStringType())
    {
        printf("\nABORTED TestGetStringType: Could not Initialize.\n");
        return (1);
    }

    //
    //  Test bad parameters.
    //
    ErrCount += GST_BadParamCheck();

    //
    //  Test normal cases.
    //
    ErrCount += GST_NormalCase();

    //
    //  Test Ansi version.
    //
    ErrCount += GST_Ansi();

    //
    //  Print out result.
    //
    printf("\nGetStringTypeW:  ERRORS = %d\n", ErrCount);

    //
    //  Return total number of errors found.
    //
    return (ErrCount);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitGetStringType
//
//  This routine initializes the global variables.  If no errors were
//  encountered, then it returns TRUE.  Otherwise, it returns FALSE.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL InitGetStringType()
{
    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  GST_BadParamCheck
//
//  This routine passes in bad parameters to the API routine and checks to
//  be sure they are handled properly.  The number of errors encountered
//  is returned to the caller.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GST_BadParamCheck()
{
    int NumErrors = 0;            // error count - to be returned
    BOOL rc;                      // return code


    //
    //  Null Pointers.
    //

    //  Variation 1  -  lpSrcStr = NULL
    rc = GetStringTypeW( CT_CTYPE1,
                         NULL,
                         -1,
                         pCharType );
    CheckReturnBadParam( rc,
                         FALSE,
                         ERROR_INVALID_PARAMETER,
                         "lpSrcStr NULL",
                         &NumErrors );

    //  Variation 2  -  lpCharType = NULL
    rc = GetStringTypeW( CT_CTYPE1,
                         pGSTSrcUpper,
                         -1,
                         NULL );
    CheckReturnBadParam( rc,
                         FALSE,
                         ERROR_INVALID_PARAMETER,
                         "lpCharType NULL",
                         &NumErrors );


    //
    //  Bad Counts.
    //

    //  Variation 1  -  cbSrc = 0
    rc = GetStringTypeW( CT_CTYPE1,
                         pGSTSrcUpper,
                         0,
                         pCharType );
    CheckReturnBadParam( rc,
                         FALSE,
                         ERROR_INVALID_PARAMETER,
                         "cbSrc = 0",
                         &NumErrors );


    //
    //  Zero or Invalid Flag Values.
    //

    //  Variation 1  -  dwInfoType = invalid
    rc = GetStringTypeW( GST_INVALID_FLAGS,
                         pGSTSrcUpper,
                         -1,
                         pCharType );
    CheckReturnBadParam( rc,
                         FALSE,
                         ERROR_INVALID_FLAGS,
                         "dwInfoType invalid",
                         &NumErrors );

    //  Variation 2  -  dwInfoType = 0
    rc = GetStringTypeW( 0,
                         pGSTSrcUpper,
                         -1,
                         pCharType );
    CheckReturnBadParam( rc,
                         FALSE,
                         ERROR_INVALID_FLAGS,
                         "dwInfoType zero",
                         &NumErrors );

    //  Variation 3  -  illegal combo case 1,2
    rc = GetStringTypeW( CT_CTYPE1 | CT_CTYPE2,
                         pGSTSrcUpper,
                         -1,
                         pCharType );
    CheckReturnBadParam( rc,
                         FALSE,
                         ERROR_INVALID_FLAGS,
                         "illegal combo case 1,2",
                         &NumErrors );

    //  Variation 4  -  illegal combo case 1,3
    rc = GetStringTypeW( CT_CTYPE1 | CT_CTYPE3,
                         pGSTSrcUpper,
                         -1,
                         pCharType );
    CheckReturnBadParam( rc,
                         FALSE,
                         ERROR_INVALID_FLAGS,
                         "illegal combo case 1,3",
                         &NumErrors );

    //  Variation 5  -  illegal combo case 2,3
    rc = GetStringTypeW( CT_CTYPE2 | CT_CTYPE3,
                         pGSTSrcUpper,
                         -1,
                         pCharType );
    CheckReturnBadParam( rc,
                         FALSE,
                         ERROR_INVALID_FLAGS,
                         "illegal combo case 2,3",
                         &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  GST_NormalCase
//
//  This routine tests the normal cases of the API routine.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GST_NormalCase()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


#ifdef PERF

  DbgBreakPoint();

#endif


    //
    //  GetStringTypeW
    //

    //
    //  cbSrc.
    //

    //  Variation 1  -  cbSrc = -1
    rc = GetStringTypeW( CT_CTYPE1,
                         pGSTSrcUpper,
                         -1,
                         pCharType );
    CheckReturnEqual( rc,
                      FALSE,
                      "cbSrc (-1)",
                      &NumErrors );

    //  Variation 2  -  cbSrc = value
    rc = GetStringTypeW( CT_CTYPE1,
                         pGSTSrcUpper,
                         WC_STRING_LEN(pGSTSrcUpper),
                         pCharType );
    CheckReturnEqual( rc,
                      FALSE,
                      "cbSrc (value)",
                      &NumErrors );


    //
    //  CTYPE 1.
    //

    //  Variation 1  -  ctype1, lower
    rc = GetStringTypeW( CT_CTYPE1,
                         pGSTSrcLower,
                         -1,
                         pCharType );
    CheckReturnGetStringType( rc,
                              TRUE,
                              pCharType,
                              CT1_LOCASE_LETTER,
                              2,
                              "ctype1, lower",
                              &NumErrors );

    //  Variation 2  -  ctype1, upper case letter
    rc = GetStringTypeW( CT_CTYPE1,
                         pGSTSrcUpper,
                         -1,
                         pCharType );
    CheckReturnGetStringType( rc,
                              TRUE,
                              pCharType,
                              CT1_UPCASE_LETTER,
                              2,
                              "ctype1, upper",
                              &NumErrors );

    //  Variation 3  -  ctype1, number
    rc = GetStringTypeW( CT_CTYPE1,
                         pGSTSrcNumber,
                         -1,
                         pCharType );
    CheckReturnGetStringType( rc,
                              TRUE,
                              pCharType,
                              CT1_NUMBER,
                              2,
                              "ctype1, number",
                              &NumErrors );

    //  Variation 4  -  ctype1, punctuation
    rc = GetStringTypeW( CT_CTYPE1,
                         pGSTSrcPunct,
                         -1,
                         pCharType );
    CheckReturnGetStringType( rc,
                              TRUE,
                              pCharType,
                              CT1_PUNCTUATION,
                              2,
                              "ctype1, punctuation",
                              &NumErrors );

    //  Variation 5  -  ctype 1
    rc = GetStringTypeW( CT_CTYPE1,
                         L"\xff53",
                         -1,
                         pCharType );
    CheckReturnGetStringType( rc,
                              TRUE,
                              pCharType,
                              L"\x0102\x0020",
                              2,
                              "ctype1 (0xff53)",
                              &NumErrors );



    //
    //  CTYPE 2.
    //

    //  Variation 1  -  ctype2, lower
    rc = GetStringTypeW( CT_CTYPE2,
                         pGSTSrcLower,
                         -1,
                         pCharType );
    CheckReturnGetStringType( rc,
                              TRUE,
                              pCharType,
                              CT2_LOCASE_LETTER,
                              2,
                              "ctype2, lower",
                              &NumErrors );

    //  Variation 2  -  ctype2, upper case letter
    rc = GetStringTypeW( CT_CTYPE2,
                         pGSTSrcUpper,
                         -1,
                         pCharType );
    CheckReturnGetStringType( rc,
                              TRUE,
                              pCharType,
                              CT2_UPCASE_LETTER,
                              2,
                              "ctype2, upper",
                              &NumErrors );

    //  Variation 3  -  ctype2, number
    rc = GetStringTypeW( CT_CTYPE2,
                         pGSTSrcNumber,
                         -1,
                         pCharType );
    CheckReturnGetStringType( rc,
                              TRUE,
                              pCharType,
                              CT2_NUMBER,
                              2,
                              "ctype2, number",
                              &NumErrors );

    //  Variation 4  -  ctype2, punctuation
    rc = GetStringTypeW( CT_CTYPE2,
                         pGSTSrcPunct,
                         -1,
                         pCharType );
    CheckReturnGetStringType( rc,
                              TRUE,
                              pCharType,
                              CT2_PUNCTUATION,
                              2,
                              "ctype2, punctuation",
                              &NumErrors );

    //  Variation 5  -  ctype 2
    rc = GetStringTypeW( CT_CTYPE2,
                         L"\xff53",
                         -1,
                         pCharType );
    CheckReturnGetStringType( rc,
                              TRUE,
                              pCharType,
                              L"\x0001\x0000",
                              2,
                              "ctype2 (0xff53)",
                              &NumErrors );



    //
    //  CTYPE 3.
    //

    //  Variation 1  -  ctype 3 should return zeros
    rc = GetStringTypeW( CT_CTYPE3,
                         pGSTSrcLower,
                         -1,
                         pCharType );
    CheckReturnGetStringType( rc,
                              TRUE,
                              pCharType,
                              CT3_VALUE,
                              2,
                              "ctype3 zero",
                              &NumErrors );

    //  Variation 2  -  ctype 3 symbol
    rc = GetStringTypeW( CT_CTYPE3,
                         pGSTSrcPunct,
                         -1,
                         pCharType );
    CheckReturnGetStringType( rc,
                              TRUE,
                              pCharType,
                              CT3_SYMBOL,
                              2,
                              "ctype3 symbol",
                              &NumErrors );

    //  Variation 3  -  ctype 3
    rc = GetStringTypeW( CT_CTYPE3,
                         L"\xff53",
                         -1,
                         pCharType );
    CheckReturnGetStringType( rc,
                              TRUE,
                              pCharType,
                              L"\x8080\x0000",
                              2,
                              "ctype3 (0xff53)",
                              &NumErrors );



////////////////////////////////////////////////////////////////////////////


    //
    //  GetStringTypeExW
    //

    //
    //  cbSrc.
    //

    //  Variation 1  -  cbSrc = -1
    rc = GetStringTypeExW( 0x0409,
                           CT_CTYPE1,
                           pGSTSrcUpper,
                           -1,
                           pCharType );
    CheckReturnEqual( rc,
                      FALSE,
                      "Ex cbSrc (-1)",
                      &NumErrors );


    //
    //  CTYPE 1.
    //

    //  Variation 1  -  ctype1, lower
    rc = GetStringTypeExW( 0x0409,
                           CT_CTYPE1,
                           pGSTSrcLower,
                           -1,
                           pCharType );
    CheckReturnGetStringType( rc,
                              TRUE,
                              pCharType,
                              CT1_LOCASE_LETTER,
                              2,
                              "Ex ctype1 lower",
                              &NumErrors );


    //
    //  CTYPE 2.
    //

    //  Variation 1  -  ctype2, lower
    rc = GetStringTypeExW( 0x0409,
                           CT_CTYPE2,
                           pGSTSrcLower,
                           -1,
                           pCharType );
    CheckReturnGetStringType( rc,
                              TRUE,
                              pCharType,
                              CT2_LOCASE_LETTER,
                              2,
                              "Ex ctype2 lower",
                              &NumErrors );


    //
    //  CTYPE 3.
    //

    //  Variation 1  -  ctype 3 should return zeros
    rc = GetStringTypeExW( 0x0409,
                           CT_CTYPE3,
                           pGSTSrcLower,
                           -1,
                           pCharType );
    CheckReturnGetStringType( rc,
                              TRUE,
                              pCharType,
                              CT3_VALUE,
                              2,
                              "Ex ctype3 zero",
                              &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  GST_Ansi
//
//  This routine tests the Ansi version of the API routine.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GST_Ansi()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code



    //
    //  GetStringTypeA
    //

    //
    //  cbSrc.
    //

    //  Variation 1  -  cbSrc = -1
    rc = GetStringTypeA( 0x0409,
                         CT_CTYPE1,
                         "TH",
                         -1,
                         pCharType );
    CheckReturnGetStringType( rc,
                              TRUE,
                              pCharType,
                              CT1_UPCASE_LETTER,
                              2,
                              "A version cbSrc (-1)",
                              &NumErrors );

    //  Variation 2  -  cbSrc = value
    rc = GetStringTypeA( 0x0409,
                         CT_CTYPE1,
                         "TH",
                         2,
                         pCharType );
    CheckReturnGetStringType( rc,
                              TRUE,
                              pCharType,
                              CT1_UPCASE_LETTER,
                              2,
                              "A version cbSrc (value)",
                              &NumErrors );


    //
    //  CTYPE 1.
    //

    //  Variation 1  -  Ab
    rc = GetStringTypeA( 0x0409,
                         CT_CTYPE1,
                         "Ab",
                         -1,
                         pCharType );
    CheckReturnGetStringType( rc,
                              TRUE,
                              pCharType,
                              L"\x0181\x0182\x0020",
                              3,
                              "A version ctype1 (Ab)",
                              &NumErrors );


    //
    //  CTYPE 2.
    //

    //  Variation 1  -  Ab
    rc = GetStringTypeA( 0x0409,
                         CT_CTYPE2,
                         "Ab",
                         -1,
                         pCharType );
    CheckReturnGetStringType( rc,
                              TRUE,
                              pCharType,
                              L"\x0001\x0001\x0000",
                              3,
                              "A version ctype2 (Ab)",
                              &NumErrors );


    //
    //  CTYPE 3.
    //

    //  Variation 1  -  Ab
    rc = GetStringTypeA( 0x0409,
                         CT_CTYPE3,
                         "Ab",
                         -1,
                         pCharType );
    CheckReturnGetStringType( rc,
                              TRUE,
                              pCharType,
                              L"\x8040\x8040\x0000",
                              3,
                              "A version ctype3 (Ab)",
                              &NumErrors );


    //
    //   Check invalid chars.
    //

    //  Variation 1  -  invalid chars
    rc = GetStringTypeA( 0x0411,
                         CT_CTYPE1,
                         "\xa0\xfd\xfe\xff\x85\x40\x81\x02\x81",
                         9,
                         pCharType );
    CheckReturnGetStringType( rc,
                              TRUE,
                              pCharType,
                              L"\x0000\x0000\x0000\x0000\x0000\x0000\x0000",
                              7,
                              "A version ctype1 (invalid chars)",
                              &NumErrors );

    //  Variation 2  -  invalid chars
    rc = GetStringTypeA( 0x0411,
                         CT_CTYPE2,
                         "\xa0\xfd\xfe\xff\x85\x40\x81\x02\x81",
                         9,
                         pCharType );
    CheckReturnGetStringType( rc,
                              TRUE,
                              pCharType,
                              L"\x0000\x0000\x0000\x0000\x0000\x0000\x0000",
                              7,
                              "A version ctype2 (invalid chars)",
                              &NumErrors );

    //  Variation 3  -  invalid chars
    rc = GetStringTypeA( 0x0411,
                         CT_CTYPE3,
                         "\xa0\xfd\xfe\xff\x85\x40\x81\x02\x81",
                         9,
                         pCharType );
    CheckReturnGetStringType( rc,
                              TRUE,
                              pCharType,
                              L"\x0000\x0000\x0000\x0000\x0000\x0000\x0000",
                              7,
                              "A version ctype3 (invalid chars)",
                              &NumErrors );



////////////////////////////////////////////////////////////////////////////


    //
    //  GetStringTypeExA
    //

    //
    //  cbSrc.
    //

    //  Variation 1  -  cbSrc = -1
    rc = GetStringTypeExA( 0x0409,
                           CT_CTYPE1,
                           "TH",
                           -1,
                           pCharType );
    CheckReturnGetStringType( rc,
                              TRUE,
                              pCharType,
                              CT1_UPCASE_LETTER,
                              2,
                              "Ex A version cbSrc (-1)",
                              &NumErrors );

    //  Variation 2  -  cbSrc = value
    rc = GetStringTypeExA( 0x0409,
                           CT_CTYPE1,
                           "TH",
                           2,
                           pCharType );
    CheckReturnGetStringType( rc,
                              TRUE,
                              pCharType,
                              CT1_UPCASE_LETTER,
                              2,
                              "Ex A version cbSrc (value)",
                              &NumErrors );


    //
    //  CTYPE 1.
    //

    //  Variation 1  -  Ab
    rc = GetStringTypeExA( 0x0409,
                           CT_CTYPE1,
                           "Ab",
                           -1,
                           pCharType );
    CheckReturnGetStringType( rc,
                              TRUE,
                              pCharType,
                              L"\x0181\x0182\x0020",
                              3,
                              "Ex A version ctype1 (Ab)",
                              &NumErrors );


    //
    //  CTYPE 2.
    //

    //  Variation 1  -  Ab
    rc = GetStringTypeExA( 0x0409,
                           CT_CTYPE2,
                           "Ab",
                           -1,
                           pCharType );
    CheckReturnGetStringType( rc,
                              TRUE,
                              pCharType,
                              L"\x0001\x0001\x0000",
                              3,
                              "Ex A version ctype2 (Ab)",
                              &NumErrors );


    //
    //  CTYPE 3.
    //

    //  Variation 1  -  Ab
    rc = GetStringTypeExA( 0x0409,
                           CT_CTYPE3,
                           "Ab",
                           -1,
                           pCharType );
    CheckReturnGetStringType( rc,
                              TRUE,
                              pCharType,
                              L"\x8040\x8040\x0000",
                              3,
                              "Ex A version ctype3 (Ab)",
                              &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  CheckReturnGetStringType
//
//  Checks the return code from the valid GetStringType[A|W] call.  It
//  prints out the appropriate error if the incorrect result is found.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

void CheckReturnGetStringType(
    int CurrentReturn,
    int ExpectedReturn,
    LPWSTR pCurrentString,
    LPWSTR pExpectedString,
    int ExpectedSize,
    LPSTR pErrString,
    int *pNumErrors)
{
    int ctr;                 // loop counter


    if ( (CurrentReturn != ExpectedReturn) ||
         ( (pCurrentString != NULL) &&
           (CompStringsW(pCurrentString, pExpectedString, ExpectedSize)) ) )
    {
        printf("ERROR: %s - \n", pErrString);
        printf("  Return = %d, Expected = %d\n", CurrentReturn, ExpectedReturn);

        if (pCurrentString != NULL)
        {
            printf("       ");
            for (ctr = 0; ctr < ExpectedSize; ctr++)
            {
                printf("%x ", pCurrentString[ctr]);
            }
            printf("\n");
        }

        (*pNumErrors)++;
    }
}
