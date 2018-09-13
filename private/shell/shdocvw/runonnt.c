#include "priv.h"

#include <mluisupp.h>

//============================================================================
// This file contains a bunch of Unicode/Ansi thunks to handle calling
// some internal functions that on Windows 95 the strings are Ansi,
// whereas the string on NT are unicode
//============================================================================

// First undefine everything that we are intercepting as to not forward back to us...
#undef ILCreateFromPath
#undef PathCleanupSpec
#undef PathQualify
#undef PathProcessCommand
#define PathProcessCommand  _PathProcessCommand 
#undef SHCLSIDFromString
#undef SHGetSpecialFolderPath
#undef SHILCreateFromPath
#undef SHSimpleIDListFromPath
#undef OleStrToStrN
#undef ShellMessageBox
#undef GetFileNameFromBrowse
#undef OpenRegStream
#undef Win32DeleteFile
#undef PathYetAnotherMakeUniqueName
#undef PathResolve
#undef IsLFNDrive
#undef Shell_GetCachedImageIndex
#undef SHRunControlPanel
#undef PickIconDlg

#if 0
#define TF_THUNK    TF_CUSTOM1
#else
#define TF_THUNK    0
#endif

#define THUNKMSG(psz)   TraceMsg(TF_THUNK, "shdv THUNK::%s", psz)

#define ISINTRESOURCE(r) (r == MAKEINTRESOURCE(r) && r != NULL)


// BUGBUG:: need to properly handle not having ILGetdisplaynameex...
typedef BOOL (*PFNILGETDISPLAYNAMEEX)(LPSHELLFOLDER psfRoot, LPCITEMIDLIST pidl, LPTSTR pszName, int fType);

#ifndef ANSI_SHELL32_ON_UNIX
#define UseUnicodeShell32() (g_fRunningOnNT)
#else
#define UseUnicodeShell32() (FALSE)
#endif


//=================================================================================
// Now the stupid thunks...

int _AorW_SHRunControlPanel(LPCTSTR pszOrig_cmdline, HWND errwnd)
{
    CHAR szPath[MAX_PATH];
    if (!UseUnicodeShell32())
    {
        UnicodeToAnsi(pszOrig_cmdline, szPath, ARRAYSIZE(szPath));
        pszOrig_cmdline = (LPCTSTR)szPath;  // overload the pointer to pass through...
    }
    return SHRunControlPanel(pszOrig_cmdline, errwnd);
}

int _AorW_Shell_GetCachedImageIndex(LPCTSTR pszIconPath, int iIconIndex, UINT uIconFlags)
{
    CHAR szPath[MAX_PATH];
    if (!UseUnicodeShell32())
    {
        UnicodeToAnsi(pszIconPath, szPath, ARRAYSIZE(szPath));
        pszIconPath = (LPCTSTR)szPath;  // overload the pointer to pass through...
    }
    return Shell_GetCachedImageIndex(pszIconPath, iIconIndex, uIconFlags);
}

// the reverse, do it for wide strings also..
int _WorA_Shell_GetCachedImageIndex(LPCWSTR pszIconPath, int iIconIndex, UINT uIconFlags)
{
    CHAR szPath[MAX_PATH];
    if (!UseUnicodeShell32())
    {
        UnicodeToAnsi(pszIconPath, szPath, ARRAYSIZE(szPath));
        pszIconPath = (LPCWSTR)szPath;  // overload the pointer to pass through...
    }
    return Shell_GetCachedImageIndex( (LPCTSTR) pszIconPath, iIconIndex, uIconFlags);
}

// Explicit prototype because only the A/W prototypes exist in the headers
WINSHELLAPI LPITEMIDLIST  WINAPI ILCreateFromPath(LPCTSTR pszPath);

LPITEMIDLIST _AorW_ILCreateFromPath(LPCTSTR pszPath)
{
    CHAR szPath[MAX_PATH];
    THUNKMSG(TEXT("ILCreateFromPath"));
    if (!UseUnicodeShell32())
    {
        UnicodeToAnsi(pszPath, szPath, ARRAYSIZE(szPath));
        pszPath = (LPCTSTR)szPath;  // overload the pointer to pass through...
    }

    return ILCreateFromPath(pszPath);
}

