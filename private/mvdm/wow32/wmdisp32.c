
/*++

 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WMDISP32.C
 *  WOW32 32-bit message thunks
 *
 *  History:
 *  Created 19-Feb-1992 by Chandan S. Chauhan (ChandanC)
 *  Changed 12-May-1992 by Mike Tricker (MikeTri) Added MultiMedia thunks
 *  Changed 09-Jul-1992 by v-cjones Added msg profiling debugger extension
--*/


#include "precomp.h"
#pragma hdrstop
#ifdef FE_IME
#include "wownls.h"
#include "ime.h"
#include "prshtp.h"
#endif // FE_IME

MODNAME(wmdisp32.c);

BOOL fThunkDDEmsg = TRUE;

extern WORD msgFINDREPLACE;  // see WCOMMDLG.C

#ifdef WOWPROFILE  // for MSG profiling only (debugger extension)
INT fWMsgProfRT = 0;
#endif


BOOL W32Win16DlgProcEx(HWND hdlg, UINT uMsg, UINT uParam, LONG lParam,
    VPWNDPROC vpDlgProc16,  // Next WndProc to call or NULL if default
    PWW pww)    // hwnd's PWW if already known or NULL
{
    BOOL fSuccess;
    register PTD ptd;
    WM32MSGPARAMEX wm32mpex;
    BOOL   fMessageNeedsThunking;

#ifdef WOWPROFILE  // for MSG profiling only (debugger extension)
    DWORD dwTics;
#endif

    ptd = CURRENTPTD();

    WOW32ASSERT(vpDlgProc16);

    // take out the marker bits and fix the RPL bits
    UnMarkWOWProc (vpDlgProc16,vpDlgProc16);

    // If the app has GP Faulted we don't want to pass it any more input
    // This should be removed when USER32 does clean up on task death so
    // it doesn't call us - mattfe june 24 92 HACK32

    if (ptd->dwFlags & TDF_IGNOREINPUT) {
        LOGDEBUG(6,("    W32Dlg16WndProc Ignoring Input Messsage %04X\n",uMsg));
        WOW32ASSERTMSG(gfIgnoreInputAssertGiven,
                       "W32Dlg16WndProc: TDF_IGNOREINPUT hack was used, shouldn't be, "
                       "please email DaveHart with repro instructions.  Hit 'g' to ignore this "
                       "and suppress this assertion from now on.\n");
        gfIgnoreInputAssertGiven = TRUE;
        return FALSE;
    }

    wm32mpex.Parm16.WndProc.hwnd   = GETHWND16(hdlg);
    wm32mpex.Parm16.WndProc.wMsg   = (WORD)uMsg;
    wm32mpex.Parm16.WndProc.wParam = (WORD)uParam;
    wm32mpex.Parm16.WndProc.lParam = (LONG)lParam;
    wm32mpex.Parm16.WndProc.hInst  = 0;   // Forces AX = SS on WndProc entry,
                                          // for Win 3.1 compatibility.

    fMessageNeedsThunking =  (uMsg < 0x400) &&
                                  (aw32Msg[uMsg].lpfnM32 != WM32NoThunking);

    if (fMessageNeedsThunking) {
        LOGDEBUG(3,("%04X (%s)\n", CURRENTPTD()->htask16, (aw32Msg[uMsg].lpszW32)));

#ifdef WOWPROFILE  // for MSG profiling only (debugger extension)
        dwTics = GetWOWTicDiff(0L);
#endif
        wm32mpex.fThunk = THUNKMSG;
        wm32mpex.hwnd = hdlg;
        wm32mpex.uMsg = uMsg;
        wm32mpex.uParam = uParam;
        wm32mpex.lParam = lParam;
        wm32mpex.pww = pww;
        wm32mpex.fFree = TRUE;
        wm32mpex.lpfnM32 = aw32Msg[uMsg].lpfnM32;
        if (!(wm32mpex.lpfnM32)(&wm32mpex)) {
            LOGDEBUG(LOG_ERROR,("    W32Win16DlgProcEx ERROR: cannot thunk 32-bit message %04x\n", uMsg));
            goto Error;
        }

#ifdef WOWPROFILE  // for MSG profiling only (debugger extension)
        if( !fWMsgProfRT ) {  // only if not profiling round trip
            aw32Msg[uMsg].cTics += GetWOWTicDiff(dwTics);
        }
#endif

    }
    else {
        LOGDEBUG(6,("    No Thunking was required for the 32-bit message %s(%04x)\n", (LPSZ)GetWMMsgName(uMsg), uMsg));
    }

    BlockWOWIdle(FALSE);

    fSuccess = CallBack16(RET_WNDPROC, &wm32mpex.Parm16, vpDlgProc16, (PVPVOID)&wm32mpex.lReturn);

    BlockWOWIdle(TRUE);

    // the callback function of a dialog is of type FARPROC whose return value
    // is of type 'int'. Since dx:ax is copied into lReturn in the above
    // CallBack16 call, we need to zero out the hiword, otherwise we will be
    // returning an erroneous value.

    wm32mpex.lReturn = (LONG)((SHORT)(LOWORD(wm32mpex.lReturn)));

    if (fMessageNeedsThunking) {

#ifdef WOWPROFILE  // for MSG profiling only (debugger extension)
        if( !fWMsgProfRT ) {  // only if not round trip profiling
            dwTics = GetWOWTicDiff(0L);
        }
#endif // WOWPROFILE


        //
        // if you send a message to a dialog what gets returned
        // to the caller is the dlg's msgresult window long.
        // app dialog functions will call
        //     SetWindowLong(hdlg, DWL_MSGRESULT, n);
        // during message processing so the right thing gets returned.
        // scottlu says we only need to do this for wm_gettext, it's
        // the only message whose result is an output count.
        //

        if (uMsg == WM_GETTEXT  &&  wm32mpex.lReturn != 0) {
            wm32mpex.lReturn = GetWindowLong(hdlg, DWL_MSGRESULT);
        }

        wm32mpex.fThunk = UNTHUNKMSG;
        (wm32mpex.lpfnM32)(&wm32mpex);

#ifdef WOWPROFILE  // for MSG profiling only (debugger extension)
        aw32Msg[uMsg].cTics += GetWOWTicDiff(dwTics);
        aw32Msg[uMsg].cCalls++;   // increment # times message passed
#endif // WOWPROFILE

    }

    if (!fSuccess)
        goto Error;

Done:

    return wm32mpex.lReturn;

Error:
    LOGDEBUG(6,("    W32Win16DlgProcEx WARNING: cannot call back, using default message handling\n"));
    wm32mpex.lReturn = 0;
    goto Done;
}








LONG W32Win16WndProcEx(HWND hwnd, UINT uMsg, UINT uParam, LONG lParam,
    VPWNDPROC vpWndProc16,  // Next WndProc to call or NULL if default
    PWW pww)    // hwnd's PWW if already known or NULL
{
    BOOL fSuccess;
    LONG ulReturn;
    register PTD ptd;
    WM32MSGPARAMEX wm32mpex;
    BOOL   fMessageNeedsThunking;
//#ifdef DEBUG
//    CHAR szClassName[80];
//#endif

#ifdef WOWPROFILE  // for MSG profiling only (debugger extension)
    DWORD dwTics;
#endif

    ptd = CURRENTPTD();

    WOW32ASSERT(vpWndProc16);

    // take out the marker bits and fix the RPL bits
    UnMarkWOWProc (vpWndProc16,vpWndProc16);


    //
    // If the app has GP Faulted we don't want to pass it any more input
    // This should be removed when USER32 does clean up on task death so
    // it doesn't call us - mattfe june 24 92 HACK32
    //
    // It's not a problem if the callback is going to a 16:16 proc in user.exe
    // like DefWindowProc.
    //

    if (ptd->dwFlags & TDF_IGNOREINPUT &&
        HIWORD(vpWndProc16) != HIWORD(gpfn16GetProcModule)) {

        LOGDEBUG(6,("    W32Win16WndProcEx Ignoring Input Messsage %04X\n",uMsg));
        WOW32ASSERTMSG(gfIgnoreInputAssertGiven,
                       "W32Win16WndProcEx: TDF_IGNOREINPUT hack was used, shouldn't be, "
                       "please email DaveHart with repro instructions.  Hit 'g' to ignore this "
                       "and suppress this assertion from now on.\n");
        gfIgnoreInputAssertGiven = TRUE;
        goto SilentError;
    }

    //
    // Figure out the class for this hwnd if we haven't seen it before
    //

    if (!pww) {
        if (!(pww = (PWW) GetWindowLong(hwnd, GWL_WOWWORDS))) {
            LOGDEBUG(LOG_ALWAYS,("WOW :: W32Win16WndProcEx ERROR: GetWindowLong(0x%x, GWL_WOWWORDS) fails\n", hwnd));
            goto Error;
        }
    }
#ifdef FE_IME
    if (( uMsg == WM_IME_REPORT ) && ( uParam == IR_STRINGEX )) {
        HANDLE hInstance;

        hInstance = (HANDLE)(ULONG)GetWindowLong(hwnd, GWL_HINSTANCE);
        hInstance = (HANDLE)(ULONG)VALIDHMOD(hInstance);
        if (W32GetExpWinVer(hInstance) < 0x030a)
            return 0;
    }

    //
    // Don't dispatch Version 4.0 IMM Messages to 16bit apps.
    //
    // WM_IME_STARTCOMPOSITION         0x010D
    // WM_IME_ENDCOMPOSITION           0x010E
    // WM_IME_COMPOSITION              0x010F
    // WM_IME_SETCONTEXT               0x0281
    // WM_IME_NOTIFY                   0x0282
    // WM_IME_CONTROL                  0x0283
    // WM_IME_COMPOSITIONFULL          0x0284
    // WM_IME_SELECT                   0x0285
    // WM_IME_CHAR                     0x0286
    // WM_IME_SYSTEM                   0x0287
    //
    if ((( uMsg >= WM_IME_STARTCOMPOSITION ) && ( uMsg <= WM_IME_COMPOSITION )) ||
        (( uMsg >= WM_IME_SETCONTEXT ) && (uMsg <= WM_IME_SYSTEM ))) {

        // Korean Edit conrol need to dispatch new IMM messages.
        if ( !(GetStdClassWndProc(WOWCLASS_EDIT) && GetSystemDefaultLangID()==0x412) ) {
             HANDLE hInstance;
             hInstance = (HANDLE)(ULONG)GetWindowLong(hwnd, GWL_HINSTANCE);
             hInstance = (HANDLE)(ULONG)VALIDHMOD(hInstance);
             if (W32GetExpWinVer(hInstance) < 0x0400)
                 goto Error;
        }
    }
#endif // FE_IME

    // This message is WIN32 only.  It is sent by WOW32 during the processing
    // of an EM_SETSEL in WU32Send/PostMessage.  If an MLE is subclassed the
    // message will come through here attempting to travel back to the 16-bit
    // app's wndproc.  Instead of sending back a message that the 16-bit app
    // doesn't understand it will be intercepted here and sent directly to the
    // standard EditWindowProc.  I'm not adding a Thunk because it shouldn't
    // go to the app.

    if (uMsg == EM_SCROLLCARET) {
        WNDPROC EditWndProc;

        // find the 32-bit EditWindowProc
        // We should only be in this state if the app has subclassed so this
        // call should be safe.

        EditWndProc = (WNDPROC)GetStdClassWndProc(WOWCLASS_EDIT);

        if (EditWndProc) {
            CallWindowProc(EditWndProc, hwnd, EM_SCROLLCARET, 0, 0);
        }
        else {
            LOGDEBUG(LOG_ALWAYS,("    W32Win16WndProcEx ERROR: cannot find 32-bit EditWindowProc\n"));
        }
        return 0;   // notification message, no return code
    }

    // Thunk this 32 bit message to 16 bit message

    LOGDEBUG(6,("    Thunking window %x message %s\n", hwnd, GetWMMsgName(uMsg)));
#ifdef DEBUG
    if((uMsg & WOWPRIVATEMSG) && ((uMsg & ~WOWPRIVATEMSG) < 0x400)) {
        LOGDEBUG(6,("     -- private WOW bit set for %s\n", GetWMMsgName(uMsg & ~WOWPRIVATEMSG)));
    }
#endif

    wm32mpex.Parm16.WndProc.hwnd   = GETHWND16(hwnd);
    wm32mpex.Parm16.WndProc.wMsg   = (WORD)uMsg;
    wm32mpex.Parm16.WndProc.wParam = (WORD)uParam;
    wm32mpex.Parm16.WndProc.lParam = (LONG)lParam;
    wm32mpex.Parm16.WndProc.hInst  = LOWORD(pww->hModule);

    // An app can send one of its private class windows a message say 401.
    // This message will not be thunked in WMSG16.C because the
    // messages >= 0x400 and we did not want to thunk it in WMSG16.C
    //

    fMessageNeedsThunking =  (uMsg < 0x400) &&
                                  (aw32Msg[uMsg].lpfnM32 != WM32NoThunking);

    if (fMessageNeedsThunking) {
        LOGDEBUG(6,("%04X (%s)\n", ptd->htask16, (aw32Msg[uMsg].lpszW32)));

#ifdef WOWPROFILE  // for MSG profiling only (debugger extension)
        dwTics = GetWOWTicDiff(0L);
#endif
        wm32mpex.fThunk = THUNKMSG;
        wm32mpex.hwnd = hwnd;
        wm32mpex.uMsg = uMsg;
        wm32mpex.uParam = uParam;
        wm32mpex.lParam = lParam;
        wm32mpex.pww = pww;
        wm32mpex.fFree = TRUE;
        wm32mpex.lpfnM32 = aw32Msg[uMsg].lpfnM32;
        ulReturn = (wm32mpex.lpfnM32)(&wm32mpex);

#ifdef WOWPROFILE  // for MSG profiling only (debugger extension)
        if( !fWMsgProfRT ) {  // only if not profiling round trip
            aw32Msg[uMsg].cTics += GetWOWTicDiff(dwTics);
        }
#endif

        if (!ulReturn) {
            LOGDEBUG(LOG_ALWAYS,("    W32Win16WndProcEx ERROR: cannot thunk 32-bit message %s (%x)\n", GetWMMsgName(uMsg), uMsg));
            goto Error;
        }
    }

    if (vpWndProc16 == (VPVOID)NULL) {
        WOW32ASSERT(vpWndProc16);
        goto SilentError;
    }

    LOGDEBUG(6,("16-bit Window Proc = %08lX\n", vpWndProc16));

    BlockWOWIdle(FALSE);

    fSuccess = CallBack16(RET_WNDPROC, &wm32mpex.Parm16, vpWndProc16, (PVPVOID)&wm32mpex.lReturn);

    BlockWOWIdle(TRUE);

    // During CreateWindow some apps draw their own non-client area and don't
    // pass WM_NCCALCSIZE to DefWindowProc which causes Win 95 and NT's user to
    // not set some needed window flags. Mavis Beacon is an example. We'll pass
    // the message for them.

    if (uMsg == WM_NCCALCSIZE) {
        if (CURRENTPTD()->dwWOWCompatFlagsEx & WOWCFEX_DEFWNDPROCNCCALCSIZE) {
            DefWindowProc(hwnd, uMsg, uParam, lParam);
        }
    }

    // UnThunk this 32 bit message

    LOGDEBUG(6,("    UnThunking window %x message %s\n", hwnd, (LPSZ)GetWMMsgName(uMsg)));
#ifdef DEBUG
    if((uMsg & WOWPRIVATEMSG) && ((uMsg - WOWPRIVATEMSG) < 0x400)) {
        LOGDEBUG(6,("     -- private WOW bit set for %s\n", (LPSZ)GetWMMsgName(uMsg)));
    }
#endif

    if (fMessageNeedsThunking) {

#ifdef WOWPROFILE  // for MSG profiling only (debugger extension)
        if( !fWMsgProfRT ) {  // only if not profiling round trip
            dwTics = GetWOWTicDiff(0L);
        }
#endif

        wm32mpex.fThunk = UNTHUNKMSG;
        (wm32mpex.lpfnM32)(&wm32mpex);

#ifdef WOWPROFILE  // for MSG profiling only (debugger extension)
        aw32Msg[uMsg].cTics += GetWOWTicDiff(dwTics);
        aw32Msg[uMsg].cCalls++;   // increment # times message passed
#endif

    }

    if (!fSuccess) {
        goto Error;
    }

    return (wm32mpex.lReturn);

Error:
    LOGDEBUG(LOG_ALWAYS,("    W32Win16WndProcEx ERROR: cannot call back, using default message handling\n"));
SilentError:
    return DefWindowProc(hwnd, uMsg, uParam, lParam);
}



