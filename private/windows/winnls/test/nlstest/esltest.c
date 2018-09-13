/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    esltest.c

Abstract:

    Test module for NLS API EnumSystemLocales.

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

#define  BUFSIZE              50            // buffer size in wide chars
#define  ESL_INVALID_FLAGS    ((DWORD)(~(LCID_INSTALLED | LCID_SUPPORTED)))

#define  NUM_INSTALLED_LCIDS  126
#define  NUM_SUPPORTED_LCIDS  126
#define  NUM_ALTERNATE_SORTS  7




//
//  Global Variables.
//

int LocaleCtr;




//
//  Forward Declarations.
//

BOOL
InitEnumSystemLocales();

int
ESL_BadParamCheck();

int
ESL_NormalCase();

int
ESL_Ansi();

BOOL
CALLBACK
MyFuncLocale(
    LPWSTR pStr);

BOOL
CALLBACK
MyFuncLocaleA(
    LPSTR pStr);




//
//  Callback function
//

BOOL CALLBACK MyFuncLocale(
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

    LocaleCtr++;

    return (TRUE);
}


BOOL CALLBACK MyFuncLocaleA(
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

    LocaleCtr++;

    return (TRUE);
}





////////////////////////////////////////////////////////////////////////////
//
//  TestEnumSystemLocales
//
//  Test routine for EnumSystemLocalesW API.
//
//  08-02-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int TestEnumSystemLocales()
{
    int ErrCount = 0;             // error count


    //
    //  Print out what's being done.
    //
    printf("\n\nTESTING EnumSystemLocalesW...\n\n");

    //
    //  Initialize global variables.
    //
    if (!InitEnumSystemLocales())
    {
        printf("\nABORTED TestEnumSystemLocales: Could not Initialize.\n");
        return (1);
    }

    //
    //  Test bad parameters.
    //
    ErrCount += ESL_BadParamCheck();

    //
    //  Test normal cases.
    //
    ErrCount += ESL_NormalCase();

    //
    //  Test Ansi version.
    //
    ErrCount += ESL_Ansi();

    //
    //  Print out result.
    //
    printf("\nEnumSystemLocalesW:  ERRORS = %d\n", ErrCount);

    //
    //  Return total number of errors found.
    //
    return (ErrCount);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitEnumSystemLocales
//
//  This routine initializes the global variables.  If no errors were
//  encountered, then it returns TRUE.  Otherwise, it returns FALSE.
//
//  08-02-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL InitEnumSystemLocales()
{
    //
    //  Initialize locale counter.
    //
    LocaleCtr = 0;

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  ESL_BadParamCheck
//
//  This routine passes in bad parameters to the API routines and checks to
//  be sure they are handled properly.  The number of errors encountered
//  is returned to the caller.
//
//  08-02-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ESL_BadParamCheck()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code



    //
    //  Invalid Function.
    //

    //  Variation 1  -  Function = invalid
    LocaleCtr = 0;
    rc = EnumSystemLocalesW( NULL,
                             LCID_INSTALLED );
    CheckReturnBadParamEnum( rc,
                             FALSE,
                             ERROR_INVALID_PARAMETER,
                             "Function invalid",
                             &NumErrors,
                             LocaleCtr,
                             0 );


    //
    //  Invalid Flag.
    //

    //  Variation 1  -  dwFlags = invalid
    LocaleCtr = 0;
    rc = EnumSystemLocalesW(MyFuncLocale, ESL_INVALID_FLAGS);
    CheckReturnBadParamEnum( rc,
                             FALSE,
                             ERROR_INVALID_FLAGS,
                             "Flag invalid",
                             &NumErrors,
                             LocaleCtr,
                             0 );

    //  Variation 2  -  dwFlags = both invalid
    LocaleCtr = 0;
    rc = EnumSystemLocalesW(MyFuncLocale, LCID_INSTALLED | LCID_SUPPORTED);
    CheckReturnBadParamEnum( rc,
                             FALSE,
                             ERROR_INVALID_FLAGS,
                             "Flag both invalid",
                             &NumErrors,
                             LocaleCtr,
                             0 );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  ESL_NormalCase
//
//  This routine tests the normal cases of the API routine.
//
//  08-02-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ESL_NormalCase()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


    if (Verbose)
    {
        printf("\n----  W version  ----\n\n");
    }

    //  Variation 1  -  Installed
    LocaleCtr = 0;
    rc = EnumSystemLocalesW( MyFuncLocale,
                             LCID_INSTALLED );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_INSTALLED_LCIDS,
                          "Flag installed",
                          &NumErrors );

    //  Variation 2  -  Supported
    LocaleCtr = 0;
    rc = EnumSystemLocalesW( MyFuncLocale,
                             LCID_SUPPORTED );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_SUPPORTED_LCIDS,
                          "Flag supported",
                          &NumErrors );

    //  Variation 3  -  Alternate Sorts
    LocaleCtr = 0;
    rc = EnumSystemLocalesW( MyFuncLocale,
                             LCID_ALTERNATE_SORTS );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_ALTERNATE_SORTS,
                          "Flag alternate sorts",
                          &NumErrors );


    //  Variation 4  -  Installed, Alternate Sorts
    LocaleCtr = 0;
    rc = EnumSystemLocalesW( MyFuncLocale,
                             LCID_INSTALLED | LCID_ALTERNATE_SORTS );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_INSTALLED_LCIDS + NUM_ALTERNATE_SORTS,
                          "Flag installed, alternate sorts",
                          &NumErrors );


    //  Variation 5  -  Supported, Alternate Sorts
    LocaleCtr = 0;
    rc = EnumSystemLocalesW( MyFuncLocale,
                             LCID_SUPPORTED | LCID_ALTERNATE_SORTS );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_SUPPORTED_LCIDS + NUM_ALTERNATE_SORTS,
                          "Flag supported, alternate sorts",
                          &NumErrors );



    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  ESL_Ansi
