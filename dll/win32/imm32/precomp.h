/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing IMM32
 * COPYRIGHT:   Copyright 1998 Patrik Stridvall
 *              Copyright 2002, 2003, 2007 CodeWeavers, Aric Stewart
 *              Copyright 2017 James Tabor <james.tabor@reactos.org>
 *              Copyright 2018 Amine Khaldi <amine.khaldi@reactos.org>
 *              Copyright 2020 Oleg Dubinskiy <oleg.dubinskij2013@yandex.ua>
 *              Copyright 2020-2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>

/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <winnls.h>
#include <winreg.h>

#include <imm.h>
#include <ddk/imm.h>

#define NTOS_MODE_USER
#include <ndk/umtypes.h>
#include <ndk/pstypes.h>
#include <ndk/rtlfuncs.h>

/* Public Win32K Headers */
#include "ntuser.h"
#include "ntwin32.h"

/* Undocumented user definitions */
#include <undocuser.h>
#include <imm32_undoc.h>

#include <strsafe.h>

#include <wine/debug.h>
#include <wine/list.h>

#define IMM_INIT_MAGIC          0x19650412
#define IMM_INVALID_CANDFORM    ULONG_MAX
#define INVALID_HOTKEY_ID       0xFFFFFFFF
#define MAX_CANDIDATEFORM       4

#define LANGID_CHINESE_SIMPLIFIED   MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)
#define LANGID_CHINESE_TRADITIONAL  MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)
#define LANGID_JAPANESE             MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT)

#define REGKEY_KEYBOARD_LAYOUTS     L"System\\CurrentControlSet\\Control\\Keyboard Layouts"
#define REGKEY_IMM                  L"Software\\Microsoft\\Windows NT\\CurrentVersion\\IMM"

#define ROUNDUP4(n) (((n) + 3) & ~3)  /* DWORD alignment */

extern HMODULE g_hImm32Inst;
extern RTL_CRITICAL_SECTION g_csImeDpi;
extern PIMEDPI g_pImeDpiList;
extern PSERVERINFO g_psi;
extern SHAREDINFO g_SharedInfo;
extern BYTE g_bClientRegd;
extern HANDLE g_hImm32Heap;

BOOL Imm32GetSystemLibraryPath(LPWSTR pszPath, DWORD cchPath, LPCWSTR pszFileName);
VOID APIENTRY LogFontAnsiToWide(const LOGFONTA *plfA, LPLOGFONTW plfW);
VOID APIENTRY LogFontWideToAnsi(const LOGFONTW *plfW, LPLOGFONTA plfA);
PWND FASTCALL ValidateHwndNoErr(HWND hwnd);
LPVOID APIENTRY Imm32HeapAlloc(DWORD dwFlags, DWORD dwBytes);
LPWSTR APIENTRY Imm32WideFromAnsi(LPCSTR pszA);
LPSTR APIENTRY Imm32AnsiFromWide(LPCWSTR pszW);
PIMEDPI APIENTRY ImmLockOrLoadImeDpi(HKL hKL);

static inline BOOL Imm32IsCrossThreadAccess(HIMC hIMC)
{
    DWORD dwImeThreadId = NtUserQueryInputContext(hIMC, 1);
    DWORD dwThreadId = GetCurrentThreadId();
    return (dwImeThreadId != dwThreadId);
}

static inline BOOL Imm32IsCrossProcessAccess(HWND hWnd)
{
    return (NtUserQueryWindow(hWnd, QUERY_WINDOW_UNIQUE_PROCESS_ID) !=
            (DWORD_PTR)NtCurrentTeb()->ClientId.UniqueProcess);
}

DWORD APIENTRY
CandidateListWideToAnsi(const CANDIDATELIST *pWideCL, LPCANDIDATELIST pAnsiCL, DWORD dwBufLen,
                        UINT uCodePage);
DWORD APIENTRY
CandidateListAnsiToWide(const CANDIDATELIST *pAnsiCL, LPCANDIDATELIST pWideCL, DWORD dwBufLen,
                        UINT uCodePage);

BOOL APIENTRY
Imm32NotifyAction(HIMC hIMC, HWND hwnd, DWORD dwAction, DWORD_PTR dwIndex, DWORD_PTR dwValue,
                  DWORD_PTR dwCommand, DWORD_PTR dwData);

DWORD APIENTRY Imm32AllocAndBuildHimcList(DWORD dwThreadId, HIMC **pphList);
