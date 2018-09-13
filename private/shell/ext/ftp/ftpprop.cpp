/*****************************************************************************\
 *
 *    ftpprop.cpp - Property sheets
 *
\*****************************************************************************/

#include "priv.h"
#include "ftpprop.h"
#include "dbgmem.h"
#include "util.h"
#include "resource.h"


void CFtpProp::_HideCHMOD_UI(HWND hDlg)
{
    // Now, so hide the UI.
    for (int nIndex = IDD_CHMOD; nIndex <= IDC_CHMOD_LAST; nIndex++)
        ShowEnableWindow(GetDlgItem(hDlg, nIndex), FALSE);
}


DWORD CFtpProp::_GetUnixPermissions(void)
{
    LPCITEMIDLIST pidl = m_pflHfpl->GetPidl(0);     // They don't give us a ref.

    return FtpItemID_GetUNIXPermissions(ILGetLastID(pidl));
}


static const DWORD c_dwUnixPermissionArray[] = {UNIX_CHMOD_READ_OWNER, UNIX_CHMOD_WRITE_OWNER, UNIX_CHMOD_EXEC_OWNER,
                                                UNIX_CHMOD_READ_GROUP, UNIX_CHMOD_WRITE_GROUP, UNIX_CHMOD_EXEC_GROUP,
                                                UNIX_CHMOD_READ_ALL, UNIX_CHMOD_WRITE_ALL, UNIX_CHMOD_EXEC_ALL};

// BUGBUG: If we need to set focus rects for the check boxes, we can steal code from:
//         \\rastaman\ntwin\src\shell\security\aclui\chklist.cpp

HRESULT CFtpProp::_SetCHMOD_UI(HWND hDlg)
{
    DWORD dwUnixPermissions = _GetUnixPermissions();

    for (int nIndex = 0; nIndex < ARRAYSIZE(c_dwUnixPermissionArray); nIndex++)
    {
        // Is this permission set?
        CheckDlgButton(hDlg, (IDD_CHMOD + nIndex), (dwUnixPermissions & c_dwUnixPermissionArray[nIndex]));
    }

    return S_OK;
}


DWORD CFtpProp::_GetCHMOD_UI(HWND hDlg)
{
    DWORD dwUnixPermissions = 0;

    for (int nIndex = 0; nIndex < ARRAYSIZE(c_dwUnixPermissionArray); nIndex++)
    {
        // Is it checked in the UI?
        if (IsDlgButtonChecked(hDlg, (IDD_CHMOD + nIndex)))
        {
            // Yes, so set the big.
            dwUnixPermissions |= c_dwUnixPermissionArray[nIndex];
        }
    }

    return dwUnixPermissions;
}


/*****************************************************************************\
    FUNCTION: _SetCHMOD_CB

    DESCRIPTION:
        If we were able to rename the file, return the output pidl.
    Also tell anybody who cares that this LPITEMIDLIST needs to be refreshed.

    The "A" emphasizes that the filename is received in ANSI.

    _UNDOCUMENTED_: The documentation on SetNameOf's treatment of
    the source pidl is random.  It seems to suggest that the source
    pidl is ILFree'd by SetNameOf, but it isn't.
\*****************************************************************************/
HRESULT CFtpProp::_CommitCHMOD(HINTERNET hint, HINTPROCINFO * phpi, BOOL * pfReleaseHint)
{
    HRESULT hr;
    HINTERNET hintResponse;
    WIRECHAR wFtpCommand[MAX_PATH];
    LPCITEMIDLIST pidl = m_pflHfpl->GetPidl(0);     // They don't give us a ref.

    // 1. Create "SITE chmod <m_dwNewPermissions> <filename>" string
    wnsprintfA(wFtpCommand, ARRAYSIZE(wFtpCommand), FTP_CMD_SITE_CHMOD_TEMPL, m_dwNewPermissions, FtpPidl_GetLastItemWireName(pidl));

    hr = FtpCommandWrap(hint, FALSE, FALSE, FTP_TRANSFER_TYPE_ASCII, wFtpCommand, NULL, &hintResponse);
    if (SUCCEEDED(hr))
    {
        // Update the pidl with the new Permissions so our cache isn't out of date.
        CFtpDir * pfd = m_pff->GetFtpDir();

        FtpItemID_SetUNIXPermissions(pidl, m_dwNewPermissions);
        if (pfd)
        {
            pfd->ReplacePidl(pidl, pidl);
            FtpChangeNotify(m_hwnd, FtpPidl_DirChoose(pidl, SHCNE_RENAMEFOLDER, SHCNE_RENAMEITEM), m_pff, pfd, pidl, pidl, TRUE);
            pfd->Release();
        }

        InternetCloseHandleWrap(hintResponse, TRUE);
    }
    else
    {
        DisplayWininetError(m_hwnd, TRUE, HRESULT_CODE(hr), IDS_FTPERR_TITLE_ERROR, IDS_FTPERR_CHMOD, IDS_FTPERR_WININET, MB_OK, NULL);
        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
    }
    return hr;
}


