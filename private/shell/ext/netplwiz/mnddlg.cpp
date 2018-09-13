#include "stdafx.h"

#include "resource.h"
#include "misc.h"
#include "password.h"

#include "mnddlg.h"

#define SHARE_NAME_PIXELS   30      // x-position of share name in the combo box
//
// Drive-related Constants
//
#define DRIVE_NAME_STRING   L" :"
#define DRIVE_NAME_LENGTH   ((sizeof(DRIVE_NAME_STRING) - 1) / sizeof(TCHAR))

#define SELECT_DRIVE        L'C'        // Highlight (select) the first
                                        // available drive after this one
#define FIRST_DRIVE         L'A'
#define LAST_DRIVE          L'Z'
#define SHARE_NAME_INDEX    5   // Index of the share name in the drive string

#define SELECT_DONE         0x00000001  // The highlight has been set
#define PAST_SELECT_DRIVE   0x00000002  // SELECT_DRIVE has been passed

//
// MPR Registry Constants
//
#define MPR_HIVE            HKEY_CURRENT_USER
#define MPR_KEY             L"Software\\Microsoft\\Windows NT\\CurrentVersion" \
                             L"\\Network\\Persistent Connections"
#define MPR_VALUE           L"SaveConnections"
#define MPR_YES             L"yes"
#define MPR_NO              L"no"

const DWORD CMapNetDriveMRU::c_nMaxMRUItems = 26; // 26 is the same as the run dialog
const TCHAR CMapNetDriveMRU::c_szMRUSubkey[] = TEXT("software\\microsoft\\windows\\currentversion\\explorer\\Map Network Drive MRU");

CMapNetDriveMRU::CMapNetDriveMRU() : m_hMRU(NULL)
{
    TraceEnter(TRACE_MND_CORE, "CMapNetDriveMRU::CMapNetDriveMRU");

    MRUINFO mruinfo;
    mruinfo.cbSize = sizeof (MRUINFO);
    mruinfo.uMax = c_nMaxMRUItems;
    mruinfo.fFlags = 0;
    mruinfo.hKey = HKEY_CURRENT_USER;
    mruinfo.lpszSubKey = c_szMRUSubkey;
    mruinfo.lpfnCompare = CompareProc;
        
    m_hMRU = CreateMRUList(&mruinfo);

    TraceLeaveVoid();
}

BOOL CMapNetDriveMRU::FillCombo(HWND hwndCombo)
{
    TraceEnter(TRACE_MND_CORE, "CMapNetDriveMRU::FillCombo");

    BOOL fSuccess = FALSE;

    if (NULL != m_hMRU)
    {
        ComboBox_ResetContent(hwndCombo);

        int nItem = 0;
        TCHAR szMRUItem[MAX_PATH + 1];

        while (TRUE)
        {
            int nResult = EnumMRUList(m_hMRU, nItem, (LPVOID) szMRUItem, ARRAYSIZE(szMRUItem));
            if (-1 != nResult)
            {
                // Add the string
                ComboBox_AddString(hwndCombo, szMRUItem);
                nItem ++;
            }
            else
            {
                // No more items; break
                break;
            }
        }

        fSuccess = TRUE;
    }

    TraceLeaveValue(fSuccess);
}

BOOL CMapNetDriveMRU::AddString(LPCTSTR psz)
{
    TraceEnter(TRACE_MND_CORE, "CMapNetDriveMRU::AddString");

    BOOL fSuccess = FALSE;

    if (NULL != m_hMRU)
    {
        int nAddResult = AddMRUString(m_hMRU, psz);
        if (-1 != nAddResult)
        {
            fSuccess = TRUE;
        }
    }

    TraceLeaveValue(fSuccess);
}

CMapNetDriveMRU::~CMapNetDriveMRU()
{
    TraceEnter(TRACE_MND_CORE, "CMapNetDriveMRU::~CMapNetDriveMRU");

    if (NULL != m_hMRU)
    {
        FreeMRUList(m_hMRU);
    }

    TraceLeaveVoid();
}

int CMapNetDriveMRU::CompareProc(LPCTSTR lpsz1, LPCTSTR lpsz2)
{
    TraceEnter(TRACE_MND_CORE, "CMapNetDriveMRU::CompareProc");
    TraceLeaveValue(StrCmpI(lpsz1, lpsz2));
}


void CMapNetDrivePage::EnableReconnect(HWND hwnd)
{
    TraceEnter(TRACE_MND_CORE, "CMapNetDrivePage::EnableReconnect");

    BOOL fEnable = !(m_pConnectStruct->dwFlags & CONNDLG_HIDE_BOX);

    EnableWindow(GetDlgItem(hwnd, IDC_RECONNECT), fEnable);

    TraceLeaveVoid();
}

BOOL CMapNetDrivePage::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    TraceEnter(TRACE_MND_CORE, "CMapNetDrivePage::OnInitDialog");

    // Check or uncheck the "reconnect at logon" box (registry)
    Button_SetCheck(GetDlgItem(hwnd, IDC_RECONNECT), ReadReconnect() ? BST_CHECKED : BST_UNCHECKED);

    EnableReconnect(hwnd);

    ComboBox_LimitText(GetDlgItem(hwnd, IDC_FOLDER), MAX_PATH);

    // Set up the drive drop-list
    FillDriveBox(hwnd);

    // Set focus to default control
    TraceLeaveValue(FALSE);
}

