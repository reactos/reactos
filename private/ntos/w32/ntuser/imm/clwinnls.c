/****************************** Module Header ******************************\
* Module Name: clwinnls.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains all the code for the NT 3.x IMM API functions.
*
* History:
* 11-Jan-1995 wkwok      Created.
* 07-May-1996 takaok     Cleaned up.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

BOOL CheckCountry();
BOOL ImmEnableIME( HWND hwnd, BOOL fEnable );
BOOL IMPGetIMEWorker( HKL hkl, LPIMEPROW lpImeProW );
VOID ConvertImeProWtoA( LPIMEPROA lpImeProA, LPIMEPROW lpImeProW );
LRESULT SendIMEMessageAll( HWND hwndApp, HANDLE lParam, BOOL fAnsi );

BOOL ImmWINNLSEnableIME(
    HWND  hwndApp,
    BOOL  bFlag)
{
    if ( ! CheckCountry() ) {
        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
        return FALSE;
    }
    return ImmEnableIME( hwndApp, bFlag );
}

//
// returns the "enable/disable" state of the
// caller thread's default input context.
//
BOOL ImmWINNLSGetEnableStatus(
    HWND hwndApp)
{
    if ( ! CheckCountry() ) {
        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
        return FALSE;
    }

    return (ImmGetSaveContext(hwndApp, IGSC_WINNLSCHECK) != NULL_HIMC);
}


UINT WINAPI ImmWINNLSGetIMEHotkey(
    HWND hwndIme)
{
    UNREFERENCED_PARAMETER(hwndIme);

    //
    // Win95/NT3.51 behavior, i.e. always return 0.
    //
    return 0;
}


/***************************************************************************\
*
*         IME APIs
*
\***************************************************************************/

LRESULT WINAPI ImmSendIMEMessageExW(
    HWND   hwndApp,
    LPARAM lParam)
{
    return SendIMEMessageAll( hwndApp, (HANDLE)lParam, FALSE );
}

LRESULT WINAPI ImmSendIMEMessageExA(
    HWND   hwndApp,
    LPARAM lParam)
{
    return SendIMEMessageAll( hwndApp, (HANDLE)lParam, TRUE );
}

LRESULT SendIMEMessageAll(
    HWND hwndApp,
    HANDLE hMemImeStruct,
    BOOL fAnsi )
{
    HWND hWnd;
    LPIMESTRUCT lpIme;
    LRESULT lResult;

#ifdef LATER
  // Need for MSTEST30a(32bit)...
  // If different process of hWnd in SendIMEMessageEx, then we should be inter-send messag
on this.
    if (PtiCurrent() != pti) {
        HWND hDefIMEWnd = ImmGetDefaultIMEWnd(hWnd);
        if (hDefIMEWnd)
            return SendMessage(hDefIMEWnd,WM_CONVERTREQUESTEX,(WPARAM)hWnd,lParam);
    }
#endif

    //
    // the passed handle must be the handle of
    // global memory block.
    //
    lpIme = (LPIMESTRUCT)GlobalLock( hMemImeStruct );
    if ( lpIme == NULL ) {
        return (FALSE);
    }

    if ( ! CheckCountry( ) ) {

        lpIme->wParam = IME_RS_INVALID;
        GlobalUnlock( hMemImeStruct );
        return (FALSE);
    }

    //
    // We don't need to handle if it's non-IME layout
    //
    if ( ! ImmIsIME( GetKeyboardLayout(0) ) ) {

        lpIme->wParam = IME_RS_INVALID;
        GlobalUnlock( hMemImeStruct );
        return (FALSE);
    }

    //
    // check if the initialize of IMM has been done.
    //
    if ( !IsWindow(ImmGetDefaultIMEWnd(hwndApp)) ) {
        //
        // for Win3.1/Win95 compatibility
        // we need to return TRUE here.
        //
        // PPT4 calls SendImeMessage at the very
        // early stage of initialization. If we
        // return FALSE here, it thinks IME is
        // not available.
        //
        if ( lpIme->fnc == 0x07 )  // IME_GETVERSION
            //
            // Excel5.0J calls this function at the early stage
            // and we need to return version number.
            //
            lResult = IMEVER_31;
        else
            lResult = TRUE;

        GlobalUnlock( hMemImeStruct );
        return lResult;
    }

    //
    // caller may give us NULL window handle...
    //
    if ( !IsWindow(hwndApp) ) {
        hWnd = GetFocus();
    } else {
        hWnd = hwndApp;
    }

    lResult = TranslateIMESubFunctions( hWnd, lpIme, fAnsi );
    GlobalUnlock( hMemImeStruct );

    return lResult;
}


