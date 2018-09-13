/**************************************************************************\
* Module Name: immhotky.c (user32 side IME hotkey handling)
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* IME hot key management routines for imm32 dll
*
* History:
* 03-Jan-1996 hiroyama      Created
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop


typedef struct tagFE_KEYBOARDS {
    BOOLEAN fJPN : 1;
    BOOLEAN fCHT : 1;
    BOOLEAN fCHS : 1;
    BOOLEAN fKOR : 1;
} FE_KEYBOARDS;

/*
 * Function pointers to registry routines in advapi32.dll.
 */
typedef struct {
    HMODULE hModule;
    LONG (WINAPI* RegCreateKeyW)(HKEY, LPCWSTR, PHKEY);
    LONG (WINAPI* RegOpenKeyW)(HKEY, LPCWSTR, PHKEY);
    LONG (WINAPI* RegCloseKey)(HKEY);
    LONG (WINAPI* RegDeleteKeyW)(HKEY, LPCWSTR);
    LONG (WINAPI* RegCreateKeyExW)(HKEY, LPCWSTR, DWORD, LPWSTR, DWORD, REGSAM, LPSECURITY_ATTRIBUTES, PHKEY, LPDWORD);
    LONG (WINAPI* RegSetValueExW)(HKEY, LPCWSTR, DWORD Reserved, DWORD, CONST BYTE*, DWORD);
    LONG (WINAPI* RegQueryValueExW)(HKEY, LPCWSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
} ADVAPI_FN;

//
// internal functions
//
BOOL CliSaveImeHotKey(DWORD dwID, UINT uModifiers, UINT uVKey, HKL hkl, BOOL fDelete);
BOOL CliImmSetHotKeyWorker(DWORD dwID, UINT uModifiers, UINT uVKey, HKL hkl, DWORD dwAction);
VOID NumToHexAscii(DWORD, PTSTR);
BOOL CliGetImeHotKeysFromRegistry(void);
BOOL CliSetSingleHotKey(PKEY_BASIC_INFORMATION pKeyInfo, HANDLE hKey);
VOID CliSetDefaultImeHotKeys(PCIMEHOTKEY ph, INT num, BOOL fCheckExistingHotKey);
VOID CliGetPreloadKeyboardLayouts(FE_KEYBOARDS* pFeKbds);

//
// IMM hotkey related registry keys under HKEY_CURRENT_USER
//
CONST TCHAR *szaRegImmHotKeys[] = {
    TEXT("Control Panel"),
    TEXT("Input Method"),
    TEXT("Hot Keys"),
    NULL
};

CONST TCHAR szRegImeHotKey[] = TEXT("Control Panel\\Input Method\\Hot Keys");
CONST TCHAR szRegKeyboardPreload[] = TEXT("Keyboard Layout\\Preload");

CONST TCHAR szRegVK[] = TEXT("Virtual Key");
CONST TCHAR szRegMOD[] = TEXT("Key Modifiers");
CONST TCHAR szRegHKL[] = TEXT("Target IME");

//
// Default IME HotKey Tables
//
// CR:takaok - move this to the resource if you have time
//
CONST IMEHOTKEY DefaultHotKeyTableJ[]= {
    {IME_JHOTKEY_CLOSE_OPEN, VK_KANJI, MOD_IGNORE_ALL_MODIFIER, NULL}
};
CONST INT DefaultHotKeyNumJ = sizeof(DefaultHotKeyTableJ) / sizeof(IMEHOTKEY);

CONST IMEHOTKEY DefaultHotKeyTableT[] = {
    { IME_THOTKEY_IME_NONIME_TOGGLE, VK_SPACE, MOD_BOTH_SIDES|MOD_CONTROL, NULL },
    { IME_THOTKEY_SHAPE_TOGGLE, VK_SPACE, MOD_BOTH_SIDES|MOD_SHIFT,  NULL }
};
CONST INT DefaultHotKeyNumT = sizeof(DefaultHotKeyTableT) / sizeof(IMEHOTKEY);

CONST IMEHOTKEY DefaultHotKeyTableC[] = {
    { IME_CHOTKEY_IME_NONIME_TOGGLE, VK_SPACE, MOD_BOTH_SIDES|MOD_CONTROL, NULL },
    { IME_CHOTKEY_SHAPE_TOGGLE, VK_SPACE, MOD_BOTH_SIDES|MOD_SHIFT,  NULL }
};
CONST INT DefaultHotKeyNumC = sizeof(DefaultHotKeyTableC) / sizeof(IMEHOTKEY);

#if 0   // just FYI.
CONST IMEHOTKEY DefaultHotKeyTableK[] = {
    { IME_KHOTKEY_ENGLISH,  VK_HANGEUL, MOD_IGNORE_ALL_MODIFIER,  NULL },
    { IME_KHOTKEY_SHAPE_TOGGLE, VK_JUNJA, MOD_IGNORE_ALL_MODIFIER,  NULL },
    { IME_KHOTKEY_HANJACONVERT, VK_HANJA, MOD_IGNORE_ALL_MODIFIER, NULL }
};
CONST INT DefaultHotKeyNumK = sizeof(DefaultHotKeyTableK) / sizeof(IMEHOTKEY);
#endif

//
// Set language flags.
//
VOID SetFeKeyboardFlags(LANGID langid, FE_KEYBOARDS* pFeKbds)
{
    switch (langid) {
    case MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL):
    case MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_HONGKONG):
        pFeKbds->fCHT = TRUE;
        break;
    case MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED):
    case MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SINGAPORE):
        pFeKbds->fCHS = TRUE;
        break;
    case MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT):
        pFeKbds->fJPN = TRUE;
        break;
    case MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT):
        pFeKbds->fKOR = TRUE;
        break;
    }
}

