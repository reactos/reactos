#include "shellprv.h"
#pragma  hdrstop

#include <userenv.h>
#include "drives.h"
#include "apithk.h"
#include "folder.h"

//---------------------------------------------------------------------------
// Get the path for the CSIDL_ folders  and optionally create it if it
// doesn't exist.
//
// Returns FALSE if the special folder given isn't one of those above or the
// directory couldn't be created.
// By default all the special folders are in the windows directory.
// This can be overidden by a [.Shell Folders] section in win.ini with
// entries like Desktop = c:\stuff\desktop
// This in turn can be overidden by a "per user" section in win.ini eg
// [Shell Folder Ianel] - the user name for this section is the current
// network user name, if this fails the default network user name is used
// and if this fails the name given at setup time is used.
//
// "Shell Folders" is the key that records all the absolute paths to the
// shell folders.  The values there are always supposed to be present.
//
// "User Shell Folders" is the key where the user's modifications from
// the defaults are stored.  On Win9x, since we don't use env vars to encode
// the path, If a folder is in the default location, the
// corresponding value is not present under this key. this is to make roaming
// work. on NT we use env vars so this is not an issue. it's only found
// under "Shell Folders".
//
// When we need to find the location of a path, we look in "User Shell Folders"
// first, and if that's not there, generate the default path.  In either
// case we then write the absolute path under "Shell Folders" for other
// apps to look at.  This is so that HKEY_CURRENT_USER can be propagated
// to a machine with Windows installed in a different directory, and as
// long as the user hasn't changed the setting, they won't have the other
// Windows directory hard-coded in the registry.
//   -- gregj, 11/10/94

typedef enum {
    SDIF_CREATE_IN_ROOT         = 0x00000001,   // create in root (not in profiles dir)
    SDIF_CREATE_IN_WINDIR       = 0x00000002,   // create in the windows dir (not in profiles dir)
    SDIF_CREATE_IN_ALLUSERS     = 0x00000003,   // create in "All Users" folder (not in profiles dir)
    SDIF_CREATE_IN_MYDOCUMENTS  = 0x00000004,   // create in CSIDL_PERSONAL folder
    SDIF_CREATE_IN_MASK         = 0x0000000F,   // mask for above values

    SDIF_CAN_DELETE             = 0x00000010,
    SDIF_SHORTCUT_RELATIVE      = 0x00000020,   // make shortcuts relative to this folder
    SDIF_HIDE                   = 0x00000040,   // hide these when we create them
    SDIF_EMPTY_IF_NOT_IN_REG    = 0x00000080,   // does not exist if nothing in the registry
    SDIF_NOT_FILESYS            = 0x00000100,   // not a file system folder
    SDIF_NOT_TRACKED            = 0x00000200,   // don't track this, it can't change
    SDIF_CONST_IDLIST           = 0x00000400,   // don't alloc or free this
    SDIF_REMOVABLE              = 0x00000800,   // Can exist on removable media
    SDIF_CANT_MOVE_RENAME       = 0x00001000,   // can't move or rename this
    SDIF_WX86                   = 0x00002000,   // do Wx86 thunking
    SDIF_NETWORKABLE            = 0x00004000,   // Can be moved to the net
} FOLDER_FLAGS;

typedef struct {
    int id;                     // CSIDL_ value
    int idsDefault;             // string id of default folder name name
    LPCTSTR pszValueName;       // reg key (not localized)
    HKEY hKey;                  // HKCU or HKLM (Current User or Local Machine)
    FOLDER_FLAGS dwFlags;
} FOLDER_INFO;

const FOLDER_INFO c_rgFolderInfo[] = 
{
    { CSIDL_DESKTOP,   IDS_CSIDL_DESKTOPDIRECTORY, TEXT("DesktopFolder"), NULL, SDIF_NOT_TRACKED | SDIF_CONST_IDLIST },
    { CSIDL_NETWORK,   -1, TEXT("NetworkFolder"), NULL, SDIF_NOT_TRACKED | SDIF_NOT_FILESYS | SDIF_CONST_IDLIST },
    { CSIDL_DRIVES,    -1, TEXT("DriveFolder"), NULL, SDIF_NOT_TRACKED | SDIF_NOT_FILESYS | SDIF_CONST_IDLIST },
    { CSIDL_INTERNET,  -1, TEXT("InternetFolder"), NULL, SDIF_NOT_TRACKED | SDIF_NOT_FILESYS | SDIF_CONST_IDLIST },
    { CSIDL_CONTROLS,  -1, TEXT("ControlPanelFolder"), NULL, SDIF_NOT_TRACKED | SDIF_NOT_FILESYS },
    { CSIDL_PRINTERS,  -1, TEXT("PrintersFolder"), NULL, SDIF_NOT_TRACKED | SDIF_NOT_FILESYS } ,
    { CSIDL_BITBUCKET, -1, TEXT("RecycleBinFolder"), NULL, SDIF_NOT_TRACKED | SDIF_NOT_FILESYS },
    { CSIDL_CONNECTIONS, -1, TEXT("ConnectionsFolder"), NULL, SDIF_NOT_TRACKED | SDIF_NOT_FILESYS | SDIF_CONST_IDLIST },

    { CSIDL_FONTS, IDS_CSIDL_FONTS, TEXT("Fonts"), HKEY_CURRENT_USER, SDIF_NOT_TRACKED | SDIF_CREATE_IN_WINDIR | SDIF_CANT_MOVE_RENAME },

    { CSIDL_DESKTOPDIRECTORY, IDS_CSIDL_DESKTOPDIRECTORY, TEXT("Desktop"), HKEY_CURRENT_USER, SDIF_SHORTCUT_RELATIVE },

    // _STARTUP is a subfolder of _PROGRAMS is a subfolder of _STARTMENU -- keep that order
    { CSIDL_STARTUP,    IDS_CSIDL_STARTUP, TEXT("Startup"), HKEY_CURRENT_USER, 0 },
    { CSIDL_PROGRAMS,   IDS_CSIDL_PROGRAMS, TEXT("Programs"), HKEY_CURRENT_USER, 0 },
    { CSIDL_STARTMENU,  IDS_CSIDL_STARTMENU, TEXT("Start Menu"), HKEY_CURRENT_USER, SDIF_SHORTCUT_RELATIVE},

    { CSIDL_RECENT,     IDS_CSIDL_RECENT, TEXT("Recent"), HKEY_CURRENT_USER, SDIF_HIDE | SDIF_CANT_MOVE_RENAME },

    { CSIDL_SENDTO,     IDS_CSIDL_SENDTO, TEXT("SendTo"), HKEY_CURRENT_USER, SDIF_HIDE },
    { CSIDL_PERSONAL,   IDS_CSIDL_PERSONAL, TEXT("Personal"), HKEY_CURRENT_USER, SDIF_SHORTCUT_RELATIVE | SDIF_NETWORKABLE | SDIF_REMOVABLE },
    { CSIDL_FAVORITES,  IDS_CSIDL_FAVORITES, TEXT("Favorites"), HKEY_CURRENT_USER, 0 },

    { CSIDL_NETHOOD,    IDS_CSIDL_NETHOOD, TEXT("NetHood"), HKEY_CURRENT_USER, SDIF_HIDE },
    { CSIDL_PRINTHOOD,  IDS_CSIDL_PRINTHOOD, TEXT("PrintHood"), HKEY_CURRENT_USER, SDIF_HIDE },
    { CSIDL_TEMPLATES,  IDS_CSIDL_TEMPLATES, TEXT("Templates"), HKEY_CURRENT_USER, SDIF_HIDE },

    // Common special folders

    // _STARTUP is a subfolder of _PROGRAMS is a subfolder of _STARTMENU -- keep that order

    { CSIDL_COMMON_STARTUP,  IDS_CSIDL_STARTUP,    TEXT("Common Startup"), HKEY_LOCAL_MACHINE, SDIF_EMPTY_IF_NOT_IN_REG | SDIF_CREATE_IN_ALLUSERS | SDIF_CANT_MOVE_RENAME},
    { CSIDL_COMMON_PROGRAMS,  IDS_CSIDL_PROGRAMS,  TEXT("Common Programs"), HKEY_LOCAL_MACHINE, SDIF_EMPTY_IF_NOT_IN_REG | SDIF_CREATE_IN_ALLUSERS },
    { CSIDL_COMMON_STARTMENU, IDS_CSIDL_STARTMENU, TEXT("Common Start Menu"), HKEY_LOCAL_MACHINE, SDIF_EMPTY_IF_NOT_IN_REG | SDIF_SHORTCUT_RELATIVE | SDIF_CREATE_IN_ALLUSERS },
    { CSIDL_COMMON_DESKTOPDIRECTORY, IDS_CSIDL_DESKTOPDIRECTORY, TEXT("Common Desktop"), HKEY_LOCAL_MACHINE, SDIF_EMPTY_IF_NOT_IN_REG | SDIF_SHORTCUT_RELATIVE | SDIF_CREATE_IN_ALLUSERS },
    { CSIDL_COMMON_FAVORITES, IDS_CSIDL_FAVORITES, TEXT("Common Favorites"), HKEY_LOCAL_MACHINE, SDIF_EMPTY_IF_NOT_IN_REG | SDIF_CREATE_IN_ALLUSERS },

    { CSIDL_COMMON_APPDATA,   IDS_CSIDL_APPDATA,   TEXT("Common AppData"),   HKEY_LOCAL_MACHINE, SDIF_SHORTCUT_RELATIVE | SDIF_CREATE_IN_ALLUSERS },
    { CSIDL_COMMON_TEMPLATES, IDS_CSIDL_TEMPLATES, TEXT("Common Templates"), HKEY_LOCAL_MACHINE, SDIF_NOT_TRACKED | SDIF_CAN_DELETE | SDIF_CREATE_IN_ALLUSERS },
    { CSIDL_COMMON_DOCUMENTS, IDS_CSIDL_ALLUSERS_DOCUMENTS, TEXT("Common Documents"), HKEY_LOCAL_MACHINE, SDIF_NOT_TRACKED | SDIF_CAN_DELETE | SDIF_CREATE_IN_ALLUSERS },

    // Application Data special folder

    { CSIDL_APPDATA, IDS_CSIDL_APPDATA, TEXT("AppData"), HKEY_CURRENT_USER, SDIF_SHORTCUT_RELATIVE },
    { CSIDL_LOCAL_APPDATA, IDS_CSIDL_LOCAL_APPDATA, TEXT("Local AppData"), HKEY_CURRENT_USER, 0 },

    // Non-localized startup folder (do not localize this folde name)
    { CSIDL_ALTSTARTUP, IDS_CSIDL_ALTSTARTUP, TEXT("AltStartup"), HKEY_CURRENT_USER, SDIF_EMPTY_IF_NOT_IN_REG },

    // Non-localized Common StartUp group (do not localize this folde name)
    { CSIDL_COMMON_ALTSTARTUP, IDS_CSIDL_ALTSTARTUP, TEXT("Common AltStartup"), HKEY_LOCAL_MACHINE, SDIF_EMPTY_IF_NOT_IN_REG | SDIF_CREATE_IN_ALLUSERS },

    // Per-user Internet-related folders

    { CSIDL_INTERNET_CACHE, IDS_CSIDL_CACHE, TEXT("Cache"), HKEY_CURRENT_USER, 0 },
    { CSIDL_COOKIES, IDS_CSIDL_COOKIES, TEXT("Cookies"), HKEY_CURRENT_USER, 0 },
    { CSIDL_HISTORY, IDS_CSIDL_HISTORY, TEXT("History"), HKEY_CURRENT_USER, 0 },

    { CSIDL_WINDOWS,                0, TEXT("Windows"), 0, SDIF_NOT_TRACKED | SDIF_SHORTCUT_RELATIVE | SDIF_CANT_MOVE_RENAME },
    { CSIDL_SYSTEM,                 0, TEXT("System"), 0, SDIF_NOT_TRACKED | SDIF_CANT_MOVE_RENAME },
    { CSIDL_PROGRAM_FILES,          0, TEXT("ProgramFiles"), 0, SDIF_NOT_TRACKED | SDIF_CAN_DELETE | SDIF_SHORTCUT_RELATIVE },
    { CSIDL_PROGRAM_FILES_COMMON,   0, TEXT("CommonProgramFiles"), 0, SDIF_NOT_TRACKED | SDIF_CAN_DELETE },
    { CSIDL_PROFILE,                0, TEXT("Profile"), 0, SDIF_NOT_TRACKED | SDIF_CANT_MOVE_RENAME },
    { CSIDL_MYPICTURES, IDS_CSIDL_MYPICTURES, TEXT("My Pictures"), HKEY_CURRENT_USER, SDIF_CAN_DELETE | SDIF_NETWORKABLE | SDIF_REMOVABLE | SDIF_CREATE_IN_MYDOCUMENTS },

    { CSIDL_SYSTEMX86, 0, TEXT("SystemX86"), 0, SDIF_NOT_TRACKED | SDIF_CANT_MOVE_RENAME | SDIF_WX86 },

#ifdef WX86
    { CSIDL_PROGRAM_FILESX86,          0, TEXT("ProgramFilesX86"), 0, SDIF_NOT_TRACKED | SDIF_CAN_DELETE | SDIF_SHORTCUT_RELATIVE|SDIF_WX86},
    { CSIDL_PROGRAM_FILES_COMMONX86,   0, TEXT("CommonProgramFilesX86"), 0, SDIF_NOT_TRACKED | SDIF_CAN_DELETE | SDIF_WX86 },
#else
    { CSIDL_PROGRAM_FILESX86,          0, TEXT("ProgramFilesX86"), 0, SDIF_NOT_TRACKED | SDIF_CAN_DELETE | SDIF_SHORTCUT_RELATIVE},
    { CSIDL_PROGRAM_FILES_COMMONX86,   0, TEXT("CommonProgramFilesX86"), 0, SDIF_NOT_TRACKED | SDIF_CAN_DELETE },
#endif
    { CSIDL_ADMINTOOLS,         IDS_CSIDL_ADMINTOOLS, TEXT("Administrative Tools"), HKEY_CURRENT_USER, 0 },
    { CSIDL_COMMON_ADMINTOOLS,  IDS_CSIDL_ADMINTOOLS, TEXT("Common Administrative Tools"), HKEY_LOCAL_MACHINE, SDIF_CREATE_IN_ALLUSERS },

    { -1, 0, NULL, 0, 0 },
};

