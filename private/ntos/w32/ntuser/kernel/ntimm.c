/**************************************************************************\
* Module Name: ntimm.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains IMM functionality
*
* History:
* 21-Dec-1995 wkwok
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop


static CONST WCHAR wszDefaultIme[] = L"Default IME";

#if DBG
BOOL CheckOwnerCirculate(PWND pwnd)
{
    PWND pwndT = pwnd->spwndOwner;

    while (pwndT) {
        UserAssert(pwndT->spwndOwner != pwnd);
        pwndT = pwndT->spwndOwner;
    }
    return TRUE;
}
#endif

/**************************************************************************\
* CreateInputContext
*
* Create input context object.
*
* History:
* 21-Dec-1995 wkwok       Created
\**************************************************************************/

PIMC CreateInputContext(
    ULONG_PTR dwClientImcData)
{
    PTHREADINFO    ptiCurrent;
    PIMC           pImc;
    PDESKTOP       pdesk = NULL;

    ptiCurrent = PtiCurrentShared();

    /*
     * Only for thread that wants IME processing.
     */
    if ((ptiCurrent->TIF_flags & TIF_DISABLEIME) || !IS_IME_ENABLED()) {
        RIPMSG1(RIP_VERBOSE, "CreateInputContext: TIF_DISABLEIME or !IME Enabled. pti=%#p", ptiCurrent);
        return NULL;
    }

    /*
     * If pti->spDefaultImc is NULL (means this is the first instance)
     * but dwClientImcData is not 0, some bogus application like NtCrash
     * has tried to trick the kernel. Just bail out.
     */
    if (dwClientImcData != 0 && ptiCurrent->spDefaultImc == NULL) {
        RIPMSG2(RIP_WARNING, "CreateInputContext: bogus value(0x%08x) is passed. pti=%#p",
                dwClientImcData, ptiCurrent);
        return NULL;
    }

    /*
     * If the windowstation has been initialized, allocate from
     * the current desktop.
     */
    pdesk = ptiCurrent->rpdesk;
#ifdef LATER
    RETURN_IF_ACCESS_DENIED(ptiCurrent->amdesk, DESKTOP_CREATEINPUTCONTEXT, NULL);
#else
    if (ptiCurrent->rpdesk == NULL) {
        return NULL;
    }
#endif

    pImc = HMAllocObject(ptiCurrent, pdesk, TYPE_INPUTCONTEXT, sizeof(IMC));

    if (pImc == NULL) {
        RIPMSG0(RIP_WARNING, "CreateInputContext: out of memory");
        return NULL;
    }

    if (dwClientImcData == 0) {
        /*
         * We are creating default input context for current thread.
         * Initialize the default input context as head of the
         * per-thread IMC list.
         */
        UserAssert(ptiCurrent->spDefaultImc == NULL);
        Lock(&ptiCurrent->spDefaultImc, pImc);
        pImc->pImcNext = NULL;
    }
    else {
        /*
         * Link it to the per-thread IMC list.
         */
        UserAssert(ptiCurrent->spDefaultImc != NULL);
        pImc->pImcNext = ptiCurrent->spDefaultImc->pImcNext;
        ptiCurrent->spDefaultImc->pImcNext = pImc;
    }

    pImc->dwClientImcData = dwClientImcData;

    return pImc;
}


/**************************************************************************\
* DestroyInputContext
*
* Destroy the specified input context object.
*
* History:
* 21-Dec-1995 wkwok       Created
\**************************************************************************/

BOOL DestroyInputContext(
    IN PIMC pImc)
{
    PTHREADINFO ptiImcOwner;
    PBWL        pbwl;
    PWND        pwnd;
    HWND       *phwnd;
    PHE         phe;

    ptiImcOwner = GETPTI(pImc);

    /*
     * Cannot destroy input context from other thread.
     */
    if (ptiImcOwner != PtiCurrent()) {
        RIPERR0(ERROR_ACCESS_DENIED, RIP_WARNING,
              "DestroyInputContext: pImc not of current pti");
        return FALSE;
    }

    /*
     * Cannot destroy default input context.
     */
    if (pImc == ptiImcOwner->spDefaultImc) {
        RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING,
              "DestroyInputContext: can't destroy default Imc");
        return FALSE;
    }

    /*
     * Cleanup destroyed input context from each associated window.
     */
    pbwl = BuildHwndList(ptiImcOwner->rpdesk->pDeskInfo->spwnd->spwndChild,
                             BWL_ENUMLIST|BWL_ENUMCHILDREN, ptiImcOwner);

    if (pbwl != NULL) {

        for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {
            /*
             * Make sure this hwnd is still around.
             */
            if ((pwnd = RevalidateHwnd(*phwnd)) == NULL)
                continue;

            /*
             * Cleanup by associating the default input context.
             */
            if (pwnd->hImc == (HIMC)PtoH(pImc))
                AssociateInputContext(pwnd, ptiImcOwner->spDefaultImc);
        }

        FreeHwndList(pbwl);
    }

    phe = HMPheFromObject(pImc);

    /*
     * Make sure this object isn't already marked to be destroyed - we'll
     * do no good if we try to destroy it now since it is locked.
     */
    if (!(phe->bFlags & HANDLEF_DESTROY))
        HMDestroyUnlockedObject(phe);

    return TRUE;
}


/**************************************************************************\
* FreeInputContext
*
* Free up the specified input context object.
*
* History:
* 21-Dec-1995 wkwok       Created
\**************************************************************************/

VOID FreeInputContext(
    IN PIMC pImc)
{
    PIMC pImcT;

    /*
     * Mark it for destruction.  If it the object is locked it can't
     * be freed right now.
     */
    if (!HMMarkObjectDestroy((PVOID)pImc))
        return;

    /*
     * Unlink it.
     */
    pImcT = GETPTI(pImc)->spDefaultImc;

    while (pImcT != NULL && pImcT->pImcNext != pImc)
        pImcT = pImcT->pImcNext;

    if (pImcT != NULL)
        pImcT->pImcNext = pImc->pImcNext;

    /*
     * We're really going to free the input context.
     */
    HMFreeObject((PVOID)pImc);

    return;
}


/**************************************************************************\
* UpdateInputContext
*
* Update the specified input context object according to UpdateType.
*
* History:
* 21-Dec-1995 wkwok       Created
\**************************************************************************/