/***************************************************************************\
*
*        IMP APIs
*
\***************************************************************************/


BOOL WINAPI ImmIMPGetIMEW(
    HWND hwndApp,
    LPIMEPROW lpImeProW)
{
    UNREFERENCED_PARAMETER(hwndApp);

    if ( ! CheckCountry() ) {
        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
        return FALSE;
    }
    return IMPGetIMEWorker( GetKeyboardLayout(0), lpImeProW );

}

BOOL WINAPI ImmIMPGetIMEA(
    HWND hwndApp,
    LPIMEPROA lpImeProA)
{
    IMEPROW ImeProW;

    UNREFERENCED_PARAMETER(hwndApp);

    if ( ! CheckCountry() ) {
        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
        return FALSE;
    }
    if ( IMPGetIMEWorker( GetKeyboardLayout(0), &ImeProW ) ) {
        ConvertImeProWtoA( lpImeProA, &ImeProW );
        return TRUE;
    }
    return FALSE;
}

VOID ConvertImeProWtoA( LPIMEPROA lpImeProA, LPIMEPROW lpImeProW )
{
    lpImeProA->hWnd = lpImeProW->hWnd;
    lpImeProA->InstDate = lpImeProW->InstDate;
    lpImeProA->wVersion = lpImeProW->wVersion;

    WideCharToMultiByte( CP_ACP, 0,
                         lpImeProW->szDescription, -1,
                         lpImeProA->szDescription, sizeof(lpImeProA->szName),
                         NULL, NULL );

    WideCharToMultiByte( CP_ACP, 0,
                         lpImeProW->szName, -1,
                         lpImeProA->szName, sizeof(lpImeProA->szName),
                         NULL, NULL );

    lpImeProA->szOptions[0] = '\0';
}

DATETIME   CleanDate = {0};

BOOL IMPGetIMEWorker( HKL hkl, LPIMEPROW lpImeProW )
{
    IMEINFOEX iiex;

    if ( ImmGetImeInfoEx( &iiex, ImeInfoExKeyboardLayout, (PVOID)&hkl) ) {

        lpImeProW->hWnd = NULL;
        lpImeProW->InstDate = CleanDate;
        lpImeProW->wVersion = iiex.dwImeWinVersion;
        lstrcpynW( lpImeProW->szDescription, iiex.wszImeDescription, 50 );
        lstrcpynW( lpImeProW->szName, iiex.wszImeFile, 80 );
        lstrcpynW( lpImeProW->szOptions, TEXT(""), 1 );

        return TRUE;
    }

    return FALSE;
}


BOOL WINAPI ImmIMPQueryIMEW(
    LPIMEPROW lpImeProW)
{
    BOOL fResult = FALSE;
    INT numLayouts = 0;
    HKL *phklRoot = NULL;
    HKL *phkl = NULL;

    if ( ! CheckCountry() ) {
        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
        return FALSE;
    }

    //
    // get the number of keyboard layouts available
    //
    numLayouts = GetKeyboardLayoutList( 0, NULL );
    if ( numLayouts > 0 ) {
        //
        // allocate the buffer for the array of layouts.
        // +1 for a NULL sentinel
        //
        phklRoot = ImmLocalAlloc( 0, (numLayouts+1) * sizeof(HKL) );
        if ( phklRoot != NULL ) {
            //
            // get the keyboard layouts
            //
            if ( GetKeyboardLayoutList( numLayouts, phklRoot ) == numLayouts ) {
                //
                // put a NULL sentinel at the end of the buffer
                //
                *(phklRoot+numLayouts) = (HKL)NULL;

                if ( lpImeProW->szName[0] == L'\0' ) {
                //
                // This is the first call of IMPQueryIME
                // We will start at the first layout.
                //
                    phkl = phklRoot;

                } else {
                //
                // The caller specifies the name of IME.
                // We will start at the next layout.
                // Note this assumes that the order of keyboard  layouts
                // returned by GetKeyboardLayoutList() is not changed
                // between calls. ( Though actually there is no such
                // guarantee, we ignore the chance of the changing
                // the list of keyboard layouts for now. )
                //
                    IMEINFOEX iiex;
                    //
                    // Let's retrieve the corresponding hkl
                    // from the IME filename specified by the caller.
                    //
                    if ( ImmGetImeInfoEx( &iiex,
                                         ImeInfoExImeFileName,
                                         (PVOID)lpImeProW->szName ) ) {
                        //
                        // let phkl point to the next hkl
                        //
                        phkl = phklRoot;
                        while ( *phkl != NULL ) {
                            if ( *phkl++ == iiex.hkl ) {
                                break;
                            }
                        }
                    }
                }
                if ( phkl != NULL ) {
                    while ( *phkl != NULL ) {
                        //
                        // IMPGetIMEWorker will return FALSE if
                        // the hkl specified is a non-IME layout.
                        //
                        if ( fResult = IMPGetIMEWorker(*phkl++, lpImeProW) ) {
                            break;
                        }
                    }
                }
            }
            ImmLocalFree( phklRoot );
        }
    }
    return fResult;
}

