#include "priv.h"

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
#undef StrRetToStrN
#undef GetFileNameFromBrowse
#undef Win32DeleteFile
#undef PathYetAnotherMakeUniqueName
#undef PathResolve
#undef IsLFNDrive
#undef Shell_GetCachedImageIndex
#undef SHRunControlPanel
#undef PickIconDlg
#undef ILCreateFromPathW
#undef SHCreateDirectory

#if 0
#define TF_THUNK    TF_CUSTOM1
#else
#define TF_THUNK    0
#endif

#define THUNKMSG(psz)   TraceMsg(TF_THUNK, "shdv THUNK::%s", psz)

#ifndef ANSI_SHELL32_ON_UNIX

#ifdef DEBUG
#define UseUnicodeShell32() (g_fRunningOnNT && !(g_dwPrototype & PF_FORCEANSI))
#else
#define UseUnicodeShell32() g_fRunningOnNT
#endif

#else

#define UseUnicodeShell32() (FALSE)

#endif

int _AorW_SHRunControlPanel(LPCTSTR pszOrig_cmdline, HWND errwnd)
{
    if (g_fRunningOnNT)
    {
        WCHAR wzPath[MAX_PATH];

        SHTCharToUnicode(pszOrig_cmdline, wzPath, ARRAYSIZE(wzPath));
        return SHRunControlPanel((LPCTSTR)wzPath, errwnd);
    }
    else
    {
        CHAR szPath[MAX_PATH];

        SHTCharToAnsi(pszOrig_cmdline, szPath, ARRAYSIZE(szPath));
        return SHRunControlPanel((LPCTSTR)szPath, errwnd);
    }
}

int _AorW_Shell_GetCachedImageIndex(LPCTSTR pszIconPath, int iIconIndex, UINT uIconFlags)
{
    if (UseUnicodeShell32())
    {
        WCHAR wzPath[MAX_PATH];

        SHTCharToUnicode(pszIconPath, wzPath, ARRAYSIZE(wzPath));
        return Shell_GetCachedImageIndex((LPCTSTR)wzPath, iIconIndex, uIconFlags);
    }
    else
    {
        CHAR szPath[MAX_PATH];

        SHTCharToAnsi(pszIconPath, szPath, ARRAYSIZE(szPath));
        return Shell_GetCachedImageIndex((LPCTSTR)szPath, iIconIndex, uIconFlags);
    }
}

// the reverse, do it for wide strings also..
int _WorA_Shell_GetCachedImageIndex(LPCWSTR pwzIconPath, int iIconIndex, UINT uIconFlags)
{
    CHAR szPath[MAX_PATH];

    if (!g_fRunningOnNT)
    {
        SHUnicodeToAnsi(pwzIconPath, szPath, ARRAYSIZE(szPath));
        pwzIconPath = (LPCWSTR)szPath;  // overload the pointer to pass through...
    }

    return Shell_GetCachedImageIndex((LPCTSTR)pwzIconPath, iIconIndex, uIconFlags);
}

// Explicit prototype because only the A/W prototypes exist in the headers
STDAPI_(LPITEMIDLIST) ILCreateFromPath(LPCTSTR pszPath);

LPITEMIDLIST _AorW_ILCreateFromPath(LPCTSTR pszPath)
{
    WCHAR wzPath[MAX_PATH];
    CHAR szPath[MAX_PATH];

    THUNKMSG(TEXT("ILCreateFromPath"));

    if (g_fRunningOnNT)
    {
        SHTCharToUnicode(pszPath, wzPath, ARRAYSIZE(wzPath));
        pszPath = (LPCTSTR)wzPath;  // overload the pointer to pass through...
    }
    else
    {
        SHTCharToAnsi(pszPath, szPath, ARRAYSIZE(szPath));
        pszPath = (LPCTSTR)szPath;  // overload the pointer to pass through...
    }

    return ILCreateFromPath(pszPath);
}

