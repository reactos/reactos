/****************************** Module Header ******************************\
* Module Name: getset.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains window manager information routines
*
* History:
* 22-Oct-1990 MikeHar   Ported functions from Win 3.0 sources.
* 13-Feb-1991 MikeKe    Added Revalidation code (None)
* 08-Feb-1991 IanJa     Unicode/ANSI aware and neutral
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/****************************************************************************\
* DefSetText
*
* Processes WM_SETTEXT messages by text-alloc'ing a string in the alternate
* ds and setting 'hwnd->hName' to it's handle.
*
* History:
* 23-Oct-1990 MikeHar   Ported from Windows.
* 09-Nov-1990 DarrinM   Cleanup.
\****************************************************************************/

BOOL DefSetText(
    PWND          pwnd,
    PLARGE_STRING cczpstr)
{
    /*
     * Note -- string buffer may be on client side.
     */
    PDESKTOP pdesk;
    DWORD cbString;
    BOOL  fTranslateOk;

    if (pwnd->head.rpdesk == NULL || cczpstr == NULL || cczpstr->Buffer == NULL) {
        pwnd->strName.Length = 0;
        return TRUE;
    }

    /*
     * Capture the new window name
     */
    if (cczpstr->bAnsi)
        cbString = (cczpstr->Length + 1) * sizeof(WCHAR);
    else
        cbString = cczpstr->Length + sizeof(WCHAR);

    /*
     * If the current buffer is not large enough,
     * reallocate it.
     */
    pdesk = pwnd->head.rpdesk;
    if (pwnd->strName.MaximumLength < cbString) {
        if (pwnd->strName.Buffer != NULL)
            DesktopFree(pdesk, pwnd->strName.Buffer);
        pwnd->strName.Buffer = (LPWSTR)DesktopAlloc(pdesk, cbString, DTAG_TEXT);
        pwnd->strName.Length = 0;
        if (pwnd->strName.Buffer == NULL) {
            pwnd->strName.MaximumLength = 0;
            return FALSE;
        }
        pwnd->strName.MaximumLength = cbString;
    }

    fTranslateOk = TRUE;
    if (cczpstr->Length != 0) {
        try {
            if (!cczpstr->bAnsi) {
                RtlCopyMemory(pwnd->strName.Buffer, cczpstr->Buffer, cbString);
            } else {
                LPCSTR ccxpszAnsi = (LPCSTR)cczpstr->Buffer;

                fTranslateOk = NT_SUCCESS(RtlMultiByteToUnicodeN(pwnd->strName.Buffer,
                        cbString, &cbString,
                        (LPSTR)ccxpszAnsi, cbString / sizeof(WCHAR)));
            }
        } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
            pwnd->strName.Length = 0;
            return FALSE;
        }
    }

    if (fTranslateOk) {
        pwnd->strName.Length = cbString - sizeof(WCHAR);
        return TRUE;
    } else {
        pwnd->strName.Length = 0;
        return FALSE;
    }
}

/***************************************************************************\
* FCallerOk
*
* Ensures that no client stomps on server windows.
*
* 04-Feb-1992 ScottLu   Created.
\***************************************************************************/

BOOL FCallerOk(
    PWND pwnd)
{
    PTHREADINFO pti = PtiCurrent();

    if ((GETPTI(pwnd)->TIF_flags & (TIF_SYSTEMTHREAD | TIF_CSRSSTHREAD)) &&
            !(pti->TIF_flags & (TIF_SYSTEMTHREAD | TIF_CSRSSTHREAD))) {
        return FALSE;
    }

    if (GETPTI(pwnd)->pEThread->Cid.UniqueProcess == gpidLogon &&
            pti->pEThread->Cid.UniqueProcess != gpidLogon) {
        return FALSE;
    }

    return TRUE;
}

/***************************************************************************\
* _SetWindowWord (supports SetWindowWordA/W API)
*
* Set a window word.  Positive index values set application window words
* while negative index values set system window words.  The negative
* indices are published in WINDOWS.H.
*
* History:
* 26-Nov-1990 DarrinM   Wrote.
\***************************************************************************/

