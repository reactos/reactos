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

class CBandProxy :
    public CComCoClass<CBandProxy, &CLSID_BandProxy>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IBandProxy
{
private:
    CComPtr<IUnknown>                       fSite;
public:
    CBandProxy();
    ~CBandProxy();
    HRESULT FindBrowserWindow(IUnknown **browser);

    // *** IBandProxy methods ***
    virtual HRESULT STDMETHODCALLTYPE SetSite(IUnknown *paramC);
    virtual HRESULT STDMETHODCALLTYPE CreateNewWindow(long paramC);
    virtual HRESULT STDMETHODCALLTYPE GetBrowserWindow(IUnknown **paramC);
    virtual HRESULT STDMETHODCALLTYPE IsConnected();
    virtual HRESULT STDMETHODCALLTYPE NavigateToPIDL(LPCITEMIDLIST pidl);
    virtual HRESULT STDMETHODCALLTYPE NavigateToURL(long paramC, long param10);

    DECLARE_REGISTRY_RESOURCEID(IDR_BANDPROXY)
    DECLARE_NOT_AGGREGATABLE(CBandProxy)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CBandProxy)
        COM_INTERFACE_ENTRY_IID(IID_IBandProxy, IBandProxy)
    END_COM_MAP()
};
