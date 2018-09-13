/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    cachapia.cxx

Abstract:

    contains the ANSI version of cache mangemant APIs.

Author:

    Madan Appiah (madana)  12-Dec-1994

Environment:

    User Mode - Win32

Revision History:

--*/

#include <cache.hxx>


/*-----------------------------------------------------------------------------
CreateContainer
----------------------------------------------------------------------------*/
URLCACHEAPI
BOOL
WINAPI
CreateUrlCacheContainerA(
                 IN LPCSTR Name, 
                 IN LPCSTR CachePrefix, 
                 IN LPCSTR CachePath, 
                 IN DWORD KBCacheLimit,
                 IN DWORD dwContainerType,
                 IN DWORD dwOptions,
                 IN OUT LPVOID pvBuffer,
                 IN OUT LPDWORD cbBuffer
)
{
    ENTER_CACHE_API ((DBG_API, Bool, "CreateUrlCacheContainerA", "%q, %q, %q, %d, %d, %d, %#x, %#x",
        Name, CachePrefix, CachePath, KBCacheLimit, dwContainerType, dwOptions, pvBuffer, cbBuffer));

    DWORD Error;

    // Initialize globals
    if (!InitGlobals())
    {
        Error = ERROR_INTERNET_INTERNAL_ERROR;
        goto Cleanup;
    }

    Error = GlobalUrlContainers->CreateContainer(
                        Name,
                        CachePrefix,
                        CachePath,
                        KBCacheLimit,
                        dwOptions);

    LEAVE_CACHE_API();
}

URLCACHEAPI
BOOL
WINAPI
DeleteUrlCacheContainerA(
IN LPCSTR Name,
IN DWORD dwOptions)
{
    ENTER_CACHE_API ((DBG_API, Bool, "DeleteContainerA", "%q, %d", Name, dwOptions));

    DWORD Error;

    // Initialize globals
    if (!InitGlobals())
    {
        Error = ERROR_INTERNET_INTERNAL_ERROR;
        goto Cleanup;
    }

    Error = GlobalUrlContainers->DeleteContainer(
                        Name,
                        0
                        );

    LEAVE_CACHE_API();
}


URLCACHEAPI
HANDLE
WINAPI
FindFirstUrlCacheContainerA(
    IN OUT LPDWORD pdwModified,
    OUT LPINTERNET_CACHE_CONTAINER_INFOA lpContainerInfo,
    IN OUT LPDWORD lpdwContainerInfoBufferSize,
    IN DWORD dwOptions
)
{
    ENTER_CACHE_API ((DBG_API, Bool, "FindFirstContainerA",
        "%#x, %#x, %#x, %#x",
        pdwModified,
        lpContainerInfo,
        lpdwContainerInfoBufferSize,
        dwOptions
    ));

    DWORD Error;
    HANDLE hFind = NULL;


    // Initialize globals.
    if (!InitGlobals())
    {
        Error = ERROR_INTERNET_INTERNAL_ERROR;
        goto Cleanup;
    }

    hFind = GlobalUrlContainers->FindFirstContainer(pdwModified, 
        lpContainerInfo, lpdwContainerInfoBufferSize, dwOptions);

    if (hFind)
        Error = ERROR_SUCCESS;
    else
    {
        Error = GetLastError();
        // BUGBUG: Free hFind?
        // does the free take NULL?
    }

Cleanup:
    if( Error != ERROR_SUCCESS )
    {
        SetLastError( Error );
        DEBUG_ERROR(API, Error);
    }

    DEBUG_LEAVE_API (hFind);
    return hFind;
}

    
URLCACHEAPI
BOOL
WINAPI
FindNextUrlCacheContainerA(
IN HANDLE hFind, 
OUT LPINTERNET_CACHE_CONTAINER_INFOA lpContainerInfo,
IN OUT LPDWORD lpdwContainerInfoBufferSize
)
{
    ENTER_CACHE_API ((DBG_API, Bool, "FindNextContainerA",
        "%#x, %#x, %#x",
        hFind, 
        lpContainerInfo,
        lpdwContainerInfoBufferSize
    ));

    DWORD Error;
    DWORD i;


    // Initialize globals.
    if (!InitGlobals())
    {
        Error = ERROR_INTERNET_INTERNAL_ERROR;
        goto Cleanup;
    }


    if (GlobalUrlContainers->FindNextContainer(hFind, 
            lpContainerInfo, lpdwContainerInfoBufferSize))
        Error = ERROR_SUCCESS;
    else
        Error = GetLastError();

    LEAVE_CACHE_API();
}