/***************************************************************************\
* ImmInitializeHotkeys()
*
* Called from user\client\UpdatePerUserSystemParameters()
*
*  Read the User registry and set the IME hotkey.
*
* History:
* 25-Mar-1996 TakaoK       Created
\***************************************************************************/
VOID CliImmInitializeHotKeys(DWORD dwAction, HKL hkl)
{
    FE_KEYBOARDS feKbds = { 0, 0, 0, 0, };
    BOOL fFoundAny;

    UNREFERENCED_PARAMETER(hkl);

    // First, initialize the hotkey list
    CliImmSetHotKeyWorker(0, 0, 0, NULL, ISHK_INITIALIZE);

    // Check if the user has customized IME hotkeys
    // (they're stored in the registry)
    fFoundAny = CliGetImeHotKeysFromRegistry();

    if (dwAction == ISHK_INITIALIZE) {
        TAGMSG0(DBGTAG_IMM, "Setting IME HotKeys for Init.\n");

        // Get the user's default locale and set its flag
        SetFeKeyboardFlags(LANGIDFROMLCID(GetUserDefaultLCID()), &feKbds);

        // Get preloaded keyboards' locales and set their flags
        CliGetPreloadKeyboardLayouts(&feKbds);

    }
    else {
        UINT i;
        UINT nLayouts;
        LPHKL lphkl;

        TAGMSG0(DBGTAG_IMM, "Setting IME HotKeys for Add.\n");

        nLayouts = NtUserGetKeyboardLayoutList(0, NULL);
        if (nLayouts == 0) {
            return;
        }
        lphkl = UserLocalAlloc(0, nLayouts * sizeof(HKL));
        if (lphkl == NULL) {
            return;
        }
        NtUserGetKeyboardLayoutList(nLayouts, lphkl);
        for (i = 0; i < nLayouts; ++i) {
            //
            // Set language flags. By its definition, LOWORD(hkl) is LANGID
            //
            SetFeKeyboardFlags(LOWORD(HandleToUlong(lphkl[i])), &feKbds);
        }
        UserLocalFree(lphkl);
    }

    if (feKbds.fJPN) {
        TAGMSG0(DBGTAG_IMM, "JPN KL Preloaded.\n");
        CliSetDefaultImeHotKeys(DefaultHotKeyTableJ, DefaultHotKeyNumJ, fFoundAny);
    }

    if (feKbds.fKOR) {
        TAGMSG0(DBGTAG_IMM, "KOR KL Preloaded, but KOR hotkeys will not be registered.\n");
    }

    if (feKbds.fCHT) {
        TAGMSG0(DBGTAG_IMM, "CHT KL Preloaded.\n");
        CliSetDefaultImeHotKeys(DefaultHotKeyTableT, DefaultHotKeyNumT, fFoundAny);
    }
    if (feKbds.fCHS) {
        TAGMSG0(DBGTAG_IMM, "CHS KL Preloaded.\n");
        CliSetDefaultImeHotKeys(DefaultHotKeyTableC, DefaultHotKeyNumC, fFoundAny);
    }
}

