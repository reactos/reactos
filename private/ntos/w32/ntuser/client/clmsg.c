/****************************** Module Header ******************************\
* Module Name: ClMsg.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Includes the mapping table for messages when calling the server.
*
* 04-11-91 ScottLu Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


#define fnINDESTROYCLIPBRD      fnDWORD
#define fnOUTDWORDDWORD         fnDWORD
#define fnPOWERBROADCAST        fnDWORD
#define fnLOGONNOTIFY           fnKERNELONLY
#define fnINLPKDRAWSWITCHWND    fnKERNELONLY

#define MSGFN(func) fn ## func
#define FNSCSENDMESSAGE CFNSCSENDMESSAGE

#include "messages.h"

#if DBG
BOOL gfTurboDWP = TRUE;
#endif

/***************************************************************************\
* GetMouseKeyState
*
* Returns the state of mouse and keyboard keys that are sent
* in a mouse message.
*
* History:
* 12-Nov-1998 adams     Created.
\***************************************************************************/

WORD
GetMouseKeyState(void)
{
    WORD keystate;

    /*
     * Note that it is more efficient to call GetKeyState for each
     * key than to call GetKeyboardState, since the keys we are testing
     * are cached and don't require a trip to the kernel to fetch.
     */

#define TESTANDSETKEYSTATE(x)            \
    if (GetKeyState(VK_##x) & 0x8000) {  \
        keystate |= MK_##x;              \
    }

    keystate = 0;
    TESTANDSETKEYSTATE(LBUTTON)
    TESTANDSETKEYSTATE(RBUTTON)
    TESTANDSETKEYSTATE(MBUTTON)
    TESTANDSETKEYSTATE(XBUTTON1)
    TESTANDSETKEYSTATE(XBUTTON2)
    TESTANDSETKEYSTATE(SHIFT)
    TESTANDSETKEYSTATE(CONTROL)

    return keystate;
}

/***************************************************************************\
* These are client side thunks for server side window procs. This is being
* done so that when an app gets a wndproc via GetWindowLong, GetClassLong,
* or GetClassInfo, it gets a real callable address - some apps don't call
* CallWindowProc, but call the return ed address directly.
*
* 01-13-92 ScottLu Created.
* 03-Dec-1993 mikeke  added client side handling of some messages
\***************************************************************************/

LRESULT WINAPI DesktopWndProcWorker(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fAnsi)
{
    PWND pwnd;

    if (FWINDOWMSG(message, FNID_DESKTOP)) {
        return CsSendMessage(hwnd, message, wParam, lParam,
                0L, FNID_DESKTOP, fAnsi);
    }

    if ((pwnd = ValidateHwnd(hwnd)) == NULL)
        return 0;

    return DefWindowProcWorker(pwnd, message, wParam, lParam, fAnsi);

}

LRESULT WINAPI DesktopWndProcA(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return DesktopWndProcWorker(hwnd, message, wParam, lParam, TRUE);
}

LRESULT WINAPI DesktopWndProcW(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return DesktopWndProcWorker(hwnd, message, wParam, lParam, FALSE);
}

/***************************************************************************\
* These are client side thunks for server side window procs. This is being
* done so that when an app gets a wndproc via GetWindowLong, GetClassLong,
* or GetClassInfo, it gets a real callable address - some apps don't call
* CallWindowProc, but call the return ed address directly.
*
* 01-13-92 ScottLu Created.
* 03-Dec-1993 mikeke  added client side handling of some messages
\***************************************************************************/

LRESULT WINAPI MenuWndProcWorker(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fAnsi)
{
    PWND pwnd;

    if (FWINDOWMSG(message, FNID_MENU)) {
        return CsSendMessage(hwnd, message, wParam, lParam,
                0L, FNID_MENU, fAnsi);
    }

    if ((pwnd = ValidateHwnd(hwnd)) == NULL)
        return 0;

    switch (message) {
    case WM_LBUTTONDBLCLK:
    case WM_NCLBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_NCRBUTTONDBLCLK:

        /*
         * Ignore double clicks on these windows.
         */
        break;

    case WM_DESTROY:
        break;

    default:
        return DefWindowProcWorker(pwnd, message, wParam, lParam, fAnsi);
    }

    return 0;
}

LRESULT WINAPI MenuWndProcA(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return MenuWndProcWorker(hwnd, message, wParam, lParam, TRUE);
}

LRESULT WINAPI MenuWndProcW(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return MenuWndProcWorker(hwnd, message, wParam, lParam, FALSE);
}

/***************************************************************************\
\***************************************************************************/


LRESULT WINAPI ScrollBarWndProcWorker(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fAnsi)
{
    PSBWND psbwnd;
    LPSCROLLINFO lpsi;
    PSBDATA pw;

    if (FWINDOWMSG(message, FNID_SCROLLBAR)) {
        return CsSendMessage(hwnd, message, wParam, lParam,
                0L, FNID_SCROLLBAR, fAnsi);
    }

    if ((psbwnd = (PSBWND)ValidateHwnd(hwnd)) == NULL)
        return 0;

    switch (message) {
    case WM_GETDLGCODE:
        return DLGC_WANTARROWS;

    case SBM_GETPOS:
        return (LONG)psbwnd->SBCalc.pos;

    case SBM_GETRANGE:
        *((LPINT)wParam) = psbwnd->SBCalc.posMin;
        *((LPINT)lParam) = psbwnd->SBCalc.posMax;
        return 0;

    case SBM_GETSCROLLINFO:
        lpsi = (LPSCROLLINFO)lParam;
        if ((lpsi->cbSize != sizeof(SCROLLINFO)) &&
            (lpsi->cbSize != sizeof(SCROLLINFO) - 4)) {
            RIPMSG0(RIP_ERROR, "SCROLLINFO: invalid cbSize");
            return FALSE;
        }

        if (lpsi->fMask & ~SIF_MASK)
        {
            RIPMSG0(RIP_ERROR, "SCROLLINFO: Invalid fMask");
            return FALSE;
        }

        pw = (PSBDATA)&(psbwnd->SBCalc);
        return(NtUserSBGetParms(hwnd, SB_CTL, pw, lpsi));

    default:
        return DefWindowProcWorker((PWND)psbwnd, message,
                wParam, lParam, fAnsi);
    }
}


LRESULT WINAPI ScrollBarWndProcA(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return ScrollBarWndProcWorker(hwnd, message, wParam, lParam, TRUE);
}

LRESULT WINAPI ScrollBarWndProcW(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return ScrollBarWndProcWorker(hwnd, message, wParam, lParam, FALSE);
}


/***************************************************************************\
* SendMessage
*
* Translates the message, calls SendMessage on server side.
*
* 04-11-91 ScottLu  Created.
* 04-27-92 DarrinM  Added code to support client-to-client SendMessages.
\***************************************************************************/

LRESULT SendMessageWorker(
    PWND pwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fAnsi)
{
    HWND hwnd = HWq(pwnd);
    PCLIENTINFO pci;
    PCLS pcls;
    BOOLEAN fAnsiRecv;
    BOOLEAN fNeedTranslation = FALSE;
    BOOLEAN bDoDbcsMessaging = FALSE;
    LRESULT lRet;

    UserAssert(pwnd);

    /*
     * Pass DDE messages to the server.
     */
    if (message >= WM_DDE_FIRST && message <= WM_DDE_LAST)
        goto lbServerSendMessage;

    /*
     * Server must handle inter-thread SendMessages and SendMessages
     * to server-side procs.
     */
    if ((PtiCurrent() != GETPTI(pwnd)) || TestWF(pwnd, WFSERVERSIDEPROC))
        goto lbServerSendMessage;

    /*
     * Server must handle hooks (at least for now).
     */
    pci = GetClientInfo();
    if (IsHooked(pci, (WHF_CALLWNDPROC | WHF_CALLWNDPROCRET))) {
lbServerSendMessage:
        return CsSendMessage(hwnd, message, wParam, lParam, 0L,
                FNID_SENDMESSAGE, fAnsi);
    }

    /*
     * If the sender and the receiver are both ANSI or both UNICODE
     * then no message translation is necessary.
     *
     * EditWndProc may need to go to the server for translation if we
     * are calling vanilla EditWndProc from SendMessageA and the edit
     * control is currently subclassed Ansi but the edit control is
     * stored Unicode.
     */
    fAnsiRecv = !!(TestWF(pwnd, WFANSIPROC));
    if (!fAnsi != !fAnsiRecv) {

        /*
         * Translation might be necessary between sender and receiver,
         * check to see if this is one of the messages we translate.
         */
        switch (message) {
        case WM_CHARTOITEM:
        case EM_SETPASSWORDCHAR:
        case WM_CHAR:
        case WM_DEADCHAR:
        case WM_SYSCHAR:
        case WM_SYSDEADCHAR:
        case WM_MENUCHAR:
        case WM_IME_CHAR:
        case WM_IME_COMPOSITION:
            if (fAnsi) {
                /*
                 * Setup DBCS Messaging for WM_CHAR...
                 */
                BUILD_DBCS_MESSAGE_TO_CLIENTW_FROM_CLIENTA(message,wParam,TRUE);

                /*
                 * Convert wParam to Unicode...
                 */
                RtlMBMessageWParamCharToWCS(message, &wParam);

                /*
                 * The message has been converted to Unicode.
                 */
                fAnsi = FALSE;
            } else {
                POINT ptZero = {0,0};
                /*
                 * Convert wParam to ANSI...
                 */
                RtlWCSMessageWParamCharToMB(message, &wParam);

                /*
                 * Let's DBCS messaging for WM_CHAR....
                 */
                BUILD_DBCS_MESSAGE_TO_CLIENTA_FROM_CLIENTW(
                    hwnd,message,wParam,lParam,0,ptZero,bDoDbcsMessaging);

                /*
                 * The message has been converted to ANSI.
                 */
                fAnsi = TRUE;
            }
            break;

        case EM_SETSEL:
        case EM_GETSEL:
        case CB_GETEDITSEL:
            if (IS_DBCS_ENABLED()) {
                RIPERR1(ERROR_INVALID_PARAMETER,
                        RIP_WARNING,
                        "Invalid DBCS message (%x) to SendMessageWorker",message);
            }
            //
            // Fall down...

        default:
            if ((message < WM_USER) && MessageTable[message].bThunkMessage) {
                fNeedTranslation = TRUE;
            }
        }
    }

#ifndef LATER
    /*
     * If the window has a client side worker proc and has
     * not been subclassed, dispatch the message directly
     * to the worker proc.  Otherwise, dispatch it normally.
     */
    pcls = REBASEALWAYS(pwnd, pcls);

    if ((pcls->fnid >= FNID_CONTROLSTART && pcls->fnid <= FNID_CONTROLEND) &&
        ((KERNEL_ULONG_PTR)pwnd->lpfnWndProc == FNID_TO_CLIENT_PFNW_KERNEL(pcls->fnid) ||
         (KERNEL_ULONG_PTR)pwnd->lpfnWndProc == FNID_TO_CLIENT_PFNA_KERNEL(pcls->fnid))) {
        PWNDMSG pwm = &gSharedInfo.awmControl[pcls->fnid - FNID_START];

        /*
         * If this message is not processed by the control, call
         * xxxDefWindowProc
         */
        if (pwm->abMsgs && ((message > pwm->maxMsgs) ||
                !((pwm->abMsgs)[message / 8] & (1 << (message & 7))))) {

            /*
             * Special case dialogs so that we can ignore unimportant
             * messages during dialog creation.
             */
            if (pcls->fnid == FNID_DIALOG &&
                    PDLG(pwnd) && PDLG(pwnd)->lpfnDlg != NULL) {
                /*
                 * If A/W translation are needed for Dialog,
                 * it should go to kernel side to perform proper message.
                 * DefDlgProcWorker will call aplication's DlgProc directly
                 * without A/W conversion.
                 */
                if (fNeedTranslation) {
                    goto lbServerSendMessage;
                }
                /*
                 * Call woker procudure.
                 */
            SendMessageToWorker1Again:
                lRet = ((PROC)(FNID_TO_CLIENT_PFNWORKER(pcls->fnid)))(pwnd, message, wParam, lParam, fAnsi);
                /*
                 * if we have DBCS TrailingByte that should be sent, send it here..
                 */
                DISPATCH_DBCS_MESSAGE_IF_EXIST(message,wParam,bDoDbcsMessaging,SendMessageToWorker1);

                return lRet;
            } else {
                /*
                 * Call worker procedure.
                 */
            SendMessageToDefWindowAgain:
                lRet = DefWindowProcWorker(pwnd, message, wParam, lParam, fAnsi);
                /*
                 * if we have DBCS TrailingByte that should be sent, send it here..
                 */
                 DISPATCH_DBCS_MESSAGE_IF_EXIST(message,wParam,bDoDbcsMessaging,SendMessageToDefWindow);

                return lRet;
            }
        } else {
            /*
             * Call woker procudure.
             */
        SendMessageToWorker2Again:
            lRet = ((PROC)(FNID_TO_CLIENT_PFNWORKER(pcls->fnid)))(pwnd, message, wParam, lParam, fAnsi);

            /*
             * if we have DBCS TrailingByte that should be sent, send it here..
             */
            DISPATCH_DBCS_MESSAGE_IF_EXIST(message,wParam,bDoDbcsMessaging,SendMessageToWorker2);

            return lRet;
        }
    }
#endif

    /*
     * If this message needs to be translated, go through the kernel.
     */
    if (fNeedTranslation) {
        goto lbServerSendMessage;
    }

    /*
     * Call Client Windows procudure.
     */
SendMessageToWndProcAgain:

    lRet = CALLPROC_WOWCHECKPWW(MapKernelClientFnToClientFn(pwnd->lpfnWndProc), hwnd, message, wParam, lParam, &(pwnd->state));

    /*
     * if we have DBCS TrailingByte that should be sent, send it here..
     */
    DISPATCH_DBCS_MESSAGE_IF_EXIST(message,wParam,bDoDbcsMessaging,SendMessageToWndProc);

    return lRet;
}

// LATER!!! can this somehow be combined or subroutinized with SendMessageWork
// so we don't have to copies of 95% identical code.

/***************************************************************************\
* SendMessageTimeoutWorker
*
* Translates the message, calls SendMessageTimeout on server side.
*
* 07-21-92 ChrisBB  Created/modified SendMessageWorkder
\***************************************************************************/

LRESULT SendMessageTimeoutWorker(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    UINT fuFlags,
    UINT uTimeout,
    PULONG_PTR lpdwResult,
    BOOL fAnsi)
{
    SNDMSGTIMEOUT smto;

    /*
     * Prevent apps from setting hi 16 bits so we can use them internally.
     */
    if (message & RESERVED_MSG_BITS) {
        RIPERR1(ERROR_INVALID_PARAMETER,
                RIP_WARNING,
                "Invalid parameter \"message\" (%ld) to SendMessageTimeoutWorker",
                message);

        return(0);
    }

    if (fuFlags & ~SMTO_VALID) {
        RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "invalid dwFlags (%x) for SendMessageTimeout\n", fuFlags);
        return(0);
    }

    if (lpdwResult != NULL)
        *lpdwResult = 0L;

    /*
     * Always send broadcast requests straight to the server.
     * Note: the xParam is used to id if it's from timeout or
     * from an normal sendmessage.
     */
    smto.fuFlags = fuFlags;
    smto.uTimeout = uTimeout;
    smto.lSMTOReturn = 0;
    smto.lSMTOResult = 0;

    /*
     * Thunk through a special sendmessage for -1 hwnd's so that the general
     * purpose thunks don't allow -1 hwnd's.
     */
    if (hwnd == (HWND)-1 || hwnd == (HWND)0x0000FFFF) {
        /*
         * Get a real hwnd so the thunks will validation ok. Note that since
         * -1 hwnd is really rare, calling GetDesktopWindow() here is not a
         * big deal.
         */
        hwnd = GetDesktopWindow();

        CsSendMessage(hwnd, message, wParam, lParam,
                (ULONG_PTR)&smto, FNID_SENDMESSAGEFF, fAnsi);
    } else {
        CsSendMessage(hwnd, message, wParam, lParam,
                (ULONG_PTR)&smto, FNID_SENDMESSAGEEX, fAnsi);
    }

    if (lpdwResult != NULL)
         *lpdwResult = smto.lSMTOResult;

    return smto.lSMTOReturn;
}

