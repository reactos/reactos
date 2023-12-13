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
#include <cicero/OSInfo.h>
#include <cicero/CModulePath.h>

#include "resource.h"

extern HINSTANCE g_hInst;
extern BOOL g_bOnWow64;
extern BOOL g_fWinLogon;
extern DWORD g_dwOsInfo;

VOID UninitApp(VOID);

typedef enum ENTRY_INDEX
{
    EI_TOGGLE            = 0,
    EI_MACHINE_TIF       = 1,
    EI_PRELOAD           = 2,
    EI_RUN               = 3,
    EI_USER_TIF          = 4,
    EI_USER_SPEECH       = 5,
    EI_APPEARANCE        = 6,
    EI_COLORS            = 7,
    EI_WINDOW_METRICS    = 8,
    EI_MACHINE_SPEECH    = 9,
    EI_KEYBOARD_LAYOUT   = 10,
    EI_ASSEMBLIES        = 11,
    EI_DESKTOP_SWITCH    = 12,
} ENTRY_INDEX;
