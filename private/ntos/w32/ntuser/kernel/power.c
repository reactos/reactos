/****************************** Module Header ******************************\
* Module Name: power.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains the code to implement power management.
*
* History:
* 02-Dec-1996 JerrySh   Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include <ntcsrmsg.h>
#include "csrmsg.h"
#include "ntddvdeo.h"

#pragma alloc_text(INIT, InitializePowerRequestList)

extern BOOL gbUserInitialized;
extern BOOL fGdiEnabled;

LIST_ENTRY gPowerRequestList;
PFAST_MUTEX gpPowerRequestMutex;
PKEVENT gpEventPowerRequest;
BOOL    gbHibernate = FALSE;

typedef struct tagPOWERREQUEST {
    LIST_ENTRY        PowerRequestLink;
    KEVENT            Event;
    NTSTATUS          Status;
    PKWIN32_POWEREVENT_PARAMETERS Parms;
} POWERREQUEST, *PPOWERREQUEST;

PPOWERREQUEST gpPowerRequestCurrent;

__inline VOID EnterPowerCrit() {
    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe(gpPowerRequestMutex);
}

__inline VOID LeavePowerCrit() {
    ExReleaseFastMutexUnsafe(gpPowerRequestMutex);
    KeLeaveCriticalRegion();
}

/***************************************************************************\
* CancelPowerRequest
*
* The power request can't be satisfied because the worker thread is gone.
*
* History:
* 20-Oct-1998 JerrySh   Created.
\***************************************************************************/

VOID
CancelPowerRequest(
    PPOWERREQUEST pPowerRequest)
{
    UserAssert(pPowerRequest != gpPowerRequestCurrent);
    pPowerRequest->Status = STATUS_UNSUCCESSFUL;
    KeSetEvent(&pPowerRequest->Event, EVENT_INCREMENT, FALSE);
}

/***************************************************************************\
* QueuePowerRequest
*
* Insert a power request into the list and wakeup CSRSS to process it.
*
* History:
* 20-Oct-1998 JerrySh   Created.
\***************************************************************************/

NTSTATUS
QueuePowerRequest(
    PKWIN32_POWEREVENT_PARAMETERS Parms)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PPOWERREQUEST pPowerRequest;
    TL tlPool;

    UserAssert(gpEventPowerRequest != NULL);
    UserAssert(gpPowerRequestMutex != NULL);

    /*
     * Allocate and initialize the power request.
     */
    pPowerRequest = UserAllocPoolNonPaged(sizeof(POWERREQUEST), TAG_POWER);
    if (pPowerRequest == NULL) {
        return STATUS_NO_MEMORY;
    }
    KeInitializeEvent(&pPowerRequest->Event, SynchronizationEvent, FALSE);
    pPowerRequest->Parms = Parms;

    /*
     * Insert the power request into the list.
     */
    EnterPowerCrit();
    if (gbNoMorePowerCallouts) {
        Status = STATUS_UNSUCCESSFUL;
    } else {
        InsertHeadList(&gPowerRequestList, &pPowerRequest->PowerRequestLink);
    }
    LeavePowerCrit();

    /*
     * If this is a system thread or a non-GUI thread, tell CSRSS to do the
     * work and wait for it to finish. Otherwise, we'll do the work ourselves.
     */
    if (NT_SUCCESS(Status)) {
        if (IS_SYSTEM_THREAD(PsGetCurrentThread()) ||
                W32GetCurrentThread() == NULL) {
            KeSetEvent(gpEventPowerRequest, EVENT_INCREMENT, FALSE);
        } else {
            EnterCrit();
            ThreadLockPool(PtiCurrent(), pPowerRequest, &tlPool);
            xxxUserPowerCalloutWorker();
            ThreadUnlockPool(PtiCurrent(), &tlPool);
            LeaveCrit();
        }
        Status = KeWaitForSingleObject(&pPowerRequest->Event,
                                       WrUserRequest,
                                       KernelMode,
                                       FALSE,
                                       NULL);

        if (NT_SUCCESS(Status)) {
            Status = pPowerRequest->Status;
        }
    }

    /*
     * Free the power request.
     */
    UserAssert(pPowerRequest != gpPowerRequestCurrent);
    UserFreePool(pPowerRequest);

    return Status;
}

/***************************************************************************\
* UnqueuePowerRequest
*
* Remove a power request from the list.
*
* History:
* 20-Oct-1998 JerrySh   Created.
\***************************************************************************/

