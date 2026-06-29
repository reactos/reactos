/*
 * PROJECT:     ReactOS shell extensions
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Quick Launch Toolbar (Taskbar Shell Extension)
 * COPYRIGHT:   Copyright 2017 Shriraj Sawant a.k.a SR13 <sr.official@hotmail.com>
 *              Copyright 2017-2018 Giannis Adamopoulos
 *              Copyright 2023-2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "shellbars.h"

#include <commoncontrols.h>
#include <shellapi.h>
#include <wingdi.h>
#include <uxtheme.h>

#undef SubclassWindow // Don't use SubclassWindow macro

/*
TODO:
    ** drag and drop support
    ** tooltips
    ** Fix position of the items context menu
    ** Implement responding to theme change
*/

#define TIMERID_DELAYED_REFRESH 0xBEEFCAFE
#define TIMER_REFRESH_DELAY 500

//*****************************************************************************************
// *** CISFBand ***

CISFBand::CISFBand()
    : m_BandID(0)
    , m_uChangeNotify(0)
    , m_bShowText(TRUE)
    , m_bSmallIcon(TRUE)
    , m_QLaunch(FALSE)
{
}

CISFBand::~CISFBand()
{
}

HRESULT CISFBand::CreateSimpleToolbar(HWND hWndParent)
{
    // Create the toolbar.
    DWORD style = WS_CHILD | TBSTYLE_FLAT | TBSTYLE_LIST | CCS_NORESIZE | CCS_NODIVIDER;
    HWND hwndToolbar = ::CreateWindowExW(0, TOOLBARCLASSNAMEW, NULL, style, 0, 0, 0, 0,
                                         hWndParent, NULL, 0, NULL);
    if (!hwndToolbar)
        return E_FAIL;

    SubclassWindow(hwndToolbar);
    ATLASSERT(m_hWnd);

    ShowHideText(m_bShowText);
    SetImageListIconSize(m_bSmallIcon);

    return AddToolbarButtons();
}

HRESULT CISFBand::RefreshToolbar()
{
    DeleteToolbarButtons();
    return AddToolbarButtons();
}

HRESULT CISFBand::AddToolbarButtons()
{
    CComPtr<IEnumIDList> pEnum;
    HRESULT hr = m_pISF->EnumObjects(0, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &pEnum);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    LPITEMIDLIST pidl;
    for (INT iItem = 0; pEnum->Next(1, &pidl, NULL) == S_OK; ++iItem)
    {
        STRRET strret;
        hr = m_pISF->GetDisplayNameOf(pidl, SHGDN_NORMAL, &strret);

        WCHAR szText[MAX_PATH];
        if (FAILED_UNEXPECTEDLY(hr))
            szText[0] = UNICODE_NULL;
        else
            StrRetToBufW(&strret, pidl, szText, _countof(szText));

        INT iImage = SHMapPIDLToSystemImageListIndex(m_pISF, pidl, NULL);
        TBBUTTON tb = { iImage, iItem, TBSTATE_ENABLED, BTNS_AUTOSIZE, { 0 },
                        (DWORD_PTR)pidl, (INT_PTR)szText };
        SendMessage(TB_INSERTBUTTONW, iItem, (LPARAM)&tb);
    }

    SendMessage(TB_AUTOSIZE, 0, 0);
    return BandInfoChanged();
}

void CISFBand::DeleteToolbarButtons()
{
    // Assumption: Deleting a button causes the remaining buttons to shift,
    // so the next button always appears at index 0. This ensures the loop
    // progresses and avoids infinite loops.
    TBBUTTON tb;
    while (SendMessage(TB_GETBUTTON, 0, (LPARAM)&tb))
    {
        CoTaskMemFree((LPITEMIDLIST)tb.dwData);
        SendMessage(TB_DELETEBUTTON, 0, 0);
    }
}

