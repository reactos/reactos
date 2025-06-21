/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing IME installation
 * COPYRIGHT:   Copyright 2020-2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

/* An IME layout entry in registry */
typedef struct tagREG_IME_LAYOUT
{
    HKL hKL;
    WCHAR szImeKey[20];   /* "E0XXYYYY": "E0XX" is the device handle. "YYYY" is a LANGID. */
    WCHAR szFileName[80]; /* The IME module filename */
} REG_IME_LAYOUT, *PREG_IME_LAYOUT;

/* For using version.dll */
typedef BOOL (WINAPI *FN_GetFileVersionInfoW)(LPCWSTR, DWORD, DWORD, LPVOID);
typedef DWORD (WINAPI *FN_GetFileVersionInfoSizeW)(LPCWSTR, LPDWORD);
typedef BOOL (WINAPI *FN_VerQueryValueW)(LPCVOID, LPCWSTR, LPVOID*, PUINT);

static FN_GetFileVersionInfoW s_fnGetFileVersionInfoW = NULL;
static FN_GetFileVersionInfoSizeW s_fnGetFileVersionInfoSizeW = NULL;
static FN_VerQueryValueW s_fnVerQueryValueW = NULL;

/* Used in Imm32CopyImeFile */
typedef INT (WINAPI *FN_LZOpenFileW)(LPWSTR, LPOFSTRUCT, WORD);
typedef LONG (WINAPI *FN_LZCopy)(INT, INT);
typedef VOID (WINAPI *FN_LZClose)(INT);

/* Gets a version information entry by using version!VerQueryValueW */
static LPWSTR
Imm32GetVerInfoValue(
    _In_reads_bytes_(cbVerInfo) LPCVOID pVerInfo,
    _In_ DWORD cbVerInfo,
    _Inout_ PWSTR pszKey,
    _In_ DWORD cchKey,
    _In_ PCWSTR pszName)
{
    size_t cchExtra;
    LPWSTR pszValue;
    UINT cbValue = 0;

    StringCchLengthW(pszKey, cchKey, &cchExtra);

    StringCchCatW(pszKey, cchKey, pszName);
    s_fnVerQueryValueW(pVerInfo, pszKey, (LPVOID *)&pszValue, &cbValue);
    pszKey[cchExtra] = UNICODE_NULL; /* Avoid buffer overrun */

    ASSERT(cbValue <= cbVerInfo);

    return (cbValue ? pszValue : NULL);
}

/* Gets the language and description of an IME */
static BOOL
Imm32LoadImeLangAndDesc(
    _Inout_ PIMEINFOEX pInfoEx,
    _In_reads_bytes_(cbVerInfo) LPCVOID pVerInfo,
    _In_ DWORD cbVerInfo)
{
    /* Getting the version info. See VerQueryValue */
    LPWORD pw;
    UINT cbData;
    BOOL ret = s_fnVerQueryValueW(pVerInfo, L"\\VarFileInfo\\Translation", (LPVOID *)&pw, &cbData);
    if (!ret || !cbData)
    {
        ERR("Translation not available\n");
        return FALSE;
    }

    ASSERT(cbData <= cbVerInfo);

    if (pInfoEx->hkl == NULL)
        pInfoEx->hkl = UlongToHandle(*pw);

    /* Try the current language and the Unicode codepage (0x04B0) */
    LANGID LangID = LANGIDFROMLCID(GetThreadLocale());
    WCHAR szKey[80];
    StringCchPrintfW(szKey, _countof(szKey), L"\\StringFileInfo\\%04X04B0\\", LangID);
    LPWSTR pszDesc = Imm32GetVerInfoValue(pVerInfo, cbVerInfo, szKey, _countof(szKey), L"FileDescription");
    if (!pszDesc)
    {
        /* Retry the language and codepage of the IME module */
        StringCchPrintfW(szKey, _countof(szKey), L"\\StringFileInfo\\%04X%04X\\", pw[0], pw[1]);
        pszDesc = Imm32GetVerInfoValue(pVerInfo, cbVerInfo, szKey, _countof(szKey), L"FileDescription");
    }

    /* The description */
    if (pszDesc)
        StringCchCopyW(pInfoEx->wszImeDescription, _countof(pInfoEx->wszImeDescription), pszDesc);
    else
        pInfoEx->wszImeDescription[0] = UNICODE_NULL;

    return TRUE;
}

