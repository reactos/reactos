/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    gnftest.c

Abstract:

    Test module for NLS API GetNumberFormat.

    NOTE: This code was simply hacked together quickly in order to
          test the different code modules of the NLS component.
          This is NOT meant to be a formal regression test.

Revision History:

    07-28-93    JulieB    Created.

--*/



//
//  Include Files.
//

#include "nlstest.h"




//
//  Constant Declarations.
//

#define  BUFSIZE             50             // buffer size in wide chars
#define  GNF_INVALID_FLAGS   ((DWORD)(~(LOCALE_NOUSEROVERRIDE)))

#define  GNF_ENGLISH_US      L"1,234,567.44"
#define  GNF_CZECH           L"1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44"




//
//  Global Variables.
//

LCID Locale;

LPWSTR pValue;
LPWSTR pNegValue;

NUMBERFMT NumFmt;

WCHAR lpNumberStr[BUFSIZE];


//
//  Number format buffers must be in line with the pAllLocales global
//  buffer.
//
LPWSTR pPosNumber[] =
{
    L"1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44",    //  0x0402
    L"1,234,567.44",                                            //  0x0404
    L"1,234,567.44",                                            //  0x0804
    L"1,234,567.44",                                            //  0x0c04
    L"1,234,567.44",                                            //  0x1004
    L"1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44",    //  0x0405
    L"1.234.567,44",                                            //  0x0406
    L"1.234.567,44",                                            //  0x0407
    L"1'234'567.44",                                            //  0x0807
    L"1.234.567,44",                                            //  0x0c07
    L"1.234.567,44",                                            //  0x0408
    L"1,234,567.44",                                            //  0x0409
    L"1,234,567.44",                                            //  0x0809
    L"1,234,567.44",                                            //  0x0c09
    L"1,234,567.44",                                            //  0x1009
    L"1,234,567.44",                                            //  0x1409
    L"1,234,567.44",                                            //  0x1809
    L"1.234.567,44",                                            //  0x040a
    L"1,234,567.44",                                            //  0x080a
    L"1.234.567,44",                                            //  0x0c0a
    L"1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44",    //  0x040b
    L"1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44",    //  0x040c
    L"1.234.567,44",                                            //  0x080c
    L"1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44",    //  0x0c0c
    L"1'234'567.44",                                            //  0x100c
    L"1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44",    //  0x040e
    L"1.234.567,44",                                            //  0x040f
    L"1.234.567,44",                                            //  0x0410
    L"1'234'567.44",                                            //  0x0810
    L"1,234,567.44",                                            //  0x0411
    L"1,234,567.44",                                            //  0x0412
    L"1.234.567,44",                                            //  0x0413
    L"1.234.567,44",                                            //  0x0813
    L"1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44",    //  0x0414
    L"1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44",    //  0x0814
    L"1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44",    //  0x0415
    L"1.234.567,44",                                            //  0x0416
    L"1.234.567,44",                                            //  0x0816
    L"1.234.567,44",                                            //  0x0418
    L"1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44",    //  0x0419
    L"1.234.567,44",                                            //  0x041a
    L"1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44",    //  0x041b
    L"1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44",    //  0x041d
    L"1.234.567,44",                                            //  0x041f
    L"1.234.567,44"                                             //  0x0424
};

