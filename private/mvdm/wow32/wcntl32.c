/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WMSG32.C
 *  WOW32 32-bit message thunks
 *
 *  History:
 *  Created 19-Feb-1992 by Chandan Chauhan (ChandanC)
--*/


#include "precomp.h"
#pragma hdrstop

MODNAME(wcntl32.c);

// This function thunks the button control messages,
//
//  BM_GETCHECK
//  BM_SETCHECK
//  BM_GETSTATE
//  BM_SETSTATE
//  BM_SETSTYLE
//

BOOL FASTCALL WM32BMControl(LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wMsg = (WORD) (WM_USER + (lpwm32mpex->uMsg - BM_GETCHECK));
    }

    return (TRUE);
}


BOOL FASTCALL WM32BMClick (LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wMsg = (WORD) (WIN31_BM_CLICK);
    }

    return (TRUE);
}



// This function thunks the following edit control messages,
//
//  EM_GETSEL
//  EM_GETMODIFY
//  EM_SETMODIFY
//  EM_GETLINECOUNT
//  EM_GETLINEINDEX
//  EM_LINELENGTH
//  EM_LIMITTEX
//  EM_CANUNDO
//  EM_UNDO
//  EM_FMTLINES
//  EM_LINEFROMCHAR
//  EM_SETPASSWORDCHAR
//  EM_EMPTYUNDOBUFFER

BOOL FASTCALL WM32EMControl(LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wMsg = (WORD) (WM_USER + (lpwm32mpex->uMsg - EM_GETSEL));
    }

    return (TRUE);
}


// This function thunks the button control messages,
//
//  EM_SETSEL
//

BOOL FASTCALL WM32EMSetSel (LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wMsg = (WORD) (WM_USER + (lpwm32mpex->uMsg - EM_GETSEL));
        LOW(lpwm32mpex->Parm16.WndProc.lParam) = (WORD) lpwm32mpex->uParam;
        HIW(lpwm32mpex->Parm16.WndProc.lParam) =
                    (lpwm32mpex->lParam != -1) ? lpwm32mpex->lParam :  32767;
    }

    return (TRUE);
}


// This function thunks the edit control messages,
//
//  EM_GETRECT
//

BOOL FASTCALL WM32EMGetRect (LPWM32MSGPARAMEX lpwm32mpex)
{
    if ( lpwm32mpex->fThunk ) {
        lpwm32mpex->Parm16.WndProc.wMsg = (WORD) (WM_USER + (lpwm32mpex->uMsg - EM_GETSEL));
        lpwm32mpex->Parm16.WndProc.lParam = malloc16(sizeof(RECT16));
        if (!(lpwm32mpex->Parm16.WndProc.lParam))
            return FALSE;
    } else {
        GETRECT16( lpwm32mpex->Parm16.WndProc.lParam, (LPRECT)lpwm32mpex->lParam );
        if (lpwm32mpex->Parm16.WndProc.lParam)
            free16((VPVOID) lpwm32mpex->Parm16.WndProc.lParam);
    }

    return (TRUE);
}


// This function thunks the edit control messages,
//
//  EM_SETRECT
//  EM_SETRECTNP
//

BOOL FASTCALL WM32EMSetRect (LPWM32MSGPARAMEX lpwm32mpex)
{

    if ( lpwm32mpex->fThunk ) {
        lpwm32mpex->Parm16.WndProc.wMsg = (WORD) (WM_USER + (lpwm32mpex->uMsg - EM_GETSEL));
        lpwm32mpex->Parm16.WndProc.lParam = malloc16(sizeof(RECT16));
        if (!(lpwm32mpex->Parm16.WndProc.lParam))
            return FALSE;
        PUTRECT16( lpwm32mpex->Parm16.WndProc.lParam, (LPRECT)lpwm32mpex->lParam );
    } else {
        if (lpwm32mpex->Parm16.WndProc.lParam)
            free16((VPVOID) lpwm32mpex->Parm16.WndProc.lParam);
    }

    return (TRUE);
}


// This function thunks the edit control messages,
//
//  EM_LINESCROLL
//

BOOL FASTCALL WM32EMLineScroll (LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wMsg = (WORD) (WM_USER + (lpwm32mpex->uMsg - EM_GETSEL));
        LOW(lpwm32mpex->Parm16.WndProc.lParam) = (WORD) lpwm32mpex->lParam;
        HIW(lpwm32mpex->Parm16.WndProc.lParam) = (WORD) lpwm32mpex->uParam;
    }

    return (TRUE);
}


// This function thunks the edit control messages,
//
//  EM_REPLACESEL
//

