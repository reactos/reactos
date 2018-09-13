/****************************** Module Header ******************************\
* Module Name: cleanup.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains code used to clean up after a dying thread.
*
* History:
* 02-15-91 DarrinM      Created.
* 01-16-92 IanJa        Neutralized ANSI/UNICODE (debug strings kept ANSI)
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* CheckForClientDeath
*
* Check to see if the client thread that is paired to the current running
* server thread has died.  If it has, we raise an exception so this thread
* can perform its cleanup duties.  NOTE: If the client has died, this
* will not be returning back to its caller.
*
* History:
* 05-23-91 DarrinM      Created.
\***************************************************************************/

/***************************************************************************\
* PseudoDestroyClassWindows
*
* Walk the window tree from hwndParent looking for windows
* of class wndClass.  If one is found, destroy it.
*
*
* WARNING windows actually destroys these windows.  We only zombie-ize them
* so this call does not have to be an xxx call.
*
* History:
* 25-Mar-1994 JohnC from win 3.1
\***************************************************************************/

VOID PseudoDestroyClassWindows(PWND pwndParent, PCLS pcls)
{
    PWND pwnd;
    PTHREADINFO pti;

    pti = PtiCurrent();

    /*
     * Recursively walk the window list and zombie any windows of this class
     */
    for (pwnd = pwndParent->spwndChild; pwnd != NULL; pwnd = pwnd->spwndNext) {

        /*
         * If this window belongs to this class then zombie it
         * if it was created by this message thread.
         */
        if (pwnd->pcls == pcls && pti == GETPTI(pwnd)) {

            /*
             * Zombie-ize the window
             *
             * Remove references to the client side window proc because that
             * WOW selector has been freed.
             */

            RIPMSG1(RIP_WARNING,
                    "USER: Wow Window not destroyed: %lX", pwnd);

            if (!TestWF(pwnd, WFSERVERSIDEPROC)) {
                pwnd->lpfnWndProc = (WNDPROC_PWND)gpsi->apfnClientA.pfnDefWindowProc;
            }
        }

        /*
         * Recurse downward to look for any children that might be
         * of this class.
         */
        if (pwnd->spwndChild != NULL)
            PseudoDestroyClassWindows(pwnd, pcls);
    }
}

/***************************************************************************\
* Go through all the windows owned by the dying queue and do the following:
*
* 1. Restore Standard window classes have their window procs restored
*    to their original value, in case they were subclassed.
*
* 2. App window classes have their window procs set to DefWindowProc
*    so that we don't execute any app code.
*
* Array of original window proc addresses,
* indexed by ICLS_* value is in globals.c now -- gpfnwp.
*
* This array is initialized in code in init.c.
\***************************************************************************/

VOID _WOWModuleUnload(HANDLE hModule) {
    PPROCESSINFO ppi = PpiCurrent();
    PHE     pheT, pheMax;
    PPCLS   ppcls;
    int     i;

    UserAssert(gpfnwp[0]);

    /*
     * PseudoDestroy windows with wndprocs from this hModule
     * If its a wow16 wndproc, check if the hMod16 is this module
     * and Nuke matches.
     */
    pheMax = &gSharedInfo.aheList[giheLast];
    for (pheT = gSharedInfo.aheList; pheT <= pheMax; pheT++) {
        PTHREADINFO ptiTest = (PTHREADINFO)pheT->pOwner;
        PWND pwnd;
        if ((pheT->bType == TYPE_WINDOW) &&
            (ptiTest->TIF_flags & TIF_16BIT) &&
            (ptiTest->ppi == ppi)) {

            pwnd = (PWND) pheT->phead;
            if (!TestWF(pwnd, WFSERVERSIDEPROC) &&
                IsWOWProc(pwnd->lpfnWndProc) &&
                (pwnd->hMod16 == (WORD)(ULONG_PTR)hModule)) {
                pwnd->lpfnWndProc = (WNDPROC_PWND)gpsi->apfnClientA.pfnDefWindowProc;
            }
        }
    }

    /*
     * Destroy private classes identified by hInstance that are not
     * referenced by any windows.  Mark in-use classes for later
     * destruction.
     */
    ppcls = &(ppi->pclsPrivateList);

    for (i = 0; i < 2; ++i) {
        while (*ppcls != NULL) {

            PWC pwc;
            PCLS pcls;

            if (HIWORD((ULONG_PTR)(*ppcls)->hModule) == (WORD)(ULONG_PTR)hModule) {
                if ((*ppcls)->cWndReferenceCount == 0) {
                    DestroyClass(ppcls);
                    /*
                     * DestroyClass does *ppcls = pcls->pclsNext;
                     * so we just want continue here
                     */
                } else {

                    /*
                     * Zap all the windows around that belong to this class.
                     */
                    PseudoDestroyClassWindows(PtiCurrent()->rpdesk->pDeskInfo->spwnd, *ppcls);

                    /*
                     * Win 3.1 does not distinguish between Dll's and Exe's
                     */
                    (*ppcls)->CSF_flags |= CSF_WOWDEFERDESTROY;
                    ppcls = &((*ppcls)->pclsNext);
                }
                continue;
            }

            pcls = *ppcls;

            if ((pcls->CSF_flags & CSF_WOWCLASS) && ((WORD)(ULONG_PTR)hModule == (pwc = PWCFromPCLS(pcls))->hMod16)) {

                ATOM atom;
                int  iSel;

                /*
                 * See if the window's class atom matches any of
                 * the system ones. If so, jam in the original window proc.
                 * Otherwise, use DefWindowProc
                 */
                atom = (*ppcls)->atomClassName;
                for (iSel = ICLS_BUTTON; iSel < ICLS_MAX; iSel++) {
                    if ((gpfnwp[iSel]) && (atom == gpsi->atomSysClass[iSel])) {
                        (*ppcls)->lpfnWndProc = (WNDPROC_PWND)gpfnwp[iSel];
                        break;
                    }
                }
                if (iSel == ICLS_MAX)
                    (*ppcls)->lpfnWndProc = (WNDPROC_PWND)gpsi->apfnClientW.pfnDefWindowProc;
            }

            ppcls = &((*ppcls)->pclsNext);
        }

        /*
         * Destroy public classes identified by hInstance that are not
         * referenced by any windows.  Mark in-use classes for later
         * destruction.
         */
        ppcls = &(ppi->pclsPublicList);
    }
    return;

}