// this array holds a cache of the valures of these folders. this cache can only
// be used in the hToken == NULL case otherwise we would need a per user version
// of this cache.

#define SFENTRY(x)  { (LPTSTR)-1, (LPITEMIDLIST)x }

EXTERN_C const IDREGITEM c_aidlConnections[];

struct {
    LPTSTR       psz;
    LPITEMIDLIST pidl;
} g_aFolderCache[] = {
    SFENTRY(&c_idlDesktop),    // CSIDL_DESKTOP                   (0x0000)
    SFENTRY(&c_idlInetRoot),   // CSIDL_INTERNET                  (0x0001)
    SFENTRY(-1),               // CSIDL_PROGRAMS                  (0x0002)
    SFENTRY(-1),               // CSIDL_CONTROLS                  (0x0003)
    SFENTRY(-1),               // CSIDL_PRINTERS                  (0x0004)
    SFENTRY(-1),               // CSIDL_PERSONAL                  (0x0005)
    SFENTRY(-1),               // CSIDL_FAVORITES                 (0x0006)
    SFENTRY(-1),               // CSIDL_STARTUP                   (0x0007)
    SFENTRY(-1),               // CSIDL_RECENT                    (0x0008)
    SFENTRY(-1),               // CSIDL_SENDTO                    (0x0009)
    SFENTRY(-1),               // CSIDL_BITBUCKET                 (0x000a)
    SFENTRY(-1),               // CSIDL_STARTMENU                 (0x000b)
    SFENTRY(-1),               // <unused>                        (0x000c)
    SFENTRY(-1),               // <unused>                        (0x000d)
    SFENTRY(-1),               // <unused>                        (0x000e)
    SFENTRY(-1),               // <unused>                        (0x000f)
    SFENTRY(-1),               // CSIDL_DESKTOPDIRECTORY          (0x0010)
    SFENTRY(&c_idlDrives),     // CSIDL_DRIVES                    (0x0011)
    SFENTRY(&c_idlNet),        // CSIDL_NETWORK                   (0x0012)
    SFENTRY(-1),               // CSIDL_NETHOOD                   (0x0013)
    SFENTRY(-1),               // CSIDL_FONTS                     (0x0014)
    SFENTRY(-1),               // CSIDL_TEMPLATES                 (0x0015)
    SFENTRY(-1),               // CSIDL_COMMON_STARTMENU          (0x0016)
    SFENTRY(-1),               // CSIDL_COMMON_PROGRAMS           (0X0017)
    SFENTRY(-1),               // CSIDL_COMMON_STARTUP            (0x0018)
    SFENTRY(-1),               // CSIDL_COMMON_DESKTOPDIRECTORY   (0x0019)
    SFENTRY(-1),               // CSIDL_APPDATA                   (0x001a)
    SFENTRY(-1),               // CSIDL_PRINTHOOD                 (0x001b)
    SFENTRY(-1),               // CSIDL_LOCAL_APPDATA             (0x001c)
    SFENTRY(-1),               // CSIDL_ALTSTARTUP                (0x001d)
    SFENTRY(-1),               // CSIDL_COMMON_ALTSTARTUP         (0x001e)
    SFENTRY(-1),               // CSIDL_COMMON_FAVORITES          (0x001f)
    SFENTRY(-1),               // CSIDL_INTERNET_CACHE            (0x0020)
    SFENTRY(-1),               // CSIDL_COOKIES                   (0x0021)
    SFENTRY(-1),               // CSIDL_HISTORY                   (0x0022)
    SFENTRY(-1),               // CSIDL_COMMON_APPDATA            (0x0023)
    SFENTRY(-1),               // CSIDL_WINDOWS                   (0x0024)
    SFENTRY(-1),               // CSIDL_SYSTEM                    (0x0025)
    SFENTRY(-1),               // CSIDL_PROGRAM_FILES             (0x0026)
    SFENTRY(-1),               // CSIDL_MYPICTURES                (0x0027)
    SFENTRY(-1),               // CSIDL_PROFILE                   (0x0028)
    SFENTRY(-1),               // CSIDL_SYSTEMX86                 (0x0029)
    SFENTRY(-1),               // CSIDL_PROGRAM_FILESX86          (0x002a)
    SFENTRY(-1),               // CSIDL_PROGRAM_FILES_COMMON      (0x002b)
    SFENTRY(-1),               // CSIDL_PROGRAM_FILES_COMMONX86   (0x002c)
    SFENTRY(-1),               // CSIDL_COMMON_TEMPLATES          (0x002d)
    SFENTRY(-1),               // CSIDL_COMMON_DOCUMENTS          (0x002e)
    SFENTRY(-1),               // CSIDL_COMMON_ADMINTOOLS         (0x002f)
    SFENTRY(-1),               // CSIDL_ADMINTOOLS                (0x0030)
    SFENTRY(c_aidlConnections), // CSIDL_CONNECTIONS              (0x0031)
};

HRESULT _OpenKeyForFolder(const FOLDER_INFO *pfi, HANDLE hToken, LPCTSTR pszSubKey, HKEY *phkey);
void _UpdateShellFolderCache(void);
BOOL GetUserProfileDir(HANDLE hToken, TCHAR *pszPath);
HRESULT VerifyAndCreateFolder(HWND hwnd, const FOLDER_INFO *pfi, UINT uFlags, LPTSTR pszPath) ;


