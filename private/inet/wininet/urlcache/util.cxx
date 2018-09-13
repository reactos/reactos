/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    util.cxx

Abstract:

    Contains the class implementation of UTILITY classes.

Author:

    Madan Appiah (madana)  16-Nov-1994

Environment:

    User Mode - Win32

Revision History:

    Ahsan Kabir (akabir)    24-Nov-1997

--*/


#include <cache.hxx>

typedef BOOL (WINAPI *PFNGETFILEATTREX)(LPCTSTR, GET_FILEEX_INFO_LEVELS, LPVOID);

static char vszDot[] = ".";
static char vszDotDot[] = "..";
#ifdef UNIX
static char vszIndexFile[] = "index.dat";
#endif /* UNIX */

static char vszSHClassInfo[]=".ShellClassInfo";

static char vszCLSIDKey[]="CLSID";
static char vszCLSID[]="{FF393560-C2A7-11CF-BFF4-444553540000}";

static char vszUICLSIDKey[]="UICLSID";
static char vszUICLSID[]="{7BD29E00-76C1-11CF-9DD0-00A0C9034933}";

typedef HRESULT (*PFNSHFLUSHCACHE)(VOID);

#ifdef UNIX
extern void UnixGetValidParentPath(LPTSTR szDevice);
#endif /* UNIX */

/*-----------------------------------------------------------------------------
DeleteOneCachedFile

    Deletes a file belonging to the cache.

Arguments:

    lpszFileName: Fully qualified filename

Return Value:

    TRUE if successful. If FALSE, GetLastError() returns the error code.

Comments:

  ---------------------------------------------------------------------------*/
BOOL
DeleteOneCachedFile(
    LPSTR   lpszFileName,
    DWORD   dostEntry)
{

    if (dostEntry)
    {
        DWORD dostCreate = 0;
        LPWORD pwCreate = (LPWORD) &dostCreate;
        WIN32_FILE_ATTRIBUTE_DATA FileAttrData;

        switch (GetFileSizeAndTimeByName(lpszFileName, &FileAttrData))
        {
            case ERROR_SUCCESS:
                break;
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
                return TRUE;
            default:
                return FALSE;
        }                    
    
        FileTimeToDosDateTime(&FileAttrData.ftCreationTime, pwCreate, pwCreate+1);

        if (dostCreate != dostEntry)
           return TRUE; // not our file, so consider it done!
    }


    if(!DeleteFile(lpszFileName))
    {
        TcpsvcsDbgPrint (( DEBUG_ERRORS, "DeleteFile failed on %s, Error=%ld\n",
            lpszFileName, GetLastError()));

        switch (GetLastError())
        {
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
                return TRUE;
            default:
                return FALSE;
        }
    }
    else
    {
        TcpsvcsDbgPrint(( DEBUG_ERRORS, "Deleted %s\n", lpszFileName ));
        return TRUE;
    }
}


/*-----------------------------------------------------------------------------
DeleteCachedFilesInDir
  ---------------------------------------------------------------------------*/
DWORD DeleteCachedFilesInDir(
    LPSTR   lpszPath,
    DWORD   dwLevel
    )
{
    TCHAR PathFiles[MAX_PATH];
    TCHAR FullFileName[MAX_PATH];
    LPTSTR FullFileNamePtr;
    WIN32_FIND_DATA FindData;

    HANDLE FindHandle = INVALID_HANDLE_VALUE;

    // Since this has become a recursive call, we don't want to go more than 6 levels.
    if (dwLevel>5)
    {
        INET_ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }
    
    DWORD Error, len, cbUsed;
    BOOL fFindSuccess;

    DWORD cb = strlen(lpszPath);
    memcpy(PathFiles, lpszPath, cb + 1);

    if(!AppendSlashIfNecessary(PathFiles, cb)) 
    {
        Error = ERROR_INVALID_NAME;
        goto Cleanup;
    }

    memcpy(FullFileName, PathFiles, cb + 1);
    memcpy(PathFiles + cb, ALLFILES_WILDCARD_STRING, sizeof(ALLFILES_WILDCARD_STRING));

    FullFileNamePtr = FullFileName + lstrlen( (LPTSTR)FullFileName );

    if ( IsValidCacheSubDir( lpszPath))
        DisableCacheVu( lpszPath);

    FindHandle = FindFirstFile( (LPTSTR)PathFiles, &FindData );

    if( FindHandle == INVALID_HANDLE_VALUE ) 
    {
        Error = GetLastError();
        goto Cleanup;
    }

    cbUsed = (unsigned int)(FullFileNamePtr-FullFileName);
    FullFileName[MAX_PATH] = '\0';
    do
    {
        cb = strlen(FindData.cFileName);
        if (cb+cbUsed+1 > MAX_PATH)
        {
            // Subtracting 1 extra so that the null terminator doesn't get overwritten
            cb = MAX_PATH - cbUsed - 2;
        }
        memcpy(FullFileNamePtr, FindData.cFileName, cb+1);

#ifndef UNIX
        if (!(!strnicmp(FindData.cFileName, vszDot, sizeof(vszDot)-1) ||
            !strnicmp(FindData.cFileName, vszDotDot, sizeof(vszDotDot)-1))) 
#else
        if (!(!strnicmp(FindData.cFileName, vszDot, sizeof(vszDot)-1) ||
            !strnicmp(FindData.cFileName, vszIndexFile, sizeof(vszIndexFile)-1) || 
            !strnicmp(FindData.cFileName, vszDotDot, sizeof(vszDotDot)-1))) 
#endif /* UNIX */
        {
            if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                Error = DeleteCachedFilesInDir(FullFileName, dwLevel + 1);
                if (Error!=ERROR_SUCCESS)
                {
                    goto Cleanup;
                }
                SetFileAttributes(FullFileName, FILE_ATTRIBUTE_DIRECTORY);
                RemoveDirectory(FullFileName);
            }
            else
            {
                DeleteOneCachedFile( (LPTSTR)FullFileName, 0);
            }
        }

        //
        // find next file.
        //

    } while (FindNextFile( FindHandle, &FindData ));

    Error = GetLastError();
    if( Error == ERROR_NO_MORE_FILES) 
    {
        Error = ERROR_SUCCESS;
    }

