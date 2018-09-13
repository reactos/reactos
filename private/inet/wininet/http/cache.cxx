/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    cache.cxx

Abstract:

    Contains HTTP cache-related functions. Cache functions pulled out of
    read.cxx and send.cxx

    Contents:
        HTTP_REQUEST_HANDLE_OBJECT::FCanWriteToCache
        HTTP_REQUEST_HANDLE_OBJECT::FAddIfModifiedSinceHeader
        HTTP_REQUEST_HANDLE_OBJECT::AddHeaderIfEtagFound
        HTTP_REQUEST_HANDLE_OBJECT::FHttpBeginCacheRetrieval
        HTTP_REQUEST_HANDLE_OBJECT::FHttpBeginCacheWrite
        HTTP_REQUEST_HANDLE_OBJECT::GetFromCachePreNetIO
        HTTP_REQUEST_HANDLE_OBJECT::GetFromCachePostNetIO
        HTTP_REQUEST_HANDLE_OBJECT::ResumePartialDownload
        HTTP_REQUEST_HANDLE_OBJECT::AddTimestampsFromCacheToResponseHeaders
        HTTP_REQUEST_HANDLE_OBJECT::AddTimeHeader
        HTTP_REQUEST_HANDLE_OBJECT::IsPartialResponseCacheable
        HTTP_REQUEST_HANDLE_OBJECT::LocalEndCacheWrite
        HTTP_REQUEST_HANDLE_OBJECT::GetTimeStampsForCache
        (FExcludedMimeType)
        (FilterHeaders)

Author:

    Richard L Firth (rfirth) 05-Dec-1997

Environment:

    Win32 user-mode DLL

Revision History:

    05-Dec-1997 rfirth
        Created

--*/

#include <wininetp.h>
#include <perfdiag.hxx>
#include "httpp.h"

//
// private prototypes
//

PRIVATE
BOOL
FExcludedMimeType(
    IN LPSTR lpszMimeType,
    IN DWORD dwMimeTypeSize
    );

PRIVATE
VOID
FilterHeaders(
    IN LPSTR lpszHeaderInfo,
    OUT LPDWORD lpdwHeaderLen
    );

//
// static data
//

LPCSTR rgszExcludedMimeTypes[] = {
    "multipart/mixed",
    "multipart/x-mixed-replace"
    };

const DWORD rgdwExcludedMimeTypeSizes[] = {
    sizeof("multipart/mixed") - 1,
    sizeof("multipart/x-mixed-replace") - 1
    };

static const char szDefaultExtension[] = "txt";

LPSTR rgszExcludeHeaders[] = {
    HTTP_SET_COOKIE_SZ,
    HTTP_LAST_MODIFIED_SZ,
    HTTP_SERVER_SZ,
    HTTP_DATE_SZ,
    HTTP_EXPIRES_SZ,
    HTTP_CONNECTION_SZ,
    HTTP_PROXY_CONNECTION_SZ,
    HTTP_VIA_SZ,
    HTTP_VARY_SZ,
    HTTP_AGE_SZ,
    HTTP_CACHE_CONTROL_SZ,
    HTTP_ACCEPT_RANGES_SZ,
    HTTP_CONTENT_DISPOSITION_SZ
    };

const char vszUserNameHeader[4] = "~U:";

//
// HTTP Request Handle Object methods
//


BOOL
HTTP_REQUEST_HANDLE_OBJECT::FCanWriteToCache(
    VOID
    )

/*++

Routine Description:

    Determines if we can write this file to the cache

Arguments:

    None.

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Bool,
                 "HTTP_REQUEST_HANDLE_OBJECT::FCanWriteToCache",
                 NULL
                 ));

    PERF_LOG(PE_CACHE_WRITE_CHECK_START);

    BOOL ok = FALSE;

    BOOL fForceToCache = FALSE;
    BOOL fCheckNeedFile = FALSE;

    BOOL fVary = FALSE;
    BOOL fContentEnc = FALSE;

    //
    // Set fNoCache if there is pragma: no-cache
    //

    BOOL fNoCache = FALSE;
    DWORD length, index;
    LPSTR lpszBuf;

    _ResponseHeaders.LockHeaders();

    if (GetOpenFlags() & INTERNET_FLAG_SECURE)
    {
        SetPerUserItem(TRUE);

        //
        // Determine if there are any Pragma: no-cache headers.
        //

        index = 0;

        while ( FastQueryResponseHeader(HTTP_QUERY_PRAGMA,
                                        (LPVOID *) &lpszBuf,
                                        &length,
                                        index) == ERROR_SUCCESS )
        {
            if (length == NO_CACHE_LEN &&
                strnicmp(NO_CACHE_SZ, lpszBuf, NO_CACHE_LEN) == 0)
            {
                fNoCache = TRUE;
                break;
            }

            index++;
        }

        if (fNoCache)
        {
            // If server disabled caching over SSL, don't even create
            // a file, let alone commit it to the cache.  Game over.
            BETA_LOG (DOWNLOAD_NO_FILE);
            goto quit;
        }
    
        //
        // If we've disabled caching for SSL servers, consider creating
        // a download file if the client insists upon it.
        //

        if (GlobalDisableSslCaching)
        {
            goto check_need_file;
        }
    }

    //
    // Also set fNoCache if there is Cache-Control: no-cache or no-store header,
    // if there is a Cache-Control: private header and we're *not* on NT with user profiles,
    // or any Vary: headers. These are only checked for HTTP 1.1 servers.
    //

    if (IsResponseHttp1_1())
    {
        CHAR *ptr, *pToken;
        index = 0;

        // Scan for Cache-Control header.
        while (FastQueryResponseHeader(HTTP_QUERY_CACHE_CONTROL,
                                    (LPVOID *) &lpszBuf,
                                    &length,
                                    index) == ERROR_SUCCESS)
        {
            // Check for no-cache or no-store or private.
            CHAR chTemp = lpszBuf[length];

            lpszBuf[length] = '\0';
            pToken = ptr = lpszBuf;
            // Parse a token from the string; test for sub headers.
            while (*pToken != '\0')
            {
                SKIPWS(pToken);

                // no-cache, no-store.
                if (strnicmp(NO_CACHE_SZ, pToken, NO_CACHE_LEN) == 0)
                {
                    fNoCache = TRUE;
                    break;
                }
                if( strnicmp(NO_STORE_SZ, pToken, NO_STORE_LEN) == 0) 
                {
                    fNoCache = TRUE;
                    fCheckNeedFile = TRUE;
                }

                // private.
                if (strnicmp(PRIVATE_SZ, pToken, PRIVATE_LEN) == 0)
                {
                    SetPerUserItem(TRUE);
                }

                while (*pToken != '\0')
                {
                    if ( *pToken == ',')
                    {
                        pToken++;
                        break;
                    }

                    pToken++;
                }

            } // while (*pToken != '\0')

            //
            // We've finished parsing it, now return our terminator back to its proper place
            //

            lpszBuf[length] = chTemp;

            // If fNoCache, we're done. Break out of switch.
            if (fNoCache)
                break;

            index++;

        } // while FastQueryResponseHeader == ERROR_SUCCESS

        if (fNoCache)
        {
            if( fCheckNeedFile ) 
            {
                // cache-control: no store
                goto check_need_file;
            }
            else 
            if (GetOpenFlags() & INTERNET_FLAG_SECURE)
            {
                // If server disabled caching over SSL, don't even create
                // a file, let alone commit it to the cache.  Game over.
                BETA_LOG (DOWNLOAD_NO_FILE);
                goto quit;
            }
            else
            {
                //
                // This is not SSL/PCT, so don't cache but consider creating a
                // download file if one was requested.
                //

                goto check_need_file;
            }
        }

        // Finally, check if any Vary: headers exist, EXCEPT "Vary: User-Agent"

        index = 0;
        if (FastQueryResponseHeader(HTTP_QUERY_VARY,
                                    (LPVOID *) &lpszBuf,
                                    &length,
                                    index) == ERROR_SUCCESS
            && !(length == USER_AGENT_LEN
               && !strnicmp (lpszBuf, USER_AGENT_SZ, length)) )
        {
            // content-encoding? 
            if (FastQueryResponseHeader(HTTP_QUERY_CONTENT_ENCODING,
                                    (LPVOID *) &lpszBuf,
                                    &length,
                                    index) == ERROR_SUCCESS ) 
            {
                fContentEnc = TRUE;
            }
            fVary = TRUE;
            goto check_need_file;
        }
    }

    if (GetCacheFlags() & INTERNET_FLAG_NO_CACHE_WRITE)
    {
        goto check_need_file;
    }

    //
    // accept HTTP/1.0 or downlevel server responses
    //

    if ((GetStatusCode() == HTTP_STATUS_OK) || (GetStatusCode() == 0))
    {
        if (FastQueryResponseHeader(HTTP_QUERY_CONTENT_TYPE, (LPVOID *) &lpszBuf, &length, 0) == ERROR_SUCCESS)
        {
            if (FExcludedMimeType(lpszBuf, length))
            {

                DEBUG_PRINT(CACHE,
                            INFO,
                            ("%s Mime Excluded from caching\n",
                            lpszBuf
                            ));

                goto check_need_file;
            }
        }

        //
        // BUGBUG should we also check for size and keep an upper bound?
        //

        ok = TRUE;
        goto quit;

    }
    else
    {
        INTERNET_SCHEME schemeType = GetSchemeType();

        if ((schemeType == INTERNET_SCHEME_FTP)
        || (schemeType == INTERNET_SCHEME_GOPHER))
        {
            ok = TRUE;
            goto quit;
        }
    }

check_need_file:

    INET_ASSERT (!ok);

    if (GetCacheFlags() & INTERNET_FLAG_NEED_FILE
        || GetCacheFlags () & INTERNET_FLAG_HYPERLINK)
    {
        //
        // Create a download file but don't commit it to the cache.
        //

        BETA_LOG (DOWNLOAD_FILE_NEEDED);

        fForceToCache = !ok;
        ok = TRUE;
        if( !fContentEnc )
        {
            // if content-enc, the client will write to cache...
            // so we should not delete the data file
            SetDeleteDataFile();
        }

        DEBUG_PRINT(CACHE, INFO, ("don't commit file\n"));
    }
    else
    {
        BETA_LOG (DOWNLOAD_FILE_NOT_NEEDED);
    }

quit:

    if ((!ok || fForceToCache) && !fVary)
        SetCacheWriteDisabled();

    _ResponseHeaders.UnlockHeaders();

    PERF_LOG(PE_CACHE_WRITE_CHECK_END);

    DEBUG_LEAVE(ok);

    return ok;
}


BOOL
HTTP_REQUEST_HANDLE_OBJECT::FAddIfModifiedSinceHeader(
    IN LPCACHE_ENTRY_INFO lpCEI
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    lpCEI   -

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Bool,
                 "HTTP_REQUEST_HANDLE_OBJECT::FAddIfModifiedSinceHeader",
                 "%#x",
                 lpCEI
                 ));

    PERF_ENTER(FAddIfModifiedSinceHeader);

    char buff[64], buffh[256];
    DWORD dwLen, error;
    BOOL success = FALSE;

    INET_ASSERT (FT2LL(lpCEI->LastModifiedTime));

    dwLen = sizeof(buff);

    if (FFileTimetoHttpDateTime(&(lpCEI->LastModifiedTime), buff, &dwLen))
    {
        LPSTR pszBuf;

        if (lpCEI->CacheEntryType & HTTP_1_1_CACHE_ENTRY)
        {
            INET_ASSERT (dwLen);
            pszBuf = buff;
        }
        else
        {
            dwLen = wsprintf(buffh, "%s; length=%d", buff, lpCEI->dwSizeLow);
            pszBuf = buffh;
        }

        DEBUG_PRINT(CACHE,
                    INFO,
                    ("%s %s - empty\n",
                    GlobalKnownHeaders[HTTP_QUERY_IF_MODIFIED_SINCE].Text,
                    buffh
                    ));

        error = ReplaceRequestHeader(HTTP_QUERY_IF_MODIFIED_SINCE,
                                     pszBuf,
                                     dwLen,
                                     0,                 // no index
                                     ADD_HEADER_IF_NEW
                                     );
        if ((error == ERROR_SUCCESS) || (error == ERROR_HTTP_HEADER_ALREADY_EXISTS))
        {
            if (error == ERROR_SUCCESS)
            {
                // we added the header on behalf of the app. This is equivalent
                // to the app having specified INTERNET_FLAG_RESYNCHRONIZE

                SetAutoSync();
            }
            success = TRUE;
        }
    }

    PERF_LEAVE(FAddIfModifiedSinceHeader);
    DEBUG_LEAVE(success);
    return success;
}


BOOL
HTTP_REQUEST_HANDLE_OBJECT::AddHeaderIfEtagFound(
    IN LPCACHE_ENTRY_INFO lpCEI
    )

/*++

Routine Description:

    Adds if-modified-since header if an etag was present in the cache entry headers.

Arguments:

    lpCEI   -

Return Value:

    BOOL

--*/