int _AorW_PathCleanupSpec(/*IN OPTIONAL*/ LPCTSTR pszDir, /*IN OUT*/ LPTSTR pszSpec)
{
    THUNKMSG(TEXT("PathCleanupSpec"));

    if (g_fRunningOnNT)
    {
        WCHAR wzDir[MAX_PATH];
        WCHAR wzSpec[MAX_PATH];
        LPWSTR pwszDir = wzDir;
        int iRet;

        if (pszDir)
            SHTCharToUnicode(pszDir, wzDir, ARRAYSIZE(wzDir));
        else
            pwszDir = NULL;

        SHTCharToUnicode(pszSpec, wzSpec, ARRAYSIZE(wzSpec));
        iRet = PathCleanupSpec((LPTSTR)pwszDir, (LPTSTR)wzSpec);

        SHUnicodeToTChar(wzSpec, pszSpec, MAX_PATH);
        return iRet;
    }
    else
    {
        CHAR szDir[MAX_PATH];
        CHAR szSpec[MAX_PATH];
        LPSTR pszDir2 = szDir;
        int iRet;

        if (pszDir)
            SHTCharToAnsi(pszDir, szDir, ARRAYSIZE(szDir));
        else
            pszDir2 = NULL;

        SHTCharToAnsi(pszSpec, szSpec, ARRAYSIZE(szSpec));
        iRet = PathCleanupSpec((LPTSTR)pszDir2, (LPTSTR)szSpec);

        SHAnsiToTChar(szSpec, pszSpec, MAX_PATH);
        return iRet;
    }
}

void _AorW_PathQualify(/*IN OUT*/ LPTSTR pszDir)
{
    THUNKMSG(TEXT("PathQualify"));
    if (g_fRunningOnNT)
    {
        WCHAR wszDir[MAX_PATH];

        SHTCharToUnicode(pszDir, wszDir, ARRAYSIZE(wszDir));
        PathQualify((LPTSTR)wszDir);
        SHUnicodeToTChar(wszDir, pszDir, MAX_PATH);
    }
    else
    {
        CHAR szDir[MAX_PATH];

        SHTCharToAnsi(pszDir, szDir, ARRAYSIZE(szDir));
        PathQualify((LPTSTR)szDir);
        SHAnsiToTChar(szDir, pszDir, MAX_PATH);
    }
}

LONG WINAPI _AorW_PathProcessCommand(/*IN*/ LPCTSTR pszSrc, /*OUT*/LPTSTR pszDest, int iDestMax, DWORD dwFlags)
{
    LONG    lReturnValue;

    THUNKMSG(TEXT("PathProcessCommand"));
    if (g_fRunningOnNT)
    {
        WCHAR wszSrc[MAX_PATH];
        WCHAR wszDest[MAX_PATH];

        SHTCharToUnicode(pszSrc, wszSrc, ARRAYSIZE(wszSrc));
        lReturnValue = PathProcessCommand((LPTSTR)wszSrc, (LPTSTR)wszDest, ARRAYSIZE(wszDest), dwFlags);
        SHUnicodeToTChar(wszDest, pszDest, iDestMax);
    }
    else
    {
        CHAR szSrc[MAX_PATH];
        CHAR szDest[MAX_PATH];

        SHTCharToAnsi(pszSrc, szSrc, ARRAYSIZE(szSrc));
        lReturnValue = PathProcessCommand((LPTSTR)szSrc, (LPTSTR)szDest, ARRAYSIZE(szDest), dwFlags);
        SHAnsiToTChar(szDest, pszDest, iDestMax);
    }

    return(lReturnValue);
}

#ifndef UNIX

// Explicit prototype because only the A/W prototypes exist in the headers
STDAPI_(BOOL) SHGetSpecialFolderPath(HWND hwndOwner, LPTSTR lpszPath, int nFolder, BOOL fCreate);

#else

#ifdef UNICODE
#define SHGetSpecialFolderPath SHGetSpecialFolderPathW
#else
#define SHGetSpecialFolderPath SHGetSpecialFolderPathA
#endif

#endif

