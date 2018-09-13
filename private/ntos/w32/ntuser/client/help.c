/**************************************************************************\
* Module Name: help.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* History:
* 23-May-95 BradG   Created to consolicate client-side help routines.
*
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop


#define MAX_ATTEMPTS    5       // maximum -1 id controls to search through
char szDefaultHelpFileA[] = "windows.hlp";

PWCHAR szEXECHELP = TEXT("\\winhlp32 - ");
PWCHAR szMS_WINHELP =     L"MS_WINHELP";    // Application class
PWCHAR szMS_POPUPHELP =   L"MS_POPUPHELP";  // Popup class
PWCHAR szMS_TCARDHELP =   L"MS_TCARDHELP";  // Training card class

// These are in winhelp.h in Chicago
#define HLP_POPUP               'p'     // Execute WinHelp as a popup
#define HLP_TRAININGCARD        'c'     // Execute WinHelp as a training card
#define HLP_APPLICATION         'x'     // Execute WinHelp as application help


/***************************************************************************\
* SendWinHelpMessage
*
* Attempts to give the winhelp process the right to take the
*  foreground (it will fail if the calling processs doesn't have
*  the right itself). Then it sends the WM_WINHELP message
*
* History:
* 02-10-98 GerardoB     Created
\***************************************************************************/
LRESULT SendWinHelpMessage (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    DWORD dwProcessId = 0;
    GetWindowThreadProcessId(hwnd, &dwProcessId);
    AllowSetForegroundWindow(dwProcessId);
    return SendMessage(hwnd, WM_WINHELP, wParam, lParam);
}
/***************************************************************************\
* HFill
*
* Builds a data block for communicating with help
*
* LATER 13 Feb 92 GregoryW
* This needs to stay ANSI until we have a Unicode help engine
*
* History:
* 04-15-91 JimA Ported.
* 03-24-95 BradG - YAP of Win95 code.  Added code to prevent memory
*                  overwrite on bad ulData == 0 parameter.
\***************************************************************************/

LPHLP HFill(
    LPCSTR lpszHelp,
    DWORD  ulCommand,        // HELP_ constant
    ULONG_PTR ulData)
{
    DWORD   cb;     // Size of the data block
    DWORD   cbStr;  // Length of the help file name
    DWORD   cbData; // Size of the dwData parameter in bytes (0 if not used)
    LPHLP   phlp;   // Pointer to data block
    BYTE    bType;  // dwData parameter type

    /*
     * Get the length of the help file name
     */
    cbStr = (lpszHelp) ? strlen(lpszHelp) + 1 : 0;

    /*
     * Get the length of any dwData parameters
     */
    bType = HIBYTE(LOWORD(ulCommand));
    if (ulData) {
        switch (bType) {
        case HIBYTE(HELP_HB_STRING):
            /*
             * ulData is an ANSI string, so compute its length
             */
            cbData = strlen((LPSTR)ulData) + 1;
            break;

        case HIBYTE(HELP_HB_STRUCT):
            /*
             * ulData points to a structure who's first member is
             * an int that contains the size of the structure in bytes.
             */
            cbData = *((int *)ulData);
            break;

        default:
            /*
             * dwData has no parameter
             */
            cbData = 0;
        }
    } else {
        /*
         * No parameter is present
         */
        cbData = 0;
    }

    /*
     * Calculate size (NOTE: HLP is called WINHLP in Win95)
     */
    cb = sizeof(HLP) + cbStr + cbData;

    /*
     * Get data block
     */
    if ((phlp = (LPHLP)LocalAlloc(LPTR, cb)) == NULL)
        return NULL;

    /*
     * Fill in info
     */
    phlp->cbData = (WORD)cb;
    phlp->usCommand = (WORD)ulCommand;
    phlp->ulReserved = 0;
    // phlp->ulTopic = 0;

    /*
     * Fill in file name
     */
    if (lpszHelp) {
        phlp->offszHelpFile = sizeof(HLP);  // NOTE: HLP is called WINHLP in Win95
        strcpy((LPSTR)(phlp + 1), lpszHelp);
    } else {
        phlp->offszHelpFile = 0;
    }

    /*
     * Fill in data
     */
    switch (bType) {
    case HIBYTE(HELP_HB_STRING):
        if (cbData) {
            phlp->offabData = (WORD)(sizeof(HLP) + cbStr);  // NOTE: HLP is called WINHLP in Win95
            strcpy((LPSTR)phlp + phlp->offabData, (LPSTR)ulData);
        } else {
            phlp->offabData = 0;
        }
        break;

    case HIBYTE(HELP_HB_STRUCT):
        if (cbData) {
            phlp->offabData = (WORD)(sizeof(HLP) + cbStr);  // NOTE: HLP is called WINHLP in Win95
            RtlCopyMemory((LPBYTE)phlp + phlp->offabData, (PVOID)ulData,
                    *((int far *)ulData));
        } else {
            phlp->offabData = 0;
        }
        break;

    default:
        phlp->offabData = 0;
// BradG - This item is named differently in the Win95 WINHLP structure
//      phlp->ctx   = ulData;
        phlp->ulTopic = ulData;
        break;
    }

    return(phlp);
}


