//*************************************************************
//
//  Profile.c   - User Profile tab
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1996
//  All rights reserved
//
//*************************************************************

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include "sysdm.h"
#include <shellapi.h>
#include <shlobj.h>
#include <shlobjp.h>
#include <userenv.h>
#include <userenvp.h>
#include <getuser.h>


#define TEMP_PROFILE     TEXT("Temp profile (sysdm.cpl)")
#define PROFILE_MAPPING  TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList")

//
// Globals
//

DWORD g_dwProfileSize;

//
// Help ID's
//
// IDH_USERPROFILE + 20 is used in the OpenUserBrowser routine.
//
DWORD aUserProfileHelpIds[] = {
    IDC_UP_LISTVIEW,              (IDH_USERPROFILE + 0),
    IDC_UP_DELETE,                (IDH_USERPROFILE + 1),
    IDC_UP_TYPE,                  (IDH_USERPROFILE + 2),
    IDC_UP_COPY,                  (IDH_USERPROFILE + 3),
    IDC_UP_ICON,                  (DWORD) -1,
    IDC_UP_TEXT,                  (DWORD) -1,

    // Change Type dialog
    IDC_UPTYPE_LOCAL,             (IDH_USERPROFILE + 4),
    IDC_UPTYPE_FLOAT,             (IDH_USERPROFILE + 5),
    IDC_UPTYPE_SLOW,              (IDH_USERPROFILE + 6),
    IDC_UPTYPE_SLOW_TEXT,         (IDH_USERPROFILE + 6),
    IDC_UPTYPE_GROUP,             (IDH_USERPROFILE + 12),

    // Copy To dialog
    IDC_COPY_PATH,                (IDH_USERPROFILE + 7),
    IDC_COPY_BROWSE,              (IDH_USERPROFILE + 8),
    IDC_COPY_USER,                (IDH_USERPROFILE + 9),
    IDC_COPY_CHANGE,              (IDH_USERPROFILE + 10),
    IDC_COPY_GROUP,               (IDH_USERPROFILE + 9),

    0, 0
};


//
// Local function proto-types
//

BOOL InitUserProfileDlg (HWND hDlg, LPARAM lParam);
BOOL FillListView (HWND hDlg, BOOL bAdmin);
LPTSTR CheckSlash (LPTSTR lpDir);
BOOL RecurseDirectory (LPTSTR lpDir);
VOID UPSave (HWND hDlg);
VOID UPCleanUp (HWND hDlg);
BOOL IsProfileInUse (LPTSTR lpSid);
void UPDoItemChanged(HWND hDlg, int idCtl);
void UPDeleteProfile(HWND hDlg);
BOOL DeleteProfile (LPTSTR lpSidString, LPTSTR lpLocalProfile);
void UPChangeType(HWND hDlg);
BOOL APIENTRY ChangeTypeDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void UPCopyProfile(HWND hDlg);
BOOL APIENTRY UPCopyDlgProc (HWND hDlg, UINT uMsg,WPARAM wParam, LPARAM lParam);
BOOL UPCreateProfile (LPUPCOPYINFO lpUPCopyInfo, LPTSTR lpDest, PSECURITY_DESCRIPTOR pSecDesc);
VOID UPDisplayErrorMessage(HWND hWnd, UINT uiSystemError);
BOOL ApplyHiveSecurity(LPTSTR lpHiveName, PSID pSid);

//*************************************************************
//
//  CreateProfilePage()
//
//  Purpose:    Creates the profile page
//
//  Parameters: hInst   -   hInstance
//
//
//  Return:     hPage if successful
//              NULL if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/11/95    ericflo    Created
//
//*************************************************************

HPROPSHEETPAGE CreateProfilePage (HINSTANCE hInst)
{
    PROPSHEETPAGE psp;

    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = 0;
    psp.hInstance = hInst;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_USERPROFILE);
    psp.pfnDlgProc = UserProfileDlgProc;
    psp.pszTitle = NULL;
    psp.lParam = 0;

    return CreatePropertySheetPage(&psp);
}


//*************************************************************
//
//  UserProfileDlgProc()
//
//  Purpose:    Dialog box procedure for profile tab
//
//  Parameters: hDlg    -   handle to the dialog box
//              uMsg    -   window message
//              wParam  -   wParam
//              lParam  -   lParam
//
//  Return:     TRUE if message was processed
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/11/95    ericflo    Created
//
//*************************************************************

BOOL APIENTRY UserProfileDlgProc (HWND hDlg, UINT uMsg,
                                  WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_INITDIALOG:
           if (!InitUserProfileDlg (hDlg, lParam)) {
               EndDialog(hDlg, FALSE);
           }
           return TRUE;


    case WM_NOTIFY:

        switch (((NMHDR FAR*)lParam)->code)
        {
        case LVN_ITEMCHANGED:
            UPDoItemChanged(hDlg, (int) wParam);
            break;

        case LVN_COLUMNCLICK:
            break;

        case PSN_APPLY:
            {
            PSHNOTIFY *lpNotify = (PSHNOTIFY *) lParam;

            UPSave(hDlg);
            }
            break;


        case PSN_RESET:
            SetWindowLong (hDlg, DWL_MSGRESULT, PSNRET_NOERROR);
            return TRUE;

        default:
            return FALSE;
        }
        break;

    case WM_DESTROY:
        UPCleanUp (hDlg);
        break;

    case WM_COMMAND:

        switch (LOWORD(wParam)) {
            case IDC_UP_DELETE:
                UPDeleteProfile(hDlg);
                break;

            case IDC_UP_TYPE:
                UPChangeType(hDlg);
                break;

            case IDC_UP_COPY:
                UPCopyProfile(hDlg);
                break;

            default:
                break;

        }
        break;

    case WM_HELP:      // F1
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
        (DWORD) (LPSTR) aUserProfileHelpIds);
        break;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
        (DWORD) (LPSTR) aUserProfileHelpIds);
        return (TRUE);

    }

    return (FALSE);
}

//*************************************************************
//
//  InitUserProfileDlg()
//
//  Purpose:    Initializes the User Profiles page
//
//  Parameters: hDlg    -   Dialog box handle
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              1/26/96     ericflo    Created
//
//*************************************************************

