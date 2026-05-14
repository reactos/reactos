/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Implement shell light-weight utility functions
 * COPYRIGHT:   Copyright 2023-2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#define _ATL_NO_EXCEPTIONS

/*
 * HACK! These functions are conflicting with <shobjidl.h> inline functions...
 */
#define IShellFolder_GetDisplayNameOf _disabled_IShellFolder_GetDisplayNameOf_
#define IShellFolder_ParseDisplayName _disabled_IShellFolder_ParseDisplayName_
#define IShellFolder_CompareIDs _disabled_IShellFolder_CompareIDs_

#include "precomp.h"
#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj_undoc.h>
#include <shlguid_undoc.h>
#include <atlstr.h>

#include <shlwapi_undoc.h>
#include <ishellfolder_helpers.h>

#include <strsafe.h>

#ifndef FAILED_UNEXPECTEDLY
#define FAILED_UNEXPECTEDLY FAILED /* FIXME: Make shellutils.h usable without ATL */
#endif

WINE_DEFAULT_DEBUG_CHANNEL(shell);

EXTERN_C LSTATUS WINAPI RegGetValueW(HKEY, LPCWSTR, LPCWSTR, DWORD, LPDWORD, PVOID, LPDWORD);

static inline WORD 
GetVersionMajorMinor()
{
    DWORD version = GetVersion();
    return MAKEWORD(HIBYTE(version), LOBYTE(version));
}

static BOOL CharLowerNoDBCSAWorker(PSTR lpString, INT cchMax, BOOL bUppercase)
{
    CHAR szBuff[MAX_PATH];
    INT cch;
    if (!lpString)
        return FALSE;
    cch = cchMax ? cchMax : lstrlenA(lpString);
    if (FAILED(StringCchCopyA(szBuff, _countof(szBuff), lpString)))
        return FALSE;
    return LCMapStringA(LOCALE_SYSTEM_DEFAULT, (bUppercase ? LCMAP_UPPERCASE : LCMAP_LOWERCASE),
                        szBuff, cch, lpString, cch);
}

static BOOL CharLowerNoDBCSWWorker(PWSTR lpString, INT cchMax, BOOL bUppercase)
{
    WCHAR szDest[MAX_PATH];
    INT cch;
    if (!lpString)
        return FALSE;
    cch = cchMax ? cchMax : lstrlenW(lpString);
    if (FAILED(StringCchCopyW(szDest, _countof(szDest), lpString)))
        return FALSE;
    return LCMapStringW(LOCALE_SYSTEM_DEFAULT, (bUppercase ? LCMAP_UPPERCASE : LCMAP_LOWERCASE),
                        szDest, cch, lpString, cch);
}

/*************************************************************************
 * CharLowerNoDBCSA [SHLWAPI.453]
 */
EXTERN_C PSTR WINAPI CharLowerNoDBCSA(_Inout_ PSTR lpString)
{
    return CharLowerNoDBCSAWorker(lpString, 0, FALSE) ? lpString : NULL;
}

/*************************************************************************
 * CharLowerNoDBCSW [SHLWAPI.454]
 */
EXTERN_C PWSTR WINAPI CharLowerNoDBCSW(_Inout_ PWSTR lpString)
{
    return CharLowerNoDBCSWWorker(lpString, 0, FALSE) ? lpString : NULL;
}

/*************************************************************************
 * CharUpperNoDBCSA [SHLWAPI.451]
 */
EXTERN_C PSTR WINAPI CharUpperNoDBCSA(_Inout_ PSTR lpString)
{
    return CharLowerNoDBCSAWorker(lpString, 0, TRUE) ? lpString : NULL;
}

/*************************************************************************
 * CharUpperNoDBCSW [SHLWAPI.452]
 */
EXTERN_C PWSTR WINAPI CharUpperNoDBCSW(_Inout_ PWSTR lpString)
{
    return CharLowerNoDBCSWWorker(lpString, 0, TRUE) ? lpString : NULL;
}

