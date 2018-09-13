/**************************************************************************\
* Module Name: context.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Context management routines for imm32 dll
*
* History:
* 03-Jan-1996 wkwok       Created
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define IMCC_ALLOC_TOOLARGE             0x1000


/**************************************************************************\
* ImmCreateContext
*
* Creates and initializes an input context.
*
* 17-Jan-1996 wkwok       Created
\**************************************************************************/

HIMC WINAPI ImmCreateContext(void)
{
    PCLIENTIMC pClientImc;
    HIMC       hImc = NULL_HIMC;

    if (!IS_IME_ENABLED()) {
        return NULL_HIMC;
    }

    pClientImc = ImmLocalAlloc(HEAP_ZERO_MEMORY, sizeof(CLIENTIMC));

    if (pClientImc != NULL) {

        hImc = NtUserCreateInputContext((ULONG_PTR)pClientImc);
        if (hImc == NULL_HIMC) {
            ImmLocalFree(pClientImc);
            return NULL_HIMC;
        }

        InitImcCrit(pClientImc);
        pClientImc->dwImeCompatFlags = (DWORD)NtUserGetThreadState(UserThreadStateImeCompatFlags);
    }

    return hImc;
}


/**************************************************************************\
* ImmDestroyContext
*
* Destroys an input context.
*
* 17-Jan-1996 wkwok       Created
\**************************************************************************/

BOOL WINAPI ImmDestroyContext(
    HIMC hImc)
{
    if (!IS_IME_ENABLED()) {
        return FALSE;
    }

    if (GetInputContextThread(hImc) != GetCurrentThreadId()) {
        RIPMSG1(RIP_WARNING,
              "ImmDestroyContext: Invalid input context access %lx.", hImc);
        return FALSE;
    }

    return DestroyInputContext(hImc, GetKeyboardLayout(0), FALSE);
}


/**************************************************************************\
* ImmAssociateContext
*
* Associates an input context to the specified window handle.
*
* 17-Jan-1996 wkwok       Created
\**************************************************************************/

HIMC WINAPI ImmAssociateContext(
    HWND hWnd,
    HIMC hImc)
{
    PWND  pWnd;
    HIMC  hPrevImc;
    AIC_STATUS Status;

    // early out
    if (!IS_IME_ENABLED()) {
        return NULL_HIMC;
    }

    if ((pWnd = ValidateHwnd(hWnd)) == (PWND)NULL) {
        RIPMSG1(RIP_WARNING,
              "ImmAssociateContext: invalid window handle %x", hWnd);
        return NULL_HIMC;
    }



    if (hImc != NULL_HIMC &&
            GetInputContextThread(hImc) != GetCurrentThreadId()) {
        RIPMSG1(RIP_WARNING,
              "ImmAssociateContext: Invalid input context access %lx.", hImc);
        return NULL_HIMC;
    }

    /*
     * associate to the same input context, do nothing.
     */
    if (pWnd->hImc == hImc)
        return hImc;

    hPrevImc = pWnd->hImc;

    Status = NtUserAssociateInputContext(hWnd, hImc, 0);

    switch (Status) {
    case AIC_FOCUSCONTEXTCHANGED:
        if (IsWndEqual(NtUserQueryWindow(hWnd, WindowFocusWindow), hWnd)) {
            ImmSetActiveContext(hWnd, hPrevImc, FALSE);
            ImmSetActiveContext(hWnd, hImc, TRUE);
        }

        // Fall thru.

    case AIC_SUCCESS:
        return hPrevImc;

    default:
        return NULL_HIMC;
    }
}


BOOL WINAPI ImmAssociateContextEx(
    HWND hWnd,
    HIMC hImc,
    DWORD dwFlag)
{
    HWND hWndFocus;
    PWND pWndFocus;
    HIMC hImcFocusOld;
    AIC_STATUS Status;

    if (!IS_IME_ENABLED()) {
        return FALSE;
    }

    hWndFocus = NtUserQueryWindow(hWnd, WindowFocusWindow);

    if (hImc != NULL_HIMC && !(dwFlag & IACE_DEFAULT) &&
            GetInputContextThread(hImc) != GetCurrentThreadId()) {
        RIPMSG1(RIP_WARNING,
              "ImmAssociateContextEx: Invalid input context access %lx.", hImc);
        return FALSE;
    }

    if ((pWndFocus = ValidateHwnd(hWndFocus)) != (PWND)NULL)
        hImcFocusOld = pWndFocus->hImc;
    else
        hImcFocusOld = NULL_HIMC;

    Status = NtUserAssociateInputContext(hWnd, hImc, dwFlag);

    switch (Status) {
    case AIC_FOCUSCONTEXTCHANGED:
        if ((pWndFocus = ValidateHwnd(hWndFocus)) != (PWND)NULL) {
            hImc = pWndFocus->hImc;
            if (hImc != hImcFocusOld) {
                ImmSetActiveContext(hWndFocus, hImcFocusOld, FALSE);
                ImmSetActiveContext(hWndFocus, hImc, TRUE);
            };
        };

        // Fall thru.

    case AIC_SUCCESS:
        return TRUE;

    default:
        return FALSE;
    }
}


/**************************************************************************\
* ImmGetContext
*
* Retrieves the input context that is associated to the given window.
*
* 17-Jan-1996 wkwok       Created
\**************************************************************************/