BOOL CMapNetDrivePage::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    TraceEnter(TRACE_MND_CORE, "CMapNetDrivePage::OnCommand");
    BOOL fHandled = FALSE;

    switch (id)
    {
    case IDC_FOLDERBROWSE:
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
                HWND hwndFolderEdit = GetDlgItem(hwnd, IDC_FOLDER);
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
                        PropSheet_SetWizButtons(GetParent(hwnd), fEnableFinish ? PSWIZB_FINISH : PSWIZB_DISABLEDFINISH);
                    }
                    
                    ILFree(pidlReturned);
                }

                ILFree(pidl);
            }

            fHandled = TRUE;
        }
        break;

    case IDC_DRIVELETTER:
    {
        if ( CBN_SELCHANGE == codeNotify )
        {
            HWND hwndCombo = GetDlgItem(hwnd, IDC_DRIVELETTER);
            int iItem = ComboBox_GetCurSel(hwndCombo);
            BOOL fNone = (BOOL)ComboBox_GetItemData(hwndCombo, iItem);
            EnableWindow(GetDlgItem(hwnd, IDC_RECONNECT), !fNone);        
        }
        break;
    }


    case IDC_FOLDER:
        // Is this an change notification
        if ((CBN_EDITUPDATE == codeNotify) || (CBN_SELCHANGE == codeNotify))
        {
            // Enable Finish only if something is typed into the folder box
            BOOL fEnableFinish = (CBN_SELCHANGE == codeNotify) ||
                (0 != GetWindowTextLength(hwndCtl));
            
            PropSheet_SetWizButtons(GetParent(hwnd), fEnableFinish ? PSWIZB_FINISH : PSWIZB_DISABLEDFINISH);

            fHandled = TRUE;
        }
        break;

    default:
        break;
    }

    TraceLeaveValue(fHandled);
}

BOOL CMapNetDrivePage::OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh)
{
    TraceEnter(TRACE_MND_CORE, "CMapNetDrivePage::OnNotify");

    BOOL fHandled = FALSE;

    switch (pnmh->code)
    {
    case PSN_SETACTIVE:
        {
            m_MRU.FillCombo(GetDlgItem(hwnd, IDC_FOLDER));

            TCHAR szPath[MAX_PATH + 1];

            // A path may have been specified. If so, use it
            if ((m_pConnectStruct->lpConnRes != NULL) && (m_pConnectStruct->lpConnRes->lpRemoteName != NULL))
            {
                // Copy over the string into our private buffer 
                lstrcpyn(szPath, m_pConnectStruct->lpConnRes->lpRemoteName, ARRAYSIZE(szPath));
        
                if (m_pConnectStruct->dwFlags & CONNDLG_RO_PATH)
                {
                    // this shit's read only
                    EnableWindow(GetDlgItem(hwnd, IDC_FOLDER), FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDC_FOLDERBROWSE), FALSE);
                }
            }
            else
            {
                szPath[0] = TEXT('\0');
            }

            // Set the path
            SetWindowText(GetDlgItem(hwnd, IDC_FOLDER), szPath);

            // Enable Finish only if something is typed into the folder box
            BOOL fEnableFinish = (0 != GetWindowTextLength(GetDlgItem(hwnd, IDC_FOLDER)));
            PropSheet_SetWizButtons(GetParent(hwnd), fEnableFinish ? PSWIZB_FINISH : PSWIZB_DISABLEDFINISH);

            fHandled = TRUE;
        }
        break;

    case PSN_QUERYINITIALFOCUS:
        {
            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG_PTR) GetDlgItem(hwnd, IDC_FOLDER));
            fHandled = TRUE;
        }
        break;
    case PSN_WIZFINISH:
        if (MapDrive(hwnd))
        {
            WriteReconnect(BST_CHECKED == Button_GetCheck(GetDlgItem(hwnd, IDC_RECONNECT)));
            // Allow wizard to exit
            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG_PTR) FALSE);
        }
        else
        {
            // Force wizard to stick around
            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG_PTR) GetDlgItem(hwnd, IDC_FOLDER));
        }
        fHandled = TRUE;
        break;
    case PSN_QUERYCANCEL:
        // Allow cancel
        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, FALSE);
        fHandled = TRUE;
        *m_pdwLastError = 0xFFFFFFFF;
        break;

    case NM_CLICK:
    case NM_RETURN:
        switch (idCtrl)
        {
        case IDC_CONNECTASLINK:
            {
                CConnectAsDlg dlg(m_szDomainUser, ARRAYSIZE(m_szDomainUser), m_szPassword,
                    ARRAYSIZE(m_szPassword));

                dlg.DoModal(g_hInstance, MAKEINTRESOURCE(IDD_MND_CONNECTAS), hwnd);
                fHandled = TRUE;
            }
            break;
        case IDC_ADDPLACELINK:
            {
                // Launch the ANP wizard
                STARTUPINFO startupinfo = {0};
                startupinfo.cb = sizeof (startupinfo);

                WCHAR szCommandLine[] = L"rundll32.exe netplwiz.dll,AddNetPlaceRunDll";

                PROCESS_INFORMATION process_information;
                if (CreateProcess(NULL, szCommandLine, NULL, NULL, 0, NULL, 
                    NULL, NULL, &startupinfo, &process_information))
                {
                    CloseHandle(process_information.hProcess);
                    CloseHandle(process_information.hThread);

                    PropSheet_PressButton(GetParent(hwnd), PSBTN_CANCEL);
                }
                else
                {
                    DisplayFormatMessage(hwnd, IDS_CONNECT_DRIVE_CAPTION, IDS_MND_ADDPLACEERR, 
                        MB_ICONERROR | MB_OK);
                }
                fHandled = TRUE;
            }
            break;
        }
        break;
    }

    TraceLeaveValue(fHandled);    
}

