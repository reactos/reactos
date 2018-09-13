/**************************************************************************\
* Module Name: misc.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
*
* History:
* 03-Jan-1996 wkwok       Created
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#ifdef HIRO_DEBUG
#define D(x)    x
#else
#define D(x)
#endif


/**************************************************************************\
* ImmGetDefaultIMEWnd
*
* 03-Jan-1996 wkwok       Created
\**************************************************************************/

HWND WINAPI ImmGetDefaultIMEWnd(
    HWND hWnd)
{
    if (!IS_IME_ENABLED()) {
        return NULL;
    }
    if (hWnd == NULL) {
        /*
         * Query default IME window of current thread.
         */
        return (HWND)NtUserGetThreadState(UserThreadStateDefaultImeWindow);
    }

    return (HWND)NtUserQueryWindow(hWnd, WindowDefaultImeWindow);
}


/**************************************************************************\
* ImmDisableIME
*
* 13-Sep-1996 wkwok       Created
\**************************************************************************/

BOOL WINAPI ImmDisableIME(DWORD dwThreadId)
{
#ifdef LATER    // hiro
    if (dwThreadId == -1) {
        // Unload all IMEs
        RtlEnterCriticalSection(&gcsImeDpi);
        while (gpImeDpi) {
            PIMEDPI pImeDpi = gpImeDpi;
            gpImeDpi = gpImeDpi->pNext;
            UnloadIME(pImeDpi, TRUE);
            ImmLocalFree(pImeDpi);
        }
        RtlLeaveCriticalSection(&gcsImeDpi);
    }
#endif
    return (BOOL)NtUserDisableThreadIme(dwThreadId);
}

/**************************************************************************\
* ImmIsUIMessageA
*
* Filter messages needed for IME window.
*
* 03-Jan-1996 wkwok       Created
\**************************************************************************/

BOOL WINAPI ImmIsUIMessageA(
    HWND   hIMEWnd,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam)
{
    return ImmIsUIMessageWorker(hIMEWnd, message, wParam, lParam, TRUE);
}


/**************************************************************************\
* ImmIsUIMessageW
*
* Filter messages needed for IME window.
*
* 29-Feb-1996 wkwok       Created
\**************************************************************************/

BOOL WINAPI ImmIsUIMessageW(
    HWND   hIMEWnd,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam)
{
    return ImmIsUIMessageWorker(hIMEWnd, message, wParam, lParam, FALSE);
}


/**************************************************************************\
* ImmIsUIMessageWorker
*
* Worker function of ImmIsUIMessageA/ImmIsUIMessageW.
*
* Return: True if message is processed by IME UI.
*         False otherwise.
*
* 29-Feb-1996 wkwok       Created
\**************************************************************************/

BOOL ImmIsUIMessageWorker(
    HWND   hIMEWnd,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam,
    BOOL   fAnsi)
{
    D(DbgPrint("ImmIsUIMessageWorker(wnd[%08X], msg[%04X], wp[%08X], lp[%08X], Ansi[%d]\n",
      hIMEWnd, message, wParam, lParam, fAnsi));

    switch (message) {
    case WM_IME_STARTCOMPOSITION:
    case WM_IME_ENDCOMPOSITION:
    case WM_IME_COMPOSITION:
    case WM_IME_SETCONTEXT:
    case WM_IME_COMPOSITIONFULL:
    case WM_IME_SELECT:
    case WM_IME_NOTIFY:
    case WM_IME_SYSTEM:

        if (!hIMEWnd)
            return TRUE;

#if DBG
        if (!IsWindow(hIMEWnd)) {
            RIPMSG1(RIP_WARNING,
                  "ImmIsUIMessage: Invalid window handle %x", hIMEWnd);
            return FALSE;
        }
#endif

        if (fAnsi) {
            SendMessageA(hIMEWnd, message, wParam, lParam);
        }
        else {
            SendMessageW(hIMEWnd, message, wParam, lParam);
        }

        return TRUE;

    default:
        break;
    }

    return FALSE;
}


/**************************************************************************\
* ImmGenerateMessage
*
* Sends message(s) that are stored in hMsgBuf of hImc to hWnd of hImc.
*
* 29-Feb-1996 wkwok       Created
\**************************************************************************/

