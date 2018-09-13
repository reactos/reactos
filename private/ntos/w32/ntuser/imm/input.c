/**************************************************************************\
* Module Name: input.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* IME key input management routines for imm32 dll
*
* History:
* 01-Apr-1996 takaok       split from hotkey.c
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#ifdef HIRO_DEBUG
#define D(x)    x
#else
#define D(x)
#endif

/***************************************************************************\
* ImmProcessKey (Callback from Win32K.SYS)
*
* Call ImeProcessKey and IME hotkey handler
*
* History:
* 01-Mar-1996 TakaoK       Created
\***************************************************************************/

DWORD WINAPI ImmProcessKey(
    HWND    hWnd,
    HKL     hkl,
    UINT    uVKey,
    LPARAM  lParam,
    DWORD   dwHotKeyID)
{
    HIMC hIMC = ImmGetContext(hWnd);
    PIMEDPI pImeDpi = ImmLockImeDpi(hkl);
    DWORD dwReturn = 0;
#if DBG
    if (dwHotKeyID >= IME_KHOTKEY_FIRST && dwHotKeyID <= IME_KHOTKEY_LAST) {
        TAGMSG2(DBGTAG_IMM, "ImmProcessKey: Kor IME Hotkeys should not come here: dwHotKeyID=%x, uVKey=%x", dwHotKeyID, uVKey);
    }
#endif

    ImmAssert(dwHotKeyID != IME_KHOTKEY_ENGLISH &&
              dwHotKeyID != IME_KHOTKEY_SHAPE_TOGGLE &&
              dwHotKeyID != IME_KHOTKEY_HANJACONVERT);

    //
    // call ImeProcessKey
    //
    if (pImeDpi != NULL) {
        PINPUTCONTEXT pInputContext = ImmLockIMC(hIMC);

        if (pInputContext != NULL) {
            BOOLEAN fTruncateWideVK = FALSE;
            BOOLEAN fCallIme = TRUE;
            BOOLEAN fSkipThisKey = FALSE;

#ifdef LATER

            //
            // if the current imc is not open and IME doesn't need
            // keys when being closed, we don't pass any keyboard
            // input to ime except hotkey and keys that change
            // the keyboard status.
            //
            if ((pImeDpi->fdwProperty & IME_PROP_NO_KEYS_ON_CLOSE) &&
                    !pInputContext->fOpen &&
                    uVKey != VK_SHIFT &&
                    uVKey != VK_CONTROL &&
                    uVKey != VK_CAPITAL &&
                    uVKey != VK_KANA &&
                    uVKey != VK_NUMLOCK &&
                    uVKey != VK_SCROLL) {
                // Check if Korea Hanja conversion mode
                if(!(pimc->fdwConvMode & IME_CMODE_HANJACONVERT)) {
                    fCallIme = FALSE;
                }
            }
            else
#endif
            //
            // Protect IMEs which are unaware of wide virtual keys.
            //
            if ((BYTE)uVKey == VK_PACKET &&
                    (pImeDpi->ImeInfo.fdwProperty & IME_PROP_ACCEPT_WIDE_VKEY) == 0) {

                if (pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE) {
                    //
                    // Since this IME is not ready to accept wide VKey, we should
                    // truncate it.
                    //
                    fTruncateWideVK = TRUE;
                }
                else {
                    //
                    // Hmm, this guy is ANSI IME, and does not declare Wide Vkey awareness.
                    // Let's guess this one is not ready to accept Wide Vkey, so let's not
                    // pass it to this guy.
                    // And if it is opened, we'd better skip this key for safety.
                    //
                    fCallIme = FALSE;
                    if (pInputContext->fOpen) {
                        fSkipThisKey = TRUE;
                    }
                }
            }

            if (fCallIme) {
                PBYTE pbKeyState = (PBYTE)ImmLocalAlloc(0, 256);

                ImmAssert(fSkipThisKey == FALSE);

                if (pbKeyState != NULL) {
                    if (GetKeyboardState(pbKeyState)) {
                        UINT uVKeyIme = uVKey;
                        if (fTruncateWideVK) {
                            uVKeyIme &= 0xffff;
                        }
                        if ( (*pImeDpi->pfn.ImeProcessKey)(hIMC, uVKeyIme, lParam, pbKeyState) ) {
                            //
                            // if the return value of ImeProcessKey is TRUE,
                            // it means the key is the one that the ime is
                            // waiting for.
                            //
                            pInputContext->fChgMsg = TRUE;
                            pInputContext->uSavedVKey = uVKey;
                            dwReturn |= IPHK_PROCESSBYIME;
                        }
                    }
                    ImmLocalFree(pbKeyState);
                }
            }
            else if (fSkipThisKey) {
                dwReturn |= IPHK_SKIPTHISKEY;
                ImmAssert((dwReturn & (IPHK_PROCESSBYIME | IPHK_HOTKEY)) == 0);
            }
            ImmUnlockIMC(hIMC);
        }
        ImmUnlockImeDpi(pImeDpi);
    }

    //
    // call hotkey handler
    //
    if (dwHotKeyID != IME_INVALID_HOTKEY && HotKeyIDDispatcher(hWnd, hIMC, hkl, dwHotKeyID)) {
        // Backward compat:
        // On Japanese system, some applications may want VK_KANJI.
        if ((uVKey != VK_KANJI) ||
                (dwHotKeyID != IME_JHOTKEY_CLOSE_OPEN)) {
            dwReturn |= IPHK_HOTKEY;
        }
    }

    //
    // some 3.x application doesn't like to see
    // VK_PROCESSKEY.
    //
    if (dwReturn & IPHK_PROCESSBYIME) {

        DWORD dwImeCompat = ImmGetAppCompatFlags(hIMC);

        if (dwImeCompat & IMECOMPAT_NOVKPROCESSKEY) {

            // Korea 3.x application doesn't like to see dummy finalize VK_PROCESSKEY
            // and IME hot key.

            if ( PRIMARYLANGID(LANGIDFROMLCID(GetSystemDefaultLCID())) == LANG_KOREAN &&
                 ( (uVKey == VK_PROCESSKEY) || (dwReturn & IPHK_HOTKEY) ) ) {
                ImmReleaseContext(hWnd, hIMC);
                return dwReturn;
            }

            ImmTranslateMessage(hWnd, WM_KEYDOWN, VK_PROCESSKEY, lParam);
            dwReturn &= ~IPHK_PROCESSBYIME;
            dwReturn |= IPHK_SKIPTHISKEY;
        }
    }
    ImmReleaseContext(hWnd, hIMC);

    return dwReturn;
}