BOOL FASTCALL WM32EMReplaceSel (LPWM32MSGPARAMEX lpwm32mpex)
{

    if ( lpwm32mpex->fThunk ) {
        lpwm32mpex->Parm16.WndProc.wMsg = (WORD) (WM_USER + (lpwm32mpex->uMsg - EM_GETSEL));
        if (lpwm32mpex->lParam) {
            INT cb;

            cb = strlen((LPSZ)lpwm32mpex->lParam)+1;
            lpwm32mpex->dwTmp[0] = (DWORD)cb; // save allocation size

            // winworks2.0a requires DS based string pointers for this message

            if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_DSBASEDSTRINGPOINTERS) {

                // be sure allocation size matches stackfree16() size below
                lpwm32mpex->Parm16.WndProc.lParam = stackalloc16(cb);

            } else {
                lpwm32mpex->Parm16.WndProc.lParam = malloc16(cb);
            }

            if (!(lpwm32mpex->Parm16.WndProc.lParam))
                return FALSE;
            putstr16((VPSZ)lpwm32mpex->Parm16.WndProc.lParam, (LPSZ)lpwm32mpex->lParam, cb);
        }
    } else {
        if (lpwm32mpex->Parm16.WndProc.lParam) {
            if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_DSBASEDSTRINGPOINTERS) {

                stackfree16((VPVOID) lpwm32mpex->Parm16.WndProc.lParam,
                            ((UINT)lpwm32mpex->dwTmp[0]));
            } else {
                free16((VPVOID) lpwm32mpex->Parm16.WndProc.lParam);
            }
        }
    }

    return (TRUE);
}


BOOL FASTCALL WM32EMSetFont (LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wMsg = (WORD) (WM_USER + (lpwm32mpex->uMsg - EM_GETSEL));
    }

    LOGDEBUG(0,(" Window %08lX is receiving Control Message %s(%08x)\n", lpwm32mpex->hwnd, (LPSZ)GetWMMsgName(lpwm32mpex->uMsg), lpwm32mpex->uMsg));
    return (TRUE);
}


// This function thunks the edit control messages,
//
//  EM_GETLINE
//

BOOL FASTCALL WM32EMGetLine (LPWM32MSGPARAMEX lpwm32mpex)
{

    if ( lpwm32mpex->fThunk ) {
        lpwm32mpex->Parm16.WndProc.wMsg = (WORD) (WM_USER + (lpwm32mpex->uMsg - EM_GETSEL));
        if (lpwm32mpex->lParam) {
            INT cb;
            PBYTE lp;

            // the first WORD is what USER uses.

            cb = *(UNALIGNED WORD *)(lpwm32mpex->lParam);
            lpwm32mpex->Parm16.WndProc.lParam = malloc16(cb);
            if (!(lpwm32mpex->Parm16.WndProc.lParam))
                return FALSE;
            ALLOCVDMPTR(lpwm32mpex->Parm16.WndProc.lParam,2,lp);
            *((UNALIGNED WORD *)lp) = (WORD)cb;
            FLUSHVDMPTR(lpwm32mpex->Parm16.WndProc.lParam,2,lp);  /* first 2 bytes modified */
        }
    } else {
        if (lpwm32mpex->Parm16.WndProc.lParam) {
            PBYTE lp;

            GETMISCPTR(lpwm32mpex->Parm16.WndProc.lParam,lp);
            RtlCopyMemory((PBYTE)lpwm32mpex->lParam,lp,lpwm32mpex->lReturn);
            FREEVDMPTR(lp);
            free16((VPVOID) lpwm32mpex->Parm16.WndProc.lParam);
        }
    }

    LOGDEBUG(3,(" Window %08lX is receiving Control Message %s(%08x)\n", lpwm32mpex->hwnd, (LPSZ)GetWMMsgName(lpwm32mpex->uMsg), lpwm32mpex->uMsg));

    return (TRUE);
}


BOOL FASTCALL WM32EMSetWordBreakProc (LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wMsg = (WORD) (WM_USER + (lpwm32mpex->uMsg - EM_GETSEL));

        // take out the marker bits and fix the RPL bits
        UnMarkWOWProc (lpwm32mpex->lParam,lpwm32mpex->Parm16.WndProc.lParam);

        LOGDEBUG(3,(" Window %08lX is receiving Control Message %s(%08x)\n", lpwm32mpex->hwnd, (LPSZ)GetWMMsgName(lpwm32mpex->uMsg), lpwm32mpex->uMsg));
    }

    return (TRUE);
}


BOOL FASTCALL WM32EMGetWordBreakProc (LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wMsg = (WORD) (WM_USER + (lpwm32mpex->uMsg - EM_GETSEL));
        LOGDEBUG(3,(" Window %08lX is receiving Control Message %s(%08x)\n", lpwm32mpex->hwnd, (LPSZ)GetWMMsgName(lpwm32mpex->uMsg), lpwm32mpex->uMsg));
    }
    else {

        // Mark the address as WOW Proc and store the high bits in the RPL field
        MarkWOWProc (lpwm32mpex->lReturn,lpwm32mpex->lReturn);
    }


    return (TRUE);
}


// This function thunks the edit control messages,
//
//  EM_SETTABSTOPS
//