BOOL CISFBand::RegisterChangeNotify(_In_ BOOL bRegister)
{
    if (bRegister)
    {
        if (!m_pidl)
            return FALSE;
#define SHCNE_WATCH (SHCNE_RENAMEITEM | SHCNE_CREATE | SHCNE_DELETE | SHCNE_MKDIR | \
                     SHCNE_RMDIR | SHCNE_UPDATEDIR | SHCNE_UPDATEITEM | SHCNE_UPDATEIMAGE | \
                     SHCNE_RENAMEFOLDER | SHCNE_ASSOCCHANGED)
        SHChangeNotifyEntry entry = { m_pidl, FALSE };
        m_uChangeNotify = SHChangeNotifyRegister(m_hWnd, SHCNRF_ShellLevel | SHCNRF_NewDelivery,
                                                 SHCNE_WATCH, WM_ISFBAND_CHANGE_NOTIFY,
                                                 1, &entry);
        return m_uChangeNotify != 0;
    }
    else // De-register?
    {
        if (m_uChangeNotify)
        {
            SHChangeNotifyDeregister(m_uChangeNotify);
            m_uChangeNotify = 0;
            return TRUE;
        }
        return FALSE;
    }
}

HRESULT CISFBand::SetImageListIconSize(_In_ BOOL bSmall)
{
    m_bSmallIcon = bSmall;

    CComPtr<IImageList> piml;
    HRESULT hr = SHGetImageList((bSmall ? SHIL_SMALL : SHIL_LARGE), IID_PPV_ARG(IImageList, &piml));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    SendMessage(TB_SETIMAGELIST, 0, (LPARAM)(HIMAGELIST)piml.Detach());
    return S_OK;
}

HRESULT CISFBand::BandInfoChanged()
{
    if (!m_Site)
        return S_OK;
    HRESULT hr = IUnknown_Exec(m_Site, IID_IDeskBand, DBID_BANDINFOCHANGED, 0, NULL, NULL);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;
    return S_OK;
}

HRESULT CISFBand::ShowHideText(_In_ BOOL bShow)
{
    // NOTE: TBSTYLE_EX_MIXEDBUTTONS hides non-BTNS_SHOWTEXT buttons' text.
    m_bShowText = bShow;
    SendMessage(TB_SETEXTENDEDSTYLE, 0, (bShow ? 0 : TBSTYLE_EX_MIXEDBUTTONS));
    return S_OK;
}

//****************************************************************************
// Message handlers

// WM_TIMER
LRESULT CISFBand::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    if (wParam == TIMERID_DELAYED_REFRESH)
    {
        KillTimer(wParam);
        RefreshToolbar();
    }
    return 0;
}

// WM_ISFBAND_CHANGE_NOTIFY
LRESULT CISFBand::OnChangeNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    // This code reduces the redrawing cost by coalescing multiple change events
    KillTimer(TIMERID_DELAYED_REFRESH);
    SetTimer(TIMERID_DELAYED_REFRESH, TIMER_REFRESH_DELAY);
    return 0;
}

// WM_DESTROY
LRESULT CISFBand::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    RegisterChangeNotify(FALSE);
    KillTimer(TIMERID_DELAYED_REFRESH);
    ShowWindow(SW_HIDE);
    DeleteToolbarButtons();
    UnsubclassWindow();
    bHandled = FALSE;
    return 0;
}

/*****************************************************************************/

// *** IObjectWithSite ***
STDMETHODIMP CISFBand::SetSite(IUnknown *pUnkSite)
{
    HRESULT hr;
    HWND hwndParent;

    TRACE("CISFBand::SetSite(0x%p)\n", pUnkSite);

    hr = IUnknown_GetWindow(pUnkSite, &hwndParent);
    if (FAILED(hr))
    {
        TRACE("Querying site window failed: 0x%x\n", hr);
        return hr;
    }
    m_Site = pUnkSite;

    hr = CreateSimpleToolbar(hwndParent);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    RegisterChangeNotify(TRUE);
    return S_OK;
}

