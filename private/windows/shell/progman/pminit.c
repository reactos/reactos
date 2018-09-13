/*
 * pminit.c - program manager
 *
 *  Copyright (c) 1991,  Microsoft Corporation
 *
 *  DESCRIPTION
 *
 *                This file is for support of program manager under NT Windows.
 *                This file is/was ported from pminit.c (program manager).
 *
 *  MODIFICATION HISTORY
 *      Initial Version: x/x/90        Author Unknown, since he didn't feel
 *                                                                like commenting the code...
 *
 *      NT 32b Version:        1/25/91        Jeff Pack
 *                                                                Intitial port to begin.
 *
 *
 */

#include "progman.h"
#include "util.h"
#include "commdlg.h"
#include <winuserp.h>
//#ifdef FE_IME
#include "winnls32.h"
//#endif
#include "uniconv.h"
#include "security.h"

#define MAX_USERNAME_LENGTH 256
#define PROGMAN_KEY  L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Program Manager"
#define WINDOWS_KEY  L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows"

//#define MYDEBUG 1
SECURITY_ATTRIBUTES SecurityAttributes;
SECURITY_ATTRIBUTES AdminSecAttr;       // security attributes for common groups

BOOL bInitialArrange;
BOOL bInNtSetup;
TCHAR szProgramGroups[]   = TEXT("UNICODE Program Groups");   // registry key for groups
TCHAR szRestrict[]        = TEXT("Restrictions");
TCHAR szNoRun[]           = TEXT("NoRun");
TCHAR szNoClose[]         = TEXT("NoClose");
TCHAR szEditLevel[]       = TEXT("EditLevel");
TCHAR szNoFileMenu[]      = TEXT("NoFileMenu");
TCHAR szNoSave[]          = TEXT("NoSaveSettings");
TCHAR szShowCommonGroups[]= TEXT("ShowCommonGroups");
TCHAR szSettings[]        = TEXT("Settings");
TCHAR szGroups[]          = TEXT("UNICODE Groups");
TCHAR szAnsiGroups[]      = TEXT("Groups");
TCHAR szCommonGroups[]    = TEXT("Common Groups");
TCHAR szSystemBoot[]      = TEXT("Boot");
TCHAR szSystemDisplay[]   = TEXT("display.drv");
TCHAR szDefPrograms[]     = TEXT("EXE COM BAT PIF");
TCHAR szSystemIni[]       = TEXT("system.ini");
TCHAR szWindows[]         = TEXT("Windows");
TCHAR szCheckBinaryType[] = TEXT("CheckBinaryType");
TCHAR szCheckBinaryTimeout[] = TEXT("CheckBinaryTimeout");
TCHAR szMigrateAnsi[] =     TEXT("Migrate ANSI");

BOOL bDisableDDE = FALSE;

/* in hotkey.c */
BOOL APIENTRY RegisterHotKeyClass(HANDLE hInstance);
/* in pmgseg.c */
HWND NEAR PASCAL IsGroupAlreadyLoaded(LPTSTR lpGroupKey, BOOL bCommonGroup);

/*---------------------------------------------------------------------------
 *
 * A fixed buffer case and space insensative compare...
 * Returns true if they compare the same.
 *
 *--------------------------------------------------------------------------*/

BOOL NEAR PASCAL StartupCmp(LPTSTR szSrc1, LPTSTR szSrc2)
{
    TCHAR sz1[MAXGROUPNAMELEN+1];
    TCHAR sz2[MAXMESSAGELEN+1];
    LPTSTR lp1, lp2;

    lstrcpy(sz1, szSrc1);
    CharUpper(sz1);
    lstrcpy(sz2, szSrc2);
    CharUpper(sz2);
    lp1 = sz1;
    lp2 = sz2;

    for (;;) {
        while(*lp1 == TEXT(' '))
            lp1++;
        while(*lp2 == TEXT(' '))
            lp2++;
        if (*lp1 != *lp2)
            return FALSE;
        if (!*lp1)
            break;
        while (*lp1 == *lp2 && *lp1)
            lp1++, lp2++;
    }
    return TRUE;
}


/*---------------------------------------------------------------------------
 *
 * Handles finding and execing the items in the startup group.
 *
 *--------------------------------------------------------------------------*/

VOID NEAR PASCAL HandleStartupGroup(int nCmdShow)
{
    TCHAR szGroupTitle[MAXGROUPNAMELEN+1];
    HWND hwndT;
    DWORD cbData = sizeof(TCHAR)*(MAXGROUPNAMELEN+1);
    PGROUP pGroup;
    LPGROUPDEF lpgd;
    TCHAR szCommonStartupGroup[MAXGROUPNAMELEN+1];
    TCHAR szDefaultStartup[MAXGROUPNAMELEN+1] = TEXT("startup");

    TCHAR        szStartupKana[]         = TEXT("^?X^?^^?[^?g^?A^?b^?v");

    if (nCmdShow != SW_SHOWMINNOACTIVE) {
        //
        // Daytona security weenies decreed that GetAsyncKeyState only work
        // if threads window is foreground, so make it so.
        //
        hwndT = GetForegroundWindow();
        if (hwndProgman != hwndT)
            SetForegroundWindow( hwndProgman );
        if (GetAsyncKeyState(VK_SHIFT) < 0)   // SHIFT will cancel the startup group
            return;
    }

    //
    // The Default startup group name is "Startup", for personal and common
    // groups.
    //
    LoadString(hAppInstance,IDS_DEFAULTSTARTUP,szDefaultStartup,CharSizeOf(szDefaultStartup));
    lstrcpy(szGroupTitle, szDefaultStartup);
    lstrcpy(szCommonStartupGroup, szDefaultStartup);

    //
    // Get the Personal startup group name.
    //
    if (hkeyPMSettings) {
        if ( RegQueryValueEx(hkeyPMSettings, szStartup, 0, 0, (LPBYTE)szGroupTitle, &cbData) != ERROR_SUCCESS ) {
            lstrcpy(szGroupTitle, szDefaultStartup);
        }
    }

    // Search for the startup group.
    hwndT = GetWindow(hwndMDIClient, GW_CHILD);
    while (hwndT) {
        //
        // Skip icon titles.
        //
        if (!GetWindow(hwndT, GW_OWNER)) {

            /* Compare the group name with the startup. */
            pGroup = (PGROUP)GetWindowLongPtr(hwndT, GWLP_PGROUP);
            if (lpgd = (LPGROUPDEF)GlobalLock(pGroup->hGroup)) {
                // ToddB: We want to check for three things here:
                // 1.) A common group with the name szCommonStartupGroup
                // 2.) Any group with the name szGroupTitle (which is a copy of szDefaultStartup)
                // 3.) If we are in Japan then we also search for two hardcoded strings,
                //      szStartupKana AND szStartup.  I think this special Japanese
                //      check is a bug and should be removed.

                if (pGroup->fCommon) {
                    if (StartupCmp(szCommonStartupGroup, (LPTSTR) PTR(lpgd, lpgd->pName)))
                        StartupGroup(hwndT);
                }
                else if (StartupCmp(szGroupTitle, (LPTSTR) PTR(lpgd, lpgd->pName))) {
                    StartupGroup(hwndT);
                }
#ifdef JAPAN_HACK_WHICH_TODDB_THINKS_IS_A_BUG
                else if (PRIMARYLANGID(LANGIDFROMLCID(GetThreadLocale())) == LANG_JAPANESE)
                {
                    if (StartupCmp(szStartupKana,(LPTSTR)PTR(lpgd,lpgd->pName)) ||  // search for hardcoded localized startup
                        StartupCmp(szStartup,(LPTSTR)PTR(lpgd,lpgd->pName)))        // search for hardcoded non-localized "startup"
                    {
                        StartupGroup( hwndT );
                    }
                }
#endif
                GlobalUnlock(pGroup->hGroup);
            }
        }
        hwndT = GetWindow(hwndT, GW_HWNDNEXT);
    }
}

/*** BoilThatDustSpec --         strips string to program name
 *
 *
 * VOID APIENTRY BoilThatDustSpec(PSTR pStart, BOOL bLoadIt)
 *
 * ENTRY - PSTR pStart - Program to exec, and possible parameters
 *         BOOL LoadIt -
 *
 * EXIT  - VOID
 *
 * SYNOPSIS -  strips everything after program name, then exec's program.
 *
 * WARNINGS -
 * EFFECTS  -
 *
 */

