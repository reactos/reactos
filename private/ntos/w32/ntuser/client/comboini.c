/**************************** Module Header ********************************\
* Module Name: comboini.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* All the (one time) initialization/destruction code used for combo boxes
*
* History:
* 12-05-90 IanJa        Ported
* 01-Feb-1991 mikeke    Added Revalidation code
* 20-Jan-1992 IanJa     ANSI/UNIOCDE netralization
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

extern LOOKASIDE ComboboxLookaside;

#define RECALC_CYDROP   -1

void xxxCBSetDroppedSize(PCBOX pcbox, LPRECT lprc);

/***************************************************************************\
* CBNcCreateHandler
*
* Allocates space for the CBOX structure and sets the window to point to it.
*
* History:
\***************************************************************************/

LONG CBNcCreateHandler(
    PCBOX pcbox,
    PWND pwnd)
{
    /*
     * Save the style bits so that we have them when we create the client area
     * of the combo box window.
     */
    pcbox->styleSave = pwnd->style & (WS_VSCROLL | WS_HSCROLL);

    if (!TestWF(pwnd, CBFOWNERDRAW))
        // Add in CBS_HASSTRINGS if the style is implied...
        SetWindowState(pwnd, CBFHASSTRINGS);

    UserAssert(HIBYTE(WFVSCROLL) == HIBYTE(WFHSCROLL));
    UserAssert(HIBYTE(WFHSCROLL) == HIBYTE(WFBORDER));
    ClearWindowState(pwnd, WFVSCROLL | WFHSCROLL | WFBORDER);

    //
    // If the window is 4.0 compatible or has a CLIENTEDGE, draw the combo
    // in 3D.  Otherwise, use a flat border.
    //
    if (TestWF(pwnd, WFWIN40COMPAT) || TestWF(pwnd, WEFCLIENTEDGE))
        pcbox->f3DCombo = TRUE;

    ClearWindowState(pwnd, WEFEDGEMASK);

    return (LONG)TRUE;
}

/***************************************************************************\
* xxxCBCreateHandler
*
* Creates all the child controls within the combo box
* Returns -1 if error
*
* History:
\***************************************************************************/

