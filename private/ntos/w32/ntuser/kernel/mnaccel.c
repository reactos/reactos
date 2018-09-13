/**************************** Module Header ********************************\
* Module Name: mnaccel.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Keyboard Accelerator Routines
*
* History:
* 10-10-90 JimA       Cleanup.
* 03-18-91 IanJa      Window revalidation added
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
*
*
* History:
\***************************************************************************/

int ItemContainingSubMenu(
    PMENU pmainMenu,
    ULONG_PTR wID)
{
    int i;
    PITEM pItem;

    if ((i = pmainMenu->cItems - 1) == -1)
        return -1;

    pItem = &pmainMenu->rgItems[i];

    /*
     * Scan through mainMenu's items (bottom up) until an item is found
     * that either has subMenu or an ancestor of subMenu as it's drop
     * down menu
     */

    /*
     * Make sure this works for new apps that set IDs for popup items that
     * aren't the same as the HMENU_16 value of the submenu.  Accelerators
     * for disabled items will get generated otherwise, like in Exchange.
     */
    while (i >= 0)
    {
        if (pItem->spSubMenu == NULL)
        {
            //
            // Does this command match?
            //
            if (pItem->wID == wID)
                break;
        }
        else
        {
            //
            // Does this popup match?
            //
            if (pItem->spSubMenu == (PMENU)wID)
                break;

            //
            // Go recurse through this popup and see if we have a match on
            // one of our children.
            //
            if (ItemContainingSubMenu(pItem->spSubMenu, wID) != -1)
                break;
        }

        i--;
        pItem--;
    }

    return i;
}

/***************************************************************************\
* UT_FindTopLevelMenuIndex
*
* !
*
* History:
\***************************************************************************/

int UT_FindTopLevelMenuIndex(
    PMENU pMenu,
    UINT cmd)
{
    PMENU pMenuItemIsOn;
    PITEM  pItem;

    /*
     * Get a pointer to the item we are searching for.
     */
    pItem = MNLookUpItem(pMenu, cmd, FALSE, &pMenuItemIsOn);
    if ((pItem == NULL) || (pItem->spSubMenu != NULL))
        return(-1);

    /*
     * We want to search for the item that contains pMenuItemIsOn,
     * unless this is a top-level item without a dropdown, in which
     * case we want to search for cmd.
     */
    return ItemContainingSubMenu(pMenu,
                    pMenuItemIsOn != pMenu ? (ULONG_PTR)pMenuItemIsOn : cmd);
}

/***************************************************************************\
* xxxHiliteMenuItem
*
* !
*
* History:
\***************************************************************************/

BOOL xxxHiliteMenuItem(
    PWND pwnd,
    PMENU pMenu,
    UINT cmd,
    UINT flags)
{

    if (!(flags & MF_BYPOSITION))
        cmd = (UINT)UT_FindTopLevelMenuIndex(pMenu, cmd);

    if (!TestMF(pMenu, MFISPOPUP))
        xxxMNRecomputeBarIfNeeded(pwnd, pMenu);

    xxxMNInvertItem(NULL, pMenu, cmd, pwnd, (flags & MF_HILITE));

    return TRUE;
}

/***************************************************************************\
* xxxTA_AccelerateMenu
*
* !
*
* History:
\***************************************************************************/

#define TA_DISABLED 1

UINT xxxTA_AccelerateMenu(
    PWND pwnd,
    PMENU pMenu,
    UINT cmd,
    HMENU *phmenuInit)
{
    int i;
    PITEM pItem;
    BOOL fDisabledTop;
    BOOL fDisabled;
    UINT rgfItem;
    PMENU pMenuItemIsOn;

    CheckLock(pwnd);
    CheckLock(pMenu);

    rgfItem = 0;
    if (pMenu != NULL) {
        if ((i = UT_FindTopLevelMenuIndex(pMenu, cmd)) != -1) {

            /*
             * 2 means we found an item
             */
            rgfItem = 2;

            xxxSendMessage(pwnd, WM_INITMENU, (WPARAM)PtoHq(pMenu), 0L);
            if ((UINT)i >= pMenu->cItems)
                return 0;

            pItem = &pMenu->rgItems[i];
            if (pItem->spSubMenu != NULL) {
                *phmenuInit = PtoHq(pItem->spSubMenu);
                xxxSendMessage(pwnd, WM_INITMENUPOPUP, (WPARAM)*phmenuInit,
                        (DWORD)i);
                if ((UINT)i >= pMenu->cItems)
                    return 0;
                fDisabledTop = TestMFS(pItem,MFS_GRAYED);
            } else {
                fDisabledTop = FALSE;
            }

            pItem = MNLookUpItem(pMenu, cmd, FALSE, &pMenuItemIsOn);

            /*
             * If the item was removed by the app in response to either of
             * the above messages, pItem will be NULL.
             */
            if (pItem == NULL) {
                rgfItem = 0;
            } else {
                fDisabled = TestMFS(pItem,MFS_GRAYED);

                /*
                 * This 1 bit means it's disabled or it's 'parent' is disabled.
                 */
                if (fDisabled || fDisabledTop)
                    rgfItem |= TA_DISABLED;
            }
        }
    }

    return rgfItem;
}