WORD _SetWindowWord(
    PWND pwnd,
    int  index,
    WORD value)
{
    WORD wOld;

    /*
     * Don't allow setting of words belonging to a system thread if the caller
     * is not a system thread. Same goes for winlogon.
     */
    if (!FCallerOk(pwnd)) {
        RIPERR0(ERROR_INVALID_INDEX, RIP_VERBOSE, "");
        return 0;
    }

    /*
     * Applications can not set a WORD into a dialog Proc or any of the
     * non-public reserved bytes in DLGWINDOWEXTRA (usersrv stores pointers
     * theres)
     */
    if (TestWF(pwnd, WFDIALOGWINDOW)) {
        if  (((index >= DWLP_DLGPROC) && (index < DWLP_MSGRESULT)) ||
                ((index > DWLP_USER+sizeof(LONG_PTR)-sizeof(WORD)) && (index < DLGWINDOWEXTRA))) {
            RIPERR3(ERROR_INVALID_INDEX, RIP_WARNING,
                  "SetWindowWord: Trying to set WORD of a windowproc pwnd=(%#p) index=(%ld) fnid (%lX)",
                pwnd, index, (DWORD)pwnd->fnid);
            return 0;
        } else {

            /*
             * If this is really a dialog and not some other server class
             * where usersrv has stored some data (Windows Compuserve -
             * wincim - does this) then store the data now that we have
             * verified the index limits.
             */
            if (GETFNID(pwnd) == FNID_DIALOG)
                goto DoSetWord;
        }
    }

    if (index == GWLP_USERDATA) {
        wOld = (WORD)pwnd->dwUserData;
        pwnd->dwUserData = MAKELONG(value, HIWORD(pwnd->dwUserData));
        return wOld;
    }

    // fix for RedShift, they call SetWindowWord
    // tn play with the low word of the style dword
    if (index == GWL_STYLE) {
        wOld = (WORD)pwnd->style;
        pwnd->style = MAKELONG(value, HIWORD(pwnd->style));
        return wOld;
    }

    if (GETFNID(pwnd) != 0) {
        if (index >= 0 &&
                (index < (int)(CBFNID(pwnd->fnid)-sizeof(WND)))) {
            switch (GETFNID(pwnd)) {
            case FNID_MDICLIENT:
                if (index == 0)
                    break;
                goto DoDefault;

            case FNID_BUTTON:
                /*
                 * CorelDraw, Direct Access 1.0 and WordPerfect 6.0 do a
                 * get/set on the first button window word.  Allow this
                 * for compatibility.
                 */
                if (index == 0) {
                    /*
                     *  Since we now use a lookaside buffer for the control's
                     *  private data, we need to indirect into this structure.
                     */
                    PBUTN pbutn = ((PBUTNWND)pwnd)->pbutn;
                    if (!pbutn || (LONG_PTR)pbutn == (LONG_PTR)-1) {
                        return 0;
                    } else {
                        try {
                            wOld = (WORD)ProbeAndReadUlong(&pbutn->buttonState);
                            pbutn->buttonState = value;
                        } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
                            wOld = 0;
                        }
                        return wOld;
                    }
                }
                goto DoDefault;

            default:
DoDefault:
                RIPERR3(ERROR_INVALID_INDEX,
                        RIP_WARNING,
                        "SetWindowWord: Trying to set private server data pwnd=(%#p) index=(%ld) fnid (%lX)",
                        pwnd, index, (DWORD)pwnd->fnid);
                return 0;
                break;
            }
        }
    }

DoSetWord:
    if ((index < 0) || ((UINT)index + sizeof(WORD) > (UINT)pwnd->cbwndExtra)) {
        RIPERR0(ERROR_INVALID_INDEX, RIP_WARNING,"SetWindowWord Fails because of invalid index");
        return 0;
    } else {
        WORD UNALIGNED *pw;

        pw = (WORD UNALIGNED *)((BYTE *)(pwnd + 1) + index);
        wOld = *pw;
        *pw = value;
        return (WORD)wOld;
    }
}

/***************************************************************************\
* xxxSetWindowLong (API)
*
* Set a window long.  Positive index values set application window longs
* while negative index values set system window longs.  The negative
* indices are published in WINDOWS.H.
*
* History:
* 26-Nov-1990 DarrinM   Wrote.
\***************************************************************************/

