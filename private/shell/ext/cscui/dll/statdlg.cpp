//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       statdlg.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "statdlg.h"
#include "resource.h"
#include "folder.h"
#include "cscst.h"
#include "options.h"
#include "fopendlg.h"
#include "msgbox.h"

// Global used for IsDialogMessage processing
HWND g_hwndStatusDlg = NULL;

CStatusDlg::CStatusDlg(
    HINSTANCE hInstance,
    LPCTSTR pszText,
    eSysTrayState eState,
    Modes mode            // Optional.  Default is MODE_NORMAL
    ) : m_hInstance(hInstance),
        m_hwndDlg(NULL),
        m_hwndLV(NULL),
        m_himl(NULL),
        m_mode(mode),
        m_bExpanded(true),
        m_bSortAscending(true),
        m_iLastColSorted(-1),
        m_eSysTrayState(eState),
        m_cyExpanded(0),
        m_pszText(StrDup(pszText)) 
{ 

}

CStatusDlg::~CStatusDlg(
    void
    ) 
{ 
    delete[] m_pszText; 
}

int
CStatusDlg::Create(
    HWND hwndParent,
    LPCTSTR pszText,
    eSysTrayState eState,
    Modes mode
    )
{
    int iResult = 0;
    CStatusDlg *pdlg = new CStatusDlg(g_hInstance, pszText, eState, mode);
    if (pdlg)
    {
        iResult = pdlg->Run(hwndParent);
        if (!iResult)
            delete pdlg;
        // else pdlg is automatically deleted when the dialog is closed
    }
    return iResult;
}

//
// Run the status dialog as a modeless dialog.
// Activates an existing instance if one is available.
//
int
CStatusDlg::Run(
    HWND hwndParent
    )
{
    //
    // First activate an existing instance if one is already running.
    //
    int iResult = 0;
    TCHAR szDlgTitle[MAX_PATH];
    LoadString(m_hInstance, IDS_STATUSDLG_TITLE, szDlgTitle, ARRAYSIZE(szDlgTitle));

    m_hwndDlg = FindWindow(NULL, szDlgTitle);
    if (NULL != m_hwndDlg && IsWindow(m_hwndDlg) && IsWindowVisible(m_hwndDlg))
    {
        SetForegroundWindow(m_hwndDlg);
    }
    else
    {
        //
        // Otherwise create a new dialog.
        // We need to use CreateDialog rather than DialogBox because
        // sometimes the dialog is hidden.  DialogBox doesn't allow us to
        // change the visibility attributed defined in the dialog template.
        //
        m_hwndDlg = CreateDialogParam(m_hInstance,
                                      MAKEINTRESOURCE(IDD_CSCUI_STATUS),
                                      hwndParent,
                                      DlgProc,
                                      (LPARAM)this);
        if (NULL != m_hwndDlg)
        {
            MSG msg;

            ShowWindow(m_hwndDlg, MODE_NORMAL == m_mode ? SW_NORMAL : SW_HIDE);
            UpdateWindow(m_hwndDlg);

            // We don't need a message loop here. We're on systray's main
            // thread which has a message pump.

            iResult = 1;
        }
    }
    g_hwndStatusDlg = m_hwndDlg;
    return iResult;
}



void
CStatusDlg::Destroy(
    void
    )
{
    if (NULL != m_hwndDlg)
        DestroyWindow(m_hwndDlg);
    g_hwndStatusDlg = NULL;
}



INT_PTR CALLBACK 
CStatusDlg::DlgProc(
    HWND hwnd, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    CStatusDlg *pThis = (CStatusDlg *)GetWindowLongPtr(hwnd, DWLP_USER);

    BOOL bResult = FALSE;
    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            pThis = (CStatusDlg *)lParam;
            SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)pThis);
            pThis->m_hwndDlg = hwnd;
            bResult = pThis->OnInitDialog(wParam, lParam);
            break;
        }

        case WM_COMMAND:
            if (NULL != pThis)
                bResult = pThis->OnCommand(wParam, lParam);
            break;

        case WM_NOTIFY:
            bResult = pThis->OnNotify(wParam, lParam);
            break;

        case WM_DESTROY:
            pThis->OnDestroy();
            break;
    }
    return bResult;
}


//
// WM_INITDIALOG handler.
//
BOOL
CStatusDlg::OnInitDialog(
    WPARAM wParam,
    LPARAM lParam
    )
{
    TCHAR szScratch[MAX_PATH];
    BOOL bResult = TRUE;
    RECT rcExpanded;
    CConfig& config = CConfig::GetSingleton();

    m_hwndLV = GetDlgItem(m_hwndDlg, IDC_LV_STATUSDLG);
    //
    // Center the dialog on the desktop before contraction.
    //
    CenterWindow(m_hwndDlg, GetDesktopWindow());
    //
    // Start with the dialog not expanded.
    //
    GetWindowRect(m_hwndDlg, &rcExpanded);
    m_cyExpanded = rcExpanded.bottom - rcExpanded.top;
    //
    // Set the cached "expanded" member to be the opposite of the user's
    // preference for expansion.  ExpandDialog will only change the
    // expanded state if it's different from the current state.
    //
    m_bExpanded = !UserLikesDialogExpanded();
    ExpandDialog(!m_bExpanded);
    //
    // Disable buttons as necessary.
    //
    if (config.NoCacheViewer())
        EnableWindow(GetDlgItem(m_hwndDlg, IDC_BTN_VIEWFILES), FALSE);
    if (config.NoConfigCache())
        EnableWindow(GetDlgItem(m_hwndDlg, IDC_BTN_SETTINGS), FALSE);
    //
    // Initialize the message text.
    //
    SetWindowText(GetDlgItem(m_hwndDlg, IDC_TXT_STATUSDLG), m_pszText ? m_pszText : TEXT(""));
    //
    // Turn on checkboxes for column 0.
    //
    EnableListviewCheckboxes(true);
    //
    // Create the imagelist.
    //
    m_himl = CreateImageList();
    if (NULL != m_himl)
        ListView_SetImageList(m_hwndLV, m_himl, LVSIL_SMALL);
    //
    // Create the listview columns.
    //
    CreateListColumns();
    //
    // Fill the listview.
    //
    FillListView();

    if (MODE_AUTOSYNC == m_mode)
    {
        //
        // The dialog is being invoked for it's synchronize function only.
        // The dialog will not be displayed but we'll invoke the synchronize 
        // function just as if it had been displayed.  This feature is used 
        // by the systray context menu to ensure we get the same synchronize 
        // behavior if the action is invoked through either the dialog or 
        // the systray context menu.
        //
        PostMessage(m_hwndDlg, WM_COMMAND, IDOK, 0);
    }
    else
    {
        //
        // Since we're a child of the hidden systray window we need to force ourselves
        // to the forground.
        //
        SetForegroundWindow(m_hwndDlg);
    }

    return bResult;
}


