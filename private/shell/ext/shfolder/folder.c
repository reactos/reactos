#define _SHFOLDER_
#define NO_SHLWAPI_PATH
#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shfolder.h>
#include <platform.h>

#include "resource.h"

#ifdef DBG
#define ASSERT(x) if (!(x)) DebugBreak();
#else
#define ASSERT(x)
#endif

#define ARRAYSIZE(a)                (sizeof(a)/sizeof(a[0]))

// We can't rely on shlwapi SHUnicodeToAnsi/SHAnsiToUnicode in this module
#define SHAnsiToUnicode(psz, pwsz, cchwsz)  MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, psz, -1, pwsz, cchwsz)
#define SHUnicodeToAnsi _SHUnicodeToAnsi

//
// Global array of static system SIDs, corresponding to UI_SystemSid
//
struct
{
    SID sid;                // contains 1 subauthority
    DWORD dwSubAuth[1];     // we currently need at most 2 subauthorities
}
c_StaticSids[] =
{
    {{SID_REVISION, 1, SECURITY_CREATOR_SID_AUTHORITY, {SECURITY_CREATOR_OWNER_RID}},      {0}                             },
    {{SID_REVISION, 1, SECURITY_NT_AUTHORITY,          {SECURITY_AUTHENTICATED_USER_RID}}, {0}                             },
    {{SID_REVISION, 1, SECURITY_NT_AUTHORITY,          {SECURITY_LOCAL_SYSTEM_RID}},       {0}                             },
    {{SID_REVISION, 2, SECURITY_NT_AUTHORITY,          {SECURITY_BUILTIN_DOMAIN_RID}},     {DOMAIN_ALIAS_RID_ADMINS}       },
    {{SID_REVISION, 2, SECURITY_NT_AUTHORITY,          {SECURITY_BUILTIN_DOMAIN_RID}},     {DOMAIN_ALIAS_RID_POWER_USERS}  },
};

#define SSI_CREATOROWNER    0
#define SSI_AUTHUSER        1
#define SSI_SYSTEM          2
#define SSI_ADMIN           3
#define SSI_POWERUSER       4

typedef struct tagACEPARAMLIST
{
    DWORD dwSidIndex;
    DWORD AccessMask;
    DWORD dwAceFlags;
}
ACEPARAMLIST;

#define ACE_INHERIT         (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE)
#define FILE_MODIFY         (FILE_ALL_ACCESS & ~(WRITE_DAC | WRITE_OWNER))

const ACEPARAMLIST c_paplUnsecure[] =
{
    SSI_SYSTEM,         FILE_ALL_ACCESS,    0,
    SSI_SYSTEM,         GENERIC_ALL,        ACE_INHERIT,
    SSI_AUTHUSER,       FILE_MODIFY,        0,
    SSI_AUTHUSER,       FILE_MODIFY,        ACE_INHERIT,
};

//
// CSIDL_COMMON_DOCUMENTS
// Admins, System, Creator Owner: Full Control - Container Inherit, Object Inherit
// Users, Power Users: Read - Container Inherit, Object Inherit
// Users, Power Users: Write - Container Inherit
//
// Non admin users can create files and directories. They have full control over 
// the files they create. All other users can read those files by default, but 
// they cannot modify the files unless the original creator gives them explicit 
// permissions to do so.
//
const ACEPARAMLIST c_paplCommonDocs[] =
{
    SSI_SYSTEM,         FILE_ALL_ACCESS,    0,
    SSI_SYSTEM,         GENERIC_ALL,        ACE_INHERIT,
    SSI_ADMIN,          FILE_ALL_ACCESS,    0,
    SSI_ADMIN,          GENERIC_ALL,        ACE_INHERIT,
    SSI_CREATOROWNER,   GENERIC_ALL,        ACE_INHERIT,
    SSI_AUTHUSER,       FILE_GENERIC_READ,  0,
    SSI_AUTHUSER,       GENERIC_READ,       ACE_INHERIT,
    SSI_AUTHUSER,       FILE_GENERIC_WRITE, 0,
    SSI_AUTHUSER,       GENERIC_WRITE,      (CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE),
    SSI_POWERUSER,      FILE_GENERIC_READ,  0,
    SSI_POWERUSER,      GENERIC_READ,       ACE_INHERIT,
    SSI_POWERUSER,      FILE_GENERIC_WRITE, 0,
    SSI_POWERUSER,      GENERIC_WRITE,      (CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE),
};

//
// CSIDL_COMMON_APPDATA
// Admins, System, Creator Owner: Full Control - Container Inherit, Object Inherit
// Power Users: Modify - Container Inherit, Object Inherit
// Users: Read - Container Inherit, Object Inherit
//
// Users can only read common appdata which is presumably created by admins or 
// power users during setup.
//
const ACEPARAMLIST c_paplCommonAppData[] =
{
    SSI_SYSTEM,         FILE_ALL_ACCESS,    0,
    SSI_SYSTEM,         GENERIC_ALL,        ACE_INHERIT,
    SSI_ADMIN,          FILE_ALL_ACCESS,    0,
    SSI_ADMIN,          GENERIC_ALL,        ACE_INHERIT,
    SSI_CREATOROWNER,   GENERIC_ALL,        ACE_INHERIT,
    SSI_AUTHUSER,       FILE_GENERIC_READ,  0,
    SSI_AUTHUSER,       GENERIC_READ,       ACE_INHERIT,
    SSI_POWERUSER,      FILE_MODIFY,        0,
    SSI_POWERUSER,      FILE_MODIFY,        ACE_INHERIT,
};


long _SHUnicodeToAnsi(LPCWSTR pwsz, LPSTR psz, long cchCount)
{
    psz[0] = 0;
    return WideCharToMultiByte(CP_ACP, 0, pwsz, -1, psz, cchCount, 0, 0);
}


