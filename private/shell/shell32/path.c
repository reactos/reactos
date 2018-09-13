#include "shellprv.h"
#pragma  hdrstop

//
// Inline function to check for a double-backslash at the
// beginning of a string
//

__inline BOOL DBL_BSLASH(LPNCTSTR psz)
{
    return (psz[0] == TEXT('\\') && psz[1] == TEXT('\\'));
}


#define IsPathSep(ch)  ((ch) == TEXT('\\') || (ch) == TEXT('/'))

// from mtpt.cpp
STDAPI_(BOOL) CMtPt_IsLFN(int iDrive);
STDAPI_(BOOL) CMtPt_IsSlow(int iDrive);

//----------------------------------------------------------------------------
// in:
//      pszPath         fully qualified path (unc or x:\) to test
//                      NULL for windows directory
//
// returns:
//      TRUE            volume supports name longer than 12 chars
//                      BUGBUG: and UNICODE on disk to avoid netware
//                      character space problems, remove this!
//
// note: this caches drive letters, but UNCs go through every time
//
// BUGBUG: should be PathIsLFN() to test LFNness of the volume that
// the path refers to.

STDAPI_(BOOL) IsLFNDrive(LPCTSTR pszPath)        OPTIONAL
{
    TCHAR szRoot[MAX_PATH];
    DWORD dwMaxLength = 13;      // assume yes

    ASSERT(NULL == pszPath || IS_VALID_STRING_PTR(pszPath, MAX_PATH));

    if (pszPath == NULL)
    {
        GetWindowsDirectory(szRoot, ARRAYSIZE(szRoot));
        pszPath = szRoot;
    }

    ASSERT(!PathIsRelative(pszPath));

    //
    // UNC name? gota check each time
    //
    if (PathIsUNC(pszPath))
    {
        lstrcpyn(szRoot, pszPath, ARRAYSIZE(szRoot));
        PathStripToRoot(szRoot);

        // Deal with busted kernel UNC stuff
        // Is it a \\foo or a \\foo\bar thing?

        if (StrChr(szRoot+2, TEXT('\\')))
        {
            // "\\foo\bar - Append a slash to be NT compatible.
            lstrcat(szRoot, TEXT("\\"));
        }
        else
        {
            // "\\foo" - assume it's always a LFN volume
            return TRUE;
        }
    }
    //
    // removable media? gota check each time
    //
    else if (IsRemovableDrive(DRIVEID(pszPath)))
    {
        PathBuildRoot(szRoot, DRIVEID(pszPath));
    }
    //
    // fixed media use cached value.
    //
    else
    {
        return CMtPt_IsLFN(DRIVEID(pszPath));
    }

    //
    // Right now we will say that it is an LFN Drive if the maximum
    // component is > 12
    GetVolumeInformation(szRoot, NULL, 0, NULL, &dwMaxLength, NULL, NULL, 0);
    return dwMaxLength > 12;
}


#ifdef UNICODE

STDAPI_(BOOL) IsLFNDriveA(LPCSTR pszPath)   OPTIONAL
{
    WCHAR wsz[MAX_PATH];

    ASSERT(NULL == pszPath || IS_VALID_STRING_PTRA(pszPath, MAX_PATH));

    if (pszPath)
    {
        SHAnsiToUnicode(pszPath, wsz, SIZECHARS(wsz));
        pszPath = (LPCSTR)wsz;
    }
    return IsLFNDrive((LPCWSTR)pszPath);
}   
 
#else

STDAPI_(BOOL) IsLFNDriveW(LPCWSTR pszPath)  OPTIONAL
{
    char sz[MAX_PATH];

    ASSERT(NULL == pszPath || IS_VALID_STRING_PTRW(pszPath, MAX_PATH));

    if (pszPath)
    {
        SHUnicodeToAnsi(pszPath, sz, SIZECHARS(sz));
        pszPath = (LPCWSTR)sz;
    }
    return IsLFNDrive((LPCSTR)pszPath);
}   

#endif  // UNICODE



// BUGBUG, we should validate the sizes of all path buffers by filing them
// with MAX_PATH fill bytes.

STDAPI_(BOOL) PathIsRemovable(LPCTSTR pszPath)
{
    BOOL fIsEjectable = FALSE;
    int iDrive = PathGetDriveNumber(pszPath);

    if (iDrive != -1)
    {
        int nType = DriveType(iDrive);

        if ((DRIVE_CDROM == nType) ||
            (DRIVE_DVD == nType) ||
            (DRIVE_REMOVABLE == nType))
        {
            fIsEjectable = TRUE;
        }
    }

    return fIsEjectable;
}


//----------------------------------------------------------------------------
// The following are creterias we currently use to tell whether a file is a temporary file
// 1. Files in Windows temp directory
// 2. Files with FILE_ATTRIBUTE_TEMPORARY set
// 3  Files from the internet cache directory
//---------------------------------------------------------------------------
STDAPI_(BOOL) PathIsTemporary(LPCTSTR pszPath)
{
    TCHAR szTempPath[MAX_PATH];
    TCHAR szShort[MAX_PATH];
    DWORD dwAttrib = GetFileAttributes(pszPath);
    if (dwAttrib == (DWORD)-1)
        return FALSE;
    
    if (dwAttrib & FILE_ATTRIBUTE_TEMPORARY)
        return TRUE;

    GetTempPath(ARRAYSIZE(szTempPath), szTempPath);

    //  temp path comes back as a short name because
    //  of compatibility issues.  therefore, we will
    //  shorten the incoming buffer to be like IKE
    GetShortPathName(pszPath, szShort, SIZECHARS(szShort));
    
    return PathIsEqualOrSubFolder(szTempPath, szShort) 
        || PathIsEqualOrSubFolder(MAKEINTRESOURCE(CSIDL_INTERNET_CACHE), pszPath);
}


#ifdef UNICODE 
STDAPI_(BOOL) PathIsTemporaryA(LPCSTR pszPath)
#else // !UNICODE
STDAPI_(BOOL) PathIsTemporaryW(LPCWSTR pszPath)
#endif // UNICODE
{
    TCHAR szPath[MAX_PATH];

    SHOtherToTChar(pszPath, szPath, SIZECHARS(szPath));

    return PathIsTemporary(szPath);
}


// unfortunately, this is exported so we need to support it
STDAPI_(LPTSTR) PathGetExtension(LPCTSTR pszPath, LPTSTR pszExtension, int cchExt)
{
    LPTSTR pszExt = PathFindExtension(pszPath);

    ASSERT(pszExtension==NULL);        // we dont handle this case.

    return *pszExt ? pszExt + 1 : pszExt;
}


// PathTruncateKeepExtension
//
// Attempts to truncate the filename pszSpec such that pszDir+pszSpec are less than MAX_PATH-5.
// The extension is protected so it won't get truncated or altered.
//
// in:
//      pszDir      the path to a directory.  No trailing '\' is needed.
//      pszSpec     the filespec to be truncated.  This should not include a path but can have an extension.
//                  This input buffer can be of any length.
//      iTruncLimit The minimum length to truncate pszSpec.  If addition truncation would be required we fail.
// out:
//      pszSpec     The truncated filespec with it's extension unaltered.
// return:
//      TRUE if the filename was truncated, FALSE if we were unable to truncate because the directory name
//      was too long, the extension was too long, or the iTruncLimit is too high.  pszSpec is unaltered
//      when this function returns FALSE.
//
STDAPI_(BOOL) PathTruncateKeepExtension( LPCTSTR pszDir, LPTSTR pszSpec, int iTruncLimit )
{
    LPTSTR pszExt = PathFindExtension(pszSpec);
    int cchExt = lstrlen(pszExt);
    int cchSpec = (int)(pszExt - pszSpec + cchExt);
    int cchKeep = MAX_PATH-lstrlen(pszDir)-5;   // the -5 is just to provide extra padding

    // IF...
    //  ...the filename is to long
    //  ...we are within the limit to which we can truncate
    //  ...the extension is short enough to allow the trunctation
    if ( (cchSpec > cchKeep) && (cchKeep >= iTruncLimit) && (cchKeep > cchExt) )
    {
        // THEN... go ahead and truncate
        StrCpy( pszSpec+cchKeep-cchExt, pszExt );
        return TRUE;
    }
    return FALSE;
}