VOID APIENTRY BoilThatDustSpec(LPTSTR pStart, BOOL bLoadIt)
{
    register LPTSTR pEnd;
    WORD ret;
    BOOL bFinished;
    TCHAR szText[MAXMESSAGELEN+1];
    TCHAR szExtra[MAXMESSAGELEN+1];
    TCHAR szFilename[MAXITEMPATHLEN+1];
    TCHAR szWindowsDirectory2[MAXITEMPATHLEN+1];

    if (*pStart == TEXT('\0')) {                  /*test for null string*/
        return;
    }

    // Used to massage any errors.
    LoadString(hAppInstance,IDS_WININIERR,szExtra, CharSizeOf(szExtra));

    // skip first spaces
    while (*pStart == ' ') {
        pStart = CharNext(pStart);
    }
    bFinished = !*pStart;

    GetWindowsDirectory(szWindowsDirectory2, CharSizeOf(szWindowsDirectory2));

    while (!bFinished){
        pEnd = pStart;
        /* strip anything after execprogram name*/
        while ((*pEnd) && (*pEnd != TEXT(' ')) && (*pEnd != TEXT(','))){
            pEnd = CharNext(pEnd);
        }
        if (*pEnd == TEXT('\0'))
            bFinished = TRUE;
        else
            *pEnd = TEXT('\0');

        if (!*pStart) {
            pStart = pEnd+1;
            continue;
        }

        if (GetFreeSpace(GMEM_NOT_BANKED) < 65535L)
            break;

        GetDirectoryFromPath(pStart, szDirField);

        // Load and Run lines are done relative to windows directory.
        SetCurrentDirectory(szWindowsDirectory2);

        GetFilenameFromPath(pStart, szFilename);
        ret = ExecProgram(szFilename, szDirField, NULL, bLoadIt, 0, 0, 0);
        if (ret) {
           // Insert a phrase mentioning win.ini after the file name.
           szText[0] = TEXT('\'');
           lstrcpy(&szText[1], pStart);
           lstrcat(szText, szExtra);
	       MyMessageBox(NULL, IDS_APPTITLE, ret, szText, MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
        }

        pStart = pEnd+1;
    }

    SetCurrentDirectory(szWindowsDirectory); // in fact system32 directory
}

/*** DoRunEquals --
 *
 *
 * VOID APIENTRY DoRunEquals(PINT pnCmdShow)
 *
 * ENTRY -         PINT        pnCmdShow        -        point to cmdshow
 *
 * EXIT  -        VOID
 *
 * SYNOPSIS -  ???
 *
 * WARNINGS -
 * EFFECTS  -
 *
 */

VOID APIENTRY DoRunEquals(PINT pnCmdShow)
{
  TCHAR szBuffer[MAX_PATH];
  DWORD dwType;
  DWORD cbData;
  HKEY hkeyWindows;

  /* "Load" apps before "Run"ning any. */
  if (RegOpenKeyEx(HKEY_CURRENT_USER,
                   WINDOWS_KEY,
                   0,
                   KEY_READ,
                   &hkeyWindows) != ERROR_SUCCESS) {
      return;
  }

  *szBuffer = 0;
  cbData = sizeof(szBuffer);
  RegQueryValueEx(hkeyWindows,
                      L"Load",
                      0,
                      &dwType,
                      (LPBYTE)szBuffer, &cbData);
  if (*szBuffer)
      BoilThatDustSpec(szBuffer, TRUE);

  *szBuffer = 0;
  cbData = sizeof(szBuffer);
  RegQueryValueEx(hkeyWindows,
                      L"Run",
                      0,
                      &dwType,
                      (LPBYTE)szBuffer, &cbData);
  if (*szBuffer) {
      BoilThatDustSpec(szBuffer, FALSE);
      *pnCmdShow = SW_SHOWMINNOACTIVE;
  }

  RegCloseKey(hkeyWindows);
}


/*** GetSettings        --
 *
 *
 * PSTR APIENTRY GetSettings(VOID)
 *
 * ENTRY -         VOID
 *
 * EXIT  -        PSTR         - if NULL then error.
 *
 * SYNOPSIS -          ??
 *
 * WARNINGS -
 * EFFECTS  -
 *
 */

LPTSTR APIENTRY GetSettings()
{
  LPTSTR pszT;
  TCHAR szGroups[32];
  TCHAR szAppTitle[MAXKEYLEN + 1];
  DWORD cbData;
  DWORD dwType;
  DWORD rc;
  DWORD dwBinaryInfo;

  #define SETTING_SIZE        160

  /* Get the flags out of the INI file. */
  LoadString(hAppInstance, IDS_GROUPS, szGroups, CharSizeOf(szGroups));
  LoadString(hAppInstance, IDS_APPTITLE, szAppTitle, CharSizeOf(szAppTitle));

  /*
   * Use direct registry call.
   */
  if (hkeyPMSettings) {
      cbData = sizeof(bMinOnRun);
      RegQueryValueEx(hkeyPMSettings, szMinOnRun, 0, &dwType, (LPBYTE)&bMinOnRun, &cbData);
      cbData = sizeof(bAutoArrange);
      RegQueryValueEx(hkeyPMSettings, szAutoArrange, 0, &dwType, (LPBYTE)&bAutoArrange, &cbData);
      cbData = sizeof(bSaveSettings);
      RegQueryValueEx(hkeyPMSettings, szSaveSettings, 0, &dwType, (LPBYTE)&bSaveSettings, &cbData);
      cbData = sizeof(bInitialArrange);
      bInitialArrange = FALSE;
      rc = RegQueryValueEx(hkeyPMSettings, TEXT("InitialArrange"), 0, &dwType, (LPBYTE)&bInitialArrange, &cbData);
      if (bInitialArrange) {
          RegDeleteValue(hkeyPMSettings, TEXT("InitialArrange"));
      }

      //
      // Check if the binary type checking information exists.  If not,
      // add it.
      //
      // First check for the enabled / disabled entry.
      //

      cbData = sizeof(dwBinaryInfo);
      if (RegQueryValueEx(hkeyPMSettings, szCheckBinaryType, 0, &dwType,
                     (LPBYTE)&dwBinaryInfo, &cbData) == ERROR_FILE_NOT_FOUND) {
          //
          // Key doesn't exist, so create the default case.
          //

          dwBinaryInfo = BINARY_TYPE_DEFAULT;
          RegSetValueEx (hkeyPMSettings, szCheckBinaryType, 0, REG_DWORD,
                         (LPBYTE) &dwBinaryInfo, cbData);
      }

      //
      // Now check for the timeout value.  This is the number of milliseconds
      // of delay after the lastkeystroke and before the background thread
      // is signaled to check the type.
      //

      cbData = sizeof(dwBinaryInfo);
      if (RegQueryValueEx(hkeyPMSettings, szCheckBinaryTimeout, 0, &dwType,
                     (LPBYTE)&dwBinaryInfo, &cbData) == ERROR_FILE_NOT_FOUND) {
          //
          // Key doesn't exist, so create the default case.
          //

          dwBinaryInfo = BINARY_TIMEOUT_DEFAULT;
          RegSetValueEx (hkeyPMSettings, szCheckBinaryTimeout, 0, REG_DWORD,
                         (LPBYTE) &dwBinaryInfo, cbData);
      }

  }
  if (hkeyPMRestrict && !UserIsAdmin) {
      cbData = sizeof(fNoRun);
      RegQueryValueEx(hkeyPMRestrict, szNoRun, 0, &dwType, (LPBYTE)&fNoRun, &cbData);
      cbData = sizeof(fNoClose);
      RegQueryValueEx(hkeyPMRestrict, szNoClose, 0, &dwType, (LPBYTE)&fNoClose, &cbData);
      cbData = sizeof(fNoSave);
      RegQueryValueEx(hkeyPMRestrict, szNoSave, 0, &dwType, (LPBYTE)&fNoSave, &cbData);
      cbData = sizeof(dwEditLevel);
      RegQueryValueEx(hkeyPMRestrict, szEditLevel, 0, &dwType, (LPBYTE)&dwEditLevel, &cbData);
  }


  pszT = (LPTSTR)LocalAlloc(LPTR, SETTING_SIZE);
  if (!pszT)
      return(NULL);

  /*
   * Use direct registry call.
   */
  if (hkeyPMSettings) {
      cbData = SETTING_SIZE;
      if (RegQueryValueEx(hkeyPMSettings, szWindow, 0, &dwType, (LPBYTE)pszT, &cbData)) {
          LocalFree((HANDLE)pszT);
          return NULL;
      }
  }
  else {
      return(NULL);
  }

  return pszT;
}

//#if 0
BOOL GetUserAndDomainName(LPTSTR lpBuffer, DWORD cb)
{
  HANDLE hToken;
  DWORD cbTokenBuffer = 0;
  PTOKEN_USER pUserToken;
  LPTSTR lpUserName = NULL;
  LPTSTR lpUserDomain = NULL;
  DWORD cbAccountName = 0;
  DWORD cbUserDomain = 0;
  SID_NAME_USE SidNameUse;

  if (!OpenProcessToken(GetCurrentProcess(),
                       TOKEN_QUERY,
                       &hToken) ){
      return(FALSE);
  }

  //
  // Get space needed for token information
  //
  if (!GetTokenInformation(hToken,
                           TokenUser,
                           NULL,
                           0,
                           &cbTokenBuffer) ) {

      if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
          return(FALSE);
      }
  }

  //
  // Get the actual token information
  //
  pUserToken = (PTOKEN_USER)Alloc(cbTokenBuffer);
  if (pUserToken == NULL) {
      return(FALSE);
  }
  if (!GetTokenInformation(hToken,
                           TokenUser,
                           pUserToken,
                           cbTokenBuffer,
                           &cbTokenBuffer) ) {
      Free(pUserToken);
      return(FALSE);
  }

  //
  // Get the space needed for the User name and the Domain name
  //
  if (!LookupAccountSid(NULL,
                       pUserToken->User.Sid,
                       NULL, &cbAccountName,
                       NULL, &cbUserDomain,
                       &SidNameUse
                       ) ) {
      if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
          Free(pUserToken);
          return(FALSE);
      }
  }
  lpUserName = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*(cbAccountName+1));
  if (!lpUserName) {
      Free(pUserToken);
    return(FALSE);
  }
  lpUserDomain = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*(1+cbUserDomain));
  if (!lpUserDomain) {
      LocalFree(lpUserName);
      Free(pUserToken);
      return(FALSE);
  }

  //
  // Now get the user name and domain name
  //
  if (!LookupAccountSid(NULL,
                       pUserToken->User.Sid,
                       lpUserName, &cbAccountName,
                       lpUserDomain, &cbUserDomain,
                       &SidNameUse
                       ) ) {

      LocalFree(lpUserName);
      LocalFree(lpUserDomain);
      Free(pUserToken);
      return(FALSE);
  }

  if (*lpUserName &&
      ((int)sizeof(TCHAR)*(lstrlen(lpBuffer) + lstrlen(lpUserName) + lstrlen(lpUserDomain)) < (int)(cb+4)) ) {

      lstrcat(lpBuffer, TEXT(" - "));
      lstrcat(lpBuffer, lpUserDomain);
      lstrcat(lpBuffer, TEXT("\\"));
      lstrcat(lpBuffer, lpUserName);
  }
  Free(pUserToken);
  LocalFree(lpUserName);
  LocalFree(lpUserDomain);
  return(TRUE);
}
//#endif

