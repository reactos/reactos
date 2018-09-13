/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    taskman.c

Abstract:

    This file contains the source for the windows Task Manager.
    Taskman basically is a dialog box, which enumerates active windows
    keep in the user window manager, then sets active focus to the selected
    dialog box element(ie active window).

--*/

// Has to be unicode because InternalGetWindowText
// user routine is strictly so.

#include "taskman.h"
#include "progman.h"
#include "security.h"

//#ifdef FE_IME // 2-Jun-92, by eichim
#include <winnls32.h>
//#endif

#include <port1632.h>

extern HINSTANCE hAppInstance;

#define MAXPATHFIELD 260

TCHAR szTMPathField[MAXPATHFIELD];
TCHAR szTMDirField[MAXPATHFIELD];
TCHAR szTMTitle[MAXPATHFIELD];
TCHAR szTMMessage[MAXMSGBOXLEN];

TCHAR szTMUserHomeDir[MAXPATHFIELD];
TCHAR szTMWindowsDirectory[MAXPATHFIELD];

TCHAR szTMOOMExitMsg[64];
TCHAR szTMOOMExitTitle[32];

VOID SetDefButton(HWND hwndDlg, INT  idButton);

// registry key for groups

BOOL bChangedDefaultButton;

INT MyX = 0;
INT MyY = 0;

BOOL fMsgBox = FALSE;