Cleanup:

    if( FindHandle != INVALID_HANDLE_VALUE ) 
    {
        FindClose( FindHandle );
    }

    if( Error != ERROR_SUCCESS ) 
    {
        TcpsvcsDbgPrint(( DEBUG_ERRORS,
            "DeleteCachedFilesInDir failed, %ld.\n",
                Error ));
    }

    return( Error );
}


/*-----------------------------------------------------------------------------
AppendSlashIfNecessary
  ---------------------------------------------------------------------------*/
BOOL AppendSlashIfNecessary(LPSTR szPath, DWORD& cbPath)
{
    if (cbPath > (MAX_PATH-2)) 
        return FALSE;
    if (szPath[cbPath-1] != DIR_SEPARATOR_CHAR)
    {
        szPath[cbPath++] = DIR_SEPARATOR_CHAR;
        szPath[cbPath] = '\0';
    }
    return TRUE;
}


/*-----------------------------------------------------------------------------
EnableCachevu
  ---------------------------------------------------------------------------*/
BOOL EnableCacheVu(LPSTR szPath, DWORD dwContainer)
{       
    DWORD cbPath = strlen(szPath);
    CHAR szDesktopIni[MAX_PATH];
    DWORD dwFileAttributes;

    HMODULE hInstShell32 = 0;
    PFNSHFLUSHCACHE pfnShFlushCache = NULL;

#define DESIRED_ATTR (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)

    // Calls with non-existant directory allowed and return false.
    dwFileAttributes = GetFileAttributes(szPath);
    if (dwFileAttributes == 0xFFFFFFFF)
        return FALSE;
    
    // Always be set to enable cachevu.
    SetFileAttributes(szPath, FILE_ATTRIBUTE_SYSTEM);
    
    // Path to DESKTOP_INI_FILENAME
    memcpy(szDesktopIni, szPath, cbPath + 1);           
    AppendSlashIfNecessary(szDesktopIni, cbPath);
            
    // Correct location for desktop.ini.
    memcpy(szDesktopIni + cbPath, DESKTOPINI_FILE_NAME, sizeof(DESKTOPINI_FILE_NAME));

    // Check for existing desktop.ini
    dwFileAttributes = GetFileAttributes(szDesktopIni);

    if (dwFileAttributes == 0xFFFFFFFF)
    {
        dwFileAttributes = 0;

        // Always write out the UICLSID
        WritePrivateProfileString(vszSHClassInfo,  vszUICLSIDKey,  vszUICLSID,  szDesktopIni);    

        // HISTORY requires an additional CLSID.
        if (dwContainer == HISTORY)
            WritePrivateProfileString(vszSHClassInfo,  vszCLSIDKey,  vszCLSID,  szDesktopIni);    

        // Flush buffer - problems on Win95 if you don't.
        WritePrivateProfileString(NULL, NULL, NULL,  szDesktopIni);
    }

    if ((dwFileAttributes & DESIRED_ATTR) != DESIRED_ATTR)
    {
        // Should be hidden, read-only and system for cachevu to work correctly.
        SetFileAttributes(szDesktopIni, DESIRED_ATTR);
    }
/*
    BUGBUG - taking this code out for raid # 45710.
    // We now need to notify the shell that a new desktop.ini has been created.
    hInstShell32 = GetModuleHandle("shell32.dll");
    if (hInstShell32)
    {    
        pfnShFlushCache = (PFNSHFLUSHCACHE) GetProcAddress(hInstShell32, (LPSTR) 526);
        if (pfnShFlushCache)
        {
            __try
            {
                (*pfnShFlushCache)();
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
            }
            ENDEXCEPT
        }
    }
*/
    return TRUE;

}

/*-----------------------------------------------------------------------------
IsValidCacheSubDir
  ---------------------------------------------------------------------------*/
