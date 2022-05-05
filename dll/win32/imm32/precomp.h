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
#include <winnls32.h>
#include <winver.h>

#include <imm.h>
#include <ddk/immdev.h>

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
#define MAX_IMM_FILENAME        80

#define LANGID_CHINESE_SIMPLIFIED   MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)
#define LANGID_CHINESE_TRADITIONAL  MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)
#define LANGID_JAPANESE             MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT)

#define REGKEY_KEYBOARD_LAYOUTS     L"System\\CurrentControlSet\\Control\\Keyboard Layouts"
#define REGKEY_IMM                  L"Software\\Microsoft\\Windows NT\\CurrentVersion\\IMM"

#define ROUNDUP4(n) (((n) + 3) & ~3)  /* DWORD alignment */

typedef struct REG_IME
{
    HKL hKL;
    WCHAR szImeKey[20];     /* "E0XXYYYY": "E0XX" is the device handle. "YYYY" is a LANGID. */
    WCHAR szFileName[80];   /* The IME module filename */
} REG_IME, *PREG_IME;

extern HMODULE ghImm32Inst;
extern RTL_CRITICAL_SECTION gcsImeDpi;
extern PIMEDPI gpImeDpiList;
extern PSERVERINFO gpsi;
extern SHAREDINFO gSharedInfo;
extern HANDLE ghImmHeap;

BOOL Imm32GetSystemLibraryPath(LPWSTR pszPath, DWORD cchPath, LPCWSTR pszFileName);
VOID APIENTRY LogFontAnsiToWide(const LOGFONTA *plfA, LPLOGFONTW plfW);
VOID APIENTRY LogFontWideToAnsi(const LOGFONTW *plfW, LPLOGFONTA plfA);
LPVOID FASTCALL ValidateHandleNoErr(HANDLE hObject, UINT uType);
LPVOID FASTCALL ValidateHandle(HANDLE hObject, UINT uType);
#define ValidateHwndNoErr(hwnd) ValidateHandleNoErr((hwnd), TYPE_WINDOW)
#define ValidateHwnd(hwnd) ValidateHandle((hwnd), TYPE_WINDOW)
BOOL APIENTRY Imm32CheckImcProcess(PIMC pIMC);

LPVOID APIENTRY ImmLocalAlloc(DWORD dwFlags, DWORD dwBytes);
#define ImmLocalFree(lpData) HeapFree(ghImmHeap, 0, (lpData))

LPWSTR APIENTRY Imm32WideFromAnsi(LPCSTR pszA);
LPSTR APIENTRY Imm32AnsiFromWide(LPCWSTR pszW);
LONG APIENTRY IchWideFromAnsi(LONG cchAnsi, LPCSTR pchAnsi, UINT uCodePage);
LONG APIENTRY IchAnsiFromWide(LONG cchWide, LPCWSTR pchWide, UINT uCodePage);
PIMEDPI APIENTRY Imm32FindOrLoadImeDpi(HKL hKL);
LPINPUTCONTEXT APIENTRY Imm32InternalLockIMC(HIMC hIMC, BOOL fSelect);
BOOL APIENTRY Imm32ReleaseIME(HKL hKL);
BOOL APIENTRY Imm32IsSystemJapaneseOrKorean(VOID);

static inline BOOL Imm32IsCrossThreadAccess(HIMC hIMC)
{
    DWORD dwImeThreadId = (DWORD)NtUserQueryInputContext(hIMC, QIC_INPUTTHREADID);
    DWORD dwThreadId = GetCurrentThreadId();
    return (dwImeThreadId != dwThreadId);
}

// Win: TestWindowProcess
static inline BOOL Imm32IsCrossProcessAccess(HWND hWnd)
{
    return (NtUserQueryWindow(hWnd, QUERY_WINDOW_UNIQUE_PROCESS_ID) !=
            (DWORD_PTR)NtCurrentTeb()->ClientId.UniqueProcess);
}

