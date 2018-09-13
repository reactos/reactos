/****************************** Module Header ******************************\
* Module Name: enumwin.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Contains the EnumWindows API, BuildHwndList and related functions.
*
* History:
* 10-20-90 darrinm      Created.
* ??-??-?? ianja        Added Revalidation code
* 02-19-91 JimA         Added enum access checks
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

PBWL pbwlCache;

#if DBG
PBWL pbwlCachePrev;
#endif

PBWL InternalBuildHwndList(PBWL pbwl, PWND pwnd, UINT flags);
PBWL InternalBuildHwndOwnerList(PBWL pbwl, PWND pwndStart, PWND pwndOwner);
#ifdef FE_IME
PBWL InternalRebuildHwndListForIMEClass(PBWL pbwl, BOOL fRemoveChild);
PWND InternalGetIMEOwner(HWND hwnd, BOOL fRetIMEWnd);
#endif


/***************************************************************************\
* xxxInternalEnumWindow
*
* History:
* 10-20-90 darrinm      Ported from Win 3.0 sources.
* 02-06-91 IanJa        rename: the call to lpfn can leave the critsect.
* 02-19-91 JimA         Added enum access check
\***************************************************************************/

BOOL xxxInternalEnumWindow(
    PWND pwndNext,
    WNDENUMPROC_PWND lpfn,
    LPARAM lParam,
    UINT flags)
{
    HWND *phwnd;
    PWND pwnd;
    PBWL pbwl;
    BOOL fSuccess;
    TL tlpwnd;

    CheckLock(pwndNext);

    if ((pbwl = BuildHwndList(pwndNext, flags, NULL)) == NULL)
        return FALSE;

    fSuccess = TRUE;
    for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {

        /*
         * Lock the window before we pass it off to the app.
         */
        if ((pwnd = RevalidateHwnd(*phwnd)) != NULL) {

            /*
             * Call the application.
             */
            ThreadLockAlways(pwnd, &tlpwnd);
            fSuccess = (*lpfn)(pwnd, lParam);
            ThreadUnlock(&tlpwnd);
            if (!fSuccess)
                break;
        }
    }

    FreeHwndList(pbwl);

    return fSuccess;
}


/***************************************************************************\
* BuildHwndList
*
* History:
* 10-20-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

#define CHWND_BWLCREATE 32

PBWL BuildHwndList(
    PWND pwnd,
    UINT flags,
    PTHREADINFO pti)
{
    PBWL pbwl;

    CheckCritIn();

    if ((pbwl = pbwlCache) != NULL) {

        /*
         * We're using the cache now; zero it out.
         */
#if DBG
        pbwlCachePrev = pbwlCache;
#endif
        pbwlCache = NULL;

#if DBG
        {
            PBWL pbwlT;
            /*
             * pbwlCache shouldn't be in the global linked list.
             */
            for (pbwlT = gpbwlList; pbwlT != NULL; pbwlT = pbwlT->pbwlNext) {
                UserAssert(pbwlT != pbwl);
            }
        }
#endif
    } else {

        /*
         * sizeof(BWL) includes the first element of array.
         */
        pbwl = (PBWL)UserAllocPool(sizeof(BWL) + sizeof(PWND) * CHWND_BWLCREATE,
                TAG_WINDOWLIST);
        if (pbwl == NULL)
            return NULL;

        pbwl->phwndMax = &pbwl->rghwnd[CHWND_BWLCREATE - 1];
    }
    pbwl->phwndNext = pbwl->rghwnd;

    /*
     * We'll use ptiOwner as temporary storage for the thread we're
     * scanning for. It will get reset to the proper thing at the bottom
     * of this routine.
     */
    pbwl->ptiOwner = pti;

#ifdef OWNERLIST
    if (flags & BWL_ENUMOWNERLIST) {
        pbwl = InternalBuildHwndOwnerList(pbwl, pwnd, NULL);
    } else {
        pbwl = InternalBuildHwndList(pbwl, pwnd, flags);
    }
