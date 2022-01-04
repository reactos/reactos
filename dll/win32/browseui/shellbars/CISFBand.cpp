/*
 * PROJECT:     ReactOS shell extensions
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/shellext/qcklnch/CISFBand.cpp
 * PURPOSE:     Quick Launch Toolbar (Taskbar Shell Extension)
 * PROGRAMMERS: Shriraj Sawant a.k.a SR13 <sr.official@hotmail.com>
 */

#include "shellbars.h"

#include <commoncontrols.h>
#include <shellapi.h>
#include <wingdi.h>
#include <uxtheme.h>

/*
TODO:
    ** drag and drop support
    ** tooltips
    ** handle change notifications
    ** Fix position of the items context menu
    ** Implement responding to theme change
*/

//*****************************************************************************************
// *** CISFBand ***

CISFBand::CISFBand() :
    m_BandID(0),
    m_pidl(NULL),
    m_textFlag(true),
    m_iconFlag(true),
    m_QLaunch(false)
{
}

CISFBand::~CISFBand()
{
    CloseDW(0);
}

// Toolbar
/*++
* @name CreateSimpleToolbar
*
* Creates a toolbar and fills it up with buttons for enumerated objects.
*
* @param hWndParent
*        Handle to the parent window, which receives the appropriate messages from child toolbar.
*
* @return The error code.
*
*--*/
HRESULT CISFBand::CreateSimpleToolbar(HWND hWndParent)
{
    // Declare and initialize local constants.
    const DWORD buttonStyles = BTNS_AUTOSIZE;

    // Create the toolbar.
    m_hWnd = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
        WS_CHILD | TBSTYLE_FLAT | TBSTYLE_LIST | CCS_NORESIZE | CCS_NODIVIDER, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0,
        hWndParent, NULL, 0, NULL);
    if (m_hWnd == NULL)
        return E_FAIL;

    if (!m_textFlag)
        SendMessage(m_hWnd, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);

    // Set the image list.
    HIMAGELIST* piml;
    HRESULT hr = SHGetImageList(SHIL_SMALL, IID_IImageList, (void**)&piml);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        DestroyWindow();
        return hr;
    }
    SendMessage(m_hWnd, TB_SETIMAGELIST, 0, (LPARAM)piml);

    // Enumerate objects
    CComPtr<IEnumIDList> pEndl;
    LPITEMIDLIST pidl;
    STRRET stret;
    hr = m_pISF->EnumObjects(0, SHCONTF_FOLDERS|SHCONTF_NONFOLDERS, &pEndl);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        DestroyWindow();
        return hr;
    }

    for (int i=0; pEndl->Next(1, &pidl, NULL) != S_FALSE; i++)
    {
         WCHAR sz[MAX_PATH];
         int index = SHMapPIDLToSystemImageListIndex(m_pISF, pidl, NULL);
         hr = m_pISF->GetDisplayNameOf(pidl, SHGDN_NORMAL, &stret);
         if (FAILED_UNEXPECTEDLY(hr))
         {
             StringCchCopyW(sz, MAX_PATH, L"<Unknown-Name>");
         }
         else
             StrRetToBuf(&stret, pidl, sz, _countof(sz));

         TBBUTTON tb = { MAKELONG(index, 0), i, TBSTATE_ENABLED, buttonStyles,{ 0 }, (DWORD_PTR)pidl, (INT_PTR)sz };
         SendMessage(m_hWnd, TB_INSERTBUTTONW, i, (LPARAM)&tb);
    }

    // Resize the toolbar, and then show it.
    SendMessage(m_hWnd, TB_AUTOSIZE, 0, 0);

    return hr;
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
            ShowWindow(SW_HIDE);

            TBBUTTON tb;
            for (int i = 0; SendMessage(m_hWnd, TB_GETBUTTON, i, (LPARAM)&tb); i++)
            {
                CoTaskMemFree((LPITEMIDLIST)tb.dwData);
            }

            DestroyWindow();
            m_hWnd = NULL;
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
                bool chk = SendMessage(m_hWnd, TB_GETBUTTON, LOWORD(wParam), (LPARAM)&tb);
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
                        POINT pt = ((LPNMMOUSE)lParam)->pt;
                        CComPtr<IContextMenu> picm;
                        HMENU fmenu = CreatePopupMenu();
                        TBBUTTON tb;

                        bool chk = SendMessage(m_hWnd, TB_GETBUTTON, ((LPNMMOUSE)lParam)->dwItemSpec, (LPARAM)&tb);
                        LPITEMIDLIST pidl = (LPITEMIDLIST)tb.dwData;

                        if (chk)
                        {
                            ClientToScreen(&pt);
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
        {
            return S_OK;
        }

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

            m_pidl = ILClone(pidl);
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
            m_QLaunch = true;
            m_textFlag = false;
            if (m_hWnd)
                SendMessage(m_hWnd, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);
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
                    m_iconFlag = false;

                    HIMAGELIST* piml = (HIMAGELIST*) SendMessage(m_hWnd, TB_GETIMAGELIST, 0, 0);
                    HRESULT hr = SHGetImageList(SHIL_LARGE, IID_IImageList, (void**)&piml);
                    if (FAILED_UNEXPECTEDLY(hr)) return hr;
                    SendMessage(m_hWnd, TB_SETIMAGELIST, 0, (LPARAM)piml);
                    hr = IUnknown_Exec(m_Site, IID_IDeskBand, DBID_BANDINFOCHANGED, 0, NULL, NULL);
                    if (FAILED_UNEXPECTEDLY(hr)) return hr;
                    break;
                }
                case IDM_SMALL_ICONS:
                {
                    m_iconFlag = true;

                    HIMAGELIST* piml = (HIMAGELIST*)SendMessage(m_hWnd, TB_GETIMAGELIST, 0, 0);
                    HRESULT hr = SHGetImageList(SHIL_SMALL, IID_IImageList, (void**)&piml);
                    if (FAILED_UNEXPECTEDLY(hr)) return hr;
                    SendMessage(m_hWnd, TB_SETIMAGELIST, 0, (LPARAM)piml);
                    hr = IUnknown_Exec(m_Site, IID_IDeskBand, DBID_BANDINFOCHANGED, 0, NULL, NULL);
                    if (FAILED_UNEXPECTEDLY(hr)) return hr;
                    break;
                }
                case IDM_OPEN_FOLDER:
                {
                    SHELLEXECUTEINFO shexinfo;

                    memset(&shexinfo, 0x0, sizeof(shexinfo));

                    shexinfo.cbSize = sizeof(shexinfo);
                    shexinfo.fMask = SEE_MASK_IDLIST;
                    shexinfo.lpVerb = _T("open");
                    shexinfo.lpIDList = m_pidl;
                    shexinfo.nShow = SW_SHOW;

                    if (!ShellExecuteEx(&shexinfo))
                        return E_FAIL;

                    break;
                }
                case IDM_SHOW_TEXT:
                {
                    if (m_textFlag)
                    {
                        m_textFlag = false;
                        SendMessage(m_hWnd, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);
                        HRESULT hr = IUnknown_Exec(m_Site, IID_IDeskBand, DBID_BANDINFOCHANGED, 0, NULL, NULL);
                        if (FAILED_UNEXPECTEDLY(hr)) return hr;
                    }
                    else
                    {
                        m_textFlag = true;
                        SendMessage(m_hWnd, TB_SETEXTENDEDSTYLE, 0, 0);
                        HRESULT hr = IUnknown_Exec(m_Site, IID_IDeskBand, DBID_BANDINFOCHANGED, 0, NULL, NULL);
                        if (FAILED_UNEXPECTEDLY(hr)) return hr;
                    }
                    break;
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

        if(m_textFlag)
            CheckMenuItem(qMenu, IDM_SHOW_TEXT, MF_CHECKED);
        else
            CheckMenuItem(qMenu, IDM_SHOW_TEXT, MF_UNCHECKED);

        if (m_iconFlag)
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
    extern "C"
    HRESULT WINAPI RSHELL_CISFBand_CreateInstance(REFIID riid, void** ppv)
    {
        return ShellObjectCreator<CISFBand>(riid, ppv);
    }

