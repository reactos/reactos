/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    cachapiw.cxx

Abstract:

    contains the UNICODE version of cache mangemant APIs.

Author:

    Madan Appiah (madana)  12-Dec-1994

Environment:

    User Mode - Win32

Revision History:

    Ahsan Kabir (akabir)   Dec-1997
--*/

#include <cache.hxx>
#include <w95wraps.h>

#define NUMBER_MEMBERS 4


const BYTE bOffsetTable[NUMBER_MEMBERS] = 
    {
        (BYTE)&(((LPINTERNET_CACHE_ENTRY_INFOW)NULL)->lpszSourceUrlName),
        (BYTE)&(((LPINTERNET_CACHE_ENTRY_INFOW)NULL)->lpszLocalFileName),
        (BYTE)&(((LPINTERNET_CACHE_ENTRY_INFOW)NULL)->lpHeaderInfo),
        (BYTE)&(((LPINTERNET_CACHE_ENTRY_INFOW)NULL)->lpszFileExtension)
    };
    
DWORD
TransformA2W(
    IN LPINTERNET_CACHE_ENTRY_INFOA pCEIA,
    IN DWORD cbCEIA,
    OUT LPINTERNET_CACHE_ENTRY_INFOW pCEIW,
    OUT LPDWORD pcbCEIW
    )
{
    DWORD cbSize = sizeof(INTERNET_CACHE_ENTRY_INFOW);
    DWORD cc;

    if (!pCEIW || (*pcbCEIW<sizeof(INTERNET_CACHE_ENTRY_INFOW)))
    {
        *pcbCEIW = 0;
        cc = 0;
    }
    else
    {
        //
        // copy fixed portion.
        //
        memcpy((PBYTE)pCEIW, (PBYTE)pCEIA, sizeof(INTERNET_CACHE_ENTRY_INFOW) );
        pCEIW->dwStructSize = sizeof(INTERNET_CACHE_ENTRY_INFOW);
        cc = (*pcbCEIW - sizeof(INTERNET_CACHE_ENTRY_INFOW))/sizeof(WCHAR);
    }

    // Destination for strings
    PWSTR pBuffer = (pCEIW ? (PWSTR)(pCEIW + 1) : NULL);
    // Convert strings
    for (int i=0; i < NUMBER_MEMBERS; i++)
    {
        PSTR *pBufferA = (PSTR*)((PBYTE)pCEIA + bOffsetTable[i]);

        if (*pBufferA)
        {
            DWORD dwTmp = MultiByteToWideChar(CP_ACP, 0,  *pBufferA, -1, NULL, 0);
            if ((dwTmp<=cc) && pCEIW)
            {
                INET_ASSERT(pBuffer);

                PWSTR *pBufferW = (PWSTR*)((PBYTE)pCEIW + bOffsetTable[i]);
                *pBufferW = pBuffer;
                MultiByteToWideChar(CP_ACP, 0,  *pBufferA, -1, *pBufferW, dwTmp);
                pBuffer += dwTmp;
                cc -= dwTmp;
            }
            cbSize += dwTmp*sizeof(WCHAR);
        }
    }

    DWORD dwErr = (*pcbCEIW>=cbSize) ? ERROR_SUCCESS : ERROR_INSUFFICIENT_BUFFER;
    *pcbCEIW = cbSize; // Tell how much space used/needed.
    return dwErr;
}