/*** CreateFrameWindow --
 *
 *
 * HWND APIENTRY CreateFrameWindow(register PRECT prc, WORD nCmdShow)
 *
 * ENTRY -         PRECT        prc                        -
 *                        WORD        nCmdShow        -
 *
 * EXIT  -        HWND                                -  (NULL = Error)
 *
 * SYNOPSIS -          ??
 *
 * WARNINGS -
 * EFFECTS  -
 *
 */

HWND APIENTRY CreateFrameWindow(register PRECT prc, WORD nCmdShow)
{
  HDC    hdc;
  HBRUSH hbr;
  HMENU  hMenu;
  HMENU  hSystemMenu;
  TCHAR   szBuffer[40 + MAX_USERNAME_LENGTH];
  TCHAR   szProgmanClass[16];
  TCHAR   szUserName[MAX_USERNAME_LENGTH + 1] = TEXT("");
  TCHAR   szUserDomain[MAX_USERNAME_LENGTH + 1] = TEXT("");
  DWORD  dwType, cbData;

  /* Create the Desktop Manager window. */
  LoadString(hAppInstance, IDS_APPTITLE, szBuffer, CharSizeOf(szBuffer));
  LoadString(hAppInstance, IDS_PMCLASS, szProgmanClass, CharSizeOf(szProgmanClass));
#if 1
  GetUserAndDomainName(szBuffer,sizeof(szBuffer));
#else
  cbData = CharSizeOf(szUserName);
  GetUserName(szUserName, &cbData) ;
  cbData = CharSizeOf(szUserDomain);
  GetEnvironmentVariable(TEXT("USERDOMAIN"), szUserDomain, cbData);
  if (*szUserName){
      lstrcat(szBuffer, TEXT(" - "));
      lstrcat(szBuffer, szUserDomain);
      lstrcat(szBuffer, TEXT("\\"));
      lstrcat(szBuffer, szUserName);
  }
#endif
  hwndProgman = CreateWindow(szProgmanClass,
                             szBuffer,
			                 WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                             prc->left, prc->top,
                             prc->right-prc->left,
                             prc->bottom-prc->top,
                             NULL,               /* No Parent           */
                             NULL,               /* Use Class Menu   */
                             hAppInstance,
                             NULL);

  if (!hwndProgman)
      return NULL;

  SetWindowLong (hwndProgman, GWL_EXITING, 0);

  hMenu = GetMenu(hwndProgman);
  hSystemMenu = GetSystemMenu(hwndProgman, FALSE);

  if (!bExitWindows) {
      LoadString(hAppInstance,IDS_EXIT,szBuffer,CharSizeOf(szBuffer));
      ModifyMenu(hMenu,IDM_EXIT,MF_BYCOMMAND|MF_STRING,IDM_EXIT,szBuffer);
      DeleteMenu(hMenu,IDM_SHUTDOWN,MF_BYCOMMAND);
  }
  else {
      // replace Close menu item with Logoff and Shutdown
      LoadString(hAppInstance,IDS_LOGOFF,szBuffer,CharSizeOf(szBuffer));
      InsertMenu(hSystemMenu, SC_CLOSE, MF_BYCOMMAND|MF_STRING, SC_CLOSE, szBuffer);
      LoadString(hAppInstance,IDS_SHUTDOWN,szBuffer,CharSizeOf(szBuffer));
      ModifyMenu(hSystemMenu, SC_CLOSE, MF_BYCOMMAND|MF_STRING, IDM_SHUTDOWN, szBuffer);
  }

  if (hkeyPMRestrict && !UserIsAdmin) {
      cbData = sizeof(fNoFileMenu);
      RegQueryValueEx(hkeyPMRestrict, szNoFileMenu, 0, &dwType, (LPBYTE)&fNoFileMenu, &cbData);
  }

  if (fNoFileMenu) {
      DeleteMenu(hMenu, IDM_FILE, MF_BYPOSITION);
  }

  if (fNoSave) {
      bSaveSettings = FALSE;
      EnableMenuItem(hMenu, IDM_SAVESETTINGS, MF_BYCOMMAND|MF_GRAYED|MF_DISABLED);
      EnableMenuItem(hMenu, IDM_SAVENOW, MF_BYCOMMAND|MF_GRAYED|MF_DISABLED);
  }

  /* Update the menu items here (no maximized kids to deal with). */
  if (bMinOnRun)
      CheckMenuItem(hMenu, IDM_MINONRUN, MF_CHECKED);
  if (bAutoArrange)
      CheckMenuItem(hMenu, IDM_AUTOARRANGE, MF_CHECKED);
  if (bSaveSettings)
      CheckMenuItem(hMenu, IDM_SAVESETTINGS, MF_CHECKED);

  if (bInNtSetup) {
      EnableWindow(hwndProgman, FALSE);
  }
  ShowWindow(hwndProgman, nCmdShow);
  UpdateWindow(hwndProgman);

  /* fake-paint the client area with the color of the MDI client so users
   * have something pleasent to stare at while we hit the disk for the
   * group files
   */
  hdc = GetDC(hwndProgman);
  GetClientRect(hwndProgman, prc);
  hbr = CreateSolidBrush(GetSysColor(COLOR_APPWORKSPACE));
  if (hbr) {
      FillRect(hdc, prc, hbr);
      DeleteObject(hbr);
  }
  ReleaseDC(hwndProgman, hdc);

  return hwndProgman;
}

/*** IsGroup        --
 *
 *
 * BOOL APIENTRY IsGroup(PSTR p)
 *
 * ENTRY -         PSTR        p        -
 *
 * EXIT  -        BOOL                - (FALSE == ERROR)
 *
 * SYNOPSIS -          ??
 *
 * WARNINGS -
 * EFFECTS  -
 *
 */

