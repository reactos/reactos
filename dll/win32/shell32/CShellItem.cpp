/*
 * IShellItem implementation
 *
 * Copyright 2008 Vincent Povirk for CodeWeavers
 * Copyright 2009 Andrew Hill
 * Copyright 2013 Katayama Hirofumi MZ
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

EXTERN_C HRESULT WINAPI SHCreateShellItem(PCIDLIST_ABSOLUTE pidlParent,
    IShellFolder *psfParent, PCUITEMID_CHILD pidl, IShellItem **ppsi);

CShellItem::CShellItem() :
    m_pidl(NULL)
{
}

CShellItem::~CShellItem()
{
    ILFree(m_pidl);
}

HRESULT CShellItem::get_parent_pidl(LPITEMIDLIST *parent_pidl)
{
    *parent_pidl = ILClone(m_pidl);
    if (*parent_pidl)
    {
        if (ILRemoveLastID(*parent_pidl))
            return S_OK;
        else
        {
            ILFree(*parent_pidl);
            *parent_pidl = NULL;
            return E_INVALIDARG;
        }
    }
    else
    {
        *parent_pidl = NULL;
        return E_OUTOFMEMORY;
    }
}

HRESULT CShellItem::get_parent_shellfolder(IShellFolder **ppsf)
{
    HRESULT hr;
    LPITEMIDLIST parent_pidl;
    CComPtr<IShellFolder>        desktop;

    hr = get_parent_pidl(&parent_pidl);
    if (SUCCEEDED(hr))
    {
        hr = SHGetDesktopFolder(&desktop);
        if (SUCCEEDED(hr))
            hr = desktop->BindToObject(parent_pidl, NULL, IID_PPV_ARG(IShellFolder, ppsf));
        ILFree(parent_pidl);
    }

    return hr;
}

HRESULT CShellItem::get_shellfolder(IBindCtx *pbc, REFIID riid, void **ppvOut)
{
    CComPtr<IShellFolder> psf;
    CComPtr<IShellFolder> psfDesktop;
    HRESULT ret;

    ret = SHGetDesktopFolder(&psfDesktop);
    if (FAILED_UNEXPECTEDLY(ret))
        return ret;

    if (_ILIsDesktop(m_pidl))
        psf = psfDesktop;
    else
    {
        ret = psfDesktop->BindToObject(m_pidl, pbc, IID_PPV_ARG(IShellFolder, &psf));
        if (FAILED_UNEXPECTEDLY(ret))
            return ret;
    }

    return psf->QueryInterface(riid, ppvOut);
}

HRESULT WINAPI CShellItem::BindToHandler(IBindCtx *pbc, REFGUID rbhid, REFIID riid, void **ppvOut)
{
    HRESULT ret;
    TRACE("(%p, %p,%s,%p,%p)\n", this, pbc, shdebugstr_guid(&rbhid), riid, ppvOut);

    *ppvOut = NULL;
    if (IsEqualGUID(rbhid, BHID_SFObject))
    {
        return get_shellfolder(pbc, riid, ppvOut);
    }
    else if (IsEqualGUID(rbhid, BHID_SFUIObject))
    {
        CComPtr<IShellFolder> psf_parent;
        if (_ILIsDesktop(m_pidl))
            ret = SHGetDesktopFolder(&psf_parent);
        else
            ret = get_parent_shellfolder(&psf_parent);
        if (FAILED_UNEXPECTEDLY(ret))
            return ret;

        LPCITEMIDLIST pidl = ILFindLastID(m_pidl);
        return psf_parent->GetUIObjectOf(NULL, 1, &pidl, riid, NULL, ppvOut);
    }
    else if (IsEqualGUID(rbhid, BHID_DataObject))
    {
        return BindToHandler(pbc, BHID_SFUIObject, IID_IDataObject, ppvOut);
    }
    else if (IsEqualGUID(rbhid, BHID_SFViewObject))
    {
        CComPtr<IShellFolder> psf;
        ret = get_shellfolder(NULL, IID_PPV_ARG(IShellFolder, &psf));
        if (FAILED_UNEXPECTEDLY(ret))
            return ret;

        return psf->CreateViewObject(NULL, riid, ppvOut);
    }

    FIXME("Unsupported BHID %s.\n", debugstr_guid(&rbhid));

    return MK_E_NOOBJECT;
}

HRESULT WINAPI CShellItem::GetParent(IShellItem **ppsi)
{
    HRESULT hr;
    LPITEMIDLIST parent_pidl;

    TRACE("(%p,%p)\n", this, ppsi);

    hr = get_parent_pidl(&parent_pidl);
    if (SUCCEEDED(hr))
    {
        hr = SHCreateShellItem(NULL, NULL, parent_pidl, ppsi);
        ILFree(parent_pidl);
    }

    return hr;
}

HRESULT WINAPI CShellItem::GetDisplayName(SIGDN sigdnName, LPWSTR *ppszName)
{
    return SHGetNameFromIDList(m_pidl, sigdnName, ppszName);
}

HRESULT WINAPI CShellItem::GetAttributes(SFGAOF sfgaoMask, SFGAOF *psfgaoAttribs)
{
    CComPtr<IShellFolder>        parent_folder;
    LPCITEMIDLIST child_pidl;
    HRESULT hr;

    TRACE("(%p,%x,%p)\n", this, sfgaoMask, psfgaoAttribs);

    if (_ILIsDesktop(m_pidl))
        hr = SHGetDesktopFolder(&parent_folder);
    else
        hr = get_parent_shellfolder(&parent_folder);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    child_pidl = ILFindLastID(m_pidl);
    *psfgaoAttribs = sfgaoMask;
    hr = parent_folder->GetAttributesOf(1, &child_pidl, psfgaoAttribs);
    *psfgaoAttribs &= sfgaoMask;

    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return (sfgaoMask == *psfgaoAttribs) ? S_OK : S_FALSE;
}

HRESULT WINAPI CShellItem::Compare(IShellItem *oth, SICHINTF hint, int *piOrder)
{
    HRESULT hr;
    CComPtr<IPersistIDList>      pIDList;
    CComPtr<IShellFolder>        psfDesktop;
    LPITEMIDLIST pidl;

    TRACE("(%p,%p,%x,%p)\n", this, oth, hint, piOrder);

    if (piOrder == NULL || oth == NULL)
        return E_POINTER;

    hr = oth->QueryInterface(IID_PPV_ARG(IPersistIDList, &pIDList));
    if (SUCCEEDED(hr))
    {
        hr = pIDList->GetIDList(&pidl);
        if (SUCCEEDED(hr))
        {
            hr = SHGetDesktopFolder(&psfDesktop);
            if (SUCCEEDED(hr))
            {
                hr = psfDesktop->CompareIDs(hint, m_pidl, pidl);
                *piOrder = (int)(short)SCODE_CODE(hr);
            }
            ILFree(pidl);
        }
    }

    if(FAILED(hr))
        return hr;

    if(*piOrder)
        return S_FALSE;
    else
        return S_OK;
}

HRESULT WINAPI CShellItem::GetClassID(CLSID *pClassID)
{
    TRACE("(%p,%p)\n", this, pClassID);

    *pClassID = CLSID_ShellItem;
    return S_OK;
}

HRESULT WINAPI CShellItem::SetIDList(PCIDLIST_ABSOLUTE pidlx)
{
    LPITEMIDLIST new_pidl;

    TRACE("(%p,%p)\n", this, pidlx);

    new_pidl = ILClone(pidlx);
    if (new_pidl)
    {
        ILFree(m_pidl);
        m_pidl = new_pidl;
        return S_OK;
    }
    else
        return E_OUTOFMEMORY;
}

HRESULT WINAPI CShellItem::GetIDList(PIDLIST_ABSOLUTE *ppidl)
{
    TRACE("(%p,%p)\n", this, ppidl);

    *ppidl = ILClone(m_pidl);
    if (*ppidl)
        return S_OK;
    else
        return E_OUTOFMEMORY;
}

HRESULT WINAPI SHCreateShellItem(PCIDLIST_ABSOLUTE pidlParent,
    IShellFolder *psfParent, PCUITEMID_CHILD pidl, IShellItem **ppsi)
{
    HRESULT hr;
    CComPtr<IShellItem> newShellItem;
    LPITEMIDLIST new_pidl;
    CComPtr<IPersistIDList>            newPersistIDList;

    TRACE("(%p,%p,%p,%p)\n", pidlParent, psfParent, pidl, ppsi);

    *ppsi = NULL;

    if (!pidl)
        return E_INVALIDARG;

    if (pidlParent || psfParent)
    {
        LPITEMIDLIST temp_parent = NULL;
        if (!pidlParent)
        {
            CComPtr<IPersistFolder2>    ppf2Parent;

            if (FAILED(psfParent->QueryInterface(IID_PPV_ARG(IPersistFolder2, &ppf2Parent))))
            {
                FIXME("couldn't get IPersistFolder2 interface of parent\n");
                return E_NOINTERFACE;
            }

            if (FAILED(ppf2Parent->GetCurFolder(&temp_parent)))
            {
                FIXME("couldn't get parent PIDL\n");
                return E_NOINTERFACE;
            }

            pidlParent = temp_parent;
        }

        new_pidl = ILCombine(pidlParent, pidl);
        ILFree(temp_parent);

        if (!new_pidl)
            return E_OUTOFMEMORY;
    }
    else
    {
        new_pidl = ILClone(pidl);
        if (!new_pidl)
            return E_OUTOFMEMORY;
    }

    hr = CShellItem::_CreatorClass::CreateInstance(NULL, IID_PPV_ARG(IShellItem, &newShellItem));
    if (FAILED(hr))
    {
        ILFree(new_pidl);
        return hr;
    }
    hr = newShellItem->QueryInterface(IID_PPV_ARG(IPersistIDList, &newPersistIDList));
    if (FAILED(hr))
    {
        ILFree(new_pidl);
        return hr;
    }
    hr = newPersistIDList->SetIDList(new_pidl);
    if (FAILED(hr))
    {
        ILFree(new_pidl);
        return hr;
    }
    ILFree(new_pidl);

    *ppsi = newShellItem.Detach();

    return hr;
}

/* Generic IEnumShellItems backed by an IShellItemArray */
class CEnumShellItems :
    public CComCoClass<CEnumShellItems, &CLSID_NULL>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IEnumShellItems
{
    CComPtr<IShellItemArray> m_psia;
    DWORD m_nPos;
    DWORD m_cItems;

public:
    CEnumShellItems() : m_nPos(0), m_cItems(0) {}

    HRESULT Initialize(IShellItemArray *psia, DWORD nStartPos = 0)
    {
        m_psia = psia;
        m_nPos = nStartPos;
        return psia->GetCount(&m_cItems);
    }

    STDMETHODIMP Next(ULONG celt, IShellItem **rgelt, ULONG *pceltFetched) override
    {
        if (!rgelt)
            return E_INVALIDARG;
        ULONG fetched = 0;
        while (fetched < celt && m_nPos < m_cItems)
        {
            if (FAILED(m_psia->GetItemAt(m_nPos, &rgelt[fetched])))
                break;
            m_nPos++;
            fetched++;
        }
        if (pceltFetched)
            *pceltFetched = fetched;
        return (fetched == celt) ? S_OK : S_FALSE;
    }

    STDMETHODIMP Skip(ULONG celt) override
    {
        m_nPos = min(m_nPos + celt, m_cItems);
        return S_OK;
    }

    STDMETHODIMP Reset() override
    {
        m_nPos = 0;
        return S_OK;
    }

    STDMETHODIMP Clone(IEnumShellItems **ppenum) override
    {
        if (!ppenum)
            return E_INVALIDARG;
        *ppenum = NULL;
        CComObject<CEnumShellItems> *pNew;
        HRESULT hr = CComObject<CEnumShellItems>::CreateInstance(&pNew);
        if (FAILED(hr))
            return hr;
        pNew->AddRef();
        hr = pNew->Initialize(m_psia, m_nPos);
        if (SUCCEEDED(hr))
            hr = pNew->QueryInterface(IID_PPV_ARG(IEnumShellItems, ppenum));
        pNew->Release();
        return hr;
    }

    DECLARE_NO_REGISTRY()
    DECLARE_NOT_AGGREGATABLE(CEnumShellItems)

    BEGIN_COM_MAP(CEnumShellItems)
        COM_INTERFACE_ENTRY_IID(IID_IEnumShellItems, IEnumShellItems)
    END_COM_MAP()
};