HRESULT CFtpProp::_CommitCHMOD_CB(HINTERNET hint, HINTPROCINFO * phpi, LPVOID pvData, BOOL * pfReleaseHint)
{
    CFtpProp * pfp = (CFtpProp *) pvData;

    return pfp->_CommitCHMOD(hint, phpi, pfReleaseHint);
}


/*****************************************************************************\
    FUNCTION: OnInitDialog

    DESCRIPTION: 
        Fill the dialog with cool stuff.
\*****************************************************************************/
BOOL CFtpProp::OnInitDialog(HWND hDlg)
{
    EVAL(SUCCEEDED(m_ftpDialogTemplate.InitDialog(hDlg, TRUE, IDC_ITEM, m_pff, m_pflHfpl)));

    m_fChangeModeSupported = FALSE; // Default to false

#ifdef FEATURE_CHANGE_PERMISSIONS
    // Is the correct number of items selected to possibly enable the CHMOD feature?
    if (1 == m_pflHfpl->GetCount())
    {
        // Yes, now the question is, is it supported by the server?
        CFtpDir * pfd = m_pff->GetFtpDir();

        if (pfd)
        {
            // Does the server support it?
            m_fChangeModeSupported = pfd->IsCHMODSupported();
            if (m_fChangeModeSupported)
            {
                // Yes, so hide the "Not supported by server" string.
                ShowEnableWindow(GetDlgItem(hDlg, IDC_CHMOD_NOT_ALLOWED), FALSE);
                _SetCHMOD_UI(hDlg); // Update the checkboxes with what's available.
            }
            else
            {
                // No, so hide the CHMOD UI.  The warning that it's not supported by
                // the server is already visible.
                _HideCHMOD_UI(hDlg);
            }

            pfd->Release();
        }
        else
        {
            // No, so hide the CHMOD UI.  This happens on the property sheet for
            // the server.
            _HideCHMOD_UI(hDlg);

            // Also remove the server not supported warning.
            ShowEnableWindow(GetDlgItem(hDlg, IDC_CHMOD_NOT_ALLOWED), FALSE);
        }
    }
    else
    {
        // No, so just remove that UI.
        _HideCHMOD_UI(hDlg);

        // Also remove the server not supported warning.
        ShowEnableWindow(GetDlgItem(hDlg, IDC_CHMOD_NOT_ALLOWED), FALSE);

        // Maybe we need a message saying, "Can't do this with this many items selected"
    }
#endif // FEATURE_CHANGE_PERMISSIONS

    return 1;
}


/*****************************************************************************\
    FUNCTION: OnClose

    DESCRIPTION: 
\*****************************************************************************/
BOOL CFtpProp::OnClose(HWND hDlg)
{
    BOOL fResult = TRUE;

#ifdef FEATURE_CHANGE_PERMISSIONS
    // Did m_ftpDialogTemplate.OnClose() finish all the work it needed in order
    // to close?  This work currently changes the filename.  If so, we
    // will then want to try to apply the UNIX Permission changes if any where
    // made.
    if (m_fChangeModeSupported)
    {
        // Now we need to apply the CHMOD.
        // TODO:
        DWORD dwCurPermissions = _GetUnixPermissions();
        m_dwNewPermissions = _GetCHMOD_UI(hDlg);

        // Did the user change the permissions
        if (dwCurPermissions != m_dwNewPermissions)
        {
            CFtpDir * pfd = m_pff->GetFtpDir();

            if (pfd)
            {
                // Yes, so commit those changes to the server.
                if (FAILED(pfd->WithHint(NULL, m_hwnd, _CommitCHMOD_CB, (LPVOID) this, NULL, m_pff)))
                {
                    EVAL(SUCCEEDED(_SetCHMOD_UI(hDlg)));
                    fResult = FALSE;
                }

                pfd->Release();
            }
        }
    }
#endif // FEATURE_CHANGE_PERMISSIONS

    if (fResult)
        m_ftpDialogTemplate.OnClose(hDlg, m_hwnd, m_pff, m_pflHfpl);

    return fResult;
}