#define _IsDefaultUserToken(hToken)     ((HANDLE)-1 == hToken)


const FOLDER_INFO *_GetFolderInfo(int csidl)
{
    const FOLDER_INFO *pfi;

    // make sure g_aFolderCache can be indexed by the CSIDL values

    COMPILETIME_ASSERT((ARRAYSIZE(g_aFolderCache) - 1) == CSIDL_CONNECTIONS);

    for (pfi = c_rgFolderInfo; pfi->id != -1; pfi++)
    {
        if (pfi->id == csidl)
            return pfi;
    }
    return NULL;
}


// expand an individual enviornment variable
// in:
//      pszVar      "%USERPROFILE%
//      pszValue    "c:\winnt\profiles\user"
//
// in/out:
//      pszToExpand in: %USERPROFILE%\My Docs", out: c:\winnt\profiles\user\My Docs"

BOOL ExpandEnvVar(LPCTSTR pszVar, LPCTSTR pszValue, LPTSTR pszToExpand)
{
    TCHAR *pszStart = StrStrI(pszToExpand, pszVar);
    if (pszStart)
    {
        TCHAR szAfter[MAX_PATH];

        lstrcpy(szAfter, pszStart + lstrlen(pszVar));   // save the tail
        lstrcpyn(pszStart, pszValue, (int) (MAX_PATH - (pszStart - pszToExpand)));
        StrCatBuff(pszToExpand, szAfter, MAX_PATH);       // put the tail back on
        return TRUE;
    }
    return FALSE;
}

HANDLE GetCurrentUserToken()
{
    HANDLE hToken;
    if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_IMPERSONATE, TRUE, &hToken) ||
        OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_IMPERSONATE, &hToken))
        return hToken;
    return NULL;
}


// like ExpandEnvironmentStrings but is robust to the enviornment variables
// not being set. this works on...
// %SYSTEMROOT%
// %SYSTEMDRIVE%
// %USERPROFILE%
// %ALLUSERSPROFILE%
//
// in the rare case (Winstone!) that there is a NULL enviornment block

DWORD ExpandEnvironmentStringsNoEnv(HANDLE hToken, LPCTSTR pszExpand, LPTSTR pszOut, UINT cchOut)
{
    TCHAR szPath[MAX_PATH];
#ifdef WINNT
    if (hToken && !_IsDefaultUserToken(hToken))
    {
        if (!ExpandEnvironmentStringsForUser(hToken, pszExpand, pszOut, cchOut))
            lstrcpyn(pszOut, pszExpand, cchOut);
    }
    else if (hToken == NULL)
#endif
    {
        // to debug env expansion failure...
        // lstrcpyn(pszOut, pszExpand, cchOut);
        SHExpandEnvironmentStrings(pszExpand, pszOut, cchOut);
    }

    // manually expand in this order since 
    //  %USERPROFILE% -> %SYSTEMDRIVE%\Docs & Settings

#ifdef WINNT
    if (StrChr(pszOut, TEXT('%')) && (hToken == NULL))
    {
        hToken = GetCurrentUserToken();
        if (hToken)
        {
            // this does %USERPROFILE% and other per user stuff
            ExpandEnvironmentStringsForUser(hToken, pszExpand, pszOut, cchOut);
            CloseHandle(hToken);
        }
    }
    else if (_IsDefaultUserToken(hToken) && StrChr(pszOut, TEXT('%')))
    {
        GetUserProfileDir(hToken, szPath);
        ExpandEnvVar(TEXT("%USERPROFILE%"), szPath, pszOut);
    }
#endif

    if (*pszOut == TEXT('%'))
    {
        GetAllUsersDirectory(szPath);
        ExpandEnvVar(TEXT("%ALLUSERSPROFILE%"), szPath, pszOut);
    }

    if (*pszOut == TEXT('%'))
    {
        GetSystemWindowsDirectory(szPath, ARRAYSIZE(szPath));
        ExpandEnvVar(TEXT("%SYSTEMROOT%"), szPath, pszOut);
    }

    if (*pszOut == TEXT('%'))
    {
        GetSystemWindowsDirectory(szPath, ARRAYSIZE(szPath));
        ASSERT(szPath[1] == TEXT(':')); // this better not be a UNC!
        szPath[2] = TEXT('\0'); // SYSTEMDRIVE = 'c:', not 'c:\'
        ExpandEnvVar(TEXT("%SYSTEMDRIVE%"), szPath, pszOut);
    }

    if (*pszOut == TEXT('%'))
        *pszOut = 0;

    return lstrlen(pszOut) + 1;    // +1 to cover the NULL
}

// get the user profile directory:
// on Win9x: reads from the registry and FAILS if user profiles are not turned on
// on NT: uses the hToken as needed to determine the proper user profile

BOOL GetUserProfileDir(HANDLE hToken, TCHAR *pszPath)
{
#ifdef WINNT
    DWORD dwcch = MAX_PATH;
    HANDLE hClose = NULL;
    BOOL fRet;
    
    *pszPath = 0;       // in case of error

    if ( !hToken )
    {
        hClose = hToken = GetCurrentUserToken();
    }
    if (_IsDefaultUserToken(hToken) && g_bRunOnNT5)
    {
        fRet = GetDefaultUserProfileDirectory( pszPath, &dwcch );
    }
    else
    {
        fRet = GetUserProfileDirectory( hToken, pszPath, &dwcch );
    }
    if ( hClose )
    {
        CloseHandle( hClose );
    }
    return fRet;
#else
    DWORD cbData = MAX_PATH * SIZEOF(TCHAR);
    *pszPath = 0;       // in case of error
    SHGetValue(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\ProfileReconciliation"), 
        TEXT("ProfileDirectory"), NULL, pszPath, &cbData);
#endif
    return (BOOL)*pszPath;
}

#ifdef WX86
void SetUseKnownWx86Dll(const FOLDER_INFO *pfi, BOOL bValue)
{
    if (pfi->dwFlags & SDIF_WX86)
    {
        //  GetSystemDirectory() knows we're looking for the Wx86 system
        //  directory when this flag is set.
        NtCurrentTeb()->Wx86Thread.UseKnownWx86Dll = bValue ? TRUE : FALSE;
    }
}
#else
#define SetUseKnownWx86Dll(pfi, bValue)
#endif

// read from registry
BOOL GetProgramFiles(LPCTSTR pszValue, LPTSTR pszPath)
{
    DWORD cbPath = MAX_PATH * sizeof(*pszPath);

    *pszPath = 0;

    SHGetValue(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion"), 
        pszValue, NULL, pszPath, &cbPath);
    return (BOOL)*pszPath;
}

