/***************************************************************************\
*
*  MDIMENU.C -
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
*      MDI "Window" Menu Support
*
* History
* 11-14-90 MikeHar     Ported from windows
* 14-Feb-1991 mikeke   Added Revalidation code
/****************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* FindPwndChild
*
* History:
* 11-14-90 MikeHar Ported from windows
\***************************************************************************/

PWND FindPwndChild(
    PWND pwndMDI,
    UINT wChildID)
{
    PWND pwndT;

    for (pwndT = REBASEPWND(pwndMDI, spwndChild);
            pwndT && (pwndT->spwndOwner || PtrToUlong(pwndT->spmenu) != wChildID);
            pwndT = REBASEPWND(pwndT, spwndNext))
        ;

    return pwndT;
}


/***************************************************************************\
* MakeMenuItem
*
* History:
* 11-14-90 MikeHar Ported from windows
*  4-16-91 Win31 Merge
\***************************************************************************/

int MakeMenuItem(
    LPWSTR lpOut,
    PWND pwnd)
{
    PMDI pmdi;
    DWORD rgParm;
    int cch = 0;
    WCHAR string[160];
    LPWSTR lpstr;
    int i = 0;

    /*
     * Get a pointer to the MDI structure
     */
    pmdi = ((PMDIWND)(REBASEPWND(pwnd, spwndParent)))->pmdi;

    *lpOut = 0;

    rgParm = PtrToUlong(pwnd->spmenu) - (DWORD)FIRST(pmdi) + 1;

    if (pwnd->strName.Length) {
        lpstr = REBASEALWAYS(pwnd, strName.Buffer);

        /*
         * Search for an & in the title string and duplicate it so that we don't
         * get bogus accelerators.
         */
        while (*lpstr && i < ((sizeof(string) / sizeof(WCHAR)) - 1)) {
            string[i] = *lpstr;
            i++;
            if (*lpstr == TEXT('&'))
                string[i++] = TEXT('&');

            lpstr++;
        }

        string[i] = 0;
        cch = wsprintfW(lpOut, L"&%d %ws", rgParm, string);

    } else {

        /*
         * Handle the case of MDI children without any window title text.
         */
        cch = wsprintfW(lpOut, L"&%d ", rgParm);
    }

    return cch;
}

/***************************************************************************\
* ModifyMenuItem
*
* History:
* 11-14-90 MikeHar Ported from windows
\***************************************************************************/

void ModifyMenuItem(
    PWND pwnd)
{
    PMDI pmdi;
    WCHAR sz[200];
    MENUITEMINFO    mii;
    PWND pwndParent;
    PMENU pmenu;

    /*
     * Get a pointer to the MDI structure
     */
    pwndParent = REBASEPWND(pwnd, spwndParent);
    pmdi = ((PMDIWND)pwndParent)->pmdi;

    if (PtrToUlong(pwnd->spmenu) > FIRST(pmdi) + (UINT)8)
        return;

    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_STRING;
    mii.dwTypeData = sz;

    /*
     * Parent is MDI Client.
     */
    MakeMenuItem(sz, pwnd);

    /*
     * Changing the active child?  Check it.
     */
    if (HWq(pwnd) == ACTIVE(pmdi)) {
        mii.fMask |= MIIM_STATE;
        mii.fState = MFS_CHECKED;
    }

    pwndParent = REBASEPWND(pwndParent, spwndParent);

    if (pwndParent->spmenu) {

        /*
         * Bug# 21566. If spmenu is NULL we used to fail
         * because REBASEALWAYS is trying to get the kernel
         * address of NULL based on pwndParent
         */
        pmenu = REBASEALWAYS(pwndParent, spmenu);
        /*
         * Internal call to SetMenuItemInfo
         */
        ThunkedMenuItemInfo(PtoH(pmenu), PtrToUlong(pwnd->spmenu), FALSE, FALSE, &mii, FALSE);
    }
}

/***************************************************************************\
* MDIAddSysMenu
*
* Insert the MDI child's system menu onto the existing Menu.
*
* History:
* 11-14-90 MikeHar Ported from windows
\***************************************************************************/