BOOL UpdateInputContext(
    IN PIMC pImc,
    IN UPDATEINPUTCONTEXTCLASS UpdateType,
    IN ULONG_PTR UpdateValue)
{
    PTHREADINFO ptiCurrent, ptiImcOwner;

    ptiCurrent = PtiCurrent();
    ptiImcOwner = GETPTI(pImc);

    /*
     * Cannot update input context from other process.
     */
    if (ptiImcOwner->ppi != ptiCurrent->ppi) {
        RIPERR0(ERROR_ACCESS_DENIED, RIP_WARNING, "UpdateInputContext: pImc not of current ppi");
        return FALSE;
    }


    switch (UpdateType) {

    case UpdateClientInputContext:
        if (pImc->dwClientImcData != 0) {
            RIPERR0(RIP_WARNING, RIP_WARNING, "UpdateInputContext: pImc->dwClientImcData != 0");
            return FALSE;
        }
        pImc->dwClientImcData = UpdateValue;
        break;

    case UpdateInUseImeWindow:
        pImc->hImeWnd = (HWND)UpdateValue;
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


/**************************************************************************\
* AssociateInputContext
*
* Associate input context object to the specified window.
*
* History:
* 21-Dec-1995 wkwok       Created
\**************************************************************************/

HIMC AssociateInputContext(
    IN PWND  pWnd,
    IN PIMC  pImc)
{
    HIMC hImcRet = pWnd->hImc;
    pWnd->hImc = (HIMC)PtoH(pImc);

    return hImcRet;
}

AIC_STATUS AssociateInputContextEx(
    IN PWND  pWnd,
    IN PIMC  pImc,
    IN DWORD dwFlag)
{
    PTHREADINFO ptiWnd = GETPTI(pWnd);
    PWND pWndFocus = ptiWnd->pq->spwndFocus;
    HIMC hImcFocus = pWndFocus->hImc;
    BOOL fIgnoreNoContext = (dwFlag & IACE_IGNORENOCONTEXT) == IACE_IGNORENOCONTEXT;
    AIC_STATUS Status = AIC_SUCCESS;

    if (dwFlag & IACE_DEFAULT) {
        /*
         * use default input context.
         */
        pImc = ptiWnd->spDefaultImc;

    } else if (pImc != NULL && GETPTI(pImc) != ptiWnd) {
        /*
         * Cannot associate input context to window created
         * by other thread.
         */
        RIPERR0(ERROR_ACCESS_DENIED, RIP_WARNING,
                "AssociateInputContextEx: pwnd not of Imc pti");
        return AIC_ERROR;
    }

    /*
     * Cannot do association under different process context.
     */
    if (GETPTI(pWnd)->ppi != PtiCurrent()->ppi) {
        RIPERR0(ERROR_ACCESS_DENIED, RIP_WARNING,
                "AssociateInputContextEx: pwnd not of current ppi");
        return AIC_ERROR;
    }

    /*
     * Finally, make sure they are on the same desktop.
     */
    if (pImc != NULL && pImc->head.rpdesk != pWnd->head.rpdesk) {
        RIPERR0(ERROR_ACCESS_DENIED, RIP_WARNING,
                "AssociateInputContextEx: no desktop access");
        return AIC_ERROR;
    }

    /*
     * If IACE_CHILDREN is specified, associate the input context
     * to the child windows of pWnd as well.
     */
    if ((dwFlag & IACE_CHILDREN) && pWnd->spwndChild != NULL) {
        PBWL        pbwl;
        PWND        pwndT;
        HWND       *phwndT;

        pbwl = BuildHwndList(pWnd->spwndChild,
                   BWL_ENUMLIST|BWL_ENUMCHILDREN, ptiWnd);

        if (pbwl != NULL) {

            for (phwndT = pbwl->rghwnd; *phwndT != (HWND)1; phwndT++) {
                /*
                 * Make sure this hwnd is still around.
                 */
                if ((pwndT = RevalidateHwnd(*phwndT)) == NULL)
                    continue;

                if (pwndT->hImc == (HIMC)PtoH(pImc))
                    continue;

                if (pwndT->hImc == NULL_HIMC && fIgnoreNoContext)
                    continue;

                AssociateInputContext(pwndT, pImc);

                if (pwndT == pWndFocus)
                    Status = AIC_FOCUSCONTEXTCHANGED;
            }

            FreeHwndList(pbwl);
        }
    }

    /*
     * Associate the input context to pWnd.
     */
    if (pWnd->hImc != NULL_HIMC || !fIgnoreNoContext) {
        if (pWnd->hImc != (HIMC)PtoH(pImc)) {
            AssociateInputContext(pWnd, pImc);
            if (pWnd == pWndFocus)
                Status = AIC_FOCUSCONTEXTCHANGED;
        }
    }

    return Status;
}


/**************************************************************************\
* xxxFocusSetInputContext
*
* Set active input context upon focus change.
*
* History:
* 21-Mar-1996 wkwok       Created
\**************************************************************************/

VOID xxxFocusSetInputContext(
    IN PWND pWnd,
    IN BOOL fActivate,
    IN BOOL fQueueMsg)
{
    PTHREADINFO pti;
    PWND        pwndDefaultIme;
    TL          tlpwndDefaultIme;

    CheckLock(pWnd);

    pti = GETPTI(pWnd);

    /*
     * CS_IME class or "IME" class windows can not be SetActivated to hImc.
     * WinWord 6.0 US Help calls ShowWindow with the default IME window.
     * HELPMACROS get the default IME window by calling GetNextWindow().
     */
    if (TestCF(pWnd, CFIME) ||
            (pWnd->pcls->atomClassName == gpsi->atomSysClass[ICLS_IME]))
        return;

    /*
     * Do nothing if the thread does not have default IME window.
     */
    if ((pwndDefaultIme = pti->spwndDefaultIme) == NULL)
        return;

    /*
     * If the thread is going away or the default IME window is being vanished,
     * then do nothing.
     */
    if (pti->TIF_flags & TIF_INCLEANUP)
        return;

    UserAssert(!TestWF(pwndDefaultIme, WFDESTROYED));

    ThreadLockAlways(pwndDefaultIme, &tlpwndDefaultIme);

    if (fQueueMsg) {
        xxxSendMessageCallback(pwndDefaultIme, WM_IME_SYSTEM,
                fActivate ? IMS_ACTIVATECONTEXT : IMS_DEACTIVATECONTEXT,
                (LPARAM)HWq(pWnd), NULL, 1L, 0);
    } else {
        xxxSendMessage(pwndDefaultIme, WM_IME_SYSTEM,
                fActivate ? IMS_ACTIVATECONTEXT : IMS_DEACTIVATECONTEXT,
                (LPARAM)HWq(pWnd));
    }

#if _DBG
    if (pti->spwndDefaultIme != pwndDefaultIme) {
        RIPMSG1(RIP_WARNING, "pti(%#p)->spwndDefaultIme got freed during the callback.", pti);
    }
#endif

    ThreadUnlock(&tlpwndDefaultIme);

    return;
}


/**************************************************************************\
* BuildHimcList
*
* Retrieve the list of input context handles created by given thread.
*
* History:
* 21-Feb-1995 wkwok       Created
\**************************************************************************/

UINT BuildHimcList(
    PTHREADINFO pti,
    UINT cHimcMax,
    HIMC *ccxphimcFirst)
{
    PIMC pImcT;
    UINT i = 0;

    if (pti == NULL) {
        /*
         * Build the list which contains all IMCs created by calling process.
         */
        for (pti = PtiCurrent()->ppi->ptiList; pti != NULL; pti = pti->ptiSibling) {
            pImcT = pti->spDefaultImc;
            while (pImcT != NULL) {
                if (i < cHimcMax) {
                    try {
                        ccxphimcFirst[i] = (HIMC)PtoH(pImcT);
                    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
                    }
                }
                i++;
                pImcT = pImcT->pImcNext;
            }
        }
    }
    else {
        /*
         * Build the list which contains all IMCs created by specified thread.
         */
        pImcT = pti->spDefaultImc;
        while (pImcT != NULL) {
            if (i < cHimcMax) {
                try {
                    ccxphimcFirst[i] = (HIMC)PtoH(pImcT);
                } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
                }
            }
            i++;
            pImcT = pImcT->pImcNext;
        }
    }

    return i;
}


/**************************************************************************\
* xxxCreateDefaultImeWindow
*
* Create per-thread based default IME window.
*
* History:
* 21-Mar-1996 wkwok       Created
\**************************************************************************/

PWND xxxCreateDefaultImeWindow(
    IN PWND pwnd,
    IN ATOM atomT,
    IN HANDLE hInst)
{
    LARGE_STRING strWindowName;
    PWND pwndDefaultIme;
    TL tlpwnd;
    PIMEUI pimeui;
    PTHREADINFO ptiCurrent = PtiCurrentShared();
    LPWSTR pwszDefaultIme;

    UserAssert(ptiCurrent == GETPTI(pwnd) && ptiCurrent->spwndDefaultIme == NULL);

    /*
     * Those conditions should have been checked by WantImeWindow()
     * before xxxCreateDefaultImeWindow gets called.
     */
    UserAssert(!(ptiCurrent->TIF_flags & TIF_DISABLEIME));
    UserAssert(!TestWF(pwnd, WFSERVERSIDEPROC));

    /*
     * The first Winlogon thread starts without default input context.
     * Create it now.
     */
    if (ptiCurrent->spDefaultImc == NULL &&
            ptiCurrent->pEThread->Cid.UniqueProcess == gpidLogon)
        CreateInputContext(0);

    /*
     * No default IME window for thread that doesn't have
     * default input context
     */
    if (ptiCurrent->spDefaultImc == NULL)
        return (PWND)NULL;

    /*
     * Avoid recursion
     */
    if (atomT == gpsi->atomSysClass[ICLS_IME] || TestCF(pwnd, CFIME))
        return (PWND)NULL;

    /*
     * B#12165-win95b
     * Yet MFC does another nice. We need to avoid to give an IME window
     * to the child of desktop window which is in different process.
     */
    if (TestwndChild(pwnd) && GETPTI(pwnd->spwndParent)->ppi != ptiCurrent->ppi &&
            !(pwnd->style & WS_VISIBLE))
        return (PWND)NULL;

    if (ptiCurrent->rpdesk->pheapDesktop == NULL)
        return (PWND)NULL;

    /*
     * Allocate storage for L"Default IME" string from desktop heap
     * so that it can be referenced from USER32.DLL in user mode.
     */
    pwszDefaultIme = (LPWSTR)DesktopAlloc(ptiCurrent->rpdesk,
                                          sizeof(wszDefaultIme),
                                          DTAG_IMETEXT);
    if (pwszDefaultIme == NULL)
        return (PWND)NULL;

    RtlCopyMemory(pwszDefaultIme, wszDefaultIme, sizeof(wszDefaultIme));

    RtlInitLargeUnicodeString((PLARGE_UNICODE_STRING)&strWindowName,
                              pwszDefaultIme,
                              (UINT)-1);

    ThreadLock(pwnd, &tlpwnd);

    pwndDefaultIme = xxxCreateWindowEx( (DWORD)0,
                             (PLARGE_STRING)gpsi->atomSysClass[ICLS_IME],
                             (PLARGE_STRING)&strWindowName,
                             WS_POPUP | WS_DISABLED,
                             0, 0, 0, 0,
                             pwnd, (PMENU)NULL,
                             hInst, NULL, VER40);


    if (pwndDefaultIme != NULL) {
        pimeui = ((PIMEWND)pwndDefaultIme)->pimeui;
        UserAssert(pimeui != NULL && (LONG_PTR)pimeui != (LONG_PTR)-1);
        try {
            ProbeForWrite(pimeui, sizeof *pimeui, sizeof(DWORD));
            pimeui->fDefault = TRUE;
            if (TestwndChild(pwnd) && GETPTI(pwnd->spwndParent) != ptiCurrent) {
                pimeui->fChildThreadDef = TRUE;
            }
        } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
        }
    }

    ThreadUnlock(&tlpwnd);

    DesktopFree(ptiCurrent->rpdesk, pwszDefaultIme);

    return pwndDefaultIme;
}


/**************************************************************************\
* xxxImmActivateThreadsLayout
*
* Activate keyboard layout for multiple threads.
*
* Return:
*     TRUE if at least one thread has changed its active keyboard layout.
*     FALSE otherwise
*
* History:
* 11-Apr-1996 wkwok       Created
\**************************************************************************/

BOOL xxxImmActivateThreadsLayout(
    PTHREADINFO pti,
    PTLBLOCK    ptlBlockPrev,
    PKL         pkl)
{
    TLBLOCK     tlBlock;
    PTHREADINFO ptiCurrent, ptiT;
    UINT        cThreads = 0;
    INT         i;

    CheckLock(pkl);

    ptiCurrent = PtiCurrentShared();

    /*
     * Build a list of threads that we need to update their active layouts.
     * We can't just walk the ptiT list while we're doing the work, because
     * for IME based keyboard layout, we will do callback to client side
     * and the ptiT could get deleted out while we leave the critical section.
     */
    for (ptiT = pti; ptiT != NULL; ptiT = ptiT->ptiSibling) {
        /*
         * Skip all the *do nothing* cases in xxxImmActivateLayout
         * so as to minimize the # of TLBLOCK required.
         */
        if (ptiT->spklActive == pkl || (ptiT->TIF_flags & TIF_INCLEANUP))
            continue;

        UserAssert(ptiT->pClientInfo != NULL);
        UserAssert(ptiT->ppi == PpiCurrent()); // can't access pClientInfo of other process

        if (ptiT->spwndDefaultIme == NULL) {
            /*
             * Keyboard layout is being switched but there's no way to callback
             * the client side to activate&initialize input context now.
             * Let's do hkl switching only in the kernel side for this thread
             * but remember the input context needs to be re-initialized
             * when this GUI thread recreates the default IME window later.
             */
            ptiT->hklPrev = ptiT->spklActive->hkl;
            Lock(&ptiT->spklActive, pkl);
            if (ptiT->spDefaultImc) {
                ptiT->pClientInfo->CI_flags |= CI_INPUTCONTEXT_REINIT;
                RIPMSG1(RIP_VERBOSE, "xxxImmActivateThreadsLayout: ptiT(%08p) will be re-initialized.", ptiT);
            }
            UserAssert((ptiT->TIF_flags & TIF_INCLEANUP) == 0);
            ptiT->pClientInfo->hKL = pkl->hkl;
            ptiT->pClientInfo->CodePage = pkl->CodePage;
            continue;
        }

        ThreadLockPti(ptiCurrent, ptiT, &tlBlock.list[cThreads].tlpti);
        tlBlock.list[cThreads++].pti = ptiT;

        if (cThreads == THREADS_PER_TLBLOCK)
            break;
    }

    /*
     * Return FALSE if all the threads already had the pkl active.
     */
    if (ptlBlockPrev == NULL && ptiT == NULL && cThreads == 0)
        return FALSE;

    /*
     * If we can't service all the threads in this run,
     * call ImmActivateThreadsLayout() again for a new TLBLOCK.
     */
    if (ptiT != NULL && ptiT->ptiSibling != NULL) {
        tlBlock.ptlBlockPrev = ptlBlockPrev;
        return xxxImmActivateThreadsLayout(ptiT->ptiSibling, &tlBlock, pkl);
    }

    /*
     * Finally, we can do the actual keyboard layout activation
     * starting from this run. Work on current TLBLOCK first.
     * We walk the list backwards so that the pti unlocks will
     * be done in the right order.
     */

    tlBlock.ptlBlockPrev = ptlBlockPrev;
    ptlBlockPrev = &tlBlock;

    while (ptlBlockPrev != NULL) {
        for (i = cThreads - 1; i >= 0; --i) {
            if ((ptlBlockPrev->list[i].pti->TIF_flags & TIF_INCLEANUP) == 0) {
                ptiT = ptlBlockPrev->list[i].pti;
                UserAssert(ptiT);
                xxxImmActivateLayout(ptiT, pkl);
                if ((ptiT->TIF_flags & TIF_INCLEANUP) == 0) {
                    ptiT->pClientInfo->hKL = pkl->hkl;
                    ptiT->pClientInfo->CodePage = pkl->CodePage;
                }
            }
            ThreadUnlockPti(ptiCurrent, &ptlBlockPrev->list[i].tlpti);
        }
        ptlBlockPrev = ptlBlockPrev->ptlBlockPrev;
        cThreads = THREADS_PER_TLBLOCK;
    }

    return TRUE;
}

