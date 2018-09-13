/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WMSG16.C
 *  WOW32 16-bit message thunks
 *
 *  History:
 *  Created 11-Mar-1991 by Jeff Parsons (jeffpar)
 *  Changed 12-May-1992 by Mike Tricker (miketri) to add MultiMedia (un)thunks and messages
--*/

#include "precomp.h"
#pragma hdrstop
#include "wmtbl32.h"
#ifdef FE_IME
#include "wownls.h"
#include "ime.h"
#endif // FE_IME

MODNAME(wmsg16.c);

#define WIN30_STM_SETICON  0x400
#define WIN30_STM_GETICON  0x401

#define WIN30_EM_LINESCROLL  0x406    // WM_USER+6
#define WIN30_EM_GETTHUMB    0x40e    // WM_USER+14

// See WARNING below!
LPFNTHUNKMSG16 apfnThunkMsg16[] = {
    ThunkWMMsg16,   // WOWCLASS_WIN16
    ThunkWMMsg16,   // WOWCLASS_WIN16
    ThunkBMMsg16,   // WOWCLASS_BUTTON
    ThunkCBMsg16,   // WOWCLASS_COMBOBOX
    ThunkEMMsg16,   // WOWCLASS_EDIT
    ThunkLBMsg16,   // WOWCLASS_LISTBOX
    ThunkWMMsg16,   // WOWCLASS_MDICLIENT
    ThunkSBMMsg16,  // WOWCLASS_SCROLLBAR
    ThunkSTMsg16,   // WOWCLASS_STATIC (presumably no messages generated)
    ThunkWMMsg16,   // WOWCLASS_DESKTOP
    ThunkWMMsg16,   // WOWCLASS_DIALOG
    ThunkWMMsg16,   // WOWCLASS_ICONTITLE
    ThunkMNMsg16,   // WOWCLASS_MENU
    ThunkWMMsg16,   // WOWCLASS_SWITCHWND
    ThunkLBMsg16    // WOWCLASS_COMBOLBOX
};
// See WARNING below!
LPFNUNTHUNKMSG16 apfnUnThunkMsg16[] = {
    UnThunkWMMsg16, // WOWCLASS_WIN16
    UnThunkWMMsg16, // WOWCLASS_WIN16
    UnThunkBMMsg16, // WOWCLASS_BUTTON
    UnThunkCBMsg16, // WOWCLASS_COMBOBOX
    UnThunkEMMsg16, // WOWCLASS_EDIT
    UnThunkLBMsg16, // WOWCLASS_LISTBOX
    UnThunkWMMsg16, // WOWCLASS_MDICLIENT
    UnThunkSBMMsg16,// WOWCLASS_SCROLLBAR
    UnThunkSTMsg16, // WOWCLASS_STATIC (presumably no messages generated)
    UnThunkWMMsg16, // WOWCLASS_DESKTOP
    UnThunkWMMsg16, // WOWCLASS_DIALOG
    UnThunkWMMsg16, // WOWCLASS_ICONTITLE
    UnThunkMNMsg16, // WOWCLASS_MENU
    UnThunkWMMsg16, // WOWCLASS_SWITCHWND
    UnThunkLBMsg16  // WOWCLASS_COMBOLBOX
};
//
// WARNING! The above sequence and values must be maintained otherwise the
// #defines in WALIAS.H must be changed.  Same goes for table in WALIAS.C.
//


#ifdef DEBUG

//
// This function returns a pointer to a static buffer containing a generated
// string.  If the function is called twice and generates a string in both
// cases, the second call will overwrite the buffer of the first call.  If
// this becomes a problem for us it would be easy to use an array of N
// static buffers which are cycled through.
//

PSZ GetWMMsgName(UINT uMsg)
{
    static char szStaticBuf[128];
    PSZ pszMsgName;

    uMsg = LOWORD(uMsg);

    pszMsgName = (uMsg < (unsigned)iMsgMax) ? aw32Msg[uMsg].lpszW32 : NULL;

    if (!pszMsgName) {

        if (uMsg < WM_USER) {
            sprintf(szStaticBuf, "(Unknown 0x%x)", uMsg);
        } else if (uMsg < 0xc000) {
            sprintf(szStaticBuf, "(WM_USER+0x%x)", uMsg - WM_USER);
        } else {
            char szAtomName[100];

            if ( ! GlobalGetAtomName((ATOM)uMsg, szAtomName, sizeof szAtomName) &&
                 !       GetAtomName((ATOM)uMsg, szAtomName, sizeof szAtomName) ) {
                szAtomName[0] = 0;
            }

            sprintf(szStaticBuf, "(Atom 0x%x '%s')", uMsg, szAtomName);
        }

        pszMsgName = szStaticBuf;
    }

    return pszMsgName;
}

#endif


// WARNING: This function may cause 16-bit memory movement, invalidating
//          flat pointers.
HWND FASTCALL ThunkMsg16(LPMSGPARAMEX lpmpex)
{
    BOOL f;
    register PWW pww = NULL;
    INT iClass;
    WORD wMsg = lpmpex->Parm16.WndProc.wMsg;

    lpmpex->uMsg = wMsg;
    lpmpex->uParam = INT32(lpmpex->Parm16.WndProc.wParam);  // Sign extend
    lpmpex->lParam =lpmpex->Parm16.WndProc.lParam;
    lpmpex->hwnd   = HWND32(lpmpex->Parm16.WndProc.hwnd);


    if (wMsg < WM_USER) {
        iClass = (aw32Msg[wMsg].lpfnM32 == WM32NoThunking) ?
                                         WOWCLASS_NOTHUNK :  WOWCLASS_WIN16;
    }
    else {
        pww = FindPWW(lpmpex->hwnd);
        if (pww) {
            if (lpmpex->iMsgThunkClass) {
                iClass = lpmpex->iMsgThunkClass;
            } else {
                iClass = GETICLASS(pww, lpmpex->hwnd);
            }
        }
        else {
            iClass = 0;
        }
    }

    lpmpex->iClass = iClass;
    if (iClass == WOWCLASS_NOTHUNK) {
        f = TRUE;
    }
    else {
        lpmpex->lpfnUnThunk16 = apfnUnThunkMsg16[iClass]; // for optimization
        lpmpex->pww = pww;
        WOW32ASSERT(iClass <= NUMEL(apfnThunkMsg16));
        f = (apfnThunkMsg16[iClass])(lpmpex);
    }

    WOW32ASSERTMSG(f, "    WARNING Will Robinson: 16-bit message thunk failure\n");

    return (f) ? lpmpex->hwnd : (HWND)NULL;

}

VOID FASTCALL UnThunkMsg16(LPMSGPARAMEX lpmpex)
{
    if (MSG16NEEDSTHUNKING(lpmpex)) {
        (lpmpex->lpfnUnThunk16)(lpmpex);
    }
    return;
}