BOOL MDIAddSysMenu(
    HMENU hMenuFrame,
    HWND hwndChild)
{
    PWND pwndChild;
    MENUITEMINFO    mii;
    PMENU pMenuChild;


// LATER -- look at passing pwndChild in -- FritzS

    UserAssert(IsWindow(hwndChild));
    pwndChild = ValidateHwnd(hwndChild);
    if (!hMenuFrame || !pwndChild || !pwndChild->spmenuSys) {
        return FALSE;
    }

    /*
     * We don't need the pMenuChild pointer but the handle. However, if you
     * do PtoH(_GetSubMenu()), you end up calling the function twice
     */
    pMenuChild = _GetSubMenu (REBASEALWAYS(pwndChild, spmenuSys), 0);
    if (!pMenuChild) {
        return FALSE;
    }

// Add MDI system button as first menu item
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_SUBMENU | MIIM_DATA | MIIM_BITMAP;  // Add MIIM_DATA because of hack described below
    mii.hSubMenu = PtoH(pMenuChild);
// Fritzs -- this won't work.
//    mii.dwTypeData = (LPSTR) MAKELONG(MENUHBM_SYSTEM, GetWindowSmIcon(hwndChild));
    mii.hbmpItem = HBMMENU_SYSTEM;
// FritzS -- so, we sneak the icon into ItemData
    mii.dwItemData = (ULONG_PTR)hwndChild;

    if (!InternalInsertMenuItem(hMenuFrame, 0, TRUE, &mii))
        return FALSE;

    // TimeLine 6.1 gets confused by the extra Min/Close buttons,
    // don't add them if WFOLDUI

    mii.fMask = MIIM_ID | MIIM_FTYPE | MIIM_BITMAP;
    mii.fType = MFT_RIGHTJUSTIFY;

    pwndChild = ValidateHwnd(hwndChild);
    if (!pwndChild) {
        NtUserRemoveMenu(hMenuFrame, 0, MF_BYPOSITION);
        return FALSE;
    }


    if (!TestWF(pwndChild, WFOLDUI))
    {
        // Add Minimize button as last menu item
        mii.hbmpItem = (TestWF(pwndChild, WFMINBOX) ? HBMMENU_MBAR_MINIMIZE : HBMMENU_MBAR_MINIMIZE_D);
        mii.wID = SC_MINIMIZE;

        if (!InternalInsertMenuItem(hMenuFrame, MFMWFP_NOITEM, TRUE, &mii))
        {
            NtUserRemoveMenu(hMenuFrame, 0, MF_BYPOSITION);
            return FALSE;
        }
        mii.fType &= ~MFT_RIGHTJUSTIFY;
    }

    // Add Restore button as last menu item
    mii.hbmpItem = HBMMENU_MBAR_RESTORE;
    mii.wID = SC_RESTORE;

    if (!InternalInsertMenuItem(hMenuFrame, MFMWFP_NOITEM, TRUE, &mii)) {
        // BOGUS -- we gotta remove the MINIMIZE button too
        NtUserRemoveMenu(hMenuFrame, 0, MF_BYPOSITION);
        return FALSE;
    }

    pwndChild = ValidateHwnd(hwndChild);
    if (!pwndChild) {
        NtUserRemoveMenu(hMenuFrame, 0, MF_BYPOSITION);
        return FALSE;
    }

    if (!TestWF(pwndChild, WFOLDUI))
    {
        // Add Close button as last menu item
        mii.hbmpItem = (xxxMNCanClose(pwndChild) ? HBMMENU_MBAR_CLOSE : HBMMENU_MBAR_CLOSE_D);
        mii.wID = SC_CLOSE;

        if (!InternalInsertMenuItem(hMenuFrame, MFMWFP_NOITEM, TRUE, &mii))
        {
            // BOGUS -- we gotta remove the MINIMIZE and RESTORE buttons too
            NtUserRemoveMenu(hMenuFrame, 0, MF_BYPOSITION);
            return FALSE;
        }
    }

    /*
     * Set the menu items to proper state since we just maximized it.  Note
     * setsysmenu doesn't work if we've cleared the sysmenu bit so do it now...
     */
    NtUserSetSysMenu(hwndChild);

    /*
     * This is so that if the user brings up the child sysmenu, it's sure
     * to be that in the frame menu bar...
     */
    ClearWindowState(pwndChild, WFSYSMENU);

    /*
     * Make sure that the child's frame is redrawn to reflect the removed
     * system menu.
     */
    NtUserRedrawFrame(hwndChild);

    return TRUE;
}

