/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    gcftest.c

Abstract:

    Test module for NLS API GetCurrencyFormat.

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
#define  GCF_INVALID_FLAGS   ((DWORD)(~(LOCALE_NOUSEROVERRIDE)))

#define GCF_ENGLISH_US       L"$1,234,567.44"
#define GCF_CZECH            L"1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44 K\x010d"




//
//  Global Variables.
//

LCID Locale;

LPWSTR pValue;
LPWSTR pNegValue;

CURRENCYFMT CurrFmt;

WCHAR lpCurrencyStr[BUFSIZE];


//
//  Currency format buffers must be in line with the pAllLocales global
//  buffer.
//
LPWSTR pPosCurrency[] =
{
    L"1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44 \x043b\x0432",  //  0x0402
    L"NT$1,234,567.44",                                                    //  0x0404
    L"\xffe5\x0031,234,567.44",                                            //  0x0804
    L"HK$1,234,567.44",                                                    //  0x0c04
    L"$1,234,567.44",                                                      //  0x1004
    L"1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44 K\x010d",       //  0x0405
    L"kr 1.234.567,44",                                                    //  0x0406
    L"1.234.567,44 DM",                                                    //  0x0407
    L"SFr. 1'234'567.44",                                                  //  0x0807
    L"\x00f6S 1.234.567,44",                                               //  0x0c07
    L"1.234.567,44 \x0394\x03c1\x3c7",                                     //  0x0408
    L"$1,234,567.44",                                                      //  0x0409
    L"£1,234,567.44",                                                      //  0x0809
    L"$1,234,567.44",                                                      //  0x0c09
    L"$1,234,567.44",                                                      //  0x1009
    L"$1,234,567.44",                                                      //  0x1409
    L"IR£1,234,567.44",                                                    //  0x1809
    L"1.234.567 pta",                                                      //  0x040a
    L"$1,234,567.44",                                                      //  0x080a
    L"1.234.567 pta",                                                      //  0x0c0a
    L"1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44 mk",            //  0x040b
    L"1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44 F",             //  0x040c
    L"1.234.567,44 FB",                                                    //  0x080c
    L"1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44 $",             //  0x0c0c
    L"SFr. 1'234'567.44",                                                  //  0x100c
    L"1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44 Ft",            //  0x040e
    L"1.234.567,44 kr.",                                                   //  0x040f
    L"L. 1.234.567",                                                       //  0x0410
    L"SFr. 1'234'567.44",                                                  //  0x0810
    L"\x00a5\x0031,234,567",                                               //  0x0411
    L"\x20a9\x0031,234,567",                                               //  0x0412
    L"fl 1.234.567,44",                                                    //  0x0413
    L"1.234.567,44 BF",                                                    //  0x0813
    L"kr 1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44",            //  0x0414
    L"kr 1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44",            //  0x0814
    L"1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44 z\x0142",       //  0x0415
    L"R$ 1.234.567,44",                                                    //  0x0416
    L"1.234.567$44 Esc.",                                                  //  0x0816
    L"1.234.567,44 lei",                                                   //  0x0418
    L"1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44\x0440.",        //  0x0419
    L"1.234.567,44 kn",                                                    //  0x041a
    L"1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44 Sk",            //  0x041b
    L"1.234.567,44 kr",                                                    //  0x041d
    L"1.234.567,44 TL",                                                    //  0x041f
    L"1.234.567,44 SIT"                                                    //  0x0424
};