BOOL CMapNetDrivePage::OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT * lpDrawItem)
{
    TraceAssert(lpDrawItem);

    // 
    // If there are no listbox items, skip this message.
    //
    if (lpDrawItem->itemID == -1)
    {
        return TRUE;
    }
    
    //
    // Draw the text for the listbox items
    //
    switch (lpDrawItem->itemAction)
    {    
        case ODA_SELECT: 
        case ODA_DRAWENTIRE:
        {              
            TCHAR       tszDriveName[MAX_PATH + DRIVE_NAME_LENGTH + SHARE_NAME_INDEX];
            LPTSTR      lpShare;
            TEXTMETRIC  tm;
            COLORREF    clrForeground;
            COLORREF    clrBackground;
            DWORD       dwError;
            DWORD       dwExStyle = 0L;
            UINT        fuETOOptions = ETO_CLIPPED;
            ZeroMemory(tszDriveName, sizeof(tszDriveName));

            //
            // Get the text string associated with the given listbox item
            //
            dwError = ComboBox_GetLBText(lpDrawItem->hwndItem, 
                                         lpDrawItem->itemID, 
                                         tszDriveName);

            TraceAssert(dwError != CB_ERR);
            TraceAssert(_tcslen(tszDriveName));

            //
            // Check to see if the drive name string has a share name at
            // index SHARE_NAME_INDEX.  If so, set lpShare to this location
            // and NUL-terminate the drive name.
            //

            // Check for special (none) item and don't screw with the string in this case
            BOOL fNone = (BOOL) ComboBox_GetItemData(lpDrawItem->hwndItem, lpDrawItem->itemID);

            if ((*(tszDriveName + DRIVE_NAME_LENGTH) == L'\0') || fNone)
            {
                lpShare = NULL;
            }
            else
            {
                lpShare = tszDriveName + SHARE_NAME_INDEX;
                *(tszDriveName + DRIVE_NAME_LENGTH) = L'\0';
            }

            dwError = GetTextMetrics(lpDrawItem->hDC, &tm);
            TraceAssert(dwError);

            clrForeground = SetTextColor(lpDrawItem->hDC,
                                         GetSysColor(lpDrawItem->itemState & ODS_SELECTED ? COLOR_HIGHLIGHTTEXT : 
                                                                                       COLOR_WINDOWTEXT));
            clrBackground = SetBkColor(lpDrawItem->hDC, 
                                       GetSysColor(lpDrawItem->itemState & ODS_SELECTED ? COLOR_HIGHLIGHT : 
                                                                                     COLOR_WINDOW));
            dwExStyle = GetWindowLong(lpDrawItem->hwndItem, GWL_EXSTYLE);
            if(dwExStyle & WS_EX_RTLREADING)
            {
               fuETOOptions |= ETO_RTLREADING; 
            }
            //
            // Draw the text into the listbox
            //
            ExtTextOut(lpDrawItem->hDC,
                       LOWORD(GetDialogBaseUnits()) / 2,
                       (lpDrawItem->rcItem.bottom + lpDrawItem->rcItem.top - tm.tmHeight) / 2,
                       fuETOOptions | ETO_OPAQUE,
                       &lpDrawItem->rcItem,
                       tszDriveName,
                       _tcslen(tszDriveName),
                       NULL);

            //
            // If there's a share name, draw it in a second column
            // at (x = SHARE_NAME_PIXELS)
            //
            if (lpShare != NULL)
            {
                ExtTextOut(lpDrawItem->hDC,
                           SHARE_NAME_PIXELS,
                           (lpDrawItem->rcItem.bottom + lpDrawItem->rcItem.top - tm.tmHeight) / 2,
                           fuETOOptions,
                           &lpDrawItem->rcItem,
                           lpShare,
                           _tcslen(lpShare),
                           NULL);

                //
                // Restore the original string
                //
                *(tszDriveName + _tcslen(DRIVE_NAME_STRING)) = L' ';
            }

            //
            // Restore the original text and background colors
            //
            SetTextColor(lpDrawItem->hDC, clrForeground); 
            SetBkColor(lpDrawItem->hDC, clrBackground);

            //
            // If the item is selected, draw the focus rectangle
            //
            if (lpDrawItem->itemState & ODS_SELECTED)
            {
                DrawFocusRect(lpDrawItem->hDC, &lpDrawItem->rcItem); 
            }                     
            break;
        }
    }             
    return TRUE;
}

INT_PTR CMapNetDrivePage::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_NOTIFY, OnNotify);
        HANDLE_MSG(hwnd, WM_DRAWITEM, OnDrawItem);
    }

    return FALSE;
}



// "Reconnect check" registry setting
BOOL CMapNetDrivePage::ReadReconnect()
{
    TraceEnter(TRACE_MND_CORE, "CMapNetDrivePage::ReadReconnect");
    
    BOOL fReconnect = TRUE;

    if (m_pConnectStruct->dwFlags & CONNDLG_PERSIST)
    {
        fReconnect = TRUE;
    }
    else if (m_pConnectStruct->dwFlags & CONNDLG_NOT_PERSIST)
    {
        fReconnect = FALSE;
    }
    else
    {
        HKEY hkeyMPR;

        // User didn't specify -- check the registry.
        if (ERROR_SUCCESS == RegOpenKeyEx(MPR_HIVE, MPR_KEY, 0, KEY_READ, &hkeyMPR))
        {
            DWORD dwType;
            TCHAR szAnswer[ARRAYSIZE(MPR_YES) + ARRAYSIZE(MPR_NO)];
            DWORD cbSize = sizeof(szAnswer);

            if (ERROR_SUCCESS == RegQueryValueEx(hkeyMPR, MPR_VALUE, NULL,
                &dwType, (BYTE*) szAnswer, &cbSize))
            {
                fReconnect = (StrCmpI(szAnswer, (const TCHAR *) MPR_YES) == 0);
            }

            RegCloseKey(hkeyMPR);
        }            
    }

    TraceLeaveValue(fReconnect);
}