BOOL InitUserProfileDlg (HWND hDlg, LPARAM lParam)
{
    TCHAR szHeaderName[30];
    LV_COLUMN col;
    RECT rc;
    HWND hLV;
    INT iTotal = 0, iCurrent;
    HWND hwndTemp;
    BOOL bAdmin;
    HCURSOR hOldCursor;


    hOldCursor = SetCursor (LoadCursor(NULL, IDC_WAIT));

    hLV = GetDlgItem(hDlg, IDC_UP_LISTVIEW);

    // Set extended LV style for whole line selection
    SendMessage(hLV, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

    //
    // Insert Columns
    //

    GetClientRect (hLV, &rc);
    LoadString(hInstance, IDS_UP_NAME, szHeaderName, 30);
    col.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
    col.fmt = LVCFMT_LEFT;
    iCurrent = (int)(rc.right * .45);
    iTotal += iCurrent;
    col.cx = iCurrent;
    col.pszText = szHeaderName;
    col.iSubItem = 0;

    ListView_InsertColumn (hLV, 0, &col);


    LoadString(hInstance, IDS_UP_SIZE, szHeaderName, 30);
    iCurrent = (int)(rc.right * .15);
    iTotal += iCurrent;
    col.cx = iCurrent;
    col.fmt = LVCFMT_RIGHT;
    col.iSubItem = 1;
    ListView_InsertColumn (hLV, 1, &col);


    LoadString(hInstance, IDS_UP_TYPE, szHeaderName, 30);
    col.iSubItem = 2;
    iCurrent = (int)(rc.right * .20);
    iTotal += iCurrent;
    col.cx = iCurrent;
    col.fmt = LVCFMT_LEFT;
    ListView_InsertColumn (hLV, 2, &col);

    LoadString(hInstance, IDS_UP_MOD, szHeaderName, 30);
    col.iSubItem = 3;
    col.cx = rc.right - iTotal - GetSystemMetrics(SM_CYHSCROLL);
    col.fmt = LVCFMT_LEFT;
    ListView_InsertColumn (hLV, 3, &col);

    bAdmin = IsUserAdmin();


    if (!bAdmin) {
        RECT rc;
        POINT pt;

        //
        // If the user is not an admin, then we hide the
        // delete and copy to buttons
        //

        hwndTemp = GetDlgItem (hDlg, IDC_UP_DELETE);
        GetWindowRect (hwndTemp, &rc);
        EnableWindow (hwndTemp, FALSE);
        ShowWindow (hwndTemp, SW_HIDE);

        hwndTemp = GetDlgItem (hDlg, IDC_UP_COPY);
        EnableWindow (hwndTemp, FALSE);
        ShowWindow (hwndTemp, SW_HIDE);

        //
        // Move the Change Type button over
        //

        pt.x = rc.left;
        pt.y = rc.top;
        ScreenToClient (hDlg, &pt);

        SetWindowPos (GetDlgItem (hDlg, IDC_UP_TYPE),
                      HWND_TOP, pt.x, pt.y, 0, 0,
                      SWP_NOSIZE | SWP_NOZORDER);

    }

    //
    // Fill the user accounts
    //

    if (!FillListView (hDlg, bAdmin)) {
        SetCursor(hOldCursor);
        return FALSE;
    }

    SetCursor(hOldCursor);

    return TRUE;
}

//*************************************************************
//
//  AddUser()
//
//  Purpose:    Adds a new user to the listview
//
//
//  Parameters: hDlg            -   handle to the dialog box
//              lpSid           -   Sid (text form)
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              1/26/96     ericflo    Created
//
//*************************************************************

BOOL AddUser (HWND hDlg, LPTSTR lpSid)
{
    LONG Error;
    HKEY hKeyUser;
    TCHAR szBuffer[MAX_PATH];
    TCHAR szBuffer2[MAX_PATH];
    TCHAR szTemp[100];
    TCHAR szTemp2[100];
    DWORD dwTempSize = 100, dwTemp2Size = 100;
    PSID pSid;
    DWORD dwSize, dwType;
    SID_NAME_USE SidName;
    LV_ITEM item;
    INT iItem;
    HWND hwndTemp;
    HKEY hkeyUser;
    HANDLE hFile;
    WIN32_FIND_DATA fd;
    LPTSTR lpEnd;
    SYSTEMTIME systime;
    TCHAR szProfileSize[20];
    INT iTypeID;
    DWORD dwProfileType;
    LPUSERINFO lpUserInfo;
    BOOL bCentralAvailable = FALSE;


    //
    // Open the user's info
    //

    wsprintf (szBuffer, TEXT("%s\\%s"), PROFILE_MAPPING, lpSid);

    Error = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         szBuffer,
                         0,
                         KEY_READ,
                         &hkeyUser);

    if (Error != ERROR_SUCCESS) {
        return FALSE;
    }


    //
    // Query for the user's central profile location
    //

    dwSize = MAX_PATH * sizeof(TCHAR);
    Error = RegQueryValueEx (hkeyUser,
                             TEXT("CentralProfile"),
                             NULL,
                             &dwType,
                             (LPBYTE) szBuffer2,
                             &dwSize);

    if ((Error == ERROR_SUCCESS) && (szBuffer2[0] != TEXT('\0'))) {
        bCentralAvailable = TRUE;
    }


    //
    // Query for the user's local profile
    //

    dwSize = MAX_PATH * sizeof(TCHAR);
    Error = RegQueryValueEx (hkeyUser,
                             TEXT("ProfileImagePath"),
                             NULL,
                             &dwType,
                             (LPBYTE) szBuffer2,
                             &dwSize);

    if (Error != ERROR_SUCCESS) {
        RegCloseKey (hkeyUser);
        return FALSE;
    }


    //
    // Profile paths need to be expanded
    //

    ExpandEnvironmentStrings (szBuffer2, szBuffer, MAX_PATH);
    lstrcpy (szBuffer2, szBuffer);


    //
    // Test if the directory exists.
    //

    hFile = FindFirstFile (szBuffer, &fd);

    if (hFile == INVALID_HANDLE_VALUE) {
        RegCloseKey (hkeyUser);
        return FALSE;
    }

    FindClose (hFile);


    //
    // Get the time stamp of the profile
    //

    lpEnd = CheckSlash (szBuffer);
    lstrcpy (lpEnd, TEXT("ntuser.dat"));


    //
    // Look for a normal user profile
    //

    hFile = FindFirstFile (szBuffer, &fd);

    if (hFile == INVALID_HANDLE_VALUE) {

        //
        // Try a mandatory user profile
        //

        lstrcpy (lpEnd, TEXT("ntuser.man"));

        hFile = FindFirstFile (szBuffer, &fd);

        if (hFile == INVALID_HANDLE_VALUE) {
            RegCloseKey (hkeyUser);
            return FALSE;
        }
    }

    FindClose (hFile);


    //
    // Get the profile size
    //

    g_dwProfileSize = 0;
    *lpEnd = TEXT('\0');

    if (!RecurseDirectory (szBuffer)) {
        RegCloseKey (hkeyUser);
        return FALSE;
    }


    if (g_dwProfileSize < 1024) {
        g_dwProfileSize = 1024;
    }

    g_dwProfileSize /= 1024;

    if (g_dwProfileSize == 0) {
        g_dwProfileSize = 1;
    }

    LoadString (hInstance, IDS_UP_KB, szTemp, 100);
    wsprintf (szProfileSize, szTemp, g_dwProfileSize);


    //
    // Query for the profile type
    //

    dwSize = sizeof(dwProfileType);
    Error = RegQueryValueEx (hkeyUser,
                             TEXT("UserPreference"),
                             NULL,
                             &dwType,
                             (LPBYTE) &dwProfileType,
                             &dwSize);

    if (Error != ERROR_SUCCESS) {

        //
        // The User hasn't specified options yet,
        // so check out the state information
        //

        Error = RegQueryValueEx (hkeyUser,
                                 TEXT("State"),
                                 NULL,
                                 &dwType,
                                 (LPBYTE) &dwProfileType,
                                 &dwSize);


        if (Error != ERROR_SUCCESS) {
            dwProfileType = 0;
        }

        //
        // The constants being used come from
        // windows\gina\userenv\profile.h
        //

        if (dwProfileType & 0x00000001) {
            dwProfileType = USERINFO_MANDATORY;

        } else if (dwProfileType & 0x00000010) {
            dwProfileType = USERINFO_FLOATING;

        } else {
            dwProfileType = USERINFO_LOCAL;
        }
    }

    switch (dwProfileType) {
        case USERINFO_MANDATORY:
            iTypeID = IDS_UP_MANDATORY;
            break;

        case USERINFO_FLOATING:
        case USERINFO_LOCAL_SLOW_LINK:
            iTypeID = IDS_UP_FLOATING;
            break;

        default:
            iTypeID = IDS_UP_LOCAL;
            break;
    }



    //
    // Get the friendly display name
    //

    Error = RegQueryValueEx (hkeyUser, TEXT("Sid"), NULL, &dwType, NULL, &dwSize);

    if (Error != ERROR_SUCCESS) {
        RegCloseKey (hkeyUser);
        return FALSE;
    }


    pSid = MemAlloc (LPTR, dwSize);

    if (!pSid) {
        RegCloseKey (hkeyUser);
        return FALSE;
    }


    Error = RegQueryValueEx (hkeyUser,
                             TEXT("Sid"),
                             NULL,
                             &dwType,
                             (LPBYTE)pSid,
                             &dwSize);

    if (Error != ERROR_SUCCESS) {
        RegCloseKey (hkeyUser);
        return FALSE;
    }


    //
    // Get the friendly names
    //

    szTemp[0] = TEXT('\0');

    if (!LookupAccountSid (NULL, pSid, szTemp, &dwTempSize,
                           szTemp2, &dwTemp2Size, &SidName)) {

        //
        // Unknown account
        //

        LoadString (hInstance, IDS_UP_ACCUNKNOWN, szBuffer, MAX_PATH);

    } else {

        if (szTemp[0] != TEXT('\0')) {
            //
            // Build the display name
            //

            wsprintf (szBuffer, TEXT("%s\\%s"), szTemp2, szTemp);
        } else {

            //
            // Account deleted.
            //

            LoadString (hInstance, IDS_UP_ACCDELETED, szBuffer, MAX_PATH);
        }
    }


    //
    // Allocate a UserInfo structure
    //

    lpUserInfo = (LPUSERINFO) MemAlloc(LPTR, (sizeof(USERINFO) +
                                       (lstrlen (lpSid) + 1) * sizeof(TCHAR) +
                                       (lstrlen (szBuffer2) + 1) * sizeof(TCHAR)));

    if (!lpUserInfo) {
        MemFree (pSid);
        RegCloseKey (hkeyUser);
        return FALSE;
    }

    lpUserInfo->dwFlags = (bCentralAvailable ? USERINFO_FLAG_CENTRAL_AVAILABLE : 0);
    lpUserInfo->lpSid = (LPTSTR)((LPBYTE)lpUserInfo + sizeof(USERINFO));
    lpUserInfo->lpProfile = (LPTSTR) (lpUserInfo->lpSid + lstrlen(lpSid) + 1);
    lstrcpy (lpUserInfo->lpSid, lpSid);
    lstrcpy (lpUserInfo->lpProfile, szBuffer2);
    lpUserInfo->dwProfileType = dwProfileType;



    //
    // Add the item to the listview
    //

    hwndTemp = GetDlgItem (hDlg, IDC_UP_LISTVIEW);


    item.mask = LVIF_TEXT | LVIF_PARAM;
    item.iItem = 0;
    item.iSubItem = 0;
    item.pszText = szBuffer;
    item.lParam = (LPARAM) lpUserInfo;

    iItem = ListView_InsertItem (hwndTemp, &item);


    //
    // Add the profile size
    //

    item.mask = LVIF_TEXT;
    item.iItem = iItem;
    item.iSubItem = 1;
    item.pszText = szProfileSize;

    SendMessage (hwndTemp, LVM_SETITEMTEXT, iItem, (LPARAM) &item);


    //
    // Add the profile type
    //

    LoadString (hInstance, iTypeID, szTemp, 100);

    item.mask = LVIF_TEXT;
    item.iItem = iItem;
    item.iSubItem = 2;
    item.pszText = szTemp;

    SendMessage (hwndTemp, LVM_SETITEMTEXT, iItem, (LPARAM) &item);


    //
    // Add the time/date stamp
    //

    FileTimeToSystemTime (&fd.ftLastAccessTime, &systime);

    GetDateFormat (LOCALE_USER_DEFAULT, DATE_SHORTDATE, &systime,
                    NULL, szBuffer, MAX_PATH);


    item.mask = LVIF_TEXT;
    item.iItem = iItem;
    item.iSubItem = 3;
    item.pszText = szBuffer;

    SendMessage (hwndTemp, LVM_SETITEMTEXT, iItem, (LPARAM) &item);


    //
    // Free the sid
    //

    MemFree (pSid);


    RegCloseKey (hkeyUser);


    return TRUE;
}

