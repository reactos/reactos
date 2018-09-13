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
#include    <imagehlp.h>
#include    <prsht.h>
#include    <commctrl.h>
#include    <wininet.h>
#include    <dbgdef.h>

#include    "cryptreg.h"
#include    "wintrust.h"
#include    "wintrustp.h"
#include    "softpub.h"
#include    "unicode.h"

#include    "crtem.h"
#include    "sipbase.h"
#include    "mssip.h"
#include    "mscat.h"

#include    "mssip32.h"
#include    "sipguids.h"

#include    "gendefs.h"

#define     DBG_SS                          DBG_SS_SIP

#pragma hdrstop