void CMapNetDrivePage::WriteReconnect(BOOL fReconnect)
{
    TraceEnter(TRACE_MND_CORE, "CMapNetDrivePage::WriteReconnect");

    // Don't write to the registry if the user didn't have a choice about reconnect
    if (!(m_pConnectStruct->dwFlags & CONNDLG_HIDE_BOX))
    {
        HKEY hkeyMPR;
        DWORD dwDisp;

        // User didn't specify -- check the registry.
        if (ERROR_SUCCESS == RegCreateKeyEx(MPR_HIVE, MPR_KEY, 0, NULL, 0, KEY_WRITE, NULL,
            &hkeyMPR, &dwDisp))
        {
            LPTSTR pszNewValue = (fReconnect ? MPR_YES : MPR_NO);

            RegSetValueEx(hkeyMPR, MPR_VALUE, NULL,
                REG_SZ, (BYTE*) pszNewValue, (lstrlen(pszNewValue) + 1) * sizeof (TCHAR));

            RegCloseKey(hkeyMPR);
        }            
    }

    TraceLeaveVoid();
}

void CMapNetDrivePage::FillDriveBox(HWND hwnd)
// This routine fills the drive letter drop-down list with all
// of the drive names and, if appropriate, the name of the share to which
// the drive is already connected
{
    TraceEnter(TRACE_MND_CORE, "CMapNetDrivePage::FillDriveBox");

    HWND    hWndCombo      = GetDlgItem(hwnd, IDC_DRIVELETTER);
    DWORD   dwFlags        = 0;
    DWORD   dwBufferLength = MAX_PATH - 1;
    TCHAR   szDriveName[SHARE_NAME_INDEX + MAX_PATH];
    TCHAR   szShareName[MAX_PATH - DRIVE_NAME_LENGTH];

    UINT    uDriveType;

    TraceAssert(hWndCombo);

    ZeroMemory(szDriveName, sizeof(szDriveName));
    ZeroMemory(szShareName, sizeof(szShareName));

    
    // lpDriveName looks like this: "<space>:<null>"
    lstrcpyn(szDriveName, DRIVE_NAME_STRING, ARRAYSIZE(szDriveName));

    
    // lpDriveName looks like this: 
    // "<space>:<null><spaces until index SHARE_NAME_INDEX>"
    for (UINT i = DRIVE_NAME_LENGTH + 1; i < SHARE_NAME_INDEX; i++)
    {
        szDriveName[i] = L' ';
    }

    for (TCHAR cDriveLetter = FIRST_DRIVE; 
               cDriveLetter <= LAST_DRIVE; 
               cDriveLetter++)
    {
        
        // lpDriveName looks like this: "<drive>:<null><lots of spaces>"
        szDriveName[0] = cDriveLetter;
        uDriveType = GetDriveType(szDriveName);

        
        // Removable == floppy drives, Fixed == hard disk, CDROM == obvious :),
        // Remote == network drive already attached to a share
        switch (uDriveType)
        {
            case DRIVE_REMOVABLE:
            case DRIVE_FIXED:
            case DRIVE_CDROM:
                // These types of drives can't be mapped
                break;
            case DRIVE_REMOTE:
            {
                UINT    i;

                // Reset the share buffer length (it 
                // gets overwritten by WNetGetConnection)
                dwBufferLength = MAX_PATH - DRIVE_NAME_LENGTH - 1;
                
                // Retrieve "\\server\share" for current drive
                if (WNetGetConnection(szDriveName,
                                      szShareName,
                                      &dwBufferLength)
                        != NO_ERROR)
                {                    
                    // If there's an error with this drive, ignore the drive
                    // and skip to the next one.  Note that dwBufferLength will
                    // only be changed if lpShareName contains MAX_PATH or more
                    // characters, which shouldn't ever happen.  For release,
                    // however, keep on limping along.
                    
                    TraceAssert(dwBufferLength == MAX_PATH - DRIVE_NAME_LENGTH - 1);
                    dwBufferLength = MAX_PATH - DRIVE_NAME_LENGTH - 1;

                    break;
                }
                // lpDriveName looks like this: 
                // "<drive>:<spaces until SHARE_NAME_INDEX><share name><null>"
                
                szDriveName[DRIVE_NAME_LENGTH] = L' ';
                lstrcpyn(szDriveName + SHARE_NAME_INDEX,
                         szShareName,
                         MAX_PATH - DRIVE_NAME_LENGTH - 1);

                int iItem = ComboBox_AddString(hWndCombo, szDriveName);

                // Store a FALSE into the item data for all items except the
                // special (none) item
                ComboBox_SetItemData(hWndCombo, iItem, (LPARAM) FALSE);

                // Reset the drive name to "<drive>:<null><lots of spaces>"
                
                szDriveName[DRIVE_NAME_LENGTH] = L'\0';

                for (i = DRIVE_NAME_LENGTH + 1; 
                     i < MAX_PATH + SHARE_NAME_INDEX; 
                     i++)
                {
                    *(szDriveName + i) = L' ';
                }
                break;
            }
            default:                                                
                // The drive is not already connected to a share
                DWORD dwIndex = ComboBox_AddString(hWndCombo, szDriveName);

                TraceAssert(dwIndex != CB_ERR && dwIndex != CB_ERRSPACE);

                // Suggest the first available and unconnected 
                // drive past the C drive
                if (!(dwFlags & SELECT_DONE) && 
                    (dwFlags & PAST_SELECT_DRIVE))
                {                
                    ComboBox_SetCurSel(hWndCombo, dwIndex);
                    dwFlags |= SELECT_DONE;
                }
        }
        // Note if we're past the C drive.  Note that the check is for >=
        // just in case we had an error above while we were at SELECT_DRIVE
        if (szDriveName[0] >= SELECT_DRIVE)
        {
            dwFlags |= PAST_SELECT_DRIVE;
        }
    }
    // Add one more item - a special (none) item that if selected causes 
    // a deviceless connection to be created.
    TCHAR szNone[MAX_CAPTION];
    LoadString(g_hInstance, IDS_DRIVE_NONE, szNone, ARRAYSIZE(szNone));
    int iItem = ComboBox_AddString(hWndCombo, szNone);
    ComboBox_SetItemData(hWndCombo, iItem, (LPARAM) TRUE);

    // If there is no selection at this point, just select (none) item
    // This will happen when all drive letters are mapped
    if (ComboBox_GetCurSel(hWndCombo) == CB_ERR)
    {
        ComboBox_SetCurSel(hWndCombo, iItem);
    }

    TraceLeaveVoid();
}