BOOL WINAPI ImmGenerateMessage(
    HIMC hImc)
{
    PCLIENTIMC    pClientImc;
    PINPUTCONTEXT pInputContext;
    PTRANSMSG     pTransMsg;
    INT           iNum;
    INT           i;
    BOOL          fUnicodeImc;

    if (GetInputContextThread(hImc) != GetCurrentThreadId()) {
        RIPMSG1(RIP_WARNING,
              "ImmGenerateMessage: Invalid input context access %lx.", hImc);
        return FALSE;
    }

    pClientImc = ImmLockClientImc(hImc);
    if (pClientImc == NULL)
        return FALSE;

    fUnicodeImc = TestICF(pClientImc, IMCF_UNICODE);

    ImmUnlockClientImc(pClientImc);

    pInputContext = ImmLockIMC(hImc);
    if (!pInputContext) {
        RIPMSG1(RIP_WARNING, "ImmGenerateMessage: Lock hImc %lx failed.", hImc);
        return FALSE;
    }

    iNum = (int)pInputContext->dwNumMsgBuf;

    if (iNum && (pTransMsg = (PTRANSMSG)ImmLockIMCC(pInputContext->hMsgBuf))) {
        PTRANSMSG pTransMsgBuf, pTransMsgTemp;

        pTransMsgBuf = (PTRANSMSG)ImmLocalAlloc(0, iNum * sizeof(TRANSMSG));

        if (pTransMsgBuf != NULL) {

            RtlCopyMemory(pTransMsgBuf, pTransMsg, iNum * sizeof(TRANSMSG));

            if (GetClientInfo()->dwExpWinVer < VER40) {
                /*
                 * translate messages for those applications that expect
                 * old style IME messages.
                 */
                DWORD dwLangId;
                dwLangId = PRIMARYLANGID(LANGIDFROMLCID(GetSystemDefaultLCID()));
                if ( (dwLangId == LANG_KOREAN && TransGetLevel(pInputContext->hWnd) == 3) ||
                     (dwLangId == LANG_JAPANESE) ) {
                    iNum = WINNLSTranslateMessage(iNum,
                                                  pTransMsgBuf,
                                                  hImc,
                                                  !fUnicodeImc,
                                                  dwLangId );
                }
            }

            pTransMsgTemp = pTransMsgBuf;

            for (i = 0; i < iNum; i++) {
                if (fUnicodeImc) {
                    SendMessageW( pInputContext->hWnd,
                                  pTransMsgTemp->message,
                                  pTransMsgTemp->wParam,
                                  pTransMsgTemp->lParam );
                } else {
                    SendMessageW( pInputContext->hWnd,
                                  pTransMsgTemp->message,
                                  pTransMsgTemp->wParam,
                                  pTransMsgTemp->lParam );
                }
                pTransMsgTemp++;
            }

            ImmLocalFree(pTransMsgBuf);
        }

        ImmUnlockIMCC(pInputContext->hMsgBuf);
    }

    /*
     * We should not reallocate the message buffer
     */
    pInputContext->dwNumMsgBuf = 0L;

    ImmUnlockIMC(hImc);

    return TRUE;
}


/**************************************************************************\
* ImmGetVirtualKey
*
* Gets the actual virtual key which is preprocessed by an IME.
*
* 03-Jan-1996 wkwok       Created
\**************************************************************************/

UINT WINAPI ImmGetVirtualKey(
    HWND hWnd)
{
    HIMC          hImc;
    PINPUTCONTEXT pInputContext;
    UINT          uVirtKey;

    hImc = ImmGetContext(hWnd);

    pInputContext = ImmLockIMC(hImc);
    if (!pInputContext) {
        RIPMSG1(RIP_WARNING, "ImmGetVirtualKey: lock IMC %x failure", hImc);
        return (VK_PROCESSKEY);
    }

    if (pInputContext->fChgMsg) {
        uVirtKey = pInputContext->uSavedVKey;
    } else {
        uVirtKey = VK_PROCESSKEY;
    }

    ImmUnlockIMC(hImc);
    return (uVirtKey);
}


/**************************************************************************\
* ImmLockIMC
*
* 03-Jan-1996 wkwok       Created
\**************************************************************************/