BOOL FASTCALL ThunkWMMsg16(LPMSGPARAMEX lpmpex)
{

    WORD wParam   = lpmpex->Parm16.WndProc.wParam;
    LONG lParam   = lpmpex->Parm16.WndProc.lParam;
    PLONG plParamNew = &lpmpex->lParam;

    LOGDEBUG(6,("    Thunking 16-bit window message %s(%04x)\n", (LPSZ)GetWMMsgName(lpmpex->Parm16.WndProc.wMsg), lpmpex->Parm16.WndProc.wMsg));

    switch(lpmpex->Parm16.WndProc.wMsg) {

    case WM_ACTIVATE:   // 006h, <SLPre,       LS>
    case WM_VKEYTOITEM: // 02Eh, <SLPre,SLPost,LS>
    case WM_CHARTOITEM: // 02Fh, <SLPre,SLPost,LS>
    case WM_NCACTIVATE: // 086h, <SLPre,       LS>
    case WM_BEGINDRAG:  // 22Ch, <SLPre,       LS>
        HIW(lpmpex->uParam) = HIWORD(lParam);
        *plParamNew = (LONG)HWND32(LOWORD(lParam));
        break;

    case WM_COMMAND:   // 111h, <SLPre,       LS>
        {
            LONG    lParamNew;

            /*
            ** Some messages cannot be translated into 32-bit messages.  If they
            ** cannot, we leave the lParam as it is, else we replace lParam with
            ** the correct HWND.
            */

            HIW(lpmpex->uParam) = HIWORD(lParam);

            lParamNew = FULLHWND32(LOWORD(lParam));
            if (lParamNew) {
                *plParamNew = lParamNew;
            }

        }
        break;

    case WM_SETFONT:
        lpmpex->uParam = (LONG) HFONT32(wParam);
        break;

    case WM_SYSTIMER:
        lpmpex->uParam = UINT32(wParam);  // un-sign extend the timer ID
        break;

    case WM_SETTEXT:    // 00Ch, <SLPre,SLPost   >
    case WM_WININICHANGE:   // 01Ah, <SLPre,       LS>
    case WM_DEVMODECHANGE:  // 01Bh, <SLPre,       LS>
        {
            LONG lParamMap;

            GETPSZPTR(lParam, (LPSZ)lParamMap);
            *plParamNew = (LONG)AddParamMap(lParamMap, lParam);
            if (lParamMap != *plParamNew) {
                FREEPSZPTR((LPSZ)lParamMap);
            }

        }
        break;

    case WM_ACTIVATEAPP:    // 01Ch
        if (lParam) {
            *plParamNew = (LONG)HTASK32(LOWORD(lParam));
        }
        break;

    case WM_GETTEXT:    // 00Dh, <SLPre,SLPost,LS>
        //
        // SDM (standard dialog manager) used by WinRaid among others
        // has a bug where it claims it has 0x7fff bytes available
        // in the buffer on WM_GETTEXT, when in fact it has much less.
        // Below we intentionally defeat the limit check if the
        // sender claims 0x7fff bytes as the size.  This is done on
        // the checked build only since the free build doesn't perform
        // limit checks.
        // DaveHart/ChandanC 9-Nov-93
        //
#ifdef DEBUG
        GETVDMPTR(lParam, (wParam == 0x7fff) ? 0 : wParam, (LPSZ)*plParamNew);
#else
        GETVDMPTR(lParam, wParam,                          (LPSZ)*plParamNew);
#endif
        break;

    case WM_GETMINMAXINFO:  // 024h, <SLPre,SLPost,LS>,MINMAXINFOSTRUCT
        *plParamNew = (LONG)lpmpex->MsgBuffer;
        ThunkWMGetMinMaxInfo16(lParam, (LPPOINT *)plParamNew);
        break;

    case WM_MDIGETACTIVE:
        //
        // not extremely important if it fails
        //
        *plParamNew = (LONG)&(lpmpex->MsgBuffer[0].msg.lParam);
        lpmpex->uParam = 0;
        break;

    case WM_GETDLGCODE:
        // NTRaid1 #9949 - Excel passes ptr to msg struct in lparam
        //                 Approach 3.1 also does this              a-craigj
        if (lParam) {
            *plParamNew = (LONG)lpmpex->MsgBuffer;
            W32CopyMsgStruct( (VPMSG16)lParam,(LPMSG)*plParamNew, TRUE);
        }
        break;

    case WM_NEXTDLGCTL: // 028h
        if (lParam)
            lpmpex->uParam = (UINT) HWND32(wParam);
        break;

    case WM_DRAWITEM:   // 02Bh  notused, DRAWITEMSTRUCT
        if (lParam) {
            *plParamNew = (LONG)lpmpex->MsgBuffer;
            getdrawitem16((VPDRAWITEMSTRUCT16)lParam, (PDRAWITEMSTRUCT)*plParamNew);
        }
        break;

    case WM_MEASUREITEM:    // 02Ch  notused, MEASUREITEMSTRUCT
        if (lParam) {
            *plParamNew = (LONG)lpmpex->MsgBuffer;
            getmeasureitem16((VPMEASUREITEMSTRUCT16)lParam, (PMEASUREITEMSTRUCT)*plParamNew, lpmpex->Parm16.WndProc.hwnd);
        }
        break;

    case WM_DELETEITEM: // 02Dh  notused, DELETEITEMSTRUCT
        if (lParam) {
            *plParamNew = (LONG)lpmpex->MsgBuffer;
            getdeleteitem16((VPDELETEITEMSTRUCT16)lParam, (PDELETEITEMSTRUCT)*plParamNew);
        }
        break;

    case WM_COMPAREITEM:    // 039h
        if (lParam) {
            *plParamNew = (LONG)lpmpex->MsgBuffer;
            getcompareitem16((VPCOMPAREITEMSTRUCT16)lParam, (PCOMPAREITEMSTRUCT)*plParamNew);
        }
        break;

    case WM_WINHELP:      // 038h  private internal message
        if (lParam) {
            // lparam is LPHLP16, but we need only the size of data, the first word.
            // lparam32 is LPHLP. LPHLP and LPHLP16 are identical.

            PWORD16 lpT;
            GETVDMPTR(lParam, 0, lpT);
            if (lpT) {
                // assert: cbData is a WORD and is the 1st field in LPHLP struct
                WOW32ASSERT((OFFSETOF(HLP,cbData) == 0) &&
                              (sizeof(((LPHLP)NULL)->cbData) == sizeof(WORD)));
                *plParamNew = (LONG)((*lpT > sizeof(lpmpex->MsgBuffer)) ?
                                                malloc_w(*lpT) : lpmpex->MsgBuffer);
                if (*plParamNew) {
                    RtlCopyMemory((PVOID)*plParamNew, lpT, *lpT);
                }
            }
            FREEVDMPTR(lpT);
        }
        break;

    case WM_SIZING: // 0214h, <SLPre,SLPost,LS>,RECT
        if (lParam) {
            *plParamNew = (LONG)lpmpex->MsgBuffer;
            getrect16((VPRECT16)lParam, (LPRECT)*plParamNew);
        }
        break;

    case WM_NCCALCSIZE: // 083h, <SLPre,SLPost,LS>,RECT
        if (lParam) {
            *plParamNew = (LONG)lpmpex->MsgBuffer;
            getrect16((VPRECT16)lParam, (LPRECT)*plParamNew);
            if (wParam) {
                PNCCALCSIZE_PARAMS16 pnc16;
                PNCCALCSIZE_PARAMS16 lpnc16;
                LPNCCALCSIZE_PARAMS  lpnc;


                lpnc  = (LPNCCALCSIZE_PARAMS)*plParamNew;
                pnc16 = (PNCCALCSIZE_PARAMS16)lParam;
                getrect16((VPRECT16)(&pnc16->rgrc[1]), &lpnc->rgrc[1]);
                getrect16((VPRECT16)(&pnc16->rgrc[2]), &lpnc->rgrc[2]);

                lpnc->lppos = (LPWINDOWPOS)(lpnc+1);

                GETVDMPTR( pnc16, sizeof(NCCALCSIZE_PARAMS16), lpnc16 );

                getwindowpos16( (VPWINDOWPOS16)lpnc16->lppos, lpnc->lppos );

                FREEVDMPTR( lpnc16 );
            }
        }
        break;

    case WM_HSCROLL:
    case WM_VSCROLL:
        *plParamNew = (LONG) HWND32(HIWORD(lParam));
#if 0
        if ((wParam == SB_THUMBPOSITION) || (wParam == SB_THUMBTRACK)) {
            HIW(lpmpex->uParam) = LOWORD(lParam);
        }
        else if (wParam > SB_ENDSCROLL) {
//        adding this '}' to balance the opening brace above.
#else

        //
        // Ventura Publisher v4.1 setup program uses nPos on messages other
        // than SB_THUMBPOSITION and SB_THUMBTRACK.  it doesn't hurt to
        // carry this word over.
        //

        if (wParam <= SB_ENDSCROLL) {

            HIW(lpmpex->uParam) = LOWORD(lParam);

        } else {
#endif

        // implies wParam is NOT an SB_* scrollbar code.
        // this could be EM_GETTHUMB or EM_LINESCROLL

        // expensive way would be to check for class etc. Instead we
        // assume that wParam is one of the above EM_message and verify
        // that it is indeed so.

        if (wParam == WIN30_EM_GETTHUMB)
            lpmpex->uParam = EM_GETTHUMB;
        else if (wParam == WIN30_EM_LINESCROLL)
            lpmpex->uParam = EM_LINESCROLL;
        }
        break;

    case WM_PARENTNOTIFY:
        if ((wParam == WM_CREATE) || (wParam == WM_DESTROY))  {
        HIW(lpmpex->uParam) = HIWORD(lParam);
        *plParamNew = (LONG) HWND32(LOWORD(lParam));
        }
        break;

    case WM_MENUCHAR:   // 120h
        LOW(lpmpex->uParam) = wParam;
        HIW(lpmpex->uParam) = LOWORD(lParam);
        *plParamNew = (LONG) HMENU32(HIWORD(lParam));
        break;

    case WM_SETFOCUS:   // 007h, <SLPre,       LS>
    case WM_KILLFOCUS:  // 008h, <SLPre,       LS>
    case WM_SETCURSOR:  // 020h, <SLPre,       LS>
    case WM_INITDIALOG:     // 110h, <SLPre,SLPost,LS>
    case WM_MOUSEACTIVATE:  // 021h, <SLPre,SLPost,LS>
    case WM_MDIDESTROY:     // 221h, <SLPre,       LS>
    case WM_MDIRESTORE:     // 223h, <SLPre,       LS>
    case WM_MDINEXT:        // 224h, <SLPre,       LS>
    case WM_MDIMAXIMIZE:    // 225h, <SLPre,       LS>
    case WM_VSCROLLCLIPBOARD:   // 30Ah, <SLPre,       LS>
    case WM_HSCROLLCLIPBOARD:   // 30Eh, <SLPre,       LS>
    case WM_PALETTECHANGED: // 311h, <SLPre,       LS>
    case WM_PALETTEISCHANGING:
        lpmpex->uParam = (UINT)HWND32(wParam);
        break;

    case WM_DDE_REQUEST:
    case WM_DDE_TERMINATE:
    case WM_DDE_UNADVISE:
        lpmpex->uParam = (UINT)FULLHWND32(wParam);
        break;

    case WM_ASKCBFORMATNAME:
        /* BUGBUGBUG -- neither thunk or unthunk should be necessary,
           since the system does not process this message in DefWindowProc
           FritzS  */
        lpmpex->uParam = (UINT) wParam;

        if (!(*plParamNew = (LPARAM)malloc_w(wParam))) {
            LOGDEBUG (0, ("WOW::WMSG16: WM_ASKCBFORMAT : Couldn't allocate 32 bit memory !\n"));
            WOW32ASSERT (FALSE);
            return FALSE;
        } else {
            getstr16((VPSZ)lParam, (LPSZ)(*plParamNew), wParam);
        }
        break;

    case WM_PAINTCLIPBOARD:
    case WM_SIZECLIPBOARD:
    {
        HANDLE  hMem32 = NULL;
        VPVOID  vp = 0;
        HAND16  hMem16 = 0;
        LPVOID  lpMem32 = NULL;
        WORD wMsg     = lpmpex->Parm16.WndProc.wMsg;

        lpmpex->uParam = (UINT) HWND32(wParam);

        hMem16 = LOWORD(lParam);

        vp = GlobalLock16(hMem16, NULL);
        if (vp) {
            hMem32 = WOWGLOBALALLOC(GMEM_DDESHARE, (wMsg == WM_SIZECLIPBOARD) ?
                                         sizeof(RECT) : sizeof(PAINTSTRUCT));
            if (hMem32) {
                if (lpMem32 = GlobalLock(hMem32)) {
                    if (wMsg == WM_SIZECLIPBOARD) {
                        GETRECT16(vp, (LPRECT)lpMem32);
                    }
                    else {
                        getpaintstruct16(vp, (LPPAINTSTRUCT)lpMem32);
                    }
                    GlobalUnlock((HANDLE) hMem32);
                }
                else {
                    WOWGLOBALFREE(hMem32);
                    hMem32 = NULL;
                    LOGDEBUG (0, ("WOW::WMSG16: WM_SIZE/PAINTCLIPBOARD : Couldn't lock 32 bit handle !\n"));
                    WOW32ASSERT (FALSE);
                }
            }
            else {
                LOGDEBUG (0, ("WOW::WMSG16: WM_SIZE/PAINTCLIPBOARD : Couldn't allocate memory !\n"));
                WOW32ASSERT (FALSE);
            }

            GlobalUnlock16(hMem16);
        }
        else {
            LOGDEBUG (0, ("WOW::WMSG16: WM_SIZE/PAINTCLIPBOARD : Couldn't lock 16 bit handle !\n"));
            WOW32ASSERT (FALSE);
        }

        *plParamNew = (LONG) hMem32;
     }
     break;

    case WM_MDIACTIVATE:
        {
            BOOL fHwndIsMdiChild;

            if (lpmpex->iMsgThunkClass != WOWCLASS_MDICLIENT) {
                PWW  pww;
                HWND hwnd32;

                // AMIPRO sends this message to its own window. If we thunk the
                // message the usual way, we will lose the information in
                // wParam and won't be able to regenerate the original message
                // when it comes back via w32win16wndproc. So the solution is
                // to determine this case and not thunk the message at all.
                //                                                  - nanduri

                // HYPERION sends this to its own DIALOG window.  Added
                // WOWCLASS_DIALOG check. - sanfords

                //
                // Expensive checks.
                // No thunking If hwnd16 is of WOWCLASS and NOT MDICHILD.
                //

                hwnd32 = HWND32(lpmpex->Parm16.WndProc.hwnd);
                if (pww = (PWW)GetWindowLong(hwnd32, GWL_WOWWORDS)) {
                    INT wClass;
                    wClass = GETICLASS(pww, hwnd32);

                    if ((wClass == WOWCLASS_WIN16 ||
                            wClass == WOWCLASS_DIALOG)
                            && (!(pww->ExStyle & WS_EX_MDICHILD))) {
                        lpmpex->uMsg = WM_MDIACTIVATE | WOWPRIVATEMSG;
                        break;
                    }

                }
            }


            //
            // see the comment in 32-16 thunk for this message.
            //

            if (lParam) {

                //
                // Corel Chart doesn't set lParam to zero.
                // Instead HIWORD(lParam) = 0 and LOWORD(lParam) = wParam
                // If we do the normal child window processing, focus won't
                // change, because the wrong window handle will be in the
                // wParam for the 32 bit message.  This would not be a problem,
                // except that win32 swapped the positions of the activate and
                // deactivate handles for the WM_MDIACTIVATE messages sent
                // to the child window.  Under win31, the non-zero lParam is
                // ignored.
                //
                if ((CURRENTPTD()->dwWOWCompatFlags & WOWCF_WMMDIACTIVATEBUG) && (HIWORD(lParam) == 0) && (wParam == LOWORD(lParam))){
                    fHwndIsMdiChild = FALSE;
                } else {
                    fHwndIsMdiChild = TRUE;
                }
            }
            else {
                if (wParam && (lpmpex->Parm16.WndProc.hwnd == (HWND16)wParam)) {
                    fHwndIsMdiChild = TRUE;
                }
                else {
                    fHwndIsMdiChild = FALSE;
                }

            }

            if (fHwndIsMdiChild) {
                lpmpex->uParam = (UINT)FULLHWND32(HIWORD(lParam));
                *plParamNew = (UINT)FULLHWND32(LOWORD(lParam));

            }
            else {
                lpmpex->uParam = (UINT)FULLHWND32(wParam);
                *plParamNew = (UINT)0;
            }
        }
        break;

    case WM_MDISETMENU: // 230h

        // Refresh if wParam of WM_MDISETMENU is TRUE (the refresh flag)
        //
        if (wParam) {
            lpmpex->uMsg = WM_MDIREFRESHMENU;
        }
        lpmpex->uParam = (UINT)HMENU32(LOWORD(lParam));
        *plParamNew = (UINT)HMENU32(HIWORD(lParam));
        break;

    case WIN31_MM_CALCSCROLL:  // 10ACh
        if (lpmpex->iClass == WOWCLASS_MDICLIENT) {
            lpmpex->uMsg = MM_CALCSCROLL;
        }
        break;

    case WM_MDITILE:    // 226h
        /* if wParam contains garbage from Win3.0 apps */
        if(wParam & ~(MDITILE_VERTICAL|MDITILE_HORIZONTAL|MDITILE_SKIPDISABLED))
           lpmpex->uParam = MDITILE_SKIPDISABLED;
        break;


    case WM_MDICASCADE: // 227h
        lpmpex->uParam = MDITILE_SKIPDISABLED;
        break;

    case WM_ERASEBKGND: // 014h, <  SLPost   >
    case WM_ICONERASEBKGND: // 027h
        lpmpex->uParam = (UINT)HDC32(wParam);
        break;

    case WM_CTLCOLOR:

        // HIWORD(lParam) need not be a standard index. The app can pass any
        // value (PowerBuilder does so.  MSGolf passes this message to
        // DefDlgProc() with HIWORD(lParam) == 62,66,67).
        //
        // If not in known range, leave it as WM_CTLCOLOR. There is code in
        // xxxDefWindowProc() & xxxDefDlgProc() that recognize this & return
        // us the value returned by the app when it processed this message.

        if (HIWORD(lParam) <= (WORD)(WM_CTLCOLORSTATIC -  WM_CTLCOLORMSGBOX)) {
            lpmpex->hwnd   = (HWND)FULLHWND32(GETHWND16(lpmpex->hwnd));
            lpmpex->uMsg   = WM_CTLCOLORMSGBOX + HIWORD(lParam);
            lpmpex->uParam = (UINT)HDC32(wParam);
            *plParamNew = (LONG)HWND32(LOWORD(lParam));
        }
        break;


    case WM_SYSCOMMAND:
    case WM_SETREDRAW:     // 027h
        lpmpex->uParam = wParam;
        break;

    case WM_INITMENU:
    case WM_INITMENUPOPUP:  // 117h
        lpmpex->uParam = (UINT)HMENU32(wParam);
        break;

    case WM_NCCREATE:
    case WM_CREATE:
        {
        register    LPCREATESTRUCT  lpCreateStruct;
        register    PCREATESTRUCT16 lpCreateStruct16;

        if (HIWORD(lParam)) {

            HWND hwnd32;
            PWW  pww = NULL;

            lpCreateStruct = (LPCREATESTRUCT) lpmpex->MsgBuffer;
            // ChandanC check the return value !!!

            GETVDMPTR(lParam, sizeof(CREATESTRUCT16), lpCreateStruct16);

            lpCreateStruct->lpCreateParams = (LPSTR)FETCHDWORD(lpCreateStruct16->vpCreateParams);
            lpCreateStruct->hInstance = HMODINST32(lpCreateStruct16->hInstance);
            lpCreateStruct->hMenu = HMENU32(lpCreateStruct16->hMenu);
            lpCreateStruct->hwndParent = HWND32(lpCreateStruct16->hwndParent);
            lpCreateStruct->cy = (SHORT) lpCreateStruct16->cy;
            lpCreateStruct->cx = (SHORT) lpCreateStruct16->cx;
            lpCreateStruct->y = (SHORT) lpCreateStruct16->y;
            lpCreateStruct->x = (SHORT) lpCreateStruct16->x;
            lpCreateStruct->style = lpCreateStruct16->dwStyle;
            GETPSZPTR(lpCreateStruct16->vpszWindow, lpCreateStruct->lpszName);
            GETPSZIDPTR(lpCreateStruct16->vpszClass, lpCreateStruct->lpszClass);
            lpCreateStruct->dwExStyle = lpCreateStruct16->dwExStyle;

            *plParamNew = (LONG) lpCreateStruct;

            FREEVDMPTR(lpCreateStruct16);

            hwnd32 = HWND32(lpmpex->Parm16.WndProc.hwnd);
            pww = (PWW)GetWindowLong(hwnd32, GWL_WOWWORDS);
            if (lpCreateStruct->lpCreateParams && pww && (pww->ExStyle & WS_EX_MDICHILD)) {
                FinishThunkingWMCreateMDIChild16(*plParamNew,
                                        (LPMDICREATESTRUCT)(lpCreateStruct+1));
            }
        }
        }
        break;

    case WM_PAINT:
    case WM_NCPAINT:
        // 1 is MAXREGION special code in Win 3.1
        lpmpex->uParam =  (wParam == 1) ? 1 :  (UINT)HDC32(wParam);
        break;

    case WM_ENTERIDLE:
        if ((wParam == MSGF_DIALOGBOX) || (wParam == MSGF_MENU)) {
        *plParamNew = (LONG) HWND32(LOWORD(lParam));
        }
        break;

    case WM_MENUSELECT:
        // Copy menu flags
        HIW(lpmpex->uParam) = LOWORD(lParam);

        // Copy "main" menu
        *plParamNew = (LONG) HMENU32(HIWORD(lParam));

        if (LOWORD(lParam) == 0xFFFF || !(LOWORD(lParam) & MF_POPUP)) {
            LOW(lpmpex->uParam) = wParam;      // copy ID
        } else {
            // convert menu to index
            LOW(lpmpex->uParam) =
                    (WORD)(pfnOut.pfnGetMenuIndex)((HMENU)*plParamNew, HMENU32(wParam));
        }
        break;

    case WM_MDICREATE:  // 220h, <SLPre,SLPost,LS>
        *plParamNew = (LONG)lpmpex->MsgBuffer;
        ThunkWMMDICreate16(lParam, (LPMDICREATESTRUCT *)plParamNew);
        break;

    // BUGBUG 25-Aug-91 JeffPar:  Use of the Kludge variables was a temporary
    // measure, and only works for messages sent by Win32;  for any WM
    // messages sent by 16-bit apps themselves, this will not work.  Ultimately,
    // any messages you see being thunked in wmsg32.c will need equivalent
    // thunks here as well.


    case WM_DDE_INITIATE:
        lpmpex->uParam = (LONG) FULLHWND32(wParam);
        WI32DDEAddInitiator((HAND16) wParam);
        break;

    case WM_DDE_ACK:
        {
            WORD wMsg     = lpmpex->Parm16.WndProc.wMsg;
            HWND16 hwnd16 = lpmpex->Parm16.WndProc.hwnd;

            lpmpex->uParam = (LONG) FULLHWND32(wParam);

            if (WI32DDEInitiate((HWND16) hwnd16)) {
                *plParamNew = lParam;
            }
            else {
                HANDLE h32;

                if (fWhoCalled == WOWDDE_POSTMESSAGE) {
                    if (h32 = DDEFindPair32(wParam, hwnd16, (HAND16) HIWORD(lParam))) {
                        *plParamNew = PackDDElParam(wMsg, (LONG) (DWORD) LOWORD(lParam), (LONG) h32);
                    }
                    else {
                        *plParamNew = PackDDElParam(wMsg, (LONG) (DWORD) LOWORD(lParam), (LONG) (DWORD) HIWORD(lParam));
                    }
                }
                else {
                    if (fThunkDDEmsg) {
                        if (h32 = DDEFindPair32(wParam, hwnd16, (HAND16) HIWORD(lParam))) {
                            *plParamNew = PackDDElParam(wMsg, (LONG) (DWORD) LOWORD(lParam), (LONG) h32);
                        }
                        else {
                            *plParamNew = PackDDElParam(wMsg, (LONG) (DWORD) LOWORD(lParam), (LONG) (DWORD) HIWORD(lParam));
                        }

                    }
                    else {
                        *plParamNew = W32GetHookDDEMsglParam();
                    }
                }
            }
        }
        break;

    case WM_DDE_POKE:
        {
        DDEINFO DdeInfo;
        HANDLE  h32;
        HWND16 hwnd16 = lpmpex->Parm16.WndProc.hwnd;
        WORD wMsg     = lpmpex->Parm16.WndProc.wMsg;

        lpmpex->uParam = (LONG) FULLHWND32(wParam);

        if (fWhoCalled == WOWDDE_POSTMESSAGE) {
            if (h32 = DDEFindPair32(hwnd16, wParam, (HAND16) LOWORD(lParam))) {
                DDEDeletehandle(LOWORD(lParam), h32);
                WOWGLOBALFREE(h32);
            }
            DdeInfo.Msg = wMsg;
            h32 = DDECopyhData32(hwnd16, wParam, (HAND16) LOWORD(lParam), &DdeInfo);
            // WARNING: 16-bit memory may have moved
            DdeInfo.Flags = DDE_PACKET;
            DdeInfo.h16 = 0;
            DDEAddhandle(hwnd16, wParam, (HAND16)LOWORD(lParam), h32, &DdeInfo);
            *plParamNew = PackDDElParam(wMsg, (LONG) h32, (LONG) HIWORD(lParam));
        }
        else {
            if (fThunkDDEmsg) {
                if (!(h32 = DDEFindPair32(hwnd16, wParam, (HAND16) LOWORD(lParam)))) {
                    LOGDEBUG (0, ("WOW::WMSG16: WM_DDE_POKE : Can't find h32 !\n"));
                }
                *plParamNew = PackDDElParam(wMsg, (LONG) h32, (LONG) HIWORD(lParam));
            }
            else {
                *plParamNew = W32GetHookDDEMsglParam();
            }
        }
        }
        break;



    case WM_DDE_ADVISE:
        {
        DDEINFO DdeInfo;
        HANDLE  h32;
        INT cb;
        VPVOID  vp;
        LPBYTE  lpMem16;
        LPBYTE  lpMem32;
        HWND16 hwnd16 = lpmpex->Parm16.WndProc.hwnd;
        WORD wMsg     = lpmpex->Parm16.WndProc.wMsg;

        lpmpex->uParam = (LONG) FULLHWND32(wParam);

        if (fWhoCalled == WOWDDE_POSTMESSAGE) {
            if (h32 = DDEFindPair32(hwnd16, wParam, (HAND16) LOWORD(lParam))) {
                DDEDeletehandle(LOWORD(lParam), h32);
                WOWGLOBALFREE(h32);
            }
            h32 = WOWGLOBALALLOC(GMEM_DDESHARE, sizeof(DDEADVISE));
            if (h32 == NULL) {
                return 0;
            }
            lpMem32 = GlobalLock(h32);
            vp = GlobalLock16(LOWORD(lParam), &cb);
            GETMISCPTR(vp, lpMem16);
            RtlCopyMemory(lpMem32, lpMem16, sizeof(DDEADVISE));
            FREEMISCPTR(lpMem16);
            GlobalUnlock(h32);
            GlobalUnlock16(LOWORD(lParam));
            DdeInfo.Msg = wMsg;
            DdeInfo.Format = 0;
            DdeInfo.Flags = DDE_PACKET;
            DdeInfo.h16 = 0;
            DDEAddhandle(hwnd16, wParam, (HAND16)LOWORD(lParam), h32, &DdeInfo);
            *plParamNew = PackDDElParam(wMsg, (LONG) h32, (LONG) HIWORD(lParam));
        }
        else {
            if (fThunkDDEmsg) {
                if (!(h32 = DDEFindPair32(hwnd16, wParam, (HAND16) LOWORD(lParam)))) {
                    LOGDEBUG (0, ("WOW::WMSG16: WM_DDE_ADVISE : Can't find h32 !\n"));
                }
                *plParamNew = PackDDElParam(wMsg, (LONG) h32, (LONG) HIWORD(lParam));
            }
            else {
                *plParamNew = W32GetHookDDEMsglParam();
            }
        }
        }
        break;

    case WM_DDE_DATA:
        {
        DDEINFO DdeInfo;
        HANDLE h32;
        HWND16 hwnd16 = lpmpex->Parm16.WndProc.hwnd;
        WORD wMsg     = lpmpex->Parm16.WndProc.wMsg;

        lpmpex->uParam = (LONG) FULLHWND32(wParam);

        if (fWhoCalled == WOWDDE_POSTMESSAGE) {
            if (h32 = DDEFindPair32(hwnd16, wParam, (HAND16) LOWORD(lParam))) {
                DDEDeletehandle(LOWORD(lParam), h32);
                WOWGLOBALFREE(h32);
            }

            if (!LOWORD(lParam)) {
                h32 = 0;
            }
            else {
                DdeInfo.Msg = wMsg;
                h32 = DDECopyhData32(hwnd16, wParam, (HAND16) LOWORD(lParam), &DdeInfo);
                // WARNING: 16-bit memory may have moved
                DdeInfo.Flags = DDE_PACKET;
                DdeInfo.h16 = 0;
                DDEAddhandle(hwnd16, wParam, (HAND16)LOWORD(lParam), h32, &DdeInfo);
            }

            *plParamNew = PackDDElParam(wMsg, (LONG) h32, (LONG) HIWORD(lParam));
        }
        else {
            if (fThunkDDEmsg) {
                if (!LOWORD(lParam)) {
                    h32 = 0;
                }
                else {
                    if (!(h32 = DDEFindPair32(hwnd16, wParam, (HAND16) LOWORD(lParam)))) {
                        LOGDEBUG (0, ("WOW::WMSG16: WM_DDE_DATA : Can't find h32 !\n"));
                    }
                }
                *plParamNew = PackDDElParam(wMsg, (LONG) h32, (LONG) HIWORD(lParam));
            }
            else {
                *plParamNew = W32GetHookDDEMsglParam();
            }
        }
        }
        break;

    case WM_DDE_EXECUTE:
        {
        DDEINFO DdeInfo;
        HANDLE  h32;
        HAND16  h16;
        INT     cb;
        VPVOID  vp;
        VPVOID  vp1;
        LPBYTE  lpMem16;
        LPBYTE  lpMem32;
        HWND16 hwnd16 = lpmpex->Parm16.WndProc.hwnd;
        WORD wMsg     = lpmpex->Parm16.WndProc.wMsg;

        lpmpex->uParam = (LONG) FULLHWND32(wParam);

        if (fWhoCalled == WOWDDE_POSTMESSAGE) {
            vp = GlobalLock16(HIWORD(lParam), &cb);

            GETMISCPTR(vp, lpMem16);
            h32 = WOWGLOBALALLOC(GMEM_DDESHARE, cb);
            if (h32) {
                lpMem32 = GlobalLock(h32);
                RtlCopyMemory(lpMem32, lpMem16, cb);
                GlobalUnlock(h32);
                FREEMISCPTR(lpMem16);
                //
                // The alias is checked to make bad apps do WM_DDE_EXECUTE
                // correctly. One such app is SuperPrint. This app issues
                // multiple WM_DDE_EXECUTEs without waiting for WM_DDE_ACK to
                // come. Also, it uses the same h16 on these messages.
                // We get around this problem by generating a unique h16-h32
                // pairing each time. And freeing h16 when the WM_DDE_ACK comes.
                // In WM32DDEAck, we need to free this h16 because we allocated
                // this one. Apply this hack only if the h16 is valid. Caere
                // OmniPage passes hard coded constants in HIWORD(lParam).
                //
                // SunilP, ChandanC 4-30-93
                //
                if (vp && DDEFindPair32(hwnd16, wParam, (HAND16) HIWORD(lParam))) {
                    vp1 = GlobalAllocLock16(GMEM_DDESHARE, cb, &h16);
                    if (vp1) {
                        GETMISCPTR(vp1, lpMem16);
                        RtlCopyMemory(lpMem16, lpMem32, cb);
                        FLUSHVDMPTR(vp1, cb, lpMem16);
                        FREEMISCPTR(lpMem16);
                        GlobalUnlock16(h16);

                        DdeInfo.Msg = wMsg;
                        DdeInfo.Format = 0;
                        DdeInfo.Flags = DDE_EXECUTE_FREE_H16 | DDE_PACKET;
                        DdeInfo.h16 = (HAND16) HIWORD(lParam);
                        DDEAddhandle(hwnd16, wParam, h16, h32, &DdeInfo);
                    }
                    else {
                        LOGDEBUG (0, ("WOW::WMSG16: WM_DDE_EXECUTE : Can't allocate h16 !\n"));
                    }
                }
                else {
                    DdeInfo.Msg = wMsg;
                    DdeInfo.Format = 0;
                    DdeInfo.Flags = DDE_PACKET;
                    DdeInfo.h16 = 0;
                    DDEAddhandle(hwnd16, wParam, (HAND16)HIWORD(lParam), h32, &DdeInfo);
                }
            }
            else {
                GlobalUnlock16(HIWORD(lParam));
            }
            GlobalUnlock16(HIWORD(lParam));
        }
        else {
            if (!(h32 = DDEFindPair32(hwnd16, wParam, (HAND16) HIWORD(lParam)))) {
                LOGDEBUG (0, ("WOW::WMSG16: WM_DDE_EXECUTE : Can't find h32 !\n"));
            }
        }

        *plParamNew = (ULONG)h32;
        }
        break;



    case WM_COPYDATA:
        {
        LPBYTE lpMem16;
        LPBYTE lpMem32;
        PCOPYDATASTRUCT16 lpCDS16;
        PCOPYDATASTRUCT lpCDS32 = NULL;
        PCPDATA pTemp;
        HWND16 hwnd16 = lpmpex->Parm16.WndProc.hwnd;

        lpmpex->uParam = (LONG) HWND32(wParam);

        if (fWhoCalled == WOWDDE_POSTMESSAGE) {
            GETMISCPTR(lParam, lpCDS16);
            if (lpCDS32 = (PCOPYDATASTRUCT) malloc_w(sizeof(COPYDATASTRUCT))) {
                lpCDS32->dwData = lpCDS16->dwData;
                if (lpCDS32->cbData = lpCDS16->cbData) {
                    if (lpMem32 = malloc_w(lpCDS32->cbData)) {
                        GETMISCPTR(lpCDS16->lpData, lpMem16);
                        if (lpMem16) {
                            RtlCopyMemory(lpMem32, lpMem16, lpCDS32->cbData);
                            CopyDataAddNode (hwnd16, wParam, (DWORD) lpMem16, (DWORD) lpMem32, COPYDATA_16);
                        }
                        FREEMISCPTR(lpMem16);
                    }
                    lpCDS32->lpData = lpMem32;
                }
                else {
                    lpCDS32->lpData = NULL;
                }
            }
            FREEMISCPTR(lpCDS16);

            CopyDataAddNode (hwnd16, wParam, (DWORD) lParam, (DWORD) lpCDS32, COPYDATA_16);
        }
        else {
            pTemp = CopyDataFindData32 (hwnd16, wParam, lParam);
            lpCDS32 = (PCOPYDATASTRUCT) pTemp->Mem32;
            WOW32ASSERTMSGF(lpCDS32, ("WOW::WM_COPYDATA:Can't locate lpCDS32\n"));
        }

        *plParamNew = (LONG)lpCDS32;
        }
        break;


    // Win 3.1 messages

    case WM_DROPFILES:
        lpmpex->uParam = (UINT)HDROP32(wParam);
        WOW32ASSERT(lpmpex->uParam != 0);
        break;

    case WM_DROPOBJECT:
    case WM_QUERYDROPOBJECT:
    case WM_DRAGLOOP:
    case WM_DRAGSELECT:
    case WM_DRAGMOVE:
        {
        register   LPDROPSTRUCT  lpds;
        register   PDROPSTRUCT16 lpds16;

        if (lParam) {

            lpds = (LPDROPSTRUCT) lpmpex->MsgBuffer;

            GETVDMPTR(lParam, sizeof(DROPSTRUCT16), lpds16);

            lpds->hwndSource     = HWND32(lpds16->hwndSource);
            lpds->hwndSink       = HWND32(lpds16->hwndSink);
            lpds->wFmt           = lpds16->wFmt;
            lpds->ptDrop.y       = (LONG)lpds16->ptDrop.y;
            lpds->ptDrop.x       = (LONG)lpds16->ptDrop.x;
            lpds->dwControlData  = lpds16->dwControlData;

            *plParamNew = (LONG) lpds;

            FREEVDMPTR(lpds16);
        }
        }
        break;

    case WM_NEXTMENU:  // Thunk
        *plParamNew = (LONG)lpmpex->MsgBuffer;
        ((PMDINEXTMENU)(*plParamNew))->hmenuIn = HMENU32(LOWORD(lParam));
        break;

    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        if (lParam) {
            lpmpex->lParam = (LONG) lpmpex->MsgBuffer;
            getwindowpos16( (VPWINDOWPOS16)lParam, (LPWINDOWPOS)lpmpex->lParam );
        }
        break;

    case WM_TIMER:
        {
        HAND16  htask16;
        PTMR    ptmr;
        WORD    wIDEvent;

        htask16  = CURRENTPTD()->htask16;
        wIDEvent = wParam;

        ptmr = FindTimer16( lpmpex->Parm16.WndProc.hwnd, htask16, wIDEvent );

        if ( !ptmr ) {
            if ( lParam == 0L ) {
                /*
                ** Edit controls have timers which can be sent straight
                ** through without thunking... (wParam=1, lParam=0)
                */
                lpmpex->uParam = (UINT)wIDEvent;
                *plParamNew = 0L;
            } else {
                LOGDEBUG(6,("    ThunkWMMSG16 WARNING: cannot find timer %04x\n", wIDEvent));
            }
        } else {
            lpmpex->uParam = (UINT)wIDEvent;
            *plParamNew = ptmr->dwTimerProc32;      // 32-bit proc or NULL
        }

        }
        break;
#ifdef FE_IME
    case WM_IME_REPORT:
        {
        INT     cb;
        INT     i;
        INT     len;
        VPVOID  vp;
        HANDLE  hMem32 = 0;
        LPBYTE  lpMem32 = 0;
        LPBYTE  lpMem16 = 0;

        if ( !lParam )
            break;

        if (wParam == IR_STRING) {
        /*********************** IR_STRING **********************************/
            vp = GlobalLock16(FETCHWORD(lParam), &cb);
            GETMISCPTR(vp, lpMem16);
            if (!(hMem32 = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE | GMEM_ZEROINIT, cb)))
                goto Err;
            lpMem32 = GlobalLock(hMem32);
            RtlCopyMemory(lpMem32, lpMem16, cb);
            GlobalUnlock( hMem32 );
            GlobalUnlock16( FETCHWORD(lParam) );

            *plParamNew = (LONG)hMem32;
        }
        /*********************** IR_STRINGEX ********************************/
        else if ( wParam == IR_STRINGEX ) {
            LPSTRINGEXSTRUCT    pss32;
            PSTRINGEXSTRUCT16  pss16;
            INT                 uDetermineDelim = 0;
            INT                 uYomiDelim = 0;

            vp = GlobalLock16( FETCHWORD(lParam), &cb );
            GETMISCPTR(vp, lpMem16);
            pss16 = (PSTRINGEXSTRUCT16)lpMem16;

            cb = sizeof(STRINGEXSTRUCT);

            /* Get exactry size */
            if ( lpMem16[ pss16->uDeterminePos ] ) {
                len = lstrlen( &lpMem16[ pss16->uDeterminePos ] );
                cb += len + 1;
                cb += sizeof(INT) - (cb % sizeof(INT)); // #2259 kksuzuka
                // DetermineDelim[0] is everytime NULL
                // BAD CODE
//                for ( i = 0; i < len && INTOF( lpMem16[ pss16->uDetermineDelimPos ], i ); i++ )
                // #7253 kksuzuka
                for ( i = 1; (i <= len) && WORDOF( lpMem16[ pss16->uDetermineDelimPos ], i ); i++ )
//                    if ( INTOF( lpMem16[ pss16->uDetermineDelimPos ], i ) >= len )
                    // #7253 kksuzuka
                    if ( WORDOF( lpMem16[ pss16->uDetermineDelimPos ], i ) >= len )
                        break;
                if ( i <= len )
                    // #7253 kksuzuka
                    cb += (i + 1) * sizeof(INT);
//                    cb += i * sizeof(INT);
                uDetermineDelim = i;
            }
            if ( lpMem16[ pss16->uYomiPos ] ) {
                len = lstrlen( &lpMem16[ pss16->uYomiPos ] );
                cb += len + 1;
                cb += sizeof(INT) - (cb % sizeof(INT)); // #2259 kksuzuka
                // YomiDelim[0] is everytime NULL
                // BAD CODE
//                for ( i = 0; i < len && INTOF( lpMem16[ pss16->uYomiDelimPos ], i ); i++ )
                // #7253 kksuzuka
                for ( i = 1; (i <= len) && WORDOF( lpMem16[ pss16->uYomiDelimPos ], i ); i++ )
//                    if ( INTOF( lpMem16[ pss16->uYomiDelimPos ], i ) >= len )
                    // #7253 kksuzuka
                    if ( WORDOF( lpMem16[ pss16->uYomiDelimPos ], i ) >= len )
                        break;
                if ( i <= len )
                    // #7253 kksuzuka
                    cb += (i + 1) * sizeof(UINT);
//                    cb += i * sizeof(UINT);
                uYomiDelim = i;
            }
            if (!(hMem32 = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE | GMEM_ZEROINIT, cb)))
                goto Err;
            lpMem32 = GlobalLock( hMem32 );
            pss32 = (LPSTRINGEXSTRUCT)lpMem32;

            pss32->dwSize = cb;
            i = sizeof( STRINGEXSTRUCT );
            if ( pss16->uDeterminePos ) {
                pss32->uDeterminePos = i;
                lstrcpy( &lpMem32[ i ], &lpMem16[ pss16->uDeterminePos ] );
                i += lstrlen( &lpMem16[ pss16->uDeterminePos ] ) + 1;
                i += sizeof(INT) - (i % sizeof(INT)); // kksuzuka #2259
            }
            if ( pss16->uDetermineDelimPos ) {
                pss32->uDetermineDelimPos = i;
//                i += uDetermineDelim * sizeof(UINT);
                // #7253 kksuzuka
                i += (uDetermineDelim + 1)* sizeof(UINT);
                for( ; uDetermineDelim ; uDetermineDelim-- ) {
                    INTOF( lpMem32[ pss32->uDetermineDelimPos ], uDetermineDelim ) =
                    WORDOF( lpMem16[ pss16->uDetermineDelimPos ], uDetermineDelim );
                }
            }
            if ( pss16->uYomiPos ) {
                pss32->uYomiPos = i;
                lstrcpy( &lpMem32[ i ], &lpMem16[ pss16->uYomiPos ] );
                i += lstrlen( &lpMem16[ pss16->uYomiPos ] ) + 1;
                i += sizeof(INT) - (i % sizeof(INT)); // kksuzuka #2259
            }
            if ( pss16->uYomiDelimPos ) {
                pss32->uYomiDelimPos = i;
                i += uYomiDelim * sizeof(UINT);
                for( ; uYomiDelim ; uYomiDelim-- ) {
                    INTOF( lpMem32[ pss32->uYomiDelimPos ], uYomiDelim ) =
                    WORDOF( lpMem16[ pss16->uYomiDelimPos ], uYomiDelim );
                }
            }

            *plParamNew = (LONG)hMem32;
            GlobalUnlock16(FETCHWORD(lParam));
            GlobalUnlock( hMem32 );
        }


        else if (wParam == IR_UNDETERMINE) {
        /********************** IR_UNDETERMINE ******************************/
            PUNDETERMINESTRUCT16  pus16;
            LPUNDETERMINESTRUCT  pus32;

            vp = GlobalLock16( FETCHWORD(lParam), &cb );
            GETMISCPTR(vp, lpMem16);
            pus16 = (PUNDETERMINESTRUCT16)lpMem16;

            cb = sizeof(UNDETERMINESTRUCT);
            cb += pus16->uDefIMESize;
            cb += (pus16->uUndetTextLen + 1);
            cb += (pus16->uDetermineTextLen + 1);
            cb += (pus16->uYomiTextLen + 1);

            if ( pus16->uUndetAttrPos )
                cb += pus16->uUndetTextLen;
            if ( pus16->uDetermineDelimPos )
                cb += pus16->uDetermineTextLen * sizeof(UINT);
            if ( pus16->uYomiDelimPos )
                cb += pus16->uYomiTextLen * sizeof(UINT);


            if (!(hMem32 = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE | GMEM_ZEROINIT, cb)))
                goto Err;
            lpMem32 = GlobalLock(hMem32);
            pus32 = (LPUNDETERMINESTRUCT)lpMem32;

            i = sizeof(UNDETERMINESTRUCT);
            if ( pus16->uUndetTextLen ) {
                RtlCopyMemory( &lpMem32[ i ], &lpMem16[ pus16->uUndetTextPos ], pus16->uUndetTextLen + 1 );
                pus32->uUndetTextPos = i;
                i += pus16->uUndetTextLen + 1;
                pus32->uUndetTextLen = pus16->uUndetTextLen;
            }
            if ( pus16->uUndetAttrPos ) {
                RtlCopyMemory( &lpMem32[ i ], &lpMem16[ pus16->uUndetAttrPos ], pus16->uUndetTextLen );
                pus32->uUndetAttrPos = i;
                i += pus16->uUndetTextLen;
            }
            if ( pus16->uDetermineTextLen ) {
                RtlCopyMemory( &lpMem32[ i ], &lpMem16[ pus16->uDetermineTextPos ], pus16->uDetermineTextLen + 1 );
                pus32->uDetermineTextPos = i;
                i += pus16->uDetermineTextLen + 1;
                pus32->uDetermineTextLen = pus16->uDetermineTextLen;
            }
            if ( pus16->uDetermineDelimPos ) {
                INT j;

                pus32->uDetermineDelimPos = i;
                for ( j = 0; j < pus16->uDetermineTextLen; j++ ) {
                    if ( WORDOF16( lpMem16[ pus16->uDetermineTextLen ], j ) || pus16->uDetermineTextLen > WORDOF16( lpMem16[ pus16->uDetermineTextLen ], j )) {
                        INTOF( lpMem32[ i ], 0 ) = WORDOF16( lpMem16[ pus16->uDetermineTextLen ], j );
                        i += sizeof(UINT);
                    }
                    else
                        break;
                }
            }
            if ( pus16->uYomiTextLen ) {
                RtlCopyMemory( &lpMem32[ i ], &lpMem16[ pus16->uYomiTextPos ], pus16->uYomiTextLen + 1 );
                pus32->uYomiTextPos = i;
                pus32->uYomiTextLen = pus16->uYomiTextLen;
                i += pus16->uYomiTextLen + 1;
            }
            if ( pus16->uYomiDelimPos ) {
                INT j;
                pus32->uYomiDelimPos = i;
                for ( j = 0; j < pus16->uYomiTextLen; j++ ) {
                    if ( WORDOF16(lpMem16[ pus16->uYomiDelimPos ], j ) || pus16->uYomiTextLen > WORDOF16(lpMem16[ pus16->uYomiDelimPos ], j )) {
                        INTOF( lpMem32[ i ], 0 ) = WORDOF16( lpMem16[ pus16->uYomiDelimPos ], j );
                        i += sizeof(UINT);
                    }
                    else
                        break;
                }
            }
            if ( pus16->uDefIMESize ) {
                RtlCopyMemory( &lpMem32[ i ], &lpMem16[ pus16->uDefIMEPos ], pus16->uDefIMESize );
                pus32->uDefIMEPos = i;
            }


            *plParamNew = (LONG)hMem32;
            GlobalUnlock16( FETCHWORD(lParam));
            GlobalUnlock( hMem32 );

        }
        break;

      Err:
        if ( lpMem16 && FETCHWORD(lParam ))
             GlobalUnlock16( FETCHWORD(lParam) );
        return FALSE;

        }
        break;
    // MSKK support WM_IMEKEYDOWN message
    // MSKK support WM_IMEKEYUP message
    // MSKK16bit IME support
    // WM_IMEKEYDOWN & WM_IMEKEYUP  16 -> 32
    // 32bit:wParam  HIWORD charactor code, LOWORD virtual key
    // 16bit:wParam  HIBYTE charactor code, LOBYTE virtual key
    // kksuzuka:#4281 1994.11.19 MSKK V-HIDEKK
    case WM_IMEKEYDOWN:
    case WM_IMEKEYUP:
#ifdef DEBUG
        LOGDEBUG( 5, ("ThunkWMMsg16:WM_IMEKEY debug\n"));
#endif
        lpmpex->uParam = MAKELONG( LOBYTE(wParam), HIBYTE(wParam) );
        break;
#endif // FE_IME

    case WM_PRINT:
    case WM_PRINTCLIENT:
        lpmpex->uParam = (WPARAM)HDC32(wParam);
        break;
    case WM_NOTIFY:         // 0x4e
        // wparam is control ID, lparam points to NMHDR or larger struct.
        {
            LONG lParamMap;

            GETVDMPTR(lParam, sizeof(NMHDR), (PSZ)lParamMap);
            *plParamNew = (LONG)AddParamMap(lParamMap, lParam);
            if (lParamMap != *plParamNew) {
                FREEVDMPTR((PSZ)lParamMap);
            }

        }
        break;

     case WM_CHANGEUISTATE:   // 0x127
     case WM_UPDATEUISTATE:   // 0x128
     case WM_QUERYUISTATE:    // 0x129
        // We should only see this message originate from the 32-bit side
        // It will come with both words of uParam used and lPram unused.
        // We 32->16 thunk it (WM32xxxUIState() - wmdisp32.c) by copying
        // uParam32 to lParam16.  Now we are just reversing the process.
        lpmpex->uParam = (UINT)lParam;
        *plParamNew = 0;

        break;


    }  // end switch
    return TRUE;
}