VOID CliSetDefaultImeHotKeys(PCIMEHOTKEY ph, INT num, BOOL fNeedToCheckExistingHotKey)
{
    IMEHOTKEY hkt;

    while( num-- > 0 ) {
        //
        // Set IME hotkey only if there is no such
        // hotkey in the registry
        //
        if (!fNeedToCheckExistingHotKey ||
                !NtUserGetImeHotKey(ph->dwHotKeyID, &hkt.uModifiers, &hkt.uVKey, &hkt.hKL)) {

            CliImmSetHotKeyWorker(ph->dwHotKeyID,
                                    ph->uModifiers,
                                    ph->uVKey,
                                    ph->hKL,
                                    ISHK_ADD);
        }
        ph++;
    }
}

/***************************************************************************\
* CliGetPreloadKeyboardLayouts()
*
*  Read the User registry and enumerate values in Keyboard Layouts\Preload
* to see which FE languages are to be preloaded.
*
* History:
* 03-Dec-1997 Hiroyama     Created
\***************************************************************************/

VOID CliGetPreloadKeyboardLayouts(FE_KEYBOARDS* pFeKbds)
{
    UINT  i;
    WCHAR szPreLoadee[4];   // up to 999 preloads
    WCHAR lpszName[KL_NAMELENGTH];
    UNICODE_STRING UnicodeString;
    HKL hkl;

    for (i = 1; i < 1000; i++) {
        wsprintf(szPreLoadee, L"%d", i);
        if ((GetPrivateProfileStringW(
                 L"Preload",
                 szPreLoadee,
                 L"",                            // default = NULL
                 lpszName,                       // output buffer
                 KL_NAMELENGTH,
                 L"keyboardlayout.ini") == -1 ) || (*lpszName == L'\0')) {
            break;
        }
        RtlInitUnicodeString(&UnicodeString, lpszName);
        RtlUnicodeStringToInteger(&UnicodeString, 16L, (PULONG)&hkl);

        RIPMSG2(RIP_VERBOSE, "PreLoaded HKL(%d): %08X\n", i, hkl);

        //
        // Set language flags. By its definition, LOWORD(hkl) is LANGID
        //
        SetFeKeyboardFlags(LOWORD(HandleToUlong(hkl)), pFeKbds);
    }
}

BOOL CliGetImeHotKeysFromRegistry()
{
    BOOL    fFoundAny = FALSE;

    HANDLE hCurrentUserKey;
    HANDLE hKeyHotKeys;

    OBJECT_ATTRIBUTES   Obja;
    UNICODE_STRING      SubKeyName;

    NTSTATUS Status;
    ULONG uIndex;

    //
    // Open the current user registry key
    //
    Status = RtlOpenCurrentUser(MAXIMUM_ALLOWED, &hCurrentUserKey);
    if (!NT_SUCCESS(Status)) {
        return fFoundAny;
    }

    RtlInitUnicodeString( &SubKeyName, szRegImeHotKey );
    InitializeObjectAttributes( &Obja,
                                &SubKeyName,
                                OBJ_CASE_INSENSITIVE,
                                hCurrentUserKey,
                                NULL);
    Status = NtOpenKey( &hKeyHotKeys, KEY_READ, &Obja );
    if (!NT_SUCCESS(Status)) {
        NtClose( hCurrentUserKey );
        return fFoundAny;
    }

    for (uIndex = 0; TRUE; uIndex++) {
        BYTE KeyBuffer[sizeof(KEY_BASIC_INFORMATION) + 16 * sizeof(WCHAR)];
        PKEY_BASIC_INFORMATION pKeyInfo;
        ULONG ResultLength;

        pKeyInfo = (PKEY_BASIC_INFORMATION)KeyBuffer;
        Status = NtEnumerateKey(hKeyHotKeys,
                                 uIndex,
                                 KeyBasicInformation,
                                 pKeyInfo,
                                 sizeof( KeyBuffer ),
                                 &ResultLength );

        if (NT_SUCCESS(Status)) {

            if (CliSetSingleHotKey(pKeyInfo, hKeyHotKeys)) {

                    fFoundAny = TRUE;
            }

        } else if (Status == STATUS_NO_MORE_ENTRIES) {
            break;
        }
    }

    NtClose(hKeyHotKeys);
    NtClose(hCurrentUserKey);

    return fFoundAny;
}