static HRESULT
SHInvokeCommandOnContextMenuInternal(
    _In_opt_ HWND hWnd,
    _In_opt_ IUnknown* pUnk,
    _In_ IContextMenu* pCM,
    _In_ UINT fCMIC,
    _In_ UINT fCMF,
    _In_opt_ LPCSTR pszVerb,
    _In_opt_ LPCWSTR pwszDir,
    _In_ bool ForceQCM)
{
    CMINVOKECOMMANDINFOEX info = { sizeof(info), fCMIC, hWnd, pszVerb };
    INT iDefItem = 0;
    HMENU hMenu = NULL;
    HCURSOR hOldCursor;
    HRESULT hr = S_OK;
    WCHAR wideverb[MAX_PATH];

    if (!pCM)
        return E_INVALIDARG;

    hOldCursor = SetCursor(LoadCursorW(NULL, (LPCWSTR)IDC_WAIT));
    info.nShow = SW_NORMAL;
    if (pUnk)
        IUnknown_SetSite(pCM, pUnk);

    if (IS_INTRESOURCE(pszVerb))
    {
        hMenu = CreatePopupMenu();
        if (hMenu)
        {
            hr = pCM->QueryContextMenu(hMenu, 0, 1, MAXSHORT, fCMF | CMF_DEFAULTONLY);
            iDefItem = GetMenuDefaultItem(hMenu, 0, 0);
            if (iDefItem != -1)
                info.lpVerb = MAKEINTRESOURCEA(iDefItem - 1);
        }
        info.lpVerbW = MAKEINTRESOURCEW(info.lpVerb);
    }
    else
    {
        if (GetVersionMajorMinor() >= _WIN32_WINNT_WIN7)
        {
            info.fMask |= CMF_OPTIMIZEFORINVOKE;
        }
        if (pszVerb && SHAnsiToUnicode(pszVerb, wideverb, _countof(wideverb)))
        {
            info.fMask |= CMIC_MASK_UNICODE;
            info.lpVerbW = wideverb;
        }
        if (ForceQCM)
        {
            hMenu = CreatePopupMenu();
            hr = pCM->QueryContextMenu(hMenu, 0, 1, MAXSHORT, fCMF);
        }
    }

    SetCursor(hOldCursor);

    if (!FAILED_UNEXPECTEDLY(hr) && (iDefItem != -1 || info.lpVerb))
    {
        if (!hWnd)
            info.fMask |= CMIC_MASK_FLAG_NO_UI;

        CHAR dir[MAX_PATH];
        if (pwszDir)
        {
            info.fMask |= CMIC_MASK_UNICODE;
            info.lpDirectoryW = pwszDir;
            if (SHUnicodeToAnsi(pwszDir, dir, _countof(dir)))
                info.lpDirectory = dir;
        }

        hr = pCM->InvokeCommand((LPCMINVOKECOMMANDINFO)&info);
        if (FAILED_UNEXPECTEDLY(hr)) { /* Diagnostic message */ }
    }

    if (pUnk)
        IUnknown_SetSite(pCM, NULL);
    if (hMenu)
        DestroyMenu(hMenu);

    return hr;
}

/*************************************************************************
 * SHInvokeCommandOnContextMenuEx [SHLWAPI.639]
 */
EXTERN_C
HRESULT WINAPI
SHInvokeCommandOnContextMenuEx(
    _In_opt_ HWND hWnd,
    _In_opt_ IUnknown* pUnk,
    _In_ IContextMenu* pCM,
    _In_ UINT fCMIC,
    _In_ UINT fCMF,
    _In_opt_ LPCSTR pszVerb,
    _In_opt_ LPCWSTR pwszDir)
{
    return SHInvokeCommandOnContextMenuInternal(hWnd, pUnk, pCM, fCMIC, fCMF, pszVerb, pwszDir, true);
}

/*************************************************************************
 * SHInvokeCommandOnContextMenu [SHLWAPI.540]
 */
EXTERN_C
HRESULT WINAPI
SHInvokeCommandOnContextMenu(
    _In_opt_ HWND hWnd,
    _In_opt_ IUnknown* pUnk,
    _In_ IContextMenu* pCM,
    _In_ UINT fCMIC,
    _In_opt_ LPCSTR pszVerb)
{
    return SHInvokeCommandOnContextMenuEx(hWnd, pUnk, pCM, fCMIC, CMF_EXTENDEDVERBS, pszVerb, NULL);
}

static inline BOOL
IsTextAsciiOnly(PCSTR psz)
{
    for (const signed char *pch = (const signed char *)psz; *pch; ++pch)
    {
        if (*pch < 0)
            return FALSE;
    }
    return TRUE;
}

/*************************************************************************
 * SHInvokeCommandsOnContextMenu [SHLWAPI.541]
 */