BOOL CMapNetDrivePage::MapDrive(HWND hwnd)
{
    TraceEnter(TRACE_MND_CORE, "CMapNetDrivePage::MapDrive");

    BOOL fMapWorked = FALSE;
    
    HWND hwndCombo = GetDlgItem(hwnd, IDC_DRIVELETTER);
    int iItem = ComboBox_GetCurSel(hwndCombo);

    // Get this item's text and itemdata (to check if its the special (none) drive)
    BOOL fNone = (BOOL) ComboBox_GetItemData(hwndCombo, iItem);
        
    // Fill in the big structure that maps a drive
    MapNetThreadData* pdata = new MapNetThreadData;

    if (pdata != NULL)
    {
        // Set reconnect
        pdata->fReconnect = (BST_CHECKED == Button_GetCheck(GetDlgItem(hwnd, IDC_RECONNECT)));

        // Set the drive
        if (fNone)
        {
            pdata->szDrive[0] = TEXT('\0');
        }
        else
        {
            ComboBox_GetText(hwndCombo, pdata->szDrive, 3);
        }

        // Set the net share
        GetWindowText(GetDlgItem(hwnd, IDC_FOLDER), pdata->szPath, ARRAYSIZE(pdata->szPath));
        PathRemoveBackslash(pdata->szPath);

        // Get an alternate username/password/domain if required
        // Domain/username
        lstrcpyn(pdata->szDomainUser, m_szDomainUser, ARRAYSIZE(pdata->szDomainUser));

        // Password
        lstrcpyn(pdata->szPassword, m_szPassword, ARRAYSIZE(pdata->szPassword));

        CMapNetProgress dlg(pdata, &m_pConnectStruct->dwDevNum, m_pdwLastError);
        
        // On IDOK == Close dialog!
        fMapWorked = (IDOK == dlg.DoModal(g_hInstance, MAKEINTRESOURCE(IDD_MND_PROGRESS_DLG), hwnd));
    }

    if (fMapWorked)
    {
        TCHAR szPath[MAX_PATH + 1];
        GetWindowText(GetDlgItem(hwnd, IDC_FOLDER), szPath, ARRAYSIZE(szPath));
        m_MRU.AddString(szPath);

        // If a drive letter wasn't assigned, open a window on the new drive now
        if (fNone)
        {
            // Use shellexecuteex to open a view folder
            SHELLEXECUTEINFO shexinfo = {0};
            shexinfo.cbSize = sizeof (shexinfo);
            shexinfo.fMask = SEE_MASK_FLAG_NO_UI;
            shexinfo.nShow = SW_SHOWNORMAL;
            shexinfo.lpFile = szPath;
            shexinfo.lpVerb = TEXT("open");

            ShellExecuteEx(&shexinfo);
        }
    }

    TraceLeaveValue(fMapWorked); 
}


// Little progress dialog implementation

// Private message for thread to signal dialog if successful
//  (DWORD) (WPARAM) dwDevNum - 0-based device number connected to (0xFFFFFFFF for none)
//  (DWORD) (LPARAM) dwRetVal - Return value
#define WM_MAPFINISH (WM_USER + 100)

INT_PTR CMapNetProgress::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        case WM_MAPFINISH: return OnMapSuccess(hwnd, (DWORD) wParam, (DWORD) lParam);
    }

    return FALSE;
}

BOOL CMapNetProgress::OnMapSuccess(HWND hwnd, DWORD dwDevNum, DWORD dwLastError)
{
    TraceEnter(TRACE_MND_CORE, "CMapNetProgress::OnMapSuccess");

    *m_pdwDevNum = dwDevNum;
    *m_pdwLastError = dwLastError;

    UINT idEnd = (dwLastError == WN_SUCCESS) ? IDOK : IDCANCEL;

    EndDialog(hwnd, idEnd);

    TraceLeaveValue(TRUE);
}

