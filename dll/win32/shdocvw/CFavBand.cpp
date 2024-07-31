/*
 * PROJECT:     ReactOS Explorer
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Favorites bar
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <undocshell.h>
#include <shlobj_undoc.h>
#include <shlguid_undoc.h>
#include <shlwapi_undoc.h>
#include <shdeprecated.h>
#include <olectlid.h>
#include <exdispid.h>
#include <shellutils.h>
#include <ui/rosctrls.h>
#include "shdocvw.h"
#include "CFavBand.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

void *operator new(size_t size)
{
    return ::LocalAlloc(LPTR, size);
}

void operator delete(void *ptr)
{
    ::LocalFree(ptr);
}

void operator delete(void *ptr, size_t size)
{
    ::LocalFree(ptr);
}

#if 1
#undef UNIMPLEMENTED
#define UNIMPLEMENTED ERR("%s is UNIMPLEMENTED!\n", __FUNCTION__)
#endif

CFavBand::CFavBand()
    : m_fVisible(FALSE)
    , m_bFocused(FALSE)
    , m_dwBandID(0)
    , m_hToolbarImageList(NULL)
    , m_hTreeViewImageList(NULL)
{
    ::InterlockedIncrement(&SHDOCVW_refCount);
    SHGetSpecialFolderLocation(NULL, CSIDL_FAVORITES, &m_pidlFav);
}

CFavBand::~CFavBand()
{
    if (m_hToolbarImageList)
    {
        ImageList_Destroy(m_hToolbarImageList);
        m_hToolbarImageList = NULL;
    }
    if (m_hTreeViewImageList)
    {
        ImageList_Destroy(m_hTreeViewImageList);
        m_hTreeViewImageList = NULL;
    }
    ::InterlockedDecrement(&SHDOCVW_refCount);
}

VOID CFavBand::OnFinalMessage(HWND)
{
    // The message loop is finished, now we can safely destruct!
    Release();
}

// *** helper methods ***

BOOL CFavBand::CreateToolbar()
{
#define IDB_SHELL_EXPLORER_SM 216 // Borrowed from browseui.dll
    HINSTANCE hinstBrowseUI = LoadLibraryExW(L"browseui.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
    ATLASSERT(hinstBrowseUI);
    HBITMAP hbmToolbar = NULL;
    if (hinstBrowseUI)
    {
        hbmToolbar = LoadBitmapW(hinstBrowseUI, MAKEINTRESOURCEW(IDB_SHELL_EXPLORER_SM));
        FreeLibrary(hinstBrowseUI);
    }
#undef IDB_SHELL_EXPLORER_SM
    ATLASSERT(hbmToolbar);
    if (!hbmToolbar)
        return FALSE;

    m_hToolbarImageList = ImageList_Create(16, 16, ILC_COLOR32, 0, 8);
    ATLASSERT(m_hToolbarImageList);
    if (!m_hToolbarImageList)
        return FALSE;

    ImageList_Add(m_hToolbarImageList, hbmToolbar, NULL);
    DeleteObject(hbmToolbar);

    DWORD style = WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_LIST | CCS_NODIVIDER |
                  TBSTYLE_WRAPABLE;
    HWND hwndTB = ::CreateWindowExW(0, TOOLBARCLASSNAMEW, NULL, style, 0, 0, 0, 0, m_hWnd,
                                    (HMENU)(LONG_PTR)IDW_TOOLBAR, instance, NULL);
    ATLASSERT(hwndTB);
    if (!hwndTB)
        return FALSE;

    m_hwndToolbar.Attach(hwndTB);
    m_hwndToolbar.SendMessage(TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
    m_hwndToolbar.SendMessage(TB_SETIMAGELIST, 0, (LPARAM)m_hToolbarImageList);
    m_hwndToolbar.SendMessage(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);

    WCHAR szzAdd[MAX_PATH], szzOrganize[MAX_PATH];
    ZeroMemory(szzAdd, sizeof(szzAdd));
    ZeroMemory(szzOrganize, sizeof(szzOrganize));
    LoadStringW(instance, IDS_ADD, szzAdd, _countof(szzAdd));
    LoadStringW(instance, IDS_ORGANIZE, szzOrganize, _countof(szzOrganize));

    TBBUTTON tbb[2] = { { 0 } };
    INT iButton = 0;
    tbb[iButton].iBitmap = 3;
    tbb[iButton].idCommand = ID_ADD;
    tbb[iButton].fsState = TBSTATE_ENABLED;
    tbb[iButton].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT;
    tbb[iButton].iString = (INT)m_hwndToolbar.SendMessage(TB_ADDSTRING, 0, (LPARAM)szzAdd);
    ++iButton;
    tbb[iButton].iBitmap = 42;
    tbb[iButton].idCommand = ID_ORGANIZE;
    tbb[iButton].fsState = TBSTATE_ENABLED;
    tbb[iButton].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT;
    tbb[iButton].iString = (INT)m_hwndToolbar.SendMessage(TB_ADDSTRING, 0, (LPARAM)szzOrganize);
    ++iButton;
    ATLASSERT(iButton == _countof(tbb));

    LRESULT ret = m_hwndToolbar.SendMessage(TB_ADDBUTTONS, iButton, (LPARAM)&tbb);
    ATLASSERT(ret);

    return ret;
}

BOOL CFavBand::CreateTreeView()
{
    m_hTreeViewImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 64, 0);
    ATLASSERT(m_hTreeViewImageList);
    if (!m_hTreeViewImageList)
        return FALSE;

    DWORD style = TVS_NOHSCROLL | TVS_NONEVENHEIGHT | TVS_FULLROWSELECT | TVS_INFOTIP |
                  TVS_SINGLEEXPAND | TVS_TRACKSELECT | TVS_SHOWSELALWAYS | TVS_EDITLABELS |
                  WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_TABSTOP;
    HWND hwndTV = ::CreateWindowExW(WS_EX_CLIENTEDGE, WC_TREEVIEWW, NULL, style, 0, 0, 0, 0,
                                    m_hWnd, (HMENU)(ULONG_PTR)IDW_TREEVIEW, instance, NULL);
    ATLASSERT(hwndTV);
    if (!hwndTV)
        return FALSE;

    m_hwndTreeView.Attach(hwndTV);
    TreeView_SetImageList(m_hwndTreeView, m_hTreeViewImageList, TVSIL_NORMAL);

    return TRUE;
}

// *** message handlers ***

LRESULT CFavBand::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    INITCOMMONCONTROLSEX iccx = { sizeof(iccx), ICC_TREEVIEW_CLASSES | ICC_BAR_CLASSES };
    if (!::InitCommonControlsEx(&iccx) || !CreateToolbar() || !CreateTreeView())
        return -1;

    return 0;
}

LRESULT CFavBand::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_hwndTreeView.DestroyWindow();
    m_hwndToolbar.DestroyWindow();
    return 0;
}

LRESULT CFavBand::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (!m_hwndTreeView)
        return 0;

    RECT rc;
    GetClientRect(&rc);
    LONG cx = rc.right, cy = rc.bottom;

    RECT rcTB;
    m_hwndToolbar.SendMessage(TB_AUTOSIZE, 0, 0);
    m_hwndToolbar.GetWindowRect(&rcTB);

    LONG cyTB = rcTB.bottom - rcTB.top;
    m_hwndTreeView.MoveWindow(0, cyTB, cx, cy - cyTB);

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

LRESULT CFavBand::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    switch (LOWORD(wParam))
    {
        case ID_ADD:
        {
            UNIMPLEMENTED;
            SHELL_ErrorBox(m_hWnd, ERROR_NOT_SUPPORTED);
            break;
        }
        case ID_ORGANIZE:
        {
            SHELLEXECUTEINFOW sei = { sizeof(sei), SEE_MASK_IDLIST };
            sei.hwnd = m_hWnd;
            sei.nShow = SW_SHOWNORMAL;
            sei.lpIDList = m_pidlFav;
            ::ShellExecuteExW(&sei);
            break;
        }
    }
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
    ShowWindow(fShow ? SW_SHOW : SW_HIDE);
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

    if (pdbi->dwMask & DBIM_TITLE)
    {
#define IDS_FAVORITES 47 // Borrowed from shell32.dll
        HINSTANCE hShell32 = LoadLibraryExW(L"shell32.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (hShell32)
        {
            LoadStringW(hShell32, IDS_FAVORITES, pdbi->wszTitle, _countof(pdbi->wszTitle));
            FreeLibrary(hShell32);
        }
#undef IDS_FAVORITES
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
        this->Create(hwndParent, NULL, NULL, WS_CHILD | WS_VISIBLE, 0, (UINT)0, NULL);
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
    *pClassID = CLSID_SH_FavBand;
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

/// Returns the ITEMIDLIST that should be navigated when an item is invoked.
STDMETHODIMP CFavBand::GetNavigateTarget(
    _In_ PCIDLIST_ABSOLUTE pidl,
    _Out_ PIDLIST_ABSOLUTE ppidlTarget,
    _Out_ ULONG *pulAttrib)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

/// Handles a user action on an item.
STDMETHODIMP CFavBand::Invoke(_In_ PCIDLIST_ABSOLUTE pidl)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

/// Called when the user has selected an item.
STDMETHODIMP CFavBand::OnSelectionChanged(_In_ PCIDLIST_ABSOLUTE pidl)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

/// Returns flags used to update the tree control.
STDMETHODIMP CFavBand::RefreshFlags(
    _Out_ DWORD *pdwStyle,
    _Out_ DWORD *pdwExStyle,
    _Out_ DWORD *dwEnum)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CFavBand::CacheItem(
    _In_ PCIDLIST_ABSOLUTE pidl)
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

STDMETHODIMP CFavBand::Invoke(
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
            // FIXME: Update current location
            return S_OK;
    }
    return E_INVALIDARG;
}

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_SH_FavBand, CFavBand)
END_OBJECT_MAP()

class CFavBandModule : public CComModule
{
public:
};

static CFavBandModule gModule;

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
