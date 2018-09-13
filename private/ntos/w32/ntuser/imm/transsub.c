/****************************** Module Header ******************************\
* Module Name: transsub.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains the translation layer functions
* of the sub functions of SendImeMessageEx.
*
* History:
* 21-May-1996 takaok      Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

LRESULT TransSetOpenK( HWND hWndApp, HIMC hImc, LPIMESTRUCT lpIme );
LRESULT TransSetOpenJ( HWND hWndApp, HIMC hImc, LPIMESTRUCT lpIme );
LRESULT TransGetOpenK( HWND hWndApp, HIMC hImc, LPIMESTRUCT lpIme, BOOL fAnsi );
LRESULT TransGetOpenJ( HWND hWndApp, HIMC hImc, LPIMESTRUCT lpIme, BOOL fAnsi );
LRESULT TransMoveImeWindow( HWND hWndApp, HIMC hImc, LPIMESTRUCT lpIme);
LRESULT TransSetConversionWindow(  HWND hWndApp, HIMC hImc, LPIMESTRUCT lpIme );
LRESULT TransSetConversionMode( HIMC hImc, LPIMESTRUCT lpIme );
LRESULT TransGetMode( HIMC hImc );
LRESULT TransGetConversionMode( HIMC hImc );
LRESULT TransSetMode( HIMC hImc, LPIMESTRUCT lpIme );
LRESULT TransSendVKey( HWND hWndApp, HIMC hImc, LPIMESTRUCT lpIme, BOOL fAnsi );
LRESULT TransEnterWordRegisterMode( HWND hWndApp, LPIMESTRUCT lpIme, BOOL fAnsi);
LRESULT TransSetConversionFontEx( HWND hWndApp, HIMC hImc, LPIMESTRUCT lpIme, BOOL fAnsi);
LRESULT TransHanjaMode( HWND hWndApp, HIMC hImc, LPIMESTRUCT lpIme);
UINT Get31ModeFrom40ModeJ( DWORD fdwConversion );
UINT Get31ModeFrom40ModeK( DWORD fdwConversion );
LRESULT TransVKDBEMode( HIMC hImc, WPARAM wVKDBE );

BOOL  SetFontForMCWVERTICAL( HWND hWndApp, HIMC hImc, LPINPUTCONTEXT pInputContext, BOOL fVert );
BOOL IsForegroundThread( HWND );
BOOL FixLogfont( LPLOGFONTW lplfW, BOOL fVert );

BOOL MySetCompFont( HWND, HIMC, LPLOGFONTW );
BOOL MySetCompWindow( HWND, HIMC, LPCOMPOSITIONFORM );
BOOL MySetCandidateWindow( HWND, HIMC, LPCANDIDATEFORM );
BOOL MyPostImsMessage( HWND hWndApp, WPARAM wParam, LPARAM lParam);

//===================================================================
// TranslateIMESubFunctions
//==========================
//
// KOREAN and JAPANESE common translate routine for the
// sub functions of SendImeMessageEx.
//
// History:
// 21-May-1996 takaok      Created.
//
//===================================================================
LRESULT TranslateIMESubFunctions(
    HWND hWndApp,
    LPIMESTRUCT lpIme,
    BOOL fAnsi)
{
    HIMC        hImc;
    LRESULT     lRet;
    DWORD       dwLangID;

    hImc = ImmGetSaveContext( hWndApp, IGSC_DEFIMCFALLBACK );
    if ( hImc == NULL_HIMC ) {
        return FALSE;
    }

    dwLangID = PRIMARYLANGID(LANGIDFROMLCID(GetSystemDefaultLCID()));

    switch (lpIme->fnc) {
    case 0x03:  // IME_QUERY, IME_GETIMECAPS: KOREAN & JAPANESE
        lRet = TRUE;
        break;

    case 0x04:  // IME_SETOPEN: KOREAN & JAPANESE
        if ( dwLangID == LANG_KOREAN )
            lRet = TransSetOpenK( hWndApp, hImc, lpIme );
        else
            lRet = TransSetOpenJ( hWndApp, hImc, lpIme );
        break;

    case 0x05:  // IME_GETOPEN: KOREAN & JAPANESE
        if ( dwLangID == LANG_KOREAN )
            lRet = TransGetOpenK( hWndApp, hImc, lpIme, fAnsi );
        else
            lRet = TransGetOpenJ( hWndApp, hImc, lpIme, fAnsi );
        break;

    case 0x06:  // IME_ENABLEDOSIME, IME_ENABLE
    // internal functions are not supported
        lRet = FALSE;
        break;

    case 0x07:  // IME_GETVERSION: KOREAN & JAPANESE
        lRet = IMEVER_31;
        break;

    case 0x08:  // IME_MOVEIMEWINDOW, IME_SETCONVERSIONWINDOW: KOREAN & JAPANESE

        if ( dwLangID == LANG_KOREAN ) {
            //
            // IME_MOVEIMEWINDOW for KOREAN
            //
            lRet = TransMoveImeWindow(hWndApp, hImc, lpIme);
        } else {
            //
            // IME_MOVECONVERTWINDOW or IME_SETCONVERSIONWINDOW for JAPANESE
            //
            lRet = TransSetConversionWindow( hWndApp, hImc, lpIme );
        }
        break;

//  case 0x09:  // undefined

    case 0x10:  // IME_SETCONVERSIONMODE: JAPANESE
        if ( dwLangID == LANG_JAPANESE ) {
            lRet = TransSetConversionMode( hImc, lpIme );
        } else {
            lRet = FALSE;
        }
        break;

    case 0x11:  // IME_GETCONVERSIONMODE, IME_GETMODE: // KOREAN & JAPANESE
        // Use hSaveIMC, If WINNSEnableIME(FALSE).
        if ( dwLangID == LANG_KOREAN ) {
        //
        // IME_GETMODE for KOREAN
        //
            lRet = TransGetMode( hImc );
        } else {
        //
        // IME_GETCONVERSIONMODE for JAPANESE
        //
            lRet = TransGetConversionMode( hImc );
        }
        break;

    case 0x12:  // IME_SET_MODE, IME_SETFONT, IME_SETCONVERSIONFONT: KOREAN & JAPANESE
        if ( dwLangID == LANG_KOREAN ) {
            //
            // IME_SET_MODE for KOREAN
            //
            lRet = TransSetMode( hImc, lpIme );
        } else {
            //
            // IME_SETCONVERSIONFONT for JAPANESE
            //
            lRet = FALSE;   // should not be called. use SETCONVERSIONFONTEX instead
        }
        break;

    case 0x13:  // IME_SENDVKEY, IME_SENDKEY: JAPANESE only
        if ( dwLangID == LANG_JAPANESE ) {
            lRet = TransSendVKey( hWndApp, hImc, lpIme, fAnsi );
        } else {
            lRet = FALSE;
        }
        break;

//
// internal sub functions are not supported
//
//  case 0x14: // IME_DESTROY, IME_DESTROYIME
//  case 0x15: // IME_PRIVATE
//  case 0x16: // IME_WINDOWUPDATE
//  case 0x17: // IME_SELECT

    case 0x18: // IME_ENTERWORDREGISTERMODE: JAPANESE only
        if ( dwLangID == LANG_JAPANESE ) {
            lRet = TransEnterWordRegisterMode( hWndApp, lpIme, fAnsi);
        } else {
            lRet = FALSE;
        }
        break;

    case 0x19:  // IME_SETCONVERSIONFONTEX: JAPANESE only
        if ( dwLangID == LANG_JAPANESE ) {
            lRet = TransSetConversionFontEx( hWndApp, hImc, lpIme, fAnsi);
        } else {
            lRet = FALSE;
        }
        break;

//
// internal sub functions are not supported
//
//  case 0x1A: // IME_DBCSNAME
//  case 0x1B: // IME_MAXKEY
//  case 0x1C: // IME_WINNLS_SK

    case 0x20:  // IME_CODECONVERT: KOREAN only
        if ( dwLangID == LANG_KOREAN ) {
            if (TransCodeConvert( hImc, lpIme))
                lRet = lpIme->wParam;
            else
                lRet = 0;
        } else {
            lRet = 0;
        }
        break;

    case 0x21:  // IME_CONVERTLIST: KOREAN only
        if ( dwLangID == LANG_KOREAN ) {
            lRet = TransConvertList( hImc, lpIme);
        } else {
            lRet = FALSE;
        }
        break;

//
// internal sub functions and undefined sub functions are not supported
//
//  case 0x22:  // IME_INPUTKEYTOSEQUENCE
//  case 0x23:  // IME_SEQUENCETOINTERNAL
//  case 0x24:  // IME_QUERYIMEINFO
//  case 0x25:  // IME_DIALOG
//  case 0x26 - 0x2f:   // undefined

    case 0x30:  // IME_AUTOMATA: KOREAN only
        if ( dwLangID == LANG_KOREAN ) {
            lRet = ImmEscape(GetKeyboardLayout(0), hImc, IME_AUTOMATA, lpIme);
        } else {
            lRet = FALSE;
        }
        break;

    case 0x31:  // IME_HANJAMODE: KOREAN only
        if ( dwLangID == LANG_KOREAN ) {
            lRet = TransHanjaMode( hWndApp,  hImc, lpIme);
        } else {
            lRet = FALSE;
        }
        break;
//
//  case 0x32 - 0x3f: // undefined
//
    case 0x40:  // IME_GETLEVEL: KOREAN only
        if ( dwLangID == LANG_KOREAN ) {
            lRet = TransGetLevel( hWndApp );
        } else {
            lRet = FALSE;
        }
        break;

    case 0x41:  // IME_SETLEVEL: KOREAN only
        if ( dwLangID == LANG_KOREAN ) {
            lRet = TransSetLevel( hWndApp, lpIme);
        } else {
            lRet = FALSE;
        }
        break;

    case 0x42:  // IME_GETMNTABLE: KOREAN only
        if ( dwLangID == LANG_KOREAN ) {
            lRet = TransGetMNTable( hImc, lpIme);
        } else {
            lRet = FALSE;
        }
        break;

#if defined(PENAPI)
    case IME_SETUNDETERMINESTRING:
        lRet = FSetUndetermine( hImc, (HGLOBAL)lpIME->lParam1);
        break;

    case IME_SETCAPTURE:
        lRet = FEnablePenUi((HWND)lpIME->wParam, (lpIME->wParam != NULL));
        break;
#endif

#ifdef LATER // IME_NOTIFYWOWTASKEXIT
    case IME_NOTIFYWOWTASKEXIT:
      //
      // Destroy the default IME window of WOW 16bit
      // applications now. We should not wait for the
      // server wow thread clean up that will destroy the
      // IME window because usrsrv won't send WM_DESTROY
      // to non-server side window procedures. Some IMEs
      // must receive WM_DESTROY to free up 32 bit objects.
      //
      //  kksuzuka #7982:IME memory leak on WOW16 applications.
      //
      PIMMTHREADINFO piti = PitiCurrent();

      if ( piti != NULL && IsWindow( piti->hwndDefaultIme ) ) {
          DestroyWindow( piti->hwndDefaultIme );
      }
      return TRUE;
#endif
    default:
        //
        // private/internal/undefined functions are not supported
        //
        lRet = FALSE;
        break;
    }

    return (lRet);
}

//===================================================================
// TransSetOpenK
//===============
//
// KOREAN only
//
// History:
// xx-xx-1995     xxx      Created
//
//===================================================================
LRESULT TransSetOpenK( HWND hWndApp, HIMC hImc, LPIMESTRUCT lpIme )
{
    // BUGBUG: We will use this function instead of ImmEscape().
    LRESULT lRet;

    lRet = ImmEscape(GetKeyboardLayout(0), hImc, IME_SETOPEN, lpIme);
    return (lRet);
    UNREFERENCED_PARAMETER(hWndApp);
}

//===================================================================
// TransSetOpenJ
//===============
//
// Japanese only
//
// History:
// 20-May-1996     takaok      Created
//
//===================================================================
LRESULT TransSetOpenJ( HWND hWndApp, HIMC hImc, LPIMESTRUCT lpIme )
{
    LRESULT lRet;

    lRet = ImmGetOpenStatus( hImc );
    //
    // if the owner thread of hIMC doesn't have the input focus,
    // we won't call UI.
    //
    if ( !IsForegroundThread( NULL ) && !GetFocus() ) {
        //
        // this thread doesn't have focus.
        // let's update the input context and return without calling UI
        //
        PINPUTCONTEXT pInputContext;

        if ( (pInputContext = ImmLockIMC(hImc)) != NULL ) {
            if ( (pInputContext->fOpen && ! lpIme->wParam ) ||
                 (!pInputContext->fOpen &&  lpIme->wParam ) )
            {
                pInputContext->fOpen = (BOOL)lpIme->wParam;
                ImmNotifyIME( hImc, NI_CONTEXTUPDATED, 0, IMC_SETOPENSTATUS);
            }
            ImmUnlockIMC( hImc );
        }
    } else {
        ImmSetOpenStatus( hImc, (BOOL)lpIme->wParam );
    }
    return lRet;
    UNREFERENCED_PARAMETER(hWndApp);
}

//===================================================================
// TransGetOpenK
//===============
//
// KOREAN only
//
// History:
// xx-xx-1995     xxx      Created
//
//===================================================================
LRESULT TransGetOpenK( HWND hWndApp, HIMC hImc, LPIMESTRUCT lpIme, BOOL fAnsi )
{
    // BUGBUG: We will use this function instead of ImmEscape().
    RECT    rc;
    LPARAM  lTemp;
    LRESULT lRet;

    lTemp = lpIme->lParam2;
    GetWindowRect(hWndApp, &rc);
    lpIme->lParam2 = MAKELONG(rc.top, rc.left);
    lRet = ImmEscape(GetKeyboardLayout(0), hImc, IME_GETOPEN, lpIme);
    lpIme->lParam2 = lTemp;
    return (lRet);
    UNREFERENCED_PARAMETER(fAnsi);
}

//===================================================================
// TransGetOpenJ
//===============
//
// Japanese only
//
// History:
// 20-May-1996     takaok      Created
//
//===================================================================
LRESULT TransGetOpenJ( HWND hWndApp, HIMC hImc, LPIMESTRUCT lpIme, BOOL fAnsi )
{
    INT Count;
    LRESULT lRet;

    lRet = ImmGetOpenStatus( hImc );

    // lpIME->wCount is the length of the composition string.
    if ( fAnsi ) {
        Count= ImmGetCompositionStringA( hImc, GCS_COMPSTR, NULL, 0L );
    } else {
        Count= ImmGetCompositionStringW( hImc, GCS_COMPSTR, NULL, 0L );
    }
    lpIme->wCount = ( Count > 0 ) ? Count : 0;

    return lRet;
    UNREFERENCED_PARAMETER(hWndApp);
}

//===================================================================
// TransMoveImeWindow
//====================
//
// Korean only
//
//===================================================================
LRESULT TransMoveImeWindow( HWND hWndApp, HIMC hImc, LPIMESTRUCT lpIme)
{
    // BUGBUG: We will use this function instead of ImmEscape().
    POINT   pt;
    LRESULT lRet;

    if (lpIme->wParam == MCW_WINDOW)
    {
        pt.x = GET_X_LPARAM(lpIme->lParam1);
        pt.y = GET_Y_LPARAM(lpIme->lParam1);
        ClientToScreen(hWndApp, &pt);
        lpIme->lParam1 = MAKELONG(pt.x, pt.y);
    }
    lRet = ImmEscape(GetKeyboardLayout(0), hImc, IME_MOVEIMEWINDOW, lpIme);
    return (lRet);
}


//===================================================================
// TransSetConversionWindow
//=========================
//
// Japanese only
//
//===================================================================
LRESULT TransSetConversionWindow(  HWND hWndApp, HIMC hImc, LPIMESTRUCT lpIme )
{
    LPINPUTCONTEXT pInputContext;
    COMPOSITIONFORM cof;
    CANDIDATEFORM   caf;
    POINT pt;
    RECT  rt;
    INT   i;

    if ( ! IsForegroundThread(NULL) && !GetFocus() ) {
        //
        // For win95 compatibility, we need to return TRUE though we
        // didn't really succeed. PP4 calls us when it doesn't have
        // the input focus to check if IME is capable to do the specfied
        // MCW_xxx. Returning TRUE here will make such application happy.
        //
        return ( TRUE );
    }

    pInputContext = ImmLockIMC( hImc );
    if ( pInputContext == NULL ) {
        return ( FALSE );
    }

    pt.x      = GET_X_LPARAM(lpIme->lParam1);
    pt.y      = GET_Y_LPARAM(lpIme->lParam1);
    rt.left   = GET_X_LPARAM(lpIme->lParam2);
    rt.top    = GET_Y_LPARAM(lpIme->lParam2);
    rt.right  = GET_X_LPARAM(lpIme->lParam3);
    rt.bottom = GET_Y_LPARAM(lpIme->lParam3);

    cof.dwStyle = CFS_DEFAULT;

    if ( lpIme->wParam & MCW_HIDDEN ) {
        pInputContext->fdw31Compat |= F31COMPAT_MCWHIDDEN;
        ScreenToClient( pInputContext->hWnd, &pt );
        MapWindowPoints( HWND_DESKTOP, pInputContext->hWnd, (LPPOINT)&rt, 2);
    } else {
        pInputContext->fdw31Compat &= ~F31COMPAT_MCWHIDDEN;
    }

    if ( lpIme->wParam & MCW_WINDOW) {
        if ( !IsWndEqual(hWndApp, pInputContext->hWnd)) {
            ClientToScreen(hWndApp, &pt);
            ScreenToClient(pInputContext->hWnd, &pt);
            if (lpIme->wParam & MCW_RECT) {
                cof.dwStyle = CFS_RECT;
                MapWindowPoints(hWndApp, HWND_DESKTOP, (LPPOINT)&rt, 2);
                MapWindowPoints(HWND_DESKTOP, pInputContext->hWnd, (LPPOINT)&rt, 2);
            } else {
                cof.dwStyle = CFS_POINT;
            }
        } else {
            if ( lpIme->wParam & MCW_RECT) {
                cof.dwStyle = CFS_RECT;
            } else {
                cof.dwStyle = CFS_POINT;
            }
        }
    }

    // Because Chicago IME can not handle CFS_SCREEN. The points should be
    // converted to client point.
    // If these points are out of Client, HOW SHOULD WE DO????

    if ( lpIme->wParam & MCW_SCREEN ) {
        ScreenToClient( pInputContext->hWnd, &pt );
        if ( lpIme->wParam & CFS_RECT ) {
            cof.dwStyle = CFS_RECT;
            MapWindowPoints( HWND_DESKTOP, pInputContext->hWnd, (LPPOINT)&rt, 2 );
        }
        else {
            cof.dwStyle = CFS_POINT;
        }
    }

    if ( lpIme->wParam & MCW_VERTICAL) {
        if ( !(pInputContext->fdw31Compat & F31COMPAT_MCWVERTICAL) ) {
            pInputContext->fdw31Compat |= F31COMPAT_MCWVERTICAL;
            SetFontForMCWVERTICAL( hWndApp, hImc, pInputContext, TRUE);
        }
    } else {
        if (pInputContext->fdw31Compat & F31COMPAT_MCWVERTICAL) {
            pInputContext->fdw31Compat &= ~F31COMPAT_MCWVERTICAL;
            SetFontForMCWVERTICAL( hWndApp, hImc, pInputContext, FALSE);
        }
    }
    cof.ptCurrentPos = pt;
    cof.rcArea       = rt;

#if defined(PENAPI)
    if ( !FSetPosPenUi(&cof) )
#endif
    if ( !(pInputContext->fdw31Compat & F31COMPAT_MCWHIDDEN) ) {
        MySetCompWindow( hWndApp, hImc, (LPCOMPOSITIONFORM)&cof );
    } else {
        // Hack for 3.1 Apps. We save the exlude area into IMC.
        pInputContext->cfCompForm.ptCurrentPos = cof.ptCurrentPos;
        pInputContext->cfCompForm.rcArea       = cof.rcArea;

        for ( i=0; i < 4; i++ ) {
            if ( pInputContext->cfCandForm[i].dwIndex != -1)
            {
                caf.dwIndex = i;
                caf.dwStyle = CFS_EXCLUDE;
                caf.ptCurrentPos = pt;
                caf.rcArea       = rt;
                MySetCandidateWindow( hWndApp, hImc, (LPCANDIDATEFORM)&caf );
            }
        }
    }
    ImmUnlockIMC( hImc );
    return ( TRUE );
}

//===================================================================
// TransSetConversionMode
//=======================
//
// Japanese only
//
// History:
// 31-May-1996 takaok      Created.
//
//===================================================================
LRESULT TransSetConversionMode( HIMC hImc, LPIMESTRUCT lpIme )
{
    DWORD fdwConversion, fdwSentence, fdwNewConversion, fdwMask;
    UINT  uPrevMode;
    UINT  u31Mode;

    //
    // Get current conversion mode and translate it to
    // the 3.1 style conversion mode.
    //
    ImmGetConversionStatus( hImc, &fdwConversion, &fdwSentence);
    uPrevMode = Get31ModeFrom40ModeJ( fdwConversion );

    //
    // translate requested 3.1 conversion mode to 4.0 conversion mode
    //
    fdwNewConversion = 0;
    u31Mode = (UINT)lpIme->wParam;

    switch ( u31Mode & 0x07 ) {
    case IME_MODE_ALPHANUMERIC:
        fdwNewConversion &= ~IME_CMODE_LANGUAGE;
        break;
    case IME_MODE_KATAKANA:
        fdwNewConversion |= IME_CMODE_NATIVE|IME_CMODE_KATAKANA;
        break;
    case IME_MODE_HIRAGANA:
        fdwNewConversion |= IME_CMODE_NATIVE;
        break;
    }
    if ( !(u31Mode & JAPAN_IME_MODE_SBCSCHAR) )
        fdwNewConversion |= IME_CMODE_FULLSHAPE;

    if ( u31Mode & IME_MODE_ROMAN )
        fdwNewConversion |= IME_CMODE_ROMAN;

    if ( u31Mode & IME_MODE_CODEINPUT )
        fdwNewConversion |= IME_CMODE_CHARCODE;

    //
    // compute the mask bit. we need to compute this because
    // application may set only bit needed to be changed.
    //
    fdwMask = 0;
    if ( u31Mode & (IME_MODE_ROMAN | IME_MODE_NOROMAN) )
        fdwMask |= IME_CMODE_ROMAN;

    if ( u31Mode & (IME_MODE_CODEINPUT|IME_MODE_NOCODEINPUT) )
        fdwMask |= IME_CMODE_CHARCODE;

    if ( u31Mode & 0x07 )
        fdwMask |= IME_CMODE_LANGUAGE;

    if ( u31Mode & (IME_MODE_DBCSCHAR|JAPAN_IME_MODE_SBCSCHAR) )
        fdwMask |= IME_CMODE_FULLSHAPE;

    //
    // set the new mode
    //
    fdwNewConversion = (fdwNewConversion & fdwMask) | (fdwConversion & ~fdwMask);
    if ( ImmSetConversionStatus( hImc, fdwNewConversion, fdwSentence) ) {
        return (LRESULT)uPrevMode;
    } else {
        return (LRESULT)0;
    }
}

//===================================================================
// TransGetMode
//==============
//
// Korean only
//
// translate 4.0 conversion mode into 3.1 conversion mode
//
// History:
// 31-May-1996 takaok      Created.
//
//===================================================================
LRESULT TransGetMode( HIMC hImc )
{
    DWORD fdwConversion, fdwSentence;
    UINT u31Mode = 0;

    ImmGetConversionStatus( hImc, &fdwConversion, &fdwSentence);
    u31Mode= Get31ModeFrom40ModeK( fdwConversion );
    // HACK: To prevent 0 result from treating FALSE, we always set MSB
    return ( u31Mode | 0x80000000 );
}

//===================================================================
// Get31ModeFrom40ModeK
//=====================
//
// Korean only
//
// translate 4.0 conversion mode into 3.1 conversion mode
//
// History:
// 31-May-1996 takaok      Created.
//
//===================================================================
UINT Get31ModeFrom40ModeK( DWORD fdwConversion )
{
    UINT u31Mode = 0;

    if ( !(fdwConversion & IME_CMODE_NATIVE) ) {

        u31Mode |= IME_MODE_ALPHANUMERIC;
    }

    if ( !(fdwConversion & IME_CMODE_FULLSHAPE) ) {

        u31Mode |= KOREA_IME_MODE_SBCSCHAR;
    }

    if ( fdwConversion & IME_CMODE_HANJACONVERT ) {
        u31Mode |= IME_MODE_HANJACONVERT;
    }

    return u31Mode;
}

//===================================================================
// TransGetConversionMode
//========================
//
// Japanese only
//
// 4.0 conversion mode => 3.1 conversion mode
//
// History:
// 31-May-1996 takaok      Created.
//
//===================================================================
LRESULT TransGetConversionMode( HIMC hImc )
{
    DWORD fdwConversion, fdwSentence;
    UINT u31Mode = 0;

    //
    // get the 4.0 style conversion mode
    //
    ImmGetConversionStatus( hImc, &fdwConversion, &fdwSentence);
    return Get31ModeFrom40ModeJ( fdwConversion );
}

//===================================================================
// Get31ModeFrom40ModeJ
//======================
//
// Japanese only
//
// 4.0 conversion mode => 3.1 conversion mode
//
// History:
// 31-May-1996 takaok      Created.
//
//===================================================================
UINT Get31ModeFrom40ModeJ( DWORD fdwConversion )
{
    UINT u31Mode = 0;

    //
    // translate the 4.0 style mode to the 3.x style conversion mode
    //
    if (fdwConversion & IME_CMODE_NATIVE) {
        if (fdwConversion & IME_CMODE_KATAKANA) {
            u31Mode |= IME_MODE_KATAKANA;
        } else {
            u31Mode |= IME_MODE_HIRAGANA;
        }
    } else {
        u31Mode |= IME_MODE_ALPHANUMERIC;
    }

    if (fdwConversion & IME_CMODE_FULLSHAPE) {
        u31Mode |= IME_MODE_DBCSCHAR;
    } else {
        u31Mode |= JAPAN_IME_MODE_SBCSCHAR;
    }

    if (fdwConversion & IME_CMODE_ROMAN) {
        u31Mode |= IME_MODE_ROMAN;
    } else {
        u31Mode |= IME_MODE_NOROMAN;
    }

    if (fdwConversion & IME_CMODE_CHARCODE) {
        u31Mode |= IME_MODE_CODEINPUT;
    } else {
        u31Mode |= IME_MODE_NOCODEINPUT;
    }

    return (u31Mode);
}


//===================================================================
// TransSetMode
//==============
//
// KOREAN only
//
//===================================================================
LRESULT TransSetMode( HIMC hImc, LPIMESTRUCT lpIme )
{
    DWORD fdwConversion, fdwSentence, fdwNewConversion, fdwMask;
    UINT  uPrevMode;
    UINT  u31Mode;

    //
    // Get current conversion mode and translate it to
    // the 3.1 style conversion mode.
    //
    ImmGetConversionStatus( hImc, &fdwConversion, &fdwSentence);
    uPrevMode = Get31ModeFrom40ModeK( fdwConversion );

    //
    // translate requested 3.1 conversion mode to 4.0 conversion mode
    //
    fdwNewConversion = 0;
    u31Mode = (UINT)lpIme->wParam;

    if ( !(u31Mode & IME_MODE_ALPHANUMERIC) )
        fdwNewConversion |= IME_CMODE_HANGEUL;
    if ( !(u31Mode & KOREA_IME_MODE_SBCSCHAR) )
        fdwConversion |= IME_CMODE_FULLSHAPE;

    //
    // In HWin3.1 there is no "not modification mode"
    //
    fdwMask = IME_CMODE_LANGUAGE|IME_CMODE_FULLSHAPE|IME_CMODE_HANJACONVERT;

    //
    // set the new mode
    //
    fdwNewConversion = (fdwNewConversion & fdwMask) | (fdwConversion & ~fdwMask);
    if ( ImmSetConversionStatus( hImc, fdwNewConversion, fdwSentence) ) {
        return (LRESULT)uPrevMode;
    } else {
        return (LRESULT)0;
    }
    return FALSE;
}

//===================================================================
// TransSendVKey
//===============
//
// Japanese only
//
//===================================================================
LRESULT TransSendVKey( HWND hWndApp, HIMC hImc, LPIMESTRUCT lpIme, BOOL fAnsi )
{
    LRESULT lRet;

    switch (lpIme->wParam)
    {
        // VK_DBE_xxx Support Check.
        case (DWORD)(-1):
        case (DWORD)(0x0000ffff):   // from WOW16
            switch (lpIme->wCount)
            {
                case VK_DBE_ALPHANUMERIC:
                case VK_DBE_KATAKANA:
                case VK_DBE_HIRAGANA:
                case VK_DBE_SBCSCHAR:
                case VK_DBE_DBCSCHAR:
                case VK_DBE_ROMAN:
                case VK_DBE_NOROMAN:
                case VK_DBE_CODEINPUT:
                case VK_DBE_NOCODEINPUT:
                case VK_DBE_ENTERWORDREGISTERMODE:
                case VK_DBE_ENTERIMECONFIGMODE:
                case VK_DBE_ENTERDLGCONVERSIONMODE:
                case VK_DBE_DETERMINESTRING:
                case VK_DBE_FLUSHSTRING:
                case VK_CONVERT:
                    lRet = TRUE;
                    break;

                default:
                    lRet = FALSE;
                    break;
            }
            break;

        case VK_DBE_ALPHANUMERIC:
        case VK_DBE_KATAKANA:
        case VK_DBE_HIRAGANA:
        case VK_DBE_SBCSCHAR:
        case VK_DBE_DBCSCHAR:
        case VK_DBE_ROMAN:
        case VK_DBE_NOROMAN:
        case VK_DBE_CODEINPUT:
        case VK_DBE_NOCODEINPUT:
            lRet = TransVKDBEMode(hImc, lpIme->wParam);
            break;

        case VK_DBE_ENTERWORDREGISTERMODE:
            {
            HKL hkl = GetKeyboardLayout(0L);

            if ( fAnsi )
                lRet = ImmConfigureIMEA(hkl, hWndApp, IME_CONFIG_REGISTERWORD, NULL);
            else
                lRet = ImmConfigureIMEW(hkl, hWndApp, IME_CONFIG_REGISTERWORD, NULL);
            }
            break;

        case VK_DBE_ENTERIMECONFIGMODE:
            {
                HKL hkl = GetKeyboardLayout(0L);
                if (fAnsi)
                    lRet = ImmConfigureIMEA(hkl, hWndApp, IME_CONFIG_GENERAL, NULL);
                else
                    lRet = ImmConfigureIMEW(hkl, hWndApp, IME_CONFIG_GENERAL, NULL);
            }
            break;

        case VK_DBE_ENTERDLGCONVERSIONMODE:
#if defined(PENAPI)
            FInitPenUi(hIMC);
#endif
            lRet = FALSE;
            break;

        case VK_DBE_DETERMINESTRING:
            // Check there is the composition string or not.
            lRet = ImmNotifyIME( ImmGetContext(hWndApp),
                                 NI_COMPOSITIONSTR,
                                 CPS_COMPLETE,
                                 0L);
            break;

        case VK_DBE_FLUSHSTRING:
            lRet = ImmNotifyIME( hImc, NI_COMPOSITIONSTR,CPS_CANCEL,0L);
            break;

        case VK_CONVERT:
            lRet = ImmNotifyIME( hImc, NI_COMPOSITIONSTR, CPS_CONVERT, 0L);
            break;

        default:
            break;
    }
    return lRet;
}

//===================================================================
// TransEnterWordRegisterMode
//===========================
//
// Japanese only
//
//===================================================================
LRESULT TransEnterWordRegisterMode( HWND hWndApp, LPIMESTRUCT lpIme, BOOL fAnsi)
{
    LRESULT lRet;
    HKL hkl = GetKeyboardLayout(0L);

    if ( ! ImmIsIME(hkl) ) {
        return FALSE;
    }

    if ( fAnsi ) {
    //
    // ANSI
    //
        REGISTERWORDA stReg = {NULL, NULL};
        LPSTR lpsz1, lpsz2;

        if (lpIme->lParam1&&(lpsz1=GlobalLock((HGLOBAL)lpIme->lParam1))) {
            stReg.lpWord = lpsz1;
        }
        if (lpIme->lParam2&&(lpsz2=GlobalLock((HGLOBAL)lpIme->lParam2))) {
            stReg.lpReading = lpsz2;
        }
        lRet = ImmConfigureIMEA(hkl,hWndApp,IME_CONFIG_REGISTERWORD, (LPVOID)&stReg);
        if (lpIme->lParam1 && lpsz1)
            GlobalUnlock((HGLOBAL)lpIme->lParam1);
        if (lpIme->lParam2 && lpsz2)
            GlobalUnlock((HGLOBAL)lpIme->lParam2);
    } else {
    //
    // UNICODE
    //
        REGISTERWORDW stReg = {NULL, NULL};
        LPWSTR lpsz1, lpsz2;

        if (lpIme->lParam1&&(lpsz1=GlobalLock((HGLOBAL)lpIme->lParam1))) {
            stReg.lpWord = lpsz1;
        }
        if (lpIme->lParam2&&(lpsz2=GlobalLock((HGLOBAL)lpIme->lParam2))) {
            stReg.lpReading = lpsz2;
        }
        lRet = ImmConfigureIMEW(hkl,hWndApp,IME_CONFIG_REGISTERWORD, (LPVOID)&stReg);
        if (lpIme->lParam1 && lpsz1)
            GlobalUnlock((HGLOBAL)lpIme->lParam1);
        if (lpIme->lParam2 && lpsz2)
            GlobalUnlock((HGLOBAL)lpIme->lParam2);
    }
    return lRet;
}


//===================================================================
// TransSetConversionFontEx
//==========================
//
// Japanese only
//
//===================================================================
LRESULT TransSetConversionFontEx( HWND hWndApp, HIMC hImc, LPIMESTRUCT lpIme, BOOL fAnsi)
{
    LPINPUTCONTEXT pInputContext;
    LRESULT lRet;
    LPLOGFONTW lplf;
    LOGFONTW  lfw;

    pInputContext = ImmLockIMC( hImc );
    if ( pInputContext == NULL ) {
        return 0L;
    }

    lplf = (LPLOGFONTW)GlobalLock((HGLOBAL)lpIme->lParam1);
    if ( lplf == NULL )
    {
        ImmUnlockIMC( hImc );
        return 0L;
    }
    if ( fAnsi ) {
        memcpy( &lfw, lplf, sizeof(LOGFONTA) );
        MultiByteToWideChar( CP_ACP,
                             0,
                             (LPCSTR)lplf->lfFaceName,     // src
                             LF_FACESIZE,          // size of src
                             lfw.lfFaceName,       // destination buffer
                             LF_FACESIZE );        // size of destination buffer
    } else {
        memcpy( &lfw, lplf, sizeof(LOGFONTW));
    }
    GlobalUnlock((HGLOBAL)lpIme->lParam1);

    if (( pInputContext->fdw31Compat & F31COMPAT_MCWVERTICAL)) {
        lRet = FixLogfont( &lfw, TRUE);
    } else {
        lRet = FixLogfont( &lfw, FALSE);
    }
    ImmUnlockIMC( hImc );

    if (lRet == FALSE ) {
        return FALSE;
    }

    return MySetCompFont( hWndApp, hImc, &lfw );
}


//===================================================================
// TransHanjaMode
//================
//
// Korean only
//
//===================================================================
LRESULT TransHanjaMode( HWND hWndApp, HIMC hImc, LPIMESTRUCT lpIme)
{
    // BUGBUG: We will use this function instead of ImmEscape().
    LRESULT lRet;
    PIMEDPI pImeDpi;
    DWORD   dwThreadId = GetInputContextThread(hImc);

    if (dwThreadId == 0) {
        RIPMSG1(RIP_WARNING,
              "TransHanjaMode: GetInputContextThread(%lx) failed.", hImc);
        return FALSE;
    }

    pImeDpi = ImmLockImeDpi(GetKeyboardLayout(dwThreadId));
    if (pImeDpi == NULL)
        return FALSE;

    /*
     * Check if we need ANSI/Unicode conversion
     */
    if (pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE) {
        WCHAR wUni;
        CHAR  chAnsi[2];
        UINT  i, dchSource;

        //The 4th word of imestruct32 contains dchSource
        dchSource = *((LPSTR)lpIme + 3 * sizeof(WORD));

        chAnsi[0] = *((LPSTR)lpIme + dchSource);
        chAnsi[1] = *((LPSTR)lpIme + dchSource + 1);

        i = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, chAnsi, 2, &wUni, 1);

        if (i)  {
           *((LPSTR)lpIme + dchSource)   = (CHAR)LOWORD(LOBYTE(wUni));
           *((LPSTR)lpIme + dchSource+1) = (CHAR)LOWORD(HIBYTE(wUni));
        }
        else {
           ImmUnlockImeDpi(pImeDpi);
           return FALSE;
        }
    }

    ImmUnlockImeDpi(pImeDpi);

    if (lRet = ImmEscape(GetKeyboardLayout(0), hImc, IME_HANJAMODE, lpIme))
        SendMessage(hWndApp, WM_IME_NOTIFY, IMN_OPENCANDIDATE, 1L);
    return (lRet);
}