LPWSTR pNegCurrency[] =
{
    L"-1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44 \x043b\x0432", //  0x0402
    L"-NT$1,234,567.44",                                                   //  0x0404
    L"\xffe5-1,234,567.44",                                                //  0x0804
    L"(HK$1,234,567.44)",                                                  //  0x0c04
    L"($1,234,567.44)",                                                    //  0x1004
    L"-1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44 K\x010d",      //  0x0405
    L"kr -1.234.567,44",                                                   //  0x0406
    L"-1.234.567,44 DM",                                                   //  0x0407
    L"SFr.-1'234'567.44",                                                  //  0x0807
    L"-\x00f6S 1.234.567,44",                                              //  0x0c07
    L"-1.234.567,44 \x0394\x03c1\x03c7",                                   //  0x0408
    L"($1,234,567.44)",                                                    //  0x0409
    L"-£1,234,567.44",                                                     //  0x0809
    L"-$1,234,567.44",                                                     //  0x0c09
    L"-$1,234,567.44",                                                     //  0x1009
    L"-$1,234,567.44",                                                     //  0x1409
    L"-IR£1,234,567.44",                                                   //  0x1809
    L"-1.234.567 pta",                                                     //  0x040a
    L"-$1,234,567.44",                                                     //  0x080a
    L"-1.234.567 pta",                                                     //  0x0c0a
    L"-1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44 mk",           //  0x040b
    L"-1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44 F",            //  0x040c
    L"-1.234.567,44 FB",                                                   //  0x080c
    L"(1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44$)",            //  0x0c0c
    L"SFr.-1'234'567.44",                                                  //  0x100c
    L"-1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44 Ft",           //  0x040e
    L"-1.234.567,44 kr.",                                                  //  0x040f
    L"-L. 1.234.567",                                                      //  0x0410
    L"SFr.-1'234'567.44",                                                  //  0x0810
    L"-\x00a5\x0031,234,567",                                              //  0x0411
    L"-\x20a9\x0031,234,567",                                              //  0x0412
    L"fl 1.234.567,44-",                                                   //  0x0413
    L"-1.234.567,44 BF",                                                   //  0x0813
    L"kr -1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44",           //  0x0414
    L"kr -1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44",           //  0x0814
    L"-1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44 z\x0142",      //  0x0415
    L"(R$ 1.234.567,44)",                                                  //  0x0416
    L"-1.234.567$44 Esc.",                                                 //  0x0816
    L"-1.234.567,44 lei",                                                  //  0x0418
    L"-1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44\x0440.",       //  0x0419
    L"-1.234.567,44 kn",                                                   //  0x041a
    L"-1\x00a0\x0032\x0033\x0034\x00a0\x0035\x0036\x0037,44 Sk",           //  0x041b
    L"-1.234.567,44 kr",                                                   //  0x041d
    L"-1.234.567,44 TL",                                                   //  0x041f
    L"-1.234.567,44 SIT"                                                   //  0x0424
};




//
//  Forward Declarations.
//

BOOL
InitGetCurrencyFormat();

int
GCF_BadParamCheck();

int
GCF_NormalCase();

int
GCF_Ansi();





