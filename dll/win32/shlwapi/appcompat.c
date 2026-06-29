/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Shell application compatibility flags
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include <winreg.h>
#include <winver.h>
#include <commctrl.h>
#include <shlwapi_undoc.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

// Indicates if the compatibility system has been initialized
static BOOL g_fAppCompatInitialized = FALSE;

static DWORD g_dwAppCompatFlags = 0; // Cached compatibility flags

// SHACF flags and their corresponding names
typedef struct tagFLAGMAP
{
    DWORD flags;
    LPCSTR name;
} FLAGMAP, *PFLAGMAP;
static FLAGMAP g_appCompatFlagMaps[] =
{
    { SHACF_CONTEXTMENU, "CONTEXTMENU" },
    { SHACF_CORELINTERNETENUM, "CORELINTERNETENUM" },
    { SHACF_OLDCREATEVIEWWND, "OLDCREATEVIEWWND" },
    { SHACF_WIN95DEFVIEW, "WIN95DEFVIEW" },
    { SHACF_DOCOBJECT, "DOCOBJECT" },
    { SHACF_FLUSHNOWAITALWAYS, "FLUSHNOWAITALWAYS" },
    { SHACF_MYCOMPUTERFIRST, "MYCOMPUTERFIRST" },
    { SHACF_OLDREGITEMGDN, "OLDREGITEMGDN" },
    { SHACF_LOADCOLUMNHANDLER, "LOADCOLUMNHANDLER" },
    { SHACF_ANSI, "ANSI" },
    { SHACF_STAROFFICE5PRINTER, "STAROFFICE5PRINTER" },
    { SHACF_NOVALIDATEFSIDS, "NOVALIDATEFSIDS" },
    { SHACF_WIN95SHLEXEC, "WIN95SHLEXEC" },
    { SHACF_FILEOPENNEEDSEXT, "FILEOPENNEEDSEXT" },
    { SHACF_WIN95BINDTOOBJECT, "WIN95BINDTOOBJECT" },
    { SHACF_IGNOREENUMRESET, "IGNOREENUMRESET" },
    { SHACF_ANSIDISPLAYNAMES, "ANSIDISPLAYNAMES" },
    { SHACF_FILEOPENBOGUSCTRLID, "FILEOPENBOGUSCTRLID" },
    { SHACF_FORCELFNIDLIST, "FORCELFNIDLIST" },
};

// Get compatibility flags from registry values
static DWORD
SHLWAPI_GetMappedFlags(_In_ HKEY hKey, _In_ const FLAGMAP *pEntries, _In_ UINT nEntries)
{
    DWORD flags = 0;
    for (UINT iEntry = 0; iEntry < nEntries; ++iEntry)
    {
        DWORD error = SHGetValueA(hKey, NULL, pEntries[iEntry].name, NULL, NULL, NULL);
        if (error == ERROR_SUCCESS)
            flags |= pEntries[iEntry].flags;
    }
    return flags;
}

#define MAJOR_VER_ONLY "\x01" // A mark to indicate that only major version will be compared.

