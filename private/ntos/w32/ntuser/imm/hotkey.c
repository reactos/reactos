/**************************************************************************\
* Module Name: hotkey.c (corresponds to Win95 hotkey.c)
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* IME hot key management routines for imm32 dll
*
* History:
* 03-Jan-1996 wkwok       Created
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop



//
// internal functions
//
BOOL CIMENonIMEToggle(HIMC hIMC, HKL hKL, HWND hWnd, LANGID langTarget);
BOOL IMENonIMEToggle( HIMC hIMC, HKL hKL, HWND hWnd, BOOL fIME, LANGID langTarget);
BOOL JCloseOpen( HIMC hIMC, HKL hKL, HWND hWnd);
BOOL CSymbolToggle(HIMC hIMC, HKL hKL, HWND hWnd);
BOOL TShapeToggle(HIMC hIMC, HKL hKL, HWND hWnd);
BOOL KEnglishHangul( HIMC hIMC);
BOOL KShapeToggle( HIMC hIMC);
BOOL KHanjaConvert( HIMC hIMC);


/***************************************************************************\
* ImmGetHotKey()
*
* Private API for IMEs and the control panel. The caller specifies
* the IME hotkey ID:dwID. If a hotkey is registered with the specified
* ID, this function returns the modifiers, vkey and hkl of the hotkey.
*
* History:
* 25-Mar-1996 TakaoK       Created
\***************************************************************************/
BOOL WINAPI ImmGetHotKey(
    DWORD dwID,
    PUINT puModifiers,
    PUINT puVKey,
    HKL   *phkl)
{
    if (puModifiers == NULL || puVKey == NULL) {
        return FALSE;
    }
    return NtUserGetImeHotKey( dwID, puModifiers, puVKey, phkl );
}

/**********************************************************************/
/* ImmSimulateHotKey()                                                */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI ImmSimulateHotKey(  // simulate the functionality of that hot key
    HWND  hAppWnd,              // application window handle
    DWORD dwHotKeyID)
{
    HIMC hImc;
    HKL  hKL;
    BOOL fReturn;

    hImc = ImmGetContext( hAppWnd );
    hKL = GetKeyboardLayout( GetWindowThreadProcessId(hAppWnd, NULL) );
    fReturn = HotKeyIDDispatcher( hAppWnd, hImc, hKL, dwHotKeyID);
    ImmReleaseContext( hAppWnd, hImc );
    return fReturn;
}


/***************************************************************************\
* SaveImeHotKey()
*
*  Put/Remove the specified IME hotkey entry from the registry
*
* History:
* 25-Mar-1996 TakaoK       Created
\***************************************************************************/

/**********************************************************************/
/* HotKeyIDDispatcher                                                 */
/* Return Value:                                                      */
/*      TRUE - a hot key processed, FALSE - not processed             */
/**********************************************************************/
BOOL HotKeyIDDispatcher( HWND hWnd, HIMC hImc, HKL hKlCurrent, DWORD dwHotKeyID )
{
    /*
     * Dispatch the IME hotkey event for the specified hImc
     * only if the calling thread owns the hImc.
     */
    if (hImc != NULL_HIMC &&
            GetInputContextThread(hImc) != GetCurrentThreadId()) {
        return FALSE;
    }

    switch ( dwHotKeyID ) {
    case IME_CHOTKEY_IME_NONIME_TOGGLE:
        return CIMENonIMEToggle(hImc, hKlCurrent, hWnd, MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED));

    case IME_THOTKEY_IME_NONIME_TOGGLE:
        return CIMENonIMEToggle(hImc, hKlCurrent, hWnd, MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL));

    case IME_CHOTKEY_SYMBOL_TOGGLE:
    case IME_THOTKEY_SYMBOL_TOGGLE:
        return CSymbolToggle( hImc, hKlCurrent, hWnd);

    case IME_JHOTKEY_CLOSE_OPEN:
        return JCloseOpen( hImc, hKlCurrent, hWnd);

    case IME_KHOTKEY_ENGLISH:           // VK_HANGUL : English/Hangul mode
        return KEnglishHangul( hImc );

    case IME_KHOTKEY_SHAPE_TOGGLE:      // VK_JUNJA : full/half width
        return KShapeToggle( hImc );

    case IME_KHOTKEY_HANJACONVERT:      // VK_HANJA : convert hangul to hanja
        return KHanjaConvert( hImc );

    case IME_CHOTKEY_SHAPE_TOGGLE:
    case IME_THOTKEY_SHAPE_TOGGLE:
        return TShapeToggle( hImc, hKlCurrent, hWnd);

    default:
        /*
         * Direct swithing hotkey should have been handled in the kernel side.
         */
        ImmAssert(dwHotKeyID < IME_HOTKEY_DSWITCH_FIRST || dwHotKeyID > IME_HOTKEY_DSWITCH_LAST);

        if ( dwHotKeyID >= IME_HOTKEY_PRIVATE_FIRST &&
                    dwHotKeyID <= IME_HOTKEY_PRIVATE_LAST ) {

            PIMEDPI pImeDpi;
            BOOL    bRet = FALSE;

            if ( (pImeDpi = ImmLockImeDpi(hKlCurrent)) != NULL ) {

                bRet = (BOOL)(*pImeDpi->pfn.ImeEscape)( hImc,
                                                  IME_ESC_PRIVATE_HOTKEY,
                                                  (PVOID)&dwHotKeyID );
                ImmUnlockImeDpi(pImeDpi);
                return bRet;
            }
        }
    }
    return (FALSE);
}

