#include "stdafx.h"

#include "pathpg.h"

#include <wininet.h>

NMHDR g_nmhdrClick = {NULL, 0, NM_CLICK};

INT_PTR CNetPlacesWizardPage1::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    switch(uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwndDlg, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwndDlg, WM_NOTIFY, OnNotify);
        HANDLE_MSG(hwndDlg, WM_DESTROY, OnDestroy);
    default:
        break;
    }

    return FALSE;
}

BOOL CNetPlacesWizardPage1::OnDestroy(HWND hwnd)
{
    DestroyWindow(m_hwndExampleTT);
    return FALSE;
}

LRESULT CALLBACK CNetPlacesWizardPage1::StaticParentSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TraceEnter(TRACE_ANP_CORE, "CNetPlacesWizardPage1::StaticParentSubclassProc");
    CNetPlacesWizardPage1* pthis = (CNetPlacesWizardPage1*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
    TraceLeaveValue(pthis->ParentSubclassProc(hwnd, uMsg, wParam, lParam));
}

LRESULT CALLBACK CNetPlacesWizardPage1::ParentSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TraceEnter(TRACE_ANP_CORE, "CNetPlacesWizardPage1::ParentSubclassProc");

    switch (uMsg)
    {
    case WM_MOVE:
        TrackExampleTooltip(m_hwnd);
        break;
    }

    TraceLeaveValue(CallWindowProc(m_pfnOldPropSheetParentProc, hwnd, uMsg, wParam, lParam));
}

BOOL CNetPlacesWizardPage1::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
   // Trace(TRACE_LEVEL_FLOW, TEXT("Entering CNetPlacesWizardPage1::OnInitDialog\n"));
        
    TCHAR   tszBuffer[MAX_PATH];
    DWORD   dwError;

    // Hook up the edit box to autocomplete
    SHAutoComplete(GetDlgItem(hwnd, IDC_FOLDER_EDIT), NULL);

    // Subclass the propsheet itself so we can intercept move messages
    HWND hwndParent = GetParent(hwnd);
    
    m_hwnd = hwnd;
    m_pfnOldPropSheetParentProc = (WNDPROC) SetWindowLongPtr(hwndParent, GWLP_WNDPROC, (LONG_PTR) StaticParentSubclassProc);
    SetWindowLongPtr(hwndParent, GWLP_USERDATA, (LONG_PTR) this);

    ZeroMemory(tszBuffer, sizeof(tszBuffer));

    // Bold the welcome title
    SendDlgItemMessage(hwnd, IDC_WELCOME_TITLE, WM_SETFONT, (WPARAM) GetIntroFont(hwnd), 0);

    //
    // Limit the size of the edit controls
    //
    Edit_LimitText(GetDlgItem(hwnd, IDC_FOLDER_EDIT),
                   NETPLACE_MAX_RESOURCE_NAME);

    LPCTSTR pszResource = m_pdata->netplace.GetResourceName();
    if (pszResource)
    {
        dwError = SetDlgItemText(hwnd, IDC_FOLDER_EDIT, pszResource);
    }

    // Create the tooltip window for the examples
    InitExampleTooltip(hwnd);
    TrackExampleTooltip(hwnd);

    // Set focus to the folder edit control
    SetFocus(GetDlgItem(hwnd, IDC_FOLDER_EDIT));

    return FALSE;
}

BOOL CNetPlacesWizardPage1::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   // Trace(TRACE_LEVEL_FLOW, TEXT("Entering CNetPlacesWizardPage1::OnCommand\n"));

    switch(id)
    {
    case IDC_BROWSE:
        {
            LPITEMIDLIST pidl;
            // BUGBUG: Need a CSIDL for computers near me to root the browse
            if (SHGetSpecialFolderLocation(hwnd, CSIDL_NETWORK, &pidl) == NOERROR)
            {
                TCHAR szReturnedPath[MAX_PATH];
                TCHAR szStartPath[MAX_PATH];
                TCHAR szTitle[256];
                
                // Get the path the user has typed so far; we'll try to begin
                // the browse at this point
                HWND hwndFolderEdit = GetDlgItem(hwnd, IDC_FOLDER_EDIT);
                GetWindowText(hwndFolderEdit, szStartPath, ARRAYSIZE(szStartPath));

                // Get the browse dialog title
                LoadString(g_hInstance, IDS_MND_SHAREBROWSE, szTitle, ARRAYSIZE(szTitle));

                BROWSEINFO bi;
                bi.hwndOwner = hwnd;
                bi.pidlRoot = pidl;
                bi.pszDisplayName = szReturnedPath;
                bi.lpszTitle = szTitle;
                bi.ulFlags = BIF_NEWDIALOGSTYLE;
                bi.lpfn = ShareBrowseCallback;
                bi.lParam = (LPARAM) szStartPath;
                bi.iImage = 0;

                LPITEMIDLIST pidlReturned = SHBrowseForFolder(&bi);

                if (pidlReturned != NULL)
                {
                    if (SHGetPathFromIDList(pidlReturned, szReturnedPath))
                    {
                        SetWindowText(hwndFolderEdit, szReturnedPath);

                        BOOL fEnableFinish = (szReturnedPath[0] != 0);
                        PropSheet_SetWizButtons(GetParent(hwnd), fEnableFinish ? PSWIZB_NEXT : 0);
                    }
                    
                    ILFree(pidlReturned);
                }

                ILFree(pidl);
            }
        }
        return TRUE;

    case IDC_FOLDER_EDIT:
        switch(codeNotify)
        {
        case EN_CHANGE:
            SetPageState(hwnd);
            return TRUE;
        case EN_KILLFOCUS:
            // Kill tooltip
            SendMessage(m_hwndExampleTT, TTM_TRACKACTIVATE, (WPARAM) FALSE, 0);
            break;
        }
        break;
    }

    return FALSE;
}