static HRESULT CreateEnumShellItems(IShellItemArray *psia, IEnumShellItems **ppESI)
{
    if (!ppESI)
        return E_INVALIDARG;
    *ppESI = NULL;
    CComObject<CEnumShellItems> *pEnum;
    HRESULT hr = CComObject<CEnumShellItems>::CreateInstance(&pEnum);
    if (FAILED(hr))
        return hr;
    pEnum->AddRef();
    hr = pEnum->Initialize(psia);
    if (SUCCEEDED(hr))
        hr = pEnum->QueryInterface(IID_PPV_ARG(IEnumShellItems, ppESI));
    pEnum->Release();
    return hr;
}

class CShellItemArray :
    public CComCoClass<CShellItemArray, &CLSID_NULL>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellItemArray
{
    CIDA *m_pCIDA;
    STGMEDIUM m_Medium;

public:
    CShellItemArray() : m_pCIDA(NULL)
    {
        m_Medium.tymed = TYMED_NULL;
    }

    virtual ~CShellItemArray()
    {
        CDataObjectHIDA::DestroyCIDA(m_pCIDA, m_Medium);
    }

    HRESULT Initialize(IDataObject *pdo)
    {
        return CDataObjectHIDA::CreateCIDA(pdo, &m_pCIDA, m_Medium);
    }

    inline UINT GetCount() const { return m_pCIDA->cidl; }

    // IShellItemArray
    STDMETHODIMP BindToHandler(IBindCtx *pbc, REFGUID rbhid, REFIID riid, void **ppv) override
    {
        if (!ppv)
            return E_INVALIDARG;
        *ppv = NULL;

        if (IsEqualGUID(rbhid, BHID_DataObject))
        {
            if (!m_pCIDA)
                return E_FAIL;
            PCIDLIST_ABSOLUTE pidlParent = (PCIDLIST_ABSOLUTE)((BYTE*)m_pCIDA + m_pCIDA->aoffset[0]);
            UINT cidl = m_pCIDA->cidl;
            if (cidl == 0)
                return SHCreateDataObject(pidlParent, 0, NULL, NULL, riid, ppv);
            PCUITEMID_CHILD_ARRAY apidl = (PCUITEMID_CHILD_ARRAY)CoTaskMemAlloc(cidl * sizeof(PCUITEMID_CHILD));
            if (!apidl)
                return E_OUTOFMEMORY;
            for (UINT i = 0; i < cidl; i++)
                apidl[i] = (PCUITEMID_CHILD)((BYTE*)m_pCIDA + m_pCIDA->aoffset[i + 1]);
            HRESULT hr = SHCreateDataObject(pidlParent, cidl, apidl, NULL, riid, ppv);
            CoTaskMemFree(apidl);
            return hr;
        }

        FIXME("Unhandled BHID %s\n", debugstr_guid(&rbhid));
        return E_NOTIMPL;
    }

    STDMETHODIMP GetPropertyStore(GETPROPERTYSTOREFLAGS flags, REFIID riid, void **ppv) override
    {
        UNIMPLEMENTED;
        *ppv = NULL;
        return E_NOTIMPL;
    }

    STDMETHODIMP GetPropertyDescriptionList(REFPROPERTYKEY keyType, REFIID riid, void **ppv) override
    {
        UNIMPLEMENTED;
        *ppv = NULL;
        return E_NOTIMPL;
    }

    STDMETHODIMP GetAttributes(SIATTRIBFLAGS dwAttribFlags, SFGAOF sfgaoMask, SFGAOF *psfgaoAttribs) override
    {
        if (!psfgaoAttribs)
            return E_INVALIDARG;
        if (!m_pCIDA || GetCount() == 0)
        {
            *psfgaoAttribs = 0;
            return E_INVALIDARG;
        }
        BOOL bAnd = (dwAttribFlags & SIATTRIBFLAGS_OR) == 0;
        SFGAOF result = bAnd ? sfgaoMask : 0;
        for (UINT i = 0; i < GetCount(); i++)
        {
            CComPtr<IShellItem> psi;
            if (FAILED(GetItemAt(i, &psi)))
                continue;
            SFGAOF attrs = sfgaoMask;
            psi->GetAttributes(sfgaoMask, &attrs);
            if (bAnd)
                result &= attrs;
            else
                result |= attrs;
        }
        *psfgaoAttribs = result;
        return (result == sfgaoMask) ? S_OK : S_FALSE;
    }

    STDMETHODIMP GetCount(DWORD *pCount) override
    {
        *pCount = m_pCIDA ? GetCount() : 0;
        return S_OK;
    }

    STDMETHODIMP GetItemAt(DWORD nIndex, IShellItem **ppItem) override
    {
        if (!ppItem)
            return E_INVALIDARG;
        *ppItem = NULL;
        if (!m_pCIDA)
            return E_UNEXPECTED;
        if (nIndex >= GetCount())
            return E_FAIL;
        return SHCreateShellItem(HIDA_GetPIDLFolder(m_pCIDA), NULL,
                                 HIDA_GetPIDLItem(m_pCIDA, nIndex), ppItem);
    }

    STDMETHODIMP EnumItems(IEnumShellItems **ppESI) override
    {
        return CreateEnumShellItems(static_cast<IShellItemArray*>(this), ppESI);
    }

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CShellItemArray)

