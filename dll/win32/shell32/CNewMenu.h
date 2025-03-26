/*
 * provides new shell item service
 *
 * Copyright 2007 Johannes Anderwald (johannes.anderwald@reactos.org)
 * Copyright 2009 Andrew Hill
 * Copyright 2012 Rafal Harabien
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

#ifndef _SHV_ITEM_NEW_H_
#define _SHV_ITEM_NEW_H_

const WCHAR ShellNewKey[] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Discardable\\PostSetup\\ShellNew";

class CNewMenu :
    public CComCoClass<CNewMenu, &CLSID_NewMenu>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IObjectWithSite,
    public IContextMenu3,
    public IShellExtInit
{
private:
    enum SHELLNEW_TYPE
    {
        SHELLNEW_TYPE_INVALID = -1,
        SHELLNEW_TYPE_COMMAND = 1,
        SHELLNEW_TYPE_DATA = 2,
        SHELLNEW_TYPE_FILENAME = 4,
        SHELLNEW_TYPE_NULLFILE = 8
    };

    struct SHELLNEW_ITEM
    {
        SHELLNEW_TYPE Type;
        LPWSTR pwszExt;
        PBYTE pData;
        ULONG cbData;
        LPWSTR pwszDesc;
        HICON hIcon;
        SHELLNEW_ITEM *pNext;
    };

    LPITEMIDLIST m_pidlFolder;
    SHELLNEW_ITEM *m_pItems;
    SHELLNEW_ITEM *m_pLinkItem; // Points to the link handler item in the m_pItems list.
    CComPtr<IUnknown> m_pSite;
    HMENU m_hSubMenu;
    UINT m_idCmdFirst, m_idCmdFolder, m_idCmdLink;
    BOOL m_bCustomIconFolder, m_bCustomIconLink;
    HICON m_hIconFolder, m_hIconLink;

    SHELLNEW_ITEM *LoadItem(LPCWSTR pwszExt);
    void UnloadItem(SHELLNEW_ITEM *pItem);
    void UnloadAllItems();
    BOOL CacheItems();
    BOOL LoadCachedItems();
    BOOL LoadAllItems();
    UINT InsertShellNewItems(HMENU hMenu, UINT idFirst, UINT idMenu);
    SHELLNEW_ITEM *FindItemFromIdOffset(UINT IdOffset);
    HRESULT CreateNewFolder(LPCMINVOKECOMMANDINFO lpici);
    HRESULT CreateNewItem(SHELLNEW_ITEM *pItem, LPCMINVOKECOMMANDINFO lpcmi);
    HRESULT SelectNewItem(LONG wEventId, UINT uFlags, LPWSTR pszName, BOOL bRename);
    HRESULT NewItemByCommand(SHELLNEW_ITEM *pItem, LPCWSTR wszPath);
    HRESULT NewItemByNonCommand(SHELLNEW_ITEM *pItem, LPWSTR wszName,
                                DWORD cchNameMax, LPCWSTR wszPath);

public:
    CNewMenu();
    ~CNewMenu();

    // IObjectWithSite
    STDMETHOD(SetSite)(IUnknown *pUnkSite) override;
    STDMETHOD(GetSite)(REFIID riid, void **ppvSite) override;

    // IContextMenu
    STDMETHOD(QueryContextMenu)(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags) override;
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpcmi) override;
    STDMETHOD(GetCommandString)(UINT_PTR idCommand, UINT uFlags, UINT *lpReserved, LPSTR lpszName, UINT uMaxNameLen) override;

    // IContextMenu3
    STDMETHOD(HandleMenuMsg2)(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult) override;

    // IContextMenu2
    STDMETHOD(HandleMenuMsg)(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

    // IShellExtInit
    STDMETHOD(Initialize)(PCIDLIST_ABSOLUTE pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID) override;

DECLARE_REGISTRY_RESOURCEID(IDR_NEWMENU)
DECLARE_NOT_AGGREGATABLE(CNewMenu)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CNewMenu)
    COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
    COM_INTERFACE_ENTRY_IID(IID_IContextMenu3, IContextMenu3)
    COM_INTERFACE_ENTRY_IID(IID_IContextMenu2, IContextMenu2)
    COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
    COM_INTERFACE_ENTRY_IID(IID_IShellExtInit, IShellExtInit)
END_COM_MAP()
};

#endif /* _SHV_ITEM_NEW_H_ */