////////////////////////////////////////////////////////////////////////////
//
//  TestGetCurrencyFormat
//
//  Test routine for GetCurrencyFormatW API.
//
//  07-28-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int TestGetCurrencyFormat()
{
    int ErrCount = 0;             // error count


    //
    //  Print out what's being done.
    //
    printf("\n\nTESTING GetCurrencyFormatW...\n\n");

    //
    //  Initialize global variables.
    //
    if (!InitGetCurrencyFormat())
    {
        printf("\nABORTED TestGetCurrencyFormat: Could not Initialize.\n");
        return (1);
    }

    //
    //  Test bad parameters.
    //
    ErrCount += GCF_BadParamCheck();

    //
    //  Test normal cases.
    //
    ErrCount += GCF_NormalCase();

    //
    //  Test Ansi version.
    //
    ErrCount += GCF_Ansi();

    //
    //  Print out result.
    //
    printf("\nGetCurrencyFormatW:  ERRORS = %d\n", ErrCount);

    //
    //  Return total number of errors found.
    //
    return (ErrCount);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitGetCurrencyFormat
//
//  This routine initializes the global variables.  If no errors were
//  encountered, then it returns TRUE.  Otherwise, it returns FALSE.
//
//  07-28-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL InitGetCurrencyFormat()
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
    //  Initialize the currency format structure.
    //
    CurrFmt.NumDigits = 3;
    CurrFmt.LeadingZero = 1;
    CurrFmt.Grouping = 3;
    CurrFmt.lpDecimalSep = L"/";
    CurrFmt.lpThousandSep = L";";
    CurrFmt.lpCurrencySymbol = L"*";
    CurrFmt.PositiveOrder = 3;
    CurrFmt.NegativeOrder = 6;

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  GCF_BadParamCheck
//
//  This routine passes in bad parameters to the API routines and checks to
//  be sure they are handled properly.  The number of errors encountered
//  is returned to the caller.
//
//  07-28-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GCF_BadParamCheck()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code
    CURRENCYFMT MyCurrFmt;        // currency format


    //
    //  Bad Locale.
    //

    //  Variation 1  -  Bad Locale
    rc = GetCurrencyFormatW( (LCID)333,
                             0,
                             pValue,
                             NULL,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "Bad Locale",
                         &NumErrors );


    //
    //  Null Pointers.
    //

    //  Variation 1  -  lpCurrencyStr = NULL
    rc = GetCurrencyFormatW( Locale,
                             0,
                             pValue,
                             NULL,
                             NULL,
                             BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "lpCurrencyStr NULL",
                         &NumErrors );

    //  Variation 2  -  lpValue = NULL
    rc = GetCurrencyFormatW( Locale,
                             0,
                             NULL,
                             NULL,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "lpValue NULL",
                         &NumErrors );


    //
    //  Bad Count.
    //

    //  Variation 1  -  cchCurrency < 0
    rc = GetCurrencyFormatW( Locale,
                             0,
                             pValue,
                             NULL,
                             lpCurrencyStr,
                             -1 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "cchCurrency < 0",
                         &NumErrors );


    //
    //  Invalid Flag.
    //

    //  Variation 1  -  dwFlags = invalid
    rc = GetCurrencyFormatW( Locale,
                             GCF_INVALID_FLAGS,
                             pValue,
                             NULL,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "Flag invalid",
                         &NumErrors );

    //  Variation 2  -  lpFormat and NoUserOverride
    rc = GetCurrencyFormatW( Locale,
                             LOCALE_NOUSEROVERRIDE,
                             pValue,
                             &CurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "lpFormat and NoUserOverride",
                         &NumErrors );

    //  Variation 3  -  Use CP ACP, lpFormat and NoUserOverride
    rc = GetCurrencyFormatW( Locale,
                             LOCALE_USE_CP_ACP | LOCALE_NOUSEROVERRIDE,
                             pValue,
                             &CurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "Use CP ACP, lpFormat and NoUserOverride",
                         &NumErrors );


    //
    //  Buffer Too Small.
    //

    //  Variation 1  -  cchCurrency = too small
    rc = GetCurrencyFormatW( Locale,
                             0,
                             pValue,
                             NULL,
                             lpCurrencyStr,
                             2 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INSUFFICIENT_BUFFER,
                         "cchCurrency too small",
                         &NumErrors );


    //
    //  Bad format passed in.
    //

    //  Variation 1  -  bad NumDigits
    MyCurrFmt.NumDigits = 10;
    MyCurrFmt.LeadingZero = 1;
    MyCurrFmt.Grouping = 3;
    MyCurrFmt.lpDecimalSep = L"/";
    MyCurrFmt.lpThousandSep = L";";
    MyCurrFmt.lpCurrencySymbol = L"*";
    MyCurrFmt.PositiveOrder = 3;
    MyCurrFmt.NegativeOrder = 6;
    rc = GetCurrencyFormatW( Locale,
                             0,
                             pValue,
                             &MyCurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad NumDigits",
                         &NumErrors );

    //  Variation 2  -  bad LeadingZero
    MyCurrFmt.NumDigits = 3;
    MyCurrFmt.LeadingZero = 2;
    MyCurrFmt.Grouping = 3;
    MyCurrFmt.lpDecimalSep = L"/";
    MyCurrFmt.lpThousandSep = L";";
    MyCurrFmt.lpCurrencySymbol = L"*";
    MyCurrFmt.PositiveOrder = 3;
    MyCurrFmt.NegativeOrder = 6;
    rc = GetCurrencyFormatW( Locale,
                             0,
                             pValue,
                             &MyCurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad LeadingZero",
                         &NumErrors );

    //  Variation 3  -  bad Grouping
    MyCurrFmt.NumDigits = 3;
    MyCurrFmt.LeadingZero = 1;
    MyCurrFmt.Grouping = 10000;
    MyCurrFmt.lpDecimalSep = L"/";
    MyCurrFmt.lpThousandSep = L";";
    MyCurrFmt.lpCurrencySymbol = L"*";
    MyCurrFmt.PositiveOrder = 3;
    MyCurrFmt.NegativeOrder = 6;
    rc = GetCurrencyFormatW( Locale,
                             0,
                             pValue,
                             &MyCurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad Grouping",
                         &NumErrors );

    //  Variation 4  -  bad DecimalSep
    MyCurrFmt.NumDigits = 3;
    MyCurrFmt.LeadingZero = 1;
    MyCurrFmt.Grouping = 3;
    MyCurrFmt.lpDecimalSep = NULL;
    MyCurrFmt.lpThousandSep = L";";
    MyCurrFmt.lpCurrencySymbol = L"*";
    MyCurrFmt.PositiveOrder = 3;
    MyCurrFmt.NegativeOrder = 6;
    rc = GetCurrencyFormatW( Locale,
                             0,
                             pValue,
                             &MyCurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad DecimalSep",
                         &NumErrors );

    //  Variation 5  -  bad DecimalSep 2
    MyCurrFmt.NumDigits = 3;
    MyCurrFmt.LeadingZero = 1;
    MyCurrFmt.Grouping = 3;
    MyCurrFmt.lpDecimalSep = L"////";
    MyCurrFmt.lpThousandSep = L";";
    MyCurrFmt.lpCurrencySymbol = L"*";
    MyCurrFmt.PositiveOrder = 3;
    MyCurrFmt.NegativeOrder = 6;
    rc = GetCurrencyFormatW( Locale,
                             0,
                             pValue,
                             &MyCurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad DecimalSep2",
                         &NumErrors );

    //  Variation 6  -  bad DecimalSep 3
    MyCurrFmt.NumDigits = 3;
    MyCurrFmt.LeadingZero = 1;
    MyCurrFmt.Grouping = 3;
    MyCurrFmt.lpDecimalSep = L"6";
    MyCurrFmt.lpThousandSep = L";";
    MyCurrFmt.lpCurrencySymbol = L"*";
    MyCurrFmt.PositiveOrder = 3;
    MyCurrFmt.NegativeOrder = 6;
    rc = GetCurrencyFormatW( Locale,
                             0,
                             pValue,
                             &MyCurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad DecimalSep3",
                         &NumErrors );

    //  Variation 7  -  bad ThousandSep
    MyCurrFmt.NumDigits = 3;
    MyCurrFmt.LeadingZero = 1;
    MyCurrFmt.Grouping = 3;
    MyCurrFmt.lpDecimalSep = L"/";
    MyCurrFmt.lpThousandSep = NULL;
    MyCurrFmt.lpCurrencySymbol = L"*";
    MyCurrFmt.PositiveOrder = 3;
    MyCurrFmt.NegativeOrder = 6;
    rc = GetCurrencyFormatW( Locale,
                             0,
                             pValue,
                             &MyCurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad ThousandSep",
                         &NumErrors );

    //  Variation 8  -  bad ThousandSep 2
    MyCurrFmt.NumDigits = 3;
    MyCurrFmt.LeadingZero = 1;
    MyCurrFmt.Grouping = 3;
    MyCurrFmt.lpDecimalSep = L"/";
    MyCurrFmt.lpThousandSep = L";;;;";
    MyCurrFmt.lpCurrencySymbol = L"*";
    MyCurrFmt.PositiveOrder = 3;
    MyCurrFmt.NegativeOrder = 6;
    rc = GetCurrencyFormatW( Locale,
                             0,
                             pValue,
                             &MyCurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad ThousandSep2",
                         &NumErrors );

    //  Variation 9  -  bad ThousandSep 3
    MyCurrFmt.NumDigits = 3;
    MyCurrFmt.LeadingZero = 1;
    MyCurrFmt.Grouping = 3;
    MyCurrFmt.lpDecimalSep = L"/";
    MyCurrFmt.lpThousandSep = L"6";
    MyCurrFmt.lpCurrencySymbol = L"*";
    MyCurrFmt.PositiveOrder = 3;
    MyCurrFmt.NegativeOrder = 6;
    rc = GetCurrencyFormatW( Locale,
                             0,
                             pValue,
                             &MyCurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad ThousandSep3",
                         &NumErrors );

    //  Variation 10  -  bad Currency Symbol
    MyCurrFmt.NumDigits = 3;
    MyCurrFmt.LeadingZero = 1;
    MyCurrFmt.Grouping = 3;
    MyCurrFmt.lpDecimalSep = L"/";
    MyCurrFmt.lpThousandSep = L";";
    MyCurrFmt.lpCurrencySymbol = NULL;
    MyCurrFmt.PositiveOrder = 3;
    MyCurrFmt.NegativeOrder = 6;
    rc = GetCurrencyFormatW( Locale,
                             0,
                             pValue,
                             &MyCurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad Currency Symbol",
                         &NumErrors );

    //  Variation 11  -  bad Currency Symbol 2
    MyCurrFmt.NumDigits = 3;
    MyCurrFmt.LeadingZero = 1;
    MyCurrFmt.Grouping = 3;
    MyCurrFmt.lpDecimalSep = L"/";
    MyCurrFmt.lpThousandSep = L";";
    MyCurrFmt.lpCurrencySymbol = L"******";
    MyCurrFmt.PositiveOrder = 3;
    MyCurrFmt.NegativeOrder = 6;
    rc = GetCurrencyFormatW( Locale,
                             0,
                             pValue,
                             &MyCurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad Currency Symbol 2",
                         &NumErrors );

    //  Variation 12  -  bad Currency Symbol 3
    MyCurrFmt.NumDigits = 3;
    MyCurrFmt.LeadingZero = 1;
    MyCurrFmt.Grouping = 3;
    MyCurrFmt.lpDecimalSep = L"/";
    MyCurrFmt.lpThousandSep = L";";
    MyCurrFmt.lpCurrencySymbol = L"*6";
    MyCurrFmt.PositiveOrder = 3;
    MyCurrFmt.NegativeOrder = 6;
    rc = GetCurrencyFormatW( Locale,
                             0,
                             pValue,
                             &MyCurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad Currency Symbol 3",
                         &NumErrors );

    //  Variation 13  -  bad Positive Order
    MyCurrFmt.NumDigits = 3;
    MyCurrFmt.LeadingZero = 1;
    MyCurrFmt.Grouping = 3;
    MyCurrFmt.lpDecimalSep = L"/";
    MyCurrFmt.lpThousandSep = L";";
    MyCurrFmt.lpCurrencySymbol = L"*";
    MyCurrFmt.PositiveOrder = 4;
    MyCurrFmt.NegativeOrder = 6;
    rc = GetCurrencyFormatW( Locale,
                             0,
                             pValue,
                             &MyCurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad Positive Order",
                         &NumErrors );


    //  Variation 14  -  bad Negative Order
    MyCurrFmt.NumDigits = 3;
    MyCurrFmt.LeadingZero = 1;
    MyCurrFmt.Grouping = 3;
    MyCurrFmt.lpDecimalSep = L"/";
    MyCurrFmt.lpThousandSep = L";";
    MyCurrFmt.lpCurrencySymbol = L"*";
    MyCurrFmt.PositiveOrder = 3;
    MyCurrFmt.NegativeOrder = 16;
    rc = GetCurrencyFormatW( Locale,
                             0,
                             pValue,
                             &MyCurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "bad Negative Order",
                         &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  GCF_NormalCase
//
//  This routine tests the normal cases of the API routine.
//
//  07-28-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GCF_NormalCase()
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
    rc = GetCurrencyFormatW( LOCALE_SYSTEM_DEFAULT,
                             0,
                             pValue,
                             NULL,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpCurrencyStr,
                       GCF_ENGLISH_US,
                       "System Default Locale",
                       &NumErrors );

    //  Variation 2  -  Current User Locale
    rc = GetCurrencyFormatW( LOCALE_USER_DEFAULT,
                             0,
                             pValue,
                             NULL,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpCurrencyStr,
                       GCF_ENGLISH_US,
                       "Current User Locale",
                       &NumErrors );


    //
    //  Language Neutral.
    //

    //  Variation 1  -  neutral
    rc = GetCurrencyFormatW( 0x0000,
                             0,
                             pValue,
                             NULL,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpCurrencyStr,
                       GCF_ENGLISH_US,
                       "neutral locale",
                       &NumErrors );

    //  Variation 2  -  sys default
    rc = GetCurrencyFormatW( 0x0400,
                             0,
                             pValue,
                             NULL,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpCurrencyStr,
                       GCF_ENGLISH_US,
                       "sys default locale",
                       &NumErrors );

    //  Variation 3  -  user default
    rc = GetCurrencyFormatW( 0x0800,
                             0,
                             pValue,
                             NULL,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpCurrencyStr,
                       GCF_ENGLISH_US,
                       "user default locale",
                       &NumErrors );

    //  Variation 4  -  sub lang neutral US
    rc = GetCurrencyFormatW( 0x0009,
                             0,
                             pValue,
                             NULL,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpCurrencyStr,
                       GCF_ENGLISH_US,
                       "sub lang neutral US",
                       &NumErrors );

    //  Variation 5  -  sub lang neutral Czech
    rc = GetCurrencyFormatW( 0x0005,
                             0,
                             pValue,
                             NULL,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpCurrencyStr,
                       GCF_CZECH,
                       "sub lang neutral Czech",
                       &NumErrors );


    //
    //  Use CP ACP.
    //

    //  Variation 1  -  System Default Locale
    rc = GetCurrencyFormatW( LOCALE_SYSTEM_DEFAULT,
                             LOCALE_USE_CP_ACP,
                             pValue,
                             NULL,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpCurrencyStr,
                       GCF_ENGLISH_US,
                       "Use CP ACP, System Default Locale",
                       &NumErrors );


    //
    //  cchCurrency.
    //

    //  Variation 1  -  cchCurrency = size of lpCurrencyStr buffer
    rc = GetCurrencyFormatW( Locale,
                             0,
                             pValue,
                             NULL,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpCurrencyStr,
                       GCF_ENGLISH_US,
                       "cchCurrency = bufsize",
                       &NumErrors );

    //  Variation 2  -  cchCurrency = 0
    lpCurrencyStr[0] = 0x0000;
    rc = GetCurrencyFormatW( Locale,
                             0,
                             pValue,
                             NULL,
                             lpCurrencyStr,
                             0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       GCF_ENGLISH_US,
                       "cchCurrency zero",
                       &NumErrors );

    //  Variation 3  -  cchCurrency = 0, lpCurrencyStr = NULL
    rc = GetCurrencyFormatW( Locale,
                             0,
                             pValue,
                             NULL,
                             NULL,
                             0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       GCF_ENGLISH_US,
                       "cchCurrency (NULL ptr)",
                       &NumErrors );


    //
    //  lpFormat - pValue = 1234567.4444
    //
    //      CurrFmt.NumDigits = 3;
    //      CurrFmt.LeadingZero = 1;
    //      CurrFmt.Grouping = 3;
    //      CurrFmt.lpDecimalSep = L"/";
    //      CurrFmt.lpThousandSep = L";";
    //      CurrFmt.lpCurrencySymbol = L"*";
    //      CurrFmt.PositiveOrder = 3;
    //      CurrFmt.NegativeOrder = 6;
    //

    //  Variation 1  -  lpFormat
    rc = GetCurrencyFormatW( 0x0409,
                             0,
                             pValue,
                             &CurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpCurrencyStr,
                       L"1;234;567/444 *",
                       "lpFormat (1;234;567/444 *)",
                       &NumErrors );

    //  Variation 2  -  lpFormat leading zero
    rc = GetCurrencyFormatW( 0x0409,
                             0,
                             L".4444",
                             &CurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpCurrencyStr,
                       L"0/444 *",
                       "lpFormat (0/444 *)",
                       &NumErrors );

    //  Variation 3  -  lpFormat no decimal
    rc = GetCurrencyFormatW( 0x0409,
                             0,
                             L"1234567",
                             &CurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpCurrencyStr,
                       L"1;234;567/000 *",
                       "lpFormat (1;234;567/000 *)",
                       &NumErrors );

    //  Variation 4  -  lpFormat
    rc = GetCurrencyFormatW( 0x0409,
                             0,
                             L"-1234567.444",
                             &CurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpCurrencyStr,
                       L"1;234;567/444-*",
                       "lpFormat (1;234;567/444-*)",
                       &NumErrors );

    //  Variation 5  -  lpFormat leading zero
    rc = GetCurrencyFormatW( 0x0409,
                             0,
                             L"-.4444",
                             &CurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpCurrencyStr,
                       L"0/444-*",
                       "lpFormat (0/444-*)",
                       &NumErrors );

    //  Variation 6  -  lpFormat no decimal
    rc = GetCurrencyFormatW( 0x0409,
                             0,
                             L"-1234567",
                             &CurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpCurrencyStr,
                       L"1;234;567/000-*",
                       "lpFormat (1;234;567/000-*)",
                       &NumErrors );


    //
    //  Flag values.
    //

    //  Variation 1  -  NOUSEROVERRIDE
    rc = GetCurrencyFormatW( Locale,
                             LOCALE_NOUSEROVERRIDE,
                             pValue,
                             NULL,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpCurrencyStr,
                       GCF_ENGLISH_US,
                       "NoUserOverride",
                       &NumErrors );


    //
    //  Test all locales - pValue = 1234567.4444
    //
    for (ctr = 0; ctr < NumLocales; ctr++)
    {
        rc = GetCurrencyFormatW( pAllLocales[ctr],
                                 0,
                                 pValue,
                                 NULL,
                                 lpCurrencyStr,
                                 BUFSIZE );
        CheckReturnValidLoopW( rc,
                               -1,
                               lpCurrencyStr,
                               pPosCurrency[ctr],
                               "Pos",
                               pAllLocales[ctr],
                               &NumErrors );
    }



    //
    //  Test all locales - pNegValue = -1234567.4444
    //

    for (ctr = 0; ctr < NumLocales; ctr++)
    {
        rc = GetCurrencyFormatW( pAllLocales[ctr],
                                 0,
                                 pNegValue,
                                 NULL,
                                 lpCurrencyStr,
                                 BUFSIZE );
        CheckReturnValidLoopW( rc,
                               -1,
                               lpCurrencyStr,
                               pNegCurrency[ctr],
                               "Neg",
                               pAllLocales[ctr],
                               &NumErrors );
    }


    //
    //  Special case checks.
    //

    //  Variation 1  -  rounding check

    CurrFmt.NumDigits = 3;
    CurrFmt.LeadingZero = 1;
    CurrFmt.Grouping = 2;
    CurrFmt.lpDecimalSep = L".";
    CurrFmt.lpThousandSep = L",";
    CurrFmt.NegativeOrder = 1;
    CurrFmt.PositiveOrder = 0;
    CurrFmt.lpCurrencySymbol = L"$";

    rc = GetCurrencyFormatW( 0x0409,
                             0,
                             L"799.9999",
                             &CurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpCurrencyStr,
                       L"$8,00.000",
                       "rounding ($8,00.000)",
                       &NumErrors );

    rc = GetCurrencyFormatW( 0x0409,
                             0,
                             L"-799.9999",
                             &CurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpCurrencyStr,
                       L"-$8,00.000",
                       "rounding (-$8,00.000)",
                       &NumErrors );


    //  Variation 2  -  rounding check

    CurrFmt.NumDigits = 0 ;
    CurrFmt.LeadingZero = 1 ;
    CurrFmt.Grouping = 2 ;
    CurrFmt.lpDecimalSep = L"." ;
    CurrFmt.lpThousandSep = L"," ;
    CurrFmt.NegativeOrder = 1 ;
    CurrFmt.PositiveOrder = 0 ;
    CurrFmt.lpCurrencySymbol = L"$" ;

    rc = GetCurrencyFormatW( 0x0409,
                             0,
                             L"9.500",
                             &CurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpCurrencyStr,
                       L"$10",
                       "rounding ($10)",
                       &NumErrors );

    rc = GetCurrencyFormatW( 0x0409,
                             0,
                             L"-9.500",
                             &CurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpCurrencyStr,
                       L"-$10",
                       "rounding (-$10)",
                       &NumErrors );

    rc = GetCurrencyFormatW( 0x0409,
                             0,
                             L"99.500",
                             &CurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpCurrencyStr,
                       L"$1,00",
                       "rounding ($1,00)",
                       &NumErrors );

    rc = GetCurrencyFormatW( 0x0409,
                             0,
                             L"-99.500",
                             &CurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpCurrencyStr,
                       L"-$1,00",
                       "rounding (-$1,00)",
                       &NumErrors );


    //  Variation 3  -  grouping check

    CurrFmt.NumDigits = 3;
    CurrFmt.LeadingZero = 1;
    CurrFmt.Grouping = 32;
    CurrFmt.lpDecimalSep = L".";
    CurrFmt.lpThousandSep = L",";
    CurrFmt.NegativeOrder = 1;
    CurrFmt.PositiveOrder = 0;
    CurrFmt.lpCurrencySymbol = L"$";

    rc = GetCurrencyFormatW( 0x0409,
                             0,
                             L"1234567.999",
                             &CurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpCurrencyStr,
                       L"$12,34,567.999",
                       "grouping ($12,34,567.999)",
                       &NumErrors );

    rc = GetCurrencyFormatW( 0x0409,
                             0,
                             L"-1234567.999",
                             &CurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpCurrencyStr,
                       L"-$12,34,567.999",
                       "grouping (-$12,34,567.999)",
                       &NumErrors );

    rc = GetCurrencyFormatW( 0x0409,
                             0,
                             L"9999999.9999",
                             &CurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpCurrencyStr,
                       L"$1,00,00,000.000",
                       "grouping/rounding ($1,00,00,000.000)",
                       &NumErrors );

    rc = GetCurrencyFormatW( 0x0409,
                             0,
                             L"-9999999.9999",
                             &CurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpCurrencyStr,
                       L"-$1,00,00,000.000",
                       "grouping/rounding (-$1,00,00,000.000)",
                       &NumErrors );


    //  Variation 4  -  grouping check

    CurrFmt.NumDigits = 3;
    CurrFmt.LeadingZero = 1;
    CurrFmt.Grouping = 320;
    CurrFmt.lpDecimalSep = L".";
    CurrFmt.lpThousandSep = L",";
    CurrFmt.NegativeOrder = 1;
    CurrFmt.PositiveOrder = 0;
    CurrFmt.lpCurrencySymbol = L"$";

    rc = GetCurrencyFormatW( 0x0409,
                             0,
                             L"123456789.999",
                             &CurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpCurrencyStr,
                       L"$1234,56,789.999",
                       "grouping ($1234,56,789.999)",
                       &NumErrors );

    rc = GetCurrencyFormatW( 0x0409,
                             0,
                             L"-123456789.999",
                             &CurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpCurrencyStr,
                       L"-$1234,56,789.999",
                       "grouping (-$1234,56789.999)",
                       &NumErrors );

    rc = GetCurrencyFormatW( 0x0409,
                             0,
                             L"9999999.9999",
                             &CurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpCurrencyStr,
                       L"$100,00,000.000",
                       "grouping/rounding ($100,00,000.000)",
                       &NumErrors );

    rc = GetCurrencyFormatW( 0x0409,
                             0,
                             L"-9999999.9999",
                             &CurrFmt,
                             lpCurrencyStr,
                             BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpCurrencyStr,
                       L"-$100,00,000.000",
                       "grouping/rounding (-$100,00,000.000)",
                       &NumErrors );





    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}

////////////////////////////////////////////////////////////////////////////
//
//  GCF_Ansi
//
//  This routine tests the Ansi version of the API routine.
//
//  07-28-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GCF_Ansi()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code
    BYTE pCurStrA[BUFSIZE];       // ptr to currency string buffer
    CURRENCYFMTA CurrFmtA;        // currency format structure


    //
    //  GetCurrencyFormatA.
    //

    //  Variation 1  -  cchCurrency = size of lpCurrencyStr buffer
    rc = GetCurrencyFormatA( Locale,
                             0,
                             "123456.789",
                             NULL,
                             pCurStrA,
                             BUFSIZE );
    CheckReturnValidA( rc,
                       -1,
                       pCurStrA,
                       "$123,456.79",
                       NULL,
                       "A version cchCurrency = bufsize",
                       &NumErrors );

    //  Variation 2  -  cchCurrency = 0
    pCurStrA[0] = 0x00;
    rc = GetCurrencyFormatA( Locale,
                             0,
                             "123456.789",
                             NULL,
                             pCurStrA,
                             0 );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       "$123,456.79",
                       NULL,
                       "A version cchCurrency = bufsize, no Dest",
                       &NumErrors );

    //  Variation 3  -  cchCurrency = 0, lpCurrencyStr = NULL
    rc = GetCurrencyFormatA(Locale, 0, "123456.789", NULL, NULL, 0);
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       "$123,456.79",
                       NULL,
                       "A version cchCurrency (NULL ptr)",
                       &NumErrors );

    //
    //  Use CP ACP.
    //

    //  Variation 1  -  Use CP ACP, cchCurrency = bufsize
    rc = GetCurrencyFormatA( Locale,
                             LOCALE_USE_CP_ACP,
                             "123456.789",
                             NULL,
                             pCurStrA,
                             BUFSIZE );
    CheckReturnValidA( rc,
                       -1,
                       pCurStrA,
                       "$123,456.79",
                       NULL,
                       "A version Use CP ACP, cchCurrency = bufsize",
                       &NumErrors );


    //
    //  lpFormat - pValue = 1234567.4444
    //

    CurrFmtA.NumDigits = 3;
    CurrFmtA.LeadingZero = 1;
    CurrFmtA.Grouping = 3;
    CurrFmtA.lpDecimalSep = "/";
    CurrFmtA.lpThousandSep = ";";
    CurrFmtA.lpCurrencySymbol = "*";
    CurrFmtA.PositiveOrder = 3;
    CurrFmtA.NegativeOrder = 6;


    //  Variation 1  -  lpFormat
    rc = GetCurrencyFormatA( 0x0409,
                             0,
                             "1234567.4444",
                             &CurrFmtA,
                             pCurStrA,
                             BUFSIZE );
    CheckReturnValidA( rc,
                       -1,
                       pCurStrA,
                       "1;234;567/444 *",
                       NULL,
                       "A version cchCurrency (NULL ptr)",
                       &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}
