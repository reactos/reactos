/**************************************************************************\
* Module Name: imectl.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* IME Window Handling Routines
*
* History:
* 20-Dec-1995 wkwok
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include <intlshar.h>

#define LATE_CREATEUI 1

CONST WCHAR szIndicDLL[] = L"indicdll.dll";

FARPROC gpfnGetIMEMenuItemData = NULL;
BOOL IMEIndicatorGetMenuIDData(PUINT puMenuID, PDWORD pdwData);

/*
 * Local Routines.
 */
LONG ImeWndCreateHandler(PIMEUI, LPCREATESTRUCT);
void ImeWndDestroyHandler(PIMEUI);
LRESULT ImeSystemHandler(PIMEUI, UINT, WPARAM, LPARAM);
LONG ImeSelectHandler(PIMEUI, UINT, WPARAM, LPARAM);
LRESULT ImeControlHandler(PIMEUI, UINT, WPARAM, LPARAM, BOOL);
LRESULT ImeSetContextHandler(PIMEUI, UINT, WPARAM, LPARAM);
LRESULT ImeNotifyHandler(PIMEUI, UINT, WPARAM, LPARAM);
HWND CreateIMEUI(PIMEUI, HKL);
VOID DestroyIMEUI(PIMEUI);
LRESULT SendMessageToUI(PIMEUI, UINT, WPARAM, LPARAM, BOOL);
VOID SendOpenStatusNotify(PIMEUI, HWND, BOOL);
VOID ImeSetImc(PIMEUI, HIMC);
VOID FocusSetIMCContext(HWND, BOOL);
BOOL ImeBroadCastMsg(PIMEUI, UINT, WPARAM, LPARAM);
VOID ImeMarkUsedContext(HWND, HIMC);
BOOL ImeIsUsableContext(HWND, HIMC);
BOOL GetIMEShowStatus(void);
LRESULT ImeCopyDataHandler(WPARAM, LPARAM);

/*
 * Common macros for IME UI, HKL and IMC handlings
 */
#define GETHKL(pimeui)        (pimeui->hKL)
#define SETHKL(pimeui, hkl)   (pimeui->hKL=(hkl))
#define GETIMC(pimeui)        (pimeui->hIMC)
#define SETIMC(pimeui, himc)  (ImeSetImc(pimeui, himc))
#define GETUI(pimeui)         (pimeui->hwndUI)
#define SETUI(pimeui, hwndui) (pimeui->hwndUI=(hwndui))

LOOKASIDE ImeUILookaside;

/***************************************************************************\
* NtUserBroadcastImeShowStatusChange(), NtUserCheckImeShowStatusInThread()
*
* Stub for kernel mode routines
*
\***************************************************************************/

_inline void NtUserBroadcastImeShowStatusChange(HWND hwndDefIme, BOOL fShow)
{
    NtUserCallHwndParamLock(hwndDefIme, fShow, SFI_XXXBROADCASTIMESHOWSTATUSCHANGE);
}

_inline void NtUserCheckImeShowStatusInThread(HWND hwndDefIme)
{
    NtUserCallHwndLock(hwndDefIme, SFI_XXXCHECKIMESHOWSTATUSINTHREAD);
}

/***************************************************************************\
* ImeWndProc
*
* WndProc for IME class
*
* History:
\***************************************************************************/

LRESULT APIENTRY ImeWndProcWorker(
    PWND pwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    DWORD fAnsi)
{
    HWND        hwnd = HWq(pwnd);
    PIMEUI      pimeui;
    static BOOL fInit = TRUE;

    CheckLock(pwnd);

    VALIDATECLASSANDSIZE(pwnd, FNID_IME);
    INITCONTROLLOOKASIDE(&ImeUILookaside, IMEUI, spwnd, 8);

    /*
     * If the control is not interested in this message,
     * pass it to DefWindowProc.
     */
    if (!FWINDOWMSG(message, FNID_IME))
        return DefWindowProcWorker(pwnd, message, wParam, lParam, fAnsi);

    /*
     * Get the pimeui for the given window now since we will use it a lot in
     * various handlers. This was stored using SetWindowLong(hwnd,0,pimeui) when
     * we initially created the IME control.
     */
    pimeui = ((PIMEWND)pwnd)->pimeui;

    if (pimeui == NULL) {
        /*
         * Further processing not needed
         */
        RIPMSG0(RIP_WARNING, "ImeWndProcWorker: pimeui == NULL\n");
        return 0L;
    }

    /*
     * This is necessary to avoid recursion call from IME UI.
     */
    UserAssert(pimeui->nCntInIMEProc >= 0);

    if (pimeui->nCntInIMEProc > 0) {
        TAGMSG5(DBGTAG_IMM, "ImeWndProcWorker: Recursive for pwnd=%08p, msg=%08x, wp=%08x, lp=%08x, fAnsi=%d\n",
                pwnd, message, wParam, lParam, fAnsi);
        switch (message) {
        case WM_IME_SYSTEM:
            switch (wParam) {
            case IMS_ISACTIVATED:
            case IMS_SETOPENSTATUS:
//          case IMS_SETCONVERSIONSTATUS:
            case IMS_SETSOFTKBDONOFF:
                /*
                 * Because these will not be pass to UI.
                 * We can do it.
                 */
                break;

            default:
                return 0L;
            }
            break;

        case WM_IME_STARTCOMPOSITION:
        case WM_IME_ENDCOMPOSITION:
        case WM_IME_COMPOSITION:
        case WM_IME_SETCONTEXT:
        case WM_IME_NOTIFY:
        case WM_IME_CONTROL:
        case WM_IME_COMPOSITIONFULL:
        case WM_IME_SELECT:
        case WM_IME_CHAR:
        case WM_IME_REQUEST:
            return 0L;

        default:
            return DefWindowProcWorker(pwnd, message, wParam, lParam, fAnsi);
        }
    }

    if (TestWF(pwnd, WFINDESTROY) || TestWF(pwnd, WFDESTROYED)) {
        switch (message) {
        case WM_DESTROY:
        case WM_NCDESTROY:
        case WM_FINALDESTROY:
            break;
        default:
            RIPMSG1(RIP_WARNING, "ImeWndProcWorker: message %x is sent to destroyed window.", message);
            return 0L;
        }
    }

    switch (message) {
    case WM_ERASEBKGND:
        return (LONG)TRUE;

    case WM_PAINT:
        break;

    case WM_CREATE:

        return ImeWndCreateHandler(pimeui, (LPCREATESTRUCT)lParam);

    case WM_DESTROY:
        /*
         * We are destroying the IME window, destroy
         * any UI window that it owns.
         */
        ImeWndDestroyHandler(pimeui);
        break;

    case WM_NCDESTROY:
    case WM_FINALDESTROY:
        if (pimeui) {
            Unlock(&pimeui->spwnd);
            FreeLookasideEntry(&ImeUILookaside, pimeui);
        }
        NtUserSetWindowFNID(hwnd, FNID_CLEANEDUP_BIT);
        goto CallDWP;

    case WM_IME_SYSTEM:
        UserAssert(pimeui->spwnd == pwnd);
        return ImeSystemHandler(pimeui, message, wParam, lParam);

    case WM_IME_SELECT:
        return ImeSelectHandler(pimeui, message, wParam, lParam);

    case WM_IME_CONTROL:
        return ImeControlHandler(pimeui, message, wParam, lParam, fAnsi);

    case WM_IME_SETCONTEXT:
        return ImeSetContextHandler(pimeui, message, wParam, lParam);

    case WM_IME_NOTIFY:
        return ImeNotifyHandler(pimeui, message, wParam, lParam);

    case WM_IME_REQUEST:
        return 0;

    case WM_IME_COMPOSITION:
    case WM_IME_ENDCOMPOSITION:
    case WM_IME_STARTCOMPOSITION:
        return SendMessageToUI(pimeui, message, wParam, lParam, fAnsi);

    case WM_COPYDATA:
        return ImeCopyDataHandler(wParam, lParam);

    default:
CallDWP:
        return DefWindowProcWorker(pwnd, message, wParam, lParam, fAnsi);
    }

    return 0L;
}


