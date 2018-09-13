/****************************** Module Header ******************************\
* Module Name: winwhere.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* History:
* 08-Nov-1990 DavidPe   Created.
* 23-Jan-1991 IanJa     Serialization: Handle revalidation added
* 19-Feb-1991 JimA      Added enum access checks
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* LayerHitTest
*
* 9/21/1998        vadimg      created
\***************************************************************************/

__inline BOOL LayerHitTest(PWND pwnd, POINT pt)
{
    ASSERT(TestWF(pwnd, WEFLAYERED));

    if (TestWF(pwnd, WEFTRANSPARENT))
        return FALSE;

    if (!GrePtInSprite(gpDispInfo->hDev, PtoHq(pwnd), pt.x, pt.y))
        return FALSE;

    return TRUE;
}

/***************************************************************************\
* ChildWindowFromPoint (API)
*
* Returns NULL if pt is not in parent's client area at all,
* hwndParent if point is not over any children, and a child window if it is
* over a child.  Will return hidden and disabled windows if they are at the
* given point.
*
* History:
* 19-Nov-1990 DavidPe   Created.
* 19-Feb-1991 JimA      Added enum access check
\***************************************************************************/

PWND _ChildWindowFromPointEx(
    PWND  pwnd,
    POINT pt,
    UINT  uFlags)
{
    if (pwnd != PWNDDESKTOP(pwnd)) {
#ifdef USE_MIRRORING
        if (TestWF(pwnd, WEFLAYOUTRTL)) {
            pt.x = pwnd->rcClient.right - pt.x;
        } else 
#endif
        {
            pt.x += pwnd->rcClient.left;
        }
        pt.y += pwnd->rcClient.top;
    }

    // _ClientToScreen(pwndParent, (LPPOINT)&pt);

    if (PtInRect(&pwnd->rcClient, pt)) {

        PWND pwndChild;

        if (pwnd->hrgnClip != NULL) {
            if (!GrePtInRegion(pwnd->hrgnClip, pt.x, pt.y))
                return NULL;
        }

        if (TestWF(pwnd, WEFLAYERED)) {
            if (!LayerHitTest(pwnd, pt))
                return NULL;
        }
        
        /*
         * Enumerate the children, skipping disabled and invisible ones
         * if so desired.  Still doesn't work for WS_EX_TRANSPARENT windows.
         */
        for (pwndChild = pwnd->spwndChild;
                 pwndChild;
                 pwndChild = pwndChild->spwndNext) {

            /*
             * Skip windows as desired.
             */
            if ((uFlags & CWP_SKIPINVISIBLE) && !TestWF(pwndChild, WFVISIBLE))
                continue;

            if ((uFlags & CWP_SKIPDISABLED) && TestWF(pwndChild, WFDISABLED))
                continue;

            if ((uFlags & CWP_SKIPTRANSPARENT) && TestWF(pwndChild, WEFTRANSPARENT))
                continue;

            if (PtInRect(&pwndChild->rcWindow, pt)) {

                if (pwndChild->hrgnClip != NULL) {
                    if (!GrePtInRegion(pwndChild->hrgnClip, pt.x, pt.y))
                        continue;
                }
                if (TestWF(pwndChild, WEFLAYERED)) {
                    if (!LayerHitTest(pwndChild, pt))
                        continue;
                }
                return(pwndChild);
            }
        }

        return pwnd;
    }

    return NULL;
}

/***************************************************************************\
* xxxWindowFromPoint (API)
*
* History:
* 19-Nov-1990 DavidPe   Created.
* 19-Feb-1991 JimA      Added enum access check
\***************************************************************************/

PWND xxxWindowFromPoint(
    POINT pt)
{
    HWND hwnd;
    PWND pwndT;
    TL   tlpwndT;

    pwndT = _GetDesktopWindow();
    ThreadLock(pwndT, &tlpwndT);
    
    hwnd = xxxWindowHitTest2(pwndT, pt, NULL, WHT_IGNOREDISABLED);

    ThreadUnlock(&tlpwndT);

    return RevalidateHwnd(hwnd);
}

#ifdef REDIRECTION

/***************************************************************************\
* xxxCallSpeedHitTestHook
*
* Call the speed hit test hook to give the opportunity to the hook to fake
* where the mouse pointer is.
*
* 25-Jan-1999 CLupu   Created.
\***************************************************************************/