EXTERN_C
HRESULT WINAPI
SHInvokeCommandsOnContextMenu(
    _In_opt_ HWND hwnd,
    _In_opt_ IUnknown *punkSite,
    _In_ IContextMenu *pCM,
    _In_ DWORD fMask,
    _In_reads_opt_(cVerbs) PCSTR *pVerbs,
    _In_ UINT cVerbs)
{
    HRESULT hr;
    CMINVOKECOMMANDINFOEX ici;
    WCHAR szVerbW[MAX_PATH];
    HMENU hMenu = NULL;
    UINT iVerb, idDefault = (UINT)-1;
    PCSTR pszVerbA = NULL;

    if (!pCM)
        return E_INVALIDARG;

    hMenu = CreatePopupMenu();
    if (!hMenu)
        return E_OUTOFMEMORY;

    if (punkSite)
        IUnknown_SetSite(pCM, punkSite);

    hr = pCM->QueryContextMenu(hMenu, 0, 1, MAXSHORT, (cVerbs ? 0 : CMF_DEFAULTONLY));
    if (FAILED(hr))
        goto Cleanup;

    if (!cVerbs)
    {
        idDefault = GetMenuDefaultItem(hMenu, FALSE, 0);
        if (idDefault != (UINT)-1)
            pszVerbA = MAKEINTRESOURCEA(idDefault - 1);
    }

    ZeroMemory(&ici, sizeof(ici));
    ici.cbSize = sizeof(ici);
    ici.hwnd   = hwnd;
    ici.nShow  = SW_SHOWNORMAL;

    iVerb = 0;
    do
    {
        if (cVerbs)
            pszVerbA = pVerbs[iVerb];

        if (!pszVerbA && idDefault == (UINT)-1)
        {
            hr = E_FAIL;
            break;
        }

        ici.fMask   = fMask;
        ici.lpVerb  = pszVerbA;
        ici.lpVerbW = NULL;

        if (idDefault == (UINT)-1 && !IS_INTRESOURCE(pszVerbA) && IsTextAsciiOnly(pszVerbA))
        {
            size_t ich;
            for (ich = 0; pszVerbA[ich] && ich + 1 < _countof(szVerbW); ++ich)
            {
                szVerbW[ich] = (BYTE)pszVerbA[ich];
            }
            szVerbW[ich] = UNICODE_NULL;

            ici.lpVerbW = szVerbW;
            ici.fMask |= CMIC_MASK_UNICODE;
        }

        hr = pCM->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici);

        if (SUCCEEDED(hr) || hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
            break;

        ++iVerb;
    } while (iVerb < cVerbs);

Cleanup:
    if (punkSite)
        IUnknown_SetSite(pCM, NULL);
    DestroyMenu(hMenu);
    return hr;
}

/*************************************************************************
 * SHInvokeCommandWithFlagsAndSite [SHLWAPI.571]
 */
EXTERN_C
HRESULT WINAPI
SHInvokeCommandWithFlagsAndSite(
    _In_opt_ HWND hWnd,
    _In_opt_ IUnknown* pUnk,
    _In_ IShellFolder* pShellFolder,
    _In_ LPCITEMIDLIST pidl,
    _In_ UINT fCMIC,
    _In_opt_ LPCSTR pszVerb)
{
    HRESULT hr = E_INVALIDARG;
    if (pShellFolder)
    {
        IContextMenu *pCM;
        hr = pShellFolder->GetUIObjectOf(hWnd, 1, &pidl, IID_IContextMenu, NULL, (void**)&pCM);
        if (SUCCEEDED(hr))
        {
            fCMIC |= CMIC_MASK_FLAG_LOG_USAGE;
            hr = SHInvokeCommandOnContextMenuEx(hWnd, pUnk, pCM, fCMIC, 0, pszVerb, NULL);
            pCM->Release();
        }
    }
    return hr;
}


/*************************************************************************
 * IContextMenu_Invoke [SHLWAPI.207]
 *
 * Used by Win:SHELL32!CISFBand::_TrySimpleInvoke.
 */
EXTERN_C
BOOL WINAPI
IContextMenu_Invoke(
    _In_ IContextMenu *pContextMenu,
    _In_ HWND hwnd,
    _In_ LPCSTR lpVerb,
    _In_ UINT uFlags)
{
    TRACE("(%p, %p, %s, %u)\n", pContextMenu, hwnd, debugstr_a(lpVerb), uFlags);
    HRESULT hr = SHInvokeCommandOnContextMenuInternal(hwnd, NULL, pContextMenu, 0,
                                                      uFlags, lpVerb, NULL, false);
    return !FAILED_UNEXPECTEDLY(hr);
}

