/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     FolderItem(s) implementation
 * COPYRIGHT:   Copyright 2015-2018 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"
#include "prop.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

HRESULT SHELL_InvokeCommandOnContextMenu(_In_opt_ HWND hWnd, _In_ IContextMenu *pCM, _In_opt_ PCWSTR pszVerb, _In_ UINT cVerbs,
                                         _In_opt_ PCWSTR pszArgs, _In_ UINT fCMIC, _In_ UINT fCMF, _In_opt_ IUnknown *pSite)
{
    // Note: We can't use SHLWAPI::SHInvokeCommandOnContextMenu because it does not have an arguments parameter
    if (!pCM)
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    int iDefItem = 0;
    HMENU hMenu = NULL;
    HCURSOR hOldCursor = SetCursor(LoadCursorW(NULL, (LPCWSTR)IDC_WAIT));
    CMINVOKECOMMANDINFOEX ici = { sizeof(ici), fCMIC, hWnd, NULL, NULL, NULL, SW_SHOWNORMAL };
    CHAR szVerb[MAX_PATH], szArgs[MAX_PATH * 3];

    if (pSite)
        IUnknown_SetSite(pCM, pSite);

    if (IS_INTRESOURCE(pszVerb))
    {
        ici.lpVerb = MAKEINTRESOURCEA(pszVerb);
        if (!cVerbs)
        {
            hMenu = CreatePopupMenu();
            if (hMenu)
            {
                hr = pCM->QueryContextMenu(hMenu, 0, 1, MAXSHORT, fCMF | CMF_DEFAULTONLY);
                if ((iDefItem = GetMenuDefaultItem(hMenu, 0, 0)) != -1)
                    ici.lpVerb = MAKEINTRESOURCEA(iDefItem - 1);
            }
        }
        ici.lpVerbW = MAKEINTRESOURCEW(ici.lpVerb);
    }
    else
    {
        ici.fMask |= CMIC_MASK_UNICODE;
        ici.lpVerbW = pszVerb;
        if (pszVerb && SHUnicodeToAnsi(pszVerb, szVerb, _countof(szVerb)))
            ici.lpVerb = szVerb;
    }

    if (pszArgs)
    {
        ici.fMask |= CMIC_MASK_UNICODE;
        ici.lpParametersW = pszArgs;
        if (pszArgs && SHUnicodeToAnsi(pszArgs, szArgs, _countof(szArgs)))
            ici.lpParameters = szArgs;
    }

    SetCursor(hOldCursor);

    if (!FAILED_UNEXPECTEDLY(hr) && (iDefItem != -1 || ici.lpVerbW))
        hr = pCM->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici);
    if (pSite)
        IUnknown_SetSite(pCM, NULL);
    if (hMenu)
        DestroyMenu(hMenu);
    return hr;
}

static inline HRESULT GetUIObjectOfFolderItems(FolderItems *pFI, REFIID riid, void **ppv)
{
    VARIANT v;
    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = static_cast<IDispatch*>(pFI);
    return CFolder::GetUIObjectFromVariant(v, riid, ppv);
}

CFolderItem::CFolderItem()
{
}

CFolderItem::~CFolderItem()
{
}

HRESULT CFolderItem::Initialize(Folder* folder, LPCITEMIDLIST idlist)
{
    m_Folder = folder;
    return SHILClone(idlist, &m_idlist);
}

inline HRESULT CFolderItem::GetParentShellFolderAndItem(REFIID riid, void**ppv, PCUITEMID_CHILD &pidlLast)
{
    return SHBindToParent(GetAbsoluteIDList(), riid, ppv, &pidlLast);
}

HRESULT CFolderItem::GetParentAndItem(const VARIANT *pV, IParentAndItem **ppPAI)
{
    if (!pV)
        return E_POINTER;
    pV = VariantDerefVariant(pV);

    IUnknown *pUnk = NULL;
    switch (V_VT(pV))
    {
        case VT_DISPATCH | VT_BYREF:
            pUnk = V_DISPATCHREF(pV) ? *V_DISPATCHREF(pV) : NULL;
            break;
        case VT_DISPATCH:
            pUnk = V_DISPATCH(pV);
            break;
    }
    return pUnk ? pUnk->QueryInterface(IID_PPV_ARG(IParentAndItem, ppPAI)) : E_INVALIDARG;
}

