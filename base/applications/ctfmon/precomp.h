/*
 * PROJECT:     ReactOS CTF Monitor
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Providing Language Bar front-end
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#define WIN32_NO_STATUS
#include <windows.h>
#include <ndk/pstypes.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <stdlib.h>
#include <strsafe.h>
#include <msctf.h>
#include <ctfutb.h>
#include <ctffunc.h>

#include <cicero/cicbase.h>
#include <cicero/osinfo.h>
#include <cicero/CModulePath.h>

#include "resource.h"

extern HINSTANCE g_hInst;
extern BOOL g_bOnWow64;
extern BOOL g_fWinLogon;
extern DWORD g_dwOsInfo;

VOID UninitApp(VOID);

typedef enum WATCH_INDEX
{
    WI_TOGGLE            = 0,
    WI_MACHINE_TIF       = 1,
    WI_PRELOAD           = 2,
    WI_RUN               = 3,
    WI_USER_TIF          = 4,
    WI_USER_SPEECH       = 5,
    WI_APPEARANCE        = 6,
    WI_COLORS            = 7,
    WI_WINDOW_METRICS    = 8,
    WI_MACHINE_SPEECH    = 9,
    WI_KEYBOARD_LAYOUT   = 10,
    WI_ASSEMBLIES        = 11,
    WI_DESKTOP_SWITCH    = 12,
} WATCH_INDEX;