// The following functions are used to "thunk" a 32 bit message to 16 bit
// message.
//
// To add a thunk function for a 32 bit message,
//    - Modify the entry for the message in "aw32Msg" function array
//  (in wmtbl32.c) to point to the new thunk function.
//    - Define the new thunk function in this file.
//


// These messages do not require any thunking so just copy the 32 bit wParam
// and lParam to 16 bit wParam and lParam.
//
//
//  WM_CANCELMODE
//  WM_CHAR
//  WM_CHILDACTIVATE
//  WM_CLEAR
//  WM_CLOSE
//  WM_COMMNOTIFY
//  WM_COMPACTING
//  WM_COPY
//  WM_CUT
//  WM_DEADCHAR
//  WM_DESTROY
//  WM_DRAWCLIPBOARD
//  WM_ENABLE
//  WM_ENDSESSION
//  WM_FONTCHANGE
//  WM_GETFONT
//  WM_GETTEXTLENGTH
//  WM_HOTKEY
//  WM_INPUTFOCUS
//  WM_ISACTIVEICON (undocumented)
//  WM_KEYDOWN
//  WM_KEYUP
//  WM_LBTRACKPOINT (undocumented)
//  WM_LBUTTONDBLCLK
//  WM_LBUTTONDOWN
//  WM_LBUTTONUP
//  WM_MBUTTONDBLCLK
//  WM_MBUTTONDOWN
//  WM_MBUTTONUP
//  WM_MDICASCADE
//  WM_MDIICONARRANGE
//  WM_MDINEXT
//  WM_MDITILE
//  WM_MOUSEENTER
//  WM_MOUSELEAVE
//  WM_MOUSEMOVE
//  WM_MOVE
//  WM_NCCALCRGN
//  WM_NCDESTROY
//  WM_NCHITTEST
//  WM_NCLBUTTONDBLCLK
//  WM_NCLBUTTONDOWN
//  WM_NCLBUTTONUP
//  WM_NCMBUTTONDBLCLK
//  WM_NCMBUTTONDOWN
//  WM_NCMBUTTONUP
//  WM_NCMOUSEMOVE
//  WM_NCRBUTTONDBLCLK
//  WM_NCRBUTTONDOWN
//  WM_NCRBUTTONUP
//  WM_PAINTICON
//  WM_PASTE
//  WM_POWER
//  WM_QUERYENDSESSION
//  WM_QUERYNEWPALETTE
//  WM_QUERYOPEN
//  WM_QUERYPARKICON (undocumented)
//  WM_QUEUESYNC
//  WM_QUIT
//  WM_RBUTTONDBLCLK
//  WM_RBUTTONDOWN
//  WM_RBUTTONUP
//  WM_RENDERALLFORMATS
//  WM_RENDERFORMAT
//  WM_SETREDRAW
//  WM_SHOWWINDOW
//  WM_SIZE
//  WM_SPOOLERSTATUS (double-check lParam conversion on this one -JTP)
//  WM_SYSCHAR
//  WM_SYSCOLORCHANGE
//  WM_SYSCOMMAND
//  WM_SYSDEADCHAR
//  WM_SYSKEYDOWN
//  WM_SYSKEYUP
//  WM_SYSTEMERROR
//  WM_TIMECHANGE
//  WM_UNDO
//  MM_JOY1BUTTONDOWN     - MultiMedia messages
//  MM_JOY1BUTTONUP
//  MM_JOY1MOVE
//  MM_JOY1ZMOVE
//  MM_JOY2BUTTONDOWN
//  MM_JOY2BUTTONUP
//  MM_JOY2MOVE
//  MM_JOY2ZMOVE
//  MM_MCINOTIFY          - MultiMedia messages


BOOL FASTCALL WM32NoThunking(LPWM32MSGPARAMEX lpwm32mpex)
{

#if 0
    //
    // this routine is never called!  It's used as a placeholder.
    // if you want to make a change here, you have to make the change
    // to the places where we compare the thunk routine to WM32NoThunking
    // and only call the thunk routine if it's not this.  also make sure
    // that this 'default' thunking happens for NoThunking messages.
    //

    if (lpwm32mpex->fThunk) {
        LOGDEBUG(6,("    No Thunking was required for the 32-bit message %s(%04x)\n", (LPSZ)GetWMMsgName(lpwm32mpex->uMsg), lpwm32mpex->uMsg));

        lpwm32mpex->Parm16.WndProc.wMsg = (WORD)lpwm32mpex->uMsg;
        lpwm32mpex->Parm16.WndProc.wParam = (WORD)lpwm32mpex->uParam;
        lpwm32mpex->Parm16.WndProc.lParam = (LONG)lpwm32mpex->lParam;
    }

    //
    // this routine is never called!  It's used as a placeholder.
    // if you want to make a change here, you have to make the change
    // to the places where we compare the thunk routine to WM32NoThunking
    // and only call the thunk routine if it's not this.
    //
#endif

    //
    // Return FALSE, so if for some reason this routine gets used
    // the failure to thunk will be apparent.
    //

    return FALSE;
}

#ifdef DEBUG         // see the macro WM32UNDOCUMENTED

// These are undocumented messages for Win 3.0 so take a look at the app
// who is using them.

BOOL FASTCALL WM32Undocumented(LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        LOGDEBUG(3,(" Window %08lX is receiving Undocumented Message %s\n", lpwm32mpex->hwnd, (LPSZ)GetWMMsgName(lpwm32mpex->uMsg), lpwm32mpex->uMsg));

        lpwm32mpex->Parm16.WndProc.wMsg = (WORD)lpwm32mpex->uMsg;
        lpwm32mpex->Parm16.WndProc.wParam = (WORD)lpwm32mpex->uParam;
        lpwm32mpex->Parm16.WndProc.lParam = (LONG)lpwm32mpex->lParam;
    }

    return (TRUE);
}

#endif



// This function thunks the messages,
//
//  WM_CREATE
//  WM_NCCREATE
//

BOOL FASTCALL WM32Create(LPWM32MSGPARAMEX lpwm32mpex)
{


    INT cb;
    VPVOID vpClass = 0;
    VPVOID vpName = 0;
    VPVOID vpCreateParams = 0;
    register PCREATESTRUCT16 pcws16;
    LPCREATESTRUCT lParam = (LPCREATESTRUCT) lpwm32mpex->lParam;

    if (lpwm32mpex->fThunk) {

        if (HIWORD(lParam)) {

            // BUGBUG -- The assumption here is that GlobalAlloc will never
            // return a memory object that isn't word-aligned, so that we can
            // assign word-aligned words directly;  we have no idea whether the
            // memory is dword-aligned or not however, so dwords must always
            // be paranoidly stored with the STOREDWORD/STORELONG macros -JTP


            if (lParam->lpszClass) {
                if ( HIWORD(lParam->lpszClass) == 0 ) {
                    vpClass = (VPVOID)lParam->lpszClass;
                }
                else {
                    cb = strlen(lParam->lpszClass)+1;
                    if (!(vpClass = malloc16(cb)))
                        goto Error;
                    putstr16(vpClass, lParam->lpszClass, cb);
                }
            }

            if (lParam->lpszName) {
                cb = strlen(lParam->lpszName)+1;
                if (!(vpName = malloc16(cb)))
                    goto Error;
                putstr16(vpName, lParam->lpszName, cb);
            }

            if (lpwm32mpex->pww == NULL) {
                lpwm32mpex->pww = (PWW)GetWindowLong(lpwm32mpex->hwnd, GWL_WOWWORDS);
                if (lpwm32mpex->pww == NULL)
                    return FALSE;   // Window is dead
            }

            if (lParam->lpCreateParams && (lpwm32mpex->pww->ExStyle & WS_EX_MDICHILD) ) {
                // This works because wm32mdicreate thunk doesn't use any
                // parameters except lParam

                WM32MSGPARAMEX wm32mpexT;
                wm32mpexT.fThunk = lpwm32mpex->fThunk;
                wm32mpexT.hwnd = lpwm32mpex->hwnd;
                wm32mpexT.uMsg = WM_MDICREATE;
                wm32mpexT.uParam = lpwm32mpex->uParam;
                wm32mpexT.lParam = (LONG)lParam->lpCreateParams;
                wm32mpexT.pww = lpwm32mpex->pww;
                wm32mpexT.fFree = lpwm32mpex->fFree;
                wm32mpexT.Parm16.WndProc.lParam = 0;
                WM32MDICreate(&wm32mpexT);
                lpwm32mpex->dwParam = wm32mpexT.dwParam;
                vpCreateParams = wm32mpexT.Parm16.WndProc.lParam;
            }
            else {
                vpCreateParams = (VPVOID)lParam->lpCreateParams;
            }

            // be sure allocation size matches stackfree16() size below
            if (!(lpwm32mpex->Parm16.WndProc.lParam = stackalloc16(sizeof(CREATESTRUCT16))))
                return FALSE;

            GETVDMPTR(lpwm32mpex->Parm16.WndProc.lParam, sizeof(CREATESTRUCT16), pcws16);

            STOREDWORD(pcws16->vpszClass, vpClass);
            STOREDWORD(pcws16->vpszWindow, vpName);
            STOREDWORD(pcws16->vpCreateParams, vpCreateParams);

            lpwm32mpex->dwTmp[0] = vpClass; // store for later freeing
            lpwm32mpex->dwTmp[1] = vpName;


            // BUGBUG 08-Apr-91 JeffPar -- What if hModule is for a 32-bit task?
            pcws16->hInstance    = GETHINST16(lParam->hInstance);
            pcws16->hMenu    = GETHMENU16(lParam->hMenu);
            pcws16->hwndParent   = GETHWND16(lParam->hwndParent);
            pcws16->cy       = (SHORT)lParam->cy;
            pcws16->cx       = (SHORT)lParam->cx;
            pcws16->y        = (SHORT)lParam->y;
            pcws16->x        = (SHORT)lParam->x;
            STOREDWORD(pcws16->dwStyle, lParam->style);
            STOREDWORD(pcws16->dwExStyle, lParam->dwExStyle);

            FLUSHVDMPTR(lpwm32mpex->Parm16.WndProc.lParam, sizeof(CREATESTRUCT16), pcws16);
            FREEVDMPTR(pcws16);

            return TRUE;

          Error:
            LOGDEBUG(LOG_ALWAYS,(" !!!! WM32Create, WM_CREATE thunking failed !!!! Window %08lX ", lpwm32mpex->hwnd));
            if (HIW(vpClass)) free16(vpClass);
            if (vpName)       free16(vpName);
            return (FALSE);

            // do some clean up
            // UnThunkWMCreate32(lParam, lpwm32mpex->Parm16.WndProc.lParam);

        } else {
            return TRUE;
        }




    }
    else {

        if (lpwm32mpex->Parm16.WndProc.lParam) {

            if (lpwm32mpex->pww == NULL) {
                lpwm32mpex->pww = (PWW)GetWindowLong(lpwm32mpex->hwnd, GWL_WOWWORDS);
                if (lpwm32mpex->pww == NULL)
                    return FALSE;   // Window is dead
            }

            if (lParam->lpCreateParams && (lpwm32mpex->pww->ExStyle & WS_EX_MDICHILD) ) {
                WM32MSGPARAMEX wm32mpexT;
                GETVDMPTR(lpwm32mpex->Parm16.WndProc.lParam, sizeof(CREATESTRUCT16), pcws16);
                wm32mpexT.fThunk = lpwm32mpex->fThunk;
                wm32mpexT.hwnd = lpwm32mpex->hwnd;
                wm32mpexT.uMsg = WM_MDICREATE;
                wm32mpexT.uParam = lpwm32mpex->uParam;
                wm32mpexT.lParam = (LONG)lParam->lpCreateParams;
                wm32mpexT.pww = lpwm32mpex->pww;
                wm32mpexT.fFree = lpwm32mpex->fFree;
                wm32mpexT.Parm16.WndProc.lParam = (VPVOID)FETCHDWORD(pcws16->vpCreateParams);
                wm32mpexT.lReturn = 0;
                wm32mpexT.dwParam = lpwm32mpex->dwParam;
                WM32MDICreate(&wm32mpexT);
                FREEVDMPTR(pcws16);
            }

            vpClass = lpwm32mpex->dwTmp[0];
            vpName  = lpwm32mpex->dwTmp[1];

            //  if HIWORD(class) is zero, class is an atom, else a pointer.

            if (HIW16(vpClass)) {
                free16(vpClass);
            }

            if (vpName) {
                free16(vpName);
            }

            stackfree16((VPVOID) lpwm32mpex->Parm16.WndProc.lParam,
                        sizeof(CREATESTRUCT16));
        }

        return TRUE;
    }

}


// This function thunks the messages,
//
//  WM_NCACTIVATE
//  WM_ACTIVATE
//

BOOL FASTCALL WM32Activate(LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wParam = LOWORD(lpwm32mpex->uParam);
        LOW(lpwm32mpex->Parm16.WndProc.lParam) = GETHWND16(lpwm32mpex->lParam);
        HIW(lpwm32mpex->Parm16.WndProc.lParam) = HIWORD(lpwm32mpex->uParam);
    }

    return (TRUE);
}



// This function thunks the messages,
//
//  WM_VKEYTOITEM
//  WM_CHARTOITEM
//  WM_BEGINDRAG
//

BOOL FASTCALL WM32VKeyToItem(LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        LOW(lpwm32mpex->Parm16.WndProc.lParam) = GETHWND16(lpwm32mpex->lParam);
        HIW(lpwm32mpex->Parm16.WndProc.lParam) = HIWORD(lpwm32mpex->uParam);
    }
    else {
        lpwm32mpex->lReturn = (INT)(SHORT)(lpwm32mpex->lReturn); // sign extend.
    }

    return (TRUE);
}


// This function thunks the messages,
//
//  WM_SETFOCUS
//  WM_KILLFOCUS
//  WM_SETCURSOR
//  WM_MOUSEACTIVATE
//  WM_MDIDESTROY
//  WM_MDIRESTORE
//  WM_MDIMAXIMIZE
//  WM_VSCROLLCLIPBOARD
//  WM_HSCROLLCLIPBOARD
//  WM_PALETTECHANGED
//  WM_PALETTEISCHANGING
//  WM_INITDIALOG
//

BOOL FASTCALL WM32SetFocus(LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wParam = GETHWND16(lpwm32mpex->uParam);
    }

    return (TRUE);
}

// This function thunks the messages,
//
//  WM_SETTEXT
//  WM_WININICHANGE
//  WM_DEVMODECHANGE
//

