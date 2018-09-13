/***************************** Module Header ******************************\
* Module Name: desktop.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains everything related to the desktop support.
*
* History:
* 23-Oct-1990 DarrinM   Created.
* 01-Feb-1991 JimA      Added new API stubs.
* 11-Feb-1991 JimA      Added access checks.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

typedef struct _DESKTOP_CONTEXT {
    PUNICODE_STRING pstrDevice;
    LPDEVMODE       lpDevMode;
    DWORD           dwFlags;
} DESKTOP_CONTEXT, *PDESKTOP_CONTEXT;

extern BOOL fGdiEnabled;

/*
 * We use these to protect a handle we're currently using from being closed.
 */
PEPROCESS gProcessInUse;
HANDLE gHandleInUse;

/*
 * Debug Related Info.
 */
#if DBG
DWORD gDesktopsBusy;     // diagnostic
#endif

VOID FreeView(
    PEPROCESS Process,
    PDESKTOP pdesk);

#ifdef POOL_INSTR
    extern FAST_MUTEX* gpAllocFastMutex;   // mutex to syncronize pool allocations
#endif // POOL_INSTR


PVOID DesktopAlloc(
    PDESKTOP pdesk,
    UINT     uSize,
    DWORD    tag)
{
    if (pdesk->dwDTFlags & DF_DESTROYED) {
        RIPMSG2(RIP_ERROR,
                "DesktopAlloc: tag %d pdesk %#p is destroyed",
                tag,
                pdesk);
        return NULL;
    }

    return Win32HeapAlloc(pdesk->pheapDesktop, uSize, tag, 0);
}

#if DBG

WCHAR s_strName[64];
WCHAR s_strNameNull[] = L"null";

/***************************************************************************\
* GetDesktopName
*
* This is for debug purposes.
*
* Dec-10-1997 CLUPU     Created.
\***************************************************************************/
PWCHAR GetDesktopName(
    PDESKTOP pdesk)
{
    POBJECT_HEADER           pHead;
    POBJECT_HEADER_NAME_INFO pNameInfo;

    if (pdesk == NULL) {
        return s_strNameNull;
    }
    pHead = OBJECT_TO_OBJECT_HEADER(pdesk);
    pNameInfo = (POBJECT_HEADER_NAME_INFO)((char*)pHead - pHead->NameInfoOffset);

    UserAssert(pNameInfo->Name.Length < sizeof(s_strName));

    RtlCopyMemory(s_strName, pNameInfo->Name.Buffer, pNameInfo->Name.Length);
    s_strName[pNameInfo->Name.Length / sizeof(WCHAR)] = 0;

    return s_strName;
}

#endif // DBG

/***************************************************************************\
* xxxDesktopThread
*
* This thread owns all desktops windows on a windowstation.
* While waiting for messages, it moves the mouse cursor without entering the
* USER critical section.  The RIT does the rest of the mouse input processing.
*
* History:
* 03-Dec-1993 JimA      Created.
\***************************************************************************/

#ifdef LOCK_MOUSE_CODE
#pragma alloc_text(MOUSE, xxxDesktopThread)
#endif

#define OBJECTS_COUNT 4

