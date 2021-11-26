/*
 * PROJECT:     shell32
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     CopyTo and MoveTo implementation
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#pragma once


class CCopyToMenu :
    public CComCoClass<CCopyToMenu, &CLSID_CopyToMenu>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IContextMenu2,
    public IObjectWithSite,
    public IShellExtInit
{
protected:
    UINT m_idCmdFirst, m_idCmdLast, m_idCmdCopyTo;
    CComPtr<IDataObject> m_pDataObject;
    CComPtr<IUnknown> m_pSite;

    HRESULT DoCopyToFolder(LPCMINVOKECOMMANDINFO lpici);
    HRESULT DoRealCopy(LPCMINVOKECOMMANDINFO lpici, PCUIDLIST_ABSOLUTE pidl);
    CStringW DoGetFileTitle();

public:
    CComHeapPtr<ITEMIDLIST> m_pidlFolder;
    WNDPROC m_fnOldWndProc;
    BOOL m_bIgnoreTextBoxChange;

    CCopyToMenu();
    ~CCopyToMenu();

    // IContextMenu
    virtual HRESULT WINAPI QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    virtual HRESULT WINAPI InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi);
    virtual HRESULT WINAPI GetCommandString(UINT_PTR idCommand, UINT uFlags, UINT *lpReserved, LPSTR lpszName, UINT uMaxNameLen);

    // IContextMenu2
    virtual HRESULT WINAPI HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // IShellExtInit
    virtual HRESULT WINAPI Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID);

    // IObjectWithSite
    virtual HRESULT WINAPI SetSite(IUnknown *pUnkSite);
    virtual HRESULT WINAPI GetSite(REFIID riid, void **ppvSite);

    DECLARE_REGISTRY_RESOURCEID(IDR_COPYTOMENU)
    DECLARE_NOT_AGGREGATABLE(CCopyToMenu)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CCopyToMenu)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu2, IContextMenu2)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
        COM_INTERFACE_ENTRY_IID(IID_IShellExtInit, IShellExtInit)
        COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
    END_COM_MAP()
};

class CMoveToMenu :
    public CComCoClass<CMoveToMenu, &CLSID_MoveToMenu>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IContextMenu2,
    public IObjectWithSite,
    public IShellExtInit
{
protected:
    UINT m_idCmdFirst, m_idCmdLast, m_idCmdMoveTo;
    CComPtr<IDataObject> m_pDataObject;
    CComPtr<IUnknown> m_pSite;

    HRESULT DoMoveToFolder(LPCMINVOKECOMMANDINFO lpici);
    HRESULT DoRealMove(LPCMINVOKECOMMANDINFO lpici, PCUIDLIST_ABSOLUTE pidl);
    CStringW DoGetFileTitle();

public:
    CComHeapPtr<ITEMIDLIST> m_pidlFolder;
    WNDPROC m_fnOldWndProc;
    BOOL m_bIgnoreTextBoxChange;

    CMoveToMenu();
    ~CMoveToMenu();

    // IContextMenu
    virtual HRESULT WINAPI QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    virtual HRESULT WINAPI InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi);
    virtual HRESULT WINAPI GetCommandString(UINT_PTR idCommand, UINT uFlags, UINT *lpReserved, LPSTR lpszName, UINT uMaxNameLen);

    // IContextMenu2
    virtual HRESULT WINAPI HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // IShellExtInit
    virtual HRESULT WINAPI Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID);

    // IObjectWithSite
    virtual HRESULT WINAPI SetSite(IUnknown *pUnkSite);
    virtual HRESULT WINAPI GetSite(REFIID riid, void **ppvSite);

    DECLARE_REGISTRY_RESOURCEID(IDR_MOVETOMENU)
    DECLARE_NOT_AGGREGATABLE(CMoveToMenu)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CMoveToMenu)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu2, IContextMenu2)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
        COM_INTERFACE_ENTRY_IID(IID_IShellExtInit, IShellExtInit)
        COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
    END_COM_MAP()
};