/* Gets the fixed version info from IME */
static BOOL
Imm32LoadImeFixedInfo(
    _Out_ PIMEINFOEX pInfoEx,
    _In_reads_bytes_(cbVerInfo) LPCVOID pVerInfo,
    _In_ DWORD cbVerInfo)
{
    UINT cbFixed = 0;
    VS_FIXEDFILEINFO *pFixed;
    if (!s_fnVerQueryValueW(pVerInfo, L"\\", (LPVOID *)&pFixed, &cbFixed) || !cbFixed)
    {
        ERR("Fixed version info not available\n");
        return FALSE;
    }

    ASSERT(cbFixed <= cbVerInfo);

    /* NOTE: The IME module must contain a version info of input method driver. */
    if (pFixed->dwFileType != VFT_DRV || pFixed->dwFileSubtype != VFT2_DRV_INPUTMETHOD)
    {
        ERR("DLL '%s' is not an IME\n", debugstr_w(pInfoEx->wszImeFile));
        return FALSE;
    }

    pInfoEx->dwProdVersion = pFixed->dwProductVersionMS;
    pInfoEx->dwImeWinVersion = IMEVER_0400;
    return TRUE;
}

/* Generates a new IMM IME HKL (0xE0XXYYYY) */
static HKL
Imm32AssignNewLayout(
    _In_ UINT cKLs,
    _In_reads_(cKLs) const REG_IME_LAYOUT *pLayouts,
    _In_ WORD wLangID)
{
#define MIN_SYSTEM_IMM_IME_ID 0xE000
#define MAX_SYSTEM_IMM_IME_ID 0xE0FF
#define MIN_USER_IMM_IME_ID   0xE020
#define MAX_USER_IMM_IME_ID   MAX_SYSTEM_IMM_IME_ID
    UINT iKL, wLow = MAX_USER_IMM_IME_ID, wHigh = MIN_USER_IMM_IME_ID;

    for (iKL = 0; iKL < cKLs; ++iKL)
    {
        wHigh = max(wHigh, HIWORD(pLayouts[iKL].hKL));
        wLow = min(wLow, HIWORD(pLayouts[iKL].hKL));
    }

    UINT wNextImeId = 0;
    if (wHigh < MAX_SYSTEM_IMM_IME_ID)
    {
        wNextImeId = wHigh + 1;
    }
    else if (wLow > MIN_SYSTEM_IMM_IME_ID)
    {
        wNextImeId = wLow - 1;
    }
    else
    {
        UINT wID;
        for (wID = MIN_USER_IMM_IME_ID; wID <= MAX_USER_IMM_IME_ID; ++wID)
        {
            for (iKL = 0; iKL < cKLs; ++iKL)
            {
                if (LOWORD(pLayouts[iKL].hKL) == wLangID && HIWORD(pLayouts[iKL].hKL) == wID)
                    break;
            }

            if (iKL >= cKLs)
                break;
        }

        if (wID <= MAX_SYSTEM_IMM_IME_ID)
            wNextImeId = wID;
    }

    if (!wNextImeId)
    {
        ERR("No next IMM IME ID\n");
        return NULL;
    }

    return UlongToHandle(MAKELONG(wLangID, wNextImeId));
}