LRESULT xxxCBCreateHandler(
    PCBOX pcbox,
    PWND pwnd)
{
    LONG lStyleT;
    RECT rcList;
    HWND hwndList;
    HWND hwndEdit;
    DWORD lExStyle;

    CheckLock(pwnd);

    /*
     * Don't lock the combobox window: this would prevent WM_FINALDESTROY
     * being sent to it, so pwnd and pcbox wouldn't get freed (zombies)
     * until thread cleanup. (IanJa)  LATER: change name from spwnd to pwnd.
     * Lock(&pcbox->spwnd, pwnd); - caused a 'catch-22'
     */
    Lock(&(pcbox->spwndParent), REBASEPWND(pwnd, spwndParent));

    /*
     * Break out the style bits so that we will be able to create the listbox
     * and editcontrol windows.
     */

    if (TestWF(pwnd, CBFDROPDOWNLIST) == LOBYTE(CBFDROPDOWNLIST)) {
        pcbox->CBoxStyle = SDROPDOWNLIST;
        pcbox->fNoEdit = TRUE;
    } else if (TestWF(pwnd, CBFDROPDOWN))
        pcbox->CBoxStyle = SDROPDOWN;
    else
        pcbox->CBoxStyle = SSIMPLE;

    pcbox->fRtoLReading = (TestWF(pwnd, WEFRTLREADING) != 0);
    pcbox->fRightAlign  = (TestWF(pwnd, WEFRIGHT) != 0);

    if (TestWF(pwnd, CBFUPPERCASE))
        pcbox->fCase = UPPERCASE;
    else if (TestWF(pwnd, CBFLOWERCASE))
        pcbox->fCase = LOWERCASE;
    else
        pcbox->fCase = 0;

    // Listbox item flags.
    if (TestWF(pwnd, CBFOWNERDRAWVAR))
        pcbox->OwnerDraw = OWNERDRAWVAR;
    if (TestWF(pwnd, CBFOWNERDRAWFIXED)) {
        pcbox->OwnerDraw = OWNERDRAWFIXED;
    }

    /*
     * Get the size of the combo box rectangle.
     */
    // Get control sizes.
    pcbox->cxCombo = pwnd->rcWindow.right - pwnd->rcWindow.left;
    pcbox->cyDrop  = RECALC_CYDROP;
    pcbox->cxDrop  = 0;
    xxxCBCalcControlRects(pcbox, &rcList);

    //
    // We need to do this because listboxes, as of VER40, have stopped
    // reinflating themselves by CXBORDER and CYBORDER.
    //
    if (!TestWF(pwnd, WFWIN40COMPAT))
        InflateRect(&rcList, -SYSMET(CXBORDER), -SYSMET(CYBORDER));

    /*
     * Note that we have to create the listbox before the editcontrol since the
     * editcontrol code looks for and saves away the listbox pwnd and the
     * listbox pwnd will be NULL if we don't create it first.  Also, hack in
     * some special +/- values for the listbox size due to the way we create
     * listboxes with borders.
     */
    lStyleT = pcbox->styleSave;

    lStyleT |= WS_CHILD | WS_VISIBLE | LBS_NOTIFY | LBS_COMBOBOX | WS_CLIPSIBLINGS;

    if (TestWF(pwnd, WFDISABLED))
        lStyleT |= WS_DISABLED;
    if (TestWF(pwnd, CBFNOINTEGRALHEIGHT))
        lStyleT |= LBS_NOINTEGRALHEIGHT;
    if (TestWF(pwnd, CBFSORT))
        lStyleT |= LBS_SORT;
    if (TestWF(pwnd, CBFHASSTRINGS))
        lStyleT |= LBS_HASSTRINGS;
    if (TestWF(pwnd, CBFDISABLENOSCROLL))
        lStyleT |= LBS_DISABLENOSCROLL;

    if (pcbox->OwnerDraw == OWNERDRAWVAR)
        lStyleT |= LBS_OWNERDRAWVARIABLE;
    else if (pcbox->OwnerDraw == OWNERDRAWFIXED)
        lStyleT |= LBS_OWNERDRAWFIXED;

    if (pcbox->CBoxStyle & SDROPPABLE)
        lStyleT |= WS_BORDER;

    lExStyle = pwnd->ExStyle & (WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR);
    hwndList = _CreateWindowEx(lExStyle |
            ((pcbox->CBoxStyle & SDROPPABLE) ? WS_EX_TOOLWINDOW : WS_EX_CLIENTEDGE),
            MAKEINTRESOURCE(gpsi->atomSysClass[ICLS_COMBOLISTBOX]), NULL, lStyleT,
            rcList.left, rcList.top, rcList.right - rcList.left,
            rcList.bottom - rcList.top,
            HW(pwnd), (HMENU)CBLISTBOXID, pcbox->spwnd->hModule, NULL,
            0);
    Lock(&(pcbox->spwndList), ValidateHwnd(hwndList));

    if (!pcbox->spwndList) {
        return -1;
    }

    /*
     * Create either the edit control or the static text rectangle.
     */
    if (pcbox->fNoEdit) {

        /*
         * No editcontrol so we will draw text directly into the combo box
         * window.
         */
        /*
         * Don't lock the combobox window: this would prevent WM_FINALDESTROY
         * being sent to it, so pwnd and pcbox wouldn't get freed (zombies)
         * until thread cleanup. (IanJa)  LATER: change name from spwnd to pwnd.
         * Lock(&(pcbox->spwndEdit), pcbox->spwnd); - caused a 'catch-22'
         */
        pcbox->spwndEdit = pcbox->spwnd;
    } else {
        DWORD dwCsFlags;

        lStyleT = WS_CHILD | WS_VISIBLE | ES_COMBOBOX | ES_NOHIDESEL;
        if (TestWF(pwnd, WFDISABLED))
            lStyleT |= WS_DISABLED;
        if (TestWF(pwnd, CBFAUTOHSCROLL))
            lStyleT |= ES_AUTOHSCROLL;
        if (TestWF(pwnd, CBFOEMCONVERT))
            lStyleT |= ES_OEMCONVERT;
        if (pcbox->fCase)
            lStyleT |= (pcbox->fCase & UPPERCASE) ? ES_UPPERCASE : ES_LOWERCASE;

        /*
         * Edit control need to know whether original CreateWindow*() call
         * was ANSI or Unicode.
         */
        dwCsFlags = TestWF(pcbox->spwnd, WFANSICREATOR) ? CW_FLAGS_ANSI : 0L;
        if (lExStyle & WS_EX_RIGHT)
            lStyleT |= ES_RIGHT;

        hwndEdit = _CreateWindowEx(lExStyle,
            MAKEINTRESOURCE(gpsi->atomSysClass[ICLS_EDIT]), NULL, lStyleT,
            pcbox->editrc.left, pcbox->editrc.top,
            pcbox->editrc.right - pcbox->editrc.left, pcbox->editrc.bottom -
            pcbox->editrc.top, HW(pwnd), (HMENU)CBEDITID,
            pcbox->spwnd->hModule, NULL,
            dwCsFlags);
        Lock(&(pcbox->spwndEdit), ValidateHwnd(hwndEdit));
    }
    if (!pcbox->spwndEdit)
        return -1L;

    if (pcbox->CBoxStyle & SDROPPABLE) {

        NtUserShowWindow(hwndList, SW_HIDE);
        NtUserSetParent(hwndList, NULL);

        // We need to do this so dropped size works right
        if (!TestWF(pwnd, WFWIN40COMPAT))
            InflateRect(&rcList, SYSMET(CXBORDER), SYSMET(CYBORDER));

        xxxCBSetDroppedSize(pcbox, &rcList);
    }

    /*
     * return anything as long as it's not -1L (-1L == error)
     */
    return (LRESULT)pwnd;
}

