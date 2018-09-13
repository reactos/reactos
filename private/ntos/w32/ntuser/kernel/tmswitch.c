/****************************** Module Header ******************************\
* Module Name: tmswitch.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* History:
* 29-May-1991 DavidPe   Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*
 * COOLSWITCHTRACE is used to trace problems
 * in the CoolSwitch window
 */
#undef COOLSWITCHTRACE


#define DGF_NODRAW      1

#define ALT_F6          2
#define ALT_ESCAPE      1

#define FDIR_FORWARD    0
#define FDIR_BACKWARD   1


/*
 *  Win95 hard codes the size of the icon matrix, the size of
 *  the icons, the highlight border and the icon spacing
 */
#define CXICONSLOT      43
#define CYICONSLOT      43
#define CXICONSIZE      32
#define CYICONSIZE      32
#define MAXTASKNAMELEN  50



VOID xxxPaintIconsInSwitchWindow(PWND, PSWINFO, HDC, INT, INT, INT, BOOL, BOOL, PICON);

/***************************************************************************\
* Getpswi
*
* 04-29-96 GerardoB  Created
\***************************************************************************/
__inline PSWINFO Getpswi (PWND pwnd)
{
    UserAssert(GETFNID(pwnd) == FNID_SWITCH);
    return ((PSWITCHWND)pwnd)->pswi;
}
/***************************************************************************\
* Setpswi
*
* 04-29-96 GerardoB  Created
\***************************************************************************/
__inline void Setpswi (PWND pwnd, PSWINFO pswi)
{
    UserAssert(GETFNID(pwnd) == FNID_SWITCH);
    ((PSWITCHWND)pwnd)->pswi = pswi;
}
/***************************************************************************\
* DSW_GetTopLevelCreatorWindow
*
*
\***************************************************************************/

PWND DSW_GetTopLevelCreatorWindow(
    PWND pwnd)
{
    UserAssert(pwnd != NULL);

    if (pwnd != NULL) {
        while (pwnd->spwndOwner)
            pwnd = pwnd->spwndOwner;
    }

    return pwnd;
}

/***************************************************************************\
* GetNextQueueWindow
*
* This routine is used to implement the Alt+Esc feature.  This feature lets
* the user switch between windows for different applications (a.k.a. "Tasks")
* currently running.  We keep track of the most recently active window in
* each task.  This routine starts with the window passed and searches for the
* next window, in the "top-level" window list, that is from a different task
* than the one passed.  We then return the most recenly active window from
* that task (or the window we found if the most recently active has been
* destroyed or is currently disabled or hidden).  This routine returns NULL
* if no other enabled, visible window for another task can be found.
*
* History:
* 30-May-1991 DavidPe   Ported from Win 3.1 sources.
\***************************************************************************/

PWND _GetNextQueueWindow(
    PWND pwnd,
    BOOL fPrev, /* 1 backward 0 forward */
    BOOL fAltEsc)
{
    PWND        pwndAltTab;
    PWND        pwndNext;
    PWND        pwndT;
    PWND        pwndDesktop;
    BOOL        bBeenHereAlready = FALSE;
    PTHREADINFO ptiAltTab;

    /*
     * HACK: We have a problem with direct-draw full apps where an alttab
     *       window is created on a queue owned other than the RIT.  This
     *       shows up by alt-tabbing away from ROIDS.EXE during fullscreen.
     *
     *       What is happening is on a ALT-TAB, from xxxSysCommand(), the
     *       thread is not a rit.  xxxSysCommand() calls xxxOldNextWindow
     *       which finds that the current-thread doesn't have a switch
     *       window, and then creates one on the current-thread-queue.
     *
     *       The hack here is to make sure the calling thread is the RIT
     *       before allowing any cool-switch creation.
     *
     *       21-Mar-1996 : Chriswil
     */
#if 0
    ptiAltTab = PtiCurrent();
#else
    ptiAltTab = gptiRit;
#endif

    /*
     * If the window we receive is Null then use the last topmost window
     */
    if (!pwnd) {
        pwnd = GetLastTopMostWindow();
        if (!pwnd) {
            return NULL;
        }
    }

    pwndAltTab = gspwndAltTab;

    pwnd = pwndNext = GetTopLevelWindow(pwnd);
    if (!pwndNext)
        return NULL;

    /*
     * Get the window's desktop
     */
    if ((pwndDesktop = pwndNext->spwndParent) == NULL) {
        pwndDesktop = grpdeskRitInput->pDeskInfo->spwnd;
        pwnd = pwndNext = pwndDesktop->spwndChild;
    }

    while (TRUE) {

        if (pwndNext == NULL)
            return NULL;

        /*
         *  Get the next window
         */
        pwndNext = _GetWindow(pwndNext, fPrev ? GW_HWNDPREV : GW_HWNDNEXT);

        if (!pwndNext) {

            pwndNext = fPrev ? _GetWindow(pwndDesktop->spwndChild, GW_HWNDLAST)
                             : pwndDesktop->spwndChild;
            /*
             * To avoid searching the child chain forever, bale out if we get
             * to the end (beginning) of the chain twice.
             * This happens if pwnd is a partially destroyed window that has
             * been unlinked from its siblings but not yet unlinked from the
             * parent. (Happens while sending WM_NCDESTROY in xxxFreeWindow)
             */
            if (bBeenHereAlready) {
                RIPMSG1(RIP_WARNING, "pwnd %#p is no longer a sibling", pwnd);
                return NULL;
            }

            bBeenHereAlready = TRUE;
        }

        /*
         *  If we have gone all the way around with no success, return NULL.
         */
        if (!pwndNext || (pwndNext == pwnd))
            return NULL;

        /*
         *  Ignore the following windows:
         *      Switch window
         *      Tool Windows
         *      NoActivate Windows
         *      Hidden windows
         *      Disabled windows
         *      Topmost windows if via Alt+Esc
         *      Bottommost windows if via Alt+Esc
         *
         *  If we're doing Alt-Esc processing, we have to skip topmost windows.
         *
         *  Because topmost windows don't really go to the back when we
         *  send them there, alt-esc would never enumerate non-topmost windows.
         *  So, although we're allowed to start enumeration at a topmost window,
         *  we only allow enumeration of non-topmost windows, so the user can
         *  enumerate his presumably more important applications.
         */
        if ((pwndNext != pwndAltTab) &&
// BradG - Win95 is missing the check for Tool Windows
            (!TestWF(pwndNext, WEFTOOLWINDOW)) &&
            (!TestWF(pwndNext, WEFNOACTIVATE)) &&
            (TestWF(pwndNext, WFVISIBLE)) &&
            ((pwndNext->spwndLastActive == NULL) || (!TestWF(pwndNext->spwndLastActive, WFDISABLED)) &&
            (!fAltEsc || (!TestWF(pwndNext, WEFTOPMOST) && !TestWF(pwndNext, WFBOTTOMMOST))))) {
            /*
             * If this window is owned, don't return it unless it is the most
             * recently active window in its owner/ownee group.
             */
            /*
             *  Hard loop to find top level owner
             */
            for (pwndT = pwndNext; pwndT->spwndOwner; pwndT = pwndT->spwndOwner)
                ;

            /*
             *  Don't return it unless it is the most recently active
             *  window in its owner/ownee group.
             */
            if (pwndNext == pwndT->spwndLastActive)
                return pwndNext;
        }
    }
}

/***************************************************************************\
*
* SwitchToThisWindow()
*
* This function was added specifically for Win386.  It is called to tell
* USER that a particular window has been switched to via Alt+Tab or
* Alt+Esc in the Win386 environment.  They call this function to maintain
* Z-ordering and consistent operation of these two functions.  This function
* must be exported, but need not be documented.
*
* The parameter fTab is TRUE if this window is to be switched to via an
* Alt/Ctl+Tab key sequence otherwise it must be FALSE.
*
* History:
* 04-Feb-1991 DarrinM   Created.
\***************************************************************************/

