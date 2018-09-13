/****************************** Module Header ******************************\
* Module Name: getsetc.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains window manager information routines
*
* History:
* 10-Mar-1993 JerrySh   Pulled functions from user\server.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* _GetWindowWord (supports the GetWindowWord API)
*
* Return a window word.  Positive index values return application window words
* while negative index values return system window words.  The negative
* indices are published in WINDOWS.H.
*
* History:
* 11-26-90 darrinm      Wrote.
\***************************************************************************/

WORD _GetWindowWord(
    PWND pwnd,
    int index)
{
    if (GETFNID(pwnd) != 0) {
        if ((index >= 0) && (index <
                (int)(CBFNID(pwnd->fnid)-sizeof(WND)))) {

            switch (GETFNID(pwnd)) {
            case FNID_MDICLIENT:
                if (index == 0)
                    break;
                goto DoDefault;

            case FNID_BUTTON:
                /*
                 * CorelDraw does a get/set on the first button window word.
                 * Allow it to.
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
                        return (WORD)(pbutn->buttonState);
                    }
                }
                goto DoDefault;

            case FNID_DIALOG:
                if (index == DWLP_USER)
                    return LOWORD(((PDIALOG)pwnd)->unused);
                if (index == DWLP_USER+2)
                    return HIWORD(((PDIALOG)pwnd)->unused);
                goto DoDefault;

            default:
DoDefault:
                RIPERR3(ERROR_INVALID_INDEX,
                        RIP_WARNING,
                        "GetWindowWord: Trying to read private server data pwnd=(%#p) index=(%ld) fnid=(%lX)",
                        pwnd, index, (DWORD)pwnd->fnid);
                return 0;
                break;
            }
        }
    }

    if (index == GWLP_USERDATA)
        return (WORD)pwnd->dwUserData;

    if ((index < 0) || ((UINT)index + sizeof(WORD) > (UINT)pwnd->cbwndExtra)) {
        RIPERR0(ERROR_INVALID_INDEX, RIP_VERBOSE, "");
        return 0;
    } else {
        return *((WORD UNALIGNED *)((BYTE *)(pwnd + 1) + index));
    }
}

ULONG_PTR GetWindowData(PWND pwnd, int index, BOOL bAnsi);

/***************************************************************************\
* _GetWindowLong (supports GetWindowLongA/W API)
*
* Return a window long.  Positive index values return application window longs
* while negative index values return system window longs.  The negative
* indices are published in WINDOWS.H.
*
* History:
* 11-26-90 darrinm      Wrote.
\***************************************************************************/

ULONG_PTR _GetWindowLongPtr(
    PWND pwnd,
    int index,
    BOOL bAnsi)
{
    ULONG_PTR           dwProc;
    DWORD              dwCPDType = 0;
    ULONG_PTR UNALIGNED *pudw;

    /*
     * If it's a dialog window, only a few indices are permitted.
     */
    if (GETFNID(pwnd) != 0) {
        if (TestWF(pwnd, WFDIALOGWINDOW)) {
            switch (index) {
            case DWLP_DLGPROC:    // See similar case GWLP_WNDGPROC

                /*
                 * Hide the window proc from other processes
                 */
                if (!TestWindowProcess(pwnd)) {
                    RIPERR1(ERROR_ACCESS_DENIED,
                            RIP_WARNING,
                            "Access denied to \"pwnd\" (%#p) in _GetWindowLong",
                            pwnd);

                    return 0;
                }

                dwProc = (ULONG_PTR)PDLG(pwnd)->lpfnDlg;

                /*
                 * If a proc exists check it to see if we need a translation
                 */
                if (dwProc) {

                    /*
                     * May need to return a CallProc handle if there is an
                     * Ansi/Unicode transition
                     */
                    if (bAnsi != ((PDLG(pwnd)->flags & DLGF_ANSI) ? TRUE : FALSE)) {
                        dwCPDType |= bAnsi ? CPD_ANSI_TO_UNICODE : CPD_UNICODE_TO_ANSI;
                    }

                    if (dwCPDType) {
                        ULONG_PTR cpd;

                        cpd = GetCPD(pwnd, dwCPDType | CPD_DIALOG, dwProc);

                        if (cpd) {
                            dwProc = cpd;
                        } else {
                            RIPMSG0(RIP_WARNING, "GetWindowLong unable to alloc CPD returning handle\n");
                        }
                    }
                }

                /*
                 * return proc (or CPD handle)
                 */
                return dwProc;

            case DWLP_MSGRESULT:
                 return (ULONG_PTR)((PDIALOG)pwnd)->resultWP;

            case DWLP_USER:
                 return (ULONG_PTR)((PDIALOG)pwnd)->unused;

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
                    if (index != 0)
                        break;

                    goto GetData;
                    break;

                case FNID_EDIT:

                    if (index != 0)
                        break;

                    /*
                     * If we get to this point we need to return the first
                     * entry in the lookaside.  This will provide backward
                     * compatibilty for 3.51 that allowed edit-controls to
                     * do this.  PeachTree is one app which required this.
                     */
                    pudw = (ULONG_PTR UNALIGNED *)((BYTE *)(pwnd + 1));

                    /*
                     * Do not dereference the pointer if we are not in
                     *  the proper address space. Apps like Spyxx like to
                     *  do this on other process' windows
                     */
                    return (TestWindowProcess(pwnd) ? *(ULONG_PTR UNALIGNED *)*pudw : (ULONG_PTR)pudw);

                }

                RIPERR3(ERROR_INVALID_INDEX,
                        RIP_WARNING,
                        "GetWindowLong: Trying to read private server data pwnd=(%#p) index=(%ld) fnid (%lX)",
                        pwnd, index, (DWORD)pwnd->fnid);
                return 0;
            }
        }
    }

    if (index < 0) {
        return GetWindowData(pwnd, index, bAnsi);
    } else {
        if ((UINT)index + sizeof(ULONG_PTR) > (UINT)pwnd->cbwndExtra) {
            RIPERR0(ERROR_INVALID_INDEX, RIP_VERBOSE, "");
            return 0;
        } else {

GetData:
            pudw = (ULONG_PTR UNALIGNED *)((BYTE *)(pwnd + 1) + index);
            return *pudw;
        }
    }
}


#ifdef _WIN64
DWORD _GetWindowLong(
    PWND pwnd,
    int index,
    BOOL bAnsi)
{
    DWORD UNALIGNED *pudw;

    /*
     * If it's a dialog window, only a few indices are permitted.
     */
    if (GETFNID(pwnd) != 0) {
        if (TestWF(pwnd, WFDIALOGWINDOW)) {
            switch (index) {
            case DWLP_DLGPROC:    // See similar case GWLP_WNDPROC
                RIPERR1(ERROR_INVALID_INDEX, RIP_WARNING, "GetWindowLong: invalid index %d", index);
                return 0;

            case DWLP_MSGRESULT:
                 return (DWORD)((PDIALOG)pwnd)->resultWP;

            case DWLP_USER:
                 return (DWORD)((PDIALOG)pwnd)->unused;

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
                    if (index != 0)
                        break;

                    goto GetData;
                    break;

                case FNID_EDIT:

                    if (index != 0)
                        break;

                    /*
                     * If we get to this point we need to return the first
                     * entry in the lookaside.  This will provide backward
                     * compatibilty for 3.51 that allowed edit-controls to
                     * do this.  PeachTree is one app which required this.
                     */
                    pudw = (DWORD UNALIGNED *)((BYTE *)(pwnd + 1));

                    /*
                     * Do not dereference the pointer if we are not in
                     *  the proper address space. Apps like Spyxx like to
                     *  do this on other process' windows
                     */
                    return (TestWindowProcess(pwnd) ? *(DWORD UNALIGNED *)*(ULONG_PTR UNALIGNED *)pudw : PtrToUlong(pudw));


                }

                RIPERR3(ERROR_INVALID_INDEX,
                        RIP_WARNING,
                        "GetWindowLong: Trying to read private server data pwnd=(%#p) index=(%ld) fnid (%lX)",
                        pwnd, index, (DWORD)pwnd->fnid);
                return 0;
            }
        }
    }

    if (index < 0) {
        if ((index != GWL_STYLE) && (index != GWL_EXSTYLE) && (index != GWL_ID) && (index != GWLP_USERDATA)) {
            RIPERR1(ERROR_INVALID_INDEX, RIP_WARNING, "GetWindowLong: invalid index %d", index);
            return 0;
        }
        return (DWORD)GetWindowData(pwnd, index, bAnsi);
    } else {
        if ((UINT)index + sizeof(DWORD) > (UINT)pwnd->cbwndExtra) {
            RIPERR0(ERROR_INVALID_INDEX, RIP_VERBOSE, "");
            return 0;
        } else {

GetData:
            pudw = (DWORD UNALIGNED *)((BYTE *)(pwnd + 1) + index);
            return *pudw;
        }
    }
}
#endif


/***************************************************************************\
* GetWindowData
*
* History:
* 11-26-90 darrinm      Wrote.
\***************************************************************************/

ULONG_PTR GetWindowData(
    PWND pwnd,
    int index,
    BOOL bAnsi)
{
    KERNEL_ULONG_PTR dwProc;
    DWORD dwCPDType = 0;
    PWND pwndParent;

    switch (index) {
    case GWLP_USERDATA:
        return KERNEL_ULONG_PTR_TO_ULONG_PTR(pwnd->dwUserData);

    case GWL_EXSTYLE:
        /*
         * Apps should not mess with unused bits.  We use them privately
         */
        return pwnd->ExStyle & WS_EX_ALLVALID;

    case GWL_STYLE:
        return pwnd->style;

    case GWLP_ID:
        if (TestwndChild(pwnd)) {
            return (ULONG_PTR)pwnd->spmenu;
        } else if (pwnd->spmenu != NULL) {
            PMENU pmenu;

            pmenu = REBASEALWAYS(pwnd, spmenu);
            return (ULONG_PTR)PtoH(pmenu);
        }
        return 0;

    case GWLP_HINSTANCE:
        return (ULONG_PTR)pwnd->hModule;

    case GWLP_WNDPROC: // See similar case DWLP_DLGPROC
        /*
         * Hide the window proc from other processes
         */
        if (!TestWindowProcess(pwnd)) {
            RIPERR1(ERROR_ACCESS_DENIED, RIP_WARNING, "Can not subclass another process's window %#p", pwnd);
            return 0;
        }

        /*
         * If the client queries a server-side winproc we return the
         * address of the client-side winproc (expecting ANSI or Unicode
         * depending on bAnsi)
         */
        if (TestWF(pwnd, WFSERVERSIDEPROC)) {
            dwProc = MapServerToClientPfn((KERNEL_ULONG_PTR)pwnd->lpfnWndProc, bAnsi);
            if (dwProc == 0)
                RIPMSG1(RIP_WARNING, "GetWindowLong: GWL_WNDPROC: Kernel-side wndproc can't be mapped for pwnd=%#p", pwnd);
        } else {

            /*
             * Keep edit control behavior compatible with NT 3.51.
             */
            if (GETFNID(pwnd) == FNID_EDIT) {
                dwProc = (ULONG_PTR)MapKernelClientFnToClientFn(pwnd->lpfnWndProc);
                goto CheckAnsiUnicodeMismatch;
            } else {
                PCLS pcls = REBASEALWAYS(pwnd, pcls);
                dwProc = MapClientNeuterToClientPfn(pcls, (KERNEL_ULONG_PTR)pwnd->lpfnWndProc, bAnsi);
            }

            /*
             * If the client mapping didn't change the window proc then see if
             * we need a callproc handle.
             */
            if (dwProc == (KERNEL_ULONG_PTR)pwnd->lpfnWndProc) {
CheckAnsiUnicodeMismatch:
                /*
                 * Need to return a CallProc handle if there is an Ansi/Unicode mismatch
                 */
                if (bAnsi != (TestWF(pwnd, WFANSIPROC) ? TRUE : FALSE)) {
                    dwCPDType |= bAnsi ? CPD_ANSI_TO_UNICODE : CPD_UNICODE_TO_ANSI;
                }
            }

            if (dwCPDType) {
                ULONG_PTR cpd;

                cpd = GetCPD(pwnd, dwCPDType | CPD_WND, KERNEL_ULONG_PTR_TO_ULONG_PTR(dwProc));

                if (cpd) {
                    dwProc = cpd;
                } else {
                    RIPMSG0(RIP_WARNING, "GetWindowLong unable to alloc CPD returning handle\n");
                }
            }
        }

        /*
         * return proc (or CPD handle)
         */
        return KERNEL_ULONG_PTR_TO_ULONG_PTR(dwProc);

    case GWLP_HWNDPARENT:

        /*
         * If the window is the desktop window, return
         * NULL to keep it compatible with Win31 and
         * to prevent any access to the desktop owner
         * window.
         */
        if (GETFNID(pwnd) == FNID_DESKTOP) {
            return 0;
        }

        /*
         * Special case for pre-1.1 versions of Windows
         * Set/GetWindowWord(GWL_HWNDPARENT) needs to be mapped
         * to the hwndOwner for top level windows.
         *
         * Note that we find the desktop window through the
         * pti because the PWNDDESKTOP macro only works in
         * the server.
         */

        /*
         * Remove this test when we later add a test for WFDESTROYED
         * in Client handle validation.
         */
        if (pwnd->spwndParent == NULL) {
            return 0;
        }
        pwndParent = REBASEALWAYS(pwnd, spwndParent);
        if (GETFNID(pwndParent) == FNID_DESKTOP) {
            pwnd = REBASEPWND(pwnd, spwndOwner);
            return (ULONG_PTR)HW(pwnd);
        }

        return (ULONG_PTR)HW(pwndParent);

    /*
     * WOW uses a pointer straight into the window structure.
     */
    case GWLP_WOWWORDS:
        return (ULONG_PTR) &pwnd->state;

    }

    RIPERR0(ERROR_INVALID_INDEX, RIP_VERBOSE, "");
    return 0;
}
