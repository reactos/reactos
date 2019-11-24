/*
 * desklink drop target handler
 *
 * Copyright 2019 Katayama Hirofumi MZ.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

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
    TRACE ("(%p)\n", this);
    if (*pdwEffect == DROPEFFECT_NONE)
        return S_OK;

    *pdwEffect = DROPEFFECT_COPY;
    return S_OK;
}

STDMETHODIMP
CDeskLinkDropHandler::DragOver(DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
{
    TRACE ("(%p)\n", this);
    *pdwEffect = DROPEFFECT_COPY;
    return S_OK;
}

STDMETHODIMP CDeskLinkDropHandler::DragLeave()
{
    TRACE ("(%p)\n", this);
    return S_OK;
}

STDMETHODIMP
CDeskLinkDropHandler::Drop(IDataObject *pDataObject, DWORD dwKeyState,
                           POINTL pt, DWORD *pdwEffect)
{
    TRACE ("(%p)\n", this);

    if (!pDataObject)
        return E_POINTER;

    FORMATETC fmt;
    InitFormatEtc(fmt, CF_HDROP, TYMED_HGLOBAL);

    WCHAR szDir[MAX_PATH], szPath[MAX_PATH];
    SHGetSpecialFolderPathW(NULL, szDir, CSIDL_DESKTOPDIRECTORY, FALSE);

    CStringW strShortcut(MAKEINTRESOURCEW(IDS_SHORTCUT));

    HRESULT hr = E_FAIL;
    STGMEDIUM medium;
    if (SUCCEEDED(pDataObject->GetData(&fmt, &medium)))
    {
        LPDROPFILES lpdf = (LPDROPFILES) GlobalLock(medium.hGlobal);
        if (!lpdf)
        {
            ERR("Error locking global\n");
            return E_FAIL;
        }

        LPBYTE pb = reinterpret_cast<LPBYTE>(lpdf);
        pb += lpdf->pFiles;

        LPWSTR psz = reinterpret_cast<LPWSTR>(pb);
        while (*psz)
        {
            LPWSTR pszFileTitle = PathFindFileNameW(psz);

            StringCbCopyW(szPath, sizeof(szPath), szDir);
            PathAppendW(szPath, pszFileTitle);
            *PathFindExtensionW(szPath) = 0;
            StringCbCatW(szPath, sizeof(szPath), strShortcut);

            hr = CreateShellLink(szPath, psz, NULL, NULL, NULL, -1, NULL);
            if (FAILED(hr))
                break;

            psz += lstrlenW(psz) + 1;
        }
    }

    return hr;
}

// IPersistFile
STDMETHODIMP CDeskLinkDropHandler::GetCurFile(LPOLESTR *ppszFileName)
{
    FIXME ("(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CDeskLinkDropHandler::IsDirty()
{
    FIXME ("(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CDeskLinkDropHandler::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
    return S_OK;
}

STDMETHODIMP CDeskLinkDropHandler::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
    FIXME ("(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CDeskLinkDropHandler::SaveCompleted(LPCOLESTR pszFileName)
{
    FIXME ("(%p)\n", this);
    return E_NOTIMPL;
}

// IPersist
STDMETHODIMP CDeskLinkDropHandler::GetClassID(CLSID * lpClassId)
{
    TRACE ("(%p)\n", this);

    if (!lpClassId)
        return E_POINTER;

    *lpClassId = CLSID_DeskLinkDropHandler;

    return S_OK;
}