//===================================================================
// TransGetLevel
//===============
//
// Korean only
//
//===================================================================
LRESULT TransGetLevel( HWND hWndApp )
{
    UINT lRet;

    if ( (lRet = NtUserGetAppImeLevel( hWndApp )) == 0 )
        lRet = 1;       // default level

    return lRet;
}

//===================================================================
// TransSetLevel
//===============
//
// Korean only
//
//===================================================================
LRESULT TransSetLevel( HWND hWndApp, LPIMESTRUCT lpIme)
{
    DWORD dwLevel;

    dwLevel = (DWORD)lpIme->wParam;
    if ( dwLevel >= 1 && dwLevel <= 5 ) {
        if ( NtUserSetAppImeLevel(hWndApp, dwLevel) ) {
            return TRUE;
        }
    }
    return FALSE;
}

//===================================================================
// TransVKDBEMode
//================
//
// Japanese only
//
//===================================================================
LRESULT TransVKDBEMode( HIMC hImc, WPARAM wVKDBE )
{
    DWORD fdwConversion,fdwSentence;

    ImmGetConversionStatus(hImc, &fdwConversion, &fdwSentence);

    switch (wVKDBE)
    {
        case VK_DBE_ALPHANUMERIC:
            fdwConversion &= ~IME_CMODE_LANGUAGE;
            break;

        case VK_DBE_KATAKANA:
            fdwConversion |= (IME_CMODE_JAPANESE | IME_CMODE_KATAKANA);
            break;

        case VK_DBE_HIRAGANA:
            fdwConversion &= ~IME_CMODE_KATAKANA;
            fdwConversion |= IME_CMODE_JAPANESE;
            break;

        case VK_DBE_SBCSCHAR:
            fdwConversion &= ~IME_CMODE_FULLSHAPE;
            break;

        case VK_DBE_DBCSCHAR:
            fdwConversion |= IME_CMODE_FULLSHAPE;
            break;

        case VK_DBE_ROMAN:
            fdwConversion |= IME_CMODE_ROMAN;
            break;

        case VK_DBE_NOROMAN:
            fdwConversion &= ~IME_CMODE_ROMAN;
            break;

        case VK_DBE_CODEINPUT:
            fdwConversion |= IME_CMODE_CHARCODE;
            break;

        case VK_DBE_NOCODEINPUT:
            fdwConversion &= ~IME_CMODE_CHARCODE;
            break;

        default:
            break;

    }

    return ImmSetConversionStatus(hImc, fdwConversion, fdwSentence);
}

