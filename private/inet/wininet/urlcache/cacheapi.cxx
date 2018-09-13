/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    cacheapi.cxx

Abstract:

    contains the URL cache mangemant APIs.

Author:

    Madan Appiah (madana)  12-Dec-1994

Environment:

    User Mode - Win32

Revision History:

    Shishir Pardikar (shishirp) added: (as of 7/6/96)

    1) Prefixed containers for supporting cookies and history
    2) Default init, for distributing winint without setup
    3) Crossprocess versionchecking scheme to allow all wininets
       to know about registry change

--*/

#include <cache.hxx>
#include <time.h>
#include <resource.h>
    
URLCACHEAPI
BOOL
WINAPI
UnlockUrlCacheEntryStream(
    HANDLE hStream,
    IN DWORD dwReserved
    )
/*++

Routine Description:

    This API checks in the file that was check out as part of
    RetrieveUrlFile API.

Arguments:

    hStreamHandle : stream handle returned by a RetrieveUrlCacheEntryStream call.

    dwReserved : reserved for future use.

Return Value:

    Windows Error code.

--*/
{
    ENTER_CACHE_API ((DBG_API, Bool, "UnlockUrlCacheEntryStream",
        "%#x, %#x", hStream, dwReserved))

    DWORD Error;


    // Map and validate handle.
    CACHE_STREAM_CONTEXT_HANDLE *pStream;
    LOCK_CACHE();
    pStream = (CACHE_STREAM_CONTEXT_HANDLE *) HandleMgr.Map(hStream);
    UNLOCK_CACHE();
    if (!pStream)
    {
        Error = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }
    
        
    TcpsvcsDbgAssert(pStream->SourceUrlName != NULL );

    //
    // close file handle.
    //

    CloseHandle( pStream->FileHandle );

    //
    // unlock cache file.
    //

    if(!UnlockUrlCacheEntryFile(pStream->SourceUrlName, dwReserved) )
        Error = GetLastError();
    else
        Error = ERROR_SUCCESS;

    //
    // freeup url name data buffer.
    //

    FREE_MEMORY (pStream->SourceUrlName);

    //
    // free up context structure.
    //

    LOCK_CACHE();
    HandleMgr.Free (hStream);
    UNLOCK_CACHE();

    LEAVE_CACHE_API();
}

URLCACHEAPI
BOOL
WINAPI
ReadUrlCacheEntryStream(
    IN HANDLE hStream,
    IN DWORD dwLocation,
    IN OUT LPVOID lpBuffer,
    IN OUT LPDWORD lpdwLen,
    IN DWORD Reserved
    )
