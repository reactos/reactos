/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    cpitest.c

Abstract:

    Test module for NLS API GetCPInfo.

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
//  Constant Definitions.
//

#define CPI_UNICODE     1
#define CPI_ANSI        2




//
//  Forward Declarations.
//

int
CPI_BadParamCheck();

int
CPI_NormalCase();

BOOL
CheckInfoStruct(
    LPCPINFO pInfo,
    UINT MaxCharSize,
    DWORD fExVer);

BOOL
CheckDBCSInfoStruct(
    LPCPINFO pInfo,
    DWORD fExVer);

void
PrintInfoStruct(
    LPCPINFO pInfo,
    DWORD fExVer);

void
CheckReturnCPInfo(
    int CurrentReturn,
    LPCPINFO pCurrentInfo,
    BOOL fIfDBCSInfo,
    UINT MaxCharSize,
    LPSTR pErrString,
    DWORD fExVer,
    int *pNumErrors);





////////////////////////////////////////////////////////////////////////////
//
//  TestGetCPInfo
//
//  Test routine for GetCPInfo API.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int TestGetCPInfo()
{
    int ErrCount = 0;             // error count


    //
    //  Print out what's being done.
    //
    printf("\n\nTESTING GetCPInfo...\n\n");

    //
    //  Test bad parameters.
    //
    ErrCount += CPI_BadParamCheck();

    //
    //  Test normal cases.
    //
    ErrCount += CPI_NormalCase();

    //
    //  Print out result.
    //
    printf("\nGetCPInfo:  ERRORS = %d\n", ErrCount);

    //
    //  Return total number of errors found.
    //
    return (ErrCount);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPI_BadParamCheck
//
//  This routine passes in bad parameters to the API routine and checks to
//  be sure they are handled properly.  The number of errors encountered
//  is returned to the caller.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int CPI_BadParamCheck()
{
    int NumErrors = 0;            // error count - to be returned
    BOOL rc;                      // return code
    CPINFO Info;                  // CPINFO structure
    CPINFOEXW InfoEx;             // CPINFOEXW structure
    CPINFOEXA InfoExA;            // CPINFOEXA structure


    //
    //  Null Pointers.
    //

    //  Variation 1  -  lpCPInfo = NULL
    rc = GetCPInfo( 1252,
                    NULL );
    CheckReturnBadParam( rc,
                         FALSE,
                         ERROR_INVALID_PARAMETER,
                         "lpCPInfo NULL",
                         &NumErrors );


    //
    //  Invalid Code Page.
    //

    //  Variation 1  -  CodePage = invalid
    rc = GetCPInfo( 5,
                    &Info );
    CheckReturnBadParam( rc,
                         FALSE,
                         ERROR_INVALID_PARAMETER,
                         "CodePage Invalid",
                         &NumErrors );


    //
    //  EX Version - Null Pointers.
    //

    //  Variation 1  -  lpCPInfo = NULL
    rc = GetCPInfoExW( 1252,
                       0,
                       NULL );
    CheckReturnBadParam( rc,
                         FALSE,
                         ERROR_INVALID_PARAMETER,
                         "Ex lpCPInfo NULL",
                         &NumErrors );

    rc = GetCPInfoExA( 1252,
                       0,
                       NULL );
    CheckReturnBadParam( rc,
                         FALSE,
                         ERROR_INVALID_PARAMETER,
                         "A version Ex lpCPInfo NULL",
                         &NumErrors );


    //
    //  EX Version - Invalid Code Page.
    //

    //  Variation 1  -  CodePage = invalid
    rc = GetCPInfoExW( 5,
                       0,
                       &InfoEx );
    CheckReturnBadParam( rc,
                         FALSE,
                         ERROR_INVALID_PARAMETER,
                         "CodePage Invalid",
                         &NumErrors );

    rc = GetCPInfoExA( 5,
                       0,
                       &InfoExA );
    CheckReturnBadParam( rc,
                         FALSE,
                         ERROR_INVALID_PARAMETER,
                         "A version CodePage Invalid",
                         &NumErrors );


    //
    //  EX Version - Invalid Flags.
    //

    //  Variation 1  -  Flags = invalid
    rc = GetCPInfoExW( 1252,
                       1,
                       &InfoEx );
    CheckReturnBadParam( rc,
                         FALSE,
                         ERROR_INVALID_FLAGS,
                         "Flags Invalid",
                         &NumErrors );

    rc = GetCPInfoExA( 1252,
                       1,
                       &InfoExA );
    CheckReturnBadParam( rc,
                         FALSE,
                         ERROR_INVALID_FLAGS,
                         "A version Flags Invalid",
                         &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPI_NormalCase
//
//  This routine tests the normal cases of the API routine.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int CPI_NormalCase()
{
    int NumErrors = 0;            // error count - to be returned
    int rc;                       // return code
    CPINFO Info;                  // CPINFO structure
    CPINFOEXW InfoEx;             // CPINFOEXW structure
    CPINFOEXA InfoExA;            // CPINFOEXA structure


#ifdef PERF

  DbgBreakPoint();

#endif


    //
    //  CodePage defaults.
    //

    //  Variation 1  -  CodePage = CP_ACP
    rc = GetCPInfo( CP_ACP,
                    &Info );
    CheckReturnCPInfo( rc,
                       &Info,
                       FALSE,
                       1,
                       "CodePage CP_ACP",
                       0,
                       &NumErrors );

    //  Variation 2  -  CodePage = CP_OEMCP
    rc = GetCPInfo( CP_OEMCP,
                    &Info );
    CheckReturnCPInfo( rc,
                       &Info,
                       FALSE,
                       1,
                       "CodePage CP_OEMCP",
                       0,
                       &NumErrors );


    //
    //  CodePage 1252.
    //

    //  Variation 1  -  CodePage = 1252
    rc = GetCPInfo( 1252,
                    &Info );
    CheckReturnCPInfo( rc,
                       &Info,
                       FALSE,
                       1,
                       "CodePage 1252",
                       0,
                       &NumErrors );


    //
    //  CodePage 437.
    //

    //  Variation 1  -  CodePage = 437
    rc = GetCPInfo( 437,
                    &Info );
    CheckReturnCPInfo( rc,
                       &Info,
                       FALSE,
                       1,
                       "CodePage 437",
                       0,
                       &NumErrors );


    //
    //  CodePage 850.
    //

    //  Variation 1  -  CodePage = 850
    rc = GetCPInfo( 850,
                    &Info );
    CheckReturnCPInfo( rc,
                       &Info,
                       FALSE,
                       1,
                       "CodePage 850",
                       0,
                       &NumErrors );


    //
    //  CodePage 10000.
    //

    //  Variation 1  -  CodePage = 10000
    rc = GetCPInfo( 10000,
                    &Info );
    CheckReturnCPInfo( rc,
                       &Info,
                       FALSE,
                       1,
                       "CodePage 10000",
                       0,
                       &NumErrors );


    //
    //  CodePage 932.
    //

    //  Variation 1  -  CodePage = 932
    rc = GetCPInfo( 932,
                    &Info );
    CheckReturnCPInfo( rc,
                       &Info,
                       TRUE,
                       2,
                       "CodePage 932",
                       0,
                       &NumErrors );


    //
    //  CodePage UTF 7.
    //

    //  Variation 1  -  CodePage = UTF 7
    rc = GetCPInfo( CP_UTF7,
                    &Info );
    CheckReturnCPInfo( rc,
                       &Info,
                       FALSE,
                       5,
                       "CodePage UTF 7",
                       0,
                       &NumErrors );


    //
    //  CodePage UTF 8.
    //

    //  Variation 1  -  CodePage = UTF 8
    rc = GetCPInfo( CP_UTF8,
                    &Info );
    CheckReturnCPInfo( rc,
                       &Info,
                       FALSE,
                       4,
                       "CodePage UTF 8",
                       0,
                       &NumErrors );


// -------------------------------------------------------------

    //
    //  Ex CodePage defaults.
    //

    //  Variation 1  -  CodePage = CP_ACP
    rc = GetCPInfoExW( CP_ACP,
                       0,
                       &InfoEx );
    CheckReturnCPInfo( rc,
                       (LPCPINFO)&InfoEx,
                       FALSE,
                       1,
                       "CodePage CP_ACP",
                       CPI_UNICODE,
                       &NumErrors );

    rc = GetCPInfoExA( CP_ACP,
                       0,
                       &InfoExA );
    CheckReturnCPInfo( rc,
                       (LPCPINFO)&InfoExA,
                       FALSE,
                       1,
                       "A version CodePage CP_ACP",
                       CPI_ANSI,
                       &NumErrors );

    //  Variation 2  -  CodePage = CP_OEMCP
    rc = GetCPInfoExW( CP_OEMCP,
                       0,
                       &InfoEx );
    CheckReturnCPInfo( rc,
                       (LPCPINFO)&InfoEx,
                       FALSE,
                       1,
                       "CodePage CP_OEMCP",
                       CPI_UNICODE,
                       &NumErrors );

    rc = GetCPInfoExA( CP_OEMCP,
                       0,
                       &InfoExA );
    CheckReturnCPInfo( rc,
                       (LPCPINFO)&InfoExA,
                       FALSE,
                       1,
                       "A version CodePage CP_OEMCP",
                       CPI_ANSI,
                       &NumErrors );


    //
    //  CodePage 1252.
    //

    //  Variation 1  -  CodePage = 1252
    rc = GetCPInfoExW( 1252,
                       0,
                       &InfoEx );
    CheckReturnCPInfo( rc,
                       (LPCPINFO)&InfoEx,
                       FALSE,
                       1,
                       "CodePage 1252",
                       CPI_UNICODE,
                       &NumErrors );

    rc = GetCPInfoExA( 1252,
                       0,
                       &InfoExA );
    CheckReturnCPInfo( rc,
                       (LPCPINFO)&InfoExA,
                       FALSE,
                       1,
                       "A version CodePage 1252",
                       CPI_ANSI,
                       &NumErrors );


    //
    //  CodePage 437.
    //

    //  Variation 1  -  CodePage = 437
    rc = GetCPInfoExW( 437,
                       0,
                       &InfoEx );
    CheckReturnCPInfo( rc,
                       (LPCPINFO)&InfoEx,
                       FALSE,
                       1,
                       "CodePage 437",
                       CPI_UNICODE,
                       &NumErrors );

    rc = GetCPInfoExA( 437,
                       0,
                       &InfoExA );
    CheckReturnCPInfo( rc,
                       (LPCPINFO)&InfoExA,
                       FALSE,
                       1,
                       "A version CodePage 437",
                       CPI_ANSI,
                       &NumErrors );


    //
    //  CodePage 850.
    //

    //  Variation 1  -  CodePage = 850
    rc = GetCPInfoExW( 850,
                       0,
                       &InfoEx );
    CheckReturnCPInfo( rc,
                       (LPCPINFO)&InfoEx,
                       FALSE,
                       1,
                       "CodePage 850",
                       CPI_UNICODE,
                       &NumErrors );

    rc = GetCPInfoExA( 850,
                       0,
                       &InfoExA );
    CheckReturnCPInfo( rc,
                       (LPCPINFO)&InfoExA,
                       FALSE,
                       1,
                       "A version CodePage 850",
                       CPI_ANSI,
                       &NumErrors );


    //
    //  CodePage 10000.
    //

    //  Variation 1  -  CodePage = 10000
    rc = GetCPInfoExW( 10000,
                       0,
                       &InfoEx );
    CheckReturnCPInfo( rc,
                       (LPCPINFO)&InfoEx,
                       FALSE,
                       1,
                       "CodePage 10000",
                       CPI_UNICODE,
                       &NumErrors );

    rc = GetCPInfoExA( 10000,
                       0,
                       &InfoExA );
    CheckReturnCPInfo( rc,
                       (LPCPINFO)&InfoExA,
                       FALSE,
                       1,
                       "A version CodePage 10000",
                       CPI_ANSI,
                       &NumErrors );


    //
    //  CodePage 932.
    //

    //  Variation 1  -  CodePage = 932
    rc = GetCPInfoExW( 932,
                       0,
                       &InfoEx );
    CheckReturnCPInfo( rc,
                       (LPCPINFO)&InfoEx,
                       TRUE,
                       2,
                       "CodePage 932",
                       CPI_UNICODE,
                       &NumErrors );

    rc = GetCPInfoExA( 932,
                       0,
                       &InfoExA );
    CheckReturnCPInfo( rc,
                       (LPCPINFO)&InfoExA,
                       TRUE,
                       2,
                       "A version CodePage 932",
                       CPI_ANSI,
                       &NumErrors );


    //
    //  CodePage UTF 7.
    //

    //  Variation 1  -  CodePage = UTF 7
    rc = GetCPInfoExW( CP_UTF7,
                       0,
                       &InfoEx );
    CheckReturnCPInfo( rc,
                       (LPCPINFO)&InfoEx,
                       FALSE,
                       5,
                       "CodePage UTF 7",
                       CPI_UNICODE,
                       &NumErrors );

    rc = GetCPInfoExA( CP_UTF7,
                       0,
                       &InfoExA );
    CheckReturnCPInfo( rc,
                       (LPCPINFO)&InfoExA,
                       FALSE,
                       5,
                       "A version CodePage UTF 7",
                       CPI_ANSI,
                       &NumErrors );


    //
    //  CodePage UTF 8.
    //

    //  Variation 1  -  CodePage = UTF 8
    rc = GetCPInfoExW( CP_UTF8,
                       0,
                       &InfoEx );
    CheckReturnCPInfo( rc,
                       (LPCPINFO)&InfoEx,
                       FALSE,
                       4,
                       "CodePage UTF 8",
                       CPI_UNICODE,
                       &NumErrors );

    rc = GetCPInfoExA( CP_UTF8,
                       0,
                       &InfoExA );
    CheckReturnCPInfo( rc,
                       (LPCPINFO)&InfoExA,
                       FALSE,
                       4,
                       "A version CodePage UTF 8",
                       CPI_ANSI,
                       &NumErrors );


    //
    //  Return total number of errors found.
    //
    return (NumErrors);
}


////////////////////////////////////////////////////////////////////////////
//
//  CheckInfoStruct
//
//  This routine checks the CPINFO structure to be sure the values are
//  consistent with code page 1252.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL CheckInfoStruct(
    LPCPINFO pInfo,
    UINT MaxCharSize,
    DWORD fExVer)
{
    int ctr;                      // loop counter


    //
    //  Check MaxCharSize field.
    //
    if (pInfo->MaxCharSize != MaxCharSize)
    {
        printf("ERROR: MaxCharSize = %x\n", pInfo->MaxCharSize);
        return (FALSE);
    }

    //
    //  Check DefaultChar field.
    //
    if (((pInfo->DefaultChar)[0] != (BYTE)0x3f) &&
        ((pInfo->DefaultChar)[1] != (BYTE)0))
    {
        printf("ERROR: DefaultChar = '%s'\n", pInfo->DefaultChar);
        return (FALSE);
    }

    //
    //  Check LeadByte field.
    //
    for (ctr = 0; ctr < MAX_LEADBYTES; ctr++)
    {
        if (pInfo->LeadByte[ctr] != 0)
        {
            printf("ERROR: LeadByte not 0 - ctr = %x\n", ctr);
            return (FALSE);
        }
    }

    //
    //  See if Ex version.
    //
    if (fExVer)
    {
        if (fExVer == CPI_ANSI)
        {
            LPCPINFOEXA pInfoA = (LPCPINFOEXA)pInfo;

            //
            //  Check UnicodeDefaultChar field.
            //
            if (pInfoA->UnicodeDefaultChar != L'?')
            {
                printf("ERROR: UnicodeDefaultChar = '%x'\n", pInfoA->UnicodeDefaultChar);
                return (FALSE);
            }
        }
        else if (fExVer == CPI_UNICODE)
        {
            LPCPINFOEXW pInfoW = (LPCPINFOEXW)pInfo;

            //
            //  Check UnicodeDefaultChar field.
            //
            if (pInfoW->UnicodeDefaultChar != L'?')
            {
                printf("ERROR: UnicodeDefaultChar = '%x'\n", pInfoW->UnicodeDefaultChar);
                return (FALSE);
            }
        }
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CheckDBCSInfoStruct
//
//  This routine checks the CPINFO structure to be sure the values are
//  consistent with code page 932.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL CheckDBCSInfoStruct(
    LPCPINFO pInfo,
    DWORD fExVer)
{
    int ctr;                      // loop counter


    //
    //  Check MaxCharSize field.
    //
    if (pInfo->MaxCharSize != 2)
    {
        printf("ERROR: MaxCharSize = %x\n", pInfo->MaxCharSize);
        return (FALSE);
    }

    //
    //  Check DefaultChar field.
    //
    if ((pInfo->DefaultChar)[0] != (BYTE)0x3f)
    {
        printf("ERROR: DefaultChar = '%s'\n", pInfo->DefaultChar);
        return (FALSE);
    }

    //
    //  Check LeadByte field.
    //
    if ( ((pInfo->LeadByte)[0] != 0x81) ||
         ((pInfo->LeadByte)[1] != 0x9f) ||
         ((pInfo->LeadByte)[2] != 0xe0) ||
         ((pInfo->LeadByte)[3] != 0xfc) )
    {
        printf("ERROR: LeadByte not correct\n");
        return (FALSE);
    }
    for (ctr = 4; ctr < MAX_LEADBYTES; ctr++)
    {
        if (pInfo->LeadByte[ctr] != 0)
        {
            printf("ERROR: LeadByte not 0 - ctr = %x\n", ctr);
            return (FALSE);
        }
    }

    //
    //  See if Ex version.
    //
    if (fExVer)
    {
        if (fExVer == CPI_ANSI)
        {
            LPCPINFOEXA pInfoA = (LPCPINFOEXA)pInfo;

            //
            //  Check UnicodeDefaultChar field.
            //
            if (pInfoA->UnicodeDefaultChar != 0x30fb)
            {
                printf("ERROR: UnicodeDefaultChar = '%x'\n", pInfoA->UnicodeDefaultChar);
                return (FALSE);
            }
        }
        else if (fExVer == CPI_UNICODE)
        {
            LPCPINFOEXW pInfoW = (LPCPINFOEXW)pInfo;

            //
            //  Check UnicodeDefaultChar field.
            //
            if (pInfoW->UnicodeDefaultChar != 0x30fb)
            {
                printf("ERROR: UnicodeDefaultChar = '%x'\n", pInfoW->UnicodeDefaultChar);
                return (FALSE);
            }
        }
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintInfoStruct
//
//  This routine prints out the CPINFO structure.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

void PrintInfoStruct(
    LPCPINFO pInfo,
    DWORD fExVer)
{
    int ctr;                      // loop counter


    //
    //  Print out MaxCharSize field.
    //
    printf("         MaxCharSize = %x\n",     pInfo->MaxCharSize);

    //
    //  Print out DefaultChar field.
    //
    printf("         DefaultChar = %x  %x\n",
            (pInfo->DefaultChar)[0], (pInfo->DefaultChar)[1] );

    //
    //  Print out LeadByte field.
    //
    for (ctr = 0; ctr < MAX_LEADBYTES; ctr += 2)
    {
        printf("         LeadByte    = %x  %x\n",
                pInfo->LeadByte[ctr], pInfo->LeadByte[ctr + 1]);
    }

    //
    //  See if we have the Ex version.
    //
    if (fExVer)
    {
        if (fExVer == CPI_ANSI)
        {
            LPCPINFOEXA pInfoA = (LPCPINFOEXA)pInfo;

            printf("         UnicodeDefaultChar = %x\n", pInfoA->UnicodeDefaultChar);
            printf("         CodePage = %d\n", pInfoA->CodePage);
            printf("         CodePageName = %s\n", pInfoA->CodePageName);
        }
        else if (fExVer == CPI_UNICODE)
        {
            LPCPINFOEXW pInfoW = (LPCPINFOEXW)pInfo;

            printf("         UnicodeDefaultChar = %x\n", pInfoW->UnicodeDefaultChar);
            printf("         CodePage = %d\n", pInfoW->CodePage);
            printf("         CodePageName = %ws\n", pInfoW->CodePageName);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CheckReturnCPInfo
//
//  Checks the return code from the GetCPInfo call.  It prints out
//  the appropriate error if the incorrect result is found.
//
//  06-14-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

void CheckReturnCPInfo(
    int CurrentReturn,
    LPCPINFO pCurrentInfo,
    BOOL fIfDBCSInfo,
    UINT MaxCharSize,
    LPSTR pErrString,
    DWORD fExVer,
    int *pNumErrors)
{
    if ( (CurrentReturn == FALSE) ||
         ( (fIfDBCSInfo == FALSE)
           ? (!CheckInfoStruct(pCurrentInfo, MaxCharSize, fExVer))
           : (!CheckDBCSInfoStruct(pCurrentInfo, fExVer)) ) )
    {
        printf("ERROR: %s - \n", pErrString);
        printf("  Return = %d, Expected = 0\n", CurrentReturn);
        printf("  LastError = %d, Expected = 0\n", GetLastError());

        PrintInfoStruct(pCurrentInfo, fExVer);

        (*pNumErrors)++;
    }
    else if (Verbose)
    {
        PrintInfoStruct(pCurrentInfo, fExVer);
    }
}