LRESULT WINAPI ImeWndProcA(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    PWND pwnd;

    if ((pwnd = ValidateHwnd(hwnd)) == NULL) {
        return (0L);
    }

    return ImeWndProcWorker(pwnd, message, wParam, lParam, TRUE);
}


LRESULT WINAPI ImeWndProcW(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    PWND pwnd;

    if ((pwnd = ValidateHwnd(hwnd)) == NULL) {
        return (0L);
    }

    return ImeWndProcWorker(pwnd, message, wParam, lParam, FALSE);
}


LONG ImeWndCreateHandler(
    PIMEUI pimeui,
    LPCREATESTRUCT lpcs)
{
    PWND pwndParent;
    HIMC hImc;
    PWND pwndIme = pimeui->spwnd;
#if _DBG
    static DWORD dwFirstWinlogonThreadId;
#endif
    extern BOOL gfLogonProcess;

#if _DBG
    /*
     * For Winlogon, only the first thread can have IME processing.
     */
    if (gfLogonProcess) {
        UserAssert(dwFirstWinLogonThreadId == 0);
        dwFirstWinlogonThreadId = GetCurrentThreadId();
    }
#endif

    if (!TestWF(pwndIme, WFPOPUP) || !TestWF(pwndIme, WFDISABLED)) {
        RIPMSG0(RIP_WARNING, "IME window should have WS_POPUP and WS_DISABLE!!");
        return -1L;
    }

    /*
     * Check with parent window, if exists, try to get IMC.
     * If this is top level window, wait for first WM_IME_SETCONTEXT.
     */
    if ((pwndParent = ValidateHwndNoRip(lpcs->hwndParent)) != NULL) {
        hImc = pwndParent->hImc;
        if (hImc != NULL_HIMC && ImeIsUsableContext(HWq(pwndIme), hImc)) {
            /*
             * Store it for later use.
             */
            SETIMC(pimeui, hImc);
        }
        else {
            SETIMC(pimeui, NULL_HIMC);
        }
    }
    else {
        SETIMC(pimeui, NULL_HIMC);
    }

    /*
     * Initialize status window show state
     * The status window is not open yet.
     */
    pimeui->fShowStatus = 0;
    pimeui->nCntInIMEProc = 0;
    pimeui->fActivate = 0;
    pimeui->fDestroy = 0;
    pimeui->hwndIMC = NULL;
    pimeui->hKL = THREAD_HKL();
    pimeui->fCtrlShowStatus = TRUE;

    /*
     * Load up the IME DLL of current keyboard layout.
     */
    fpImmLoadIME(pimeui->hKL);

#ifdef LATE_CREATEUI
    SETUI(pimeui, NULL);
#else
    SETUI(pimeui, CreateIMEUI(pimeui, pimeui->hKL));
#endif

    return 0L;
}

void ImeWndDestroyHandler(
    PIMEUI pimeui)
{
    DestroyIMEUI(pimeui);
}


/***************************************************************************\
* ImeRunHelp
*
* Display Help file (HLP and CHM).
*
* History:
* 27-Oct-98 Hiroyama
\***************************************************************************/

