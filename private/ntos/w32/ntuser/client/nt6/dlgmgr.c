/***************************************************************************\
*
*  DLGMGR.C -
*
*      Dialog Box Manager Routines
*
* ??-???-???? mikeke    Ported from Win 3.0 sources
* 12-Feb-1991 mikeke    Added Revalidation code
* 19-Feb-1991 JimA      Added access checks
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define UNICODE_MINUS_SIGN 0x2212

// FA
LRESULT ResizeDlgMessage( LPVOID pObject, 
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
	);


LOOKASIDE DialogLookaside;

BOOL ValidateCallback(HANDLE h);

#define IsInForegroundQueue(hwnd) \
    (NtUserQueryWindow(hwnd, WindowIsForegroundThread) != NULL)
#define IsCurrentThreadForeground() \
    ((BOOL)NtUserGetThreadState(UserThreadStateIsForeground))

/***************************************************************************\
*
* GetParentDialog()
*
* Gets top level window, not a control parent.  If not a dialog, then use
* "highest" control parent guy.
*
* BOGUS
* Need a way to mark a window as a dialog.  If it ever comes into
* DefDlgProc(), set an internal flag.  Will be used by thunking and
* CallDlgProc() optimizations also!
*
\***************************************************************************/

PWND GetParentDialog(PWND pwndDialog)
{
    PWND    pwndParent;

    pwndParent = pwndDialog;

    //
    // Walk up the parent chain.  We're looking for the top-most dialog
    // window.  Most cases, the window is a top level one.  But in case of
    // backup app, the window will be a child of some other window.
    //
    for (; pwndDialog; pwndDialog = REBASEPWND(pwndDialog, spwndParent))
    {
        if (TestWF(pwndDialog, WFDIALOGWINDOW))
        {
            //
            // For old guys:  If not DS_RECURSE, then stop here.
            // that way old apps which try to do the nested dialog
            // stuff in their old limited way don't die.
            //
            if (TestWF(pwndDialog, WEFCONTROLPARENT))
                pwndParent = pwndDialog;
            else if (!TestWF(pwndDialog, DFCONTROL))
                break;
        }

        if (!TestWF(pwndDialog, WFCHILD))
            break;
    }

    return(pwndParent);
}

/***************************************************************************\
* xxxSaveDlgFocus
*
* History:
* 02-18-92 JimA             Ported from Win31 sources
\***************************************************************************/

BOOL xxxSaveDlgFocus(
    PWND pwnd)
{
    HWND hwndFocus = GetFocus();

    CheckLock(pwnd);

    if (hwndFocus != NULL && IsChild(HWq(pwnd), hwndFocus) &&
            PDLG(pwnd)->hwndFocusSave == NULL) {
        PDLG(pwnd)->hwndFocusSave = hwndFocus;
        xxxRemoveDefaultButton(pwnd, ValidateHwnd(hwndFocus));
        return TRUE;
    }
    return FALSE;
}

/***************************************************************************\
* xxxRestoreDlgFocus
*
* History:
* 02-18-92 JimA             Ported from Win31 sources
\***************************************************************************/

// LATER
// 21-Mar-1992 mikeke
// does pwndFocusSave need to be unlocked when the dialog is destroyed?

BOOL xxxRestoreDlgFocus(
    PWND pwnd)
{
    HWND hwndFocus;
    HWND hwndFocusSave;
    BOOL fRestored = FALSE;

    CheckLock(pwnd);

    if (PDLG(pwnd)->hwndFocusSave && !TestWF(pwnd, WFMINIMIZED)) {

        hwndFocus = GetFocus();
        hwndFocusSave = PDLG(pwnd)->hwndFocusSave;

        if (IsWindow(hwndFocusSave)) {
            xxxCheckDefPushButton(pwnd, hwndFocus, hwndFocusSave);
            fRestored = (NtUserSetFocus(hwndFocusSave) != NULL);
#ifdef FE_IME
            //
            // after calling SetFocus(), we need to re-validate
            // the window. PDLG(pwnd) might be NULL. This can
            // be happened in FE environment where IME window
            // exist. (kkntbug #12613)
            //
            if (IS_IME_ENABLED() && !ValidateDialogPwnd(pwnd)) {
                return fRestored;
        }
#endif
        }
        PDLG(pwnd)->hwndFocusSave = NULL;
    }

    return fRestored;
}


/***************************************************************************\
* DlgSetFocus
*
* History:
\***************************************************************************/

void DlgSetFocus(
    HWND hwnd)
{
    if (((UINT)SendMessage(hwnd, WM_GETDLGCODE, 0, 0)) & DLGC_HASSETSEL) {
        SendMessage(hwnd, EM_SETSEL, 0, MAXLONG);
    }

    NtUserSetFocus(hwnd);
}


int GetDlgCtrlID(
    HWND hwnd)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);
    if (pwnd == NULL)
        return 0;

    return PtrToLong(pwnd->spmenu);
}



/***************************************************************************\
* ValidateDialogPwnd
*
* Under Win3, DLGWINDOWEXTRA is 30 bytes. We cannot change that for 16 bit
* compatibility reasons. Problem is there is no way to tell if a given
* 16 bit window depends on byte count. If there was, this would be easy.
* The only way to tell is when a window is about to be used as a dialog
* window. This window may be of the class DIALOGCLASS, but again it may
* not!! So we keep dialog window words at 30 bytes, and allocate another
* structure for the real dialog structure fields. Problem is that this
* structure has to be created lazily! And that's what we're doing here.
*
* 05-21-91 ScottLu      Created.
\***************************************************************************/