BOOL IsValidCacheSubDir(LPSTR szPath)
{
    DWORD dwFileAttributes, cb, cbPath;
    CHAR szDesktopIni[MAX_PATH];
    CHAR szCLSID     [MAX_PATH];
    CHAR szWindowsDir[MAX_PATH];
    CHAR szSystemDir [MAX_PATH];

    cbPath = strlen(szPath);
            
    // Root, Windows or System directories
    // are decidedly not cache subdirectories.
    cb = GetWindowsDirectory(szWindowsDir, MAX_PATH);
    if (!cb || cb>MAX_PATH)
    {
        INET_ASSERT(FALSE);
        return FALSE;
    }
    AppendSlashIfNecessary(szWindowsDir, cb);

    cb = GetSystemDirectory(szSystemDir, MAX_PATH);
    AppendSlashIfNecessary(szSystemDir, cb);

    if (cbPath < 4 
        || !strnicmp(szPath, szWindowsDir, cbPath)
        || !strnicmp(szPath, szSystemDir, cbPath))
    {
        INET_ASSERT(FALSE);
        return FALSE;
    }

    // Path to DESKTOP_INI_FILENAME
    memcpy(szDesktopIni, szPath, cbPath + 1);           
    AppendSlashIfNecessary(szDesktopIni, cbPath);
    
    // Correct location for desktop.ini.
    memcpy(szDesktopIni + cbPath, DESKTOPINI_FILE_NAME, sizeof(DESKTOPINI_FILE_NAME));

    // Check for existing desktop.ini
    dwFileAttributes = GetFileAttributes(szDesktopIni);

    // No desktop.ini found or system attribute not set.
    if (dwFileAttributes == 0xFFFFFFFF)
    {
        return FALSE;
    }

    // Found UICLSID (CONTENT cachevu) ?
    if (GetPrivateProfileString(vszSHClassInfo,  vszUICLSIDKey,  
                                "", szCLSID, MAX_PATH, szDesktopIni)
        && !strcmp(szCLSID, vszUICLSID)) 
    {
        return TRUE;
    }

    return FALSE;
}


/*-----------------------------------------------------------------------------
DisableCachevu
  ---------------------------------------------------------------------------*/
BOOL DisableCacheVu(LPSTR szPath)
{
    DWORD cbPath = strlen(szPath);
    CHAR szDesktopIni[MAX_PATH];

    // Path to DESKTOP_INI_FILENAME
    memcpy(szDesktopIni, szPath, cbPath + 1);           
    AppendSlashIfNecessary(szDesktopIni, cbPath);
            
    // Correct location for desktop.ini.
    memcpy(szDesktopIni + cbPath, DESKTOPINI_FILE_NAME, sizeof(DESKTOPINI_FILE_NAME));
    SetFileAttributes(szDesktopIni, FILE_ATTRIBUTE_NORMAL);
    DeleteFile(szDesktopIni);
    return TRUE;
}

/*-----------------------------------------------------------------------------
StripTrailingWhiteSpace
  ---------------------------------------------------------------------------*/
VOID StripTrailingWhiteSpace(LPSTR szString, LPDWORD pcb)
{
    INET_ASSERT(szString);

    if (*pcb == 0)
        return;

    CHAR* ptr = szString + *pcb - 1;

    while (*ptr == ' ')
    {
        ptr--;
        if (--(*pcb) == 0)
            break;
    }
    *(ptr+1) = '\0';
}


/* PerformOperationOverUrlCache-----------------------

The purpose of this function is to iterate through the content cache and perform the same action (here, called
an operation) on each entry in the cache.

This function takes all the parameters that FindFirstUrlCacheEntryEx accepts, 
plus two more:

op              -- This is of type CACHE_OPERATOR, discussed below
pOperatorData   -- a pointer to an array of data that the calling process and op use to collect/maintain info
 */

/* CACHE_OPERATOR
    is a pointer to function, that takes three arguments(pointer to a cache entry, cache entry size, and a pointer to 
    state data.

    The operator can perform whatever operation (move/copy/data collection) it wishes on the supplied cache entry.
    It must return TRUE if the operation has succeeded and PerformOperationOverUrlCache can continue to iterate through
    the cache, FALSE otherwise.

    pOpData can be null, or a cast pointer to whatever structure the operator will use to maintain state information.
        
    PerformOperationOverUrlCache guarantees that each cache entry will have sufficient space for its information.
*/ 

typedef BOOL (*CACHE_OPERATOR)(INTERNET_CACHE_ENTRY_INFO* pcei, PDWORD pcbcei, PVOID pOpData);


// hAdjustMemory is a helper function
// that ensures that the buffer used by PerformOperationOverUrlCache
// is large enough to hold all of a cache entry's info

BOOL hAdjustMemory(PBYTE pbSrc, PDWORD pcbAvail, LPINTERNET_CACHE_ENTRY_INFO* pbNew, PDWORD pcbNeeded)
{
    if ((PBYTE)*pbNew!=pbSrc)
    {
        FREE_MEMORY(*pbNew);
    }
    do
    {
        *pcbAvail += 1024;
    } 
    while (*pcbAvail < *pcbNeeded);
    *pcbNeeded = *pcbAvail;
    *pbNew = (LPINTERNET_CACHE_ENTRY_INFO)ALLOCATE_FIXED_MEMORY(*pcbAvail);
    return (*pbNew!=NULL);
}


// PerformOperationOverUrlCache
// described above
// uses FindFirstUrlCacheEntryEx and FindNext as any other wininet client would.
// and passes a complete cache entry to the operator for processing

