/****************************** Module Header ******************************\
* Module Name: hooks.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains the user hook APIs and support routines.
*
* History:
* 01-28-91 DavidPe      Created.
* 08 Feb 1992 IanJa     Unicode/ANSI aware & neutral
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*
 * This table is used to determine whether a particular hook
 * can be set for the system or a task, and other hook-ID specific things.
 */
#define HKF_SYSTEM          0x01
#define HKF_TASK            0x02
#define HKF_JOURNAL         0x04    // JOURNAL the mouse on set
#define HKF_NZRET           0x08    // Always return NZ hook for <=3.0 compatibility
#define HKF_INTERSENDABLE   0x10    // OK to call hookproc in context of hooking thread
#define HKF_LOWLEVEL        0x20    // Low level hook

CONST int ampiHookError[CWINHOOKS] = {
    0,                                   // WH_MSGFILTER (-1)
    0,                                   // WH_JOURNALRECORD 0
    -1,                                  // WH_JOURNALPLAYBACK 1
    0,                                   // WH_KEYBOARD 2
    0,                                   // WH_GETMESSAGE 3
    0,                                   // WH_CALLWNDPROC 4
    0,                                   // WH_CBT 5
    0,                                   // WH_SYSMSGFILTER 6
    0,                                   // WH_MOUSE 7
    0,                                   // WH_HARDWARE 8
    0,                                   // WH_DEBUG 9
    0,                                   // WH_SHELL 10
    0,                                   // WH_FOREGROUNDIDLE 11
    0,                                   // WH_CALLWNDPROCRET 12
    0,                                   // WH_KEYBOARD_LL 13
    0                                    // WH_MOUSE_LL 14
#ifdef REDIRECTION
   ,0                                    // WH_HITTEST
#endif // REDIRECTION
};

CONST BYTE abHookFlags[CWINHOOKS] = {
    HKF_SYSTEM | HKF_TASK | HKF_NZRET                       , // WH_MSGFILTER (-1)
    HKF_SYSTEM | HKF_JOURNAL          | HKF_INTERSENDABLE   , // WH_JOURNALRECORD 0
    HKF_SYSTEM | HKF_JOURNAL          | HKF_INTERSENDABLE   , // WH_JOURNALPLAYBACK 1
    HKF_SYSTEM | HKF_TASK | HKF_NZRET | HKF_INTERSENDABLE   , // WH_KEYBOARD 2
    HKF_SYSTEM | HKF_TASK                                   , // WH_GETMESSAGE 3
    HKF_SYSTEM | HKF_TASK                                   , // WH_CALLWNDPROC 4
    HKF_SYSTEM | HKF_TASK                                   , // WH_CBT 5
    HKF_SYSTEM                                              , // WH_SYSMSGFILTER 6
    HKF_SYSTEM | HKF_TASK             | HKF_INTERSENDABLE   , // WH_MOUSE 7
    HKF_SYSTEM | HKF_TASK                                   , // WH_HARDWARE 8
    HKF_SYSTEM | HKF_TASK                                   , // WH_DEBUG 9
    HKF_SYSTEM | HKF_TASK                                   , // WH_SHELL 10
    HKF_SYSTEM | HKF_TASK                                   , // WH_FOREGROUNDIDLE 11
    HKF_SYSTEM | HKF_TASK                                   , // WH_CALLWNDPROCRET 12
    HKF_SYSTEM | HKF_LOWLEVEL         | HKF_INTERSENDABLE   , // WH_KEYBOARD_LL 13
    HKF_SYSTEM | HKF_LOWLEVEL         | HKF_INTERSENDABLE     // WH_MOUSE_LL 14

#ifdef REDIRECTION
   ,HKF_SYSTEM | HKF_LOWLEVEL         | HKF_INTERSENDABLE     // WH_HITTEST 15
#endif // REDIRECTION
};


/*
 * HACK (hiroyama) see xxxCallJournalPlaybackHook()
 * Optimization: faster determination whether the message is one of
 * WM_[SYS][DEAD]CHAR.
 * Argument (msg) requires to be one of keyboard messages. Range check
 * should be done before calling IS_CHAR_MSG() macro.
 *
 * (i.e. WM_KEYFIRST <= msg < WM_KEYLAST)
 *
 * We expect bit 0x02 of all WM_*CHAR messages to be set.
 * and bit 0x02 of all WM_*KEY* messages to be clear
 *
 * WM_KEYDOWN       0x100   000
 * WM_KEYUP         0x101   001
 * WM_CHAR          0x102   010
 * WM_DEADCHAR      0x103   011
 *
 * WM_SYSKEYDOWN    0x104   100
 * WM_SYSKEYUP      0x105   101
 * WM_SYSCHAR       0x106   110
 * WM_SYSDEADCHAR   0x107   111
 *
 */

  /*
   */
#if (WM_KEYFIRST != 0x100) ||    \
    (WM_KEYLAST != 0x108) ||     \
    (WM_KEYDOWN & 0x2) ||        \
    (WM_KEYUP & 0x2) ||          \
    (WM_SYSKEYDOWN & 0x2) ||     \
    (WM_SYSKEYUP & 0x2) ||       \
    !(WM_CHAR & 0x02) ||         \
    !(WM_DEADCHAR & 0x02) ||     \
    !(WM_SYSCHAR & 0x02) ||      \
    !(WM_SYSDEADCHAR & 0x02)
#error "unexpected value in keyboard messages."
#endif


#if DBG

BOOL IsCharMsg(UINT msg)
{
    UserAssert(msg >= WM_KEYFIRST && msg < WM_KEYLAST);

    return msg & 0x02;
}

#define IS_CHAR_MSG(msg)    IsCharMsg(msg)

#else

#define IS_CHAR_MSG(msg)    ((msg) & 0x02)

#endif




void UnlinkHook(PHOOK phkFree);
/***************************************************************************\
* DbgValidateThisHook
*
* Validates a hook structure and returns the start of its chain.
*
* History:
* 03-25-97  GerardoB    Created
\***************************************************************************/
#if DBG
PHOOK * DbgValidateThisHook (PHOOK phk, int iType, PTHREADINFO ptiHooked)
{
    CheckCritIn();
    /*
     * No bogus flags
     */
    UserAssert(!(phk->flags & ~HF_DBGUSED));
    /*
     * Type
     */
    UserAssert(phk->iHook == iType);
    /*
     * HF_GLOBAL & ptiHooked. return the start of its hook chain.
     */
    if (phk->flags & HF_GLOBAL) {
        UserAssert(phk->ptiHooked == NULL);
        if (phk->rpdesk != NULL) {
            UserAssert(GETPTI(phk) == gptiRit);
            return &phk->rpdesk->pDeskInfo->aphkStart[iType + 1];
        } else {
            return &GETPTI(phk)->pDeskInfo->aphkStart[iType + 1];
        }
    } else {
        UserAssert((phk->ptiHooked == ptiHooked)
                    || (abHookFlags[iType + 1] & HKF_INTERSENDABLE));

        return &(phk->ptiHooked->aphkStart[iType + 1]);
    }
}
/***************************************************************************\
* DbgValidatefsHook
*
* Make sure that the fsHook bit masks are in sync. If the bits
*  are out of sync, some hook must have the HF_INCHECKWHF flag
*  (this means the bits are being adjusted right now)
*
* History:
* 05-20-97  GerardoB    Extracted from PhkFirst*Valid
\***************************************************************************/
void DbgValidatefsHook(PHOOK phk, int nFilterType, PTHREADINFO pti, BOOL fGlobal)
{
    CheckCritIn();
    /*
     * If no pti is provided, figure out what it should be.
     *  phk is expected to be NULL.
     */
    if (pti == NULL) {
        fGlobal = (phk->flags & HF_GLOBAL);
        if (fGlobal) {
            pti = GETPTI(phk);
        } else {
            pti = phk->ptiHooked;
            UserAssert(pti != NULL);
        }
    }

    if (fGlobal) {
        if ((phk != NULL) ^ IsGlobalHooked(pti, WHF_FROM_WH(nFilterType))) {
            PHOOK phkTemp = pti->pDeskInfo->aphkStart[nFilterType + 1];
            while ((phkTemp != NULL) && !(phkTemp->flags & HF_INCHECKWHF)) {
                phkTemp = phkTemp->phkNext;
            }
            UserAssert(phkTemp != NULL);
        }
    } else {
        if ((phk != NULL) ^ IsHooked(pti, WHF_FROM_WH(nFilterType))) {
            PHOOK phkTemp = pti->aphkStart[nFilterType + 1];
            while ((phkTemp != NULL) && !(phkTemp->flags & HF_INCHECKWHF)) {
                phkTemp = phkTemp->phkNext;
            }
            if (phkTemp == NULL) {
                phkTemp = pti->pDeskInfo->aphkStart[nFilterType + 1];
                while ((phkTemp != NULL) && !(phkTemp->flags & HF_INCHECKWHF)) {
                    phkTemp = phkTemp->phkNext;
                }
            }
            UserAssert(phkTemp != NULL);
        }
    }
}
/***************************************************************************\
* DbgValidateHooks
*
* This functions expects valid (not destroyed) and properly linked.
* History:
* 03-25-97  GerardoB    Created
\***************************************************************************/
void DbgValidateHooks (PHOOK phk, int iType)
{
    PHOOK *pphkStart, *pphkNext;
    if (phk == NULL) {
        return;
    }
    /*
     * It shouldn't be destroyed
     */
    UserAssert(!(phk->flags & (HF_DESTROYED | HF_FREED)));
    /*
     * Validate fsHooks
     */
    DbgValidatefsHook(phk, iType, NULL, FALSE);
    /*
     * Validate this hook and get the beginning of the hook chain
     */
    pphkStart = DbgValidateThisHook(phk, iType, phk->ptiHooked);
    /*
     * There must be at least one hook in the chain
     */
    UserAssert(*pphkStart != NULL);
    /*
     * Validate the link.
     * And while your're at it, validate all hooks!
     */
    pphkNext = pphkStart;
    while ((*pphkNext != phk) && (*pphkNext != NULL)) {
       UserAssert(pphkStart == DbgValidateThisHook(*pphkNext, iType, phk->ptiHooked));
       pphkNext = &(*pphkNext)->phkNext;
    }
    /*
     * Verify that we found it.
     */
    UserAssert(*pphkNext == phk);
    /*
     * Walk until the end of the chain
     */
    while (*pphkNext != NULL) {
       UserAssert(pphkStart == DbgValidateThisHook(*pphkNext, iType, phk->ptiHooked));
       pphkNext = &(*pphkNext)->phkNext;
    }
}
#else
#define DbgValidatefsHook(phk, nFilterType, pti, fGlobal)
#endif /* DBG */
/***************************************************************************\
* zzzJournalAttach
*
* This attaches/detaches threads to one input queue so input is synchronized.
* Journalling requires this.
*
* 12-10-92 ScottLu      Created.
\***************************************************************************/