/**********************************************************************/
/* JCloseOpen()                                                       */
/* Return Value:                                                      */
/*      TRUE - a hot key processed, FALSE - not processed             */
/**********************************************************************/
BOOL JCloseOpen(         // open/close toggle
    HIMC        hIMC,
    HKL         hCurrentKL,
    HWND        hWnd)
{

    if (ImmIsIME(hCurrentKL) &&
            LOWORD(HandleToUlong(hCurrentKL)) == MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT)) {
        //
        // If current KL is IME and its language is Japanese,
        // we only have to switch the open/close status.
        //
        ImmSetOpenStatus( hIMC, !ImmGetOpenStatus(hIMC) );
    } else {
        //
        // If current KL is not IME or its language is not Japanese,
        // we should find the Japanese IME and set it open.
        //
        if (IMENonIMEToggle(hIMC, hCurrentKL, hWnd, FALSE, MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT))) {
            //
            // Mark it so that later we can initialize the fOpen
            // as expected.
            //
            PINPUTCONTEXT pInputContext = ImmLockIMC(hIMC);

            if (pInputContext) {
                pInputContext->fdwDirty |= IMSS_INIT_OPEN;
                ImmUnlockIMC(hIMC);
            }
        }
    }
    return TRUE;

#if 0   // for your reference : old code ported from Win95
    LPINPUTCONTEXT pInputContext;
    PIMEDPI            pImeDpi;


    if ( (pInputContext = ImmLockIMC( hIMC )) == NULL ) {
    //
    // The return value is same as Win95.
    // Not happens so often any way.
    //
        return TRUE;
    }

    pImeDpi = ImmLockImeDpi( hCurrentKL );
    if ( pImeDpi != NULL ) {
    //
    // update Input Context
    //
        pInputContext->fOpen = !pInputContext->fOpen;

    //
    // notify IME
    //
        (*pImeDpi->pfn.NotifyIME)( hIMC,
                                   NI_CONTEXTUPDATED,
                                   0L,
                                   IMC_SETOPENSTATUS );
    //
    // inform UI
    //
        SendMessage(hWnd, WM_IME_NOTIFY, IMN_SETOPENSTATUS, 0L);
        SendMessage(hWnd, WM_IME_SYSTEM, IMS_SETOPENSTATUS, 0L);

        ImmUnlockIMC( hIMC );
        ImmUnlockImeDpi(pImeDpi);
        return TRUE;

    } else {

        if ( !pInputContext->fOpen ) {
            pInputContext->fOpen = TRUE;
            SendMessage(hWnd, WM_IME_NOTIFY, IMN_SETOPENSTATUS, 0L);
            SendMessage(hWnd, WM_IME_SYSTEM, IMS_SETOPENSTATUS, 0L);
        }
        ImmUnlockIMC( hIMC );

        return IMENonIMEToggle(hIMC, hCurrentKL, hWnd, FALSE);
    }
#endif
}