#ifdef FEATURE_CHANGE_PERMISSIONS
INT_PTR CFtpProp::_SetWhiteBGCtlColor(HWND hDlg, HDC hdc, HWND hwndCtl)
{
    INT_PTR fResult = 0;

    if ((hwndCtl == GetDlgItem(hDlg, IDC_CHMOD_GROUPBOX)) ||
        (hwndCtl == GetDlgItem(hDlg, IDC_CHMOD_LABEL_OWNER)) ||
        (hwndCtl == GetDlgItem(hDlg, IDC_CHMOD_LABEL_GROUP)) ||
        (hwndCtl == GetDlgItem(hDlg, IDC_CHMOD_LABEL_ALL)) ||
        (hwndCtl == GetDlgItem(hDlg, IDC_CHMOD_OR)) ||
        (hwndCtl == GetDlgItem(hDlg, IDC_CHMOD_OW)) ||
        (hwndCtl == GetDlgItem(hDlg, IDC_CHMOD_OE)) ||
        (hwndCtl == GetDlgItem(hDlg, IDC_CHMOD_GR)) ||
        (hwndCtl == GetDlgItem(hDlg, IDC_CHMOD_GW)) ||
        (hwndCtl == GetDlgItem(hDlg, IDC_CHMOD_GE)) ||
        (hwndCtl == GetDlgItem(hDlg, IDC_CHMOD_AR)) ||
        (hwndCtl == GetDlgItem(hDlg, IDC_CHMOD_AW)) ||
        (hwndCtl == GetDlgItem(hDlg, IDC_CHMOD_AE)))
    {
        SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
        SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));

        fResult = (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
    }

    return fResult;
}
#endif // FEATURE_CHANGE_PERMISSIONS


/*****************************************************************************\
 *    DlgProc
\*****************************************************************************/
INT_PTR CFtpProp::DlgProc(HWND hDlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    INT_PTR fResult = 0;   // not Handled
    CFtpProp * pfp = (CFtpProp *)GetWindowLongPtr(hDlg, GWLP_USERDATA);

    switch (wm)
    {
    case WM_INITDIALOG:
    {
        LPPROPSHEETPAGE ppsp = (LPPROPSHEETPAGE) lParam;
        pfp =  (CFtpProp *)ppsp->lParam;
        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LPARAM)pfp);

        ASSERT(pfp);
        fResult = pfp->OnInitDialog(hDlg);
    }
    break;

    case WM_NOTIFY:
        if (lParam)
        {
            switch (((NMHDR *)lParam)->code) 
            {
                case PSN_APPLY:
                    if (pfp->OnClose(hDlg))
                    {
                        fResult = FALSE;    // Tell comctl32 I'm happy
                    }
                    else
                    {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID);
                        fResult = TRUE;    // Tell comctl32 to look at the error code and don't close.
                    }
                break;
                
                case PSN_TRANSLATEACCELERATOR:
                    if (pfp->m_ftpDialogTemplate.HasNameChanged(hDlg, pfp->m_pff, pfp->m_pflHfpl))
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                    break;
            }
        }
        break;
#ifdef FEATURE_CHANGE_PERMISSIONS
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
        fResult = pfp->_SetWhiteBGCtlColor(hDlg, (HDC)wParam, (HWND)lParam);
        break;
#endif // FEATURE_CHANGE_PERMISSIONS
    }

    return fResult;
}