BOOL _AorW_SHGetSpecialFolderPath(HWND hwndOwner, /*OUT*/ LPTSTR pszPath, int nFolder, BOOL fCreate)
{
    THUNKMSG(TEXT("SHGetSpecialFolderPath"));

    if (g_fRunningOnNT)
    {
        WCHAR wzPath[MAX_PATH];

        BOOL fRet = SHGetSpecialFolderPath(hwndOwner, (LPTSTR)wzPath, nFolder, fCreate);
        if (fRet)
            SHUnicodeToTChar(wzPath, pszPath, MAX_PATH);

        return fRet;
    }
    else
    {
        CHAR szPath[MAX_PATH];

        BOOL fRet = SHGetSpecialFolderPath(hwndOwner, (LPTSTR)szPath, nFolder, fCreate);
        if (fRet)
            SHAnsiToTChar(szPath, pszPath, MAX_PATH);

        return fRet;
    }
}

HRESULT _AorW_SHILCreateFromPath(/*IN OPTIONAL*/LPCTSTR pszPath, LPITEMIDLIST *ppidl, DWORD *rgfInOut)
{
    WCHAR wzPath[MAX_PATH];
    CHAR szPath[MAX_PATH];

    THUNKMSG(TEXT("SHILCreateFromPath"));

    if (pszPath)
    {
        if (g_fRunningOnNT)
        {
            SHTCharToUnicode(pszPath, wzPath, ARRAYSIZE(wzPath));
            pszPath = (LPCTSTR)wzPath;  // overload the pointer to pass through...
        }
        else
        {
            SHTCharToAnsi(pszPath, szPath, ARRAYSIZE(szPath));
            pszPath = (LPCTSTR)szPath;  // overload the pointer to pass through...
        }
    }

    return SHILCreateFromPath(pszPath, ppidl, rgfInOut);
}