/***************************************************************************\
* DefWindowProcWorker
*
* Handles any messages that can be dealt with wholly on the client and
* passes the rest to the server.
*
* 03-31-92 DarrinM      Created.
\***************************************************************************/

LRESULT DefWindowProcWorker(
    PWND pwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    DWORD fAnsi)
{
    HWND hwnd = HWq(pwnd);
    int icolBack;
    int icolFore;
    PWND pwndParent;
    HWND hwndDefIme;
    PWND pwndDefIme;
    PIMEUI pimeui;

#if DBG
    if (!gfTurboDWP) {
        return CsSendMessage(hwnd, message, wParam, lParam, 0L,
                FNID_DEFWINDOWPROC, fAnsi);
    } else {
#endif


    if (FDEFWINDOWMSG(message, DefWindowMsgs)) {
        return CsSendMessage(hwnd, message, wParam, lParam, 0L,
                FNID_DEFWINDOWPROC, fAnsi);
    } else if (!FDEFWINDOWMSG(message, DefWindowSpecMsgs)) {
        return 0;
    }

    /*
     * Important:  If you add cases to the switch statement below,
     *             add the messages to server.c's gawDefWindowSpecMsgs.
     *             Similarly if you add cases to dwp.c's DefWindowProc
     *             which can come from the client, add the messages
     *             to gawDefWindowMsgs.
     */

    switch (message) {

    case WM_HELP:
        {
        PWND  pwndDest;

        /*
         * If this window is a child window, Help message must be passed on
         * to it's parent; Else, this must be passed on to the owner window.
         */
        pwndDest = (TestwndChild(pwnd) ? pwnd->spwndParent : pwnd->spwndOwner);
        if (pwndDest) {
            pwndDest = REBASEPTR(pwnd, pwndDest);
            if (pwndDest != _GetDesktopWindow())
                return SendMessageW(HWq(pwndDest), WM_HELP, wParam, lParam);;
        }
        return(0L);
        }

    case WM_MOUSEWHEEL:
        if (TestwndChild(pwnd)) {
            pwndParent = REBASEPWND(pwnd, spwndParent);
            SendMessageW(HW(pwndParent), WM_MOUSEWHEEL, wParam, lParam);
        }
        break;

    case WM_CONTEXTMENU:
        if (TestwndChild(pwnd)) {
            pwndParent = REBASEPWND(pwnd, spwndParent);
            SendMessageW(HW(pwndParent), WM_CONTEXTMENU,
                    (WPARAM)hwnd, lParam);
        }
        break;

    /*
     * Default handling for WM_CONTEXTMENU support
     */
    case WM_RBUTTONUP:
#ifdef USE_MIRRORING
        if (TestWF(pwnd, WEFLAYOUTRTL)) {
            lParam = MAKELONG(pwnd->rcClient.right - GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) + pwnd->rcClient.top);
        } else
#endif
        {
            lParam = MAKELONG(GET_X_LPARAM(lParam) + pwnd->rcClient.left, GET_Y_LPARAM(lParam) + pwnd->rcClient.top);
        }
        SendMessageWorker(pwnd, WM_CONTEXTMENU, (WPARAM)hwnd, lParam, fAnsi);
        break;

    case WM_APPCOMMAND:
        if (TestwndChild(pwnd)) {
            /*
             * Bubble the message to the parent
             */
            pwndParent = REBASEPWND(pwnd, spwndParent);
            return SendMessageW(HW(pwndParent), WM_APPCOMMAND, wParam, lParam);
        } else {
            /*
             * Call the server side to send the shell hook HSHELL_APPCOMMAND
             */
            return CsSendMessage(hwnd, WM_APPCOMMAND, wParam, lParam, 0L, FNID_DEFWINDOWPROC, fAnsi);
        }
        break;

    /*
     * Default handling for WM_APPCOMMAND support
     */
    case WM_NCXBUTTONUP:
    case WM_XBUTTONUP:
        {
            WORD cmd;
            WORD keystate;
            LPARAM lParamAppCommand;

            switch (GET_XBUTTON_WPARAM(wParam)) {
            case XBUTTON1:
                cmd = APPCOMMAND_BROWSER_BACKWARD;
                break;

            case XBUTTON2:
                cmd = APPCOMMAND_BROWSER_FORWARD;
                break;

            default:
                cmd = 0;
                break;
            }

            if (cmd == 0) {
                break;
            }

            cmd |= FAPPCOMMAND_MOUSE;
            if (message == WM_XBUTTONUP) {
                keystate = GET_KEYSTATE_WPARAM(wParam);
            } else {
                keystate = GetMouseKeyState();
            }

            lParamAppCommand = MAKELPARAM(keystate, cmd);
            SendMessageWorker(pwnd, WM_APPCOMMAND, (WPARAM)hwnd, lParamAppCommand, fAnsi);
            break;
        }

    case WM_WINDOWPOSCHANGED: {
        PWINDOWPOS ppos = (PWINDOWPOS)lParam;

        if (!(ppos->flags & SWP_NOCLIENTMOVE)) {
            POINT pt = {pwnd->rcClient.left, pwnd->rcClient.top};
            pwndParent = REBASEPWND(pwnd, spwndParent);

            if (pwndParent != _GetDesktopWindow()) {
                pt.x -= pwndParent->rcClient.left;
                pt.y -= pwndParent->rcClient.top;
            }

            SendMessageWorker(pwnd, WM_MOVE, FALSE, MAKELPARAM(pt.x, pt.y), fAnsi);
        }

        if ((ppos->flags & SWP_STATECHANGE) || !(ppos->flags & SWP_NOCLIENTSIZE)) {
            UINT cmd;
            RECT rc;

            if (TestWF(pwnd, WFMINIMIZED))
                cmd = SIZEICONIC;
            else if (TestWF(pwnd, WFMAXIMIZED))
                cmd = SIZEFULLSCREEN;
            else
                cmd = SIZENORMAL;

        /*
         *  HACK ALERT:
         *  If the window is minimized then the real client width and height are
         *  zero. But, in win3.1 they were non-zero. Under Chicago, PrintShop
         *  Deluxe ver 1.2 hits a divide by zero. To fix this we fake the width
         *  and height for old apps to be non-zero values.
         *  GetClientRect does that job for us.
         */
            _GetClientRect(pwnd, &rc);
            SendMessageWorker(pwnd, WM_SIZE, cmd,
                    MAKELONG(rc.right - rc.left,
                    rc.bottom - rc.top), fAnsi);
        }
        return 0;
        }

    case WM_MOUSEACTIVATE: {
        PWND pwndT;
        LRESULT lt;

        /*
         * GetChildParent returns either a kernel pointer or NULL.
         */
        pwndT = GetChildParent(pwnd);
        if (pwndT != NULL) {
            pwndT = REBASEPTR(pwnd, pwndT);
            lt = SendMessageWorker(pwndT, WM_MOUSEACTIVATE, wParam, lParam, fAnsi);
            if (lt != 0)
                return lt;
        }

        /*
         * Moving, sizing or minimizing? Activate AFTER we take action.
         */
        return ((LOWORD(lParam) == HTCAPTION) && (HIWORD(lParam) == WM_LBUTTONDOWN )) ?
                (LONG)MA_NOACTIVATE : (LONG)MA_ACTIVATE;
        }

    case WM_CTLCOLORSCROLLBAR:
        if ((gpsi->BitCount < 8) ||
            (SYSRGB(3DHILIGHT) != SYSRGB(SCROLLBAR)) ||
            (SYSRGB(3DHILIGHT) == SYSRGB(WINDOW)))
        {
            /*
             * Remove call to UnrealizeObject().  GDI Handles this for
             * brushes on NT.
             *
             * UnrealizeObject(ghbrGray);
             */

            SetBkColor((HDC)wParam, SYSRGB(3DHILIGHT));
            SetTextColor((HDC)wParam, SYSRGB(3DFACE));
            return((LRESULT)gpsi->hbrGray);
        }

        icolBack = COLOR_3DHILIGHT;
        icolFore = COLOR_BTNTEXT;
        goto SetColor;

    case WM_CTLCOLORBTN:
        if (pwnd == NULL)
            goto ColorDefault;

        if (TestWF(pwnd, WFWIN40COMPAT)) {
            icolBack = COLOR_3DFACE;
            icolFore = COLOR_BTNTEXT;
        } else {
            goto ColorDefault;
        }
        goto SetColor;

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORMSGBOX:
        // We want static controls in dialogs to have the 3D
        // background color, but statics in windows to inherit
        // their parents' background.

        if (pwnd == NULL)
            goto ColorDefault;

        if (TestWF(pwnd, WFWIN40COMPAT)) {
            icolBack = COLOR_3DFACE;
            icolFore = COLOR_WINDOWTEXT;
            goto SetColor;
        }
        // ELSE FALL THRU...

    case WM_CTLCOLOR:              // here for WOW only
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLOREDIT:
      ColorDefault:
        icolBack = COLOR_WINDOW;
        icolFore = COLOR_WINDOWTEXT;

      SetColor:
      {
        SetBkColor((HDC)wParam, gpsi->argbSystem[icolBack]);
        SetTextColor((HDC)wParam, gpsi->argbSystem[icolFore]);
        return (LRESULT)(SYSHBRUSH(icolBack));
      }

    case WM_NCHITTEST:
        return FindNCHit(pwnd, (LONG)lParam);

    case WM_GETTEXT:
        if (wParam != 0) {

            LPWSTR lpszText;
            UINT   cchSrc;

            if (pwnd->strName.Length) {

                lpszText = REBASE(pwnd, strName.Buffer);
                cchSrc = (UINT)pwnd->strName.Length / sizeof(WCHAR);

                if (fAnsi) {

                    LPSTR lpName = (LPSTR)lParam;

                    /*
                     * Non-zero retval means some text to copy out.  Do not
                     * copy out more than the requested byte count
                     * 'chMaxCount'.
                     */
                    cchSrc = WCSToMB(lpszText,
                                     cchSrc,
                                     (LPSTR *)&lpName,
                                     (UINT)(wParam - 1),
                                     FALSE);

                    lpName[cchSrc] = '\0';

                } else {

                    LPWSTR lpwName = (LPWSTR)lParam;

                    cchSrc = min(cchSrc, (UINT)(wParam - 1));
                    RtlCopyMemory(lpwName, lpszText, cchSrc * sizeof(WCHAR));
                    lpwName[cchSrc] = 0;
                }

                return cchSrc;
            }

            /*
             * else Null terminate the text buffer since there is no text.
             */
            if (fAnsi) {
                ((LPSTR)lParam)[0] = 0;
            } else {
                ((LPWSTR)lParam)[0] = 0;
            }
        }

        return 0;

    case WM_GETTEXTLENGTH:
        if (pwnd->strName.Length) {
            UINT cch;
            if (fAnsi) {
                RtlUnicodeToMultiByteSize(&cch,
                                          REBASE(pwnd, strName.Buffer),
                                          pwnd->strName.Length);
            } else {
                cch = pwnd->strName.Length / sizeof(WCHAR);
            }
            return cch;
        }
        return 0L;

    case WM_QUERYDRAGICON:
        /*
         * If the window is WIN40COMPAT or has a kernel side procedure
         * do not attempt to look into the instance module
         */
        if (TestWF(pwnd, WFWIN40COMPAT) || TestWF(pwnd, WFSERVERSIDEPROC)) {
            return 0;
        }
        /*
         * For old apps, like the VB3 ones, try to load the icon from resources
         * This is how Win95 does.
         */
        return (LRESULT)LoadIconW(pwnd->hModule, MAKEINTRESOURCE(1));

    case WM_QUERYOPEN:
    case WM_QUERYENDSESSION:
    case WM_DEVICECHANGE:
    case WM_POWERBROADCAST:
        return TRUE;

    case WM_KEYDOWN:
        if (wParam == VK_F10) {
            return CsSendMessage(hwnd, message, wParam, lParam, 0L,
                    FNID_DEFWINDOWPROC, fAnsi);
        }
        break;

    case WM_SYSKEYDOWN:
        if ((HIWORD(lParam) & SYS_ALTERNATE) || (wParam == VK_F10) ||
                (wParam == VK_ESCAPE))
            return CsSendMessage(hwnd, message, wParam, lParam, 0L,
                    FNID_DEFWINDOWPROC, fAnsi);
        break;

    case WM_CHARTOITEM:
    case WM_VKEYTOITEM:
        /*
         * Do default processing for keystrokes into owner draw listboxes.
         */
        return -1;

    case WM_ACTIVATE:
        if (LOWORD(wParam))
            return CsSendMessage(hwnd, message, wParam, lParam, 0L,
                    FNID_DEFWINDOWPROC, fAnsi);
        break;

    case WM_SHOWWINDOW:
        if (lParam != 0)
            return CsSendMessage(hwnd, message, wParam, lParam, 0L,
                    FNID_DEFWINDOWPROC, fAnsi);
        break;

    case WM_DROPOBJECT:
       return DO_DROPFILE;

    case WM_WINDOWPOSCHANGING:
        /*
         * If the window's size is changing, adjust the passed-in size
         */
        #define ppos ((WINDOWPOS *)lParam)
        if (!(ppos->flags & SWP_NOSIZE))
            return CsSendMessage(hwnd, message, wParam, lParam, 0L,
                    FNID_DEFWINDOWPROC, fAnsi);
        #undef ppos
        break;

    case WM_KLUDGEMINRECT:
        {
        SHELLHOOKINFO shi;
        LPRECT lprc = (LPRECT)lParam;

        shi.hwnd = (HWND)wParam;
        shi.rc.left = MAKELONG(lprc->left, lprc->top);
        shi.rc.top = MAKELONG(lprc->right, lprc->bottom);

        if (gpsi->uiShellMsg == 0)
            SetTaskmanWindow(NULL);
        if (SendMessageWorker(pwnd, gpsi->uiShellMsg, HSHELL_GETMINRECT,
                (LPARAM)&shi, fAnsi)) {
            //
            // Now convert the RECT back from two POINTS structures into two POINT
            // structures.
            //
            lprc->left   = (SHORT)LOWORD(shi.rc.left);  // Sign extend
            lprc->top    = (SHORT)HIWORD(shi.rc.left);  // Sign extend
            lprc->right  = (SHORT)LOWORD(shi.rc.top);   // Sign extend
            lprc->bottom = (SHORT)HIWORD(shi.rc.top);   // Sign extend
        }
        break;
        }

    case WM_NOTIFYFORMAT:
        if (lParam == NF_QUERY)
            return(TestWF(pwnd, WFANSICREATOR) ? NFR_ANSI : NFR_UNICODE);
        break;

    case WM_IME_KEYDOWN:
        if (fAnsi)
            PostMessageA(hwnd, WM_KEYDOWN, wParam, lParam);
        else
            PostMessageW(hwnd, WM_KEYDOWN, wParam, lParam);
        break;

    case WM_IME_KEYUP:
        if (fAnsi)
            PostMessageA(hwnd, WM_KEYUP, wParam, lParam);
        else
            PostMessageW(hwnd, WM_KEYUP, wParam, lParam);
        break;

    case WM_IME_CHAR:
        //if (TestCF(pwnd, CFIME))
        //    break;

        if ( fAnsi ) {
            if( IsDBCSLeadByteEx(THREAD_CODEPAGE(),(BYTE)(wParam >> 8)) ) {
                PostMessageA(hwnd,
                             WM_CHAR,
                             (WPARAM)((BYTE)(wParam >> 8)),   // leading byte
                             1L);
                PostMessageA(hwnd,
                             WM_CHAR,
                             (WPARAM)((BYTE)wParam),         // trailing byte
                             1L);
            }
            else
                PostMessageA(hwnd,
                             WM_CHAR,
                             (WPARAM)(wParam),
                             1L);
        } else {
            PostMessageW(hwnd, WM_CHAR, wParam, 1L);
        }
        break;

    case WM_IME_COMPOSITION:
        //if (TestCF(pwnd, CFIME))
        //    break;

        if (lParam & GCS_RESULTSTR) {
            HIMC  hImc;
            DWORD cbLen;

            if ((hImc = fpImmGetContext(hwnd)) == NULL_HIMC)
                goto dwpime_ToIMEWnd_withchk;

            if (fAnsi) {
                LPSTR pszBuffer, psz;

                /*
                 * ImmGetComposition returns the size of buffer needed in byte.
                 */
                if (!(cbLen = fpImmGetCompositionStringA(hImc, GCS_RESULTSTR, NULL, 0))) {
                    fpImmReleaseContext(hwnd, hImc);
                    goto dwpime_ToIMEWnd_withchk;
                }

                pszBuffer = psz = (LPSTR)UserLocalAlloc(HEAP_ZERO_MEMORY,
                                                        cbLen + sizeof(CHAR));

                if (pszBuffer == NULL) {
                    fpImmReleaseContext(hwnd, hImc);
                    goto dwpime_ToIMEWnd_withchk;
                }

                fpImmGetCompositionStringA(hImc, GCS_RESULTSTR, psz, cbLen);

                while (*psz) {
                    if (IsDBCSLeadByteEx(THREAD_CODEPAGE(),*psz)) {
                        if (*(psz+1)) {
                            SendMessageA( hwnd,
                                          WM_IME_CHAR,
                                          MAKEWPARAM(MAKEWORD(*(psz+1), *psz), 0),
                                          1L );
                            psz++;
                        }
                        psz++;
                    }
                    else
                        SendMessageA( hwnd,
                                      WM_IME_CHAR,
                                      MAKEWPARAM(MAKEWORD(*(psz++), 0), 0),
                                      1L );
                }

                UserLocalFree(pszBuffer);

                fpImmReleaseContext(hwnd, hImc);
            }
            else {
                LPWSTR pwszBuffer, pwsz;

                /*
                 * ImmGetComposition returns the size of buffer needed in byte
                 */
                if (!(cbLen = fpImmGetCompositionStringW(hImc, GCS_RESULTSTR, NULL, 0))) {
                    fpImmReleaseContext(hwnd, hImc);
                    goto dwpime_ToIMEWnd_withchk;
                }

                pwszBuffer = pwsz = (LPWSTR)UserLocalAlloc(HEAP_ZERO_MEMORY,
                                                           cbLen + sizeof(WCHAR));

                if (pwszBuffer == NULL) {
                    fpImmReleaseContext(hwnd, hImc);
                    goto dwpime_ToIMEWnd_withchk;
                }

                fpImmGetCompositionStringW(hImc, GCS_RESULTSTR, pwsz, cbLen);

                while (*pwsz)
                    SendMessageW(hwnd, WM_IME_CHAR, MAKEWPARAM(*pwsz++, 0), 1L);

                UserLocalFree(pwszBuffer);

                fpImmReleaseContext(hwnd, hImc);
            }
        }

        /*
         * Fall through to send to Default IME Window with checking
         * activated hIMC.
         */

    case WM_IME_STARTCOMPOSITION:
    case WM_IME_ENDCOMPOSITION:
dwpime_ToIMEWnd_withchk:
        //if (TestCF(pwnd, CFIME))
        //    break;

        if (GetClientInfo()->dwTIFlags & TIF_DISABLEIME) {
            break;
        }
        /*
         * We assume this Wnd uses DefaultIMEWindow.
         * If this window has its own IME window, it have to call
         * ImmIsUIMessage()....
         */
        hwndDefIme = fpImmGetDefaultIMEWnd(hwnd);

        if (hwndDefIme == hwnd) {
            /*
             * VC++ 1.51 TLW0NCL.DLL subclass IME class window
             * and pass IME messages to DefWindowProc().
             */
            RIPMSG1(RIP_WARNING,
                "IME Class window is hooked and IME message [%X] are sent to DefWindowProc",
                message);
            ImeWndProcWorker(pwnd, message, wParam, lParam, fAnsi);
            break;
        }

        if ((pwndDefIme = ValidateHwndNoRip(hwndDefIme)) != NULL) {
            /*
             * If hImc of this window is not activated for IME window,
             * we don't send WM_IME_NOTIFY.
             */
            pimeui = ((PIMEWND)pwndDefIme)->pimeui;
            if (pimeui->hIMC == fpImmGetContext(hwnd))
                return SendMessageWorker(pwndDefIme, message, wParam, lParam, fAnsi);
            else
                RIPMSG1(RIP_WARNING,
                    "DefWindowProc can not send WM_IME_message [%X] now",
                    message);
        }
        break;

dwpime_ToTopLevel_withchk:
        //if (TestCF(pwnd, CFIME))
        //    break;

        /*
         * We assume this Wnd uses DefaultIMEWindow.
         * If this window has its own IME window, it have to call
         * ImmIsUIMessage()....
         */
        hwndDefIme = fpImmGetDefaultIMEWnd(hwnd);

        if (hwndDefIme == hwnd) {
            /*
             * VC++ 1.51 TLW0NCL.DLL subclass IME class window
             * and pass IME messages to DefWindowProc().
             */
            RIPMSG1(RIP_WARNING,
                "IME Class window is hooked and IME message [%X] are sent to DefWindowProc",
                message);
            ImeWndProcWorker(pwnd, message, wParam, lParam, fAnsi);
            break;
        }

        pwndDefIme = ValidateHwndNoRip(hwndDefIme);

        if ((pwndDefIme = ValidateHwndNoRip(hwndDefIme)) != NULL) {
            PWND pwndT, pwndParent;

            pwndT = pwnd;

            while (TestwndChild(pwndT)) {
                pwndParent = REBASEPWND(pwndT, spwndParent);
                if (GETPTI(pwndParent) != GETPTI(pwnd))
                    break;
                pwndT = pwndParent;
            }

            /*
             * If hImc of this window is not activated for IME window,
             * we don't send WM_IME_NOTIFY.
             */
            if (pwndT != pwnd) {
                pimeui = ((PIMEWND)pwndDefIme)->pimeui;
                if (pimeui->hIMC == fpImmGetContext(hwnd))
                    return SendMessageWorker(pwndT, message, wParam, lParam, fAnsi);
                else
                    RIPMSG1(RIP_WARNING,
                        "DefWindowProc can not send WM_IME_message [%X] now",
                        message);
            }
            else {
                /*
                 * Review !!
                 * If this is the toplevel window, we pass messages to
                 * the default IME window...
                 */
                return SendMessageWorker(pwndDefIme, message, wParam, lParam, fAnsi);
            }
        }
        break;

    case WM_IME_NOTIFY:
        switch (wParam) {
        case IMN_OPENSTATUSWINDOW:
        case IMN_CLOSESTATUSWINDOW:
#ifndef WKWOK_DEBUG
            goto dwpime_ToIMEWnd_withchk;
#endif
            goto dwpime_ToTopLevel_withchk;

        default:
            goto dwpime_ToIMEWnd_withchk;
        }
        break;

    case WM_IME_REQUEST:
        switch (wParam) {
        case IMR_QUERYCHARPOSITION:
            goto dwpime_ToIMEWnd_withchk;
        default:
            break;
        }
        break;

    case WM_IME_SYSTEM:
        if (wParam == IMS_SETACTIVECONTEXT) {
            RIPMSG0(RIP_WARNING, "DefWindowProc received unexpected WM_IME_SYSTEM");
            break;
        }

        /*
         * IMS_SETOPENSTATUS is depended on the activated input context.
         * It needs to be sent to only the activated system window.
         */
        if (wParam == IMS_SETOPENSTATUS)
            goto dwpime_ToIMEWnd_withchk;

        /*
         * Fall through to send to Default IME Window.
         */

    case WM_IME_SETCONTEXT:
        //if (TestCF(pwnd, CFIME))
        //    break;

        hwndDefIme = fpImmGetDefaultIMEWnd(hwnd);

        if (hwndDefIme == hwnd) {
            /*
             * VC++ 1.51 TLW0NCL.DLL subclass IME class window
             * and pass IME messages to DefWindowProc().
             */
            RIPMSG1(RIP_WARNING,
                "IME Class window is hooked and IME message [%X] are sent to DefWindowProc",
                message);
            ImeWndProcWorker(pwnd, message, wParam, lParam, fAnsi);
            break;
        }

        if ((pwndDefIme = ValidateHwndNoRip(hwndDefIme)) != NULL)
            return SendMessageWorker(pwndDefIme, message, wParam, lParam, fAnsi);

        break;

    case WM_IME_SELECT:
        RIPMSG0(RIP_WARNING, "DefWindowProc should not receive WM_IME_SELECT");
        break;

    case WM_IME_COMPOSITIONFULL:
        //if (TestCF(pwnd, CFIME))
        //    break;

        if (GETAPPVER() < VER40) {
            /*
             * This is a temporary solution for win31app.
             * FEREVIEW: For M5 this will call WINNLS message mapping logic
             *           -yutakan
             */
            return SendMessageWorker(pwnd, WM_IME_REPORT,
                             IR_FULLCONVERT, (LPARAM)0L, fAnsi);
        }
        break;

    case WM_CHANGEUISTATE:
        {
            WORD wAction = LOWORD(wParam);
            WORD wFlags = HIWORD(wParam);
            BOOL bRealChange = FALSE;

            if (wFlags & ~UISF_VALID || wAction > UIS_LASTVALID ||
                lParam || !TEST_KbdCuesPUSIF) {
                    return 0;
            }

            if (wAction == UIS_INITIALIZE) {
                if (gpsi->bLastRITWasKeyboard) {
                    wAction = UIS_CLEAR;
                } else {
                    wAction = UIS_SET;
                }
                wFlags = UISF_HIDEFOCUS | UISF_HIDEACCEL;
                wParam = MAKEWPARAM(wAction, wFlags);
            }

            UserAssert(wAction == UIS_SET || wAction == UIS_CLEAR);
            /*
             * If the state is not going to change, there's nothing to do here
             */
            if (wFlags & UISF_HIDEFOCUS) {
                bRealChange = (!!TestWF(pwnd, WEFPUIFOCUSHIDDEN)) ^ (wAction == UIS_SET);
            }
            if (wFlags & UISF_HIDEACCEL) {
                bRealChange |= (!!TestWF(pwnd, WEFPUIACCELHIDDEN)) ^ (wAction == UIS_SET);
            }

            if (!bRealChange) {
                break;
            }
            /*
             * Children pass this message up
             * Top level windows update send down to themselves WM_UPDATEUISTATE.
             * WM_UPDATEUISTATE will change the state bits and broadcast down the message
             */
            if (TestwndChild(pwnd)) {

                return SendMessageWorker(REBASEPWND(pwnd, spwndParent), WM_CHANGEUISTATE,
                              wParam, lParam, fAnsi);
            } else {
                return SendMessageWorker(pwnd, WM_UPDATEUISTATE, wParam, lParam, fAnsi);
            }

        }
        break;

    case WM_QUERYUISTATE:
        return (TestWF(pwnd, WEFPUIFOCUSHIDDEN) ? UISF_HIDEFOCUS : 0) |
               (TestWF(pwnd, WEFPUIACCELHIDDEN) ? UISF_HIDEACCEL : 0);
        break;
    }

    return 0;

#if DBG
    } // gfTurboDWP
