/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    dbtest.c

Abstract:

    Test module for NLS API IsDBCSLeadByte and IsDBCSLeadByteEx.

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
//  Forward Declarations.
//

int
DB_BadParamCheck();

int
DB_NormalCase();

void
CheckReturnIsDBCS(
    int CurrentReturn,
    int ExpectedReturn,
    LPSTR pErrString,
    int *pNumErrors);





////////////////////////////////////////////////////////////////////////////
//
//  TestIsDBCSLeadByte
//
//  Test routine for IsDBCSLeadByte API.
//
//  06-14-91 JulieB       Created.
////////////////////////////////////////////////////////////////////////////

int TestIsDBCSLeadByte()
{
    int ErrCount = 0;             // error count


    //
    //  Print out what's being done.
    //
    printf("\n\nTESTING IsDBCSLeadByte and IsDBCSLeadByteEx...\n\n");

    //
    //  Test bad parameters.
    //
    ErrCount += DB_BadParamCheck();

    //
    //  Test normal cases.
    //
    ErrCount += DB_NormalCase();

    //
    //  Print out result.
    //
    printf("\nIsDBCSLeadByte:  ERRORS = %d\n", ErrCount);

    //
    //  Return total number of errors found.
    //
    return (ErrCount);
}


////////////////////////////////////////////////////////////////////////////
//
//  DB_BadParamCheck
//
//  This routine passes in bad parameters to the API routine and checks to
//  be sure they are handled properly.  The number of errors encountered
//  is returned to the caller.
//
//  06-14-91 JulieB       Created.
////////////////////////////////////////////////////////////////////////////