BOOL FASTCALL WM32EMSetTabStops (LPWM32MSGPARAMEX lpwm32mpex)
{

    if ( lpwm32mpex->fThunk ) {
        lpwm32mpex->Parm16.WndProc.wMsg = (WORD) (WM_USER + (lpwm32mpex->uMsg - EM_GETSEL));
        if (lpwm32mpex->uParam != 0) {
            lpwm32mpex->Parm16.WndProc.lParam = malloc16(lpwm32mpex->uParam * sizeof(WORD));
            if (!(lpwm32mpex->Parm16.WndProc.lParam))
                return FALSE;
            putintarray16((VPINT16)lpwm32mpex->Parm16.WndProc.lParam, (INT)lpwm32mpex->uParam, (LPINT)lpwm32mpex->lParam);
        }
    } else {
        if (lpwm32mpex->Parm16.WndProc.lParam)
            free16((VPVOID) lpwm32mpex->Parm16.WndProc.lParam);

    }

    return (TRUE);
}


// This function thunks the following combo box control messages,
//
//  CB_GETEDITSEL
//  CB_LIMITTEXT
//  CB_SETEDITSEL
//  CB_DELETESTRING
//  CB_GETCOUNT
//  CB_GETCURSEL
//  CB_GETLBTEXTLEN
//  CB_SETCURSEL
//  CB_SHOWDROPDOWN
//  CB_GETITEMDATA
//  CB_SETITEMDATA


BOOL FASTCALL  WM32CBControl (LPWM32MSGPARAMEX lpwm32mpex)
{


    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wMsg = (WORD) (WM_USER + (lpwm32mpex->uMsg - CB_GETEDITSEL));
    }

    return (TRUE);
}


// This function thunks the following combo box control messages,
//
//  CB_ADDSTRING
//  CB_INSERTSTRING
//  CB_FINDSTRING
//  CB_SELECTSTRING

BOOL FASTCALL  WM32CBAddString (LPWM32MSGPARAMEX lpwm32mpex)
{
    PWW pww;




    if ( lpwm32mpex->fThunk ) {
        if (!(pww = lpwm32mpex->pww)) {
            if (pww = FindPWW (lpwm32mpex->hwnd))
                lpwm32mpex->pww = pww;
            else
                return FALSE;
        }

        lpwm32mpex->Parm16.WndProc.wMsg = (WORD) (WM_USER + (lpwm32mpex->uMsg - CB_GETEDITSEL));

        //
        // Determine if this combobox has string pointers or handles passed
        // in with CB_ADDSTRING messages.  Normal comboboxes have string
        // pointers passed.  Owner-draw comboboxes that don't have the
        // CBS_HASSTRINGS style bit set have handles passed in.  These handles
        // are simply passed back to the owner at paint time.  If the
        // CBS_HASSTRINGS style bit is set, strings are used instead of
        // handles as the "cookie" which is passed back to the application
        // at paint time.
        //
        // We treat lpwm32mpex->dwParam as a BOOL indicating this combobox
        // takes handles instead of strings.
        //

        lpwm32mpex->dwParam =
            (pww->style & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)) &&
            !(pww->style & CBS_HASSTRINGS);

        if ( !lpwm32mpex->dwParam ) {        // if strings are used
            if (lpwm32mpex->lParam) {
                INT cb;

                cb = strlen((LPSZ)lpwm32mpex->lParam)+1;
                lpwm32mpex->Parm16.WndProc.lParam = malloc16(cb);
                if (!(lpwm32mpex->Parm16.WndProc.lParam))
                    return FALSE;
                putstr16((VPSZ)lpwm32mpex->Parm16.WndProc.lParam, (LPSZ)lpwm32mpex->lParam, cb);
            }
        }
    } else {
        if ( !lpwm32mpex->dwParam ) {        // if strings are used
            if (lpwm32mpex->Parm16.WndProc.lParam) {
                getstr16((VPSZ)lpwm32mpex->Parm16.WndProc.lParam, (LPSZ)lpwm32mpex->lParam, -1);
                free16((VPVOID) lpwm32mpex->Parm16.WndProc.lParam);
            }
        }
    }

    LOGDEBUG(3,(" Window %08lX is receiving Control Message %s(%08x)\n", lpwm32mpex->hwnd, (LPSZ)GetWMMsgName(lpwm32mpex->uMsg), lpwm32mpex->uMsg));

    return(TRUE);

}


// This function thunks the following combo box control messages,
//
//  CB_DIR
//
//  Code in this routine references code in wparam.c in order to circumvent
//  copying memory to 16-bit memory space.
//  GetParam16 verifies that the parameter we get (lparam) had not originated
//  in 16-bit code. If it did come from 16-bit code, then we send an original
//  16:16 pointer to the application.
//  This fixes PagePlus 3.0 application and (if implemented on a broader scale)
//  will positively affect performance of applications which send a lot of
//  standard messages and use subclassing a lot.
//  -- VadimB