void LoadDefaultString(int idString, LPTSTR lpBuffer, int cchBufferMax)
{
#ifdef WINNT
    BOOL fSucceeded = FALSE;
    HRSRC hResInfo;
    HANDLE hStringSeg;
    LPWSTR lpsz;
    int    cch;
    HMODULE hmod = GetModuleHandle(TEXT("SHELL32"));
    
    // Make sure the parms are valid.     
    if (lpBuffer == NULL || cchBufferMax == 0) 
    {
        return;
    }

    cch = 0;
    
    // String Tables are broken up into 16 string segments.  Find the segment
    // containing the string we are interested in.     
    if (hResInfo = FindResourceExW(hmod, (LPCWSTR)RT_STRING,
                                   (LPWSTR)((LONG)(((USHORT)idString >> 4) + 1)), GetSystemDefaultUILanguage())) 
    {        
        // Load that segment.        
        hStringSeg = LoadResource(hmod, hResInfo);
        
        // Lock the resource.        
        if (lpsz = (LPWSTR)LockResource(hStringSeg)) 
        {            
            // Move past the other strings in this segment.
            // (16 strings in a segment -> & 0x0F)             
            idString &= 0x0F;
            while (TRUE) 
            {
                cch = *((WORD *)lpsz++);   // PASCAL like string count
                                            // first UTCHAR is count if TCHARs
                if (idString-- == 0) break;
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

            fSucceeded = TRUE;

        }
    }

    if (!fSucceeded)
#endif
    {
        LoadString(HINST_THISDLL, idString, lpBuffer, cchBufferMax);
    }
}


// out:
//      pszPath     fills in with the full path with no env gunk (MAX_PATH)

HRESULT _GetFolderDefaultPath(const FOLDER_INFO *pfi, HANDLE hToken, LPTSTR pszPath)
{
    TCHAR szEntry[MAX_PATH];

    *pszPath = 0;

    switch (pfi->id)
    {
    case CSIDL_PROFILE:
        GetUserProfileDir(hToken, pszPath);
        break;

    case CSIDL_PROGRAM_FILES:
        GetProgramFiles(TEXT("ProgramFilesDir"), pszPath);
        break;


    case CSIDL_PROGRAM_FILES_COMMON:
        GetProgramFiles(TEXT("CommonFilesDir"  ), pszPath);
        break;

    case CSIDL_PROGRAM_FILESX86:
        GetProgramFiles(TEXT("ProgramFilesDir (x86)"), pszPath);
        break;

    case CSIDL_PROGRAM_FILES_COMMONX86:
        GetProgramFiles(TEXT("CommonFilesDir (x86)"), pszPath);
        break;

    case CSIDL_SYSTEMX86:
    case CSIDL_SYSTEM:
        // tell GetSystemDirectory() we are know what we are doing
        SetUseKnownWx86Dll(pfi, TRUE);
        GetSystemDirectory(pszPath, MAX_PATH);
        SetUseKnownWx86Dll(pfi, FALSE);
        break;

    case CSIDL_WINDOWS:
        GetWindowsDirectory(pszPath, MAX_PATH);
        break;

    default:
        switch (pfi->dwFlags & SDIF_CREATE_IN_MASK)
        {
        case SDIF_CREATE_IN_ROOT:
            GetWindowsDirectory(pszPath, MAX_PATH);
            PathStripToRoot(pszPath);
            break;

        case SDIF_CREATE_IN_ALLUSERS:
            GetAllUsersDirectory(pszPath);
            break;

        case SDIF_CREATE_IN_WINDIR:
            GetWindowsDirectory(pszPath, MAX_PATH);
            break;

        case SDIF_CREATE_IN_MYDOCUMENTS:
        {
            HRESULT hr = SHGetFolderPath(NULL, CSIDL_PERSONAL, hToken, SHGFP_TYPE_CURRENT, pszPath);
            if (FAILED(hr))
            {
                return hr;
            }
            break;
        }

        default:
            if (!GetUserProfileDir(hToken, pszPath))
            {
                // Win9x, failed to get user profile dir. Using windows directory...
                GetWindowsDirectory(pszPath, MAX_PATH);
                if (pfi->id == CSIDL_PERSONAL) 
                {
                    PathStripToRoot(pszPath);
                }
            }
            break;
        }

        LoadDefaultString(pfi->idsDefault, szEntry, ARRAYSIZE(szEntry));
        PathAppend(pszPath, szEntry);
        break;
    }
    return *pszPath ? S_OK : E_FAIL;
}


void RegSetFolderPath(const FOLDER_INFO *pfi, LPCTSTR pszSubKey, LPCTSTR pszPath)
{
    HKEY hk;
    if (SUCCEEDED(_OpenKeyForFolder(pfi, NULL, pszSubKey, &hk)))
    {
        if (pszPath)
            RegSetValueEx(hk, pfi->pszValueName, 0, REG_SZ, (LPBYTE)pszPath, (1 + lstrlen(pszPath)) * SIZEOF(TCHAR));
        else
            RegDeleteValue(hk, pfi->pszValueName);
        RegCloseKey(hk);
    }
}

BOOL RegQueryPath(HKEY hk, LPCTSTR pszValue, LPTSTR pszPath)
{
    DWORD cbPath = MAX_PATH * SIZEOF(TCHAR);

    *pszPath = 0;
    SHQueryValueEx(hk, pszValue, 0, NULL, pszPath, &cbPath);
    return (BOOL)*pszPath;
}


// More than 50 is silly
#define MAX_TEMP_FILE_TRIES         50

// returns:
//      S_OK        the path exists and it is a folder
//      FAILED()    result
HRESULT _IsFolderNotFile(LPCTSTR pszFolder)
{
    HRESULT hr;
    DWORD dwAttribs = GetFileAttributes(pszFolder);
    if (dwAttribs == -1)
    {
        DWORD err = GetLastError();
        hr = HRESULT_FROM_WIN32(err);
    }
    else
    {
        // see if it is a file, if so we need to rename that file
        if (dwAttribs & FILE_ATTRIBUTE_DIRECTORY)
        {
            hr = S_OK;
        }
        else
        {
            int iExt = 0;
            do
            {
                TCHAR szExt[32], szDst[MAX_PATH];

                wsprintf(szExt, TEXT(".%03d"), iExt);
                lstrcpy(szDst, pszFolder);
                lstrcat(szDst, szExt);
                if (MoveFile(pszFolder, szDst))
                    iExt = 0;
                else
                {
                    // Normally we fail because .00x already exists but that may not be true.
                    DWORD dwError = GetLastError();
                    if (ERROR_ALREADY_EXISTS == dwError)
                        iExt++;     // Try the next one...
                    else
                        iExt = 0;   // We have problems and need to give up. (No write access?)
                }

            } while (iExt && (iExt < MAX_TEMP_FILE_TRIES));

            hr = HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
        }
    }
    return hr;
}

HRESULT _OpenKeyForFolder(const FOLDER_INFO *pfi, HANDLE hToken, LPCTSTR pszSubKey, HKEY *phkey)
{
    TCHAR szRegPath[255];
    LONG err;
    HKEY hkRoot, hkeyToFree = NULL;

    *phkey = NULL;

    lstrcpy(szRegPath, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\"));
    lstrcat(szRegPath, pszSubKey);

#ifdef  WINNT
    if (_IsDefaultUserToken(hToken) && (pfi->hKey == HKEY_CURRENT_USER))
    {
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_USERS, TEXT(".Default"), 0, KEY_READ, &hkRoot))
            hkeyToFree = hkRoot;
        else
            return E_FAIL;
    }
    else
#endif
    if (hToken && (pfi->hKey == HKEY_CURRENT_USER))
    {
        if (GetUserProfileKey(hToken, &hkRoot))
            hkeyToFree = hkRoot;
        else
            return E_FAIL;
    }
    else
        hkRoot = pfi->hKey;

    err = RegCreateKeyEx(hkRoot, szRegPath, 0, NULL, REG_OPTION_NON_VOLATILE,
                MAXIMUM_ALLOWED, NULL, phkey, NULL);
    
    if (hkeyToFree)
        RegCloseKey(hkeyToFree);

    return HRESULT_FROM_WIN32(err);
}

// returns:
//      S_OK        found in registry, path well formed
//      S_FALSE     empty registry
//      FAILED()    failure result

HRESULT _GetFolderFromReg(const FOLDER_INFO *pfi, HANDLE hToken, LPTSTR pszPath)
{
    HKEY hkUSF;
    HRESULT hr;

    *pszPath = 0;

    hr = _OpenKeyForFolder(pfi, hToken, TEXT("User Shell Folders"), &hkUSF);
    if (SUCCEEDED(hr))
    {
        TCHAR szExpand[MAX_PATH];
        DWORD dwType, cbPath = SIZEOF(szExpand);

        if (RegQueryValueEx(hkUSF, pfi->pszValueName, 0, &dwType, (BYTE *)szExpand, &cbPath) == ERROR_SUCCESS)
        {
            if (REG_SZ == dwType)
            {
                lstrcpyn(pszPath, szExpand, MAX_PATH);
            }
            else if (REG_EXPAND_SZ == dwType)
            {
                ExpandEnvironmentStringsNoEnv(hToken, szExpand, pszPath, MAX_PATH);
            }
            TraceMsg(TF_PATH, "_CreateFolderPath 'User Shell Folders' %s = %s", pfi->pszValueName, pszPath);
        }

        if (*pszPath == 0)
        {
            hr = S_FALSE;     // empty registry, success but empty
        }
        else if ((PathGetDriveNumber(pszPath) != -1) || PathIsUNC(pszPath))
        {
            hr = S_OK;        // good reg path, fully qualified
        }
        else
        {
            *pszPath = 0;       // bad reg data
            hr = E_INVALIDARG;
        }

        RegCloseKey(hkUSF);
    }
    return hr;
}


HRESULT _GetFolderPath(HWND hwnd, const FOLDER_INFO *pfi, HANDLE hToken, UINT uFlags, LPTSTR pszPath)
{
    HRESULT hr;

    *pszPath = 0;       // assume failure

    if (pfi->hKey)
    {
        hr = _GetFolderFromReg(pfi, hToken, pszPath);
        if (SUCCEEDED(hr))
        {
            if (hr == S_FALSE)
            {
                // empty registry, SDIF_EMPTY_IF_NOT_IN_REG means they don't exist
                // if the registry is not populated with a value. this lets us disable
                // the common items on platforms that don't want them

                if (pfi->dwFlags & SDIF_EMPTY_IF_NOT_IN_REG)
                    return S_FALSE;     // success, but empty

                hr = _GetFolderDefaultPath(pfi, hToken, pszPath);
            }


            if (!(uFlags & CSIDL_FLAG_DONT_VERIFY))
            {
               hr = VerifyAndCreateFolder(hwnd, pfi, uFlags, pszPath) ;
            }

            if (hr != S_OK)
                *pszPath = 0;

            if (!(uFlags & CSIDL_FLAG_DONT_VERIFY))
            {
                HKEY hkey;
                // record value in "Shell Folders", even in the failure case

                // NOTE: we only do this for historical reasons. there may be some
                // apps that depend on these values being in the registry, but in general
                // the contetens here are unreliable as they are only written after someone
                // asks for the folder through this API.

                if (SUCCEEDED(_OpenKeyForFolder(pfi, hToken, TEXT("Shell Folders"), &hkey)))
                {
                    RegSetValueEx(hkey, pfi->pszValueName, 0, REG_SZ, (LPBYTE)pszPath, (1 + lstrlen(pszPath)) * SIZEOF(TCHAR));
                    RegCloseKey(hkey);
                }
            }
        }
    }
    else
    {
        hr = _GetFolderDefaultPath(pfi, hToken, pszPath);

        if (S_OK == hr)
        {
            if (!(uFlags & CSIDL_FLAG_DONT_VERIFY))
            {
                hr = VerifyAndCreateFolder(hwnd, pfi, uFlags, pszPath);
            }
        }
        else
        {
            *pszPath = 0;
        }
    }
    ASSERT(hr == S_OK ? *pszPath != 0 : *pszPath == 0);
    return hr;
}

