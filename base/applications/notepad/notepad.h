/*
 * PROJECT:    ReactOS Notepad
 * LICENSE:    LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:    Providing a Windows-compatible simple text editor for ReactOS
 * COPYRIGHT:  Copyright 1997,98 Marcel Baur <mbaur@g26.ethz.ch>
 *             Copyright 2002 Sylvain Petreolle <spetreolle@yahoo.fr>
 *             Copyright 2002 Andriy Palamarchuk
 *             Copyright 2000 Mike McCormack <Mike_McCormack@looksmart.com.au>
 *             Copyright 2020-2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#ifndef STRSAFE_NO_DEPRECATE
    #define STRSAFE_NO_DEPRECATE
#endif

#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winnls.h>
#include <wingdi.h>
#include <shellapi.h>
#include <commdlg.h>
#include <tchar.h>
#include <stdlib.h>
#include <malloc.h>

#include "dialog.h"
#include "notepad_res.h"

#define EDIT_STYLE_WRAP (WS_CHILD | WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL)
#define EDIT_STYLE      (EDIT_STYLE_WRAP | WS_HSCROLL | ES_AUTOHSCROLL)
#define EDIT_CLASS      _T("EDIT")

#define MAX_STRING_LEN  255

/* Values are indexes of the items in the Encoding combobox. */
typedef enum
{
    ENCODING_AUTO    = -1,
    ENCODING_ANSI    =  0,
    ENCODING_UTF16LE =  1,
    ENCODING_UTF16BE =  2,
    ENCODING_UTF8    =  3,
    ENCODING_UTF8BOM =  4
} ENCODING;

#define ENCODING_DEFAULT    ENCODING_UTF8 // ENCODING_ANSI

typedef enum
{
    EOLN_CRLF = 0, /* "\r\n" */
    EOLN_LF   = 1, /* "\n" */
    EOLN_CR   = 2  /* "\r" */
} EOLN; /* End of line (NewLine) type */

typedef struct
{
    HINSTANCE hInstance;
    HWND hMainWnd;
    HWND hFindReplaceDlg;
    HWND hEdit;
    HWND hStatusBar;
    HFONT hFont; /* Font used by the edit control */
    HMENU hMenu;
    HGLOBAL hDevMode;
    HGLOBAL hDevNames;
    LOGFONT lfFont;
    BOOL bWrapLongLines;
    BOOL bShowStatusBar;
    TCHAR szFindText[MAX_PATH];
    TCHAR szReplaceText[MAX_PATH];
    TCHAR szFileName[MAX_PATH];
    TCHAR szFileTitle[MAX_PATH];
    TCHAR szFilter[512];
    RECT lMargins; /* The margin values in 100th millimeters */
    TCHAR szHeader[MAX_PATH];
    TCHAR szFooter[MAX_PATH];
    TCHAR szStatusBarLineCol[MAX_PATH];

    ENCODING encFile;
    EOLN iEoln;

    FINDREPLACE find;
    WNDPROC EditProc;
    BOOL bWasModified;
} NOTEPAD_GLOBALS;

extern NOTEPAD_GLOBALS Globals;

BOOL ReadText(HANDLE hFile, HLOCAL *phLocal, ENCODING *pencFile, EOLN *piEoln);
BOOL WriteText(HANDLE hFile, LPCWSTR pszText, DWORD dwTextLen, ENCODING encFile, EOLN iEoln);

void NOTEPAD_LoadSettingsFromRegistry(PWINDOWPLACEMENT pWP);
void NOTEPAD_SaveSettingsToRegistry(void);

BOOL NOTEPAD_FindNext(FINDREPLACE *pFindReplace, BOOL bReplace, BOOL bShowAlert);
VOID NOTEPAD_EnableSearchMenu(VOID);
VOID SetFileName(LPCTSTR szFileName);