/***************************************************************************\
* _WOWCleanup
*
* Private API to allow WOW to cleanup any process-owned resources when
* a WOW thread exits or when a DLL is unloaded.
*
* Note that at module cleanup, hInstance = the module handle and hTaskWow
* is NULL.  On task cleanup, hInstance = the hInst/hTask combined which
* matches the value passed in hModule to WowServerCreateCursorIcon and
* hTaskWow != NULL.
*
* History:
* 09-02-92 JimA         Created.
\***************************************************************************/

VOID _WOWCleanup(
    HANDLE hInstance,
    DWORD hTaskWow)
{
    PPROCESSINFO ppi = PpiCurrent();
    PPCLS   ppcls;
    PHE     pheT, pheMax;
    int     i;

    if (hInstance != NULL) {

        /*
         * Task cleanup
         */

        PWND pwnd;
        hTaskWow = (DWORD) LOWORD(hTaskWow);
        /*
         * Task exit called by wow. This loop will Pseudo-Destroy windows
         * created by this task.
         */
        pheMax = &gSharedInfo.aheList[giheLast];
        for (pheT = gSharedInfo.aheList; pheT <= pheMax; pheT++) {
            PTHREADINFO ptiTest = (PTHREADINFO)pheT->pOwner;
            if ((pheT->bType == TYPE_WINDOW) &&
                (ptiTest->TIF_flags & TIF_16BIT) &&
                (ptiTest->ptdb) &&
                (ptiTest->ptdb->hTaskWow == hTaskWow) &&
                (ptiTest->ppi == ppi)) {

                pwnd = (PWND) pheT->phead;
                if (!TestWF(pwnd, WFSERVERSIDEPROC)) {
                    pwnd->lpfnWndProc = (WNDPROC_PWND)gpsi->apfnClientA.pfnDefWindowProc;
                }
            }
        }
        return;
    }

    /*
     * If we get here, we are in thread cleanup and all of the thread's windows
     * have been destroyed or disassociated with any classes.  If a class
     * marked for destruction at this point still has windows, they must
     * belong to a dll.
     */

    /*
     * Destroy private classes marked for destruction
     */
    ppcls = &(ppi->pclsPrivateList);
    for (i = 0; i < 2; ++i) {
        while (*ppcls != NULL) {
            if ((*ppcls)->hTaskWow == hTaskWow &&
                    ((*ppcls)->CSF_flags & CSF_WOWDEFERDESTROY)) {
                if ((*ppcls)->cWndReferenceCount == 0) {
                    DestroyClass(ppcls);
                } else {
                    RIPMSG0(RIP_ERROR, "Windows remain for a WOW class marked for destruction");
                    ppcls = &((*ppcls)->pclsNext);
                }
            } else
                ppcls = &((*ppcls)->pclsNext);
        }

        /*
         * Destroy public classes marked for destruction
         */
        ppcls = &(ppi->pclsPublicList);
    }

    /*
     * Destroy menus, cursors, icons and accel tables identified by hTaskWow
     */
    pheMax = &gSharedInfo.aheList[giheLast];
    for (pheT = gSharedInfo.aheList; pheT <= pheMax; pheT++) {

        /*
         * Check against free before we look at ppi... because pq is stored
         * in the object itself, which won't be there if TYPE_FREE.
         */
        if (pheT->bType == TYPE_FREE)
            continue;

        /*
         * Destroy those objects created by this task.
         */
        if (    !(gahti[pheT->bType].bObjectCreateFlags & OCF_PROCESSOWNED) ||
                (PPROCESSINFO)pheT->pOwner != ppi ||
                (((PPROCOBJHEAD)pheT->phead)->hTaskWow != hTaskWow) ||
                (pheT->bType == TYPE_CALLPROC)  /* Do not destroy CALLPROCDATA objects.
                                                      * These should only get nuked when the
                                                      * process goes away or when the class
                                                      * is nuked.
                                                      */
                    ) {

            continue;
        }

        /*
         * Make sure this object isn't already marked to be destroyed - we'll
         * do no good if we try to destroy it now since it is locked.
         */
        if (pheT->bFlags & HANDLEF_DESTROY) {
            continue;
        }

        /*
         * Destroy this object.
         */
        HMDestroyUnlockedObject(pheT);
    }
}