BOOL zzzJournalAttach(
    PTHREADINFO pti,
    BOOL fAttach)
{
    PTHREADINFO ptiT;
    PQ pq;
    PLIST_ENTRY pHead, pEntry;

    /*
     * If we're attaching, calculate the pqAttach for all threads journalling.
     * If we're unattaching, just call zzzReattachThreads() and it will calculate
     * the non-journalling queues to attach to.
     */
    if (fAttach) {
        if ((pq = AllocQueue(pti, NULL)) == NULL)
            return FALSE;

        pHead = &pti->rpdesk->PtiList;
        for (pEntry = pHead->Flink; pEntry != pHead; pEntry = pEntry->Flink) {
            ptiT = CONTAINING_RECORD(pEntry, THREADINFO, PtiLink);

            /*
             * This is the Q to attach to for all threads that will do
             * journalling.
             */
            if (!(ptiT->TIF_flags & (TIF_DONTJOURNALATTACH | TIF_INCLEANUP))) {
                ptiT->pqAttach = pq;
                ptiT->pqAttach->cThreads++;
            }
        }
    }

    return zzzReattachThreads(fAttach);
}
/***************************************************************************\
* InterQueueMsgCleanup
*
* Walk gpsmsList looking for inter queue messages with a hung receiver;
*  if one is found and it's a message that would have been an async event or
*  intra queue if not journalling, then it cleans it up.
*
* While Journalling most threads are attached to the same queue. This causes
*  activation and input stuff to be synchronous; if a thread hangs or dies,
*  any other thread sending a message to the hung/dead thread will be
*  blocked for good.
* This is critical when the blocked thread is cssr; this can happen with
*  console windows or when some one requests a hard error box, specially
*  during window activation.
*
* This function must be called when all queues have been detached (unless previously attached),
*  so we can take care of hung/dead receivers with pending SMSs.
*
* 03-28-96 GerardoB     Created
\***************************************************************************/
void InterQueueMsgCleanup (DWORD dwTimeFromLastRead)
{
    PSMS *ppsms;
    PSMS psmsNext;

    CheckCritIn();

    /*
     * Walk  gpsmsList
     */
    for (ppsms = &gpsmsList; *ppsms; ) {
        psmsNext = (*ppsms)->psmsNext;
        /*
         * If this is an inter queue message
         */
        if (((*ppsms)->ptiSender != NULL)
                && ((*ppsms)->ptiReceiver != NULL)
                && ((*ppsms)->ptiSender->pq != (*ppsms)->ptiReceiver->pq)) {
            /*
             * If the receiver has been hung for a while
             */
            if (FHungApp ((*ppsms)->ptiReceiver, dwTimeFromLastRead)) {

                switch ((*ppsms)->message) {
                    /*
                     * Activation messages
                     */
                    case WM_NCACTIVATE:
                    case WM_ACTIVATEAPP:
                    case WM_ACTIVATE:
                    case WM_SETFOCUS:
                    case WM_KILLFOCUS:
                    case WM_QUERYNEWPALETTE:
                    /*
                     * Sent to spwndFocus, which now can be in a different queue
                     */
                    case WM_INPUTLANGCHANGE:
                        RIPMSG3 (RIP_WARNING, "InterQueueMsgCleanup: ptiSender:%#p ptiReceiver:%#p message:%#lx",
                                    (*ppsms)->ptiSender, (*ppsms)->ptiReceiver, (*ppsms)->message);
                        ReceiverDied(*ppsms, ppsms);
                        break;

                } /* switch */

            } /* If hung receiver */

        } /* If inter queue message */

         /*
          * If the message was not unlinked, go to the next one.
          */
        if (*ppsms != psmsNext)
            ppsms = &(*ppsms)->psmsNext;

    } /* for */
}
/***************************************************************************\
* zzzCancelJournalling
*
* Journalling is cancelled with control-escape is pressed, or when the desktop
* is switched.
*
* 01-27-93 ScottLu      Created.
\***************************************************************************/

void zzzCancelJournalling(void)
{
    PTHREADINFO ptiCancelJournal;
    PHOOK phook;
    PHOOK phookNext;

    /*
     * Mouse buttons sometimes get stuck down due to hardware glitches,
     * usually due to input concentrator switchboxes or faulty serial
     * mouse COM ports, so clear the global button state here just in case,
     * otherwise we may not be able to change focus with the mouse.
     * Also do this in Alt-Tab processing.
     */
#if DBG
    if (gwMouseOwnerButton)
        RIPMSG1(RIP_WARNING,
                "gwMouseOwnerButton=%x, being cleared forcibly\n",
                gwMouseOwnerButton);
#endif
    gwMouseOwnerButton = 0;

    /*
     * Remove journal hooks. This'll cause threads to associate with
     * different queues.
     * DeferWinEventNotify() so we can traverse the phook list safely
     */
    DeferWinEventNotify();
    UserAssert(gptiRit->pDeskInfo == grpdeskRitInput->pDeskInfo);
    phook = PhkFirstGlobalValid(gptiRit, WH_JOURNALPLAYBACK);
    while (phook != NULL) {
        ptiCancelJournal = phook->head.pti;

        if (ptiCancelJournal != NULL) {
            /*
             * Let the thread that set the journal hook know this is happening.
             */
            _PostThreadMessage(ptiCancelJournal, WM_CANCELJOURNAL, 0, 0);

            /*
             * If there was an app waiting for a response back from the journal
             * application, cancel that request so the app can continue running
             * (for example, we don't want winlogon or console to wait for an
             * app that may be hung!)
             */
            SendMsgCleanup(ptiCancelJournal);
        }

        phookNext = PhkNextValid(phook);
        zzzUnhookWindowsHookEx(phook);        // May free phook memory
        phook = phookNext;
    }
    zzzEndDeferWinEventNotify();

    /*
     * DeferWinEventNotify() so we can traverse the phook list safely
     */
    DeferWinEventNotify();
    UserAssert(gptiRit->pDeskInfo == grpdeskRitInput->pDeskInfo);
    phook = PhkFirstGlobalValid(gptiRit, WH_JOURNALRECORD);
    while (phook != NULL) {
        ptiCancelJournal = phook->head.pti;

        if (ptiCancelJournal != NULL) {
            /*
             * Let the thread that set the journal hook know this is happening.
             */
            _PostThreadMessage(ptiCancelJournal, WM_CANCELJOURNAL, 0, 0);

            /*
             * If there was an app waiting for a response back from the journal
             * application, cancel that request so the app can continue running
             * (for example, we don't want winlogon or console to wait for an
             * app that may be hung!)
             */
            SendMsgCleanup(ptiCancelJournal);
        }

        phookNext = PhkNextValid(phook);
        zzzUnhookWindowsHookEx(phook);        // May free phook memory
        phook = phookNext;
    }
    zzzEndDeferWinEventNotify();


    /*
     * Make sure journalling ssync mode didn't hose any one
     */
    InterQueueMsgCleanup(CMSWAITTOKILLTIMEOUT);

    /*
     * Unlock SetForegroundWindow (if locked)
     */
    gppiLockSFW = NULL;

    /*
     * NT5's last minute hack for evil applications, who disables the desktop window
     * (perhaps by accidents though) leaving the system pretty unusable.
     * See Raid #423704.
     */
    if (grpdeskRitInput && grpdeskRitInput->pDeskInfo) {
        PWND pwndDesktop = grpdeskRitInput->pDeskInfo->spwnd;

        if (pwndDesktop && TestWF(pwndDesktop, WFDISABLED)) {
            ClrWF(pwndDesktop, WFDISABLED);
        }
    }
}

/***************************************************************************\
* zzzSetWindowsHookAW (API)
*
* This is the Win32 version of the SetWindowsHook() call.  It has the
* same characteristics as far as return values, but only sets 'local'
* hooks.  This is because we weren't provided a DLL we can load into
* other processes.  Because of this WH_SYSMSGFILTER is no longer a
* valid hook.  Apps will either need to call with WH_MSGFILTER or call
* the new API SetWindowsHookEx().  Essentially this API is obsolete and
* everyone should call SetWindowsHookEx().
*
* History:
* 10-Feb-1991 DavidPe       Created.
* 30-Jan-1992 IanJa         Added bAnsi parameter
\***************************************************************************/

PROC zzzSetWindowsHookAW(
    int nFilterType,
    PROC pfnFilterProc,
    DWORD dwFlags)
{
    PHOOK phk;

    phk = zzzSetWindowsHookEx(NULL, NULL, PtiCurrent(),
            nFilterType, pfnFilterProc, dwFlags);

    /*
     * If we get an error from zzzSetWindowsHookEx() then we return
     * -1 to be compatible with older version of Windows.
     */
    if (phk == NULL) {
        return (PROC)-1;
    }

    /*
     * Handle the backwards compatibility return value cases for
     * SetWindowsHook.  If this was the first hook in the chain,
     * then return NULL, else return something non-zero.  HKF_NZRET
     * is a special case where SetWindowsHook would always return
     * something because there was a default hook installed.  Some
     * apps relied on a non-zero return value in those cases.
     */
    if ((phk->phkNext != NULL) || (abHookFlags[nFilterType + 1] & HKF_NZRET)) {
        return (PROC)phk;
    }

    return NULL;
}