int _AorW_PathCleanupSpec(LPCTSTR pszDir, LPTSTR pszSpec)
{
    THUNKMSG(TEXT("PathCleanupSpec"));
    if (!UseUnicodeShell32())
    {
        CHAR szDir[MAX_PATH];
        CHAR szSpec[MAX_PATH];
        LPSTR pszDir2 = szDir;
        int iRet;

        if (pszDir) {
            UnicodeToAnsi(pszDir, szDir, ARRAYSIZE(szDir));
        } else {
            pszDir2 = NULL;
        }

        UnicodeToAnsi(pszSpec, szSpec, ARRAYSIZE(szSpec));

        iRet = PathCleanupSpec((LPTSTR)pszDir2, (LPTSTR)szSpec);

        AnsiToUnicode(szSpec, pszSpec, MAX_PATH);
        return iRet;
    }
    else
        return PathCleanupSpec(pszDir, pszSpec);
}

void _AorW_PathQualify(LPTSTR pszDir)
{
    THUNKMSG(TEXT("PathQualify"));
    if (!UseUnicodeShell32())
    {
        CHAR szDir[MAX_PATH];

        UnicodeToAnsi(pszDir, szDir, ARRAYSIZE(szDir));
        PathQualify((LPTSTR)szDir);
        AnsiToUnicode(szDir, pszDir, MAX_PATH);
    }
    else
        PathQualify(pszDir);
}

LONG WINAPI _AorW_PathProcessCommand(LPCTSTR lpSrc, LPTSTR lpDest, int iDestMax, DWORD dwFlags)
{
    LONG    lReturnValue;

    THUNKMSG(TEXT("PathProcessCommand"));
    if (!UseUnicodeShell32())
    {
        CHAR szSrc[MAX_PATH];
        CHAR szDest[MAX_PATH];

        UnicodeToAnsi(lpSrc, szSrc, ARRAYSIZE(szSrc));
        lReturnValue = PathProcessCommand((LPTSTR)szSrc, (LPTSTR)szDest, iDestMax, dwFlags);
        AnsiToUnicode(szDest, lpDest, iDestMax);
    }
    else
        lReturnValue = PathProcessCommand(lpSrc, lpDest, iDestMax, dwFlags);

    return(lReturnValue);
}

HRESULT _AorW_SHCLSIDFromString(LPCTSTR lpsz, LPCLSID lpclsid)
{
    CHAR szPath[MAX_PATH];
    THUNKMSG(TEXT("SHCLSIDFromString"));
    if (!UseUnicodeShell32())
    {
        UnicodeToAnsi(lpsz, szPath, ARRAYSIZE(szPath));
        lpsz = (LPCTSTR)szPath;  // overload the pointer to pass through...
    }

    return SHCLSIDFromString(lpsz, lpclsid);
}

#ifndef UNIX
// Explicit prototype because only the A/W prototypes exist in the headers
WINSHELLAPI BOOL WINAPI SHGetSpecialFolderPath(HWND hwndOwner, LPTSTR lpszPath, int nFolder, BOOL fCreate);
#else
#ifdef UNICODE
#define SHGetSpecialFolderPath SHGetSpecialFolderPathW
#else
#define SHGetSpecialFolderPath SHGetSpecialFolderPathA
#endif
#endif

BOOL _AorW_SHGetSpecialFolderPath(HWND hwndOwner, LPTSTR pszPath, int nFolder, BOOL fCreate)
{
    THUNKMSG(TEXT("SHGetSpecialFolderPath"));
    if (!UseUnicodeShell32())
    {
        CHAR szPath[MAX_PATH];
        BOOL fRet = SHGetSpecialFolderPath(hwndOwner, (LPTSTR)szPath, nFolder, fCreate);
        if (fRet)
            AnsiToUnicode(szPath, pszPath, MAX_PATH);
        return fRet;
    }
    else
        return SHGetSpecialFolderPath(hwndOwner, pszPath, nFolder, fCreate);
}