#else
    pbwl = InternalBuildHwndList(pbwl, pwnd, flags);
#endif

    /*
     * If phwndNext == phwndMax, it indicates that the pbwl has failed to expand.
     * The list is no longer valid, so we should just bail.
     */
    if (pbwl->phwndNext >= pbwl->phwndMax) {
        UserAssert(pbwl->phwndNext == pbwl->phwndMax);
        /*
         * Even if we had picked pbwl from the global single cache (pbwlCache),
         * it should have already been unlinked from the global link list when it was put in the cache.
         * So we should just free it without manupilating the link pointers.
         * If we have allocated the pwbl for ourselves, we can simply free it.
         * In both cases, we should just call UserFreePool().
         * As the side effect, it may make some room by providing a free pool block.
         */
        UserFreePool(pbwl);
        return NULL;
    }

    /*
     * Stick in the terminator.
     */
    *pbwl->phwndNext = (HWND)1;

#ifdef FE_IME
    if (flags & BWL_ENUMIMELAST) {
        UserAssert(IS_IME_ENABLED());
        /*
         * For IME windows.
         * Rebuild window list for EnumWindows API. Because ACCESS 2.0 assumes
         * the first window that is called CallBack Functions in the task is
         * Q-Card Wnd. We should change the order of IME windows
         */
        pbwl = InternalRebuildHwndListForIMEClass(pbwl,
                    (flags & BWL_REMOVEIMECHILD) == BWL_REMOVEIMECHILD);
    }
#endif

    /*
     * Finally link this guy into the list.
     */
    pbwl->ptiOwner = PtiCurrent();
    pbwl->pbwlNext = gpbwlList;
    gpbwlList = pbwl;


    /*
     * We should have given out the cache if it was available
     */
    UserAssert(pbwlCache == NULL);

    return pbwl;
}

/***************************************************************************\
* ExpandWindowList
*
* This routine expands a window list.
*
* 01-16-92 ScottLu      Created.
\***************************************************************************/

BOOL ExpandWindowList(
    PBWL *ppbwl)
{
    PBWL pbwl;
    PBWL pbwlT;
    HWND *phwnd;

    pbwl = *ppbwl;
    phwnd = pbwl->phwndNext;

    /*
     * Map phwnd to an offset.
     */
    phwnd = (HWND *)((BYTE *)phwnd - (BYTE *)pbwl);

    /*
     * Increase size of BWL by 8 slots.  (8 + 1) is
     * added since phwnd is "sizeof(HWND)" less
     * than actual size of handle.
     */
    pbwlT = (PBWL)UserReAllocPool((HANDLE)pbwl,
            PtrToUlong(phwnd) + sizeof(PWND),
            PtrToUlong(phwnd) + (BWL_CHWNDMORE + 1) * sizeof(PWND),
            TAG_WINDOWLIST);

    /*
     * Did alloc succeed?
     */
    if (pbwlT != NULL)
        pbwl = pbwlT;                 /* Yes, use new block. */

    /*
     * Map phwnd back into a pointer.
     */
    phwnd = (HWND *)((ULONG_PTR)pbwl + (ULONG_PTR)phwnd);

    /*
     * Did ReAlloc() fail?
     */
    if (pbwlT == NULL) {
        RIPMSG0(RIP_WARNING, "ExpandWindowList: out of memory.");
        return FALSE;
    }

    /*
     * Reset phwndMax.
     */
    pbwl->phwndNext = phwnd;
    pbwl->phwndMax = phwnd + BWL_CHWNDMORE;

    *ppbwl = pbwl;

    return TRUE;
}

#ifdef OWNERLIST

/***************************************************************************\
* InternalBuildHwndOwnerList
*
* Builds an hwnd list sorted by owner. Ownees go first. Shutdown uses this for
* WM_CLOSE messages.
*
* 01-16-93 ScottLu      Created.
\***************************************************************************/

