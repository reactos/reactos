/*
 *  Notepad (notepad.h)
 *
 *  Copyright 1997,98 Marcel Baur <mbaur@g26.ethz.ch>
 *  Copyright 2002 Sylvain Petreolle <spetreolle@yahoo.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#define SIZEOF(a) (sizeof(a)/sizeof((a)[0]))

#include "notepad_res.h"

#define EDIT_STYLE_WRAP (WS_CHILD | WS_VSCROLL \
    | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL)
#define EDIT_STYLE (EDIT_STYLE_WRAP | WS_HSCROLL | ES_AUTOHSCROLL)

#define EDIT_CLASS          _T("EDIT")

#define MAX_STRING_LEN      255

#define ENCODING_ANSI       0
#define ENCODING_UNICODE    1
#define ENCODING_UNICODE_BE 2
#define ENCODING_UTF8       3

#define EOLN_CRLF           0
#define EOLN_LF             1
#define EOLN_CR             2

typedef struct
{
    HINSTANCE hInstance;
    HWND hMainWnd;
    HWND hFindReplaceDlg;
    HWND hEdit;
    HWND hStatusBar;
    HFONT hFont; /* Font used by the edit control */
    HMENU hMenu;
    LOGFONT lfFont;
    BOOL bWrapLongLines;
    BOOL bShowStatusBar;
    TCHAR szFindText[MAX_PATH];
    TCHAR szReplaceText[MAX_PATH];
    TCHAR szFileName[MAX_PATH];
    TCHAR szFileTitle[MAX_PATH];
    TCHAR szFilter[2 * MAX_STRING_LEN + 100];
    TCHAR szMarginTop[MAX_PATH];
    TCHAR szMarginBottom[MAX_PATH];
    TCHAR szMarginLeft[MAX_PATH];
    TCHAR szMarginRight[MAX_PATH];
    TCHAR szHeader[MAX_PATH];
    TCHAR szFooter[MAX_PATH];
    TCHAR szStatusBarLineCol[MAX_PATH];
    int iEncoding;
    int iEoln;

    FINDREPLACE find;
    WNDPROC EditProc;
    RECT main_rect;
} NOTEPAD_GLOBALS;

extern NOTEPAD_GLOBALS Globals;

VOID SetFileName(LPCTSTR szFileName);

/* from text.c */
BOOL ReadText(HANDLE hFile, LPWSTR *ppszText, DWORD *pdwTextLen, int *piEncoding, int *piEoln);
BOOL WriteText(HANDLE hFile, LPCWSTR pszText, DWORD dwTextLen, int iEncoding, int iEoln);

/* from settings.c */
void LoadSettings(void);
void SaveSettings(void);

/* from main.c */
BOOL NOTEPAD_FindNext(FINDREPLACE *, BOOL , BOOL );
VOID NOTEPAD_EnableSearchMenu(VOID);