#endif
}


/***************************************************************************\
* CallWindowProc
*
* Calls pfn with the passed message parameters. If pfn is a server-side
* window proc the server is called to deliver the message to the window.
* Currently we have the following restrictions:
*
* 04-17-91 DarrinM Created.
\***************************************************************************/

LRESULT WINAPI CallWindowProcAorW(
    WNDPROC pfn,
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    BOOL bAnsi)             // Denotes if input is Ansi or Unicode
{
    PCALLPROCDATA pCPD;

    /*
     * Raid# 78954: SPY++
     *
     * Under FE NT4.0 or NT5.0, the sytem sends WM_GETTEXTLENGTH
     * corresponding to WM_xxxGETTEXT to optimize buffer allocation.
     * This is really needed to avoid the buffer size inflation.
     * For some reasons, Spy++ passes NULL as pfn to CallWindowProc
     *
     */
    if (pfn == NULL) {
        RIPMSG0(RIP_WARNING, "CallWidowProcAorW(): pfn == NULL!");
        return 0L;
    }

// OPT!! check an ANSI\UNICODE table rather than fnDWORD
// OPT!! convert WM_CHAR family messages in line

    /*
     * Check if pfn is really a CallProcData Handle
     * if it is and there is no ANSI data then convert the handle
     * into an address; otherwise call the server for translation
     */
    if (ISCPDTAG(pfn)) {
        if (pCPD = HMValidateHandleNoRip((HANDLE)pfn, TYPE_CALLPROC)) {
            if ((message >= WM_USER) || !MessageTable[message].bThunkMessage) {
                pfn = (WNDPROC)pCPD->pfnClientPrevious;
            } else {
                return CsSendMessage(hwnd, message, wParam, lParam, (ULONG_PTR)pfn,
                        FNID_CALLWINDOWPROC, bAnsi);
            }
        } else {
            RIPMSG1(RIP_WARNING, "CallWindowProc tried using a deleted CPD %#p\n", pfn);
            return 0;
        }
    }

    return CALLPROC_WOWCHECK(pfn, hwnd, message, wParam, lParam);
}

