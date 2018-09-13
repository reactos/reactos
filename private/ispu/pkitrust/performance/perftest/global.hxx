//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       global.hxx
//
//  Contents:   WinVerifyTrust Stress
//
//  History:    12-Aug-1997 pberkman   created
//
//--------------------------------------------------------------------------

#define STRICT
#define NO_ANSIUNI_ONLY

#include    <afxdisp.h>
#include    <wincrypt.h>
#include    <string.h>
#include    <malloc.h>
#include    <memory.h>
#include    <stdlib.h>
#include    <stddef.h>
#include    <stdio.h>
#include    <wchar.h>
#include    <tchar.h>
#include    <time.h>
#include    <shellapi.h>
#include    <dbgdef.h>
#include    <unicode.h>

#include    <wintrust.h>
#include    <softpub.h>
#include    <mscat.h>

#include    "gendefs.h"
#include    "cwargv.hxx"
#include    "stack.hxx"
#include    "fparse.hxx"

#include    "resource.h"

typedef struct ThreadData_
{
    HANDLE              hThread;
    DWORD               dwId;
    DWORD               dwRetCode;
    DWORD               dwTotalProcessed;
    COleDateTimeSpan    tsTotal;

    DWORD               dwPassThrough;

} ThreadData;

#define     PASSTHROUGH_SHA1                0x00010000


typedef DWORD           (*PFN_TEST)(ThreadData *psData);


typedef struct WVTLOOPDATA_
{
    WCHAR       *pwszFileName;
    GUID        *pgProvider;

    WCHAR       *pwszCatalogFile;
    WCHAR       *pwszTag;

    DWORD       dwStateControl;

} WVTLOOPDATA;

extern DWORD WINAPI TestWVTCat(ThreadData *psData);
extern DWORD WINAPI TestWVTCert(ThreadData *psData);
extern DWORD WINAPI TestWVTFile(ThreadData *psData);
extern DWORD WINAPI TestCatAdd(ThreadData *psData);
extern DWORD WINAPI TestCryptHash(ThreadData *psData);

extern GUID gAuthCode;
extern GUID gDriver;
extern GUID gCertProvider;

extern BOOL     fVerbose;
extern DWORD    cPasses;
extern WCHAR    *pwszInFile;

#pragma hdrstop