//
//  This routine tests the Ansi version of the API routine.
//
//  08-02-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ESL_Ansi()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


    if (Verbose)
    {
        printf("\n----  A version  ----\n\n");
    }

    //  Variation 1  -  installed
    LocaleCtr = 0;
    rc = EnumSystemLocalesA( MyFuncLocaleA,
                             LCID_INSTALLED );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_INSTALLED_LCIDS,
                          "A version Flag installed",
                          &NumErrors );

    //  Variation 2  -  Supported
    LocaleCtr = 0;
    rc = EnumSystemLocalesA( MyFuncLocaleA,
                             LCID_SUPPORTED );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_SUPPORTED_LCIDS,
                          "A version Flag supported",
                          &NumErrors );

    //  Variation 3  -  Alternate Sorts
    LocaleCtr = 0;
    rc = EnumSystemLocalesA( MyFuncLocaleA,
                             LCID_ALTERNATE_SORTS );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_ALTERNATE_SORTS,
                          "A version Flag alternate sorts",
                          &NumErrors );


    //  Variation 4  -  Installed, Alternate Sorts
    LocaleCtr = 0;
    rc = EnumSystemLocalesA( MyFuncLocaleA,
                             LCID_INSTALLED | LCID_ALTERNATE_SORTS );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_INSTALLED_LCIDS + NUM_ALTERNATE_SORTS,
                          "A version Flag installed, alternate sorts",
                          &NumErrors );


    //  Variation 5  -  Supported, Alternate Sorts
    LocaleCtr = 0;
    rc = EnumSystemLocalesA( MyFuncLocaleA,
                             LCID_SUPPORTED | LCID_ALTERNATE_SORTS );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_SUPPORTED_LCIDS + NUM_ALTERNATE_SORTS,
                          "A version Flag supported, alternate sorts",
                          &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}
