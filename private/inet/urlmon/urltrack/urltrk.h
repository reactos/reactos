//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       urltrack.h
//
//  Contents:   precompiled header file for the urltrack directory
//
//  Classes:
//
//  Functions:
//
//  History:    07-25-97   PeihwaL   Created
//
//----------------------------------------------------------------------------

#include <windows.h>
#include <windowsx.h>
#include <ole2.h>               // to get IStream for image.c
#define DISALLOW_Assert
#include <debug.h>
#include <winerror.h>
#include <winnlsp.h>
#include <wininet.h>
#include <winineti.h>
#include <urlmon.h>

typedef struct _MY_LOGGING_INFO {
   LPHIT_LOGGING_INFO   pLogInfo;
   BOOL                 fuseCache;
   BOOL                 fOffLine;
} MY_LOGGING_INFO, * LPMY_LOGGING_INFO;


ULONG _IsLoggingEnabled(LPCSTR  pszUrl);
BOOL  _WriteHitLogging(LPMY_LOGGING_INFO pLogInfo);

BOOL   IsGlobalOffline(void);