BEGIN_COM_MAP(CShellItemArray)
    COM_INTERFACE_ENTRY_IID(IID_IShellItemArray, IShellItemArray)
END_COM_MAP()
};

EXTERN_C HRESULT WINAPI
SHCreateShellItemArrayFromDataObject(_In_ IDataObject *pdo, _In_ REFIID riid, _Out_ void **ppv)
{
    return ShellObjectCreatorInit<CShellItemArray>(pdo, riid, ppv);
}

class CShellItemArrayExplicit :
    public CComCoClass<CShellItemArrayExplicit, &CLSID_NULL>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellItemArray
{
    UINT          m_cidl;
    LPITEMIDLIST *m_rgpidl;

public:
    CShellItemArrayExplicit() : m_cidl(0), m_rgpidl(NULL) {}

    virtual ~CShellItemArrayExplicit()
    {
        for (UINT i = 0; i < m_cidl; i++)
            ILFree(m_rgpidl[i]);
        CoTaskMemFree(m_rgpidl);
    }

    HRESULT Initialize(PCIDLIST_ABSOLUTE pidlParent, UINT cidl, PCUITEMID_CHILD_ARRAY ppidl)
    {
        m_rgpidl = (LPITEMIDLIST *)CoTaskMemAlloc(cidl * sizeof(LPITEMIDLIST));
        if (!m_rgpidl)
            return E_OUTOFMEMORY;
        m_cidl = 0;
        for (UINT i = 0; i < cidl; i++)
        {
            m_rgpidl[i] = ILCombine(pidlParent, ppidl[i]);
            if (!m_rgpidl[i])
            {
                for (UINT j = 0; j < m_cidl; j++)
                    ILFree(m_rgpidl[j]);
                CoTaskMemFree(m_rgpidl);
                m_rgpidl = NULL;
                return E_OUTOFMEMORY;
            }
            m_cidl++;
        }
        return S_OK;
    }

    // IShellItemArray
    STDMETHODIMP BindToHandler(IBindCtx *pbc, REFGUID rbhid, REFIID riid, void **ppv) override
    {
        if (!ppv)
            return E_INVALIDARG;
        *ppv = NULL;

        if (IsEqualGUID(rbhid, BHID_DataObject))
        {
            if (m_cidl == 0)
                return E_FAIL;
            /* Use parent of first item; assume all items share the same parent folder */
            LPITEMIDLIST pidlParent = ILClone(m_rgpidl[0]);
            if (!pidlParent)
                return E_OUTOFMEMORY;
            ILRemoveLastID(pidlParent);

            PCUITEMID_CHILD_ARRAY apidl = (PCUITEMID_CHILD_ARRAY)CoTaskMemAlloc(m_cidl * sizeof(PCUITEMID_CHILD));
            if (!apidl)
            {
                ILFree(pidlParent);
                return E_OUTOFMEMORY;
            }
            for (UINT i = 0; i < m_cidl; i++)
                apidl[i] = ILFindLastID(m_rgpidl[i]);
            HRESULT hr = SHCreateDataObject(pidlParent, m_cidl, apidl, NULL, riid, ppv);
            CoTaskMemFree(apidl);
            ILFree(pidlParent);
            return hr;
        }

        FIXME("Unhandled BHID %s\n", debugstr_guid(&rbhid));
        return E_NOTIMPL;
    }

    STDMETHODIMP GetPropertyStore(GETPROPERTYSTOREFLAGS flags, REFIID riid, void **ppv) override
    {
        UNIMPLEMENTED;
        *ppv = NULL;
        return E_NOTIMPL;
    }

    STDMETHODIMP GetPropertyDescriptionList(REFPROPERTYKEY keyType, REFIID riid, void **ppv) override
    {
        UNIMPLEMENTED;
        *ppv = NULL;
        return E_NOTIMPL;
    }

    STDMETHODIMP GetAttributes(SIATTRIBFLAGS dwAttribFlags, SFGAOF sfgaoMask, SFGAOF *psfgaoAttribs) override
    {
        BOOL bAnd = (dwAttribFlags & SIATTRIBFLAGS_OR) == 0;
        SFGAOF result = bAnd ? sfgaoMask : 0;
        for (UINT i = 0; i < m_cidl; i++)
        {
            CComPtr<IShellItem> psi;
            if (FAILED(SHCreateShellItem(NULL, NULL, m_rgpidl[i], &psi)))
                continue;
            SFGAOF attrs = sfgaoMask;
            psi->GetAttributes(sfgaoMask, &attrs);
            if (bAnd)
                result &= attrs;
            else
                result |= attrs;
        }
        *psfgaoAttribs = result;
        return (result == sfgaoMask) ? S_OK : S_FALSE;
    }

    STDMETHODIMP GetCount(DWORD *pCount) override
    {
        *pCount = m_cidl;
        return S_OK;
    }

    STDMETHODIMP GetItemAt(DWORD nIndex, IShellItem **ppItem) override
    {
        if (!ppItem)
            return E_INVALIDARG;
        *ppItem = NULL;
        if (nIndex >= m_cidl)
            return E_FAIL;
        return SHCreateShellItem(NULL, NULL, m_rgpidl[nIndex], ppItem);
    }

    STDMETHODIMP EnumItems(IEnumShellItems **ppESI) override
    {
        return CreateEnumShellItems(static_cast<IShellItemArray*>(this), ppESI);
    }

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CShellItemArrayExplicit)