/*************************************************************************
 * ShellExecuteCommand [INTERNAL]
 */
static HRESULT
ShellExecuteCommand(_In_opt_ HWND hWnd, _In_ PCWSTR Command, _In_opt_ UINT Flags)
{
    WCHAR szCmd[MAX_PATH * 2];
    int len = PathProcessCommand(Command, szCmd, _countof(szCmd), PPCF_ADDARGUMENTS | PPCF_FORCEQUALIFY);
    if (len <= 0) // Could not resolve the command, just use the input
    {
        HRESULT hr = StringCchCopyW(szCmd, _countof(szCmd), Command);
        if (FAILED(hr))
            return hr;
    }
    PWSTR pszArgs = PathGetArgsW(szCmd);
    PathRemoveArgsW(szCmd);
    PathUnquoteSpacesW(szCmd);

    SHELLEXECUTEINFOW sei = { sizeof(sei), Flags, hWnd, NULL, szCmd, pszArgs };
    sei.nShow = SW_SHOW;
    UINT error = ShellExecuteExW(&sei) ? ERROR_SUCCESS : GetLastError();
    return HRESULT_FROM_WIN32(error);
}

/*************************************************************************
 * RunRegCommand [SHLWAPI.469]
 */
EXTERN_C HRESULT WINAPI
RunRegCommand(_In_opt_ HWND hWnd, _In_ HKEY hKey, _In_opt_ PCWSTR pszSubKey)
{
    WCHAR szCmd[MAX_PATH * 2];
    DWORD cb = sizeof(szCmd);
    DWORD error = RegGetValueW(hKey, pszSubKey, NULL, RRF_RT_REG_SZ, NULL, szCmd, &cb);
    if (error)
        return HRESULT_FROM_WIN32(error);
    return ShellExecuteCommand(hWnd, szCmd, SEE_MASK_FLAG_LOG_USAGE);
}

/*************************************************************************
 * RunIndirectRegCommand [SHLWAPI.468]
 */
EXTERN_C HRESULT WINAPI
RunIndirectRegCommand(_In_opt_ HWND hWnd, _In_ HKEY hKey, _In_opt_ PCWSTR pszSubKey, _In_ PCWSTR pszVerb)
{
    WCHAR szKey[MAX_PATH];
    HRESULT hr;
    if (pszSubKey)
        hr = StringCchPrintfW(szKey, _countof(szKey), L"%s\\shell\\%s\\command", pszSubKey, pszVerb);
    else
        hr = StringCchPrintfW(szKey, _countof(szKey), L"shell\\%s\\command", pszVerb);
    return SUCCEEDED(hr) ? RunRegCommand(hWnd, hKey, szKey) : hr;
}

/*************************************************************************
 * SHRunIndirectRegClientCommand [SHLWAPI.467]
 */
EXTERN_C HRESULT WINAPI
SHRunIndirectRegClientCommand(_In_opt_ HWND hWnd, _In_ PCWSTR pszClientType)
{
    WCHAR szKey[MAX_PATH], szClient[MAX_PATH];
    HRESULT hr = StringCchPrintfW(szKey, _countof(szKey), L"Software\\Clients\\%s", pszClientType);
    if (FAILED(hr))
        return hr;

    // Find the default client
    DWORD error, cb;
    cb = sizeof(szClient);
    error = RegGetValueW(HKEY_CURRENT_USER, szKey, NULL, RRF_RT_REG_SZ, NULL, szClient, &cb);
    if (error)
    {
        cb = sizeof(szClient);
        if (error != ERROR_MORE_DATA && error != ERROR_BUFFER_OVERFLOW)
            error = RegGetValueW(HKEY_LOCAL_MACHINE, szKey, NULL, RRF_RT_REG_SZ, NULL, szClient, &cb);
        if (error)
            return HRESULT_FROM_WIN32(error);
    }

    hr = StringCchPrintfW(szKey, _countof(szKey), L"Software\\Clients\\%s\\%s", pszClientType, szClient);
    if (SUCCEEDED(hr))
        hr = RunIndirectRegCommand(hWnd, HKEY_LOCAL_MACHINE, szKey, L"open");
    return hr;
}

