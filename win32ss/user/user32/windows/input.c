/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * PROJECT:         ReactOS user32.dll
 * FILE:            win32ss/user/user32/windows/input.c
 * PURPOSE:         Input
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

#include <user32.h>

#include <strsafe.h>

WINE_DEFAULT_DEBUG_CHANNEL(user32);

typedef struct tagIMEHOTKEYENTRY
{
    DWORD  dwHotKeyId;
    UINT   uVirtualKey;
    UINT   uModifiers;
    HKL    hKL;
} IMEHOTKEYENTRY, *PIMEHOTKEYENTRY;

// Japanese
IMEHOTKEYENTRY DefaultHotKeyTableJ[] =
{
    { IME_JHOTKEY_CLOSE_OPEN, VK_KANJI, MOD_IGNORE_ALL_MODIFIER, NULL },
};

// Chinese Traditional
IMEHOTKEYENTRY DefaultHotKeyTableT[] =
{
    { IME_THOTKEY_IME_NONIME_TOGGLE, VK_SPACE, MOD_LEFT | MOD_RIGHT | MOD_CONTROL, NULL },
    { IME_THOTKEY_SHAPE_TOGGLE, VK_SPACE, MOD_LEFT | MOD_RIGHT | MOD_SHIFT, NULL },
};

// Chinese Simplified
IMEHOTKEYENTRY DefaultHotKeyTableC[] =
{
    { IME_CHOTKEY_IME_NONIME_TOGGLE, VK_SPACE, MOD_LEFT | MOD_RIGHT | MOD_CONTROL, NULL },
    { IME_CHOTKEY_SHAPE_TOGGLE, VK_SPACE, MOD_LEFT | MOD_RIGHT | MOD_SHIFT, NULL },
};

// The far-east flags
#define FE_JAPANESE             (1 << 0)
#define FE_CHINESE_TRADITIONAL  (1 << 1)
#define FE_CHINESE_SIMPLIFIED   (1 << 2)
#define FE_KOREAN               (1 << 3)

// Sets the far-east flags
// Win: SetFeKeyboardFlags
VOID FASTCALL IntSetFeKeyboardFlags(LANGID LangID, PBYTE pbFlags)
{
    switch (LangID)
    {
        case MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT):
            *pbFlags |= FE_JAPANESE;
            break;

        case MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL):
        case MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_HONGKONG):
            *pbFlags |= FE_CHINESE_TRADITIONAL;
            break;

        case MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED):
        case MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SINGAPORE):
            *pbFlags |= FE_CHINESE_SIMPLIFIED;
            break;

        case MAKELANGID(LANG_KOREAN, SUBLANG_KOREAN):
            *pbFlags |= FE_KOREAN;
            break;

        default:
            break;
    }
}

DWORD FASTCALL CliReadRegistryValue(HANDLE hKey, LPCWSTR pszName)
{
    DWORD dwValue, cbValue;
    LONG error;

    cbValue = sizeof(dwValue);
    error = RegQueryValueExW(hKey, pszName, NULL, NULL, (LPBYTE)&dwValue, &cbValue);
    if (error != ERROR_SUCCESS || cbValue < sizeof(DWORD))
        return 0;

    return dwValue;
}

BOOL APIENTRY
CliImmSetHotKeyWorker(DWORD dwHotKeyId, UINT uModifiers, UINT uVirtualKey, HKL hKL, DWORD dwAction)
{
    if (dwAction == SETIMEHOTKEY_ADD)
    {
        if (IME_HOTKEY_DSWITCH_FIRST <= dwHotKeyId && dwHotKeyId <= IME_HOTKEY_DSWITCH_LAST)
        {
            if (!hKL)
                goto Failure;
        }
        else
        {
            if (hKL)
                goto Failure;

            if (IME_KHOTKEY_SHAPE_TOGGLE <= dwHotKeyId &&
                dwHotKeyId < IME_THOTKEY_IME_NONIME_TOGGLE)
            {
                // The Korean cannot set the IME hotkeys
                goto Failure;
            }
        }

#define MOD_ALL_MODS (MOD_ALT | MOD_CONTROL | MOD_SHIFT | MOD_WIN)
        if ((uModifiers & MOD_ALL_MODS) && !(uModifiers & (MOD_LEFT | MOD_RIGHT)))
            goto Failure;
#undef MOD_ALL_MODS
    }

    return NtUserSetImeHotKey(dwHotKeyId, uModifiers, uVirtualKey, hKL, dwAction);

Failure:
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}


