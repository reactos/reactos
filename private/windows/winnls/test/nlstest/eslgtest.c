/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    eslgtest.c

Abstract:

    Test module for NLS API EnumSystemLanguageGroups.

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
#define  ESLG_INVALID_FLAGS   ((DWORD)(~(LGRPID_INSTALLED | LGRPID_SUPPORTED)))

#define  NUM_INSTALLED_LGRPIDS  17
#define  NUM_SUPPORTED_LGRPIDS  17




//
//  Global Variables.
//

int LanguageGroupCtr;




//
//  Forward Declarations.
//

BOOL
InitEnumSystemLanguageGroups();

int
ESLG_BadParamCheck();

int
ESLG_NormalCase();

int
ESLG_Ansi();

BOOL
CALLBACK
MyFuncLanguageGroup(
    LGRPID LangGroup,
    LPWSTR pStr1,
    LPWSTR pStr2,
    DWORD dwFlags,
    LONG_PTR lParam);

BOOL
CALLBACK
MyFuncLanguageGroupA(
    LGRPID LangGroup,
    LPSTR pStr1,
    LPSTR pStr2,
    DWORD dwFlags,
    LONG_PTR lParam);




//
//  Callback function
//

BOOL CALLBACK MyFuncLanguageGroup(
    LGRPID LangGroup,
    LPWSTR pStr1,
    LPWSTR pStr2,
    DWORD dwFlags,
    LONG_PTR lParam)
{
    if (Verbose)
    {
        while (*pStr1)
        {
            printf((*pStr1 > 0xff) ? "(0x%x)" : "%wc", *pStr1);
            pStr1++;
        }
        printf("    -    ");
        while (*pStr2)
        {
            printf((*pStr2 > 0xff) ? "(0x%x)" : "%wc", *pStr2);
            pStr2++;
        }
        printf("    -    ");
        if (dwFlags & LGRPID_SUPPORTED)
        {
            printf("Supported");
        }
        if (dwFlags & LGRPID_INSTALLED)
        {
            printf("Installed");
        }
        printf("\n");
    }

    LanguageGroupCtr++;

    return (TRUE);
}


BOOL CALLBACK MyFuncLanguageGroupA(
    LGRPID LangGroup,
    LPSTR pStr1,
    LPSTR pStr2,
    DWORD dwFlags,
    LONG_PTR lParam)
{
    if (Verbose)
    {
        while (*pStr1)
        {
            printf((*pStr1 > 0xff) ? "(0x%x)" : "%c", *pStr1);
            pStr1++;
        }
        printf("    -    ");
        while (*pStr2)
        {
            printf((*pStr2 > 0xff) ? "(0x%x)" : "%c", *pStr2);
            pStr2++;
        }
        printf("    -    ");
        if (dwFlags & LGRPID_SUPPORTED)
        {
            printf("Supported");
        }
        if (dwFlags & LGRPID_INSTALLED)
        {
            printf("Installed");
        }
        printf("\n");
    }

    LanguageGroupCtr++;

    return (TRUE);
}