int DB_BadParamCheck()
{
    int NumErrors = 0;            // error count - to be returned
    BYTE ch;                      // character to check
    BOOL rc;                      // return code


    //
    //  Invalid Code Page.
    //

    //  Variation 1  -  CodePage = invalid
    ch = 0x00;
    rc = IsDBCSLeadByteEx(5, ch);
    CheckReturnBadParam( rc,
                         FALSE,
                         ERROR_INVALID_PARAMETER,
                         "CodePage invalid",
                         &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  DB_NormalCase
//
//  This routine tests the normal cases of the API routine.
//
//  06-14-91 JulieB       Created.
////////////////////////////////////////////////////////////////////////////

int DB_NormalCase()
{
    int NumErrors = 0;            // error count - to be returned
    BYTE ch;                      // character to check
    BOOL rc;                      // return code


#ifdef PERF

  DbgBreakPoint();

#endif

    //--------------------//
    //  IsDBCSLeadByte    //
    //--------------------//


    //
    //  Different values for ch.
    //

    //  Variation 1  -  ch = 0x00
    ch = 0x00;
    rc = IsDBCSLeadByte(ch);
    CheckReturnIsDBCS( rc,
                       FALSE,
                       "ch = 0x00",
                       &NumErrors );

    //  Variation 2  -  ch = 0x23
    ch = 0x23;
    rc = IsDBCSLeadByte(ch);
    CheckReturnIsDBCS( rc,
                       FALSE,
                       "ch = 0x23",
                       &NumErrors );

    //  Variation 3  -  ch = 0xb3
    ch = 0xb3;
    rc = IsDBCSLeadByte(ch);
    CheckReturnIsDBCS( rc,
                       FALSE,
                       "ch = 0xb3",
                       &NumErrors );

    //  Variation 4  -  ch = 0xff
    ch = 0xff;
    rc = IsDBCSLeadByte(ch);
    CheckReturnIsDBCS( rc,
                       FALSE,
                       "ch = 0xff",
                       &NumErrors );



#ifdef JDB

    //
    //  DBCS Chars for Japanese - cp 932.
    //

    //  Variation 1  -  DBCS lead byte  0x81
    rc = IsDBCSLeadByte(0x81);
    CheckReturnIsDBCS( rc,
                       TRUE,
                       "DBCS 0x81",
                       &NumErrors );

    //  Variation 2  -  DBCS lead byte  0x85
    rc = IsDBCSLeadByte(0x85);
    CheckReturnIsDBCS( rc,
                       TRUE,
                       "DBCS 0x85",
                       &NumErrors );

    //  Variation 3  -  DBCS lead byte  0x9f
    rc = IsDBCSLeadByte(0x9f);
    CheckReturnIsDBCS( rc,
                       TRUE,
                       "DBCS 0x9f",
                       &NumErrors );

    //  Variation 4  -  DBCS lead byte  0xe0
    rc = IsDBCSLeadByte(0xe0);
    CheckReturnIsDBCS( rc,
                       TRUE,
                       "DBCS 0xe0",
                       &NumErrors );

    //  Variation 5  -  DBCS lead byte  0xfb
    rc = IsDBCSLeadByte(0xfb);
    CheckReturnIsDBCS( rc,
                       TRUE,
                       "DBCS 0xfb",
                       &NumErrors );

    //  Variation 6  -  DBCS lead byte  0xfc
    rc = IsDBCSLeadByte(0xfc);
    CheckReturnIsDBCS( rc,
                       TRUE,
                       "DBCS 0xfc",
                       &NumErrors );



    //
    //  Non DBCS Chars for Japanese - cp 932.
    //

    //  Variation 1  -  Non DBCS lead byte  0x80
    CheckReturnIsDBCS( rc,
                       FALSE,
                       "Non DBCS lead byte  0x80",
                       &NumErrors );

    //  Variation 2  -  Non DBCS lead byte  0xfd
    rc = IsDBCSLeadByte(0xfd);
    CheckReturnIsDBCS( rc,
                       FALSE,
                       "Non DBCS lead byte  0xfd",
                       &NumErrors );

    //  Variation 3  -  Non DBCS lead byte  0xa0
    rc = IsDBCSLeadByte(0xa0);
    CheckReturnIsDBCS( rc,
                       FALSE,
                       "Non DBCS lead byte  0xa0",
                       &NumErrors );

    //  Variation 4  -  Non DBCS lead byte  0xdf
    rc = IsDBCSLeadByte(0xdf);
    CheckReturnIsDBCS( rc,
                       FALSE,
                       "Non DBCS lead byte  0xdf",
                       &NumErrors );

#endif



    //--------------------//
    //  IsDBCSLeadByteEx  //
    //--------------------//


    //
    //  Different values for ch.
    //

    //  Variation 1  -  ch = 0x00
    ch = 0x00;
    rc = IsDBCSLeadByteEx(1252, ch);
    CheckReturnIsDBCS( rc,
                       FALSE,
                       "Ex ch = 0x00",
                       &NumErrors );

    //  Variation 2  -  ch = 0x23
    ch = 0x23;
    rc = IsDBCSLeadByteEx(1252, ch);
    CheckReturnIsDBCS( rc,
                       FALSE,
                       "Ex ch = 0x23",
                       &NumErrors );

    //  Variation 3  -  ch = 0xb3
    ch = 0xb3;
    rc = IsDBCSLeadByteEx(1252, ch);
    CheckReturnIsDBCS( rc,
                       FALSE,
                       "Ex ch = 0xb3",
                       &NumErrors );

    //  Variation 4  -  ch = 0xff
    ch = 0xff;
    rc = IsDBCSLeadByteEx(1252, ch);
    CheckReturnIsDBCS( rc,
                       FALSE,
                       "Ex ch = 0xff",
                       &NumErrors );



    //
    //  DBCS Chars for Japanese - cp 932.
    //

    //  Variation 1  -  DBCS lead byte  0x81
    rc = IsDBCSLeadByteEx(932, 0x81);
    CheckReturnIsDBCS( rc,
                       TRUE,
                       "Ex DBCS 0x81",
                       &NumErrors );

    //  Variation 2  -  DBCS lead byte  0x85
    rc = IsDBCSLeadByteEx(932, 0x85);
    CheckReturnIsDBCS( rc,
                       TRUE,
                       "Ex DBCS 0x85",
                       &NumErrors );

    //  Variation 3  -  DBCS lead byte  0x9f
    rc = IsDBCSLeadByteEx(932, 0x9f);
    CheckReturnIsDBCS( rc,
                       TRUE,
                       "Ex DBCS 0x9f",
                       &NumErrors );

    //  Variation 4  -  DBCS lead byte  0xe0
    rc = IsDBCSLeadByteEx(932, 0xe0);
    CheckReturnIsDBCS( rc,
                       TRUE,
                       "Ex DBCS 0xe0",
                       &NumErrors );

    //  Variation 5  -  DBCS lead byte  0xfb
    rc = IsDBCSLeadByteEx(932, 0xfb);
    CheckReturnIsDBCS( rc,
                       TRUE,
                       "Ex DBCS 0xfb",
                       &NumErrors );

    //  Variation 6  -  DBCS lead byte  0xfc
    rc = IsDBCSLeadByteEx(932, 0xfc);
    CheckReturnIsDBCS( rc,
                       TRUE,
                       "Ex DBCS 0xfc",
                       &NumErrors );



    //
    //  Non DBCS Chars for Japanese - cp 932.
    //

    //  Variation 1  -  Non DBCS lead byte  0x80
    rc = IsDBCSLeadByteEx(932, 0x80);
    CheckReturnIsDBCS( rc,
                       FALSE,
                       "Ex Non DBCS lead byte  0x80",
                       &NumErrors );

    //  Variation 2  -  Non DBCS lead byte  0xfd
    rc = IsDBCSLeadByteEx(932, 0xfd);
    CheckReturnIsDBCS( rc,
                       FALSE,
                       "Ex Non DBCS lead byte  0xfd",
                       &NumErrors );

    //  Variation 3  -  Non DBCS lead byte  0xa0
    rc = IsDBCSLeadByteEx(932, 0xa0);
    CheckReturnIsDBCS( rc,
                       FALSE,
                       "Ex Non DBCS lead byte  0xa0",
                       &NumErrors );

    //  Variation 4  -  Non DBCS lead byte  0xdf
    rc = IsDBCSLeadByteEx(932, 0xdf);
    CheckReturnIsDBCS( rc,
                       FALSE,
                       "Ex Non DBCS lead byte  0xdf",
                       &NumErrors );



    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  CheckReturnIsDBCS
//
//  Checks the return code from the IsDBCSLeadByte call.  It prints out
//  the appropriate error if the incorrect result is found.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

void CheckReturnIsDBCS(
    int CurrentReturn,
    int ExpectedReturn,
    LPSTR pErrString,
    int *pNumErrors)
{
    if ( (CurrentReturn != ExpectedReturn) ||
         ( (CurrentReturn == FALSE) &&
           (GetLastError() == ERROR_FILE_NOT_FOUND) ) )
    {
        printf("ERROR: %s - \n", pErrString);
        printf("  Return = %d, Expected = %d\n", CurrentReturn, ExpectedReturn);

        (*pNumErrors)++;
    }
}