/* Win: LoadPreloadKeyboardLayouts */
VOID IntLoadPreloadKeyboardLayouts(VOID)
{
    UINT nNumber, uFlags;
    DWORD cbValue, dwType;
    WCHAR szNumber[32], szValue[KL_NAMELENGTH];
    HKEY hPreloadKey;
    BOOL bOK = FALSE;
    HKL hKL, hDefaultKL = NULL;

    if (RegOpenKeyW(HKEY_CURRENT_USER,
                    L"Keyboard Layout\\Preload",
                    &hPreloadKey) != ERROR_SUCCESS)
    {
        return;
    }

    for (nNumber = 1; nNumber <= 1000; ++nNumber)
    {
        _ultow(nNumber, szNumber, 10);

        cbValue = sizeof(szValue);
        if (RegQueryValueExW(hPreloadKey,
                             szNumber,
                             NULL,
                             &dwType,
                             (LPBYTE)szValue,
                             &cbValue) != ERROR_SUCCESS)
        {
            break;
        }

        if (dwType != REG_SZ)
            continue;

        if (nNumber == 1) /* The first entry is for default keyboard layout */
            uFlags = KLF_SUBSTITUTE_OK | KLF_ACTIVATE | KLF_RESET;
        else
            uFlags = KLF_SUBSTITUTE_OK | KLF_NOTELLSHELL | KLF_REPLACELANG;

        hKL = LoadKeyboardLayoutW(szValue, uFlags);
        if (hKL)
        {
            bOK = TRUE;
            if (nNumber == 1) /* The first entry */
                hDefaultKL = hKL;
        }
    }

    RegCloseKey(hPreloadKey);

    if (hDefaultKL)
        SystemParametersInfoW(SPI_SETDEFAULTINPUTLANG, 0, &hDefaultKL, 0);

    if (!bOK)
    {
        /* Fallback to English (US) */
        LoadKeyboardLayoutW(L"00000409", KLF_SUBSTITUTE_OK | KLF_ACTIVATE | KLF_RESET);
    }
}


BOOL APIENTRY
CliSaveImeHotKey(DWORD dwID, UINT uModifiers, UINT uVirtualKey, HKL hKL, BOOL bDelete)
{
    WCHAR szName[MAX_PATH];
    LONG error;
    HKEY hControlPanel = NULL, hInputMethod = NULL, hHotKeys = NULL, hKey = NULL;
    BOOL ret = FALSE, bRevertOnFailure = FALSE;

    if (bDelete)
    {
        StringCchPrintfW(szName, _countof(szName),
                         L"Control Panel\\Input Method\\Hot Keys\\%08lX", dwID);
        error = RegDeleteKeyW(HKEY_CURRENT_USER, szName);
        return (error == ERROR_SUCCESS);
    }

    // Open "Control Panel"
    error = RegCreateKeyExW(HKEY_CURRENT_USER, L"Control Panel", 0, NULL, 0, KEY_ALL_ACCESS,
                            NULL, &hControlPanel, NULL);
    if (error == ERROR_SUCCESS)
    {
        // Open "Input Method"
        error = RegCreateKeyExW(hControlPanel, L"Input Method", 0, NULL, 0, KEY_ALL_ACCESS,
                                NULL, &hInputMethod, NULL);
        if (error == ERROR_SUCCESS)
        {
            // Open "Hot Keys"
            error = RegCreateKeyExW(hInputMethod, L"Hot Keys", 0, NULL, 0, KEY_ALL_ACCESS,
                                    NULL, &hHotKeys, NULL);
            if (error == ERROR_SUCCESS)
            {
                // Open "Key"
                StringCchPrintfW(szName, _countof(szName), L"%08lX", dwID);
                error = RegCreateKeyExW(hHotKeys, szName, 0, NULL, 0, KEY_ALL_ACCESS,
                                        NULL, &hKey, NULL);
                if (error == ERROR_SUCCESS)
                {
                    bRevertOnFailure = TRUE;

                    // Set "Virtual Key"
                    error = RegSetValueExW(hKey, L"Virtual Key", 0, REG_BINARY,
                                           (LPBYTE)&uVirtualKey, sizeof(uVirtualKey));
                    if (error == ERROR_SUCCESS)
                    {
                        // Set "Key Modifiers"
                        error = RegSetValueExW(hKey, L"Key Modifiers", 0, REG_BINARY,
                                               (LPBYTE)&uModifiers, sizeof(uModifiers));
                        if (error == ERROR_SUCCESS)
                        {
                            // Set "Target IME"
                            error = RegSetValueExW(hKey, L"Target IME", 0, REG_BINARY,
                                                   (LPBYTE)&hKL, sizeof(hKL));
                            if (error == ERROR_SUCCESS)
                            {
                                // Success!
                                ret = TRUE;
                                bRevertOnFailure = FALSE;
                            }
                        }
                    }
                    RegCloseKey(hKey);
                }
                RegCloseKey(hHotKeys);
            }
            RegCloseKey(hInputMethod);
        }
        RegCloseKey(hControlPanel);
    }

    if (bRevertOnFailure)
        CliSaveImeHotKey(dwID, uVirtualKey, uModifiers, hKL, TRUE);

    return ret;
}