void ImeRunHelp(LPWSTR wszHelpFile)
{
    static const WCHAR wszHelpFileExt[] = L".HLP";
    UINT cchLen = wcslen(wszHelpFile);

    if (cchLen > 4 && _wcsicmp(wszHelpFile + cchLen - 4, wszHelpFileExt) == 0) {
#ifdef FYI
        WinHelpW(NULL, wszHelpFile, HELP_CONTENTS, 0);
#else
        WinHelpW(NULL, wszHelpFile, HELP_FINDER, 0);
#endif
    } else {
        //
        // If it's not HLP file, try to run hh.exe, HTML based
        // help tool. It should be in %windir%\hh.exe.
        //
        static const WCHAR wszHH[] = L"hh.exe ";
        WCHAR wszCmdLine[MAX_PATH * 2];
        LPWSTR lpwszCmdLine = wszCmdLine;
        DWORD               idProcess;
        STARTUPINFO         StartupInfo;
        PROCESS_INFORMATION ProcessInformation;
        UINT i;

        i = GetSystemWindowsDirectoryW(wszCmdLine, MAX_PATH);
        if (i > 0 && i < MAX_PATH - cchLen - (sizeof L'\\' + sizeof wszHH) / sizeof(WCHAR)) {
            lpwszCmdLine += i;
            if (lpwszCmdLine[-1] != L'\\') {
                *lpwszCmdLine++ = L'\\';
            }
        }
        wcscpy(lpwszCmdLine, wszHH);
        wcscat(lpwszCmdLine, wszHelpFile);

        /*
         *  Launch HTML Help.
         */
        RtlZeroMemory(&StartupInfo, sizeof(StartupInfo));
        StartupInfo.cb = sizeof(StartupInfo);
        StartupInfo.wShowWindow = SW_SHOW;
        StartupInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_FORCEONFEEDBACK;

        TAGMSG1(DBGTAG_IMM, "Invoking help with '%S'", wszCmdLine);

        idProcess = (DWORD)CreateProcessW(NULL, wszCmdLine,
                NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &StartupInfo,
                &ProcessInformation);

        if (idProcess) {
            WaitForInputIdle(ProcessInformation.hProcess, 10000);
            NtClose(ProcessInformation.hProcess);
            NtClose(ProcessInformation.hThread);
        }
    }
}

LRESULT ImeSystemHandler(
    PIMEUI pimeui,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam)
{
    PINPUTCONTEXT pInputContext;
    HIMC          hImc = GETIMC(pimeui);
    LRESULT       dwRet = 0L;

    UNREFERENCED_PARAMETER(message);

    switch (wParam) {

    case IMS_SETOPENCLOSE:
        if (hImc != NULL_HIMC)
            fpImmSetOpenStatus(hImc, (BOOL)lParam);
        break;

#ifdef LATER
    case IMS_WINDOWPOS:
        if (hImc != NULL_HIMC) {
            INT  i;
            BOOL f31Hidden = FALSE:

            if ((pInputContext = fpImmLockIMC(hImc)) != NULL) {
                f31Hidden = (pInputContext & F31COMPAT_MCWHIDDEN) ? TRUE : FALSE;
                fpImmUnlockIMC(hImc);
            }

            if (IsWindow(pimeui->hwndIMC)) {
                if (!f31Hidden) {
                }
            }
        }
#endif

    case IMS_ACTIVATECONTEXT:
        FocusSetIMCContext((HWND)(lParam), TRUE);
        break;

    case IMS_DEACTIVATECONTEXT:
        FocusSetIMCContext((HWND)(lParam), FALSE);
        break;

    case IMS_UNLOADTHREADLAYOUT:
        return (LONG)(fpImmFreeLayout((DWORD)lParam));

    case IMS_ACTIVATETHREADLAYOUT:
        return (LONG)(fpImmActivateLayout((HKL)lParam));

    case IMS_SETCANDIDATEPOS:
        if ( (pInputContext = fpImmLockIMC( hImc )) != NULL ) {
            LPCANDIDATEFORM lpcaf;
            DWORD dwIndex = (DWORD)lParam;

            lpcaf = &(pInputContext->cfCandForm[dwIndex]);
            fpImmSetCandidateWindow( hImc, lpcaf );
            fpImmUnlockIMC( hImc );
        }
        break;

    case IMS_SETCOMPOSITIONWINDOW:
        if ( (pInputContext = fpImmLockIMC( hImc )) != NULL ) {
            LPCOMPOSITIONFORM lpcof;

            lpcof = &(pInputContext->cfCompForm);
            pInputContext->fdw31Compat |= F31COMPAT_CALLFROMWINNLS;
            fpImmSetCompositionWindow( hImc, lpcof);
        }
        break;

    case IMS_SETCOMPOSITIONFONT:
        if ( (pInputContext = fpImmLockIMC( hImc )) != NULL ) {
            LPLOGFONT lplf;

            lplf = &(pInputContext->lfFont.W);
            fpImmSetCompositionFont( hImc, lplf );
        }
        break;

    case IMS_CONFIGUREIME:
        fpImmConfigureIMEW( (HKL)lParam, pimeui->hwndIMC, IME_CONFIG_GENERAL, NULL);
        break;

    case IMS_CHANGE_SHOWSTAT:
        // Private message from internat.exe
        // Before it reaches here, the registry is already updated.
        if (GetIMEShowStatus() == !lParam) {
#if 1
            NtUserBroadcastImeShowStatusChange(HW(pimeui->spwnd), !!lParam);
#else
            SystemParametersInfo(SPI_SETSHOWIMEUI, lParam, NULL, FALSE);
#endif
        }
        break;

    case IMS_GETCONVERSIONMODE:
    {
        DWORD dwConv = 0;
        DWORD dwTemp;

        fpImmGetConversionStatus(hImc, &dwConv, &dwTemp);
        return (dwConv);
        break;
    }

    case IMS_SETSOFTKBDONOFF:
        fpImmEnumInputContext(0, SyncSoftKbdState, lParam);
        break;

    case IMS_GETIMEMENU:
        // new in NT50
        // IMS_GETIEMMENU is used to handle Inter Process GetMenu.
        // NOTE: This operation is only intended to internat.exe
        return fpImmPutImeMenuItemsIntoMappedFile((HIMC)lParam);

    case IMS_IMEHELP:
        dwRet = IME_ESC_GETHELPFILENAME;
        dwRet = fpImmEscapeW(pimeui->hKL, pimeui->hIMC, IME_ESC_QUERY_SUPPORT, (LPVOID)&dwRet);
        if (lParam) {
            // try to run WinHelp
            WCHAR wszHelpFile[MAX_PATH];

            if (dwRet) {
                if (fpImmEscapeW(pimeui->hKL, pimeui->hIMC, IME_ESC_GETHELPFILENAME,
                        (LPVOID)wszHelpFile)) {
                    ImeRunHelp(wszHelpFile);
                }
            }
        }
        return dwRet;

    case IMS_GETCONTEXT:
        dwRet = (ULONG_PTR)fpImmGetContext((HWND)lParam);
        return dwRet;

    case IMS_ENDIMEMENU:
        // New in NT5.0: Special support for Internat.exe
        if (IsWindow((HWND)lParam)) {
            HIMC hImc;
            UINT uID;
            DWORD dwData;

            hImc = fpImmGetContext((HWND)lParam);

            if (hImc != NULL) {
                //
                // Call Indicator to get IME menu data.
                //
                if (IMEIndicatorGetMenuIDData(&uID, &dwData)) {
                    fpImmNotifyIME(hImc, NI_IMEMENUSELECTED, uID, dwData);
                }
                fpImmReleaseContext((HWND)lParam, hImc);
            }
        }
        break;

    case IMS_SENDNOTIFICATION:
    case IMS_FINALIZE_COMPSTR:
        dwRet = fpImmSystemHandler(hImc, wParam, lParam);
        break;

    default:
        break;
    }

    return dwRet;
}