PWND xxxCallSpeedHitTestHook(POINT* ppt)
{
    PHOOK pHook;
    PWND  pwnd = NULL;

    /*
     * Call the hit test hooks to give them the opportunity to change
     * the coordinates and the hwnd
     */
    if ((pHook = PhkFirstValid(PtiCurrent(), WH_HITTEST)) != NULL) {
        HTHOOKSTRUCT ht;
        BOOL         bAnsiHook;

        ht.pt      = *ppt;
        ht.hwndHit = NULL;

        xxxCallHook2(pHook, HC_ACTION, 0, (LPARAM)&ht, &bAnsiHook);

        if (ht.hwndHit != NULL) {
            
            pwnd = HMValidateHandle(ht.hwndHit, TYPE_WINDOW);

            if (pwnd != NULL) {
                ppt->x = ht.pt.x;
                ppt->y = ht.pt.y;
            }
        }
    }
    return pwnd;
}

#endif // REDIRECTION

/***************************************************************************\
* SpeedHitTest
*
* This routine quickly finds out what top level window this mouse point
* belongs to. Used purely for ownership purposes.
*
* 12-Nov-1992 ScottLu   Created.
\***************************************************************************/

PWND SpeedHitTest(
    PWND   pwndParent,
    POINT  pt)
{
    PWND pwndT;
    PWND pwnd;

    if (pwndParent == NULL)
        return NULL;

    for (pwnd = pwndParent->spwndChild; pwnd != NULL; pwnd = pwnd->spwndNext) {

        /*
         * Are we looking at an hidden window?
         */
        if (!TestWF(pwnd, WFVISIBLE))
            continue;

        /*
         * Are we barking up the wrong tree?
         */
        if (!PtInRect((LPRECT)&pwnd->rcWindow, pt)) {
            continue;
        }

        /*
         * Check to see if in window region (if it has one)
         */
        if (pwnd->hrgnClip != NULL) {
            if (!GrePtInRegion(pwnd->hrgnClip, pt.x, pt.y))
                continue;
        }

        /*
         * Is this a sprite?
         */
        if (TestWF(pwnd, WEFLAYERED)) {
            if (!LayerHitTest(pwnd, pt))
                continue;
        }

#ifdef REDIRECTION
        if (TestWF(pwnd, WEFREDIRECTED)) {
            continue;
        }
#endif // REDIRECTION

        /*
         * Children?
         */
        if ((pwnd->spwndChild != NULL) &&
                PtInRect((LPRECT)&pwnd->rcClient, pt)) {

            pwndT = SpeedHitTest(pwnd, pt);
            if (pwndT != NULL)
                return pwndT;
        }

        return pwnd;
    }

    return pwndParent;
}

/***************************************************************************\
* xxxWindowHitTest
*
* History:
* 08-Nov-1990 DavidPe   Ported.
* 28-Nov-1990 DavidPe   Add pwndTransparent support for HTTRANSPARENT.
* 25-Jan-1991 IanJa     change PWNDPOS parameter to int *
* 19-Feb-1991 JimA      Added enum access check
* 02-Nov-1992 ScottLu   Removed pwndTransparent.
* 12-Nov-1992 ScottLu   Took out fSendHitTest, fixed locking bug
\***************************************************************************/

HWND xxxWindowHitTest(
    PWND  pwnd,
    POINT pt,
    int   *piPos,
    DWORD dwHitTestFlags)
{
    HWND hwndT;
    TL   tlpwnd;

    CheckLock(pwnd);

    hwndT = NULL;
    ThreadLockNever(&tlpwnd);
    while (pwnd != NULL) {
        ThreadLockExchangeAlways(pwnd, &tlpwnd);
        hwndT = xxxWindowHitTest2(pwnd, pt, piPos, dwHitTestFlags);
        if (hwndT != NULL)
            break;

        pwnd = pwnd->spwndNext;
    }

    ThreadUnlock(&tlpwnd);
    return hwndT;
}

/***************************************************************************\
* xxxWindowHitTest2
*
* When this routine is entered, all windows must be locked.  When this
* routine returns a window handle, it locks that window handle and unlocks
* all windows.  If this routine returns NULL, all windows are still locked.
* Ignores disabled and hidden windows.
*
* History:
* 08-Nov-1990 DavidPe   Ported.
* 25-Jan-1991 IanJa     change PWNDPOS parameter to int *
* 19-Feb-1991 JimA      Added enum access check
* 12-Nov-1992 ScottLu   Took out fSendHitTest
\***************************************************************************/