VOID xxxDesktopThread(
    PTERMINAL pTerm)
{
    KPRIORITY       Priority;
    PTHREADINFO     ptiCurrent;
    PQ              pqOriginal;
    UNICODE_STRING  strThreadName;
    PKEVENT         *apRITEvents;
    PKEVENT         pEvent;
    MSGWAITCALLBACK pfnHidChangeRoutine = NULL;
    DWORD           nEvents = 0;
    UINT            idMouseInput;
    UINT            idDesktopDestroy;
    UINT            idPumpMessages;
    UINT            idHungThread;
    PKWAIT_BLOCK    WaitBlockArray;

    UserAssert(pTerm != NULL);

    /*
     * Set the desktop thread's priority to low realtime.
     */
    Priority = LOW_REALTIME_PRIORITY;
    ZwSetInformationThread(NtCurrentThread(),
                           ThreadPriority,
                           &Priority,
                           sizeof(KPRIORITY));

    /*
     * There are just two TERMINAL structures. One is for the
     * interactive windowstation and the other is for all the
     * non-interactive windowstations.
     */
    if (pTerm->dwTERMF_Flags & TERMF_NOIO) {
        RtlInitUnicodeString(&strThreadName, L"NOIO_DT");
    } else {
        RtlInitUnicodeString(&strThreadName, L"IO_DT");
    }

    if (!NT_SUCCESS(InitSystemThread(&strThreadName))) {
        pTerm->dwTERMF_Flags |= TERMF_DTINITFAILED;
        KeSetEvent(pTerm->pEventTermInit, EVENT_INCREMENT, FALSE);
        RIPMSG0(RIP_ERROR, "Fail to create the desktop thread");
        return;
    }

    ptiCurrent = PtiCurrentShared();

    pTerm->ptiDesktop = ptiCurrent;
    pTerm->pqDesktop  = pqOriginal = ptiCurrent->pq;

    (pqOriginal->cLockCount)++;
    ptiCurrent->pDeskInfo = &diStatic;

    /*
     * Set the winsta to NULL. It will be set to the right
     * windowstation in xxxCreateDesktop before pEventInputReady
     * is set.
     */
    ptiCurrent->pwinsta = NULL;

    /*
     * Allocate non-paged array.  Include an extra entry for
     * the thread's input event.
     */
    apRITEvents = UserAllocPoolNonPaged((OBJECTS_COUNT * sizeof(PKEVENT)),
                                        TAG_SYSTEM);

    if (apRITEvents == NULL) {
        pTerm->dwTERMF_Flags |= TERMF_DTINITFAILED;
        KeSetEvent(pTerm->pEventTermInit, EVENT_INCREMENT, FALSE);
        return;
    }

    WaitBlockArray = UserAllocPoolNonPaged((OBJECTS_COUNT * sizeof(KWAIT_BLOCK)),
                                           TAG_SYSTEM);
    if (WaitBlockArray == NULL) {
        pTerm->dwTERMF_Flags |= TERMF_DTINITFAILED;
        KeSetEvent(pTerm->pEventTermInit, EVENT_INCREMENT, FALSE);
        UserFreePool(apRITEvents);
        return;
    }

    idMouseInput     = 0xFFFF;
    idDesktopDestroy = 0xFFFF;
    idHungThread     = 0xFFFF;

    /*
     * Reference the mouse input event.  The system terminal doesn't
     * wait for any mouse input.
     */
    if (!(pTerm->dwTERMF_Flags & TERMF_NOIO)) {
        pfnHidChangeRoutine = (MSGWAITCALLBACK)ProcessDeviceChanges;
        idMouseInput  = nEvents++;
        UserAssert(aDeviceTemplate[DEVICE_TYPE_MOUSE].pkeHidChange);
        apRITEvents[idMouseInput] = aDeviceTemplate[DEVICE_TYPE_MOUSE].pkeHidChange;
    }

    /*
     * Create the desktop destruction event.
     */
    idDesktopDestroy = nEvents++;
    apRITEvents[idDesktopDestroy] = CreateKernelEvent(SynchronizationEvent, FALSE);
    if (apRITEvents[idDesktopDestroy] == NULL) {
        pTerm->dwTERMF_Flags |= TERMF_DTINITFAILED;
        KeSetEvent(pTerm->pEventTermInit, EVENT_INCREMENT, FALSE);
        UserFreePool(apRITEvents);
        UserFreePool(WaitBlockArray);
        return;
    }
    pTerm->pEventDestroyDesktop = apRITEvents[idDesktopDestroy];

    /*
     * Hung thread not needed for noninteractive windowstations.
     */
    if (!(pTerm->dwTERMF_Flags & TERMF_NOIO)) {
        idHungThread = nEvents++;
        apRITEvents[idHungThread] = CreateKernelEvent(SynchronizationEvent, FALSE);
        if (apRITEvents[idHungThread] == NULL) {
            pTerm->dwTERMF_Flags |= TERMF_DTINITFAILED;
            KeSetEvent(pTerm->pEventTermInit, EVENT_INCREMENT, FALSE);
            FreeKernelEvent(&apRITEvents[idDesktopDestroy]);
            UserFreePool(apRITEvents);
            UserFreePool(WaitBlockArray);
            return;
        }

        gpEventHungThread = apRITEvents[idHungThread];
    }

    EnterCrit();
    UserAssert(IsWinEventNotifyDeferredOK());

    /*
     * Set the event that tells the initialization of desktop
     * thread is done.
     */
    pTerm->dwTERMF_Flags |= TERMF_DTINITSUCCESS;
    KeSetEvent(pTerm->pEventTermInit, EVENT_INCREMENT, FALSE);

    /*
     * Prepare to wait on input ready event.
     */
    pEvent = pTerm->pEventInputReady;
    ObReferenceObjectByPointer(pEvent,
                               EVENT_ALL_ACCESS,
                               *ExEventObjectType,
                               KernelMode);

    LeaveCrit();

    KeWaitForSingleObject(pEvent, WrUserRequest, KernelMode, FALSE, NULL);
    ObDereferenceObject(pEvent);

    EnterCrit();

    /*
     * Adjust the event ids
     */
    idMouseInput     += WAIT_OBJECT_0;
    idDesktopDestroy += WAIT_OBJECT_0;
    idHungThread     += WAIT_OBJECT_0;
    idPumpMessages    = WAIT_OBJECT_0 + nEvents;

    /*
     * message loop lasts until we get a WM_QUIT message
     * upon which we shall return from the function
     */
    while (TRUE) {
        DWORD result;

        /*
         * Wait for any message sent or posted to this queue, while calling
         * ProcessDeviceChanges whenever the mouse change event (pkeHidChange)
         * is set.
         */
        result = xxxMsgWaitForMultipleObjects(nEvents,
                                              apRITEvents,
                                              pfnHidChangeRoutine,
                                              WaitBlockArray);

#if DBG
        gDesktopsBusy++; // diagnostic
        if (gDesktopsBusy >= 2) {
            RIPMSG0(RIP_WARNING, "2 or more desktop threads busy");
        }
#endif

        /*
         * result tells us the type of event we have:
         * a message or a signalled handle
         *
         * if there are one or more messages in the queue ...
         */
        if (result == (DWORD)idPumpMessages) {

            /*
             * block-local variable
             */
            MSG msg ;

            CheckCritIn();

            /*
             * read all of the messages in this next loop
             * removing each message as we read it
             */
            while (xxxPeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {

                /*
                 * if it's a quit message we're out of here
                 */
                if (msg.message == WM_QUIT && ptiCurrent->cWindows == 1) {

                    TRACE_DESKTOP(("WM_QUIT: Destroying the desktop thread. cWindows %d\n",
                                   ptiCurrent->cWindows));

                    /*
                     * The window station is gone, so
                     *
                     *      DON'T USE PWINSTA ANYMORE
                     */

                    /*
                     * We could have received a mouse message in between the
                     * desktop destroy event and the WM_QUIT message in which
                     * case we may need to clear spwndTrack again to make sure
                     * that a window (gotta be the desktop) isn't locked in.
                     */
                    Unlock(&ptiCurrent->rpdesk->spwndTrack);

                    /*
                     * If we're running on the last interactive desktop,
                     *  then we never unlocked pdesk->pDeskInfo->spwnd.
                     * However, it seems to me that the system stops
                     *  running before we make it here; otherwise, (or
                     *  for a Hydra-like thing) we need to unlock that
                     *  window here.....
                     */
                    UserAssert(ptiCurrent->rpdesk != NULL &&
                               ptiCurrent->rpdesk->pDeskInfo != NULL &&
                               ptiCurrent->rpdesk->pDeskInfo->spwnd != NULL);

                    Unlock(&ptiCurrent->rpdesk->pDeskInfo->spwnd);

                    /*
                     * Because there is no desktop, we need to fake a
                     * desktop info structure so that the IsHooked()
                     * macro can test a "valid" fsHooks value.
                     */
                    ptiCurrent->pDeskInfo = &diStatic;

                    /*
                     * The desktop window is all that's left, so
                     * let's exit.  The thread cleanup code will
                     * handle destruction of the window.
                     */

                    /*
                     * If the thread is not using the original queue,
                     * destroy it.
                     */
                    UserAssert(pqOriginal->cLockCount);
                    (pqOriginal->cLockCount)--;
                    if (ptiCurrent->pq != pqOriginal) {
                        zzzDestroyQueue(pqOriginal, ptiCurrent); // DeferWinEventNotify() ?? IANJA ??
                    }

#if DBG
                    gDesktopsBusy--; // diagnostic
#endif

                    LeaveCrit();

                    /*
                     * Deref the events now that we're done with them.
                     * Also free the wait array.
                     */
                    if (!(pTerm->dwTERMF_Flags & TERMF_NOIO)) {
                        FreeKernelEvent(&gpEventHungThread);
                    }
                    FreeKernelEvent(&apRITEvents[idDesktopDestroy]);
                    UserFreePool(apRITEvents);
                    UserFreePool(WaitBlockArray);

                    /*
                     * Terminate the thread.  This will call the
                     * Win32k thread cleanup code.
                     */
                    TRACE_DESKTOP(("call PsTerminateSystemThread\n"));

                    pTerm->ptiDesktop = NULL;
                    pTerm->pqDesktop  = NULL;

                    pTerm->dwTERMF_Flags &= TERMF_DTDESTROYED;

                    PsTerminateSystemThread(0);
                }

                UserAssert(msg.message != WM_QUIT);

                /*
                 * otherwise dispatch it
                 */
                xxxDispatchMessage(&msg);

            } // end of PeekMessage while loop

        } else if (result == idDesktopDestroy) {

            PDESKTOP        *ppdesk;
            PDESKTOP        pdesk;
            PWND            pwnd;
            PMENU           pmenu;
            TL              tlpwinsta;
            PWINDOWSTATION  pwinsta;
            TL              tlpdesk;
            TL              tlpwnd;
            PDESKTOP        pdeskTemp;
            HDESK           hdeskTemp;
            TL              tlpdeskTemp;

            /*
             * Destroy desktops on the destruction list.
             */
            for (ppdesk = &pTerm->rpdeskDestroy; *ppdesk != NULL; ) {
                /*
                 * Unlink from the list.
                 */
                pdesk = *ppdesk;

                TRACE_DESKTOP(("Destroying desktop '%ws' %#p ...\n",
                       GetDesktopName(pdesk), pdesk));

                UserAssert(!(pdesk->dwDTFlags & DF_DYING));

                ThreadLockDesktop(ptiCurrent, pdesk, &tlpdesk, LDLT_FN_DESKTOPTHREAD_DESK);
                pwinsta = pdesk->rpwinstaParent;
                ThreadLockWinSta(ptiCurrent, pdesk->rpwinstaParent, &tlpwinsta);

                LockDesktop(ppdesk, pdesk->rpdeskNext, LDL_TERM_DESKDESTROY1, (ULONG_PTR)pTerm);
                UnlockDesktop(&pdesk->rpdeskNext, LDU_DESK_DESKNEXT, 0);

                /*
                 * !!! If this is the current desktop, switch to another one.
                 */
                if (pdesk == grpdeskRitInput) {
                    PDESKTOP pdeskNew;

                    TRACE_DESKTOP(("Destroying the current active desktop\n"));

                    if (pwinsta->dwWSF_Flags & WSF_SWITCHLOCK) {

                        TRACE_DESKTOP(("The windowstation is locked\n"));

                        /*
                         * this should be the interactive windowstation
                         */
                        UserAssert(!(pwinsta->dwWSF_Flags & WSF_NOIO));

                        /*
                         * Switch to the disconnected desktop if the logon desktop
                         * is being destroyed, or there is no logon desktop, or
                         * if the logon desktop has already been destroyed.
                         */
                        if (gbRemoteSession && gspdeskDisconnect &&
                             (pdesk == grpdeskLogon ||
                              grpdeskLogon == NULL  ||
                              (grpdeskLogon->dwDTFlags & DF_DESKWNDDESTROYED))) {
                            TRACE_DESKTOP(("disable the screen and switch to the disconnect desktop\n"));
                            RemoteDisableScreen();
                            goto skip;

                        } else {
                            TRACE_DESKTOP(("Switch to the logon desktop '%ws' %#p ...\n",
                                   GetDesktopName(grpdeskLogon), grpdeskLogon));

                            pdeskNew = grpdeskLogon;
                        }
                    } else {
                        pdeskNew = pwinsta->rpdeskList;
                        if (pdeskNew == pdesk)
                            pdeskNew = pdesk->rpdeskNext;

                        /*
                         * You can hit this assert if you exit winlogon before
                         * logging in.  I.E. all desktop's close so there is
                         * no "next" one to switch to.  I'm assuming that there
                         * is a check for a NULL desktop in xxxSwitchDesktop().
                         *
                         * You can't switch to a NULL desktop.  But this means
                         * there isn't any input desktop so clear it manually.
                         */
                        if (gbRemoteSession) {
                            if (pdeskNew == NULL) {

                                TRACE_DESKTOP(("NO INPUT FOR DT FROM THIS POINT ON ...\n"));

                                ClearWakeBit(ptiCurrent, QS_INPUT | QS_EVENT | QS_MOUSEMOVE, FALSE);
                            }
                        } else {
                            UserAssert(pdeskNew);
                        }
                    }

                    TRACE_DESKTOP(("Switch to desktop '%ws' %#p\n",
                           GetDesktopName(pdeskNew), pdeskNew));

                    xxxSwitchDesktop(pwinsta, pdeskNew, FALSE);
                }
skip:

                /*
                 * Close the display if this desktop did not use
                 * the global display.
                 */
                if ((pdesk->pDispInfo->hDev != NULL) &&
                    (pdesk->pDispInfo->hDev != gpDispInfo->hDev)) {

                    TRACE_DESKTOP(("Destroy MDEV\n"));

                    DrvDestroyMDEV(pdesk->pDispInfo->pmdev);
                    GreFreePool(pdesk->pDispInfo->pmdev);
                    pdesk->pDispInfo->pmdev = NULL;
                }

                if (pdesk->pDispInfo != gpDispInfo) {
                    UserAssert(pdesk->pDispInfo->pMonitorFirst == NULL);
                    UserFreePool(pdesk->pDispInfo);
                    pdesk->pDispInfo = NULL;
                }

                /*
                 * Destroy desktop and menu windows.
                 */
                pdeskTemp = ptiCurrent->rpdesk;            // save current desktop
                hdeskTemp = ptiCurrent->hdesk;
                ThreadLockDesktop(ptiCurrent, pdeskTemp, &tlpdeskTemp, LDLT_FN_DESKTOPTHREAD_DESKTEMP);
                xxxSetThreadDesktop(NULL, pdesk);
                Unlock(&pdesk->spwndForeground);
                Unlock(&pdesk->spwndTray);

                Unlock(&pdesk->spwndTrack);
                pdesk->dwDTFlags &= ~DF_MOUSEMOVETRK;

                if (pdesk->spmenuSys != NULL) {
                    pmenu = pdesk->spmenuSys;
                    if (UnlockDesktopSysMenu(&pdesk->spmenuSys))
                        _DestroyMenu(pmenu);
                }

                if (pdesk->spmenuDialogSys != NULL) {
                    pmenu = pdesk->spmenuDialogSys;
                    if (UnlockDesktopSysMenu(&pdesk->spmenuDialogSys))
                        _DestroyMenu(pmenu);
                }

                if (pdesk->spmenuHScroll != NULL) {
                    pmenu = pdesk->spmenuHScroll;
                    if (UnlockDesktopMenu(&pdesk->spmenuHScroll))
                        _DestroyMenu(pmenu);
                }

                if (pdesk->spmenuVScroll != NULL) {
                    pmenu = pdesk->spmenuVScroll;
                    if (UnlockDesktopMenu(&pdesk->spmenuVScroll))
                        _DestroyMenu(pmenu);
                }

                /*
                 * If this desktop doesn't have a pDeskInfo, then
                 * something is wrong.  All desktops should have
                 * this until the object is freed.
                 */
                if (pdesk->pDeskInfo == NULL) {
                    RIPMSG0(RIP_ERROR,
                          "xxxDesktopThread: There is no pDeskInfo for this desktop");
                }

                if (pdesk->pDeskInfo) {
                    if (pdesk->pDeskInfo->spwnd == gspwndFullScreen)
                        Unlock(&gspwndFullScreen);

                    if (pdesk->pDeskInfo->spwndShell)
                        Unlock(&pdesk->pDeskInfo->spwndShell);

                    if (pdesk->pDeskInfo->spwndBkGnd)
                        Unlock(&pdesk->pDeskInfo->spwndBkGnd);

                    if (pdesk->pDeskInfo->spwndTaskman)
                        Unlock(&pdesk->pDeskInfo->spwndTaskman);

                    if (pdesk->pDeskInfo->spwndProgman)
                        Unlock(&pdesk->pDeskInfo->spwndProgman);
                }

                UserAssert(!(pdesk->dwDTFlags & DF_DYING));

                if (pdesk->spwndMenu != NULL) {

                    pwnd = pdesk->spwndMenu;

                    /*
                     * Hide this window without activating anyone else.
                     */
                    if (TestWF(pwnd, WFVISIBLE)) {
                        ThreadLockAlwaysWithPti(ptiCurrent, pwnd, &tlpwnd);
                        xxxSetWindowPos(pwnd,
                                        NULL,
                                        0,
                                        0,
                                        0,
                                        0,
                                        SWP_HIDEWINDOW | SWP_NOACTIVATE |
                                            SWP_NOMOVE | SWP_NOSIZE |
                                            SWP_NOZORDER | SWP_NOREDRAW |
                                            SWP_NOSENDCHANGING);

                        ThreadUnlock(&tlpwnd);
                    }

                    /*
                     * Reset the flag in the popupmenu structure that tells this
                     * popup menu belongs to the pdesk->spwndMenu so it can go away.
                     */
                    ((PMENUWND)pwnd)->ppopupmenu->fDesktopMenu = FALSE;
                    ((PMENUWND)pwnd)->ppopupmenu->fDelayedFree = FALSE;
                    #if DBG
                        /* We used to zero out this popup when closing the menu.
                         * now we zero it out when about to re-use it.
                         * So make ValidatepPopupMenu happy by clearing the
                         * leftover ppopupmenuRoot, if any.
                         */
                        ((PMENUWND)pwnd)->ppopupmenu->ppopupmenuRoot = NULL;
                    #endif

                    if (Unlock(&pdesk->spwndMenu)) {
                        xxxDestroyWindow(pwnd);
                    }
                }

                if (pdesk->spwndMessage != NULL) {

                    pwnd = pdesk->spwndMessage;

                    if (Unlock(&pdesk->spwndMessage)) {
                        xxxDestroyWindow(pwnd);
                    }
                }

                if (pdesk->spwndTooltip != NULL) {

                    pwnd = pdesk->spwndTooltip;

                    if (Unlock(&pdesk->spwndTooltip)) {
                        xxxDestroyWindow(pwnd);
                    }
                    UserAssert(!(pdesk->dwDTFlags & DF_TOOLTIPSHOWING));
                }

                UserAssert(!(pdesk->dwDTFlags & DF_DYING));

                /*
                 * If the dying desktop is the owner of the desktop
                 * owner window, reassign it to the first available
                 * desktop.  This is needed to ensure that
                 * xxxSetWindowPos will work on desktop windows.
                 */
                if (pTerm->spwndDesktopOwner != NULL &&
                    pTerm->spwndDesktopOwner->head.rpdesk == pdesk) {

                    PDESKTOP pdeskR;

                    /*
                     * Find out to what desktop the mother desktop window
                     * should go. Careful with the NOIO case where there
                     * might be several windowstations using the same
                     * mother desktop window
                     */
                    if (pTerm->dwTERMF_Flags & TERMF_NOIO) {

                        PWINDOWSTATION pwinstaW;

                        UserAssert(grpWinStaList != NULL);

                        pwinstaW = grpWinStaList->rpwinstaNext;

                        pdeskR = NULL;

                        while (pwinstaW != NULL) {
                            if (pwinstaW->rpdeskList != NULL) {
                                pdeskR = pwinstaW->rpdeskList;
                                break;
                            }
                            pwinstaW = pwinstaW->rpwinstaNext;
                        }
                    } else {
                        pdeskR = pwinsta->rpdeskList;
                    }

                    if (pdeskR == NULL) {

                        PWND pwnd;

                        TRACE_DESKTOP(("DESTROYING THE MOTHER DESKTOP WINDOW %#p\n",
                                pTerm->spwndDesktopOwner));

                        pwnd = pTerm->spwndDesktopOwner;

                        /*
                         * Hide it first
                         */
                        SetVisible(pwnd, SV_UNSET);

                        Unlock(&(pTerm->spwndDesktopOwner));

                        xxxDestroyWindow(pwnd);

                    } else {
                        TRACE_DESKTOP(("MOVING THE MOTHER DESKTOP WINDOW %#p to pdesk %#p '%ws'\n",
                                pTerm->spwndDesktopOwner, pdeskR, GetDesktopName(pdeskR)));

                        LockDesktop(&(pTerm->spwndDesktopOwner->head.rpdesk),
                                    pdeskR, LDL_MOTHERDESK_DESK1, (ULONG_PTR)(pTerm->spwndDesktopOwner));
                    }
                }

                if (pdesk->pDeskInfo && (pdesk->pDeskInfo->spwnd != NULL)) {

                    UserAssert(!(pdesk->dwDTFlags & DF_DESKWNDDESTROYED));

                    pwnd = pdesk->pDeskInfo->spwnd;

                    /*
                     * Hide this window without activating anyone else.
                     */
                    if (TestWF(pwnd, WFVISIBLE)) {
                        ThreadLockAlwaysWithPti(ptiCurrent, pwnd, &tlpwnd);
                        xxxSetWindowPos(pwnd,
                                        NULL,
                                        0,
                                        0,
                                        0,
                                        0,
                                        SWP_HIDEWINDOW | SWP_NOACTIVATE |
                                            SWP_NOMOVE | SWP_NOSIZE |
                                            SWP_NOZORDER | SWP_NOREDRAW |
                                            SWP_NOSENDCHANGING);

                        ThreadUnlock(&tlpwnd);
                    }

                    /*
                     * A lot of pwnd related code assumes that we
                     *  always have a valid desktop window. So we call
                     *  xxxDestroyWindow first to clean up and then
                     *  we unlock it to free it (now or eventually).
                     * However, if we're destroying the last destkop, then
                     *  we don't unlock the window since we're are forced
                     *  to continue running on that desktop.
                     */

                    TRACE_DESKTOP(("Destroying the desktop window\n"));

                    xxxDestroyWindow(pdesk->pDeskInfo->spwnd);
                    if (pdesk != grpdeskRitInput) {
                        Unlock(&pdesk->pDeskInfo->spwnd);
                    } else {

                        /*
                         * unlock the gspwndShouldBeForeground window
                         */
                        if (ISTS() && gspwndShouldBeForeground != NULL) {
                            Unlock(&gspwndShouldBeForeground);
                        }

                        /*
                         * This is hit in HYDRA when the last desktop does away
                         */
                        RIPMSG1(RIP_WARNING, "xxxDesktopThread: Running on zombie desk:%#p", pdesk);
                    }
                    pdesk->dwDTFlags |= DF_DESKWNDDESTROYED;
                }

                /*
                 * Restore the previous desktop
                 */
                xxxSetThreadDesktop(hdeskTemp, pdeskTemp);


                ThreadUnlockDesktop(ptiCurrent, &tlpdeskTemp, LDUT_FN_DESKTOPTHREAD_DESKTEMP);
                ThreadUnlockWinSta(ptiCurrent, &tlpwinsta);
                ThreadUnlockDesktop(ptiCurrent, &tlpdesk, LDUT_FN_DESKTOPTHREAD_DESK);
            }

            if (gbRemoteSession) {
                /*
                 * Wakeup ntinput thread for exit processing
                 */
                TRACE_DESKTOP(("Wakeup ntinput thread for exit processing\n"));

                UserAssert(gpevtDesktopDestroyed != NULL);

                KeSetEvent(gpevtDesktopDestroyed, EVENT_INCREMENT, FALSE);
            }

        } else if (result == idHungThread) {
            /*
             * By using gspwndMouseOwner this code relies on this event being
             * received before another click may occur. In the worst case we
             * may "lose" a click and only react to the latest one.
             */
            PWND pwnd = gspwndMouseOwner;
            if (pwnd != NULL && FHungApp(GETPTI(pwnd), CMSHUNGAPPTIMEOUT)) {

                int ht = FindNCHit(pwnd, POINTTOPOINTS(glinp.ptLastClick));

                if (ht == HTCLOSE) {
                    /*
                     * Note -- this is private, and does not go to the hook,
                     *   only the posted ones from the shell.
                     */

                    PostShellHookMessages(HSHELL_ENDTASK, (LPARAM)HWq(pwnd));
                } else if (ht == HTMINBUTTON) {
                    TL tlpwnd;

                    ThreadLockAlways(pwnd, &tlpwnd);
                    xxxMinimizeHungWindow(pwnd);
                    ThreadUnlock(&tlpwnd);
                }
            }

        } else {
            RIPMSG0(RIP_WARNING, "Desktop woke up for what?");
        }

#if DBG
        gDesktopsBusy--; // diagnostic
#endif
    }
}

/***************************************************************************\
* xxxRealizeDesktop
*
* 4/28/97   vadimg      created
\***************************************************************************/

VOID xxxRealizeDesktop(PWND pwnd)
{
    CheckLock(pwnd);
    UserAssert(GETFNID(pwnd) == FNID_DESKTOP);

    if (ghpalWallpaper) {
        HDC hdc = _GetDC(pwnd);
        xxxInternalPaintDesktop(pwnd, hdc, FALSE);
        _ReleaseDC(hdc);
    }
}

/***************************************************************************\
* xxxDesktopWndProc
*
* History:
* 23-Oct-1990 DarrinM   Ported from Win 3.0 sources.
* 08-Aug-1996 jparsons  51725 - added fix to prevent crash on WM_SETICON
\***************************************************************************/

LRESULT xxxDesktopWndProc(
    PWND   pwnd,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam)
{
    PTHREADINFO ptiCurrent = PtiCurrent();
    HDC         hdcT;
    PAINTSTRUCT ps;
    PWINDOWPOS  pwp;


    CheckLock(pwnd);
    UserAssert(IsWinEventNotifyDeferredOK());

    VALIDATECLASSANDSIZE(pwnd, message, wParam, lParam, FNID_DESKTOP, WM_CREATE);


    if (pwnd->spwndParent == NULL) {
        switch (message) {

            case WM_SETICON:
                /*
                 * cannot allow this as it will cause a callback to user mode from the
                 * desktop system thread.
                 */
                RIPMSG0(RIP_WARNING, "WM_ICON sent to desktop window was discarded.\n") ;
                return 0L ;

            default:
                break;
        } /* switch */

        return xxxDefWindowProc(pwnd, message, wParam, lParam);
    }

    switch (message) {

    case WM_WINDOWPOSCHANGING:

        /*
         * We receive this when switch desktop is called.  Just
         * to be consistent, set the rit desktop as this
         * thread's desktop.
         */
        pwp = (PWINDOWPOS)lParam;
        if (!(pwp->flags & SWP_NOZORDER) && pwp->hwndInsertAfter == HWND_TOP) {

            xxxSetThreadDesktop(NULL, grpdeskRitInput);

            /*
             * If some app has taken over the system-palette, we should make
             * sure the system is restored.  Otherwise, if this is the logon
             * desktop, we might not be able to view the dialog correctly.
             */
            if (GreGetSystemPaletteUse(gpDispInfo->hdcScreen) != SYSPAL_STATIC)
                GreRealizeDefaultPalette(gpDispInfo->hdcScreen, TRUE);

            /*
             * Let everyone know if the palette has changed
             */
            if (grpdeskRitInput->dwDTFlags & DTF_NEEDSPALETTECHANGED) {
                xxxSendNotifyMessage(PWND_BROADCAST,
                                     WM_PALETTECHANGED,
                                     (WPARAM)HWq(pwnd),
                                     0);
                grpdeskRitInput->dwDTFlags &= ~DTF_NEEDSPALETTECHANGED;
            }
        }
        break;

    case WM_FULLSCREEN: {
            TL tlpwndT;

            ThreadLockWithPti(ptiCurrent, grpdeskRitInput->pDeskInfo->spwnd, &tlpwndT);
            xxxMakeWindowForegroundWithState(
                    grpdeskRitInput->pDeskInfo->spwnd, GDIFULLSCREEN);
            ThreadUnlock(&tlpwndT);

            /*
             * We have to tell the switch window to repaint if we switched
             * modes
             */
            if (gspwndAltTab != NULL) {
                ThreadLockAlwaysWithPti(ptiCurrent, gspwndAltTab, &tlpwndT);
                xxxSendMessage(gspwndAltTab, WM_FULLSCREEN, 0, 0);
                ThreadUnlock(&tlpwndT);
            }

            break;
        }

    case WM_CLOSE:

        /*
         * Make sure nobody sends this window a WM_CLOSE and causes it to
         * destroy itself.
         */
        break;

    case WM_SETICON:
        /*
         * cannot allow this as it will cause a callback to user mode from the
         * desktop system thread.
         */
        RIPMSG0(RIP_WARNING, "WM_ICON sent to desktop window was discarded.\n") ;
        break;

    case WM_CREATE: {
        TL tlName;
        PUNICODE_STRING pProfileUserName = CreateProfileUserName(&tlName);
        /*
         * Is there a desktop pattern, or bitmap name in WIN.INI?
         */
        xxxSetDeskPattern(pProfileUserName, (LPWSTR)-1, TRUE);

        FreeProfileUserName(pProfileUserName, &tlName);
        /*
         * Initialize the system colors before we show the desktop window.
         */
        xxxSendNotifyMessage(pwnd, WM_SYSCOLORCHANGE, 0, 0L);

        hdcT = _GetDC(pwnd);
        xxxInternalPaintDesktop(pwnd, hdcT, FALSE); // use "normal" HDC so SelectPalette() will work
        _ReleaseDC(hdcT);

        /*
         * Save process and thread ids.
         */
        xxxSetWindowLong(pwnd,
                         0,
                         HandleToUlong(PsGetCurrentThread()->Cid.UniqueProcess),
                         FALSE);

        xxxSetWindowLong(pwnd,
                         4,
                         HandleToUlong(PsGetCurrentThread()->Cid.UniqueThread),
                         FALSE);
        break;
    }
    case WM_PALETTECHANGED:
        if (HWq(pwnd) == (HWND)wParam)
            break;

        // FALL THROUGH

    case WM_QUERYNEWPALETTE:
        xxxRealizeDesktop(pwnd);
        break;

    case WM_SYSCOLORCHANGE:

        /*
         * We do the redrawing if someone has changed the sys-colors from
         * another desktop and we need to redraw.  This is appearent with
         * the MATROX card which requires OGL applications to take over
         * the entire sys-colors for drawing.  When switching desktops, we
         * never broadcast the WM_SYSCOLORCHANGE event to tell us to redraw
         * This is only a DAYTONA related fix, and should be removed once
         * we move the SYSMETS to a per-desktop state.
         *
         * 05-03-95 : ChrisWil.
         */
        xxxRedrawWindow(pwnd,
                        NULL,
                        NULL,
                        RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE);
        break;

    case WM_ERASEBKGND:
        hdcT = (HDC)wParam;
        xxxInternalPaintDesktop(pwnd, hdcT, TRUE);
        return TRUE;

    case WM_PAINT:
        xxxBeginPaint(pwnd, (LPPAINTSTRUCT)&ps);
        xxxEndPaint(pwnd, (LPPAINTSTRUCT)&ps);
        break;

#ifdef HUNGAPP_GHOSTING
    case WM_HUNGTHREAD:
        {
            PWND pwnd = RevalidateHwnd((HWND)lParam);

            if (pwnd != NULL && FHungApp(GETPTI(pwnd), CMSHUNGAPPTIMEOUT)) {
                TL tlpwnd;

                pwnd = GetTopLevelWindow(pwnd);

                ThreadLockAlways(pwnd, &tlpwnd);
                xxxCreateGhost(pwnd);
                ThreadUnlock(&tlpwnd);
            }
            break;
        }
#endif // HUNGAPP_GHOSTING

    case WM_LBUTTONDBLCLK:
        message = WM_SYSCOMMAND;
        wParam = SC_TASKLIST;

        /*
         *** FALL THRU **
         */

    default:
        return xxxDefWindowProc(pwnd, message, wParam, lParam);
    }

    return 0L;
}

/***************************************************************************\
* SetDeskPattern
*
* NOTE: the lpszPattern parameter is new for Win 3.1.
*
* History:
* 23-Oct-1990 DarrinM   Created stub.
* 22-Apr-1991 DarrinM   Ported code from Win 3.1 sources.
\***************************************************************************/

BOOL xxxSetDeskPattern(PUNICODE_STRING pProfileUserName,
    LPWSTR   lpszPattern,
    BOOL     fCreation)
{
    LPWSTR p;
    int    i;
    UINT   val;
    WCHAR  wszNone[20];
    WCHAR  wchValue[MAX_PATH];
    WORD   rgBits[CXYDESKPATTERN];
    HBRUSH hBrushTemp;

    CheckCritIn();

    /*
     * Get rid of the old bitmap (if any).
     */
    if (ghbmDesktop != NULL) {
        GreDeleteObject(ghbmDesktop);
        ghbmDesktop = NULL;
    }

    /*
     * Check if a pattern is passed via lpszPattern.
     */
    if (lpszPattern != (LPWSTR)LongToPtr( (LONG)-1 )) {

        /*
         * Yes! Then use that pattern;
         */
        p = lpszPattern;
        goto GotThePattern;
    }

    /*
     * Else, pickup the pattern selected in WIN.INI.
     * Get the "DeskPattern" string from WIN.INI's [Desktop] section.
     */
    if (!FastGetProfileStringFromIDW(pProfileUserName,
                                        PMAP_DESKTOP,
                                        STR_DESKPATTERN,
                                        TEXT(""),
                                        wchValue,
                                        sizeof(wchValue)/sizeof(WCHAR)
                                        )) {
        return FALSE;
    }

    ServerLoadString(hModuleWin,
                     STR_NONE,
                     wszNone,
                     sizeof(wszNone)/sizeof(WCHAR));

    p = wchValue;

GotThePattern:

    /*
     * Was a Desk Pattern selected?
     */
    if (*p == TEXT('\0') || _wcsicmp(p, wszNone) == 0) {
        hBrushTemp = GreCreateSolidBrush(SYSRGB(DESKTOP));
        if (hBrushTemp != NULL) {
            if (SYSHBR(DESKTOP)) {
                GreMarkDeletableBrush(SYSHBR(DESKTOP));
                GreDeleteObject(SYSHBR(DESKTOP));
            }
            GreMarkUndeletableBrush(hBrushTemp);
            SYSHBR(DESKTOP) = hBrushTemp;
        }
        GreSetBrushOwnerPublic(hBrushTemp);
        goto SDPExit;
    }

    /*
     * Get eight groups of numbers seprated by non-numeric characters.
     */
    for (i = 0; i < CXYDESKPATTERN; i++) {
        val = 0;

        /*
         * Skip over any non-numeric characters, check for null EVERY time.
         */
        while (*p && !(*p >= TEXT('0') && *p <= TEXT('9')))
            p++;

        /*
         * Get the next series of digits.
         */
        while (*p >= TEXT('0') && *p <= TEXT('9'))
            val = val * (UINT)10 + (UINT)(*p++ - TEXT('0'));

        rgBits[i] = (WORD)val;
    }

    ghbmDesktop = GreCreateBitmap(CXYDESKPATTERN,
                                  CXYDESKPATTERN,
                                  1,
                                  1,
                                  (LPBYTE)rgBits);

    if (ghbmDesktop == NULL)
        return FALSE;

    GreSetBitmapOwner(ghbmDesktop, OBJECT_OWNER_PUBLIC);

    RecolorDeskPattern();

SDPExit:
    if (!fCreation) {

        /*
         * Notify everyone that the colors have changed.
         */
        xxxSendNotifyMessage(PWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0L);

        /*
         * Update the entire screen.  If this is creation, don't update: the
         * screen hasn't drawn, and also there are some things that aren't
         * initialized yet.
         */
        xxxRedrawScreen();
    }

    return TRUE;
}

/***************************************************************************\
* RecolorDeskPattern
*
* Remakes the desktop pattern (if it exists) so that it uses the new
* system colors.
*
* History:
* 22-Apr-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

VOID RecolorDeskPattern(VOID)
{
    HBITMAP hbmOldDesk;
    HBITMAP hbmOldMem;
    HBITMAP hbmMem;
    HBRUSH  hBrushTemp;
    if (ghbmDesktop == NULL)
        return;

    /*
     * Redo the desktop pattern in the new colors.
     */

    if (hbmOldDesk = GreSelectBitmap(ghdcMem, ghbmDesktop)) {

        if (!SYSMET(SAMEDISPLAYFORMAT)) {

            BYTE bmi[sizeof(BITMAPINFOHEADER)+sizeof(RGBQUAD)*2];
            PBITMAPINFO pbmi = (PBITMAPINFO) &bmi;

            pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            pbmi->bmiHeader.biWidth = CXYDESKPATTERN;
            pbmi->bmiHeader.biHeight = CXYDESKPATTERN;
            pbmi->bmiHeader.biPlanes = 1;
            pbmi->bmiHeader.biBitCount = 1;
            pbmi->bmiHeader.biCompression = BI_RGB;
            pbmi->bmiHeader.biSizeImage = 0;
            pbmi->bmiHeader.biXPelsPerMeter = 0;
            pbmi->bmiHeader.biYPelsPerMeter = 0;
            pbmi->bmiHeader.biClrUsed = 2;
            pbmi->bmiHeader.biClrImportant = 2;

            pbmi->bmiColors[0].rgbBlue  = (BYTE)((SYSRGB(DESKTOP) >> 16) & 0xff);
            pbmi->bmiColors[0].rgbGreen = (BYTE)((SYSRGB(DESKTOP) >>  8) & 0xff);
            pbmi->bmiColors[0].rgbRed   = (BYTE)((SYSRGB(DESKTOP)      ) & 0xff);

            pbmi->bmiColors[1].rgbBlue  = (BYTE)((SYSRGB(WINDOWTEXT) >> 16) & 0xff);
            pbmi->bmiColors[1].rgbGreen = (BYTE)((SYSRGB(WINDOWTEXT) >>  8) & 0xff);
            pbmi->bmiColors[1].rgbRed   = (BYTE)((SYSRGB(WINDOWTEXT)      ) & 0xff);

            hbmMem = GreCreateDIBitmapReal(
               HDCBITS(), 0, NULL,
               pbmi,DIB_RGB_COLORS,sizeof(bmi),0,
               NULL,0,NULL,0,0);

        } else {

            hbmMem = GreCreateCompatibleBitmap(
               HDCBITS(), CXYDESKPATTERN, CXYDESKPATTERN);
        }

        if (hbmMem) {

            if (hbmOldMem = GreSelectBitmap(ghdcMem2, hbmMem)) {

                GreSetTextColor(ghdcMem2, SYSRGB(DESKTOP));
                GreSetBkColor(ghdcMem2, SYSRGB(WINDOWTEXT));

                GreBitBlt(ghdcMem2,
                          0,
                          0,
                          CXYDESKPATTERN,
                          CXYDESKPATTERN,
                          ghdcMem,
                          0,
                          0,
                          SRCCOPY,
                          0);

                if (hBrushTemp = GreCreatePatternBrush(hbmMem)) {

                    if (SYSHBR(DESKTOP) != NULL) {
                        GreMarkDeletableBrush(SYSHBR(DESKTOP));
                        GreDeleteObject(SYSHBR(DESKTOP));
                    }

                    GreMarkUndeletableBrush(hBrushTemp);
                    SYSHBR(DESKTOP) = hBrushTemp;
                }

                GreSetBrushOwnerPublic(hBrushTemp);
                GreSelectBitmap(ghdcMem2, hbmOldMem);
            }

            GreDeleteObject(hbmMem);
        }

        GreSelectBitmap(ghdcMem, hbmOldDesk);
    }
}

