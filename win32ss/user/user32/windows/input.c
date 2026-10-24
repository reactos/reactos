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
#include <cjkcode.h>
#include <strsafe.h>

WINE_DEFAULT_DEBUG_CHANNEL(user32);

#define KLF_INITTIME 0x80000000
#define KLF_FORCE 0x200

#define KBDLAYOUTS_KEY L"System\\CurrentControlSet\\Control\\Keyboard Layouts"
#define KBDLAYOUT_KEY  L"System\\CurrentControlSet\\Control\\Keyboard Layout"
#define ATTR_MASK      0x00FF0000
#define LANGID_ENGLISH_US MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)
#define WCSIEQL(s1, s2) ((LPCVOID)(s1) == (LPCVOID)(s2) || !_wcsicmp((s1), (s2)))

/* Procedure ordinals of keyboard layout file */
#define IFN_KbdLayerDescriptor      MAKEINTRESOURCEA(1)
#define IFN_KbdNlsLayerDescriptor   MAKEINTRESOURCEA(2)
#define IFN_KbdLayerRealDllFileNT4  MAKEINTRESOURCEA(3)
#define IFN_KbdLayerRealDllFile     MAKEINTRESOURCEA(5)
#define IFN_KbdLayerMultiDescriptor MAKEINTRESOURCEA(6)

/* The function types of keyboard layout file */
typedef PVOID (WINAPI *PFN_KbdLayerDescriptor)(void);
typedef PVOID (WINAPI *PFN_KbdNlsLayerDescriptor)(void);
typedef BOOL (WINAPI *PFN_KbdLayerRealDllFileNT4)(PWSTR);
typedef BOOL (WINAPI *PFN_KbdLayerRealDllFile)(ULONG, PWSTR, PCLIENTKEYBOARDTYPE, DWORD);
typedef BOOL (WINAPI *PFN_KbdLayerMultiDescriptor)(PKBDTABLE_MULTI);

static BOOL IntIsConsoleSession(void)
{
    return FALSE; /* FIXME */
}

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
    HKEY hKey;
    BOOL ret = FALSE, bRevertOnFailure = FALSE;

    StringCchPrintfW(szName, _countof(szName),
                     L"Control Panel\\Input Method\\Hot Keys\\%08lX", dwID);

    if (bDelete)
    {
        error = RegDeleteKeyW(HKEY_CURRENT_USER, szName);
        return (error == ERROR_SUCCESS);
    }

    error = RegCreateKeyExW(HKEY_CURRENT_USER, szName, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
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

    if (bRevertOnFailure)
        CliSaveImeHotKey(dwID, uVirtualKey, uModifiers, hKL, TRUE);

    return ret;
}

/**
 * @implemented
 * @note Same as imm32!ImmSetHotKey.
 */
BOOL WINAPI
CliImmSetHotKey(
    _In_ DWORD dwID,
    _In_ UINT uModifiers,
    _In_ UINT uVirtualKey,
    _In_opt_ _When_((dwAction == SETIMEHOTKEY_ADD) &&
                    !(IME_HOTKEY_DSWITCH_FIRST <= dwHotKeyId &&
                      dwHotKeyId <= IME_HOTKEY_DSWITCH_LAST), _Null_) HKL hKL)
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
                          KEY_READ,
                          &hKey);
    if (error != ERROR_SUCCESS)
        return ret;

    for (dwIndex = 0; dwIndex < 1000; ++dwIndex)
    {
        cchKeyName = _countof(szKeyName);
        error = RegEnumKeyExW(hKey, dwIndex, szKeyName, &cchKeyName, NULL, NULL, NULL, NULL);
        if (error != ERROR_SUCCESS)
            break;

        szKeyName[_countof(szKeyName) - 1] = 0; /* Avoid stack overrun */

        if (CliSetSingleHotKey(szKeyName, hKey))
            ret = TRUE;
    }

    RegCloseKey(hKey);
    return ret;
}

