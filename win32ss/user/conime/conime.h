/*
 * PROJECT:     ReactOS Console IME
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Console IME
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <wincon.h>
#include <winnls.h>
#include <winreg.h>
#include <kernel32_undoc.h>
#include <undocuser.h>
#include <imm.h>
#include <immdev.h>
#include <imm32_undoc.h>

#define NTOS_MODE_USER
#include <ndk/umtypes.h>
#include <ndk/psfuncs.h>
#include <ndk/sefuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/rtlfuncs.h>

#include <stdlib.h>
#include <strsafe.h>

/* PSEH for SEH Support */
#include <pseh/pseh2.h>

#include "conmsgs.h"
#include "conimebase.h"
#include "cjklangs.h"

#include "resource.h"

typedef struct tagKLINFO
{
    HKL hKL;
    DWORD dwImeState;
} KLINFO, *PKLINFO;

typedef struct tagCONSOLE_ENTRY
{
    HANDLE ConsoleHandle;
    HWND hwndConsole;
    COORD dwCoord;
    HKL hKL;
    HIMC hOldIMC;
    HIMC hIMC;
    BOOL bOpened;
    DWORD dwConversion;
    DWORD dwSentence;
    WORD wInputCodePage;
    WORD wOutputCodePage;
    WCHAR szLayoutText[256];
    WCHAR szMode[10];
    BOOL bInComposition;
    PCONIME_COMPSTR pCompStr;
    WORD unknown2[8];
    BOOL bHasCands;
    LPCANDIDATELIST apCandList[32];
    PCONIME_SYSTEMLINE pSystemLine;
    DWORD cbSystemLine;
    DWORD acbCandList[32];
    DWORD dwUnknown1;
    DWORD dwUnknown2;
    LPVOID pLocal2;
    DWORD unknown3_5_0;
    DWORD bSettingCandInfo;
    DWORD dwImeProp;
    BOOL bConsoleEnabled;
    BOOL bWndEnabled;
    INT cKLs;
    PKLINFO pKLInfo;
} CONSOLE_ENTRY, *PCONSOLE_ENTRY;

#ifndef _WIN64
C_ASSERT(sizeof(CONSOLE_ENTRY) == 0x388);
#endif

typedef struct tagUSER_DATA
{
    PCONSOLE_ENTRY apEntries[10];
} USER_DATA, *PUSER_DATA;
