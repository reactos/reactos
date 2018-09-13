/********************************************************
 ulistpg.cpp

  User Manager user list property page

 History:
  09/23/98: dsheldon created
********************************************************/

#include "stdafx.h"
#include "resource.h"

#include "ulistpg.h"
#include "data.h"
#include "unpage.h"
#include "pwpage.h"
#include "grppage.h"
#include "netpage.h"
#include "autolog.h"

#include "usercom.h"

#include "misc.h"

// Help ID array
static const DWORD rgHelpIds[] =
{
    IDC_AUTOLOGON_CHECK,        IDH_AUTOLOGON_CHECK,
    IDC_LISTTITLE_STATIC,       IDH_USER_LIST,
    IDC_USER_LIST,              IDH_USER_LIST,
    IDC_ADDUSER_BUTTON,         IDH_ADDUSER_BUTTON,
    IDC_REMOVEUSER_BUTTON,      IDH_REMOVEUSER_BUTTON,
    IDC_USERPROPERTIES_BUTTON,  IDH_USERPROPERTIES_BUTTON,
    IDC_PASSWORD_STATIC,        IDH_PASSWORD_BUTTON,
    IDC_CURRENTUSER_ICON,       IDH_PASSWORD_BUTTON,
    IDC_PASSWORD_BUTTON,        IDH_PASSWORD_BUTTON,
    IDC_PWGROUP_STATIC,         (DWORD) -1,
    IDC_ULISTPG_TEXT,           (DWORD) -1,
    IDC_USERLISTPAGE_ICON,      (DWORD) -1,
    0, 0
};

// Control ID arrays for enabling/disabling/moving
static const UINT rgidDisableOnAutologon[] =
{
    IDC_USER_LIST,
    IDC_ADDUSER_BUTTON,
    IDC_REMOVEUSER_BUTTON,
    IDC_USERPROPERTIES_BUTTON,
    IDC_PASSWORD_BUTTON
};

static const UINT rgidDisableOnNoSelection[] =
{
    IDC_REMOVEUSER_BUTTON,
    IDC_USERPROPERTIES_BUTTON,
    IDC_PASSWORD_BUTTON
};

static const UINT rgidMoveOnNoAutologonCheck[] =
{
    IDC_LISTTITLE_STATIC,
    IDC_USER_LIST,
    // IDC_ADDUSER_BUTTON,
    // IDC_e_BUTTON,
    // IDC_USERPROPERTIES_BUTTON,
    // IDC_PWGROUP_STATIC,
    // IDC_CURRENTUSER_ICON,
    // IDC_PASSWORD_STATIC,
    // IDC_PASSWORD_BUTTON
};

CUserlistPropertyPage::~CUserlistPropertyPage()
{
    if (m_himlLarge != NULL)
        ImageList_Destroy(m_himlLarge);
}

INT_PTR CUserlistPropertyPage::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwndDlg, WM_NOTIFY, OnNotify);
        HANDLE_MSG(hwndDlg, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwndDlg, WM_SETCURSOR, OnSetCursor);
        case WM_HELP: return OnHelp(hwndDlg, (LPHELPINFO) lParam);
        case WM_CONTEXTMENU: return OnContextMenu((HWND) wParam);
        case WM_ADDUSERTOLIST: return SUCCEEDED(AddUserToListView(GetDlgItem(hwndDlg, IDC_USER_LIST),
            (CUserInfo*) lParam, (BOOL) wParam));
    }
    
    return FALSE;
}

BOOL CUserlistPropertyPage::OnHelp(HWND hwnd, LPHELPINFO pHelpInfo)
{
    TraceEnter(TRACE_USR_CORE, "CUserlistPropertyPage::OnHelp");

    WinHelp((HWND) pHelpInfo->hItemHandle, m_pData->GetHelpfilePath(), 
        HELP_WM_HELP, (ULONG_PTR) (LPTSTR) rgHelpIds);

    TraceLeaveValue(TRUE);
}

BOOL CUserlistPropertyPage::OnContextMenu(HWND hwnd)
{
    TraceEnter(TRACE_USR_CORE, "CUserlistPropertyPage::OnContextMenu");

    WinHelp(hwnd, m_pData->GetHelpfilePath(), 
        HELP_CONTEXTMENU, (ULONG_PTR) (LPTSTR) rgHelpIds);

    TraceLeaveValue(TRUE);
}

BOOL CUserlistPropertyPage::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    TraceEnter(TRACE_USR_CORE, "CUserlistPropertyPage::OnInitDialog");

    HWND hwndList = GetDlgItem(hwnd, IDC_USER_LIST);

    InitializeListView(hwndList, m_pData->IsComputerInDomain());
    m_pData->Initialize(hwnd);

    SetupList(hwnd);
    
    m_fAutologonCheckChanged = FALSE;

    TraceLeaveValue(TRUE);
}

BOOL CUserlistPropertyPage::OnListViewDeleteItem(HWND hwndList,
                                                 int iItem)
{
    TraceEnter(TRACE_USR_CORE, "CUserlistPropertyPage::OnListViewDeleteItem");

    LVITEM lvi = {0};
    lvi.iItem = iItem;
    lvi.mask = LVIF_PARAM;

    ListView_GetItem(hwndList, &lvi);

    CUserInfo* pUserInfo = (CUserInfo*) lvi.lParam;

    if (NULL != pUserInfo)
    {
        delete pUserInfo;
    }

    TraceLeaveValue(TRUE);
}

