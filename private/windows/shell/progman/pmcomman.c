/*
 * pmcomman.c - program manager
 *
 *  Copyright (c) 1991,  Microsoft Corporation
 *
 *  DESCRIPTION
 *
 *                This file is for support of program manager under NT Windows.
 *                This file is/was ported from pmcomman.c (program manager).
 *
 *  MODIFICATION HISTORY
 *      Initial Version: x/x/90        Author Unknown, since he didn't feel
 *                                                                like commenting the code...
 *
 *      NT 32b Version:         1/22/91        Jeff Pack   Intitial port to begin.
 *                              6/15/91        JohanneC    re-ported.
 *
 */

#include "progman.h"

/*** IsRemoteDrive -- tests to see if drive is a remote drive
 *
 *
 * BOOL APIENTRY IsRemoteDrive(wDrive)
 *
 * ENTRY -         int                wDrive - drive number to test
 *
 * EXIT  -        BOOL        xxx -  returns TRUE if remote, false if not.
 *
 * SYNOPSIS -  calls GetDriveType to determine if media is remote or not.
 *
 * WARNINGS -
 * EFFECTS  -
 *
 */

BOOL APIENTRY IsRemoteDrive(int wDrive)
{

  TCHAR         pszdrive[5] = TEXT("c:\\");       /*default string value*/

  pszdrive[0] = (TCHAR)wDrive + (TCHAR)TEXT('A');  /*convert wDrive (0-25) to drive letter*/
                                           /*and place in string to pass to GetDriveType*/

  return((BOOL) (GetDriveType(pszdrive) == DRIVE_REMOTE));
}


/*** IsRemovableDrive -- tests to see if drive is removable
 *
 *
 * BOOL APIENTRY IsRemovableDrive( int wDrive)
 *
 * ENTRY -         int                wDrive - drive number to test
 *
 * EXIT  -        BOOL        xxx -  returns TRUE if removable, false if not.
 *
 * SYNOPSIS -  calls GetDriveType to determine if media is removable or not.
 *
 * WARNINGS -
 * EFFECTS  -
 *
 */

BOOL APIENTRY IsRemovableDrive( int wDrive)
{
  TCHAR         pszdrive[5] = TEXT("c:\\");                /*default string value*/


  pszdrive[0] = (TCHAR)wDrive + (TCHAR)TEXT('A');                /*convert wDrive (0-25) to drive letter*/
                                                                        /*and place in string to pass to GetDriveType*/
  return((BOOL)(GetDriveType(pszdrive) == DRIVE_REMOVABLE));
}

/*** BuildDescription --
 *
 *
 * VOID APIENTRY BuildDescription(LPTSTR szName, LPTSTR szPath)
 *
 * ENTRY -         LPTSTR        szName
 *                 LPTSTR        szPath
 *
 * EXIT  -        VOID        xxx -
 *
 * SYNOPSIS -
 *
 * WARNINGS -  sordid coding style  BUG BUG assumes 8.3 filename convention
 * EFFECTS  -
 *
 */

VOID APIENTRY BuildDescription(LPTSTR szName, LPTSTR szPath)
{
  TCHAR      ch;
  TCHAR      ch2 = 0;
  LPTSTR     p;
  LPTSTR     p2;
  LPTSTR     p3  = NULL;

//When User creating new icon with command line added quote (such as "a b.exe")
// and no description, then invalid description ("a b) added new icon.

  BOOL      bQuote = FALSE;

  if (*szPath == TEXT('"') && *(szPath+lstrlen(szPath)-1) == TEXT('"')) {
      bQuote = TRUE;
      *(szPath+lstrlen(szPath)-1) = TEXT('\0');
      szPath++;
  }

  p = p2 = szPath;

  /* Scan the string looking for the last filename. */
  while (*p) {
      if (*p == TEXT('\\'))
          p2 = p+1;
      else if (*p == TEXT('.'))
          p3 = p;
      p = CharNext(p);
  }

  if (!p3)
      p3 = p;

  ch = *p3;
  *p3 = TEXT('\0');

  if (lstrlen(p2) > MAXITEMNAMELEN) {
      ch2 = *(p2 + MAXITEMNAMELEN);
      *(p2 + MAXITEMNAMELEN) = TEXT('\0');
  }

  lstrcpy(szName, p2);
  *p3 = ch;

  if (ch2) {
      *(p2 + MAXITEMNAMELEN) = ch2;
  }

  if( bQuote )
    *(szPath+lstrlen(szPath)) = TEXT('"');

  CharUpper(szName);
  CharLower(CharNext(szName));
}