ULONG_PTR xxxSetWindowLongPtr(
    PWND  pwnd,
    int   index,
    ULONG_PTR dwData,
    BOOL  bAnsi)
{
    ULONG_PTR dwOld;
    DWORD dwCPDType = 0;

    /*
     * Hide the window proc from other processes
     */
#if DBG
    if (PpiCurrent() != GETPTI(pwnd)->ppi) {
        RIPMSG0(RIP_WARNING, "Setting cross process windowlong; win95 would fail");
    }
#endif

    /*
     * CheckLock(pwnd);  The only case that leaves the critical section is
     * where xxxSetWindowData is called, which does this.  Saves us some locks.
     *
     *
     * Don't allow setting of words belonging to a system thread if the caller
     * is not a system thread. Same goes for winlogon.
     */
    if (!FCallerOk(pwnd)) {
        RIPERR0(ERROR_INVALID_INDEX, RIP_VERBOSE, "");
        return 0;
    }

    /*
     * If it's a dialog window, only a few indices are permitted.
     */
    if (GETFNID(pwnd) != 0) {
        if (TestWF(pwnd, WFDIALOGWINDOW)) {
            switch (index) {
            case DWLP_MSGRESULT:
                 dwOld = (ULONG_PTR)((PDIALOG)(pwnd))->resultWP;
                 ((PDIALOG)(pwnd))->resultWP = (LONG_PTR)dwData;
                 return dwOld;

            case DWLP_USER:
                 dwOld = (ULONG_PTR)((PDIALOG)(pwnd))->unused;
                 ((PDIALOG)(pwnd))->unused = (LONG_PTR)dwData;
                 return dwOld;

            default:
                if (index >= 0 && index < DLGWINDOWEXTRA) {
                    RIPERR0(ERROR_PRIVATE_DIALOG_INDEX, RIP_VERBOSE, "");
                    return 0;
                }
            }
        } else {
            if (index >= 0 &&
                    (index < (int)(CBFNID(pwnd->fnid)-sizeof(WND)))) {
                switch (GETFNID(pwnd)) {
                case FNID_BUTTON:
                case FNID_COMBOBOX:
                case FNID_COMBOLISTBOX:
                case FNID_DIALOG:
                case FNID_LISTBOX:
                case FNID_STATIC:
                case FNID_EDIT:
#ifdef FE_IME
                case FNID_IME:
#endif
                    /*
                     * Allow the 0 index for controls to be set if it's
                     * still NULL or the window is being destroyed. This
                     * is where controls store their private data.
                     */
                    if (index == 0) {
                        dwOld = *((PULONG_PTR)(pwnd + 1));
                        if (dwOld == 0 || TestWF(pwnd, WFDESTROYED))
                            goto SetData;
                    }
                    break;

                case FNID_MDICLIENT:
                    /*
                     * Allow the 0 index (which is reserved) to be set/get.
                     * Quattro Pro 1.0 uses this index!
                     *
                     * Allow the 4 index to be set if it's still NULL or
                     * the window is being destroyed. This is where we
                     * store our private data.
                     */
#ifndef _WIN64
                    if (index == 0) {
                        goto SetData;
                    }
#endif
                    if (index == GWLP_MDIDATA) {
                        dwOld = *((PULONG_PTR)(pwnd + 1));
                        if (dwOld == 0 || TestWF(pwnd, WFDESTROYED))
                            goto SetData;
                    }
                    break;
                }

                RIPERR3(ERROR_INVALID_INDEX,
                        RIP_WARNING,
                        "SetWindowLongPtr: Trying to set private server data pwnd=(%#p) index=(%ld) FNID=(%lX)",
                        pwnd, index, (DWORD)pwnd->fnid);
                return 0;
            }
        }
    }

    if (index < 0) {
        return xxxSetWindowData(pwnd, index, dwData, bAnsi);
    } else {
        if ((UINT)index + sizeof(ULONG_PTR) > (UINT)pwnd->cbwndExtra) {
            RIPERR3(ERROR_INVALID_INDEX,
                    RIP_WARNING,
                    "SetWindowLongPtr: Index %d too big for cbWndExtra %d on pwnd %#p",
                    index, pwnd->cbwndExtra, pwnd);
            return 0;
        } else {
            ULONG_PTR UNALIGNED *pudw;

SetData:
            pudw = (ULONG_PTR UNALIGNED *)((BYTE *)(pwnd + 1) + index);
            dwOld = *pudw;
            *pudw = dwData;
            return dwOld;
        }
    }
}