/***************************************************************************\
* MDIRemoveSysMenu
*
* History:
* 11-14-90 MikeHar Ported from windows
\***************************************************************************/

BOOL MDIRemoveSysMenu(
    HMENU hMenuFrame,
    HWND hwndChild)
{
    int iLastItem;
    UINT    iLastCmd;
    PWND pwndChild;

// LATER -- look at passing pwndChild in -- FritzS

    if (hMenuFrame == NULL)
        return FALSE;

    pwndChild = ValidateHwnd(hwndChild);

    if (pwndChild == NULL)
        return FALSE;

    iLastItem = GetMenuItemCount(hMenuFrame) - 1;
    iLastCmd = TestWF(pwndChild, WFOLDUI) ? SC_RESTORE : SC_CLOSE;

    if ((UINT) GetMenuItemID(hMenuFrame, iLastItem) != iLastCmd)
        return FALSE;

    /*
     * Enable the sysmenu in the child window.
     */
    SetWindowState(pwndChild, WFSYSMENU);

    /*
     * Take the child sysmenu popup out of the frame menu.
     */
    NtUserRemoveMenu(hMenuFrame, 0, MF_BYPOSITION);

    /*
     * Delete the restore button from the menu bar.
     */
    NtUserDeleteMenu(hMenuFrame, iLastItem - 1, MF_BYPOSITION);

    pwndChild = ValidateHwnd(hwndChild);
    if (pwndChild == NULL)
        return FALSE;

    if (!TestWF(pwndChild, WFOLDUI)) {
        NtUserDeleteMenu(hMenuFrame, iLastItem - 2, MF_BYPOSITION);
        NtUserDeleteMenu(hMenuFrame, iLastItem - 3, MF_BYPOSITION);
    }

    /*
     * Make sure that the child's frame is redrawn to reflect the added
     * system menu.
     */
    NtUserRedrawFrame(hwndChild);

    return TRUE;
}

/***************************************************************************\
* AppendToWindowsMenu
*
* Add the title of the MDI child window 'hwndChild' to the bottom of the
* "Window" menu (or add the "More Windows ..." item) if there's room.
*
*   MDI Child #                    Add
*  -------------          --------------------
*   < MAXITEMS             Child # and Title
*   = MAXITEMS             "More Windows ..."
*   > MAXITEMS             nothing
*
* History:
* 17-Mar-1992 mikeke   from win31
\***************************************************************************/

BOOL FAR PASCAL AppendToWindowsMenu(
    PWND pwndMDI,
    PWND pwndChild)
{
    PMDI pmdi;
    WCHAR szMenuItem[165];
    int item;
    MENUITEMINFO    mii;

    /*
     * Get a pointer to the MDI structure
     */
    pmdi = ((PMDIWND)pwndMDI)->pmdi;

    item = PtrToUlong(pwndChild->spmenu) - FIRST(pmdi);

    if (WINDOW(pmdi) && (item < MAXITEMS)) {
        mii.cbSize = sizeof(MENUITEMINFO);
        if (!item) {

            /*
             * Add separator before first item
             */
            mii.fMask = MIIM_FTYPE;
            mii.fType = MFT_SEPARATOR;
            if (!InternalInsertMenuItem(WINDOW(pmdi), MFMWFP_NOITEM, TRUE, &mii))
                return FALSE;
        }

        if (item == (MAXITEMS - 1))
            LoadString(hmodUser, STR_MOREWINDOWS, szMenuItem,
                       sizeof(szMenuItem) / sizeof(WCHAR));
        else
            MakeMenuItem(szMenuItem, pwndChild);

        mii.fMask = MIIM_ID | MIIM_STRING;
        mii.wID = PtrToUlong(pwndChild->spmenu);
        mii.dwTypeData = szMenuItem;
        mii.cch = (UINT)-1;
        if (!InternalInsertMenuItem(WINDOW(pmdi), MFMWFP_NOITEM, TRUE, &mii))
            return FALSE;
    }
    return TRUE;
}