BOOL PerformOperationOverUrlCacheA(
    IN     PCSTR     pszUrlSearchPattern,
    IN     DWORD     dwFlags,
    IN     DWORD     dwFilter,
    IN     GROUPID   GroupId,
    OUT    PVOID     pReserved1,
    IN OUT PDWORD    pdwReserved2,
    IN     PVOID     pReserved3,
    IN       CACHE_OPERATOR op, 
    IN OUT PVOID     pOperatorData
    )
{
    BOOL fResult = FALSE;

    BYTE buffer[sizeof(INTERNET_CACHE_ENTRY_INFO) + 1024];
    DWORD cbAvail = sizeof(buffer);
    DWORD cbCEI = cbAvail;
    LPINTERNET_CACHE_ENTRY_INFO pCEI = (LPINTERNET_CACHE_ENTRY_INFO)buffer;
    HANDLE hFind = NULL;
    
    hFind = FindFirstUrlCacheEntryEx(pszUrlSearchPattern, 
                                    dwFlags,
                                    dwFilter,
                                    GroupId,
                                    pCEI, 
                                    &cbCEI,
                                    pReserved1,
                                    pdwReserved2,
                                    pReserved3);
    if (!hFind && (GetLastError()!=ERROR_INSUFFICIENT_BUFFER) && hAdjustMemory(buffer, &cbAvail, &pCEI, &cbCEI))
    {
        hFind = FindFirstUrlCacheEntryEx(pszUrlSearchPattern, 
                                    dwFlags,
                                    dwFilter,
                                    GroupId,
                                    pCEI, 
                                    &cbCEI,
                                    pReserved1,
                                    pdwReserved2,
                                    pReserved3);
    }

    if (hFind!=NULL)
    {
        do
        {
            fResult = op(pCEI, &cbCEI, pOperatorData);
            if (fResult)
            {
                cbCEI = cbAvail;
                fResult = FindNextUrlCacheEntryEx(hFind, pCEI, &cbCEI, NULL, NULL, NULL);
                if (!fResult && (GetLastError()==ERROR_INSUFFICIENT_BUFFER) && hAdjustMemory(buffer, &cbAvail, &pCEI, &cbCEI))
                {
                    fResult = FindNextUrlCacheEntryEx(hFind, pCEI, &cbCEI, NULL, NULL, NULL);
                }
            } 
        }
        while (fResult);
        FindCloseUrlCache(hFind);

        if (GetLastError()==ERROR_NO_MORE_ITEMS)
        {
            fResult = TRUE;
        } 
    }

    if (pCEI!=(LPINTERNET_CACHE_ENTRY_INFO)buffer)
    {
        FREE_MEMORY(pCEI);
    }
    return fResult;
}

// ------ MoveCachedFiles ---------------------------------------------------------------------------------------
// Purpose: Moves as many files as possible from the current Temporary Internet Files to the new location


// State information required for the move operation
struct MOVE_OP_STATE
{
    TCHAR szNewPath[MAX_PATH];
    TCHAR szOldPath[MAX_PATH];
    DWORD ccNewPath;
    DWORD ccOldPath;
    DWORDLONG dlCacheSize;
    DWORD dwClusterSizeMinusOne;
    DWORD dwClusterSizeMask;
};

// Helper function that, 
// given a string pointer, 
// returns the next occurrence of DIR_SEPARATOR_CHAR ('/' || '\\')
PTSTR hScanPastSeparator(PTSTR pszPath)
{
    while (*pszPath && *pszPath!=DIR_SEPARATOR_CHAR)
    {
        pszPath++;
    }
    if (*pszPath)
    {
        return pszPath+1;
    }
    return NULL;
}

// Helper function that,
// given a path,
// ensures that all the directories in the path exist
BOOL hConstructSubDirs(PTSTR pszBase)
{
    PTSTR pszLast = hScanPastSeparator(pszBase);
    PTSTR pszNext = pszLast;
    while ((pszNext=hScanPastSeparator(pszNext))!=NULL)
    {
        *(pszNext-1) = '\0';
        CreateDirectory(pszBase, NULL);
        *(pszNext-1) = DIR_SEPARATOR_CHAR;
        pszLast = pszNext;
    }
    return TRUE;
}

// MoveOperation
// actually moves a cached file to the new location

BOOL MoveOperation(LPINTERNET_CACHE_ENTRY_INFO pCEI, PDWORD pcbCEI, PVOID pOpData)
{
    MOVE_OP_STATE* pmos = (MOVE_OP_STATE*)pOpData;
    BOOL fResult = TRUE;

    if (pCEI->lpszLocalFileName)
    {
        if (!strnicmp(pCEI->lpszLocalFileName, pmos->szOldPath, pmos->ccOldPath))
        {
            // Copy the file
            lstrcpy(pmos->szNewPath + pmos->ccNewPath, pCEI->lpszLocalFileName + pmos->ccOldPath);        
            fResult = CopyFile(pCEI->lpszLocalFileName, pmos->szNewPath, FALSE);
            if (!fResult && GetLastError()==ERROR_PATH_NOT_FOUND)
            {
                if (hConstructSubDirs(pmos->szNewPath))
                {
                    fResult = CopyFile(pCEI->lpszLocalFileName, pmos->szNewPath, FALSE);
                }
            }
            // If the move was successful, we need to adjust the size of the new cache
            if (fResult)
            {
                fResult = FALSE;
                
                HANDLE h1 = CreateFile(pCEI->lpszLocalFileName, 
                                      GENERIC_READ,
                                      0,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL,
                                      NULL);

                // If we can't open the original file, then the new file will never
                // get scavenged because we'll never be able to match creation times
                if (h1!=INVALID_HANDLE_VALUE)
                {
                    HANDLE h2 = CreateFile(pmos->szNewPath, 
                                      GENERIC_WRITE,
                                      0,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL,
                                      NULL);
                    if (h2!=INVALID_HANDLE_VALUE)
                    {
                        FILETIME ft;
                        if (GetFileTime(h1, &ft, NULL, NULL))
                        {
                            fResult = SetFileTime(h2, &ft, NULL, NULL);
                        }
                        CloseHandle(h2);
                    }
                    CloseHandle(h1);
                }
            }

            // If we haven't been able to set the create time, then we've got a problem
            // we'd sooner not deal with.
            if (!fResult)
            {
                DeleteUrlCacheEntry(pCEI->lpszSourceUrlName);
                DeleteFile(pmos->szNewPath);
            }
            else
            {
                pmos->dlCacheSize += ((LONGLONG) (pCEI->dwSizeLow + pmos->dwClusterSizeMinusOne) 
                                        & pmos->dwClusterSizeMask);
            }

            // Delete the old one
            DeleteFile(pCEI->lpszLocalFileName);
        }
    }
    return TRUE;
}