//
// the WM_CREATE message has already been thunked, but this WM_CREATE
// is coming from an MDI client window so lParam->lpCreateParams needs
// special attention
//

BOOL FinishThunkingWMCreateMDI16(LONG lParamNew, LPCLIENTCREATESTRUCT lpCCS)
{
    PCLIENTCREATESTRUCT16  pCCS16;

    GETVDMPTR(((LPCREATESTRUCT)lParamNew)->lpCreateParams,
            sizeof(CLIENTCREATESTRUCT16), pCCS16);

    lpCCS->hWindowMenu = HMENU32(FETCHWORD(pCCS16->hWindowMenu));
    lpCCS->idFirstChild = WORD32(FETCHWORD(pCCS16->idFirstChild));

    ((LPCREATESTRUCT)lParamNew)->lpCreateParams = (LPVOID)lpCCS;

    FREEVDMPTR(pCCS16);

    return TRUE;
}

//
// the WM_CREATE message has already been thunked, but this WM_CREATE
// is coming from an MDI child window so lParam->lpCreateParams needs
// special attention
//

BOOL FinishThunkingWMCreateMDIChild16(LONG lParamNew, LPMDICREATESTRUCT lpMCS)
{
    PMDICREATESTRUCT16  pMCS16;

    GETVDMPTR(((LPCREATESTRUCT)lParamNew)->lpCreateParams,
            sizeof(MDICREATESTRUCT16), pMCS16);

    GETPSZIDPTR(pMCS16->vpszClass, lpMCS->szClass);
    GETPSZPTR(pMCS16->vpszTitle, lpMCS->szTitle);
    lpMCS->hOwner = HMODINST32(FETCHWORD(pMCS16->hOwner));
    lpMCS->x = (int)FETCHWORD(pMCS16->x);
    lpMCS->y = (int)FETCHWORD(pMCS16->y);
    lpMCS->cx = (int)FETCHWORD(pMCS16->cx);
    lpMCS->cy = (int)FETCHWORD(pMCS16->cy);
    lpMCS->style = FETCHDWORD(pMCS16->style);
    lpMCS->lParam = FETCHDWORD(pMCS16->lParam);

    ((LPCREATESTRUCT)lParamNew)->lpCreateParams = (LPVOID)lpMCS;

    FREEVDMPTR(pMCS16);

    return TRUE;
}