STDAPI_(int) PathCleanupSpec(LPCTSTR pszDir, LPTSTR pszSpec)
{
    LPTSTR pszNext, pszCur;
    UINT   uMatch = IsLFNDrive(pszDir) ? GCT_LFNCHAR : GCT_SHORTCHAR;
    int    iRet = 0;
    LPTSTR pszPrevDot = NULL;

    for (pszCur = pszNext = pszSpec; *pszNext; pszNext = CharNext(pszNext))
    {
        if (PathGetCharType(*pszNext) & uMatch)
        {
            *pszCur = *pszNext;
            if (uMatch == GCT_SHORTCHAR && *pszCur == TEXT('.'))
            {
                if (pszPrevDot)    // Only one '.' allowed for short names
                {
                    *pszPrevDot = TEXT('-');
                    iRet |= PCS_REPLACEDCHAR;
                }
                pszPrevDot = pszCur;
            }
            if (IsDBCSLeadByte(*pszNext))
                *(pszCur + 1) = *(pszNext + 1);
            pszCur = CharNext(pszCur);
        }
        else
        {
            switch (*pszNext)
            {
            case TEXT('/'):         // used often for things like add/remove
            case TEXT(' '):         // blank (only replaced for short name drives)
               *pszCur = TEXT('-');
               pszCur = CharNext(pszCur);
               iRet |= PCS_REPLACEDCHAR;
               break;
            default:
               iRet |= PCS_REMOVEDCHAR;
            }
        }
    }
    *pszCur = 0;     // null terminate

    //
    //  For short names, limit to 8.3
    //
    if (uMatch == GCT_SHORTCHAR)
    {
        int i = 8;
        for (pszCur = pszNext = pszSpec; *pszNext; pszNext = CharNext(pszNext))
        {
            if (*pszNext == TEXT('.'))
            {
                i = 4; // Copy "." + 3 more characters
            }
            if (i > 0)
            {
                *pszCur = *pszNext;
                pszCur = CharNext(pszCur);
                i--;
            }
            else
            {
                iRet |= PCS_TRUNCATED;
            }
        }
        *pszCur = 0;
        CharUpperNoDBCS(pszSpec);
    }
    else    // Path too long only possible on LFN drives
    {
        if (pszDir && (lstrlen(pszDir) + lstrlen(pszSpec) > MAX_PATH - 1))
        {
            iRet |= PCS_PATHTOOLONG | PCS_FATAL;
        }
    }
    return(iRet);
}


// PathCleanupSpecEx
//
// Just like PathCleanupSpec, PathCleanupSpecEx removes illegal characters from pszSpec
// and enforces 8.3 format on non-LFN drives.  In addition, this function will attempt to
// truncate pszSpec if the combination of pszDir + pszSpec is greater than MAX_PATH.
//
// in:
//      pszDir      The directory in which the filespec pszSpec will reside
//      pszSpec     The filespec that is being cleaned up which includes any extension being used
// out:
//      pszSpec     The modified filespec with illegal characters removed, truncated to
//                  8.3 if pszDir is on a non-LFN drive, and truncated to a shorter number
//                  of characters if pszDir is an LFN drive but pszDir + pszSpec is more
//                  than MAX_PATH characters.
// return:
//      returns a bit mask indicating what happened.  This mask can include the following cases:
//          PCS_REPLACEDCHAR    One or more illegal characters were replaced with legal characters
//          PCS_REMOVEDCHAR     One or more illegal characters were removed
//          PCS_TRUNCATED       Truncated to fit 8.3 format or because pszDir+pszSpec was too long
//          PCS_PATHTOOLONG     pszDir is so long that we cannot truncate pszSpec to form a legal filename
//          PCS_FATAL           The resultant pszDir+pszSpec is not a legal filename.  Always used with PCS_PATHTOOLONG.
//
STDAPI_(int) PathCleanupSpecEx(LPCTSTR pszDir, LPTSTR pszSpec)
{
    int iRet = 0;

    iRet = PathCleanupSpec(pszDir, pszSpec);
    if ( iRet & (PCS_PATHTOOLONG|PCS_FATAL) )
    {
        // 30 is the shortest we want to truncate pszSpec to to satisfy the
        // pszDir+pszSpec<MAX_PATH requirement.  If this amount of truncation isn't enough
        // then we go ahead and return PCS_PATHTOOLONG|PCS_FATAL without doing any further
        // truncation of pszSpec
        if ( PathTruncateKeepExtension(pszDir, pszSpec, 30 ) )
        {
            // We fixed the error returned by PathCleanupSpec so mask out the error.
            iRet |= PCS_TRUNCATED;
            iRet &= ~(PCS_PATHTOOLONG|PCS_FATAL);
        }
    }
    else
    {
        // ensure that if both of these aren't set then neither is set.
        ASSERT( !(iRet&PCS_PATHTOOLONG) && !(iRet&PCS_FATAL) );
    }

    return(iRet);
}


STDAPI_(BOOL) PathIsWild(LPCTSTR pszPath)
{
    while (*pszPath) 
    {
        if (*pszPath == TEXT('?') || *pszPath == TEXT('*'))
            return TRUE;
        pszPath = CharNext(pszPath);
    }
    return FALSE;
}


// given a path that potentially points to an un-extensioned program
// file, check to see if a program file exists with that name.
//
// returns: TRUE if a program with that name is found.
//               (extension is added to name).
//          FALSE no program file found or the path did not have an extension
//
BOOL LookForExtensions(LPTSTR pszPath, LPCTSTR dirs[], BOOL bPathSearch, UINT fExt)
{
    ASSERT(fExt);       // should have some bits set

    if (*PathFindExtension(pszPath) == 0)
    {
        if (bPathSearch)
        {
            // NB Try every extension on each path component in turn to
            // mimic command.com's search order.
            return PathFindOnPathEx(pszPath, dirs, fExt);
        }
        else
        {
            return PathFileExistsDefExt(pszPath, fExt);
        }
    }
    return FALSE;
}


//
// converts the relative or unqualified path name to the fully
// qualified path name.
//
// If this path is a URL, this function leaves it alone and
// returns FALSE.
//
// in:
//      pszPath        path to convert
//      lpszCurrentDir  current directory to use
//
//  PRF_TRYPROGRAMEXTENSIONS (implies PRF_VERIFYEXISTS)
//  PRF_VERIFYEXISTS
//
// returns:
//      TRUE    the file was verified to exist
//      FALSE   the file was not verified to exist (but it may)
//
STDAPI_(BOOL) PathResolve(LPTSTR lpszPath, LPCTSTR dirs[], UINT fFlags)
{
    UINT fExt = (fFlags & PRF_DONTFINDLNK) ? (PFOPEX_COM | PFOPEX_BAT | PFOPEX_PIF | PFOPEX_EXE) : PFOPEX_DEFAULT;

    //
    //  NOTE:  if VERIFY SetLastError() default to FNF.  - ZekeL 9-APR-98
    //  ShellExec uses GLE() to find out why we failed.  
    //  any win32 API that we end up calling
    //  will do a SLE() to overrider ours.  specifically
    //  if VERIFY is set we call GetFileAttributes() 
    //
    if (fFlags & PRF_VERIFYEXISTS)
        SetLastError(ERROR_FILE_NOT_FOUND);
    
    PathUnquoteSpaces(lpszPath);

    if (PathIsRoot(lpszPath))
    {
        // No sense qualifying just a server or share name...
        if (!PathIsUNCServer(lpszPath) && !PathIsUNCServerShare(lpszPath))
        {
            // Be able to resolve "\" from different drives.
            if (lpszPath[0] == TEXT('\\') && lpszPath[1] == TEXT('\0') )
            {
                PathQualifyDef(lpszPath, fFlags & PRF_FIRSTDIRDEF ? dirs[0] : NULL, 0);
            }
        }

        if (fFlags & PRF_VERIFYEXISTS)
        {
            if (PathFileExistsAndAttributes(lpszPath, NULL))
            {
                return(TRUE);
            }
#ifdef DEBUG
            // BUGBUG: there is a better way to do this...
            //   PathFileExistsAndAttributes() should catch this well enough.
            // If it is a UNC root, then we will see if the root exists
            //
            if (PathIsUNC(lpszPath))
            {
                // See if the network knows about this one.
                // It appears like some network provider croak if not everything
                // if filled in, so we might as well bloat ourself to make them happy...
                NETRESOURCE nr = {RESOURCE_GLOBALNET,RESOURCETYPE_ANY,
                        RESOURCEDISPLAYTYPE_GENERIC, RESOURCEUSAGE_CONTAINER,
                        NULL, lpszPath, NULL, NULL};
                HANDLE hEnum;

                if (WNetOpenEnum(RESOURCE_GLOBALNET, RESOURCETYPE_ANY,
                        RESOURCEUSAGE_ALL, &nr, &hEnum) == WN_SUCCESS)
                {
                    // If it succeeded then assume it worked...
                    WNetCloseEnum(hEnum);
                    ASSERT(FALSE);
                    return(TRUE);
                }
            }
#endif // DEBUG

            return(FALSE);
        }

        return TRUE;

    }
    else if (PathIsFileSpec(lpszPath))
    {

        // REVIEW: look for programs before looking for paths

        if ((fFlags & PRF_TRYPROGRAMEXTENSIONS) && (LookForExtensions(lpszPath, dirs, TRUE, fExt)))
            return TRUE;

        if (PathFindOnPath(lpszPath, dirs))
        {
            // PathFindOnPath() returns TRUE iff PathFileExists(lpszPath),
            // so we always returns true here:
            //return (!(fFlags & PRF_VERIFYEXISTS)) || PathFileExists(lpszPath);
            return TRUE;
        }

    }
    else if (!PathIsURL(lpszPath))
    {
        // If there is a trailing '.', we should not try extensions
        PathQualifyDef(lpszPath, fFlags & PRF_FIRSTDIRDEF ? dirs[0] : NULL,
                PQD_NOSTRIPDOTS);
        if (fFlags & PRF_VERIFYEXISTS)
        {
            if ((fFlags & PRF_TRYPROGRAMEXTENSIONS) && (LookForExtensions(lpszPath, dirs, FALSE, fExt)))
                return TRUE;

            if (PathFileExistsAndAttributes(lpszPath, NULL))
                return TRUE;
        }
        else
        {
            return TRUE;
        }

    }
    return FALSE;
}