////////////////////////////////////////////////////////////////////////////
//
//  TestEnumSystemLanguageGroups
//
//  Test routine for EnumSystemLanguageGroupsW API.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int TestEnumSystemLanguageGroups()
{
    int ErrCount = 0;             // error count


    //
    //  Print out what's being done.
    //
    printf("\n\nTESTING EnumSystemLanguageGroupsW...\n\n");

    //
    //  Initialize global variables.
    //
    if (!InitEnumSystemLanguageGroups())
    {
        printf("\nABORTED TestEnumSystemLanguageGroups: Could not Initialize.\n");
        return (1);
    }

    //
    //  Test bad parameters.
    //
    ErrCount += ESLG_BadParamCheck();

    //
    //  Test normal cases.
    //
    ErrCount += ESLG_NormalCase();

    //
    //  Test Ansi version.
    //
    ErrCount += ESLG_Ansi();

    //
    //  Print out result.
    //
    printf("\nEnumSystemLanguageGroupsW:  ERRORS = %d\n", ErrCount);

    //
    //  Return total number of errors found.
    //
    return (ErrCount);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitEnumSystemLanguageGroups
//
//  This routine initializes the global variables.  If no errors were
//  encountered, then it returns TRUE.  Otherwise, it returns FALSE.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL InitEnumSystemLanguageGroups()
{
    //
    //  Initialize locale counter.
    //
    LanguageGroupCtr = 0;

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  ESLG_BadParamCheck
//
//  This routine passes in bad parameters to the API routines and checks to
//  be sure they are handled properly.  The number of errors encountered
//  is returned to the caller.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ESLG_BadParamCheck()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code



    //
    //  Invalid Function.
    //

    //  Variation 1  -  Function = invalid
    LanguageGroupCtr = 0;
    rc = EnumSystemLanguageGroupsW( NULL,
                                    LGRPID_INSTALLED,
                                    0 );
    CheckReturnBadParamEnum( rc,
                             FALSE,
                             ERROR_INVALID_PARAMETER,
                             "Function invalid",
                             &NumErrors,
                             LanguageGroupCtr,
                             0 );


    //
    //  Invalid Flag.
    //

    //  Variation 1  -  dwFlags = invalid
    LanguageGroupCtr = 0;
    rc = EnumSystemLanguageGroupsW( MyFuncLanguageGroup,
                                    ESLG_INVALID_FLAGS,
                                    0 );
    CheckReturnBadParamEnum( rc,
                             FALSE,
                             ERROR_INVALID_FLAGS,
                             "Flag invalid",
                             &NumErrors,
                             LanguageGroupCtr,
                             0 );

    //  Variation 2  -  dwFlags = both invalid
    LanguageGroupCtr = 0;
    rc = EnumSystemLanguageGroupsW( MyFuncLanguageGroup,
                                    LGRPID_INSTALLED | LGRPID_SUPPORTED,
                                    0 );
    CheckReturnBadParamEnum( rc,
                             FALSE,
                             ERROR_INVALID_FLAGS,
                             "Flag both invalid",
                             &NumErrors,
                             LanguageGroupCtr,
                             0 );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  ESLG_NormalCase
//
//  This routine tests the normal cases of the API routine.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ESLG_NormalCase()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


    if (Verbose)
    {
        printf("\n----  W version  ----\n\n");
    }

    //  Variation 1  -  Installed
    LanguageGroupCtr = 0;
    rc = EnumSystemLanguageGroupsW( MyFuncLanguageGroup,
                                    LGRPID_INSTALLED,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LanguageGroupCtr,
                          NUM_INSTALLED_LGRPIDS,
                          "Flag installed",
                          &NumErrors );

    //  Variation 2  -  Supported
    LanguageGroupCtr = 0;
    rc = EnumSystemLanguageGroupsW( MyFuncLanguageGroup,
                                    LGRPID_SUPPORTED,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LanguageGroupCtr,
                          NUM_SUPPORTED_LGRPIDS,
                          "Flag supported",
                          &NumErrors );



    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  ESLG_Ansi
//
//  This routine tests the Ansi version of the API routine.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ESLG_Ansi()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


    if (Verbose)
    {
        printf("\n----  A version  ----\n\n");
    }

    //  Variation 1  -  installed
    LanguageGroupCtr = 0;
    rc = EnumSystemLanguageGroupsA( MyFuncLanguageGroupA,
                                    LGRPID_INSTALLED,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LanguageGroupCtr,
                          NUM_INSTALLED_LGRPIDS,
                          "A version Flag installed",
                          &NumErrors );

    //  Variation 2  -  Supported
    LanguageGroupCtr = 0;
    rc = EnumSystemLanguageGroupsA( MyFuncLanguageGroupA,
                                    LGRPID_SUPPORTED,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LanguageGroupCtr,
                          NUM_SUPPORTED_LGRPIDS,
                          "A version Flag supported",
                          &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}
