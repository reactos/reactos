/****************************** Module Header ******************************\
* Module Name: imehotky.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Contents:   Manage IME hotkey
*
* There are the following two kind of hotkeys defined in the IME specification.
*
* 1) IME hotkeys that changes the mode/status of current IME
* 2) IME hotkeys that causes IME (keyboard layout) change
*
* History:
* 10-Sep-1995 takaok   Created for NT 3.51.
* 15-Mar-1996 takaok   Ported to NT 4.0
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

PIMEHOTKEYOBJ DeleteImeHotKey(PIMEHOTKEYOBJ *ppHead, PIMEHOTKEYOBJ pDelete);
VOID AddImeHotKey(PIMEHOTKEYOBJ *ppHead, PIMEHOTKEYOBJ pAdd);
PIMEHOTKEYOBJ FindImeHotKeyByKey(PIMEHOTKEYOBJ pHead, UINT uModifyKeys, UINT uRL, UINT uVKey);
PIMEHOTKEYOBJ FindImeHotKeyByID(PIMEHOTKEYOBJ pHead, DWORD dwHotKeyID);
PIMEHOTKEYOBJ FindImeHotKeyByKeyWithLang(PIMEHOTKEYOBJ pHead, UINT uModifyKeys, UINT uRL, UINT uVKey, LANGID langId);


#define L_CHS   MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)
#define L_JPN   MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT)
#define L_KOR   MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT)
#define L_CHT   MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)

enum {
    ILANG_NO_MATCH = 0,         // 0: does not match.
    ILANG_MATCH_SYSTEM,         // 1: matches the system locale
    ILANG_MATCH_THREAD,         // 2: matches the thread locale
    ILANG_MATCH_PERFECT,        // 3: matches the current HKL, or direct KL switching hotkey.
};


// Make sure constants are within the range we expect
#if IME_CHOTKEY_FIRST != 0x10 || IME_JHOTKEY_FIRST != 0x30 || IME_KHOTKEY_FIRST != 0x50 || IME_THOTKEY_FIRST != 0x70
#error unexpected IME_xHOTKEY range !
#endif

LANGID GetHotKeyLangID(DWORD dwHotKeyID)
{
    LANGID langId = -1;
    static CONST LANGID aLangId[] = {
        ~0,             // 0x00 - 0x0f: illegal
        L_CHS, L_CHS,   // 0x10 - 0x2f
        L_JPN, L_JPN,   // 0x30 - 0x4f
        L_KOR, L_KOR,   // 0x50 - 0x6f
        L_CHT, L_CHT,   // 0x70 - 0x8f
    };

    if (dwHotKeyID >= IME_CHOTKEY_FIRST && dwHotKeyID <= IME_THOTKEY_LAST) {
        langId = aLangId[dwHotKeyID >> 4];
    }
    else {
        langId = LANG_NEUTRAL;
    }

    // Because KOR IME does not want IME hot key handling
    UserAssert(langId != L_KOR);

    return langId;
}

BOOL
GetImeHotKey(
    DWORD dwHotKeyID,
    PUINT puModifiers,
    PUINT puVKey,
    HKL   *phKL )
{
    PIMEHOTKEYOBJ ph;

    ph = FindImeHotKeyByID( gpImeHotKeyListHeader, dwHotKeyID );
    if ( ph == NULL ) {
        RIPERR0(ERROR_HOTKEY_NOT_REGISTERED, RIP_VERBOSE, "No such IME hotkey");
        return (FALSE);
    }

    //
    // it is OK for NULL phKL, if the target hKL is NULL
    //
    if ( phKL ) {
       *phKL = ph->hk.hKL;
    } else if ( ph->hk.hKL != NULL ) {
        RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "phKL is null");
        return (FALSE);
    }

    *puModifiers = ph->hk.uModifiers;
    *puVKey = ph->hk.uVKey;

    return (TRUE);
}

//
// Insert/remove the specified IME hotkey into/from
// the IME hotkey list (gpImeHotKeyListHeader).
//
BOOL
SetImeHotKey(
    DWORD  dwHotKeyID,
    UINT   uModifiers,
    UINT   uVKey,
    HKL    hKL,
    DWORD  dwAction )
{
    PIMEHOTKEYOBJ ph;

    switch ( dwAction ) {
    case ISHK_REMOVE:
        ph = FindImeHotKeyByID( gpImeHotKeyListHeader, dwHotKeyID );
        if ( ph != NULL ) {
            if ( DeleteImeHotKey( &gpImeHotKeyListHeader, ph ) == ph ) {
                UserFreePool( ph );
                return ( TRUE );
            } else {
                RIPMSG0( RIP_ERROR, "IME hotkey list is messed up" );
                return FALSE;
            }
        } else {
            RIPERR0( ERROR_INVALID_PARAMETER,
                     RIP_WARNING,
                     "no such IME hotkey registered");
            return FALSE;
        }
        break;

    case ISHK_INITIALIZE:
        ph = gpImeHotKeyListHeader;
        while ( ph != NULL ) {
            PIMEHOTKEYOBJ phNext;

            phNext = ph->pNext;
            UserFreePool( ph );
            ph = phNext;
        }
        gpImeHotKeyListHeader = NULL;
        return TRUE;

    case ISHK_ADD:
        if (dwHotKeyID >= IME_KHOTKEY_FIRST && dwHotKeyID <= IME_KHOTKEY_LAST) {
            // Korean IME does not want any IMM hotkey handling.
            // We should not register any Korean IME hot keys.
            return FALSE;
        }

        if ((WORD)uVKey == VK_PACKET) {
            //
            // VK_PACKET should not be a IME hot key.
            //
            return FALSE;
        }

        ph = FindImeHotKeyByKeyWithLang(gpImeHotKeyListHeader,
                                uModifiers & MOD_MODIFY_KEYS,
                                uModifiers & MOD_BOTH_SIDES,
                                uVKey,
                                GetHotKeyLangID(dwHotKeyID));
        if ( ph != NULL ) {
            if ( ph->hk.dwHotKeyID != dwHotKeyID ) {
                RIPERR0( ERROR_HOTKEY_ALREADY_REGISTERED,
                         RIP_WARNING,
                         "There is an IME hotkey that has the same vkey/modifiers/Lang Id");
                return FALSE;
            }
            // So far we found a hotkey that has the
            // same vkey and same ID.
            // But because modifiers may be slightly
            // different, so go ahead and change it.
        } else {
            //
            // the specified vkey/modifiers combination cound not be found
            // in the hotkey list. The caller may want to change the key
            // assignment of an existing hotkey or add a new hotkey.
            //
            ph = FindImeHotKeyByID( gpImeHotKeyListHeader, dwHotKeyID );
        }

        if ( ph == NULL ) {
        //
        // adding a new hotkey
        //
            ph = (PIMEHOTKEYOBJ)UserAllocPool( sizeof(IMEHOTKEYOBJ), TAG_IMEHOTKEY );
            if ( ph == NULL ) {
                RIPERR0( ERROR_OUTOFMEMORY,
                         RIP_WARNING,
                        "Memory allocation failed in SetImeHotKey");
                return FALSE;
            }
            ph->hk.dwHotKeyID = dwHotKeyID;
            ph->hk.uModifiers = uModifiers;
            ph->hk.uVKey = uVKey;
            ph->hk.hKL = hKL;
            ph->pNext = NULL;
            AddImeHotKey( &gpImeHotKeyListHeader, ph );

        } else {
        //
        // changing an existing hotkey
        //
            ph->hk.uModifiers = uModifiers;
            ph->hk.uVKey = uVKey;
            ph->hk.hKL = hKL;

        }
        return TRUE;
    }

    return FALSE;
}


PIMEHOTKEYOBJ DeleteImeHotKey( PIMEHOTKEYOBJ *ppHead, PIMEHOTKEYOBJ pDelete )
{
    PIMEHOTKEYOBJ ph;

    if ( pDelete == *ppHead ) {
        *ppHead = pDelete->pNext;
        return pDelete;
    }

    for ( ph = *ppHead; ph != NULL; ph = ph->pNext ) {
        if ( ph->pNext == pDelete ) {
            ph->pNext = pDelete->pNext;
            return pDelete;
        }
    }
    return NULL;
}

VOID AddImeHotKey( PIMEHOTKEYOBJ *ppHead, PIMEHOTKEYOBJ pAdd )
{
    PIMEHOTKEYOBJ ph;

    if ( *ppHead == NULL ) {
        *ppHead = pAdd;
    } else {
        ph = *ppHead;
        while( ph->pNext != NULL )
            ph = ph->pNext;
        ph->pNext = pAdd;
    }
    return;
}

VOID FreeImeHotKeys(VOID)
{
    PIMEHOTKEYOBJ phk;

    while (gpImeHotKeyListHeader != NULL) {
        phk = gpImeHotKeyListHeader->pNext;
        UserFreePool(gpImeHotKeyListHeader);
        gpImeHotKeyListHeader = phk;
    }
}


LCID glcidSystem;

int GetLangIdMatchLevel(HKL hkl, LANGID langId)
{

    if (langId == LANG_NEUTRAL) {
        //
        // If langId is LANG_NEUTRAL, the hot key does not depend on
        // the current HKL. Make it perfect match always.
        //
        return ILANG_MATCH_PERFECT;
    }

    {
        LCID lcid;

        if (LOWORD(HandleToUlong(hkl)) == langId) {
            // langId matches the current KL locale
            return ILANG_MATCH_PERFECT;
        }

        lcid = NtCurrentTeb()->CurrentLocale;
        if (LANGIDFROMLCID(lcid) == langId) {
            // langId matches the current thread's locale
            return ILANG_MATCH_THREAD;
        }

        if (glcidSystem == 0) {
            // If we've not got system default locale yet, get it here.
            ZwQueryDefaultLocale(FALSE, &glcidSystem);
        }
        if (LANGIDFROMLCID(glcidSystem) == langId) {
            // langId matches the system locale.
            return ILANG_MATCH_SYSTEM;
        }
    }

    return ILANG_NO_MATCH;
}

////////////////////////////////////////////////////////////////////////
// FindImeHotKeyByKey()
// Return Value:
//      pHotKey - IMEHOTKEY pointer with the key,
//      else NULL - failure
//
// Finds the best matching of IME hot keys considering the current
// input locale.
//
////////////////////////////////////////////////////////////////////////

PIMEHOTKEYOBJ FindImeHotKeyByKey(   // Finds pHotKey with this input key
    PIMEHOTKEYOBJ pHead,
    UINT uModifyKeys,               // the modify keys of this input key
    UINT uRL,                       // the right and left hand side
    UINT uVKey)                     // the input key
{
    PTHREADINFO ptiCurrent = PtiCurrent();
    PIMEHOTKEYOBJ phResult = NULL;
    PIMEHOTKEYOBJ ph;
    HKL hkl = GetActiveHKL();
    WORD langPrimary = PRIMARYLANGID(LOWORD(HandleToUlong(hkl)));
    int iLevel = ILANG_NO_MATCH;

    for (ph = pHead; ph != NULL; ph = ph->pNext) {

        if (ph->hk.uVKey == uVKey) {
            BOOL fDoCheck = FALSE;

            // Check if the modifiers match
            if ((ph->hk.uModifiers & MOD_IGNORE_ALL_MODIFIER)) {
                fDoCheck = TRUE;
            } else if ((ph->hk.uModifiers & MOD_MODIFY_KEYS) != uModifyKeys) {
                continue;
            }

            if ((ph->hk.uModifiers & MOD_BOTH_SIDES) == uRL ||
                    (ph->hk.uModifiers & MOD_BOTH_SIDES) & uRL) {
                fDoCheck = TRUE;
            }

            if (fDoCheck) {
                LANGID langId = GetHotKeyLangID(ph->hk.dwHotKeyID);
                int iMatch = GetLangIdMatchLevel(hkl, langId);

#if 0   // Test only
                if (iMatch != ILANG_NO_MATCH) {
                    DbgPrint("GetIdMatchLevel(%X, %X)=%d\n", hkl, langId);
                }
#endif

                if (iMatch == ILANG_MATCH_PERFECT) {
                    // Perfect match !
                    return ph;
                }

                // If the hotkey is DSWITCH, GetLangIdMatchLevel() must return 3.
                UserAssert(ph->hk.dwHotKeyID < IME_HOTKEY_DSWITCH_FIRST ||
                           ph->hk.dwHotKeyID > IME_HOTKEY_DSWITCH_LAST);

                if (langPrimary == LANG_KOREAN) {
                    // Korean IME wants no hotkeys except the direct
                    // keyboard layout switching hotkeys.
                    continue;
                }

                if (iMatch == ILANG_NO_MATCH) {
                    // Special case for CHT/CHS toggle
                    if (ph->hk.dwHotKeyID == IME_CHOTKEY_IME_NONIME_TOGGLE ||
                            ph->hk.dwHotKeyID == IME_THOTKEY_IME_NONIME_TOGGLE) {
                        //
                        // If the key is for CHT/CHS toggle and the previous
                        // hkl is either CHT/CHS, it is a IME hotkey.
                        //
                        if (LOWORD(HandleToUlong(ptiCurrent->hklPrev)) == langId) {
#if 0   // Test only
                            DbgPrint("FindImeHotKeyByKey() found CHT/CHS hotkey.\n");
#endif
                            return ph;
                        }
                    }
                }
                else if (iMatch > iLevel) {
                    // Current ph is the strongest candidate so far.
                    iLevel = iMatch;
                    phResult = ph;
                }
            }
        }
    }

    return phResult;
}

/**********************************************************************/
/* FindImeHotKeyByID()                                                */
/* Return Value:                                                      */
/*      pHotKey   - IMEHOTKEY pointer with the dwHotKeyID,            */
/*      else NULL - failure                                           */
/**********************************************************************/
PIMEHOTKEYOBJ FindImeHotKeyByID( PIMEHOTKEYOBJ pHead, DWORD dwHotKeyID )
{
    PIMEHOTKEYOBJ ph;

    for ( ph = pHead; ph != NULL; ph = ph->pNext ) {
        if ( ph->hk.dwHotKeyID == dwHotKeyID )
                return (ph);
    }
    return (PIMEHOTKEYOBJ)NULL;
}