/*
 * @implemented
 * Same as imm32!ImmSetHotKey.
 */
BOOL WINAPI CliImmSetHotKey(DWORD dwID, UINT uModifiers, UINT uVirtualKey, HKL hKL)
{
    BOOL ret;

    if (uVirtualKey == 0) // Delete?
    {
        ret = CliSaveImeHotKey(dwID, uModifiers, uVirtualKey, hKL, TRUE);
        if (ret)
            CliImmSetHotKeyWorker(dwID, uModifiers, uVirtualKey, hKL, SETIMEHOTKEY_DELETE);
        return ret;
    }

    // Add
    ret = CliImmSetHotKeyWorker(dwID, uModifiers, uVirtualKey, hKL, SETIMEHOTKEY_ADD);
    if (ret)
    {
        ret = CliSaveImeHotKey(dwID, uModifiers, uVirtualKey, hKL, FALSE);
        if (!ret) // Failure?
            CliImmSetHotKeyWorker(dwID, uModifiers, uVirtualKey, hKL, SETIMEHOTKEY_DELETE);
    }

    return ret;
}

BOOL FASTCALL CliSetSingleHotKey(LPCWSTR pszSubKey, HANDLE hKey)
{
    LONG error;
    HKEY hSubKey;
    DWORD dwHotKeyId = 0;
    UINT uModifiers = 0, uVirtualKey = 0;
    HKL hKL = NULL;
    UNICODE_STRING ustrName;

    error = RegOpenKeyExW(hKey, pszSubKey, 0, KEY_READ, &hSubKey);
    if (error != ERROR_SUCCESS)
        return FALSE;

    RtlInitUnicodeString(&ustrName, pszSubKey);
    RtlUnicodeStringToInteger(&ustrName, 16, &dwHotKeyId);

    uModifiers = CliReadRegistryValue(hSubKey, L"Key Modifiers");
    hKL = (HKL)(ULONG_PTR)CliReadRegistryValue(hSubKey, L"Target IME");
    uVirtualKey = CliReadRegistryValue(hSubKey, L"Virtual Key");

    RegCloseKey(hSubKey);

    return CliImmSetHotKeyWorker(dwHotKeyId, uModifiers, uVirtualKey, hKL, SETIMEHOTKEY_ADD);
}

BOOL FASTCALL CliGetImeHotKeysFromRegistry(VOID)
{
    HKEY hKey;
    LONG error;
    BOOL ret = FALSE;
    DWORD dwIndex, cchKeyName;
    WCHAR szKeyName[16];

    error = RegOpenKeyExW(HKEY_CURRENT_USER,
                          L"Control Panel\\Input Method\\Hot Keys",
                          0,
                          KEY_ALL_ACCESS,
                          &hKey);
    if (error != ERROR_SUCCESS)
        return ret;

    for (dwIndex = 0; ; ++dwIndex)
    {
        cchKeyName = _countof(szKeyName);
        error = RegEnumKeyExW(hKey, dwIndex, szKeyName, &cchKeyName, NULL, NULL, NULL, NULL);
        if (error == ERROR_NO_MORE_ITEMS || error != ERROR_SUCCESS)
            break;

        szKeyName[_countof(szKeyName) - 1] = 0;

        if (CliSetSingleHotKey(szKeyName, hKey))
            ret = TRUE;
    }

    RegCloseKey(hKey);
    return ret;
}