HIMC WINAPI ImmGetContext(
    HWND hWnd)
{
    if ( hWnd == NULL ) {
        RIPMSG1(RIP_WARNING,
              "ImmGetContext: invalid window handle %x", hWnd);
        return NULL_HIMC;
    }
    /*
     * for non-NULL hWnd, ImmGetSaveContext will do the
     * validation and "same process" checking.
     */
    return ImmGetSaveContext( hWnd, IGSC_WINNLSCHECK );
}


/**************************************************************************\
* ImmGetSaveContext
*
* Retrieves the input context that is associated to the given window.
*
* 15-Mar-1996 wkwok       Created
\**************************************************************************/

HIMC ImmGetSaveContext(
    HWND  hWnd,
    DWORD dwFlag)
{
    HIMC       hRetImc;
    PCLIENTIMC pClientImc;
    PWND  pwnd;

    if (!IS_IME_ENABLED()) {
        return NULL_HIMC;
    }

    if (hWnd == NULL) {
        /*
         * Retrieves the default input context of current thread.
         */
        hRetImc = (HIMC)NtUserGetThreadState(UserThreadStateDefaultInputContext);
    }
    else {
        /*
         * Retrieves the input context associated to the given window.
         */
        if ((pwnd = ValidateHwnd(hWnd)) == (PWND)NULL) {
            RIPMSG1(RIP_WARNING,
                  "ImmGetSaveContext: invalid window handle %x", hWnd);
            return NULL_HIMC;
        }
        /*
         * Don't allow other process to access input context
         */
        if (!TestWindowProcess(pwnd)) {
            RIPMSG0(RIP_WARNING,
                  "ImmGetSaveContext: can not get input context of other process");
            return NULL_HIMC;
        }
        hRetImc = pwnd->hImc;

        if (hRetImc == NULL_HIMC && (dwFlag & IGSC_DEFIMCFALLBACK)) {
            /*
             * hWnd associated with NULL input context, retrieves the
             * default input context of the hWnd's creator thread.
             */
            hRetImc = (HIMC)NtUserQueryWindow(hWnd, WindowDefaultInputContext);
        }
    }

    pClientImc = ImmLockClientImc(hRetImc);
    if (pClientImc == NULL)
        return NULL_HIMC;

    if ((dwFlag & IGSC_WINNLSCHECK) && TestICF(pClientImc, IMCF_WINNLSDISABLE))
        hRetImc = NULL_HIMC;

    ImmUnlockClientImc(pClientImc);

    return hRetImc;
}


/**************************************************************************\
* ImmReleaseContext
*
* Releases the input context retrieved by ImmGetContext().
*
* 17-Jan-1996 wkwok       Created
\**************************************************************************/

BOOL WINAPI ImmReleaseContext(
    HWND hWnd,
    HIMC hImc)
{
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(hImc);

    return TRUE;
}


/**************************************************************************\
* ImmSetActiveContext
*
* 15-Mar-1996 wkwok       Created
\**************************************************************************/

BOOL ImmSetActiveContext(
    HWND hWnd,
    HIMC hImc,
    BOOL fActivate)
{
    PCLIENTIMC    pClientImc;
    PINPUTCONTEXT pInputContext;
    PIMEDPI       pImeDpi;
    DWORD         dwISC;
    HIMC          hSaveImc;
    HWND          hDefImeWnd;
    DWORD         dwOpenStatus = 0;
    DWORD         dwConversion = 0;
#ifdef DEBUG
    PWND          pWnd = ValidateHwnd(hWnd);

    if (pWnd != NULL && GETPTI(pWnd) != PtiCurrent()) {
        RIPMSG1(RIP_WARNING, "hWnd (=%lx) is not of current thread.", hWnd);
    }
#endif

    if (!IS_IME_ENABLED()) {
        return FALSE;
    }

    dwISC = ISC_SHOWUIALL;

    pClientImc = ImmLockClientImc(hImc);

    if (!fActivate) {
        if (pClientImc != NULL)
            ClrICF(pClientImc, IMCF_ACTIVE);
        goto NotifySetActive;
    }

    if (hImc == NULL_HIMC) {
        hSaveImc = ImmGetSaveContext(hWnd, IGSC_DEFIMCFALLBACK);
        pInputContext = ImmLockIMC(hSaveImc);
        if (pInputContext != NULL) {
            pInputContext->hWnd = hWnd;
            ImmUnlockIMC(hSaveImc);
        }
        goto NotifySetActive;
    }

    /*
     * Non-NULL input context, window handle have to be updated.
     */
    if (pClientImc == NULL)
        return FALSE;

    pInputContext = ImmLockIMC(hImc);
    if (pInputContext == NULL) {
        ImmUnlockClientImc(pClientImc);
        return FALSE;
    }

    pInputContext->hWnd = hWnd;
    SetICF(pClientImc, IMCF_ACTIVE);

#ifdef LATER
    // Do uNumLangVKey checking later
#endif

    if (pInputContext->fdw31Compat & F31COMPAT_MCWHIDDEN)
        dwISC = ISC_SHOWUIALL - ISC_SHOWUICOMPOSITIONWINDOW;

    dwOpenStatus = (DWORD)pInputContext->fOpen;
    dwConversion = pInputContext->fdwConversion;
    ImmUnlockIMC(hImc);

NotifySetActive:

    pImeDpi = ImmLockImeDpi(GetKeyboardLayout(0));
    if (pImeDpi != NULL) {
        (*pImeDpi->pfn.ImeSetActiveContext)(hImc, fActivate);
        ImmUnlockImeDpi(pImeDpi);
    }

    /*
     * Notify UI
     */
    if (IsWindow(hWnd)) {
        SendMessage(hWnd, WM_IME_SETCONTEXT, fActivate, dwISC);

        /*
         * send notify to shell / keyboard driver
         */
        if ( fActivate )
            NtUserNotifyIMEStatus( hWnd, dwOpenStatus, dwConversion );
    }
    else if (!fActivate) {
        /*
         * Because hWnd is not there (maybe destroyed), we send
         * WM_IME_SETCONTEXT to the default IME window.
         */
        if ((hDefImeWnd = ImmGetDefaultIMEWnd(NULL)) != NULL) {
            SendMessage(hDefImeWnd, WM_IME_SETCONTEXT, fActivate, dwISC);
        }
        else {
            RIPMSG0(RIP_WARNING,
                  "ImmSetActiveContext: can't send WM_IME_SETCONTEXT(FALSE).");
        }
    }
#ifdef DEBUG
    else {
        RIPMSG0(RIP_WARNING,
              "ImmSetActiveContext: can't send WM_IME_SETCONTEXT(TRUE).");
    }
#endif

#ifdef LATER
    // Implements ProcessIMCEvent() later.
#endif

    if (pClientImc != NULL)
        ImmUnlockClientImc(pClientImc);

    return TRUE;
}

