/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     SHOpenPropSheetW implementation
 * COPYRIGHT:   Copyright 2024 Whindmar Saksit <whindsaks@proton.me>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

static HRESULT
SHELL_GetShellExtensionRegCLSID(HKEY hKey, LPCWSTR KeyName, CLSID &clsid)
{
    // First try the key name
    if (SUCCEEDED(SHCLSIDFromStringW(KeyName, &clsid)))
        return S_OK;
    WCHAR buf[42];
    DWORD cb = sizeof(buf);
    // and then the default value
    DWORD err = RegGetValueW(hKey, KeyName, NULL, RRF_RT_REG_SZ, NULL, buf, &cb);
    return !err ? SHCLSIDFromStringW(buf, &clsid) : HRESULT_FROM_WIN32(err);
}

static HRESULT
SHELL_InitializeExtension(REFCLSID clsid, PCIDLIST_ABSOLUTE pidlFolder, IDataObject *pDO, HKEY hkeyProgID, REFIID riid, void **ppv)
{
    *ppv = NULL;
    IUnknown *pUnk;
    HRESULT hr = SHCoCreateInstance(NULL, &clsid, NULL, riid, (void**)&pUnk);
    if (SUCCEEDED(hr))
    {
        CComPtr<IShellExtInit> Init;
        hr = pUnk->QueryInterface(IID_PPV_ARG(IShellExtInit, &Init));
        if (SUCCEEDED(hr))
            hr = Init->Initialize(pidlFolder, pDO, hkeyProgID);

        if (SUCCEEDED(hr))
            *ppv = (void*)pUnk;
        else
            pUnk->Release();
    }
    return hr;
}

static HRESULT
AddPropSheetHandlerPages(REFCLSID clsid, IDataObject *pDO, HKEY hkeyProgID, PROPSHEETHEADERW &psh)
{
    CComPtr<IShellPropSheetExt> SheetExt;
    HRESULT hr = SHELL_InitializeExtension(clsid, NULL, pDO, hkeyProgID, IID_PPV_ARG(IShellPropSheetExt, &SheetExt));
    if (SUCCEEDED(hr))
    {
        UINT OldCount = psh.nPages;
        hr = SheetExt->AddPages(AddPropSheetPageCallback, (LPARAM)&psh);
        // The returned index is one-based (relative to this extension).
        // See https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-ishellpropsheetext-addpages
        if (hr > 0)
        {
            hr += OldCount;
            psh.nStartPage = hr - 1;
        }
    }
    return hr;
}

static HRESULT
SHELL_CreatePropSheetStubWindow(CStubWindow32 &stub, PCIDLIST_ABSOLUTE pidl, const POINT *pPt)
{
    PWSTR Path;
    if (!pidl || FAILED(SHGetNameFromIDList(pidl, SIGDN_DESKTOPABSOLUTEPARSING, &Path)))
        Path = NULL; // If we can't get a path, we simply will not be able to reuse this window

    HRESULT hr = stub.CreateStub(CStubWindow32::TYPE_PROPERTYSHEET, Path, pPt);
    SHFree(Path);
    UINT flags = SHGFI_ICON | SHGFI_ADDOVERLAYS;
    SHFILEINFO sfi;
    if (SUCCEEDED(hr) && SHGetFileInfoW((LPWSTR)pidl, 0, &sfi, sizeof(sfi), SHGFI_PIDL | flags))
        stub.SendMessage(WM_SETICON, ICON_BIG, (LPARAM)sfi.hIcon);
    return hr;
}

/*************************************************************************
 *  SHELL32_PropertySheet [INTERNAL]
 *  PropertySheetW with stub window.
 */
static INT_PTR
SHELL32_PropertySheet(LPPROPSHEETHEADERW ppsh, IDataObject *pDO)
{
    CStubWindow32 stub;
    POINT pt, *ppt = NULL;
    if (pDO && SUCCEEDED(DataObject_GetOffset(pDO, &pt)))
        ppt = &pt;
    PIDLIST_ABSOLUTE pidl = SHELL_DataObject_ILCloneFullItem(pDO, 0);
    HRESULT hr = SHELL_CreatePropSheetStubWindow(stub, pidl, ppt);
    ILFree(pidl);
    if (FAILED(hr))
        return hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) ? 0 : -1;

    INT_PTR Result = -1;
    ppsh->hwndParent = stub;
    if (ppsh->nPages)
    {
        Result = PropertySheetW(ppsh);
    }
    else
    {
        WCHAR szFormat[128], szMessage[_countof(szFormat) + 42];
        LoadStringW(shell32_hInstance, IDS_CANTSHOWPROPERTIES, szFormat, _countof(szFormat));
        wsprintfW(szMessage, szFormat, ERROR_CAN_NOT_COMPLETE);
        MessageBoxW(NULL, szMessage, NULL, MB_ICONERROR);
    }
    stub.DestroyWindow();
    return Result;
}