PINPUTCONTEXT WINAPI InternalImmLockIMC(
    HIMC hImc,
    BOOL fCanCallImeSelect)
{
    PCLIENTIMC    pClientImc;
    PINPUTCONTEXT pInputContext;
    DWORD         dwImcThreadId;

    if ((pClientImc = ImmLockClientImc(hImc)) == NULL)
        return NULL;

    EnterImcCrit(pClientImc);

    if (pClientImc->hInputContext == NULL) {
        /*
         * If the owner thread of this hImc does not have
         * default IME window, don't bother to create the
         * INPUTCONTEXT. It could happen when some other
         * thread which call ImmGetContext() to retrieve
         * the associate hImc before the default IME window
         * is created.
         */
        if ((HWND)NtUserQueryInputContext(hImc,
                InputContextDefaultImeWindow) == NULL) {
            LeaveImcCrit(pClientImc);
            ImmUnlockClientImc(pClientImc);
            return NULL;
        }

        /*
         * This is a delay creation of INPUTCONTEXT structure. Create
         * it now for this hImc.
         */
        pClientImc->hInputContext = LocalAlloc(LHND, sizeof(INPUTCONTEXT));

        if (pClientImc->hInputContext == NULL) {
            LeaveImcCrit(pClientImc);
            ImmUnlockClientImc(pClientImc);
            return NULL;
        }

        dwImcThreadId = (DWORD)NtUserQueryInputContext(hImc, InputContextThread);

        if (!CreateInputContext(hImc, GetKeyboardLayout(dwImcThreadId), fCanCallImeSelect)) {
            RIPMSG0(RIP_WARNING, "ImmLockIMC: CreateInputContext failed");
            LocalFree(pClientImc->hInputContext);
            pClientImc->hInputContext = NULL;
            LeaveImcCrit(pClientImc);
            ImmUnlockClientImc(pClientImc);
            return NULL;
        }
    }

    LeaveImcCrit(pClientImc);

    pInputContext = (PINPUTCONTEXT)LocalLock(pClientImc->hInputContext);

    /*
     * Increment lock count so that the ImmUnlockClientImc() won't
     * free up the pClientImc->hInputContext.
     */
    InterlockedIncrement(&pClientImc->cLockObj);

    ImmUnlockClientImc(pClientImc);

    return pInputContext;
}

PINPUTCONTEXT WINAPI ImmLockIMC(
    HIMC hImc)
{
    return InternalImmLockIMC(hImc, TRUE);
}

/**************************************************************************\
* ImmUnlockIMC
*
* 03-Jan-1996 wkwok       Created
\**************************************************************************/

BOOL WINAPI ImmUnlockIMC(
    HIMC hImc)
{
    PCLIENTIMC pClientImc;

    if ((pClientImc = ImmLockClientImc(hImc)) == NULL)
        return FALSE;

    if (pClientImc->hInputContext != NULL)
        LocalUnlock(pClientImc->hInputContext);

    /*
     * Decrement lock count so that the ImmUnlockClientImc() can
     * free up the pClientImc->hInputContext if required.
     */
    InterlockedDecrement(&pClientImc->cLockObj);

    ImmUnlockClientImc(pClientImc);

    return TRUE;
}


/**************************************************************************\
* ImmGetIMCLockCount
*
* 03-Jan-1996 wkwok       Created
\**************************************************************************/

DWORD WINAPI ImmGetIMCLockCount(
    HIMC hImc)
{
    PCLIENTIMC pClientImc;
    DWORD      dwRet = 0;

    if ((pClientImc = ImmLockClientImc(hImc)) == NULL)
        return dwRet;

    if (pClientImc->hInputContext != NULL)
        dwRet = (DWORD)(LocalFlags(pClientImc->hInputContext) & LMEM_LOCKCOUNT);

    ImmUnlockClientImc(pClientImc);

    return dwRet;
}


/**************************************************************************\
* ImmCreateIMCC
*
* 03-Jan-1996 wkwok       Created
\**************************************************************************/

HIMCC WINAPI ImmCreateIMCC(
    DWORD dwSize)
{
    // At least size should be DWORD.
    if (dwSize < sizeof(DWORD)) {
        dwSize = sizeof(DWORD);
    }

    return (HIMCC)LocalAlloc(LHND, dwSize);
}


/**************************************************************************\
* ImmDestroyIMCC
*
* 03-Jan-1996 wkwok       Created
\**************************************************************************/

HIMCC WINAPI ImmDestroyIMCC(
    HIMCC hIMCC)
{
    if (hIMCC == NULL) {
        return NULL;
    }

    return (HIMCC)LocalFree(hIMCC);
}