HRESULT VerifyAndCreateFolder(HWND hwnd, const FOLDER_INFO *pfi, UINT uFlags, LPTSTR pszPath)
{
    HRESULT hr ;

    hr = _IsFolderNotFile(pszPath);

    // 99/06/16 vtan: In the case of ERROR_ACCESS_DENIED don't
    // even bother trying to do anything more with the directory.

    if (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
    {
        return(hr);
    }

    if ((hr != S_OK) && hwnd)
    {
        // we might be able to reconnect if this is a net path
        if (PathIsUNC(pszPath))
        {
            if (SHValidateUNC(hwnd, pszPath, 0))
                hr = _IsFolderNotFile(pszPath);
        }
        else if (IsDisconnectedNetDrive(DRIVEID(pszPath)))
        {
            TCHAR szDrive[4];
            PathBuildSimpleRoot(DRIVEID(pszPath), szDrive);

            if (WNetRestoreConnection(hwnd, szDrive) == WN_SUCCESS)
                hr = _IsFolderNotFile(pszPath);
         }
    }

    if ((hr != S_OK) && (uFlags & CSIDL_FLAG_CREATE))
    {
        DWORD err = SHCreateDirectory(NULL, pszPath);
        hr = HRESULT_FROM_WIN32(err);
        if (hr == S_OK)
        {
#ifdef DBG 
            //  we want to fire this assert in checked builds, even when FULL_DEBUG is not on
            if (StrChr(pszPath, TEXT('%')))
                DebugBreak();
#endif
            if (pfi->dwFlags & SDIF_HIDE)
                SetFileAttributes(pszPath, GetFileAttributes(pszPath) | FILE_ATTRIBUTE_HIDDEN);
        }
    }

    return hr ;
}

void _SetPathCache(const FOLDER_INFO *pfi, LPCTSTR psz)
{
    LPTSTR pszOld;

    AssertMsg(((LPCTSTR)-1 == psz) || (NULL == psz) || PathFileExistsAndAttributes(psz, NULL), TEXT("Caching non existant folder!"));

    pszOld = (LPTSTR)InterlockedExchangePointer((void **)&g_aFolderCache[pfi->id].psz, (void *)psz);
    if (pszOld && pszOld != (LPTSTR)-1)
    {
        // check for the concurent use... very rare case
        LocalFree(pszOld);
    }
}


HRESULT _GetFolderPathCached(HWND hwnd, const FOLDER_INFO *pfi, HANDLE hToken, UINT uFlags, LPTSTR pszPath)
{
    HRESULT hr;

    *pszPath = 0;

    // can only cache for the current user, hToken == NULL or per machine folders
    if (!hToken || (pfi->hKey != HKEY_CURRENT_USER))
    {
        LPTSTR pszCache;

        _UpdateShellFolderCache();

        pszCache = (LPTSTR)InterlockedExchangePointer((void **)&g_aFolderCache[pfi->id].psz, (void *)-1);
        if ((pszCache == (LPTSTR)-1) || (pszCache == NULL))
        {
            // either not cached or cached failed state
            if ((pszCache == (LPTSTR)-1) || (uFlags & CSIDL_FLAG_CREATE))
            {
                hr = _GetFolderPath(hwnd, pfi, hToken, uFlags, pszPath);

                // only set the cache value if CSIDL_FLAG_DONT_VERIFY was NOT passed
                if (!(uFlags & CSIDL_FLAG_DONT_VERIFY))
                {
                    if (hr == S_OK)
                    {
                        // dupe the string so we can add it to the cache
                        pszCache = StrDup(pszPath);
                    }
                    else
                    {
                        // we failed to get the folder path, null out the cache
                        ASSERT(*pszPath == 0);
                        pszCache = NULL;
                    }
                    _SetPathCache(pfi, pszCache);
                }

                
            }
            else
            {
                // cache was null and user didnt pass create flag so we just fail
                ASSERT(pszCache == NULL);
                ASSERT(*pszPath == 0);
                hr = E_FAIL;
            }
        }
        else
        {
            // cache hit case: copy the cached string and then restore the cached value back
            lstrcpyn(pszPath, pszCache, MAX_PATH);
            _SetPathCache(pfi, pszCache);
            hr = S_OK;
        }
    }
    else
    {
        hr = _GetFolderPath(hwnd, pfi, hToken, uFlags, pszPath);
    }

    return hr;
}


HRESULT _CreateFolderIDList(HWND hwnd, const FOLDER_INFO *pfi, HANDLE hToken, UINT uFlags, LPITEMIDLIST *ppidl)
{
    HRESULT hr = S_OK;

    switch (pfi->id)
    {
    case CSIDL_PRINTERS:

        if (SHGetAppCompatFlags(ACF_STAROFFICE5PRINTER))
        {
            // Star Office 5.0 relies on the fact that the printer pidl used to be like below.  They skip the 
            // first simple pidl (My Computer) and do not check if there is anything else, they assume that the
            // second simple pidl is the Printer folder one. (stephstm, 07/30/99)

            // CLSID_MyComputer, CLSID_Printers
            hr = SHILCreateFromPath(TEXT("::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{2227A280-3AEA-1069-A2DE-08002B30309D}"), ppidl, NULL);
        }
        else
        {
            // CLSID_MyComputer, CLSID_ControlPanel, CLSID_Printers
            hr = SHILCreateFromPath(TEXT("::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{2227A280-3AEA-1069-A2DE-08002B30309D}"), ppidl, NULL);
        }
        break;

    case CSIDL_CONTROLS:
        // CLSID_MyComputer, CLSID_ControlPanel
        hr = SHILCreateFromPath(TEXT("::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}"), ppidl, NULL);
        break;

    case CSIDL_BITBUCKET:
        // CLSID_RecycleBin
        hr = SHILCreateFromPath(TEXT("::{645FF040-5081-101B-9F08-00AA002F954E}"), ppidl, NULL);
        break;

    default:
        {
            TCHAR szPath[MAX_PATH];
            hr = _GetFolderPathCached(hwnd, pfi, hToken, uFlags, szPath);
            if (hr == S_OK)
            {
                HRESULT hrInit = SHCoInitialize();
                hr = ILCreateFromPathEx(szPath, NULL, ILCFP_FLAG_SKIPJUNCTIONS, ppidl, NULL);
                SHCoUninitialize(hrInit);
            }
            else
            {
                *ppidl = NULL;      // assume failure or empty
            }
        }
        break;
    }
    return hr;
}

void _SetIDListCache(const FOLDER_INFO *pfi, LPCITEMIDLIST pidl)
{
    if (!(pfi->dwFlags & SDIF_CONST_IDLIST))
    {
        LPITEMIDLIST pidlOld = (LPITEMIDLIST)InterlockedExchangePointer((void **)&g_aFolderCache[pfi->id].pidl, (void *)pidl);
        if (pidlOld && pidlOld != (LPITEMIDLIST)-1)
        {
            // check for the concurent use... very rare case
            // ASSERT(pidl == (LPCITEMIDLIST)-1);   // should not really be ASSERT
            ILFree(pidlOld);
        }
    }
}

// hold this lock for the minimal amout of time possible to avoid other users
// of this resource requring them to re-create the pidl

HRESULT _GetFolderIDListCached(HWND hwnd, const FOLDER_INFO *pfi, UINT uFlags, LPITEMIDLIST *ppidl)
{
    HRESULT hr;

    ASSERT(pfi->id < ARRAYSIZE(g_aFolderCache));

    if (pfi->dwFlags & SDIF_CONST_IDLIST)
    {
        // these are CONST, never change
        hr = SHILClone(g_aFolderCache[pfi->id].pidl, ppidl);     
    }
    else
    {
        LPITEMIDLIST pidlCache;

        _UpdateShellFolderCache();

        pidlCache = (LPITEMIDLIST)InterlockedExchangePointer((void **)&g_aFolderCache[pfi->id].pidl, (void *)-1);
        if ((pidlCache == (LPCITEMIDLIST)-1) || (pidlCache == NULL))
        {
            // either uninitalized cache state OR cached failure (NULL)
            if ((pidlCache == (LPCITEMIDLIST)-1) || (uFlags & CSIDL_FLAG_CREATE))
            {
                // not initialized (or concurent use) try creating it for this use
                hr = _CreateFolderIDList(hwnd, pfi, NULL, uFlags, ppidl);
                if (S_OK == hr)
                    hr = SHILClone(*ppidl, &pidlCache); // create cache copy
                else
                    pidlCache = NULL;
            }
            else
                hr = E_FAIL;            // return cached failure
        }
        else
        {
            hr = SHILClone(pidlCache, ppidl);   // cache hit
        }

        // store back the PIDL if it is non NULL or they specified CREATE
        // and we failed to create it (cache the not existant state). this is needed
        // so we don't cache a NULL if the first callers don't ask for create and
        // subsequent callers do
        if (pidlCache || (uFlags & CSIDL_FLAG_CREATE))
            _SetIDListCache(pfi, pidlCache);
    }

    return hr;
}

void _ClearCacheEntry(const FOLDER_INFO *pfi)
{
    if (!(pfi->dwFlags & SDIF_CONST_IDLIST))
        _SetIDListCache(pfi, (LPCITEMIDLIST)-1);
    _SetPathCache(pfi, (LPCTSTR)-1);
}

void _ClearAllCacheEntrys()
{
    const FOLDER_INFO *pfi;
    for (pfi = c_rgFolderInfo; pfi->id != -1; pfi++)
    {
        _ClearCacheEntry(pfi);
    }
}


// Per instance count of mods to Special Folder cache.
HANDLE g_hCounter = NULL;   // Global count of mods to Special Folder cache.
int g_lPerProcessCount = 0;


//----------------------------------------------------------------------------
// Make sure the special folder cache is up to date.
void _UpdateShellFolderCache(void)
{
    long lGlobalCount;
    HANDLE hCounter = SHGetCachedGlobalCounter(&g_hCounter, &GUID_SystemPidlChange);

    // Is the cache up to date?
    lGlobalCount = SHGlobalCounterGetValue(hCounter);
    if (lGlobalCount != g_lPerProcessCount)
    {
        _ClearAllCacheEntrys();
        g_lPerProcessCount = lGlobalCount;
    }
}

STDAPI_(void) SHFlushSFCache(void)
{
    // Increment the shared variable;  the per-process versions will no
    // longer match, causing this and/or other processes to refresh their
    // pidl caches when they next need to access a folder.
    if (g_hCounter)
        SHGlobalCounterIncrement(g_hCounter);
}

// use SHGetFolderLocation() instead using CSIDL_FLAG_CREATE

STDAPI_(LPITEMIDLIST) SHCloneSpecialIDList(HWND hwnd, int csidl, BOOL fCreate)
{
    LPITEMIDLIST pidlReturn;

    if (fCreate)
        csidl |= CSIDL_FLAG_CREATE;

    SHGetSpecialFolderLocation(hwnd, csidl, &pidlReturn);
    return pidlReturn;
}


STDAPI SHGetSpecialFolderLocation(HWND hwnd, int csidl, LPITEMIDLIST *ppidl)
{
    HRESULT hr = SHGetFolderLocation(hwnd, csidl, NULL, 0, ppidl);
    if (hr == S_FALSE)
        hr = E_FAIL;        // mail empty case into failure for compat with this API
    return hr;
}

// return IDLIST for special folder
//      fCreate encoded in csidl with CSIDL_FLAG_CREATE (new for NT5)
//
//  in:
//      hwnd    should be NULL
//      csidl   CSIDL_ value with CSIDL_FLAG_ values ORed in as well
//      dwType  must be SHGFP_TYPE_CURRENT
//
//  out:
//      *ppild  NULL on failure or empty, PIDL to be freed by caller on success
//
//  returns:
//      S_OK        *ppidl is non NULL
//      S_FALISE    *ppidl is NULL, but valid csidl was passed (folder does not exist)
//      FAILED(hr)

STDAPI SHGetFolderLocation(HWND hwnd, int csidl, HANDLE hToken, DWORD dwType, LPITEMIDLIST *ppidl)
{
    const FOLDER_INFO *pfi;
    HRESULT hr;

    *ppidl = NULL;  // in case of error or empty

    // -1 is an invalid csidl
    if ((dwType != SHGFP_TYPE_CURRENT) || (-1 == csidl))
        return E_INVALIDARG;    // no flags used yet, validate this param

    pfi = _GetFolderInfo(csidl & ~CSIDL_FLAG_MASK);
    if (pfi)
    {
        HANDLE hTokenToFree = NULL;

#ifdef WINNT
        if (hToken == NULL && (pfi->hKey == HKEY_CURRENT_USER))
        {
            if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_IMPERSONATE, TRUE, &hToken))
                hTokenToFree = hToken;
        }
#endif
        if (hToken && (pfi->hKey == HKEY_CURRENT_USER))
        {
            // we don't cache PIDLs for other users, do all of the work
            hr = _CreateFolderIDList(hwnd, pfi, hToken, csidl & CSIDL_FLAG_MASK, (LPITEMIDLIST *)ppidl);
        }
        else
        {
            hr = _GetFolderIDListCached(hwnd, pfi, csidl & CSIDL_FLAG_MASK, ppidl);
        }

        if (hTokenToFree)
            CloseHandle(hTokenToFree);
    }
    else
        hr = E_INVALIDARG;    // bad CSIDL (apps can check to veryify our support)
    return hr;
}