// qualify a DOS (or LFN) file name based on the currently active window.
// this code is not careful to not write more than MAX_PATH characters
// into psz
//
// in:
//      psz     path to be qualified of at least MAX_PATH characters
//              ANSI string
//
// out:
//      psz     fully qualified version of input string based
//              on the current active window (current directory)
//

void PathQualifyDef(LPTSTR psz, LPCTSTR szDefDir, DWORD dwFlags)
{
    int cb, nSpaceLeft;
    TCHAR szTemp[MAX_PATH], szRoot[MAX_PATH];
    int iDrive;
    LPTSTR pOrig, pFileName;
    BOOL fLFN;
    LPTSTR pExt;
    
    /* Save it away. */
    lstrcpyn(szTemp, psz, ARRAYSIZE(szTemp));
    
    FixSlashesAndColon(szTemp);
    
    nSpaceLeft = ARRAYSIZE(szTemp);
    
    pOrig = szTemp;
    pFileName = PathFindFileName(szTemp);
    
    if (PathIsUNC(pOrig))
    {
        // leave the \\ in thebuffer so that the various parts
        // of the UNC path will be qualified and appended.  Note
        // we must assume that UNCs are LFN's, since computernames
        // and sharenames can be longer than 11 characters.
        fLFN = IsLFNDrive(pOrig);
        if (fLFN)
        {
            psz[2] = 0;
            nSpaceLeft -= 3;
            pOrig += 2;
            goto GetComps;
        }
        else
        {
            // NB UNC doesn't support LFN's but we don't want to truncate
            // \\foo or \\foo\bar so skip them here.
            
            // Is it a \\foo\bar\fred thing?
            LPTSTR pszSlash = StrChr(psz+2, TEXT('\\'));
            if (pszSlash && (NULL != (pszSlash = StrChr(pszSlash+1, TEXT('\\')))))
            {
                // Yep - skip the first bits but mush the rest.
                *(pszSlash+1) = TEXT('\0');
                nSpaceLeft -= (int)(pszSlash-psz)+1;
                pOrig += pszSlash-psz;
                goto GetComps;
            }
            else
            {
                // Nope - just pretend it's an LFN and leave it alone.
                fLFN = TRUE;
                psz[2] = 0;
                nSpaceLeft -= 2;
                pOrig+=2;
                goto GetComps;
            }
        }
    }
    
    iDrive = PathGetDriveNumber(pOrig);
    if (iDrive != -1)
    {
        PathBuildRoot(szRoot, iDrive);    // root specified by the file name
        
        ASSERT(pOrig[1] == TEXT(':'));    // PathGetDriveNumber does this
        
        pOrig += 2;   // Skip over the drive letter
        
        // and the slash if it is there...
        if (pOrig[0] == TEXT('\\'))
            pOrig++;
    }
    else
    {
        if (szDefDir)
            lstrcpyn(szRoot, szDefDir, ARRAYSIZE(szRoot));
        else
        {
            //
            // As a default, use the windows drive (usually "C:\").
            //
            GetWindowsDirectory(szRoot, ARRAYSIZE(szRoot));
            iDrive = PathGetDriveNumber(szRoot);
            if (iDrive != -1)
            {
                PathBuildRoot(szRoot, iDrive);
            }
        }
        
        // if path is scopped to the root with "\" use working dir root
        
        if (pOrig[0] == TEXT('\\'))
            PathStripToRoot(szRoot);
    }
    fLFN = IsLFNDrive(szRoot);
    
    // REVIEW, do we really need to do different stuff on LFN names here?
    // on FAT devices, replace any illegal chars with underscores
    if (!fLFN)
    {
        LPTSTR pT;
        for (pT = pOrig; *pT; pT = CharNext(pT))
        {
            if (!PathIsValidChar(*pT, PIVC_SFN_FULLPATH))
            {
                // not a valid sfn path character
                *pT = TEXT('_');
            }
        }
    }
    
    lstrcpy(psz, szRoot);
    nSpaceLeft -= (lstrlen(psz) + 1);
    
GetComps:
    
    while (*pOrig && nSpaceLeft > 0)
    {
        // If the component 0is parent dir, go up one dir.
        // If its the current dir, skip it, else add it normally
        if (pOrig[0] == TEXT('.'))
        {
            if (pOrig[1] == TEXT('.') && (!pOrig[2] || pOrig[2] == TEXT('\\')))
                PathRemoveFileSpec(psz);
            else if (pOrig[1] && pOrig[1] != TEXT('\\'))
                goto addcomponent;
            
            while (*pOrig && *pOrig != TEXT('\\'))
                pOrig = CharNext(pOrig);
            
            if (*pOrig)
                pOrig++;
        }
        else
        {
            LPTSTR pT, pTT = NULL;
            
addcomponent:
            PathAddBackslash(psz);
            nSpaceLeft--;
            
            pT = psz + lstrlen(psz);
            
            if (fLFN)
            {
                // copy the component
                while (*pOrig && *pOrig != TEXT('\\') && nSpaceLeft>0)
                {
                    nSpaceLeft--;
                    if (IsDBCSLeadByte(*pOrig))
                    {
                        if (nSpaceLeft <= 0)
                        {
                            // Copy nothing more
                            continue;
                        }
                        
                        nSpaceLeft--;
                        *pT++ = *pOrig++;
                    }
                    *pT++ = *pOrig++;
                }
            }
            else
            {
                // copy the filename (up to 8 chars)
                for (cb = 8; *pOrig && !IsPathSep(*pOrig) && *pOrig != TEXT('.') && nSpaceLeft > 0;)
                {
                    if (cb > 0)
                    {
                        cb--;
                        nSpaceLeft--;
                        if (IsDBCSLeadByte(*pOrig))
                        {
                            if (nSpaceLeft<=0 || cb<=0)
                            {
                                // Copy nothing more
                                cb = 0;
                                continue;
                            }
                            
                            cb--;
                            nSpaceLeft--;
                            *pT++ = *pOrig++;
                        }
                        *pT++ = *pOrig++;
                    }
                    else
                    {
                        pOrig = CharNext(pOrig);
                    }
                }
                
                // if there's an extension, copy it, up to 3 chars
                if (*pOrig == TEXT('.') && nSpaceLeft > 0)
                {
                    int nOldSpaceLeft;
                    
                    *pT++ = TEXT('.');
                    nSpaceLeft--;
                    pOrig++;
                    pExt = pT;
                    nOldSpaceLeft = nSpaceLeft;
                    
                    for (cb = 3; *pOrig && *pOrig != TEXT('\\') && nSpaceLeft > 0;)
                    {
                        if (*pOrig == TEXT('.'))
                        {
                            // Another extension, start again.
                            cb = 3;
                            pT = pExt;
                            nSpaceLeft = nOldSpaceLeft;
                            pOrig++;
                        }
                        
                        if (cb > 0)
                        {
                            cb--;
                            nSpaceLeft--;
                            if (IsDBCSLeadByte(*pOrig))
                            {
                                if (nSpaceLeft<=0 || cb<=0)
                                {
                                    // Copy nothing more
                                    cb = 0;
                                    continue;
                                }
                                
                                cb--;
                                nSpaceLeft--;
                                *pT++ = *pOrig++;
                            }
                            *pT++ = *pOrig++;
                        }
                        else
                        {
                            pOrig = CharNext(pOrig);
                        }
                    }
                }
            }
            
            // skip the backslash
            
            if (*pOrig)
                pOrig++;
            
            // null terminate for next pass...
            *pT = 0;
        }
    }
    
    PathRemoveBackslash(psz);
    
    if (!(dwFlags & PQD_NOSTRIPDOTS))
    {
        // remove any trailing dots
        
        LPTSTR pszPrev = CharPrev(psz, psz + lstrlen(psz));
        if (*pszPrev == TEXT('.'))
        {
            *pszPrev = TEXT('\0');
        }
    }
}