BOOL FASTCALL  WM32CBDir (LPWM32MSGPARAMEX lpwm32mpex)
{
    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wMsg = (WORD) (WM_USER + (lpwm32mpex->uMsg - CB_GETEDITSEL));
        if (lpwm32mpex->lParam) {
            INT cb;

            if (W32CheckThunkParamFlag()) {
                LONG lParam = (LONG)GetParam16(lpwm32mpex->lParam);
                if (lParam) {
                    lpwm32mpex->Parm16.WndProc.lParam = lParam;
                    return (TRUE);
                }
            }

            cb = strlen((LPSZ)lpwm32mpex->lParam)+1;
            lpwm32mpex->Parm16.WndProc.lParam = malloc16(cb);
            if (!(lpwm32mpex->Parm16.WndProc.lParam))
                return FALSE;
            putstr16((VPSZ)lpwm32mpex->Parm16.WndProc.lParam, (LPSZ)lpwm32mpex->lParam, cb);
        }
    } else {
        if (W32CheckThunkParamFlag()) {
            if (DeleteParamMap(lpwm32mpex->Parm16.WndProc.lParam, PARAM_16, NULL)) {
                return TRUE;
            }
        }
        if (lpwm32mpex->Parm16.WndProc.lParam)
            free16((VPVOID) lpwm32mpex->Parm16.WndProc.lParam);
    }

    return(TRUE);
}


// This function thunks the following combo box control messages,
//
//  CB_GETLBTEXT

BOOL FASTCALL WM32CBGetLBText (LPWM32MSGPARAMEX lpwm32mpex)
{
    PWW   pww;



    if ( lpwm32mpex->fThunk ) {
        INT cb;

        if (!(pww = lpwm32mpex->pww)) {
            if (pww = FindPWW (lpwm32mpex->hwnd))
                lpwm32mpex->pww = pww;
            else
                return FALSE;
        }

        lpwm32mpex->Parm16.WndProc.wMsg = (WORD) (WM_USER + (lpwm32mpex->uMsg - CB_GETEDITSEL));

        //
        // Determine if this combobox has string pointers or handles passed
        // in with CB_ADDSTRING messages.  Normal comboboxes have string
        // pointers passed.  Owner-draw comboboxes that don't have the
        // CBS_HASSTRINGS style bit set have handles passed in.  These handles
        // are simply passed back to the owner at paint time.  If the
        // CBS_HASSTRINGS style bit is set, strings are used instead of
        // handles as the "cookie" which is passed back to the application
        // at paint time.
        //
        // We treat lpwm32mpex->dwParam as a BOOL indicating this combobox
        // takes handles instead of strings.
        //

        lpwm32mpex->dwParam =
            (pww->style & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)) &&
            !(pww->style & CBS_HASSTRINGS);

        //
        // Determine the size of the buffer to allocate on the 16-bit side
        // to receive the text.
        //

        if (lpwm32mpex->dwParam) {           // if handles are used
            cb = 4;
        } else {
            cb = SendMessage(lpwm32mpex->hwnd, CB_GETLBTEXTLEN, lpwm32mpex->uParam, 0);
            if (cb == CB_ERR) {
                //
                // lpwm32mpex->dwTmp[0] is initialized to 0 so that nothing
                // gets copied to the buffer by getstr16() while unthunking
                // this message.
                //
                // bug # 24415, ChandanC
                //

                cb = SIZE_BOGUS;
                lpwm32mpex->dwTmp[0] = 0;
            }
            else {
                //
                // Add one for NULL character.
                //
                cb = cb + 1;
                (INT) lpwm32mpex->dwTmp[0] = (INT) -1;
            }
        }
        if (lpwm32mpex->lParam) {
            BYTE *lpT;

            // See comment on similar code below

            lpwm32mpex->Parm16.WndProc.lParam = malloc16(cb);
            if (!(lpwm32mpex->Parm16.WndProc.lParam))
                return FALSE;
            GETVDMPTR((lpwm32mpex->Parm16.WndProc.lParam), sizeof(BYTE), lpT);
            *lpT = 0;
            FREEVDMPTR(lpT);
        }
    }
    else {
        if (lpwm32mpex->lParam && lpwm32mpex->Parm16.WndProc.lParam) {
            if (lpwm32mpex->dwParam) {       // if handles are used
                UNALIGNED DWORD *lpT;
                GETVDMPTR((lpwm32mpex->Parm16.WndProc.lParam), sizeof(DWORD), lpT);
                *(UNALIGNED DWORD *)lpwm32mpex->lParam = *lpT;
                FREEVDMPTR(lpT);
            }
            else {
                getstr16((VPSZ)lpwm32mpex->Parm16.WndProc.lParam, (LPSZ)lpwm32mpex->lParam,
                         (INT) lpwm32mpex->dwTmp[0]);
            }

            free16((VPVOID) lpwm32mpex->Parm16.WndProc.lParam);

        }
    }

    return(TRUE);
}