VOID xxxImmActivateAndUnloadThreadsLayout(
    IN PTHREADINFO *ptiList,
    IN UINT         nEntries,
    IN PTLBLOCK     ptlBlockPrev,
    PKL             pklCurrent,
    DWORD           dwHklReplace)
{
    TLBLOCK     tlBlock;
    PTHREADINFO ptiCurrent;
    int         i, cThreads;
    enum { RUN_ACTIVATE = 1, RUN_UNLOAD = 2, RUN_FLAGS_MASK = RUN_ACTIVATE | RUN_UNLOAD, RUN_INVALID = 0xffff0000 };

    CheckLock(pklCurrent);

    ptiCurrent = PtiCurrentShared();

    tlBlock.ptlBlockPrev = ptlBlockPrev;

    /*
     * Build a list of threads that we need to unload their IME DLL(s).
     * We can't just walk the ptiList while we're doing the work, because
     * for IME based keyboard layout, we will do callback to client side
     * and the pti could get deleted out while we leave the critical section.
     */
    for (i = 0, cThreads = 0; i < (INT)nEntries; i++) {
        DWORD dwFlags = 0;

        /*
         * Skip all the *do nothing* cases in xxxImmActivateLayout
         * so as to minimize the # of TLBLOCKs required.
         */
        if (ptiList[i]->TIF_flags & TIF_INCLEANUP) {
            dwFlags = RUN_INVALID;
        }
        else if (ptiList[i]->spklActive != pklCurrent) {
            if (ptiList[i]->spwndDefaultIme == NULL) {
                BOOLEAN fAttached = FALSE;

                Lock(&ptiList[i]->spklActive, pklCurrent);
                if (ptiList[i]->pClientInfo != ptiCurrent->pClientInfo &&
                        ptiList[i]->ppi != ptiCurrent->ppi) {
                    /*
                     * If the thread is in another process, attach
                     * to that process so that we can access its ClientInfo.
                     */
                    KeAttachProcess(&ptiList[i]->ppi->Process->Pcb);
                    fAttached = TRUE;
                }

                try {
                    ptiList[i]->pClientInfo->CodePage = pklCurrent->CodePage;
                    ptiList[i]->pClientInfo->hKL = pklCurrent->hkl;
                } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
                      dwFlags = RUN_INVALID;
                }
                if (fAttached) {
                    KeDetachProcess();
                }
            } else {
                dwFlags = RUN_ACTIVATE;
            }
        }

        /*
         * Skip all the *do nothing* cases in xxxImmUnloadLayout()
         * so as to minimize the # of TLBLOCK required.
         * (#99321)
         */
        if (ptiList[i]->spwndDefaultIme != NULL &&
                ptiList[i]->spklActive != NULL &&
                (dwHklReplace != IFL_DEACTIVATEIME || IS_IME_KBDLAYOUT(ptiList[i]->spklActive->hkl)) &&
                dwFlags != RUN_INVALID) {
            dwFlags |= RUN_UNLOAD;
        }

        if (dwFlags && dwFlags != RUN_INVALID) {
            ThreadLockPti(ptiCurrent, ptiList[i], &tlBlock.list[cThreads].tlpti);
#if DBG
            tlBlock.list[cThreads].dwUnlockedCount = 0;
#endif
            tlBlock.list[cThreads].pti = ptiList[i];
            tlBlock.list[cThreads++].dwFlags = dwFlags;

            if (cThreads == THREADS_PER_TLBLOCK) {
                i++;   // 1 more before exit the loop.
                break;
            }
        }
    }

    /*
     * If we can't service all the threads in this run,
     * call xxxImmActivateAndUnloadThreadsLayout again for a new TLBLOCK.
     */
    if (i < (INT)nEntries) {
        ptiList  += i;
        nEntries -= i;
        xxxImmActivateAndUnloadThreadsLayout(ptiList, nEntries, &tlBlock, pklCurrent, dwHklReplace);
        return;
    }

    /*
     * Finally, we can do the actual keyboard layout activation
     * starting from this run. Work on current TLBLOCK first.
     * We walk the list backwards so that the pti unlocks will
     * be done in the right order.
     */
    i = cThreads - 1;
    for (ptlBlockPrev = &tlBlock; ptlBlockPrev != NULL; ptlBlockPrev = ptlBlockPrev->ptlBlockPrev) {
        for ( ; i >= 0; i--) {
            if ((ptlBlockPrev->list[i].dwFlags & RUN_ACTIVATE) &&
                    !(ptlBlockPrev->list[i].pti->TIF_flags & TIF_INCLEANUP)) {
                xxxImmActivateLayout(ptlBlockPrev->list[i].pti, pklCurrent);
            }

            // unlock the thread if the thread is only locked for the first run
            if ((ptlBlockPrev->list[i].dwFlags & RUN_FLAGS_MASK) == RUN_ACTIVATE) {
                ThreadUnlockPti(ptiCurrent, &ptlBlockPrev->list[i].tlpti);
#if DBG
                ptlBlockPrev->list[i].dwUnlockedCount++;
#endif
            }
        }
        i = THREADS_PER_TLBLOCK - 1;
    }

    i = cThreads - 1;
    for (ptlBlockPrev = &tlBlock; ptlBlockPrev != NULL; ptlBlockPrev = ptlBlockPrev->ptlBlockPrev) {
        for ( ; i >= 0; --i) {
            if (ptlBlockPrev->list[i].dwFlags & RUN_UNLOAD) {
                if (!(ptlBlockPrev->list[i].pti->TIF_flags & TIF_INCLEANUP)) {
                    xxxImmUnloadLayout(ptlBlockPrev->list[i].pti, dwHklReplace);
                }
                else {
                    RIPMSG1(RIP_WARNING, "xxxImmActivateAndUnloadThreadsLayout: thread %#p is cleaned up.",
                            ptlBlockPrev->list[i].pti);
                }
                // unlock the thread
                UserAssert((ptlBlockPrev->list[i].dwFlags & RUN_FLAGS_MASK) != RUN_ACTIVATE);
                UserAssert(ptlBlockPrev->list[i].dwUnlockedCount == 0);
                ThreadUnlockPti(ptiCurrent, &ptlBlockPrev->list[i].tlpti);
#if DBG
                ptlBlockPrev->list[i].dwUnlockedCount++;
#endif
            }
        }
        i = THREADS_PER_TLBLOCK - 1;
    }

#if DBG
    // Check if all the locked thread is properly unlocked
    i = cThreads - 1;
    for (ptlBlockPrev = &tlBlock; ptlBlockPrev; ptlBlockPrev = ptlBlockPrev->ptlBlockPrev) {
        for ( ; i >= 0; --i) {
            UserAssert(ptlBlockPrev->list[i].dwUnlockedCount == 1);
        }
        i = THREADS_PER_TLBLOCK - 1;
    }
#endif

    return;
}

/**************************************************************************\
* xxxImmActivateLayout
*
* Activate IME based keyboard layout.
*
* History:
* 21-Mar-1996 wkwok       Created
\**************************************************************************/

VOID xxxImmActivateLayout(
    IN PTHREADINFO pti,
    IN PKL pkl)
{
    TL tlpwndDefaultIme;
    PTHREADINFO ptiCurrent;

    CheckLock(pkl);

    /*
     * Do nothing if it's already been the current active layout.
     */
    if (pti->spklActive == pkl)
        return;

    if (pti->spwndDefaultIme == NULL) {
        /*
         * Only activate kernel side keyboard layout if this pti
         * doesn't have the default IME window.
         */
        Lock(&pti->spklActive, pkl);
        return;
    }

    ptiCurrent = PtiCurrentShared();

    /*
     * Activate client side IME based keyboard layout.
     */
    ThreadLockAlwaysWithPti(ptiCurrent, pti->spwndDefaultIme, &tlpwndDefaultIme);
    xxxSendMessage(pti->spwndDefaultIme, WM_IME_SYSTEM,
                (WPARAM)IMS_ACTIVATETHREADLAYOUT, (LPARAM)pkl->hkl);
    ThreadUnlock(&tlpwndDefaultIme);

    Lock(&pti->spklActive, pkl);

    return;
}