/**********************************************************************/
/* FindImeHotKeyByKeyWithLang()                                       */
/* Return Value:                                                      */
/*      pHotKey - IMEHOTKEY pointer with the key,                     */
/*      else NULL - failure                                           */
/**********************************************************************/
PIMEHOTKEYOBJ FindImeHotKeyByKeyWithLang(      // Finds pHotKey with this input key
    PIMEHOTKEYOBJ pHead,
    UINT uModifyKeys,               // the modify keys of this input key
    UINT uRL,                       // the right and left hand side
    UINT uVKey,                     // the input key
    LANGID langIdKey)               // the language id
{
    PIMEHOTKEYOBJ ph;

    for (ph = pHead; ph != NULL; ph = ph->pNext) {

        if (ph->hk.uVKey == uVKey) {
            BOOL fDoCheck = FALSE;

            // Check if the modifiers match
            if ((ph->hk.uModifiers & MOD_IGNORE_ALL_MODIFIER)) {
                fDoCheck = TRUE;
            } else if ((ph->hk.uModifiers & MOD_MODIFY_KEYS) != uModifyKeys) {
                continue;
            }

            if ((ph->hk.uModifiers & MOD_BOTH_SIDES) == uRL ||
                    (ph->hk.uModifiers & MOD_BOTH_SIDES) & uRL) {
                fDoCheck = TRUE;
            }

            if (fDoCheck) {
                LANGID langId = GetHotKeyLangID(ph->hk.dwHotKeyID);

                if (langIdKey == langId || langId == LANG_NEUTRAL) {
                    return ph;
                }
            }
        }
    }

    return NULL;
}