/***************************************************************************\
* xxxCreateDesktop (API)
*
* Create a new desktop object
*
* History:
* 16-Jan-1991 JimA      Created scaffold code.
* 11-Feb-1991 JimA      Added access checks.
\***************************************************************************/

NTSTATUS xxxCreateDesktop2(
    PWINDOWSTATION   pwinsta,
    PACCESS_STATE    pAccessState,
    KPROCESSOR_MODE  AccessMode,
    PUNICODE_STRING  pstrName,
    PDESKTOP_CONTEXT Context,
    PVOID            *pObject)
{
    LUID              luidCaller;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PEPROCESS         Process;
    PDESKTOP          pdesk;
    PDESKTOPINFO      pdi;
    ULONG             ulHeapSize;
    NTSTATUS          Status;

    CheckCritIn();

    /*
     * If this is a desktop creation, make sure
     * that the windowstation grants create access.
     */
    if (!ObCheckCreateObjectAccess(
            pwinsta,
            WINSTA_CREATEDESKTOP,
            pAccessState,
            pstrName,
            TRUE,
            AccessMode,
            &Status)) {

        return Status;
    }
    /*
     * Fail if the windowstation is locked
     */
    Process = PsGetCurrentProcess();

    if (pwinsta->dwWSF_Flags & WSF_OPENLOCK &&
            Process->UniqueProcessId != gpidLogon) {

        /*
         * If logoff is occuring and the caller does not
         * belong to the session that is ending, allow the
         * open to proceed.
         */
        Status = GetProcessLuid(NULL, &luidCaller);

        if (!NT_SUCCESS(Status) ||
                !(pwinsta->dwWSF_Flags & WSF_SHUTDOWN) ||
                RtlEqualLuid(&luidCaller, &pwinsta->luidEndSession)) {
            return STATUS_DEVICE_BUSY;
        }
    }

    /*
     * If a devmode has been specified, we also must be able
     * to switch desktops.
     */
    if (Context->lpDevMode != NULL && (pwinsta->dwWSF_Flags & WSF_OPENLOCK) &&
            Process->UniqueProcessId != gpidLogon) {
        return STATUS_DEVICE_BUSY;
    }

    /*
     * Allocate the new object
     */
    InitializeObjectAttributes(&ObjectAttributes, pstrName, 0, NULL, NULL);
    Status = ObCreateObject(
            KernelMode,
            *ExDesktopObjectType,
            &ObjectAttributes,
            UserMode,
            NULL,
            sizeof(DESKTOP),
            0,
            0,
            &pdesk);
    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_WARNING, "xxxCreateDesktop2: ObCreateObject failed with Status 0x%x",
                Status);
        return Status;
    }
    RtlZeroMemory(pdesk, sizeof(DESKTOP));

    /*
     * Store the session id of the session who created the desktop
     */
    pdesk->dwSessionId = gSessionId;

    /*
     * Create security descriptor
     */
    Status = ObAssignSecurity(
            pAccessState,
            OBJECT_TO_OBJECT_HEADER(pwinsta)->SecurityDescriptor,
            pdesk,
            *ExDesktopObjectType);

    if (!NT_SUCCESS(Status))
        goto Error;

    /*
     * Set up desktop heap.  The first desktop (logon desktop) uses a
     * small heap (128).
     */
    if (!(pwinsta->dwWSF_Flags & WSF_NOIO) && (pwinsta->rpdeskList == NULL)) {
#ifdef _WIN64
        /*
         * Temporary fix. When winlogon starts creating processes on the right
         * desktop, we'll have to figure out what this size should really be.
         */
        ulHeapSize = gdwDesktopSectionSize;
#else
        ulHeapSize = 128;
#endif
    } else {
        if (pwinsta->dwWSF_Flags & WSF_NOIO) {
            ulHeapSize = gdwNOIOSectionSize;
        } else {

            /*
             * The disconnected desktop should be small also.
             */
            if (gbRemoteSession && gspdeskDisconnect == NULL)
                ulHeapSize = 64;
            else
                ulHeapSize = gdwDesktopSectionSize;
        }
    }

    ulHeapSize *= 1024;
    /*
     * Create the desktop heap.
     */
    pdesk->hsectionDesktop = CreateDesktopHeap(&pdesk->pheapDesktop, ulHeapSize);
    if (pdesk->hsectionDesktop == NULL) {
        RIPMSG1(RIP_WARNING, "xxxCreateDesktop: CreateDesktopHeap failed for pdesk %#p",
                pdesk);
        goto ErrorOutOfMemory;
    }

    if (pwinsta->rpdeskList == NULL || (pwinsta->dwWSF_Flags & WSF_NOIO)) {

        /*
         * The first desktop or invisible desktops must also
         * use the default settings.  This is because specifying
         * the devmode causes a desktop switch, which must be
         * avoided in this case.
         */
        Context->lpDevMode = NULL;
    }

    /*
     * Allocate desktopinfo
     */
    pdi = (PDESKTOPINFO)DesktopAlloc(pdesk, sizeof(DESKTOPINFO), DTAG_DESKTOPINFO);
    if (pdi == NULL) {
        RIPMSG0(RIP_WARNING, "xxxCreateDesktop: failed DeskInfo Alloc");
        goto ErrorOutOfMemory;
    }

    /*
     * Initialize everything.
     */
    pdesk->pDeskInfo = pdi;
    InitializeListHead(&pdesk->PtiList);

    /*
     * If a DEVMODE or another device name is passed in, then use that
     * information.
     * Otherwise use the default information (gpDispInfo).
     */

    if (Context->lpDevMode) {

        BOOL  bDisabled = FALSE;
        PMDEV pmdev = NULL;
        LONG  ChangeStat = GRE_DISP_CHANGE_FAILED;

        /*
         * Allocate a display-info for this device.
         */
        pdesk->pDispInfo = (PDISPLAYINFO)UserAllocPoolZInit(
                sizeof(DISPLAYINFO), TAG_DISPLAYINFO);

        if (!pdesk->pDispInfo) {
            RIPMSG1(RIP_WARNING, "xxxCreateDesktop: failed to allocate pDispInfo for pdesk %#p",
                    pdesk);
            goto ErrorOutOfMemory;
        }

        if ((bDisabled = DrvDisableMDEV(gpDispInfo->pmdev, TRUE)) == TRUE) {

            ChangeStat = DrvChangeDisplaySettings(Context->pstrDevice,
                                                  NULL,
                                                  Context->lpDevMode,
                                                  LongToPtr( gdwDesktopId ),
                                                  UserMode,
                                                  FALSE,
                                                  TRUE,
                                                  NULL,
                                                  &pmdev,
                                                  GRE_DEFAULT,
                                                  FALSE);
        }

        if (ChangeStat != GRE_DISP_CHANGE_SUCCESSFUL) {

            if (bDisabled) {
                DrvEnableMDEV(gpDispInfo->pmdev, TRUE);
            }

            //
            // If there is a failure, then repaint the whole screen.
            //

            RIPMSG1(RIP_WARNING, "xxxCreateDesktop2 callback for pdesk %#p !",
                    pdesk);

            xxxUserResetDisplayDevice();

            Status = STATUS_UNSUCCESSFUL;
            goto Error;
        }

        pdesk->pDispInfo->hDev  = pmdev->hdevParent;
        pdesk->pDispInfo->pmdev = pmdev;
        pdesk->dwDesktopId      = gdwDesktopId++;

        CopyRect(&pdesk->pDispInfo->rcScreen, &gpDispInfo->rcScreen);
        pdesk->pDispInfo->dmLogPixels = gpDispInfo->dmLogPixels;

        pdesk->pDispInfo->pMonitorFirst = NULL;
        pdesk->pDispInfo->pMonitorPrimary = NULL;

    } else {

        pdesk->pDispInfo   = gpDispInfo;
        pdesk->dwDesktopId = GW_DESKTOP_ID;

    }

    /*
     * Heap is HEAP_ZERO_MEMORY, so we should be zero-initialized already.
     */
    UserAssert(pdi->pvwplShellHook == NULL);

    pdi->pvDesktopBase  = Win32HeapGetHandle(pdesk->pheapDesktop);
    pdi->pvDesktopLimit = (PBYTE)pdi->pvDesktopBase + ulHeapSize;

    /*
     * Reference the parent windowstation
     */
    LockWinSta(&(pdesk->rpwinstaParent), pwinsta);

    /*
     * Link the desktop into the windowstation list
     */
    if (pwinsta->rpdeskList == NULL) {

        if (!(pwinsta->dwWSF_Flags & WSF_NOIO))
            LockDesktop(&grpdeskLogon, pdesk, LDL_DESKLOGON, 0);
        /*
         * Make the first desktop the "owner" of the top
         * desktop window.  This is needed to ensure that
         * xxxSetWindowPos will work on desktop windows.
         */
        LockDesktop(&(pwinsta->pTerm->spwndDesktopOwner->head.rpdesk),
                    pdesk, LDL_MOTHERDESK_DESK2, (ULONG_PTR)(pwinsta->pTerm->spwndDesktopOwner));
    }


    LockDesktop(&pdesk->rpdeskNext, pwinsta->rpdeskList, LDL_DESK_DESKNEXT1, (ULONG_PTR)pwinsta);
    LockDesktop(&pwinsta->rpdeskList, pdesk, LDL_WINSTA_DESKLIST1, (ULONG_PTR)pwinsta);

    /*
     * Mask off invalid access bits
     */
    if (pAccessState->RemainingDesiredAccess & MAXIMUM_ALLOWED) {
        pAccessState->RemainingDesiredAccess &= ~MAXIMUM_ALLOWED;
        pAccessState->RemainingDesiredAccess |= GENERIC_ALL;
    }

    RtlMapGenericMask( &pAccessState->RemainingDesiredAccess, (PGENERIC_MAPPING)&DesktopMapping);
    pAccessState->RemainingDesiredAccess &=
            (DesktopMapping.GenericAll | ACCESS_SYSTEM_SECURITY);

    *pObject = pdesk;

    /*
     * Add the desktop to the global list of desktops in this win32k.
     * This is HYDRA only
     */
    DbgTrackAddDesktop(pdesk);

    return STATUS_SUCCESS;

ErrorOutOfMemory:
    Status = STATUS_NO_MEMORY;
    // fall-through

Error:
    LogDesktop(pdesk, LD_DEREF_FN_2CREATEDESKTOP, FALSE, 0);
    ObDereferenceObject(pdesk);

    UserAssert(!NT_SUCCESS(Status));

    return Status;
}

BOOL xxxCreateDisconnectDesktop(
    HWINSTA        hwinsta,
    PWINDOWSTATION pwinsta)
{
    UNICODE_STRING      strDesktop;
    OBJECT_ATTRIBUTES   oa;
    HDESK               hdeskDisconnect;
    HRGN                hrgn;
    NTSTATUS            Status;

    /*
     * If not created yet, then create the Disconnected desktop
     * (used when WinStation is disconnected), and lock the desktop
     * and desktop window to ensure they never get deleted.
     */
    RtlInitUnicodeString(&strDesktop, TEXT("Disconnect"));
    InitializeObjectAttributes(&oa, &strDesktop,
            OBJ_OPENIF | OBJ_CASE_INSENSITIVE, hwinsta, NULL);

    hdeskDisconnect = xxxCreateDesktop(&oa, KernelMode,
            NULL, NULL, 0, MAXIMUM_ALLOWED);

    if (hdeskDisconnect == NULL) {
        RIPMSG0(RIP_WARNING, "Could not create Disconnect desktop");
        return FALSE;
    }

    /*
     * Keep around an extra reference to the disconnect desktop from
     * the CSR so it will stay around even if winlogon exits.
     */
    Status = ObReferenceObjectByHandle(hdeskDisconnect,
                                       0,
                                       NULL,
                                       KernelMode,
                                       &gspdeskDisconnect,
                                       NULL);
    if (!NT_SUCCESS(Status)) {

        RIPMSG1(RIP_WARNING, "Disconnect Desktop reference failed 0x%x", Status);

        xxxCloseDesktop(hdeskDisconnect, KernelMode);
        gspdeskDisconnect = NULL;
        return FALSE;
    }

    LogDesktop(gspdeskDisconnect, LDL_DESKDISCONNECT, TRUE, 0);

    /*
     * Set the region of the desktop window to be (0, 0, 0, 0) so
     * that there is no hittesting going on the 'disconnect' desktop
     */
    hrgn = CreateEmptyRgn();

    UserAssert(gspdeskDisconnect->pDeskInfo != NULL);

    SelectWindowRgn(gspdeskDisconnect->pDeskInfo->spwnd, hrgn);

    KeAttachProcess(&gpepCSRSS->Pcb);

    Status = ObOpenObjectByPointer(
                 gspdeskDisconnect,
                 0,
                 NULL,
                 EVENT_ALL_ACCESS,
                 NULL,
                 KernelMode,
                 &ghDisconnectDesk);

    if (NT_SUCCESS(Status)) {

        Status = ObOpenObjectByPointer(
                     pwinsta,
                     0,
                     NULL,
                     EVENT_ALL_ACCESS,
                     NULL,
                     KernelMode,
                     &ghDisconnectWinSta);
    }

    KeDetachProcess();

    if (!NT_SUCCESS(Status)) {

        RIPMSG0(RIP_WARNING, "Could not create Disconnect desktop");

        if (ghDisconnectDesk != NULL) {
            CloseProtectedHandle(ghDisconnectDesk);
            ghDisconnectDesk = NULL;
        }

        xxxCloseDesktop(hdeskDisconnect, KernelMode);
        return FALSE;
    }

    /*
     * Don't want to do alot of paints if we disconnected before this.
     */
    if (!gbConnected) {
        RIPMSG0(RIP_WARNING,
            "RemoteDisconnect was issued during CreateDesktop(\"Winlogon\"...");
    }

    return TRUE;
}

VOID CleanupDirtyDesktops(
    VOID)
{
    PWINDOWSTATION pwinsta;
    PDESKTOP*      ppdesk;

    CheckCritIn();

    for (pwinsta = grpWinStaList; pwinsta != NULL; pwinsta = pwinsta->rpwinstaNext) {

        ppdesk = &pwinsta->rpdeskList;

        while (*ppdesk != NULL) {

            if (!((*ppdesk)->dwDTFlags & DF_DESKCREATED)) {
                RIPMSG1(RIP_WARNING, "Desktop %#p in a dirty state", *ppdesk);

                if (grpdeskLogon == *ppdesk) {
                    UnlockDesktop(&grpdeskLogon, LDU_DESKLOGON, 0);
                }

                if (pwinsta->pTerm->spwndDesktopOwner &&
                    pwinsta->pTerm->spwndDesktopOwner->head.rpdesk == *ppdesk) {

                    UnlockDesktop(&(pwinsta->pTerm->spwndDesktopOwner->head.rpdesk),
                                  LDU_MOTHERDESK_DESK, (ULONG_PTR)(pwinsta->pTerm->spwndDesktopOwner));
                }

                LockDesktop(ppdesk, (*ppdesk)->rpdeskNext, LDL_WINSTA_DESKLIST1, (ULONG_PTR)pwinsta);
            } else {
                ppdesk = &(*ppdesk)->rpdeskNext;
            }
        }
    }
}