VOID APIENTRY CliGetPreloadKeyboardLayouts(PBYTE pbFlags)
{
    WCHAR szValueName[33], szValue[16];
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
        _ultow(iNumber, szValueName, 10);

        cbValue = sizeof(szValue);
        error = RegQueryValueExW(hKey, szValueName, NULL, &dwType, (LPBYTE)szValue, &cbValue);
        if (error != ERROR_SUCCESS)
            break;

        if (dwType != REG_SZ)
            continue;

        szValue[_countof(szValue) - 1] = 0; /* Avoid stack overrun */

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

    NtUserSetImeHotKey(0, 0, 0, NULL, SETIMEHOTKEY_INITIALIZE);

    bCheck = CliGetImeHotKeysFromRegistry();

    if (dwAction == SETIMEHOTKEY_INITIALIZE)
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

static inline BOOL IsValidKLID(_In_ LPCWSTR pwszKLID)
{
    return pwszKLID && (wcsspn(pwszKLID, L"0123456789ABCDEFabcdef") == (KL_NAMELENGTH - 1));
}

static inline BOOL IntIsValidLayoutFileName(PCWSTR pszFileName)
{
    if (!*pszFileName)
        return FALSE;
    SIZE_T cch = wcscspn(pszFileName, L"\\/:");
    return (cch < MAX_LAYOUT_FILENAME && !pszFileName[cch]);
}

BOOL GetSystemLibraryPath(PWSTR pszPath, SIZE_T cchPath, PCWSTR pszFileName)
{
    WCHAR szSysDir[MAX_PATH];
    UINT cch = GetSystemDirectoryW(szSysDir, _countof(szSysDir));
    if (!cch || cch >= _countof(szSysDir))
        return FALSE;
    return SUCCEEDED(StringCchPrintfW(pszPath, cchPath, L"%s\\%s", szSysDir, pszFileName));
}

static HMODULE LoadSysLibraryW(PCWSTR pszFileName)
{
    WCHAR szPath[MAX_PATH];
    if (!GetSystemLibraryPath(szPath, _countof(szPath), pszFileName))
        return NULL;
    return LoadLibraryW(szPath);
}

/* Closes the aux file handles that IntLoadKeyboardLayoutTablesAndFile stored in pMulti.
   Skips NULL and INVALID_HANDLE_VALUE (a failed aux file), and resets the table count. */
static VOID
IntCloseAuxFiles(_Inout_opt_ PKBDTABLE_MULTI_EX pMulti)
{
    UINT iTable;

    if (!pMulti)
        return;

    for (iTable = 0; iTable < pMulti->Multi.nTables && iTable < KBDTABLE_MULTI_MAX; ++iTable)
    {
        HANDLE hAuxFile = pMulti->AuxEntries[iTable].hFile;
        if (hAuxFile && hAuxFile != INVALID_HANDLE_VALUE)
            CloseHandle(hAuxFile);
        pMulti->AuxEntries[iTable].hFile = NULL;
    }

    pMulti->Multi.nTables = 0;
}

static HANDLE
IntLoadKeyboardLayoutTablesAndFile(
    _In_z_ PCWSTR pszLayoutFile,
    _In_opt_z_ PCWSTR pszKLID,
    _Inout_ PUINT puFlags,
    _Out_ PULONG poffTable,
    _Out_opt_ PKBDTABLE_MULTI_EX pMulti);

/* Loads the multi-table (if any). Each aux table file is loaded by a recursive call, and its
   file handle and offset are stored in pMulti. The caller owns the handles: see IntCloseAuxFiles. */
static VOID
IntLoadMultiTables(
    _In_ HMODULE hModule,
    _Inout_ PUINT puFlags,
    _Inout_ PKBDTABLE_MULTI_EX pMulti)
{
    PFN_KbdLayerMultiDescriptor pfnMulti;
    UINT iTable;

    pfnMulti = (PFN_KbdLayerMultiDescriptor)GetProcAddress(hModule, IFN_KbdLayerMultiDescriptor);
    if (!pfnMulti || !pfnMulti(&pMulti->Multi))
        return;

    if (pMulti->Multi.nTables >= KBDTABLE_MULTI_MAX)
        pMulti->Multi.nTables = 0;

    for (iTable = 0; iTable < pMulti->Multi.nTables; iTable++)
    {
        PKBDTABLE_DESC pDesc = &pMulti->Multi.aKbdTables[iTable];
        ULONG offTable = 0;
        HANDLE hAuxFile;

        /* Avoid buffer overrun */
        pDesc->wszDllName[_countof(pDesc->wszDllName) - 1] = UNICODE_NULL;

        hAuxFile = IntLoadKeyboardLayoutTablesAndFile(pDesc->wszDllName, NULL,
                                                      puFlags, &offTable, NULL);
        pMulti->AuxEntries[iTable].hFile = hAuxFile; /* NULL if failed */
        if (hAuxFile)
            pMulti->AuxEntries[iTable].offTable = offTable;
    }
}

/* If the layout DLL redirects to a "real" DLL, loads that one instead and releases the original.
   On return, *phModule and *ppszFile name the DLL that is actually used. */
static VOID
IntFollowRealDll(
    _Inout_ HMODULE *phModule,
    _Inout_ PCWSTR *ppszFile,
    _Out_writes_(MAX_LAYOUT_FILENAME) PWSTR pszRealFile,
    _In_opt_z_ PCWSTR pszKLID)
{
    PFN_KbdLayerRealDllFile pfnReal;
    PFN_KbdLayerRealDllFileNT4 pfnRealNT4;
    HMODULE hRealModule;
    ULONG ulKLID;

    pfnReal    = (PFN_KbdLayerRealDllFile)GetProcAddress(*phModule, IFN_KbdLayerRealDllFile);
    pfnRealNT4 = (PFN_KbdLayerRealDllFileNT4)GetProcAddress(*phModule, IFN_KbdLayerRealDllFileNT4);
    if (!pfnReal && !pfnRealNT4)
        return;

    ulKLID = pszKLID ? wcstoul(pszKLID, NULL, 16) : 0;
    pszRealFile[0] = UNICODE_NULL;

    /* FIXME: Specify PCLIENTKEYBOARDTYPE parameter for pfnReal */
    if (!((pfnReal    && pfnReal(ulKLID, pszRealFile, NULL, 0)) ||
          (pfnRealNT4 && pfnRealNT4(pszRealFile))))
    {
        return;
    }

    pszRealFile[MAX_LAYOUT_FILENAME - 1] = UNICODE_NULL;
    if (!IntIsValidLayoutFileName(pszRealFile))
        return;

    hRealModule = LoadSysLibraryW(pszRealFile);
    if (!hRealModule)
    {
        WARN("LoadSysLibraryW(%s) failed\n", debugstr_w(pszRealFile));
        return;
    }

    FreeLibrary(*phModule);
    *phModule = hRealModule;
    *ppszFile = pszRealFile;
}

/* Prepares a freshly loaded layout DLL: loads the multi-table, follows the "real" DLL, and gets
   KbdLayerDescriptor. Returns NULL on failure. The caller owns *phModule either way (it can be
   replaced by the real DLL). The layout DLL code can crash, so everything is under SEH. */
static PFN_KbdLayerDescriptor
IntPrepareLayoutModule(
    _Inout_ HMODULE *phModule,
    _Inout_ PCWSTR *ppszFile,
    _Out_writes_(MAX_LAYOUT_FILENAME) PWSTR pszRealFile,
    _In_opt_z_ PCWSTR pszKLID,
    _Inout_ PUINT puFlags,
    _Inout_opt_ PKBDTABLE_MULTI_EX pMulti)
{
    _SEH2_TRY
    {
        if (!WCSIEQL(*ppszFile, L"kbdus.dll")) /* Not English US? */
        {
            if (pMulti && IntIsConsoleSession())
                IntLoadMultiTables(*phModule, puFlags, pMulti);

            IntFollowRealDll(phModule, ppszFile, pszRealFile, pszKLID);
        }

        /* Get KbdLayerDescriptor procedure */
        _SEH2_YIELD(return (PFN_KbdLayerDescriptor)GetProcAddress(*phModule,
                                                                  IFN_KbdLayerDescriptor));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ERR("Exception!\n");
    }
    _SEH2_END;

    return NULL;
}

/* Gets the 16-bit offsets of the tables, as NtUserLoadKeyboardLayoutEx expects them:
   LOWORD is the main table and HIWORD is the NLS table (0 if none). */
static BOOL
IntGetTableOffsets(
    _In_ HMODULE hModule,
    _In_ PFN_KbdLayerDescriptor pfnMain,
    _Out_ PULONG poffTable)
{
    PFN_KbdNlsLayerDescriptor pfnNls;
    INT_PTR MainOffset, NlsOffset = 0;

    _SEH2_TRY
    {
        /* Get the main table offset */
        MainOffset = (PBYTE)pfnMain() - (PBYTE)hModule;
        if (MainOffset <= 0 || MainOffset > MAXWORD)
        {
            ERR("MainOffset: %p\n", (PVOID)MainOffset);
            _SEH2_YIELD(return FALSE);
        }

        /* Get the NLS table offset */
        pfnNls = (PFN_KbdNlsLayerDescriptor)GetProcAddress(hModule, IFN_KbdNlsLayerDescriptor);
        if (pfnNls)
        {
            NlsOffset = (PBYTE)pfnNls() - (PBYTE)hModule;
            if (NlsOffset <= 0 || NlsOffset > MAXWORD)
            {
                ERR("NlsOffset: %p\n", (PVOID)NlsOffset);
                _SEH2_YIELD(return FALSE);
            }
        }

        *poffTable = MAKELONG(MainOffset, NlsOffset);
        _SEH2_YIELD(return TRUE);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ERR("Exception!\n");
    }
    _SEH2_END;

    return FALSE;
}

/* Returns a handle to the layout file, or NULL on failure. On success, the handles in pMulti
   (if any) stay open for the caller; on failure, this function has closed them. */
static HANDLE
IntLoadKeyboardLayoutTablesAndFile(
    _In_z_ PCWSTR pszLayoutFile,
    _In_opt_z_ PCWSTR pszKLID,
    _Inout_ PUINT puFlags,
    _Out_ PULONG poffTable,
    _Out_opt_ PKBDTABLE_MULTI_EX pMulti)
{
    HMODULE hModule;
    PCWSTR pszCurrentFile;
    PFN_KbdLayerDescriptor pfnMain;
    WCHAR szRealFile[MAX_LAYOUT_FILENAME], szFull[MAX_PATH];
    DWORD cchFull;
    HANDLE hFile;

    TRACE("Loading layout file %s\n", debugstr_w(pszLayoutFile));
    if (!IntIsValidLayoutFileName(pszLayoutFile))
    {
        ERR("Invalid layout filename: %s\n", debugstr_w(pszLayoutFile));
        return NULL;
    }

    for (;;)
    {
        pszCurrentFile = pszLayoutFile;

        /* Load the layout file */
        hModule = LoadSysLibraryW(pszLayoutFile);
        if (!hModule)
        {
            ERR("LoadSysLibraryW(%s) failed\n", debugstr_w(pszLayoutFile));
            return NULL;
        }

        pfnMain = IntPrepareLayoutModule(&hModule, &pszCurrentFile, szRealFile, pszKLID,
                                         puFlags, pMulti);
        if (pfnMain)
            break; /* Success */

        /* Otherwise, it's a failure. We leave this module either way (pszCurrentFile does not
           point into it), so release it here; a retry loads a fresh one. */
        FreeLibrary(hModule);
        IntCloseAuxFiles(pMulti);

        if ((*puFlags & KLF_INITTIME) && !WCSIEQL(pszCurrentFile, L"kbdus.dll"))
        {
            pszLayoutFile = L"kbdus.dll";
            continue; /* Retry with "kbdus.dll" */
        }

        ERR("Invalid layout file: %s\n", debugstr_w(pszLayoutFile));
        return NULL; /* Failure */
    }

    /* From here on, every failure goes to Cleanup */
    if (!IntGetTableOffsets(hModule, pfnMain, poffTable))
        goto Cleanup;

    if (pMulti)
        StringCbCopyW(pMulti->szRealDllName, sizeof(pMulti->szRealDllName), pszCurrentFile);

    TRACE("FileName:%s, *poffTable:0x%08X\n", debugstr_w(pszCurrentFile), *poffTable);

    /* Get the full pathname of hModule */
    cchFull = GetModuleFileNameW(hModule, szFull, _countof(szFull));
    if (!cchFull || cchFull >= _countof(szFull))
    {
        ERR("GetModuleFileNameW failed\n");
        goto Cleanup;
    }

    FreeLibrary(hModule);
    hModule = NULL;

    hFile = CreateFileW(szFull, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
        return hFile; /* Success */

    ERR("CreateFileW failed: %s\n", debugstr_w(szFull));

Cleanup:
    if (hModule)
        FreeLibrary(hModule);
    IntCloseAuxFiles(pMulti);
    return NULL;
}

static BOOL
IntGetKeyboardDllName(
    _Out_writes_(cchDllName) PWSTR pszDllName,
    _In_ SIZE_T cchDllName,
    _Inout_z_ PWSTR pszKLID,
    _In_ SIZE_T cchKLID,
    _In_ LANGID wLangID,
    _Inout_ PUINT puFlags,
    _Out_ PULONG pdwKL)
{
    HKEY hKey;
    DWORD error, cbData, dwAttributes, dwKL;
    PCWSTR pszSource = NULL;
    WCHAR szKLID[KL_NAMELENGTH], szKeyPath[MAX_PATH], szSubstitute[KL_NAMELENGTH];
    WCHAR szLayoutFile[MAX_PATH], szLayoutId[16];
    HRESULT hr;

    if (FAILED(StringCbCopyW(szKLID, sizeof(szKLID), pszKLID)) || !IsValidKLID(szKLID))
        return FALSE;

    if (!wLangID)
        wLangID = (LANGID)wcstoul(pszKLID, NULL, 16);

    /* Substitute pszKLID */
    if (*puFlags & KLF_SUBSTITUTE_OK)
    {
        /* FIXME: Prepare keyboardlayout.ini */
        GetPrivateProfileStringW(L"Substitutes", pszKLID, pszKLID,
                                 szSubstitute, (DWORD)_countof(szSubstitute),
                                 L"keyboardlayout.ini");
        WritePrivateProfileStringW(NULL, NULL, NULL, NULL); /* Flush! */
        szSubstitute[_countof(szSubstitute) - 1] = UNICODE_NULL; /* Avoid buffer overrun */
        if (!IsValidKLID(szSubstitute))
            return FALSE;
        StringCchCopyW(pszKLID, cchKLID, szSubstitute);
    }

    /* Open 'Keyboard Layouts' registry key */
    dwKL = wcstoul(pszKLID, NULL, 16);
    hr = StringCbPrintfW(szKeyPath, sizeof(szKeyPath), KBDLAYOUTS_KEY L"\\%s", pszKLID);
    if (FAILED(hr))
        return FALSE;

    error = RegOpenKeyExW(HKEY_LOCAL_MACHINE, szKeyPath, 0, KEY_READ, &hKey);
    if (error == ERROR_SUCCESS)
    {
        /* "Layout File" */
        cbData = sizeof(szLayoutFile);
        error = RegQueryValueExW(hKey, L"Layout File", NULL, NULL, (PBYTE)szLayoutFile, &cbData);
        if (error == ERROR_SUCCESS)
        {
            szLayoutFile[_countof(szLayoutFile) - 1] = UNICODE_NULL;
            pszSource = szLayoutFile;
        }

        /* "Attributes" */
        cbData = sizeof(dwAttributes);
        error = RegQueryValueExW(hKey, L"Attributes", NULL, NULL, (PBYTE)&dwAttributes, &cbData);
        if (error == ERROR_SUCCESS)
            *puFlags |= (dwAttributes & ATTR_MASK);

        if (IS_IME_HKL(UlongToHandle(dwKL))) /* IME? */
        {
            dwKL = HIWORD(dwKL);
        }
        else if (HIWORD(dwKL))
        {
            /* "Layout ID" */
            cbData = sizeof(szLayoutId);
            error = RegQueryValueExW(hKey, L"Layout ID", NULL, NULL, (PBYTE)szLayoutId, &cbData);
            if (error == ERROR_SUCCESS)
            {
                szLayoutId[_countof(szLayoutId) - 1] = UNICODE_NULL; /* Avoid buffer overrun */
                dwKL = (wcstol(szLayoutId, NULL, 16) & 0xFFF) | 0xF000;
            }
            else
            {
                dwKL = 0xFFFE; /* Invalid value */
            }
        }

        RegCloseKey(hKey);
    }
    else
    {
        WARN("pszKLID %s not found!\n", debugstr_w(pszKLID));

        /* Fallback to 'Keyboard Layout' */
        error = RegOpenKeyExW(HKEY_LOCAL_MACHINE, KBDLAYOUT_KEY, 0, KEY_READ, &hKey);
        if (error == ERROR_SUCCESS)
        {
            cbData = sizeof(szLayoutFile);
            error = RegQueryValueExW(hKey, pszKLID, NULL, NULL, (PBYTE)szLayoutFile, &cbData);
            if (error == ERROR_SUCCESS)
            {
                szLayoutFile[_countof(szLayoutFile) - 1] = UNICODE_NULL; /* Avoid buffer overrun */
                pszSource = szLayoutFile;
            }
            RegCloseKey(hKey);
        }
    }

    *pdwKL = MAKELONG(wLangID, (WORD)dwKL);

    if (!pszSource) /* Layout not found in registry? */
    {
        WARN("Layout %s not found in registry\n", debugstr_w(pszKLID));

        if (IntIsConsoleSession() || IS_IME_HKL(UlongToHandle(dwKL)))
        {
            pszSource = L"kbdus.dll";
            *pdwKL = MAKELONG(LANGID_ENGLISH_US, LANGID_ENGLISH_US);
        }
        else if (PRIMARYLANGID(wLangID) == LANG_JAPANESE)
        {
            pszSource = L"kbdjpn.dll";
            *pdwKL = MAKELONG(LANGID_JAPANESE, LANGID_JAPANESE);
        }
        else if (PRIMARYLANGID(wLangID) == LANG_KOREAN)
        {
            pszSource = L"kbdkor.dll";
            *pdwKL = MAKELONG(LANGID_KOREAN, LANGID_KOREAN);
        }
        else
        {
            pszSource = L"kbdus.dll";
            *pdwKL = MAKELONG(wLangID, wLangID);
        }
    }

    return IntIsValidLayoutFileName(pszSource) &&
           SUCCEEDED(StringCchCopyW(pszDllName, cchDllName, pszSource));
}

/* Open and load keyboard layout file(s). */
static HANDLE
IntLoadKeyboardLayoutFile(
    _Inout_z_ PWSTR pszKLID,
    _In_ SIZE_T cchKLID,
    _In_ DWORD wLangID,
    _Inout_ PUINT pFlags,
    _Out_ PULONG pOffTable,
    _Out_ PULONG pdwNewKL,
    _Out_opt_ PKBDTABLE_MULTI_EX pMulti)
{
    WCHAR szDllName[MAX_PATH];

    if (pMulti)
        ZeroMemory(pMulti, sizeof(*pMulti));

    if (!IntGetKeyboardDllName(szDllName, _countof(szDllName), pszKLID, cchKLID, wLangID,
                               pFlags, pdwNewKL))
    {
        ERR("Failed in IntGetKeyboardDllName\n");
        return NULL;
    }

    return IntLoadKeyboardLayoutTablesAndFile(szDllName, pszKLID, pFlags, pOffTable, pMulti);
}

/*
 * @implemented
 */
HKL APIENTRY
IntLoadKeyboardLayout(
    _In_opt_ HKL    hklUnload,
    _In_z_   PCWSTR pwszKLID,
    _In_     LANGID wLangID,
    _In_     UINT   Flags,
    _In_     BOOL   bForce)
{
    HANDLE hFile;
    WCHAR szKLID[KL_NAMELENGTH];
    KBDTABLE_MULTI_EX multi;
    DWORD offTable = 0, dwNewKL = 0, iTable;
    HKL hNewKL = NULL;
    UNICODE_STRING ustrKLID;

    if (FAILED(StringCbCopyW(szKLID, sizeof(szKLID), pwszKLID)))
        return NULL;
    szKLID[KL_NAMELENGTH - 1] = UNICODE_NULL;

    /* Load keyboard layout file(s) */
    hFile = IntLoadKeyboardLayoutFile(szKLID, _countof(szKLID), wLangID, &Flags, &offTable,
                                      &dwNewKL, &multi);
    if (!hFile || hFile == INVALID_HANDLE_VALUE)
    {
        if (!bForce && !(Flags & KLF_FORCE))
        {
            ERR("Failed to IntLoadKeyboardLayoutFile\n");
            return NULL; /* Failure */
        }

        /* Fallback to English */
        Flags &= ~KLF_SUBSTITUTE_OK;
        multi.Multi.nTables = 0;
        if (wcscmp(szKLID, L"00000409") != 0) /* Not English? Then retry with English */
        {
            StringCbCopyW(szKLID, sizeof(szKLID), L"00000409");
            hFile = IntLoadKeyboardLayoutFile(szKLID, _countof(szKLID), wLangID, &Flags,
                                              &offTable, &dwNewKL, NULL);
        }
    }

    /* Main dish */
    RtlInitUnicodeString(&ustrKLID, szKLID);
    hNewKL = NtUserLoadKeyboardLayoutEx(hFile, offTable, &multi, hklUnload, &ustrKLID, dwNewKL,
                                        Flags);

    /* Close file handles. We own them: win32k only uses its own duplicates. */
    if (hFile && hFile != INVALID_HANDLE_VALUE)
        NtClose(hFile);
    for (iTable = 0; iTable < multi.Multi.nTables && iTable < KBDTABLE_MULTI_MAX; ++iTable)
    {
        HANDLE hAuxFile = multi.AuxEntries[iTable].hFile;
        if (hAuxFile && hAuxFile != INVALID_HANDLE_VALUE)
            NtClose(hAuxFile);
    }

    if (hNewKL)
    {
        /* Initialize the IME hot keys */
        CliImmInitializeHotKeys(SETIMEHOTKEY_ADD, UlongToHandle(dwNewKL));
    }

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
 * @implemented
 */
HKL WINAPI
LoadKeyboardLayoutEx(
    _In_ HKL hklUnload,
    _In_ PCWSTR pwszKLID,
    _In_ UINT Flags)
{
    TRACE("(%p, %s, 0x%X)", hklUnload, debugstr_w(pwszKLID), Flags);

    if (!hklUnload)
    {
        ERR("hklUnload was NULL\n");
        return NULL;
    }

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