void CNetPlacesWizardPage1::InitExampleTooltip(HWND hwnd)
{
    // Show the examples tool tip
    m_hwndExampleTT = CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL,
                                WS_POPUP | TTS_NOPREFIX | TTS_BALLOON,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                hwnd, NULL, g_hInstance,
                                NULL);

    SetWindowPos(m_hwndExampleTT, HWND_TOPMOST,0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    TCHAR szExamples[256];
    LoadString(g_hInstance, IDS_NETPLACE_EXAMPLES, szExamples, ARRAYSIZE(szExamples));

    TOOLINFO info;
    info.cbSize = sizeof (info);
    info.uFlags = TTF_IDISHWND | TTF_TRACK;
    info.hwnd = hwnd;
    info.uId = (WPARAM) hwnd;
    info.hinst = g_hInstance;
    info.lpszText = szExamples;
    info.lParam = 0;

    SendMessage(m_hwndExampleTT, TTM_ADDTOOL, 0, (LPARAM) &info);

    // Maximum width of the balloon should be the amount of space from the left edge
    // of the edit control to the right edge of the dialog. The following calculates this.
    RECT rcDialog;
    GetClientRect(hwnd, &rcDialog);

    SendMessage(m_hwndExampleTT, TTM_SETMAXTIPWIDTH, 0, (LPARAM) rcDialog.right);
    
    // Set the title and info icon
    TCHAR szTitle[256];
    LoadString(g_hInstance, IDS_NETPLACE_EXAMPLES_TITLE, szTitle, ARRAYSIZE(szTitle));

    SendMessage(m_hwndExampleTT, TTM_SETTITLE, (WPARAM) 1, (LPARAM) szTitle);
}

void CNetPlacesWizardPage1::TrackExampleTooltip(HWND hwnd)
{
    RECT rc;
    GetWindowRect(GetDlgItem(hwnd, IDC_FOLDER_EDIT), &rc);

    // Position the tip stem three-quarters across and directly below the edit box
    DWORD dwPackedCoords = (DWORD) MAKELONG(rc.right - ((rc.right - rc.left) / 4), rc.bottom);

    PostMessage(m_hwndExampleTT, TTM_TRACKPOSITION, 0, (LPARAM) dwPackedCoords);
}
 
BOOL CNetPlacesWizardPage1::OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh)
{
    switch (pnmh->code) 
    {
    case PSN_SETACTIVE:
        // Disable "Next" if the we aren't looking in the DS for
        // our folder and the folder edit is empty
        SetPageState(hwnd);
        return TRUE;

    case PSN_WIZNEXT:
        // Figure out the appropriate next page and go to it
        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, GetNextPage(hwnd));
        return -1;

    case NM_CLICK:
    case NM_RETURN:
        switch (idCtrl)
        {
        case IDC_EXAMPLESLINK:
            ShowExampleTooltip(hwnd);
            return TRUE;
        }
    }

    return FALSE;
}