#ifdef _WIN64
DWORD xxxSetWindowLong(
    PWND  pwnd,
    int   index,
    DWORD dwData,
    BOOL  bAnsi)
{
    DWORD dwOld;

    /*
     * Hide the window proc from other processes
     */
#if DBG
    if (PpiCurrent() != GETPTI(pwnd)->ppi) {
        RIPMSG0(RIP_WARNING, "Setting cross process windowlong; win95 would fail");
    }
#endif

    /*
     * CheckLock(pwnd);  The only case that leaves the critical section is
     * where xxxSetWindowData is called, which does this.  Saves us some locks.
     *
     *
     * Don't allow setting of words belonging to a system thread if the caller
     * is not a system thread. Same goes for winlogon.
     */
    if (!FCallerOk(pwnd)) {
        RIPERR0(ERROR_INVALID_INDEX, RIP_VERBOSE, "");
        return 0;
    }

    /*
     * If it's a dialog window, only a few indices are permitted.
     */
    if (GETFNID(pwnd) != 0) {
        if (TestWF(pwnd, WFDIALOGWINDOW)) {
            switch (index) {
            case DWLP_MSGRESULT:
                 dwOld = (DWORD)((PDIALOG)(pwnd))->resultWP;
                 ((PDIALOG)(pwnd))->resultWP = (long)dwData;
                 return dwOld;

            case DWLP_USER:
                 dwOld = (DWORD)((PDIALOG)(pwnd))->unused;
                 ((PDIALOG)(pwnd))->unused = (long)dwData;
                 return dwOld;

            default:
                if (index >= 0 && index < DLGWINDOWEXTRA) {
                    RIPERR0(ERROR_PRIVATE_DIALOG_INDEX, RIP_VERBOSE, "");
                    return 0;
                }
            }
        } else {
            if (index >= 0 &&
                    (index < (int)(CBFNID(pwnd->fnid)-sizeof(WND)))) {
                switch (GETFNID(pwnd)) {
                case FNID_MDICLIENT:
                    /*
                     * Allow the 0 index (which is reserved) to be set/get.
                     * Quattro Pro 1.0 uses this index!
                     */
                    if (index == 0) {
                        goto SetData;
                    }
                    break;
                }

                RIPERR3(ERROR_INVALID_INDEX,
                        RIP_WARNING,
                        "SetWindowLong: Trying to set private server data pwnd=(%#p) index=(%ld) FNID=(%lX)",
                        pwnd, index, (DWORD)pwnd->fnid);
                return 0;
            }
        }
    }

    if (index < 0) {
        if ((index != GWL_STYLE) && (index != GWL_EXSTYLE) && (index != GWL_ID) && (index != GWLP_USERDATA)) {
            RIPERR1(ERROR_INVALID_INDEX, RIP_WARNING, "SetWindowLong: invalid index %d", index);
            return 0;
        }
        return (DWORD)xxxSetWindowData(pwnd, index, dwData, bAnsi);
    } else {
        if ((UINT)index + sizeof(DWORD) > (UINT)pwnd->cbwndExtra) {
            RIPERR3(ERROR_INVALID_INDEX,
                    RIP_WARNING,
                    "SetWindowLong: Index %d too big for cbWndExtra %d on pwnd %#p",
                    index, pwnd->cbwndExtra, pwnd);
            return 0;
        } else {
            DWORD UNALIGNED *pudw;

SetData:
            pudw = (DWORD UNALIGNED *)((BYTE *)(pwnd + 1) + index);
            dwOld = *pudw;
            *pudw = dwData;
            return dwOld;
        }
    }
}
#endif

/***************************************************************************\
* xxxHandleOwnerSwitch
*
\***************************************************************************/