//===================================================================
// IsForegroundThread
//===================
//
// Check if the caller thread has the foreground window.
// If hwnd is specified, the function checks if the creator
// thread of the specified window has the foreground window.
//
//===================================================================
BOOL    IsForegroundThread(HWND hwnd)
{
    HWND  hwndFG;
    DWORD dwThreadId;

    hwndFG = GetForegroundWindow();
    if ( IsWindow( hwnd ) ) {
        dwThreadId = GetWindowThreadProcessId( hwnd, NULL );
    } else {
        dwThreadId = GetCurrentThreadId();
    }
    return ( GetWindowThreadProcessId(hwndFG,NULL) == dwThreadId );
}


//===================================================================
// SetFontForMCWVERTICAL
//======================
//
// Japanese only
//
// set/reset vertical writing font
//
//===================================================================
BOOL  SetFontForMCWVERTICAL( HWND hWndApp, HIMC hImc, LPINPUTCONTEXT pInputContext, BOOL fVert )
{
    LOGFONTW lf;
    PCLIENTIMC pClientImc;

    if ( pInputContext->fdwInit & INIT_LOGFONT) {
    //
    // If a font has ever been set, use it
    //
        BOOL fAnsi;

        memcpy(&lf,&pInputContext->lfFont.W,sizeof(LOGFONTW));
        //
        // check if the input context is unicode
        //
        pClientImc = ImmLockClientImc( hImc );
        if (pClientImc == NULL) {
            return FALSE;
        }
        fAnsi = ! TestICF( pClientImc, IMCF_UNICODE );
        ImmUnlockClientImc( pClientImc );

        if ( fAnsi ) {
            CHAR FaceNameA[ LF_FACESIZE ];

            //
            // we need a temporary buffer because MultiByteToWideChar
            // doesn't allow us to specify src==dest.
            //
            memcpy( FaceNameA, &lf.lfFaceName, LF_FACESIZE );
            MultiByteToWideChar( CP_ACP,
                                 0,
                                 FaceNameA,     // src
                                 LF_FACESIZE,   // size of src
                                 lf.lfFaceName, // destination buffer
                                 LF_FACESIZE ); // size of destination buffer
        }
    } else {
    //
    // system font should be used as the default font
    //
        GetObjectW( GetStockObject(SYSTEM_FONT), sizeof(lf), (LPVOID)&lf );
    }

    //
    // put/remove '@' from the font facename.
    // "@facename" means vertical writing font
    //
    if ( FixLogfont( &lf, fVert ) == FALSE ) {
       return FALSE;
    }

    return MySetCompFont( hWndApp, hImc, &lf );
}