LRESULT WINAPI CallWindowProcA(
    WNDPROC pfn,
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return CallWindowProcAorW(pfn, hwnd, message, wParam, lParam, TRUE);
}
LRESULT WINAPI CallWindowProcW(
    WNDPROC pfn,
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return CallWindowProcAorW(pfn, hwnd, message, wParam, lParam, FALSE);
}

/***************************************************************************\
* MenuWindowProc
*
* Calls the sever-side function xxxMenuWindowProc
*
* 07-27-92 Mikehar Created.
\***************************************************************************/

LRESULT WINAPI MenuWindowProcW(
    HWND hwnd,
    HWND hwndMDIClient,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return CsSendMessage(hwnd, message, wParam, lParam,
        (ULONG_PTR)hwndMDIClient, FNID_MENU, FALSE);
}

LRESULT WINAPI MenuWindowProcA(
    HWND hwnd,
    HWND hwndMDIClient,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return CsSendMessage(hwnd, message, wParam, lParam,
        (ULONG_PTR)hwndMDIClient, FNID_MENU, TRUE);
}

/***************************************************************************\
* _ClientGetListboxString
*
* This special function exists because LB_GETTEXT and CB_GETLBTEXT don't have
* buffer counts in them anywhere. Because there is no buffer count we have
* no idea how much room to reserved in the shared memory stack for this
* string to be copied into. The solution is to get the string length ahead
* of time, and send the message with this buffer length. Since this buffer
* length isn't a part of the original message, this routine is used for
* just this purpose.
*
* This routine gets called from the server.
*
* 04-13-91 ScottLu Created.
\***************************************************************************/

