/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    nlstest.h

Abstract:

    This file contains the header information for the NLS test module.

    NOTE: This code was simply hacked together quickly in order to
          test the different code modules of the NLS component.
          This is NOT meant to be a formal regression test.

Revision History:

    06-14-91    JulieB    Created.

--*/




////////////////////////////////////////////////////////////////////////////
//
//  Includes Files.
//
////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <excpt.h>
#include <windows.h>
#include <winerror.h>
#include <winnlsp.h>

#include <stdio.h>
#include <string.h>




////////////////////////////////////////////////////////////////////////////
//
//  Constant Declarations.
//
////////////////////////////////////////////////////////////////////////////

//
//  Error codes.
//

#ifndef NO_ERROR

  #define NO_ERROR  0

#endif



//
//  Performance Break Points.
//  Uncomment this definition to enable the break points.
//
//#define PERF




////////////////////////////////////////////////////////////////////////////
//
//  Typedef Declarations.
//
////////////////////////////////////////////////////////////////////////////




////////////////////////////////////////////////////////////////////////////
//
//  Macro Definitions.
//
////////////////////////////////////////////////////////////////////////////

//
//  Macros for getting string lengths.
//
#define  MB_STRING_LEN(pStr)           ((int)(strlen(pStr)))
#define  MB_STRING_LEN_NULL(pStr)      ((int)(strlen(pStr) + 1))

#define  WC_STRING_LEN(pStr)           ((int)(wcslen(pStr)))
#define  WC_STRING_LEN_NULL(pStr)      ((int)(wcslen(pStr) + 1))




////////////////////////////////////////////////////////////////////////////
//
//  Function Prototypes
//
////////////////////////////////////////////////////////////////////////////

int
CompStringsW(
    WCHAR  *WCStr1,
    WCHAR  *WCStr2,
    int    size);

int
CompStringsA(
    BYTE   *MBStr1,
    BYTE   *MBStr2,
    int    size);

void
PrintWC(
    WCHAR  *WCStr,
    int    size);

void
PrintMB(
    BYTE   *MBStr,
    int    size);

void
CheckReturnBadParam(
    int    CurrentReturn,
    int    ExpectedReturn,
    DWORD  ExpectedLastError,
    LPSTR  pErrString,
    int    *pNumErrors);

void
CheckReturnBadParamEnum(
    int    CurrentReturn,
    int    ExpectedReturn,
    DWORD  ExpectedLastError,
    LPSTR  pErrString,
    int    *pNumErrors,
    int    CurrentEnumCtr,
    int    ExpectedEnumCtr);

void
CheckReturnEqual(
    int    CurrentReturn,
    int    NonExpectedReturn,
    LPSTR  pErrString,
    int    *pNumErrors);

void
CheckReturnValidEnumLoop(
    int    CurrentReturn,
    int    ExpectedReturn,
    int    CurrentCtr,
    int    ExpectedCtr,
    LPSTR  pErrString,
    DWORD  ItemValue,
    int    *pNumErrors);

void
CheckReturnValidEnum(
    int    CurrentReturn,
    int    ExpectedReturn,
    int    CurrentCtr,
    int    ExpectedCtr,
    LPSTR  pErrString,
    int    *pNumErrors);

void
CheckReturnValidLoopW(
    int    CurrentReturn,
    int    ExpectedReturn,
    LPWSTR pCurrentString,
    LPWSTR pExpectedString,
    LPSTR  pErrString,
    DWORD  ItemValue,
    int    *pNumErrors);

void
CheckReturnValidW(
    int    CurrentReturn,
    int    ExpectedReturn,
    LPWSTR pCurrentString,
    LPWSTR pExpectedString,
    LPSTR  pErrString,
    int    *pNumErrors);

void
CheckReturnValidA(
    int    CurrentReturn,
    int    ExpectedReturn,
    LPSTR  pCurrentString,
    LPSTR  pExpectedString,
    LPBOOL pUsedDef,
    LPSTR  pErrString,
    int    *pNumErrors);

void
CheckReturnValidInt(
    int    CurrentReturn,
    int    ExpectedReturn,
    DWORD  CurrentInt,
    DWORD  ExpectedInt,
    LPSTR  pErrString,
    int    *pNumErrors);


int
TestMBToWC(void);

int
TestWCToMB(void);

int
TestGetCPInfo(void);

int
TestCompareString(void);

int
TestGetStringType(void);

int
TestFoldString(void);

int
TestLCMapString(void);

int
TestGetLocaleInfo(void);

int
TestSetLocaleInfo(void);

int
TestGetCalendarInfo(void);

int
TestSetCalendarInfo(void);

int
TestIsDBCSLeadByte(void);

int
TestIsValidCodePage(void);

int
TestIsValidLanguageGroup(void);

int
TestIsValidLocale(void);

int
TestUtilityAPIs(void);

int
TestGetTimeFormat(void);

int
TestGetDateFormat(void);

int
TestGetNumberFormat(void);

int
TestGetCurrencyFormat(void);

int
TestEnumUILanguages(void);

int
TestEnumSystemLanguageGroups(void);

int
TestEnumLanguageGroupLocales(void);

int
TestEnumSystemLocales(void);

int
TestEnumSystemCodePages(void);

int
TestEnumCalendarInfo(void);

int
TestEnumTimeFormats(void);

int
TestEnumDateFormats(void);




////////////////////////////////////////////////////////////////////////////
//
//  Global Variables
//
//  All of the global variables for the NLSTEST should be put here.
//
//  Globals are included last because they may require some of the types
//  being defined above.
//
////////////////////////////////////////////////////////////////////////////

extern BOOL    Verbose;           // verbose flag
extern LCID    pAllLocales[];     // all supported locale ids
extern int     NumLocales;        // number of all supported locale ids
