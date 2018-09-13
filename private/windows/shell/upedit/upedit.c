/****************************************************************************

    PROGRAM: UPEdit.c

    PURPOSE: User Profile Editor

             Allows an administrator to create profiles, enables editing
             of profile parameters which can not be set through the
             standard Windows tools. This includes the ability to Lock
             Program Manager Groupss and delete the file menu from the
             Program Manager...

    FUNCTIONS:

        main() - calls initialization function, processes message loop
        InitApplication() - initializes window data and global variables
        UPEDlgProc() - processes messages
    InitDialog() - initializes the dialog (main window)

    COMMENTS:

    MODIFICATION HISTORY
        Created:  4/10/92       JohanneC

****************************************************************************/

#include "upedit.h"
#include <dlgs.h>

/*
 * Globals
 */
HANDLE hInst;
HWND hwndUPE;
HICON  hIconOld = NULL;
DLGPROC lpfnUPEdlgProc = NULL;
HKEY hkeyCurrentUser;
HKEY hkeyProgramGroupsCurrent = NULL;
HKEY hkeyProgramManagerCurrent = NULL;
HKEY hkeyPMSettingsCurrent = NULL;
HKEY hkeyPMRestrictCurrent = NULL;
HKEY hkeyProgramGroups = NULL;
HKEY hkeyProgramManager = NULL;
HKEY hkeyPMSettings = NULL;
HKEY hkeyPMRestrict = NULL;
HANDLE hProgramGroupsEvent = NULL;
PSID pCurrentUserSid = NULL;
PSID pUserOrGroupSid = NULL;
BOOL bUserSelected = FALSE;
BOOL bWorkFromCurrent = TRUE;
BOOL bHelpDisplayed = FALSE;

UINT uiCommdlgHelpMessage;
UINT uiSaveSettingsMessage;
UINT uiHelpMessage;
DWORD dwHelpContext;
HHOOK hhkMsgFilter;
UINT   uMenuID      = 0;
HMENU  hMenu        = 0;
UINT   uMenuFlags   = 0;

BOOL bSettingsChanged = FALSE;
INT  iChangeGroupId;  // used to keep track of the button in the Allow User To
                      // group that is checked when the Remove FIle Menu is
                      // checked.

//
// Current users original settings.
// These need to be reset before exiting.
//
BOOL bNoRunOrg;
BOOL bNoSaveOrg;
BOOL bShowCommonGrpsOrg;
DWORD dwEditLevelOrg;
LPTSTR lpStartupGroupOrg = NULL;
BOOL bPrintNetworkOrg;
BOOL bFileManNetworkOrg;
BOOL bSyncLogonScriptOrg;

PSECURITY_DESCRIPTOR pInitialSecDesc = NULL;

TCHAR szProgramManager[]  = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Program Manager");
TCHAR szFileManager[]     = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\File Manager");
TCHAR szPrintManager[]    = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Print Manager");
TCHAR szWinlogon[]        = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon");
TCHAR szWinfileINI[]      = TEXT("winfile.ini");
TCHAR szSettings[]    = TEXT("Settings");
TCHAR szStartup[]     = TEXT("startup");
TCHAR szNoRun[]       = TEXT("NoRun");
TCHAR szNoSave[]      = TEXT("NoSaveSettings");
TCHAR szNoClose[]     = TEXT("NoClose");
TCHAR szEditLevel[]   = TEXT("EditLevel");
TCHAR szShowCommonGrps[]   = TEXT("ShowCommonGroups");
TCHAR szRestrict[]    = TEXT("Restrictions");
TCHAR szINIFile[]     = TEXT("PROGMAN.INI");
TCHAR szUPEHelp[]     = TEXT("UPEDIT.HLP");
TCHAR szNetwork[]     = TEXT("Network");
TCHAR szSyncLogonScript[] = TEXT("RunLogonScriptSync");
TCHAR szNULL[]        = TEXT("");

TCHAR szUPETitle[(MAXMESSAGELEN + 1)*sizeof(TCHAR)];
TCHAR szUPETitleFile[(MAXMESSAGELEN + 1)*sizeof(TCHAR)];
TCHAR szNone[16*sizeof(TCHAR)];
TCHAR szExit[(MAXMESSAGELEN + 1)*sizeof(TCHAR)];
TCHAR szSaveSettings[(MAXMESSAGELEN + 1)*sizeof(TCHAR)];
TCHAR szDefExt[(MAXMESSAGELEN + 1)*sizeof(TCHAR)];
TCHAR szProgramGroups[(MAXKEYLEN + 1)*sizeof(TCHAR)];   // registry key for groups
TCHAR szSaveFileMError[(MAXMESSAGELEN + 1)*sizeof(TCHAR)];
TCHAR szSavePrintMError[(MAXMESSAGELEN + 1)*sizeof(TCHAR)];
TCHAR szPrivilegeError[(MAXMESSAGELEN + 1)*sizeof(TCHAR)];
TCHAR szMessage[(MAXMESSAGELEN + 1)*sizeof(TCHAR)];
TCHAR szCurrentUserName[MAX_PATH*sizeof(TCHAR)];

BOOL GetPermittedUser(HWND hwnd, PSID *pSid);

/****************************************************************************
 *
 *   FUNCTION: main()
 *
 *   PURPOSE: calls initialization function, processes message loop
 *
 *   COMMENTS:
 *
 *
 ****************************************************************************/

int __cdecl main(
    int argc,
    char *argv[],
    char *envp[])
{
    MSG       msg;


    hInst =  GetModuleHandle(NULL);

    if (!InitApplication(hInst))
        return (FALSE);

    while (GetMessage(&msg,     NULL, 0, 0)) {
        if (msg.message == WM_SYSKEYDOWN && msg.wParam == VK_RETURN &&
                IsIconic(hwndUPE)) {
            ShowWindow(hwndUPE, SW_NORMAL);

        } else {
            if (hwndUPE == NULL || !IsDialogMessage(hwndUPE, &msg)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
            }
        }
    }
    return (int)(msg.wParam);
}


/****************************************************************************
 *
 *   FUNCTION: InitApplication(HANDLE, HANDLE, LPTSR, INT)
 *
 *   PURPOSE: Creates the apps window
 *
 *   COMMENTS:
 *
 *
 ****************************************************************************/

BOOL InitApplication(HANDLE hInst)
{
    LoadString(hInst, IDS_UPETITLE, szUPETitle, sizeof(szUPETitle));
    LoadString(hInst, IDS_UPETITLE1, szUPETitleFile, sizeof(szUPETitleFile));
    LoadString(hInst, IDS_PROGRAMGROUPS, szProgramGroups, sizeof(szProgramGroups));
    LoadString(hInst, IDS_SAVESETTINGS, szSaveSettings, sizeof(szSaveSettings));
    LoadString(hInst, IDS_EXIT, szExit, sizeof(szExit));
    LoadString(hInst, IDS_DEFEXT, szDefExt, sizeof(szDefExt));
    LoadString(hInst, IDS_SAVEFILEMERROR, szSaveFileMError, sizeof(szSaveFileMError));
    LoadString(hInst, IDS_SAVEPRINTMERROR, szSavePrintMError, sizeof(szSavePrintMError));
    LoadString(hInst, IDS_PRIVILEGEERROR, szPrivilegeError, sizeof(szPrivilegeError));

    //
    // Set initial working current user key.
    //
    hkeyCurrentUser = HKEY_CURRENT_USER;
    GetCurrentUserSid(&pCurrentUserSid);

    //
    // Initialize global Admin and System sids.
    //
    InitializeGlobalSids();


    //
    // Register message for commdlg help.
    //
    uiCommdlgHelpMessage = RegisterWindowMessage(HELPMSGSTRING);
    //
    // Register message for saving Progman's internal settings.
    //
    uiSaveSettingsMessage = RegisterWindowMessage(TEXT("SaveSettings"));
    //
    // Register shell help  message.
    //
    uiHelpMessage = RegisterWindowMessage(TEXT("UpeditHelp"));

    hhkMsgFilter = SetWindowsHook(WH_MSGFILTER, (HOOKPROC)MessageFilter);

    lpfnUPEdlgProc = (DLGPROC) MakeProcInstance(UPEDlgProc, hInst);
    hwndUPE = CreateDialog(hInst,
                        (LPTSTR) MAKEINTRESOURCE(IDD_UPEDLG),
                        NULL,
                        lpfnUPEdlgProc);

    /* If window could not be created, return "failure" */

    if (!hwndUPE)
        return (FALSE);

    CentreWindow(hwndUPE);

    /* Make the window visible; update its client area; and return "success" */

    ShowWindow(hwndUPE, SW_SHOWNORMAL);  /* Show the window                        */
    UpdateWindow(hwndUPE);          /* Sends WM_PAINT message                 */
    return (TRUE);               /* Returns the value from PostQuitMessage */

}

