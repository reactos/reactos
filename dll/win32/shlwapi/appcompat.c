/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Shell application compatibility flags
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winnls.h>
#include <winreg.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shlwapi_undoc.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(shell);

static BOOL g_bInitAppCompat = FALSE; // Is it initialized?
static DWORD g_dwAppCompatFlags = 0; // The cached compatibility flags

// Exclusive Control
CRITICAL_SECTION g_csAppCompatLock; // A critical section (initialized in shlwapi_main.c)
#define AppCompat_Lock()    EnterCriticalSection(&g_csAppCompatLock);
#define AppCompat_Unlock()  LeaveCriticalSection(&g_csAppCompatLock);

// The SHACF_... flags and flag names
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
typedef struct tagAPPCOMPATINFO
{
    PCSTR pszAppName;
    PCSTR pszAppVersion;
    DWORD dwCompatFlags;
} APPCOMPATINFO, *PAPPCOMPATINFO;
static APPCOMPATINFO g_appCompatInfo[] =
{
    { "wpwin7.exe", NULL, SHACF_CONTEXTMENU | SHACF_CORELINTERNETENUM },
    { "prwin70.exe", NULL, SHACF_CONTEXTMENU | SHACF_CORELINTERNETENUM },
    { "ps80.exe", NULL, SHACF_CONTEXTMENU | SHACF_CORELINTERNETENUM | SHACF_OLDREGITEMGDN },
    { "qpw.exe", MAJOR_VER_ONLY "7", SHACF_CONTEXTMENU },
    { "qfinder.exe", NULL, SHACF_CORELINTERNETENUM | SHACF_OLDREGITEMGDN },
    { "pfim80.exe", NULL, SHACF_CONTEXTMENU | SHACF_CORELINTERNETENUM  | SHACF_OLDREGITEMGDN },
    { "ua80.exe", NULL, SHACF_CONTEXTMENU | SHACF_CORELINTERNETENUM  | SHACF_OLDREGITEMGDN },
    { "pdxwin32.exe", NULL, SHACF_CONTEXTMENU | SHACF_CORELINTERNETENUM  | SHACF_OLDREGITEMGDN },
    { "sitebuilder.exe", NULL, SHACF_CONTEXTMENU | SHACF_CORELINTERNETENUM  | SHACF_OLDREGITEMGDN },
    { "hotdog4.exe", NULL, SHACF_DOCOBJECT },
    { "rnaapp.exe", NULL, SHACF_CONTEXTMENU },
    { "pdexplo.exe", MAJOR_VER_ONLY "2", SHACF_CONTEXTMENU | SHACF_MYCOMPUTERFIRST },
    { "pdexplo.exe", MAJOR_VER_ONLY "1", SHACF_CONTEXTMENU | SHACF_MYCOMPUTERFIRST },
    { "pdexplo.exe", MAJOR_VER_ONLY "3", SHACF_MYCOMPUTERFIRST | SHACF_OLDREGITEMGDN },
    { "sizemgr.exe", MAJOR_VER_ONLY "3", SHACF_CORELINTERNETENUM | SHACF_OLDREGITEMGDN },
    { "smartctr.exe", "96.0", SHACF_CONTEXTMENU },
    { "wpwin8.exe", NULL, SHACF_CORELINTERNETENUM | SHACF_OLDREGITEMGDN },
    { "prwin8.exe", NULL, SHACF_CORELINTERNETENUM | SHACF_OLDREGITEMGDN },
    { "ue32.exe", "2.00.0.0", SHACF_OLDREGITEMGDN },
    { "pp70.exe", NULL, SHACF_LOADCOLUMNHANDLER },
    { "pp80.exe", NULL, SHACF_LOADCOLUMNHANDLER },
    { "ps80.exe", NULL, SHACF_OLDREGITEMGDN },
    { "abcmm.exe", NULL, SHACF_LOADCOLUMNHANDLER },
    { "qpw.exe", MAJOR_VER_ONLY "8", SHACF_CORELINTERNETENUM | SHACF_OLDREGITEMGDN | SHACF_ANSIDISPLAYNAMES },
    { "coreldrw.exe", MAJOR_VER_ONLY "7", SHACF_OLDREGITEMGDN },
    { "filler51.exe", NULL, SHACF_OLDREGITEMGDN },
    { "autorun.exe", "4.10.1998", SHACF_ANSI },
    { "autorun.exe", "4.00.950", SHACF_ANSI },
    { "powerpnt.exe", MAJOR_VER_ONLY "8", SHACF_WIN95SHLEXEC },
    { "msmoney.exe", "7.05.1107", SHACF_WIN95SHLEXEC },
    { "soffice.exe", MAJOR_VER_ONLY "5", SHACF_STAROFFICE5PRINTER },
    { "wpwin9.exe", MAJOR_VER_ONLY "9", SHACF_CORELINTERNETENUM },
    { "qpw.exe", MAJOR_VER_ONLY "9", SHACF_CORELINTERNETENUM },
    { "prwin9.exe", MAJOR_VER_ONLY "9", SHACF_CORELINTERNETENUM },
    { "dad9.exe", MAJOR_VER_ONLY "9", SHACF_CORELINTERNETENUM },
};