STDAPI_(BOOL) SHGetSpecialFolderPath(HWND hwnd, LPTSTR pszPath, int csidl, BOOL fCreate)
{
    if (fCreate)
        csidl |= CSIDL_FLAG_CREATE;
    return SHGetFolderPath(hwnd, csidl, NULL, 0, pszPath) == S_OK;
}

//  in:
//      hwnd    should be NULL
//      csidl   CSIDL_ value with CSIDL_FLAG_ values ORed in as well
//      dwType  must be SHGFP_TYPE_CURRENT
//
//  out:
//      *pszPath    MAX_PATH buffer to get path name, zeroed on failure or empty case
//
//  returns:
//      S_OK        filled in pszPath with path value
//      S_FALSE     pszPath is NULL, valid CSIDL value, but this folder does not exist
//      E_FAIL

STDAPI SHGetFolderPath(HWND hwnd, int csidl, HANDLE hToken, DWORD dwType, LPTSTR pszPath)
{
    HRESULT hr = E_INVALIDARG;
    const FOLDER_INFO *pfi;

    ASSERT(IS_VALID_WRITE_BUFFER(pszPath, TCHAR, MAX_PATH));
    *pszPath = 0;

    pfi = _GetFolderInfo(csidl & ~CSIDL_FLAG_MASK);
    if (pfi && !(pfi->dwFlags & SDIF_NOT_FILESYS))
    {
        switch (dwType)
        {
        case SHGFP_TYPE_DEFAULT:
            ASSERT((csidl & CSIDL_FLAG_MASK) == 0); // meaningless for default
            hr = _GetFolderDefaultPath(pfi, hToken, pszPath);
            break;
    
        case SHGFP_TYPE_CURRENT:
            {
                HANDLE hTokenToFree = NULL;
#ifdef WINNT
                if (hToken == NULL && (pfi->hKey == HKEY_CURRENT_USER))
                {
                    if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_IMPERSONATE, TRUE, &hToken))
                        hTokenToFree = hToken;
                }
#endif
                hr = _GetFolderPathCached(hwnd, pfi, hToken, csidl & CSIDL_FLAG_MASK, pszPath);

                if (hTokenToFree)
                    CloseHandle(hTokenToFree);
            }
            break;
        }
    }
    return hr;
}

#ifdef UNICODE

STDAPI SHGetFolderPathA(HWND hwnd, int csidl, HANDLE hToken, DWORD dwType, LPSTR pszPath)
{
    WCHAR wsz[MAX_PATH];
    HRESULT hr = SHGetFolderPath(hwnd, csidl, hToken, dwType, wsz);

    ASSERT(IS_VALID_WRITE_BUFFER(pszPath, CHAR, MAX_PATH));

    SHUnicodeToAnsi(wsz, pszPath, MAX_PATH);
    return hr;
}

STDAPI_(BOOL) SHGetSpecialFolderPathA(HWND hwnd, LPSTR pszPath, int csidl, BOOL fCreate)
{
    if (fCreate)
        csidl |= CSIDL_FLAG_CREATE;
    return SHGetFolderPathA(hwnd, csidl, NULL, 0, pszPath) == S_OK;
}

#else

STDAPI SHGetFolderPathW(HWND hwnd, int csidl, HANDLE hToken, DWORD dwType, LPWSTR pszPath)
{
    CHAR sz[MAX_PATH];
    HRESULT hr = SHGetFolderPath(hwnd, csidl, hToken, dwType, sz);

    ASSERT(IS_VALID_WRITE_BUFFER(pszPath, WCHAR, MAX_PATH));

    SHAnsiToUnicode(sz, pszPath, MAX_PATH);
    return hr;
}

STDAPI_(BOOL) SHGetSpecialFolderPathW(HWND hwnd, LPWSTR pszPath, int csidl, BOOL fCreate)
{
    if (fCreate)
        csidl |= CSIDL_FLAG_CREATE;
    return SHGetFolderPathW(hwnd, csidl, NULL, 0, pszPath) == S_OK;
}

#endif

//  HRESULT SHSetFolderPath (int csidl, HANDLE hToken, DWORD dwFlags, LPTSTR pszPath)
//
//  in:
//      csidl       CSIDL_ value with CSIDL_FLAG_ values ORed in as well
//      dwFlags     reserved: should be 0x00000000
//      pszPath     path to change shell folder to (will optionally be unexpanded)
//
//  returns:
//      S_OK        function succeeded and flushed cache

STDAPI  SHSetFolderPath (int csidl, HANDLE hToken, DWORD dwFlags, LPCTSTR pszPath)

{
    HRESULT             hr;
    const FOLDER_INFO   *pfi;

    hr = E_INVALIDARG;

    // Validate csidl and dwFlags. Add extra valid flags as needed.

    RIPMSG(((csidl & CSIDL_FLAG_MASK) & ~(CSIDL_FLAG_DONT_UNEXPAND | 0x00000000)) == 0, "SHSetFolderPath: CSIDL flag(s) invalid");
    RIPMSG(dwFlags == 0, "SHSetFolderPath: dwFlags parameter must be 0x00000000");

    // Exit with E_INVALIDARG if bad parameters.

    if ((((csidl & CSIDL_FLAG_MASK) & ~(CSIDL_FLAG_DONT_UNEXPAND | 0x00000000)) != 0) ||
        (dwFlags != 0) ||
        (pszPath == NULL) ||
        (pszPath[0] == TEXT('\0')))
    {
        return(hr);
    }

    pfi = _GetFolderInfo(csidl & ~CSIDL_FLAG_MASK);

    // Only allow setting for SDIF_NOT_FILESYS is clear
    //                        SDIF_NOT_TRACKED is clear
    //                        SDIF_CANT_MOVE_RENAME is clear
    // and for non-NULL value

    // If HKLM is used then rely on security or registry restrictions
    // to enforce whether the change can be made.

    if ((pfi != NULL) &&
        ((pfi->dwFlags & (SDIF_NOT_FILESYS | SDIF_NOT_TRACKED | SDIF_CANT_MOVE_RENAME)) == 0))
    {
        BOOL    fSuccessfulUnexpand, fSuccessfulExpand, fEmptyOrNullPath;
        LONG    lError;
        HANDLE  hTokenToFree;
        TCHAR   szPath[MAX_PATH];
        TCHAR   szExpandedPath[MAX_PATH];   // holds expanded path for "Shell Folder" compat key
        LPCTSTR pszWritePath;

        hTokenToFree = NULL;
#ifdef WINNT
        if ((hToken == NULL) && (pfi->hKey == HKEY_CURRENT_USER))
        {
            if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_IMPERSONATE, TRUE, &hToken))
            {
                hTokenToFree = hToken;
            }
        }
