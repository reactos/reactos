#include <stdarg.h>
#include <assert.h>

#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winnls.h>
#include <winreg.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shlwapi_undoc.h>
#include <wine/unicode.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

static DWORD g_dwAppCompatFlags = 0;

typedef struct tagFLAGMAP
{
    DWORD flags;
    LPCSTR name;
} FLAGMAP, *PFLAGMAP;

static FLAGMAP g_appCompatFlagMaps[] =
{
    { 0x1, "CONTEXTMENU" },
    { 0x4, "CORELINTERNETENUM" },
    { 0x4, "OLDCREATEVIEWWND" },
    { 0x4, "WIN95DEFVIEW" },
    { 0x2, "DOCOBJECT" },
    { 0x1, "FLUSHNOWAITALWAYS" },
    { 0x8, "MYCOMPUTERFIRST" },
    { 0x10, "OLDREGITEMGDN" },
    { 0x40, "LOADCOLUMNHANDLER" },
    { 0x80, "ANSI" },
    { 0x400, "STAROFFICE5PRINTER" },
    { 0x800, "NOVALIDATEFSIDS" },
    { 0x200, "WIN95SHLEXEC" },
    { 0x1000, "FILEOPENNEEDSEXT" },
    { 0x2000, "WIN95BINDTOOBJECT" },
    { 0x4000, "IGNOREENUMRESET" },
    { 0x10000, "ANSIDISPLAYNAMES" },
    { 0x20000, "FILEOPENBOGUSCTRLID" },
    { 0x40000, "FORCELFNIDLIST" },
};

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

typedef struct tagAPPCOMPATINFO
{
    PCSTR pszAppName;
    PCSTR pszAppVersion;
    DWORD dwCompatFlags;
} APPCOMPATINFO, *PAPPCOMPATINFO;

#define SPECIAL_MARK "\x01"

static APPCOMPATINFO g_appCompatInfo[] =
{
    { "WPWIN7.EXE", NULL, 0x5 },
    { "PRWIN70.EXE", NULL, 0x5 },
    { "PS80.EXE", NULL, 0x15 },
    { "QPW.EXE", SPECIAL_MARK "7", 0x1 },
    { "QFINDER.EXE", NULL, 0x14 },
    { "PFIM80.EXE", NULL, 0x15 },
    { "UA80.EXE", NULL, 0x15 },
    { "PDXWIN32.EXE", NULL, 0x15 },
    { "SITEBUILDER.EXE", NULL, 0x15 },
    { "HOTDOG4.EXE", NULL, 0x2 },
    { "RNAAPP.EXE", NULL, 0x1 },
    { "PDEXPLO.EXE", SPECIAL_MARK "2", 0x9 },
    { "PDEXPLO.EXE", SPECIAL_MARK "1", 0x9 },
    { "PDEXPLO.EXE", SPECIAL_MARK "3", 0x18 },
    { "SIZEMGR.EXE", SPECIAL_MARK "3", 0x14 },
    { "SMARTCTR.EXE", "96.0", 0x1 },
    { "WPWIN8.EXE", NULL, 0x14 },
    { "PRWIN8.EXE", NULL, 0x14 },
    { "UE32.EXE", "2.00.0.0", 0x10 },
    { "PP70.EXE", NULL, 0x40 },
    { "PP80.EXE", NULL, 0x40 },
    { "PS80.EXE", NULL, 0x10 },
    { "ABCMM.EXE", NULL, 0x40 },
    { "QPW.EXE", SPECIAL_MARK "8", 0x10014 },
    { "CORELDRW.EXE", SPECIAL_MARK "7", 10 },
    { "FILLER51.EXE", NULL, 0x10 },
    { "AUTORUN.EXE", "4.10.1998", 0x80 },
    { "AUTORUN.EXE", "4.00.950", 0x80 },
    { "POWERPNT.EXE", SPECIAL_MARK "8", 0x200 },
    { "MSMONEY.EXE", "7.05.1107", 0x200 },
    { "soffice.EXE", SPECIAL_MARK "5", 0x400 },
    { "WPWIN9.EXE", SPECIAL_MARK "9", 0x4 },
    { "QPW.EXE", SPECIAL_MARK "9", 0x4 },
    { "PRWIN9.EXE", SPECIAL_MARK "9", 0x4 },
    { "DAD9.EXE", SPECIAL_MARK "9", 0x4 },
};