VOID FASTCALL UnThunkWMMsg16(LPMSGPARAMEX lpmpex)
{
    switch(lpmpex->Parm16.WndProc.wMsg) {

    case WM_SETTEXT:        // 00Ch, <SLPre,SLPost   >
    case WM_WININICHANGE:   // 01Ah, <SLPre,       LS>
    case WM_DEVMODECHANGE:  // 01Bh, <SLPre,       LS>
        {
            BOOL fFreePtr;
            DeleteParamMap(lpmpex->lParam, PARAM_32, &fFreePtr);
            if (fFreePtr) {
                FREEPSZPTR((LPSZ)lpmpex->lParam);
            }
        }
        break;

    case WM_GETTEXT:        // 00Dh, <SLPre,SLPost,LS>
        if ((WORD)lpmpex->lReturn > 0) {
            FLUSHVDMPTR(lpmpex->Parm16.WndProc.lParam, lpmpex->Parm16.WndProc.wParam, (LPSZ)lpmpex->lParam);
            FREEPSZPTR((LPSZ)lpmpex->lParam);
        }
        break;

    case WM_GETMINMAXINFO:  // 024h, <SLPre,SLPost,LS>,MINMAXINFOSTRUCT
        UnThunkWMGetMinMaxInfo16(lpmpex->Parm16.WndProc.lParam, (LPPOINT)lpmpex->lParam);
        break;

    case WM_DRAWITEM:       // 02Bh  notused, DRAWITEMSTRUCT
        if (lpmpex->lParam) {
            putdrawitem16((VPDRAWITEMSTRUCT16)lpmpex->Parm16.WndProc.lParam, (PDRAWITEMSTRUCT)lpmpex->lParam);
        }
        break;

    case WM_MEASUREITEM:    // 02Ch  notused, MEASUREITEMSTRUCT
        if (lpmpex->lParam) {
            putmeasureitem16((VPMEASUREITEMSTRUCT16)lpmpex->Parm16.WndProc.lParam, (PMEASUREITEMSTRUCT)lpmpex->lParam);
        }
        break;

    case WM_DELETEITEM:     // 02Dh  notused, DELETEITEMSTRUCT
        if (lpmpex->lParam) {
            putdeleteitem16((VPDELETEITEMSTRUCT16)lpmpex->Parm16.WndProc.lParam, (PDELETEITEMSTRUCT)lpmpex->lParam);
        }
        break;

    case WM_GETFONT:        // 031h
        lpmpex->lReturn = GETHFONT16(lpmpex->lReturn);
        break;

    case WM_COMPAREITEM:    // 039h
        if (lpmpex->lParam) {
            putcompareitem16((VPCOMPAREITEMSTRUCT16)lpmpex->Parm16.WndProc.lParam, (PCOMPAREITEMSTRUCT)lpmpex->lParam);
        }
        break;

    case WM_WINHELP:
        if (lpmpex->lParam && lpmpex->lParam != (LONG)lpmpex->MsgBuffer) {
            free_w((PVOID)lpmpex->lParam);
        }
        break;

    case WM_SIZING:     // 214h, <SLPre,SLPost,LS>,RECT
        if (lpmpex->lParam) {
            putrect16((VPRECT16)lpmpex->Parm16.WndProc.lParam, (LPRECT)lpmpex->lParam);
        }
        break;

    case WM_NCCALCSIZE:     // 083h, <SLPre,SLPost,LS>,RECT
        if (lpmpex->lParam) {
            putrect16((VPRECT16)lpmpex->Parm16.WndProc.lParam, (LPRECT)lpmpex->lParam);
            if (lpmpex->Parm16.WndProc.wParam) {
                PNCCALCSIZE_PARAMS16 pnc16;
                PNCCALCSIZE_PARAMS16 lpnc16;
                LPNCCALCSIZE_PARAMS  lpnc;

                lpnc  = (LPNCCALCSIZE_PARAMS)lpmpex->lParam;
                pnc16 = (PNCCALCSIZE_PARAMS16)lpmpex->Parm16.WndProc.lParam;

                putrect16((VPRECT16)(&pnc16->rgrc[1]), &lpnc->rgrc[1]);
                putrect16((VPRECT16)(&pnc16->rgrc[2]), &lpnc->rgrc[2]);

                GETVDMPTR( pnc16, sizeof(NCCALCSIZE_PARAMS16), lpnc16 );

                putwindowpos16( (VPWINDOWPOS16)lpnc16->lppos, lpnc->lppos );

                FREEVDMPTR( lpnc16 );

            }
        }
        break;

    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        if (lpmpex->lParam) {
            putwindowpos16( (VPWINDOWPOS16)lpmpex->Parm16.WndProc.lParam, (LPWINDOWPOS)lpmpex->lParam);
        }
        break;

    case WM_CTLCOLOR:
        // see thunking of wm_ctlcolor.

        if ((ULONG)lpmpex->lReturn > COLOR_ENDCOLORS) {
            lpmpex->lReturn = GETHBRUSH16(lpmpex->lReturn);
        }
        break;

    case WM_MDICREATE:      // 220h, <SLPre,SLPost,LS>,MDICREATESTRUCT
        UnThunkWMMDICreate16(lpmpex->Parm16.WndProc.lParam, (LPMDICREATESTRUCT)lpmpex->lParam);
        lpmpex->lReturn = GETHWND16(lpmpex->lReturn);
        break;

    case WM_MDIGETACTIVE:
        //
        // LOWORD(lReturn) == hwndMDIActive
        // HIWORD(lReturn) == fMaximized
        //

        LOW(lpmpex->lReturn) = GETHWND16((HWND)(lpmpex->lReturn));
        if (lpmpex->lParam != 0) {
            HIW(lpmpex->lReturn) = (WORD)(*((LPBOOL)lpmpex->lParam) != 0);
        }
        break;

    case WM_MDISETMENU:
        lpmpex->lReturn = GETHMENU16(lpmpex->lReturn);
        break;

    case WM_PAINTCLIPBOARD:
    case WM_SIZECLIPBOARD:
        if (lpmpex->lParam) {
            WOWGLOBALFREE((HANDLE)lpmpex->lParam);
        }
        break;

    case WM_ASKCBFORMATNAME:
        /* BUGBUGBUG -- neither thunk or unthunk should be necessary,
           since the system does not process this message in DefWindowProc
           FritzS  */
        if (lpmpex->lParam) {
            putstr16((VPSZ)lpmpex->Parm16.WndProc.lParam, (LPSZ)lpmpex->lParam, lpmpex->Parm16.WndProc.wParam);
            free_w((PBYTE)lpmpex->lParam);
        }
        break;

    case WM_DDE_INITIATE:
        WI32DDEDeleteInitiator((HAND16) lpmpex->Parm16.WndProc.wParam);
        break;

    case WM_NEXTMENU:
        {
            PMDINEXTMENU pT = (PMDINEXTMENU)lpmpex->lParam;
            LOW(lpmpex->lReturn) = GETHMENU16(pT->hmenuNext);
            HIW(lpmpex->lReturn) = GETHWND16(pT->hwndNext);
        }
        break;

    case WM_COPYDATA:
        if (fWhoCalled == WOWDDE_POSTMESSAGE) {
            HWND16 hwnd16 = lpmpex->Parm16.WndProc.hwnd;
            WORD wParam   = lpmpex->Parm16.WndProc.wParam;
            LONG lParamNew = lpmpex->lParam;

            if (((PCOPYDATASTRUCT)lParamNew)->lpData) {
                free_w (((PCOPYDATASTRUCT)lParamNew)->lpData);
                CopyDataDeleteNode (hwnd16, wParam, (DWORD) ((PCOPYDATASTRUCT)lParamNew)->lpData);
            }

            if (lParamNew) {
                free_w ((PVOID)lParamNew);
                CopyDataDeleteNode (hwnd16, wParam, lParamNew);
            }
            else {
                LOGDEBUG (LOG_ALWAYS, ("WOW::WM_COPYDATA16:Unthunking - lpCDS32 is NULL\n"));
            }
        }
        break;

    case WM_QUERYDRAGICON:
        lpmpex->lReturn = (LONG)GETHICON16(lpmpex->lReturn);
        break;

    case WM_QUERYDROPOBJECT:

        //
        // Return value is either TRUE, FALSE,
        // or a cursor!
        //
        if (lpmpex->lReturn && lpmpex->lReturn != (LONG)TRUE) {
            lpmpex->lReturn = (LONG)GETHCURSOR16(lpmpex->lReturn);
        }
        break;

    case WM_NOTIFY:    // 0x4e
        {
            BOOL fFreePtr;
            DeleteParamMap(lpmpex->lParam, PARAM_32, &fFreePtr);
            if (fFreePtr) {
                FREEVDMPTR((PSZ)lpmpex->lParam);
            }
        }
        break;

    case WM_CHANGEUISTATE:   // 0x127
    case WM_UPDATEUISTATE:   // 0x128
    case WM_QUERYUISTATE:    // 0x129
        {
        // See thunking notes for this message in ThunkWMMsg16() above.
        lpmpex->Parm16.WndProc.lParam = (LONG)lpmpex->uParam;
        lpmpex->Parm16.WndProc.wParam = 0;
        }

        break;


#ifdef FE_IME
    case WM_IME_REPORT:
        switch( lpmpex->Parm16.WndProc.wParam ) {
        case IR_STRING:
        case IR_STRINGEX:
        case IR_UNDETERMINE:
            if ( lpmpex->lParam ) {
                GlobalFree((HANDLE)lpmpex->lParam);
            }
            break;
        }
        break;
    // MSKK support WM_IMEKEYDOWN message
    // MSKK support WM_IMEKEYUP message
    // MSKK16bit IME support
    case WM_IMEKEYDOWN:
    case WM_IMEKEYUP:
#ifdef DEBUG
        LOGDEBUG( 5,("UnThunkWMMsg16:WM_IMEKEY debug\n"));
#endif
        break;
#endif // FE_IME

    } // end switch
}