DWORD
MoveCachedFiles(
    LPSTR     pszOldPath,
    LPSTR     pszNewPath
)
{
    MOVE_OP_STATE mos;
    INET_ASSERT(pszOldPath && pszNewPath);

    mos.ccNewPath = lstrlen(pszNewPath);
    memcpy(mos.szNewPath, pszNewPath, mos.ccNewPath*sizeof(TCHAR));
    AppendSlashIfNecessary(mos.szNewPath, mos.ccNewPath);
    memcpy(mos.szNewPath + mos.ccNewPath, CONTENT_VERSION_SUBDIR, sizeof(CONTENT_VERSION_SUBDIR)*sizeof(TCHAR));
    mos.ccNewPath += sizeof(CONTENT_VERSION_SUBDIR)-1;
    AppendSlashIfNecessary(mos.szNewPath, mos.ccNewPath);

    mos.ccOldPath = lstrlen(pszOldPath);
    memcpy(mos.szOldPath, pszOldPath, mos.ccOldPath*sizeof(TCHAR));
    AppendSlashIfNecessary(mos.szOldPath, mos.ccOldPath);

    mos.dlCacheSize = 0;
    GetDiskInfo(mos.szNewPath, &mos.dwClusterSizeMinusOne, NULL, NULL);
    mos.dwClusterSizeMinusOne--;
    mos.dwClusterSizeMask = ~mos.dwClusterSizeMinusOne;
    
    GlobalUrlContainers->WalkLeakList(CONTENT);

    // We don't need to get all the information about each and every entry.
    PerformOperationOverUrlCacheA(
        NULL, 
        FIND_FLAGS_RETRIEVE_ONLY_FIXED_AND_FILENAME,
        NORMAL_CACHE_ENTRY | STICKY_CACHE_ENTRY | SPARSE_CACHE_ENTRY,
        NULL,
        NULL,
        NULL,
        NULL,
        MoveOperation, 
        (PVOID)&mos);

    GlobalUrlContainers->SetCacheSize(CONTENT, mos.dlCacheSize);
    
    // Copy desktop.ini and index.dat, since these aren't cached
    TCHAR szFile[MAX_PATH];
    DWORD ccOldPath = lstrlen(pszOldPath);
    memcpy(szFile, pszOldPath, ccOldPath);
    AppendSlashIfNecessary(szFile, ccOldPath);
    memcpy(szFile + ccOldPath, MEMMAP_FILE_NAME, sizeof(MEMMAP_FILE_NAME));
    memcpy(mos.szNewPath + mos.ccNewPath, MEMMAP_FILE_NAME, sizeof(MEMMAP_FILE_NAME));
    CopyFile(szFile, mos.szNewPath, FALSE);

    memcpy(szFile + ccOldPath, DESKTOPINI_FILE_NAME, sizeof(DESKTOPINI_FILE_NAME));
    memcpy(mos.szNewPath + mos.ccNewPath, DESKTOPINI_FILE_NAME, sizeof(DESKTOPINI_FILE_NAME));
    CopyFile(szFile, mos.szNewPath, FALSE);

    return ERROR_SUCCESS;
}