/***************************************************************************\
* zzzSetWindowsHookEx
*
* SetWindowsHookEx() is the updated version of SetWindowsHook().  It allows
* applications to set hooks on specific threads or throughout the entire
* system.  The function returns a hook handle to the application if
* successful and NULL if a failure occured.
*
* History:
* 28-Jan-1991 DavidPe      Created.
* 15-May-1991 ScottLu      Changed to work client/server.
* 30-Jan-1992 IanJa        Added bAnsi parameter
\***************************************************************************/

PHOOK zzzSetWindowsHookEx(
    HANDLE hmod,
    PUNICODE_STRING pstrLib,
    PTHREADINFO ptiThread,
    int nFilterType,
    PROC pfnFilterProc,
    DWORD dwFlags)
{
    ACCESS_MASK amDesired;
    PHOOK       phkNew;
    TL          tlphkNew;
    PHOOK       *pphkStart;
    PTHREADINFO ptiCurrent;

    /*
     * Check to see if filter type is valid.
     */
    if ((nFilterType < WH_MIN) || (nFilterType > WH_MAX)) {
        RIPERR0(ERROR_INVALID_HOOK_FILTER, RIP_VERBOSE, "");
        return NULL;
    }

    /*
     * Check to see if filter proc is valid.
     */
    if (pfnFilterProc == NULL) {
        RIPERR0(ERROR_INVALID_FILTER_PROC, RIP_VERBOSE, "");
        return NULL;
    }

    ptiCurrent = PtiCurrent();

    if (ptiThread == NULL) {
        /*
         * Is the app trying to set a global hook without a library?
         * If so return an error.
         */
         if (hmod == NULL) {
             RIPERR0(ERROR_HOOK_NEEDS_HMOD, RIP_VERBOSE, "");
             return NULL;
         }
    } else {
        /*
         * Is the app trying to set a local hook that is global-only?
         * If so return an error.
         */
        if (!(abHookFlags[nFilterType + 1] & HKF_TASK)) {
            RIPERR0(ERROR_GLOBAL_ONLY_HOOK, RIP_VERBOSE, "");
            return NULL;
        }

        /*
         * Can't hook outside our own desktop.
         */
        if (ptiThread->rpdesk != ptiCurrent->rpdesk) {
            RIPERR0(ERROR_ACCESS_DENIED,
                   RIP_WARNING,
                   "Access denied to desktop in zzzSetWindowsHookEx - can't hook other desktops");

            return NULL;
        }

        if (ptiCurrent->ppi != ptiThread->ppi) {
            /*
             * Is the app trying to set hook in another process without a library?
             * If so return an error.
             */
            if (hmod == NULL) {
                RIPERR0(ERROR_HOOK_NEEDS_HMOD, RIP_VERBOSE, "");
                return NULL;
            }

            /*
             * Is the app hooking another user without access?
             * If so return an error. Note that this check is done
             * for global hooks every time the hook is called.
             */
            if ((!RtlEqualLuid(&ptiThread->ppi->luidSession,
                               &ptiCurrent->ppi->luidSession)) &&
                        !(ptiThread->TIF_flags & TIF_ALLOWOTHERACCOUNTHOOK)) {

                RIPERR0(ERROR_ACCESS_DENIED,
                        RIP_WARNING,
                        "Access denied to other user in zzzSetWindowsHookEx");

                return NULL;
            }

            if ((ptiThread->TIF_flags & (TIF_CSRSSTHREAD | TIF_SYSTEMTHREAD)) &&
                    !(abHookFlags[nFilterType + 1] & HKF_INTERSENDABLE)) {

                /*
                 * Can't hook console or GUI system thread if inter-thread
                 * calling isn't implemented for this hook type.
                 */
                 RIPERR1(ERROR_HOOK_TYPE_NOT_ALLOWED,
                         RIP_WARNING,
                         "nFilterType (%ld) not allowed in zzzSetWindowsHookEx",
                         nFilterType);

                 return NULL;
            }
        }
    }

    /*
     * Check if this thread has access to hook its desktop.
     */
    switch( nFilterType ) {
    case WH_JOURNALRECORD:
        amDesired = DESKTOP_JOURNALRECORD;
        break;

    case WH_JOURNALPLAYBACK:
        amDesired = DESKTOP_JOURNALPLAYBACK;
        break;

    default:
        amDesired = DESKTOP_HOOKCONTROL;
        break;
    }

    if (!RtlAreAllAccessesGranted(ptiCurrent->amdesk, amDesired)) {
         RIPERR0(ERROR_ACCESS_DENIED,
                RIP_WARNING,
                "Access denied to desktop in zzzSetWindowsHookEx");

         return NULL;
    }

    if (amDesired != DESKTOP_HOOKCONTROL &&
        (ptiCurrent->rpdesk->rpwinstaParent->dwWSF_Flags & WSF_NOIO)) {
        RIPERR0(ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION,
                RIP_WARNING,
                "Journal hooks invalid on a desktop belonging to a non-interactive WindowStation.");

        return NULL;
    }

#if 0
    /*
     * Is this a journal hook?
     */
    if (abHookFlags[nFilterType + 1] & HKF_JOURNAL) {
        /*
         * Is a journal hook of this type already installed?
         * If so it's an error.
         * If this code is enabled, use PhkFirstGlobalValid instead
         *  of checking phkStart directly
         */
        if (ptiCurrent->pDeskInfo->asphkStart[nFilterType + 1] != NULL) {
            RIPERR0(ERROR_JOURNAL_HOOK_SET, RIP_VERBOSE, "");
            return NULL;
        }
    }
#endif

    /*
     * Allocate the new HOOK structure.
     */
    phkNew = (PHOOK)HMAllocObject(ptiCurrent, ptiCurrent->rpdesk,
            TYPE_HOOK, sizeof(HOOK));
    if (phkNew == NULL) {
        return NULL;
    }

    /*
     * If a DLL is required for this hook, register the library with
     * the library management routines so we can assure it's loaded
     * into all the processes necessary.
     */
    phkNew->ihmod = -1;
    if (hmod != NULL) {

#if defined(WX86)

        phkNew->flags |= (dwFlags & HF_WX86KNOWNDLL);

#endif

        phkNew->ihmod = GetHmodTableIndex(pstrLib);

        if (phkNew->ihmod == -1) {
            RIPERR0(ERROR_MOD_NOT_FOUND, RIP_VERBOSE, "");
            HMFreeObject((PVOID)phkNew);
            return NULL;
        }

        /*
         * Add a dependency on this module - meaning, increment a count
         * that simply counts the number of hooks set into this module.
         */
        if (phkNew->ihmod >= 0) {
            AddHmodDependency(phkNew->ihmod);
        }
    }

    /*
     * Depending on whether we're setting a global or local hook,
     * get the start of the appropriate linked-list of HOOKs.  Also
     * set the HF_GLOBAL flag if it's a global hook.
     */
    if (ptiThread != NULL) {
        pphkStart = &ptiThread->aphkStart[nFilterType + 1];

        /*
         * Set the WHF_* in the THREADINFO so we know it's hooked.
         */
        ptiThread->fsHooks |= WHF_FROM_WH(nFilterType);

        /*
         * Set the flags in the thread's TEB
         */
        if (ptiThread->pClientInfo) {
            BOOL fAttached;

            /*
             * If the thread being hooked is in another process, attach
             * to that process so that we can access its ClientInfo.
             */
            if (ptiThread->ppi != ptiCurrent->ppi) {
                KeAttachProcess(&ptiThread->ppi->Process->Pcb);
                fAttached = TRUE;
            } else
                fAttached = FALSE;

            ptiThread->pClientInfo->fsHooks = ptiThread->fsHooks;

            if (fAttached)
                KeDetachProcess();
        }

        /*
         * Remember which thread we're hooking.
         */
        phkNew->ptiHooked = ptiThread;

    } else {
        pphkStart = &ptiCurrent->pDeskInfo->aphkStart[nFilterType + 1];
        phkNew->flags |= HF_GLOBAL;

        /*
         * Set the WHF_* in the SERVERINFO so we know it's hooked.
         */
        ptiCurrent->pDeskInfo->fsHooks |= WHF_FROM_WH(nFilterType);

        phkNew->ptiHooked = NULL;
    }

    /*
     * Does the hook function expect ANSI or Unicode text?
     */
    phkNew->flags |= (dwFlags & HF_ANSI);

    /*
     * Initialize the HOOK structure.  Unreferenced parameters are assumed
     * to be initialized to zero by LocalAlloc().
     */
    phkNew->iHook = nFilterType;

    /*
     * Libraries are loaded at different linear addresses in different
     * process contexts.  For this reason, we need to convert the filter
     * proc address into an offset while setting the hook, and then convert
     * it back to a real per-process function pointer when calling a
     * hook.  Do this by subtracting the 'hmod' (which is a pointer to the
     * linear and contiguous .exe header) from the function index.
     */
    phkNew->offPfn = ((ULONG_PTR)pfnFilterProc) - ((ULONG_PTR)hmod);

#ifdef HOOKBATCH
    phkNew->cEventMessages = 0;
    phkNew->iCurrentEvent  = 0;
    phkNew->CacheTimeOut = 0;
    phkNew->aEventCache = NULL;
#endif //HOOKBATCH

    /*
     * Link this hook into the front of the hook-list.
     */
    phkNew->phkNext = *pphkStart;
    *pphkStart = phkNew;

    /*
     * If this is a journal hook, setup synchronized input processing
     * AFTER we set the hook - so this synchronization can be cancelled
     * with control-esc.
     */
    if (abHookFlags[nFilterType + 1] & HKF_JOURNAL) {
        /*
         * Attach everyone to us so journal-hook processing
         * will be synchronized.
         * No need to DeferWinEventNotify() here, since we lock phkNew.
         */
        ThreadLockAlwaysWithPti(ptiCurrent, phkNew, &tlphkNew);
        if (!zzzJournalAttach(ptiCurrent, TRUE)) {
            RIPMSG1(RIP_WARNING, "zzzJournalAttach failed, so abort hook %#p", phkNew);
            if (ThreadUnlock(&tlphkNew) != NULL) {
                zzzUnhookWindowsHookEx(phkNew);
            }
            return NULL;
        }
        if ((phkNew = ThreadUnlock(&tlphkNew)) == NULL) {
            return NULL;
        }
    }

    UserAssert(phkNew != NULL);

    /*
     * Later 5.0 GerardoB: The old code just to check this but
     *  I think it's some left over stuff from server side days.
    .* Let's assert on it for a while
     * Also, I added the assertions in the else's below because I reorganized
     *  the code and want to make sure we don't change behavior
     */
    UserAssert(ptiCurrent->pEThread && THREAD_TO_PROCESS(ptiCurrent->pEThread));

    /*
     * Can't allow a process that has set a global hook that works
     * on server-side winprocs to run at background priority! Bump
     * up it's dynamic priority and mark it so it doesn't get reset.
     */
    if ((phkNew->flags & HF_GLOBAL) &&
            (abHookFlags[nFilterType + 1] & HKF_INTERSENDABLE)) {

        ptiCurrent->TIF_flags |= TIF_GLOBALHOOKER;
        KeSetPriorityThread(&ptiCurrent->pEThread->Tcb, LOW_REALTIME_PRIORITY-2);

        if (abHookFlags[nFilterType + 1] & HKF_JOURNAL) {
            ThreadLockAlwaysWithPti(ptiCurrent, phkNew, &tlphkNew);
            /*
             * If we're changing the journal hooks, jiggle the mouse.
             * This way the first event will always be a mouse move, which
             * will ensure that the cursor is set properly.
             */
            zzzSetFMouseMoved();
            phkNew = ThreadUnlock(&tlphkNew);
            /*
             * If setting a journal playback hook, this process is the input
             *  provider. This gives it the right to call SetForegroundWindow
             */
            if (nFilterType == WH_JOURNALPLAYBACK) {
                gppiInputProvider = ptiCurrent->ppi;
            }
        } else {
            UserAssert(nFilterType != WH_JOURNALPLAYBACK);
        }
    } else {
        UserAssert(!(abHookFlags[nFilterType + 1] & HKF_JOURNAL));
        UserAssert(nFilterType != WH_JOURNALPLAYBACK);
    }




    /*
     * Return pointer to our internal hook structure so we know
     * which hook to call next in CallNextHookEx().
     */
    DbgValidateHooks(phkNew, phkNew->iHook);
    return phkNew;
}