BOOL ThunkWMGetMinMaxInfo16(VPVOID lParam, LPPOINT *plParamNew)
{
    register LPPOINT lppt;
    register PPOINT16 ppt16;

    if (lParam) {

    lppt = *plParamNew;

    GETVDMPTR(lParam, sizeof(POINT16)*5, ppt16);

    lppt[0].x = ppt16[0].x;
    lppt[0].y = ppt16[0].y;
    lppt[1].x = ppt16[1].x;
    lppt[1].y = ppt16[1].y;
    lppt[2].x = ppt16[2].x;
    lppt[2].y = ppt16[2].y;
    lppt[3].x = ppt16[3].x;
    lppt[3].y = ppt16[3].y;
    lppt[4].x = ppt16[4].x;
    lppt[4].y = ppt16[4].y;

    FREEVDMPTR(ppt16);
    }
    RETURN(TRUE);
}


VOID UnThunkWMGetMinMaxInfo16(VPVOID lParam, register LPPOINT lParamNew)
{
    register PPOINT16 ppt16;

    if (lParamNew) {

    GETVDMPTR(lParam, sizeof(POINT16)*5, ppt16);

    ppt16[0].x = (SHORT)lParamNew[0].x;
    ppt16[0].y = (SHORT)lParamNew[0].y;
    ppt16[1].x = (SHORT)lParamNew[1].x;
    ppt16[1].y = (SHORT)lParamNew[1].y;
    ppt16[2].x = (SHORT)lParamNew[2].x;
    ppt16[2].y = (SHORT)lParamNew[2].y;
    ppt16[3].x = (SHORT)lParamNew[3].x;
    ppt16[3].y = (SHORT)lParamNew[3].y;
    ppt16[4].x = (SHORT)lParamNew[4].x;
    ppt16[4].y = (SHORT)lParamNew[4].y;

    FLUSHVDMPTR(lParam, sizeof(POINT16)*5, ppt16);
    FREEVDMPTR(ppt16);

    }
    RETURN(NOTHING);
}