/****************************************************************************
 *
 *   FUNCTION: GetGroupName(HKEY, LPTSTR, LPTSTR)
 *
 *   PURPOSE: Gets the group name from the group data. The group structure
 *            used in Progman is needed to extract the name from the group.
 *            The name may differ from the group key name since the a registry
 *            key name is more restrictive than the group name.
 *
 *   COMMENTS:
 *
 *
 ****************************************************************************/
void GetGroupName(HKEY hkeyGroups, LPTSTR lpSubKey, LPTSTR lpGroupName)
{
    HKEY hKey;
    DWORD cbMaxValueLen = 0;
    DWORD dummy;
    TCHAR szClass[64];
    FILETIME ft;
    LPGROUPDEF lpgd = NULL;

    //
    // Try to open the group key.
    //
    if (RegOpenKeyEx(hkeyGroups, lpSubKey, 0, KEY_READ, &hKey)) {
        // can't open the group key, so forget about the group name.
        return;
    }

    RegQueryInfoKey(hKey,
                    szClass,
                    &dummy,   // cbClass
                    NULL,     // Title index
                    &dummy,   // cbSubKeys
                    &dummy,   // cb Max subkey length
                    &dummy,   // max class len
                    &dummy,   // values count
                    &dummy,   // max value name length
                    &cbMaxValueLen,
                    &dummy,   // cb Security Descriptor
                    &ft);

    //
    // Find the size of the file by seeking to the end.
    //
    if (cbMaxValueLen < sizeof(GROUPDEF)) {
        goto Error;
    }

    //
    // Allocate some memory for the thing.
    //
    lpgd = (LPGROUPDEF)GlobalAlloc(GPTR, cbMaxValueLen);
    if (!lpgd) {
        goto Error;
    }

    //
    // Read the whole group data into memory.
    //
    if (RegQueryValueEx(hKey, NULL, 0, 0, (LPBYTE)lpgd, &cbMaxValueLen)) {
        goto Error;
    }

    //
    // The group data is all Ansi (not UNICODE). So be sure to do the right
    // thing for UNICODE version of upedit.exe
    //
    // The groups in product 1.0a are now in UNICODE - Johannec 8/26/93
    //
    if (lpgd->dwMagic == GROUP_UNICODE) {
        lstrcpy(lpGroupName, (LPTSTR)((PBYTE)lpgd + lpgd->pName));
    }
    else if (lpgd->dwMagic == GROUP_MAGIC){
        //
        // the group is of format NT 1.0 and therefore is Ansi
        //
        LPSTR lpName;
        UNICODE_STRING usName;
        ANSI_STRING asName;
        LPGROUPDEF_A lpgdA;

        lpgdA = (LPGROUPDEF_A)lpgd;

        lpName = (LPSTR)((PBYTE)lpgdA + (WORD)(lpgdA->pName));

        //
        // First convert lpName to unicode (if we are UNICODE)
        //
        RtlInitAnsiString(&asName, lpName);
        RtlAnsiStringToUnicodeString(&usName, &asName, TRUE);

        lstrcpy(lpGroupName, usName.Buffer);

        RtlFreeUnicodeString(&usName);
    }

Error:
    RegCloseKey(hKey);
    if (lpgd) {
        GlobalFree(lpgd);
    }

}

/****************************************************************************
 *
 *   FUNCTION: InitGroupLists(HANDLE)
 *
 *   PURPOSE: Initializes the the program groups list boxes (unlocked and
 *            locked). We keeping the original Locked status of each group
 *            and the group key name in the GROUPDATE structure.
 *
 *   COMMENTS:
 *
 *
 ****************************************************************************/

void InitGroupLists(HWND hwnd)
{
    INT i = 0;
    INT index;
    HWND hwndUnlocked;
    HWND hwndLocked;
    HWND hwndStartup;
    TCHAR szGroupName[(MAXKEYLEN+1)*sizeof(TCHAR)];
    TCHAR szSubKey[(MAXKEYLEN+1)*sizeof(TCHAR)];
    DWORD cbSubKey = (MAXKEYLEN + 1)*sizeof(TCHAR);
    LPTSTR lpGroupName = NULL;
    FILETIME ft;
    LPGROUPDATA lpGroupData;

    if (bWorkFromCurrent) {
        hkeyProgramGroups = hkeyProgramGroupsCurrent;
    }
    else if (RegOpenKeyEx(hkeyCurrentUser, szProgramGroups, 0,
                       KEY_READ | KEY_WRITE, &hkeyProgramGroups)) {
        //
        // Try openning the key for the ANSI groups.
        //
        if (RegOpenKeyEx(hkeyCurrentUser, TEXT("Program Groups"), 0,
                       KEY_READ | KEY_WRITE, &hkeyProgramGroups))
            hkeyProgramGroups = NULL;
    }

    if (hkeyProgramGroups) {

        /* list the groups */
        hwndUnlocked = GetDlgItem(hwnd, IDD_UNLOCKEDGRPS);
        hwndLocked = GetDlgItem(hwnd, IDD_LOCKEDGRPS);
        hwndStartup = GetDlgItem(hwnd, IDD_STARTUP);
        while (!RegEnumKeyEx(hkeyProgramGroups, i, szSubKey, &cbSubKey, 0, 0, 0, &ft)) {
            if (cbSubKey) {
                //
                // Get the group's name which may differ from the group's
                // key name.
                //
                GetGroupName(hkeyProgramGroups, szSubKey, szGroupName);
                if (!*szGroupName) {
                    //
                    // cannot get the group name so skip this group.
                    //
                    goto GetNextGroup;
                }

                if (!(lpGroupData = (LPGROUPDATA)GlobalAlloc(GPTR, sizeof(GROUPDATA))))
                    goto GetNextGroup;

                if (lpGroupData->lpGroupKey = (LPTSTR) GlobalAlloc(GPTR,
                                     (lstrlen(szSubKey) + 1) * sizeof(TCHAR))) {
                    lstrcpy(lpGroupData->lpGroupKey, szSubKey);
                }

                //
                // Need to get the subkey's security to find out
                // if group goes in Unlocked or Locked list box.
                //
                if (lpGroupData->bOrgLock = IsGroupLocked(szSubKey)) {
                    if ((index = (INT)SendMessage(hwndLocked, LB_ADDSTRING, 0, (LPARAM)szGroupName)) != LB_ERR)
                        SendMessage(hwndLocked, LB_SETITEMDATA, index, (LPARAM)lpGroupData);
                }
                else {
                    if ((index = (INT)SendMessage(hwndUnlocked, LB_ADDSTRING, 0, (LPARAM)szGroupName)) != LB_ERR)
                        SendMessage(hwndUnlocked, LB_SETITEMDATA, index, (LPARAM)lpGroupData);
                }
                SendMessage(hwndStartup, CB_ADDSTRING, 0, (LPARAM)szGroupName);
            }
GetNextGroup:
            i++;
            cbSubKey = MAXKEYLEN + 1;
        }
    }
}

/****************************************************************************
 *
 *   FUNCTION: FreeGroupData
 *
 *   PURPOSE: Free any allocated memory for group information in the groups
 *            lists boxes. And delete the entries in the Startup combobox.
 *
 ****************************************************************************/