PIMEHOTKEYOBJ
CheckImeHotKey(
    PQ   pq,            // input queue
    UINT uVKey,         // virtual key
    LPARAM lParam       // lparam of WM_KEYxxx message
    )
{
    static UINT uVKeySaved = 0;
    PIMEHOTKEYOBJ ph;
    UINT uModifiers = 0;
    BOOL fKeyUp;

    //
    // early return for key up message
    //
    fKeyUp = ( lParam & 0x80000000 ) ? TRUE : FALSE;
    if ( fKeyUp ) {
        //
        // if the uVKey is not same as the vkey
        // we previously saved, there is no chance
        // that this is a hotkey.
        //
        if ( uVKeySaved != uVKey ) {
            uVKeySaved = 0;
            return NULL;
        }
        uVKeySaved = 0;
        //
        // If it's same, we still need to check
        // the hotkey list because there is a
        // chance that the hotkey list is modified
        // between the key make and break.
        //
    }

    //
    // Current specification doesn't allow us to use a complex
    // hotkey such as LSHIFT+RMENU+SPACE
    //

    //
    // Setup the shift, control, alt key states
    //
    uModifiers |= TestKeyStateDown(pq, VK_LSHIFT) ? (MOD_SHIFT | MOD_LEFT) : 0;
    uModifiers |= TestKeyStateDown(pq, VK_RSHIFT) ? (MOD_SHIFT | MOD_RIGHT) : 0;

    uModifiers |= TestKeyStateDown(pq, VK_LCONTROL) ? (MOD_CONTROL | MOD_LEFT) : 0;
    uModifiers |= TestKeyStateDown(pq, VK_RCONTROL) ? (MOD_CONTROL | MOD_RIGHT) : 0;

    uModifiers |= TestKeyStateDown(pq, VK_LMENU) ? (MOD_ALT | MOD_LEFT) : 0;
    uModifiers |= TestKeyStateDown(pq, VK_RMENU) ? (MOD_ALT | MOD_RIGHT) : 0;

    ph = FindImeHotKeyByKey( gpImeHotKeyListHeader,
                             uModifiers & MOD_MODIFY_KEYS,
                             uModifiers & MOD_BOTH_SIDES,
                             uVKey );

    if ( ph != NULL ) {
        if ( fKeyUp ) {
            if ( ph->hk.uModifiers & MOD_ON_KEYUP ) {
                return ph;
            }
        } else {
            if ( ph->hk.uModifiers & MOD_ON_KEYUP ) {
            //
            // save vkey for next keyup message time
            //
            // when ALT+Z is a hotkey, we don't want
            // to handle #2 as the hotkey sequence.
            // 1) ALT make -> 'Z' make -> 'Z' break
            // 2) 'Z' make -> ALT make -> 'Z' break
            //
                uVKeySaved = uVKey;
            } else {
                return ph;
            }
        }
    }

    return NULL;
}