STDAPI_(void) PathQualify(LPTSTR psz)
{
    PathQualifyDef(psz, NULL, 0);
}


BOOL OnExtList(LPCTSTR pszExtList, LPCTSTR pszExt)
{
    for (; *pszExtList; pszExtList += lstrlen(pszExtList) + 1)
    {
        if (!lstrcmpi(pszExt, pszExtList))
        {
            // yes
            return TRUE;        
        }
    }

    return FALSE;
}


#ifdef WINNT
    // Character offset where binary exe extensions begin in above
    #define BINARY_EXE_OFFSET 20
    const TCHAR c_achExes[] = TEXT(".cmd\0.bat\0.pif\0.scf\0.exe\0.com\0.scr\0");
#else
    // Character offset where binary exe extensions begin in above
    #define BINARY_EXE_OFFSET 15
    const TCHAR c_achExes[] = TEXT(".bat\0.pif\0.scf\0.exe\0.com\0.scr\0");
#endif

STDAPI_(BOOL) PathIsBinaryExe(LPCTSTR szFile)
{
    ASSERT(BINARY_EXE_OFFSET < ARRAYSIZE(c_achExes) &&
           c_achExes[BINARY_EXE_OFFSET] == TEXT('.'));

    return OnExtList(c_achExes + BINARY_EXE_OFFSET, PathFindExtension(szFile));
}


//
// determine if a path is a program by looking at the extension
//
STDAPI_(BOOL) PathIsExe(LPCTSTR szFile)
{
    LPCTSTR temp = PathFindExtension(szFile);
    return OnExtList(c_achExes, temp);
}


//
// determine if a path is a .lnk file by looking at the extension
//
STDAPI_(BOOL) PathIsLnk(LPCTSTR szFile)
{
    if (szFile)
    {
        // Both PathFindExtension() and lstrcmpi() will crash
        // if passed NULL.  PathFindExtension() will never return
        // NULL.
        LPCTSTR lpszFileName = PathFindExtension(szFile);
        return lstrcmpi(TEXT(".lnk"), lpszFileName) == 0;
    }
    else
    {
        return FALSE;
    }
}


// BUGBUG: asside from the length check this is bogus.
// there should be a real Win32 way to do this... NT guys?
STDAPI_(BOOL) PathIsInvalid(LPCTSTR pPath)
{
  TCHAR  sz[9];
  int   n = 0;
  // REVIEW, this is the list of dos devices that will cause problems if
  // we try to move/copy/delete/rename.  are there more? (ask aaronr/jeffpar)
  static const LPTSTR aDevices[] = {
    TEXT("CON"),
    TEXT("MS$MOUSE"),
    TEXT("EMMXXXX0"),
    TEXT("CLOCK$")
  };

  // BUGBUG, this should check for invalid chars in the path

  if (lstrlen(pPath) >= MAX_PATH-1)
      return TRUE;

  pPath = PathFindFileName(pPath);

  while (*pPath && *pPath != TEXT('.') && *pPath != TEXT(':') && n < 8)
  {
      if (IsDBCSLeadByte( *pPath ))
      {
        if (n == 7)
          break;
        sz[n++] = *pPath;
      }
      sz[n++] = *pPath++;
  }

  sz[n] = TEXT('\0');

  for (n = 0; n < ARRAYSIZE(aDevices); n++)
  {
    if (!lstrcmpi(sz, aDevices[n]))
    {
        return TRUE;
    }
  }
  return FALSE;
}

//
// Funciton: PathMakeUniqueName
//
// Parameters:
//  pszUniqueName -- Specify the buffer where the unique name should be copied
//  cchMax        -- Specify the size of the buffer
//  pszTemplate   -- Specify the base name
//  pszLongPlate  -- Specify the base name for a LFN drive. format below
//  pszDir        -- Specify the directory
//
// History:
//  03-11-93    SatoNa      Created
//
// REVIEW:
//  For long names, we should be able to generate more user friendly name
//  such as "Copy of MyDocument" of "Link #2 to MyDocument". In this case,
//  we need additional flags which indicates if it is copy, or link.
//
// Format:
// pszLongPlate will search for the first ( and then finds the matching )
// to look for a number:
//    given:  Copy () of my doc       gives:  Copy (_number_) of my doc
//    given:  Copy (1023) of my doc   gives:  Copy (_number_) of my doc

// BUGBUG: if making n unique names, the time grows n^2 because it always
// starts from 0 and checks for existing file.
STDAPI_(BOOL) PathMakeUniqueNameEx(LPTSTR  pszUniqueName,
                               UINT   cchMax,
                               LPCTSTR pszTemplate,
                               LPCTSTR pszLongPlate,
                               LPCTSTR pszDir,
                                 int iMinLong)
{
    BOOL fSuccess=FALSE;
    LPTSTR lpszFormat = pszUniqueName; // use their buffer instead of creating our own
    LPTSTR pszName, pszDigit;
    LPCTSTR pszRest;
    LPCTSTR pszEndUniq;  // End of Uniq sequence part...
    LPCTSTR pszStem;
    int cchRest, cchStem, cchDir, cchMaxName;
    int iMax, iMin, i;
    TCHAR achFullPath[MAX_PATH];

    if (pszLongPlate == NULL)
        pszLongPlate = pszTemplate;

    // this if/else set up lpszFormat and all the other pointers for the
    // sprintf/file_exists loop below
    iMin = iMinLong;
    if (pszLongPlate && IsLFNDrive(pszDir)) {

        cchMaxName = 0;

        // for long name drives
        pszStem = pszLongPlate;
        pszRest = StrChr(pszLongPlate, TEXT('('));
        while (pszRest)
        {
            // First validate that this is the right one
            pszEndUniq = CharNext(pszRest);
            while (*pszEndUniq && *pszEndUniq >= TEXT('0') && *pszEndUniq <= TEXT('9')) {
                pszEndUniq++;
            }
            if (*pszEndUniq == TEXT(')'))
                break;  // We have the right one!
            pszRest = StrChr(CharNext(pszRest), TEXT('('));
        }

        // if no (, punt to short name
        if (!pszRest) {
            // if no (, then tack it on at the end. (but before the extension)
            // eg.  New Link yields New Link (1)
            pszRest = PathFindExtension(pszLongPlate);
            cchStem = (int)(pszRest - pszLongPlate);
            wsprintf(lpszFormat, TEXT(" (%%d)%s"), pszRest ? pszRest : c_szNULL);
            iMax = 100;
        } else {
            pszRest++; // stop over the #

            cchStem = (int) (pszRest - pszLongPlate);

            while (*pszRest && *pszRest >= TEXT('0') && *pszRest <= TEXT('9')) {
                pszRest++;
            }

            // how much room do we have to play?
            switch(cchMax - cchStem - lstrlen(pszRest)) {
                case 0:
                    // no room, bail to short name
                    return PathMakeUniqueName(pszUniqueName, cchMax, pszTemplate, NULL, pszDir);
                case 1:
                    iMax = 10;
                    break;
                case 2:
                    iMax = 100;
                    break;
                default:
                    iMax = 1000;
                    break;
            }

            // we are guaranteed enough room because we don't include
            // the stuff before the # in this format
            wsprintf(lpszFormat, TEXT("%%d%s"), pszRest);
        }

    } else {

        // for short name drives
        pszStem = pszTemplate;
        pszRest = PathFindExtension(pszTemplate);

        cchRest=lstrlen(pszRest)+1;          // 5 for ".foo";
        if (cchRest<5)
            cchRest=5;
        cchStem= (int) (pszRest-pszTemplate);        // 8 for "fooobarr.foo"
        cchDir=lstrlen(pszDir);

        cchMaxName = 8+cchRest-1;

        //
        // Remove all the digit characters from the stem
        //
        for (;cchStem>1; cchStem--)
        {
            TCHAR ch;
            LPCTSTR pszPrev = CharPrev(pszTemplate, pszTemplate + cchStem);
            // Don't remove if it is a DBCS character
            if (pszPrev != pszTemplate+cchStem-1)
                break;

            // Don't remove it it is not a digit
            ch=pszPrev[0];
            if (ch<TEXT('0') || ch>TEXT('9'))
                break;
        }

        //
        // Truncate characters from the stem, if it does not fit.
        // In the case were LFNs are supported we use the cchMax that was passed in
        // but for Non LFN drives we use the 8.3 rule.
        //
        if ((UINT)cchStem > (8-1)) {
            cchStem=8-1;          // Needs to fit into the 8 part of the name
        }

        //
        // We should have at least one character in the stem.
        //
        if (cchStem < 1 || (cchDir+cchStem+1+cchRest+1) > MAX_PATH)
        {
            goto Error;
        }
        wsprintf(lpszFormat, TEXT("%%d%s"), pszRest);
        iMax = 1000; iMin = 1;
    }

    if (pszDir)
    {
        lstrcpy(achFullPath, pszDir);
        PathAddBackslash(achFullPath);
    }
    else
    {
        achFullPath[0] = 0;
    }

    pszName=achFullPath+lstrlen(achFullPath);
    lstrcpyn(pszName, pszStem, cchStem+1);
    pszDigit = pszName + cchStem;

    for (i = iMin; i < iMax ; i++) {

        wsprintf(pszDigit, lpszFormat, i);

        //
        // if we have a limit on the length of the name (ie on a non-LFN drive)
        // backup the pszDigit pointer when i wraps from 9to10 and 99to100 etc
        //
        if (cchMaxName && lstrlen(pszName) > cchMaxName)
        {
            pszDigit = CharPrev(pszName, pszDigit);
            wsprintf(pszDigit, lpszFormat, i);
        }

        TraceMsg(TF_PATH, "PathMakeUniqueNameEx: trying %s", (LPCTSTR)achFullPath);

        //
        // Check if this name is unique or not.
        //
        if (!PathFileExistsAndAttributes(achFullPath, NULL))
        {
            lstrcpyn(pszUniqueName, pszName, cchMax);
            fSuccess=TRUE;
            break;
        }
    }

  Error:
    return fSuccess;
}

