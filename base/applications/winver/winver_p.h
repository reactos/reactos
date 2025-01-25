/*
 * PROJECT:    ReactOS Version Program
 * LICENSE:    LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:    Main header
 * COPYRIGHT:  Copyright 2025 Thamatip Chitpong <thamatip.chitpong@reactos.org>
 */

#pragma once

#include <stdarg.h>
#include <stdlib.h>

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winuser.h>
#include <commctrl.h>
#include <shellapi.h>

#include <strsafe.h>

#include "resource.h"

typedef struct _WINVER_OS_INFO
{
    WCHAR szName[64];
    WCHAR szNtVersion[16];
    WCHAR szNtBuild[16];
    WCHAR szNtSpk[64];
    WCHAR szCompatInfo[256];
} WINVER_OS_INFO, *PWINVER_OS_INFO;

extern HINSTANCE Winver_hInstance;

BOOL
Winver_GetOSInfo(
    _Out_ PWINVER_OS_INFO OSInfo);