LPWSTR _lstrcpyW(LPWSTR pwszDest, LPCWSTR pwszOrig)
{
    if (pwszDest && pwszOrig)
    {
        do 
        {
            *pwszDest = *pwszOrig;
            pwszDest ++;
        } while ( *pwszOrig++);

        return pwszDest;
    }
    return 0;
}



BOOL _SetDirAccess(LPCWSTR pszFile, const ACEPARAMLIST* papl, ULONG cPapl);

HINSTANCE g_hinst = NULL;

typedef void (__stdcall * PFNSHFLUSHSFCACHE)();

BOOL IsNewShlwapi(HMODULE hmod)
{
    DLLGETVERSIONPROC pfnGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hmod, "DllGetVersion");
    if (pfnGetVersion)
    {
        DLLVERSIONINFO dllinfo;
        dllinfo.cbSize = sizeof(DLLVERSIONINFO);
        if (pfnGetVersion(&dllinfo) == NOERROR)
        {
            return  (dllinfo.dwMajorVersion > 5) ||
                    ((dllinfo.dwMajorVersion == 5) &&
                     ((dllinfo.dwMinorVersion > 0) ||
                      ((dllinfo.dwMinorVersion == 0) &&
                       (dllinfo.dwBuildNumber > 2012))));
        }
    }
    return 0;
}

void FlushShellFolderCache()
{
    // We could link directly now, but this is a smaller delta...
    HMODULE hmod = LoadLibraryA("shlwapi.dll");
    if (hmod) 
    {
        // avoid IE5 beta1 shlwapi.dll that has an export here but
        // not what we expect
        if (IsNewShlwapi(hmod))
        {
            PFNSHFLUSHSFCACHE pfn = (PFNSHFLUSHSFCACHE)GetProcAddress(hmod, (CHAR *) MAKEINTRESOURCE(419));
            if (pfn) 
                pfn();
        }
        FreeLibrary(hmod);
    }
}

// typedef HRESULT (__stdcall * PFNSHGETFOLDERPATH)(HWND, int, HANDLE, DWORD, LPWSTR);

HRESULT _SHGetFolderPath(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPWSTR pszPath)
{
    HRESULT hres = E_NOTIMPL;
    HMODULE hmod = LoadLibraryA("shell32.dll");
    if (hmod) 
    {
        PFNSHGETFOLDERPATHW pfn = (PFNSHGETFOLDERPATHW)GetProcAddress(hmod, "SHGetFolderPathW");
        if (pfn) 
            hres = pfn(hwnd, csidl, hToken, dwFlags, pszPath);
        FreeLibrary(hmod);
    }
    return hres;
}


BOOL RunningOnNT()
{
    static BOOL s_fRunningOnNT = 42;
    if (s_fRunningOnNT == 42)
    {
        OSVERSIONINFO osvi;

        osvi.dwOSVersionInfoSize = sizeof(osvi);
        GetVersionEx(&osvi);
        s_fRunningOnNT = (VER_PLATFORM_WIN32_NT == osvi.dwPlatformId);
    }
    return s_fRunningOnNT;
}


// shell32.SHGetSpecialFolderPath (175)
// undocumented API, but the only one that exists on all platforms
//
// this thunk deals with the A/W issues based on the platform as
// the export was TCHAR
//      

typedef BOOL(__stdcall * PFNSHGETSPECIALFOLDERPATH)(HWND hwnd, LPWSTR pszPath, int csidl, BOOL fCreate);

BOOL _SHGetSpecialFolderPath(HWND hwnd, LPWSTR pszPath, int csidl, BOOL fCreate)
{
    BOOL bRet = FALSE;
    HMODULE hmod = LoadLibraryA("shell32.dll");
    if (hmod) 
    {
        PFNSHGETSPECIALFOLDERPATH pfn = (PFNSHGETSPECIALFOLDERPATH)GetProcAddress(hmod, (CHAR*) MAKEINTRESOURCE(175));
        if (pfn)
        {
            if (RunningOnNT())         // compute from Get
            {
                bRet = pfn(hwnd, pszPath, csidl, fCreate);
            }
            else
            {
                CHAR szPath[MAX_PATH];
                szPath[0] = 0;
                bRet = pfn(hwnd, (LPWSTR)szPath, csidl, fCreate);
                if (bRet)
                    SHAnsiToUnicode(szPath, pszPath, MAX_PATH);      // WideCharToMultiByte wrapper
            }
        }
        FreeLibrary(hmod);
    }
    return bRet;
}