VOID APIENTRY CliGetPreloadKeyboardLayouts(PBYTE pbFlags)
{
    WCHAR szValueName[8], szValue[16];
    UNICODE_STRING ustrValue;
    DWORD dwKL, cbValue, dwType;
    UINT iNumber;
    HKEY hKey;
    LONG error;

    error = RegOpenKeyExW(HKEY_CURRENT_USER, L"Keyboard Layout\\Preload", 0, KEY_READ, &hKey);
    if (error != ERROR_SUCCESS)
        return;

    for (iNumber = 1; iNumber < 1000; ++iNumber)
    {
        StringCchPrintfW(szValueName, _countof(szValueName), L"%u", iNumber);

        cbValue = sizeof(szValue);
        error = RegQueryValueExW(hKey, szValueName, NULL, &dwType, (LPBYTE)szValue, &cbValue);
        if (error != ERROR_SUCCESS || dwType != REG_SZ)
            break;

        szValue[_countof(szValue) - 1] = 0;

        RtlInitUnicodeString(&ustrValue, szValue);
        RtlUnicodeStringToInteger(&ustrValue, 16, &dwKL);

        IntSetFeKeyboardFlags(LOWORD(dwKL), pbFlags);
    }

    RegCloseKey(hKey);
}

VOID APIENTRY CliSetDefaultImeHotKeys(PIMEHOTKEYENTRY pEntries, UINT nCount, BOOL bCheck)
{
    UINT uVirtualKey, uModifiers;
    HKL hKL;

    while (nCount-- > 0)
    {
        if (!bCheck || !NtUserGetImeHotKey(pEntries->dwHotKeyId, &uModifiers, &uVirtualKey, &hKL))
        {
            CliImmSetHotKeyWorker(pEntries->dwHotKeyId,
                                  pEntries->uModifiers,
                                  pEntries->uVirtualKey,
                                  pEntries->hKL,
                                  SETIMEHOTKEY_ADD);
        }
        ++pEntries;
    }
}

VOID APIENTRY CliImmInitializeHotKeys(DWORD dwAction, HKL hKL)
{
    UINT nCount;
    LPHKL pList;
    UINT iIndex;
    LANGID LangID;
    BYTE bFlags = 0;
    BOOL bCheck;

    NtUserSetImeHotKey(0, 0, 0, NULL, SETIMEHOTKEY_DELETEALL);

    bCheck = CliGetImeHotKeysFromRegistry();

    if (dwAction == SETIMEHOTKEY_DELETEALL)
    {
        LangID = LANGIDFROMLCID(GetUserDefaultLCID());
        IntSetFeKeyboardFlags(LangID, &bFlags);

        CliGetPreloadKeyboardLayouts(&bFlags);
    }
    else
    {
        nCount = NtUserGetKeyboardLayoutList(0, NULL);
        if (!nCount)
            return;

        pList = RtlAllocateHeap(RtlGetProcessHeap(), 0, nCount * sizeof(HKL));
        if (!pList)
            return;

        NtUserGetKeyboardLayoutList(nCount, pList);

        for (iIndex = 0; iIndex < nCount; ++iIndex)
        {
            LangID = LOWORD(pList[iIndex]);
            IntSetFeKeyboardFlags(LangID, &bFlags);
        }

        RtlFreeHeap(RtlGetProcessHeap(), 0, pList);
    }

    if (bFlags & FE_JAPANESE)
        CliSetDefaultImeHotKeys(DefaultHotKeyTableJ, _countof(DefaultHotKeyTableJ), bCheck);

    if (bFlags & FE_CHINESE_TRADITIONAL)
        CliSetDefaultImeHotKeys(DefaultHotKeyTableT, _countof(DefaultHotKeyTableT), bCheck);

    if (bFlags & FE_CHINESE_SIMPLIFIED)
        CliSetDefaultImeHotKeys(DefaultHotKeyTableC, _countof(DefaultHotKeyTableC), bCheck);
}

/*
 * @implemented
 */