VOID xxxImmUnloadThreadsLayout(
    IN PTHREADINFO *ptiList,
    IN UINT         nEntries,
    IN PTLBLOCK     ptlBlockPrev,
    IN DWORD        dwFlag)
{
    TLBLOCK     tlBlock;
    PTHREADINFO ptiCurrent;
    INT         i, cThreads;
    BOOLEAN     fPerformUnlock;

    ptiCurrent = PtiCurrentShared();
    tlBlock.ptlBlockPrev = ptlBlockPrev;

    /*
     * Build a list of threads that we need to unload their IME DLL(s).
     * We can't just walk the ptiList while we're doing the work, because
     * for IME based keyboard layout, we will do callback to client side
     * and the pti could get deleted out while we leave the critical section.
     */
    for (i = 0, cThreads = 0; i < (INT)nEntries; i++) {
        /*
         * Skip all the *do nothing* cases in xxxImmUnloadLayout()
         * so as to minimize the # of TLBLOCK required.
         */
        if ((ptiList[i]->TIF_flags & TIF_INCLEANUP) || ptiList[i]->spwndDefaultIme == NULL)
            continue;

        if (ptiList[i]->spklActive == NULL)
            continue;

        if (dwFlag == IFL_DEACTIVATEIME &&
                !IS_IME_KBDLAYOUT(ptiList[i]->spklActive->hkl)) // #99321
            continue;

#if DBG
        tlBlock.list[cThreads].dwUnlockedCount = 0;
#endif
        ThreadLockPti(ptiCurrent, ptiList[i], &tlBlock.list[cThreads].tlpti);
        tlBlock.list[cThreads++].pti = ptiList[i];
        if (cThreads == THREADS_PER_TLBLOCK) {
            i++;   // 1 more before exit the loop.
            break;
        }
    }

    if (i < (INT)nEntries) {
        ptiList  += i;
        nEntries -= i;
        xxxImmUnloadThreadsLayout(ptiList, nEntries, &tlBlock, dwFlag);
        return;
    }

    UserAssert(dwFlag == IFL_UNLOADIME || dwFlag == IFL_DEACTIVATEIME);
    if (dwFlag == IFL_UNLOADIME) {
        dwFlag = IFL_DEACTIVATEIME;
        fPerformUnlock = FALSE;
    } else {
        fPerformUnlock = TRUE;
    }
RepeatForUnload:
    /*
     * Finally, we can unload the IME based keyboard layout
     * starting from this run. Work on current TLBLOCK first.
     * We walk the list backwards so that the pti unlocks will
     * be done in the right order.
     */
    i = cThreads - 1;
    for (ptlBlockPrev = &tlBlock; ptlBlockPrev; ptlBlockPrev = ptlBlockPrev->ptlBlockPrev) {
        for ( ; i >= 0; --i) {
            if (!(ptlBlockPrev->list[i].pti->TIF_flags & TIF_INCLEANUP)) {
                xxxImmUnloadLayout(ptlBlockPrev->list[i].pti, dwFlag);
            }
            else {
                RIPMSG2(RIP_WARNING, "Thread %#p is cleaned during the loop for %x!", ptlBlockPrev->list[i].pti, dwFlag);
            }

            if (fPerformUnlock) {
#if DBG
                ptlBlockPrev->list[i].dwUnlockedCount++;
#endif
                ThreadUnlockPti(ptiCurrent, &ptlBlockPrev->list[i].tlpti);
            }
        }
        i = THREADS_PER_TLBLOCK - 1;
    }

    if (!fPerformUnlock) {
        fPerformUnlock = TRUE;
        dwFlag = IFL_UNLOADIME;
        goto RepeatForUnload;
    }

#if DBG
    // Check if all the locked thread is properly unlocked
    i = cThreads - 1;
    for (ptlBlockPrev = &tlBlock; ptlBlockPrev; ptlBlockPrev = ptlBlockPrev->ptlBlockPrev) {
        for ( ; i >= 0; --i) {
            UserAssert(ptlBlockPrev->list[i].dwUnlockedCount == 1);
        }
        i = THREADS_PER_TLBLOCK - 1;
    }
#endif

    return;
}



VOID xxxImmUnloadLayout(
    IN PTHREADINFO pti,
    IN DWORD dwFlag)
{
    TL tlpwndDefaultIme;
    PTHREADINFO ptiCurrent;
    ULONG_PTR dwResult;
    LRESULT r;

    /*
     * Do nothing if the thread does not have default IME window.
     */
    if (pti->spwndDefaultIme == NULL)
        return;

    if (pti->spklActive == NULL)
        return;

    if (dwFlag == IFL_DEACTIVATEIME &&
            !IS_IME_KBDLAYOUT(pti->spklActive->hkl))
        return;

    ptiCurrent = PtiCurrentShared();

    ThreadLockAlwaysWithPti(ptiCurrent, pti->spwndDefaultIme, &tlpwndDefaultIme);
    r = xxxSendMessageTimeout(pti->spwndDefaultIme, WM_IME_SYSTEM,
                          IMS_UNLOADTHREADLAYOUT, (LONG)dwFlag,
                          SMTO_NOTIMEOUTIFNOTHUNG, CMSHUNGAPPTIMEOUT, &dwResult);

    if (!r) {
        RIPMSG1(RIP_WARNING, "Timeout in xxxImmUnloadLayout: perhaps this thread (0x%x) is not pumping messages.",
                pti->pEThread->Cid.UniqueThread);
    }

    ThreadUnlock(&tlpwndDefaultIme);

    return;
}

/**************************************************************************\
* xxxImmLoadLayout
*
* Retrieves extended IMEINFO for the given IME based keyboard layout.
*
* History:
* 21-Mar-1996 wkwok       Created
\**************************************************************************/

PIMEINFOEX xxxImmLoadLayout(
    IN HKL hKL)
{
    PIMEINFOEX  piiex;
    PTHREADINFO ptiCurrent;
    TL          tlPool;

    /*
     * No IMEINFOEX for non-IME based keyboard layout.
     */
    if (!IS_IME_KBDLAYOUT(hKL))
        return (PIMEINFOEX)NULL;

    piiex = (PIMEINFOEX)UserAllocPool(sizeof(IMEINFOEX), TAG_IME);

    if (piiex == NULL) {
        RIPMSG1(RIP_WARNING,
              "xxxImmLoadLayout: failed to create piiex for hkl = %lx", hKL);
        return (PIMEINFOEX)NULL;
    }

    ptiCurrent = PtiCurrent();

    /*
     * Lock this allocations since we are going to the client side
     */
    ThreadLockPool(ptiCurrent, piiex, &tlPool);

    if (!ClientImmLoadLayout(hKL, piiex)) {
        ThreadUnlockAndFreePool(ptiCurrent, &tlPool);
        return (PIMEINFOEX)NULL;
    }

    ThreadUnlockPool(ptiCurrent, &tlPool);

    return piiex;
}


/**************************************************************************\
* GetImeInfoEx
*
* Query extended IMEINFO.
*
* History:
* 21-Mar-1996 wkwok       Created
\**************************************************************************/

BOOL GetImeInfoEx(
    PWINDOWSTATION pwinsta,
    PIMEINFOEX piiex,
    IMEINFOEXCLASS SearchType)
{
    PKL pkl, pklFirst;

    /*
     * Note: this check was forced to insert due to winmm.dll who indirectly
     * loads imm32.dll in CSRSS context. CSRSS is not always bound to
     * specific window station, thus pwinsta could be NULL.
     * This has been avoided by not loading imm32.dll.
     * After winmm.dll gets removed from CSRSS, this if statement should be
     * removed, or substituted as an assertion.
     */
    if (pwinsta == NULL) {
        return FALSE;
    }

    /*
     * Keyboard layer has not been initialized.
     */
    if (pwinsta->spklList == NULL)
        return FALSE;

    pkl = pklFirst = pwinsta->spklList;

    switch (SearchType) {
    case ImeInfoExKeyboardLayout:
        do {
            if (pkl->hkl == piiex->hkl) {

                if (pkl->piiex == NULL)
                    break;

                RtlCopyMemory(piiex, pkl->piiex, sizeof(IMEINFOEX));
                return TRUE;
            }
            pkl = pkl->pklNext;
        } while (pkl != pklFirst);
        break;

    case ImeInfoExImeFileName:
        do {
            if (pkl->piiex != NULL &&
                !_wcsnicmp(pkl->piiex->wszImeFile, piiex->wszImeFile, IM_FILE_SIZE)) {

                RtlCopyMemory(piiex, pkl->piiex, sizeof(IMEINFOEX));
                return TRUE;
            }
            pkl = pkl->pklNext;
        } while (pkl != pklFirst);
        break;

    default:
        break;
    }

    return FALSE;
}


/**************************************************************************\
* SetImeInfoEx
*
* Set extended IMEINFO.
*
* History:
* 21-Mar-1996 wkwok       Created
\**************************************************************************/

BOOL SetImeInfoEx(
    PWINDOWSTATION pwinsta,
    PIMEINFOEX piiex)
{
    PKL pkl, pklFirst;

    if (pwinsta == NULL) {
        return FALSE;
    }

    UserAssert(pwinsta->spklList != NULL);

    pkl = pklFirst = pwinsta->spklList;

    do {
        if (pkl->hkl == piiex->hkl) {

            /*
             * Error out for non-IME based keyboard layout.
             */
            if (pkl->piiex == NULL)
                return FALSE;

            /*
             * Update kernel side IMEINFOEX for this keyboard layout
             * only if this is its first loading.
             */
            if (pkl->piiex->fLoadFlag == IMEF_NONLOAD) {
                RtlCopyMemory(pkl->piiex, piiex, sizeof(IMEINFOEX));
            }

            return TRUE;
        }
        pkl = pkl->pklNext;

    } while (pkl != pklFirst);

    return FALSE;
}


/***************************************************************************\
* xxxImmProcessKey
*
*
* History:
* 03-03-96 TakaoK             Created.
\***************************************************************************/