BOOL ValidateDialogPwnd(
    PWND pwnd)
{
    static BOOL sfInit = TRUE;
    PDLG pdlg;

    /*
     * This bit is set if we've already run through this initialization and
     * have identified this window as a dialog window (able to withstand
     * peeks into window words at random moments in time).
     */
    if (TestWF(pwnd, WFDIALOGWINDOW))
        return TRUE;

    if (pwnd->cbwndExtra < DLGWINDOWEXTRA) {
        RIPERR0(ERROR_WINDOW_NOT_DIALOG, RIP_VERBOSE, "");
        return FALSE;
    }

    /*
     * See if the pdlg was destroyed and this is a rogue message to be ignored
     */
    if (pwnd->fnid & FNID_STATUS_BITS) {
        return FALSE;
    }

    /*
     * If the lookaside buffer has not been initialized, do it now.
     */
    if (sfInit) {
        if (!NT_SUCCESS(InitLookaside(&DialogLookaside, sizeof(DLG), 2))) {
            return FALSE;
        }
        sfInit = FALSE;
    }

    if ((pdlg = (PDLG)AllocLookasideEntry(&DialogLookaside)) == NULL) {
        return FALSE;
    }

    NtUserCallHwndParam(HWq(pwnd), (ULONG_PTR)pdlg, SFI_SETDIALOGPOINTER);

    return TRUE;
}


/***************************************************************************\
* CvtDec
*
* LATER!!! convert to itoa?
*
* History:
\***************************************************************************/

void CvtDec(
    int u,
    LPWSTR *lplpch)
{
    if (u >= 10) {
        CvtDec(u / 10, lplpch);
        u %= 10;
    }

    *(*lplpch)++ = (WCHAR)(u + '0');
}


/***************************************************************************\
* SetDlgItemInt
*
* History:
\***************************************************************************/

BOOL SetDlgItemInt(
    HWND hwnd,
    int item,
    UINT u,
    BOOL fSigned)
{
    LPWSTR lpch;
    WCHAR rgch[16];

    lpch = rgch;
    if (fSigned) {
        if ((int)u < 0) {
            *lpch++ = TEXT('-');
            u = (UINT)(-(int)u);
        }
    } else {
        if (u & 0x80000000) {
            CvtDec(u / 10, (LPWSTR FAR *)&lpch);
            u = u % 10;
        }
    }

    CvtDec(u, (LPWSTR FAR *)&lpch);
    *lpch = 0;

    return SetDlgItemTextW(hwnd, item, rgch);
}


/***************************************************************************\
* CheckDlgButton
*
* History:
\***************************************************************************/

BOOL CheckDlgButton(
    HWND hwnd,
    int id,
    UINT cmdCheck)
{
    if ((hwnd = GetDlgItem(hwnd, id)) == NULL) {
        return FALSE;
    }

    SendMessage(hwnd, BM_SETCHECK, cmdCheck, 0);

    return TRUE;
}


/***************************************************************************\
* GetDlgItemInt
*
* History:
\***************************************************************************/

UINT GetDlgItemInt(
    HWND hwnd,
    int item,
    BOOL FAR *lpfValOK,
    BOOL fSigned)
{
    int i, digit, ch;
    BOOL fOk, fNeg;
    LPWSTR lpch;
    WCHAR rgch[48];
    WCHAR rgchDigits[48];

    fOk = FALSE;
    if (lpfValOK != NULL)
        *lpfValOK = FALSE;

    if (!GetDlgItemTextW(hwnd, item, rgch, sizeof(rgch)/sizeof(WCHAR) - 1))
        return 0;

    lpch = rgch;

    /*
     * Skip leading white space.
     */
    while (*lpch == TEXT(' '))
        lpch++;

    fNeg = FALSE;
    while (fSigned && ((*lpch == L'-') || (*lpch == UNICODE_MINUS_SIGN))) {
        lpch++;
        fNeg ^= TRUE;
    }

    /*
     * Convert all decimal digits to ASCII Unicode digits 0x0030 - 0x0039
     */
    FoldStringW(MAP_FOLDDIGITS, lpch, -1, rgchDigits,
            sizeof(rgchDigits)/sizeof(rgchDigits[0]));
    lpch = rgchDigits;

    i = 0;
    while (ch = *lpch++) {
        digit = ch - TEXT('0');
        if (digit < 0 || digit > 9) {
            break;
        }
        if (fSigned) {
            if (i > (INT_MAX - digit) / 10) {
                return(0);
            }
        } else {
            if ((UINT)i > (UINT)((UINT_MAX - digit) / 10)) {
                return(0);
            }
        }

        fOk = TRUE;
        i = ((UINT)i * 10) + digit;
    }

    if (fNeg)
        i = -i;

    if (lpfValOK != NULL)
        *lpfValOK = ((ch == 0) && fOk);

    return (UINT)i;
}

/***************************************************************************\
* CheckRadioButton
*
* History:
\***************************************************************************/

BOOL CheckRadioButton(
    HWND hwnd,
    int idFirst,
    int idLast,
    int id)
{
    PWND pwnd, pwndDialog;
    BOOL    fCheckOn;

    pwndDialog = ValidateHwnd(hwnd);
    if (pwndDialog == NULL)
        return 0;

    for (pwnd = REBASE(pwndDialog, spwndChild); pwnd; pwnd = REBASE(pwnd, spwndNext)) {

        if ((PtrToLong(pwnd->spmenu) >= idFirst) &&
            (PtrToLong(pwnd->spmenu) <= idLast)) {

            fCheckOn = (PtrToLong(pwnd->spmenu) == id);
            SendMessage(PtoHq(pwnd), BM_SETCHECK, fCheckOn, 0L);
        }
    }

    return TRUE;
}