PPOWERREQUEST
UnqueuePowerRequest(VOID)
{
    PLIST_ENTRY pEntry;
    PPOWERREQUEST pPowerRequest = NULL;

    /*
     * Remove a power request from the list.
     */
    EnterPowerCrit();
    if (!IsListEmpty(&gPowerRequestList)) {
        pEntry = RemoveTailList(&gPowerRequestList);
        pPowerRequest = CONTAINING_RECORD(pEntry, POWERREQUEST, PowerRequestLink);
    }
    LeavePowerCrit();

    return pPowerRequest;
}

/***************************************************************************\
* InitializePowerRequestList
*
* Initialize global power request list state.
*
* History:
* 20-Oct-1998 JerrySh   Created.
\***************************************************************************/

NTSTATUS
InitializePowerRequestList(
    HANDLE hPowerRequestEvent)
{
    NTSTATUS Status;

    InitializeListHead(&gPowerRequestList);

    Status = ObReferenceObjectByHandle(hPowerRequestEvent,
                                       EVENT_ALL_ACCESS,
                                       *ExEventObjectType,
                                       KernelMode,
                                       &gpEventPowerRequest,
                                       NULL);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    gpPowerRequestMutex = UserAllocPoolNonPaged(sizeof(FAST_MUTEX), TAG_POWER);
    if (gpPowerRequestMutex == NULL) {
        return STATUS_NO_MEMORY;
    }
    ExInitializeFastMutex(gpPowerRequestMutex);

    return STATUS_SUCCESS;
}

/***************************************************************************\
* CleanupPowerRequestList
*
* Cancel any pending power requests.
*
* History:
* 20-Oct-1998 JerrySh   Created.
\***************************************************************************/

VOID
CleanupPowerRequestList(VOID)
{
    PPOWERREQUEST pPowerRequest;

    /*
     * Make sure no new power requests come in.
     */
    gbNoMorePowerCallouts = TRUE;

    /*
     * If we never allocated anything, there's nothing to clean up.
     */
    if (gpPowerRequestMutex == NULL) {
        return;
    }

    /*
     * Mark any pending power requests as cacelled.
     */
    while ((pPowerRequest = UnqueuePowerRequest()) != NULL) {
        CancelPowerRequest(pPowerRequest);
    }
}

/***************************************************************************\
* DeletePowerRequestList
*
* Clean up any global power request state.
*
* History:
* 20-Oct-1998 JerrySh   Created.
\***************************************************************************/

VOID
DeletePowerRequestList(VOID)
{
    if (gpPowerRequestMutex) {

        /*
         * Make sure there are no pending power requests.
         */
        UserAssert(IsListEmpty(&gPowerRequestList));

        /*
         * Free the power request structures.
         */
        UserFreePool(gpPowerRequestMutex);
        gpPowerRequestMutex = NULL;
    }
}

/***************************************************************************\
* UserPowerEventCalloutWorker
*
* History:
* 02-Dec-1996 JerrySh   Created.
\***************************************************************************/