/**************************************************************************\
* ModeSaver related routines
*
* Dec-1998 hiroyama     Created
\**************************************************************************/

PIMEMODESAVER GetImeModeSaver(
    PINPUTCONTEXT pInputContext,
    HKL hkl)
{
    PIMEMODESAVER pModeSaver;
    USHORT langId = PRIMARYLANGID(HKL_TO_LANGID(hkl));

    for (pModeSaver = pInputContext->pImeModeSaver; pModeSaver; pModeSaver = pModeSaver->next) {
        if (pModeSaver->langId == langId) {
            break;
        }
    }

    if (pModeSaver == NULL) {
        TAGMSG1(DBGTAG_IMM, "GetImeModeSaver: creating ModeSaver for langId=%04x", langId);
        pModeSaver = ImmLocalAlloc(HEAP_ZERO_MEMORY, sizeof *pModeSaver);
        if (pModeSaver == NULL) {
            RIPMSG1(RIP_WARNING, "GetImeModeSaver: failed to create ModeSaver for langId=%04x", langId);
            return NULL;
        }
        pModeSaver->langId = langId;
        pModeSaver->next = pInputContext->pImeModeSaver;
        pInputContext->pImeModeSaver = pModeSaver;
    }

    return pModeSaver;
}

VOID DestroyImeModeSaver(
    PINPUTCONTEXT pInputContext)
{
    PIMEMODESAVER pModeSaver = pInputContext->pImeModeSaver;

    //
    // Destroy mode savers
    //
    while (pModeSaver) {
        PIMEMODESAVER pNext = pModeSaver->next;
        PIMEPRIVATEMODESAVER pPrivateModeSaver = pModeSaver->pImePrivateModeSaver;

        //
        // Destroy private mode savers
        //
        while (pPrivateModeSaver) {
            PIMEPRIVATEMODESAVER pPrivateNext = pPrivateModeSaver->next;
            ImmLocalFree(pPrivateModeSaver);
            pPrivateModeSaver = pPrivateNext;
        }

        ImmLocalFree(pModeSaver);
        pModeSaver = pNext;
    }

    pInputContext->pImeModeSaver = NULL;
}

PIMEPRIVATEMODESAVER GetImePrivateModeSaver(
    PIMEMODESAVER pImeModeSaver,
    HKL hkl)
{
    PIMEPRIVATEMODESAVER pPrivateModeSaver;

    for (pPrivateModeSaver = pImeModeSaver->pImePrivateModeSaver; pPrivateModeSaver; pPrivateModeSaver = pPrivateModeSaver->next) {
        if (pPrivateModeSaver->hkl == hkl) {
            break;
        }
    }

    if (pPrivateModeSaver == NULL) {
        TAGMSG1(DBGTAG_IMM, "GetImePrivateModeSaver: creating private mode saver for hkl=%08x", hkl);
        pPrivateModeSaver = ImmLocalAlloc(0, sizeof *pPrivateModeSaver);
        if (pPrivateModeSaver == NULL) {
            RIPMSG1(RIP_WARNING, "GetImePrivateModeSaver: failed to create PrivateModeSaver for hlk=%08x", hkl);
            return NULL;
        }
        pPrivateModeSaver->hkl = hkl;
        pPrivateModeSaver->fdwSentence = 0;
        pPrivateModeSaver->next = pImeModeSaver->pImePrivateModeSaver;
        pImeModeSaver->pImePrivateModeSaver = pPrivateModeSaver;
    }

    return pPrivateModeSaver;
}

BOOL SavePrivateMode(
    PINPUTCONTEXT pInputContext,
    PIMEMODESAVER pImeModeSaver,
    HKL hkl)
{
    PIMEPRIVATEMODESAVER pPrivateModeSaver = GetImePrivateModeSaver(pImeModeSaver, hkl);

    if (pPrivateModeSaver == NULL) {
        return FALSE;
    }

    //
    // Save private sentence mode
    //
    pPrivateModeSaver->fdwSentence = pInputContext->fdwSentence & 0xffff0000;
    return TRUE;
}