BOOL CUserlistPropertyPage::OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh)
{
    TraceEnter(TRACE_USR_CORE, "CUserlistPropertyPage::OnNotify");
    BOOL fResult = TRUE;

    switch (pnmh->code)
    {
    case PSN_APPLY:
        {
            long applyEffect = OnApply(hwnd);
            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, applyEffect);
        }
        break;

    case LVN_GETINFOTIP:
        fResult = OnGetInfoTip(pnmh->hwndFrom, (LPNMLVGETINFOTIP) pnmh);
        break;

    case LVN_ITEMCHANGED:
        fResult = OnListViewItemChanged(hwnd);
        break;

    case LVN_DELETEITEM:
        fResult = OnListViewDeleteItem(GetDlgItem(hwnd, IDC_USER_LIST), 
            ((LPNMLISTVIEW) pnmh)->iItem);
        break;

    case NM_DBLCLK:
        LaunchUserProperties(hwnd);
        fResult = TRUE;
        break;

    case LVN_COLUMNCLICK:
        {
            int iColumn = ((LPNMLISTVIEW) pnmh)->iSubItem;
            
            // Want to work with 1-based columns so we can use zero as
            // a special value
            iColumn += 1;

            // If we aren't showing the domain column because we're in
            // non-domain mode, then map column 2 (group since we're not in
            // domain mode to column 3 since the callback always expects 
            // the columns to be, "username", "domain", "group".
            if ((iColumn == 2) && (!m_pData->IsComputerInDomain()))
            {
                iColumn = 3;
            }

            if (m_iReverseColumnIfSelected == iColumn)
            {
                m_iReverseColumnIfSelected = 0;
                iColumn = -iColumn;
            }
            else
            {
                m_iReverseColumnIfSelected = iColumn;
            }

            ListView_SortItems(pnmh->hwndFrom, ListCompare, (LPARAM) iColumn);
            fResult = TRUE;
        }
        break;

    default:
        fResult = FALSE;
        break;
    }

    TraceLeaveValue(fResult);
}

BOOL CUserlistPropertyPage::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    TraceEnter(TRACE_USR_CORE, "CUserlistPropertyPage::OnCommand");
    
    switch (id)
    {
    case IDC_ADDUSER_BUTTON:
        if (m_pData->IsComputerInDomain())
        {
            // Launch the wizard to add a network user to a local group
            LaunchAddNetUserWizard(hwnd);
        }
        else
        {
            // No domain; create a new local machine user
            LaunchNewUserWizard(hwnd);
        }
        TraceLeaveValue(TRUE);

    case IDC_REMOVEUSER_BUTTON:
        OnRemove(hwnd);
        TraceLeaveValue(TRUE);       
        
    case IDC_AUTOLOGON_CHECK:
        m_fAutologonCheckChanged = TRUE;

        // We may need some user input to change the autologon state
        SetAutologonState(hwnd, BST_UNCHECKED == 
            SendMessage(GetDlgItem(hwnd, IDC_AUTOLOGON_CHECK), BM_GETCHECK, 0, 0));
        SetPageState(hwnd);
        break;

    case IDC_ADVANCED_BUTTON:
        {
            // Launch the MMC local user manager
            STARTUPINFO startupinfo = {0};
            startupinfo.cb = sizeof (startupinfo);

            PROCESS_INFORMATION process_information;

            // Consider using env. vars and ExpandEnvironmentString here
            static const TCHAR szMMCCommandLineFormat[] = 
                TEXT("mmc.exe /computer=%s %%systemroot%%\\system32\\lusrmgr.msc");
            
            TCHAR szMMCCommandLine[MAX_PATH];
            TCHAR szExpandedCommandLine[MAX_PATH];

            wnsprintf(szMMCCommandLine, ARRAYSIZE(szMMCCommandLine), szMMCCommandLineFormat,
            m_pData->GetComputerName());

            if (ExpandEnvironmentStrings(szMMCCommandLine, szExpandedCommandLine, 
                ARRAYSIZE(szExpandedCommandLine)) > 0)
            {
                if (CreateProcess(NULL, szExpandedCommandLine, NULL, NULL, FALSE, 0, NULL, NULL,
                    &startupinfo, &process_information))
                {
                    CloseHandle(process_information.hProcess);
                    CloseHandle(process_information.hThread);
                }
            }
        }
        break;

    case IDC_PASSWORD_BUTTON:
        LaunchSetPasswordDialog(hwnd);

        break;

    case IDC_USERPROPERTIES_BUTTON:
        LaunchUserProperties(hwnd);
        break;
    }

    TraceLeaveValue(FALSE);
}

BOOL CUserlistPropertyPage::OnSetCursor(HWND hwnd, HWND hwndCursor, UINT codeHitTest, UINT msg)
{
    TraceEnter(TRACE_USR_CORE, "CUserlistPropertyPage::OnSetCursor");

    BOOL fHandled = FALSE;

    // If the thread is filling, handle by setting the appstarting cursor
    if (m_pData->GetUserListLoader()->InitInProgress())
    {
        fHandled = TRUE;
        SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
    }

    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, fHandled);
    TraceLeaveValue(TRUE);
}


