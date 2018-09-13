/****************************** Module Header ******************************\
* Module Name: tounicod.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* History:
* 02-08-92 GregoryW      Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*
 *     "To a new truth there is nothing more hurtful than an old error."
 *             - Johann Wolfgang von Goethe (1749-1832)
 */

/*
 * macros used locally to make life easier
 */
#define ISCAPSLOCKON(pf) (TestKeyToggleBit(pf, VK_CAPITAL) != 0)
#define ISNUMLOCKON(pf)  (TestKeyToggleBit(pf, VK_NUMLOCK) != 0)
#define ISSHIFTDOWN(w)   (w & 0x01)
#define ISKANALOCKON(pf) (TestKeyToggleBit(pf, VK_KANA)    != 0)

WCHAR xxxClientCharToWchar(
    IN WORD CodePage,
    IN WORD wch);

/***************************************************************************\
* _ToUnicodeEx (API)
*
* This routine provides Unicode translation for the virtual key code
* passed in.
*
* History:
* 02-10-92 GregoryW    Created.
* 01-23-95 GregoryW    Expanded from _ToUnicode to _ToUnicodeEx
\***************************************************************************/
int xxxToUnicodeEx(
    UINT wVirtKey,
    UINT wScanCode,
    CONST BYTE *pbKeyState,
    LPWSTR pwszBuff,
    int cchBuff,
    UINT wKeyFlags,
    HKL hkl)
{
    int i;
    BYTE afKeyState[CBKEYSTATE];
    DWORD dwDummy;

    /*
     * pKeyState is an array of 256 bytes, each byte representing the
     * following virtual key state: 0x80 means down, 0x01 means toggled.
     * InternalToUnicode() takes an array of bits, so pKeyState needs to
     * be translated. _ToAscii only a public api and rarely gets called,
     * so this is no big deal.
     */
    for (i = 0; i < 256; i++, pbKeyState++) {
        if (*pbKeyState & 0x80) {
            SetKeyDownBit(afKeyState, i);
        } else {
            ClearKeyDownBit(afKeyState, i);
        }

        if (*pbKeyState & 0x01) {
            SetKeyToggleBit(afKeyState, i);
        } else {
            ClearKeyToggleBit(afKeyState, i);
        }
    }

    i = xxxInternalToUnicode(wVirtKey, wScanCode, afKeyState, pwszBuff, cchBuff,
            wKeyFlags, &dwDummy, hkl);


    return i;
}

int ComposeDeadKeys(
    PKL pkl,
    PDEADKEY pDeadKey,
    WCHAR wchTyped,
    WORD *pUniChar,
    INT cChar,
    BOOL bBreak)
{
   /*
    * Attempt to compose this sequence:
    */
   DWORD dwBoth;

   TAGMSG4(DBGTAG_ToUnicode | RIP_THERESMORE,
           "ComposeDeadKeys dead '%C'(%x)+base '%C'(%x)",
           pkl->wchDiacritic, pkl->wchDiacritic,
           wchTyped, wchTyped);
   TAGMSG2(DBGTAG_ToUnicode | RIP_NONAME | RIP_THERESMORE,
           "cChar = %d, bBreak = %d", cChar, bBreak);
   UserAssert(pDeadKey);

   if (cChar < 1) {
       TAGMSG0(DBGTAG_ToUnicode | RIP_NONAME,
               "return 0 because cChar < 1");
       return 0;
   }

   /*
    * Use the layout's built-in table for dead char composition
    */
   dwBoth = MAKELONG(wchTyped, pkl->wchDiacritic);

   if (pDeadKey != NULL) {
       /*
        * Don't let character upstrokes erase the cached dead char: else
        * if this was the dead char key again (being released after the
        * AltGr is released) the dead char would be prematurely cleared.
        */
       if (!bBreak) {
           pkl->wchDiacritic = 0;
       }
       while (pDeadKey->dwBoth != 0) {
           if (pDeadKey->dwBoth == dwBoth) {
               /*
                * found a composition
                */
               if (pDeadKey->uFlags & DKF_DEAD) {
                   /*
                    * Dead again! Save the new 'dead' key
                    */
                   if (!bBreak) {
                       pkl->wchDiacritic = (WORD)pDeadKey->wchComposed;
                   }
                   TAGMSG2(DBGTAG_ToUnicode | RIP_NONAME,
                           "return -1 with dead char '%C'(%x)",
                           pkl->wchDiacritic, pkl->wchDiacritic);
                   return -1;
               }
               *pUniChar = (WORD)pDeadKey->wchComposed;
               TAGMSG2(DBGTAG_ToUnicode | RIP_NONAME,
                       "return 1 with char '%C'(%x)",
                       *pUniChar, *pUniChar);
               return 1;
           }
           pDeadKey++;
       }
   }
   *pUniChar++ = HIWORD(dwBoth);
   if (cChar > 1) {
       *pUniChar = LOWORD(dwBoth);
       TAGMSG4(DBGTAG_ToUnicode | RIP_NONAME,
               "return 2 with uncomposed chars '%C'(%x), '%C'(%x)",
               *(pUniChar-1), *(pUniChar-1), *pUniChar, *pUniChar);
       return 2;
   }
   TAGMSG2(DBGTAG_ToUnicode | RIP_NONAME | RIP_THERESMORE,
           "return 1 - only one char '%C'(%x) because cChar is 1, '%C'(%x)",
           *(pUniChar-1), *(pUniChar-1));
   TAGMSG2(DBGTAG_ToUnicode | RIP_NONAME,
           "  the second char would have been '%C'(%x)",
           LOWORD(dwBoth), LOWORD(dwBoth));
   return 1;
}


