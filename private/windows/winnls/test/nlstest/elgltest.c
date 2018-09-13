/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    elgltest.c

Abstract:

    Test module for NLS API EnumLanguageGroupLocales.

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
#define  ELGL_INVALID_FLAGS   ((DWORD)(~(0)))

#define  NUM_LG1_LOCALES      66
#define  NUM_LG2_LOCALES      10
#define  NUM_LG3_LOCALES      3
#define  NUM_LG4_LOCALES      1
#define  NUM_LG5_LOCALES      10
#define  NUM_LG6_LOCALES      3
#define  NUM_LG7_LOCALES      2
#define  NUM_LG8_LOCALES      2
#define  NUM_LG9_LOCALES      9
#define  NUM_LG10_LOCALES     6
#define  NUM_LG11_LOCALES     1
#define  NUM_LG12_LOCALES     1
#define  NUM_LG13_LOCALES     18
#define  NUM_LG14_LOCALES     1
#define  NUM_LG15_LOCALES     5
#define  NUM_LG16_LOCALES     1
#define  NUM_LG17_LOCALES     1




//
//  Global Variables.
//

int LocaleCtr;




//
//  Forward Declarations.
//

BOOL
InitEnumLanguageGroupLocales();

int
ELGL_BadParamCheck();

int
ELGL_NormalCase();

int
ELGL_Ansi();

BOOL
CALLBACK
MyFuncLanguageGroupLocale(
    LGRPID LangGroup,
    LCID Locale,
    LPWSTR pStr,
    LONG_PTR lParam);

BOOL
CALLBACK
MyFuncLanguageGroupLocaleA(
    LGRPID LangGroup,
    LCID Locale,
    LPSTR pStr,
    LONG_PTR lParam);




//
//  Callback function
//