/*-----------------------------------------------------------------------------
IsCorrectUser

Routine Description:

    checks to see from the headers whether there is any username in there and
    whether it matches the currently logged on user. If no one is logged on a
    default username string is used

Arguments:

    lpszHeaderInfo: headers to check

    dwheaderSize:   size of the headers buffer

Return Value:

    BOOL


---------------------------------------------------------------------------*/
BOOL
IsCorrectUser(
    IN LPSTR lpszHeaderInfo,
    IN DWORD dwHeaderSize
    )
{
    LPSTR lpTemp, lpTemp2;

    INET_ASSERT (lpszHeaderInfo);

    lpTemp = lpszHeaderInfo+dwHeaderSize-1;

    // start searching backwards

    while (lpTemp >= lpszHeaderInfo) {

        if (*lpTemp ==':') {
            // If this is less than the expected header:
            // then we know that there is no such usernameheader 
            // <MH> i.e. it's not a peruseritem so allow access</MH>

            if ((DWORD)PtrDifference((lpTemp+1), lpszHeaderInfo) < (sizeof(vszUserNameHeader)-1)) {
                TcpsvcsDbgPrint((DEBUG_CONTAINER,
                    "IsCorrectUser (Util.cxx): Didn't find header <lpTemp = 0x%x %s> <lpszHeaderInfo = 0x%x %s> <vszCurrentUser = %s> <PtrDifference = %d> <sizeof(vszUserNameHeader)-1) = %d>\r\n",
                    lpTemp,
                    lpTemp,
                    lpszHeaderInfo,
                    lpszHeaderInfo,
                    vszCurrentUser,
                    PtrDifference(lpTemp, lpszHeaderInfo),
                    (sizeof(vszUserNameHeader)-1)
                    ));
                return (TRUE); // No such header. just ay it is OK
            }

            // point this puppy to the expected header start
            lpTemp2 = lpTemp - (sizeof(vszUserNameHeader)-2);

            // if the earlier char is not a white space [0x9-0xd or 0x20]
            // then this is not the beginning of the header
            // <MH> Also need to check for the first header which would not 
            // have whitespace preceding it. Want to first check lpTemp2 ==
            // lpszheaderInfo to prevent underflowing when dereferencing.</MH>

            if (((lpTemp2) == lpszHeaderInfo) || isspace(*(lpTemp2-1))) {

                // we have the beginning of a header
                if (!strnicmp(lpTemp2
                                , vszUserNameHeader
                                , sizeof(vszUserNameHeader)-1)) {

                    // right header, let us see whether this is the right person
                    if(!strnicmp(lpTemp+1, vszCurrentUser, vdwCurrentUserLen)) {
                        TcpsvcsDbgPrint((DEBUG_CONTAINER,
                            "IsCorrectUser (Util.cxx): Match!! %s header == %s current user.\r\n",
                            lpTemp+1,
                            vszCurrentUser
                            ));
                    
                        return (TRUE); // right guy
                    }
                    else {
                        TcpsvcsDbgPrint((DEBUG_CONTAINER,
                            "IsCorrectUser (Util.cxx): No match!! %s header != %s current user.\r\n",
                            lpTemp+1,
                            vszCurrentUser
                            ));

                    }

                    return(FALSE); // wrong guy
                }
            }
        }
        --lpTemp;
    }

    return (TRUE); // there was no UserName header, just say it is OK
}

#ifndef UNICODE
#define SZ_GETDISKFREESPACEEX   "GetDiskFreeSpaceExA"
#define SZ_WNETUSECONNECTION    "WNetUseConnectionA"
#define SZ_WNETCANCELCONNECTION "WNetCancelConnectionA"
#else
#define SZ_GETDISKFREESPACEEX   "GetDiskFreeSpaceExW"
#define SZ_WNETUSECONNECTION    "WNetUseConnectionW"
#define SZ_WNETCANCELCONNECTION "WNetCancelConnectionW"
#endif

typedef BOOL (WINAPI *PFNGETDISKFREESPACEEX)(LPCTSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);
typedef BOOL (WINAPI *PFNWNETUSECONNECTION)(HWND, LPNETRESOURCE, PSTR, PSTR, DWORD, PSTR, PDWORD, PDWORD);
typedef BOOL (WINAPI *PFNWNETCANCELCONNECTION)(LPCTSTR, BOOL);

BOOL EstablishFunction(PTSTR pszModule, PTSTR pszFunction, PFN* pfn)
{
    if (*pfn==(PFN)-1)
    {
        *pfn = NULL;
        HMODULE ModuleHandle = GetModuleHandle(pszModule);
        if (ModuleHandle)
        {
            *pfn = (PFN)GetProcAddress(ModuleHandle, pszFunction);
        }
    }        

    return (*pfn!=NULL);
}


// GetPartitionClusterSize

// GetDiskFreeSpace has the annoying habit of lying about the layout
// of the drive; thus we've been ending up with bogus sizes for the cluster size.
// You can't imagine how annoying it is to think you've a 200 MB cache, but it
// starts scavenging at 20MB.

// This function will, if given reason to doubt the veracity of GDFS, go straight 
// to the hardware and get the information for itself, otherwise return the passed-in
// value.

// The code that follows is heavily doctored from msdn sample code. Copyright violation? I think not.

static PFNGETDISKFREESPACEEX pfnGetDiskFreeSpaceEx = (PFNGETDISKFREESPACEEX)-1;
#define VWIN32_DIOC_DOS_DRIVEINFO   6

typedef struct _DIOC_REGISTERS 
{
    DWORD reg_EBX;
    DWORD reg_EDX;
    DWORD reg_ECX;
    DWORD reg_EAX;
    DWORD reg_EDI;
    DWORD reg_ESI;
    DWORD reg_Flags;
} 
DIOC_REGISTERS, *PDIOC_REGISTERS;

// Important: All MS_DOS data structures must be packed on a 
// one-byte boundary. 

#pragma pack(1) 

