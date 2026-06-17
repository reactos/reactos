/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     FolderItem(s) implementation
 * COPYRIGHT:   Copyright 2015-2018 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"
#include "prop.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);


CFolderItem::CFolderItem()
{
}

CFolderItem::~CFolderItem()
{
}

HRESULT CFolderItem::Initialize(Folder* folder, LPCITEMIDLIST idlist)
{
    m_idlist.Attach(ILClone(idlist));
    if (!m_idlist)
        return E_OUTOFMEMORY;
    m_Folder = folder;
    return S_OK;
}

inline HRESULT CFolderItem::GetParentShellFolderAndItem(REFIID riid, void**ppv, PCUITEMID_CHILD &pidlLast)
{
    return SHBindToParent(GetAbsoluteIDList(), riid, ppv, &pidlLast);
}

LPCITEMIDLIST CFolderItem::GetInternalPidlRef(IUnknown *pUnk)
{
    LPCITEMIDLIST pidl = NULL;
    FolderItem2 *pFI2;
    if (pUnk && SUCCEEDED(pUnk->QueryInterface(IID_PPV_ARG(FolderItem2, &pFI2))))
    {
        // This assumes we are the only implementer of CFolderItem (probably true)
        // but when we get IParentAndItem we can do this in a safer way
        pidl = static_cast<CFolderItem*>(pFI2)->m_idlist;
        pFI2->Release();
    }
    return pidl;
}

LPCITEMIDLIST CFolderItem::GetInternalPidlRef(const VARIANT *pV)
{
    if (!pV)
        return NULL;

    if (V_VT(pV) == (VT_VARIANT | VT_BYREF) && V_VARIANTREF(pV))
        pV = V_VARIANTREF(pV);

    switch (V_VT(pV))
    {
        case VT_DISPATCH | VT_BYREF:
            return V_DISPATCHREF(pV) ? GetInternalPidlRef(*V_DISPATCHREF(pV)) : NULL;
        case VT_DISPATCH:
            return GetInternalPidlRef(V_DISPATCH(pV));
    }
    return NULL;
}

PCUITEMID_CHILD CFolderItem::GetLeafPidlRef(const VARIANT *pV)
{
    return (PCUITEMID_CHILD)ILFindLastID(GetInternalPidlRef(pV));
}

HRESULT CFolderItem::GetFindDataFromIDList(WIN32_FIND_DATA &wfd)
{
    CComPtr<IShellFolder2> psf;
    PCUITEMID_CHILD pidlLeaf;
    HRESULT hr = GetParentShellFolderAndItem(IID_PPV_ARG(IShellFolder2, &psf), pidlLeaf);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;
    return SHGetDataFromIDListW(psf, pidlLeaf, SHGDFIL_FINDDATA, &wfd, sizeof(wfd));
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

    if (!ppid)
        return E_INVALIDARG;
    return m_Folder->QueryInterface(IID_PPV_ARG(IDispatch, ppid));
}

HRESULT STDMETHODCALLTYPE CFolderItem::get_Name(BSTR *pbs)
{
    TRACE("(%p, %p)\n", this, pbs);

    *pbs = NULL;

    CComPtr<IShellFolder> Parent;
    PCUITEMID_CHILD last_part;
    HRESULT hr = GetParentShellFolderAndItem(IID_PPV_ARG(IShellFolder, &Parent), last_part);
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

    CComPtr<IShellFolder> psf;
    PCUITEMID_CHILD pidlLeaf;
    HRESULT hr = GetParentShellFolderAndItem(IID_PPV_ARG(IShellFolder, &psf), pidlLeaf);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    LPITEMIDLIST pidlNew;
    if (SUCCEEDED(hr = psf->SetNameOf(GetHwnd(), pidlLeaf, bs, SHGDN_INFOLDER, &pidlNew)) && pidlNew)
    {
        LPITEMIDLIST pidlClone = ILClone(m_idlist);
        ILRemoveLastID(pidlClone);
        if (pidlClone && SUCCEEDED(SHILAppend(pidlNew, &pidlClone)))
        {
            m_idlist.Free();
            m_idlist.Attach(pidlClone);
        }
    }
    return hr;
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
    return ShellObjectCreatorInit<CFolder>(const_cast<LPITEMIDLIST>(GetAbsoluteIDList()), IID_PPV_ARG(IDispatch, ppid));
}