//*************************************************************
//
//  FillListView()
//
//  Purpose:    Fills the listview with either all the profiles
//              or just the current user profile depending if
//              the user has Admin privilages
//
//  Parameters: hDlg            -   Dialog box handle
//              bAdmin          -   User an admin
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              1/26/96     ericflo    Created
//
//*************************************************************

BOOL FillListView (HWND hDlg, BOOL bAdmin)
{
    LV_ITEM item;
    BOOL bRetVal = FALSE;

    //
    // If the current user has admin privilages, then
    // he/she can see all the profiles on this machine,
    // otherwise the user only gets their profile.
    //

    if (bAdmin) {

        DWORD SubKeyIndex = 0;
        TCHAR SubKeyName[100];
        DWORD cchSubKeySize;
        HKEY hkeyProfiles;
        LONG Error;


        //
        // Open the profile list
        //

        Error = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             PROFILE_MAPPING,
                             0,
                             KEY_READ,
                             &hkeyProfiles);

        if (Error != ERROR_SUCCESS) {
            return FALSE;
        }


        cchSubKeySize = 100;

        while (TRUE) {

            //
            // Get the next sub-key name
            //

            Error = RegEnumKey(hkeyProfiles, SubKeyIndex, SubKeyName, cchSubKeySize);


            if (Error != ERROR_SUCCESS) {

                if (Error == ERROR_NO_MORE_ITEMS) {

                    //
                    // Successful end of enumeration
                    //

                    Error = ERROR_SUCCESS;

                }

                break;
            }


            AddUser (hDlg, SubKeyName);

            //
            // Go enumerate the next sub-key
            //

            SubKeyIndex ++;
        }

        //
        // Close the registry
        //

        RegCloseKey (hkeyProfiles);

        bRetVal = ((Error == ERROR_SUCCESS) ? TRUE : FALSE);

    } else {

        LPTSTR lpUserSid;

        //
        // The current user doesn't have admin privilages
        //

        lpUserSid = GetSidString();

        if (lpUserSid) {
            AddUser (hDlg, lpUserSid);
            DeleteSidString (lpUserSid);
            bRetVal = TRUE;
        }
    }

    if (bRetVal) {
        //
        // Select the first item
        //

        item.mask = LVIF_STATE;
        item.iItem = 0;
        item.iSubItem = 0;
        item.state = LVIS_SELECTED | LVIS_FOCUSED;
        item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

        SendDlgItemMessage (hDlg, IDC_UP_LISTVIEW,
                            LVM_SETITEMSTATE, 0, (LPARAM) &item);
    }

    return (bRetVal);
}


//*************************************************************
//
//  RecurseDirectory()
//
//  Purpose:    Recurses through the subdirectories counting the size.
//
//  Parameters: lpDir     -   Directory
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              1/30/96     ericflo    Created
//
//*************************************************************

BOOL RecurseDirectory (LPTSTR lpDir)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA fd;
    LPTSTR lpEnd;
    BOOL bResult = TRUE;


    //
    // Setup the ending pointer
    //

    lpEnd = CheckSlash (lpDir);


    //
    // Append *.* to the source directory
    //

    lstrcpy(lpEnd, TEXT("*.*"));



    //
    // Search through the source directory
    //

    hFile = FindFirstFile(lpDir, &fd);

    if (hFile == INVALID_HANDLE_VALUE) {

        if ( (GetLastError() == ERROR_FILE_NOT_FOUND) ||
             (GetLastError() == ERROR_PATH_NOT_FOUND) ) {

            //
            // bResult is already initialized to TRUE, so
            // just fall through.
            //

        } else {

            bResult = FALSE;
        }

        goto RecurseDir_Exit;
    }


    do {

        //
        // Append the file / directory name to the working buffer
        //

        lstrcpy (lpEnd, fd.cFileName);


        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

            //
            // Check for "." and ".."
            //

            if (!lstrcmpi(fd.cFileName, TEXT("."))) {
                continue;
            }

            if (!lstrcmpi(fd.cFileName, TEXT(".."))) {
                continue;
            }


            //
            // Found a directory.
            //
            // 1)  Change into that subdirectory on the source drive.
            // 2)  Recurse down that tree.
            // 3)  Back up one level.
            //

            //
            // Recurse the subdirectory
            //

            if (!RecurseDirectory(lpDir)) {
                bResult = FALSE;
                goto RecurseDir_Exit;
            }

        } else {

            //
            // Found a file, add the filesize
            //

            g_dwProfileSize += fd.nFileSizeLow;
        }


        //
        // Find the next entry
        //

    } while (FindNextFile(hFile, &fd));


RecurseDir_Exit:

    //
    // Remove the file / directory name appended above
    //

    *lpEnd = TEXT('\0');


    //
    // Close the search handle
    //

    if (hFile != INVALID_HANDLE_VALUE) {
        FindClose(hFile);
    }

    return bResult;
}

//*************************************************************
//
//  UPCleanUp()
//
//  Purpose:    Free's resources for this dialog box
//
//  Parameters: hDlg    -   Dialog box handle
//
//  Return:     void
//
//  Comments:
//
//  History:    Date        Author     Comment
//              1/31/96     ericflo    Created
//
//*************************************************************