//===================================================================
// FixLogfont
//============
//
// Japanese only
//
// put/remove '@' from the font facename.
// "@facename" means vertical writing font
//
//===================================================================
BOOL FixLogfont(  LPLOGFONTW lplfW, BOOL fVert )
{
    int i;

    if ( fVert ) {
    //
    // convert the specified font to vertical writing font
    //
        lplfW->lfEscapement = 2700;
        lplfW->lfOrientation = 2700;
        if ((lplfW->lfCharSet == SHIFTJIS_CHARSET) && (lplfW->lfFaceName[0] != L'@')) {
            for(i=0;lplfW->lfFaceName[i];++i)    // Search NULL
            if (i > (LF_FACESIZE-2))         // if not remain 2 char
                return FALSE;                // then error
                                         // Because insert @ char

            for( ; i>=0 ; --i )                  // Copy facename from tail
                lplfW->lfFaceName[i+1] = lplfW->lfFaceName[i];

            lplfW->lfFaceName[0] = L'@';         // insert @ character
        }
    } else {
    //
    // convert the specified font to normal font
    //
        lplfW->lfEscapement = 0;
        lplfW->lfOrientation = 0;
        if ((lplfW->lfCharSet == SHIFTJIS_CHARSET) && (lplfW->lfFaceName[0] == L'@'))
            lstrcpynW(lplfW->lfFaceName,&(lplfW->lfFaceName[1]),LF_FACESIZE-1);
    }
    return TRUE;
}