/***************************************************************************\
* xxxCBCalcControlRects
*
* History:
\***************************************************************************/

void xxxCBCalcControlRects(PCBOX pcbox, LPRECT lprcList)
{
    HDC hdc;
    HANDLE hOldFont = NULL;
    int             dyEdit, dxEdit;
    MEASUREITEMSTRUCT mis;
    SIZE size;
    HWND hwnd = HWq(pcbox->spwnd);
    TL tlpwndParent;

    CheckLock(pcbox->spwnd);

    /*
     * Determine height of the edit control.  We can use this info to center
     * the button with recpect to the edit/static text window.  For example
     * this will be useful if owner draw and this window is tall.
     */
    hdc = NtUserGetDC(hwnd);
    if (pcbox->hFont) {
        hOldFont = SelectObject(hdc, pcbox->hFont);
    }

    // Add on CYEDGE just for some extra space in the edit field/static item.
    // It's really only for static text items, but we want static & editable
    // controls to be the same height.
    GetTextExtentPoint(hdc, szOneChar, 1, &size);
    dyEdit = size.cy + SYSMET(CYEDGE);

    if (hOldFont) {
        SelectObject(hdc, hOldFont);
    }

    /*
     * IanJa: was ReleaseDC(pcbox->hwnd, hdc);
     */
    NtUserReleaseDC(hwnd, hdc);

    if (pcbox->OwnerDraw) {
        // This is an ownerdraw combo.  Have the owner tell us how tall this
        // item is.
        int iOwnerDrawHeight;

        if (iOwnerDrawHeight = pcbox->editrc.bottom - pcbox->editrc.top) {
            dyEdit = iOwnerDrawHeight;
        } else {
            /*
             * No height has been defined yet for the static text window.  Send
             * a measure item message to the parent
             */
            mis.CtlType = ODT_COMBOBOX;
            mis.CtlID = PtrToUlong(pcbox->spwnd->spmenu);
            mis.itemID = (UINT)-1;
            mis.itemHeight = dyEdit;
            mis.itemData = 0;

            ThreadLock(pcbox->spwndParent, &tlpwndParent);
            SendMessage(HW(pcbox->spwndParent), WM_MEASUREITEM, mis.CtlID, (LPARAM)&mis);
            ThreadUnlock(&tlpwndParent);

            dyEdit = mis.itemHeight;
        }
    }
    /*
     * Set the initial width to be the combo box rect.  Later we will shorten it
     * if there is a dropdown button.
     */
    pcbox->cyCombo = 2*SYSMET(CYFIXEDFRAME) + dyEdit;
    dxEdit = pcbox->cxCombo - (2 * SYSMET(CXFIXEDFRAME));

    if (pcbox->cyDrop == RECALC_CYDROP)
    {
        // recompute the max height of the dropdown listbox -- full window
        // size MINUS edit/static height
        pcbox->cyDrop = max((pcbox->spwnd->rcWindow.bottom - pcbox->spwnd->rcWindow.top) - pcbox->cyCombo, 0);

        if (!TestWF(pcbox->spwnd, WFWIN40COMPAT) && (pcbox->cyDrop == 23))
            // This is VC++ 2.1's debug/release dropdown that they made super
            // small -- let's make 'em a wee bit bigger so the world can
            // continue to spin -- jeffbog -- 4/19/95 -- B#10029
            pcbox->cyDrop = 28;
    }

    /*
     * Determine the rectangles for each of the windows...  1.  Pop down button 2.
     * Edit control or generic window for static text or ownerdraw...  3.  List
     * box
     */

    // Is there a button?
    if (pcbox->CBoxStyle & SDROPPABLE) {
        // Determine button's rectangle.
        pcbox->buttonrc.top = SYSMET(CYEDGE);
        pcbox->buttonrc.bottom = pcbox->cyCombo - SYSMET(CYEDGE);
        if (pcbox->fRightAlign) {
            pcbox->buttonrc.left  = SYSMET(CXFIXEDFRAME);
            pcbox->buttonrc.right = pcbox->buttonrc.left + SYSMET(CXVSCROLL);
        } else {
            pcbox->buttonrc.right = pcbox->cxCombo - SYSMET(CXEDGE);
            pcbox->buttonrc.left  = pcbox->buttonrc.right - SYSMET(CXVSCROLL);
        }

        // Reduce the width of the edittext window to make room for the button.
        dxEdit = max(dxEdit - SYSMET(CXVSCROLL), 0);

    } else {

        /*
         * No button so make the rectangle 0 so that a point in rect will always
         * return false.
         */
        SetRectEmpty(&pcbox->buttonrc);
    }

    /*
     * So now, the edit rect is really the item area.
     */
    pcbox->editrc.left      = SYSMET(CXFIXEDFRAME);
    pcbox->editrc.right     = pcbox->editrc.left + dxEdit;
    pcbox->editrc.top       = SYSMET(CYFIXEDFRAME);
    pcbox->editrc.bottom    = pcbox->editrc.top + dyEdit;

    // Is there a right-aligned button?
    if ((pcbox->CBoxStyle & SDROPPABLE) && (pcbox->fRightAlign)) {
        pcbox->editrc.right   = pcbox->cxCombo - SYSMET(CXEDGE);
        pcbox->editrc.left    = pcbox->editrc.right - dxEdit;
    }

    lprcList->left          = 0;
    lprcList->top           = pcbox->cyCombo;
    lprcList->right         = max(pcbox->cxDrop, pcbox->cxCombo);
    lprcList->bottom        = pcbox->cyCombo + pcbox->cyDrop;
}