BOOL WINAPI ImmIMPQueryIMEA(
    LPIMEPROA lpImeProA)
{

    IMEPROW    ImeProW;

    if ( ! CheckCountry() ) {
        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
        return FALSE;
    }

    if ( lpImeProA->szName[0] != '\0' ) {
    //
    // Convert MultiByteString(szName) to UnicodeString
    //
        INT i;

        i = MultiByteToWideChar( CP_ACP, (DWORD)MB_PRECOMPOSED,
                                 lpImeProA->szName,
                                 -1,
                                 ImeProW.szName,
                                 (INT)sizeof(ImeProW.szName)/sizeof(WCHAR));
        if ( i == 0 ) {
            return FALSE;
        }

    } else {
        ImeProW.szName[0] = L'\0';
    }

    if ( ImmIMPQueryIMEW( &ImeProW ) ) {
        ConvertImeProWtoA( lpImeProA, &ImeProW );
        return TRUE;
    }
    return FALSE;
}

BOOL WINAPI ImmIMPSetIMEW(
    HWND hwndApp,
    LPIMEPROW lpImeProW)
{
    IMEINFOEX iiex;
    HKL hkl = NULL;

    UNREFERENCED_PARAMETER(hwndApp);

    if ( ! CheckCountry() ) {
        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
        return FALSE;
    }

    if ( lpImeProW->szName[0] != L'\0' ) {
    //
    // IME name is specified. Switch to the IME specified.
    //
        if ( ImmGetImeInfoEx(&iiex,ImeInfoExImeFileName,(PVOID)lpImeProW->szName) ) {
            hkl = iiex.hkl;
        }
    } else {
    //
    // IME name is not specified. Switch to a non-IME layout
    //
        INT numLayouts;
        HKL   *phkl;
        HKL   *phklRoot;

        numLayouts = GetKeyboardLayoutList( 0, NULL );
        if ( numLayouts > 0 ) {
            phkl = phklRoot = ImmLocalAlloc( 0, (numLayouts + 1) * sizeof(HKL) );
            if ( phkl != NULL ) {
                if ( GetKeyboardLayoutList( numLayouts, phkl ) == numLayouts ) {
                    *(phklRoot+numLayouts) = (HKL)NULL;
                    while ( *phkl != NULL ) {
                        if ( ! ImmIsIME( *phkl ) ) {
                            hkl = *phkl;
                            break;
                        }
                        phkl++;
                    }
                    ImmLocalFree( phklRoot );
                }
            }
        }
    }

    if ( hkl != NULL && GetKeyboardLayout(0) != hkl ) {
        HWND hwndFocus;

        hwndFocus = GetFocus();
        if ( hwndFocus != NULL ) {
            PostMessage( hwndFocus,
                         WM_INPUTLANGCHANGEREQUEST,
                         DEFAULT_CHARSET,
                         (LPARAM)hkl);
            return TRUE;
        }
    }
    return FALSE;
}

