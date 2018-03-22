/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Precompiled Header for all source files
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck (colin@reactos.org)
 */

#ifndef _PRECOMP_H
#define _PRECOMP_H

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winsvc.h>
#include <wingdi.h>
#include <winspool.h>
#include <winsplp.h>
#include <winspool_s.h>
#include <ndk/rtlfuncs.h>

#include <spoolss.h>
#include <marshalling/marshalling.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(spoolsv);

// rpcserver.c
DWORD WINAPI LrpcThreadProc(LPVOID lpParameter);

#endif