URLCACHEAPI
BOOL
WINAPI
CreateUrlCacheEntryW(
    IN LPCWSTR lpszUrlName,
    IN DWORD dwExpectedFileSize,
    IN LPCWSTR lpszFileExtension,
    OUT LPWSTR lpszFileName,
    IN DWORD dwReserved
    )
{
    ENTER_CACHE_API ((DBG_API, Bool, "CreateUrlCacheEntryW", "%wq, %wq, %d, %wq, %#x",
        lpszUrlName, lpszFileExtension, dwExpectedFileSize, lpszFileName, dwReserved));

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE, fStrNotSafe = FALSE;
    MEMORYPACKET mpUrlName, mpFileExtension, mpFileName;

    if (lpszUrlName)
    {
        ALLOC_MB(lpszUrlName,0,mpUrlName);
        if (!mpUrlName.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI_CHECKED(lpszUrlName,mpUrlName, &fStrNotSafe);
        if (fStrNotSafe)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
    }
    if (lpszFileExtension)
    {
        ALLOC_MB(lpszFileExtension,0,mpFileExtension);
        if (!mpFileExtension.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI_CHECKED(lpszFileExtension,mpFileExtension, &fStrNotSafe);
        if (fStrNotSafe)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
    }
    ALLOC_MB(NULL, MAX_PATH, mpFileName);
    if (!mpFileName.psStr)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    fResult = CreateUrlCacheEntryA(
        mpUrlName.psStr,
        dwExpectedFileSize,
        mpFileExtension.psStr,
        mpFileName.psStr,
        dwReserved);
    if (fResult)
    {
        MultiByteToWideChar(CP_ACP, 0, mpFileName.psStr, -1, lpszFileName, MAX_PATH);
    }

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}

URLCACHEAPI
BOOL
WINAPI
CommitUrlCacheEntryW(
    IN LPCWSTR lpszUrlName,
    IN LPCWSTR lpszLocalFileName,
    IN FILETIME ExpireTime,
    IN FILETIME LastModifiedTime,
    IN DWORD CacheEntryType,
    IN LPWSTR lpszHeaderInfo,
    IN DWORD dwHeaders,
    IN LPCWSTR lpszFileExtension,
    IN LPCWSTR lpszOriginalUrl
    )
{
    ENTER_CACHE_API ((DBG_API, Bool, "CommitUrlCacheEntryW",
        "%wq, %wq, <expires>, <last-mod>, %d, %wq, %d, %wq, %wq",
        lpszUrlName,
        lpszLocalFileName,
        CacheEntryType,
        lpszHeaderInfo,
        dwHeaders,
        lpszFileExtension,
        lpszOriginalUrl
    ));

    BOOL fResult = FALSE;
    BOOL fStrNotSafe = FALSE;
    DWORD dwErr = ERROR_SUCCESS;
    MEMORYPACKET mpUrlName, mpLocalFileName, mpFileExtension, mpHeaders, mpOriginalUrl;

    if( IsBadUrlW( lpszUrlName ) ||
        ( lpszLocalFileName ? IsBadStringPtrW( lpszLocalFileName, MAX_PATH ) : FALSE ) ) 
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    ALLOC_MB(lpszUrlName,0,mpUrlName);
    if (!mpUrlName.psStr)
    {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
    }
    UNICODE_TO_ANSI_CHECKED(lpszUrlName,mpUrlName, &fStrNotSafe);
    if (fStrNotSafe)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if (lpszLocalFileName)
    {
        ALLOC_MB(lpszLocalFileName,0,mpLocalFileName);
        if (!mpLocalFileName.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI_CHECKED(lpszLocalFileName,mpLocalFileName, &fStrNotSafe);
        if (fStrNotSafe)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
    }
    
    if (lpszFileExtension)
    {
        ALLOC_MB(lpszFileExtension,0,mpFileExtension);
        if (!mpFileExtension.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI_CHECKED(lpszFileExtension,mpFileExtension, &fStrNotSafe);
        if (fStrNotSafe)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
    }
    if (lpszHeaderInfo)
    {
        ALLOC_MB(lpszHeaderInfo,0,mpHeaders);
        if (!mpHeaders.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI_CHECKED(lpszHeaderInfo,mpHeaders, &fStrNotSafe);
        if (fStrNotSafe)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }

    }
    if (lpszOriginalUrl)
    {
        ALLOC_MB(lpszOriginalUrl,0,mpOriginalUrl);
        if (!mpOriginalUrl.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI_CHECKED(lpszOriginalUrl,mpOriginalUrl, &fStrNotSafe);
        if (fStrNotSafe)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
    }

    fResult = CommitUrlCacheEntryA(
            mpUrlName.psStr,
            mpLocalFileName.psStr,
            ExpireTime,
            LastModifiedTime,
            CacheEntryType,
            (LPBYTE)mpHeaders.psStr,
            mpHeaders.dwSize,
            mpFileExtension.psStr,
            mpOriginalUrl.psStr);

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}


BOOL
RetrieveUrlCacheEntryWCore(
    IN LPCWSTR  lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
    IN OUT LPDWORD lpcbCacheEntryInfo,
    IN DWORD dwReserved,
    IN DWORD dwLookupFlags,
    IN DWORD dwRetrievalFlags)
{
    DWORD dwErr = ERROR_SUCCESS;
    BOOL fStrNotSafe = FALSE;
    MEMORYPACKET mpUrlName;
    LPINTERNET_CACHE_ENTRY_INFOA pCEIA = NULL;
    DWORD dwCEI = 0;
    
    if (!InitGlobals())
    {
        dwErr = ERROR_INTERNET_INTERNAL_ERROR;
        goto cleanup;
    }
    if (!(lpszUrlName && lpcbCacheEntryInfo))
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    ALLOC_MB(lpszUrlName, 0, mpUrlName);
    if (!mpUrlName.psStr)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    UNICODE_TO_ANSI_CHECKED(lpszUrlName, mpUrlName, &fStrNotSafe);
    if (fStrNotSafe)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    
    dwErr = GlobalUrlContainers->RetrieveUrl(
                        mpUrlName.psStr,
                        &pCEIA,
                        &dwCEI,
                        dwLookupFlags,
                        dwRetrievalFlags | RETRIEVE_WITH_ALLOCATION);

    if (dwErr==ERROR_SUCCESS)
    {
        dwErr = TransformA2W(
            pCEIA,
            dwCEI,
            lpCacheEntryInfo,
            lpcbCacheEntryInfo);

        if (dwErr!=ERROR_SUCCESS)
        {
            UnlockUrlCacheEntryFileW(lpszUrlName, 0);
        }
    }

cleanup:
    if (pCEIA)
    {
        FREE_MEMORY(pCEIA);
    }
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    return (dwErr == ERROR_SUCCESS);
}


URLCACHEAPI
BOOL
WINAPI
RetrieveUrlCacheEntryFileW(
    IN LPCWSTR  lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
    IN OUT LPDWORD lpcbCacheEntryInfo,
    IN DWORD dwReserved
    )
{
    ENTER_CACHE_API ((DBG_API, Bool, "RetrieveUrlCacheEntryFileW","%wq, %#x, %#x, %#x",
        lpszUrlName, lpCacheEntryInfo, lpcbCacheEntryInfo, dwReserved));

    BOOL fResult = RetrieveUrlCacheEntryWCore(
                        lpszUrlName,
                        lpCacheEntryInfo,
                        lpcbCacheEntryInfo,
                        dwReserved,
                        LOOKUP_URL_CREATE,
                        RETRIEVE_WITH_CHECKS);

    DEBUG_LEAVE_API(fResult);
    return fResult;
}

URLCACHEAPI
HANDLE
WINAPI
RetrieveUrlCacheEntryStreamW(
    IN LPCWSTR  lpszUrlName,
    OUT LPCACHE_ENTRY_INFOW lpCacheEntryInfo,
    IN OUT LPDWORD lpcbCacheEntryInfo,
    IN BOOL fRandomRead,
    IN DWORD dwReserved
    )
{
    ENTER_CACHE_API ((DBG_API, Handle, "RetrieveUrlCacheEntryStreamW",
        "%wq, %#x, %#x, %d, %#x",
        lpszUrlName,
        lpCacheEntryInfo,
        lpcbCacheEntryInfo,
        fRandomRead,
        dwReserved
    ));

    BOOL fLocked = FALSE;
    HANDLE hInternet = NULL;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dwErr = ERROR_SUCCESS, dwFileSize;
    CACHE_STREAM_CONTEXT_HANDLE* pStream;

    if (!RetrieveUrlCacheEntryWCore(
                        lpszUrlName,
                        lpCacheEntryInfo,
                        lpcbCacheEntryInfo,
                        dwReserved,
                        LOOKUP_URL_NOCREATE,
                        RETRIEVE_WITHOUT_CHECKS))
    {
        goto cleanup;
    }

    fLocked = TRUE;        

    // Allocate a stream handle.
    LOCK_CACHE();
    hInternet = HandleMgr.Alloc (sizeof(CACHE_STREAM_CONTEXT_HANDLE));
    if (hInternet)
    {        
        pStream = (CACHE_STREAM_CONTEXT_HANDLE*) HandleMgr.Map (hInternet);
        INET_ASSERT (pStream);
    }
    UNLOCK_CACHE();
    if (!hInternet)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    // Open the file.
    // Does CreateFileW exist on Win9x?

    hFile = CreateFileW
    (
        lpCacheEntryInfo->lpszLocalFileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL |
          (fRandomRead ? FILE_FLAG_RANDOM_ACCESS : FILE_FLAG_SEQUENTIAL_SCAN),
                // improves file read (cache) performance?
        NULL
    );
    if( hFile == INVALID_HANDLE_VALUE )
    {
        dwErr = GetLastError();
        goto cleanup;
    }

    dwFileSize = GetFileSize(hFile, NULL);
    if (dwFileSize != lpCacheEntryInfo->dwSizeLow) 
    {
        dwErr = (dwFileSize==0xFFFFFFFF) ? GetLastError() : ERROR_INVALID_DATA;
        goto cleanup;
    }

    pStream->FileHandle = hFile;

    // Copy URL name storage.
    {
        MEMORYPACKET mpUrl;
        ALLOC_MB(lpszUrlName,0,mpUrl);
        if (!mpUrl.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(lpszUrlName,mpUrl);
        
        pStream->SourceUrlName = NewString(mpUrl.psStr);
        if( !pStream->SourceUrlName)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }

cleanup:
    if (dwErr!=ERROR_SUCCESS)
    {
        if (hInternet)
        {
            HandleMgr.Free(hInternet);
            hInternet = NULL;
        }
        if (hFile)
            CloseHandle (hFile);

        if (fLocked)
        {
            UnlockUrlCacheEntryFileW(lpszUrlName, 0);
        }
        SetLastError (dwErr);
        DEBUG_ERROR(API, dwErr);
    }

    DEBUG_LEAVE_API(hInternet);
    return hInternet;
}


BOOL
GetUrlCacheEntryWCore(
        IN LPCWSTR lpszUrl,
        OUT LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
        IN OUT LPDWORD lpcbCacheEntryInfo,
        DWORD dwFlags,
        DWORD dwLookupFlags,
        BOOL fConvertHeaders)
{
    BOOL fResult = FALSE;
    DWORD dwErr = ERROR_SUCCESS;
    BOOL fStrNotSafe = FALSE;
    MEMORYPACKET mpUrlName;
    LPINTERNET_CACHE_ENTRY_INFOA pCEIA = NULL;
    DWORD cbCEIA;

    if (!InitGlobals())
    {
        dwErr = ERROR_INTERNET_INTERNAL_ERROR;
        goto cleanup;
    }

    if (IsBadUrlW(lpszUrl))
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    ALLOC_MB(lpszUrl,0,mpUrlName);
    if (!mpUrlName.psStr)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    UNICODE_TO_ANSI_CHECKED(lpszUrl,mpUrlName, &fStrNotSafe);
    if (fStrNotSafe)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if (lpcbCacheEntryInfo)
    {
        dwErr = GlobalUrlContainers->GetUrlInfo(
            mpUrlName.psStr,
            &pCEIA,
            &cbCEIA,
            dwFlags,
            dwLookupFlags,
            RETRIEVE_WITH_ALLOCATION);
            
        // convert from ansi to unicode. 
        if (dwErr==ERROR_SUCCESS)
        {
            dwErr = TransformA2W(pCEIA, cbCEIA, lpCacheEntryInfo, lpcbCacheEntryInfo);
            if (dwErr==ERROR_SUCCESS)
            {
                fResult = TRUE;
            }
        }
    }
    else
    {
        fResult = GetUrlCacheEntryInfoExA(
                mpUrlName.psStr, 
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                dwFlags);
    }

cleanup: 
    if (pCEIA)
    {
        FREE_MEMORY(pCEIA);
    }
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    return fResult;
}


URLCACHEAPI
BOOL
WINAPI
GetUrlCacheEntryInfoW(
    IN LPCWSTR lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
    IN OUT LPDWORD lpcbCacheEntryInfo
    )
{
    ENTER_CACHE_API ((DBG_API, Bool, "GetUrlCacheEntryInfoW", "%wq, %#x, %#x",
        lpszUrlName, lpCacheEntryInfo, lpcbCacheEntryInfo));

    BOOL fResult = GetUrlCacheEntryWCore(
                            lpszUrlName,
                            lpCacheEntryInfo,
                            lpcbCacheEntryInfo,
                            0,
                            LOOKUP_URL_NOCREATE,
                            TRUE);

    DEBUG_LEAVE_API(fResult);
    return fResult;
}

BOOLAPI
GetUrlCacheEntryInfoExW(
        IN LPCWSTR lpszUrl,
        OUT LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
        IN OUT LPDWORD lpcbCacheEntryInfo,
        OUT LPWSTR lpszRedirectUrl,
        IN OUT LPDWORD lpcbRedirectUrl,
        LPVOID lpReserved,
        DWORD dwFlags
    )
{
    ENTER_CACHE_API ((DBG_API, Bool, "GetUrlCacheEntryInfoExW",
        "%wq, %#x, %#x, %wq, %#x, %#x, %#x", 
        lpszUrl, lpCacheEntryInfo, lpcbCacheEntryInfo, lpszRedirectUrl, lpcbRedirectUrl, lpReserved, dwFlags));

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    
    if (lpszRedirectUrl
        || lpcbRedirectUrl 
        || lpReserved
       )
    {
        INET_ASSERT (FALSE);
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    
    fResult = GetUrlCacheEntryWCore(
                            lpszUrl,
                            lpCacheEntryInfo,
                            lpcbCacheEntryInfo,
                            dwFlags,
                            LOOKUP_URL_TRANSLATE | (dwFlags & INTERNET_CACHE_FLAG_ALLOW_COLLISIONS),
                            TRUE);

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}


URLCACHEAPI
BOOL
WINAPI
SetUrlCacheEntryInfoW(
    IN LPCWSTR lpszUrlName,
    IN LPCACHE_ENTRY_INFOW lpCacheEntryInfo,
    IN DWORD dwFieldControl
    )
{
    ENTER_CACHE_API ((DBG_API, Bool, "SetUrlCacheEntryInfoW", "%wq, %#x, %d",
        lpszUrlName, lpCacheEntryInfo, dwFieldControl));

    BOOL fResult = FALSE;
    BOOL fStrNotSafe = FALSE;
    DWORD dwErr = ERROR_SUCCESS;
    MEMORYPACKET mpUrlName;
    INTERNET_CACHE_ENTRY_INFOA CacheEntryInfoA;

    if (!lpszUrlName)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    memcpy( &CacheEntryInfoA, lpCacheEntryInfo, sizeof(CacheEntryInfoA) );

    ALLOC_MB(lpszUrlName,0,mpUrlName);
    if (!mpUrlName.psStr)
    {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
    }
    UNICODE_TO_ANSI_CHECKED(lpszUrlName,mpUrlName, &fStrNotSafe);
    if (fStrNotSafe)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    fResult = SetUrlCacheEntryInfoA(
            mpUrlName.psStr,
            &CacheEntryInfoA,
            dwFieldControl );

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}


BOOL FindUrlCacheEntryWCore(
    IN OUT HANDLE     *phFind,
    IN     LPCWSTR    lpszUrlSearchPattern,
    IN     DWORD     dwFlags,
    IN     DWORD     dwFilter,
    IN     GROUPID   GroupId,
    OUT    LPINTERNET_CACHE_ENTRY_INFOW pEntryInfo,
    IN OUT LPDWORD   pcbEntryInfo,
    IN     BOOL      fConvertHeaders
    )
{
    DWORD dwErr = ERROR_SUCCESS;
    BOOL fStrNotSafe = FALSE;
    MEMORYPACKET mpSearchPattern;
    LPINTERNET_CACHE_ENTRY_INFOA pCEIA = NULL;
    DWORD cbCEIA;
    BOOL fFindFirst = *phFind==NULL;
    
    // DebugBreak();
    
    if (!InitGlobals())
    {
        dwErr = ERROR_INTERNET_INTERNAL_ERROR;
        goto cleanup;
    }
    if (!pcbEntryInfo)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    if (lpszUrlSearchPattern)
    {
        ALLOC_MB(lpszUrlSearchPattern, 0, mpSearchPattern);
        if (!mpSearchPattern.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI_CHECKED(lpszUrlSearchPattern, mpSearchPattern, &fStrNotSafe);
        if (fStrNotSafe)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
    }

    dwErr = GlobalUrlContainers->FindNextEntry(phFind, 
                                               mpSearchPattern.psStr, 
                                               &pCEIA, 
                                               &cbCEIA,
                                               dwFilter,
                                               GroupId,
                                               dwFlags,
                                               RETRIEVE_WITH_ALLOCATION);

    // TransformA2W will convert from ansi to unicode. ERROR_SUCCESS always means that
    // the cache entry has been returned.
    if (dwErr==ERROR_SUCCESS)
    {
        dwErr = TransformA2W(pCEIA,
            cbCEIA,
            pEntryInfo,
            pcbEntryInfo);
    }

cleanup: 
    if (pCEIA)
    {
        FREE_MEMORY(pCEIA);
    }
    if (dwErr!=ERROR_SUCCESS) 
    { 
        if (fFindFirst && *phFind)
        {
            GlobalUrlContainers->FreeFindHandle(*phFind);
            *phFind = NULL;
        }
        
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    return (dwErr==ERROR_SUCCESS) ;
}


URLCACHEAPI
HANDLE
WINAPI
FindFirstUrlCacheEntryW(
    IN LPCWSTR lpszUrlSearchPattern,
    OUT LPCACHE_ENTRY_INFOW lpFirstCacheEntryInfo,
    IN OUT LPDWORD lpcbCacheEntryInfo
    )
{
    ENTER_CACHE_API ((DBG_API, Bool, "FindFirstUrlCacheEntryW",
        "%wq, %#x, %#x",
        lpszUrlSearchPattern,
        lpFirstCacheEntryInfo,
        lpcbCacheEntryInfo
    ));

    HANDLE hInternet = FindFirstUrlCacheEntryExW(
                        lpszUrlSearchPattern,
                        FIND_FLAGS_OLD_SEMANTICS,
                        URLCACHE_FIND_DEFAULT_FILTER,
                        NULL,
                        lpFirstCacheEntryInfo,
                        lpcbCacheEntryInfo,
                        NULL,
                        NULL,
                        NULL);

    DEBUG_LEAVE_API(hInternet);
    return hInternet;
}

URLCACHEAPI
BOOL
WINAPI
FindNextUrlCacheEntryW(
    IN HANDLE hEnumHandle,
    OUT LPCACHE_ENTRY_INFOW pEntryInfo,
    IN OUT LPDWORD pcbEntryInfo
    )
{
    ENTER_CACHE_API ((DBG_API, Bool, "FindNextUrlCacheEntryW",
       "%#x, %#x, %#x",
        hEnumHandle, 
        pEntryInfo,
        pcbEntryInfo
    ));

    BOOL fResult = FindNextUrlCacheEntryExW(
                    hEnumHandle,
                    pEntryInfo,
                    pcbEntryInfo,
                    NULL, 
                    NULL,
                    NULL);

    DEBUG_LEAVE_API(fResult);
    return fResult;
}


INTERNETAPI
HANDLE
WINAPI
FindFirstUrlCacheEntryExW(
    IN     LPCWSTR    lpszUrlSearchPattern,
    IN     DWORD     dwFlags,
    IN     DWORD     dwFilter,
    IN     GROUPID   GroupId,
    OUT    LPINTERNET_CACHE_ENTRY_INFOW pEntryInfo,
    IN OUT LPDWORD   pcbEntryInfo,
    OUT    LPVOID    lpGroupAttributes,     // must pass NULL
    IN OUT LPDWORD   pcbGroupAttributes,    // must pass NULL
    IN     LPVOID    lpReserved             // must pass NULL
    )
{
    ENTER_CACHE_API ((DBG_API, Bool, "FindFirstUrlCacheEntryExW",
        "%wq, %#x, %#x, %#x, %#x, %#x, %#x, %#x, %#x",
        lpszUrlSearchPattern,
        dwFlags,
        dwFilter,
        GroupId,
        pEntryInfo,
        pcbEntryInfo,
        lpGroupAttributes,
        pcbGroupAttributes,
        lpReserved
    ));

    HANDLE hInternet = NULL;

    FindUrlCacheEntryWCore(
            &hInternet,
            lpszUrlSearchPattern,
            dwFlags,
            dwFilter,
            GroupId,
            pEntryInfo,
            pcbEntryInfo,
            TRUE);

    DEBUG_LEAVE_API(hInternet);
    return hInternet;
}


BOOLAPI
FindNextUrlCacheEntryExW(
    IN     HANDLE    hEnumHandle,
    OUT    LPINTERNET_CACHE_ENTRY_INFOW pEntryInfo,
    IN OUT LPDWORD   pcbEntryInfo,
    OUT    LPVOID    lpGroupAttributes,     // must pass NULL
    IN OUT LPDWORD   pcbGroupAttributes,    // must pass NULL
    IN     LPVOID    lpReserved             // must pass NULL
    )
{
    ENTER_CACHE_API ((DBG_API, Bool, "FindNextUrlCacheEntryExW",
        "%#x, %#x, %#x, %#x, %#x, %#x",
        hEnumHandle,
        pEntryInfo,
        pcbEntryInfo,
        lpGroupAttributes,
        pcbGroupAttributes,
        lpReserved
    ));

    BOOL fResult = FALSE;
    DWORD dwErr = ERROR_SUCCESS;
    
    if (!hEnumHandle)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    fResult = FindUrlCacheEntryWCore(
                        &hEnumHandle,
                        NULL,
                        0,
                        0,
                        0,
                        pEntryInfo,
                        pcbEntryInfo,
                        TRUE);

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}

URLCACHEAPI
BOOL
WINAPI
FreeUrlCacheSpaceW(
    IN LPCWSTR lpszCachePath,
    IN DWORD dwSize,
    IN DWORD dwReserved
    )
{
    ENTER_CACHE_API ((DBG_API, Bool, "FreeUrlCacheSpaceW", 
        "<path>,%d, %#x", dwSize, dwReserved));

    BOOL fResult = FALSE;
    DWORD dwErr = ERROR_SUCCESS;
    BOOL fStrNotSafe = FALSE;

    MEMORYPACKET mpCachePath;
    if (lpszCachePath)
    {
        ALLOC_MB(lpszCachePath,0,mpCachePath);
        if (!mpCachePath.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI_CHECKED(lpszCachePath,mpCachePath, &fStrNotSafe);
        if (fStrNotSafe)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
    }
    fResult = FreeUrlCacheSpaceA(
            mpCachePath.psStr,
            dwSize,
            dwReserved );

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}


URLCACHEAPI
BOOL
WINAPI
UnlockUrlCacheEntryFileW(
    LPCWSTR lpszUrlName,
    IN DWORD dwReserved
    )
/*++

Routine Description:

    This API checks in the file that was check out as part of
    RetrieveUrlFile API.

Arguments:

    lpszUrlName : name of the URL that is being retrieved.

    dwReserved : reserved for future use.

Return Value:

    Windows Error code.

--*/
{
    ENTER_CACHE_API ((DBG_API, Bool, "UnlockUrlCacheEntryFileW",
        "%wq, %#x", lpszUrlName, dwReserved));

    BOOL fResult = FALSE;
    BOOL fStrNotSafe = FALSE;
    DWORD dwErr = ERROR_SUCCESS;
    MEMORYPACKET mpUrl;

    if (!lpszUrlName)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    ALLOC_MB(lpszUrlName,0,mpUrl);
    if (!mpUrl.psStr)
    {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
    }
    UNICODE_TO_ANSI_CHECKED(lpszUrlName,mpUrl, &fStrNotSafe);
    if (fStrNotSafe)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    fResult = UnlockUrlCacheEntryFileA(mpUrl.psStr, dwReserved);

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}

URLCACHEAPI
BOOL
WINAPI
DeleteUrlCacheEntryW(
    IN LPCWSTR lpszUrlName
    )
{
    ENTER_CACHE_API ((DBG_API, Bool, "DeleteUrlCacheEntryW",
        "%wq", lpszUrlName));

    BOOL fResult = FALSE;
    BOOL fStrNotSafe = FALSE;
    DWORD dwErr = ERROR_SUCCESS;
    MEMORYPACKET mpUrl;

    if (!lpszUrlName)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    ALLOC_MB(lpszUrlName,0,mpUrl);
    if (!mpUrl.psStr)
    {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
    }
    UNICODE_TO_ANSI_CHECKED(lpszUrlName,mpUrl, &fStrNotSafe);
    if (fStrNotSafe)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    fResult = DeleteUrlCacheEntryA(mpUrl.psStr);

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}

BOOLAPI
IsUrlCacheEntryExpiredW(
    IN      LPCWSTR      lpszUrlName,
    IN      DWORD        dwFlags,
    IN OUT  FILETIME*    pftLastModifiedTime
)
{
    ENTER_CACHE_API ((DBG_API, Bool, "UrlCacheEntryExpiredW", 
        "%wq, %#x", lpszUrlName, dwFlags));

    BOOL fResult = FALSE;
    BOOL fStrNotSafe = FALSE;
    DWORD dwErr = ERROR_SUCCESS;
    MEMORYPACKET mpUrl;

    if (!lpszUrlName)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    ALLOC_MB(lpszUrlName,0,mpUrl);
    if (!mpUrl.psStr)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    UNICODE_TO_ANSI_CHECKED(lpszUrlName,mpUrl, &fStrNotSafe);
    if (fStrNotSafe)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    fResult = IsUrlCacheEntryExpiredA(
                    mpUrl.psStr,
                    dwFlags,
                    pftLastModifiedTime);

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}

BOOL CacheGroupInfoA2W(
    IN          LPINTERNET_CACHE_GROUP_INFOA    lpAnsiGroupInfo,
    IN          DWORD                           dwAnsiGroupInfoSize,
    OUT         LPINTERNET_CACHE_GROUP_INFOW    lpUnicodeGroupInfo,
    IN OUT      LPDWORD                         lpdwUnicodeGroupInfoSize
)
{
    INET_ASSERT( lpUnicodeGroupInfo && lpAnsiGroupInfo);

    lpUnicodeGroupInfo->dwGroupSize = sizeof(INTERNET_CACHE_GROUP_INFOW);
    lpUnicodeGroupInfo->dwGroupFlags = lpAnsiGroupInfo->dwGroupFlags;
    lpUnicodeGroupInfo->dwGroupType = lpAnsiGroupInfo->dwGroupType;
    lpUnicodeGroupInfo->dwDiskUsage = lpAnsiGroupInfo->dwDiskUsage;
    lpUnicodeGroupInfo->dwDiskQuota = lpAnsiGroupInfo->dwDiskQuota;

    memcpy(lpUnicodeGroupInfo->dwOwnerStorage,
           lpAnsiGroupInfo->dwOwnerStorage,
           GROUP_OWNER_STORAGE_SIZE * sizeof(DWORD) );


    BOOL fRet = MultiByteToWideChar(
               CP_ACP,
               MB_PRECOMPOSED,
               lpAnsiGroupInfo->szGroupName,
               -1,         // null terminated ansi string.
               lpUnicodeGroupInfo->szGroupName,
               GROUPNAME_MAX_LENGTH
    );

    if( fRet )
    {
        *lpdwUnicodeGroupInfoSize = lpUnicodeGroupInfo->dwGroupSize;
    }
    else
    {
        *lpdwUnicodeGroupInfoSize = 0;
        SetLastError(ERROR_INVALID_PARAMETER);
    }
    return fRet;
}

BOOL CacheGroupInfoW2A(
    IN          LPINTERNET_CACHE_GROUP_INFOW    lpUnicodeGroupInfo,
    IN          DWORD                           dwUnicodeGroupInfoSize,
    OUT         LPINTERNET_CACHE_GROUP_INFOA    lpAnsiGroupInfo,
    IN OUT      LPDWORD                         lpdwAnsiGroupInfoSize
)
{
    INET_ASSERT( lpUnicodeGroupInfo && lpAnsiGroupInfo);
    BOOL fStrNotSafe = FALSE;

    lpAnsiGroupInfo->dwGroupSize = sizeof(INTERNET_CACHE_GROUP_INFOA);
    lpAnsiGroupInfo->dwGroupFlags = lpUnicodeGroupInfo->dwGroupFlags;
    lpAnsiGroupInfo->dwGroupType = lpUnicodeGroupInfo->dwGroupType;
    lpAnsiGroupInfo->dwDiskUsage = lpUnicodeGroupInfo->dwDiskUsage;
    lpAnsiGroupInfo->dwDiskQuota = lpUnicodeGroupInfo->dwDiskQuota;

    memcpy( lpAnsiGroupInfo->dwOwnerStorage,
            lpUnicodeGroupInfo->dwOwnerStorage,
            GROUP_OWNER_STORAGE_SIZE * sizeof(DWORD) );

    BOOL fRet = WideCharToMultiByte(
                CP_ACP,
                0,              // no flags.
                lpUnicodeGroupInfo->szGroupName,
                -1,             // null terminated unicode string.
                lpAnsiGroupInfo->szGroupName,
                GROUPNAME_MAX_LENGTH,
                NULL,           // lpDefaultChar
                &fStrNotSafe    // lpUseDefaultChar
    );
    if (fStrNotSafe)
    {
        fRet = FALSE;
    }

    if( fRet )
    {
        *lpdwAnsiGroupInfoSize = lpAnsiGroupInfo->dwGroupSize;
    }
    else
    {
        *lpdwAnsiGroupInfoSize = 0;
    }
    return fRet;
}


URLCACHEAPI
BOOLAPI
SetUrlCacheEntryGroupW(
    IN LPCWSTR   lpszUrlName,
    IN DWORD    dwFlags,
    IN GROUPID  GroupId,
    IN LPBYTE   pbGroupAttributes, // must pass NULL
    IN DWORD    cbGroupAttributes, // must pass 0
    IN LPVOID   lpReserved         // must pass NULL
    )
{
    ENTER_CACHE_API ((DBG_API, Bool, "SetUrlCacheEntryGroupW", 
        "%wq, %#x, %#x, %#x, %#x, %#x", lpszUrlName, dwFlags, GroupId, pbGroupAttributes, cbGroupAttributes, lpReserved));

    BOOL fResult = FALSE;
    BOOL fStrNotSafe = FALSE;
    DWORD dwErr = ERROR_SUCCESS;
    MEMORYPACKET mpUrl;

    if (!lpszUrlName)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    ALLOC_MB(lpszUrlName,0,mpUrl);
    if (!mpUrl.psStr)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    UNICODE_TO_ANSI_CHECKED(lpszUrlName,mpUrl, &fStrNotSafe);
    if (fStrNotSafe)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    fResult = SetUrlCacheEntryGroupA(
                    mpUrl.psStr,
                    dwFlags,
                    GroupId,
                    pbGroupAttributes,
                    cbGroupAttributes,
                    lpReserved);

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}

URLCACHEAPI
BOOL
WINAPI
GetUrlCacheGroupAttributeW(
    IN      GROUPID                         gid,
    IN      DWORD                           dwFlags,
    IN      DWORD                           dwAttributes,
    OUT     LPINTERNET_CACHE_GROUP_INFOW    lpGroupInfo,
    IN OUT  LPDWORD                         lpdwGroupInfo,
    IN OUT  LPVOID                          lpReserved
)
{
    ENTER_CACHE_API ((DBG_API, Bool, "GetUrlCacheGroupAttributeW",
        "%d, %d, %d, %#x, %#x, %#x", 
        gid, dwFlags, dwAttributes, lpGroupInfo, lpdwGroupInfo, lpReserved ));

    BOOL fResult = FALSE;
    DWORD Error = ERROR_SUCCESS;
    INTERNET_CACHE_GROUP_INFOA AnsiGroupInfo;
    DWORD  dwAnsiGroupInfoSize = sizeof(INTERNET_CACHE_GROUP_INFOA);

    if( !lpGroupInfo || !lpdwGroupInfo )
    {
        Error = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if( *lpdwGroupInfo < sizeof(INTERNET_CACHE_GROUP_INFOW) )
    {
        Error = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }

    if( IsBadWriteUrlInfo(lpGroupInfo, *lpdwGroupInfo) )
    {
        Error = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if( GetUrlCacheGroupAttributeA(
            gid, dwFlags, dwAttributes,
            &AnsiGroupInfo, &dwAnsiGroupInfoSize, lpReserved ) )
    {
        fResult = CacheGroupInfoA2W( &AnsiGroupInfo, 
                                    dwAnsiGroupInfoSize,
                                    lpGroupInfo, 
                                    lpdwGroupInfo );
    }


Cleanup:
    if (Error!=ERROR_SUCCESS)
    {
        SetLastError(Error);
        DEBUG_ERROR(API, Error);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}

URLCACHEAPI
BOOL
WINAPI
SetUrlCacheGroupAttributeW(
    IN      GROUPID                         gid,
    IN      DWORD                           dwFlags,
    IN      DWORD                           dwAttributes,
    IN      LPINTERNET_CACHE_GROUP_INFOW    lpGroupInfo,
    IN OUT  LPVOID                          lpReserved
)
{
    ENTER_CACHE_API ((DBG_API, Bool, "SetUrlCacheGroupAttributeA",
        "%#x, %d, %d, %#x, %#x", 
        gid, dwFlags, dwAttributes, lpGroupInfo, lpReserved));

    BOOL fResult = FALSE;
    DWORD Error = ERROR_SUCCESS;
    INTERNET_CACHE_GROUP_INFOA AnsiGroupInfo;
    DWORD  dwAnsiGroupInfoSize = sizeof(INTERNET_CACHE_GROUP_INFOA);

    if( IsBadReadPtr(lpGroupInfo, sizeof(INTERNET_CACHE_GROUP_INFOW) ) )
    {
        Error = ERROR_INVALID_PARAMETER;
    }
    else if( CacheGroupInfoW2A(
            lpGroupInfo, sizeof(INTERNET_CACHE_GROUP_INFOW),
            &AnsiGroupInfo, &dwAnsiGroupInfoSize ) )
    {
        fResult = SetUrlCacheGroupAttributeA(
            gid, dwFlags, dwAttributes, &AnsiGroupInfo, lpReserved );
    }

    if (Error!=ERROR_SUCCESS)
    {
        SetLastError(Error);
        DEBUG_ERROR(API, Error);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}


// Convert all the ansi strings in a structure to unicode

/* How does this work?
    Take this structure for example:

    struct foo 
    {
        DWORD dwA;
        LPTSTR pszB;
        DWORD dwC;
        LPTSTR pszD;
    };
    where LPTSTR are embedded pointers
    
    The memory layout is thus:

    [DWORD][LPTSTR][DWORD][LPTSTR][embedded string pszB][embedded string pszD]
    ^                             ^
    |                             |
    |-struct beginning            |-beginning of embedded strings

    Assuming a 32-bit platform, we can construct pointers (relative to the struct beginning) to each element
    in the structure. In this case,

    { 0, sizeof(DWORD), sizeof(DWORD)+sizeof(LPTSTR), sizeof(DWORD)+sizeof(LPTSTR)+sizeof(DWORD) }

    Let's say we're interested in strings only, and we know that these strings are embedded. We can create a byte table thus:
    BYTE bFoo[] = { sizeof(DWORD), sizeof(DWORD)+sizeof(LPTSTR)+sizeof(DWORD) }

    Alternatively:
    BYTE bFoo[] = 
    {
        (BYTE)&(((foo*)NULL)->pszB),
        (BYTE)&(((foo*)NULL)->pszD)
    };

    This layout is the same for both Ansi and Unicode versions of a struct, UNLESS the struct contains for example 
    a TCHAR szWhat[256] (in which case, we can't use the bulk converter).

    Pass BulkConverter the following parameters, to convert strings in one swoop.

    pbSrc       = casted pointer to the beginning of the ansi structure
    pbDest      = casted pointer to the beginning of the unicode structure
    cbAvail     = number of bytes available for embedded strings
    wSkip       = offset from the beginning of the structure, at which point embedded strings may be written
    cElements   = number of elements to convert from ansi to unicode

    If BulkConverter succeeds, it'll return the number of bytes used.
    If it fails, it will return the number of bytes needed to store all the unicode strings.

BUT HOW DOES THE DAMN THING WORK?
Oh. 

1. Using the offset table, we figure out where the pointer to the string is in both the structures.
2. Then using magic, we decided where to place the unicode string. 
3. Figure how much space we'll need to store the unicode string
4. If that much is available, convert.
5. Keep track, either way.
6. Go to 1, if we have any other strings left.
*/

LONG BulkConverter(PBYTE pbSrc, PBYTE pbDest, LONG cbAvail, WORD wSkip, CONST BYTE abTable[], WORD cElements)
{
    PWSTR pBuffer = (PWSTR)(pbDest + wSkip);
    PSTR *pBufferA;
    PWSTR *pBufferW;

    for (DWORD i=0; i < cElements; i++)
    {
        pBufferA = (PSTR*)((PBYTE)pbSrc + abTable[i]);
        pBufferW = (PWSTR*)((PBYTE)pbDest + abTable[i]);

        if (*pBufferA)
        {
            *pBufferW = pBuffer;
            LONG dwTmp = MultiByteToWideChar(CP_ACP, 0,  *pBufferA, -1,
                                             *pBufferW, 0);
            if (dwTmp<cbAvail)
            {
                MultiByteToWideChar(CP_ACP, 0,  *pBufferA, -1,
                                             *pBufferW, cbAvail);
                pBuffer += dwTmp;
            }
            cbAvail -= dwTmp;
        }
    }
    return cbAvail;
}


const BYTE bOffsetTableContainer[] = 
    {
        (BYTE)&(((LPINTERNET_CACHE_CONTAINER_INFOW)NULL)->lpszName),
        (BYTE)&(((LPINTERNET_CACHE_CONTAINER_INFOW)NULL)->lpszCachePrefix),
        (BYTE)&(((LPINTERNET_CACHE_CONTAINER_INFOW)NULL)->lpszVolumeLabel),
        (BYTE)&(((LPINTERNET_CACHE_CONTAINER_INFOW)NULL)->lpszVolumeTitle)
    };

BOOL
TransformCacheContainerInfoToW(
    IN BOOL fResult,
    IN LPINTERNET_CACHE_CONTAINER_INFOA pCCIA,
    IN DWORD cbCCIA,
    OUT LPINTERNET_CACHE_CONTAINER_INFOW pCCIW,
    OUT LPDWORD pcbCCIW
)
{
    DWORD cbSize = *pcbCCIW;

    if (fResult)
    {
        // If we have pointers, try to convert from 

        LONG cc = *pcbCCIW - sizeof(INTERNET_CACHE_CONTAINER_INFOW);
        if (*pcbCCIW > sizeof(INTERNET_CACHE_CONTAINER_INFOW))
        {
            pCCIW->dwCacheVersion = pCCIA->dwCacheVersion;
        }
        cc /= sizeof(WCHAR);
        // Convert strings
        cc = BulkConverter((PBYTE)pCCIA, 
                (PBYTE)pCCIW, 
                cc, 
                sizeof(INTERNET_CACHE_CONTAINER_INFOW), 
                bOffsetTableContainer, 
                ARRAY_ELEMENTS(bOffsetTableContainer));


       // Tell how much space was actually used.
        *pcbCCIW -= cc*sizeof(WCHAR);

        if (*pcbCCIW>cbSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            fResult = FALSE;
        }
    }
    else if (GetLastError()==ERROR_INSUFFICIENT_BUFFER)
    {
        *pcbCCIW = (cbCCIA - sizeof(INTERNET_CACHE_CONTAINER_INFOA))*sizeof(WCHAR) + sizeof(INTERNET_CACHE_CONTAINER_INFOW);
    }

    return fResult;
}

#define USE_ORIGINAL_CODE

URLCACHEAPI
BOOL
WINAPI
CreateUrlCacheContainerW(
                 IN LPCWSTR Name,
                 IN LPCWSTR CachePrefix,
                 IN LPCWSTR CachePath,
                 IN DWORD KBCacheLimit,
                 IN DWORD dwContainerType,
                     IN DWORD dwOptions,
                     IN OUT LPVOID pvBuffer,
                     IN OUT LPDWORD cbBuffer)
{
    ENTER_CACHE_API ((DBG_API, Bool, "CreateUrlCacheContainerW", "%wq, %wq, %wq, %d, %d, %d, %#x, %#x",
        Name, CachePrefix, CachePath, KBCacheLimit, dwContainerType, dwOptions, pvBuffer, cbBuffer));

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;

    MEMORYPACKET mpName, mpCachePrefix, mpCachePath;
    BOOL fStrNotSafe = FALSE;

#ifdef USE_ORIGINAL_CODE
    if (Name)
    {
        ALLOC_MB(Name, 0, mpName);
        if (!mpName.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI_CHECKED(Name, mpName, &fStrNotSafe);
        if (fStrNotSafe)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
    }
    if (CachePrefix)
    {
        ALLOC_MB(CachePrefix, 0, mpCachePrefix);
        if (!mpCachePrefix.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI_CHECKED(CachePrefix, mpCachePrefix, &fStrNotSafe);
        if (fStrNotSafe)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
    }
    if (CachePath)
    {
        ALLOC_MB(CachePath,0,mpCachePath);
        if (!mpCachePath.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI_CHECKED(CachePath,mpCachePath, &fStrNotSafe);
        if (fStrNotSafe)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
    }
#else
// Theoretically, the following fragment should be smaller than the above fragment. 
// Although the retail obj shows a function that's about 100 bytes shorter, the
// actual dll doesn't show this gain. Until I figure this out, we won't use it.

    DWORD c;
    do
    {
        MEMORYPACKET* mp;
        PCWSTR psz;

        switch (c)
        {
        case 0:
            psz = Name;
            mp = &mpName;
            break;
            
        case 1:
            psz = CachePrefix;
            mp = &mpCachePrefix;
            break;

        case 2:
            psz = CachePath;
            mp = &mpCachePath;
            break;
        }
        ALLOC_MB(psz, 0, (*mp));
        if (!mp->psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI_CHECKED(psz, (*mp), &fStrNotSafe);
        if (fStrNotSafe)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
        c++;
    }
    while (c<3);

#endif

    fResult = CreateUrlCacheContainerA(
                mpName.psStr,
                mpCachePrefix.psStr,
                mpCachePath.psStr,
                KBCacheLimit,
                dwContainerType,
                dwOptions,
                pvBuffer,
                cbBuffer);

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}

URLCACHEAPI
BOOL
WINAPI
DeleteUrlCacheContainerW(
IN LPCWSTR Name,
IN DWORD dwOptions)
{
    ENTER_CACHE_API ((DBG_API, Bool, "DeleteContainerW", "%wq, %#x", Name, dwOptions));

    DWORD dwErr = ERROR_SUCCESS;
    MEMORYPACKET mpName;
    BOOL fResult = FALSE, fStrNotSafe = FALSE;

    ALLOC_MB(Name, 0, mpName);
    if (!mpName.psStr)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    UNICODE_TO_ANSI_CHECKED(Name, mpName, &fStrNotSafe);
    if (fStrNotSafe)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    fResult = DeleteUrlCacheContainerA(mpName.psStr, dwOptions);

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}


URLCACHEAPI
HANDLE
WINAPI
FindFirstUrlCacheContainerW(
    IN OUT DWORD *pdwModified,
        OUT LPINTERNET_CACHE_CONTAINER_INFOW lpContainerInfo,
        IN OUT LPDWORD lpcbContainerInfo,
    IN DWORD dwOptions
    )
{
    ENTER_CACHE_API ((DBG_API, Bool, "FindFirstContainerW",
        "%#x, %#x, %#x, %#x",
        pdwModified,
        lpContainerInfo,
        lpcbContainerInfo,
        dwOptions
    ));

    DWORD dwErr = ERROR_SUCCESS;
    MEMORYPACKET mpCacheInfo;
    HANDLE hInternet = NULL;

    if (!(lpcbContainerInfo && lpContainerInfo))
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    mpCacheInfo.psStr = (PSTR)ALLOC_BYTES(*lpcbContainerInfo);
    if (!mpCacheInfo.psStr)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    mpCacheInfo.dwSize = mpCacheInfo.dwAlloc = *lpcbContainerInfo;

    hInternet = FindFirstUrlCacheContainerA(pdwModified,
        (LPINTERNET_CACHE_CONTAINER_INFOA)mpCacheInfo.psStr, &mpCacheInfo.dwSize, dwOptions);

    // TransformCacheContainerInfoToW takes the return value and decides if any further actions need to be taken
    // (eg. if successful, then try to convert from ansi to unicode; else if the ansi api failed, should we care?)
    
    if (!TransformCacheContainerInfoToW(
            hInternet ? TRUE : FALSE,
            (LPINTERNET_CACHE_CONTAINER_INFOA)mpCacheInfo.psStr,
            mpCacheInfo.dwSize,
            lpContainerInfo,
            lpcbContainerInfo))
    {
        if (hInternet)
        {
            FindCloseUrlCache(hInternet);
            hInternet = NULL;
        }
    }

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(hInternet);
    return hInternet;
}

URLCACHEAPI
BOOL
WINAPI
FindNextUrlCacheContainerW(
    IN HANDLE hEnumHandle,
        OUT LPINTERNET_CACHE_CONTAINER_INFOW lpContainerInfo,
        IN OUT LPDWORD lpcbContainerInfo
    )
{
    ENTER_CACHE_API ((DBG_API, Bool, "FindNextContainerW",
        "%#x, %#x, %#x",
        hEnumHandle, 
        lpContainerInfo,
        lpcbContainerInfo
    ));

    DWORD dwErr = ERROR_SUCCESS;
    MEMORYPACKET mpCacheInfo;
    BOOL fResult = FALSE;

    if (!(lpcbContainerInfo && lpContainerInfo))
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    mpCacheInfo.psStr = (PSTR)ALLOC_BYTES(*lpcbContainerInfo);
    if (!mpCacheInfo.psStr)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    mpCacheInfo.dwSize = mpCacheInfo.dwAlloc = *lpcbContainerInfo;

    fResult = FindNextUrlCacheContainerA(
                    hEnumHandle,
                    (LPINTERNET_CACHE_CONTAINER_INFOA)mpCacheInfo.psStr, 
                    &mpCacheInfo.dwSize);

    // TransformCacheContainerInfoToW takes the return value and decides if any further actions need to be taken
    // (eg. if successful, then try to convert from ansi to unicode; else if the ansi api failed, should we care?)
    
    fResult = TransformCacheContainerInfoToW(
                fResult,
                (LPINTERNET_CACHE_CONTAINER_INFOA)mpCacheInfo.psStr,
                mpCacheInfo.dwSize,
                lpContainerInfo,
                lpcbContainerInfo);

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}

/* here's the struct referred to below 

typedef struct _INTERNET_CACHE_CONFIG_INFOA {
    DWORD dwStructSize;
    DWORD dwContainer;
    DWORD dwQuota;
    DWORD dwReserved4;
    BOOL  fPerUser;
    DWORD dwSyncMode;
    DWORD dwNumCachePaths;
    union 
    { 
        struct 
        {
            CHAR   CachePath[MAX_PATH];
            DWORD dwCacheSize;
        };
        INTERNET_CACHE_CONFIG_PATH_ENTRYA CachePaths[ANYSIZE_ARRAY];
    };
    DWORD dwNormalUsage;
    DWORD dwExemptUsage;
} INTERNET_CACHE_CONFIG_INFOA, * LPINTERNET_CACHE_CONFIG_INFOA;

*/

#define ICCIA_FIXED_PORTION_SIZE ((sizeof(DWORD)*6)+sizeof(BOOL))

URLCACHEAPI
BOOL
WINAPI
GetUrlCacheConfigInfoW(
    OUT LPINTERNET_CACHE_CONFIG_INFOW pCacheConfigInfo,
    IN OUT LPDWORD pcbCacheConfigInfo,
    IN DWORD dwFieldControl
    )
{
    ENTER_CACHE_API ((DBG_API, Bool, "GetUrlCacheConfigInfoW", "%#x, %#x, %#x",
        pCacheConfigInfo, pcbCacheConfigInfo, dwFieldControl ));

    INTERNET_CACHE_CONFIG_INFOA iccia;
    
    iccia.dwContainer = pCacheConfigInfo->dwContainer;
    iccia.dwStructSize = sizeof(INTERNET_CACHE_CONFIG_INFOA);
    DWORD dwSize = sizeof(INTERNET_CACHE_CONFIG_INFOA);
    BOOL fResult = GetUrlCacheConfigInfoA(&iccia, &dwSize, dwFieldControl);
    if (fResult)
    {
        memcpy(pCacheConfigInfo, &iccia, ICCIA_FIXED_PORTION_SIZE);

        // These are appended to the _end_ of the structure.
        pCacheConfigInfo->dwNormalUsage = iccia.dwNormalUsage;
        pCacheConfigInfo->dwExemptUsage = iccia.dwExemptUsage;
        pCacheConfigInfo->dwStructSize = sizeof(INTERNET_CACHE_CONFIG_INFOW);
        if ((pCacheConfigInfo->dwContainer <= HISTORY) && (pCacheConfigInfo->dwContainer >= CONTENT))
        {
            MultiByteToWideChar(CP_ACP, 0, iccia.CachePath, -1, pCacheConfigInfo->CachePath, ARRAY_ELEMENTS(pCacheConfigInfo->CachePath));
        }
    }

    DEBUG_LEAVE_API (fResult);
    return fResult;
}


URLCACHEAPI
BOOL
WINAPI
SetUrlCacheConfigInfoW(
    LPCACHE_CONFIG_INFOW lpConfigConfigInfo,
    DWORD dwFieldControl
    )
{
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return( FALSE );
}