// App compatibility info
// Extracted from: https://www.geoffchappell.com/studies/windows/shell/shlwapi/api/util/getappcompatflags.htm
typedef struct tagAPPCOMPATINFO
{
    PCSTR pszAppName;
    PCSTR pszAppVersion;
    DWORD dwCompatFlags;
} APPCOMPATINFO, *PAPPCOMPATINFO;
static APPCOMPATINFO g_appCompatInfo[] =
{
    { "abcmm.exe", NULL, SHACF_LOADCOLUMNHANDLER },
    { "autorun.exe", "4.00.950", SHACF_ANSI },
    { "autorun.exe", "4.10.1998", SHACF_ANSI },
    { "coreldrw.exe", MAJOR_VER_ONLY "7", SHACF_OLDREGITEMGDN },
    { "dad9.exe", MAJOR_VER_ONLY "9", SHACF_CORELINTERNETENUM },
    { "filler51.exe", NULL, SHACF_OLDREGITEMGDN },
    { "hotdog4.exe", NULL, SHACF_DOCOBJECT },
    { "msmoney.exe", "7.05.1107", SHACF_WIN95SHLEXEC },
    { "pdexplo.exe", MAJOR_VER_ONLY "1", SHACF_CONTEXTMENU | SHACF_MYCOMPUTERFIRST },
    { "pdexplo.exe", MAJOR_VER_ONLY "2", SHACF_CONTEXTMENU | SHACF_MYCOMPUTERFIRST },
    { "pdexplo.exe", MAJOR_VER_ONLY "3", SHACF_MYCOMPUTERFIRST | SHACF_OLDREGITEMGDN },
    { "pdxwin32.exe", NULL, SHACF_CONTEXTMENU | SHACF_CORELINTERNETENUM  | SHACF_OLDREGITEMGDN },
    { "pfim80.exe", NULL, SHACF_CONTEXTMENU | SHACF_CORELINTERNETENUM  | SHACF_OLDREGITEMGDN },
    { "powerpnt.exe", MAJOR_VER_ONLY "8", SHACF_WIN95SHLEXEC },
    { "pp70.exe", NULL, SHACF_LOADCOLUMNHANDLER },
    { "pp80.exe", NULL, SHACF_LOADCOLUMNHANDLER },
    { "prwin70.exe", NULL, SHACF_CONTEXTMENU | SHACF_CORELINTERNETENUM },
    { "prwin8.exe", NULL, SHACF_CORELINTERNETENUM | SHACF_OLDREGITEMGDN },
    { "prwin9.exe", MAJOR_VER_ONLY "9", SHACF_CORELINTERNETENUM },
    { "ps80.exe", NULL, SHACF_CONTEXTMENU | SHACF_CORELINTERNETENUM | SHACF_OLDREGITEMGDN },
    { "ps80.exe", NULL, SHACF_OLDREGITEMGDN },
    { "qfinder.exe", NULL, SHACF_CORELINTERNETENUM | SHACF_OLDREGITEMGDN },
    { "qpw.exe", MAJOR_VER_ONLY "7", SHACF_CONTEXTMENU },
    { "qpw.exe", MAJOR_VER_ONLY "8", SHACF_CORELINTERNETENUM | SHACF_OLDREGITEMGDN | SHACF_ANSIDISPLAYNAMES },
    { "qpw.exe", MAJOR_VER_ONLY "9", SHACF_CORELINTERNETENUM },
    { "rnaapp.exe", NULL, SHACF_CONTEXTMENU },
    { "sitebuilder.exe", NULL, SHACF_CONTEXTMENU | SHACF_CORELINTERNETENUM  | SHACF_OLDREGITEMGDN },
    { "sizemgr.exe", MAJOR_VER_ONLY "3", SHACF_CORELINTERNETENUM | SHACF_OLDREGITEMGDN },
    { "smartctr.exe", "96.0", SHACF_CONTEXTMENU },
    { "soffice.exe", MAJOR_VER_ONLY "5", SHACF_STAROFFICE5PRINTER },
    { "ua80.exe", NULL, SHACF_CONTEXTMENU | SHACF_CORELINTERNETENUM  | SHACF_OLDREGITEMGDN },
    { "ue32.exe", "2.00.0.0", SHACF_OLDREGITEMGDN },
    { "wpwin7.exe", NULL, SHACF_CONTEXTMENU | SHACF_CORELINTERNETENUM },
    { "wpwin8.exe", NULL, SHACF_CORELINTERNETENUM | SHACF_OLDREGITEMGDN },
    { "wpwin9.exe", MAJOR_VER_ONLY "9", SHACF_CORELINTERNETENUM },
};

// Window class name and compatibility flags
typedef struct tagWNDCOMPATINFO
{
    PCSTR pszLengthAndClassName;
    DWORD dwCompatFlags;
} WNDCOMPATINFO, *PWNDCOMPATINFO;
static WNDCOMPATINFO g_wndCompatInfo[] =
{
    // The first byte indicates the string length
    { "\x09" "bosa_sdm_", SHACF_UNKNOWN1 | SHACF_UNKNOWN2 },
    { "\x18" "File Open Message Window", SHACF_UNKNOWN1 | SHACF_UNKNOWN2 },
};

// Internal structure for SHLWAPI_WndCompatEnumProc
typedef struct tagAPPCOMPATENUM
{
    PWNDCOMPATINFO pItems;
    SIZE_T nItems;
    DWORD dwProcessId;
    INT iFound;
} APPCOMPATENUM, *PAPPCOMPATENUM;