typedef struct 
_DPB {
    BYTE    dpb_drive;          // Drive number (1-indexed)
    BYTE    dpb_unit;           // Unit number
    WORD    dpb_sector_size;    // Size of sector in bytes
    BYTE    dpb_cluster_mask;   // Number of sectors per cluster, minus 1
    BYTE    dpb_cluster_shift;  // The stuff after this, we don't really care about. 
    WORD    dpb_first_fat;
    BYTE    dpb_fat_count;
    WORD    dpb_root_entries;
    WORD    dpb_first_sector;
    WORD    dpb_max_cluster;
    WORD    dpb_fat_size;
    WORD    dpb_dir_sector;
    DWORD   dpb_reserved2;
    BYTE    dpb_media;
    BYTE    dpb_first_access;
    DWORD   dpb_reserved3;
    WORD    dpb_next_free;
    WORD    dpb_free_cnt;
    WORD    extdpb_free_cnt_hi;
    WORD    extdpb_flags;
    WORD    extdpb_FSInfoSec;
    WORD    extdpb_BkUpBootSec;
    DWORD   extdpb_first_sector;
    DWORD   extdpb_max_cluster;
    DWORD   extdpb_fat_size;
    DWORD   extdpb_root_clus;
    DWORD   extdpb_next_free;
} 
DPB, *PDPB;

#pragma pack()

DWORD GetPartitionClusterSize(PTSTR szDevice, DWORD dwClusterSize)
{
    switch (GlobalPlatformType)
    {
    case PLATFORM_TYPE_WIN95:
        // If GetDiskFreeSpaceEx is present _and_ we're running Win9x, this implies
        // that we must be doing OSR2 or later. We can trust earlier versions 
        // of the GDFS (we think; this assumption may be invalid.)

        // Since Win95 can't read NTFS drives, we'll freely assume we're reading a FAT drive.
        // Basically, we're performing an MSDOS INT21 call to get the drive partition record. Joy.
        
        if (pfnGetDiskFreeSpaceEx)
        {
            HANDLE hDevice;
            DIOC_REGISTERS reg;
            BYTE buffer[sizeof(WORD)+sizeof(DPB)];
            PDPB pdpb = (PDPB)(buffer + sizeof(WORD));
    
            BOOL fResult;
            DWORD cb;

            // We must always have a drive letter in this case
            int nDrive = *szDevice - TEXT('A') + 1;  // Drive number, 1-indexed

            hDevice = CreateFile("\\\\.\\vwin32", 0, 0, NULL, 0, FILE_FLAG_DELETE_ON_CLOSE, NULL);

            if (hDevice!=INVALID_HANDLE_VALUE)
            {
                reg.reg_EDI = PtrToUlong(buffer);
                reg.reg_EAX = 0x7302;        
                reg.reg_ECX = sizeof(buffer);
                reg.reg_EDX = (DWORD) nDrive; // drive number (1-based) 
                reg.reg_Flags = 0x0001;     // assume error (carry flag is set) 

                fResult = DeviceIoControl(hDevice, 
                                          VWIN32_DIOC_DOS_DRIVEINFO,
                                          &reg, sizeof(reg), 
                                          &reg, sizeof(reg), 
                                          &cb, 0);

                if (fResult && !(reg.reg_Flags & 0x0001))
                {
                    // no error if carry flag is clear
                    dwClusterSize = DWORD((pdpb->dpb_cluster_mask+1)*pdpb->dpb_sector_size);
                }
                CloseHandle(hDevice);
            }
        }
        break;

    default:
        // Do nothing. Trust the value we've been passed.
        // UNIX guys will have to treat this separately.

        // For NT, however, this might be another issue. We can't use the DOS INT21.
        // Questions:
        // NT5 (but not NT4) supports FAT32; will we get honest answers? Apparently, yes.
        // NT4/5: NTFS drives and other FAT drives -- do we still get honest answers? Investigation
        // so far says, Yes. 
        break;
    }
    
    return dwClusterSize;
}