BOOL PASCAL IsGroup(LPTSTR p)
{
    if (_wcsnicmp(p, TEXT("GROUP"), CCHGROUP) != 0) {
        return FALSE;
    }

    /*
     * Can't have 0 for first digit
     */
    if (p[5] == TEXT('0')) {
        return FALSE;
    }

    /*
     * Everything else must be a number
     */
    for (p += CCHGROUP; *p; p++) {
        if (*p != TEXT('C') && (*p < TEXT('0') || *p > TEXT('9'))) {
            return FALSE;
        }
    }

    return TRUE;
}


/*** RemoveString --
 *
 *
 * VOID APIENTRY RemoveString(PSTR pString)
 *
 * ENTRY -         PSTR        pString                -
 *
 * EXIT  -        VOID
 *
 * SYNOPSIS -          ??
 *
 * WARNINGS -
 * EFFECTS  -
 *
 */

VOID APIENTRY RemoveString(LPTSTR pString)
{

    LPTSTR pT = pString + lstrlen(pString) + 1;

    while (*pT) {
        while (*pString++ = *pT++)
            ;
    }
    *pString = 0;
}

/*** StringToEnd --
 *
 *
 * VOID APIENTRY StringToEnd(PSTR pString)
 *
 * ENTRY -         PSTR        pString                -
 *
 * EXIT  -        VOID
 *
 * SYNOPSIS -          ??
 *
 * WARNINGS -
 * EFFECTS  -
 *
 */

VOID PASCAL StringToEnd(LPTSTR pString)
{
    TCHAR *pT,*pTT;

    for (pT = pString; *pT; )           //go to end of strings
        while (*pT++)
            ;
    for (pTT = pString; *pT++ = *pTT++;)  // copy first string to the end
        ;
    *pT = 0;

    RemoveString(pString);                // remove first string
}

/*** GetGroupList --
 *
 *
 * VOID APIENTRY GetGroupList(PSTR szList)
 *
 * ENTRY -         PSTR        szList                -
 *
 * EXIT  -        VOID
 *
 * SYNOPSIS -          ??
 *
 * WARNINGS -
 * EFFECTS  -
 *
 */

VOID PASCAL GetGroupList(LPTSTR szList, HKEY hkeyPMGroups)
{
    TCHAR szOrd[CGROUPSMAX*8+7];
    TCHAR szT[20];
    LPTSTR pT, pTT, pS;
    INT cGroups;   // The number of Groups= lines.
    LPTSTR  lpList;
    DWORD  dwType;
    DWORD  dwIndex = 0;
    DWORD  cbValueName = 8;
    DWORD  cbData;
    INT    cbList = (CGROUPSMAX+1)*18;
    LPTSTR lpOrder;

    lpList = szList;
    //
    // Get the user's list of personal groups.
    //
    if (hkeyPMGroups) {
        cbValueName = cbList;
        while (!RegEnumValue(hkeyPMGroups, dwIndex, lpList, &cbValueName, 0, 0,
                             0, 0)) {
            dwIndex++; cbValueName++;
            lpList += cbValueName;
            cbList -= cbValueName;
            cbValueName = cbList;
        }
    }
    //
    // Now get the user's list of common groups.
    //
    if (hkeyPMCommonGroups) {
        cbValueName = cbList;
        dwIndex = 0;
        while (!RegEnumValue(hkeyPMCommonGroups, dwIndex, lpList, &cbValueName, 0, 0,
                             0, 0)) {
            dwIndex++; cbValueName++;
            lpList += cbValueName;
            cbList -= cbValueName;
            cbValueName = cbList;
        }
    }
    *lpList = TEXT('\0');

    cbData = sizeof(szOrd);
    if (bUseANSIGroups)
        lpOrder = szAnsiOrder;
    else
        lpOrder = szOrder;

    if (!hkeyPMSettings || RegQueryValueEx(hkeyPMSettings, lpOrder, 0, &dwType, (LPBYTE)szOrd, &cbData))
        *szOrd = TEXT('\0');

    cGroups = 0;

    /*
     * Filter out anything that isn't group#.
     */
    for (pT = szList; *pT; ) {
        CharUpper(pT);

        if (IsGroup(pT)) {
            pT += lstrlen(pT) + 1;
            cGroups++;
        } else {
            RemoveString(pT);
        }
    }

    /*
     * Sort the groups
     */
    lstrcpy(szT, TEXT("Group"));
    for (pT = szOrd; *pT; ) {
        while (*pT == TEXT(' ')) {
            pT++;
        }

        if ((*pT == TEXT('C') && (*(pT+1) < TEXT('0') || *(pT+1) > TEXT('9'))) ||
             (*pT != TEXT('C') && (*pT < TEXT('0') || *pT > TEXT('9'))) ) {
            break;
        }

        pTT = szT + CCHGROUP;
        while (*pT == TEXT('C') || (*pT >= TEXT('0') && *pT <= TEXT('9'))) {
            *pTT++ = *pT++;
        }
        *pTT=0;

        for (pS = szList; *pS; pS += lstrlen(pS) + 1) {
            if (!lstrcmpi(pS,szT)) {
                StringToEnd(pS);
                cGroups--;
                break;
            }
        }
    }

    /*
     * Move any remaining groups to the end of the list so that they load
     * last and appear on top of everything else - keeps DOS based install
     * programs happy.
     * If bInitialArrange is set then the remaining groups come from the
     * Windows 3.1 migration and we want these groups to be loaded before
     * the remaining groups so they appear below the regular groups.
     * 10-15-93 johannec
     */
    if (!bInitialArrange) {
        while (cGroups>0) {
            StringToEnd(szList);
            cGroups--;
        }
    }

}

/*** LoadCommonGroups --
 *
 *
 * VOID APIENTRY LoadCommonGroups(LPTSTR)
 *
 * ENTRY - LPTSTR the key name of the common group that should have the focus.
 *
 * EXIT  - HWND  hwnd of the common group which should have the focus.
 *
 * SYNOPSIS -
 *
 * WARNINGS -
 * EFFECTS  -
 *
 */

HWND LoadCommonGroups(LPTSTR lpFocusGroup)
{
    int i = 0;
    DWORD cbGroupKey = MAXKEYLEN;
    TCHAR szGroupKey[MAXKEYLEN];
    BOOL bRealArrange;
    FILETIME ft;
    HWND hwnd;


    if (!hkeyCommonGroups) {  // cannot access registry.
        return(NULL);
    }

    /*
     * Set global to note that we haven't run out of memory yet.
     */
    fLowMemErrYet = FALSE;
    /*
     * Flag for extraction problems.
     */
    fErrorOnExtract = FALSE;

    // REVIEW - Why stop AutoArrange on load ? Switch it off for now.
    bRealArrange = bAutoArrange;

    //
    // For mow, just load the groups in whatever order they are enumerated
    // in the registry.
    //
    while (!RegEnumKeyEx(hkeyCommonGroups, i, szGroupKey, &cbGroupKey, 0, 0, 0, &ft)) {
        if (cbGroupKey) {
            hwnd = LoadGroupWindow(szGroupKey, 0, TRUE);
        }
        cbGroupKey = sizeof(szGroupKey);
        i++;
    }

    bAutoArrange = bRealArrange;

    /*
     * Check to see if there was any trouble.
     */
    if (fErrorOnExtract) {
        /*
         * On observed problem with icon extraction has been to do
         * with a low memory.
         */
        MyMessageBox(hwndProgman, IDS_OOMEXITTITLE, IDS_LOWMEMONEXTRACT,
        NULL, MB_OK|MB_ICONHAND|MB_SYSTEMMODAL);
    }
    return(hwnd);
}


/*** LoadAllGroups --
 *
 *
 * VOID APIENTRY LoadAllGroups(VOID)
 *
 * ENTRY -         VOID
 *
 * EXIT  -        VOID
 *
 * SYNOPSIS -          ??
 *
 * WARNINGS -
 * EFFECTS  -
 *
 */