BOOL CMapNetProgress::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    TraceEnter(TRACE_MND_CORE, "CMapNetProgress::OnInitDialog");

    HANDLE hThread = NULL;

    // Set the progress dialog text
    TCHAR szText[256];
    FormatMessageString(IDS_MND_PROGRESS, szText, ARRAYSIZE(szText), m_pdata->szPath);

    SetWindowText(GetDlgItem(hwnd, IDC_CONNECTING), szText);


    // We'll signal this guy when the thread should close down
    static const TCHAR EVENT_NAME[] = TEXT("Thread Close Event");
    m_hEventCloseNow = CreateEvent(NULL, TRUE, FALSE, EVENT_NAME);
    m_pdata->hEventCloseNow = NULL;

    if (m_hEventCloseNow != NULL)
    {
        // Get a copy of this puppy for the thread
        m_pdata->hEventCloseNow = OpenEvent(SYNCHRONIZE, FALSE, EVENT_NAME);

        if (m_pdata->hEventCloseNow != NULL)
        {
            m_pdata->hwnd = hwnd;

            // All we have to do is start up the worker thread, who will dutifully report back to us
            DWORD dwId;
            hThread = CreateThread(NULL, 0, CMapNetProgress::MapDriveThread, (LPVOID) m_pdata, 0, &dwId);
        }
    }

    // Abandon the poor little guy (he'll be ok)
    if (hThread != NULL)
    {
        CloseHandle(hThread);

        /* TAKE SPECIAL CARE
        At this point the thread owns m_pdata! Don't access it any more except on the thread.
        It may be deleted at any time! */

        m_pdata = NULL;
    }
    else
    {
        // Usually the thread would do this
        if (m_pdata->hEventCloseNow != NULL)
        {
            CloseHandle(m_pdata->hEventCloseNow);
        }
    
        delete m_pdata;

        // BUGBUG: Add an unexpected error messagebox here
        EndDialog(hwnd, IDCANCEL);
    }
    
    TraceLeaveValue(FALSE);
}

BOOL CMapNetProgress::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    TraceEnter(TRACE_MND_CORE, "CMapNetProgress::OnCommand");

    switch (id)
    {
    case IDCANCEL:
        {
            // Tell the thread to quit
            SetEvent(m_hEventCloseNow);
            EndDialog(hwnd, id);
        }
        break;
    };

    TraceLeaveValue(FALSE);
}

DWORD CMapNetProgress::MapDriveThread(LPVOID pvoid)
{
    TraceEnter(TRACE_MND_CORE, "CMapNetProgress::MapDriveThread");

    MapNetThreadData* pdata = (MapNetThreadData*) pvoid;

    DWORD dwDevNum;
    DWORD dwLastError;
    BOOL fSuccess = MapDriveHelper(pdata, &dwDevNum, &dwLastError);

    if (WAIT_OBJECT_0 == WaitForSingleObject(pdata->hEventCloseNow, 0))
    {
        // The user clicked cancel, don't call back to the progress wnd
    }
    else
    {
        PostMessage(pdata->hwnd, WM_MAPFINISH, (WPARAM) dwDevNum, 
            (LPARAM) dwLastError);
    }
    
    CloseHandle(pdata->hEventCloseNow);

    delete pdata;

    TraceLeaveValue(0);
}