LONG ImeSelectHandler(
    PIMEUI pimeui,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam)
{
    HWND hwndUI;

    /*
     * Deliver this message to other IME windows in this thread.
     */
    if (pimeui->fDefault)
        ImeBroadCastMsg(pimeui, message, wParam, lParam);

    /*
     * We must re-create UI window of newly selected IME.
     */
    if ((BOOL)wParam == TRUE) {
        UserAssert(!IsWindow(GETUI(pimeui)));

        SETHKL(pimeui, (HKL)lParam);

#ifdef LATE_CREATEUI
        if (!pimeui->fActivate)
            return 0L;
#endif

        hwndUI = CreateIMEUI(pimeui, (HKL)lParam);

        SETUI(pimeui, hwndUI);

        if (hwndUI != NULL) {
            SetWindowLongPtr(hwndUI, IMMGWLP_IMC, (LONG_PTR)GETIMC(pimeui));
            SendMessageToUI(pimeui, message, wParam, lParam, FALSE);
        }

        if (GetIMEShowStatus() && pimeui->fCtrlShowStatus) {
            if (!pimeui->fShowStatus && pimeui->fActivate &&
                    IsWindow(pimeui->hwndIMC)) {
                /*
                 * This must be sent to an application as an app may want
                 * to hook this message to do its own UI.
                 */
                SendOpenStatusNotify(pimeui, pimeui->hwndIMC, TRUE);
            }
        }
    }
    else {

        if (pimeui->fShowStatus && pimeui->fActivate &&
                IsWindow(pimeui->hwndIMC)) {
            /*
             * This must be sent to an application as an app may want
             * to hook this message to do its own UI.
             */
            SendOpenStatusNotify(pimeui, pimeui->hwndIMC, FALSE);
        }

        SendMessageToUI(pimeui, message, wParam, lParam, FALSE);

        DestroyIMEUI(pimeui);

        SETHKL(pimeui, (HKL)NULL);
    }

    return 0L;
}


LRESULT ImeControlHandler(
    PIMEUI pimeui,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam,
    BOOL   fAnsi)
{
    HIMC  hImc;
    DWORD dwConversion, dwSentence;

    /*
     * Do nothing with NULL hImc.
     */
    if ((hImc = GETIMC(pimeui)) == NULL_HIMC)
        return 0L;

    switch (wParam) {

    case IMC_OPENSTATUSWINDOW:
        if (GetIMEShowStatus() && !pimeui->fShowStatus) {
            pimeui->fShowStatus = TRUE;
            SendMessageToUI(pimeui, WM_IME_NOTIFY,
                    IMN_OPENSTATUSWINDOW, 0L, FALSE);
        }
        pimeui->fCtrlShowStatus = TRUE;
        break;

    case IMC_CLOSESTATUSWINDOW:
        if (GetIMEShowStatus() && pimeui->fShowStatus) {
            pimeui->fShowStatus = FALSE;
            SendMessageToUI(pimeui, WM_IME_NOTIFY,
                    IMN_CLOSESTATUSWINDOW, 0L, FALSE);
        }
        pimeui->fCtrlShowStatus = FALSE;
        break;

    /*
     * ------------------------------------------------
     * IMC_SETCOMPOSITIONFONT,
     * IMC_SETCONVERSIONMODE,
     * IMC_SETOPENSTATUS
     * ------------------------------------------------
     * Don't pass these WM_IME_CONTROLs to UI window.
     * Call Imm in order to process these requests instead.
     * It makes message flows simpler.
     */
    case IMC_SETCOMPOSITIONFONT:
        if (fAnsi) {
            if (!fpImmSetCompositionFontA(hImc, (LPLOGFONTA)lParam))
                return 1L;
        }
        else {
            if (!fpImmSetCompositionFontW(hImc, (LPLOGFONTW)lParam))
                return 1L;
        }
        break;

    case IMC_SETCONVERSIONMODE:
        if (!fpImmGetConversionStatus(hImc, &dwConversion, &dwSentence) ||
            !fpImmSetConversionStatus(hImc, (DWORD)lParam, dwSentence))
            return 1L;
        break;

    case IMC_SETSENTENCEMODE:
        if (!fpImmGetConversionStatus(hImc, &dwConversion, &dwSentence) ||
            !fpImmSetConversionStatus(hImc, dwConversion, (DWORD)lParam))
            return 1L;
        break;

    case IMC_SETOPENSTATUS:
        if (!fpImmSetOpenStatus(hImc, (BOOL)lParam))
            return 1L;
        break;

    case IMC_GETCONVERSIONMODE:
        if (!fpImmGetConversionStatus(hImc,&dwConversion, &dwSentence))
            return 1L;

        return (LONG)dwConversion;

    case IMC_GETSENTENCEMODE:
        if (!fpImmGetConversionStatus(hImc,&dwConversion, &dwSentence))
            return 1L;

        return (LONG)dwSentence;

    case IMC_GETOPENSTATUS:
        return (LONG)fpImmGetOpenStatus(hImc);

    case IMC_GETCOMPOSITIONFONT:
        if (fAnsi) {
            if (!fpImmGetCompositionFontA(hImc, (LPLOGFONTA)lParam))
                return 1L;
        }
        else {
            if (!fpImmGetCompositionFontW(hImc, (LPLOGFONTW)lParam))
                return 1L;
        }
        break;

    case IMC_SETCOMPOSITIONWINDOW:
        if (!fpImmSetCompositionWindow(hImc, (LPCOMPOSITIONFORM)lParam))
            return 1L;
        break;

    case IMC_SETSTATUSWINDOWPOS:
        {
            POINT ppt;

            ppt.x = (LONG)((LPPOINTS)&lParam)->x;
            ppt.y = (LONG)((LPPOINTS)&lParam)->y;

            if (!fpImmSetStatusWindowPos(hImc, &ppt))
                return 1L;
        }
        break;

    case IMC_SETCANDIDATEPOS:
        if (!fpImmSetCandidateWindow(hImc, (LPCANDIDATEFORM)lParam))
            return 1;
        break;

    /*
     * Followings are the messsages to be sent to UI.
     */
    case IMC_GETCANDIDATEPOS:
    case IMC_GETSTATUSWINDOWPOS:
    case IMC_GETCOMPOSITIONWINDOW:
    case IMC_GETSOFTKBDPOS:
    case IMC_SETSOFTKBDPOS:
        return SendMessageToUI(pimeui, message, wParam, lParam, fAnsi);

    default:
        break;
    }

    return 0L;
}