static UINT
Imm32GetImeLayoutList(
    _Out_writes_opt_(cLayouts) PREG_IME_LAYOUT pLayouts,
    _In_ UINT cLayouts)
{
    /* Open the registry keyboard layouts */
    HKEY hkeyLayouts;
    LSTATUS lError = RegOpenKeyW(HKEY_LOCAL_MACHINE, REGKEY_KEYBOARD_LAYOUTS, &hkeyLayouts);
    if (IS_ERROR_UNEXPECTEDLY(lError))
        return 0;

    UINT iKey, nCount;
    WCHAR szImeFileName[80], szImeKey[20];
    DWORD cbData, Value;
    HKL hKL;
    for (iKey = nCount = 0; ; ++iKey)
    {
        /* Get the key name */
        lError = RegEnumKeyW(hkeyLayouts, iKey, szImeKey, _countof(szImeKey));
        if (lError != ERROR_SUCCESS)
            break;

        if (szImeKey[0] != L'E' && szImeKey[0] != L'e')
            continue; /* Not an IME layout */

        if (pLayouts == NULL) /* for counting only */
        {
            ++nCount;
            continue;
        }

        if (cLayouts <= nCount)
            break;

        HKEY hkeyIME;
        lError = RegOpenKeyW(hkeyLayouts, szImeKey, &hkeyIME); /* Open the IME key */
        if (IS_ERROR_UNEXPECTEDLY(lError))
            continue;

        /* Load the "Ime File" value */
        szImeFileName[0] = UNICODE_NULL;
        cbData = sizeof(szImeFileName);
        RegQueryValueExW(hkeyIME, L"Ime File", NULL, NULL, (LPBYTE)szImeFileName, &cbData);
        szImeFileName[_countof(szImeFileName) - 1] = UNICODE_NULL; /* Avoid buffer overrun */

        RegCloseKey(hkeyIME);

        /* We don't allow the invalid "IME File" values for security reason */
        if (!szImeFileName[0] || wcscspn(szImeFileName, L":\\/") != wcslen(szImeFileName))
        {
            WARN("Invalid IME Filename (%s)\n", debugstr_w(szImeFileName));
            continue;
        }

        Imm32StrToUInt(szImeKey, &Value, 16);
        hKL = UlongToHandle(Value);
        if (!IS_IME_HKL(hKL)) /* Not an IMM IME */
        {
            WARN("Not IMM IME HKL: %p\n", hKL);
            continue;
        }

        /* Store the IME key and the IME filename */
        pLayouts[nCount].hKL = hKL;
        StringCchCopyW(pLayouts[nCount].szImeKey, _countof(pLayouts[nCount].szImeKey), szImeKey);
        StringCchCopyW(pLayouts[nCount].szFileName, _countof(pLayouts[nCount].szFileName),
                       szImeFileName);
        ++nCount;
    }

    RegCloseKey(hkeyLayouts);
    return nCount;
}