BOOL
WINAPI
DragDetect(
    HWND hWnd,
    POINT pt)
{
    return NtUserDragDetect(hWnd, pt);
#if 0
    MSG msg;
    RECT rect;
    POINT tmp;
    ULONG dx = GetSystemMetrics(SM_CXDRAG);
    ULONG dy = GetSystemMetrics(SM_CYDRAG);

    rect.left = pt.x - dx;
    rect.right = pt.x + dx;
    rect.top = pt.y - dy;
    rect.bottom = pt.y + dy;

    SetCapture(hWnd);

    for (;;)
    {
        while (
            PeekMessageW(&msg, 0, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE) ||
            PeekMessageW(&msg, 0, WM_KEYFIRST,   WM_KEYLAST,   PM_REMOVE)
        )
        {
            if (msg.message == WM_LBUTTONUP)
            {
                ReleaseCapture();
                return FALSE;
            }
            if (msg.message == WM_MOUSEMOVE)
            {
                tmp.x = LOWORD(msg.lParam);
                tmp.y = HIWORD(msg.lParam);
                if (!PtInRect(&rect, tmp))
                {
                    ReleaseCapture();
                    return TRUE;
                }
            }
            if (msg.message == WM_KEYDOWN)
            {
                if (msg.wParam == VK_ESCAPE)
                {
                    ReleaseCapture();
                    return TRUE;
                }
            }
        }
        WaitMessage();
    }
    return 0;
#endif
}

/*
 * @implemented
 */
BOOL WINAPI
EnableWindow(HWND hWnd, BOOL bEnable)
{
    return NtUserxEnableWindow(hWnd, bEnable);
}

/*
 * @implemented
 */
SHORT
WINAPI
DECLSPEC_HOTPATCH
GetAsyncKeyState(int vKey)
{
    if (vKey < 0 || vKey > 256)
        return 0;
    return (SHORT)NtUserGetAsyncKeyState((DWORD)vKey);
}


/*
 * @implemented
 */
HKL WINAPI
GetKeyboardLayout(DWORD idThread)
{
    return NtUserxGetKeyboardLayout(idThread);
}


/*
 * @implemented
 */
UINT WINAPI
GetKBCodePage(VOID)
{
    return GetOEMCP();
}


/*
 * @implemented
 */
int WINAPI
GetKeyNameTextA(LONG lParam,
                LPSTR lpString,
                int nSize)
{
    LPWSTR pwszBuf;
    UINT cchBuf = 0;
    int iRet = 0;
    BOOL defChar = FALSE;

    pwszBuf = HeapAlloc(GetProcessHeap(), 0, nSize * sizeof(WCHAR));
    if (!pwszBuf)
        return 0;

    cchBuf = NtUserGetKeyNameText(lParam, pwszBuf, nSize);

    iRet = WideCharToMultiByte(CP_ACP, 0,
                              pwszBuf, cchBuf,
                              lpString, nSize, ".", &defChar); // FIXME: do we need defChar?
    lpString[iRet] = 0;
    HeapFree(GetProcessHeap(), 0, pwszBuf);

    return iRet;
}

/*
 * @implemented
 */
int WINAPI
GetKeyNameTextW(LONG lParam,
                LPWSTR lpString,
                int nSize)
{
    return NtUserGetKeyNameText(lParam, lpString, nSize);
}

/*
 * @implemented
 */
SHORT
WINAPI
DECLSPEC_HOTPATCH
GetKeyState(int nVirtKey)
{
    return (SHORT)NtUserGetKeyState((DWORD)nVirtKey);
}

/*
 * @implemented
 */
BOOL WINAPI
GetKeyboardLayoutNameA(LPSTR pwszKLID)
{
    WCHAR buf[KL_NAMELENGTH];

    if (!GetKeyboardLayoutNameW(buf))
        return FALSE;

    if (!WideCharToMultiByte(CP_ACP, 0, buf, -1, pwszKLID, KL_NAMELENGTH, NULL, NULL))
        return FALSE;

    return TRUE;
}

/*
 * @implemented
 */
BOOL WINAPI
GetKeyboardLayoutNameW(LPWSTR pwszKLID)
{
    UNICODE_STRING Name;

    RtlInitEmptyUnicodeString(&Name,
                              pwszKLID,
                              KL_NAMELENGTH * sizeof(WCHAR));

    return NtUserGetKeyboardLayoutName(&Name);
}

/*
 * @implemented
 */
int WINAPI
GetKeyboardType(int nTypeFlag)
{
    return NtUserxGetKeyboardType(nTypeFlag);
}

/*
 * @implemented
 */
BOOL WINAPI
GetLastInputInfo(PLASTINPUTINFO plii)
{
    TRACE("%p\n", plii);

    if (plii->cbSize != sizeof (*plii))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    plii->dwTime = gpsi->dwLastRITEventTickCount;
    return TRUE;
}