DWORD WINAPI _ClientGetListboxString(
    PWND pwnd,
    UINT msg,
    WPARAM wParam,
    LPSTR lParam, // May be a unicode or ANSI string
    ULONG_PTR xParam,
    PROC xpfn)
{
    return ((GENERICPROC)xpfn)(pwnd, msg, wParam, (LPARAM)lParam, xParam);
}

/***************************************************************************\
* DispatchMessageWorker
*
* Handles any messages that can be dealt with wholly on the client and
* passes the rest to the server.
*
* 04-24-92 DarrinM      Created.
\***************************************************************************/

LRESULT DispatchMessageWorker(
    MSG *pmsg,
    BOOL fAnsi)
{
    PWND pwnd;
    WPARAM wParamSaved;
    LRESULT lRet;
    BOOL bDoDbcsMessaging = FALSE;

    /*
     * Prevent apps from setting hi 16 bits so we can use them internally.
     */
    if (pmsg->message & RESERVED_MSG_BITS) {
        RIPERR1(ERROR_INVALID_PARAMETER,
                RIP_WARNING,
                "Invalid parameter \"pmsg->message\" (%ld) to DispatchMessageWorker",
                pmsg->message);

        return 0;
    }

    if (pmsg->hwnd != NULL) {
        pwnd = ValidateHwnd(pmsg->hwnd);
        if (pwnd == NULL)
            return 0;
        pmsg->hwnd = HWq(pwnd);     // get full 32-bit HWND in case this came from WoW
    } else {
        pwnd = NULL;
    }

    /*
     * If this is a synchronous-only message (takes a pointer in wParam or
     * lParam), then don't allow this message to go through since those
     * parameters have not been thunked, and are pointing into outer-space
     * (which would case exceptions to occur).
     *
     * (This api is only called in the context of a message loop, and you
     * don't get synchronous-only messages in a message loop).
     */
    if (TESTSYNCONLYMESSAGE(pmsg->message, pmsg->wParam)) {
        /*
         * Fail if 32 bit app is calling.
         */
        if (!(GetClientInfo()->dwTIFlags & TIF_16BIT)) {
            RIPERR0(ERROR_MESSAGE_SYNC_ONLY, RIP_WARNING, "DispatchMessageWorker: must be sync only");
            return FALSE;
        }

        /*
         * For wow apps, allow it to go through (for compatibility). Change
         * the message id so our code doesn't understand the message - wow
         * will get the message and strip out this bit before dispatching
         * the message to the application.
         */
        pmsg->message |= MSGFLAG_WOW_RESERVED;
    }

    /*
     * Timer callbacks that don't go through window procs are sent with
     * the callback address in lParam.  Identify and dispatch those timers.
     */
    if ((pmsg->message == WM_TIMER) || (pmsg->message == WM_SYSTIMER)) {
        if (pmsg->lParam != 0) {
            /*
             * System timers must be executed on the server's context.
             */
            if (pmsg->message == WM_SYSTIMER) {
                return NtUserDispatchMessage(pmsg);
            } else {
                /*
                 * We can't really trust what's in lParam, so make sure we
                 * handle any exceptions that occur during this call.
                 */
                try {
                    /* Bug 234292 - joejo
                     * Since the called window/dialog proc may have a different calling
                     * convention, we must wrap the call and, check esp and replace with
                     * a good esp when the call returns. This is what UserCallWinProc* does.
                    */
                    lRet = UserCallWinProc((WNDPROC)pmsg->lParam, pmsg->hwnd, pmsg->message,
                            pmsg->wParam, NtGetTickCount());
                } except ((GetAppCompatFlags2(VER40) & GACF2_NO_TRYEXCEPT_CALLWNDPROC) ?
                          EXCEPTION_CONTINUE_SEARCH : W32ExceptionHandler(FALSE, RIP_WARNING)) {
                      /* #359866
                       * Some applications like Hagaki Studio 2000 need to handle the exception
                       * in WndProc in their handler, even though it skips the API calls.
                       * For those apps, we have to honor the behavior of NT4, with no protection.
                       */
                    lRet = 0;
                }
                return lRet;
            }
        }
    }

    if (pwnd == NULL)
        return 0;

    /*
     * To be safe (in case some bizarre app wants to look at the message
     * again after dispatching it) save wParam so it can be restored after
     * RtlMBMessageWParamCharToWCS() or RtlWCSMessageToMB() mangle it.
     */
    wParamSaved = pmsg->wParam;

    /*
     * Pass messages intended for server-side windows over to the server.
     * WM_PAINTs are passed over so the WFPAINTNOTPROCESSED code can be
     * executed.
     */
    if (TestWF(pwnd, WFSERVERSIDEPROC) || (pmsg->message == WM_PAINT)) {
        if (fAnsi) {
            /*
             * Setup DBCS Messaging for WM_CHAR...
             */
            BUILD_DBCS_MESSAGE_TO_SERVER_FROM_CLIENTA(pmsg->message,pmsg->wParam,TRUE);

            /*
             * Convert wParam to Unicode, if nessesary.
             */
            RtlMBMessageWParamCharToWCS(pmsg->message, &pmsg->wParam);
        }
        lRet = NtUserDispatchMessage(pmsg);
        pmsg->wParam = wParamSaved;
        return lRet;
    }

    /*
     * If the dispatcher and the receiver are both ANSI or both UNICODE
     * then no message translation is necessary.  NOTE: this test
     * assumes that fAnsi is FALSE or TRUE, not just zero or non-zero.
     */
    if (!fAnsi != !TestWF(pwnd, WFANSIPROC)) {
        // before: if (fAnsi != ((TestWF(pwnd, WFANSIPROC)) ? TRUE : FALSE)) {
        UserAssert(PtiCurrent() == GETPTI(pwnd)); // use receiver's codepage
        if (fAnsi) {
            /*
             * Setup DBCS Messaging for WM_CHAR...
             */
            BUILD_DBCS_MESSAGE_TO_CLIENTW_FROM_CLIENTA(pmsg->message,pmsg->wParam,TRUE);

            /*
             * Convert wParam to Unicode, if nessesary.
             */
            RtlMBMessageWParamCharToWCS(pmsg->message, &pmsg->wParam);
        } else {
            /*
             * Convert wParam to ANSI...
             */
            RtlWCSMessageWParamCharToMB(pmsg->message, &pmsg->wParam);

            /*
             * Let's DBCS messaging for WM_CHAR....
             */
            BUILD_DBCS_MESSAGE_TO_CLIENTA_FROM_CLIENTW(
                pmsg->hwnd,pmsg->message,pmsg->wParam,pmsg->lParam,
                pmsg->time,pmsg->pt,bDoDbcsMessaging);
        }
    }

DispatchMessageAgain:
    lRet = CALLPROC_WOWCHECKPWW(MapKernelClientFnToClientFn(pwnd->lpfnWndProc), pmsg->hwnd, pmsg->message,
            pmsg->wParam, pmsg->lParam, &(pwnd->state));

    /*
     * if we have DBCS TrailingByte that should be sent, send it here..
     */
    DISPATCH_DBCS_MESSAGE_IF_EXIST(pmsg->message,pmsg->wParam,bDoDbcsMessaging,DispatchMessage);

    pmsg->wParam = wParamSaved;
    return lRet;
}