void FreeGroupData(HWND hwnd)
{
    int i, iCount;
    HWND hwndStartup = GetDlgItem(hwnd, IDD_STARTUP);
    HWND hwndUnlocked = GetDlgItem(hwnd, IDD_UNLOCKEDGRPS);
    HWND hwndLocked = GetDlgItem(hwnd, IDD_LOCKEDGRPS);
    LPGROUPDATA lpGroupData;

    iCount = (int)SendMessage(hwndUnlocked, LB_GETCOUNT, 0, 0);
    for (i = 0; i < iCount; i++) {
        lpGroupData = (LPGROUPDATA)SendMessage(hwndUnlocked, LB_GETITEMDATA, 0, 0);
        if (lpGroupData) {
            if (lpGroupData->lpGroupKey) {
                GlobalFree(lpGroupData->lpGroupKey);
            }
            GlobalFree(lpGroupData);
        }
        SendMessage(hwndUnlocked, LB_DELETESTRING, 0, 0);
    }

    iCount = (int)SendMessage(hwndLocked, LB_GETCOUNT, 0, 0);
    for (i = 0; i < iCount; i++) {
        lpGroupData = (LPGROUPDATA)SendMessage(hwndLocked, LB_GETITEMDATA, 0, 0);
        if (lpGroupData) {
            if (lpGroupData->lpGroupKey) {
                GlobalFree(lpGroupData->lpGroupKey);
            }
            GlobalFree(lpGroupData);
        }
        SendMessage(hwndLocked, LB_DELETESTRING, 0, 0);
    }

    iCount = (int)SendMessage(hwndStartup, CB_GETCOUNT, 0, 0);
    for (i = 0; i < iCount; i++) {
        SendMessage(hwndStartup, CB_DELETESTRING, 0, 0);
    }

}

/****************************************************************************
 *
 *   FUNCTION: InitializeUPESettings
 *
 *   PURPOSE: Initializes all the settings in the UPE dialog using the new opened
 *            user profile ot the current profile.
 *
 ****************************************************************************/
BOOL InitializeUPESettings(HWND hwnd, BOOL bInitOrgSettings)
{
    INT  i = 0;
    TCHAR szStartupGroup[(MAXKEYLEN+1)*sizeof(TCHAR)] = TEXT("");
    BOOL bNoSave = FALSE;
    BOOL bNoRun = FALSE;
    BOOL bShowCommonGrps = TRUE;
    DWORD dwEditLevel = 0;
    HWND hwndStartup;
    HWND hwndEditLevel = GetDlgItem(hwnd, IDD_EDITLEVEL);
    HWND hwndUnlocked = GetDlgItem(hwnd, IDD_UNLOCKEDGRPS);
    HWND hwndLocked = GetDlgItem(hwnd, IDD_LOCKEDGRPS);
    DWORD cbData;
    HKEY hkeyPrintManager = NULL;
    HKEY hkeyFileManager = NULL;
    HKEY hkeyFMSettings = NULL;
    HKEY hkeyWinlogon = NULL;
    BOOL bNetwork = TRUE;
    BOOL bStartupValuePresent = TRUE;
    BOOL bSyncLogonScript = FALSE;

    //
    // First free global memory for all group data information, before
    // reading in new group info.
    //
    FreeGroupData(hwnd);


    /*
     * Create/Open the registry key corresponding to progman.ini to access
     * the settings. Only open if we have just open a new profile
     */
    if (bWorkFromCurrent) {
        hkeyProgramManager = hkeyProgramManagerCurrent;
        hkeyPMSettings = hkeyPMSettingsCurrent;
        hkeyPMRestrict = hkeyPMRestrictCurrent;
    }
    else if (!RegCreateKeyEx(hkeyCurrentUser, szProgramManager, 0, szProgramManager, 0,
                         KEY_READ | KEY_WRITE,
                         0, &hkeyProgramManager, NULL)) {
        RegCreateKeyEx(hkeyProgramManager, szSettings, 0, szProgramManager, 0,
                         KEY_READ | KEY_WRITE,
                         0, &hkeyPMSettings, NULL);

        RegCreateKeyEx(hkeyProgramManager, szRestrict, 0, szProgramManager, 0,
                         KEY_READ | KEY_WRITE,
                         0, &hkeyPMRestrict, NULL);

    }

    /* Initialize startup group. */
    hwndStartup = GetDlgItem(hwnd, IDD_STARTUP);
    LoadString(hInst, IDS_NONE, szNone, sizeof(szNone));
    SendMessage(hwndStartup, CB_ADDSTRING, 0, (LPARAM)szNone);
    if (hkeyPMSettings) {
        cbData = sizeof(szStartupGroup);
        if (RegQueryValueEx(hkeyPMSettings, szStartup, 0, 0,
                    (LPBYTE)szStartupGroup, &cbData) != ERROR_SUCCESS) {
            bStartupValuePresent = FALSE;
        }
    }
    else {
        *szStartupGroup = (TCHAR)0;
    }

    InitGroupLists(hwnd);

    /*
     * Set the startup group.
     */
    if (SendMessage(hwndStartup, CB_SELECTSTRING, (WPARAM)-1,
                                 (LPARAM)(LPTSTR)szStartupGroup) == CB_ERR) {
        /* set the startup group to (none). */
        SendMessage(hwndStartup, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)(LPTSTR)szNone);
    }

    /* when starting nothing in the list boxes is selected, disable the Lock
     * and Unlock buttons.
     */
    EnableWindow(GetDlgItem(hwnd, IDD_LOCK), FALSE);
    EnableWindow(GetDlgItem(hwnd, IDD_UNLOCK), FALSE);

    /* query registry for the different settings.
     */
    if (hkeyPMRestrict) {
        cbData = sizeof(bNoRun);
        RegQueryValueEx(hkeyPMRestrict, szNoRun, 0, 0, (LPBYTE)&bNoRun, &cbData);
        cbData = sizeof(bNoSave);
        RegQueryValueEx(hkeyPMRestrict, szNoSave, 0, 0, (LPBYTE)&bNoSave, &cbData);
        cbData = sizeof(dwEditLevel);
        RegQueryValueEx(hkeyPMRestrict, szEditLevel, 0, 0, (LPBYTE)&dwEditLevel, &cbData);
        cbData = sizeof(bShowCommonGrps);
        RegQueryValueEx(hkeyPMRestrict, szShowCommonGrps, 0, 0, (LPBYTE)&bShowCommonGrps, &cbData);
    }

    CheckDlgButton(hwnd, IDD_NORUN, bNoRun);
    CheckDlgButton(hwnd, IDD_NOSAVE, bNoSave);
    CheckDlgButton(hwnd, IDD_SHOWCOMMONGRPS, bShowCommonGrps);

    if (!SendMessage(hwndLocked, LB_GETCOUNT, 0, 0)) {
        EnableWindow(hwndLocked, FALSE);
    }
    else {
        EnableWindow(hwndLocked, TRUE);
    }

    //
    // Set the EditLevel.
    //
    SendMessage(hwndEditLevel, CB_SETCURSEL, dwEditLevel, 0);

    if (!SendMessage(hwndUnlocked, LB_GETCOUNT, 0, 0)) {
        /*
         * Disable the listbox window.
         */
        EnableWindow(hwndUnlocked, FALSE);
    }
    else {
        EnableWindow(hwndUnlocked, TRUE);
    }

    /*
     * Create/Open the registry key corresponding to winfile.ini to access
     * the settings.
     */
    if (!RegCreateKeyEx(hkeyCurrentUser, szFileManager, 0, szFileManager, 0,
                         KEY_READ | KEY_WRITE,
                         0, &hkeyFileManager, NULL)) {
        if (!RegCreateKeyEx(hkeyFileManager, szSettings, 0, szFileManager, 0,
                         KEY_READ,
                         0, &hkeyFMSettings, NULL)){
            cbData = sizeof(bNetwork);
            RegQueryValueEx(hkeyFMSettings, szNetwork, 0, 0, (LPBYTE)&bNetwork, &cbData);
            RegCloseKey(hkeyFMSettings);
        }
        RegCloseKey(hkeyFileManager);
    }

    CheckDlgButton(hwnd, IDD_NETFILEMGR, bNetwork);

    /*
     * Create/Open the registry key corresponding to spawning the
     * logon script synchronously vs asynchronously.
     */
    if (!RegCreateKeyEx(hkeyCurrentUser, szWinlogon, 0, NULL, 0,
                         KEY_READ | KEY_WRITE, 0, &hkeyWinlogon, NULL)) {
        cbData = sizeof(bSyncLogonScript);
        RegQueryValueEx(hkeyWinlogon, szSyncLogonScript, 0, 0,
                        (LPBYTE)&bSyncLogonScript, &cbData);
        RegCloseKey(hkeyWinlogon);
    }

    CheckDlgButton(hwnd, IDD_SYNCLOGONSCRIPT, bSyncLogonScript);

    /*
     * Create/Open the registry key corresponding to Print Manager setting for
     * printer network connections.
     */
    if (!RegCreateKeyEx(hkeyCurrentUser, szPrintManager, 0, szPrintManager, 0,
                         KEY_READ | KEY_WRITE,
                         0, &hkeyPrintManager, NULL)) {
        cbData = sizeof(bNetwork);
        RegQueryValueEx(hkeyPrintManager, szNetwork, 0, 0, (LPBYTE)&bNetwork, &cbData);
        RegCloseKey(hkeyPrintManager);
    }
    CheckDlgButton(hwnd, IDD_NETPRINTMGR, bNetwork);

    if (bInitOrgSettings) {
        if (!bStartupValuePresent) {
            lpStartupGroupOrg = NULL;
         }
         else {
            lpStartupGroupOrg = (LPTSTR)LocalAlloc(LPTR, (lstrlen(szStartupGroup)+1) * sizeof(TCHAR));
            if (lpStartupGroupOrg) {
                lstrcpy(lpStartupGroupOrg, szStartupGroup);
            }
        }
        bNoRunOrg = bNoRun;
        bNoSaveOrg = bNoSave;
        bShowCommonGrpsOrg = bShowCommonGrps;
        dwEditLevelOrg = dwEditLevel;
        bFileManNetworkOrg = bNetwork;
        bPrintNetworkOrg = bNetwork;
        bSyncLogonScriptOrg = bSyncLogonScript;
    }
    return(TRUE);
}