HRESULT CFolderItem::HasAttribute(DWORD sfgaof, VARIANT_BOOL *pB)
{
    *pB = SHGetAttributes(NULL, GetAbsoluteIDList(), sfgaof) ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CFolderItem::get_IsLink(VARIANT_BOOL *pb)
{
    TRACE("(%p, %p)\n", this, pb);
    return HasAttribute(SFGAO_LINK, pb);
}

HRESULT STDMETHODCALLTYPE CFolderItem::get_IsFolder(VARIANT_BOOL *pb)
{
    TRACE("(%p, %p)\n", this, pb);
    return HasAttribute(SFGAO_FOLDER, pb);
}

HRESULT STDMETHODCALLTYPE CFolderItem::get_IsFileSystem(VARIANT_BOOL *pb)
{
    TRACE("(%p, %p)\n", this, pb);
    return HasAttribute(SFGAO_FILESYSTEM, pb);
}

HRESULT STDMETHODCALLTYPE CFolderItem::get_IsBrowsable(VARIANT_BOOL *pb)
{
    TRACE("(%p, %p)\n", this, pb);
    return HasAttribute(SFGAO_BROWSABLE, pb);
}

HRESULT STDMETHODCALLTYPE CFolderItem::get_ModifyDate(DATE *pdt)
{
    TRACE("(%p, %p)\n", this, pdt);

    WIN32_FIND_DATA wfd;
    if (SUCCEEDED(GetFindDataFromIDList(wfd)))
    {
        FILETIME ft;
        FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &ft);
        WORD dd, dt;
        if (FileTimeToDosDateTime(&ft, &dd, &dt) && DosDateTimeToVariantTime(dd, dt, pdt))
            return S_OK;
    }
    *pdt = 0;
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CFolderItem::put_ModifyDate(DATE dt)
{
    TRACE("(%p, %f)\n", this, dt);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFolderItem::get_Size(LONG *pul)
{
    TRACE("(%p, %p)\n", this, pul);

    WIN32_FIND_DATA wfd;
    if (SUCCEEDED(GetFindDataFromIDList(wfd)))
    {
        *pul = wfd.nFileSizeLow;
        return S_OK;
    }
    *pul = 0;
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CFolderItem::get_Type(BSTR *pbs)
{
    TRACE("(%p, %p)\n", this, pbs);

    VARIANT v;
    V_VT(&v) = VT_EMPTY;
    if (SUCCEEDED(GetExtendedProperty(PKEY_ItemTypeText, &v)))
    {
        if (SUCCEEDED(VariantChangeType(&v, &v, 0, VT_BSTR)))
        {
            *pbs = V_BSTR(&v);
            return S_OK;
        }
        VariantClear(&v);
    }
    HRESULT hr = SHELL_SysAllocString(L"", pbs);
    return hr == S_OK ? S_FALSE : hr;
}

HRESULT STDMETHODCALLTYPE CFolderItem::Verbs(FolderItemVerbs **ppfic)
{
    if (!ppfic)
        return E_POINTER;

    CComPtr<CFolderItemVerbs> pVerbs;
    HRESULT hr = CFolderItemVerbs::CreateInstance(pVerbs);
    if (SUCCEEDED(hr))
        hr = pVerbs->Init(GetAbsoluteIDList());
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    *ppfic = pVerbs.Detach();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CFolderItem::InvokeVerb(VARIANT vVerb)
{
    VARIANT empty;
    V_VT(&empty) = VT_EMPTY;
    return InvokeVerbEx(vVerb, empty);
}

HRESULT STDMETHODCALLTYPE CFolderItem::InvokeVerbEx(VARIANT vVerb, VARIANT vArgs)
{
    TRACE("(%p, %s)\n", this, wine_dbgstr_variant(&vVerb));

    SHELLEXECUTEINFOW sei;
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_INVOKEIDLIST | SEE_MASK_FLAG_NO_UI;
    sei.hwnd = GetHwnd();
    sei.lpVerb = V_VT(&vVerb) == VT_BSTR ? V_BSTR(&vVerb) : NULL;
    sei.lpFile = NULL;
    sei.lpParameters = V_VT(&vArgs) == VT_BSTR ? V_BSTR(&vArgs) : NULL;
    sei.lpDirectory = NULL;
    sei.nShow = SW_SHOW;
    sei.lpIDList = const_cast<LPITEMIDLIST>(GetAbsoluteIDList());
    ShellExecuteExW(&sei);
    return S_OK;
}

HRESULT CFolderItem::GetExtendedProperty(REFPROPERTYKEY pkey, VARIANT *pv)
{
    CComPtr<IShellFolder2> psf;
    PCUITEMID_CHILD pidlLeaf;
    HRESULT hr = GetParentShellFolderAndItem(IID_PPV_ARG(IShellFolder2, &psf), pidlLeaf);
    return FAILED(hr) ? hr : psf->GetDetailsEx(pidlLeaf, reinterpret_cast<const SHCOLUMNID*>(&pkey), pv);
}

HRESULT STDMETHODCALLTYPE CFolderItem::ExtendedProperty(BSTR bsPropName, VARIANT *pv)
{
    if (!pv)
        return E_INVALIDARG;

    PROPERTYKEY pkeybuf;
    const PROPERTYKEY *pPK = SHELL_GetPropertyKeyFromString(bsPropName, &pkeybuf);
    if (pPK && GetExtendedProperty(*pPK, pv) == S_OK)
        return S_OK;

    V_VT(pv) = VT_EMPTY;
    return S_FALSE;
}


CFolderItems::CFolderItems()
    :m_Count(-1)
{
}

CFolderItems::~CFolderItems()
{
}

HRESULT CFolderItems::Initialize(LPCITEMIDLIST idlist, Folder* parent)
{
    CComPtr<IShellFolder> psfDesktop, psfTarget;

    m_idlist.Attach(ILClone(idlist));
    if (!m_idlist)
        return E_OUTOFMEMORY;

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

