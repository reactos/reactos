/*
 * ReactOS Explorer
 *
 * Copyright 2009 Andrew Hill <ash77 at domain reactos.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#pragma once

class CRegTreeOptions :
    public CComCoClass<CRegTreeOptions, &CLSID_CRegTreeOptions>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IRegTreeOptions,
    public IObjectWithSite
{
private:
public:
    CRegTreeOptions();
    ~CRegTreeOptions();

    // *** IRegTreeOptions methods ***
    virtual HRESULT STDMETHODCALLTYPE InitTree(HWND paramC, HKEY param10, char const *param14, char const *param18);
    virtual HRESULT STDMETHODCALLTYPE WalkTree(WALK_TREE_CMD paramC);
    virtual HRESULT STDMETHODCALLTYPE ToggleItem(HTREEITEM paramC);
    virtual HRESULT STDMETHODCALLTYPE ShowHelp(HTREEITEM paramC, unsigned long param10);

    // *** IObjectWithSite methods ***
    virtual HRESULT STDMETHODCALLTYPE SetSite(IUnknown *pUnkSite);
    virtual HRESULT STDMETHODCALLTYPE GetSite(REFIID riid, void **ppvSite);

    DECLARE_REGISTRY_RESOURCEID(IDR_REGTREEOPTIONS)
    DECLARE_NOT_AGGREGATABLE(CRegTreeOptions)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CRegTreeOptions)
        COM_INTERFACE_ENTRY_IID(IID_IRegTreeOptions, IRegTreeOptions)
        COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
    END_COM_MAP()
};