/***************************************************************************\
* xxxCallNextHookEx
*
* In the new world DefHookProc() is a bit deceptive since SetWindowsHook()
* isn't returning the actual address of the next hook to call, but instead
* a hook handle.  CallNextHookEx() is a slightly clearer picture of what's
* going on so apps don't get tempted to try and call the value we return.
*
* As a side note we don't actually use the hook handle passed in.  We keep
* track of which hooks is currently being called on a thread in the Q
* structure and use that.  This is because SetWindowsHook() will sometimes
* return NULL to be compatible with the way it used to work, but even though
* we may be dealing with the last 'local' hook, there may be further 'global'
* hooks we need to call.  PhkNext() is smart enough to jump over to the
* 'global' hook chain if it reaches the end of the 'local' hook chain.
*
* History:
* 01-30-91  DavidPe         Created.
\***************************************************************************/

LRESULT xxxCallNextHookEx(
    int nCode,
    WPARAM wParam,
    LPARAM lParam)
{
    BOOL bAnsiHook;

    if (PtiCurrent()->sphkCurrent == NULL) {
        return 0;
    }

    return xxxCallHook2(PhkNextValid(PtiCurrent()->sphkCurrent), nCode, wParam, lParam, &bAnsiHook);
}


/***************************************************************************\
* CheckWHFBits
*
* This routine checks to see if any hooks for nFilterType exist, and clear
* the appropriate WHF_ in the THREADINFO and SERVERINFO.
*
* History:
* 08-17-92  DavidPe         Created.
\***************************************************************************/

VOID CheckWHFBits(
    PTHREADINFO pti,
    int nFilterType)
{
    BOOL fClearThreadBits;
    BOOL fClearDesktopBits;
    PHOOK phook;


    /*
     * Assume we're are going to clear local(thread) and
     *   global(desktop) bits.
     */
    fClearThreadBits = TRUE;
    fClearDesktopBits = TRUE;
    /*
     * Get the first valid hook for this thread
     */
    phook = PhkFirstValid(pti, nFilterType);
    if (phook != NULL) {
        /*
         * If it found a global hook, don't clear the desktop bits
         * (that would mean that there are no local(thread) hooks
         *  so we fall through to clear the thread bits)
         */
        if (phook->flags & HF_GLOBAL) {
            fClearDesktopBits = FALSE;
        } else {
            /*
             * It found a thread hook so don't clear the thread bits
             */
            fClearThreadBits = FALSE;
            /*
             * Check for global hooks now. If there is one, don't
             *  clear the desktop bits
             */
            phook = PhkFirstGlobalValid(pti, nFilterType);
            fClearDesktopBits = (phook == NULL);
        }
    } /* if (phook != NULL) */

    if (fClearThreadBits) {
        pti->fsHooks &= ~(WHF_FROM_WH(nFilterType));
        /*
         * Set the flags in the thread's TEB
         */
        if (pti->pClientInfo) {
            BOOL fAttached;
            /*
             * If the hooked thread is in another process, attach
             * to that process to access its address space.
             */
            if (pti->ppi != PpiCurrent()) {
                KeAttachProcess(&pti->ppi->Process->Pcb);
                fAttached = TRUE;
            } else
                fAttached = FALSE;

            pti->pClientInfo->fsHooks = pti->fsHooks;

            if (fAttached)
                KeDetachProcess();
        }
    }

    if (fClearDesktopBits) {
        pti->pDeskInfo->fsHooks &= ~(WHF_FROM_WH(nFilterType));
    }
}


/***************************************************************************\
* zzzUnhookWindowsHook (API)
*
* This is the old version of the Unhook API.  It does the same thing as
* zzzUnhookWindowsHookEx(), but takes a filter-type and filter-proc to
* identify which hook to unhook.
*
* History:
* 01-28-91  DavidPe         Created.
\***************************************************************************/

BOOL zzzUnhookWindowsHook(
    int nFilterType,
    PROC pfnFilterProc)
{
    PHOOK phk;
    PTHREADINFO ptiCurrent;

    if ((nFilterType < WH_MIN) || (nFilterType > WH_MAX)) {
        RIPERR0(ERROR_INVALID_HOOK_FILTER, RIP_VERBOSE, "");
        return FALSE;
    }

    ptiCurrent = PtiCurrent();

    for (phk = PhkFirstValid(ptiCurrent, nFilterType); phk != NULL; phk = PhkNextValid(phk)) {
        /*
         * Is this the hook we're looking for?
         */
        if (PFNHOOK(phk) == pfnFilterProc) {

            /*
             * Are we on the thread that set the hook?
             * If not return an error.
             */
            if (GETPTI(phk) != ptiCurrent) {
                RIPERR0(ERROR_ACCESS_DENIED,
                        RIP_WARNING,
                        "Access denied in zzzUnhookWindowsHook: "
                        "this thread is not the same as that which set the hook");

                return FALSE;
            }

            return zzzUnhookWindowsHookEx( phk );
        }
    }

    /*
     * Didn't find the hook we were looking for so return FALSE.
     */
    RIPERR0(ERROR_HOOK_NOT_INSTALLED, RIP_VERBOSE, "");
    return FALSE;
}


/***************************************************************************\
* zzzUnhookWindowsHookEx (API)
*
* Applications call this API to 'unhook' a hook.  First we check if someone
* is currently calling this hook.  If no one is we go ahead and free the
* HOOK structure now.  If someone is then we simply clear the filter-proc
* in the HOOK structure.  In xxxCallHook2() we check for this and if by
* that time no one is calling the hook in question we free it there.
*
* History:
* 01-28-91  DavidPe         Created.
\***************************************************************************/

BOOL zzzUnhookWindowsHookEx(
    PHOOK phkFree)
{
    PTHREADINFO pti;

    pti = GETPTI(phkFree);

    /*
     * If this hook is already destroyed, bail
     */
    if (phkFree->flags & HF_DESTROYED) {
        RIPMSG1(RIP_WARNING, "_UnhookWindowsHookEx(%#p) already destroyed", phkFree);
        return FALSE;
    }

    /*
     * Clear the journaling flags in all the queues.
     */
    if (abHookFlags[phkFree->iHook + 1] & HKF_JOURNAL) {
        zzzJournalAttach(pti, FALSE);
        /*
         * If someone got stuck because of the hook, let him go
         *
         * I want to get some performance numbers before checking this in.
         * MSTest hooks and unhooks all the time when running a script.
         * This code has never been in. 5/22/96. GerardoB
         */
        // InterQueueMsgCleanup(3 * CMSWAITTOKILLTIMEOUT);
    }

    /*
     * If no one is currently calling this hook,
     * go ahead and free it now.
     */
    FreeHook(phkFree);

    /*
     * If this thread has no more global hooks that are able to hook
     * server-side window procs, we must clear it's TIF_GLOBALHOOKER bit.
     */
    if (pti->TIF_flags & TIF_GLOBALHOOKER) {
        int iHook;
        PHOOK phk;
        for (iHook = WH_MIN ; iHook <= WH_MAX ; ++iHook) {
            /*
             * Ignore those that can't hook server-side winprocs
             */
            if (!(abHookFlags[iHook + 1] & HKF_INTERSENDABLE)) {
                continue;
            }

            /*
             * Scan the global hooks
             */
            for (phk = PhkFirstGlobalValid(pti, iHook);
                    phk != NULL; phk = PhkNextValid(phk)) {

                if (GETPTI(phk) == pti) {
                    goto StillHasGlobalHooks;
                }
            }
        }
        pti->TIF_flags &= ~TIF_GLOBALHOOKER;
    }

StillHasGlobalHooks:
    /*
     * Success, return TRUE.
     */
    return TRUE;
}