BOOL WINAPI Imm32IsImcAnsi(HIMC hIMC);

#define ImeDpi_IsUnicode(pImeDpi)   ((pImeDpi)->ImeInfo.fdwProperty & IME_PROP_UNICODE)
#define IS_IMM_MODE()               (gpsi && (gpsi->dwSRVIFlags & SRVINFO_IMM32))
#define Imm32IsCiceroMode()         (gpsi && (gpsi->dwSRVIFlags & SRVINFO_CICERO_ENABLED))
#define Imm32Is16BitMode()          (GetWin32ClientInfo()->dwTIFlags & TIF_16BIT)

DWORD APIENTRY
CandidateListWideToAnsi(const CANDIDATELIST *pWideCL, LPCANDIDATELIST pAnsiCL, DWORD dwBufLen,
                        UINT uCodePage);
DWORD APIENTRY
CandidateListAnsiToWide(const CANDIDATELIST *pAnsiCL, LPCANDIDATELIST pWideCL, DWORD dwBufLen,
                        UINT uCodePage);

BOOL APIENTRY
Imm32MakeIMENotify(HIMC hIMC, HWND hwnd, DWORD dwAction, DWORD_PTR dwIndex, DWORD_PTR dwValue,
                   DWORD_PTR dwCommand, DWORD_PTR dwData);

DWORD APIENTRY Imm32BuildHimcList(DWORD dwThreadId, HIMC **pphList);

INT APIENTRY
Imm32ImeMenuAnsiToWide(const IMEMENUITEMINFOA *pItemA, LPIMEMENUITEMINFOW pItemW,
                       UINT uCodePage, BOOL bBitmap);
INT APIENTRY
Imm32ImeMenuWideToAnsi(const IMEMENUITEMINFOW *pItemW, LPIMEMENUITEMINFOA pItemA,
                       UINT uCodePage);

PIME_STATE APIENTRY Imm32FetchImeState(LPINPUTCONTEXTDX pIC, HKL hKL);
PIME_SUBSTATE APIENTRY Imm32FetchImeSubState(PIME_STATE pState, HKL hKL);

BOOL APIENTRY
Imm32LoadImeStateSentence(LPINPUTCONTEXTDX pIC, PIME_STATE pState, HKL hKL);
BOOL APIENTRY
Imm32SaveImeStateSentence(LPINPUTCONTEXTDX pIC, PIME_STATE pState, HKL hKL);

DWORD APIENTRY
Imm32ReconvertAnsiFromWide(LPRECONVERTSTRING pDest, const RECONVERTSTRING *pSrc, UINT uCodePage);
DWORD APIENTRY
Imm32ReconvertWideFromAnsi(LPRECONVERTSTRING pDest, const RECONVERTSTRING *pSrc, UINT uCodePage);

HRESULT APIENTRY Imm32StrToUInt(LPCWSTR pszText, LPDWORD pdwValue, ULONG nBase);
HRESULT APIENTRY Imm32UIntToStr(DWORD dwValue, ULONG nBase, LPWSTR pszBuff, USHORT cchBuff);
BOOL APIENTRY Imm32LoadImeVerInfo(PIMEINFOEX pImeInfoEx);
UINT APIENTRY Imm32GetImeLayout(PREG_IME pLayouts, UINT cLayouts);
BOOL APIENTRY Imm32WriteImeLayout(HKL hKL, LPCWSTR pchFilePart, LPCWSTR pszLayout);
HKL APIENTRY Imm32AssignNewLayout(UINT cKLs, const REG_IME *pLayouts, WORD wLangID);
BOOL APIENTRY Imm32CopyImeFile(LPWSTR pszOldFile, LPCWSTR pszNewFile);

static inline PTHREADINFO FASTCALL Imm32CurrentPti(VOID)
{
    if (NtCurrentTeb()->Win32ThreadInfo == NULL)
        NtUserGetThreadState(THREADSTATE_GETTHREADINFO);
    return NtCurrentTeb()->Win32ThreadInfo;
}
