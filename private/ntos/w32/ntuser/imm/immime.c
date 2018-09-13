/**************************************************************************\
* Module Name: immime.c (corresponds to Win95 ime.c)
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* IME DLL related functinality
*
* History:
* 03-Jan-1996 wkwok       Created
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

typedef struct tagSELECTCONTEXT_ENUM {
    HKL hSelKL;
    HKL hUnSelKL;
} SCE, *PSCE;


BOOL NotifyIMEProc(
    HIMC hImc,
    LPARAM lParam)
{
    UserAssert(lParam == CPS_COMPLETE || lParam == CPS_CANCEL);
    ImmNotifyIME(hImc, NI_COMPOSITIONSTR, (DWORD)lParam, 0);
    return TRUE;
}


BOOL SelectContextProc(
    HIMC hImc,
    PSCE psce)
{
    SelectInputContext(psce->hSelKL, psce->hUnSelKL, hImc);
    return TRUE;
}


BOOL InquireIme(
    PIMEDPI pImeDpi)
{
    WNDCLASS    wc;
    BYTE        ClassName[IM_UI_CLASS_SIZE * sizeof(WCHAR)];
    DWORD       dwSystemInfoFlags;
    PIMEINFO    pImeInfo = &pImeDpi->ImeInfo;

    dwSystemInfoFlags = (NtUserGetThreadState(UserThreadStateIsWinlogonThread))
                      ? IME_SYSINFO_WINLOGON : 0;

    if (GetClientInfo()->dwTIFlags & TIF_16BIT)
        dwSystemInfoFlags |= IME_SYSINFO_WOW16;

    (*pImeDpi->pfn.ImeInquire.w)(pImeInfo, (PVOID)ClassName, dwSystemInfoFlags);

    /*
     * parameter checking for each fields.
     */
    if (pImeInfo->dwPrivateDataSize == 0)
        pImeInfo->dwPrivateDataSize = sizeof(UINT);

    if (pImeInfo->fdwProperty & ~(IME_PROP_ALL)) {
        RIPMSG0(RIP_WARNING, "wrong property");
        return FALSE;
    }

    if (pImeInfo->fdwConversionCaps & ~(IME_CMODE_ALL)) {
        RIPMSG0(RIP_WARNING, "wrong conversion capabilities");
        return FALSE;
    }

    if (pImeInfo->fdwSentenceCaps & ~(IME_SMODE_ALL)) {
        RIPMSG0(RIP_WARNING, "wrong sentence capabilities");
        return FALSE;
    }

    if (pImeInfo->fdwUICaps & ~(UI_CAP_ALL)) {
        RIPMSG0(RIP_WARNING, "wrong UI capabilities");
        return FALSE;
    }

    if (pImeInfo->fdwSCSCaps & ~(SCS_CAP_ALL)) {
        RIPMSG0(RIP_WARNING, "wrong set comp string capabilities");
        return FALSE;
    }

    if (pImeInfo->fdwSelectCaps & ~(SELECT_CAP_ALL)) {
        RIPMSG0(RIP_WARNING, "wrong select capabilities");
        return FALSE;
    }

    if (!(pImeInfo->fdwProperty & IME_PROP_UNICODE)) {

        /*
         * This is ANSI IME. Ensure that it is usable under current system
         * codepage.
         */
        if (pImeDpi->dwCodePage != GetACP() && pImeDpi->dwCodePage != CP_ACP) {
            // Note: in the future, if possible, these reference to dwCodepage
            // should be IMECodePage()...
            RIPMSG1(RIP_WARNING, "incompatible codepage(%d) for ANSI IME", pImeDpi->dwCodePage);
            return FALSE;
        }

        /*
         * ANSI -> Unicode Class name.
         */
        MultiByteToWideChar(IMECodePage(pImeDpi),
                            (DWORD)MB_PRECOMPOSED,
                            (LPSTR)ClassName,               // src
                            (INT)-1,
                            pImeDpi->wszUIClass,            // dest
                            IM_UI_CLASS_SIZE);
    } else {
        RtlCopyMemory(pImeDpi->wszUIClass, ClassName, sizeof(ClassName));
    }
    pImeDpi->wszUIClass[IM_UI_CLASS_SIZE-1] = L'\0';

    if (!GetClassInfoW((HINSTANCE)pImeDpi->hInst, pImeDpi->wszUIClass, &wc)) {
        RIPMSG1(RIP_WARNING, "UI class (%ws) not found in this IME", pImeDpi->wszUIClass);
        return FALSE;
    } else if (wc.cbWndExtra < sizeof(DWORD) * 2) {
        RIPMSG0(RIP_WARNING, "UI class cbWndExtra problem");
        return FALSE;
    }

    return TRUE;
}