// This function thunks the following combo box control messages,
//
//  CB_GETDROPPEDCONTROLRECT

BOOL FASTCALL WM32CBGetDropDownControlRect (LPWM32MSGPARAMEX lpwm32mpex)
{

    if ( lpwm32mpex->fThunk ) {
        lpwm32mpex->Parm16.WndProc.wMsg = (WORD) (WM_USER + (lpwm32mpex->uMsg - CB_GETEDITSEL));
        lpwm32mpex->Parm16.WndProc.lParam = malloc16(sizeof(RECT16));
        if (!(lpwm32mpex->Parm16.WndProc.lParam))
            return FALSE;
    } else {
        GETRECT16( lpwm32mpex->Parm16.WndProc.lParam, (LPRECT)lpwm32mpex->lParam );
        if (lpwm32mpex->Parm16.WndProc.lParam)
            free16((VPVOID) lpwm32mpex->Parm16.WndProc.lParam);
    }

    return(TRUE);
}


// This function thunks the following combo box control messages,
//
//  CBEC_SETCOMBOFOCUS           (WM_USER+CB_MSGMAX+1)
//  CBEC_KILLCOMBOFOCUS          (WM_USER+CB_MSGMAX+2)
// These undocumented messages are used by Excel 5.0
//

BOOL FASTCALL  WM32CBComboFocus (LPWM32MSGPARAMEX lpwm32mpex)
{


    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wMsg =
                (WORD)((lpwm32mpex->uMsg-CBEC_SETCOMBOFOCUS) + OLDCBEC_SETCOMBOFOCUS);
    }

    return (TRUE);
}


// This function thunks the list box control messages
//
//  LB_RESETCONTENT
//  LB_SETCURSEL
//  LB_GETSEL
//  LB_GETCURSEL
//  LB_GETTEXTLEN
//  LB_GETCOUNT
//  LB_GETCARETINDEX
//  LB_GETTOPINDEX
//  LB_GETSELCOUNT
//  LB_GETHORIZONTALEXTENT
//  LB_SETHORIZONTALEXTENT
//  LB_SETCOLUMNWIDTH
//  LB_SETTOPINDEX
//  LB_SETCARETINDEX
//  LB_SETITEMDATA
//  LB_SELITEMRANGE
//  LB_SETITEMHEIGHT
//  LB_GETITEMHEIGHT
//  LB_DELETESTRING
//

BOOL FASTCALL  WM32LBControl (LPWM32MSGPARAMEX lpwm32mpex)
{


    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wMsg = (WORD) (WM_USER + (lpwm32mpex->uMsg - LB_ADDSTRING + 1));
    }

    return (TRUE);
}

// This function thunks the list box control messages
//
//  LB_GETTEXT

BOOL FASTCALL WM32LBGetText (LPWM32MSGPARAMEX lpwm32mpex)
{
    PWW   pww;



    if ( lpwm32mpex->fThunk ) {
        INT cb;

        if (!(pww = lpwm32mpex->pww)) {
            if (pww = FindPWW (lpwm32mpex->hwnd))
                lpwm32mpex->pww = pww;
            else
                return FALSE;
        }

        lpwm32mpex->Parm16.WndProc.wMsg = (WORD) (WM_USER + (lpwm32mpex->uMsg - LB_ADDSTRING + 1));

        //
        // Determine if this listbox has string pointers or handles passed
        // in with LB_ADDSTRING messages.  Owner-draw listboxes that don't
        // have the LBS_HASSTRINGS style bit set have handles passed in.
        // These handles are simply passed back to the owner at paint time.
        // If the LBS_HASSTRINGS style bit is set, strings are used instead of
        // handles as the "cookie" which is passed back to the application
        // at paint time.
        //
        // We treat lpwm32mpex->dwParam as a BOOL indicating this listbox
        // takes handles instead of strings.
        //

        lpwm32mpex->dwParam =
            (pww->style & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)) &&
            !(pww->style & LBS_HASSTRINGS);

        if (lpwm32mpex->dwParam) {    // if this listbox takes handles
            cb = 4;
        }
        else {
            cb = SendMessage(lpwm32mpex->hwnd, LB_GETTEXTLEN, lpwm32mpex->uParam, 0);

            // Check for LB_ERR (which is -1) on the above SendMessage().
            // When cb is equal to LB_ERR make the size as SIZE_BOGUS (256 bytes),
            // and allocate a buffer just in case if the app diddles the lParam.
            // We will free the buffer while unthunking the message (LB_GETTEXT).
            // This fix makes the app MCAD happy.
            // ChandanC 4-21-93.

            if (cb == LB_ERR) {
                cb = SIZE_BOGUS;
            }
            else {
                //
                // Add one for NULL character.
                //
                cb = cb + 1;
            }

        }

        if (lpwm32mpex->lParam) {
            BYTE *lpT;

            lpwm32mpex->Parm16.WndProc.lParam = malloc16(cb);
            if (!(lpwm32mpex->Parm16.WndProc.lParam))
                return FALSE;

            // The reason for this code to be here is that sometimes thunks
            // are executed on a buffer that has not been initialized, e.g.
            // if the hooks are installed by a wow app. That means we will
            // alloc 16-bit buffer while thunking (boils down to uninitialized
            // data buffer and will try to copy the buffer back while unthunking
            // overwriting the stack sometimes (as user allocates temp bufs from
            // the stack). This code initializes data so problem is avoided
            // App: Grammatik/Windows v6.0 -- VadimB

            GETVDMPTR((lpwm32mpex->Parm16.WndProc.lParam), sizeof(BYTE), lpT);
            *lpT = 0;
            FREEVDMPTR(lpT);
        }
    }
    else {

        if ((lpwm32mpex->lReturn != LB_ERR) && lpwm32mpex->lParam && lpwm32mpex->Parm16.WndProc.lParam) {
            if (lpwm32mpex->dwParam) {   // if this listbox takes handles
                UNALIGNED DWORD *lpT;
                GETVDMPTR((lpwm32mpex->Parm16.WndProc.lParam), sizeof(DWORD), lpT);
                *(UNALIGNED DWORD *)lpwm32mpex->lParam = *lpT;
                FREEVDMPTR(lpT);
            }
            else {
                getstr16((VPSZ)lpwm32mpex->Parm16.WndProc.lParam, (LPSZ)lpwm32mpex->lParam, -1);
            }
        }

        if (lpwm32mpex->Parm16.WndProc.lParam) {
            free16((VPVOID) lpwm32mpex->Parm16.WndProc.lParam);
        }
    }

    return(TRUE);
}