BOOL FASTCALL WM32SetText(LPWM32MSGPARAMEX lpwm32mpex)
{
    INT cb;


    if (lpwm32mpex->fThunk) {
        if (lpwm32mpex->lParam) {

            LONG lParam = (LONG)GetParam16(lpwm32mpex->lParam);
            if (lParam) {
                lpwm32mpex->Parm16.WndProc.lParam = lParam;
                return (TRUE);
            }

            cb = strlen((LPSZ)lpwm32mpex->lParam)+1;
            lpwm32mpex->dwTmp[0] = (DWORD)cb;

            // winworks2.0a requires DS based string pointers for this message
            if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_DSBASEDSTRINGPOINTERS) {

                // be sure allocation size matches stackfree16() size below
                if (!(lpwm32mpex->Parm16.WndProc.lParam = stackalloc16(cb)))
                    return FALSE;

            } else {
                if (!(lpwm32mpex->Parm16.WndProc.lParam = malloc16(cb)))
                    return FALSE;
            }
            putstr16((VPSZ)lpwm32mpex->Parm16.WndProc.lParam, (LPSZ)lpwm32mpex->lParam, cb);
        }
    }
    else {
// BUGBUG 09-Apr-91 -- Should I copy back?
        if (DeleteParamMap(lpwm32mpex->Parm16.WndProc.lParam, PARAM_16, NULL)) {
            return TRUE;
        }

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


// This function thunks the message,
//
//  WM_GETTEXT
//

BOOL FASTCALL WM32GetText(LPWM32MSGPARAMEX lpwm32mpex)
{
    INT cb;
    LPSTR   psz;
    INT cbWrote;



    if (lpwm32mpex->fThunk) {

        if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_DSBASEDSTRINGPOINTERS) {
            //
            // msworks 2.0a has a wndproc called EdWnProc() which when it gets
            // a WM_GETTEXT, assumes lParam is a based pointer whose segment
            // value is equal to winwork's ds. That is true under win3.1, but
            // if wow calls malloc16, it'll have a different segment value.
            // so instead alloc the space on the caller's stack. Since most
            // apps have SS == DS, this will fix apps that do this, including
            // msworks 2.0a.
            //

            // be sure allocation size matches stackfree16() size below
            lpwm32mpex->dwTmp[0] = (DWORD)lpwm32mpex->Parm16.WndProc.wParam;
            lpwm32mpex->Parm16.WndProc.lParam = stackalloc16(lpwm32mpex->dwTmp[0]);

        } else {
            lpwm32mpex->Parm16.WndProc.lParam = malloc16(lpwm32mpex->Parm16.WndProc.wParam);
        }

        //
        // non-zero fill to detect people who write more than they
        // say that they do!
        //
        GETVDMPTR(lpwm32mpex->Parm16.WndProc.lParam, lpwm32mpex->Parm16.WndProc.wParam, psz);
        RtlFillMemory(psz, lpwm32mpex->Parm16.WndProc.wParam, 0xff);
        FLUSHVDMPTR(lpwm32mpex->Parm16.WndProc.lParam, lpwm32mpex->Parm16.WndProc.wParam, psz);
        FREEVDMPTR(psz);
        return (BOOL)lpwm32mpex->Parm16.WndProc.lParam;
    }
    else {
        // some apps return garbage in the high word.  safely assume
        // that cbWindowText < 64K
        HIW(lpwm32mpex->lReturn) = 0;

        // it is necessary to check the length of the buffer, specified in
        // lpwm32mpex->uParam. if number of bytes (lpwm32mpex->lReturn) that are to be copied is
        // EQUAL to the length of the buffer, then copy ONLY the bytes EQUAL
        // to the length of the buffer.
        //

        // Paradox is one of the apps where this condition is hit.
        // bug # 4272.


        //

        if (lpwm32mpex->Parm16.WndProc.lParam) {

            cb = lpwm32mpex->lReturn + 1;

            if (lpwm32mpex->uParam == 0) {
                // cb = 0 if lReturn == 0 && uParam == 0

                if (cb == 1)
                    cb--;
            }
            else if (cb == 2 || cb == 1) {
                // Here only if uParam != 0
                //
                // Determine how much of the buffer they touched!
                //
                // MyAdvancedLabelMaker returns 1 when they really return
                // more than 1.  Since the return 1, cb will be 2.  Then
                // We check to see how much of the buffer they really modified.
                // Then we lie and say that they really filled in that much
                // of the buffer.
                //
                // Sql administator also does this, except it returns 0
                // bug 7731

                GETVDMPTR(lpwm32mpex->Parm16.WndProc.lParam, lpwm32mpex->Parm16.WndProc.wParam, psz);

                cbWrote = lpwm32mpex->uParam;
                while (cbWrote && (psz[cbWrote-1] == '\xff')) {
                    cbWrote--;
                }
                // copy out as many bytes as they wrote
                // distinguish between 'zerobytes written vs. one byte written'

                lpwm32mpex->lReturn = (cbWrote) ? (cbWrote - 1) : 0;
                cb = cbWrote;

                FREEVDMPTR(psz);
            }


            // cb = min(cb, wparam) only if wparam != 0
            // MSPROFIT: does
            //    ret = sendmessage(hwnd, wm_gettest, wparam = 0, lparam);
            //    where ret != 0. so we have to copy the necessary bytes into
            //    lparam eventhough wparam is zero. It does this for reading
            //    those ominprseent "$0.00" strings in the app (ledgers etc).
            //
            //                                   - nanduri

            if (lpwm32mpex->uParam && (UINT)cb > lpwm32mpex->uParam) {
                cb = lpwm32mpex->uParam;
            }

            getstr16((VPSZ)lpwm32mpex->Parm16.WndProc.lParam, (LPSZ)lpwm32mpex->lParam, cb);

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


// This function thunks the messages,
//
//  WM_ERASEBKGND
//  WM_ICONERASEBKGND
//

BOOL FASTCALL WM32EraseBkGnd(LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wParam = GETHDC16(lpwm32mpex->uParam);
    }

    return (TRUE);
}




// This function thunks the message
//
//  WM_CHANGEUISTATE
//  WM_UPDATEUISTATE
//  WM_QUERYUISTATE
//

BOOL FASTCALL WM32xxxUIState(LPWM32MSGPARAMEX lpwm32mpex)
{

    // just copy the wParam into the lParam
    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.lParam = (LONG)lpwm32mpex->uParam;
        lpwm32mpex->Parm16.WndProc.wParam = 0;

        // this is here because there is talk that they might extend this
        // message to use lParam -- heck, they didn't even tell us that they
        // implemented this message in the first place!! (shame on shell!!)
        WOW32WARNMSG((lpwm32mpex->lParam == 0),
                     ("WOW::WM32xxxUIState:lParam != 0. Better investigate!\n"));
    }

    // now just reverse the process
    else {
        lpwm32mpex->uParam = (UINT)lpwm32mpex->Parm16.WndProc.lParam;
        lpwm32mpex->lParam = 0;
    }

    return (TRUE);
}





// This function thunks the messages,
//
//  WM_ACTIVATEAPP
//

BOOL FASTCALL WM32ActivateApp(LPWM32MSGPARAMEX lpwm32mpex)
{
    extern void UpdateInt16State(void);

    if (lpwm32mpex->fThunk) {

        LOW(lpwm32mpex->Parm16.WndProc.lParam) =
            lpwm32mpex->lParam
              ? ThreadID32toHtask16((DWORD)lpwm32mpex->lParam)
              : 0;

        // We need to update wow int 16 bios when I wow app gets the focus.
        UpdateInt16State();
    }

    return (TRUE);
}


// This function thunks the messages,
//
//  WM_GETMINMAXINFO
//

BOOL FASTCALL WM32GetMinMaxInfo(LPWM32MSGPARAMEX lpwm32mpex)
{
    LPPOINT lParam = (LPPOINT) lpwm32mpex->lParam;


    if (lpwm32mpex->fThunk) {
        if (lParam) {

            // be sure allocation size matches stackfree16() size below
            lpwm32mpex->Parm16.WndProc.lParam = stackalloc16(sizeof(POINT16)*5);

            UnThunkWMGetMinMaxInfo16(lpwm32mpex->Parm16.WndProc.lParam, lParam);
        }
    }
    else {
        ThunkWMGetMinMaxInfo16(lpwm32mpex->Parm16.WndProc.lParam, &lParam);
        if(lpwm32mpex->Parm16.WndProc.lParam) {
            stackfree16((VPVOID) lpwm32mpex->Parm16.WndProc.lParam,
                        sizeof(POINT16)*5);
        }
    }

    return(TRUE);
}



// This function thunks the messages,
//
//  WM_NCPAINT
//

BOOL FASTCALL WM32NCPaint(LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wParam = (lpwm32mpex->uParam == 1) ? 1 :
                                              GETHDC16(lpwm32mpex->uParam);
    }
    return (TRUE);
}



// This function thunks the messages,
//
//  WM_NCDESTROY
//

BOOL FASTCALL WM32NCDestroy(LPWM32MSGPARAMEX lpwm32mpex)
{

    // destroy any timers associated with this window
    if (!lpwm32mpex->fThunk) {
        FreeWindowTimers16(lpwm32mpex->hwnd);
    }
    return (TRUE);
}


// This function thunks the messages,
//
//  WM_GETDLGCODE
//
BOOL FASTCALL WM32GetDlgCode(LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        if (lpwm32mpex->lParam) {

            // BUGBUG -- The assumption here is that GlobalAlloc will never
            // return a memory object that isn't word-aligned, so that we can
            // assign word-aligned words directly;  we have no idea whether the
            // memory is dword-aligned or not however, so dwords must always
            // be paranoidly stored with the STOREDWORD/STORELONG macros -JTP

            if (!(lpwm32mpex->Parm16.WndProc.lParam = malloc16(sizeof(MSG16))))
                return FALSE;

            putmsg16(lpwm32mpex->Parm16.WndProc.lParam, (LPMSG)lpwm32mpex->lParam);

            return TRUE;
        }
    }
    else {
        // Message structure doesn't need to be copied back does it? -Bob

        if (lpwm32mpex->Parm16.WndProc.lParam) {
            free16((VPVOID) lpwm32mpex->Parm16.WndProc.lParam);
        }

    }
    return (TRUE);
}

// This function thunks the messages,
//
//  WM_NEXTDLGCTL
//

BOOL FASTCALL WM32NextDlgCtl(LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        if (lpwm32mpex->lParam) {
            lpwm32mpex->Parm16.WndProc.wParam = GETHWND16(lpwm32mpex->uParam);
        }
    }
    return (TRUE);
}


// This function thunks the messages,
//
//  WM_DRAWITEM
//

BOOL FASTCALL WM32DrawItem(LPWM32MSGPARAMEX lpwm32mpex)
{
    LPDRAWITEMSTRUCT lParam = (LPDRAWITEMSTRUCT) lpwm32mpex->lParam;


    if (lpwm32mpex->fThunk) {
        if (lParam) {

            // be sure allocation size matches stackfree16() size below
            lpwm32mpex->Parm16.WndProc.lParam = stackalloc16(sizeof(DRAWITEMSTRUCT16));
            putdrawitem16(lpwm32mpex->Parm16.WndProc.lParam, lParam);
        }
    }
    else {
        // BUGBUG 08-Apr-91 JeffPar -- Reflect changes back to 32-bit structure?
        if (lpwm32mpex->Parm16.WndProc.lParam)
            stackfree16((VPVOID) lpwm32mpex->Parm16.WndProc.lParam,
                        sizeof(DRAWITEMSTRUCT16));
    }

    return(TRUE);
}


// This function thunks the messages,
//
//  WM_MEASUREITEM
//

BOOL FASTCALL WM32MeasureItem(LPWM32MSGPARAMEX lpwm32mpex)
{
    PMEASUREITEMSTRUCT16 pmis16;
    LPMEASUREITEMSTRUCT lParam = (LPMEASUREITEMSTRUCT) lpwm32mpex->lParam;
    BOOL    fHasStrings;
    DWORD   cSize;


    //
    // Compatibility hack
    //
    // CrossTalk 2.0 has a bug where it fails to distinguish between
    // WM_MEASUREITEM and WM_INITDIALOG when doing file.open
    // on WM_MEASUREITEM it calls CallWindowProc() to send what it
    // thinks is lpOpenFileName->lpCust but is really random stack.
    // currently the high word of this random pointer is an hInstance
    // and gets through the validation layer, whereas on Win31 it doesn't.
    // if this WM_MEASUREITEM gets to the app's proc then the app will
    // initialize incorrectly and take a GP.  i have increased the stack
    // allocation by XTALKHACK to ensure that the random data does is not
    // a valid pointer.
    //

#define XTALKHACK (sizeof(OPENFILENAME16)-sizeof(MEASUREITEMSTRUCT16))


    if (lpwm32mpex->fThunk) {
        if (lParam) {

            fHasStrings = FALSE;
            if ( lParam->CtlType == ODT_COMBOBOX || lParam->CtlType == ODT_LISTBOX ) {
                if (lParam->itemWidth == MIFLAG_FLAT) {
                    fHasStrings = TRUE;
                }
            }

            cSize = sizeof(MEASUREITEMSTRUCT16);
            if ( fHasStrings ) {
                cSize += strlen((LPSTR)lParam->itemData) + 1;
            }

            if ( cSize < XTALKHACK+sizeof(MEASUREITEMSTRUCT16) ) {
                cSize = XTALKHACK+sizeof(MEASUREITEMSTRUCT16);
            }

            // be sure allocation size matches stackfree16() size below
            lpwm32mpex->dwTmp[0] = cSize;            
            if ( !(lpwm32mpex->Parm16.WndProc.lParam = stackalloc16(cSize)) )
                return FALSE;

            GETVDMPTR(lpwm32mpex->Parm16.WndProc.lParam, cSize, pmis16);

            pmis16->CtlType = (WORD)lParam->CtlType;
            pmis16->CtlID   = (WORD)lParam->CtlID;
            pmis16->itemID  = (WORD)lParam->itemID;
            pmis16->itemWidth   = (WORD)lParam->itemWidth;
            pmis16->itemHeight  = (WORD)lParam->itemHeight;

#ifdef XTALKHACK
            ((POPENFILENAME16)pmis16)->lCustData = 7;   // invalid far pointer
#endif
            if ( fHasStrings ) {
                pmis16->itemData = lpwm32mpex->Parm16.WndProc.lParam+sizeof(MEASUREITEMSTRUCT16);
                strcpy( (LPSTR)(pmis16+1), (LPSTR)lParam->itemData );
            } else {
                STOREDWORD(pmis16->itemData, lParam->itemData);
            }

            WOW32ASSERT(HIWORD(cSize) == 0);
            FLUSHVDMPTR(lpwm32mpex->Parm16.WndProc.lParam, (USHORT) cSize, pmis16);
            FREEVDMPTR(pmis16);
        }
    }
    else {
        if (lpwm32mpex->Parm16.WndProc.lParam) {
            GETVDMPTR(lpwm32mpex->Parm16.WndProc.lParam, sizeof(MEASUREITEMSTRUCT16), pmis16);

            lParam->CtlType = WORD32(pmis16->CtlType);
            lParam->CtlID   = WORD32(pmis16->CtlID);
            lParam->itemID  = WORD32(pmis16->itemID);

            // itemWidth must sign extend (PPT3 bug & Win3.1 treats it as signed!)
            lParam->itemWidth   = INT32(pmis16->itemWidth);

            lParam->itemHeight  = WORD32(pmis16->itemHeight);
            lParam->itemData    = pmis16->itemData;

            FREEVDMPTR(pmis16);

            stackfree16((VPVOID) lpwm32mpex->Parm16.WndProc.lParam,
                        ((UINT)lpwm32mpex->dwTmp[0]));
        }
    }

    return(TRUE);
}


// This function thunks the messages,
//
//  WM_DELETEITEM
//