VOID PASCAL LoadAllGroups()
{
    LPTSTR pT, pszT;
    TCHAR szGroupList[(CGROUPSMAX+1)*18];
    WORD wIndex;
    TCHAR szPath[120];
    TCHAR szGroupKey[MAXKEYLEN];
    BOOL bRealArrange;
    DWORD cbData;
    DWORD dwType;
    BOOL fShowCommonGrps = TRUE;
    HKEY hkeyPMAnsiGroups = NULL;
    HKEY hkeyGroups;
    TCHAR szCommonGrpInfo[MAXKEYLEN];
    INT i;
    BOOL bDefaultPosition = FALSE;
    INT rgiPos[7];
    HWND hwndGroup;


    if (bUseANSIGroups) {
        RegCreateKeyEx(hkeyProgramManager, szAnsiGroups, 0, szProgman, 0,
                         KEY_READ | KEY_WRITE,
                         pSecurityAttributes, &hkeyPMAnsiGroups, NULL);
        hkeyGroups = hkeyPMAnsiGroups;
    }
    else {
        hkeyGroups = hkeyPMGroups;
    }
    if (!hkeyGroups) {  // cannot access registry.
        return;
    }

    /*
     * Set global to note that we haven't run out of memory yet.
     */
    fLowMemErrYet = FALSE;
    /*
     * Flag for extraction problems.
     */
    fErrorOnExtract = FALSE;

    // REVIEW - Why stop AutoArrange on load ? Switch it off for now.
    bRealArrange = bAutoArrange;

    //
    // If the user is allowed to see the common program groups, load them.
    //

    if (hkeyPMRestrict) {
        cbData = sizeof(fShowCommonGrps);
        RegQueryValueEx(hkeyPMRestrict, szShowCommonGroups, 0, &dwType, (LPBYTE)&fShowCommonGrps, &cbData);
    }
    if (fShowCommonGrps || AccessToCommonGroups) {

        RegCreateKeyEx(hkeyProgramManager, szCommonGroups, 0, szProgman, 0,
                         KEY_READ | KEY_WRITE,
                         pSecurityAttributes, &hkeyPMCommonGroups, NULL);
        //
        // Load all common program groups
        //
        LoadCommonGroups(szNULL);
    }

    //
    // Now load the user's personal program groups.
    //

    pT = szGroupList;
    for (GetGroupList(pT, hkeyGroups); *pT; pT += (lstrlen(pT) + 1)) {

        *szGroupKey = TEXT('\0');
        cbData = sizeof(szCommonGrpInfo);
        //
        // If we're loading a common group...
        //
        if (*(pT+CCHGROUP) == TEXT('C') && hkeyPMCommonGroups) {
            if (RegQueryValueEx(hkeyPMCommonGroups, pT, 0, 0,
                                 (LPBYTE)szCommonGrpInfo, &cbData))
                continue;
            wIndex = 0;
            for (pszT = pT + CCHCOMMONGROUP; *pszT; pszT++) {
                wIndex *= 10;
                wIndex += *pszT - TEXT('0');
            }
            //
            // Get the window coordinates of this common group.
            //
            pszT = szCommonGrpInfo;
            for (i=0; i < 7; i++) {
                 rgiPos[i] = 0;
                 while (*pszT && !((*pszT >= TEXT('0') && *pszT <= TEXT('9')) || *pszT == TEXT('-')))
                    pszT++;

                if (!*pszT) {
                    bDefaultPosition = TRUE;
                    break;
                }
                rgiPos[i] = MyAtoi(pszT);

                while (*pszT && ((*pszT >= TEXT('0') && *pszT <= TEXT('9')) || *pszT == TEXT('-')))
                    pszT++;
            }
            //
            // Now get the common group's name.
            //
            if (*pszT) {
                while(*pszT && *pszT == TEXT(' ')) pszT++;
                lstrcpy(szGroupKey, pszT);
            }

            hwndGroup = IsGroupAlreadyLoaded(szGroupKey, TRUE);
            if (!hwndGroup) {
                //
                // The common group no longer exists, remove this entry from
                // the user's list.
                //
                RegDeleteValue(hkeyPMCommonGroups, pT);
            }
            if (hwndGroup && !bDefaultPosition) {
                //
                // Position the common group according to the user's choice.
                //
                SetInternalWindowPos(hwndGroup, (UINT)rgiPos[6],
                                    (LPRECT)&rgiPos[0], (LPPOINT)&rgiPos[4]);
            }

        }
        else {
            if (RegQueryValueEx(hkeyGroups, pT, 0, 0, (LPBYTE)szGroupKey, &cbData))
                continue;
            wIndex = 0;
            for (pszT = pT + CCHGROUP; *pszT; pszT++) {
                wIndex *= 10;
                wIndex += *pszT - TEXT('0');
            }

            LoadGroupWindow(szGroupKey, wIndex, FALSE);
        }
    }
    bAutoArrange = bRealArrange;

    //
    // If we started with ANSI groups, save the newly converted unicode
    // groups now.
    //
    if (bUseANSIGroups) {
        WriteINIFile();
    }

    if (hkeyPMAnsiGroups) {
        RegCloseKey(hkeyPMAnsiGroups);
    }

    /*
     * Record the current display driver.
     */
    GetPrivateProfileString(szSystemBoot, szSystemDisplay, szPath, szPath, CharSizeOf(szPath), szSystemIni);
    RegSetValueEx(hkeyPMSettings, szSystemDisplay, 0, REG_SZ, (LPBYTE)szPath, sizeof(TCHAR)*(lstrlen(szPath)+1));

    /*
     * Check to see if there was any trouble.
     */
    if (fErrorOnExtract) {
        /*
         * On observed problem with icon extraction has been to do
         * with a low memory.
         */
        MyMessageBox(hwndProgman, IDS_OOMEXITTITLE, IDS_LOWMEMONEXTRACT,
        NULL, MB_OK|MB_ICONHAND|MB_SYSTEMMODAL);
    }
}

//*************************************************************
//
//  UseAnsiGroups()
//
//  Purpose:    Determine if we should convert the ANSI groups
//              to Unicode groups.
//
//  Parameters: DWORD dwDisp - disposition from RegCreateKeyEx
//                             on UNICODE Program Groups
//
//  Return:     BOOL  TRUE if the groups should be converted
//                    FALSE if not
//
//*************************************************************

BOOL UseAnsiGroups(DWORD dwDisp)
{
    DWORD dwType, dwMigrateValue, dwSize, dwAnsiValue = 0;
    LONG  lResult;
    BOOL  bRet = FALSE;
    HKEY  hKeyAnsiPG;


    //
    // If the dwDisp is a new key, then we return immediately and use
    // the ANSI groups if they exist.
    //

    if (dwDisp == REG_CREATED_NEW_KEY) {
        return TRUE;
    }

    //
    // dwDisp is an existing key.
    // If the "Migrate Ansi" value exist and the ANSI groups exist,
    // then use them otherwise use the current UNICODE information.
    //

    dwSize = sizeof (DWORD);
    lResult = RegQueryValueEx (hkeyProgramGroups, szMigrateAnsi,
                               NULL, &dwType, (LPBYTE) &dwMigrateValue,
                               &dwSize);

    //
    // Check the return value of registry call.  If it fails
    // then we are working with a machine that has UNICODE Program
    // Groups, and does not need to be updated from the ANSI groups.
    // Most of the time, we will exit here.
    //

    if (lResult != ERROR_SUCCESS) {
        return FALSE;
    }


    //
    // Now we need to know if any ANSI groups exist.
    //

    lResult = RegOpenKeyEx (HKEY_CURRENT_USER, szAnsiProgramGroups,
                            0, KEY_READ, &hKeyAnsiPG);

    if (lResult == ERROR_SUCCESS) {
        TCHAR szName[MAX_PATH];
        DWORD dwNameSize = MAX_PATH;
        FILETIME ft;

        //
        // The "Program Groups" key exists, check to see if there is
        // really something in it.
        //

        lResult = RegEnumKeyEx (hKeyAnsiPG, 0, szName, &dwNameSize, NULL,
                                NULL, NULL, &ft);

        //
        // If the return value is success, then there is one or more
        // items in the ANSI key.
        //

        if (lResult == ERROR_SUCCESS) {
            dwAnsiValue = 1;
        } else {
            dwAnsiValue = 0;
        }

        //
        // Close the key
        //

        RegCloseKey (hKeyAnsiPG);
    }

    //
    // If the MigrateValue is set, then we want to delete this entry
    // so the next time the user logs we don't try to convert the ANSI
    // groups again (and this function will execute faster).
    //

    if (dwMigrateValue) {
        RegDeleteValue (hkeyProgramGroups, szMigrateAnsi);
    }

    //
    // Determine the return value.
    //

    if (dwMigrateValue && dwAnsiValue) {
        bRet = TRUE;
    } else {
        bRet = FALSE;
    }


    return (bRet);

}