/*****************************************************************************\
 *    DoProp_OnThread
 *
 *    Display a property sheet on the current thread.
 *
 *    WARNING!  VIOLATION OF OLE REFERENCE STUFF!
 *
 *    The PFP that comes in must be Release()d when we're done.
 *
 *    The reason is that the caller has "given us" the reference;
 *    we now own it and are responsible for releasing it.
\*****************************************************************************/
DWORD CFtpProp::_PropertySheetThread(void)
{
    HRESULT hrOleInit;
    PROPSHEETHEADER psh;
    PROPSHEETPAGE psp;
    TCHAR szTitle[MAX_PATH];

    FTPDebugMemLeak(DML_TYPE_THREAD | DML_BEGIN);
    hrOleInit = SHOleInitialize(0);
    ASSERT(SUCCEEDED(hrOleInit));

    // This will allow the dialog to work with items outside of the font.
    // So Date, Name, and URL can be in the correct font even through
    // it's not supported by the DLL's font.
    InitComctlForNaviteFonts();
    LoadString(HINST_THISDLL, IDS_PROP_SHEET_TITLE, szTitle, ARRAYSIZE(szTitle));

    // psh.hwndParent being NULL or valid will determine if the property
    // sheet appears in the taskbar.  We do want it there to be consistent
    // with the shell.
    //
    // BUGBUG: Comctl32's property sheet code will make this act modal by
    //         disabling the parent window (m_hwnd).  We need to fix this
    //         (#202885) by creating a dummy window and using that as the
    //         parent.

    psh.hwndParent = SHCreateWorkerWindow(NULL, m_hwnd, 0, 0, NULL, NULL);
    psh.dwSize = sizeof(psh);
    psh.dwFlags = (PSH_PROPTITLE | PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP);
    psh.hInstance = g_hinst;
    psh.pszCaption = szTitle;
    psh.nPages = 1;
    psh.nStartPage = 0;
    psh.ppsp = &psp;

    psp.dwSize = sizeof(psp);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = g_hinst;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_FILEPROP);
    psp.pfnDlgProc = CFtpProp::DlgProc;
    psp.lParam = (LPARAM)this;

    PropertySheet(&psh);

    this->Release();
    FTPDebugMemLeak(DML_TYPE_THREAD | DML_END);

    SHOleUninitialize(hrOleInit);
    return 0;
}


/*****************************************************************************\
 *    CFtpProp_DoProp
 *
 *    Display a property sheet with stuff in it.
\*****************************************************************************/
HRESULT CFtpProp_DoProp(CFtpPidlList * pflHfpl, CFtpFolder * pff, HWND hwnd)
{
    CFtpProp * pfp;
    HRESULT hres = CFtpProp_Create(pflHfpl, pff, hwnd, &pfp);

    if (EVAL(SUCCEEDED(hres)))
    {
        HANDLE hThread;
        DWORD id;

        hThread = CreateThread(0, 0, CFtpProp::_PropertySheetThreadProc, (LPVOID) pfp, 0, &id);
        if (EVAL(hThread))
        {
            // It will release it self if the thread was created.
            CloseHandle(hThread);
            hres = S_OK;
        }
        else
        {
            pfp->Release();
            hres = E_UNEXPECTED;
        }
    }

    return hres;
}


/*****************************************************************************\
 *    CFtpProp_Create
 *
 *    Display a property sheet with stuff in it.
\*****************************************************************************/
HRESULT CFtpProp_Create(CFtpPidlList * pflHfpl, CFtpFolder * pff, HWND hwnd, CFtpProp ** ppfp)
{
    HRESULT hr = E_OUTOFMEMORY;
    CFtpProp * pfp;

    pfp = *ppfp = new CFtpProp();
    if (EVAL(pfp))
    {
        pfp->m_pff = pff;
        if (pff)
            pff->AddRef();

        pfp->m_pflHfpl = pflHfpl;
        if (pflHfpl)
            pflHfpl->AddRef();

        pfp->m_hwnd = hwnd;

        hr = S_OK;
    }

    return hr;
}



/****************************************************\
    Constructor
\****************************************************/
CFtpProp::CFtpProp() : m_cRef(1)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!m_pff);
    ASSERT(!m_hwnd);

    LEAK_ADDREF(LEAK_CFtpProp);
}


/****************************************************\
    Destructor
\****************************************************/
CFtpProp::~CFtpProp()
{
    IUnknown_Set(&m_pff, NULL);
    IUnknown_Set(&m_pflHfpl, NULL);

    DllRelease();
    LEAK_DELREF(LEAK_CFtpProp);
}


//===========================
// *** IUnknown Interface ***
//===========================

ULONG CFtpProp::AddRef()
{
    m_cRef++;
    return m_cRef;
}

ULONG CFtpProp::Release()
{
    ASSERT(m_cRef > 0);
    m_cRef--;

    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}

HRESULT CFtpProp::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = SAFECAST(this, IUnknown *);
    }
    else
    {
        TraceMsg(TF_FTPQI, "CFtpProp::QueryInterface() failed.");
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}
