/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     FileSystem PropertySheet implementation
 * COPYRIGHT:   Copyright 2024 Whindmar Saksit <whindsaks@proton.me>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

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

struct ShellPropSheetDialog
{
    typedef void (CALLBACK*PFNINITIALIZE)(LPCWSTR InitString, IDataObject *pDO,
                                          HKEY *hKeys, UINT *cKeys);

    static HRESULT Show(const CLSID *pClsidDefault, IDataObject *pDO,
                        PFNINITIALIZE InitFunc, LPCWSTR InitString)
    {
        HRESULT hr;
        CRegKeyHandleArray keys;
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
        HANDLE hEvent;
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
        HANDLE hEvent = pData->hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

        HRESULT hr = S_OK;
        if (pDO)
            hr = CoMarshalInterThreadInterfaceInStream(IID_IDataObject, pDO, &pData->pObjStream);

        const UINT flags = CTF_COINIT | CTF_PROCESS_REF | CTF_INSIST;
        if (SUCCEEDED(hr) && !SHCreateThread(ShowPropertiesThread, pData, flags, NULL))
        {
            if (pData->pObjStream)
                pData->pObjStream->Release();
            hr = E_FAIL;
        }

        if (SUCCEEDED(hr))
        {
            if (hEvent)
            {
                DWORD index;
                // Pump COM messages until the thread can create its own IDataObject (for CORE-19933).
                // SHOpenPropSheetW is modal and we cannot wait for it to complete.
                CoWaitForMultipleHandles(COWAIT_DEFAULT, INFINITE, 1, &hEvent, &index);
                CloseHandle(hEvent);
            }
        }
        else
        {
            FreeData(pData);
        }
        return hr;
    }

    static DWORD CALLBACK ShowPropertiesThread(LPVOID Param)
    {
        DATA *pData = (DATA*)Param;
        CComPtr<IDataObject> pDO, pLocalDO;
        if (pData->pObjStream)
            CoGetInterfaceAndReleaseStream(pData->pObjStream, IID_PPV_ARG(IDataObject, &pDO));
        if (pDO && SUCCEEDED(SHELL_CloneDataObject(pDO, &pLocalDO)))
            pDO = pLocalDO;
        if (pData->hEvent)
            SetEvent(pData->hEvent);
        Show(pData->pClsidDefault, pDO, pData->InitFunc, pData->InitString);
        FreeData(pData);
        return 0;
    }
};

static void CALLBACK
FSFolderItemPropDialogInitCallback(LPCWSTR InitString, IDataObject *pDO, HKEY *hKeys, UINT *cKeys)
{
    UNREFERENCED_PARAMETER(InitString);
    CDataObjectHIDA cida(pDO);
    if (SUCCEEDED(cida.hr()) && cida->cidl)
    {
        PCUITEMID_CHILD pidl = HIDA_GetPIDLItem(cida, 0);
        AddFSClassKeysToArray(1, &pidl, hKeys, cKeys); // Add file-type specific pages
    }
}

static void CALLBACK
ClassPropDialogInitCallback(LPCWSTR InitString, IDataObject *pDO, HKEY *hKeys, UINT *cKeys)
{
    UNREFERENCED_PARAMETER(pDO);
    AddClassKeyToArray(InitString, hKeys, cKeys); // Add pages from HKCR\%ProgId% (with shellex\PropertySheetHandlers appended later)
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
        InitFunc = ClassPropDialogInitCallback;
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
    return Dialog.ShowAsync(NULL, pDO, ClassPropDialogInitCallback, ClassBuf);
}

HRESULT
SHELL_ShowItemIDListProperties(LPCITEMIDLIST pidl)
{
    assert(pidl);

    CComHeapPtr<ITEMIDLIST> alloc;
    if (IS_INTRESOURCE(pidl))
    {
        HRESULT hr = SHGetSpecialFolderLocation(NULL, LOWORD(pidl), const_cast<LPITEMIDLIST*>(&pidl));
        if (FAILED(hr))
            return hr;
        alloc.Attach(const_cast<LPITEMIDLIST>(pidl));
    }
    SHELLEXECUTEINFOA sei = {
        sizeof(sei), SEE_MASK_INVOKEIDLIST | SEE_MASK_ASYNCOK, NULL, "properties",
        NULL, NULL, NULL, SW_SHOWNORMAL, NULL, const_cast<LPITEMIDLIST>(pidl)
    };
    return ShellExecuteExA(&sei) ? S_OK : HResultFromWin32(GetLastError());
}