//
// WM_DESTROY handler.
//
BOOL
CStatusDlg::OnDestroy(
    void
    )
{
    RememberUsersDialogSizePref(m_bExpanded);
    DestroyLVEntries();

    //
    // Destroy the CStatusDlg object
    //
    delete this;

    //
    // Image list is automatically destroyed by the listview in comctl32.
    //
    return FALSE;
}


//
// WM_COMMAND handler.
//
BOOL
CStatusDlg::OnCommand(
    WPARAM wParam,
    LPARAM lParam
    )
{
    BOOL bResult = TRUE;
    switch(LOWORD(wParam))
    {
        case IDOK:
            SynchronizeServers();
            // Fall through and destroy the dialog
        case IDCANCEL:
        case IDCLOSE:
            Destroy();
            break;
            
        case IDC_BTN_VIEWFILES:
            COfflineFilesFolder::Open();
            break;

        case IDC_BTN_SETTINGS:
            COfflineFilesSheet::CreateAndRun(g_hInstance, GetDesktopWindow(), &g_cRefCount);
            break;

        case IDC_BTN_DETAILS:
            ExpandDialog(!m_bExpanded);
            break;

        default:
            bResult = FALSE;
            break;  
    }
    return bResult;
}


//
// WM_NOTIFY handler.
//
BOOL
CStatusDlg::OnNotify(
    WPARAM wParam,
    LPARAM lParam
    )
{
    BOOL bResult = TRUE;
  //int idCtl    = int(wParam);
    LPNMHDR pnm  = (LPNMHDR)lParam;

    switch(pnm->code)
    {
        case LVN_GETDISPINFO:
            OnLVN_GetDispInfo((LV_DISPINFO *)lParam);
            break;

        case LVN_COLUMNCLICK:
            OnLVN_ColumnClick((NM_LISTVIEW *)lParam);
            break;

        default:
            bResult = FALSE;
            break;
    }
    return bResult;
}

//
// LVN_GETDISPINFO handler.
//
void
CStatusDlg::OnLVN_GetDispInfo(
    LV_DISPINFO *plvdi
    )
{
    LVEntry *pEntry = (LVEntry *)plvdi->item.lParam;
    if (LVIF_TEXT & plvdi->item.mask)
    {
        static TCHAR szText[MAX_PATH];
        szText[0] = TEXT('\0');
        switch(plvdi->item.iSubItem)
        {
            case iLVSUBITEM_SERVER:
                lstrcpyn(szText, pEntry->Server(), ARRAYSIZE(szText));
                break;
                
            case iLVSUBITEM_STATUS:
                pEntry->GetStatusText(szText, ARRAYSIZE(szText));
                break;

            case iLVSUBITEM_INFO:
                pEntry->GetInfoText(szText, ARRAYSIZE(szText));
                break;
        }
        plvdi->item.pszText = szText;
    }
    if (LVIF_IMAGE & plvdi->item.mask)
    {
        plvdi->item.iImage = pEntry->GetImageIndex();
    }
}

//
// LVN_COLUMNCLICK handler.
//
void
CStatusDlg::OnLVN_ColumnClick(
    NM_LISTVIEW *pnmlv
    )
{
    if (m_iLastColSorted != pnmlv->iSubItem)
    {
        m_bSortAscending = true;
        m_iLastColSorted = pnmlv->iSubItem;
    }
    else
    {
        m_bSortAscending = !m_bSortAscending;
    }

    ListView_SortItems(m_hwndLV, CompareLVItems, LPARAM(this));
}


//
// Create the server listview columns.
//
void
CStatusDlg::CreateListColumns(
    void
    )
{
    //
    // Clear out the listview and header.
    //
    ListView_DeleteAllItems(m_hwndLV);
    HWND hwndHeader = ListView_GetHeader(m_hwndLV);
    if (NULL != hwndHeader)
    {
        while(0 < Header_GetItemCount(hwndHeader))
            ListView_DeleteColumn(m_hwndLV, 0);
    }

    //
    // Create the header titles.
    //
    TCHAR szServer[80] = {0};
    TCHAR szStatus[80] = {0};
    TCHAR szInfo[80]   = {0};
    LoadString(m_hInstance, IDS_STATUSDLG_HDR_SERVER, szServer, ARRAYSIZE(szServer));
    LoadString(m_hInstance, IDS_STATUSDLG_HDR_STATUS, szStatus, ARRAYSIZE(szStatus));
    LoadString(m_hInstance, IDS_STATUSDLG_HDR_INFO,   szInfo,   ARRAYSIZE(szInfo));

    RECT rcList;
    GetClientRect(m_hwndLV, &rcList);
    int cxLV = rcList.right - rcList.left;

#define LVCOLMASK (LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM)

    LV_COLUMN rgCols[] = { 
         { LVCOLMASK, LVCFMT_LEFT, cxLV/4, szServer, 0, iLVSUBITEM_SERVER },
         { LVCOLMASK, LVCFMT_LEFT, cxLV/4, szStatus, 0, iLVSUBITEM_STATUS },
         { LVCOLMASK, LVCFMT_LEFT, cxLV/2, szInfo,   0, iLVSUBITEM_INFO   }
                         };
    //
    // Add the columns to the listview.
    //
    for (INT i = 0; i < ARRAYSIZE(rgCols); i++)
    {
        ListView_InsertColumn(m_hwndLV, i, &rgCols[i]);
    }
}



