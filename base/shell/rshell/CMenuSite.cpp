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
#include <atlwin.h>
#include <shlwapi_undoc.h>

WINE_DEFAULT_DEBUG_CHANNEL(menusite);

bool _assert(bool cond, LPCSTR expr, LPCSTR file, DWORD line, LPCSTR func)
{
#if DBG
    if (!cond)
    {
        wine_dbg_printf("%s(%d): Assertion failed '%s', at %s", file, line, expr, func);
        DebugBreak();
    }
#endif
    return cond;
}
#define DBGASSERT(x) _assert(!!(x), #x, __FILE__, __LINE__, __FUNCSIG__)

class CMenuSite :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public CWindowImpl<CMenuSite, CWindow, CControlWinTraits>,
    public IBandSite,
    public IDeskBarClient,
    public IOleCommandTarget,
    public IInputObject,
    public IInputObjectSite,
    public IWinEventHandler,
    public IServiceProvider
{
    IUnknown *          m_DeskBarSite;
    IUnknown *          m_BandObject;
    IDeskBand *         m_DeskBand;
    IWinEventHandler *  m_WinEventHandler;
    HWND                m_hWndBand;

public:
    CMenuSite();
    ~CMenuSite() {}

    DECLARE_WND_CLASS_EX(_T("MenuSite"), 0, COLOR_WINDOW)

    DECLARE_NOT_AGGREGATABLE(CMenuSite)
    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(CMenuSite)
        COM_INTERFACE_ENTRY_IID(IID_IBandSite, IBandSite)
        COM_INTERFACE_ENTRY_IID(IID_IDeskBarClient, IDeskBarClient)
        COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
        COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
        COM_INTERFACE_ENTRY_IID(IID_IInputObject, IInputObject)
        COM_INTERFACE_ENTRY_IID(IID_IInputObjectSite, IInputObjectSite)
        COM_INTERFACE_ENTRY_IID(IID_IWinEventHandler, IWinEventHandler)
        COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
    END_COM_MAP()

    // IBandSite
    virtual HRESULT STDMETHODCALLTYPE AddBand(IUnknown * punk);
    virtual HRESULT STDMETHODCALLTYPE EnumBands(UINT uBand, DWORD* pdwBandID);
    virtual HRESULT STDMETHODCALLTYPE QueryBand(DWORD dwBandID, IDeskBand **ppstb, DWORD *pdwState, LPWSTR pszName, int cchName);
    virtual HRESULT STDMETHODCALLTYPE GetBandObject(DWORD dwBandID, REFIID riid, VOID **ppv);

    // IDeskBarClient
    virtual HRESULT STDMETHODCALLTYPE SetDeskBarSite(IUnknown *punkSite);
    virtual HRESULT STDMETHODCALLTYPE GetSize(DWORD dwWhich, LPRECT prc);
    virtual HRESULT STDMETHODCALLTYPE UIActivateDBC(DWORD dwState);

    // IOleWindow
    virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *phwnd);

    // IOleCommandTarget
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID * pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText);
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


    // Using custom message map instead 
    virtual BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &lResult, DWORD mapId = 0);

    // UNIMPLEMENTED
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);
    virtual HRESULT STDMETHODCALLTYPE GetBandSiteInfo(BANDSITEINFO *pbsinfo);
    virtual HRESULT STDMETHODCALLTYPE RemoveBand(DWORD dwBandID);
    virtual HRESULT STDMETHODCALLTYPE SetBandSiteInfo(const BANDSITEINFO *pbsinfo);
    virtual HRESULT STDMETHODCALLTYPE SetBandState(DWORD dwBandID, DWORD dwMask, DWORD dwState);
    virtual HRESULT STDMETHODCALLTYPE SetModeDBC(DWORD dwMode);

private:
    BOOL CreateSiteWindow(HWND hWndParent);
};

extern "C"
HRESULT CMenuSite_Constructor(REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;

    CMenuSite * site = new CComObject<CMenuSite>();

    if (!site)
        return E_OUTOFMEMORY;

    HRESULT hr = site->QueryInterface(riid, ppv);

    if (FAILED(hr))
        site->Release();

    return hr;
}

CMenuSite::CMenuSite() :
    m_DeskBarSite(NULL),
    m_BandObject(NULL),
    m_DeskBand(NULL),
    m_WinEventHandler(NULL),
    m_hWndBand(NULL)
{
}

