/***************************************************************************\
*
*  DLGMGR2.C
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
*      Dialog Management Routines
*
* ??-???-???? mikeke    Ported from Win 3.0 sources
* 12-Feb-1991 mikeke    Added Revalidation code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* xxxRemoveDefaultButton
*
* Scan through all the controls in the dialog box and remove the default
* button style from any button that has it.  This is done since at times we
* do not know who has the default button.
*
* History:
*
* Bug 19449 - joejo
*
*   Stop infinite loop when pwnd != pwndStart but pwnd == pwnd after calling
*   _NextControl!
\***************************************************************************/

void xxxRemoveDefaultButton(
    PWND pwndRoot,
    PWND pwndStart)
{
    UINT code;
    PWND pwnd;
    PWND pwndDup;
    TL tlpwnd;

    CheckLock(pwndRoot);
    CheckLock(pwndStart);

    if (!pwndStart || TestWF(pwndStart, WEFCONTROLPARENT))
        pwndStart = _NextControl(pwndRoot, NULL, CWP_SKIPINVISIBLE | CWP_SKIPDISABLED);
    else
        pwndStart = _GetChildControl(pwndRoot, pwndStart);

    if (!pwndStart)
        return;

    pwnd = pwndStart;
    do {
        pwndDup = pwnd;
        
        ThreadLock(pwnd, &tlpwnd);

        code = (UINT)SendMessage(HWq(pwnd), WM_GETDLGCODE, 0, 0L);

        if (code & DLGC_DEFPUSHBUTTON) {
            SendMessage(HWq(pwnd), BM_SETSTYLE, BS_PUSHBUTTON, (LONG)TRUE);
        }

        pwnd = _NextControl(pwndRoot, pwnd, 0);

        ThreadUnlock(&tlpwnd);

    } while (pwnd && (pwnd != pwndStart) && (pwnd != pwndDup));
    
#if DBG
    if (pwnd && (pwnd != pwndStart) && (pwnd != pwndDup)) {
        RIPMSG0(RIP_WARNING, "xxxRemoveDefaultButton bailing potential infinite loop!");
    }
#endif
    
}


/***************************************************************************\
* xxxCheckDefPushButton
*
* History:
\***************************************************************************/