/*
 * @implemented
 */
HKL WINAPI
LoadKeyboardLayoutA(LPCSTR pszKLID,
                    UINT Flags)
{
    WCHAR wszKLID[16];

    if (!MultiByteToWideChar(CP_ACP, 0, pszKLID, -1,
                             wszKLID, sizeof(wszKLID)/sizeof(wszKLID[0])))
    {
        return FALSE;
    }

    return LoadKeyboardLayoutW(wszKLID, Flags);
}

inline BOOL IsValidKLID(_In_ LPCWSTR pwszKLID)
{
    return (pwszKLID != NULL) && (wcsspn(pwszKLID, L"0123456789ABCDEFabcdef") == (KL_NAMELENGTH - 1));
}

VOID GetSystemLibraryPath(LPWSTR pszPath, INT cchPath, LPCWSTR pszFileName)
{
    WCHAR szSysDir[MAX_PATH];
    GetSystemDirectoryW(szSysDir, _countof(szSysDir));
    StringCchPrintfW(pszPath, cchPath, L"%s\\%s", szSysDir, pszFileName);
}

#define ENGLISH_US MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)

/*
 * @unimplemented
 *
 * NOTE: We adopt a different design from Microsoft's one for security reason.
 */
/* Win: LoadKeyboardLayoutWorker */
HKL APIENTRY
IntLoadKeyboardLayout(
    _In_    HKL     hklUnload,
    _In_z_  LPCWSTR pwszKLID,
    _In_    LANGID  wLangID,
    _In_    UINT    Flags,
    _In_    BOOL    unknown5)
{
    DWORD dwKLID, dwHKL, dwType, dwSize;
    UNICODE_STRING ustrKbdName;
    UNICODE_STRING ustrKLID;
    WCHAR wszRegKey[256] = L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\";
    WCHAR wszLayoutId[10], wszNewKLID[KL_NAMELENGTH], szImeFileName[80];
    HKL hNewKL;
    HKEY hKey;
    BOOL bIsIME;
    WORD wLow, wHigh;

    if (!IsValidKLID(pwszKLID))
    {
        ERR("pwszKLID: %s\n", debugstr_w(pwszKLID));
        return UlongToHandle(MAKELONG(ENGLISH_US, ENGLISH_US));
    }

    dwKLID = wcstoul(pwszKLID, NULL, 16);
    bIsIME = IS_IME_HKL(UlongToHandle(dwKLID));

    wLow = LOWORD(dwKLID);
    wHigh = HIWORD(dwKLID);

    if (Flags & KLF_SUBSTITUTE_OK)
    {
        /* Check substitutes key */
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Keyboard Layout\\Substitutes", 0,
                          KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            dwSize = sizeof(wszNewKLID);
            if (RegQueryValueExW(hKey, pwszKLID, NULL, &dwType, (LPBYTE)wszNewKLID,
                                 &dwSize) == ERROR_SUCCESS &&
                dwType == REG_SZ)
            {
                /* Use new KLID value */
                pwszKLID = wszNewKLID;
                dwKLID = wcstoul(pwszKLID, NULL, 16);
                wHigh = LOWORD(dwKLID);
            }

            /* Close the key now */
            RegCloseKey(hKey);
        }
    }

    /* Append KLID at the end of registry key */
    StringCbCatW(wszRegKey, sizeof(wszRegKey), pwszKLID);

    /* Open layout registry key for read */
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, wszRegKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(wszLayoutId);
        if (RegQueryValueExW(hKey, L"Layout Id", NULL, &dwType, (LPBYTE)wszLayoutId,
                             &dwSize) == ERROR_SUCCESS && dwType == REG_SZ)
        {
            /* If Layout Id is specified, use this value | f000 as HIWORD */
            wHigh = (0xF000 | wcstoul(wszLayoutId, NULL, 16));
        }

        if (bIsIME)
        {
            /* Check "IME File" value */
            dwSize = sizeof(szImeFileName);
            if (RegQueryValueExW(hKey, L"IME File", NULL, &dwType, (LPBYTE)szImeFileName,
                                 &dwSize) != ERROR_SUCCESS)
            {
                bIsIME = FALSE;
                wHigh = 0;
            }
            else
            {
                WCHAR szPath[MAX_PATH];
                szImeFileName[_countof(szImeFileName) - 1] = UNICODE_NULL;
                GetSystemLibraryPath(szPath, _countof(szPath), szImeFileName);

                /* We don't allow the invalid "IME File" values for security reason */
                if (dwType != REG_SZ || szImeFileName[0] == 0 ||
                    wcsspn(szImeFileName, L":\\/") != wcslen(szImeFileName) ||
                    GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES) /* Does not exist? */
                {
                    bIsIME = FALSE;
                    wHigh = 0;
                }
            }
        }

        /* Close the key now */
        RegCloseKey(hKey);
    }
    else
    {
        ERR("Could not find keyboard layout %S.\n", pwszKLID);
        return NULL;
    }

    if (wHigh == 0)
        wHigh = wLow;

    dwHKL = MAKELONG(wLow, wHigh);

    ZeroMemory(&ustrKbdName, sizeof(ustrKbdName));
    RtlInitUnicodeString(&ustrKLID, pwszKLID);
    hNewKL = NtUserLoadKeyboardLayoutEx(NULL, 0, &ustrKbdName, NULL, &ustrKLID, dwHKL, Flags);
    CliImmInitializeHotKeys(SETIMEHOTKEY_ADD, hNewKL);
    return hNewKL;
}

