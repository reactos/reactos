/****************************** Module Header ******************************\
* Module Name: dlgmgrc.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains client side dialog functionality
*
* History:
* 15-Dec-1993 JohnC      Pulled functions from user\server.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************\
* UT_PrevGroupItem
*
* History:
\***************************************************************************/

PWND UT_PrevGroupItem(
    PWND pwndDlg,
    PWND pwndCurrent)
{
    PWND pwnd, pwndPrev;

    if (pwndCurrent == NULL || !TestWF(pwndCurrent, WFGROUP))
        return _PrevControl(pwndDlg, pwndCurrent, CWP_SKIPINVISIBLE | CWP_SKIPDISABLED);

    pwndPrev = pwndCurrent;

    while (TRUE) {
        pwnd = _NextControl(pwndDlg, pwndPrev, CWP_SKIPINVISIBLE | CWP_SKIPDISABLED);

        if (TestWF(pwnd, WFGROUP) || pwnd == pwndCurrent)
            return pwndPrev;

        pwndPrev = pwnd;
    }
}


/***************************************************************************\
* UT_NextGroupItem
*
* History:
\***************************************************************************/

PWND UT_NextGroupItem(
    PWND pwndDlg,
    PWND pwndCurrent)
{
    PWND pwnd, pwndNext;

    pwnd = _NextControl(pwndDlg, pwndCurrent, CWP_SKIPINVISIBLE | CWP_SKIPDISABLED);

    if (pwndCurrent == NULL || !TestWF(pwnd, WFGROUP))
        return pwnd;

    pwndNext = pwndCurrent;

    while (!TestWF(pwndNext, WFGROUP)) {
        pwnd = _PrevControl(pwndDlg, pwndNext, CWP_SKIPINVISIBLE | CWP_SKIPDISABLED);
        if (pwnd == pwndCurrent)
            return pwndNext;
        pwndNext = pwnd;
    }

    return pwndNext;
}

/***************************************************************************\
* _PrevControl
*
* History:
\***************************************************************************/
PWND _PrevControl(
    PWND pwndRoot,
    PWND pwndStart,
    UINT uFlags)
{
    BOOL fFirstFound;
    PWND pwndNext;
    PWND pwnd, pwndFirst;

    if (!pwndStart)
        return(NULL);

    UserAssert(pwndRoot != pwndStart);
    UserAssert(!TestWF(pwndStart, WEFCONTROLPARENT));

    pwnd = _NextControl(pwndRoot, NULL, uFlags);

    pwndFirst = pwnd;
    fFirstFound = FALSE;
    while (pwndNext = _NextControl(pwndRoot, pwnd, uFlags)) {

        if (pwndNext == pwndStart)
            break;

        if (pwndNext == pwndFirst) {
            if (fFirstFound) {
                RIPMSG0(RIP_WARNING, "_PrevControl: Loop Detected");
                break;
            } else {
                fFirstFound = TRUE;
            }
        }

        pwnd = pwndNext;
    }

    return pwnd;
}
/***************************************************************************\
*
*  GetChildControl()
*
*  Gets valid ancestor of given window.
*  A valid dialog control is a direct descendant of a "form" control.
*
\***************************************************************************/

PWND  _GetChildControl(PWND pwndRoot, PWND pwndChild) {
    PWND    pwndControl = NULL;

    while (pwndChild && TestwndChild(pwndChild) && (pwndChild != pwndRoot)) {
        pwndControl = pwndChild;
        pwndChild = REBASEPWND(pwndChild, spwndParent);

        if (TestWF(pwndChild, WEFCONTROLPARENT))
            break;
    }

    return(pwndControl);
}