static BOOL CALLBACK
SHLWAPI_WndCompatEnumProc(_In_ HWND hWnd, _In_ LPARAM lParam)
{
    PAPPCOMPATENUM pEnum = (PAPPCOMPATENUM)lParam;

    CHAR szClass[256];
    if (!pEnum->nItems || !GetClassNameA(hWnd, szClass, _countof(szClass)))
        return TRUE; // Ignore and continue

    // Search for the target window in pEnum
    const INT cchClass = lstrlenA(szClass);
    for (UINT iItem = 0; iItem < pEnum->nItems; ++iItem)
    {
        PCSTR pszLengthAndClassName = pEnum->pItems[iItem].pszLengthAndClassName;

        INT cchLength = pszLengthAndClassName[0]; // The first byte represents the length
        if (cchClass < cchLength)
            cchLength = cchClass; // Ignore trailing characters

        // Compare the string
        if (StrCmpNA(szClass, &pszLengthAndClassName[1], cchLength) == 0) // Class name matched
        {
            // Get the process ID
            DWORD dwProcessId;
            GetWindowThreadProcessId(hWnd, &dwProcessId);
            if (dwProcessId == pEnum->dwProcessId) // Same process
            {
                pEnum->iFound = iItem; // Found
                return FALSE; // Quit enumeration
            }
        }
    }

    return TRUE; // Continue enumeration
}

// English (US) UTF-16
#define PRODUCT_VER_ENGLISH_US_UTF16    "\\StringFileInfo\\040904E4\\ProductVersion"
// German (Germany) UTF-16
#define PRODUCT_VER_GERMAN_UTF16        "\\StringFileInfo\\040704E4\\ProductVersion"
// English (US) Western European
#define PRODUCT_VER_ENGLISH_US_WE       "\\StringFileInfo\\040904B0\\ProductVersion"
// English (US) Neutral
#define PRODUCT_VER_ENGLISH_US_NEUTRAL  "\\StringFileInfo\\04090000\\ProductVersion"
// Swedish (Sweden) Western European
#define PRODUCT_VER_SWEDISH_WE          "\\StringFileInfo\\041D04B0\\ProductVersion"

typedef struct tagLANGANDCODEPAGE
{
    WORD wLanguage;
    WORD wCodePage;
} LANGANDCODEPAGE, *PLANGANDCODEPAGE;

static HRESULT
SHLWAPI_GetModuleVersion(_In_ PCSTR pszFileName, _Out_ PSTR *ppszDest)
{
    DWORD dwHandle;
    PBYTE pbData;
    PVOID pvData;

    *ppszDest = NULL;

    // Obtain the version info
    UINT size = GetFileVersionInfoSizeA(pszFileName, &dwHandle);
    if (!size)
    {
        TRACE("No version info\n");
        return E_FAIL;
    }
    pbData = (PBYTE)LocalAlloc(LPTR, size);
    if (!pbData)
    {
        ERR("E_OUTOFMEMORY\n");
        return E_OUTOFMEMORY;
    }
    GetFileVersionInfoA(pszFileName, dwHandle, size, pbData);

    // Getting the product version
    HRESULT hr = E_FAIL;
    if ((VerQueryValueA(pbData, PRODUCT_VER_ENGLISH_US_UTF16,   &pvData, &size) ||
         VerQueryValueA(pbData, PRODUCT_VER_GERMAN_UTF16,       &pvData, &size) ||
         VerQueryValueA(pbData, PRODUCT_VER_ENGLISH_US_WE,      &pvData, &size) ||
         VerQueryValueA(pbData, PRODUCT_VER_ENGLISH_US_NEUTRAL, &pvData, &size) ||
         VerQueryValueA(pbData, PRODUCT_VER_SWEDISH_WE,         &pvData, &size)) && size)
    {
        // NOTE: *ppszDest must be freed using LocalFree later
        *ppszDest = StrDupA((PSTR)pvData);
        hr = *ppszDest ? S_OK : E_OUTOFMEMORY;
    }
    else if (VerQueryValueA(pbData, "\\VarFileInfo\\Translation", &pvData, &size))
    {
        PVOID pDataSaved = pvData;
        PLANGANDCODEPAGE pEntry = (PLANGANDCODEPAGE)pvData;
        for (; (PBYTE)pEntry + sizeof(LANGANDCODEPAGE) <= (PBYTE)pDataSaved + size; ++pEntry)
        {
            CHAR szPath[MAX_PATH];
            wnsprintfA(szPath, _countof(szPath), "\\StringFileInfo\\%04X%04X\\ProductVersion",
                       pEntry->wLanguage, pEntry->wCodePage);
            if (VerQueryValueA(pbData, szPath, &pvData, &size) && size)
            {
                // NOTE: *ppszDest must be freed using LocalFree later
                *ppszDest = StrDupA((PSTR)pvData);
                hr = *ppszDest ? S_OK : E_OUTOFMEMORY;
            }
        }
    }

    if (FAILED(hr))
        WARN("hr: 0x%lX\n", hr);

    LocalFree(pbData);
    return hr;
}

