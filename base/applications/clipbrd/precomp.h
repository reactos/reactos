/*
 * PROJECT:     ReactOS Clipboard Viewer
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Precompiled header.
 * COPYRIGHT:   Copyright 2015-2018 Ricardo Hanke
 */

#ifndef _CLIPBRD_PCH_
#define _CLIPBRD_PCH_

// #pragma once

#undef _WIN32_WINNT
#define _WIN32_WINNT    0x600

#include <limits.h>

#include <assert.h>

#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wingdi.h>
#include <shellapi.h>
#include <htmlhelp.h>
#include <commdlg.h>
#include <winnls.h>
#include <stdlib.h>

#include "resources.h"
#include "cliputils.h"
#include "fileutils.h"
#include "scrollutils.h"
#include "winutils.h"

#define MAX_STRING_LEN 255
#define DISPLAY_MENU_POS 2

#define CF_NONE 0

typedef struct _CLIPBOARD_GLOBALS
{
    HINSTANCE hInstance;
    HWND hMainWnd;
    HWND hWndNext;
    HWND hwndText;
    HMENU hMenu;
    BOOL bTextMode;
    BOOL bTextAlloced;
    UINT uDisplayFormat;
    UINT uCheckedItem;

    /* Registered clipboard formats */
    UINT uCFSTR_FILENAMEA;
    UINT uCFSTR_FILENAMEW;
    UINT uCF_HTML;
    UINT uCFSTR_INETURLA;
    UINT uCFSTR_INETURLW;
} CLIPBOARD_GLOBALS;

extern CLIPBOARD_GLOBALS Globals;

HWND ReCreateTextBox(HWND hwndParent, BOOL bUnicode, BOOL bUseHandle, BOOL bTextAlloced, LPVOID pvText);
void DestroyTextBox(void);

#endif /* _CLIPBRD_PCH_ */
