/*
 * provides new shell item service
 *
 * Copyright 2019 Katayama Hirofumi MZ.
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

#ifndef _SHV_ITEM_SENDTO_H_
#define _SHV_ITEM_SENDTO_H_

extern "C" const GUID CLSID_SendToMenu;

class CSendToMenu :
    public CComCoClass<CSendToMenu, &CLSID_SendToMenu>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IContextMenu3,
    public IShellExtInit
{
private:
    struct SENDTO_ITEM
    {
        SENDTO_ITEM *pNext;
        PITEMID_CHILD pidlChild;
        LPWSTR pszText;
        HICON hIcon;

        SENDTO_ITEM(PITEMID_CHILD child, LPWSTR text, HICON icon)
            : pNext(NULL)
            , pidlChild(child)
            , pszText(text)
            , hIcon(icon)
        {
        }

        ~SENDTO_ITEM()
        {
            CoTaskMemFree(pidlChild);
            CoTaskMemFree(pszText);
            DestroyIcon(hIcon);
        }

    private:
        SENDTO_ITEM();
        SENDTO_ITEM(const SENDTO_ITEM&);
        SENDTO_ITEM& operator=(const SENDTO_ITEM&);
    };

    HMENU m_hSubMenu;
    SENDTO_ITEM *m_pItems;
    UINT m_idCmdFirst;

    CComPtr<IShellFolder> m_pDesktop;
    CComPtr<IShellFolder> m_pSendTo;
    CComPtr<IDataObject> m_pDataObject;

    HRESULT LoadAllItems(HWND hwnd);
    void UnloadAllItems();

    BOOL FolderHasAnyItems() const;
    HRESULT CreateSendToFiles(LPCWSTR pszSendTo);

    UINT InsertSendToItems(HMENU hMenu, UINT idFirst, UINT idMenu);

    SENDTO_ITEM *FindItemFromIdOffset(UINT IdOffset);
    HRESULT DoSendToItem(SENDTO_ITEM *pItem, LPCMINVOKECOMMANDINFO lpici);

    HRESULT DoDrop(IDataObject *pDataObject, IDropTarget *pDropTarget);
    HRESULT GetSpecialFolder(HWND hwnd, IShellFolder **ppFolder, int csidl,
                             PIDLIST_ABSOLUTE *ppidl = NULL);
    HRESULT GetUIObjectFromPidl(HWND hwnd, PIDLIST_ABSOLUTE pidl, REFIID riid, LPVOID *ppvOut);

public:
    CSendToMenu();
    ~CSendToMenu();

    // IContextMenu
    STDMETHODIMP QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi);
    STDMETHODIMP GetCommandString(UINT_PTR idCommand, UINT uFlags, UINT *lpReserved, LPSTR lpszName, UINT uMaxNameLen);

    // IContextMenu3
    STDMETHODIMP HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);

    // IContextMenu2
    STDMETHODIMP HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // IShellExtInit
    STDMETHODIMP Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID);

    DECLARE_REGISTRY_RESOURCEID(IDR_SENDTOMENU)
    DECLARE_NOT_AGGREGATABLE(CSendToMenu)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSendToMenu)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu3, IContextMenu3)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu2, IContextMenu2)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
        COM_INTERFACE_ENTRY_IID(IID_IShellExtInit, IShellExtInit)
    END_COM_MAP()
};

#endif /* _SHV_ITEM_SENDTO_H_ */