static BOOL
SHLWAPI_DoesModuleVersionMatch(_In_ PCSTR pszFileName, _In_opt_ PCSTR pszVersionPattern)
{
    if (!pszVersionPattern)
        return TRUE;

    PSTR pszModuleVersion = NULL;
    HRESULT hr = SHLWAPI_GetModuleVersion(pszFileName, &pszModuleVersion);
    if (FAILED(hr))
        return FALSE;

    BOOL ret = FALSE;
    if (pszVersionPattern[0] == MAJOR_VER_ONLY[0]) // Special handling for major version only
    {
        // Truncate at comma (',') if present
        PSTR pchComma = StrChrA(pszModuleVersion, ',');
        if (pchComma)
            *pchComma = ANSI_NULL;

        // Truncate at dot ('.') if present
        PSTR pchDot = StrChrA(pszModuleVersion, '.');
        if (pchDot)
            *pchDot = ANSI_NULL;

        ret = (lstrcmpiA(pszModuleVersion, &pszVersionPattern[1]) == 0);
    }
    else // Otherwise, perform a normal match
    {
        PSTR pchAsterisk = StrChrA(pszVersionPattern, '*'); // Find an asterisk ('*')
        if (pchAsterisk) // Asterisk found
        {
            // Check for a match, ignoring the substring after '*'
            INT cchPrefix = (INT)(pchAsterisk - pszVersionPattern);
            if (cchPrefix > 0)
                ret = (StrCmpNIA(pszModuleVersion, pszVersionPattern, cchPrefix) == 0);
        }

        if (!ret)
            ret = (lstrcmpiA(pszModuleVersion, pszVersionPattern) == 0); // Full match
    }

    LocalFree(pszModuleVersion);
    return ret;
}

static DWORD
SHLWAPI_GetRegistryCompatFlags(_In_ PCSTR pszPath)
{
    // Build the path to the "application compatibility" registry key
    CHAR szText[MAX_PATH];
    wnsprintfA(szText, _countof(szText),
               "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ShellCompatibility\\Applications\\%s",
               PathFindFileNameA(pszPath));

    // Open the key
    HKEY hKey;
    const REGSAM samDesired = KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS;
    LSTATUS error = RegOpenKeyExA(HKEY_LOCAL_MACHINE, szText, 0, samDesired, &hKey);
    if (error != ERROR_SUCCESS)
    {
        ERR("error: %lu\n", error);
        return 0; // Failed
    }

    // Build the base directory
    CHAR szBaseDir[MAX_PATH];
    lstrcpynA(szBaseDir, pszPath, _countof(szBaseDir));
    PathRemoveFileSpecA(szBaseDir);

    // Search from the registry key
    DWORD dwCompatFlags = 0;
    szText[0] = ANSI_NULL; // The first attempt is for the non-subkey path
    for (DWORD dwIndex = 0; error == ERROR_SUCCESS;)
    {
        HKEY hSubKey;
        error = RegOpenKeyExA(hKey, szText, 0, KEY_QUERY_VALUE, &hSubKey);
        if (error != ERROR_SUCCESS)
            break;

        // Get the "RequiredFile" value
        CHAR szRequired[MAX_PATH];
        DWORD cbData = sizeof(szRequired);
        error = SHGetValueA(hSubKey, NULL, "RequiredFile", NULL, szRequired, &cbData);
        BOOL bValueExists = (error == ERROR_SUCCESS);
        BOOL bRequiredFileExists = FALSE;
        if (bValueExists) // Does the "RequiredFile" value exist?
        {
            // Build required file path to szText
            PathCombineA(szText, szBaseDir, szRequired);
            TRACE("RequiredFile: %s\n", wine_dbgstr_a(szRequired));
            TRACE("szText: %s\n", wine_dbgstr_a(szText));
            // Now szText is a full path
            bRequiredFileExists = (GetFileAttributesA(szText) != INVALID_FILE_ATTRIBUTES);
        }

        // If the "RequiredFile" value doesn't exist, or the file specified by szText exists:
        if (!bValueExists || bRequiredFileExists)
        {
            // Check the "Version" value if necessary
            error = SHGetValueA(hSubKey, NULL, "Version", NULL, szText, &cbData);
            PCSTR pszVersionPattern = ((error == ERROR_SUCCESS) ? szText : NULL);
            TRACE("pszVersionPattern: %s\n", wine_dbgstr_a(pszVersionPattern));

            // Does the pattern match?
            if (SHLWAPI_DoesModuleVersionMatch(pszPath, pszVersionPattern))
            {
                // Add additional flags from the registry key
                dwCompatFlags |= SHLWAPI_GetMappedFlags(hSubKey, g_appCompatFlagMaps,
                                                        _countof(g_appCompatFlagMaps));
            }
        }

        RegCloseKey(hSubKey);

        // Go to the next sub-key
        ++dwIndex;
        error = RegEnumKeyA(hKey, dwIndex, szText, _countof(szText));
    }

    RegCloseKey(hKey);

    return dwCompatFlags;
}