/*
 * TranslateInjectedVKey
 *
 * Returns the number of characters (cch) translated.
 *
 * Note on VK_PACKET:
 * Currently, the only purpose of VK_PACKET is to inject a Unicode character
 * into the input stream, but it is intended to be extensible to include other
 * manipulations of the input stream (including the message loop so that IMEs
 * can be involved).  For example, we might send commands to the IME or other
 * parts of the system.
 * For Unicode character injection, we tried widening virtual keys to 32 bits
 * of the form nnnn00e7, where nnnn is 0x0000 - 0xFFFF (representing Unicode
 * characters 0x0000 - 0xFFFF) See KEYEVENTF_UNICODE.
 * But many apps truncate wParam to 16-bits (poorly ported from 16-bits?) and
 * several AV with these VKs (indexing into a table by WM_KEYDOWN wParam?) so
 * we have to cache the character in pti->wchInjected for TranslateMessage to
 * pick up (cf. GetMessagePos, GetMessageExtraInfo and GetMessageTime)
 */
int TranslateInjectedVKey(
    IN UINT uScanCode,
    OUT PWCHAR awchChars,
    IN UINT uiTMFlags)
{
    UserAssert(LOBYTE(uScanCode) == 0);
    if (!(uScanCode & KBDBREAK) || (uiTMFlags & TM_POSTCHARBREAKS)) {
        awchChars[0] = PtiCurrent()->wchInjected;
        return 1;
    }
    return 0;
}



enum {
    NUMPADCONV_OEMCP = 0,
    NUMPADCONV_HKLCP,
    NUMPADCONV_HEX_HKLCP,
    NUMPADCONV_HEX_UNICODE,
};

#define NUMPADSPC_INVALID   (-1)

int NumPadScanCodeToHex(UINT uScanCode, UINT uVirKey)
{
    if (uScanCode >= SCANCODE_NUMPAD_FIRST && uScanCode <= SCANCODE_NUMPAD_LAST) {
        int digit = aVkNumpad[uScanCode - SCANCODE_NUMPAD_FIRST];

        if (digit != 0xff) {
            return digit - VK_NUMPAD0;
        }
        return NUMPADSPC_INVALID;
    }

    if (gfInNumpadHexInput & NUMPAD_HEXMODE_HL) {
        //
        // Full keyboard
        //
        if (uVirKey >= L'A' && uVirKey <= L'F') {
            return uVirKey - L'A' + 0xa;
        }
        if (uVirKey >= L'0' && uVirKey <= L'9') {
            return uVirKey - L'0';
        }
    }

    return NUMPADSPC_INVALID;
}

/*
 * IsDbcsExemptionForHighAnsi
 *
 * returns TRUE if Unicode to ANSI conversion should be
 * done on CP 1252 (Latin-1).
 *
 * If this function is changed, winsrv's equivalent
 * routine should be changed too.
 */