LPWSTR pNegNumber[] =
{
    L"-1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44",   //  0x0402
    L"-1,234,567.44",                                           //  0x0404
    L"-1,234,567.44",                                           //  0x0804
    L"-1,234,567.44",                                           //  0x0c04
    L"-1,234,567.44",                                           //  0x1004
    L"-1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44",   //  0x0405
    L"-1.234.567,44",                                           //  0x0406
    L"-1.234.567,44",                                           //  0x0407
    L"-1'234'567.44",                                           //  0x0807
    L"-1.234.567,44",                                           //  0x0c07
    L"-1.234.567,44",                                           //  0x0408
    L"-1,234,567.44",                                           //  0x0409
    L"-1,234,567.44",                                           //  0x0809
    L"-1,234,567.44",                                           //  0x0c09
    L"-1,234,567.44",                                           //  0x1009
    L"-1,234,567.44",                                           //  0x1409
    L"-1,234,567.44",                                           //  0x1809
    L"-1.234.567,44",                                           //  0x040a
    L"-1,234,567.44",                                           //  0x080a
    L"-1.234.567,44",                                           //  0x0c0a
    L"-1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44",   //  0x040b
    L"-1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44",   //  0x040c
    L"-1.234.567,44",                                           //  0x080c
    L"-1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44",   //  0x0c0c
    L"-1'234'567.44",                                           //  0x100c
    L"-1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44",   //  0x040e
    L"-1.234.567,44",                                           //  0x040f
    L"-1.234.567,44",                                           //  0x0410
    L"-1'234'567.44",                                           //  0x0810
    L"-1,234,567.44",                                           //  0x0411
    L"-1,234,567.44",                                           //  0x0412
    L"-1.234.567,44",                                           //  0x0413
    L"-1.234.567,44",                                           //  0x0813
    L"-1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44",   //  0x0414
    L"-1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44",   //  0x0814
    L"-1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44",   //  0x0415
    L"-1.234.567,44",                                           //  0x0416
    L"-1.234.567,44",                                           //  0x0816
    L"-1.234.567,44",                                           //  0x0418
    L"-1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44",   //  0x0419
    L"- 1.234.567,44",                                          //  0x041a
    L"-1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44",   //  0x041b
    L"-1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44",   //  0x041d
    L"-1.234.567,44",                                           //  0x041f
    L"-1.234.567,44"                                            //  0x0424
};




//
//  Forward Declarations.
//

BOOL
InitGetNumberFormat();

int
GNF_BadParamCheck();

int
GNF_NormalCase();

int
GNF_Ansi();





