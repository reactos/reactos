/*****************************************************************************\
    FILE: Dialogs.cpp

    DESCRIPTION:
        This file exists to display dialogs needed during FTP operations.
\*****************************************************************************/

#include "priv.h"
#include <mshtmhst.h>
#include "dialogs.h"


#ifdef ADD_ABOUTBOX
/*****************************************************************************\
    FUNCTION: DisplayAboutBox

    DESCRIPTION:
        The about box is now an HTML dialog. It is sent a ~ (tilde) 
    delimited BSTR that has, in this order, version number, 
    person software is licensed to, company software is licensed to, and 
    whether 40, 56, or 128 bit ie is installed.
\*****************************************************************************/
HRESULT DisplayAboutBox(HWND hWnd)
{
    TCHAR szInfo[512];
    szInfo[0] = 0;

    SHAboutInfo(szInfo, ARRAYSIZE(szInfo));     // from shlwapi

    BSTR bstrVal = TCharSysAllocString(szInfo);
    if (bstrVal)
    {
        VARIANT var = {0};      // variant containing version and user info
        var.vt = VT_BSTR;
        var.bstrVal = bstrVal;

        IMoniker *pmk;
        if (SUCCEEDED(CreateURLMoniker(NULL, L"res://msieftp.dll/about.htm", &pmk)))
        {
            ShowHTMLDialog(hWnd, pmk, &var, NULL, NULL);
            pmk->Release();
        }

        SysFreeString(bstrVal);
    }

    return S_OK;
}
#endif // ADD_ABOUTBOX


// This function exists to see if the FTP version of the Copy To Folder
// feature's target is valid.  The shell has "Copy To Folder" in the toolbar
// that accomplishes the copy by using Drag and Drop.  FTP has it's own
// version of this feature in the context menu and file menu that doesn't
// use drag and drop.  This exists because the type of drag and drop
// that we need (CFSTR_FILECONTENTS) isn't correctly implemented on
// old shells and our implmentation is 3 times faster!!!  However,
// we only support file system targets so let's see if this is one
// of those.
BOOL IsValidFTPCopyToFolderTarget(LPCITEMIDLIST pidl)
{
    BOOL fAllowed = FALSE;

    if (pidl)
    {
        TCHAR szPath[MAX_PATH];
    
        if (SHGetPathFromIDList((LPITEMIDLIST)pidl, szPath))
        {
            fAllowed = TRUE;
        }
    }

    return fAllowed;
}


int BrowseCallback(HWND hwnd, UINT msg, LPARAM lParam, LPARAM lpData)
{
    int nResult = 0;

    switch (msg)
    {
    case BFFM_INITIALIZED:
        if (lpData)   // Documentation says it will be NULL but other code does this.
        {
            // we passed ppidl as lpData so pass on just pidl
            // Notice I pass BFFM_SETSELECTIONA which would normally indicate ANSI.
            // I do this because Win95 requires it, but it doesn't matter because I'm
            // only passing a pidl
            SendMessage(hwnd, BFFM_SETSELECTIONA, FALSE, (LPARAM)((LPITEMIDLIST)lpData));
        }
        break;
// BUGBUG: NT #282886: Need to verify if the pass is supported. (A:\ with floppy inserted w/o cancel)

    case BFFM_SELCHANGED:
        // We need to make sure that the selected item is valid for us to
        // accept.  This is because the tree will contain items that don't
        // pass the filter (file sys only) because they have non-filtered
        // children.  We need to disable the OK button when this happens
        // to prevent getting
        SendMessage(hwnd, BFFM_ENABLEOK, 0, (LPARAM)IsValidFTPCopyToFolderTarget((LPCITEMIDLIST) lParam));
        break;

    case BFFM_VALIDATEFAILEDA:
        AssertMsg(0, TEXT("How can we get this?  That's not the structure I sent them."));
        break;

    case BFFM_VALIDATEFAILEDW:
        // If we return zero, then we are saying it's OK.  We only want to do this with
        // file paths.
        nResult = !PathIsRoot((LPCWSTR) lParam);

        // Is this invalid?
        if (nResult)
        {
            TCHAR szErrorTitle[MAX_PATH];
            TCHAR szErrorMsg[MAX_PATH];

            // Yes, so we need to inform the user so they know why the dialog doesn't
            // close.
            EVAL(LoadString(HINST_THISDLL, IDS_HELP_MSIEFTPTITLE, szErrorTitle, ARRAYSIZE(szErrorTitle)));
            EVAL(LoadString(HINST_THISDLL, IDS_FTPERR_BAD_DL_TARGET, szErrorMsg, ARRAYSIZE(szErrorMsg)));
            MessageBox(hwnd, szErrorMsg, szErrorTitle, (MB_OK | MB_ICONERROR));
        }

        break;
    }

    return nResult;
}


