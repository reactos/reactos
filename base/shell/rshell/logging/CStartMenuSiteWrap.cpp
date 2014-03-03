/*
 * Shell Menu Site
 *
 * Copyright 2014 David Quintana
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

#include "precomp.h"
#include "wraplog.h"

WINE_DEFAULT_DEBUG_CHANNEL(menusite);

class CStartMenuSiteWrap :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IServiceProvider,
    public ITrayPriv,
    public IOleWindow,
    public IOleCommandTarget
{
public:
    CStartMenuSiteWrap() {}
    ~CStartMenuSiteWrap();

    HRESULT InitWrap(ITrayPriv * bandSite);

private:
    CComPtr<IServiceProvider > m_IServiceProvider;
    CComPtr<ITrayPriv        > m_ITrayPriv;
    CComPtr<IOleWindow       > m_IOleWindow;
    CComPtr<IOleCommandTarget> m_IOleCommandTarget;

public:
    // IServiceProvider
    virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void **ppvObject);

    // ITrayPriv
    virtual HRESULT STDMETHODCALLTYPE Execute(THIS_ IShellFolder*, LPCITEMIDLIST);
    virtual HRESULT STDMETHODCALLTYPE Unknown(THIS_ PVOID, PVOID, PVOID, PVOID);
    virtual HRESULT STDMETHODCALLTYPE AppendMenu(THIS_ HMENU*);

    // IOleWindow
    virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *phwnd);
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

    // IOleCommandTarget
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID * pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText);
    virtual HRESULT STDMETHODCALLTYPE Exec(const GUID * pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

    DECLARE_NOT_AGGREGATABLE(CStartMenuSiteWrap)
    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(CStartMenuSiteWrap)
        COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
        COM_INTERFACE_ENTRY_IID(IID_ITrayPriv, ITrayPriv)
        COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
        COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
    END_COM_MAP()
};

extern "C"
HRESULT WINAPI CStartMenuSite_Wrapper(ITrayPriv * trayPriv, REFIID riid, LPVOID *ppv)
{
    HRESULT hr;

    *ppv = NULL;

    CStartMenuSiteWrap * site = new CComObject<CStartMenuSiteWrap>();

    if (!site)
        return E_OUTOFMEMORY;

    hr = site->InitWrap(trayPriv);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        site->Release();
        return hr;
    }

    hr = site->QueryInterface(riid, ppv);

    if (FAILED_UNEXPECTEDLY(hr))
        site->Release();

    return hr;
}

HRESULT CStartMenuSiteWrap::InitWrap(ITrayPriv * bandSite)
{
    HRESULT hr;

    WrapLogOpen();

    m_ITrayPriv = bandSite;

    hr = bandSite->QueryInterface(IID_PPV_ARG(IServiceProvider, &m_IServiceProvider));
    if (FAILED_UNEXPECTEDLY(hr)) return hr;
    hr = bandSite->QueryInterface(IID_PPV_ARG(IOleWindow, &m_IOleWindow));
    if (FAILED_UNEXPECTEDLY(hr)) return hr;
    hr = bandSite->QueryInterface(IID_PPV_ARG(IOleCommandTarget, &m_IOleCommandTarget));
    return hr;
}

CStartMenuSiteWrap::~CStartMenuSiteWrap()
{
    WrapLogClose();
}

// *** IServiceProvider methods ***
HRESULT STDMETHODCALLTYPE CStartMenuSiteWrap::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    WrapLogEnter("CStartMenuSiteWrap<%p>::QueryService(REFGUID guidService=%s, REFIID riid=%s, void **ppvObject=%p)\n", this, Wrap(guidService), Wrap(riid), ppvObject);
    HRESULT hr = m_IServiceProvider->QueryService(guidService, riid, ppvObject);
    if (ppvObject) WrapLogPost("*ppvObject=%p\n", *ppvObject);
    WrapLogExit("CStartMenuSiteWrap::QueryService()", hr);
    return hr;
}

// *** ITrayPriv methods ***
HRESULT STDMETHODCALLTYPE CStartMenuSiteWrap::Execute(IShellFolder* psf, LPCITEMIDLIST pidl)
{
    WrapLogEnter("CStartMenuSiteWrap<%p>::Execute(IShellFolder* psf=%p, LPCITEMIDLIST pidl=%p)\n", this, psf, pidl);
    HRESULT hr = m_ITrayPriv->Execute(psf, pidl);
    WrapLogExit("CStartMenuSiteWrap::Execute()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CStartMenuSiteWrap::Unknown(PVOID unk1, PVOID unk2, PVOID unk3, PVOID unk4)
{
    WrapLogEnter("CStartMenuSiteWrap<%p>::Unknown(PVOID unk1=%p, PVOID unk2=%p, PVOID unk3=%p, PVOID unk4=%p)\n", this, unk1, unk2, unk3, unk4);
    HRESULT hr = m_ITrayPriv->Unknown(unk1, unk2, unk3, unk4);
    WrapLogExit("CStartMenuSiteWrap::Unknown()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CStartMenuSiteWrap::AppendMenu(HMENU * phmenu)
{
    WrapLogEnter("CStartMenuSiteWrap<%p>::AppendMenu(HMENU * phmenu=%p)\n", this, phmenu);
    HRESULT hr = m_ITrayPriv->AppendMenu(phmenu);
    WrapLogExit("CStartMenuSiteWrap::AppendMenu()", hr);
    return hr;
}

// *** IOleWindow methods ***
HRESULT STDMETHODCALLTYPE CStartMenuSiteWrap::GetWindow(HWND *phwnd)
{
    WrapLogEnter("CStartMenuSiteWrap<%p>::GetWindow(HWND *phwnd=%p)\n", this, phwnd);
    HRESULT hr = m_IOleWindow->GetWindow(phwnd);
    if (phwnd) WrapLogPost("*phwnd=%p\n", *phwnd);
    WrapLogExit("CStartMenuSiteWrap::GetWindow()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CStartMenuSiteWrap::ContextSensitiveHelp(BOOL fEnterMode)
{
    WrapLogEnter("CStartMenuSiteWrap<%p>::ContextSensitiveHelp(BOOL fEnterMode=%d)\n", this, fEnterMode);
    HRESULT hr = m_IOleWindow->ContextSensitiveHelp(fEnterMode);
    WrapLogExit("CStartMenuSiteWrap::ContextSensitiveHelp()", hr);
    return hr;
}

// *** IOleCommandTarget methods ***
HRESULT STDMETHODCALLTYPE CStartMenuSiteWrap::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    WrapLogEnter("CStartMenuSiteWrap<%p>::QueryStatus(const GUID *pguidCmdGroup=%p, ULONG cCmds=%u, prgCmds=%p, pCmdText=%p)\n", this, pguidCmdGroup, cCmds, prgCmds, pCmdText);
    HRESULT hr = m_IOleCommandTarget->QueryStatus(pguidCmdGroup, cCmds, prgCmds, pCmdText);
    if (pguidCmdGroup) WrapLogPost("*pguidCmdGroup=%s\n", Wrap(*pguidCmdGroup));
    WrapLogExit("CStartMenuSiteWrap::QueryStatus()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CStartMenuSiteWrap::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    WrapLogEnter("CStartMenuSiteWrap<%p>::Exec(const GUID *pguidCmdGroup=%p, DWORD nCmdID=%d, DWORD nCmdexecopt=%d, VARIANT *pvaIn=%p, VARIANT *pvaOut=%p)\n", this, pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);
    if (pguidCmdGroup) WrapLogPre("*pguidCmdGroup=%s\n", Wrap(*pguidCmdGroup));
    HRESULT hr = m_IOleCommandTarget->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);
    WrapLogExit("CStartMenuSiteWrap::Exec()", hr);
    return hr;
}