VOID UPCleanUp (HWND hDlg)
{
    int           i, n;
    HWND          hwndTemp;
    LPUSERINFO    lpUserInfo;
    LV_ITEM       item;


    //
    //  Free memory used for the listview items
    //

    hwndTemp = GetDlgItem (hDlg, IDC_UP_LISTVIEW);
    n = SendMessage (hwndTemp, LVM_GETITEMCOUNT, 0, 0L);

    item.mask = LVIF_PARAM;
    item.iSubItem = 0;

    for (i = 0; i < n; i++) {

        item.iItem = i;

        if (SendMessage (hwndTemp, LVM_GETITEM, 0, (LPARAM) &item)) {
            lpUserInfo = (LPUSERINFO) item.lParam;

        } else {
            lpUserInfo = NULL;
        }

        if (lpUserInfo) {
            MemFree (lpUserInfo);
        }
    }
}

//*************************************************************
//
//  UPSave()
//
//  Purpose:    Saves the settings
//
//  Parameters: hDlg    -   Dialog box handle
//
//  Return:     void
//
//  Comments:
//
//  History:    Date        Author     Comment
//              1/31/96     ericflo    Created
//
//*************************************************************

VOID UPSave (HWND hDlg)
{
    int           i, n;
    HWND          hwndTemp;
    LPUSERINFO    lpUserInfo;
    LV_ITEM       item;
    TCHAR         szBuffer[MAX_PATH];
    HKEY          hkeyUser;
    LONG          Error;


    //
    //  Save type info
    //

    hwndTemp = GetDlgItem (hDlg, IDC_UP_LISTVIEW);
    n = SendMessage (hwndTemp, LVM_GETITEMCOUNT, 0, 0L);

    item.mask = LVIF_PARAM;
    item.iSubItem = 0;

    for (i = 0; i < n; i++) {

        item.iItem = i;

        if (SendMessage (hwndTemp, LVM_GETITEM, 0, (LPARAM) &item)) {
            lpUserInfo = (LPUSERINFO) item.lParam;

        } else {
            lpUserInfo = NULL;
        }

        if (lpUserInfo) {

            if (lpUserInfo->dwFlags & USERINFO_FLAG_DIRTY) {

                lpUserInfo->dwFlags &= ~USERINFO_FLAG_DIRTY;

                wsprintf (szBuffer, TEXT("%s\\%s"), PROFILE_MAPPING,
                          lpUserInfo->lpSid);

                Error = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                     szBuffer,
                                     0,
                                     KEY_WRITE,
                                     &hkeyUser);

                if (Error != ERROR_SUCCESS) {
                    continue;
                }

                RegSetValueEx (hkeyUser,
                               TEXT("UserPreference"),
                               0,
                               REG_DWORD,
                               (LPBYTE) &lpUserInfo->dwProfileType,
                               sizeof(DWORD));

                RegCloseKey(hkeyUser);
            }
        }
    }
}

//*************************************************************
//
//  IsProfileInUse()
//
//  Purpose:    Determines if the given profile is currently in use
//
//  Parameters: lpSid   -   Sid (text) to test
//
//  Return:     TRUE if in use
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/7/96      ericflo    Created
//
//*************************************************************

BOOL IsProfileInUse (LPTSTR lpSid)
{
    LONG lResult;
    HKEY hkeyProfile;


    lResult = RegOpenKeyEx (HKEY_USERS, lpSid, 0, KEY_READ, &hkeyProfile);

    if (lResult == ERROR_SUCCESS) {
        RegCloseKey (hkeyProfile);
        return TRUE;
    }

    return FALSE;
}

void UPDoItemChanged(HWND hDlg, int idCtl)
{
    int     selection;
    HWND    hwndTemp;
    LPUSERINFO lpUserInfo;
    LV_ITEM item;


    hwndTemp = GetDlgItem (hDlg, idCtl);

    selection = GetSelectedItem (hwndTemp);

    if (selection != -1)
    {
        item.mask = LVIF_PARAM;
        item.iItem = selection;
        item.iSubItem = 0;

        if (SendMessage (hwndTemp, LVM_GETITEM, 0, (LPARAM) &item)) {
            lpUserInfo = (LPUSERINFO) item.lParam;

        } else {
            lpUserInfo = NULL;
        }

        if (lpUserInfo) {

            //
            //  Set the "Delete" button state
            //

            if (IsProfileInUse(lpUserInfo->lpSid)) {
                EnableWindow (GetDlgItem (hDlg, IDC_UP_DELETE), FALSE);

            } else {
                EnableWindow (GetDlgItem (hDlg, IDC_UP_DELETE), TRUE);
            }


            //
            // Set the "Change Type" button state
            //

            if (lpUserInfo->dwProfileType == USERINFO_MANDATORY) {
                EnableWindow (GetDlgItem (hDlg, IDC_UP_TYPE), FALSE);
            } else {
                EnableWindow (GetDlgItem (hDlg, IDC_UP_TYPE), TRUE);
            }
        }
    }
}

//*************************************************************
//
//  UPDeleteProfile()
//
//  Purpose:    Deletes a user's profile
//
//  Parameters: hDlg    -   Dialog box handle
//
//  Return:
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/8/96      ericflo    Created
//
//*************************************************************

void UPDeleteProfile(HWND hDlg)
{
    int     selection;
    HWND    hwndTemp;
    LPUSERINFO lpUserInfo;
    LV_ITEM item;
    TCHAR   szName[100];
    TCHAR   szBuffer1[100];
    TCHAR   szBuffer2[200];
    HCURSOR hOldCursor;


    //
    // Get the selected profile
    //

    hwndTemp = GetDlgItem (hDlg, IDC_UP_LISTVIEW);

    selection = GetSelectedItem (hwndTemp);

    if (selection == -1) {
        return;
    }

    item.mask = LVIF_PARAM;
    item.iItem = selection;
    item.iSubItem = 0;

    if (SendMessage (hwndTemp, LVM_GETITEM, 0, (LPARAM) &item)) {
        lpUserInfo = (LPUSERINFO) item.lParam;

    } else {
        lpUserInfo = NULL;
    }

    if (!lpUserInfo) {
        return;
    }


    //
    // Confirm that the user really wants to delete the profile
    //

    szBuffer1[0] = TEXT('\0');
    ListView_GetItemText (hwndTemp, selection, 0, szName, 100);

    LoadString (hInstance, IDS_UP_CONFIRM, szBuffer1, 100);
    wsprintf (szBuffer2, szBuffer1, szName);

    LoadString (hInstance, IDS_UP_CONFIRMTITLE, szBuffer1, 100);
    if (MessageBox (hDlg, szBuffer2, szBuffer1,
                    MB_ICONQUESTION | MB_DEFBUTTON2| MB_YESNO) == IDNO) {
        return;
    }



    //
    // Delete the profile and remove the entry from the listview
    //

    hOldCursor = SetCursor (LoadCursor(NULL, IDC_WAIT));

    DeleteProfile (lpUserInfo->lpSid, lpUserInfo->lpProfile);

    if (ListView_DeleteItem(hwndTemp, selection)) {
        MemFree (lpUserInfo);
    }


    //
    // Select another item
    //

    if (selection > 0) {
        selection--;
    }

    item.mask = LVIF_STATE;
    item.iItem = selection;
    item.iSubItem = 0;
    item.state = LVIS_SELECTED | LVIS_FOCUSED;
    item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

    SendDlgItemMessage (hDlg, IDC_UP_LISTVIEW,
                        LVM_SETITEMSTATE, selection, (LPARAM) &item);


    SetCursor(hOldCursor);
}

//*************************************************************
//
//  DeleteProfile()
//
//  Purpose:    Deletes the specified profile from the
//              registry and disk.
//
//  Parameters: lpSidString     -   Registry subkey
//              lpProfileDir    -   Profile directory
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/8/96      ericflo    Created
//
//*************************************************************