typedef struct tagAPPCOMPATINFO2
{
    PCSTR pszLengthAndClassName;
    DWORD dwFlags;
} APPCOMPATINFO2, *PAPPCOMPATINFO2;

static APPCOMPATINFO2 g_appCompatInfo2[] =
{
    /* The first byte is the length of string */
    { "\x09" "bosa_sdm_", 0x1000100 },
    { "\x18" "File Open Message Window", 0x1000100 },
};

typedef struct tagAPPCOMPATENUM
{
    PAPPCOMPATINFO2 pItems;
    UINT nItems;
    DWORD dwProcessId;
    INT iFound;
} APPCOMPATENUM, *PAPPCOMPATENUM;

static BOOL g_bAppCompatInit = FALSE;

static BOOL CALLBACK
SHLWAPI_AppCompatEnumWndProc(_In_ HWND hWnd, _In_ LPARAM lParam)
{
    PAPPCOMPATENUM pEnum = (PAPPCOMPATENUM)lParam;

    CHAR szClass[256];
    if (!pEnum->nItems || !GetClassNameA(hWnd, szClass, _countof(szClass)))
        return TRUE; // Continue

    INT cchClass = lstrlenA(szClass);

    UINT iItem;
    for (iItem = 0; iItem < pEnum->nItems; ++iItem)
    {
        PCSTR pszLengthAndClassName = pEnum->pItems[iItem].pszLengthAndClassName;
        INT cchLength = *pszLengthAndClassName;
        if (cchClass < cchLength)
            cchLength = cchClass;

        DWORD dwProcessId;
        if (!StrCmpNA(szClass, &pszLengthAndClassName[1], cchLength))
        {
            GetWindowThreadProcessId(hWnd, &dwProcessId);
            if (dwProcessId == pEnum->dwProcessId)
            {
                pEnum->iFound = iItem;
                return FALSE; // Quit
            }
        }
    }

    return TRUE; // Continue
}

static HRESULT
SHLWAPI_GetModuleVersionString(_In_ PCSTR pszFileName, _Out_ PSTR *ppszDest)
{
    DWORD dwHandle;
    BYTE Data[4096];
    PVOID pszA;

    *ppszDest = NULL;

    UINT size = GetFileVersionInfoSizeA(pszFileName, &dwHandle);
    if (size <= _countof(Data) &&
        GetFileVersionInfoA(pszFileName, dwHandle, sizeof(Data), Data) &&
        (// English (US) UTF-16
         VerQueryValueA(Data, "\\StringFileInfo\\040904E4\\ProductVersion", &pszA, &size) ||
         // German (Germany) UTF-16
         VerQueryValueA(Data, "\\StringFileInfo\\040704E4\\ProductVersion", &pszA, &size) ||
         // English (US) Western European
         VerQueryValueA(Data, "\\StringFileInfo\\040904B0\\ProductVersion", &pszA, &size) ||
         // English (US) Neutral
         VerQueryValueA(Data, "\\StringFileInfo\\04090000\\ProductVersion", &pszA, &size) ||
         // Swedish (Sweden) Western European
         VerQueryValueA(Data, "\\StringFileInfo\\041D04B0\\ProductVersion", &pszA, &size)) &&
        size && Str_SetPtrA(ppszDest, (PSTR)pszA))
    {
        return S_OK;
    }

    return E_OUTOFMEMORY;
}

static BOOL
SHLWAPI_IsAppCompatVersion(_In_ PCSTR pszFileName, _In_opt_ PCSTR pszStart)
{
    if (!pszStart)
        return TRUE;

    PSTR moduleVersion = NULL;
    if (SHLWAPI_GetModuleVersionString(pszFileName, &moduleVersion) < 0)
        return FALSE;

    BOOL bCompat = FALSE;

    if (*pszStart == SPECIAL_MARK[0]) // Special handling
    {
        PSTR commaPos = StrChrA(moduleVersion, ',');
        if (commaPos)
            *commaPos = ANSI_NULL;

        PSTR dotPos = StrChrA(moduleVersion, '.');
        if (dotPos)
            *dotPos = ANSI_NULL;

        bCompat = (lstrcmpiA(moduleVersion, &pszStart[1]) == 0);
    }
    else
    {
        PSTR asteriskPos = StrChrA(pszStart, '*');
        if (asteriskPos)
        {
            INT prefixLength = asteriskPos - pszStart;
            if (prefixLength > 0)
                bCompat = (StrCmpNIA(moduleVersion, pszStart, prefixLength) == 0);
        }

        if (!bCompat)
            bCompat = (lstrcmpiA(moduleVersion, pszStart) == 0);
    }

    LocalFree(moduleVersion);
    return bCompat;
}

