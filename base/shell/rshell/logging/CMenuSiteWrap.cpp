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

class CMenuSiteWrap :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IBandSite,
    public IDeskBarClient,
    public IOleCommandTarget,
    public IInputObject,
    public IInputObjectSite,
    public IWinEventHandler,
    public IServiceProvider
{
public:
    CMenuSiteWrap() {}
    ~CMenuSiteWrap();

    HRESULT InitWrap(IBandSite * bandSite);

private:
    CComPtr<IBandSite        > m_IBandSite;
    CComPtr<IDeskBarClient   > m_IDeskBarClient;
    CComPtr<IOleWindow       > m_IOleWindow;
    CComPtr<IOleCommandTarget> m_IOleCommandTarget;
    CComPtr<IInputObject     > m_IInputObject;
    CComPtr<IInputObjectSite > m_IInputObjectSite;
    CComPtr<IWinEventHandler > m_IWinEventHandler;
    CComPtr<IServiceProvider > m_IServiceProvider;

public:
    // IBandSite
    virtual HRESULT STDMETHODCALLTYPE AddBand(IUnknown * punk);
    virtual HRESULT STDMETHODCALLTYPE EnumBands(UINT uBand, DWORD* pdwBandID);
    virtual HRESULT STDMETHODCALLTYPE QueryBand(DWORD dwBandID, IDeskBand **ppstb, DWORD *pdwState, LPWSTR pszName, int cchName);
    virtual HRESULT STDMETHODCALLTYPE GetBandObject(DWORD dwBandID, REFIID riid, VOID **ppv);
    virtual HRESULT STDMETHODCALLTYPE GetBandSiteInfo(BANDSITEINFO *pbsinfo);
    virtual HRESULT STDMETHODCALLTYPE RemoveBand(DWORD dwBandID);
    virtual HRESULT STDMETHODCALLTYPE SetBandSiteInfo(const BANDSITEINFO *pbsinfo);
    virtual HRESULT STDMETHODCALLTYPE SetBandState(DWORD dwBandID, DWORD dwMask, DWORD dwState);

    // IDeskBarClient
    virtual HRESULT STDMETHODCALLTYPE SetDeskBarSite(IUnknown *punkSite);
    virtual HRESULT STDMETHODCALLTYPE SetModeDBC(DWORD dwMode);
    virtual HRESULT STDMETHODCALLTYPE GetSize(DWORD dwWhich, LPRECT prc);
    virtual HRESULT STDMETHODCALLTYPE UIActivateDBC(DWORD dwState);

    // IOleWindow
    virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *phwnd);
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

    // IOleCommandTarget
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID * pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText);
    virtual HRESULT STDMETHODCALLTYPE Exec(const GUID * pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

    // IInputObject
    virtual HRESULT STDMETHODCALLTYPE UIActivateIO(BOOL fActivate, LPMSG lpMsg);
    virtual HRESULT STDMETHODCALLTYPE HasFocusIO();
    virtual HRESULT STDMETHODCALLTYPE TranslateAcceleratorIO(LPMSG lpMsg);

    // IInputObjectSite
    virtual HRESULT STDMETHODCALLTYPE OnFocusChangeIS(IUnknown *punkObj, BOOL fSetFocus);

    // IWinEventHandler
    virtual HRESULT STDMETHODCALLTYPE IsWindowOwner(HWND hWnd);
    virtual HRESULT STDMETHODCALLTYPE OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult);

    // IServiceProvider
    virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void **ppvObject);

    DECLARE_NOT_AGGREGATABLE(CMenuSiteWrap)
    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(CMenuSiteWrap)
        COM_INTERFACE_ENTRY_IID(IID_IBandSite, IBandSite)
        COM_INTERFACE_ENTRY_IID(IID_IDeskBarClient, IDeskBarClient)
        COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
        COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
        COM_INTERFACE_ENTRY_IID(IID_IInputObject, IInputObject)
        COM_INTERFACE_ENTRY_IID(IID_IInputObjectSite, IInputObjectSite)
        COM_INTERFACE_ENTRY_IID(IID_IWinEventHandler, IWinEventHandler)
        COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
    END_COM_MAP()
};

extern "C"
HRESULT WINAPI CMenuSite_Wrapper(IBandSite * bandSite, REFIID riid, LPVOID *ppv)
{
    HRESULT hr;

    *ppv = NULL;

    CMenuSiteWrap * site = new CComObject<CMenuSiteWrap>();

    if (!site)
        return E_OUTOFMEMORY;

    hr = site->InitWrap(bandSite);
    if (FAILED(hr))
    {
        site->Release();
        return hr;
    }

    hr = site->QueryInterface(riid, ppv);

    if (FAILED(hr))
        site->Release();

    return hr;
}

