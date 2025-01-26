/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Shell fusion functions
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

VOID SHELL_DelayLoadCC(VOID);

BOOL SHActivateContext(_Out_ ULONG_PTR *puCookie);
VOID SHDeactivateContext(_In_ ULONG_PTR uCookie);

VOID
SHGetManifest(
    _Out_ LPWSTR pszManifest,
    _In_ UINT cchManifestMax);

BOOL
SHFusionInitializeIDCC(
    _In_ LPCWSTR pszPath,
    _In_ WORD nResID,
    _In_ BOOL bDelayLoadCC);

BOOL
SHFusionInitializeFromModuleID(
    _In_ HMODULE hModule,
    _In_ WORD nResID);

VOID SHFusionUninitialize(VOID);

HMODULE
SHFusionLoadLibrary(
    _In_ LPCWSTR lpLibFileName);

HWND
SHFusionCreateWindow(
    _In_ LPCWSTR lpClassName,
    _In_opt_ LPCWSTR lpWindowName,
    _In_ DWORD dwStyle,
    _In_ INT X,
    _In_ INT Y,
    _In_ INT nWidth,
    _In_ INT nHeight,
    _In_opt_ HWND hWndParent,
    _In_opt_ HMENU hMenu,
    _In_ HINSTANCE hInstance,
    _Inout_opt_ LPVOID lpParam);

HWND
SHFusionCreateWindowEx(
    _In_ DWORD dwExStyle,
    _In_ LPCWSTR lpClassName,
    _In_opt_ LPCWSTR lpWindowName,
    _In_ DWORD dwStyle,
    _In_ INT X,
    _In_ INT Y,
    _In_ INT nWidth,
    _In_ INT nHeight,
    _In_opt_ HWND hWndParent,
    _In_opt_ HMENU hMenu,
    _In_ HINSTANCE hInstance,
    _Inout_opt_ LPVOID lpParam);

INT_PTR
SHFusionDialogBoxParam(
    _In_ HINSTANCE hInstance,
    _In_ LPCWSTR lpTemplateName,
    _In_ HWND hWndParent,
    _In_ DLGPROC lpDialogFunc,
    _Inout_opt_ LPARAM dwInitParam);

#ifdef __cplusplus
} // extern "C"
#endif