#define TRANSMSGCOUNT 256

/***************************************************************************\
* ImmTranslateMessage (Called from user\client\ntstubs.c\TranslateMessage())
*
* Call ImeToAsciiEx()
*
* History:
* 01-Mar-1996 TakaoK       Created
\***************************************************************************/
BOOL ImmTranslateMessage(
    HWND        hwnd,
    UINT        message,
    WPARAM      wParam,
    LPARAM      lParam)
{
    HIMC hImc;
    PINPUTCONTEXT pInputContext;
    BOOL fReturn = FALSE;
    HKL  hkl;
    PIMEDPI pImeDpi = NULL;
    PBYTE pbKeyState;
    PTRANSMSG pTransMsg;
    PTRANSMSGLIST pTransMsgList;
    DWORD dwSize;
    UINT uVKey;
    INT iNum;

    UNREFERENCED_PARAMETER(wParam);

    //
    // we're interested in only those keyboard messages.
    //
    switch (message) {
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
        break;
    default:
        return FALSE;
    }

    //
    // input context is necessary for further handling
    //
    hImc = ImmGetContext(hwnd);
    pInputContext = ImmLockIMC(hImc);
    if (pInputContext == NULL) {
        ImmReleaseContext(hwnd, hImc);
        return FALSE;
    }

    //
    // At first, handle VK_PROCESSKEY generated by IME.
    //
    if (!pInputContext->fChgMsg) {

        if ((iNum=pInputContext->dwNumMsgBuf) != 0) {

            pTransMsg = (PTRANSMSG)ImmLockIMCC(pInputContext->hMsgBuf);
            if (pTransMsg != NULL) {
                ImmPostMessages(hwnd, hImc, iNum, pTransMsg);
                ImmUnlockIMCC(pInputContext->hMsgBuf);
                fReturn = TRUE;
            }

            pInputContext->dwNumMsgBuf = 0;
        }
        goto ExitITM;
    }

    pInputContext->fChgMsg = FALSE;

    //
    // retrieve the keyboard layout and IME entry points
    //
    hkl = GetKeyboardLayout( GetWindowThreadProcessId(hwnd, NULL) );
    pImeDpi = ImmLockImeDpi(hkl);
    if (pImeDpi == NULL) {
        RIPMSG1(RIP_WARNING, "ImmTranslateMessage pImeDpi is NULL(hkl=%x)", hkl);
        goto ExitITM;
    }

    pbKeyState = ImmLocalAlloc(0, 256);
    if ( pbKeyState == NULL ) {
        RIPMSG0(RIP_WARNING, "ImmTranslateMessage out of memory" );
        goto ExitITM;
    }

    if (!GetKeyboardState(pbKeyState)) {
        RIPMSG0(RIP_WARNING, "ImmTranslateMessage GetKeyboardState() failed" );
        ImmLocalFree( pbKeyState );
        goto ExitITM;
    }

    //
    // Translate the saved vkey into character code if needed
    //
    uVKey = pInputContext->uSavedVKey;

    if (pImeDpi->ImeInfo.fdwProperty & IME_PROP_KBD_CHAR_FIRST) {

        if (pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE) {
            WCHAR wcTemp;

            iNum = ToUnicode(pInputContext->uSavedVKey, // virtual-key code
                             HIWORD(lParam),            // scan code
                             pbKeyState,                // key-state array
                             &wcTemp,                   // buffer for translated key
                             1,                         // size of buffer
                             0);
            if (iNum == 1) {
                //
                // hi word            : unicode character code
                // hi byte of lo word : zero
                // lo byte of lo word : virtual key
                //
                uVKey = (uVKey & 0x00ff) | ((UINT)wcTemp << 16);
            }

        } else {
            WORD wTemp = 0;

            iNum = ToAsciiEx(pInputContext->uSavedVKey, // virtual-key code
                             HIWORD(lParam),            // scan code
                             pbKeyState,                // key-state array
                             &wTemp,                    // buffer for translated key
                             0,                         // active-menu flag
                             hkl);
            ImmAssert(iNum <= 2);
            if (iNum > 0) {
                //
                // hi word            : should be zero
                // hi byte of lo word : character code
                // lo byte of lo word : virtual key
                //
                uVKey = (uVKey & 0x00FF) | ((UINT)wTemp << 8);

                if ((BYTE)uVKey == VK_PACKET) {
                    //
                    // If ANSI IME is wide vkey aware, its ImeToAsciiEx will receive the uVKey
                    // as follows:
                    //
                    //  31            24 23                         16 15                8 7             0
                    // +----------------+-----------------------------+-------------------+---------------+
                    // | 24~31:reserved | 16~23:trailing byte(if any) | 8~15:leading byte | 0~7:VK_PACKET |
                    // +----------------+-----------------------------+-------------------+---------------+
                    //
                    ImmAssert(pImeDpi->ImeInfo.fdwProperty & IME_PROP_ACCEPT_WIDE_VKEY);
                }
                else {
                    uVKey &= 0xffff;
                }
            }
        }
    }

    dwSize = FIELD_OFFSET(TRANSMSGLIST, TransMsg)
           + TRANSMSGCOUNT * sizeof(TRANSMSG);

    pTransMsgList = (PTRANSMSGLIST)ImmLocalAlloc(0, dwSize);

    if (pTransMsgList == NULL) {
        RIPMSG0(RIP_WARNING, "ImmTranslateMessage out of memory" );
        ImmLocalFree(pbKeyState);
        goto ExitITM;
    }

    pTransMsgList->uMsgCount = TRANSMSGCOUNT;
    iNum = (*pImeDpi->pfn.ImeToAsciiEx)(uVKey,
                                        HIWORD(lParam),
                                        pbKeyState,
                                        pTransMsgList,
                                        0,
                                        hImc);

    if (iNum > TRANSMSGCOUNT) {

        //
        // The message buffer is not big enough. IME put messages
        // into hMsgBuf in the input context.
        //

        pTransMsg = (PTRANSMSG)ImmLockIMCC(pInputContext->hMsgBuf);
        if (pTransMsg != NULL) {
            ImmPostMessages(hwnd, hImc, iNum, pTransMsg);
            ImmUnlockIMCC(pInputContext->hMsgBuf);
        }

#ifdef LATER
        // Shouldn't we need this ?
        fReturn = TRUE;
#endif

    } else if (iNum > 0) {
        ImmPostMessages(hwnd, hImc, iNum, &pTransMsgList->TransMsg[0]);
        fReturn = TRUE;
    }

    ImmLocalFree(pbKeyState);
    ImmLocalFree(pTransMsgList);

ExitITM:
    ImmUnlockImeDpi(pImeDpi);
    ImmUnlockIMC(hImc);
    ImmReleaseContext(hwnd, hImc);

    return fReturn;
}