#endif

        fEmptyOrNullPath = ((pszPath == NULL) || (pszPath[0] == TEXT('\0')));
        if (fEmptyOrNullPath)
        {
#ifdef  WINNT
            HKEY    hKeyDefaultUser;

            pszWritePath = NULL;
            if (SUCCEEDED(_OpenKeyForFolder(pfi, (HANDLE)-1, TEXT("User Shell Folders"), &hKeyDefaultUser)))
            {
                DWORD   dwPathSize;

                dwPathSize = sizeof(szPath);
                if (ERROR_SUCCESS == RegQueryValueEx(hKeyDefaultUser,
                                                     pfi->pszValueName,
                                                     NULL,
                                                     NULL,
                                                     (LPBYTE)szPath,
                                                     &dwPathSize))
                {
                    pszWritePath = szPath;
                }
                RegCloseKey(hKeyDefaultUser);
            }
#else
            pszWritePath = NULL;
#endif
            fSuccessfulUnexpand = TRUE;
        }
        else if ((csidl & CSIDL_FLAG_DONT_UNEXPAND) != 0)
        {

            // Does the caller want to write the string as is? Leave
            // it alone if so.

            pszWritePath = pszPath;
            fSuccessfulUnexpand = TRUE;
        }
        else
        {
            if (pfi->hKey == HKEY_CURRENT_USER)
            {
                fSuccessfulUnexpand = (PathUnExpandEnvStringsForUser(hToken, pszPath, szPath, ARRAYSIZE(szPath)) != FALSE);
            }
            pszWritePath = szPath;
        }

        if (fSuccessfulUnexpand)
        {
            HKEY    hKeyUser, hKeyUSF, hKeyToFree;

            // we also get the fully expanded path so that we can write it out to the "Shell Folders" key for lame apps that depend on
            // the old registry values
            fSuccessfulExpand = (SHExpandEnvironmentStringsForUser(hToken, pszPath, szExpandedPath, ARRAYSIZE(szExpandedPath)) != 0);

            // Get either the current users HKCU or HKU\SID if a token
            // was specified and running in NT.

            if ((hToken != NULL) && (GetUserProfileKey(hToken, &hKeyUser) != FALSE))
            {
                hKeyToFree = hKeyUser;
            }
            else
            {
                hKeyUser = pfi->hKey;
                hKeyToFree = NULL;
            }

            // Open the key to the User Shell Folders and write the string
            // there. Clear the shell folder cache.

            // NOTE: This functionality is duplicated in SetFolderPath but
            // that function deals with the USF key only. This function
            // requires HKU\SID so while there is identical functionality
            // from the point of view of settings the USF value that is
            // where it ends. To make this function simple it just writes
            // the value to registry itself.

            // Additional note: there is a threading issue here with
            // clearing the cache entry incrementing the counter. This
            // should be locked access.

            lError = RegOpenKeyEx(hKeyUser,
                                  TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders"),
                                  0,
                                  KEY_READ | KEY_WRITE,
                                  &hKeyUSF);
            if (lError == ERROR_SUCCESS)
            {
                if (pszWritePath != NULL)
                {

                    // vtan: Win98 SE supports writing whatever type of data
                    // you give it. It blindly gives back the same type. If
                    // this function writes REG_EXPAND_SZ any corresponding
                    // reading function (SHGetFolderPath) will automatically
                    // expand the variables so this doesn't appear to present
                    // a compatibility issue. If it does (I have foreseen
                    // incorrectly) it can be solved using an "#ifdef WINNT".

                    lError = RegSetValueEx(hKeyUSF,
                                           pfi->pszValueName,
                                           0,
                                           REG_EXPAND_SZ,
                                           (LPBYTE)pszWritePath,
                                           (lstrlen(pszWritePath) + sizeof('\0')) * sizeof(TCHAR));
                }
                else
                {
                    lError = RegDeleteValue(hKeyUSF, pfi->pszValueName);
                }
                RegCloseKey(hKeyUSF);
                _ClearCacheEntry(pfi);
                g_lPerProcessCount = SHGlobalCounterIncrement(g_hCounter);
            }

            // update the old "Shell Folders" value for compat
            if ((lError == ERROR_SUCCESS) && fSuccessfulExpand)
            {
                HKEY hkeySF;

                if (RegOpenKeyEx(hKeyUser,
                                 TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"),
                                 0,
                                 KEY_READ | KEY_WRITE,
                                 &hkeySF) == ERROR_SUCCESS)
                {
                    if (pszWritePath != NULL)
                    {
                        RegSetValueEx(hkeySF,
                                      pfi->pszValueName,
                                      0,
                                      REG_SZ,
                                      (LPBYTE)szExpandedPath,
                                      (lstrlen(szExpandedPath) + sizeof('\0')) * sizeof(TCHAR));
                    }
                    else
                    {
                        RegDeleteValue(hkeySF, pfi->pszValueName);
                    }

                    RegCloseKey(hkeySF);
                }
            }

            if ((lError == ERROR_SUCCESS) && (pfi->hKey == HKEY_CURRENT_USER))
            {
                switch (csidl & ~CSIDL_FLAG_MASK)
                {
                    case CSIDL_APPDATA:
                    {
                        HKEY    hKeyVolatileEnvironment;

                        // In the case of AppData there is a matching environment variable
                        // for this shell folder. Make sure the place in the registry where
                        // userenv.dll places this value is updated and correct so that when
                        // the user context is created by winlogon it will have the updated
                        // value.

                        // It's probably also a good thing to check for a %APPDATA% variable
                        // in the calling process' context but this would only be good for
                        // the life of the process. What is really required is a mechanism
                        // to change the environment variable for the entire logon session.

                        lError = RegOpenKeyEx(hKeyUser,
                                              TEXT("Volatile Environment"),
                                              0,
                                              KEY_READ | KEY_WRITE,
                                              &hKeyVolatileEnvironment);
                        if (lError == ERROR_SUCCESS)
                        {
                            if (SUCCEEDED(SHGetFolderPath(NULL,
                                                          csidl | CSIDL_FLAG_DONT_VERIFY,
                                                          hToken,
                                                          SHGFP_TYPE_CURRENT,
                                                          szPath)))
                            {
                                lError = RegSetValueEx(hKeyVolatileEnvironment,
                                                       TEXT("APPDATA"),
                                                       0,
                                                       REG_SZ,
                                                       (LPBYTE)szPath,
                                                       (lstrlen(szPath) + sizeof('\0')) * sizeof(TCHAR));
                            }
                            RegCloseKey(hKeyVolatileEnvironment);
                        }
                        break;
                    }

                    default:
                    {
                        break;
                    }
                }
            }

            if (hKeyToFree != NULL)
            {
                RegCloseKey(hKeyToFree);
            }

            if (lError == ERROR_SUCCESS)
            {
                hr = S_OK;
            }
            else
            {
                hr = HRESULT_FROM_WIN32(lError);
            }
        }
        if (hTokenToFree != NULL)
        {
            CloseHandle(hTokenToFree);
        }
    }

    return(hr);
}

#ifdef UNICODE

STDAPI SHSetFolderPathA(int csidl, HANDLE hToken, DWORD dwType, LPCSTR pszPath)
{
    WCHAR       wsz[MAX_PATH];

    SHAnsiToUnicode(pszPath, wsz, MAX_PATH);
    return(SHSetFolderPath(csidl, hToken, dwType, wsz));
}

#else

STDAPI SHSetFolderPathW(int csidl, HANDLE hToken, DWORD dwType, LPCWSTR pszPath)
{
    CHAR        sz[MAX_PATH];

    SHUnicodeToAnsi(pszPath, sz, MAX_PATH);
    return(SHSetFolderPath(csidl, hToken, dwType, sz));
}

#endif

// NOTE: called from DllEntry

void SpecialFolderIDTerminate()
{
    ASSERTDLLENTRY      // does not require a critical section

    _ClearAllCacheEntrys();

    if (g_hCounter)
    {
        CloseHandle(g_hCounter);
        g_hCounter = NULL;
    }
}

// update our cache and the registry for pfi with pszPath. this also invalidates the
// cache in other processes so they stay in sync

void SetFolderPath(const FOLDER_INFO *pfi, LPCTSTR pszPath)
{
    _ClearCacheEntry(pfi);
    
    if (pszPath)
    {
        HKEY hk;
        if (SUCCEEDED(_OpenKeyForFolder(pfi, NULL, TEXT("User Shell Folders"), &hk)))
        {
            LONG err;
            TCHAR szDefaultPath[MAX_PATH];
            
#ifndef WINNT
            // If the path being set is the default, delete the custom
            // setting.  Otherwise, set the new path as the custom setting.
            
            _GetFolderDefaultPath(pfi, NULL, szDefaultPath);
            
            // There is no reason to delete the value on NT, since we are
            // able to write REG_EXPAND_SZ strings to the registry, which
            // handles the roaming case that this RegDeleteValue() attempts
            // to solve for Win9x
            
            // don't delete SDIF_EMPTY_IF_NOT_IN_REG entries, as that will make our
            // code think they are not to be used
            if (!(pfi->dwFlags & SDIF_EMPTY_IF_NOT_IN_REG) &&
                lstrcmpi(szDefaultPath, pszPath) == 0)
            {
                TraceMsg(TF_PATH, "deleting 'User Shell Folders' %s", pfi->pszValueName);
                err = RegDeleteValue(hk, pfi->pszValueName);
            }
            else
#endif // ! WINNT
            {
                // Check for an existing path, and if the unexpanded version
                // of the existing path does not match the new path, then
                // write the new path to the registry.
                //
                // RegQueryPath expands the environment variables for us
                // so we can't just blindly set the new value to the registry.
                //
                
                RegQueryPath(hk, pfi->pszValueName, szDefaultPath);
                
                if (lstrcmpi(szDefaultPath, pszPath) != 0)
                {
                    // The paths are different. Write to the registry as file
                    // system path.

                    err = SHRegSetPath(hk, NULL, pfi->pszValueName, pszPath, 0);
                } 
                else
                    err = ERROR_SUCCESS;
            }
            
            // clear out any temp paths
            RegSetFolderPath(pfi, TEXT("User Shell Folders\\New"), NULL);
            
            if (err == ERROR_SUCCESS)
            {
                // this will force a new creation (see TRUE as fCreate).
                // This will also copy the path from "User Shell Folders"
                // to "Shell Folders".
                LPITEMIDLIST pidl;
                if (S_OK == _GetFolderIDListCached(NULL, pfi, CSIDL_FLAG_CREATE, &pidl))
                {
                    ILFree(pidl);
                }
                else
                {
                    // failed!  null out the entry.  this will go back to our default
                    RegDeleteValue(hk, pfi->pszValueName);
                    _ClearCacheEntry(pfi);
                }
            }
            RegCloseKey(hk);
        }
    }
    else
    {
        RegSetFolderPath(pfi, TEXT("User Shell Folders"), NULL);
        // clear out any temp paths
        RegSetFolderPath(pfi, TEXT("User Shell Folders\\New"), NULL);
    }
    
    // set the global different from the per process variable
    // to signal an update needs to happen other processes
    g_lPerProcessCount = SHGlobalCounterIncrement(g_hCounter);
}