/***************************************************************************\
* SwitchWindowsMenus
*
* Switch the "Window" menu in the frame menu bar 'hMenu' from
* 'hOldWindow' to 'hNewWindow'
*
* History:
* 17-Mar-1992 mikeke    from win31
\***************************************************************************/

BOOL SwitchWindowsMenus(
    HMENU hmenu,
    HMENU hOldWindow,
    HMENU hNewWindow)
{
    int i;
    HMENU hsubMenu;
    WCHAR szMenuName[128];
    MENUITEMINFO    mii;

    if (hOldWindow == hNewWindow)
        return TRUE;

    mii.cbSize = sizeof(MENUITEMINFO);

    /*
     * Determine position of old "Window" menu
     */
    for (i = 0; hsubMenu = GetSubMenu(hmenu, i); i++) {
        if (hsubMenu == hOldWindow)
        {
            // Extract the name of the old menu to use it for the new menu
            mii.fMask = MIIM_STRING;
            mii.dwTypeData = szMenuName;
            mii.cch = sizeof(szMenuName)/sizeof(WCHAR);
            GetMenuItemInfoInternalW(hmenu, i, TRUE, &mii);
            // Out with the old, in with the new
            if (!NtUserRemoveMenu(hmenu, i, MF_BYPOSITION))
                return(FALSE);

            mii.fMask |= MIIM_SUBMENU;
            mii.hSubMenu = hNewWindow;
            return(InternalInsertMenuItem(hmenu, i, TRUE, &mii));
        }
    }

    return(FALSE);
}

/***************************************************************************\
* ShiftMenuIDs
*
* Shift the id's of the MDI child windows of the MDI client window 'hWnd'
* down by 1 (id--) starting with the child window 'hwndVictim' -- moving
* 'hwndVictim' to the end of the list
*
* History:
* 17-Mar-1992 mikeke   from win31
\***************************************************************************/

void ShiftMenuIDs(
    PWND pwnd,
    PWND pwndVictim)
{
    PMDI pmdi;
    PWND pwndChild;
    PWND pwndParent;
    /*
     * Get a pointer to the MDI structure
     */
    pmdi = ((PMDIWND)pwnd)->pmdi;

    pwndParent = REBASEPWND(pwndVictim, spwndParent);
    pwndChild = REBASEPWND(pwndParent, spwndChild);

    while (pwndChild) {
        if (!pwndChild->spwndOwner && (pwndChild->spmenu > pwndVictim->spmenu)) {
            SetWindowLongPtr(HWq(pwndChild), GWLP_ID, PtrToUlong(pwndChild->spmenu) - 1);
        }
        pwndChild = REBASEPWND(pwndChild, spwndNext);
    }

    SetWindowLongPtr(HWq(pwndVictim), GWLP_ID, FIRST(pmdi) + CKIDS(pmdi) - 1);
}

/***************************************************************************\
* MDISetMenu
*
* History:
* 11-14-90 MikeHar Ported from windows
\***************************************************************************/