STDAPI_(BOOL) PathMakeUniqueName(LPTSTR  pszUniqueName,
                               UINT   cchMax,
                               LPCTSTR pszTemplate,
                               LPCTSTR pszLongPlate,
                               LPCTSTR pszDir)
{
    return PathMakeUniqueNameEx(pszUniqueName, cchMax, pszTemplate, pszLongPlate, pszDir, 1);
}


// in:
//      pszPath         directory to do this into or full dest path
//                      if pszShort is NULL
//      pszShort        file name (short version) if NULL assumes
//                      pszPath is both path and spec
//      pszFileSpec     file name (long version)
//
// out:
//      pszUniqueName
//
// returns:
//      TRUE    success, name can be used

STDAPI_(BOOL) PathYetAnotherMakeUniqueName(LPTSTR  pszUniqueName,
                                         LPCTSTR pszPath,
                                         LPCTSTR pszShort,
                                         LPCTSTR pszFileSpec)
{
    BOOL fRet = FALSE;

    TCHAR szTemp[MAX_PATH];
    TCHAR szPath[MAX_PATH];

    if (pszShort == NULL)
    {
        pszShort = PathFindFileName(pszPath);
        lstrcpy(szPath, pszPath);
        PathRemoveFileSpec(szPath);
        pszPath = szPath;
    }
    if (pszFileSpec == NULL)
    {
        pszFileSpec = pszShort;
    }

    if (IsLFNDrive(pszPath))
    {
        LPTSTR lpsz;
        LPTSTR lpszNew;

        // REVIEW:  If the path+filename is too long how about this, instead of bailing out we trunctate the name
        // using my new PathTruncateKeepExtension?  Currently we have many places where the return result of this
        // function is not checked which cause failures in abserdly long filename cases.  The result ends up having
        // the wrong path which screws things up.
        if ((lstrlen(pszPath) + lstrlen(pszFileSpec) + 5 ) > MAX_PATH)
            return FALSE;

        // try it without the ( if there's a space after it
        lpsz = StrChr(pszFileSpec, TEXT('('));
        while (lpsz)
        {
            if (*(CharNext(lpsz)) == TEXT(')'))
                break;
             lpsz = StrChr(CharNext(lpsz), TEXT('('));
        }

        if (lpsz)
        {
            // We have the ().  See if we have either x () y or x ().y in which case
            // we probably want to get rid of one of the blanks...
            int ichSkip = 2;
            LPTSTR lpszT = CharPrev(pszFileSpec, lpsz);
            if (*lpszT == TEXT(' '))
            {
                ichSkip = 3;
                lpsz = lpszT;
            }

            lstrcpy(szTemp, pszPath);
            lpszNew = PathAddBackslash(szTemp);
            lstrcpy(lpszNew, pszFileSpec);
            lpszNew += (lpsz - pszFileSpec);
            lstrcpy(lpszNew, lpsz + ichSkip);
            fRet = !PathFileExistsAndAttributes(szTemp, NULL);
        }
        else
        {
            // 1taro registers its document with '/'.
            if (lpsz=StrChr(pszFileSpec, '/'))
            {
                LPTSTR lpszT = CharNext(lpsz);
                lstrcpy(szTemp, pszPath);
                lpszNew = PathAddBackslash(szTemp);
                lstrcpy(lpszNew, pszFileSpec);
                lpszNew += (lpsz - pszFileSpec);
                lstrcpy(lpszNew, lpszT);
            }
            else
            {
                PathCombine(szTemp, pszPath, pszFileSpec);
            }
            fRet = !PathFileExistsAndAttributes(szTemp, NULL);
        }
    }
    else
    {
        ASSERT(lstrlen(PathFindExtension(pszShort)) <= 4);

        lstrcpy(szTemp,pszShort);
        PathRemoveExtension(szTemp);

        if (lstrlen(szTemp) <= 8)
        {
            PathCombine(szTemp, pszPath, pszShort);
            fRet = !PathFileExistsAndAttributes(szTemp, NULL);
        }
    }

    if (!fRet)
    {
        fRet =  PathMakeUniqueNameEx(szTemp, ARRAYSIZE(szTemp), pszShort, pszFileSpec, pszPath, 2);
        PathCombine(szTemp, pszPath, szTemp);
    }

    if (fRet)
        lstrcpy(pszUniqueName, szTemp);

    return fRet;
}


STDAPI_(void) PathGetShortPath(LPTSTR pszLongPath)
{
    TCHAR szShortPath[MAX_PATH];
    if (GetShortPathName(pszLongPath, szShortPath, ARRAYSIZE(szShortPath)))
        lstrcpy(pszLongPath, szShortPath);
}


//
// PathIsHighLatency()
//  pszFile    -- file path
//  dwFileAttr -- The file attributes, pass -1 if not available
//
//  Note: pszFile arg may be NULL if dwFileAttr != -1.

STDAPI_(BOOL) PathIsHighLatency(LPCTSTR pszFile /*optional*/, DWORD dwFileAttr)
{
    BOOL bRet = FALSE;
    if (dwFileAttr == -1)
    {
        ASSERT(pszFile != NULL) ;
        dwFileAttr = pszFile ? GetFileAttributes(pszFile) : -1;
    }
    
    if ((dwFileAttr != -1) && (dwFileAttr & FILE_ATTRIBUTE_OFFLINE))
    {
        bRet = TRUE;
    }

    return bRet;
}


//
//  PathIsSlow() - is a path slow or not
//  dwFileAttr -- The file attributes, pass -1 if not available
//
STDAPI_(BOOL) PathIsSlow(LPCTSTR pszFile, DWORD dwFileAttr)
{
    BOOL bSlow = FALSE;
    if (PathIsUNC(pszFile))
    {
        DWORD speed = GetPathSpeed(pszFile);
        bSlow = (speed != 0) && (speed <= SPEED_SLOW);
    }
    else if (CMtPt_IsSlow(PathGetDriveNumber(pszFile)))
        bSlow = TRUE;

    if( !bSlow )
        bSlow = PathIsHighLatency(pszFile, dwFileAttr);

    return bSlow;
}

#ifndef UNICODE
STDAPI_(BOOL) PathIsSlowW( LPCWSTR pszFile, DWORD dwFileAttr )
{
    CHAR szBuffer[MAX_PATH];

    SHUnicodeToAnsi( pszFile, szBuffer, MAX_PATH );
    return PathIsSlowA( szBuffer, dwFileAttr );
}

#else
STDAPI_(BOOL) PathIsSlowA( LPCSTR pszFile, DWORD dwFileAttr )
{
    WCHAR szBuffer[MAX_PATH];

    SHAnsiToUnicode( pszFile, szBuffer, MAX_PATH );
    return PathIsSlowW( szBuffer, dwFileAttr );
}
#endif


