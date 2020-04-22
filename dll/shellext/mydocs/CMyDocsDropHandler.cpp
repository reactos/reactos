/*
 * PROJECT:     sendmail
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     MyDocs implementation
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.hpp"

WINE_DEFAULT_DEBUG_CHANNEL(sendmail);

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

    *pdwEffect &= DROPEFFECT_LINK;

    return S_OK;
}

STDMETHODIMP
CMyDocsDropHandler::DragOver(DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
{
    TRACE("(%p)\n", this);

    *pdwEffect &= DROPEFFECT_LINK;

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

    STGMEDIUM stg;
    FORMATETC etc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    HRESULT hr = pDataObject->GetData(&etc, &stg);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        *pdwEffect = 0;
        DragLeave();
        return E_FAIL;
    }

    // "My Documents"
    WCHAR szDir[MAX_PATH + 1];
    SHGetSpecialFolderPathW(NULL, szDir, CSIDL_PERSONAL, FALSE);
    szDir[lstrlenW(szDir) + 1] = 0;

    HDROP hDrop = reinterpret_cast<HDROP>(stg.hGlobal);
    UINT cItems = ::DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);

    CStringW strSrcList;
    WCHAR szSrc[MAX_PATH];
    for (UINT iItem = 0; iItem < cItems; ++iItem)
    {
        DragQueryFileW(hDrop, iItem, szSrc, MAX_PATH);

        if (!PathFileExistsW(szSrc))
        {
            *pdwEffect = 0;
            DragLeave();
            return E_FAIL;
        }

        if (iItem > 0)
            strSrcList += L"|";
        strSrcList = szSrc;
    }
    strSrcList += L"|";
    strSrcList += L"|";

    for (INT i = 0, cch = strSrcList.GetLength(); i < cch; ++i)
    {
        // because strSrcList[i] is constant, we have to do workaround...
        if (strSrcList[i] == L'|')
            memcpy(const_cast<WCHAR *>(&strSrcList[i]), L"\0", sizeof(WCHAR));
    }

    SHFILEOPSTRUCTW fileop = { NULL };
    fileop.wFunc = FO_COPY;
    fileop.pFrom = strSrcList;
    fileop.pTo = szDir;
    fileop.fFlags = FOF_ALLOWUNDO | FOF_FILESONLY | FOF_MULTIDESTFILES | FOF_NOCONFIRMMKDIR;
    SHFileOperationW(&fileop);

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
