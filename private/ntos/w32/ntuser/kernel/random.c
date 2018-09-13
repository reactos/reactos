/****************************** Module Header ******************************\
* Module Name: random.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains a random collection of support routines for the User
* API functions.  Many of these functions will be moved to more appropriate
* files once we get our act together.
*
* History:
* 10-17-90 DarrinM      Created.
* 02-06-91 IanJa        HWND revalidation added (none required)
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************\
* xxxUpdateWindows
*
* User mode wrapper
\***************************************************************************/

BOOL xxxUpdateWindows(PWND pwnd, HRGN hrgn)
{
    CheckLock(pwnd);

    xxxUpdateThreadsWindows(PtiCurrent(), pwnd, hrgn);

    /*
     * This function needs to return a value, since it is
     * called through NtUserCallHwndParam.
     */
    return TRUE;
}

/***************************************************************************\
* ValidateState
*
* States allowed to be set/cleared by Set/ClearWindowState. If you're
* allowing a new flag here, you must make sure it won't cause an AV
* in the kernel if someone sets it maliciously.
\***************************************************************************/

#define NUM_BYTES 16  // Window state bytes are 0 to F, explanation in user.h

CONST BYTE abValidateState[NUM_BYTES] = {
    0,      // 0
    0,      // 1
    0,      // 2
    0,      // 3
    0,      // 4
    LOBYTE(WFWIN40COMPAT),
    0,      // 6
    LOBYTE(WFNOANIMATE),
    0,      // 8
    LOBYTE(WEFEDGEMASK),
    LOBYTE(WEFSTATICEDGE),
    0,      // B
    LOBYTE(EFPASSWORD),
    LOBYTE(CBFHASSTRINGS | EFREADONLY),
    LOBYTE(WFTABSTOP | WFSYSMENU | WFVSCROLL | WFHSCROLL | WFBORDER),
    LOBYTE(WFCLIPCHILDREN)
};

BOOL ValidateState(DWORD dwFlags)
{
    BYTE bOffset = HIBYTE(dwFlags), bState = LOBYTE(dwFlags);

    if (bOffset > NUM_BYTES - 1)
        return FALSE;

    return ((bState & abValidateState[bOffset]) == bState);
}

/***************************************************************************\
* Set/ClearWindowState
*
* Wrapper functions for User mode to be able to set state fags
\***************************************************************************/

void SetWindowState(
    PWND pwnd,
    DWORD dwFlags)
{
    /*
     * Don't let anyone mess with someone else's window
     */
    if (GETPTI(pwnd)->ppi == PtiCurrent()->ppi) {
        if (ValidateState(dwFlags)) {
            SetWF(pwnd, dwFlags);
        } else {
            RIPMSG1(RIP_ERROR, "SetWindowState: invalid flag %x", dwFlags);
        }
    } else {
        RIPMSG1(RIP_WARNING, "SetWindowState: current ppi doesn't own pwnd %#p", pwnd);
    }

}

void ClearWindowState(
    PWND pwnd,
    DWORD dwFlags)
{
    /*
     * Don't let anyone mess with someone else's window
     */
    if (GETPTI(pwnd)->ppi == PtiCurrent()->ppi) {
        if (ValidateState(dwFlags)) {
            ClrWF(pwnd, dwFlags);
        } else {
            RIPMSG1(RIP_ERROR, "SetWindowState: invalid flag %x", dwFlags);
        }
    } else {
        RIPMSG1(RIP_WARNING, "ClearWindowState: current ppi doesn't own pwnd %#p", pwnd);
    }

}


/***************************************************************************\
* CheckPwndFilter
*
*
*
* History:
* 11-07-90 DarrinM      Translated Win 3.0 ASM code.
\***************************************************************************/

BOOL CheckPwndFilter(
    PWND pwnd,
    PWND pwndFilter)
{
    if ((pwndFilter == NULL) || (pwndFilter == pwnd) ||
            ((pwndFilter == (PWND)1) && (pwnd == NULL))) {
        return TRUE;
    }

    return _IsChild(pwndFilter, pwnd);
}


/***************************************************************************\
* AllocateUnicodeString
*
* History:
* 10-25-90 MikeHar      Wrote.
* 11-09-90 DarrinM      Fixed.
* 01-13-92 GregoryW     Neutralized.
* 03-05-98 FritzS       Only allocate Length+1
\***************************************************************************/

