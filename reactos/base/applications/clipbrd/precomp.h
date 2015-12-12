#ifndef _CLIPBRD_PCH_
#define _CLIPBRD_PCH_

// #pragma once

#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wingdi.h>
#include <shellapi.h>
#include <htmlhelp.h>
#include <commdlg.h>

#include "resources.h"
#include "cliputils.h"
#include "fileutils.h"
#include "winutils.h"
#include "scrollutils.h"

#define MAX_STRING_LEN 255
#define DISPLAY_MENU_POS 2
#define CF_NONE 0

typedef struct _CLIPBOARD_GLOBALS
{
    HINSTANCE hInstance;
    HWND hMainWnd;
    HWND hWndNext;
    HMENU hMenu;
    UINT uDisplayFormat;
    UINT uCheckedItem;
    HBITMAP hDspBmp;
} CLIPBOARD_GLOBALS;

extern CLIPBOARD_GLOBALS Globals;

#endif /* _CLIPBRD_PCH_ */