VOID xxxSwitchToThisWindow(
    PWND pwnd,
    BOOL fAltTab)
{
    CheckLock(pwnd);

    /*
     *  If we need to, push old window to the bottom.
     */
    if (gpqForeground && !fAltTab) {

        BOOL fPush;
        PWND pwndActive;
        TL   tlpwndActive;

        /*
         *  if ALT-ESC, and the window brought forward is the next one in the
         *  list, we must be rotating the zorder forward, so push the current
         *  window to the back
         */
        pwndActive = gpqForeground->spwndActive;
        fPush = pwndActive && _GetNextQueueWindow(pwndActive, FDIR_FORWARD, !fAltTab);
        if (fPush && !TestWF(pwndActive, WEFTOPMOST) && !TestWF(pwndActive, WFBOTTOMMOST)) {
            ThreadLock(pwndActive, &tlpwndActive);
            xxxSetWindowPos(pwndActive, PWND_BOTTOM, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
            ThreadUnlock(&tlpwndActive);
        }
    }

    /*
     *  Switch this new window to the foreground
     *  This window can go away during the SetForeground call if it isn't
     *  on the thread calling SwitchToThisWindow()!
     */
    xxxSetForegroundWindow(pwnd, TRUE);

    /*
     * Restore minimized windows if the Alt+Tab case
     */
    if (fAltTab && TestWF(pwnd,WFMINIMIZED)) {

        /*
         * We need to package up a special 'posted' queue message here.  This
         * ensures that this message gets processed after the asynchronous
         * activation event occurs (via SetForegroundWindow).
         */
        PostEventMessage(GETPTI(pwnd), GETPTI(pwnd)->pq,
                         QEVENT_POSTMESSAGE, pwnd, WM_SYSCOMMAND,
                         SC_RESTORE, 0 );
    }
}

/***************************************************************************\
* NextPrevTaskIndex
*
* History:
* 01-Jun-1995 BradG     Ported from Win95
\***************************************************************************/

INT NextPrevTaskIndex(
    PSWINFO pswInfo,
    INT     iIndex,
    INT     iCount,
    BOOL    fNext)
{
    UserAssert(iCount <= pswInfo->iTotalTasks);

    if (fNext) {
        iIndex += iCount;
        if(iIndex >= pswInfo->iTotalTasks)
            iIndex -= pswInfo->iTotalTasks;
    } else {
        iIndex -= iCount;
        if(iIndex < 0)
            iIndex += pswInfo->iTotalTasks;
    }

    UserAssert((iIndex >= 0) && (iIndex < pswInfo->iTotalTasks));
    return iIndex;
}

/***************************************************************************\
* NextPrevPhwnd
*
* Given a pointer to one entry in the window list, this can return
* the pointer to the next/prev entry in a circular list fashion.
*
* History:
* 01-Jun-1995 BradG     Ported from Win95
\***************************************************************************/

PHWND NextPrevPhwnd(
    PSWINFO pswInfo,
    PHWND   phwnd,
    BOOL    fNext)
{
    PBWL  pbwl;
    PHWND phwndStart;
    PHWND phwndLast;

    pbwl = pswInfo->pbwl;
    phwndStart = &(pbwl->rghwnd[0]);
    phwndLast = pswInfo->phwndLast;

    UserAssert(*phwndLast == (HWND)1);   // Last entry must have a 1.
    UserAssert(phwndStart < phwndLast);  // There must be atleast one entry.
    UserAssert(phwnd != phwndLast);      // Can't be passing in an invalid entry.

    if (fNext) {
        phwnd++;
        if(phwnd == phwndLast)
            phwnd = phwndStart;
    } else {
        if (phwnd == phwndStart) {
            phwnd = phwndLast - 1;  // we have atleast one valid entry.
        } else {
            phwnd--;
        }
    }

    return phwnd;
}

/***************************************************************************\
* _IsTaskWindow
*
* History:
* 01-Jun-95 BradG       Ported from Win95
\***************************************************************************/

BOOL _IsTaskWindow(
    PWND pwnd,
    PWND pwndActive)
{
    /*
     *  Following windows do not qualify to be shown in task list:
     *  Switch  Window, Hidden windows (unless they are the active
     *  window), Disabled windows, Kanji Conv windows.
     *
     *  Also, check for a combobox popup list which has the top-most
     *  style (it's spwndLastActive will be NULL).
     */
    UserAssert(pwnd != NULL);
    return( (TestWF(pwnd, WEFAPPWINDOW)
                || (!TestWF(pwnd, WEFTOOLWINDOW) && !TestWF(pwnd, WEFNOACTIVATE))) &&
            (TestWF(pwnd, WFVISIBLE) || (pwnd == pwndActive)) &&
            (!(pwnd->spwndLastActive && TestWF(pwnd->spwndLastActive, WFDISABLED))));
}

/***************************************************************************\
* RemoveNonTaskWindows()
*
* Given a list of Windows, this walks down the list and removes the
* windows that do not qualify to be shown in the Task-Switch screen.
* This also shrinks the list when it removes some entries.
* Returns the total number of "tasks" (windows) qualified and remained
* in the list. The last entry will have a 1 as usual.
* It also returns a pointer to this last entry via params. It also
* returns the index of the Currently active task via params.
*
* History:
* 01-Jun-1995 BradG     Ported from Win95
\***************************************************************************/

INT _RemoveNonTaskWindows(
    PBWL  pbwl,
    PWND  pwndActive,
    LPINT lpiActiveTask,
    PHWND *pphwndLast)
{
    INT   iTaskCount = 0;
    PHWND phwnd;
    PWND  pwnd;
    PWND  pwndUse;
    PWND  pwndOwnee;
    PHWND phwndHole;

    *lpiActiveTask = -1;

    /*
     * Walk down the window list and do the following:
     *   1. Remove all entries that do not qualify to be shown in the task list.
     *   2. Count the total number of windows that qualify.
     *   3. Get the pointer to the entry that contains current active window.
     *   4. Get the pointer to the last dummy entry (that has 1 in it)
     */
    for (phwndHole = phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {
        pwnd = RevalidateHwnd( *phwnd );
// BradG - Win95 Assert
//         Maybe we just want to remove this window if it is gone.
//        UserAssert(pwnd != NULL);
        if (!pwnd)
            continue;

        if (_IsTaskWindow(pwnd, pwndActive)) {
            pwndUse = pwnd;

// BradG - Why is Win95 doing this?
//         We aren't making any callbacks.
//            Assert(IsWindow32(hwndUse));

            /*
             *  First let's find the task-list owner of this window
             */
            while (!TestWF(pwndUse, WEFAPPWINDOW) && pwndUse->spwndOwner) {
                pwndOwnee = pwndUse;
                pwndUse = pwndUse->spwndOwner;
                if (TestWF(pwndUse, WEFTOOLWINDOW)) {
                    /*
                     * If this is the owner of a top level property sheet,
                     *  show the property sheet.
                     */
                    if (TestWF(pwndOwnee, WEFCONTROLPARENT) && (pwndUse->spwndOwner == NULL)) {
                        pwndUse = pwnd;
                    } else {
                        pwndUse = NULL;
                    }
                    break;
                }
            }

            if (!pwndUse || !pwndUse->spwndLastActive)
                continue;

            /*
             *  walk up from the last active window 'til we find a valid task
             *  list window or until we run out of windows in the ownership
             *  chain
             */
            for (pwndUse = pwndUse->spwndLastActive; pwndUse; pwndUse = pwndUse->spwndOwner)
                if (_IsTaskWindow(pwndUse, pwndActive))
                    break;

            /*
             *  if we ran out of windows in the ownership chain then use the
             *  owned window itself -- or if we didn't run out of the ownership
             *  chain, then only include this window if it's the window in the
             *  ownership chain that we just found (VB will love us for it !)
             *  -- jeffbog -- 4/20/95 -- Win95C B#2821
             */
            if (!pwndUse || (pwndUse == pwnd)) {

                /*
                 *  Do we have any holes above this? If so, move this handle to
                 *  that hole.
                 */
                if (phwndHole < phwnd) {

                    /*
                     * Yes! There is a hole. Let us move the valid
                     * handle there.
                     */
                    *phwndHole = *phwnd;
                }

                if (pwndActive == pwnd)
                    *lpiActiveTask = iTaskCount;
                iTaskCount++;
                phwndHole++;  // Move to the next entry.
            }

            /*
             *  Else, leave it as a hole for later filling.
             */
        }
    }

    *phwndHole = (HWND)1;
    *pphwndLast = phwndHole;

    return iTaskCount;
}

/***************************************************************************\
* DrawSwitchWndHilite()
*
* This draws or erases the Hilite we draw around the icon to show which
* task we are going to switch to.
* This also updates the name on the Task title window.
*
* History:
* 01-Jun-1995 BradG     Ported from Win95
\***************************************************************************/

VOID DrawSwitchWndHilite(
    PSWINFO pswInfo,
    HDC     hdcSwitch,
    int     iCol,
    int     iRow,
    BOOL    fShow)
{
    BOOL        fGetAndReleaseIt;
    RECT        rcTemp;
    PTHREADINFO ptiAltTab;
    /*
     * HACK: We have a problem with direct-draw full apps where an alttab
     *       window is created on a queue owned other than the RIT.  This
     *       shows up by alt-tabbing away from ROIDS.EXE during fullscreen.
     *
     *       What is happening is on a ALT-TAB, from xxxSysCommand(), the
     *       thread is not a rit.  xxxSysCommand() calls xxxOldNextWindow
     *       which finds that the current-thread doesn't have a switch
     *       window, and then creates one on the current-thread-queue.
     *
     *       The hack here is to make sure the calling thread is the RIT
     *       before allowing any cool-switch creation.
     *
     *       21-Mar-1996 : Chriswil
     */
#if 0
    ptiAltTab = PtiCurrent();
#else
    ptiAltTab = gptiRit;
#endif

    /*
     *  Draw or erase the hilite depending on "fShow".
     */
    if (fGetAndReleaseIt = (hdcSwitch == NULL))
        hdcSwitch = _GetDCEx(gspwndAltTab, NULL, DCX_USESTYLE);

    rcTemp.left   = pswInfo->ptFirstRowStart.x + iCol * CXICONSLOT;
    rcTemp.top    = pswInfo->ptFirstRowStart.y + iRow * CYICONSLOT;
    rcTemp.right  = rcTemp.left + CXICONSLOT;
    rcTemp.bottom = rcTemp.top + CYICONSLOT;

    DrawFrame(hdcSwitch,
              &rcTemp,
              2,
              DF_PATCOPY | ((fShow ? COLOR_HIGHLIGHT : COLOR_3DFACE) << 3));


    /*
     *  Update the Task title window.
     */
    if (fShow) {
        WCHAR    szText[CCHTITLEMAX];
        INT      cch;
        COLORREF clrOldText, clrOldBk;
        PWND     pwnd;
        RECT     rcRect;
        HFONT    hOldFont;
        INT      iLeft;
        ULONG_PTR dwResult = 0;

        clrOldText = GreSetTextColor(hdcSwitch, SYSRGB(BTNTEXT));
        clrOldBk   = GreSetBkColor(hdcSwitch, SYSRGB(3DFACE));
        hOldFont = GreSelectFont(hdcSwitch, gpsi->hCaptionFont);


        /*
         * Validate this window handle; This could be some app that terminated
         * in the background and the following line will GP fault in that case;
         * BOGUS: We should handle it some other better way.
         */
        pwnd = RevalidateHwnd( *(pswInfo->phwndCurrent) );
        if (pwnd) {
            /*
             *  Get the windows title (up to MAXTASKNAMELEN's worth)
             */
// BradG - We can do better than MAXTASKNAMELEN characters!
            if (pwnd->strName.Length) {
                cch = TextCopy(&pwnd->strName, szText, CCHTITLEMAX);
            } else {
                *szText = TEXT('\0');
                cch = 0;
            }

            /*
             *  Draw the text
             */
            CopyRect(&rcRect, &pswInfo->rcTaskName);
            iLeft = rcRect.left;
            FillRect(hdcSwitch, &rcRect, SYSHBR(3DFACE));
            /*
             * If an lpk is installed let it draw the text.
             */
            if (GETPWNDPPI(pwnd)->dwLpkEntryPoints) {
                TL    tlpwnd;
                LPKDRAWSWITCHWND LpkDrawSwitchWnd;

                RtlInitLargeUnicodeString(&LpkDrawSwitchWnd.strName, szText, (UINT)-1);
                LpkDrawSwitchWnd.rcRect = rcRect;

                ThreadLock(pwnd, &tlpwnd);
                xxxSendMessageTimeout(pwnd, WM_LPKDRAWSWITCHWND, (WPARAM)hdcSwitch,
                        (LPARAM)&LpkDrawSwitchWnd, SMTO_ABORTIFHUNG, 100, &dwResult);
                ThreadUnlock(&tlpwnd);
            }
            /*
             * If the text wasn't draw by the lpk, draw it as best we can.
             */
            if (dwResult == 0) {
                DRAWTEXTPARAMS  dtp;

                dtp.cbSize = sizeof(dtp);
                dtp.iLeftMargin = 0;
                dtp.iRightMargin = 0;
                DrawTextEx(hdcSwitch, szText, cch, &rcRect, DT_NOPREFIX | DT_END_ELLIPSIS | DT_SINGLELINE, &dtp );
            }
        }

        GreSelectFont(hdcSwitch, hOldFont);
        GreSetBkColor(hdcSwitch, clrOldBk);
        GreSetTextColor(hdcSwitch, clrOldText);
    }

    if (fGetAndReleaseIt)
        _ReleaseDC(hdcSwitch);
}

/***************************************************************************\
* DrawIconCallBack
*
* This function is called by a Windows app returning his icon.
*
* History:
* 17-Jun-1993 mikesch       Created.
\***************************************************************************/

VOID CALLBACK DrawIconCallBack(
    HWND    hwnd,
    UINT    uMsg,
    ULONG_PTR dwData,
    LRESULT lResult)
{
    PWND pwndAltTab;

    /*
     *  dwData is the pointer to the switch window handle.
     *  If this Alt+Tab instance is still active, we need to derive this
     *  window's index in the bwl array, otherwise, we are receiving an icon
     *  for an old Alt+Tab window.
     */
    pwndAltTab = RevalidateHwnd((HWND)dwData);
    if (pwndAltTab && TestWF(pwndAltTab, WFVISIBLE)) {

        PSWINFO pswCurrent;
        PICON   pIcon;
        PHWND   phwnd;
        PWND    pwnd;
        PWND    pwndT;
        INT     iStartTaskIndex;
        TL      tlpwndAltTab;

        /*
         *  Derive this window's index in the BWL array
         */
        if ((pwnd = RevalidateHwnd(hwnd)) == NULL)
            return;

        /*
         *  Get the switch window info
         */
        pswCurrent = Getpswi(pwndAltTab);
        if (!pswCurrent)
            return;

        for (iStartTaskIndex = 0, phwnd=&(pswCurrent->pbwl->rghwnd[0]); *phwnd != (HWND)1; phwnd++, iStartTaskIndex++) {
            /*
             *  Because we list the active window in the Switch Window, the
             *  hwnd here might not be the same, so we also need to walk back
             *  to the top-level window to see if this is the right entry
             *  in the list.
             */
            for(pwndT = RevalidateHwnd(*phwnd); pwndT; pwndT = pwndT->spwndOwner) {
                if (pwnd == pwndT)
                    goto DrawIcon;
            }
        }
        return;

        /*
         *  Convert the App's HICON into a PICON, or if the App did not return
         *  an icon, use the Windows default icon.
         */
DrawIcon:
        pIcon = NULL;
        if (lResult)
            pIcon = HMValidateHandleNoRip((HCURSOR)lResult, TYPE_CURSOR);

        if (!pIcon)
            pIcon = SYSICO(WINLOGO);

        /*
         *  Paint this icon in the Alt+Tab window.
         */
        ThreadLockAlways(pwndAltTab, &tlpwndAltTab);
        xxxPaintIconsInSwitchWindow(pwndAltTab,
                                    pswCurrent,
                                    NULL,
                                    iStartTaskIndex,
                                    0,
                                    1,
                                    FALSE,
                                    FALSE,
                                    pIcon);
        ThreadUnlock(&tlpwndAltTab);
    }

    UNREFERENCED_PARAMETER(uMsg);
}

/***************************************************************************\
* TSW_CalcRowAndCol
*
* History:
* 01-Jun-1995 BradG     Ported from Win95
\***************************************************************************/

BOOL TSW_CalcRowAndCol(
    PSWINFO pswInfo,
    INT     iTaskIndex,
    LPINT   lpiRow,
    LPINT   lpiCol)
{
    INT iDiff;
    INT iRow;

    /*
     *  Calculate how far is the given task from the first task shown
     *  on the switch window.
     */
    if ((iDiff = (iTaskIndex - pswInfo->iFirstTaskIndex)) < 0)
        iDiff += pswInfo->iTotalTasks;

    /*
     *  Calculate the Row and if this lies outside the switch window, return FALSE
     */
    if ((iRow = iDiff / pswInfo->iNoOfColumns) >= pswInfo->iNoOfRows)
        return FALSE;

    /*
     *  Return the Row and column where this task lies.
     */
    *lpiRow = iRow;
    *lpiCol = iDiff - (iRow * pswInfo->iNoOfColumns);

    return TRUE;  // This task lies within the switch window.
}

/***************************************************************************\
* xxxPaintIconsInSwitchWindow()
*
* This can simply paint the icons in the switch window or Scroll the
* whole window UP/DOWN and then paint the remaining area;
*   * If fScroll is TRUE, then the second, third and fourth params are ignored.
*   * If hIcon is passed in, then we're being called by DrawIconCallBack and
*       iStartRow parameter is ignored in this case.
*
* History:
* 02-Jun-1995 BradG     Ported from Win95
\***************************************************************************/

VOID xxxPaintIconsInSwitchWindow(
    PWND    pwndAltTab,
    PSWINFO pswInfo,
    HDC     hdc,
    INT     iStartTaskIndex,
    INT     iStartRow,
    INT     iNoOfIcons,
    BOOL    fScroll,
    BOOL    fUp,
    PICON   pIcon)
{
    INT   cx, cy, xStart;
    PHWND phwnd;
    BOOL  fGetAndReleaseIt;
    INT   iColumnIndex = 0;
    RECT  rcScroll;
    PWND  pwnd;
    TL    tlpwnd;
    HICON hIcon;
    RECT  rcIcon;

    CheckLock(pwndAltTab);

    /*
     *  If we were not supplied a DC, get ghwndSwitch's and set a flag
     *  so we remember to release it.
     */
    if (fGetAndReleaseIt = (hdc == NULL))
        hdc = _GetDCEx(pwndAltTab, NULL, DCX_USESTYLE);

    cx = pswInfo->ptFirstRowStart.x;
    cy = pswInfo->ptFirstRowStart.y;

    if (fScroll) {

        rcScroll.left   = cx;
        rcScroll.top    = cy;
        rcScroll.right  = cx + CXICONSLOT * pswInfo->iNoOfColumns;
        rcScroll.bottom = cy + CYICONSLOT * pswInfo->iNoOfRows;

        _ScrollDC(hdc,
                  0,
                  (fUp ? -CYICONSLOT : CYICONSLOT),
                  &rcScroll,
                  &rcScroll,
                  NULL,
                  NULL);

        iStartRow = (fUp ? pswInfo->iNoOfRows - 1 : 0);
        iNoOfIcons = pswInfo->iNoOfColumns;
        iStartTaskIndex = (fUp ? NextPrevTaskIndex(pswInfo, pswInfo->iFirstTaskIndex,
                  (pswInfo->iNoOfRows - 1) * pswInfo->iNoOfColumns, TRUE) :
                   pswInfo->iFirstTaskIndex);
    }

    if (pIcon) {
        /*
         *  If pIcon is given, this is to paint just one icon during callback.
         */
// BradG - Win95 Assert
        UserAssert(iNoOfIcons == 1);

        /*
         *  Due to earlier scrolling, the row number would have changed. So,
         *  recalc the row and column from the iStartTaskIndex given.
         */
        if (!TSW_CalcRowAndCol(pswInfo, iStartTaskIndex, &iStartRow, &iColumnIndex))
            goto Cleanup;
    }

    xStart = cx += (CXICONSLOT - CXICONSIZE) / 2;
    cx += iColumnIndex * CXICONSLOT;
    cy += ((CYICONSLOT - CYICONSIZE) / 2) + iStartRow * CYICONSLOT;
    phwnd = &(pswInfo->pbwl->rghwnd[iStartTaskIndex]);

    /*
     *  Draw all the icons one by one.
     */
    while (iNoOfIcons--) {
        /*
         *  If the Alt+Key is no longer down, abort painting icons.
         */
        if ((pswInfo->fJournaling && _GetKeyState(VK_MENU) >= 0) ||
                (!pswInfo->fJournaling && _GetAsyncKeyState(VK_MENU) >= 0))
            goto Cleanup;

        /*
         *  Check if this window is still alive. (Some task could have
         *  terminated in the background)
         */
        if (pwnd = RevalidateHwnd(*phwnd)) {
            /*
             *  Find the window's top-level owner
             */
            pwnd = DSW_GetTopLevelCreatorWindow(pwnd);

            /*
             *  If we don't have an icon, find one
             */
            if (!pIcon) {
                /*
                 *  Try window icon
                 */
                hIcon = (HICON)_GetProp(pwnd, MAKEINTATOM(gpsi->atomIconProp), PROPF_INTERNAL);
                if (hIcon) {
                    pIcon = (PICON)HMValidateHandleNoRip(hIcon, TYPE_CURSOR);
                }

                /*
                 * If we don't have an icon yet, try the class icon
                 */
                if (!pIcon) {
                    pIcon = pwnd->pcls->spicn;
                }

                /*
                 * If we don't have an icon yet, use WM_QUERYDRAGICON to ask
                 * 3,x apps for their icon.
                 */
                if (!pIcon && !TestWF(pwnd, WFWIN40COMPAT)) {
                    /*
                     *  The callback routine will paint the icon for
                     *  us, so just leave pIcon set to NULL
                     */
                    ThreadLock(pwnd, &tlpwnd);
                    xxxSendMessageCallback(pwnd, WM_QUERYDRAGICON, 0, 0,
                            (SENDASYNCPROC)DrawIconCallBack,
                            HandleToUlong(PtoH(pwndAltTab)), FALSE);
                    ThreadUnlock(&tlpwnd);
                } else {
                    /*
                     *  If we can't find an icon, so use the Windows icon
                     */
                    if (!pIcon) {
                        pIcon = SYSICO(WINLOGO);
                    }
                }
            }
        }

        if (pIcon) {
            _DrawIconEx(hdc, cx, cy, pIcon, SYSMET(CXICON), SYSMET(CYICON),
                0, SYSHBR(3DFACE), DI_NORMAL);
        } else if (fScroll) {
            /*
             *  NOT IN WIN95
             *
             *  No icon was available, do while we are waiting for the
             *  application to paint it's icon, we need to "erase" the
             *  background in case we have scrolled the window.
             */
            rcIcon.left = cx;
            rcIcon.top = cy;
            rcIcon.right = cx + SYSMET(CXICON);
            rcIcon.bottom = cy + SYSMET(CYICON);
            FillRect(hdc, &rcIcon, SYSHBR(3DFACE));
        }

        /*
         *  Check if we are done.
         */
        if (iNoOfIcons <= 0)
            break;

        /*
         *  Reset hIcon for the next run through the loop
         */
        pIcon = NULL;

        /*
         *  Move all pointers to the next task/icon.
         */
        phwnd = NextPrevPhwnd(pswInfo, phwnd, TRUE); // Get next.

        /*
         *  Is it going to be in the same row; then adjust cx and cy.
         */
        if (++iColumnIndex >= pswInfo->iNoOfColumns) {
            iColumnIndex = 0;
            cx = xStart;        // Move to first column
            cy += CYICONSLOT;   // Move to next row.
            iStartRow++;
        } else {
            /*
             *  else, adjust cx;
             */
            cx += CXICONSLOT;
        }

        iStartTaskIndex = NextPrevTaskIndex(pswInfo, iStartTaskIndex, 1, TRUE);
    }

Cleanup:
    if (fGetAndReleaseIt)
        _ReleaseDC(hdc);
}

/***************************************************************************\
* PaintSwitchWindow
*
* History:
* 02-Jun-1995 BradG     Ported from Win95
\***************************************************************************/

VOID xxxPaintSwitchWindow(
    PWND pwndSwitch)
{
    LPRECT  lprcRect;
    RECT    rcRgn;
    HDC     hdcSwitch;
    PSWINFO pswCurrent;
    CheckLock(pwndSwitch);

    /*
     *  If our window isn't visible, return
     */
    if (!TestWF(pwndSwitch, WFVISIBLE))
        return;

    /*
     *  Get the switch window information
     */
    pswCurrent = Getpswi(pwndSwitch);
    if (!pswCurrent)
        return;

    /*
     * Get the Switch windows DC so we can paint with it
     */
    hdcSwitch = _GetDCEx(pwndSwitch, NULL, DCX_USESTYLE );

    /*
     *  Paint the background of the Switch Screen.
     */
    if ((pswCurrent->fJournaling && _GetKeyState(VK_MENU) >= 0) ||
            (!pswCurrent->fJournaling && _GetAsyncKeyState(VK_MENU) >= 0))
        goto PSWExit;

    lprcRect = &(pswCurrent->rcTaskName);
    _GetClientRect(pwndSwitch, lprcRect);
    FillRect(hdcSwitch, lprcRect, SYSHBR(3DFACE));

    /*
     * Store this "caption" area back into the current switch
     * window data structure.
     */
    InflateRect(lprcRect, -(gcxCaptionFontChar << 1), -(gcyCaptionFontChar));
    lprcRect->top = lprcRect->bottom - gcyCaptionFontChar;

    /*
     *  Draw the sunken edge for showing the task names.
     */
    if ((pswCurrent->fJournaling && _GetKeyState(VK_MENU) >= 0) ||
            (!pswCurrent->fJournaling && _GetAsyncKeyState(VK_MENU) >= 0))
        goto PSWExit;
    CopyInflateRect(&rcRgn, lprcRect, gcxCaptionFontChar >> 1, gcyCaptionFontChar >> 1);
    DrawEdge(hdcSwitch, &rcRgn, EDGE_SUNKEN, BF_RECT);

    /*
     *  Paint the icons
     */
    if ((pswCurrent->fJournaling && _GetKeyState(VK_MENU) >= 0) ||
            (!pswCurrent->fJournaling && _GetAsyncKeyState(VK_MENU) >= 0))
        goto PSWExit;

    xxxPaintIconsInSwitchWindow(pwndSwitch,
                                pswCurrent,
                                hdcSwitch,
                                pswCurrent->iFirstTaskIndex,
                                0,
                                pswCurrent->iTasksShown,
                                FALSE,
                                FALSE,
                                NULL);

    /*
     *  So, just draw the hilite.
     */
    if ((pswCurrent->fJournaling && _GetKeyState(VK_MENU) >= 0) ||
            (!pswCurrent->fJournaling && _GetAsyncKeyState(VK_MENU) >= 0))
        goto PSWExit;

    DrawSwitchWndHilite(pswCurrent,
                        hdcSwitch,
                        pswCurrent->iCurCol,
                        pswCurrent->iCurRow,
                        TRUE);

    /*
     *  Release the switch windows DC
     */
PSWExit:
    _ReleaseDC(hdcSwitch);
}

/***************************************************************************\
* InitSwitchWndInfo()
*
* This function allocs and Initializes all the data structures
* required the build and show the tasks in the system.
* If there is insufficient mem, then this find the next window to switch
* to and returns it. In this case, we will behave as if the end user hit
* ALT+ESC. The SWitchScreen will not comeup in this case.
* If there is only one task in the whole system, then this function
* fails and returns a NULL. (No ALT+TAB processing is required).
* Otherwise, it allocs one SwitchWndInfo struc, fills it up and returns
* the window we gonna switch to.
*
* History:
* 02-Jun-95 BradG       Ported from Win95
\***************************************************************************/

PWND InitSwitchWndInfo(
    PSWINFO *   lppswInfo,
    PWND        pwndCurActive,
    BOOL        fPrev)
{
    PBWL            pbwl;
    INT             iTotalTasks;
    INT             iCols, iRows, iIconsInLastRow;
    INT             iDiff;
    PHWND           phwndLast;
    PSWINFO         pswInfo;
    INT             iIconIndex;
    INT             iCurRow, iCurCol;
    INT             cxSwitch, cySwitch;
    INT             iFirstRowIcons;
    INT             iActiveTask;
    PWND            pwnd = NULL;
    PTHREADINFO     ptiCurrent = PtiCurrent();
    PDESKTOPINFO    pdeskinfo = GETDESKINFO(ptiCurrent);
    PMONITOR        pMonitor = GetPrimaryMonitor();

    /*
     *  Initialize the list
     */
    *lppswInfo = (PSWINFO)NULL;

    /*
     *  Build the Window list of all the top level windows.
     */
#if 0
    if (!(pbwl = BuildHwndList(NULL, BWL_ENUMLIST | BWL_ALLDESKTOPS, NULL)))
        goto ReturnNextWnd;
#else
// BradG - HACK, enumerate on current desktop!
//   For the long run, we will need to enumerate all desktops
//   This will be tricky because we need to check the security of
//   each desktop, thus needing the user's security "token".
    if (!(pbwl = BuildHwndList(pdeskinfo->spwnd->spwndChild, BWL_ENUMLIST, NULL))) {
#ifdef COOLSWITCHTRACE
        DbgPrint("CoolSwitch: BuildHwndList failed (contact bradg).\n");
        UserAssert(pbwl != NULL);
#endif
        goto ReturnNextWnd;
    }
#endif

    /*
     *  Walk down the list and remove all non-task windows from the list.
     *  Replace those hwnds with 0.
     */
    if ((iTotalTasks = _RemoveNonTaskWindows(pbwl, pwndCurActive, &iActiveTask, &phwndLast)) < 2) {
        if (iTotalTasks == 1) {
            /*
             *  If we have only one window and it's in full screen mode, we will
             *  return the shell window so the can switch back to GDI mode.
             */
            pwnd = RevalidateHwnd(pbwl->rghwnd[0]);
            if (pwnd && GetFullScreen(pwnd) == FULLSCREEN && pwndCurActive == pwnd)
                pwnd = pdeskinfo->spwndShell;

        } else {
            pwnd = pdeskinfo->spwndShell;
        }
#ifdef COOLSWITCHTRACE
        DbgPrint("CoolSwitch: Not enough windows to switch.\n");
#endif
        goto FreeAndReturnNextWnd;  // If there isn't even two tasks, no switch wnd processing.
    }

    /*
     *  Allocate the Switch Info structure.  If we don't have enough
     *  memory, act as if we are doing Alt+Esc.
     */
    if (!(pswInfo = (PSWINFO)UserAllocPoolWithQuota(sizeof(SWITCHWNDINFO), TAG_ALTTAB))) {
#ifdef COOLSWITCHTRACE
        DbgPrint("CoolSwitch: UserAllocPool failed on 0x%X bytes (contact bradg).\n", sizeof(SWITCHWNDINFO));
        UserAssert(pswInfo != NULL);
#endif
        goto FreeAndReturnNextWnd;  // Unable to alloc SwitchWndInfo struct.
    }

    pswInfo->pbwl        = pbwl;
    pswInfo->phwndLast   = phwndLast;
    pswInfo->iTasksShown = pswInfo->iTotalTasks = iTotalTasks;

    /*
     *  Get the next/prev window that must become active.
     */
    iIconIndex = NextPrevTaskIndex(pswInfo, iActiveTask, 1, !fPrev);
    pswInfo->phwndCurrent = &(pbwl->rghwnd[iIconIndex]);

    iCols = min(gnFastAltTabColumns, iTotalTasks);
    iRows = iTotalTasks / iCols;  // Truncation might occur.

    iIconsInLastRow = iTotalTasks - iRows * iCols;
    iRows += (iIconsInLastRow ? 1 : 0);  // Take care of earlier truncation.

    /*
     *  Restrict the number of rows to just MAXROWSALLOWED (3)
     */
    if (iRows > gnFastAltTabRows) {
        iRows = gnFastAltTabRows;
        pswInfo->fScroll = TRUE;    // We need to scroll.
        iIconsInLastRow = iCols;
        pswInfo->iTasksShown = iCols * iRows;
    } else {
        pswInfo->fScroll = FALSE;
    }

    pswInfo->iNoOfColumns = iCols;
    pswInfo->iNoOfRows    = iRows;

    if (iIconsInLastRow == 0)
       iIconsInLastRow = pswInfo->iNoOfColumns; // Last Row is full.
    pswInfo->iIconsInLastRow = iIconsInLastRow;

    /*
     *  Find out Row and Col where the next/prev icon will lie.
     */
    if (iIconIndex >= (iRows * iCols)) {
        /*
         *  Next Icon lies outside. Bring it to the center.
         */
        iCurRow = (iRows >> 2) + 1;
        iCurCol = (iCols >> 2) + 1;
        iDiff = (iIconIndex - ((iCurRow * iCols) + iCurCol));
    } else {
        iDiff = 0;
        iCurRow = iIconIndex / iCols;
        iCurCol = iIconIndex - (iCurRow * iCols);
    }

    pswInfo->iFirstTaskIndex = iDiff;
    pswInfo->iCurRow         = iCurRow;
    pswInfo->iCurCol         = iCurCol;

    /*
     *  Calculate the Switch Window Dimensions.
     */
    cxSwitch = min(
            pMonitor->rcMonitor.right - pMonitor->rcMonitor.left,
            gnFastAltTabColumns * CXICONSLOT +
                CXICONSIZE / 2 +
                6 * gpsi->gclBorder * SYSMET(CXBORDER) +
                gcxCaptionFontChar);

    cySwitch = min(
            pMonitor->rcMonitor.bottom - pMonitor->rcMonitor.top,
            iRows * CYICONSLOT +
                CYICONSIZE +
                gcyCaptionFontChar * 2 +
                gcyCaptionFontChar / 2);

    /*
     *  Find the number of icons in first row
     */
    if (iRows == 1) {
        iFirstRowIcons = iIconsInLastRow;
    } else {
        iFirstRowIcons = iCols;
    }

    /*
     *  Center the icons based on the number of icons in first row.
     */
    pswInfo->ptFirstRowStart.x = (cxSwitch - 4*gpsi->gclBorder*SYSMET(CXBORDER) - iFirstRowIcons * CXICONSLOT) >> 1;
    pswInfo->ptFirstRowStart.y = (CYICONSIZE >> 1);

    pswInfo->cxSwitch = cxSwitch;
    pswInfo->cySwitch = cySwitch;

    *lppswInfo = pswInfo;

    return RevalidateHwnd(*(pswInfo->phwndCurrent));  // Success!


    /*
     *  When there is insufficient mem to create the reqd structures, we simply
     *  return the next window. We make the phwndInfo as NULL. So, we won't
     *  attempt to draw the switch window.
     */

FreeAndReturnNextWnd:
    FreeHwndList(pbwl);
ReturnNextWnd:
    if (pwnd)
        return(pwnd);

    return(_GetNextQueueWindow(pwndCurActive, _GetKeyState(VK_SHIFT) < 0, FALSE));
}

/***************************************************************************\
* xxxMoveSwitchWndHilite()
*
* This moves the Hilite to the next/prev icon.
* Checks if this move results in a scrolling. If it does, then
* make sure scroll occurs.
* Else, erase hilite from the current icon;
* Then draw hilite on the new Icon.
* fPrev indicates whether you want the prev task or next.
*
* History:
* 02-Jun-1995 BradG     Ported from Win95
\***************************************************************************/

HWND xxxMoveSwitchWndHilite(
    PWND    pwndSwitch,
    PSWINFO pswInfo,
    BOOL    fPrev)
{
    INT  iCurCol, iCurRow;
    INT  iMaxColumns;
    BOOL fLastRow;
    BOOL fNeedToScroll = FALSE;
    HDC  hdc;

    CheckLock(pwndSwitch);
    UserAssert(IsWinEventNotifyDeferredOK());

    iCurCol = pswInfo->iCurCol;
    iCurRow = pswInfo->iCurRow;

    /*
     *  Find out the new postion (row and column) of hilite.
     */
    if (fPrev) {
        if (iCurCol > 0) {
            /*
             *  Move cursor to prev column on the same row.
             */
            iCurCol--;
        } else {
            /*
             *  Try to move to the previous row.
             */
            if (iCurRow > 0) {
                /*
                 *  Move to the last column on the previous row.
                 */
                iCurRow--;
                iCurCol = pswInfo->iNoOfColumns - 1;
            } else {
                /*
                 *  We are already at (0,0); See if we need to scroll.
                 */
                if (pswInfo->fScroll) {
                    /*
                     * Time to scroll; Scroll by one Row;
                     * Repaint the whole window.
                     */
                    fNeedToScroll = TRUE;
                    pswInfo->iFirstTaskIndex = NextPrevTaskIndex(pswInfo, pswInfo->iFirstTaskIndex,
                      pswInfo->iNoOfColumns, FALSE);
                    iCurCol = pswInfo->iNoOfColumns - 1;
                } else {
                    /*
                     *  Move the hilite to the last icon shown.
                     */
                    iCurRow = pswInfo->iNoOfRows - 1;
                    iCurCol = pswInfo->iIconsInLastRow - 1;
                }
            }
        }

    } else {
        /*
         *  !fPrev
         *  Get the number of columns in the current row.
         */
        if (fLastRow = (iCurRow == (pswInfo->iNoOfRows - 1))) // Are we at the last row?
            iMaxColumns = pswInfo->iIconsInLastRow;
        else
            iMaxColumns = pswInfo->iNoOfColumns;

        /*
         *  Are we at the last column yet?
         */
        if (iCurCol < (iMaxColumns - 1)) {
            /*
             *  No! Move to the right.
             */
            iCurCol++;
        } else {
            /*
             *  We are at the last column.
             *  If we are not at last row, then move to next row.
             */
            if (!fLastRow) {
                iCurCol = 0;
                iCurRow++;
            } else {
                /*
                 *  We are at the last row, last col;
                 *  See if we need to scroll.
                 */
                if (pswInfo->fScroll) {
                    fNeedToScroll = TRUE;
                    pswInfo->iFirstTaskIndex = NextPrevTaskIndex(pswInfo, pswInfo->iFirstTaskIndex,
                          pswInfo->iNoOfColumns, TRUE);
                    iCurCol = 0;
                } else {
                    /*
                     *  Move to the top left corner (0, 0).
                     */
                    iCurRow = iCurCol = 0;
                }
            }
        }
    }

    /*
     *  Move the phwnd to the next/prev
     */
    pswInfo->phwndCurrent = NextPrevPhwnd(pswInfo, pswInfo->phwndCurrent, !fPrev);

    /*
     *  Remove Hilite from the current location.
     */
    hdc = _GetDCEx(pwndSwitch, NULL, DCX_USESTYLE);
    DrawSwitchWndHilite(pswInfo, hdc, pswInfo->iCurCol, pswInfo->iCurRow, FALSE);

    /*
     *  Repaint if needed.
     */
    if (fNeedToScroll)
        xxxPaintIconsInSwitchWindow(pwndSwitch, pswInfo, hdc, pswInfo->iFirstTaskIndex, 0, 0, TRUE, !fPrev, NULL);

    /*
     *  Draw Hilite at the new location.
     */
    DrawSwitchWndHilite(pswInfo, hdc, iCurCol, iCurRow, TRUE);

    _ReleaseDC(hdc);

    pswInfo->iCurRow = iCurRow;
    pswInfo->iCurCol = iCurCol;

    if (FWINABLE()) {
        xxxWindowEvent(EVENT_OBJECT_FOCUS, pwndSwitch, OBJID_CLIENT,
            iCurRow * pswInfo->iNoOfColumns + iCurCol + 1, WEF_USEPWNDTHREAD);
    }

    return (*(pswInfo->phwndCurrent));
}

/***************************************************************************\
* xxxShowSwitchWindow()
*
* Show the switch Window.
* Returns: TRUE if succeeded.   FALSE, if the window was not shown because
* of the pre-mature release of ALT key. The selection has been made already.
*
* History:
* 07-Jun-1995 BradG     Ported from Win95
\***************************************************************************/

BOOL xxxShowSwitchWindow(
        PWND        pwndAltTab)
{
    PSWINFO pswInfo;
    PMONITOR pMonitorSwitch = GetPrimaryMonitor();
    CheckLock(pwndAltTab);
    UserAssert(IsWinEventNotifyDeferredOK());

    /*
     *  Get the switch window information
     */
    pswInfo = Getpswi(pwndAltTab);
    UserAssert(pswInfo != NULL);

    /*
     *  If the key is not down, don't bother to display Switch Window.
     */
    if ((pswInfo->fJournaling && _GetKeyState(VK_MENU) >= 0) ||
            (!pswInfo->fJournaling && _GetAsyncKeyState(VK_MENU) >= 0)) {
#ifdef COOLSWITCHTRACE
        DbgPrint("CoolSwitch: Not displaying window because VM_MENU is up (contact bradg).\n");
#endif
        return FALSE;
    }

    /*
     *  Bring and position the window on top.
     */
    xxxSetWindowPos(pwndAltTab, PWND_TOPMOST, 0,0,0,0,
        SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOREDRAW );

    if (!TestWF(pwndAltTab, WFVISIBLE)) {
        xxxSetWindowPos(
            pwndAltTab,
            PWND_TOPMOST,
            (pMonitorSwitch->rcWork.left + pMonitorSwitch->rcWork.right - pswInfo->cxSwitch) / 2,
            (pMonitorSwitch->rcWork.top + pMonitorSwitch->rcWork.bottom - pswInfo->cySwitch) / 2,
            pswInfo->cxSwitch,
            pswInfo->cySwitch,
            SWP_SHOWWINDOW | SWP_NOACTIVATE);
    }
#ifdef COOLSWITCHTRACE
    UserAssert(TestWF(pwndAltTab, WFVISIBLE));
#endif
    xxxUpdateWindow(pwndAltTab);

    if (FWINABLE()) {
        xxxWindowEvent(EVENT_SYSTEM_SWITCHSTART, pwndAltTab, OBJID_CLIENT,
                0, WEF_USEPWNDTHREAD);
        xxxWindowEvent(EVENT_OBJECT_FOCUS, pwndAltTab, OBJID_CLIENT,
                pswInfo->iCurRow * pswInfo->iNoOfColumns + pswInfo->iCurCol + 1,
                WEF_USEPWNDTHREAD);
    }

    return TRUE;
}

/***************************************************************************\
* SwitchWndCleanup()
*
* Clean up all the mem allocated etc.,
*
* History:
* 07-Jun-1995 BradG     Ported from Win95
\***************************************************************************/

VOID SwitchWndCleanup(
    PSWINFO *ppswInfo)
{
    UserAssert(ppswInfo != NULL);
    UserAssert(*ppswInfo != NULL);

    /*
     *  First of all free the Window list.
     */
    if ((*ppswInfo)->pbwl)
        FreeHwndList((*ppswInfo)->pbwl);
    UserFreePool(*ppswInfo);
    *ppswInfo = NULL;
}


/***************************************************************************\
*
*  xxxSwitchWndProc()
*
\***************************************************************************/

LRESULT xxxSwitchWndProc(
    PWND  pwnd,
    UINT  message,
    WPARAM wParam,
    LPARAM lParam)
{
    TL          tlpwndActivate;
    PTHREADINFO ptiCurrent = PtiCurrent();

    CheckLock(pwnd);
    UserAssert(IsWinEventNotifyDeferredOK());

    VALIDATECLASSANDSIZE(pwnd, message, wParam, lParam, FNID_SWITCH, WM_CREATE);

    switch (message) {
    case WM_CREATE:
        /*
         * When the queue was created, the cursor was set to the wait cursor.
         * We want to use the normal one.
         */
        zzzSetCursor(pwnd->pcls->spcur);
        break;

    case WM_CLOSE:
        /*
         *  Hide this window without activating anyone else.
         */
        xxxSetWindowPos(pwnd, NULL, 0, 0, 0, 0, SWP_HIDEWINDOW |
                SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);

        /*
         * Get us out of Alt+Tab mode.  Since the alttab information
         * is stored in the gptiRit->pq, we will reference that insteatd
         * of the current-thread.
         */
        xxxCancelCoolSwitch();
        break;

    case WM_ERASEBKGND:
    case WM_FULLSCREEN:
        ThreadLockWithPti(ptiCurrent, pwnd, &tlpwndActivate);
        xxxPaintSwitchWindow(pwnd);
        ThreadUnlock(&tlpwndActivate);
        return 0;

    case WM_DESTROY:
        {
            /*
             *  Get the switch window info for this window
             */
            PSWINFO pswCurrent = Getpswi(pwnd);


            if (pswCurrent)
                SwitchWndCleanup(&pswCurrent);
        }
        break;
    }

    return xxxDefWindowProc(pwnd, message, wParam, lParam);
}

/***************************************************************************\
* xxxCancelCoolSwitch
*
* This functions destroys the cool switch window and removed the INALTTAB
* mode flag from the specified queue.
*
* History:
* 18-Sep-1995 BradG     Created
\***************************************************************************/
VOID xxxCancelCoolSwitch(
    void)
{
    CheckCritIn();
    UserAssert(IsWinEventNotifyDeferredOK());

    /*
     *  Destroy the Cool Switch window
     */
    if (gspwndAltTab != NULL) {

        /*
         * Make sure that the thread calling this is the same
         * thread which created the alttab window.  Otherwise,
         * we could end up with this window floating around until
         * the calling process dies.  Remember, we can't destroy
         * windows across different threads.
         */
        if (gspwndAltTab->head.pti != PtiCurrent())
            return;

        if (FWINABLE()) {
            xxxWindowEvent(EVENT_SYSTEM_SWITCHEND, gspwndAltTab, OBJID_CLIENT,
                0, WEF_USEPWNDTHREAD);
        }
        xxxDestroyWindow(gspwndAltTab);

        Lock(&gspwndAltTab, NULL);
    }
}

/***************************************************************************\
* xxxNextWindow
*
* This function does the processing for the alt-tab/esc/F6 UI.
*
* History:
* 30-May-1991 DavidPe       Created.
\***************************************************************************/

VOID xxxNextWindow(
    PQ    pq,
    DWORD wParam)
{
    PWND        pwndActivateNext;
    PWND        pwndCurrentActivate, pwndCurrentTopFocus;
    int         fDir;
    TL          tlpwndActivateNext;
    TL          tlpwndCurrentActivate;
    TL          tlpwndT;
    PSWINFO     pswCurrent;
    ULONG_PTR    dwResult;
    BOOL        fNonRit = FALSE;
    PTHREADINFO ptiCurrent = PtiCurrent();
    PTHREADINFO ptiAltTab;

    UserAssert(!IsWinEventNotifyDeferred());

    /*
     * HACK: We have a problem with direct-draw full apps where an alttab
     *       window is created on a queue owned other than the RIT.  This
     *       shows up by alt-tabbing away from ROIDS.EXE during fullscreen.
     *
     *       What is happening is on a ALT-TAB, from xxxSysCommand(), the
     *       thread is not a rit.  xxxSysCommand() calls xxxOldNextWindow
     *       which finds that the current-thread doesn't have a switch
     *       window, and then creates one on the current-thread-queue.
     *
     *       The hack here is to make sure the calling thread is the RIT
     *       before allowing any cool-switch creation.
     *
     *       21-Mar-1996 : Chriswil
     */
#if 0
    ptiAltTab = ptiCurrent;
#else
    ptiAltTab = gptiRit;
#endif

    if (pq == NULL)
        return;

    fDir = (_GetAsyncKeyState(VK_SHIFT) < 0) ? FDIR_BACKWARD : FDIR_FORWARD;

    /*
     *  NOTE: As of NT 4.0 the slow Alt+Tab functionality now officially acts
     *  like Alt+Esc with the exception that Alt+Tab will activate the window
     *  where Alt+Esc will not.
     */
    switch (wParam) {

    case VK_TAB:

        if (gspwndAltTab == NULL) {

            PWND pwndSwitch;
            TL   tlpSwitchInfo;

            /*
             *  We are entering Alt+Tab for the first time, we need to
             *  initialize the Switch Window structure and if needed
             *  create and display the Alt+Tab window.  We have two special
             *  cases: (1) The user does not want to use the Switch window,
             *  (2) The initialize switch window fails thus we will act
             *  just like slow Alt+Tab
             */

            /*
             * Since Alt+Shift is the default hotkey for keyboard layout switching,
             * Alt+Shift+Tab may cause a KL switching while AltTab window is up.
             * To prevent it, we'd better reset the global toggle key state here,
             * so that xxxScanSysQueue will not confuse when it handles keyup messages.
             */
            gLangToggleKeyState = KLT_NONE;

            /*
             * Mouse buttons sometimes get stuck down due to hardware glitches,
             * usually due to input concentrator switchboxes or faulty serial
             * mouse COM ports, so clear the global button state here just in case,
             * otherwise we may not be able to change focus with the mouse.
             * Also do this in zzzCancelJournalling (Ctr-Esc, Ctrl-Alt-Del, etc.)
             */
#if DBG
            if (gwMouseOwnerButton)
                RIPMSG1(RIP_WARNING,
                        "gwMouseOwnerButton=%x, being forcibly cleared\n",
                        gwMouseOwnerButton);
#endif
            gwMouseOwnerButton = 0;

            /*
             *  Determine the current active window.
             */
            Lock(&gspwndActivate, pq->spwndActive);
            if (gspwndActivate == NULL) {
                Lock(&gspwndActivate, grpdeskRitInput->pDeskInfo->spwnd->spwndChild);
            }

            if (!gspwndActivate) {
                return;
            }

            /*
             *  Make a local copy of gspwndActivate and lock it because xxxFreeWindow will
             *  unlock if it is the window being freed.
             */
            pwndCurrentActivate = gspwndActivate;
            ThreadLockAlwaysWithPti(ptiCurrent, pwndCurrentActivate, &tlpwndCurrentActivate);

            /*
             *   Cancel the active window's mode
             */
            xxxSendMessageTimeout(pwndCurrentActivate, WM_CANCELMODE, 0, 0, SMTO_ABORTIFHUNG, 100, &dwResult);

            /*
             *  Initialize the Switch Window data structure, if we
             *  succeed create and display the window, otherwise act
             *  like slow Alt+Tab.
             */
            pwndActivateNext = InitSwitchWndInfo(&pswCurrent, pwndCurrentActivate, fDir);
            
            ThreadLockWithPti(ptiCurrent, pwndActivateNext, &tlpwndActivateNext);

            if (pswCurrent == NULL) {
                /*
                 *  Couldn't initialize our switch window data structure, so we
                 *  will act like Alt+Esc.
                 */
                goto DoSlowAltTab;
            }

            if (pwndActivateNext == NULL) {
                SwitchWndCleanup(&pswCurrent);
                ThreadUnlock(&tlpwndActivateNext);
                ThreadUnlock(&tlpwndCurrentActivate);
                Unlock(&gspwndActivate);
                return;
            }

            ThreadLockPool(ptiCurrent, pswCurrent, &tlpSwitchInfo);
            
            /*
             * Since we are in the RIT, test the physical state of the keyboard
             */
            pswCurrent->fJournaling = FALSE;

            /*
             *  Create the Alt+Tab window
             */
            pwndSwitch =
                  xxxCreateWindowEx( WS_EX_TOOLWINDOW | WS_EX_WINDOWEDGE | WS_EX_DLGMODALFRAME,
                      (PLARGE_STRING)SWITCHWNDCLASS, NULL,
                      WS_POPUP | WS_BORDER | WS_DISABLED,
                      0, 0, 10, 10, NULL, NULL, NULL, NULL, VER40);

            if (gspwndAltTab != NULL) {
                UserAssert(0);

                _PostMessage(gspwndAltTab, WM_CLOSE, 0, 0);
            }

            Lock(&gspwndAltTab, pwndSwitch);

            ThreadUnlockPool(ptiCurrent, &tlpSwitchInfo);
            
            if (gspwndAltTab == NULL) {
                /*
                 *  Could not create the cool switch window, do the Alt+Esc thing
                 */
#ifdef COOLSWITCHTRACE
                DbgPrint("CoolSwitch: Could not create window (contact bradg).\n");
                UserAssert(gspwndAltTab != NULL);
#endif
                SwitchWndCleanup(&pswCurrent);
                goto DoSlowAltTab;
            }

            /*
             *  Save the pointer to the switch window info structure
             */
            Setpswi(gspwndAltTab, pswCurrent);
            /*
             *  Set gspwndActivate so the RIT knows what window we would like
             *  it to activate.
             */
            Lock(&gspwndActivate, pwndActivateNext);

            /*
             * Make sure that our rit queue has the correct pdesk
             */
            if (ptiCurrent->TIF_flags & TIF_SYSTEMTHREAD) {
                xxxSetThreadDesktop(NULL, grpdeskRitInput); // DeferWinEventNotify() ?? IANJA ??
            }

            /*
             * If we're currently full screen tell console to switch to
             * the desktop to GDI mode; we can't do this on the RIT because
             * it can be slow.
             */
            if (gspwndFullScreen != grpdeskRitInput->pDeskInfo->spwnd) {
                ThreadLockWithPti(ptiCurrent, grpdeskRitInput->pDeskInfo->spwnd, &tlpwndT);
                xxxSendNotifyMessage(grpdeskRitInput->pDeskInfo->spwnd, WM_FULLSCREEN, GDIFULLSCREEN, (LPARAM)HW(grpdeskRitInput->pDeskInfo->spwnd));
                ThreadUnlock(&tlpwndT);
            }

            /*
             *  Show the Alt+Tab window.  If it returns FALSE this means
             *  the ALT key has been released, so there is no need to
             *  paint the icons.
             */
            ThreadLockAlwaysWithPti(ptiCurrent, gspwndAltTab, &tlpwndT);
            xxxShowSwitchWindow(gspwndAltTab);
            ThreadUnlock(&tlpwndT);

            /*
             *  Exit now because the Switch window will have been
             *  already updated.
             */
            ThreadUnlock(&tlpwndActivateNext);
            ThreadUnlock(&tlpwndCurrentActivate);

        } else {

            /*
             *  We come here to do the actual switching and/or updating of
             *  the switch window when in Alt+Tab mode.
             */
            PWND    pwndSwitch;
            TL      tlpwndSwitch;
            PSWINFO pswCurrent;
            HWND    hwndActivateNext;
            HWND    hwndStop;

            if (!(pwndSwitch = gspwndAltTab)) {

                goto DoAltEsc;

            } else {
                /*
                 *  Move the hilight rect to the next/prev task.  It is possible
                 *  that some tasks were destoryed, so we need to skip those.
                 */
                pswCurrent = Getpswi(pwndSwitch);
                ThreadLockAlwaysWithPti(ptiCurrent, pwndSwitch, &tlpwndSwitch);
                hwndStop = NULL;
                do {
                    hwndActivateNext = xxxMoveSwitchWndHilite(pwndSwitch, pswCurrent, fDir);
                    if (!hwndStop) {
                        hwndStop = hwndActivateNext;
                    } else {
                        if (hwndStop == hwndActivateNext) {
                            pwndActivateNext = NULL;
                            break;
                        }
                    }
                    pwndActivateNext = RevalidateHwnd(hwndActivateNext);
                } while (!pwndActivateNext);
                ThreadUnlock(&tlpwndSwitch);
                Lock(&gspwndActivate, pwndActivateNext);
                if (!gspwndActivate) {
                    /*
                     *  No Window to activate, bail out of Alt+Tab mode
                     */
                    xxxCancelCoolSwitch();
                }
            }
        }
        break;

DoAltEsc:
    case VK_ESCAPE:
        /*
         *  NOTE: The RIT doesn't use gspwndActivate to activate the window when
         *        processing Alt+Esc, we just use it here as a convenient
         *        variable.  The actual activation takes place below.
         */
        pwndCurrentActivate = pq->spwndActive;
        if (pwndCurrentActivate == NULL) {
            pwndCurrentActivate = pq->ptiKeyboard->rpdesk->pDeskInfo->spwnd->spwndChild;
        }
        if (!pwndCurrentActivate)
            return;
        ThreadLockAlwaysWithPti(ptiCurrent, pwndCurrentActivate, &tlpwndCurrentActivate);

        /*
         *   Cancel the active window's mode
         */
        xxxSendMessageTimeout(pwndCurrentActivate, WM_CANCELMODE, 0, 0, SMTO_ABORTIFHUNG, 100, &dwResult);

        /*
         * Determine the next window to activate
         */
        pwndActivateNext = _GetNextQueueWindow(pwndCurrentActivate, fDir, TRUE);
        ThreadLockWithPti(ptiCurrent, pwndActivateNext, &tlpwndActivateNext);

        /*
         * If we're going forward through the windows, move the currently
         * active window to the bottom so we'll do the right thing when
         * we go backwards.
         */
        if (pwndActivateNext != pwndCurrentActivate) {
DoSlowAltTab:
            if (pwndActivateNext) {

                /*
                 * We're about to activate another window while the ALT key is down,
                 *  so let the current focus window know that it doesn't need the
                 *  menu underlines anymore
                 */
                pwndCurrentTopFocus = GetTopLevelWindow(pq->spwndFocus);
                if ((pwndCurrentTopFocus != NULL) && (pwndCurrentTopFocus->spmenu != NULL)) {
                    ClearMF(pwndCurrentTopFocus->spmenu, MFUNDERLINE);
                }

                if (fDir == FDIR_FORWARD) {
                    /*
                     * For Alt+ESC only move the window to the bottom if it's
                     * not a top most window
                     */
                    if (!TestWF(pwndCurrentActivate, WEFTOPMOST)) {
                        xxxSetWindowPos(pwndCurrentActivate, PWND_BOTTOM, 0, 0, 0, 0,
                                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE |
                                SWP_DEFERDRAWING | SWP_NOSENDCHANGING |
                                SWP_ASYNCWINDOWPOS);
                    }
                }

                /*
                 * The ALT key is down, so this window needs menu underlines
                 */
                if (pwndActivateNext->spmenu != NULL) {
                    SetMF(pwndActivateNext->spmenu, MFUNDERLINE);
                }


                /*
                 * This little ugly hack will cause xxxSetForegroundWindow2()
                 * to send out an activation messages to a queue that is
                 * already the active queue allowing us to change the active
                 * window on that queue.
                 */
                if (gpqForeground == GETPTI(pwndActivateNext)->pq)
                    gpqForeground = NULL;

                /*
                 * Make the selected window thread the owner of the last input;
                 *  since he's next, he owns the ALT-ESC.
                 */
                glinp.ptiLastWoken = GETPTI(pwndActivateNext);

                xxxSetForegroundWindow2(pwndActivateNext, NULL,
                        (wParam == VK_TAB) ? SFW_SWITCH | SFW_ACTIVATERESTORE : SFW_SWITCH);

                /*
                 * Win3.1 calls SetWindowPos() with activate, which z-orders
                 * first regardless, then activates. Our code relies on
                 * xxxActivateThisWindow() to z-order, and it'll only do
                 * it if the window does not have the child bit set (regardless
                 * that the window is a child of the desktop).
                 *
                 * To be compatible, we'll just force z-order here if the
                 * window has the child bit set. This z-order is asynchronous,
                 * so this'll z-order after the activate event is processed.
                 * That'll allow it to come on top because it'll be foreground
                 * then. (Grammatik has a top level window with the child
                 * bit set that wants to be come the active window).
                 */
                if (wParam == VK_TAB && TestWF(pwndActivateNext, WFCHILD)) {
                    xxxSetWindowPos(pwndActivateNext, (PWND)HWND_TOP, 0, 0, 0, 0,
                            SWP_NOSIZE | SWP_NOMOVE | SWP_ASYNCWINDOWPOS);
                }
            }
        }
        ThreadUnlock(&tlpwndActivateNext);
        ThreadUnlock(&tlpwndCurrentActivate);
        break;

    case VK_F6:
        if ((pwndCurrentActivate = pq->spwndActive) == NULL)
            pwndCurrentActivate = pq->ptiKeyboard->rpdesk->pDeskInfo->spwnd->spwndChild;

        pwndActivateNext = pwndCurrentActivate;

        /*
         * HACK! console sessions are all one thread but we want them
         * to act like different threads so if its a console thread (csrss.exe)
         * then ALT-F6 does nothing just like in Win 3.1
         * Note: we never get called with wParam == VK_F6 anyway. Win NT 3.51
         * doesn't seem to either, but Windows '95 does.  BUG?? (IanJa)
         */
        if (!(GETPTI(pwndActivateNext)->TIF_flags & TIF_CSRSSTHREAD)) {
            /*
             * on a alt-f6, we want to keep the switch within the thread.
             * We may want to rethink this because this will look strange
             * when you alt-f6 on a multi-threaded app we will not rotate
             * through the windows on the different threads.  This works
             * fine on Win 3.x because it is single threaded.
             */
            do {
                pwndActivateNext = NextTopWindow(pq->ptiKeyboard, pwndActivateNext, NULL,
                        fDir ? NTW_PREVIOUS : 0);
            } while( (pwndActivateNext != NULL) &&
                    (GETPTI(pwndActivateNext) != pq->ptiKeyboard));

            if (pwndActivateNext != NULL) {

                if (pwndActivateNext != pwndCurrentActivate) {
                    /*
                     * We're about to activate another window while the ALT key is down,
                     *  so let the current focus window know that it doesn't need the
                     *  menu underlines anymore
                     */
                    pwndCurrentTopFocus = GetTopLevelWindow(pq->spwndFocus);
                    if ((pwndCurrentTopFocus != NULL) && (pwndCurrentTopFocus->spmenu != NULL)) {
                        ClearMF(pwndCurrentTopFocus->spmenu, MFUNDERLINE);
                    }
                    /*
                     * The ALT key is down, so this window needs menu underlines
                     */
                    if (pwndActivateNext->spmenu != NULL) {
                        SetMF(pwndActivateNext->spmenu, MFUNDERLINE);
                    }
                }


                ThreadLockAlwaysWithPti(ptiCurrent, pwndActivateNext, &tlpwndActivateNext);
                xxxSetWindowPos(pwndActivateNext, PWND_BOTTOM, 0, 0, 0, 0,
                        SWP_DEFERDRAWING | SWP_NOSENDCHANGING | SWP_NOCHANGE |
                        SWP_ASYNCWINDOWPOS);
                xxxSetForegroundWindow2(pwndActivateNext, NULL, SFW_SWITCH);
                ThreadUnlock(&tlpwndActivateNext);
            }
        }
        break;
    }
}

/***************************************************************************\
* xxxOldNextWindow
*
* This function does the processing for the alt-tab/esc/F6 UI.
*
* History:
* 03-17-92  DavidPe     Ported from Win 3.1 sources
\***************************************************************************/

VOID xxxOldNextWindow(
    UINT flags)
{
    MSG         msg;
    HWND        hwndSel;
    PWND        pwndNewSel;
    PWND        pwndSel;
    BOOL        fType = 0;
    BOOL        fDrawIcon;
    WORD        vk;
    TL          tlpwndT;
    TL          tlpwndSel;
    TL          tlpwndSwitch;
    PSWINFO     pswCurrent;
    PWND        pwndSwitch;
    HWND        hwndStop;
    HWND        hwndNewSel;
    PTHREADINFO ptiCurrent = PtiCurrent();
    PTHREADINFO ptiAltTab;
    UserAssert(IsWinEventNotifyDeferredOK());

    /*
     * HACK: We have a problem with direct-draw full apps where an alttab
     *       window is created on a queue owned other than the RIT.  This
     *       shows up by alt-tabbing away from ROIDS.EXE during fullscreen.
     *
     *       What is happening is on a ALT-TAB, from xxxSysCommand(), the
     *       thread is not a rit.  xxxSysCommand() calls xxxOldNextWindow
     *       which finds that the current-thread doesn't have a switch
     *       window, and then creates one on the current-thread-queue.
     *
     *       The hack here is to make sure the calling thread is the RIT
     *       before allowing any cool-switch creation.
     *
     *       21-Mar-1996 : Chriswil
     */
#if 0
    ptiAltTab = ptiCurrent;
#else
    ptiAltTab = gptiRit;
#endif

    /*
     * Don't allow entering this routine when we're already in the AltTab
     * mode. The AltTab window may have been created via xxxNextWindow.
     */
    if (gspwndAltTab != NULL)
        return;

    if ((pwndSel = ptiCurrent->pq->spwndActive) == NULL)
        return;

    ThreadLockWithPti(ptiCurrent, pwndSel, &tlpwndSel);
    xxxCapture(ptiCurrent, pwndSel, SCREEN_CAPTURE);

    vk = (WORD)flags;
    msg.wParam = (UINT)flags;

    pwndNewSel = NULL;

    if (vk == VK_TAB) {

        TL tlpSwitchInfo;

        /*
         *  Initialize the Switch window data structures
         */
        pwndNewSel = InitSwitchWndInfo(&pswCurrent,
                                       pwndSel,
                                       _GetKeyState(VK_SHIFT) < 0);

        if (pswCurrent == NULL) {
            /*
             *  We were unable to initialize the data structure used by
             *  the Switch window, so we will act like Alt+Esc.
             */
        } else {

            PWND pwndSwitch;

            /*
             * We are doing a journal playback do use _GetKeyState to
             * test the keyboard
             */
            pswCurrent->fJournaling = TRUE;

            ThreadLockPool(ptiCurrent, pswCurrent, &tlpSwitchInfo);
            
            /*
             *  Attempt to create the Switch Window
             */

            pwndSwitch =
                 xxxCreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_WINDOWEDGE | WS_EX_DLGMODALFRAME,
                                   (PLARGE_STRING)SWITCHWNDCLASS,
                                   NULL,
                                   WS_POPUP | WS_BORDER | WS_DISABLED,
                                   0,
                                   0,
                                   10,
                                   10,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   VER40);

            if (gspwndAltTab != NULL) {
                UserAssert(0);

                _PostMessage(gspwndAltTab, WM_CLOSE, 0, 0);
            }

            ThreadUnlockPool(ptiCurrent, &tlpSwitchInfo);

            Lock(&gspwndAltTab, pwndSwitch);


            if (!(pwndSwitch = gspwndAltTab)) {

                SwitchWndCleanup(&pswCurrent);

            } else {
                /*
                 *  Lock the switch window
                 */
                ThreadLockAlwaysWithPti(ptiCurrent, pwndSwitch, &tlpwndSwitch);

                /*
                 *  Save the switch window info
                 */
                Setpswi(pwndSwitch, pswCurrent);

// Don't we need to switch from full screen mode if needed?
#if 0
                /*
                 * If we're currently full screen tell console to switch to
                 * the desktop to GDI mode; we can't do this on the RIT because
                 * it can be slow.
                 */
                if (gspwndFullScreen != grpdeskRitInput->pDeskInfo->spwnd) {
                    ThreadLockWithPti(pti, grpdeskRitInput->pDeskInfo->spwnd, &tlpwndT);
                    xxxSendNotifyMessage(grpdeskRitInput->pDeskInfo->spwnd, WM_FULLSCREEN, GDIFULLSCREEN, (LONG)HW(grpdeskRitInput->pDeskInfo->spwnd));
                    ThreadUnlock(&tlpwndT);
                }
#endif

                /*
                 *  Show the switch window, this also will paint the window
                 */
                xxxShowSwitchWindow(gspwndAltTab);
                ThreadUnlock(&tlpwndSwitch);
            }
        }

    }

    if (!pwndNewSel)
        goto StartTab;

    pwndSel = pwndNewSel;

    while (TRUE) {

        hwndSel = PtoH(pwndSel);
        /*
         * Wait for a message without getting it out of the queue.
         */
        while (!xxxPeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE | PM_NOYIELD))
            xxxWaitMessage();

        if ((pwndSel = RevalidateHwnd(hwndSel)) == NULL)
            pwndSel = ptiCurrent->pq->spwndActive;

        if (_CallMsgFilter(&msg, MSGF_NEXTWINDOW)) {
            /*
             * Swallow the message if the hook processed it
             */
            xxxPeekMessage(&msg, NULL, msg.message, msg.message, PM_REMOVE);
            continue;
        }

        /*
         * If we are doing Alt+Tab and some other key comes in (other than
         * tab, escape or shift), then bomb out of this loop and leave that
         * key in the queue.
         */
        if ((msg.message == WM_SYSKEYDOWN) && gspwndAltTab != NULL) {

            vk = (WORD)msg.wParam;

            if ((vk != VK_TAB) && (vk != VK_ESCAPE) && (vk != VK_SHIFT)) {
                pwndSel = ptiCurrent->pq->spwndActive;
                fType = 0;
                goto Exit;
            }
        }

        switch (msg.message) {

        case WM_CANCELJOURNAL:
            /*
             *  If journalling was canceled we need to exit our loop and
             *  remove the Alt+Tab window.  We don't want to remove this
             *  meesage because we want the app to know that journalling
             *  was canceled.
             */

            /* > > >  F A L L   T H R O U G H  < < < */

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
            /*
             * If mouse message, cancel and get out of loop.
             */
            pwndSel = ptiCurrent->pq->spwndActive;
            fType = 0;
            goto Exit;

        case WM_KEYUP:
        case WM_KEYDOWN:
        case WM_SYSCHAR:
        case WM_SYSKEYUP:
        case WM_MOUSEMOVE:
            /*
             * Swallow the message
             */
            hwndSel = PtoH(pwndSel);
            xxxPeekMessage(&msg, NULL, msg.message, msg.message, PM_REMOVE);

            if ((pwndSel = RevalidateHwnd(hwndSel)) == NULL)
                pwndSel = ptiCurrent->pq->spwndActive;

            if (msg.message == WM_KEYUP || msg.message == WM_SYSKEYUP) {

                vk = (WORD)msg.wParam;

                /*
                 * If alt-tab up, then exit.
                 */
                if (vk == VK_MENU) {
                    /*
                     * If doing Alt+Esc, wait for up of ESC to get out.
                     */
                    if (gspwndAltTab == NULL)
                        break;

                    fType = 0;
                    goto Exit;

                } else if (vk == VK_ESCAPE || vk == VK_F6) {
                    /*
                     * Get out on up transition of ESC or F6 keys.
                     */
                    if (gspwndAltTab != NULL) {

                        pwndSel = ptiCurrent->pq->spwndActive;
                        fType = 0;

                    } else {

                        fType = ((vk == VK_ESCAPE) ? ALT_ESCAPE : ALT_F6);
                    }

                    goto Exit;
                }

            } else if (msg.message == WM_KEYDOWN) {
                /*
                 *  Exit out loop is a stray key stroke comes through.  In
                 *  particular look for VK_CONTROL.
                 */
                pwndSel = ptiCurrent->pq->spwndActive;
                fType = 0;
                goto Exit;
            }
            break;

        case WM_SYSKEYDOWN:
            vk = (WORD)msg.wParam;

            switch (vk) {

            case VK_SHIFT:
            case VK_TAB:
            case VK_ESCAPE:
            case VK_F6:

                hwndSel = PtoH(pwndSel);
                xxxPeekMessage(&msg, NULL, msg.message, msg.message, PM_REMOVE);

                if ((pwndSel = RevalidateHwnd(hwndSel)) == NULL)
                    pwndSel = ptiCurrent->pq->spwndActive;

                if (!(vk == VK_TAB))
                    break;
StartTab:
                if (vk == VK_ESCAPE) {
                    pwndNewSel = _GetNextQueueWindow(
                            pwndSel,
                            _GetKeyState(VK_SHIFT) < 0,
                            TRUE);

                    if (pwndNewSel == NULL)
                        break;

                    fType = ALT_ESCAPE;
                    pwndSel = pwndNewSel;

                    /*
                     * Wait until ESC goes up to activate new window.
                     */
                    break;
                }
                if (vk == VK_F6) {

                    PWND pwndFirst;
                    PWND pwndSaveSel = pwndSel;

                    /*
                     * Save the first returned window to act as a limit
                     * to the search because NextTopWindow will return NULL
                     * only if pwndSel is the only window that meets its
                     * selection criteria.
                     *
                     * This prevents a hang that can occur in winword or
                     * excel when then Alt-F4-F6 key combination is hit
                     * and unsaved changes exist.
                     */
                    pwndFirst = pwndNewSel = (PWND)NextTopWindow(ptiCurrent, pwndSel, NULL,
                            _GetKeyState(VK_SHIFT) < 0 ? NTW_PREVIOUS : 0);

                    while (TRUE) {

                        /*
                         * If pwndNewSel is NULL, pwndSel is the only candidate.
                         */
                        if (pwndNewSel == NULL)
                            break;

                        pwndSel = pwndNewSel;

                        /*
                         * If the window is on the same thread, wait until
                         * F6 goes up to activate new window.
                         */
                        if (GETPTI(pwndSel) == ptiCurrent)
                            break;

                        pwndNewSel = (PWND)NextTopWindow(ptiCurrent, pwndSel, NULL,
                                _GetKeyState(VK_SHIFT) < 0 ? NTW_PREVIOUS : 0);

                        /*
                         * If we've looped around, use the original window.
                         * Wait until F6 goes up to activate new window.
                         */
                        if (pwndNewSel == pwndFirst) {
                            pwndSel = pwndSaveSel;
                            break;
                        }
                    }
                    break;
                }

                /*
                 * Here for the Alt+Tab case
                 */
                if ((pwndSwitch = gspwndAltTab) != NULL) {
                    pswCurrent = Getpswi(pwndSwitch);
                    ThreadLockWithPti(ptiCurrent, pwndSwitch, &tlpwndSwitch);
                    hwndStop = NULL;
                    do {

                        hwndNewSel = xxxMoveSwitchWndHilite(
                                pwndSwitch,
                                pswCurrent,
                                _GetKeyState(VK_SHIFT) < 0);

                        if (!hwndStop) {
                            hwndStop = hwndNewSel;
                        } else {
                            if (hwndStop == hwndNewSel) {
                                pwndNewSel = NULL;
                                break;
                            }
                        }
                        pwndNewSel = RevalidateHwnd(hwndNewSel);
                    } while (!pwndNewSel);
                    ThreadUnlock(&tlpwndSwitch);
                    pwndSel = pwndNewSel;

                } else {

                    pwndNewSel = _GetNextQueueWindow(
                            pwndSel,
                            _GetKeyState(VK_SHIFT) < 0,
                            FALSE);

                    if (pwndNewSel && pwndNewSel != pwndSel) {

                        if (!TestWF(pwndSel, WEFTOPMOST)) {
                            /*
                             *  Force the old window to the bottom
                             */
                            ThreadLockWithPti(ptiCurrent, pwndSel, &tlpwndT);
                            xxxSetWindowPos(pwndSel,
                                            PWND_BOTTOM,
                                            0,
                                            0,
                                            0,
                                            0,
                                            SWP_NOMOVE             |
                                                SWP_NOSIZE         |
                                                SWP_NOACTIVATE     |
                                                SWP_DEFERDRAWING   |
                                                SWP_NOSENDCHANGING |
                                                SWP_ASYNCWINDOWPOS);
                            ThreadUnlock(&tlpwndT);
                        }

                        pwndSel = pwndNewSel; // Will be revalidated at top of loop
                    }
                }
                break;

            default:
                goto Exit;
            }
            break;

        default:
            hwndSel = PtoH(pwndSel);
            xxxPeekMessage(&msg, NULL, msg.message, msg.message, PM_REMOVE);
            xxxTranslateMessage(&msg, 0);
            xxxDispatchMessage(&msg);

            if ((pwndSel = RevalidateHwnd(hwndSel)) == NULL)
                pwndSel = ptiCurrent->pq->spwndActive;

            break;
        }
    }

Exit:
    xxxReleaseCapture();

    fDrawIcon = (gspwndAltTab != NULL);

    /*
     * If this is an Alt-Escape we also have to send the current window
     * to the bottom.
     */
    if (fType == ALT_ESCAPE) {

        PWND pwndActive;

        if (gpqForeground) {

            pwndActive = gpqForeground->spwndActive;

            if (pwndActive && (pwndActive != pwndSel)) {
                ThreadLockWithPti(ptiCurrent, pwndActive, &tlpwndT);
                xxxSetWindowPos(pwndActive,
                                PWND_BOTTOM,
                                0,
                                0,
                                0,
                                0,
                                SWP_NOMOVE             |
                                    SWP_NOSIZE         |
                                    SWP_NOACTIVATE     |
                                    SWP_DEFERDRAWING   |
                                    SWP_NOSENDCHANGING |
                                    SWP_ASYNCWINDOWPOS);
                ThreadUnlock(&tlpwndT);
            }
        }
    }

    if (pwndSel) {
        ThreadLockWithPti(ptiCurrent, pwndSel, &tlpwndT);
        xxxSetForegroundWindow(pwndSel, FALSE);

        if (TestWF(pwndSel, WFMINIMIZED)) {

            if ((fType == 0) && fDrawIcon)
                _PostMessage(pwndSel, WM_SYSCOMMAND, (UINT)SC_RESTORE, 0);

        }
        ThreadUnlock(&tlpwndT);
    }

    /*
     * destroy the alt-tab window
     */
    xxxCancelCoolSwitch();

    ThreadUnlock(&tlpwndSel);
}