BEGIN_COM_MAP(CShellItemArrayExplicit)
    COM_INTERFACE_ENTRY_IID(IID_IShellItemArray, IShellItemArray)
END_COM_MAP()
};

EXTERN_C HRESULT WINAPI
SHCreateItemFromIDList(_In_ PCIDLIST_ABSOLUTE pidl, _In_ REFIID riid, _Out_ void **ppv)
{
    CComPtr<IShellItem> psi;
    HRESULT hr = SHCreateShellItem(NULL, NULL, (PCUITEMID_CHILD)pidl, &psi);
    if (SUCCEEDED(hr))
        hr = psi->QueryInterface(riid, ppv);
    return hr;
}

EXTERN_C HRESULT WINAPI
SHGetItemFromObject(_In_ IUnknown *punk, _In_ REFIID riid, _Out_ void **ppv)
{
    CComPtr<IPersistIDList> ppidl;
    if (SUCCEEDED(punk->QueryInterface(IID_PPV_ARG(IPersistIDList, &ppidl))))
    {
        LPITEMIDLIST pidl;
        HRESULT hr = ppidl->GetIDList(&pidl);
        if (SUCCEEDED(hr))
        {
            hr = SHCreateItemFromIDList(pidl, riid, ppv);
            ILFree(pidl);
        }
        return hr;
    }

    CComPtr<IPersistFolder2> ppf2;
    if (SUCCEEDED(punk->QueryInterface(IID_PPV_ARG(IPersistFolder2, &ppf2))))
    {
        LPITEMIDLIST pidl;
        HRESULT hr = ppf2->GetCurFolder(&pidl);
        if (SUCCEEDED(hr))
        {
            hr = SHCreateItemFromIDList(pidl, riid, ppv);
            ILFree(pidl);
        }
        return hr;
    }

    return E_NOINTERFACE;
}