/* Write an IME layout to registry */
static BOOL
Imm32WriteImeLayout(
    _In_ HKL hKL,
    _In_ PCWSTR pchFileTitle,
    _In_ PCWSTR pszLayoutText)
{
    UINT iPreload;
    HKEY hkeyLayouts, hkeyIME, hkeyPreload;
    WCHAR szImeKey[20], szPreloadNumber[20], szPreloadKey[20];
    DWORD cbData;
    LANGID LangID;
    LONG lError;
    LPCWSTR pszLayoutFile;

    /* Open the registry keyboard layouts */
    lError = RegOpenKeyW(HKEY_LOCAL_MACHINE, REGKEY_KEYBOARD_LAYOUTS, &hkeyLayouts);
    if (IS_ERROR_UNEXPECTEDLY(lError))
        return FALSE;

    /* Get the IME key from hKL */
    StringCchPrintfW(szImeKey, _countof(szImeKey), L"%08X", HandleToUlong(hKL));

    /* Create a registry IME key */
    lError = RegCreateKeyW(hkeyLayouts, szImeKey, &hkeyIME);
    if (IS_ERROR_UNEXPECTEDLY(lError))
        goto Failure;

    /* Write "Ime File" */
    cbData = (wcslen(pchFileTitle) + 1) * sizeof(WCHAR);
    lError = RegSetValueExW(hkeyIME, L"Ime File", 0, REG_SZ, (LPBYTE)pchFileTitle, cbData);
    if (IS_ERROR_UNEXPECTEDLY(lError))
        goto Failure;

    /* Write "Layout Text" */
    cbData = (wcslen(pszLayoutText) + 1) * sizeof(WCHAR);
    lError = RegSetValueExW(hkeyIME, L"Layout Text", 0, REG_SZ, (LPBYTE)pszLayoutText, cbData);
    if (IS_ERROR_UNEXPECTEDLY(lError))
        goto Failure;

    /* Choose "Layout File" from hKL */
    LangID = LOWORD(hKL);
    switch (LOBYTE(LangID))
    {
        case LANG_JAPANESE: pszLayoutFile = L"kbdjpn.dll"; break;
        case LANG_KOREAN:   pszLayoutFile = L"kbdkor.dll"; break;
        default:            pszLayoutFile = L"kbdus.dll"; break;
    }

    /* Write "Layout File" */
    cbData = (wcslen(pszLayoutFile) + 1) * sizeof(WCHAR);
    lError = RegSetValueExW(hkeyIME, L"Layout File", 0, REG_SZ, (LPBYTE)pszLayoutFile, cbData);
    if (IS_ERROR_UNEXPECTEDLY(lError))
        goto Failure;

    RegCloseKey(hkeyIME);
    RegCloseKey(hkeyLayouts);

    /* Create "Preload" key */
    RegCreateKeyW(HKEY_CURRENT_USER, L"Keyboard Layout\\Preload", &hkeyPreload);

#define MAX_PRELOAD 0x400
    for (iPreload = 1; iPreload < MAX_PRELOAD; ++iPreload)
    {
        Imm32UIntToStr(iPreload, 10, szPreloadNumber, _countof(szPreloadNumber));

        /* Load the key of the preload number */
        cbData = sizeof(szPreloadKey);
        lError = RegQueryValueExW(hkeyPreload, szPreloadNumber, NULL, NULL,
                                  (LPBYTE)szPreloadKey, &cbData);
        szPreloadKey[_countof(szPreloadKey) - 1] = UNICODE_NULL; /* Avoid buffer overrun */

        if (lError != ERROR_SUCCESS || _wcsicmp(szImeKey, szPreloadKey) == 0)
            break; /* Found an empty room or the same key */
    }

    if (iPreload >= MAX_PRELOAD) /* Not found */
    {
        ERR("iPreload: %u\n", iPreload);
        RegCloseKey(hkeyPreload);
        return FALSE;
    }
#undef MAX_PRELOAD

    /* Write the IME key to the preload number */
    cbData = (wcslen(szImeKey) + 1) * sizeof(WCHAR);
    lError = RegSetValueExW(hkeyPreload, szPreloadNumber, 0, REG_SZ, (LPBYTE)szImeKey, cbData);
    RegCloseKey(hkeyPreload);
    return lError == ERROR_SUCCESS;

Failure:
    RegCloseKey(hkeyIME);
    RegDeleteKeyW(hkeyLayouts, szImeKey);
    RegCloseKey(hkeyLayouts);
    return FALSE;
}

/* Copy an IME file with using LZ decompression */
static BOOL
Imm32CopyImeFile(
    _In_ PCWSTR pszOldFile,
    _In_ PCWSTR pszNewFile)
{
    BOOL ret = FALSE, bLoaded = FALSE;

    /* Load LZ32.dll for copying/decompressing file */
    WCHAR szLZ32Path[MAX_PATH];
    Imm32GetSystemLibraryPath(szLZ32Path, _countof(szLZ32Path), L"LZ32");
    HMODULE hinstLZ32 = GetModuleHandleW(szLZ32Path);
    if (!hinstLZ32)
    {
        hinstLZ32 = LoadLibraryW(szLZ32Path);
        if (IS_NULL_UNEXPECTEDLY(hinstLZ32))
            return FALSE;
        bLoaded = TRUE;
    }

    FN_LZOpenFileW fnLZOpenFileW;
    FN_LZCopy fnLZCopy;
    FN_LZClose fnLZClose;
#define GET_FN(name) do { \
    fn##name = (FN_##name)GetProcAddress(hinstLZ32, #name); \
    if (!fn##name) goto Quit; \
} while (0)
    GET_FN(LZOpenFileW);
    GET_FN(LZCopy);
    GET_FN(LZClose);
#undef GET_FN

    CHAR szDestA[MAX_PATH];
    HFILE hfDest, hfSrc;
    OFSTRUCT OFStruct;

    if (!WideCharToMultiByte(CP_ACP, 0, pszNewFile, -1, szDestA, _countof(szDestA), NULL, NULL))
        goto Quit;
    szDestA[_countof(szDestA) - 1] = ANSI_NULL; /* Avoid buffer overrun */

    hfSrc = fnLZOpenFileW((PWSTR)pszOldFile, &OFStruct, OF_READ);
    if (hfSrc < 0)
        goto Quit;

    hfDest = OpenFile(szDestA, &OFStruct, OF_CREATE);
    if (hfDest != HFILE_ERROR)
    {
        ret = (fnLZCopy(hfSrc, hfDest) >= 0);
        _lclose(hfDest);
    }

    fnLZClose(hfSrc);

Quit:
    if (bLoaded)
        FreeLibrary(hinstLZ32);
    return ret;
}

