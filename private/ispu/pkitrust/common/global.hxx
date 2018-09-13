//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       global.hxx
//
//  Contents:   Top level internal header file for crypt32 APIs. This file
//              includes all base header files and contains other global
//              stuff.
//
//  History:    31-Mar-1997     pberkman        created
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <assert.h>
#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "wincrypt.h"
#include "wintrust.h"
#include "wintrustp.h"
#include "unicode.h"
#include "crtem.h"
#include "dbgdef.h"

#include "gendefs.h"

#define DBG_SS          DBG_SS_TRUSTCOMMON

#pragma hdrstop
