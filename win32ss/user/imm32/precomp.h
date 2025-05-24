/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing IMM32
 * COPYRIGHT:   Copyright 1998 Patrik Stridvall
 *              Copyright 2002, 2003, 2007 CodeWeavers, Aric Stewart
 *              Copyright 2017 James Tabor <james.tabor@reactos.org>
 *              Copyright 2018 Amine Khaldi <amine.khaldi@reactos.org>
 *              Copyright 2020 Oleg Dubinskiy <oleg.dubinskij2013@yandex.ua>
 *              Copyright 2020-2022 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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
#include <immdev.h>
#include <imm32_undoc.h>

#define NTOS_MODE_USER
#include <ndk/umtypes.h>
#include <ndk/pstypes.h>
#include <ndk/rtlfuncs.h>

/* Public Win32K Headers */
#include "ntuser.h"
#include "ntwin32.h"

/* Undocumented user definitions */
#include <undocuser.h>

#include <strsafe.h>

#include <wine/debug.h>

#define ERR_PRINTF(fmt, ...) (__WINE_IS_DEBUG_ON(_ERR, __wine_dbch___default) ? \
    (wine_dbg_printf("err:(%s:%d) " fmt, __RELFILE__, __LINE__, ##__VA_ARGS__), TRUE) : TRUE)

/* Unexpected Condition Checkers */
#if DBG
    #define FAILED_UNEXPECTEDLY(hr) \
        (FAILED(hr) ? ERR_PRINTF("FAILED(0x%08X)\n", hr) : FALSE)
    #define IS_NULL_UNEXPECTEDLY(p) \
        (!(p) ? ERR_PRINTF("%s was NULL\n", #p) : FALSE)
    #define IS_ZERO_UNEXPECTEDLY(p) \
        (!(p) ? ERR_PRINTF("%s was zero\n", #p) : FALSE)
    #define IS_TRUE_UNEXPECTEDLY(x) \
        ((x) ? ERR_PRINTF("%s was non-zero\n", #x) : FALSE)
    #define IS_ERROR_UNEXPECTEDLY(x) \
        ((x) != ERROR_SUCCESS ? ERR_PRINTF("%s was %d\n", #x, (int)(x)) : FALSE)
#else
    #define FAILED_UNEXPECTEDLY(hr) FAILED(hr)
    #define IS_NULL_UNEXPECTEDLY(p) (!(p))
    #define IS_ZERO_UNEXPECTEDLY(p) (!(p))
    #define IS_TRUE_UNEXPECTEDLY(x) (x)
    #define IS_ERROR_UNEXPECTEDLY(x) ((x) != ERROR_SUCCESS)
#endif

#define IS_CROSS_THREAD_HIMC(hIMC)   IS_TRUE_UNEXPECTEDLY(Imm32IsCrossThreadAccess(hIMC))
#define IS_CROSS_PROCESS_HWND(hWnd)  IS_TRUE_UNEXPECTEDLY(Imm32IsCrossProcessAccess(hWnd))

#define IMM_INIT_MAGIC          0x19650412
#define IMM_INVALID_CANDFORM    ULONG_MAX
#define INVALID_HOTKEY_ID       0xFFFFFFFF
#define MAX_CANDIDATEFORM       4
#define MAX_IMM_FILENAME        80

#define LANGID_CHINESE_SIMPLIFIED   MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)
#define LANGID_CHINESE_TRADITIONAL  MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)
#define LANGID_JAPANESE             MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT)

#define REGKEY_KEYBOARD_LAYOUTS     L"System\\CurrentControlSet\\Control\\Keyboard Layouts"

extern HMODULE ghImm32Inst;
extern RTL_CRITICAL_SECTION gcsImeDpi;
extern PIMEDPI gpImeDpiList;
extern PSERVERINFO gpsi;
extern SHAREDINFO gSharedInfo;
extern HANDLE ghImmHeap;
extern DWORD g_aimm_compat_flags;

BOOL Imm32GetSystemLibraryPath(LPWSTR pszPath, DWORD cchPath, LPCWSTR pszFileName);
VOID APIENTRY LogFontAnsiToWide(const LOGFONTA *plfA, LPLOGFONTW plfW);
VOID APIENTRY LogFontWideToAnsi(const LOGFONTW *plfW, LPLOGFONTA plfA);
LPVOID FASTCALL ValidateHandleNoErr(HANDLE hObject, UINT uType);
LPVOID FASTCALL ValidateHandle(HANDLE hObject, UINT uType);
#define ValidateHwndNoErr(hwnd) ValidateHandleNoErr((hwnd), TYPE_WINDOW)
#define ValidateHwnd(hwnd) ValidateHandle((hwnd), TYPE_WINDOW)
BOOL APIENTRY Imm32CheckImcProcess(PIMC pIMC);

LPVOID ImmLocalAlloc(_In_ DWORD dwFlags, _In_ DWORD dwBytes);
#define ImmLocalFree(lpData) HeapFree(ghImmHeap, 0, (lpData))

LPWSTR APIENTRY Imm32WideFromAnsi(UINT uCodePage, LPCSTR pszA);
LPSTR APIENTRY Imm32AnsiFromWide(UINT uCodePage, LPCWSTR pszW);
LONG APIENTRY IchWideFromAnsi(LONG cchAnsi, LPCSTR pchAnsi, UINT uCodePage);
LONG APIENTRY IchAnsiFromWide(LONG cchWide, LPCWSTR pchWide, UINT uCodePage);
PIMEDPI APIENTRY Imm32FindOrLoadImeDpi(HKL hKL);
LPINPUTCONTEXT APIENTRY Imm32InternalLockIMC(HIMC hIMC, BOOL fSelect);
BOOL APIENTRY Imm32ReleaseIME(HKL hKL);
BOOL Imm32IsSystemJapaneseOrKorean(VOID);
BOOL APIENTRY Imm32IsCrossThreadAccess(HIMC hIMC);
BOOL APIENTRY Imm32IsCrossProcessAccess(HWND hWnd);
BOOL Imm32IsImcAnsi(HIMC hIMC);
BOOL Imm32LoadImeVerInfo(_Out_ PIMEINFOEX pImeInfoEx);

#define ImeDpi_IsUnicode(pImeDpi)      ((pImeDpi)->ImeInfo.fdwProperty & IME_PROP_UNICODE)

DWORD APIENTRY
CandidateListWideToAnsi(const CANDIDATELIST *pWideCL, LPCANDIDATELIST pAnsiCL, DWORD dwBufLen,
                        UINT uCodePage);
DWORD APIENTRY
CandidateListAnsiToWide(const CANDIDATELIST *pAnsiCL, LPCANDIDATELIST pWideCL, DWORD dwBufLen,
                        UINT uCodePage);

BOOL
Imm32MakeIMENotify(
    _In_ HIMC hIMC,
    _In_ HWND hwnd,
    _In_ DWORD dwAction,
    _In_ DWORD dwIndex,
    _Inout_opt_ DWORD_PTR dwValue,
    _In_ DWORD dwCommand,
    _Inout_opt_ DWORD_PTR dwData);

DWORD APIENTRY Imm32BuildHimcList(DWORD dwThreadId, HIMC **pphList);

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

HRESULT
Imm32StrToUInt(
    _In_ PCWSTR pszText,
    _Out_ PDWORD pdwValue,
    _In_ ULONG nBase);

HRESULT
Imm32UIntToStr(
    _In_ DWORD dwValue,
    _In_ ULONG nBase,
    _Out_ PWSTR pszBuff,
    _In_ USHORT cchBuff);

PTHREADINFO FASTCALL Imm32CurrentPti(VOID);

HRESULT CtfImmTIMCreateInputContext(_In_ HIMC hIMC);
HRESULT CtfImmTIMDestroyInputContext(_In_ HIMC hIMC);
HRESULT CtfImmCoInitialize(VOID);
HRESULT CtfImeCreateThreadMgr(VOID);
HRESULT CtfImeDestroyThreadMgr(VOID);
HRESULT Imm32ActivateOrDeactivateTIM(_In_ BOOL bCreate);

HRESULT
CtfImeSetActiveContextAlways(
    _In_ HIMC hIMC,
    _In_ BOOL fActive,
    _In_ HWND hWnd,
    _In_ HKL hKL);

BOOL
CtfImeProcessCicHotkey(
    _In_ HIMC hIMC,
    _In_ UINT vKey,
    _In_ LPARAM lParam);

LRESULT
CtfImmSetLangBand(
    _In_ HWND hWnd,
    _In_ BOOL fSet);