BOOL GetProgramFiles(LPCWSTR pszValue, LPWSTR pszPath)
{
    HKEY hkey;
    DWORD cbPath = MAX_PATH;

    *pszPath = 0;
    if (ERROR_SUCCESS == RegOpenKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion", &hkey)) 
    {
        if (RunningOnNT()) 
        {
            cbPath *= sizeof(WCHAR);
            RegQueryValueExW(hkey, pszValue, NULL, NULL, (LPBYTE) pszPath, &cbPath);
        }
        else 
        {
            CHAR szPath[MAX_PATH], szValue[64];
            szPath[0] = 0;
            _SHUnicodeToAnsi(pszValue, szValue, ARRAYSIZE(szValue));
            RegQueryValueExA(hkey, szValue, NULL, NULL, szPath, &cbPath);
            SHAnsiToUnicode(szPath, pszPath, MAX_PATH);
        }
        RegCloseKey(hkey);
    }
    return (BOOL)*pszPath;
}


// get the equiv of %USERPROFILE% on both win95 and NT
//
// on Win95 without user profiles turned on this will fail
// out:
//      phkey   optional out param
//
// returns:
//      length of the profile path

UINT GetProfilePath(LPWSTR pszPath, HKEY *phkey, UINT *pcchProfile)
{
    
    if (phkey)
        *phkey = NULL;

    if (pcchProfile)
        *pcchProfile = 0;

    if (RunningOnNT()) 
    {
        ExpandEnvironmentStringsW(L"%USERPROFILE%", pszPath, MAX_PATH);
        if (pszPath[0] == L'%')
            pszPath[0] = 0;
    }
    else 
    {
        HKEY hkeyProfRec;
        LONG err;
        CHAR szProfileDir[MAX_PATH];
        szProfileDir [0] = 0;
        err = RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\ProfileReconciliation", 0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                                  NULL, &hkeyProfRec, NULL);
        if (err == ERROR_SUCCESS) 
        {
            DWORD cbData = sizeof(szProfileDir);
            RegQueryValueExA(hkeyProfRec, "ProfileDirectory", 0, NULL, (LPBYTE)szProfileDir, &cbData);
            if (phkey)
                *phkey = hkeyProfRec;
            else
                RegCloseKey(hkeyProfRec);
            if (pcchProfile)
                *pcchProfile = lstrlenA(szProfileDir);
            SHAnsiToUnicode(szProfileDir, pszPath, MAX_PATH);
        }
    }

    return lstrlenW(pszPath);
}

void SHGetWindowsDirectory(LPWSTR pszPath)
{
    if (RunningOnNT())
        GetWindowsDirectoryW(pszPath, MAX_PATH);
    else 
    {
        CHAR szPath[MAX_PATH];
        if (GetWindowsDirectoryA(szPath, ARRAYSIZE(szPath)-1))
            SHAnsiToUnicode(szPath, pszPath, MAX_PATH);
    }
}

#ifndef UNIX
#define CH_WHACK FILENAME_SEPARATOR_W
#else
#define CH_WHACK FILENAME_SEPARATOR
#endif

// add a backslash to a qualified path
//
// in:
//  pszPath    path (A:, C:\foo, etc)
//
// out:
//  pszPath    A:\, C:\foo\    ;
//
// returns:
//  pointer to the NULL that terminates the path


STDAPI_(LPWSTR) PathAddBackslash(LPWSTR pszPath)
{
    LPWSTR pszEnd;

    // try to keep us from tromping over MAX_PATH in size.
    // if we find these cases, return NULL.  Note: We need to
    // check those places that call us to handle their GP fault
    // if they try to use the NULL!

    int ichPath = lstrlenW(pszPath);
    if (ichPath >= (MAX_PATH - 1))
        return NULL;

    pszEnd = pszPath + ichPath;

    // this is really an error, caller shouldn't pass
    // an empty string
    if (!*pszPath)
        return pszEnd;

    /* Get the end of the source directory
    */
    switch(* (pszEnd-1)) {
    case CH_WHACK:
        break;

    default:
        *pszEnd++ = CH_WHACK;
        *pszEnd = 0;
    }
    return pszEnd;
}

// Returns a pointer to the last component of a path string.
//
// in:
//      path name, either fully qualified or not
//
// returns:
//      pointer into the path where the path is.  if none is found
//      returns a poiter to the start of the path
//
//  c:\foo\bar  -> bar
//  c:\foo      -> foo
//  c:\foo\     -> c:\foo\      (REVIEW: is this case busted?)
//  c:\         -> c:\          (REVIEW: this case is strange)
//  c:          -> c:
//  foo         -> foo

STDAPI_(LPWSTR) PathFindFileName(LPCWSTR pPath)
{
    LPCWSTR pT;

    for (pT = pPath; *pPath; ++pPath) 
    {
        if ((pPath[0] == L'\\' || pPath[0] == L':' || pPath[0] == L'/')
            && pPath[1] &&  pPath[1] != L'\\'  &&   pPath[1] != L'/')
            pT = pPath + 1;
    }
    return (LPWSTR)pT;   // const -> non const
}

STDAPI_(LPWSTR) PathFindSecondFileName(LPCWSTR pPath)
{
    LPCWSTR pT, pRet = NULL;
    
    for (pT = pPath; *pPath; ++pPath) 
    {
        if ((pPath[0] == L'\\' || pPath[0] == L':' || pPath[0] == L'/')
            && pPath[1] &&  pPath[1] != L'\\'  &&   pPath[1] != L'/')
        {
            pRet = pT;    // remember last
            
            pT = pPath + 1;
        }
    }
    return (LPWSTR)pRet;   // const -> non const
}


// This function is modified in that if the string's length is 0, the null terminator is NOT copied to the buffer.

int _LoadStringExW(
    UINT      wID,
    LPWSTR    lpBuffer,            // Unicode buffer
    int       cchBufferMax,        // cch in Unicode buffer
    WORD      wLangId)
{
    HRSRC hResInfo;
    HANDLE hStringSeg;
    LPWSTR lpsz;
    int    cch;

    cch = 0;

    // String Tables are broken up into 16 string segments.  Find the segment
    // containing the string we are interested in.
    if (hResInfo = FindResourceExW(g_hinst, (LPCWSTR)RT_STRING,
                                   (LPWSTR)((LONG)(((USHORT)wID >> 4) + 1)), wLangId))
    {
        // Load that segment.
        hStringSeg = LoadResource(g_hinst, hResInfo);

        // Lock the resource.
        if (lpsz = (LPWSTR)LockResource(hStringSeg))
        {
            // Move past the other strings in this segment.
            // (16 strings in a segment -> & 0x0F)
            wID &= 0x0F;
            while (TRUE)
            {
                cch = *((WORD *)lpsz++);   // PASCAL like string count
                                            // first UTCHAR is count if TCHARs
                if (wID-- == 0) break;
                lpsz += cch;                // Step to start if next string
             }


            // Account for the NULL
            cchBufferMax--;

            // Don't copy more than the max allowed.
            if (cch > cchBufferMax)
                cch = cchBufferMax;

            // Copy the string into the buffer.
            CopyMemory(lpBuffer, lpsz, cch*sizeof(WCHAR));


            // Attach Null terminator.
            lpBuffer[cch] = 0;
        }
    }
    return cch;
}

BOOL CALLBACK EnumResLangProc(HINSTANCE hinst, LPCWSTR lpszType, LPCWSTR lpszName, LANGID wLangId, LPARAM lParam)
{
    *(LANGID *)lParam = wLangId;
    return FALSE;
}

BOOL CALLBACK EnumResNameProc(HINSTANCE hinst, LPCWSTR lpszType, LPCWSTR lpszName, LPARAM lParam)
{
    EnumResourceLanguagesW(hinst, lpszType, lpszName, EnumResLangProc, lParam);
    return FALSE;
}

LANGID GetShellLangId()
{
    static LANGID wShellLangID=0xffff;
    if (0xffff == wShellLangID) 
    {
        BOOL fSuccess;
        HINSTANCE hShell;
        hShell = LoadLibraryA("shell32.dll");
        if (hShell)
        {
            EnumResourceNamesW(hShell,  (LPWSTR) RT_VERSION, EnumResNameProc, (LPARAM) &wShellLangID);
            FreeLibrary(hShell);
        }
        if (0xffff == wShellLangID)
            wShellLangID = GetSystemDefaultLangID();
    }
    return wShellLangID;
}


void PathAppendResource(LPWSTR pszPath, UINT id)
{
    WCHAR sz[MAX_PATH];
    sz[0] = 0;

    if (!_LoadStringExW(id, sz, ARRAYSIZE(sz), GetShellLangId()))
        _LoadStringExW(id, sz, ARRAYSIZE(sz), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
    if (*sz && ((lstrlenW(pszPath) + lstrlenW(sz)) < MAX_PATH))
        _lstrcpyW(PathAddBackslash(pszPath),sz);
}

void PathAppend(LPWSTR pszPath, LPCWSTR pszAppend)
{
    if (((lstrlenW(pszPath) + lstrlenW(pszAppend)) < MAX_PATH))
        _lstrcpyW(PathAddBackslash(pszPath), pszAppend);
}

const CHAR c_szUSF[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders";
const CHAR c_szSF[]  = "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders";

LONG RegSetStrW(HKEY hkey, LPCWSTR pszValueName, LPCWSTR pszValue)
{
    return RegSetValueExW(hkey, pszValueName, 0, REG_SZ, (LPBYTE)pszValue, (lstrlenW(pszValue) + 1) * sizeof(WCHAR));
}

LONG RegSetStrA(HKEY hkey, LPCSTR pszValueName, LPCSTR pszValue)
{
    return RegSetValueExA(hkey, pszValueName, 0, REG_SZ, (LPBYTE)pszValue, (lstrlenA(pszValue) + 1) * sizeof(CHAR));
}

#ifdef UNIX
LONG RegSetExpandStr(HKEY hkey, LPCTSTR pszValueName, LPCTSTR pszValue)
{
    return RegSetValueEx(hkey, pszValueName, 0, REG_EXPAND_SZ, (LPBYTE)pszValue, (lstrlen(pszValue) + 1) * sizeof(TCHAR));
}
#endif /* UNIX */


void MakeFolderRoam(HKEY hkeyProfRec, LPCSTR pszName, LPCWSTR pszPath, UINT cchProfile)
{
    HKEY hSubKey;
    LONG err;
    CHAR szPath[MAX_PATH];


    ASSERT(!RunningOnNT());

    _SHUnicodeToAnsi(pszPath, szPath, MAX_PATH);

    err = RegCreateKeyExA(hkeyProfRec, pszName, 0, NULL, REG_OPTION_NON_VOLATILE,
                              KEY_WRITE, NULL, &hSubKey, NULL);
    if (err == ERROR_SUCCESS)
    {
        CHAR szDefaultPath[MAX_PATH];
        DWORD dwOne = 1;
        LPCSTR pszEnd = szPath + cchProfile + 1;

        szDefaultPath[0] = 0;
        lstrcpyA(szDefaultPath, "*windir");
        lstrcatA(szDefaultPath, szPath + cchProfile);

        RegSetStrA(hSubKey, "CentralFile", pszEnd);
        RegSetStrA(hSubKey, "LocalFile",   pszEnd);
        RegSetStrA(hSubKey, "Name",        "*.*");
        RegSetStrA(hSubKey, "DefaultDir",  szDefaultPath);

        RegSetValueExA(hSubKey, "MustBeRelative", 0, REG_DWORD, (LPBYTE)&dwOne, sizeof(dwOne));
        RegSetValueExA(hSubKey, "Default",        0, REG_DWORD, (LPBYTE)&dwOne, sizeof(dwOne));

        RegSetStrA(hSubKey, "RegKey",   c_szUSF);
        RegSetStrA(hSubKey, "RegValue", pszName);

        RegCloseKey(hSubKey);
    }
}

typedef struct _FOLDER_INFO
{
    int id;                 // CSIDL value
    HKEY hkRoot;            // per user, per machine
    UINT idsDirName;        // esource ID for directory name 
    LPCSTR pszRegValue;    // Name of reg value and ProfileReconciliation subkey
    BOOL (*pfnGetPath)(const struct _FOLDER_INFO *, LPWSTR);  // compute the path if not found
    const ACEPARAMLIST* papl;
    ULONG cApl;
}
FOLDER_INFO;

typedef struct _NT_FOLDER_INFO
{
    const FOLDER_INFO *pfi; 
    WCHAR wszRegValue[60]; // this should be long enough to hold the longest member of FOLDER_INFO.pszRegValue
}
NT_FOLDER_INFO;

BOOL DownLevelRoaming(const FOLDER_INFO *pfi, LPWSTR pszPath)
{
    HKEY hkeyProfRec;
    UINT cchProfile;
    UINT cwchProfile = GetProfilePath(pszPath, &hkeyProfRec, &cchProfile);
    if (cwchProfile)
    {
        PathAppendResource(pszPath, pfi->idsDirName);
        if (hkeyProfRec)
        {
            MakeFolderRoam(hkeyProfRec, pfi->pszRegValue, pszPath, cchProfile);
            RegCloseKey(hkeyProfRec);
        }
    }
    else
    {
        SHGetWindowsDirectory(pszPath);
        if (pfi->id == CSIDL_PERSONAL)
        {
            if (pszPath[1] == TEXT(':') &&
                pszPath[2] == TEXT('\\'))
            {
                pszPath[3] = 0; // strip to "C:\"
            }
        }
        PathAppendResource(pszPath, pfi->idsDirName);
    }

    return (BOOL)*pszPath;
}

BOOL DownLevelNonRoaming(const FOLDER_INFO *pfi, LPWSTR pszPath)
{
    UINT cchProfile = GetProfilePath(pszPath, NULL, 0);
    if (cchProfile)
    {
        PathAppendResource(pszPath, pfi->idsDirName);
    }
    else
    {
        SHGetWindowsDirectory(pszPath);
        PathAppendResource(pszPath, pfi->idsDirName);
    }

    return (BOOL)*pszPath;
}

BOOL DownLevelRelative(UINT csidl, UINT id, LPWSTR pszPath)
{
    *pszPath = 0;   // assume error

    // since this is inside MyDocs make sure MyDocs exists first (for the create call)
    if (SHGetFolderPathW(NULL, csidl | CSIDL_FLAG_CREATE, NULL, 0, pszPath) == S_OK)
    {
        PathAppendResource(pszPath, id);
    }
    return (BOOL)*pszPath;
}

// we explictly don't want the MyPics folder to roam. the reasonaing being
// that the contents of this are typically too large to give a good roaming
// experience. but of course NT4 (< SP4) still roams everyting in the profile
// dir thus this will roam on those platforms.

BOOL DownLevelMyPictures(const FOLDER_INFO *pfi, LPWSTR pszPath)
{
    return DownLevelRelative(CSIDL_PERSONAL, IDS_CSIDL_MYPICTURES, pszPath);
}

BOOL DownLevelAdminTools(const FOLDER_INFO *pfi, LPWSTR pszPath)
{
    return DownLevelRelative(CSIDL_PROGRAMS, IDS_CSIDL_ADMINTOOLS, pszPath);
}

BOOL DownLevelCommonAdminTools(const FOLDER_INFO *pfi, LPWSTR pszPath)
{
    return DownLevelRelative(CSIDL_COMMON_PROGRAMS, IDS_CSIDL_ADMINTOOLS, pszPath);
}

const WCHAR c_wszAllUsers[] = L"All Users"; // not localized

BOOL GetAllUsersRoot(LPWSTR pszPath)
{
    if (GetProfilePath(pszPath, NULL, 0))
    {
        // yes, non localized "All Users" per ericflo (NT4 behavior)

        if (lstrlenW(pszPath) + ARRAYSIZE(c_wszAllUsers) < MAX_PATH)
            _lstrcpyW(PathFindFileName(pszPath), c_wszAllUsers);
    }
    else
    {
        // Win95 case
        SHGetWindowsDirectory(pszPath);
        // yes, non localized "All Users" per ericflo (NT4 behavior)
        _lstrcpyW(PathAddBackslash(pszPath), c_wszAllUsers);
    }
    return *pszPath;
}

BOOL DownLevelCommon(const FOLDER_INFO *pfi, LPWSTR pszPath)
{
    if (GetAllUsersRoot(pszPath))
    {
        PathAppendResource(pszPath, pfi->idsDirName);
    }
    return (BOOL)*pszPath;
}

BOOL DownLevelCommonPrograms(const FOLDER_INFO *pfi, LPWSTR pszPath)
{
    WCHAR szPath[MAX_PATH];

    if (S_OK == SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, 0, szPath))
    {
        if (GetAllUsersRoot(pszPath))
        {
            PathAppend(pszPath, PathFindSecondFileName(szPath));
        }
    }
    return (BOOL)*pszPath;
}


#define HKLM    HKEY_LOCAL_MACHINE
#define HKCU    HKEY_CURRENT_USER

const FOLDER_INFO c_rgFolders[] =
{
    { CSIDL_PERSONAL,           HKCU, IDS_CSIDL_PERSONAL,         "Personal",
            DownLevelRoaming,           NULL,                0 },
    { CSIDL_MYPICTURES,         HKCU, IDS_CSIDL_MYPICTURES,       "My Pictures",
            DownLevelMyPictures,        NULL,                0 },
    { CSIDL_APPDATA,            HKCU, IDS_CSIDL_APPDATA,          "AppData",
            DownLevelRoaming,           NULL,                0 },
    { CSIDL_LOCAL_APPDATA,      HKCU, IDS_CSIDL_LOCAL_APPDATA,    "Local AppData",
            DownLevelNonRoaming,        NULL,                0 },
    { CSIDL_INTERNET_CACHE,     HKCU, IDS_CSIDL_CACHE,            "Cache",
            DownLevelNonRoaming,        NULL,                0 },
    { CSIDL_COOKIES,            HKCU, IDS_CSIDL_COOKIES,          "Cookies",
            DownLevelRoaming,           NULL,                0 },
    { CSIDL_HISTORY,            HKCU, IDS_CSIDL_HISTORY,          "History",
            DownLevelRoaming,           NULL,                0 },
    { CSIDL_ADMINTOOLS,         HKCU, IDS_CSIDL_ADMINTOOLS,       "Administrative Tools",
            DownLevelAdminTools,        NULL,                0 },
    { CSIDL_COMMON_APPDATA,     HKLM, IDS_CSIDL_APPDATA,          "Common AppData",
            DownLevelCommon,            c_paplCommonAppData, ARRAYSIZE(c_paplCommonAppData) },
    { CSIDL_COMMON_DOCUMENTS,   HKLM, IDS_CSIDL_COMMON_DOCUMENTS, "Common Documents",
            DownLevelCommon,            c_paplCommonDocs,    ARRAYSIZE(c_paplCommonDocs) },
    { CSIDL_COMMON_PROGRAMS,    HKLM, 0,                          "Common Programs",
            DownLevelCommonPrograms,    c_paplUnsecure,      ARRAYSIZE(c_paplUnsecure) },
    { CSIDL_COMMON_ADMINTOOLS,  HKLM, IDS_CSIDL_ADMINTOOLS,       "Common Administrative Tools",
            DownLevelCommonAdminTools,  c_paplUnsecure,      ARRAYSIZE(c_paplUnsecure) },

    { -1, HKCU, 0, NULL, NULL, NULL }
};

BOOL UnExpandEnvironmentString(LPCWSTR pszPath, LPCWSTR pszEnvVar, LPWSTR pszResult, UINT cbResult)
{
    DWORD nToCmp;
    WCHAR szEnvVar[MAX_PATH];
    szEnvVar[0] = 0;
    ASSERT(RunningOnNT());
    ExpandEnvironmentStringsW(pszEnvVar, szEnvVar, ARRAYSIZE(szEnvVar)); // don't count the NULL
    nToCmp = lstrlenW(szEnvVar);
   
    if (CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, szEnvVar, nToCmp, pszPath, nToCmp) == 2) 
    {
        if (lstrlenW(pszPath) - (int)nToCmp  + lstrlenW(pszEnvVar) < (int)cbResult)
        {
            _lstrcpyW(pszResult, pszEnvVar);
            _lstrcpyW(pszResult + lstrlenW(pszResult), pszPath + nToCmp);
            return TRUE;
        }
    }
    return FALSE;
}

const FOLDER_INFO *FindFolderInfo(int csidl)
{
    const FOLDER_INFO *pfi;
    for (pfi = c_rgFolders; pfi->id != -1; pfi++)
    {
        if (pfi->id == csidl) 
            return pfi;
    }
    return NULL;
}

BOOL _SHCreateDirectory(LPCWSTR pszPath) 
{
    if (RunningOnNT())
        return CreateDirectoryW(pszPath, NULL);
    else 
    {
        // no check for Unicode -> Ansi needed here, because we validated 
        // the path in _EnsureExistsOrCreate()
        CHAR szPath[MAX_PATH];
        _SHUnicodeToAnsi(pszPath, szPath, ARRAYSIZE(szPath));
        return CreateDirectoryA(szPath, NULL);
    }
}

BOOL _CreateDirectoryDeep(LPCWSTR pszPath)
{
    BOOL bRet = _SHCreateDirectory(pszPath);
    if (!bRet && (lstrlenW(pszPath) < MAX_PATH))
    {
        WCHAR *pEnd, *pSlash, szTemp[MAX_PATH + 1];  // +1 for PathAddBackslash()

        // There are certain error codes that we should bail out here
        // before going through and walking up the tree...
        switch (GetLastError())
        {
        case ERROR_FILENAME_EXCED_RANGE:
        case ERROR_FILE_EXISTS:
            return FALSE;
        }

        _lstrcpyW(szTemp, pszPath);
        pEnd = PathAddBackslash(szTemp); // for the loop below

        // assume we have 'X:\' to start this should even work
        // on UNC names because will will ignore the first error

        pSlash = szTemp + 3;

        // create each part of the dir in order

        while (*pSlash) 
        {
            while (*pSlash && *pSlash != CH_WHACK)
                pSlash ++;

            if (*pSlash) 
            {
                *pSlash = 0;    // terminate path at seperator
                bRet = _SHCreateDirectory(szTemp);
            }
            *pSlash++ = CH_WHACK;     // put the seperator back
        }
    }
    return bRet;
}

// check for
//      X:\foo
//      \\foo

BOOL PathIsFullyQualified(LPCWSTR pszPath)
{
#ifndef UNIX
    return pszPath[0] && pszPath[1] && 
        (pszPath[1] == ':' || (pszPath[0] == '\\' && pszPath[1] == '\\'));
#else
    /* On Unix, an absolute path starts with a / i.e. from root
     * FILENAME_SEPARATOR is / from platform.h
     */
    return pszPath[0] && pszPath[0] == CH_WHACK;
#endif /* UNIX */
}

HRESULT GetPathFromRegOrDefault(const NT_FOLDER_INFO *npfi, LPWSTR pszPath)
{
    HRESULT hres;
    HKEY hkeyUserShellFolders;
    LONG err;
    CHAR szPath[MAX_PATH];
    const FOLDER_INFO *pfi = npfi->pfi;

    szPath[0] = 0;

    err = RegCreateKeyExA(pfi->hkRoot, c_szUSF, 0, NULL, REG_OPTION_NON_VOLATILE,
                    KEY_READ | KEY_WRITE, NULL, &hkeyUserShellFolders, NULL);

    if (err == ERROR_SUCCESS)
    {
        DWORD dwType, cbData = MAX_PATH * sizeof(*pszPath);
        if (RunningOnNT()) 
        {
            err = RegQueryValueExW(hkeyUserShellFolders, npfi->wszRegValue, NULL, &dwType, (LPBYTE)pszPath, &cbData);
        }
        else
        {
            err = RegQueryValueExA(hkeyUserShellFolders, pfi->pszRegValue, NULL, &dwType, (LPBYTE)szPath, &cbData);
            SHAnsiToUnicode(szPath, pszPath, MAX_PATH);
        }

        if (err == ERROR_SUCCESS && cbData)
        {
            if (dwType == REG_EXPAND_SZ)
            {
                if (RunningOnNT()) 
                {
                    WCHAR szExpand[MAX_PATH];
                    szExpand[0] = 0;
                    if (ExpandEnvironmentStringsW(pszPath, szExpand, ARRAYSIZE(szExpand)))
                        lstrcpynW(pszPath, szExpand, MAX_PATH);
                }
                else
                {
                    CHAR szExpand[MAX_PATH];
                    szExpand[0] = 0;
                    ExpandEnvironmentStringsA(szPath, szExpand, ARRAYSIZE(szExpand));   
                    SHAnsiToUnicode(szExpand,  pszPath, MAX_PATH);
                }
            }

        }
        else if (pfi->pfnGetPath && pfi->pfnGetPath(pfi, pszPath))
        {
            err = ERROR_SUCCESS;

            // store results back to "User Shell Folders" on NT, but not on Win95

            if (RunningOnNT())
            {
                WCHAR szDefaultPath[MAX_PATH];
                szDefaultPath[0] = 0;

                if (!UnExpandEnvironmentString(pszPath, L"%USERPROFILE%", szDefaultPath, ARRAYSIZE(szDefaultPath)))
                {
                    if (!UnExpandEnvironmentString(pszPath, L"%SYSTEMROOT%", szDefaultPath, ARRAYSIZE(szDefaultPath)))
                    {
                        _lstrcpyW(szDefaultPath, pszPath);
                    }
                }
                RegSetValueExW(hkeyUserShellFolders, npfi->wszRegValue, 0, REG_EXPAND_SZ, (LPBYTE)szDefaultPath, (lstrlenW(szDefaultPath) + 1) * sizeof(szDefaultPath[0]));
            }
        }
        else
            err = ERROR_PATH_NOT_FOUND;

        // validate the returned path here
        if (err == ERROR_SUCCESS)
        {
            // expand failed (or some app messed up and did not use REG_EXPAND_SZ)
            if (*pszPath == L'%')
            {
                err = ERROR_ENVVAR_NOT_FOUND;
                *pszPath = 0;
            }
            else if (!PathIsFullyQualified(pszPath))
            {
                err = ERROR_PATH_NOT_FOUND;
                *pszPath = 0;
            }
        }

        RegCloseKey(hkeyUserShellFolders);
    }
    return HRESULT_FROM_WIN32(err);
}

HRESULT _EnsureExistsOrCreate(LPWSTR pszPath, BOOL bCreate, const ACEPARAMLIST* papl, ULONG cApl)
{
    HRESULT hres;
    DWORD dwFileAttributes;


    if (RunningOnNT()) 
        dwFileAttributes = GetFileAttributesW(pszPath);
    else 
    {
        CHAR szPath[MAX_PATH];
        if (_SHUnicodeToAnsi(pszPath, szPath, ARRAYSIZE(szPath)))
            dwFileAttributes = GetFileAttributesA(szPath);
        else
        {
            pszPath[0] = 0;
            return HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);
        }
    }

        
    if (dwFileAttributes == -1)
    {
        if (bCreate)
        {
            if (_CreateDirectoryDeep(pszPath))
            {
                hres = S_OK;
                if (papl && RunningOnNT())
                {
                   _SetDirAccess(pszPath, papl, cApl);
                }
            }
            else
            {
                hres = HRESULT_FROM_WIN32(GetLastError());
                *pszPath = 0;
            }
        }
        else
        {
            hres = S_FALSE;
            *pszPath = 0;
        }
    }
    else
        hres = S_OK;

    return hres;
}