BOOL RestorePrivateMode(
    PINPUTCONTEXT pInputContext,
    PIMEMODESAVER pImeModeSaver,
    HKL hkl)
{
    PIMEPRIVATEMODESAVER pPrivateModeSaver = GetImePrivateModeSaver(pImeModeSaver, hkl);

    if (pPrivateModeSaver == NULL) {
        return FALSE;
    }

    //
    // Restore private sentence mode
    //
    ImmAssert(LOWORD(pPrivateModeSaver->fdwSentence) == 0);
    pInputContext->fdwSentence |= pPrivateModeSaver->fdwSentence;
    return TRUE;
}

/**************************************************************************\
* CreateInputContext
*
* 20-Feb-1996 wkwok       Created
\**************************************************************************/

BOOL CreateInputContext(
    HIMC hImc,
    HKL  hKL,
    BOOL fCanCallImeSelect)
{
    PIMEDPI            pImeDpi;
    PCLIENTIMC         pClientImc;
    DWORD              dwPrivateDataSize;
    DWORD              fdwInitConvMode = 0;    // do it later
    BOOL               fInitOpen = FALSE;      // do it later
    PINPUTCONTEXT      pInputContext;
    PCOMPOSITIONSTRING pCompStr;
    PCANDIDATEINFO     pCandInfo;
    PGUIDELINE         pGuideLine;
    int                i;

    pInputContext = ImmLockIMC(hImc);
    if (!pInputContext) {
        RIPMSG1(RIP_WARNING, "CreateContext: Lock hIMC %x failure", hImc);
        goto CrIMCLockErrOut;
    }

    /*
     * Initialize the member of INPUTCONTEXT
     */
    pInputContext->hCompStr = ImmCreateIMCC(sizeof(COMPOSITIONSTRING));
    if (!pInputContext->hCompStr) {
        RIPMSG0(RIP_WARNING, "CreateContext: Create hCompStr failure");
        goto CrIMCUnlockIMC;
    }

    pCompStr = (PCOMPOSITIONSTRING)ImmLockIMCC(pInputContext->hCompStr);
    if (!pCompStr) {
        RIPMSG1(RIP_WARNING,
              "CreateContext: Lock hCompStr %x failure", pInputContext->hCompStr);
        goto CrIMCFreeCompStr;
    }

    pCompStr->dwSize = sizeof(COMPOSITIONSTRING);
    ImmUnlockIMCC(pInputContext->hCompStr);

    pInputContext->hCandInfo = ImmCreateIMCC(sizeof(CANDIDATEINFO));
    if (!pInputContext->hCandInfo) {
        RIPMSG0(RIP_WARNING, "CreateContext: Create hCandInfo failure");
        goto CrIMCFreeCompStr;
    }

    pCandInfo = (PCANDIDATEINFO)ImmLockIMCC(pInputContext->hCandInfo);
    if (!pCandInfo) {
        RIPMSG1(RIP_WARNING,
              "CreateContext: Lock hCandInfo %x failure", pInputContext->hCandInfo);
        goto CrIMCFreeCandInfo;
    }

    pCandInfo->dwSize = sizeof(CANDIDATEINFO);
    ImmUnlockIMCC(pInputContext->hCandInfo);

    pInputContext->hGuideLine = ImmCreateIMCC(sizeof(GUIDELINE));
    if (!pInputContext->hGuideLine) {
        RIPMSG0(RIP_WARNING, "CreateContext: Create hGuideLine failure");
        goto CrIMCFreeCandInfo;
    }

    pGuideLine = (PGUIDELINE)ImmLockIMCC(pInputContext->hGuideLine);
    if (!pGuideLine) {
        RIPMSG1(RIP_WARNING,
              "CreateContext: Lock hGuideLine %x failure", pInputContext->hGuideLine);
        goto CrIMCFreeGuideLine;
    }

    pGuideLine->dwSize = sizeof(GUIDELINE);
    ImmUnlockIMCC(pInputContext->hGuideLine);

    pInputContext->hMsgBuf = ImmCreateIMCC(sizeof(UINT));
    if (!pInputContext->hMsgBuf) {
        RIPMSG0(RIP_WARNING, "CreateContext: Create hMsgBuf failure");
        goto CrIMCFreeGuideLine;
    }

    pInputContext->dwNumMsgBuf = 0;
    pInputContext->fOpen = fInitOpen;
    pInputContext->fdwConversion = fdwInitConvMode;
    pInputContext->fdwSentence = 0;

    for (i = 0; i < 4; i++) {
        pInputContext->cfCandForm[i].dwIndex = (DWORD)(-1);
    }

    pImeDpi = ImmLockImeDpi(hKL);
    if (pImeDpi != NULL) {
        if ((pClientImc = ImmLockClientImc(hImc)) == NULL) {
            RIPMSG0(RIP_WARNING, "CreateContext: ImmLockClientImc() failure");
            ImmUnlockImeDpi(pImeDpi);
            goto CrIMCFreeMsgBuf;
        }

        /*
         * Unicode based IME expects an Uncode based input context.
         */
        if (pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE)
            SetICF(pClientImc, IMCF_UNICODE);

        pClientImc->dwCodePage = IMECodePage(pImeDpi);

        ImmUnlockClientImc(pClientImc);

        dwPrivateDataSize = pImeDpi->ImeInfo.dwPrivateDataSize;
    }
    else {
        dwPrivateDataSize = sizeof(UINT);
    }

    pInputContext->hPrivate = ImmCreateIMCC(dwPrivateDataSize);
    if (!pInputContext->hPrivate) {
        RIPMSG0(RIP_WARNING, "CreateContext: Create hPrivate failure");
        ImmUnlockImeDpi(pImeDpi);
        goto CrIMCFreeMsgBuf;
    }

    pInputContext->pImeModeSaver = NULL;

    if (pImeDpi != NULL) {
        if (fCanCallImeSelect) {
            (*pImeDpi->pfn.ImeSelect)(hImc, TRUE);
        }
        ImmUnlockImeDpi(pImeDpi);
    }

    ImmUnlockIMC(hImc);
    return TRUE;

    /*
     * context failure case
     */
CrIMCFreeMsgBuf:
    ImmDestroyIMCC(pInputContext->hMsgBuf);
CrIMCFreeGuideLine:
    ImmDestroyIMCC(pInputContext->hGuideLine);
CrIMCFreeCandInfo:
    ImmDestroyIMCC(pInputContext->hCandInfo);
CrIMCFreeCompStr:
    ImmDestroyIMCC(pInputContext->hCompStr);
CrIMCUnlockIMC:
    ImmUnlockIMC(hImc);
CrIMCLockErrOut:
    return FALSE;
}