BOOL DeleteProfile (LPTSTR lpSidString, LPTSTR lpProfileDir)
{
    LONG lResult;
    TCHAR szTemp[MAX_PATH];


    //
    // Cleanup the registry first.
    //

    if (lpSidString && *lpSidString) {

        lstrcpy(szTemp, TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\"));
        lstrcat(szTemp, lpSidString);
        lResult = RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        if (lResult != ERROR_SUCCESS) {
            return FALSE;
        }
    }


    //
    // Now delete the profile
    //

    if (!Delnode (lpProfileDir)) {
        return FALSE;
    }

    return TRUE;
}

//*************************************************************
//
//  UPChangeType()
//
//  Purpose:    Display the "Change Type" dialog box
//
//  Parameters: hDlg    -   Dialog box handle
//
//  Return:     void
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/09/96     ericflo    Created
//
//*************************************************************

void UPChangeType(HWND hDlg)
{
    int     selection, iTypeID;
    HWND    hwndTemp;
    LPUSERINFO lpUserInfo;
    LV_ITEM item;
    TCHAR   szType[100];


    //
    // Get the selected profile
    //

    hwndTemp = GetDlgItem (hDlg, IDC_UP_LISTVIEW);

    selection = GetSelectedItem (hwndTemp);

    if (selection == -1) {
        return;
    }

    item.mask = LVIF_PARAM;
    item.iItem = selection;
    item.iSubItem = 0;

    if (SendMessage (hwndTemp, LVM_GETITEM, 0, (LPARAM) &item)) {
        lpUserInfo = (LPUSERINFO) item.lParam;

    } else {
        lpUserInfo = NULL;
    }

    if (!lpUserInfo) {
        return;
    }

    //
    // Display the Change Type dialog
    //

    if (!DialogBoxParam (hInstance, MAKEINTRESOURCE(IDD_UP_TYPE), hDlg,
                         ChangeTypeDlgProc, (LPARAM)lpUserInfo)) {
        return;
    }


    //
    // Activate the Apply button
    //

    PropSheet_Changed(GetParent(hDlg), hDlg);


    //
    // Mark this item as 'dirty' so it will be saved
    //

    lpUserInfo->dwFlags |= USERINFO_FLAG_DIRTY;


    //
    // Fix the 'Type' field in the display
    //

    switch (lpUserInfo->dwProfileType) {
        case USERINFO_MANDATORY:
            iTypeID = IDS_UP_MANDATORY;
            break;

        case USERINFO_FLOATING:
        case USERINFO_LOCAL_SLOW_LINK:
            iTypeID = IDS_UP_FLOATING;
            break;

        default:
            iTypeID = IDS_UP_LOCAL;
            break;
    }


    LoadString (hInstance, iTypeID, szType, 100);

    item.mask = LVIF_TEXT;
    item.iItem = selection;
    item.iSubItem = 2;
    item.pszText = szType;

    SendMessage (hwndTemp, LVM_SETITEMTEXT, selection, (LPARAM) &item);

}

//*************************************************************
//
//  ChangeTypeDlgProc()
//
//  Purpose:    Dialog box procedure for changing the profile type
//
//  Parameters: hDlg    -   handle to the dialog box
//              uMsg    -   window message
//              wParam  -   wParam
//              lParam  -   lParam
//
//  Return:     TRUE if message was processed
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/9/96      ericflo    Created
//
//*************************************************************

BOOL APIENTRY ChangeTypeDlgProc (HWND hDlg, UINT uMsg,
                                 WPARAM wParam, LPARAM lParam)
{
    LPUSERINFO lpUserInfo;

    switch (uMsg) {
        case WM_INITDIALOG:
           lpUserInfo = (LPUSERINFO) lParam;

           if (!lpUserInfo) {
               EndDialog(hDlg, FALSE);
           }

           SetWindowLong (hDlg, GWL_USERDATA, (LONG) lpUserInfo);

           if (lpUserInfo->dwFlags & USERINFO_FLAG_CENTRAL_AVAILABLE) {

               if (lpUserInfo->dwProfileType == USERINFO_LOCAL) {
                   CheckRadioButton (hDlg, IDC_UPTYPE_LOCAL, IDC_UPTYPE_FLOAT,
                                     IDC_UPTYPE_LOCAL);
                   EnableWindow (GetDlgItem(hDlg, IDC_UPTYPE_SLOW), FALSE);
                   EnableWindow (GetDlgItem(hDlg, IDC_UPTYPE_SLOW_TEXT), FALSE);

               } else if (lpUserInfo->dwProfileType == USERINFO_FLOATING) {
                   CheckRadioButton (hDlg, IDC_UPTYPE_LOCAL, IDC_UPTYPE_FLOAT,
                                     IDC_UPTYPE_FLOAT);

               } else {
                   CheckRadioButton (hDlg, IDC_UPTYPE_LOCAL, IDC_UPTYPE_FLOAT,
                                     IDC_UPTYPE_FLOAT);
                   CheckDlgButton (hDlg, IDC_UPTYPE_SLOW, 1);
               }
           } else {

               CheckRadioButton (hDlg, IDC_UPTYPE_LOCAL, IDC_UPTYPE_FLOAT,
                                 IDC_UPTYPE_LOCAL);
               EnableWindow (GetDlgItem(hDlg, IDC_UPTYPE_FLOAT), FALSE);
               EnableWindow (GetDlgItem(hDlg, IDC_UPTYPE_SLOW), FALSE);
               EnableWindow (GetDlgItem(hDlg, IDC_UPTYPE_SLOW_TEXT), FALSE);
               EnableWindow (GetDlgItem(hDlg, IDOK), FALSE);

           }

           return TRUE;

        case WM_COMMAND:

          switch (LOWORD(wParam)) {
              case IDOK:

                  lpUserInfo = (LPUSERINFO) GetWindowLong(hDlg, GWL_USERDATA);

                  if (!lpUserInfo) {
                      EndDialog (hDlg, FALSE);
                      break;
                  }

                  //
                  // Determine what the user wants
                  //

                  if (IsDlgButtonChecked(hDlg, IDC_UPTYPE_LOCAL)) {
                      lpUserInfo->dwProfileType = USERINFO_LOCAL;
                  } else if (IsDlgButtonChecked(hDlg, IDC_UPTYPE_SLOW)) {
                      lpUserInfo->dwProfileType = USERINFO_LOCAL_SLOW_LINK;
                  } else {
                      lpUserInfo->dwProfileType = USERINFO_FLOATING;
                  }

                  EndDialog(hDlg, TRUE);
                  break;

              case IDCANCEL:
                  EndDialog(hDlg, FALSE);
                  break;

              case IDC_UPTYPE_LOCAL:
                  if (HIWORD(wParam) == BN_CLICKED) {
                      CheckDlgButton (hDlg, IDC_UPTYPE_SLOW, 0);
                      EnableWindow (GetDlgItem(hDlg, IDC_UPTYPE_SLOW), FALSE);
                      EnableWindow (GetDlgItem(hDlg, IDC_UPTYPE_SLOW_TEXT), FALSE);
                  }
                  break;

              case IDC_UPTYPE_FLOAT:
                  if (HIWORD(wParam) == BN_CLICKED) {
                      EnableWindow (GetDlgItem(hDlg, IDC_UPTYPE_SLOW), TRUE);
                      EnableWindow (GetDlgItem(hDlg, IDC_UPTYPE_SLOW_TEXT), TRUE);
                  }
                  break;

              default:
                  break;

          }
          break;

        case WM_HELP:      // F1
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
            (DWORD) (LPSTR) aUserProfileHelpIds);
            break;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
            (DWORD) (LPSTR) aUserProfileHelpIds);
            return (TRUE);

    }

    return (FALSE);
}

//*************************************************************
//
//  UPInitCopyDlg()
//
//  Purpose:    Initializes the copy profile dialog
//
//  Parameters: hDlg    -   Dialog box handle
//              lParam  -   lParam (lpUserInfo)
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/26/96     ericflo    Created
//
//*************************************************************