// This function thunks the list box control messages
//
//  LB_GETTEXTLEN

BOOL FASTCALL  WM32LBGetTextLen (LPWM32MSGPARAMEX lpwm32mpex)
{


    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wMsg = (WORD) (WM_USER + (lpwm32mpex->uMsg - LB_ADDSTRING + 1));

        // USER32 and so do we send the LB_GETTEXTLEN message whenever an
        // LB_GETTEXT message is sent. This LB_GETTEXTLEN message is an
        // additional message that an app normally wouldn't see in WIN31.
        // lParam by definition is NULL.
        //
        // Super Project dies (at times) when it receives the LB_GETTEXTLEN
        // message. It doesn't expect to see this message and as a result does
        // strlen(lParam) and dies.
        //                                               - nanduri

        if (CURRENTPTD()->dwWOWCompatFlags &  WOWCF_LB_NONNULLLPARAM) {

            // be sure allocation size matches stackfree16() size below
            LPBYTE lpT = (LPBYTE)stackalloc16(0x2);  // just an even number

            lpwm32mpex->Parm16.WndProc.lParam = (LONG)lpT;
            GETVDMPTR(lpT, 0x2, lpT);
            *lpT = '\0';
        }
    } else {
        if (CURRENTPTD()->dwWOWCompatFlags &  WOWCF_LB_NONNULLLPARAM) {
            if(lpwm32mpex->Parm16.WndProc.lParam) {
                stackfree16((VPVOID)lpwm32mpex->Parm16.WndProc.lParam, 0x2);
            }
        }
    }

    return (TRUE);
}



// This function thunks the list box control messages
//
//  LB_DIR

BOOL FASTCALL WM32LBDir (LPWM32MSGPARAMEX lpwm32mpex)
{
    INT cb;
    VPVOID vp;


    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wMsg = (WORD) (WM_USER + (lpwm32mpex->uMsg - LB_ADDSTRING + 1));
        if (lpwm32mpex->lParam) {
            cb = strlen((LPSTR)lpwm32mpex->lParam)+1;
            if (!(vp = malloc16(cb))) {
                LOGDEBUG(0,(" WOW32.DLL : WM32LBDir() :: Could not allocate memory for string, ChandanC\n"));
                WOW32ASSERT(vp);
                return FALSE;
            }
            putstr16(vp, (LPSTR) lpwm32mpex->lParam, cb);
            lpwm32mpex->Parm16.WndProc.lParam = vp;
        }
    }
    else {
        if (lpwm32mpex->Parm16.WndProc.lParam) {
            free16((VPVOID) lpwm32mpex->Parm16.WndProc.lParam);
        }
    }

    return(TRUE);
}

// This function thunks the list box control messages
//
//  LB_GETSELITEMS

BOOL FASTCALL WM32LBGetSelItems (LPWM32MSGPARAMEX lpwm32mpex)
{

    if ( lpwm32mpex->fThunk ) {
        lpwm32mpex->Parm16.WndProc.wMsg = (WORD) (WM_USER + (lpwm32mpex->uMsg - LB_ADDSTRING + 1));
        if (lpwm32mpex->lParam) {
            INT cb;

            cb = lpwm32mpex->uParam * sizeof(WORD);
            lpwm32mpex->Parm16.WndProc.lParam = malloc16(cb);
            if (!(lpwm32mpex->Parm16.WndProc.lParam))
                return FALSE;

        }
    } else {
        getintarray16((VPRECT16)lpwm32mpex->Parm16.WndProc.lParam, (INT)lpwm32mpex->uParam, (LPINT)lpwm32mpex->lParam);
        if (lpwm32mpex->Parm16.WndProc.lParam)
            free16((VPVOID) lpwm32mpex->Parm16.WndProc.lParam);
    }

    return(TRUE);
}