/**************************************************************************\
* DestroyInputContext
*
* 20-Feb-1996 wkwok       Created
\**************************************************************************/

BOOL DestroyInputContext(
    HIMC      hImc,
    HKL       hKL,
    BOOL      bTerminate)
{
    PINPUTCONTEXT pInputContext;
    PIMEDPI       pImeDpi;
    PIMC          pImc;
    PCLIENTIMC    pClientImc;

    if (!IS_IME_ENABLED()) {
        return FALSE;

    }
    if (hImc == NULL_HIMC) {
        RIPMSG0(RIP_VERBOSE, "DestroyInputContext: hImc is NULL.");
        return FALSE;
    }

    pImc = HMValidateHandle((HANDLE)hImc, TYPE_INPUTCONTEXT);

    /*
     * Cannot destroy input context from other thread.
     */
    if (pImc == NULL || GETPTI(pImc) != PtiCurrent())
        return FALSE;

    /*
     * We are destroying this hImc so we don't bother calling
     * ImmLockClientImc() to get the pClientImc. Instead, we
     * reference the pImc->dwClientImcData directly and call
     * InterlockedIncrement(&pClientImc->cLockObj) right after
     * several quick checks.
     */
    pClientImc = (PCLIENTIMC)pImc->dwClientImcData;

    if (pClientImc == NULL) {
        /*
         * Client side Imc has not been initialzed yet.
         * We simply destroy this input context from kernel.
         */
        if (bTerminate) {
            /*
             * If called from THREAD_DETACH, we don't
             * have to destroy kernel side Input Context.
             */
            return TRUE;
        }
        return NtUserDestroyInputContext(hImc);
    }

    if (TestICF(pClientImc, IMCF_DEFAULTIMC) && !bTerminate) {
        /*
         * Cannot destroy default input context unless the
         * thread is terminating.
         */
        return FALSE;
    }

    if (TestICF(pClientImc, IMCF_INDESTROY)) {
        /*
         * This hImc is being destroyed. Returns as success.
         */
        return TRUE;
    }

    /*
     * Time to lock up the pClientImc.
     */
    InterlockedIncrement(&pClientImc->cLockObj);

    if (pClientImc->hInputContext != NULL) {

        pInputContext = ImmLockIMC(hImc);
        if (!pInputContext) {
            RIPMSG1(RIP_WARNING, "DestroyContext: Lock hImc %x failure", hImc);
            ImmUnlockClientImc(pClientImc);
            return FALSE;
        }

        pImeDpi = ImmLockImeDpi(hKL);
        if (pImeDpi != NULL) {
            (*pImeDpi->pfn.ImeSelect)(hImc, FALSE);
            ImmUnlockImeDpi(pImeDpi);
        }

        ImmDestroyIMCC(pInputContext->hPrivate);
        ImmDestroyIMCC(pInputContext->hMsgBuf);
        ImmDestroyIMCC(pInputContext->hGuideLine);
        ImmDestroyIMCC(pInputContext->hCandInfo);
        ImmDestroyIMCC(pInputContext->hCompStr);

        /*
         * Free all ImeModeSaver.
         */
        DestroyImeModeSaver(pInputContext);

        ImmUnlockIMC(hImc);
    }

    SetICF(pClientImc, IMCF_INDESTROY);

    /*
     * ImmUnlockClientImc() will free up the pClientImc
     * when InterlockedDecrement(&pClientImc->cLockObj)
     * reaches 0.
     */
    ImmUnlockClientImc(pClientImc);

    return (bTerminate) ? TRUE : NtUserDestroyInputContext(hImc);
}


/**************************************************************************\
* SelectInputContext
*
* 20-Feb-1996 wkwok       Created
\**************************************************************************/