BOOL
AllocateUnicodeString(
    PUNICODE_STRING pstrDst,
    PUNICODE_STRING cczpstrSrc)
{
    if (cczpstrSrc == NULL) {
        RtlInitUnicodeString(pstrDst, NULL);
        return TRUE;
    }

    pstrDst->Buffer = UserAllocPoolWithQuota(cczpstrSrc->Length+sizeof(UNICODE_NULL), TAG_TEXT);
    if (pstrDst->Buffer == NULL) {
        return FALSE;
    }

    try {
        RtlCopyMemory(pstrDst->Buffer, cczpstrSrc->Buffer, cczpstrSrc->Length);
    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
        UserFreePool(pstrDst->Buffer);
        pstrDst->Buffer = NULL;
        return FALSE;
    }
    pstrDst->MaximumLength = cczpstrSrc->Length+sizeof(UNICODE_NULL);
    pstrDst->Length = cczpstrSrc->Length;
    pstrDst->Buffer[pstrDst->Length / sizeof(WCHAR)] = 0;

    return TRUE;
}


/***************************************************************************\
* xxxGetControlColor
*
* <brief description>
*
* History:
* 02-12-92 JimA     Ported from Win31 sources
\***************************************************************************/

HBRUSH xxxGetControlColor(
    PWND pwndParent,
    PWND pwndCtl,
    HDC hdc,
    UINT message)
{
    HBRUSH hbrush;

    /*
     * If we're sending to a window of another thread, don't send this message
     * but instead call DefWindowProc().  New rule about the CTLCOLOR messages.
     * Need to do this so that we don't send an hdc owned by one thread to
     * another thread.  It is also a harmless change.
     */
    if (PpiCurrent() != GETPTI(pwndParent)->ppi) {
        return (HBRUSH)xxxDefWindowProc(pwndParent, message, (WPARAM)hdc, (LPARAM)HW(pwndCtl));
    }

    hbrush = (HBRUSH)xxxSendMessage(pwndParent, message, (WPARAM)hdc, (LPARAM)HW(pwndCtl));

    /*
     * If the brush returned from the parent is invalid, get a valid brush from
     * xxxDefWindowProc.
     */
    if ((hbrush == 0) || !GreValidateServerHandle(hbrush, BRUSH_TYPE)) {
#if DBG
        if (hbrush != 0)
            RIPMSG2(RIP_WARNING,
                    "Invalid HBRUSH from WM_CTLCOLOR*** msg %lX brush %lX", message, hbrush);
#endif
        hbrush = (HBRUSH)xxxDefWindowProc(pwndParent, message,
                (WPARAM)hdc, (LPARAM)pwndCtl);
    }

    return hbrush;
}


/***************************************************************************\
* xxxGetControlBrush
*
* <brief description>
*
* History:
* 12-10-90 IanJa   type replaced with new 32-bit message
* 01-21-91 IanJa   Prefix '_' denoting exported function (although not API)
\***************************************************************************/

HBRUSH xxxGetControlBrush(
    PWND pwnd,
    HDC hdc,
    UINT message)
{
    HBRUSH hbr;
    PWND pwndSend;
    TL tlpwndSend;

    CheckLock(pwnd);

    if ((pwndSend = (TestwndPopup(pwnd) ? pwnd->spwndOwner : pwnd->spwndParent))
         == NULL)
        pwndSend = pwnd;

    ThreadLock(pwndSend, &tlpwndSend);

    /*
     * Last parameter changes the message into a ctlcolor id.
     */
    hbr = xxxGetControlColor(pwndSend, pwnd, hdc, message);
    ThreadUnlock(&tlpwndSend);

    return hbr;
}

/***************************************************************************\
* xxxHardErrorControl
*
* Performs kernel-mode hard error support functions.
*
* History:
* 02-08-95 JimA         Created.
\***************************************************************************/

