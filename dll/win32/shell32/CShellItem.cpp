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

/* FMTID_Storage filesystem property keys — not in ReactOS propkey.h yet */
static const PROPERTYKEY PKEY_ItemNameDisplay_L = { {0xB725F130,0x47EF,0x101A,{0xA5,0xF1,0x02,0x60,0x8C,0x9E,0xEB,0xAC}}, 10 };
static const PROPERTYKEY PKEY_Size_L            = { {0xB725F130,0x47EF,0x101A,{0xA5,0xF1,0x02,0x60,0x8C,0x9E,0xEB,0xAC}}, 12 };
static const PROPERTYKEY PKEY_FileAttributes_L  = { {0xB725F130,0x47EF,0x101A,{0xA5,0xF1,0x02,0x60,0x8C,0x9E,0xEB,0xAC}}, 13 };
static const PROPERTYKEY PKEY_DateModified_L    = { {0xB725F130,0x47EF,0x101A,{0xA5,0xF1,0x02,0x60,0x8C,0x9E,0xEB,0xAC}}, 14 };
static const PROPERTYKEY PKEY_DateCreated_L     = { {0xB725F130,0x47EF,0x101A,{0xA5,0xF1,0x02,0x60,0x8C,0x9E,0xEB,0xAC}}, 15 };
static const PROPERTYKEY PKEY_DateAccessed_L    = { {0xB725F130,0x47EF,0x101A,{0xA5,0xF1,0x02,0x60,0x8C,0x9E,0xEB,0xAC}}, 16 };

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
            PCUITEMID_CHILD *apidl = (PCUITEMID_CHILD*)CoTaskMemAlloc(cidl * sizeof(PCUITEMID_CHILD));
            if (!apidl)
                return E_OUTOFMEMORY;
            for (UINT i = 0; i < cidl; i++)
                apidl[i] = (PCUITEMID_CHILD)((BYTE*)m_pCIDA + m_pCIDA->aoffset[i + 1]);
            HRESULT hr = SHCreateDataObject(pidlParent, cidl, apidl, NULL, riid, ppv);
            CoTaskMemFree((LPVOID)apidl);
            return hr;
        }

        if (IsEqualGUID(rbhid, BHID_SFObject) ||
            IsEqualGUID(rbhid, BHID_SFUIObject) ||
            IsEqualGUID(rbhid, BHID_SFViewObject))
        {
            if (!m_pCIDA || m_pCIDA->cidl == 0)
                return E_FAIL;

            PCIDLIST_ABSOLUTE pidlParent = (PCIDLIST_ABSOLUTE)((BYTE*)m_pCIDA + m_pCIDA->aoffset[0]);
            CComPtr<IShellFolder> psfParent;
            HRESULT hr = SHBindToObject(NULL, pidlParent, NULL, IID_PPV_ARG(IShellFolder, &psfParent));
            if (FAILED(hr))
                return hr;

            if (IsEqualGUID(rbhid, BHID_SFObject))
            {
                return psfParent->QueryInterface(riid, ppv);
            }
            else if (IsEqualGUID(rbhid, BHID_SFUIObject))
            {
                UINT cidl = m_pCIDA->cidl;
                PCUITEMID_CHILD *apidl = (PCUITEMID_CHILD*)CoTaskMemAlloc(cidl * sizeof(PCUITEMID_CHILD));
                if (!apidl)
                    return E_OUTOFMEMORY;
                for (UINT i = 0; i < cidl; i++)
                    apidl[i] = (PCUITEMID_CHILD)((BYTE*)m_pCIDA + m_pCIDA->aoffset[i + 1]);
                hr = psfParent->GetUIObjectOf(NULL, cidl, apidl, riid, NULL, ppv);
                CoTaskMemFree((LPVOID)apidl);
                return hr;
            }
            else /* BHID_SFViewObject */
            {
                return psfParent->CreateViewObject(NULL, riid, ppv);
            }
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

    HRESULT InitializeAbsolute(UINT cidl, PCIDLIST_ABSOLUTE_ARRAY rgpidl)
    {
        m_rgpidl = (LPITEMIDLIST *)CoTaskMemAlloc(cidl * sizeof(LPITEMIDLIST));
        if (!m_rgpidl)
            return E_OUTOFMEMORY;
        m_cidl = 0;
        for (UINT i = 0; i < cidl; i++)
        {
            m_rgpidl[i] = ILClone(rgpidl[i]);
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

            PCUITEMID_CHILD *apidl = (PCUITEMID_CHILD*)CoTaskMemAlloc(m_cidl * sizeof(PCUITEMID_CHILD));
            if (!apidl)
            {
                ILFree(pidlParent);
                return E_OUTOFMEMORY;
            }
            for (UINT i = 0; i < m_cidl; i++)
                apidl[i] = ILFindLastID(m_rgpidl[i]);
            HRESULT hr = SHCreateDataObject(pidlParent, m_cidl, apidl, NULL, riid, ppv);
            CoTaskMemFree((LPVOID)apidl);
            ILFree(pidlParent);
            return hr;
        }

        if (IsEqualGUID(rbhid, BHID_SFObject) ||
            IsEqualGUID(rbhid, BHID_SFUIObject) ||
            IsEqualGUID(rbhid, BHID_SFViewObject))
        {
            if (m_cidl == 0)
                return E_FAIL;

            /* Get the parent folder (all items assumed to share same parent) */
            LPITEMIDLIST pidlParent = ILClone(m_rgpidl[0]);
            if (!pidlParent)
                return E_OUTOFMEMORY;
            ILRemoveLastID(pidlParent);

            CComPtr<IShellFolder> psfParent;
            HRESULT hr = SHBindToObject(NULL, pidlParent, NULL, IID_PPV_ARG(IShellFolder, &psfParent));
            ILFree(pidlParent);
            if (FAILED(hr))
                return hr;

            if (IsEqualGUID(rbhid, BHID_SFObject))
            {
                return psfParent->QueryInterface(riid, ppv);
            }
            else if (IsEqualGUID(rbhid, BHID_SFUIObject))
            {
                PCUITEMID_CHILD *apidl = (PCUITEMID_CHILD*)CoTaskMemAlloc(m_cidl * sizeof(PCUITEMID_CHILD));
                if (!apidl)
                    return E_OUTOFMEMORY;
                for (UINT i = 0; i < m_cidl; i++)
                    apidl[i] = ILFindLastID(m_rgpidl[i]);
                hr = psfParent->GetUIObjectOf(NULL, m_cidl, apidl, riid, NULL, ppv);
                CoTaskMemFree((LPVOID)apidl);
                return hr;
            }
            else /* BHID_SFViewObject */
            {
                return psfParent->CreateViewObject(NULL, riid, ppv);
            }
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
SHCreateItemFromRelativeName(
    _In_ IShellItem *psiParent,
    _In_ PCWSTR pszName,
    _In_opt_ IBindCtx *pbc,
    _In_ REFIID riid,
    _Outptr_ void **ppv)
{
    HRESULT hr;

    TRACE("(%p, %s, %p, %s, %p)\n", psiParent, debugstr_w(pszName),
          pbc, debugstr_guid(&riid), ppv);

    if (!ppv)
        return E_INVALIDARG;
    *ppv = NULL;

    if (!psiParent || !pszName)
        return E_INVALIDARG;

    /* Get the parent's IShellFolder */
    CComPtr<IShellFolder> psfParent;
    hr = psiParent->BindToHandler(pbc, BHID_SFObject, IID_PPV_ARG(IShellFolder, &psfParent));
    if (FAILED(hr))
        return hr;

    /* Parse the relative name to get child PIDL */
    CComHeapPtr<ITEMIDLIST> pidlChild;
    hr = psfParent->ParseDisplayName(NULL, pbc, const_cast<LPWSTR>(pszName),
                                     NULL, &pidlChild, NULL);
    if (FAILED(hr))
        return hr;

    /* Get the parent's absolute PIDL */
    CComHeapPtr<ITEMIDLIST> pidlParent;
    hr = SHGetIDListFromObject(psiParent, &pidlParent);
    if (FAILED(hr))
        return hr;

    /* Combine to get full absolute PIDL and create the shell item */
    CComHeapPtr<ITEMIDLIST> pidlFull(ILCombine(pidlParent, pidlChild));
    if (!pidlFull)
        return E_OUTOFMEMORY;

    return SHCreateItemFromIDList(pidlFull, riid, ppv);
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

EXTERN_C HRESULT WINAPI
SHCreateShellItemArrayFromIDLists(
    _In_ UINT cidl,
    _In_reads_(cidl) PCIDLIST_ABSOLUTE_ARRAY rgpidl,
    _Out_ IShellItemArray **ppsiItemArray)
{
    HRESULT hr;

    TRACE("(%u, %p, %p)\n", cidl, rgpidl, ppsiItemArray);

    if (!ppsiItemArray)
        return E_INVALIDARG;
    *ppsiItemArray = NULL;

    if (cidl == 0 || !rgpidl)
        return E_INVALIDARG;

    CComObject<CShellItemArrayExplicit> *pObj;
    hr = CComObject<CShellItemArrayExplicit>::CreateInstance(&pObj);
    if (FAILED(hr))
        return hr;
    pObj->AddRef();

    hr = pObj->InitializeAbsolute(cidl, rgpidl);
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

/* ---- IShellItem2 methods on CShellItem ---- */

HRESULT WINAPI CShellItem::GetPropertyStore(GETPROPERTYSTOREFLAGS flags, REFIID riid, void **ppv)
{
    WCHAR szPath[MAX_PATH];
    WIN32_FILE_ATTRIBUTE_DATA wfad;
    PROPVARIANT pv;
    CComPtr<IPropertyStore> pps;
    HRESULT hr;

    TRACE("(%p, 0x%x, %s, %p)\n", this, flags, debugstr_guid(&riid), ppv);

    if (!ppv) return E_INVALIDARG;
    *ppv = NULL;

    hr = CoCreateInstance(CLSID_InMemoryPropertyStore, NULL, CLSCTX_INPROC_SERVER,
                          IID_PPV_ARG(IPropertyStore, &pps));
    if (FAILED(hr)) return hr;

    if (SHGetPathFromIDListW(m_pidl, szPath))
    {
        PropVariantInit(&pv);
        pv.vt = VT_LPWSTR;
        if (SUCCEEDED(SHStrDupW(PathFindFileNameW(szPath), &pv.pwszVal)))
        {
            pps->SetValue(PKEY_ItemNameDisplay_L, pv);
            PropVariantClear(&pv);
        }

        if (GetFileAttributesExW(szPath, GetFileExInfoStandard, &wfad))
        {
            pv.vt = VT_UI8;
            pv.uhVal.QuadPart = ((ULONGLONG)wfad.nFileSizeHigh << 32) | wfad.nFileSizeLow;
            pps->SetValue(PKEY_Size_L, pv);

            pv.vt = VT_UI4;
            pv.ulVal = wfad.dwFileAttributes;
            pps->SetValue(PKEY_FileAttributes_L, pv);

            pv.vt = VT_FILETIME;
            pv.filetime = wfad.ftLastWriteTime;
            pps->SetValue(PKEY_DateModified_L, pv);

            pv.filetime = wfad.ftCreationTime;
            pps->SetValue(PKEY_DateCreated_L, pv);

            pv.filetime = wfad.ftLastAccessTime;
            pps->SetValue(PKEY_DateAccessed_L, pv);
        }
    }

    pps->Commit();
    return pps->QueryInterface(riid, ppv);
}

HRESULT WINAPI CShellItem::GetPropertyStoreWithCreateObject(GETPROPERTYSTOREFLAGS flags,
    IUnknown *punkCreateObject, REFIID riid, void **ppv)
{
    TRACE("(%p, 0x%x, %p, %s, %p)\n", this, flags, punkCreateObject, debugstr_guid(&riid), ppv);
    return GetPropertyStore(flags, riid, ppv);
}

HRESULT WINAPI CShellItem::GetPropertyStoreForKeys(const PROPERTYKEY *rgKeys, UINT cKeys,
    GETPROPERTYSTOREFLAGS flags, REFIID riid, void **ppv)
{
    TRACE("(%p, %p, %u, 0x%x, %s, %p)\n", this, rgKeys, cKeys, flags, debugstr_guid(&riid), ppv);
    return GetPropertyStore(flags, riid, ppv);
}

HRESULT WINAPI CShellItem::GetPropertyDescriptionList(REFPROPERTYKEY keyType, REFIID riid, void **ppv)
{
    TRACE("(%p, %p, %s, %p)\n", this, &keyType, debugstr_guid(&riid), ppv);
    if (ppv) *ppv = NULL;
    return E_NOTIMPL;
}

HRESULT WINAPI CShellItem::Update(IBindCtx *pbc)
{
    TRACE("(%p, %p)\n", this, pbc);
    return S_OK;
}

HRESULT WINAPI CShellItem::GetProperty(REFPROPERTYKEY key, PROPVARIANT *ppropvar)
{
    CComPtr<IPropertyStore> pps;
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", this, &key, ppropvar);

    if (!ppropvar) return E_INVALIDARG;
    PropVariantInit(ppropvar);

    hr = GetPropertyStore(GPS_FASTPROPERTIESONLY, IID_PPV_ARG(IPropertyStore, &pps));
    if (FAILED(hr)) return hr;
    return pps->GetValue(key, ppropvar);
}

HRESULT WINAPI CShellItem::GetCLSID(REFPROPERTYKEY key, CLSID *pclsid)
{
    PROPVARIANT pv;
    PropVariantInit(&pv);
    HRESULT hr = GetProperty(key, &pv);
    if (FAILED(hr)) return hr;
    if (pv.vt == VT_CLSID)
    {
        *pclsid = *pv.puuid;
        PropVariantClear(&pv);
        return S_OK;
    }
    PropVariantClear(&pv);
    return DISP_E_TYPEMISMATCH;
}

HRESULT WINAPI CShellItem::GetFileTime(REFPROPERTYKEY key, FILETIME *pft)
{
    PROPVARIANT pv;
    PropVariantInit(&pv);
    HRESULT hr = GetProperty(key, &pv);
    if (FAILED(hr)) return hr;
    if (pv.vt == VT_FILETIME)
    {
        *pft = pv.filetime;
        return S_OK;
    }
    PropVariantClear(&pv);
    return DISP_E_TYPEMISMATCH;
}

HRESULT WINAPI CShellItem::GetInt32(REFPROPERTYKEY key, int *pi)
{
    PROPVARIANT pv;
    PropVariantInit(&pv);
    HRESULT hr = GetProperty(key, &pv);
    if (FAILED(hr)) return hr;
    if (pv.vt == VT_I4) { *pi = pv.lVal; return S_OK; }
    PropVariantClear(&pv);
    return DISP_E_TYPEMISMATCH;
}

HRESULT WINAPI CShellItem::GetString(REFPROPERTYKEY key, LPWSTR *ppsz)
{
    PROPVARIANT pv;
    PropVariantInit(&pv);
    HRESULT hr = GetProperty(key, &pv);
    if (FAILED(hr)) return hr;
    if (pv.vt == VT_LPWSTR)
    {
        *ppsz = pv.pwszVal;
        return S_OK;
    }
    if (pv.vt == VT_BSTR)
    {
        hr = SHStrDupW(pv.bstrVal, ppsz);
        PropVariantClear(&pv);
        return hr;
    }
    PropVariantClear(&pv);
    return DISP_E_TYPEMISMATCH;
}

HRESULT WINAPI CShellItem::GetUInt32(REFPROPERTYKEY key, ULONG *pui)
{
    PROPVARIANT pv;
    PropVariantInit(&pv);
    HRESULT hr = GetProperty(key, &pv);
    if (FAILED(hr)) return hr;
    if (pv.vt == VT_UI4) { *pui = pv.ulVal; return S_OK; }
    PropVariantClear(&pv);
    return DISP_E_TYPEMISMATCH;
}

HRESULT WINAPI CShellItem::GetUInt64(REFPROPERTYKEY key, ULONGLONG *pull)
{
    PROPVARIANT pv;
    PropVariantInit(&pv);
    HRESULT hr = GetProperty(key, &pv);
    if (FAILED(hr)) return hr;
    if (pv.vt == VT_UI8) { *pull = pv.uhVal.QuadPart; return S_OK; }
    PropVariantClear(&pv);
    return DISP_E_TYPEMISMATCH;
}

HRESULT WINAPI CShellItem::GetBool(REFPROPERTYKEY key, BOOL *pf)
{
    PROPVARIANT pv;
    PropVariantInit(&pv);
    HRESULT hr = GetProperty(key, &pv);
    if (FAILED(hr)) return hr;
    if (pv.vt == VT_BOOL) { *pf = (pv.boolVal != VARIANT_FALSE); return S_OK; }
    PropVariantClear(&pv);
    return DISP_E_TYPEMISMATCH;
}

EXTERN_C HRESULT WINAPI
SHGetPropertyStoreFromIDList(
    _In_ PCIDLIST_ABSOLUTE pidl,
    _In_ GETPROPERTYSTOREFLAGS flags,
    _In_ REFIID riid,
    _Out_ void **ppv)
{
    CComPtr<IShellItem2> psi2;
    HRESULT hr;

    TRACE("(%p, 0x%x, %s, %p)\n", pidl, flags, debugstr_guid(&riid), ppv);

    if (!ppv) return E_INVALIDARG;
    *ppv = NULL;

    hr = SHCreateItemFromIDList(pidl, IID_PPV_ARG(IShellItem2, &psi2));
    if (FAILED(hr)) return hr;
    return psi2->GetPropertyStore(flags, riid, ppv);
}

EXTERN_C HRESULT WINAPI
SHGetPropertyStoreForWindow(
    _In_ HWND hwnd,
    _In_ REFIID riid,
    _Out_ void **ppv)
{
    TRACE("(%p, %s, %p)\n", hwnd, debugstr_guid(&riid), ppv);

    if (!ppv) return E_INVALIDARG;
    *ppv = NULL;

    return CoCreateInstance(CLSID_InMemoryPropertyStore, NULL, CLSCTX_INPROC_SERVER, riid, ppv);
}