BOOL CMapNetProgress::MapDriveHelper(MapNetThreadData* pdata, DWORD* pdwDevNum, DWORD* pdwLastError)
{
    TraceEnter(TRACE_MND_CORE, "CMapNetProgress::MapDriveHelper");

    NETRESOURCE     nrResource = {0};
    LPTSTR          lpMessage = NULL;
   
    //
    // Fill in the NETRESOURCE structure -- the local name is the drive and
    // the remote name is \\server\share (stored in the global buffer).
    // The provider is NULL to let NT find the provider on its own.
    //
    nrResource.dwType         = RESOURCETYPE_DISK;
    nrResource.lpLocalName    = pdata->szDrive[0] == TEXT('\0') ? NULL : pdata->szDrive;
    nrResource.lpRemoteName   = pdata->szPath;
    nrResource.lpProvider     = NULL;

    BOOL fRetry = TRUE;
    while (fRetry)
    {        
        *pdwLastError = WNetAddConnection3(pdata->hwnd, &nrResource,
            pdata->szDomainUser[0] == TEXT('\0') ? NULL : pdata->szPassword, 
            pdata->szDomainUser[0] == TEXT('\0') ? NULL : pdata->szDomainUser, 
            pdata->fReconnect ? CONNECT_INTERACTIVE | CONNECT_UPDATE_PROFILE : CONNECT_INTERACTIVE);

        // Don't display anything if we're supposed to quit
        if (WAIT_OBJECT_0 == WaitForSingleObject(pdata->hEventCloseNow, 0))
        {   
            // We should quit (quietly exit if we just failed)!
            if (*pdwLastError != NO_ERROR)
            {
                *pdwLastError = RETCODE_CANCEL;
                *pdwDevNum = 0;
                fRetry = FALSE;
            }
        }
        else
        {
            fRetry = FALSE;

            switch (*pdwLastError)
            {
                case NO_ERROR:
                    {
                        // Put the number of the connection into dwDevNum, where 
                        // drive A is 1, B is 2, ... Note that a deviceless connection
                        // is 0xFFFFFFFF
                        if (pdata->szDrive[0] == TEXT('\0'))
                        {
                            *pdwDevNum = 0xFFFFFFFF;
                        }
                        else
                        {
                            *pdwDevNum = *pdata->szDrive - FIRST_DRIVE + 1;
                        }
                
                        *pdwLastError = WN_SUCCESS;
                    }
                    break;

                //
                // The user cancelled the password dialog or cancelled the
                // connection through a different dialog
                //
                case ERROR_CANCELLED:
                    {
                        *pdwLastError = RETCODE_CANCEL;
                    }
                    break;

                //
                // An error involving the user's password/credentials occurred, so
                // bring up the password prompt.
                //
                case ERROR_ACCESS_DENIED:
                case ERROR_CANNOT_OPEN_PROFILE:
                case ERROR_INVALID_PASSWORD:
                case ERROR_LOGON_FAILURE:
                case ERROR_BAD_USERNAME:
                    {
                        CPasswordDialog dlg(pdata->szPath, pdata->szDomainUser, ARRAYSIZE(pdata->szDomainUser), 
                            pdata->szPassword, ARRAYSIZE(pdata->szPassword), *pdwLastError);

                        if (IDOK == dlg.DoModal(g_hInstance, MAKEINTRESOURCE(IDD_WIZ_NETPASSWORD),
                            pdata->hwnd))
                        {
                            fRetry = TRUE;
                        }
                    }
                    break;

                // There's an existing/remembered connection to this drive
                case ERROR_ALREADY_ASSIGNED:
                case ERROR_DEVICE_ALREADY_REMEMBERED:

                    // See if the user wants us to break the connection
                    if (ConfirmDisconnectDrive(pdata->hwnd, 
                                                pdata->szDrive,
                                                pdata->szPath,
                                                *pdwLastError))
                    {
                        // Break the connection, but don't force it 
                        // if there are open files
                        *pdwLastError = WNetCancelConnection2(pdata->szDrive,
                                                        CONNECT_UPDATE_PROFILE,
                                                        FALSE);
     
                        if (*pdwLastError == ERROR_OPEN_FILES || 
                            *pdwLastError == ERROR_DEVICE_IN_USE)
                        {                    
                            // See if the user wants to force the disconnection
                            if (ConfirmDisconnectOpenFiles(pdata->hwnd))
                            {
                                // Roger 1-9er -- we have confirmation, 
                                // so force the disconnection.
                                *pdwLastError = WNetCancelConnection2(pdata->szDrive,
                                                      CONNECT_UPDATE_PROFILE,
                                                      TRUE);

                                if (*pdwLastError == NO_ERROR)
                                {
                                    fRetry = TRUE;
                                }
                                else
                                {
                                    DisplayFormatMessage(pdata->hwnd, IDS_CONNECT_DRIVE_CAPTION, IDS_CANTCLOSEFILES_WARNING,
                                        MB_OK | MB_ICONERROR);
                                }
                            }
                        }
                        else
                        {
                            fRetry = TRUE;
                        }
                    }
                    break;

                // Errors caused by an invalid remote path
                case ERROR_BAD_DEV_TYPE:
                case ERROR_BAD_NET_NAME:
                case ERROR_BAD_NETPATH:
                    {

                        DisplayFormatMessage(pdata->hwnd, IDS_ERR_CAPTION, IDS_INVALID_REMOTE_PATH,
                            MB_OK | MB_ICONERROR, pdata->szPath);
                    }
                    break;

                // Provider is busy (e.g., initializing), so user should retry
                case ERROR_BUSY:
                    {
                        DisplayFormatMessage(pdata->hwnd, IDS_ERR_CAPTION, IDS_INVALID_REMOTE_PATH,
                            MB_OK | MB_ICONERROR);
                    }
                    break;
                //
                // Network problems
                //
                case ERROR_NO_NET_OR_BAD_PATH:
                case ERROR_NO_NETWORK:
                    {
                        DisplayFormatMessage(pdata->hwnd, IDS_ERR_CAPTION, IDS_ERR_NONETWORK,
                            MB_OK | MB_ICONERROR);
                    }
                    break;

                // Share already mapped with different credentials
                case ERROR_SESSION_CREDENTIAL_CONFLICT:
                    {
                        DisplayFormatMessage(pdata->hwnd, IDS_ERR_CAPTION,
                            IDS_MND_ALREADYMAPPED, MB_OK | MB_ICONERROR);
                    }

                //
                // Errors that we (in theory) shouldn't get -- bad local name 
                // (i.e., format of drive name is invalid), user profile in a bad 
                // format, or a bad provider.  Problems here most likely indicate 
                // an NT system bug.  Also note that provider-specific errors 
                // (ERROR_EXTENDED_ERROR) and trust failures are lumped in here 
                // as well, since the below errors will display an "Unexpected 
                // Error" message to the user.
                //
                case ERROR_BAD_DEVICE:
                case ERROR_BAD_PROFILE:
                case ERROR_BAD_PROVIDER:
                default:
                    {
                        TCHAR szMessage[512];

                        DWORD dwFormatResult = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, (DWORD) *pdwLastError, 0, szMessage, ARRAYSIZE(szMessage), NULL);

                        if (0 == dwFormatResult)
                        {
                            LoadString(g_hInstance, IDS_ERR_UNEXPECTED, szMessage, ARRAYSIZE(szMessage));
                        }

                        ::DisplayFormatMessage(pdata->hwnd, IDS_ERR_CAPTION, IDS_MND_GENERICERROR, MB_OK|MB_ICONERROR, szMessage);
                    }
                    break;
                
            }
        }
    }

    TraceLeaveValue(*pdwLastError == NO_ERROR);
}