DWORD CliReadRegistryValue(HANDLE hKey, PCWSTR pName)
{
    BYTE ValueBuffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 16 * sizeof(UCHAR)];
    PKEY_VALUE_PARTIAL_INFORMATION pKeyValue;
    UNICODE_STRING      ValueName;
    ULONG ResultLength;
    NTSTATUS Status;

    pKeyValue = (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;

    RtlInitUnicodeString(&ValueName, pName);
    Status = NtQueryValueKey(hKey,
                             &ValueName,
                             KeyValuePartialInformation,
                             pKeyValue,
                             sizeof(ValueBuffer),
                             &ResultLength );

    if (NT_SUCCESS(Status) && pKeyValue->DataLength > 3) {
        //
        // In Win95 registry, these items are written as BYTE data...
        //
        return (DWORD)(MAKEWORD( pKeyValue->Data[0], pKeyValue->Data[1])) |
                 (((DWORD)(MAKEWORD( pKeyValue->Data[2], pKeyValue->Data[3]))) << 16);
    }

    return 0;
}

BOOL CliSetSingleHotKey(PKEY_BASIC_INFORMATION pKeyInfo, HANDLE hKey)
{
    UNICODE_STRING      SubKeyName;
    HANDLE    hKeySingleHotKey;
    OBJECT_ATTRIBUTES   Obja;

    DWORD dwID = 0;
    UINT  uVKey = 0;
    UINT  uModifiers = 0;
    HKL   hKL = NULL;

    NTSTATUS Status;

    SubKeyName.Buffer = (PWSTR)&(pKeyInfo->Name[0]);
    SubKeyName.Length = (USHORT)pKeyInfo->NameLength;
    SubKeyName.MaximumLength = (USHORT)pKeyInfo->NameLength;
    InitializeObjectAttributes(&Obja,
                               &SubKeyName,
                               OBJ_CASE_INSENSITIVE,
                               hKey,
                               NULL);

    Status = NtOpenKey(&hKeySingleHotKey, KEY_READ, &Obja);
    if (!NT_SUCCESS(Status)) {
        return FALSE;
    }

    RtlUnicodeStringToInteger(&SubKeyName, 16L, &dwID);
    uVKey = CliReadRegistryValue(hKeySingleHotKey, szRegVK);
    uModifiers = CliReadRegistryValue(hKeySingleHotKey, szRegMOD);
    hKL = (HKL)LongToHandle( CliReadRegistryValue(hKeySingleHotKey, szRegHKL) );

    NtClose(hKeySingleHotKey);

    return CliImmSetHotKeyWorker(dwID, uModifiers, uVKey, hKL, ISHK_ADD);
}

/***************************************************************************\
* ImmSetHotKey()
*
* Private API for IMEs and the control panel.
*
* History:
* 25-Mar-1996 TakaoK       Created
\***************************************************************************/
BOOL WINAPI CliImmSetHotKey(
    DWORD dwID,
    UINT uModifiers,
    UINT uVKey,
    HKL hkl)
{
    BOOL fResult;
    BOOL fTmp;
    BOOL fDelete = (uVKey == 0 );

    if (fDelete) {
        //
        // Removing an IME hotkey from the list in the kernel side
        // should not be failed, if we succeed to remove the IME
        // hotkey entry from the registry. Therefore CliSaveImeHotKey
        // is called first.
        //
        fResult = CliSaveImeHotKey( dwID, uModifiers, uVKey, hkl,  fDelete );
        if (fResult) {
            fTmp = CliImmSetHotKeyWorker( dwID, uModifiers, uVKey, hkl, ISHK_REMOVE );
            UserAssert(fTmp);
        }
    } else {
        //
        // CliImmSetHotKeyWorker should be called first since
        // adding an IME hotkey into the list in the kernel side
        // will be failed in various reasons.
        //
        fResult = CliImmSetHotKeyWorker(dwID, uModifiers, uVKey, hkl, ISHK_ADD);
        if (fResult) {
            fResult = CliSaveImeHotKey(dwID, uModifiers, uVKey, hkl, fDelete);
            if (!fResult) {
                //
                // We failed to save the hotkey to the registry.
                // We need to remove the entry from the IME hotkey
                // list in the kernel side.
                //
                fTmp = CliImmSetHotKeyWorker(dwID, uModifiers, uVKey, hkl, ISHK_REMOVE);
                UserAssert(fTmp);
            }
        }
    }
    return fResult;
}

/***************************************************************************\
* Open/CloseRegApi
*
*  Open and Close Regstry APIs in ADVAPI32.DLL
*
* History:
* 17-Aug-98 Hiroyama
\***************************************************************************/

