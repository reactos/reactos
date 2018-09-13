/*
 * pmwprocs.c - window procs for program manager
 *
 *  Copyright (c) 1991,  Microsoft Corporation
 *
 *  DESCRIPTION
 *
 *        This file is for support of program manager under NT Windows.
 *        This file is/was ported from pmwprocs.c (program manager).
 *
 *  MODIFICATION HISTORY
 *      Initial Version: x/x/90    Author Unknown, since he didn't feel
 *                                like commenting the code...
 *
 *      NT 32b Version:     1/18/91    Jeff Pack
 *                                Intitial port to begin.
 *
 *
 */

#include "progman.h"
#include "dde.h"

extern BOOL bInNtSetup;

/****************************************************************************
 *
 * SetProgmanProperties(DWORD dwDdeId, WORD wHotKey)
 *
 * Called when a new instance of progman was started from a Progman group
 * item. This will set the properties of the first instance of Progman,
 * setting the hotkey, the window title, the icon, and minimize Progman
 * if the item has Run Mimimized set.
 *
 * Called when Progman receives WM_EXECINSTANCE message send from 2nd progman
 * instance.
 *
 * 08-28-92 JohanneC   Created.
 *
 ****************************************************************************/


BOOL SetProgmanProperties(DWORD dwDdeId, WORD wHotKey)
{
    LPGROUPDEF lpgd;
    LPITEMDEF lpid;
    PGROUP pGroup = NULL;
    PITEM pItem = NULL;
    HWND hwndT;
    BOOL Found = FALSE;
    HICON hIcon;
    BOOL bWasIconic = FALSE;

    //
    // Set Progman's hotkey.
    //
    SendMessage(hwndProgman, WM_SETHOTKEY, wHotKey, 0L);

    //
    // Find the group and the item corresponding to this information.
    //
    for (hwndT = GetWindow(hwndMDIClient, GW_CHILD);
         hwndT;
         hwndT = GetWindow(hwndT, GW_HWNDNEXT)) {

        if (GetWindow(hwndT, GW_OWNER))
            continue;

        pGroup = (PGROUP)GetWindowLongPtr(hwndT, GWLP_PGROUP);
        for (pItem = pGroup->pItems; pItem; pItem = pItem->pNext) {
            if (pItem->dwDDEId == dwDdeId) {

                //
                // Found a match.
                //
                Found = TRUE;
                break;
            }
        }
        if (Found)
            break;
    }

    if (!Found)
        return(FALSE);

    //
    // Set Progman Properties using the properties of its item.
    //

    //
    // Set the new icon.
    //
    hIcon = DuplicateIcon(hAppInstance, GetItemIcon(pGroup->hwnd, pItem));
    if (hIcon) {
        DestroyIcon((HICON)GetClassLongPtr(hwndProgman, GCLP_HICON));
        SetClassLongPtr(hwndProgman, GCLP_HICON, (LONG_PTR)hIcon);
    }

    if (IsIconic(hwndProgman))
        bWasIconic = TRUE;

    //
    // Check the minimize flag.
    //
    lpgd = GlobalLock(pGroup->hGroup);
    if (lpgd) {
        lpid = ITEM(lpgd, pItem->iItem);
        SetWindowText(hwndProgman, (LPTSTR) PTR(lpgd, lpid->pName));
        if (GroupFlag(pGroup, pItem, (WORD)ID_MINIMIZE))
            ShowWindow(hwndProgman, SW_SHOWMINNOACTIVE);
        GlobalUnlock(pGroup->hGroup);
    }

    if (bWasIconic) {
        //
        // to update the icon and text.
        //
        ShowWindow(hwndProgman, SW_HIDE);
        ShowWindow(hwndProgman, SW_SHOW);
    }

    return(TRUE);
}