PITEMID_CHILD CFolderItem::CloneLeafPidl(const VARIANT *pV)
{
    CComPtr<IParentAndItem> pPAI;
    PITEMID_CHILD pidl;
    if (SUCCEEDED(GetParentAndItem(pV, &pPAI)) && SUCCEEDED(pPAI->GetParentAndItem(NULL, NULL, &pidl)))
        return pidl;
    return NULL;
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
    return CFolder::CreateInstance(GetAbsoluteIDList(), *static_cast<Folder*>(m_Folder), IID_PPV_ARG(IDispatch, ppid));
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

    HWND hWnd = GetHwnd();
    CComPtr<IShellFolder> psf;
    PCUITEMID_CHILD pidlItem;
    HRESULT hr = GetParentShellFolderAndItem(IID_PPV_ARG(IShellFolder, &psf), pidlItem);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComPtr<IContextMenu> pCM;
    if (SUCCEEDED(hr = psf->GetUIObjectOf(hWnd, 1, &pidlItem, IID_NULL_PPV_ARG(IContextMenu, &pCM))))
        hr = CFolderItems::InvokeVerbHelper(hWnd, *pCM, vVerb, vArgs, GetSite());
    return hr;
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

    if (!lstrcmpiW(bsPropName, L"InfoTip"))
    {
        VARIANT v;
        V_VT(&v) = VT_DISPATCH;
        V_DISPATCH(&v) = static_cast<IDispatch*>(this);
        if (SUCCEEDED(m_Folder->GetDetailsOf(v, CFolder::INFOTIPCOLUMN, &V_BSTR(pv))))
        {
            V_VT(pv) = VT_BSTR;
            return S_OK;
        }
    }

    PROPERTYKEY pkeybuf;
    const PROPERTYKEY *pPK = SHELL_GetPropertyKeyFromString(bsPropName, &pkeybuf);
    if (pPK && GetExtendedProperty(*pPK, pv) == S_OK)
        return S_OK;

    V_VT(pv) = VT_EMPTY;
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CFolderItem::GetParentAndItem(PIDLIST_ABSOLUTE *ppidlParent, IShellFolder **ppsf, PITEMID_CHILD *ppidlChild)
{
    if (ppidlParent)
        *ppidlParent = NULL;
    if (ppsf)
        *ppsf = NULL;
    if (ppidlChild)
        *ppidlChild = NULL;

    HRESULT hr = S_FALSE;
    CComPtr<IShellFolder> psf;
    PCUITEMID_CHILD pidlChild;
    if (ppsf || ppidlChild)
    {
        if (FAILED(hr = GetParentShellFolderAndItem(IID_PPV_ARG(IShellFolder, &psf), pidlChild)))
            return hr;
    }

    if (ppidlParent && FAILED(hr = SHILCloneParent(GetAbsoluteIDList(), ppidlParent)))
        return hr;
    if (ppidlChild && FAILED(hr = SHILClone(pidlChild, ppidlChild)))
    {
        if (ppidlParent)
        {
            ILFree(*ppidlParent);
            *ppidlParent = NULL;
        }
        return hr;
    }
    if (ppsf)
        *ppsf = psf.Detach();
    return hr;
}


CFolderItems::CFolderItems()
    :m_Count(-1)
{
}

CFolderItems::~CFolderItems()
{
    SysFreeString(m_bstrFilter);
}

HRESULT CFolderItems::Initialize(LPCITEMIDLIST idlist, Folder* parent)
{
    m_Folder = parent;
    return SHILClone(idlist, &m_idlist);
}

void CFolderItems::ResetEnum()
{
    m_EnumIDList = NULL;
    m_Count = -1;
}

BOOL CFolderItems::IncludeItem(LPCITEMIDLIST pidl)
{
    if (!m_bstrFilter)
        return TRUE;

    CComHeapPtr<ITEMIDLIST> pidlFull;
    if (FAILED(SHILCombine(m_idlist, pidl, &pidlFull)))
        return FALSE;
    CComHeapPtr<WCHAR> pszPath;
    if (FAILED(SHELL_DisplayNameOf(NULL, pidlFull, SHGDN_FORPARSING | SHGDN_INFOLDER, &pszPath)))
        return FALSE;
    return PathMatchSpecW(pszPath, m_bstrFilter);
}

HRESULT CALLBACK CFolderItems::ItemsEnumFilter(void *Cookie, LPCITEMIDLIST pidl)
{
    return ((CFolderItems*)Cookie)->IncludeItem(pidl) ? S_OK : S_FALSE;
}

HRESULT CFolderItems::EnumObjects()
{
    if (m_EnumIDList)
        return S_OK;

    CComPtr<IShellFolder> psf;
    HRESULT hr = SHBindToObject(NULL, m_idlist, NULL, IID_PPV_ARG(IShellFolder, &psf));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComPtr<IEnumIDList> pEnum;
    if (FAILED(hr = psf->EnumObjects(NULL, m_Contf, &pEnum)))
        return hr;
    hr = pEnum ? S_OK : E_UNEXPECTED;
    if (SUCCEEDED(hr) && m_bstrFilter)
    {
        CComPtr<IEnumIDList> pFilteredEnum;
        hr = CEnumIDListBase::CreateInstance(*pEnum, ItemsEnumFilter, this, &pFilteredEnum);
        if (SUCCEEDED(hr))
            pEnum = pFilteredEnum;
    }

    if (FAILED(hr))
        return hr;
    m_EnumIDList = pEnum;
    return S_OK;
}

// *** FolderItems methods ***
HRESULT STDMETHODCALLTYPE CFolderItems::get_Count(long *plCount)
{
    if (!plCount)
        return E_POINTER;

    if (m_Count == -1)
    {
        long count = 0;

        if (FAILED(EnumObjects()))
            return E_FAIL;
        HRESULT hr = m_EnumIDList->Reset();
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        CComHeapPtr<ITEMIDLIST> Pidl;
        while ((hr = GetNext(&Pidl)) == S_OK)
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

    hr = VariantCopyInd(&index, &var);
    if (FAILED(hr))
        return hr;

    if (V_VT(&index) == VT_I2)
        VariantChangeType(&index, &index, 0, VT_I4);

    if (V_VT(&index) == VT_I4)
    {
        if (FAILED(EnumObjects()))
            return E_FAIL;

        ULONG count = V_UI4(&index);

        hr = m_EnumIDList->Reset();
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        hr = m_EnumIDList->Skip(count);

        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        CComHeapPtr<ITEMIDLIST> pidlChild;
        hr = GetNext(&pidlChild);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
        if (hr != S_OK)
            return E_INVALIDARG;

        CComHeapPtr<ITEMIDLIST> pidlFull;
        if (FAILED_UNEXPECTEDLY(hr = SHILCombine(m_idlist, pidlChild, &pidlFull)))
            return hr;
        hr = ShellObjectCreatorInit<CFolderItem>(m_Folder, pidlFull, IID_PPV_ARG(FolderItem, ppid));
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

HRESULT STDMETHODCALLTYPE CFolderItems::InvokeVerbEx(VARIANT vVerb, VARIANT vArgs)
{
    CComPtr<IContextMenu> pCM;
    HRESULT hr = GetUIObjectOfFolderItems(this, IID_PPV_ARG(IContextMenu, &pCM));
    if (FAILED(hr))
        return hr == HRESULT_FROM_WIN32(ERROR_NO_DATA) ? S_FALSE : hr;
    return InvokeVerbHelper(GetHwnd(), *pCM, vVerb, vArgs, GetSite());
}

HRESULT STDMETHODCALLTYPE CFolderItems::Filter(LONG grfFlags, BSTR bstrFilter)
{
    if (bstrFilter)
    {
        if (!*bstrFilter)
            bstrFilter = NULL;
        else if ((bstrFilter = SysAllocString(bstrFilter)) == NULL)
            return E_OUTOFMEMORY;
    }
    ResetEnum();
    SysFreeString(m_bstrFilter);
    m_bstrFilter = bstrFilter;
    m_Contf = grfFlags;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CFolderItems::get_Verbs(FolderItemVerbs **ppfic)
{
    if (!ppfic)
        return E_POINTER;

    CComPtr<CFolderItemVerbs> pVerbs;
    HRESULT hr = CFolderItemVerbs::CreateInstance(pVerbs);
    if (FAILED(hr))
        return hr;
    long count;
    if (SUCCEEDED(hr = get_Count(&count)) && count)
    {
        CComPtr<IContextMenu> pCM;
        if (SUCCEEDED(hr = GetUIObjectOfFolderItems(this, IID_PPV_ARG(IContextMenu, &pCM))))
            hr = pVerbs->Init(*static_cast<IContextMenu*>(pCM));
    }
    *ppfic = pVerbs.Detach();
    return S_OK;
}

HRESULT CFolderItems::InvokeVerbHelper(HWND hWnd, IContextMenu &cm, VARIANT &vVerb, VARIANT &vArgs, IUnknown *pSite)
{
    PCWSTR pszVerb = V_VT(&vVerb) == VT_BSTR && !StrIsNullOrEmpty(V_BSTR(&vVerb)) ? V_BSTR(&vVerb) : NULL;
    PCWSTR pszArgs = V_VT(&vArgs) == VT_BSTR && !StrIsNullOrEmpty(V_BSTR(&vArgs)) ? V_BSTR(&vArgs) : NULL;
    HRESULT hr = SHELL_InvokeCommandOnContextMenu(hWnd, &cm, pszVerb, !!pszVerb, pszArgs, 0, 0, pSite);
    return SUCCEEDED(hr) ? hr : S_FALSE;
}