/**********************************************************************/
/* CIMENonIMEToggle()                                                 */
/* Return Value:                                                      */
/*      TRUE - a hot key processed, FALSE - not processed             */
/**********************************************************************/
BOOL CIMENonIMEToggle(   // non-IME and IME toggle
    HIMC        hIMC,
    HKL         hKlCurrent,
    HWND        hWnd,
    LANGID      langId)
{
    if (hWnd == NULL)
        return(FALSE);

    if (!ImmIsIME(hKlCurrent) || LOWORD(HandleToUlong(hKlCurrent)) != langId) {
        //
        // Current keyboard layout is not IME or its language does not match.
        // Let's try to switch to our IME.
        //
        IMENonIMEToggle(hIMC, hKlCurrent, hWnd, FALSE, langId);
        return TRUE;

    } else {

        LPINPUTCONTEXT pInputContext = ImmLockIMC( hIMC );

        if ( pInputContext == NULL ) {
            //
            // returning TRUE even if we didn't change
            //
            return TRUE;
        }
        if (!pInputContext->fOpen) {
            //
            // toggle close to open
            //
            ImmSetOpenStatus(hIMC, TRUE);
            ImmUnlockIMC(hIMC);
            return TRUE;
        } else {
            ImmUnlockIMC(hIMC);
            IMENonIMEToggle(hIMC, hKlCurrent, hWnd, TRUE, 0);
            return TRUE;
        }
    }
}

/**********************************************************************/
/* IMENonIMEToggle()                                                  */
/* Return Value:                                                      */
/*      TRUE - a hot key processed, FALSE - not processed             */
/**********************************************************************/
BOOL IMENonIMEToggle(
    HIMC        hIMC,
    HKL         hCurrentKL,
    HWND        hWnd,
    BOOL        fCurrentIsIME,
    LANGID      langTarget)
{
    HKL  hEnumKL[32], hTargetKL;
    UINT nLayouts, i;
    HKL hPrevKL;

    UNREFERENCED_PARAMETER(hIMC);

    hPrevKL = (HKL)NtUserGetThreadState( UserThreadStatePreviousKeyboardLayout );

    //
    // If we find the same layout in the layout list, let's switch to
    // the layout. If we fail, let's switch to a first-found good
    // layout.
    //

    hTargetKL = NULL;
    nLayouts = GetKeyboardLayoutList(sizeof(hEnumKL)/sizeof(HKL), hEnumKL);

    // LATER:
    // Hmm, looks like we can't simply rely on hPrevKL on multiple lanugage
    // environment..
    //
    if (hPrevKL != NULL) {
        if (langTarget == 0 || LOWORD(HandleToUlong(hPrevKL)) == langTarget) {
            //
            // If langtarget is not specified, or
            // if it matches the previous langauge.
            //
            for (i = 0; i < nLayouts; i++) {
                // valid target HKL
                if (hEnumKL[i] == hPrevKL) {
                    hTargetKL = hPrevKL;
                    break;
                }
            }
        }
    }
    if (hTargetKL == NULL) {
        for (i = 0; i < nLayouts; i++) {
            // find a valid target HKL
            if (fCurrentIsIME ^ ImmIsIME(hEnumKL[i])) {
                if (langTarget != 0 && LOWORD(HandleToUlong(hEnumKL[i])) != langTarget) {
                    // If the target language is specified, check it
                    continue;
                }
                hTargetKL = hEnumKL[i];
                break;
            }
        }
    }
    if (hTargetKL != NULL && hCurrentKL != hTargetKL) {

        // depends on multilingual message and how to get the base charset
        // wait for confirmation of multiingual spec - tmp solution
        PostMessage(hWnd, WM_INPUTLANGCHANGEREQUEST, DEFAULT_CHARSET, (LPARAM)hTargetKL);
    }
    //
    // returning TRUE, even if we failed to switch
    //
    return ImmIsIME(hTargetKL);
}

/**********************************************************************/
/* CSymbolToggle()                                                    */
/* Return Value:                                                      */
/*      TRUE - a hot key processed, FALSE - not processed             */
/**********************************************************************/
BOOL CSymbolToggle(              // symbol & non symbol toggle
    HIMC        hIMC,
    HKL         hKL,
    HWND        hWnd)
{
    LPINPUTCONTEXT pInputContext;

    //
    // Return TRUE even no layout switching - Win95 behavior
    //
    if (hWnd == NULL)
        return(FALSE);

    if ( ! ImmIsIME( hKL ) ) {
        return (FALSE);
    }

    if ( (pInputContext = ImmLockIMC( hIMC )) == NULL ) {
        //
        // The return value is same as Win95.
        // Not happens so often any way.
        //
        return TRUE;
    }

    if (pInputContext->fOpen) {
        //
        // toggle the symbol mode
        //
        ImmSetConversionStatus(hIMC,
                               pInputContext->fdwConversion ^ IME_CMODE_SYMBOL,
                               pInputContext->fdwSentence);
    }
    else {
        //
        // change close -> open
        //
        ImmSetOpenStatus(hIMC, TRUE);
    }

    ImmUnlockIMC(hIMC);
    return (TRUE);

}