/***************************************************************************\
* xxxCBNcDestroyHandler
*
* Destroys the combobox and frees up all memory used by it
*
* History:
\***************************************************************************/

void xxxCBNcDestroyHandler(
    PWND pwnd,
    PCBOX pcbox)
{
    CheckLock(pwnd);

    /*
     * If there is no pcbox, there is nothing to clean up.
     */
    if (pcbox != NULL) {

        /*
         * Destroy the list box here so that it'll send WM_DELETEITEM messages
         * before the combo box turns into a zombie.
         */
        if (pcbox->spwndList != NULL) {
            NtUserDestroyWindow(HWq(pcbox->spwndList));
            Unlock(&pcbox->spwndList);
        }

        pcbox->spwnd = NULL;
        Unlock(&pcbox->spwndParent);

        /*
         * If there is no editcontrol, spwndEdit is the combobox window which
         * isn't locked (that would have caused a 'catch-22').
         */
        if (pwnd != pcbox->spwndEdit) {
            Unlock(&pcbox->spwndEdit);
        }

        /*
         * Since a pointer and a handle to a fixed local object are the same.
         */
        FreeLookasideEntry(&ComboboxLookaside, pcbox);
    }

    /*
     * Set the window's fnid status so that we can ignore rogue messages
     */
    NtUserSetWindowFNID(HWq(pwnd), FNID_CLEANEDUP_BIT);
}