BOOL IsDbcsExemptionForHighAnsi(
    WORD wCodePage,
    WORD wNumpadChar)
{
    UserAssert(HIBYTE(wNumpadChar) == 0);

    if (wCodePage == CP_JAPANESE && IS_JPN_1BYTE_KATAKANA(wNumpadChar)) {
        /*
         * If hkl is JAPANESE and NumpadChar is in KANA range,
         * NumpadChar should be handled by the input locale.
         */
        return FALSE;
    }
    else if (wNumpadChar >= 0x80 && wNumpadChar <= 0xff) {
        /*
         * Otherwise if NumpadChar is in High ANSI range,
         * use 1252 for conversion.
         */
        return TRUE;
    }

    /*
     * None of the above.
     * This case includes the compound Leading Byte and Trailing Byte,
     * which is larger than 0xff.
     */
    return FALSE;
}

#undef MODIFIER_FOR_ALT_NUMPAD

#define MODIFIER_FOR_ALT_NUMPAD(wModBit) \
    ((((wModBits) & ~KBDKANA) == KBDALT) || (((wModBits) & ~KBDKANA) == (KBDALT | KBDSHIFT)))


int xxxInternalToUnicode(
    IN  UINT   uVirtKey,
    IN  UINT   uScanCode,
    CONST IN PBYTE pfvk,
    OUT PWCHAR awchChars,
    IN  INT    cChar,
    IN  UINT   uiTMFlags,
    OUT PDWORD pdwKeyFlags,
    IN  HKL    hkl)
{
    WORD wModBits;
    WORD nShift;
    WCHAR *pUniChar;
    PVK_TO_WCHARS1 pVK;
    PVK_TO_WCHAR_TABLE pVKT;
    static WORD NumpadChar;
    static WORD VKLastDown;
    static BYTE ConvMode;   // 0 == NUMPADCONV_OEMCP
    PTHREADINFO ptiCurrent = PtiCurrentShared();
    PKL pkl;
    PKBDTABLES pKbdTbl;
    PLIGATURE1 pLigature;

    *pdwKeyFlags = (uScanCode & KBDBREAK);

    if ((hkl == NULL) && ptiCurrent->spklActive) {
        pkl = ptiCurrent->spklActive;
        pKbdTbl = pkl->spkf->pKbdTbl;
    } else {
        pkl = HKLtoPKL(ptiCurrent, hkl);
        if (!pkl) {
            return 0;
        }
        pKbdTbl = pkl->spkf->pKbdTbl;
    }
    UserAssert(pkl != NULL);
    UserAssert(pKbdTbl != NULL);

    pUniChar = awchChars;

    uScanCode &= (0xFF | KBDEXT);

    if (*pdwKeyFlags & KBDBREAK) {        // break code processing
        /*
         * Finalize number pad processing
         *
         */
        if (uVirtKey == VK_MENU) {
            if (NumpadChar) {
                if (ConvMode == NUMPADCONV_HEX_UNICODE) {
                    *pUniChar = NumpadChar;
                } else if (ConvMode == NUMPADCONV_OEMCP &&
                        (ptiCurrent->TIF_flags & TIF_CSRSSTHREAD)) {
                    /*
                     * Pass the OEM char to Console to be converted to Unicode
                     * there, since we don't know the OEM codepage it is using.
                     * Set ALTNUMPAD_BIT for console so it knows!
                     */
                    *pdwKeyFlags |= ALTNUMPAD_BIT;
                    *pUniChar = NumpadChar;
                } else {
                    /*
                     * Conversion based on OEMCP or current input language.
                     */
                    WORD wCodePage;

                    if (ConvMode == NUMPADCONV_OEMCP) {
                        // NlsOemCodePage is exported from ntoskrnl.exe.
                        extern __declspec(dllimport) USHORT NlsOemCodePage;

                        wCodePage = (WORD)NlsOemCodePage;
                    } else {
                        wCodePage = pkl->CodePage;
                    }
                    if (IS_DBCS_CODEPAGE(wCodePage)) {
                        if (NumpadChar & (WORD)~0xff) {
                            /*
                             * Might be a double byte character.
                             * Let's swab it so that NumpadChar has LB in LOBYTE,
                             * TB in HIBYTE.
                             */
                            NumpadChar = MAKEWORD(HIBYTE(NumpadChar), LOBYTE(NumpadChar));
                        } else if (IsDbcsExemptionForHighAnsi(wCodePage, NumpadChar)) {
                            /*
                             * FarEast hack:
                             * treat characters in High ANSI area as if they are
                             * the ones of Codepage 1252.
                             */
                            wCodePage = 1252;
                        }
                    } else {
                        /*
                         * Backward compatibility:
                         * Simulate the legacy modulo behavior for non-FarEast keyboard layouts.
                         */
                        NumpadChar &= 0xff;
                    }

                    *pUniChar = xxxClientCharToWchar(wCodePage, NumpadChar);
                }

                /*
                 * Clear Alt-Numpad state, the ALT key-release generates 1 character.
                 */
                VKLastDown = 0;
                ConvMode = NUMPADCONV_OEMCP;
                NumpadChar = 0;
                gfInNumpadHexInput &= ~NUMPAD_HEXMODE_HL;

                return 1;
            } else if (ConvMode != NUMPADCONV_OEMCP) {
                ConvMode = NUMPADCONV_OEMCP;
            }
        } else if (uVirtKey == VKLastDown) {
            /*
             * The most recently depressed key has now come up: we are now
             * ready to accept a new NumPad key for Alt-Numpad processing.
             */
            VKLastDown = 0;
        }
    }

    if (!(*pdwKeyFlags & KBDBREAK) || (uiTMFlags & TM_POSTCHARBREAKS)) {
        /*
         * Get the character modification bits.
         * The bit-mask (wModBits) encodes depressed modifier keys:
         * these bits are commonly KBDSHIFT, KBDALT and/or KBDCTRL
         * (representing Shift, Alt and Ctrl keys respectively)
         */
        wModBits = GetModifierBits(pKbdTbl->pCharModifiers, pfvk);

        /*
         * If the current shift state is either Alt or Alt-Shift:
         *
         *   1. If a menu is currently displayed then clear the
         *      alt bit from wModBits and proceed with normal
         *      translation.
         *
         *   2. If this is a number pad key then do alt-<numpad>
         *      calculations.
         *
         *   3. Otherwise, clear alt bit and proceed with normal
         *      translation.
         */

        /*
         * Equivalent code is in xxxKeyEvent() to check the
         * low level mode. If you change this code, you may
         * need to change xxxKeyEvent() as well.
         */
        if (!(*pdwKeyFlags & KBDBREAK) && MODIFIER_FOR_ALT_NUMPAD(wModBits)) {
            /*
             * If this is a numeric numpad key
             */
            if ((uiTMFlags & TM_INMENUMODE) == 0) {
                if (gfEnableHexNumpad && uScanCode == SCANCODE_NUMPAD_DOT) {
                    if ((gfInNumpadHexInput & NUMPAD_HEXMODE_HL) == 0) {
                        /*
                         * If the first key is '.', then we're
                         * entering hex input lang input mode.
                         */
                        ConvMode = NUMPADCONV_HEX_HKLCP;
                        /*
                         * Inidicate to the rest of the system
                         * we're in Hex Alt+Numpad mode.
                         */
                        gfInNumpadHexInput |= NUMPAD_HEXMODE_HL;
                        TAGMSG0(DBGTAG_ToUnicode, "NUMPADCONV_HEX_HKLCP");
                    } else {
                        goto ExitNumpadMode;
                    }
                } else if (gfEnableHexNumpad && uScanCode == SCANCODE_NUMPAD_PLUS) {
                    if ((gfInNumpadHexInput & NUMPAD_HEXMODE_HL) == 0) {
                        /*
                         * If the first key is '+', then we're
                         * entering hex UNICODE input mode.
                         */
                        ConvMode = NUMPADCONV_HEX_UNICODE;
                        /*
                         * Inidicate to the rest of the system
                         * we're in Hex Alt+Numpad mode.
                         */
                        gfInNumpadHexInput |= NUMPAD_HEXMODE_HL;
                        TAGMSG0(DBGTAG_ToUnicode, "NUMPADCONV_HEX_UNICODE");
                    } else {
                        goto ExitNumpadMode;
                    }
                } else {
                    int digit = NumPadScanCodeToHex(uScanCode, uVirtKey);

                    if (digit < 0) {
                        goto ExitNumpadMode;
                    }

                    /*
                     * Ignore repeats
                     */
                    if (VKLastDown == uVirtKey) {
                        return 0;
                    }

                    switch (ConvMode) {
                    case NUMPADCONV_HEX_HKLCP:
                    case NUMPADCONV_HEX_UNICODE:
                        /*
                         * Input is treated as hex number.
                         */
                        TAGMSG1(DBGTAG_ToUnicode, "->NUMPADCONV_HEX_*: old NumpadChar=%02x\n", NumpadChar);
                        NumpadChar = NumpadChar * 0x10 + digit;
                        TAGMSG1(DBGTAG_ToUnicode, "<-NUMPADCONV_HEX_*: new NumpadChar=%02x\n", NumpadChar);
                        break;
                    default:
                       /*
                        * Input is treated as decimal number.
                        */
                       NumpadChar = NumpadChar * 10 + digit;

                       /*
                        * Do Alt-Numpad0 processing
                        */
                       if (NumpadChar == 0 && digit == 0) {
                           ConvMode = NUMPADCONV_HKLCP;
                       }
                       break;
                    }
                }
                VKLastDown = (WORD)uVirtKey;
            } else {
ExitNumpadMode:
                /*
                 * Clear Alt-Numpad state and the ALT shift state.
                 */
                VKLastDown = 0;
                ConvMode = NUMPADCONV_OEMCP;
                NumpadChar = 0;
                wModBits &= ~KBDALT;
                gfInNumpadHexInput &= ~NUMPAD_HEXMODE_HL;
            }
        }

        /*
         * LShift/RSHift+Backspace -> Left-to-Right and Right-to-Left marker
         */
        if ((uVirtKey == VK_BACK) && (pKbdTbl->fLocaleFlags & KLLF_LRM_RLM)) {
            if (TestKeyDownBit(pfvk, VK_LSHIFT)) {
                *pUniChar = 0x200E; // LRM
                return 1;
            } else if (TestKeyDownBit(pfvk, VK_RSHIFT)) {
                *pUniChar = 0x200F; // RLM
                return 1;
            }
        } else if (((WORD)uVirtKey == VK_PACKET) && (LOBYTE(uScanCode) == 0)) {
            return TranslateInjectedVKey(uScanCode, awchChars, uiTMFlags);
        }

        /*
         * Scan through all the shift-state tables until a matching Virtual
         * Key is found.
         */
        for (pVKT = pKbdTbl->pVkToWcharTable; pVKT->pVkToWchars != NULL; pVKT++) {
            pVK = pVKT->pVkToWchars;
            while (pVK->VirtualKey != 0) {
                if (pVK->VirtualKey == (BYTE)uVirtKey) {
                    goto VK_Found;
                }
                pVK = (PVK_TO_WCHARS1)((PBYTE)pVK + pVKT->cbSize);
            }
        }

        /*
         * Not found: virtual key is not a character.
         */
        goto ReturnBadCharacter;

VK_Found:
        /*
         * The virtual key has been found in table pVKT, at entry pVK
         */

        /*
         * If KanaLock affects this key and it is on: toggle KANA state
         * only if no other state is on. "KANALOK" attributes only exist
         * in Japanese keyboard layout, and only Japanese keyboard hardware
         * can be "KANA" lock on state.
         */
        if ((pVK->Attributes & KANALOK) && (ISKANALOCKON(pfvk))) {
            wModBits |= KBDKANA;
        } else
        /*
         * If CapsLock affects this key and it is on: toggle SHIFT state
         * only if no other state is on.
         * (CapsLock doesn't affect SHIFT state if Ctrl or Alt are down).
         * OR
         * If CapsLockAltGr affects this key and it is on: toggle SHIFT
         * state only if both Alt & Control are down.
         * (CapsLockAltGr only affects SHIFT if AltGr is being used).
         */
        if ((pVK->Attributes & CAPLOK) && ((wModBits & ~KBDSHIFT) == 0) &&
                ISCAPSLOCKON(pfvk)) {
            wModBits ^= KBDSHIFT;
        } else if ((pVK->Attributes & CAPLOKALTGR) &&
                ((wModBits & (KBDALT | KBDCTRL)) == (KBDALT | KBDCTRL)) &&
                ISCAPSLOCKON(pfvk)) {
            wModBits ^= KBDSHIFT;
        }

        /*
         * If SGCAPS affects this key and CapsLock is on: use the next entry
         * in the table, but not is Ctrl or Alt are down.
         * (SGCAPS is used in Swiss-German, Czech and Czech 101 layouts)
         */
        if ((pVK->Attributes & SGCAPS) && ((wModBits & ~KBDSHIFT) == 0) &&
                ISCAPSLOCKON(pfvk)) {
            pVK = (PVK_TO_WCHARS1)((PBYTE)pVK + pVKT->cbSize);
        }

        /*
         * Convert the shift-state bitmask into one of the enumerated
         * logical shift states.
         */
        nShift = GetModificationNumber(pKbdTbl->pCharModifiers, wModBits);

        if (nShift == SHFT_INVALID) {
            /*
             * An invalid combination of Shifter Keys
             */
            goto ReturnBadCharacter;

        } else if ((nShift < pVKT->nModifications) &&
                (pVK->wch[nShift] != WCH_NONE)) {
            /*
             * There is an entry in the table for this combination of
             * Shift State (nShift) and Virtual Key (uVirtKey).
             */
            if (pVK->wch[nShift] == WCH_DEAD) {
                /*
                 * It is a dead character: the next entry contains
                 * its value.
                 */
                pVK = (PVK_TO_WCHARS1)((PBYTE)pVK + pVKT->cbSize);

                /*
                 * If the previous char was not dead return a dead character.
                 */
                if (pkl->wchDiacritic == 0) {
                    TAGMSG2(DBGTAG_ToUnicode,
                            "xxxInternalToUnicode: new dead char '%C'(%x), goto ReturnDeadCharacter",
                            pVK->wch[nShift], pVK->wch[nShift]);
                    goto ReturnDeadCharacter;
                }
                /*
                 * Else go to ReturnGoodCharacter which will attempt to
                 * compose this dead character with the previous dead char.
                 */
                /*
                 * N.B. NTBUG 6141
                 * If dead key is hit twice in sequence, Win95/98 gives
                 * two composed characters from dead chars...
                 */
                TAGMSG4(DBGTAG_ToUnicode,
                        "xxxInternalToUnicode: 2 dead chars '%C'(%x)+'%C'(%x)",
                        pkl->wchDiacritic, pkl->wchDiacritic,
                        pVK->wch[nShift], pVK->wch[nShift]);
                if (GetAppCompatFlags2(VER40) & GACF2_NOCHAR_DEADKEY) {
                    /*
                     * AppCompat 377217: Publisher calls TranslateMessage and ToUnicode for
                     * the same dead key when it's not expecting real characters.
                     * On NT4, this resulted like "pushing the dead key in the stack and
                     * no character is compossed", but on NT5 with fix to 6141,
                     * two dead keys compose real characters clearing the internal
                     * dead key cache. The app shouldn't call both TranslateMessage and ToUnicode
                     * for the same key stroke in the first place -- in a way the app was working on
                     * NT4 by just a thin luck.
                     * In any case, since the app has been shipped broadly and hard to fix,
                     * let's simulate the NT4 behavior here, but with just one level cache (not the
                     * stack).
                     */
                    goto ReturnDeadCharacter;
                }

                goto ReturnGoodCharacter;

            } else if (pVK->wch[nShift] == WCH_LGTR) {
                /*
                 * It is a ligature.  Look in ligature table for a match.
                 */
                if ((GET_KBD_VERSION(pKbdTbl) == 0) || ((pLigature = pKbdTbl->pLigature) == NULL)) {
                    /*
                     * Hey, where's the table?
                     */
                    xxxMessageBeep(0);
                    goto ReturnBadCharacter;
                }

                while (pLigature->VirtualKey != 0) {
                    int iLig = 0;
                    int cwchT = 0;

                    if ((pLigature->VirtualKey == pVK->VirtualKey) &&
                            (pLigature->ModificationNumber == nShift)) {
                        /*
                         * Found the ligature!
                         */
                        while ((iLig < pKbdTbl->nLgMax) && (cwchT < cChar)) {
                            if (pLigature->wch[iLig] == WCH_NONE) {
                                /*
                                 * End of ligature.
                                 */
                                return cwchT;
                            }
                            if (pkl->wchDiacritic != 0) {
                                int cComposed;
                                /*
                                 * Attempt to compose the previous deadkey with current
                                 * ligature character.  If this generates yet another
                                 * dead key, go round again without adding to pUniChar
                                 * or cwchT.
                                 */
                                cComposed = ComposeDeadKeys(
                                            pkl,
                                            pKbdTbl->pDeadKey,
                                            pLigature->wch[iLig],
                                            pUniChar + cwchT,
                                            cChar - cwchT,
                                            *pdwKeyFlags & KBDBREAK
                                            );
                                if (cComposed > 0) {
                                    cwchT += cComposed;
                                } else {
                                    RIPMSG2(RIP_ERROR, // we really don't expect this
                                            "InternalToUnicode: dead+lig(%x)->dead(%x)",
                                            pLigature->wch[0], pkl->wchDiacritic);
                                }
                            } else {
                                pUniChar[cwchT++] = pLigature->wch[iLig];
                            }
                            iLig++;
                        }
                        return cwchT;
                    }
                    /*
                     * Not a match, try the next entry.
                     */
                    pLigature = (PLIGATURE1)((PBYTE)pLigature + pKbdTbl->cbLgEntry);
                }
                /*
                 * No match found!
                 */
                xxxMessageBeep(0);
                goto ReturnBadCharacter;
            }

            /*
             * Match found: return the unshifted character
             */
            TAGMSG2(DBGTAG_ToUnicode,
                    "xxxInternalToUnicode: Match found '%C'(%x), goto ReturnGoodChar",
                    pVK->wch[nShift], pVK->wch[nShift]);
            goto ReturnGoodCharacter;

        } else if ((wModBits == KBDCTRL) || (wModBits == (KBDCTRL|KBDSHIFT)) ||
             (wModBits == (KBDKANA|KBDCTRL)) || (wModBits == (KBDKANA|KBDCTRL|KBDSHIFT))) {
            /*
             * There was no entry for this combination of Modification (nShift)
             * and Virtual Key (uVirtKey).  It may still be an ASCII control
             * character though:
             */
            if ((uVirtKey >= 'A') && (uVirtKey <= 'Z')) {
                /*
                 * If the virtual key is in the range A-Z we can convert
                 * it directly to a control character.  Otherwise, we
                 * need to search the control key conversion table for
                 * a match to the virtual key.
                 */
                *pUniChar = (WORD)(uVirtKey & 0x1f);
                return 1;
            } else if ((uVirtKey >= 0xFF61) && (uVirtKey <= 0xFF91)) {
                /*
                 * If the virtual key is in range FF61-FF91 (halfwidth
                 * katakana), we convert it to Virtual scan code with
                 * KANA modifier.
                 */
                *pUniChar = (WORD)(InternalVkKeyScanEx((WCHAR)uVirtKey,pKbdTbl) & 0x1f);
                return 1;
            }
        }
    }

ReturnBadCharacter:
    // pkl->wchDiacritic = 0;
    return 0;

ReturnDeadCharacter:
    *pUniChar = pVK->wch[nShift];

    /*
     * Save 'dead' key: overwrite an existing one.
     */
    if (!(*pdwKeyFlags & KBDBREAK)) {
        pkl->wchDiacritic = *pUniChar;
    }

    UserAssert(pKbdTbl->pDeadKey);

    /*
     * return negative count for dead characters
     */
    return -1;

ReturnGoodCharacter:
    if ((pKbdTbl->pDeadKey != NULL) && (pkl->wchDiacritic != 0)) {
        return ComposeDeadKeys(
                  pkl,
                  pKbdTbl->pDeadKey,
                  pVK->wch[nShift],
                  pUniChar,
                  cChar,
                  *pdwKeyFlags & KBDBREAK
                  );
    }
    *pUniChar = (WORD)pVK->wch[nShift];
    return 1;
}