BOOL FASTCALL WM32DeleteItem(LPWM32MSGPARAMEX lpwm32mpex)
{
    register PDELETEITEMSTRUCT16 pdes16;
    LPDELETEITEMSTRUCT lParam = (LPDELETEITEMSTRUCT) lpwm32mpex->lParam;



    if (lpwm32mpex->fThunk) {
        if (lParam) {

            // BUGBUG -- The assumption here is that GlobalAlloc will never
            // return a memory object that isn't word-aligned, so that we can
            // assign word-aligned words directly;  we have no idea whether the
            // memory is dword-aligned or not however, so dwords must always
            // be paranoidly stored with the STOREDWORD/STORELONG macros -JTP

            // be sure allocation size matches stackfree16() size below
            lpwm32mpex->Parm16.WndProc.lParam = stackalloc16(sizeof(DELETEITEMSTRUCT16));
            GETVDMPTR(lpwm32mpex->Parm16.WndProc.lParam, sizeof(DELETEITEMSTRUCT16), pdes16);

            pdes16->CtlType = (WORD)lParam->CtlType;
            pdes16->CtlID   = (WORD)lParam->CtlID;
            pdes16->itemID  = (WORD)lParam->itemID;
            pdes16->hwndItem    = GETHWND16(lParam->hwndItem);
            STOREDWORD(pdes16->itemData, lParam->itemData);

            FLUSHVDMPTR(lpwm32mpex->Parm16.WndProc.lParam, sizeof(DELETEITEMSTRUCT16), pdes16);
            FREEVDMPTR(pdes16);
        }
    }
    else {
        if (lpwm32mpex->Parm16.WndProc.lParam)
            stackfree16((VPVOID) lpwm32mpex->Parm16.WndProc.lParam,
                        sizeof(DELETEITEMSTRUCT16));
    }

    return(TRUE);
}


// This function thunks the messages,
//
//  WM_SETFONT
//

BOOL FASTCALL WM32SetFont(LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wParam = GETHFONT16(lpwm32mpex->uParam);
    }

    return (TRUE);
}


// This function thunks the messages,
//
//  WM_QUERYDRAGICON

BOOL FASTCALL WM32QueryDragIcon(LPWM32MSGPARAMEX lpwm32mpex)
{

    if (!lpwm32mpex->fThunk) {
        lpwm32mpex->lReturn = (LONG)HICON32(lpwm32mpex->lReturn);
    }

    return (TRUE);
}


// This function thunks the messages,
//
//  WM_COMPAREITEM
//

BOOL FASTCALL WM32CompareItem(LPWM32MSGPARAMEX lpwm32mpex)
{
    LPCOMPAREITEMSTRUCT lParam = (LPCOMPAREITEMSTRUCT) lpwm32mpex->lParam;


    if (lpwm32mpex->fThunk) {
        if (lParam) {

            // BUGBUG -- The assumption here is that GlobalAlloc will never
            // return a memory object that isn't word-aligned, so that we can
            // assign word-aligned words directly;  we have no idea whether the
            // memory is dword-aligned or not however, so dwords must always
            // be paranoidly stored with the STOREDWORD/STORELONG macros -JTP

            // be sure allocation size matches stackfree16() size below
            lpwm32mpex->Parm16.WndProc.lParam = stackalloc16(sizeof(COMPAREITEMSTRUCT16));
            putcompareitem16(lpwm32mpex->Parm16.WndProc.lParam, lParam);
        }
    }
    else {
        // BUGBUG 08-Apr-91 JeffPar -- Reflect changes back to 32-bit structure?
        if (lpwm32mpex->Parm16.WndProc.lParam)
            stackfree16((VPVOID) lpwm32mpex->Parm16.WndProc.lParam,
                        sizeof(COMPAREITEMSTRUCT16));
    }

    return (TRUE);
}




// This function thunks the messages,
//
//  WM_SIZING
//

BOOL FASTCALL WM32Sizing(LPWM32MSGPARAMEX lpwm32mpex)
{
    VPRECT16   vpRect16;


    if (lpwm32mpex->fThunk) {

        if (lpwm32mpex->lParam) {

            // be sure allocation size matches stackfree16() size below
            vpRect16 = (VPVOID)stackalloc16(sizeof(RECT16));

            if(vpRect16) {
                putrect16(vpRect16, (LPRECT)lpwm32mpex->lParam);

                lpwm32mpex->Parm16.WndProc.lParam = (LONG)vpRect16;
            }
        }
    }
    else {

        if (lpwm32mpex->lParam) {

            vpRect16 = (VPVOID)lpwm32mpex->Parm16.WndProc.lParam;

            if(vpRect16) {
                getrect16(vpRect16, (LPRECT)lpwm32mpex->lParam);

                stackfree16(vpRect16, sizeof(RECT16));
            }
        }
    }

    return (TRUE);
}






// This function thunks the messages,
//
//  WM_NCCALCSIZE
//

BOOL FASTCALL WM32NCCalcSize(LPWM32MSGPARAMEX lpwm32mpex)
{
    PNCCALCSIZE_PARAMS16 pnc16;
    PNCCALCSIZE_PARAMS16 lpnc16;
    VPWINDOWPOS16        vpwp16;
    LPNCCALCSIZE_PARAMS  lParam = (LPNCCALCSIZE_PARAMS)lpwm32mpex->lParam;
    UINT                 cb;
    VPVOID               vp;


    // lpwm32mpex->uParam == TRUE ?  (lParam is LPNCCALCSIZE_PARAMS) : (lParam is LPRECT);
    //

    if (lpwm32mpex->fThunk) {
        if (lParam) {
            if (lpwm32mpex->uParam)
                cb = sizeof(NCCALCSIZE_PARAMS16) + sizeof(WINDOWPOS16);
            else
                cb = sizeof(RECT16);

            // be sure allocation size matches stackfree16() size below
            lpwm32mpex->dwTmp[0] = cb;
            vp = (VPVOID)stackalloc16(cb);

            lpwm32mpex->Parm16.WndProc.lParam = (LONG)vp;

            putrect16((VPRECT16)vp, (LPRECT)lParam);
            if (lpwm32mpex->uParam) {
                pnc16 = (PNCCALCSIZE_PARAMS16)vp;
                putrect16((VPRECT16)(&pnc16->rgrc[1]), &lParam->rgrc[1]);
                putrect16((VPRECT16)(&pnc16->rgrc[2]), &lParam->rgrc[2]);

                GETVDMPTR( pnc16, sizeof(NCCALCSIZE_PARAMS16), lpnc16 );

                vpwp16 = (VPWINDOWPOS16)(pnc16+1);
                lpnc16->lppos = (PWINDOWPOS16)vpwp16;

                FREEVDMPTR( lpnc16 );

                putwindowpos16( vpwp16, lParam->lppos );

            }
        }
    }
    else {
        vp = (VPVOID)lpwm32mpex->Parm16.WndProc.lParam;
        getrect16((VPRECT16)vp, (LPRECT)lParam);
        if (lpwm32mpex->uParam) {
            pnc16 = (PNCCALCSIZE_PARAMS16)vp;

            getrect16((VPRECT16)(&pnc16->rgrc[1]), &lParam->rgrc[1]);
            getrect16((VPRECT16)(&pnc16->rgrc[2]), &lParam->rgrc[2]);

            GETVDMPTR( pnc16, sizeof(NCCALCSIZE_PARAMS16), lpnc16 );

            vpwp16 = (VPWINDOWPOS16)lpnc16->lppos;

            FREEVDMPTR( lpnc16 );

            getwindowpos16( vpwp16, lParam->lppos );


        }
        if(vp) {
            stackfree16(vp, ((UINT)lpwm32mpex->dwTmp[0]));
        }
    }

    return (TRUE);
}


// This function thunks the messages,
//
//  WM_COMMAND
//

BOOL FASTCALL WM32Command(LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
            // it's from a control
        HIW(lpwm32mpex->Parm16.WndProc.lParam) = HIWORD(lpwm32mpex->uParam);
        LOW(lpwm32mpex->Parm16.WndProc.lParam) = GETHWND16(lpwm32mpex->lParam);
    }

    return (TRUE);
}


// This function thunks the messages,
//
//  WM_TIMER
//

BOOL FASTCALL WM32Timer(LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {

        /*
        ** map the timer number and the timer proc address (cause its easy)
        */
        PTMR ptmr;

        ptmr = FindTimer32((HAND16)GETHWND16(lpwm32mpex->hwnd), lpwm32mpex->uParam);

        if ( !ptmr ) {
            /*
            ** Edit controls create their own timer, which can safely be
            ** thunked to itself.
            */
            if ( lpwm32mpex->lParam || HIWORD(lpwm32mpex->uParam) ) {
                LOGDEBUG(LOG_WARNING,("  WM32Timer ERROR: cannot find timer %08x\n", lpwm32mpex->uParam));
            }
            return TRUE;
        }

        lpwm32mpex->Parm16.WndProc.lParam = ptmr->vpfnTimerProc;
    }

    return (TRUE);
}




// This function thunks the messages,
//
//  WM_HSCROLL
//  WM_VSCROLL
//

BOOL FASTCALL WM32HScroll(LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        LOW(lpwm32mpex->Parm16.WndProc.lParam) = HIWORD(lpwm32mpex->uParam);
        HIW(lpwm32mpex->Parm16.WndProc.lParam) = GETHWND16(lpwm32mpex->lParam);
    }

    return (TRUE);
}


// This function thunks the messages,
//
//  WM_INITMENU
//  WM_INITMENUPOPUP
//

BOOL FASTCALL WM32InitMenu(LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wParam = GETHMENU16(lpwm32mpex->uParam);
    }

    return (TRUE);
}


// This function thunks the messages,
//
//  WM_MENUSELECT
//

BOOL FASTCALL WM32MenuSelect(LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {

        // Copy the menu flags
        LOW(lpwm32mpex->Parm16.WndProc.lParam) = HIWORD(lpwm32mpex->uParam);

        // Copy the "main" menu
        HIW(lpwm32mpex->Parm16.WndProc.lParam) = GETHMENU16(lpwm32mpex->lParam);

        if (HIWORD(lpwm32mpex->uParam) == 0xFFFF || !(HIWORD(lpwm32mpex->uParam) & MF_POPUP)) {
            lpwm32mpex->Parm16.WndProc.wParam = LOWORD(lpwm32mpex->uParam);       // Its an ID
        }
        else {
            // convert menu index into menu handle
            lpwm32mpex->Parm16.WndProc.wParam = GETHMENU16(GetSubMenu((HMENU)lpwm32mpex->lParam, LOWORD(lpwm32mpex->uParam)));
        }
    }

    return (TRUE);
}





// This function thunks the messages,
//
//  WM_MENUCHAR
//

BOOL FASTCALL WM32MenuChar(LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        LOW(lpwm32mpex->Parm16.WndProc.lParam) = HIWORD(lpwm32mpex->uParam);
        HIW(lpwm32mpex->Parm16.WndProc.lParam) = GETHMENU16(lpwm32mpex->lParam);
    }

    return (TRUE);
}


// This function thunks the messages,
//
//  WM_ENTERIDLE
//

BOOL FASTCALL WM32EnterIdle(LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        if ((lpwm32mpex->uParam == MSGF_DIALOGBOX) || (lpwm32mpex->uParam == MSGF_MENU)) {
            LOW(lpwm32mpex->Parm16.WndProc.lParam) = GETHWND16(lpwm32mpex->lParam);
            HIW(lpwm32mpex->Parm16.WndProc.lParam) = 0;
        }
        else {
            LOGDEBUG(LOG_ALWAYS,(" WOW::WM_ENTERIDLE: wParam has unknown value, wParam=%08x, Contact ChandanC\n", lpwm32mpex->uParam));
        }
    }
    return (TRUE);
}


// This function thunks the messages,
//
//  WM_PARENTNOTIFY
//

BOOL FASTCALL WM32ParentNotify(LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        if ((LOWORD(lpwm32mpex->uParam) == WM_CREATE) || (LOWORD(lpwm32mpex->uParam) == WM_DESTROY)) {
            HIW(lpwm32mpex->Parm16.WndProc.lParam) = HIWORD(lpwm32mpex->uParam);
            LOW(lpwm32mpex->Parm16.WndProc.lParam) = GETHWND16(lpwm32mpex->lParam);
        }
    }

    return (TRUE);
}


// This function thunks the messages,
//
//  WM_MDICreate
//

BOOL FASTCALL WM32MDICreate(LPWM32MSGPARAMEX lpwm32mpex)
{
    INT cb;
    VPVOID vp;
    register PMDICREATESTRUCT16 pmcs16;
    LPMDICREATESTRUCT lParam = (LPMDICREATESTRUCT) lpwm32mpex->lParam;


    if (lpwm32mpex->fThunk) {
        if (lParam) {

            lpwm32mpex->dwParam = (DWORD)0;
            if (lParam->szClass) {
                if ( HIWORD(lParam->szClass) == 0 ) {
                    vp = (VPVOID)lParam->szClass;
                }
                else {
                    cb = strlen(lParam->szClass)+1;
                    if (!(vp = malloc16(cb)))
                        goto Error;
                    putstr16(vp, lParam->szClass, cb);
                }
            }
            else {
                vp = (VPVOID)NULL;
            }

            //
            // pfs:windowsworks overwrite pszclass, so we need to save the
            // so that we can free the memory we just alloced
            //
            lpwm32mpex->dwParam = (DWORD)vp;

            if (lParam->szTitle) {
                cb = strlen(lParam->szTitle)+1;
                if (!(vp = malloc16(cb)))
                    goto Error;
                putstr16(vp, lParam->szTitle, cb);
            }
            else {
                vp = (VPVOID)NULL;
            }

            // BUGBUG -- The assumption here is that GlobalAlloc will never
            // return a memory object that isn't word-aligned, so that we can
            // assign word-aligned words directly;  we have no idea whether the
            // memory is dword-aligned or not however, so dwords must always
            // be paranoidly stored with the STOREDWORD/STORELONG macros -JTP

            if (!(lpwm32mpex->Parm16.WndProc.lParam = malloc16(sizeof(MDICREATESTRUCT16))))
                goto Error;

            GETVDMPTR(lpwm32mpex->Parm16.WndProc.lParam, sizeof(MDICREATESTRUCT16), pmcs16);
            STOREDWORD(pmcs16->vpszClass, lpwm32mpex->dwParam);
            STOREDWORD(pmcs16->vpszTitle, vp);
            pmcs16->hOwner  = GETHINST16(lParam->hOwner);
            pmcs16->x       = (SHORT)lParam->x;
            pmcs16->y       = (SHORT)lParam->y;
            pmcs16->cx      = (SHORT)lParam->cx;
            pmcs16->cy      = (SHORT)lParam->cy;
            STORELONG(pmcs16->style, lParam->style);
            STORELONG(pmcs16->lParam, lParam->lParam);

            FLUSHVDMPTR(lpwm32mpex->Parm16.WndProc.lParam, sizeof(MDICREATESTRUCT16), pmcs16);
            FREEVDMPTR(pmcs16);

            return (TRUE);

          Error:
            LOGDEBUG(LOG_ALWAYS,(" !!!! WM32MDICreate, WM_MDICREATE thunking failed !!!! Window %08lX ", lpwm32mpex->hwnd));
            if (HIW16(lpwm32mpex->dwParam)) free16(lpwm32mpex->dwParam);
            if (vp)                         free16(vp);
            return FALSE;
        }
    }
    else {
        if (lpwm32mpex->Parm16.WndProc.lParam) {
            GETVDMPTR(lpwm32mpex->Parm16.WndProc.lParam, sizeof(MDICREATESTRUCT16), pmcs16);

            if (FETCHDWORD(pmcs16->vpszTitle)) {
                free16(FETCHDWORD(pmcs16->vpszTitle));
            }

            FREEVDMPTR(pmcs16);

            //  if HIWORD(class) is zero, class is an atom, else a pointer.

            if (HIW16(lpwm32mpex->dwParam)) {
                free16(lpwm32mpex->dwParam);
            }


            lpwm32mpex->lReturn = (LONG)HWND32(LOWORD(lpwm32mpex->lReturn));
            free16((VPVOID) lpwm32mpex->Parm16.WndProc.lParam);
        }
    }

    return (TRUE);
}