/***************************************************************************\
* ImmPostMessages(Called from ImmTranslateMessage() )
*
*  Post IME messages to application. If application is 3.x, messages
*  are translated to old IME messages.
*
* History:
* 01-Mar-1996 TakaoK       Created
\***************************************************************************/

VOID
ImmPostMessages(
    HWND      hWnd,
    HIMC      hImc,
    INT       iNum,
    PTRANSMSG pTransMsg)
{
    INT i;
    BOOL fAnsiIME;
    PCLIENTIMC pClientImc;
    PTRANSMSG pTransMsgTemp, pTransMsgBuf = NULL;

    //
    // Check if the IME is unicode or not.
    // The message buffer contains unicode messages
    // if the IME is unicode.
    //
    pClientImc = ImmLockClientImc(hImc);
    if (pClientImc == NULL) {
        RIPMSG1(RIP_WARNING,
                "ImmPostMessages: Invalid hImc %lx.", hImc);
        return;
    }

    fAnsiIME = ! TestICF(pClientImc, IMCF_UNICODE);
    ImmUnlockClientImc(pClientImc);

    //
    // translate messages to 3.x format if the App's version is 3.x.
    //
    pTransMsgTemp = pTransMsg;
    if (GetClientInfo()->dwExpWinVer < VER40) {
        DWORD dwLangId = PRIMARYLANGID(
                                      LANGIDFROMLCID(
                                                    GetSystemDefaultLCID()));
        if ( (dwLangId == LANG_KOREAN && TransGetLevel(hWnd) == 3) ||
             dwLangId == LANG_JAPANESE ) {

            pTransMsgBuf = ImmLocalAlloc(0, iNum * sizeof(TRANSMSG));
            if (pTransMsgBuf != NULL) {
                RtlCopyMemory(pTransMsgBuf, pTransMsg, iNum * sizeof(TRANSMSG));
                iNum = WINNLSTranslateMessage(iNum,
                                              pTransMsgBuf,
                                              hImc,
                                              fAnsiIME,
                                              dwLangId );
                pTransMsgTemp = pTransMsgBuf;
            }
        }
    }

    for (i = 0; i < iNum; i++) {
        if (fAnsiIME) {
            PostMessageA(hWnd,
                    pTransMsgTemp->message,
                    pTransMsgTemp->wParam,
                    pTransMsgTemp->lParam);
        } else {
            PostMessageW(hWnd,
                    pTransMsgTemp->message,
                    pTransMsgTemp->wParam,
                    pTransMsgTemp->lParam);
        }
        pTransMsgTemp++;
    }

    if (pTransMsgBuf != NULL) {
        ImmLocalFree(pTransMsgBuf);
    }
}

