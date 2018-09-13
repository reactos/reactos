
/****************************** Module Header ******************************\
* Module Name: register.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* DDE Manager - server registration module
*
* Created: 4/15/94 sanfords
*       to allow interoperability between DDEML16 and DDEML32
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*
 * interoperable DDEML service registration is accomplished via the
 * two messages UM_REGISTER and UM_UNREGISTER. (WM_USER range)
 * wParam=gaApp,
 * lParam=src hwndListen, (for instance specific HSZ generation.)
 * These messages are sent and the sender is responsible for freeing
 * the gaApp.
 */


/*
 * Broadcast-sends the given message to all top-level windows of szClass
 * except to hwndSkip.
 */
VOID SendRegisterMessageToClass(
ATOM atomClass,
UINT msg,
GATOM ga,
HWND hwndFrom,
BOOL fPost)
{
    HWND hwnd;
    PWND pwnd;
    PCLS pcls;

    hwnd = GetWindow(GetDesktopWindow(), GW_CHILD);
    while (hwnd != NULL) {
        pwnd=ValidateHwndNoRip(hwnd);
        if (pwnd) {
            pcls = (PCLS)REBASEALWAYS(pwnd, pcls);
            if (pcls->atomClassName == atomClass) {
                IncGlobalAtomCount(ga); // receiver frees
                if (fPost) {
                    PostMessage(hwnd, msg, (WPARAM)ga, (LPARAM)hwndFrom);
                } else {
                    SendMessage(hwnd, msg, (WPARAM)ga, (LPARAM)hwndFrom);
                }
            }
        }
        hwnd = GetWindow(hwnd, GW_HWNDNEXT);
    }
}


/*
 * Broadcast-sends a UM_REGISTER or UM_UNREGISTER message to all DDEML16
 * and DDEML32 listening windows in the system except hwndListen.
 *
 * We post Registration messages to prevent DdeConnectList recursion
 * and send Unregistration messages to avoid invalid source window
 * errors.
 */
VOID RegisterService(
BOOL fRegister,
GATOM gaApp,
HWND hwndListen)
{
    CheckDDECritOut;

    /*
     * Send notification to each DDEML32 listening window.
     */
    SendRegisterMessageToClass(gpsi->atomSysClass[ICLS_DDEMLMOTHER], fRegister ? UM_REGISTER : UM_UNREGISTER,
            gaApp, hwndListen, fRegister);
    /*
     * Send notification to each DDEML16 listening window.
     */
    SendRegisterMessageToClass(gpsi->atomSysClass[ICLS_DDEML16BIT], fRegister ? UM_REGISTER : UM_UNREGISTER,
            gaApp, hwndListen, fRegister);
}




LRESULT ProcessRegistrationMessage(
HWND hwnd,
UINT msg,
WPARAM wParam,
LPARAM lParam)
{
    PCL_INSTANCE_INFO pcii;
    LRESULT lRet = 0;

    CheckDDECritOut;

    /*
     * wParam = GATOM of app
     * lParam = hwndListen of source - may be a WOW DDEML source unthunked.
     */
    lParam = (LPARAM)HMValidateHandleNoRip((HWND)lParam, TYPE_WINDOW);
    lParam = (LPARAM)PtoH((PVOID)lParam);

    if (lParam == 0) {
        return(0);
    }

    EnterDDECrit;

    pcii = (PCL_INSTANCE_INFO)GetWindowLongPtr(hwnd, GWLP_INSTANCE_INFO);
    if (pcii != NULL &&
            !((msg == UM_REGISTER) && (pcii->afCmd & CBF_SKIP_REGISTRATIONS)) &&
            !((msg == UM_UNREGISTER) && (pcii->afCmd & CBF_SKIP_UNREGISTRATIONS))) {

        LATOM la, lais;

        la = GlobalToLocalAtom((GATOM)wParam);
        lais = MakeInstSpecificAtom(la, (HWND)lParam);

        DoCallback(pcii,
                (WORD)((msg == UM_REGISTER) ? XTYP_REGISTER : XTYP_UNREGISTER),
                0,
                (HCONV)0L,
                (HSZ)NORMAL_HSZ_FROM_LATOM(la),
                INST_SPECIFIC_HSZ_FROM_LATOM(lais),
                (HDDEDATA)0L,
                0L,
                0L);

        DeleteAtom(la);
        DeleteAtom(lais);
        lRet = 1;
    }

    GlobalDeleteAtom((ATOM)wParam);  // receiver frees
    LeaveDDECrit;
    return(1);
}
