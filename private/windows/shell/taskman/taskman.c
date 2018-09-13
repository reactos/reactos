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

#define UNICODE

#include "taskman.h"
#include <port1632.h>
#include <shellapi.h>
#include <shlapip.h>

//LATER find correct define for NT
#if !defined(NTWIN) && !defined(DOSWIN32) && !defined(WIN16)
#define NTWIN   1
#endif

#define MAXPATHFIELD 260

#define INIT_MAX_FILES 4
#define FILES_KEY  L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Program Manager\\Recent File List"
#define MAXFILES_ENTRY L"Max Files"
#define FILE_ENTRY L"File%lu"

#define WM_CONTENTSCHANGED (WM_USER+5)

TCHAR szPathField[MAXPATHFIELD];
TCHAR szDirField[MAXPATHFIELD];
TCHAR szTitle[MAXPATHFIELD];
TCHAR szMessage[MAXMSGBOXLEN];

TCHAR szUserHomeDir[MAXPATHFIELD];
TCHAR szWindowsDirectory[MAXPATHFIELD];

TCHAR szOOMExitMsg[64] = TEXT("Close an application and try again."); // 64
TCHAR szOOMExitTitle[32] = TEXT("Extremely Low on Memory"); // 32

TCHAR szNoRun[] = TEXT("NoRun");
// registry key for groups

HANDLE hInst;

BOOL bChangedDefaultButton;

LONG APIENTRY TaskmanDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);

WORD APIENTRY ExecProgram(LPTSTR lpszPath,LPTSTR lpDir,LPTSTR lpTitle);
void APIENTRY SaveRecentFileList (HWND, LPTSTR);

VOID GetPathInfo(PTSTR szPath,PTSTR *pszFileName,PTSTR *pszExt,WORD *pich,BOOL *pfUnc);
VOID GetFilenameFromPath(PTSTR szPath, PTSTR szFilename);
VOID GetDirectoryFromPath(PTSTR szFilePath, PTSTR szDir);
BOOL IsUserAdmin();
BOOL OKToExec();
BOOL TestTokenForAdmin(HANDLE Token);
VOID SetDefButton(HWND hwndDlg, INT  idButton);

WINUSERAPI VOID SwitchToThisWindow(HWND, BOOL);
INT MyMessageBox(HWND hWnd,WORD idTitle,WORD idMessage,PWSTR psz,WORD wStyle);

INT MyX = 0;
INT MyY = 0;
INT dxTaskman;
INT dyTaskman;
INT dxScreen;
INT dyScreen;

HWND ghwndDialog;
BOOL fExecOK = TRUE;
BOOL fMsgBox = FALSE;


PVOID Alloc(
    DWORD   Bytes)
{
    HANDLE  hMem;
    PVOID   Buffer;

    hMem = LocalAlloc(LMEM_MOVEABLE, Bytes + sizeof(hMem));

    if (hMem == NULL) {
        return(NULL);
    }

    Buffer = LocalLock(hMem);
    if (Buffer == NULL) {
        LocalFree(hMem);
        return(NULL);
    }

    *((PHANDLE)Buffer) = hMem;

    return (PVOID)(((PHANDLE)Buffer)+1);
}


BOOL Free(
    PVOID   Buffer)
{
    HANDLE  hMem;

    hMem = *(((PHANDLE)Buffer) - 1);

    LocalUnlock(hMem);

    return(LocalFree(hMem) == NULL);
}