DWORD xxxImmProcessKey(
    IN PQ   pq,
    IN PWND pwnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    UINT  uVKey;
    PKL   pkl;
    DWORD dwHotKeyID;
    DWORD dwReturn = 0;
    PIMC  pImc = NULL;
    BOOL  fDBERoman = FALSE;
    PIMEHOTKEYOBJ pImeHotKeyObj;
    HKL hklTarget;

    CheckLock(pwnd);

    //
    // we're interested in only keyboard messages.
    //
    if ( message != WM_KEYDOWN    &&
         message != WM_SYSKEYDOWN &&
         message != WM_KEYUP      &&
         message != WM_SYSKEYUP ) {

        return dwReturn;
    }

    //
    // Check if it's IME hotkey. This must be done before checking
    // the keyboard layout because IME hotkey handler should be
    // called even if current keyboard layout is non-IME layout.
    //
    pkl = GETPTI(pwnd)->spklActive;
    if ( pkl == NULL ) {
        return dwReturn;
    }

    uVKey = (UINT)wParam & 0xff;

    pImeHotKeyObj = CheckImeHotKey(pq, uVKey, lParam);
    if (pImeHotKeyObj) {
        dwHotKeyID = pImeHotKeyObj->hk.dwHotKeyID;
        hklTarget = pImeHotKeyObj->hk.hKL;
    }
    else {
        dwHotKeyID = IME_INVALID_HOTKEY;
        hklTarget = (HKL)NULL;
    }

    //
    // Handle Direct KL switching here.
    //
    if (dwHotKeyID >= IME_HOTKEY_DSWITCH_FIRST && dwHotKeyID <= IME_HOTKEY_DSWITCH_LAST) {
        UserAssert(hklTarget != NULL);
        if (pkl->hkl != hklTarget) {
            //
            // Post the message only if the new Keyboard Layout is different from
            // the current Keyboard Layout.
            //
            _PostMessage(pwnd, WM_INPUTLANGCHANGEREQUEST,
                         (pkl->dwFontSigs & gSystemFS) ? INPUTLANGCHANGE_SYSCHARSET : 0,
                         (LPARAM)hklTarget);
        }
        if (GetAppImeCompatFlags(GETPTI(pwnd)) & IMECOMPAT_HYDRACLIENT) {
            return 0;
        }
        return IPHK_HOTKEY;
    }

    if (!IS_IME_ENABLED()) {
        //
        // Since IMM is disabled, no need to process further.
        // Just bail out.
        //
        return 0;
    }

    if ( dwHotKeyID != IME_INVALID_HOTKEY ) {
        //
        // if it's a valid hotkey, go straight and call back
        // the IME in the client side.
        //
        goto ProcessKeyCallClient;
    }

    //
    // if it's not a hotkey, we may want to check something
    // before calling back.
    //
    if ( pkl->piiex == NULL ) {
        return dwReturn;
    }

    //
    // Check input context
    //
    pImc = HtoP(pwnd->hImc);
    if ( pImc == NULL ) {
        return dwReturn;
    }

#ifdef LATER
    //
    // If there is an easy way to check the input context open/close status
    // from the kernel side, IME_PROP_NO_KEYS_ON_CLOSE checking should be
    // done here in kernel side.  [ 3/10/96 takaok]
    //

    //
    // Check IME_PROP_NO_KEYS_ON_CLOSE bit
    //
    // if the current imc is not open and IME doesn't need
    // keys when being closed, we don't pass any keyboard
    // input to ime except hotkey and keys that change
    // the keyboard status.
    //
    if ( (piix->ImeInfo.fdwProperty & IME_PROP_NO_KEYS_ON_CLOSE) &&
         (!pimc->fdwState & IMC_OPEN)                            &&
         uVKey != VK_SHIFT                                       &&  // 0x10
         uVKey != VK_CONTROL                                     &&  // 0x11
         uVKey != VK_CAPITAL                                     &&  // 0x14
         uVKey != VK_KANA                                        &&  // 0x15
         uVKey != VK_NUMLOCK                                     &&  // 0x90
         uVKey != VK_SCROLL )                                        // 0x91
    {
      // Check if Korea Hanja conversion mode
      if( !(pimc->fdwConvMode & IME_CMODE_HANJACONVERT) ) {
          return dwReturn;
      }
    }
#endif

    //
    // if the IME doesn't need key up messages, we don't call ime.
    //
    if ( lParam & 0x80000000 && // set if key up, clear if key down
         pkl->piiex->ImeInfo.fdwProperty & IME_PROP_IGNORE_UPKEYS )
    {
        return dwReturn;
    }

    //
    // we don't want to handle sys keys since many functions for
    // acceelerators won't work without this
    //
    fDBERoman = (BOOL)( (uVKey == VK_DBE_ROMAN)            ||
                        (uVKey == VK_DBE_NOROMAN)          ||
                        (uVKey == VK_DBE_HIRAGANA)         ||
                        (uVKey == VK_DBE_KATAKANA)         ||
                        (uVKey == VK_DBE_CODEINPUT)        ||
                        (uVKey == VK_DBE_NOCODEINPUT)      ||
                        (uVKey == VK_DBE_IME_WORDREGISTER) ||
                        (uVKey == VK_DBE_IME_DIALOG) );

    if (message == WM_SYSKEYDOWN || message == WM_SYSKEYUP ) {
        //
        // IME may be waiting for VK_MENU, VK_F10 or VK_DBE_xxx
        //
        if ( uVKey != VK_MENU && uVKey != VK_F10 && !fDBERoman ) {
            return dwReturn;
        }
    }

    //
    // check if the IME doesn't need ALT key
    //
    if ( !(pkl->piiex->ImeInfo.fdwProperty & IME_PROP_NEED_ALTKEY) ) {
        //
        // IME doesn't need ALT key
        //
        // we don't pass the ALT and ALT+xxx except VK_DBE_xxx keys.
        //
        if ( ! fDBERoman &&
             (uVKey == VK_MENU || (lParam & 0x20000000))  // KF_ALTDOWN
           )
        {
            return dwReturn;
        }
    }

    //
    // finaly call back the client
    //

ProcessKeyCallClient:

    if ((uVKey & 0xff) == VK_PACKET) {
        //
        // need to retrieve UNICODE character from pti
        //
        uVKey = MAKELONG(wParam, PtiCurrent()->wchInjected);
    }
    dwReturn = ClientImmProcessKey( PtoH(pwnd),
                                    pkl->hkl,
                                    uVKey,
                                    lParam,
                                    dwHotKeyID);

    //
    // Hydra server wants to see the IME hotkeys.
    //
    if (GetAppImeCompatFlags(GETPTI(pwnd)) & IMECOMPAT_HYDRACLIENT) {
        dwReturn &= ~IPHK_HOTKEY;
    }
    return dwReturn;
}


/**************************************************************************\
* ImeCanDestroyDefIME
*
* History:
* 02-Apr-1996 wkwok       Ported from FE Win95 (imeclass.c)
\**************************************************************************/

BOOL ImeCanDestroyDefIME(
    PWND pwndDefaultIme,
    PWND pwndDestroy)
{
    PWND   pwnd;
    PIMEUI pimeui;

    pimeui = ((PIMEWND)pwndDefaultIme)->pimeui;

    if (pimeui == NULL || (LONG_PTR)pimeui == (LONG_PTR)-1)
        return FALSE;

    try {
        if (ProbeAndReadStructure(pimeui, IMEUI).fDestroy) {
            return FALSE;
        }
    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
    }

    /*
     * If the pwndDestroy has no owner/ownee relationship with
     * pwndDefaultIme, don't bother to change anything.
     *
     * If pwndDefaultIme->spwndOwner is NULL, this means we need
     * to search for a new good owner window.
     */
    if ( pwndDefaultIme->spwndOwner != NULL ) {
        for (pwnd = pwndDefaultIme->spwndOwner;
             pwnd != pwndDestroy && pwnd != NULL; pwnd = pwnd->spwndOwner) ;

        if (pwnd == NULL)
            return FALSE;
    }

    /*
     * If the destroying window is IME or UI window, do nothing
     */
    pwnd = pwndDestroy;

    while (pwnd != NULL) {
        if (TestCF(pwnd, CFIME) ||
                pwnd->pcls->atomClassName == gpsi->atomSysClass[ICLS_IME])
            return FALSE;

        pwnd = pwnd->spwndOwner;
    }

    ImeSetFutureOwner(pwndDefaultIme, pwndDestroy);

    /*
     * If new owner is lower z-order than IME class window,
     * we need to check topmost to change z-order.
     */
    pwnd = pwndDefaultIme->spwndOwner;
    while (pwnd != NULL && pwnd != pwndDefaultIme)
        pwnd = pwnd->spwndNext;

    if (pwnd == pwndDefaultIme)
        ImeCheckTopmost(pwndDefaultIme);

#if DBG
    CheckOwnerCirculate(pwndDefaultIme);
#endif

    /*
     * If ImeSetFutureOwner can not find the owner window any
     * more, this IME window should be destroyed.
     */
    if (pwndDefaultIme->spwndOwner == NULL ||
            pwndDestroy == pwndDefaultIme->spwndOwner) {

//        RIPMSG1(RIP_WARNING, "ImeCanDestroyDefIME: TRUE for pwnd=%#p", pwndDestroy);
        Unlock(&pwndDefaultIme->spwndOwner);

        /*
         * Return TRUE! Please destroy me.
         */
        return TRUE;
    }

    return FALSE;
}


/**************************************************************************\
* IsChildSameThread (IsChildSameQ)
*
* History:
* 02-Apr-1996 wkwok       Ported from FE Win95 (imeclass.c)
\**************************************************************************/

BOOL IsChildSameThread(
    PWND pwndParent,
    PWND pwndChild)
{
    PWND pwnd;
    PTHREADINFO ptiChild = GETPTI(pwndChild);

    for (pwnd = pwndParent->spwndChild; pwnd; pwnd = pwnd->spwndNext) {
        /*
         * If pwnd is not child window, we need to skip MENU window and
         * IME related window.
         */
        if (!TestwndChild(pwnd)) {
            PWND pwndOwner = pwnd;
            BOOL fFoundOwner = FALSE;

            /*
             * Skip MENU window.
             */
            if (pwnd->pcls->atomClassName == gpsi->atomSysClass[ICLS_MENU])
                continue;

            while (pwndOwner != NULL) {
                /*
                 * CS_IME class or "IME" class windows can not be the owner of
                 * IME windows.
                 */
                if (TestCF(pwndOwner, CFIME) ||
                        pwndOwner->pcls->atomClassName == gpsi->atomSysClass[ICLS_IME]) {
                    fFoundOwner = TRUE;
                    break;
                }

                pwndOwner = pwndOwner->spwndOwner;
            }

            if (fFoundOwner)
                continue;
        }

        /*
         * We need to skip pwndChild.
         */
        if (pwnd == pwndChild)
            continue;

        /*
         * pwnd and pwndChild are on same thread?
         */
        if (GETPTI(pwnd) == ptiChild) {
            PWND pwndT = pwnd;
            BOOL fFoundImeWnd = FALSE;

            /*
             * Check again. If hwndT is children or ownee of
             * IME related window, skip it.
             */
            if (TestwndChild(pwndT)) {

                for (; TestwndChild(pwndT) && GETPTI(pwndT) == ptiChild;
                        pwndT = pwndT->spwndParent) {
                    if (TestCF(pwndT, CFIME) ||
                            pwndT->pcls->atomClassName == gpsi->atomSysClass[ICLS_IME])
                        fFoundImeWnd = TRUE;
                }
            }

            if (!TestwndChild(pwndT)) {

                for (; pwndT != NULL && GETPTI(pwndT) == ptiChild;
                        pwndT = pwndT->spwndOwner) {
                    if (TestCF(pwndT, CFIME) ||
                            pwndT->pcls->atomClassName == gpsi->atomSysClass[ICLS_IME])
                        fFoundImeWnd = TRUE;
                }
            }

            if (!fFoundImeWnd)
                return TRUE;
        }
    }

    return FALSE;
}


/**************************************************************************\
* ImeCanDestroyDefIMEforChild
*
* History:
* 02-Apr-1996 wkwok       Ported from FE Win95 (imeclass.c)
\**************************************************************************/

BOOL ImeCanDestroyDefIMEforChild(
    PWND pwndDefaultIme,
    PWND pwndDestroy)
{
    PWND pwnd;
    PIMEUI pimeui;

    pimeui = ((PIMEWND)pwndDefaultIme)->pimeui;

    /*
     * If this window is not for Child Thread.....
     */
    if (pimeui == NULL || (LONG_PTR)pimeui == (LONG_PTR)-1)
        return FALSE;

    try {
        if (!ProbeAndReadStructure(pimeui, IMEUI).fChildThreadDef) {
            return FALSE;
        }
    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
    }

    /*
     * If parent belongs to different thread,
     * we don't need to check any more...
     */
    if (pwndDestroy->spwndParent == NULL ||
            GETPTI(pwndDestroy) == GETPTI(pwndDestroy->spwndParent))
        return FALSE;

    pwnd = pwndDestroy;

    while (pwnd != NULL && pwnd != PWNDDESKTOP(pwnd)) {
        if (IsChildSameThread(pwnd->spwndParent, pwndDestroy))
            return FALSE;
        pwnd = pwnd->spwndParent;
    }

    /*
     * We could not find any other window created by GETPTI(pwndDestroy).
     * Let's destroy the default IME window of this Q.
     */
    return TRUE;
}


/**************************************************************************\
* ImeCheckTopmost
*
* History:
* 02-Apr-1996 wkwok       Ported from FE Win95 (imeclass.c)
\**************************************************************************/