/***************************************************************************\
* LaunchHelper
*
* This function launches the WinHlp32 executable with the correct command
* line arguments.
*
* History:
*   3/23/95 BradG   YAP (yet another port) of changes from Win95.
\***************************************************************************/
BOOL LaunchHelper(LPWSTR lpfile, DWORD dwType)
{
    int                 cchLen;
    int                 cchShift;
    DWORD               idProcess;
    STARTUPINFO         StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    PWCHAR pwcExecHelp;

    pwcExecHelp = szEXECHELP;

    /*
     *  Are we at the root?? If so, skip over leading backslash in text string
     */
    if (*lpfile) {
        cchLen = wcslen(lpfile);
        cchShift = (lpfile[cchLen - 1] == TEXT('\\')) ? 1 : 0;
        wcscat(lpfile, pwcExecHelp + cchShift);
    } else {
        wcscat(lpfile, pwcExecHelp + 1);
    }


    /*
     * Defaultly send "winhlp32 -x" or adjust the last flag character
     */
    switch (dwType) {
    case TYPE_POPUP:
        lpfile[wcslen(lpfile)-1] = TEXT(HLP_POPUP);
        break;

    case TYPE_TCARD:
        lpfile[wcslen(lpfile)-1] = TEXT(HLP_TRAININGCARD);
        break;

    default:
        lpfile[wcslen(lpfile)-1] = TEXT(HLP_APPLICATION);
        break;
    }

    /*
     *  Launch winhelp
     */
    memset(&StartupInfo,0,sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.wShowWindow = SW_SHOW;
    StartupInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_FORCEONFEEDBACK;

    idProcess = (DWORD)CreateProcessW(NULL, lpfile,
            NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &StartupInfo,
            &ProcessInformation);

    if (idProcess) {
        WaitForInputIdle(ProcessInformation.hProcess, 10000);
        NtClose(ProcessInformation.hProcess);
        NtClose(ProcessInformation.hThread);
        return TRUE;
    }

    return FALSE;
}


/***************************************************************************\
* LaunchHelp
*
* Traverse the Windows, System and %PATH% directories attempting to launch
* the WinHelp32 application.
*
* History:
* 3/23/95 BradG     YAP of new changes from Win95
\***************************************************************************/

BOOL LaunchHelp(DWORD dwType)
{
    WCHAR wszPath[MAX_PATH];

    GetSystemWindowsDirectoryW(wszPath, MAX_PATH);

    if (LaunchHelper(wszPath, dwType))
        return TRUE;

    /*
     * Search the system directory (Not in Win95)
     */
    GetSystemDirectoryW(wszPath, MAX_PATH);
    if (LaunchHelper(wszPath, dwType))
        return TRUE;

    /*
     * Try the search path
     */
    wszPath[0] = L'\0';
    return LaunchHelper(wszPath, dwType);

}


/***************************************************************************\
* GetNextDlgHelpItem
*
* This is a reduced version of the GetNextDlgTabItem function that does not
* skip disabled controls.
*
* History:
* 3/25/95 BradG     Ported from Win95
\***************************************************************************/
PWND GetNextDlgHelpItem(PWND pwndDlg, PWND pwnd)
{
    PWND pwndSave;

    if (pwnd == pwndDlg)
        pwnd = NULL;
    else
    {
        pwnd = _GetChildControl(pwndDlg, pwnd);
        if (pwnd)
        {
            if (!_IsDescendant(pwndDlg, pwnd))
                return(NULL);
        }
    }

    /*
     *  BACKWARD COMPATIBILITY
     *
     *  Note that the result when there are no tabstops of
     *  IGetNextDlgTabItem(hwndDlg, NULL, FALSE) was the last item, now
     *  will be the first item.  We could put a check for fRecurse here
     *  and do the old thing if not set.
     */

    /*
     *  We are going to bug out if we hit the first child a second time.
     */
    pwndSave = pwnd;
    pwnd = _NextControl(pwndDlg, pwnd, CWP_SKIPINVISIBLE);

    while ((pwnd != pwndSave) && (pwnd != pwndDlg))
    {
        UserAssert(pwnd);

        if (!pwndSave)
            pwndSave = pwnd;

        if ((pwnd->style & (WS_TABSTOP | WS_VISIBLE))  == (WS_TABSTOP | WS_VISIBLE))
            /*
             *  Found it.
             */
            break;

        pwnd = _NextControl(pwndDlg, pwnd, CWP_SKIPINVISIBLE);
    }

    return(pwnd);
}


/***************************************************************************\
* HelpMenu
*
* History:
* 01-Feb-1994 mikeke    Ported.
\***************************************************************************/

UINT HelpMenu(
    HWND hwnd,
    PPOINT ppt)
{
    INT     cmd;
    HMENU   hmenu = LoadMenu( hmodUser, MAKEINTRESOURCE(ID_HELPMENU));

    if (hmenu != NULL) {
        cmd = TrackPopupMenu( GetSubMenu(hmenu, 0),
              TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_RIGHTBUTTON,
              ppt->x, ppt->y, 0, hwnd, NULL);
        NtUserDestroyMenu(hmenu);
        return cmd;
    }

    return (UINT)-1;
}

/***************************************************************************\
* FindWinHelpWindow
*
* This function attempts to locate the help window.  If it fails, it attempts
* to launch WinHlp32.exe and then look for its window.
*
* History:
* 03/24/95 BradG   Created by extracting code from xxxWinHelpA
\***************************************************************************/
HWND FindWinHelpWindow(
    LPCWSTR lpwstrHelpWindowClass,
    DWORD   dwType,
    BOOL    bLaunchIt
    )
{
    HWND    hwndHelp;

    /*
     *  Find the current help window.  If not found, try and launch
     *  the WinHlp32 application.  We are interested only in 32 bit help.
     *  Note that 16 bit apps don't walk this path, ntvdm taking care of
     *  starting the 16 bit help for them.
     */
    hwndHelp = InternalFindWindowExW(NULL, NULL, lpwstrHelpWindowClass, NULL, FW_32BIT);

    if (hwndHelp == NULL) {
        if (bLaunchIt) {
            /*
             *  Can't find it --> see if we want to launch it
             */
            if ((LaunchHelp(dwType) == FALSE) ||
                (hwndHelp = FindWindowEx(NULL, NULL, (LPWSTR)lpwstrHelpWindowClass, NULL)) == NULL)
              {

                /*
                 *  Can't find help, or not enough memory to load help.
                 *  pwndHelp will be NULL at this point.
                 */
#if DBG
                RIPMSG0( RIP_WARNING, "xxxWinHelpA: xxxLaunchHelp and xxxFindWinow failed." );
#endif
            }
        }
    }

    return hwndHelp;
}


/*
 *  HWND version of Enumeration function to fins controls while
 *  ignoring group boxes but not disabled controls.
 */
BOOL CALLBACK EnumHwndDlgChildProc(HWND hwnd, LPARAM lParam )
{
    PWND pwnd;
    BOOL bResult;

    if (pwnd = ValidateHwnd(hwnd))
        bResult = EnumPwndDlgChildProc( pwnd, lParam );
    else
        bResult = TRUE;

    return bResult;
}


/***************************************************************************\
* WinHelp
*
* Displays help
*
* History:
* 04-15-91 JimA             Ported.
* 01-29-92 GregoryW         Neutral version.
* 05-22-92 SanfordS         Added support for help structures
* 03-24-95 BradG            Moved from Client side WinHelpA to server side
*                           xxxWinHelpA because of changes in Win95.  The
*                           function xxxServerWinHelp was merged.
\***************************************************************************/

BOOL WinHelpA(
    HWND    hwnd,       // hwndMain may be NULL.
    LPCSTR  lpszHelp,
    UINT    uCommand,
    ULONG_PTR dwData)
{
    LPWSTR  lpwstrHelpWindowClass;
    LPHLP   lpHlp = NULL;
    DWORD   dwType;
    PWND    pwnd;
    HWND    hwndHelp = NULL;    /* Handle of help's main window         */
    PWND    pwndTop = NULL;     /* Top level window that WinHelp uses.  */
    PWND    pwndMain;           /* pointer to main help control         */
    LRESULT lResult;
    POINT   ptCur;
    BOOL    bResult = TRUE;

    pwnd = ValidateHwnd(hwnd);

    if (uCommand & HELP_TCARD) {
        /*
         * For Training Cards, the HELP_TCARD bit is set.  We need to
         * set our help window class to szMS_TCARDHELP and then remove
         * the HELP_TCARD bit.
         */
        lpwstrHelpWindowClass = szMS_TCARDHELP;
        uCommand &= ~HELP_TCARD;    // mask out the tcard flag
        dwType = TYPE_TCARD;
    } else {
        if( (uCommand == HELP_CONTEXTMENU) || (uCommand == HELP_CONTEXTPOPUP) ||
            (uCommand == HELP_SETPOPUP_POS) || (uCommand == HELP_WM_HELP)) {
            /*
             *  Popups should be connected to a valid window.  pwndMain has already
             *  been validated as a real window handle or NULL, so we just need to
             *  check the NULL case here.
             */
            if (pwnd == NULL) {
                RIPERR1(ERROR_INVALID_PARAMETER,
                        RIP_WARNING,
                        "WinHelpA: NULL hWnd invalid for this type of help command (0x%X)",
                        uCommand);

                bResult = FALSE;
                goto Exit_WinHelp;
            }
            dwType = TYPE_POPUP;
            lpwstrHelpWindowClass = szMS_POPUPHELP;
        } else {
            dwType = TYPE_NORMAL;
            lpwstrHelpWindowClass = szMS_WINHELP;
        }
    }

    /*
     * Get the cursor's current location  This is where we assume the user
     * clicked.  We will use this position to search for a child window and
     * to set the context sensitive help popup window's location.
     *
     * If the last input was a keyboard one, use the point in the center
     * of the focus window rectangle. MCostea #249270
     */
    if (gpsi->bLastRITWasKeyboard) {
        HWND hWndFocus = GetFocus();
        RECT rcWindow;

        if (GetWindowRect(hWndFocus, &rcWindow)) {
            ptCur.x = (rcWindow.left + rcWindow.right)/2;
            ptCur.y = (rcWindow.top + rcWindow.bottom)/2;
        } else {
            goto getCursorPos;
        }
    } else {
getCursorPos:
        GetCursorPos(&ptCur);
    }

    /*
     * If we are handling the HELP_CONTEXTMENU command, see if we
     * can determine the correct child window.
     */
    if (uCommand == HELP_CONTEXTMENU && FIsParentDude(pwnd)) {
        LONG        lPt;
        int         nHit;
        DLGENUMDATA DlgEnumData;

        /*
         *  If the user really clicked on the caption or the system menu,
         *  then we want the context menu for the window, not help for a
         *  control.  This makes it consistent across all 3.x and 4.0
         *  windows.
         */
        lPt = MAKELONG(ptCur.x,ptCur.y);
        nHit = FindNCHit(pwnd, lPt);
        if ((nHit == HTCAPTION) || (nHit == HTSYSMENU))
            DefWindowProc(hwnd, WM_CONTEXTMENU, (WPARAM)hwnd, lPt);

        /*
         * If this is a dialog class, then one of three things has
         * happened:
         *
         *  o   This is a disabled control
         *  o   This is a static text control
         *  o   This is the background of the dialog box.
         *
         * What we do is enumerate the child windows and see if
         * any of them contain the current cursor point. If they do,
         * change our window handle and continue on. Otherwise,
         * return doing nothing -- we don't want context-sensitive
         * help for a dialog background.
         *
         * If this is a group box, then we might have clicked on a
         * disabled control, so we enumerate child windows to see
         * if we get another control.
         */
        DlgEnumData.pwndDialog = pwnd;
        DlgEnumData.pwndControl = NULL;
        DlgEnumData.ptCurHelp = ptCur;
        EnumChildWindows(hwnd, (WNDENUMPROC)EnumHwndDlgChildProc, (LPARAM)&DlgEnumData);
        if (DlgEnumData.pwndControl == NULL) {
            /*
             * Can't find a control, so nothing to do.
             */
            goto Exit_WinHelp;
        } else {
            /*
             * Remember this control because it will be used as the
             * control for context sensitive help.
             */
            pwndMain = DlgEnumData.pwndControl;
        }
    } else {
        /*
         * We will use pwnd as our main control.  No need to lock it
         * because it is already locked.
         */
        pwndMain = pwnd;
    }

    /*
     * For HELP_CONTEXTPOPUP and HELP_WM_HELP, see if we can derive the
     * context id by looking at the array of double word ID pairs that
     * have been passed in in dwData.
     */
    if (uCommand == HELP_CONTEXTMENU || uCommand == HELP_WM_HELP) {
        int     id;
        int     i;
        LPDWORD pid;

// MapIdToHelp:
        /*
         * Be careful about the cast below.  We need the ID, which is stored
         * in the LOWORD of spmenu to be sign extended to an int.
         * Don't sign extend so IDs like 8008 work
         */
        id = (DWORD)(PTR_TO_ID(pwndMain->spmenu));   // get control id
        pid = (LPDWORD) dwData;

        /*
         * Is the control's ID -1?
         */
        if ((SHORT)id == -1)
        {
            /*
             * This is a static (i.e., ID'less) control
             */
            PWND pwndCtrl;
            int cAttempts = 0;

            /*
             * If the control is a group box, with an ID of -1, bail out
             * as the UI specs decided to have no context help
             * for these cases.  MCostea
             */
            if ((TestWF(pwndMain, BFTYPEMASK) == BS_GROUPBOX) &&
                (GETFNID(pwndMain) == FNID_BUTTON)) {
                goto Exit_WinHelp;
            }

            /*
             * For non-id controls (typically static controls), step
             * through to the next tab item. Keep finding the next tab
             * item until we find a valid id, or we have tried
             * MAX_ATTEMPTS times.
             */
            do {
                pwndCtrl = GetNextDlgHelpItem( REBASEPWND(pwndMain,spwndParent), pwndMain );

                /*
                 * pwndCtrl will be NULL if hwndMain doesn't have a parent,
                 * or if there are no tab stops.
                 */
                if (!pwndCtrl) {
                    /*
                     * Remember to unlock the control
                     */
                    bResult = FALSE;
                    goto Exit_WinHelp;
                }

                /*
                 * Be careful about the cast below.  We need the ID, which is stored
                 * in the LOWORD of spmenu to be sign extended to an int.
                 * Don't sign extend so IDs like 8008 work
                 */
                id = (DWORD)(PTR_TO_ID(pwndCtrl->spmenu));

            } while (((SHORT)id == -1) && (++cAttempts < MAX_ATTEMPTS));
        }

        if ((SHORT)id == -1) {
            id = -1;
        }

        /*
         * Find the id value in array of id/help context values
         */
        for (i = 0; pid[i]; i += 2) {
            if ((int) pid[i] == id)
                break;
        }

        /*
         * Since no help was specified for the found control, see if
         * the control is one of the known ID (i.e., OK, Cancel...)
         */
        if (!pid[i]) {
            /*
             * Help for the standard controls is in the default
             * help file windows.hlp.  Switch to this file.
             */
            lpszHelp = szDefaultHelpFileA;

            switch (id) {
            case IDOK:
                dwData = IDH_OK;
                break;

            case IDCANCEL:
                dwData = IDH_CANCEL;
                break;

            case IDHELP:
                dwData = IDH_HELP;
                break;

            default:
                /*
                 * Unknown control, give a generic missing context info
                 * popup message in windows.hlp.
                 */
                dwData = IDH_MISSING_CONTEXT;
            }
        } else {
            dwData = pid[i + 1];
            if (dwData == (DWORD)-1) {
                /*
                 * Remember, to unlock the control
                 */
                goto Exit_WinHelp;     // caller doesn't want help after all
            }
        }

        /*
         *  Now that we know the caller wants help for this control, display the
         *  help menu.
         */
        if (uCommand == HELP_CONTEXTMENU)
        {
            int cmd;

            /*
             * Must lock pwndMain because it may have been reassigned above.
             */
            cmd = HelpMenu(HW(pwndMain), &ptCur);
            if (cmd <= 0)   // probably means user cancelled the menu
                goto Exit_WinHelp;
        }

        /*
         * Create WM_WINHELP's HLP data structure for HELP_SETPOPUP_POS
         */
        if (!(lpHlp = HFill(lpszHelp, HELP_SETPOPUP_POS,
                MAKELONG(pwndMain->rcWindow.left, pwndMain->rcWindow.top)))) {
            /*
             * Remember to unlock pwndMain if needed
             */
            bResult = FALSE;
            goto Exit_WinHelp;
        }

        /*
         * Tell WinHelp where to put the popup.  This is different than Win95
         * because we try and avoid a recursive call here.  So, we find the
         * WinHlp32 window and send the HELP_SETPOPUP_POS.  No recursion.
         */
        hwndHelp = FindWinHelpWindow( lpwstrHelpWindowClass, dwType, TRUE);
        if (hwndHelp == NULL ) {
            /*
             * Uable to communicate with WinHlp32.exe.
             * Remember to unlock the control
             */
            bResult = FALSE;
            goto Exit_WinHelp;
        }

        /*
         * Send the WM_WINHELP message to WinHlp32's window.
         */
        lResult = SendWinHelpMessage(hwndHelp, (WPARAM)HW(pwndMain), (LPARAM)lpHlp);
        LocalFree(lpHlp);
        lpHlp = NULL;

/* BradG - revalidate  pwndMain? */

        if (!lResult)
        {
            /*
             * WinHlp32 couldn't process the command.  Bail out!
             */
            bResult = FALSE;
            goto Exit_WinHelp;
        }

        /*
         * Make HELP_WM_HELP and HELP_CONTEXTMENU act like HELP_CONTEXTPOPUP
         */
        uCommand = HELP_CONTEXTPOPUP;
    }


    if (uCommand == HELP_CONTEXTPOPUP ) {
// MapNullHlpToWindowsHlp:
        /*
         * If no help file was specified, use windows.hlp
         */
        if (lpszHelp == NULL || *lpszHelp == '\0')
            lpszHelp = szDefaultHelpFileA;  // default: use windows.hlp

        /*
         *  WINHELP.EXE will call SetForegroundWindow on the hwnd that we pass
         *  to it below.  We really want to pass the parent dialog hwnd of the
         *  control so that focus will properly be restored to the dialog and
         *  not the control that wants help.
         */
        pwndTop = GetTopLevelWindow(pwndMain);
    } else {
        pwndTop = pwndMain;
    }


    /*
     * Move Help file name to a handle
     */
    if (!(lpHlp = HFill(lpszHelp, uCommand, dwData))) {
        /*
         * Can't allocate memory
         */
        bResult = FALSE;
        goto Exit_WinHelp;
    }

    /*
     *  Get a pointer to the help window.
     */
    hwndHelp = FindWinHelpWindow( lpwstrHelpWindowClass, dwType, (uCommand != HELP_QUIT));
    if (hwndHelp == NULL) {
        if (uCommand != HELP_QUIT)
            /*
             * Can't find Winhlp
             */
            bResult = FALSE;
        goto Exit_WinHelp;
    }

    /*
     * Send the WM_WINHELP message to WinHlp32's window
     * Must ThreadLock pwndHelp AND pwndMain (because pwndMain may have been
     * reassigned above).
     */
    SendWinHelpMessage(hwndHelp, (WPARAM)HW(pwndTop), (LPARAM)lpHlp);

    /*
     * Free the help info data structure (if not already free).
     */
Exit_WinHelp:
    if (lpHlp != NULL) {
        LocalFree(lpHlp);
    }

    return bResult;
}












#if 0
/*
 *
 * Communicating with WinHelp involves using Windows SendMessage function
 * to pass blocks of information to WinHelp. The call looks like.
 *
 * SendMessage(hwndHelp, WM_WINHELP, pidSource, pwinhlp);
 *
 * Where:
 *
 * hwndHelp - the window handle of the help application. This
 * is obtained by enumerating all the windows in the
 * system and sending them HELP_FIND commands. The
 * application may have to load WinHelp.
 * pidSource - the process id of the sending process
 * pwinhlp - a pointer to a WINHLP structure
 *
 * The data in the handle will look like:
 *
 * +-------------------+
 * |     cbData        |
 * |    ulCommand      |
 * |    hwndHost       |
 * |     ulTopic       |
 * |    ulReserved     |
 * |   offszHelpFile   |\ - offsets measured from beginning
 * | offaData          | \ of header.
 * +-------------------+ /
 * | Help file name    |/
 * | and path          |
 * +-------------------+
 * | Other data        |
 * |    (keyword)      |
 * +-------------------+
 *
 * hwndMain - the handle to the main window of the application
 * calling help
 *
 * The defined commands are:
 *
 * HELP_CONTEXT 0x0001 Display topic in ulTopic
 * HELP_KEY 0x0101 Display topic for keyword in offabData
 * HELP_QUIT 0x0002 Terminate help
 *
 */

BOOL WinHelpA(
    HWND hwndMain,
    LPCSTR lpszHelp,
    UINT uCommand,
    ULONG_PTR dwData)
{
    LPHLP lpHlp;
    BOOL fSuccess;
    DWORD dwType;
    HWND hwndHelp; /* Handle of help's main window */
    LPWSTR lpwstr;
    PWND pwndMain;

    pwndMain = ValidateHwnd(hwndMain);

    if ((uCommand == HELP_CONTEXTMENU) || (uCommand == HELP_CONTEXTPOPUP) ||
            (uCommand == HELP_SETPOPUP_POS) || (uCommand == HELP_WM_HELP)) {
        dwType = TYPE_POPUP;
    } else {
        dwType = TYPE_NORMAL;
    }

    switch (uCommand) {

    case HELP_WM_HELP:
        {
#if 0
        // Tell WinHelp where to put the popup

        if (!WinHelpA(hwndMain, lpszHelp, HELP_SETPOPUP_POS,
                MAKELONG(pwndMain->rcWindow.left, pwndMain->rcWindow.top)))
            return FALSE;

        /*
         * Unlike HELP_CONTEXTMENU, with this command we pop up the help
         * topic immediately instead of making the user go through a menu.
         */
#endif

        goto MapIdToHelp;

        }
        break;

    case HELP_CONTEXTMENU:
        {
        int cmd;
        int id;
        int i;
        POINT ptCur;
        LPDWORD pid;

        GetCursorPos(&ptCur);

        if (pwndMain != NULL && pwndMain->fnid == FNID_DIALOG) {
            /*
             * If this is a dialog class, then one of three things has
             * happened:
             *
             *  o   This is a disabled control
             *  o   This is a static text control
             *  o   This is the background of the dialog box.
             *
             * What we do is enumerate the child windows and see if
             * any of them contain the current cursor point. If they do,
             * change our window handle and continue on. Otherwise,
             * return doing nothing -- we don't want context-sensitive
             * help for a dialog background.
             */

            pwndMain = REBASEPWND(pwndMain, spwndChild);

            while (pwndMain != NULL
                   && PtInRect(&(pwndMain->rcWindow), ptCur)) {
                pwndMain = REBASEPWND(pwndMain, spwndNext);
            }

            if (pwndMain == NULL)
                return FALSE;
        }

        cmd = HelpMenu(hwndMain, &ptCur);

        if (cmd <= 0) { // probably means user cancelled the menu
            return(FALSE);      // !!! mem leak?

        } else if (cmd == HELP_INDEX) {
            // Search
            uCommand = HELP_FINDER;
        } else {
MapIdToHelp:
            UserAssert(pwndMain);

            // Tell WinHelp where to put the popup

            if (!WinHelpA(hwndMain, lpszHelp, HELP_SETPOPUP_POS,
                    MAKELONG(pwndMain->rcWindow.left, pwndMain->rcWindow.top)))
                return FALSE;
            id = LOWORD(pwndMain->spmenu);   // get control id

            pid = (LPDWORD) dwData;

            if (id == -1) {
                // static control?
                PWND pwndCtrl;
                int cAttempts = 0;

                // For non-id controls (typically static controls), step
                // through to the next tab item. Keep finding the next tab
                // item until we find a valid id, or we have tried
                // MAX_ATTEMPTS times.
                do {
                    pwndCtrl = _GetNextDlgTabItem(REBASEPWND(pwndMain, spwndParent),
                                    pwndMain, FALSE);

                    // hwndCtrl will be NULL if hwndMain doesn't have a parent,
                    // or if there are no tab stops.

                    if (!pwndCtrl) {
                        return(FALSE);
                    }



                    id = LOWORD(pwndCtrl->spmenu);
                }
                while ((id == -1) && (++cAttempts < MAX_ATTEMPTS));
            }

            // Find the id value in array of id/help context values

            for (i = 0; pid[i]; i += 2) {
                if ((int) pid[i] == id)
                    break;
            }

            if (!pid[i]) {


                lpszHelp = szDefaultHelpFileA;  // switch to windows.hlp

                switch (id) {
                    case IDOK:
                        dwData = IDH_OK;
                        break;

                    case IDCANCEL:
                        dwData = IDH_CANCEL;
                        break;

                    case IDHELP:
                        dwData = IDH_HELP;
                        break;

                    default:
                        dwData = IDH_MISSING_CONTEXT;
                }
            } else {
                dwData = pid[i + 1];
                if (dwData == (DWORD) -1) {
                    return TRUE;   // caller doesn't want help after all
                }
            }

            uCommand = HELP_CONTEXTPOPUP;
        }

        // If no help file was specified, use windows.hlp

        if (lpszHelp == NULL || *lpszHelp == '\0')
            lpszHelp = szDefaultHelpFileA;  // default: use windows.hlp
        }
    }

    /*
     * Move Help file name to a handle
     */
    if (!(lpHlp = HFill(lpszHelp, uCommand, dwData)))
        return FALSE;

    /*
     * Pass it on to WinHelp32
     */
    fSuccess = TRUE;
    if (dwType == TYPE_POPUP)
        lpwstr = szMS_POPUPHELP;
    else
        lpwstr = szMS_WINHELP;

    if ((hwndHelp = FindWindowEx(NULL, NULL, lpwstr, NULL)) == NULL) {
        if (uCommand == HELP_QUIT)
            fSuccess = TRUE;

        /*
         * Can't find it --> launch it
         */
        if (!LaunchHelp(dwType) ||
                (hwndHelp = FindWindowEx(NULL, NULL, lpwstr, NULL)) == NULL)
            fSuccess = FALSE;
    }

    if (hwndHelp != NULL)
        SendMessage(hwndHelp, WM_WINHELP, (DWORD)hwndMain, (LONG)lpHlp);

    LocalFree(lpHlp);

    return fSuccess;
}
#endif



/***************************************************************************\
* WinHelpW
*
* Calls WinHelpA after doing any necessary translation.
* Our help engine is ASCII only.
*
\***************************************************************************/

BOOL WinHelpW(
    HWND hwndMain,
    LPCWSTR lpwszHelp,
    UINT uCommand,
    ULONG_PTR dwData)
{
    BOOL fSuccess = FALSE;
    LPSTR lpAnsiHelp = NULL;
    LPSTR lpAnsiKey = NULL;
    PMULTIKEYHELPA pmkh = NULL;
    PHELPWININFOA phwi = NULL;
    NTSTATUS Status;


    /*
     * First convert the string.
     */
    if (lpwszHelp != NULL &&
            !WCSToMB(lpwszHelp, -1, &lpAnsiHelp, -1, TRUE)) {
        return FALSE;
    }

    /*
     * Then convert dwData if needed
     */
    switch (uCommand) {
    case HELP_MULTIKEY:
        if (!WCSToMB(((PMULTIKEYHELPW)dwData)->szKeyphrase, -1, &lpAnsiKey,
                -1, TRUE)) {
            goto FreeAnsiHelp;
        }

        pmkh = (PMULTIKEYHELPA)LocalAlloc(LPTR,
                sizeof(MULTIKEYHELPA) + strlen(lpAnsiKey));
        if (pmkh == NULL) {
            goto FreeAnsiKeyAndHelp;
        }

        pmkh->mkSize = sizeof(MULTIKEYHELPA) + strlen(lpAnsiKey);
        Status = RtlUnicodeToMultiByteN((LPSTR)&pmkh->mkKeylist, sizeof(CHAR),
                NULL, (LPWSTR)&((PMULTIKEYHELPW)dwData)->mkKeylist,
                sizeof(WCHAR));
        strcpy(pmkh->szKeyphrase, lpAnsiKey);
        if (!NT_SUCCESS(Status)) {
            goto FreeAnsiKeyAndHelp;
        }

        dwData = (ULONG_PTR)pmkh;
        break;

    case HELP_SETWINPOS:
        if (!WCSToMB(((PHELPWININFOW)dwData)->rgchMember, -1, &lpAnsiKey,
                -1, TRUE)) {
            goto FreeAnsiKeyAndHelp;
        }

        phwi = (PHELPWININFOA)LocalAlloc(LPTR, ((PHELPWININFOW)dwData)->wStructSize);
        if (phwi == NULL) {
            goto FreeAnsiKeyAndHelp;
        }

        *phwi = *((PHELPWININFOA)dwData);   // copies identical parts
        strcpy(phwi->rgchMember, lpAnsiKey);
        dwData = (ULONG_PTR)phwi;
        break;

    case HELP_KEY:
    case HELP_PARTIALKEY:
    case HELP_COMMAND:
        if (!WCSToMB((LPCTSTR)dwData, -1, &lpAnsiKey, -1, TRUE)) {
            goto FreeAnsiKeyAndHelp;
        }

        dwData = (ULONG_PTR)lpAnsiKey;
        break;
    }

    /*
     * Call the Ansi version
     */
    fSuccess = WinHelpA(hwndMain, lpAnsiHelp, uCommand, dwData);

    if (pmkh) {
        LocalFree(pmkh);
    }

    if (phwi) {
        LocalFree(phwi);
    }

FreeAnsiKeyAndHelp:
    if (lpAnsiKey) {
        LocalFree(lpAnsiKey);
    }


FreeAnsiHelp:
    if (lpAnsiHelp)
        LocalFree(lpAnsiHelp);

    return fSuccess;
}