/*** ReadConfigFile --
 *
 *
 * BOOL APIENTRY ReadConfigFile(int nCmdShow)
 *
 * ENTRY -         int                CmdShow        -
 *
 * EXIT  -        void
 *
 * SYNOPSIS -          ??
 *
 * WARNINGS -
 * EFFECTS  -
 *
 */

VOID NEAR PASCAL ReadConfigFile(int nCmdShow)
{
  int       j;
  int       rgiPos[5];
  LPTSTR    pszT, pT;
  HCURSOR   hCursor;
  BOOL      bErrorMsgDisplayed = FALSE;
  TCHAR     szCommonGroupsKey[MAXKEYLEN];
  DWORD     dwDisposition;
  HKEY      hkey = NULL;

  hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
  ShowCursor(TRUE);

  /*
   * Create/Open the registry keys corresponding to progman.ini sections.
   */
  if (!RegCreateKeyEx(HKEY_CURRENT_USER, PROGMAN_KEY, 0, szProgman, 0,
                         KEY_READ | KEY_WRITE,
                         pSecurityAttributes, &hkeyProgramManager, NULL)) {

      RegCreateKeyEx(hkeyProgramManager, szSettings, 0, szProgman, 0,
                         KEY_READ | KEY_WRITE,
                         pSecurityAttributes, &hkeyPMSettings, NULL);

      RegCreateKeyEx(hkeyProgramManager, szRestrict, 0, szProgman, 0,
                         KEY_READ,
                         pSecurityAttributes, &hkeyPMRestrict, NULL);

      RegCreateKeyEx(hkeyProgramManager, szGroups, 0, szProgman, 0,
                         KEY_READ | KEY_WRITE,
                         pSecurityAttributes, &hkeyPMGroups, NULL);
  }
  else {
      MyMessageBox(NULL, IDS_APPTITLE, IDS_REGISTRYERROR, NULL, MB_OK | MB_ICONEXCLAMATION);
      bErrorMsgDisplayed = TRUE;
  }

  /* Get the global variable settings out of the INI file. */
  if (pszT = GetSettings()) {
      /* Get the window coordinates for PROGMAN's main window. */
      pT = pszT;
      for (j=0; j < 5; j++) {
          rgiPos[j] = 0;
          while (*pT && !((*pT >= TEXT('0') && *pT <= TEXT('9')) || *pT == TEXT('-')))
              pT++;

          if (!*pT) {
              LocalFree((HANDLE)pszT);
              goto DefaultPosition;
          }

          rgiPos[j] = MyAtoi(pT);

          while (*pT && ((*pT >= TEXT('0') && *pT <= TEXT('9')) || *pT == TEXT('-')))
              pT++;
      }
      LocalFree((HANDLE)pszT);
  }
  else {

DefaultPosition:
      /* NOTE: cx = 0 - CW_USEDEFAULT == CW_USEDEFAULT (0x8000) */
      rgiPos[0] = rgiPos[1] = CW_USEDEFAULT;
      rgiPos[2] = rgiPos[3] = 0;
      rgiPos[4] = SW_SHOWNORMAL;
  }

  if (nCmdShow != SW_SHOWNORMAL)
      rgiPos[4] = nCmdShow;

  /*
   * We don't want an invisible Program Manager!
   */
  if (!(rgiPos[4]))
      rgiPos[4] = SW_SHOWNORMAL;

  /* Create and paint the top-level frame window. */
  if (!CreateFrameWindow((PRECT)rgiPos, (WORD)rgiPos[4]))
      goto RCFErrExit;

  /*
   * Will create/open the key Program Groups, parent of all groups.
   */
  if (RegCreateKeyEx(HKEY_CURRENT_USER, szProgramGroups, 0, szGroups, 0,
                     KEY_READ | KEY_WRITE,
                     pSecurityAttributes, &hkeyProgramGroups, &dwDisposition)) {
      if (!bErrorMsgDisplayed) {
          MyMessageBox(NULL, IDS_APPTITLE, IDS_REGISTRYERROR, NULL, MB_OK | MB_ICONEXCLAMATION);
      }
      goto RCFErrExit;
  }
  if (UseAnsiGroups(dwDisposition)) {
      //
      // There are no UNICODE groups, so convert the ANSI groups and save
      // them as UNICODE groups.
      //
      bUseANSIGroups = TRUE;
      if (RegCreateKeyEx(HKEY_CURRENT_USER, szAnsiProgramGroups, 0, szGroups, 0,
                     KEY_READ,
                     pSecurityAttributes, &hkeyAnsiProgramGroups, &dwDisposition)) {
          if (!bErrorMsgDisplayed) {
              MyMessageBox(NULL, IDS_APPTITLE, IDS_REGISTRYERROR, NULL, MB_OK | MB_ICONEXCLAMATION);
          }
          goto RCFErrExit;
      }
  }


  /*
   * Will create/open the key Program Groups for common groups on the local
   * machine.
   */

  lstrcpy(szCommonGroupsKey, TEXT("SOFTWARE\\"));
  lstrcat(szCommonGroupsKey, szAnsiProgramGroups);

  //
  // Try opening/creating the common groups key with Write access
  //
OpenCommonGroupsKey:

  if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, szCommonGroupsKey, 0, szGroups, 0,
                         KEY_READ | KEY_WRITE | DELETE,
                         pAdminSecAttr, &hkeyCommonGroups, &dwDisposition)
               == ERROR_SUCCESS) {
      if (dwDisposition == REG_CREATED_NEW_KEY) {
          //
          // need to close and reopen the key to make sure we have the
          // right access
          //
          RegCloseKey(hkeyCommonGroups);
          goto OpenCommonGroupsKey;
      }
      AccessToCommonGroups = TRUE;

  } else {

      RegOpenKeyEx(HKEY_LOCAL_MACHINE, szCommonGroupsKey, 0, KEY_READ,
                         &hkeyCommonGroups);
  }

  //
  // If we have Ansi groups in the profile, add a menu item under the Options menu
  // to remove the old NT1.0 ANSI groups. This menu item will be deleted we the
  // user selects to remove the old groups
  //
  if (!fNoSave && (bUseANSIGroups || !RegOpenKeyEx(HKEY_CURRENT_USER,
                                      szAnsiProgramGroups,
                                      0,
                                      DELETE | KEY_READ | KEY_WRITE, &hkey))){

      HMENU hMenu = GetSubMenu(GetMenu(hwndProgman), 1);

      AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
      LoadString(hAppInstance, IDS_ANSIGROUPSMENU, szMessage, CharSizeOf(szMessage));
      AppendMenu(hMenu, MF_STRING | MF_ENABLED, IDM_ANSIGROUPS, szMessage);
      if (hkey) {
          RegCloseKey(hkey);
      }
  }


  /* The main frame window's been created, shown, and filled.
   * It's time to read the various group files, by enumerating group#
   * lines in PROGMAN.INI
   */
  LoadAllGroups();

  /*
   * Restriction key is no longer needed.
   */
  if (hkeyPMRestrict) {
      RegCloseKey(hkeyPMRestrict);
      hkeyPMRestrict = NULL;
  }


RCFErrExit:
  //
  // We've got the Ansi groups, reset this value.
  //
  bUseANSIGroups = FALSE;
  RegCloseKey(hkeyAnsiProgramGroups);
  hkeyAnsiProgramGroups = NULL;

  ShowCursor(FALSE);
  SetCursor(hCursor);
  ShowWindow(hwndMDIClient,SW_SHOWNORMAL);
}

/****************************************************************************
 *
 *   FUNCTION: ParseReserved(LPTSTR lpReserved, LPDWORD lpDdeId, LPDWORD lpHotKey)
 *
 *   PURPOSE: Parses the lpReserved field of the StartupInfo structure to
 *            get the Progman's new instance DDE id and its Hot key.
 *            The lpReserved field is a string of thee following format:
 *            "dde.%d,hotkey.%d"
 *
 *            Returns the dde id and hotkey.
 *
 *   COMMENTS: This is to be compatible with Win3.1 by allowing users to
 *             set a hotkey for Progman, and to allow them to change
 *             Progman's icon and window title (see SetProgmanProperties in
 *             pmwprocs.c)
 *
 *
 *   HISTORY:  08-28-92 JohanneC   Created.
 *
 ****************************************************************************/