static DWORD
SHLWAPI_InitAppCompat(VOID)
{
    if (GetProcessVersion(0) >= MAKELONG(0, 5))
        return 0; // Flags are not needed

    // Get module pathname
    CHAR szModulePathA[MAX_PATH];
    if (!GetModuleFileNameA(NULL, szModulePathA, _countof(szModulePathA)))
        return 0;

    PCSTR pszFileName = PathFindFileNameA(szModulePathA); // Get the file title from the path

    // Search the file title from g_appCompatInfo
    DWORD dwAppCompatFlags = 0;
    for (UINT iItem = 0; iItem < _countof(g_appCompatInfo); ++iItem)
    {
        const APPCOMPATINFO *pInfo = &g_appCompatInfo[iItem];
        if (lstrcmpiA(pInfo->pszAppName, pszFileName) == 0 &&
            SHLWAPI_DoesModuleVersionMatch(pszFileName, pInfo->pszAppVersion))
        {
            // Found, set the flags
            dwAppCompatFlags = g_appCompatInfo[iItem].dwCompatFlags;
            break;
        }
    }

    // Add additional flags from the registry
    dwAppCompatFlags |= SHLWAPI_GetRegistryCompatFlags(pszFileName);

    return dwAppCompatFlags;
}

// These flags require SHLWAPI_InitAppCompat
#define SHACF_TO_INIT ( \
    SHACF_CONTEXTMENU | \
    SHACF_DOCOBJECT | \
    SHACF_CORELINTERNETENUM | \
    SHACF_MYCOMPUTERFIRST | \
    SHACF_OLDREGITEMGDN | \
    SHACF_LOADCOLUMNHANDLER | \
    SHACF_ANSI | \
    SHACF_WIN95SHLEXEC | \
    SHACF_STAROFFICE5PRINTER | \
    SHACF_NOVALIDATEFSIDS | \
    SHACF_FILEOPENNEEDSEXT | \
    SHACF_WIN95BINDTOOBJECT | \
    SHACF_IGNOREENUMRESET | \
    SHACF_ANSIDISPLAYNAMES | \
    SHACF_FILEOPENBOGUSCTRLID | \
    SHACF_FORCELFNIDLIST \
)

/*************************************************************************
 * SHGetAppCompatFlags [SHLWAPI.461]
 *
 * Thanks to Geoff Chappell.
 * @see https://www.geoffchappell.com/studies/windows/shell/shlwapi/api/util/getappcompatflags.htm
 */
DWORD WINAPI
SHGetAppCompatFlags(_In_ DWORD dwMask)
{
    TRACE("(0x%lX)\n", dwMask);

    // Initialize and retrieve flags if necessary
    if (dwMask & SHACF_TO_INIT)
    {
        if (!g_fAppCompatInitialized)
        {
            DWORD dwAppCompatFlags = SHLWAPI_InitAppCompat();
            InterlockedExchange((PLONG)&g_dwAppCompatFlags, dwAppCompatFlags);

            g_fAppCompatInitialized = TRUE; // Mark as initialized
        }
    }

    // Retrieve additional flags if necessary
    DWORD dwAppCompatFlags = g_dwAppCompatFlags;
    if (dwAppCompatFlags && (dwMask & (SHACF_UNKNOWN1 | SHACF_UNKNOWN2)))
    {
        // Find the target window and its flags using g_wndCompatInfo
        APPCOMPATENUM data =
        {
            g_wndCompatInfo, _countof(g_wndCompatInfo), GetCurrentProcessId(), -1
        };
        EnumWindows(SHLWAPI_WndCompatEnumProc, (LPARAM)&data);

        // Add the target flags if a match is found
        if (data.iFound >= 0)
            dwAppCompatFlags |= g_wndCompatInfo[data.iFound].dwCompatFlags;

        dwAppCompatFlags |= SHACF_UNKNOWN3;

        InterlockedExchange((PLONG)&g_dwAppCompatFlags, dwAppCompatFlags);
    }

    return dwAppCompatFlags;
}

// FIXME: SHGetObjectCompatFlags