/***************************************************************************\
* _CreateAcceleratorTable
*
* History:
* 05-01-91 ScottLu      Changed to work client/server
* 02-26-91 mikeke       Created.
\***************************************************************************/

HANDLE APIENTRY _CreateAcceleratorTable(
    LPACCEL ccxpaccel,
    int cbAccel)
{
    LPACCELTABLE pat;
    int size;

    size = cbAccel + sizeof(ACCELTABLE) - sizeof(ACCEL);

    pat = (LPACCELTABLE)HMAllocObject(PtiCurrent(), NULL, TYPE_ACCELTABLE, size);
    if (pat == NULL)
        return NULL;

    try {
        RtlCopyMemory(pat->accel, ccxpaccel, cbAccel);
    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
        HMFreeObject(pat);
        return NULL;
    }

    pat->cAccel = cbAccel / sizeof(ACCEL);
    pat->accel[pat->cAccel - 1].fVirt |= FLASTKEY;

    return pat;
}

/***************************************************************************\
* xxxTranslateAccelerator
*
* !
*
* History:
\***************************************************************************/

int xxxTranslateAccelerator(
    PWND pwnd,
    LPACCELTABLE pat,
    LPMSG lpMsg)
{
    UINT cmd;
    BOOL fVirt;
    PMENU pMenu;
    BOOL fFound;
    UINT flags;
    UINT keystate;
    UINT message;
    UINT rgfItem;
    BOOL fDisabled;
    BOOL fSystemMenu;
    LPACCEL paccel;
    TL tlpMenu;
    int vkAlt, vkCtrl;
    HMENU hmenuInit = NULL;

    CheckLock(pwnd);
    CheckLock(pat);

    if (gfInNumpadHexInput & NUMPAD_HEXMODE_HL) {
        return FALSE;
    }

    paccel = pat->accel;

    fFound = FALSE;

    message = SystoChar(lpMsg->message, lpMsg->lParam);

    switch (message) {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        fVirt = TRUE;
        break;

    case WM_CHAR:
    case WM_SYSCHAR:
        fVirt = FALSE;
        break;

    default:
        return FALSE;
    }

    /*
     * Many kbd layouts use the r.h. Alt key like a shift key to generate some
     * additional chars: this r.h. Alt (or "AltGr") key synthesizes a left Ctrl
     * (for backward compatibility with 84-key kbds), so when the AltGr key is
     * down neither the left Ctrl nor the right Alt should be counted as part
     * of the keystate.
     * Note: Don't expect spklActive == NULL (winlogon should have loaded kbd
     * layouts already), but test it  anyway to be robust. #99321)
     */
    keystate = 0;
    UserAssert(PtiCurrent()->spklActive != NULL);   // #99321
    if (PtiCurrent()->spklActive &&
            (PtiCurrent()->spklActive->spkf->pKbdTbl->fLocaleFlags & KLLF_ALTGR) &&
            (_GetKeyState(VK_RMENU) & 0x8000)) {
        /*
         * count only right hand Ctrl as a Ctrl keystate
         * count only left hand Alt as a Alt keystate
         */
        vkCtrl = VK_RCONTROL;
        vkAlt = VK_LMENU;
    } else {
        /*
         * count left or right hand Ctrl as a Ctrl keystate
         * count left or right hand Alt as a Alt keystate
         */
        vkAlt = VK_MENU;
        vkCtrl = VK_CONTROL;
    }

    if (_GetKeyState(vkCtrl) & 0x8000) {
        keystate |= FCONTROL;
    }
    if (_GetKeyState(vkAlt) & 0x8000) {
        keystate |= FALT;
    }
    if (_GetKeyState(VK_SHIFT) & 0x8000) {
        keystate |= FSHIFT;
    }

    do
    {
        flags = paccel->fVirt;
        if ( (DWORD)paccel->key != lpMsg->wParam ||
             ((fVirt != 0) != ((flags & FVIRTKEY) != 0))) {
            goto Next;
        }

        if (fVirt && ((keystate & (FSHIFT | FCONTROL)) != (flags & (FSHIFT | FCONTROL)))) {
            goto Next;
        }

        if ((keystate & FALT) != (flags & FALT)) {
            goto Next;
        }

        fFound = TRUE;
        fSystemMenu = 0;
        rgfItem = 0;

        cmd = paccel->cmd;
        if (cmd != 0) {

            /*
             * The order of these next two if's is important for default
             * situations.  Also, just check accelerators in the system
             * menu of child windows passed to TranslateAccelerator.
             */
            pMenu = pwnd->spmenu;
            rgfItem = 0;

            if (!TestWF(pwnd, WFCHILD)) {
                ThreadLock(pMenu, &tlpMenu);
                rgfItem = xxxTA_AccelerateMenu(pwnd, pMenu, cmd, &hmenuInit);
                ThreadUnlock(&tlpMenu);
            }

            if (TestWF(pwnd, WFCHILD) || rgfItem == 0) {
                UserAssert(hmenuInit == NULL);
                pMenu = pwnd->spmenuSys;
                if (pMenu == NULL && TestWF(pwnd, WFSYSMENU)) {

                    /*
                     * Change owner so this app can access this menu.
                     */
                    pMenu = pwnd->head.rpdesk->spmenuSys;
                    if (pMenu == NULL) {
                        pMenu = xxxLoadSysDesktopMenu (&pwnd->head.rpdesk->spmenuSys, ID_SYSMENU);
                    }
                    ThreadLock(pMenu, &tlpMenu);
                    /*
                     * Must reset the system menu for this window.
                     */
                    xxxSetSysMenu(pwnd);
                } else {
                    ThreadLock(pMenu, &tlpMenu);
                }

                if ((rgfItem = xxxTA_AccelerateMenu(pwnd, pMenu, cmd, &hmenuInit)) != 0) {
                    fSystemMenu = TRUE;
                }
                ThreadUnlock(&tlpMenu);
            }
        }

        fDisabled = TestWF(pwnd, WFDISABLED);

        /*
         * Send only if:  1.  The Item is not disabled, AND
         *                2.  The Window's not being captured AND
         *                3.  The Window's not minimzed, OR
         *                4.  The Window's minimized but the Item is in
         *                   the System Menu.
         */
        if (!(rgfItem & TA_DISABLED) &&
                !(rgfItem && TestWF(pwnd, WFICONIC) && !fSystemMenu)) {
            if (!(rgfItem != 0 && (PtiCurrent()->pq->spwndCapture != NULL ||
                    fDisabled))) {

                if (fSystemMenu) {
                    xxxSendMessage(pwnd, WM_SYSCOMMAND, cmd, 0x00010000L);
                } else {
                    xxxSendMessage(pwnd, WM_COMMAND, MAKELONG(cmd, 1), 0);
                }

                /*
                 * Get outta here
                 */
                flags = FLASTKEY;
            }
        }

        /*
         * Send matching WM_UNINITMENUPOPUP if needed
         */
        if (hmenuInit != NULL) {
            xxxSendMessage(pwnd, WM_UNINITMENUPOPUP, (WPARAM)hmenuInit, 0);
            hmenuInit = NULL;
        }

    Next:
        paccel++;

    } while (!(flags & FLASTKEY) && !fFound);


    return fFound;
}

/***************************************************************************\
* SystoChar
*
* EXIT: If the message was not made with the ALT key down, convert
*       the message from a WM_SYSKEY* to a WM_KEY* message.
*
* IMPLEMENTATION:
*     The 0x2000 bit in the hi word of lParam is set if the key was
*     made with the ALT key down.
*
* History:
*   11/30/90 JimA       Ported.
\***************************************************************************/

UINT SystoChar(
    UINT message,
    LPARAM lParam)
{
    if (CheckMsgFilter(message, WM_SYSKEYDOWN, WM_SYSDEADCHAR) &&
            !(HIWORD(lParam) & SYS_ALTERNATE))
        return (message - (WM_SYSKEYDOWN - WM_KEYDOWN));

    return message;
}