VOID SelectInputContext(
    HKL  hSelKL,
    HKL  hUnSelKL,
    HIMC hImc)
{
    PIMEDPI            pSelImeDpi, pUnSelImeDpi;
    PCLIENTIMC         pClientImc;
    PINPUTCONTEXT      pInputContext;
    DWORD              dwSelPriv = 0, dwUnSelPriv = 0, dwSize;
    HIMCC              hImcc;
    PCOMPOSITIONSTRING pCompStr;
    PCANDIDATEINFO     pCandInfo;
    PGUIDELINE         pGuideLine;
    BOOLEAN            fLogFontInited;

    TAGMSG3(DBGTAG_IMM, "SelectInputContext: called for sel=%08p unsel=%08p hImc=%08p",
            hSelKL, hUnSelKL, hImc);

    pClientImc = ImmLockClientImc(hImc);
    if (pClientImc == NULL) {
        RIPMSG0(RIP_VERBOSE, "SelectInputContext: cannot lock client Imc. Bailing out.");
        return;
    }

    pSelImeDpi   = ImmLockImeDpi(hSelKL);

    if (hSelKL != hUnSelKL) {
        /*
         * If those new sel and unsel do no match but
         * somehow SelectInput is called, that means
         * we should initialize the input contex again
         * without dumping the old information.
         */
        pUnSelImeDpi = ImmLockImeDpi(hUnSelKL);
    } else {
        pUnSelImeDpi = NULL;
    }

    if (pSelImeDpi != NULL) {
        /*
         * According to private memory size of the two layout, we decide
         * whether we nee to reallocate this memory block
         */
        dwSelPriv = pSelImeDpi->ImeInfo.dwPrivateDataSize;

        /*
         * Setup the code page of the newly selected IME.
         */
        pClientImc->dwCodePage = IMECodePage(pSelImeDpi);
    }
    else {
        pClientImc->dwCodePage = CP_ACP;
    }

    if (pUnSelImeDpi != NULL)
        dwUnSelPriv = pUnSelImeDpi->ImeInfo.dwPrivateDataSize;

    dwSelPriv   = max(dwSelPriv,   sizeof(UINT));
    dwUnSelPriv = max(dwUnSelPriv, sizeof(UINT));

    /*
     * Unselect the input context.
     */
    if (pUnSelImeDpi != NULL)
        (*pUnSelImeDpi->pfn.ImeSelect)(hImc, FALSE);

    /*
     * Reinitialize the client side input context for the selected layout.
     */
    if ((pInputContext = InternalImmLockIMC(hImc, FALSE)) != NULL) {
        DWORD fdwOldConversion = pInputContext->fdwConversion;
        DWORD fdwOldSentence = pInputContext->fdwSentence;
        BOOL fOldOpen = pInputContext->fOpen;
        PIMEMODESAVER pUnSelModeSaver, pSelModeSaver;
        const DWORD fdwConvPreserve = IME_CMODE_EUDC;

        fLogFontInited = ((pInputContext->fdwInit & INIT_LOGFONT) == INIT_LOGFONT);

        if (TestICF(pClientImc, IMCF_UNICODE) && pSelImeDpi != NULL &&
                !(pSelImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE)) {
            /*
             * Check if there is any LOGFONT to be converted.
             */
            if (fLogFontInited) {
                LOGFONTA LogFontA;

                LFontWtoLFontA(&pInputContext->lfFont.W, &LogFontA);
                RtlCopyMemory(&pInputContext->lfFont.A, &LogFontA, sizeof(LOGFONTA));
            }

            ClrICF(pClientImc, IMCF_UNICODE);
        }
        else if (!TestICF(pClientImc, IMCF_UNICODE) && pSelImeDpi != NULL &&
                 (pSelImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE)) {
            /*
             * Check if there is any LOGFONT to be converted.
             */
            if (fLogFontInited) {
                LOGFONTW LogFontW;

                LFontAtoLFontW(&pInputContext->lfFont.A, &LogFontW);
                RtlCopyMemory(&pInputContext->lfFont.W, &LogFontW, sizeof(LOGFONTW));
            }

            SetICF(pClientImc, IMCF_UNICODE);
        }

        /*
         * hPrivate
         */
        if (dwUnSelPriv != dwSelPriv) {
            hImcc = ImmReSizeIMCC(pInputContext->hPrivate, dwSelPriv);
            if (hImcc) {
                pInputContext->hPrivate = hImcc;
            }
            else {
                RIPMSG1(RIP_WARNING,
                      "SelectContext: resize hPrivate %lX failure",
                      pInputContext->hPrivate);
                ImmDestroyIMCC(pInputContext->hPrivate);
                pInputContext->hPrivate = ImmCreateIMCC(dwSelPriv);
            }
        }

        /*
         * hMsgBuf
         */
        dwSize = ImmGetIMCCSize(pInputContext->hMsgBuf);

        if (ImmGetIMCCLockCount(pInputContext->hMsgBuf) != 0 ||
                dwSize > IMCC_ALLOC_TOOLARGE) {

            RIPMSG0(RIP_WARNING, "SelectContext: create new hMsgBuf");
            ImmDestroyIMCC(pInputContext->hMsgBuf);
            pInputContext->hMsgBuf = ImmCreateIMCC(sizeof(UINT));
            pInputContext->dwNumMsgBuf = 0;
        }

        /*
         * hGuideLine
         */
        dwSize = ImmGetIMCCSize(pInputContext->hGuideLine);

        if (ImmGetIMCCLockCount(pInputContext->hGuideLine) != 0 ||
                dwSize < sizeof(GUIDELINE) || dwSize > IMCC_ALLOC_TOOLARGE) {

            RIPMSG0(RIP_WARNING, "SelectContext: create new hGuideLine");
            ImmDestroyIMCC(pInputContext->hGuideLine);
            pInputContext->hGuideLine = ImmCreateIMCC(sizeof(GUIDELINE));
            pGuideLine = (PGUIDELINE)ImmLockIMCC(pInputContext->hGuideLine);

            if (pGuideLine != NULL) {
                pGuideLine->dwSize = sizeof(GUIDELINE);
                ImmUnlockIMCC(pInputContext->hGuideLine);
            }
        }

        /*
         * hCandInfo
         */
        dwSize = ImmGetIMCCSize(pInputContext->hCandInfo);

        if (ImmGetIMCCLockCount(pInputContext->hCandInfo) != 0 ||
                dwSize < sizeof(CANDIDATEINFO) || dwSize > IMCC_ALLOC_TOOLARGE) {

            RIPMSG0(RIP_WARNING, "SelectContext: create new hCandInfo");
            ImmDestroyIMCC(pInputContext->hCandInfo);
            pInputContext->hCandInfo = ImmCreateIMCC(sizeof(CANDIDATEINFO));
            pCandInfo = (PCANDIDATEINFO)ImmLockIMCC(pInputContext->hCandInfo);

            if (pCandInfo != NULL) {
                pCandInfo->dwSize = sizeof(CANDIDATEINFO);
                ImmUnlockIMCC(pInputContext->hCandInfo);
            }
        }

        /*
         * hCompStr
         */
        dwSize = ImmGetIMCCSize(pInputContext->hCompStr);

        if (ImmGetIMCCLockCount(pInputContext->hCompStr) != 0 ||
                dwSize < sizeof(COMPOSITIONSTRING) || dwSize > IMCC_ALLOC_TOOLARGE) {

            RIPMSG0(RIP_WARNING, "SelectContext: create new hCompStr");
            ImmDestroyIMCC(pInputContext->hCompStr);
            pInputContext->hCompStr = ImmCreateIMCC(sizeof(COMPOSITIONSTRING));
            pCompStr = (PCOMPOSITIONSTRING)ImmLockIMCC(pInputContext->hCompStr);

            if (pCompStr != NULL) {
                pCompStr->dwSize = sizeof(COMPOSITIONSTRING);
                ImmUnlockIMCC(pInputContext->hCompStr);
            }
        }

        //
        // Save and restore the IME modes when the primary
        // language changes.
        //

        if (pUnSelImeDpi) {
            //
            // If UnSelKL is IME, get ModeSaver per language.
            //
            pUnSelModeSaver = GetImeModeSaver(pInputContext, hUnSelKL);
            TAGMSG1(DBGTAG_IMM, "pUnSelModeSaver=%p", pUnSelModeSaver);

            //
            // Firstly save the private sentence mode per IME.
            //
            SavePrivateMode(pInputContext, pUnSelModeSaver, hUnSelKL);
        }
        else {
            pUnSelModeSaver = NULL;
        }

        if (pSelImeDpi) {
            //
            // If SelKL is IME, get is ModeSaver per language.
            //
            pSelModeSaver = GetImeModeSaver(pInputContext, hSelKL);
            TAGMSG1(DBGTAG_IMM, "pSelImeDpi. pImeModeSaver=%p", pSelModeSaver);
        }
        else {
            pSelModeSaver = NULL;
        }

        //
        // If the primary language of KL changes, save the current mode
        // and restore the previous modes of new language.
        //
        if (pUnSelModeSaver != pSelModeSaver) {
            //
            // If old KL is IME, save the current conversion, sentence and open mode.
            //
            if (pUnSelModeSaver) {
                pUnSelModeSaver->fOpen = (pInputContext->fOpen != FALSE);

                //
                // Don't have to save the preserved bits for conversion mode.
                //
                pUnSelModeSaver->fdwConversion = pInputContext->fdwConversion & ~fdwConvPreserve;

                pUnSelModeSaver->fdwSentence = LOWORD(pInputContext->fdwSentence);
                pUnSelModeSaver->fdwInit = pInputContext->fdwInit;
            }

            //
            // If new KL is IME, restore the previous conversion, sentence and open mode.
            //
            if (pSelModeSaver) {
                if (pInputContext->fdwDirty & IMSS_INIT_OPEN) {
                    //
                    // HKL change may be kicked from private IME hotkey, and
                    // a user wants it opened when switched.
                    //
                    pInputContext->fOpen = TRUE;
                    pInputContext->fdwDirty &= ~IMSS_INIT_OPEN;
                } else {
                    pInputContext->fOpen = pSelModeSaver->fOpen;
                }

                //
                // Some bits are preserved across the languages.
                //
                pInputContext->fdwConversion &= fdwConvPreserve;
                ImmAssert((pSelModeSaver->fdwConversion & fdwConvPreserve) == 0);
                pInputContext->fdwConversion |= pSelModeSaver->fdwConversion & ~fdwConvPreserve;

                ImmAssert(HIWORD(pSelModeSaver->fdwSentence) == 0);
                pInputContext->fdwSentence = pSelModeSaver->fdwSentence;
                pInputContext->fdwInit = pSelModeSaver->fdwInit;
            }
        }
        if (pSelModeSaver) {
            //
            // Restore the private sentence mode per IME.
            //
            RestorePrivateMode(pInputContext, pSelModeSaver, hSelKL);
        }

        /*
         * Select the input context.
         */
        if (pSelImeDpi != NULL)
            (*pSelImeDpi->pfn.ImeSelect)(hImc, TRUE);

        //
        // Set the dirty bits so that IMM can send notifications later.
        // See SendNotificatonProc.
        //
        pInputContext->fdwDirty = 0;
        if (pInputContext->fOpen != fOldOpen) {
            pInputContext->fdwDirty |= IMSS_UPDATE_OPEN;
        }
        if (pInputContext->fdwConversion != fdwOldConversion) {
            pInputContext->fdwDirty |= IMSS_UPDATE_CONVERSION;
        }
        if (pInputContext->fdwSentence != fdwOldSentence) {
            pInputContext->fdwDirty |= IMSS_UPDATE_SENTENCE;
        }
        TAGMSG4(DBGTAG_IMM, "fOpen:%d fdwConv:%08x fdwSent:%08x dirty:%02x",
                pInputContext->fOpen, pInputContext->fdwConversion, pInputContext->fdwSentence, pInputContext->fdwDirty);

        ImmUnlockIMC(hImc);
    }
    else {
        //
        // To keep the backward compatibility,
        // select the input context here.
        //
        if (pSelImeDpi != NULL)
            (*pSelImeDpi->pfn.ImeSelect)(hImc, TRUE);
    }

    ImmUnlockImeDpi(pUnSelImeDpi);
    ImmUnlockImeDpi(pSelImeDpi);
    ImmUnlockClientImc(pClientImc);
}