#define GET(a) \
    if ((pfn->a = (void*)GetProcAddress(pfn->hModule, #a)) == NULL) {   \
        RIPMSG0(RIP_WARNING, "OpenRegApi: " #a " cannot be imported.");  \
        FreeLibrary(pfn->hModule);                                      \
        return FALSE;                                                   \
    }

ADVAPI_FN gAdvApiFn;

BOOL OpenRegApi(ADVAPI_FN* pfn)
{
    pfn->hModule = LoadLibraryW(L"ADVAPI32.DLL");

    if (pfn->hModule != NULL) {
        GET(RegCreateKeyW);
        GET(RegOpenKeyW);
        GET(RegCloseKey);
        GET(RegDeleteKeyW);
        GET(RegCreateKeyExW);
        GET(RegSetValueExW);
        GET(RegQueryValueExW);

        //
        // Succeeded.
        //
        return TRUE;
    }

    //
    // Failed to open registry APIs.
    //
    return TRUE;
}

void CloseRegApi(ADVAPI_FN* pfn)
{
    UserAssert(pfn->hModule);
    if (pfn->hModule) {
        FreeLibrary(pfn->hModule);
        pfn->hModule;
    }
}

#undef GET

/***************************************************************************\
* CliSaveImeHotKey()
*
*  Put/Remove the specified IME hotkey entry from the registry
*
* History:
* 25-Mar-1996 TakaoK       Created
\***************************************************************************/

extern BOOL CliSaveImeHotKeyWorker(DWORD, UINT, UINT, HKL, BOOL, ADVAPI_FN*);

BOOL CliSaveImeHotKey(DWORD id, UINT mod, UINT vk, HKL hkl, BOOL fDelete)
{
    BOOL fRet = FALSE;
    ADVAPI_FN fn;

    if (OpenRegApi(&fn)) {
        fRet = CliSaveImeHotKeyWorker(id, mod, vk, hkl, fDelete, &fn);
        CloseRegApi(&fn);
    }

    return fRet;
}

BOOL CliSaveImeHotKeyWorker(DWORD id, UINT mod, UINT vk, HKL hkl, BOOL fDelete, ADVAPI_FN* fn)
{
    HKEY hKey, hKeyParent;
    INT i;
    LONG lResult;
    TCHAR szHex[16];

    if (fDelete) {
        TCHAR szRegTmp[(sizeof(szRegImeHotKey) / sizeof(TCHAR) + 1 + 8 + 1)];

        lstrcpy(szRegTmp, szRegImeHotKey);
        lstrcat(szRegTmp, TEXT("\\"));
        NumToHexAscii(id, szHex);
        lstrcat(szRegTmp, szHex);

        lResult = fn->RegDeleteKey(HKEY_CURRENT_USER, szRegTmp);
        if (lResult != ERROR_SUCCESS) {
            RIPERR1(lResult, RIP_WARNING,
                     "CliSaveImeHotKeyWorker: deleting %s failed", szRegTmp);
            return FALSE;
        }
        return TRUE;
    }

    hKeyParent = HKEY_CURRENT_USER;
    for (i = 0; szaRegImmHotKeys[i] != NULL; i++) {
        lResult = fn->RegCreateKeyEx(hKeyParent,
                                  szaRegImmHotKeys[i],
                                  0,
                                  NULL,
                                  REG_OPTION_NON_VOLATILE,
                                  KEY_WRITE|KEY_READ,
                                  NULL,
                                  &hKey,
                                  NULL );
        fn->RegCloseKey(hKeyParent);
        if (lResult == ERROR_SUCCESS) {
            hKeyParent = hKey;
        } else {
            RIPERR1(lResult, RIP_WARNING,
                    "CliSaveImeHotKeyWorker: creating %s failed", szaRegImmHotKeys[i]);

            return FALSE;
        }
    }

    NumToHexAscii(id, szHex);
    lResult = fn->RegCreateKeyEx(hKeyParent,
                             szHex,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_WRITE|KEY_READ,
                             NULL,
                             &hKey,
                             NULL );
    fn->RegCloseKey(hKeyParent);
    if (lResult != ERROR_SUCCESS) {
        RIPERR1(lResult, RIP_WARNING,
                "CliSaveImeHotKeyWorker: creating %s failed", szHex );
        return FALSE;
    }

    lResult = fn->RegSetValueExW(hKey,
                             szRegVK,
                             0,
                             REG_BINARY,
                            (LPBYTE)&vk,
                            sizeof(DWORD));
    if (lResult != ERROR_SUCCESS) {
        fn->RegCloseKey(hKey);
        CliSaveImeHotKey(id, vk, mod, hkl, TRUE);
        RIPERR1( lResult, RIP_WARNING,
                 "SaveImeHotKey:setting value on %s failed", szRegVK );
        return ( FALSE );
    }
    lResult = fn->RegSetValueExW(hKey,
                             szRegMOD,
                             0,
                             REG_BINARY,
                            (LPBYTE)&mod,
                            sizeof(DWORD));

    if (lResult != ERROR_SUCCESS) {
        fn->RegCloseKey(hKey);
        CliSaveImeHotKey(id, vk, mod, hkl, TRUE);
        RIPERR1(lResult, RIP_WARNING,
                "CliSaveImeHotKeyWorker: setting value on %s failed", szRegMOD);
        return FALSE;
    }

    lResult = fn->RegSetValueExW(hKey,
                             szRegHKL,
                             0,
                             REG_BINARY,
                            (LPBYTE)&hkl,
                            sizeof(DWORD) );

    if (lResult != ERROR_SUCCESS) {
        fn->RegCloseKey(hKey);
        CliSaveImeHotKey(id, vk, mod, hkl, TRUE);
        RIPERR1(lResult, RIP_WARNING,
                "CliSaveImeHotKeyWorker: setting value on %s failed", szRegHKL);
        return FALSE;
    }

    fn->RegCloseKey(hKey);
    return TRUE;
}

BOOL CliImmSetHotKeyWorker(
    DWORD dwID,
    UINT uModifiers,
    UINT uVKey,
    HKL hkl,
    DWORD dwAction)
{
    //
    // if we're adding an IME hotkey entry, let's check
    // the parameters before calling the kernel side code
    //
    if (dwAction == ISHK_ADD) {

        if (dwID >= IME_HOTKEY_DSWITCH_FIRST &&
                dwID <= IME_HOTKEY_DSWITCH_LAST) {
            //
            // IME direct switching hot key - switch to
            // the keyboard layout specified.
            // We need to specify keyboard layout.
            //
            if (hkl == NULL) {
                RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "hkl should be specified");
                return FALSE;
            }

        } else {
            //
            // normal hot keys - change the mode of current iME
            //
            // Because it should be effective in all IME no matter
            // which IME is active we should not specify a target IME
            //
            if (hkl != NULL) {
                RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "hkl shouldn't be specified");
                return FALSE;
            }

            if (dwID >= IME_KHOTKEY_FIRST && dwID <= IME_KHOTKEY_LAST) {
                RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "Hotkey for Korean IMEs are invalid.");
                return FALSE;
            }
        }

        if (uModifiers & MOD_MODIFY_KEYS) {
            //
            // Because normal keyboard has left and right key for
            // these keys, you should specify left or right ( or both )
            //
            if ((uModifiers & MOD_BOTH_SIDES) == 0) {
                RIPERR3(ERROR_INVALID_PARAMETER, RIP_WARNING, "invalid modifiers %x for id %x vKey %x", uModifiers, dwID, uVKey);
                return FALSE;
            }
        }

#if 0   // Skip this check for now
        //
        // It doesn't make sense if vkey is same as modifiers
        //
        if ( ((uModifiers & MOD_ALT) && (uVKey == VK_MENU))        ||
             ((uModifiers & MOD_CONTROL) && (uVKey == VK_CONTROL)) ||
             ((uModifiers & MOD_SHIFT) && (uVKey == VK_SHIFT))     ||
             ((uModifiers & MOD_WIN) && ((uVKey == VK_LWIN)||(uVKey == VK_RWIN)))
           ) {

            RIPERR0( ERROR_INVALID_PARAMETER, RIP_WARNING, "vkey and modifiers are same");
            return FALSE;
        }
#endif
    }
    return NtUserSetImeHotKey(dwID, uModifiers, uVKey, hkl, dwAction);
}

//
// NumToHexAscii
//
// convert a DWORD into the hex string
// (e.g. 0x31 -> "00000031")
//
// 29-Jan-1996 takaok   ported from Win95.
//
static CONST TCHAR szHexString[] = TEXT("0123456789ABCDEF");

VOID
NumToHexAscii(
    DWORD dwNum,
    PWSTR szAscii)
{
    int i;

    for (i = 7; i >= 0; i--) {
        szAscii[i] = szHexString[dwNum & 0x0000000f];
        dwNum >>= 4;
    }
    szAscii[8] = TEXT('\0');

    return;
}