BOOL ThunkWMMDICreate16(VPVOID lParam, LPMDICREATESTRUCT *plParamNew)
{
    register LPMDICREATESTRUCT lpmdicreate;
    register PMDICREATESTRUCT16 pmdicreate16;

    if (lParam) {

    lpmdicreate = *plParamNew;

    GETVDMPTR(lParam, sizeof(MDICREATESTRUCT16), pmdicreate16);

    GETPSZIDPTR( pmdicreate16->vpszClass, lpmdicreate->szClass );
    GETPSZPTR( pmdicreate16->vpszTitle, lpmdicreate->szTitle );

    lpmdicreate->hOwner  = HMODINST32( pmdicreate16->hOwner );
    lpmdicreate->x       = INT32DEFAULT(pmdicreate16->x);
    lpmdicreate->y       = INT32DEFAULT(pmdicreate16->y);
    lpmdicreate->cx      = INT32DEFAULT(pmdicreate16->cx);
    lpmdicreate->cy      = INT32DEFAULT(pmdicreate16->cy);
    lpmdicreate->style   = pmdicreate16->style;
    lpmdicreate->lParam  = pmdicreate16->lParam;


    FREEVDMPTR(pmdicreate16);
    }
    RETURN(TRUE);
}


VOID UnThunkWMMDICreate16(VPVOID lParam, register LPMDICREATESTRUCT lParamNew)
{
    register PMDICREATESTRUCT16 pmdicreate16;

    if (lParamNew) {

    GETVDMPTR(lParam, sizeof(MDICREATESTRUCT16), pmdicreate16);

    pmdicreate16->hOwner = GETHINST16(lParamNew->hOwner);
    pmdicreate16->x      = (SHORT)lParamNew->x;
    pmdicreate16->y      = (SHORT)lParamNew->y;
    pmdicreate16->cx     = (SHORT)lParamNew->cx;
    pmdicreate16->cy     = (SHORT)lParamNew->cy;
    pmdicreate16->style  = lParamNew->style;
    pmdicreate16->lParam = lParamNew->lParam;

    FLUSHVDMPTR(lParam, sizeof(MDICREATESTRUCT16), pmdicreate16);
    FREEVDMPTR(pmdicreate16);

    }
    RETURN(NOTHING);
}