BOOL CUserlistPropertyPage::OnGetInfoTip(HWND hwndList, LPNMLVGETINFOTIP pGetInfoTip)
{
    TraceEnter(TRACE_USR_CORE, "CUserlistPropertyPage::OnGetInfoTip");

    // Get the UserInfo structure for the selected item
    LVITEM lvi;
    lvi.mask = LVIF_PARAM;
    lvi.iItem = pGetInfoTip->iItem;
    lvi.iSubItem = 0;

    if ((lvi.iItem >= 0) && (ListView_GetItem(hwndList, &lvi)))
    {
        CUserInfo* pUserInfo = (CUserInfo*) lvi.lParam;
        TraceAssert(pUserInfo != NULL);

        // Ensure full name and comment are available
        pUserInfo->GetExtraUserInfo();

        // Make a string containing our "Full Name: %s\nComment: %s" message
        if ((pUserInfo->m_szFullName[0] != TEXT('\0')) &&
            (pUserInfo->m_szComment[0] != TEXT('\0')))
        {
            // We have a full name and comment
            FormatMessageString(IDS_USR_TOOLTIPBOTH_FORMAT, pGetInfoTip->pszText, pGetInfoTip->cchTextMax, pUserInfo->m_szFullName, pUserInfo->m_szComment);
        }
        else if (pUserInfo->m_szFullName[0] != TEXT('\0'))
        {
            // We only have full name
            FormatMessageString(IDS_USR_TOOLTIPFULLNAME_FORMAT, pGetInfoTip->pszText, pGetInfoTip->cchTextMax, pUserInfo->m_szFullName);
        }
        else if (pUserInfo->m_szComment[0] != TEXT('\0'))
        {
            // We only have comment
            FormatMessageString(IDS_USR_TOOLTIPCOMMENT_FORMAT, pGetInfoTip->pszText, pGetInfoTip->cchTextMax, pUserInfo->m_szComment);
        }
        else
        {
            // We have no extra information - do nothing (show no tip)
        }
    }

    TraceLeaveValue(TRUE);
}

struct MYCOLINFO
{
    int percentWidth;
    UINT idString;
};