//
// Populate the listview.
//
void
CStatusDlg::FillListView(
    void
    )
{
    DWORD dwStatus;
    DWORD dwPinCount;
    DWORD dwHintFlags;
    FILETIME ft;
    WIN32_FIND_DATA fd;

    CCscFindHandle hFind = CacheFindFirst(NULL, &fd, &dwStatus, &dwPinCount, &dwHintFlags, &ft);
    if (hFind.IsValid())
    {
        LPTSTR pszServer = fd.cFileName;
        LPTSTR pszEnd;
        LVEntry *pEntry;
        CSCSHARESTATS stats;
        do
        {
            //
            // Exclude the following:
            //   1. Directories.
            //   2. Files marked as "locally deleted".
            //
            // NOTE:  The filtering done by this function must be the same as 
            //        in several other places throughout the CSCUI code.
            //        To locate these, search the source for the comment
            //        string CSCUI_ITEM_FILTER.
            //
            const DWORD fExclude = SSEF_LOCAL_DELETED | 
                                   SSEF_DIRECTORY;

            CSCGETSTATSINFO si = { fExclude,
                                   SSUF_NONE,
                                   false,      // No access info reqd (faster).
                                   false };     

            if (_GetShareStatisticsForUser(fd.cFileName, &si, &stats) && 
                (0 < stats.cTotal || stats.bOffline))
            {
                bool bReplacedBackslash = false;
                //
                // Extract the server name from the share name returned by CSC.
                //
                while(*pszServer && TEXT('\\') == *pszServer)
                    pszServer++;
                pszEnd = pszServer;
                while(*pszEnd && TEXT('\\') != *pszEnd)
                    pszEnd++;
                if (TEXT('\\') == *pszEnd)
                {
                    *pszEnd = TEXT('\0');
                    bReplacedBackslash = true;
                }
                //
                // Find an existing server entry.  If none found, create a new one.
                //
                if (NULL == (pEntry = FindLVEntry(pszServer)))
                {
                    bool bConnectable = boolify(SendToSystray(CSCWM_ISSERVERBACK, 0, (LPARAM)fd.cFileName));
                    pEntry = CreateLVEntry(pszServer, bConnectable);
                }
                if (NULL != pEntry)
                {
                    if (bReplacedBackslash)
                        *pszEnd = TEXT('\\');

                    //
                    // If we're running in "normal" mode, we 
                    // can't trust the share's "modified offline" bit.
                    // Use the info we got by scanning the cache.
                    // If we're running in "autosync" mode, we can just
                    // use the share's "modified offline" indicator.
                    // If something is truly modified offline, the bit
                    // will be set.
                    //
                    if (MODE_NORMAL == m_mode)
                    {
                        dwStatus &= ~FLAG_CSC_SHARE_STATUS_MODIFIED_OFFLINE;
                        if (0 < stats.cModified)
                            dwStatus |= FLAG_CSC_SHARE_STATUS_MODIFIED_OFFLINE;
                    }
                    //
                    // Add this share and it's statistics to the 
                    // server's list entry.
                    //
                    pEntry->AddShare(fd.cFileName, stats, dwStatus);
                }
            }
        }
        while(CacheFindNext(hFind, &fd, &dwStatus, &dwPinCount, &dwHintFlags, &ft));
        //
        // Remove those servers that the user won't be interested in.
        // Also place a checkmark next to those servers that are available
        // for reconnection.
        //
        PrepListForDisplay();
    }
}


//
// Build the image list used by the server listview.
//
HIMAGELIST
CStatusDlg::CreateImageList(
    void
    )
{
    HIMAGELIST himl = NULL;

    //
    // Note:  The order of these icon ID's in this array must match with the
    //        iIMAGELIST_ICON_XXXXX enumeration.
    //        The enum values represent the image indices in the image list.
    //
    static const struct IconDef
    {
        LPTSTR szName;
        HINSTANCE hInstance;

    } rgIcons[] = { 
                    { MAKEINTRESOURCE(IDI_SERVER),         m_hInstance },
                    { MAKEINTRESOURCE(IDI_SERVER_OFFLINE), m_hInstance },
                    { MAKEINTRESOURCE(IDI_CSCINFORMATION), m_hInstance },
                    { MAKEINTRESOURCE(IDI_CSCWARNING),     m_hInstance }
                  };
    //
    // Create the image lists for the listview.
    //
    int cxIcon = GetSystemMetrics(SM_CXSMICON);
    int cyIcon = GetSystemMetrics(SM_CYSMICON);

    himl = ImageList_Create(cxIcon,
                            cyIcon,
                            ILC_MASK, 
                            ARRAYSIZE(rgIcons), 
                            10);
    if (NULL != himl)
    {
        for (UINT i = 0; i < ARRAYSIZE(rgIcons); i++)
        {
            HICON hIcon = (HICON)LoadImage(rgIcons[i].hInstance, 
                                           rgIcons[i].szName,
                                           IMAGE_ICON,
                                           cxIcon,
                                           cyIcon,
                                           0);
            if (NULL != hIcon)
            {
                ImageList_AddIcon(himl, hIcon);
                DestroyIcon(hIcon);
            }
        }
        ImageList_SetBkColor(himl, CLR_NONE);  // Transparent background.
    }

    return himl;
}