/***************************************************************************\
* xxxCBSetFontHandler
*
* History:
\***************************************************************************/

void xxxCBSetFontHandler(
    PCBOX pcbox,
    HANDLE hFont,
    BOOL fRedraw)
{
    TL tlpwndEdit;
    TL tlpwndList;

    CheckLock(pcbox->spwnd);

    ThreadLock(pcbox->spwndEdit, &tlpwndEdit);
    ThreadLock(pcbox->spwndList, &tlpwndList);

    pcbox->hFont = hFont;

    if (!pcbox->fNoEdit && pcbox->spwndEdit) {
        SendMessageWorker(pcbox->spwndEdit, WM_SETFONT, (WPARAM)hFont, FALSE, FALSE);
    }

    SendMessageWorker(pcbox->spwndList, WM_SETFONT, (WPARAM)hFont, FALSE, FALSE);

    // Recalculate the layout of controls.  This will hide the listbox also.
    xxxCBPosition(pcbox);

    if (fRedraw) {
        NtUserInvalidateRect(HWq(pcbox->spwnd), NULL, TRUE);
// LATER UpdateWindow(HW(pcbox->spwnd));
    }

    ThreadUnlock(&tlpwndList);
    ThreadUnlock(&tlpwndEdit);
}

/***************************************************************************\
* xxxCBSetEditItemHeight
*
* Sets the height of the edit/static item of a combo box.
*
* History:
* 06-27-91 DarrinM      Ported from Win 3.1.
\***************************************************************************/