EXTERN_C HRESULT WINAPI
SHCreateItemWithParent(
    _In_opt_ PCIDLIST_ABSOLUTE pidlParent,
    _In_opt_ IShellFolder *psfParent,
    _In_ PCUITEMID_CHILD pidl,
    _In_ REFIID riid,
    _Outptr_ void **ppv)
{
    CComPtr<IShellItem> psi;
    HRESULT hr;

    if (!ppv)
        return E_INVALIDARG;
    *ppv = NULL;

    hr = SHCreateShellItem(pidlParent, psfParent, pidl, &psi);
    if (SUCCEEDED(hr))
        hr = psi->QueryInterface(riid, ppv);
    return hr;
}

EXTERN_C HRESULT WINAPI
SHGetKnownFolderItem(
    _In_ REFKNOWNFOLDERID rfid,
    _In_ DWORD dwFlags,
    _In_opt_ HANDLE hToken,
    _In_ REFIID riid,
    _Outptr_ void **ppv)
{
    PIDLIST_ABSOLUTE pidl;
    HRESULT hr;

    if (!ppv)
        return E_INVALIDARG;
    *ppv = NULL;

    hr = SHGetKnownFolderIDList(rfid, dwFlags, hToken, &pidl);
    if (FAILED(hr))
        return hr;

    hr = SHCreateItemFromIDList(pidl, riid, ppv);
    ILFree(pidl);
    return hr;
}

