/*
 * PROJECT:     sendmail
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     DeskLink implementation
 * COPYRIGHT:   Copyright 2019 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.hpp"

WINE_DEFAULT_DEBUG_CHANNEL(sendmail);

CDeskLinkDropHandler::CDeskLinkDropHandler()
{
    InterlockedIncrement(&g_ModuleRefCnt);
}

CDeskLinkDropHandler::~CDeskLinkDropHandler()
{
    InterlockedDecrement(&g_ModuleRefCnt);
}

// IDropTarget
STDMETHODIMP
CDeskLinkDropHandler::DragEnter(IDataObject *pDataObject, DWORD dwKeyState,
                                POINTL pt, DWORD *pdwEffect)
{
    TRACE("(%p)\n", this);

    *pdwEffect &= DROPEFFECT_LINK;

    return S_OK;
}

STDMETHODIMP
CDeskLinkDropHandler::DragOver(DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
{
    TRACE("(%p)\n", this);

    *pdwEffect &= DROPEFFECT_LINK;

    return S_OK;
}

STDMETHODIMP CDeskLinkDropHandler::DragLeave()
{
    TRACE("(%p)\n", this);
    return S_OK;
}

STDMETHODIMP
CDeskLinkDropHandler::Drop(IDataObject *pDataObject, DWORD dwKeyState,
                           POINTL pt, DWORD *pdwEffect)
{
    TRACE("(%p)\n", this);

    if (!pDataObject)
    {
        ERR("pDataObject is NULL\n");
        return E_POINTER;
    }

    WCHAR szDir[MAX_PATH], szDest[MAX_PATH], szSrc[MAX_PATH];
    SHGetSpecialFolderPathW(NULL, szDir, CSIDL_DESKTOPDIRECTORY, FALSE);

    CComPtr<IShellFolder> pDesktop;
    HRESULT hr = SHGetDesktopFolder(&pDesktop);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CDataObjectHIDA pida(pDataObject);
    if (FAILED_UNEXPECTEDLY(pida.hr()))
        return pida.hr();

    LPCITEMIDLIST pidlParent = HIDA_GetPIDLFolder(pida);
    for (UINT i = 0; i < pida->cidl; ++i)
    {
        LPCITEMIDLIST pidlChild = HIDA_GetPIDLItem(pida, i);

        CComHeapPtr<ITEMIDLIST> pidl(ILCombine(pidlParent, pidlChild));
        if (!pidl)
        {
            ERR("Out of memory\n");
            break;
        }

        StringCbCopyW(szDest, sizeof(szDest), szDir);
        if (SHGetPathFromIDListW(pidl, szSrc))
        {
            CStringW strTitle;
            strTitle.Format(IDS_SHORTCUT, PathFindFileNameW(szSrc));

            PathAppendW(szDest, strTitle);
            PathRemoveExtensionW(szDest);
            StringCbCatW(szDest, sizeof(szDest), L".lnk");

            hr = CreateShellLink(szDest, szSrc, NULL, NULL, NULL, NULL, -1, NULL);
        }
        else
        {
            STRRET strret;
            hr = pDesktop->GetDisplayNameOf(pidl, SHGDN_INFOLDER, &strret);
            if (FAILED_UNEXPECTEDLY(hr))
                break;

            hr = StrRetToBufW(&strret, pidl, szSrc, _countof(szSrc));
            if (FAILED_UNEXPECTEDLY(hr))
                break;

            CStringW strTitle;
            strTitle.Format(IDS_SHORTCUT, szSrc);

            PathAppendW(szDest, strTitle);
            PathRemoveExtensionW(szDest);
            StringCbCatW(szDest, sizeof(szDest), L".lnk");

            hr = CreateShellLink(szDest, NULL, pidl, NULL, NULL, NULL, -1, NULL);
        }

        if (FAILED_UNEXPECTEDLY(hr))
            break;
    }

    return hr;
}

// IPersistFile
STDMETHODIMP CDeskLinkDropHandler::GetCurFile(LPOLESTR *ppszFileName)
{
    FIXME("(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CDeskLinkDropHandler::IsDirty()
{
    FIXME("(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CDeskLinkDropHandler::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
    return S_OK;
}

STDMETHODIMP CDeskLinkDropHandler::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
    FIXME("(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CDeskLinkDropHandler::SaveCompleted(LPCOLESTR pszFileName)
{
    FIXME("(%p)\n", this);
    return E_NOTIMPL;
}

// IPersist
STDMETHODIMP CDeskLinkDropHandler::GetClassID(CLSID * lpClassId)
{
    TRACE("(%p)\n", this);

    if (!lpClassId)
    {
        ERR("lpClassId is NULL\n");
        return E_POINTER;
    }

    *lpClassId = CLSID_DeskLinkDropHandler;

    return S_OK;
}