/***************************************************************************\
* _CallMsgFilter (API)
*
* CallMsgFilter() allows applications to call the WH_*MSGFILTER hooks.
* If there's a sysmodal window we return FALSE right away.  WH_MSGFILTER
* isn't called if WH_SYSMSGFILTER returned non-zero.
*
* History:
* 01-29-91  DavidPe         Created.
\***************************************************************************/

BOOL _CallMsgFilter(
    LPMSG pmsg,
    int nCode)
{
    PTHREADINFO pti;

    pti = PtiCurrent();

    /*
     * First call WH_SYSMSGFILTER.  If it returns non-zero, don't
     * bother calling WH_MSGFILTER, just return TRUE.  Otherwise
     * return what WH_MSGFILTER gives us.
     */
    if (IsHooked(pti, WHF_SYSMSGFILTER) && xxxCallHook(nCode, 0, (LPARAM)pmsg,
            WH_SYSMSGFILTER)) {
        return TRUE;
    }

    if (IsHooked(pti, WHF_MSGFILTER)) {
        return (BOOL)xxxCallHook(nCode, 0, (LPARAM)pmsg, WH_MSGFILTER);
    }

    return FALSE;
}


/***************************************************************************\
* xxxCallHook
*
* User code calls this function to call the first hook of a specific
* type.
*
* History:
* 01-29-91  DavidPe         Created.
\***************************************************************************/

int xxxCallHook(
    int nCode,
    WPARAM wParam,
    LPARAM lParam,
    int iHook)
{
    BOOL bAnsiHook;

    return (int)xxxCallHook2(PhkFirstValid(PtiCurrent(), iHook), nCode, wParam, lParam, &bAnsiHook);
}


/***************************************************************************\
* xxxCallHook2
*
* When you have an actual HOOK structure to call, you'd use this function.
* It will check to see if the hook hasn't already been unhooked, and if
* is it will free it and keep looking until it finds a hook it can call
* or hits the end of the list.  We also make sure any needed DLLs are loaded
* here.  We also check to see if the HOOK was unhooked inside the call
* after we return.
*
* Note: Hooking server-side window procedures (such as the desktop and console
* windows) can only be done by sending the hook message to the hooking app.
* (This is because we must not load the hookproc DLL into the server process).
* The hook types this can be done with are currently WH_JOURNALRECORD,
* WH_JOURNALPLAYBACK, WH_KEYBOARD and WH_MOUSE : these are all marked as
* HKF_INTERSENDABLE.  In order to prevent a global hooker from locking up the whole
* system, the hook message is sent with a timeout.  To ensure minimal
* performance degradation, the hooker process is set to foreground priority,
* and prevented from being set back to background priority with the
* TIF_GLOBALHOOKER bit in hooking thread's pti->flags.
* Hooking emulated DOS apps is prevented with the TIF_DOSEMULATOR bit in the
* console thread: this is because these apps typically hog the CPU so much that
* the hooking app does not respond rapidly enough to the hook messsages sent
* to it.  IanJa Nov 1994.
*
* History:
* 02-07-91     DavidPe     Created.
* 1994 Nov 02  IanJa       Hooking desktop and console windows.
\***************************************************************************/

LRESULT xxxCallHook2(
    PHOOK phkCall,
    int nCode,
    WPARAM wParam,
    LPARAM lParam,
    LPBOOL lpbAnsiHook)
{
    UINT        iHook;
    PHOOK       phkSave;
    LONG_PTR     nRet;
    PTHREADINFO ptiCurrent;
    BOOL        fLoadSuccess;
    TL          tlphkCall;
    TL          tlphkSave;
    BYTE        bHookFlags;
    BOOL        fMustIntersend;

    CheckCritIn();

    if (phkCall == NULL) {
        return 0;
    }

    iHook = phkCall->iHook;

    ptiCurrent = PtiCurrent();
    /*
     * Only low level hooks are allowed in the RIT context
     * (This check used to be done in PhkFirstValid).
     */
    if (ptiCurrent == gptiRit) {
        switch (iHook) {
        case WH_MOUSE_LL:
        case WH_KEYBOARD_LL:

#ifdef REDIRECTION
        case WH_HITTEST:
#endif // REDIRECTION

            break;

        default:
            return 0;
        }
    }

    /*
     * If this queue is in cleanup, exit: it has no business calling back
     * a hook proc. Also check if hooks are disabled for the thread.
     */
    if (    ptiCurrent->TIF_flags & (TIF_INCLEANUP | TIF_DISABLEHOOKS) ||
            ((ptiCurrent->rpdesk == NULL) && (phkCall->iHook != WH_MOUSE_LL))) {
        return ampiHookError[iHook + 1];
    }

    /*
     * Try to call each hook in the list until one is successful or
     * we reach the end of the list.
     */
    do {
        *lpbAnsiHook = phkCall->flags & HF_ANSI;
        bHookFlags = abHookFlags[phkCall->iHook + 1];

        /*
         * Some WH_SHELL hook types can be called from console
         * HSHELL_APPCOMMAND added for bug 346575 DefWindowProc invokes a shell hook
         * for console windows if they don't handle the wm_appcommand message - we need the hook
         * to go through for csrss.
         */
        if ((phkCall->iHook == WH_SHELL) && (ptiCurrent->TIF_flags & TIF_CSRSSTHREAD)) {
            if ((nCode == HSHELL_LANGUAGE) || (nCode == HSHELL_WINDOWACTIVATED) ||
                (nCode == HSHELL_APPCOMMAND)) {
                bHookFlags |= HKF_INTERSENDABLE;
            }
        }

        if ((phkCall->iHook == WH_SHELL) && (ptiCurrent->TIF_flags & TIF_SYSTEMTHREAD)) {
            if ((nCode == HSHELL_ACCESSIBILITYSTATE) ) {
                bHookFlags |= HKF_INTERSENDABLE;
            }
        }

        fMustIntersend =
            (GETPTI(phkCall) != ptiCurrent) &&
            (
                /*
                 * We always want to intersend journal hooks.
                 * CONSIDER (adams): Why? There's a performance hit by
                 * doing so, so if we haven't a reason, we shouldn't
                 * do it.
                 *
                 * we also need to intersend low level hooks. They can be called
                 * from the desktop thread, the raw input thread AND also from
                 * any thread that calls CallNextHookEx.
                 */
                (bHookFlags & (HKF_JOURNAL | HKF_LOWLEVEL))

                /*
                 * We must intersend if a 16bit app hooks a 32bit app
                 * because we can't load a 16bit dll into a 32bit process.
                 * We must also intersend if a 16bit app hooks another 16bit app
                 * in a different VDM, because we can't load a 16bit dll from
                 * one VDM into a 16bit app in another VDM (because that
                 * VDM is actually a 32bit process).
                 */
                ||
                (   GETPTI(phkCall)->TIF_flags & TIF_16BIT &&
                    (   !(ptiCurrent->TIF_flags & TIF_16BIT) ||
                        ptiCurrent->ppi != GETPTI(phkCall)->ppi))

#if defined(_WIN64)

                /*
                 * Intersend if a 64bit app hooks a 32bit app or
                 * a 32bit app hooks a 64bit app.
                 * This is necessary since a hook DLL can not be loaded
                 * cross bit type.
                 */
                ||
                (   (GETPTI(phkCall)->TIF_flags & TIF_WOW64) !=
                    (ptiCurrent->TIF_flags & TIF_WOW64)
                )

#endif /* defined(_WIN64) */

                /*
                 * We must intersend if a console or system thread is calling a hook
                 * that is not in the same console or the system process.
                 */
                ||
                (   ptiCurrent->TIF_flags & (TIF_CSRSSTHREAD | TIF_SYSTEMTHREAD) &&
                    GETPTI(phkCall)->ppi != ptiCurrent->ppi)

                /*
                 * If this is a global and non-journal hook, do a security
                 * check on the current desktop to see if we can call here.
                 * Note that we allow processes with the SYSTEM_LUID to hook
                 * other processes even if the other process says that it
                 * doesn't allow other accounts to hook them.  We did this
                 * because there was a bug in NT 3.x that allowed it and some
                 * services were written to use it.
                 */
                ||
                (   phkCall->flags & HF_GLOBAL &&
                    !RtlEqualLuid(&GETPTI(phkCall)->ppi->luidSession, &ptiCurrent->ppi->luidSession) &&
                    !(ptiCurrent->TIF_flags & TIF_ALLOWOTHERACCOUNTHOOK) &&
                    !RtlEqualLuid(&GETPTI(phkCall)->ppi->luidSession, &luidSystem))

                /*
                 * We must intersend if the hooking thread is running in
                 * another process and is restricted.
                 */
                ||
                (   GETPTI(phkCall)->ppi != ptiCurrent->ppi &&
                    IsRestricted(GETPTI(phkCall)->pEThread))
             );

        /*
         * We're calling back... make sure the hook doesn't go away while
         * we're calling back. We've thread locked here: we must unlock before
         * returning or enumerating the next hook in the chain.
         */
        ThreadLockAlwaysWithPti(ptiCurrent, phkCall, &tlphkCall);

        if (!fMustIntersend) {
            /*
             * Make sure the DLL for this hook, if any, has been loaded
             * for the current process.
             */
            if ((phkCall->ihmod != -1) &&
                    (TESTHMODLOADED(ptiCurrent, phkCall->ihmod) == 0)) {

                BOOL bWx86KnownDll;

                /*
                 * Try loading the library, since it isn't loaded in this processes
                 * context.  First lock this hook so it doesn't go away while we're
                 * loading this library.
                 */
                bWx86KnownDll = (phkCall->flags & HF_WX86KNOWNDLL) != 0;
                fLoadSuccess = (xxxLoadHmodIndex(phkCall->ihmod, bWx86KnownDll) != NULL);

                /*
                 * If the LoadLibrary() failed, skip to the next hook and try
                 * again.
                 */
                if (!fLoadSuccess) {
                    goto LoopAgain;
                }
            }

            /*
             * Is WH_DEBUG installed?  If we're not already calling it, do so.
             */
            if (IsHooked(ptiCurrent, WHF_DEBUG) && (phkCall->iHook != WH_DEBUG)) {
                DEBUGHOOKINFO debug;

                debug.idThread = TIDq(ptiCurrent);
                debug.idThreadInstaller = 0;
                debug.code = nCode;
                debug.wParam = wParam;
                debug.lParam = lParam;

                if (xxxCallHook(HC_ACTION, phkCall->iHook, (LPARAM)&debug, WH_DEBUG)) {
                    /*
                     * If WH_DEBUG returned non-zero, skip this hook and
                     * try the next one.
                     */
                    goto LoopAgain;
                }
            }

            /*
             * Make sure the hook is still around before we
             * try and call it.
             */
            if (HMIsMarkDestroy(phkCall)) {
                goto LoopAgain;
            }

            /*
             * Time to call the hook! Lock it first so that it doesn't go away
             * while we're using it. Thread lock right away in case the lock frees
             * the previous contents.
             */

#if DBG
            if (phkCall->flags & HF_GLOBAL) {
                UserAssert(phkCall->ptiHooked == NULL);
            } else {
                UserAssert(phkCall->ptiHooked == ptiCurrent);
            }
#endif
            phkSave = ptiCurrent->sphkCurrent;
            ThreadLockWithPti(ptiCurrent, phkSave, &tlphkSave);

            Lock(&ptiCurrent->sphkCurrent, phkCall);
            if (ptiCurrent->pClientInfo)
                ptiCurrent->pClientInfo->phkCurrent = phkCall;

            nRet = xxxHkCallHook(phkCall, nCode, wParam, lParam);

            Lock(&ptiCurrent->sphkCurrent, phkSave);
            if (ptiCurrent->pClientInfo)
                ptiCurrent->pClientInfo->phkCurrent = phkSave;

            ThreadUnlock(&tlphkSave);

            /*
             * This hook proc faulted, so unhook it and try the next one.
             */
            if (phkCall->flags & HF_HOOKFAULTED) {
                PHOOK   phkFault;

                phkCall = PhkNextValid(phkCall);
                phkFault = ThreadUnlock(&tlphkCall);
                if (phkFault != NULL) {
                    FreeHook(phkFault);
                }

                continue;
            }

            /*
             * Lastly, we're done with this hook so it is ok to unlock it (it may
             * get freed here!
             */
            ThreadUnlock(&tlphkCall);

            return nRet;

        } else if (bHookFlags & HKF_INTERSENDABLE) {

            /*
             * Receiving thread can access this structure since the
             * sender thread's stack is locked down during xxxInterSendMsgEx
             */
            HOOKMSGSTRUCT hkmp;
            int           timeout = 200; // 1/5 second !!!

            hkmp.lParam = lParam;
            hkmp.phk = phkCall;
            hkmp.nCode = nCode;

            /*
             * Thread lock right away in case the lock frees the previous contents
             */
            phkSave = ptiCurrent->sphkCurrent;

            ThreadLockWithPti(ptiCurrent, phkSave, &tlphkSave);

            Lock(&ptiCurrent->sphkCurrent, phkCall);
            if (ptiCurrent->pClientInfo)
                ptiCurrent->pClientInfo->phkCurrent = phkCall;

            /*
             * Make sure we don't get hung!
             */
            if (bHookFlags & HKF_LOWLEVEL)
                timeout = gnllHooksTimeout;

            /*
             * CONSIDER(adams): Why should a journaling hook be allowed to
             * hang the console or a system thread? Will that interfere with
             * the user's ability to cancel journaling through Ctrl+Esc?
             */
            if (((bHookFlags & HKF_LOWLEVEL) == 0) &&
                (   (bHookFlags & HKF_JOURNAL) ||
                    !(ptiCurrent->TIF_flags & (TIF_CSRSSTHREAD | TIF_SYSTEMTHREAD)))) {

                nRet = xxxInterSendMsgEx(NULL, WM_HOOKMSG, wParam,
                    (LPARAM)&hkmp, ptiCurrent, GETPTI(phkCall), NULL);
            } else {
                /*
                 * We are a server thread (console/desktop) and we aren't
                 * journalling, so we can't allow the hookproc to hang us -
                 * we must use a timeout.
                 */
                INTRSENDMSGEX ism;

                ism.fuCall     = ISM_TIMEOUT;
                ism.fuSend     = SMTO_ABORTIFHUNG | SMTO_NORMAL;
                ism.uTimeout   = timeout;
                ism.lpdwResult = &nRet;

                /*
                 * Don't hook DOS apps connected to the emulator - they often
                 * grab too much CPU for the callback to the hookproc to
                 * complete in a timely fashion, causing poor response.
                 */
                if ((ptiCurrent->TIF_flags & TIF_DOSEMULATOR) ||
                    FHungApp(GETPTI(phkCall), CMSHUNGAPPTIMEOUT) ||
                    !xxxInterSendMsgEx(NULL, WM_HOOKMSG, wParam,
                            (LPARAM)&hkmp, ptiCurrent, GETPTI(phkCall), &ism)) {
                    nRet = ampiHookError[iHook + 1];
                }

                /*
                 * If the low-level hook is eaten, the app may wake up from
                 * MsgWaitForMultipleObjects, clear the wake mask, but not get
                 * anything in GetMessage / PeekMessage and we will think it's
                 * hung. This causes problems in DirectInput because then the
                 * app may miss some hooks if FHungApp returns true, see bug
                 * 430342 for more details on this.
                 */
                if ((bHookFlags & HKF_LOWLEVEL) && nRet) {
                    SET_TIME_LAST_READ(GETPTI(phkCall));
                }
            }

            Lock(&ptiCurrent->sphkCurrent, phkSave);
            if (ptiCurrent->pClientInfo)
                ptiCurrent->pClientInfo->phkCurrent = phkSave;

            ThreadUnlock(&tlphkSave);
            ThreadUnlock(&tlphkCall);
            return nRet;
        }
        // fall-through

LoopAgain:
        phkCall = PhkNextValid(phkCall);
        ThreadUnlock(&tlphkCall);
    } while (phkCall != NULL);

    return ampiHookError[iHook + 1];
}