STDMETHODIMP CISFBand::GetSite(IN REFIID riid, OUT VOID **ppvSite)
{
    TRACE("CISFBand::GetSite(0x%p,0x%p)\n", riid, ppvSite);

    HRESULT hr;
    if (m_Site != NULL)
    {
        hr = m_Site->QueryInterface(riid, ppvSite);
        if (FAILED(hr)) return hr;
    }

    *ppvSite = NULL;
    return E_FAIL;
}

/*****************************************************************************/
// *** IDeskBand ***
STDMETHODIMP CISFBand::GetWindow(OUT HWND *phwnd)
{
    if (!m_hWnd)
        return E_FAIL;
    if (!phwnd)
        return E_POINTER;
    *phwnd = m_hWnd;

    return S_OK;
}

STDMETHODIMP CISFBand::ContextSensitiveHelp(IN BOOL fEnterMode)
{
    /* FIXME: Implement */
    return E_NOTIMPL;
}

STDMETHODIMP CISFBand::ShowDW(IN BOOL bShow)
{
    if (m_hWnd)
    {
        ShowWindow(bShow ? SW_SHOW : SW_HIDE);
        return S_OK;
    }

    return E_FAIL;
}

STDMETHODIMP CISFBand::CloseDW(IN DWORD dwReserved)
{
    if (m_hWnd)
    {
        DestroyWindow();
        return S_OK;
    }

    return E_FAIL;
}

STDMETHODIMP CISFBand::ResizeBorderDW(LPCRECT prcBorder, IUnknown *punkToolbarSite, BOOL fReserved)
{
    /* No need to implement this method */
    return E_NOTIMPL;
}

STDMETHODIMP CISFBand::GetBandInfo(IN DWORD dwBandID, IN DWORD dwViewMode, IN OUT DESKBANDINFO *pdbi)
{
    TRACE("CTaskBand::GetBandInfo(0x%x,0x%x,0x%p) hWnd=0x%p\n", dwBandID, dwViewMode, pdbi, m_hWnd);

    if (m_hWnd && pdbi)
    {
        m_BandID = dwBandID;

        RECT actualRect;
        POINTL actualSize;
        POINTL idealSize;
        POINTL maxSize;
        POINTL itemSize;

        GetWindowRect(&actualRect);
        actualSize.x = actualRect.right - actualRect.left;
        actualSize.y = actualRect.bottom - actualRect.top;

        // Obtain the ideal size, to be used as min and max
        SendMessageW(m_hWnd, TB_AUTOSIZE, 0, 0);
        SendMessageW(m_hWnd, TB_GETMAXSIZE, 0, reinterpret_cast<LPARAM>(&maxSize));

        idealSize = maxSize;
        SendMessageW(m_hWnd, TB_GETIDEALSIZE, FALSE, reinterpret_cast<LPARAM>(&idealSize));

        // Obtain the button size, to be used as the integral size
        DWORD size = SendMessageW(m_hWnd, TB_GETBUTTONSIZE, 0, 0);
        itemSize.x = GET_X_LPARAM(size);
        itemSize.y = GET_Y_LPARAM(size);

        if (pdbi->dwMask & DBIM_MINSIZE)
        {
            if (m_QLaunch)
                pdbi->ptMinSize.x = idealSize.x;
            else
                pdbi->ptMinSize.x = -1;
            pdbi->ptMinSize.y = idealSize.y;
        }
        if (pdbi->dwMask & DBIM_MAXSIZE)
        {
            pdbi->ptMaxSize = maxSize;
        }
        if (pdbi->dwMask & DBIM_INTEGRAL)
        {
            pdbi->ptIntegral = itemSize;
        }
        if (pdbi->dwMask & DBIM_ACTUAL)
        {
            pdbi->ptActual = actualSize;
        }
        if (pdbi->dwMask & DBIM_TITLE)
        {
            if (m_QLaunch || !ILGetDisplayNameEx(NULL, m_pidl, pdbi->wszTitle, ILGDN_INFOLDER))
            {
                pdbi->dwMask &= ~DBIM_TITLE;
            }
        }
        if (pdbi->dwMask & DBIM_MODEFLAGS)
        {
            pdbi->dwModeFlags = DBIMF_NORMAL | DBIMF_VARIABLEHEIGHT | DBIMF_USECHEVRON | DBIMF_NOMARGINS | DBIMF_BKCOLOR;
            if (m_QLaunch)
            {
                pdbi->dwModeFlags |= DBIMF_ADDTOFRONT;
            }
        }
        if (pdbi->dwMask & DBIM_BKCOLOR)
            pdbi->dwMask &= ~DBIM_BKCOLOR;

        return S_OK;
    }

    return E_FAIL;
}