LRESULT ImeSetContextHandler(
    PIMEUI pimeui,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam)
{
    HWND  hwndPrevIMC, hwndFocus;   // focus window in the thread
    HIMC  hFocusImc;                // focus window's IMC
    LRESULT lRet;

    pimeui->fActivate = (BOOL)wParam ? 1 : 0;
    hwndPrevIMC = pimeui->hwndIMC;

    if (wParam) {
        /*
         * if it's being activated
         */
#ifdef LATE_CREATEUI
        if (!GETUI(pimeui))
            SETUI(pimeui, CreateIMEUI(pimeui, GETHKL(pimeui)));
#endif

        /*
         * Check if this process a console IME ?
         */
        if (gfConIme == UNKNOWN_CONIME) {
            gfConIme = (DWORD)NtUserGetThreadState(UserThreadStateIsConImeThread);
            if (gfConIme) {
                RIPMSG0(RIP_VERBOSE, "ImmSetContextHandler: This thread is console IME.\n");
                UserAssert(pimeui);
                // Console IME will never show the IME status window.
                pimeui->fCtrlShowStatus = FALSE;
            }
        }

        if (gfConIme) {
            /*
             * Special handling for Console IME is needed
             */
            PWND pwndOwner;

            UserAssert(pimeui->spwnd);
            pwndOwner = REBASEPWND(pimeui->spwnd, spwndOwner);
            if (pwndOwner != NULL) {
                /*
                 * Set current associated hIMC in IMEUI.
                 */
                SETIMC(pimeui, pwndOwner->hImc);
                /*
                 * Store it to the window memory.
                 */
                if (GETUI(pimeui) != NULL)
                    SetWindowLongPtr(GETUI(pimeui), IMMGWLP_IMC, (LONG_PTR)pwndOwner->hImc);
            }

            hwndFocus = NtUserQueryWindow(HW(pimeui->spwnd), WindowFocusWindow);
            hFocusImc = pwndOwner->hImc;
            RIPMSG2(RIP_VERBOSE, "CONSOLE IME: hwndFocus = %x, hFocusImc = %x", hwndFocus, hFocusImc);

            return SendMessageToUI(pimeui, message, wParam, lParam, FALSE);
        }
        else {
            hwndFocus = NtUserQueryWindow(HW(pimeui->spwnd), WindowFocusWindow);
            hFocusImc = fpImmGetContext(hwndFocus);
        }

        /*
         * Cannot share input context with other IME window.
         */
        if (hFocusImc != NULL_HIMC &&
                !ImeIsUsableContext(HW(pimeui->spwnd), hFocusImc)) {
            SETIMC(pimeui, NULL_HIMC);
            return 0L;
        }

        SETIMC(pimeui, hFocusImc);

        /*
         * Store it to the window memory.
         */
        if (GETUI(pimeui) != NULL)
            SetWindowLongPtr(GETUI(pimeui), IMMGWLP_IMC, (LONG_PTR)hFocusImc);

        /*
         * When we're receiving context,
         * it is necessary to set the owner to this window.
         * This is for:
         *     Give the UI moving information.
         *     Give the UI automatic Z-ordering.
         *     Hide the UI when the owner is minimized.
         */
        if (hFocusImc != NULL_HIMC) {
            PINPUTCONTEXT pInputContext;

            /*
             * Get the window who's given the context.
             */
            if ((pInputContext = fpImmLockIMC(hFocusImc)) != NULL) {
                //UserAssert(hwndFocus == pInputContext->hWnd);
                if (hwndFocus != pInputContext->hWnd) {
                    /*
                     * Pq->spwndFocus has been changed so far...
                     * All we can do is just to bail out.
                     */
                    return 0L;
                }
            }
            else
                return 0L; // the context was broken

            if ((pInputContext->fdw31Compat & F31COMPAT_ECSETCFS) &&
                    hwndPrevIMC != hwndFocus) {
                COMPOSITIONFORM cf;

                /*
                 * Set CFS_DEFAULT....
                 */
                RtlZeroMemory(&cf, sizeof(cf));
                fpImmSetCompositionWindow(hFocusImc, &cf);
                pInputContext->fdw31Compat &= ~F31COMPAT_ECSETCFS;
            }

            fpImmUnlockIMC(hFocusImc);

            if (NtUserSetImeOwnerWindow(HW(pimeui->spwnd), hwndFocus))
                pimeui->hwndIMC = hwndFocus;

        }
        else {
            /*
             * NULL IMC is getting activated
             */
            pimeui->hwndIMC = hwndFocus;

            NtUserSetImeOwnerWindow(HW(pimeui->spwnd), NULL);

        }
    }

    lRet = SendMessageToUI(pimeui, message, wParam, lParam, FALSE);

    if (pimeui->spwnd == NULL) {
        // Unusual case in stress..
        // IME window has been destroyed during the callback
        RIPMSG0(RIP_WARNING, "ImmSetContextHandler: pimeui->spwnd is NULL after SendMessageToUI.");
        return 0L;
    }

    if (pimeui->fCtrlShowStatus && GetIMEShowStatus()) {
        PWND pwndFocus, pwndIMC, pwndPrevIMC;
        HWND hwndActive;

        hwndFocus = NtUserQueryWindow(HWq(pimeui->spwnd), WindowFocusWindow);
        pwndFocus = ValidateHwndNoRip(hwndFocus);

        if ((BOOL)wParam == TRUE) {
            HWND hwndIme;

            /*
             * BOGUS BOGUS
             * The following if statement is still insufficient
             * it needs to think what WM_IME_SETCONTEXT:TRUE should do
             * in the case of WINNLSEnableIME(true) - ref.win95c B#8548.
             */
            UserAssert(pimeui->spwnd);
            if (pwndFocus != NULL && GETPTI(pimeui->spwnd) == GETPTI(pwndFocus)) {

                if (!pimeui->fShowStatus) {
                    /*
                     * We have never sent IMN_OPENSTATUSWINDOW yet....
                     */
                    if (ValidateHwndNoRip(pimeui->hwndIMC)) {
                        SendOpenStatusNotify(pimeui, pimeui->hwndIMC, TRUE);
                    }
                }
                else if ((pwndIMC = ValidateHwndNoRip(pimeui->hwndIMC)) != NULL &&
                         (pwndPrevIMC = ValidateHwndNoRip(hwndPrevIMC)) != NULL &&
                         GetTopLevelWindow(pwndIMC) != GetTopLevelWindow(pwndPrevIMC)) {
                    /*
                     * Because the top level window of IME Wnd was changed.
                     */
                    SendOpenStatusNotify(pimeui, hwndPrevIMC, FALSE);
                    SendOpenStatusNotify(pimeui, pimeui->hwndIMC, TRUE);
                }
            }
            /*
             * There may have other IME windows that have fShowStatus.
             * We need to check the fShowStatus in the window list.
             */
            hwndIme = HW(pimeui->spwnd);
            if (hwndIme) {
                NtUserCheckImeShowStatusInThread(hwndIme);
            }
        }
        else {
            /*
             * When focus was removed from this thread, we close the
             * status window.
             * Because focus was already removed from whndPrevIMC,
             * hwndPrevIMC may be destroyed but we need to close the
             * status window.
             */
            hwndActive = NtUserQueryWindow(HW(pimeui->spwnd), WindowActiveWindow);
            UserAssert(pimeui->spwnd);
            if (pwndFocus == NULL || GETPTI(pimeui->spwnd) != GETPTI(pwndFocus) ||
                    hwndActive == NULL) {

                if (IsWindow(hwndPrevIMC))
                    SendOpenStatusNotify(pimeui, hwndPrevIMC, FALSE);
                else {
                    pimeui->fShowStatus = FALSE;
                    SendMessageToUI(pimeui, WM_IME_NOTIFY,
                            IMN_CLOSESTATUSWINDOW, 0L, FALSE);
                }
            }
        }
    }

    return lRet;
}


