/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    etftest.c

Abstract:

    Test module for NLS API EnumTimeFormats.

    NOTE: This code was simply hacked together quickly in order to
          test the different code modules of the NLS component.
          This is NOT meant to be a formal regression test.

Revision History:

    08-02-93    JulieB    Created.

--*/



//
//  Include Files.
//

#include "nlstest.h"




//
//  Constant Declarations.
//

#define  ETF_INVALID_FLAGS        ((DWORD)(~(0)))

#define  NUM_TIME_ENGLISH         4
#define  NUM_TIME_JAPAN           4
#define  NUM_TIME_GERMAN          4




//
//  Global Variables.
//

int TimeCtr;




//
//  Forward Declarations.
//

BOOL
InitEnumTimeFormats();

int
ETF_BadParamCheck();

int
ETF_NormalCase();

int
ETF_Ansi();

BOOL
CALLBACK
MyFuncTime(
    LPWSTR pStr);

BOOL
CALLBACK
MyFuncTimeA(
    LPSTR pStr);




//
//  Callback function
//

BOOL CALLBACK MyFuncTime(
    LPWSTR pStr)
{

    if (Verbose)
    {
        while (*pStr)
        {
            printf((*pStr > 0xff) ? "(0x%x)" : "%wc", *pStr);
            pStr++;
        }
        printf("\n");
    }

    TimeCtr++;

    return (TRUE);
}


BOOL CALLBACK MyFuncTimeA(
    LPSTR pStr)
{

    if (Verbose)
    {
        while (*pStr)
        {
            printf((*pStr > 0xff) ? "(0x%x)" : "%c", *pStr);
            pStr++;
        }
        printf("\n");
    }

    TimeCtr++;

    return (TRUE);
}





////////////////////////////////////////////////////////////////////////////
//
//  TestEnumTimeFormats
//
//  Test routine for EnumTimeFormatsW API.
//
//  08-02-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int TestEnumTimeFormats()
{
    int ErrCount = 0;             // error count


    //
    //  Print out what's being done.
    //
    printf("\n\nTESTING EnumTimeFormatsW...\n\n");

    //
    //  Initialize global variables.
    //
    if (!InitEnumTimeFormats())
    {
        printf("\nABORTED TestEnumTimeFormats: Could not Initialize.\n");
        return (1);
    }

    //
    //  Test bad parameters.
    //
    ErrCount += ETF_BadParamCheck();

    //
    //  Test normal cases.
    //
    ErrCount += ETF_NormalCase();

    //
    //  Test Ansi version.
    //
    ErrCount += ETF_Ansi();

    //
    //  Print out result.
    //
    printf("\nEnumTimeFormatsW:  ERRORS = %d\n", ErrCount);

    //
    //  Return total number of errors found.
    //
    return (ErrCount);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitEnumTimeFormats
//
//  This routine initializes the global variables.  If no errors were
//  encountered, then it returns TRUE.  Otherwise, it returns FALSE.
//
//  08-02-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL InitEnumTimeFormats()
{
    //
    //  Initialize date counter.
    //
    TimeCtr = 0;

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  ETF_BadParamCheck
//
//  This routine passes in bad parameters to the API routines and checks to
//  be sure they are handled properly.  The number of errors encountered
//  is returned to the caller.
//
//  08-02-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ETF_BadParamCheck()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


    //
    //  Bad Function.
    //

    //  Variation 1  -  bad function
    TimeCtr = 0;
    rc = EnumTimeFormatsW( NULL,
                           0x0409,
                           0 );
    CheckReturnBadParamEnum( rc,
                             FALSE,
                             ERROR_INVALID_PARAMETER,
                             "Function invalid",
                             &NumErrors,
                             TimeCtr,
                             0 );


    //
    //  Bad Locale.
    //

    //  Variation 1  -  bad locale
    TimeCtr = 0;
    rc = EnumTimeFormatsW( MyFuncTime,
                           (LCID)333,
                           0 );
    CheckReturnBadParamEnum( rc,
                             FALSE,
                             ERROR_INVALID_PARAMETER,
                             "Locale invalid",
                             &NumErrors,
                             TimeCtr,
                             0 );


    //
    //  Invalid Flag.
    //

    //  Variation 1  -  dwFlags = invalid
    TimeCtr = 0;
    rc = EnumTimeFormatsW( MyFuncTime,
                           0x0409,
                           ETF_INVALID_FLAGS );
    CheckReturnBadParamEnum( rc,
                             FALSE,
                             ERROR_INVALID_FLAGS,
                             "Flag invalid",
                             &NumErrors,
                             TimeCtr,
                             0 );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  ETF_NormalCase
//
//  This routine tests the normal cases of the API routine.
//
//  08-02-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ETF_NormalCase()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


    if (Verbose)
    {
        printf("\n----  W version  ----\n\n");
    }

    //  Variation 1  -  english
    TimeCtr = 0;
    rc = EnumTimeFormatsW( MyFuncTime,
                           0x0409,
                           0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          TimeCtr,
                          NUM_TIME_ENGLISH,
                          "English",
                          &NumErrors );

    //  Variation 2  -  japan
    TimeCtr = 0;
    rc = EnumTimeFormatsW( MyFuncTime,
                           0x0411,
                           0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          TimeCtr,
                          NUM_TIME_JAPAN,
                          "Japan",
                          &NumErrors );

    //  Variation 3  -  german
    TimeCtr = 0;
    rc = EnumTimeFormatsW( MyFuncTime,
                           0x0407,
                           0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          TimeCtr,
                          NUM_TIME_GERMAN,
                          "German",
                          &NumErrors );


    //
    //  Use CP ACP.
    //

    //  Variation 1  -  Use CP ACP
    TimeCtr = 0;
    rc = EnumTimeFormatsW( MyFuncTime,
                           0x0409,
                           LOCALE_USE_CP_ACP );
    CheckReturnValidEnum( rc,
                          TRUE,
                          TimeCtr,
                          NUM_TIME_ENGLISH,
                          "Use CP ACP, English",
                          &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  ETF_Ansi
//
//  This routine tests the Ansi version of the API routine.
//
//  08-02-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ETF_Ansi()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


    if (Verbose)
    {
        printf("\n----  A version  ----\n\n");
    }

    //  Variation 1  -  english
    TimeCtr = 0;
    rc = EnumTimeFormatsA( MyFuncTimeA,
                           0x0409,
                           0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          TimeCtr,
                          NUM_TIME_ENGLISH,
                          "A version English",
                          &NumErrors );

    //  Variation 2  -  japan
    TimeCtr = 0;
    rc = EnumTimeFormatsA( MyFuncTimeA,
                           0x0411,
                           0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          TimeCtr,
                          NUM_TIME_JAPAN,
                          "A version Japan",
                          &NumErrors );

    //  Variation 3  -  german
    TimeCtr = 0;
    rc = EnumTimeFormatsA( MyFuncTimeA,
                           0x0407,
                           0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          TimeCtr,
                          NUM_TIME_GERMAN,
                          "A version German",
                          &NumErrors );


    //
    //  Use CP ACP.
    //

    //  Variation 1  -  Use CP ACP
    TimeCtr = 0;
    rc = EnumTimeFormatsA( MyFuncTimeA,
                           0x0409,
                           LOCALE_USE_CP_ACP );
    CheckReturnValidEnum( rc,
                          TRUE,
                          TimeCtr,
                          NUM_TIME_ENGLISH,
                          "A version Use CP ACP, English",
                          &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}