void  ParseReserved(LPTSTR lpReserved, LPDWORD lpDdeId, LPDWORD lpHotKey)
{
   TCHAR *pch, *pchT, ch;

    //
    // The string will be of the format "dde.%d,hotkey.%d"
    //

    //
    // Get the DDE id.
    //
    if ((pch = wcsstr(lpReserved, TEXT("dde."))) != NULL) {
        pch += 4;

        pchT = pch;
        while (*pchT >= TEXT('0') && *pchT <= TEXT('9'))
            pchT++;

        ch = *pchT;
        *pchT = 0;
        *lpDdeId = MyAtoi(pch);
        *pchT = ch;

    }

    //
    // Get the hot key.
    //
    if ((pch = wcsstr(lpReserved, TEXT("hotkey."))) != NULL) {
        pch += 7;

        pchT = pch;
        while (*pchT >= TEXT('0') && *pchT <= TEXT('9'))
            pchT++;

        ch = *pchT;
        *pchT = 0;
        *lpHotKey = MyAtoi(pch);
        *pchT = ch;

    }
}

/*** IsHandleReallyProgman --
 *
 *
 * BOOL IsHandleReallyProgman (HWND hProgman, LPTSTR lpClassName)
 *
 * ENTRY -  HWND        hProgman
 *
 * EXIT  -  BOOL        TRUE if it is progman
 *                      FALSE if not
 *
 * SYNOPSIS -          ??
 *
 * WARNINGS -
 * EFFECTS  -
 *
 */

BOOL IsHandleReallyProgman(HWND hwndProgman)
{

    //
    // Test to see if we found Progman or Explorer.
    //

    if ((GetClassLong (hwndProgman, GCL_STYLE) == 0) &&
        (GetClassLongPtr (hwndProgman, GCLP_HICON) != 0) &&
        (GetClassLongPtr (hwndProgman, GCLP_MENUNAME) != 0)) {

        return TRUE;
    }

    return FALSE;
}


/*** AppInit --
 *
 *
 * BOOL APIENTRY AppInit(HANDLE hInstance, HANDLE hPrevInstance,
 *                                                  LPTSTR lpszCmdLine, int nCmdShow)
 *
 * ENTRY -  HANDLE        hInstance
 *          HANDLE        hPrevInstance
 *          LPTSTR        lpszCmdLine
 *          int                nCmdSHow
 *
 * EXIT  -  BOOL        xxx                - (FALSE == ERROR)
 *
 * SYNOPSIS -          ??
 *
 * WARNINGS -
 * EFFECTS  -
 *
 */

BOOL APIENTRY AppInit(HANDLE hInstance, LPTSTR lpszCmdLine, int nCmdShow)
{
  WORD              ret;
  WNDCLASS          wndClass;
  TCHAR              szClass[16];
  TCHAR              szBuffer[MAX_PATH];
  LOGFONT           lf;
  TCHAR              szText[MAXMESSAGELEN+1];
  STARTUPINFO       si;
  HWND hwndPrev;
  INT               nTempCmdShow = nCmdShow;

#ifdef DEBUG_PROGMAN_DDE
  {
  TCHAR szDebug[300];

  wsprintf (szDebug, TEXT("%d   PROGMAN:   Enter AppInit\r\n"),
            GetTickCount());
  OutputDebugString(szDebug);
  }
#endif


  //
  // Preserve this instance's module handle.
  //
  hAppInstance = hInstance;

  //
  // Specify the shutdown order of the progman process.
  // 2 means Porgman will shutdown before taskman (level = 1) and
  // ntsd or windbg (level = 0)
  //
  SetProcessShutdownParameters(2,0);

#ifndef MYDEBUG
  LoadString(hAppInstance, IDS_PMCLASS, szClass, CharSizeOf(szClass));
  if (hwndPrev = FindWindow(szClass, NULL)) {

      bDisableDDE = TRUE;           // Only 1 "PROGMAN" should respond to dde

      if (IsHandleReallyProgman(hwndPrev)) {
          DWORD dwDdeId = 0;
          DWORD dwHotKey = 0;
          LONG  lExiting;

          GetStartupInfo(&si);
          if (si.lpReserved) {
              ParseReserved(si.lpReserved, &dwDdeId, &dwHotKey);
          }

          PostMessage(hwndPrev, WM_EXECINSTANCE, (WPARAM)dwDdeId, dwHotKey);

          //
          // Need to check the other progman to see if it is exiting currently.
          // If so, then we will continue.  GetWindowLong returns 0 as a
          // failure case and as the "Not exiting" case (1 if we are exiting),
          // so we need to confirm that the last error is also
          // zero.
          //

          lExiting = GetWindowLong (hwndPrev, GWL_EXITING);

          if (lExiting != 1) {
              return FALSE;
          }
      }
  }
#endif

  /*
   * Compute general constants.
   */
  dyBorder = GetSystemMetrics(SM_CYBORDER);
  hItemIcon = LoadIcon(hAppInstance, (LPTSTR) MAKEINTRESOURCE(ITEMICON));
  if (!hItemIcon) {
      return FALSE;
  }

  /*
   * Load the accelerator table.
   */
  hAccel = LoadAccelerators(hAppInstance, (LPTSTR) MAKEINTRESOURCE(PMACCELS));
  if (!hAccel)
      return FALSE;

  cxIcon = GetSystemMetrics(SM_CXICON);
  cyIcon = GetSystemMetrics(SM_CYICON);

  cxOffset = 2 * GetSystemMetrics(SM_CXBORDER);
  cyOffset = 2 * GetSystemMetrics(SM_CYBORDER);

  cxIconSpace = cxIcon + 2 * cxOffset;
  cyIconSpace = cyIcon + 2 * cyOffset;

  SystemParametersInfo(SPI_ICONHORIZONTALSPACING, 0, (PVOID)(LPINT)&cxArrange, FALSE);
  SystemParametersInfo(SPI_ICONVERTICALSPACING, 0, (PVOID)(LPINT)&cyArrange, FALSE);
  SystemParametersInfo(SPI_GETICONTITLEWRAP, 0, (PVOID)(LPWORD)&bIconTitleWrap, FALSE);
  SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), (PVOID)(LPLOGFONT)&lf, FALSE);


  // lhb tracks check this out !!!! save this one for later! 1/21/93
  //lf.lfCharSet = ANSI_CHARSET ;
  //lstrcpy (lf.lfFaceName, TEXT("Lucida Sans Unicode"));

  hFontTitle = CreateFontIndirect(&lf);

  if (!hFontTitle)
      return FALSE;

  hIconGlobal = LoadIcon(hAppInstance,(LPTSTR) MAKEINTRESOURCE(WORDICON));
  if (!hIconGlobal) {
      return FALSE;
  }

  /*
   * Remember the original directory.
   */
  GetCurrentDirectory(MAXITEMPATHLEN+1, szOriginalDirectory);

  //
  // Set Progman's working directory to system32 directory instead of the
  // windows directory. johannec 5-4-93 bug 8364
  //
  //GetWindowsDirectory(szWindowsDirectory, MAXITEMPATHLEN+1);
  GetSystemDirectory(szWindowsDirectory, MAXITEMPATHLEN+1);

  /*
   * Make sure drive letter is upper case.
   */
  CharUpperBuff(szWindowsDirectory, 1);

  bInNtSetup = FALSE;

  if (lpszCmdLine && *lpszCmdLine &&
      !lstrcmpi(lpszCmdLine, TEXT("/NTSETUP"))) {
          //
          // Progman was started from ntsetup.exe, so it can be exited
          // without causing NT Windows to exit.
          //
          bExitWindows = FALSE;
          bInNtSetup = TRUE;
          *lpszCmdLine = 0;

  }
  else {
      HKEY hkeyWinlogon;
      DWORD dwType;
      DWORD cbBuffer;
      LPTSTR lpt;

      /* Check if we should be the shell by looking at shell= line for WInlogon
       */
      if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                   TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"),
                   0,
                   KEY_READ,
                   &hkeyWinlogon) == ERROR_SUCCESS) {
          cbBuffer = sizeof(szBuffer);
          if (RegQueryValueEx(hkeyWinlogon,
                              TEXT("Shell"),
                              0,
                              &dwType,
                              (LPBYTE)szBuffer,
                              &cbBuffer) == ERROR_SUCCESS) {
              CharLower(szBuffer);
              lpt = szBuffer;
              while (lpt = wcsstr(lpt, szProgman)) {
                  //
                  // we probably found progman
                  //
                  lpt += lstrlen(szProgman);
                  if (*lpt == TEXT(' ') || *lpt == TEXT('.') || *lpt == TEXT(',') || !*lpt)
                      bExitWindows = TRUE;
              }
          }
          else {
              //
              // assume that progman is the shell.
              //
              bExitWindows = TRUE;
          }
          RegCloseKey(hkeyWinlogon);
      }
      else {
          //
          // assume that progman is the shell.
          //
          bExitWindows = TRUE;
      }

  }

  if (lpszCmdLine && *lpszCmdLine) {
      nCmdShow = SW_SHOWMINNOACTIVE;
  }


    /*
     * call private api to mark task man as a system app. This causes
     * it to be killed after all other non-system apps during shutdown.
     */
