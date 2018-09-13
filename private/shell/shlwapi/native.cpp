// Contains code that needs to be dual compiled, once for ansi and once for unicode

#include "priv.h"
#include "native.h"     // definitions of files in native.cpp
#include "fstream.h"    // definition of CFileStream used by _CreateStreamOnFile
#include <w95wraps.h>

// create an IStream from a Win32 file name.
// in:
//      pszFile     file name to open
//      grfMode     STGM_ flags
//
STDAPI _CreateStreamOnFile(LPCTSTR pszFile, DWORD grfMode, IStream **ppstm)
{
    HANDLE hFile;

    *ppstm = NULL;

    //
    //  we only handle the below modes (they map to STGM_ flags),
    //  don't let anything else through
    //
    if (grfMode &
        ~(STGM_READ             |
          STGM_WRITE            |
          STGM_SHARE_DENY_NONE  |
          STGM_SHARE_DENY_READ  |
          STGM_SHARE_DENY_WRITE |
          STGM_SHARE_EXCLUSIVE  |
          STGM_READWRITE        |
          STGM_CREATE         ))
    {
        DebugMsg(DM_ERROR, TEXT("CreateSreamOnFile: Invalid STGM_ mode"));
        return E_INVALIDARG;
    }

    if ( grfMode & STGM_CREATE)
    {
        // Need to get the file attributes of the file first, so
        // that CREATE_ALWAYS will succeed for HIDDEN and SYSTEM
        // attributes.
        DWORD dwAttrib = GetFileAttributes( pszFile );
        if ((DWORD)-1 == dwAttrib )
        {
            // something went wrong, so set attributes to something
            // normal before we try to create the file...
            dwAttrib = 0;
        }

        // STGM_CREATE
        hFile = CreateFile(pszFile, GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
            dwAttrib, NULL);
    }
    else
    {
        DWORD dwDesiredAccess, dwShareMode, dwShareBits;

        // not STGM_CREATE
        if ( grfMode & STGM_WRITE )
        {
            dwDesiredAccess = GENERIC_WRITE;
        }
        else
        {
            dwDesiredAccess = GENERIC_READ;
        }
        if ( grfMode & STGM_READWRITE )
        {
            dwDesiredAccess |= (GENERIC_READ | GENERIC_WRITE);
        }
        dwShareBits = grfMode & (STGM_SHARE_EXCLUSIVE | 
                                 STGM_SHARE_DENY_WRITE | 
                                 STGM_SHARE_DENY_READ | 
                                 STGM_SHARE_DENY_NONE);
        switch( dwShareBits ) {
            case STGM_SHARE_DENY_WRITE:
                dwShareMode = FILE_SHARE_READ;
                break;
            case STGM_SHARE_DENY_READ:
                dwShareMode = FILE_SHARE_WRITE;
                break;
            case STGM_SHARE_EXCLUSIVE:
                dwShareMode = 0;
                break;
            default:
                dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
                break;
        }


        hFile = CreateFile(pszFile, dwDesiredAccess, dwShareMode, NULL,
                                   OPEN_EXISTING, 0, NULL);
    }

    if (hFile == INVALID_HANDLE_VALUE)
    {
        DebugMsg(DM_TRACE, TEXT("CreateSreamOnFile: CreateFileW() failed %s"), pszFile);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    *ppstm = (IStream *)new CFileStream(hFile, grfMode);
    if (*ppstm)
    {
        return S_OK;
    }
    else
    {
        CloseHandle(hFile);
        return E_OUTOFMEMORY;
    }
}

BOOL _PathAppend(LPCTSTR pszBase, LPCTSTR pszAppend, LPTSTR pszOut, DWORD cchOut)
{
    DWORD cchBase = lstrlen(pszBase);

    //  +1 is one for the whack 
    if (cchOut > cchBase + lstrlen(pszAppend) + 1)
    {
        StrCpy(pszOut, pszBase);
        pszOut+=cchBase;
        *pszOut++ = TEXT('\\');
        StrCpy(pszOut, pszAppend);
        return TRUE;
    }
    return FALSE;
}


#define REGSTR_PATH_EXPLORER_FILEEXTS   TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts")

LWSTDAPI AssocMakeFileExtsToApplication(ASSOCMAKEF flags, LPCTSTR pszExt, LPCTSTR pszApplication)
{
    HRESULT hr = E_INVALIDARG;

    if (pszExt)
    {
        TCHAR szKey[MAX_PATH];
        _PathAppend(REGSTR_PATH_EXPLORER_FILEEXTS, pszExt, szKey, SIZECHARS(szKey));

        DWORD err;
        if (!pszApplication)
            err = SHDeleteValue(HKEY_CURRENT_USER, szKey, TEXT("Application"));
        else
            err = SHSetValue(HKEY_CURRENT_USER, szKey, TEXT("Application"),
                REG_SZ, pszApplication, CbFromCch(lstrlen(pszApplication) +1));

        hr = HRESULT_FROM_WIN32(err);
    }

    return hr;
}


HRESULT _AllocValueString(HKEY hkey, LPCTSTR pszKey, LPCTSTR pszVal, LPTSTR *ppsz)
{
    DWORD cb, err;
    err = SHGetValue(hkey, pszKey, pszVal, NULL, NULL, &cb);

    ASSERT(ppsz);
    *ppsz = NULL;
    
    if (NOERROR == err)
    {
        LPTSTR psz = (LPTSTR) LocalAlloc(LPTR, cb);

        if (psz)
        {
            err = SHGetValue(hkey, pszKey, pszVal, NULL, (LPVOID)psz, &cb);

            if (NOERROR == err)
                *ppsz = psz;
            else
                LocalFree(psz);
        }
        else
            err = ERROR_OUTOFMEMORY;
    }

    return HRESULT_FROM_WIN32(err);
}


// <Swipped from the NT5 version of Shell32>
#define SZ_REGKEY_FILEASSOCIATION TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileAssociation")

VOID WINAPI PrettifyFileDescription(LPTSTR pszDesc, LPCTSTR pszCutList)
{
    LPTSTR pszCutListReg;
    
    if (!pszDesc || !*pszDesc)
        return;

    // get the Cut list from registry
    //  this is MULTI_SZ
    if (S_OK == _AllocValueString(HKEY_LOCAL_MACHINE, SZ_REGKEY_FILEASSOCIATION, TEXT("CutList"), &pszCutListReg))
    {
        pszCutList = pszCutListReg;
    }

    if (pszCutList)
    {

        // cut strings in cut list from file description
        for (LPCTSTR pszCut = pszCutList; *pszCut; pszCut = pszCut + lstrlen(pszCut) + 1)
        {
            LPTSTR pch = StrRStrI(pszDesc, NULL, pszCut);

            // cut the exact substring from the end of file description
            if (pch && !*(pch + lstrlen(pszCut)))
            {
                *pch = '\0';

                // remove trailing spaces
                for (--pch; (pch >= pszDesc) && (TEXT(' ') == *pch); pch--)
                    *pch = 0;

                break;
            }
        }
        
        if (pszCutListReg)
            LocalFree(pszCutListReg);
    }
}

/*
    <Swipped from the NT5 version of Shell32>

    GetFileDescription retrieves the friendly name from a file's verion rsource.
    The first language we try will be the first item in the
    "\VarFileInfo\Translations" section;  if there's nothing there,
    we try the one coded into the IDS_VN_FILEVERSIONKEY resource string.
    If we can't even load that, we just use English (040904E4).  We
    also try English with a null codepage (04090000) since many apps
    were stamped according to an old spec which specified this as
    the required language instead of 040904E4.
    
    If there is no FileDescription in version resource, return the file name.

    Parameters:
        LPCTSTR pszPath: full path of the file
        LPTSTR pszDesc: pointer to the buffer to receive friendly name. If NULL, 
                        *pcchDesc will be set to the length of friendly name in 
                        characters, including ending NULL, on successful return.
        UINT *pcchDesc: length of the buffer in characters. On successful return,
                        it contains number of characters copied to the buffer,
                        including ending NULL.

    Return:
        TRUE on success, and FALSE otherwise
*/
BOOL WINAPI SHGetFileDescription(LPCTSTR pszPath, LPCTSTR pszVersionKeyIn, LPCTSTR pszCutListIn, LPTSTR pszDesc, UINT *pcchDesc)
{
    UINT cchValue = 0;
    TCHAR szPath[MAX_PATH], *pszValue = NULL;
    
    DWORD dwHandle;                 /* version subsystem handle */
    DWORD dwVersionSize;            /* size of the version data */
    LPTSTR lpVersionBuffer = NULL;  /* pointer to version data */
    TCHAR szVersionKey[60];         /* big enough for anything we need */
    
    struct _VERXLATE
    {
        WORD wLanguage;
        WORD wCodePage;
    } *lpXlate;                     /* ptr to translations data */
    
    ASSERT(pszPath && *pszPath && pcchDesc);
    
    if (!PathFileExists(pszPath))
        return FALSE;

    lstrcpyn(szPath, pszPath, ARRAYSIZE(szPath));
        
    dwVersionSize = GetFileVersionInfoSize(szPath, &dwHandle);
    if (dwVersionSize == 0L)
        goto Error;                 /* no version info */

    lpVersionBuffer = (LPTSTR)LocalAlloc(LPTR, dwVersionSize);
    if (lpVersionBuffer == NULL)
        goto Error;

    if (!GetFileVersionInfo(szPath, dwHandle, dwVersionSize, lpVersionBuffer))
        goto Error;

    // Try same language as the caller 
    if (pszVersionKeyIn)
    {
        lstrcpyn(szVersionKey, pszVersionKeyIn, ARRAYSIZE(szVersionKey));

        if (VerQueryValue(lpVersionBuffer, szVersionKey, (void **)&pszValue, &cchValue))
        {
            goto Exit;
        }
    }

    // Try first language this supports
    // Look for translations
    if (VerQueryValue(lpVersionBuffer, TEXT("\\VarFileInfo\\Translation"),
                      (void **)&lpXlate, &cchValue)
        && cchValue)
    {
        wnsprintf(szVersionKey, ARRAYSIZE(szVersionKey), TEXT("\\StringFileInfo\\%04X%04X\\FileDescription"),
                 lpXlate[0].wLanguage, lpXlate[0].wCodePage);
        if (VerQueryValue(lpVersionBuffer, szVersionKey, (void **)&pszValue, &cchValue))
            goto Exit;

    }

#ifdef UNICODE
    // try English, unicode code page
    lstrcpy(szVersionKey, TEXT("\\StringFileInfo\\040904B0\\FileDescription"));
    if (VerQueryValue(lpVersionBuffer, szVersionKey, (void **)&pszValue, &cchValue))
        goto Exit;
#endif

    // try English
    lstrcpy(szVersionKey, TEXT("\\StringFileInfo\\040904E4\\FileDescription"));
    if (VerQueryValue(lpVersionBuffer, szVersionKey, (void **)&pszValue, &cchValue))
        goto Exit;

    // try English, null codepage
    lstrcpy(szVersionKey, TEXT("\\StringFileInfo\\04090000\\FileDescription"));
    if (VerQueryValue(lpVersionBuffer, szVersionKey, (void **)&pszValue, &cchValue))
        goto Exit;

Error:  // Could not find FileVersion info in a reasonable format, return file name
    PathRemoveExtension(szPath);
    pszValue = PathFindFileName(szPath);
    cchValue = lstrlen(pszValue);
    
Exit:   
    PrettifyFileDescription(pszValue, pszCutListIn);
    cchValue = lstrlen(pszValue) + 1;
    
    if (!pszDesc)   // only want to know the length of the friendly name
        *pcchDesc = cchValue;
    else
    {
        *pcchDesc = min(*pcchDesc, cchValue);
        lstrcpyn(pszDesc, pszValue, *pcchDesc);
    }
    
    if (lpVersionBuffer) 
        LocalFree(lpVersionBuffer);
        
    return TRUE;
}


// Convert LPTSTR to LPSTR and return TRUE if the LPSTR can
// be converted back to LPTSTR without unacceptible data loss
//
BOOL DoesStringRoundTrip(LPCTSTR pwszIn, LPSTR pszOut, UINT cchOut)
{
#ifdef UNICODE
    // On NT5 we have to be more stringent since you can switch UI
    // languages on the fly, thereby breaking this constant codepage
    // assumption inherent in the downlevel implementations.
    //
    if (g_bRunningOnNT5OrHigher)
    {
        LPCTSTR pIn = pwszIn;
        LPSTR pOut = pszOut;
        UINT cch = cchOut;
        
        while (*pIn && cch)
        {
            if (*pIn > ((TCHAR)127))
            {
#ifdef DEBUG
                SHUnicodeToAnsiCP(CP_ACPNOVALIDATE, pwszIn, pszOut, cchOut);
#else
                SHUnicodeToAnsi(pwszIn, pszOut, cchOut);
#endif
                return FALSE;
            }
            *pOut++ = (char)*pIn++;
            cch--;
        }

        // Null terminate the out buffer
        if (cch)
        {
            *pOut = '\0';
        }
        else if (cchOut)
        {
            *(pOut-1) = '\0';
        }

        // Everything was low ascii, no dbcs worries and it will always round-trip
        return TRUE;
    }
    else
    // but we probably don't want to change downlevel shell behavior
    // in this regard, so we keep that implementation:
    //
    {
        BOOL fRet = FALSE;
        WCHAR wszTemp[MAX_PATH];
        LPWSTR pwszTemp = wszTemp;
        UINT cchTemp = ARRAYSIZE(wszTemp);

        // We better have enough room for the buffer.
        if (ARRAYSIZE(wszTemp) < cchOut)
        {
            pwszTemp = (LPWSTR)LocalAlloc(LPTR, cchOut*sizeof(WCHAR));
            cchTemp = cchOut;
        }
        if (pwszTemp)
        {
#ifdef DEBUG
            SHUnicodeToAnsiCP(CP_ACPNOVALIDATE, pwszIn, pszOut, cchOut);
#else
            SHUnicodeToAnsi(pwszIn, pszOut, cchOut);
#endif
            SHAnsiToUnicode(pszOut, pwszTemp, cchTemp);
            fRet = lstrcmp(pwszIn, pwszTemp) == 0;     // are they the same?

            if (pwszTemp != wszTemp)
            {
                LocalFree(pwszTemp);
            }
        }
        return fRet;
    }
    
#else

    StrCpyN(pszOut, pwszIn, cchOut);
    return TRUE;
#endif
}