BOOL SendNotificationProc(
    HIMC hImc,
    LPARAM lParam)
{
    PINPUTCONTEXT pInputContext = ImmLockIMC(hImc);

    UNREFERENCED_PARAMETER(lParam);

    if (pInputContext != NULL) {
        HWND hwnd = pInputContext->hWnd;

        if (IsWindow(hwnd)) {
            TAGMSG2(DBGTAG_IMM, "SendNotificationProc: updating hImc=%08x dirty=%04x",
                    hImc, pInputContext->fdwDirty);

            if (pInputContext->fdwDirty & IMSS_UPDATE_OPEN) {
                SendMessageW(hwnd, WM_IME_NOTIFY, IMN_SETOPENSTATUS, 0);
            }
            if (pInputContext->fdwDirty & IMSS_UPDATE_CONVERSION) {
                SendMessageW(hwnd, WM_IME_NOTIFY, IMN_SETCONVERSIONMODE, 0);
            }
            if (pInputContext->fdwDirty & (IMSS_UPDATE_OPEN | IMSS_UPDATE_CONVERSION)) {
                NtUserNotifyIMEStatus(hwnd, pInputContext->fOpen, pInputContext->fdwConversion);
            }
            if (pInputContext->fdwDirty & IMSS_UPDATE_SENTENCE) {
                SendMessageW(hwnd, WM_IME_NOTIFY, IMN_SETSENTENCEMODE, 0);
            }
        }
        pInputContext->fdwDirty = 0;
    }

    return TRUE;
}