DWORD CNetPlacesWizardPage1::GetNextPage(HWND hwndDlg)
{
    TraceEnter(TRACE_ANP_CORE, "CNetPlacesWizardPage1::GetNextPage");
    
    // If something should go wrong, dwNextPage = to p1
    DWORD dwNextPage = m_pdata->idWelcomePage;
    CNetworkPlaceData* pNetPlace = &m_pdata->netplace;

    // Also assume we won't show the folders page next
    m_pdata->fShowFoldersPage = FALSE;
    m_pdata->fShowFTPUserPage = FALSE;

    // Get the volume name from the edit box   
    TCHAR szResourceName[NETPLACE_MAX_RESOURCE_NAME + 1];
    FetchText(hwndDlg, IDC_FOLDER_EDIT, szResourceName, 
        ARRAYSIZE(szResourceName));

    HRESULT hrNameSet = pNetPlace->SetResourceName(hwndDlg, szResourceName);

    if (SUCCEEDED(hrNameSet))
    {
        switch (pNetPlace->GetType())
        {
        case CNetworkPlaceData::PLACETYPE_UNCSERVER:
            {
                // Ensure we can enumerate the shares on the server before we continue
                if (SUCCEEDED(m_pdata->netplace.ConnectToUnc(hwndDlg)))
                {
                    m_pdata->fShowFoldersPage = TRUE;
                    dwNextPage = m_pdata->idFoldersPage;
                }
                else
                {
                    dwNextPage = m_pdata->idWelcomePage;
                }
            break;
            }
        case CNetworkPlaceData::PLACETYPE_UNCSHARE:
            {
                dwNextPage = m_pdata->idCompletionPage;
            }
            break;
        case CNetworkPlaceData::PLACETYPE_FTP:
            {
                TCHAR szUserName[INTERNET_MAX_USER_NAME_LENGTH]; *szUserName = 0;

                URL_COMPONENTS urlComps = {sizeof(URL_COMPONENTS), NULL, 0, INTERNET_SCHEME_FTP, NULL, 0,
                                            0, szUserName, ARRAYSIZE(szUserName), NULL, 0, NULL, 0, NULL, 0};

                // By default we want to inform the user about anonymous vs. user logins.
                dwNextPage = m_pdata->idPasswordPage;

                // We see that the user already entered a user name, so skip the password page.
                if (InternetCrackUrl(pNetPlace->GetResourceName(), 0, 0, &urlComps) && szUserName[0])
                    dwNextPage = m_pdata->idCompletionPage;
                else
                    m_pdata->fShowFTPUserPage = TRUE;
            }
            break;
        case CNetworkPlaceData::PLACETYPE_WEBFOLDERURL:
            {
                dwNextPage = m_pdata->idCompletionPage;
            }
            break;
        default:
            {
                // Can't do anything with this crappy string!
                ::DisplayFormatMessage(hwndDlg, IDS_CANTFINDFOLDER_CAPTION,
                    IDS_CANTFINDFOLDER_TEXT, MB_OK|MB_ICONINFORMATION);

                dwNextPage = m_pdata->idWelcomePage;

                // Show examples
                PostMessage(hwndDlg, WM_NOTIFY, (WPARAM) IDC_EXAMPLESLINK, (LPARAM) &g_nmhdrClick);
            }
            break;
        }
    }
    else
    {
        // Can't do anything with this crappy string!
        ::DisplayFormatMessage(hwndDlg, IDS_CANTFINDFOLDER_CAPTION,
            IDS_CANTFINDFOLDER_TEXT, MB_OK|MB_ICONINFORMATION);

        dwNextPage = m_pdata->idWelcomePage;

        // Show examples
        PostMessage(hwndDlg, WM_NOTIFY, (WPARAM) IDC_EXAMPLESLINK, (LPARAM) &g_nmhdrClick);
    }

    TraceLeaveValue(dwNextPage);
}

void CNetPlacesWizardPage1::SetPageState(HWND hwnd)
// Update the state of dialog including which fields are enabled or disabled
// and which of the wizard buttons are shown.
{
    BOOL fEnableFolderEdit = FALSE;
    BOOL fEnableBrowse = FALSE;
    BOOL fEnableNext = FALSE;

    fEnableFolderEdit = TRUE;
    fEnableBrowse = TRUE;
    if (GetWindowTextLength(GetDlgItem(hwnd, IDC_FOLDER_EDIT)) != 0)
    {
        fEnableNext = TRUE;
    }

    EnableWindow(GetDlgItem(hwnd, IDC_FOLDER_EDIT), fEnableFolderEdit);
    EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), fEnableBrowse);

    PropSheet_SetWizButtons(GetParent(hwnd), fEnableNext ? PSWIZB_NEXT : 0);
}

void CNetPlacesWizardPage1::ShowExampleTooltip(HWND hwnd)
{
    // Position tip correctly before showing
    TrackExampleTooltip(hwnd);

    TOOLINFO info;
    info.cbSize = sizeof (info);
    info.hwnd = hwnd;
    info.uId = (WPARAM) hwnd;

    SendMessage(m_hwndExampleTT, TTM_TRACKACTIVATE, (WPARAM) TRUE, (LPARAM) &info);
    SetFocus(GetDlgItem(hwnd, IDC_FOLDER_EDIT));
}