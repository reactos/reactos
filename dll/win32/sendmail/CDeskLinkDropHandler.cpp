/*
 * PROJECT:     sendmail
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     DeskLink implementation
 * COPYRIGHT:   Copyright 2019 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.hpp"

WINE_DEFAULT_DEBUG_CHANNEL(sendmail);

/* initialisation for FORMATETC */
#define InitFormatEtc(fe, cf, med) do { \
    (fe).cfFormat = (cf); \
    (fe).dwAspect = DVASPECT_CONTENT; \
    (fe).ptd = NULL; \
    (fe).tymed = (med); \
    (fe).lindex = -1; \
} while(0)

CDeskLinkDropHandler::CDeskLinkDropHandler()
{
}

CDeskLinkDropHandler::~CDeskLinkDropHandler()
{
}

// IDropTarget
STDMETHODIMP
CDeskLinkDropHandler::DragEnter(IDataObject *pDataObject, DWORD dwKeyState,
                                POINTL pt, DWORD *pdwEffect)
{
    TRACE("(%p)\n", this);
    if (*pdwEffect == DROPEFFECT_NONE)
        return S_OK;

    *pdwEffect = DROPEFFECT_COPY;
    return S_OK;
}

STDMETHODIMP
CDeskLinkDropHandler::DragOver(DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
{
    TRACE("(%p)\n", this);
    *pdwEffect = DROPEFFECT_COPY;
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
    InitFormatEtc(fmt, CF_HDROP, TYMED_HGLOBAL);

    WCHAR szDir[MAX_PATH], szPath[MAX_PATH];
    SHGetSpecialFolderPathW(NULL, szDir, CSIDL_DESKTOPDIRECTORY, FALSE);

    CStringW strShortcut(MAKEINTRESOURCEW(IDS_SHORTCUT));
    strShortcut += L".lnk";

    STGMEDIUM medium;
    HRESULT hr = pDataObject->GetData(&fmt, &medium);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    const DROPFILES *lpdf = reinterpret_cast<LPDROPFILES>(GlobalLock(medium.hGlobal));
    if (!lpdf)
    {
        ERR("Error locking global\n");
        ReleaseStgMedium(&medium);
        return E_FAIL;
    }

    const BYTE *pb = reinterpret_cast<const BYTE *>(lpdf);
    pb += lpdf->pFiles;

    LPCWSTR psz = reinterpret_cast<LPCWSTR>(pb);
    while (*psz)
    {
        LPWSTR pszFileTitle = PathFindFileNameW(psz);

        StringCbCopyW(szPath, sizeof(szPath), szDir);
        PathAppendW(szPath, pszFileTitle);
        *PathFindExtensionW(szPath) = 0;
        StringCbCatW(szPath, sizeof(szPath), strShortcut);

        hr = CreateShellLink(szPath, psz, NULL, NULL, NULL, -1, NULL);
        if (FAILED_UNEXPECTEDLY(hr))
            break;

        psz += wcslen(psz) + 1;
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