HRESULT _DownLevelGetFolderPath(int csidl, LPWSTR pszPath, BOOL bCreate)
{
    const FOLDER_INFO *pfi;
    HRESULT hres = E_INVALIDARG;
    
    *pszPath = 0;   // assume error
    
    pfi = FindFolderInfo(csidl);
    if (pfi)
    {
        NT_FOLDER_INFO nfi;
        nfi.pfi = pfi;
        SHAnsiToUnicode(pfi->pszRegValue, nfi.wszRegValue, ARRAYSIZE(nfi.wszRegValue));
        // get default value from "User Shell Folders"
        
        hres = GetPathFromRegOrDefault(&nfi, pszPath);
        if (SUCCEEDED(hres))
        {
            hres = _EnsureExistsOrCreate(pszPath, bCreate, pfi->papl, pfi->cApl);
            if (hres == S_OK)
            {
                HKEY hkeyShellFolders;
                LONG err;
                
                // store to "Shell Folders"
                err = RegCreateKeyExA(pfi->hkRoot, c_szSF, 0, NULL, REG_OPTION_NON_VOLATILE,
                    KEY_READ | KEY_WRITE, NULL, &hkeyShellFolders, NULL);

                if (err == ERROR_SUCCESS)
                {
#ifdef UNIX
                    TCHAR szEnvPath[MAX_PATH];
                    
                    /* On Unix, we want to preserve environment variables in
                    * the paths, so that if $HOME changes, but preserving
                    * the directory structure under it, caches do not have
                    * to be re-created
                    */
                    
                    if (UnExpandEnvironmentString(pszPath,
                        TEXT("%USERPROFILE%"),
                        szEnvPath,
                        ARRAYSIZE(szEnvPath)))
                    {
                        RegSetExpandStr(hkeyShellFolders, pfi->pszRegValue, szEnvPath);
                    }
                    else
#endif /* UNIX */
                    if (RunningOnNT())  
                    {
                        RegSetStrW(hkeyShellFolders, nfi.wszRegValue, pszPath);
                    }
                    else 
                    {
                        CHAR szPath[MAX_PATH]; 
                        _SHUnicodeToAnsi(pszPath, szPath, ARRAYSIZE(szPath));
                        RegSetStrA(hkeyShellFolders, pfi->pszRegValue, szPath);
                    }
                    RegCloseKey(hkeyShellFolders);
                }
                
                FlushShellFolderCache();
            }
        }
    }
    else
    {
        if (csidl == CSIDL_WINDOWS)
        {
            SHGetWindowsDirectory(pszPath);
            hres = S_OK;
        }
        else if (csidl == CSIDL_SYSTEM)
        {
            if (RunningOnNT())
                GetSystemDirectoryW(pszPath, MAX_PATH);
            else {
                CHAR szPath[MAX_PATH];
                szPath[0] = 0;
                GetSystemDirectoryA(szPath, MAX_PATH);
                SHAnsiToUnicode(szPath, pszPath, MAX_PATH);
            }
            hres = S_OK;
        }
        else if (csidl == CSIDL_PROGRAM_FILES)
        {
            hres = GetProgramFiles(L"ProgramFilesDir", pszPath) ? S_OK : S_FALSE;
        }
        else if (csidl == CSIDL_PROGRAM_FILES_COMMON)
        {
            hres = GetProgramFiles(L"CommonFilesDir", pszPath) ? S_OK : S_FALSE;
        }
    }
    return hres;
}