void xxxHandleOwnerSwitch(PWND pwnd, PWND pwndNewParent, PWND pwndOldParent)
{
    CheckLock(pwnd);
    CheckLock(pwndNewParent);
    CheckLock(pwndOldParent);

    if ((pwndOldParent != NULL) &&
            (GETPTI(pwndOldParent) != GETPTI(pwnd))) {

        /*
         * See if it needs to be unattached.
         */
        if ((pwndNewParent == NULL) ||
            (GETPTI(pwndNewParent) == GETPTI(pwnd)) ||
            (GETPTI(pwndNewParent) != GETPTI(pwndOldParent))) {

            zzzAttachThreadInput(GETPTI(pwnd), GETPTI(pwndOldParent), FALSE);
        }
    }

    /*
     * See if it needs to be attached.
     */
    if ((pwndNewParent != NULL) &&
            (GETPTI(pwndNewParent) != GETPTI(pwnd)) &&
            ((pwndOldParent == NULL) ||
                (GETPTI(pwndNewParent) != GETPTI(pwndOldParent)))) {

        zzzAttachThreadInput(GETPTI(pwnd), GETPTI(pwndNewParent), TRUE);
    }

    /*
     * Post hook messages for tray-windows.
     */
    if (IsTrayWindow(pwnd)) {

        HWND hw = PtoH(pwnd);

        /*
         * If we're setting the owner and it's changing from owned
         * to unowned or vice-versa, notify the tray.
         */
        if ((pwndOldParent != NULL) && (pwndNewParent == NULL)) {
            xxxCallHook(HSHELL_WINDOWCREATED,
                        (WPARAM)hw,
                        (LONG)0,
                        WH_SHELL);
            PostShellHookMessages(HSHELL_WINDOWCREATED, (LPARAM)hw);

        } else if ((pwndOldParent == NULL) && (pwndNewParent != NULL)) {
            xxxCallHook(HSHELL_WINDOWDESTROYED,
                        (WPARAM)hw,
                        (LONG)0,
                        WH_SHELL);
            PostShellHookMessages(HSHELL_WINDOWDESTROYED, (LPARAM)hw);
        }
    }
}

/***************************************************************************\
* xxxSetWindowData
*
* SetWindowWord and ServerSetWindowLong are now identical routines because they
* both can return DWORDs.  This single routine performs the work for them both.
*
* History:
* 26-Nov-1990 DarrinM   Wrote.
\***************************************************************************/