NTSTATUS xxxUserPowerEventCalloutWorker(
    PKWIN32_POWEREVENT_PARAMETERS Parms)
{
    BROADCASTSYSTEMMSGPARAMS bsmParams;
    NTSTATUS Status = STATUS_SUCCESS;
    PSPOWEREVENTTYPE EventNumber = Parms->EventNumber;
    ULONG_PTR Code = Parms->Code;
    BOOL bCurrentPowerOn;

    /*
     * Make sure CSRSS is still running.
     */
    if (gbNoMorePowerCallouts) {
        return STATUS_UNSUCCESSFUL;
    }

    switch (EventNumber) {
    case PsW32FullWake:
        /*
         * Let all the applications know that they can resume operation.
         */
        bsmParams.dwRecipients = BSM_ALLDESKTOPS;
        bsmParams.dwFlags = BSF_QUEUENOTIFYMESSAGE;
        xxxSendMessageBSM(NULL,
                          WM_POWERBROADCAST,
                          PBT_APMRESUMESUSPEND,
                          0,
                          &bsmParams);
        break;

    case PsW32EventCode:
        /*
         * Post a message to winlogon, and let them put up a message box
         * or play a sound.
         */

        if (gspwndLogonNotify) {
            _PostMessage(gspwndLogonNotify, WM_LOGONNOTIFY, LOGON_PLAYPOWERSOUND, (ULONG)Code);
            Status = STATUS_SUCCESS;
        } else {
            Status = STATUS_UNSUCCESSFUL;
        }

        break;

    case PsW32PowerPolicyChanged:
        /*
         * Set video timeout value.
         */
        xxxSystemParametersInfo(SPI_SETLOWPOWERTIMEOUT, (ULONG)Code, 0, 0);
        xxxSystemParametersInfo(SPI_SETPOWEROFFTIMEOUT, (ULONG)Code, 0, 0);
        break;

    case PsW32SystemPowerState:
        /*
         * Let all the applications know that the power status has changed.
         */
        bsmParams.dwRecipients = BSM_ALLDESKTOPS;
        bsmParams.dwFlags = BSF_POSTMESSAGE;
        xxxSendMessageBSM(NULL,
                          WM_POWERBROADCAST,
                          PBT_APMPOWERSTATUSCHANGE,
                          0,
                          &bsmParams);
        break;

    case PsW32SystemTime:
        /*
         * Let all the applications know that the system time has changed.
         */
        bsmParams.dwRecipients = BSM_ALLDESKTOPS;
        bsmParams.dwFlags = BSF_POSTMESSAGE;
        xxxSendMessageBSM(NULL,
                          WM_TIMECHANGE,
                          0,
                          0,
                          &bsmParams);
        break;

    case PsW32DisplayState:
        /*
         * Set video timeout active status.
         */
        xxxSystemParametersInfo(SPI_SETLOWPOWERACTIVE, !Code, 0, 0);
        xxxSystemParametersInfo(SPI_SETPOWEROFFACTIVE, !Code, 0, 0);
        break;

    case PsW32GdiOff:
        /*
         * At this point we will disable the display device
         */

        DrvSetMonitorPowerState(gpDispInfo->pmdev, PowerDeviceD3);

        bCurrentPowerOn = DrvQueryMDEVPowerState(gpDispInfo->pmdev);
        if (bCurrentPowerOn) {
            DrvDisableMDEV(gpDispInfo->pmdev, TRUE);
        }
        DrvSetMDEVPowerState(gpDispInfo->pmdev, FALSE);

        break;

    case PsW32GdiOn:
        /*
         * Call video driver to turn the display back on.
         */
        bCurrentPowerOn = DrvQueryMDEVPowerState(gpDispInfo->pmdev);
        if (!bCurrentPowerOn) {
            DrvEnableMDEV(gpDispInfo->pmdev, TRUE);
        }
        DrvSetMDEVPowerState(gpDispInfo->pmdev, TRUE);
        DrvSetMonitorPowerState(gpDispInfo->pmdev, PowerDeviceD0);

        /*
         * Repaint the whole screen
         */
        xxxUserResetDisplayDevice();

        if (gbHibernate)
        {
            HANDLE pdo;

            PVOID PhysDisp = DrvWakeupHandler(&pdo);

            if (PhysDisp)
            {
                UNICODE_STRING   strDeviceName;
                DEVMODEW         NewMode;
                ULONG            bPrune;

                if (DrvDisplaySwitchHandler(PhysDisp, &strDeviceName, &NewMode, &bPrune))
                {
                    /*
                     * CSRSS is not the only process to diliver power callout
                     */
                    if (!ISCSRSS()) {
                        xxxUserChangeDisplaySettings(NULL, NULL, NULL, grpdeskRitInput,
                                 ((bPrune) ? 0 : CDS_RAWMODE) | CDS_TRYCLOSEST | CDS_RESET, 0, KernelMode);
                    }
                    else
                    {
                        DESKRESTOREDATA drdRestore;

                        drdRestore.pdeskRestore = NULL;
                        if (NT_SUCCESS (xxxSetCsrssThreadDesktop(grpdeskRitInput, &drdRestore)) )
                        {
                            xxxUserChangeDisplaySettings(NULL, NULL, NULL, NULL,
                                     ((bPrune) ? 0 : CDS_RAWMODE) | CDS_TRYCLOSEST | CDS_RESET, 0, KernelMode);
                            xxxRestoreCsrssThreadDesktop(&drdRestore);
                        }
                    }
                }

                //
                // If there is a requirement to reenumerate sub-devices
                //
                if (pdo)
                {
                    IoInvalidateDeviceRelations((PDEVICE_OBJECT)pdo, BusRelations);
                }
            }
        }
        gbHibernate = FALSE;

        break;

    default:
        Status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    return Status;
}

/***************************************************************************\
* UserPowerEventCallout
*
* History:
* 02-Dec-1996 JerrySh   Created.
\***************************************************************************/

NTSTATUS UserPowerEventCallout(
    PKWIN32_POWEREVENT_PARAMETERS Parms)
{

    /*
     * Make sure CSRSS is running.
     */
    if (!gbVideoInitialized || gbNoMorePowerCallouts) {
        return STATUS_UNSUCCESSFUL;
    }

    UserAssert(gpepCSRSS != NULL);

    /*
     * Process the power request.
     */
    return QueuePowerRequest(Parms);
}

/***************************************************************************\
* UserPowerStateCalloutWorker
*
* History:
* 02-Dec-1996 JerrySh   Created.
\***************************************************************************/

NTSTATUS xxxUserPowerStateCalloutWorker(VOID)
{
    BOOL fContinue;
    BROADCASTSYSTEMMSGPARAMS bsmParams;
    NTSTATUS Status = STATUS_SUCCESS;
    TL tlpwnd;

    /*
     * Make sure CSRSS is still running.
     */
    if (gbNoMorePowerCallouts) {
        return STATUS_UNSUCCESSFUL;
    }

    /*
     * Store the event so this thread can be promoted later.
     */
    EnterPowerCrit();
    gPowerState.pEvent = PtiCurrent()->pEventQueueServer;
    LeavePowerCrit();

    if (!gPowerState.fCritical) {
        /*
         * Ask the applications if we can suspend operation.
         */
        if (gPowerState.fQueryAllowed) {
            gPowerState.bsmParams.dwRecipients = BSM_ALLDESKTOPS;
            gPowerState.bsmParams.dwFlags = BSF_NOHANG | BSF_FORCEIFHUNG;
            if (gPowerState.fUIAllowed) {
                gPowerState.bsmParams.dwFlags |= BSF_ALLOWSFW;
            }
            if (gPowerState.fOverrideApps == FALSE) {
                gPowerState.bsmParams.dwFlags |= (BSF_QUERY | BSF_NOTIMEOUTIFNOTHUNG);
            }
            fContinue = xxxSendMessageBSM(NULL,
                                          WM_POWERBROADCAST,
                                          PBT_APMQUERYSUSPEND,
                                          gPowerState.fUIAllowed,
                                          &gPowerState.bsmParams);

            /*
             * If an app says to abort and we're not in override apps or
             * critical mode, send out the suspend failed message and bail.
             */
            if (!(fContinue || gPowerState.fOverrideApps || gPowerState.fCritical)) {
                gPowerState.bsmParams.dwRecipients = BSM_ALLDESKTOPS;
                gPowerState.bsmParams.dwFlags = BSF_QUEUENOTIFYMESSAGE;
                xxxSendMessageBSM(NULL,
                                  WM_POWERBROADCAST,
                                  PBT_APMQUERYSUSPENDFAILED,
                                  0,
                                  &gPowerState.bsmParams);
                EnterPowerCrit();
                gPowerState.pEvent = NULL;
                gPowerState.fInProgress = FALSE;
                LeavePowerCrit();
                return STATUS_CANCELLED;
            }
        }

        /*
         * Let all the applications know they should suspend operation.
         */
        if (!gPowerState.fCritical) {
            gPowerState.bsmParams.dwRecipients = BSM_ALLDESKTOPS;
            gPowerState.bsmParams.dwFlags = BSF_NOHANG | BSF_FORCEIFHUNG;
            xxxSendMessageBSM(NULL,
                              WM_POWERBROADCAST,
                              PBT_APMSUSPEND,
                              0,
                              &gPowerState.bsmParams);
        }
    }

    /*
     * Clear the event so the thread won't wake up prematurely.
     */
    EnterPowerCrit();
    gPowerState.pEvent = NULL;
    LeavePowerCrit();

    /*
     * Look for a Winlogon window to notify.
     */
    if (gspwndLogonNotify != NULL) {
        gPowerState.psParams.FullScreenMode = !fGdiEnabled;
        ThreadLockAlways(gspwndLogonNotify, &tlpwnd);
        Status = (NTSTATUS)xxxSendMessage(gspwndLogonNotify,
                                          WM_LOGONNOTIFY,
                                          LOGON_POWERSTATE,
                                          (LPARAM)&gPowerState.psParams);
        ThreadUnlock(&tlpwnd);
    }

    /*
     * The power state broadcast is over.
     */
    EnterPowerCrit();
    gPowerState.fInProgress = FALSE;
    LeavePowerCrit();

    /*
     * Tickle the input time so we don't fire up a screen saver right away.
     */
    glinp.timeLastInputMessage = NtGetTickCount();

    /*
     * Let all the applications know that we're waking up.
     */
    bsmParams.dwRecipients = BSM_ALLDESKTOPS;
    bsmParams.dwFlags = BSF_QUEUENOTIFYMESSAGE;
    xxxSendMessageBSM(NULL,
                      WM_POWERBROADCAST,
                      PBT_APMRESUMEAUTOMATIC,
                      0,
                      &bsmParams);

    return Status;
}

/***************************************************************************\
* UserPowerStateCallout
*
* History:
* 02-Dec-1996 JerrySh   Created.
\***************************************************************************/

NTSTATUS UserPowerStateCallout(
    PKWIN32_POWERSTATE_PARAMETERS Parms)
{
    BOOLEAN Promotion = Parms->Promotion;
    POWER_ACTION SystemAction = Parms->SystemAction;
    SYSTEM_POWER_STATE MinSystemState = Parms->MinSystemState;
    ULONG Flags = Parms->Flags;

    /*
     * Make sure CSRSS is running.
     */
    if (!gbVideoInitialized || gbNoMorePowerCallouts || !gspwndLogonNotify) {
        return STATUS_UNSUCCESSFUL;
    }

    UserAssert(gpepCSRSS != NULL);

    EnterPowerCrit();

    /*
     * Make sure we're not trying to promote a non-existent request
     * or start a new one when we're already doing it.
     */
    if ((Promotion && !gPowerState.fInProgress) ||
        (!Promotion && gPowerState.fInProgress)) {
        LeavePowerCrit();
        return STATUS_INVALID_PARAMETER;
    }

    /*
     * Save our state.
     */
    gPowerState.fInProgress = TRUE;
    gPowerState.fOverrideApps = (Flags & POWER_ACTION_OVERRIDE_APPS) != 0;
    gPowerState.fCritical = (Flags & POWER_ACTION_CRITICAL) != 0;
    gPowerState.fQueryAllowed = (Flags & POWER_ACTION_QUERY_ALLOWED) != 0;
    gPowerState.fUIAllowed = (Flags & POWER_ACTION_UI_ALLOWED) != 0;
    gPowerState.psParams.SystemAction = SystemAction;
    gPowerState.psParams.MinSystemState = MinSystemState;
    gPowerState.psParams.Flags = Flags;
    if (gPowerState.fOverrideApps) {
        gPowerState.bsmParams.dwFlags = BSF_NOHANG | BSF_FORCEIFHUNG;
    }
    if (gPowerState.fCritical) {
        gPowerState.bsmParams.dwFlags = BSF_NOHANG | BSF_QUERY;
    }
    if (gPowerState.pEvent) {
        KeSetEvent(gPowerState.pEvent, EVENT_INCREMENT, FALSE);
    }

    LeavePowerCrit();

    /*
     * If this is a promotion, we're done.
     */
    if (Promotion) {
        return STATUS_SUCCESS;
    }

    /*
     * Process the power request.
     */
    return QueuePowerRequest(NULL);
}

/***************************************************************************\
* UserPowerCalloutWorker
*
* Pull any pending power requests off the list and call the appropriate
* power callout function.
*
* History:
* 02-Dec-1996 JerrySh   Created.
\***************************************************************************/

VOID
xxxUserPowerCalloutWorker(VOID)
{
    PPOWERREQUEST pPowerRequest;
    TL tlPool;

    while ((pPowerRequest = UnqueuePowerRequest()) != NULL) {
        /*
         * Make sure the event gets signalled even if the thread dies in a
         * callback or the waiting thread might get stuck.
         */
        ThreadLockPoolCleanup(PtiCurrent(), pPowerRequest, &tlPool, CancelPowerRequest);

        /*
         * Call the appropriate power worker function.
         */
        gpPowerRequestCurrent = pPowerRequest;
        if (pPowerRequest->Parms) {
            pPowerRequest->Status = xxxUserPowerEventCalloutWorker(pPowerRequest->Parms);
        } else {
            pPowerRequest->Status = xxxUserPowerStateCalloutWorker();
        }
        gpPowerRequestCurrent = NULL;

        /*
         * Tell the waiting thread to proceed.
         */
        ThreadUnlockPoolCleanup(PtiCurrent(), &tlPool);
        KeSetEvent(&pPowerRequest->Event, EVENT_INCREMENT, FALSE);
    }
}



/***************************************************************************\
* VideoPortCalloutThread
*
* Call the appropriate power callout function and return.
*
* History:
* 02-Dec-1996 JerrySh   Created.
\***************************************************************************/

VOID
VideoPortCalloutThread(
    PVIDEO_WIN32K_CALLBACKS_PARAMS Params
    )
{

    /*
     * Convert this thread to GUI if it's not already converted
     */
    UserAssert(W32GetCurrentThread() == NULL);

    Params->Status = InitSystemThread(NULL);

    if (!NT_SUCCESS(Params->Status)) {
        return;
    }

    // DbgPrint("video --- Before CritSect\n");
    EnterCrit();
    // DbgPrint("video --- After CritSect\n");


    switch (Params->CalloutType) {

    case VideoWakeupCallout:
        gbHibernate = TRUE;
        break;

    case VideoDisplaySwitchCallout:
        {
            UNICODE_STRING   strDeviceName;
            DEVMODEW         NewMode;
            ULONG            bPrune;

            if (Params->PhysDisp != NULL)
            {
                if (DrvDisplaySwitchHandler(Params->PhysDisp, &strDeviceName, &NewMode, &bPrune))
                {
                    DESKRESTOREDATA drdRestore;

                    drdRestore.pdeskRestore = NULL;

                    /*
                     * CSRSS is not the only process to diliver power callout
                     */

                    if (!ISCSRSS() ||
                        NT_SUCCESS (xxxSetCsrssThreadDesktop(grpdeskRitInput, &drdRestore)) )
                    {
                        if (!DrvQueryMDEVPowerState(gpDispInfo->pmdev)) {

                            DrvEnableMDEV(gpDispInfo->pmdev, TRUE);
                            DrvSetMDEVPowerState(gpDispInfo->pmdev, TRUE);
                        }

                        xxxUserChangeDisplaySettings(NULL, NULL, NULL, grpdeskRitInput,
                                 ((bPrune) ? 0 : CDS_RAWMODE) | CDS_TRYCLOSEST | CDS_RESET, 0, KernelMode);

                        if (ISCSRSS())
                        {
                            xxxRestoreCsrssThreadDesktop(&drdRestore);
                        }
                    }
                }
            }
        }

        //
        // If there is a requirement to reenumerate sub-devices
        //
        if (Params->Param)
        {
            IoInvalidateDeviceRelations((PDEVICE_OBJECT)Params->Param, BusRelations);
        }

        Params->Status = STATUS_SUCCESS;
        break;

    case VideoFindAdapterCallout:

        if (Params->Param) {

            DrvEnableMDEV(gpDispInfo->pmdev, TRUE);
            xxxUserResetDisplayDevice();

        } else {

            DrvDisableMDEV(gpDispInfo->pmdev, TRUE);
        }

        Params->Status = STATUS_SUCCESS;
        break;

    default:

        UserAssert(FALSE);

        Params->Status = STATUS_UNSUCCESSFUL;

    }


    // DbgPrint("video --- Before Leave CritSect\n");
    LeaveCrit();
    // DbgPrint("video --- After Leave CritSect\n");


    return;
}


/***************************************************************************\
* VideoPortCallout
*
* History:
* 26-Jul-1998 AndreVa   Created.
\***************************************************************************/

VOID
VideoPortCallout(
    IN PVOID Params
    )
{

    /*
     * To make sure this is a system thread, we create a new thread.
     */

    HANDLE   hThread;
    NTSTATUS Status;

    // DbgPrint("Callout --- Enter !!!\n");

    Status = CreateSystemThread(VideoPortCalloutThread, Params, &hThread);
    if (NT_SUCCESS(Status)) {
        Status = NtWaitForSingleObject(hThread, FALSE, NULL);
        if (NT_SUCCESS(Status)) {
            Status = ((PVIDEO_WIN32K_CALLBACKS_PARAMS)(Params))->Status;
        }
        ZwClose(hThread);
    }

    // DbgPrint("Callout --- Leave !!!\n");

    ((PVIDEO_WIN32K_CALLBACKS_PARAMS)(Params))->Status = Status;
}