HDESK xxxCreateDesktop(
    POBJECT_ATTRIBUTES ccxObjectAttributes,
    KPROCESSOR_MODE    ProbeMode,
    PUNICODE_STRING    ccxpstrDevice,
    LPDEVMODE          ccxlpdevmode,
    DWORD              dwFlags,
    DWORD              dwDesiredAccess)
{
    HWINSTA         hwinsta;
    HDESK           hdesk;
    DESKTOP_CONTEXT Context;
    PDESKTOP        pdesk;
    PDESKTOPINFO    pdi;
    PWINDOWSTATION  pwinsta;
    PDESKTOP        pdeskTemp;
    HDESK           hdeskTemp;
    PWND            pwndDesktop = NULL;
    PWND            pwndMessage = NULL;
    PWND            pwndTooltip = NULL;
    PWND            pwndMenu = NULL;
    TL              tlpwnd;
    PTHREADINFO     ptiCurrent = PtiCurrent();
    BOOL            fWasNull;
    BOOL            bSuccess;
    PPROCESSINFO    ppi;
    PPROCESSINFO    ppiSave;
    PTERMINAL       pTerm;
    NTSTATUS        Status;
    DWORD           dwDisableHooks;

#if DBG
    /*
     * Too many jumps in this function to use BEGIN/ENDATOMICHCECK
     */
    DWORD dwCritSecUseSave = gdwCritSecUseCount;
#endif

    CheckCritIn();

    UserAssert(IsWinEventNotifyDeferredOK());

    /*
     * Capture directory handle and check for create access.
     */
    try {
        hwinsta = ccxObjectAttributes->RootDirectory;
    } except (W32ExceptionHandler(TRUE, RIP_WARNING)) {
        return NULL;
    }
    if (hwinsta != NULL) {
        Status = ObReferenceObjectByHandle(
                hwinsta,
                WINSTA_CREATEDESKTOP,
                *ExWindowStationObjectType,
                ProbeMode,
                &pwinsta,
                NULL);
        if (NT_SUCCESS(Status)) {
            ObDereferenceObject(pwinsta);
        } else {
            RIPNTERR0(Status, RIP_VERBOSE, "ObReferenceObjectByHandle Failed");
            return NULL;
        }
    }

    /*
     * Set up creation context
     */
    Context.lpDevMode  = ccxlpdevmode;
    Context.pstrDevice = ccxpstrDevice;
    Context.dwFlags    = dwFlags;

    /*
     * Create the desktop -- the object manager uses try blocks
     */
    Status = ObOpenObjectByName(
            ccxObjectAttributes,
            *ExDesktopObjectType,
            ProbeMode,
            NULL,
            dwDesiredAccess,
            &Context,
            &hdesk);

    if (!NT_SUCCESS(Status)) {

        RIPNTERR1(Status,
                  RIP_WARNING,
                  "xxxCreateDesktop: ObOpenObjectByName failed with Status 0x%x",
                  Status);

        /*
         * Cleanup desktop objects that were created in xxxCreateDesktop2
         * but later on the Ob manager failed the creation for other
         * reasons (ex: no quota)
         */
        CleanupDirtyDesktops();

        return NULL;
    }

    /*
     * If the desktop already exists, we're done.  This will only happen
     * if OBJ_OPENIF was specified.
     */
    if (Status == STATUS_OBJECT_NAME_EXISTS) {
        SetHandleFlag(hdesk, HF_PROTECTED, TRUE);
        RIPMSG0(RIP_WARNING, "xxxCreateDesktop: Object name exists");
        return hdesk;
    }

    /*
     * Reference the desktop to finish initialization
     */
    Status = ObReferenceObjectByHandle(
            hdesk,
            0,
            *ExDesktopObjectType,
            KernelMode,
            &pdesk,
            NULL);
    if (!NT_SUCCESS(Status)) {
        RIPNTERR0(Status, RIP_VERBOSE, "");
        CloseProtectedHandle(hdesk);
        return NULL;
    }

    pdesk->dwDTFlags |= DF_DESKCREATED;

    LogDesktop(pdesk, LD_REF_FN_CREATEDESKTOP, TRUE, (ULONG_PTR)PtiCurrent());

    pwinsta = pdesk->rpwinstaParent;
    pTerm   = pwinsta->pTerm;
    pdi = pdesk->pDeskInfo;

    pdi->ppiShellProcess = NULL;

    /*
     * If the desktop was not mapped in as a result of the open,
     * fail.
     */
    ppi = PpiCurrent();
    if (GetDesktopView(ppi, pdesk) == NULL) {

        /*
         * Desktop mapping failed.
         */
        CloseProtectedHandle(hdesk);

        LogDesktop(pdesk, LD_DEREF_FN_CREATEDESKTOP1, FALSE, (ULONG_PTR)PtiCurrent());

        ObDereferenceObject(pdesk);
        RIPNTERR0(STATUS_ACCESS_DENIED, RIP_WARNING, "Desktop mapping failed");
        return NULL;
    }

    if (gpepCSRSS != NULL) {
        /*
         * Map the desktop into CSRSS to ensure that the
         * hard error handler can get access.
         */
        try {
            MapDesktop(ObOpenHandle, gpepCSRSS, pdesk, 0, 1);
        } except (W32ExceptionHandler(TRUE, RIP_WARNING)) {

            /*
             * Desktop mapping failed.
             */
            CloseProtectedHandle(hdesk);

            LogDesktop(pdesk, LD_DEREF_FN_CREATEDESKTOP2, FALSE, (ULONG_PTR)PtiCurrent());

            ObDereferenceObject(pdesk);
            RIPNTERR0(STATUS_ACCESS_DENIED, RIP_WARNING, "Desktop mapping failed (2)");
            return NULL;
        }

        UserAssert(GetDesktopView(PpiFromProcess(gpepCSRSS), pdesk) != NULL);
    }

    /*
     * Set hook flags
     */
    SetHandleFlag(hdesk, HF_DESKTOPHOOK, dwFlags & DF_ALLOWOTHERACCOUNTHOOK);

    /*
     * Set up to create the desktop window.
     */
    fWasNull = (ptiCurrent->ppi->rpdeskStartup == NULL);
    pdeskTemp = ptiCurrent->rpdesk;            // save current desktop
    hdeskTemp = ptiCurrent->hdesk;

    /*
     * Switch ppi values so window will be created using the
     * system's desktop window class.
     */
    ppiSave  = ptiCurrent->ppi;
    ptiCurrent->ppi = pTerm->ptiDesktop->ppi;

    DeferWinEventNotify();
    BeginAtomicCheck();

    zzzSetDesktop(ptiCurrent, pdesk, hdesk);

    /*
     * Create the desktop window
     */
    /*
     * HACK HACK HACK!!! (adams) In order to create the desktop window
     * with the correct desktop, we set the desktop of the current thread
     * to the new desktop. But in so doing we allow hooks on the current
     * thread to also hook this new desktop. This is bad, because we don't
     * want the desktop window to be hooked while it is created. So we
     * temporarily disable hooks of the current thread and its desktop,
     * and reenable them after switching back to the original desktop.
     */

    dwDisableHooks = ptiCurrent->TIF_flags & TIF_DISABLEHOOKS;
    ptiCurrent->TIF_flags |= TIF_DISABLEHOOKS;

    pwndDesktop = xxxCreateWindowEx(
            (DWORD)0,
            (PLARGE_STRING)DESKTOPCLASS,
            NULL,
            (WS_POPUP | WS_CLIPCHILDREN),
            pdesk->pDispInfo->rcScreen.left,
            pdesk->pDispInfo->rcScreen.top,
            pdesk->pDispInfo->rcScreen.right - pdesk->pDispInfo->rcScreen.left,
            pdesk->pDispInfo->rcScreen.bottom - pdesk->pDispInfo->rcScreen.top,
            NULL,
            NULL,
            hModuleWin,
            NULL,
            VER31);

    if (pwndDesktop == NULL) {
        RIPMSG1(RIP_WARNING,
                "xxxCreateDesktop: Failed to create the desktop window for pdesk %#p",
                pdesk);
        goto Error;
    }

    /*
     * NOTE: In order for the message window to be created without
     * the desktop as it's owner, it needs to be created before
     * setting pdi->spwnd to the desktop window. This is a complete
     * hack and should be fixed.
     */
    pwndMessage = xxxCreateWindowEx(
            (DWORD)0,
            (PLARGE_STRING)gatomMessage,
            NULL,
            (WS_POPUP | WS_CLIPCHILDREN),
            0,
            0,
            100,
            100,
            NULL,
            NULL,
            hModuleWin,
            NULL,
            VER31);

    if (pwndMessage == NULL) {
        RIPMSG0(RIP_WARNING, "xxxCreateDesktop: Failed to create the message window");
        goto Error;
    }

    UserAssert(pdi->spwnd == NULL);

    Lock(&(pdi->spwnd), pwndDesktop);

    SetFullScreen(pwndDesktop, GDIFULLSCREEN);

    /*
     * set this windows to the fullscreen window if we don't have one yet
     */

    /*
     * LATER mikeke
     * this can be a problem if a desktop is created while we are in
     * FullScreenCleanup()
     */

    /*
     * Don't set gspwndFullScreen if fGdiEnabled has been cleared
     * (we may be in the middle of a disconnect).
     */
    UserAssert(fGdiEnabled == TRUE);

    if (gspwndFullScreen == NULL && !(pwinsta->dwWSF_Flags & WSF_NOIO)) {
        Lock(&(gspwndFullScreen), pwndDesktop);
    }

    /*
     * NT Bug 388747: Link the message window to the mother desktop window
     * so that it properly has a parent.  We will do this before we link the
     * desktop window just so the initial message window appears after the
     * initial desktop window (a minor optimization, but not necessary).
     */
    Lock(&pwndMessage->spwndParent, pTerm->spwndDesktopOwner);
    LinkWindow(pwndMessage, NULL, pTerm->spwndDesktopOwner);
    Lock(&pdesk->spwndMessage, pwndMessage);
    Unlock(&pwndMessage->spwndOwner);

    /*
     * Link it as a child but don't use WS_CHILD style
     */
    LinkWindow(pwndDesktop, NULL, pTerm->spwndDesktopOwner);
    Lock(&pwndDesktop->spwndParent, pTerm->spwndDesktopOwner);
    Unlock(&pwndDesktop->spwndOwner);

    /*
     * Make it regional if it's display configuration is regional.
     */
    if (!pdesk->pDispInfo->fDesktopIsRect) {
        pwndDesktop->hrgnClip = pdesk->pDispInfo->hrgnScreen;
    }

    /*
     * Create shared menu window and tooltip window
     */
    ThreadLock(pdesk->spwndMessage, &tlpwnd);

    /*
     * Create the tooltip window only for desktops in
     * interactive windowstations.
     */
    if (!(pwinsta->dwWSF_Flags & WSF_NOIO)) {
        pwndTooltip = xxxCreateWindowEx(
                WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
                (PLARGE_STRING)TOOLTIPCLASS,
                NULL,
                WS_POPUP | WS_BORDER,
                0,
                0,
                100,
                100,
                pdesk->spwndMessage,
                NULL,
                hModuleWin,
                NULL,
                VER31);


        if (pwndTooltip == NULL) {
            ThreadUnlock(&tlpwnd);
            RIPMSG0(RIP_WARNING, "xxxCreateDesktop: Failed to create the tooltip window");
            goto Error;
        }

        Lock(&pdesk->spwndTooltip, pwndTooltip);
    }

    pwndMenu = xxxCreateWindowEx(
            WS_EX_TOOLWINDOW | WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE,
            (PLARGE_STRING)MENUCLASS,
            NULL,
            WS_POPUP | WS_BORDER,
            0,
            0,
            100,
            100,
            pdesk->spwndMessage,
            NULL,
            hModuleWin,
            NULL,
            WINVER);

    ThreadUnlock(&tlpwnd);

    if (pwndMenu == NULL) {
        RIPMSG0(RIP_WARNING, "xxxCreateDesktop: Failed to create the menu window");
        goto Error;
    }

    Lock(&(pdesk->spwndMenu), pwndMenu);

    /*
     * Set the flag in the popupmenu structure that tells this
     * popup menu belongs to the pdesk->spwndMenu
     */
    ((PMENUWND)pdesk->spwndMenu)->ppopupmenu->fDesktopMenu = TRUE;
    /*
     * Unlock spwndPopupMenu since the menu is not in use but mainly
     *  so we won't have to special case this later when the menu is used.
     */
    Unlock(&((PMENUWND)pdesk->spwndMenu)->ppopupmenu->spwndPopupMenu);

    HMChangeOwnerThread(pdi->spwnd, pTerm->ptiDesktop);
    HMChangeOwnerThread(pwndMessage, pTerm->ptiDesktop);
    HMChangeOwnerThread(pdesk->spwndMenu, pTerm->ptiDesktop);

    if (!(pwinsta->dwWSF_Flags & WSF_NOIO)) {
        HMChangeOwnerThread(pwndTooltip, pTerm->ptiDesktop);
    }

    /*
     * Restore caller's ppi
     */
    PtiCurrent()->ppi = ppiSave;

    /*
     * HACK HACK HACK (adams): Renable hooks.
     */
    UserAssert(ptiCurrent->TIF_flags & TIF_DISABLEHOOKS);
    ptiCurrent->TIF_flags = (ptiCurrent->TIF_flags & ~TIF_DISABLEHOOKS) | dwDisableHooks;

    /*
     * Restore the previous desktop
     */
    zzzSetDesktop(ptiCurrent, pdeskTemp, hdeskTemp);

    EndAtomicCheck();
    UserAssert(dwCritSecUseSave == gdwCritSecUseCount);
    zzzEndDeferWinEventNotify();

    /*
     * If this is the first desktop, let the worker threads run now
     * that there is someplace to send input to.  Reassign the event
     * to handle desktop destruction.
     */
    if (pTerm->pEventInputReady != NULL) {

        /*
         * Set the windowstation for RIT and desktop thread
         * so when EventInputReady is signaled the RIT and the desktop
         * will have a windowstation.
         */
        if (!(pTerm->dwTERMF_Flags & TERMF_NOIO)) {
            gptiRit->pwinsta = pwinsta;
        } else {
            /*
             * let the desktop thread of the system terminal have
             * a rpdesk.
             */
            zzzSetDesktop(pTerm->ptiDesktop, pdesk, NULL);
        }

        pTerm->ptiDesktop->pwinsta = pwinsta;

        KeSetEvent(pTerm->pEventInputReady, EVENT_INCREMENT, FALSE);

        if (!(pTerm->dwTERMF_Flags & TERMF_NOIO)) {

            LeaveCrit();
            while (grpdeskRitInput == NULL) {
                UserSleep(20);
                RIPMSG0(RIP_WARNING, "Waiting for grpdeskRitInput to be set ...");
            }
            EnterCrit();
        }

        ObDereferenceObject(pTerm->pEventInputReady);
        pTerm->pEventInputReady = NULL;
    }


    /*
     * HACK HACK:
     * LATER
     *
     * If we have a devmode passed in, then switch desktops ...
     */

    if (ccxlpdevmode) {

        TRACE_INIT(("xxxCreateDesktop: about to call switch desktop\n"));

        bSuccess = xxxSwitchDesktop(pwinsta, pdesk, TRUE);
        if (!bSuccess) {
            RIPMSG0(RIP_ERROR, "Failed to switch desktop on Create\n");
        }

    } else if (pTerm == &gTermIO){

        UserAssert(grpdeskRitInput != NULL);

        /*
         * Force the window to the bottom of the z-order if there
         * is an active desktop so any drawing done on the desktop
         * window will not be seen.  This will also allow
         * IsWindowVisible to work for apps on invisible
         * desktops.
         */
        ThreadLockWithPti(ptiCurrent, pwndDesktop, &tlpwnd);
        xxxSetWindowPos(pwndDesktop, PWND_BOTTOM, 0, 0, 0, 0,
                    SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOMOVE |
                    SWP_NOREDRAW | SWP_NOSIZE | SWP_NOSENDCHANGING);
        ThreadUnlock(&tlpwnd);
    }

    /*
     * If it was null when we came in, make it null going out, or else
     * we'll have the wrong desktop selected into this.
     */
    if (fWasNull)
        UnlockDesktop(&ptiCurrent->ppi->rpdeskStartup,
                      LDU_PPI_DESKSTARTUP1, (ULONG_PTR)(ptiCurrent->ppi));

    if (gbRemoteSession &&
        gspdeskDisconnect == NULL &&
        pdesk == grpdeskLogon) {

        UserAssert(hdesk != NULL);

        /*
         * Create the 'disconnect' desktop
         */
        if (!xxxCreateDisconnectDesktop(hwinsta, pwinsta)) {
            RIPMSG0(RIP_WARNING, "Failed to create the 'disconnect' desktop");

            LogDesktop(pdesk, LD_DEREF_FN_CREATEDESKTOP3, FALSE, (ULONG_PTR)PtiCurrent());
            ObDereferenceObject(pdesk);

            xxxCloseDesktop(hdesk, KernelMode);

            return NULL;
        }

        /*
         * Signal that the disconnect desktop got created.
         */
        KeSetEvent(gpEventDiconnectDesktop, EVENT_INCREMENT, FALSE);

        HYDRA_HINT(HH_DISCONNECTDESKTOP);
    }

Cleanup:

    LogDesktop(pdesk, LD_DEREF_FN_CREATEDESKTOP3, FALSE, (ULONG_PTR)PtiCurrent());
    ObDereferenceObject(pdesk);

    TRACE_INIT(("xxxCreateDesktop: Leaving\n"));

    if (hdesk != NULL) {
        SetHandleFlag(hdesk, HF_PROTECTED, TRUE);
    }
    return hdesk;

Error:

    EndAtomicCheck();
    UserAssert(dwCritSecUseSave == gdwCritSecUseCount);
    zzzEndDeferWinEventNotify();

    UserAssert(pwndMenu == NULL);

    if (pwndTooltip != NULL) {
        xxxDestroyWindow(pwndTooltip);
        Unlock(&pdesk->spwndTooltip);
    }
    if (pwndMessage != NULL) {
        xxxDestroyWindow(pwndMessage);
        Unlock(&pdesk->spwndMessage);
    }
    if (pwndDesktop != NULL) {
        xxxDestroyWindow(pwndDesktop);
        Unlock(&pdi->spwnd);
        Unlock(&gspwndFullScreen);
    }
    /*
     * Restore caller's ppi
     */
    PtiCurrent()->ppi = ppiSave;

    UserAssert(ptiCurrent->TIF_flags & TIF_DISABLEHOOKS);
    ptiCurrent->TIF_flags = (ptiCurrent->TIF_flags & ~TIF_DISABLEHOOKS) | dwDisableHooks;
    zzzSetDesktop(ptiCurrent, pdeskTemp, hdeskTemp);

    CloseProtectedHandle(hdesk);
    hdesk = NULL;

    /*
     * If it was null when we came in, make it null going out, or else
     * we'll have the wrong desktop selected into this.
     */
    if (fWasNull)
        UnlockDesktop(&ptiCurrent->ppi->rpdeskStartup,
                      LDU_PPI_DESKSTARTUP1, (ULONG_PTR)(ptiCurrent->ppi));

    goto Cleanup;

}

/***************************************************************************\
* ParseDesktop
*
* Parse a desktop path.
*
* History:
* 14-Jun-1995 JimA      Created.
\***************************************************************************/

NTSTATUS ParseDesktop(
    PVOID                        pContainerObject,
    POBJECT_TYPE                 pObjectType,
    PACCESS_STATE                pAccessState,
    KPROCESSOR_MODE              AccessMode,
    ULONG                        Attributes,
    PUNICODE_STRING              pstrCompleteName,
    PUNICODE_STRING              pstrRemainingName,
    PVOID                        Context,
    PSECURITY_QUALITY_OF_SERVICE pqos,
    PVOID                        *pObject)
{
    PWINDOWSTATION  pwinsta = pContainerObject;
    PDESKTOP        pdesk;
    PUNICODE_STRING pstrName;
    NTSTATUS        Status = STATUS_OBJECT_NAME_NOT_FOUND;

    *pObject = NULL;

    BEGIN_REENTERCRIT();

    UserAssert(OBJECT_TO_OBJECT_HEADER(pContainerObject)->Type == *ExWindowStationObjectType);
    UserAssert(pObjectType == *ExDesktopObjectType);

    /*
     * See if the desktop exists
     */
    for (pdesk = pwinsta->rpdeskList; pdesk != NULL; pdesk = pdesk->rpdeskNext) {
        pstrName = POBJECT_NAME(pdesk);
        if (pstrName && RtlEqualUnicodeString(pstrRemainingName, pstrName,
                (BOOLEAN)((Attributes & OBJ_CASE_INSENSITIVE) != 0))) {
            if (Context != NULL) {
                if (!(Attributes & OBJ_OPENIF)) {

                    /*
                     * We are attempting to create a desktop and one
                     * already exists.
                     */
                    Status = STATUS_OBJECT_NAME_COLLISION;
                    goto Exit;

                } else {
                    Status = STATUS_OBJECT_NAME_EXISTS;
                }
            } else {
                Status = STATUS_SUCCESS;
            }

            ObReferenceObject(pdesk);

            *pObject = pdesk;
            goto Exit;
        }
    }

    /*
     * Handle creation request
     */
    if (Context != NULL) {
        Status = xxxCreateDesktop2(pContainerObject,
                                   pAccessState,
                                   AccessMode,
                                   pstrRemainingName,
                                   Context,
                                   pObject);
    }

Exit:
    END_REENTERCRIT();

    return Status;

    UNREFERENCED_PARAMETER(pObjectType);
    UNREFERENCED_PARAMETER(pstrCompleteName);
    UNREFERENCED_PARAMETER(pqos);
}

/***************************************************************************\
* DestroyDesktop
*
* Called upon last close of a desktop to remove the desktop from the
* desktop list and free all desktop resources.
*
* History:
* 08-Dec-1993 JimA      Created.
\***************************************************************************/

BOOL DestroyDesktop(
    PDESKTOP pdesk)
{
    PWINDOWSTATION pwinsta = pdesk->rpwinstaParent;
    PTERMINAL      pTerm;
    PDESKTOP       *ppdesk;

    if (pdesk->dwDTFlags & DF_DESTROYED) {
        RIPMSG1(RIP_WARNING, "DestroyDesktop: Already destroyed:%#p", pdesk);
        return FALSE;
    }

    /*
     * Unlink the desktop, if it has not yet been unlinked.
     */
    if (pwinsta != NULL) {

        ppdesk = &pwinsta->rpdeskList;
        while (*ppdesk != NULL && *ppdesk != pdesk) {
            ppdesk = &((*ppdesk)->rpdeskNext);
        }

        if (*ppdesk != NULL) {

            /*
             * remove desktop from the list
             */
            LockDesktop(ppdesk, pdesk->rpdeskNext, LDL_WINSTA_DESKLIST2, (ULONG_PTR)pwinsta);
            UnlockDesktop(&pdesk->rpdeskNext, LDU_DESK_DESKNEXT, (ULONG_PTR)pwinsta);
        }
    }

    /*
     * Link it into the destruction list and signal the desktop thread.
     */
    pTerm = pwinsta->pTerm;

    LockDesktop(&pdesk->rpdeskNext, pTerm->rpdeskDestroy, LDL_DESK_DESKNEXT2, 0);
    LockDesktop(&pTerm->rpdeskDestroy, pdesk, LDL_TERM_DESKDESTROY2, (ULONG_PTR)pTerm);
    KeSetEvent(pTerm->pEventDestroyDesktop, EVENT_INCREMENT, FALSE);

    pdesk->dwDTFlags |= DF_DESTROYED;

    TRACE_DESKTOP(("pdesk %#p '%ws' marked as destroyed\n", pdesk, GetDesktopName(pdesk)));

    return TRUE;
}

/***************************************************************************\
* FreeDesktop
*
* Called to free desktop object and section when last lock is released.
*
* History:
* 08-Dec-1993 JimA      Created.
\***************************************************************************/

VOID FreeDesktop(
    PVOID pobj)
{
    PDESKTOP pdesk = (PDESKTOP)pobj;
    NTSTATUS Status;

    BEGIN_REENTERCRIT();

    UserAssert(OBJECT_TO_OBJECT_HEADER(pobj)->Type == *ExDesktopObjectType);

#ifdef LOGDESKTOPLOCKS

    if (pdesk->pLog != NULL) {

        /*
         * By the time we get here the lock count for lock/unlock
         * tracking code should be 0
         */
        if (pdesk->nLockCount != 0) {
            RIPMSG3(RIP_WARNING,
                    "FreeDesktop pdesk %#p, pLog %#p, nLockCount %d should be 0",
                    pdesk, pdesk->pLog, pdesk->nLockCount);
        }
        UserFreePool(pdesk->pLog);
        pdesk->pLog = NULL;
    }
#endif // LOGDESKTOPLOCKS

#if DBG
    if (pdesk->pDeskInfo && (pdesk->pDeskInfo->spwnd != NULL)) {

        /*
         * assert if the desktop has a desktop window but the flag
         * that says the window is destroyed is not set
         */
        UserAssert(pdesk->dwDTFlags & DF_DESKWNDDESTROYED);
    }
#endif // DBG

    /*
     * Mark the desktop as dying.  Make sure we aren't recursing.
     */
    UserAssert(!(pdesk->dwDTFlags & DF_DYING));
    pdesk->dwDTFlags |= DF_DYING;

#ifdef DEBUG_DESK
    {
        /*
         * Verify that the desktop has been cleaned out.
         */
#if 0
        PPROCESSINFO ppi;
        PCLS pcls, pclsClone;
#endif
        PHE pheT, pheMax;
        BOOL fDirty = FALSE;

#if 0
        for (ppi = gppiFirst; ppi != NULL; ppi = ppi->ppiNext) {
            for (pcls = ppi->pclsPrivateList; pcls != NULL; pcls = pcls->pclsNext) {
                if (pcls->pheapDesktop == pdesk->pheapDesktop) {
                    DbgPrint("ppi %08x private class at %08x exists\n", ppi, pcls);
                    fDirty = TRUE;
                }
                for (pclsClone = pcls->pclsClone; pclsClone != NULL;
                        pclsClone = pclsClone->pclsNext) {
                    if (pclsClone->pheapDesktop == pdesk->pheapDesktop) {
                        DbgPrint("ppi %08x private class clone at %08x exists\n", ppi, pclsClone);
                        fDirty = TRUE;
                    }
                }
            }
            for (pcls = ppi->pclsPublicList; pcls != NULL; pcls = pcls->pclsNext) {
                if (pcls->pheapDesktop == pdesk->pheapDesktop) {
                    DbgPrint("ppi %08x public class at %08x exists\n", ppi, pcls);
                    fDirty = TRUE;
                }
                for (pclsClone = pcls->pclsClone; pclsClone != NULL;
                        pclsClone = pclsClone->pclsNext) {
                    if (pclsClone->pheapDesktop == pdesk->pheapDesktop) {
                        DbgPrint("ppi %08x public class clone at %08x exists\n", ppi, pclsClone);
                        fDirty = TRUE;
                    }
                }
            }
        }
#endif

        pheMax = &gSharedInfo.aheList[giheLast];
        for (pheT = gSharedInfo.aheList; pheT <= pheMax; pheT++) {
            switch (pheT->bType) {
                case TYPE_WINDOW:
                    if (((PWND)pheT->phead)->head.rpdesk == pdesk) {
                        DbgPrint("Window at %08x exists\n", pheT->phead);
                        break;
                    }
                    continue;
                case TYPE_MENU:
                    if (((PMENU)pheT->phead)->head.rpdesk == pdesk) {
                        DbgPrint("Menu at %08x exists\n", pheT->phead);
                        break;
                    }
                    continue;
                case TYPE_CALLPROC:
                    if (((PCALLPROCDATA)pheT->phead)->head.rpdesk == pdesk) {
                        DbgPrint("Callproc at %08x exists\n", pheT->phead);
                        break;
                    }
                    continue;
                case TYPE_HOOK:
                    if (((PHOOK)pheT->phead)->head.rpdesk == pdesk) {
                        DbgPrint("Hook at %08x exists\n", pheT->phead);
                        break;
                    }
                    continue;
                default:
                    continue;
            }
            fDirty = TRUE;
        }
        if (fDirty) {
            RIPMSG0(RIP_ERROR,"Desktop cleanup failed\n");
        }
    }
#endif

    /*
     * If the desktop is mapped into CSR, unmap it.  Note the
     * handle count values passed in will cause the desktop
     * to be unmapped and skip the desktop destruction tests.
     */
    FreeView(gpepCSRSS, pdesk);

    if (pdesk->pheapDesktop != NULL) {

        PVOID hheap = Win32HeapGetHandle(pdesk->pheapDesktop);

        Win32HeapDestroy(pdesk->pheapDesktop);

        Status = Win32UnmapViewInSessionSpace(hheap);

        UserAssert(NT_SUCCESS(Status));
        Win32DestroySection(pdesk->hsectionDesktop);
    }

    UnlockWinSta(&pdesk->rpwinstaParent);

    DbgTrackRemoveDesktop(pdesk);

    END_REENTERCRIT();
}