/*****************************************************************************\
    FUNCTION: BrowseForDir

    DESCRIPTION:
        Let the user browser for a directory on the local file system
    in order to chose a destination for the FTP transfer.

    S_FALSE will be returned if the user cancelled the action.
\*****************************************************************************/
HRESULT BrowseForDir(HWND hwndParent, LPCTSTR pszTitle, LPCITEMIDLIST pidlDefaultSelect, LPITEMIDLIST * ppidlSelected)
{
    HRESULT hr = S_OK;

    if (ppidlSelected)
    {
        ASSERT(hwndParent);
        BROWSEINFO bi = {0};
        
        bi.hwndOwner = hwndParent;
        bi.lpszTitle = pszTitle;
        bi.lpfn = BrowseCallback;
        bi.lParam = (LPARAM) pidlDefaultSelect;
        bi.ulFlags = (BIF_RETURNONLYFSDIRS | BIF_RETURNFSANCESTORS | BIF_EDITBOX | BIF_USENEWUI | BIF_VALIDATE);

        *ppidlSelected = SHBrowseForFolder(&bi);
        if (!*ppidlSelected)
            hr = S_FALSE;
    }

    return hr;
}


/****************************************************\
    FUNCTION: ShowDialog

    DESCRIPTION:
\****************************************************/
HRESULT CDownloadDialog::ShowDialog(HWND hwndOwner, LPTSTR pszDir, DWORD cchSize, DWORD * pdwDownloadType)
{
    HRESULT hr = S_OK;

    if (DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(IDD_DOWNLOADDIALOG), hwndOwner, DownloadDialogProc, (LPARAM)this))
    {
        StrCpyN(pszDir, HANDLE_NULLSTR(m_pszDir), cchSize);
        *pdwDownloadType = m_dwDownloadType;
        hr = S_OK;
    }
    else
        hr = S_FALSE;

    return hr;
}


/****************************************************\
    FUNCTION: DownloadDialogProc

    DESCRIPTION:
\****************************************************/
INT_PTR CALLBACK CDownloadDialog::DownloadDialogProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    CDownloadDialog * ppd = (CDownloadDialog *)GetWindowLongPtr(hDlg, GWLP_USERDATA);

    if (WM_INITDIALOG == wMsg)
    {
        SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
        ppd = (CDownloadDialog *)lParam;
    }

    if (ppd)
        return ppd->_DownloadDialogProc(hDlg, wMsg, wParam, lParam);

    return TRUE;
}


/****************************************************\
    FUNCTION: _DownloadDialogProc

    DESCRIPTION:
\****************************************************/
BOOL CDownloadDialog::_DownloadDialogProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    switch (wMsg)
    {
    case WM_INITDIALOG:
        return _InitDialog(hDlg);

    case WM_COMMAND:
        return _OnCommand(hDlg, wParam, lParam);
    }

    return FALSE;
}


