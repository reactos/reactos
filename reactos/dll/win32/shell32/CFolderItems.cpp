/*
 * FolderItem(s) implementation
 *
 * Copyright 2015,2016 Mark Jansen
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
    :m_Count(-1)
{
}

CFolderItems::~CFolderItems()
{
}

HRESULT CFolderItems::Init(LPITEMIDLIST idlist)
{
    CComPtr<IShellFolder> psfDesktop, psfTarget;

    m_idlist.Attach(idlist);

    HRESULT hr = SHGetDesktopFolder(&psfDesktop);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = psfDesktop->BindToObject(m_idlist, NULL, IID_PPV_ARG(IShellFolder, &psfTarget));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = psfTarget->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &m_EnumIDList);

    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return S_OK;
}

// *** FolderItems methods ***
HRESULT STDMETHODCALLTYPE CFolderItems::get_Count(long *plCount)
{
    if (!m_EnumIDList)
        return E_FAIL;

    if (!plCount)
        return E_POINTER;

    if (m_Count == -1)
    {
        long count = 0;

        HRESULT hr = m_EnumIDList->Reset();
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        CComHeapPtr<ITEMIDLIST> Pidl;
        hr = m_EnumIDList->Next(1, &Pidl, 0);
        while (hr != S_FALSE)
        {
            count++;
            Pidl.Free();
        }
        m_Count = count;
    }
    *plCount = m_Count;

    return S_OK;
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
    if (!m_EnumIDList)
        return E_FAIL;

    if (V_VT(&index) != VT_I4 && V_VT(&index) != VT_UI4)
        return E_INVALIDARG;

    ULONG count = V_UI4(&index);

    HRESULT hr = m_EnumIDList->Reset();
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = m_EnumIDList->Skip(count);

    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComHeapPtr<ITEMIDLIST> spPidl;
    hr = m_EnumIDList->Next(1, &spPidl, 0);
    if (hr == S_OK)
    {
        CFolderItem* item = new CComObject<CFolderItem>();
        item->AddRef();
        item->Init(spPidl.Detach());
        *ppid = item;
        return S_OK;
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE CFolderItems::_NewEnum(IUnknown **ppunk)
{
    CFolderItems* items = new CComObject<CFolderItems>();
    items->AddRef();
    items->Init(ILClone(m_idlist));
    *ppunk = items;
    return S_OK;
}