VOID ImeCheckTopmost(
    PWND pwndIme)
{
    if (pwndIme->spwndOwner) {
        PWND pwndInsertBeforeThis;
        /*
         * The ime window have to be same topmost tyle with the owner window.
         * If the Q of this window is not foreground Q, we don't need to
         * forground the IME window.
         * But the topmost attribute of owner was changed, this IME window
         * should be re-calced.
         */
        if (GETPTI(pwndIme) == gptiForeground) {
            pwndInsertBeforeThis = NULL;
        } else {
            pwndInsertBeforeThis = pwndIme->spwndOwner;
        }

        ImeSetTopmost(pwndIme, TestWF(pwndIme->spwndOwner, WEFTOPMOST) != 0, pwndInsertBeforeThis);
    }
}


/**************************************************************************\
* ImeSetFutureOwner
*
* History:
* 02-Apr-1996 wkwok       Ported from FE Win95 (imeclass.c)
\**************************************************************************/

VOID ImeSetFutureOwner(
    PWND pwndIme,
    PWND pwndOrgOwner)
{
    PWND pwnd, pwndOwner;
    PTHREADINFO ptiImeWnd = GETPTI(pwndIme);

    if (pwndOrgOwner == NULL || TestWF(pwndOrgOwner, WFCHILD))
        return;

    pwnd = pwndOrgOwner;

    /*
     * Get top of owner created by the same thread.
     */
    while ((pwndOwner = pwnd->spwndOwner) != NULL &&
            GETPTI(pwndOwner) == ptiImeWnd)
        pwnd = pwndOwner;

    /*
     * Bottom window can not be the owner of IME window easily...
     */
    if (TestWF(pwnd, WFBOTTOMMOST) && !TestWF(pwndOrgOwner, WFBOTTOMMOST))
        pwnd = pwndOrgOwner;

    /*
     * CS_IME class or "IME" class windows can not be the owner of
     * IME windows.
     */
    if (TestCF(pwnd, CFIME) ||
            pwnd->pcls->atomClassName == gpsi->atomSysClass[ICLS_IME])
        pwnd = pwndOrgOwner;

    /*
     * If hwndOrgOwner is a top of owner, we start to search
     * another top owner window in same queue.
     */
    if (pwndOrgOwner == pwnd && pwnd->spwndParent != NULL) {
        PWND pwndT;

        for (pwndT = pwnd->spwndParent->spwndChild;
                pwndT != NULL; pwndT = pwndT->spwndNext) {

            if (GETPTI(pwnd) != GETPTI(pwndT))
                continue;

            if (pwndT->pcls->atomClassName == gpsi->atomSysClass[ICLS_MENU])
                continue;

            /*
             * CS_IME class or "IME" class windows can not be the owner of
             * IME windows.
             */
            if (TestCF(pwndT, CFIME) ||
                    pwndT->pcls->atomClassName == gpsi->atomSysClass[ICLS_IME])
                continue;

            // We don't like the window that is being destroyed.
            if (TestWF(pwndT, WFINDESTROY))
                continue;

            /*
             * !!!!WARNING!!!!!
             * Is hwndT a good owner of hIMEwnd??
             *  1. Of cource, it should no CHILD window!
             *  2. If it is hwnd,.. I know it and find next!
             *  3. Does hwndT have owner in the same thread?
             */
            if (!TestWF(pwndT, WFCHILD) && pwnd != pwndT &&
                    (pwndT->spwndOwner == NULL ||
                     GETPTI(pwndT) != GETPTI(pwndT->spwndOwner))) {
                UserAssert(GETPTI(pwndIme) == GETPTI(pwndT));
                pwnd = pwndT;
                break;
            }
        }
    }

    UserAssert(!TestCF(pwnd, CFIME));
    Lock(&pwndIme->spwndOwner, pwnd);

    return;
}


/**************************************************************************\
* ImeSetTopmostChild
*
* History:
* 02-Apr-1996 wkwok       Ported from FE Win95 (imeclass.c)
\**************************************************************************/

VOID ImeSetTopmostChild(
    PWND pwndParent,
    BOOL fMakeTopmost)
{
    PWND pwnd = pwndParent->spwndChild;

    while (pwnd != NULL) {
        if (fMakeTopmost)
            SetWF(pwnd, WEFTOPMOST);
        else
            ClrWF(pwnd, WEFTOPMOST);

        ImeSetTopmostChild(pwnd, fMakeTopmost);

        pwnd = pwnd->spwndNext;
    }

    return;
}


/**************************************************************************\
*
*  GetLastTopMostWindowNoIME() -
*
*  Get the last topmost window which is not the ownee of pwndRoot (IME window).
*
\**************************************************************************/

PWND GetLastTopMostWindowNoIME(PWND pwndRoot)
{
    PWND pwndT = _GetDesktopWindow();
    PWND pwndRet = NULL;

    /*
     * pwndRoot should not be NULL, and should be IME window.
     */
    UserAssert(pwndRoot && pwndRoot->pcls->atomClassName == gpsi->atomSysClass[ICLS_IME]);

    if (pwndT == NULL || pwndT->spwndChild == NULL) {
#if _DBG
        if (pwndT == NULL) {
            RIPMSG0(RIP_WARNING, "GetLastTopMostWindowNoIME: there's no desktop window !!");
        }
        else {
            RIPMSG0(RIP_WARNING, "GetLastTopMostWindowNoIME: there is no toplevel window !!");
        }
#endif
        return NULL;
    }

    /*
     * Get the first child of the desktop window.
     */
    pwndT = pwndT->spwndChild;

    /*
     * Loop through the toplevel windows while they are topmost.
     */
    while (TestWF(pwndT, WEFTOPMOST)) {
        PWND pwndOwner = pwndT;
        BOOL fOwned = FALSE;

        /*
         * If pwndT is a IME related window, track the owner. If pwndRoot is not
         * pwndT's owner, remember pwndT as a candidate.
         */
        if (TestCF(pwndT,CFIME) || (pwndT->pcls->atomClassName == gpsi->atomSysClass[ICLS_IME])) {
            while (pwndOwner != NULL) {
                if (pwndRoot == pwndOwner) {
                    fOwned = TRUE;
                    break;
                }
                pwndOwner = pwndOwner->spwndOwner;
            }
        }
        if (!fOwned)
            pwndRet = pwndT;

        /*
         * Next toplevel window.
         */
        pwndT = pwndT->spwndNext;
        UserAssert(pwndT->spwndParent == _GetDesktopWindow());
    }

    return pwndRet;
}


#if DBG
void ImeCheckSetTopmostLink(PWND pwnd, PWND pwndInsFirst, PWND pwndIns)
{
    PWND pwndDebT0 = pwndInsFirst;
    BOOL fFound = FALSE;

    if (pwndDebT0) {
        while (pwndDebT0 && (pwndDebT0 != pwndIns)) {
            if (pwndDebT0 == pwnd)
                fFound = TRUE;

            pwndDebT0 = pwndDebT0->spwndNext;
        }

        if (pwndDebT0 == NULL) {
            RIPMSG3(RIP_ERROR, "pwndIns(%#p) is upper that pwndInsFirst(%#p) pwnd is (%#p)", pwndIns, pwndInsFirst, pwnd);
        } else if (fFound) {
            RIPMSG3(RIP_ERROR, "pwnd(%#p) is between pwndInsFirst(%#p) and pwndIns(%#p)", pwnd, pwndInsFirst, pwndIns);
        }
    } else if (pwndIns) {
        pwndDebT0 = pwnd->spwndParent->spwndChild;

        while (pwndDebT0 && (pwndDebT0 != pwndIns)) {
            if (pwndDebT0 == pwnd)
                fFound = TRUE;

            pwndDebT0 = pwndDebT0->spwndNext;
        }

        if (fFound) {
            RIPMSG3(RIP_ERROR, "pwnd(%#p) is between TOPLEVEL pwndInsFirst(%#p) and pwndIns(%#p)", pwnd, pwndInsFirst, pwndIns);
        }
    }
}
#endif

/**************************************************************************\
* ImeSetTopmost
*
* History:
* 02-Apr-1996 wkwok       Ported from FE Win95 (imeclass.c)
\**************************************************************************/

VOID ImeSetTopmost(
    PWND pwndRootIme,
    BOOL fMakeTopmost,
    PWND pwndInsertBefore)
{
    PWND pwndParent = pwndRootIme->spwndParent;
    PWND pwndInsert = PWND_TOP; // pwnd which should be prior to pwndRootIme.
    PWND pwnd, pwndT;
    PWND pwndInsertFirst;
    BOOLEAN fFound;

    if (pwndParent == NULL)
        return;

    pwnd = pwndParent->spwndChild;

    if (!fMakeTopmost) {
        /*
         * Get the last topmost window. This should be after unlink pwndRootIme
         * because pwndRootIme may be the last topmost window.
         */
        pwndInsert = GetLastTopMostWindowNoIME(pwndRootIme);

        if (pwndInsertBefore) {

            fFound = FALSE;
            pwndT = pwndInsert;

            while (pwndT != NULL && pwndT->spwndNext != pwndInsertBefore) {
                if (pwndT == pwndRootIme)
                    fFound = TRUE;
                pwndT = pwndT->spwndNext;
            }

            if (pwndT == NULL || fFound)
                return;

            pwndInsert = pwndT;
        }

        if (TestWF(pwndRootIme->spwndOwner, WFBOTTOMMOST)) {
            pwndT = pwndInsert;

            while (pwndT != NULL && pwndT != pwndRootIme->spwndOwner) {
                if (!TestCF(pwndT, CFIME) &&
                        pwndT->pcls->atomClassName != gpsi->atomSysClass[ICLS_IME]) {
                    pwndInsert = pwndT;
                }
                pwndT = pwndT->spwndNext;
            }
        }
    }

    pwndInsertFirst = pwndInsert;

    /*
     * Enum the all toplevel windows and if the owner of the window is same as
     * the owner of pwndRootIme, the window should be changed the position of
     * window link.
     */
    while (pwnd != NULL) {
        /*
         * Get the next window before calling ImeSetTopmost.
         * Because the next window will be changed in LinkWindow.
         */
        PWND pwndNext = pwnd->spwndNext;

        /*
         * the owner relation between IME and UI window is in same thread.
         */
        if (GETPTI(pwnd) != GETPTI(pwndRootIme))
            goto ist_next;

        /*
         * pwnd have to be CS_IME class or "IME" class.
         */
        if (!TestCF(pwnd, CFIME) &&
                pwnd->pcls->atomClassName != gpsi->atomSysClass[ICLS_IME])
            goto ist_next;

        /*
         * If pwnd is pwndInsert, we don't need to do anything...
         */
        if (pwnd == pwndInsert)
            goto ist_next;

        pwndT = pwnd;
        while (pwndT != NULL) {
            if (pwndT == pwndRootIme) {
                /*
                 * Found!!
                 * pwnd is the ownee of pwndRootIme.
                 */

                UserAssert(GETPTI(pwnd) == GETPTI(pwndRootIme));
                UserAssert(TestCF(pwnd,CFIME) ||
                            (pwnd->pcls->atomClassName == gpsi->atomSysClass[ICLS_IME]));
                UserAssert(pwnd != pwndInsert);

                UnlinkWindow(pwnd, pwndParent);

                if (fMakeTopmost) {
                    if (pwndInsert != PWND_TOP)
                        UserAssert(TestWF(pwndInsert, WEFTOPMOST));
                    SetWF(pwnd, WEFTOPMOST);
                }
                else {
                    if (pwndInsert == PWND_TOP) {
                        /*
                         * In rare cases, the first toplevel window could be the one we'll look next,
                         * who may still have obscure topmost flag.
                         */
                        UserAssert(pwndParent->spwndChild == pwndNext || !TestWF(pwndParent->spwndChild, WEFTOPMOST));
                    }
                    else if (pwndInsert->spwndNext != NULL) {
                        /*
                         * In rare cases, pwndInsert->spwndNext could be the one we'll look next,
                         * who may still have obscure topmost flag.
                         */
                        UserAssert(pwndInsert->spwndNext == pwndNext || !TestWF(pwndInsert->spwndNext, WEFTOPMOST));
                    }
                    ClrWF(pwnd, WEFTOPMOST);
                }

                LinkWindow(pwnd, pwndInsert, pwndParent);
#if 0   // Let's see what happens if we disable this
                ImeSetTopmostChild(pwnd, fMakeTopmost);
#endif

                pwndInsert = pwnd;
                break;  // goto ist_next;
            }
            pwndT = pwndT->spwndOwner;
        }
ist_next:
        pwnd = pwndNext;

        /*
         * Skip the windows that were inserted before.
         */
        if (pwnd != NULL && pwnd == pwndInsertFirst)
            pwnd = pwndInsert->spwndNext;

#if DBG
        if (pwnd)
            ImeCheckSetTopmostLink(pwnd, pwndInsertFirst, pwndInsert);
#endif
    }
}