void NEAR PASCAL RedoAllIconTitles()
    // Stomps on all the title rects.
    {
    HWND hwndGroup;
    PGROUP pGroup;
    PITEM  pItem;
    HDC hdc;
    int cch;
    HFONT hFontT;
    LPRECT lprcTitle;
    LPTSTR lpText;
    LPGROUPDEF lpgd;
    LPITEMDEF lpid;
    POINT pt;

    for (hwndGroup=GetWindow(hwndMDIClient, GW_CHILD); hwndGroup; hwndGroup=GetWindow(hwndGroup, GW_HWNDNEXT))
        {
        if (GetWindow(hwndGroup, GW_OWNER))
    	    continue;

        pGroup = (PGROUP)GetWindowLongPtr(hwndGroup, GWLP_PGROUP);

        lpgd = LockGroup(hwndGroup);
        if (!lpgd)
            {
            continue;
            }

        for (pItem = pGroup->pItems; pItem; pItem = pItem->pNext)
            {
    	    lprcTitle = &(pItem->rcTitle);
            lpid = ITEM(lpgd, pItem->iItem);
            lpText = (LPTSTR) PTR(lpgd, lpid->pName);
            pt.x = pItem->rcIcon.left;
            pt.y = pItem->rcIcon.top;

            cch = lstrlen(lpText);

    	    hdc = GetDC(pGroup->hwnd);
	        hFontT = SelectObject(hdc,hFontTitle);

	        // compute the icon rect using DrawText
            SetRectEmpty(lprcTitle);
    	    lprcTitle->right = cxArrange - (2 * cxOffset);
            DrawText(hdc, lpText, -1,
                lprcTitle, bIconTitleWrap ?
                DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX :
                DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX | DT_SINGLELINE);

            if (hFontT)
                SelectObject(hdc,hFontT);
    	    ReleaseDC(pGroup->hwnd,hdc);
            lprcTitle->right += cxOffset*2;
            lprcTitle->bottom+= dyBorder*2;
            OffsetRect
                (
                lprcTitle,
                (pt.x+(cxIconSpace/2)-((lprcTitle->right
                    -lprcTitle->left)/2)),
                (pt.y+cyIconSpace-dyBorder)
                );
            }
            UnlockGroup(hwndGroup);

        }
    }

/*** GetRealParent --
 *
 *
 * HWND APIENTRY GetRealParent(HWND hWnd)
 *
 * ENTRY -     HWND    hWnd
 *
 * EXIT  -    HWND
 *
 * SYNOPSIS -  ???
 *
 * WARNINGS -
 * EFFECTS  -
 *
 */

HWND APIENTRY GetRealParent(HWND hwnd)
{
    /* run up the parent chain until you find a hwnd */
    /* that doesn't have WS_CHILD set*/

    /* BUG BUG, these should work as is????*/
    while (GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD){
        hwnd = (HANDLE) GetWindowLongPtr(hwnd, GWLP_HWNDPARENT);
    }
    return hwnd;
}

/*** AnyWriteable --
 *
 *
 * BOOL APIENTRY AnyWriteable()
 *
 * ENTRY -     none
 *
 * EXIT  -    BOOL    xxx -  TRUE if read only, FALSE if not
 *
 * SYNOPSIS -  ???
 *
 * WARNINGS -
 * EFFECTS  -
 *
 */

BOOL APIENTRY AnyWriteable()
{
    PGROUP pGroup;

    for (pGroup=pFirstGroup; pGroup; pGroup = pGroup->pNext)
    if (!pGroup->fRO){
        return TRUE;
    }

    return FALSE;
}

/*** ProgmanProc -- window procedure for program manager
 *
 *
 * LONG APIENTRY ProgmanWndProc(register HWND hwnd, UINT uiMsg,
 *                                register WPARAM wParam, LONG lParam)
 *
 * ENTRY -    HWND    hWnd
 *            WORD    uiMsg
 *            WPARAM  wParam
 *            LONG    lParam
 * EXIT  -    LONG    xxx - returns info, or zero, for nothing to return
 *
 * SYNOPSIS -  ???
 *
 * WARNINGS -
 * EFFECTS  -
 *
 */

