/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     FileSystem PropertySheet implementation
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
        if (hr > 0) // The returned index is one-based (relative to this extension).
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

    HRESULT hr = CStubWindow32::CreateStub(stub, CStubWindow32::TYPE_PROPERTYSHEET, Path, pPt);
    SHFree(Path);
    UINT flags = SHGFI_ICON | SHGFI_ADDOVERLAYS;
    SHFILEINFO sfi;
    if (SUCCEEDED(hr) && SHGetFileInfoW((LPWSTR)pidl, 0, &sfi, sizeof(sfi), SHGFI_PIDL | flags))
        stub.SendMessage(WM_SETICON, ICON_BIG, (LPARAM)sfi.hIcon);
    return hr;
}

static HRESULT
SHELL_GetCaptionFromDataObject(IDataObject *pDO, LPWSTR Buf, UINT cchBuf)
{
    HRESULT hr = E_INVALIDARG;
    if (PIDLIST_ABSOLUTE pidl = SHELL_DataObject_ILCloneFullItem(pDO, 0))
    {
        hr = SHGetNameAndFlagsW(pidl, SHGDN_INFOLDER, Buf, cchBuf, NULL);
        ILFree(pidl);
    }
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
EXTERN_C BOOL
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
        // Note: We can't use SHCreatePropSheetExtArrayEx because we need the AddPages return value.
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

struct ShellPropSheetDialog
{
    typedef void (CALLBACK*PFNINITIALIZE)(LPCWSTR InitString, IDataObject *pDO,
                                          HKEY *hKeys, UINT *cKeys);

    static HRESULT Show(const CLSID *pClsidDefault, IDataObject *pDO,
                        PFNINITIALIZE InitFunc, LPCWSTR InitString)
    {
        HRESULT hr;
        CRegKeyArray keys;
        if (InitFunc)
            InitFunc(InitString, pDO, keys, keys);
        WCHAR szCaption[MAX_PATH], *pszCaption = NULL;
        if (SUCCEEDED(SHELL_GetCaptionFromDataObject(pDO, szCaption, _countof(szCaption))))
            pszCaption = szCaption;
        hr = SHOpenPropSheetW(pszCaption, keys, keys, pClsidDefault, pDO, NULL, NULL) ? S_OK : E_FAIL;
        return hr;
    }

    struct DATA
    {
        PFNINITIALIZE InitFunc;
        LPWSTR InitString;
        CLSID ClsidDefault;
        const CLSID *pClsidDefault;
        IStream *pObjStream;
    };

    static void FreeData(DATA *pData)
    {
        if (pData->InitString)
            SHFree(pData->InitString);
        SHFree(pData);
    }

    static HRESULT ShowAsync(const CLSID *pClsidDefault, IDataObject *pDO,
                             PFNINITIALIZE InitFunc, LPCWSTR InitString)
    {
        DATA *pData = (DATA*)SHAlloc(sizeof(*pData));
        if (!pData)
            return E_OUTOFMEMORY;
        ZeroMemory(pData, sizeof(*pData));
        pData->InitFunc = InitFunc;
        if (InitString && FAILED(SHStrDupW(InitString, &pData->InitString)))
        {
            FreeData(pData);
            return E_OUTOFMEMORY;
        }
        if (pClsidDefault)
        {
            pData->ClsidDefault = *pClsidDefault;
            pData->pClsidDefault = &pData->ClsidDefault;
        }

        HRESULT hr = S_OK;
        if (pDO)
            hr = CoMarshalInterThreadInterfaceInStream(IID_IDataObject, pDO, &pData->pObjStream);

        UINT flags = CTF_COINIT | CTF_PROCESS_REF | CTF_INSIST;
        if (SUCCEEDED(hr) && !SHCreateThread(ShowPropertiesThread, pData, flags, NULL))
        {
            if (pData->pObjStream)
                pData->pObjStream->Release();
            hr = E_FAIL;
        }
        if (FAILED(hr))
            FreeData(pData);
        return hr;
    }

    static DWORD CALLBACK ShowPropertiesThread(LPVOID Param)
    {
        DATA *pData = (DATA*) Param;
        CComPtr<IDataObject> pDO;
        if (pData->pObjStream)
            CoGetInterfaceAndReleaseStream(pData->pObjStream, IID_PPV_ARG(IDataObject, &pDO));
        Show(pData->pClsidDefault, pDO, pData->InitFunc, pData->InitString);
        FreeData(pData);
        return 0;
    }
};

static void CALLBACK
FSFolderItemPropDialogInitCallback(LPCWSTR InitString, IDataObject *pDO, HKEY *hKeys, UINT *cKeys)
{
    // Add file-type specific pages
    UNREFERENCED_PARAMETER(InitString);
    CDataObjectHIDA cida(pDO);
    if (SUCCEEDED(cida.hr()) && cida->cidl)
    {
        PCUITEMID_CHILD pidl = HIDA_GetPIDLItem(cida, 0);
        AddFSClassKeysToArray(1, &pidl, hKeys, cKeys);
    }
}

static void CALLBACK
ProgIdPropDialogInitCallback(LPCWSTR InitString, IDataObject *pDO, HKEY *hKeys, UINT *cKeys)
{
    // Add pages from HKCR\%ProgId% (with shellex\PropertySheetHandlers appended later)
    UNREFERENCED_PARAMETER(pDO);
    AddClassKeyToArray(InitString, hKeys, cKeys);
}

HRESULT
SHELL32_ShowFilesystemItemPropertiesDialogAsync(IDataObject *pDO)
{
    CDataObjectHIDA cida(pDO);
    HRESULT hr = cida.hr();
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    const CLSID *pClsid = NULL;
    ShellPropSheetDialog::PFNINITIALIZE InitFunc = NULL;
    LPCWSTR InitString = NULL;

    if (_ILIsDrive(HIDA_GetPIDLItem(cida, 0)))
    {
        pClsid = &CLSID_ShellDrvDefExt;
        InitFunc = ProgIdPropDialogInitCallback;
        InitString = L"Drive";
    }
    else
    {
        pClsid = &CLSID_ShellFileDefExt;
        InitFunc = FSFolderItemPropDialogInitCallback;
    }
    ShellPropSheetDialog Dialog;
    return Dialog.ShowAsync(pClsid, pDO, InitFunc, InitString);
}

HRESULT
SHELL32_ShowShellExtensionProperties(const CLSID *pClsid, IDataObject *pDO)
{
    WCHAR ClassBuf[6 + 38 + 1] = L"CLSID\\";
    StringFromGUID2(*pClsid, ClassBuf + 6, 38 + 1);
    ShellPropSheetDialog Dialog;
    return Dialog.ShowAsync(NULL, pDO, ProgIdPropDialogInitCallback, ClassBuf);
}
