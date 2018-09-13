//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       global.hxx
//
//  Contents:   Microsoft Internet Security Trust Provider
//
//  History:    28-May-1997 pberkman   created
//
//--------------------------------------------------------------------------

#define STRICT
#define NO_ANSIUNI_ONLY

#include    <windows.h>
#include    <assert.h>
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

#include    "wincrypt.h"
#include    "dbgdef.h"
#include    "unicode.h"
#include    "crtem.h"
#include    "crypttls.h"
#include    "crypthlp.h"
#include    "wincrypt.h"
#include    "sipbase.h"
#include    "mssip.h"
#include    "mscat.h"
#include    "wintrust.h"
#include    "wintrustp.h"

#include    "gendefs.h"

#include    "eventlst.h"

#include    "provload.h"
#include    "storprov.h"

#include    "locals.h"

#include    "catcache.h"

#define     DBG_SS              DBG_SS_TRUST

#pragma hdrstop