HRESULT _AorW_SHILCreateFromPath(LPCTSTR pszPath, LPITEMIDLIST *ppidl, DWORD *rgfInOut)
{
    WCHAR wszPath[MAX_PATH];
    CHAR szPath[MAX_PATH];

    THUNKMSG(TEXT("SHILCreateFromPath"));

    if (pszPath)
    {
        //
        //  Shell32 will blindly copy pszPath into a MAX_PATH buffer.  This
        //  results in a exploitable buffer overrun.  Do not pass more than
        //  MAX_PATH characters.
        //
        if (!UseUnicodeShell32())
        {
            UnicodeToAnsi(pszPath, szPath, ARRAYSIZE(szPath));
            pszPath = (LPCTSTR)szPath;  // overload the pointer to pass through...
        }
        else if (lstrlenW(pszPath) >= MAX_PATH)
        {
            StrCpyN(wszPath, pszPath, ARRAYSIZE(wszPath));
            pszPath = wszPath; // overload the pointer to pass through...
        }
    }

    return SHILCreateFromPath(pszPath, ppidl, rgfInOut);
}

LPITEMIDLIST _AorW_SHSimpleIDListFromPath(LPCTSTR pszPath)
{
    CHAR szPath[MAX_PATH];
    THUNKMSG(TEXT("SHSimpleIDListFromPath"));
    if (!UseUnicodeShell32() && pszPath)
    {
        UnicodeToAnsi(pszPath, szPath, ARRAYSIZE(szPath));
        pszPath = (LPCTSTR)szPath;  // overload the pointer to pass through...
    }

    return SHSimpleIDListFromPath(pszPath);
}


int _AorW_OleStrToStrN(LPTSTR psz, int cchMultiByte, LPCOLESTR pwsz, int cchWideChar)
{
    int cch;

    THUNKMSG(TEXT("OleStrToStrN"));
    //
    // This function shouldn't be used on UNICODE builds!
    //
    cch = (-1 == cchWideChar) ? lstrlenW(pwsz) : cchWideChar;

    if(cch < cchMultiByte)
        StrCpyN(psz, pwsz, cchMultiByte);

    return (cch < cchMultiByte || 0 == cchMultiByte) ? cch : 0;
}


#define TEMP_SMALL_BUF_SZ  256
BOOL WINAPI _AorW_GetFileNameFromBrowse(HWND hwnd, LPTSTR pszFilePath, UINT cchFilePath,
        LPCTSTR pszWorkingDir, LPCTSTR pszDefExt, LPCTSTR pszFilters, LPCTSTR pszTitle)
{
    CHAR    szPath[MAX_PATH];
    CHAR    szDir[MAX_PATH];
    CHAR    szExt[TEMP_SMALL_BUF_SZ];
    CHAR    szFilters[TEMP_SMALL_BUF_SZ*2];
    CHAR    szTitle[TEMP_SMALL_BUF_SZ];
    LPTSTR  pszPath = pszFilePath;
    BOOL    bResult;
    THUNKMSG(TEXT("GetFileNameFromBrowse"));

    // thunk strings to ansi 
    if (!UseUnicodeShell32()) 
    {
        // always move szFilePath stuff to wszPath buffer. Should never be a resourceid.
        UnicodeToAnsi((LPCTSTR)pszFilePath, szPath, ARRAYSIZE(szPath));
        pszPath = (LPTSTR)szPath;
        if (!ISINTRESOURCE(pszWorkingDir)) //not a resource
        {
            UnicodeToAnsi((LPCTSTR)pszWorkingDir, szDir, ARRAYSIZE(szDir));
            pszWorkingDir = (LPCTSTR)szDir;
        }
        if (!ISINTRESOURCE(pszDefExt)) //not a resource
        {
            UnicodeToAnsi((LPCTSTR)pszDefExt, szExt, ARRAYSIZE(szExt));
            pszDefExt = (LPCTSTR)szExt;
        }
        if (!ISINTRESOURCE(pszFilters)) //not a resource
        {
            int l=1;
            while (*(pszFilters+l) != 0 || *(pszFilters+l-1) != 0)
                l++;
            WideCharToMultiByte(CP_ACP, 0, (LPCTSTR)pszFilters, l+1, szFilters,
                                ARRAYSIZE(szFilters), NULL, NULL);
            pszFilters = (LPCTSTR)szFilters;
        }
        if (!ISINTRESOURCE(pszTitle)) //not a resource
        {
            UnicodeToAnsi((LPCTSTR)pszTitle, szTitle, ARRAYSIZE(szTitle));
            pszTitle = (LPCTSTR)szTitle;
        }
    }

    bResult = GetFileNameFromBrowse(hwnd, pszPath, cchFilePath, pszWorkingDir, pszDefExt, pszFilters, pszTitle);

    if (!UseUnicodeShell32())
    {
        AnsiToUnicode(szPath, pszFilePath, cchFilePath);
    }

    return (bResult);
}