//  MarkProcess(MP_SYSTEMAPP);


  /*
   * Load these strings now.  If we need them later,
   * we won't be able to load them at that time.
   */
  LoadString(hAppInstance, IDS_OOMEXITTITLE, szOOMExitTitle, CharSizeOf(szOOMExitTitle));
  LoadString(hAppInstance, IDS_OOMEXITMSG, szOOMExitMsg, CharSizeOf(szOOMExitMsg));

  LoadString(hAppInstance, IDS_PMCLASS, szClass, CharSizeOf(szClass));

  SetCurrentDirectory(szWindowsDirectory);

  SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS); // Bounce errors to us, not fs.

  // Set global exit flag.
  fExiting = FALSE;

  uiHelpMessage = RegisterWindowMessage(TEXT("ShellHelp"));
  uiBrowseMessage = RegisterWindowMessage(HELPMSGSTRING);
  uiActivateShellWindowMessage = RegisterWindowMessage(TEXT("ACTIVATESHELLWINDOW"));
  uiConsoleWindowMessage = RegisterWindowMessage(TEXT("ConsoleProgmanHandle"));
  uiSaveSettingsMessage = RegisterWindowMessage(TEXT("SaveSettings"));  // for UPEDIT.exe : User Profile Editor

  hhkMsgFilter = SetWindowsHook(WH_MSGFILTER, MessageFilter);
  if (hhkMsgFilter == 0) {
      GetLastError();
  }
  /*
   * Register the Frame window class.
   */
  wndClass.lpszClassName    = szClass;
  wndClass.style            = 0;
  wndClass.lpfnWndProc      = ProgmanWndProc;
  wndClass.cbClsExtra       = 0;
  wndClass.cbWndExtra       = sizeof(LONG);
  wndClass.hInstance        = hAppInstance;
  wndClass.hIcon            = hProgmanIcon = LoadIcon(hAppInstance, (LPTSTR) MAKEINTRESOURCE(PROGMANICON));
  wndClass.hCursor          = LoadCursor(NULL, IDC_ARROW);
  wndClass.hbrBackground    = NULL;
  wndClass.lpszMenuName     = (LPTSTR) MAKEINTRESOURCE(PROGMANMENU);

  if (!RegisterClass(&wndClass))
      return(FALSE);

  /*
   * Register the Program Group window class.
   */
  LoadString(hAppInstance, IDS_GROUPCLASS, szClass, 16);
  wndClass.lpszClassName    = szClass;
  wndClass.style            = CS_DBLCLKS;
  wndClass.lpfnWndProc      = GroupWndProc;
/*wndClass.cbClsExtra       = 0;*/
  wndClass.cbWndExtra       = sizeof(PGROUP);                    /* <== PGROUP */
/*wndClass.hInstance        = hAppInstance;*/
  wndClass.hIcon            = NULL;
/*wndClass.hCursor          = LoadCursor(NULL, IDC_ARROW);*/
  wndClass.hbrBackground    = (HBRUSH)(COLOR_WINDOW + 1);
  wndClass.lpszMenuName     = NULL;

  if (!RegisterClass(&wndClass))
      return(FALSE);

  {
      //
      // Set the working set size to 300k.
      //

      QUOTA_LIMITS QuotaLimits;
      NTSTATUS status;

      status = NtQueryInformationProcess( NtCurrentProcess(),
                                          ProcessQuotaLimits,
                                          &QuotaLimits,
                                          sizeof(QUOTA_LIMITS),
                                          NULL );
      if (NT_SUCCESS(status)) {
          QuotaLimits.MinimumWorkingSetSize = 400 * 1024;
          QuotaLimits.MaximumWorkingSetSize = 508 * 1024;

          NtSetInformationProcess( NtCurrentProcess(),
                                   ProcessQuotaLimits,
                                   &QuotaLimits,
                                   sizeof(QUOTA_LIMITS) );
      }
  }

  hGroupIcon = LoadIcon(hAppInstance, (LPTSTR) MAKEINTRESOURCE(PERSGROUPICON));
  hCommonGrpIcon = LoadIcon(hAppInstance, (LPTSTR) MAKEINTRESOURCE(COMMGROUPICON));

  if (!RegisterHotKeyClass((HANDLE)hAppInstance))
      return FALSE;

  RegisterDDEClasses((HANDLE)hAppInstance);

  /*
   * Initialize the security descriptor for the registry keys that
   * will be added to the user's personal profile.
   */
  pSecurityAttributes = &SecurityAttributes;
  if (!InitializeSecurityAttributes(pSecurityAttributes, TRUE))
      pSecurityAttributes = NULL;

  /*
   * Initialize the security descriptor for the registry keys that
   * will be added to the local machine program groups. Only
   * Administrators, Power Users and Server Operators
   * have all access to these keys, other users have only read access.
   */
  pAdminSecAttr = &AdminSecAttr;
  if (!InitializeSecurityAttributes(pAdminSecAttr, FALSE))
      pAdminSecAttr = NULL;


  /*
   * Test if the current user is an admin. If so, ignore restrictions
   * from the profile.
   */

  UserIsAdmin = TestUserForAdmin();

  /*
   * Read in the Group/Item data structures and create the windows.
   */
#ifdef DEBUG_PROGMAN_DDE
  {
  TCHAR szDebug[300];

  wsprintf (szDebug, TEXT("%d   PROGMAN:   Before ReadConfigFile\r\n"),
            GetTickCount());
  OutputDebugString(szDebug);
  }
#endif

  ReadConfigFile(nCmdShow);

#ifdef DEBUG_PROGMAN_DDE
  {
  TCHAR szDebug[300];

  wsprintf (szDebug, TEXT("%d   PROGMAN:   After ReadConfigFile\r\n"),
            GetTickCount());
  OutputDebugString(szDebug);
  }
#endif
  if (hwndProgman == NULL)
      return FALSE;

  /*
   * NOTE: the nCmdShow stuff from here down is bogus
   *
   * Do load/run lines, then the command line, then the startup group...
   */
  if (bExitWindows)
      DoRunEquals(&nCmdShow);

  /* Process the Command Line */
  if (lpszCmdLine && *lpszCmdLine) {
      WORD cbText;
      TCHAR szFilename[MAXITEMPATHLEN+1];

      lstrcpy(szPathField, lpszCmdLine);
      // win foo.bar is done relative to the original directory.
      SetCurrentDirectory(szOriginalDirectory);
      GetDirectoryFromPath(szPathField, szDirField);

      // now kernel converts the DOS cmd line to Ansi for us!
      GetFilenameFromPath(szPathField, szFilename);
      ret = ExecProgram(szFilename, szDirField, NULL, FALSE, 0, 0, 0);
      if (ret) {
          szText[0] = TEXT('\'');
          lstrcpy(&szText[1],szPathField);
          cbText = (WORD)lstrlen(szText);
          LoadString(hAppInstance,IDS_CMDLINEERR,&szText[cbText],CharSizeOf(szText)-cbText);
          MyMessageBox(NULL, IDS_APPTITLE, ret, szText, MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
      } else
 	      nCmdShow = SW_SHOWMINNOACTIVE;
      SetCurrentDirectory(szWindowsDirectory);
  }

  /*
   * See if we have a startup group last.
   */
#ifndef MYDEBUG
  if (bExitWindows || GetAsyncKeyState(VK_CONTROL) < 0)
#endif
      HandleStartupGroup(nTempCmdShow);

  /*
   * create an event for monitoring the ProgramGroups key.
   */
  InitializeGroupKeyNotification();

  if (bInitialArrange) {
      PostMessage(hwndMDIClient, WM_MDIICONARRANGE, 0, 0L);
      PostMessage(hwndProgman, WM_COMMAND, IDM_SAVENOW, 0L);
  }

#ifdef DEBUG_PROGMAN_DDE
  {
  TCHAR szDebug[300];

  wsprintf (szDebug, TEXT("%d   PROGMAN:   Leave AppInit\r\n"),
            GetTickCount());
  OutputDebugString(szDebug);
  }
#endif

  return TRUE;
}