/* Returns 0 for success.  Otherwise returns a IDS_ string code. */

/*** ExecProgram -- exec program function
 *
 *
 * WORD APIENTRY ExecProgram(LPTSTR lpszPath, LPTSTR lpDir, LPTSTR lpTitle, BOOL bLoadIt)
 *
 * ENTRY -        LPTSTR      lpszPath    -
 *                LPTSTR      lpDir       -
 *                BOOL        bLoadIt     -
 *
 * EXIT  -        BOOL        xxx         -         returns (0)FALSE if successful, else returns
 *                                                                IDS_ string code.
 *
 * SYNOPSIS -
 *
 * WARNINGS -
 * EFFECTS  -
 *
 */

WORD APIENTRY ExecProgram (
    LPTSTR lpszPath,
    LPTSTR lpDir,
    LPTSTR lpTitle,
    BOOL bLoadIt,
    DWORD dwDDEId,
    WORD wHotKeyId,
    BOOL bNewVDM
    )
{
  WORD      ret;
  WORD      wNTVDMFlags=0;
  HCURSOR   hCursor;
  LPTSTR     lpP;
  TCHAR cSeparator;
  TCHAR lpReservedFormat[] = TEXT("dde.%d,hotkey.%d,ntvdm.%d");
  TCHAR lpReserved[100];  // used for DDE request of icons from console apps
                         // add for passing the hotkey associated with an item.
  DWORD OldErrorMode;

  ret = 0;
  //hPendingWindow = NULL;

  hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

  /* Don't mess with the mouse state; unless we're on a mouseless system.
   */
  if (!GetSystemMetrics(SM_MOUSEPRESENT))
      ShowCursor(TRUE);

  /* skip leading spaces
   */
  while (*lpszPath == TEXT(' '))
      lpszPath++;

  /* skip past path
   */
  lpP = lpszPath;
  if (*lpszPath == TEXT('"')) {
     cSeparator = TEXT('"');
     lpP++;
  }
  else {
     cSeparator = TEXT(' ');
  }

  for (; *lpP && *lpP != cSeparator; lpP = CharNext(lpP))
      ;
  if (*lpP == TEXT('"')) {
     lpP++;
  }

  /* if stuff on end, separate it
   */
  if (*lpP)
      *lpP++ = 0;

  /* Try to exec 'szCommandLine'. */
  fInExec = TRUE;

  /*changed order, since wPendINstance is a 32b HANDLE, and ret is WORD*/
    if (!lpP)
        lpP = TEXT("");

    // Setup this flags variable so NTVDM can overwrite pif information
    // if user has specified info in the icon properties.

    if (lpDir && *lpDir)
       wNTVDMFlags |= PROPERTY_HAS_CURDIR;

    if (wHotKeyId)
       wNTVDMFlags |= PROPERTY_HAS_HOTKEY;

    if (lpTitle && *lpTitle)
       wNTVDMFlags |= PROPERTY_HAS_TITLE;

    wsprintf(lpReserved, lpReservedFormat, dwDDEId, wHotKeyId, wNTVDMFlags);

    OldErrorMode = SetErrorMode(0);
    ret = (WORD)RealShellExecuteEx(hwndProgman, NULL, lpszPath, lpP,
                            lpDir, NULL, lpTitle, lpReserved,
                            (WORD)(bLoadIt ? SW_SHOWMINNOACTIVE : SW_SHOWNORMAL), NULL,
                            bNewVDM ? EXEC_SEPARATE_VDM : 0);
    SetErrorMode(OldErrorMode);

  fInExec = FALSE;

   // Unfortunately we are still using the 0 - 31 error code
   // combinations instead of the NT error codes.  SE_ERR_OOM
   // is the default case for all errors in RealShellExecuteExW,
   // thus displaying an out of memory error is probably bogus.
   // We can call GetLastError ourselves to get the _real_ error.
   //
   // Special cases:
   //
   // 1)  If you have a really deep directory structure(70 chars), and you
   //     try to spawn a WOW app from it CreateProcess will fail with
   //     and error of ERROR_INVALID_PARAMETER.  We'll grab this case,
   //     and map it into the bad path message since it is the closest.

   if (ret == SE_ERR_OOM)
      {
      DWORD dwResult = GetLastError();

      if (dwResult == ERROR_INVALID_PARAMETER)
         ret = SE_ERR_PNF;
      }


  /*BUG BUG these are DOS exec function return codes, no map yet to NT return codes!*/
  switch (ret) {
      case 0:
      case SE_ERR_OOM:    // 8
          ret = IDS_NOMEMORYMSG;
          break;

      case SE_ERR_FNF:    // 2
          ret = IDS_FILENOTFOUNDMSG;
          break;

      case SE_ERR_PNF:    // 3
          ret = IDS_BADPATHMSG;
          break;

      case 4:
          ret = IDS_MANYOPENFILESMSG;
          break;

      case 5:
          ret = IDS_ACCESSDENIED;
          break;

      case 10:
          ret = IDS_NEWWINDOWSMSG;
          break;

      case 12:
          ret = IDS_OS2APPMSG;
          break;

      case 15:
          /* KERNEL has already put up a messagebox for this one. */
          ret = 0;
          break;

      case 16:
          ret = IDS_MULTIPLEDSMSG;
          break;

      case 18:
          ret = IDS_PMODEONLYMSG;
          break;

      case 19:
          ret = IDS_COMPRESSEDEXE;
          break;

      case 20:
          ret = IDS_INVALIDDLL;
          break;

      case ERROR_NOT_ENOUGH_QUOTA:
      case STATUS_PAGEFILE_QUOTA:
          ret = IDS_INSUFFICIENTQUOTA;
          break;

      case SE_ERR_SHARE:
          ret = IDS_SHAREERROR;
	  break;

      case SE_ERR_ASSOCINCOMPLETE:
          ret = IDS_ASSOCINCOMPLETE;
	  break;

      case SE_ERR_DDETIMEOUT:
      case SE_ERR_DDEFAIL:
      case SE_ERR_DDEBUSY:
          ret = IDS_DDEFAIL;
          break;

      case SE_ERR_NOASSOC:
          ret = IDS_NOASSOCMSG;
          break;

      default:
          if (ret < 32)
              goto EPExit;

          if (bMinOnRun && !bLoadIt)
              ShowWindow(hwndProgman, SW_SHOWMINNOACTIVE);

          ret = 0;
  }

EPExit:

  if (!GetSystemMetrics(SM_MOUSEPRESENT)) {
      /*
       * We want to turn the mouse off here on mouseless systems, but
       * the mouse will already have been turned off by USER if the
       * app has GP'd so make sure everything's kosher.
       */
      if (ShowCursor(FALSE) != -1)
          ShowCursor(TRUE);
  }

  SetCursor(hCursor);

  return(ret);
}