EXTERN_C HRESULT WINAPI
SHCreateItemFromParsingName(
    _In_ PCWSTR pszPath,
    _In_opt_ IBindCtx *pbc,
    _In_ REFIID riid,
    _Out_ void **ppv)
{
    LPITEMIDLIST pidl;
    SFGAOF sfgao = 0;
    HRESULT hr;

    if (!ppv)
        return E_INVALIDARG;
    *ppv = NULL;

    hr = SHParseDisplayName(pszPath, pbc, &pidl, 0, &sfgao);
    if (SUCCEEDED(hr))
    {
        hr = SHCreateItemFromIDList(pidl, riid, ppv);
        ILFree(pidl);
    }
    return hr;
}

EXTERN_C HRESULT WINAPI
SHCreateShellItemArray(
    _In_opt_ PCIDLIST_ABSOLUTE pidlParent,
    _In_opt_ IShellFolder *psf,
    _In_ UINT cidl,
    _In_reads_opt_(cidl) PCUITEMID_CHILD_ARRAY ppidl,
    _Out_ IShellItemArray **ppsiItemArray)
{
    HRESULT hr;
    LPITEMIDLIST pidlParentOwned = NULL;

    if (!ppsiItemArray)
        return E_INVALIDARG;
    *ppsiItemArray = NULL;

    if (!pidlParent && psf)
    {
        CComPtr<IPersistFolder2> ppf2;
        hr = psf->QueryInterface(IID_PPV_ARG(IPersistFolder2, &ppf2));
        if (FAILED(hr))
            return hr;
        hr = ppf2->GetCurFolder(&pidlParentOwned);
        if (FAILED(hr))
            return hr;
        pidlParent = pidlParentOwned;
    }

    CComObject<CShellItemArrayExplicit> *pObj;
    hr = CComObject<CShellItemArrayExplicit>::CreateInstance(&pObj);
    if (FAILED(hr))
    {
        ILFree(pidlParentOwned);
        return hr;
    }
    pObj->AddRef();

    hr = pObj->Initialize(pidlParent, cidl, ppidl);
    ILFree(pidlParentOwned);
    if (SUCCEEDED(hr))
        hr = pObj->QueryInterface(IID_PPV_ARG(IShellItemArray, ppsiItemArray));
    pObj->Release();
    return hr;
}