// We pass csidl to _SHGetSpecialFolderPath only for NT 4 English folders
// NT bug # 60970
// NT bug # 222510
// NT bug # 221492

STDAPI SHGetFolderPathW(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPWSTR pszPath)
{
    HRESULT hres;

    if (IsBadWritePtr(pszPath, MAX_PATH * sizeof(WCHAR)))
        return E_INVALIDARG;

    pszPath[0] = 0;
    hres = _SHGetFolderPath(hwnd, csidl, hToken, dwFlags, pszPath);
    if (hres == E_NOTIMPL || hres == E_INVALIDARG)
    {
        BOOL bCreate = csidl & CSIDL_FLAG_CREATE;
        csidl &= ~CSIDL_FLAG_MASK;    // strip the flags

        if (hToken || dwFlags)
            return E_INVALIDARG;

        if ((csidl < CSIDL_LOCAL_APPDATA) && _SHGetSpecialFolderPath(hwnd, pszPath, csidl, bCreate))
        {
            hres = S_OK;
        }
        else
        {
            hres = _DownLevelGetFolderPath(csidl, pszPath, bCreate);
        }
    }
    return hres;
}


STDAPI SHGetFolderPathA(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPSTR pszPath)
{
    WCHAR wsz[MAX_PATH];
    HRESULT hres;

    wsz[0] = 0;
    if (IsBadWritePtr(pszPath, MAX_PATH * sizeof(*pszPath)))
        return E_INVALIDARG;

    pszPath[0] = 0;

    hres = SHGetFolderPathW(hwnd, csidl, NULL, 0, wsz);
    if (_SHUnicodeToAnsi(wsz, pszPath, MAX_PATH))
        return hres;
    else
        return HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);       
}