HMENU MDISetMenu(
    PWND pwndMDI,
    BOOL fRefresh,
    HMENU hNewSys,
    HMENU hNewWindow)
{
    int i;
    int iFirst;
    int item;
    PMDI pmdi;
    PWND pwndParent;
    HMENU hOldSys;
    HMENU hOldWindow;
    PWND pwndChild;

    /*
     * Get a pointer to the MDI structure
     */
    pmdi = ((PMDIWND)pwndMDI)->pmdi;

    /*
     * Save the old values
     */
    pwndParent = REBASEPWND(pwndMDI, spwndParent);
    hOldSys = GetMenu(HW(pwndParent));
    hOldWindow = WINDOW(pmdi);

    if (fRefresh) {
        hNewSys = hOldSys;
        hNewWindow = hOldWindow;
    }

    /*
     * Change the Frame Menu.
     */
    if (hNewSys && (hNewSys != hOldSys)) {
        if (MAXED(pmdi))
            MDIRemoveSysMenu(hOldSys, MAXED(pmdi));

        NtUserSetMenu(HW(pwndParent), hNewSys, FALSE);

        if (MAXED(pmdi))
            MDIAddSysMenu(hNewSys, MAXED(pmdi));
    } else
        hNewSys = hOldSys;

    /*
     * Now update the Window menu.
     */
    if (fRefresh || (hOldWindow != hNewWindow)) {
        iFirst = FIRST(pmdi);

        if (hOldWindow) {
            int cItems = GetMenuItemCount(hOldWindow);

            for (i = cItems - 1; i >= 0; i--) {
                if (GetMenuState(hOldWindow, i, MF_BYPOSITION) & MF_SEPARATOR)
                   break;
            }
            if ((i >= 0) && (GetMenuItemID(hOldWindow, i + 1) == (UINT)iFirst)) {
                int idTrim = i;

                for (i = idTrim; i < cItems; i++)
                    NtUserDeleteMenu(hOldWindow, idTrim, MF_BYPOSITION);
            }
        }

        Lock(&WINDOW(pmdi), hNewWindow);

        if (hNewWindow != NULL) {

           /*
            * Add the list of child windows to the new window
            */
           for (i = 0, item = 0; ((UINT)i < CKIDS(pmdi)) && (item < MAXITEMS);
                    i++) {
               pwndChild = FindPwndChild(pwndMDI, iFirst + item);
               if (pwndChild != NULL) {
                   if ((!TestWF(pwndChild, WFVISIBLE) &&
                          (LOWORD(pwndMDI->style) & 0x0001)) ||
                          TestWF(pwndChild, WFDISABLED)) {
                       ShiftMenuIDs(pwndMDI, pwndChild);
                   } else {
                       AppendToWindowsMenu(pwndMDI, pwndChild);
                       item++;
                   }
               }
           }

           /*
            * Add checkmark by the active child's menu item
            */
           if (ACTIVE(pmdi))
               CheckMenuItem(hNewWindow, (WORD)GetWindowID(ACTIVE(pmdi)),
                       MF_BYCOMMAND | MF_CHECKED);
        }

        /*
         * Out with the old, in with the new
         */
        SwitchWindowsMenus(hNewSys, hOldWindow, hNewWindow);
    }
    return hOldSys;
}

/***************************************************************************\
* xxxInitActivateDlg
*
* History:
* 11-14-90 MikeHar Ported from windows
\***************************************************************************/

void xxxInitActivateDlg(
    HWND hwnd,
    PWND pwndMDI)
{
    PMDI pmdi;
    UINT wKid;
    HWND hwndT;
    PWND pwndT;
    WCHAR szTitle[CCHTITLEMAX];
    TL tlpwndT;
    SIZE Size;
    HDC hDC;
    DWORD width = 0;

    CheckLock(pwndMDI);

    /*
     * Get a pointer to the MDI structure
     */
    pmdi = ((PMDIWND)pwndMDI)->pmdi;

    hDC = NtUserGetDC(hwnd);

    /*
     * Insert the list of titles.
     * Note the wKid-th item in the listbox has ID wKid+FIRST(pwnd), so that
     * the listbox is in creation order (like the menu).  This is also
     * helpful when we go to select one...
     */

    for (wKid = 0; wKid < CKIDS(pmdi); wKid++) {
        pwndT = FindPwndChild(pwndMDI, (UINT)(wKid + FIRST(pmdi)));

        if (pwndT && TestWF(pwndT, WFVISIBLE) && !TestWF(pwndT, WFDISABLED)) {
            ThreadLockAlways(pwndT, &tlpwndT);
            GetWindowText(HWq(pwndT), szTitle, CCHTITLEMAX);
            SendDlgItemMessage(hwnd, 100, LB_ADDSTRING, 0, (LPARAM)szTitle);
            GetTextExtentPoint(hDC, szTitle, lstrlen(szTitle), &Size);
            if (Size.cx > (LONG)width) {
                width = Size.cx;
            }
            ThreadUnlock(&tlpwndT);
        }
    }

    /*
     * Select the currently active window.
     */
    SendDlgItemMessage(hwnd, 100, LB_SETTOPINDEX, MAXITEMS - 1, 0L);
    SendDlgItemMessage(hwnd, 100, LB_SETCURSEL, MAXITEMS - 1, 0L);

    /*
     * Set the horizontal extent of the list box to the longest window title.
     */
    SendDlgItemMessage(hwnd, 100, LB_SETHORIZONTALEXTENT, width, 0L);
    NtUserReleaseDC(hwnd, hDC);

    /*
     * Set the focus to the listbox.
     */
    hwndT = GetDlgItem(hwnd, 100);
    NtUserSetFocus(hwndT);
}