UINT WINNLSTranslateMessage(
    INT       iNum,        // number of messages in the source buffer
    PTRANSMSG pTransMsg,   // source buffer that contains 4.0 style messages
    HIMC      hImc,        // input context handle
    BOOL      fAnsi,       // TRUE if pdwt contains ANSI messages
    DWORD     dwLangId )   // language ID ( KOREAN or JAPANESE )
{
    LPINPUTCONTEXT      pInputContext;
    LPCOMPOSITIONSTRING pCompStr;
    UINT uiRet = 0;

    pInputContext = ImmLockIMC(hImc);
    if (pInputContext == NULL) {
        return uiRet;
    }

    pCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC( pInputContext->hCompStr );

    if (dwLangId == LANG_KOREAN) {
        uiRet = WINNLSTranslateMessageK((UINT)iNum,
                                        pTransMsg,
                                        pInputContext,
                                        pCompStr,
                                        fAnsi );
    } else if ( dwLangId == LANG_JAPANESE ) {
        uiRet = WINNLSTranslateMessageJ((UINT)iNum,
                                        pTransMsg,
                                        pInputContext,
                                        pCompStr,
                                        fAnsi );
    }

    if (pCompStr != NULL) {
        ImmUnlockIMCC(pInputContext->hCompStr);
    }
    ImmUnlockIMC(hImc);

    return uiRet;
}

