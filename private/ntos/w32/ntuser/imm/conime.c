/**************************************************************************\
* Module Name: conime.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* client side receiveing stubs
*
* History:
* 19-Sep-1995 v-HirShi Created
* 12-Jun-1996 v-HirShi Attached to SUR
\**************************************************************************/
#include "precomp.h"
#pragma hdrstop

#define GUI_VKEY_MASK (0x00ff)

DWORD
ImmProcessKey(
    HWND hWnd,
    HKL  hkl,
    UINT uVKey,
    LPARAM lParam,
    DWORD dwHotKeyID
    ) ;

BOOL
ImmSetActiveContext(
    HWND   hWnd,
    HIMC   hIMC,
    BOOL   fFlag
    ) ;

DWORD
ImmCallImeConsoleIME(
    HWND   hWnd,
    UINT   Message,
    WPARAM wParam,
    LPARAM lParam,
    PUINT  puVKey
    )
/*++

Routine Description:

        Called by Console IME to convert Character
        This routine copy from user\kernel\ntimm.c :: xxxImmProcessKey
        for Console IME could not calls kernel function.

Arguments:

Return Value:
--*/
{
    DWORD          dwReturn ;
    PIMC           pImc ;
    HIMC           hImc ;
    BOOL           fDBERoman ;
    PWND           pwnd ;
    PIMEDPI        pImeDpi;
    HKL            hkl ;

    dwReturn = 0;
    pImc = NULL;
    fDBERoman = FALSE;

    //
    // we're interested in only keyboard messages.
    //
    if ( Message != WM_KEYDOWN    &&
         Message != WM_SYSKEYDOWN &&
         Message != WM_KEYUP      &&
         Message != WM_SYSKEYUP ) {

        return dwReturn;
    }

    hkl = GetKeyboardLayout( GetWindowThreadProcessId(hWnd, NULL) );
    pwnd = ValidateHwnd(hWnd);
    if ( pwnd == NULL) {
        return dwReturn;
    }
    hImc = ImmGetContext(hWnd);
    if ( hImc == NULL_HIMC ){
        return dwReturn;
    }

    *puVKey = (UINT)wParam & GUI_VKEY_MASK;

    //
    // Check input context
    //
    pImc = HMValidateHandle((HANDLE)hImc, TYPE_INPUTCONTEXT);
    if ( pImc == NULL ) {
        return dwReturn;
    }

#ifdef LATER
    //
    // If there is an easy way to check the input context open/close status
    // from the kernel side, IME_PROP_NO_KEYS_ON_CLOSE checking should be
    // done here in kernel side.  [ 3/10/96 takaok]
    //

    //
    // Check IME_PROP_NO_KEYS_ON_CLOSE bit
    //
    // if the current imc is not open and IME doesn't need
    // keys when being closed, we don't pass any keyboard
    // input to ime except hotkey and keys that change
    // the keyboard status.
    //
    if ( (piix->ImeInfo.fdwProperty & IME_PROP_NO_KEYS_ON_CLOSE) &&
         (!pimc->fdwState & IMC_OPEN)                            &&
         uVKey != VK_SHIFT                                       &&  // 0x10
         uVKey != VK_CONTROL                                     &&  // 0x11
         uVKey != VK_CAPITAL                                     &&  // 0x14
         uVKey != VK_KANA                                        &&  // 0x15
         uVKey != VK_NUMLOCK                                     &&  // 0x90
         uVKey != VK_SCROLL )                                        // 0x91
    {
      // Check if Korea Hanja conversion mode
      if( !(pimc->fdwConvMode & IME_CMODE_HANJACONVERT) ) {
          return dwReturn;
      }
    }
#endif

    //
    // if the IME doesn't need key up messages, we don't call ime.
    //
    pImeDpi = ImmLockImeDpi(hkl);
    if ( pImeDpi == NULL ) {
        return dwReturn;
    }

    if ( lParam & 0x80000000 &&          // set if key up, clear if key down
         pImeDpi->ImeInfo.fdwProperty & IME_PROP_IGNORE_UPKEYS )
    {
        return dwReturn;
    }

    //
    // we don't want to handle sys keys since many functions for
    // acceelerators won't work without this
    //
    fDBERoman = (BOOL)( (*puVKey == VK_DBE_ROMAN)            ||
                        (*puVKey == VK_DBE_NOROMAN)          ||
                        (*puVKey == VK_DBE_HIRAGANA)         ||
                        (*puVKey == VK_DBE_KATAKANA)         ||
                        (*puVKey == VK_DBE_CODEINPUT)        ||
                        (*puVKey == VK_DBE_NOCODEINPUT)      ||
                        (*puVKey == VK_DBE_IME_WORDREGISTER) ||
                        (*puVKey == VK_DBE_IME_DIALOG) );

    if (Message == WM_SYSKEYDOWN || Message == WM_SYSKEYUP ) {
        //
        // IME may be waiting for VK_MENU, VK_F10 or VK_DBE_xxx
        //
        if ( *puVKey != VK_MENU && *puVKey != VK_F10 && !fDBERoman ) {
            return dwReturn;
        }
    }

    //
    // check if the IME doesn't need ALT key
    //

    if ( !(pImeDpi->ImeInfo.fdwProperty & IME_PROP_NEED_ALTKEY) ) {
        //
        // IME doesn't need ALT key
        //
        // we don't pass the ALT and ALT+xxx except VK_DBE_xxx keys.
        //
        if ( ! fDBERoman &&
             (*puVKey == VK_MENU || (lParam & 0x20000000))  // KF_ALTDOWN
           )
        {
            return dwReturn;
        }
    }


    dwReturn = ImmProcessKey(hWnd, hkl, *puVKey, lParam, IME_INVALID_HOTKEY ) ;
    return dwReturn;
}

BOOL
ImmSetActiveContextConsoleIME(
    HWND   hWnd,
    BOOL   fFlag
    )

/*++

Routine Description:

    Set this context as active one.

Arguments:

    hWnd         - the get focus window
    fFlag        - get focus or kill focus

Return Value:

--*/

{
    HIMC hImc;

    hImc = ImmGetContext(hWnd) ;
    if (hImc == NULL_HIMC) {
        return FALSE;
    }
    return(ImmSetActiveContext(hWnd, hImc, fFlag)) ;

}