HRESULT CMenuSiteWrap::InitWrap(IBandSite * bandSite)
{
    HRESULT hr;

    WrapLogOpen();

    m_IBandSite = bandSite;

    hr = bandSite->QueryInterface(IID_PPV_ARG(IDeskBarClient, &m_IDeskBarClient));
    if (FAILED(hr)) return hr;
    hr = bandSite->QueryInterface(IID_PPV_ARG(IOleWindow, &m_IOleWindow));
    if (FAILED(hr)) return hr;
    hr = bandSite->QueryInterface(IID_PPV_ARG(IOleCommandTarget, &m_IOleCommandTarget));
    if (FAILED(hr)) return hr;
    hr = bandSite->QueryInterface(IID_PPV_ARG(IInputObject, &m_IInputObject));
    if (FAILED(hr)) return hr;
    hr = bandSite->QueryInterface(IID_PPV_ARG(IInputObjectSite, &m_IInputObjectSite));
    if (FAILED(hr)) return hr;
    hr = bandSite->QueryInterface(IID_PPV_ARG(IWinEventHandler, &m_IWinEventHandler));
    if (FAILED(hr)) return hr;
    hr = bandSite->QueryInterface(IID_PPV_ARG(IServiceProvider, &m_IServiceProvider));
    return hr;
}

CMenuSiteWrap::~CMenuSiteWrap()
{
    WrapLogClose();
}