// This function thunks the list box control messages
//
//  LB_SETTABSTOPS

BOOL FASTCALL WM32LBSetTabStops (LPWM32MSGPARAMEX lpwm32mpex)
{

    if ( lpwm32mpex->fThunk ) {
        lpwm32mpex->Parm16.WndProc.wMsg = (WORD) (WM_USER + (lpwm32mpex->uMsg - LB_ADDSTRING + 1));
        if (lpwm32mpex->uParam != 0) {
            lpwm32mpex->Parm16.WndProc.lParam = malloc16(lpwm32mpex->uParam * sizeof(WORD));
            if (!(lpwm32mpex->Parm16.WndProc.lParam))
                return FALSE;
            putintarray16((VPRECT16)lpwm32mpex->Parm16.WndProc.lParam, (INT)lpwm32mpex->uParam, (LPINT)lpwm32mpex->lParam);
        }
    } else {
        if (lpwm32mpex->Parm16.WndProc.lParam)
            free16((VPVOID) lpwm32mpex->Parm16.WndProc.lParam);
    }

    return(TRUE);
}

// This function thunks the list box control messages
//
//  LB_GETITEMRECT

BOOL FASTCALL WM32LBGetItemRect (LPWM32MSGPARAMEX lpwm32mpex)
{

    if ( lpwm32mpex->fThunk ) {
        lpwm32mpex->Parm16.WndProc.wMsg = (WORD) (WM_USER + (lpwm32mpex->uMsg - LB_ADDSTRING + 1));
        lpwm32mpex->Parm16.WndProc.lParam = malloc16(sizeof(RECT16));
        if (!(lpwm32mpex->Parm16.WndProc.lParam))
            return FALSE;
    } else {
        GETRECT16( lpwm32mpex->Parm16.WndProc.lParam, (LPRECT)lpwm32mpex->lParam );
        if (lpwm32mpex->Parm16.WndProc.lParam)
            free16((VPVOID) lpwm32mpex->Parm16.WndProc.lParam);
    }

    LOGDEBUG(3,(" Window %08lX is receiving Control Message %s(%08x)\n", lpwm32mpex->hwnd, (LPSZ)GetWMMsgName(lpwm32mpex->uMsg), lpwm32mpex->uMsg));

    return(TRUE);

}


// This function thunks the list box control messages
//
//  LB_ADDSTRING
//  LB_INSERTSTRING
//  LB_FINDSTRING
//  LB_SELECTSTRING

BOOL FASTCALL WM32LBAddString (LPWM32MSGPARAMEX lpwm32mpex)
{
    PWW   pww;


    if ( lpwm32mpex->fThunk ) {
        if (!(pww = lpwm32mpex->pww)) {
            if (pww = FindPWW (lpwm32mpex->hwnd))
                lpwm32mpex->pww = pww;
            else
                return FALSE;
        }

        lpwm32mpex->Parm16.WndProc.wMsg = (WORD) (WM_USER + (lpwm32mpex->uMsg - LB_ADDSTRING + 1));

        //
        // Determine if this listbox has string pointers or handles passed
        // in with LB_ADDSTRING messages.  Owner-draw listboxes that don't
        // have the LBS_HASSTRINGS style bit set have handles passed in.
        // These handles are simply passed back to the owner at paint time.
        // If the LBS_HASSTRINGS style bit is set, strings are used instead of
        // handles as the "cookie" which is passed back to the application
        // at paint time.
        //
        // We treat lpwm32mpex->dwParam as a BOOL indicating this listbox
        // takes handles instead of strings.
        //

        lpwm32mpex->dwParam =
            (pww->style & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)) &&
            !(pww->style & LBS_HASSTRINGS);

        if ( !lpwm32mpex->dwParam ) {   // if this listbox takes strings
            if (lpwm32mpex->lParam) {
                INT cb;

                cb = strlen((LPSZ)lpwm32mpex->lParam)+1;
                lpwm32mpex->Parm16.WndProc.lParam = malloc16(cb);
                if (!(lpwm32mpex->Parm16.WndProc.lParam))
                    return FALSE;
                putstr16((VPSZ)lpwm32mpex->Parm16.WndProc.lParam, (LPSZ)lpwm32mpex->lParam, cb);
            }
        }
    } else {
        if ( !lpwm32mpex->dwParam ) {   // if this listbox takes strings
            if (lpwm32mpex->Parm16.WndProc.lParam) {
                getstr16((VPSZ)lpwm32mpex->Parm16.WndProc.lParam, (LPSZ)lpwm32mpex->lParam, -1);
                free16((VPVOID) lpwm32mpex->Parm16.WndProc.lParam);
            }
        }
    }

    return(TRUE);
}

