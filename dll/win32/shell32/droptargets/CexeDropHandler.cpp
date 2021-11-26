/*
 * executable drop target handler
 *
 * Copyright 2014              Huw Campbell
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

WINE_DEFAULT_DEBUG_CHANNEL (shell);

CExeDropHandler::CExeDropHandler()
{
    pclsid = (CLSID *)&CLSID_ExeDropHandler;
}

CExeDropHandler::~CExeDropHandler()
{

}

// IDropTarget
HRESULT WINAPI CExeDropHandler::DragEnter(IDataObject *pDataObject, DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
{
    TRACE ("(%p)\n", this);
    if (*pdwEffect == DROPEFFECT_NONE)
        return S_OK;

    *pdwEffect = DROPEFFECT_COPY;
    return S_OK;
}

HRESULT WINAPI CExeDropHandler::DragOver(DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
{
    TRACE ("(%p)\n", this);
    *pdwEffect = DROPEFFECT_COPY;
    return S_OK;
}

HRESULT WINAPI CExeDropHandler::DragLeave()
{
    TRACE ("(%p)\n", this);
    return S_OK;
}

HRESULT WINAPI CExeDropHandler::Drop(IDataObject *pDataObject, DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
{
    TRACE ("(%p)\n", this);
    FORMATETC fmt;
    STGMEDIUM medium;
    LPWSTR pszSrcList;
    InitFormatEtc (fmt, CF_HDROP, TYMED_HGLOBAL);
    WCHAR wszBuf[MAX_PATH * 2 + 8], *pszEnd = wszBuf;
    size_t cchRemaining = _countof(wszBuf);

    if (SUCCEEDED(pDataObject->GetData(&fmt, &medium)) /* && SUCCEEDED(pDataObject->GetData(&fmt2, &medium))*/)
    {
        LPDROPFILES lpdf = (LPDROPFILES) GlobalLock(medium.hGlobal);
        if (!lpdf)
        {
            ERR("Error locking global\n");
            return E_FAIL;
        }
        pszSrcList = (LPWSTR) (((byte*) lpdf) + lpdf->pFiles);
        while (*pszSrcList)
        {
            if (StrChrW(pszSrcList, L' ') && cchRemaining > 3)
                StringCchPrintfExW(pszEnd, cchRemaining, &pszEnd, &cchRemaining, 0, L"\"%ls\" ", pszSrcList);
            else
                StringCchPrintfExW(pszEnd, cchRemaining, &pszEnd, &cchRemaining, 0, L"%ls ", pszSrcList);

            pszSrcList += wcslen(pszSrcList) + 1;
        }
    }

    ShellExecuteW(NULL, L"open", sPathTarget, wszBuf, NULL,SW_SHOWNORMAL);

    return S_OK;
}


// IPersistFile
HRESULT WINAPI CExeDropHandler::GetCurFile(LPOLESTR *ppszFileName)
{
    FIXME ("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CExeDropHandler::IsDirty()
{
    FIXME ("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CExeDropHandler::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
    UINT len = strlenW(pszFileName);
    sPathTarget = (WCHAR *)SHAlloc((len + 1) * sizeof(WCHAR));
    memcpy(sPathTarget, pszFileName, (len + 1) * sizeof(WCHAR));
    return S_OK;
}

HRESULT WINAPI CExeDropHandler::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
    FIXME ("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CExeDropHandler::SaveCompleted(LPCOLESTR pszFileName)
{
    FIXME ("(%p)\n", this);
    return E_NOTIMPL;
}

/************************************************************************
 * CFSFolder::GetClassID
 */
HRESULT WINAPI CExeDropHandler::GetClassID(CLSID * lpClassId)
{
    TRACE ("(%p)\n", this);

    if (!lpClassId)
        return E_POINTER;

    *lpClassId = *pclsid;

    return S_OK;
}