/**************************************************************************\
* ProbeAndCaptureSoftKbdData
*
* Captures SoftKbdData that comes from user mode.
*
* 23-Apr-1996 wkwok     created
\**************************************************************************/

PSOFTKBDDATA ProbeAndCaptureSoftKbdData(
    PSOFTKBDDATA Source)
{
    PSOFTKBDDATA Destination = NULL;
    DWORD        cbSize;
    UINT         uCount;

    try {
        uCount = ProbeAndReadUlong((PULONG)Source);

#if defined(_X86_)
        ProbeForReadBuffer(&Source->wCode, uCount, sizeof(BYTE));
#else
        ProbeForReadBuffer(&Source->wCode, uCount, sizeof(WORD));
#endif

        cbSize = FIELD_OFFSET(SOFTKBDDATA, wCode[0])
               + uCount * sizeof(WORD) * 256;

        Destination = (PSOFTKBDDATA)UserAllocPool(cbSize, TAG_IME);

        if (Destination != NULL) {
            RtlCopyMemory(Destination, Source, cbSize);
        } else {
            ExRaiseStatus(STATUS_NO_MEMORY);
        }

    } except (W32ExceptionHandler(TRUE, RIP_WARNING)) {

        if (Destination != NULL) {
            UserFreePool(Destination);
        }

        return NULL;
    }

    return Destination;
}

//
// ported from Win95:ctxtman.c\SetConvMode()
//
VOID  SetConvMode( PTHREADINFO pti, DWORD dwConversion )
{
    if ( pti->spklActive == NULL )
        return;

    switch ( PRIMARYLANGID(HandleToUlong(pti->spklActive->hkl)) ) {
    case LANG_JAPANESE:

        ClearKeyStateDown(pti->pq, VK_DBE_ALPHANUMERIC);
        ClearKeyStateToggle(pti->pq, VK_DBE_ALPHANUMERIC);
        ClearKeyStateDown(pti->pq, VK_DBE_KATAKANA);
        ClearKeyStateToggle(pti->pq, VK_DBE_KATAKANA);
        ClearKeyStateDown(pti->pq, VK_DBE_HIRAGANA);
        ClearKeyStateToggle(pti->pq, VK_DBE_HIRAGANA);

        if ( dwConversion & IME_CMODE_NATIVE ) {
            if ( dwConversion & IME_CMODE_KATAKANA ) {
                SetKeyStateDown( pti->pq, VK_DBE_KATAKANA);
                SetKeyStateToggle( pti->pq, VK_DBE_KATAKANA);
            } else {
                SetKeyStateDown( pti->pq, VK_DBE_HIRAGANA);
                SetKeyStateToggle( pti->pq, VK_DBE_HIRAGANA);
            }
        } else {
            SetKeyStateDown( pti->pq, VK_DBE_ALPHANUMERIC);
            SetKeyStateToggle( pti->pq, VK_DBE_ALPHANUMERIC);
        }

        if ( dwConversion & IME_CMODE_FULLSHAPE ) {
            SetKeyStateDown( pti->pq, VK_DBE_DBCSCHAR);
            SetKeyStateToggle( pti->pq, VK_DBE_DBCSCHAR);
            ClearKeyStateDown(pti->pq, VK_DBE_SBCSCHAR);
            ClearKeyStateToggle(pti->pq, VK_DBE_SBCSCHAR);
        } else {
            SetKeyStateDown( pti->pq, VK_DBE_SBCSCHAR);
            SetKeyStateToggle( pti->pq, VK_DBE_SBCSCHAR);
            ClearKeyStateDown(pti->pq, VK_DBE_DBCSCHAR);
            ClearKeyStateToggle(pti->pq, VK_DBE_DBCSCHAR);
        }

        if ( dwConversion & IME_CMODE_ROMAN ) {
            SetKeyStateDown( pti->pq, VK_DBE_ROMAN);
            SetKeyStateToggle( pti->pq, VK_DBE_ROMAN);
            ClearKeyStateDown(pti->pq, VK_DBE_NOROMAN);
            ClearKeyStateToggle(pti->pq, VK_DBE_NOROMAN);
        } else {
            SetKeyStateDown( pti->pq, VK_DBE_NOROMAN);
            SetKeyStateToggle( pti->pq, VK_DBE_NOROMAN);
            ClearKeyStateDown(pti->pq, VK_DBE_ROMAN);
            ClearKeyStateToggle(pti->pq, VK_DBE_ROMAN);
        }

        if ( dwConversion & IME_CMODE_CHARCODE ) {
            SetKeyStateDown( pti->pq, VK_DBE_CODEINPUT);
            SetKeyStateToggle( pti->pq, VK_DBE_CODEINPUT);
            ClearKeyStateDown(pti->pq, VK_DBE_NOCODEINPUT);
            ClearKeyStateToggle(pti->pq, VK_DBE_NOCODEINPUT);
        } else {
            SetKeyStateDown( pti->pq, VK_DBE_NOCODEINPUT);
            SetKeyStateToggle( pti->pq, VK_DBE_NOCODEINPUT);
            ClearKeyStateDown(pti->pq, VK_DBE_CODEINPUT);
            ClearKeyStateToggle(pti->pq, VK_DBE_CODEINPUT);
        }
        break;

    case LANG_KOREAN:
        if ( dwConversion & IME_CMODE_NATIVE) {
            SetKeyStateToggle( pti->pq, VK_HANGUL);
        } else {
            ClearKeyStateToggle( pti->pq, VK_HANGUL);
        }

        if ( dwConversion & IME_CMODE_FULLSHAPE ) {
            SetKeyStateToggle( pti->pq, VK_JUNJA);
        } else {
            ClearKeyStateToggle( pti->pq, VK_JUNJA);
        }

        if ( dwConversion & IME_CMODE_HANJACONVERT ) {
            SetKeyStateToggle( pti->pq, VK_HANJA);
        } else {
            ClearKeyStateToggle( pti->pq, VK_HANJA);
        }
        break;

    default:
        break;
    }
    return;
}

//
// called by IMM32 client when:
//
//      input focus is switched
//   or IME open status is changed
//   or IME conversion status is changed
//
//
VOID xxxNotifyIMEStatus(
                       IN PWND pwnd,
                       IN DWORD dwOpen,
                       IN DWORD dwConversion )
{
    PTHREADINFO pti;

    CheckLock(pwnd);

    if ( (pti = GETPTI(pwnd)) != NULL && gptiForeground != NULL ) {
        if ( pti == gptiForeground || pti->pq == gptiForeground->pq ) {

            if ( gHimcFocus != pwnd->hImc   ||
                 gdwIMEOpenStatus != dwOpen ||
                 gdwIMEConversionStatus != dwConversion ) {

                //
                // save the new status
                //
                gHimcFocus = pwnd->hImc;
                if ( gHimcFocus != (HIMC)NULL ) {

                    RIPMSG2(RIP_VERBOSE, "xxxNotifyIMEStatus: newOpen=%x newConv=%x",
                            dwOpen, dwConversion);
                    gdwIMEOpenStatus = dwOpen;
                    gdwIMEConversionStatus = dwConversion;

                    //
                    // set keyboard states that are related to IME conversion status
                    //
                    SetConvMode(pti, dwOpen ? dwConversion : 0);
                }

                //
                // notify shell the IME status change
                //
                // Implementation note: [takaok 9/5/96]
                //
                // Using HSHELL_LANGUAGE is not the best way to inform shell
                // IME status change because we didn't change the keyboard layout.
                // ( The spec says HSHELL_LANGUAGE is for keyboard layout change.
                //  Also passing window handle as WPARAM is not documented )
                //
                // This is same as what Win95 does. I won't change this for now
                // because in the future shell will be developed by a different
                // group in MS.
                //
                // Currently only Korean Windows is interested in getting
                // the conversion status change.
                //
                if (IsHooked(pti, WHF_SHELL)) {
                    HKL hkl = NULL;

                    if (pti->spklActive != NULL) {
                        hkl = pti->spklActive->hkl;
                    }
                    xxxCallHook(HSHELL_LANGUAGE, (WPARAM)HWq(pwnd), (LPARAM)hkl, WH_SHELL);
                }

                //
                // notify keyboard driver
                //
                NlsKbdSendIMENotification(dwOpen,dwConversion);
            }
        }
    }
}

//---------------------------------------------------------------------------
//
// xxxCheckImeShowStatus() -
//
// Only one Status Window should be shown in the System.
// This functsion enums all IME window and check the show status of them.
//
// If pti is NULL, check all toplevel windows regardless their owners.
// If pti is not NULL, check only windows belong to the thread.
//
//----------------------------------------------------------------------------