/* GetDiskInfo
    A nice way to get volume information
*/
BOOL GetDiskInfoA(PTSTR pszPath, PDWORD pdwClusterSize, PDWORDLONG pdlAvail, PDWORDLONG pdlTotal)
{
    static PFNWNETUSECONNECTION pfnWNetUseConnection = (PFNWNETUSECONNECTION)-1;
    static PFNWNETCANCELCONNECTION pfnWNetCancelConnection = (PFNWNETCANCELCONNECTION)-1;

    if (!pszPath)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    INET_ASSERT(pdwClusterSize || pdlAvail || pdlTotal);
    // If GetDiskFreeSpaceExA is available, we can be confident we're running W95OSR2+ || NT4
    EstablishFunction(TEXT("KERNEL32"), SZ_GETDISKFREESPACEEX, (PFN*)&pfnGetDiskFreeSpaceEx);
  
    BOOL fRet = FALSE;
    TCHAR szDevice[MAX_PATH];
    PTSTR pszGDFSEX = NULL;
   
    if (*pszPath==DIR_SEPARATOR_CHAR)
    {
        // If we're dealing with a cache that's actually located on a network share, 
        // that's fine so long as we have GetDiskFreeSpaceEx at our disposal.
        // _However_, if we need the cluster size on Win9x, we'll need to use
        // INT21 stuff (see above), even if we have GDFSEX available, so we need to map
        // the share to a local drive.
        
        if (pfnGetDiskFreeSpaceEx 
            && !((GlobalPlatformType==PLATFORM_TYPE_WIN95) && pdwClusterSize))
        {
            DWORD cbPath = lstrlen(pszPath);
            cbPath -= ((pszPath[cbPath-1]==DIR_SEPARATOR_CHAR) ? 1 : 0);
            if (cbPath>MAX_PATH-2)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
            memcpy(szDevice, pszPath, cbPath);
            szDevice[cbPath] = DIR_SEPARATOR_CHAR;
            cbPath++;
            szDevice[cbPath] = '\0';
            pszGDFSEX = szDevice;
        }
        else
        {
            if (!(EstablishFunction(TEXT("MPR"), SZ_WNETUSECONNECTION, (PFN*)&pfnWNetUseConnection)
                &&
               EstablishFunction(TEXT("MPR"), SZ_WNETCANCELCONNECTION, (PFN*)&pfnWNetCancelConnection)))
            {
                return FALSE;
            }

           // If it's a UNC, map it to a local drive for backwards compatibility
            NETRESOURCE nr = { 0, RESOURCETYPE_DISK, 0, 0, szDevice, pszPath, NULL, NULL };
            DWORD cbLD = sizeof(szDevice);
            DWORD dwNull;
            if (pfnWNetUseConnection(NULL, 
                          &nr, 
                          NULL, 
                          NULL, 
                          CONNECT_INTERACTIVE | CONNECT_REDIRECT, 
                          szDevice,
                          &cbLD,
                          &dwNull)!=ERROR_SUCCESS)
            {
                SetLastError(ERROR_NO_MORE_DEVICES);        
                return FALSE;
            }
        }
    }
    else
    {
        memcpy(szDevice, pszPath, sizeof(TEXT("?:\\")));
        szDevice[3] = '\0';
        pszGDFSEX = pszPath;
    }
    if (*szDevice!=DIR_SEPARATOR_CHAR)
    {
        *szDevice = (TCHAR)CharUpper((LPTSTR)*szDevice);
    }

#ifdef UNIX
    /* On Unix, GetDiskFreeSpace and GetDiskFreeSpaceEx will work successfully
     * only if the path exists. So, let us pass a path that exists
     */
    UnixGetValidParentPath(szDevice);
#endif /* UNIX */

    // I hate goto's, and this is a way to avoid them...
    for (;;)
    {
        DWORDLONG cbFree = 0, cbTotal = 0;
    
        if (pfnGetDiskFreeSpaceEx && (pdlTotal || pdlAvail))
        {
            ULARGE_INTEGER ulFree, ulTotal;

            // BUG BUG BUG Is the following problematic? Also, we'll need to add checks to make sure that 
            // the  cKBlimit fits a DWORD (in the obscene if unlikely case drive spaces grow that large)
            // For instance, if this is a per user system with a non-shared cache, we might want to change
            // the ratios.
            INET_ASSERT(pszGDFSEX);
            fRet = pfnGetDiskFreeSpaceEx(pszGDFSEX, &ulFree, &ulTotal, NULL);

            // HACK Some versions of GetDiskFreeSpaceEx don't accept the whole directory; they
            // take only the drive letter. Pfft.
            if (!fRet)
            {
                fRet = pfnGetDiskFreeSpaceEx(szDevice, &ulFree, &ulTotal, NULL);
            }

            if (fRet)
            {
                cbFree = ulFree.QuadPart;
                cbTotal = ulTotal.QuadPart;
            }
        }

        if ((!fRet) || pdwClusterSize)
        {
            DWORD dwSectorsPerCluster, dwBytesPerSector, dwFreeClusters, dwClusters, dwClusterSize;
            if (!GetDiskFreeSpace(szDevice, &dwSectorsPerCluster, &dwBytesPerSector, &dwFreeClusters, &dwClusters))
            {
                fRet = FALSE;
                break;
            }
            
            dwClusterSize = dwBytesPerSector * dwSectorsPerCluster;

            if (!fRet)
            {
                cbFree = (DWORDLONG)dwClusterSize * (DWORDLONG)dwFreeClusters;
                cbTotal = (DWORDLONG)dwClusterSize * (DWORDLONG)dwClusters;
            }
            
            if (pdwClusterSize)
            {
                *pdwClusterSize = GetPartitionClusterSize(szDevice, dwClusterSize);
            }
        }

        if (pdlTotal)
        {
             *pdlTotal = cbTotal;
        }
        if (pdlAvail)
        {
             *pdlAvail = cbFree;
        }
        fRet = TRUE;
        break;
    };
    
    // We've got the characteristics. Now delete local device connection, if any.
    if (*pszPath==DIR_SEPARATOR_CHAR && !pfnGetDiskFreeSpaceEx)
    {
        pfnWNetCancelConnection(szDevice, FALSE);
    }

    return fRet;
}


// -- ScanToLastSeparator
// Given a path, and a pointer within the path, discover where the path separator prior to the path
// is located and return the pointer to it. If there is none, return NULL.

BOOL ScanToLastSeparator(PTSTR pszPath, PTSTR* ppszCurrent)
{
    PTSTR pszActual = *ppszCurrent;
    pszActual--;
    while ((pszActual>(pszPath+1)) && (*pszActual!=DIR_SEPARATOR_CHAR))
    {
        pszActual--;
    }
    if ((*pszActual==DIR_SEPARATOR_CHAR) && (pszActual!=*ppszCurrent))
    {
        *ppszCurrent = pszActual;
        return TRUE;
    }

    return FALSE;
}