// This function thunks the messages,
//
//  WM_MDIActivate
//

BOOL FASTCALL WM32MDIActivate(LPWM32MSGPARAMEX lpwm32mpex)
{
    BOOL fHwndIsMdiChild;


    if (lpwm32mpex->fThunk) {

        // the format of the message is different based on the window that's
        // receiving the message. If 'hwnd' is a MdiClient window it is of one
        // form and if 'hwnd' is MdiChild it is of another form. We need to
        // distinguish between the formats to correctly thunk the message.
        //
        // NOTE: we donot make calls like GetClassName because they are
        //       expensive and also I think we came across a case where a
        //       window of 'wow private class' processes these messages
        //
        //                                                - Nanduri

        if (lpwm32mpex->lParam) {

            // lParam != NULL. The message is definitely going to a MdiChild.
            //

            fHwndIsMdiChild = TRUE;
        }
        else {

            // lParam == NULL, doesnot necessarily mean that the message is
            // going to a MdiClient window. So distinguish...

            if (lpwm32mpex->uParam && (GETHWND16(lpwm32mpex->hwnd) ==
                    GETHWND16(lpwm32mpex->uParam))) {

                // if hwnd is same as uParam then definitely hwnd is a MdiChild
                // window. (because if hwnd is a MdiClient then uParam will be
                // a MdiChild and thus they will not be equal)

                fHwndIsMdiChild = TRUE;
            }
            else {
                fHwndIsMdiChild = FALSE;
            }

        }

        if (fHwndIsMdiChild) {
            lpwm32mpex->Parm16.WndProc.wParam =
                    (WORD)(GETHWND16(lpwm32mpex->hwnd) == GETHWND16(lpwm32mpex->lParam));
            LOW(lpwm32mpex->Parm16.WndProc.lParam) = GETHWND16(lpwm32mpex->lParam);
            HIW(lpwm32mpex->Parm16.WndProc.lParam) = GETHWND16(lpwm32mpex->uParam);
        } else {
            lpwm32mpex->Parm16.WndProc.wParam = GETHWND16(lpwm32mpex->uParam);
            lpwm32mpex->Parm16.WndProc.lParam = 0;
        }
    }

    return (TRUE);
}


// This function thunks the messages,
//
//  WM_MDIGETACTIVE
//

BOOL FASTCALL WM32MDIGetActive(LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.lParam = 0;
    }
    else {

        if (lpwm32mpex->lParam != 0)
            *((LPBOOL)lpwm32mpex->lParam) = (BOOL)HIWORD(lpwm32mpex->lReturn);

        lpwm32mpex->lReturn = (LONG)HWND32(LOWORD(lpwm32mpex->lReturn));
    }

    return (TRUE);
}


// This function thunks the messages,
//
//  WM_MDISETMENU
//

BOOL FASTCALL WM32MDISetMenu(LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        if (lpwm32mpex->uMsg == WM_MDIREFRESHMENU) {
            lpwm32mpex->Parm16.WndProc.wParam = TRUE;
            lpwm32mpex->Parm16.WndProc.wMsg = WM_MDISETMENU;
        }
        else {
            lpwm32mpex->Parm16.WndProc.wParam = 0;
        }
        LOW(lpwm32mpex->Parm16.WndProc.lParam) = GETHMENU16(lpwm32mpex->uParam);
        HIW(lpwm32mpex->Parm16.WndProc.lParam) = GETHMENU16(lpwm32mpex->lParam);
    }
    else {
        lpwm32mpex->lReturn = (LONG)HMENU32(lpwm32mpex->lReturn);
    }

    return (TRUE);
}


// This function thunks the messages,
//
//  WM_SIZECLIPBOARD
//  WM_PAINTCLIPBOARD
//


BOOL FASTCALL WM32SizeClipBoard(LPWM32MSGPARAMEX lpwm32mpex)
{
    HAND16 hMem16 = 0;
    VPVOID  vp;
    LPRECT  lp;



    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wParam = GETHWND16(lpwm32mpex->uParam);

        vp = GlobalAllocLock16(GMEM_MOVEABLE, (lpwm32mpex->uMsg == WM_SIZECLIPBOARD) ?
                               sizeof(RECT) : sizeof(PAINTSTRUCT),  &hMem16);
        if (vp) {
            if (lp = (LPRECT) GlobalLock((HANDLE) lpwm32mpex->lParam)) {
                if (lpwm32mpex->uMsg == WM_SIZECLIPBOARD) {
                    PUTRECT16(vp, lp);
                }
                else {
                    putpaintstruct16(vp, (LPPAINTSTRUCT) lp);
                }
                GlobalUnlock((HANDLE) lpwm32mpex->lParam);
            }
            else {
                LOGDEBUG(LOG_ALWAYS, ("WOW::WM32SizeClipboard: Couldn't lock 32 bit memory handle!\n"));
                // WOW32ASSERT (FALSE);
            }

            GlobalUnlock16(hMem16);
        }
        else {
            hMem16 = 0;
            LOGDEBUG(LOG_ALWAYS, ("WOW::WM32SizeClipboard: Couldn't allocate memory !\n"));
            WOW32ASSERT (FALSE);
        }

        LOW(lpwm32mpex->Parm16.WndProc.lParam) = (WORD) hMem16;
    }
    else {
        if (LOW(lpwm32mpex->Parm16.WndProc.lParam)) {
            GlobalUnlockFree16(GlobalLock16(LOW(lpwm32mpex->Parm16.WndProc.lParam), NULL));
        }
    }

    return (TRUE);
}



// This function thunks the messages,
//
//  WM_ASKCBFORMATNAME
//


BOOL FASTCALL WM32AskCBFormatName(LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.lParam = malloc16(lpwm32mpex->Parm16.WndProc.wParam);
        if (lpwm32mpex->Parm16.WndProc.lParam) {
            putstr16((VPSZ)lpwm32mpex->Parm16.WndProc.lParam, (LPSZ)lpwm32mpex->lParam, lpwm32mpex->uParam);
        }

        return (BOOL)lpwm32mpex->Parm16.WndProc.lParam;
    }
    else {
        if (lpwm32mpex->Parm16.WndProc.lParam) {
            getstr16((VPSZ)lpwm32mpex->Parm16.WndProc.lParam, (LPSZ)lpwm32mpex->lParam, lpwm32mpex->uParam);
            free16((VPVOID) lpwm32mpex->Parm16.WndProc.lParam);
        }
    }

    return (TRUE);
}


// This function thunks the messages,
//
//  WM_CHANGECBCHAIN
//

BOOL FASTCALL WM32ChangeCBChain(LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wParam = GETHWND16(lpwm32mpex->uParam);
        lpwm32mpex->Parm16.WndProc.lParam = GETHWND16(lpwm32mpex->lParam);
    }

    return (TRUE);
}



// This function thunks the messages,
//
//  WM_DDEINITIATE
//

BOOL FASTCALL WM32DDEInitiate(LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wParam = GETHWND16(lpwm32mpex->uParam);
        lpwm32mpex->Parm16.WndProc.lParam = lpwm32mpex->lParam;
        WI32DDEAddInitiator(lpwm32mpex->Parm16.WndProc.wParam);
    }
    else {
        WI32DDEDeleteInitiator((HAND16)GETHWND16(lpwm32mpex->uParam));
    }

    return (TRUE);
}


// This function thunks the messages,
//
//  WM_DDEACK
//

BOOL FASTCALL WM32DDEAck(LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wParam = GETHWND16(lpwm32mpex->uParam);

        if (WI32DDEInitiate((HAND16)GETHWND16(lpwm32mpex->hwnd))) {
            //
            // Initiate ACK
            //
            lpwm32mpex->Parm16.WndProc.lParam = lpwm32mpex->lParam;
        }
        else {
            //
            // NON-Initiate ACK
            //

            UINT    lLo = 0;
            UINT    lHi = 0;
            PHDDE   pDdeNode;

            UnpackDDElParam(lpwm32mpex->uMsg, lpwm32mpex->lParam, &lLo, &lHi);

            if (!HIWORD(lHi)) {
                //
                // NON-Execute ACK
                //
                HIW(lpwm32mpex->Parm16.WndProc.lParam) = (WORD) lHi;
            }
            else {
                //
                // Execute ACK
                //

                //
                // The order of To_hwnd and From_hwnd is reversed in the following
                // DDEFirstPair16(), below. This is done to locate the h32.
                //

                pDdeNode = DDEFindAckNode ((HAND16)lpwm32mpex->Parm16.WndProc.wParam,
                                           (HAND16)GETHWND16(lpwm32mpex->hwnd),
                                           (HANDLE) lHi);

                if (!pDdeNode) {

                    //
                    // When ShellExecute does DDE_EXECUTE to open a document,
                    // we don't see its Win32 PostMessage calls so we have no
                    // record of the conversation.  This is our first opportunity
                    // to rectify that, the context of WU32GetMessage thunking
                    // a WM_DDE_ACK message.  We could also get here for other
                    // kinds of ACKs, fortunately the Win32 message alone gives
                    // enough context to distinguish the various flavors, unlike
                    // the Win16 WM_DDE_ACK.
                    //

                    if (lpwm32mpex->lParam >= 0xc0000000) {

                        //
                        // ack responding to initiate
                        //

                        lpwm32mpex->Parm16.WndProc.lParam = lpwm32mpex->lParam;
                    }

                    if (lHi > 0xffff) {

                        //
                        // ack responding to execute: global handle in hiword
                        //

                        HAND16 h16 = 0;
                        DWORD cb;
                        VPVOID vp;
                        LPBYTE lpMem16, lpMem32;
                        DDEINFO DdeInfo;

                        if (!lpwm32mpex->fFree) {
                            cb = GlobalSize((HANDLE)lHi);
                            vp = GlobalAllocLock16(GMEM_DDESHARE, cb, &h16);
                            if (vp) {
                                GETMISCPTR(vp, lpMem16);
                                lpMem32 = GlobalLock((HANDLE)lHi);
                                RtlCopyMemory(lpMem16, lpMem32, cb);
                                GlobalUnlock((HANDLE)lHi);
                                GlobalUnlock16(h16);
                                FREEMISCPTR(lpMem16);

                                DdeInfo.Msg = WM_DDE_EXECUTE;
                                DdeInfo.Format = 0;
                                DdeInfo.Flags = DDE_PACKET;
                                DdeInfo.h16 = 0;

                                DDEAddhandle(
                                    lpwm32mpex->Parm16.WndProc.wParam,
                                    GETHWND16(lpwm32mpex->hwnd),
                                    h16,
                                    (HANDLE)lHi,
                                    &DdeInfo
                                    );

                                pDdeNode = DDEFindAckNode (
                                           (HAND16)lpwm32mpex->Parm16.WndProc.wParam,
                                           (HAND16)GETHWND16(lpwm32mpex->hwnd),
                                           (HANDLE) lHi);

                                WOW32ASSERTMSG(pDdeNode, "WM32DDEAck: Can't find just-added DDE node.\n");
                            }
                        }

                        lpwm32mpex->Parm16.WndProc.lParam = MAKELONG(LOWORD(lLo), h16);

                    } else {

                        //
                        // All other acks have same form: status in loword and
                        // item atom in hiword.
                        //

                        lpwm32mpex->Parm16.WndProc.lParam = MAKELONG(LOWORD(lLo), lHi);
                    }

                }

                if (pDdeNode && pDdeNode->DdeMsg == WM_DDE_EXECUTE) {

                    HIW(lpwm32mpex->Parm16.WndProc.lParam) = pDdeNode->hMem16;

                    if (lpwm32mpex->fFree) {
                        if (lHi) {
                            if (pDdeNode->DdeFlags & DDE_EXECUTE_FREE_MEM) {
                                LOGDEBUG (12, ("WOW::W32DDEAck : Freeing EXECUTE pair h16 = %04x, h32 = %08x\n",
                                                                    pDdeNode->hMem16, lHi));
                                W32UnMarkDDEHandle (pDdeNode->hMem16);
                                GlobalUnlockFree16(GlobalLock16(pDdeNode->hMem16, NULL));
                                if (DDEDeletehandle(pDdeNode->hMem16, (HANDLE) lHi)) {
                                    WOWGLOBALFREE((HANDLE)lHi);
                                }
                                else {
                                    LOGDEBUG (0, ("WOW::DDE Ack : Ack can't find 16 - 32 aliasing :  %04x, %04x, %04x, %08lx, %08lx\n",
                                                  lpwm32mpex->hwnd,
                                                  lpwm32mpex->uMsg,
                                                  lpwm32mpex->Parm16.WndProc.wParam,
                                                  lpwm32mpex->Parm16.WndProc.lParam,
                                                  lHi
                                                  ));
                                }
                            }
                            else {
                                if (pDdeNode->DdeFlags & DDE_EXECUTE_FREE_H16) {
                                    W32UnMarkDDEHandle (pDdeNode->hMem16);
                                    GlobalUnlockFree16(GlobalLock16(pDdeNode->hMem16, NULL));

                                    HIW(lpwm32mpex->Parm16.WndProc.lParam) = pDdeNode->h16;
                                }

                                if (DDEDeletehandle(pDdeNode->hMem16, (HANDLE) lHi)) {
                                    WOWGLOBALFREE((HANDLE)lHi);
                                }
                                else {
                                    LOGDEBUG (0, ("WOW::DDE Ack : Ack can't find 16 - 32 aliasing :  %04x, %04x, %04x, %08lx, %08lx\n",
                                                  lpwm32mpex->hwnd,
                                                  lpwm32mpex->uMsg,
                                                  lpwm32mpex->Parm16.WndProc.wParam,
                                                  lpwm32mpex->Parm16.WndProc.lParam,
                                                  lHi
                                                  ));
                                }


                            }
                        }
                        else {
                            LOGDEBUG (2, ("WOW::W32DDEAck : h32 is NULL \n"));
                            WOW32ASSERT (FALSE);
                        }
                    }
                }
                else {
                    LOGDEBUG (2, ("WOW::DDE Ack : Ack received unexpectedly :  %x, %04x, %04x, %08lx, %08lx\n", lpwm32mpex->hwnd, lpwm32mpex->uMsg, lpwm32mpex->Parm16.WndProc.wParam, lpwm32mpex->Parm16.WndProc.lParam, lHi));
                }
            }

            LOW(lpwm32mpex->Parm16.WndProc.lParam) = (WORD) lLo;

            if (fThunkDDEmsg) {
                FreeDDElParam(lpwm32mpex->uMsg, lpwm32mpex->lParam);
            }

            LOGDEBUG (12, ("WOW::DDE Ack : %04x, %04x, %04x, %08lx, %08lx\n", lpwm32mpex->hwnd, lpwm32mpex->uMsg, lpwm32mpex->Parm16.WndProc.wParam, lpwm32mpex->Parm16.WndProc.lParam, lHi));
        }
    }
    else {
        //
        // We will execute this scenario only if the app ate the message,
        // because we need to free up the memory.
        //

        if (!fThunkDDEmsg) {
            if (lpwm32mpex->lReturn) {
                FreeDDElParam(lpwm32mpex->uMsg, lpwm32mpex->lParam);
            }
        }
    }

    return (TRUE);
}