/*----------------------------------------------------------------------------
/ PathProcessCommand implementation
/ ------------------
/ Purpose:
/   Process the specified command line and generate a suitably quoted
/   name, with arguments attached if required.
/
/ Notes:
/   - The destination buffer size can be determined if NULL is passed as a
/     destination pointer.
/   - If the source string is quoted then we assume that it exists on the
/     filing system.
/
/ In:
/   lpSrc -> null terminate source path
/   lpDest -> destination buffer / = NULL to return buffer size
/   iMax = maximum number of characters to return into destination
/   dwFlags =
/       PPCF_ADDQUOTES         = 1 => if path requires quotes then add them
/       PPCF_ADDARGUMENTS      = 1 => append trailing arguments to resulting string (forces ADDQUOTES)
/       PPCF_NODIRECTORIES     = 1 => don't match against directories, only file objects
/       PPCF_NORELATIVEOBJECTQUALIFY = 1 => locate relative objects and return the full qualified path
/       PPCF_LONGESTPOSSIBLE   = 1 => always choose the longest possible executable name ex: d:\program files\fun.exe vs. d:\program.exe
/ Out:
/   > 0 if the call works
/   < 0 if the call fails (object not found, buffer too small for resulting string)
/----------------------------------------------------------------------------*/

STDAPI_(LONG) PathProcessCommand(LPCTSTR lpSrc, LPTSTR lpDest, int iDestMax, DWORD dwFlags)
{
    TCHAR szName[MAX_PATH];
    TCHAR szLastChoice[MAX_PATH];

    LPTSTR lpBuffer, lpBuffer2;
    LPCTSTR lpArgs = NULL;
    DWORD dwAttrib;
    LONG i, iTotal;
    LONG iResult = -1;
    BOOL bAddQuotes = FALSE;
    BOOL bQualify = FALSE;
    BOOL bFound = FALSE;
    BOOL bHitSpace = FALSE;
    BOOL bRelative = FALSE;
    LONG iLastChoice = 0;

    // Process the given source string, attempting to find what is that path, and what is its
    // arguments.

    if ( lpSrc )
    {
        // Extract the sub string, if its is realative then resolve (if required).

        if ( *lpSrc == TEXT('\"') )
        {
            for ( lpSrc++, i=0 ; i<MAX_PATH && *lpSrc && *lpSrc!=TEXT('\"') ; i++, lpSrc++ )
                szName[i] = *lpSrc;

            szName[i] = TEXT('\0');

            if ( *lpSrc )
                lpArgs = lpSrc+1;

            if ( ((dwFlags & PPCF_FORCEQUALIFY) || PathIsRelative( szName ))
                    && !( dwFlags & PPCF_NORELATIVEOBJECTQUALIFY ) )
            {
                if ( !PathResolve( szName, NULL, PRF_TRYPROGRAMEXTENSIONS ) )
                    goto exit_gracefully;
            }

            bFound = TRUE;
        }
        else
        {
            // Is this a relative object, and then take each element upto a seperator
            // and see if we hit an file system object.  If not then we can

            bRelative = PathIsRelative( lpSrc );
            if (bRelative)
                dwFlags &= ~PPCF_LONGESTPOSSIBLE;
            
            bQualify = bRelative || ((dwFlags & PPCF_FORCEQUALIFY) != 0);

            for ( i=0; i < MAX_PATH; i++ )
            {
                szName[i] = lpSrc[i];

                // If we hit a space then the string either contains a LFN or we have
                // some arguments.  Therefore attempt to get the attributes for the string
                // we have so far, if we are unable to then we can continue
                // checking, if we hit then we know that the object exists and the
                // trailing string are its arguments.

                if ( !szName[i] || szName[i] == TEXT(' ') )
                {
                    szName[i] = TEXT('\0');
                    if ( !bQualify || PathResolve( szName, NULL, PRF_TRYPROGRAMEXTENSIONS ) )
                    {
                        dwAttrib = GetFileAttributes( szName );

                        if ((dwAttrib != -1) && (! (( dwAttrib & FILE_ATTRIBUTE_DIRECTORY ) && (dwFlags & PPCF_NODIRECTORIES))))
                        {
                            if ( bQualify && ( dwFlags & PPCF_NORELATIVEOBJECTQUALIFY ) )
                                *lstrcpyn( szName, lpSrc, i ) = TEXT(' ');

                            bFound = TRUE;                  // success
                            lpArgs = &lpSrc[i];
                        
                            if (dwFlags & PPCF_LONGESTPOSSIBLE)
                            {
                                lstrcpyn(szLastChoice, szName, i+1);
                                iLastChoice = i;
                            }
                            else
                                goto exit_gracefully;
                        }
                    }

                    if ( bQualify )
                        memcpy( szName, lpSrc, (i+1)*SIZEOF(TCHAR) );
                    else
                        szName[i]=lpSrc[i];

                    bHitSpace = TRUE;
                }

                if ( !szName[i] )
                    break;
            }
        }
    }

exit_gracefully:

    // Work out how big the temporary buffer should be, allocate it and
    // build the returning string into it.  Then compose the string
    // to be returned.

    if ( bFound )
    {
        
        if ((dwFlags & PPCF_LONGESTPOSSIBLE) && iLastChoice)
        {
            lstrcpyn(szName, szLastChoice, iLastChoice+1);
            lpArgs = &lpSrc[iLastChoice];
        }
        
        if ( StrChr( szName, TEXT(' ') ) )
            bAddQuotes = dwFlags & PPCF_ADDQUOTES;

        iTotal  = lstrlen(szName) + 1;                // for terminator
        iTotal += bAddQuotes ? 2 : 0;
        iTotal += (dwFlags & PPCF_ADDARGUMENTS) && lpArgs ? lstrlen(lpArgs) : 0;

        if ( lpDest )
        {
            if ( iTotal <= iDestMax )
            {
                lpBuffer = lpBuffer2 = (LPTSTR)LocalAlloc( LMEM_FIXED, SIZEOF(TCHAR)*iTotal );

                if ( lpBuffer )
                {
                    // First quote if required
                    if ( bAddQuotes )
                        *lpBuffer2++ = TEXT('\"');

                    // Matching name
                    lstrcpy( lpBuffer2, szName );

                    // Closing quote if required
                    if ( bAddQuotes )
                        lstrcat( lpBuffer2, TEXT("\"") );

                    // Arguments (if requested)
                    if ( (dwFlags & PPCF_ADDARGUMENTS) && lpArgs )
                        lstrcat( lpBuffer2, lpArgs );

                    // Then copy into callers buffer, and free out temporary buffer
                    lstrcpy( lpDest, lpBuffer );
                    LocalFree( (HGLOBAL)lpBuffer );

                    // Return the length of the resulting string
                    iResult = iTotal;
                }
            }
        }
        else
        {
            // Resulting string is this big, although nothing returned (allows them to allocate a buffer)
            iResult = iTotal;
        }
    }

    return iResult;
}


#define GREEK_CODEPAGE  1253

//---------------------------------------------------------------------------
// Returns whether the given net path exists.  This fails for NON net paths.
//

STDAPI_(BOOL) NetPathExists(LPCTSTR lpszPath, LPDWORD lpdwType)
{
    BOOL fResult = FALSE;
    NETRESOURCE nr;
    LPTSTR lpSystem;
    DWORD dwRes, dwSize = 1024;
    LPVOID lpv;

    if (!lpszPath || !*lpszPath)
        return FALSE;

#ifndef WINNT
    // This is very ugly HACK because of Greek Win95 MPR.DLL problem. 
    // WNetGetResourceInformation can't return correctly if share name has final sigma
    // character in the share name on Greek Win95.
    if (g_uCodePage == GREEK_CODEPAGE)
        return PathFileExistsAndAttributes(lpszPath, NULL);
#endif

    lpv = (LPVOID)LocalAlloc( LPTR, dwSize );
    if (!lpv)
        return FALSE;

TryWNetAgain:
    nr.dwScope = RESOURCE_GLOBALNET;
    nr.dwType = RESOURCETYPE_ANY;
    nr.dwDisplayType = 0;
    nr.lpLocalName = NULL;
    nr.lpRemoteName = (LPTSTR)lpszPath;
    nr.lpProvider = NULL;
    nr.lpComment = NULL;
    dwRes = WNetGetResourceInformation(&nr, lpv, &dwSize, &lpSystem);

    // If our buffer wasn't big enough, try a bigger buffer...
    if (dwRes == WN_MORE_DATA)
    {
        LPVOID tmp = LocalReAlloc( lpv, dwSize, LMEM_MOVEABLE );
        if (!tmp)
        {
            LocalFree( lpv );
            SetLastError( ERROR_OUTOFMEMORY );
            return FALSE;
        }

        lpv = tmp;
        goto TryWNetAgain;
    }

    fResult = (dwRes == WN_SUCCESS);

    if (fResult && lpdwType)
        *lpdwType = ((LPNETRESOURCE)lpv)->dwType;

    LocalFree(lpv);

    return fResult;
}

