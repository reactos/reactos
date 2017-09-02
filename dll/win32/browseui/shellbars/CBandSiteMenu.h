/*
 *  Band site menu
 *
 *  Copyright 2007  Hervé Poussineua
 *  Copyright 2009  Andrew Hill
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

#pragma once

// oddly, this class also responds to QueryInterface for CLSID_BandSiteMenu by returning the vtable at offset 0
class CBandSiteMenu :
    public CComCoClass<CBandSiteMenu, &CLSID_BandSiteMenu>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IContextMenu3,
    public IShellService
{
    CComPtr<IBandSite> m_BandSite;
    HDSA m_menuDsa;
    HMENU m_hmenu;

    HRESULT CreateMenuPart();

public:
    CBandSiteMenu();
    ~CBandSiteMenu();

    // *** IShellService methods ***
    virtual HRESULT STDMETHODCALLTYPE SetOwner(IUnknown *);

    // *** IContextMenu methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    virtual HRESULT STDMETHODCALLTYPE InvokeCommand(LPCMINVOKECOMMANDINFO lpici);
    virtual HRESULT STDMETHODCALLTYPE GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax);

    // *** IContextMenu2 methods ***
    virtual HRESULT STDMETHODCALLTYPE HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // *** IContextMenu3 methods ***
    virtual HRESULT STDMETHODCALLTYPE HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);

    DECLARE_REGISTRY_RESOURCEID(IDR_BANDSITEMENU)
    DECLARE_NOT_AGGREGATABLE(CBandSiteMenu)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CBandSiteMenu)
        COM_INTERFACE_ENTRY_IID(IID_IShellService, IShellService)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu2, IContextMenu2)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
    END_COM_MAP()
};
