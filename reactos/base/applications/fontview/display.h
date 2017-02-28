#pragma once

/* Messages for the display class */
#define FVM_SETTYPEFACE WM_USER
#define FVM_SETSTRING (WM_USER + 1)
#define FVM_GETFULLNAME (WM_USER + 2)

/* Size restrictions */
#define MAX_STRING 100
#define MAX_FORMAT 20
#define MAX_SIZES 8

extern const WCHAR g_szFontDisplayClassName[];

/* Public function */
BOOL Display_InitClass(HINSTANCE hInstance);
LRESULT Display_OnPrint(HWND hwnd);
