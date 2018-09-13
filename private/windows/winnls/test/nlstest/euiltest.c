/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    euiltest.c

Abstract:

    Test module for NLS API EnumUILanguages.

    NOTE: This code was simply hacked together quickly in order to
          test the different code modules of the NLS component.
          This is NOT meant to be a formal regression test.

Revision History:

    03-10-98    JulieB    Created.

--*/



//
//  Include Files.
//

#include "nlstest.h"




//
//  Constant Declarations.
//

#define  BUFSIZE              50            // buffer size in wide chars
#define  EUIL_INVALID_FLAGS   ((DWORD)(~(0)))

#define  NUM_UI_LANGS         1




//
//  Global Variables.
//

int UILanguageCtr;




//
//  Forward Declarations.
//

BOOL
InitEnumUILanguages();

int
EUIL_BadParamCheck();

int
EUIL_NormalCase();

int
EUIL_Ansi();

BOOL
CALLBACK
MyFuncUILanguage(
    LPWSTR pStr,
    LONG_PTR lParam);

BOOL
CALLBACK
MyFuncUILanguageA(
    LPSTR pStr,
    LONG_PTR lParam);




//
//  Callback function
//

BOOL CALLBACK MyFuncUILanguage(
    LPWSTR pStr,
    LONG_PTR lParam)
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

    UILanguageCtr++;

    return (TRUE);
}


BOOL CALLBACK MyFuncUILanguageA(
    LPSTR pStr,
    LONG_PTR lParam)
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

    UILanguageCtr++;

    return (TRUE);
}





////////////////////////////////////////////////////////////////////////////
//
//  TestEnumUILanguages
//
//  Test routine for EnumUILanguagesW API.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int TestEnumUILanguages()
{
    int ErrCount = 0;             // error count


    //
    //  Print out what's being done.
    //
    printf("\n\nTESTING EnumUILanguagesW...\n\n");

    //
    //  Initialize global variables.
    //
    if (!InitEnumUILanguages())
    {
        printf("\nABORTED TestEnumUILanguages: Could not Initialize.\n");
        return (1);
    }

    //
    //  Test bad parameters.
    //
    ErrCount += EUIL_BadParamCheck();

    //
    //  Test normal cases.
    //
    ErrCount += EUIL_NormalCase();

    //
    //  Test Ansi version.
    //
    ErrCount += EUIL_Ansi();

    //
    //  Print out result.
    //
    printf("\nEnumUILanguagesW:  ERRORS = %d\n", ErrCount);

    //
    //  Return total number of errors found.
    //
    return (ErrCount);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitEnumUILanguages
//
//  This routine initializes the global variables.  If no errors were
//  encountered, then it returns TRUE.  Otherwise, it returns FALSE.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL InitEnumUILanguages()
{
    //
    //  Initialize locale counter.
    //
    UILanguageCtr = 0;

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  EUIL_BadParamCheck
//
//  This routine passes in bad parameters to the API routines and checks to
//  be sure they are handled properly.  The number of errors encountered
//  is returned to the caller.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int EUIL_BadParamCheck()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code



    //
    //  Invalid Function.
    //

    //  Variation 1  -  Function = invalid
    UILanguageCtr = 0;
    rc = EnumUILanguagesW( NULL,
                           0,
                           0 );
    CheckReturnBadParamEnum( rc,
                             FALSE,
                             ERROR_INVALID_PARAMETER,
                             "Function invalid",
                             &NumErrors,
                             UILanguageCtr,
                             0 );


    //
    //  Invalid Flag.
    //

    //  Variation 1  -  dwFlags = invalid
    UILanguageCtr = 0;
    rc = EnumUILanguagesW( MyFuncUILanguage,
                           EUIL_INVALID_FLAGS,
                           0 );
    CheckReturnBadParamEnum( rc,
                             FALSE,
                             ERROR_INVALID_FLAGS,
                             "Flag invalid",
                             &NumErrors,
                             UILanguageCtr,
                             0 );

    //  Variation 2  -  dwFlags = both invalid
    UILanguageCtr = 0;
    rc = EnumUILanguagesW( MyFuncUILanguage,
                           LGRPID_INSTALLED | LGRPID_SUPPORTED,
                           0 );
    CheckReturnBadParamEnum( rc,
                             FALSE,
                             ERROR_INVALID_FLAGS,
                             "Flag both invalid",
                             &NumErrors,
                             UILanguageCtr,
                             0 );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  EUIL_NormalCase
//
//  This routine tests the normal cases of the API routine.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int EUIL_NormalCase()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


    if (Verbose)
    {
        printf("\n----  W version  ----\n\n");
    }

    //  Variation 1  -  valid
    UILanguageCtr = 0;
    rc = EnumUILanguagesW( MyFuncUILanguage,
                           0,
                           0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          UILanguageCtr,
                          NUM_UI_LANGS,
                          "Valid",
                          &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  EUIL_Ansi
//
//  This routine tests the Ansi version of the API routine.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int EUIL_Ansi()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


    if (Verbose)
    {
        printf("\n----  A version  ----\n\n");
    }

    //  Variation 1  -  valid
    UILanguageCtr = 0;
    rc = EnumUILanguagesA( MyFuncUILanguageA,
                           0,
                           0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          UILanguageCtr,
                          NUM_UI_LANGS,
                          "A version Valid",
                          &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}