ULONG_PTR xxxSetWindowData(
    PWND  pwnd,
    int   index,
    ULONG_PTR dwData,
    BOOL  bAnsi)
{
    ULONG_PTR dwT;
    ULONG_PTR dwOld;
    PMENU pmenu;
    PWND  *ppwnd;
    PWND  pwndNewParent;
    PWND  pwndOldParent;
    BOOL  fTopOwner;
    TL    tlpwndOld;
    TL    tlpwndNew;
    DWORD dwCPDType = 0;

    CheckLock(pwnd);
    UserAssert(IsWinEventNotifyDeferredOK());

    switch (index) {
    case GWLP_USERDATA:
        dwOld = pwnd->dwUserData;
        pwnd->dwUserData = dwData;
        break;

    case GWL_EXSTYLE:
    case GWL_STYLE:
        dwOld = xxxSetWindowStyle(pwnd, index, (DWORD)dwData);
        break;

    case GWLP_ID:
        /*
         * Win95 does a TestWF(pwnd, WFCHILD) here, but we'll do the same
         * check we do everywhere else or it'll cause us trouble.
         */
        if (TestwndChild(pwnd)) {

            /*
             * pwnd->spmenu is an id in this case.
             */
            dwOld = (ULONG_PTR)pwnd->spmenu;
            pwnd->spmenu = (struct tagMENU *)dwData;
        } else {
            dwOld = 0;
            if (pwnd->spmenu != NULL)
                dwOld = (ULONG_PTR)PtoH(pwnd->spmenu);

            if (dwData == 0) {
                UnlockWndMenu(pwnd, &pwnd->spmenu);
            } else {
                pmenu = ValidateHmenu((HANDLE)dwData);
                if (pmenu != NULL) {
                    LockWndMenu(pwnd, &pwnd->spmenu, pmenu);
                } else {

                    /*
                     * Menu is invalid, so don't set a new one!
                     */
                    dwOld = 0;
                }
            }
        }
        break;

    case GWLP_HINSTANCE:
        dwOld = (ULONG_PTR)pwnd->hModule;
        pwnd->hModule = (HANDLE)dwData;
        break;

    case GWLP_WNDPROC:  // See similar case DWLP_DLGPROC

        /*
         * Hide the window proc from other processes
         */
        if (PpiCurrent() != GETPTI(pwnd)->ppi) {
            RIPERR1(ERROR_ACCESS_DENIED, RIP_WARNING,
                "SetWindowLong: Window owned by another process %#p", pwnd);
            return 0;
        }

        /*
         * If the window has been zombized by a DestroyWindow but is still
         * around because the window was locked don't let anyone change
         * the window proc from DefWindowProc!
         *
         * !!! LATER long term move this test into the ValidateHWND; kind of
         * !!! LATER close to shipping for that
         */
        if (pwnd->fnid & FNID_DELETED_BIT) {
            UserAssert(pwnd->lpfnWndProc == xxxDefWindowProc);
            RIPERR1(ERROR_ACCESS_DENIED, RIP_WARNING,
                "SetWindowLong: Window is a zombie %#p", pwnd);
            return 0;
        }

        /*
         * If the application (client) subclasses a window that has a server -
         * side window proc we must return an address that the client can call:
         * this client-side wndproc expectes Unicode or ANSI depending on bAnsi
         */

        if (TestWF(pwnd, WFSERVERSIDEPROC)) {
            dwOld = MapServerToClientPfn((ULONG_PTR)pwnd->lpfnWndProc, bAnsi);

            /*
             * If we don't have a client side address (like for the DDEMLMon
             *  window) then blow off the subclassing.
             */
            if (dwOld == 0) {
                RIPMSG0(RIP_WARNING, "SetWindowLong: subclass server only window");
                return(0);
            }

            ClrWF(pwnd, WFSERVERSIDEPROC);
        } else {
            /*
             * Keep edit control behavior compatible with NT 3.51.
             */
            if (GETFNID(pwnd) == FNID_EDIT) {
                dwOld = (ULONG_PTR)pwnd->lpfnWndProc;
            } else {
                dwOld = MapClientNeuterToClientPfn(pwnd->pcls, (ULONG_PTR)pwnd->lpfnWndProc, bAnsi);
            }

            /*
             * If the client mapping didn't change the window proc then see if
             * we need a callproc handle.
             */
            if (dwOld == (ULONG_PTR)pwnd->lpfnWndProc) {
                /*
                 * May need to return a CallProc handle if there is an Ansi/Unicode mismatch
                 */
                if (bAnsi != (TestWF(pwnd, WFANSIPROC) ? TRUE : FALSE)) {
                    dwCPDType |= bAnsi ? CPD_ANSI_TO_UNICODE : CPD_UNICODE_TO_ANSI;
                }
            }

            UserAssert(!ISCPDTAG(dwOld));

            if (dwCPDType) {
                ULONG_PTR cpd;

                cpd = GetCPD(pwnd, dwCPDType | CPD_WND, dwOld);

                if (cpd) {
                    dwOld = cpd;
                } else {
                    RIPMSG0(RIP_WARNING, "SetWindowLong unable to alloc CPD returning handle\n");
                }
            }
        }

        /*
         * Convert a possible CallProc Handle into a real address.  They may
         * have kept the CallProc Handle from some previous mixed GetClassinfo
         * or SetWindowLong.
         *
         * WARNING bAnsi is modified here to represent real type of
         * proc rather than if SetWindowLongA or W was called
         *
         */
        if (ISCPDTAG(dwData)) {
            PCALLPROCDATA pCPD;
            if (pCPD = HMValidateHandleNoRip((HANDLE)dwData, TYPE_CALLPROC)) {
                dwData = pCPD->pfnClientPrevious;
                bAnsi = pCPD->wType & CPD_UNICODE_TO_ANSI;
            }
        }

        /*
         * If an app 'unsubclasses' a server-side window proc we need to
         * restore everything so SendMessage and friends know that it's
         * a server-side proc again.  Need to check against client side
         * stub addresses.
         */
        if ((dwT = MapClientToServerPfn(dwData)) != 0) {
            pwnd->lpfnWndProc = (WNDPROC_PWND)dwT;
            SetWF(pwnd, WFSERVERSIDEPROC);
            ClrWF(pwnd, WFANSIPROC);
        } else {
            pwnd->lpfnWndProc = (WNDPROC_PWND)MapClientNeuterToClientPfn(pwnd->pcls, dwData, bAnsi);
            if (bAnsi) {
                SetWF(pwnd, WFANSIPROC);
            } else {
                ClrWF(pwnd, WFANSIPROC);
            }

            pwnd->hMod16 = xxxClientWOWGetProcModule(pwnd->lpfnWndProc);

        }

        break;

    case GWLP_HWNDPARENT:
        /*
         * Special case for pre-1.1 versions of Windows
         * Set/GetWindowWord(GWW_HWNDPARENT) needs to be mapped
         * to the hwndOwner for top level windows.
         */
        fTopOwner = FALSE;
        if (pwnd->spwndParent == PWNDDESKTOP(pwnd)) {
            ppwnd = &pwnd->spwndOwner;
            fTopOwner = TRUE;
        } else {
            ppwnd = &pwnd->spwndParent;
        }


        /*
         * If we're a topmost, then we're only changing the owner
         * relationship.  Otherwise, we are doing a relinking of the
         * parent/child relationship.
         */
        pwndOldParent = *ppwnd;
        pwndNewParent = ValidateHwnd((HWND)dwData);

        if ((pwndNewParent == NULL) && dwData) {
            RIPERR1(ERROR_INVALID_PARAMETER, RIP_VERBOSE, "Set GWL_HWNDPARENT, invalid hwndParent %#p", dwData);
            return 0;
        }

        dwOld = (ULONG_PTR)HW(*ppwnd);

        ThreadLock(pwndNewParent, &tlpwndNew);

        if (fTopOwner) {

            ThreadLock(pwndOldParent, &tlpwndOld);

            xxxHandleOwnerSwitch(pwnd, pwndNewParent, pwndOldParent);

            if (ValidateOwnerDepth(pwnd, pwndNewParent)) {

                /*
                 * Set the owner.
                 */
                if (pwndNewParent) {
                    Lock(ppwnd, pwndNewParent);
                } else {
                    Unlock(ppwnd);
                }
            } else {

                /*
                 * Undo the switch and set last error.
                 */
                xxxHandleOwnerSwitch(pwnd, pwndOldParent, pwndNewParent);
                RIPERR0(ERROR_INVALID_PARAMETER, RIP_ERROR, "Detected loop in owner chain");
                dwOld = 0;
            }

            ThreadUnlock(&tlpwndOld);

        } else {
            if (!xxxSetParent(pwnd, pwndNewParent)) {
                dwOld = 0;
            }
        }

        ThreadUnlock(&tlpwndNew);
        break;

    default:
        RIPERR0(ERROR_INVALID_INDEX, RIP_VERBOSE, "");
        return 0;
    }

    return dwOld;
}