/*
 * @implemented
 */
HKL WINAPI
LoadKeyboardLayoutW(LPCWSTR pwszKLID,
                    UINT Flags)
{
    TRACE("(%s, 0x%X)\n", debugstr_w(pwszKLID), Flags);
    return IntLoadKeyboardLayout(NULL, pwszKLID, 0, Flags, FALSE);
}

/*
 * @unimplemented
 */
HKL WINAPI
LoadKeyboardLayoutEx(HKL hklUnload,
                     LPCWSTR pwszKLID,
                     UINT Flags)
{
    FIXME("(%p, %s, 0x%X)", hklUnload, debugstr_w(pwszKLID), Flags);
    if (!hklUnload)
        return NULL;
    return IntLoadKeyboardLayout(hklUnload, pwszKLID, 0, Flags, FALSE);
}

/*
 * @implemented
 */
BOOL WINAPI UnloadKeyboardLayout(HKL hKL)
{
    if (!NtUserUnloadKeyboardLayout(hKL))
        return FALSE;

    CliImmInitializeHotKeys(SETIMEHOTKEY_DELETE, hKL);
    return TRUE;
}

/*
 * @implemented
 */
UINT WINAPI
MapVirtualKeyA(UINT uCode,
               UINT uMapType)
{
    return MapVirtualKeyExA(uCode, uMapType, GetKeyboardLayout(0));
}

/*
 * @implemented
 */
UINT WINAPI
MapVirtualKeyExA(UINT uCode,
                 UINT uMapType,
                 HKL dwhkl)
{
    return MapVirtualKeyExW(uCode, uMapType, dwhkl);
}


/*
 * @implemented
 */
UINT WINAPI
MapVirtualKeyExW(UINT uCode,
                 UINT uMapType,
                 HKL dwhkl)
{
    return NtUserMapVirtualKeyEx(uCode, uMapType, 0, dwhkl);
}


/*
 * @implemented
 */
UINT WINAPI
MapVirtualKeyW(UINT uCode,
               UINT uMapType)
{
    return MapVirtualKeyExW(uCode, uMapType, GetKeyboardLayout(0));
}


/*
 * @implemented
 */
DWORD WINAPI
OemKeyScan(WORD wOemChar)
{
    WCHAR p;
    SHORT Vk;
    UINT Scan;

    MultiByteToWideChar(CP_OEMCP, 0, (PCSTR)&wOemChar, 1, &p, 1);
    Vk = VkKeyScanW(p);
    Scan = MapVirtualKeyW((Vk & 0x00ff), 0);
    if (!Scan) return -1;
    /*
       Page 450-1, MS W2k SuperBible by SAMS. Return, low word has the
       scan code and high word has the shift state.
     */
    return ((Vk & 0xff00) << 8) | Scan;
}


/*
 * @implemented
 */
BOOL WINAPI
SetDoubleClickTime(UINT uInterval)
{
    return (BOOL)NtUserSystemParametersInfo(SPI_SETDOUBLECLICKTIME,
                                            uInterval,
                                            NULL,
                                            0);
}


/*
 * @implemented
 */