BOOL UPInitCopyDlg (HWND hDlg, LPARAM lParam)
{
    LPUSERINFO lpUserInfo;
    LPUPCOPYINFO lpUPCopyInfo;
    HKEY hKey;
    LONG lResult;
    TCHAR szBuffer[MAX_PATH];
    TCHAR szTemp[100];
    TCHAR szTemp2[100];
    DWORD dwTempSize = 100, dwTemp2Size = 100;
    PSID pSid;
    DWORD dwSize, dwType;
    SID_NAME_USE SidName;

    lpUserInfo = (LPUSERINFO) lParam;

    if (!lpUserInfo) {
        return FALSE;
    }

    //
    // Create a CopyInfo structure
    //

    lpUPCopyInfo = MemAlloc(LPTR, sizeof(UPCOPYINFO));

    if (!lpUPCopyInfo) {
        return FALSE;
    }

    lpUPCopyInfo->dwFlags = 0;
    lpUPCopyInfo->lpUserInfo = lpUserInfo;

    //
    // Get the user's sid
    //

    wsprintf (szBuffer, TEXT("%s\\%s"), PROFILE_MAPPING, lpUserInfo->lpSid);
    lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                            szBuffer,
                            0,
                            KEY_READ,
                            &hKey);

    if (lResult != ERROR_SUCCESS) {
        MemFree (lpUPCopyInfo);
        return FALSE;
    }

    //
    // Query for the sid size
    //

    dwSize = 0;
    lResult = RegQueryValueEx (hKey,
                               TEXT("Sid"),
                               NULL,
                               &dwType,
                               NULL,
                               &dwSize);

    if (lResult != ERROR_SUCCESS) {
        RegCloseKey (hKey);
        MemFree (lpUPCopyInfo);
        return FALSE;
    }


    //
    // Actually get the sid
    //

    pSid = MemAlloc (LPTR, dwSize);

    if (!pSid) {
        RegCloseKey (hKey);
        MemFree (lpUPCopyInfo);
        return FALSE;
    }

    lResult = RegQueryValueEx (hKey,
                               TEXT("Sid"),
                               NULL,
                               &dwType,
                               (LPBYTE) pSid,
                               &dwSize);

    if (lResult != ERROR_SUCCESS) {
        RegCloseKey (hKey);
        MemFree (pSid);
        MemFree (lpUPCopyInfo);
        return FALSE;
    }

    lpUPCopyInfo->pSid = pSid;

    RegCloseKey (hKey);


    //
    // Get the friendly name
    //

    if (!LookupAccountSid (NULL, pSid, szTemp, &dwTempSize,
                           szTemp2, &dwTemp2Size, &SidName)) {
        MemFree (pSid);
        MemFree (lpUPCopyInfo);
        return FALSE;
    }


    //
    // Display the friendly name in the edit control
    //

    wsprintf (szBuffer, TEXT("%s\\%s"), szTemp2, szTemp);
    SetDlgItemText (hDlg, IDC_COPY_USER, szBuffer);



    //
    // Save the copyinfo structure in the extra words
    //

    SetWindowLong (hDlg, GWL_USERDATA, (LONG) lpUPCopyInfo);
    EnableWindow (GetDlgItem(hDlg, IDOK), FALSE);

    return TRUE;
}

//*************************************************************
//
//  UPCopyProfile()
//
//  Purpose:    Displays the copy profile dialog box
//
//  Parameters: hDlg    -   Dialog box handle
//
//  Return:     void
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/13/96     ericflo    Created
//
//*************************************************************

void UPCopyProfile(HWND hDlg)
{
    int     selection, iTypeID;
    HWND    hwndTemp;
    LPUSERINFO lpUserInfo;
    LV_ITEM item;


    //
    // Get the selected profile
    //

    hwndTemp = GetDlgItem (hDlg, IDC_UP_LISTVIEW);

    selection = GetSelectedItem (hwndTemp);

    if (selection == -1) {
        return;
    }

    item.mask = LVIF_PARAM;
    item.iItem = selection;
    item.iSubItem = 0;

    if (SendMessage (hwndTemp, LVM_GETITEM, 0, (LPARAM) &item)) {
        lpUserInfo = (LPUSERINFO) item.lParam;

    } else {
        lpUserInfo = NULL;
    }

    if (!lpUserInfo) {
        return;
    }

    //
    // Display the Copy Profile dialog
    //

    if (!DialogBoxParam (hInstance, MAKEINTRESOURCE(IDD_UP_COPY), hDlg,
                         UPCopyDlgProc, (LPARAM)lpUserInfo)) {
        return;
    }
}


//*************************************************************
//
//  UPCopyDlgProc()
//
//  Purpose:    Dialog box procedure for copying a profile
//
//  Parameters: hDlg    -   handle to the dialog box
//              uMsg    -   window message
//              wParam  -   wParam
//              lParam  -   lParam
//
//  Return:     TRUE if message was processed
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/13/96     ericflo    Created
//
//*************************************************************