/*****************************************************************************/
// *** IPersistStream ***
STDMETHODIMP CISFBand::GetClassID(OUT CLSID *pClassID)
{
    *pClassID = CLSID_ISFBand;
    return S_OK;
}

STDMETHODIMP CISFBand::IsDirty()
{
    /* The object hasn't changed since the last save! */
    return S_FALSE;
}

STDMETHODIMP CISFBand::Load(IN IStream *pStm)
{
    TRACE("CISFBand::Load called\n");
    /* Nothing to do */
    return S_OK;
}

STDMETHODIMP CISFBand::Save(IN IStream *pStm, IN BOOL fClearDirty)
{
    /* Nothing to do */
    return S_OK;
}

STDMETHODIMP CISFBand::GetSizeMax(OUT ULARGE_INTEGER *pcbSize)
{
    TRACE("CISFBand::GetSizeMax called\n");
    return S_OK;
}

/*****************************************************************************/
// *** IWinEventHandler ***
STDMETHODIMP CISFBand::ContainsWindow(IN HWND hWnd)
{
    if (hWnd == m_hWnd || IsChild(hWnd))
    {
        TRACE("CISFBand::ContainsWindow(0x%p) returns S_OK\n", hWnd);
        return S_OK;
    }

    return S_FALSE;
}

STDMETHODIMP CISFBand::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    switch (uMsg)
    {
        case WM_COMMAND:
        {
            TBBUTTON tb;
            BOOL chk = SendMessage(TB_GETBUTTON, LOWORD(wParam), (LPARAM)&tb);
            if (chk)
                SHInvokeDefaultCommand(m_hWnd, m_pISF, (LPITEMIDLIST)tb.dwData);

            if (theResult)
                *theResult = TRUE;
            break;
        }
        case WM_NOTIFY:
        {
            switch (((LPNMHDR)lParam)->code)
            {
                case NM_RCLICK:
                {
                    HRESULT hr;
                    POINT pt = ((LPNMMOUSE)lParam)->pt; // Already in screen coordinates
                    CComPtr<IContextMenu> picm;
                    HMENU fmenu = CreatePopupMenu();
                    TBBUTTON tb;

                    bool chk = SendMessage(m_hWnd, TB_GETBUTTON, ((LPNMMOUSE)lParam)->dwItemSpec, (LPARAM)&tb);
                    LPITEMIDLIST pidl = (LPITEMIDLIST)tb.dwData;

                    if (chk)
                    {
                        hr = m_pISF->GetUIObjectOf(m_hWnd, 1, &pidl, IID_NULL_PPV_ARG(IContextMenu, &picm));
                        if (FAILED_UNEXPECTEDLY(hr))
                            return hr;

                        hr = picm->QueryContextMenu(fmenu, 0, 1, 0x7FFF, CMF_DEFAULTONLY);
                        if (FAILED_UNEXPECTEDLY(hr))
                            return hr;

                        int id = TrackPopupMenuEx(fmenu, TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RETURNCMD, pt.x, pt.y, m_hWnd, 0);
                        if (id > 0)
                        {
                            CMINVOKECOMMANDINFOEX info = { 0 };
                            info.cbSize = sizeof(info);
                            info.fMask = CMIC_MASK_PTINVOKE;
                            if (GetKeyState(VK_CONTROL) < 0)
                            {
                                info.fMask |= CMIC_MASK_CONTROL_DOWN;
                            }
                            if (GetKeyState(VK_SHIFT) < 0)
                            {
                                info.fMask |= CMIC_MASK_SHIFT_DOWN;
                            }
                            info.hwnd = m_hWnd;
                            info.lpVerb = MAKEINTRESOURCEA(id - 1);
                            info.nShow = SW_SHOWNORMAL;
                            info.ptInvoke = pt;
                            picm->InvokeCommand((LPCMINVOKECOMMANDINFO)&info);
                        }
                    }
                    DestroyMenu(fmenu);

                    if (theResult)
                        *theResult = TRUE;
                    break;
                }
                default:
                    if (theResult)
                        *theResult = FALSE;
            }

            break;
        }
        default:
            if (theResult)
                *theResult = FALSE;
    }

    return S_OK;
}