LPITEMIDLIST _AorW_SHSimpleIDListFromPath(/*IN OPTIONAL*/ LPCTSTR pszPath)
{
    WCHAR wzPath[MAX_PATH];
    CHAR szPath[MAX_PATH];

    THUNKMSG(TEXT("SHSimpleIDListFromPath"));

    if (pszPath)
    {
        if (g_fRunningOnNT)
        {
            SHTCharToUnicode(pszPath, wzPath, ARRAYSIZE(wzPath));
            pszPath = (LPCTSTR)wzPath;  // overload the pointer to pass through...
        }
        else
        {
            SHTCharToAnsi(pszPath, szPath, ARRAYSIZE(szPath));
            pszPath = (LPCTSTR)szPath;  // overload the pointer to pass through...
        }
    }

    return SHSimpleIDListFromPath(pszPath);
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


#define ISNOT_RESOURCE(pItem)      ((pItem) && HIWORD((pItem)) && LOWORD((pItem)))

int FindDoubleTerminator(LPCTSTR pszStr)
{
    int nIndex = 1;

    // Find the double terminator
    while (pszStr[nIndex] || pszStr[nIndex-1])
        nIndex++;

    return nIndex;
}

#define TEMP_SMALL_BUF_SZ  256

BOOL WINAPI _AorW_GetFileNameFromBrowse(HWND hwnd, /*IN OUT*/ LPTSTR pszFilePath, UINT cchFilePath,
        /*IN OPTIONAL*/ LPCTSTR pszWorkingDir, /*IN OPTIONAL*/ LPCTSTR pszDefExt, 
        /*IN OPTIONAL*/ LPCTSTR pszFilters, /*IN OPTIONAL*/ LPCTSTR pszTitle)
{
    WCHAR wszPath[MAX_PATH];
    WCHAR wszDir[MAX_PATH];
    WCHAR wszExt[TEMP_SMALL_BUF_SZ];
    WCHAR wszTitle[TEMP_SMALL_BUF_SZ];

#ifndef UNICODE
    WCHAR wszFilters[TEMP_SMALL_BUF_SZ*2];
#else // UNICODE
    CHAR szFilters[TEMP_SMALL_BUF_SZ*2];
#endif // UNICODE

    CHAR szPath[MAX_PATH];
    CHAR szDir[MAX_PATH];
    CHAR szExt[TEMP_SMALL_BUF_SZ];
    CHAR szTitle[TEMP_SMALL_BUF_SZ];
    BOOL    bResult;
    THUNKMSG(TEXT("GetFileNameFromBrowse"));

    // thunk strings to unicode 
    if (g_fRunningOnNT)
    {
        // always move pszFilePath stuff to wszPath buffer. Should never be a resourceid.
        SHTCharToUnicode(pszFilePath, wszPath, ARRAYSIZE(wszPath));
        pszFilePath = (LPTSTR)wszPath;

        if (ISNOT_RESOURCE(pszWorkingDir)) //not a resource
        {
            SHTCharToUnicode(pszWorkingDir, wszDir, ARRAYSIZE(wszDir));
            pszWorkingDir = (LPCTSTR)wszDir;
        }
        if (ISNOT_RESOURCE(pszDefExt)) //not a resource
        {
            SHTCharToUnicode(pszDefExt, wszExt, ARRAYSIZE(wszExt));
            pszDefExt = (LPCTSTR)wszExt;
        }
        if (ISNOT_RESOURCE(pszFilters)) //not a resource
        {
#ifndef UNICODE
            int nIndex = FindDoubleTerminator(pszFilters);

            // nIndex+1 looks like bunk unless it goes past the terminator
            MultiByteToWideChar(CP_ACP, 0, (LPCTSTR)pszFilters, nIndex+1, wszFilters, ARRAYSIZE(wszFilters));
            pszFilters = (LPCTSTR)wszFilters;
#endif // UNICODE
        }
        if (ISNOT_RESOURCE(pszTitle)) //not a resource
        {
            SHTCharToUnicode(pszTitle, wszTitle, ARRAYSIZE(wszTitle));
            pszTitle = (LPCTSTR)wszTitle;
        }
    }
    else
    {
        // always move pszFilePath stuff to wszPath buffer. Should never be a resourceid.
        SHTCharToAnsi(pszFilePath, szPath, ARRAYSIZE(szPath));
        pszFilePath = (LPTSTR)szPath;

        if (ISNOT_RESOURCE(pszWorkingDir)) //not a resource
        {
            SHTCharToAnsi(pszWorkingDir, szDir, ARRAYSIZE(szDir));
            pszWorkingDir = (LPCTSTR)szDir;
        }
        if (ISNOT_RESOURCE(pszDefExt)) //not a resource
        {
            SHTCharToAnsi(pszDefExt, szExt, ARRAYSIZE(szExt));
            pszDefExt = (LPCTSTR)szExt;
        }
        if (ISNOT_RESOURCE(pszFilters)) //not a resource
        {
#ifdef UNICODE
            int nIndex = FindDoubleTerminator(pszFilters);

            // nIndex+1 looks like bunk unless it goes past the terminator
            WideCharToMultiByte(CP_ACP, 0, (LPCTSTR)pszFilters, nIndex+1, szFilters, ARRAYSIZE(szFilters), NULL, NULL);
            pszFilters = (LPCTSTR)szFilters;
#endif // UNICODE
        }
        if (ISNOT_RESOURCE(pszTitle)) //not a resource
        {
            SHTCharToAnsi(pszTitle, szTitle, ARRAYSIZE(szTitle));
            pszTitle = (LPCTSTR)szTitle;
        }
    }

    bResult = GetFileNameFromBrowse(hwnd, pszFilePath, cchFilePath, pszWorkingDir, pszDefExt, pszFilters, pszTitle);

    // thunk string back to multibyte
    if (g_fRunningOnNT)
        SHUnicodeToTChar(wszPath, pszFilePath, cchFilePath);
    else
        SHAnsiToTChar(szPath, pszFilePath, cchFilePath);

    return bResult;
}

DWORD Win95_GetProcessDword(DWORD idProcess, LONG iIndex)
{
    if (EVAL(!g_fRunningOnNT))
    {
        // Normally we'd just GetProcAddress from KERNEL32.  However,
        // kernel prohibits anyone from getting a no-named export from
        // itself.  So we've added a no-named export to SHELL32 which
        // will call directly into GetProcessDword on the Win95 platform.
        return SHGetProcessDword(idProcess, iIndex);
    }
    return 0;
}


BOOL _AorW_Win32DeleteFile(/*IN*/ LPCTSTR pszFileName)
{
    WCHAR wzPath[MAX_PATH];
    CHAR szPath[MAX_PATH];

    THUNKMSG(TEXT("Win32DeleteFile"));
    if (g_fRunningOnNT)
    {
        SHTCharToUnicode(pszFileName, wzPath, ARRAYSIZE(wzPath));
        pszFileName = (LPCTSTR)wzPath;  // overload the pointer to pass through...
    }
    else
    {
        SHTCharToAnsi(pszFileName, szPath, ARRAYSIZE(szPath));
        pszFileName = (LPCTSTR)szPath;  // overload the pointer to pass through...
    }

    return Win32DeleteFile(pszFileName);
}

BOOL _AorW_PathYetAnotherMakeUniqueName(LPTSTR pszUniqueName,
                                        LPCTSTR pszPath,
                                        LPCTSTR pszShort,
                                        LPCTSTR pszFileSpec)
{
    THUNKMSG(TEXT("PathYetAnotherMakeUniqueName"));
    if (UseUnicodeShell32())
    {
        WCHAR wszUniqueName[MAX_PATH];
        WCHAR wszPath[MAX_PATH];
        WCHAR wszShort[32];
        WCHAR wszFileSpec[MAX_PATH];
        BOOL fRet;

        SHTCharToUnicode(pszPath, wszPath, ARRAYSIZE(wszPath));
        pszPath = (LPCTSTR)wszPath;  // overload the pointer to pass through...

        if (pszShort)
        {
            SHTCharToUnicode(pszShort, wszShort, ARRAYSIZE(wszShort));
            pszShort = (LPCTSTR)wszShort;  // overload the pointer to pass through...
        }

        if (pszFileSpec)
        {
            SHTCharToUnicode(pszFileSpec, wszFileSpec, ARRAYSIZE(wszFileSpec));
            pszFileSpec = (LPCTSTR)wszFileSpec;  // overload the pointer to pass through...
        }

        fRet = PathYetAnotherMakeUniqueName((LPTSTR)wszUniqueName, pszPath, pszShort, pszFileSpec);
        if (fRet)
            SHUnicodeToTChar(wszUniqueName, pszUniqueName, MAX_PATH);

        return fRet;
    }
    else
    {
        CHAR szUniqueName[MAX_PATH];
        CHAR szPath[MAX_PATH];
        CHAR szShort[32];
        CHAR szFileSpec[MAX_PATH];
        BOOL fRet;

        SHTCharToAnsi(pszPath, szPath, ARRAYSIZE(szPath));
        pszPath = (LPCTSTR)szPath;  // overload the pointer to pass through...

        if (pszShort)
        {
            SHTCharToAnsi(pszShort, szShort, ARRAYSIZE(szShort));
            pszShort = (LPCTSTR)szShort;  // overload the pointer to pass through...
        }

        if (pszFileSpec)
        {
            SHTCharToAnsi(pszFileSpec, szFileSpec, ARRAYSIZE(szFileSpec));
            pszFileSpec = (LPCTSTR)szFileSpec;  // overload the pointer to pass through...
        }

        fRet = PathYetAnotherMakeUniqueName((LPTSTR)szUniqueName, pszPath, pszShort, pszFileSpec);
        if (fRet)
            SHAnsiToTChar(szUniqueName, pszUniqueName, MAX_PATH);

        return fRet;
    }
}

BOOL _AorW_PathResolve(/*IN OUT*/ LPTSTR pszPath, /*IN OPTIONAL*/ LPCTSTR rgpszDirs[], UINT fFlags)
{
    THUNKMSG(TEXT("PathResolve"));
    if (g_fRunningOnNT)
    {
        WCHAR wzPath[MAX_PATH];
        WCHAR wzDir[MAX_PATH];
        BOOL fRet;
        // BUGBUG!!!
        // Super Hack, we assume dirs has only one element since it's the only case
        // this is called in SHDOCVW.

        SHTCharToUnicode(pszPath, wzPath, ARRAYSIZE(wzPath));

        if (rgpszDirs && rgpszDirs[0])
        {
            SHTCharToUnicode(rgpszDirs[0], wzDir, ARRAYSIZE(wzDir));
            rgpszDirs[0] = (LPCTSTR)wzDir;  // overload the pointer to pass through...

            if (rgpszDirs[1])
            {
                AssertMsg(0, TEXT("PathResolve thunk needs to be fixed to handle more than one dirs."));
                rgpszDirs[1] = NULL;
            }
        }

        fRet = PathResolve((LPTSTR)wzPath, rgpszDirs, fFlags);
        if (fRet)
            SHUnicodeToTChar(wzPath, pszPath, MAX_PATH);

        return fRet;
    }
    else
    {
        CHAR szPath[MAX_PATH];
        CHAR szDir[MAX_PATH];
        BOOL fRet;
        // BUGBUG!!!
        // Super Hack, we assume dirs has only one element since it's the only case
        // this is called in SHDOCVW.

        SHTCharToAnsi(pszPath, szPath, ARRAYSIZE(szPath));

        if (rgpszDirs && rgpszDirs[0])
        {
            SHTCharToAnsi(rgpszDirs[0], szDir, ARRAYSIZE(szDir));
            rgpszDirs[0] = (LPCTSTR)szDir;  // overload the pointer to pass through...

            if (rgpszDirs[1])
            {
                AssertMsg(0, TEXT("PathResolve thunk needs to be fixed to handle more than one dirs."));
                rgpszDirs[1] = NULL;
            }
        }

        fRet = PathResolve((LPTSTR)szPath, rgpszDirs, fFlags);
        if (fRet)
            SHAnsiToTChar(szPath, pszPath, MAX_PATH);

        return fRet;
    }
}


#ifndef UNIX

// Explicit prototype because only the A/W prototypes exist in the headers
BOOL IsLFNDrive(LPCTSTR pszPath);

#else

#ifdef UNICODE
#define IsLFNDrive IsLFNDriveW
#else
#define IsLFNDrive IsLFNDriveA
#endif

#endif


BOOL _AorW_IsLFNDrive(/*IN*/ LPTSTR pszPath)
{
    THUNKMSG(TEXT("IsLFNDrive"));

    if (g_fRunningOnNT)
    {
        WCHAR wszPath[MAX_PATH];

        SHTCharToUnicode(pszPath, wszPath, ARRAYSIZE(wszPath));
        return IsLFNDrive((LPTSTR)wszPath);
    }
    else
    {
        CHAR szPath[MAX_PATH];

        SHTCharToAnsi(pszPath, szPath, ARRAYSIZE(szPath));
        return IsLFNDrive((LPTSTR)szPath);
    }
}


int _AorW_PickIconDlg(
    IN     HWND  hwnd, 
    IN OUT LPTSTR pszIconPath, 
    IN     UINT  cchIconPath, 
    IN OUT int * piIconIndex)
{
    int nRet;
    WCHAR wszPath[MAX_PATH];
    CHAR szPath[MAX_PATH];
    LPTSTR psz = pszIconPath;
    UINT cch = cchIconPath;
    
    if (g_fRunningOnNT)
    {
        SHTCharToUnicode(pszIconPath, wszPath, ARRAYSIZE(wszPath));
        psz = (LPTSTR)wszPath;  // overload the pointer to pass through...
        cch = SIZECHARS(wszPath);
    }
    else
    {
        SHTCharToAnsi(pszIconPath, szPath, ARRAYSIZE(wszPath));
        psz = (LPTSTR)szPath;  // overload the pointer to pass through...
        cch = SIZECHARS(szPath);
    }

    nRet = PickIconDlg(hwnd, psz, cch, piIconIndex);

    if (g_fRunningOnNT)
        SHUnicodeToTChar(wszPath, pszIconPath, cchIconPath);
    else
        SHAnsiToTChar(szPath, pszIconPath, cchIconPath);

    return nRet;
}

//
//  Now the thunks that allow us to run on Windows 95.
//
//

//
//  This thunks a unicode string to ANSI, but if it's an ordinal, then
//  we just leave it alone.
//
LPSTR Thunk_UnicodeToAnsiOrOrdinal(LPCWSTR pwsz, LPSTR pszBuf, UINT cchBuf)
{
    if (HIWORD64(pwsz)) {
        SHUnicodeToAnsi(pwsz, pszBuf, cchBuf);
        return pszBuf;
    } else {
        return (LPSTR)pwsz;
    }
}

#define THUNKSTRING(pwsz, sz) Thunk_UnicodeToAnsiOrOrdinal(pwsz, sz, ARRAYSIZE(sz))


//
//  This function is new for IE4, so on IE3,
//  we emulate (poorly) with ExtractIcon.
//

//
//  Win95 exported ILCreateFromPathA under the name ILCreateFromPath.
//  Fortunately, NT kept the same ordinal.
//
//
//  If linking with Win95 header files, then call it ILCreateFromPath.
//

#ifdef UNICODE
STDAPI_(LPITEMIDLIST) _ILCreateFromPathA(LPCSTR pszPath)
{
    if (g_fRunningOnNT) {
        WCHAR wszPath[MAX_PATH];
        SHAnsiToUnicode(pszPath, wszPath, ARRAYSIZE(wszPath));
        return ILCreateFromPath((LPVOID)wszPath);
    } else {
        return ILCreateFromPath((LPVOID)pszPath);
    }
}
#else
STDAPI_(LPITEMIDLIST) _ILCreateFromPathW(LPCWSTR pszPath)
{
    if (g_fRunningOnNT) {
        return ILCreateFromPath((LPVOID)pszPath);
    } else {
        CHAR szPath[MAX_PATH];
        SHUnicodeToAnsi(pszPath, szPath, ARRAYSIZE(szPath));
        return ILCreateFromPath((LPVOID)szPath);
    }
}
#endif

        
STDAPI_(int) _AorW_SHCreateDirectory(HWND hwnd, LPCTSTR pszPath)
{
    if (g_fRunningOnNT)
    {
        WCHAR wsz[MAX_PATH];

        SHTCharToUnicode(pszPath, wsz, ARRAYSIZE(wsz));
        return SHCreateDirectory(hwnd, (LPCTSTR)wsz);
    }
    else
    {
        CHAR  sz[MAX_PATH];

        SHTCharToAnsi(pszPath, sz, ARRAYSIZE(sz));
        return SHCreateDirectory(hwnd, (LPCTSTR)sz);
    }
}

#ifdef UNICODE

//
//  Either ptsz1 or ptsz2 can be NULL, so be careful when thunking.
//
STDAPI_(int) _AorW_ShellAbout(HWND hWnd, LPCTSTR ptsz1, LPCTSTR ptsz2, HICON hIcon)
{
    if (g_fRunningOnNT)
    {
        return ShellAboutW(hWnd, ptsz1, ptsz2, hIcon);
    }
    else
    {
        CHAR  sz1[MAX_PATH], sz2[MAX_PATH];
        LPSTR psz1, psz2;

        if (ptsz1) {
            psz1 = sz1;
            SHTCharToAnsi(ptsz1, sz1, ARRAYSIZE(sz1));
        } else {
            psz1 = NULL;
        }

        if (ptsz2) {
            psz2 = sz2;
            SHTCharToAnsi(ptsz2, sz2, ARRAYSIZE(sz2));
        } else {
            psz2 = NULL;
        }

        return ShellAboutA(hWnd, psz1, psz2, hIcon);
    }
}

#endif