/***************************************************************************\
*
*  _NextSibblingOrAncestor
*
* Called by _NextControl. It returns the next control to pwndStart. If there
* is a next window (pwndStart->spwndNext), then that is it.
* Otherwise, the next control is up the parent chain. However, if it's already
* at the top of the chain (pwndRoot == pwndStart->spwndParent), then the next
* control is the first child of pwndRoot. But if it's not at the top of the chain,
* then the next control is pwndStart->spwndParent or an ancestor.
*
\***************************************************************************/
PWND _NextSibblingOrAncestor (PWND pwndRoot, PWND pwndStart)
{
    PWND pwndParent;
#if DBG
    PWND pwndNext;
#endif

    // If there is a sibbling, go for it
    if (pwndStart->spwndNext != NULL) {
        return (REBASEALWAYS(pwndStart, spwndNext));
    }

    // If it cannot go up the parent chain, then return the first sibbling.
    pwndParent = REBASEALWAYS(pwndStart, spwndParent);
    if (pwndParent == pwndRoot) {
        // Note that if pwndStart doesn't have any sibblings,
        //  this will return pwndStart again
        return (REBASEALWAYS(pwndParent, spwndChild));
    }


    // Otherwise walk up the parent chain looking for the first window with
    // a WS_EX_CONTROLPARENT parent.

#if DBG
    pwndNext =
#else
    return
#endif
        _GetChildControl(pwndRoot, pwndParent);

#if DBG
    if ((pwndNext != pwndParent) || !TestWF(pwndParent, WEFCONTROLPARENT)) {
        // Code looping through the controls in a dialog might go into an infinite
        //  loop because of this (i.e., xxxRemoveDefaultButton, _GetNextDlgTabItem,..)
        // We've walked up the parent chain but will never walk down the child chain again
        //  because there is a NON WS_EX_CONTROLPARENT parent window somewhere in the chain.
        RIPMSG0 (RIP_ERROR, "_NextSibblingOrAncestor: Non WS_EX_CONTROLPARENT window in parent chain");
    }
    return pwndNext;
#endif
}
/***************************************************************************\
*
*  _NextControl()
*
* It searches for the next NON WS_EX_CONTROLPARENT control following pwndStart.
* If pwndStart is NULL, the search begins with pwndRoot's first child;
* otherwise, it starts with the control next to pwndStart.
* This is a depth-first search that can start anywhere in the window tree.
* uFlags determine what WS_EX_CONTROLPARENT windows should be skipped or recursed into.
* If skipping a window, the search moves to the next control (see _NextSibblingOrAncestor);
* otherwise, the search walks down the child chain (recursive call).
* If the search fails, it returns pwndRoot.
*
\***************************************************************************/
PWND _NextControl(
    PWND pwndRoot,
    PWND pwndStart,
    UINT uFlags)
{
    BOOL fSkip, fAncestor;
    PWND pwndLast, pwndSibblingLoop;
    /* Bug 272874 - joejo
     *
     * Stop infinite loop by only looping a finite number of times and
     * then bailing.
     */
    int nLoopCount = 0;
    
    UserAssert (pwndRoot != NULL);

    if (pwndStart == NULL) {
        // Start with pwndRoot's first child
        pwndStart = REBASEPWND(pwndRoot, spwndChild);
        pwndLast = pwndStart;
        fAncestor = FALSE;
    } else {
        UserAssert ((pwndRoot != pwndStart) && _IsDescendant(pwndRoot, pwndStart));

        // Save starting handle and get next one
        pwndLast = pwndStart;
        pwndSibblingLoop = pwndStart;
        fAncestor = TRUE;
        goto TryNextOne;
    }


    // If no more controls, game over
    if (pwndStart == NULL) {
        return pwndRoot;
    }

    // Search for a non WS_EX_CONTROLPARENT window; if a window should be skipped,
    // try its spwndNext; otherwise, walk down its child chain.
    pwndSibblingLoop = pwndStart;
    do {
        
        //If not WS_EX_CONTROLPARENT parent, done.
        if (!TestWF(pwndStart, WEFCONTROLPARENT)) {
            return pwndStart;
        }

        // Do they want to skip this window?
        fSkip = ((uFlags & CWP_SKIPINVISIBLE) && !TestWF(pwndStart, WFVISIBLE))
                || ((uFlags & CWP_SKIPDISABLED) && TestWF(pwndStart, WFDISABLED));


        // Remember the current window
        pwndLast = pwndStart;

        // Walk down child chain?
        if (!fSkip && !fAncestor) {
            pwndStart = _NextControl (pwndStart, NULL, uFlags);
            // If it found one, done.
            if (pwndStart != pwndLast) {
                return pwndStart;
            }
        }

TryNextOne:
        // Try the next one.
        pwndStart = _NextSibblingOrAncestor (pwndRoot, pwndStart);
        if (pwndStart == NULL) {
            break;
        }

        // If parents are the same, we are still in the same sibbling chain
        if (pwndLast->spwndParent == pwndStart->spwndParent) {
            // If we had just moved up the parent chain last time around,
            //  mark this as the beginning of the new sibbling chain.
            // Otherwise, check if we've looped through all sibblings already.
            if (fAncestor) {
                // Beggining of new sibbling chain.
                pwndSibblingLoop = pwndStart;
            } else if (pwndStart == pwndSibblingLoop) {
                // Already visited all sibblings, so done.
                break;
            }
            fAncestor = FALSE;
        } else {
            // We must have moved up the parent chain, so don't
            //  walk down the child chain right away (try the next window first)
            // Eventhough we are on a new sibbling chain, we don't update
            // pwndSibblingLoop yet; this is because we must walk down this
            // child chain again to make sure we visit all the descendents
            fAncestor = TRUE;
        }

    /* Bug 272874 - joejo
     *
     * Stop infinite loop by only looping a finite number of times and
     * then bailing.
     */
    } while (nLoopCount++ < 256 * 4);

    // It couldn't find one...
    return pwndRoot;
}