static DWORD
SHLWAPI_GetRegistryCompatFlags(_In_ PCSTR pszPath)
{
    DWORD dwFlags = 0;
    PCSTR pszFileNameA = PathFindFileNameA(pszPath);

    CHAR szText[MAX_PATH];
    wnsprintfA(szText, _countof(szText),
               "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ShellCompatibility\\Applications\\%s",
               pszFileNameA);

    HKEY hKey;
    const REGSAM samDesired = KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS;
    LSTATUS error = RegOpenKeyExA(HKEY_LOCAL_MACHINE, szText, 0, samDesired, &hKey);
    if (!error)
        return dwFlags;

    CHAR szBaseDir[MAX_PATH];
    lstrcpynA(szBaseDir, pszPath, _countof(szBaseDir));
    PathRemoveFileSpecA(szBaseDir);

    szText[0] = ANSI_NULL;
    for (DWORD dwIndex = 0; !error; ++dwIndex)
    {
        HKEY hSubKey;
        error = RegOpenKeyExA(hKey, szText, 0, KEY_QUERY_VALUE, &hSubKey);
        if (error)
            break;

        DWORD cbData = sizeof(szText);
        error = SHGetValueA(hSubKey, NULL, "RequiredFile", NULL, szText, &cbData);
        if (!error)
            PathCombineA(szText, szBaseDir, szText);

        if (error || GetFileAttributesA(szText) != INVALID_FILE_ATTRIBUTES)
        {
            error = SHGetValueA(hSubKey, NULL, "Version", NULL, szText, &cbData);
            if (SHLWAPI_IsAppCompatVersion(pszPath, error ? NULL : szText))
            {
                dwFlags |= SHLWAPI_GetMappedFlags(hSubKey,
                    g_appCompatFlagMaps, _countof(g_appCompatFlagMaps));
            }
        }
        RegCloseKey(hSubKey);

        error = RegEnumKeyA(hKey, dwIndex, szText, _countof(szText));
    }

    RegCloseKey(hKey);
    return dwFlags;
}

static VOID
SHLWAPI_InitAppCompat(VOID)
{
    if (GetProcessVersion(0) >= 0x50000)
        return;

    CHAR szModulePathA[MAX_PATH];
    if (!GetModuleFileNameA(NULL, szModulePathA, _countof(szModulePathA)))
        return;

    PCSTR pszFileName = PathFindFileNameA(szModulePathA);
    for (UINT iItem = 0; iItem < _countof(g_appCompatInfo); ++iItem)
    {
        const APPCOMPATINFO *pInfo = &g_appCompatInfo[iItem];
        if (lstrcmpiA(pInfo->pszAppName, pszFileName) == 0 &&
            SHLWAPI_IsAppCompatVersion(pszFileName, pInfo->pszAppVersion))
        {
            g_dwAppCompatFlags = g_appCompatInfo[iItem].dwCompatFlags;
            break;
        }
    }

    g_dwAppCompatFlags |= SHLWAPI_GetRegistryCompatFlags(pszFileName);
}

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
 * Thanks for Geoff Chappell.
 * @see https://www.geoffchappell.com/studies/windows/shell/shlwapi/api/util/getappcompatflags.htm
 */
DWORD WINAPI
SHGetAppCompatFlags(_In_ DWORD dwMask)
{
    TRACE("(0x%lX)\n", dwMask);

    if ((dwMask & SHACF_TO_INIT) && !g_bAppCompatInit)
    {
        SHLWAPI_InitAppCompat();
        g_bAppCompatInit = TRUE;
    }

    if (g_dwAppCompatFlags && (dwMask & (SHACF_UNKNOWN1 | SHACF_UNKNOWN2)))
    {
        APPCOMPATENUM data;
        data.iFound = -1;
        data.dwProcessId = GetCurrentProcessId();
        data.pItems = g_appCompatInfo2;
        data.nItems = _countof(g_appCompatInfo2);
        EnumWindows(SHLWAPI_AppCompatEnumWndProc, (LPARAM)&data);

        if (data.iFound >= 0)
            g_dwAppCompatFlags |= g_appCompatInfo2[data.iFound].dwFlags;

        g_dwAppCompatFlags |= SHACF_UNKNOWN3;
    }

    return g_dwAppCompatFlags;
}

// FIXME: SHGetObjectCompatFlags
