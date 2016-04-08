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

// *** IDispatch methods ***
HRESULT STDMETHODCALLTYPE CFolder::GetTypeInfoCount(UINT *pctinfo)
{
    TRACE("(%p, %p)\n", this, pctinfo);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolder::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    TRACE("(%p, %lu, %lu, %p)\n", this, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolder::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    TRACE("(%p, %s, %p, %lu, %lu, %p)\n", this, wine_dbgstr_guid(&riid), rgszNames, cNames, lcid, rgDispId);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolder::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    TRACE("(%p, %lu, %s, %lu, %lu, %p, %p, %p, %p)\n", this, dispIdMember, wine_dbgstr_guid(&riid), lcid, (DWORD)wFlags,
        pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}


// *** Folder methods ***
HRESULT STDMETHODCALLTYPE CFolder::get_Title(BSTR *pbs)
{
    TRACE("(%p, %p)\n", this, pbs);
    return E_NOTIMPL;
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
    TRACE("(%p, %p)\n", this, ppid);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolder::ParseName(BSTR bName, FolderItem **ppid)
{
    TRACE("(%p, %s, %p)\n", this, wine_dbgstr_w(bName), ppid);

    CComPtr<IShellFolder> psfDesktop;

    HRESULT hr = SHGetDesktopFolder(&psfDesktop);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComPtr<IShellFolder> psfCurrent;
    hr = psfDesktop->BindToObject(m_idlist, NULL, IID_PPV_ARG(IShellFolder, &psfCurrent));
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