BOOL LoadIME(
    PIMEINFOEX piiex,
    PIMEDPI    pImeDpi)
{
    WCHAR wszImeFile[MAX_PATH];
    BOOL  fSuccess;

    GetSystemPathName(wszImeFile, piiex->wszImeFile, MAX_PATH);

    pImeDpi->hInst = LoadLibraryW(wszImeFile);

    if (!pImeDpi->hInst) {
        RIPMSG1(RIP_WARNING, "LoadIME: LoadLibraryW(%ws) failed", wszImeFile);
        goto LoadIME_ErrOut;
    }

#define GET_IMEPROCT(x) \
    if (!(pImeDpi->pfn.##x.t = (PVOID) GetProcAddress(pImeDpi->hInst, #x))) { \
        RIPMSG1(RIP_WARNING, "LoadIME: " #x " not supported in %ws", wszImeFile);           \
        goto LoadIME_ErrOut; }

#define GET_IMEPROC(x) \
    if (!(pImeDpi->pfn.##x = (PVOID) GetProcAddress(pImeDpi->hInst, #x))) {   \
        RIPMSG1(RIP_WARNING, "LoadIME: " #x " not supported in %ws", wszImeFile);           \
        goto LoadIME_ErrOut; }

    GET_IMEPROCT(ImeInquire);
    GET_IMEPROCT(ImeConversionList);
    GET_IMEPROCT(ImeRegisterWord);
    GET_IMEPROCT(ImeUnregisterWord);
    GET_IMEPROCT(ImeGetRegisterWordStyle);
    GET_IMEPROCT(ImeEnumRegisterWord);
    GET_IMEPROC (ImeConfigure);
    GET_IMEPROC (ImeDestroy);
    GET_IMEPROC (ImeEscape);
    GET_IMEPROC (ImeProcessKey);
    GET_IMEPROC (ImeSelect);
    GET_IMEPROC (ImeSetActiveContext);
    GET_IMEPROC (ImeToAsciiEx);
    GET_IMEPROC (NotifyIME);
    GET_IMEPROC (ImeSetCompositionString);

    // 4.0 IMEs don't have this entry. could be NULL.
    pImeDpi->pfn.ImeGetImeMenuItems = (PVOID)GetProcAddress(pImeDpi->hInst, "ImeGetImeMenuItems");

#undef GET_IMEPROCT
#undef GET_IMEPROC

    if (!InquireIme(pImeDpi)) {
        RIPMSG0(RIP_WARNING, "LoadIME: InquireIme failed");
LoadIME_ErrOut:
        FreeLibrary(pImeDpi->hInst);
        pImeDpi->hInst = NULL;
        fSuccess = FALSE;
    }
    else {
        fSuccess = TRUE;
    }

    /*
     * Update kernel side IMEINFOEX for this keyboard layout if
     * this is its first loading.
     */
    if (piiex->fLoadFlag == IMEF_NONLOAD) {
        if (fSuccess) {
            RtlCopyMemory((PBYTE)&piiex->ImeInfo,
                          (PBYTE)&pImeDpi->ImeInfo, sizeof(IMEINFO));
            RtlCopyMemory((PBYTE)piiex->wszUIClass,
                          (PBYTE)pImeDpi->wszUIClass, sizeof(pImeDpi->wszUIClass));
            piiex->fLoadFlag = IMEF_LOADED;
        }
        else {
            piiex->fLoadFlag = IMEF_LOADERROR;
        }
        NtUserSetImeInfoEx(piiex);
    }

    return fSuccess;
}


VOID UnloadIME(
    PIMEDPI pImeDpi,
    BOOL    fTerminateIme)
{
    if (pImeDpi->hInst == NULL) {
        RIPMSG0(RIP_WARNING, "UnloadIME: No IME's hInst.");
        return;
    }

    if (fTerminateIme) {
        /*
         * Destroy IME first.
         */
        (*pImeDpi->pfn.ImeDestroy)(0);
    }

    FreeLibrary(pImeDpi->hInst);
    pImeDpi->hInst = NULL;

    return;
}

PIMEDPI LoadImeDpi(
    HKL  hKL,
    BOOL fLock)
{
    PIMEDPI        pImeDpi, pImeDpiT;
    IMEINFOEX      iiex;

    /*
     * Query the IME information.
     */
    if (!ImmGetImeInfoEx(&iiex, ImeInfoExKeyboardLayout, &hKL)) {
        RIPMSG1(RIP_WARNING, "LoadImeDpi: ImmGetImeInfoEx(%lx) failed", hKL);
        return NULL;
    }

    /*
     * Win95 behaviour: If there was an IME load error for this layout,
     * further attempt to load the same IME layout will be rejected.
     */
    if (iiex.fLoadFlag == IMEF_LOADERROR)
        return NULL;

    /*
     * Allocate a new IMEDPI for this layout.
     */
    pImeDpi = (PIMEDPI)ImmLocalAlloc(HEAP_ZERO_MEMORY, sizeof(IMEDPI));
    if (pImeDpi == NULL)
        return NULL;

    pImeDpi->hKL = hKL;

    // get code page of IME
    {
        CHARSETINFO cs;
        if (TranslateCharsetInfo((DWORD*)LOWORD(HandleToUlong(hKL)), &cs, TCI_SRCLOCALE)) {
            pImeDpi->dwCodePage = cs.ciACP;
        }
        else {
            pImeDpi->dwCodePage = CP_ACP;
        }
    }

    /*
     * Load up IME DLL.
     */
    if (!LoadIME(&iiex, pImeDpi)) {
        ImmLocalFree(pImeDpi);
        return NULL;
    }

    /*
     * Link in the newly allocated entry.
     */
    RtlEnterCriticalSection(&gcsImeDpi);

    pImeDpiT = ImmGetImeDpi(hKL);

    if (pImeDpiT == NULL) {
        if (fLock) {
            /*
             * Newly loaded with lock, will unload upon unlock.
             */
            pImeDpi->cLock = 1;
            pImeDpi->dwFlag |= IMEDPI_UNLOCKUNLOAD;
        }

        /*
         * Update the global list for this new pImeDpi entry.
         */
        pImeDpi->pNext = gpImeDpi;
        gpImeDpi = pImeDpi;

        RtlLeaveCriticalSection(&gcsImeDpi);
    }
    else {

        if (!fLock) {
            pImeDpiT->dwFlag &= ~IMEDPI_UNLOCKUNLOAD;
        }

        /*
         * The same IME has been loaded, discard this extra entry.
         */
        RtlLeaveCriticalSection(&gcsImeDpi);
        UnloadIME(pImeDpi, FALSE);
        ImmLocalFree(pImeDpi);
        pImeDpi = pImeDpiT;
    }

    return pImeDpi;
}


PIMEDPI FindOrLoadImeDpi(
    HKL  hKL)
{
    PIMEDPI pImeDpi;

    /*
     * Non IME based keyboard layout doesn't have IMEDPI.
     */
    if (!IS_IME_KBDLAYOUT(hKL))
        return (PIMEDPI)NULL;

    pImeDpi = ImmLockImeDpi(hKL);
    if (pImeDpi == NULL)
        pImeDpi = LoadImeDpi(hKL, TRUE);

    return pImeDpi;
}


BOOL WINAPI ImmLoadIME(
    HKL hKL)
{
    PIMEDPI pImeDpi;

    /*
     * Non IME based keyboard layout doesn't have IMEDPI.
     */
    if (!IS_IME_KBDLAYOUT(hKL))
        return FALSE;

    pImeDpi = ImmGetImeDpi(hKL);
    if (pImeDpi == NULL)
        pImeDpi = LoadImeDpi(hKL, FALSE);

    return (pImeDpi != NULL);
}


BOOL WINAPI ImmUnloadIME(
    HKL hKL)
{
    PIMEDPI pImeDpi, pImeDpiT;

    RtlEnterCriticalSection(&gcsImeDpi);

    pImeDpi = gpImeDpi;

    while (pImeDpi != NULL && pImeDpi->hKL != hKL)
        pImeDpi = pImeDpi->pNext;

    if (pImeDpi == NULL) {
        RtlLeaveCriticalSection(&gcsImeDpi);
        return TRUE;
    }
    else if (pImeDpi->cLock != 0) {
        pImeDpi->dwFlag |= IMEDPI_UNLOADED;
        RtlLeaveCriticalSection(&gcsImeDpi);
        return FALSE;
    }

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

    RtlLeaveCriticalSection(&gcsImeDpi);

    return TRUE;
}


BOOL WINAPI ImmFreeLayout(
    DWORD  dwFlag)
{
    PIMEDPI pImeDpi;
    HKL   *phklRoot, hklCurrent;
    WCHAR  pwszNonImeKLID[KL_NAMELENGTH];
    UINT   nLayouts, uNonImeKLID = 0, i;

    hklCurrent = GetKeyboardLayout(0);

    switch (dwFlag) {

    case IFL_DEACTIVATEIME:
        /*
         * Do nothing if no IME to be deactivated.
         */
        if (!IS_IME_KBDLAYOUT(hklCurrent))
            return TRUE;

        /*
         * Deactivate IME based layout by activating a non-IME based
         * keyboard layout.
         */
        uNonImeKLID = (UINT)LANGIDFROMLCID(GetSystemDefaultLCID());

        nLayouts = GetKeyboardLayoutList(0, NULL);

        if (nLayouts != 0) {
            phklRoot = ImmLocalAlloc(0, nLayouts * sizeof(HKL));
            if (phklRoot == NULL)
                return FALSE;

            nLayouts = GetKeyboardLayoutList(nLayouts, phklRoot);

            for (i = 0; i < nLayouts && IS_IME_KBDLAYOUT(phklRoot[i]); i++) ;

            if (i < nLayouts)
                uNonImeKLID = HandleToUlong(phklRoot[i]) & 0xffff;

            ImmLocalFree(phklRoot);
        }

        wsprintf(pwszNonImeKLID, L"%08x", uNonImeKLID);

        if (LoadKeyboardLayoutW(pwszNonImeKLID, KLF_ACTIVATE) == NULL) {
            RIPMSG1(RIP_WARNING, "ImmFreeLayout: LoadKeyboardLayoutW(%S, KLF_ACTIVATE) failed. Trying 00000409", pwszNonImeKLID);
            // Somehow it failed (probably a bad setup), let's try
            // 409 KL, which should be installed on all localized NTs.
            if (LoadKeyboardLayoutW(L"00000409", KLF_ACTIVATE | KLF_FAILSAFE) == NULL) {
                RIPMSG0(RIP_WARNING, "LoadKeyboardLayoutW(00000409) failed either. will try NULL.");
            }
        }

        break;

    case IFL_UNLOADIME:
        RtlEnterCriticalSection(&gcsImeDpi);
UnloadImeDpiLoop:
        for (pImeDpi = gpImeDpi; pImeDpi != NULL; pImeDpi = pImeDpi->pNext) {
            if (ImmUnloadIME(pImeDpi->hKL))
                goto UnloadImeDpiLoop;        // Rescan as list was updated.
        }
        RtlLeaveCriticalSection(&gcsImeDpi);
        break;

    default:
        {
            HKL hklFlag = (HKL)LongToHandle( dwFlag );
            if (IS_IME_KBDLAYOUT(hklFlag) && hklFlag != hklCurrent) {
                ImmUnloadIME(hklFlag);
            }
        }
        break;
    }

    return TRUE;
}


BOOL WINAPI ImmActivateLayout(
    HKL    hSelKL)
{
    HKL     hUnSelKL;
    HWND    hWndDefaultIme;
    SCE     sce;
    DWORD   dwCPS;
    PIMEDPI pImeDpi;
    BOOLEAN fOptimizeActivation = TRUE;

    hUnSelKL = GetKeyboardLayout(0);

    {
        PCLIENTINFO pClientInfo = GetClientInfo();

        if (pClientInfo->CI_flags & CI_INPUTCONTEXT_REINIT) {
            fOptimizeActivation = FALSE;
        }
    }

    /*
     * if already current active, do nothing
     */
    if (hUnSelKL == hSelKL && fOptimizeActivation)
        return TRUE;

    ImmLoadIME(hSelKL);

    if (hUnSelKL != hSelKL) {
        pImeDpi = ImmLockImeDpi(hUnSelKL);
        if (pImeDpi != NULL) {
            /*
             * Send out CPS_CANCEL or CPS_COMPLETE to every input
             * context assoicated to window(s) created by this thread.
             * Starting from SUR, we only assoicate input context to window created
             * by the same thread.
             */
            dwCPS = (pImeDpi->ImeInfo.fdwProperty & IME_PROP_COMPLETE_ON_UNSELECT) ? CPS_COMPLETE : CPS_CANCEL;
            ImmUnlockImeDpi(pImeDpi);
            ImmEnumInputContext(0, NotifyIMEProc, dwCPS);
        }

        hWndDefaultIme = ImmGetDefaultIMEWnd(NULL);

        if (IsWindow(hWndDefaultIme))
            SendMessage(hWndDefaultIme, WM_IME_SELECT, FALSE, (LPARAM)hUnSelKL);

        /*
         * This is the time to update the kernel side layout handles.
         * We must do this before sending WM_IME_SELECT.
         */
        NtUserSetThreadLayoutHandles(hSelKL, hUnSelKL);
    }

    /*
     * Unselect and select input context(s).
     */
    sce.hSelKL   = hSelKL;
    sce.hUnSelKL = hUnSelKL;
    ImmEnumInputContext(0, (IMCENUMPROC)SelectContextProc, (LPARAM)&sce);

    /*
     * inform UI select after all hIMC select
     */
    if (IsWindow(hWndDefaultIme))
        SendMessage(hWndDefaultIme, WM_IME_SELECT, TRUE, (LPARAM)hSelKL);

    return (TRUE);
}


/***************************************************************************\
* ImmConfigureIMEA
*
* Brings up the configuration dialogbox of the IME with the specified hKL.
*
* History:
* 29-Feb-1995   wkwok   Created
\***************************************************************************/

BOOL WINAPI ImmConfigureIMEA(
    HKL    hKL,
    HWND   hWnd,
    DWORD  dwMode,
    LPVOID lpData)
{
    PWND    pWnd;
    PIMEDPI pImeDpi;
    BOOL    fRet = FALSE;

    if ((pWnd = ValidateHwnd(hWnd)) == (PWND)NULL) {
        RIPMSG1(RIP_WARNING,
              "ImmConfigureIMEA: invalid window handle %x", hWnd);
        return FALSE;
    }

    if (!TestWindowProcess(pWnd)) {
        RIPMSG1(RIP_WARNING,
              "ImmConfigureIMEA: hWnd=%lx belongs to different process!", hWnd);
        return FALSE;
    }

    pImeDpi = FindOrLoadImeDpi(hKL);
    if (pImeDpi == NULL) {
        RIPMSG0(RIP_WARNING, "ImmConfigureIMEA: no pImeDpi entry.");
        return FALSE;
    }

    if (!(pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE) || lpData == NULL) {
        /*
         * Doesn't need A/W conversion. Calls directly to IME to
         * bring up the configuration dialogbox.
         */
            // This message handles by Console IME.
            SendMessage(hWnd, WM_IME_SYSTEM, IMS_OPENPROPERTYWINDOW, 0L);
        fRet = (*pImeDpi->pfn.ImeConfigure)(hKL, hWnd, dwMode, lpData);
            // This message handles by Console IME.
            SendMessage(hWnd, WM_IME_SYSTEM, IMS_CLOSEPROPERTYWINDOW, 0L);
        ImmUnlockImeDpi(pImeDpi);
        return fRet;
    }

    /*
     * ANSI caller, Unicode IME. Needs A/W conversion on lpData when
     * dwMode == IME_CONFIG_REGISTERWORD. In this case, lpData points
     * to a structure of REGISTERWORDA.
     */
    switch (dwMode) {
    case IME_CONFIG_REGISTERWORD:
        {
            LPREGISTERWORDA lpRegisterWordA;
            REGISTERWORDW   RegisterWordW;
            LPVOID          lpBuffer;
            ULONG           cbBuffer;
            INT             i;

            lpRegisterWordA = (LPREGISTERWORDA)lpData;
            cbBuffer = 0;
            lpBuffer = NULL;

            if (lpRegisterWordA->lpReading != NULL)
                cbBuffer += strlen(lpRegisterWordA->lpReading) + 1;

            if (lpRegisterWordA->lpWord != NULL)
                cbBuffer += strlen(lpRegisterWordA->lpWord) + 1;

            if (cbBuffer != 0) {
                cbBuffer *= sizeof(WCHAR);
                if ((lpBuffer = ImmLocalAlloc(0, cbBuffer)) == NULL) {
                    RIPMSG0(RIP_WARNING, "ImmConfigureIMEA: memory failure.");
                    break;
                }
            }

            if (lpRegisterWordA->lpReading != NULL) {
                RegisterWordW.lpReading = lpBuffer;
                i = MultiByteToWideChar(IMECodePage(pImeDpi),
                                        (DWORD)MB_PRECOMPOSED,
                                        (LPSTR)lpRegisterWordA->lpReading,
                                        (INT)strlen(lpRegisterWordA->lpReading),
                                        (LPWSTR)RegisterWordW.lpReading,
                                        (INT)(cbBuffer/sizeof(WCHAR)));
                RegisterWordW.lpReading[i] = L'\0';
                cbBuffer -= (i * sizeof(WCHAR));
            }
            else {
                RegisterWordW.lpReading = NULL;
            }

            if (lpRegisterWordA->lpWord != NULL) {
                if (RegisterWordW.lpReading != NULL)
                    RegisterWordW.lpWord = &RegisterWordW.lpReading[i+1];
                else
                    RegisterWordW.lpWord = lpBuffer;
                i = MultiByteToWideChar(IMECodePage(pImeDpi),
                                        (DWORD)MB_PRECOMPOSED,
                                        (LPSTR)lpRegisterWordA->lpWord,
                                        (INT)strlen(lpRegisterWordA->lpWord),
                                        (LPWSTR)RegisterWordW.lpWord,
                                        (INT)(cbBuffer/sizeof(WCHAR)));
                RegisterWordW.lpWord[i] = L'\0';
            }
            else
                RegisterWordW.lpWord = NULL;

            fRet = ImmConfigureIMEW(hKL, hWnd, dwMode, &RegisterWordW);

            if (lpBuffer != NULL)
                ImmLocalFree(lpBuffer);

            break;
        }
    default:
        fRet = ImmConfigureIMEW(hKL, hWnd, dwMode, lpData);
        break;
    }

    ImmUnlockImeDpi(pImeDpi);

    return fRet;
}


/***************************************************************************\
* ImmConfigureIMEW
*
* Brings up the configuration dialogbox of the IME with the specified hKL.
*
* History:
* 29-Feb-1995   wkwok   Created
\***************************************************************************/

BOOL WINAPI ImmConfigureIMEW(
    HKL    hKL,
    HWND   hWnd,
    DWORD  dwMode,
    LPVOID lpData)
{
    PWND    pWnd;
    PIMEDPI pImeDpi;
    BOOL    fRet = FALSE;

    if ((pWnd = ValidateHwnd(hWnd)) == (PWND)NULL) {
        RIPMSG1(RIP_WARNING,
              "ImmConfigureIMEA: invalid window handle %x", hWnd);
        return FALSE;
    }

    if (!TestWindowProcess(pWnd)) {
        RIPMSG1(RIP_WARNING,
              "ImmConfigureIMEA: hWnd=%lx belongs to different process!", hWnd);
        return FALSE;
    }

    pImeDpi = FindOrLoadImeDpi(hKL);
    if (pImeDpi == NULL) {
        RIPMSG0(RIP_WARNING, "ImmConfigureIMEA: no pImeDpi entry.");
        return FALSE;
    }

    if ((pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE) || lpData == NULL) {
        /*
         * Doesn't need A/W conversion. Calls directly to IME to
         * bring up the configuration dialogbox.
         */
            // This message handles by Console IME.
            SendMessage(hWnd, WM_IME_SYSTEM, IMS_OPENPROPERTYWINDOW, 0L);
        fRet = (*pImeDpi->pfn.ImeConfigure)(hKL, hWnd, dwMode, lpData);
            // This message handles by Console IME.
            SendMessage(hWnd, WM_IME_SYSTEM, IMS_CLOSEPROPERTYWINDOW, 0L);
        ImmUnlockImeDpi(pImeDpi);
        return fRet;
    }

    /*
     * Unicode caller, ANSI IME. Needs A/W conversion on lpData when
     * dwMode == IME_CONFIG_REGISTERWORD. In this case, lpData points
     * to a structure of REGISTERWORDW.
     */
    switch (dwMode) {
    case IME_CONFIG_REGISTERWORD:
        {
            LPREGISTERWORDW lpRegisterWordW;
            REGISTERWORDA   RegisterWordA;
            LPVOID          lpBuffer;
            ULONG           cbBuffer;
            BOOL            bUDC;
            INT             i;

            lpRegisterWordW = (LPREGISTERWORDW)lpData;
            cbBuffer = 0;
            lpBuffer = NULL;

            if (lpRegisterWordW->lpReading != NULL)
                cbBuffer += wcslen(lpRegisterWordW->lpReading) + 1;

            if (lpRegisterWordW->lpWord != NULL)
                cbBuffer += wcslen(lpRegisterWordW->lpWord) + 1;

            if (cbBuffer != 0) {
                cbBuffer *= sizeof(WCHAR);
                if ((lpBuffer = ImmLocalAlloc(0, cbBuffer)) == NULL) {
                    RIPMSG0(RIP_WARNING, "ImmConfigureIMEW: memory failure.");
                    break;
                }
            }

            if (lpRegisterWordW->lpReading != NULL) {
                RegisterWordA.lpReading = lpBuffer;
                i = WideCharToMultiByte(IMECodePage(pImeDpi),
                                        (DWORD)0,
                                        (LPWSTR)lpRegisterWordW->lpReading,
                                        (INT)wcslen(lpRegisterWordW->lpReading),
                                        (LPSTR)RegisterWordA.lpReading,
                                        (INT)cbBuffer,
                                        (LPSTR)NULL,
                                        (LPBOOL)&bUDC);
                RegisterWordA.lpReading[i] = '\0';
                cbBuffer -= (i * sizeof(CHAR));
            }
            else {
                RegisterWordA.lpReading = NULL;
            }

            if (lpRegisterWordW->lpWord != NULL) {
                if (RegisterWordA.lpReading != NULL)
                    RegisterWordA.lpWord = &RegisterWordA.lpReading[i+1];
                else
                    RegisterWordA.lpWord = lpBuffer;
                i = WideCharToMultiByte(IMECodePage(pImeDpi),
                                        (DWORD)0,
                                        (LPWSTR)lpRegisterWordW->lpWord,
                                        (INT)wcslen(lpRegisterWordW->lpWord),
                                        (LPSTR)RegisterWordA.lpWord,
                                        (INT)cbBuffer,
                                        (LPSTR)NULL,
                                        (LPBOOL)&bUDC);
                RegisterWordA.lpWord[i] = '\0';
            }
            else
                RegisterWordA.lpWord = NULL;

            fRet = ImmConfigureIMEA(hKL, hWnd, dwMode, &RegisterWordA);

            if (lpBuffer != NULL)
                ImmLocalFree(lpBuffer);

            break;
        }
    default:
        fRet = ImmConfigureIMEA(hKL, hWnd, dwMode, lpData);
        break;
    }

    ImmUnlockImeDpi(pImeDpi);

    return fRet;
}


#define IME_T_EUDC_DIC_SIZE 80  // the Traditional Chinese EUDC dictionary

/***************************************************************************\
* ImmEscapeA
*
* This API allows an application to access capabilities of a particular
* IME with specified hKL not directly available thru. other IMM APIs.
* This is necessary mainly for country specific functions or private
* functions in IME.
*
* History:
* 29-Feb-1995   wkwok   Created
\***************************************************************************/

LRESULT WINAPI ImmEscapeA(
    HKL    hKL,
    HIMC   hImc,
    UINT   uSubFunc,
    LPVOID lpData)
{
    PIMEDPI pImeDpi;
    LRESULT lRet = 0;

    pImeDpi = FindOrLoadImeDpi(hKL);
    if (pImeDpi == NULL) {
        RIPMSG0(RIP_WARNING, "ImmEscapeA: no pImeDpi entry.");
        return lRet;
    }

    if ((pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE) == 0 || lpData == NULL) {
        /*
         * Doesn't need A/W conversion. Calls directly to IME to
         * bring up the configuration dialogbox.
         */
        lRet = (*pImeDpi->pfn.ImeEscape)(hImc, uSubFunc, lpData);
        ImmUnlockImeDpi(pImeDpi);
        return lRet;
    }

    /*
     * ANSI caller, Unicode IME. Needs A/W conversion depending on
     * uSubFunc.
     */
    switch (uSubFunc) {
    case IME_ESC_GET_EUDC_DICTIONARY:
    case IME_ESC_IME_NAME:
    case IME_ESC_GETHELPFILENAME:
        {
            WCHAR wszData[IME_T_EUDC_DIC_SIZE];
            BOOL  bUDC;
            INT   i;

            lRet = ImmEscapeW(hKL, hImc, uSubFunc, (LPVOID)wszData);

            if (lRet != 0) {

                try {
                    i = WideCharToMultiByte(IMECodePage(pImeDpi),
                                            (DWORD)0,
                                            (LPWSTR)wszData,         // src
                                            (INT)wcslen(wszData),
                                            (LPSTR)lpData,           // dest
                                            (INT)IME_T_EUDC_DIC_SIZE,
                                            (LPSTR)NULL,
                                            (LPBOOL)&bUDC);
                    ((LPSTR)lpData)[i] = '\0';
                }
                except (EXCEPTION_EXECUTE_HANDLER) {
                    lRet = 0;
                }
            }

            break;
        }

    case IME_ESC_SET_EUDC_DICTIONARY:
    case IME_ESC_HANJA_MODE:
        {
            WCHAR wszData[IME_T_EUDC_DIC_SIZE];
            INT   i;

            i = MultiByteToWideChar(IMECodePage(pImeDpi),
                                    (DWORD)MB_PRECOMPOSED,
                                    (LPSTR)lpData,             // src
                                    (INT)strlen(lpData),
                                    (LPWSTR)wszData,          // dest
                                    (INT)sizeof(wszData)/sizeof(WCHAR));
            wszData[i] = L'\0';

            lRet = ImmEscapeW(hKL, hImc, uSubFunc, (LPVOID)wszData);

            break;
        }

    case IME_ESC_SEQUENCE_TO_INTERNAL:
        {
            CHAR    szData[4];
            WCHAR   wszData[4];
            INT     i = 0;

            lRet = ImmEscapeW(hKL, hImc, uSubFunc, lpData);

            if (HIWORD(lRet))
                wszData[i++] = HIWORD(lRet);

            if (LOWORD(lRet))
                wszData[i++] = LOWORD(lRet);

            i = WideCharToMultiByte(IMECodePage(pImeDpi),
                                    (DWORD)0,
                                    (LPWSTR)wszData,        // src
                                    (INT)i,
                                    (LPSTR)szData,          // dest
                                    (INT)sizeof(szData),
                                    (LPSTR)NULL,
                                    (LPBOOL)NULL);

            switch (i) {
            case 1:
                lRet = MAKELONG(MAKEWORD(szData[0], 0), 0);
                break;

            case 2:
                lRet = MAKELONG(MAKEWORD(szData[1], szData[0]), 0);
                break;

            case 3:
                lRet = MAKELONG(MAKEWORD(szData[2], szData[1]), MAKEWORD(szData[0], 0));
                break;

            case 4:
                lRet = MAKELONG(MAKEWORD(szData[3], szData[2]), MAKEWORD(szData[1], szData[0]));
                break;

            default:
                lRet = 0;
                break;
            }

            break;
        }
    default:
        lRet = ImmEscapeW(hKL, hImc, uSubFunc, lpData);
        break;
    }

    ImmUnlockImeDpi(pImeDpi);

    return lRet;
}


/***************************************************************************\
* ImmEscapeW
*
* This API allows an application to access capabilities of a particular
* IME with specified hKL not directly available thru. other IMM APIs.
* This is necessary mainly for country specific functions or private
* functions in IME.
*
* History:
* 29-Feb-1995   wkwok   Created
\***************************************************************************/

LRESULT WINAPI ImmEscapeW(
    HKL    hKL,
    HIMC   hImc,
    UINT   uSubFunc,
    LPVOID lpData)
{
    PIMEDPI pImeDpi;
    LRESULT lRet = 0;

    pImeDpi = FindOrLoadImeDpi(hKL);
    if (pImeDpi == NULL) {
        RIPMSG0(RIP_WARNING, "ImmEscapeW: no pImeDpi entry.");
        return lRet;
    }

    if ((pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE) || lpData == NULL) {
        /*
         * Doesn't need W/A conversion. Calls directly to IME to
         * bring up the configuration dialogbox.
         */
        lRet = (*pImeDpi->pfn.ImeEscape)(hImc, uSubFunc, lpData);
        ImmUnlockImeDpi(pImeDpi);
        return lRet;
    }

    /*
     * Unicode caller, ANSI IME. Needs W/A conversion depending on
     * uSubFunc.
     */
    switch (uSubFunc) {
    case IME_ESC_GET_EUDC_DICTIONARY:
    case IME_ESC_IME_NAME:
    case IME_ESC_GETHELPFILENAME:
        {
            CHAR szData[IME_T_EUDC_DIC_SIZE];
            INT  i;

            lRet = ImmEscapeA(hKL, hImc, uSubFunc, (LPVOID)szData);

            if (lRet != 0) {

                try {
                    i = MultiByteToWideChar(IMECodePage(pImeDpi),
                                            (DWORD)MB_PRECOMPOSED,
                                            (LPSTR)szData,          // src
                                            (INT)strlen(szData),
                                            (LPWSTR)lpData,         // dest
                                            (INT)IME_T_EUDC_DIC_SIZE);
                    ((LPWSTR)lpData)[i] = L'\0';
                }
                except (EXCEPTION_EXECUTE_HANDLER) {
                    lRet = 0;
                }
            }

            break;
        }

    case IME_ESC_SET_EUDC_DICTIONARY:
    case IME_ESC_HANJA_MODE:
        {
            CHAR szData[IME_T_EUDC_DIC_SIZE];
            BOOL bUDC;
            INT  i;

            i = WideCharToMultiByte(IMECodePage(pImeDpi),
                                    (DWORD)0,
                                    (LPWSTR)lpData,          // src
                                    (INT)wcslen(lpData),
                                    (LPSTR)szData,          // dest
                                    (INT)sizeof(szData),
                                    (LPSTR)NULL,
                                    (LPBOOL)&bUDC);
            szData[i] = '\0';

            lRet = ImmEscapeA(hKL, hImc, uSubFunc, (LPVOID)szData);

            break;
        }

    case IME_ESC_SEQUENCE_TO_INTERNAL:
        {
            CHAR    szData[4];
            WCHAR   wszData[4];
            INT     i = 0;

            lRet = ImmEscapeA(hKL, hImc, uSubFunc, lpData);

            if (HIBYTE(LOWORD(lRet)))
                szData[i++] = HIBYTE(LOWORD(lRet));

            if (LOBYTE(LOWORD(lRet)))
                szData[i++] = LOBYTE(LOWORD(lRet));

            i = MultiByteToWideChar(IMECodePage(pImeDpi),
                                    (DWORD)MB_PRECOMPOSED,
                                    (LPSTR)szData,            // src
                                    i,
                                    (LPWSTR)wszData,          // dest
                                    (INT)sizeof(wszData)/sizeof(WCHAR));

            switch (i) {
            case 1:
                lRet = MAKELONG(wszData[0], 0);
                break;

            case 2:
                lRet = MAKELONG(wszData[1], wszData[0]);
                break;

            default:
                lRet = 0;
                break;
            }

            break;
        }

    default:
        lRet = ImmEscapeA(hKL, hImc, uSubFunc, lpData);
        break;
    }

    ImmUnlockImeDpi(pImeDpi);

    return lRet;
}


BOOL WINAPI ImmPenAuxInput(HWND hwndSender, LPVOID lpData)
{
    PIMEDPI pImeDpi = NULL;
    PCOPYDATASTRUCT lpCopyData = (PCOPYDATASTRUCT)lpData;
    PENINPUTDATA* lpPenInputData = (LPVOID)lpCopyData->lpData;
    IMEPENDATA ImePenData;
    HWND hwnd;
    HIMC himc;
    HKL hkl;
    DWORD dwData = 0 ;
    LPDWORD lpdwData = NULL;

    UNREFERENCED_PARAMETER(hwndSender);

    if (lpCopyData->dwData != LM_IMM_MAGIC || lpCopyData->cbData < sizeof(PENINPUTDATA)) {
        RIPMSG0(RIP_WARNING, "ImmPenAuxInput: invalid COPYDATASTRUCT signagure.");
        return FALSE;
    }

    if (lpPenInputData->dwVersion != 0) {
        RIPMSG0(RIP_WARNING, "ImmPenAuxInput: invalid Pendata version.");
        return FALSE;
    }

    hwnd = GetFocus();
    hkl = GetKeyboardLayout(0);
    if (hwnd == NULL || hkl == NULL || (himc = ImmGetContext(hwnd)) == NULL) {
        RIPMSG0(RIP_WARNING, "ImmPenAuxInput: hwnd, hkl or himc cannot be aquired.");
        return FALSE;
    }

    if ((pImeDpi = FindOrLoadImeDpi(hkl)) == NULL) {
        RIPMSG0(RIP_WARNING, "ImmPenAuxInput: IME DPI cannot be found.");
        return FALSE;
    }

    do {    // dummy loop (execute just once) so that we can 'break' in case of unexpected errors
        dwData = IME_ESC_PENAUXDATA;
        if (!pImeDpi->pfn.ImeEscape(himc, IME_ESC_QUERY_SUPPORT, (LPVOID)&dwData)) {
            //
            // IME_ESC_PENAUXDATA is not supported by the current IME.
            //
            RIPMSG1(RIP_VERBOSE, "ImmPenAuxInput: IME(hkl=%08x) does not support IME_ESC_PENDATA", hkl);
            break;
        }

        dwData = 0; // Be prepared for unexpected exodus

        //
        // Makeup the IMEPENDATA structure.
        //

        RtlZeroMemory(&ImePenData, sizeof ImePenData);

        ImePenData.dwCount = lpPenInputData->cnt;

        if (lpPenInputData->flags & ~(LMDATA_SYMBOL_DWORD | LMDATA_SKIP_WORD | LMDATA_SCORE_WORD)) {
            RIPMSG1(RIP_WARNING, "ImmPenAuxInput: flag out of range (0x%08x)", lpPenInputData->flags);
        }

        //
        // Setup the structure for IME.
        //

        if (lpPenInputData->flags & LMDATA_SYMBOL_DWORD) {
            if (lpPenInputData->dwOffsetSymbols > lpCopyData->cbData ||
                    lpPenInputData->dwOffsetSymbols + lpPenInputData->cnt * sizeof(DWORD) > lpCopyData->cbData) {
                //
                // Invalid structure
                //
                RIPMSG1(RIP_WARNING, "ImmPenAuxInput: illegal dwOffsetSymbols (0x%x)", lpPenInputData->dwOffsetSymbols);
                break;
            }
            ImePenData.wd.lpSymbol = (LPVOID)&lpPenInputData->ab[lpPenInputData->dwOffsetSymbols];
            ImePenData.dwFlags |= IME_PEN_SYMBOL;

            //
            // If it's ANSI IME, we need to translate the symbols
            //
            if ((pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE) == 0) {
                USHORT wCodePage = (USHORT)GetKeyboardLayoutCP(hkl);
                int i;

                lpdwData = ImmLocalAlloc(HEAP_ZERO_MEMORY, sizeof *lpdwData * ImePenData.dwCount);
                if (lpdwData == NULL) {
                    RIPMSG0(RIP_WARNING, "ImmPenAuxInput: could not allocate lpdwData");
                    break;
                }
                for (i = 0; i < (int)ImePenData.dwCount; ++i) {
                    LPSTR lpstr = (LPSTR)(lpdwData + i);

                    // Assuming little endian:
                    WCSToMBEx(wCodePage,
                              (LPCWSTR)(ImePenData.wd.lpSymbol + i), 1,
                              &lpstr, 2,
                              FALSE);
                    ImmAssert(HIWORD(lpdwData[i]) == 0);

                    // Copy the high word (column #).
                    lpdwData[i] |= (ImePenData.wd.lpSymbol[i] & ~0xffff);
                }
                ImePenData.wd.lpSymbol = lpdwData;
            }

        }

        if (lpPenInputData->flags & LMDATA_SKIP_WORD) {
            if (lpPenInputData->dwOffsetSkip > lpCopyData->cbData ||
                    lpPenInputData->dwOffsetSkip + lpPenInputData->cnt * sizeof(WORD) > lpCopyData->cbData) {
                //
                // Invalid structure
                //
                RIPMSG1(RIP_WARNING, "ImmPenAuxInput: illegal dwOffsetSkip (0x%x)", lpPenInputData->dwOffsetSkip);
                break;
            }
            ImePenData.wd.lpSkip = (LPVOID)&lpPenInputData->ab[lpPenInputData->dwOffsetSkip];
            ImePenData.dwFlags |= IME_PEN_SKIP;
        }

        if (lpPenInputData->flags & LMDATA_SCORE_WORD) {
            if (lpPenInputData->dwOffsetScore > lpCopyData->cbData ||
                    lpPenInputData->dwOffsetScore + lpPenInputData->cnt * sizeof(WORD) > lpCopyData->cbData) {
                //
                // Invalid structure
                //
                RIPMSG1(RIP_WARNING, "ImmPenAuxInput: illegal dwOffsetScore (0x%x)", lpPenInputData->dwOffsetScore);
                break;
            }
            ImePenData.wd.lpScore = (LPVOID)&lpPenInputData->ab[lpPenInputData->dwOffsetScore];
            ImePenData.dwFlags |= IME_PEN_SCORE;
        }
        dwData = (DWORD)pImeDpi->pfn.ImeEscape(himc, IME_ESC_PENAUXDATA, &ImePenData);
    } while (FALSE);

    if (lpdwData) {
        ImmLocalFree(lpdwData);
    }

    ImmAssert(pImeDpi);
    ImmUnlockImeDpi(pImeDpi);

    return dwData;
}

LRESULT WINAPI ImmSendMessageToActiveDefImeWndW(
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    HWND hwndIme;

    //
    // Today we only support this for WM_COPYDATA
    //
    if (msg != WM_COPYDATA) {
        return 0;
    }

    hwndIme = NtUserQueryWindow((HWND)wParam, WindowActiveDefaultImeWindow);
    if (hwndIme == NULL) {
        return 0;
    }

    return SendMessage(hwndIme, msg, wParam, lParam);
}

BOOL WINAPI ImmNotifyIME(
    HIMC  hImc,
    DWORD dwAction,
    DWORD dwIndex,
    DWORD dwValue)
{
    PIMEDPI pImeDpi;
    BOOL    bRet;

    if (hImc != NULL_HIMC &&
            GetInputContextThread(hImc) != GetCurrentThreadId()) {
        RIPMSG1(RIP_WARNING,
              "ImmNotifyIME: Invalid input context access %lx.", hImc);
        return FALSE;
    }

    pImeDpi = ImmLockImeDpi(GetKeyboardLayout(0));
    if (pImeDpi == NULL)
        return FALSE;

    bRet = (*pImeDpi->pfn.NotifyIME)(hImc, dwAction, dwIndex, dwValue);

    ImmUnlockImeDpi(pImeDpi);

    return bRet;
}