/**********************************************************************/
/* TShapeToggle()                                                     */
/* Return Value:                                                      */
/*      TRUE - a hot key processed, FALSE - not processed             */
/**********************************************************************/
BOOL TShapeToggle(               // fullshape & halfshape toggle
    HIMC        hIMC,
    HKL         hKL,
    HWND        hWnd)
{
    LPINPUTCONTEXT pInputContext;

    //
    // Return TRUE even no layout switching - Win95 behavior
    //
    if (hWnd == NULL)
        return(FALSE);

    if ( ! ImmIsIME( hKL ) ) {
        return (FALSE);
    }

    if ( (pInputContext = ImmLockIMC( hIMC )) == NULL ) {
        //
        // The return value is same as Win95.
        // Not happens so often any way.
        //
        return TRUE;
    }

    if (pInputContext->fOpen) {
        //
        // toggle the symbol mode
        //
        ImmSetConversionStatus(hIMC,
                               pInputContext->fdwConversion ^ IME_CMODE_FULLSHAPE,
                               pInputContext->fdwSentence);
    }
    else {
        //
        // change close -> open
        //
        ImmSetOpenStatus(hIMC, TRUE);
    }

    ImmUnlockIMC(hIMC);
    return (TRUE);
}

/**********************************************************************/
/* KEnglishHangul() - Egnlish & Hangeul toggle                       */
/* Return Value:                                                      */
/*      TRUE - a hot key processed, FALSE - not processed             */
/**********************************************************************/
BOOL KEnglishHangul( HIMC hImc )
{
    PINPUTCONTEXT pInputContext;

    if ((pInputContext = ImmLockIMC(hImc)) != NULL) {

        ImmSetConversionStatus(hImc,
                pInputContext->fdwConversion ^ IME_CMODE_HANGEUL,
                pInputContext->fdwSentence);

        if ((pInputContext->fdwConversion & IME_CMODE_HANGEUL) ||
                (pInputContext->fdwConversion & IME_CMODE_FULLSHAPE)) {
            ImmSetOpenStatus(hImc, TRUE);
        } else {
            ImmSetOpenStatus(hImc, FALSE);
        }
        ImmUnlockIMC(hImc);
        return TRUE;
    }

    return FALSE;
}

/**********************************************************************/
/* KShapeToggle() - Fullshape & Halfshape toggle                      */
/* Return Value:                                                      */
/*      TRUE - a hot key processed, FALSE - not processed             */
/**********************************************************************/
BOOL KShapeToggle( HIMC hImc )
{
    PINPUTCONTEXT pInputContext;

    if ( (pInputContext = ImmLockIMC( hImc )) != NULL ) {

        ImmSetConversionStatus(hImc,
            pInputContext->fdwConversion ^ IME_CMODE_FULLSHAPE,
            pInputContext->fdwSentence);

        if ((pInputContext->fdwConversion & IME_CMODE_HANGEUL)
                || (pInputContext->fdwConversion & IME_CMODE_FULLSHAPE))
            ImmSetOpenStatus(hImc, TRUE);
        else
            ImmSetOpenStatus(hImc, FALSE);
        ImmUnlockIMC(hImc);
        return TRUE;
    }

    return FALSE;
}

/**********************************************************************/
/* KHanjaConvert() - Hanja conversion toggle                          */
/* Return Value:                                                      */
/*      TRUE - a hot key processed, FALSE - not processed             */
/**********************************************************************/
BOOL KHanjaConvert( HIMC hImc )
{
    PINPUTCONTEXT pInputContext;

    if ( (pInputContext = ImmLockIMC( hImc )) != NULL ) {

        ImmSetConversionStatus( hImc,
                                pInputContext->fdwConversion ^ IME_CMODE_HANJACONVERT,
                                pInputContext->fdwSentence );

        ImmUnlockIMC( hImc );
        return TRUE;
    }
    return FALSE;
}