/***************************************************************************\
* GetMessageTime (API)
*
* This API returns the time when the last message was read from
* the current message queue.
*
* History:
* 11-19-90 DavidPe      Created.
\***************************************************************************/

LONG GetMessageTime(VOID)
{
    return (LONG)NtUserGetThreadState(UserThreadStateMessageTime);
}

/***************************************************************************\
* GetMessageExtraInfo (API)
*
* History:
* 28-May-1991 mikeke
\***************************************************************************/

LPARAM GetMessageExtraInfo(VOID)
{
    return (LPARAM)NtUserGetThreadState(UserThreadStateExtraInfo);
}

LPARAM SetMessageExtraInfo(LPARAM lParam)
{
    return (LPARAM)NtUserCallOneParam(lParam, SFI__SETMESSAGEEXTRAINFO);
}



/***********************************************************************\
* InSendMessage (API)
*
* This function determines if the current thread is preocessing a message
* from another application.
*
* History:
* 01-13-91 DavidPe      Ported.
\***********************************************************************/

BOOL InSendMessage(VOID)
{
    PCLIENTTHREADINFO pcti = GetClientInfo()->pClientThreadInfo;

    if (pcti) {
        return TEST_BOOL_FLAG(pcti->CTIF_flags, CTIF_INSENDMESSAGE);
    }
    return NtUserGetThreadState(UserThreadStateInSendMessage) != ISMEX_NOSEND;
}
/***********************************************************************\
* InSendMessageEx (API)
*
* This function tells you what type of send message is being processed
*  by the application, if any
*
* History:
* 01/22/97 GerardoB Created
\***********************************************************************/