//===================================================================
// MySetCompFont
//==============
//
// Japanese only
//
//===================================================================
BOOL MySetCompFont( HWND hWndApp, HIMC hImc, LPLOGFONTW lplf )
{
    BOOL lRet = FALSE;
    DWORD dwCompat;
    PINPUTCONTEXT pInputContext;
    PCLIENTIMC pClientImc;
    LOGFONTW lfw;
    LPLOGFONTW lplfw = &lfw;
    BOOL fUnicode;

    // BOGUS!!
    // Some application call SendIMEMessage(IME_SETCONVERSIONFONT)
    // when the apps is handling WM_PAINT.
    // New Win95 IME try to draw the UI during calling ImmSetCompositionFont,
    // and WM_PAINT will be sent in the API....
    // To avoid this thing, WINNLS makes the notification to IME and APPS later.
    // ........

    if ( (pInputContext = ImmLockIMC(hImc)) != NULL ) {
        dwCompat = ImmGetAppCompatFlags( hImc );
        pClientImc = ImmLockClientImc(hImc);
        if (pClientImc != NULL) {
            fUnicode = TestICF(pClientImc, IMCF_UNICODE);

            ImmUnlockClientImc(pClientImc);

            if ( fUnicode )
                lplfw = &(pInputContext->lfFont.W);
            else
                LFontAtoLFontW( &(pInputContext->lfFont.A), lplfw );

            if ( RtlEqualMemory(lplfw, lplf, sizeof(LOGFONTA)-LF_FACESIZE)
                 && !lstrcmp(lplfw->lfFaceName, lplf->lfFaceName) ) {

                /*
                 * Don't inform IME ahd UI when logfont is not changed.
                 */
                lRet = TRUE;

            } else if ( dwCompat & IMECOMPAT_UNSYNC31IMEMSG ) {

                memcpy( &(pInputContext->lfFont.W), lplf, sizeof(LOGFONT));
                if ( dwCompat & IMECOMPAT_UNSYNC31IMEMSG2 )
                    /*
                     * BOGUS!! for PageMaker5J
                     */
                    lRet = PostMessage( hWndApp, WM_IME_SYSTEM, IMS_SETCOMPOSITIONFONT, 0 );
                else
                    lRet = MyPostImsMessage( hWndApp, IMS_SETCOMPOSITIONFONT, 0);

            } else {

                lRet = ImmSetCompositionFont( hImc, lplf );

            }
        }
        ImmUnlockIMC( hImc );
    }
    return lRet;
}