BOOL FASTCALL ThunkSTMsg16(LPMSGPARAMEX lpmpex)
{
    WORD wMsg     = lpmpex->Parm16.WndProc.wMsg;

    LOGDEBUG(9,("    Thunking 16-bit STM window message %s(%04x)\n", (LPSZ)GetWMMsgName(wMsg), wMsg));

    switch(wMsg) {

    case WIN30_STM_SETICON:
        lpmpex->uMsg = STM_SETICON;
        lpmpex->uParam = (UINT) HICON32(lpmpex->Parm16.WndProc.wParam);
        break;

    case WIN30_STM_GETICON:
        lpmpex->uMsg = STM_GETICON;
        break;
    }
    return (TRUE);
}


VOID FASTCALL UnThunkSTMsg16(LPMSGPARAMEX lpmpex)
{
    WORD wMsg     = lpmpex->Parm16.WndProc.wMsg;

    LOGDEBUG(9,("    UnThunking 16-bit STM window message %s(%04x)\n", (LPSZ)GetWMMsgName(wMsg), wMsg));

    switch(wMsg) {

    case WIN30_STM_GETICON:
    case WIN30_STM_SETICON:
        lpmpex->lReturn = GETHICON16(lpmpex->lReturn);
        break;
    }
}


BOOL FASTCALL ThunkMNMsg16(LPMSGPARAMEX lpmpex)
{
    WORD wMsg     = lpmpex->Parm16.WndProc.wMsg;

    LOGDEBUG(9,("    Thunking 16-bit MN_ window message %s(%04x)\n", (LPSZ)GetWMMsgName(wMsg), wMsg));

    switch(wMsg) {

    case WIN30_MN_GETHMENU:
        lpmpex->uMsg = MN_GETHMENU;
        break;

    case WIN30_MN_FINDMENUWINDOWFROMPOINT:
        lpmpex->uMsg = MN_FINDMENUWINDOWFROMPOINT;
        lpmpex->uParam = (UINT)lpmpex->MsgBuffer; // enough room for UINT
        *(PUINT)lpmpex->uParam = 0;
        break;

    }
    return (TRUE);
}


VOID FASTCALL UnThunkMNMsg16(LPMSGPARAMEX lpmpex)
{
    WORD wMsg     = lpmpex->Parm16.WndProc.wMsg;

    LOGDEBUG(9,("    UnThunking 16-bit MN_ window message %s(%04x)\n", (LPSZ)GetWMMsgName(wMsg), wMsg));

    switch(wMsg) {

    case WIN30_MN_FINDMENUWINDOWFROMPOINT:
        if (lpmpex->uParam) {
            lpmpex->lReturn = MAKELONG((HWND16)lpmpex->lReturn,
                                              LOWORD(*(PUINT)lpmpex->uParam));
        }
        break;
    }
}