/*************************************************************************
 * PathFileExistsDefExtAndAttributesW [SHLWAPI.511]
 *
 * @param pszPath The path string.
 * @param dwWhich The WHICH_... flags.
 * @param pdwFileAttributes A pointer to the file attributes. Optional.
 * @return TRUE if successful.
 */
BOOL WINAPI
PathFileExistsDefExtAndAttributesW(
    _Inout_ LPWSTR pszPath,
    _In_ DWORD dwWhich,
    _Out_opt_ LPDWORD pdwFileAttributes)
{
    TRACE("(%s, 0x%lX, %p)\n", debugstr_w(pszPath), dwWhich, pdwFileAttributes);

    if (pdwFileAttributes)
        *pdwFileAttributes = INVALID_FILE_ATTRIBUTES;

    if (!pszPath)
        return FALSE;

    if (!dwWhich || (*PathFindExtensionW(pszPath) && (dwWhich & WHICH_OPTIONAL)))
        return PathFileExistsAndAttributesW(pszPath, pdwFileAttributes);

    if (!PathFileExistsDefExtW(pszPath, dwWhich))
    {
        if (pdwFileAttributes)
            *pdwFileAttributes = INVALID_FILE_ATTRIBUTES;
        return FALSE;
    }

    if (pdwFileAttributes)
        *pdwFileAttributes = GetFileAttributesW(pszPath);

    return TRUE;
}

static inline BOOL
SHLWAPI_IsBogusHRESULT(HRESULT hr)
{
    return (hr == E_FAIL || hr == E_INVALIDARG || hr == E_NOTIMPL);
}

// Used for IShellFolder_GetDisplayNameOf
struct RETRY_DATA
{
    SHGDNF uRemove;
    SHGDNF uAdd;
    DWORD dwRetryFlags;
};
static const RETRY_DATA g_RetryData[] =
{
    { SHGDN_FOREDITING,    SHGDN_NORMAL,     SFGDNO_RETRYALWAYS         },
    { SHGDN_FORADDRESSBAR, SHGDN_NORMAL,     SFGDNO_RETRYALWAYS         },
    { SHGDN_NORMAL,        SHGDN_FORPARSING, SFGDNO_RETRYALWAYS         },
    { SHGDN_FORPARSING,    SHGDN_NORMAL,     SFGDNO_RETRYWITHFORPARSING },
    { SHGDN_INFOLDER,      SHGDN_NORMAL,     SFGDNO_RETRYALWAYS         },
};

/*************************************************************************
 * IShellFolder_GetDisplayNameOf [SHLWAPI.316]
 *
 * @note Don't confuse with <shobjidl.h> inline function of the same name.
 *       If the original call fails with the given uFlags, this function will
 *       retry with other flags to attempt retrieving any meaningful description.
 */
EXTERN_C HRESULT WINAPI
IShellFolder_GetDisplayNameOf(
    _In_ IShellFolder *psf,
    _In_ LPCITEMIDLIST pidl,
    _In_ SHGDNF uFlags,
    _Out_ LPSTRRET lpName,
    _In_ DWORD dwRetryFlags) // dwRetryFlags is an additional parameter
{
    HRESULT hr;

    TRACE("(%p)->(%p, 0x%lX, %p, 0x%lX)\n", psf, pidl, uFlags, lpName, dwRetryFlags);

    hr = psf->GetDisplayNameOf(pidl, uFlags, lpName);
    if (!SHLWAPI_IsBogusHRESULT(hr))
        return hr;

    dwRetryFlags |= SFGDNO_RETRYALWAYS;

    if ((uFlags & SHGDN_FORPARSING) == 0)
        dwRetryFlags |= SFGDNO_RETRYWITHFORPARSING;

    // Retry with other flags to get successful results
    for (SIZE_T iEntry = 0; iEntry < _countof(g_RetryData); ++iEntry)
    {
        const RETRY_DATA *pData = &g_RetryData[iEntry];
        if (!(dwRetryFlags & pData->dwRetryFlags))
            continue;

        SHGDNF uNewFlags = ((uFlags & ~pData->uRemove) | pData->uAdd);
        if (uNewFlags == uFlags)
            continue;

        hr = psf->GetDisplayNameOf(pidl, uNewFlags, lpName);
        if (!SHLWAPI_IsBogusHRESULT(hr))
            break;

        uFlags = uNewFlags; // Update flags every time
    }

    return hr;
}

/*************************************************************************
 * IShellFolder_ParseDisplayName [SHLWAPI.317]
 *
 * @note Don't confuse with <shobjidl.h> inline function of the same name.
 *       This function is safer than IShellFolder::ParseDisplayName.
 */
