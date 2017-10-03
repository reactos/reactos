/*
 * Folder implementation
 *
 * Copyright 2015 Mark Jansen
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

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);


CFolder::CFolder()
{
}

CFolder::~CFolder()
{
}

void CFolder::Init(LPITEMIDLIST idlist)
{
    m_idlist.Attach(idlist);
}

HRESULT CFolder::GetShellFolder(CComPtr<IShellFolder>& psfCurrent)
{
    CComPtr<IShellFolder> psfDesktop;

    HRESULT hr = SHGetDesktopFolder(&psfDesktop);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return psfDesktop->BindToObject(m_idlist, NULL, IID_PPV_ARG(IShellFolder, &psfCurrent));
}

// *** Folder methods ***
HRESULT STDMETHODCALLTYPE CFolder::get_Title(BSTR *pbs)
{
    if (!pbs)
        return E_POINTER;

    WCHAR path[MAX_PATH+2] = {0};
    HRESULT hr = ILGetDisplayNameExW(NULL, m_idlist, path, ILGDN_INFOLDER);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    *pbs = SysAllocString(path);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CFolder::get_Application(IDispatch **ppid)
{
    TRACE("(%p, %p)\n", this, ppid);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolder::get_Parent(IDispatch **ppid)
{
    TRACE("(%p %p)\n", this, ppid);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolder::get_ParentFolder(Folder **ppsf)
{
    TRACE("(%p, %p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolder::Items(FolderItems **ppid)
{
    CFolderItems* items = new CComObject<CFolderItems>();
    items->AddRef();

    HRESULT hr = items->Init(ILClone(m_idlist));
    if (FAILED_UNEXPECTEDLY(hr))
    {
        items->Release();
        return hr;
    }

    *ppid = items;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CFolder::ParseName(BSTR bName, FolderItem **ppid)
{
    TRACE("(%p, %s, %p)\n", this, wine_dbgstr_w(bName), ppid);
    if (!ppid)
        return E_POINTER;
    *ppid = NULL;

    CComPtr<IShellFolder> psfCurrent;
    HRESULT hr = GetShellFolder(psfCurrent);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComHeapPtr<ITEMIDLIST_RELATIVE> relativePidl;
    hr = psfCurrent->ParseDisplayName(NULL, NULL, bName, NULL, &relativePidl, NULL);
    if (!SUCCEEDED(hr))
        return S_FALSE;

    CFolderItem* item = new CComObject<CFolderItem>();
    item->AddRef();
    item->Init(ILCombine(m_idlist, relativePidl));
    *ppid = item;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CFolder::NewFolder(BSTR bName, VARIANT vOptions)
{
    TRACE("(%p, %s, %s)\n", this, wine_dbgstr_w(bName), wine_dbgstr_variant(&vOptions));
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolder::MoveHere(VARIANT vItem, VARIANT vOptions)
{
    TRACE("(%p, %s, %s)\n", this, wine_dbgstr_variant(&vItem), wine_dbgstr_variant(&vOptions));
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolder::CopyHere(VARIANT vItem, VARIANT vOptions)
{
    TRACE("(%p, %s, %s)\n", this, wine_dbgstr_variant(&vItem), wine_dbgstr_variant(&vOptions));
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolder::GetDetailsOf(VARIANT vItem, int iColumn, BSTR *pbs)
{
    TRACE("(%p, %s, %i, %p)\n", this, wine_dbgstr_variant(&vItem), iColumn, pbs);
    return E_NOTIMPL;
}


// *** Folder2 methods ***
HRESULT STDMETHODCALLTYPE CFolder::get_Self(FolderItem **ppfi)
{
    TRACE("(%p, %p)\n", this, ppfi);
    if (!ppfi)
        return E_POINTER;
    CFolderItem* item = new CComObject<CFolderItem>();
    item->AddRef();
    item->Init(ILClone(m_idlist));
    *ppfi = item;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CFolder::get_OfflineStatus(LONG *pul)
{
    TRACE("(%p, %p)\n", this, pul);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolder::Synchronize()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolder::get_HaveToShowWebViewBarricade(VARIANT_BOOL *pbHaveToShowWebViewBarricade)
{
    TRACE("(%p, %p)\n", this, pbHaveToShowWebViewBarricade);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolder::DismissedWebViewBarricade()
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}