//
// needed because we export TCHAR versions of these functions that 
// internal components still call
//
// Functions are forwarded to shlwapi
//

#undef PathMakePretty

STDAPI_(BOOL) PathMakePretty(LPTSTR lpPath)
{
    SHELLSTATE ss;
    
    SHGetSetSettings(&ss, SSF_DONTPRETTYPATH, FALSE);
    if (ss.fDontPrettyPath)
        return FALSE;

#ifdef UNICODE
    return PathMakePrettyW(lpPath);
#else
    return PathMakePrettyA(lpPath);
#endif
}

#undef PathGetArgs

STDAPI_(LPTSTR) PathGetArgs(LPCTSTR pszPath)
{
#ifdef UNICODE
    return PathGetArgsW(pszPath);
#else
    return PathGetArgsA(pszPath);
#endif
}


#undef PathRemoveArgs

STDAPI_(void) PathRemoveArgs(LPTSTR pszPath)
{
#ifdef UNICODE
    PathRemoveArgsW(pszPath);
#else
    PathRemoveArgsA(pszPath);
#endif
}


#undef PathFindOnPath

STDAPI_(BOOL) PathFindOnPath(LPTSTR pszFile, LPCTSTR *ppszOtherDirs)
{
#ifdef UNICODE
    return PathFindOnPathW(pszFile, ppszOtherDirs);
#else
    return PathFindOnPathA(pszFile, ppszOtherDirs);
#endif
}


#undef PathFindExtension

STDAPI_(LPTSTR) PathFindExtension(LPCTSTR pszPath)
{
#ifdef UNICODE
    return PathFindExtensionW(pszPath);
#else
    return PathFindExtensionA(pszPath);
#endif
}

#undef PathAddExtension

STDAPI_(BOOL) PathAddExtension(LPTSTR  pszPath, LPCTSTR pszExtension)
{
#ifdef UNICODE
    return PathAddExtensionW(pszPath, pszExtension);
#else
    return PathAddExtensionA(pszPath, pszExtension);
#endif
}

#undef PathRemoveExtension

STDAPI_(void) PathRemoveExtension(LPTSTR pszPath)
{
#ifdef UNICODE
    PathRemoveExtensionW(pszPath);
#else
    PathRemoveExtensionA(pszPath);
#endif
}

#undef PathRenameExtension

STDAPI_(BOOL) PathRenameExtension(LPTSTR  pszPath, LPCTSTR pszExt)
{
#ifdef UNICODE
    return PathRenameExtensionW(pszPath, pszExt);
#else
    return PathRenameExtensionA(pszPath, pszExt);
#endif
}

#undef PathCommonPrefix

STDAPI_(int) PathCommonPrefix(LPCTSTR pszFile1, LPCTSTR pszFile2, LPTSTR pszPath)
{
#ifdef UNICODE
    return PathCommonPrefixW(pszFile1, pszFile2, pszPath);
#else
    return PathCommonPrefixA(pszFile1, pszFile2, pszPath);
#endif
}

#undef PathIsPrefix

STDAPI_(BOOL) PathIsPrefix(IN LPCTSTR  pszPrefix, IN LPCTSTR  pszPath)
{
#ifdef UNICODE
    return PathIsPrefixW(pszPrefix, pszPath);
#else
    return PathIsPrefixA(pszPrefix, pszPath);
#endif
}

#undef PathRelativePathTo

STDAPI_(BOOL) PathRelativePathTo(LPTSTR pszPath, 
     LPCTSTR pszFrom, DWORD dwAttrFrom,  LPCTSTR pszTo, DWORD dwAttrTo)
{
#ifdef UNICODE
    return PathRelativePathToW(pszPath, pszFrom, dwAttrFrom, pszTo, dwAttrTo);
#else
    return PathRelativePathToA(pszPath, pszFrom, dwAttrFrom, pszTo, dwAttrTo);
#endif
}

#undef PathRemoveBlanks

STDAPI_(void) PathRemoveBlanks(LPTSTR lpszString)
{
#ifdef UNICODE
    PathRemoveBlanksW(lpszString);
#else
    PathRemoveBlanksA(lpszString);
#endif
}

#undef PathRemoveBackslash

STDAPI_(LPTSTR) PathRemoveBackslash(LPTSTR lpszPath)
{
#ifdef UNICODE
    return PathRemoveBackslashW(lpszPath);
#else
    return PathRemoveBackslashA(lpszPath);
#endif
}

#undef PathCanonicalize

STDAPI_(BOOL) PathCanonicalize(LPTSTR  lpszDst, LPCTSTR lpszSrc)
{
#ifdef UNICODE
    return PathCanonicalizeW(lpszDst, lpszSrc);
#else
    return PathCanonicalizeA(lpszDst, lpszSrc);
#endif
}

#undef PathStripToRoot

STDAPI_(BOOL) PathStripToRoot(LPTSTR szRoot)
{
#ifdef UNICODE
    return PathStripToRootW(szRoot);
#else
    return PathStripToRootA(szRoot);
#endif
}

//CD-Autorun for Win9x called the TCHAR internal api's. So as a workaround we stub them through these function calls.

#undef PathRemoveFileSpec

STDAPI_(BOOL) PathRemoveFileSpec(LPTSTR pFile)
{
    if (SHGetAppCompatFlags(ACF_ANSI) == ACF_ANSI)
        return PathRemoveFileSpecA((LPSTR)pFile);
    else
#ifdef UNICODE
        return PathRemoveFileSpecW(pFile);
#else
        return PathRemoveFileSpecA(pFile);
#endif
}

#undef PathAddBackslash

STDAPI_(LPTSTR) PathAddBackslash(LPTSTR lpszPath)
{
#ifdef UNICODE
    return PathAddBackslashW(lpszPath);
#else
    return PathAddBackslashA(lpszPath);
#endif
}

#undef PathFindFileName

STDAPI_(LPTSTR) PathFindFileName(LPCTSTR pPath)
{
#ifdef UNICODE
    return PathFindFileNameW(pPath);
#else
    return PathFindFileNameA(pPath);
#endif
}

#undef PathIsFileSpec

STDAPI_(BOOL) PathIsFileSpec(LPCTSTR lpszPath)
{
#ifdef UNICODE
    return PathIsFileSpecW(lpszPath);
#else
    return PathIsFileSpecA(lpszPath);
#endif
}

#undef PathIsUNCServer

STDAPI_(BOOL) PathIsUNCServer(LPCTSTR pszPath)
{
#ifdef UNICODE
    return PathIsUNCServerW(pszPath);
#else
    return PathIsUNCServerA(pszPath);
#endif
}

#undef PathIsUNCServerShare

STDAPI_(BOOL) PathIsUNCServerShare(LPCTSTR pszPath)
{
#ifdef UNICODE
    return PathIsUNCServerShareW(pszPath);
#else
    return PathIsUNCServerShareA(pszPath);
#endif
}

#undef PathStripPath

STDAPI_(void) PathStripPath(LPTSTR pszPath)
{
#ifdef UNICODE
    PathStripPathW(pszPath);
#else
    PathStripPathA(pszPath);
#endif
}

#undef PathSearchAndQualify

STDAPI_(BOOL) PathSearchAndQualify(LPCTSTR pcszPath, LPTSTR pszFullyQualifiedPath, UINT cchFullyQualifiedPath)
{
#ifdef UNICODE
    return PathSearchAndQualifyW(pcszPath, pszFullyQualifiedPath, cchFullyQualifiedPath);
#else
    return PathSearchAndQualifyA(pcszPath, pszFullyQualifiedPath, cchFullyQualifiedPath);
#endif
}

//CD-Autorun for Win9x called the TCHAR internal api's. So as a workaround we stub them through these function calls.

#undef PathIsRoot

STDAPI_(BOOL) PathIsRoot(LPCTSTR pPath)
{
    if (SHGetAppCompatFlags(ACF_ANSI) == ACF_ANSI)
        return PathIsRootA((LPCSTR)pPath);
    else
#ifdef UNICODE
        return PathIsRootW(pPath);
#else
        return PathIsRootA(pPath);
#endif
}


#undef PathCompactPath

STDAPI_(BOOL) PathCompactPath(HDC hDC, LPTSTR lpszPath, UINT dx)
{
#ifdef UNICODE
    return PathCompactPathW(hDC, lpszPath, dx);
#else
    return PathCompactPathA(hDC, lpszPath, dx);
#endif
}


#undef PathCompactPathEx

STDAPI_(BOOL) PathCompactPathEx(LPTSTR  pszOut, LPCTSTR pszSrc, UINT cchMax, DWORD dwFlags)
{
#ifdef UNICODE
    return PathCompactPathExW(pszOut, pszSrc, cchMax, dwFlags);
#else
    return PathCompactPathExA(pszOut, pszSrc, cchMax, dwFlags);
#endif
}