VOID
HideWindow(HWND hwnd)
{
   if (!fMsgBox) {

      if (fExecOK) {
         SetDlgItemText(ghwndDialog, IDD_PATH, TEXT(""));
      }

      // redundant?  why do they do the reverse twice for show below?
      ShowWindow(ghwndDialog, SW_HIDE);

      SetWindowPos(ghwndDialog, HWND_NOTOPMOST, 0, 0, 0, 0,
         SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
   }

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

/*
 * We call HideTasklist() when we want to remove the tasklist window
 * from the screen but not select another window (ie. when we're about
 * to select another app.  We call ShowWindow(SW_HIDE) directly when
 * we're doing something like tiling or cascading so a window other than
 * the tasklist will become the foreground window.
 */
VOID HideTasklist(VOID)
{
    if (fExecOK) {
       SetDlgItemText(ghwndDialog, IDD_PATH, TEXT(""));
    }

    SetWindowPos(ghwndDialog, HWND_TOP, 0, 0, 0, 0, SWP_HIDEWINDOW |
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);

}


VOID ShowTasklist(
   POINT pt)
{
    /*
     * Retract the drop down listbox.
     */

     if (fExecOK) {
        SendDlgItemMessage(ghwndDialog, IDD_PATH,
                           CB_SHOWDROPDOWN,0,0);
     }

    SetWindowPos(ghwndDialog, HWND_TOPMOST, pt.x, pt.y, 0, 0,
       SWP_NOSIZE | SWP_NOACTIVATE );
    SetForegroundWindow(ghwndDialog);

    ShowWindow(ghwndDialog, SW_NORMAL);
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

#ifdef NTWIN
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


   if (!EndTask(hwnd, FALSE, FALSE)) {
      /* App does not want to close, ask user if
       * he wants to blow it away
       */

       InternalGetWindowText(hwnd, (LPTSTR)szTempField, MAXTASKNAMELEN);

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

    ExitThread(0);
    return 0; /* placate compiler */
}
#endif


/*** TaskmanDlgProc --         Dialog Procedure for Taskman Window
 *
 *
 *
 * TaskmanDlgProc(HWND hDlg, WORD wMSG, DWORD wParam, LPARAM lParam)
 *
 * ENTRY -         HWND hhDlg                 - handle to dialog box.
 *                        WORD wMsg                  - message to be acted upon.
 *                        DWORD wParam        - value specific to wMsg.
 *                        LPARAM lParam                - value specific to wMsg.
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
    LPARAM lParam)
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
#ifndef NTWIN
    LONG lTemp;
#endif

    hwndLB = GetDlgItem(hwnd, IDD_TASKLISTBOX);

    switch (wMsg) {

    case WM_INITDIALOG:
        /*
         * call private api to mark task man as a system app. This causes
         * it to be killed after all other non-system apps during shutdown.
         */
//      MarkProcess(MP_SYSTEMAPP);
        GetWindowRect(hwnd, &rc);
        dxTaskman = rc.right - rc.left;
        dyTaskman = rc.bottom - rc.top;
        dxScreen = GetSystemMetrics(SM_CXSCREEN);
        dyScreen = GetSystemMetrics(SM_CYSCREEN);

        pt.x = (dxScreen - dxTaskman) / 2;
        pt.y = (dyScreen - dyTaskman) / 2;

        SetWindowPos(hwnd, HWND_NOTOPMOST, pt.x, pt.y, 0, 0,
           SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

        SendDlgItemMessage(hwnd, IDD_PATH, EM_LIMITTEXT, MAXPATHFIELD-4, 0L);
        szPathField[0] = TEXT('\0');

        bChangedDefaultButton = FALSE;

        return FALSE;

    case WM_SHOWWINDOW:
        /*
         * If we're being shown fill in the listbox.  We do this here
         * rather than in WM_ACTIVATE process so we can do it while the
         * dialog is still invisible.
         */
        if (wParam != 0) {
            DWORD pidTaskMan = GetCurrentProcessId();

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
                    DWORD pidNext;
                    GetWindowThreadProcessId(hwndNext, &pidNext);
                    if (pidNext != pidTaskMan) {
                        if (InternalGetWindowText(hwndNext, (LPTSTR)szTempField,
                                MAXTASKNAMELEN)) {
                            nIndex = (int)SendMessage(hwndLB, LB_ADDSTRING, 0,
                                    (DWORD_PTR)(LPTSTR)szTempField);
                            SendMessage(hwndLB, LB_SETITEMDATA, nIndex,
                                    (DWORD_PTR)hwndNext);
                        }
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

            if (GetDlgItem(hwnd, IDD_PATH)) {

                //
                // FIrst empty the combo box from the last time.
                //

                SendDlgItemMessage (hwnd, IDD_PATH,
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

                                    SendDlgItemMessage (hwnd, IDD_PATH,
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

#ifdef JAPAN // bug fix
    //
    // Do nothing. Let the progman main thread do the work.
    //
#else // not JAPAN
    case WM_WININICHANGE:
        //
        // Check if the user's environment variables have changed, if so
        // regenerate the environment, so that new apps started from
        // taskman will have the latest environment.
        //
        if (lParam && (!lstrcmpi((LPTSTR)lParam, (LPTSTR) TEXT("Environment")))) {
            PVOID pEnv;

            RegenerateUserEnvironment(&pEnv, TRUE);
            break;
        }
        else {
            return FALSE;
        }
#endif // JAPAN

    case WM_CONTENTSCHANGED:
       if (fExecOK) {
          if (GetDlgItemText(hwnd, IDD_PATH, (LPTSTR)szPathField, MAXPATHFIELD)) {
             EnableWindow(GetDlgItem(hwnd, IDD_RUN), TRUE);
             if (!bChangedDefaultButton) {
                SetDefButton(hwnd,IDD_RUN);
                bChangedDefaultButton = TRUE;
             }
          } else {
             EnableWindow(GetDlgItem(hwnd, IDD_RUN), FALSE);
             if (bChangedDefaultButton) {
                SetDefButton(hwnd,IDD_SWITCH);
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

            default:
                return FALSE;
            }
            break;

        case IDD_PATH:
           PostMessage (hwnd, WM_CONTENTSCHANGED, 0, 0);
           break;


        case IDOK:
           if (!bChangedDefaultButton) {
              goto Switchem;
           }

        case IDD_RUN:
           if (fExecOK) {
              TCHAR szFilename[MAXPATHFIELD+4];
              WORD ret;

              GetDlgItemText(hwnd, IDD_PATH, szPathField, MAXPATHFIELD);
              DoEnvironmentSubst(szPathField, MAXPATHFIELD);
              GetDirectoryFromPath(szPathField, szDirField);
              if (*szDirField) {
                  // Convert path into a .\foo.exe style thing.
                  lstrcpy(szFilename, L".\\");
                  // Tag the filename and params on to the end of the dot slash.
                  GetFilenameFromPath(szPathField, szFilename+2);
                  if (*(szFilename+2) == L'"' ) {
                      SheRemoveQuotes(szFilename+2);
                      CheckEscapes(szFilename, sizeof(szFilename));
                  }
              }
              else {
                  GetFilenameFromPath(szPathField, szFilename);
              }

              ret = ExecProgram(szFilename, szDirField, szFilename);

              if (ret) {
                 fMsgBox = TRUE;
                 MyMessageBox( hwnd, IDS_EXECERRTITLE, ret, szPathField,
                    MB_SYSTEMMODAL | MB_OK | MB_ICONEXCLAMATION );
                 fMsgBox = FALSE;

                 SetFocus(GetDlgItem(hwnd, IDD_PATH));
              } else {
                 GetDlgItemText(hwnd, IDD_PATH, szPathField, MAXPATHFIELD);
                 SaveRecentFileList (hwnd, szPathField);
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

#ifndef NTWIN   /* Since NTWIN uses WM_ENDSESSION to kill the app,
                 * It is OK to kill it when it is disabled
                 */
            /*
             * Test if the toplevel window is disabled. If it is
             * diabled, we don't want to send a WM_CLOSE message to
             * the parent. This is because the app could have a dialog
             * box up and it's not expecting a CLOSE message...
             * Nasty rips can happen... Instead, active the window so
             * that the user can dismis any dialog box or fix whatever
             * is causing the top level window to be disabled...
             */
            lTemp = GetWindowLong(hwndNext, GWL_STYLE);
            if (lTemp & WS_DISABLED) {
                HideTasklist();
                MessageBeep(0);
                ActivateSelectedWindow(hwndLB);

            } else
#endif
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
#ifdef NTWIN
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
#else
                EndTask(hwndNext, FALSE, FALSE);
#endif
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

    lParam;
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


/*** SaveRecentFileList --    Save the list of recently used files
 *
 * void APIENTRY SaveRecentFileList (HWND hwnd, LPTSTR szCurrentFile);
 *
 *
 *
 * ENTRY - HWND   hwnd            - handle to dialog box.
 *         LPTSTR szCurrentFile   - pointer to selected filename
 *
 * EXIT  -
 * SYNOPSIS -
 *
 * WARNINGS -
 * EFFECTS  -
 *
 */


void APIENTRY SaveRecentFileList (HWND hwnd, LPTSTR szCurrentFile)
{
    HKEY  hKey;
    DWORD dwDisp;
    DWORD dwDataType, dwMaxFiles=INIT_MAX_FILES, dwMaxFilesSize, dwCount;
    TCHAR szFileEntry[20];
    DWORD dwEnd=0;
    DWORD dwFileNum=0;
    DWORD dwDup;
    static TCHAR szRecentFilePath[MAXPATHFIELD+1];

    //
    // Open registry key
    //

    if ( RegCreateKeyEx (HKEY_CURRENT_USER, FILES_KEY, 0, 0,
                             REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                             NULL, &hKey, &dwDisp) != ERROR_SUCCESS) {
        return;
    }


    //
    // Query the max number of files to save first.
    //

    dwMaxFilesSize = sizeof (DWORD);

    RegQueryValueEx (hKey, MAXFILES_ENTRY, NULL, &dwDataType,
                    (LPBYTE)&dwMaxFiles, &dwMaxFilesSize);

    //
    // If the user request 0 entries, then exit now.
    //
    if (dwMaxFiles == 0) {
        RegCloseKey (hKey);
        return;
    }

    //
    // Find out how many items are in the list box.
    //

    dwEnd = (DWORD)SendDlgItemMessage (hwnd, IDD_PATH, CB_GETCOUNT, 0, 0);

    //
    // If the max number of items we want to save is less than the
    // number of entries, then change the ending point.
    //

    if (dwMaxFiles < dwEnd) {
        dwEnd = dwMaxFiles;
    }

    //
    // Add the first entry (the current file)
    //

    wsprintf (szFileEntry, FILE_ENTRY, dwFileNum++);
    dwMaxFilesSize = MAXPATHFIELD+1;

    RegSetValueEx (hKey, szFileEntry, 0, REG_SZ, (CONST BYTE *)szCurrentFile,
                   sizeof (TCHAR) * (lstrlen (szCurrentFile)+1));


    //
    // Check for a duplicate string.
    //

    dwDup = (DWORD)SendDlgItemMessage (hwnd, IDD_PATH, CB_FINDSTRING,
                                (WPARAM) -1, (LPARAM) szCurrentFile);

    //
    // If we already have dwMaxFiles in the list and we don't have any
    // duplicates, then we only want to save dwMaxFiles - 1 entries
    // (drop the last entry).
    //
    //

    if ( (dwEnd == dwMaxFiles) && (dwDup == CB_ERR) ) {
        dwEnd--;
    }

    //
    // Now loop through the remaining entries
    //

    for (dwCount=0; dwCount < dwEnd; dwCount++) {

        //
        // Check to see if we are at the duplicate entry.  If
        // so skip on to the next item.
        //

        if ((dwDup != CB_ERR) && (dwCount == dwDup)) {
            continue;
        }

        //
        // Get an entry out of the listbox.
        //

        SendDlgItemMessage (hwnd, IDD_PATH, CB_GETLBTEXT, (WPARAM) dwCount,
                            (LPARAM) szRecentFilePath);

        //
        // If we get a NULL string, break out of the loop.
        //

        if (!(*szRecentFilePath) || !szRecentFilePath) {
            break;
        }

        //
        // Build the entry name
        //

        wsprintf (szFileEntry, FILE_ENTRY, dwFileNum);
        dwMaxFilesSize = MAXPATHFIELD+1;

        //
        // Save the entry
        //

        RegSetValueEx (hKey, szFileEntry, 0, REG_SZ,(CONST BYTE *) szRecentFilePath,
                       sizeof (TCHAR) * (lstrlen (szRecentFilePath)+1));

        //
        // Increment our current file number
        //

        dwFileNum++;
    }

    //
    // Close the key
    //

    RegCloseKey (hKey);

}



/*** Main --         Program entry point (was WinMain).
 *
 *
 *
 * Main(int argc, char *argv[], char *envp[])
 *
 * ENTRY -         int argc                - argument count.
 *                        char *argv[]        - argument list.
 *                        char *envp[]        - environment.
 *
 * EXIT  -           TRUE if success, FALSE if not.
 * SYNOPSIS -  Parses command line, for position to place dialog box, if no
 *                                position (came from ctl/esc) then center on screen.
 *                                Also make sure only one instance of taskman.
 *
 * WARNINGS -
 * EFFECTS  -
 */

INT __cdecl main(
   INT argc,
   CHAR *argv[],
   CHAR *envp[])
{
   MSG msg;

   /*
    * First set the priority of taskman so it is higher than foreground apps
    * that spin in loops - this way it'll always come up when you hit
    * ctrl-esc.
    */
   hInst = GetModuleHandle(NULL);

   SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

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

   GetCurrentDirectory(MAXPATHFIELD, szUserHomeDir);
   GetWindowsDirectory(szWindowsDirectory, MAXPATHFIELD);
   SetCurrentDirectory(szWindowsDirectory);

   fExecOK = OKToExec();
   if (!IsUserAdmin() && !fExecOK) {
      ghwndDialog = CreateDialog(hInst, MAKEINTRESOURCE(WMPTASKMANDLG), NULL,
         (DLGPROC)TaskmanDlgProc);
   } else {
      ghwndDialog = CreateDialog(hInst, MAKEINTRESOURCE(PWRTASKMANDLG), NULL,
         (DLGPROC)TaskmanDlgProc);
   }

   if (ghwndDialog == NULL)
       return 0;

   LoadString(hInst, IDS_OOMEXITTITLE, szOOMExitTitle, 32);
   LoadString(hInst, IDS_OOMEXITMSG, szOOMExitMsg, 64);

   if (!RegisterHotKey(ghwndDialog, 1, MOD_CONTROL, VK_ESCAPE) ||
           !RegisterTasklist(ghwndDialog)) {
       goto exit;
   }

   while (GetMessage(&msg, (HWND)NULL, (UINT)0, (UINT)0)) {
       if (!IsDialogMessage(ghwndDialog, &msg)) {
          if ((msg.message == WM_SYSCOMMAND) && (msg.wParam == SC_TASKLIST)) {
             POINT pt;

             GetCursorPos(&pt);
             pt.x = max(pt.x - (dyTaskman / 2), 0);
             pt.x = min(pt.x, dxScreen - dxTaskman);
             pt.y = max(pt.y - (GetSystemMetrics(SM_CYCAPTION) * 2), 0);
             pt.y = min(pt.y, dyScreen - dyTaskman);

             ShowTasklist(pt);
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

exit:
    DestroyWindow(ghwndDialog);
    return 0;

    argc;
    argv;
    envp;
}


WORD APIENTRY
ExecProgram(
    LPTSTR lpszPath,
    LPTSTR lpDir,
    LPTSTR lpTitle
    )
{
  WORD      ret;
  HCURSOR   hCursor;
  LPTSTR     lpP;
  TCHAR cSeparator;
  TCHAR lpReservedFormat[] = TEXT("dde.%d,hotkey.%d");
  TCHAR lpReserved[100];  // used for DDE request of icons from console apps
                         // add for passing the hotkey associated with an item.
  HANDLE hProcess;

  ret = 0;

  /*
   * Set the current directory to the user's home directory if possible.
   */
  SetCurrentDirectory(szUserHomeDir);

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

  /*changed order, since wPendINstance is a 32b HANDLE, and ret is WORD*/
    if (!lpP)
        lpP = TEXT("");

    wsprintf(lpReserved, lpReservedFormat, 0, 0);

    ret = (WORD)RealShellExecute(ghwndDialog, NULL, lpszPath, lpP,
                            lpDir, NULL, lpTitle, lpReserved,
                            (WORD)SW_SHOWNORMAL, &hProcess);

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
          ret = 0;
          break;
  }

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

  /*
   * Reset the working directory to the windows directory.
   */
  SetCurrentDirectory(szWindowsDirectory);

  return(ret);
}


VOID
GetDirectoryFromPath(
   PTSTR szFilePath,
   PTSTR szDir)
{
   PTSTR pFileName;
   PTSTR pExt;
   WORD ich;
   BOOL fUnc;

   *szDir = TEXT('\0');

   /* Get info about file path. */
   GetPathInfo(szFilePath, &pFileName, &pExt, &ich, &fUnc);

   /* UNC paths don't (conceptually to Progman) have a directory component. */
   if (fUnc)
      return;

   /* Does it have a directory component ? */
   if (pFileName != szFilePath) { // Yep.
      /* copy path to temp. */
      if (*szFilePath == TEXT('"')) {
         szFilePath++;
      }
      lstrcpy(szDir, szFilePath);
      /* check path style. */
      if (ich <= 3 && *(szDir+1) == TEXT(':')) {
         /*
          * The path is "c:\foo.c" or "c:foo.c" style.
          * Don't remove the last slash/colon, just the filename.
          */
         szDir[pFileName-szFilePath] = TEXT('\0');
      }

      else if (ich == 1) {
         /*
          * something like "\foo.c"
          * Don't remove the last slash/colon, just the filename.
          */
          szDir[pFileName-szFilePath] = TEXT('\0');
       }
       else {
          /*
           * The filepath is a full normal path.
           * Could be something like "..\foo.c" or ".\foo.c" though.
           * Stomp on the last slash to get just the path.
           */
           szDir[pFileName-szFilePath-1] = TEXT('\0');
       }
    }

    /* else just a filename with no path. */
}

VOID
GetFilenameFromPath(
   PTSTR szPath,
   PTSTR szFilename)
{
   DWORD dummy;
   PTSTR pFileName;
   BOOL fUNC;

   GetPathInfo(szPath, &pFileName, (PTSTR*) &dummy, (WORD*) &dummy,
      &fUNC);

   /* If it's a UNC then the 'filename' part is the whole thing. */
   if (fUNC || (szPath == pFileName))
      lstrcpy(szFilename, szPath);
   else {
      if (*szPath == TEXT('"')) {
         *szFilename++ = TEXT('"');
      }
      lstrcpy(szFilename, pFileName);
   }
}


VOID
GetPathInfo(
   PTSTR szPath,
   PTSTR *pszFileName,
   PTSTR *pszExt,
   WORD *pich,
   BOOL *pfUnc)
{
   TCHAR *pch;          // Temp variable.
   WORD ich = 0;       // Temp.
   BOOL InQuotes;

   *pszExt = NULL;         // If no extension, return NULL.
   *pszFileName = szPath;  // If no seperate filename component, return path.
   *pich = 0;
   *pfUnc = FALSE;         // Default to not UNC style.

   //
   // Check if path is in quotes.
   //
   if (InQuotes = (*szPath == TEXT('"'))) {
      szPath++;
   }

   // Check for UNC style paths.
   if (*szPath == TEXT('\\') && *(szPath+1) == TEXT('\\'))
      *pfUnc = TRUE;

   // Search forward to find the last backslash or colon in the path.
   // While we're at it, look for the last dot.
   for (pch = szPath; *pch; pch = CharNext(pch)) {

      if ((*pch == TEXT(' ')) && (!InQuotes)) {
         // Found a space - stop here.
         break;
      }
      if (*pch == TEXT('"')) {
         // Found a the second quote - stop here.
         pch++;
         break;
      }
      if (*pch == TEXT('\\') || *pch == TEXT(':')) {
         // Found it, record ptr to it and it's index.
         *pszFileName = pch+1;
         *pich = ich + (WORD)1;
      }
      if (*pch == TEXT('.')) {
         // Found a dot.
         *pszExt = pch;
      }
      ich++;
   }

   /* Check that the last dot is part of the last filename. */
   if (*pszExt < *pszFileName)
      *pszExt = NULL;
}


BOOL
IsUserAdmin()
{
    BOOL UserIsAdmin = FALSE;
    HANDLE Token;
    PSID    AdminAliasSid = NULL;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    //
    // Get the token of the current process.
    //

    if (!OpenProcessToken(
          GetCurrentProcess(),
          TOKEN_QUERY,
          &Token) ) {
       return(FALSE);
    }

    UserIsAdmin = TestTokenForAdmin(Token);


    CloseHandle(Token);

    return(UserIsAdmin);
}

BOOL
TestTokenForAdmin(
   HANDLE Token
   )
{
   NTSTATUS    Status;
   DWORD       InfoLength;
   PTOKEN_GROUPS TokenGroupList;
   DWORD       GroupIndex;
   PSID        AdminSid;
   BOOL        FoundAdmin;
   SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;

   //
   // Get a list of groups in the token
   //

   Status = NtQueryInformationToken(
      Token,                    // Handle
      TokenGroups,              // TokenInformationClass
      NULL,                     // TokenInformation
      0,                        // TokenInformationLength
      &InfoLength               // ReturnLength
   );

   if ((Status != STATUS_SUCCESS) && (Status != STATUS_BUFFER_TOO_SMALL)) {
      return(FALSE);
   }

   TokenGroupList = Alloc(InfoLength);

   if (TokenGroupList == NULL) {
      return(FALSE);
   }

   Status = NtQueryInformationToken(
      Token,                    // Handle
      TokenGroups,              // TokenInformationClass
      TokenGroupList,           // TokenInformation
      InfoLength,               // TokenInformationLength
      &InfoLength               // ReturnLength
   );

   if (!NT_SUCCESS(Status)) {
      LocalFree(TokenGroupList);
      return(FALSE);
   }

   //
   // Create the admin sid
   //
   Status = RtlAllocateAndInitializeSid(
      &SystemSidAuthority, 2,
      SECURITY_BUILTIN_DOMAIN_RID,
      DOMAIN_ALIAS_RID_ADMINS,
      0, 0, 0, 0, 0, 0,
      &AdminSid);

   if (!NT_SUCCESS(Status)) {
      Free(TokenGroupList);
      return(FALSE);
   }

   //
   // Search group list for admin alias
   //
   FoundAdmin = FALSE;

   for (GroupIndex=0; GroupIndex < TokenGroupList->GroupCount; GroupIndex++ ) {
      if (RtlEqualSid(TokenGroupList->Groups[GroupIndex].Sid, AdminSid)) {
         FoundAdmin = TRUE;
         break;
      }
   }

   //
   // Tidy up
   //

   RtlFreeSid(AdminSid);
   Free(TokenGroupList);

   return(FoundAdmin);
}

BOOL
OKToExec()
{
   TCHAR szRestrict[]        = TEXT("Restrictions");
   TCHAR szProgramManager[]  = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Program Manager");
   HKEY hkeyProgramManager  = NULL;  // progman.ini key
   HKEY hkeyPMRestrict      = NULL;
   DWORD cbData, dwType;
   BOOL fNoRun = FALSE;

  /*
   * Create/Open the registry keys corresponding to progman.ini sections.
   */
  if (!RegCreateKeyEx(HKEY_CURRENT_USER, szProgramManager, 0, szProgramManager, 0,
                         KEY_READ | KEY_WRITE, NULL, &hkeyProgramManager, NULL)) {

      RegCreateKeyEx(hkeyProgramManager, szRestrict, 0, szProgramManager, 0,
                         KEY_READ, NULL, &hkeyPMRestrict, NULL);

  } else {
    return(FALSE);
  }

  if (hkeyPMRestrict) {
      cbData = sizeof(fNoRun);
      RegQueryValueEx(hkeyPMRestrict, szNoRun, 0, &dwType, (LPBYTE)&fNoRun,
         &cbData);
  }

  if (hkeyPMRestrict) {
      RegCloseKey(hkeyPMRestrict);
      hkeyPMRestrict = NULL;
  }

  RegCloseKey(hkeyProgramManager);

  return(!fNoRun);

}

INT
MyMessageBox(
   HWND hWnd,
   WORD idTitle,
   WORD idMessage,
   PWSTR psz,
   WORD wStyle)
{
    WCHAR    szTempField[MAXMSGBOXLEN];
    INT     iMsgResult;

    if (!LoadString(hInst, idTitle, szTitle, sizeof(szTitle))){
        goto MessageBoxOOM;
    }
    if (idMessage < 32){
        if (!LoadString(hInst, IDS_UNKNOWNMSG, szTempField, sizeof(szTempField))){
            goto MessageBoxOOM;
        }
        wsprintf(szMessage, szTempField, idMessage);
    }
    else{
        if (!LoadString(hInst, idMessage, szTempField, sizeof(szTempField)))
            goto MessageBoxOOM;

        if (psz) {
            wsprintf(szMessage, szTempField, (LPTSTR)psz);
        }
        else
            lstrcpy(szMessage, szTempField);
    }

    iMsgResult = MessageBox(hWnd, szMessage, szTitle, wStyle );

    if (iMsgResult == -1){

MessageBoxOOM:
        MessageBox(hWnd, szOOMExitMsg, szOOMExitTitle, MB_SYSTEMMODAL | MB_ICONHAND | MB_OK);
    }

    return(iMsgResult);
}