/*****************************************************************************\
*
* GetAltTabInfo()   -   Active Accessibility API for OLEACC
*
* This succeeds if we are currently in alt-tab mode.
*
\*****************************************************************************/
BOOL WINAPI
_GetAltTabInfo(
    int iItem,
    PALTTABINFO pati,
    LPWSTR ccxpwszItemText,
    UINT cchItemText OPTIONAL,
    BOOL bAnsi)
{
    PSWINFO pswCurrent;

    if (!gspwndAltTab || ((pswCurrent = Getpswi(gspwndAltTab)) == NULL)) {
        RIPERR0(ERROR_NOT_FOUND, RIP_WARNING, "no Alt-Tab window");
        return FALSE;
    }

    /*
     * Fill in general information
     */
    pati->cItems = pswCurrent->iTotalTasks;
    pati->cColumns = pswCurrent->iNoOfColumns;
    pati->cRows = pswCurrent->iNoOfRows;

    pati->iColFocus = pswCurrent->iCurCol;
    pati->iRowFocus = pswCurrent->iCurRow;

    pati->cxItem = CXICONSLOT;
    pati->cyItem = CYICONSLOT;
    pati->ptStart = pswCurrent->ptFirstRowStart;

    /*
     * Fill in specific information if asked.
     */
    if (cchItemText && (iItem >= 0)) {
        PWND    pwndCur;

        pwndCur = NULL;

        try {
            if ((iItem < pswCurrent->iTotalTasks) &&
                    (pwndCur = RevalidateHwnd(pswCurrent->pbwl->rghwnd[iItem]))) {
                if (bAnsi) {
                    LPSTR ccxpszItemText = (LPSTR)ccxpwszItemText;
                    ULONG cch;
                    RtlUnicodeToMultiByteN(ccxpszItemText, cchItemText - 1,
                            &cch, pwndCur->strName.Buffer, pwndCur->strName.Length);
                    ccxpszItemText[cch] = '\0';
                } else {
                    TextCopy(&pwndCur->strName, ccxpwszItemText, cchItemText);
                }
            } else {
                // no such item
                NullTerminateString(ccxpwszItemText, bAnsi);
            }
        } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
            return FALSE;
        }
    }

    return TRUE;
}