/****************************************************\
    FUNCTION: _OnCommand

    DESCRIPTION:
\****************************************************/
BOOL CDownloadDialog::_OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    UINT idc = GET_WM_COMMAND_ID(wParam, lParam);

    switch (idc)
    {
    case IDC_DOWNLOAD_BUTTON:
        if (SUCCEEDED(_DownloadButton(hDlg)))
            EndDialog(hDlg, TRUE);
        break;

    case IDCANCEL:
        EndDialog(hDlg, FALSE);
        break;

    case IDC_BROWSE_BUTTON:
        _BrowseButton(hDlg);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


/****************************************************\
    FUNCTION: _InitDialog

    DESCRIPTION:
\****************************************************/
BOOL CDownloadDialog::_InitDialog(HWND hDlg)
{
    HRESULT hr;
    TCHAR szDir[MAX_PATH] = TEXT("C:\\");    // If all else fails.
    DWORD cbSize = sizeof(szDir);

    // Set the Directory
    if ((ERROR_SUCCESS != SHGetValue(HKEY_CURRENT_USER, SZ_REGKEY_INTERNET_EXPLORER, SZ_REGVALUE_DOWNLOAD_DIR, NULL, szDir, &cbSize)) ||
        (!PathFileExists(szDir)))
    {
        LPITEMIDLIST pidlMyDocuments;
        // Create the default dir, which should be "My Documents"

        SHGetSpecialFolderLocation(hDlg, CSIDL_PERSONAL, &pidlMyDocuments);
        if (pidlMyDocuments)
        {
            SHGetPathFromIDList(pidlMyDocuments, szDir);
            ILFree(pidlMyDocuments);
        }
    }
    SetWindowText(GetDlgItem(hDlg, IDC_DOWNLOAD_DIR), szDir);

    // Set the Download Type
    cbSize = sizeof(m_dwDownloadType);
    m_dwDownloadType = FTP_TRANSFER_TYPE_UNKNOWN; // Default.
    SHGetValue(HKEY_CURRENT_USER, SZ_REGKEY_FTPFOLDER, SZ_REGVALUE_DOWNLOAD_TYPE, NULL, &m_dwDownloadType, &cbSize);

    for (UINT idDownloadType = IDS_DL_TYPE_AUTOMATIC; idDownloadType <= IDS_DL_TYPE_BINARY; idDownloadType++)
    {
        LoadString(HINST_THISDLL, idDownloadType, szDir, ARRAYSIZE(szDir));
        SendMessage(GetDlgItem(hDlg, IDC_DOWNLOAD_AS_LIST), CB_ADDSTRING, NULL, (LPARAM) szDir);
    }
    SendMessage(GetDlgItem(hDlg, IDC_DOWNLOAD_AS_LIST), CB_SETCURSEL, (WPARAM) m_dwDownloadType, 0);
    hr = AutoCompleteFileSysInEditbox(GetDlgItem(hDlg, IDC_DOWNLOAD_DIR));
    ASSERT(SUCCEEDED(hr));

    return FALSE;
}


/****************************************************\
    FUNCTION: _DownloadButton

    DESCRIPTION:
\****************************************************/
HRESULT CDownloadDialog::_DownloadButton(HWND hDlg)
{
    HRESULT hr = S_OK;
    TCHAR szDirOriginal[MAX_PATH];    // If all else fails.
    TCHAR szDir[MAX_PATH];    // If all else fails.

    // Get the Directory
    GetWindowText(GetDlgItem(hDlg, IDC_DOWNLOAD_DIR), szDirOriginal, ARRAYSIZE(szDirOriginal));
    EVAL(ExpandEnvironmentStrings(szDirOriginal, szDir, ARRAYSIZE(szDir)));
    Str_SetPtr(&m_pszDir, szDir);
    SHSetValue(HKEY_CURRENT_USER, SZ_REGKEY_INTERNET_EXPLORER, SZ_REGVALUE_DOWNLOAD_DIR, REG_SZ, szDir, ARRAYSIZE(szDir));

    // Get the Download Type
    m_dwDownloadType = (DWORD)SendMessage(GetDlgItem(hDlg, IDC_DOWNLOAD_AS_LIST), CB_GETCURSEL, 0, 0);
    SHSetValue(HKEY_CURRENT_USER, SZ_REGKEY_FTPFOLDER, SZ_REGVALUE_DOWNLOAD_TYPE, REG_DWORD, &m_dwDownloadType, sizeof(m_dwDownloadType));

    // Make sure this path is usable
    ASSERT(hDlg);

    if (S_OK == SHPathPrepareForWriteWrapW(hDlg, NULL, szDir, FO_COPY, SHPPFW_DEFAULT))
    {
        if (!PathIsRoot(szDir) && !PathFileExists(szDir))
        {
            TCHAR szErrorTitle[MAX_PATH];
            TCHAR szErrorMsg[MAX_PATH];
            TCHAR szErrorTemplate[MAX_PATH];

            hr = E_FAIL;    // Until we get a valid directory, we can't do the download.
            EVAL(LoadString(HINST_THISDLL, IDS_HELP_MSIEFTPTITLE, szErrorTitle, ARRAYSIZE(szErrorTitle)));
            EVAL(LoadString(HINST_THISDLL, IDS_FTPERR_CREATEDIRPROMPT, szErrorTemplate, ARRAYSIZE(szErrorTemplate)));
            wnsprintf(szErrorMsg, ARRAYSIZE(szErrorMsg), szErrorTemplate, szDir);

            if (IDYES == MessageBox(hDlg, szErrorMsg, szErrorTitle, (MB_YESNO | MB_ICONQUESTION)))
            {
                if (CreateDirectory(szDir, NULL))
                    hr = S_OK;
                else
                {
                    EVAL(LoadString(HINST_THISDLL, IDS_FTPERR_CREATEFAILED, szErrorMsg, ARRAYSIZE(szErrorMsg)));
                    MessageBox(hDlg, szErrorMsg, szErrorTitle, (MB_OK | MB_ICONERROR));
                }
            }
        }
    }

    return hr;
}


/****************************************************\
    FUNCTION: _BrowseButton

    DESCRIPTION:
\****************************************************/
void CDownloadDialog::_BrowseButton(HWND hDlg)
{
    TCHAR szDefaultDir[MAX_PATH];
    TCHAR szTitle[MAX_PATH];

    GetWindowText(GetDlgItem(hDlg, IDC_DOWNLOAD_DIR), szDefaultDir, ARRAYSIZE(szDefaultDir));

    EVAL(LoadString(HINST_THISDLL, IDS_DLG_DOWNLOAD_TITLE, szTitle, ARRAYSIZE(szTitle)));
    if (S_OK == BrowseForDir(hDlg, szTitle, NULL, NULL))
        SetWindowText(GetDlgItem(hDlg, IDC_DOWNLOAD_DIR), szDefaultDir);
}


/****************************************************\
    Constructor
\****************************************************/
CDownloadDialog::CDownloadDialog()
{
    // NOTE: This can go on the stack so it may not be zero inited.
    m_pszDir = NULL;
    m_hwnd = NULL;
}


/****************************************************\
    Destructor
\****************************************************/
CDownloadDialog::~CDownloadDialog()
{
    Str_SetPtr(&m_pszDir, NULL);
}