void
CStatusDlg::EnableListviewCheckboxes(
    bool bEnable
    )
{    
    DWORD dwStyle    = ListView_GetExtendedListViewStyle(m_hwndLV);
    DWORD dwNewStyle = bEnable ? (dwStyle | LVS_EX_CHECKBOXES) :
                                 (dwStyle & ~LVS_EX_CHECKBOXES);
                                 
    ListView_SetExtendedListViewStyle(m_hwndLV, dwNewStyle);
}


//
// The "Details" button changes it's title depending on 
// the dialog state (expanded or not).
//
void
CStatusDlg::UpdateDetailsBtnTitle(
    void
    )
{
    TCHAR szBtnTitle[80];
    int idsBtnTitle = IDS_OPENDETAILS;
    if (m_bExpanded)
        idsBtnTitle = IDS_CLOSEDETAILS;

    LoadString(m_hInstance, idsBtnTitle, szBtnTitle, ARRAYSIZE(szBtnTitle));
    SetWindowText(GetDlgItem(m_hwndDlg, IDC_BTN_DETAILS), szBtnTitle);
}

//
// Expand or contract the dialog vertically.
// When expanded, the server listview is made visible along
// with the "Settings..." and "View Files..." buttons.
//
void 
CStatusDlg::ExpandDialog(
    bool bExpand
    )
{
    if (bExpand != m_bExpanded)
    {
        CConfig& config = CConfig::GetSingleton();
        //
        // Table describing enable/disable state of controls in the lower part
        // of the dialog that are displayed when the dialog is expanded.
        //
        struct
        {
            UINT idCtl;
            bool bEnable;

        } rgidExpanded[] = { { IDC_LV_STATUSDLG,  bExpand },
                             { IDC_BTN_SETTINGS,  bExpand && !config.NoConfigCache() },
                             { IDC_BTN_VIEWFILES, bExpand && !config.NoCacheViewer() }
                           };

        RECT rcDlg;
        GetWindowRect(m_hwndDlg, &rcDlg);
        if (!bExpand)
        {
            //
            // Closing details.
            //
            RECT rcSep;
            GetWindowRect(GetDlgItem(m_hwndDlg, IDC_SEP_STATUSDLG), &rcSep);
            rcDlg.bottom = rcSep.top;
        }
        else
        {
            //
            // Opening details.
            //
            rcDlg.bottom = rcDlg.top + m_cyExpanded;
        }

        //
        // If the dialog is not expanded, we want to disable all of the
        // "tabbable" items in the hidden part so they don't participate
        // in the dialog's tab order.  Note that the "Settings" and
        // "View Files" buttons also have a policy setting involved in
        // the enabling logic.
        //
        for (int i = 0; i < ARRAYSIZE(rgidExpanded); i++)
        {
            EnableWindow(GetDlgItem(m_hwndDlg, rgidExpanded[i].idCtl), rgidExpanded[i].bEnable);
        }

        SetWindowPos(m_hwndDlg,
                     NULL,
                     rcDlg.left,
                     rcDlg.top,
                     rcDlg.right - rcDlg.left,
                     rcDlg.bottom - rcDlg.top,
                     SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);

        m_bExpanded = bExpand;
        UpdateDetailsBtnTitle();
    }
}




//
// Queries the HKCU reg data to see if the user closed the dialog
// expanded or not expanded last time the dialog was used.
// Returns:  true = expanded, false = not expanded.
//
bool
CStatusDlg::UserLikesDialogExpanded(
    void
    )
{
    DWORD dwExpanded = 0;
    HKEY hkey;
    DWORD dwStatus = RegOpenKeyEx(HKEY_CURRENT_USER,
                                  REGSTR_KEY_OFFLINEFILES,
                                  0,
                                  KEY_QUERY_VALUE,
                                  &hkey);
    if (ERROR_SUCCESS == dwStatus)
    {
        DWORD dwType;
        DWORD cbData = sizeof(DWORD);
        RegQueryValueEx(hkey,
                        REGSTR_VAL_EXPANDSTATUSDLG,
                        NULL,
                        &dwType,
                        (LPBYTE)&dwExpanded,
                        &cbData);
        RegCloseKey(hkey);
    }
    return !!dwExpanded;
}


//
// Stores the current state of the status dialog in per-user
// reg data.  Used next time the dialog is opened so that if the
// user likes the dialog expanded, it opens expanded.
//
void
CStatusDlg::RememberUsersDialogSizePref(
    bool bExpanded
    )
{
    HKEY hkey;
    DWORD dwStatus = RegOpenKeyEx(HKEY_CURRENT_USER,
                                  REGSTR_KEY_OFFLINEFILES,
                                  0,
                                  KEY_SET_VALUE,
                                  &hkey);

    if (ERROR_SUCCESS == dwStatus)
    {
        DWORD cbData = sizeof(DWORD);
        DWORD dwExpanded = DWORD(bExpanded);
        RegSetValueEx(hkey,
                      REGSTR_VAL_EXPANDSTATUSDLG,
                      0,
                      REG_DWORD,
                      (CONST BYTE *)&dwExpanded,
                      cbData);

        RegCloseKey(hkey);
    }
}