BOOL APIENTRY DllMain(IN HANDLE hDll, IN DWORD dwReason, IN LPVOID lpReserved)
{
    switch(dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hDll);
        g_hinst = hDll;
        break;
        
    default:
        break;
    }
    
    return TRUE;
}

BOOL _AddAccessAllowedAce(PACL pAcl, DWORD dwAceRevision, DWORD AccessMask, PSID pSid)
{
    //
    // First verify that the SID is valid on this platform
    //
    WCHAR szName[MAX_PATH], szDomain[MAX_PATH];
    DWORD cbName = ARRAYSIZE(szName);
    DWORD cbDomain = ARRAYSIZE(szDomain);

    SID_NAME_USE snu;
    if (LookupAccountSidW(NULL, pSid, szName, &cbName, szDomain, &cbDomain, &snu))
    {
        //
        // Yes, it's valid; now add the ACE
        //
        return AddAccessAllowedAce(pAcl, dwAceRevision, AccessMask, pSid);
    }

    return FALSE;
}

BOOL _AddAces(PACL pAcl, const ACEPARAMLIST* papl, ULONG cPapl)
{
    ULONG i;
    for (i = 0; i < cPapl; i++)
    {
        PSID psid = &c_StaticSids[papl[i].dwSidIndex];

        if (_AddAccessAllowedAce(pAcl, ACL_REVISION, papl[i].AccessMask, psid))
        {
            if (papl[i].dwAceFlags)
            {
                ACE_HEADER* pAceHeader;
                if (GetAce(pAcl, i, &pAceHeader))
                {
                    pAceHeader->AceFlags |= papl[i].dwAceFlags;
                }
                else
                {
                    return FALSE;
                }
            }
        }
        else
        {
            return FALSE;
        }
    }

    return TRUE;
}

PACL _CreateAcl(ULONG cPapl)
{
    // Allocate space for the ACL
    DWORD cbAcl = (cPapl * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + sizeof(c_StaticSids[0])))
                  + sizeof(ACL);

    PACL pAcl = (PACL) GlobalAlloc(GPTR, cbAcl);
    if (pAcl) 
    {
        InitializeAcl(pAcl, cbAcl, ACL_REVISION);
    }

    return pAcl;
}

BOOL _SetDirAccess(LPCWSTR pszDir, const ACEPARAMLIST* papl, ULONG cPapl)
{
    BOOL bRetVal = FALSE;
    PACL pAcl;

    ASSERT(RunningOnNT());

    pAcl = _CreateAcl(cPapl);
    if (pAcl)
    {
        if (_AddAces(pAcl, papl, cPapl))
        {
            SECURITY_DESCRIPTOR sd;

            // Put together the security descriptor
            if (InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
            {
                if (SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE))
                {
                    // Set the security
                    bRetVal = SetFileSecurityW(pszDir, DACL_SECURITY_INFORMATION, &sd);
                }
            }
        }

        GlobalFree(pAcl);
    }

    return bRetVal;
}
