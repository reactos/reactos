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

    FORMATETC fmt;
    fmt.cfFormat = RegisterClipboardFormatW(CFSTR_SHELLIDLIST);
    fmt.ptd = NULL;
    fmt.dwAspect = DVASPECT_CONTENT;
    fmt.lindex = -1;
    fmt.tymed = TYMED_HGLOBAL;

    WCHAR szDir[MAX_PATH], szDest[MAX_PATH], szSrc[MAX_PATH];
    SHGetSpecialFolderPathW(NULL, szDir, CSIDL_DESKTOPDIRECTORY, FALSE);

    CComPtr<IShellFolder> pDesktop;
    HRESULT hr = SHGetDesktopFolder(&pDesktop);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    STGMEDIUM medium;
    hr = pDataObject->GetData(&fmt, &medium);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    LPIDA pida = reinterpret_cast<LPIDA>(GlobalLock(medium.hGlobal));
    if (!pida)
    {
        ERR("Error locking global\n");
        ReleaseStgMedium(&medium);
        return E_FAIL;
    }

    LPBYTE pb = reinterpret_cast<LPBYTE>(pida);
    LPCITEMIDLIST pidlParent = reinterpret_cast<LPCITEMIDLIST>(pb + pida->aoffset[0]);
    for (UINT i = 1; i <= pida->cidl; ++i)
    {
        LPCITEMIDLIST pidlChild = reinterpret_cast<LPCITEMIDLIST>(pb + pida->aoffset[i]);

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

    GlobalUnlock(medium.hGlobal);
    ReleaseStgMedium(&medium);

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