BOOL APIENTRY UPCopyDlgProc (HWND hDlg, UINT uMsg,
                             WPARAM wParam, LPARAM lParam)
{
    LPUPCOPYINFO lpUPCopyInfo;

    switch (uMsg) {
        case WM_INITDIALOG:

           if (!UPInitCopyDlg(hDlg, lParam)) {
               EndDialog(hDlg, FALSE);
           }
           return TRUE;

        case WM_COMMAND:

          switch (LOWORD(wParam)) {
              case IDOK:
                  {
                  TCHAR szDir[MAX_PATH];
                  TCHAR szTemp[MAX_PATH];
                  HCURSOR hOldCursor;

                  lpUPCopyInfo = (LPUPCOPYINFO) GetWindowLong(hDlg, GWL_USERDATA);

                  if (!lpUPCopyInfo) {
                      EndDialog (hDlg, FALSE);
                      break;
                  }

                  GetDlgItemText (hDlg, IDC_COPY_PATH, szTemp, MAX_PATH);
                  if (ExpandEnvironmentStrings (szTemp, szDir, MAX_PATH) > MAX_PATH) {
                     lstrcpy (szDir, szTemp);
                  }

                  hOldCursor = SetCursor (LoadCursor(NULL, IDC_WAIT));

                  if (!UPCreateProfile (lpUPCopyInfo, szDir, NULL)) {
                      SetCursor(hOldCursor);
                      UPDisplayErrorMessage(hDlg, GetLastError());
                      break;
                  }

                  MemFree (lpUPCopyInfo->pSid);
                  MemFree (lpUPCopyInfo);
                  SetCursor(hOldCursor);
                  EndDialog(hDlg, TRUE);
                  }
                  break;

              case IDCANCEL:
                  lpUPCopyInfo = (LPUPCOPYINFO) GetWindowLong(hDlg, GWL_USERDATA);

                  if (lpUPCopyInfo) {
                      MemFree (lpUPCopyInfo->pSid);
                      MemFree(lpUPCopyInfo);
                  }

                  EndDialog(hDlg, FALSE);
                  break;

              case IDC_COPY_BROWSE:
                  {
                  BROWSEINFO BrowseInfo;
                  TCHAR      szBuffer[MAX_PATH];
                  LPITEMIDLIST pidl;


                  LoadString(hInstance, IDS_UP_DIRPICK, szBuffer, MAX_PATH);

                  BrowseInfo.hwndOwner = hDlg;
                  BrowseInfo.pidlRoot = NULL;
                  BrowseInfo.pszDisplayName = szBuffer;
                  BrowseInfo.lpszTitle = szBuffer;
                  BrowseInfo.ulFlags = BIF_RETURNONLYFSDIRS;
                  BrowseInfo.lpfn = NULL;
                  BrowseInfo.lParam = 0;

                  pidl = SHBrowseForFolder (&BrowseInfo);

                  if (pidl) {
                     SHGetPathFromIDList(pidl, szBuffer);
                     SHFree (pidl);
                     SetDlgItemText (hDlg, IDC_COPY_PATH, szBuffer);
                     SetFocus (GetDlgItem(hDlg, IDOK));
                  }

                  }
                  break;

              case IDC_COPY_PATH:
                  if (HIWORD(wParam) == EN_UPDATE) {
                      if (SendDlgItemMessage(hDlg, IDC_COPY_PATH,
                                             EM_LINELENGTH, 0, 0)) {
                          EnableWindow (GetDlgItem(hDlg, IDOK), TRUE);
                      } else {
                          EnableWindow (GetDlgItem(hDlg, IDOK), FALSE);
                      }
                  }
                  break;

              case IDC_COPY_CHANGE:
                  {
                  USERBROWSER UserParms;
                  HUSERBROW hBrowse;
                  LPUSERDETAILS lpUserDetails;
                  DWORD dwSize = 1024;
                  WCHAR szTitle[100];
                  TCHAR szUserName[200];
                  PSID pNewSid;


                  lpUPCopyInfo = (LPUPCOPYINFO) GetWindowLong(hDlg, GWL_USERDATA);

                  if (!lpUPCopyInfo) {
                      EndDialog (hDlg, FALSE);
                      break;
                  }

                  lpUserDetails = MemAlloc (LPTR, dwSize);

                  if (!lpUserDetails) {
                      break;
                  }


                  LoadStringW (hInstance, IDS_UP_PICKUSER, szTitle, 100);

                  UserParms.ulStructSize = sizeof(USERBROWSER);
                  UserParms.fExpandNames = FALSE;
                  UserParms.hwndOwner = hDlg;
                  UserParms.pszTitle = szTitle;
                  UserParms.pszInitialDomain = NULL;
                  UserParms.ulHelpContext = IDH_USERPROFILE + 20;
                  UserParms.pszHelpFileName = L"SYSDM.HLP";
                  UserParms.Flags = USRBROWS_SINGLE_SELECT |
                                    USRBROWS_INCL_REMOTE_USERS |
                                    USRBROWS_INCL_INTERACTIVE |
                                    USRBROWS_INCL_EVERYONE |
                                    USRBROWS_SHOW_ALL;



                  //
                  // Display the dialog box
                  //

                  hBrowse = OpenUserBrowser (&UserParms);

                  if (!hBrowse) {
                      MemFree (lpUserDetails);
                      break;
                  }


                  //
                  // Get the user's selection
                  //

                  if (!EnumUserBrowserSelection(hBrowse, lpUserDetails, &dwSize)) {

                      if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
                          MemFree (lpUserDetails);
                          CloseUserBrowser(hBrowse);
                          break;
                      }

                      MemFree (lpUserDetails);
                      lpUserDetails = MemAlloc(LPTR, dwSize + 2);
                      if (!lpUserDetails) {
                          MemFree (lpUserDetails);
                          CloseUserBrowser(hBrowse);
                          break;
                      }

                      if (!EnumUserBrowserSelection(hBrowse, lpUserDetails, &dwSize)) {
                          MemFree(lpUserDetails);
                          CloseUserBrowser(hBrowse);
                          break;
                      }
                  }


                  //
                  // Duplicate the sid
                  //

                  dwSize = GetLengthSid (lpUserDetails->psidUser);

                  pNewSid = MemAlloc (LPTR, dwSize);

                  if (!pNewSid) {
                      MemFree(lpUserDetails);
                      CloseUserBrowser(hBrowse);
                      break;
                  }

                  if (!CopySid (dwSize, pNewSid, lpUserDetails->psidUser)) {
                      MemFree (pNewSid);
                      MemFree(lpUserDetails);
                      CloseUserBrowser(hBrowse);
                      break;
                  }

                  //
                  // Save our new sid
                  //

                  MemFree (lpUPCopyInfo->pSid);
                  lpUPCopyInfo->pSid = pNewSid;

                  //
                  // Update the edit control

                  lstrcpy(szUserName, lpUserDetails->pszDomainName);
                  lstrcat(szUserName, TEXT("\\"));
                  lstrcat(szUserName, lpUserDetails->pszAccountName);
                  SetDlgItemText (hDlg, IDC_COPY_USER, szUserName);


                  //
                  // Cleanup
                  //

                  CloseUserBrowser (hBrowse);

                  MemFree (lpUserDetails);
                  }
                  break;

              default:
                  break;

          }
          break;

        case WM_HELP:      // F1
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
            (DWORD) (LPSTR) aUserProfileHelpIds);
            break;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
            (DWORD) (LPSTR) aUserProfileHelpIds);
            return (TRUE);

    }

    return (FALSE);
}

//*************************************************************
//
//  UPCreateProfile()
//
//  Purpose:    Creates a copy of the specified profile with
//              the correct security.
//
//  Parameters: lpUPCopyInfo  -   Copy Dialog information
//              lpDest      -   Destination directory
//              pNewSecDesc -   New security descriptor
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/13/96     ericflo    Created
//
//*************************************************************

BOOL UPCreateProfile (LPUPCOPYINFO lpUPCopyInfo, LPTSTR lpDest,
                      PSECURITY_DESCRIPTOR pNewSecDesc)
{
    HKEY RootKey;
    SECURITY_DESCRIPTOR sd;
    SECURITY_ATTRIBUTES sa;
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    PACL pAcl = NULL;
    PSID  psidUser = NULL, psidSystem = NULL, psidAdmin = NULL;
    DWORD cbAcl, AceIndex;
    ACE_HEADER * lpAceHeader;
    HANDLE hFile;
    BOOL bMandatory = FALSE;
    TCHAR szTempPath[MAX_PATH];
    TCHAR szBuffer[MAX_PATH];
    TCHAR szHive[MAX_PATH];
    BOOL bRetVal = FALSE;
    HKEY hKeyProfile;
    LONG Error;
    LPTSTR lpEnd;


    if (!lpDest || !(*lpDest)) {
        return FALSE;
    }



    //
    // Create the security descriptor
    //

    //
    // User Sid
    //

    psidUser = lpUPCopyInfo->pSid;


    //
    // Get the system sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidSystem)) {
         goto Exit;
    }


    //
    // Get the admin sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, &psidAdmin)) {
         goto Exit;
    }


    //
    // Allocate space for the ACL
    //

    cbAcl = (2 * GetLengthSid (psidUser)) + (2 * GetLengthSid (psidSystem)) +
            (2 * GetLengthSid (psidAdmin)) + sizeof(ACL) +
            (6 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));


    pAcl = (PACL) MemAlloc(LPTR, cbAcl);
    if (!pAcl) {
        goto Exit;
    }


    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) {
        goto Exit;
    }



    //
    // Add Aces for User, System, and Admin.  Non-inheritable ACEs first
    //

    AceIndex = 0;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidUser)) {
        goto Exit;
    }


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidSystem)) {
        goto Exit;
    }

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidAdmin)) {
        goto Exit;
    }


    //
    // Now the inheritable ACEs
    //

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidUser)) {
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidSystem)) {
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidAdmin)) {
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    //
    // Put together the security descriptor
    //

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
        goto Exit;
    }


    if (!SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE)) {
        goto Exit;
    }

    //
    // Add the security descriptor to the sa structure
    //

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;


    //
    // Create the destination directory
    //

    if (!CreateNestedDirectory (lpDest, &sa)) {
        goto Exit;
    }


    //
    // Save/copy the user's profile to a temp file
    //

    if (!GetTempPath(MAX_PATH, szTempPath)) {
        goto Exit;
    }


    if (!GetTempFileName (szTempPath, TEXT("TMP"), 0, szBuffer)) {
        goto Exit;
    }

    DeleteFile (szBuffer);


    //
    // Determine if we are working with a mandatory profile
    //

    lstrcpy (szHive, lpUPCopyInfo->lpUserInfo->lpProfile);
    lpEnd = CheckSlash(szHive);
    lstrcpy (lpEnd, TEXT("ntuser.man"));

    hFile = CreateFile(szHive, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);


    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle (hFile);
        bMandatory = TRUE;
    }


    //
    // Test if the requested profile is in use.
    //

    if (IsProfileInUse (lpUPCopyInfo->lpUserInfo->lpSid)) {


        Error = RegOpenKeyEx (HKEY_USERS, lpUPCopyInfo->lpUserInfo->lpSid, 0,
                              KEY_READ, &hKeyProfile);

        if (Error != ERROR_SUCCESS) {
            goto Exit;
        }

        Error = MyRegSaveKey (hKeyProfile, szBuffer);

        RegCloseKey (hKeyProfile);

        if (Error != ERROR_SUCCESS) {
            DeleteFile (szBuffer);
            goto Exit;
        }

    } else {

       if (!bMandatory) {
           lstrcpy (lpEnd, TEXT("ntuser.dat"));
       }

       if (!CopyFile(szHive, szBuffer, FALSE)) {
           goto Exit;
       }
    }

    //
    // Apply security to the hive
    //

    Error = MyRegLoadKey (HKEY_USERS, TEMP_PROFILE, szBuffer);

    if (Error != ERROR_SUCCESS) {
        DeleteFile (szBuffer);
        goto Exit;
    }

    bRetVal = ApplyHiveSecurity(TEMP_PROFILE, psidUser);

    MyRegUnLoadKey(HKEY_USERS, TEMP_PROFILE);

    if (!bRetVal) {
        DeleteFile (szBuffer);
        lstrcat (szBuffer, TEXT(".log"));
        DeleteFile (szBuffer);
        goto Exit;
    }


    //
    // Copy the profile without the hive
    //

    bRetVal = CopyProfileDirectory (lpUPCopyInfo->lpUserInfo->lpProfile,
                                    lpDest,
                                    CPD_IGNOREHIVE |
                                    CPD_COPYIFDIFFERENT |
                                    CPD_SYNCHRONIZE);

    if (!bRetVal) {
        DeleteFile(szBuffer);
        lstrcat (szBuffer, TEXT(".log"));
        DeleteFile (szBuffer);
        goto Exit;
    }


    //
    // Now copy the hive
    //

    lstrcpy (szHive, lpDest);
    lpEnd = CheckSlash (szHive);
    if (bMandatory) {
        lstrcpy (lpEnd, TEXT("ntuser.man"));
    } else {
        lstrcpy (lpEnd, TEXT("ntuser.dat"));
    }

    bRetVal = CopyFile (szBuffer, szHive, FALSE);

    //
    // Delete the temp file (and log file)
    //

    DeleteFile (szBuffer);
    lstrcat (szBuffer, TEXT(".log"));
    DeleteFile (szBuffer);