/**************************************************************************\
* ImmLockIMCC
*
* 03-Jan-1996 wkwok       Created
\**************************************************************************/

LPVOID WINAPI ImmLockIMCC(
    HIMCC hIMCC)
{
    if (hIMCC == NULL) {
        return NULL;
    }

    return LocalLock(hIMCC);
}


/**************************************************************************\
* ImmUnlockIMCC
*
* 03-Jan-1996 wkwok       Created
\**************************************************************************/

BOOL WINAPI ImmUnlockIMCC(
    HIMCC hIMCC)
{
    if (hIMCC == NULL) {
        return FALSE;
    }

    return LocalUnlock(hIMCC);
}


/**************************************************************************\
* ImmGetIMCCLockCount
*
* 03-Jan-1996 wkwok       Created
\**************************************************************************/

DWORD WINAPI ImmGetIMCCLockCount(
    HIMCC hIMCC)
{
    if (hIMCC == NULL) {
        return 0;
    }

    return (DWORD)(LocalFlags(hIMCC) & LMEM_LOCKCOUNT);
}


/**************************************************************************\
* ImmReSizeIMCC
*
* 03-Jan-1996 wkwok       Created
\**************************************************************************/

HIMCC WINAPI ImmReSizeIMCC(
    HIMCC hIMCC,
    DWORD dwSize)
{
    if (hIMCC == NULL) {
        return NULL;
    }

    return (HIMCC)LocalReAlloc(hIMCC, dwSize, LHND);
}


/**************************************************************************\
* ImmGetIMCCSize
*
* 03-Jan-1996 wkwok       Created
\**************************************************************************/

DWORD WINAPI ImmGetIMCCSize(
    HIMCC hIMCC)
{
    if (hIMCC == NULL) {
        return 0;
    }

    return (DWORD)LocalSize(hIMCC);
}


/**************************************************************************\
* ImmLocalAlloc
*
* 18-Jun-1996 wkwok       Created
\**************************************************************************/

LPVOID ImmLocalAlloc(
    DWORD uFlag,
    DWORD uBytes)
{
    if (pImmHeap == NULL) {
        pImmHeap = RtlProcessHeap();
        if (pImmHeap == NULL) {
            RIPMSG0(RIP_WARNING, "ImmLocalAlloc: NULL pImmHeap!");
            return NULL;
        }
    }

    return HeapAlloc(pImmHeap, uFlag, uBytes);
}


/***************************************************************************\
* PtiCurrent
*
* Returns the THREADINFO structure for the current thread.
* LATER: Get DLL_THREAD_ATTACH initialization working right and we won't
*        need this connect code.
*
* History:
* 10-28-90 DavidPe      Created.
* 02-21-96 wkwok        Copied from USER32.DLL
\***************************************************************************/

PTHREADINFO PtiCurrent(VOID)
{
    ConnectIfNecessary();
    return (PTHREADINFO)NtCurrentTebShared()->Win32ThreadInfo;
}


/**************************************************************************\
* TestInputContextProcess
*
* 02-21-96 wkwok        Created
\**************************************************************************/

BOOL TestInputContextProcess(
    PIMC pImc)
{
    /*
     * If the threads are the same, don't bother going to the kernel
     * to get the input context's process id.
     */
    if (GETPTI(pImc) == PtiCurrent()) {
        return TRUE;
    }

    return (GetInputContextProcess(PtoH(pImc)) == GETPROCESSID());
}

/**************************************************************************\
* TestWindowProcess
*
* 11-14-94 JimA         Created.
* 02-29-96 wkwok        Copied from USER32.DLL
\**************************************************************************/

BOOL TestWindowProcess(
    PWND pwnd)
{
    /*
     * If the threads are the same, don't bother going to the kernel
     * to get the window's process id.
     */
    if (GETPTI(pwnd) == PtiCurrent()) {
        return TRUE;
    }

    return (GetWindowProcess(HW(pwnd)) == GETPROCESSID());
}


/**************************************************************************\
* GetKeyboardLayoutCP
*
* 12-Mar-1996 wkwok       Created
\**************************************************************************/

static LCID CachedLCID = 0;
static UINT CachedCP = CP_ACP;

