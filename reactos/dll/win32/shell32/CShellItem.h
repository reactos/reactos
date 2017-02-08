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
    public IShellItem,
    public IPersistIDList
{
private:
    LPITEMIDLIST        m_pidl;

public:
    CShellItem();
    ~CShellItem();
    HRESULT get_parent_pidl(LPITEMIDLIST *parent_pidl);
    HRESULT get_parent_shellfolder(IShellFolder **ppsf);

    // IShellItem
    virtual HRESULT WINAPI BindToHandler(IBindCtx *pbc, REFGUID rbhid, REFIID riid, void **ppvOut);
    virtual HRESULT WINAPI GetParent(IShellItem **ppsi);
    virtual HRESULT WINAPI GetDisplayName(SIGDN sigdnName, LPWSTR *ppszName);
    virtual HRESULT WINAPI GetAttributes(SFGAOF sfgaoMask, SFGAOF *psfgaoAttribs);
    virtual HRESULT WINAPI Compare(IShellItem *oth, SICHINTF hint, int *piOrder);

    // IPersistIDList
    virtual HRESULT WINAPI GetClassID(CLSID *pClassID);
    virtual HRESULT WINAPI SetIDList(LPCITEMIDLIST pidl);
    virtual HRESULT WINAPI GetIDList(LPITEMIDLIST *ppidl);

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CShellItem)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CShellItem)
    COM_INTERFACE_ENTRY_IID(IID_IShellItem, IShellItem)
    COM_INTERFACE_ENTRY_IID(IID_IPersistIDList, IPersistIDList)
END_COM_MAP()
};

#endif /* _SHELLITEM_H_ */