{
    if (!(lpCEI->CacheEntryType & HTTP_1_1_CACHE_ENTRY))
        return TRUE;

    // BUGBUG - always parsing, use flag.
    CHAR buf[512];

    DWORD nIndex = 0, cbBuf = 512, dwError = ERROR_SUCCESS;

    HTTP_HEADER_PARSER hp((CHAR*) lpCEI->lpHeaderInfo, lpCEI->dwHeaderInfoSize);

    if (hp.FindHeader((CHAR*) lpCEI->lpHeaderInfo, HTTP_QUERY_ETAG,
            0, buf, &cbBuf, &nIndex) == ERROR_SUCCESS)
    {
        DWORD dwQueryIndex;

        if (lpCEI->CacheEntryType & SPARSE_CACHE_ENTRY)
        {
            dwQueryIndex = HTTP_QUERY_IF_RANGE;
        }
        else
        {
            dwQueryIndex = HTTP_QUERY_IF_NONE_MATCH;
        }

        dwError = ReplaceRequestHeader(dwQueryIndex,
                                       buf,
                                       cbBuf,
                                       0,
                                       ADD_HEADER_IF_NEW
                                       );
    }

    if ((dwError == ERROR_SUCCESS) || (dwError == ERROR_HTTP_HEADER_ALREADY_EXISTS))
        return TRUE;

    return FALSE;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::FHttpBeginCacheRetrieval(
    IN BOOL bReset,
    IN BOOL bOffline,
    IN BOOL bNoRetrieveIfExist
    )

/*++

Routine Description:

    Starts retrieving data for this object from the cache

Arguments:

    bReset  - if TRUE, forcefully reset the (keep-alive) connection

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_REQUEST_HANDLE_OBJECT::FHttpBeginCacheRetrieval",
                 "%B, %B",
                 bReset,
                 bOffline
                 ));

    PERF_LOG(PE_CACHE_RETRIEVE_START);

    DWORD error;
    LPSTR lpHeaders = NULL;

    if (bOffline)
    {
        if (!IsOffline() && IsCacheReadDisabled())
        {
            error = ERROR_FILE_NOT_FOUND;
            goto quit;
        }

        if( !bNoRetrieveIfExist )
            error = UrlCacheRetrieve (TRUE);
        else
            error = ERROR_SUCCESS;

        if (error != ERROR_SUCCESS)
            goto quit2;
    }

    INET_ASSERT(_hCacheStream && _pCacheEntryInfo);

    //
    // RecordCacheRetrieval() will set end-of-file if it succeeds
    //

    error = RecordCacheRetrieval (_pCacheEntryInfo);
    if (error != ERROR_SUCCESS)
    {
        UrlCacheUnlock();
        goto quit2;
    }

    if (bOffline && lstrcmp(_pCacheEntryInfo->lpszSourceUrlName, GetCacheKey()))
    {
        // Simulate a redirect to the client.
        InternetIndicateStatusString
            (INTERNET_STATUS_REDIRECT, _pCacheEntryInfo->lpszSourceUrlName);

        // The cache translated through a redirect.
        FreeSecondaryCacheKey (); // POST redirects must be to GET
        SetURL (_pCacheEntryInfo->lpszSourceUrlName);
    }

    if (bOffline &&
        (_pCacheEntryInfo->CacheEntryType & MUST_REVALIDATE_CACHE_ENTRY))
    {
        INET_ASSERT (_pCacheEntryInfo->CacheEntryType & HTTP_1_1_CACHE_ENTRY);
        
        // Offline mode. Check for a Cache-Control: must-revalidate header.
        // If so, allow cache data to be retrieved only if not expired.
        FILETIME ftCurrentTime;
        GetCurrentGmtTime(&ftCurrentTime);

        LONGLONG qwExpire = FT2LL(_pCacheEntryInfo->ExpireTime);
        LONGLONG qwCurrent = FT2LL(ftCurrentTime);

        if ((qwCurrent > qwExpire) && !(GetCacheFlags() & INTERNET_FLAG_FWD_BACK))
        {
            error = ERROR_FILE_NOT_FOUND;
            goto quit;
        }
        else
        {
            goto check_if_modified_since;
        }
    }

check_if_modified_since:

    if (!IsOffline() && IsCacheReadDisabled())
    {
        //
        // We are in offline mode.  (Really should assert this.)
        // Allow cache data to be retrieved only if last-modified time is
        // STRICTLY greater than if-modified-since time added by client.
        //

        LONGLONG qwLastModified = FT2LL(_pCacheEntryInfo->LastModifiedTime);

        LONGLONG qwIfModifiedSince;
        SYSTEMTIME stIfModifiedSince;
        DWORD length = sizeof(stIfModifiedSince);
        DWORD index = 0;

        _RequestHeaders.LockHeaders();

        error = QueryRequestHeader(HTTP_QUERY_IF_MODIFIED_SINCE,
                                   &stIfModifiedSince,
                                   &length,
                                   HTTP_QUERY_FLAG_SYSTEMTIME,
                                   &index
                                   );

        _RequestHeaders.UnlockHeaders();

        if (error != ERROR_SUCCESS)
        {
            // It's theoretically possible the client was redirected,
            // removed i-m-s on redirect callback, then went offline.
            error = ERROR_FILE_NOT_FOUND;
            goto quit;
        }

        if (!SystemTimeToFileTime(&stIfModifiedSince,
                                  (FILETIME*)&qwIfModifiedSince))
        {
            error = ERROR_FILE_NOT_FOUND;
            goto quit;
        }

        if (qwLastModified <= qwIfModifiedSince)
        {
            error = ERROR_FILE_NOT_FOUND;
            goto quit;
        }
    }

    // allocate buffer for headers

    lpHeaders = (LPSTR)ALLOCATE_FIXED_MEMORY(_pCacheEntryInfo->dwHeaderInfoSize);
    if (!lpHeaders)
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    memcpy(lpHeaders, _pCacheEntryInfo->lpHeaderInfo, _pCacheEntryInfo->dwHeaderInfoSize);

    // we are hijacking the HTTP request object. If there is already
    // a pending network operation (we started a network transaction
    // but realised that we have the same data in cache (i.e. the
    // data has not expired since we last wrote it to cache)) then
    // we need to kill it

    if (bReset)
    {   
        // if we have a keep-alive connection AND the server returned
        // 304 then the parameter can be FALSE so that we don't kill
        // the connection, just release it
        //
        // ResetObject() will reset end-of-file if it succeeds
        //
        // We DO NOT clear out the request headers so that the app can still
        // query them if necessary

//dprintf(">>> resetting %s\n", GetURL());
        error = ResetObject(!(IsKeepAlive() && (GetBytesInSocket() == 0)), FALSE);
        if (error != ERROR_SUCCESS)
            goto quit;
    }

    error = CreateResponseHeaders(&lpHeaders, _pCacheEntryInfo->dwHeaderInfoSize);
    if (error != ERROR_SUCCESS)
        goto quit;

    error = AddTimestampsFromCacheToResponseHeaders(_pCacheEntryInfo);
    if (error != ERROR_SUCCESS) 
        goto quit;

    // we have to set end-of-file again: RecordCacheRetrieval() set
    // it, then we wiped it out in ResetObject()
    // FYI: We set end-of-file because we have all the data in the
    // stream available locally; it doesn't mean that our virtual
    // stream pointer is positioned at the end of the file

    DEBUG_PRINT(CACHE, INFO, ("Found in the cache\n"));

    SetState(HttpRequestStateObjectData);
    SetFromCache();
    SetEndOfFile();
    SetHaveContentLength(TRUE);
    _VirtualCacheFileSize =
        _RealCacheFileSize =
            _BytesRemaining =
                _ContentLength = _pCacheEntryInfo->dwSizeLow;

    INET_ASSERT (error == ERROR_SUCCESS);

quit:
    if (error != ERROR_SUCCESS)
        EndCacheRetrieval();
    if (lpHeaders)
        FREE_MEMORY(lpHeaders);

quit2:

    PERF_LOG(PE_CACHE_RETRIEVE_END);
    DEBUG_LEAVE(error);
    return error;
}


BOOL
HTTP_REQUEST_HANDLE_OBJECT::FHttpBeginCacheWrite(
    VOID
    )

/*++

Routine Description:

    Preps the cache for writing. Sets up the cache filename with an extension
    based on the MIME type and optionally writes the headers to the cache data
    stream

Arguments:

    None.

Return Value:

    BOOL
        TRUE    - Success

        FALSE   - Failure

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Bool,
                 "HTTP_REQUEST_HANDLE_OBJECT::FHttpBeginCacheWrite",
                 NULL
                 ));

    PERF_ENTER(FHttpBeginCacheWrite);

    INET_ASSERT(!IsCacheReadInProgress());
    INET_ASSERT(!IsCacheWriteInProgress());

    PERF_TRACE(FHttpBeginCacheWrite, 1);

    _ResponseHeaders.LockHeaders();

    LPSTR lpszFileExtension = NULL;
    BOOL fIsUncertainMime = FALSE;
    char cExt[DEFAULT_MAX_EXTENSION_LENGTH + 1];
    char buf[256];

    DWORD dwLen, cbFileName, dwIndex;
    CHAR *ptr, *pToken, *pszFileName = NULL;
    CHAR szFileName[MAX_PATH];
    LPSTR lpszBuf;

    // Create a temp local cache file.

    // If we find a Content-Disposition header, parse out any filename and use it.
    if (QueryResponseHeader (HTTP_QUERY_CONTENT_DISPOSITION,
        buf, &(dwLen = sizeof(buf)), 0, &(dwIndex = 0) ) == ERROR_SUCCESS)
    {
        // Could have multiple tokens in it. Scan for the "filename" token.
        ptr = pToken = buf;
        while (ptr = StrTokEx(&pToken, ";"))
        {
            // Skip any leading ws in token.
            SKIPWS(ptr);

            // Compare against "filename".
            if (!strnicmp(ptr, FILENAME_SZ, FILENAME_LEN))
            {
                // Found it.
                ptr += FILENAME_LEN;

                // Skip ws before '='.
                SKIPWS(ptr);

                // Must have '='
                if (*ptr == '=')
                {
                    // Skip any ws after '=' and point
                    // to beginning of the file name
                    ptr++;
                    SKIPWS(ptr);

                    // Skip past any quotes
                    if (*ptr == '\"')
                        ptr++;

                    SKIPWS(ptr);

                    cbFileName = strlen(ptr);

                    if (cbFileName)
                    {
                        // Ignore any trailing quote.
                        if (ptr[cbFileName-1] == '\"')
                            cbFileName--;
                            
                        memcpy(szFileName, ptr, cbFileName);
                        szFileName[cbFileName] = '\0';
                        pszFileName = szFileName;
                    }
                }
                break;
            }
        }
    }

    // Either no Content-disposition header or filename not parsed.
    if (!pszFileName)
    {
        DWORD dwMimeLen;
        if (FastQueryResponseHeader(HTTP_QUERY_CONTENT_ENCODING, (LPVOID *) &lpszBuf, &dwMimeLen, 0) == ERROR_SUCCESS)
        {
            // if there is content encoding, we should not use
            // content-type for file extension
            lpszFileExtension = NULL;
        }

        else if (FastQueryResponseHeader(HTTP_QUERY_CONTENT_TYPE, (LPVOID *) &lpszBuf, &dwMimeLen, 0) == ERROR_SUCCESS)
        {
            dwLen = sizeof(cExt);
            fIsUncertainMime = strnicmp(lpszBuf, "text/plain", dwMimeLen)==0;

            PERF_TRACE(FHttpBeginCacheWrite, 2);

            if (!fIsUncertainMime &&
                GetFileExtensionFromMimeType(lpszBuf, dwMimeLen, cExt, &dwLen))
            {

                //
                // get past the '.' because the cache expects it that way
                //

                lpszFileExtension = &cExt[1];
            }
        }

        //
        // if we couldn't get the MIME type or failed to map it then try to get
        // the file extension from the object name requested
        //

        if (lpszFileExtension == NULL)
        {
            dwLen = sizeof(cExt);

            PERF_TRACE(FHttpBeginCacheWrite, 3);

            lpszFileExtension = GetFileExtensionFromUrl(GetURL(), &dwLen);
            if (lpszFileExtension != NULL)
            {
                memcpy(cExt, lpszFileExtension, dwLen);
                cExt[dwLen] = '\0';
                lpszFileExtension = cExt;
            }

            PERF_TRACE(FHttpBeginCacheWrite, 4);

        }

        if ((lpszFileExtension == NULL) && fIsUncertainMime)
        {

            INET_ASSERT(sizeof(szDefaultExtension) < DEFAULT_MAX_EXTENSION_LENGTH);

            strcpy(cExt, szDefaultExtension);
            lpszFileExtension = cExt;
        }
    }

    //
    // BUGBUG - BeginCacheWrite() wants the estimated file size, but we just
    //          give it 0 for now
    //

    //dwError = pRequest->BeginCacheWrite(dwLen, lpszFileExtension);

    PERF_TRACE(FHttpBeginCacheWrite, 5);

    DWORD dwError = BeginCacheWrite(0, lpszFileExtension, pszFileName);

    PERF_TRACE(FHttpBeginCacheWrite, 6);

    _ResponseHeaders.UnlockHeaders();

    PERF_LEAVE(FHttpBeginCacheWrite);

    BOOL success = (dwError == ERROR_SUCCESS);

    DEBUG_LEAVE(success);

    return success;
}


/*============================================================================
IsExpired (...)

4/17/00 (RajeevD) Corrected back arrow behavior and wrote detailed comment.
 
We have a cache entry for the URL.  This routine determines whether we should
synchronize, i.e. do an if-modified-since request.  This answer depends on 3
factors: navigation mode, expiry on cache entry if any, and syncmode setting.

1. There are two navigation modes:
a. hyperlinking - clicking on a link, typing a URL, starting browser etc.
b. back/forward - using the back or forward buttons in the browser.

In b/f case we generally want to display what was previously shown.  Ideally
wininet would cache multiple versions of a given URL and trident would specify
which one to use when hitting back arrow.  For now, the best we can do is use
the latest (only) cache entry or resync with the server.

EXCEPTION: if the cache entry sets http/1.1 cache-control: must-revalidate,
we treat as if we were always hyperlinking to the cache entry.  This is 
normally used during offline mode to suppress using a cache entry after
expiry.  This overloaded usage gives sites a workaround if they dislike our
new back button behavior.

2. Expiry may fall into one of 3 buckets:
a. no expiry information
b. expiry in past of current time (hyperlink) or last-access time (back/fwd)
c. expiry in future of current time (hyperlink) or-last access time (back/fwd)

3. Syncmode may have 3 settings
a. always - err on side of freshest data at expense of net perf.
b. never - err on side of best net perf at expense of stale data.
c. once-per-session - middle-of-the-road setting
d. automatic - slight variation of once-per-session where we decay frequency
of i-m-s for images that appear to be static.  This is the default.

Based on these factors, there are 5 possible result values in matrices below:
1   synchronize
0   don't synchronize
?   synchronize if last-sync time was before start of the current session, 
?-  Like per-session except if URL is marked static and has a delay interval.
0+  Don't sync if URL is marked static, else fall back to per-session


HYPERLINKING

When hyperlinking, expiry takes precedence, then we look at syncmode.

                No Expiry       Expiry in Future    Expiry in Past
Syncmode                        of Current Time     of Current Time
                               
   Always           1                   0                   1  


   Never            0                   0                   1


   Per-Session      ?                   0                   1


   Automatic        ?-                  0                   1

   
BACK/FORWARD

When going back or forward, we generally don't sync.  The exception is if
we should have sync'ed the URL on the previous navigate but didn't.  We
deduce this by looking at the last-sync time of the entry.


                No Expiry       Expiry in Future    Expiry in Past
Syncmode                        of Last-Access Time of Last-Access Time
    
   Always           ?                   0                   ?


   Never            0                   0                   ?


   Per-Session      ?                   0                   ?


   Automatic        0+                  0                   ?


When considering what might have happened when hyperlinking to this URL,
the decision tree has 5 outcomes:
1. We might have had no cache entry and downloaded to cache for the first time
2. Else we might have had a cache entry and used it w/o i-m-s
3. Else we did i-m-s but the download was aborted
4. Or the i-m-s returned not modified
5. Or the i-m-s returned new content
Only in case 3 do we want to resync the cache entry.

============================================================================*/

BOOL IsExpired (
        CACHE_ENTRY_INFOEX* pInfo, 
        DWORD dwCacheFlags, 
        BOOL* pfLazyUpdate )
{
    BOOL fExpired;
    FILETIME ftCurrentTime;
    GetCurrentGmtTime (&ftCurrentTime);

    if ((dwCacheFlags & INTERNET_FLAG_FWD_BACK)
        && !(pInfo->CacheEntryType & MUST_REVALIDATE_CACHE_ENTRY))
    {
        // BACK/FORWARD CASE

        if (FT2LL (pInfo->ExpireTime) != LONGLONG_ZERO)
        {
            // We have an expires time.
            if (FT2LL (pInfo->ExpireTime) > FT2LL(pInfo->LastAccessTime))
            {
                // Expiry was in future of last access time, so don't resync.
                fExpired = FALSE;
            }                
            else
            {
                // Entry was originally expired.  Make sure it was sync'ed once.
                fExpired = (FT2LL(pInfo->LastSyncTime) < dwdwSessionStartTime);
            }
        }
        else switch (GlobalUrlCacheSyncMode)
        {
            default:
            case WININET_SYNC_MODE_AUTOMATIC:
                if (pInfo->CacheEntryType & STATIC_CACHE_ENTRY)
                {
                    fExpired = FALSE;
                    break;
                }
            // else intentional fall-through...
        
            case WININET_SYNC_MODE_ALWAYS:
            case WININET_SYNC_MODE_ONCE_PER_SESSION:
                fExpired = (FT2LL(pInfo->LastSyncTime) < dwdwSessionStartTime);
                break;

            case WININET_SYNC_MODE_NEVER:
                fExpired = FALSE;
                break;
                
        } // end switch
    }
    else
    {
        // HYPERLINKING CASE

        // Always strictly honor expire time from the server.
        INET_ASSERT(pfLazyUpdate);
        *pfLazyUpdate = FALSE;
        
        if(   (pInfo->CacheEntryType & POST_CHECK_CACHE_ENTRY ) &&
             !(dwCacheFlags & INTERNET_FLAG_BGUPDATE) )
        {
            //
            // this is the (instlled) post check cache entry, so we will do
            // post check on this ietm
            //
            fExpired = FALSE;
            *pfLazyUpdate = TRUE;
            
        }
        else if (FT2LL(pInfo->ExpireTime) != LONGLONG_ZERO)
        {
            // do we have postCheck time?
            //
            //           ftPostCheck                   ftExpire
            //               |                            |
            // --------------|----------------------------|-----------> time
            //               |                            | 
            //   not expired |   not expired (bg update)  |   expired
            //
            //               
            LONGLONG qwPostCheck = FT2LL(pInfo->ftPostCheck);
            if( qwPostCheck != LONGLONG_ZERO )
            {
                LONGLONG qwCurrent = FT2LL(ftCurrentTime);

                if( qwCurrent < qwPostCheck )
                {
                    fExpired = FALSE;
                }
                else
                if( qwCurrent < FT2LL(pInfo->ExpireTime) ) 
                {
                    fExpired = FALSE;

                    // set background update flag  
                    // (only if we are not doing lazy updating ourselfs)
                    if ( !(dwCacheFlags & INTERNET_FLAG_BGUPDATE) )
                    {
                        *pfLazyUpdate = TRUE;
                    }
                }
                else
                {
                    fExpired = TRUE;
                }
            }
            else 
                fExpired = FT2LL(pInfo->ExpireTime) <= FT2LL(ftCurrentTime);
        }
        else switch (GlobalUrlCacheSyncMode)
        {

            case WININET_SYNC_MODE_NEVER:
                // Never check, unless the page has expired
                fExpired = FALSE;
                break;

            case WININET_SYNC_MODE_ALWAYS:
                fExpired = TRUE;
                break;

            default:
            case WININET_SYNC_MODE_AUTOMATIC:

                if (pInfo->CacheEntryType & STATIC_CACHE_ENTRY)
                {
                    // We believe this entry never actually changes.
                    // Check the entry if interval since last checked
                    // is less than 25% of the time we had it cached.
                    LONGLONG qwTimeSinceLastCheck = FT2LL (ftCurrentTime)
                        - FT2LL(pInfo->LastSyncTime);
                    LONGLONG qwTimeSinceDownload = FT2LL (ftCurrentTime)
                        - FT2LL (pInfo->ftDownload);
                    fExpired = qwTimeSinceLastCheck > qwTimeSinceDownload/4;
                    break;
                }
                // else intentional fall through to once-per-session rules.

            case WININET_SYNC_MODE_ONCE_PER_SESSION:

                fExpired = TRUE;

                // Huh. We don't have an expires, so we'll improvise
                // but wait! if we are hyperlinking then there is added
                // complication. This semantic has been figured out
                // on Netscape after studying various sites
                // if the server didn't send us expiry time or lastmodifiedtime
                // then this entry expires when hyperlinking
                // this happens on queries

                if (dwCacheFlags & INTERNET_FLAG_HYPERLINK
                    && !FT2LL(pInfo->LastModifiedTime))
                {
                    // shouldn't need the hyperlink test anymore
                    DEBUG_PRINT(UTIL, INFO, ("Hyperlink semantics\n"));
                    INET_ASSERT(fExpired==TRUE);
                    break;
                }

                // We'll assume the data could change within a day of the last time
                // we sync'ed.
                // We want to refresh UNLESS we've seen the page this session
                // AND the session's upper bound hasn't been exceeded.
                if      ((dwdwSessionStartTime < FT2LL(pInfo->LastSyncTime))
                    &&
                        (FT2LL(ftCurrentTime) < FT2LL(pInfo->LastSyncTime) + 
                            dwdwHttpDefaultExpiryDelta))
                {                    
                    fExpired = FALSE;
                }            
                break;

        } // end switch
        
    } // end else for hyperlinking case

#ifdef DBG
    char buff[64];
    PrintFileTimeInInternetFormat(&ftCurrentTime, buff, sizeof(buff));
    DEBUG_PRINT(UTIL, INFO, ("Current Time: %s\n", buff));
    PrintFileTimeInInternetFormat(&(pInfo->ExpireTime), buff, sizeof(buff));
    DEBUG_PRINT(UTIL, INFO, ("Expiry Time: %s\n", buff));
    PrintFileTimeInInternetFormat(&(pInfo->ftPostCheck), buff, sizeof(buff));
    DEBUG_PRINT(UTIL, INFO, ("PostCheck Time: %s\n", buff));
    PrintFileTimeInInternetFormat(&(pInfo->LastSyncTime), buff, sizeof(buff));
    DEBUG_PRINT(UTIL, INFO, ("Last Sync Time: %s\n", buff));
    PrintFileTimeInInternetFormat(&(pInfo->ftDownload), buff, sizeof(buff));
    DEBUG_PRINT(UTIL, INFO, ("Last Download Time: %s\n", buff));
    PrintFileTimeInInternetFormat((FILETIME *)&dwdwSessionStartTime, buff, sizeof(buff));
    DEBUG_PRINT(UTIL, INFO, ("Session start Time: %s\n", buff));
    DEBUG_PRINT(UTIL, INFO, ("CacheFlags=%x\n", dwCacheFlags));
    DEBUG_PRINT(HTTP, INFO,
        ("CheckExpired: Url %s Expired \n", (fExpired ? "" : "Not")));
#endif //DBG
        
    return fExpired;
}



DWORD
HTTP_REQUEST_HANDLE_OBJECT::GetFromCachePreNetIO(
    VOID
    )

/*++

Routine Description:

    Check if in the cache. If so, check for expired, if expired do if-modified
    -since, else get from the cache.

Arguments:

    None.

Return Value:

    DWORD   Windows Error Code
        ERROR_SUCCESS: started retrieval from the cache
        ERROR_FILE_NOT_FOUND: go to the wire

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_REQUEST_HANDLE_OBJECT::GetFromCachePreNetIO",
                 NULL
                 ));

    PERF_LOG(PE_CACHE_EXPIRY_CHECK_START);

    DWORD error = ERROR_FILE_NOT_FOUND;

    if (_hCacheStream || IsCacheReadDisabled())
    {
        INET_ASSERT (error == ERROR_FILE_NOT_FOUND);
        goto done;
    }

    PERF_LOG(PE_TRACE, 0x91);

    //
    // See if we have a cache entry and if so lock it.
    //

    // Check for the cdrom insertion case Set error to
    // ERROR_INTERNET_INSERT_CDROM ONLY only in this
    // case, since this will be handled. Otherwise,
    // error defaults to ERROR_FILE_NOT_FOUND.
    DWORD dwError;
    dwError = UrlCacheRetrieve(FALSE);
    if (dwError != ERROR_SUCCESS)
    {
        if (dwError == ERROR_INTERNET_INSERT_CDROM)
            error = dwError;
        else
            BETA_LOG (CACHE_MISS);
        goto done;
    }

    if (_pCacheEntryInfo->CacheEntryType & SPARSE_CACHE_ENTRY)
    {

        //
        // Open the file so it can't be deleted.
        //

        _CacheFileHandle =
            CreateFile
            (
                _pCacheEntryInfo->lpszLocalFileName,
                GENERIC_WRITE,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
            );

        //
        // Check the file size.
        //

        if (_CacheFileHandle == INVALID_HANDLE_VALUE
            || (_pCacheEntryInfo->dwSizeLow !=
               SetFilePointer (_CacheFileHandle, 0, NULL, FILE_END)))
        {

            FREE_MEMORY (_pCacheEntryInfo);
            _pCacheEntryInfo = NULL;
            INET_ASSERT (error == ERROR_FILE_NOT_FOUND);
            goto done;
        }

        // We have a partial cache entry for this URL.  No need to check
        // for expiry or correct user.  Add a Range: header to get the
        // rest of the data.

        char szBuf[64];
        DWORD cbBuf =
            wsprintf (szBuf, "bytes=%d-", _pCacheEntryInfo->dwSizeLow);
        if (ERROR_SUCCESS != ReplaceRequestHeader
            (HTTP_QUERY_RANGE, szBuf, cbBuf, 0, ADD_HEADER))
        {
            goto done;
        }

        //
        // If there was Last-Modified-Time, add Unless-Modified-Since header,
        // which assures coherency in the event the URL data changed since
        // the partial download.
        //
        if (FT2LL(_pCacheEntryInfo->LastModifiedTime) != LONGLONG_ZERO)
        {
            cbBuf = sizeof(szBuf);
            FFileTimetoHttpDateTime(&(_pCacheEntryInfo->LastModifiedTime),
                szBuf, &cbBuf);

            ReplaceRequestHeader (
                HTTP_QUERY_UNLESS_MODIFIED_SINCE,
                szBuf, cbBuf, 0, ADD_HEADER
            );
        }

        //
        // Similarly, if the entry has a 1.1 ETag, add the If-Range header.
        //

        AddHeaderIfEtagFound(_pCacheEntryInfo);

    }
    else
    {
        PERF_LOG(PE_TRACE, 0x93);

        BOOL fIsExpired = IsExpired 
            (_pCacheEntryInfo, GetCacheFlags(), &_fLazyUpdate);


        PERF_LOG(PE_TRACE, 0x94);

        //
        // found the cache entry
        // if it is expired, or we are asked to do if-modified-since
        // then we do it. However, if the entry is an Installed entry,
        // we skip netio unless the entry is also marked EDITED_CACHE_ENTRY
        // and the client has specifically asked for
        // INTERNET_OPTION_BYPASS_EDITED_ENTRY.

        if ((!(_pCacheEntryInfo->CacheEntryType & INSTALLED_CACHE_ENTRY)
            || ((_pCacheEntryInfo->CacheEntryType & EDITED_CACHE_ENTRY)
            && GlobalBypassEditedEntry))
             && (fIsExpired || (GetCacheFlags() & INTERNET_FLAG_RESYNCHRONIZE)
             ))
        {

            //
            // add if-modified-since only if there is lastmodifiedtime
            // sent back by the site. This way you never get into trouble
            // where the sitedoesn't send you an lastmodtime and you
            // send if-modified-since based on a clock which might be ahead
            // of the site. So the site might say nothing is modified even though
            // something might be. www.microsoft.com is one such example
            //
            if (FT2LL(_pCacheEntryInfo->LastModifiedTime) != LONGLONG_ZERO)
            {
                DEBUG_PRINT(HTTP,
                            INFO,
                            ("%s expired, doing IF-MODIFIED-SINCE\n",
                            GetURL()
                            ));

                PERF_LOG(PE_TRACE, 0x97);
                FAddIfModifiedSinceHeader(_pCacheEntryInfo);
                PERF_LOG(PE_TRACE, 0x98);
            }

            // If this is an HTTP 1.1 then check to see if an Etag:
            // header is present and add an If-None-Match header.
            AddHeaderIfEtagFound(_pCacheEntryInfo);

            INET_ASSERT(error == ERROR_FILE_NOT_FOUND);

        }
        else
        {

            //
            // The cache entry is not expired, so we don't hit the net.
            //

            BETA_LOG (CACHE_NOT_EXPIRED);
            PERF_LOG(PE_TRACE, 0x99);
            ReuseObject();
            error = FHttpBeginCacheRetrieval(FALSE, FALSE);
            INET_ASSERT (error == ERROR_SUCCESS);
        }
    }

done:
    PERF_LOG(PE_CACHE_EXPIRY_CHECK_END);
    DEBUG_LEAVE(error);
    return (error);
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::GetFromCachePostNetIO(
    IN DWORD dwStatusCode,
    IN BOOL fVariation
    )

/*++

Routine Description:

    Check if this response needs to be cached or pulled from the cache
    after we have received the response from the net. Here we check for
    if-modified-since.

Arguments:

    dwStatusCode    - HTTP status code

Return Value:

    DWORD   Windows Error Code

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_REQUEST_HANDLE_OBJECT::GetFromCachePostNetIO",
                 "%d",
                 dwStatusCode
                 ));

    DWORD error = ERROR_FILE_NOT_FOUND;
    BOOL fLastModTimeFromServer = FALSE;
    BOOL fExpireTimeFromServer = FALSE;
    BOOL fPostCheckTimeFromServer = FALSE;

    //
    // check if
    // we can actually read the data from the cache after all. The idea here is
    // to improve efficiency by returning data from the cache if it hasn't
    // changed at the server. We determine this by the server returning a 304
    // response, or by determining that the date on the response is less than or
    // equal to the expires timestamp on the data we already wrote to cache
    //

    INET_ASSERT((dwStatusCode == HTTP_STATUS_NOT_MODIFIED)
                || (dwStatusCode == HTTP_STATUS_PRECOND_FAILED)
                || (dwStatusCode == HTTP_STATUS_OK)
                || (dwStatusCode == HTTP_STATUS_PARTIAL_CONTENT)
                || (dwStatusCode == 0));

    GetTimeStampsForCache(&_ftExpires,
                          &_ftLastModified,
                          &_ftPostCheck,
                          &fExpireTimeFromServer,
                          &fLastModTimeFromServer,
                          &fPostCheckTimeFromServer
                          );

    if (IsCacheReadDisabled())
    {
        INET_ASSERT (error == ERROR_FILE_NOT_FOUND);
        goto quit;
    }

    //
    // we may have no cache entry info, so no point in continuing - this file
    // was not in the cache when we started. (We could try to retrieve again
    // in case another download has put it in the cache)
    //

    if (_pCacheEntryInfo == NULL)
    {
        if (fVariation)
        {
            error = UrlCacheRetrieve(TRUE);
        }
        if (error!=ERROR_SUCCESS)
        {
            goto quit;
        }
    }

    if (_pCacheEntryInfo->CacheEntryType & SPARSE_CACHE_ENTRY)
    {

        if (dwStatusCode == HTTP_STATUS_PARTIAL_CONTENT)
        {
            BETA_LOG (CACHE_RESUMED);
            error = ResumePartialDownload();
        }
        else
        {
            BETA_LOG (CACHE_NOT_RESUMED);
            INET_ASSERT (error == ERROR_FILE_NOT_FOUND);
        }

        goto quit;
    }

    //
    // we are optimizing further here
    // if the return status is OK and the server sent us a
    // last modified time and this time is the same as
    // what we got earlier then we use the entry from the cache
    //

    if ((dwStatusCode == HTTP_STATUS_NOT_MODIFIED)
        || ((fLastModTimeFromServer)
            && (FT2LL(_ftLastModified)
                == FT2LL(_pCacheEntryInfo->LastModifiedTime))
            && GetMethodType() == HTTP_METHOD_TYPE_GET))
    {
        BETA_LOG (CACHE_NOT_MODIFIED);

        DEBUG_PRINT(HTTP,
                    INFO,
                    ("response code=%d %s, using cache entry\n",
                    GetStatusCode(),
                    GetURL()
                    ));

        DWORD dwAction = CACHE_ENTRY_SYNCTIME_FC;
        GetCurrentGmtTime(&(_pCacheEntryInfo->LastSyncTime));

        if (fExpireTimeFromServer)
        {
            (_pCacheEntryInfo->ExpireTime).dwLowDateTime = _ftExpires.dwLowDateTime;
            (_pCacheEntryInfo->ExpireTime).dwHighDateTime = _ftExpires.dwHighDateTime;
            dwAction |= CACHE_ENTRY_EXPTIME_FC;
        }

        //
        // Update the last sync time to the current time
        // so we can do once_per_session logic
        //
        if (!SetUrlCacheEntryInfoA(_pCacheEntryInfo->lpszSourceUrlName, _pCacheEntryInfo, dwAction))
        {
            // NB if this call fails, the worst that could happen is
            // that next time around we will do an if-modified-since
            // again
            INET_ASSERT(FALSE);
        }
        
        error = FHttpBeginCacheRetrieval(TRUE, FALSE);
        
        if (error != ERROR_SUCCESS)
        {
            //
            // if we failed to abort the transaction, or to retrieve the
            // data from the cache, then return the error only if the
            // response if not OK. Otherwise we can continue and
            // get the data
            //

            if (dwStatusCode != HTTP_STATUS_OK)
            {
                // this should never happen to us

                INET_ASSERT(FALSE);

                error = ERROR_FILE_NOT_FOUND;
            }
        }

    }
    else // We could not use the cache entry so release it.
    {
        BETA_LOG (CACHE_MODIFIED);
        INET_ASSERT (error == ERROR_FILE_NOT_FOUND);
    }

quit:

    if (error != ERROR_SUCCESS)
        UrlCacheUnlock();
    DEBUG_LEAVE(error);
    return error;
}


DWORD HTTP_REQUEST_HANDLE_OBJECT::ResumePartialDownload(void)

/*++

Routine Description:

    description-of-function.

Arguments:

    None.

Return Value:

    DWORD

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_REQUEST_HANDLE_OBJECT::ResumePartialDownload",
                 NULL));

    _ResponseHeaders.LockHeaders();

    const static char sz200[] = "200";

    INET_ASSERT (!_CacheWriteInProgress);

    LPSTR pszHeader;
    DWORD cbHeader;

    // Retrieve the start and end of Content-Range header
    if (!_iSlotContentRange)
        goto quit;
    pszHeader = _ResponseHeaders.GetHeaderPointer
        (_ResponseBuffer, _iSlotContentRange);
    INET_ASSERT (pszHeader);
    PSTR pszLimit;
    pszLimit = StrChr (pszHeader, '\n');
    INET_ASSERT (pszLimit);
    *pszLimit = 0;

    // Extract the document length as a string and number.
    // The http 1.1 spec is very explicit that we MUST parse
    // the header and return an error if invalid.  We expect
    // it to be of the form Content-Range: bytes xxx-yyy/zzz.
    PSTR pszStart, pszEnd, pszLength;
    DWORD dwStart, dwEnd, dwLength;

    // Ensure that value is prefixed with "bytes"
    PSTR pszBytes;
    pszBytes = pszHeader + (GlobalKnownHeaders[HTTP_QUERY_CONTENT_RANGE].Length+1); // +1 for the ':'
    SKIPWS (pszBytes);
    if (strnicmp (pszBytes, BYTES_SZ, BYTES_LEN))
        goto quit;

    // Parse and validate start of range.
    pszStart = pszBytes + BYTES_LEN;
    SKIPWS (pszStart);
    dwStart = StrToInt (pszStart);
    if (dwStart != _pCacheEntryInfo->dwSizeLow)
        goto quit;

    // Parse and validate end of range.
    pszEnd = StrChr (pszStart, '-');
    if (!pszEnd++)
        goto quit;
    dwEnd = StrToInt (pszEnd);
    if (dwStart > dwEnd)
        goto quit;

    // Parse and validate length.
    pszLength = StrChr (pszEnd, '/');
    if (!pszLength)
        goto quit;
    pszLength++;
    dwLength = StrToInt (pszLength);
    if (dwEnd + 1 != dwLength)
        goto quit;

    // The Content-Length header is the amount transmitted, however
    // as far as the client is concerned only the total amount of
    // data matters, so we fix it up from the Content-Range header.
    // Use the Content-Range buffer for the new Content-Length.
    // We know the buffer is big enough because the last number
    // in the Content-Range header is the new Content-Length value.
    _ContentLength = dwLength;

    // Remove the old Content-Length header, if any.
    if (_iSlotContentLength)
    {
        _ResponseHeaders.RemoveAllByIndex(HTTP_QUERY_CONTENT_LENGTH);
        //_ResponseHeaders.RemoveHeader (_iSlotContentLength, HTTP_QUERY_CONTENT_LENGTH, &_bKnownHeaders[dwQueryIndex]);
    }
    
    cbHeader = wsprintf (pszHeader, "%s: %d", GlobalKnownHeaders[HTTP_QUERY_CONTENT_LENGTH].Text,
            dwLength);
    _ResponseHeaders.ShrinkHeader
            (_ResponseBuffer, _iSlotContentRange, HTTP_QUERY_CONTENT_RANGE, HTTP_QUERY_CONTENT_LENGTH, cbHeader);

    // Fix up the response headers to appear same as a 200 response.
    // Revise the status line from 206 to 200.  The status code
    // starts after the first space, e.g. "HTTP/1.0 206 Partial Content"
    pszHeader = _ResponseHeaders.GetHeaderPointer (_ResponseBuffer, 0);
    INET_ASSERT (pszHeader);
    LPSTR pszStatus;
    pszStatus = StrChr (pszHeader, ' ');
    SKIPWS (pszStatus);
    INET_ASSERT (!memcmp(pszStatus, "206", 3));
    memcpy (pszStatus, sz200, sizeof(sz200));
    _ResponseHeaders.ShrinkHeader (_ResponseBuffer, 0,
            HTTP_QUERY_STATUS_TEXT, HTTP_QUERY_STATUS_TEXT,
            (DWORD) (pszStatus - pszHeader) + sizeof(sz200) - 1);
    _StatusCode = HTTP_STATUS_OK;

    // Some servers omit "Accept-Ranges: bytes" since it is implicit
    // in a 206 response.  This is important to Adobe Amber ActiveX
    // control and other clients that may issue their own range
    // requests. Add the header if not present.
    if (!IsResponseHeaderPresent(HTTP_QUERY_ACCEPT_RANGES))
    {
        const static char szAccept[] = "Accept-Ranges: bytes";
        DWORD dwErr = AddInternalResponseHeader
            (HTTP_QUERY_ACCEPT_RANGES, (LPSTR) szAccept, CSTRLEN(szAccept));
        INET_ASSERT (dwErr == ERROR_SUCCESS);
    }    

    // Adjust the handle values for socket read and file write positions.
    if (IsKeepAlive())
        _BytesRemaining = _ContentLength;
    _VirtualCacheFileSize = dwStart;
    _RealCacheFileSize    = dwStart;
    SetAvailableDataLength (dwStart);

    // Save the cache file path.
    INET_ASSERT(!_CacheFileName);
    _CacheFileName = NewString(_pCacheEntryInfo->lpszLocalFileName);
    if (!_CacheFileName)
        goto quit;

    // Open the file for read (should already be open for write)
    INET_ASSERT (_CacheFileHandle != INVALID_HANDLE_VALUE);

    _CacheFileHandleRead = CreateFile (_CacheFileName, GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, NULL);
    if (_CacheFileHandleRead == INVALID_HANDLE_VALUE)
        goto quit;

    // We can discard the partial cache entry info.
    FREE_MEMORY (_pCacheEntryInfo);
    _pCacheEntryInfo = NULL;

    _CacheWriteInProgress = TRUE;

quit:

    _ResponseHeaders.UnlockHeaders();

    DWORD dwErr;

    if (_CacheWriteInProgress)
    {
        dwErr = ERROR_SUCCESS;
    }
    else
    {
        INET_ASSERT (FALSE);

        dwErr = ERROR_HTTP_INVALID_SERVER_RESPONSE;

        if (_CacheFileName)
        {
            FREE_MEMORY (_CacheFileName);
            _CacheFileName = NULL;
        }
        if (_CacheFileHandleRead != INVALID_HANDLE_VALUE)
        {
            CloseHandle (_CacheFileHandleRead);
            _CacheFileHandleRead = INVALID_HANDLE_VALUE;
        }
    }

    DEBUG_LEAVE (dwErr);
    return dwErr;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::AddTimestampsFromCacheToResponseHeaders(
    IN LPCACHE_ENTRY_INFO lpCacheEntryInfo
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    lpCacheEntryInfo    -

Return Value:

    DWORD

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_REQUEST_HANDLE_OBJECT::AddTimestampsFromCacheToResponseHeaders",
                 "%#x",
                 lpCacheEntryInfo
                 ));

    PERF_LOG(PE_TRACE, 0x701);

    _ResponseHeaders.LockHeaders();

    DWORD error = AddTimeHeader(lpCacheEntryInfo->ExpireTime,
                                HTTP_QUERY_EXPIRES
                                );

    if (error == ERROR_SUCCESS) {
        error = AddTimeHeader(lpCacheEntryInfo->LastModifiedTime,
                              HTTP_QUERY_LAST_MODIFIED
                              );
    }

    if (error == ERROR_INVALID_PARAMETER) {
        error = ERROR_SUCCESS;
    }

    _ResponseHeaders.UnlockHeaders();

    PERF_LOG(PE_TRACE, 0x702);

    DEBUG_LEAVE(error);

    return error;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::AddTimeHeader(
    IN FILETIME fTime,
    IN DWORD dwHeaderIndex
    )

/*++

Routine Description:

    Adds a time header to this object

Arguments:

    fTime               - FILETIME value to add as header value

    dwHeaderIndex       - contains an index into a global array of Header strings.

    NOTE: Must be called under Header Critical Section!!!

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_PARAMETER
                    Couldn't convert fTime

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 None,
                 "HTTP_REQUEST_HANDLE_OBJECT::AddTimeHeader",
                 "%#x:%#x, %u [%q]",
                 fTime.dwLowDateTime,
                 fTime.dwHighDateTime,
                 dwHeaderIndex,
                 GlobalKnownHeaders[dwHeaderIndex].Text
                 ));

    char buf[MAX_PATH];
    SYSTEMTIME systemTime;
    DWORD error = ERROR_SUCCESS;

    if (FT2LL(fTime) != LONGLONG_ZERO) {
        if (FileTimeToSystemTime((CONST FILETIME *)&fTime, &systemTime)) {
            memcpy(buf, GlobalKnownHeaders[dwHeaderIndex].Text, GlobalKnownHeaders[dwHeaderIndex].Length);
            buf[GlobalKnownHeaders[dwHeaderIndex].Length]   = ':';
            buf[GlobalKnownHeaders[dwHeaderIndex].Length+1] = ' ';
            if (InternetTimeFromSystemTime((CONST SYSTEMTIME *)&systemTime,
                                           INTERNET_RFC1123_FORMAT,
                                           buf + (GlobalKnownHeaders[dwHeaderIndex].Length+2),
                                           sizeof(buf) - (GlobalKnownHeaders[dwHeaderIndex].Length+2)
                                           )) {
                error = AddInternalResponseHeader(dwHeaderIndex, buf, lstrlen(buf));

                INET_ASSERT(error == ERROR_SUCCESS);

                //
                // if it came upto here and all went well, only then would
                // the error be set to ERROR_SUCCESS
                //

                DEBUG_PRINT(HTTP,
                            INFO,
                            ("Cache: Adding header %s, errorcode=%d\n",
                            buf,
                            error
                            ));

            } else {

                INET_ASSERT(FALSE);

                error = ERROR_INVALID_PARAMETER;
            }
        } else {

            INET_ASSERT(FALSE);

            error = ERROR_INVALID_PARAMETER;
        }
    }

    DEBUG_LEAVE(error);

    return error;
}


BOOL HTTP_REQUEST_HANDLE_OBJECT::IsPartialResponseCacheable(void)

/*++

Routine Description:

    description-of-function.

Arguments:

    None.

Return Value:

    BOOL

--*/

{
    LPSTR lpszHeader;
    DWORD cbHeader;
    DWORD dwIndex;

    BOOL fRet = FALSE;
    DWORD err;

    _ResponseHeaders.LockHeaders();

    if (GlobalDisableReadRange || GetMethodType() != HTTP_METHOD_TYPE_GET)
    {
        INET_ASSERT(fRet == FALSE);
        goto quit;
    }

    if (_RealCacheFileSize >= _ContentLength)
    {
        // We don't handle chunked transfer upon resuming a partial
        // download, so we require a Content-Length header.  Also,
        // if download file is actually complete, yet not committed
        // to cache (possibly because the client didn't read to eof)
        // then we don't want to save a partial download.  Otherwise
        // we might later start a range request for the file starting
        // one byte beyond eof.  MS Proxy 1.0 will return an invalid
        // 206 response containing the last byte of the file.  Other
        // servers or proxies that follow the latest http spec might
        // return a 416 response which is equally useless to us.
        
        INET_ASSERT(fRet == FALSE);
        goto quit;
    }

    // For HTTP/1.0, must have last-modified time, otherwise we
    // don't have a way to tell if the partial downloads are coherent.
    if (!IsResponseHttp1_1() && (FT2LL(_ftLastModified) == 0))
    {
        INET_ASSERT(fRet == FALSE);
        goto quit;
    }

    if (!_iSlotContentRange)
    {
        // We didn't get a Content-Range header which implies the server
        // supports byte range for this URL, so we must look for the
        // explicit invitation of "Accept-Ranges: bytes" response header.

        dwIndex = 0;
        err = FastQueryResponseHeader (HTTP_QUERY_ACCEPT_RANGES,
            (LPVOID *) &lpszHeader,&cbHeader, dwIndex);
        if (err != ERROR_SUCCESS || !(cbHeader == BYTES_LEN && !strnicmp(lpszHeader, BYTES_SZ, cbHeader)) )
        {
            INET_ASSERT(fRet == FALSE);
            goto quit;
        }
    }

    if (!IsResponseHttp1_1())
    {
        // For HTTP/1.0, only cache responses from Server: Microsoft-???/*
        // Microsoft-PWS-95/*.* will respond with a single range but
        // with incorrect Content-Length and Content-Range headers.
        // Other 1.0 servers may return single range in multipart response.

        const static char szPrefix[] = "Microsoft-";
        const static DWORD ibSlashOffset = sizeof(szPrefix)-1 + 3;

        dwIndex = 0;
        if (    ERROR_SUCCESS != FastQueryResponseHeader (HTTP_QUERY_SERVER,
                    (LPVOID *) &lpszHeader, &cbHeader, dwIndex)
            ||  cbHeader <= ibSlashOffset
            ||  lpszHeader[ibSlashOffset] != '/'
            ||  memcmp (lpszHeader, szPrefix, sizeof(szPrefix) - 1)
           )
        {
            INET_ASSERT(fRet == FALSE);
            goto quit;
        }
    }

    else // if (IsResponseHttp1_1())
    {
        // For http 1.1, must have strong etag.  A weak etag starts with
        // a character other than a quote mark and cannot be used as a 
        // coherency validator.  IIS returns a weak etag when content is
        // modified within a single file system time quantum.
        
        if (ERROR_SUCCESS != FastQueryResponseHeader (HTTP_QUERY_ETAG,
                (LPVOID *) &lpszHeader, &cbHeader, 0)
            || *lpszHeader != '\"')
        {
            INET_ASSERT(fRet == FALSE);
            goto quit;
        }
    }

    fRet = TRUE;

quit:

    _ResponseHeaders.UnlockHeaders();

    return fRet;
 }


DWORD
HTTP_REQUEST_HANDLE_OBJECT::LocalEndCacheWrite(
    IN BOOL fNormal
    )

/*++

Routine Description:

    Finishes up the cache-write operation, either committing the cache entry or
    deleting it (because we failed)

Arguments:

    fNormal - TRUE if normal end-cache-write operation (i.e. FALSE if error)

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY
                  ERROR_INTERNET_INTERNAL_ERROR

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_REQUEST_HANDLE_OBJECT::LocalEndCacheWrite",
                 "%B",
                 fNormal
                 ));

    PERF_LOG(PE_TRACE, 0x2001);

    LPSTR lpszBuf = NULL;
    char buff[256];
    DWORD dwBuffLen;
    DWORD dwError;
    DWORD dwUserNameHeader = 0;
    LPSTR lpszHeaderInfo = NULL;

    DWORD dwEntryType = 0;

    if (!fNormal)
    {
        if (!IsPartialResponseCacheable())
        {
            BETA_LOG (DOWNLOAD_ABORTED);
        }
        else
        {
            //
            // Flush any buffered data to disk.
            // BUGBUG: doesn't work with chunked transfer
            // WriteResponseBufferToCache();
            // WriteQueryBufferToCache();
            //

            // dprintf ("wininet: partial cached %s\n", GetURL());

            BETA_LOG (DOWNLOAD_PARTIAL);

            fNormal = TRUE;
            dwEntryType = SPARSE_CACHE_ENTRY;

            //
            // Disable InternetUnlockRequest
            //

            LOCK_REQUEST_INFO* pLock =
                (LOCK_REQUEST_INFO*) GetLockRequestHandle();
            if (pLock)
                pLock->fNoDelete = TRUE;
        }
    }

    if (fNormal)
    {
        if (GetCacheFlags() & INTERNET_FLAG_MAKE_PERSISTENT)
            dwEntryType |= STICKY_CACHE_ENTRY;

        if (GetSecondaryCacheKey())
            dwEntryType |= POST_RESPONSE_CACHE_ENTRY;

        if (IsResponseHttp1_1())
        {
            dwEntryType |= HTTP_1_1_CACHE_ENTRY;
            if (IsMustRevalidate())
                dwEntryType |= MUST_REVALIDATE_CACHE_ENTRY;
        }                
        
        if (IsPerUserItem())
        {
            //
            // create per-user header, e.g. "~U:JoeBlow\r\n", if it fits in the
            // buffer
            //

            INET_ASSERT(vdwCurrentUserLen);

            dwUserNameHeader = sizeof(vszUserNameHeader) - 1
                             + vdwCurrentUserLen
                             + sizeof("\r\n");
            if (sizeof(buff) >= dwUserNameHeader)
            {
                memcpy(buff, vszUserNameHeader, sizeof(vszUserNameHeader) - 1);

                DWORD dwSize = lstrlen(vszCurrentUser);

                memcpy(&buff[sizeof(vszUserNameHeader) - 1],
                       vszCurrentUser,
                       dwSize);
                dwSize += sizeof(vszUserNameHeader) - 1;
                memcpy(&buff[dwSize], "\r\n", sizeof("\r\n"));
            }
            else
            {
                // if it failed, mark it as expired

                dwUserNameHeader = 0;
                GetCurrentGmtTime(&_ftExpires);
                AddLongLongToFT(&_ftExpires, (-1)*(ONE_HOUR_DELTA));
            }
        }

        //
        // start off with 512 byte buffer (used to be 256, but headers are
        // getting larger)
        //

        dwBuffLen = 512;

        do
        {
            DWORD dwPreviousLength;

            lpszBuf = (LPSTR)ResizeBuffer(lpszBuf, dwBuffLen, FALSE);
            if (lpszBuf == NULL)
            {
                dwError = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
            dwBuffLen -= dwUserNameHeader;

            dwPreviousLength = dwBuffLen;
            lpszHeaderInfo = lpszBuf;
            lpszBuf[0] = '\0';

            dwError = QueryRawResponseHeaders(TRUE,
                                              (LPVOID)lpszHeaderInfo,
                                              &dwBuffLen
                                              );
            if (dwError == ERROR_SUCCESS)
            {

                FilterHeaders(lpszHeaderInfo, &dwBuffLen);
                if (dwUserNameHeader)
                {
                    lstrcat(lpszHeaderInfo, buff);
                    dwBuffLen += dwUserNameHeader;
                }
                break; // get out
            }

            //
            // we have the right error and we haven't allocated more memory yet
            //

            if (dwError == ERROR_INSUFFICIENT_BUFFER)
            {
                if (dwBuffLen <= dwPreviousLength)
                {
                    dwError = ERROR_INTERNET_INTERNAL_ERROR;
                    break;
                }

                dwBuffLen += dwUserNameHeader;
            }
            else
            {
                //
                // if HttpQueryInfo() returned ERROR_HTTP_DOWNLEVEL_SERVER
                // or we are making CERN proxy requests for FTP or gopher, then
                // there are no headers from the origin server, but the
                // operation succeeded
                //

                if (dwError == ERROR_HTTP_DOWNLEVEL_SERVER)
                    dwError = ERROR_SUCCESS;

                //
                // either allready tried once with allocation or
                // we got some error other than ERROR_INSUFFICIENT_BUFFER
                //

                INET_ASSERT(*lpszHeaderInfo == '\0');

                lpszHeaderInfo = NULL;
                dwBuffLen = 0;
                break;
            }

        } while (TRUE);

        fNormal = (dwError == ERROR_SUCCESS);
    }

    if (!fNormal)
    {
        dwEntryType = 0xffffffff;
        lpszHeaderInfo = NULL;
    }

    //
    // Simulate a redirect if the original object was the root
    // ("" or "/") and we did not otherwise get a redirect.
    //

    if (IsObjectRoot() && _OriginalUrl && !lstrcmpi (_OriginalUrl, GetURL()))
    {
        DWORD dwLen = lstrlen (_OriginalUrl);
        INET_ASSERT (_OriginalUrl[dwLen - 1] == '/');
        _OriginalUrl[dwLen - 1] = 0;
    }

    //
    // Let the cache know if the item is likely to be a static image.
    // 1. No expire time.
    // 2. Has last-modified time.
    // 3. The content-type is image/*
    // 4. No '?' in the URL.
    //

    LPSTR lpszHeader;
    DWORD cbHeader;
    BOOL fImage =
        (  !FT2LL(_ftExpires)
        && FT2LL(_ftLastModified)
        && (ERROR_SUCCESS == FastQueryResponseHeader (HTTP_QUERY_CONTENT_TYPE,
                (LPVOID *) &lpszHeader, &cbHeader, 0))
        && (StrCmpNI (lpszHeader, "image/", sizeof("image/")-1) == 0)
        && (!StrChr (GetURL(), '?'))
        );

    DEBUG_PRINT(CACHE,
                INFO,
                ("Cache write EntryType = %x\r\n",
                dwEntryType
                ));

    dwError = EndCacheWrite(&_ftExpires,
                            &_ftLastModified,
                            &_ftPostCheck,
                            dwEntryType,
                            dwBuffLen,
                            lpszHeaderInfo,
                            NULL,
                            fImage
                            );

    if (fNormal && !(dwEntryType & SPARSE_CACHE_ENTRY))
    {
        if (dwError == ERROR_SUCCESS)
        {
            BETA_LOG (DOWNLOAD_CACHED);
        }
        else
        {
            BETA_LOG (DOWNLOAD_NOT_CACHED);
        }
    }

    if (lpszBuf != NULL) {

        lpszBuf = (LPSTR)FREE_MEMORY(lpszBuf);

        INET_ASSERT(lpszBuf == NULL);

    }

    PERF_LOG(PE_TRACE, 0x2002);

    DEBUG_LEAVE(dwError);

    return dwError;
}


VOID
HTTP_REQUEST_HANDLE_OBJECT::GetTimeStampsForCache(
    OUT LPFILETIME lpftExpiryTime,
    OUT LPFILETIME lpftLastModTime,
    OUT LPFILETIME lpftPostCheckTime,
    OUT LPBOOL lpfHasExpiry,
    OUT LPBOOL lpfHasLastModTime,
    OUT LPBOOL lpfHasPostCheck
    )

/*++

Routine Description:

    extracts timestamps from the http response. If the timestamps don't exist,
    does the default thing. has additional goodies like checking for expiry etc.

Arguments:

    lpftExpiryTime      - returned expiry time

    lpftLatsModTime     - returned last-modified time

    lpfHasExpiry        - returned TRUE if the response header contains an expiry
                          timestamp. This patameter can be NULL

    lpfHasLastModTime   - returned TRUE if the response header contains a
                          last-modified timestamp. This patameter can be NULL


Return Value:

    None.

Comment:

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 None,
                 "HTTP_REQUEST_HANDLE_OBJECT::GetTimeStampsForCache",
                 "%#x, %#x, %#x, %#x",
                 lpftExpiryTime,
                 lpftLastModTime,
                 lpfHasExpiry,
                 lpfHasLastModTime
                 ));

    PERF_LOG(PE_TRACE, 0x9001);

    char buf[512];
    LPSTR lpszBuf;
    BOOL fRet;

    DWORD length, index = 0;
    BOOL fPostCheck = FALSE;
    BOOL fPreCheck = FALSE;
    FILETIME ftPreCheckTime;
    FILETIME ftPostCheckTime;

    fRet = FALSE;

    _ResponseHeaders.LockHeaders();

    // Determine if a Cache-Control: max-age header exists. If so, calculate expires
    // time from current time + max-age minus any delta indicated by Age:

    //
    // we really want all the post-fetch stuff works with 1.0 proxy
    // so we loose our grip a little bit here: enable all Cache-Control
    // max-age work with 1.0 response.
    //
    //if (IsResponseHttp1_1())
    {
        CHAR  *ptr, *pToken;
        INT nDeltaSecsPostCheck = 0;
        INT nDeltaSecsPreCheck = 0;
        while (1)
        {
            // Scan headers for Cache-Control: max-age header.
            length = sizeof(buf);
            switch (QueryResponseHeader(HTTP_QUERY_CACHE_CONTROL,
                buf,
                &length,
                0,
                &index))
            {
                case ERROR_SUCCESS:

                    buf[length] = '\0';
                    pToken = ptr = buf;

                    // Parse a token from the string; test for sub headers.
                    while (pToken = StrTokEx(&ptr, ","))
                    {
                        SKIPWS(pToken);

                        if (strnicmp(POSTCHECK_SZ, pToken, POSTCHECK_LEN) == 0)
                        {
                            pToken += POSTCHECK_LEN;

                            SKIPWS(pToken);

                            if (*pToken != '=')
                                break;

                            pToken++;

                            SKIPWS(pToken);

                            nDeltaSecsPostCheck = atoi(pToken);

                            // Calculate post fetch time 
                            GetCurrentGmtTime(&ftPostCheckTime);
                            AddLongLongToFT(&ftPostCheckTime, (nDeltaSecsPostCheck * (LONGLONG) 10000000));
                
                            fPostCheck = TRUE;
                        }

                        else if (strnicmp(PRECHECK_SZ, pToken, PRECHECK_LEN) == 0)
                        {
                            // found
                            pToken += PRECHECK_LEN;

                            SKIPWS(pToken);

                            if (*pToken != '=')
                                break;

                            pToken++;

                            SKIPWS(pToken);

                            nDeltaSecsPreCheck = atoi(pToken);

                            // Calculate pre fetch time (overwrites ftExpire ) 
                            GetCurrentGmtTime(&ftPreCheckTime);
                            AddLongLongToFT(&ftPreCheckTime, (nDeltaSecsPreCheck * (LONGLONG) 10000000));

                            fPreCheck = TRUE;
                        }

                        else if (strnicmp(MAX_AGE_SZ, pToken, MAX_AGE_LEN) == 0)
                        {
                            // Found max-age. Convert to integer form.
                            // Parse out time in seconds, text and convert.
                            pToken += MAX_AGE_LEN;

                            SKIPWS(pToken);

                            if (*pToken != '=')
                                break;

                            pToken++;

                            SKIPWS(pToken);

                            INT nDeltaSecs = atoi(pToken);
                            INT nAge;

                            // See if an Age: header exists.
                            index = 0;
                            length = sizeof(INT)+1;

                            if (QueryResponseHeader(HTTP_QUERY_AGE,
                                &nAge,
                                &length,
                                HTTP_QUERY_FLAG_NUMBER,
                                &index) == ERROR_SUCCESS)

                            {
                                // Found Age header. Convert and subtact from max-age.
                                // If less or = 0, attempt to get expires header.
                                nAge = ((nAge < 0) ? 0 : nAge);
                                nDeltaSecs -= nAge;
                                if (nDeltaSecs <= 0)
                                    break;
                            }

                            // Calculate expires time from max age.
                            GetCurrentGmtTime(lpftExpiryTime);
                            //*((LONGLONG *)lpftExpiryTime) += (nDeltaSecs * (LONGLONG) 10000000);
                            AddLongLongToFT(lpftExpiryTime, (nDeltaSecs * (LONGLONG) 10000000));
                            fRet = TRUE;
                        }

                        else if (strnicmp(MUST_REVALIDATE_SZ, pToken, MUST_REVALIDATE_LEN) == 0)
                        {
                            pToken += MUST_REVALIDATE_LEN;
                            SKIPWS(pToken);
                            if (*pToken == 0 || *pToken == ',')
                                SetMustRevalidate();
                        }
                    }

                    // If an expires time has been found, break switch.
                    if (fRet)
                        break;
                    continue;

                case ERROR_INSUFFICIENT_BUFFER:
                    index++;
                    continue;

                default:
                    break; // no more Cache-Control headers.
            }

            //
            // pre-post fetch headers must come in pair, also
            // pre fetch header overwrites the expire 
            // and make sure postcheck < precheck
            //
            if( fPreCheck && fPostCheck && 
                ( nDeltaSecsPostCheck < nDeltaSecsPreCheck ) ) 
            {
                fRet = TRUE;
                *lpftPostCheckTime  = ftPostCheckTime;
                *lpftExpiryTime     = ftPreCheckTime;
                if( lpfHasPostCheck )
                    *lpfHasPostCheck = TRUE;

                if( nDeltaSecsPostCheck == 0 && 
                    !(GetCacheFlags() & INTERNET_FLAG_BGUPDATE) )
                {
                    //
                    // "post-check = 0"
                    // this page has already passed the lazy update time
                    // this means server wants us to do background update 
                    // after the first download  
                    //
                    // (bg fsm will be created at the end of the cache write)
                    //
                    _fLazyUpdate = TRUE;
                }

            }
            else
            {
                fPreCheck = FALSE;
                fPostCheck = FALSE;
            }

            break; // no more Cache-Control headers.
        }

    } // Is http 1.1

   

    // If no expires time is calculated from max-age, check for expires header.
    if (!fRet)
    {
        length = sizeof(buf) - 1;
        index = 0;
        if (QueryResponseHeader(HTTP_QUERY_EXPIRES, buf, &length, 0, &index) == ERROR_SUCCESS)
        {
            fRet = FParseHttpDate(lpftExpiryTime, buf);

            //
            // as per HTTP spec, if the expiry time is incorrect, then the page is
            // considered to have expired
            //

            if (!fRet)
            {
                GetCurrentGmtTime(lpftExpiryTime);
                AddLongLongToFT(lpftExpiryTime, (-1)*ONE_HOUR_DELTA); // subtract 1 hour
                fRet = TRUE;
            }
        }
    }

    // We found or calculated a valid expiry time, let us check it against the
    // server date if possible
    FILETIME ft;
    length = sizeof(buf) - 1;
    index = 0;

    if (QueryResponseHeader(HTTP_QUERY_DATE, buf, &length, 0, &index) == ERROR_SUCCESS
        && FParseHttpDate(&ft, buf))
    {

        // we found a valid Data: header

        // if the expires: date is less than or equal to the Date: header
        // then we put an expired timestamp on this item.
        // Otherwise we let it be the same as was returned by the server.
        // This may cause problems due to mismatched clocks between
        // the client and the server, but this is the best that can be done.

        // Calulating an expires offset from server date causes pages
        // coming from proxy cache to expire later, because proxies
        // do not change the date: field even if the reponse has been
        // sitting the proxy cache for days.

        // This behaviour is as-per the HTTP spec.


        if (FT2LL(*lpftExpiryTime) <= FT2LL(ft))
        {
            GetCurrentGmtTime(lpftExpiryTime);
            AddLongLongToFT(lpftExpiryTime, (-1)*ONE_HOUR_DELTA); // subtract 1 hour
        }
    }

    if (lpfHasExpiry)
    {
        *lpfHasExpiry = fRet;
    }

    if (!fRet)
    {
        lpftExpiryTime->dwLowDateTime = 0;
        lpftExpiryTime->dwHighDateTime = 0;
    }

    fRet = FALSE;
    length = sizeof(buf) - 1;
    index = 0;

    if (QueryResponseHeader(HTTP_QUERY_LAST_MODIFIED, buf, &length, 0, &index) == ERROR_SUCCESS)
    {
        DEBUG_PRINT(CACHE,
                    INFO,
                    ("Last Modified date is: %q\n",
                    buf
                    ));

        fRet = FParseHttpDate(lpftLastModTime, buf);

        if (!fRet)
        {
            DEBUG_PRINT(CACHE,
                        ERROR,
                        ("FParseHttpDate() returns FALSE\n"
                        ));
        }
    }

    if (lpfHasLastModTime)
    {
        *lpfHasLastModTime = fRet;
    }

    if (!fRet)
    {
        lpftLastModTime->dwLowDateTime = 0;
        lpftLastModTime->dwHighDateTime = 0;
    }

    _ResponseHeaders.UnlockHeaders();

    PERF_LOG(PE_TRACE, 0x9002);

    DEBUG_LEAVE(0);
}

//
// private functions
//


PRIVATE
VOID
FilterHeaders(
    IN LPSTR lpszHeaderInfo,
    OUT LPDWORD lpdwHeaderLen
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    lpszHeaderInfo  -
    lpdwHeaderLen   -

Return Value:

    None.

--*/

{
    PERF_LOG(PE_TRACE, 0x3001);

    DWORD i, len, lenT, reduced = 0, dwHeaderTableCount;
    LPSTR lpT, lpMark, lpNext, *lprgszHeaderExcludeTable;

    //
    // skip over the status line
    // NB this assumes that the raw buffer is nullterminated
    //

    if (lpvrgszHeaderExclusionTable) {
        lprgszHeaderExcludeTable = lpvrgszHeaderExclusionTable;
        dwHeaderTableCount = vdwHeaderExclusionTableCount;
    } else {
        lprgszHeaderExcludeTable = rgszExcludeHeaders;
        dwHeaderTableCount = sizeof(rgszExcludeHeaders) / sizeof(LPSTR);
    }

    lpT = strchr(lpszHeaderInfo, '\r');
    if (!lpT) {

        PERF_LOG(PE_TRACE, 0x3002);

        return;
    }

    INET_ASSERT(*(lpT + 1) == '\n');

    lpT += 2;

    do {

        //
        // find the header portion
        //

        lpMark = strchr(lpT, ':');
        if (!lpMark) {
            break;
        }

        //
        // get the end of the header line
        //

        lpNext = strchr(lpMark, '\r');

        if (!lpNext)
        {
            INET_ASSERT(FALSE);
            // A properly formed header _should_ terminate with \r\n, but sometimes
            // that just doesn't happen
            lpNext = lpMark;
            while (*lpNext)
            {
                lpNext++;
            }
        }
        else
        {
            INET_ASSERT(*(lpNext + 1) == '\n');
            lpNext += 2;
        }


        len = (DWORD) PtrDifference(lpMark, lpT) + 1; 
        lenT = *lpdwHeaderLen;  // doing all this to see it properly in debugger

        BOOL bFound = FALSE;

        for (i = 0; i < dwHeaderTableCount; ++i) {
            if (!strnicmp(lpT, lprgszHeaderExcludeTable[i], len)) {
                bFound = TRUE;
                break;
            }
        }
        if (bFound) {

            //
            // nuke this header
            //

            len = lenT - (DWORD)PtrDifference(lpNext, lpszHeaderInfo) + 1; // for NULL character

            //
            // ACHTUNG memove because of overlapped copies
            //

            memmove(lpT, lpNext, len);

            //
            // keep count of how much we reduced the header by
            //

            reduced += (DWORD) PtrDifference(lpNext, lpT);

            //
            // lpT is already properly positioned because of the move
            //

        } else {
            lpT = lpNext;
        }
    } while (TRUE);
    *lpdwHeaderLen -= reduced;

    PERF_LOG(PE_TRACE, 0x3003);
}


PRIVATE
BOOL
FExcludedMimeType(
    IN LPSTR lpszMimeType,
    IN DWORD dwMimeTypeSize
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    lpszMimeType    -

Return Value:

    BOOL

--*/

{
    PERF_LOG(PE_TRACE, 0x501);

    DWORD i;
    LPCSTR * lprgszMimeExcludeTable = rgszExcludedMimeTypes;
    DWORD dwMimeExcludeCount = (sizeof(rgszExcludedMimeTypes)/sizeof(LPSTR));
    const DWORD *lprgdwMimeExcludeTableOfSizes = rgdwExcludedMimeTypeSizes;

    if (lpvrgszMimeExclusionTable) {
        lprgszMimeExcludeTable = (LPCSTR *)lpvrgszMimeExclusionTable;
        dwMimeExcludeCount = vdwMimeExclusionTableCount;
        lprgdwMimeExcludeTableOfSizes = lpvrgdwMimeExclusionTableOfSizes;
    }
    for (i = 0; i < dwMimeExcludeCount; ++i) {
        if ((dwMimeTypeSize == lprgdwMimeExcludeTableOfSizes[i]) &&
            !strnicmp(lpszMimeType,
                      lprgszMimeExcludeTable[i],
                      lprgdwMimeExcludeTableOfSizes[i])) {

            PERF_LOG(PE_TRACE, 0x502);

            return TRUE;
        }
    }

    PERF_LOG(PE_TRACE, 0x503);

    return FALSE;
}