UINT GetKeyboardLayoutCP(
    HKL hKL)
{
    #define LOCALE_CPDATA 7
    WCHAR wszCodePage[LOCALE_CPDATA];
    LCID  lcid;

    lcid = MAKELCID(LOWORD(HandleToUlong(hKL)), SORT_DEFAULT);

    if (lcid == CachedLCID)
        return CachedCP;

    if (!GetLocaleInfoW(lcid, LOCALE_IDEFAULTANSICODEPAGE,
                wszCodePage, LOCALE_CPDATA))
        return CP_ACP;

    CachedLCID = lcid;
    CachedCP = (UINT)wcstol(wszCodePage, NULL, 10);

    return CachedCP;
}


/**************************************************************************\
* GetKeyboardLayoutCP
*
* 12-Mar-1996 wkwok       Created
\**************************************************************************/

UINT GetThreadKeyboardLayoutCP(
    DWORD dwThreadId)
{
    HKL hKL;

    hKL = GetKeyboardLayout(dwThreadId);

    return GetKeyboardLayoutCP(hKL);
}


/**************************************************************************\
* ImmLockClientImc
*
* 13-Mar-1996 wkwok       Created
\**************************************************************************/

PCLIENTIMC WINAPI ImmLockClientImc(
    HIMC hImc)
{
    PIMC       pImc;
    PCLIENTIMC pClientImc;

    if (hImc == NULL_HIMC)
        return NULL;

    pImc = HMValidateHandle((HANDLE)hImc, TYPE_INPUTCONTEXT);

    /*
     * Cannot access input context from other process.
     */
    if (pImc == NULL || !TestInputContextProcess(pImc))
        return NULL;

    pClientImc = (PCLIENTIMC)pImc->dwClientImcData;

    if (pClientImc == NULL) {
        /*
         * We delay the creation of client side per-thread default Imc.
         * Now, this is the time to create it.
         */
        pClientImc = ImmLocalAlloc(HEAP_ZERO_MEMORY, sizeof(CLIENTIMC));
        if (pClientImc == NULL)
            return NULL;

        InitImcCrit(pClientImc);
        pClientImc->dwImeCompatFlags = (DWORD)NtUserGetThreadState(UserThreadStateImeCompatFlags);

        /*
         * Update the kernel side input context.
         */
        if (!NtUserUpdateInputContext(hImc,
                UpdateClientInputContext, (ULONG_PTR)pClientImc)) {
            ImmLocalFree(pClientImc);
            return NULL;
        }

        /*
         * Marks with default input context signature.
         */
        SetICF(pClientImc, IMCF_DEFAULTIMC);
    }
    else if (TestICF(pClientImc, IMCF_INDESTROY)) {
        /*
         * Cannot access destroyed input context.
         */
        return NULL;
    }

    InterlockedIncrement(&pClientImc->cLockObj);

    return pClientImc;
}


VOID WINAPI ImmUnlockClientImc(
    PCLIENTIMC pClientImc)
{
    if (InterlockedDecrement(&pClientImc->cLockObj) == 0) {
        if (TestICF(pClientImc, IMCF_INDESTROY)) {
            if (pClientImc->hInputContext != NULL)
                LocalFree(pClientImc->hInputContext);

            DeleteImcCrit(pClientImc);
            ImmLocalFree(pClientImc);
        }
    }

    return;
}

/**************************************************************************\
* ImmGetImeDpi
*
* 08-Jan-1996 wkwok       Created
\**************************************************************************/

PIMEDPI WINAPI ImmGetImeDpi(
    HKL hKL)
{
    PIMEDPI pImeDpi;

    RtlEnterCriticalSection(&gcsImeDpi);

    pImeDpi = gpImeDpi;

    while (pImeDpi != NULL && pImeDpi->hKL != hKL)
        pImeDpi = pImeDpi->pNext;

    RtlLeaveCriticalSection(&gcsImeDpi);

    return (PIMEDPI)pImeDpi;
}


/**************************************************************************\
* ImmLockImeDpi
*
* 08-Jan-1996 wkwok       Created
\**************************************************************************/

PIMEDPI WINAPI ImmLockImeDpi(
    HKL hKL)
{
    PIMEDPI pImeDpi;

    RtlEnterCriticalSection(&gcsImeDpi);

    pImeDpi = gpImeDpi;

    while (pImeDpi != NULL && pImeDpi->hKL != hKL)
        pImeDpi = pImeDpi->pNext;

    if (pImeDpi != NULL) {
        if (pImeDpi->dwFlag & IMEDPI_UNLOADED)
            pImeDpi = NULL;
        else
            pImeDpi->cLock++;
    }

    RtlLeaveCriticalSection(&gcsImeDpi);

    return (PIMEDPI)pImeDpi;
}