DWORD InSendMessageEx(LPVOID lpReserved)
{
    PCLIENTTHREADINFO pcti = GetClientInfo()->pClientThreadInfo;
    UNREFERENCED_PARAMETER(lpReserved);

    if (pcti && !TEST_FLAG(pcti->CTIF_flags, CTIF_INSENDMESSAGE)) {
        return ISMEX_NOSEND;
    }
    return (DWORD)NtUserGetThreadState(UserThreadStateInSendMessage);
}

/***********************************************************************\
* GetCPD
*
* This function calls the server to allocate a CPD structure.
*
* History:
* 11-15-94 JimA         Created.
\***********************************************************************/

ULONG_PTR GetCPD(
    PVOID pWndOrCls,
    DWORD options,
    ULONG_PTR dwData)
{
    return NtUserGetCPD(HW(pWndOrCls), options, dwData);
}

#ifdef BUILD_WOW6432
/***********************************************************************\
* MapKernelClientFnToClientFn
*
* Maps a function pointer from what the kernel expects to what the
* client(user-mode) expects.
*
* History:
* 11-15-98 PeterHal         Created.
\***********************************************************************/
WNDPROC_PWND
MapKernelClientFnToClientFn(
    WNDPROC_PWND lpfnWndProc
    )
{
    KERNEL_ULONG_PTR *pp;

    for (pp = (KERNEL_ULONG_PTR*)&gpsi->apfnClientA; pp < (KERNEL_ULONG_PTR*) (&gpsi->apfnClientA+1); pp ++) {
        if ((KERNEL_ULONG_PTR)lpfnWndProc == *pp) {
            return (WNDPROC_PWND)((KERNEL_ULONG_PTR*) &pfnClientA) [ (pp - (KERNEL_ULONG_PTR*)&gpsi->apfnClientA) ];
        }
    }

    for (pp = (KERNEL_ULONG_PTR*)&gpsi->apfnClientW; pp < (KERNEL_ULONG_PTR*) (&gpsi->apfnClientW+1); pp ++) {
        if ((KERNEL_ULONG_PTR)lpfnWndProc == *pp) {
            return (WNDPROC_PWND)((KERNEL_ULONG_PTR*) &pfnClientW) [ (pp - (KERNEL_ULONG_PTR*)&gpsi->apfnClientW) ];
        }
    }

    return lpfnWndProc;
}
#endif
