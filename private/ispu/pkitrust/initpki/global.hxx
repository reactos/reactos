//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       global.hxx
//
//  Contents:   Microsoft Internet Security Policy Provider
//
//  History:    05-Jun-1997 pberkman   created
//
//--------------------------------------------------------------------------

#define STRICT
#define NO_ANSIUNI_ONLY


#define STRICT

#include    <windows.h>
#include    <assert.h>
#include    <ole2.h>
#include    <regstr.h>
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
#include    <prsht.h>
#include    <commctrl.h>
#include    <wininet.h>
#include    <dbgdef.h>

#include    "unicode.h"
#include    "crtem.h"
#include    "crypttls.h"
#include    "pkialloc.h"
#include    "certprot.h"
#include    "sipbase.h"
#include    "mssip.h"
#include    "mscat.h"
#include    "wintrust.h"
#include    "softpub.h"
#include    "spcmem.h"

#include    "gendefs.h"

#include    "resource.h"
#include    "locals.h"
#include    "tcrack.h"

#define DBG_SS      DBG_SS_TRUSTCOMMON

#pragma hdrstop