BOOL CALLBACK MyFuncLanguageGroupLocale(
    LGRPID LangGroup,
    LCID Locale,
    LPWSTR pStr,
    LONG_PTR lParam)
{
    if (Verbose)
    {
        printf("%x", LangGroup);
        printf("    -    ");
        printf("%x", Locale);
        printf("    -    ");
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


BOOL CALLBACK MyFuncLanguageGroupLocaleA(
    LGRPID LangGroup,
    LCID Locale,
    LPSTR pStr,
    LONG_PTR lParam)
{
    if (Verbose)
    {
        printf("%x", LangGroup);
        printf("    -    ");
        printf("%x", Locale);
        printf("    -    ");
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
//  TestEnumLanguageGroupLocales
//
//  Test routine for EnumLanguageGroupLocalesW API.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int TestEnumLanguageGroupLocales()
{
    int ErrCount = 0;             // error count


    //
    //  Print out what's being done.
    //
    printf("\n\nTESTING EnumLanguageGroupLocalesW...\n\n");

    //
    //  Initialize global variables.
    //
    if (!InitEnumLanguageGroupLocales())
    {
        printf("\nABORTED TestEnumLanguageGroupLocales: Could not Initialize.\n");
        return (1);
    }

    //
    //  Test bad parameters.
    //
    ErrCount += ELGL_BadParamCheck();

    //
    //  Test normal cases.
    //
    ErrCount += ELGL_NormalCase();

    //
    //  Test Ansi version.
    //
    ErrCount += ELGL_Ansi();

    //
    //  Print out result.
    //
    printf("\nEnumLanguageGroupLocalesW:  ERRORS = %d\n", ErrCount);

    //
    //  Return total number of errors found.
    //
    return (ErrCount);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitEnumLanguageGroupLocales
//
//  This routine initializes the global variables.  If no errors were
//  encountered, then it returns TRUE.  Otherwise, it returns FALSE.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL InitEnumLanguageGroupLocales()
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
//  ELGL_BadParamCheck
//
//  This routine passes in bad parameters to the API routines and checks to
//  be sure they are handled properly.  The number of errors encountered
//  is returned to the caller.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ELGL_BadParamCheck()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code



    //
    //  Invalid Function.
    //

    //  Variation 1  -  Function = invalid
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesW( NULL,
                                    1,
                                    0,
                                    0 );
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
    rc = EnumLanguageGroupLocalesW( MyFuncLanguageGroupLocale,
                                    1,
                                    ELGL_INVALID_FLAGS,
                                    0 );
    CheckReturnBadParamEnum( rc,
                             FALSE,
                             ERROR_INVALID_FLAGS,
                             "Flag invalid",
                             &NumErrors,
                             LocaleCtr,
                             0 );

    //  Variation 2  -  dwFlags = both invalid
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesW( MyFuncLanguageGroupLocale,
                                    1,
                                    LGRPID_INSTALLED | LGRPID_SUPPORTED,
                                    0 );
    CheckReturnBadParamEnum( rc,
                             FALSE,
                             ERROR_INVALID_FLAGS,
                             "Flag both invalid",
                             &NumErrors,
                             LocaleCtr,
                             0 );


    //
    //  Invalid Language Group.
    //

    //  Variation 1  -  LanguageGroup = invalid
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesW( MyFuncLanguageGroupLocale,
                                    0,
                                    0,
                                    0 );
    CheckReturnBadParamEnum( rc,
                             FALSE,
                             ERROR_INVALID_PARAMETER,
                             "LanguageGroup invalid (0)",
                             &NumErrors,
                             LocaleCtr,
                             0 );

    //  Variation 2  -  LanguageGroup = invalid
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesW( MyFuncLanguageGroupLocale,
                                    18,
                                    0,
                                    0 );
    CheckReturnBadParamEnum( rc,
                             FALSE,
                             ERROR_INVALID_PARAMETER,
                             "LanguageGroup invalid (18)",
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
//  ELGL_NormalCase
//
//  This routine tests the normal cases of the API routine.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ELGL_NormalCase()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


    if (Verbose)
    {
        printf("\n----  W version  ----\n\n");
    }

    //  Variation 1  -  Language Group 1
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesW( MyFuncLanguageGroupLocale,
                                    1,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG1_LOCALES,
                          "LanguageGroup 1",
                          &NumErrors );

    //  Variation 2  -  Language Group 2
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesW( MyFuncLanguageGroupLocale,
                                    2,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG2_LOCALES,
                          "LanguageGroup 2",
                          &NumErrors );

    //  Variation 3  -  Language Group 3
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesW( MyFuncLanguageGroupLocale,
                                    3,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG3_LOCALES,
                          "LanguageGroup 3",
                          &NumErrors );

    //  Variation 4  -  Language Group 4
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesW( MyFuncLanguageGroupLocale,
                                    4,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG4_LOCALES,
                          "LanguageGroup 4",
                          &NumErrors );

    //  Variation 5  -  Language Group 5
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesW( MyFuncLanguageGroupLocale,
                                    5,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG5_LOCALES,
                          "LanguageGroup 5",
                          &NumErrors );

    //  Variation 6  -  Language Group 6
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesW( MyFuncLanguageGroupLocale,
                                    6,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG6_LOCALES,
                          "LanguageGroup 6",
                          &NumErrors );

    //  Variation 7  -  Language Group 7
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesW( MyFuncLanguageGroupLocale,
                                    7,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG7_LOCALES,
                          "LanguageGroup 7",
                          &NumErrors );

    //  Variation 8  -  Language Group 8
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesW( MyFuncLanguageGroupLocale,
                                    8,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG8_LOCALES,
                          "LanguageGroup 8",
                          &NumErrors );

    //  Variation 9  -  Language Group 9
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesW( MyFuncLanguageGroupLocale,
                                    9,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG9_LOCALES,
                          "LanguageGroup 9",
                          &NumErrors );

    //  Variation 10  -  Language Group 10
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesW( MyFuncLanguageGroupLocale,
                                    10,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG10_LOCALES,
                          "LanguageGroup 10",
                          &NumErrors );

    //  Variation 11  -  Language Group 11
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesW( MyFuncLanguageGroupLocale,
                                    11,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG11_LOCALES,
                          "LanguageGroup 11",
                          &NumErrors );

    //  Variation 12  -  Language Group 12
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesW( MyFuncLanguageGroupLocale,
                                    12,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG12_LOCALES,
                          "LanguageGroup 12",
                          &NumErrors );

    //  Variation 13  -  Language Group 13
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesW( MyFuncLanguageGroupLocale,
                                    13,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG13_LOCALES,
                          "LanguageGroup 13",
                          &NumErrors );

    //  Variation 14  -  Language Group 14
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesW( MyFuncLanguageGroupLocale,
                                    14,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG14_LOCALES,
                          "LanguageGroup 14",
                          &NumErrors );

    //  Variation 15  -  Language Group 15
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesW( MyFuncLanguageGroupLocale,
                                    15,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG15_LOCALES,
                          "LanguageGroup 15",
                          &NumErrors );

    //  Variation 16  -  Language Group 16
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesW( MyFuncLanguageGroupLocale,
                                    16,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG16_LOCALES,
                          "LanguageGroup 16",
                          &NumErrors );

    //  Variation 17  -  Language Group 17
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesW( MyFuncLanguageGroupLocale,
                                    17,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG17_LOCALES,
                          "LanguageGroup 17",
                          &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  ELGL_Ansi
//
//  This routine tests the Ansi version of the API routine.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int ELGL_Ansi()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code


    if (Verbose)
    {
        printf("\n----  A version  ----\n\n");
    }

    //  Variation 1  -  Language Group 1
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesA( MyFuncLanguageGroupLocaleA,
                                    1,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG1_LOCALES,
                          "A version Language Group 1",
                          &NumErrors );

    //  Variation 2  -  Language Group 2
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesA( MyFuncLanguageGroupLocaleA,
                                    2,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG2_LOCALES,
                          "A version Language Group 2",
                          &NumErrors );

    //  Variation 3  -  Language Group 3
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesA( MyFuncLanguageGroupLocaleA,
                                    3,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG3_LOCALES,
                          "A version Language Group 3",
                          &NumErrors );

    //  Variation 4  -  Language Group 4
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesA( MyFuncLanguageGroupLocaleA,
                                    4,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG4_LOCALES,
                          "A version Language Group 4",
                          &NumErrors );

    //  Variation 5  -  Language Group 5
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesA( MyFuncLanguageGroupLocaleA,
                                    5,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG5_LOCALES,
                          "A version Language Group 5",
                          &NumErrors );

    //  Variation 6  -  Language Group 6
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesA( MyFuncLanguageGroupLocaleA,
                                    6,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG6_LOCALES,
                          "A version Language Group 6",
                          &NumErrors );

    //  Variation 7  -  Language Group 7
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesA( MyFuncLanguageGroupLocaleA,
                                    7,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG7_LOCALES,
                          "A version Language Group 7",
                          &NumErrors );

    //  Variation 8  -  Language Group 8
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesA( MyFuncLanguageGroupLocaleA,
                                    8,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG8_LOCALES,
                          "A version Language Group 8",
                          &NumErrors );

    //  Variation 9  -  Language Group 9
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesA( MyFuncLanguageGroupLocaleA,
                                    9,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG9_LOCALES,
                          "A version Language Group 9",
                          &NumErrors );

    //  Variation 10  -  Language Group 10
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesA( MyFuncLanguageGroupLocaleA,
                                    10,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG10_LOCALES,
                          "A version Language Group 10",
                          &NumErrors );

    //  Variation 11  -  Language Group 11
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesA( MyFuncLanguageGroupLocaleA,
                                    11,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG11_LOCALES,
                          "A version Language Group 11",
                          &NumErrors );

    //  Variation 12  -  Language Group 12
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesA( MyFuncLanguageGroupLocaleA,
                                    12,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG12_LOCALES,
                          "A version Language Group 12",
                          &NumErrors );

    //  Variation 13  -  Language Group 13
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesA( MyFuncLanguageGroupLocaleA,
                                    13,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG13_LOCALES,
                          "A version Language Group 13",
                          &NumErrors );

    //  Variation 14  -  Language Group 14
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesA( MyFuncLanguageGroupLocaleA,
                                    14,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG14_LOCALES,
                          "A version Language Group 14",
                          &NumErrors );

    //  Variation 15  -  Language Group 15
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesA( MyFuncLanguageGroupLocaleA,
                                    15,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG15_LOCALES,
                          "A version Language Group 15",
                          &NumErrors );

    //  Variation 16  -  Language Group 16
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesA( MyFuncLanguageGroupLocaleA,
                                    16,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG16_LOCALES,
                          "A version Language Group 16",
                          &NumErrors );

    //  Variation 17  -  Language Group 17
    LocaleCtr = 0;
    rc = EnumLanguageGroupLocalesA( MyFuncLanguageGroupLocaleA,
                                    17,
                                    0,
                                    0 );
    CheckReturnValidEnum( rc,
                          TRUE,
                          LocaleCtr,
                          NUM_LG17_LOCALES,
                          "A version Language Group 17",
                          &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}