/* Loads version info of an IME by using version!{GetFileVersionInfo,VerQueryValue} etc. */
BOOL
Imm32LoadImeVerInfo(_Inout_ PIMEINFOEX pImeInfoEx)
{
    HINSTANCE hinstVersion;
    BOOL ret = FALSE, bLoaded = FALSE;
    WCHAR szPath[MAX_PATH];
    LPVOID pVerInfo;
    DWORD cbVerInfo, dwHandle;

    /* Load version.dll to use the version info API */
    Imm32GetSystemLibraryPath(szPath, _countof(szPath), L"version.dll");
    hinstVersion = GetModuleHandleW(szPath);
    if (!hinstVersion)
    {
        hinstVersion = LoadLibraryW(szPath);
        if (IS_NULL_UNEXPECTEDLY(hinstVersion))
            return FALSE;

        bLoaded = TRUE;
    }

#define GET_FN(name) do { \
    s_fn##name = (FN_##name)GetProcAddress(hinstVersion, #name); \
    if (!s_fn##name) goto Quit; \
} while (0)
    GET_FN(GetFileVersionInfoW);
    GET_FN(GetFileVersionInfoSizeW);
    GET_FN(VerQueryValueW);
#undef GET_FN

    /* The path of the IME module */
    Imm32GetSystemLibraryPath(szPath, _countof(szPath), pImeInfoEx->wszImeFile);

    cbVerInfo = s_fnGetFileVersionInfoSizeW(szPath, &dwHandle);
    if (IS_ZERO_UNEXPECTEDLY(cbVerInfo))
        goto Quit;

    pVerInfo = ImmLocalAlloc(0, cbVerInfo);
    if (IS_NULL_UNEXPECTEDLY(pVerInfo))
        goto Quit;

    /* Load the version info of the IME module */
    if (s_fnGetFileVersionInfoW(szPath, dwHandle, cbVerInfo, pVerInfo) &&
        Imm32LoadImeFixedInfo(pImeInfoEx, pVerInfo, cbVerInfo))
    {
        ret = Imm32LoadImeLangAndDesc(pImeInfoEx, pVerInfo, cbVerInfo);
    }

    ImmLocalFree(pVerInfo);

Quit:
    if (bLoaded)
        FreeLibrary(hinstVersion);
    TRACE("ret: %d\n", ret);
    return ret;
}

/***********************************************************************
 *		ImmInstallIMEA (IMM32.@)
 */
HKL WINAPI
ImmInstallIMEA(
    _In_ LPCSTR lpszIMEFileName,
    _In_ LPCSTR lpszLayoutText)
{
    HKL hKL = NULL;
    LPWSTR pszFileNameW = NULL, pszLayoutTextW = NULL;

    TRACE("(%s, %s)\n", debugstr_a(lpszIMEFileName), debugstr_a(lpszLayoutText));

    pszFileNameW = Imm32WideFromAnsi(CP_ACP, lpszIMEFileName);
    if (IS_NULL_UNEXPECTEDLY(pszFileNameW))
        goto Quit;

    pszLayoutTextW = Imm32WideFromAnsi(CP_ACP, lpszLayoutText);
    if (IS_NULL_UNEXPECTEDLY(pszLayoutTextW))
        goto Quit;

    hKL = ImmInstallIMEW(pszFileNameW, pszLayoutTextW);

Quit:
    ImmLocalFree(pszFileNameW);
    ImmLocalFree(pszLayoutTextW);
    return hKL;
}