/***************************************************************************\
* MDIActivateDlgSize
*
* The minimum allowed size and the previous one are saved as properties
* of the parent window.
*
* History:
* Oct 97 MCostea Created
\***************************************************************************/

VOID MDIActivateDlgSize(HWND hwnd, int width, int height)
{
    PMDIACTIVATEPOS pPos;
    PWND pwnd, pwndList, pwndButtonLeft, pwndButtonRight;
    HDWP hdwp;
    int  deltaX, deltaY;

    pPos = (PMDIACTIVATEPOS)GetProp(GetParent(hwnd), MAKEINTATOM(atomMDIActivateProp));
    if (pPos == NULL) {
        return;
    }

    /*
     * Retrieve the children
     */
    if ((pwnd = ValidateHwnd(hwnd)) == NULL) {
        return;
    }
    pwndList = REBASEPWND(pwnd, spwndChild);
    pwndButtonLeft  = REBASEPWND(pwndList, spwndNext);
    pwndButtonRight = REBASEPWND(pwndButtonLeft, spwndNext);

    UserAssert(GETFNID(pwndList) == FNID_LISTBOX);
    UserAssert(GETFNID(pwndButtonLeft) == FNID_BUTTON);
    UserAssert(GETFNID(pwndButtonRight) == FNID_BUTTON);
    UserAssert(pwndButtonRight->rcWindow.left > pwndButtonLeft->rcWindow.left);

    deltaX = width - pPos->cx;
    deltaY = height - pPos->cy;

    pPos->cx = width;
    pPos->cy = height;

    /*
     * Move/resize the child windows accordingly
     */
    hdwp = NtUserBeginDeferWindowPos(3);

    if (hdwp)
    {
        hdwp = NtUserDeferWindowPos( hdwp,
                               PtoH(pwndList),
                               NULL,
                               0,
                               0,
                               deltaX + pwndList->rcWindow.right - pwndList->rcWindow.left,
                               deltaY + pwndList->rcWindow.bottom - pwndList->rcWindow.top,
                               SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );

        if (hdwp)
        {
            hdwp = NtUserDeferWindowPos(hdwp,
                               PtoH(pwndButtonLeft),
                               NULL,
                               pwndButtonLeft->rcWindow.left - pwnd->rcClient.left,
                               deltaY + pwndButtonLeft->rcWindow.top - pwnd->rcClient.top,
                               0,
                               0,
                               SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );


            if (hdwp)
            {
                hdwp = NtUserDeferWindowPos( hdwp,
                               PtoH(pwndButtonRight),
                               NULL,
                               pwndButtonRight->rcWindow.left - pwnd->rcClient.left,
                               deltaY + pwndButtonRight->rcWindow.top - pwnd->rcClient.top,
                               0,
                               0,
                               SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
            }

        }
        if (hdwp) {
            NtUserEndDeferWindowPosEx(hdwp, FALSE);
        }
    }
}

/***************************************************************************\
* MDIActivateDlgInit
*
* The minimum allowed size and the previous one are saved as properties
* of the parent window.
*
* History:
* Oct 97 MCostea Created
\***************************************************************************/

VOID MDIActivateDlgInit(HWND hwnd, LPARAM lParam)
{
    PMDIACTIVATEPOS pPos;
    HWND  hwndParent;
    RECT  rc;

    xxxInitActivateDlg(hwnd, (PWND)lParam);

    hwndParent = GetParent(hwnd);
    /*
     * Preserve the previous size of the dialog, if any
     */
    if (atomMDIActivateProp == 0) {

        atomMDIActivateProp = AddAtomW(MDIACTIVATE_PROP_NAME);
        UserAssert(atomMDIActivateProp);
    }

    GetWindowRect(hwnd, &rc);

    pPos = (PMDIACTIVATEPOS)GetProp(hwndParent, MAKEINTATOM(atomMDIActivateProp));
    /*
     * If the dialog was used before, retrieve it's size
     */
    if (pPos != NULL) {

        int cxBorder, cyBorder, cx, cy;

        /*
         * The stored size and the ones in WM_SIZE are client window coordinates
         * Need to adjust them for NtUserSetWindowPos and WM_INITDIALOG
         */
        cxBorder = rc.right - rc.left;
        cyBorder = rc.bottom - rc.top;
        GetClientRect(hwnd, &rc);
        cxBorder -= rc.right - rc.left;
        cyBorder -= rc.bottom - rc.top;

        NtUserSetWindowPos(hwnd, NULL, 0, 0,
                           pPos->cx + cxBorder,
                           pPos->cy + cyBorder,
                           SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER
                           | SWP_NOSENDCHANGING | SWP_NOREDRAW);
        cx = pPos->cx;
        cy = pPos->cy;
        pPos->cx = pPos->cxMin - cxBorder;
        pPos->cy = pPos->cyMin - cyBorder;
        MDIActivateDlgSize(hwnd, cx, cy);

    } else {
        /*
         *
         */
        pPos = UserLocalAlloc(0, sizeof(MDIACTIVATEPOS));
        if (pPos == NULL) {
            return;
        }
        pPos->cxMin = rc.right - rc.left;
        pPos->cyMin = rc.bottom - rc.top;

        GetClientRect(hwnd, &rc);
        pPos->cx = rc.right - rc.left;
        pPos->cy = rc.bottom - rc.top;
        SetProp(hwndParent, MAKEINTATOM(atomMDIActivateProp), (HANDLE)pPos);
    }
}

/***************************************************************************\
* MDIActivateDlgProc
*
* History:
* 11-14-90 MikeHar Ported from windows
\***************************************************************************/

INT_PTR MDIActivateDlgProcWorker(
    HWND hwnd,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    int i;

    switch (wMsg) {

    case WM_INITDIALOG:
        /*
         * NOTE: Code above uses DialogBoxParam, passing pwndMDI in the low
         * word of the parameter...
         */
        MDIActivateDlgInit(hwnd, lParam);
        return FALSE;

    case WM_COMMAND:
        i = -2;

        switch (LOWORD(wParam)) {

        /*
         * Listbox doubleclicks act like OK...
         */
        case 100:
            if (HIWORD(wParam) != LBN_DBLCLK)
                break;

        /*
         ** FALL THRU **
         */
        case IDOK:
            i = (UINT)SendDlgItemMessage(hwnd, 100, LB_GETCURSEL, 0, 0L);

        /*
         ** FALL THRU **
         */
        case IDCANCEL:
            EndDialog(hwnd, i);
            break;
        default:
            return FALSE;
        }
        break;

    case WM_SIZE:
        MDIActivateDlgSize(hwnd, LOWORD(lParam), HIWORD(lParam));
        return FALSE;

    case WM_GETMINMAXINFO:
        {
            PMDIACTIVATEPOS pPos;

            if (pPos = (PMDIACTIVATEPOS)GetProp(GetParent(hwnd), MAKEINTATOM(atomMDIActivateProp))) {
                 ((LPMINMAXINFO)lParam)->ptMinTrackSize.x = pPos->cxMin;
                 ((LPMINMAXINFO)lParam)->ptMinTrackSize.y = pPos->cyMin;
            }
            return FALSE;
        }

    default:
        return FALSE;
    }
    return TRUE;
}

INT_PTR WINAPI MDIActivateDlgProcA(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return MDIActivateDlgProcWorker(hwnd, message, wParam, lParam);
}

INT_PTR WINAPI MDIActivateDlgProcW(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return MDIActivateDlgProcWorker(hwnd, message, wParam, lParam);
}