//===================================================================
// MySetCompWindow
//================
//
// Japanese only
//
//===================================================================
BOOL MySetCompWindow(
    HWND              hWndApp,
    HIMC              hImc,
    LPCOMPOSITIONFORM lpcof
    )
{
    BOOL fRet = FALSE;
    DWORD dwCompat;
    PINPUTCONTEXT pInputContext;

    // BOGUS!!
    // Some application call SendIMEMessage(IME_SETCONVERSIONWINDOW)
    // when the apps is handling WM_PAINT.
    // New Win95 IME try to draw the UI during calling ImmSetCompositionWindow,
    // and WM_PAINT will be sent in the API....
    // To avoid this thing, WINNLS makes the notification to IME and APPS later.
    // ........
    if ( (pInputContext = ImmLockIMC(hImc)) != NULL ) {
        dwCompat = ImmGetAppCompatFlags( hImc );
        if ( dwCompat & IMECOMPAT_UNSYNC31IMEMSG ) {
            memcpy( &(pInputContext->cfCompForm), lpcof, sizeof(COMPOSITIONFORM));
            if ( dwCompat & IMECOMPAT_UNSYNC31IMEMSG2 ) {
                /*
                 * BOGUS!! for PageMaker5J
                 */
                fRet = PostMessage( hWndApp, WM_IME_SYSTEM, IMS_SETCOMPOSITIONWINDOW, 0 );
            } else {
                fRet = MyPostImsMessage( hWndApp, IMS_SETCOMPOSITIONWINDOW, 0 );
            }
        } else {
            pInputContext->fdw31Compat |= F31COMPAT_CALLFROMWINNLS;
            fRet = ImmSetCompositionWindow( hImc, lpcof );
        }
        ImmUnlockIMC( hImc );
    }
    return fRet;
}