/***************************************************************************\
* xxxCallMouseHook
*
* This is a helper routine that packages up a MOUSEHOOKSTRUCTEX and calls
* the WH_MOUSE hook.
*
* History:
* 02-09-91  DavidPe         Created.
\***************************************************************************/

BOOL xxxCallMouseHook(
    UINT message,
    PMOUSEHOOKSTRUCTEX pmhs,
    BOOL fRemove)
{
    BOOL bAnsiHook;

    /*
     * Call the mouse hook.
     */
    if (xxxCallHook2(PhkFirstValid(PtiCurrent(), WH_MOUSE), fRemove ?
            HC_ACTION : HC_NOREMOVE, (DWORD)message, (LPARAM)pmhs, &bAnsiHook)) {
        return TRUE;
    }

    return FALSE;
}


/***************************************************************************\
* xxxCallJournalRecordHook
*
* This is a helper routine that packages up an EVENTMSG and calls
* the WH_JOURNALRECORD hook.
*
* History:
* 02-28-91  DavidPe         Created.
\***************************************************************************/

void xxxCallJournalRecordHook(
    PQMSG pqmsg)
{
    EVENTMSG emsg;
    BOOL bAnsiHook;

    /*
     * Setup the EVENTMSG structure.
     */
    emsg.message = pqmsg->msg.message;
    emsg.time = pqmsg->msg.time;

    if (RevalidateHwnd(pqmsg->msg.hwnd)) {
        emsg.hwnd = pqmsg->msg.hwnd;
    } else {
        emsg.hwnd = NULL;
    }

    if ((emsg.message >= WM_MOUSEFIRST) && (emsg.message <= WM_MOUSELAST)) {
        emsg.paramL = (UINT)pqmsg->msg.pt.x;
        emsg.paramH = (UINT)pqmsg->msg.pt.y;

    } else if ((emsg.message >= WM_KEYFIRST) && (emsg.message <= WM_KEYLAST)) {
        BYTE bScanCode = LOBYTE(HIWORD(pqmsg->msg.lParam));
        /*
         * Build up a Win 3.1 compatible journal record key
         * Win 3.1  ParamL 00 00 SC VK  (SC=scan code VK=virtual key)
         * Also set ParamH 00 00 00 SC  to be compatible with our Playback
         *
         * If WM_*CHAR messages ever come this way we would have a problem
         * because we would lose the top byte of the Unicode character. We'd
         * We'd get ParamL 00 00 SC CH  (SC=scan code, CH = low byte of WCHAR)
         *
         */
        if ((LOWORD(pqmsg->msg.wParam) == VK_PACKET) && (bScanCode == 0)) {
            /*
             * If we have an injected Unicode char (from SendInput), the
             * character value was cached, let's give that to them too.
             */
            emsg.paramL = (UINT)MAKELONG(pqmsg->msg.wParam, PtiCurrent()->wchInjected);
        } else {
            emsg.paramL = MAKELONG(MAKEWORD(pqmsg->msg.wParam, bScanCode),0);
        }
        emsg.paramH = bScanCode;

        UserAssert((emsg.message != WM_CHAR) &&
                   (emsg.message != WM_DEADCHAR) &&
                   (emsg.message != WM_SYSCHAR) &&
                   (emsg.message != WM_SYSDEADCHAR));
        /*
         * Set extended-key bit.
         */
        if (pqmsg->msg.lParam & 0x01000000) {
            emsg.paramH |= 0x8000;
        }

    } else {
        RIPMSG2(RIP_WARNING,
                "Bad journal record message!\n"
                "   message  = 0x%08lx\n"
                "   dwQEvent = 0x%08lx",
                pqmsg->msg.message,
                pqmsg->dwQEvent);
    }

    /*
     * Call the journal recording hook.
     */
    xxxCallHook2(PhkFirstGlobalValid(PtiCurrent(), WH_JOURNALRECORD), HC_ACTION, 0,
            (LPARAM)&emsg, &bAnsiHook);

    /*
     * Write the MSG parameters back because the app may have modified it.
     * AfterDark's screen saver password actually zero's out the keydown
     * chars.
     *
     * If it was a mouse message patch up the mouse point.  If it was a
     * WM_KEYxxx message convert the Win 3.1 compatible journal record key
     * back into a half backed WM_KEYxxx format.  Only the VK and SC fields
     * where initialized at this point.
     *
     *      wParam  00 00 00 VK   lParam 00 SC 00 00
     */
    if ((pqmsg->msg.message >= WM_MOUSEFIRST) && (pqmsg->msg.message <= WM_MOUSELAST)) {
        pqmsg->msg.pt.x = emsg.paramL;
        pqmsg->msg.pt.y = emsg.paramH;

    } else if ((pqmsg->msg.message >= WM_KEYFIRST) && (pqmsg->msg.message <= WM_KEYLAST)) {
        (BYTE)pqmsg->msg.wParam = (BYTE)emsg.paramL;
        ((PBYTE)&pqmsg->msg.lParam)[2] = HIBYTE(LOWORD(emsg.paramL));
    }
}


