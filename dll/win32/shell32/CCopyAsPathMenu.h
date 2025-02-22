/*
 * PROJECT:     ReactOS shell32
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Copy as Path Menu implementation
 * COPYRIGHT:   Copyright 2024 Whindmar Saksit <whindsaks@proton.me>
 *              Copyright 2024 Thamatip Chitpong <thamatip.chitpong@reactos.org>
 */

#pragma once

class CCopyAsPathMenu :
    public CComCoClass<CCopyAsPathMenu, &CLSID_CopyAsPathMenu>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IContextMenu,
    public IShellExtInit
{
private:
    CComPtr<IDataObject> m_pDataObject;

    HRESULT DoCopyAsPath(IDataObject *pdto);

public:
    CCopyAsPathMenu();
    ~CCopyAsPathMenu();

    // IContextMenu
    STDMETHODIMP QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags) override;
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi) override;
    STDMETHODIMP GetCommandString(UINT_PTR idCommand, UINT uFlags, UINT *lpReserved, LPSTR lpszName, UINT uMaxNameLen) override;

    // IShellExtInit
    STDMETHODIMP Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID) override;

    DECLARE_REGISTRY_RESOURCEID(IDR_COPYASPATHMENU)
    DECLARE_NOT_AGGREGATABLE(CCopyAsPathMenu)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CCopyAsPathMenu)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
        COM_INTERFACE_ENTRY_IID(IID_IShellExtInit, IShellExtInit)
    END_COM_MAP()
};