HRESULT CUserlistPropertyPage::InitializeListView(HWND hwndList, BOOL fShowDomain)
{
    // Array of icon ids icons 0, 1, and 2 respectively
    static const UINT rgIcons[] = 
    {
        IDI_USR_LOCALUSER_ICON,
        IDI_USR_DOMAINUSER_ICON,
        IDI_USR_GROUP_ICON
    };

    // Array of relative column widths, for columns 0, 1, and 2 respectively
    static const MYCOLINFO rgColWidthsWithDomain[] = 
    {
        {40, IDS_USR_NAME_COLUMN},
        {30, IDS_USR_DOMAIN_COLUMN},
        {30, IDS_USR_GROUP_COLUMN}
    };

    static const MYCOLINFO rgColWidthsNoDomain[] =
    {
        {50, IDS_USR_NAME_COLUMN},
        {50, IDS_USR_GROUP_COLUMN}
    };

    TraceEnter(TRACE_USR_CORE, "CUserlistPropertyPage::InitializeListView");
    HRESULT hr = S_OK;

    // Create a listview with three columns
    RECT rect;
    GetClientRect(hwndList, &rect);

    // Width of our window minus width of a verticle scroll bar minus one for the
    // little bevel at the far right of the header.
    int cxListView = (rect.right - rect.left) - GetSystemMetrics(SM_CXVSCROLL) - 1;

    // Make our columns
    int i;
    int nColumns; 
    const MYCOLINFO* pColInfo;
    if (fShowDomain)
    {
        nColumns = ARRAYSIZE(rgColWidthsWithDomain);
        pColInfo = rgColWidthsWithDomain;
    }
    else
    {
        nColumns = ARRAYSIZE(rgColWidthsNoDomain);
        pColInfo = rgColWidthsNoDomain;
    }      

    LVCOLUMN lvc;
    lvc.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
    for (i = 0; i < nColumns; i++)
    {
        TCHAR szText[MAX_PATH];
        // Load this column's caption
        LoadString(g_hInstance, pColInfo[i].idString, szText, ARRAYSIZE(szText));

        lvc.iSubItem = i;
        lvc.cx = (int) MulDiv(pColInfo[i].percentWidth, cxListView, 100);
        lvc.pszText = szText;

        ListView_InsertColumn(hwndList, i, &lvc);
    }

    UINT flags = ILC_MASK;
    if(IS_WINDOW_RTL_MIRRORED(hwndList))
    {
        flags |= ILC_MIRROR;
    }
    // Create an image list for the listview
    HIMAGELIST himlSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON), 
        GetSystemMetrics(SM_CYSMICON), flags, 0, ARRAYSIZE(rgIcons));

    // Large image lists for the "set password" group icon
    m_himlLarge = ImageList_Create(32, 32, flags, 0, ARRAYSIZE(rgIcons));

    if (himlSmall && m_himlLarge)
    {
        // Add our icons to the image list
        for(i = 0; i < ARRAYSIZE(rgIcons); i ++)
        {
            HICON hIconSmall = (HICON) LoadImage(g_hInstance, MAKEINTRESOURCE(rgIcons[i]), IMAGE_ICON,
                16, 16, 0);

            if (hIconSmall)
            {
                ImageList_AddIcon(himlSmall, hIconSmall);
                DestroyIcon(hIconSmall);
            }

            HICON hIconLarge = (HICON) LoadImage(g_hInstance, MAKEINTRESOURCE(rgIcons[i]), IMAGE_ICON,
                32, 32, 0);

            if (hIconLarge)
            {
                ImageList_AddIcon(m_himlLarge, hIconLarge);
                DestroyIcon(hIconLarge);
            }
        }
    }

    ListView_SetImageList(hwndList, himlSmall, LVSIL_SMALL);

    // Set extended styles for the listview
    ListView_SetExtendedListViewStyleEx(hwndList, 
        LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP, 
        LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

    // Set some settings for our tooltips - stolen from defview.cpp code
    HWND hwndInfoTip = ListView_GetToolTips(hwndList);
    if (hwndInfoTip != NULL)
    {
        //make the tooltip window  to be topmost window
        SetWindowPos(hwndInfoTip, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

        // increase the ShowTime (the delay before we show the tooltip) to 2 times the default value
        LRESULT uiShowTime = SendMessage(hwndInfoTip, TTM_GETDELAYTIME, TTDT_INITIAL, 0);
        SendMessage(hwndInfoTip, TTM_SETDELAYTIME, TTDT_INITIAL, uiShowTime * 2);
    }

    TraceLeaveResult(hr);
}

HRESULT CUserlistPropertyPage::AddUserToListView(HWND hwndList, 
                                                 CUserInfo* pUserInfo,
                                                 BOOL fSelectUser /* = FALSE */)
{
    TraceEnter(TRACE_USR_CORE, "CUserlistPropertyPage::AddUserToListView");

    HRESULT hr = S_OK;

    // Do the add

    LVITEM lvi;
    int iItem;
    lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM; 

    // Always select the first loaded user
    if (ListView_GetItemCount(hwndList) == 0)
    {
        fSelectUser = TRUE;
    }

    if (fSelectUser)
    {
        lvi.mask |= LVIF_STATE;
        lvi.state = lvi.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
    }

    lvi.iItem = 0;
    lvi.iSubItem = 0;
    lvi.pszText = pUserInfo->m_szUsername;
    lvi.iImage = pUserInfo->m_userType;
    lvi.lParam = (LPARAM) pUserInfo;

    iItem = ListView_InsertItem(hwndList, &lvi);

    if (iItem >= 0)
    {
        if (fSelectUser)
        {
            // Make the item visible
            ListView_EnsureVisible(hwndList, iItem, FALSE);
        }

        // Success! Now add the subitems (domain, groups)
        lvi.iItem = iItem;
        lvi.mask = LVIF_TEXT;
    
        // Only add the domain field if the user is in a domain
        if (::IsComputerInDomain())
        {
            lvi.iSubItem = 1;
            lvi.pszText = pUserInfo->m_szDomain;
            ListView_SetItem(hwndList, &lvi);

            // User is in a domain; group should be third column
            lvi.iSubItem = 2;
        }
        else
        {
            // User isn't in a domain, group should be second column
            lvi.iSubItem = 1;
        }

        // Add group regardless of whether user is in a domain
        lvi.pszText = pUserInfo->m_szGroups;
        ListView_SetItem(hwndList, &lvi);
    }

    TraceLeaveResult(hr);
}

HRESULT CUserlistPropertyPage::LaunchNewUserWizard(HWND hwndParent)
{
    TraceEnter(TRACE_USR_CORE, "CUserlistPropertyPage::LaunchNewUserWizard");
    
    HRESULT hr = S_OK;

    static const int nPages = 3;
    int cPages = 0;
    HPROPSHEETPAGE rghPages[nPages];

    // Create a new user record
    CUserInfo* pNewUser = new CUserInfo;
    if (pNewUser != NULL)
    {
        pNewUser->InitializeForNewUser();
        pNewUser->m_userType = CUserInfo::LOCALUSER;

        PROPSHEETPAGE psp = {0};
        // Common propsheetpage settings
        psp.dwSize = sizeof (psp);
        psp.hInstance = g_hInstance;
        psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;

        // Page 1: Username entry page
        psp.pszTemplate = MAKEINTRESOURCE(IDD_USR_USERNAME_WIZARD_PAGE);
        CUsernameWizardPage page1(pNewUser);
        page1.SetPropSheetPageMembers(&psp);
        rghPages[cPages++] = CreatePropertySheetPage(&psp);

        // Page 2: Password page
        psp.pszTemplate = MAKEINTRESOURCE(IDD_USR_PASSWORD_WIZARD_PAGE);
        CPasswordWizardPage page2(pNewUser);
        page2.SetPropSheetPageMembers(&psp);
        rghPages[cPages++] = CreatePropertySheetPage(&psp);

        // Page 3: Local group addition
        psp.pszTemplate = MAKEINTRESOURCE(IDD_USR_CHOOSEGROUP_WIZARD_PAGE);
        CGroupWizardPage page3(pNewUser, m_pData->GetGroupList());
        page3.SetPropSheetPageMembers(&psp);
        rghPages[cPages++] = CreatePropertySheetPage(&psp);

        TraceAssert(cPages <= nPages);

        PROPSHEETHEADER psh = {0};
        psh.dwSize = sizeof (psh);
        psh.dwFlags = PSH_NOCONTEXTHELP | PSH_WIZARD | PSH_WIZARD_LITE;
        psh.hwndParent = hwndParent;
        psh.hInstance = g_hInstance;
        psh.nPages = nPages;
        psh.phpage = rghPages;
 
        int iRetCode = (int)PropertySheet(&psh);
    
        if (iRetCode == IDOK)
        {
            AddUserToListView(GetDlgItem(hwndParent, IDC_USER_LIST), pNewUser, TRUE);
        }
        else
        {
            // User clicked cancel
            delete pNewUser;
            pNewUser = NULL;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
        TraceMsg("CreateNewUserInfo() failed!");
        DisplayFormatMessage(hwndParent, IDS_USR_NEWUSERWIZARD_CAPTION, 
            IDS_USR_CREATE_MISC_ERROR, MB_OK | MB_ICONERROR);
    }

    if (FAILED(hr))
    {
        if (pNewUser)
        {
            delete pNewUser;
        }
    }

    TraceLeaveResult(hr);
}

HRESULT CUserlistPropertyPage::LaunchAddNetUserWizard(HWND hwndParent)
{
    TraceEnter(TRACE_USR_CORE, "CUserlistPropertyPage::LaunchAddNetUserWizard");
    HRESULT hr = S_OK;

    static const int nPages = 2;
    int cPages = 0;
    HPROPSHEETPAGE rghPages[nPages];

    // Create a new user record
    CUserInfo* pNewUser = new CUserInfo;
    if (pNewUser != NULL)
    {
        pNewUser->InitializeForNewUser();
        pNewUser->m_userType = CUserInfo::DOMAINUSER;

        PROPSHEETPAGE psp = {0};
        // Common propsheetpage settings
        psp.dwSize = sizeof (psp);
        psp.hInstance = g_hInstance;
        psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;

        // Page 1: Find a network user page
        psp.pszTemplate = MAKEINTRESOURCE(IDD_USR_FINDNETUSER_WIZARD_PAGE);
        CNetworkUserWizardPage page1(pNewUser);
        page1.SetPropSheetPageMembers(&psp);
        rghPages[cPages++] = CreatePropertySheetPage(&psp);

        // Page 2: Local group addition
        psp.pszTemplate = MAKEINTRESOURCE(IDD_USR_CHOOSEGROUP_WIZARD_PAGE);
        CGroupWizardPage page2(pNewUser, m_pData->GetGroupList());
        page2.SetPropSheetPageMembers(&psp);
        rghPages[cPages++] = CreatePropertySheetPage(&psp);

        TraceAssert(cPages <= nPages);

        PROPSHEETHEADER psh = {0};
        psh.dwSize = sizeof (psh);
        psh.dwFlags = PSH_NOCONTEXTHELP | PSH_WIZARD | PSH_WIZARD_LITE;
        psh.hwndParent = hwndParent;
        psh.hInstance = g_hInstance;
        psh.nPages = nPages;
        psh.phpage = rghPages;
 
        int iRetCode = (int)PropertySheet(&psh);
    
        if (iRetCode == IDOK)
        {
            AddUserToListView(GetDlgItem(hwndParent, IDC_USER_LIST), pNewUser, TRUE);
            m_pData->UserInfoChanged(pNewUser->m_szUsername, pNewUser->m_szDomain);
        }
        else
        {
            // No errors, but the user clicked Cancel...
            delete pNewUser;
            pNewUser = NULL;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
        TraceMsg("CreateNewUserInfo() failed!");
        DisplayFormatMessage(hwndParent, IDS_USR_NEWUSERWIZARD_CAPTION,
            IDS_USR_CREATE_MISC_ERROR, MB_OK | MB_ICONERROR);
    }

    if (FAILED(hr))
    {
        if (pNewUser)
        {
            delete pNewUser;
        }
    }

    TraceLeaveResult(hr);
}

HRESULT CUserlistPropertyPage::LaunchUserProperties(HWND hwndParent)
{
    TraceEnter(TRACE_USR_CORE, "CUserlistPropertyPage::LaunchUserProperties");
    
    HRESULT hr = S_OK;

    ADDPROPSHEETDATA apsd;
    apsd.nPages = 0;

    // Create a new user record
    HWND hwndList = GetDlgItem(hwndParent, IDC_USER_LIST);
    CUserInfo* pUserInfo = GetSelectedUserInfo(hwndList);
    if (pUserInfo != NULL)
    {
        pUserInfo->GetExtraUserInfo();
        PROPSHEETPAGE psp = {0};
        // Common propsheetpage settings
        psp.dwSize = sizeof (psp);
        psp.hInstance = g_hInstance;
        psp.dwFlags = PSP_DEFAULT;

        // If we have a local user, show both the username and group page, ow
        // just the group page
        // Page 1: Username entry page
        psp.pszTemplate = MAKEINTRESOURCE(IDD_USR_USERNAME_PROP_PAGE);
        CUsernamePropertyPage page1(pUserInfo);
        page1.SetPropSheetPageMembers(&psp);

        // Only actually create the prop page if we have a local user
        if (pUserInfo->m_userType == CUserInfo::LOCALUSER)
        {
            apsd.rgPages[apsd.nPages++] = CreatePropertySheetPage(&psp);
        }

        // Always add the second page
        // Page 2: Local group addition
        psp.pszTemplate = MAKEINTRESOURCE(IDD_USR_CHOOSEGROUP_PROP_PAGE);
        CGroupPropertyPage page2(pUserInfo, m_pData->GetGroupList());
        page2.SetPropSheetPageMembers(&psp);
        apsd.rgPages[apsd.nPages++] = CreatePropertySheetPage(&psp);

        HPSXA hpsxa = AddExtraUserPropPages(&apsd, pUserInfo->m_psid);

        PROPSHEETHEADER psh = {0};
        psh.dwSize = sizeof (psh);
        psh.dwFlags = PSH_DEFAULT | PSH_PROPTITLE;

        TCHAR szDomainUser[MAX_USER + MAX_DOMAIN + 2];
        MakeDomainUserString(pUserInfo->m_szDomain, pUserInfo->m_szUsername, szDomainUser,
            ARRAYSIZE(szDomainUser));

        psh.pszCaption = szDomainUser;
        psh.hwndParent = hwndParent;
        psh.hInstance = g_hInstance;
        psh.nPages = apsd.nPages;
        psh.phpage = apsd.rgPages;
 
        int iRetCode = (int)PropertySheet(&psh);

        if (hpsxa != NULL)
            SHDestroyPropSheetExtArray(hpsxa);

        if (iRetCode == IDOK)
        {
            // PropSheet_Changed(GetParent(hwndParent), hwndParent);

            // So that we don't delete this pUserInfo when we remove
            // this user from the list:
            m_pData->UserInfoChanged(pUserInfo->m_szUsername, (pUserInfo->m_szDomain[0] == 0) ? NULL : pUserInfo->m_szDomain);
            RemoveSelectedUserFromList(hwndList, FALSE);
            AddUserToListView(hwndList, pUserInfo, TRUE);
        }
    }
    else
    {
        TraceMsg("Couldn't Get selected CUserInfo");
    }

    TraceLeaveResult(hr);
}


CUserInfo* CUserlistPropertyPage::GetSelectedUserInfo(HWND hwndList)
{
    TraceEnter(TRACE_USR_CORE, "CUserlistPropertyPage::GetSelectedUserInfo");
    CUserInfo* pUserInfo = NULL;
        
    int iItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

    if (iItem >= 0)
    {
        LVITEM lvi = {0};
        lvi.mask = LVIF_PARAM;
        lvi.iItem = iItem;
        if (ListView_GetItem(hwndList, &lvi))
        {
            pUserInfo = (CUserInfo*) lvi.lParam;
        }
    }

    TraceLeaveValue(pUserInfo);
}

void CUserlistPropertyPage::RemoveSelectedUserFromList(HWND hwndList, 
                                                       BOOL fFreeUserInfo)
{
    TraceEnter(TRACE_USR_CORE, "CUserlistPropertyPage::RemoveSelectedUserFromList");

    int iItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

    // If we don't want to delete this user info, better set it to NULL
    if (!fFreeUserInfo)
    {
        LVITEM lvi = {0};
        lvi.iItem = iItem;
        lvi.mask = LVIF_PARAM;
        lvi.lParam = (LPARAM) (CUserInfo*) NULL;

        ListView_SetItem(hwndList, &lvi);
    }

    ListView_DeleteItem(hwndList, iItem);

    int iSelect = iItem > 0 ? iItem - 1 : 0;

    ListView_SetItemState(hwndList, iSelect, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    
    SetFocus(hwndList);

    TraceLeaveVoid();
}


void CUserlistPropertyPage::OnRemove(HWND hwnd)
{
    TraceEnter(TRACE_USR_CORE, "CUserlistPropertyPage::OnRemove");
    HWND hwndList = GetDlgItem(hwnd, IDC_USER_LIST);
    CUserInfo* pUserInfo = GetSelectedUserInfo(hwndList);

    if (pUserInfo != NULL)
    {
        if (ConfirmRemove(hwnd, pUserInfo) == IDYES)
        {
            if (SUCCEEDED(pUserInfo->Remove()))
            {
                RemoveSelectedUserFromList(hwndList, TRUE);
            }
            else
            {
                // Error removing user
               TCHAR szDisplayName[MAX_USER + MAX_DOMAIN + 2];
        
                ::MakeDomainUserString(pUserInfo->m_szDomain, pUserInfo->m_szUsername, szDisplayName,
                    ARRAYSIZE(szDisplayName));

                DisplayFormatMessage(hwnd, IDS_USR_APPLET_CAPTION,
                    IDS_USR_REMOVE_MISC_ERROR, MB_ICONERROR | MB_OK, szDisplayName);
            }
        }
    }
    else
    {
        // Unexpected! There should always be a selection
        TraceMsg("GetSelectedUserInfo failed");
    }

    TraceLeaveVoid();
}

int CUserlistPropertyPage::ConfirmRemove(HWND hwnd, CUserInfo* pUserInfo)
{
    TraceEnter(TRACE_USR_CORE, "CUserlistPropertyPage::ConfirmRemove");
    
    TCHAR szDomainUser[MAX_USER + MAX_DOMAIN + 2];
    MakeDomainUserString(pUserInfo->m_szDomain, pUserInfo->m_szUsername, szDomainUser,
        ARRAYSIZE(szDomainUser));

    int iReturn = DisplayFormatMessage(hwnd, IDS_USR_APPLET_CAPTION, IDS_USR_REMOVEUSER_WARNING,
        MB_ICONEXCLAMATION | MB_YESNO, szDomainUser);

    TraceLeaveValue(iReturn);
}

void CUserlistPropertyPage::SetPageState(HWND hwnd)
{
    TraceEnter(TRACE_USR_CORE, "CUserlistPropertyPage::SetPageState");
    BOOL fAutologon = (BST_UNCHECKED == 
        SendMessage(GetDlgItem(hwnd, IDC_AUTOLOGON_CHECK), BM_GETCHECK, 0, 0));

    EnableControls(hwnd, rgidDisableOnAutologon, ARRAYSIZE(rgidDisableOnAutologon),
        !fAutologon);

    HWND hwndList = GetDlgItem(hwnd, IDC_USER_LIST);
    CUserInfo* pUserInfo = GetSelectedUserInfo(hwndList);

    if (pUserInfo != NULL)
    {
        // EnableControls(hwnd, rgidDisableOnNoSelection, ARRAYSIZE(rgidDisableOnNoSelection),
        //     TRUE);
        
        TCHAR szPWGroup[128];
        FormatMessageString(IDS_USR_PWGROUP_FORMAT, szPWGroup, ARRAYSIZE(szPWGroup), pUserInfo->m_szUsername);
        SetWindowText(GetDlgItem(hwnd, IDC_PWGROUP_STATIC), szPWGroup);

        TCHAR szPWMessage[128];

        // If the logged on user is the selected user
        CUserInfo* pLoggedOnUser = m_pData->GetLoggedOnUserInfo();
        if ((StrCmpI(pUserInfo->m_szUsername, pLoggedOnUser->m_szUsername) == 0) &&
            (StrCmpI(pUserInfo->m_szDomain, pLoggedOnUser->m_szDomain) == 0))
        {
            LoadString(g_hInstance, IDS_USR_YOURPWMESSAGE_FORMAT, szPWMessage,
                ARRAYSIZE(szPWMessage));
            EnableWindow(GetDlgItem(hwnd, IDC_PASSWORD_BUTTON), FALSE);
        }
        // If the user is a local user
        else if (pUserInfo->m_userType == CUserInfo::LOCALUSER)
        {
            // We can set this user's password
            FormatMessageString(IDS_USR_PWMESSAGE_FORMAT, szPWMessage, ARRAYSIZE(szPWMessage), pUserInfo->m_szUsername);
        }
        else
        {
            // Nothing can be done with this user's password
            // the selected user may be a domain user or a group or something
            // We can set this user's password
            FormatMessageString(IDS_USR_CANTCHANGEPW_FORMAT, szPWMessage, ARRAYSIZE(szPWMessage), pUserInfo->m_szUsername);
            EnableWindow(GetDlgItem(hwnd, IDC_PASSWORD_BUTTON), FALSE);
        }

        SetWindowText(GetDlgItem(hwnd, IDC_PASSWORD_STATIC), szPWMessage);

        // Set the icon for the user
        HICON hIcon = ImageList_GetIcon(m_himlLarge, pUserInfo->m_userType, ILD_NORMAL);
        Static_SetIcon(GetDlgItem(hwnd, IDC_CURRENTUSER_ICON), hIcon);
    }
    else
    {
        EnableControls(hwnd, rgidDisableOnNoSelection, ARRAYSIZE(rgidDisableOnNoSelection),
            FALSE);
    }

    // Ensure the password button wasn't enabled in ANY CASE when autologon is
    // enabled
    /*if (fAutologon)
    {
        EnableWindow(GetDlgItem(hwnd, IDC_PASSWORD_BUTTON), FALSE);
    }
*/

    TraceLeaveVoid();
}

HRESULT CUserlistPropertyPage::SetAutologonState(HWND hwnd, BOOL fAutologon)
{
    TraceEnter(TRACE_USR_CORE, "CUserlistPropertyPage::SetAutologonState");
    HRESULT hr = S_OK;

    PropSheet_Changed(GetParent(hwnd), hwnd);

    TraceLeaveResult(hr);
}

BOOL CUserlistPropertyPage::OnListViewItemChanged(HWND hwnd)
{
    TraceEnter(TRACE_USR_CORE, "CUserlistPropertyPage::OnListViewItemChanged");

    SetPageState(hwnd);

    TraceLeaveValue(TRUE);
}

long CUserlistPropertyPage::OnApply(HWND hwnd)
{
    TraceEnter(TRACE_USR_CORE, "CUserlistPropertyPage::OnApply");
    long applyEffect = PSNRET_NOERROR;

    // Use a big buffer to catch any error that might occur so we can report them
    // to the user
    static TCHAR szErrors[2048];

    szErrors[0] = TEXT('\0');

    BOOL fAutologonSet = (BST_UNCHECKED == SendMessage(GetDlgItem(hwnd, IDC_AUTOLOGON_CHECK), BM_GETCHECK, 0, 0));

    if (!fAutologonSet)
    {
        // Ensure autologon is cleared
        ClearAutoLogon();
    }
    else
    {
        // Autologon should be set - ask for credentials if this is a change...
        if (m_fAutologonCheckChanged)
        {
            CUserInfo* pSelectedUser = GetSelectedUserInfo(GetDlgItem(hwnd, IDC_USER_LIST));

            TCHAR szNullName[] = TEXT("");
            CAutologonUserDlg dlg((pSelectedUser != NULL) ? 
                pSelectedUser->m_szUsername : szNullName);

            if (dlg.DoModal(g_hInstance, MAKEINTRESOURCE(IDD_USR_AUTOLOGON_DLG), hwnd) == IDCANCEL)
            {
                applyEffect = PSNRET_INVALID_NOCHANGEPAGE;
            }
        }
    }

    m_fAutologonCheckChanged = FALSE;

    if (applyEffect == PSNRET_INVALID_NOCHANGEPAGE)
    {
        // Reload the data and list
        m_pData->Initialize(hwnd);
        SetupList(hwnd);
    }

    TraceLeaveValue(applyEffect);
}

void CUserlistPropertyPage::SetupList(HWND hwnd)
{
    TraceEnter(TRACE_USR_CORE, "CUserlistPropertyPage::SetupList");

    HWND hwndList = GetDlgItem(hwnd, IDC_USER_LIST);
    
    // Disable autologon check box in the domain case where autologon isn't
    // enabled
    HWND hwndCheck = GetDlgItem(hwnd, IDC_AUTOLOGON_CHECK);
    if (m_pData->IsComputerInDomain() && !m_pData->IsAutologonEnabled())
    {

        ShowWindow(hwndCheck, SW_HIDE);
        EnableWindow(hwndCheck, FALSE);

        // Move most controls up a bit if the autologon is not visible
        RECT rcBottom;
        GetWindowRect(GetDlgItem(hwnd, IDC_LISTTITLE_STATIC), &rcBottom);

        RECT rcTop;
        GetWindowRect(hwndCheck, &rcTop);

        int dy = rcTop.top - rcBottom.top;

        OffsetControls(hwnd, rgidMoveOnNoAutologonCheck, 
            ARRAYSIZE(rgidMoveOnNoAutologonCheck), 0, dy);

        // Grow the list by this amount also
        RECT rcList;
        GetWindowRect(hwndList, &rcList);

        SetWindowPos(hwndList, NULL, 0, 0, rcList.right - rcList.left, 
            rcList.bottom - rcList.top - dy, SWP_NOZORDER|SWP_NOMOVE);
    }

    SendMessage(hwndCheck, BM_SETCHECK, 
        m_pData->IsAutologonEnabled() ? BST_UNCHECKED : BST_CHECKED, 0);

    // Set the text in the set password group.
    SetPageState(hwnd);

    TraceLeaveVoid();
}

HRESULT CUserlistPropertyPage::LaunchSetPasswordDialog(HWND hwndParent)
{
    TraceEnter(TRACE_USR_CORE, "CUserlistPropertyPage::LaunchSetPasswordDialog");

    HRESULT hr = S_OK;

    CUserInfo* pUserInfo = GetSelectedUserInfo(GetDlgItem(hwndParent, IDC_USER_LIST));

    if ((pUserInfo != NULL) && (pUserInfo->m_userType == CUserInfo::LOCALUSER))
    {
        CChangePasswordDlg dlg(pUserInfo);

        dlg.DoModal(g_hInstance, MAKEINTRESOURCE(IDD_USR_SETPASSWORD_DLG), hwndParent);
    }
    else
    {
        // Unexpected: The Set Password button should be disabled if we don't have 
        // a valid selected user
        TraceMsg("LaunchSetPasswordDialog called with no user or wrong user type selected!");
        hr = E_FAIL;
    }
   
    TraceLeaveResult(hr);
}

#define MAX_EXTRA_USERPROP_PAGES    10
HPSXA CUserlistPropertyPage::AddExtraUserPropPages(ADDPROPSHEETDATA* ppsd, PSID psid)
{
    TraceEnter(TRACE_USR_CORE, "CUserlistPropertyPage::AddExtraUserPropPages");
    HPSXA hpsxa = NULL;

    CUserSidDataObject* pDataObj = new CUserSidDataObject();

    if (pDataObj != NULL)
    {
        HRESULT hr = pDataObj->SetSid(psid);

        if (SUCCEEDED(hr))
        {
            hpsxa = SHCreatePropSheetExtArrayEx(HKEY_LOCAL_MACHINE, REGSTR_USERPROPERTIES_SHEET, 
                MAX_EXTRA_USERPROP_PAGES, pDataObj);


            if (hpsxa != NULL)
            {
                UINT nPagesAdded = SHAddFromPropSheetExtArray(hpsxa, AddPropSheetPageCallback, (LPARAM) ppsd);

            }
        }

        pDataObj->Release();
    }

    TraceLeaveValue(hpsxa);
}



// ListCompare
//  Compares list items in for sorting the listview by column
//  lParamSort gets the 1-based column index. If lParamSort is negative
//  it indicates that the given column should be sorted in reverse.
int CUserlistPropertyPage::ListCompare(LPARAM lParam1, LPARAM lParam2, 
	LPARAM lParamSort)
{
    TraceEnter(TRACE_USR_CORE, "CUserlistPropertyPage::ListCompare");
    
    CUserInfo* pUserInfo1 = 
        (CUserInfo*) lParam1;

    CUserInfo* pUserInfo2 = 
        (CUserInfo*) lParam2;

    LPTSTR psz1;
    LPTSTR psz2;

    int iColumn = (int) lParamSort;
    BOOL fReverse;

    if (iColumn < 0)
    {
        fReverse = TRUE;
        iColumn = -iColumn;
    }
    else
    {
        fReverse = FALSE;
    }
    
    switch (iColumn)
    {
    case 1:
        // user name column
        psz1 = pUserInfo1->m_szUsername;
        psz2 = pUserInfo2->m_szUsername;
        break;
    case 2:
        // domain column
        psz1 = pUserInfo1->m_szDomain;
        psz2 = pUserInfo2->m_szDomain;
        break;
    case 3:
        psz1 = pUserInfo1->m_szGroups;
        psz2 = pUserInfo2->m_szGroups;
        break;
    }
    
    
    int iResult = lstrcmp(psz1, psz2);

    if (fReverse)
        iResult = -iResult;
    
    TraceLeaveValue(iResult);
}