// This function thunks the messages,
//
//  WM_DDEREQUEST
//  WM_DDETERMINATE
//  WM_DDEUNADVISE
//

BOOL FASTCALL WM32DDERequest(LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wParam = GETHWND16(lpwm32mpex->uParam);
    }

    return (TRUE);
}


// This function thunks the messages,
//
//  WM_DDEADVISE
//

BOOL FASTCALL WM32DDEAdvise(LPWM32MSGPARAMEX lpwm32mpex)
{
    HAND16      h16;
    VPVOID      vp;
    LPBYTE      lpMem16;
    LPBYTE      lpMem32;
    UINT        lLo = 0;
    UINT        lHi = 0;
    DDEINFO     DdeInfo;


    if (lpwm32mpex->fThunk) {
        UnpackDDElParam(lpwm32mpex->uMsg, lpwm32mpex->lParam, &lLo, &lHi);
        lpwm32mpex->Parm16.WndProc.wParam = GETHWND16(lpwm32mpex->uParam);

        if (h16 = DDEFindPair16((HAND16)GETHWND16(lpwm32mpex->hwnd),
                                (HAND16)lpwm32mpex->Parm16.WndProc.wParam,
                                (HANDLE) lLo)) {
            LOW(lpwm32mpex->Parm16.WndProc.lParam) = h16;
        } else {
            vp = GlobalAllocLock16(GMEM_DDESHARE, sizeof(DDEADVISE), &h16);
            if (vp) {
                GETMISCPTR(vp, lpMem16);
                lpMem32 = GlobalLock((HANDLE) lLo);
                RtlCopyMemory(lpMem16, lpMem32, sizeof(DDEADVISE));
                GlobalUnlock((HANDLE) lLo);
                GlobalUnlock16(h16);
                DdeInfo.Msg = LOW(lpwm32mpex->uMsg);
                DdeInfo.Format = 0;
                DdeInfo.Flags = DDE_PACKET;
                DdeInfo.h16 = 0;
                DDEAddhandle((HAND16)GETHWND16(lpwm32mpex->hwnd),
                             (HAND16)lpwm32mpex->Parm16.WndProc.wParam,
                             h16,
                             (HANDLE) lLo,
                             &DdeInfo);
                LOW(lpwm32mpex->Parm16.WndProc.lParam) = h16;
            }
        }

        HIW(lpwm32mpex->Parm16.WndProc.lParam) = (WORD) lHi;

        if (fThunkDDEmsg) {
            FreeDDElParam(lpwm32mpex->uMsg, lpwm32mpex->lParam);
        }
    }
    else {
        //
        // We will execute this scenario only if the app ate the message,
        // because we need to free up the memory.
        //

        if (!fThunkDDEmsg) {
            if (lpwm32mpex->lReturn) {
                FreeDDElParam(lpwm32mpex->uMsg, lpwm32mpex->lParam);
            }
        }
    }

    return (TRUE);
}


// This function thunks the messages,
//
//  WM_DDEDATA
//

BOOL FASTCALL WM32DDEData(LPWM32MSGPARAMEX lpwm32mpex)
{
    HAND16  h16;
    UINT    lLo = 0;
    UINT    lHi = 0;
    DDEINFO DdeInfo;


    if (lpwm32mpex->fThunk) {
        UnpackDDElParam(lpwm32mpex->uMsg, lpwm32mpex->lParam, &lLo, &lHi);
        lpwm32mpex->Parm16.WndProc.wParam = GETHWND16(lpwm32mpex->uParam);

        if (!lLo) {
            LOW(lpwm32mpex->Parm16.WndProc.lParam) = 0;
        } else if (h16 = DDEFindPair16((HAND16)GETHWND16(lpwm32mpex->hwnd),
                                       (HAND16)lpwm32mpex->Parm16.WndProc.wParam,
                                       (HANDLE) lLo)) {
            LOW(lpwm32mpex->Parm16.WndProc.lParam) = h16;
        } else {
            DdeInfo.Msg = LOW(lpwm32mpex->uMsg);
            h16 = DDECopyhData16((HAND16)GETHWND16(lpwm32mpex->hwnd),
                                 (HAND16)lpwm32mpex->Parm16.WndProc.wParam,
                                 (HANDLE) lLo,
                                 &DdeInfo);

            //
            // If we could not allocate 16 bit memory, then return NULL to the
            // caller.
            //

            if (!h16) {
                if (fThunkDDEmsg) {
                    FreeDDElParam(lpwm32mpex->uMsg, lpwm32mpex->lParam);
                }

                lpwm32mpex->Parm16.WndProc.wParam = (WORD) lHi;
                lpwm32mpex->Parm16.WndProc.lParam = lLo;
                return (0);
            }


            DdeInfo.Flags = DDE_PACKET;
            DdeInfo.h16 = 0;
            DDEAddhandle((HAND16)GETHWND16(lpwm32mpex->hwnd),
                         (HAND16)lpwm32mpex->Parm16.WndProc.wParam,
                         h16,
                         (HANDLE) lLo,
                         &DdeInfo);

            LOW(lpwm32mpex->Parm16.WndProc.lParam) = h16;
        }

        HIW(lpwm32mpex->Parm16.WndProc.lParam) = (WORD) lHi;

        if (fThunkDDEmsg) {
            FreeDDElParam(lpwm32mpex->uMsg, lpwm32mpex->lParam);
        }


    }
    else {

        //
        // We will execute this scenario only if the app ate the message,
        // because we need to free up the memory.
        //

        if (!fThunkDDEmsg) {
            if (lpwm32mpex->lReturn) {
                FreeDDElParam(lpwm32mpex->uMsg, lpwm32mpex->lParam);
            }
        }
    }

    return (TRUE);
}


// This function thunks the messages,
//
//  WM_POKE
//

BOOL FASTCALL WM32DDEPoke(LPWM32MSGPARAMEX lpwm32mpex)
{

    HAND16  h16;
    UINT    lLo = 0;
    UINT    lHi = 0;
    DDEINFO DdeInfo;


    if (lpwm32mpex->fThunk) {
        UnpackDDElParam(lpwm32mpex->uMsg, lpwm32mpex->lParam, &lLo, &lHi);
        lpwm32mpex->Parm16.WndProc.wParam = GETHWND16(lpwm32mpex->uParam);

        // sudeepb 03-Apr-1996
        // House Design Gold Edition sends a DDE_POKE message with lParam
        // being 0. We are suppose to thunk this message with lParam being
        // zero. Without this check, the below code will fail this call
        // and the message will not be thunked to the app.

        if (lLo == 0) {
            LOW(lpwm32mpex->Parm16.WndProc.lParam) = 0;
            HIW(lpwm32mpex->Parm16.WndProc.lParam) = (WORD) lHi;
            return (TRUE);
        }

        if (h16 = DDEFindPair16((HAND16)GETHWND16(lpwm32mpex->hwnd),
                                (HAND16)lpwm32mpex->Parm16.WndProc.wParam,
                                (HANDLE) lLo)) {
            LOW(lpwm32mpex->Parm16.WndProc.lParam) = h16;
        } else {
            DdeInfo.Msg = LOW(lpwm32mpex->uMsg);
            h16 = DDECopyhData16((HAND16)GETHWND16(lpwm32mpex->hwnd),
                                 (HAND16)lpwm32mpex->Parm16.WndProc.wParam,
                                 (HANDLE) lLo,
                                 &DdeInfo);


            //
            // If we could not allocate 16 bit memory, then return NULL to the
            // caller.
            //

            if (!h16) {
                if (fThunkDDEmsg) {
                    FreeDDElParam(lpwm32mpex->uMsg, lpwm32mpex->lParam);
                }

                lpwm32mpex->Parm16.WndProc.lParam = lLo;
                return (0);
            }

            DdeInfo.Flags = DDE_PACKET;
            DdeInfo.h16 = 0;
            DDEAddhandle((HAND16)GETHWND16(lpwm32mpex->hwnd),
                         (HAND16)lpwm32mpex->Parm16.WndProc.wParam,
                         h16,
                         (HANDLE) lLo,
                         &DdeInfo);

            LOW(lpwm32mpex->Parm16.WndProc.lParam) = h16;
        }

        HIW(lpwm32mpex->Parm16.WndProc.lParam) = (WORD) lHi;

        if (fThunkDDEmsg) {
            FreeDDElParam(lpwm32mpex->uMsg, lpwm32mpex->lParam);
        }
    }
    else {
        //
        // We will execute this scenario only if the app ate the message,
        // because we need to free up the memory.
        //

        if (!fThunkDDEmsg) {
            if (lpwm32mpex->lReturn) {
                FreeDDElParam(lpwm32mpex->uMsg, lpwm32mpex->lParam);
            }
        }
    }

    return (TRUE);
}


// This function thunks the messages,
//
//  WM_DDE_EXECUTE
//

BOOL FASTCALL WM32DDEExecute(LPWM32MSGPARAMEX lpwm32mpex)
{

    HAND16  h16;
    VPVOID  vp;
    LPBYTE  lpMem16;
    LPBYTE  lpMem32;
    DDEINFO DdeInfo;


    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wParam = GETHWND16(lpwm32mpex->uParam);

        if (h16 = DDEFindPair16((HAND16)GETHWND16(lpwm32mpex->hwnd),
                                (HAND16)lpwm32mpex->Parm16.WndProc.wParam,
                                (HANDLE) lpwm32mpex->lParam)) {
            HIW(lpwm32mpex->Parm16.WndProc.lParam) = h16;
        } else {
            vp = GlobalAllocLock16(GMEM_DDESHARE, GlobalSize((HANDLE) lpwm32mpex->lParam), &h16);
            if (vp) {
                GETMISCPTR(vp, lpMem16);
                lpMem32 = GlobalLock((HANDLE) lpwm32mpex->lParam);
                RtlCopyMemory(lpMem16, lpMem32, GlobalSize((HANDLE) lpwm32mpex->lParam));
                GlobalUnlock((HANDLE) lpwm32mpex->lParam);
                GlobalUnlock16(h16);

                DdeInfo.Msg = LOW(lpwm32mpex->uMsg);
                DdeInfo.Format = 0;
                DdeInfo.Flags = DDE_PACKET;
                DdeInfo.h16 = 0;
                DDEAddhandle((HAND16)GETHWND16(lpwm32mpex->hwnd),
                             (HAND16)lpwm32mpex->Parm16.WndProc.wParam,
                             h16,
                             (HANDLE) lpwm32mpex->lParam,
                             &DdeInfo);

                HIW(lpwm32mpex->Parm16.WndProc.lParam) = h16;
            }
        }
        LOW(lpwm32mpex->Parm16.WndProc.lParam) = 0;
    }

    return (TRUE);
}


// This function thunks the messages,
//
//  WM_CTLCOLORMSGBOX
//  WM_CTLCOLOREDIT
//  WM_CTLCOLORLISTBOX
//  WM_CTLCOLORBTN
//  WM_CTLCOLORDLG
//  WM_CTLCOLORSCROLLBAR
//  WM_CTLCOLORSTATIC
//
// into WM_CTLCOLOR and the high word of lParam specifies the
// control type.
//

BOOL FASTCALL WM32CtlColor(LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wMsg = WM_CTLCOLOR;
        if(lpwm32mpex->uMsg != WM_CTLCOLOR) {  // see 16-bit thunk for this special case
            lpwm32mpex->Parm16.WndProc.wParam = GETHDC16(lpwm32mpex->uParam);
            LOW(lpwm32mpex->Parm16.WndProc.lParam) = GETHWND16(lpwm32mpex->lParam);
            HIW(lpwm32mpex->Parm16.WndProc.lParam) = (WORD) (lpwm32mpex->uMsg - WM_CTLCOLORMSGBOX);
        }
    }
    else {
        if ((ULONG)lpwm32mpex->lReturn > COLOR_ENDCOLORS) {
            lpwm32mpex->lReturn = (LONG) HBRUSH32(lpwm32mpex->lReturn);
        }
    }

    return (TRUE);
}


// This function thunks the messages,
//
//  WM_GETFONT
//

BOOL FASTCALL WM32GetFont(LPWM32MSGPARAMEX lpwm32mpex)
{

    if (!lpwm32mpex->fThunk) {
        lpwm32mpex->lReturn = (LONG)HFONT32(lpwm32mpex->lReturn);
    }

    return (TRUE);
}


// This function thunks the messages,
//
// WM_NEXTMENU
//
//           Win16          NT
//  wParam   VK_KEY         VK_KEY
//  lParam.l hmenu          PMDINEXTMENU
//  lParam.h 0
//  return.l menu           BOOL
//  return.h window
//


BOOL FASTCALL WM32NextMenu(LPWM32MSGPARAMEX lpwm32mpex)
{

    if (lpwm32mpex->fThunk) {
        if (lpwm32mpex->lParam) {
            LOW(lpwm32mpex->Parm16.WndProc.lParam) = GETHMENU16(((PMDINEXTMENU)lpwm32mpex->lParam)->hmenuIn);
            HIW(lpwm32mpex->Parm16.WndProc.lParam) = 0;
        }
    } else {
        if (lpwm32mpex->lParam) {
            ((PMDINEXTMENU)lpwm32mpex->lParam)->hmenuNext = HMENU32(LOWORD(lpwm32mpex->lReturn));
            ((PMDINEXTMENU)lpwm32mpex->lParam)->hwndNext = HWND32(HIWORD(lpwm32mpex->lReturn));
            lpwm32mpex->lReturn = TRUE;
        } else {
            lpwm32mpex->lReturn = FALSE;
        }
    }

    return (TRUE);
}


BOOL FASTCALL WM32Destroy (LPWM32MSGPARAMEX lpwm32mpex)
{

    if (!lpwm32mpex->fThunk) {
        if (CACHENOTEMPTY()) {
            // because of our method of window aliasing, 'hwnd' may or may
            // not be a real 32bit handle. ie. it may be (hwnd16 | 0xffff0000).
            // So always use hwnd16.

            ReleaseCachedDCs((CURRENTPTD())->htask16, GETHWND16(lpwm32mpex->hwnd), 0,
                               (HWND)0, SRCHDC_TASK16_HWND16);
        }
    }
    return (TRUE);
}






// This function thunks the messages,
//  WM_DROPFILES

BOOL FASTCALL WM32DropFiles(LPWM32MSGPARAMEX lpwm32mpex)
{
    if (lpwm32mpex->fThunk) {
        return (BOOL)(lpwm32mpex->Parm16.WndProc.wParam = GETHDROP16(lpwm32mpex->uParam));
    }

    return (TRUE);
}






// This function thunks the messages,
//  WM_PRINT
//  WM_PRINTCLIENT

BOOL FASTCALL WM32PrintClient(LPWM32MSGPARAMEX lpwm32mpex)
{
    if (lpwm32mpex->fThunk) {
        return (BOOL)(lpwm32mpex->Parm16.WndProc.wParam = GETHDC16(lpwm32mpex->uParam));
    }

    return (TRUE);
}




// This function thunks the messages,
//
//  WM_DROPOBJECT
//  WM_QUERYDROPOBJECT
//  WM_DRAGLOOP
//  WM_DRAGSELECT
//  WM_DRAGMOVE
//