/***************************************************************************\
* IsDlgButtonChecked
*
* History:
\***************************************************************************/

UINT IsDlgButtonChecked(
    HWND hwnd,
    int id)
{
    if ((hwnd = GetDlgItem(hwnd, id)) != NULL) {
        return (UINT)SendMessage(hwnd, BM_GETCHECK, 0, 0);
    }

    return FALSE;
}


/***************************************************************************\
* DefDlgProc
*
* History:
\***************************************************************************/

LRESULT DefDlgProcWorker(
    PWND pwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    DWORD fAnsi)
{
    HWND hwnd = HWq(pwnd);
    TL tlpwndT1, tlpwndT2, tlpwndT3, tlpwndTop;
    PWND pwndT;
    PWND pwndT1, pwndT2, pwndT3, pwndTop;
    HWND hwndT1;
    LRESULT result;
    BOOL fSetBit;
    DLGPROC pfn;

    CheckLock(pwnd);

    /*
     * use the Win 3.1 documented size
     */
    VALIDATECLASSANDSIZE(pwnd, FNID_DIALOG);

    /*
     * Must do special validation here to make sure pwnd is a dialog window.
     */
    if (!ValidateDialogPwnd(pwnd))
        return 0;

    if (((PDIALOG)pwnd)->resultWP != 0)
        NtUserSetWindowLongPtr(hwnd, DWLP_MSGRESULT, 0, FALSE);
    result = 0;   // no dialog proc

    if (message == WM_FINALDESTROY) {
        goto DoCleanup;
    }

    if ((pfn = PDLG(pwnd)->lpfnDlg) != NULL) {
        if (IsWOWProc(pfn)) {
            result = (*pfnWowDlgProcEx)(hwnd, message, wParam, lParam, (ULONG_PTR)pfn, &(pwnd->state));
        } else {
            result = (pfn)(hwnd, message, wParam, lParam);
        }

        /*
         * Get out if the window was destroyed in the dialog proc.
         */
        if ((RevalidateHwnd(hwnd)==NULL) || (pwnd->fnid & FNID_STATUS_BITS))
            return result;
    }

    /*
     * SPECIAL CASED ... and DOCUMENTED that way !!!
     * These 6, and ONLY these 6, should be hacked in this fashion.
     * Anybody who needs the REAL return value to a message should
     * use SetDlgMsgResult in WINDOWSX.H
     */

    switch (message)
    {
        case WM_INITDIALOG:
			//
			// FA
			//
			if( PDLG(pwnd)->pDlgResize != NULL )
				ResizeDlgMessage( PDLG(pwnd)->pDlgResize, hwnd, message, wParam, lParam );
			// fall through
        case WM_COMPAREITEM:
        case WM_VKEYTOITEM:
        case WM_CHARTOITEM:
        case WM_QUERYDRAGICON:
            return ((LRESULT)(DWORD)result);

        case WM_CTLCOLOR:
        case WM_CTLCOLORMSGBOX:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSCROLLBAR:
        case WM_CTLCOLORSTATIC:
            // QuarkXPress doesn't like finding the WM_CTLCOLOR result in
            // resultWP -- we should never be setting resultWP -- that's meant
            // as a pass-thru return value -- so let's go back to doing it the
            // old way -- Win95B B#21269 -- 03/13/95 -- tracysh (cr: jeffbog)
            if (result)
                return ((LRESULT)(DWORD)result);
            break;
    }

    if (!result) {

        /*
         * Save the result value in case our private memory is freed
         * before we return
         */
//        result = PDLG(pwnd)->resultWP;

        switch (message) {
        case WM_CTLCOLOR:
        case WM_CTLCOLORMSGBOX:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSCROLLBAR:
        case WM_CTLCOLORSTATIC:
        {
            //
            // HACK OF DEATH:
            // To get 3D colors for non 4.0 apps who use 3DLOOK,
            // we temporarily add on the 4.0 compat bit, pass this
            // down to DWP, and clear it.
            //
            // Use "result" var for bool saying we have to add/clear 4.0
            // compat bit.

            fSetBit = (TestWF(pwnd, DF3DLOOK)!= 0) &&
                     (TestWF(pwnd, WFWIN40COMPAT) == 0);

            if (fSetBit)
                SetWindowState(pwnd, WFWIN40COMPAT);

            result = DefWindowProcWorker(pwnd, message,
                    wParam, lParam, fAnsi);

            if (fSetBit)
                ClearWindowState(pwnd, WFWIN40COMPAT);
            return((LRESULT)(DWORD) result);
        }

        case WM_ERASEBKGND:
            FillWindow(hwnd, hwnd, (HDC)wParam, (HBRUSH)CTLCOLOR_DLG);

			//
			// FA
			if( PDLG(pwnd)->pDlgResize != NULL )
				return ResizeDlgMessage( PDLG(pwnd)->pDlgResize, hwnd, message, wParam, lParam );
            return TRUE;

        case WM_SHOWWINDOW:

            /*
             * If hiding the window, save the focus.  If showing the window
             * by means of a SW_* command and the fEnd bit is set, do not
             * pass to DWP so it won't get shown.
             */
            if (GetParentDialog(pwnd) == pwnd) {
                if (!wParam) {
                    xxxSaveDlgFocus(pwnd);
                } else {

                    if (LOWORD(lParam) != 0 && PDLG(pwnd)->fEnd)
                        break;

                    /*
                     * Snap the cursor to the center of the default button.
                     * Only do this if the current thread is in the foreground.
                     * The _ShowCursor() code is added to work around a
                     * problem with hardware cursors.  If change is done
                     * in the same refresh cycle, the display of the cursor
                     * would not reflect the new position.
                     */
                    if (TEST_PUSIF(PUSIF_SNAPTO) &&
                            IsInForegroundQueue(hwnd)) {
                        hwndT1 = GetDlgItem(hwnd, PDLG(pwnd)->result);
                        if (hwndT1) {
                            RECT rc;

                            NtUserShowCursor(FALSE);

                            GetWindowRect(hwndT1, &rc);
                            NtUserSetCursorPos(rc.left + ((rc.right - rc.left)/2),
                                         rc.top + ((rc.bottom - rc.top)/2));

                            NtUserShowCursor(TRUE);
                        }
                    }
                }
            }
            goto CallDWP;

        case WM_SYSCOMMAND:
            if (GetParentDialog(pwnd) == pwnd) {
                /*
                 * If hiding the window, save the focus.  If showing the window
                 * by means of a SW_* command and the fEnd bit is set, do not
                 * pass to DWP so it won't get shown.
                 */
                if ((int)wParam == SC_MINIMIZE)
                    xxxSaveDlgFocus(pwnd);
            }
            goto CallDWP;

        case WM_ACTIVATE:
            pwndT1 = GetParentDialog(pwnd);
            if ( pwndT1 != pwnd) {

                /*
                 * This random bit is used during key processing - bit
                 * 08000000 of WM_CHAR messages is set if a dialog is currently
                 * active.
                 */
                NtUserSetThreadState(wParam ? QF_DIALOGACTIVE : 0, QF_DIALOGACTIVE);
            }
            ThreadLock(pwndT1, &tlpwndT1);
            if (wParam != 0)
                xxxRestoreDlgFocus(pwndT1);
            else
                xxxSaveDlgFocus(pwndT1);

            ThreadUnlock(&tlpwndT1);
            break;

        case WM_SETFOCUS:
            pwndT1 = GetParentDialog(pwnd);
            if (!PDLG(pwndT1)->fEnd && !xxxRestoreDlgFocus(pwndT1)) {

                pwndT = _GetNextDlgTabItem(pwndT1, NULL, FALSE);
                DlgSetFocus(HW(pwndT));
            }
            break;

        case WM_CLOSE:
            /*
             * Make sure cancel button is not disabled before sending the
             * IDCANCEL.  Note that we need to do this as a message instead
             * of directly calling the dlg proc so that any dialog box
             * filters get this.
             */
            pwndT1 = _GetDlgItem(pwnd, IDCANCEL);
            if (pwndT1 && TestWF(pwndT1, WFDISABLED))
                NtUserMessageBeep(0);
            else
                PostMessage(hwnd, WM_COMMAND, MAKELONG(IDCANCEL, BN_CLICKED),
                        (LPARAM)HW(pwndT1));
            break;

        case WM_NCDESTROY:
        case WM_FINALDESTROY:
DoCleanup:
            NtUserSetThreadState(0, QF_DIALOGACTIVE);
            if (!(pwnd->style & DS_LOCALEDIT)) {
                if (PDLG(pwnd)->hData) {
                    ReleaseEditDS(PDLG(pwnd)->hData);
                    PDLG(pwnd)->hData = NULL;
                }
            }

            /*
             * Delete the user defined font if any
             */
            if (PDLG(pwnd)->hUserFont) {
                DeleteObject(PDLG(pwnd)->hUserFont);
                PDLG(pwnd)->hUserFont = NULL;
            }

            /*
             * Free the dialog memory and mark this as a non-dialog window
             */
            FreeLookasideEntry(&DialogLookaside, PDLG(pwnd));
            NtUserCallHwndParam(hwnd, 0, SFI_SETDIALOGPOINTER);
            break;

        case DM_REPOSITION:
            {
                RECT        rc;
                PMONITOR    pMonitor;

                // DAT recorder APP sends it's own private message 0x402
                // through and we mistake it to be DM_REPOSITION. To avoid
                // this confusion, we do the following check.
                // Fix for Bug#25747 -- 9/29/94 --
                if (!TestWF(pwnd, WEFCONTROLPARENT) ||
                    (GETFNID(pwnd) != FNID_DESKTOP &&
                     GETFNID(REBASEPWND(pwnd, spwndParent)) != FNID_DESKTOP)) {

                    goto CallDWP;
                }

                CopyRect(&rc, &pwnd->rcWindow);
                pMonitor = _MonitorFromRect(&rc, MONITOR_DEFAULTTOPRIMARY);
                RepositionRect(pMonitor, &rc, pwnd->style, pwnd->ExStyle);
                NtUserSetWindowPos(hwnd, HWND_TOP, rc.left, rc.top,
                             rc.right-rc.left, rc.bottom-rc.top,
                             SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
            }
            break;

        case DM_SETDEFID:
            pwndT1 = GetParentDialog(pwnd);
            ThreadLock(pwndT1, &tlpwndT1);

            if (!(PDLG(pwndT1)->fEnd)) {

                pwndT2 = NULL;
                if (PDLG(pwndT1)->result != 0)
                    pwndT2 = _FindDlgItem(pwndT1, PDLG(pwndT1)->result);

                pwndT3 = NULL;
                if (wParam != 0) {
                    pwndT3 = _GetDlgItem(pwnd, wParam);
                }

                ThreadLock(pwndT2, &tlpwndT2);
                ThreadLock(pwndT3, &tlpwndT3);

                xxxCheckDefPushButton(pwndT1, HW(pwndT2), HW(pwndT3));

                ThreadUnlock(&tlpwndT3);
                ThreadUnlock(&tlpwndT2);

                PDLG(pwndT1)->result = wParam;
//                if (PDLG(pwnd)->spwndFocusSave) {
//                    Lock(&(PDLG(pwnd)->spwndFocusSave), pwndT2);
//                }

                if (FWINABLE()) {
                    NotifyWinEvent(EVENT_OBJECT_DEFACTIONCHANGE, HW(pwndT1), OBJID_CLIENT, INDEXID_CONTAINER);
                }
            }
            ThreadUnlock(&tlpwndT1);
            return TRUE;

        case DM_GETDEFID:
            pwndT1 = GetParentDialog(pwnd);

            if (!PDLG(pwndT1)->fEnd && PDLG(pwndT1)->result)
                return(MAKELONG(PDLG(pwndT1)->result, DC_HASDEFID));
            else
                return 0;
            break;

        /*
         * This message was added so that user defined controls that want
         * tab keys can pass the tab off to the next/previous control in the
         * dialog box.  Without this, all they could do was set the focus
         * which didn't do the default button stuff.
         */
        case WM_NEXTDLGCTL:
            pwndTop = GetParentDialog(pwnd);
            ThreadLock(pwndTop, &tlpwndTop);

            hwndT1 = GetFocus();
            pwndT2 = ValidateHwndNoRip(hwndT1);
            if (LOWORD(lParam)) {
                if (pwndT2 == NULL)
                    pwndT2 = pwndTop;

                /*
                 * wParam contains the pwnd of the ctl to set focus to.
                 */
                if ((pwndT1 = ValidateHwnd((HWND)wParam)) == NULL) {
                    ThreadUnlock(&tlpwndTop);
                    return TRUE;
                }
            } else {
                if (pwndT2 == NULL) {

                    /*
                     * Set focus to the first tab item.
                     */
                    pwndT1 = _GetNextDlgTabItem(pwndTop, NULL, FALSE);
                    pwndT2 = pwndTop;
                } else {

                    /*
                     * If window with focus not a dlg ctl, ignore message.
                     */
                    if (!_IsChild(pwndTop, pwndT2)) {
                        ThreadUnlock(&tlpwndTop);
                        return TRUE;
                    }
                    /*
                     * wParam = TRUE for previous, FALSE for next
                     */
                    pwndT1 = _GetNextDlgTabItem(pwndTop, pwndT2, wParam);

                    /*
                     * If there is no next item, ignore the message.
                     */
                    if (pwndT1 == NULL) {
                        ThreadUnlock(&tlpwndTop);
                        return TRUE;
                    }
                }
            }

            ThreadLock(pwndT1, &tlpwndT1);
            ThreadLock(pwndT2, &tlpwndT2);

            DlgSetFocus(HW(pwndT1));
            xxxCheckDefPushButton(pwndTop, HW(pwndT2), HW(pwndT1));

            ThreadUnlock(&tlpwndT2);
            ThreadUnlock(&tlpwndT1);
            ThreadUnlock(&tlpwndTop);

            return TRUE;

        case WM_ENTERMENULOOP:

            /*
             * We need to pop up the combo box window if the user brings
             * down a menu.
             *
             * ...  FALL THROUGH...
             */

        case WM_LBUTTONDOWN:
        case WM_NCLBUTTONDOWN:
            hwndT1 = GetFocus();
            if (hwndT1 != NULL) {
                pwndT1 = ValidateHwndNoRip(hwndT1);

                if (GETFNID(pwndT1) == FNID_COMBOBOX) {

                    /*
                     * If user clicks anywhere in dialog box and a combo box (or
                     * the editcontrol of a combo box) has the focus, then hide
                     * it's listbox.
                     */
                    ThreadLockAlways(pwndT1, &tlpwndT1);
                    SendMessage(HWq(pwndT1), CB_SHOWDROPDOWN, FALSE, 0);
                    ThreadUnlock(&tlpwndT1);

                } else {
                    PWND pwndParent;

                    /*
                     * It's a subclassed combo box.  See if the listbox and edit
                     * boxes exist (this is a very cheezy evaluation - what if
                     * these controls are subclassed too? NOTE: Not checking
                     * for EditWndProc: it's a client proc address.
                     */
                    pwndParent = REBASEPWND(pwndT1, spwndParent);
                    if (GETFNID(pwndParent) == FNID_COMBOBOX) {
                        pwndT1 = pwndParent;
                        ThreadLock(pwndT1, &tlpwndT1);
                        SendMessage(HWq(pwndT1), CB_SHOWDROPDOWN, FALSE, 0);
                        ThreadUnlock(&tlpwndT1);
                    }
                }
            }

            /*
             * Always send the message off to DefWndProc
             */
            goto CallDWP;

        case WM_GETFONT:
            return (LRESULT)PDLG(pwnd)->hUserFont;

        case WM_VKEYTOITEM:
        case WM_COMPAREITEM:
        case WM_CHARTOITEM:
        case WM_INITDIALOG:

            /*
             * We need to return the 0 the app may have returned for these
             * items instead of calling defwindow proc.
             */
            return result;

			// FA
			//
		case WM_NCHITTEST:
				if( PDLG(pwnd)->pDlgResize != NULL )
				{
					LRESULT lres=ResizeDlgMessage( PDLG(pwnd)->pDlgResize, hwnd, message, wParam, lParam );
					if( lres ) // && (((PDIALOG)pwnd)->resultWP != 0) )
						return ((PDIALOG)pwnd)->resultWP;
					goto CallDWP;
				}
				
		case WM_SIZE:
		case WM_WINDOWPOSCHANGING:
			if( PDLG(pwnd)->pDlgResize != NULL )
				return ResizeDlgMessage( PDLG(pwnd)->pDlgResize, hwnd, message, wParam, lParam );
			goto CallDWP;
		break;
		//
		// FA

        case WM_NOTIFYFORMAT:
            if (lParam == NF_QUERY)
                return((PDLG(pwnd)->flags & DLGF_ANSI ) ? NFR_ANSI : NFR_UNICODE);
            return result;

        default:
CallDWP:
            return DefWindowProcWorker(pwnd, message, wParam, lParam, fAnsi);
        }
    } else if ((message == WM_SHOWWINDOW) && result) {

        /*
         * For a visible-case we want to snap the cursor regardless of
         * what was returned from the dialog-handler on the client.  If
         * we're going visible, snap the cursor to the dialog-button.
         */
        if (GetParentDialog(pwnd) == pwnd) {

            if (wParam && ((LOWORD(lParam) == 0) || !PDLG(pwnd)->fEnd)) {

                /*
                 * Snap the cursor to the center of the default button.
                 * Only do this if the current thread is in the foreground.
                 * The _ShowCursor() code is added to work around a
                 * problem with hardware cursors.  If change is done
                 * in the same refresh cycle, the display of the cursor
                 * would not reflect the new position.
                 */
                if (TEST_PUSIF(PUSIF_SNAPTO) &&
                        IsInForegroundQueue(hwnd)) {
                    hwndT1 = GetDlgItem(hwnd, PDLG(pwnd)->result);
                    if (hwndT1) {
                        RECT rc;

                        NtUserShowCursor(FALSE);

                        GetWindowRect(hwndT1, &rc);
                        NtUserSetCursorPos(rc.left + ((rc.right - rc.left)/2),
                                     rc.top + ((rc.bottom - rc.top)/2));

                        NtUserShowCursor(TRUE);
                    }
                }
            }
        }
    }


    /*
     * If this is still marked as a dialog window then return the real
     * result. Otherwise, we've already processed the WM_NCDESTROY message
     * and freed our private memory so return the stored value.
     */
    if (TestWF(pwnd, WFDIALOGWINDOW))
        return ((PDIALOG)pwnd)->resultWP;
    else
        return result;
}


/***************************************************************************\
* DefDlgProc
*
* Translates the message, calls DefDlgProc on server side.  DefDlgProc
* is the default WindowProc for dialogs (NOT the dialog's dialog proc)
*
* 04-11-91 ScottLu Created.
\***************************************************************************/

LRESULT WINAPI DefDlgProcW(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    PWND pwnd;

    if ((pwnd = ValidateHwnd(hwnd)) == NULL) {
        return (0L);
    }

    return DefDlgProcWorker(pwnd, message, wParam, lParam, FALSE);
}

LRESULT WINAPI DefDlgProcA(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    PWND pwnd;

    if ((pwnd = ValidateHwnd(hwnd)) == NULL) {
        return (0L);
    }

    return DefDlgProcWorker(pwnd, message, wParam, lParam, TRUE);
}


/***************************************************************************\
* DialogBox2
*
* History:
\***************************************************************************/

int PASCAL DialogBox2(
    HWND hwnd,
    HWND hwndOwner,
    BOOL fDisabled,
    BOOL fOwnerIsActiveWindow)
{
    MSG msg;
    int result;
    BOOL fShown;
    BOOL fWantIdleMsgs;
    BOOL fSentIdleMessage = FALSE;
    HWND hwndCapture;
    PWND pwnd;

    if (hwnd) {
        pwnd = ValidateHwnd(hwnd);
    } else {
        pwnd = NULL;
    }

    CheckLock(pwnd);

    if (pwnd == NULL) {
        if ((hwndOwner != NULL) && !fDisabled && IsWindow(hwndOwner)) {
            NtUserEnableWindow(hwndOwner, TRUE);
            if (fOwnerIsActiveWindow) {

                /*
                 * The dialog box failed but we disabled the owner in
                 * xxxDialogBoxIndirectParam and if it had the focus, the
                 * focus was set to NULL.  Now, when we enable the window, it
                 * doesn't get the focus back if it had it previously so we
                 * need to correct this.
                 */
                NtUserSetFocus(hwndOwner);
            }
        }
        return -1;
    }

    hwndCapture = GetCapture();
    if (hwndCapture != NULL) {
        SendMessage(hwndCapture, WM_CANCELMODE, 0, 0);
    }

    /*
     * Set the 'parent disabled' flag for EndDialog().
     * convert BOOL to definite bit 0 or 1
     */
    PDLG(pwnd)->fDisabled = !!fDisabled;

    fShown = TestWF(pwnd, WFVISIBLE);

    /*
     * Should the WM_ENTERIDLE messages be sent?
     */
    fWantIdleMsgs = !(pwnd->style & DS_NOIDLEMSG);

    if ((SYSMET(SLOWMACHINE) & 1) && !fShown && !PDLG(pwnd)->fEnd)
        goto ShowIt;

    while (PDLG(pwnd) && (!PDLG(pwnd)->fEnd)) {
        if (!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
ShowIt:
            if (!fShown) {
                fShown = TRUE;

#ifdef SYSMODALWINDOWS
                if (pwnd == gspwndSysModal) {
                    /*
                     * Make this a topmost window
                     */
                    NtUserSetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                               SWP_NOSIZE | SWP_NOMOVE |
                               SWP_NOREDRAW | SWP_NOACTIVATE);
                }
#endif

                NtUserShowWindow(hwnd, SHOW_OPENWINDOW);
                UpdateWindow(hwnd);

                if (FWINABLE()) {
                    NotifyWinEvent(EVENT_SYSTEM_DIALOGSTART, hwnd, OBJID_WINDOW, INDEXID_CONTAINER);
                }
            } else {
                /*
                 * Make sure window still exists
                 */
                if (hwndOwner && !IsWindow(hwndOwner))
                    hwndOwner = NULL;

                if (hwndOwner && fWantIdleMsgs && !fSentIdleMessage) {
                    fSentIdleMessage = TRUE;

                    SendMessage(hwndOwner, WM_ENTERIDLE, MSGF_DIALOGBOX, (LPARAM)hwnd);
                } else {
                    if ((RevalidateHwnd(hwnd)==NULL) || (pwnd->fnid & FNID_STATUS_BITS))
                        break;

                    NtUserWaitMessage();
                }
            }

        } else {
            /*
             * We got a real message.  Reset fSentIdleMessage so that we send
             * one next time things are calm.
             */
            fSentIdleMessage = FALSE;

            if (msg.message == WM_QUIT) {
                PostQuitMessage(msg.wParam);
                break;
            }

            /*
             * If pwnd is a message box, allow Ctrl-C and Ctrl-Ins
             * to copy its content to the clipboard.
             * Fall through in case hooking apps look for these keys.
             */
            if (TestWF(pwnd, WFMSGBOX)) {
                if ( (msg.message == WM_CHAR && LOBYTE(msg.wParam) == 3) ||
                     (msg.message == WM_KEYDOWN && LOBYTE(msg.wParam) == VK_INSERT && GetKeyState(VK_CONTROL) < 0)) {
                        /*
                         * Send the WM_COPY message and let the original message fall through
                         * as some apps might want it
                         */
                        SendMessage(hwnd, WM_COPY, 0, 0);
                }
            }

            /*
             * Moved the msg filter hook call to IsDialogMessage to allow
             * messages to be hooked for both modal and modeless dialog
             * boxes.
             */
            if (!IsDialogMessage(hwnd, &msg)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            /*
             * If we get a timer message, go ahead and show the window.
             * We may continuously get timer msgs if there are zillions of
             * apps running.
             *
             * If we get a syskeydown message, show the dialog box because
             * the user may be bringing down a menu and we want the dialog
             * box to become visible.
             */
            if (!fShown && (msg.message == WM_TIMER ||
                    msg.message == WM_SYSTIMER || msg.message == WM_SYSKEYDOWN))
                goto ShowIt;
        }

#if DBG
        if (!RevalidateHwnd(hwnd)) {
            /*
             * Bogus case - we've already been destroyed somehow (by app,
             * GP, etc.)
             */
            RIPMSG0(RIP_WARNING,
               "Dialog should be dismissed with EndDialog, not DestroyWindow");
        }
#endif
    }

    if (FWINABLE()) {
        NotifyWinEvent(EVENT_SYSTEM_DIALOGEND, hwnd, OBJID_WINDOW, INDEXID_CONTAINER);
    }

    /*
     * Make sure the window still exists
     */
    if (!RevalidateHwnd(hwnd)) {
        return 0;
    }

    if (PDLG(pwnd))
        result = PDLG(pwnd)->result;
    else
        result = 0;

    NtUserDestroyWindow(hwnd);

    /*
     * If the owner window belongs to another thread, the reactivation
     * of the owner may have failed within DestroyWindow().  Therefore,
     * if the current thread is in the foreground and the owner is not
     * in the foreground we can safely set the foreground back
     * to the owner.
     */
    if (hwndOwner != NULL) {
        if (IsCurrentThreadForeground() &&
            !IsInForegroundQueue(hwndOwner)) {
            NtUserSetForegroundWindow(hwndOwner);
        }
    }

    return result;
}


/***************************************************************************\
* InternalDialogBox
*
* Server portion of DialogBoxIndirectParam.
*
* 04-05-91 ScottLu      Created.
\***************************************************************************/

extern HCURSOR hCurCursor;

int InternalDialogBox(
    HANDLE hModule,
    LPDLGTEMPLATE lpdt,
    HWND hwndOwner,
    DLGPROC pfnDialog,
    LPARAM lParam,
    UINT fSCDLGFlags)
{
    int i;
    BOOL fDisabled;
    HWND hwnd;
    PWND pwndOwner;
    BOOL fOwnerIsActiveWindow = FALSE;
    TL tlpwndOwner;
    BOOL fUnlockOwner;

    UserAssert(!(fSCDLGFlags & ~(SCDLG_CLIENT|SCDLG_ANSI|SCDLG_16BIT)));    // These are the only valid flags

    /*
     * If hwndOwner == HWNDESKTOP, change it to NULL.  This way the desktop
     * (and all its children) won't be disabled if the dialog is modal.
     */
    if (hwndOwner && SAMEWOWHANDLE(hwndOwner, GetDesktopWindow()))
        hwndOwner = NULL;

    /*
     * We return 0 if the ValidateHwnd fails in order to match Win 3.1
     * validation layer which always returns 0 for invalid hwnds even
     * if the function is spec'ed to return -1.  Autocad setup bug #3615
     */
    if (hwndOwner) {
        if ((pwndOwner = ValidateHwnd(hwndOwner)) == NULL) {
            return (0L);
        }
    } else {
        pwndOwner = NULL;
    }

    CheckLock(pwndOwner);

    fUnlockOwner = FALSE;
    if (pwndOwner != NULL) {

        /* The following fixes an AV in Corel Photo-Paint 6.0.  It passes a
         * 16-bit HWND in, and croaks at some point when it gets 16-bit hwnds
         * back in send messages. FritzS -- fixing bug 12531
         */
        hwndOwner = PtoHq(pwndOwner);

        /*
         * Make sure the owner is a top level window.
         */
        if (TestwndChild(pwndOwner)) {
            pwndOwner = GetTopLevelWindow(pwndOwner);
            hwndOwner = HWq(pwndOwner);
            ThreadLock(pwndOwner, &tlpwndOwner);
            fUnlockOwner = TRUE;
        }

        /*
         * Remember if window was originally disabled (so we can set
         * the correct state when the dialog goes away.
         */
        fDisabled = TestWF(pwndOwner, WFDISABLED);
        fOwnerIsActiveWindow = (SAMEWOWHANDLE(hwndOwner, GetActiveWindow()));

        /*
         * Disable the window.
         */
        NtUserEnableWindow(hwndOwner, FALSE);
    }

    /*
     * Don't show cursors on a mouseless system. Put up an hour glass while
     * the dialog comes up.
     */
    if (SYSMET(MOUSEPRESENT)) {
        NtUserSetCursor(LoadCursor(NULL, IDC_WAIT));
    }

    /*
     * Creates the dialog.  Frees the menu if this routine fails.
     */
    hwnd = InternalCreateDialog(hModule, lpdt, 0, hwndOwner,
            pfnDialog, lParam, fSCDLGFlags);

    if (hwnd == NULL) {

        /*
         * The dialog creation failed.  Re-enable the window, destroy the
         * menu, ie., fail gracefully.
         */
        if (!fDisabled && hwndOwner != NULL)
            NtUserEnableWindow(hwndOwner, TRUE);

        if (fUnlockOwner)
            ThreadUnlock(&tlpwndOwner);
        return -1;
    }

    i = DialogBox2(hwnd, hwndOwner, fDisabled, fOwnerIsActiveWindow);

    if (fUnlockOwner)
        ThreadUnlock(&tlpwndOwner);
    return i;
}

/***************************************************************************\
**
**  RepositionRect()
**
**  Used to ensure that toplevel dialogs are still visible within the
**  desktop area after they've resized.
**
\***************************************************************************/

void
RepositionRect(
        PMONITOR    pMonitor,
        LPRECT      lprc,
        DWORD       dwStyle,
        DWORD       dwExStyle)
{
    LPRECT      lprcClip;
    int         y;

    UserAssert(lprc);
    UserAssert(pMonitor);

    if (dwStyle & WS_CHILD) {
        if (dwExStyle & WS_EX_CONTROLPARENT)
            return;

        /*
         * Old style 3.1 child dialogs--do this nonsense anyway.  Keeps
         * FedEx happy.
         */
        pMonitor = GetPrimaryMonitor();
        lprcClip = &pMonitor->rcMonitor;
    } else if (dwExStyle & WS_EX_TOOLWINDOW) {
        lprcClip = &pMonitor->rcMonitor;
    } else {
        lprcClip = &pMonitor->rcWork;
    }

    UserAssert(lprc);

    y = lprcClip->bottom - (SYSMET(CYEDGE) * 2 + SYSMET(CYKANJIWINDOW));

    if (lprc->bottom > y) {
        OffsetRect(lprc, 0, y - lprc->bottom);
    }

    if (lprc->top < lprcClip->top) {
        OffsetRect(lprc, 0, lprcClip->top - lprc->top);
    }

    if (lprc->right > lprcClip->right) {
        OffsetRect(lprc, lprcClip->right - lprc->right, 0);
    }

    if (lprc->left < lprcClip->left) {
        OffsetRect(lprc, lprcClip->left - lprc->left, 0);
    }
}

/***************************************************************************\
* MapDialogRect
*
* History:
\***************************************************************************/

BOOL MapDialogRect(
    HWND hwnd,
    LPRECT lprc)
{
    PWND pwnd;

    if ((pwnd = ValidateHwnd(hwnd)) == NULL) {
        return FALSE;
    }

    /*
     * Must do special validation here to make sure pwnd is a dialog window.
     */
    if (!ValidateDialogPwnd(pwnd))
        return FALSE;

    lprc->left = XPixFromXDU(lprc->left, PDLG(pwnd)->cxChar);
    lprc->right = XPixFromXDU(lprc->right, PDLG(pwnd)->cxChar);
    lprc->top = YPixFromYDU(lprc->top, PDLG(pwnd)->cyChar);
    lprc->bottom = YPixFromYDU(lprc->bottom, PDLG(pwnd)->cyChar);

    return TRUE;
}