LRESULT APIENTRY ProgmanWndProc(
    HWND hWnd,
    UINT uiMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uiMsg) {

    case WM_CREATE:
    {
        RECT rc;
        CLIENTCREATESTRUCT ccs;

        hwndProgman = hWnd;

        ccs.hWindowMenu = GetSubMenu(GetMenu(hWnd), IDM_WINDOW);
        ccs.idFirstChild = IDM_CHILDSTART;

        GetClientRect(hwndProgman, &rc);

	    /*
         * Don't show the MDI client until all groups have
	     * been created to avoid ungly painting.
         */

        hwndMDIClient = CreateWindow(TEXT("MDIClient"),
                                NULL,
                                WS_CLIPCHILDREN | WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_BORDER,
                                rc.left - 1, rc.top - 1,
                                rc.right + 2, rc.bottom + 2,
                                hWnd, (HMENU)1, hAppInstance,
                                (LPTSTR)&ccs);
        if (!hwndMDIClient) {
            return -1;
        }
        break;
    }

    case WM_ENDSESSION:
        if (wParam != 0) {  // nonzero means the session is being ended.
EndSession:
            /* Don't close if restricted. */
            if (fNoClose)
                break;

            if (bSaveSettings)
                WriteINIFile();
            else
                SaveGroupsContent(FALSE);

            //
            // should Flush and close all registry keys here.
            //
            RegFlushKey(HKEY_CURRENT_USER);
            if (hkeyProgramManager) {
                if (hkeyPMSettings) {
                    RegCloseKey(hkeyPMSettings);
                }
                if (hkeyPMGroups) {
                    RegCloseKey(hkeyPMGroups);
                }
                RegCloseKey(hkeyProgramManager);
            }

            if (hkeyProgramGroups) {
                RegCloseKey(hkeyProgramGroups);
            }

            if (hkeyCommonGroups) {
                RegFlushKey(hkeyCommonGroups);
                RegCloseKey(hkeyCommonGroups);
            }

            /* Free the commdlg */
            if (hCommdlg)
                FreeLibrary(hCommdlg);

	        ExitProcess(0);
            }

        break;

    case WM_CLOSE:
        /*
	 * if wParam != 0, then this is shutdown. We don't want to logoff
	 * again in this case, because we're already in the middle of
	 * logoff. We just want to exit now.
         */

        if (wParam != 0)
            goto EndSession;

        /* Don't close if restricted. */
        if (fNoClose)
            return FALSE;

	    if (GetKeyState(VK_SHIFT) < 0) {
            WriteINIFile();
    	    return TRUE;
        }

        /* Check if we've already tried to exit once... */
        if (fExiting) {
            SetCurrentDirectory(szOriginalDirectory);
            ExitWindows(0, 0);
        }

        fExiting = TRUE;      // Stop LockGroup from trying to do a RELOAD
                              // if the a lock fails due to group being
                              // out of date.
        SetWindowLong (hwndProgman, GWL_EXITING, 1);

    	if (bExitWindows) {

            if (lParam != (LPARAM)-1) {
                //
                // The user double-clicked on the system menu, use the new
                // logoff dialog.
                //

                if (MyDialogBox(NEWLOGOFFDLG,
                             hwndProgman,
                             NewLogoffDlgProc)) {
                }
            }
            else {
                if (MyDialogBox(IDD_END_WINDOWS_SESSION,
                             hwndProgman,
                             ExitDlgProc)) {
                    SetCurrentDirectory(szOriginalDirectory);
                    ExitWindows(0, 0);
                }
            }

            /* User clicked cancel or some app refused the ExitWindows... */
            fExiting = FALSE;
            SetWindowLong (hwndProgman, GWL_EXITING, 0);
            break;
        }
        else {

            if (bSaveSettings)
                WriteINIFile();
            else {
                //
                // If we are in setup, the groups and settings
                // will have already been saved in the
                // ExitProgman(1) dde handler.
                //

                if (!bInNtSetup)
                    SaveGroupsContent(FALSE);
            }

            goto CallDFP;
        }

    case WM_LOGOFF:
        DestroyWindow(hwndProgman);
        return 0;

    case WM_DESTROY:
        if (!WinHelp(hwndProgman, szProgmanHelp, HELP_QUIT, 0L)) {
            MyMessageBox(hwndProgman, IDS_APPTITLE, IDS_WINHELPERR, NULL, MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
        }

        /* Free font. */
        DeleteObject(hFontTitle);

        /* Free the commdlg junk. */
        if (hCommdlg)
            FreeLibrary(hCommdlg);

        /*
         * If user hit CTRL-ALT-DEL to logoff, restart or shutdown,
         * we still need to save the settings.
         */
        if (!fExiting && bSaveSettings) {
            WriteINIFile();
        }
        else
            SaveGroupsContent(FALSE);

	    if (hbrWorkspace) {
	        DeleteObject(hbrWorkspace);
	        hbrWorkspace = NULL;
	    }

        /*
         * Stop all translations
         */
        hwndMDIClient = NULL;
        hwndProgman = NULL;
        PostQuitMessage(0);
        break;

    case WM_DDE_TERMINATE:
    case WM_DDE_EXECUTE:
    case WM_DDE_ACK:
    case WM_DDE_REQUEST:
    case WM_DDE_DATA:
    case WM_DDE_ADVISE:
    case WM_DDE_UNADVISE:
    case WM_DDE_POKE:
    case WM_DDE_INITIATE:
#ifdef DEBUG_PROGMAN_DDE
        {
        TCHAR szDebug[300];

        wsprintf (szDebug, TEXT("%d   PROGMAN:   Received DDE msg 0x%x\r\n"),
                  GetTickCount(), uiMsg);
        OutputDebugString(szDebug);
        }
#endif
        if (!bDisableDDE)
        {
            return (DDEMsgProc(hWnd, uiMsg, wParam, lParam));
        }
        goto CallDFP;

    case WM_INITMENU:
    {
        BOOL bGroup;
        WORD wEnable;
        INT i;
        PGROUP pGroup;

        bGroup = (SelectionType() != TYPE_ITEM);

        /*
         * Disable Delete/Properties if there aren't any groups.
         */
        if (!pCurrentGroup) {
            wEnable = MF_BYCOMMAND | MF_DISABLED | MF_GRAYED;
        } else {
            wEnable = MF_BYCOMMAND | MF_ENABLED;
        }

        EnableMenuItem((HMENU)wParam, IDM_PROPS,  wEnable);

        if ((pCurrentGroup && pCurrentGroup->fRO) || dwEditLevel >= 2
               || (dwEditLevel == 1 && bGroup)) {
            wEnable = MF_BYCOMMAND | MF_GRAYED | MF_DISABLED;
        }

        EnableMenuItem((HMENU)wParam, IDM_DELETE, wEnable);

        /* Handle ArrangeItems menu... */
	    if (pCurrentGroup && pCurrentGroup->fRO && !bGroup)
	        wEnable = MF_BYCOMMAND | MF_GRAYED | MF_DISABLED;
        else
	        wEnable = MF_BYCOMMAND | MF_ENABLED;
	    EnableMenuItem((HMENU)wParam, IDM_ARRANGEICONS, wEnable);

        /*
         * Disable Move/Copy if 1. There aren't any groups,
         *                      2. There aren't any items in the group,
         *                      3. A group is selected.
         *                      4. The group is read only.
         *                      5. Restrictions do not permit it.
         *                      6.There is only one group - just move
         *                        disabled.
         */
        if (!pCurrentGroup || !pCurrentGroup->pItems ||
                    bGroup || !AnyWriteable() || dwEditLevel >= 2) {
            wEnable = MF_BYCOMMAND | MF_DISABLED | MF_GRAYED;
        } else {
            wEnable = MF_BYCOMMAND | MF_ENABLED;
        }

        EnableMenuItem((HMENU)wParam, IDM_COPY, wEnable);

        if (pCurrentGroup && pCurrentGroup->fRO) {
            wEnable = MF_BYCOMMAND | MF_DISABLED | MF_GRAYED;
        }
        EnableMenuItem((HMENU)wParam, IDM_MOVE, wEnable);

        i = 0;
        for (pGroup=pFirstGroup; pGroup; pGroup = pGroup->pNext) {
            if (!pGroup->fRO)
                i++;
        }
        if (i<2) {
	        wEnable = MF_BYCOMMAND | MF_DISABLED | MF_GRAYED;
	        EnableMenuItem((HMENU)wParam, IDM_MOVE, wEnable);
        }

        /*
         * Disable Open if  1. There aren't any groups,
         *                  2. An empty, non-minimized group is selected.
         */
        if ((!pCurrentGroup) || (!bGroup && (!pCurrentGroup->pItems)
                && (!IsIconic(pCurrentGroup->hwnd)))) {
            wEnable = MF_BYCOMMAND | MF_DISABLED | MF_GRAYED;
        } else {
            wEnable = MF_BYCOMMAND | MF_ENABLED;
        }
        EnableMenuItem((HMENU)wParam, IDM_OPEN, wEnable);

        /*
         * Grey new if
         *   can't create items, or
         *   can't create groups and either the group is read only (can't
         *     create an item in it) or a group is selected
         */
        if (dwEditLevel >= 2 || (dwEditLevel >= 1 &&
                    (bGroup || pCurrentGroup->fRO))) {
            wEnable = MF_BYCOMMAND | MF_DISABLED | MF_GRAYED;
        } else {
            wEnable = MF_BYCOMMAND | MF_ENABLED;
        }
        EnableMenuItem((HMENU)wParam, IDM_NEW, wEnable);

        if (fNoRun) {
            EnableMenuItem((HMENU)wParam, IDM_RUN,
                        MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        }

        if (fNoClose) {
            EnableMenuItem((HMENU)wParam, IDM_EXIT,
                        MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

            wParam = (WPARAM)GetSystemMenu(hWnd, FALSE);
            EnableMenuItem((HMENU)wParam, (WORD)SC_CLOSE,
                        MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        }
        break;
    }

    case WM_SIZE:
        if (wParam != SIZEICONIC) {
            MoveWindow(hwndMDIClient, -1, -1, LOWORD(lParam) + 2,
                    HIWORD(lParam) + 2, TRUE);
        }
        break;

    case WM_SYSCOLORCHANGE:
	    if (hbrWorkspace) {
    	    DeleteObject(hbrWorkspace);
	    }
	    hbrWorkspace = CreateSolidBrush(GetSysColor(COLOR_APPWORKSPACE));

	    //
	    // Fall thru
	    //

    case WM_WININICHANGE:
    {
	    PGROUP pGroup;
        BOOL bOldIconTitleWrap;
        PVOID pEnv ;

        if (lstrcmpi((LPTSTR)lParam,TEXT("Environment")) ==0) {
	    //
            // Check if the user's environment variables have changed, if so
            // regenerate the environment.
            //
            RegenerateUserEnvironment(&pEnv, TRUE);
            break;
        }

        bOldIconTitleWrap = bIconTitleWrap;

        SystemParametersInfo(SPI_ICONHORIZONTALSPACING, 0, (PVOID)&cxArrange, FALSE);
        SystemParametersInfo(SPI_ICONVERTICALSPACING, 0, (PVOID)&cyArrange, FALSE);
        SystemParametersInfo(SPI_GETICONTITLEWRAP, 0, (PVOID)&bIconTitleWrap, FALSE);

        // Handle title wrapping.
        if (bOldIconTitleWrap != bIconTitleWrap)
            RedoAllIconTitles();
	    for (pGroup = pFirstGroup; pGroup; pGroup = pGroup->pNext) {
	        NukeIconBitmap(pGroup);
            if (bAutoArrange) {
                ArrangeItems(pGroup->hwnd);
            }
	    }

        InvalidateRect(hWnd, NULL, TRUE);
        break;
    }

    case WM_MENUSELECT:

        if (lParam) {        /*make sure menu handle isn't null*/
            wMenuID =  GET_WM_COMMAND_ID(wParam, lParam);    /*get cmd from loword of wParam*/
            hSaveMenuHandle = (HANDLE)lParam;    /*Save hMenu into one variable*/
            wSaveFlags = HIWORD(wParam);/*Save flags into another*/
            if (wMenuID >= IDM_CHILDSTART && wMenuID < IDM_HELPINDEX)  {
                wMenuID = IDM_CHILDSTART;
            }
            bFrameSysMenu = (hSaveMenuHandle == GetSystemMenu(hwndProgman, FALSE));
            if (!bFrameSysMenu && wMenuID >= 0xf000) {
                //
                // According to winhelp: GetSystemMenu, uMenuID >= 0xf000
                // means system menu items!
                //
                // The group window is maximized, and group system menu
                // was selected
                //
                wSaveFlags |= MF_SYSMENU;
            }
        }
        break;

    case WM_EXECINSTANCE:
    {
        /*
         * Another instance of program manager has been started.
         * This can not be checked using the Windows 3.1 way because
         * starting apps is done asynchronously in NT, and the values
         * of pExecingGroup, pExecingItem and fInExec will always be FALSE.
         *
         * So we use the lpReserved field in the startupInfo structure
         * of the other instance to make sure is was called from progman.
         * This string is of the format "dde.%d,hotkey.%d" and it is
         * parsed in the other instance to extract the dde id and the hotkey.
         * These are passed as wParam and lParam respectively in the
         * WM_EXECINSTANCE message.
         *
         * - johannec 8/28/92
         */
        if (wParam) {
            SetProgmanProperties((DWORD)wParam, (WORD)lParam);
        }
        else {
            /*
             * The user isn't trying to run progman from within progman
             * so just show them that there's a progman already running...
             */
	        if (IsIconic(hWnd))
	            ShowWindow(hWnd,SW_SHOWNORMAL);
           SetForegroundWindow(hWnd);
	        BringWindowToTop(hWnd);
	        BringWindowToTop(GetLastActivePopup(hWnd));
        }
        break;
    }

    case WM_UNLOADGROUP:
        UnloadGroupWindow((HWND)wParam);
        break;

    case WM_RELOADGROUP:
    {
        TCHAR szGroupKey[MAXKEYLEN+1];
        WORD idGroup;
        BOOL bCommonGroup;

        lstrcpy(szGroupKey,((PGROUP)wParam)->lpKey);
        idGroup = ((PGROUP)wParam)->wIndex;
        bCommonGroup = ((PGROUP)wParam)->fCommon;
        UnloadGroupWindow(((PGROUP)wParam)->hwnd);
        fLowMemErrYet = FALSE;
        LoadGroupWindow(szGroupKey,idGroup, bCommonGroup);
        MyMessageBox(hwndProgman, IDS_GROUPFILEERR, IDS_GRPHASCHANGED,
                    /* szGroupKey */ NULL, MB_OK | MB_ICONEXCLAMATION);
        break;
    }

    case WM_SYSCOMMAND:
        if (fNoClose && wParam == SC_CLOSE ||
            fNoClose && wParam == IDM_EXIT ||
            fNoClose && wParam == IDM_SHUTDOWN ) {
            break;
        }
        if (wParam == IDM_EXIT) {
            if (fNoFileMenu)
                break;

            PostMessage(hwndProgman, WM_CLOSE, 0, (LPARAM)-1);
	        break;
        }
        if (wParam == IDM_SHUTDOWN) {
            if (fNoFileMenu)
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
        }
        goto CallDFP;

    case WM_COMMAND:
        if (ProgmanCommandProc(hWnd, wParam, lParam)) {
            break;
        }
        goto CallDFP;

    default:

        if (uiMsg == uiActivateShellWindowMessage) {
	        if (IsIconic(hwndProgman))
	            ShowWindow(hwndProgman, SW_RESTORE);
	        else
                BringWindowToTop(hwndProgman);

        } else if (uiMsg == uiConsoleWindowMessage) {
            PostMessage((HWND)wParam, uiConsoleWindowMessage, (WPARAM)hWnd, 0);

        } else if (uiMsg == uiSaveSettingsMessage) {
            WriteINIFile();

        } else if (uiMsg == uiHelpMessage) {
               if (wParam == MSGF_MENU) {
                /*
                 * Get outta menu mode if help for a menu item.
                 */

                if (wMenuID && hSaveMenuHandle) {
                    wSaveMenuIDAroundSendMessage = wMenuID;    /* save*/
                    hSaveMenuHandleAroundSendMessage = hSaveMenuHandle;
                    wSaveFlagsAroundSendMessage = wSaveFlags;

                    SendMessage(hWnd, WM_CANCELMODE, 0, 0L);
                    wMenuID = wSaveMenuIDAroundSendMessage;    /* restore*/
                    hSaveMenuHandle = hSaveMenuHandleAroundSendMessage;
                    wSaveFlags = wSaveFlagsAroundSendMessage;
                }

                if (!(wSaveFlags & MF_POPUP)) {

                    if (wSaveFlags & MF_SYSMENU){
                        dwContext = bFrameSysMenu ? IDH_SYSMENU : IDH_SYSMENUCHILD;
                    }
                    else {
                        dwContext = wMenuID + IDH_HELPFIRST;
                    }

                    PMHelp(hWnd);
                }
            }
            else if (wParam == MSGF_DIALOGBOX) {
                /* context range for message boxes*/

                if (dwContext >= IDH_MBFIRST && dwContext <= IDH_MBLAST){
                    PMHelp(hWnd);
                }

                /* let dialog box deal with it*/
                PostMessage(GetRealParent((HWND)lParam), uiHelpMessage, 0, 0L);
            }
        }
        else{
CallDFP:
            return DefFrameProc(hWnd, hwndMDIClient, uiMsg, wParam, lParam);
        }
    }
    return 0L;
}