////////////////////////////////////////////////////////////////////////////
//
//  TestGetNumberFormat
//
//  Test routine for GetNumberFormatW API.
//
//  07-28-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int TestGetNumberFormat()
{
    int ErrCount = 0;             // error count


    //
    //  Print out what's being done.
    //
    printf("\n\nTESTING GetNumberFormatW...\n\n");

    //
    //  Initialize global variables.
    //
    if (!InitGetNumberFormat())
    {
        printf("\nABORTED TestGetNumberFormat: Could not Initialize.\n");
        return (1);
    }

    //
    //  Test bad parameters.
    //
    ErrCount += GNF_BadParamCheck();

    //
    //  Test normal cases.
    //
    ErrCount += GNF_NormalCase();

    //
    //  Test Ansi version.
    //
    ErrCount += GNF_Ansi();

    //
    //  Print out result.
    //
    printf("\nGetNumberFormatW:  ERRORS = %d\n", ErrCount);

    //
    //  Return total number of errors found.
    //
    return (ErrCount);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitGetNumberFormat
//
//  This routine initializes the global variables.  If no errors were
//  encountered, then it returns TRUE.  Otherwise, it returns FALSE.
//
//  07-28-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL InitGetNumberFormat()
{
    //
    //  Make a Locale.
    //
    Locale = MAKELCID(0x0409, 0);

    //
    //  Initialize the value.
    //
    pValue = L"1234567.4444";
    pNegValue = L"-1234567.4444";

    //
    //  Initialize the number format structure.
    //
    NumFmt.NumDigits = 3;
    NumFmt.LeadingZero = 1;
    NumFmt.Grouping = 3;
    NumFmt.lpDecimalSep = L"/";
    NumFmt.lpThousandSep = L";";
    NumFmt.NegativeOrder = 1;

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  GNF_BadParamCheck
//
//  This routine passes in bad parameters to the API routines and checks to
//  be sure they are handled properly.  The number of errors encountered
//  is returned to the caller.
//
//  07-28-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GNF_BadParamCheck()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code
    NUMBERFMT MyNumFmt;           // number format


    //
    //  Bad Locale.
    //

    //  Variation 1  -  Bad Locale
    rc = GetNumberFormatW( (LCID)333,
                           0,
                           pValue,
                           NULL,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "Bad Locale",
                         &NumErrors );


    //
    //  Null Pointers.
    //

    //  Variation 1  -  lpNumberStr = NULL
    rc = GetNumberFormatW( Locale,
                           0,
                           pValue,
                           NULL,
                           NULL,
                           BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "lpNumberStr NULL",
                         &NumErrors );

    //  Variation 2  -  lpValue = NULL
    rc = GetNumberFormatW( Locale,
                           0,
                           NULL,
                           NULL,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "lpNumberStr NULL",
                         &NumErrors );


    //
    //  Bad Count.
    //

    //  Variation 1  -  cchNumber < 0
    rc = GetNumberFormatW( Locale,
                           0,
                           pValue,
                           NULL,
                           lpNumberStr,
                           -1 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "cchNumber < 0",
                         &NumErrors );


    //
    //  Invalid Flag.
    //

    //  Variation 1  -  dwFlags = invalid
    rc = GetNumberFormatW( Locale,
                           GNF_INVALID_FLAGS,
                           pValue,
                           NULL,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "Flag invalid",
                         &NumErrors );

    //  Variation 2  -  lpFormat and NoUserOverride
    rc = GetNumberFormatW( Locale,
                           LOCALE_NOUSEROVERRIDE,
                           pValue,
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "lpFormat and NoUserOverride",
                         &NumErrors );

    //  Variation 3  -  Use CP ACP, lpFormat and NoUserOverride
    rc = GetNumberFormatW( Locale,
                           LOCALE_USE_CP_ACP | LOCALE_NOUSEROVERRIDE,
                           pValue,
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "Use CP ACP, lpFormat and NoUserOverride",
                         &NumErrors );


    //
    //  Buffer Too Small.
    //

    //  Variation 1  -  cchNumber = too small
    rc = GetNumberFormatW( Locale,
                           0,
                           pValue,
                           NULL,
                           lpNumberStr,
                           2 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INSUFFICIENT_BUFFER,
                         "cchNumber too small",
                         &NumErrors );


    //
    //  Bad format passed in.
    //

    //  Variation 1  -  bad NumDigits
    MyNumFmt.NumDigits = 10;
    MyNumFmt.LeadingZero = 1;
    MyNumFmt.Grouping = 3;
    MyNumFmt.lpDecimalSep = L"/";
    MyNumFmt.lpThousandSep = L";";
    MyNumFmt.NegativeOrder = 1;
    rc = GetNumberFormatW( Locale,
                           0,
                           pValue,
                           &MyNumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad NumDigits",
                         &NumErrors );

    //  Variation 2  -  bad LeadingZero
    MyNumFmt.NumDigits = 3;
    MyNumFmt.LeadingZero = 2;
    MyNumFmt.Grouping = 3;
    MyNumFmt.lpDecimalSep = L"/";
    MyNumFmt.lpThousandSep = L";";
    MyNumFmt.NegativeOrder = 1;
    rc = GetNumberFormatW( Locale,
                           0,
                           pValue,
                           &MyNumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad LeadingZero",
                         &NumErrors );

    //  Variation 3  -  bad Grouping
    MyNumFmt.NumDigits = 3;
    MyNumFmt.LeadingZero = 1;
    MyNumFmt.Grouping = 10000;
    MyNumFmt.lpDecimalSep = L"/";
    MyNumFmt.lpThousandSep = L";";
    MyNumFmt.NegativeOrder = 1;
    rc = GetNumberFormatW( Locale,
                           0,
                           pValue,
                           &MyNumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad Grouping",
                         &NumErrors );

    //  Variation 4  -  bad DecimalSep
    MyNumFmt.NumDigits = 3;
    MyNumFmt.LeadingZero = 1;
    MyNumFmt.Grouping = 3;
    MyNumFmt.lpDecimalSep = NULL;
    MyNumFmt.lpThousandSep = L";";
    MyNumFmt.NegativeOrder = 1;
    rc = GetNumberFormatW( Locale,
                           0,
                           pValue,
                           &MyNumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad DecimalSep",
                         &NumErrors );

    //  Variation 5  -  bad DecimalSep 2
    MyNumFmt.NumDigits = 3;
    MyNumFmt.LeadingZero = 1;
    MyNumFmt.Grouping = 3;
    MyNumFmt.lpDecimalSep = L"////";
    MyNumFmt.lpThousandSep = L";";
    MyNumFmt.NegativeOrder = 1;
    rc = GetNumberFormatW( Locale,
                           0,
                           pValue,
                           &MyNumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad DecimalSep 2",
                         &NumErrors );

    //  Variation 6  -  bad DecimalSep 3
    MyNumFmt.NumDigits = 3;
    MyNumFmt.LeadingZero = 1;
    MyNumFmt.Grouping = 3;
    MyNumFmt.lpDecimalSep = L"6";
    MyNumFmt.lpThousandSep = L";";
    MyNumFmt.NegativeOrder = 1;
    rc = GetNumberFormatW( Locale,
                           0,
                           pValue,
                           &MyNumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad DecimalSep 3",
                         &NumErrors );

    //  Variation 7  -  bad ThousandSep
    MyNumFmt.NumDigits = 3;
    MyNumFmt.LeadingZero = 1;
    MyNumFmt.Grouping = 3;
    MyNumFmt.lpDecimalSep = L"/";
    MyNumFmt.lpThousandSep = NULL;
    MyNumFmt.NegativeOrder = 1;
    rc = GetNumberFormatW( Locale,
                           0,
                           pValue,
                           &MyNumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad ThousandSep",
                         &NumErrors );

    //  Variation 8  -  bad ThousandSep 2
    MyNumFmt.NumDigits = 3;
    MyNumFmt.LeadingZero = 1;
    MyNumFmt.Grouping = 3;
    MyNumFmt.lpDecimalSep = L"/";
    MyNumFmt.lpThousandSep = L";;;;";
    MyNumFmt.NegativeOrder = 1;
    rc = GetNumberFormatW( Locale,
                           0,
                           pValue,
                           &MyNumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad ThousandSep 2",
                         &NumErrors );

    //  Variation 9  -  bad ThousandSep 3
    MyNumFmt.NumDigits = 3;
    MyNumFmt.LeadingZero = 1;
    MyNumFmt.Grouping = 3;
    MyNumFmt.lpDecimalSep = L"/";
    MyNumFmt.lpThousandSep = L"6";
    MyNumFmt.NegativeOrder = 1;
    rc = GetNumberFormatW( Locale,
                           0,
                           pValue,
                           &MyNumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad ThousandSep 3",
                         &NumErrors );

    //  Variation 10  -  bad negative order
    MyNumFmt.NumDigits = 3;
    MyNumFmt.LeadingZero = 1;
    MyNumFmt.Grouping = 3;
    MyNumFmt.lpDecimalSep = L"/";
    MyNumFmt.lpThousandSep = L";";
    MyNumFmt.NegativeOrder = 5;
    rc = GetNumberFormatW( Locale,
                           0,
                           pValue,
                           &MyNumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad negative order",
                         &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  GNF_NormalCase
//
//  This routine tests the normal cases of the API routine.
//
//  07-28-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GNF_NormalCase()
{
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
    rc = GetNumberFormatW( LOCALE_SYSTEM_DEFAULT,
                           0,
                           pValue,
                           NULL,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       GNF_ENGLISH_US,
                       "sys default locale",
                       &NumErrors );

    //  Variation 2  -  Current User Locale
    rc = GetNumberFormatW( LOCALE_USER_DEFAULT,
                           0,
                           pValue,
                           NULL,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       GNF_ENGLISH_US,
                       "current user locale",
                       &NumErrors );


    //
    //  Language Neutral.
    //

    //  Variation 1  -  neutral
    rc = GetNumberFormatW( 0x0000,
                           0,
                           pValue,
                           NULL,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       GNF_ENGLISH_US,
                       "neutral locale",
                       &NumErrors );

    //  Variation 2  -  sys default
    rc = GetNumberFormatW( 0x0400,
                           0,
                           pValue,
                           NULL,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       GNF_ENGLISH_US,
                       "sys default locale",
                       &NumErrors );

    //  Variation 3  -  user default
    rc = GetNumberFormatW( 0x0800,
                           0,
                           pValue,
                           NULL,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       GNF_ENGLISH_US,
                       "user default locale",
                       &NumErrors );

    //  Variation 4  -  sub lang neutral US
    rc = GetNumberFormatW( 0x0009,
                           0,
                           pValue,
                           NULL,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       GNF_ENGLISH_US,
                       "sub lang neutral US",
                       &NumErrors );

    //  Variation 5  -  sub lang neutral Czech
    rc = GetNumberFormatW( 0x0005,
                           0,
                           pValue,
                           NULL,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       GNF_CZECH,
                       "sub lang neutral Czech",
                       &NumErrors );


    //
    //  Use CP ACP.
    //

    //  Variation 1  -  Use CP ACP, System Default Locale
    rc = GetNumberFormatW( LOCALE_SYSTEM_DEFAULT,
                           LOCALE_USE_CP_ACP,
                           pValue,
                           NULL,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       GNF_ENGLISH_US,
                       "Use CP ACP, sys default locale",
                       &NumErrors );


    //
    //  cchNumber.
    //

    //  Variation 1  -  cchNumber = size of lpNumberStr buffer
    rc = GetNumberFormatW( Locale,
                           0,
                           pValue,
                           NULL,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       GNF_ENGLISH_US,
                       "cchNumber = bufsize",
                       &NumErrors );

    //  Variation 2  -  cchNumber = 0
    lpNumberStr[0] = 0x0000;
    rc = GetNumberFormatW( Locale,
                           0,
                           pValue,
                           NULL,
                           lpNumberStr,
                           0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       GNF_ENGLISH_US,
                       "cchNumber zero",
                       &NumErrors );

    //  Variation 3  -  cchNumber = 0, lpNumberStr = NULL
    rc = GetNumberFormatW( Locale,
                           0,
                           pValue,
                           NULL,
                           NULL,
                           0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       GNF_ENGLISH_US,
                       "cchNumber (NULL ptr)",
                       &NumErrors );


    //
    //  lpFormat - pValue = 1234567.4444
    //
    //      NumFmt.NumDigits = 3;
    //      NumFmt.LeadingZero = 1;
    //      NumFmt.Grouping = 3;
    //      NumFmt.lpDecimalSep = L"/";
    //      NumFmt.lpThousandSep = L";";
    //      NumFmt.NegativeOrder = 1;
    //

    //  Variation 1  -  lpFormat
    rc = GetNumberFormatW( 0x0409,
                           0,
                           pValue,
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"1;234;567/444",
                       "lpFormat (1;234;567/444)",
                       &NumErrors );

    //  Variation 2  -  lpFormat leading zero
    rc = GetNumberFormatW( 0x0409,
                           0,
                           L".4444",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"0/444",
                       "lpFormat (0/444)",
                       &NumErrors );

    //  Variation 3  -  lpFormat no decimal
    rc = GetNumberFormatW( 0x0409,
                           0,
                           L"1234567",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"1;234;567/000",
                       "lpFormat (1;234;567/000)",
                       &NumErrors );

    //  Variation 4  -  grouping check
    rc = GetNumberFormatW( 0x0409,
                           0,
                           L"123456",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"123;456/000",
                       "lpFormat (123;456/000)",
                       &NumErrors );

    //  Variation 5  -  grouping check
    rc = GetNumberFormatW( 0x0409,
                           0,
                           L"12",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"12/000",
                       "grouping (12/000)",
                       &NumErrors );

    //  Variation 6  -  rounding check
    NumFmt.NumDigits = 0;
    rc = GetNumberFormatW( 0x0409,
                           0,
                           L".9999",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"1",
                       "rounding (1)",
                       &NumErrors );
    NumFmt.NumDigits = 3;

    //  Variation 7  -  rounding check
    NumFmt.NumDigits = 0;
    NumFmt.LeadingZero = 0;
    rc = GetNumberFormatW( 0x0409,
                           0,
                           L".9999",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"1",
                       "rounding (1) 2",
                       &NumErrors );
    NumFmt.NumDigits = 3;
    NumFmt.LeadingZero = 1;

    //  Variation 8  -  rounding check
    NumFmt.NumDigits = 0;
    rc = GetNumberFormatW( 0x0409,
                           0,
                           L".4999",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"0",
                       "rounding (0)",
                       &NumErrors );
    NumFmt.NumDigits = 3;

    //  Variation 9  -  rounding check
    NumFmt.NumDigits = 0;
    NumFmt.LeadingZero = 0;
    rc = GetNumberFormatW( 0x0409,
                           0,
                           L".4999",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"0",
                       "rounding (0) 2",
                       &NumErrors );
    NumFmt.NumDigits = 3;
    NumFmt.LeadingZero = 1;

    //  Variation 10  -  strip leading zeros
    rc = GetNumberFormatW( 0x0409,
                           0,
                           L"000000034.5",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"34/500",
                       "strip zeros (34/500)",
                       &NumErrors );

    //  Variation 11  -  neg zero value
    rc = GetNumberFormatW( 0x0409,
                           0,
                           L"-0.0001",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"0/000",
                       "neg zero (0/000)",
                       &NumErrors );

    //  Variation 12  -  neg zero value
    rc = GetNumberFormatW( 0x0409,
                           0,
                           L"-0.0009",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"-0/001",
                       "neg zero (-0/001)",
                       &NumErrors );


    //
    //  Flag values.
    //

    //  Variation 1  -  NOUSEROVERRIDE
    rc = GetNumberFormatW( Locale,
                           LOCALE_NOUSEROVERRIDE,
                           pValue,
                           NULL,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       GNF_ENGLISH_US,
                       "NoUserOverride",
                       &NumErrors );


    //
    //  Test all locales - pValue = 1234567.4444
    //

    for (ctr = 0; ctr < NumLocales; ctr++)
    {
        rc = GetNumberFormatW( pAllLocales[ctr],
                               0,
                               pValue,
                               NULL,
                               lpNumberStr,
                               BUFSIZE );
        CheckReturnValidLoopW( rc,
                               -1,
                               lpNumberStr,
                               pPosNumber[ctr],
                               "Pos",
                               pAllLocales[ctr],
                               &NumErrors );
    }


    //
    //  Test all locales - pNegValue = -1234567.4444
    //

    for (ctr = 0; ctr < NumLocales; ctr++)
    {
        rc = GetNumberFormatW( pAllLocales[ctr],
                               0,
                               pNegValue,
                               NULL,
                               lpNumberStr,
                               BUFSIZE );
        CheckReturnValidLoopW( rc,
                               -1,
                               lpNumberStr,
                               pNegNumber[ctr],
                               "Neg",
                               pAllLocales[ctr],
                               &NumErrors );
    }



    //
    //  Special case checks.
    //

    //  Variation 1  -  rounding check

    NumFmt.NumDigits = 3;
    NumFmt.LeadingZero = 1;
    NumFmt.Grouping = 0;
    NumFmt.lpDecimalSep = L".";
    NumFmt.lpThousandSep = L",";
    NumFmt.NegativeOrder = 1;

    rc = GetNumberFormatW( 0x0409,
                           0,
                           L"799.9999",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"800.000",
                       "rounding (800.000)",
                       &NumErrors );

    NumFmt.NumDigits = 3;
    NumFmt.LeadingZero = 1;
    NumFmt.Grouping = 2;
    NumFmt.lpDecimalSep = L".";
    NumFmt.lpThousandSep = L",";
    NumFmt.NegativeOrder = 1;

    rc = GetNumberFormatW( 0x0409,
                           0,
                           L"799.9999",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"8,00.000",
                       "rounding (8,00.000)",
                       &NumErrors );

    rc = GetNumberFormatW( 0x0409,
                           0,
                           L"-799.9999",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"-8,00.000",
                       "rounding (-8,00.000)",
                       &NumErrors );


    //  Variation 2  -  rounding check

    NumFmt.NumDigits = 0 ;
    NumFmt.LeadingZero = 1 ;
    NumFmt.Grouping = 2 ;
    NumFmt.lpDecimalSep = L"." ;
    NumFmt.lpThousandSep = L"," ;
    NumFmt.NegativeOrder = 1 ;

    rc = GetNumberFormatW( 0x0409,
                           0,
                           L"9.500",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"10",
                       "rounding (10)",
                       &NumErrors );

    rc = GetNumberFormatW( 0x0409,
                           0,
                           L"-9.500",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"-10",
                       "rounding (-10)",
                       &NumErrors );

    rc = GetNumberFormatW( 0x0409,
                           0,
                           L"99.500",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"1,00",
                       "rounding (1,00)",
                       &NumErrors );

    rc = GetNumberFormatW( 0x0409,
                           0,
                           L"-99.500",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"-1,00",
                       "rounding (-1,00)",
                       &NumErrors );


    //  Variation 3  -  rounding check

    NumFmt.NumDigits = 2;
    NumFmt.LeadingZero = 1;
    NumFmt.Grouping = 3;
    NumFmt.NegativeOrder = 1;
    NumFmt.lpDecimalSep = L"/";
    NumFmt.lpThousandSep = L";";

    rc = GetNumberFormatW( 0x0409,
                           0,
                           L"1.3",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"1/30",
                       "rounding (1/30)",
                       &NumErrors );

    rc = GetNumberFormatW( 0x0409,
                           0,
                           L"1.399",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"1/40",
                       "rounding (1/40)",
                       &NumErrors );

    rc = GetNumberFormatW( 0x0409,
                           0,
                           L"0.999",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"1/00",
                       "rounding (1/00)",
                       &NumErrors );

    rc = GetNumberFormatW( 0x0409,
                           0,
                           L".999",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"1/00",
                       "rounding (1/00) 2",
                       &NumErrors );

    rc = GetNumberFormatW( 0x0409,
                           0,
                           L"-1.3",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"-1/30",
                       "rounding (-1/30)",
                       &NumErrors );



    //
    //  Variation 4 - rounding check
    //

    NumFmt.NumDigits = 3;
    NumFmt.LeadingZero = 0;
    NumFmt.Grouping = 2;
    NumFmt.NegativeOrder = 1;
    NumFmt.lpDecimalSep = L".";
    NumFmt.lpThousandSep = L",";

    rc = GetNumberFormatW( 0x0409,
                           0,
                           L".9999",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"1.000",
                       "rounding (1.000)",
                       &NumErrors );


    //  Variation 5  -  grouping check

    NumFmt.NumDigits = 3;
    NumFmt.LeadingZero = 1;
    NumFmt.Grouping = 32;
    NumFmt.lpDecimalSep = L".";
    NumFmt.lpThousandSep = L",";
    NumFmt.NegativeOrder = 1;

    rc = GetNumberFormatW( 0x0409,
                           0,
                           L"1234567.999",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"12,34,567.999",
                       "grouping (12,34,567.999)",
                       &NumErrors );

    rc = GetNumberFormatW( 0x0409,
                           0,
                           L"-1234567.999",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"-12,34,567.999",
                       "grouping (-12,34,567.999)",
                       &NumErrors );

    rc = GetNumberFormatW( 0x0409,
                           0,
                           L"9999999.9999",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"1,00,00,000.000",
                       "grouping/rounding (1,00,00,000.000)",
                       &NumErrors );

    rc = GetNumberFormatW( 0x0409,
                           0,
                           L"-9999999.9999",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"-1,00,00,000.000",
                       "grouping/rounding (-1,00,00,000.000)",
                       &NumErrors );


    //  Variation 6  -  grouping check

    NumFmt.NumDigits = 3;
    NumFmt.LeadingZero = 1;
    NumFmt.Grouping = 320;
    NumFmt.lpDecimalSep = L".";
    NumFmt.lpThousandSep = L",";
    NumFmt.NegativeOrder = 1;

    rc = GetNumberFormatW( 0x0409,
                           0,
                           L"123456789.999",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"1234,56,789.999",
                       "grouping (1234,56789.999)",
                       &NumErrors );

    rc = GetNumberFormatW( 0x0409,
                           0,
                           L"-123456789.999",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"-1234,56,789.999",
                       "grouping (-1234,56,789.999)",
                       &NumErrors );

    rc = GetNumberFormatW( 0x0409,
                           0,
                           L"9999999.9999",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"100,00,000.000",
                       "grouping/rounding (100,00,000.000)",
                       &NumErrors );

    rc = GetNumberFormatW( 0x0409,
                           0,
                           L"-9999999.9999",
                           &NumFmt,
                           lpNumberStr,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpNumberStr,
                       L"-100,00,000.000",
                       "grouping/rounding (-100,00,000.000)",
                       &NumErrors );




    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  GNF_Ansi
//
//  This routine tests the Ansi version of the API routine.
//
//  07-28-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GNF_Ansi()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code
    BYTE pNumStrA[BUFSIZE];       // ptr to number string buffer
    NUMBERFMTA NumFmtA;           // number format structure


    //
    //  GetNumberFormatA.
    //

    //  Variation 1  -  cchNumber = size of lpNumberStr buffer
    rc = GetNumberFormatA( Locale,
                           0,
                           "123456.789",
                           NULL,
                           pNumStrA,
                           BUFSIZE );
    CheckReturnValidA( rc,
                       -1,
                       pNumStrA,
                       "123,456.79",
                       NULL,
                       "A version cchNumber = bufsize",
                       &NumErrors );

    //  Variation 2  -  cchNumber = 0
    pNumStrA[0] = 0x00;
    rc = GetNumberFormatA( Locale,
                           0,
                           "123456.789",
                           NULL,
                           pNumStrA,
                           0 );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       "123,456.79",
                       NULL,
                       "A version cchNumber zero",
                       &NumErrors );

    //  Variation 3  -  cchNumber = 0, lpNumberStr = NULL
    rc = GetNumberFormatA( Locale,
                           0,
                           "123456.789",
                           NULL,
                           NULL,
                           0 );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       "123,456.79",
                       NULL,
                       "A version cchNumber (NULL ptr)",
                       &NumErrors );


    //
    //  Use CP ACP.
    //

    //  Variation 1  -  Use CP ACP, cchNumber = bufsize
    rc = GetNumberFormatA( Locale,
                           LOCALE_USE_CP_ACP,
                           "123456.789",
                           NULL,
                           pNumStrA,
                           BUFSIZE );
    CheckReturnValidA( rc,
                       -1,
                       pNumStrA,
                       "123,456.79",
                       NULL,
                       "A version Use CP ACP, cchNumber = bufsize",
                       &NumErrors );


    //
    //  lpFormat - pValue = 1234567.4444
    //

    NumFmtA.NumDigits = 3;
    NumFmtA.LeadingZero = 1;
    NumFmtA.Grouping = 3;
    NumFmtA.lpDecimalSep = "/";
    NumFmtA.lpThousandSep = NULL;
    NumFmtA.NegativeOrder = 3;


    //  Variation 1  -  lpFormat
    rc = GetNumberFormatA( 0x0409,
                           0,
                           "1234567.4444",
                           &NumFmtA,
                           pNumStrA,
                           BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "A version bad ThousandSep",
                         &NumErrors );


    NumFmtA.NumDigits = 3;
    NumFmtA.LeadingZero = 1;
    NumFmtA.Grouping = 3;
    NumFmtA.lpDecimalSep = "/";
    NumFmtA.lpThousandSep = ";";
    NumFmtA.NegativeOrder = 3;

    //  Variation 1  -  lpFormat
    rc = GetNumberFormatA( 0x0409,
                           0,
                           "1234567.4444",
                           &NumFmtA,
                           pNumStrA,
                           BUFSIZE );
    CheckReturnValidA( rc,
                       -1,
                       pNumStrA,
                       "1;234;567/444",
                       NULL,
                       "lpFormat (1;234;567/444)",
                       &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}
