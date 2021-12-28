/*
 * PROJECT:     mydocs
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     MyDocs implementation
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.hpp"

WINE_DEFAULT_DEBUG_CHANNEL(mydocs);

CMyDocsDropHandler::CMyDocsDropHandler()
{
    InterlockedIncrement(&g_ModuleRefCnt);
}

CMyDocsDropHandler::~CMyDocsDropHandler()
{
    InterlockedDecrement(&g_ModuleRefCnt);
}

// IDropTarget
STDMETHODIMP
CMyDocsDropHandler::DragEnter(IDataObject *pDataObject, DWORD dwKeyState,
                                POINTL pt, DWORD *pdwEffect)
{
    TRACE("(%p)\n", this);

    *pdwEffect &= DROPEFFECT_COPY; // Copy only

    return S_OK;
}

STDMETHODIMP
CMyDocsDropHandler::DragOver(DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
{
    TRACE("(%p)\n", this);

    *pdwEffect &= DROPEFFECT_COPY; // Copy only

    return S_OK;
}

STDMETHODIMP CMyDocsDropHandler::DragLeave()
{
    TRACE("(%p)\n", this);
    return S_OK;
}

STDMETHODIMP
CMyDocsDropHandler::Drop(IDataObject *pDataObject, DWORD dwKeyState,
                         POINTL pt, DWORD *pdwEffect)
{
    TRACE("(%p)\n", this);

    if (!pDataObject)
    {
        ERR("pDataObject is NULL\n");
        *pdwEffect = 0;
        DragLeave();
        return E_POINTER;
    }

    CComPtr<IShellFolder> pDesktop;
    HRESULT hr = SHGetDesktopFolder(&pDesktop);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    // Retrieve an HIDA (handle of IDA)
    CDataObjectHIDA pida(pDataObject);
    if (FAILED_UNEXPECTEDLY(pida.hr()))
    {
        *pdwEffect = 0;
        DragLeave();
        return pida.hr();
    }

    UINT iItem, cItems = pida->cidl;

    // get the path of "My Documents"
    WCHAR szzDir[MAX_PATH + 1];
    SHGetSpecialFolderPathW(NULL, szzDir, CSIDL_PERSONAL, FALSE);
    szzDir[lstrlenW(szzDir) + 1] = 0; // ends with double NULs

    // for all source items
    CStringW strSrcList;
    WCHAR szSrc[MAX_PATH];
    PCIDLIST_ABSOLUTE pidlParent = HIDA_GetPIDLFolder(pida);
    for (iItem = 0; iItem < cItems; ++iItem)
    {
        // query source pidl
        PCITEMID_CHILD pidlChild = HIDA_GetPIDLItem(pida, iItem);
        CComHeapPtr<ITEMIDLIST> pidl(ILCombine(pidlParent, pidlChild));

        // can get path?
        szSrc[0] = 0;
        if (!SHGetPathFromIDListW(pidl, szSrc))
        {
            // try to retrieve path from desktop
            STRRET strret;
            hr = pDesktop->GetDisplayNameOf(pidl, SHGDN_INFOLDER, &strret);
            if (FAILED_UNEXPECTEDLY(hr))
                break;
            hr = StrRetToBufW(&strret, pidl, szSrc, _countof(szSrc));
            if (FAILED_UNEXPECTEDLY(hr))
                break;
            if (!PathFileExistsW(szSrc))
                break;
        }

        if (iItem > 0)
            strSrcList += L'|'; // separator is '|'
        strSrcList += szSrc;
    }

    if (iItem != cItems)
    {
        // source not found
        CStringW strText;
        strText.Format(IDS_NOSRCFILEFOUND, szSrc[0] ? szSrc : L"(null)");
        MessageBoxW(NULL, strText, NULL, MB_ICONERROR);

        *pdwEffect = 0;
        DragLeave();
        return E_FAIL;
    }

    strSrcList += L"||"; // double separators

    // lock the buffer
    LPWSTR pszzSrcList = strSrcList.GetBuffer();

    // convert every separator to a NUL
    INT cch = strSrcList.GetLength();
    for (INT i = 0; i < cch; ++i)
    {
        if (pszzSrcList[i] == L'|')
            pszzSrcList[i] = L'\0';
    }

    // copy them
    SHFILEOPSTRUCTW fileop = { NULL };
    fileop.wFunc = FO_COPY;
    fileop.pFrom = pszzSrcList;
    fileop.pTo = szzDir;
    fileop.fFlags = FOF_ALLOWUNDO | FOF_FILESONLY | FOF_MULTIDESTFILES | FOF_NOCONFIRMMKDIR;
    int res = SHFileOperationW(&fileop);
    if (res)
    {
        ERR("SHFileOperationW failed with 0x%x\n", res);
        hr = E_FAIL;
    }

    // unlock buffer
    strSrcList.ReleaseBuffer();

    DragLeave();
    return hr;
}

// IPersistFile
STDMETHODIMP CMyDocsDropHandler::GetCurFile(LPOLESTR *ppszFileName)
{
    FIXME("(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CMyDocsDropHandler::IsDirty()
{
    FIXME("(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CMyDocsDropHandler::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
    return S_OK;
}

STDMETHODIMP CMyDocsDropHandler::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
    FIXME("(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CMyDocsDropHandler::SaveCompleted(LPCOLESTR pszFileName)
{
    FIXME("(%p)\n", this);
    return E_NOTIMPL;
}

// IPersist
STDMETHODIMP CMyDocsDropHandler::GetClassID(CLSID * lpClassId)
{
    TRACE("(%p)\n", this);

    if (!lpClassId)
    {
        ERR("lpClassId is NULL\n");
        return E_POINTER;
    }

    *lpClassId = CLSID_MyDocsDropHandler;

    return S_OK;
}
