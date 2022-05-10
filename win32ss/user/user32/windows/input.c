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
    return NtUserGetKeyboardLayoutName(pwszKLID);
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

/*
 * @implemented
 */
HKL WINAPI
LoadKeyboardLayoutW(LPCWSTR pwszKLID,
                    UINT Flags)
{
    DWORD dwhkl, dwType, dwSize;
    UNICODE_STRING ustrKbdName;
    UNICODE_STRING ustrKLID;
    WCHAR wszRegKey[256] = L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\";
    WCHAR wszLayoutId[10], wszNewKLID[10];
    HKEY hKey;

    /* LOWORD of dwhkl is Locale Identifier */
    dwhkl = LOWORD(wcstoul(pwszKLID, NULL, 16));

    if (Flags & KLF_SUBSTITUTE_OK)
    {
        /* Check substitutes key */
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Keyboard Layout\\Substitutes", 0,
                          KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            dwSize = sizeof(wszNewKLID);
            if (RegQueryValueExW(hKey, pwszKLID, NULL, &dwType, (LPBYTE)wszNewKLID, &dwSize) == ERROR_SUCCESS)
            {
                /* Use new KLID value */
                pwszKLID = wszNewKLID;
            }

            /* Close the key now */
            RegCloseKey(hKey);
        }
    }

    /* Append KLID at the end of registry key */
    StringCbCatW(wszRegKey, sizeof(wszRegKey), pwszKLID);

    /* Open layout registry key for read */
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, wszRegKey, 0,
                      KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(wszLayoutId);
        if (RegQueryValueExW(hKey, L"Layout Id", NULL, &dwType, (LPBYTE)wszLayoutId, &dwSize) == ERROR_SUCCESS)
        {
            /* If Layout Id is specified, use this value | f000 as HIWORD */
            /* FIXME: Microsoft Office expects this value to be something specific
             * for Japanese and Korean Windows with an IME the value is 0xe001
             * We should probably check to see if an IME exists and if so then
             * set this word properly.
             */
            dwhkl |= (0xf000 | wcstol(wszLayoutId, NULL, 16)) << 16;
        }

        /* Close the key now */
        RegCloseKey(hKey);
    }
    else
    {
        ERR("Could not find keyboard layout %S.\n", pwszKLID);
        return NULL;
    }

    /* If Layout Id is not given HIWORD == LOWORD (for dwhkl) */
    if (!HIWORD(dwhkl))
        dwhkl |= dwhkl << 16;

    ZeroMemory(&ustrKbdName, sizeof(ustrKbdName));
    RtlInitUnicodeString(&ustrKLID, pwszKLID);
    return NtUserLoadKeyboardLayoutEx(NULL, 0, &ustrKbdName,
                                      NULL, &ustrKLID,
                                      dwhkl, Flags);
}

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
                goto Failure;
            }
        }

#define MOD_ALL_MODS (MOD_ALT | MOD_CONTROL | MOD_SHIFT | MOD_WIN)
        if ((uModifiers & MOD_ALL_MODS) && !(uModifiers & (MOD_LEFT | MOD_RIGHT)))
        {
            goto Failure;
        }
#undef MOD_ALL_MODS
    }

    return NtUserSetImeHotKey(dwHotKeyId, uModifiers, uVirtualKey, hKL, dwAction);

Failure:
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
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

VOID APIENTRY CliGetPreloadKeyboardLayouts(PBYTE pbFlags)
{
    WCHAR szValue[9], szKeyName[4];
    UNICODE_STRING ValueString;
    DWORD dwKL, ret;
    UINT iNumber;

    for (iNumber = 1; iNumber < 1000; ++iNumber)
    {
        StringCchPrintfW(szKeyName, _countof(szKeyName), L"%u", iNumber);

        // This code should cause redirection to the registry...
        ret = GetPrivateProfileStringW(L"Preload", szKeyName, L"", szValue, _countof(szValue),
                                       L"keyboardlayout.ini");
        if (ret == (DWORD)-1 || !szValue[0])
            break;

        RtlInitUnicodeString(&ValueString, szValue);
        RtlUnicodeStringToInteger(&ValueString, 16, &dwKL);

        IntSetFeKeyboardFlags(LOWORD(dwKL), pbFlags);
    }
}

BOOL FASTCALL CliGetImeHotKeysFromRegistry(VOID)
{
    // FIXME:
    return FALSE;
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