LRESULT ImeNotifyHandler(
    PIMEUI pimeui,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam)
{
    HWND hwndUI;
    LRESULT lRet = 0L;
    HIMC hImc;
    PINPUTCONTEXT pInputContext;

    switch (wParam) {
    case IMN_PRIVATE:
        hwndUI = GETUI(pimeui);
        if (IsWindow(hwndUI))
            lRet = SendMessage(hwndUI, message, wParam, lParam);
        break;

    case IMN_SETCONVERSIONMODE:
    case IMN_SETOPENSTATUS:
        //
        // notify shell and keyboard the conversion mode change
        //
        // if this message is sent from ImmSetOpenStatus or
        // ImmSetConversionStatus, we have already notified
        // kernel the change. This is a little bit redundant.
        //
        // if application has eaten the message, we won't be here.
        // We need to think about the possibility later.
        //
        hImc = GETIMC(pimeui);
        if ((pInputContext = fpImmLockIMC(hImc)) != NULL) {
            if ( IsWindow(pimeui->hwndIMC) ) {
                NtUserNotifyIMEStatus( pimeui->hwndIMC,
                                       (DWORD)pInputContext->fOpen,
                                       pInputContext->fdwConversion );
            }
            else if (gfConIme == TRUE) {
                /*
                 * Special handling for Console IME is needed
                 */
                if (pimeui->spwnd) {    // If IME window is still there.
                    PWND pwndOwner = REBASEPWND(pimeui->spwnd, spwndOwner);

                    if (pwndOwner != NULL) {
                        NtUserNotifyIMEStatus(HWq(pwndOwner),
                                              (DWORD)pInputContext->fOpen,
                                              pInputContext->fdwConversion);
                    }
                }
            }
            fpImmUnlockIMC(hImc);
        }
        /*** FALL THROUGH ***/
    default:
        TAGMSG4(DBGTAG_IMM, "ImeNotifyHandler: sending to pimeui->ui=%p, msg=%x, wParam=%x, lParam=%x\n", GETUI(pimeui), message, wParam, lParam);
        lRet = SendMessageToUI(pimeui, message, wParam, lParam, FALSE);
    }

    return lRet;
}