IStream * _AorW_OpenRegStream(HKEY hkey, LPCTSTR pszSubkey, LPCTSTR pszValue, DWORD grfMode)
{
    CHAR szSubkey[MAX_PATH];      // large enough to hold most any name...
    CHAR szValue[MAX_PATH];       // dito.
    if (!UseUnicodeShell32())
    {

        UnicodeToAnsi(pszSubkey, szSubkey, ARRAYSIZE(szSubkey));
        pszSubkey = (LPCTSTR)szSubkey;
        if (pszValue)
        {
            UnicodeToAnsi(pszValue, szValue, ARRAYSIZE(szValue));
            pszValue = (LPCTSTR)szValue;
        }
    }

    return OpenRegStream(hkey, pszSubkey, pszValue, grfMode);

}


DWORD
Win95_GetProcessDword(
    IN DWORD idProcess,
    IN LONG  iIndex)
{
    DWORD dwRet = 0;

    if (!UseUnicodeShell32())
    {
        // Normally we'd just GetProcAddress from KERNEL32.  However,
        // kernel prohibits anyone from getting a no-named export from
        // itself.  So we've added a no-named export to SHELL32 which
        // will call directly into GetProcessDword on the Win95 platform.
        dwRet = SHGetProcessDword(idProcess, iIndex);
    }

    return dwRet;
}


BOOL 
_AorW_Win32DeleteFile(LPCTSTR lpszFileName)
{
    CHAR szPath[MAX_PATH];
    THUNKMSG(TEXT("Win32DeleteFile"));
    if (!UseUnicodeShell32())
    {
        UnicodeToAnsi(lpszFileName, szPath, ARRAYSIZE(szPath));
        lpszFileName = (LPCTSTR)szPath;  // overload the pointer to pass through...
    }
    return Win32DeleteFile(lpszFileName);
}

BOOL
_AorW_PathYetAnotherMakeUniqueName(LPTSTR  pszUniqueName,
                                  LPCTSTR pszPath,
                                  LPCTSTR pszShort,
                                  LPCTSTR pszFileSpec)
{
    CHAR szUniqueName[MAX_PATH];
    CHAR szPath[MAX_PATH];
    CHAR szShort[32];
    CHAR szFileSpec[MAX_PATH];
    BOOL fRet;
    THUNKMSG(TEXT("PathYetAnotherMakeUniqueName"));
    if (!UseUnicodeShell32())
    {
        UnicodeToAnsi(pszPath, szPath, ARRAYSIZE(szPath));
        pszPath = (LPCTSTR)szPath;  // overload the pointer to pass through...

        if (pszShort)
        {
            UnicodeToAnsi(pszShort, szShort, ARRAYSIZE(szShort));
            pszShort = (LPCTSTR)szShort;  // overload the pointer to pass through...
        }

        if (pszFileSpec)
        {
            UnicodeToAnsi(pszFileSpec, szFileSpec, ARRAYSIZE(szFileSpec));
            pszFileSpec = (LPCTSTR)szFileSpec;  // overload the pointer to pass through...
        }

        fRet = PathYetAnotherMakeUniqueName((LPTSTR)szUniqueName, pszPath, pszShort, pszFileSpec);
        if (fRet)
            AnsiToUnicode(szUniqueName, pszUniqueName, MAX_PATH);

        return fRet;
    }
    else
        return PathYetAnotherMakeUniqueName(pszUniqueName, pszPath, pszShort, pszFileSpec);
}