/***********************************************************************
 *		ImmInstallIMEW (IMM32.@)
 */
HKL WINAPI
ImmInstallIMEW(
    _In_ LPCWSTR lpszIMEFileName,
    _In_ LPCWSTR lpszLayoutText)
{
    TRACE("(%s, %s)\n", debugstr_w(lpszIMEFileName), debugstr_w(lpszLayoutText));

    /* Get full pathname and file title */
    WCHAR szImeFileName[MAX_PATH];
    PWSTR pchFileTitle;
    GetFullPathNameW(lpszIMEFileName, _countof(szImeFileName), szImeFileName, &pchFileTitle);
    if (IS_NULL_UNEXPECTEDLY(pchFileTitle))
        return NULL;

    /* Load the IME version info */
    IMEINFOEX InfoEx;
    HKL hNewKL;
    InfoEx.hkl = hNewKL = NULL;
    StringCchCopyW(InfoEx.wszImeFile, _countof(InfoEx.wszImeFile), pchFileTitle);
    if (!Imm32LoadImeVerInfo(&InfoEx) || !InfoEx.hkl)
    {
        ERR("Can't get version info (error: %lu)\n", GetLastError());
        return NULL;
    }
    WORD wLangID = LOWORD(InfoEx.hkl);

    /* Get the IME layouts from registry */
    PREG_IME_LAYOUT pLayouts = NULL;
    UINT iLayout, cLayouts = Imm32GetImeLayoutList(NULL, 0);
    if (cLayouts)
    {
        pLayouts = ImmLocalAlloc(0, cLayouts * sizeof(REG_IME_LAYOUT));
        if (IS_NULL_UNEXPECTEDLY(pLayouts))
            return NULL;

        if (!Imm32GetImeLayoutList(pLayouts, cLayouts))
        {
            ERR("Can't get IME layouts\n");
            ImmLocalFree(pLayouts);
            return NULL;
        }

        for (iLayout = 0; iLayout < cLayouts; ++iLayout)
        {
            if (_wcsicmp(pLayouts[iLayout].szFileName, pchFileTitle) == 0)
            {
                if (wLangID != LOWORD(pLayouts[iLayout].hKL))
                {
                    ERR("The language is mismatched\n");
                    goto Quit;
                }

                hNewKL = pLayouts[iLayout].hKL; /* Found */
                break;
            }
        }
    }

    /* If the IME for the specified filename is already loaded, then unload it now */
    if (ImmGetImeInfoEx(&InfoEx, ImeInfoExImeFileName, pchFileTitle) &&
        !UnloadKeyboardLayout(InfoEx.hkl))
    {
        ERR("Can't unload laybout\n");
        hNewKL = NULL;
        goto Quit;
    }

    WCHAR szImeDestPath[MAX_PATH], szImeKey[20];
    Imm32GetSystemLibraryPath(szImeDestPath, _countof(szImeDestPath), pchFileTitle);

    /* If the source and the destination pathnames were different, then copy the IME file */
    if (_wcsicmp(szImeFileName, szImeDestPath) != 0 &&
        !Imm32CopyImeFile(szImeFileName, szImeDestPath))
    {
        ERR("Can't copy file (%s -> %s)\n", debugstr_w(szImeFileName), debugstr_w(szImeDestPath));
        hNewKL = NULL;
        goto Quit;
    }

    if (hNewKL == NULL)
        hNewKL = Imm32AssignNewLayout(cLayouts, pLayouts, wLangID);

    if (hNewKL)
    {
        /* Write the IME layout to registry */
        if (Imm32WriteImeLayout(hNewKL, pchFileTitle, lpszLayoutText))
        {
            /* Replace the current keyboard layout */
            StringCchPrintfW(szImeKey, _countof(szImeKey), L"%08X", HandleToUlong(hNewKL));
            hNewKL = LoadKeyboardLayoutW(szImeKey, KLF_REPLACELANG);
        }
        else
        {
            ERR("Can't write IME layout to registry\n");
            hNewKL = NULL;
        }
    }

Quit:
    ImmLocalFree(pLayouts);
    return hNewKL;
}