VOID SaveProgmanInternalSettings()
{
    TCHAR szProgmanClass[] = TEXT("Progman");
    HWND hwndProgman;

    if (!bWorkFromCurrent) {
        //
        // Since we are not working on the current profile, we are not
        // chnaging the user's profile groups.
        //
        return;
    }

    if (hwndProgman = FindWindow(szProgmanClass, NULL)) {
        SendMessage(hwndProgman, uiSaveSettingsMessage, 0, 0);
    }
}



/****************************************************************************
 *
 *   FUNCTION: SaveUPESettingsToRegistry()
 *
 *   PURPOSE: Sets the profile settings using the dialog control selections.
 *            Returns false if an error occured and the user hit cancel to
 *            to error message box.
 *
 ****************************************************************************/
BOOL SaveUPESettingsToRegistry()
{
    INT   i;
    TCHAR szGroupName[(MAXGROUPNAMELEN + 1)*sizeof(TCHAR)];
    TCHAR szDefStartup[(MAXGROUPNAMELEN + 1)*sizeof(TCHAR)];
    TCHAR szTitle[MAXTITLELEN * sizeof(TCHAR)];
    DWORD dwEditLevel;
    BOOL  bNoSave;
    BOOL  bNoRun;
    BOOL  bNetwork;
    BOOL  bShowCommonGrps;
    BOOL  bSyncLogonScript;
    HKEY  hkeyPrintManager;
    HKEY  hkeyFileManager;
    HKEY  hkeyFMSettings;
    HKEY  hkeyWinlogon;
    HWND  hwnd;

    hwnd = hwndUPE;

    if (bWorkFromCurrent) {
        SaveProgmanInternalSettings();
    }

    /*
     * Set the EditLevel.
     */
    dwEditLevel = (DWORD)SendDlgItemMessage(hwnd, IDD_EDITLEVEL, CB_GETCURSEL, 0, 0);

    bNoRun = IsDlgButtonChecked(hwnd, IDD_NORUN);
    bNoSave = IsDlgButtonChecked(hwnd, IDD_NOSAVE);
    bShowCommonGrps = IsDlgButtonChecked(hwnd, IDD_SHOWCOMMONGRPS);

    if (hkeyPMRestrict) {
        RegSetValueEx(hkeyPMRestrict, szEditLevel, 0, REG_DWORD,
                                   (LPBYTE)&dwEditLevel, sizeof(dwEditLevel));
        RegSetValueEx(hkeyPMRestrict, szNoRun, 0, REG_DWORD,
                                   (LPBYTE)&bNoRun, sizeof(bNoRun));
        RegSetValueEx(hkeyPMRestrict, szNoSave, 0, REG_DWORD,
                                   (LPBYTE)&bNoSave, sizeof(bNoSave));
        RegSetValueEx(hkeyPMRestrict, szShowCommonGrps, 0, REG_DWORD,
                                   (LPBYTE)&bShowCommonGrps, sizeof(bShowCommonGrps));

    }

    /*
     * Get the startup group.
     */
    if ((i = (INT)SendDlgItemMessage(hwnd, IDD_STARTUP, CB_GETCURSEL, 0, 0)) >= 0) {
        SendDlgItemMessage(hwnd, IDD_STARTUP, CB_GETLBTEXT, i, (LPARAM)szGroupName);
        if (lstrcmpi(szNone, szGroupName)) {
            if (hkeyPMSettings) {
                RegSetValueEx( hkeyPMSettings, szStartup, 0, REG_SZ,
                              (LPBYTE)szGroupName,
                              (lstrlen(szGroupName) + 1) * sizeof(TCHAR) );
            }
        }
        else {
            //
            // '(none)' is selected.
            // Warn the administrator that '(none)' will default to the
            // group titled 'startup' if such a group exists
            //
            LoadString(hInst, IDS_STARTUPGRP, szDefStartup, sizeof(szDefStartup));
            i = (INT)SendDlgItemMessage(hwnd, IDD_STARTUP,
                                         CB_FINDSTRINGEXACT, (WPARAM)-1,
                                         (LPARAM)szDefStartup);
            if (i != CB_ERR) {
                // we found a group titled 'startup'
                LoadString(hInst, IDS_SAVE, szTitle, sizeof(szTitle));
                LoadString(hInst, IDS_STARTUPNONE, szMessage, sizeof(szMessage));
                if (MessageBox(hwndUPE, szMessage, szTitle,
                              MB_ICONEXCLAMATION | MB_OKCANCEL) == IDCANCEL) {
                    return(FALSE);
                }
            }

            if (hkeyPMSettings)
                RegDeleteValue(hkeyPMSettings, szStartup);
        }
    }

    /*
     * Set the net connections settings.
     */

    bNetwork = IsDlgButtonChecked(hwnd, IDD_NETFILEMGR);
    /*
     * Create/Open the registry key corresponding to winfile.ini to access
     * the settings.
     */
    if (!RegCreateKeyEx(hkeyCurrentUser, szFileManager, 0, szFileManager, 0,
                         KEY_READ | KEY_WRITE,
                         0, &hkeyFileManager, NULL)) {
        if (!RegCreateKeyEx(hkeyFileManager, szSettings, 0, szFileManager, 0,
                         KEY_READ | KEY_WRITE,
                         0, &hkeyFMSettings, NULL)) {
            RegSetValueEx(hkeyFMSettings, szNetwork, 0, REG_DWORD,
                          (LPBYTE)&bNetwork, sizeof(bNetwork));
            RegCloseKey(hkeyFMSettings);
        }
        RegCloseKey(hkeyFileManager);
    }

    bNetwork = IsDlgButtonChecked(hwnd, IDD_NETPRINTMGR);
    /*
     * Create/Open the registry key corresponding to Print Manager seeting for
     * printer network connections.
     */
    if (!RegCreateKeyEx(hkeyCurrentUser, szPrintManager, 0, szPrintManager, 0,
                         KEY_READ | KEY_WRITE,
                         0, &hkeyPrintManager, NULL)) {
        RegSetValueEx(hkeyPrintManager, szNetwork, 0, REG_DWORD,
                          (LPBYTE)&bNetwork, sizeof(bNetwork));
        RegCloseKey(hkeyPrintManager);
    }

    bSyncLogonScript = IsDlgButtonChecked(hwnd, IDD_SYNCLOGONSCRIPT);

    /*
     * Create/Open the registry key corresponding to synchronous vs
     * asynchronous logon script
     */
    if (!RegCreateKeyEx(hkeyCurrentUser, szWinlogon, 0, NULL, 0,
                         KEY_READ | KEY_WRITE,
                         0, &hkeyWinlogon, NULL)) {
        RegSetValueEx(hkeyWinlogon, szSyncLogonScript, 0, REG_DWORD,
                          (LPBYTE)&bSyncLogonScript, sizeof(bSyncLogonScript));
        RegCloseKey(hkeyWinlogon);
    }
    return(TRUE);
}