void xxxCheckDefPushButton(
    PWND pwndDlg,
    HWND hwndOldFocus,
    HWND hwndNewFocus)
{
    PWND pwndNewFocus;
    PWND pwndOldFocus;
    TL tlpwndT;
    PWND pwndT;
    UINT codeNewFocus = 0;
    UINT styleT;
    LONG lT;
    int id;

    if (hwndNewFocus)
        pwndNewFocus = ValidateHwnd(hwndNewFocus);
    else
        pwndNewFocus = NULL;

     if (hwndOldFocus)
         pwndOldFocus = ValidateHwnd(hwndOldFocus);
     else
         pwndOldFocus = NULL;

    CheckLock(pwndDlg);
    CheckLock(pwndNewFocus);
    CheckLock(pwndOldFocus);

    if (pwndNewFocus)
    {
        // Do nothing if clicking on dialog background or recursive dialog
        // background.
        if (TestWF(pwndNewFocus, WEFCONTROLPARENT))
            return;

        codeNewFocus = (UINT)SendMessage(hwndNewFocus, WM_GETDLGCODE, 0, 0L);
    }

    if (SAMEWOWHANDLE(hwndOldFocus, hwndNewFocus)) {
        //
        // NEW FOR 4.0:
        //
        // There is a very common frustrating scenario for ISVs who try to
        // set the default ID.  Our dialog manager assumes that if a push
        // button has the focus, it is the default button also.  As such
        // it passes in the focus window to this routine.  If someone tries
        // to change the focus or set the def ID such that they reside with
        // two different push buttons, the double-default-push button case
        // will result shortly.
        //
        // As such, for 4.0 dialogs, we will go check the def ID and see if
        // is the same as hwndOldFocus' ID.  If not, then we will find IT
        // and use that dude as hwndOldFocus
        //
        if (codeNewFocus & DLGC_UNDEFPUSHBUTTON)
        {
           if (TestWF(pwndDlg, WFWIN40COMPAT) && hwndOldFocus)
           {
               lT = (LONG)SendMessage(HWq(pwndDlg), DM_GETDEFID, 0, 0L);
               id = (HIWORD(lT) == DC_HASDEFID ? LOWORD(lT) : IDOK);
               lT = MAKELONG(id, 0);

               if (lT != PtrToLong(pwndNewFocus->spmenu))
               {
                   if (pwndOldFocus = _FindDlgItem(pwndDlg, lT))
                   {
                       hwndOldFocus = HW(pwndOldFocus);
                       if (SendMessage(hwndOldFocus, WM_GETDLGCODE, 0, 0L) & DLGC_DEFPUSHBUTTON)
                       {
                           xxxRemoveDefaultButton(pwndDlg, pwndOldFocus);
                           goto SetNewDefault;
                       }
                   }
               }
           }

           SendMessage(hwndNewFocus, BM_SETSTYLE, BS_DEFPUSHBUTTON, (LONG)TRUE);
        }
        return;
    }

    /*
     * If the focus is changing to or from a pushbutton, then remove the
     * default style from the current default button
     */
    if ((hwndOldFocus != NULL && (SendMessage(hwndOldFocus, WM_GETDLGCODE,
                0, 0) & (DLGC_DEFPUSHBUTTON | DLGC_UNDEFPUSHBUTTON))) ||
            (hwndNewFocus != NULL &&
                (codeNewFocus & (DLGC_DEFPUSHBUTTON | DLGC_UNDEFPUSHBUTTON)))) {
        xxxRemoveDefaultButton(pwndDlg, pwndNewFocus);
    }

SetNewDefault:
    /*
     * If moving to a button, make that button the default.
     */
    if (codeNewFocus & DLGC_UNDEFPUSHBUTTON) {
        SendMessage(hwndNewFocus, BM_SETSTYLE, BS_DEFPUSHBUTTON, (LONG)TRUE);
    } else {

        /*
         * Otherwise, make sure the original default button is default
         * and no others.
         */

        /*
         * Get the original default button handle
         */
        lT = (LONG)SendMessage(HWq(pwndDlg), DM_GETDEFID, 0, 0L);
        id = (HIWORD(lT) == DC_HASDEFID ? LOWORD(lT) : IDOK);
        pwndT = _FindDlgItem(pwndDlg, id);

        if (pwndT == NULL)
            return;
        ThreadLockAlways(pwndT, &tlpwndT);

        /*
         * If it already has the default button style, do nothing.
         */
        if ((styleT = (UINT)SendMessage(HWq(pwndT), WM_GETDLGCODE, 0, 0L)) & DLGC_DEFPUSHBUTTON) {
            ThreadUnlock(&tlpwndT);
            return;
        }

        /*
         * Also check to make sure it is really a button.
         */
        if (!(styleT & DLGC_UNDEFPUSHBUTTON)) {
            ThreadUnlock(&tlpwndT);
            return;
        }

        if (!TestWF(pwndT, WFDISABLED)) {
            SendMessage(HWq(pwndT), BM_SETSTYLE, BS_DEFPUSHBUTTON, (LONG)TRUE);
        }
        ThreadUnlock(&tlpwndT);
    }
}


/***************************************************************************\
* IsDialogMessage (API)
*
* History:
\***************************************************************************/

BOOL IsDialogMessageA(
    HWND hwndDlg,
    LPMSG lpmsg)
{
    WPARAM wParamSaved = lpmsg->wParam;
    BOOL bRet;

    switch (lpmsg->message) {
#ifdef FE_SB // IsDialogMessageA()
    case WM_CHAR:
    case EM_SETPASSWORDCHAR:
        /*
         * BUILD_DBCS_MESSAGE_TO_CLIENTW_FROM_CLIENTA() macro will return TRUE
         * for DBCS leadbyte message everytime, then we check there is some
         * possibility the return value become FALSE, here.
         *
         * These code originally come from IsDialogMessageW().
         */
         if (IS_DBCS_ENABLED()) {
            PWND pwndDlg, pwnd;
            TL tlpwndDlg;
            BOOL fLockDlg = FALSE;

            if ((pwndDlg = ValidateHwndNoRip(hwndDlg)) == NULL) {
                return FALSE;
            }

            if (lpmsg->hwnd == NULL) {
                return FALSE;
            }

            pwnd = ValidateHwnd(lpmsg->hwnd);
            //
            // THIS IS FOR MFC.
            //
            // This solves many problems with apps that use MFC but want to take
            // advantage of DS_CONTROL.  MFC blindly passes in child dialogs sometimes
            // to IsDialogMessage, which can screw up tabbing etc.
            //
            if (TestWF(pwndDlg, WEFCONTROLPARENT) && TestWF(pwndDlg, WFCHILD)) {
                pwndDlg = GetParentDialog(pwndDlg);
                ThreadLock(pwndDlg, &tlpwndDlg);
                fLockDlg = TRUE;
                hwndDlg = HWq(pwndDlg);
            }

            if (pwnd != pwndDlg && !_IsChild(pwndDlg, pwnd)) {
                if (fLockDlg)
                    ThreadUnlock(&tlpwndDlg);
                return FALSE;
            }

            /*
             * Build DBCS-aware message.
             */
            BUILD_DBCS_MESSAGE_TO_CLIENTW_FROM_CLIENTA(lpmsg->message,lpmsg->wParam,TRUE);

            /*
             * Fall through.....
             */
        }
#else
    case WM_CHAR:
    case EM_SETPASSWORDCHAR:
#endif // FE_SB
    case WM_CHARTOITEM:
    case WM_DEADCHAR:
    case WM_SYSCHAR:
    case WM_SYSDEADCHAR:
    case WM_MENUCHAR:
#ifdef FE_IME // IsDialogMessageA()
    case WM_IME_CHAR:
    case WM_IME_COMPOSITION:
#endif // FE_IME

        RtlMBMessageWParamCharToWCS(lpmsg->message, &lpmsg->wParam);
    }

    bRet = IsDialogMessageW(hwndDlg, lpmsg);

    /*
     * Restore the original ANSI char.
     */
    lpmsg->wParam = wParamSaved;
    return bRet;
}