//
// Build a list of share names for synchronization and
// reconnect.
//
HRESULT 
CStatusDlg::BuildFilenameList(
    CscFilenameList *pfnl
    )
{
    LVEntry *pEntry;
    LVITEM item;

    int cEntries = ListView_GetItemCount(m_hwndLV);
    for (int i = 0; i < cEntries; i++)
    {
        if (ListView_GetCheckState(m_hwndLV, i))
        {
            //
            // Server has checkmark so we add it's
            // shares to the filename list.
            //
            item.mask     = LVIF_PARAM;
            item.iItem    = i;
            item.iSubItem = 0;
            if (ListView_GetItem(m_hwndLV, &item))
            {
                pEntry = (LVEntry *)item.lParam;
                int cShares = pEntry->GetShareCount();
                for (int iShare = 0; iShare < cShares; iShare++)
                {
                    LPCTSTR pszShare = pEntry->GetShareName(iShare);
                    if (NULL != pszShare)
                    {
                        if (!pfnl->AddFile(pszShare, TRUE))
                            return E_OUTOFMEMORY;
                    }
                }
            }
        }
    }
    return NOERROR;
}


//
// Synchronize all of the checked servers from the listview and
// reconnect them.
//
HRESULT
CStatusDlg::SynchronizeServers(
    void
    )
{
    HRESULT hr = NOERROR;
    const bool bSkipTheSync = BST_CHECKED == IsDlgButtonChecked(m_hwndDlg, IDC_CBX_STATUSDLG);
    
    if (::IsSyncInProgress())
    {
        CscMessageBox(m_hwndDlg, 
                      MB_OK | MB_ICONINFORMATION, 
                      m_hInstance, 
                      bSkipTheSync ? IDS_CANTRECONN_SYNCINPROGRESS : IDS_CANTSYNC_SYNCINPROGRESS);
    }
    else
    {
        //
        // First build the FilenameList containing shares to sync.
        //
        CscFilenameList fnl;
        hr = BuildFilenameList(&fnl);
        if (SUCCEEDED(hr))
        {
            if (bSkipTheSync)
            {
                //
                // User has checked "reconnect without sync" checkbox.
                // Therefore, we skip the sync and go straight to reconnect.
                //
                hr = ReconnectServers(&fnl, TRUE, FALSE);
                if (S_OK == hr)
                {
                    PostToSystray(CSCWM_UPDATESTATUS, STWM_STATUSCHECK, 0);
                }

            }
            else
            {
                const DWORD dwUpdateFlags = CSC_UPDATE_STARTNOW |
                                            CSC_UPDATE_SELECTION |
                                            CSC_UPDATE_REINT |
                                            CSC_UPDATE_NOTIFY_DONE |
                                            CSC_UPDATE_SHOWUI_ALWAYS |
                                            CSC_UPDATE_RECONNECT;
                //
                // Syncing is an asynchronous operation involving
                // mobsync.exe.  The code in CscUpdateCache will check for open
                // files and notify the user.  When the sync is done, it will
                // transition everything in the file list to online mode.
                //
                hr = CscUpdateCache(dwUpdateFlags, &fnl);
            }
        }
    }
    return hr;
}


//
// Create an entry for the listview.
// Returns ptr to new entry on success.  NULL on failure.
//
CStatusDlg::LVEntry *
CStatusDlg::CreateLVEntry(
    LPCTSTR pszServer,
    bool bConnectable
    )
{
    LVEntry *pEntry = new LVEntry(m_hInstance, pszServer, bConnectable);
    if (NULL != pEntry)
    {
        LVITEM item;
        item.mask     = LVIF_PARAM | LVIF_TEXT | LVIF_IMAGE;
        item.lParam   = (LPARAM)pEntry;
        item.iItem    = ListView_GetItemCount(m_hwndLV);
        item.iSubItem = 0;
        item.pszText  = LPSTR_TEXTCALLBACK;
        item.iImage   = I_IMAGECALLBACK;
        if (-1 == ListView_InsertItem(m_hwndLV, &item))
        {
            delete pEntry;
            pEntry = NULL;
        }
    }
    return pEntry;
}

//
// Find an entry in the listview using the servername as the key.
// Return ptr to entry on success.  NULL on failure.
//
CStatusDlg::LVEntry *
CStatusDlg::FindLVEntry(
    LPCTSTR pszServer
    )
{
    LVEntry *pEntry = NULL;
    LVITEM item;

    int cEntries = ListView_GetItemCount(m_hwndLV);
    for (int i = 0; i < cEntries; i++)
    {
        item.mask     = LVIF_PARAM;
        item.iItem    = i;
        item.iSubItem = 0;
        if (ListView_GetItem(m_hwndLV, &item))
        {
            pEntry = (LVEntry *)item.lParam;
            //
            // This comparison must be case-INSENSITIVE.  Entries
            // in the CSC database are on a "\\server\share" basis and
            // are at the mercy of what was passed in through the CSC APIs.
            // Therefore, the database can contain "\\Foo\bar" and
            // "\\foo\bar2".  We must treat "Foo" and "foo" as the single
            // server they represent.
            //
            if (0 == lstrcmpi(pEntry->Server(), pszServer))
                break;
            pEntry = NULL;
        }
    }
    return pEntry;
}

//
// Clear out the listview.  Ensures all listview item objects
// are destroyed.
//
void
CStatusDlg::DestroyLVEntries(
    void
    )
{
    LVITEM item;

    int cEntries = ListView_GetItemCount(m_hwndLV);
    for (int i = 0; i < cEntries; i++)
    {
        item.mask     = LVIF_PARAM;
        item.iItem    = i;
        item.iSubItem = 0;
        if (ListView_GetItem(m_hwndLV, &item))
        {
            delete (LVEntry *)item.lParam;
            if (ListView_DeleteItem(m_hwndLV, i))
            {
                i--;
                cEntries--;
            }
        }
    }
}

//
// Determine if a listview entry should remain visible in the listview.
// Currently we include servers that currently connected through the 
// network redirector and are offline OR those that are dirty.
// 
bool
CStatusDlg::ShouldIncludeLVEntry(
    const CStatusDlg::LVEntry& entry
    )
{
    DWORD dwCscStatus;
    entry.GetStats(NULL, &dwCscStatus);

    return (FLAG_CSC_SHARE_STATUS_DISCONNECTED_OP & dwCscStatus) || 
           (FLAG_CSC_SHARE_STATUS_MODIFIED_OFFLINE & dwCscStatus);
}


