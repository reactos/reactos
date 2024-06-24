/*
 * PROJECT:     ReactOS shell32
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     CopyTo and MoveTo implementation
 * COPYRIGHT:   Copyright 2020-2023 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#pragma once

class CCopyMoveToMenu :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IContextMenu2,
    public IObjectWithSite,
    public IShellExtInit
{
protected:
    CComPtr<IDataObject> m_pDataObject;
    CComPtr<IUnknown> m_pSite;

    HRESULT DoRealFileOp(const CIDA *pCIDA, LPCMINVOKECOMMANDINFO lpici, PCUIDLIST_ABSOLUTE pidlDestination);
    HRESULT DoAction(LPCMINVOKECOMMANDINFO lpici);

public:
    CComHeapPtr<ITEMIDLIST> m_pidlFolder;
    WNDPROC m_fnOldWndProc;
    BOOL m_bIgnoreTextBoxChange;

    CCopyMoveToMenu();

    virtual UINT GetCaptionStringID() const = 0;
    virtual UINT GetButtonStringID() const = 0;
    virtual UINT GetActionTitleStringID() const = 0;
    virtual UINT GetFileOp() const = 0;
    virtual LPCSTR GetVerb() const = 0;
    STDMETHODIMP QueryContextMenuImpl(BOOL IsCopyOp, HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);

    // IContextMenu
    STDMETHODIMP GetCommandString(UINT_PTR idCommand, UINT uFlags, UINT *lpReserved, LPSTR lpszName, UINT uMaxNameLen) override;
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi) override;

    // IContextMenu2
    STDMETHODIMP HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

    // IShellExtInit
    STDMETHODIMP Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID) override;

    // IObjectWithSite
    STDMETHODIMP SetSite(IUnknown *pUnkSite) override;
    STDMETHODIMP GetSite(REFIID riid, void **ppvSite) override;
};

class CCopyToMenu
    : public CComCoClass<CCopyToMenu, &CLSID_CopyToMenu>
    , public CCopyMoveToMenu
{
public:
    CComHeapPtr<ITEMIDLIST> m_pidlFolder;
    WNDPROC m_fnOldWndProc;
    BOOL m_bIgnoreTextBoxChange;

    CCopyToMenu() { }

    // IContextMenu
    STDMETHODIMP QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags) override;

    DECLARE_REGISTRY_RESOURCEID(IDR_COPYTOMENU)
    DECLARE_NOT_AGGREGATABLE(CCopyToMenu)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CCopyToMenu)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu2, IContextMenu2)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
        COM_INTERFACE_ENTRY_IID(IID_IShellExtInit, IShellExtInit)
        COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
    END_COM_MAP()

    UINT GetCaptionStringID() const override { return IDS_COPYITEMS; }
    UINT GetButtonStringID() const override { return IDS_COPYBUTTON; }
    UINT GetActionTitleStringID() const override { return IDS_COPYTOTITLE; }
    UINT GetFileOp() const override { return FO_COPY; }
    LPCSTR GetVerb() const override { return "copyto"; }
};

class CMoveToMenu
    : public CComCoClass<CMoveToMenu, &CLSID_MoveToMenu>
    , public CCopyMoveToMenu
{
public:
    CMoveToMenu() { }

    // IContextMenu
    STDMETHODIMP QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags) override;

    DECLARE_REGISTRY_RESOURCEID(IDR_MOVETOMENU)
    DECLARE_NOT_AGGREGATABLE(CMoveToMenu)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CMoveToMenu)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu2, IContextMenu2)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
        COM_INTERFACE_ENTRY_IID(IID_IShellExtInit, IShellExtInit)
        COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
    END_COM_MAP()

    UINT GetCaptionStringID() const override { return IDS_MOVEITEMS; }
    UINT GetButtonStringID() const override { return IDS_MOVEBUTTON; }
    UINT GetActionTitleStringID() const override { return IDS_MOVETOTITLE; }
    UINT GetFileOp() const override { return FO_MOVE; }
    LPCSTR GetVerb() const override { return "moveto"; }
};