/*************************************************************************
 *  SHELL32_OpenPropSheet [INTERNAL]
 *  The real implementation of SHOpenPropSheetW.
 */
static BOOL
SHELL32_OpenPropSheet(LPCWSTR pszCaption, HKEY *ahKeys, UINT cKeys,
                      const CLSID *pclsidDefault, IDataObject *pDO, LPCWSTR pStartPage)
{
    HKEY hKeyProgID = cKeys ? ahKeys[cKeys - 1] : NULL; // Windows uses the last key for some reason
    HPROPSHEETPAGE hppages[MAX_PROPERTY_SHEET_PAGE];
    PROPSHEETHEADERW psh = { sizeof(psh), PSH_PROPTITLE };
    psh.phpage = hppages;
    psh.hInstance = shell32_hInstance;
    psh.pszCaption = pszCaption;
    psh.pStartPage = pStartPage;

    if (pclsidDefault)
        AddPropSheetHandlerPages(*pclsidDefault, pDO, hKeyProgID, psh);

    for (UINT i = 0; i < cKeys; ++i)
    {
        // Note: We can't use SHCreatePropSheetExtArrayEx because we need the AddPages() return value (see AddPropSheetHandlerPages).
        HKEY hKey;
        if (RegOpenKeyExW(ahKeys[i], L"shellex\\PropertySheetHandlers", 0, KEY_ENUMERATE_SUB_KEYS, &hKey))
            continue;
        for (UINT index = 0;; ++index)
        {
            WCHAR KeyName[MAX_PATH];
            LRESULT err = RegEnumKeyW(hKey, index, KeyName, _countof(KeyName));
            KeyName[_countof(KeyName)-1] = UNICODE_NULL;
            if (err == ERROR_MORE_DATA)
                continue;
            if (err)
                break;
            CLSID clsid;
            if (SUCCEEDED(SHELL_GetShellExtensionRegCLSID(hKey, KeyName, clsid)))
                AddPropSheetHandlerPages(clsid, pDO, hKeyProgID, psh);
        }
        RegCloseKey(hKey);
    }

    if (pStartPage == psh.pStartPage && pStartPage)
        psh.dwFlags |= PSH_USEPSTARTPAGE;
    INT_PTR Result = SHELL32_PropertySheet(&psh, pDO);
    return (Result != -1);
}

/*************************************************************************
 *  SHOpenPropSheetW [SHELL32.80]
 *
 * @see https://learn.microsoft.com/en-us/windows/win32/api/shlobj/nf-shlobj-shopenpropsheetw
 */
EXTERN_C BOOL WINAPI
SHOpenPropSheetW(
    _In_opt_ LPCWSTR pszCaption,
    _In_opt_ HKEY *ahKeys,
    _In_ UINT cKeys,
    _In_ const CLSID *pclsidDefault,
    _In_ IDataObject *pDataObject,
    _In_opt_ IShellBrowser *pShellBrowser,
    _In_opt_ LPCWSTR pszStartPage)
{
    UNREFERENCED_PARAMETER(pShellBrowser); /* MSDN says "Not used". */
    return SHELL32_OpenPropSheet(pszCaption, ahKeys, cKeys, pclsidDefault, pDataObject, pszStartPage);
}

/*************************************************************************
 * SH_CreatePropertySheetPage [Internal]
 *
 * creates a property sheet page from a resource id
 */
HPROPSHEETPAGE
SH_CreatePropertySheetPageEx(WORD wDialogId, DLGPROC pfnDlgProc, LPARAM lParam,
                             LPCWSTR pwszTitle, LPFNPSPCALLBACK Callback)
{
    PROPSHEETPAGEW Page = { sizeof(Page), PSP_DEFAULT, shell32_hInstance };
    Page.pszTemplate = MAKEINTRESOURCEW(wDialogId);
    Page.pfnDlgProc = pfnDlgProc;
    Page.lParam = lParam;
    Page.pszTitle = pwszTitle;
    Page.pfnCallback = Callback;

    if (pwszTitle)
        Page.dwFlags |= PSP_USETITLE;

    if (Callback)
        Page.dwFlags |= PSP_USECALLBACK;

    return CreatePropertySheetPageW(&Page);
}

HPROPSHEETPAGE
SH_CreatePropertySheetPage(WORD wDialogId, DLGPROC pfnDlgProc, LPARAM lParam, LPCWSTR pwszTitle)
{
    return SH_CreatePropertySheetPageEx(wDialogId, pfnDlgProc, lParam, pwszTitle, NULL);
}
