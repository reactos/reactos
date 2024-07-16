/*
 * PROJECT:     ReactOS Explorer
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Favorites bar
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <windows.h>
#include <shlobj.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <undocshell.h>
#include <shlguid_undoc.h>
#include "CFavBand.h"

CFavBand::CFavBand()
    : m_fVisible(FALSE)
    , m_bFocused(FALSE)
    , m_dwBandID(0)
{
    ::InterlockedIncrement(&SHDOCVW_refCount);
}

CFavBand::~CFavBand()
{
    ::InterlockedDecrement(&SHDOCVW_refCount);
}

HRESULT CFavBand::UpdateLocation()
{
    m_pidlCurrent = NULL;

    CComPtr<IShellBrowser> psb;
    HRESULT hr = IUnknown_QueryService(m_pSite, SID_STopLevelBrowser, IID_PPV_ARG(IShellBrowser, &psb));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComPtr<IBrowserService> pbs;
    hr = psb->QueryInterface(IID_PPV_ARG(IBrowserService, &pbs));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return pbs->GetPidl(&m_pidlCurrent);
}

// *** message handlers ***

LRESULT CFavBand::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    UpdateLocation();
    return 0;
}

LRESULT CFavBand::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return 0;
}

LRESULT CFavBand::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RECT rc;
    GetClientRect(&rc);
    return 0;
}

LRESULT CFavBand::OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    m_bFocused = TRUE;
    IUnknown_OnFocusChangeIS(m_pSite, reinterpret_cast<IUnknown*>(this), TRUE);
    return 0;
}

LRESULT CFavBand::OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    IUnknown_OnFocusChangeIS(m_pSite, reinterpret_cast<IUnknown*>(this), FALSE);
    m_bFocused = FALSE;
    return 0;
}

// *** IOleWindow ***

STDMETHODIMP CFavBand::GetWindow(HWND *lphwnd)
{
    if (!lphwnd)
        return E_INVALIDARG;
    *lphwnd = m_hWnd;
    return S_OK;
}

STDMETHODIMP CFavBand::ContextSensitiveHelp(BOOL fEnterMode)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// *** IDockingWindow ***

STDMETHODIMP CFavBand::CloseDW(DWORD dwReserved)
{
    // We do nothing, we don't have anything to save yet
    TRACE("CloseDW called\n");
    return S_OK;
}

STDMETHODIMP CFavBand::ResizeBorderDW(const RECT *prcBorder, IUnknown *punkToolbarSite, BOOL fReserved)
{
    /* Must return E_NOTIMPL according to MSDN */
    return E_NOTIMPL;
}

STDMETHODIMP CFavBand::ShowDW(BOOL fShow)
{
    m_fVisible = fShow;
    ShowWindow(fShow ? SW_SHOWNORMAL : SW_HIDE);
    return S_OK;
}

// *** IDeskBand ***

STDMETHODIMP CFavBand::GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO *pdbi)
{
    if (!pdbi)
        return E_INVALIDARG;

    m_dwBandID = dwBandID;

    if (pdbi->dwMask & DBIM_MINSIZE)
    {
        pdbi->ptMinSize.x = 200;
        pdbi->ptMinSize.y = 30;
    }

    if (pdbi->dwMask & DBIM_MAXSIZE)
        pdbi->ptMaxSize.y = -1;

    if (pdbi->dwMask & DBIM_INTEGRAL)
        pdbi->ptIntegral.y = 1;

    if (pdbi->dwMask & DBIM_ACTUAL)
    {
        pdbi->ptActual.x = 200;
        pdbi->ptActual.y = 30;
    }

    if (pdbi->dwMask & DBIM_MODEFLAGS)
        pdbi->dwModeFlags = DBIMF_NORMAL | DBIMF_VARIABLEHEIGHT;

    if (pdbi->dwMask & DBIM_BKCOLOR)
        pdbi->dwMask &= ~DBIM_BKCOLOR;

    return S_OK;
}

// *** IObjectWithSite ***

STDMETHODIMP CFavBand::SetSite(IUnknown *pUnkSite)
{
    HRESULT hr;

    if (pUnkSite == m_pSite)
        return S_OK;

    TRACE("SetSite called\n");
    if (!pUnkSite)
    {
        DestroyWindow();
        m_hWnd = NULL;
    }

    if (pUnkSite != m_pSite)
        m_pSite = NULL;

    if (!pUnkSite)
        return S_OK;

    HWND hwndParent;
    hr = IUnknown_GetWindow(pUnkSite, &hwndParent);
    if (!SUCCEEDED(hr))
    {
        ERR("Could not get parent's window! 0x%08lX\n", hr);
        return E_INVALIDARG;
    }

    m_pSite = pUnkSite;

    if (m_hWnd)
    {
        SetParent(hwndParent); // Change its parent
    }
    else
    {
        DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
        Create(hwndParent, 0, NULL, dwStyle);
    }

    return S_OK;
}

