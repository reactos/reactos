/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     FolderItem(s) implementation
 * COPYRIGHT:   Copyright 2015-2018 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);


CFolderItem::CFolderItem()
{
}

CFolderItem::~CFolderItem()
{
}

HRESULT CFolderItem::Initialize(Folder* folder, LPITEMIDLIST idlist)
{
    m_idlist.Attach(ILClone(idlist));
    m_Folder = folder;
    return S_OK;
}

// *** FolderItem methods ***
HRESULT STDMETHODCALLTYPE CFolderItem::get_Application(IDispatch **ppid)
{
    TRACE("(%p, %p)\n", this, ppid);
    return m_Folder->get_Application(ppid);
}

HRESULT STDMETHODCALLTYPE CFolderItem::get_Parent(IDispatch **ppid)
{
    TRACE("(%p, %p)\n", this, ppid);
    if (ppid)
    {
        *ppid = m_Folder;
        m_Folder->AddRef();
    }
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItem::get_Name(BSTR *pbs)
{
    TRACE("(%p, %p)\n", this, pbs);

    *pbs = NULL;

    CComPtr<IShellFolder2> Parent;
    LPCITEMIDLIST last_part;
    HRESULT hr = SHBindToParent(m_idlist, IID_PPV_ARG(IShellFolder2, &Parent), &last_part);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    STRRET strret;
    hr = Parent->GetDisplayNameOf(last_part, SHGDN_INFOLDER, &strret);
    if (!FAILED_UNEXPECTEDLY(hr))
        hr = StrRetToBSTR(&strret, last_part, pbs);

    return hr;
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

HRESULT CFolderItems::Initialize(LPITEMIDLIST idlist, Folder* parent)
{
    CComPtr<IShellFolder> psfDesktop, psfTarget;

    m_idlist.Attach(ILClone(idlist));

    HRESULT hr = SHGetDesktopFolder(&psfDesktop);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = psfDesktop->BindToObject(m_idlist, NULL, IID_PPV_ARG(IShellFolder, &psfTarget));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = psfTarget->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &m_EnumIDList);

    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    m_Folder = parent;
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
        while ((hr = m_EnumIDList->Next(1, &Pidl, 0)) != S_FALSE)
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
    return m_Folder->get_Application(ppid);
}

HRESULT STDMETHODCALLTYPE CFolderItems::get_Parent(IDispatch **ppid)
{
    TRACE("(%p, %p)\n", this, ppid);

    if (ppid)
        *ppid = NULL;

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItems::Item(VARIANT var, FolderItem **ppid)
{
    CComVariant index;
    HRESULT hr;

    if (!m_EnumIDList)
        return E_FAIL;

    hr = VariantCopyInd(&index, &var);
    if (FAILED(hr))
        return hr;

    if (V_VT(&index) == VT_I2)
        VariantChangeType(&index, &index, 0, VT_I4);

    if (V_VT(&index) == VT_I4)
    {
        ULONG count = V_UI4(&index);

        hr = m_EnumIDList->Reset();
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        hr = m_EnumIDList->Skip(count);

        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        CComHeapPtr<ITEMIDLIST> spPidl;
        hr = m_EnumIDList->Next(1, &spPidl, 0);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
        hr = ShellObjectCreatorInit<CFolderItem>(m_Folder, static_cast<LPITEMIDLIST>(spPidl), IID_PPV_ARG(FolderItem, ppid));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
        return hr;
    }
    else if (V_VT(&index) == VT_BSTR)
    {
        if (!V_BSTR(&index))
            return S_FALSE;

        hr = m_Folder->ParseName(V_BSTR(&index), ppid);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
        return hr;
    }

    FIXME("Index type %d not handled.\n", V_VT(&index));
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItems::_NewEnum(IUnknown **ppunk)
{
    return ShellObjectCreatorInit<CFolderItems>(static_cast<LPITEMIDLIST>(m_idlist), m_Folder, IID_FolderItems, reinterpret_cast<void**>(ppunk));
}