/***************************************************************************\
* CreateDesktopHeap
*
* Create a new desktop heap
*
* History:
* 27-Jul-1992 JimA      Created.
\***************************************************************************/

HANDLE CreateDesktopHeap(
    PWIN32HEAP* ppheapRet,
    ULONG       ulHeapSize)
{
    HANDLE        hsection;
    LARGE_INTEGER SectionSize;
    SIZE_T        ulViewSize;
    NTSTATUS      Status;
    PWIN32HEAP    pheap;
    PVOID         pHeapBase;

    /*
     * Create desktop heap section and map it into the kernel
     */
    SectionSize.QuadPart = ulHeapSize;

    Status = Win32CreateSection(&hsection,
                                SECTION_ALL_ACCESS,
                                (POBJECT_ATTRIBUTES)NULL,
                                &SectionSize,
                                PAGE_EXECUTE_READWRITE,
                                SEC_RESERVE,
                                (HANDLE)NULL,
                                NULL,
                                TAG_SECTION_DESKTOP);

    if (!NT_SUCCESS( Status )) {
        RIPNTERR0(Status, RIP_WARNING, "Can't create section for desktop heap.");
        return NULL;
    }

    ulViewSize = ulHeapSize;
    pHeapBase = NULL;

    Status = Win32MapViewInSessionSpace(hsection, &pHeapBase, &ulViewSize);

    if (!NT_SUCCESS(Status)) {
        RIPNTERR0(Status,
                  RIP_WARNING,
                  "Can't map section for desktop heap into system space.");
        goto Error;
    }

    /*
     * Create desktop heap.
     */
    if ((pheap = UserCreateHeap(
            hsection,
            0,
            pHeapBase,
            ulHeapSize,
            UserCommitDesktopMemory)) == NULL) {

        RIPERR0(ERROR_NOT_ENOUGH_MEMORY, RIP_WARNING, "Can't create Desktop heap.");

        Win32UnmapViewInSessionSpace(pHeapBase);
Error:
        Win32DestroySection(hsection);
        *ppheapRet = NULL;
        return NULL;
    }

    UserAssert(Win32HeapGetHandle(pheap) == pHeapBase);
    *ppheapRet = pheap;

    return hsection;
}

/***************************************************************************\
* GetDesktopView
*
* Determines if a desktop has already been mapped into a process.
*
* History:
* 10-Apr-1995 JimA      Created.
\***************************************************************************/

PDESKTOPVIEW GetDesktopView(
    PPROCESSINFO ppi,
    PDESKTOP     pdesk)
{
    PDESKTOPVIEW pdv;

    if(ppi->Process != gpepCSRSS && pdesk == NULL) {
        RIPMSG1(RIP_WARNING, "Process %#p isn't CSRSS but pdesk is NULL in GetDesktopView", ppi);
    }

    for (pdv = ppi->pdvList; pdv != NULL; pdv = pdv->pdvNext)
        if (pdv->pdesk == pdesk)
            break;
    return pdv;
}

/***************************************************************************\
* _MapDesktopObject
*
* Maps a desktop object into the client's address space
*
* History:
* 11-Apr-1995 JimA      Created.
\***************************************************************************/

PVOID _MapDesktopObject(
    HANDLE h)
{
    PDESKOBJHEAD  pobj;
    PDESKTOPVIEW pdv;

    /*
     * Validate the handle
     */
    pobj = HMValidateHandle(h, TYPE_GENERIC);
    if (pobj == NULL)
        return NULL;

    UserAssert(HMObjectFlags(pobj) & OCF_DESKTOPHEAP);

    /*
     * Locate the client's view of the desktop.  Realistically,
     * this should never fail for valid objects.
     */
    pdv = GetDesktopView(PpiCurrent(), pobj->rpdesk);
    if (pdv == NULL) {
        RIPMSG1(RIP_WARNING, "MapDesktopObject: can not map handle %#p", h);
        return NULL;
    }

    UserAssert(pdv->ulClientDelta != 0);
    return (PVOID)((PBYTE)pobj - pdv->ulClientDelta);
}

/***************************************************************************\
* MapDesktop
*
* Attempts to map a desktop heap into a process.
*
* History:
* 20-Oct-1994 JimA      Created.
\***************************************************************************/

VOID MapDesktop(
    OB_OPEN_REASON OpenReason,
    PEPROCESS      Process,
    PVOID          pobj,
    ACCESS_MASK    amGranted,
    ULONG          cHandles)
{
    PPROCESSINFO  ppi;
    PDESKTOP      pdesk = (PDESKTOP)pobj;
    NTSTATUS      Status;
    SIZE_T        ulViewSize;
    LARGE_INTEGER liOffset;
    PDESKTOPVIEW  pdvNew;
    PBYTE         pClientBase;
    BOOL          bFailed = FALSE;

    UserAssert(OBJECT_TO_OBJECT_HEADER(pobj)->Type == *ExDesktopObjectType);

    /*
     * Ignore handle inheritance because MmMapViewOfSection
     * cannot be called during process creation.
     */
    if (OpenReason == ObInheritHandle)
        return;

    /*
     * If there is no ppi, we can't map the desktop
     */
    ppi = PpiFromProcess(Process);
    if (ppi == NULL)
        return;

    /*
     * If the desktop has already been mapped we're done.
     */
    if (GetDesktopView(ppi, pdesk) != NULL)
        return;

    /*
     * Allocate a view of the desktop
     */
    pdvNew = UserAllocPoolWithQuota(sizeof(*pdvNew), TAG_PROCESSINFO);
    if (pdvNew == NULL) {
        RIPERR0(ERROR_NOT_ENOUGH_MEMORY, RIP_VERBOSE, "");

        /*
         * Raise an exception so that the object manager
         * knows OpenProcedure wasn't successful
         */
        ExRaiseStatus(STATUS_NO_MEMORY);
        return;
    }

    BEGIN_REENTERCRIT();

    /*
     * Read/write access has been granted.  Map the desktop
     * memory into the client process.
     */
    ulViewSize = 0;
    liOffset.QuadPart = 0;
    pClientBase = NULL;

    Status = MmMapViewOfSection(pdesk->hsectionDesktop,
                                Process,
                                &pClientBase,
                                0,
                                0,
                                &liOffset,
                                &ulViewSize,
                                ViewUnmap,
                                SEC_NO_CHANGE,
                                PAGE_EXECUTE_READ);

    if (!NT_SUCCESS(Status)) {
#if DBG
        if (    Status != STATUS_NO_MEMORY &&
                Status != STATUS_PROCESS_IS_TERMINATING &&
                Status != STATUS_COMMITMENT_LIMIT) {
            RIPMSG1(RIP_WARNING, "MapDesktop - failed to map to client process (status == %lX). Contact ChrisWil",Status);
        }
#endif

        RIPNTERR0(Status, RIP_VERBOSE, "");
        UserFreePool(pdvNew);
        bFailed = TRUE;
        goto Exit;
    }

    /*
     * Link the view into the ppi
     */
    pdvNew->pdesk         = pdesk;
    pdvNew->ulClientDelta = (ULONG_PTR)((PBYTE)Win32HeapGetHandle(pdesk->pheapDesktop) - pClientBase);
    pdvNew->pdvNext       = ppi->pdvList;
    ppi->pdvList          = pdvNew;

Exit:
    END_REENTERCRIT();

    /*
     * If something failed raise an exception so that the object manager
     * knows OpenProcedure wasn't successful
     */
    if (bFailed) {
        ExRaiseStatus(STATUS_NO_MEMORY);
    }

    UNREFERENCED_PARAMETER(cHandles);
    UNREFERENCED_PARAMETER(amGranted);
}


VOID FreeView(
    PEPROCESS Process,
    PDESKTOP pdesk)
{
    PPROCESSINFO ppi;
    NTSTATUS     Status;
    PDESKTOPVIEW pdv;
    PDESKTOPVIEW *ppdv;

    /*
     * Bug 277291: gpepCSRSS can be NULL when FreeView is
     * called from FreeDesktop.
     */
    if (Process == NULL) {
        return;
    }

    ppi = PpiFromProcess(Process);

    /*
     * If there is no ppi, then the process is gone and nothing
     * needs to be unmapped.
     */
    if (ppi != NULL) {
        pdv = GetDesktopView(ppi, pdesk);

        /*
         * Because mapping cannot be done when a handle is
         * inherited, there may not be a view of the desktop.
         * Only unmap if there is a view.
         */
        if (pdv != NULL) {
            Status = MmUnmapViewOfSection(Process,
                    (PBYTE)Win32HeapGetHandle(pdesk->pheapDesktop) - pdv->ulClientDelta);
            UserAssert(NT_SUCCESS(Status) || Status == STATUS_PROCESS_IS_TERMINATING);
            if (!NT_SUCCESS(Status)) {
                RIPMSG1(RIP_WARNING, "FreeView unmap status = 0x%#lx", Status);
            }

            /*
             * Unlink and delete the view.
             */
            for (ppdv = &ppi->pdvList; *ppdv && *ppdv != pdv;
                    ppdv = &(*ppdv)->pdvNext)
                ;
            UserAssert(*ppdv);
             *ppdv = pdv->pdvNext;
            UserFreePool(pdv);
        }

#if DBG
        /*
         * No thread in this process should be on this desktop
         */
        {
            PTHREADINFO pti = ppi->ptiList;
            while (pti != NULL) {
                if (pti->rpdesk == pdesk) {
                    RIPMSG2(RIP_ERROR, "FreeView: pti %#p still on pdesk %#p", pti, pdesk);
                }
                pti = pti->ptiSibling;
            }
        }
#endif
    }
}


VOID UnmapDesktop(
    PEPROCESS   Process,
    PVOID       pobj,
    ACCESS_MASK amGranted,
    ULONG       cProcessHandles,
    ULONG       cSystemHandles)
{
    PDESKTOP pdesk = (PDESKTOP)pobj;

    BEGIN_REENTERCRIT();

    UserAssert(OBJECT_TO_OBJECT_HEADER(pobj)->Type == *ExDesktopObjectType);

    /*
     * Update cSystemHandles with the correct information
     */
    cSystemHandles = OBJECT_TO_OBJECT_HEADER(pobj)->HandleCount + 1;

    /*
     * Only unmap the desktop if this is the last process handle and
     * the process is not CSR.
     */
    if (cProcessHandles == 1 && Process != gpepCSRSS) {
        FreeView(Process, pdesk);
    }

    if (cSystemHandles > 2)
        goto Exit;

    if (cSystemHandles == 2 && pdesk->dwConsoleThreadId != 0) {

        /*
         * If a console thread exists and we're down to two handles,
         * it means that the last application handle to the
         * desktop is being closed.  Terminate the console
         * thread so the desktop can be freed.
         */
        TerminateConsole(pdesk);
    } else if (cSystemHandles == 1) {

        /*
         * If this is the last handle to this desktop in the system,
         * destroy the desktop.
         */

        /*
         * No pti should be linked to this desktop.
         */
        if ((&pdesk->PtiList != pdesk->PtiList.Flink)
                || (&pdesk->PtiList != pdesk->PtiList.Blink)) {

            RIPMSG1(RIP_WARNING, "UnmapDesktop: PtiList not Empty. pdesk:%#p", pdesk);
        }

        DestroyDesktop(pdesk);
    }

Exit:
    END_REENTERCRIT();

    UNREFERENCED_PARAMETER(amGranted);
}

/***************************************************************************\
* OkayToCloseDesktop
*
* We can only close desktop handles if they're not in use.
*
* History:
* 08-Feb-1999 JerrySh   Created.
\***************************************************************************/

BOOLEAN OkayToCloseDesktop(
    PEPROCESS Process OPTIONAL,
    PVOID Object,
    HANDLE Handle)
{
    PDESKTOP pdesk = (PDESKTOP)Object;

    UNREFERENCED_PARAMETER(Process);

    UserAssert(OBJECT_TO_OBJECT_HEADER(Object)->Type == *ExDesktopObjectType);

    /*
     * Kernel mode code can close anything.
     */
    if (KeGetPreviousMode() == KernelMode) {
        return TRUE;
    }

    /*
     * We can't close the desktop if we're still initializing it.
     */
    if (!(pdesk->dwDTFlags & DF_DESKCREATED)) {
        RIPMSG1(RIP_WARNING, "Trying to close desktop %#p during initialization", pdesk);
        return FALSE;
    }

    /*
     * We can't close a desktop that's being used.
     */
    if (CheckHandleInUse(Handle) || CheckHandleFlag(Handle, HF_PROTECTED)) {
        RIPMSG1(RIP_WARNING, "Trying to close desktop %#p while still in use", pdesk);
        return FALSE;
    }

    return TRUE;
}

/***************************************************************************\
* xxxUserResetDisplayDevice
*
* Called to reset the display device after a switch to another device.
* Used when opening a new device, or when switching back to an old desktop
*
* History:
* 31-May-1994 AndreVa   Created.
\***************************************************************************/

VOID xxxUserResetDisplayDevice()
{
    TL tlpwnd;
    TRACE_INIT(("xxxUserResetDisplayDevice: about to reset the device\n"));

    /*
     * Handle early system initialization gracefully.
     */
    if (grpdeskRitInput != NULL) {
        ThreadLock(grpdeskRitInput->pDeskInfo->spwnd, &tlpwnd);
#if 0
        // what should we do here to notify the display applet ?
        // AndreVa

        /*
         * Broadcast that the display has changed resolution.  We are going
         * to specify the desktop for the changing-desktop.  That way we
         * don't get confused as to what desktop to broadcast to.
         */
        xxxBroadcastMessage(grpdeskRitInput->pDeskInfo->spwnd,
                            WM_DISPLAYCHANGE,
                            gpsi->BitCount,
                            MAKELONG(SYSMET(CXSCREEN), SYSMET(CYSCREEN)),
                            BMSG_SENDNOTIFYMSG,
                            NULL);
#endif
        xxxRedrawWindow(grpdeskRitInput->pDeskInfo->spwnd,
                        NULL,
                        NULL,
                        RDW_INVALIDATE | RDW_ERASE | RDW_ERASENOW |
                            RDW_ALLCHILDREN);
        gpqCursor = NULL;

        /*
         * No need to DeferWinEventNotify() - we just made an xxx call above
         */
        zzzInternalSetCursorPos(gpsi->ptCursor.x, gpsi->ptCursor.y);

        SetPointer(TRUE);

        ThreadUnlock(&tlpwnd);
    }

    TRACE_INIT(("xxxUserResetDisplayDevice: complete\n"));

}

/***************************************************************************\
* OpenDesktopCompletion
*
* Verifies that a given desktop has successfully opened.
*
* History:
* 03-Oct-1995 JimA      Created.
\***************************************************************************/

BOOL OpenDesktopCompletion(
    PDESKTOP pdesk,
    HDESK    hdesk,
    DWORD    dwFlags,
    BOOL*    pbShutDown)
{
    PPROCESSINFO   ppi = PpiCurrent();
    PWINDOWSTATION pwinsta;
    BOOL           fMapped;

    /*
     * If the desktop was not mapped in as a result of the open,
     * fail.
     */
    fMapped = (GetDesktopView(ppi, pdesk) != NULL);
    if (!fMapped) {

        RIPMSG0(RIP_WARNING, "OpenDesktopCompletion failed."
                " The desktop is not mapped");

        /*
         * Desktop mapping failed.  Status is set by MapDesktop
         */
        return FALSE;
    } else {

        /*
         * Fail if the windowstation is locked
         */
        pwinsta = pdesk->rpwinstaParent;
        if (pwinsta->dwWSF_Flags & WSF_OPENLOCK &&
                ppi->Process->UniqueProcessId != gpidLogon) {
            LUID luidCaller;
            NTSTATUS Status;

            /*
             * If logoff is occuring and the caller does not
             * belong to the session that is ending, allow the
             * open to proceed.
             */
            Status = GetProcessLuid(NULL, &luidCaller);

            if (!NT_SUCCESS(Status) ||
                    (pwinsta->dwWSF_Flags & WSF_REALSHUTDOWN) ||
                    RtlEqualLuid(&luidCaller, &pwinsta->luidEndSession)) {

                RIPERR0(ERROR_BUSY, RIP_WARNING, "OpenDesktopCompletion failed");

                /*
                 * Set the shut down flag
                 */
                *pbShutDown = TRUE;
                return FALSE;
            }
        }
    }

    SetHandleFlag(hdesk, HF_DESKTOPHOOK, dwFlags & DF_ALLOWOTHERACCOUNTHOOK);

    return TRUE;
}

/***************************************************************************\
* xxxOpenDesktop (API)
*
* Open a desktop object.  This is an 'xxx' function because it leaves the
* critical section while waiting for the windowstation desktop open lock
* to be available.
*
* History:
* 16-Jan-1991 JimA      Created scaffold code.
\***************************************************************************/

HDESK xxxOpenDesktop(
    POBJECT_ATTRIBUTES ccxObjA,
    KPROCESSOR_MODE    AccessMode,
    DWORD              dwFlags,
    DWORD              dwDesiredAccess,
    BOOL*              pbShutDown)
{
    HDESK    hdesk;
    PDESKTOP pdesk;
    NTSTATUS Status;

    /*
     * Require read/write access
     */
    dwDesiredAccess |= DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS;

    /*
     * Open the desktop -- Ob routines capture Obj attributes.
     */
    Status = ObOpenObjectByName(
            ccxObjA,
            *ExDesktopObjectType,
            AccessMode,
            NULL,
            dwDesiredAccess,
            NULL,
            &hdesk);
    if (!NT_SUCCESS(Status)) {
        RIPNTERR0(Status, RIP_VERBOSE, "");
        return NULL;
    }

    /*
     * Reference the desktop
     */
    ObReferenceObjectByHandle(
            hdesk,
            0,
            *ExDesktopObjectType,
            KernelMode,
            &pdesk,
            NULL);
    if (!NT_SUCCESS(Status)) {
        RIPNTERR0(Status, RIP_VERBOSE, "");

Error:
        CloseProtectedHandle(hdesk);
        return NULL;
    }

    if (pdesk->dwSessionId != gSessionId) {
        RIPNTERR1(STATUS_INVALID_HANDLE, RIP_WARNING,
                  "xxxOpenDesktop pdesk %#p belongs to a different session",
                  pdesk);
        ObDereferenceObject(pdesk);
        goto Error;
    }

    LogDesktop(pdesk, LD_REF_FN_OPENDESKTOP, TRUE, (ULONG_PTR)PtiCurrent());

    /*
     * Complete the desktop open
     */
    if (!OpenDesktopCompletion(pdesk, hdesk, dwFlags, pbShutDown)) {
        CloseProtectedHandle(hdesk);
        hdesk = NULL;
    }

    TRACE_INIT(("xxxOpenDesktop: Leaving\n"));

    LogDesktop(pdesk, LD_DEREF_FN_OPENDESKTOP, FALSE, (ULONG_PTR)PtiCurrent());
    ObDereferenceObject(pdesk);

    if (hdesk != NULL) {
        SetHandleFlag(hdesk, HF_PROTECTED, TRUE);
    }

    return hdesk;
}