//
// Determine if a checkmark should be placed next to an item in 
// the listview.
//
bool
CStatusDlg::ShouldCheckLVEntry(
    const CStatusDlg::LVEntry& entry
    )
{
    return true;
}


//
// Remove all entries not to be displayed from the listview.
// Initially we create LV entries for each server in the CSC cache.  
// After all servers have been entered and their statistics tallied,
// we call PrepListForDisplay to remove the ones that the
// user won't want to see.
// 
void
CStatusDlg::PrepListForDisplay(
    void
    )
{
    LVEntry *pEntry;
    LVITEM item;

    int cEntries = ListView_GetItemCount(m_hwndLV);
    for (int i = 0; i < cEntries; i++)
    {
        item.mask     = LVIF_PARAM;
        item.iItem    = i;
        item.iSubItem = 0;
        if (ListView_GetItem(m_hwndLV, &item))
        {
            pEntry = (LVEntry *)item.lParam;
            if (ShouldIncludeLVEntry(*pEntry))
            {
                ListView_SetCheckState(m_hwndLV, i, ShouldCheckLVEntry(*pEntry));
            }
            else
            {
                delete pEntry;
                if (ListView_DeleteItem(m_hwndLV, i))
                {
                    i--;
                    cEntries--;
                }
            }
        }
    }
}


//
// Listview item comparison callback.
//
int CALLBACK 
CStatusDlg::CompareLVItems(
    LPARAM lParam1, 
    LPARAM lParam2,
    LPARAM lParamSort
    )
{
    CStatusDlg *pdlg = reinterpret_cast<CStatusDlg *>(lParamSort);
    CStatusDlg::LVEntry *pEntry1 = reinterpret_cast<CStatusDlg::LVEntry *>(lParam1);
    CStatusDlg::LVEntry *pEntry2 = reinterpret_cast<CStatusDlg::LVEntry *>(lParam2);
    int diff = 0;
    TCHAR szText[2][MAX_PATH];

    //
    // This array controls the comparison column IDs used when
    // values for the selected column are equal.  These should
    // remain in order of the iLVSUBITEM_xxxxx enumeration with
    // respect to the first element in each row.
    //
    static const int rgColComp[3][3] = { 
        { iLVSUBITEM_SERVER, iLVSUBITEM_STATUS, iLVSUBITEM_INFO   },
        { iLVSUBITEM_STATUS, iLVSUBITEM_SERVER, iLVSUBITEM_INFO   },
        { iLVSUBITEM_INFO,   iLVSUBITEM_SERVER, iLVSUBITEM_STATUS }
                                       };
    int iCompare = 0;
    while(0 == diff && iCompare < ARRAYSIZE(rgColComp))
    {
        switch(rgColComp[pdlg->m_iLastColSorted][iCompare++])
        {
            case iLVSUBITEM_SERVER:
                lstrcpyn(szText[0], pEntry1->Server(), ARRAYSIZE(szText[0]));
                lstrcpyn(szText[1], pEntry2->Server(), ARRAYSIZE(szText[1]));
                break;

            case iLVSUBITEM_STATUS:
                pEntry1->GetStatusText(szText[0], ARRAYSIZE(szText[0]));
                pEntry2->GetStatusText(szText[1], ARRAYSIZE(szText[1]));
                break;

            case iLVSUBITEM_INFO:
                pEntry1->GetInfoText(szText[0], ARRAYSIZE(szText[0]));
                pEntry2->GetInfoText(szText[1], ARRAYSIZE(szText[1]));
                break;

            default:
                //
                // If you hit this, you need to update this function
                // to handle the new column you've added to the listview.
                //
                TraceAssert(false);
                break;
        }
        //
        // This comparison should be case-sensitive since it is controlling
        // sort order of display columns.
        //
        diff = lstrcmp(szText[0], szText[1]);
    }

    return pdlg->m_bSortAscending ? diff : -1 * diff;
}


//
// Your basic alloc-and-copy-a-string function.
//
LPTSTR 
CStatusDlg::StrDup(
    LPCTSTR psz
    )
{
    LPTSTR pszNew = new TCHAR[lstrlen(psz) + 1];
    if (NULL != pszNew)
        lstrcpy(pszNew, psz);
    return pszNew;
}


const TCHAR CStatusDlg::LVEntry::s_szBlank[] = TEXT("");
//
// There are 3 binary conditions that control the selection of the entry 
// display information.  Therefore we can use a simple 8-element map of string resource
// IDs and icon image list indices to determine the correct display information
// string for the corresponding LV entry state.  GetDispInfoIndex() is called
// to retrieve the index for a particular LVEntry.
//
const CStatusDlg::LVEntry::DispInfo 
CStatusDlg::LVEntry::s_rgDispInfo[] = {                                                                   // online available modified
    { IDS_SHARE_STATUS_OFFLINE, IDS_SHARE_INFO_UNAVAIL,     CStatusDlg::iIMAGELIST_ICON_SERVER_OFFLINE }, //    0       0         0      
    { IDS_SHARE_STATUS_OFFLINE, IDS_SHARE_INFO_UNAVAIL_MOD, CStatusDlg::iIMAGELIST_ICON_SERVER_OFFLINE }, //    0       0         1
    { IDS_SHARE_STATUS_OFFLINE, IDS_SHARE_INFO_AVAIL,       CStatusDlg::iIMAGELIST_ICON_SERVER_BACK    }, //    0       1         0
    { IDS_SHARE_STATUS_OFFLINE, IDS_SHARE_INFO_AVAIL_MOD,   CStatusDlg::iIMAGELIST_ICON_SERVER_BACK    }, //    0       1         1 
    { IDS_SHARE_STATUS_ONLINE,  IDS_SHARE_INFO_BLANK,       CStatusDlg::iIMAGELIST_ICON_SERVER         }, //    1       0         0
    { IDS_SHARE_STATUS_ONLINE,  IDS_SHARE_INFO_DIRTY,       CStatusDlg::iIMAGELIST_ICON_SERVER_DIRTY   }, //    1       0         1
    { IDS_SHARE_STATUS_ONLINE,  IDS_SHARE_INFO_BLANK,       CStatusDlg::iIMAGELIST_ICON_SERVER         }, //    1       1         0
    { IDS_SHARE_STATUS_ONLINE,  IDS_SHARE_INFO_DIRTY,       CStatusDlg::iIMAGELIST_ICON_SERVER_DIRTY   }  //    1       1         1
    };