/**************************************************************************\
* ImmUnlockImeDpi
*
* 03-Jan-1996 wkwok       Created
\**************************************************************************/

VOID WINAPI ImmUnlockImeDpi(
    PIMEDPI pImeDpi)
{
    PIMEDPI pImeDpiT;

    if (pImeDpi == NULL)
        return;

    RtlEnterCriticalSection(&gcsImeDpi);

    if (--pImeDpi->cLock == 0) {

        if ((pImeDpi->dwFlag & IMEDPI_UNLOADED) ||
            ((pImeDpi->dwFlag & IMEDPI_UNLOCKUNLOAD) &&
             (pImeDpi->ImeInfo.fdwProperty & IME_PROP_END_UNLOAD)))
        {
            /*
             * Unlink it.
             */
            if (gpImeDpi == pImeDpi) {
                gpImeDpi = pImeDpi->pNext;
            }
            else {
                pImeDpiT = gpImeDpi;

                while (pImeDpiT != NULL && pImeDpiT->pNext != pImeDpi)
                    pImeDpiT = pImeDpiT->pNext;

                if (pImeDpiT != NULL)
                    pImeDpiT->pNext = pImeDpi->pNext;
            }

            /*
             * Unload the IME DLL.
             */
            UnloadIME(pImeDpi, TRUE);
            ImmLocalFree(pImeDpi);
        }
    }

    RtlLeaveCriticalSection(&gcsImeDpi);

    return;
}


/**************************************************************************\
* ImmGetImeInfoEx
*
* 03-Jan-1996 wkwok       Created
\**************************************************************************/

BOOL WINAPI ImmGetImeInfoEx(
    PIMEINFOEX piiex,
    IMEINFOEXCLASS SearchType,
    PVOID pvSearchKey)
{
    ImmAssert(piiex != NULL && pvSearchKey != NULL);

    switch (SearchType) {
    case ImeInfoExKeyboardLayout:
        piiex->hkl = *((HKL *)pvSearchKey);
        /*
         * Quick return for non-IME based keyboard layout
         */
        if (!IS_IME_KBDLAYOUT(piiex->hkl))
            return FALSE;
        break;

    case ImeInfoExImeFileName:
        wcscpy(piiex->wszImeFile, (PWSTR)pvSearchKey);
        break;

    default:
        return FALSE;
    }

    return NtUserGetImeInfoEx(piiex, SearchType);
}

/**************************************************************************\
* ImmGetAppCompatFlags
*
* private function
* returns Win95 compatible IME Compatibility flags
*
* 02-July-1996 takaok       Created
\**************************************************************************/
DWORD ImmGetAppCompatFlags( HIMC hImc )
{
    PCLIENTIMC    pClientImc;
    DWORD         dwImeCompat = 0;

    pClientImc = ImmLockClientImc( hImc );
    if ( pClientImc != NULL ) {
        dwImeCompat = pClientImc->dwImeCompatFlags;
        ImmUnlockClientImc( pClientImc );
    }
    return dwImeCompat;
}

/**************************************************************************\
* ImmPtInRect
*
* private function
*
* 02-July-1997 hiroyama     Created
\**************************************************************************/

BOOL ImmPtInRect(
    int left,
    int top,
    int width,
    int height,
    LPPOINT lppt)
{
    return (lppt->x >= left && lppt->x < (left + width) &&
            lppt->y >= top  && lppt->y < (top + height));
}


/**************************************************************************\
* ImmSystemHandler
*
* private function
*
* IMM bulk helper to handle WM_IME_SYSTEM message
*
* 02-July-1997 hiroyama     Created
\**************************************************************************/

LRESULT ImmSystemHandler(
    HIMC hImc,
    WPARAM wParam,
    LPARAM lParam)
{
    LRESULT lRet = 0;

    switch (wParam) {
    case IMS_SENDNOTIFICATION:
        ImmSendNotification((BOOL)lParam);
        break;
    case IMS_FINALIZE_COMPSTR:
        ImmNotifyIME(hImc, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
        break;
    }

    return lRet;
}