/***************************************************************************\
* xxxSwitchDesktop (API)
*
* Switch input focus to another desktop and bring it to the top of the
* desktops
*
* bCreateNew is set when a new desktop has been created on the device, and
* when we do not want to send another enable\disable
*
* History:
* 16-Jan-1991 JimA      Created scaffold code.
\***************************************************************************/

BOOL xxxSwitchDesktop(
    PWINDOWSTATION pwinsta,
    PDESKTOP       pdesk,
    BOOL           bCreateNew)
{
    PETHREAD    Thread;
    PWND        pwndSetForeground;
    TL          tlpwndChild;
    TL          tlpwnd;
    TL          tlhdesk;
    PQ          pq;
    BOOL        bUpdateCursor = FALSE;
    PLIST_ENTRY pHead, pEntry;
    PTHREADINFO pti;
    PTHREADINFO ptiCurrent = PtiCurrent();
    PTERMINAL   pTerm;
    NTSTATUS    Status;
    HDESK       hdesk;

    CheckCritIn();

    UserAssert(IsWinEventNotifyDeferredOK());

    if (pdesk == NULL) {
        return FALSE;
    }

    if (pdesk == grpdeskRitInput) {
        return TRUE;
    }

    if (pdesk->dwDTFlags & DF_DESTROYED) {
        RIPMSG1(RIP_ERROR, "xxxSwitchDesktop: destroyed:%#p", pdesk);
        return FALSE;
    }

    UserAssert(!(pdesk->dwDTFlags & (DF_DESKWNDDESTROYED | DF_DYING)));

    if (pwinsta == NULL)
        pwinsta = pdesk->rpwinstaParent;

    /*
     * Get the windowstation, and assert if this process doesn't have one.
     */
    UserAssert(pwinsta);
    if (pwinsta == NULL) {
        RIPMSG1(RIP_WARNING,
                "xxxSwitchDesktop: failed for pwinsta NULL pdesk %#p", pdesk);
        return FALSE;
    }

    /*
     * Don't allow invisible desktops to become active
     */
    if (pwinsta->dwWSF_Flags & WSF_NOIO) {
        RIPMSG1(RIP_VERBOSE,
                "xxxSwitchDesktop: failed for NOIO pdesk %#p", pdesk);
        return FALSE;
    }

    pTerm = pwinsta->pTerm;

    UserAssert(grpdeskRitInput == pwinsta->pdeskCurrent);

    /*
     * Do tracing only if compiled in.
     */
    TRACE_INIT(("xxxSwitchDesktop: Entering, desktop = %ws, createdNew = %01lx\n", POBJECT_NAME(pdesk), (DWORD)bCreateNew));
    if (grpdeskRitInput) {
        TRACE_INIT(("               coming from desktop = %ws\n", POBJECT_NAME(grpdeskRitInput)));
    }

    /*
     * Wait if the logon has the windowstation locked
     */
    Thread = PsGetCurrentThread();

    /*
     * Allow switches to the disconnected desktop
     */
    if (pdesk != gspdeskDisconnect)
        if (!IS_SYSTEM_THREAD(Thread) && pwinsta->dwWSF_Flags & WSF_SWITCHLOCK &&
                pdesk != grpdeskLogon &&
                Thread->Cid.UniqueProcess != gpidLogon)
            return FALSE;

    /*
     * We don't allow switching away from the disconnect desktop.
     */
    if (gbDesktopLocked && ((!gspdeskDisconnect) || (pdesk != gspdeskDisconnect))) {
        TRACE_DESKTOP(("Attempt to switch away from the disconnect desktop\n"));

        /*
         * we should not lock this global !!! clupu
         */
        LockDesktop(&gspdeskShouldBeForeground, pdesk, LDL_DESKSHOULDBEFOREGROUND1, 0);
        return TRUE;
    }

    /*
     * HACKHACK LATER !!!
     * Where should we really switch the desktop ...
     * And we need to send repaint messages to everyone...
     *
     */

    UserAssert(grpdeskRitInput == pwinsta->pdeskCurrent);

    if (!bCreateNew && grpdeskRitInput &&
        (grpdeskRitInput->pDispInfo->hDev != pdesk->pDispInfo->hDev)) {

        if (!DrvDisableMDEV(grpdeskRitInput->pDispInfo->pmdev, TRUE)) {
            RIPMSG1(RIP_WARNING, "xxxSwitchDesktop: DrvDisableMDEV failed for pdesk %#p",
                    grpdeskRitInput);
            return FALSE;
        }
        DrvEnableMDEV(pdesk->pDispInfo->pmdev, TRUE);
        bUpdateCursor = TRUE;

    }

    /*
     * Grab a handle to the pdesk
     */
    Status = ObOpenObjectByPointer(pdesk,
                                   0,
                                   NULL,
                                   EVENT_ALL_ACCESS,
                                   NULL,
                                   KernelMode,
                                   &hdesk);
    if (!NT_SUCCESS(Status)) {

        RIPMSG2(RIP_ERROR, "Could not get a handle for pdesk %#p Status %x",
                pdesk, Status);
        return FALSE;
    }

    ThreadLockDesktopHandle(ptiCurrent, &tlhdesk, hdesk);

#if DBG
    /*
     * The current desktop is now the new desktop.
     */
    pwinsta->pdeskCurrent = pdesk;
#endif // DBG

    /*
     * Kill any journalling that is occuring.  If an app is journaling to
     * the CoolSwitch window, zzzCancelJournalling() will kill the window.
     */
    if (ptiCurrent->rpdesk != NULL)
        zzzCancelJournalling();

    /*
     * Remove the cool switch window if it's on the RIT.  Sending the message
     * is OK because the destination is the RIT, which should never block.
     */
    if (gspwndAltTab != NULL) {

        TL   tlpwndT;

        ThreadLockWithPti(ptiCurrent, gspwndAltTab, &tlpwndT);
        xxxSendMessage(gspwndAltTab, WM_CLOSE, 0, 0);
        ThreadUnlock(&tlpwndT);
    }

    /*
     * Remove all trace of previous active window.
     */
    if (grpdeskRitInput != NULL) {

        UserAssert(grpdeskRitInput->spwndForeground == NULL);

        if (grpdeskRitInput->pDeskInfo->spwnd != NULL) {
            if (gpqForeground != NULL) {

                Lock(&grpdeskRitInput->spwndForeground,
                        gpqForeground->spwndActive);

            /*
             * This is an API so ptiCurrent can pretty much be on any
             *  state; it might not be in grpdeskRitInput (current) or
             *  pdesk (the one we're switching to). It can be sharing its
             *  queue with other threads from another desktop.
             * This is tricky because we're calling xxxSetForegroundWindow
             *  and xxxSetWindowPos but PtiCurrent might be on whatever
             *  desktop. We cannot cleanly switch ptiCurrent to the proper
             *  desktop because it might be sharing its queue with other
             *  threads, own windows, hooks, etc. So this is kind of broken.
             *
             * Old Comment:
             * Fixup the current-thread (system) desktop.  This
             * could be needed in case the xxxSetForegroundWindow()
             * calls xxxDeactivate().  There is logic in their which
             * requires the desktop.  This is only needed temporarily
             * for this case.
             *
             * We would only go into xxxDeactivate if
             *  ptiCurrent->pq == qpqForeground; but if this is the case,
             *  then ptiCurrent must be in grpdeskRitInput already. So
             *  I don't think we need this at all. Let's find out.
             * Note that we might switch queues while processing the
             *  xxxSetForegroundWindow call. That should be fine as long
             *  as we don't switch desktops.....
             */
            UserAssert((ptiCurrent->pq != gpqForeground)
                        || (ptiCurrent->rpdesk == grpdeskRitInput));

            /*
             * The SetForegroundWindow call must succed here, so we call
             * xxxSetForegroundWindow2() directly
             */
            xxxSetForegroundWindow2(NULL, ptiCurrent, 0); // WHAT KEEPS pdesk LOCKED - IANJA ???

            }
        }
    }

    /*
     * Post update events to all queues sending input to the desktop
     * that is becoming inactive.  This keeps the queues in sync up
     * to the desktop switch.
     */
    if (grpdeskRitInput != NULL) {

        pHead = &grpdeskRitInput->PtiList;

        for (pEntry = pHead->Flink; pEntry != pHead; pEntry = pEntry->Flink) {

            pti = CONTAINING_RECORD(pEntry, THREADINFO, PtiLink);
            pq  = pti->pq;

            if (pq->QF_flags & QF_UPDATEKEYSTATE)
                PostUpdateKeyStateEvent(pq);

            /*
             * Clear the reset bit to ensure that we can properly
             * reset the key state when this desktop again becomes
             * active.
             */
            pq->QF_flags &= ~QF_KEYSTATERESET;
        }
    }

    /*
     * Send the RIT input to the desktop.  We do this before any window
     * management since DoPaint() uses grpdeskRitInput to go looking for
     * windows with update regions.
     */
    LockDesktop(&grpdeskRitInput, pdesk, LDL_DESKRITINPUT, 0);

    /*
     * Free any spbs that are only valid for the previous desktop.
     */
    FreeAllSpbs();

    /*
     * Lock it into the RIT thread (we could use this desktop rather than
     * the global grpdeskRitInput to direct input!)
     */
    zzzSetDesktop(gptiRit, pdesk, NULL); // DeferWinEventNotify() ?? IANJA ??

    /*
     * Lock the desktop into the desktop thread.  Be sure
     * that the thread is using an unattached queue before
     * setting the desktop.  This is needed to ensure that
     * the thread does not using a shared journal queue
     * for the old desktop.
     */
    if (pTerm->ptiDesktop->pq != pTerm->pqDesktop) {
        UserAssert(pTerm->pqDesktop->cThreads == 0);
        AllocQueue(NULL, pTerm->pqDesktop);
        pTerm->pqDesktop->cThreads++;
        zzzAttachToQueue(pTerm->ptiDesktop, pTerm->pqDesktop, NULL, FALSE);
    }
    zzzSetDesktop(pTerm->ptiDesktop, pdesk, NULL); // DeferWinEventNotify() ?? IANJA ??

    /*
     * Bring the desktop window to the top and invalidate
     * everything.
     */
    ThreadLockWithPti(ptiCurrent, pdesk->pDeskInfo->spwnd, &tlpwnd);


    /*
     * Suspend DirectDraw before we bring up the desktop window, so we make
     * sure that everything is repainted properly once DirectDraw is disabled.
     */

    GreSuspendDirectDraw(pdesk->pDispInfo->hDev, TRUE);

    xxxSetWindowPos(pdesk->pDeskInfo->spwnd, // WHAT KEEPS pdesk LOCKED - IANJA ???
                    NULL,
                    0,
                    0,
                    0,
                    0,
                    SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE | SWP_NOCOPYBITS);

    /*
     * At this point, my understanding is that the new desktop window has been
     * brought to the front, and therefore the vis-region of any app on any
     * other desktop is now NULL.
     *
     * So this is the appropriate time to resume DirectDraw, which will
     * ensure the DirectDraw app can not draw anything in the future.
     *
     * If this is not the case, then this code needs to be moved to a more
     * appropriate location.
     *
     * [andreva] 6-26-96
     */

    GreResumeDirectDraw(pdesk->pDispInfo->hDev, TRUE);

    /*
     * Find the first visible top-level window.
     */
    pwndSetForeground = pdesk->spwndForeground;
    if (pwndSetForeground == NULL || HMIsMarkDestroy(pwndSetForeground)) {

        pwndSetForeground = pdesk->pDeskInfo->spwnd->spwndChild;

        while ((pwndSetForeground != NULL) &&
                !TestWF(pwndSetForeground, WFVISIBLE)) {

            pwndSetForeground = pwndSetForeground->spwndNext;
        }
    }
    Unlock(&pdesk->spwndForeground);

    /*
     * Now set it to the foreground.
     */

    if (pwndSetForeground == NULL) {
        xxxSetForegroundWindow2(NULL, NULL, 0);
    } else {

        UserAssert(GETPTI(pwndSetForeground)->rpdesk == grpdeskRitInput);
        /*
         * If the new foreground window is a minimized fullscreen app,
         * make it fullscreen.
         */
        if (GetFullScreen(pwndSetForeground) == FULLSCREENMIN) {
            SetFullScreen(pwndSetForeground, FULLSCREEN);
        }

        ThreadLockAlwaysWithPti(ptiCurrent, pwndSetForeground, &tlpwndChild);
        /*
         * The SetForegroundWindow call must succed here, so we call
         * xxxSetForegroundWindow2() directly
         */
        xxxSetForegroundWindow2(pwndSetForeground, ptiCurrent, 0);
        ThreadUnlock(&tlpwndChild);
    }


    ThreadUnlock(&tlpwnd);

    /*
     * Overwrite key state of all queues sending input to the new
     * active desktop with the current async key state.  This
     * prevents apps on inactive desktops from spying on active
     * desktops.  This blows away anything set with SetKeyState,
     * but there is no way of preserving this without giving
     * away information about what keys were hit on other
     * desktops.
     */
    pHead = &grpdeskRitInput->PtiList;
    for (pEntry = pHead->Flink; pEntry != pHead; pEntry = pEntry->Flink) {

        pti = CONTAINING_RECORD(pEntry, THREADINFO, PtiLink);
        pq  = pti->pq;

        if (!(pq->QF_flags & QF_KEYSTATERESET)) {
            pq->QF_flags |= QF_UPDATEKEYSTATE | QF_KEYSTATERESET;
            RtlFillMemory(pq->afKeyRecentDown, CBKEYSTATERECENTDOWN, 0xff);
            PostUpdateKeyStateEvent(pq);
        }
    }

    /*
     * If there is a hard-error popup up, nuke it and notify the
     * hard error thread that it needs to pop it up again.
     */
    if (gHardErrorHandler.pti) {
        IPostQuitMessage(gHardErrorHandler.pti, 0);
    }

    /*
     * Notify anyone waiting for a desktop switch.
     */
    UserAssert(!(pdesk->rpwinstaParent->dwWSF_Flags & WSF_NOIO));

    KePulseEvent(gpEventSwitchDesktop, EVENT_INCREMENT, FALSE);

    /*
     * reset the cursor when we come back from another pdev
     */
    if (bUpdateCursor == TRUE) {

        gpqCursor = NULL;
        zzzInternalSetCursorPos(gpsi->ptCursor.x, gpsi->ptCursor.y);

        SetPointer(TRUE);
    }

    /*
     * Make sure we come back to the right mode when this is all done, because
     * the device may be left in an interesting state if we were running
     * DirectDraw.
     */

    {
        /*
         * Don't check the return code right now since there is nothing
         * we can do if we can not reset the mode ...
         */

            // BUGBUG
            // DONT_CHECKIN

        //UserChangeDisplaySettings(pdesk->pDispInfo->hDevInfo,
        //                          pdesk->pDesktopDevmode,
        //                          _GetDesktopWindow(),
        //                          pdesk,
        //                          0,
        //                          NULL,
        //                          TRUE);
    }


    /*
     * If this desktop was not active during last display settings change
     * let's now bradcast the settings change to its windows. This
     * code is copied from xxxResetDisplayDevice().
     */


    if ((pdesk->dwDTFlags & DF_NEWDISPLAYSETTINGS) && pdesk->pDeskInfo && pdesk->pDeskInfo->spwnd ){

        pdesk->dwDTFlags &= ~DF_NEWDISPLAYSETTINGS;
        xxxBroadcastDisplaySettingsChange(pdesk, TRUE);


    }

    ThreadUnlockDesktopHandle(&tlhdesk);

    TRACE_INIT(("xxxSwitchDesktop: Leaving\n"));

    return TRUE;
}

/***************************************************************************\
* zzzSetDesktop
*
* Set desktop and desktop info in the specified pti.
*
* History:
* 23-Dec-1993 JimA      Created.
\***************************************************************************/

VOID zzzSetDesktop(
    PTHREADINFO pti,
    PDESKTOP    pdesk,
    HDESK       hdesk)
{
    PTEB                      pteb;
    OBJECT_HANDLE_INFORMATION ohi;
    PDESKTOP                  pdeskRef;
    PDESKTOP                  pdeskOld;
    PCLIENTTHREADINFO         pctiOld;
    TL                        tlpdesk;
    PTHREADINFO               ptiCurrent = PtiCurrent();

    if (pti == NULL) {
        UserAssert(pti);
        return;
    }

    /*
     * A handle without an object pointer is bad news.
     */
    UserAssert(pdesk != NULL || hdesk == NULL);

    /*
     * This desktop must not be destroyed
     */
    if (pdesk != NULL && (pdesk->dwDTFlags & (DF_DESKWNDDESTROYED | DF_DYING)) &&
        pdesk != pti->rpdesk) {
        RIPMSG2(RIP_ERROR, "Assigning pti %#p to a dying desktop %#p",
                pti, pdesk);
        return;
    }

#if DBG
    /*
     * Catch reset of important desktops
     */
    if (pti->rpdesk && pti->rpdesk->dwConsoleThreadId == TIDq(pti) &&
            pti->cWindows != 0) {
        RIPMSG0(RIP_ERROR, "Reset of console desktop");
    }
#endif

    /*
     * Clear hook flag
     */
    pti->TIF_flags &= ~TIF_ALLOWOTHERACCOUNTHOOK;

    /*
     * Get granted access
     */
    pti->hdesk = hdesk;
    if (hdesk != NULL) {
        if (NT_SUCCESS(ObReferenceObjectByHandle(hdesk,
                                                 0,
                                                 *ExDesktopObjectType,
                                                 KernelMode,
                                                 &pdeskRef,
                                                 &ohi))) {

            UserAssert(pdesk->dwSessionId == gSessionId);

            LogDesktop(pdeskRef, LD_REF_FN_SETDESKTOP, TRUE, (ULONG_PTR)PtiCurrent());

            UserAssert(pdeskRef == pdesk);
            LogDesktop(pdesk, LD_DEREF_FN_SETDESKTOP, FALSE, (ULONG_PTR)PtiCurrent());
            ObDereferenceObject(pdeskRef);
            pti->amdesk = ohi.GrantedAccess;
            if (CheckHandleFlag(hdesk, HF_DESKTOPHOOK))
                pti->TIF_flags |= TIF_ALLOWOTHERACCOUNTHOOK;

            SetHandleFlag(hdesk, HF_PROTECTED, TRUE);

        } else {
            pti->amdesk = 0;
        }

    } else {
        pti->amdesk = 0;
    }

    /*
     * Do nothing else if the thread has initialized and the desktop
     * is not changing.
     */
    if ((pdesk != NULL) && (pdesk == pti->rpdesk))
        return;

    /*
     * Save old pointers for later.  Locking the old desktop ensures
     * that we will be able to free the CLIENTTHREADINFO structure.
     */
    pdeskOld = pti->rpdesk;
    ThreadLockDesktop(ptiCurrent, pdeskOld, &tlpdesk, LDLT_FN_SETDESKTOP);
    pctiOld = pti->pcti;

    /*
     * Remove the pti from the current desktop.
     */
     if (pti->rpdesk) {
        UserAssert(ISATOMICCHECK() || pti->pq == NULL || pti->pq->cThreads == 1);
        RemoveEntryList(&pti->PtiLink);
     }

    LockDesktop(&pti->rpdesk, pdesk, LDL_PTI_DESK, (ULONG_PTR)pti);


    /*
     * If there is no desktop, we need to fake a desktop info
     * structure so that the IsHooked() macro can test a "valid"
     * fsHooks value.  Also link the pti to the desktop.
     */
    if (pdesk != NULL) {
        pti->pDeskInfo = pdesk->pDeskInfo;
        InsertHeadList(&pdesk->PtiList, &pti->PtiLink);
    } else {
        pti->pDeskInfo = &diStatic;
    }

    // UserAssert((pti->ppi != NULL) || (pti->ppi == PpiCurrent())); // can't access TEB/pClientInfo of other process

    pteb = pti->pEThread->Tcb.Teb;
    if (pteb) {
        PDESKTOPVIEW pdv;
        if (pdesk && (pdv = GetDesktopView(pti->ppi, pdesk))) {

            pti->pClientInfo->pDeskInfo =
                    (PDESKTOPINFO)((PBYTE)pti->pDeskInfo - pdv->ulClientDelta);

            pti->pClientInfo->ulClientDelta = pdv->ulClientDelta;

        } else {

            pti->pClientInfo->pDeskInfo     = NULL;
            pti->pClientInfo->ulClientDelta = 0;

            /*
             * Reset the cursor level to its orginal state.
             */
            pti->iCursorLevel = TEST_GTERMF(GTERMF_MOUSE) ? 0 : -1;
            if (pti->pq)
                pti->pq->iCursorLevel = pti->iCursorLevel;
        }
    }

    /*
     * Allocate thread information visible from client, then copy and free
     * any old info we have lying around.
     */
    if (pdesk != NULL) {

        /*
         * Do not use DesktopAlloc here because the desktop might
         * have DF_DESTROYED set.
         */
        pti->pcti = DesktopAllocAlways(pdesk,
                                       sizeof(CLIENTTHREADINFO),
                                       DTAG_CLIENTTHREADINFO);
    }

    if (pdesk == NULL || pti->pcti == NULL) {
        pti->pcti = &(pti->cti);
        pti->pClientInfo->pClientThreadInfo = NULL;
    } else {
        pti->pClientInfo->pClientThreadInfo =
                (PCLIENTTHREADINFO)((PBYTE)pti->pcti - pti->pClientInfo->ulClientDelta);
    }

    if (pctiOld != NULL) {

        if (pctiOld != pti->pcti) {
            RtlCopyMemory(pti->pcti, pctiOld, sizeof(CLIENTTHREADINFO));
        }

        if (pctiOld != &(pti->cti)) {
            DesktopFree(pdeskOld, pctiOld);
        }

    } else {
        RtlZeroMemory(pti->pcti, sizeof(CLIENTTHREADINFO));
    }

    /*
     * If journalling is occuring on the new desktop, attach to
     * the journal queue.
     * Assert that the pti and the pdesk point to the same deskinfo
     *  if not, we will check the wrong hooks.
     */
    UserAssert((pdesk == NULL ) || (pti->pDeskInfo == pdesk->pDeskInfo));
    UserAssert(pti->rpdesk == pdesk);
    if (pti->pq != NULL) {
        PQ pq = GetJournallingQueue(pti);
        if (pq != NULL) {
            pq->cThreads++;
            zzzAttachToQueue(pti, pq, NULL, FALSE);
        }
    }

    ThreadUnlockDesktop(ptiCurrent, &tlpdesk, LDUT_FN_SETDESKTOP);
}