// Window class name and compatibility flags
typedef struct tagWNDCOMPATINFO
{
    PCSTR pszLengthAndClassName;
    DWORD dwCompatFlags;
} WNDCOMPATINFO, *PWNDCOMPATINFO;
static WNDCOMPATINFO g_wndCompatInfo[] =
{
    // The first byte is the length of string
    { "\x09" "bosa_sdm_", 0x1000100 },
    { "\x18" "File Open Message Window", 0x1000100 },
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
        return TRUE; // Ignore it, continue

    // Search the target window from pEnum
    const INT cchClass = lstrlenA(szClass);
    for (UINT iItem = 0; iItem < pEnum->nItems; ++iItem)
    {
        PCSTR pszLengthAndClassName = pEnum->pItems[iItem].pszLengthAndClassName;

        INT cchLength = pszLengthAndClassName[0]; // First byte is length
        if (cchClass < cchLength)
            cchLength = cchClass; // Ignore the trailing

        // Compare the string
        if (StrCmpNA(szClass, &pszLengthAndClassName[1], cchLength) == 0) // Class name matched?
        {
            // Get the process ID
            DWORD dwProcessId;
            GetWindowThreadProcessId(hWnd, &dwProcessId);
            if (dwProcessId == pEnum->dwProcessId) // Same process?
            {
                pEnum->iFound = iItem; // Found
                return FALSE; // Quit
            }
        }
    }

    return TRUE; // Continue
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

static HRESULT
SHLWAPI_GetModuleVersion(_In_ PCSTR pszFileName, _Out_ PSTR *ppszDest)
{
    DWORD dwHandle;
    BYTE Data[4096];
    PVOID pszA;

    *ppszDest = NULL;

    UINT size = GetFileVersionInfoSizeA(pszFileName, &dwHandle);
    if (size <= _countof(Data) &&
        GetFileVersionInfoA(pszFileName, dwHandle, sizeof(Data), Data) &&
        (VerQueryValueA(Data, PRODUCT_VER_ENGLISH_US_UTF16,     &pszA, &size) ||
         VerQueryValueA(Data, PRODUCT_VER_GERMAN_UTF16,         &pszA, &size) ||
         VerQueryValueA(Data, PRODUCT_VER_ENGLISH_US_WE,        &pszA, &size) ||
         VerQueryValueA(Data, PRODUCT_VER_ENGLISH_US_NEUTRAL,   &pszA, &size) ||
         VerQueryValueA(Data, PRODUCT_VER_SWEDISH_WE,           &pszA, &size)) &&
        size && Str_SetPtrA(ppszDest, (PSTR)pszA))
    {
        // NOTE: You have to LocalFree *ppszDest later
        return S_OK;
    }

    return E_OUTOFMEMORY;
}

static BOOL
SHLWAPI_DoesModuleMatchVersion(_In_ PCSTR pszFileName, _In_opt_ PCSTR pszEntryVersion)
{
    if (!pszEntryVersion)
        return TRUE;

    PSTR pszModuleVersion = NULL;
    HRESULT hr = SHLWAPI_GetModuleVersion(pszFileName, &pszModuleVersion);
    if (FAILED(hr))
        return FALSE;

    BOOL ret = FALSE;
    if (pszEntryVersion[0] == MAJOR_VER_ONLY[0]) // Special handling?
    {
        // Truncate at comma (',') if any
        PSTR pchComma = StrChrA(pszModuleVersion, ',');
        if (pchComma)
            *pchComma = ANSI_NULL;

        // Truncate at dot ('.') if any
        PSTR pchDot = StrChrA(pszModuleVersion, '.');
        if (pchDot)
            *pchDot = ANSI_NULL;

        ret = (lstrcmpiA(pszModuleVersion, &pszEntryVersion[1]) == 0);
    }
    else // Otherwise normal match
    {
        PSTR pchAsterisk = StrChrA(pszEntryVersion, '*'); // Find an asterisk ('*')
        if (pchAsterisk) // Found an asterisk?
        {
            // Check matching with ignoring the trailing substring from '*'
            INT cchPrefix = (INT)(pchAsterisk - pszEntryVersion);
            if (cchPrefix > 0)
                ret = (StrCmpNIA(pszModuleVersion, pszEntryVersion, cchPrefix) == 0);
        }

        if (!ret)
            ret = (lstrcmpiA(pszModuleVersion, pszEntryVersion) == 0); // Whole match?
    }

    LocalFree(pszModuleVersion);
    return ret;
}

static DWORD
SHLWAPI_GetRegistryCompatFlags(_In_ PCSTR pszPath)
{
    // Build the path of the "application compatibility" registry key
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
    szText[0] = ANSI_NULL; // The 1st try is for the non-sub key
    for (DWORD dwIndex = 0; error == ERROR_SUCCESS;)
    {
        // Open the sub-key
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
        if (bValueExists) // "RequiredFile" value exists?
        {
            // Build required file path to szText
            PathCombineA(szText, szBaseDir, szRequired);
            TRACE("RequiredFile: %s\n", wine_dbgstr_a(szRequired));
            TRACE("szText: %s\n", wine_dbgstr_a(szText));
            // Now szText is a full path
            bRequiredFileExists = (GetFileAttributesA(szText) != INVALID_FILE_ATTRIBUTES);
        }

        // The "RequiredFile" value doesn't exist, or the file of szText exists?
        if (!bValueExists || bRequiredFileExists)
        {
            // Check the "Version" value if necessary
            error = SHGetValueA(hSubKey, NULL, "Version", NULL, szText, &cbData);
            PCSTR pszVersionPattern = ((error == ERROR_SUCCESS) ? szText : NULL);
            TRACE("pszVersionPattern: %s\n", wine_dbgstr_a(pszVersionPattern));

            // Does the pattern match?
            if (SHLWAPI_DoesModuleMatchVersion(pszPath, pszVersionPattern))
            {
                // Add additional flags from the registry key
                dwCompatFlags |= SHLWAPI_GetMappedFlags(hSubKey, g_appCompatFlagMaps,
                                                        _countof(g_appCompatFlagMaps));
            }
        }

        // Close sub-key
        RegCloseKey(hSubKey);

        // Go to the next sub-key
        ++dwIndex;
        error = RegEnumKeyA(hKey, dwIndex, szText, _countof(szText));
    }

    // Close the key
    RegCloseKey(hKey);

    return dwCompatFlags;
}

static VOID
SHLWAPI_InitAppCompat(VOID)
{
    if (GetProcessVersion(0) >= 0x50000)
        return; // No need of flags

    // Get module pathname
    CHAR szModulePathA[MAX_PATH];
    if (!GetModuleFileNameA(NULL, szModulePathA, _countof(szModulePathA)))
        return;

    PCSTR pszFileName = PathFindFileNameA(szModulePathA); // Get the file title

    // Search the file title from g_appCompatInfo
    for (UINT iItem = 0; iItem < _countof(g_appCompatInfo); ++iItem)
    {
        const APPCOMPATINFO *pInfo = &g_appCompatInfo[iItem];
        if (lstrcmpiA(pInfo->pszAppName, pszFileName) == 0 &&
            SHLWAPI_DoesModuleMatchVersion(pszFileName, pInfo->pszAppVersion))
        {
            // Found. Set flags
            g_dwAppCompatFlags = g_appCompatInfo[iItem].dwCompatFlags;
            break;
        }
    }

    // Add more flags from registry
    g_dwAppCompatFlags |= SHLWAPI_GetRegistryCompatFlags(pszFileName);
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

    // Initialize and get flags if necessary
    if (!g_bInitAppCompat && (dwMask & SHACF_TO_INIT))
    {
        AppCompat_Lock();
        SHLWAPI_InitAppCompat();
        g_bInitAppCompat = TRUE; // Remember
        AppCompat_Unlock();
    }

    // Get additional flags if necessary
    if (g_dwAppCompatFlags && (dwMask & (SHACF_UNKNOWN1 | SHACF_UNKNOWN2)))
    {
        // Find the target window and flags by using g_wndCompatInfo
        APPCOMPATENUM data =
        {
            g_wndCompatInfo, _countof(g_wndCompatInfo), GetCurrentProcessId(), -1
        };
        EnumWindows(SHLWAPI_WndCompatEnumProc, (LPARAM)&data);

        // Add the target flags if found
        AppCompat_Lock();
        if (data.iFound >= 0)
            g_dwAppCompatFlags |= g_wndCompatInfo[data.iFound].dwCompatFlags;
        g_dwAppCompatFlags |= SHACF_UNKNOWN3;
        AppCompat_Unlock();
    }

    return g_dwAppCompatFlags;
}

// FIXME: SHGetObjectCompatFlags
