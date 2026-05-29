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

    // IShellItem
    STDMETHOD(BindToHandler)(IBindCtx *pbc, REFGUID rbhid, REFIID riid, void **ppvOut) override;
    STDMETHOD(GetParent)(IShellItem **ppsi) override;
    STDMETHOD(GetDisplayName)(SIGDN sigdnName, LPWSTR *ppszName) override;
    STDMETHOD(GetAttributes)(SFGAOF sfgaoMask, SFGAOF *psfgaoAttribs) override;
    STDMETHOD(Compare)(IShellItem *oth, SICHINTF hint, int *piOrder) override;

    // IShellItem2
    STDMETHOD(GetPropertyStore)(GETPROPERTYSTOREFLAGS flags, REFIID riid, void **ppv) override;
    STDMETHOD(GetPropertyStoreWithCreateObject)(GETPROPERTYSTOREFLAGS flags, IUnknown *punkCreateObject, REFIID riid, void **ppv) override;
    STDMETHOD(GetPropertyStoreForKeys)(const PROPERTYKEY *rgKeys, UINT cKeys, GETPROPERTYSTOREFLAGS flags, REFIID riid, void **ppv) override;
    STDMETHOD(GetPropertyDescriptionList)(REFPROPERTYKEY keyType, REFIID riid, void **ppv) override;
    STDMETHOD(Update)(IBindCtx *pbc) override;
    STDMETHOD(GetProperty)(REFPROPERTYKEY key, PROPVARIANT *ppropvar) override;
    STDMETHOD(GetCLSID)(REFPROPERTYKEY key, CLSID *pclsid) override;
    STDMETHOD(GetFileTime)(REFPROPERTYKEY key, FILETIME *pft) override;
    STDMETHOD(GetInt32)(REFPROPERTYKEY key, int *pi) override;
    STDMETHOD(GetString)(REFPROPERTYKEY key, LPWSTR *ppsz) override;
    STDMETHOD(GetUInt32)(REFPROPERTYKEY key, ULONG *pui) override;
    STDMETHOD(GetUInt64)(REFPROPERTYKEY key, ULONGLONG *pull) override;
    STDMETHOD(GetBool)(REFPROPERTYKEY key, BOOL *pf) override;

    // IPersistIDList
    STDMETHOD(GetClassID)(CLSID *pClassID) override;
    STDMETHOD(SetIDList)(PCIDLIST_ABSOLUTE pidl) override;
    STDMETHOD(GetIDList)(PIDLIST_ABSOLUTE *ppidl) override;

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CShellItem)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CShellItem)
    COM_INTERFACE_ENTRY_IID(IID_IShellItem2, IShellItem2)
    COM_INTERFACE_ENTRY_IID(IID_IShellItem, IShellItem)
    COM_INTERFACE_ENTRY_IID(IID_IPersistIDList, IPersistIDList)
END_COM_MAP()
};

#endif /* _SHELLITEM_H_ */
