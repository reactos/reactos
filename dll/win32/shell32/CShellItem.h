/*
 * IShellItem implementation
 *
 * Copyright 2008 Vincent Povirk for CodeWeavers
 * Copyright 2009 Andrew Hill
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

#ifndef _SHELLITEM_H_
#define _SHELLITEM_H_

class CShellItem :
    public CComCoClass<CShellItem, &CLSID_ShellItem>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellItem2,
    public IPersistIDList
{
private:
    LPITEMIDLIST        m_pidl;

public:
    CShellItem();
    ~CShellItem();
    HRESULT get_parent_pidl(LPITEMIDLIST *parent_pidl);
    HRESULT get_parent_shellfolder(IShellFolder **ppsf);
    HRESULT get_shellfolder(IBindCtx *pbc, REFIID riid, void **ppvOut);
    HRESULT get_property_from_folder(REFPROPERTYKEY key, VARIANT *pvar);
    HRESULT get_converted_property(REFPROPERTYKEY key, PROPVARIANT *ppropvar, VARTYPE vt);

    // IShellItem
    STDMETHOD(BindToHandler)(IBindCtx *pbc, REFGUID rbhid, REFIID riid, void **ppvOut) override;
    STDMETHOD(GetParent)(IShellItem **ppsi) override;
    STDMETHOD(GetDisplayName)(SIGDN sigdnName, LPWSTR *ppszName) override;
    STDMETHOD(GetAttributes)(SFGAOF sfgaoMask, SFGAOF *psfgaoAttribs) override;
    STDMETHOD(Compare)(IShellItem *oth, SICHINTF hint, int *piOrder) override;

    // IShellItem2
    STDMETHOD(GetPropertyStore)(_In_ GETPROPERTYSTOREFLAGS flags, _In_ REFIID riid, _Out_ void **ppv);
    STDMETHOD(GetPropertyStoreWithCreateObject)(_In_ GETPROPERTYSTOREFLAGS flags, _In_ IUnknown *punkCreateObject, _In_ REFIID riid, _Out_ void **ppv) override;
    STDMETHOD(GetPropertyStoreForKeys)(const PROPERTYKEY *rgKeys, _In_ UINT cKeys, _In_ GETPROPERTYSTOREFLAGS flags, _In_ REFIID riid, _Out_ void **ppv) override;
    STDMETHOD(GetPropertyDescriptionList)(_In_ REFPROPERTYKEY keyType, _In_ REFIID riid, _Out_ void **ppv) override;
    STDMETHOD(Update)(_In_ IBindCtx *pbc) override;
    STDMETHOD(GetProperty)(_In_ REFPROPERTYKEY key, _Out_ PROPVARIANT *ppropvar) override;
    STDMETHOD(GetCLSID)(_In_ REFPROPERTYKEY key, _Out_ CLSID *pclsid) override;
    STDMETHOD(GetFileTime)(_In_ REFPROPERTYKEY key, _Out_ FILETIME *pft) override;
    STDMETHOD(GetInt32)(_In_ REFPROPERTYKEY key, _Out_ int *pi) override;
    STDMETHOD(GetString)(_In_ REFPROPERTYKEY key, _Out_ LPWSTR *ppsz) override;
    STDMETHOD(GetUInt32)(_In_ REFPROPERTYKEY key, _Out_ ULONG *pui) override;
    STDMETHOD(GetUInt64)(_In_ REFPROPERTYKEY key, _Out_ ULONGLONG *pull) override;
    STDMETHOD(GetBool)(_In_ REFPROPERTYKEY key, _Out_ BOOL *pf) override;

    // IPersistIDList
    STDMETHOD(GetClassID)(CLSID *pClassID) override;
    STDMETHOD(SetIDList)(PCIDLIST_ABSOLUTE pidl) override;
    STDMETHOD(GetIDList)(PIDLIST_ABSOLUTE *ppidl) override;

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CShellItem)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CShellItem)
    COM_INTERFACE_ENTRY_IID(IID_IShellItem, IShellItem)
    COM_INTERFACE_ENTRY_IID(IID_IShellItem2, IShellItem2)
    COM_INTERFACE_ENTRY_IID(IID_IPersistIDList, IPersistIDList)
END_COM_MAP()
};

#endif /* _SHELLITEM_H_ */