/***************************************************************************\
* xxxCallJournalPlaybackHook
*
*
* History:
* 03-01-91  DavidPe         Created.
\***************************************************************************/

DWORD xxxCallJournalPlaybackHook(
    PQMSG pqmsg)
{
    EVENTMSG emsg;
    LONG dt;
    PWND pwnd;
    WPARAM wParam;
    LPARAM lParam;
    POINT pt;
    PTHREADINFO ptiCurrent;
    BOOL bAnsiHook;
    PHOOK phkCall;
    TL tlphkCall;

    UserAssert(IsWinEventNotifyDeferredOK());

TryNextEvent:

    /*
     * Initialized to the current time for compatibility with
     * <= 3.0.
     */
    emsg.time = NtGetTickCount();
    ptiCurrent = PtiCurrent();
    pwnd = NULL;

    phkCall = PhkFirstGlobalValid(ptiCurrent, WH_JOURNALPLAYBACK);
    ThreadLockWithPti(ptiCurrent, phkCall, &tlphkCall);

    dt = (DWORD)xxxCallHook2(phkCall, HC_GETNEXT, 0, (LPARAM)&emsg, &bAnsiHook);

    /*
     * -1 means some error occured. Return -1 for error.
     */
    if (dt == 0xFFFFFFFF) {
        ThreadUnlock(&tlphkCall);
        return dt;
    }

    /*
     * Update the message id. Need this if we decide to sleep.
     */
    pqmsg->msg.message = emsg.message;

    if (dt > 0) {
        if (ptiCurrent->TIF_flags & TIF_IGNOREPLAYBACKDELAY) {
            /*
             * This flag tells us to ignore the requested delay (set in mnloop)
             * We clear it to indicate that we did so.
             */
            RIPMSG1(RIP_WARNING, "Journal Playback delay ignored (%lx)", emsg.message);
            ptiCurrent->TIF_flags &= ~TIF_IGNOREPLAYBACKDELAY;
            dt = 0;
        } else {
            ThreadUnlock(&tlphkCall);
            return dt;
        }
    }

    /*
     * The app is ready to be asked for the next event
     */

    if ((emsg.message >= WM_MOUSEFIRST) && (emsg.message <= WM_MOUSELAST)) {

        pt.x = (int)emsg.paramL;
        pt.y = (int)emsg.paramH;

        lParam = MAKELONG(LOWORD(pt.x), LOWORD(pt.y));
        wParam = 0;

        /*
         * If the message has changed the mouse position,
         * update the cursor.
         */
        if (pt.x != gpsi->ptCursor.x || pt.y != gpsi->ptCursor.y) {
            zzzInternalSetCursorPos(pt.x, pt.y);
        }

    } else if ((emsg.message >= WM_KEYFIRST) && (emsg.message <= WM_KEYLAST)) {
        UINT wExtraStuff = 0;

        if ((emsg.message == WM_KEYUP) || (emsg.message == WM_SYSKEYUP)) {
            wExtraStuff |= 0x8000;
        }

        if ((emsg.message == WM_SYSKEYUP) || (emsg.message == WM_SYSKEYDOWN)) {
            wExtraStuff |= 0x2000;
        }

        if (emsg.paramH & 0x8000) {
            wExtraStuff |= 0x0100;
        }

        if (TestKeyStateDown(ptiCurrent->pq, (BYTE)emsg.paramL)) {
            wExtraStuff |= 0x4000;
        }
        lParam = MAKELONG(1, (UINT)((emsg.paramH & 0xFF) | wExtraStuff));

        if ((LOWORD(emsg.paramL) == VK_PACKET) && (LOBYTE(emsg.paramH) == 0)) {
            /*
             * We are playing back an injected Unicode char (see SendInput)
             * save the character for TranslateMessage to pick up.
             */
            ptiCurrent->wchInjected = HIWORD(emsg.paramL);
        } else {
            /*
             * Raid# 65331
             * WM_KEY* and WM_SYSKEY* messages should only contain 8bit Virtual Keys.
             * Some applications passes scan code in HIBYTE and could screw up
             * the system. E.g. Tab Keydown, paramL: 0x0f09 where 0f is scan code
             */
            DWORD dwMask = 0xff;

            /*
             * There are old ANSI apps that only fill in the byte for when
             * they generate journal playback so we used to strip everything
             * else off.  That however breaks unicode journalling; 22645
             * (Yes, some apps apparently do Playback WM_*CHAR msgs!)
             *
             */
            if (!bAnsiHook || IS_DBCS_ENABLED()) {
                if (IS_CHAR_MSG(emsg.message)) {
                    RIPMSG1(RIP_VERBOSE, "Unusual char message(%x) passed through JournalPlayback.", emsg.message);
                    /*
                     * Don't mask off HIBYTE(LOWORD(paramL)) for DBCS and UNICODE.
                     */
                    dwMask = 0xffff;
                }
            }

            wParam = emsg.paramL & dwMask;
        }

    } else if (emsg.message == WM_QUEUESYNC) {
        if (emsg.paramL == 0) {
            pwnd = ptiCurrent->pq->spwndActive;
        } else {
            if ((pwnd = RevalidateHwnd((HWND)IntToPtr( emsg.paramL ))) == NULL)
                pwnd = ptiCurrent->pq->spwndActive;
        }

    } else {
        /*
         * This event doesn't match up with what we're looking
         * for. If the hook is still valid, then skip this message
         * and try the next.
         */
        if (phkCall == NULL || phkCall->offPfn == 0L) {
            /* Hook is nolonger valid, return -1 */
            ThreadUnlock(&tlphkCall);
            return 0xFFFFFFFF;
        }

        RIPMSG1(RIP_WARNING,
                "Bad journal playback message=0x%08lx",
                emsg.message);

        xxxCallHook(HC_SKIP, 0, 0, WH_JOURNALPLAYBACK);
        ThreadUnlock(&tlphkCall);
        goto TryNextEvent;
    }

    StoreQMessage(pqmsg, pwnd, emsg.message, wParam, lParam, 0, 0, 0);

    ThreadUnlock(&tlphkCall);
    return 0;
}

/***************************************************************************\
* FreeHook
*
* Free hook unlinks the HOOK structure from its hook-list and removes
* any hmod dependencies on this hook.  It also frees the HOOK structure.
*
* History:
* 01-31-91  DavidPe         Created.
\***************************************************************************/