/***************************************************************************\
* xxxSetThreadDesktop (API)
*
* Associate the current thread with a desktop.
*
* History:
* 16-Jan-1991 JimA      Created stub.
\***************************************************************************/

BOOL xxxSetThreadDesktop(
    HDESK    hdesk,
    PDESKTOP pdesk)
{
    PTHREADINFO  ptiCurrent;
    PPROCESSINFO ppiCurrent;
    PQ           pqAttach;

    ptiCurrent = PtiCurrent();
    ppiCurrent = ptiCurrent->ppi;


    /*
     * If the handle has not been mapped in, do it now.
     */
    if (pdesk != NULL) {
        try {
            MapDesktop(ObOpenHandle, ppiCurrent->Process, pdesk, 0, 1);
        } except (W32ExceptionHandler(TRUE, RIP_WARNING)) {
            return FALSE;
        }

        UserAssert(GetDesktopView(ppiCurrent, pdesk) != NULL);
    }

    /*
     * Check non-system thread status
     */
    if (ptiCurrent->pEThread->ThreadsProcess != gpepCSRSS) {

        /*
         * Fail if the non-system thread has any windows or thread hooks.
         */
        if (ptiCurrent->cWindows != 0 || ptiCurrent->fsHooks) {
            RIPERR0(ERROR_BUSY, RIP_WARNING, "Thread has windows or hooks");
            return FALSE;
        }

        /*
         * If this is the first desktop assigned to the process,
         * make it the startup desktop.
         */
        if (ppiCurrent->rpdeskStartup == NULL && hdesk != NULL) {
            LockDesktop(&ppiCurrent->rpdeskStartup, pdesk, LDL_PPI_DESKSTARTUP1, (ULONG_PTR)ppiCurrent);
            ppiCurrent->hdeskStartup = hdesk;
        }
    }


    /*
     * If the desktop is changing and the thread is sharing a queue,
     * detach the thread.  This will ensure that threads sharing
     * queues are all on the same desktop.  This will prevent
     * zzzDestroyQueue from getting confused and setting ptiKeyboard
     * and ptiMouse to NULL when a thread detachs.
     */
    if (ptiCurrent->rpdesk != pdesk) {
        if (ptiCurrent->pq->cThreads > 1) {
            pqAttach = AllocQueue(NULL, NULL);
            if (pqAttach != NULL) {
                pqAttach->cThreads++;
                zzzAttachToQueue(ptiCurrent, pqAttach, NULL, FALSE);
            } else {
                RIPERR0(ERROR_NOT_ENOUGH_MEMORY, RIP_WARNING, "Thread could not be detached");
                return FALSE;
            }
        } else if (ptiCurrent->pq == gpqForeground) {
            /*
             * This thread doesn't own any windows, still it's attached to qpgForeground and
             *  it's the only thread attached to it.
             * Since any threads attached to qpgForeground must be in grpdeskRitInput,
             *  we must set qpgForeground to NULL here because this thread is going to another
             *  desktop.
             */
            UserAssert(ptiCurrent->pq->spwndActive == NULL);
            UserAssert(ptiCurrent->pq->spwndCapture == NULL);
            UserAssert(ptiCurrent->pq->spwndFocus == NULL);
            UserAssert(ptiCurrent->pq->spwndActivePrev == NULL);
            xxxSetForegroundWindow2(NULL, ptiCurrent, 0);
        } else if (ptiCurrent->rpdesk == NULL) {
            /*
             * We need to initialize iCursorLevel.
             */
            ptiCurrent->iCursorLevel = TEST_GTERMF(GTERMF_MOUSE) ? 0 : -1;
            ptiCurrent->pq->iCursorLevel = ptiCurrent->iCursorLevel;
        }

        UserAssert(ptiCurrent->pq != gpqForeground);

    }

    zzzSetDesktop(ptiCurrent, pdesk, hdesk);

    return TRUE;
}
/***************************************************************************\
* xxxDuplicateObject
*
* ZwDuplicateObject grabs ObpInitKillMutant so we have to leave our
*  critical section.
*
* 04-24-96 GerardoB  Created
\***************************************************************************/
NTSTATUS
xxxUserDuplicateObject(
    IN HANDLE SourceProcessHandle,
    IN HANDLE SourceHandle,
    IN HANDLE TargetProcessHandle OPTIONAL,
    OUT PHANDLE TargetHandle OPTIONAL,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG HandleAttributes,
    IN ULONG Options
    )

{
    NTSTATUS Status;

    CheckCritIn();

    LeaveCrit();

    Status = ZwDuplicateObject(SourceProcessHandle, SourceHandle, TargetProcessHandle,
            TargetHandle, DesiredAccess, HandleAttributes, Options);

    EnterCrit();

    return Status;
}
/***************************************************************************\
* xxxUserFindHandleForObject
*
* ObFindHandleForObject grabs ObpInitKillMutant so we have to leave our
*  critical section.
*
* 04-24-96 GerardoB  Created
\***************************************************************************/

BOOLEAN
xxxUserFindHandleForObject(
    IN PEPROCESS Process,
    IN PVOID Object OPTIONAL,
    IN POBJECT_TYPE ObjectType OPTIONAL,
    IN POBJECT_HANDLE_INFORMATION HandleInformation OPTIONAL,
    OUT PHANDLE Handle
    )
{
    BOOLEAN fRet;
    BOOL fExclusive, fShared;

    fExclusive = ExIsResourceAcquiredExclusiveLite(gpresUser);
    if (!fExclusive) {
        fShared = ExIsResourceAcquiredSharedLite(gpresUser);
    }

    if (fExclusive || fShared) {
        LeaveCrit();
    }

    fRet = ObFindHandleForObject(Process, Object, ObjectType, HandleInformation, Handle);

    if (fExclusive) {
        EnterCrit();
    } else if (fShared) {
        EnterSharedCrit();
    }

    return fRet;
}

/***************************************************************************\
* xxxGetThreadDesktop (API)
*
* Return a handle to the desktop assigned to the specified thread.
*
* History:
* 16-Jan-1991 JimA      Created stub.
\***************************************************************************/

HDESK xxxGetThreadDesktop(
    DWORD           dwThread,
    HDESK           hdeskConsole,
    KPROCESSOR_MODE AccessMode)
{
    PTHREADINFO  pti = PtiFromThreadId(dwThread);
    PPROCESSINFO ppiThread;
    HDESK        hdesk;
    NTSTATUS     Status;

    if (pti == NULL) {

        /*
         * If the thread has a console use that desktop.  If
         * not, then the thread is either invalid or not
         * a Win32 thread.
         */
        if (hdeskConsole == NULL) {
            RIPERR1(ERROR_INVALID_PARAMETER, RIP_VERBOSE,
                    "xxxGetThreadDesktop: invalid threadId 0x%x",
                    dwThread);
            return NULL;
        }

        hdesk = hdeskConsole;
        ppiThread = PpiFromProcess(gpepCSRSS);
    } else {
        hdesk = pti->hdesk;
        ppiThread = pti->ppi;
    }

    /*
     * If there is no desktop, return NULL with no error
     */
    if (hdesk != NULL) {

        /*
         * If the thread belongs to this process, return the
         * handle.  Otherwise, enumerate the handle table of
         * this process to find a handle with the same
         * attributes.
         */
        if (ppiThread != PpiCurrent()) {
            PVOID pobj;
            OBJECT_HANDLE_INFORMATION ohi;

            RIPMSG4(RIP_VERBOSE, "[%x.%x] %s called xxxGetThreadDesktop for pti %#p\n",
                    PsGetCurrentThread()->Cid.UniqueProcess,
                    PsGetCurrentThread()->Cid.UniqueThread,
                    PsGetCurrentProcess()->ImageFileName,
                    pti);

            KeAttachProcess(&ppiThread->Process->Pcb);
            Status = ObReferenceObjectByHandle(hdesk,
                                               0,
                                               *ExDesktopObjectType,
                                               AccessMode,
                                               &pobj,
                                               &ohi);
            KeDetachProcess();
            if (!NT_SUCCESS(Status) ||
                !xxxUserFindHandleForObject(PsGetCurrentProcess(), pobj, NULL, &ohi, &hdesk)) {

                RIPMSG0(RIP_VERBOSE, "Cannot find hdesk for current process");

                hdesk = NULL;

            } else {
                LogDesktop(pobj, LD_REF_FN_GETTHREADDESKTOP, TRUE, (ULONG_PTR)PtiCurrent());
            }
            if (NT_SUCCESS(Status)) {
                LogDesktop(pobj, LD_DEREF_FN_GETTHREADDESKTOP, FALSE, (ULONG_PTR)PtiCurrent());
                ObDereferenceObject(pobj);
            }
        }

        if (hdesk == NULL) {
            RIPERR0(ERROR_ACCESS_DENIED, RIP_VERBOSE, "xxxGetThreadDesktop: hdesk is null");
        } else {
            SetHandleFlag(hdesk, HF_PROTECTED, TRUE);
        }
    }

    return hdesk;
}


/***************************************************************************\
* xxxGetInputDesktop (API)
*
* Obsolete - kept for compatibility only.  Return a handle to the
* desktop currently receiving input.  Returns the first handle to
* the input desktop found.
*
* History:
* 16-Jan-1991 JimA      Created scaffold code.
\***************************************************************************/

HDESK xxxGetInputDesktop(VOID)
{
    HDESK             hdesk;

    if (xxxUserFindHandleForObject(PsGetCurrentProcess(), grpdeskRitInput, NULL, NULL, &hdesk)) {
        SetHandleFlag(hdesk, HF_PROTECTED, TRUE);
        return hdesk;
    } else {
        return NULL;
    }
}

/***************************************************************************\
* xxxCloseDesktop (API)
*
* Close a reference to a desktop and destroy the desktop if it is no
* longer referenced.
*
* History:
* 16-Jan-1991 JimA      Created scaffold code.
* 11-Feb-1991 JimA      Added access checks.
\***************************************************************************/

BOOL xxxCloseDesktop(
    HDESK           hdesk,
    KPROCESSOR_MODE AccessMode)
{
    PDESKTOP     pdesk;
    PTHREADINFO  ptiT;
    PPROCESSINFO ppi;
    NTSTATUS     Status;

    ppi = PpiCurrent();

    /*
     * Get a pointer to the desktop.
     */
    Status = ObReferenceObjectByHandle(
            hdesk,
            0,
            *ExDesktopObjectType,
            AccessMode,
            &pdesk,
            NULL);
    if (!NT_SUCCESS(Status)) {
        RIPNTERR0(Status, RIP_VERBOSE, "");
        return FALSE;
    }

    UserAssert(pdesk->dwSessionId == gSessionId);

    LogDesktop(pdesk, LD_REF_FN_CLOSEDESKTOP, TRUE, (ULONG_PTR)PtiCurrent());

    if (ppi->Process != gpepCSRSS) {

        /*
         * Disallow closing of the desktop if the handle is in use by
         * any threads in the process.
         */
        for (ptiT = ppi->ptiList; ptiT != NULL; ptiT = ptiT->ptiSibling) {
            if (ptiT->hdesk == hdesk) {
                RIPERR2(ERROR_BUSY, RIP_WARNING,
                        "CloseDesktop: Desktop %#p still in use by thread %#p",
                        pdesk, ptiT);
                LogDesktop(pdesk, LD_DEREF_FN_CLOSEDESKTOP1, FALSE, (ULONG_PTR)PtiCurrent());
                ObDereferenceObject(pdesk);
                return FALSE;
            }
        }

        /*
         * If this is the startup desktop, unlock it
         */
         /*
          * Bug 41394. Make sure that hdesk == ppi->hdeskStartup. We might
          * be getting a handle to the desktop object that is different
          * from ppi->hdeskStartup but we still end up
          * setting ppi->hdeskStartup to NULL.
          */
        if ((pdesk == ppi->rpdeskStartup) && (hdesk == ppi->hdeskStartup)) {
            UnlockDesktop(&ppi->rpdeskStartup, LDU_PPI_DESKSTARTUP2, (ULONG_PTR)ppi);
            ppi->hdeskStartup = NULL;
        }
    }

    /*
     * Clear hook flag
     */
    SetHandleFlag(hdesk, HF_DESKTOPHOOK, FALSE);

    /*
     * Close the handle
     */
    Status = CloseProtectedHandle(hdesk);

    LogDesktop(pdesk, LD_DEREF_FN_CLOSEDESKTOP2, FALSE, (ULONG_PTR)PtiCurrent());
    ObDereferenceObject(pdesk);
    UserAssert(NT_SUCCESS(Status));

    return TRUE;
}

/***************************************************************************\
* TerminateConsole
*
* Post a quit message to a console thread and wait for it to terminate.
*
* History:
* 08-May-1995 JimA      Created.
\***************************************************************************/

VOID TerminateConsole(
    PDESKTOP pdesk)
{
    NTSTATUS Status;
    PETHREAD Thread;
    PTHREADINFO pti;

    if (pdesk->dwConsoleThreadId == 0)
        return;

    /*
     * Locate the console thread.
     */
    Status = LockThreadByClientId((HANDLE)LongToHandle( pdesk->dwConsoleThreadId ), &Thread);
    if (!NT_SUCCESS(Status))
        return;

    /*
     * Post a quit message to the console.
     */
    pti = PtiFromThread(Thread);
    UserAssert(pti != NULL);
    if (pti != NULL) {
        _PostThreadMessage(pti, WM_QUIT, 0, 0);
    }

    /*
     * Clear thread id so we don't post twice
     */
    pdesk->dwConsoleThreadId = 0;

    UnlockThread(Thread);
}

/***************************************************************************\
* CheckHandleFlag
*
* Returns TRUE if the desktop handle allows other accounts
* to hook this process.
*
* History:
* 07-13-95 JimA         Created.
\***************************************************************************/

BOOL CheckHandleFlag(
    HANDLE       hObject,
    DWORD        dwFlag)
{
    PPROCESSINFO ppi;
    ULONG Index = ((PEXHANDLE)&hObject)->Index * HF_LIMIT + dwFlag;
    BOOL fRet = FALSE;

    EnterHandleFlagsCrit();

    if ((ppi = PpiCurrent()) != NULL) {
        fRet = (Index < ppi->bmHandleFlags.SizeOfBitMap &&
                RtlCheckBit(&ppi->bmHandleFlags, Index));
    }

    LeaveHandleFlagsCrit();

    return fRet;
}

/***************************************************************************\
* SetHandleFlag
*
* Sets and clears the ability of a desktop handle to allow
* other accounts to hook this process.
*
* History:
* 07-13-95 JimA         Created.
\***************************************************************************/

BOOL SetHandleFlag(
    HANDLE       hObject,
    DWORD        dwFlag,
    BOOL         fSet)
{
    PPROCESSINFO ppi;
    ULONG Index = ((PEXHANDLE)&hObject)->Index * HF_LIMIT + dwFlag;
    PRTL_BITMAP pbm;
    ULONG       cBits;
    PULONG      Buffer;
    BOOL fRet = TRUE;

    UserAssert(dwFlag < HF_LIMIT);

    EnterHandleFlagsCrit();

    if ((ppi = PpiCurrent()) != NULL) {
        pbm = &ppi->bmHandleFlags;
        if (fSet) {

            /*
             * Expand the bitmap if needed
             */
            if (Index >= pbm->SizeOfBitMap) {
                /*
                 * Index is zero-based - cBits is an exact number of dwords
                 */
                cBits = ((Index + 1) + 0x1F) & ~0x1F;
                Buffer = UserAllocPoolWithQuotaZInit(cBits / 8, TAG_PROCESSINFO);
                if (Buffer == NULL) {
                    fRet = FALSE;
                    goto Exit;
                }
                if (pbm->Buffer) {
                    RtlCopyMemory(Buffer, pbm->Buffer, pbm->SizeOfBitMap / 8);
                    UserFreePool(pbm->Buffer);
                }

                RtlInitializeBitMap(pbm, Buffer, cBits);
            }

            RtlSetBits(pbm, Index, 1);
        } else if (Index < pbm->SizeOfBitMap) {
            RtlClearBits(pbm, Index, 1);
        }
    }

Exit:
    LeaveHandleFlagsCrit();

    return fRet;
}


/***************************************************************************\
* CheckHandleInUse
*
* Returns TRUE if the handle is currently in use.
*
* History:
* 02-Jun-1999 JerrySh   Created.
\***************************************************************************/

BOOL CheckHandleInUse(
    HANDLE hObject)
{
    BOOL fRet;

    EnterHandleFlagsCrit();
    fRet = ((gProcessInUse == PsGetCurrentProcess()) &&
            (gHandleInUse == hObject));
    LeaveHandleFlagsCrit();

    return fRet;
}

/***************************************************************************\
* SetHandleInUse
*
* Mark the handle as in use.
*
* History:
* 02-Jun-1999 JerrySh   Created.
\***************************************************************************/

VOID SetHandleInUse(
    HANDLE hObject)
{
    EnterHandleFlagsCrit();
    gProcessInUse = PsGetCurrentProcess();
    gHandleInUse = hObject;
    LeaveHandleFlagsCrit();
}