UINT xxxHardErrorControl(
    DWORD dwCmd,
    HANDLE handle,
    PDESKRESTOREDATA pdrdRestore)
{
    PTHREADINFO ptiClient, ptiCurrent = PtiCurrent();
    PDESKTOP pdesk;
    PUNICODE_STRING pstrName;
    NTSTATUS Status;
    PETHREAD Thread;
    BOOL fAllowForeground;

    /*
     * turn off BlockInput so the user can respond to the hard error popup
     */
    gptiBlockInput = NULL;

    UserAssert(ISCSRSS());
    switch (dwCmd) {
    case HardErrorSetup:

        /*
         * Don't do it if the system has not been initialized.
         */
        if (grpdeskRitInput == NULL) {
            RIPMSG0(RIP_WARNING, "HardErrorControl: System not initialized");
            return HEC_ERROR;
        }

        /*
         * Setup caller as the hard error handler.
         */
        if (gHardErrorHandler.pti != NULL) {
            RIPMSG1(RIP_WARNING, "HardErrorControl: pti not NULL %#p", gHardErrorHandler.pti);
            return HEC_ERROR;
        }

        /*
         * Mark the handler as active.
         */
        gHardErrorHandler.pti = ptiCurrent;

        /*
         * Clear any pending quits.
         */
        ptiCurrent->cQuit = 0;

        break;

    case HardErrorCleanup:

        /*
         * Remove caller as the hard error handler.
         */
        if (gHardErrorHandler.pti != ptiCurrent)  {
            return HEC_ERROR;
        }

        gHardErrorHandler.pti = NULL;
        break;

    case HardErrorAttachUser:
    case HardErrorInDefDesktop:
        /*
         * Check for exit conditions. We do not allow attaches to the
         * disconnect desktop.
         */
        if (ISTS()) {
            if ((grpdeskRitInput == NULL) ||

                 ((grpdeskRitInput == gspdeskDisconnect) &&
                  (gspdeskShouldBeForeground == NULL)) ||

                 ((grpdeskRitInput == gspdeskDisconnect) &&
                  (gspdeskShouldBeForeground == gspdeskDisconnect))) {
                return HEC_ERROR;
            }
        }

        /*
         * Only attach to a user desktop.
         */
        if (ISTS() && grpdeskRitInput == gspdeskDisconnect) {
            pstrName = POBJECT_NAME(gspdeskShouldBeForeground);
        } else {
            pstrName = POBJECT_NAME(grpdeskRitInput);
        }

        if (pstrName && (!_wcsicmp(TEXT("Winlogon"), pstrName->Buffer) ||
                !_wcsicmp(TEXT("Disconnect"), pstrName->Buffer) ||
                !_wcsicmp(TEXT("Screen-saver"), pstrName->Buffer))) {
            RIPERR0(ERROR_ACCESS_DENIED, RIP_VERBOSE, "");
            return HEC_WRONGDESKTOP;
        }
        if (dwCmd == HardErrorInDefDesktop) {
            /*
             * Clear any pending quits.
             */
            ptiCurrent->cQuit = 0;
            return HEC_SUCCESS;
        }


        /*
         * Fall through.
         */

    case HardErrorAttach:

        /*
         * Save a pointer to and prevent destruction of the
         * current queue.  This will give us a queue to return
         * to if journalling is occuring when we tear down the
         * hard error popup.
         */
        gHardErrorHandler.pqAttach = ptiCurrent->pq;
        (ptiCurrent->pq->cLockCount)++;

        /*
         * Fall through.
         */

    case HardErrorAttachNoQueue:

        /*
         * Check for exit conditions. We do not allow attaches to the
         * disconnect desktop.
         */
        if (ISTS()) {
            if ((grpdeskRitInput == NULL) ||

                 ((grpdeskRitInput == gspdeskDisconnect) &&
                  (gspdeskShouldBeForeground == NULL)) ||

                 ((grpdeskRitInput == gspdeskDisconnect) &&
                  (gspdeskShouldBeForeground == gspdeskDisconnect))) {
                return HEC_ERROR;
            }
        }

        /*
         * Attach the handler to the current desktop.
         */
        /*
         * Don't allow an attach to the disconnected desktop, but
         * remember this for later when we detach.
         */
        gbDisconnectHardErrorAttach = FALSE;

        if (ISTS() && grpdeskRitInput == gspdeskDisconnect) {
            pdesk = gspdeskShouldBeForeground;
            gbDisconnectHardErrorAttach = TRUE;
        } else {
            pdesk = grpdeskRitInput;
        }

        UserAssert(pdesk != NULL);

        Status = xxxSetCsrssThreadDesktop(pdesk, pdrdRestore);

        if (!NT_SUCCESS(Status)) {
            RIPMSG1(RIP_WARNING, "HardErrorControl: HardErrorAttachNoQueue failed:%#lx", Status);
            if (dwCmd != HardErrorAttachNoQueue) {
                gHardErrorHandler.pqAttach = NULL;
                UserAssert(ptiCurrent->pq->cLockCount);
                (ptiCurrent->pq->cLockCount)--;
            }
            return HEC_ERROR;
        }

        /*
         * Make sure we actually set the pdesk in the current thread
         */
        UserAssert(ptiCurrent->rpdesk != NULL);

        /*
         * Determine if this box can come to the foreground.
         * Let it come to the foreground if it doesn't have a pti
         * (it might have just failed to load).
         */
        fAllowForeground = FALSE;
        if (handle != NULL) {
            Status = ObReferenceObjectByHandle(handle,
                                                THREAD_QUERY_INFORMATION,
                                                *PsThreadType,
                                                UserMode,
                                                &Thread,
                                                NULL);
            if (NT_SUCCESS(Status)) {
                ptiClient = PtiFromThread(Thread);
                if ((ptiClient == NULL) || CanForceForeground(ptiClient->ppi)) {
                    fAllowForeground = TRUE;
                }

                UnlockThread(Thread);

            } else {
                RIPMSG2(RIP_WARNING, "HardErrorControl: HardErrorAttach failed to get thread (%#lx) pointer:%#lx", handle, Status);
            }
        }

        if (fAllowForeground) {
            ptiCurrent->TIF_flags |= TIF_ALLOWFOREGROUNDACTIVATE;
            TAGMSG1(DBGTAG_FOREGROUND, "xxxHardErrorControl set TIF %#lx", ptiCurrent);
        } else {
            ptiCurrent->TIF_flags &= ~TIF_ALLOWFOREGROUNDACTIVATE;
            TAGMSG1(DBGTAG_FOREGROUND, "xxxHardErrorControl clear TIF %#lx", ptiCurrent);
        }

        break;

    case HardErrorDetach:

        /*
         * xxxSwitchDesktop may have sent WM_QUIT to the msgbox, so
         * ensure that the quit flag is reset.
         */
        ptiCurrent->cQuit = 0;

        /*
         * We will reset the hard-error queue to the pre-allocated
         * one so if we end up looping back (i.e. from a desktop
         * switch), we will have a valid queue in case the desktop
         * was deleted.
         */
        UserAssert(gHardErrorHandler.pqAttach->cLockCount);
        (gHardErrorHandler.pqAttach->cLockCount)--;

        DeferWinEventNotify();

        BEGINATOMICCHECK();

        if (ptiCurrent->pq != gHardErrorHandler.pqAttach) {
            UserAssert(gHardErrorHandler.pqAttach->cThreads == 0);
            AllocQueue(NULL, gHardErrorHandler.pqAttach);
            gHardErrorHandler.pqAttach->cThreads++;
            zzzAttachToQueue(ptiCurrent, gHardErrorHandler.pqAttach, NULL, FALSE);
        }

        gHardErrorHandler.pqAttach = NULL;

        ENDATOMICCHECK();

        zzzEndDeferWinEventNotify();

        /*
         * Fall through.
         */

    case HardErrorDetachNoQueue:
        /*
         * Detach the handler from the desktop and return
         * status to indicate if a switch has occured.
         */
        pdesk = ptiCurrent->rpdesk;
        xxxRestoreCsrssThreadDesktop(pdrdRestore);

        if (ISTS()) {
            /*
             * The hard error message box gets a desktop switch notification,
             * so remember that we lied to him and lie (or unlie) to him again.
             * A desktop switch did occur.
             */
            if (gbDisconnectHardErrorAttach) {
               gbDisconnectHardErrorAttach = FALSE;
               return HEC_DESKTOPSWITCH;
            }
#ifdef WAY_LATER
            /*
             * This happened once and caused a trap when a KeyEvent() came in and we
             * directed it to this queue.  I think this is a MS window that we caught
             * since we use this so much for license popup's.
             */
            if (gHardErrorHandler.pqAttach == gpqForeground) {
                gpqForeground = NULL;
            }
#endif
        }

        return (pdesk != grpdeskRitInput ? HEC_DESKTOPSWITCH : HEC_SUCCESS);
    }
    return HEC_SUCCESS;
}

#if 0 // not used anywhere (IanJa)
/***************************************************************************\
* VersionFromWindowFlag
*
* Returns the version of a window from its window state flags.
*
* History:
* 04-Apr-1997 adams     Created.
\***************************************************************************/

WORD
VersionFromWindowFlag(PWND pwnd)
{
    BYTE    bFlags = TestWF(pwnd, WFWINCOMPATMASK);
    if (bFlags == LOBYTE(WFWIN50COMPAT | WFWIN40COMPAT | WFWIN31COMPAT)) {
        return VER50;
    } else if (bFlags == LOBYTE(WFWIN40COMPAT | WFWIN31COMPAT)) {
        return VER40;
    } else if (bFlags == LOBYTE(WFWIN31COMPAT)) {
        return VER31;
    } else {
        return VER30;
    }
}
#endif // not used anywhere (IanJa)