BOOL IsDialogMessageW(
    HWND hwndDlg,
    LPMSG lpMsg)
{
    PWND pwndDlg;
    PWND pwnd;
    PWND pwnd2;
    HWND hwnd2;
    HWND hwndFocus;
    int iOK;
    BOOL fBack;
    UINT code;
    LONG lT;
    TL tlpwnd;
    TL tlpwndDlg;
    BOOL fLockDlg = FALSE;
    TL tlpwnd2;
    WORD langID;

    langID = PRIMARYLANGID(LANGIDFROMLCID(GetUserDefaultLCID()));

    if ((pwndDlg = ValidateHwndNoRip(hwndDlg)) == NULL) {
        return FALSE;
    }

    CheckLock(pwndDlg);

    /*
     * If this is a synchronous-only message (takes a pointer in wParam or
     * lParam), then don't allow this message to go through since those
     * parameters have not been thunked, and are pointing into outer-space
     * (which would case exceptions to occur).
     *
     * (This api is only called in the context of a message loop, and you
     * don't get synchronous-only messages in a message loop).
     */
    if (TESTSYNCONLYMESSAGE(lpMsg->message, lpMsg->wParam)) {
        /*
         * Fail if 32 bit app is calling.
         */
        if (!(GetClientInfo()->dwTIFlags & TIF_16BIT)) {
            RIPERR0(ERROR_MESSAGE_SYNC_ONLY, RIP_WARNING, "IsDialogMessage: must be sync only");
            return FALSE;
        }

        /*
         * For wow apps, allow it to go through (for compatibility). Change
         * the message id so our code doesn't understand the message - wow
         * will get the message and strip out this bit before dispatching
         * the message to the application.
         */
        lpMsg->message |= MSGFLAG_WOW_RESERVED;
    }

    if (CallMsgFilter(lpMsg, MSGF_DIALOGBOX))
        return TRUE;

    if (lpMsg->hwnd == NULL) {
        return FALSE;
    }

    pwnd = ValidateHwnd(lpMsg->hwnd);
    //
    // THIS IS FOR MFC.
    //
    // This solves many problems with apps that use MFC but want to take
    // advantage of DS_CONTROL.  MFC blindly passes in child dialogs sometimes
    // to IsDialogMessage, which can screw up tabbing etc.
    //
    if (TestWF(pwndDlg, WEFCONTROLPARENT) && TestWF(pwndDlg, WFCHILD)) {
        pwndDlg = GetParentDialog(pwndDlg);
        ThreadLock(pwndDlg, &tlpwndDlg);
        fLockDlg = TRUE;
        hwndDlg = HWq(pwndDlg);
    }

    if (pwnd != pwndDlg && !_IsChild(pwndDlg, pwnd)) {
        if (fLockDlg)
            ThreadUnlock(&tlpwndDlg);
        return FALSE;
    }
    ThreadLock(pwnd, &tlpwnd);

    fBack = FALSE;
    iOK = IDCANCEL;
    switch (lpMsg->message) {
    case WM_LBUTTONDOWN:

        /*
         * Move the default button styles around on button clicks in the
         * same way as TABs.
         */
        if ((pwnd != pwndDlg) && ((hwndFocus = GetFocus()) != NULL)) {
            xxxCheckDefPushButton(pwndDlg, hwndFocus, lpMsg->hwnd);
        }
        break;

    case WM_SYSCHAR:

        /*
         * If no control has focus, and Alt not down, then ignore.
         */
        if ((GetFocus() == NULL) && (GetKeyState(VK_MENU) >= 0)) {
            if (lpMsg->wParam == VK_RETURN && TestWF(pwnd, WFMINIMIZED)) {

                /*
                 * If this is an iconic dialog box window and the user hits
                 * return, send the message off to DefWindowProc so that it
                 * can be restored.  Especially useful for apps whose top
                 * level window is a dialog box.
                 */
                goto CallDefWindowProcAndReturnTrue;
            } else {
                NtUserMessageBeep(0);
            }

            ThreadUnlock(&tlpwnd);
            if (fLockDlg)
                ThreadUnlock(&tlpwndDlg);
            return TRUE;
        }

        /*
         * If alt+menuchar, process as menu.
         */
        if (lpMsg->wParam == MENUSYSMENU) {
            DefWindowProcWorker(pwndDlg, lpMsg->message, lpMsg->wParam,
                    lpMsg->lParam, FALSE);
            ThreadUnlock(&tlpwnd);
            if (fLockDlg)
                ThreadUnlock(&tlpwndDlg);
            return TRUE;
        }

    /*
     *** FALL THRU **
     */

    case WM_CHAR:

        /*
         * Ignore chars sent to the dialog box (rather than the control).
         */
        if (pwnd == pwndDlg) {
            ThreadUnlock(&tlpwnd);
            if (fLockDlg)
                ThreadUnlock(&tlpwndDlg);
            return TRUE;
        }

        code = (UINT)SendMessage(lpMsg->hwnd, WM_GETDLGCODE, lpMsg->wParam,
                (LPARAM)lpMsg);

        /*
         * If the control wants to process the message, then don't check for
         * possible mnemonic key.
         */
        if ((lpMsg->message == WM_CHAR) && (code & (DLGC_WANTCHARS | DLGC_WANTMESSAGE)))
            break;

        /* If the control wants tabs, then don't let tab fall thru here
         */
        if ((lpMsg->wParam == VK_TAB) && (code & DLGC_WANTTAB))
            break;


        /*
         * HACK ALERT
         *
         * If ALT is held down (i.e., SYSCHARs), then ALWAYS do mnemonic
         * processing.  If we do away with SYSCHARS, then we should
         * check key state of ALT instead.
         */

        /*
         * Space is not a valid mnemonic, but it IS the char that toggles
         * button states.  Don't look for it as a mnemonic or we will
         * beep when it is typed....
         */
        if (lpMsg->wParam == VK_SPACE) {
            ThreadUnlock(&tlpwnd);
            if (fLockDlg)
                ThreadUnlock(&tlpwndDlg);
            return TRUE;
        }

        if (!(pwnd2 = xxxGotoNextMnem(pwndDlg, pwnd, (WCHAR)lpMsg->wParam))) {

            if (code & DLGC_WANTMESSAGE)
                break;

            /*
             * No mnemonic could be found so we will send the sys char over
             * to xxxDefWindowProc so that any menu bar on the dialog box is
             * handled properly.
             */
            if (lpMsg->message == WM_SYSCHAR) {
CallDefWindowProcAndReturnTrue:
                DefWindowProcWorker(pwndDlg, lpMsg->message, lpMsg->wParam,
                        lpMsg->lParam, FALSE);

                ThreadUnlock(&tlpwnd);
                if (fLockDlg)
                    ThreadUnlock(&tlpwndDlg);
                return TRUE;
            }
            NtUserMessageBeep(0);
        } else {

            /*
             * pwnd2 is 1 if the mnemonic took us to a pushbutton.  We
             * don't change the default button status here since doing this
             * doesn't change the focus.
             */
            if (pwnd2 != (PWND)1) {
                ThreadLockAlways(pwnd2, &tlpwnd2);
                xxxCheckDefPushButton(pwndDlg, lpMsg->hwnd, HWq(pwnd2));
                ThreadUnlock(&tlpwnd2);
            }
        }

        ThreadUnlock(&tlpwnd);
        if (fLockDlg)
            ThreadUnlock(&tlpwndDlg);
        return TRUE;

    case WM_SYSKEYDOWN:
        /*
         * If Alt is down, deal with keyboard cues
         */
        if ((HIWORD(lpMsg->lParam) & SYS_ALTERNATE) && TEST_KbdCuesPUSIF) {
            if (TestWF(pwnd, WEFPUIFOCUSHIDDEN) || (TestWF(pwnd, WEFPUIACCELHIDDEN))) {
                    SendMessageWorker(pwndDlg, WM_CHANGEUISTATE,
                                      MAKEWPARAM(UIS_CLEAR, UISF_HIDEACCEL | UISF_HIDEFOCUS), 0, FALSE);
                }
        }
        break;

    case WM_KEYDOWN:
        code = (UINT)SendMessage(lpMsg->hwnd, WM_GETDLGCODE, lpMsg->wParam,
                (LPARAM)lpMsg);
        if (code & (DLGC_WANTALLKEYS | DLGC_WANTMESSAGE))
            break;

        switch (lpMsg->wParam) {
        case VK_TAB:
            if (code & DLGC_WANTTAB)
                break;
            pwnd2 = _GetNextDlgTabItem(pwndDlg, pwnd,
                    (GetKeyState(VK_SHIFT) & 0x8000));

            if (TEST_KbdCuesPUSIF) {
                if (TestWF(pwnd, WEFPUIFOCUSHIDDEN)) {
                    SendMessageWorker(pwndDlg, WM_CHANGEUISTATE,
                                          MAKEWPARAM(UIS_CLEAR, UISF_HIDEFOCUS), 0, FALSE);
                }
            }

            if (pwnd2 != NULL) {
                hwnd2 = HWq(pwnd2);
                ThreadLockAlways(pwnd2, &tlpwnd2);
                DlgSetFocus(hwnd2);
                xxxCheckDefPushButton(pwndDlg, lpMsg->hwnd, hwnd2);
                ThreadUnlock(&tlpwnd2);
            }
            ThreadUnlock(&tlpwnd);
            if (fLockDlg)
                ThreadUnlock(&tlpwndDlg);
            return TRUE;

        /*
         * For Arabic and Hebrew locales the arrow keys are reversed. Also reverse them if
         * the dialog is RTL mirrored.
         */
        case VK_LEFT:
            if ((((langID == LANG_ARABIC) || (langID == LANG_HEBREW)) && TestWF(pwndDlg,WEFRTLREADING))
#ifdef USE_MIRRORING
               ^ (!!TestWF(pwndDlg, WEFLAYOUTRTL))
#endif
               )
                goto DoKeyStuff;
        case VK_UP:
            fBack = TRUE;
            goto DoKeyStuff;

        /*
         *** FALL THRU **
         */
        case VK_RIGHT:
            if ((((langID == LANG_ARABIC) || (langID == LANG_HEBREW)) && TestWF(pwndDlg,WEFRTLREADING)) 
#ifdef USE_MIRRORING
                 ^ (!!TestWF(pwndDlg, WEFLAYOUTRTL))
#endif
               )
                fBack = TRUE;
        case VK_DOWN:
DoKeyStuff:
            if (code & DLGC_WANTARROWS)
                break;

            if (TEST_KbdCuesPUSIF) {
                if (TestWF(pwnd, WEFPUIFOCUSHIDDEN)) {
                        SendMessageWorker(pwndDlg, WM_CHANGEUISTATE,
                                          MAKEWPARAM(UIS_CLEAR, UISF_HIDEFOCUS), 0, FALSE);
                    }
            }

            pwnd2 = _GetNextDlgGroupItem(pwndDlg, pwnd, fBack);
            if (pwnd2 == NULL) {
                ThreadUnlock(&tlpwnd);
                if (fLockDlg)
                    ThreadUnlock(&tlpwndDlg);
                return TRUE;
            }
            hwnd2 = HWq(pwnd2);
            ThreadLockAlways(pwnd2, &tlpwnd2);

            code = (UINT)SendMessage(hwnd2, WM_GETDLGCODE, lpMsg->wParam,
                    (LPARAM)lpMsg);

            /*
             * We are just moving the focus rect around! So, do not send
             * BN_CLICK messages, when WM_SETFOCUSing.  Fix for Bug
             * #4358.
             */
            if (code & (DLGC_UNDEFPUSHBUTTON | DLGC_DEFPUSHBUTTON)) {
                PBUTN pbutn;
                BOOL fIsNTButton = (GETFNID(pwnd2) == FNID_BUTTON);
                if (fIsNTButton) {
                    pbutn = ((PBUTNWND)pwnd2)->pbutn;
                    BUTTONSTATE(pbutn) |= BST_DONTCLICK;
                }
                DlgSetFocus(hwnd2);
                if (fIsNTButton) {
                    BUTTONSTATE(pbutn) &= ~BST_DONTCLICK;
                }
                xxxCheckDefPushButton(pwndDlg, lpMsg->hwnd, hwnd2);
            } else if (code & DLGC_RADIOBUTTON) {
                DlgSetFocus(hwnd2);
                xxxCheckDefPushButton(pwndDlg, lpMsg->hwnd, hwnd2);
                if (TestWF(pwnd2, BFTYPEMASK) == LOBYTE(BS_AUTORADIOBUTTON)) {

                    /*
                     * So that auto radio buttons get clicked on
                     */
                    if (!SendMessage(hwnd2, BM_GETCHECK, 0, 0L)) {
                        SendMessage(hwnd2, BM_CLICK, TRUE, 0L);
                    }
                }
            } else if (!(code & DLGC_STATIC)) {
                DlgSetFocus(hwnd2);
                xxxCheckDefPushButton(pwndDlg, lpMsg->hwnd, hwnd2);
            }
            ThreadUnlock(&tlpwnd2);
            ThreadUnlock(&tlpwnd);
            if (fLockDlg)
                ThreadUnlock(&tlpwndDlg);
            return TRUE;

        case VK_EXECUTE:
        case VK_RETURN:

            /*
             * Guy pressed return - if button with focus is
             * defpushbutton, return its ID.  Otherwise, return id
             * of original defpushbutton.
             */
            if (!(hwndFocus = GetFocus()))
                code = 0;
            else
            {
                code = (WORD)(DWORD)SendMessage(hwndFocus, WM_GETDLGCODE,
                    0, 0L);
            }

            if (code & DLGC_DEFPUSHBUTTON)
            {
                iOK = GetDlgCtrlID(hwndFocus);
                pwnd2 = ValidateHwnd(hwndFocus);
                goto HaveWindow;
            }
            else
            {
                lT = (LONG)SendMessage(hwndDlg, DM_GETDEFID, 0, 0L);
                iOK = MAKELONG(
                    (HIWORD(lT)==DC_HASDEFID ? LOWORD(lT) : IDOK),
                    0);
            }
            // FALL THRU

        case VK_ESCAPE:
        case VK_CANCEL:

            /*
             * Make sure button is not disabled.
             */
            pwnd2 = _FindDlgItem(pwndDlg, iOK);
HaveWindow:
            if (pwnd2 != NULL && TestWF(pwnd2, WFDISABLED)) {
                NtUserMessageBeep(0);
            } else {
                SendMessage(hwndDlg, WM_COMMAND,
                        MAKELONG(iOK, BN_CLICKED), (LPARAM)HW(pwnd2));
            }

            ThreadUnlock(&tlpwnd);
            if (fLockDlg)
                ThreadUnlock(&tlpwndDlg);
            return TRUE;
        }
        break;
    }

    ThreadUnlock(&tlpwnd);
    if (fLockDlg)
        ThreadUnlock(&tlpwndDlg);

    TranslateMessage(lpMsg);
    DispatchMessage(lpMsg);

    return TRUE;
}

/***************************************************************************\
*
* FindDlgItem32()
*
* Given a dialog, finds the window with the given ID anywhere w/in the
* descendant chain.
*
\***************************************************************************/

PWND _FindDlgItem(PWND pwndParent, DWORD id)
{
    PWND    pwndChild;
    PWND    pwndOrig;

    // QUICK TRY:
    pwndChild = _GetDlgItem(pwndParent, id);
    if (pwndChild || !TestWF(pwndParent, WFWIN40COMPAT))
        return(pwndChild);

    pwndOrig = _NextControl(pwndParent, NULL, CWP_SKIPINVISIBLE);
    if (pwndOrig == pwndParent)
        return(NULL);

    pwndChild = pwndOrig;

//    VerifyWindow(pwndChild);

    do
    {
        if (PtrToUlong(pwndChild->spmenu) == id)
            return(pwndChild);

        pwndChild = _NextControl(pwndParent, pwndChild, CWP_SKIPINVISIBLE);
    }
    while (pwndChild && (pwndChild != pwndOrig));

    return(NULL);
}