LONG xxxCBSetEditItemHeight(
    PCBOX pcbox,
    int dyEdit)
{
    TL tlpwndEdit;
    TL tlpwndList;

    CheckLock(pcbox->spwnd);

    if (dyEdit > 255) {
        RIPERR0(ERROR_INVALID_EDIT_HEIGHT, RIP_VERBOSE, "");
        return CB_ERR;
    }

    pcbox->editrc.bottom = pcbox->editrc.top + dyEdit;
    pcbox->cyCombo = pcbox->editrc.bottom + SYSMET(CYFIXEDFRAME);

    if (pcbox->CBoxStyle & SDROPPABLE) {
        pcbox->buttonrc.bottom = pcbox->cyCombo - SYSMET(CYEDGE);
    }

    ThreadLock(pcbox->spwndEdit, &tlpwndEdit);
    ThreadLock(pcbox->spwndList, &tlpwndList);


    /*
     * Reposition the editfield.
     * Don't let spwndEdit or List of NULL go through; if someone adjusts
     * the height on a NCCREATE; same as not having
     * HW instead of HWq but we don't go to the kernel.
     */
    if (!pcbox->fNoEdit && pcbox->spwndEdit) {
        NtUserMoveWindow(HWq(pcbox->spwndEdit), pcbox->editrc.left, pcbox->editrc.top,
            pcbox->editrc.right-pcbox->editrc.left, dyEdit, TRUE);
    }

    /*
     * Reposition the list and combobox windows.
     */
    if (pcbox->CBoxStyle == SSIMPLE) {
        if (pcbox->spwndList != 0) {
            NtUserMoveWindow(HWq(pcbox->spwndList), 0, pcbox->cyCombo, pcbox->cxCombo,
                pcbox->cyDrop, FALSE);

            NtUserSetWindowPos(HWq(pcbox->spwnd), HWND_TOP, 0, 0,
                pcbox->cxCombo, pcbox->cyCombo +
                pcbox->spwndList->rcWindow.bottom - pcbox->spwndList->rcWindow.top,
                SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
    } else {
        if (pcbox->spwndList != NULL) {
            NtUserMoveWindow(HWq(pcbox->spwndList), pcbox->spwnd->rcWindow.left,
                pcbox->spwnd->rcWindow.top + pcbox->cyCombo,
                max(pcbox->cxDrop, pcbox->cxCombo), pcbox->cyDrop, FALSE);
        }

        NtUserSetWindowPos(HWq(pcbox->spwnd), HWND_TOP, 0, 0,
            pcbox->cxCombo, pcbox->cyCombo,
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    ThreadUnlock(&tlpwndList);
    ThreadUnlock(&tlpwndEdit);

    return CB_OKAY;
}


/***************************************************************************\
* xxxCBSizeHandler
*
* Recalculates the sizes of the internal controls in response to a
* resizing of the combo box window.  The app must size the combo box to its
* maximum open/dropped down size.
*
* History:
\***************************************************************************/

void xxxCBSizeHandler(
    PCBOX pcbox)
{
    CheckLock(pcbox->spwnd);

    /*
     * Assume listbox is visible since the app should size it to its maximum
     * visible size.
     */
    pcbox->cxCombo = pcbox->spwnd->rcWindow.right - pcbox->spwnd->rcWindow.left;
    // only recalc cyDrop if the drop down is not TOTALLY nuked by the sizing
    // -- Visio 1.0 -- B#13112
    if (((pcbox->spwnd->rcWindow.bottom - pcbox->spwnd->rcWindow.top) - pcbox->cyCombo) > 0)
        pcbox->cyDrop = RECALC_CYDROP;

    // Reposition everything.
    xxxCBPosition(pcbox);
}

/***************************************************************************\
*
*  CBPosition()
*
*  Repositions components of edit control.
*
\***************************************************************************/
void xxxCBPosition(PCBOX pcbox)
{
    RECT rcList;

    // Calculate placement of components--button, item, list
    xxxCBCalcControlRects(pcbox, &rcList);

    if (!pcbox->fNoEdit && pcbox->spwndEdit) {
        TL tlpwndEdit;

        ThreadLock(pcbox->spwndEdit, &tlpwndEdit);
        NtUserMoveWindow(HWq(pcbox->spwndEdit), pcbox->editrc.left, pcbox->editrc.top,
            pcbox->editrc.right - pcbox->editrc.left,
            pcbox->editrc.bottom - pcbox->editrc.top, TRUE);
        ThreadUnlock(&tlpwndEdit);
    }

    // Recalculate drop height & width
    xxxCBSetDroppedSize(pcbox, &rcList);
}

/***************************************************************************\
*
*  CBSetDroppedSize()
*
*  Compute the drop down window's width and max height
*
\***************************************************************************/
void xxxCBSetDroppedSize(PCBOX pcbox, LPRECT lprc)
{
    TL tlpwndList;

    pcbox->fLBoxVisible = TRUE;
    xxxCBHideListBoxWindow(pcbox, FALSE, FALSE);

    ThreadLock(pcbox->spwndList, &tlpwndList);
    NtUserMoveWindow(HWq(pcbox->spwndList), lprc->left, lprc->top,
        lprc->right - lprc->left, lprc->bottom - lprc->top, FALSE);
    ThreadUnlock(&tlpwndList);

}