CStatusDlg::LVEntry::LVEntry(
    HINSTANCE hInstance,
    LPCTSTR pszServer,
    bool bConnectable
    ) : m_hInstance(hInstance),
        m_pszServer(CStatusDlg::StrDup(pszServer)),
        m_dwCscStatus(0),
        m_hdpaShares(DPA_Create(4)),
        m_bConnectable(bConnectable),
        m_iDispInfo(-1)
{
    m_stats.cTotal    = 0;
    m_stats.cPinned   = 0;
    m_stats.cModified = 0;
    m_stats.cSparse   = 0;

    if (NULL == m_pszServer)
        m_pszServer = const_cast<LPTSTR>(s_szBlank);
}


CStatusDlg::LVEntry::~LVEntry(
    void
    )
{
    if (s_szBlank != m_pszServer)
        delete[] m_pszServer;

    if (NULL != m_hdpaShares)
    {
        int cShares = DPA_GetPtrCount(m_hdpaShares);
        for (int i = 0; i < cShares; i++)
        {
            delete[] DPA_GetPtr(m_hdpaShares, i);
        }
        DPA_Destroy(m_hdpaShares);
    }
}


bool
CStatusDlg::LVEntry::AddShare(
    LPCTSTR pszShare,
    const CSCSHARESTATS& s,
    DWORD dwCscStatus
    )
{
    bool bResult = false;
    if (NULL != m_hdpaShares)
    {
        LPTSTR pszCopy = CStatusDlg::StrDup(pszShare);
        if (NULL != pszCopy)
        {
            if (-1 != DPA_AppendPtr(m_hdpaShares, pszCopy))
            {
                m_stats.cTotal    += s.cTotal;
                m_stats.cPinned   += s.cPinned;
                m_stats.cModified += s.cModified;
                m_stats.cSparse   += s.cSparse;

                m_dwCscStatus |= dwCscStatus;

                bResult = true;
            }
            else
            {
                delete[] pszCopy;
            }
        }
    }
    return bResult;
}


int 
CStatusDlg::LVEntry::GetShareCount(
    void
    ) const
{
    if (NULL != m_hdpaShares)
        return DPA_GetPtrCount(m_hdpaShares);
    return 0;
}


LPCTSTR 
CStatusDlg::LVEntry::GetShareName(
    int iShare
    ) const
{
    if (NULL != m_hdpaShares)
        return (LPCTSTR)DPA_GetPtr(m_hdpaShares, iShare);
    return NULL;
}


void 
CStatusDlg::LVEntry::GetStats(
    CSCSHARESTATS *ps,
    DWORD *pdwCscStatus
    ) const
{
    if (NULL != ps)
        *ps = m_stats;
    if (NULL != pdwCscStatus)
        *pdwCscStatus = m_dwCscStatus;
}

//
// Determines the index into s_rgDispInfo[] for obtaining display information
// for the LV entry.
//
int
CStatusDlg::LVEntry::GetDispInfoIndex(
    void
    ) const
{
    if (-1 == m_iDispInfo)
    {
        m_iDispInfo = 0;

        if (IsModified())
            m_iDispInfo |= DIF_MODIFIED;

        if (!IsOffline())
            m_iDispInfo |= DIF_ONLINE;

        if (IsConnectable())
            m_iDispInfo |= DIF_AVAILABLE;
    }
    return m_iDispInfo;
}


//
// Retrieve the entry's text for the "Status" column in the listview.
//
void 
CStatusDlg::LVEntry::GetStatusText(
    LPTSTR pszStatus, 
    int cchStatus
    ) const
{
    UINT idsStatusText = s_rgDispInfo[GetDispInfoIndex()].idsStatusText;
    LoadString(m_hInstance, idsStatusText, pszStatus, cchStatus);
}


//
// Retrieve entry's text for the "Information" column in the listview.
//
void 
CStatusDlg::LVEntry::GetInfoText(
    LPTSTR pszInfo, 
    int cchInfo
    ) const
{
    int iInfoText = GetDispInfoIndex();
    int idsInfoText = s_rgDispInfo[iInfoText].idsInfoText;
    if (iInfoText & DIF_MODIFIED)
    {
        //
        // Info text has embedded modified file count.
        // Requires a little more work.
        //
        TCHAR szTemp[MAX_PATH];
        int rgcModified[] = { m_stats.cModified };
        LoadString(m_hInstance, idsInfoText, szTemp, ARRAYSIZE(szTemp));
        FormatMessage(FORMAT_MESSAGE_FROM_STRING |
                      FORMAT_MESSAGE_ARGUMENT_ARRAY,
                      szTemp,
                      0,
                      0,
                      pszInfo,
                      cchInfo,
                      (va_list *)rgcModified);
    }
    else
    {
        LoadString(m_hInstance, idsInfoText, pszInfo, cchInfo);
    }
}


//
// Retrieve entry's imagelist index for the image displayed next to the
// entry in the listview.
//
int
CStatusDlg::LVEntry::GetImageIndex(
    void
    ) const
{
    return s_rgDispInfo[GetDispInfoIndex()].iImage;
}