/***************************************************************************\
* GetNextDlgTabItem
*
* History:
* 19-Feb-1991 JimA      Added access check
\***************************************************************************/

HWND WINAPI GetNextDlgTabItem(
    HWND hwndDlg,
    HWND hwnd,
    BOOL fPrev)
{

    PWND pwnd;
    PWND pwndDlg;
    PWND pwndNext;

    pwndDlg = ValidateHwnd(hwndDlg);

    if (pwndDlg == NULL)
        return NULL;

    if (hwnd != (HWND)0) {
        pwnd = ValidateHwnd(hwnd);

        if (pwnd == NULL)
            return NULL;

    } else {
        pwnd = (PWND)NULL;
    }

    pwndNext = _GetNextDlgTabItem(pwndDlg, pwnd, fPrev);

    return (HW(pwndNext));
}

PWND _GetNextDlgTabItem(
    PWND pwndDlg,
    PWND pwnd,
    BOOL fPrev)
{
    PWND pwndSave;

    if (pwnd == pwndDlg)
        pwnd = NULL;
    else
    {
        pwnd = _GetChildControl(pwndDlg, pwnd);
        if (pwnd && !_IsDescendant(pwndDlg, pwnd))
            return(NULL);
    }

    //
    // BACKWARD COMPATIBILITY
    //
    // Note that the result when there are no tabstops of
    // IGetNextDlgTabItem(pwndDlg, NULL, FALSE) was the last item, now
    // will be the first item.  We could put a check for fRecurse here
    // and do the old thing if not set.
    //

    // We are going to bug out if we hit the first child a second time.

    pwndSave = pwnd;

    pwnd = (fPrev ? _PrevControl(pwndDlg, pwnd, CWP_SKIPINVISIBLE | CWP_SKIPDISABLED) :
                    _NextControl(pwndDlg, pwnd, CWP_SKIPINVISIBLE | CWP_SKIPDISABLED));

    if (!pwnd)
        goto AllOver;

    while ((pwnd != pwndSave) && (pwnd != pwndDlg)) {
        UserAssert(pwnd);

        if (!pwndSave)
            pwndSave = pwnd;

        if ((pwnd->style & (WS_TABSTOP | WS_VISIBLE | WS_DISABLED))  == (WS_TABSTOP | WS_VISIBLE))
            // Found it.
            break;

        pwnd = (fPrev ? _PrevControl(pwndDlg, pwnd, CWP_SKIPINVISIBLE | CWP_SKIPDISABLED) :
                        _NextControl(pwndDlg, pwnd, CWP_SKIPINVISIBLE | CWP_SKIPDISABLED));
    }

AllOver:
    return pwnd;
}

/***************************************************************************\
*
*  _GetNextDlgGroupItem()
*
\***************************************************************************/

HWND GetNextDlgGroupItem(
    HWND hwndDlg,
    HWND hwndCtl,
    BOOL bPrevious)
{
    PWND pwndDlg;
    PWND pwndCtl;
    PWND pwndNext;

    pwndDlg = ValidateHwnd(hwndDlg);

    if (pwndDlg == NULL)
        return 0;


    if (hwndCtl != (HWND)0) {
        pwndCtl = ValidateHwnd(hwndCtl);

        if (pwndCtl == NULL)
            return 0;
    } else {
        pwndCtl = (PWND)NULL;
    }

    if (pwndCtl == pwndDlg)
        pwndCtl = pwndDlg;

    pwndNext = _GetNextDlgGroupItem(pwndDlg, pwndCtl, bPrevious);

    return (HW(pwndNext));
}

PWND _GetNextDlgGroupItem(
    PWND pwndDlg,
    PWND pwnd,
    BOOL fPrev)
{
    PWND pwndCurrent;
    BOOL fOnceAround = FALSE;

    pwnd = pwndCurrent = _GetChildControl(pwndDlg, pwnd);

    do {
        pwnd = (fPrev ? UT_PrevGroupItem(pwndDlg, pwnd) :
                        UT_NextGroupItem(pwndDlg, pwnd));

        if (pwnd == pwndCurrent)
            fOnceAround = TRUE;

        if (!pwndCurrent)
            pwndCurrent = pwnd;
    }
    while (!fOnceAround && ((TestWF(pwnd, WFDISABLED) || !TestWF(pwnd, WFVISIBLE))));

    return pwnd;
}
