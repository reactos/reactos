/*
 * PROJECT:     ReactOS Console IME
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing Console IME Input for Far-East Asian
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#define WM_USER_INIT (WM_USER + 0x00) // 0x400
#define WM_USER_UNINIT (WM_USER + 0x01) // 0x401
#define WM_USER_SWITCHIME (WM_USER + 0x02) // 0x402
#define WM_USER_DEACTIVATE (WM_USER + 0x03) // 0x403
#define WM_USER_SIMHOTKEY (WM_USER + 0x04) // 0x404
#define WM_USER_GETIMESTATE (WM_USER + 0x05) // 0x405
#define WM_USER_SETIMESTATE (WM_USER + 0x06) // 0x406
#define WM_USER_SETSCREENSIZE (WM_USER + 0x07) // 0x407
#define WM_USER_SENDIMESTATUS (WM_USER + 0x08) // 0x408
#define WM_USER_CHANGEKEYBOARD (WM_USER + 0x09) // 0x409
#define WM_USER_SETCODEPAGE (WM_USER + 0x0A) // 0x40A
#define WM_USER_GO (WM_USER + 0x0B) // 0x40B
#define WM_USER_GONEXT (WM_USER + 0x0C) // 0x40C
#define WM_USER_GOBACK (WM_USER + 0x0D) // 0x40D

#define WM_ROUTE 0x800
#define WM_ROUTE_KEYDOWN (WM_KEYDOWN + WM_ROUTE) // 0x900
#define WM_ROUTE_KEYUP (WM_KEYUP + WM_ROUTE) // 0x901
#define WM_ROUTE_CHAR (WM_CHAR + WM_ROUTE) // 0x902
#define WM_ROUTE_DEADCHAR (WM_DEADCHAR + WM_ROUTE) // 0x903
#define WM_ROUTE_SYSKEYDOWN (WM_SYSKEYDOWN + WM_ROUTE) // 0x904
#define WM_ROUTE_SYSKEYUP (WM_SYSKEYUP + WM_ROUTE) // 0x905
#define WM_ROUTE_SYSCHAR (WM_SYSCHAR + WM_ROUTE) // 0x906
#define WM_ROUTE_SYSDEADCHAR (WM_SYSDEADCHAR + WM_ROUTE) // 0x907

#define _GCS_SINGLECHAR 0x2000

#define IMEDISPLAY_MAX_X 160

// IME display-related
typedef struct tagIMEDISPLAY
{
    UINT uCharInfoLen;
    BOOL bFlag;
    CHAR_INFO CharInfo[IMEDISPLAY_MAX_X];
} IMEDISPLAY, *PIMEDISPLAY;

// Keyboard layout info
typedef struct tagKLINFO
{
    HKL hKL;
    DWORD dwConversion;
} KLINFO, *PKLINFO;

// Flags for KLINFO.dwConversion
#define _IME_CMODE_OPEN 0x20000000
#define _IME_CMODE_DEACTIVATE 0x40000000
#define _IME_CMODE_MASK (_IME_CMODE_OPEN | _IME_CMODE_DEACTIVATE)

#define MAX_CANDLIST 32
#define MAX_ATTR_COLORS 8

// IME composition string info
typedef struct tagCOMPSTRINFO
{
    DWORD dwSize;
    DWORD dwCompAttrLen;
    DWORD dwCompAttrOffset;
    DWORD dwCompStrLen;
    DWORD dwCompStrOffset;
    DWORD dwResultStrLen;
    DWORD dwResultStrOffset;
    WORD  awAttrColor[MAX_ATTR_COLORS];
} COMPSTRINFO, *PCOMPSTRINFO;

// IME candidate info
typedef struct tagCANDINFO
{
    DWORD dwAttrsOffset;
    WCHAR szCandStr[ANYSIZE_ARRAY];
} CANDINFO, *PCANDINFO;

// Console entry
typedef struct tagCONENTRY
{
    HANDLE hConsole;
    HWND hwndConsole;
    COORD ScreenSize;
    HKL hKL;
    HIMC hOldIMC;
    HIMC hNewIMC;
    BOOL bOpened;
    DWORD dwConversion;
    DWORD dwSentence;
    WORD nCodePage;
    WORD nOutputCodePage;
    WCHAR szLayoutText[256];
    WCHAR szMode[10];
    BOOL bInComposition;
    PCOMPSTRINFO pCompStr;
    WORD awAttrColor[MAX_ATTR_COLORS]; // See COMPSTRINFO.awAttrColor
    BOOL bHasAnyCand;
    PCANDIDATELIST apCandList[MAX_CANDLIST]; // See acbCandList below
    PCANDINFO pCandInfo;
    DWORD dwSystemLineSize;
    DWORD acbCandList[MAX_CANDLIST]; // See apCandList above
    DWORD dwCandOffset;
    DWORD dwCandIndexMax;
    PDWORD pdwCandPageStart;
    DWORD cbCandPageData;
    BOOL bSkipPageMsg;
    DWORD dwImeProp;
    BOOL bConsoleEnabled;
    BOOL bWndEnabled;
    INT cKLs;
    PKLINFO pKLInfo;
} CONENTRY, *PCONENTRY;