/****************************************************************************
 *
 *   FUNCTION: InitDialog(HWND)
 *
 *   PURPOSE: Initializes the dialog controls with the settings from
 *            the current user's profile.
 *
 *   COMMENTS:
 *
 *
 ****************************************************************************/

BOOL PASCAL InitDialog(HWND hwnd)
{
    HWND hwndEditLevel = GetDlgItem(hwnd, IDD_EDITLEVEL);
    TCHAR szEditLevelTmp[64*sizeof(TCHAR)];
    int i;

    /*
     * Verify if the user  has the privilege to save the profile i.e.
     * SeBackupPrivilege
     */
    if (!EnablePrivilege(SE_BACKUP_PRIVILEGE, TRUE) ||
                  !EnablePrivilege(SE_RESTORE_PRIVILEGE, TRUE)) {
        MessageBox(hwnd, szPrivilegeError, szUPETitle,
                   MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
        return(FALSE);
    }

    /*
     * Create/Open the registry key corresponding to progman.ini to access
     * the settings.
     */
    if (!RegCreateKeyEx(hkeyCurrentUser, szProgramManager, 0, szProgramManager, 0,
                         KEY_READ | KEY_WRITE,
                         0, &hkeyProgramManagerCurrent, NULL)) {
        RegCreateKeyEx(hkeyProgramManagerCurrent, szSettings, 0, szProgramManager, 0,
                         KEY_READ | KEY_WRITE,
                         0, &hkeyPMSettingsCurrent, NULL);

        RegCreateKeyEx(hkeyProgramManagerCurrent, szRestrict, 0, szProgramManager, 0,
                         KEY_READ | KEY_WRITE,
                         0, &hkeyPMRestrictCurrent, NULL);

    }

    RegOpenKeyEx(hkeyCurrentUser, szProgramGroups, 0,
                       KEY_READ | KEY_WRITE, &hkeyProgramGroupsCurrent);
    if (!hkeyProgramGroupsCurrent) {
        //
        // Try the ANSI groups
        //
        RegOpenKeyEx(hkeyCurrentUser, TEXT("Program Groups"), 0,
                       KEY_READ | KEY_WRITE, &hkeyProgramGroupsCurrent);
    }


    /*
     * Create the ProgramGroups key event, so we know when program
     * groups in the Program Manager are deleted or added. This will only be
     * used for the Current Profile. No updates will be shown when another
     * profile is openned.
     */
    if (hkeyProgramGroupsCurrent) {
        hProgramGroupsEvent = CreateProgramGroupsEvent();
        RegNotifyChangeKeyValue(hkeyProgramGroupsCurrent, TRUE, REG_NOTIFY_CHANGE_NAME,
                                hProgramGroupsEvent, TRUE);
    }

    /*
     * We set the class long here so that when we
     * are minimized we use the UPE icon.
     */
    hIconOld = (HICON)GetClassLongPtr(hwnd, GCLP_HICON);
    SetClassLongPtr(hwnd, GCLP_HICON, (LONG_PTR)LoadIcon(hInst, (LPTSTR) MAKEINTRESOURCE(IDI_UPEICON)));

    //
    // Initialize the edit level combobox
    //
    for (i = IDS_EDITLEVEL3; i >= IDS_EDITLEVEL0; i--){
        if (LoadString(hInst, i, szEditLevelTmp, sizeof(szEditLevelTmp)))
            SendMessage(hwndEditLevel, CB_INSERTSTRING, 0, (LPARAM)szEditLevelTmp);
    }

    InitializeUPESettings(hwnd, TRUE);

    GetCurrentProfileSecurityDescriptor(&pInitialSecDesc);

    if (GetPermittedUser(hwnd, &pUserOrGroupSid)) {
        bUserSelected = TRUE;
    }
    else if (GetPermittedUser(hwnd, &pCurrentUserSid)) {
        pUserOrGroupSid = pCurrentUserSid;
        bUserSelected = TRUE;
    }

    GetDlgItemText(hwnd, IDD_USEDBY, szCurrentUserName, sizeof(szCurrentUserName));

    return(TRUE);
}

/****************************************************************************
 *
 *   FUNCTION: IsGroupLocked(LPTSTR)
 *
 *   PURPOSE: Returns TRUE is the group is locked.
 *
 ****************************************************************************/

BOOL APIENTRY IsGroupLocked(LPTSTR lpGroupName)
{

    UNICODE_STRING UnicodeName;

    RtlInitUnicodeString(&UnicodeName, lpGroupName);

    return( IsKeyUserAdminWriteableOnly( hkeyProgramGroups, &UnicodeName ) );
}

/****************************************************************************
 *
 *   FUNCTION: LockUnlockGroups(HWND, INT, INT)
 *
 *   PURPOSE: Transfers selected groups from the Locked listbox to the
 *            Unlocked listbox or from Unlocked to Locked listboxes.
 *
 ****************************************************************************/

BOOL PASCAL LockUnlockGroups(HWND hwnd, INT FromId, INT ToId)
{
    TCHAR szGroupName[(MAXGROUPNAMELEN + 1)*sizeof(TCHAR)];
    LPINT lpiSelected;
    INT   iSelCount;
    INT   i;
    INT   iDelCnt;
    INT   iIndex;
    INT   iNewIndex;
    LPGROUPDATA lpGroupData;
    BOOL  bRet = FALSE;

    if (iSelCount = (INT)SendDlgItemMessage(hwnd, FromId, LB_GETSELCOUNT, 0, 0)) {
        lpiSelected = (LPINT)LocalAlloc(LPTR,
                                     iSelCount * sizeof(INT));
        SendDlgItemMessage(hwnd, FromId, LB_GETSELITEMS,
                                            iSelCount, (LPARAM)lpiSelected);
        bRet = TRUE;
    }
    for (i = 0, iDelCnt = 0; i < iSelCount; i++, iDelCnt++) {
        iIndex = lpiSelected[i] - iDelCnt;
        SendDlgItemMessage(hwnd, FromId, LB_GETTEXT, iIndex, (LPARAM)szGroupName);
        lpGroupData = (LPGROUPDATA)SendDlgItemMessage(hwnd, FromId, LB_GETITEMDATA, iIndex, 0);
        iNewIndex = (INT)SendDlgItemMessage(hwnd, ToId, LB_ADDSTRING, 0, (LPARAM)szGroupName);
        SendDlgItemMessage(hwnd, ToId, LB_SETITEMDATA, iNewIndex, (LPARAM)lpGroupData);
        SendDlgItemMessage(hwnd, ToId, LB_SETSEL, (WPARAM)TRUE, (LPARAM)iNewIndex);
        SendDlgItemMessage(hwnd, FromId, LB_DELETESTRING, iIndex, 0);
    }
    if (lpiSelected) {
        LocalFree(lpiSelected);
    }
    return(bRet);
}



/****************************************************************************
 *
 *   FUNCTION: ResetCurrentUserSettings
 *
 *   PURPOSE: Resets the current users settings to the original settings
 *            when he/she logged on. Called just before exiting.
 *
 *
 ****************************************************************************/
VOID ResetCurrentUserSettings(HWND hwnd)
{

    TCHAR szTitle[(MAXMESSAGELEN + 1)*sizeof(TCHAR)];
    HKEY hkeyPrintManager = NULL;
    HKEY hkeyFileManager = NULL;
    HKEY  hkeyFMSettings = NULL;
    HKEY  hkeyWinlogon = NULL;
    NTSTATUS Status;

    /*
     * Reset the Progman's settings.
     */
    if (hkeyPMRestrict) {
        RegSetValueEx(hkeyPMRestrict, szEditLevel, 0, REG_DWORD,
                                   (LPBYTE)&dwEditLevelOrg, sizeof(dwEditLevelOrg));
        RegSetValueEx(hkeyPMRestrict, szNoRun, 0, REG_DWORD,
                                   (LPBYTE)&bNoRunOrg, sizeof(bNoRunOrg));
        RegSetValueEx(hkeyPMRestrict, szNoSave, 0, REG_DWORD,
                                   (LPBYTE)&bNoSaveOrg, sizeof(bNoSaveOrg));
        RegSetValueEx(hkeyPMRestrict, szShowCommonGrps, 0, REG_DWORD,
                                   (LPBYTE)&bShowCommonGrpsOrg, sizeof(bShowCommonGrpsOrg));
    }

    /*
     * Reset the startup group.
     */
    if (hkeyPMSettings) {
        if (lpStartupGroupOrg) {
            RegSetValueEx(hkeyPMSettings, szStartup, 0, REG_SZ,
                           (LPBYTE)lpStartupGroupOrg, (lstrlen(lpStartupGroupOrg)+1) * sizeof(TCHAR));
            LocalFree(lpStartupGroupOrg);
            lpStartupGroupOrg = NULL;
        }
        else {
            RegDeleteValue(hkeyPMSettings, szStartup);
        }
    }

    /*
     * Reset the File Manager's network setting.
     */
    if (!RegCreateKeyEx(hkeyCurrentUser, szFileManager, 0, szFileManager, 0,
                         KEY_READ | KEY_WRITE,
                         0, &hkeyFileManager, NULL)) {
        if (!RegCreateKeyEx(hkeyFileManager, szSettings, 0, szFileManager, 0,
                         KEY_READ | KEY_WRITE,
                         0, &hkeyFMSettings, NULL)) {
            RegSetValueEx(hkeyFMSettings, szNetwork, 0, REG_DWORD,
                          (LPBYTE)&bFileManNetworkOrg, sizeof(bFileManNetworkOrg));
            RegCloseKey(hkeyFMSettings);
        }
        RegCloseKey(hkeyFileManager);
    }

    /*
     * Reset the Print Manager's network setting.
     */
    if (!RegCreateKeyEx(hkeyCurrentUser, szPrintManager, 0, szPrintManager, 0,
                         KEY_READ | KEY_WRITE,
                         0, &hkeyPrintManager, NULL)) {
        RegSetValueEx(hkeyPrintManager, szNetwork, 0, REG_DWORD,
                          (LPBYTE)&bPrintNetworkOrg, sizeof(bPrintNetworkOrg));
        RegCloseKey(hkeyPrintManager);
    }

    /*
     * Reset the synchronous logon on script setting.
     */
    if (!RegCreateKeyEx(hkeyCurrentUser, szWinlogon, 0, NULL, 0,
                         KEY_READ | KEY_WRITE,
                         0, &hkeyWinlogon, NULL)) {
        RegSetValueEx(hkeyWinlogon, szSyncLogonScript, 0, REG_DWORD,
                          (LPBYTE)&bSyncLogonScriptOrg, sizeof(bSyncLogonScriptOrg));
        RegCloseKey(hkeyWinlogon);
    }
    RegFlushKey(hkeyCurrentUser);

    //
    // Reset current user protection.
    //
    Status = ApplyProfileProtection(pInitialSecDesc, pCurrentUserSid, hkeyCurrentUser);
    if (Status) {
        LoadString(hInst, IDS_OPENTITLE, szTitle, sizeof(szTitle));
        LoadString(hInst, IDS_RESETPROTECTIONFAILED, szMessage, sizeof(szMessage));
        MessageBox(hwndUPE, szMessage, szTitle, MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    //
    // Reset the locked groups.
    //
    LockGroups(TRUE);
}

// Turn hourglass on or off

VOID HourGlass (BOOL bOn)
{
    if (!GetSystemMetrics (SM_MOUSEPRESENT))
        ShowCursor (bOn);

    SetCursor (LoadCursor (NULL, bOn ? IDC_WAIT : IDC_ARROW));
}

VOID UpeHelp(HWND hwnd, LPTSTR lpHelpFile, DWORD dwHelpCommand, DWORD_PTR dwHelpContext)
{
    TCHAR szHelpError[(MAXMESSAGELEN + 1)*sizeof(TCHAR)];

    if (!WinHelp(hwnd, lpHelpFile, dwHelpCommand, dwHelpContext)) {
        if (LoadString(hInst, IDS_HELPERROR, szHelpError, sizeof(szHelpError)))
            MessageBox(hwnd, szHelpError, szUPETitle,
                               MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
    } else {
        bHelpDisplayed = TRUE;
    }
}

HWND GetRealParent(HWND hwnd)
{
   // run up the parent chain until you find a hwnd
   // that doesn't have WS_CHILD set

   while (GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD)
      hwnd = (HWND)GetWindowLongPtr(hwnd, GWLP_HWNDPARENT);

   return hwnd;
}

/****************************************************************************
 *
 *   FUNCTION: UPEDlgProc(HWND, UINT, WPARAM, LPARAM)
 *
 *   PURPOSE: Dilaog Proc
 *
 *   COMMENTS:
 *
 *
 ****************************************************************************/

LONG APIENTRY UPEDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LONG lParam)
{
    INT ret;
    INT i;
    UINT uiHelpCmd;
    BOOL bResetSettings = TRUE;
    TCHAR szBuffer1[256];
    TCHAR szBuffer2[256];

    switch (msg) {
    case WM_INITDIALOG:
        if (!InitDialog(hwnd)) {
           bResetSettings = FALSE;
           goto Exit;
        }
        LoadString(hInst, IDS_COPYCURRENT, szBuffer1, sizeof(szBuffer1));
        wsprintf(szBuffer2, szUPETitleFile, szBuffer1);
        SetWindowText(hwnd, szBuffer2);
        //return(FALSE);
        break;

    case WM_ENDSESSION:
    case WM_CLOSE:
        goto Exit;

    case WM_SETCURSOR:
    case WM_ACTIVATEAPP:
        if (HasProgramGroupsKeyChanged()) {
            HandleProgramGroupsKeyChange(hwnd);
        }
        return(FALSE);

    case WM_MENUSELECT:

        if (GET_WM_MENUSELECT_HMENU(wParam, lParam)) {

            // Save the menu the user selected
            uMenuID = GET_WM_MENUSELECT_CMD(wParam, lParam);
            uMenuFlags = GET_WM_MENUSELECT_FLAGS(wParam, lParam);
            hMenu = GET_WM_MENUSELECT_HMENU(wParam, lParam);
        }

        break;

    case WM_COMMAND:
        switch(LOWORD(wParam)) {
        /* can not have selection in both list boxes at the same time. */

        case IDD_BROWSER:
            {
            TCHAR szUserOrGroupName[MAX_PATH*sizeof(TCHAR)];

            //
            // Call Browser api to get the username of usergroup
            //
            if (GetUserOrGroup(&pUserOrGroupSid, szUserOrGroupName, sizeof(szUserOrGroupName))) {
                //
                // Display selected name in edit field
                //
                SetDlgItemText(hwnd, IDD_USEDBY, (LPTSTR)szUserOrGroupName);
                bUserSelected = TRUE;
            }
            }

            break;

        case IDD_LOCKEDGRPS:
            if (GET_WM_COMMAND_CMD(wParam, lParam) == LBN_SELCHANGE) {
                if (SendDlgItemMessage(hwnd, IDD_UNLOCKEDGRPS, LB_GETSELCOUNT, 0, 0)) {
                    i = (INT)SendDlgItemMessage(hwnd, IDD_UNLOCKEDGRPS, LB_GETCOUNT, 0, 0);
                    SendDlgItemMessage(hwnd, IDD_UNLOCKEDGRPS, LB_SELITEMRANGE, 0, MAKELONG(0, i-1));
                    EnableWindow(GetDlgItem(hwnd, IDD_LOCK), FALSE);
                }
                if (!(i = (INT)SendMessage(GET_WM_COMMAND_HWND(wParam, lParam),
                                       LB_GETSELCOUNT, 0, 0))) {
                    EnableWindow(GetDlgItem(hwnd, IDD_UNLOCK), FALSE);
                }
                else if (i == 1) {
                    EnableWindow(GetDlgItem(hwnd, IDD_UNLOCK), TRUE);
                }
            }
            break;

        case IDD_UNLOCKEDGRPS:
            if (GET_WM_COMMAND_CMD(wParam, lParam) == LBN_SELCHANGE) {
                if (SendDlgItemMessage(hwnd, IDD_LOCKEDGRPS, LB_GETSELCOUNT, 0, 0)) {
                    i = (INT)SendDlgItemMessage(hwnd, IDD_LOCKEDGRPS, LB_GETCOUNT, 0, 0);
                    SendDlgItemMessage(hwnd, IDD_LOCKEDGRPS, LB_SELITEMRANGE, 0, MAKELONG(0, i-1));
                    EnableWindow(GetDlgItem(hwnd, IDD_UNLOCK), FALSE);
                }
                if (!(i = (INT)SendMessage(GET_WM_COMMAND_HWND(wParam, lParam),
                                       LB_GETSELCOUNT, 0, 0))) {
                    EnableWindow(GetDlgItem(hwnd, IDD_LOCK), FALSE);
                }
                else if (i == 1) {
                    EnableWindow(GetDlgItem(hwnd, IDD_LOCK), TRUE);
                }
            }
            break;

        case IDD_LOCK:
            if (LockUnlockGroups(hwnd, IDD_UNLOCKEDGRPS, IDD_LOCKEDGRPS)) {
                bSettingsChanged = TRUE;
                EnableWindow(GetDlgItem(hwnd, IDD_LOCKEDGRPS), TRUE);
                EnableWindow(GetDlgItem(hwnd, IDD_LOCK), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDD_UNLOCK), TRUE);
                SetFocus(GetDlgItem(hwnd, IDD_LOCKEDGRPS));
            }
            if (!SendDlgItemMessage(hwnd, IDD_UNLOCKEDGRPS, LB_GETCOUNT, 0, 0)) {

                /*
                 * Disable list box window.
                 */
                EnableWindow(GetDlgItem(hwnd, IDD_UNLOCKEDGRPS), FALSE);
            }
            break;

        case IDD_UNLOCK:
            if (LockUnlockGroups(hwnd, IDD_LOCKEDGRPS, IDD_UNLOCKEDGRPS)) {
                bSettingsChanged = TRUE;
                EnableWindow(GetDlgItem(hwnd, IDD_UNLOCKEDGRPS), TRUE);
                EnableWindow(GetDlgItem(hwnd, IDD_LOCK), TRUE);
                EnableWindow(GetDlgItem(hwnd, IDD_UNLOCK), FALSE);
                SetFocus(GetDlgItem(hwnd, IDD_UNLOCKEDGRPS));
            }
            if (!SendDlgItemMessage(hwnd, IDD_LOCKEDGRPS, LB_GETCOUNT, 0, 0)) {
                /*
                 * Disable list box window.
                 */
                EnableWindow(GetDlgItem(hwnd, IDD_LOCKEDGRPS), FALSE);
            }
            break;

        case IDD_NORUN:
        case IDD_NOSAVE:
        case IDD_SHOWCOMMONGRPS:
        case IDD_NETPRINTMGR:
        case IDD_NETFILEMGR:
        case IDD_SYNCLOGONSCRIPT:
            bSettingsChanged = TRUE;
            break;

        case IDD_EDITLEVEL:
          {
            static LONG Selection;

            if (HIWORD(wParam) == CBN_DROPDOWN) {
                Selection = (LONG)SendMessage(hwnd, CB_GETCURSEL, 0, 0);
            }
            if (HIWORD(wParam) == CBN_SELCHANGE &&
                Selection == SendMessage(hwnd, CB_GETCURSEL, 0, 0) ) {
                bSettingsChanged = TRUE;
            }
            break;
          }
        case IDD_STARTUP:
          {
            static LONG Selection;

            if (HIWORD(wParam) == CBN_DROPDOWN) {
                Selection = (LONG)SendMessage(hwnd, CB_GETCURSEL, 0, 0);
            }
            if (HIWORD(wParam) == CBN_SELCHANGE &&
                Selection == SendMessage(hwnd, CB_GETCURSEL, 0, 0) ) {
                bSettingsChanged = TRUE;
            }
            break;
          }

        case IDM_NEW:
          {
            DWORD BytesRequired;
            NTSTATUS Status;

            if (bSettingsChanged) {
                LoadString(hInst, IDS_OPENWARNING, szBuffer1, sizeof(szBuffer1));
                LoadString(hInst, IDS_OPENWARNINGMSG, szBuffer2, sizeof(szBuffer2));
                if (MessageBox(hwnd, szBuffer2, szBuffer1, MB_OKCANCEL | MB_ICONQUESTION) == IDCANCEL) {
                    break;
                }
            }
            if (!bWorkFromCurrent) {
                bWorkFromCurrent = TRUE;
                EnableMenuItem(GetMenu(hwnd), IDM_SAVECURRENT, MF_BYCOMMAND|MF_ENABLED);
                LoadString(hInst, IDS_COPYCURRENT, szBuffer1, sizeof(szBuffer1));
                wsprintf(szBuffer2, szUPETitleFile, szBuffer1);
                SetWindowText(hwnd, szBuffer2);

                ClearTempUserProfile();
            }
            else {
                ResetCurrentUserSettings(hwnd);
            }

            InitializeUPESettings(hwnd, FALSE);

            //
            // copy current user sid and reset user Permitted to Use field
            // with current user domain and name.
            //
            BytesRequired = RtlLengthSid(pCurrentUserSid);
            if (pUserOrGroupSid) {
                pUserOrGroupSid = (PSID)LocalReAlloc(pUserOrGroupSid, BytesRequired, LMEM_MOVEABLE);
            }
            else {
                pUserOrGroupSid = (PSID)LocalAlloc(LPTR, BytesRequired);
            }
            Status = RtlCopySid(BytesRequired, pUserOrGroupSid, pCurrentUserSid);
            if (!NT_SUCCESS(Status)) {
                LocalFree(pUserOrGroupSid);
                pUserOrGroupSid = NULL;
            }
            SetDlgItemText(hwnd, IDD_USEDBY, szCurrentUserName);
            SetFocus(GetDlgItem(hwnd, IDD_USEDBY));

            bSettingsChanged = FALSE;

            break;
          }

        case IDM_OPEN:
            {
                TCHAR szFilePath[MAX_PATH];

                if (bSettingsChanged) {
                    LoadString(hInst, IDS_OPENWARNING, szBuffer1, sizeof(szBuffer1));
                    LoadString(hInst, IDS_OPENWARNINGMSG, szBuffer2, sizeof(szBuffer2));
                    if (MessageBox(hwnd, szBuffer2, szBuffer1, MB_OKCANCEL | MB_ICONQUESTION) == IDCANCEL) {
                        break;
                    }
                }
                dwHelpContext = IDH_OPENDLG;
                if (!GetProfileName(szFilePath, MAX_PATH, TRUE)) {
                    break;
                }

                HourGlass(TRUE);

                if (bWorkFromCurrent) {
                    ResetCurrentUserSettings(hwndUPE);
                }
                else {
                    ClearTempUserProfile();
                }

                if (!OpenUserProfile(szFilePath, &pUserOrGroupSid)) {
                    if (!bWorkFromCurrent) {
                        bWorkFromCurrent = TRUE;
                        EnableMenuItem(GetMenu(hwnd), IDM_SAVECURRENT, MF_BYCOMMAND|MF_ENABLED);
                        LoadString(hInst, IDS_COPYCURRENT, szBuffer1, sizeof(szBuffer1));
                        wsprintf(szBuffer2, szUPETitleFile, szBuffer1);
                        SetWindowText(hwnd, szBuffer2);
                        InitializeUPESettings(hwnd, FALSE);
                        SetDlgItemText(hwnd, IDD_USEDBY, szCurrentUserName);
                        SetFocus(GetDlgItem(hwnd, IDD_USEDBY));
                    }
                    HourGlass(FALSE);
                    break;
                }
                if (pUserOrGroupSid) {
                    bUserSelected = TRUE;
                }
                bWorkFromCurrent = FALSE;
                EnableMenuItem(GetMenu(hwnd), IDM_SAVECURRENT, MF_BYCOMMAND|MF_GRAYED|MF_DISABLED);
                //
                // Change title so that the file name is displayed
                //
                if (*szFilePath) {
                    wsprintf(szBuffer2, szUPETitleFile, szFilePath);
                    SetWindowText(hwnd, szBuffer2);
                }

                InitializeUPESettings(hwnd, FALSE);
                bSettingsChanged = FALSE;

                HourGlass(FALSE);
            }
            break;

        case IDM_SAVECURRENT:
            HourGlass(TRUE);
            if (SaveCurrentProfile(pInitialSecDesc, pCurrentUserSid))
                bSettingsChanged = FALSE;

            HourGlass(FALSE);
            break;

        case IDM_SAVEDEFAULT:
            HourGlass(TRUE);
            if (SaveDefaultProfile())
                bSettingsChanged = FALSE;

            HourGlass(FALSE);
            break;

        case IDM_SAVESYSTEM:
            HourGlass(TRUE);
            if (SaveSystemProfile())
                bSettingsChanged = FALSE;

            HourGlass(FALSE);
            break;

        case IDM_SAVEAS:
          {
            TCHAR szFilePath[MAX_PATH*sizeof(TCHAR)];

            /* call GetOpenFileName from commdlg */
            if (!bUserSelected) {
                //
                // Must have selected a user or a group for proper security
                //
                LoadString(hInst, IDS_SELECTUSER, szBuffer1, sizeof(szBuffer1));
                MessageBox(hwndUPE, szBuffer1, szUPETitle,
                           MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
                SetFocus(GetDlgItem(hwnd, IDD_BROWSER));
                break;
            }

            dwHelpContext = IDH_SAVEASDLG;

            if (!GetProfileName(szFilePath, sizeof(szFilePath), FALSE)) {
                break;
            }

            HourGlass(TRUE);

            if (SaveUserProfile(pUserOrGroupSid, szFilePath)) {
                bSettingsChanged = FALSE;
            }

            HourGlass(FALSE);
            break;
          }

        case IDM_HELPINDEX:
            uiHelpCmd = HELP_INDEX;
            goto CallHelp;

        case IDM_HELPSEARCH:
            uiHelpCmd = HELP_PARTIALKEY;
            goto CallHelp;

        case IDM_HELPHELP:
            uiHelpCmd = HELP_HELPONHELP;

CallHelp:

            UpeHelp(hwnd, szUPEHelp, uiHelpCmd, (DWORD_PTR)(LPTSTR)szNULL);
            break;

        case IDM_ABOUT:
            ShellAbout(hwnd, szUPETitle, NULL, (HICON)GetClassLongPtr(hwnd, GCLP_HICON));
            break;

        case IDCANCEL:
            break;

        case IDM_EXIT:
Exit:
            if (bSettingsChanged) {

                if ((ret = MessageBox(hwnd, szSaveSettings, szExit,
                             MB_YESNO | MB_ICONQUESTION)) == IDNO)
                     break;
            }

            //
            // Take care of any temporary loaded user profile.
            //
            if (bWorkFromCurrent) {
                if (bResetSettings) {
                    ResetCurrentUserSettings(hwndUPE);
                }
            }
            else {
                ClearTempUserProfile();
            }

            if (pCurrentUserSid) {
                GlobalFree(pCurrentUserSid);
            }

            if (hIconOld) {
                SetClassLongPtr(hwnd, GCLP_HICON, (LONG_PTR)hIconOld);
            }
            if (hProgramGroupsEvent) {
                CloseHandle(hProgramGroupsEvent);
                hProgramGroupsEvent = NULL;
            }

            //
            // Close registry keys for Current profile.
            //
            if (bWorkFromCurrent) {
                if (hkeyPMSettingsCurrent) {
                    RegCloseKey(hkeyPMSettingsCurrent);
                }
                if (hkeyPMRestrictCurrent) {
                    RegCloseKey(hkeyPMRestrictCurrent);
                }
                if (hkeyProgramManagerCurrent) {
                    RegCloseKey(hkeyProgramManagerCurrent);
                }
                if (hkeyProgramGroupsCurrent) {
                    RegCloseKey(hkeyProgramGroupsCurrent);
                }
            }

            DestroyWindow(hwnd);
            if (lpfnUPEdlgProc) {
                FreeProcInstance(lpfnUPEdlgProc);
            }
            break;

        default:
            return(FALSE);
        }

        break;

    case WM_DESTROY:
        if (bHelpDisplayed) {
            WinHelp(hwnd, szUPEHelp, HELP_QUIT, 0);
        }

        PostQuitMessage(0);
        break;

    case WM_DELETEITEM:
    {
        #define lpdis ((LPDELETEITEMSTRUCT)lParam)
        LPGROUPDATA lpGroupData;

        if (wParam == IDD_LOCKEDGRPS || wParam == IDD_UNLOCKEDGRPS) {
            if (lpGroupData = (LPGROUPDATA)lpdis->itemData) {
                if (lpGroupData->lpGroupKey) {
                    GlobalFree(lpGroupData->lpGroupKey);
                }
                GlobalFree(lpGroupData);
            }
        }
        break;
    }

    default:
        if (msg == uiCommdlgHelpMessage) {
            UpeHelp(hwnd, szUPEHelp, HELP_CONTEXT, dwHelpContext);
        }
        else if (msg == uiHelpMessage) {

            if (wParam == MSGF_MENU) {

                // Get outta menu mode if help for a menu item

                if (uMenuID && hMenu) {
                   UINT m = uMenuID;       // save
                   HMENU hM = hMenu;
                   UINT  mf = uMenuFlags;

                   SendMessage(hwnd, WM_CANCELMODE, 0, 0L);

                   uMenuID   = m;          // restore
                   hMenu = hM;
                   uMenuFlags = mf;
                }

                if (!(uMenuFlags & MF_POPUP)) {

                    if (uMenuFlags & MF_SYSMENU)
                        dwHelpContext = IDH_SYSMENU;
                    else
                        dwHelpContext = uMenuID + IDH_HELPFIRST;

                    UpeHelp(hwnd, szUPEHelp, HELP_CONTEXT, dwHelpContext);
                }

            }
            else if (wParam == MSGF_DIALOGBOX) {
                HWND hwndParent;

                hwndParent = GetRealParent((HWND)lParam);
                if (hwndParent == hwndUPE) {
                    //
                    // F1 was hit from the Upedit main dialog window
                    //
                    UpeHelp(hwnd, szUPEHelp, HELP_INDEX, 0);
                }
                else {
                    if (dwHelpContext == IDH_OPENDLG || dwHelpContext == IDH_SAVEASDLG)
                        //
                        // F1 was hit from either the Open dialog or the save dialog.
                        //
                        UpeHelp(hwnd, szUPEHelp, HELP_CONTEXT, dwHelpContext);
                    else
                        PostMessage(hwndParent, uiHelpMessage, 0, 0L);
                }
            }

        }
        else {
            return(FALSE);
        }
    }

    return(TRUE);
}

/****************************************************************************
 *
 *   FUNCTION: MessageFilter(INT, WPARAM, LPMSG
 *
 *   PURPOSE: Filters help messages
 *
 *   COMMENTS:
 *
 *   History:
 *   02-11-93  Johannec     Created
 *
 ****************************************************************************/

INT MessageFilter(INT nCode, WPARAM wParam, LPMSG lpMsg)
{
   if (nCode < 0)
      goto DefHook;

   if (nCode == MSGF_MENU) {

      if (lpMsg->message == WM_KEYDOWN && lpMsg->wParam == VK_F1) {
         PostMessage(hwndUPE, uiHelpMessage, MSGF_MENU, (LPARAM)lpMsg->hwnd);
         return 1;
      }

   }
   else if (nCode == MSGF_DIALOGBOX) {

      if (lpMsg->message == WM_KEYDOWN && lpMsg->wParam == VK_F1) {
         PostMessage(hwndUPE, uiHelpMessage, MSGF_DIALOGBOX, (LPARAM)lpMsg->hwnd);
         return 1;
      }

   } else

DefHook:
      return (INT)DefHookProc(nCode, wParam, (DWORD_PTR)lpMsg, &hhkMsgFilter);

  return 0;
}