VOID ImmSendNotification(
    BOOL fForProcess)
{
    DWORD dwThreadId;

    if (fForProcess) {
        dwThreadId = -1;
    } else {
        dwThreadId = 0;
    }

    ImmEnumInputContext(dwThreadId, (IMCENUMPROC)SendNotificationProc, 0);
}

/**************************************************************************\
* ImmEnumInputContext
*
* 20-Feb-1996 wkwok       Created
\**************************************************************************/

BOOL WINAPI ImmEnumInputContext(
    DWORD idThread,
    IMCENUMPROC lpfn,
    LPARAM lParam)
{
    UINT i;
    UINT cHimc;
    HIMC *phimcT;
    HIMC *phimcFirst;
    BOOL fSuccess = TRUE;

    /*
     * Get the himc list.  It is returned in a block of memory
     * allocated with ImmLocalAlloc.
     */
    if ((cHimc = BuildHimcList(idThread, &phimcFirst)) == 0) {
        return FALSE;
    }

    /*
     * Loop through the input contexts, call the function pointer back for
     * each one. End loop if either FALSE is returned or the end-of-list is
     * reached.
     */
    phimcT = phimcFirst;
    for (i = 0; i < cHimc; i++) {
        if (RevalidateHimc(*phimcT)) {
            if (!(fSuccess = (*lpfn)(*phimcT, lParam)))
                break;
        }
        phimcT++;
    }

    /*
     * Free up buffer and return status - TRUE if entire list was enumerated,
     * FALSE otherwise.
     */
    ImmLocalFree(phimcFirst);

    return fSuccess;
}

/**************************************************************************\
* BuildHimcList
*
* 20-Feb-1996 wkwok       Created
\**************************************************************************/

DWORD BuildHimcList(
    DWORD idThread,
    HIMC **pphimcFirst)
{
    UINT cHimc;
    HIMC *phimcFirst;
    NTSTATUS Status;
    int cTries;

    /*
     * Allocate a buffer to hold the names.
     */
    cHimc = 64;
    phimcFirst = ImmLocalAlloc(0, cHimc * sizeof(HIMC));
    if (phimcFirst == NULL)
        return 0;

    Status = NtUserBuildHimcList(idThread, cHimc, phimcFirst, &cHimc);

    /*
     * If the buffer wasn't big enough, reallocate
     * the buffer and try again.
     */
    cTries = 0;
    while (Status == STATUS_BUFFER_TOO_SMALL) {
        ImmLocalFree(phimcFirst);

        /*
         * If we can't seem to get it right,
         * call it quits
         */
        if (cTries++ == 10)
            return 0;

        phimcFirst = ImmLocalAlloc(0, cHimc * sizeof(HIMC));
        if (phimcFirst == NULL)
            return 0;

        Status = NtUserBuildHimcList(idThread, cHimc, phimcFirst, &cHimc);
    }

    if (!NT_SUCCESS(Status) || cHimc == 0) {
        ImmLocalFree(phimcFirst);
        return 0;
    }

    *pphimcFirst = phimcFirst;

    return cHimc;
}