Exit:

    //
    // Free the sids and acl
    //

    if (psidSystem) {
        FreeSid(psidSystem);
    }

    if (psidAdmin) {
        FreeSid(psidAdmin);
    }

    if (pAcl) {
        MemFree (pAcl);
    }


    return (bRetVal);
}

//*************************************************************
//
//  UPDisplayErrorMessage()
//
//  Purpose:    Display an error message
//
//  Parameters: hWnd            -   parent window handle
//              uiSystemError   -   Error code
//
//  Return:     void
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/14/96     ericflo    Created
//
//*************************************************************

VOID UPDisplayErrorMessage(HWND hWnd, UINT uiSystemError)
{
   TCHAR szMessage[MAX_PATH];
   TCHAR szTitle[100];

   //
   // retrieve the string matching the Win32 system error
   //

   FormatMessage(
            FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            uiSystemError,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
            szMessage,
            MAX_PATH,
            NULL);

   //
   // display a message box with this error
   //

   LoadString (hInstance, IDS_UP_ERRORTITLE, szTitle, 100);
   MessageBox(hWnd, szMessage, szTitle, MB_OK | MB_ICONSTOP);

   return;

}

//*************************************************************
//
//  ApplySecurityToRegistryTree()
//
//  Purpose:    Applies the passed security descriptor to the passed
//              key and all its descendants.  Only the parts of
//              the descriptor inddicated in the security
//              info value are actually applied to each registry key.
//
//  Parameters: RootKey   -     Registry key
//              pSD       -     Security Descriptor
//
//  Return:     ERROR_SUCCESS if successful
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/19/95     ericflo    Created
//
//*************************************************************

DWORD ApplySecurityToRegistryTree(HKEY RootKey, PSECURITY_DESCRIPTOR pSD)

{
    DWORD Error;
    DWORD SubKeyIndex;
    LPTSTR SubKeyName;
    HKEY SubKey;
    DWORD cchSubKeySize = MAX_PATH + 1;



    //
    // First apply security
    //

    RegSetKeySecurity(RootKey, DACL_SECURITY_INFORMATION, pSD);


    //
    // Open each sub-key and apply security to its sub-tree
    //

    SubKeyIndex = 0;

    SubKeyName = MemAlloc (LPTR, cchSubKeySize * sizeof(TCHAR));

    if (!SubKeyName) {
        return GetLastError();
    }

    while (TRUE) {

        //
        // Get the next sub-key name
        //

        Error = RegEnumKey(RootKey, SubKeyIndex, SubKeyName, cchSubKeySize);


        if (Error != ERROR_SUCCESS) {

            if (Error == ERROR_NO_MORE_ITEMS) {

                //
                // Successful end of enumeration
                //

                Error = ERROR_SUCCESS;

            }

            break;
        }


        //
        // Open the sub-key
        //

        Error = RegOpenKeyEx(RootKey,
                             SubKeyName,
                             0,
                             WRITE_DAC | KEY_ENUMERATE_SUB_KEYS | READ_CONTROL,
                             &SubKey);

        if (Error == ERROR_SUCCESS) {

            //
            // Apply security to the sub-tree
            //

            ApplySecurityToRegistryTree(SubKey, pSD);


            //
            // We're finished with the sub-key
            //

            RegCloseKey(SubKey);
        }


        //
        // Go enumerate the next sub-key
        //

        SubKeyIndex ++;
    }


    MemFree (SubKeyName);

    return Error;

}

//*************************************************************
//
//  ApplyHiveSecurity()
//
//  Purpose:    Initializes the new user hive created by copying
//              the default hive.
//
//  Parameters: lpHiveName      -   Name of hive in HKEY_USERS
//              pSid            -   Sid (used by CreateNewUser)
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/18/95     ericflo    Created
//
//*************************************************************

BOOL ApplyHiveSecurity(LPTSTR lpHiveName, PSID pSid)
{
    DWORD Error;
    HKEY RootKey;
    SECURITY_DESCRIPTOR sd;
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    PACL pAcl = NULL;
    PSID  psidUser = pSid, psidSystem = NULL, psidAdmin = NULL;
    DWORD cbAcl, AceIndex;
    ACE_HEADER * lpAceHeader;
    BOOL bRetVal = FALSE;



    //
    // Get the system sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidSystem)) {
         goto Exit;
    }


    //
    // Get the admin sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, &psidAdmin)) {
         goto Exit;
    }


    //
    // Allocate space for the ACL
    //

    cbAcl = (2 * GetLengthSid (psidUser)) + (2 * GetLengthSid (psidSystem)) +
            (2 * GetLengthSid (psidAdmin)) + sizeof(ACL) +
            (6 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));


    pAcl = (PACL) MemAlloc(LPTR, cbAcl);
    if (!pAcl) {
        goto Exit;
    }


    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) {
        goto Exit;
    }



    //
    // Add Aces for User, System, and Admin.  Non-inheritable ACEs first
    //

    AceIndex = 0;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidUser)) {
        goto Exit;
    }


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidSystem)) {
        goto Exit;
    }

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidAdmin)) {
        goto Exit;
    }


    //
    // Now the inheritable ACEs
    //

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidUser)) {
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidSystem)) {
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidAdmin)) {
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    //
    // Put together the security descriptor
    //

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
        goto Exit;
    }


    if (!SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE)) {
        goto Exit;
    }


    //
    // Open the root of the user's profile
    //

    Error = RegOpenKeyEx(HKEY_USERS,
                         lpHiveName,
                         0,
                         WRITE_DAC | KEY_ENUMERATE_SUB_KEYS | READ_CONTROL,
                         &RootKey);

    if (Error == ERROR_SUCCESS) {

        //
        // Set the security descriptor on the entire tree
        //

        Error = ApplySecurityToRegistryTree(RootKey, &sd);


        if (Error == ERROR_SUCCESS) {
            bRetVal = TRUE;
        }

        RegFlushKey (RootKey);

        RegCloseKey(RootKey);
    }


Exit:

    //
    // Free the sids and acl
    //

    if (psidSystem) {
        FreeSid(psidSystem);
    }

    if (psidAdmin) {
        FreeSid(psidAdmin);
    }

    if (pAcl) {
        MemFree (pAcl);
    }


    return(bRetVal);

}