EXTERN_C HRESULT WINAPI
IShellFolder_ParseDisplayName(
    _In_ IShellFolder *psf,
    _In_opt_ HWND hwndOwner,
    _In_opt_ LPBC pbcReserved,
    _In_ LPOLESTR lpszDisplayName,
    _Out_opt_ ULONG *pchEaten,
    _Out_opt_ PIDLIST_RELATIVE *ppidl,
    _Out_opt_ ULONG *pdwAttributes)
{
    ULONG dummy1, dummy2;

    TRACE("(%p)->(%p, %p, %s, %p, %p, %p)\n", psf, hwndOwner, pbcReserved,
          debugstr_w(lpszDisplayName), pchEaten, ppidl, pdwAttributes);

    if (!pdwAttributes)
    {
        dummy1 = 0;
        pdwAttributes = &dummy1;
    }

    if (!pchEaten)
    {
        dummy2 = 0;
        pchEaten = &dummy2;
    }

    if (ppidl)
        *ppidl = NULL;

    return psf->ParseDisplayName(hwndOwner, pbcReserved, lpszDisplayName, pchEaten,
                                 ppidl, pdwAttributes);
}

/*************************************************************************
 * IShellFolder_CompareIDs [SHLWAPI.551]
 *
 * @note Don't confuse with <shobjidl.h> inline function of the same name.
 *       This function tries IShellFolder2 if possible.
 */
EXTERN_C HRESULT WINAPI
IShellFolder_CompareIDs(
    _In_ IShellFolder *psf,
    _In_ LPARAM lParam,
    _In_ PCUIDLIST_RELATIVE pidl1,
    _In_ PCUIDLIST_RELATIVE pidl2)
{
    TRACE("(%p, %p, %p, %p)\n", psf, lParam, pidl1, pidl2);

    if (lParam & ~(SIZE_T)SHCIDS_COLUMNMASK)
    {
        /* Try as IShellFolder2 if possible */
        HRESULT hr = psf->QueryInterface(IID_IShellFolder2, (void **)&psf);
        if (FAILED(hr))
            lParam &= SHCIDS_COLUMNMASK;
        else
            psf->Release();
    }

    return psf->CompareIDs(lParam, pidl1, pidl2);
}

/*************************************************************************
 * SHDialogProc [INTERNAL]
 *
 * Used in SHDialogBox below
 */

typedef struct tagSHDIALOG
{
    SHDIALOGPROC fn;
    PVOID pThis;
} SHDIALOG, *PSHDIALOG;

static INT_PTR CALLBACK
SHDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PSHDIALOG pData;
    INT_PTR result;
    HWND hwndItem;
    LRESULT ret;

    if (uMsg == WM_INITDIALOG)
    {
        pData = (PSHDIALOG)lParam;
        SetWindowLongPtrA(hWnd, DWLP_USER, lParam);
        lParam = (LPARAM)pData->pThis;
    }
    else
    {
        pData = (PSHDIALOG)GetWindowLongPtrA(hWnd, DWLP_USER);
    }

    if (pData && pData->fn)
    {
        result = pData->fn(pData->pThis, hWnd, uMsg, wParam, lParam);
        if (result)
            return result;
    }

    switch (uMsg)
    {
        case WM_INITDIALOG:
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDHELP)
                return FALSE;

            hwndItem = GetDlgItem(hWnd, LOWORD(wParam));
            if (!hwndItem)
                return FALSE;

            ret = SendMessageA(hwndItem, WM_GETDLGCODE, 0, 0);
            if (!(ret & (DLGC_DEFPUSHBUTTON | DLGC_UNDEFPUSHBUTTON)))
                return FALSE;

            EndDialog(hWnd, LOWORD(wParam));
            return TRUE;

        default:
            return FALSE;
    }
}

/*************************************************************************
 * SHDialogBox [SHLWAPI.277]
 */
EXTERN_C INT_PTR WINAPI
SHDialogBox(
    _In_opt_ HINSTANCE hInstance,
    _In_ PCSTR lpTemplateName,
    _In_opt_ HWND hWndParent,
    _In_opt_ SHDIALOGPROC fn,
    _In_opt_ PVOID pThis)
{
    SHDIALOG data = { fn, pThis };
    return DialogBoxParamA(hInstance, lpTemplateName, hWndParent, SHDialogProc, (LPARAM)&data);
}