// file system change notifies come in here AFTER the folders have been moved/deleted
// we fix up the registry to match what occured in the file system

void SFP_FSEvent(LONG lEvent, LPITEMIDLIST pidl, LPITEMIDLIST pidlExtra)
{
    const FOLDER_INFO *pfi;
    TCHAR szSrc[MAX_PATH];

    if (!(lEvent & (SHCNE_RENAMEFOLDER | SHCNE_RMDIR | SHCNE_MKDIR)) ||
        !SHGetPathFromIDList(pidl, szSrc)                            ||
        (pidlExtra && ILIsEqual(pidl, pidlExtra)))  // when volume label changes, pidl==pidlExtra so we detect this case and skip it for perf
    {
        return;
    }

    for (pfi = c_rgFolderInfo; pfi->id != -1; pfi++)
    {
        if (0 == (pfi->dwFlags & (SDIF_NOT_TRACKED | SDIF_NOT_FILESYS)))
        {
            TCHAR szCurrent[MAX_PATH];
            if (S_OK == _GetFolderPathCached(NULL, pfi, NULL, CSIDL_FLAG_DONT_VERIFY, szCurrent) &&
                PathIsEqualOrSubFolder(szSrc, szCurrent))
            {
                TCHAR szDest[MAX_PATH];

                szDest[0] = 0;

                if (lEvent & SHCNE_RMDIR)
                {
                    // complete the "move accross volume" case
                    HKEY hk;
                    if (SUCCEEDED(_OpenKeyForFolder(pfi, NULL, TEXT("User Shell Folders\\New"), &hk)))
                    {
                        RegQueryPath(hk, pfi->pszValueName, szDest);
                        RegCloseKey(hk);
                    }
                }
                else if (pidlExtra)
                {
                    SHGetPathFromIDList(pidlExtra, szDest);
                }

                if (szDest[0])
                {
                    // rename the specal folder
                    UINT cch = PathCommonPrefix(szCurrent, szSrc, NULL);
                    ASSERT(cch != 0);
                    
                    if (szCurrent[cch])
                    {
                        PathAppend(szDest, szCurrent + cch);
                    }

                    SetFolderPath(pfi, szDest);
                }
            }
        }
    }
}

// returns the first special folder CSIDL_ id that is a parent
// of the passed in pidl or 0 if not found. only CSIDL_ entries marked as
// SDIF_SHORTCUT_RELATIVE are considered for this.
//
// returns:
//      CSIDL_ values
//      *pcbOffset  offset into pidl

int GetSpecialFolderParentIDAndOffset(LPCITEMIDLIST pidl, ULONG *pcbOffset)
{
    BOOL bFound = FALSE;
    const FOLDER_INFO *pfi;

    for (pfi = c_rgFolderInfo; pfi->id != -1; pfi++)
    {
        if (pfi->dwFlags & SDIF_SHORTCUT_RELATIVE)
        {
            LPITEMIDLIST pidlFolder;
            if (S_OK == _GetFolderIDListCached(NULL, pfi, 0, &pidlFolder))
            {
                BOOL bParent = ILIsParent(pidlFolder, pidl, FALSE);
                if (bParent)
                {
                    LPCITEMIDLIST pidlT = pidl, pidlFolderT = pidlFolder;

                    while (!ILIsEmpty(pidlFolderT))
                    {
                        pidlFolderT = _ILNext(pidlFolderT);
                        pidlT = _ILNext(pidlT);
                    }

                    *pcbOffset = (ULONG)((LPBYTE)pidlT - (LPBYTE)pidl);
                    bFound = TRUE;
                }
                ILFree(pidlFolder);
            }
            if (bFound)
                return pfi->id;
        }
    }
    return 0;
}

// this is copied from mydocs.dll so that we can completely enforce its DisablePersonalDirChange
// policy by blocking rename/move even when mydocs.dll isn't loaded

BOOL CanChangePersonalPath( void )
{
    return (ERROR_SUCCESS != SHGetValue(HKEY_CURRENT_USER,
                                        TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"),
                                        TEXT("DisablePersonalDirChange"),
                                        NULL,
                                        NULL,
                                        NULL));
}

// this is called from the copy engine (like all other copy hooks)
// this is where we put up UI blocking the delete/move of some special folders

int PathCopyHookCallback(HWND hwnd, UINT wFunc, LPCTSTR pszSrc, LPCTSTR pszDest)
{
    int ret = IDYES;

    if ((wFunc == FO_DELETE) || (wFunc == FO_MOVE) || (wFunc == FO_RENAME))
    {
        const FOLDER_INFO *pfi;

        // is one of our system directories being affected?

        for (pfi = c_rgFolderInfo; ret == IDYES && pfi->id != -1; pfi++)
        {
            // even non tracked folders (windows, system) come through here
            if (0 == (pfi->dwFlags & SDIF_NOT_FILESYS))
            {
                TCHAR szCurrent[MAX_PATH];
                if (S_OK == _GetFolderPathCached(NULL, pfi, NULL, CSIDL_FLAG_DONT_VERIFY, szCurrent) &&
                    PathIsEqualOrSubFolder(pszSrc, szCurrent))
                {
                    // Yes
                    if (wFunc == FO_DELETE)
                    {
                        if (pfi->dwFlags & SDIF_CAN_DELETE)
                        {
                            SetFolderPath(pfi, NULL);  // Let them delete some folders
                        }
                        else
                        {
                            ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_CANTDELETESPECIALDIR),
                                            MAKEINTRESOURCE(IDS_DELETE), MB_OK | MB_ICONINFORMATION, PathFindFileName(pszSrc));
                            ret = IDNO;
                        }
                    }
                    else
                    {
                        int idSrc = PathGetDriveNumber(pszSrc);
                        int idDest = PathGetDriveNumber(pszDest);

                        ASSERT((wFunc == FO_MOVE) || (wFunc == FO_RENAME));

                        if ((pfi->dwFlags & SDIF_CANT_MOVE_RENAME) || 
                            (((idSrc != -1) && (idDest == -1) && !(pfi->dwFlags & SDIF_NETWORKABLE)) ||
                             ((idSrc != idDest) && PathIsRemovable(pszDest) && !(pfi->dwFlags & SDIF_REMOVABLE))) ||
                              ((CSIDL_PERSONAL == pfi->id) && !CanChangePersonalPath()))
                        {
                            ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_CANTMOVESPECIALDIRHERE),
                                wFunc == FO_MOVE ? MAKEINTRESOURCE(IDS_MOVE) : MAKEINTRESOURCE(IDS_RENAME), 
                                MB_ICONERROR, PathFindFileName(pszSrc));
                            ret = IDNO;
                        }
                        else
                        {
                            // if this is a logical move that is really conveted into
                            // a copy we detect that here. 2 cases, cross volume
                            if ((idSrc != idDest) || PathIsDirectory(pszDest))
                            {
                                // this is going to be a move across volumes
                                // which means a delete then a create notifications,
                                // store this info here
                                RegSetFolderPath(pfi, TEXT("User Shell Folders\\New"), pszDest);
                            }
                        }
                    }
                }
            }
        }
    }
    return ret;
}

STDAPI_(int) SHGetSpecialFolderID(LPCWSTR pszName)
{
    const FOLDER_INFO *pfi;
    USES_CONVERSION;
    LPCTSTR szPath = W2CT(pszName);

    // make sure g_aFolderCache can be indexed by the CSIDL values

    COMPILETIME_ASSERT((ARRAYSIZE(g_aFolderCache) - 1) == CSIDL_CONNECTIONS);

    for (pfi = c_rgFolderInfo; pfi->id != -1; pfi++)
    {
        if (0 == StrCmpI(szPath, pfi->pszValueName))
            return pfi->id;
    }

    return -1;
}

// Return the special folder ID, if this folder is one of them.
// At this point, we handle PROGRAMS folder only.

//
//  GetSpecialFolderID() 
//  this allows a list of CSIDLs to be passed in.
//  they will be searched in order for the specified csidl
//  and the path will be checked against it.
//  if -1 is specified as the csidl, then all of array entries should
//  be checked for a match with the folder.
//
int GetSpecialFolderID(LPCTSTR pszFolder, const int *rgcsidl, UINT count)
{
    UINT i;
    for (i = 0; i < count; i++)
    {
        int csidlSpecial = rgcsidl[i] & ~TEST_SUBFOLDER;
        TCHAR szPath[MAX_PATH];
        if (S_OK == SHGetFolderPath(NULL, csidlSpecial | CSIDL_FLAG_DONT_VERIFY, NULL, SHGFP_TYPE_CURRENT, szPath))
        {
            if (((rgcsidl[i] & TEST_SUBFOLDER) && PathIsEqualOrSubFolder(szPath, pszFolder)) ||
                (lstrcmpi(szPath, pszFolder) == 0))
            {
                return csidlSpecial;
            }
        }
    }

    return -1;
}