SHORT InternalVkKeyScanEx(
    WCHAR wchChar,
    PKBDTABLES pKbdTbl)
{
    PVK_TO_WCHARS1 pVK;
    PVK_TO_WCHAR_TABLE pVKT;
    BYTE nShift;
    WORD wModBits;
    WORD wModNumCtrl, wModNumShiftCtrl;
    SHORT shRetvalCtrl = 0;
    SHORT shRetvalShiftCtrl = 0;

    if (pKbdTbl == NULL) {
        pKbdTbl = gspklBaseLayout->spkf->pKbdTbl;
    }

    /*
     * Ctrl and Shift-Control combinations are less favored, so determine
     * the values for nShift which we prefer not to use if at all possible.
     * This is for compatibility with Windows 95/98, which only returns a
     * Ctrl or Shift+Ctrl combo as a last resort. See bugs #78891 & #229141
     */
    wModNumCtrl = GetModificationNumber(pKbdTbl->pCharModifiers, KBDCTRL);
    wModNumShiftCtrl = GetModificationNumber(pKbdTbl->pCharModifiers, KBDSHIFT | KBDCTRL);

    for (pVKT = pKbdTbl->pVkToWcharTable; pVKT->pVkToWchars != NULL; pVKT++) {
        for (pVK = pVKT->pVkToWchars;
                pVK->VirtualKey != 0;
                pVK = (PVK_TO_WCHARS1)((PBYTE)pVK + pVKT->cbSize)) {
            for (nShift = 0; nShift < pVKT->nModifications; nShift++) {
                if (pVK->wch[nShift] == wchChar) {
                    /*
                     * A matching character has been found!
                     */
                    if (pVK->VirtualKey == 0xff) {
                        /*
                         * dead char: back up to previous line to get the VK.
                         */
                        pVK = (PVK_TO_WCHARS1)((PBYTE)pVK - pVKT->cbSize);
                    }

                    /*
                     * If this is the first Ctrl or the first Shift+Ctrl match,
                     * remember in case we don't find any better match.
                     * In the meantime, keep on looking.
                     */
                    if (nShift == wModNumCtrl) {
                        if (shRetvalCtrl == 0) {
                            shRetvalCtrl = (SHORT)MAKEWORD(pVK->VirtualKey, KBDCTRL);
                        }
                    } else if (nShift == wModNumShiftCtrl) {
                        if (shRetvalShiftCtrl == 0) {
                            shRetvalShiftCtrl = (SHORT)MAKEWORD(pVK->VirtualKey, KBDCTRL | KBDSHIFT);
                        }
                    } else {
                        /*
                         * this seems like a very good match!
                         */
                        goto GoodMatchFound;
                    }
                }
            }
        }
    }

    /*
     * Didn't find a good match: use whatever Ctrl/Shift+Ctrl match was found
     */
    if (shRetvalCtrl) {
        return shRetvalCtrl;
    }
    if (shRetvalShiftCtrl) {
        return shRetvalShiftCtrl;
    }

    /*
     * May be a control character not explicitly in the layout tables
     */
    if (wchChar < 0x0020) {
        /*
         * Ctrl+char -> char - 0x40
         */
        return (SHORT)MAKEWORD((wchChar + 0x40), KBDCTRL);
    }
    return -1;

GoodMatchFound:
    /*
     * Scan aModification[] to find nShift: the index will be a bitmask
     * representing the Shifter Keys that need to be pressed to produce
     * this Shift State.
     */
    for (wModBits = 0;
         wModBits <= pKbdTbl->pCharModifiers->wMaxModBits;
         wModBits++)
    {
        if (pKbdTbl->pCharModifiers->ModNumber[wModBits] == nShift) {
            if (pVK->VirtualKey == 0xff) {
                /*
                 * The previous entry contains the actual virtual key in this case.
                 */
                pVK = (PVK_TO_WCHARS1)((PBYTE)pVK - pVKT->cbSize);
            }
            return (SHORT)MAKEWORD(pVK->VirtualKey, wModBits);
        }
    }

    /*
     * huh? should never reach here! (IanJa)
     */
    UserAssertMsg1(FALSE, "InternalVkKeyScanEx error: wchChar = 0x%x", wchChar);
    return -1;
}