/*++

Routine Description:

    This API provides a method  of reading the cached data from a stream
    which has been opened using the RetrieveUrlCacheEntryStream API.

Arguments:

    hStream : Handle that was returned by the RetrieveCacheEntryStream API.

    dwLocation  : file offset to read from.

    lpBuffer : Pointer to a buffer where the data is read.

    lpdwLen : Pointer to a DWORD location where the length of the above buffer passed in, on return it contains the actual length of the data read.

    dwReserved : For future use.

Return Value:

    Windows Error code.

--*/
{
    ENTER_CACHE_API ((DBG_API, Bool, "ReadUrlCacheEntryStream",
        "%#x, %d, %#x, %#x, %#x", hStream, dwLocation, lpBuffer, lpdwLen, Reserved));

    DWORD Error;

    // Map and validate handle.
    CACHE_STREAM_CONTEXT_HANDLE* pStream;
    LOCK_CACHE();
    pStream = (CACHE_STREAM_CONTEXT_HANDLE*) HandleMgr.Map(hStream);
    UNLOCK_CACHE();
    if (!pStream)
    {
        Error = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    TcpsvcsDbgAssert( pStream->SourceUrlName);

    // PERFPERF: should we remember position to avoid this call?
    if ((DWORD) -1L == SetFilePointer
        (pStream->FileHandle, dwLocation, NULL, FILE_BEGIN))
    {
        Error = GetLastError();
        goto Cleanup;
    }

    if( !ReadFile
        (pStream->FileHandle, lpBuffer, *lpdwLen, lpdwLen, NULL ) )
    {
        Error = GetLastError();
        goto Cleanup;
    }

    Error = ERROR_SUCCESS;

    LEAVE_CACHE_API();
}


URLCACHEAPI
BOOL
WINAPI
FindCloseUrlCache(
    IN HANDLE hFind
    )
/*++

Routine Description:

    This member function returns the next entry in the cache.

Arguments:

    hEnumHandle : Find First handle.

Return Value:

    Returns the find first handle. If the returned handle is NULL,
    GetLastError() returns the extended error code. It returns
    ERROR_NO_MORE_ITEMS after it returns the last entry in the cache.

--*/
{
    ENTER_CACHE_API ((DBG_API, Bool, "FindCloseUrlCache",
        "%#x", hFind));

    DWORD Error;
    
    Error = GlobalUrlContainers->FreeFindHandle(hFind);

    if (Error != ERROR_SUCCESS)
    {
        SetLastError( Error );
        DEBUG_ERROR(INET, Error);
    }
    DEBUG_LEAVE_API (Error==ERROR_SUCCESS);
    return (Error==ERROR_SUCCESS);
}


BOOL
GetCurrentSettingsVersion(LPDWORD   lpdwVer) {

    // Initialize globals
    if (!InitGlobals())
    {
        SetLastError (ERROR_INTERNET_INTERNAL_ERROR);
        return FALSE;
    }
    return GlobalUrlContainers->GetHeaderData(CACHE_HEADER_DATA_CURRENT_SETTINGS_VERSION, 
                                              lpdwVer);
}

BOOL
IncrementCurrentSettingsVersion(LPDWORD lpdwVer) {

    if (!InitGlobals())
    {
        SetLastError (ERROR_INTERNET_INTERNAL_ERROR);
        return FALSE;
    }
    return GlobalUrlContainers->IncrementHeaderData(CACHE_HEADER_DATA_CURRENT_SETTINGS_VERSION, 
                                                    lpdwVer);
}


BOOL
GetUrlCacheHeaderData(IN DWORD nIdx, OUT LPDWORD lpdwData)
{
    if (!InitGlobals())
    {
        SetLastError (ERROR_INTERNET_INTERNAL_ERROR);
        return FALSE;
    }
    return GlobalUrlContainers->GetHeaderData(nIdx, lpdwData);
}

BOOL
SetUrlCacheHeaderData(IN DWORD nIdx, IN  DWORD  dwData)
{
    if (!InitGlobals())
    {
        SetLastError (ERROR_INTERNET_INTERNAL_ERROR);
        return FALSE;
    }
    return GlobalUrlContainers->SetHeaderData(nIdx, dwData);
}

BOOL
IncrementUrlCacheHeaderData(IN DWORD nIdx, OUT LPDWORD lpdwData)
{
    if (!InitGlobals())
    {
        SetLastError (ERROR_INTERNET_INTERNAL_ERROR);
        return FALSE;
    }
    return GlobalUrlContainers->IncrementHeaderData(nIdx, lpdwData);
}


BOOL
LoadUrlCacheContent(VOID)
{
    DWORD dwError;
    if (!InitGlobals())
    {
        SetLastError(ERROR_INTERNET_INTERNAL_ERROR);
        return FALSE;
    }

    dwError = GlobalUrlContainers->LoadContent();
    if (dwError == ERROR_SUCCESS)
        return TRUE;
    SetLastError(dwError);
    return FALSE;
}


BOOL
GetUrlCacheContainerInfo(
    IN LPSTR lpszUrlName,
	OUT LPINTERNET_CACHE_CONTAINER_INFOA lpContainerInfo,
	IN OUT LPDWORD lpdwContainerInfoBufferSize,
	IN DWORD dwOptions
)
{
    DWORD dwError;

    // Initialize globals.
    if (!InitGlobals())
    {
        SetLastError(ERROR_INTERNET_INTERNAL_ERROR);
        return FALSE;
    }

    dwError = GlobalUrlContainers->GetContainerInfo(lpszUrlName,
            lpContainerInfo, lpdwContainerInfoBufferSize);

    if (dwError == ERROR_SUCCESS)
        return TRUE;
    SetLastError(dwError);
    return FALSE;
}

BOOL
UpdateUrlCacheContentPath(
    IN LPSTR lpszNewPath
)
{
    ENTER_CACHE_API ((DBG_API, Bool, "UpdateUrlCacheContentPath", "%q", lpszNewPath));
    INET_ASSERT(GlobalCacheInitialized);

    BOOL fResult = GlobalUrlContainers->SetContentPath(lpszNewPath);

    DEBUG_LEAVE_API(fResult);
    return fResult;
}



INTERNETAPI
GROUPID 
WINAPI
CreateUrlCacheGroup(
    IN DWORD  dwFlags,
    IN LPVOID lpReserved  // must pass NULL
    )
{
    ENTER_CACHE_API ((DBG_API, Bool, "CreateUrlCacheGroup", "%#x, %#x", dwFlags, lpReserved));
    GROUPID gid = 0;
    DWORD   Error; 

    // Initialize globals
    if (!InitGlobals())
    {
        Error = ERROR_INTERNET_INTERNAL_ERROR;
        goto Cleanup;
    }
    
    Error = GlobalUrlContainers->CreateGroup(dwFlags, &gid);

Cleanup:
    if( Error != ERROR_SUCCESS )
    {
        SetLastError(Error);
        DEBUG_ERROR(API, Error);
    }
    DEBUG_LEAVE_API(gid);    
    return gid;
}

BOOLAPI
DeleteUrlCacheGroup(
    IN  GROUPID GroupId,
    IN  DWORD   dwFlags,       // must pass 0
    IN  LPVOID  lpReserved    // must pass NULL
    )
{
    ENTER_CACHE_API ((DBG_API, Bool, "DeleteUrlCacheGroup", "%#x, %#x, %#x", GroupId, dwFlags, lpReserved));
    DWORD   Error;

    // Initialize globals
    if (!InitGlobals())
    {
        Error = ERROR_INTERNET_INTERNAL_ERROR;
        goto Cleanup;
    }

    Error = GlobalUrlContainers->DeleteGroup(GroupId, dwFlags);

    LEAVE_CACHE_API();
}



URLCACHEAPI
HANDLE
WINAPI
FindFirstUrlCacheGroup(
    IN      DWORD                           dwFlags,
    IN      DWORD                           dwFilter,
    IN      LPVOID                          lpSearchCondition,
    IN      DWORD                           dwSearchCondition,
    OUT     GROUPID*                        lpGroupId,
    IN OUT  LPVOID                          lpReserved 
) 
{
    ENTER_CACHE_API ((DBG_API, Handle, "FindFirstUrlCacheGroup",
        "%d, %d, %#x, %d, %#x, %#x", 
        dwFlags, dwFilter, lpSearchCondition, 
        dwSearchCondition, lpGroupId, lpReserved ));

    DWORD Error;
    HANDLE hFind = 0;

    // Validate parameters.
    if( !lpGroupId )
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
    Error = GlobalUrlContainers->FindNextGroup(&hFind, dwFlags, lpGroupId);

Cleanup:

    if( Error != ERROR_SUCCESS )
    {
        GlobalUrlContainers->FreeFindHandle(hFind);
        SetLastError(Error);
        DEBUG_ERROR(API, Error);
        hFind = NULL;
    }

    INET_ASSERT (hFind);
    DEBUG_LEAVE_API (hFind);
    return hFind;
}

URLCACHEAPI
BOOL
WINAPI
FindNextUrlCacheGroup(
    IN HANDLE                               hFind,
    OUT     GROUPID*                        lpGroupId,
    IN OUT  LPVOID                          lpReserved 
    )
{

    ENTER_CACHE_API ((DBG_API, Bool, "FindNextUrlCacheGroup",
        "%#x, %#x, %#x", hFind, lpGroupId, lpReserved ));

    DWORD Error;

    // Validate parameters.
    if( !lpGroupId )
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
    Error = GlobalUrlContainers->FindNextGroup(&hFind, 0, lpGroupId);

    LEAVE_CACHE_API();
}



BOOL
AnyFindsInProgress(DWORD ContainerID)
{
    LOCK_CACHE();
    BOOL fInProgress = HandleMgr.InUse();
    UNLOCK_CACHE();
    return fInProgress;
}


BOOL
RegisterUrlCacheNotification(
    HWND        hWnd, 
    UINT        uMsg, 
    GROUPID     gid, 
    DWORD       dwFilter, 
    DWORD       dwReserve
)
{
    DWORD Error;
    ENTER_CACHE_API ((DBG_API, Bool, "RegisterUrlCacheNotification", 
        "%#x,,%#x, %#x, %#x, %#x", hWnd, uMsg, gid, dwFilter, dwReserve));

    // Initialize globals.
    if (!InitGlobals())
    {
        Error = ERROR_INTERNET_INTERNAL_ERROR;
        goto Cleanup;
    }

    Error = GlobalUrlContainers->RegisterCacheNotify(hWnd, uMsg, gid, dwFilter);

    LEAVE_CACHE_API();
}