/*** SelectionType --
 *
 *
 * WORD APIENTRY SelectionType(VOID)
 *
 * ENTRY -         VOID
 *
 * EXIT  -        WORD        xxx         -
 *
 *
 * SYNOPSIS -
 *
 * WARNINGS -
 * EFFECTS  -
 *
 */

WORD APIENTRY SelectionType(VOID)
{
  /* If no groups, return GROUP type (so user can create one). */
  if (!pCurrentGroup)
      return(TYPE_PERSGROUP);

  if (IsIconic(pCurrentGroup->hwnd))
      if (pCurrentGroup->fCommon)
          return(TYPE_COMMONGROUP);
      else
          return(TYPE_PERSGROUP);

  return(TYPE_ITEM);
}

/*** ExecItem --
 *
 *
 * VOID APIENTRY ExecItem(PGROUP pGroup, PITEM pItem, BOOL fShift, BOOL fStartup)
 *
 * ENTRY -         PGROUP        pGroup                -
 *                        PITEM        pItem                -
 *
 * EXIT  -        VOID
 *
 *
 * SYNOPSIS -
 *
 * WARNINGS -
 * EFFECTS  -
 *
 */
VOID APIENTRY ExecItem(PGROUP pGroup, PITEM pItem, BOOL fShift, BOOL fStartup)
{
    WORD      ret;
    TCHAR      szCommand[MAXITEMPATHLEN + 1];
    TCHAR      szDir[2*(MAXITEMPATHLEN + 1)];
    TCHAR      szTemp[MAXMESSAGELEN+1];
    TCHAR      *szTitle;
    GROUPDEF  *lpgd;

    //
    // Exec the item in the user's home directory.
    //
    SetCurrentDirectory(szOriginalDirectory);

    GetItemCommand(pGroup,pItem,szCommand,szDir);

    if (fShift) {
        fShift = GetKeyState(VK_SHIFT) < (SHORT)0;
    }

    ret = (WORD)((WORD)fShift || GroupFlag(pGroup, pItem, (WORD)ID_MINIMIZE));
    wPendingHotKey = GroupFlag(pGroup, pItem, (WORD)ID_HOTKEY);

    pExecingGroup = pGroup;
    pExecingItem = pItem;
    DoEnvironmentSubst(szCommand, MAXITEMPATHLEN + 1);
    DoEnvironmentSubst(szDir, 2 * (MAXITEMPATHLEN + 1));

    // REVIEW Check the working directory first because the user may
    // still want to try to run the app even with an invalid working
    // dir.
    // Check working directory.
    GetCurrentDirectory(MAXMESSAGELEN+1, szTemp);

    SheRemoveQuotes(szDir);

    // Allow dir field to be NULL without error.
    if (szDir && *szDir) {
        // NB The VPD call is because SCD sometimes returns success even
        // if the drive is invalid. SCD returns FALSE on success.
        if ((!ValidPathDrive(szDir) ||
		     !SetCurrentDirectory(szDir)) &&
		     !fStartup) {
            if (MyMessageBox(hwndProgman, IDS_BADPATHTITLE, IDS_BADPATHMSG3, NULL, MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONEXCLAMATION) == IDCANCEL)
	        goto Exit;
        }
    }

    SetCurrentDirectory(szTemp);
    //OemToChar(szDir, szDir);

    if ((lpgd = LockGroup(pGroup->hwnd)) == NULL)
        return;
    szTitle = (TCHAR *)PTR(lpgd, ((LPITEMDEF)ITEM(lpgd, pItem->iItem))->pName);

    if (!dwDDEAppId || (dwDDEAppId != pItem->dwDDEId)) {
        dwDDEAppId++;
    }
    pItem->dwDDEId = dwDDEAppId;

    ret = ExecProgram(szCommand, szDir, szTitle, ret, dwDDEAppId,
                      GroupFlag(pGroup, pItem, ID_HOTKEY),
                      GroupFlag(pGroup, pItem, ID_NEWVDM) );

    UnlockGroup(pGroup->hwnd);

    pExecingGroup = NULL;
    pExecingItem = NULL;

    // Check for errors.
    if (ret) {
        if (fStartup) {
             /*
              * This exec is for an item in the startup group, the error
              * message will need to be patched up by adding a comment
              * after the path bit.
              */
             szTemp[0] = TEXT('\'');
             lstrcpy(&szTemp[1], szCommand);
             LoadString(hAppInstance, IDS_STARTUPERR, szCommand, CharSizeOf(szCommand));
             lstrcat(szTemp, szCommand);
             lstrcpy(szCommand, szTemp);
        }
        MyMessageBox(hwndProgman, IDS_EXECERRTITLE, ret, szCommand, MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
    }
Exit:
  SetCurrentDirectory(szWindowsDirectory);
}

VOID RemoveAnsiGroups()
{
    HMENU hMenu;
    FILETIME ft;
    DWORD cbGroupKey = MAXKEYLEN;
    TCHAR szGroupKey[MAXKEYLEN];
    HCURSOR hCursor;
    INT i = 0;

    if (!MyDialogBox(UPDATEGROUPSDLG, hwndProgman, UpdateGroupsDlgProc)) {
        return;
    }

    if (RegOpenKeyEx(HKEY_CURRENT_USER, szAnsiProgramGroups, 0,
                     KEY_READ | DELETE, &hkeyAnsiProgramGroups) != ERROR_SUCCESS) {
        return;
    }

    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    ShowCursor(TRUE);

    while (!RegEnumKeyEx(hkeyAnsiProgramGroups, i, szGroupKey, &cbGroupKey, 0, 0, 0, &ft)) {
        if (cbGroupKey) {
            if (RegDeleteKey(hkeyAnsiProgramGroups, szGroupKey))
                i++;
        }
        cbGroupKey = sizeof(szGroupKey);
    }
    RegCloseKey(hkeyAnsiProgramGroups);

    if (!RegDeleteKey(HKEY_CURRENT_USER, szAnsiProgramGroups)) {
        //
        // Change Options menu: remove the 'Update Program Groups' menu item
        //
        if (pCurrentGroup &&
              GetWindowLong(pCurrentGroup->hwnd, GWL_STYLE) & WS_MAXIMIZE) {
            hMenu = GetSubMenu(GetMenu(hwndProgman), 2);
        }
        else {
            hMenu = GetSubMenu(GetMenu(hwndProgman), 1);
        }
        DeleteMenu(hMenu, 6, MF_BYPOSITION);
        DeleteMenu(hMenu, 5, MF_BYPOSITION);
        DrawMenuBar(hwndProgman);
    }
    RegDeleteKey(hkeyProgramManager, TEXT("Groups"));
    RegDeleteValue(hkeyPMSettings, szAnsiOrder);

    ShowCursor(FALSE);
    SetCursor(hCursor);

}

/*** ProgmanCommandProc --
 *
 *
 * BOOL APIENTRY ProgmanCommandProc(register HWND hwnd, WORD wMsg,
 *                                                                register WPARAM wParam, LPARAM lParam)
 *
 * ENTRY -         HWND        hWnd
 *                        WORD        wMsg
 *                        WPARAM        wParam
 *                        LPARAM        lParam
 * EXIT  -        BOOL        xxx - returns info, or zero, for nothing to return
 *
 * SYNOPSIS -  ???
 *
 * WARNINGS -
 * EFFECTS  -
 *
 */

BOOL APIENTRY ProgmanCommandProc(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    BOOL  bMaxed;
    WORD  wHelpCmd;
    HMENU hMenu;
    LPTSTR  psz;
    HWND  hwndMdiActive;

    hwndMdiActive = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);
    if (hwndMdiActive != NULL)
        bMaxed = (GetWindowLong(hwndMdiActive, GWL_STYLE) & WS_MAXIMIZE) ? TRUE : FALSE;
    else
        bMaxed = 0;

    dwContext = (DWORD)(IDH_HELPFIRST + wParam);

    switch (GET_WM_COMMAND_ID(wParam, lParam)) {

    case IDM_OPEN:
        if (!pCurrentGroup)
            break;

        if (SelectionType() == TYPE_ITEM)
            ExecItem(pCurrentGroup,pCurrentGroup->pItems,TRUE, FALSE);
        else
            SendMessage(pCurrentGroup->hwnd, WM_SYSCOMMAND, SC_RESTORE, 0L);
        break;

    case IDM_NEW:
        if (fNoFileMenu)
            break;

        if (dwEditLevel == 1)
            goto PCPNewItem;

        if (dwEditLevel > 1)
            break;

        if (MyDialogBox(CHOOSERDLG, hwndProgman, ChooserDlgProc)) {
            switch (wNewSelection) {

            case TYPE_PERSGROUP:
            case TYPE_COMMONGROUP:
                MyDialogBox(GROUPDLG, hwndProgman, NewGroupDlgProc);
                break;

            case TYPE_ITEM:
PCPNewItem:
                /*
                 * We are creating a new program item.
                 */
                MyDialogBox(ITEMDLG, hwndProgman, NewItemDlgProc);
                break;
            }
        }
        break;

    case IDM_MOVE:
        if (fNoFileMenu)
            break;

        MyDialogBox(MOVECOPYDLG, hwndProgman, MoveItemDlgProc);
        break;

    case IDM_COPY:
        if (fNoFileMenu)
            break;

        MyDialogBox(MOVECOPYDLG, hwndProgman, CopyItemDlgProc);
        break;

    case IDM_DELETE:
        if (fNoFileMenu)
            break;

        switch (SelectionType()) {

        case TYPE_ITEM:

            if (pCurrentGroup->pItems) {
                wParam = IDS_CONFIRMDELITEMMSG;
                GetItemText(pCurrentGroup, pCurrentGroup->pItems, szNameField, 0);
                psz = szNameField;
                break;
            }
                /*** FALL THRU ***/

        case TYPE_PERSGROUP:
        case TYPE_COMMONGROUP:

            wParam = IDS_CONFIRMDELGROUPMSG;
            GetWindowText(pCurrentGroup->hwnd, (LPTSTR)szNameField, MAXITEMPATHLEN);
            psz = szNameField;
            break;
        }

        if (MyMessageBox(hwndProgman, (WORD)IDS_CONFIRMDELTITLE, (WORD)wParam,
                psz, (WORD)(MB_YESNO | MB_ICONEXCLAMATION)) == IDYES) {
            if (wParam == (WPARAM)IDS_CONFIRMDELITEMMSG) {
                DeleteItem(pCurrentGroup,pCurrentGroup->pItems);
            } else {
                DeleteGroup(pCurrentGroup->hwnd);
            }
        }
        break;

    case IDM_PROPS:
        if (fNoFileMenu)
            break;

        switch (SelectionType()) {

        case TYPE_ITEM:
            if (pCurrentGroup->pItems) {
                MyDialogBox(ITEMDLG, hwndProgman, EditItemDlgProc);
                break;
            }
            /*** FALL THRU ***/

        case TYPE_PERSGROUP:
        case TYPE_COMMONGROUP:
            {
                LPGROUPDEF lpgd;

                lpgd = LockGroup(pCurrentGroup->hwnd);
                if (lpgd == 0L) {
                    break;
                }

                lstrcpy(szNameField, (LPTSTR) PTR(lpgd, lpgd->pName));
                UnlockGroup(pCurrentGroup->hwnd);
                MyDialogBox(GROUPDLG, hwndProgman, EditGroupDlgProc);
                break;
            }
        }
        break;

    case IDM_RUN:
        if (fNoFileMenu)
            break;

        MyDialogBox(RUNDLG, hwndProgman, RunDlgProc);
        break;

    case IDM_EXIT:
        if (fNoFileMenu)
              break;

        PostMessage(hwndProgman, WM_CLOSE, 0, (LPARAM)-1);
	    break;

    case IDM_SHUTDOWN:
        if (fNoFileMenu)
              break;

        /* Don't close if restricted. */
        if (fNoClose)
            break;

        if (bExitWindows) {

            fExiting = TRUE;
            SetWindowLong (hwndProgman, GWL_EXITING, 1);

            /* Call the ShutdownDialog API. */
            ShutdownDialog(hAppInstance, hwndProgman);

            /* User clicked cancel or some app refused the ExitWindows... */
            fExiting = FALSE;
            SetWindowLong (hwndProgman, GWL_EXITING, 0);
        }
        break;

    case IDM_AUTOARRANGE:
          bAutoArrange = !bAutoArrange;

          /* Check/Uncheck the menu item. */
          hMenu = GetSubMenu(GetMenu(hwndProgman), IDM_OPTIONS + (int)bMaxed - (int)fNoFileMenu);
          CheckMenuItem(hMenu, GET_WM_COMMAND_ID(wParam, lParam),
                (WORD)(bAutoArrange ? MF_CHECKED : MF_UNCHECKED));
          if (hkeyPMSettings)
              RegSetValueEx(hkeyPMSettings, szAutoArrange, 0, REG_DWORD, (LPBYTE)&bAutoArrange, sizeof(bAutoArrange));
          if (bAutoArrange) {
              HWND hwndT;

              for (hwndT=GetWindow(hwndMDIClient, GW_CHILD); hwndT;
                                    hwndT=GetWindow(hwndT, GW_HWNDNEXT)) {
                  if (GetWindow(hwndT, GW_OWNER))
                      continue;

                  ArrangeItems(hwndT);
              }
          }
          break;

    case IDM_MINONRUN:
          bMinOnRun = !bMinOnRun;

          /* Check/Uncheck the menu item. */
          hMenu = GetSubMenu(GetMenu(hwndProgman), IDM_OPTIONS + (int)bMaxed - (int)fNoFileMenu);
          CheckMenuItem(hMenu, GET_WM_COMMAND_ID(wParam, lParam),
                (WORD)(bMinOnRun ? MF_CHECKED : MF_UNCHECKED));
          if (hkeyPMSettings)
              RegSetValueEx(hkeyPMSettings,
                            szMinOnRun,
                            0,
                            REG_DWORD,
                            (LPBYTE)&bMinOnRun,
                            sizeof(bMinOnRun));
          break;

    case IDM_SAVESETTINGS:
          bSaveSettings = !bSaveSettings;

          /* Check/Uncheck the menu item. */
          hMenu = GetSubMenu(GetMenu(hwndProgman), IDM_OPTIONS + (int)bMaxed - (int)fNoFileMenu);
          CheckMenuItem(hMenu, GET_WM_COMMAND_ID(wParam, lParam),
                (UINT)(bSaveSettings ? MF_CHECKED : MF_UNCHECKED));
          if (hkeyPMSettings)
              RegSetValueEx(hkeyPMSettings,
                            szSaveSettings,
                            0,
                            REG_DWORD,
                            (LPBYTE)&bSaveSettings,
                            sizeof(bSaveSettings));
          break;

      case IDM_SAVENOW:
          WriteINIFile();
          break;

      case IDM_ANSIGROUPS:
          RemoveAnsiGroups();
          break;

      case IDM_CASCADE:
          SendMessage(hwndMDIClient, WM_MDICASCADE, 0, 0L);
          break;

      case IDM_TILE:
          SendMessage(hwndMDIClient, WM_MDITILE, 0, 0L);
          break;

      case IDM_ARRANGEICONS:
          if (SelectionType() != TYPE_ITEM)
              SendMessage(hwndMDIClient, WM_MDIICONARRANGE, 0, 0L);
          else
              ArrangeItems(pCurrentGroup->hwnd);
          break;

      case IDM_HELPINDEX:
          wHelpCmd = HELP_INDEX;
          wParam = 0;
          goto ACPCallHelp;

      case IDM_HELPSEARCH:
          wHelpCmd = HELP_PARTIALKEY;
          wParam = (WPARAM)szNULL;
          goto ACPCallHelp;

      case IDM_HELPHELP:
          wHelpCmd = HELP_HELPONHELP;
          wParam = 0;

ACPCallHelp:
          SetCurrentDirectory(szOriginalDirectory);
          if (!WinHelp(hwndProgman, szProgmanHelp, wHelpCmd, (DWORD)wParam)) {
              MyMessageBox(hwndProgman, IDS_APPTITLE, IDS_WINHELPERR, NULL, MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
          }
          SetCurrentDirectory(szWindowsDirectory);
          break;

      case IDM_ABOUT:
      {
          TCHAR szTitle[40];

          LoadString(hAppInstance, IDS_APPTITLE, szTitle, CharSizeOf(szTitle));
          if (ShellAbout(hwndProgman, szTitle, NULL, NULL) == -1)
              MessageBox(hwndProgman, szOOMExitMsg, szOOMExitTitle, MB_ICONHAND | MB_SYSTEMMODAL | MB_OK);
          break;
      }

      default:
          return(FALSE);
    }

  return(TRUE);

}
