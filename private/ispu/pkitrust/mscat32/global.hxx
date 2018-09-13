//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       global.hxx
//
//  History:    17-Feb-97   pberkman    created
//
//--------------------------------------------------------------------------

#define STRICT
#define NO_ANSIUNI_ONLY

#include    <windows.h>
#include    <assert.h>
#include    <ole2.h>
#include    <regstr.h>
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
#include    <prsht.h>
#include    <commctrl.h>
#include    <wininet.h>

#include    "crtem.h"
#include    "wincrypt.h"
#include    "dbgdef.h"
#include    "wintrust.h"
#include    "unicode.h"
#include    "crypttls.h"
#include    "crypthlp.h"
#include    "cryptreg.h"
#include    "wincrypt.h"
#include    "sipbase.h"
#include    "mssip.h"
#include    "mssip32.h"
#include    "gendefs.h"
#include    "stack.hxx"

#include    "wintrust.h"

#include    "mscat.h"

#include    "dbcomp.h"

extern CRITICAL_SECTION    MSCAT_CriticalSection;

#define DBG_SS              DBG_SS_CATALOG

#pragma hdrstop