/***************************************************************************\
* FindPCPD
*
* Searches the list of CallProcData's associated with window to see if
* one already exists representing this transition.  CPD can be re-used
* and aren't deleted until a window or thread dies
*
*
* 04-Feb-1993 JohnC     Created.
\***************************************************************************/

PCALLPROCDATA FindPCPD(
    PCALLPROCDATA pCPD,
    ULONG_PTR      dwClientPrevious,
    WORD          wCPDType)
{
    while (pCPD) {
        if ((pCPD->pfnClientPrevious == dwClientPrevious) &&
                (pCPD->wType == wCPDType))
            return pCPD;
        pCPD = pCPD->spcpdNext;
    }

    return NULL;
}

/***************************************************************************\
* GetCPD
*
* Searches the list of CallProcData's associated with a class or window
* (if the class is not provided).  If one already exists representing this
* transition it is returned or else a new CPD is created
*
* 04-Feb-1993 JohnC     Created.
\***************************************************************************/

ULONG_PTR GetCPD(
    PVOID pWndOrCls,
    DWORD CPDOption,
    ULONG_PTR dwProc32)
{
    PCALLPROCDATA pCPD;
    PCLS          pcls;
#if DBG
    BOOL          bAnsiProc;
#endif

    PTHREADINFO ptiCurrent;

    if (CPDOption & (CPD_WND | CPD_DIALOG)) {
        UserAssert(!(CPDOption & (CPD_CLASS | CPD_WNDTOCLS)));
        pcls = ((PWND)pWndOrCls)->pcls;

#if DBG
        if (CPDOption & CPD_WND) {
            bAnsiProc = !!(TestWF(pWndOrCls, WFANSIPROC));
        } else {
            /*
             * We'll assume the client-side dialog box code knows
             * what it's doing, since we can't check it from here.
             */
            bAnsiProc = !!(CPDOption & CPD_UNICODE_TO_ANSI);
        }
#endif
    } else {
        UserAssert(CPDOption & (CPD_CLASS | CPD_WNDTOCLS));
        if (CPDOption & CPD_WNDTOCLS)
            pcls = ((PWND)pWndOrCls)->pcls;
        else
            pcls = pWndOrCls;
#if DBG
        bAnsiProc = !!(pcls->CSF_flags & CSF_ANSIPROC);
#endif
    }

#if DBG
    /*
     * We should never have a CallProc handle as the calling address
     */
    UserAssert(!ISCPDTAG(dwProc32));

    if (CPDOption & CPD_UNICODE_TO_ANSI) {
        UserAssert(bAnsiProc);
    } else if (CPDOption & CPD_ANSI_TO_UNICODE) {
        UserAssert(!bAnsiProc);
    }

#endif // DBG

    /*
     * See if we already have a CallProc Handle that represents this
     * transition
     */
    pCPD = FindPCPD(pcls->spcpdFirst, dwProc32, (WORD)CPDOption);

    if (pCPD) {
        return MAKE_CPDHANDLE(PtoH(pCPD));
    }

    CheckCritIn();

    ptiCurrent = PtiCurrent();

    pCPD = HMAllocObject(ptiCurrent,
                         ptiCurrent->rpdesk,
                         TYPE_CALLPROC,
                         sizeof(CALLPROCDATA));
    if (pCPD == NULL) {
        RIPMSG0(RIP_WARNING, "GetCPD unable to alloc CALLPROCDATA\n");
        return 0;
    }

    /*
     * Link in the new CallProcData to the class list.
     * Note -- these pointers are locked because WOWCleanup can come in
     * and delete objects, so we need to keep the pointers locked.
     */
    Lock(&pCPD->spcpdNext, pcls->spcpdFirst);
    Lock(&pcls->spcpdFirst, pCPD);

    /*
     * Initialize the CPD
     */
    pCPD->pfnClientPrevious = dwProc32;
    pCPD->wType = (WORD)CPDOption;

    return MAKE_CPDHANDLE(PtoH(pCPD));
}