BOOL
_AorW_PathResolve(LPTSTR lpszPath, LPCTSTR rgpszDirs[], UINT fFlags)
{
    CHAR szPath[MAX_PATH];
    CHAR szDir[MAX_PATH];
    BOOL fRet;
    THUNKMSG(TEXT("PathResolve"));
    if (!UseUnicodeShell32())
    {
        // BUGBUG!!!
        // Super Hack, we assume dirs has only one element since it's the only case
        // this is called in SHDOCVW.

        UnicodeToAnsi(lpszPath, szPath, ARRAYSIZE(szPath));

        if (rgpszDirs && rgpszDirs[0])
        {
            UnicodeToAnsi(rgpszDirs[0], szDir, ARRAYSIZE(szDir));
            rgpszDirs[0] = (LPCTSTR)szDir;  // overload the pointer to pass through...

            if (rgpszDirs[1])
            {
                AssertMsg(0, TEXT("PathResolve thunk needs to be fixed to handle more than one dirs."));
                rgpszDirs[1] = NULL;
            }
        }

        fRet = PathResolve((LPTSTR)szPath, rgpszDirs, fFlags);
        if (fRet)
            AnsiToUnicode(szPath, lpszPath, MAX_PATH);

        return fRet;
    }
    else
        return PathResolve(lpszPath, rgpszDirs, fFlags);
}


// Explicit prototype because only the A/W prototypes exist in the headers
// For UNIX, the old prototype should be defined, because there is no export
// by ordinal there and IsLFNDrive is exported from shell32 just this way.

#ifndef UNIX
BOOL IsLFNDrive(LPCTSTR pszPath);
#else
#  ifdef UNICODE
#    define IsLFNDrive IsLFNDriveW
#  else
#    define IsLFNDrive IsLFNDriveA
#  endif
#endif

BOOL
_AorW_IsLFNDrive(LPTSTR lpszPath)
{
    CHAR szPath[MAX_PATH];
    THUNKMSG(TEXT("IsLFNDrive"));
    if (!UseUnicodeShell32())
    {
        UnicodeToAnsi(lpszPath, szPath, ARRAYSIZE(szPath));
        return IsLFNDrive((LPCTSTR)szPath);
    }
    return IsLFNDrive((LPCTSTR)lpszPath);
}


int _AorW_PickIconDlg(
    IN     HWND  hwnd, 
    IN OUT LPTSTR pszIconPath, 
    IN     UINT  cchIconPath, 
    IN OUT int * piIconIndex)
{
    int  nRet;

    if (UseUnicodeShell32())
    {
        nRet = PickIconDlg(hwnd, pszIconPath, cchIconPath, piIconIndex);
    }
    else
    {
        CHAR szPath[MAX_PATH];
        UINT cch = ARRAYSIZE(szPath);

        UnicodeToAnsi(pszIconPath, szPath, cch);
        nRet = PickIconDlg(hwnd, (LPTSTR)szPath, cch, piIconIndex);
        AnsiToUnicode(szPath, pszIconPath, cchIconPath);
    }

    return nRet;
}

BOOL _AorW_StrRetToStrN(/*OUT*/ LPTSTR szOut, UINT uszOut, LPSTRRET pStrRet, LPCITEMIDLIST pidl)
{
    THUNKMSG(TEXT("StrRetToStrN"));
    switch (pStrRet->uType)
    {
    case STRRET_WSTR:
        {
            SHUnicodeToTChar(pStrRet->DUMMYUNION_MEMBER(pOleStr), szOut, uszOut);
            SHFree(pStrRet->DUMMYUNION_MEMBER(pOleStr));
        }
        break;

    case STRRET_CSTR:
        SHAnsiToTChar(pStrRet->DUMMYUNION_MEMBER(cStr), szOut, uszOut);
        break;

    case STRRET_OFFSET:
        if (pidl)
        {
            // BUGBUG (DavePl) Alignment problems here

            SHAnsiToTChar(STRRET_OFFPTR(pidl,pStrRet), szOut, uszOut);
            break;
        }
        goto punt;

    default:
        ASSERT( FALSE && "Bad STRRET uType");
punt:
        if (uszOut)
        {
            *szOut = TEXT('\0');
        }
        return(FALSE);
    }

    return(TRUE);
}