HWND xxxWindowHitTest2(
    PWND  pwnd,
    POINT pt,
    int   *piPos,
    DWORD dwHitTestFlags)
{
    int  ht = HTERROR, htGrip=HTBOTTOMRIGHT;
    HWND hwndT;
    TL   tlpwndChild;

    CheckLock(pwnd);

    /*
     * Are we at the bottom of the window chain?
     */
    if (pwnd == NULL)
        return NULL;

    /*
     * Are we looking at an hidden window?
     */
    if (!TestWF(pwnd, WFVISIBLE))
        return NULL;

    /*
     * Are we barking up the wrong tree?
     */
    if (!PtInRect((LPRECT)&pwnd->rcWindow, pt)) {
        return NULL;
    }

    if (pwnd->hrgnClip != NULL) {
        if (!GrePtInRegion(pwnd->hrgnClip, pt.x, pt.y))
            return(NULL);
    }
    
    if (TestWF(pwnd, WEFLAYERED)) {
        if (!LayerHitTest(pwnd, pt))
            return NULL;
    }
    
#ifdef REDIRECTION
    /*
     * If this is called when the layered window is actually trying
     * to process the message then let it see the hit test
     */
    if (TestWF(pwnd, WEFREDIRECTED) && PpiCurrent() != GETPTI(pwnd)->ppi) {
        return NULL;
    }
#endif // REDIRECTION

    /*
     * Are we looking at an disabled window?
     */
    if (TestWF(pwnd, WFDISABLED) && (dwHitTestFlags & WHT_IGNOREDISABLED)) {
        if (TestwndChild(pwnd)) {
            return NULL;
        } else {
            ht = HTERROR;
            goto Exit;
        }
    }

#ifdef SYSMODALWINDOWS
    /*
     * If SysModal window present and we're not in it, return an error.
     * Be sure to assign the point to the SysModal window, so the message
     * will be sure to be removed from the queue.
     */
    if (!CheckPwndFilter(pwnd, gspwndSysModal)) {
        pwnd = gspwndSysModal;

        /*
         * Fix notorious stack overflow bug (some WINABLE fix from Memphis)
         */
        ht = HTCLIENT;
        goto Exit;
    }
#endif

    /*
     * Are we on a minimized window?
     */
    if (!TestWF(pwnd, WFMINIMIZED)) {
        /*
         * Are we in the window's client area?
         */
        if (PtInRect((LPRECT)&pwnd->rcClient, pt)) {
            /*
             * Recurse through the children.
             */
            ThreadLock(pwnd->spwndChild, &tlpwndChild);
            hwndT = xxxWindowHitTest(pwnd->spwndChild,
                                     pt,
                                     piPos,
                                     dwHitTestFlags);
            
            ThreadUnlock(&tlpwndChild);
            if (hwndT != NULL)
                return hwndT;
        }

    }

    /*
     * If window not in same task, don't send WM_NCHITTEST.
     */
    if (GETPTI(pwnd) != PtiCurrent()) {
        ht = HTCLIENT;
        goto Exit;
    }

    /*
     * Send the message.
     */
    ht = (int)xxxSendMessage(pwnd, WM_NCHITTEST, 0, MAKELONG(pt.x, pt.y));

    /*
     * If window is transparent keep enumerating.
     */
    if (ht == HTTRANSPARENT) {
        return NULL;
    }

Exit:

    /*
     * Set wndpos accordingly.
     */
    if (piPos) {
        *piPos = ht;
    }

    /*
     * If this is a RTL mirrored window, then the grip is at
     * HTBOTTOMLEFT (in terms of screen coordinates since they are
     * not RTL mirrored).
     */
    if (TestWF(pwnd, WEFLAYOUTRTL)) {
        htGrip = HTBOTTOMLEFT;
    }

    /*
     * if the click is in the sizebox of the window and this window itself is
     * not sizable, return the window that will be sized by this sizebox
     */
    if ((ht == htGrip) && !TestWF(pwnd, WFSIZEBOX)) {

        PWND  pwndT;
         /*
          * SizeBoxHwnd() can return NULL!  We don't want to act like this
          * is transparent if the sizebox isn't a grip
          */
         pwnd = (pwndT = SizeBoxHwnd(pwnd)) ? pwndT : pwnd;
    }

    return HWq(pwnd);
}
