/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Private header for msctf.dll
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include <msctf.h>
#include <inputscope.h>

#ifdef __cplusplus
extern "C" {
#endif

BOOL WINAPI TF_InitSystem(VOID);
BOOL WINAPI TF_UninitSystem(VOID);
HRESULT WINAPI TF_GetGlobalCompartment(_Out_ ITfCompartmentMgr **ppCompMgr);
HRESULT WINAPI TF_PostAllThreadMsg(_In_opt_ WPARAM wParam, _In_ DWORD dwFlags);
HANDLE WINAPI TF_CreateCicLoadMutex(_Out_ LPBOOL pfWinLogon);
HRESULT WINAPI TF_InvalidAssemblyListCache(VOID);
HRESULT WINAPI TF_InvalidAssemblyListCacheIfExist(VOID);
HICON WINAPI TF_GetLangIcon(_In_ LANGID LangID, _Out_writes_(cchText) PWSTR pszText, _In_ INT cchText);
VOID WINAPI TF_InitMlngInfo(VOID);
INT WINAPI TF_MlngInfoCount(VOID);
INT WINAPI TF_GetMlngIconIndex(_In_ INT iKL);
HICON WINAPI TF_InatExtractIcon(_In_ INT iKL);
HRESULT WINAPI TF_RunInputCPL(VOID);
LONG WINAPI TF_CheckThreadInputIdle(_In_ DWORD dwThreadId, _In_ DWORD dwMilliseconds);
BOOL WINAPI TF_IsInMarshaling(_In_ DWORD dwThreadId);

// This is intentionally misspelled to match the original name:
BOOL WINAPI TF_IsFullScreenWindowAcitvated(VOID);

HRESULT WINAPI TF_CUASAppFix(_In_ LPCSTR pszName);
HRESULT WINAPI TF_ClearLangBarAddIns(_In_ REFGUID rguid);
HRESULT WINAPI TF_GetInputScope(_In_opt_ HWND hWnd, _Out_ ITfInputScope **ppInputScope);
BOOL WINAPI TF_DllDetachInOther(VOID);

BOOL WINAPI
TF_GetMlngHKL(
    _In_ INT iKL,
    _Out_opt_ HKL *phKL,
    _Out_writes_opt_(cchText) LPWSTR pszText,
    _In_ INT cchText);

BOOL WINAPI
TF_GetThreadFlags(
    _In_ DWORD dwThreadId,
    _Out_ LPDWORD pdwFlags1,
    _Out_ LPDWORD pdwFlags2,
    _Out_ LPDWORD pdwFlags3);

#ifdef __cplusplus
} // extern "C"
#endif