EXTERN_C BOOL WINAPI
SHGetPathFromIDListEx(
    _In_ PCIDLIST_ABSOLUTE pidl,
    _Out_writes_(cchPath) PWSTR pszPath,
    _In_ DWORD cchPath,
    _In_ GPFIDL_FLAGS uOpts)
{
    WCHAR szTmp[MAX_PATH];
    DWORD len;

    TRACE("(%p, %p, %u, %d)\n", pidl, pszPath, cchPath, uOpts);

    if (!SHGetPathFromIDListW(pidl, szTmp))
        return FALSE;

    len = lstrlenW(szTmp);
    if (cchPath == 0 || len >= cchPath)
        return FALSE;

    lstrcpyW(pszPath, szTmp);
    return TRUE;
}

EXTERN_C HRESULT WINAPI
SHCreateItemInKnownFolder(
    _In_ REFKNOWNFOLDERID kfid,
    _In_ DWORD dwKFFlags,
    _In_opt_ PCWSTR pszItem,
    _In_ REFIID riid,
    _Outptr_ void **ppv)
{
    PIDLIST_ABSOLUTE pidlFolder;
    HRESULT hr;

    TRACE("(%s, %08x, %s, %s, %p)\n", debugstr_guid(&kfid), dwKFFlags,
          debugstr_w(pszItem), debugstr_guid(&riid), ppv);

    if (!ppv)
        return E_INVALIDARG;
    *ppv = NULL;

    hr = SHGetKnownFolderIDList(kfid, dwKFFlags, NULL, &pidlFolder);
    if (FAILED(hr))
        return hr;

    if (pszItem)
    {
        WCHAR szFolder[MAX_PATH], szFull[MAX_PATH];
        if (SHGetPathFromIDListW(pidlFolder, szFolder))
        {
            PathCombineW(szFull, szFolder, pszItem);
            ILFree(pidlFolder);
            hr = SHCreateItemFromParsingName(szFull, NULL, riid, ppv);
        }
        else
        {
            ILFree(pidlFolder);
            hr = E_FAIL;
        }
    }
    else
    {
        hr = SHCreateItemFromIDList(pidlFolder, riid, ppv);
        ILFree(pidlFolder);
    }

    return hr;
}