HWND CreateIMEUI(
    PIMEUI pimeui,
    HKL    hKL)
{
    PWND      pwndIme = pimeui->spwnd;
    HWND      hwndUI;
    IMEINFOEX iiex;
    PIMEDPI   pimedpi;
    WNDCLASS  wndcls;

    if (!fpImmGetImeInfoEx(&iiex, ImeInfoExKeyboardLayout, &hKL))
        return (HWND)NULL;

    if ((pimedpi = fpImmLockImeDpi(hKL)) == NULL) {
        RIPMSG1(RIP_WARNING, "CreateIMEUI: ImmLockImeDpi(%lx) failed.", hKL);
        return (HWND)NULL;
    }

    if (!GetClassInfoW(pimedpi->hInst, iiex.wszUIClass, &wndcls)) {
        RIPMSG1(RIP_WARNING, "CreateIMEUI: GetClassInfoW(%ws) failed\n", iiex.wszUIClass);
        fpImmUnlockImeDpi(pimedpi);
        return (HWND)NULL;
    }

    // HACK HACK HACK
    if ((wndcls.style & CS_IME) == 0) {
        RIPMSG1(RIP_ERROR, "CreateIMEUI: the Window Class (%S) does not have CS_IME flag on !!!\n",
                wndcls.lpszClassName);
    }

    if (iiex.ImeInfo.fdwProperty & IME_PROP_UNICODE) {
        /*
         * For Unicode IME, we create an Unicode IME UI window.
         */
        hwndUI = CreateWindowExW(0L,
                        iiex.wszUIClass,
                        iiex.wszUIClass,
                        WS_POPUP|WS_DISABLED,
                        0, 0, 0, 0,
                        HWq(pwndIme), 0, wndcls.hInstance, NULL);
    }
    else {
        /*
         * For ANSI IME, we create an ANSI IME UI window.
         */

        LPSTR pszClass;
        int i;
        i = WCSToMB(iiex.wszUIClass, -1, &pszClass, -1, TRUE);
        if (i == 0) {
            RIPMSG1(RIP_WARNING, "CreateIMEUI: failed in W->A conversion (%S)", iiex.wszUIClass);
            return (HWND)NULL;
        }
        pszClass[i] = '\0';

        hwndUI = CreateWindowExA(0L,
                        pszClass,
                        pszClass,
                        WS_POPUP|WS_DISABLED,
                        0, 0, 0, 0,
                        HWq(pwndIme), 0, wndcls.hInstance, NULL);

        UserLocalFree(pszClass);
    }

    if (hwndUI)
        NtUserSetWindowLongPtr(hwndUI, IMMGWLP_IMC, (LONG_PTR)GETIMC(pimeui), FALSE);

    fpImmUnlockImeDpi(pimedpi);

    return hwndUI;
}


VOID DestroyIMEUI(
    PIMEUI pimeui)
{
    // This has currently nothing to do except for destroying the UI.
    // Review: Need to notify the UI with WM_IME_SETCONTEXT ?
    // Review: This doesn't support Multiple IME install yet.

    HWND hwndUI = GETUI(pimeui);

    if (IsWindow(hwndUI)) {
        pimeui->fDestroy = TRUE;
        /*
         * We need this verify because the owner might have already
         * killed it during its termination.
         */
        NtUserDestroyWindow(hwndUI);
    }
    pimeui->fDestroy = FALSE;

    /*
     * Reinitialize show status of the IME status window so that
     * notification message will be sent when needed.
     */
    pimeui->fShowStatus = FALSE;

    SETUI(pimeui, NULL);

    return;
}


/***************************************************************************\
* SendMessageToUI
*
* History:
* 09-Apr-1996 wkwok       Created
\***************************************************************************/

LRESULT SendMessageToUI(
    PIMEUI pimeui,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam,
    BOOL   fAnsi)
{
    PWND  pwndUI;
    LRESULT lRet;

    TAGMSG1(DBGTAG_IMM, "Sending to UI msg[%04X]\n", message);

    pwndUI = ValidateHwndNoRip(GETUI(pimeui));

    if (pwndUI == NULL || pimeui->spwnd == NULL)
        return 0L;

    if (TestWF(pimeui->spwnd, WFINDESTROY) || TestWF(pimeui->spwnd, WFDESTROYED) ||
            TestWF(pwndUI, WFINDESTROY) || TestWF(pwndUI, WFDESTROYED)) {
        return 0L;
    }

    InterlockedIncrement(&pimeui->nCntInIMEProc); // Mark to avoid recursion.

    lRet = SendMessageWorker(pwndUI, message, wParam, lParam, fAnsi);

    InterlockedDecrement(&pimeui->nCntInIMEProc); // Mark to avoid recursion.

    return lRet;
}


/***************************************************************************\
* SendOpenStatusNotify
*
* History:
* 09-Apr-1996 wkwok       Created
\***************************************************************************/

VOID SendOpenStatusNotify(
    PIMEUI pimeui,
    HWND   hwndApp,
    BOOL   fOpen)
{
    WPARAM wParam = fOpen ? IMN_OPENSTATUSWINDOW : IMN_CLOSESTATUSWINDOW;

    pimeui->fShowStatus = fOpen;


    if (GetClientInfo()->dwExpWinVer >= VER40) {
        TAGMSG2(DBGTAG_IMM, "SendOpenStatusNotify: sending to hwnd=%lx, wParam=%d\n", hwndApp, wParam);
        SendMessage(hwndApp, WM_IME_NOTIFY, wParam, 0L);
    }
    else {
        TAGMSG2(DBGTAG_IMM, "SendOpenStatusNotify:sending to imeui->UI=%p, wParam=%d\n", GETUI(pimeui), wParam);
        SendMessageToUI(pimeui, WM_IME_NOTIFY, wParam, 0L, FALSE);
    }

    return;
}


VOID ImeSetImc(
    PIMEUI pimeui,
    HIMC hImc)
{
    HWND hImeWnd = HW(pimeui->spwnd);
    HIMC hOldImc = GETIMC(pimeui);

    /*
     * return if nothing to change.
     */
    if (hImc == hOldImc)
        return;

    /*
     * Unmark the old input context.
     */
    if (hOldImc != NULL_HIMC)
        ImeMarkUsedContext(NULL, hOldImc);

    /*
     * Update the in use input context for this IME window.
     */
    pimeui->hIMC = hImc;

    /*
     * Mark the new input context.
     */
    if (hImc != NULL_HIMC)
        ImeMarkUsedContext(hImeWnd, hImc);
}