BOOL FASTCALL WM32DropObject(LPWM32MSGPARAMEX lpwm32mpex)
{
    register PDROPSTRUCT16 pds16;
    register LPDROPSTRUCT  lParam = (LPDROPSTRUCT)lpwm32mpex->lParam;

    if (lpwm32mpex->fThunk) {

        lpwm32mpex->Parm16.WndProc.wParam = (WORD)lpwm32mpex->uParam;

        // BUGBUG -- The assumption here is that GlobalAlloc will never
        // return a memory object that isn't word-aligned, so that we can
        // assign word-aligned words directly;  we have no idea whether the
        // memory is dword-aligned or not however, so dwords must always
        // be paranoidly stored with the STOREDWORD/STORELONG macros -JTP

        if (!(lpwm32mpex->Parm16.WndProc.lParam = malloc16(sizeof(DROPSTRUCT16))))
            return FALSE;

        GETVDMPTR(lpwm32mpex->Parm16.WndProc.lParam, sizeof(DROPSTRUCT16), pds16);

        pds16->hwndSource = GETHWND16(lParam->hwndSource);
        pds16->hwndSink   = GETHWND16(lParam->hwndSink);
        pds16->wFmt       = (WORD) lParam->wFmt;
        STOREDWORD(pds16->dwData, lParam->dwData);

        pds16->ptDrop.x = (SHORT)lParam->ptDrop.x;
        pds16->ptDrop.y = (SHORT)lParam->ptDrop.y;
        STOREDWORD(pds16->dwControlData, lParam->dwControlData);

        FLUSHVDMPTR(lpwm32mpex->Parm16.WndProc.lParam, sizeof(DROPSTRUCT16), pds16);
        FREEVDMPTR(pds16);

    } else {

        free16((VPVOID) lpwm32mpex->Parm16.WndProc.lParam);

        if (lpwm32mpex->uMsg == WM_QUERYDROPOBJECT) {

            //
            // Return value is either TRUE, FALSE,
            // or a cursor!
            //
            if (lpwm32mpex->lReturn && lpwm32mpex->lReturn != (LONG)TRUE) {
                lpwm32mpex->lReturn = (LONG)HCURSOR32(lpwm32mpex->lReturn);
            }
        }
    }

    return (TRUE);
}


// This function thunks the messages,
//
//  WM_WINDOWPOSCHANGING
//  WM_WINDOWPOSCHANGED
//

BOOL FASTCALL WM32WindowPosChanging (LPWM32MSGPARAMEX lpwm32mpex)
{
    LPWINDOWPOS lParam = (LPWINDOWPOS) lpwm32mpex->lParam;


    if (lpwm32mpex->fThunk) {

        // be sure allocation size matches stackfree16() size below
        lpwm32mpex->Parm16.WndProc.lParam = stackalloc16(sizeof(WINDOWPOS16));

        putwindowpos16( (VPWINDOWPOS16)lpwm32mpex->Parm16.WndProc.lParam, lParam );

    }
    else {
        getwindowpos16( (VPWINDOWPOS16)lpwm32mpex->Parm16.WndProc.lParam, lParam );
        if(lpwm32mpex->Parm16.WndProc.lParam) {
            stackfree16((VPVOID) lpwm32mpex->Parm16.WndProc.lParam,
                        sizeof(WINDOWPOS16));
        }
    }

    return (TRUE);
}

// This function thunks the message,
//
// WM_COPYDATA
//

BOOL FASTCALL WM32CopyData (LPWM32MSGPARAMEX lpwm32mpex)
{

    HAND16  h16;
    HAND16  hMem16;
    VPVOID  vpCDS16;
    VPVOID  vpData16;
    LPBYTE  lpMem16;
    PCOPYDATASTRUCT lpCDS32;
    PCOPYDATASTRUCT lpCDS16;
    PCPDATA pTemp;


    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wParam = GETHWND16(lpwm32mpex->uParam);

        if (vpCDS16 = CopyDataFindData16 (GETHWND16(lpwm32mpex->hwnd), lpwm32mpex->Parm16.WndProc.wParam, lpwm32mpex->lParam)) {
            lpwm32mpex->Parm16.WndProc.lParam = vpCDS16;
        }
        else {
            vpCDS16 = GlobalAllocLock16(GMEM_DDESHARE, sizeof(COPYDATASTRUCT), &h16);
            if (vpCDS16) {
                GETMISCPTR(vpCDS16, lpCDS16);
                lpCDS32 = (PCOPYDATASTRUCT) lpwm32mpex->lParam;
                lpCDS16->dwData = lpCDS32->dwData;
                if (lpCDS16->cbData = lpCDS32->cbData) {

                    FREEMISCPTR(lpCDS16);

                    vpData16 = GlobalAllocLock16(GMEM_DDESHARE, lpCDS32->cbData, &hMem16);
                    GETMISCPTR(vpData16, lpMem16);
                    if (lpMem16 && lpCDS32->lpData) {
                        RtlCopyMemory(lpMem16, lpCDS32->lpData, lpCDS32->cbData);
                        CopyDataAddNode (GETHWND16(lpwm32mpex->hwnd), lpwm32mpex->Parm16.WndProc.wParam, vpData16, (DWORD) lpCDS32->lpData, 0);
                    }
                    FREEMISCPTR(lpMem16);

                    GETMISCPTR(vpCDS16, lpCDS16);
                    lpCDS16->lpData = (PVOID) vpData16;
                }
                else {
                    lpCDS16->lpData = NULL;
                }
                FREEMISCPTR(lpCDS16);
            }

            lpwm32mpex->Parm16.WndProc.lParam = vpCDS16;
            CopyDataAddNode (GETHWND16(lpwm32mpex->hwnd), lpwm32mpex->Parm16.WndProc.wParam, vpCDS16, lpwm32mpex->lParam, 0);
        }
    }
    else {
        if (lpwm32mpex->fFree) {
            pTemp = CopyDataFindData32 (GETHWND16(lpwm32mpex->hwnd), GETHWND16(lpwm32mpex->uParam), lpwm32mpex->Parm16.WndProc.lParam);
            if (pTemp && (!(pTemp->Flags))) {
                GETMISCPTR(lpwm32mpex->Parm16.WndProc.lParam, lpCDS16);
                GlobalUnlockFree16 ((VPVOID)lpCDS16->lpData);
                CopyDataDeleteNode (GETHWND16(lpwm32mpex->hwnd), lpwm32mpex->Parm16.WndProc.wParam, (DWORD) ((PCOPYDATASTRUCT)lpwm32mpex->lParam)->lpData);
                GlobalUnlockFree16 ((VPVOID)lpwm32mpex->Parm16.WndProc.lParam);
                CopyDataDeleteNode (GETHWND16(lpwm32mpex->hwnd), lpwm32mpex->Parm16.WndProc.wParam, (DWORD) lpwm32mpex->lParam);
                FREEMISCPTR(lpCDS16);
            }
        }
    }

    return (TRUE);
}

// This function thunks the message,
//
// WM_WINHELP
//

BOOL FASTCALL WM32WinHelp (LPWM32MSGPARAMEX lpwm32mpex)
{
    static WORD msgWinHelp = 0;
    if (lpwm32mpex->fThunk) {
        lpwm32mpex->Parm16.WndProc.wMsg   = msgWinHelp ? msgWinHelp : (msgWinHelp = (WORD)RegisterWindowMessage("WM_WINHELP"));
        lpwm32mpex->Parm16.WndProc.wParam = GETHWND16(lpwm32mpex->uParam);
        if (lpwm32mpex->lParam) {
            // lpwm32mpex->lParam is LPHLP - however we need only the firstword,ie the size of data

            HAND16  hMem16;
            VPVOID  vp;
            LPBYTE  lpT;
            WORD cb;

            cb = ((LPHLP)lpwm32mpex->lParam)->cbData;
            if (vp = GlobalAllocLock16(GMEM_DDESHARE | GMEM_MOVEABLE, cb, &hMem16)) {
                GETMISCPTR(vp, lpT);
                RtlCopyMemory(lpT, (PVOID)lpwm32mpex->lParam, cb);
                FREEMISCPTR(lpT);
            }
            lpwm32mpex->Parm16.WndProc.lParam = hMem16;
            lpwm32mpex->dwParam = vp;
        }
    }
    else {
        // Make sure WinHelp is in the foreground
        SetForegroundWindow(lpwm32mpex->hwnd);
        if (lpwm32mpex->Parm16.WndProc.lParam) {
            GlobalUnlockFree16((VPVOID)lpwm32mpex->dwParam);
        }
    }

    return (TRUE);
}

//
// Thunk the undocumented MM_CALCSCROLL MDI message. Message has no parameters,
// but has different message values; 32-bit msg: 0x3F, 16-bit msg: 0x10AC.
//
BOOL FASTCALL WM32MMCalcScroll (LPWM32MSGPARAMEX lpwm32mpex)
{
    if ( lpwm32mpex->fThunk ) {
        lpwm32mpex->Parm16.WndProc.wMsg = (WORD) WIN31_MM_CALCSCROLL;
    }

    return (TRUE);
}

// Calculate the size of the structure passed with WM_NOTIFY based
// on the code field in NMHDR.
// NOTE: Do NOT rely on the documentation for the size of the struct passed in
//       lParam.  In some cases the struct is actually part of a larger struct
//       and we need to copy all of it. See PSN_xxxx codes in comctl32\prsht.c
//       They are documented to be NMHDR but are really a PSHNOTIFY which has
//       NMHDR as the first field.  Also watch for some of the WIDE UNICODE
//       char cases -- the struct may or may not be the same size as the ANSI
//       version of the struct.
// Heaven help us when they start adding more of these codes!


UINT GetNMHDRextensionSize(LPNMHDR pnmhdr32)
{
#ifdef DEBUG
    char  szLabel[40] = "    WOW:WM_NOTIFY code: ";
#endif


    // Caller already checked against NM_LAST.
    if (pnmhdr32->code >= LVN_LAST) {
        LOGDEBUG(2,("%sLVN_ %x\n", szLabel, pnmhdr32->code));
        switch (pnmhdr32->code) {

            case LVN_ITEMCHANGING:
            case LVN_ITEMCHANGED:
            case LVN_INSERTITEM:
            case LVN_DELETEITEM:
            case LVN_DELETEALLITEMS:
            case LVN_COLUMNCLICK:
            case LVN_BEGINDRAG:
            case LVN_BEGINRDRAG:
            case LVN_HOTTRACK:
                return sizeof(NM_LISTVIEW);

            case LVN_BEGINLABELEDITA:
            case LVN_ENDLABELEDITA:
            case LVN_GETDISPINFOA:
            case LVN_SETDISPINFOA:
            case LVN_BEGINLABELEDITW:
            case LVN_ENDLABELEDITW:
            case LVN_GETDISPINFOW:
            case LVN_SETDISPINFOW:
                return sizeof(LV_DISPINFO);

            case LVN_KEYDOWN:
                return sizeof(LV_KEYDOWN);

            case LVN_ODCACHEHINT:
                return sizeof(NM_CACHEHINT);

            case LVN_ODFINDITEMA:
            case LVN_ODFINDITEMW:
                return sizeof(NM_FINDITEM);

            case LVN_ODSTATECHANGED:
                return sizeof(NM_ODSTATECHANGE);

            case LVN_ITEMACTIVATE:
                return sizeof(NMKEY);

            default:
                goto unknown_nmhdr_code;
        }
    }

    if (pnmhdr32->code >= PSN_LAST) {
        LOGDEBUG(2,("%sPSN_ %x\n", szLabel, pnmhdr32->code));
        switch (pnmhdr32->code) {
            case PSN_SETACTIVE:
            case PSN_KILLACTIVE:
            case PSN_APPLY:
            case PSN_RESET:
            case PSN_HELP:
            case PSN_WIZBACK:
            case PSN_WIZNEXT:
            case PSN_WIZFINISH:
            case PSN_QUERYCANCEL:
            case PSN_TRANSLATEACCELERATOR:
            case PSN_QUERYINITIALFOCUS:
            case PSN_HASHELP:          // this one "is dead" - RaymondC
                return sizeof(PSHNOTIFY);

            case PSN_GETOBJECT:
                return sizeof(NMOBJECTNOTIFY);

            case PSN_LASTCHANCEAPPLY:  // this is undocumented
                return sizeof(NMHDR);  // (in widows\inc\prshtp.h)

            default:
                goto unknown_nmhdr_code;
        }
    }

    if (pnmhdr32->code >= HDN_LAST) {
        LOGDEBUG(2,("%sHDN_ %x\n", szLabel, pnmhdr32->code));
        switch (pnmhdr32->code) {
            case HDN_ITEMCHANGINGA:
            case HDN_ITEMCHANGEDA:
            case HDN_ITEMCLICKA:
            case HDN_DIVIDERDBLCLICKA:
            case HDN_BEGINTRACKA:
            case HDN_ENDTRACKA:
            case HDN_TRACKA:
            case HDN_ITEMCHANGINGW:
            case HDN_ITEMCHANGEDW:
            case HDN_ITEMCLICKW:
            case HDN_DIVIDERDBLCLICKW:
            case HDN_BEGINTRACKW:
            case HDN_ENDTRACKW:
            case HDN_TRACKW:
            case HDN_BEGINDRAG:
            case HDN_ENDDRAG:
            case HDN_ITEMDBLCLICKA:
            case HDN_ITEMDBLCLICKW:
                return sizeof(HD_NOTIFY);

            case HDN_GETDISPINFOA:
            case HDN_GETDISPINFOW:
                return sizeof(NMHDDISPINFO);

            default:
                goto unknown_nmhdr_code;
        }
    }

    if (pnmhdr32->code >= TVN_LAST) {
        LOGDEBUG(2,("%sTVN_ %x\n", szLabel, pnmhdr32->code));
        switch (pnmhdr32->code) {
            case TVN_SELCHANGINGA:
            case TVN_SELCHANGEDA:
            case TVN_ITEMEXPANDINGA:
            case TVN_ITEMEXPANDEDA:
            case TVN_BEGINDRAGA:
            case TVN_BEGINRDRAGA:
            case TVN_DELETEITEMA:
            case TVN_SELCHANGINGW:
            case TVN_SELCHANGEDW:
            case TVN_ITEMEXPANDINGW:
            case TVN_ITEMEXPANDEDW:
            case TVN_BEGINDRAGW:
            case TVN_BEGINRDRAGW:
            case TVN_DELETEITEMW:
            case TVN_SINGLEEXPAND:
                return sizeof(NM_TREEVIEW);

            case TVN_GETDISPINFOA:
            case TVN_SETDISPINFOA:
            case TVN_BEGINLABELEDITA:
            case TVN_ENDLABELEDITA:
            case TVN_GETDISPINFOW:
            case TVN_SETDISPINFOW:
            case TVN_BEGINLABELEDITW:
            case TVN_ENDLABELEDITW:
                return sizeof(TV_DISPINFO);

            case TVN_KEYDOWN:
                return sizeof(TV_KEYDOWN);

            case TVN_GETINFOTIPA:
            case TVN_GETINFOTIPW:
                return sizeof(NMTVGETINFOTIP);

            default:
                goto unknown_nmhdr_code;
        }
    }

    if (pnmhdr32->code >= TTN_LAST) {
        LOGDEBUG(2,("%sTTN_ %x\n", szLabel, pnmhdr32->code));
        switch (pnmhdr32->code) {
            case TTN_NEEDTEXTA:        // (aka TTN_GETDISPINFO)
                return sizeof(TOOLTIPTEXTA);

            case TTN_NEEDTEXTW:
                return sizeof(TOOLTIPTEXTW);

            case TTN_SHOW:
            case TTN_POP:
                return sizeof(NMHDR);

            default:
                goto unknown_nmhdr_code;
        }
    }

    if (pnmhdr32->code >= TCN_LAST) {
        LOGDEBUG(2,("%sTCN_ %x\n", szLabel, pnmhdr32->code));
        switch (pnmhdr32->code) {
            case TCN_KEYDOWN:
                return sizeof(TC_KEYDOWN);

            case TCN_SELCHANGE:
            case TCN_SELCHANGING:
                return sizeof(NMHDR);

            default:
                goto unknown_nmhdr_code;
        }
    }

    if (pnmhdr32->code >= UDN_LAST) {
        LOGDEBUG(2,("%sUDN_ %x\n", szLabel, pnmhdr32->code));
        switch (pnmhdr32->code) {
            case UDN_DELTAPOS:
                return sizeof(NM_UPDOWN);

            default:
                goto unknown_nmhdr_code;
        }
    }

unknown_nmhdr_code:
    LOGDEBUG(LOG_ALWAYS, ("WOW:GetNMHDRextensionSize unknown nmhdr->code: %d!\n", pnmhdr32->code));
    WOW32ASSERT(FALSE);
    return sizeof(NMHDR);  // the first field of most of the structs is NMHDR
}