BOOL xxxCheckImeShowStatus(PWND pwndIme, PTHREADINFO pti)
{
    PBWL pbwl;
    PHWND phwnd;
    BOOL fRet = FALSE;
    PTHREADINFO ptiCurrent = PtiCurrent();

    if (TestWF(pwndIme, WFINDESTROY)) {
        return FALSE;
    }

    // Parent window of IME window should be the desktop window
    UserAssert(pwndIme);
    UserAssert(pwndIme->spwndParent == GETPTI(pwndIme)->pDeskInfo->spwnd);

    pbwl = BuildHwndList(pwndIme->spwndParent->spwndChild, BWL_ENUMLIST, NULL);
    if (pbwl != NULL) {
        fRet = TRUE;
        for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {
            PWND pwndT = RevalidateHwnd(*phwnd);

            // If pwndT is the current active IME window, we should skip it
            // since it's the only one window allowed to show status, and
            // we've already taken care of it.
            if (pwndT == NULL || (/*pwndIme && */pwndIme == pwndT)) {   // Can skip pwndIme != NULL test
                continue;
            }

            // We are going to touch IME windows only
            if (pwndT->pcls->atomClassName == gpsi->atomSysClass[ICLS_IME] &&
                    !TestWF(pwndT, WFINDESTROY)) {

                PIMEUI pimeui = ((PIMEWND)pwndT)->pimeui;

                if (pimeui == NULL || pimeui == (PIMEUI)-1) {
                    continue;
                }

                if (pti == NULL || pti == GETPTI(pwndT)) {
                    BOOLEAN fAttached = FALSE;
                    PWND    pwndIMC;

                    // If pwndT is not a window of the current process, we have to138163
                    // attach the process to get access to pimeui.
                    if (GETPTI(pwndT)->ppi != ptiCurrent->ppi) {
                        RIPMSG0(RIP_VERBOSE, "Attaching process in xxxCheckImeShowStatus");
                        KeAttachProcess(&GETPTI(pwndT)->ppi->Process->Pcb);
                        fAttached = TRUE;
                    }

                    try {
                        if (ProbeAndReadStructure(pimeui, IMEUI).fShowStatus) {
                            if (pti == NULL) {
                                RIPMSG0(RIP_VERBOSE, "xxxCheckImeShowStatus: the status window is shown !!");
                            }
                            if ((pwndIMC = RevalidateHwnd(pimeui->hwndIMC)) != NULL) {
                                pimeui->fShowStatus = FALSE;
                            }
                        } else {
                            pwndIMC = NULL;
                        }

                    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
                          pwndIMC = NULL;
                    }
                    if (fAttached) {
                        KeDetachProcess();
                    }

                    if (pwndIMC && GETPTI(pwndIMC) && !(GETPTI(pwndIMC)->TIF_flags & TIF_INCLEANUP)) {
                        TL tl;

                        ThreadLockAlways(pwndIMC, &tl);
                        RIPMSG1(RIP_VERBOSE, "Sending WM_IME_NOTIFY to %#p, IMN_CLOSESTATUSWINDOW", pwndIMC);
                        xxxSendMessage(pwndIMC, WM_IME_NOTIFY, IMN_CLOSESTATUSWINDOW, 0L);
                        ThreadUnlock(&tl);
                    }

                }
            }
        }
        FreeHwndList(pbwl);
    }
    return fRet;
}

LRESULT xxxSendMessageToUI(
    PTHREADINFO ptiIme,
    PIMEUI pimeui,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam)
{
    PWND  pwndUI;
    LRESULT lRet = 0L;
    TL    tl;
    BOOL  fAttached = FALSE;

    CheckCritIn();

    if (ptiIme != PtiCurrent()) {
        fAttached = TRUE;
        KeAttachProcess(&ptiIme->ppi->Process->Pcb);
    }

    try {
        pwndUI = RevalidateHwnd(ProbeAndReadStructure(pimeui, IMEUI).hwndUI);
    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
        pwndUI = NULL;
    }

    if (pwndUI != NULL){
        try {
            ProbeAndReadUlong((PULONG)&pimeui->nCntInIMEProc);
            InterlockedIncrement(&pimeui->nCntInIMEProc);   // Mark to avoid recursion.
        } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
              goto skip_it;
        }
        if (fAttached) {
            KeDetachProcess();
        }

        ThreadLockAlways(pwndUI, &tl);
        RIPMSG3(RIP_VERBOSE, "Sending message UI pwnd=%#p, msg:%x to wParam=%#p", pwndUI, message, wParam);
        lRet = xxxSendMessage(pwndUI, message, wParam, lParam);
        ThreadUnlock(&tl);

        if (fAttached) {
            KeAttachProcess(&ptiIme->ppi->Process->Pcb);
        }
        try {
            InterlockedDecrement(&pimeui->nCntInIMEProc);   // Mark to avoid recursion.
        } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
        }
    }
skip_it:
    if (fAttached) {
        KeDetachProcess();
    }

    return lRet;
}

VOID xxxSendOpenStatusNotify(
    PTHREADINFO ptiIme,
    PIMEUI pimeui,
    PWND   pwndApp,
    BOOL   fOpen)
{
    WPARAM wParam = fOpen ? IMN_OPENSTATUSWINDOW : IMN_CLOSESTATUSWINDOW;

    UserAssert(GETPTI(pwndApp));

    if (GETPTI(pwndApp)->dwExpWinVer >= VER40 && pwndApp->hImc != NULL) {
        TL tl;
        ThreadLockAlways(pwndApp, &tl);
        RIPMSG2(RIP_VERBOSE, "Sending %s to pwnd=%#p",
                fOpen ? "IMN_OPENSTATUSWINDOW" : "IMN_CLOSESTATUSWINDOW",
                pwndApp);
        xxxSendMessage(pwndApp, WM_IME_NOTIFY, wParam, 0L);
        ThreadUnlock(&tl);
    }
    else {
        xxxSendMessageToUI(ptiIme, pimeui, WM_IME_NOTIFY, wParam, 0L);
    }

    return;
}

VOID xxxNotifyImeShowStatus(PWND pwndIme)
{
    PIMEUI pimeui;
    BOOL fShow;
    PWND pwnd;
    PTHREADINFO ptiIme, ptiCurrent;
    BOOL fSendOpenStatusNotify = FALSE;

    if (!IS_IME_ENABLED() || TestWF(pwndIme, WFINDESTROY)) {
        RIPMSG0(RIP_WARNING, "IME is not enabled or in destroy.");
        return;
    }

    ptiCurrent = PtiCurrent();
    ptiIme = GETPTI(pwndIme);

    if (ptiIme != ptiCurrent) {
        RIPMSG1(RIP_VERBOSE, "Attaching pti=%#p", ptiIme);
        KeAttachProcess(&GETPTI(pwndIme)->ppi->Process->Pcb);
    }

    try {
        pimeui = ((PIMEWND)pwndIme)->pimeui;
        fShow = gfIMEShowStatus && ProbeAndReadStructure(pimeui, IMEUI).fCtrlShowStatus;

        pwnd = RevalidateHwnd(pimeui->hwndIMC);

        if (pwnd != NULL || (pwnd = GETPTI(pwndIme)->pq->spwndFocus) != NULL) {
            RIPMSG0(RIP_VERBOSE, "Setting new show status.");
            fSendOpenStatusNotify = TRUE;
            pimeui->fShowStatus = fShow;
        }
    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
          if (ptiIme != ptiCurrent) {
              KeDetachProcess();
          }
          return;
    }
    if (ptiIme != ptiCurrent) {
        KeDetachProcess();
    }

    if (fSendOpenStatusNotify) {
        RIPMSG1(RIP_VERBOSE, "Sending OpenStatus fShow=%d", fShow);
        xxxSendOpenStatusNotify(ptiIme, pimeui, pwnd, fShow);
    }

    // Check the show status of all IME windows in the system.
    if (!TestWF(pwndIme, WFINDESTROY)) {
        xxxCheckImeShowStatus(pwndIme, NULL);
    }
}


/***************************************************************************\
* xxxSetIMEShowStatus() -
*
* Set IME Status windows' show status. Called from SystemParametersInfo()
* handler.
*
\***************************************************************************/

BOOL xxxSetIMEShowStatus(IN BOOL fShow)
{
    CheckCritIn();

    UserAssert(fShow == FALSE || fShow == TRUE);

    if (gfIMEShowStatus == fShow) {
        return TRUE;
    }

    if (gfIMEShowStatus == IMESHOWSTATUS_NOTINITIALIZED) {
        /*
         * Called for the first time after logon.
         * No need to write the value to the registry.
         */
        gfIMEShowStatus = fShow;
    }
    else {
        /*
         * We need to save the new fShow status to the registry.
         */
        TL tlName;
        PUNICODE_STRING pProfileUserName;
        BOOL fOK = FALSE;

        pProfileUserName = CreateProfileUserName(&tlName);
        if (pProfileUserName) {
            UserAssert(pProfileUserName != NULL);
            fOK = UpdateWinIniInt(pProfileUserName, PMAP_INPUTMETHOD, STR_SHOWIMESTATUS, fShow);
            FreeProfileUserName(pProfileUserName, &tlName);
        }
        if (!fOK) {
            return FALSE;
        }
        gfIMEShowStatus = fShow;
    }

    /*
     * If IME is not enabled, further processing is not needed
     */
    if (!IS_IME_ENABLED()) {
        return TRUE;
    }

    /*
     * Let the current active IME window know the change.
     */
    if (gpqForeground && gpqForeground->spwndFocus) {
        PTHREADINFO ptiFocus = GETPTI(gpqForeground->spwndFocus);
        TL tl;

        UserAssert(ptiFocus);

        if (ptiFocus->spwndDefaultIme && !(ptiFocus->TIF_flags & TIF_INCLEANUP)) {
            ThreadLockAlways(ptiFocus->spwndDefaultIme, &tl);
            xxxNotifyImeShowStatus(ptiFocus->spwndDefaultIme);
            ThreadUnlock(&tl);
        }
    }

    return TRUE;
}

/***************************************************************************\
* xxxBroadcastImeShowStatusChange() -
*
* Let all IME windows in the desktop, including myself  know about the
* status change.
* This routine does not touch the registry, assuming internat.exe updated
* the registry.
*
\***************************************************************************/

VOID xxxBroadcastImeShowStatusChange(PWND pwndIme, BOOL fShow)
{
    CheckCritIn();

    gfIMEShowStatus = !!fShow;
    xxxNotifyImeShowStatus(pwndIme);
}

/***************************************************************************\
* xxxCheckImeShowStatusInThread() -
*
* Let all IME windows in the same thread know about the status change.
* Called from ImeSetContextHandler().
*
\***************************************************************************/
VOID xxxCheckImeShowStatusInThread(PWND pwndIme)
{
    if (IS_IME_ENABLED()) {
        UserAssert(pwndIme);

        if (!TestWF(pwndIme, WFINDESTROY)) {
            xxxCheckImeShowStatus(pwndIme, GETPTI(pwndIme));
        }
    }
}

BOOL _GetIMEShowStatus(VOID)
{
    return gfIMEShowStatus != FALSE;
}