BOOL WINAPI ImmIMPSetIMEA(
    HWND hwndApp,
    LPIMEPROA lpImeProA)
{
    IMEPROW ImeProW;

    UNREFERENCED_PARAMETER(hwndApp);

    if ( ! CheckCountry() ) {
        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
        return FALSE;
    }

    if ( lpImeProA->szName[0] != '\0' ) {
    //
    // Convert MultiByteString(szName) to UnicodeString
    //
        INT i;

        i = MultiByteToWideChar( CP_ACP, (DWORD)MB_PRECOMPOSED,
                                 lpImeProA->szName,
                                 -1,
                                 ImeProW.szName,
                                 (INT)sizeof(ImeProW.szName)/sizeof(WCHAR));
        if ( i == 0 ) {
            return FALSE;
        }

    } else {
        ImeProW.szName[0] = L'\0';
    }
    return ImmIMPSetIMEW(hwndApp, &ImeProW);
}

//
// if the "enable/disable" state of the default input context
// of the caller thread is same as the state specified by
// fEnalble parameter, this function does nothing but returns
// the current "enable/disable" state.
//
// if fEnable is FALSE, this function disables the default
// input context of caller thread.
//
// if fEnable is TRUE, this function enables the default
// input context of caller thread.
//
//
BOOL ImmEnableIME(
    HWND hwnd,
    BOOL fEnable
    )
{
    HIMC hImc;
    PCLIENTIMC pClientImc;
    BOOL fCurrentState;
    HWND hwndFocus;
    BOOL fImeInitialized;

    //
    // Get the caller thread's default input context
    //
    hImc = (HIMC)NtUserGetThreadState(UserThreadStateDefaultInputContext);
    if ( hImc == NULL_HIMC ) {
        return FALSE;
    }
    pClientImc = ImmLockClientImc( hImc );
    if ( pClientImc == NULL ) {
        return FALSE;
    }

    //
    // we will return the curren t"enable/disable" state of the input context
    //
    fCurrentState =  TestICF(pClientImc, IMCF_WINNLSDISABLE) ? FALSE : TRUE;

    //
    // if the current thread (caller thread) doesn't have the focus window,
    // UI windows will not be updated. When we're called later, we will end
    // up to just return the fCurrentState without calling ImmSetActiveContext.
    // To avoid that, the "same status" check below is disabled...

    if ( (fCurrentState && fEnable) || (!fCurrentState && !fEnable) ) {
       ImmUnlockClientImc( pClientImc );
        //
        // nothing has been changed. return the current state
        //
        return fCurrentState;
    }


    if ( ! IsWindow(hwnd) ) {
        hwndFocus = GetFocus();
    } else {
        hwndFocus = hwnd;
    }

    //
    // check if the initialize of IMM has been done.
    //
    if ( IsWindow(ImmGetDefaultIMEWnd(hwndFocus)) ) {
        fImeInitialized = TRUE;
    } else {
        fImeInitialized = FALSE;
    }

    if ( fImeInitialized ) {
        if ( ! fEnable ) {
        //
        // we're going to disable the target IMC
        //
            //
            // make the target IMC non-active
            //
            ImmSetActiveContext( hwndFocus, hImc, FALSE );

        } else {
        //
        // we're going to enable the target IMC
        //
            //
            // make NULL context non-active
            //
            ImmSetActiveContext( hwndFocus, NULL_HIMC, FALSE );
        }
    }

    //
    // update the state of the input context
    //
    if ( fEnable )
        ClrICF( pClientImc, IMCF_WINNLSDISABLE );
    else
        SetICF( pClientImc, IMCF_WINNLSDISABLE );
    ImmUnlockClientImc( pClientImc );


    if ( fImeInitialized ) {
        if ( fEnable ) {
        //
        // we're going to enable the target IMC
        //
            //
            // make the target IMC active
            //
            ImmSetActiveContext( hwndFocus, hImc, TRUE );
        } else {
        //
        // we're going to disable the target IMC
        //
            //
            // make NULL context active
            //
            ImmSetActiveContext( hwndFocus, NULL_HIMC, TRUE );
        }
    }

    //
    // the return value is previous state
    //
    return fCurrentState;
}

BOOL CheckCountry()
{
    WORD LangId;

    LangId = PRIMARYLANGID(LANGIDFROMLCID(GetSystemDefaultLCID()));
    if ( LangId == LANG_JAPANESE || LangId == LANG_KOREAN ) {
        return TRUE;
    }
    return FALSE;
}