NTSTATUS xxxResolveDesktopForWOW (
    IN OUT PUNICODE_STRING pstrDesktop)
{
    NTSTATUS           Status;
    UNICODE_STRING     strDesktop, strWinSta, strStatic;
    LPWSTR             pszDesktop;
    BOOL               fWinStaDefaulted;
    BOOL               fDesktopDefaulted;
    HWINSTA            hwinsta;
    HDESK              hdesk;
    OBJECT_ATTRIBUTES  ObjA;
    PUNICODE_STRING    pstrStatic;
    POBJECT_ATTRIBUTES pObjA = NULL;
    SIZE_T             cbObjA;
    BOOL bShutDown = FALSE;
    PTEB               pteb = NtCurrentTeb();

    UserAssert(pteb);

    /*
     * Determine windowstation and desktop names.
     */


    if (pstrDesktop == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    strStatic.Length = 0;
    strStatic.MaximumLength = STATIC_UNICODE_BUFFER_LENGTH * sizeof(WCHAR);

    /*
     * Use the StaticUnicodeBuffer on the TEB as the buffer for the object name.
     * Even if this is client side and we pass KernelMode to the Ob call in
     * _OpenWindowStation this is safe because the TEB is not going to go away
     * during this call. The worst it can happen is to have the buffer in the TEB
     * trashed.
     */
    strStatic.Buffer = pteb->StaticUnicodeBuffer;

    UserAssert(pteb->StaticUnicodeBuffer == pteb->StaticUnicodeString.Buffer);
    UserAssert(pteb->StaticUnicodeString.MaximumLength ==STATIC_UNICODE_BUFFER_LENGTH * sizeof(WCHAR));

    if (pstrDesktop->Length == 0) {
        RtlInitUnicodeString(&strDesktop, TEXT("Default"));
        fWinStaDefaulted = fDesktopDefaulted = TRUE;
    } else {
        USHORT cch;
        /*
         * The name be of the form windowstation\desktop.  Parse
         * the string to separate out the names.
         */
        strWinSta = *pstrDesktop;
        cch = strWinSta.Length / sizeof(WCHAR);
        pszDesktop = strWinSta.Buffer;
        while (cch && *pszDesktop != L'\\') {
            cch--;
            pszDesktop++;
        }
        fDesktopDefaulted = FALSE;

        if (cch == 0) {

            /*
             * No windowstation name was specified, only the desktop.
             */
            strDesktop = strWinSta;
            fWinStaDefaulted = TRUE;
        } else {
            /*
             * Both names were in the string.
             */
            strDesktop.Buffer = pszDesktop + 1;
            strDesktop.Length = strDesktop.MaximumLength = (cch - 1) * sizeof(WCHAR);
            strWinSta.Length = (USHORT)(pszDesktop - strWinSta.Buffer) * sizeof(WCHAR);

            /*
             * zero terminate the strWinSta buffer so the rebuild of the desktop
             * name at the end of the function works.
             */
            *pszDesktop = (WCHAR)0;

            fWinStaDefaulted = FALSE;

            RtlAppendUnicodeToString(&strStatic, (PWSTR)szWindowStationDirectory);
            RtlAppendUnicodeToString(&strStatic, L"\\");
            RtlAppendUnicodeStringToString(&strStatic, &strWinSta);

        }
    }

    if (fWinStaDefaulted) {

        //Default Window Station
        RtlInitUnicodeString(&strWinSta, L"WinSta0");

        RtlAppendUnicodeToString(&strStatic, (PWSTR)szWindowStationDirectory);
        RtlAppendUnicodeToString(&strStatic, L"\\");
        RtlAppendUnicodeStringToString(&strStatic, &strWinSta);

    }

    /*
     * Open the computed windowstation. This will also do an access check
     */
    if (gbSecureDesktop) {
        /*
         * Allocate an object attributes structure in user address space.
         */
        cbObjA = sizeof(*pObjA) + sizeof(*pstrStatic);
        Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                &pObjA, 0, &cbObjA, MEM_COMMIT, PAGE_READWRITE);
        pstrStatic = (PUNICODE_STRING)((PBYTE)pObjA + sizeof(*pObjA));

        if (NT_SUCCESS(Status)) {
            /*
             * Note -- the string must be in client-space or the
             * address validation in OpenWindowStation will fail.
             */
            try {
                *pstrStatic = strStatic;
                InitializeObjectAttributes( pObjA,
                                            pstrStatic,
                                            OBJ_CASE_INSENSITIVE,
                                            NULL,
                                            NULL
                                            );
            } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
                Status = GetExceptionCode();
            }

            if (NT_SUCCESS(Status)) {
                hwinsta = _OpenWindowStation(pObjA, MAXIMUM_ALLOWED, UserMode);
            } else {
                hwinsta = NULL;
            }
            if (!hwinsta) {
                ZwFreeVirtualMemory(NtCurrentProcess(), &pObjA, &cbObjA,
                        MEM_RELEASE);
                return STATUS_ACCESS_DENIED;
            }
        } else {
            return STATUS_NO_MEMORY;
        }
    } else {
        InitializeObjectAttributes( &ObjA,
                                    &strStatic,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL
                                    );
        hwinsta = _OpenWindowStation(&ObjA, MAXIMUM_ALLOWED, KernelMode);
        if (!hwinsta) {
            return STATUS_ACCESS_DENIED;
        }
    }

    /*
     * Do an access check on the desktop by opening it
     */

    RtlCopyUnicodeString(&strStatic, &strDesktop);

    if (gbSecureDesktop) {
        /*
         * Note -- the string must be in client-space or the
         * address validation in OpenDesktop will fail.
         */
        try {
            *pstrStatic = strStatic;
            InitializeObjectAttributes( pObjA,
                                        pstrStatic,
                                        OBJ_CASE_INSENSITIVE,
                                        hwinsta,
                                        NULL
                                        );
        } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
            Status = GetExceptionCode();
        }

        if (NT_SUCCESS(Status)) {
            hdesk = xxxOpenDesktop(pObjA,
                                   UserMode,
                                   0,
                                   MAXIMUM_ALLOWED,
                                   &bShutDown);
        } else {
            hdesk = NULL;
        }

        ZwFreeVirtualMemory(NtCurrentProcess(), &pObjA, &cbObjA,
                MEM_RELEASE);
    } else {
        InitializeObjectAttributes( &ObjA,
                                    &strStatic,
                                    OBJ_CASE_INSENSITIVE,
                                    hwinsta,
                                    NULL
                                    );
        hdesk = xxxOpenDesktop(&ObjA,
                               KernelMode,
                               0,
                               MAXIMUM_ALLOWED,
                               &bShutDown);
    }

    if (!hdesk) {
        UserVerify(NT_SUCCESS(ZwClose(hwinsta)));
        return STATUS_ACCESS_DENIED;
    }

    CloseProtectedHandle(hdesk);
    UserVerify(NT_SUCCESS(ZwClose(hwinsta)));

    /*
     * Copy the final Computed String
     */
    RtlCopyUnicodeString(pstrDesktop, &strWinSta);
    RtlAppendUnicodeToString(pstrDesktop, L"\\");
    RtlAppendUnicodeStringToString(pstrDesktop, &strDesktop);

    return STATUS_SUCCESS;
}
/***************************************************************************\
* xxxResolveDesktop
*
* Attempts to return handles to a windowstation and desktop associated
* with the logon session.
*
* History:
* 25-Apr-1994 JimA      Created.
\***************************************************************************/

HDESK xxxResolveDesktop(
    HANDLE          hProcess,
    PUNICODE_STRING pstrDesktop,
    HWINSTA         *phwinsta,
    BOOL            fInherit,
    BOOL*           pbShutDown)
{
    PEPROCESS          Process;
    PPROCESSINFO       ppi;
    HWINSTA            hwinsta;
    HDESK              hdesk;
    PDESKTOP           pdesk;
    PWINDOWSTATION     pwinsta;
    BOOL               fInteractive;
    UNICODE_STRING     strDesktop;
    UNICODE_STRING     strWinSta, strStatic;
    OBJECT_ATTRIBUTES  ObjA;
    PUNICODE_STRING    pstrStatic;
    POBJECT_ATTRIBUTES pObjA = NULL;
    SIZE_T             cbObjA;
    LPWSTR             pszDesktop;
    WCHAR              awchName[sizeof(L"Service-0x0000-0000$") / sizeof(WCHAR)];
    BOOL               fWinStaDefaulted;
    BOOL               fDesktopDefaulted;
    LUID               luidService;
    NTSTATUS           Status;
    HWINSTA            hwinstaDup;
    PTEB               pteb = NtCurrentTeb();

    CheckCritIn();

    UserAssert(pteb);

    Status = ObReferenceObjectByHandle(hProcess,
                                       PROCESS_QUERY_INFORMATION,
                                       *PsProcessType,
                                       UserMode,
                                       &Process,
                                       NULL);
    if (!NT_SUCCESS(Status)) {
       RIPMSG1(RIP_WARNING, "ResolveDesktop: Could not reference process handle (0x%X)", hProcess);
       return NULL;
    }

    strStatic.Length = 0;
    strStatic.MaximumLength = STATIC_UNICODE_BUFFER_LENGTH * sizeof(WCHAR);

    /*
     * Use the StaticUnicodeBuffer on the TEB as the buffer for the object name.
     * Even if this is client side and we pass KernelMode to the Ob call in
     * _OpenWindowStation this is safe because the TEB is not going to go away
     * during this call. The worst it can happen is to have the buffer in the TEB
     * trashed.
     */
    strStatic.Buffer = pteb->StaticUnicodeBuffer;

    UserAssert(pteb->StaticUnicodeBuffer == pteb->StaticUnicodeString.Buffer);
    UserAssert(pteb->StaticUnicodeString.MaximumLength ==STATIC_UNICODE_BUFFER_LENGTH * sizeof(WCHAR));
    /*
     * The static unicode buffer in the teb
    /*
     * If the process already has a windowstation and a startup desktop,
     * return them.
     */
    hwinsta = NULL;
    hwinstaDup = NULL;
    hdesk = NULL;
    ppi = PpiFromProcess(Process);

    /*
     * Make sure the process has not been destroyed. Bug 214643
     */
    if (ppi != NULL) {

        if (ppi->W32PF_Flags & W32PF_TERMINATED) {

            ObDereferenceObject(Process);

            RIPMSG1(RIP_WARNING, "xxxResolveDesktop: ppi %#p has been destroyed", ppi);
            return NULL;
        }

        if (ppi->hwinsta != NULL && ppi->hdeskStartup != NULL) {

            /*
             * If the target process is the current process, simply
             * return the handles.  Otherwise, open the objects.
             */
            if (Process == PsGetCurrentProcess()) {
                hwinsta = ppi->hwinsta;
                hdesk = ppi->hdeskStartup;
            } else {
                Status = ObOpenObjectByPointer(
                        ppi->rpwinsta,
                        0,
                        NULL,
                        MAXIMUM_ALLOWED,
                        *ExWindowStationObjectType,
                        (KPROCESSOR_MODE)(gbSecureDesktop ? UserMode : KernelMode),
                        &hwinsta);
                if (NT_SUCCESS(Status)) {
                    Status = ObOpenObjectByPointer(
                            ppi->rpdeskStartup,
                            0,
                            NULL,
                            MAXIMUM_ALLOWED,
                            *ExDesktopObjectType,
                            (KPROCESSOR_MODE)(gbSecureDesktop ? UserMode : KernelMode),
                            &hdesk);
                    if (!NT_SUCCESS(Status)) {
                        UserVerify(NT_SUCCESS(ZwClose(hwinsta)));
                        hwinsta = NULL;
                    }
                }
                if (!NT_SUCCESS(Status)) {
                    RIPNTERR2(
                            Status,
                            RIP_WARNING,
                            "ResolveDesktop: Could not reference winsta=%#p and/or desk=%#p",
                            ppi->rpwinsta, ppi->rpdeskStartup);
                }
            }

            RIPMSG2(RIP_VERBOSE,
                    "ResolveDesktop: to hwinsta=%#p desktop=%#p",
                    hwinsta, hdesk);

            ObDereferenceObject(Process);
            *phwinsta = hwinsta;
            return hdesk;
        }
    }

    /*
     * Determine windowstation and desktop names.
     */
    if (pstrDesktop == NULL || pstrDesktop->Length == 0) {
        RtlInitUnicodeString(&strDesktop, TEXT("Default"));
        fWinStaDefaulted = fDesktopDefaulted = TRUE;
    } else {
        USHORT cch;
        /*
         * The name be of the form windowstation\desktop.  Parse
         * the string to separate out the names.
         */
        strWinSta = *pstrDesktop;
        cch = strWinSta.Length / sizeof(WCHAR);
        pszDesktop = strWinSta.Buffer;
        while (cch && *pszDesktop != L'\\') {
            cch--;
            pszDesktop++;
        }
        fDesktopDefaulted = FALSE;

        if (cch == 0) {

            /*
             * No windowstation name was specified, only the desktop.
             */
            strDesktop = strWinSta;
            fWinStaDefaulted = TRUE;
        } else {
             /*
             * Both names were in the string.
             */
            strDesktop.Buffer = pszDesktop + 1;
            strDesktop.Length = strDesktop.MaximumLength = (cch - 1) * sizeof(WCHAR);
            strWinSta.Length = (USHORT)(pszDesktop - strWinSta.Buffer) * sizeof(WCHAR);
            fWinStaDefaulted = FALSE;

            RtlAppendUnicodeToString(&strStatic, (PWSTR)szWindowStationDirectory);
            RtlAppendUnicodeToString(&strStatic, L"\\");
            RtlAppendUnicodeStringToString(&strStatic, &strWinSta);

            if (!NT_SUCCESS(Status = _UserTestForWinStaAccess(&strStatic,TRUE))) {
                if (strStatic.MaximumLength > strStatic.Length)
                    strStatic.Buffer[strStatic.Length/sizeof(WCHAR)] = 0;
                else
                    strStatic.Buffer[(strStatic.Length - sizeof(WCHAR))/sizeof(WCHAR)] = 0;
                RIPMSG2(RIP_WARNING,
                        "ResolveDesktop: Error (0x%X) resolving to WinSta='%ws'",
                        Status, strStatic.Buffer);
                ObDereferenceObject(Process);
                *phwinsta = NULL;
                return NULL;
            }

        }
    }

    /*
     * If the desktop name is defaulted, make the handles
     * not inheritable.
     */
    if (fDesktopDefaulted)
        fInherit = FALSE;

    /*
     * If a windowstation has not been assigned to this process yet and
     * there are existing windowstations, attempt an open.
     */
    if (hwinsta == NULL && grpWinStaList) {

        /*
         * If the windowstation name was defaulted, create a name
         * based on the session.
         */
        if (fWinStaDefaulted) {
            //Default Window Station
            RtlInitUnicodeString(&strWinSta, L"WinSta0");

            RtlAppendUnicodeToString(&strStatic, (PWSTR)szWindowStationDirectory);
            RtlAppendUnicodeToString(&strStatic, L"\\");
            RtlAppendUnicodeStringToString(&strStatic, &strWinSta);

            if (gbRemoteSession) {
                /*
                 * Fake this out if it's an non-interactive winstation startup.
                 * We don't want an extra winsta.
                 */
                fInteractive = NT_SUCCESS(_UserTestForWinStaAccess(&strStatic, TRUE));
            } else {
                fInteractive = NT_SUCCESS(_UserTestForWinStaAccess(&strStatic,fInherit));
            }

            if (!fInteractive) {
                    GetProcessLuid(NULL, &luidService);
                    swprintf(awchName, L"Service-0x%x-%x$",
                            luidService.HighPart, luidService.LowPart);
                    RtlInitUnicodeString(&strWinSta, awchName);
            }
        }

        /*
         * If no windowstation name was passed in and a windowstation
         * handle was inherited, assign it.
         */
        if (fWinStaDefaulted) {
            if (xxxUserFindHandleForObject(Process, NULL, *ExWindowStationObjectType,
                    NULL, &hwinsta)) {

                /*
                 * If the handle belongs to another process,
                 * dup it into this one
                 */
                if (Process != PsGetCurrentProcess()) {

                    Status = xxxUserDuplicateObject(
                            hProcess,
                            hwinsta,
                            NtCurrentProcess(),
                            &hwinstaDup,
                            0,
                            0,
                            DUPLICATE_SAME_ACCESS);
                    if (!NT_SUCCESS(Status)) {
                        hwinsta = NULL;
                    } else {
                        hwinsta = hwinstaDup;
                    }
                }
            }
        }

        /*
         * If we were assigned to a windowstation, make sure
         * it matches our fInteractive flag
         */
        if (hwinsta != NULL) {
            Status = ObReferenceObjectByHandle(hwinsta,
                                               0,
                                               *ExWindowStationObjectType,
                                               KernelMode,
                                               &pwinsta,
                                               NULL);
            if (NT_SUCCESS(Status)) {
                BOOL fIO = (pwinsta->dwWSF_Flags & WSF_NOIO) ? FALSE : TRUE;
                if (fIO != fInteractive) {
                    if (hwinstaDup) {
                        CloseProtectedHandle(hwinsta);
                    }
                    hwinsta = NULL;
                }
                ObDereferenceObject(pwinsta);
            }
        }

        /*
         * If not, open the computed windowstation.
         */
        if (NT_SUCCESS(Status) && hwinsta == NULL) {

            /*
             * Fill in the path to the windowstation
             */
            strStatic.Length = 0;
            RtlAppendUnicodeToString(&strStatic, (PWSTR)szWindowStationDirectory);
            RtlAppendUnicodeToString(&strStatic, L"\\");
            RtlAppendUnicodeStringToString(&strStatic, &strWinSta);

            if (gbSecureDesktop) {
                /*
                 * Allocate an object attributes structure in user address space.
                 */
                cbObjA = sizeof(*pObjA) + sizeof(*pstrStatic);
                Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                        &pObjA, 0, &cbObjA, MEM_COMMIT, PAGE_READWRITE);
                pstrStatic = (PUNICODE_STRING)((PBYTE)pObjA + sizeof(*pObjA));

                if (NT_SUCCESS(Status)) {
                    /*
                     * Note -- the string must be in client-space or the
                     * address validation in OpenWindowStation will fail.
                     */
                    try {
                        *pstrStatic = strStatic;
                        InitializeObjectAttributes( pObjA,
                                                    pstrStatic,
                                                    OBJ_CASE_INSENSITIVE,
                                                    NULL,
                                                    NULL
                                                    );
                        if (fInherit)
                            pObjA->Attributes |= OBJ_INHERIT;
                    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
                        Status = GetExceptionCode();
                    }

                    if (NT_SUCCESS(Status)) {
                        hwinsta = _OpenWindowStation(pObjA, MAXIMUM_ALLOWED, UserMode);
                    }
                }
            } else {
                InitializeObjectAttributes( &ObjA,
                                            &strStatic,
                                            OBJ_CASE_INSENSITIVE,
                                            NULL,
                                            NULL
                                            );
                if (fInherit)
                    ObjA.Attributes |= OBJ_INHERIT;
                hwinsta = _OpenWindowStation(&ObjA, MAXIMUM_ALLOWED, KernelMode);
            }
        }

        /*
         * Only allow service logons at the console.  I don't think our
         * win32k exit routines cope with more than one windowstation.
         */
        /*
         * If the open failed and the process is in a non-interactive
         * logon session, attempt to create a windowstation and
         * desktop for that session.  Note that the desktop handle
         * will be closed after the desktop has been assigned.
         */
        if (!gbRemoteSession && NT_SUCCESS(Status) &&
            hwinsta == NULL && !fInteractive && fWinStaDefaulted) {

            *phwinsta = xxxConnectService(
                    &strStatic,
                    &hdesk);

            /*
             * Clean up and leave.
             */
            if (pObjA != NULL) {
                ZwFreeVirtualMemory(NtCurrentProcess(), &pObjA, &cbObjA,
                        MEM_RELEASE);
            }
            ObDereferenceObject(Process);

            RIPMSG2(RIP_VERBOSE,
                    "ResolveDesktop: xxxConnectService was called\n"
                    "to hwinsta=%#p desktop=%#p",
                    *phwinsta, hdesk);

            return hdesk;
        }
    }

    /*
     * Attempt to assign a desktop.
     */
    if (hwinsta != NULL) {

        /*
         * Every gui thread needs an associated desktop.  We'll use the default
         * to start with and the application can override it if it wants.
         */
        if (hdesk == NULL) {

            /*
             * If no desktop name was passed in and a desktop
             * handle was inherited, assign it.
             */
            if (fDesktopDefaulted) {
                if (xxxUserFindHandleForObject(Process, NULL, *ExDesktopObjectType,
                         NULL, &hdesk)) {

                    /*
                     * If the handle belongs to another process,
                     * dup it into this one
                     */
                    if (Process != PsGetCurrentProcess()) {
                        HDESK hdeskDup;

                        Status = xxxUserDuplicateObject(
                                hProcess,
                                hdesk,
                                NtCurrentProcess(),
                                &hdeskDup,
                                0,
                                0,
                                DUPLICATE_SAME_ACCESS);
                        if (!NT_SUCCESS(Status)) {
                            CloseProtectedHandle(hdesk);
                            hdesk = NULL;
                        } else {
                            hdesk = hdeskDup;
                        }
                    }

                    /*
                     * Map the desktop into the process.
                     */
                    if (hdesk != NULL && ppi != NULL) {
                        Status = ObReferenceObjectByHandle(hdesk,
                                                  0,
                                                  *ExDesktopObjectType,
                                                  KernelMode,
                                                  &pdesk,
                                                  NULL);
                        if (NT_SUCCESS(Status)) {

                            LogDesktop(pdesk, LD_REF_FN_RESOLVEDESKTOP, TRUE, (ULONG_PTR)PtiCurrent());

                            try {
                                MapDesktop(ObOpenHandle, Process, pdesk, 0, 1);
                            } except (W32ExceptionHandler(TRUE, RIP_WARNING)) {
                                Status = STATUS_NO_MEMORY;
                                CloseProtectedHandle(hdesk);
                                hdesk = NULL;
                            }
#if DBG
                            if (hdesk != NULL) {
                                UserAssert(GetDesktopView(ppi, pdesk) != NULL);
                            }
#endif // DBG
                            LogDesktop(pdesk, LD_DEREF_FN_RESOLVEDESKTOP, FALSE, (ULONG_PTR)PtiCurrent());
                            ObDereferenceObject(pdesk);
                        } else {
                            CloseProtectedHandle(hdesk);
                            hdesk = NULL;
                        }
                    }
                }
            }

            /*
             * If not, open the desktop.
             */
            if (NT_SUCCESS(Status) && hdesk == NULL) {
                RtlCopyUnicodeString(&strStatic, &strDesktop);

                if (gbSecureDesktop) {
                    if (pObjA == NULL) {
                        /*
                         * Allocate an object attributes structure in user address space.
                         */
                        cbObjA = sizeof(*pObjA) + sizeof(*pstrStatic);
                        Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                &pObjA, 0, &cbObjA, MEM_COMMIT, PAGE_READWRITE);
                        pstrStatic = (PUNICODE_STRING)((PBYTE)pObjA + sizeof(*pObjA));
                    }

                    if (NT_SUCCESS(Status)) {
                        /*
                         * Note -- the string must be in client-space or the
                         * address validation in OpenDesktop will fail.
                         */
                        try {
                            *pstrStatic = strStatic;
                            InitializeObjectAttributes( pObjA,
                                                        pstrStatic,
                                                        OBJ_CASE_INSENSITIVE,
                                                        hwinsta,
                                                        NULL
                                                        );
                            if (fInherit)
                                pObjA->Attributes |= OBJ_INHERIT;
                        } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
                            Status = GetExceptionCode();
                        }

                        if (NT_SUCCESS(Status)) {
                            hdesk = xxxOpenDesktop(pObjA,
                                                   UserMode,
                                                   0,
                                                   MAXIMUM_ALLOWED,
                                                   pbShutDown);
                        }
                    }
                } else {
                    InitializeObjectAttributes( &ObjA,
                                                &strStatic,
                                                OBJ_CASE_INSENSITIVE,
                                                hwinsta,
                                                NULL
                                                );
                    if (fInherit)
                        ObjA.Attributes |= OBJ_INHERIT;
                    hdesk = xxxOpenDesktop(&ObjA,
                                           KernelMode,
                                           0,
                                           MAXIMUM_ALLOWED,
                                           pbShutDown);
                }
            }
        }
        if (hdesk == NULL) {
            UserVerify(NT_SUCCESS(ZwClose(hwinsta)));
            hwinsta = NULL;
        }
    }

    ObDereferenceObject(Process);

    if (pObjA != NULL) {
        ZwFreeVirtualMemory(NtCurrentProcess(), &pObjA, &cbObjA,
                MEM_RELEASE);
    }

    *phwinsta = hwinsta;

    RIPMSG2(RIP_VERBOSE,
            "ResolveDesktop: to hwinsta=%#p desktop=%#p",
            *phwinsta, hdesk);

    return hdesk;
}