VOID
HideWindow(HWND hwnd)
{
   if (!fMsgBox) {

      if (!fNoRun) {
         SetDlgItemText(ghwndTMDialog, IDD_TMPATH, TEXT(""));
      }

      // redundant?  why do they do the reverse twice for show below?
      ShowWindow(ghwndTMDialog, SW_HIDE);

      SetWindowPos(ghwndTMDialog, HWND_NOTOPMOST, 0, 0, 0, 0,
         SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

      // Erase dark border from depressed pushbuttons
      SendMessage(GetDlgItem(hwnd, IDCANCEL), // IDCANCEL
         BM_SETSTYLE, BS_PUSHBUTTON, MAKELPARAM(TRUE, 0));
      SendMessage(GetDlgItem(hwnd, IDD_TERMINATE),
         BM_SETSTYLE, BS_PUSHBUTTON, MAKELPARAM(TRUE, 0));
      SendMessage(GetDlgItem(hwnd, IDD_CASCADE),
         BM_SETSTYLE, BS_PUSHBUTTON, MAKELPARAM(TRUE, 0));
      SendMessage(GetDlgItem(hwnd, IDD_TILE),
         BM_SETSTYLE, BS_PUSHBUTTON, MAKELPARAM(TRUE, 0));
      SendMessage(GetDlgItem(hwnd, IDD_ARRANGEICONS),
         BM_SETSTYLE, BS_PUSHBUTTON, MAKELPARAM(TRUE, 0));

   }
}

/*
 * We call HideTasklist() when we want to remove the tasklist window
 * from the screen but not select another window (ie. when we're about
 * to select another app.  We call ShowWindow(SW_HIDE) directly when
 * we're doing something like tiling or cascading so a window other than
 * the tasklist will become the foreground window.
 */
VOID HideTasklist(VOID)
{
    if (!fNoRun) {
       SetDlgItemText(ghwndTMDialog, IDD_TMPATH, TEXT(""));
    }

    SetWindowPos(ghwndTMDialog, HWND_TOP, 0, 0, 0, 0, SWP_HIDEWINDOW |
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);

}


VOID ShowTasklist(
   POINT pt)
{
    if (!fMsgBox) {
        /*
         * Retract the drop down listbox.
         */

         if (!fNoRun) {
            SendDlgItemMessage(ghwndTMDialog, IDD_TMPATH,
                               CB_SHOWDROPDOWN,0,0);
         }

        SetWindowPos(ghwndTMDialog, HWND_TOPMOST, pt.x, pt.y, 0, 0,
           SWP_NOSIZE | SWP_NOACTIVATE );

        //
        // WinCim disables the Taskman window which make it behave strangely on NT
        //
        EnableWindow(ghwndTMDialog, TRUE);
        SetForegroundWindow(ghwndTMDialog);

        ShowWindow(ghwndTMDialog, SW_NORMAL);

    }
}


/*** ActivateSelectedWindow --         Calls user, to set active window, selected
 *                                                                 by the user.
 *
 *
 * ActivateSelectedWindow(HWND hwndLB)
 *
 * ENTRY -         HWND hwndLB - handle to window, which is to become the active
 *                                                  window, with focus.
 * EXIT  -
 * SYNOPSIS -  This function takes the hwnd passed into it, calls user
 *                                to set active focus to that window.
 * WARNINGS -
 * EFFECTS  -
 *
 */

VOID ActivateSelectedWindow(
    HWND hwndLB)
{
   INT nIndex;
   HWND hwndT;
   HWND hwndLastActive;
   DWORD lTemp;

    /*
     * Get the hwnd of the item which was selected.
     */
    nIndex = (int)SendMessage(hwndLB, LB_GETCURSEL, 0, 0);
    hwndT = (HWND)SendMessage(hwndLB, LB_GETITEMDATA, nIndex, 0);

    if (!IsWindow(hwndT)) {
        /*
         * We gotta make sure the window is valid before doing stuff with it.
         * An app may terminate itself in the background rendering these
         * window handles invalid.
         */
        goto Beep;
    }

    /*
     * Switch to that task.
     * HACK! Activate the window in the hwndLastActive field of the WndStruct.
     */
    hwndLastActive = GetLastActivePopup(hwndT);

    if (!IsWindow(hwndLastActive)) {
        goto Beep;
    }

    /*
     * But only if it isn't disabled.
     */
    lTemp = GetWindowLong(hwndLastActive, GWL_STYLE);
    if (!(lTemp & WS_DISABLED)) {
        /*
         * HACK!! Use SwitchToThisWindow() to bring dialog parents as well.
         */
        SwitchToThisWindow(hwndLastActive, TRUE);

    } else {
Beep:
        MessageBeep(0);
    }
}

/*** DoEndTask --
 *
 * void DoEndTask( HWND hwnd )
 */
VOID DoEndTask(
   HWND hwnd )
{
   TCHAR szMsgBoxText[MAXMSGBOXLEN];
   TCHAR szTempField[MAXTASKNAMELEN];
   INT nch;
   DWORD dwProcessId = 0;

   /*
    * We don't want to let someone do an EndTask on progman or EndTask of
    * a EndTask dialog (which would make progman die)
    */
   GetWindowThreadProcessId(hwnd, &dwProcessId);

   if (dwProcessId == GetCurrentProcessId()) {
      MessageBeep(MB_OK);
      return;
   }


   if (!EndTask(hwnd, FALSE, FALSE)) {
      /* App does not want to close, ask user if
       * he wants to blow it away
       */

       InternalGetWindowText(hwnd, szTempField, MAXTASKNAMELEN);

       /* Load the message box string, it is very long (greater than 255 chars
        * which is why we load it in two pieces
        */
        nch = LoadString(NULL, IDS_MSGBOXSTR1, szMsgBoxText, MAXMSGBOXLEN);
        LoadString(NULL, IDS_MSGBOXSTR2, &szMsgBoxText[nch], MAXMSGBOXLEN-nch);

        if( MessageBox( NULL, szMsgBoxText, szTempField,
                MB_SETFOREGROUND | MB_SYSTEMMODAL | MB_YESNO ) == IDYES) {
            EndTask(hwnd, FALSE, TRUE);
        }
    }
}


/*** CallEndTask --                 A separate thread to instigate EndTask
 *
 * CallEndTask( HWND hwnd );
 *
 * ENTRY -      HWND hwnd - window handle for the task to be killed
 * EXIT  -
 * SYNOPSIS -  This function calls EndTask on the given window to kill the
 *              task that owns that window.
 *
 * WARNINGS -
 * EFFECTS  -   Kills the task that owns hwnd.
 *
 */

DWORD CallEndTask(
    HWND hwnd)
{
    DoEndTask(hwnd);

    return 0;
}

/*** TaskmanDlgProc --         Dialog Procedure for Taskman Window
 *
 *
 *
 * TaskmanDlgProc(HWND hDlg, WORD wMSG, DWORD wParam, LPARAM lparam)
 *
 * ENTRY -         HWND hhDlg                 - handle to dialog box.
 *                        WORD wMsg                  - message to be acted upon.
 *                        DWORD wParam        - value specific to wMsg.
 *                        LPARAM lparam                - value specific to wMsg.
 *
 * EXIT  -           True if success, False if not.
 * SYNOPSIS -  Dialog box message processing function.
 *
 * WARNINGS -
 * EFFECTS  -
 *
 */

LONG TaskmanDlgProc(
    HWND hwnd,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lparam)
{
    int nIndex;
    RECT rc;
    HWND hwndLB;
    HWND hwndNext;
    TCHAR szTempField[MAXTASKNAMELEN];
    POINT pt;
    HKEY  hKey;
    DWORD dwDisp;
    DWORD dwDataType, dwMaxFiles=INIT_MAX_FILES, dwMaxFilesSize, dwCount;
    TCHAR szFileEntry[20];
    TCHAR szFullPath[MAXPATHFIELD];


    hwndLB = GetDlgItem(hwnd, IDD_TASKLISTBOX);

    switch (wMsg) {

    case WM_INITDIALOG:
        /*
         * call private api to mark task man as a system app. This causes
         * it to be killed after all other non-system apps during shutdown.
         */
        GetWindowRect(hwnd, &rc);
        dxTaskman = rc.right - rc.left;
        dyTaskman = rc.bottom - rc.top;
        dxScreen = GetSystemMetrics(SM_CXSCREEN);
        dyScreen = GetSystemMetrics(SM_CYSCREEN);

        pt.x = (dxScreen - dxTaskman) / 2;
        pt.y = (dyScreen - dyTaskman) / 2;

        SetWindowPos(hwnd, HWND_NOTOPMOST, pt.x, pt.y, 0, 0,
           SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

        SendDlgItemMessage(hwnd, IDD_TMPATH, EM_LIMITTEXT, MAXPATHFIELD-4, 0L);
        szTMPathField[0] = TEXT('\0');

        bChangedDefaultButton = FALSE;

        return FALSE;

    case WM_SHOWWINDOW:
        /*
         * If we're being shown fill in the listbox.  We do this here
         * rather than in WM_ACTIVATE process so we can do it while the
         * dialog is still invisible.
         */
        if (wParam != 0) {

            /*
             * First delete any previous entries.
             */
            while ((int)SendMessage(hwndLB, LB_DELETESTRING, 0, 0) != LB_ERR);

            /*
             * Search the window list for enabled top level windows.
             */
            hwndNext = GetWindow(hwnd, GW_HWNDFIRST);
            while (hwndNext) {

                /*
                 * Only add non-owned, visible, non-Taskman, Top Level Windows.
                 */
                if ((hwndNext != hwnd) && (IsWindowVisible(hwndNext)) &&
                        (!GetWindow(hwndNext, GW_OWNER))) {
                    if (InternalGetWindowText(hwndNext, szTempField, MAXTASKNAMELEN )) {
                        nIndex = (int)SendMessage(hwndLB, LB_ADDSTRING, 0,
                                (LPARAM)(LPTSTR)szTempField);
                        SendMessage(hwndLB, LB_SETITEMDATA, nIndex,
                                (LPARAM)hwndNext);
                    }
                }

                hwndNext = GetWindow(hwndNext, GW_HWNDNEXT);
            }
            SendMessage(hwndLB, LB_SETCURSEL, 0, 0);

            //
            // Set the default button to "Switch To"
            //

            SetDefButton(hwnd,IDD_SWITCH);

            //
            // Load the combobox with the recently used files.
            //

            if (GetDlgItem(hwnd, IDD_TMPATH)) {

                //
                // FIrst empty the combo box from the last time.
                //

                SendDlgItemMessage (hwnd, IDD_TMPATH,
                                    CB_RESETCONTENT, 0, 0);


                //
                // Load the combobox with recently used files from the registry.
                //
                // Query the max number of files first.
                //

                if (RegCreateKeyEx (HKEY_CURRENT_USER, FILES_KEY, 0, 0,
                                    REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                                    NULL, &hKey, &dwDisp) == ERROR_SUCCESS) {

                    if (dwDisp == REG_OPENED_EXISTING_KEY) {

                        //
                        //  Query the max number of entries
                        //

                        dwMaxFilesSize = sizeof (DWORD);

                        if (RegQueryValueEx (hKey, MAXFILES_ENTRY, NULL, &dwDataType,
                                       (LPBYTE)&dwMaxFiles, &dwMaxFilesSize) == ERROR_SUCCESS) {

                            //
                            //  Now Query each entry and add it to the list box.
                            //

                            for (dwCount=0; dwCount < dwMaxFiles; dwCount++) {

                                wsprintf (szFileEntry, FILE_ENTRY, dwCount);
                                dwMaxFilesSize = MAXPATHFIELD+1;

                                if (RegQueryValueEx (hKey, szFileEntry, NULL, &dwDataType,
                                                 (LPBYTE) szFullPath, &dwMaxFilesSize) == ERROR_SUCCESS) {

                                    //
                                    // Found an entry.  Add it to the combo box.
                                    //

                                    SendDlgItemMessage (hwnd, IDD_TMPATH,
                                                        CB_ADDSTRING, 0,
                                                        (LPARAM)szFullPath);

                                } else {
                                    break;
                                }
                            }
                        }
                    } else {
                        //
                        // We are working with a new key, so we need to
                        // set the default number of files.
                        //

                        RegSetValueEx (hKey, MAXFILES_ENTRY, 0, REG_DWORD,
                                       (CONST BYTE *) &dwMaxFiles, sizeof (DWORD));
                    }

                    //
                    //  Close the registry key
                    //

                    RegCloseKey (hKey);

                }
            }

            //
            // Disable the Run button and set the focus to the
            // listbox.
            //

            EnableWindow(GetDlgItem(hwnd, IDD_RUN), FALSE);

            SetFocus(hwndLB);
        }
        break;

    case WM_ACTIVATE:
        /*
         * If we're being deactivated clear the listbox so we
         * can fill it in afresh when we're re-activated.
         */
        if (wParam == 0) {
            /*
             * If we're not already invisible, hide ourself.
             */
            if (IsWindowVisible(hwnd)) {
                HideWindow(hwnd);
            }
        }

        if (!bChangedDefaultButton) {
           SetDefButton(hwnd,IDD_SWITCH);
        }

        break;

    case WM_ACTIVATEAPP:
        if (wParam)
            return FALSE;

        /*
         * If we are not visible when we get this message it is because
         * we are already in the process of terminating.  If we don't
         * ignore this we get into a weird race condition and the frame
         * of the window being activated doesn't get fully drawn.  (BG)
         */
        if (IsWindowVisible(hwnd)) {
            HideWindow(hwnd);
        }
        break;

    case WM_WININICHANGE:
        //
        // Check if the user's environment variables have changed, if so
        // regenerate the environment, so that new apps started from
        // taskman will have the latest environment.
        //
        if (lparam && (!lstrcmpi((LPTSTR)lparam, (LPTSTR) TEXT("Environment")))) {
            PVOID pEnv;

            RegenerateUserEnvironment(&pEnv, TRUE);
            break;
        }
        else {
            return FALSE;
        }

    case MYCBN_SELCHANGE:
       if (!fNoRun) {
          if (GetDlgItemText(hwnd, IDD_TMPATH, (LPTSTR)szTMPathField, MAXPATHFIELD)) {
             EnableWindow(GetDlgItem(hwnd, IDD_RUN), TRUE);
             if (!bChangedDefaultButton) {
                SetDefButton (hwnd, IDD_RUN);
                bChangedDefaultButton = TRUE;
             }
          } else {
             EnableWindow(GetDlgItem(hwnd, IDD_RUN), FALSE);
             if (bChangedDefaultButton) {
                SetDefButton (hwnd, IDD_SWITCH);
                bChangedDefaultButton = FALSE;
             }
          }
       }
       break;


    case WM_COMMAND:
        switch(LOWORD(wParam)) {

        case IDD_TASKLISTBOX:

            switch(HIWORD(wParam)) {

            case LBN_DBLCLK:
                HideTasklist();
                ActivateSelectedWindow(hwndLB);
                break;

//#ifdef FE_IME // 2-Jun-92, by eichim
//	    {
            // NOTE: bOpen should be TRUE and the ime should be disabled
            // for the tasklistbox but when we tab to the edit control it
            // needs to be enabled.
            case LBN_SETFOCUS:
                WINNLSEnableIME((HWND)NULL, FALSE);
                break;

            case LBN_KILLFOCUS:
                WINNLSEnableIME((HWND)NULL, TRUE);
                break;
//	    }
//#endif // FE_IME

            default:
                // Always change the default button to Switch when we tab to
                // the task listbox
                //

                if (!fNoRun) {
                    if (bChangedDefaultButton) {
                       SetDefButton (hwnd, IDD_SWITCH);
                       bChangedDefaultButton = FALSE;
                    }
                }

                return FALSE;
            }
            break;

        case IDD_TMPATH:
           PostMessage (hwnd, MYCBN_SELCHANGE, 0, 0);
           break;

        case IDOK:
           if (!bChangedDefaultButton) {
              goto Switchem;
           }

        case IDD_RUN:
           if (!fNoRun) {
              TCHAR szFilename[MAXPATHFIELD];
              WORD ret;
              BOOL bMinOnRunSave;

              //
              // Run this app in the user's home directory
              //
              SetCurrentDirectory(szOriginalDirectory);

              GetDlgItemText(hwnd, IDD_TMPATH, szTMPathField, MAXPATHFIELD);
              DoEnvironmentSubst(szTMPathField, MAXPATHFIELD);
              GetDirectoryFromPath(szTMPathField, szTMDirField);
              if (*szTMDirField) {
                  // Convert path into a .\foo.exe style thing.
                  lstrcpy(szFilename, TEXT(".\\"));
                  // Tag the filename and params on to the end of the dot slash.
                  GetFilenameFromPath(szTMPathField, szFilename+2);
                  if (*(szFilename+2) == TEXT('"') ) {
                      SheRemoveQuotes(szFilename+2);
                      CheckEscapes(szFilename, CharSizeOf(szFilename));
                  }
              }
              else {
                  GetFilenameFromPath(szTMPathField, szFilename);
              }

              //
              // Don't minimize ProgMan when exec'ing a program from taskman.
              //
              bMinOnRunSave = bMinOnRun;
              bMinOnRun = FALSE;

              ret = ExecProgram(szFilename, szTMDirField, szFilename, FALSE, 0, 0, 0);

              //
              // Reset Minimized on Run
              //
              bMinOnRun = bMinOnRunSave;


              //
              // reset Progman's working directory.
              //
              SetCurrentDirectory(szWindowsDirectory);

              if (ret) {
                 fMsgBox = TRUE;
                 MyMessageBox( hwnd, IDS_EXECERRTITLE, ret, szTMPathField,
                    MB_SYSTEMMODAL | MB_OK | MB_ICONEXCLAMATION );
                 fMsgBox = FALSE;

                 SetFocus(GetDlgItem(hwnd, IDD_TMPATH));
              } else {
                 GetDlgItemText(hwnd, IDD_TMPATH, szTMPathField, MAXPATHFIELD);
                 SaveRecentFileList (hwnd, szTMPathField, IDD_TMPATH);
                 HideWindow(hwnd);
              }

           }
           break;

Switchem:

        case IDD_SWITCH:
            HideTasklist();
            ActivateSelectedWindow(hwndLB);
            break;

        case IDCANCEL:
           HideWindow(hwnd);
           break;

        case IDD_TERMINATE:
            /*
             * Get the hwnd of the item which was selected.
             */
            nIndex = (int)SendMessage(hwndLB, LB_GETCURSEL, 0, 0);
            hwndNext = (HWND)SendMessage(hwndLB, LB_GETITEMDATA, nIndex, 0);

            if (!IsWindow(hwndNext)) {
                HideWindow(hwnd);
                MessageBeep(0);
                break;
            }

            {
                /* Always activate the window first.  This prevents
                 * apps from going to Beep mode.  Failing to do this
                 * can cause re-entrancy problems in the app if we
                 * do this again before activating the app.
                 *
                 * However, don't do this if it is a old app task.
                 */

#ifdef WIN16    /* if NTWIN, then always do this, as is no winoldapp */
                if (!IsWinoldapTask(GetTaskFromHwnd(hwndNext)))
#endif
                HideWindow(hwnd);
                ActivateSelectedWindow(hwndLB);
                {
                    DWORD idt;
                    HANDLE hThread;

                    hThread = CreateThread(NULL, 0,
                             (LPTHREAD_START_ROUTINE)CallEndTask,
                             (LPVOID)hwndNext, 0,
                             &idt);

                    if (hThread == NULL) {
                        /*
                         * Can not create thread, just call EndTask
                         * syncronously
                         */
                        DoEndTask( hwndNext );
                    } else {
                        CloseHandle(hThread);
                    }
                }
            }
            break;

        case IDD_TILE:
        case IDD_CASCADE:
            {
                HWND hwndDesktop;

                HideWindow(hwnd);

                hwndDesktop = GetDesktopWindow();

                if (wParam == IDD_CASCADE) {
                    CascadeChildWindows(hwndDesktop, 0);

                } else {
                    /*
                     * If shift is down, tile vertically, else horizontally.
                     */
                    TileChildWindows(hwndDesktop, ((GetKeyState(VK_SHIFT) &
                            0x8000) ? MDITILE_HORIZONTAL : MDITILE_VERTICAL));
                }
                break;
            }

        case IDD_ARRANGEICONS:
            /*
             * Let's restore the saved bits before ArrangeIcons
             * FIX for Bug #4884; --SANKAR-- 10-02-89
             */
            HideWindow(hwnd);
            ArrangeIconicWindows(GetDesktopWindow());
            break;
        }

        break;


    case WM_CLOSE:
        /*
         * If wParam != 0, this is a shutdown request, so exit.
         */
        if (wParam != 0)
            ExitProcess(0);
        return FALSE;
        break;

    case WM_HOTKEY:
        if (wParam == 1) {
            pt.x = (dxScreen - dxTaskman) / 2;
            pt.y = (dyScreen - dyTaskman) / 2;
            ShowTasklist(pt);
        }
        break;

    case WM_LOGOFF:
        PostQuitMessage(0);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

//*************************************************************
//
//  SetDefButton()
//
//  Purpose:    Sets the default button
//
//  Parameters: HWND hDlg     - Window handle of dialog box
//              INT  idButton - ID of button
//
//  Return:     void
//
//*************************************************************

VOID SetDefButton(HWND hwndDlg, INT  idButton)
{
    LRESULT lr;

    if (HIWORD(lr = SendMessage(hwndDlg, DM_GETDEFID, 0, 0)) == DC_HASDEFID)
    {
        HWND hwndOldDefButton = GetDlgItem(hwndDlg, LOWORD(lr));

        SendMessage (hwndOldDefButton,
                     BM_SETSTYLE,
                     MAKEWPARAM(BS_PUSHBUTTON, 0),
                     MAKELPARAM(TRUE, 0));
    }

    SendMessage( hwndDlg, DM_SETDEFID, idButton, 0L );
    SendMessage( GetDlgItem(hwndDlg, idButton),
                 BM_SETSTYLE,
                 MAKEWPARAM( BS_DEFPUSHBUTTON, 0 ),
                 MAKELPARAM( TRUE, 0 ));
}


BOOL
InitTaskman()
{
   WNDCLASS wc;

   /*
    * First set the priority of taskman so it is higher than foreground apps
    * that spin in loops - this way it'll always come up when you hit
    * ctrl-esc.
    */

   SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

   wc.style = 0;
   wc.lpfnWndProc = DefDlgProc;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = DLGWINDOWEXTRA;
   wc.hInstance = hAppInstance;
   wc.hIcon = LoadIcon(hAppInstance, MAKEINTRESOURCE(PROGMANICON));
   wc.hCursor = LoadCursor(NULL, IDC_ARROW);
   wc.hbrBackground = NULL;
   wc.lpszMenuName =  NULL;
   wc.lpszClassName = TEXT("TakoHachi");

   RegisterClass(&wc);

   {
       //
       // Set the working set size to 200k.
       //

       QUOTA_LIMITS QuotaLimits;
       NTSTATUS status;

       status = NtQueryInformationProcess( NtCurrentProcess(),
                                           ProcessQuotaLimits,
                                           &QuotaLimits,
                                           sizeof(QUOTA_LIMITS),
                                           NULL );
       if (NT_SUCCESS(status)) {
           QuotaLimits.MinimumWorkingSetSize = 300 * 1024;
           QuotaLimits.MaximumWorkingSetSize = 372 * 1024;

           NtSetInformationProcess( NtCurrentProcess(),
                                    ProcessQuotaLimits,
                                    &QuotaLimits,
                                    sizeof(QUOTA_LIMITS) );
       }
   }

   /*
    * Taskman will work in the windows directory, and switch to the
    * original directory (home directory) before execing programs.
    * This is to prevent weird popups if a UNC original directory is
    * disconnected.
    */

   GetCurrentDirectory(MAXPATHFIELD, szTMUserHomeDir);
   GetWindowsDirectory(szTMWindowsDirectory, MAXPATHFIELD);
   SetCurrentDirectory(szTMWindowsDirectory);

   if (fNoRun) {
      ghwndTMDialog = CreateDialog(hAppInstance, MAKEINTRESOURCE(WMPTASKMANDLG), NULL,
         (DLGPROC)TaskmanDlgProc);
   } else {
      ghwndTMDialog = CreateDialog(hAppInstance, MAKEINTRESOURCE(PWRTASKMANDLG), NULL,
         (DLGPROC)TaskmanDlgProc);
   }

   if (ghwndTMDialog == NULL)
       return(FALSE);

   LoadString(hAppInstance, IDS_OOMEXITTITLE, szTMOOMExitTitle, 32);
   LoadString(hAppInstance, IDS_OOMEXITMSG, szTMOOMExitMsg, 64);

   if (!RegisterHotKey(ghwndTMDialog, 1, MOD_CONTROL, VK_ESCAPE) ||
           !RegisterTasklist(ghwndTMDialog)) {

       DestroyWindow(ghwndTMDialog);
       return(FALSE);
   }

   return(TRUE);
}

VOID
TMMain()
{
    MSG msg;
    LPSTR  lpszCmdLine = NULL;
    int    nCmdShow = SW_SHOWNORMAL;

    if (InitTaskman()) {
        while (GetMessage(&msg, (HWND)NULL, (UINT)0, (UINT)0)) {
            if (!IsDialogMessage(ghwndTMDialog, &msg)) {
                if ((msg.message == WM_SYSCOMMAND) && (msg.wParam == SC_TASKLIST)) {
                    POINT pt;

                    GetCursorPos(&pt);
                    pt.x = max(pt.x - (dyTaskman / 2), 0);
                    pt.x = min(pt.x, dxScreen - dxTaskman);
                    pt.y = max(pt.y - (GetSystemMetrics(SM_CYCAPTION) * 2), 0);
                    pt.y = min(pt.y, dyScreen - dyTaskman);

                    ShowTasklist(pt);

                    continue;
                } else {

                    //
                    //  We need to have a regular message loop in order
                    //  to handle the DDE messages generated by spawning
                    //  an application via an association.
                    //

                    TranslateMessage (&msg);
                    DispatchMessage (&msg);
                }
            }
        }

    }
}