// IBandSite
HRESULT STDMETHODCALLTYPE CMenuSiteWrap::AddBand(IUnknown * punk)
{
    WrapLogEnter("CMenuSiteWrap<%p>::AddBand(IUnknown * punk=%p)\n", this, punk);
    HRESULT hr = m_IBandSite->AddBand(punk);
    WrapLogExit("CMenuSiteWrap::AddBand()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuSiteWrap::EnumBands(UINT uBand, DWORD* pdwBandID)
{
    WrapLogEnter("CMenuSiteWrap<%p>::EnumBands(UINT uBand=%u, DWORD* pdwBandID=%p)\n", this, uBand, pdwBandID);
    HRESULT hr = m_IBandSite->EnumBands(uBand, pdwBandID);
    if (pdwBandID) WrapLogPost("*pdwBandID=%d\n", *pdwBandID);
    WrapLogExit("CMenuSiteWrap::EnumBands()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuSiteWrap::QueryBand(DWORD dwBandID, IDeskBand **ppstb, DWORD *pdwState, LPWSTR pszName, int cchName)
{
    WrapLogEnter("CMenuSiteWrap<%p>::QueryBand(DWORD dwBandID=%d, IDeskBand **ppstb=%p, DWORD *pdwState=%p, LPWSTR pszName=%p, int cchName=%d)\n", this, dwBandID, ppstb, pdwState, pszName, cchName);
    HRESULT hr = m_IBandSite->QueryBand(dwBandID, ppstb, pdwState, pszName, cchName);
    if (ppstb) WrapLogPost("*ppstb=%p\n", *ppstb);
    if (pdwState) WrapLogPost("*pdwState=%d\n", *pdwState);
    WrapLogExit("CMenuSiteWrap::QueryBand()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuSiteWrap::GetBandObject(DWORD dwBandID, REFIID riid, VOID **ppv)
{
    WrapLogEnter("CMenuSiteWrap<%p>::GetBandObject(DWORD dwBandID=%d, REFIID riid=%s, VOID **ppv=%p)\n", this, dwBandID, Wrap(riid), ppv);
    HRESULT hr = m_IBandSite->GetBandObject(dwBandID, riid, ppv);
    if (ppv) WrapLogPost("*ppv=%p\n", *ppv);
    WrapLogExit("CMenuSiteWrap::GetBandObject()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuSiteWrap::GetBandSiteInfo(BANDSITEINFO *pbsinfo)
{
    WrapLogEnter("CMenuSiteWrap<%p>::GetBandSiteInfo(BANDSITEINFO *pbsinfo=%p)\n", this, pbsinfo);
    HRESULT hr = m_IBandSite->GetBandSiteInfo(pbsinfo);
    if (pbsinfo) WrapLogPost("*pbsinfo=%s\n", Wrap(*pbsinfo));
    WrapLogExit("CMenuSiteWrap::GetBandSiteInfo()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuSiteWrap::RemoveBand(DWORD dwBandID)
{
    WrapLogEnter("CMenuSiteWrap<%p>::RemoveBand(DWORD dwBandID=%d)\n", this, dwBandID);
    HRESULT hr = m_IBandSite->RemoveBand(dwBandID);
    WrapLogExit("CMenuSiteWrap::RemoveBand()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuSiteWrap::SetBandSiteInfo(const BANDSITEINFO *pbsinfo)
{
    WrapLogEnter("CMenuSiteWrap<%p>::SetBandSiteInfo(const BANDSITEINFO *pbsinfo=%p)\n", this, pbsinfo);
    //if (phwnd) WrapLogPost("*pbsinfo=%s\n", Wrap(*pbsinfo));
    HRESULT hr = m_IBandSite->SetBandSiteInfo(pbsinfo);
    WrapLogExit("CMenuSiteWrap::SetBandSiteInfo()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuSiteWrap::SetBandState(DWORD dwBandID, DWORD dwMask, DWORD dwState)
{
    WrapLogEnter("CMenuSiteWrap<%p>::SetBandState(DWORD dwBandID=%d, DWORD dwMask=%08x, DWORD dwState=%d)\n", this, dwBandID, dwMask, dwState);
    HRESULT hr = m_IBandSite->SetBandState(dwBandID, dwMask, dwState);
    WrapLogExit("CMenuSiteWrap::SetBandState()", hr);
    return hr;
}

// *** IDeskBarClient ***
HRESULT STDMETHODCALLTYPE CMenuSiteWrap::SetDeskBarSite(IUnknown *punkSite)
{
    WrapLogEnter("CMenuSiteWrap<%p>::SetDeskBarSite(IUnknown *punkSite=%p)\n", this, punkSite);
    HRESULT hr = m_IDeskBarClient->SetDeskBarSite(punkSite);
    WrapLogExit("CMenuSiteWrap::SetDeskBarSite()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuSiteWrap::SetModeDBC(DWORD dwMode)
{
    WrapLogEnter("CMenuSiteWrap<%p>::SetModeDBC(DWORD dwMode=%d)\n", this, dwMode);
    HRESULT hr = m_IDeskBarClient->SetModeDBC(dwMode);
    WrapLogExit("CMenuSiteWrap::SetModeDBC()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuSiteWrap::GetSize(DWORD dwWhich, LPRECT prc)
{
    WrapLogEnter("CMenuSiteWrap<%p>::GetSize(DWORD dwWhich=%d, LPRECT prc=%p)\n", this, dwWhich, prc);
    HRESULT hr = m_IDeskBarClient->GetSize(dwWhich, prc);
    if (prc) WrapLogPost("*prc=%s\n", Wrap(*prc));
    WrapLogExit("CMenuSiteWrap::GetSize()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuSiteWrap::UIActivateDBC(DWORD dwState)
{
    WrapLogEnter("CMenuSiteWrap<%p>::UIActivateDBC(DWORD dwState=%d)\n", this, dwState);
    HRESULT hr = m_IDeskBarClient->UIActivateDBC(dwState);
    WrapLogExit("CMenuSiteWrap::UIActivateDBC()", hr);
    return hr;
}

// *** IOleWindow methods ***
HRESULT STDMETHODCALLTYPE CMenuSiteWrap::GetWindow(HWND *phwnd)
{
    WrapLogEnter("CMenuSiteWrap<%p>::GetWindow(HWND *phwnd=%p)\n", this, phwnd);
    HRESULT hr = m_IOleWindow->GetWindow(phwnd);
    if (phwnd) WrapLogPost("*phwnd=%p\n", *phwnd);
    WrapLogExit("CMenuSiteWrap::GetWindow()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuSiteWrap::ContextSensitiveHelp(BOOL fEnterMode)
{
    WrapLogEnter("CMenuSiteWrap<%p>::ContextSensitiveHelp(BOOL fEnterMode=%d)\n", this, fEnterMode);
    HRESULT hr = m_IOleWindow->ContextSensitiveHelp(fEnterMode);
    WrapLogExit("CMenuSiteWrap::ContextSensitiveHelp()", hr);
    return hr;
}

// *** IOleCommandTarget methods ***
HRESULT STDMETHODCALLTYPE CMenuSiteWrap::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    WrapLogEnter("CMenuSiteWrap<%p>::QueryStatus(const GUID *pguidCmdGroup=%p, ULONG cCmds=%u, prgCmds=%p, pCmdText=%p)\n", this, pguidCmdGroup, cCmds, prgCmds, pCmdText);
    HRESULT hr = m_IOleCommandTarget->QueryStatus(pguidCmdGroup, cCmds, prgCmds, pCmdText);
    if (pguidCmdGroup) WrapLogPost("*pguidCmdGroup=%s\n", Wrap(*pguidCmdGroup));
    WrapLogExit("CMenuSiteWrap::QueryStatus()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuSiteWrap::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    WrapLogEnter("CMenuSiteWrap<%p>::Exec(const GUID *pguidCmdGroup=%p, DWORD nCmdID=%d, DWORD nCmdexecopt=%d, VARIANT *pvaIn=%p, VARIANT *pvaOut=%p)\n", this, pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);
    if (pguidCmdGroup) WrapLogPre("*pguidCmdGroup=%s\n", Wrap(*pguidCmdGroup));
    HRESULT hr = m_IOleCommandTarget->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);
    WrapLogExit("CMenuSiteWrap::Exec()", hr);
    return hr;
}

// *** IInputObject methods ***
HRESULT STDMETHODCALLTYPE CMenuSiteWrap::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    WrapLogEnter("CMenuSiteWrap<%p>::UIActivateIO(BOOL fActivate=%d, LPMSG lpMsg=%p)\n", this, fActivate, lpMsg);
    HRESULT hr = m_IInputObject->UIActivateIO(fActivate, lpMsg);
    WrapLogExit("CMenuSiteWrap::UIActivateIO()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuSiteWrap::HasFocusIO()
{
    WrapLogEnter("CMenuSiteWrap<%p>::HasFocusIO()\n", this);
    HRESULT hr = m_IInputObject->HasFocusIO();
    WrapLogExit("CMenuSiteWrap::HasFocusIO()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuSiteWrap::TranslateAcceleratorIO(LPMSG lpMsg)
{
    WrapLogEnter("CMenuSiteWrap<%p>::TranslateAcceleratorIO(LPMSG lpMsg=%p)\n", this, lpMsg);
    if (lpMsg) WrapLogPre("*lpMsg=%s\n", Wrap(*lpMsg));
    HRESULT hr = m_IInputObject->TranslateAcceleratorIO(lpMsg);
    WrapLogExit("CMenuSiteWrap::TranslateAcceleratorIO()", hr);
    return hr;
}

// *** IInputObjectSite methods ***
HRESULT STDMETHODCALLTYPE CMenuSiteWrap::OnFocusChangeIS(LPUNKNOWN lpUnknown, BOOL bFocus)
{
    WrapLogEnter("CMenuSiteWrap<%p>::OnFocusChangeIS(LPUNKNOWN lpUnknown=%p, BOOL bFocus=%d)\n", this, lpUnknown, bFocus);
    HRESULT hr = m_IInputObjectSite->OnFocusChangeIS(lpUnknown, bFocus);
    WrapLogExit("CMenuSiteWrap::OnFocusChangeIS()", hr);
    return hr;
}

// *** IWinEventHandler methods ***
HRESULT STDMETHODCALLTYPE CMenuSiteWrap::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    //WrapLogEnter("CMenuSiteWrap<%p>::OnWinEvent(HWND hWnd=%p, UINT uMsg=%u, WPARAM wParam=%08x, LPARAM lParam=%08x, LRESULT *theResult=%p)\n", this, hWnd, uMsg, wParam, lParam, theResult);
    HRESULT hr = m_IWinEventHandler->OnWinEvent(hWnd, uMsg, wParam, lParam, theResult);
    //WrapLogExit("CMenuSiteWrap::OnWinEvent()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuSiteWrap::IsWindowOwner(HWND hWnd)
{
    //WrapLogEnter("CMenuSiteWrap<%p>::IsWindowOwner(HWND hWnd=%08x)\n", this, hWnd);
    HRESULT hr = m_IWinEventHandler->IsWindowOwner(hWnd);
    //WrapLogExit("CMenuSiteWrap::IsWindowOwner()", hr);
    return hr;
}

// *** IServiceProvider methods ***
HRESULT STDMETHODCALLTYPE CMenuSiteWrap::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    WrapLogEnter("CMenuSiteWrap<%p>::QueryService(REFGUID guidService=%s, REFIID riid=%s, void **ppvObject=%p)\n", this, Wrap(guidService), Wrap(riid), ppvObject);
    HRESULT hr = m_IServiceProvider->QueryService(guidService, riid, ppvObject);
    if (ppvObject) WrapLogPost("*ppvObject=%p\n", *ppvObject);
    WrapLogExit("CMenuSiteWrap::QueryService()", hr);
    return hr;
}