/***************************************************************************\
*  FocusSetIMCContext()
*
* History:
* 21-Mar-1996 wkwok       Created
\***************************************************************************/

VOID FocusSetIMCContext(
    HWND hWnd,
    BOOL fActivate)
{
    HIMC hImc;

    if (IsWindow(hWnd)) {
        hImc = fpImmGetContext(hWnd);
        fpImmSetActiveContext(hWnd, hImc, fActivate);
        fpImmReleaseContext(hWnd, hImc);
    }
    else {
        fpImmSetActiveContext(NULL, NULL_HIMC, fActivate);
    }

    return;
}


BOOL ImeBroadCastMsg(
    PIMEUI pimeui,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam)
{
    UNREFERENCED_PARAMETER(pimeui);
    UNREFERENCED_PARAMETER(message);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    return TRUE;
}

/***************************************************************************\
*  ImeMarkUsedContext()
*
*  Some IME windows can not share the same input context. This function
*  marks the specified hImc to be in used by the specified IME window.
*
* History:
* 12-Mar-1996 wkwok       Created
\***************************************************************************/

VOID ImeMarkUsedContext(
    HWND hImeWnd,
    HIMC hImc)
{
    PIMC pImc;

    pImc = HMValidateHandle((HANDLE)hImc, TYPE_INPUTCONTEXT);
    if (pImc == NULL) {
        RIPMSG1(RIP_WARNING, "ImeMarkUsedContext: Invalid hImc (=%lx).", hImc);
        return;
    }

    UserAssert( ValidateHwndNoRip(pImc->hImeWnd) == NULL || hImeWnd == NULL );

    /*
     * Nothing to change?
     */
    if (pImc->hImeWnd == hImeWnd)
        return;

    NtUserUpdateInputContext(hImc, UpdateInUseImeWindow, (ULONG_PTR)hImeWnd);

    return;
}


/***************************************************************************\
*  ImeIsUsableContext()
*
*  Some IME windows can not share the same input context. This function
*  checks whether the specified hImc can be used (means 'Set activated')
*  by the specified IME window.
*
*  Return: TRUE  - OK to use the hImc by hImeWnd.
*          FALSE - otherwise.
*
* History:
* 12-Mar-1996 wkwok       Created
\***************************************************************************/

BOOL ImeIsUsableContext(
    HWND hImeWnd,
    HIMC hImc)
{
    PIMC pImc;

    UserAssert(hImeWnd != NULL);

    pImc = HMValidateHandle((HANDLE)hImc, TYPE_INPUTCONTEXT);
    if (pImc == NULL) {
        RIPMSG1(RIP_WARNING, "ImeIsUsableContext: Invalid hImc (=%lx).", hImc);
        return FALSE;
    }

    if ( pImc->hImeWnd == NULL     ||
         pImc->hImeWnd == hImeWnd  ||
         ValidateHwndNoRip(pImc->hImeWnd) == NULL )
    {
        return TRUE;
    }


    return FALSE;
}

/***************************************************************************\
* GetIMEShowStatus()
*
* Get the global IME show status from kernel.
*
* History:
* 19-Sep-1996 takaok       Ported from internat.exe.
\***************************************************************************/

BOOL GetIMEShowStatus(void)
{
    return (BOOL)NtUserCallNoParam(SFI__GETIMESHOWSTATUS);
}



/***************************************************************************\
*  IMEIndicatorGetMenuIDData
*
* History:
* 3-Nov-97 Hiroyama
\***************************************************************************/

BOOL IMEIndicatorGetMenuIDData(PUINT puMenuID, PDWORD pdwData)
{
    HANDLE hinstIndic;

    hinstIndic = GetModuleHandle(szIndicDLL);
    if (hinstIndic == NULL) {
        gpfnGetIMEMenuItemData = NULL;
        return FALSE;
    }

    if (!gpfnGetIMEMenuItemData) {
        gpfnGetIMEMenuItemData = GetProcAddress(hinstIndic, (LPSTR)ORD_GETIMEMENUITEMDATA);
    }
    if (!gpfnGetIMEMenuItemData)
        return FALSE;

    (*(FPGETIMEMENUITEMDATA)gpfnGetIMEMenuItemData)(puMenuID, pdwData);
    return TRUE;
}


BOOL SyncSoftKbdState(
    HIMC hImc,
    LPARAM lParam)
{
    DWORD fdwConversion, fdwSentence, fdwNewConversion;

    fpImmGetConversionStatus(hImc, &fdwConversion, &fdwSentence);

    if (lParam) {
        fdwNewConversion = fdwConversion | IME_CMODE_SOFTKBD;
    } else {
        fdwNewConversion = fdwConversion & ~IME_CMODE_SOFTKBD;
    }

    if (fdwNewConversion != fdwConversion) {
        fpImmSetConversionStatus(hImc, fdwNewConversion, fdwSentence);
    }

    return TRUE;
}

/***************************************************************************\
*  ImeCopyDataHandler
*
* History:
* 10-Nov-98 Hiroyama
\***************************************************************************/

LRESULT ImeCopyDataHandler(
    WPARAM wParam,
    LPARAM lParam)
{
    HINSTANCE hInstImm;
    BOOL (WINAPI* fpImmPenAuxInput)(HWND, LPVOID);

    hInstImm = GetModuleHandleW(L"IMM32.DLL");
    if (hInstImm == NULL) {
        return FALSE;
    }

    fpImmPenAuxInput = (LPVOID)GetProcAddress(hInstImm, "ImmPenAuxInput");
    if (fpImmPenAuxInput == NULL) {
        return FALSE;
    }

    return fpImmPenAuxInput((HWND)wParam, (LPVOID)lParam);
}