//
// Wrapper for CSCTransitionServerOnline
//
//
// CAUTION!
//
// TransitionShareOnline is called from ReconnectServers (below) which
// means we may be running in either explorer or mobsync.  It's also
// called directly from the systray code when auto-reconnecting a
// server.
//
// 1. Be careful with SendMessage, since it may go out of process. Note that
// STDBGOUT uses SendMessage.
// 2. Be careful not to do anything that could cause a transition to offline,
// which results in a SendMessage from WinLogon, which deadlocks if we are
// running on the systray thread (the recipient of the SendMessage).
// For example, use SHSimpleIDListFromFindData instead of ILCreateFromPath.
//
BOOL
TransitionShareOnline(LPCTSTR pszShare,
                      BOOL  bShareIsAlive,  // TRUE skips CheckShareOnline
                      BOOL  bCheckSpeed,    // FALSE skips slow link check
                      DWORD dwPathSpeed)    // Used if (bShareIsAlive && bCheckSpeed)
{
    BOOL bShareTransitioned = FALSE;
    DWORD dwShareStatus;

    if (!pszShare || !*pszShare)
        return FALSE;

    //
    // Protect against calling CSCCheckShareOnline & CSCTransitionServerOnline
    // for shares that are already online. In particular, CSCCheckShareOnline
    // establishes a net connection so it can be slow.
    //
    // This also means that we call CSCTransitionServerOnline for only one
    // share on a given server, since all shares transition online/offline
    // at the same time.
    //
    if (CSCQueryFileStatus(pszShare, &dwShareStatus, NULL, NULL)
        && (FLAG_CSC_SHARE_STATUS_DISCONNECTED_OP & dwShareStatus))
    {
        //
        // Only try to transition the server online if the share is
        // truly available on the net.  Otherwise we'll put the server online
        // and the next call for net resources on that server will cause a net
        // timeout.
        //
        if (!bShareIsAlive)
        {
            bShareIsAlive = CSCCheckShareOnlineEx(pszShare, &dwPathSpeed);
            if (!bShareIsAlive)
            {
                DWORD dwErr = GetLastError();
                if (ERROR_ACCESS_DENIED == dwErr ||
                    ERROR_LOGON_FAILURE == dwErr)
                {
                    // The share is reachable, but we don't have valid
                    // credentials.  We don't have a valid path speed,
                    // so the best we can do is assume fast link.
                    //
                    // Currently, this function is always called with
                    // bShareIsAlive and bCheckSpeed both TRUE or both FALSE
                    // so it doesn't make any difference.  Could change in
                    // the future, though.
                    //
                    bShareIsAlive = TRUE;
                    bCheckSpeed = FALSE;
                }
            }
        }
        if (bShareIsAlive)
        {
            //
            // Also, for auto-reconnection, we only transition if not on a
            // slow link.
            //
            if (!bCheckSpeed || !_PathIsSlow(dwPathSpeed))
            {
                //
                // Transition to online
                //
                STDBGOUT((2, TEXT("Transitioning server \"%s\" to online"), pszShare));
                if (CSCTransitionServerOnline(pszShare))
                {
                    STDBGOUT((1, TEXT("Server \"%s\" reconnected."), pszShare));
                    LPTSTR pszTemp;
                    if (LocalAllocString(&pszTemp, pszShare))
                    {
	                //
                        // Strip the path to only the "\\server" part.
                        //
                        PathStripToRoot(pszTemp);
                        PathRemoveFileSpec(pszTemp);
                        SendCopyDataToSystrayAsync(PWM_REFRESH_SHELL, StringByteSize(pszTemp), pszTemp);
                        LocalFreeString(&pszTemp);
                    }
                    bShareTransitioned = TRUE;
                }
                else
                {
                    STDBGOUT((1, TEXT("Error %d reconnecting \"%s\""), GetLastError(), pszShare));
                }
            }
            else
            {
                STDBGOUT((1, TEXT("Path to \"%s\" is SLOW.  No reconnect."), pszShare));
            }
        }
        else
        {
            STDBGOUT((1, TEXT("Unable to connect to \"%s\".  No reconnect."), pszShare));
        }
    }

    return bShareTransitioned;
}


//
// CAUTION!
//
// ReconnectServers is called from both the Status Dialog (explorer) and
// the sync update handler (mobsync.exe). Any communication with systray
// must be done in a process-safe manner.
//
// See comments for TransitionShareOnline above.
//
HRESULT
ReconnectServers(CscFilenameList *pfnl,
                 BOOL bCheckForOpenFiles,
                 BOOL bCheckSpeed)          // FALSE skips slow link check
{
    if (pfnl)
    {
        CscFilenameList::ShareIter si = pfnl->CreateShareIterator();
        CscFilenameList::HSHARE hShare;
        BOOL bRefreshShell = FALSE;

        if (bCheckForOpenFiles)
        {
            //
            // First scan the shares to see if any have open files.
            //
            while(si.Next(&hShare))
            {
                DWORD dwShareStatus;
                if (CSCQueryFileStatus(pfnl->GetShareName(hShare), &dwShareStatus, NULL, NULL))
                {
                    if ((FLAG_CSC_SHARE_STATUS_FILES_OPEN & dwShareStatus) &&
                        (FLAG_CSC_SHARE_STATUS_DISCONNECTED_OP & dwShareStatus))
                    {
                        if (IDOK != OpenFilesWarningDialog())
                        {
                            return S_FALSE; // User cancelled.
                        }

                        break;
                    }
                }
            }
            si.Reset();
        }

        //
        // Walk through the list, transitioning everything to online.
        //
        while(si.Next(&hShare))
        {
            if (TransitionShareOnline(pfnl->GetShareName(hShare), FALSE, bCheckSpeed, 0))
                bRefreshShell = TRUE;
        }
    }

    return S_OK;
}