//===================================================================
// MySetCandidateWindow
//=====================
//
// Japanese only
//
//===================================================================
BOOL MySetCandidateWindow( HWND hWndApp, HIMC hImc, LPCANDIDATEFORM lpcaf)
{
    BOOL fRet = FALSE;
    DWORD dwCompat;
    PINPUTCONTEXT pInputContext;

    // BOGUS!!
    // Some application call SendIMEMessage(IME_SETCONVERSIONWINDOW)
    // when the apps is handling WM_PAINT.
    // New Win95 IME try to draw the UI during calling ImmSetCandidateWindow,
    // and WM_PAINT will be sent in the API....
    // To avoid this thing, WINNLS makes the notification to IME and APPS later.
    // ........
    if ( (pInputContext = ImmLockIMC(hImc)) != NULL ) {
        dwCompat = ImmGetAppCompatFlags( hImc );
        if ( dwCompat & IMECOMPAT_UNSYNC31IMEMSG ) {
            memcpy( &(pInputContext->cfCandForm[lpcaf->dwIndex]), lpcaf, sizeof(CANDIDATEFORM));
            fRet = MyPostImsMessage( hWndApp, IMS_SETCANDIDATEPOS, lpcaf->dwIndex );
        } else {
            fRet = ImmSetCandidateWindow( hImc, lpcaf );
        }
        ImmUnlockIMC( hImc );
    }
    return fRet;
}

//===================================================================
// MyPostImsMessage
//==================
//
// Japanese only
//
// BOGUS!!
// Some application call SendIMEMessage(IME_SETCONVERSIONWINDOW)
// when the apps is handling WM_PAINT.
// New Win95 IME try to draw the UI during calling ImmSetCompositionWindow,
// and WM_PAINT will be sent in the API....
// To avoid this thing, WINNLS makes the notification to IME and APPS later.
// ........
//===================================================================
BOOL MyPostImsMessage( HWND hWndApp, WPARAM wParam, LPARAM lParam )
{
    HWND   hDefIMEWnd;
    BOOL   fRet = FALSE;

    hDefIMEWnd = ImmGetDefaultIMEWnd(hWndApp);
    if ( hDefIMEWnd != NULL ) {
        if ( PostMessage( hDefIMEWnd, WM_IME_SYSTEM, wParam, lParam) ) {
            fRet = TRUE;
        }
    }
    return fRet;
}