BOOL CMapNetProgress::ConfirmDisconnectDrive(HWND hWndDlg, LPCTSTR lpDrive, LPCTSTR lpShare, DWORD dwType)
/*++

Routine Description:

    This routine verifies that the user wants to break a pre-existing
    connection to a drive.

Arguments:

    hWndDlg -- HWND of the Completion page
    lpDrive -- The name of the drive to disconnect
    lpShare -- The share to which the "freed" drive will be connected
    dwType  -- The connection error -- ERROR_ALREADY_ASSIGNED 
               or ERROR_DEVICE_ALREADY_REMEMBERED
    
Return Value:

    TRUE if the user wants to break the connection, FALSE otherwise

--*/
{
   // Trace(TRACE_LEVEL_FLOW, TEXT("Entering ConfirmDisconnectDrive\n"));

    TCHAR   tszConfirmMessage[2 * MAX_PATH + MAX_STATIC] = {0};
    TCHAR   tszCaption[MAX_CAPTION + 1] = {0};
    TCHAR   tszConnection[MAX_PATH + 1] = {0};

    DWORD   dwLength = MAX_PATH;

    LoadString(g_hInstance, IDS_ERR_CAPTION, tszCaption, ARRAYSIZE(tszCaption));

    //
    // Bug #143955 -- call WNetGetConnection here since with two instances of
    // the wizard open and on the Completion page with the same suggested
    // drive, the Completion combo box doesn't contain info about the connected
    // share once "Finish" is selected on one of the two wizards.
    //
    // BUGBUG -- Check return value
    //
    WNetGetConnection(lpDrive, tszConnection, &dwLength);

    //
    // Load the appropriate propmt string, based on the type of 
    // error we encountered
    //
    FormatMessageString((dwType == ERROR_ALREADY_ASSIGNED ? IDS_ERR_ALREADY_ASSIGNED :
        IDS_ERR_ALREADY_REMEMBERED), tszConfirmMessage, ARRAYSIZE(tszConfirmMessage), lpDrive, tszConnection, lpShare);

    return (MessageBox(hWndDlg, tszConfirmMessage, tszCaption, MB_YESNO | MB_ICONWARNING)
        == IDYES);
}


BOOL CMapNetProgress::ConfirmDisconnectOpenFiles(HWND hWndDlg)
/*++

Routine Description:

    This routine verifies that the user wants to break a pre-existing
    connection to a drive where the user has open files/connections

Arguments:

    hWndDlg -- HWND of the Completion dialog

Return Value:

    TRUE if the user wants to break the connection, FALSE otherwise    

--*/
{
   // Trace(TRACE_LEVEL_FLOW, TEXT("Entering ConfirmDisconnectOpenFiles\n"));

    TCHAR tszCaption[MAX_CAPTION + 1] = {0};
    TCHAR tszBuffer[MAX_STATIC + 1] = {0};

    //
    // Load the prompt strings
    //
    LoadString(g_hInstance, IDS_ERR_OPENFILES, tszBuffer, ARRAYSIZE(tszBuffer));
    LoadString(g_hInstance, IDS_ERR_CAPTION, tszCaption, ARRAYSIZE(tszCaption));

    return (MessageBox(hWndDlg,
                       tszBuffer,
                       tszCaption,
                       MB_YESNO | MB_ICONWARNING)

                == IDYES);
}

// Little "username and password" dialog for connecting as a different user
INT_PTR CConnectAsDlg::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
    HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
    }

    return FALSE;
}

BOOL CConnectAsDlg::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    TraceEnter(TRACE_MND_CORE, "CConnectAsDlg::OnInitDialog");

    // Fill in the user name and password
    HWND hwndUser = GetDlgItem(hwnd, IDC_USER);
    Edit_LimitText(hwndUser, MAX_DOMAIN + MAX_USER + 1);
    SetWindowText(hwndUser, m_pszDomainUser);

    HWND hwndPassword = GetDlgItem(hwnd, IDC_PASSWORD);
    Edit_LimitText(hwndPassword, MAX_PASSWORD);
    SetWindowText(hwndPassword, m_pszPassword);

    TCHAR szUser[MAX_USER + 1];
    TCHAR szDomain[MAX_DOMAIN + 1];
    TCHAR szDomainUser[MAX_USER + MAX_DOMAIN + 2];

    DWORD cchUser = ARRAYSIZE(szUser);
    DWORD cchDomain = ARRAYSIZE(szDomain);
    ::GetCurrentUserAndDomainName(szUser, &cchUser, szDomain,
        &cchDomain);

    ::MakeDomainUserString(szDomain, szUser, szDomainUser, ARRAYSIZE(szDomainUser));

    TCHAR szMessage[256];
    FormatMessageString(IDS_CONNECTASUSER_MESSAGE, szMessage, ARRAYSIZE(szMessage), szDomainUser);

    SetWindowText(GetDlgItem(hwnd, IDC_MESSAGE), szMessage);

    if (!IsComputerInDomain())
    {
        EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), FALSE);
    }

    TraceLeaveValue(FALSE);
}

BOOL CConnectAsDlg::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    TraceEnter(TRACE_MND_CORE, "CConnectAsDlg::OnCommand");
    BOOL fHandled = FALSE;

    switch (id)
    {
    case IDC_BROWSE:
        {
            // User wants to look up a username
            TCHAR szUser[MAX_USER + 1];
            TCHAR szDomain[MAX_DOMAIN + 1];
            if (S_OK == ::BrowseForUser(hwnd, szUser, ARRAYSIZE(szUser), 
                szDomain, ARRAYSIZE(szDomain)))
            {
                TCHAR szDomainUser[MAX_USER + MAX_DOMAIN + 2];
                ::MakeDomainUserString(szDomain, szUser, szDomainUser,
                    ARRAYSIZE(szDomainUser));

                // Ok clicked and buffers valid
                SetDlgItemText(hwnd, IDC_USER, szDomainUser);
            }

            fHandled = TRUE;
        }
        break;
    case IDOK:
        GetWindowText(GetDlgItem(hwnd, IDC_USER), m_pszDomainUser, m_cchDomainUser);
        GetWindowText(GetDlgItem(hwnd, IDC_PASSWORD), m_pszPassword, m_cchPassword);
        // fall through
    case IDCANCEL:
        EndDialog(hwnd, id);
        fHandled = TRUE;
        break;
    }

    TraceLeaveValue(fHandled);
}
