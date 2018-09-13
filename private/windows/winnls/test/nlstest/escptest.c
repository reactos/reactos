/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    escptest.c

Abstract:

    Test module for NLS API EnumSystemCodePages.

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

#define  BUFSIZE             50             // buffer size in wide chars
#define  ESCP_INVALID_FLAGS  ((DWORD)(~(CP_INSTALLED | CP_SUPPORTED)))

#define  NUM_INSTALLED_CPS   73
#define  NUM_SUPPORTED_CPS   134




//
//  Global Variables.
//

int CodePageCtr;




//
//  Forward Declarations.
//

BOOL
InitEnumSystemCodePages();

int
ESCP_BadParamCheck();

int
ESCP_NormalCase();

int
ESCP_Ansi();

BOOL
CALLBACK
MyFuncCP(
    LPWSTR pStr);

BOOL
CALLBACK
MyFuncCPA(
    LPSTR pStr);




//
//  Callback function
//

BOOL CALLBACK MyFuncCP(
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

    CodePageCtr++;

    return (TRUE);
}


BOOL CALLBACK MyFuncCPA(
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

    CodePageCtr++;

    return (TRUE);
}





////////////////////////////////////////////////////////////////////////////
//
//  TestEnumSystemCodePages
//
//  Test routine for EnumSystemCodePagesW API.
//
//  08-02-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int TestEnumSystemCodePages()
{
    int ErrCount = 0;             // error count


    //
    //  Print out what's being done.
    //
    printf("\n\nTESTING EnumSystemCodePagesW...\n\n");

    //
    //  Initialize global variables.
    //
    if (!InitEnumSystemCodePages())
    {
        printf("\nABORTED TestEnumSystemCodePages: Could not Initialize.\n");
        return (1);
    }

    //
    //  Test bad parameters.
    //
    ErrCount += ESCP_BadParamCheck();

    //
    //  Test normal cases.
    //
    ErrCount += ESCP_NormalCase();

    //
    //  Test Ansi version.
    //
    ErrCount += ESCP_Ansi();

    //
    //  Print out result.
    //
    printf("\nEnumSystemCodePagesW:  ERRORS = %d\n", ErrCount);

    //
    //  Return total number of errors found.
    //
    return (ErrCount);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitEnumSystemCodePages
//
//  This routine initializes the global variables.  If no errors were
//  encountered, then it returns TRUE.  Otherwise, it returns FALSE.
//
//  08-02-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL InitEnumSystemCodePages()
{
    //
    //  Initialize code page counter.
    //
    CodePageCtr = 0;

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  ESCP_BadParamCheck
//
//  This routine passes in bad parameters to the API routines and checks to
//  be sure they are handled properly.  The number of errors encountered
//  is returned to the caller.
//
//  08-02-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ESCP_BadParamCheck()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


    //
    //  Invalid Function.
    //

    //  Variation 1  -  function = invalid
    CodePageCtr = 0;
    rc = EnumSystemCodePagesW( NULL,
                               CP_INSTALLED );
    CheckReturnBadParamEnum( rc,
                             FALSE,
                             ERROR_INVALID_PARAMETER,
                             "Function invalid",
                             &NumErrors,
                             CodePageCtr,
                             0 );


    //
    //  Invalid Flag.
    //

    //  Variation 1  -  dwFlags = invalid
    CodePageCtr = 0;
    rc = EnumSystemCodePagesW( MyFuncCP,
                               ESCP_INVALID_FLAGS );
    CheckReturnBadParamEnum( rc,
                             FALSE,
                             ERROR_INVALID_FLAGS,
                             "Flag invalid",
                             &NumErrors,
                             CodePageCtr,
                             0 );

    //  Variation 2  -  dwFlags = both invalid
    CodePageCtr = 0;
    rc = EnumSystemCodePagesW( MyFuncCP,
                               CP_INSTALLED | CP_SUPPORTED );
    CheckReturnBadParamEnum( rc,
                             FALSE,
                             ERROR_INVALID_FLAGS,
                             "Flag both invalid",
                             &NumErrors,
                             CodePageCtr,
                             0 );

    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  ESCP_NormalCase
//
//  This routine tests the normal cases of the API routine.
//
//  08-02-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ESCP_NormalCase()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


    if (Verbose)
    {
        printf("\n----  W version  ----\n\n");
    }

    //  Variation 1  -  installed
    CodePageCtr = 0;
    rc = EnumSystemCodePagesW( MyFuncCP,
                               CP_INSTALLED );
    CheckReturnValidEnum( rc,
                          TRUE,
                          CodePageCtr,
                          NUM_INSTALLED_CPS,
                          "Flag installed",
                          &NumErrors );

    //  Variation 2  -  Supported
    CodePageCtr = 0;
    rc = EnumSystemCodePagesW( MyFuncCP,
                               CP_SUPPORTED );
    CheckReturnValidEnum( rc,
                          TRUE,
                          CodePageCtr,
                          NUM_SUPPORTED_CPS,
                          "Flag supported",
                          &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  ESCP_Ansi
//
//  This routine tests the Ansi version of the API routine.
//
//  08-02-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ESCP_Ansi()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


    if (Verbose)
    {
        printf("\n----  A version  ----\n\n");
    }

    //  Variation 1  -  installed
    CodePageCtr = 0;
    rc = EnumSystemCodePagesA( MyFuncCPA,
                               CP_INSTALLED );
    CheckReturnValidEnum( rc,
                          TRUE,
                          CodePageCtr,
                          NUM_INSTALLED_CPS,
                          "A version Flag installed",
                          &NumErrors );

    //  Variation 2  -  Supported
    CodePageCtr = 0;
    rc = EnumSystemCodePagesA( MyFuncCPA,
                               CP_SUPPORTED );
    CheckReturnValidEnum( rc,
                          TRUE,
                          CodePageCtr,
                          NUM_SUPPORTED_CPS,
                          "A version Flag supported",
                          &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}