BOOL
WINAPI
SwapMouseButton(
    BOOL fSwap)
{
    return NtUserxSwapMouseButton(fSwap);
}


/*
 * @implemented
 */
int WINAPI
ToAscii(UINT uVirtKey,
        UINT uScanCode,
        CONST BYTE *lpKeyState,
        LPWORD lpChar,
        UINT uFlags)
{
    return ToAsciiEx(uVirtKey, uScanCode, lpKeyState, lpChar, uFlags, 0);
}


/*
 * @implemented
 */
int WINAPI
ToAsciiEx(UINT uVirtKey,
          UINT uScanCode,
          CONST BYTE *lpKeyState,
          LPWORD lpChar,
          UINT uFlags,
          HKL dwhkl)
{
    WCHAR UniChars[2];
    int Ret, CharCount;

    Ret = ToUnicodeEx(uVirtKey, uScanCode, lpKeyState, UniChars, 2, uFlags, dwhkl);
    CharCount = (Ret < 0 ? 1 : Ret);
    WideCharToMultiByte(CP_ACP, 0, UniChars, CharCount, (LPSTR)lpChar, 2, NULL, NULL);

    return Ret;
}


/*
 * @implemented
 */
int WINAPI
ToUnicode(UINT wVirtKey,
          UINT wScanCode,
          CONST BYTE *lpKeyState,
          LPWSTR pwszBuff,
          int cchBuff,
          UINT wFlags)
{
    return ToUnicodeEx(wVirtKey, wScanCode, lpKeyState, pwszBuff, cchBuff,
                       wFlags, 0);
}


/*
 * @implemented
 */
int WINAPI
ToUnicodeEx(UINT wVirtKey,
            UINT wScanCode,
            CONST BYTE *lpKeyState,
            LPWSTR pwszBuff,
            int cchBuff,
            UINT wFlags,
            HKL dwhkl)
{
    return NtUserToUnicodeEx(wVirtKey, wScanCode, (PBYTE)lpKeyState, pwszBuff, cchBuff,
                             wFlags, dwhkl);
}



/*
 * @implemented
 */
SHORT WINAPI
VkKeyScanA(CHAR ch)
{
    WCHAR wChar;

    if (IsDBCSLeadByte(ch))
        return -1;

    MultiByteToWideChar(CP_ACP, 0, &ch, 1, &wChar, 1);
    return VkKeyScanW(wChar);
}


/*
 * @implemented
 */
SHORT WINAPI
VkKeyScanExA(CHAR ch,
             HKL dwhkl)
{
    WCHAR wChar;

    if (IsDBCSLeadByte(ch))
        return -1;

    MultiByteToWideChar(CP_ACP, 0, &ch, 1, &wChar, 1);
    return VkKeyScanExW(wChar, dwhkl);
}


/*
 * @implemented
 */
SHORT WINAPI
VkKeyScanExW(WCHAR ch,
             HKL dwhkl)
{
    return (SHORT)NtUserVkKeyScanEx(ch, dwhkl, TRUE);
}


/*
 * @implemented
 */
SHORT WINAPI
VkKeyScanW(WCHAR ch)
{
    return (SHORT)NtUserVkKeyScanEx(ch, 0, FALSE);
}


/*
 * @implemented
 */
VOID
WINAPI
keybd_event(
    BYTE bVk,
    BYTE bScan,
    DWORD dwFlags,
    ULONG_PTR dwExtraInfo)
{
    INPUT Input;

    Input.type = INPUT_KEYBOARD;
    Input.ki.wVk = bVk;
    Input.ki.wScan = bScan;
    Input.ki.dwFlags = dwFlags;
    Input.ki.time = 0;
    Input.ki.dwExtraInfo = dwExtraInfo;

    NtUserSendInput(1, &Input, sizeof(INPUT));
}


/*
 * @implemented
 */
VOID
WINAPI
mouse_event(
    DWORD dwFlags,
    DWORD dx,
    DWORD dy,
    DWORD dwData,
    ULONG_PTR dwExtraInfo)
{
    INPUT Input;

    Input.type = INPUT_MOUSE;
    Input.mi.dx = dx;
    Input.mi.dy = dy;
    Input.mi.mouseData = dwData;
    Input.mi.dwFlags = dwFlags;
    Input.mi.time = 0;
    Input.mi.dwExtraInfo = dwExtraInfo;

    NtUserSendInput(1, &Input, sizeof(INPUT));
}

/* EOF */