/***************************************************************************\
* MapClientToServerPfn
*
* Checks to see if a dword is a client wndproc stub to a server wndproc.
* If it is, this returns the associated server side wndproc. If it isn't
* this returns 0.
*
* 13-Jan-1992 ScottLu   Created.
\***************************************************************************/

ULONG_PTR MapClientToServerPfn(
    ULONG_PTR dw)
{
    ULONG_PTR *pdw;
    int   i;

    pdw = (ULONG_PTR *)&gpsi->apfnClientW;
    for (i = FNID_WNDPROCSTART; i <= FNID_WNDPROCEND; i++, pdw++) {
        if (*pdw == dw)
            return (ULONG_PTR)STOCID(i);
    }

    pdw = (ULONG_PTR *)&gpsi->apfnClientA;
    for (i = FNID_WNDPROCSTART; i <= FNID_WNDPROCEND; i++, pdw++) {
        if (*pdw == dw)
            return (ULONG_PTR)STOCID(i);
    }

    return 0;
}

ULONG DBGGetWindowLong(PWND pwnd, int index)
{
    UserAssert(index >= 0);
    UserAssert((UINT)index + sizeof(DWORD) <= (UINT)pwnd->cbwndExtra);
    return __GetWindowLong(pwnd, index);
}

ULONG_PTR DBGGetWindowLongPtr(PWND pwnd, int index)
{
    UserAssert(index >= 0);
    UserAssert((UINT)index + sizeof(ULONG_PTR) <= (UINT)pwnd->cbwndExtra);
    return __GetWindowLongPtr(pwnd, index);
}