URLCACHEAPI
BOOL
WINAPI
CreateUrlCacheEntryA(
    IN LPCSTR   lpszUrlName,
    IN DWORD    dwExpectedFileSize,
    IN LPCSTR   lpszFileExtension,
    OUT LPSTR lpszFileName,
    IN DWORD dwReserved
    )
/*++

Routine Description:

    This function creates a temperary file in the cache storage. This call
    is called by the application when it receives a url file from a
    server. When the receive is completed it caches this file to url cache
    management, which will move the file to permanent cache file. The idea
    is the cache file is written only once directly into the cache store.

Arguments:

    lpszUrlName : name of the url file (unused now).

    lpszFileExtension: File extension for the saved data file

    dwExpectedFileSize : expected size of the incoming file. If it is unknown
        this value is set to null.

    lpszFileName : pointer to a buffer that receives the full path name of
        the the temp file.

    dwReserved : reserved for future use.

Return Value:

    Windows Error Code.

--*/
{
    ENTER_CACHE_API ((DBG_API, Bool, "CreateUrlCacheEntryA", "%q, %q, %d, %q, %#x",
        lpszUrlName, lpszFileExtension, dwExpectedFileSize, lpszFileName, dwReserved));

    DWORD Error;

    //
    // validate parameters.
    //

    if( IsBadUrl( lpszUrlName ) || IsBadWriteFileName( lpszFileName )  ) {
        Error =  ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Initialize globals
    if (!InitGlobals())
    {
        Error = ERROR_INTERNET_INTERNAL_ERROR;
        goto Cleanup;
    }

    // Null first char in lpszFileName cues CreateUniqueFile
    // to generate a file name from scratch. Otherwise,
    // an attempt will be made to generate the filename
    // using the contents of the buffer.
    *lpszFileName = '\0';

    Error = GlobalUrlContainers->CreateUniqueFile(
                        lpszUrlName,
                        dwExpectedFileSize,
                        lpszFileExtension,
                        lpszFileName, 
                        NULL
                        );

    LEAVE_CACHE_API();
}

URLCACHEAPI
BOOL
WINAPI
CommitUrlCacheEntryA(
    IN LPCSTR lpszUrlName,
    IN LPCSTR lpszLocalFileName,
    IN FILETIME ExpireTime,
    IN FILETIME LastModifiedTime,
    IN DWORD CacheEntryType,
    IN LPBYTE lpHeaderInfo,
    IN DWORD dwHeaderSize,
    IN LPCSTR lpszFileExtension,
    IN LPCSTR lpszOriginalUrl
    )

/*++

Routine Description:

    This API caches a specified URL in the internet service  cache
    storage. It creates a database entry of the URL info and moves the
    URL file to cache storage.

Arguments:

    lpszUrlName : name of the URL that is cached.

    lpszLocalFileName : name of the local file where the URL data is
        stored. This file will be moved to an another file in cache storage, so
        this name is invalid after this api successfully returns. The
        name should include full path.

    ExpireTime : Expire time (GMT) of the file being cached. If it is
        unknown set it to zero.

    LastModifiedTime : Last modified time of this file. if this value is
        zero, current time is set as the last modified time.

    CacheEntryType : type of this new entry.

    lpHeaderInfo : if this pointer is non-NULL, it stores the HeaderInfo
        data as part of the URL entry in the memory mapped file, otherwise
        the app may store it else where. The size of the header info is
        specified by the HeaderSize parameter.

    dwHeaderSize : size of the header info associated with this URL, this
        can be non-zero even if the HeaderInfo specified above is NULL.

    lpszFileExtension :  file extension used to create this file.

    dwReserved : reserved for future use.

Return Value:

    Windows Error code.

--*/
{
    ENTER_CACHE_API ((DBG_API, Bool, "CommitUrlCacheEntryA",
        "%q, %q, <expires>, <last-mod>, %d, %#x, %d, %q, %q",
        lpszUrlName,
        lpszLocalFileName,
        CacheEntryType,
        lpHeaderInfo,
        dwHeaderSize,
        lpszFileExtension,
        lpszOriginalUrl
    ));

    DWORD Error;

    // validate parameters.
    if( IsBadUrl( lpszUrlName ) ||
        ( lpszLocalFileName ? IsBadReadFileName( lpszLocalFileName ) : FALSE ) ) 
    {
        Error =  ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if( lpHeaderInfo != NULL ) 
    {
        if( IsBadReadPtr(lpHeaderInfo, dwHeaderSize) ) 
        {
            Error =  ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
    }

    if( lpszFileExtension != NULL ) 
    {
        if( IsBadReadPtr(lpszFileExtension, 3) ) 
        {
            Error =  ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
    }

    FILETIME     ftPostCheck;
    ftPostCheck.dwLowDateTime = 0;
    ftPostCheck.dwHighDateTime = 0; 
  
    // Record args in structure.
    AddUrlArg Args;
    Args.pszUrl      = lpszUrlName;
    Args.pszFilePath = lpszLocalFileName;
    Args.dwFileSize  = 0;
    Args.qwExpires   = FT2LL(ExpireTime);
    Args.qwLastMod   = FT2LL(LastModifiedTime);
    Args.qwPostCheck = FT2LL(ftPostCheck);
    Args.ftCreate = LastModifiedTime;
    Args.dwEntryType = CacheEntryType;
    Args.pbHeaders   = (LPSTR)lpHeaderInfo;
    Args.cbHeaders   = dwHeaderSize;
    Args.pszFileExt  = lpszFileExtension;
    Args.pszRedirect = lpszOriginalUrl ? (LPSTR) lpszOriginalUrl : NULL;
    Args.fImage      = FALSE;

    Error = UrlCacheCommitFile(&Args);

    LEAVE_CACHE_API();
}



URLCACHEAPI
BOOL
WINAPI
RetrieveUrlCacheEntryFileA(
    IN LPCSTR  lpszUrlName,
    OUT LPCACHE_ENTRY_INFOA lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize,
    IN DWORD dwReserved
    )
/*++

Routine Description:

    This API retrieves the specified URL file. When the file is retrieved
    it also checked out to the user to use. The user has to call
    UnlockUrlFile when he/she finished using it.

Arguments:

    lpszUrlName : name of the URL that is being retrieved.

    lpCacheEntryInfo : pointer to the url info structure that receives the url
        info.

    lpdwCacheEntryInfoBufferSize : pointer to a location where length of
        the above buffer is passed in. On return, this contains the length
        of the above buffer that is fulled in.

    dwReserved : reserved for future use.

Return Value:

    Windows Error code.

--*/
{
    ENTER_CACHE_API ((DBG_API, Bool, "RetrieveUrlCacheEntryFileA","%q, %#x, %#x, %#x",
        lpszUrlName, lpCacheEntryInfo, lpdwCacheEntryInfoBufferSize, dwReserved));
        
    DWORD Error;

    // validate parameters.
    if( IsBadUrl( lpszUrlName ) ||
            IsBadWriteUrlInfo(
                lpCacheEntryInfo,
                *lpdwCacheEntryInfoBufferSize) ) {

        Error =  ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (!InitGlobals())
    {
        Error = ERROR_INTERNET_INTERNAL_ERROR;
        goto Cleanup;
    }

    Error = GlobalUrlContainers->RetrieveUrl(
                        lpszUrlName,
                        (lpCacheEntryInfo ? &lpCacheEntryInfo : NULL),
                        lpdwCacheEntryInfoBufferSize,
                        LOOKUP_URL_CREATE,
                        RETRIEVE_WITH_CHECKS);

    LEAVE_CACHE_API();
}

URLCACHEAPI
HANDLE
WINAPI
RetrieveUrlCacheEntryStreamA(
    IN LPCSTR  lpszUrlName,
    OUT LPCACHE_ENTRY_INFOA lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize,
    IN BOOL fRandomRead,
    IN DWORD dwReserved
    )
/*++

Routine Description:

    This API retrieves the specified URL file. When the file is retrieved
    it also checked out to the user to use. The user has to call
    UnlockUrlFile when he/she finished using it.

Arguments:

    lpszUrlName : name of the URL that is being retrieved.

    lpCacheEntryInfo : pointer to the url info structure that receives the url
        info.

    lpdwCacheEntryInfoBufferSize : pointer to a location where length of
        the above buffer is passed in. On return, this contains the length
        of the above buffer that is fulled in.

    fRandomRead : if this flag is set to TRUE, then stream is open for
        random access.

    dwReserved: must pass 0

Return Value:

    Windows Error code.

--*/
{
    ENTER_CACHE_API ((DBG_API, Handle, "RetrieveUrlCacheEntryStreamA",
        "%q, %#x, %#x, %d, %#x",
        lpszUrlName,
        lpCacheEntryInfo,
        lpdwCacheEntryInfoBufferSize,
        fRandomRead,
        dwReserved
    ));

    BOOL fLocked = FALSE;
    HANDLE hStream = NULL;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD Error, dwFileSize;

    // Validate parameters.
    if(   IsBadUrl( lpszUrlName )
       || IsBadWriteUrlInfo(lpCacheEntryInfo, *lpdwCacheEntryInfoBufferSize))
    {
        Error =  ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Initialize globals.
    if (!InitGlobals())
    {
        Error = ERROR_INTERNET_INTERNAL_ERROR;
        goto Cleanup;
    }
    
    Error = GlobalUrlContainers->RetrieveUrl
        (lpszUrlName, (lpCacheEntryInfo ? &lpCacheEntryInfo : NULL), lpdwCacheEntryInfoBufferSize,
            LOOKUP_URL_NOCREATE, RETRIEVE_WITHOUT_CHECKS);

    if( Error != ERROR_SUCCESS )
        goto Cleanup;
    fLocked = TRUE;        

    // Allocate a stream handle.
    CACHE_STREAM_CONTEXT_HANDLE* pStream;
    LOCK_CACHE();
    hStream = HandleMgr.Alloc (sizeof(CACHE_STREAM_CONTEXT_HANDLE));
    if (hStream)
    {        
        pStream = (CACHE_STREAM_CONTEXT_HANDLE*) HandleMgr.Map (hStream);
        INET_ASSERT (pStream);
    }
    UNLOCK_CACHE();
    if (!hStream)
    {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    // Open the file.
    hFile = CreateFile
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
        Error = GetLastError();
        goto Cleanup;
    }

    dwFileSize = GetFileSize(hFile, NULL);

    if (dwFileSize != lpCacheEntryInfo->dwSizeLow) 
    {
        Error = (dwFileSize==0xFFFFFFFF) ? GetLastError() : ERROR_INVALID_DATA;
        goto Cleanup;
    }

    pStream->FileHandle = hFile;

    // Copy URL name storage.
    pStream->SourceUrlName = NewString(lpszUrlName);
    if( !pStream->SourceUrlName)
    {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    Error = ERROR_SUCCESS;

Cleanup:

    if( Error != ERROR_SUCCESS )
    {
        if (hStream)
        {
            HandleMgr.Free (hStream);
            hStream = NULL;
        }
        if (hFile)
            CloseHandle (hFile);
        if (fLocked)
            GlobalUrlContainers->UnlockUrl(lpszUrlName);
        SetLastError (Error);
        DEBUG_ERROR(API, Error);
    }

    DEBUG_LEAVE_API (hStream);
    return hStream;
}


URLCACHEAPI
BOOL
WINAPI
GetUrlCacheEntryInfoA(
    IN LPCSTR lpszUrlName,
    OUT LPCACHE_ENTRY_INFOA lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize
    )
/*++

Routine Description:

    This function retrieves the specified cache entry info.

Arguments:

    lpszUrlName : name of the url file (unused now).

    lpCacheEntryInfo : pointer to the url info structure that receives the url
        info.

    lpdwCacheEntryInfoBufferSize : pointer to a location where length of
        the above buffer is passed in. On return, this contains the length
        of the above buffer that is fulled in.

Return Value:

    Windows Error Code.

--*/
{
    ENTER_CACHE_API ((DBG_API, Bool, "GetUrlCacheEntryInfoA", "%q, %#x, %#x",
        lpszUrlName, lpCacheEntryInfo, lpdwCacheEntryInfoBufferSize));

    DWORD Error;

    // Validate parameters.
    if( IsBadUrl( lpszUrlName ) ||
        (lpCacheEntryInfo && !lpdwCacheEntryInfoBufferSize) ||
        (lpCacheEntryInfo && lpdwCacheEntryInfoBufferSize && IsBadWriteUrlInfo(
                lpCacheEntryInfo,
                *lpdwCacheEntryInfoBufferSize) ) )
    {
        Error =  ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Initialize globals.
    if (!InitGlobals())
    {
        Error = ERROR_INTERNET_INTERNAL_ERROR;
        goto Cleanup;
    }

    Error = GlobalUrlContainers->GetUrlInfo(
                        lpszUrlName,
                        lpCacheEntryInfo,
                        lpdwCacheEntryInfoBufferSize,
                        LOOKUP_URL_NOCREATE,
                        0);

    LEAVE_CACHE_API();
}


BOOLAPI
GetUrlCacheEntryInfoExA(
    IN LPCSTR       lpszUrl,
    OUT LPINTERNET_CACHE_ENTRY_INFOA lpCEI,
    IN OUT LPDWORD  lpcbCEI,
    OUT LPSTR       lpszOut,
    IN OUT LPDWORD  lpcbOut,
    LPVOID          lpReserved,
    DWORD           dwFlags
)
{
    ENTER_CACHE_API ((DBG_API, Bool, "GetUrlCacheEntryInfoExA",
        "%q, %#x, %#x, %#x, %#x, %#x, %#x", lpszUrl, lpCEI, lpcbCEI, lpszOut, lpcbOut, lpReserved, dwFlags));

    DWORD Error;

    // Validate parameters
    // NOTE: once the following params change, edit GetUrlCacheEntryInfoExW accordingly.
    if (   IsBadUrl(lpszUrl)
        || lpszOut
        || lpcbOut 
        || lpReserved
       )
    {
        INET_ASSERT (FALSE);
        Error = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Initialize globals.
    if (!InitGlobals())
    {
        Error = ERROR_INTERNET_INTERNAL_ERROR;
        goto Cleanup;
    }

    // We allow mixing of INTERNET_CACHE_FLAG_ALLOW_COLLISIONS with lookup flags
    Error = GlobalUrlContainers->GetUrlInfo
        (lpszUrl, lpCEI, lpcbCEI, LOOKUP_URL_TRANSLATE | (dwFlags & INTERNET_CACHE_FLAG_ALLOW_COLLISIONS), dwFlags);
        
    LEAVE_CACHE_API();
}


URLCACHEAPI
BOOL
WINAPI
SetUrlCacheEntryInfoA(
    IN LPCSTR lpszUrlName,
    IN LPCACHE_ENTRY_INFOA lpCacheEntryInfo,
    IN DWORD dwFieldControl
    )
/*++

Routine Description:

    This function sets the specified fields of the cache entry info.

Arguments:

    lpszUrlName : name of the url file (unused now).

    lpCacheEntryInfo : pointer to the url info structure that has the url info to
        be set.

    dwFieldControl : Bitmask that specifies the fields to be set.

Return Value:

    Windows Error Code.

--*/
{
    ENTER_CACHE_API ((DBG_API, Bool, "SetUrlCacheEntryInfoA", "%q, %#x, %d",
        lpszUrlName, lpCacheEntryInfo, dwFieldControl));

    DWORD Error;

    //
    // validate parameters.
    //

    if( IsBadUrl( lpszUrlName ) ||
            IsBadReadUrlInfo( lpCacheEntryInfo )) {

        Error =  ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Initialize globals.
    if (!InitGlobals())
    {
        Error = ERROR_INTERNET_INTERNAL_ERROR;
        goto Cleanup;
    }

    Error = GlobalUrlContainers->SetUrlInfo(
                        lpszUrlName,
                        lpCacheEntryInfo,
                        dwFieldControl );

    LEAVE_CACHE_API();
}

URLCACHEAPI
HANDLE
WINAPI
FindFirstUrlCacheEntryA(
    IN LPCSTR lpszUrlSearchPattern,
    OUT LPCACHE_ENTRY_INFOA lpFirstCacheEntryInfo,
    IN OUT LPDWORD lpdwFirstCacheEntryInfoBufferSize
    )
/*++

Routine Description:

    This member function starts the cache entries enumeration and returns
    the first entry in the cache.

Arguments:

    lpszUrlSearchPattern : pointer to a search pattern string. Currently
        it is not implemented.

    lpFirstCacheEntryInfo : pointer to a cache entry info structure.

Return Value:

    Returns the find first handle. If the returned handle is NULL,
    GetLastError() returns the extended error code.

--*/
{
    ENTER_CACHE_API ((DBG_API, Bool, "FindFirstUrlCacheEntryA",
        "%q, %#x, %#x",
        lpszUrlSearchPattern,
        lpFirstCacheEntryInfo,
        lpdwFirstCacheEntryInfoBufferSize
    ));

    DWORD Error;
    HANDLE hFind = 0;

    // Validate parameters.
    if (IsBadWriteUrlInfo(lpFirstCacheEntryInfo,
                          *lpdwFirstCacheEntryInfoBufferSize))
    {
        Error =  ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Initialize globals.
    if (!InitGlobals())
    {
        Error = ERROR_INTERNET_INTERNAL_ERROR;
        goto Cleanup;
    }

    // Get the first entry.
    Error = GlobalUrlContainers->FindNextEntry(&hFind, 
                                               lpszUrlSearchPattern, 
                                               lpFirstCacheEntryInfo, 
                                               lpdwFirstCacheEntryInfoBufferSize, 
                                               URLCACHE_FIND_DEFAULT_FILTER,
                                               NULL,
                                               FIND_FLAGS_OLD_SEMANTICS);

Cleanup:

    if( Error != ERROR_SUCCESS )
    {
        GlobalUrlContainers->FreeFindHandle(hFind);
        hFind = NULL;
        SetLastError(Error);
        DEBUG_ERROR(API, Error);
    }

    DEBUG_LEAVE_API (hFind);
    return hFind;
}

URLCACHEAPI
BOOL
WINAPI
FindNextUrlCacheEntryA(
    IN HANDLE hFind,
    OUT LPCACHE_ENTRY_INFOA lpNextCacheEntryInfo,
    IN OUT LPDWORD lpdwNextCacheEntryInfoBufferSize
    )
/*++

Routine Description:

    This member function returns the next entry in the cache.

Arguments:

    hEnumHandle : Find First handle.

    lpFirstCacheEntryInfo : pointer to a cache entry info structure.

Return Value:

    Returns the find first handle. If the returned handle is NULL,
    GetLastError() returns the extended error code. It returns
    ERROR_NO_MORE_ITEMS after it returns the last entry in the cache.

--*/
{
    ENTER_CACHE_API ((DBG_API, Bool, "FindNextUrlCacheEntryA",
        "%#x, %#x, %#x",
        hFind, 
        lpNextCacheEntryInfo,
        lpdwNextCacheEntryInfoBufferSize
    ));

    DWORD Error = ERROR_SUCCESS;
    CACHE_FIND_FIRST_HANDLE* pFind;

    // Validate parameters.
    if (!hFind || IsBadWriteUrlInfo(lpNextCacheEntryInfo,
                                    *lpdwNextCacheEntryInfoBufferSize)) 
    {
        Error =  ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Initialize globals.
    if (!InitGlobals())
    {
        Error = ERROR_INTERNET_INTERNAL_ERROR;
        goto Cleanup;
    }


    // Get the next entry.
    Error = GlobalUrlContainers->FindNextEntry(&hFind, 
                                               NULL, 
                                               lpNextCacheEntryInfo, 
                                               lpdwNextCacheEntryInfoBufferSize,
                                               URLCACHE_FIND_DEFAULT_FILTER,
                                               NULL,
                                               FIND_FLAGS_OLD_SEMANTICS);



Cleanup:
    if (Error!=ERROR_SUCCESS)
    {
        SetLastError(Error);
        DEBUG_ERROR(INET, Error);
    }
    DEBUG_LEAVE_API(Error==ERROR_SUCCESS);
    return (Error == ERROR_SUCCESS );
}


INTERNETAPI
HANDLE
WINAPI
FindFirstUrlCacheEntryExA(
    IN     LPCSTR    lpszUrlSearchPattern,
    IN     DWORD     dwFlags,
    IN     DWORD     dwFilter,
    IN     GROUPID   GroupId,
    OUT    LPINTERNET_CACHE_ENTRY_INFOA pEntryInfo,
    IN OUT LPDWORD   pcbEntryInfo,
    OUT    LPVOID    lpGroupAttributes,     // must pass NULL
    IN OUT LPDWORD   pcbGroupAttributes,    // must pass NULL
    IN     LPVOID    lpReserved             // must pass NULL
    )
{
    ENTER_CACHE_API ((DBG_API, Bool, "FindFirstUrlCacheEntryExA",
        "%q, %#x, %#x, %#x, %#x, %#x, %#x, %#x, %#x",
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

    DWORD Error;
    HANDLE hFind = NULL;

    // Validate parameters.
    if (IsBadWritePtr (pcbEntryInfo, sizeof(DWORD)))
    {
        Error = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Initialize globals.
    if (!InitGlobals())
    {
        Error = ERROR_INTERNET_INTERNAL_ERROR;
        goto Cleanup;
    }

    // Get the first entry.
    Error = GlobalUrlContainers->FindNextEntry(&hFind, 
                                               lpszUrlSearchPattern, 
                                               pEntryInfo, 
                                               pcbEntryInfo,
                                               dwFilter,
                                               GroupId,
                                               dwFlags);

Cleanup:

    if( Error != ERROR_SUCCESS )
    {
        if (hFind)
        {
            GlobalUrlContainers->FreeFindHandle(hFind);
            hFind = NULL;
        }
        SetLastError(Error);
        DEBUG_ERROR(API, Error);
    }

    DEBUG_LEAVE_API (hFind);
    return hFind;    
}

BOOLAPI
FindNextUrlCacheEntryExA(
    IN     HANDLE    hFind,
    OUT    LPINTERNET_CACHE_ENTRY_INFOA pEntryInfo,
    IN OUT LPDWORD   pcbEntryInfo,
    OUT    LPVOID    lpGroupAttributes,     // must pass NULL
    IN OUT LPDWORD   pcbGroupAttributes,    // must pass NULL
    IN     LPVOID    lpReserved             // must pass NULL
    )
{
    ENTER_CACHE_API ((DBG_API, Bool, "FindNextUrlCacheEntryExA",
        "%#x, %#x, %#x, %#x, %#x, %#x",
        hFind,
        pEntryInfo,
        pcbEntryInfo,
        lpGroupAttributes,
        pcbGroupAttributes,
        lpReserved
    ));

    DWORD Error;

    // Validate parameters.
    if (!hFind || IsBadWritePtr (pcbEntryInfo, sizeof(DWORD)))
    {
        Error = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Initialize globals.
    if (!InitGlobals())
    {
        Error = ERROR_INTERNET_INTERNAL_ERROR;
        goto Cleanup;
    }
    

    // Get the next entry.
    Error = GlobalUrlContainers->FindNextEntry(&hFind, 
                                               NULL, 
                                               pEntryInfo, 
                                               pcbEntryInfo, 
                                               NULL, 
                                               NULL,
                                               NULL);
    

    LEAVE_CACHE_API();
}

URLCACHEAPI
BOOL
WINAPI
FreeUrlCacheSpaceA(
    IN LPCSTR lpszCachePath,
    IN DWORD dwFactor,
    IN DWORD dwFilter
    )
/*++

Routine Description:

    This function cleans up the cache entries in the specified ccahe
    path to make space for future cache entries.

Arguments:

    dwFactor: % of free space

Return Value:

    TRUE if the cleanup is successful. Otherwise FALSE, GetLastError()
    returns the extended error.

--*/
{
    DWORD Error;
    
    ENTER_CACHE_API ((DBG_API, Bool, "FreeUrlCacheSpace", 
        "<path>,%d, %#x", dwFactor, dwFilter));

    // Initialize globals.
    if (!InitGlobals())
    {
        Error = ERROR_INTERNET_INTERNAL_ERROR;
        goto Cleanup;
    }

    Error = GlobalUrlContainers->CleanupUrls(lpszCachePath, dwFactor, dwFilter);

    LEAVE_CACHE_API();
}

URLCACHEAPI
BOOL
WINAPI
UnlockUrlCacheEntryFileA(
    LPCSTR lpszUrlName,
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
   
    DWORD Error;
    DWORD i;

    ENTER_CACHE_API ((DBG_API, Bool, "UnlockUrlCacheEntryFile",
        "%q, %#x", lpszUrlName, dwReserved));

    // validate parameters.
    if( IsBadUrl( lpszUrlName )  ) {
        Error =  ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Initialize globals
    if (!InitGlobals())
    {
        Error = ERROR_INTERNET_INTERNAL_ERROR;
        goto Cleanup;
    }

    Error = GlobalUrlContainers->UnlockUrl(lpszUrlName);

    LEAVE_CACHE_API();
}

URLCACHEAPI
BOOL
WINAPI
DeleteUrlCacheEntryA(
    IN LPCSTR lpszUrlName
    )
{
    ENTER_CACHE_API ((DBG_API, Bool, "DeleteUrlCacheEntry",
        "%q", lpszUrlName));


    DWORD Error;

    // Validate parameters.
    if( IsBadUrl( lpszUrlName ) ) {
        Error =  ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Initialize globals
    if (!InitGlobals())
    {
        Error = ERROR_INTERNET_INTERNAL_ERROR;
        goto Cleanup;
    }
    
    Error = GlobalUrlContainers->DeleteUrl(lpszUrlName);

    LEAVE_CACHE_API();
}

BOOLAPI
SetUrlCacheEntryGroupA(
    IN LPCSTR   lpszUrlName,
    IN DWORD    dwFlags,
    IN GROUPID  GroupId,
    IN LPBYTE   pbGroupAttributes, // must pass NULL
    IN DWORD    cbGroupAttributes, // must pass 0
    IN LPVOID   lpReserved         // must pass NULL
    )
{
    ENTER_CACHE_API ((DBG_API, Bool, "SetUrlCacheEntryGroupA", 
        "%q, %#x, %#x, %#x, %#x, %#x", lpszUrlName, dwFlags, GroupId, pbGroupAttributes, cbGroupAttributes, lpReserved));

    DWORD Error;

    // Validate parameters.
    if (IsBadUrl(lpszUrlName)
        || !GroupId
        || pbGroupAttributes
        || cbGroupAttributes
        || lpReserved
        )
    {
        Error =  ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Initialize globals
    if (!InitGlobals())
    {
        Error = ERROR_INTERNET_INTERNAL_ERROR;
        goto Cleanup;
    }

    Error = GlobalUrlContainers->SetUrlGroup (lpszUrlName, dwFlags, GroupId);

    LEAVE_CACHE_API();
}



URLCACHEAPI
BOOL
WINAPI
GetUrlCacheGroupAttributeA(
    IN      GROUPID                         gid,
    IN      DWORD                           dwFlags,
    IN      DWORD                           dwAttributes,
    OUT     LPINTERNET_CACHE_GROUP_INFOA    lpGroupInfo,
    IN OUT  LPDWORD                         lpdwGroupInfo,
    IN OUT  LPVOID                          lpReserved
)
{
    ENTER_CACHE_API ((DBG_API, Bool, "GetUrlCacheGroupAttributeA",
        "%#x, %d, %d, %#x, %#x, %#x", 
        gid, dwFlags, dwAttributes, lpGroupInfo, lpdwGroupInfo, lpReserved ));

    DWORD Error;

    // Validate parameters.
    if( !lpGroupInfo ||
        !lpdwGroupInfo ||
        IsBadWriteUrlInfo(lpGroupInfo, *lpdwGroupInfo) ) 
    {
        Error = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if( *lpdwGroupInfo < sizeof(INTERNET_CACHE_GROUP_INFOA) )
    {
        Error = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }

    // Initialize globals.
    if (!InitGlobals())
    {
        Error = ERROR_INTERNET_INTERNAL_ERROR;
        goto Cleanup;
    }

    Error = GlobalUrlContainers->GetGroupAttributes(
                        gid,
                        dwAttributes,
                        lpGroupInfo,
                        lpdwGroupInfo );
    LEAVE_CACHE_API();
}

URLCACHEAPI
BOOL
WINAPI
SetUrlCacheGroupAttributeA(
    IN      GROUPID                         gid,
    IN      DWORD                           dwFlags,
    IN      DWORD                           dwAttributes,
    IN      LPINTERNET_CACHE_GROUP_INFOA    lpGroupInfo,
    IN OUT  LPVOID                          lpReserved
)
{
    ENTER_CACHE_API ((DBG_API, Bool, "SetUrlCacheGroupAttributeA",
        "%#x, %d, %d, %#x, %#x", 
        gid, dwFlags, dwAttributes, lpGroupInfo, lpReserved));

    DWORD Error;

    // validate parameters.
    if( IsBadReadPtr(lpGroupInfo, sizeof(INTERNET_CACHE_GROUP_INFOA) ) ) 
    {
        Error = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Initialize globals.
    if (!InitGlobals())
    {
        Error = ERROR_INTERNET_INTERNAL_ERROR;
        goto Cleanup;
    }

    Error = GlobalUrlContainers->SetGroupAttributes(
            gid, dwAttributes, lpGroupInfo);

    LEAVE_CACHE_API();
}

BOOLAPI
IsUrlCacheEntryExpiredA(
    IN      LPCSTR       lpszUrlName,
    IN      DWORD        dwFlags,
    IN OUT  FILETIME*    pftLastModifiedTime
)
{
    BOOL                        bRet = TRUE;
    CACHE_ENTRY_INFOEX*         pCEI = NULL;    
    DWORD                       cbCEI;
    DWORD                       dwError;
    BOOL                        bLazy = FALSE;
    BOOL                        fLocked = FALSE;

    // Validate parameters.
    if( IsBadUrl( lpszUrlName ) || !pftLastModifiedTime ) {
        INET_ASSERT(FALSE); 
        return ERROR_INVALID_PARAMETER;
    }

    // set out LastModTime to 0
    pftLastModifiedTime->dwLowDateTime = 0 ;
    pftLastModifiedTime->dwHighDateTime = 0 ;


    if (!InitGlobals())
    {
        INET_ASSERT(FALSE);
        return ERROR_INTERNET_INTERNAL_ERROR;
    }

    //
    // BUGBUG
    // ideally, we should use GlobalUrlContainers->GetUrlInfo()
    // with NO_ALLOCATION and HEADONLY flag for perf.
    // however, there is a flag (lookup flag v.s entry flag) collision 
    // in that code path which prevents this working
    // so we use this anti-perf RetrieveUrl for now until that one
    // gets fixed 
    //                                           --DanpoZ, 98.09.09
    
    // Find the container and search the index.
    dwError = GlobalUrlContainers->RetrieveUrl(
                    lpszUrlName, 
                    (CACHE_ENTRY_INFO**) &pCEI, 
                    &cbCEI, 
                    (dwFlags & INTERNET_FLAG_FWD_BACK)?
                        LOOKUP_URL_TRANSLATE : LOOKUP_URL_NOCREATE,
                    RETRIEVE_WITHOUT_CHECKS | RETRIEVE_WITH_ALLOCATION);

    
    // not found in cache
    if( dwError != ERROR_SUCCESS )
        goto Cleanup;    

    fLocked = TRUE;

    // found in cache, get the last modified time
    *pftLastModifiedTime = pCEI->LastModifiedTime;

    bRet = IsExpired(pCEI, dwFlags, &bLazy);
    if( bRet && bLazy )
    {
        //
        // the entry is not expired, however, we need to post-fetch
        // so we have to return EXPIRED back to trident to force them
        // issue a binding, on the new binding, urlmon-wininet returns
        // the cache content and queue a background update
        // (an alternative would be to ask trident to catch this case
        //  and call background update themself)
        // 
        bRet = FALSE;
    }

Cleanup:
    if( pCEI )
        FREE_MEMORY(pCEI);

    if (fLocked)
        GlobalUrlContainers->UnlockUrl(lpszUrlName);

    return bRet;
}
