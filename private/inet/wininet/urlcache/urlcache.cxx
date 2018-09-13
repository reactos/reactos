/*++

Copyright (c) 1997  Microsoft Corp.

Module Name: urlcache.cxx

Abstract:

    Urlcache API enhanced and optimized for internal use by wininet.

Author:
    Rajeev Dujari (rajeevd) 10-Apr-97

--*/

#include <cache.hxx>

DWORD
UrlCacheRetrieve
(
        IN  LPSTR                pszUrl,
        IN  BOOL                 fOffline,
        OUT HANDLE*              phStream,
        OUT CACHE_ENTRY_INFOEX** ppCEI
)
{
    BOOL fLocked = FALSE;
    HANDLE hStream = NULL;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dwErr;

    if (!InitGlobals())
    {
        INET_ASSERT(FALSE);
        return ERROR_INTERNET_INTERNAL_ERROR;
    }

    DWORD dwLookupFlags = fOffline? LOOKUP_URL_TRANSLATE
        : (LOOKUP_BIT_SPARSE | LOOKUP_URL_NOCREATE);

    DWORD cbCEI;
    
    // Find the container and search the index.
    dwErr = GlobalUrlContainers->RetrieveUrl(
                    pszUrl, 
                    (CACHE_ENTRY_INFO **) ppCEI, 
                    &cbCEI, 
                    dwLookupFlags, 
                    RETRIEVE_WITHOUT_CHECKS | RETRIEVE_WITH_ALLOCATION);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    fLocked = TRUE;
    if ((*ppCEI)->CacheEntryType & SPARSE_CACHE_ENTRY)
    {    
        *phStream = NULL;    
    }
    else
    {
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
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        // Open the file.
        hFile = CreateFile
        (
            (*ppCEI)->lpszLocalFileName,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
            NULL
        );
        if( hFile == INVALID_HANDLE_VALUE )
        {
            dwErr = GetLastError();
            goto Cleanup;
        }

        DWORD dwFileSize = GetFileSize(hFile, NULL);

        if (dwFileSize != (*ppCEI)->dwSizeLow) 
        {
            dwErr = (dwFileSize==0xFFFFFFFF) ? GetLastError() : ERROR_INVALID_DATA;
            goto Cleanup;
        }

        pStream->FileHandle = hFile;

        // Copy URL name storage.
        pStream->SourceUrlName = NewString(pszUrl);
        if( !pStream->SourceUrlName)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        *phStream = hStream;
    }
    
    dwErr = ERROR_SUCCESS;

Cleanup:

    if( dwErr != ERROR_SUCCESS )
    {
        if (*ppCEI)
        {
            FREE_MEMORY (*ppCEI);
            *ppCEI = NULL;
        }
        if (hStream)
            HandleMgr.Free (hStream);
        if (hFile)
            CloseHandle (hFile);
        if (fLocked)
            GlobalUrlContainers->UnlockUrl(pszUrl);
    }
    return dwErr;
}


void UrlCacheFlush (void)
{
    DWORD fPersist;
    
    REGISTRY_OBJ regCache (HKEY_CURRENT_USER, OLD_CACHE_KEY);
    
    if (    ERROR_SUCCESS == regCache.GetStatus()
        &&  ERROR_SUCCESS == regCache.GetValue (CACHE_PERSISTENT, &fPersist)
        &&  !fPersist
       )
    {
        FreeUrlCacheSpace (NULL, 100, STICKY_CACHE_ENTRY);
    }
}

DWORD UrlCacheCreateFile(LPCSTR szUrl, LPTSTR szExt, LPTSTR szFile, HANDLE *phfHandle)
{
    if (!InitGlobals())
        return ERROR_INTERNET_INTERNAL_ERROR;
    else
        return GlobalUrlContainers->CreateUniqueFile(szUrl, 0, szExt, szFile, phfHandle);
}

DWORD UrlCacheCommitFile(AddUrlArg* pArgs)
{
    if (!InitGlobals())
        return ERROR_INTERNET_INTERNAL_ERROR;
    else        
        return GlobalUrlContainers->AddUrl(pArgs);
}

DWORD UrlCacheSendNotification(DWORD   dwOp)
{
    DWORD Error;

    // Initialize globals.
    if (!InitGlobals())
    {
        Error = ERROR_INTERNET_INTERNAL_ERROR;
        goto Cleanup;
    }
        
    Error = GlobalUrlContainers->SendCacheNotification(dwOp);

Cleanup:
    return Error;
}

DWORD UrlCacheAddLeakFile (IN LPCSTR pszFile)
{
    if (!InitGlobals())
        return ERROR_INTERNET_INTERNAL_ERROR;
    else
        return GlobalUrlContainers->AddLeakFile (pszFile);
}