STDMETHODIMP CFavBand::GetSite(REFIID riid, void **ppvSite)
{
    if (!ppvSite)
        return E_POINTER;
    *ppvSite = m_pSite;
    return S_OK;
}

// *** IOleCommandTarget ***

STDMETHODIMP CFavBand::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CFavBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// *** IServiceProvider ***

STDMETHODIMP CFavBand::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// *** IServiceProvider ***

STDMETHODIMP CFavBand::QueryContextMenu(
    HMENU hmenu,
    UINT indexMenu,
    UINT idCmdFirst,
    UINT idCmdLast,
    UINT uFlags)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CFavBand::InvokeCommand(
    LPCMINVOKECOMMANDINFO lpici)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CFavBand::GetCommandString(
    UINT_PTR idCmd,
    UINT uType,
    UINT *pwReserved,
    LPSTR pszName,
    UINT cchMax)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// *** IInputObject ***

STDMETHODIMP CFavBand::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    if (fActivate)
    {
        //SetFocus();
        SetActiveWindow();
    }

    if (lpMsg)
    {
        TranslateMessage(lpMsg);
        DispatchMessage(lpMsg);
    }

    return S_OK;
}

STDMETHODIMP CFavBand::HasFocusIO()
{
    return m_bFocused ? S_OK : S_FALSE;
}

STDMETHODIMP CFavBand::TranslateAcceleratorIO(LPMSG lpMsg)
{
    if (lpMsg->hwnd == m_hWnd)
    {
        TranslateMessage(lpMsg);
        DispatchMessage(lpMsg);
        return S_OK;
    }

    return S_FALSE;
}

// *** IPersist ***

STDMETHODIMP CFavBand::GetClassID(CLSID *pClassID)
{
    if (!pClassID)
        return E_POINTER;
    memcpy(pClassID, &CLSID_SH_FavBand, sizeof(CLSID));
    return S_OK;
}


// *** IPersistStream ***

STDMETHODIMP CFavBand::IsDirty()
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CFavBand::Load(IStream *pStm)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CFavBand::Save(IStream *pStm, BOOL fClearDirty)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CFavBand::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    // TODO: calculate max size
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// *** IWinEventHandler ***

STDMETHODIMP CFavBand::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CFavBand::IsWindowOwner(HWND hWnd)
{
    return (hWnd == m_hWnd) ? S_OK : S_FALSE;
}

// *** IBandNavigate ***

STDMETHODIMP CFavBand::Select(long paramC)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// *** INamespaceProxy ***

STDMETHODIMP CFavBand::GetNavigateTarget(long paramC, long param10, long param14)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CFavBand::Invoke(long paramC)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CFavBand::OnSelectionChanged(long paramC)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CFavBand::RefreshFlags(long paramC, long param10, long param14)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CFavBand::CacheItem(long paramC)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// *** IDispatch ***

STDMETHODIMP CFavBand::GetTypeInfoCount(UINT *pctinfo)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CFavBand::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CFavBand::GetIDsOfNames(
    REFIID riid,
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODSTDMETHODIMP CFavBand::Invoke(
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr)
{
    switch (dispIdMember)
    {
        case DISPID_DOWNLOADCOMPLETE:
        case DISPID_NAVIGATECOMPLETE2:
            UpdateLocation();
            return S_OK;
    }
    return E_INVALIDARG;
}

EXTERN_C HRESULT
CFavBand_CreateInstance(DWORD_PTR dwUnused1, void **ppv, DWORD_PTR dwUnused3)
{
    UNREFERENCED_PARAMETER(dwUnused1);
    UNREFERENCED_PARAMETER(dwUnused3);

    if (!ppv)
        return E_POINTER;

    CFavBand *pFavBand = new CFavBand();
    *ppv = static_cast<IUnknown *>(pFavBand);
    TRACE("%p\n", *ppv);
    return S_OK;
}

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_ExplorerBand, CExplorerBand)
END_OBJECT_MAP()

class CExplorerBandModule : public CComModule
{
public:
};

static CExplorerBandModule gModule;

EXTERN_C VOID
CFavBand_Init(HINSTANCE hInstance)
{
    gModule.Init(ObjectMap, hInstance, NULL);
}

EXTERN_C HRESULT
CFavBand_DllCanUnloadNow(VOID)
{
    return gModule.DllCanUnloadNow();
}

EXTERN_C HRESULT
CFavBand_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    return gModule.DllGetClassObject(rclsid, riid, ppv);
}

EXTERN_C HRESULT
CFavBand_DllRegisterServer(VOID)
{
    return gModule.DllRegisterServer(FALSE);
}

EXTERN_C HRESULT
CFavBand_DllUnregisterServer(VOID)
{
    return gModule.DllUnregisterServer(FALSE);
}