//
// This function thunks the 32-bit message WM_NOTIFY.  This message existed
// but was undocumented in Win3.1.  Win95 thunks it by translating lParam
// from a flat to 16:16 pointer without thunking the contents.  That's tricky
// for us since on RISC we can't map random linear memory into the VDM without
// a lot of overhead. We'll use the code field in NMHDR to calculate the size
// of the passed structure.
//
BOOL FASTCALL WM32Notify(LPWM32MSGPARAMEX lpwm32mpex)
{
    LPNMHDR pnmhdr32;
    LPNMHDR pnmhdr16;

    if (lpwm32mpex->fThunk) {

        pnmhdr32 = (LPNMHDR) lpwm32mpex->Parm16.WndProc.lParam;

        // Save the original 32-bit flat pointer for unthunking.
        lpwm32mpex->dwTmp[0] = (DWORD) pnmhdr32;

        // If this 32bit message came from WOW, we have the original
        // 16:16 lparam squirrelled away. (mapped to the original 32-bit lParam)
        // Note: If the mapping is found, the ref count gets incremented.
        lpwm32mpex->Parm16.WndProc.lParam = (LONG)GetParam16(lpwm32mpex->lParam);

        // if we don't already have a 16:16 ptr for this -- create one
        // This means we are seeing this message for the 1st time -- coming
        // from the 32-bit world.
        if ( ! lpwm32mpex->Parm16.WndProc.lParam) {
            if (pnmhdr32->code >= NM_LAST) {
                lpwm32mpex->dwParam = sizeof(NMHDR);
            }
            else {
                lpwm32mpex->dwParam = GetNMHDRextensionSize(pnmhdr32);
            }

            // be sure allocation size matches stackfree16() size below
            lpwm32mpex->dwTmp[1] = (DWORD)lpwm32mpex->dwParam;
            lpwm32mpex->Parm16.WndProc.lParam = stackalloc16(lpwm32mpex->dwTmp[1]);
            GETVDMPTR(lpwm32mpex->Parm16.WndProc.lParam, lpwm32mpex->dwParam, pnmhdr16);
            CopyMemory(pnmhdr16, pnmhdr32, lpwm32mpex->dwParam);
            FREEVDMPTR(pnmhdr16);

        // else don't allocate (or free) another 16:16 ptr
        } else {

            // set to FALSE so we don't free this thing too soon
            lpwm32mpex->fFree = FALSE;
        }

    } else {

        if (lpwm32mpex->fFree) {
            GETVDMPTR(lpwm32mpex->Parm16.WndProc.lParam, lpwm32mpex->dwParam, pnmhdr16);
            // retrieve original 32-bit pointer
            pnmhdr32 = (LPNMHDR) lpwm32mpex->dwTmp[0];
            CopyMemory(pnmhdr32, pnmhdr16, lpwm32mpex->dwParam);
            FREEVDMPTR(pnmhdr16);
            if(lpwm32mpex->Parm16.WndProc.lParam) {
                stackfree16(lpwm32mpex->Parm16.WndProc.lParam, lpwm32mpex->dwTmp[1]);
            }
        } else {

            // Decrement the ref count. If the ref count goes to zero, the
            // mapping is nuked.
            DeleteParamMap(lpwm32mpex->Parm16.WndProc.lParam, PARAM_16, NULL);
        }
    }

    return TRUE;
}

// This function thunks the 32-bit message WM_NOTIFYWOW.
// uParam dictates where the notification should be dispatched.
//


BOOL FASTCALL WM32NotifyWow(LPWM32MSGPARAMEX lpwm32mpex)
{
    switch (lpwm32mpex->uParam) {
        case WMNW_UPDATEFINDREPLACE:
           if (lpwm32mpex->fThunk) {
                // Update the 16-bit FINDREPLACE struct.
                lpwm32mpex->Parm16.WndProc.lParam = WCD32UpdateFindReplaceTextAndFlags(lpwm32mpex->hwnd, lpwm32mpex->lParam);
                lpwm32mpex->Parm16.WndProc.wMsg = msgFINDREPLACE;
                return(TRUE);
            }
            break;

        default:
            LOGDEBUG(LOG_ALWAYS, ("WOW::WM32NotifyWow: Unknown dispatch parameter!\n"));
            WOW32ASSERT (FALSE);

    }

    return (FALSE);
}

//
//  In ThunkMsg16 we use the data in 32->16 message thunk table to optimize
//  thunking process based on 'WM32NoThunking'.
//
//  This is place holder for those messages which need nothunking on 32-16
//  trasitions but need some kind of thunking on 16->32 transistions.
//
//  So this marks the message as 'this message needs 16-32 thunking but
//  not 32-16 thunking'
//
//                                            - nanduri

BOOL FASTCALL WM32Thunk16To32(LPWM32MSGPARAMEX lpwm32mpex)
{
    return (TRUE);
}
#ifdef FE_IME
//
// This function thunks the messages,
//
//  WM_IME_REPORT

/*
BOOL FASTCALL WM32IMEReport (HWND hwnd, UINT uMsg, UINT uParam, LONG lParam,
          PWORD pwMsgNew, PWORD pwParamNew, PLONG plParamNew,
          PLONG plReturn, BOOL fThunk, LPWM32MSGPARAMEX lpwm32mpex)
*/
BOOL FASTCALL WM32IMEReport (LPWM32MSGPARAMEX lpwm32mpex)
{
//lpwm32mpex->hwnd
//lpwm32mpex->uMsg
//lpwm32mpex->uParam
//lpwm32mpex->lParam
//lpwm32mpex->Param16.WndProc.wMsg
//lpwm32mpex->Param16.WndProc.wParam
//lpwm32mpex->Param16.WndProc.lParam
//&lpwm32mpex->lReturn
//lpwm32mpex->fThunk
//

    INT     cb;
    INT     i;
    HMEM16  hMem16 = 0;
    LPBYTE  lpMem32 = 0;
    LPBYTE  lpMem16 = 0;
    VPVOID  vp;


    if (lpwm32mpex->fThunk) {
	if (lpwm32mpex->uParam == IR_STRING) {
	    lpMem32 = GlobalLock((HANDLE)lpwm32mpex->lParam);
            if (lpMem32 == NULL)
                goto Err;

	    cb = strlen( lpMem32 ) + 1;
	    if (!(lpwm32mpex->Parm16.WndProc.lParam = GlobalAllocLock16(GMEM_MOVEABLE | GMEM_SHARE | GMEM_ZEROINIT, cb, &hMem16 )))
		goto Err;
	    putstr16((VPSZ)lpwm32mpex->Parm16.WndProc.lParam, lpMem32, cb);
            LOW( lpwm32mpex->Parm16.WndProc.lParam ) = hMem16;
            HIW( lpwm32mpex->Parm16.WndProc.lParam ) = 0;         // must be zero
            GlobalUnlock( (HANDLE)lpwm32mpex->lParam );
            GlobalUnlock16( hMem16 );
	}
        /**** IR_STRINGEX ****/
        else if ( lpwm32mpex->uParam == IR_STRINGEX ) {
            LPSTRINGEXSTRUCT    pss32;
            PSTRINGEXSTRUCT16  pss16;

            lpMem32 = GlobalLock((HANDLE)lpwm32mpex->lParam);
            if (lpMem32 == NULL)
                goto Err;

            pss32 = (LPSTRINGEXSTRUCT)lpMem32;

            cb = pss32->dwSize;
            if ( cb >= ( 64 * K )) {
                // It's problem !
                LOGDEBUG(0,(" WOW:: WM_IME_REPORT:IR_STRINGEX data size must be less than 64K on WOW. cb = %d\n", cb ));
                /** goto Err; **/
            }
            // Shuld I pack size of this structure ?
	    if (!(vp = GlobalAllocLock16(GMEM_MOVEABLE | GMEM_SHARE | GMEM_ZEROINIT, cb, &hMem16 )))
		return FALSE;

            GETVDMPTR(vp, cb, lpMem16 );
            pss16 = (PSTRINGEXSTRUCT16)lpMem16;

            STOREDWORD( pss16->dwSize, pss32->dwSize );
            STOREWORD( pss16->uDeterminePos, pss32->uDeterminePos );
            STOREWORD( pss16->uDetermineDelimPos, pss32->uDetermineDelimPos );
            STOREWORD( pss16->uYomiPos, pss32->uYomiPos );
            STOREWORD( pss16->uYomiDelimPos, pss32->uYomiDelimPos );

            if ( pss32->uDeterminePos ) {
                cb = strlen( (LPBYTE)&lpMem32[ pss32->uDeterminePos ] );
                RtlCopyMemory( &lpMem16[ pss16->uDeterminePos ],
                               &lpMem32[ pss32->uDeterminePos ], cb + 1 );
                if ( pss32->uDetermineDelimPos ) {
                    for ( i = 0; pss32->uDetermineDelimPos + i * sizeof(INT) < pss32->dwSize; i++ ) {
                        WORDOF16( lpMem16[ pss16->uDetermineDelimPos ], i ) = (WORD)
                        INTOF( lpMem32[ pss32->uDetermineDelimPos ], i );
                        if ( INTOF( lpMem32[ pss32->uDetermineDelimPos ], i ) >= cb )
                            break;
                    }
                }
            }

            if ( pss32->uYomiPos ) {
                cb = strlen( (LPBYTE)&lpMem32[ pss32->uYomiPos ] );
                RtlCopyMemory( &lpMem16[ pss16->uYomiPos ],
                               &lpMem32[ pss32->uYomiPos ], cb + 1 );
                if ( pss32->uYomiDelimPos ) {
                    for ( i = 0; pss32->uYomiDelimPos + i * sizeof(INT) < pss32->dwSize; i++ ) {
                        WORDOF16( lpMem16[ pss16->uYomiDelimPos ], i ) = (WORD)
                        INTOF( lpMem32[ pss32->uYomiDelimPos ], i );
                        if ( INTOF( lpMem32[ pss32->uYomiDelimPos ], i ) >= cb )
                            break;
                    }
                }
            }

            FLUSHVDMPTR(vp, cb, lpMem16);
            FREEVDMPTR(lpMem16);

            LOW( lpwm32mpex->Parm16.WndProc.lParam ) = hMem16;
            HIW( lpwm32mpex->Parm16.WndProc.lParam ) = 0;         // must be zero
            GlobalUnlock( (HANDLE)lpwm32mpex->lParam );
            GlobalUnlock16( hMem16 );
        }
        /**** IR_UNDETERMINE ****/
	else if (lpwm32mpex->uParam == IR_UNDETERMINE) {
            PUNDETERMINESTRUCT16  pus16;
            LPUNDETERMINESTRUCT    pus32;

            lpMem32 = GlobalLock( (HANDLE)lpwm32mpex->lParam );
            if (lpMem32 == NULL)
                goto Err;

            pus32 = (LPUNDETERMINESTRUCT)lpMem32;
            cb = pus32->dwSize;

            if ( cb >= ( 64 * K )) {
                // It's problem !
                LOGDEBUG(0,(" WOW:: WM_IME_REPORT:IR_UNDETERMINE data size must be less than 64K on WOW. cb = %d\n", cb ));
                /** goto Err; **/
            }
	    if (!( vp = GlobalAllocLock16(GMEM_MOVEABLE | GMEM_SHARE | GMEM_ZEROINIT, cb, &hMem16 )))
		goto Err;

            GETVDMPTR(vp, cb, lpMem16 );
            pus16 = (PUNDETERMINESTRUCT16)lpMem16;

            STOREDWORD( pus16->dwSize,            (pus32)->dwSize );
            STOREWORD( pus16->uDefIMESize,        (pus32)->uDefIMESize );
            STOREWORD( pus16->uDefIMEPos,         (pus32)->uDefIMEPos );
            STOREWORD( pus16->uUndetTextLen,      (pus32)->uUndetTextLen );
            STOREWORD( pus16->uUndetTextPos,      (pus32)->uUndetTextPos );
            STOREWORD( pus16->uUndetAttrPos,      (pus32)->uUndetAttrPos );
            STOREWORD( pus16->uCursorPos,         (pus32)->uCursorPos );
            STOREWORD( pus16->uDeltaStart,        (pus32)->uDeltaStart );
            STOREWORD( pus16->uDetermineTextLen,  (pus32)->uDetermineTextLen );
            STOREWORD( pus16->uDetermineTextPos,  (pus32)->uDetermineTextPos );
            STOREWORD( pus16->uDetermineDelimPos, (pus32)->uDetermineDelimPos );
            STOREWORD( pus16->uYomiTextLen,       (pus32)->uYomiTextLen );
            STOREWORD( pus16->uYomiTextPos,       (pus32)->uYomiTextPos );
            STOREWORD( pus16->uYomiDelimPos,      (pus32)->uYomiDelimPos );


            // dada copy
            RtlCopyMemory( &lpMem16[ sizeof(UNDETERMINESTRUCT) ],
                      &lpMem32[ sizeof(UNDETERMINESTRUCT) ],
                      cb - sizeof( UNDETERMINESTRUCT ));

            //adjustment
            if ( pus32->uDetermineDelimPos ) {
                cb = pus32->uDetermineTextLen;
                for ( i = 0; pus32->uDetermineDelimPos + i * sizeof(INT) < pus32->dwSize; i++ ) {
                    INTOF( lpMem16[ pus16->uDetermineDelimPos ], i ) = 0;
                    WORDOF16( lpMem16[ pus16->uDetermineDelimPos ], i ) = (WORD)
                    INTOF( lpMem32[ pus32->uDetermineDelimPos ], i );
                    if ( INTOF( lpMem32[ pus32->uDetermineDelimPos ], i ) >= cb )
                        break;
                }
            }
            if ( pus32->uYomiDelimPos ) {
                cb = pus32->uYomiTextLen;
                for ( i = 0; pus32->uYomiDelimPos + i * sizeof(INT) < pus32->dwSize; i++ ) {
                    INTOF( lpMem16[ pus16->uYomiDelimPos ], i ) = 0;
                    WORDOF16( lpMem16[ pus16->uYomiDelimPos ], i ) = (WORD)
                    INTOF( lpMem32[ pus32->uYomiDelimPos ], i );
                    if ( INTOF( lpMem32[ pus32->uYomiDelimPos ], i ) >= cb )
                        break;
                }
            }

            FLUSHVDMPTR(vp, cb, lpMem16);
            FREEVDMPTR(lpMem16);

            LOW( lpwm32mpex->Parm16.WndProc.lParam ) = hMem16;
            HIW( lpwm32mpex->Parm16.WndProc.lParam ) = 0;         // must be zero
            GlobalUnlock( (HANDLE)lpwm32mpex->lParam );
            GlobalUnlock16( hMem16 );

	}
    }
    else { // fThunk
	if (lpwm32mpex->Parm16.WndProc.lParam) {
	    GlobalUnlockFree16(GlobalLock16(LOW(lpwm32mpex->Parm16.WndProc.lParam), NULL));
        }
    }
    return (TRUE);

Err:
    if ( lpMem32 && lpwm32mpex->lParam )
        GlobalUnlock( (HANDLE)lpwm32mpex->lParam );
    return FALSE;

}
#endif  // FE_IME