HRESULT STDMETHODCALLTYPE CMenuSite::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuSite::GetBandSiteInfo(BANDSITEINFO *pbsinfo)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuSite::RemoveBand(DWORD dwBandID)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuSite::SetBandSiteInfo(const BANDSITEINFO *pbsinfo)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuSite::SetBandState(DWORD dwBandID, DWORD dwMask, DWORD dwState)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuSite::SetModeDBC(DWORD dwMode)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuSite::TranslateAcceleratorIO(LPMSG lpMsg)
{
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CMenuSite::HasFocusIO()
{
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CMenuSite::OnFocusChangeIS(IUnknown *punkObj, BOOL fSetFocus)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuSite::AddBand(IUnknown * punk)
{
    if (SHIsSameObject(punk, m_BandObject))
        return S_OK + 0;

    IUnknown_SetSite(m_BandObject, NULL);

    if (m_BandObject)
    {
        m_BandObject->Release();
        m_BandObject = NULL;
    }

    if (m_DeskBand)
    {
        m_DeskBand->Release();
        m_DeskBand = NULL;
    }

    if (m_WinEventHandler)
    {
        m_WinEventHandler->Release();
        m_WinEventHandler = NULL;
    }

    BOOL result = m_hWndBand != NULL;

    m_hWndBand = NULL;

    if (!punk)
        return result ? S_OK + 0 : E_FAIL;

    DBGASSERT(SUCCEEDED(punk->QueryInterface(IID_PPV_ARG(IDeskBand, &m_DeskBand))));
    DBGASSERT(SUCCEEDED(punk->QueryInterface(IID_PPV_ARG(IWinEventHandler, &m_WinEventHandler))));

    IUnknown_SetSite(punk, (IDeskBarClient*)this);
    IUnknown_GetWindow(punk, &m_hWndBand);

    m_BandObject = punk;

    punk->AddRef();

    return S_OK + 0;
}

HRESULT STDMETHODCALLTYPE CMenuSite::EnumBands(UINT uBand, DWORD* pdwBandID)
{
    if (uBand != 0)
        return E_FAIL;

    *pdwBandID = 0;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuSite::Exec(const GUID * pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    return IUnknown_Exec(m_DeskBarSite, *pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);
}

HRESULT STDMETHODCALLTYPE CMenuSite::GetBandObject(DWORD dwBandID, REFIID riid, VOID **ppv)
{
    if (!DBGASSERT(dwBandID == 0) || m_BandObject == NULL)
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    return m_BandObject->QueryInterface(riid, ppv);
}

HRESULT STDMETHODCALLTYPE CMenuSite::GetSize(DWORD dwWhich, LPRECT prc)
{
    memset(prc, 0, sizeof(*prc));

    if (dwWhich != 0)
        return S_OK;

    if (m_DeskBand == NULL)
        return S_OK;

    DESKBANDINFO info = { 0 };
    info.dwMask = DBIM_MAXSIZE;

    m_DeskBand->GetBandInfo(0, 0, &info);

    prc->right = info.ptMaxSize.x;
    prc->bottom = info.ptMaxSize.y;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuSite::GetWindow(HWND *phwnd)
{
    DBGASSERT(IsWindow());

    *phwnd = m_hWnd;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuSite::IsWindowOwner(HWND hWnd)
{
    if (hWnd == m_hWnd)
        return S_OK;

    if (!m_WinEventHandler)
        return S_FALSE;

    return m_WinEventHandler->IsWindowOwner(hWnd);
}

HRESULT STDMETHODCALLTYPE CMenuSite::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    if (!m_WinEventHandler)
        return S_OK;

    return m_WinEventHandler->OnWinEvent(hWnd, uMsg, wParam, lParam, theResult);
}

HRESULT STDMETHODCALLTYPE CMenuSite::QueryBand(DWORD dwBandID, IDeskBand **ppstb, DWORD *pdwState, LPWSTR pszName, int cchName)
{
    DBGASSERT(dwBandID == 0);
    DBGASSERT(!IsBadWritePtr(ppstb, sizeof(*ppstb)));

    if (!m_BandObject)
    {
        *ppstb = NULL;
        return E_NOINTERFACE;
    }

    HRESULT hr = m_BandObject->QueryInterface(IID_PPV_ARG(IDeskBand, ppstb));

    *pdwState = 1;

    if (cchName > 0)
        pszName[0] = 0;

    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuSite::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    *ppvObject = NULL;

    if (IsEqualGUID(guidService, SID_SMenuBandBottom) ||
        IsEqualGUID(guidService, SID_SMenuBandBottomSelected) ||
        IsEqualGUID(guidService, SID_SMenuBandChild))
    {
        if (m_BandObject == NULL)
            return E_FAIL;

        return IUnknown_QueryService(m_BandObject, guidService, riid, ppvObject);
    }

    DBGASSERT(m_DeskBarSite);

    return IUnknown_QueryService(m_DeskBarSite, guidService, riid, ppvObject);
}

HRESULT STDMETHODCALLTYPE CMenuSite::QueryStatus(const GUID * pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText)
{
    if (!DBGASSERT(m_DeskBarSite))
        return E_FAIL;
    return IUnknown_QueryStatus(m_DeskBarSite, *pguidCmdGroup, cCmds, prgCmds, pCmdText);
}

HRESULT STDMETHODCALLTYPE CMenuSite::SetDeskBarSite(IUnknown *punkSite)
{
    HWND hWndSite;

    ((IDeskBarClient*)this)->AddRef();

    if (punkSite)
    {
        if (m_DeskBarSite)
        {
            m_DeskBarSite->Release();
            m_DeskBarSite = NULL;
        }

        IUnknown_GetWindow(punkSite, &hWndSite);

        if (hWndSite)
        {
            CreateSiteWindow(hWndSite);

            m_DeskBarSite = punkSite;

            punkSite->AddRef();
        }
    }
    else
    {
        if (m_DeskBand)
        {
            m_DeskBand->CloseDW(0);
        }

        IUnknown_SetSite(m_BandObject, NULL);

        if (m_BandObject)
        {
            m_BandObject->Release();
            m_BandObject = NULL;
        }

        if (m_DeskBand)
        {
            m_DeskBand->Release();
            m_DeskBand = NULL;
        }

        if (m_WinEventHandler)
        {
            m_WinEventHandler->Release();
            m_WinEventHandler = NULL;
        }

        m_hWndBand = NULL;

        if (m_hWnd)
        {
            DestroyWindow();
            m_hWnd = NULL;
        }

        if (m_DeskBarSite)
            m_DeskBarSite->Release();

        m_DeskBarSite = NULL;
    }

    ((IDeskBarClient*)this)->Release();

    if (!m_hWnd)
        return E_FAIL;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuSite::UIActivateDBC(DWORD dwState)
{
    if (!DBGASSERT(m_DeskBand))
        return S_OK;

    return m_DeskBand->ShowDW(dwState != 0);
}

HRESULT STDMETHODCALLTYPE CMenuSite::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    if (lpMsg && DBGASSERT(IsBadWritePtr(lpMsg, sizeof(*lpMsg))))
        return E_FAIL;

    return IUnknown_UIActivateIO(m_BandObject, fActivate, lpMsg);
}

BOOL CMenuSite::CreateSiteWindow(HWND hWndParent)
{
    if (m_hWnd)
    {
        return DBGASSERT(IsWindow());
    }

    Create(hWndParent, NULL, L"MenuSite");

    return m_hWnd != NULL;
}

BOOL CMenuSite::ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &lResult, DWORD mapId)
{
    HWND hWndToCall;
    IMenuPopup * pMenuPopup;

    ((IDeskBarClient*)this)->AddRef();

    switch (uMsg)
    {
    case WM_SIZE:
        if (m_BandObject)
        {
            if (SUCCEEDED(m_BandObject->QueryInterface(IID_PPV_ARG(IMenuPopup, &pMenuPopup))))
            {
                RECT Rect = { 0 };
                GetClientRect(&Rect);
                pMenuPopup->OnPosRectChangeDB(&Rect);
                pMenuPopup->Release();
            }
        }
        hWndToCall = hWnd;
        lResult = 1;
        break;
    case WM_NOTIFY:
        hWndToCall = *(HWND *) lParam;
        break;
    case WM_COMMAND:
        hWndToCall = (HWND) lParam;
        break;
    default:
        ((IDeskBarClient*)this)->Release();
        return FALSE;
    }

    if (hWndToCall)
    {
        if (m_WinEventHandler)
        {
            if (m_WinEventHandler->IsWindowOwner(hWndToCall) == S_OK)
            {
                HRESULT hr = m_WinEventHandler->OnWinEvent(hWndToCall, uMsg, wParam, lParam, &lResult);
                ((IDeskBarClient*)this)->Release();
                return hr == S_OK;
            }
        }
    }

    ((IDeskBarClient*)this)->Release();
    return FALSE;
}