#undef PathSetDlgItemPath

STDAPI_(void) PathSetDlgItemPath(HWND hDlg, int id, LPCTSTR pszPath)
{
#ifdef UNICODE
    PathSetDlgItemPathW(hDlg, id, pszPath);
#else
    PathSetDlgItemPathA(hDlg, id, pszPath);
#endif
}


#undef PathUnquoteSpaces

STDAPI_(void) PathUnquoteSpaces(LPTSTR lpsz)
{
#ifdef UNICODE
    PathUnquoteSpacesW(lpsz);
#else
    PathUnquoteSpacesA(lpsz);
#endif
}


#undef PathQuoteSpaces

STDAPI_(void) PathQuoteSpaces(LPTSTR lpsz)
{
#ifdef UNICODE
    PathQuoteSpacesW(lpsz);
#else
    PathQuoteSpacesA(lpsz);
#endif
}


#undef PathFindNextComponent

STDAPI_(LPTSTR) PathFindNextComponent(LPCTSTR pszPath)
{
#ifdef UNICODE
    return PathFindNextComponentW(pszPath);
#else
    return PathFindNextComponentA(pszPath);
#endif
}


#undef PathMatchSpec

STDAPI_(BOOL) PathMatchSpec(LPCTSTR pszFileParam, LPCTSTR pszSpec)
{
#ifdef UNICODE
    return PathMatchSpecW(pszFileParam, pszSpec);
#else
    return PathMatchSpecA(pszFileParam, pszSpec);
#endif
}

#undef PathSkipRoot

STDAPI_(LPTSTR) PathSkipRoot(LPCTSTR pszPath)
{
#ifdef UNICODE
    return PathSkipRootW(pszPath);
#else
    return PathSkipRootA(pszPath);
#endif
}

#undef PathIsSameRoot

STDAPI_(BOOL) PathIsSameRoot(LPCTSTR pszPath1, LPCTSTR pszPath2)
{
#ifdef UNICODE
    return PathIsSameRootW(pszPath1, pszPath2);
#else
    return PathIsSameRootA(pszPath1, pszPath2);
#endif
}


#undef PathParseIconLocation

STDAPI_(int) PathParseIconLocation(IN OUT LPTSTR pszIconFile)
{
#ifdef UNICODE
    return PathParseIconLocationW(pszIconFile);
#else
    return PathParseIconLocationA(pszIconFile);
#endif
}

#undef PathIsURL

STDAPI_(BOOL) PathIsURL(IN LPCTSTR pszPath)
{
#ifdef UNICODE
    return PathIsURLW(pszPath);
#else
    return PathIsURLA(pszPath);
#endif
}

#undef PathIsDirectory

STDAPI_(BOOL) PathIsDirectory(LPCTSTR pszPath)
{
#ifdef UNICODE
    return PathIsDirectoryW(pszPath);
#else
    return PathIsDirectoryA(pszPath);
#endif

}

// Gets the mounting point for the path passed in
//
// Return Value: TRUE:  means that we found mountpoint, e.g. c:\ or c:\hostfolder\
//               FALSE: for now means that the path is UNC or buffer too small
//
//           Mounted volume                                 Returned Path
//
//      Passed in E:\MountPoint\path 1\path 2
// C:\ as E:\MountPoint                                 E:\MountPoint
//
//      Passed in E:\MountPoint\MountInter\path 1
// C:\ as D:\MountInter and D:\ as E:\MountPoint        E:\MountPoint\MountInter
//
//      Passed in E:\MountPoint\MountInter\path 1
// No mount                                             E:\ 
BOOL PathGetMountPointFromPath(LPCTSTR pcszPath, LPTSTR pszMountPoint, int cchMountPoint)
{
    BOOL bRet = FALSE;

    if (!PathIsUNC(pcszPath) && (cchMountPoint >= lstrlen(pcszPath) + 1))
    {
        lstrcpy(pszMountPoint, pcszPath);

        // Is this only 'c:' or 'c:\'
        if (lstrlen(pcszPath) > 3)
        {
            //no
            LPTSTR pszNextComp = NULL;
            LPTSTR pszBestChoice = NULL;
            TCHAR cTmpChar;

            PathAddBackslash(pszMountPoint);
            //skip the first one, e.g. c:\ 
            pszBestChoice = pszNextComp = PathFindNextComponent(pszMountPoint);
            pszNextComp = PathFindNextComponent(pszNextComp);
            while (pszNextComp)
            {
                cTmpChar = *pszNextComp;
                *pszNextComp = TEXT('\0');

                if (GetVolumeInformation(pszMountPoint, NULL, 0, NULL, NULL, NULL, NULL, 0))
                {//found something better than previous shorter path
                    pszBestChoice = pszNextComp;
                }

                *pszNextComp = cTmpChar;
                pszNextComp = PathFindNextComponent(pszNextComp);
            }

            *pszBestChoice = TEXT('\0');
        }
        bRet = TRUE;
    }
    else
        *pszMountPoint = 0;

    return bRet;
}


const TCHAR c_szRegstrSystemPrograms[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\App Management\\System Programs");

//
// Returns TRUE if the path is a shortcut to an installed program that can
// be found under Add/Remvoe Programs 
// The current algorithm is just to make sure the target is an exe and is
// located under "program files"
//
STDAPI_(BOOL) PathIsShortcutToProgram(LPCTSTR pszFile)
{
    BOOL bRet = FALSE;
    if (PathIsShortcut(pszFile))
    {
        TCHAR szTarget[MAX_PATH];
        HRESULT hr = GetPathFromLinkFile(pszFile, szTarget, ARRAYSIZE(szTarget));
        if (hr == S_OK)
        {
            if (PathIsExe(szTarget))
            {
                BOOL bSpecialApp = FALSE;
                HKEY hkeySystemPrograms = NULL;
                if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegstrSystemPrograms, 0, KEY_READ, &hkeySystemPrograms))
                {
                    TCHAR szValue[MAX_PATH];
                    TCHAR szSystemPrograms[MAX_PATH];
                    DWORD cbSystemPrograms = SIZEOF(szSystemPrograms);
                    DWORD cchValue = ARRAYSIZE(szValue);

                    DWORD dwType; 
                    LPTSTR pszFileName = PathFindFileName(szTarget);
                    int iValue = 0;
                    while (RegEnumValue(hkeySystemPrograms, iValue, szValue, &cchValue, NULL, &dwType,
                                        (LPBYTE)szSystemPrograms, &cbSystemPrograms) == ERROR_SUCCESS)
                    {
                        if ((dwType == REG_SZ) && !StrCmpI(pszFileName, szSystemPrograms))
                        {
                            bSpecialApp = TRUE;
                            break;
                        }

                        cbSystemPrograms = SIZEOF(szSystemPrograms);
                        cchValue = ARRAYSIZE(szValue);
                        iValue++;
                    }
                    
                    RegCloseKey(hkeySystemPrograms);
                }

                if (!bSpecialApp)
                {
                    TCHAR szProgramFiles[MAX_PATH];
                    if (SHGetSpecialFolderPath(NULL, szProgramFiles, CSIDL_PROGRAM_FILES, FALSE))
                    {
                        if (PathIsPrefix(szProgramFiles, szTarget))
                        {
                            bRet = TRUE;
                        }
                    }
                }
                else
                    bRet = FALSE;
            }
        }
        else if (hr == S_FALSE && szTarget[0])
        {
            // Darwin shortcuts, say yes
            bRet = TRUE;
        }
    }
    return bRet;
}

//CD-Autorun for Win9x called the TCHAR internal api's. So as a workaround we stub them through these function calls.

#undef PathFileExists

STDAPI_(BOOL) PathFileExists(LPCTSTR lpszPath)
{
    if (SHGetAppCompatFlags(ACF_ANSI) == ACF_ANSI)
        return PathFileExistsAndAttributesA((LPCSTR)lpszPath, NULL);
    else
#ifdef UNICODE
        return PathFileExistsAndAttributesW(lpszPath, NULL);
#else
        return PathFileExistsAndAttributesA(lpszPath, NULL);
#endif
}

#undef PathAppend

STDAPI_(BOOL) PathAppend(LPTSTR pPath, LPCTSTR pMore)
{
    /* Skip any initial terminators on input. */

  if (SHGetAppCompatFlags(ACF_ANSI) == ACF_ANSI)
     return PathAppendA((LPSTR)pPath, (LPCSTR)pMore);
  else
#ifdef UNICODE
     return PathAppendW(pPath, pMore);
#else
     return PathAppendA(pPath, pMore);
#endif
}