PBWL InternalBuildHwndOwnerList(
    PBWL pbwl,
    PWND pwndStart,
    PWND pwndOwner)
{
    PWND pwndT;

    /*
     * Put ownees first in the list.
     */
    for (pwndT = pwndStart; pwndT != NULL; pwndT = pwndT->spwndNext) {

        /*
         * Not the ownee we're looking for? Continue.
         */
        if (pwndT->spwndOwner != pwndOwner)
            continue;

        /*
         * Only top level windows that have system menus (the ones that can
         * receive a WM_CLOSE message).
         */
        if (!TestWF(pwndT, WFSYSMENU))
            continue;

        /*
         * Add it and its ownees to our list.
         */
        pbwl = InternalBuildHwndOwnerList(pbwl, pwndStart, pwndT);

        /*
         * If ExpandWindowList() failed in recursive calls,
         * just bail here.
         */
        if (pbwl->phwndNext >= pbwl->phwndMax) {
            UserAssert(pbwl->phwndNext == pbwl->phwndMax);
            return pbwl;
        }
        UserAssert(pbwl->phwndNext < pbwl->phwndMax);
    }

    /*
     * Finally add this owner to our list.
     */
    if (pwndOwner != NULL) {
        UserAssert(pbwl->phwndNext < pbwl->phwndMax);
        *pbwl->phwndNext = HWq(pwndOwner);
        pbwl->phwndNext++;
        if (pbwl->phwndNext == pbwl->phwndMax) {
            if (!ExpandWindowList(&pbwl))
                return pbwl;
        }
    }

    return pbwl;
}

#endif

/***************************************************************************\
* InternalBuildHwndList
*
* History:
* 10-20-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

#define BWLGROW 8

PBWL InternalBuildHwndList(
    PBWL pbwl,
    PWND pwnd,
    UINT flags)
{
    /*
     * NOTE: pbwl->phwndNext is used as a place to keep
     *       the phwnd across calls to InternalBuildHwndList().
     *       This is OK since we don't link pbwl into the list
     *       of pbwl's until after we've finished enumerating windows.
     */

    while (pwnd != NULL) {
        /*
         * Make sure it matches the thread id, if there is one.
         */
        if (pbwl->ptiOwner == NULL || pbwl->ptiOwner == GETPTI(pwnd)) {
            UserAssert(pbwl->phwndNext < pbwl->phwndMax);
            *pbwl->phwndNext = HWq(pwnd);
            pbwl->phwndNext++;
            if (pbwl->phwndNext == pbwl->phwndMax) {
#if EMULATE_EXPAND_FAILURE
                static int n = 0;
                if (++n % 32 == 0) {
                    RIPMSG0(RIP_WARNING, "InternalBuildHwndList: emulating ExpandWindowList failure.");
                    break;
                }
#endif
                if (!ExpandWindowList(&pbwl))
                    break;
            }
        }

        /*
         * Should we step through the Child windows?
         */
        if ((flags & BWL_ENUMCHILDREN) && pwnd->spwndChild != NULL) {
            pbwl = InternalBuildHwndList(pbwl, pwnd->spwndChild, BWL_ENUMLIST | BWL_ENUMCHILDREN);
            /*
             * If ExpandWindowList() failed in the recursive call,
             * we should just bail.
             */
            if (pbwl->phwndNext >= pbwl->phwndMax) {
                UserAssert(pbwl->phwndNext == pbwl->phwndMax);
                RIPMSG1(RIP_WARNING, "InternalBuildHwndList: failed to expand BWL in enumerating children. pbwl=%#p", pbwl);
                break;
            }
            UserAssert(pbwl->phwndNext < pbwl->phwndMax);
        }

        /*
         * Are we enumerating only one window?
         */
        if (!(flags & BWL_ENUMLIST))
            break;

        pwnd = pwnd->spwndNext;
    }

    return pbwl;
}


