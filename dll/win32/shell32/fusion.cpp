/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Shell fusion functions
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include "fusion.h"

WINE_DEFAULT_DEBUG_CHANNEL(fusion);

HANDLE g_hActCtx = INVALID_HANDLE_VALUE;

EXTERN_C
VOID
SHELL_DelayLoadCC(VOID)
{
    // FIXME: Am I wanted by this module?
}

EXTERN_C
BOOL
SHActivateContext(_Out_ ULONG_PTR *puCookie)
{
    *puCookie = 0;

    if (g_hActCtx == INVALID_HANDLE_VALUE)
        return TRUE;

    return ActivateActCtx(g_hActCtx, puCookie);
}

EXTERN_C
VOID
SHDeactivateContext(_In_ ULONG_PTR uCookie)
{
    if (uCookie)
        DeactivateActCtx(0, uCookie);
}

EXTERN_C
VOID
SHGetManifest(
    _Out_ LPWSTR pszManifest,
    _In_ UINT cchManifestMax)
{
    UINT cch = GetSystemWindowsDirectoryW(pszManifest, cchManifestMax);
    if (cch < cchManifestMax)
        lstrcpynW(&pszManifest[cch], L"\\WindowsShell.Manifest", cchManifestMax - cch);
}

EXTERN_C
BOOL
SHFusionInitializeIDCC(
    _In_ LPCWSTR pszPath,
    _In_ WORD nResID,
    _In_ BOOL bDelayLoadCC)
{
    WCHAR szManifest[MAX_PATH];
    ACTCTXW ActCtx = { sizeof(ActCtx) };

    if (pszPath)
    {
        ActCtx.dwFlags = QUERY_ACTCTX_FLAG_ACTCTX_IS_HMODULE;
        ActCtx.lpResourceName = MAKEINTRESOURCEW(nResID);
    }
    else
    {
        SHGetManifest(szManifest, _countof(szManifest));
        pszPath = szManifest;
    }

    if (g_hActCtx == INVALID_HANDLE_VALUE)
    {
        ActCtx.lpSource = pszPath;
        g_hActCtx = CreateActCtx(&ActCtx);
    }

    if (bDelayLoadCC)
        SHELL_DelayLoadCC();

    return g_hActCtx != INVALID_HANDLE_VALUE;
}

EXTERN_C
BOOL
SHFusionInitializeFromModuleID(
    _In_ HMODULE hModule,
    _In_ WORD nResID)
{
    WCHAR szPath[MAX_PATH];
    GetModuleFileNameW(hModule, szPath, _countof(szPath));
    return SHFusionInitializeIDCC(szPath, nResID, TRUE);
}

EXTERN_C
VOID
SHFusionUninitialize(VOID)
{
    if (g_hActCtx != INVALID_HANDLE_VALUE)
    {
        ReleaseActCtx(g_hActCtx);
        g_hActCtx = INVALID_HANDLE_VALUE;
    }
}

EXTERN_C
HMODULE
SHFusionLoadLibrary(
    _In_ LPCWSTR lpLibFileName)
{
    ULONG_PTR uCookie;
    if (!SHActivateContext(&uCookie))
        return NULL;

    HINSTANCE hInst;
    _SEH2_TRY
    {
        hInst = LoadLibraryW(lpLibFileName);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        hInst = NULL;
    }
    _SEH2_END;

    SHDeactivateContext(uCookie);
    return hInst;
}

EXTERN_C
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
    _Inout_opt_ LPVOID lpParam)
{
    return SHFusionCreateWindowEx(0, lpClassName, lpWindowName, dwStyle,
                                  X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
}

EXTERN_C
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
    _Inout_opt_ LPVOID lpParam)
{
    ULONG_PTR uCookie;
    if (!SHActivateContext(&uCookie))
        return FALSE;

    HWND hWnd;
    _SEH2_TRY
    {
        SHELL_DelayLoadCC();
        hWnd = CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle,
                               X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        hWnd = NULL;
    }
    _SEH2_END;

    SHDeactivateContext(uCookie);
    return hWnd;
}

EXTERN_C
HWND
SHFusionCreateDialogParam(
    _In_ HINSTANCE hInstance,
    _In_ LPCWSTR lpTemplateName,
    _In_ HWND hWndParent,
    _In_ DLGPROC lpDialogFunc,
    _Inout_opt_ LPARAM dwInitParam)
{
    ULONG_PTR uCookie;
    if (!SHActivateContext(&uCookie))
        return NULL;

    HWND hWnd;
    _SEH2_TRY
    {
        SHELL_DelayLoadCC();
        hWnd = CreateDialogParamW(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        hWnd = NULL;
    }
    _SEH2_END;

    SHDeactivateContext(uCookie);
    return hWnd;
}

EXTERN_C
INT_PTR
SHFusionDialogBoxParam(
    _In_ HINSTANCE hInstance,
    _In_ LPCWSTR lpTemplateName,
    _In_ HWND hWndParent,
    _In_ DLGPROC lpDialogFunc,
    _Inout_opt_ LPARAM dwInitParam)
{
    ULONG_PTR uCookie;
    if (!SHActivateContext(&uCookie))
        return 0;

    INT_PTR ret;
    _SEH2_TRY
    {
        SHELL_DelayLoadCC();
        ret = DialogBoxParamW(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = 0;
    }
    _SEH2_END;

    SHDeactivateContext(uCookie);
    return ret;
}
