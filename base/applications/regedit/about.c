/*
 * Regedit About Dialog Box
 *
 * Copyright (C) 2002 Robert Dickenson <robd@reactos.org>
 * LICENSE: LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 */

#include "regedit.h"

#include <shellapi.h>

void ShowAboutBox(HWND hWnd)
{
    WCHAR AppStr[255];
    LoadStringW(hInst, IDS_APP_TITLE, AppStr, ARRAY_SIZE(AppStr));
    ShellAboutW(hWnd, AppStr, NULL, LoadIconW(hInst, MAKEINTRESOURCEW(IDI_REGEDIT)));
}

/* EOF */