VOID FreeHook(
    PHOOK phkFree)
{
    /*
     * Paranoia...
     */
    UserAssert(!(phkFree->flags & HF_FREED));
    /*
     * BUGBUG
     * Unless we come from zzzUnhookWindowsHook, if this was a journaling hook
     * we don't unattach the threads.  We should reconsider this one day.
     * MCostea Jan 21, 99
     */

    /*
     * Clear fsHooks bits the first time around (and mark it as destroyed).
     */
    if (!(phkFree->flags & HF_DESTROYED)) {
        DbgValidateHooks (phkFree, phkFree->iHook);
        phkFree->flags |= HF_DESTROYED;
        /*
         * This hook has been marked as destroyed so CheckWHSBits
         * won't take it into account when updating the fsHooks bits.
         * However, this means that right at this moment fsHooks is
         * out of sync. So we need a flag to make the assertion freaks
         * (i.e., me) happy.
         */
#if DBG
        phkFree->flags |= HF_INCHECKWHF;
#endif
        UserAssert((phkFree->ptiHooked != NULL) || (phkFree->flags & HF_GLOBAL));
        CheckWHFBits(phkFree->ptiHooked != NULL
                        ? phkFree->ptiHooked
                        : GETPTI(phkFree),
                     phkFree->iHook);
#if DBG
        phkFree->flags &= ~HF_INCHECKWHF;
#endif
    }
    /*
     * Mark it for destruction.  If it the object is locked it can't
     * be freed right now.
     */
    if (!HMMarkObjectDestroy((PVOID)phkFree)) {
        return;
    }
    /*
     * We're going to free this hook so get it off the list.
     */
    UnlinkHook(phkFree);
    /*
     * Now remove the hmod dependency and free the
     * HOOK structure.
     */
    if (phkFree->ihmod >= 0) {
        RemoveHmodDependency(phkFree->ihmod);
    }

#ifdef HOOKBATCH
    /*
     * Free the cached Events
     */
    if (phkFree->aEventCache) {
        UserFreePool(phkFree->aEventCache);
        phkFree->aEventCache = NULL;
    }
#endif //HOOKBATCH

#if DBG
    phkFree->flags |= HF_FREED;
#endif

    HMFreeObject((PVOID)phkFree);
    return;
}
/***************************************************************************\
* UnlinkHook
*
* Gets a hook out of its chain. Note that FreeThreadsWindowHooks unlinks
*  some hooks but don't free them. So this function doesn't assume that
*  the hook is going away.
*
* History:
* 04-25-97  GerardoB    Added Header
\***************************************************************************/
void UnlinkHook(
    PHOOK phkFree)
{
    PHOOK *pphkNext;
    PTHREADINFO ptiT;

    CheckCritIn();
    /*
     * Since we have the HOOK structure, we can tell if this a global
     * or local hook and start on the right list.
     */
    if (phkFree->flags & HF_GLOBAL) {
        pphkNext = &GETPTI(phkFree)->pDeskInfo->aphkStart[phkFree->iHook + 1];
    } else {
        ptiT = phkFree->ptiHooked;
        if (ptiT == NULL) {
            /*
             * Already unlinked (by FreeThreadsWindowHooks)
             */
            return;
        } else {
            /*
             * Clear ptiHooked so we won't try to unlink it again.
             */
            phkFree->ptiHooked = NULL;
        }
        pphkNext = &(ptiT->aphkStart[phkFree->iHook + 1]);
        /*
         * There must be at least one hook in the chain
         */
        UserAssert(*pphkNext != NULL);
    }
    /*
     * Find the address of the phkNext pointing to phkFree
     */
    while ((*pphkNext != phkFree) && (*pphkNext != NULL)) {
       pphkNext = &(*pphkNext)->phkNext;
    }
    /*
     * If we haven't found it, it must be global hook whose owner is gone or
     *  has switched desktops.
     */
    if (*pphkNext == NULL) {
        UserAssert(phkFree->flags & HF_GLOBAL);
        /*
         * if we saved a pdesk, use it. Else use the one we allocated it from
         */
        if (phkFree->rpdesk != NULL) {
            UserAssert(GETPTI(phkFree) == gptiRit);
            UserAssert(phkFree->rpdesk != NULL);
            UserAssert(phkFree->rpdesk->pDeskInfo != gptiRit->pDeskInfo);

            pphkNext = &phkFree->rpdesk->pDeskInfo->aphkStart[phkFree->iHook + 1];
        } else {
            UserAssert(GETPTI(phkFree)->pDeskInfo != phkFree->head.rpdesk->pDeskInfo);
            pphkNext = &phkFree->head.rpdesk->pDeskInfo->aphkStart[phkFree->iHook + 1];
        }

        UserAssert(*pphkNext != NULL);
        while ((*pphkNext != phkFree) && (*pphkNext != NULL)) {
           pphkNext = &(*pphkNext)->phkNext;
        }
    }
    /*
     * We're supposed to find it
     */
    UserAssert(*pphkNext == phkFree);
    /*
     * Unlink it
     */
    *pphkNext = phkFree->phkNext;
    phkFree->phkNext = NULL;
    /*
     * If we had a desktop, unlock it
     */
    if (phkFree->rpdesk != NULL) {
        UserAssert(phkFree->flags & HF_GLOBAL);
        UserAssert(GETPTI(phkFree) == gptiRit);
        UnlockDesktop(&phkFree->rpdesk, LDU_HOOK_DESK, 0);
    }
}

/***************************************************************************\
* PhkFirstGlobalValid
*
* Returns the first not-destroyed hook on the given desktop info.
*
* History:
* 03/24/97 GerardoB Created
\***************************************************************************/
PHOOK PhkFirstGlobalValid(PTHREADINFO pti, int nFilterType)
{
    PHOOK phk;

    CheckCritIn();
    phk = pti->pDeskInfo->aphkStart[nFilterType + 1];
    /*
     * Return the first hook that it's not destroyed (i.e, the
     *  first valid one).
     */
    if ((phk != NULL) && (phk->flags & HF_DESTROYED)) {
        phk = PhkNextValid(phk);
    }
    /*
     * Good place to check fsHooks. If the bits are out of sync,
     *  someone must be adjusting them.
     */
    DbgValidatefsHook(phk, nFilterType, pti, TRUE);
    DbgValidateHooks(phk, nFilterType);
    return phk;
}

/***************************************************************************\
* PhkFirstValid
*
* Given a filter-type PhkFirstValid() returns the first hook, if any, of the
* specified type.
*
* History:
* 02-10-91  DavidPe         Created.
\***************************************************************************/

PHOOK PhkFirstValid(
    PTHREADINFO pti,
    int nFilterType)
{
    PHOOK phk;
    CheckCritIn();
    /*
     * Grab the first hook off the local hook-list
     * for the current queue.
     */
    phk = pti->aphkStart[nFilterType + 1];
    /*
     * If there aren't any local hooks, try the global hooks.
     */
    if (phk == NULL) {
        phk = pti->pDeskInfo->aphkStart[nFilterType + 1];
    }
    /*
     * Return the first hook that it's not destroyed (i.e, the
     *  first valid one).
     */
    if ((phk != NULL) && (phk->flags & HF_DESTROYED)) {
        phk = PhkNextValid(phk);
    }
    /*
     * Good place to check fsHooks. If the bits are out of sync,
     *  someone must be adjusting them.
     */

    DbgValidatefsHook(phk, nFilterType, pti, FALSE);
    DbgValidateHooks(phk, nFilterType);
    return phk;
}

/***************************************************************************\
* FreeThreadsWindowHooks
*
* During 'exit-list' processing this function is called to free any hooks
* created on, or set for the current queue.
*
* History:
* 02-10-91  DavidPe         Created.
\***************************************************************************/

VOID FreeThreadsWindowHooks(VOID)
{
    int iHook;
    PHOOK phk, phkNext;
    PTHREADINFO ptiCurrent = PtiCurrent();

    /*
     * If there is not thread info, there are not hooks to worry about
     */
    if (ptiCurrent == NULL || ptiCurrent->rpdesk == NULL) {
        return;
    }
    /*
     * In case we have a hook locked in as the current hook unlock it
     * so it can be freed
     */
    Unlock(&ptiCurrent->sphkCurrent);

    UserAssert(ptiCurrent->TIF_flags & TIF_INCLEANUP);
    // Why bother doing this? We won't be calling back to user mode again!
    // if (ptiCurrent->pClientInfo) {
    //    ptiCurrent->pClientInfo->phkCurrent = NULL;
    // }

    /*
     * Loop through all the hook types.
     */
    for (iHook = WH_MIN ; iHook <= WH_MAX ; ++iHook) {
        /*
         * Loop through all the hooks of this type, including the
         *  ones already marked as destroyed (so don't call
         *  PhkFirstValid and PhkNextValid).
         */
        phk = ptiCurrent->aphkStart[iHook + 1];
        if (phk == NULL) {
            phk = ptiCurrent->pDeskInfo->aphkStart[iHook + 1];
            UserAssert((phk == NULL) || (phk->flags & HF_GLOBAL));
        }

        while (phk != NULL) {
            /*
             * We might free phk below, so grab the next now
             * If at end of local chain, jump to the global chain
             */
            phkNext = phk->phkNext;
            if ((phkNext == NULL) && !(phk->flags & HF_GLOBAL)) {
                phkNext = ptiCurrent->pDeskInfo->aphkStart[iHook + 1];
                UserAssert((phkNext == NULL) || (phkNext->flags & HF_GLOBAL));
            }
            /*
             * If this is a local(thread) hook, unlink it and mark it as
             *  destroyed so we won't call it anymore. We want to do
             *  this even if not calling FreeHook; also note that
             *  FreeHook won't unlink it if locked so we do it here anyway.
             */
            if (!(phk->flags & HF_GLOBAL)) {
                UserAssert(ptiCurrent == phk->ptiHooked);
                UnlinkHook(phk);
                phk->flags |= HF_DESTROYED;
                phk->phkNext = NULL;
            }
            /*
             * If this hook was created by this thread, free it
             */
            if (GETPTI(phk) == ptiCurrent) {
                FreeHook(phk);
            }

            phk = phkNext;
        }
       /*
        * All local hooks should be unlinked
        */
       UserAssert(ptiCurrent->aphkStart[iHook + 1] == NULL);
    } /* for (iHook = WH_MIN....*/

    /*
     * Keep fsHooks in sync.
     */
    ptiCurrent->fsHooks = 0;
}

/***************************************************************************\
* zzzRegisterSystemThread: Private API
*
*  Used to set various attributes pertaining to a thread.
*
* History:
* 21-Jun-1994 from Chicago Created.
\***************************************************************************/

VOID zzzRegisterSystemThread (DWORD dwFlags, DWORD dwReserved)
{
    PTHREADINFO ptiCurrent;

    UserAssert(dwReserved == 0);

    if (dwReserved != 0)
        return;

    ptiCurrent = PtiCurrent();

    if (dwFlags & RST_DONTATTACHQUEUE)
        ptiCurrent->TIF_flags |= TIF_DONTATTACHQUEUE;

    if (dwFlags & RST_DONTJOURNALATTACH) {
        ptiCurrent->TIF_flags |= TIF_DONTJOURNALATTACH;

        /*
         * If we are already journaling, then this queue was already
         * journal attached.  We need to unattach and reattach journaling
         * so that we are removed from the journal attached queues.
         */
        if (FJOURNALPLAYBACK() || FJOURNALRECORD()) {
            zzzJournalAttach(ptiCurrent, FALSE);
            zzzJournalAttach(ptiCurrent, TRUE);
        }
    }
}

#ifdef REDIRECTION

/***************************************************************************\
* xxxGetCursorPos
*
\***************************************************************************/

BOOL
xxxGetCursorPos(
    LPPOINT lpPt)
{
    POINT pt;

    CheckCritIn();

    /*
     * If there is no CBT hook installed bail out.
     */
    if (!IsHooked(PtiCurrent(), WHF_CBT))
        return TRUE;

    try {
        ProbeForWrite(lpPt, sizeof(RECT), DATAALIGN);
        RtlCopyMemory(&pt, lpPt, sizeof(RECT));
    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
        return FALSE;
    }

    xxxCallHook(HCBT_GETCURSORPOS, 0, (LPARAM)&pt, WH_CBT);

    try {
        RtlCopyMemory(lpPt, &pt, sizeof(RECT));
    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
        return FALSE;
    }

    return TRUE;
}

#endif // REDIRECTION

