/****************************** Module Header ******************************\
* Module Name: ddemlsvr.C
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* DDE Manager main module - Contains all server side ddeml functions.
*
* 27-Aug-1991 Sanford Staab   Created
* 21-Jan-1992 IanJa           ANSI/Unicode neutralized
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

// globals

PSVR_INSTANCE_INFO psiiList;

DWORD xxxCsDdeInitialize(
PHANDLE phInst,
HWND *phwndEvent,
LPDWORD pMonitorFlags,
DWORD afCmd,
PVOID pcii)
{
    PSVR_INSTANCE_INFO psii;
    PTHREADINFO        ptiCurrent = PtiCurrent();

    CheckCritIn();

    psii = (PSVR_INSTANCE_INFO)HMAllocObject(PtiCurrent(), NULL,
            TYPE_DDEACCESS, sizeof(SVR_INSTANCE_INFO));
    if (psii == NULL) {
        return DMLERR_SYS_ERROR;
    }

    /*
     * We have to tell CreateWindow that window is not created for the same
     * module has the app, (CW_FLAGS_DIFFHMOD), so CreateWindow doesn't
     * assign a hotkey to this window.  Other window are done in the
     * client-server thunk
     */
    Lock(&(psii->spwndEvent), xxxCreateWindowEx(
            0,
            (PLARGE_STRING)gpsi->atomSysClass[ICLS_DDEMLEVENT],
            NULL,
            WS_POPUP | WS_CHILD,
            0, 0, 0, 0,
            (PWND)NULL,
            (PMENU)NULL,
            hModuleWin,
            NULL,
            CW_FLAGS_DIFFHMOD | VER31));

    if (psii->spwndEvent == NULL) {
        HMFreeObject((PVOID)psii);
        return DMLERR_SYS_ERROR;
    }
    /*
     * This GWL offset does NOT leave the critical section!
     */
    xxxSetWindowLongPtr(psii->spwndEvent, GWLP_PSII, (LONG_PTR)PtoH(psii), FALSE);
    psii->afCmd = 0;
    psii->pcii = pcii;
    //
    // Link into global list
    //
    psii->next = psiiList;
    psiiList = psii;

    //
    // Link into per-process list
    //
    psii->nextInThisThread = ptiCurrent->psiiList;
    ptiCurrent->psiiList = psii;

    *phInst = PtoH(psii);
    *phwndEvent = PtoH(psii->spwndEvent);
    xxxChangeMonitorFlags(psii, afCmd);        // sets psii->afCmd;
    *pMonitorFlags = MonitorFlags;
    return DMLERR_NO_ERROR;
}





DWORD _CsUpdateInstance(
HANDLE hInst,
LPDWORD pMonitorFlags,
DWORD afCmd)
{
    PSVR_INSTANCE_INFO psii;

    CheckCritIn();

    psii = (PSVR_INSTANCE_INFO)HMValidateHandleNoRip(hInst, TYPE_DDEACCESS);
    if (psii == NULL) {
        return DMLERR_INVALIDPARAMETER;
    }
    xxxChangeMonitorFlags(psii, afCmd);
    *pMonitorFlags = MonitorFlags;
    return DMLERR_NO_ERROR;
}





BOOL _CsDdeUninitialize(
HANDLE hInst)
{
    PSVR_INSTANCE_INFO psii;

    CheckCritIn();

    psii = HMValidateHandleNoRip(hInst, TYPE_DDEACCESS);
    if (psii == NULL) {
        return TRUE;
    }

    xxxDestroyThreadDDEObject(PtiCurrent(), psii);
    return TRUE;
}


VOID xxxDestroyThreadDDEObject(
PTHREADINFO pti,
PSVR_INSTANCE_INFO psii)
{
    PSVR_INSTANCE_INFO psiiT;

    CheckCritIn();

    if (HMIsMarkDestroy(psii)) {
        return;
    }

    //
    // Unlink psii from the global list.
    //
    if (psii == psiiList) {
        psiiList = psii->next;
    } else {
        for (psiiT = psiiList; psiiT->next != psii; psiiT = psiiT->next) {
            UserAssert(psiiT->next != NULL);
        }
        psiiT->next = psii->next;
    }
    // psii->next = NULL;

    //
    // Unlink psii from the per-process list.
    //
    if (psii == pti->psiiList) {
        pti->psiiList = psii->nextInThisThread;
    } else {
        for (psiiT = pti->psiiList; psiiT->nextInThisThread != psii; psiiT = psiiT->nextInThisThread) {
            UserAssert(psiiT->nextInThisThread != NULL);
        }
        psiiT->nextInThisThread = psii->nextInThisThread;
    }
    // psii->nextInThisThread = NULL;

    if (HMMarkObjectDestroy(psii)) {
        xxxDestroyWindow(psii->spwndEvent);
        Unlock(&(psii->spwndEvent));

        HMFreeObject(psii);
    }
}
