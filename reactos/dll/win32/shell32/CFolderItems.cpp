/*
 * FolderItem(s) implementation
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


CFolderItem::CFolderItem()
{
}

CFolderItem::~CFolderItem()
{
}

void CFolderItem::Init(LPITEMIDLIST idlist)
{
    m_idlist.Attach(idlist);
}

// *** FolderItem methods ***
HRESULT STDMETHODCALLTYPE CFolderItem::get_Application(IDispatch **ppid)
{
    TRACE("(%p, %p)\n", this, ppid);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItem::get_Parent(IDispatch **ppid)
{
    TRACE("(%p, %p)\n", this, ppid);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItem::get_Name(BSTR *pbs)
{
    TRACE("(%p, %p)\n", this, pbs);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItem::put_Name(BSTR bs)
{
    TRACE("(%p, %s)\n", this, wine_dbgstr_w(bs));
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItem::get_Path(BSTR *pbs)
{
    CComPtr<IShellFolder> psfDesktop;

    HRESULT hr = SHGetDesktopFolder(&psfDesktop);
    if (!SUCCEEDED(hr))
        return hr;

    STRRET strret;
    hr = psfDesktop->GetDisplayNameOf(m_idlist, SHGDN_FORPARSING, &strret);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return StrRetToBSTR(&strret, NULL, pbs);
}

HRESULT STDMETHODCALLTYPE CFolderItem::get_GetLink(IDispatch **ppid)
{
    TRACE("(%p, %p)\n", this, ppid);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItem::get_GetFolder(IDispatch **ppid)
{
    TRACE("(%p, %p)\n", this, ppid);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItem::get_IsLink(VARIANT_BOOL *pb)
{
    TRACE("(%p, %p)\n", this, pb);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItem::get_IsFolder(VARIANT_BOOL *pb)
{
    TRACE("(%p, %p)\n", this, pb);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItem::get_IsFileSystem(VARIANT_BOOL *pb)
{
    TRACE("(%p, %p)\n", this, pb);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItem::get_IsBrowsable(VARIANT_BOOL *pb)
{
    TRACE("(%p, %p)\n", this, pb);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItem::get_ModifyDate(DATE *pdt)
{
    TRACE("(%p, %p)\n", this, pdt);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItem::put_ModifyDate(DATE dt)
{
    TRACE("(%p, %f)\n", this, dt);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItem::get_Size(LONG *pul)
{
    TRACE("(%p, %p)\n", this, pul);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItem::get_Type(BSTR *pbs)
{
    TRACE("(%p, %p)\n", this, pbs);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItem::Verbs(FolderItemVerbs **ppfic)
{
    if (!ppfic)
        return E_POINTER;
    CFolderItemVerbs* verbs = new CComObject<CFolderItemVerbs>();
    HRESULT hr = verbs->Init(m_idlist);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        delete verbs;
        return hr;
    }
    verbs->AddRef();
    *ppfic = verbs;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CFolderItem::InvokeVerb(VARIANT vVerb)
{
    TRACE("(%p, %s)\n", this, wine_dbgstr_variant(&vVerb));
    return E_NOTIMPL;
}



CFolderItems::CFolderItems()
{
}

CFolderItems::~CFolderItems()
{
}

// *** FolderItems methods ***
HRESULT STDMETHODCALLTYPE CFolderItems::get_Count(long *plCount)
{
    TRACE("(%p, %p)\n", this, plCount);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItems::get_Application(IDispatch **ppid)
{
    TRACE("(%p, %p)\n", this, ppid);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItems::get_Parent(IDispatch **ppid)
{
    TRACE("(%p, %p)\n", this, ppid);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItems::Item(VARIANT index, FolderItem **ppid)
{
    TRACE("(%p, %s, %p)\n", this, wine_dbgstr_variant(&index), ppid);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItems::_NewEnum(IUnknown **ppunk)
{
    TRACE("(%p, %p)\n", this, ppunk);
    return E_NOTIMPL;
}