/***************************************************************************\
* FreeHwndList
*
* History:
* 10-20-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

void FreeHwndList(
    PBWL pbwl)
{
    PBWL *ppbwl;
    PBWL pbwlT;

    CheckCritIn();

    /*
     * We should never have an active bwl that is the free cached bwl
     */
    UserAssert(pbwl != pbwlCache);

    /*
     * Unlink this bwl from the list.
     */
    for (ppbwl = &gpbwlList; *ppbwl != NULL; ppbwl = &(*ppbwl)->pbwlNext) {
        if (*ppbwl == pbwl) {
            *ppbwl = pbwl->pbwlNext;

            /*
             * If the cache is empty or this pbwl is larger than the
             * cached one, save the pbwl there.
             */
            if (pbwlCache == NULL) {
                pbwlCache = pbwl;
            } else if ((pbwl->phwndMax - pbwl->rghwnd) >
                       (pbwlCache->phwndMax - pbwlCache->rghwnd)) {
                pbwlT = pbwlCache;
                pbwlCache = pbwl;
                UserFreePool((HANDLE)pbwlT);
            } else {
                UserFreePool((HANDLE)pbwl);
            }
            return;
        }
    }

    /*
     * Assert if we couldn't find the pbwl in the list...
     */
    UserAssert(FALSE);
}

#ifdef FE_IME

PBWL InternalRebuildHwndListForIMEClass(
    PBWL pbwl,
    BOOL fRemoveChild)
{
    PHWND phwndIME, phwndIMECur, phwnd, phwndCur;
    DWORD dwSize = (DWORD)((BYTE *)pbwl->phwndMax - (BYTE *)pbwl) + sizeof(HWND);

    phwndIMECur = phwndIME = (PHWND)UserAllocPool(dwSize, TAG_WINDOWLIST);
    if (phwndIME == NULL) {
        RIPMSG0(RIP_WARNING, "RebuildHwndListForIMEClass: invalid phwndIME");
        return pbwl;
    }

    phwndCur = pbwl->rghwnd;

    for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {
        PWND pwndIMEOwner;

        // Find the IME class or CS_IME window in the owners of hwnd.
        // When fRemoveChild is TRUE, we want IME class window as the return
        // of InternalGetIMEOwner.
        if (pwndIMEOwner = InternalGetIMEOwner(*phwnd, fRemoveChild)) {
            try {
                if (!fRemoveChild ||
                    (pwndIMEOwner->pcls->atomClassName == gpsi->atomSysClass[ICLS_IME] &&
                      ((PIMEWND)pwndIMEOwner)->pimeui != NULL &&
                     !ProbeAndReadStructure(((PIMEWND)pwndIMEOwner)->pimeui, IMEUI).fChildThreadDef))
                {
                    *phwndIMECur++ = *phwnd;
                }
            } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
            }
        } else {
            *phwndCur++ = *phwnd;
        }
    }

    // Here NULL s used as terminator.
    *phwndIMECur = NULL;

    phwndIMECur = phwndIME;
    while(*phwndIMECur != NULL)
        *phwndCur++ = *phwndIMECur++;

    if (*phwndCur != (HWND)1) {
        RIPMSG0(RIP_WARNING, "RebuildHwndListForIMEClass: Where is terminator?");
        *phwndCur = (HWND)1;
    }

    UserFreePool((HANDLE)phwndIME);
    return pbwl;
}

PWND InternalGetIMEOwner(
    HWND hwnd,
    BOOL fRetIMEWnd)
{
    PWND pwnd, pwndT, pwndIME;

    pwnd = RevalidateHwnd(hwnd);
    if (pwnd == NULL)
        return NULL;

    for (pwndT = pwnd; pwndT != NULL; pwndT = pwndT->spwndOwner) {
        if (TestCF(pwndT,CFIME) ||
                pwndT->pcls->atomClassName == gpsi->atomSysClass[ICLS_IME]) {

            if (!fRetIMEWnd)
                return pwndT;

            pwndIME = pwndT;

            while (pwndT && (pwndT->pcls->atomClassName != gpsi->atomSysClass[ICLS_IME]))
                pwndT = pwndT->spwndOwner;

            if (pwndT)
                pwndIME = pwndT;
            else
                RIPMSG0(RIP_WARNING, "Can't find IME Class window");

            return pwndIME;
        }
    }

    return NULL;
}

#endif