// This function thunks the scrollbar control messages,
//
//  SBM_SETPOS
//  SBM_GETPOS
//  SBM_ENABLE_ARROWS
//

BOOL FASTCALL WM32SBMControl (LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wMsg = WM_USER + (lpwm32mpex->uMsg - SBM_SETPOS);
    }

    return (TRUE);
}


//  SBM_GETRANGE

BOOL FASTCALL WM32SBMGetRange (LPWM32MSGPARAMEX lpwm32mpex)
{
    //
    // Changed semantics for this message to support 32-bit
    // scroll bar ranges (vs. 16-bit).
    //
    // Win16:
    //   posMin = LOWORD(SendMessage(hwnd, SBM_GETRANGE, 0, 0));
    //   posMax = HIWORD(SendMessage(hwnd, SBM_GETRANGE, 0, 0));
    //
    // Win32:
    //   SendMessage(hwnd, SBM_GETRANGE,
    //               (WPARAM) &posMin, (LPARAM) &posMax);
    //

    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wMsg = OLDSBM_GETRANGE;
    } else {
        *(DWORD *)lpwm32mpex->uParam = INT32(LOWORD(lpwm32mpex->lReturn));
        *(DWORD *)lpwm32mpex->lParam = INT32(HIWORD(lpwm32mpex->lReturn));
        lpwm32mpex->lReturn = 0;
    }

    return (TRUE);
}


//  SBM_SETRANGE
//  SBM_SETRANGEREDRAW (new for Win32)

BOOL FASTCALL WM32SBMSetRange (LPWM32MSGPARAMEX lpwm32mpex)
{

    //
    // Changed semantics to support 32-bit scroll bar range:
    //
    // Win16:
    //   SendMessage(hwnd, SBM_SETRANGE, fRedraw, MAKELONG(posMin, posMax);
    //
    // Win32:
    //   SendMessage(hwnd, fRedraw ? SBM_SETRANGE : SBM_SETRANGEREDRAW,
    //               posMin, posMax);
    //

    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wMsg   = OLDSBM_SETRANGE;
        lpwm32mpex->Parm16.WndProc.wParam = (SBM_SETRANGEREDRAW == lpwm32mpex->uMsg);
        lpwm32mpex->Parm16.WndProc.lParam = MAKELONG( (WORD)lpwm32mpex->uParam, (WORD)lpwm32mpex->lParam);
    }

    return (TRUE);
}


//  LB_SETSEL

BOOL FASTCALL  WM32LBSetSel (LPWM32MSGPARAMEX lpwm32mpex)
{


    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wMsg = (WORD) (WM_USER + (lpwm32mpex->uMsg - LB_ADDSTRING + 1));
        lpwm32mpex->Parm16.WndProc.wParam = (WORD) lpwm32mpex->uParam;
        lpwm32mpex->Parm16.WndProc.lParam = (WORD)lpwm32mpex->lParam;  // loword = index, hiword = 0
    }

    return (TRUE);
}


// This function thunks the static control messages,
//
//  STM_SETICON
//  STM_GETICON
//

BOOL FASTCALL WM32STMControl (LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        switch (lpwm32mpex->uMsg) {
            case STM_SETICON:
                lpwm32mpex->Parm16.WndProc.wParam = (WORD)GETHICON16(lpwm32mpex->uParam);
                break;

            case STM_GETICON:
                break;

        }
        lpwm32mpex->Parm16.WndProc.wMsg = WM_USER + (lpwm32mpex->uMsg - STM_SETICON);
    }
    else {
        lpwm32mpex->lReturn = (LONG)HICON32(lpwm32mpex->lReturn);
    }


    return (TRUE);
}


// This function thunks the messages,
//
// MN_FINDMENUWINDOWFROMPOINT
//

// NT -    wparam = (PUINT)pitem  lParam = MAKELONG(pt.x, pt.y)
//         returns flags or hwnd    *pitem = index or -1
//
// win31   wParam = 0   lParam = same
//         returns 0 or MAKELONG(-1, item) or MAKELONG(-2, item) or MAKELONG(hwnd, item)


BOOL FASTCALL WM32MNFindMenuWindow (LPWM32MSGPARAMEX lpwm32mpex)
{
    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wMsg = WIN30_MN_FINDMENUWINDOWFROMPOINT;
        lpwm32mpex->Parm16.WndProc.wParam = 0;

    } else {
        USHORT n =  LOWORD(lpwm32mpex->lReturn);

        *(PLONG)lpwm32mpex->uParam = (SHORT)HIWORD(lpwm32mpex->lReturn);
        lpwm32mpex->lReturn = (LONG)HWND32(n);  // this sign-extends -1, -2 and leaves 0 as 0
    }
    return TRUE;
}