STDMETHODIMP CISFBand::IsWindowOwner(HWND hWnd)
{
    return (hWnd == m_hWnd) ? S_OK : S_FALSE;
}

/*****************************************************************************/
// *** IOleCommandTarget methods ***
STDMETHODIMP CISFBand::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText)
{
    UNIMPLEMENTED;

    return E_NOTIMPL;
}

STDMETHODIMP CISFBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    if (IsEqualIID(*pguidCmdGroup, IID_IBandSite))
        return S_OK;

    if (IsEqualIID(*pguidCmdGroup, IID_IDeskBand))
    {
        if (nCmdID == DBID_SETWINDOWTHEME)
        {
            if (pvaIn && V_VT(pvaIn) == VT_BSTR && V_BSTR(pvaIn))
            {
                SetWindowTheme(m_hWnd, V_BSTR(pvaIn), NULL);
            }
        }
        return S_OK;
    }

    UNIMPLEMENTED;

    return E_NOTIMPL;
}

/*****************************************************************************/
// *** IShellFolderBand ***
STDMETHODIMP CISFBand::GetBandInfoSFB(PBANDINFOSFB pbi)
{
    if (pbi->dwMask == ISFB_MASK_IDLIST)
    {
        pbi->pidl = ILClone(m_pidl);
        if (!pbi->pidl)
            return E_OUTOFMEMORY;
        return S_OK;
    }

    return E_NOTIMPL;
}

STDMETHODIMP CISFBand::InitializeSFB(IShellFolder *psf, PCIDLIST_ABSOLUTE pidl)
{
    HRESULT hr;

    if (!psf && !pidl)
        return E_INVALIDARG;

    if (psf && pidl)
        return E_INVALIDARG;

    m_pidl.Free();

    if (pidl != NULL)
    {
        CComPtr<IShellFolder> psfDesktop;
        hr = SHGetDesktopFolder(&psfDesktop);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        if (_ILIsDesktop(pidl))
        {
            m_pISF = psfDesktop;
        }
        else
        {
            hr = psfDesktop->BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder, &m_pISF));
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;
        }

        m_pidl.Attach(ILClone(pidl));
        if (!m_pidl)
            ERR("Out of memory\n");
    }

    if (psf != NULL)
    {
        CComPtr<IPersistFolder2> ppf2;
        hr = psf->QueryInterface(IID_PPV_ARG(IPersistFolder2, &ppf2));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        hr = ppf2->GetCurFolder(&m_pidl);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        ATLASSERT(m_pidl);
        m_pISF = psf;
    }

    return S_OK;
}

STDMETHODIMP CISFBand::SetBandInfoSFB( PBANDINFOSFB pbi)
{
    if ((pbi->dwMask & ISFB_MASK_STATE) &&
        (pbi->dwState & ISFB_STATE_QLINKSMODE) &&
        (pbi->dwStateMask & ISFB_STATE_QLINKSMODE))
    {
        m_QLaunch = TRUE;
        ShowHideText(FALSE);
        return BandInfoChanged();
    }

    return E_NOTIMPL;
}

/*****************************************************************************/
// *** IContextMenu ***
STDMETHODIMP CISFBand::GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
    /*HRESULT hr = E_INVALIDARG;

    if (idCmd == IDM_DISPLAY)
    {
        switch (uFlags)
        {
        case GCS_HELPTEXTW:
            // Only useful for pre-Vista versions of Windows that
            // have a Status bar.
            hr = StringCchCopyW(reinterpret_cast<PWSTR>(pszName),
                cchMax,
                L"Display File Name");
            break;

        case GCS_VERBW:
            // GCS_VERBW is an optional feature that enables a caller
            // to discover the canonical name for the verb that is passed in
            // through idCommand.
            hr = StringCchCopyW(reinterpret_cast<PWSTR>(pszName),
                cchMax,
                L"DisplayFileName");
            break;
        }
    }
    return hr;  */

    return S_OK;
}

STDMETHODIMP CISFBand::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    if (!HIWORD(pici->lpVerb))
    {
        switch (LOWORD(pici->lpVerb))
        {
            case IDM_LARGE_ICONS:
            {
                HRESULT hr = SetImageListIconSize(FALSE);
                if (FAILED(hr))
                    return hr;
                return BandInfoChanged();
            }
            case IDM_SMALL_ICONS:
            {
                HRESULT hr = SetImageListIconSize(TRUE);
                if (FAILED(hr))
                    return hr;
                return BandInfoChanged();
            }
            case IDM_OPEN_FOLDER:
            {
                SHELLEXECUTEINFOW shexinfo = { sizeof(shexinfo) };
                shexinfo.fMask = SEE_MASK_IDLIST;
                shexinfo.lpVerb = L"open";
                shexinfo.lpIDList = m_pidl;
                shexinfo.nShow = SW_SHOW;
                if (!ShellExecuteExW(&shexinfo))
                    return E_FAIL;
                break;
            }
            case IDM_SHOW_TEXT:
            {
                ShowHideText(!m_bShowText);
                return BandInfoChanged();
            }
            default:
                return E_FAIL;
        }
    }

    return S_OK;
}

STDMETHODIMP CISFBand::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    HMENU qMenu = LoadMenu(GetModuleHandleW(L"browseui.dll"), MAKEINTRESOURCE(IDM_POPUPMENU));

    CheckMenuItem(qMenu, IDM_SHOW_TEXT, (m_bShowText ? MF_CHECKED : MF_UNCHECKED));

    if (m_bSmallIcon)
    {
        CheckMenuItem(qMenu, IDM_SMALL_ICONS, MF_CHECKED);
        CheckMenuItem(qMenu, IDM_LARGE_ICONS, MF_UNCHECKED);
    }
    else
    {
        CheckMenuItem(qMenu, IDM_LARGE_ICONS, MF_CHECKED);
        CheckMenuItem(qMenu, IDM_SMALL_ICONS, MF_UNCHECKED);
    }

    if (_ILIsDesktop(m_pidl))
        DeleteMenu(qMenu, IDM_OPEN_FOLDER, MF_BYCOMMAND);

    UINT idMax = Shell_MergeMenus(hmenu, GetSubMenu(qMenu, 0), indexMenu, idCmdFirst, idCmdLast, MM_SUBMENUSHAVEIDS);
    DestroyMenu(qMenu);
    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(idMax - idCmdFirst +1));
}

/*****************************************************************************/
// C Constructor

EXTERN_C
HRESULT WINAPI RSHELL_CISFBand_CreateInstance(REFIID riid, void** ppv)
{
    return ShellObjectCreator<CISFBand>(riid, ppv);
}
