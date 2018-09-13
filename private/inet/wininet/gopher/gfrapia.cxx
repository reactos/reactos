/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    gfrapia.cxx

Abstract:

    ANSI versions of Windows Internet Extensions Gopher Protocol APIs

    Contents:
        GopherCreateLocatorA
        GopherGetLocatorTypeA
        GopherFindFirstFileA
        GopherFindNextA
        GopherOpenFileA
        GopherReadFile
        GopherGetAttributeA
        GopherSendDataA
        pGfrGetUrlInfo
        pGopherGetUrlString

Author:

    Richard L Firth (rfirth) 11-Oct-1994

Environment:

    Win32 user-level DLL

Revision History:

    11-Oct-1994 rfirth
        Created

--*/

#include <wininetp.h>
#include "gfrapih.h"

//
// manifests
//

#define GOPHER_ATTRIBUTE_BUFFER_LENGTH  (4 K)   // arbitrary
#define MAX_GOPHER_SEARCH_STRING_LENGTH (1 K)   // arbitrary

// used as delimiter in creating a unique cache name to append
// searchstring or viewtype to a url
#define GOPHER_EXTENSION_DELIMITER_SZ   "<>"

DWORD
pGopherGetUrlString(
    IN INTERNET_SCHEME SchemeType,
    IN LPSTR    lpszTargetHost,
    IN LPSTR    lpszCWD,
    IN LPSTR    lpszObjectLocator,
    IN LPSTR    lpszExtension,
    IN DWORD    dwPort,
    OUT LPSTR   *lplpUrlName,
    OUT LPDWORD lpdwUrlLen
    );

//
// private functions
//

PRIVATE
BOOL
FGopherBeginCacheReadProcessing(
    IN HINTERNET hGopherSession,
    IN LPCSTR lpszFileName,
    IN LPCSTR lpszViewType,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext,
    BOOL    fIsHtmlFind
    );

PRIVATE
BOOL
FGopherCanReadFromCache(
    HINTERNET   hGopherSession
    );

PRIVATE
BOOL
FGopherBeginCacheWriteProcessing(
    IN HINTERNET hGopherSession,
    IN LPCSTR lpszFileName,
    IN LPCSTR lpszViewType,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext,
    BOOL    fIsHtmlFind
    );

PRIVATE
BOOL
FGopherCanWriteToCache(
    HINTERNET   hGopherSession
    );

DWORD
InbGopherLocalEndCacheWrite(
    IN HINTERNET hGopherFile,
    IN LPSTR    lpszFileExtension,
    IN BOOL fNormal
    );


PRIVATE
BOOL
FIsGopherExpired(
    HINTERNET hGopher,
    LPCACHE_ENTRY_INFO lpCEI
    );

//
// functions
//


INTERNETAPI
BOOL
WINAPI
GopherCreateLocatorA(
    IN LPCSTR lpszHost,
    IN INTERNET_PORT nServerPort,
    IN LPCSTR lpszDisplayString OPTIONAL,
    IN LPCSTR lpszSelectorString OPTIONAL,
    IN DWORD dwGopherType,
    OUT LPSTR lpszLocator OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
    )

/*++

Routine Description:

    Creates a gopher locator string. The string should be an opaque type so far
    as the app is concerned.

    This API mainly exists for situations where the app may want to request
    explicit information, without first having contacted a server and asked for
    a list of available information

Arguments:

    lpszHost            - Name of the host where the gopher server lives

    nServerPort         - Port at which the gopher server listens. The default
                          value 70 will be substituted if 0 is passed in

    lpszDisplayString   - Optional display string. Mainly a place holder. Can be
                          NULL, or NUL string, in which case a default string
                          will be substituted (just \t)

    lpszSelectorString  - The string used to select the item at the gopher
                          server. Can be NULL

    dwGopherType        - Tells us that the item to return is a file or
                          directory, graphics image, audio file, etc...
                          If 0, the default GOPHER_TYPE_DIRECTORY is used

    lpszLocator         - Place where the locator is returned

    lpdwBufferLength    - IN: Length of Buffer
                          OUT: Required length of Buffer only if
                          ERROR_INSUFFICIENT_BUFFER returned

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError()/InternetGetLastResponseInfo() to
                  get more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "GopherCreateLocatorA",
                     "%q, %d, %q, %q, %#x, %#x, %#x [%d]",
                     lpszHost,
                     nServerPort,
                     lpszDisplayString,
                     lpszSelectorString,
                     dwGopherType,
                     lpszLocator,
                     lpdwBufferLength,
                     lpdwBufferLength ? *lpdwBufferLength : 0
                     ));

    DWORD requiredLength;
    char portBuffer[INTERNET_MAX_PORT_NUMBER_LENGTH + 1];
    char gopherChar;
    DWORD displayStringLength;
    DWORD selectorStringLength;
    DWORD hostNameLength;
    DWORD portLength;
    BOOL gopherPlus;
    BOOL success;

    //
    // default is directory, ordinary gopher (i.e. not gopher+)
    //

    if (dwGopherType == 0) {
        dwGopherType = GOPHER_TYPE_DIRECTORY;
    }

    gopherChar = GopherTypeToChar(dwGopherType);

    //
    // validate parameters
    //

    if (IsBadStringPtr(lpszHost, MAX_GOPHER_HOST_NAME)
    || (*lpszHost == '\0')
    || (ARGUMENT_PRESENT(lpszDisplayString)
        && IsBadStringPtr(lpszDisplayString, MAX_GOPHER_DISPLAY_TEXT))
    || (ARGUMENT_PRESENT(lpszSelectorString)
        && IsBadStringPtr(lpszSelectorString, MAX_GOPHER_SELECTOR_TEXT))
    || IsBadWritePtr(lpdwBufferLength, sizeof(*lpdwBufferLength))
    || (ARGUMENT_PRESENT(lpszLocator)
        && IsBadWritePtr(lpszLocator, *lpdwBufferLength))
    || (gopherChar == INVALID_GOPHER_TYPE)) {

        DEBUG_ERROR(API, ERROR_INVALID_PARAMETER);

        DEBUG_LEAVE(FALSE);

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // ensure that if the caller passed in a NULL locator pointer then the size
    // of the buffer is 0
    //

    if (!ARGUMENT_PRESENT(lpszLocator)) {
        *lpdwBufferLength = 0;
    }

    if (nServerPort == 0) {
        nServerPort = INTERNET_DEFAULT_GOPHER_PORT;
    }

    //ULTOA(nServerPort, portBuffer, 10);
    wsprintf (portBuffer, "%u", nServerPort);


    if (!(ARGUMENT_PRESENT(lpszDisplayString) && (*lpszDisplayString != '\0'))) {
        lpszDisplayString = DEFAULT_GOPHER_DISPLAY_STRING;
    }

    if (!(ARGUMENT_PRESENT(lpszSelectorString) && (*lpszSelectorString != '\0'))) {
        lpszSelectorString = DEFAULT_GOPHER_SELECTOR_STRING;
    }

    gopherPlus = IS_GOPHER_PLUS(dwGopherType);

    displayStringLength = lstrlen(lpszDisplayString);
    selectorStringLength = lstrlen(lpszSelectorString);
    hostNameLength = lstrlen(lpszHost);
    portLength = lstrlen(portBuffer);

    //
    // calculate how many bytes of buffer required for the locator
    //

    requiredLength = sizeof(char)           // descriptor character
                   + displayStringLength
                   + sizeof(char)           // TAB
                   + selectorStringLength
                   + sizeof(char)           // TAB
                   + hostNameLength
                   + sizeof(char)           // TAB
                   + portLength
                   + (gopherPlus ? 2 : 0)   // TAB, '+'
                   + sizeof(char)           // CR
                   + sizeof(char)           // LF
                   + sizeof(char)           // EOS
                   ;

    //
    // and if the caller supplied at least that much, then create the locator,
    // else just return the size of buffer needed
    //

    if (*lpdwBufferLength >= requiredLength) {
        *lpszLocator++ = gopherChar;

        CopyMemory(lpszLocator, lpszDisplayString, displayStringLength);
        lpszLocator += displayStringLength;
        *lpszLocator++ = '\t';

        CopyMemory(lpszLocator, lpszSelectorString, selectorStringLength);
        lpszLocator += selectorStringLength;
        *lpszLocator++ = '\t';

        CopyMemory(lpszLocator, lpszHost, hostNameLength);
        lpszLocator += hostNameLength;
        *lpszLocator++ = '\t';

        CopyMemory(lpszLocator, portBuffer, portLength);
        lpszLocator += portLength;

        if (gopherPlus) {
            *lpszLocator++ = '\t';
            *lpszLocator++ = '+';
        }

        *lpszLocator++ = '\r';
        *lpszLocator++ = '\n';
        *lpszLocator = '\0';

        //
        // in the case of a successful copy, we return *lpdwBufferLength as the
        // number of characters in the string, as if returned by lstrlen()
        //

        --requiredLength;
        success = TRUE;
    } else {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);

        DEBUG_ERROR(API, ERROR_INSUFFICIENT_BUFFER);

        success = FALSE;
    }
    *lpdwBufferLength = requiredLength;

    DEBUG_LEAVE_API(success);

    return success;
}


INTERNETAPI
BOOL
WINAPI
GopherGetLocatorTypeA(
    IN LPCSTR lpszLocator,
    OUT LPDWORD lpdwGopherType
    )

/*++

Routine Description:

    Returns the type of the locator

Arguments:

    lpszLocator     - pointer to locator to return type of

    lpdwGopherType  - pointer to DWORD where type is returned

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Check GetLastError() for more information:
                    ERROR_INVALID_PARAMETER
                    ERROR_GOPHER_UNKNOWN_LOCATOR

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "GopherGetLocatorTypeA",
                     "%q, %#x",
                     lpszLocator,
                     lpdwGopherType
                     ));

    DWORD error;

    if (IsValidLocator(lpszLocator, MAX_GOPHER_LOCATOR_LENGTH)
    && !IsBadWritePtr(lpdwGopherType, sizeof(*lpdwGopherType))) {

        DWORD gopherType;

        gopherType = GopherCharToType(*lpszLocator);
        if (gopherType != INVALID_GOPHER_CHAR) {
            gopherType |= IsGopherPlus(lpszLocator)
                            ? GOPHER_TYPE_GOPHER_PLUS
                            : 0
                            ;
            *lpdwGopherType = gopherType;
            error = ERROR_SUCCESS;
        } else {
            error = ERROR_GOPHER_UNKNOWN_LOCATOR;
        }
    } else {
        error = ERROR_INVALID_PARAMETER;
    }

    DWORD success;

    if (error == ERROR_SUCCESS) {
        success = TRUE;
    } else {

        DEBUG_ERROR(API, error);

        SetLastError(error);
        success = FALSE;
    }

    DEBUG_LEAVE_API(success);

    return success;
}


INTERNETAPI
HINTERNET
WINAPI
GopherFindFirstFileA(
    IN HINTERNET hGopherSession,
    IN LPCSTR lpszLocator OPTIONAL,
    IN LPCSTR lpszSearchString OPTIONAL,
    OUT LPGOPHER_FIND_DATAA lpBuffer OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    Used to return directory/hierarchy information from the gopher server

Arguments:

    hGopherSession      - identifies gopher server context to use - either via
                          gateway, or straight to internet

    lpszLocator         - descriptor of information to get. If NULL then we get
                          the default directory at the server

    lpszSearchString    - if this request is to an search server, this
                          parameter specifies the string(s) to search for. If
                          Locator does not specify a search server then
                          the paraneter is not used, but it IS validated

    lpBuffer            - pointer to user-allocated buffer in which to return
                          info. This parameter may be NULL, in which case if
                          this function returns success, then the results of
                          the request will be returned via InternetFindNextFile()

    dwFlags             - controlling caching, etc.

    dwContext           - app-supplied context value for use in call-backs

Return Value:

    HINTERNET
        Success - valid handle value
                    If lpBuffer was not NULL, a GOPHER_FIND_DATA structure has
                    been returned in lpBuffer. Use InternetFindNextFile() to
                    retrieve the remainder of the directory entries.
                    If lpBuffer was NULL, then this call is still successful,
                    but all directory entries (including the first) will be
                    returned via InternetFindNextFile()

        Failure - NULL
                    Use GetLastError()/InternetGetLastResponseInfo() to get
                    more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Handle,
                     "GopherFindFirstFileA",
                     "%#x, %q, %q, %#x, %#x, %$x",
                     hGopherSession,
                     lpszLocator,
                     lpszSearchString,
                     lpBuffer,
                     dwFlags,
                     dwContext
                     ));

    HINTERNET findHandle;
    HINTERNET hConnectMapped = NULL;
    HINTERNET hObject;
    HINTERNET hObjectMapped = NULL;

    DWORD error;
    LPINTERNET_THREAD_INFO lpThreadInfo;
    DWORD nestingLevel = 0;
    BOOL fDeref = TRUE;

    if (!GlobalDataInitialized) {
        error = ERROR_INTERNET_NOT_INITIALIZED;
        goto quit;
    }

    //
    // get the per-thread info block
    //

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto done;
    }

    _InternetIncNestingCount();
    nestingLevel = 1;

    //
    // if this is the async worker thread then what we think is hGopherSession
    // is really the find handle object. Get the handles in the right variables
    //

    if (lpThreadInfo->IsAsyncWorkerThread
    && (lpThreadInfo->NestedRequests == 1)) {
        hObject = hGopherSession;
        error = MapHandleToAddress(hObject, (LPVOID *)&hObjectMapped, FALSE);
        if ((error != ERROR_SUCCESS) && (hObjectMapped == NULL)) {
            goto quit;
        }
        findHandle = hObjectMapped;
        hConnectMapped = ((FTP_FIND_HANDLE_OBJECT *)findHandle)->GetParent();
    } else {

        //
        // map the handle
        //

        hObject = hGopherSession;
        error = MapHandleToAddress(hGopherSession, (LPVOID *)&hObjectMapped, FALSE);
        if ((error != ERROR_SUCCESS) && (hObjectMapped == NULL)) {
            goto quit;
        }
        hConnectMapped = hObjectMapped;
        findHandle = NULL;
    }

    //
    // set the context info and clear the error variables
    //

    _InternetSetObjectHandle(lpThreadInfo, hObject, hObjectMapped);
    _InternetSetContext(lpThreadInfo, dwContext);
    _InternetClearLastError(lpThreadInfo);

    //
    // quit now if the handle is invalid
    //

    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // validate parameters - check handle and discover sync/async and
    // local/remote
    //

    BOOL isLocal, isAsync;

    error = RIsHandleLocal(hConnectMapped,
                           &isLocal,
                           &isAsync,
                           TypeGopherConnectHandle
                           );
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // skip rest of validation if we're in the async worker thread context -
    // we already did this
    //

    if (!lpThreadInfo->IsAsyncWorkerThread
    || (lpThreadInfo->NestedRequests > 1)) {
        if ((ARGUMENT_PRESENT(lpBuffer)
            && IsBadWritePtr(lpBuffer, sizeof(GOPHER_FIND_DATA)))
        || (ARGUMENT_PRESENT(lpszSearchString)

            //
            // BUGBUG - limit on search string?
            //

            && IsBadStringPtr(lpszSearchString,
                              MAX_GOPHER_SEARCH_STRING_LENGTH))) {

            error = ERROR_INVALID_PARAMETER;
            goto quit;
        }

        if (ARGUMENT_PRESENT(lpszLocator)) {

            //
            // BUGBUG - limit on locator?
            //

            if (IsValidLocator(lpszLocator, MAX_GOPHER_LOCATOR_LENGTH)) {

                DWORD locatorType;

                locatorType = GopherCharToType(*lpszLocator);

                //
                // we only allow directories and index/cso servers to be searched
                //

                if (  !IS_GOPHER_DIRECTORY(locatorType)
                   && !IS_GOPHER_SEARCH_SERVER(locatorType)) {
                    error = ERROR_GOPHER_INCORRECT_LOCATOR_TYPE;
                    goto quit;
                }

                //
                // if this is an search server then there must be search strings
                // - the server will only tell us that we need to supply them,
                // so no point in going to the expense of sending a request just
                // to get this response
                //

                if (  IS_GOPHER_SEARCH_SERVER (locatorType)
                   && (  !ARGUMENT_PRESENT(lpszSearchString)
                      || (*lpszSearchString == '\0'))) {

                    error = ERROR_INVALID_PARAMETER;
                    goto quit;
                }
            } else {
                error = ERROR_GOPHER_INVALID_LOCATOR;
                goto quit;
            }
        }

        //
        // create the handle object now. This can be used to cancel the async
        // operation, or the sync operation if InternetCloseHandle() is called
        // from a different thread
        //

        INET_ASSERT(findHandle == NULL);

        error = RMakeGfrFindObjectHandle(hConnectMapped,
                                         &findHandle,
                                         (CLOSE_HANDLE_FUNC)wGopherFindClose,
                                         dwContext
                                         );
        if (error != ERROR_SUCCESS) {
            goto quit;
        }

        //
        // this new handle will be used in callbacks
        //

        _InternetSetObjectHandle(lpThreadInfo,
                                 ((HANDLE_OBJECT *)findHandle)->GetPseudoHandle(),
                                 findHandle
                                 );

        //
        // get the OFFLINE flag whether set globally (@ InternetOpen() level) or
        // locally (for this function)
        //

        if ((((INTERNET_CONNECT_HANDLE_OBJECT *)findHandle)->GetInternetOpenFlags()
             | dwFlags) & INTERNET_FLAG_OFFLINE) {
            dwFlags |= INTERNET_FLAG_OFFLINE;
        }

try_again:

        //
        // check to see if the data is in the cache. Do it here so that we don't
        // waste any time going async if we already have the data locally
        //

        //
        // can't do it for default locator
        //

        if ((lpszLocator != NULL)
        && FGopherBeginCacheReadProcessing(findHandle,
                                           lpszLocator,
                                           lpszSearchString,
                                           dwFlags,
                                           dwContext,
                                           ((INTERNET_CONNECT_HANDLE_OBJECT *)hConnectMapped)->IsHtmlFind()
                                           )) {

            error = ERROR_SUCCESS;

            if (lpBuffer) {

                DWORD dwBytes = sizeof(GOPHER_FIND_DATA);

                error = ((INTERNET_CONNECT_HANDLE_OBJECT *)findHandle)->ReadCache(
                            (LPBYTE)lpBuffer,
                            sizeof(GOPHER_FIND_DATA),
                            &dwBytes
                            );
            }
            if (error == ERROR_SUCCESS) {
                goto quit;
            } else {
                ((INTERNET_CONNECT_HANDLE_OBJECT *)findHandle)->EndCacheRetrieval();
            }
        }

        if (dwFlags & INTERNET_FLAG_OFFLINE) {

            //
            // we are supposed to be in offline mode
            // if we are not reading from the cache, let us bailout
            //

            if (!((INTERNET_CONNECT_HANDLE_OBJECT *)findHandle)->IsCacheReadInProgress()) {
                error = ERROR_PATH_NOT_FOUND;
                goto quit;
            }
        }
    }

    //
    // if the handle was created for async I/O AND there is a non-zero context
    // value AND we are not in the context of an async worker thread then queue
    // the request
    //

    if (isAsync
    && (dwContext != INTERNET_NO_CALLBACK)
    && !lpThreadInfo->IsAsyncWorkerThread) {

        // MakeAsyncRequest
        CFsm_GopherFindFirstFile * pFsm;

        pFsm = new CFsm_GopherFindFirstFile(((HANDLE_OBJECT *)findHandle)->GetPseudoHandle(),
                                             dwContext,
                                             lpszLocator,
                                             lpszSearchString,
                                             lpBuffer,
                                             dwFlags
                                             );

        if (pFsm != NULL &&
            pFsm->GetError() == ERROR_SUCCESS)
        {
            error = pFsm->QueueWorkItem();
            if ( error == ERROR_IO_PENDING ) {
                fDeref = FALSE;
            }
        }
        else
        {
            error = ERROR_NOT_ENOUGH_MEMORY;

            if ( pFsm )
            {
                error = pFsm->GetError();
                delete pFsm;
                pFsm = NULL;
            }
        }

        //
        // if we're here then ERROR_SUCCESS cannot have been returned from
        // the above calls
        //

        INET_ASSERT(error != ERROR_SUCCESS);


        DEBUG_PRINT(FTP,
                    INFO,
                    ("processing request asynchronously: error = %d\n",
                    error
                    ));

        goto quit;
    }

    //
    // make the request
    //

    char defaultLocator[MAX_GOPHER_LOCATOR_LENGTH * 2];

    //
    // if we were given a NULL locator then create a locator to access
    // the default directory at the configured default server.
    //
    // N.B. This only gives us gopher access (i.e. not gopher+)
    //

    if (!ARGUMENT_PRESENT(lpszLocator)) {

        BOOL ok;
        DWORD defaultLocatorLength;

        defaultLocatorLength = sizeof(defaultLocator);
        ok = GopherCreateLocator(
            ((INTERNET_CONNECT_HANDLE_OBJECT *)hConnectMapped)->GetHostName(),
            ((INTERNET_CONNECT_HANDLE_OBJECT *)hConnectMapped)->GetHostPort(),
            NULL,
            NULL,
            GOPHER_TYPE_DIRECTORY,
            defaultLocator,
            &defaultLocatorLength
            );
        if (ok) {
            lpszLocator = defaultLocator;
        } else {

            //
            // set error to ERROR_SUCCESS so that cleanup doesn't set it
            // again (already set by GopherCreateLocator)
            //

            error = ERROR_SUCCESS;
            goto quit;
        }
    }

    HINTERNET protocolFindHandle;

    error = wGopherFindFirst(lpszLocator,
                             lpszSearchString,
                             lpBuffer,
                             &protocolFindHandle
                             );
    if (error == ERROR_SUCCESS) {
        ((GOPHER_FIND_HANDLE_OBJECT *)findHandle)->SetFindHandle(protocolFindHandle);

        //
        // if we succeeded in getting the data, add it to the cache. Don't worry
        // about errors if cache write fails
        //

        if (FGopherBeginCacheWriteProcessing(
            findHandle,
            lpszLocator,
            lpszSearchString,
            0,
            dwContext,
            ((INTERNET_CONNECT_HANDLE_OBJECT *)hConnectMapped)->IsHtmlFind())) {

            if (lpBuffer != NULL) {

                DWORD dwBytes = sizeof(GOPHER_FIND_DATA);
                DWORD errorCache;

                errorCache = ((INTERNET_CONNECT_HANDLE_OBJECT *)findHandle)->WriteCache(
                    (LPBYTE)lpBuffer,
                    sizeof(GOPHER_FIND_DATA)
                    );
                if (errorCache != ERROR_SUCCESS) {
                    InbGopherLocalEndCacheWrite(findHandle,
                                                NULL,
                                                (errorCache == ERROR_NO_MORE_FILES)
                                                );
                }
            }
        }
    } else if (IsOffline() && !(dwFlags & INTERNET_FLAG_OFFLINE)) {

        //
        // if we failed because we went offline before retrieving it from the
        // cache, then try from the cache again
        //

        dwFlags |= INTERNET_FLAG_OFFLINE;
        goto try_again;
    }

quit:

    _InternetDecNestingCount(nestingLevel);

done:

    //
    // if we got an error then set this thread's error variable and return
    // NULL. The app must call GetLastError()
    //

    if (error != ERROR_SUCCESS) {

        DEBUG_ERROR(API, error);

        //
        // if we are not pending an async request but we created a handle object
        // then close it
        //

        if ((error != ERROR_IO_PENDING) && (findHandle != NULL)) {
            InternetCloseHandle(((HANDLE_OBJECT *)findHandle)->GetPseudoHandle());
        }

        //
        // error situation, or request is being processed asynchronously: return
        // a NULL handle
        //

        findHandle = NULL;
    } else {

        //
        // success - return generated pseudo-handle
        //

        findHandle = ((HANDLE_OBJECT *)findHandle)->GetPseudoHandle();
    }

    if ((hConnectMapped != NULL) && fDeref) {
        DereferenceObject((LPVOID)hConnectMapped);
    }

    if ( error != ERROR_SUCCESS ) {
        SetLastError(error);
    }

    DEBUG_LEAVE_API(findHandle);

    return findHandle;
}


BOOL
GopherFindNextA(
    IN HINTERNET hFind,
    OUT LPGOPHER_FIND_DATA lpBuffer
    )

/*++

Routine Description:

    Continues a search created by GopherFindFirstFile(). The same search
    criteria as specfied in GopherFindFirstFile() will be applied

    Assumes:    1. We are being called from InternetFindNextFile() which has
                   already validated the parameters, set the thread variables,
                   and cleared the object last error info

Arguments:

    hFind       - search handle created by call to GopherFindFirstFile()

    lpBuffer    - pointer to user-allocated buffer in which to return info

Return Value:

    BOOL
        TRUE    - Information has been returned in lpBuffer

        FALSE   - Use GetLastError()/InternetGetLastResponseInfo() to get nore
                  information

--*/

{
    DEBUG_ENTER((DBG_GOPHER,
                 Bool,
                 "GopherFindNextA",
                 "%#x, %#x",
                 hFind,
                 lpBuffer
                 ));

    INET_ASSERT(GlobalDataInitialized);

    DWORD error;

    //
    // find path from internet handle
    //

    BOOL isLocal;
    BOOL isAsync, fIsHtml = FALSE;

    error = RIsHandleLocal(hFind,
                           &isLocal,
                           &isAsync,
                           TypeGopherFindHandle
                           );
    if (error != ERROR_SUCCESS) {

        //
        // if the handle is actually a HTML gopher find handle, then we allow
        // the operation. Note: we can do this because GopherFindNext() is not
        // exported, so a rogue app cannot call this function after opening the
        // handle via InternetOpenUrl()
        //

        error = RIsHandleLocal(hFind,
                               &isLocal,
                               &isAsync,
                               TypeGopherFindHandleHtml
                               );
        if (error != ERROR_SUCCESS) {
            goto quit;
        }
        fIsHtml = TRUE;
    }

    INET_ASSERT(error == ERROR_SUCCESS);

    if (((INTERNET_CONNECT_HANDLE_OBJECT *)hFind)->IsCacheReadInProgress()) {
        DWORD   dwLen = sizeof(GOPHER_FIND_DATA);
        error = ((INTERNET_CONNECT_HANDLE_OBJECT *)hFind)->ReadCache((LPBYTE)lpBuffer,
                                                            dwLen,
                                                            &dwLen);
        if ((error == ERROR_SUCCESS) && !dwLen) {
            error = ERROR_NO_MORE_FILES;
        }
        goto quit;
    }

    HINTERNET localHandle;

    error = RGetLocalHandle(hFind, &localHandle);
    if (error == ERROR_SUCCESS) {
        error = wGopherFindNext(localHandle, lpBuffer);
    }

    DWORD errorCache;

    errorCache = error;

    if (error == ERROR_SUCCESS) {
        if (((INTERNET_CONNECT_HANDLE_OBJECT *)hFind)->IsCacheWriteInProgress()) {
            if (!fIsHtml) {
                errorCache = ((INTERNET_CONNECT_HANDLE_OBJECT *)hFind)->WriteCache((LPBYTE)lpBuffer,
                                                                sizeof(GOPHER_FIND_DATA));
            }
        }

    }
    if (errorCache != ERROR_SUCCESS) {
        if (!fIsHtml) {
            InbGopherLocalEndCacheWrite(hFind,
                                        NULL,
                                        (errorCache == ERROR_NO_MORE_FILES)
                                        );
        }
    }

quit:

    BOOL success;

    if (error == ERROR_SUCCESS) {
        success = TRUE;
    } else {

        DEBUG_ERROR(API, error);

        success = FALSE;
    }

    DEBUG_LEAVE(success);

    SetLastError(error);

    return success;
}


INTERNETAPI
HINTERNET
WINAPI
GopherOpenFileA(
    IN HINTERNET hGopherSession,
    IN LPCSTR lpszLocator,
    IN LPCSTR lpszView OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    'Opens' a file at a gopher server. Right now this means transferring the
    file locally, keeping it in a buffer

Arguments:

    hGopherSession  - defines where to go for the file - gateway or internet

    lpszLocator     - descriptor of file to get

    lpszView        - optional type of file to read as MIME content-type

    dwFlags         - open options

    dwContext       - app-supplied context value for use in call-backs

Return Value:

    HINTERNET
        Success - valid handle value

        Failure - NULL
                    Use GetLastError()/InternetGetLastResponseInfo() to get
                    more information

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Handle,
                     "GopherOpenFileA",
                     "%#x, %q, %q, %#x, %#x",
                     hGopherSession,
                     lpszLocator,
                     lpszView,
                     dwFlags,
                     dwContext
                     ));

    HINTERNET fileHandle = NULL;
    HINTERNET hConnectMapped = NULL;
    HINTERNET hObject;
    HINTERNET hObjectMapped = NULL;

    LPINTERNET_THREAD_INFO lpThreadInfo;
    DWORD error;
    DWORD nestingLevel = 0;
    BOOL fDeref = TRUE;

    if (!GlobalDataInitialized) {
        error = ERROR_INTERNET_NOT_INITIALIZED;
        goto done;
    }

    //
    // need the per-thread info block
    //

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto done;
    }

    _InternetIncNestingCount();
    nestingLevel = 1;

    //
    // if this is the async worker thread then what we think is hGopherSession
    // is really the file handle object. Get the handles in the right variables
    //

    if (lpThreadInfo->IsAsyncWorkerThread
    && (lpThreadInfo->NestedRequests == 1)) {
        hObject = hGopherSession;
        error = MapHandleToAddress(hObject, (LPVOID *)&hObjectMapped, FALSE);
        if ((error != ERROR_SUCCESS) && (hObjectMapped == NULL)) {
            goto quit;
        }
        fileHandle = hObjectMapped;
        hConnectMapped = ((FTP_FIND_HANDLE_OBJECT *)fileHandle)->GetParent();
    } else {

        //
        // map the handle
        //

        hObject = hGopherSession;
        error = MapHandleToAddress(hObject, (LPVOID *)&hObjectMapped, FALSE);
        if ((error != ERROR_SUCCESS) && (hObjectMapped == NULL)) {
            goto quit;
        }
        hConnectMapped = hObjectMapped;
    }

    //
    // handle must be valid type
    //

    BOOL isLocal;
    BOOL isAsync;

    error = RIsHandleLocal(hConnectMapped,
                           &isLocal,
                           &isAsync,
                           TypeGopherConnectHandle
                           );
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // set the context info & clear the error variables
    //

    _InternetSetObjectHandle(lpThreadInfo, hObject, hObjectMapped);
    _InternetSetContext(lpThreadInfo, dwContext);
    _InternetClearLastError(lpThreadInfo);

    //
    // if this is an async request and we're in the context of an async worker
    // thread then skip the rest of parameter validation - it was already done
    // when the request was originally queued
    //

    if (isAsync
    && lpThreadInfo->IsAsyncWorkerThread
    && (lpThreadInfo->NestedRequests == 1)) {
        goto synchronous_path;
    }

    //
    // validate parameters - locator must identify a file (and be a valid
    // locator), and the flags parameter cannot contain any undefined flags.
    // lpszView must be NULL or valid string
    //

    if (dwFlags & ~INTERNET_FLAGS_MASK) {
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }

    if (ARGUMENT_PRESENT(lpszView)) {

        int len;

        __try {
            len = lstrlen(lpszView);
            error = ERROR_SUCCESS;
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            error = ERROR_INVALID_PARAMETER;
        }
        ENDEXCEPT
        if (error != ERROR_SUCCESS) {
            goto quit;
        }
    }

    error = TestLocatorType(lpszLocator, GOPHER_FILE_MASK);
    if (error != ERROR_SUCCESS) {

        //
        // TestLocatorType uses ERROR_INVALID_FUNCTION to mean Locator is not
        // the specified type. Map this to ERROR_SUCCESS and NOT SUCCESS
        //

        if (error == ERROR_INVALID_FUNCTION) {
            error = ERROR_GOPHER_NOT_FILE;
        }
        goto quit;
    }

    //
    // create the handle object now. This can be used to cancel the async
    // operation, or the sync operation if InternetCloseHandle() is called
    // from a different thread
    //

    fileHandle = NULL;
    error = RMakeGfrFileObjectHandle(hConnectMapped,
                                     &fileHandle,
                                     (CLOSE_HANDLE_FUNC)wGopherCloseHandle,
                                     dwContext
                                     );
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // this new handle will be used in callbacks
    //

    _InternetSetObjectHandle(lpThreadInfo,
                             ((HANDLE_OBJECT *)fileHandle)->GetPseudoHandle(),
                             fileHandle
                             );

    //
    // get the OFFLINE flag whether set globally (@ InternetOpen() level) or
    // locally (for this function)
    //

    if ((((GOPHER_FILE_HANDLE_OBJECT *)fileHandle)->GetInternetOpenFlags()
         | dwFlags) & INTERNET_FLAG_OFFLINE) {
        dwFlags |= INTERNET_FLAG_OFFLINE;
    }

try_again:

    //
    // check to see if the data is in the cache
    //

    if (FGopherBeginCacheReadProcessing(fileHandle,
                                        lpszLocator,
                                        lpszView,
                                        dwFlags,
                                        dwContext,
                                        FALSE)) {
        error = ERROR_SUCCESS;
        goto quit;
    } else if (dwFlags & INTERNET_FLAG_OFFLINE) {

        //
        // we are supposed to be in offline mode, let us bailout
        //

        error = ERROR_FILE_NOT_FOUND;
        goto quit;
    }

    //
    // if we're here then we know we're not in the context of an async worker
    // thread, so if the request is async, we queue it and get out
    //

    if (!lpThreadInfo->IsAsyncWorkerThread
    && isAsync
    && (dwContext != INTERNET_NO_CALLBACK))
    {
        // MakeAsyncRequest
        CFsm_GopherOpenFile * pFsm;

        pFsm = new CFsm_GopherOpenFile(((HANDLE_OBJECT *)fileHandle)->GetPseudoHandle(),
                                        dwContext,
                                        lpszLocator,
                                        lpszView,
                                        dwFlags
                                        );

        if (pFsm != NULL &&
            pFsm->GetError() == ERROR_SUCCESS)
        {
            error = pFsm->QueueWorkItem();
            if ( error == ERROR_IO_PENDING ) {
                fDeref = FALSE;
            }
        }
        else
        {
            error = ERROR_NOT_ENOUGH_MEMORY;

            if ( pFsm )
            {
                error = pFsm->GetError();
                delete pFsm;
                pFsm = NULL;
            }
        }

        //
        // if we're here then ERROR_SUCCESS cannot have been returned from
        // the above calls
        //

        INET_ASSERT(error != ERROR_SUCCESS);


        DEBUG_PRINT(FTP,
                    INFO,
                    ("processing request asynchronously: error = %d\n",
                    error
                    ));

        goto quit;
    }

synchronous_path:

    //
    // local call, call the worker directly.
    //

    HINTERNET protocolFileHandle;

    error = wGopherOpenFile(lpszLocator,
                            lpszView,
                            &protocolFileHandle
                            );
    if (error == ERROR_SUCCESS) {
        ((GOPHER_FILE_HANDLE_OBJECT *)fileHandle)->SetFileHandle(protocolFileHandle);

        //
        // don't worry about errors if cache write does not begin
        //

        FGopherBeginCacheWriteProcessing(fileHandle,
                                         lpszLocator,
                                         lpszView,
                                         dwFlags,
                                         dwContext,
                                         FALSE
                                         );
    } else if (IsOffline() && !(dwFlags & INTERNET_FLAG_OFFLINE)) {

        //
        // if we failed because we went offline before retrieving it from the
        // cache, then try from the cache again
        //

        dwFlags |= INTERNET_FLAG_OFFLINE;
        goto try_again;
    }

quit:

    _InternetDecNestingCount(nestingLevel);

done:

    if (error != ERROR_SUCCESS) {

        DEBUG_ERROR(API, error);

        SetLastError(error);

        //
        // if we are not pending an async request but we created a handle object
        // then close it
        //

        if ((error != ERROR_IO_PENDING) && (fileHandle != NULL)) {
            InternetCloseHandle(((HANDLE_OBJECT *)fileHandle)->GetPseudoHandle());
        }

        //
        // error situation, or request is being processed asynchronously: return
        // a NULL handle
        //

        fileHandle = NULL;
    } else {

        //
        // success - return generated pseudo-handle
        //

        fileHandle = ((HANDLE_OBJECT *)fileHandle)->GetPseudoHandle();
    }

    if ((hConnectMapped != NULL) && fDeref ) {
        DereferenceObject((LPVOID)hConnectMapped);
    }

    DEBUG_LEAVE_API(fileHandle);

    return fileHandle;
}


BOOL
GopherReadFile(
    IN HINTERNET hFile,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead
    )

/*++

Routine Description:

    Reads from a file opened by GopherOpenFile into the caller's buffer. The
    number of bytes returned is the smaller of dwNumberOfBytesToRead and the
    number of bytes between the current file pointer and the end of the file

    Assumes:    1. We are being called from InternetReadFile() which has
                   already validated the parameters, handled the zero byte
                   read case, set the thread variables, and cleared the object
                   last error info

Arguments:

    hFile                   - file handle created by GopherOpenFile

    lpBuffer                - pointer to caller's buffer

    dwNumberOfBytesToRead   - pointer to length of Buffer

    lpdwNumberOfBytesRead   - number of bytes copied into Buffer

Return Value:

    BOOL
        TRUE    - lpdwNumberOfBytesRead contains amount of data written to
                  lpBuffer

        FALSE   - use GetLastError()/InternetGetLastResponseInfo() to get more
                  info

--*/

{
    DEBUG_ENTER((DBG_GOPHER,
                 Bool,
                 "GopherReadFile",
                 "%#x, %#x, %d, %#x",
                 hFile,
                 lpBuffer,
                 dwNumberOfBytesToRead,
                 lpdwNumberOfBytesRead
                 ));

    INET_ASSERT(GlobalDataInitialized);

    DWORD error;

    //
    // find path from file handle
    //

    BOOL isLocal;
    BOOL isAsync;

    error = RIsHandleLocal(hFile,
                           &isLocal,
                           &isAsync,
                           TypeGopherFileHandle
                           );
    if (error != ERROR_SUCCESS) {
        goto quit;
    }
    if (((GOPHER_FILE_HANDLE_OBJECT *)hFile)->IsCacheReadInProgress()) {
        error = ((GOPHER_FILE_HANDLE_OBJECT *)hFile)->ReadCache((LPBYTE)lpBuffer,
                                                  dwNumberOfBytesToRead,
                                                  lpdwNumberOfBytesRead
                                                  );
        if (!*lpdwNumberOfBytesRead || (error != ERROR_SUCCESS)) {
            // Don't end cache retrieval. So any extraneous reads
            // continue down this path. bug #9086
//            ((GOPHER_FILE_HANDLE_OBJECT *)hFile)->EndCacheRetrieval();
        }

        // quit whether we succeed or we fail

        goto quit;
    }

    HINTERNET localHandle;

    error = RGetLocalHandle(hFile, &localHandle);
    if (error == ERROR_SUCCESS) {
        error = wGopherReadFile(localHandle,
                                (LPBYTE)lpBuffer,
                                dwNumberOfBytesToRead,
                                lpdwNumberOfBytesRead
                                );
    }

    if (error == ERROR_SUCCESS) {
        DWORD   errorCache;
        if (((GOPHER_FILE_HANDLE_OBJECT *)hFile)->IsCacheWriteInProgress()) {

            if(!*lpdwNumberOfBytesRead) {

                DEBUG_PRINT(GOPHER,
                            INFO,
                            ("Cache write complete\r\n"
                            ));

                errorCache = InbGopherLocalEndCacheWrite(hFile, NULL, TRUE);

                INET_ASSERT(error == ERROR_SUCCESS);

                goto quit;
            }

            INET_ASSERT(((GOPHER_FILE_HANDLE_OBJECT *)hFile)->IsCacheReadInProgress()==FALSE);

            if (((GOPHER_FILE_HANDLE_OBJECT *)hFile)->WriteCache((LPBYTE)lpBuffer,
                                                   *lpdwNumberOfBytesRead
                                                   ) != ERROR_SUCCESS) {

                DEBUG_PRINT(GOPHER,
                            ERROR,
                            ("Error in Cache write\n"
                            ));

                errorCache = InbGopherLocalEndCacheWrite(hFile, NULL, FALSE);

                INET_ASSERT(error == ERROR_SUCCESS);

            }
        }

    }

quit:

    BOOL success;

    if (error == ERROR_SUCCESS) {
        success = TRUE;
    } else {

        DEBUG_ERROR(API, error);

        SetLastError(error);
        success = FALSE;
    }

    DEBUG_LEAVE(success);

    return success;
}


INTERNETAPI
BOOL
WINAPI
GopherGetAttributeA(
    IN HINTERNET hGopherSession,
    IN LPCSTR lpszLocator,
    IN LPCSTR lpszAttributeName OPTIONAL,
    OUT LPBYTE lpBuffer,
    IN DWORD dwBufferLength,
    OUT LPDWORD lpdwCharactersReturned,
    IN GOPHER_ATTRIBUTE_ENUMERATOR lpfnEnumerator OPTIONAL,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    Gets an attribute from a server

    BUGBUG - it should be possible for a caller to specify e.g. "+VIEWS+ABSTRACT"
             (according to gopher plus documentation) and get both types back

Arguments:

    hGopherSession          - identifies the gopher session object

    lpszLocator             - pointer to locator identifying item to get
                              attribute for

    lpszAttributeName       - name of attribute to return. May be NULL,
                              meaning return everything

    lpBuffer                - pointer to buffer where attributes are to be
                              returned

    dwBufferLength          - length of buffer

    lpdwCharactersReturned  - pointer to variable which will receive the
                              number of bytes in lpBuffer on output (if no
                              error occurs)

    lpfnEnumerator          - optional enumerator. If supplied, we return an
                              enumerated series of GOPHER_ATTRIBUTE_TYPE
                              items, else we just return the info in the
                              caller's buffer

    dwContext               - app-supplied context value for use in call-backs

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Use GetLastError()/InternetGetLastResponseInfo() for
                  more information

--*/

{
#if !defined(GOPHER_ATTRIBUTE_SUPPORT)

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;

#else

    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "GopherGetAttributeA",
                     "%#x, %q, %q, %#x, %d, %#x, %#x, %#x",
                     hGopherSession,
                     lpszLocator,
                     lpszAttributeName,
                     lpBuffer,
                     dwBufferLength,
                     lpdwCharactersReturned,
                     lpfnEnumerator,
                     dwContext
                     ));

    DWORD error;
    DWORD gopherType;
    LPSTR gopherPlusInfo;
    DWORD categoryId;
    DWORD attributeId;
    DWORD attributeLength;
    LPBYTE attributeBuffer = NULL;
    LPINTERNET_THREAD_INFO lpThreadInfo;
    DWORD bufferSize;
    HINTERNET hMapped = NULL;
    BOOL fDeref = TRUE;

    if (!GlobalDataInitialized) {
        error = ERROR_INTERNET_NOT_INITIALIZED;
        goto quit;
    }

    //
    // get the per-thread info block
    //

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    //
    // map the handle
    //

    error = MapHandleToAddress(hGopherSession, (LPVOID *)&hMapped, FALSE);
    if ((error != ERROR_SUCCESS) && (hMapped == NULL)) {
        goto quit;
    }

    //
    // set the call context info & clear the error variables
    //

    _InternetSetObjectHandle(lpThreadInfo, hGopherSession, hMapped);
    _InternetSetContext(lpThreadInfo, dwContext);
    _InternetClearLastError(lpThreadInfo);

    //
    // quit now if the handle is invalid
    //

    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // validate the handle on all paths
    //

    BOOL isLocal;
    BOOL isAsync;

    error = RIsHandleLocal(hMapped,
                           &isLocal,
                           &isAsync,
                           TypeGopherConnectHandle
                           );
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // if we're in the async worker thread context then we've already validated
    // the parameters - skip validation and go straight to local/remote path
    //

    if (lpThreadInfo->IsAsyncWorkerThread) {
        goto synchronous_path;
    }

    //
    // validate parameters
    //

    if (!IsValidLocator(lpszLocator, MAX_GOPHER_LOCATOR_LENGTH)

    //
    // we say Buffer must be >= MIN_GOPHER_ATTRIBUTE_LENGTH to give us a chance
    // of returning enumerated attributes
    //

    || (dwBufferLength < MIN_GOPHER_ATTRIBUTE_LENGTH)
    || IsBadWritePtr(lpBuffer, dwBufferLength)
    || IsBadWritePtr(lpdwCharactersReturned, sizeof(*lpdwCharactersReturned))
    || ARGUMENT_PRESENT(lpfnEnumerator) && IsBadCodePtr((FARPROC)lpfnEnumerator)) {
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }

    //
    // crack open the locator. We want the type and to know if its a gopher+
    // locator. Note that we could *really* accept a gopher0 locator if the
    // server identified therein was actually a gopher+ server, but that
    // would require more work just to identify whether we could proceed or
    // not. If the caller doesn't supply a gopher+ locator then 1) the caller
    // supplied their own locator and it won't be painful to recreate a
    // locator of the correct type, or 2) the locator was returned from a
    // previous find first/find next call and therefore we are justified in
    // refusing the request
    //

    CrackLocator(lpszLocator,
                 &gopherType,
                 NULL,  // DisplayString
                 NULL,  // DisplayStringLength
                 NULL,  // SelectorString
                 NULL,  // SelectorStringLength
                 NULL,  // HostName
                 NULL,  // HostNameLength
                 NULL,  // GopherPort
                 &gopherPlusInfo
                 );

    //
    // the locator must identify a gopher+ item - i.e. there must at least be a
    // "\t+" after the gopher server port number in the locator
    //

    if (!gopherPlusInfo) {
        error = ERROR_GOPHER_NOT_GOPHER_PLUS;
        goto quit;
    }

    //
    // and must be a file or directory
    //
    // BUGBUG - I can't find a gopher+ index server. It may be wrong to preclude
    //          this type from getting attributes. Likewise for telnet sessions,
    //          TN3270 sessions, etc. etc.?
    //

    if (!(IS_GOPHER_FILE(gopherType) || IS_GOPHER_DIRECTORY(gopherType))) {
        error = ERROR_GOPHER_INCORRECT_LOCATOR_TYPE;
        goto quit;
    }

    //
    // convert a NULL string to a NULL pointer, indicating that the caller wants
    // all available attributes for the locator
    //

    if (ARGUMENT_PRESENT(lpszAttributeName) && (*lpszAttributeName == '\0')) {
        lpszAttributeName = NULL;
    }

    //
    // we don't allow the app to request the +INFO attribute - we would have to
    // return it as a GOPHER_FIND_DATA which is already catered for by
    // GopherFindFirstFile()/GopherFindNext()
    //

    MapAttributeToIds(lpszAttributeName, &categoryId, &attributeId);
    if (categoryId == GOPHER_CATEGORY_ID_INFO) {
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }

    //
    // queue async request if that's what we're asked to do. We know we're not
    // in the async worker thread context at this point
    //

    INET_ASSERT(!lpThreadInfo->IsAsyncWorkerThread);

    if (isAsync && (dwContext != INTERNET_NO_CALLBACK))
    {
        // MakeAsyncRequest
        CFsm_GopherGetAttribute * pFsm;

        pFsm = new CFsm_GopherGetAttribute(hGopherSession, dwContext, lpszLocator, lpszAttributeName, lpBuffer,
                                            dwBufferLength, lpdwCharactersReturned, lpfnEnumerator );

        if (pFsm != NULL &&
            pFsm->GetError() == ERROR_SUCCESS)
        {
            error = pFsm->QueueWorkItem();
            if ( error == ERROR_IO_PENDING ) {
                fDeref = FALSE;
            }
        }
        else
        {
            error = ERROR_NOT_ENOUGH_MEMORY;

            if ( pFsm )
            {
                error = pFsm->GetError();
                delete pFsm;
                pFsm = NULL;
            }
        }

        //
        // if we're here then ERROR_SUCCESS cannot have been returned from
        // the above calls
        //

        INET_ASSERT(error != ERROR_SUCCESS);


        DEBUG_PRINT(FTP,
                    INFO,
                    ("processing request asynchronously: error = %d\n",
                    error
                    ));

        goto quit;
    }

synchronous_path:

    //
    // we will allocate a buffer to receive the attributes into. Since we
    // won't be growing the buffer, we have to loop until we receive all
    // attribute information, or until an error occurs
    //

    bufferSize = GOPHER_ATTRIBUTE_BUFFER_LENGTH;
    error = ERROR_INSUFFICIENT_BUFFER;
    while (error == ERROR_INSUFFICIENT_BUFFER) {

        //
        // we need to allocate an intermediate buffer to receive the
        // attributes into, for two reasons: 1) we don't return the +INFO
        // attribute, 2) we probably need to filter the returned attributes
        // before returning the requested attributes to the caller
        //

        attributeBuffer = NEW_MEMORY(bufferSize, BYTE);
        if (attributeBuffer != NULL) {
            attributeLength = bufferSize;
            error = wGopherGetAttribute(lpszLocator,
                                        lpszAttributeName,
                                        attributeBuffer,
                                        &attributeLength
                                        );

        } else {

            DEBUG_PRINT(GOPHER,
                        ERROR,
                        ("failed to allocate %d byte attribute buffer\n",
                        GOPHER_ATTRIBUTE_BUFFER_LENGTH
                        ));

            error = ERROR_NOT_ENOUGH_MEMORY;
        }

        //
        // if we have tried up to 64K then something serious has gone wrong.
        // Return an internal error (BUGBUG?)
        //

        if (error == ERROR_INSUFFICIENT_BUFFER) {
            if (bufferSize >= 64 K) {
                error = ERROR_INTERNET_INTERNAL_ERROR;
            } else {
                DEL(attributeBuffer);
                bufferSize += GOPHER_ATTRIBUTE_BUFFER_LENGTH;

                DEBUG_PRINT(GOPHER,
                            WARNING,
                            ("attribute buffer too small: trying %d\n",
                            bufferSize
                            ));

            }
        }
    }
    if (error == ERROR_SUCCESS) {

        //
        // we successfully got a buffer of attributes. Return the attributes
        // requested, either in the user buffer or via the enumerator
        //

        error = GetAttributes(lpfnEnumerator,
                              categoryId,
                              attributeId,
                              lpszAttributeName,
                              (LPSTR)attributeBuffer,
                              attributeLength,
                              lpBuffer,
                              dwBufferLength,
                              lpdwCharactersReturned
                              );
    }
    if (attributeBuffer != NULL) {
        DEL(attributeBuffer);
    }

quit:

    BOOL success;

    if (error != ERROR_SUCCESS) {

        DEBUG_ERROR(API, error);

        SetLastError(error);
        success = FALSE;
    } else {
        success = TRUE;
    }

    if ((hMapped != NULL) && fDeref) {
        DereferenceObject((LPVOID)hMapped);
    }

    DEBUG_LEAVE_API(success);

    return success;

#endif
}

//
//INTERNETAPI
//BOOL
//WINAPI
//GopherSendDataA(
//    IN HINTERNET hGopherSession,
//    IN LPCSTR lpszLocator,
//    IN LPCSTR lpszBuffer,
//    IN DWORD dwNumberOfCharactersToSend,
//    OUT LPDWORD lpdwNumberOfCharactersSent,
//    IN DWORD dwContext
//    )
//
///*++
//
//Routine Description:
//
//    Sends arbitrary text to a gopher server. This function is used primarily
//    for returning the results of a gopher+ ASK item
//
//Arguments:
//
//    hGopherSession              - identifies the gopher session object
//
//    lpszLocator                 - pointer to locator identifying item to send
//                                  data for
//
//    lpszBuffer                  - pointer to buffer containing data to send
//
//    dwNumberOfCharactersToSend  - number of bytes in lpszBuffer
//
//    lpdwNumberOfCharactersSent  - pointer to returned number of bytes sent
//
//    dwContext                   - app-supplied context value for use in call-backs
//
//Return Value:
//
//    BOOL
//        Success - TRUE
//
//        Failure - FALSE. Call GetLastError()/InternetGetLastResponseInfo() for
//                  more information
//
//--*/
//
//{
//    DEBUG_ENTER_API((DBG_API,
//                     Bool,
//                     "GopherSendDataA",
//                     "%#x, %q, %#x, %d, %#x, %#x",
//                     hGopherSession,
//                     lpszLocator,
//                     lpszBuffer,
//                     dwNumberOfCharactersToSend,
//                     lpdwNumberOfCharactersSent,
//                     dwContext
//                     ));
//
//    DEBUG_ERROR(API, ERROR_CALL_NOT_IMPLEMENTED);
//
//    //
//    // this is the handle we are currently working on
//    //
//
//    InternetSetObjectHandle(hGopherSession, hGopherSession);
//
//    //
//    // clear the per-handle object last error variables
//    //
//
//    InternetClearLastError();
//
//    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
//
//    DEBUG_LEAVE_API(FALSE);
//
//    return FALSE;
//}

//
//DWORD
//pGfrGetUrlInfo(
//    IN HANDLE InternetConnectHandle,
//    OUT LPSTR Url
//    )
//
///*++
//
//Routine Description:
//
//    description-of-function.
//
//Arguments:
//
//    InternetConnectHandle   -
//    Url                     -
//
//Return Value:
//
//    DWORD
//
//--*/
//
//{
//    return( ERROR_CALL_NOT_IMPLEMENTED );
//}


DWORD
pGopherGetUrlString(
    IN INTERNET_SCHEME SchemeType,
    IN LPSTR    lpszTargetHost,
    IN LPSTR    lpszCWD,
    IN LPSTR    lpszObjectLocator,
    IN LPSTR    lpszExtension,
    IN DWORD    dwPort,
    OUT LPSTR   *lplpUrlName,
    OUT LPDWORD lpdwUrlLen
    )

/*++

Routine Description:

    Creates an URL for a gopher entity

Arguments:

    SchemeType          - protocol scheme (INTERNET_SCHEME_GOPHER)

    lpszTargetHost      - server

    lpszCWD             - current directory at server

    lpszObjectLocator   - gopher locator string

    lpszExtension       - file extension

    dwPort              - port at server

    lplpUrlName         - pointer to returned URL

    lpdwUrlLen          - pointer to returned URL length

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_PARAMETER

                  ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DWORD error, dwUrlLen, dwExtensionLength;
    BOOL fDoneOnce = FALSE;

    if (lpszExtension) {
        dwExtensionLength = lstrlen(lpszExtension)
                            + lstrlen(GOPHER_EXTENSION_DELIMITER_SZ);
    } else {
        dwExtensionLength = 0;
    }
    *lplpUrlName = NULL;
    *lpdwUrlLen = 128 + dwExtensionLength;  // let us try with less first

    BOOL isDir;

    error = TestLocatorType(lpszObjectLocator, GOPHER_TYPE_DIRECTORY);
    isDir = error == ERROR_SUCCESS;

    error = ERROR_INVALID_PARAMETER;

    do {

        INET_ASSERT(*lplpUrlName == NULL);

        //
        // BUGBUG - LPTR!
        //

        //*lplpUrlName = (LPSTR)ALLOCATE_MEMORY(LPTR, *lpdwUrlLen);

        *lplpUrlName = (LPSTR)ALLOCATE_FIXED_MEMORY(*lpdwUrlLen);
        if (!*lplpUrlName) {
            error = GetLastError();
            goto Cleanup;
        }
        dwUrlLen = *lpdwUrlLen;
#ifdef MAYBE
        if (isDir) {
            **lplpUrlName = '~';
        }
#endif //MAYBE
        error = GopherLocatorToUrl(lpszObjectLocator,
#ifdef MAYBE
                                   isDir ? (*lplpUrlName + 1) : *lplpUrlName,
                                   isDir ? (*lpdwUrlLen - 1) : *lpdwUrlLen,
#endif //MAYBE
                                   *lplpUrlName,
                                   *lpdwUrlLen,
                                   &dwUrlLen
                                   );
        if (error != ERROR_SUCCESS) {

            //
            // BUGBUG - *lplpUrlName cannot be non-NULL?
            //

            if (*lplpUrlName) {
                FREE_MEMORY(*lplpUrlName);
                *lplpUrlName = NULL;
            }
        }
        if (!fDoneOnce) {
            if (error != ERROR_INSUFFICIENT_BUFFER) {
                break;
            } else {
                *lpdwUrlLen = INTERNET_MAX_URL_LENGTH + dwExtensionLength;
                fDoneOnce = TRUE;
            }
        } else {
            break;
        }
    } while (TRUE);

    if (error == ERROR_SUCCESS) {
        if (lpszExtension) {

            INET_ASSERT(dwExtensionLength);

            lstrcat(*lplpUrlName, GOPHER_EXTENSION_DELIMITER_SZ);
            lstrcat(*lplpUrlName, lpszExtension);
        }
    }

Cleanup:

    return error;
}


PRIVATE
BOOL
FGopherBeginCacheReadProcessing(
    IN HINTERNET hGopher,
    IN LPCSTR lpszFileName,
    IN LPCSTR lpszViewType,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext,
    IN BOOL fIsHtmlFind
    )
{
    DEBUG_ENTER((DBG_GOPHER,
                 Bool,
                 "FGopherBeginCacheReadProcessing",
                 "%#x, %q, %q, %#x, %#x, %B",
                 hGopher,
                 lpszFileName,
                 lpszViewType,
                 dwFlags,
                 dwContext,
                 fIsHtmlFind
                 ));

    DWORD dwError = ERROR_SUCCESS;
    URLGEN_FUNC fn = pGopherGetUrlString;
    LPCACHE_ENTRY_INFO lpCEI = NULL;
    BOOL ok;

    if (((INTERNET_CONNECT_HANDLE_OBJECT *)hGopher)->IsCacheReadInProgress()) {
        goto quit;
    }

    if (!(dwFlags & INTERNET_FLAG_NO_CACHE_WRITE)) {

        //
        // if the object name is not set then all cache methods fail
        //

        ((INTERNET_CONNECT_HANDLE_OBJECT *)hGopher)->SetObjectName(
                                                        (LPSTR)lpszFileName,
                                                        (LPSTR)lpszViewType,
                                                        &fn
                                                        );

        //
        // set the cache flags like RELOAD etc.
        //

        ((INTERNET_CONNECT_HANDLE_OBJECT *)hGopher)->SetCacheFlags(dwFlags);
    } else {

        //
        // set flags to disable both read and write
        //

        ((INTERNET_CONNECT_HANDLE_OBJECT *)hGopher)->SetCacheFlags(
                                                        INTERNET_FLAG_DONT_CACHE
                                                        | INTERNET_FLAG_RELOAD
                                                        );
    }

    if (!FGopherCanReadFromCache(hGopher)) {
        dwError = !ERROR_SUCCESS;
        goto quit;
    }

    DEBUG_PRINT(GOPHER,
                INFO,
                ("Checking in the cache\n"
                ));

    dwError = ((INTERNET_CONNECT_HANDLE_OBJECT *)hGopher)->BeginCacheRetrieval(&lpCEI);
    if (dwError == ERROR_SUCCESS) {

        //
        // found it in the cache
        //

        DEBUG_PRINT(GOPHER,
                    INFO,
                    ("Found in the cache\n"
                    ));

        if (IsOffline()
        || (!FIsGopherExpired(hGopher, lpCEI)
            && ((fIsHtmlFind && lpCEI->lpszFileExtension)
            || (!fIsHtmlFind && !lpCEI->lpszFileExtension)))) {

            dwError = ERROR_SUCCESS;
            ((INTERNET_CONNECT_HANDLE_OBJECT *)hGopher)->SetFromCache();
        } else {

            DEBUG_PRINT(GOPHER,
                        INFO,
                        ("Expired or invalid datatype\n"
                        ));

            dwError = ((INTERNET_CONNECT_HANDLE_OBJECT *)hGopher)->EndCacheRetrieval();

            INET_ASSERT(dwError == ERROR_SUCCESS);

            dwError = ERROR_FILE_NOT_FOUND;
        }
    }

    if (lpCEI != NULL) {
        lpCEI = (LPCACHE_ENTRY_INFO)FREE_MEMORY(lpCEI);

        INET_ASSERT(lpCEI == NULL);

    }

quit:

    ok = (dwError == ERROR_SUCCESS);

    DEBUG_LEAVE(ok);

    return ok;
}


PRIVATE
BOOL
FGopherCanReadFromCache(
    HINTERNET hGopher
    )
{
    DEBUG_ENTER((DBG_GOPHER,
                 Bool,
                 "FGopherCanReadFromCache",
                 "%#x",
                 hGopher
                 ));

    DWORD dwOpenFlags = ((INTERNET_CONNECT_HANDLE_OBJECT *)hGopher)->GetInternetOpenFlags();
    DWORD dwCacheFlags = ((INTERNET_CONNECT_HANDLE_OBJECT *)hGopher)->GetCacheFlags();
    BOOL ok = TRUE;

    //
    // in disconnected state client always reads
    //

    if (((dwOpenFlags | dwCacheFlags) & INTERNET_FLAG_OFFLINE) || IsOffline()) {

        DEBUG_PRINT(GOPHER,
                    INFO,
                    ("open flags=%#x, cache flags=%#x, offline=%B\n",
                    dwOpenFlags,
                    dwCacheFlags,
                    IsOffline()
                    ));

    } else if (dwCacheFlags & (INTERNET_FLAG_RELOAD | INTERNET_FLAG_RESYNCHRONIZE)) {

        //
        // if we are asked to reload data, it is not OK to read from cache
        //

        DEBUG_PRINT(GOPHER,
                    INFO,
                    ("no cache option\n"
                    ));

        ok = FALSE;
    }

    DEBUG_LEAVE(ok);

    return ok;
}


PRIVATE
BOOL
FGopherBeginCacheWriteProcessing(
    IN HINTERNET hGopher,
    IN LPCSTR lpszFileName,
    IN LPCSTR lpszViewType,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext,
    IN BOOL fIsHtmlFind
    )
{
    DWORD   dwError = ERROR_INVALID_FUNCTION;
    URLGEN_FUNC fn = pGopherGetUrlString;
    LPSTR   lpszFileExtension, lpTemp;
    char cExt[DEFAULT_MAX_EXTENSION_LENGTH + sizeof("%09%09%2B") + 1];
    DWORD dwBuffLen;


    if (!((INTERNET_CONNECT_HANDLE_OBJECT *)hGopher)->IsCacheReadInProgress()) {

        //
        // we are not reading from the cache
        // Let us ask a routine whether we should cache this
        // stuff or not
        //

        if (FGopherCanWriteToCache(hGopher)) {

            //
            // if the object name is not set then all cache methods fail
            //

            ((INTERNET_CONNECT_HANDLE_OBJECT *)hGopher)->SetObjectName(
                                                            (LPSTR)lpszFileName,
                                                            (LPSTR)lpszViewType,
                                                            &fn
                                                            );

            //
            // set the cache flags
            //

            ((INTERNET_CONNECT_HANDLE_OBJECT *)hGopher)->SetCacheFlags(dwFlags);

            //
            // he says we can cache it.
            //

            DEBUG_PRINT(GOPHER,
                        INFO,
                        ("Starting cache write\n"
                        ));

            if (!fIsHtmlFind) {
                dwBuffLen = sizeof(cExt);

                lpszFileExtension = GetFileExtensionFromUrl(
                    ((INTERNET_CONNECT_HANDLE_OBJECT *)hGopher)->GetURL(),
                    &dwBuffLen
                    );

                if (lpszFileExtension != NULL) {

                    //
                    // strip off gopher+ strings
                    //

                    for (lpTemp = lpszFileExtension;
                        *lpTemp != 0 && * lpTemp != '%';
                        ++lpTemp) {

                        //
                        // EMPTY LOOP
                        //

                    }

                    dwBuffLen = (DWORD) PtrDifference(lpTemp, lpszFileExtension);
                    memcpy(cExt, lpszFileExtension, dwBuffLen);
                    cExt[dwBuffLen] = '\0';
                    lpszFileExtension = cExt;
                }
            }
            else {
                //generate htm extension
                strcpy(cExt, "htm");
                lpszFileExtension = cExt;
            }

            dwError = ((INTERNET_CONNECT_HANDLE_OBJECT *)hGopher)->BeginCacheWrite(
                0,
                lpszFileExtension
                );

            if (dwError != ERROR_SUCCESS) {

                DEBUG_PRINT(GOPHER,
                            ERROR,
                            ("Error in BeginCacheWrite %ld\n",
                            dwError
                            ));

            }
        }
    }
    return (dwError == ERROR_SUCCESS);
}


PRIVATE
BOOL
FGopherCanWriteToCache(
    HINTERNET   hGopher
    )
{
    if (((INTERNET_CONNECT_HANDLE_OBJECT *)hGopher)->
    GetCacheFlags() & INTERNET_FLAG_DONT_CACHE) {
        return (FALSE);
    }

    return (TRUE);
}


DWORD
InbGopherLocalEndCacheWrite(
    IN HINTERNET hGopher,
    IN LPSTR    lpszFileExtension,
    IN BOOL fNormal
    )
{

    FILETIME ftLastModTime, ftExpiryTime, ftPostCheck;
    DWORD   dwEntryType;

    if (((INTERNET_CONNECT_HANDLE_OBJECT *)hGopher)->IsCacheWriteInProgress()) {

        ftLastModTime.dwLowDateTime =
        ftLastModTime.dwHighDateTime = 0;

        ftExpiryTime.dwLowDateTime =
        ftExpiryTime.dwHighDateTime = 0;

        ftPostCheck.dwLowDateTime =
        ftPostCheck.dwHighDateTime = 0;


        dwEntryType = (!fNormal)?0xffffffff:
                        ((((INTERNET_CONNECT_HANDLE_OBJECT *)hGopher)->

                            GetCacheFlags() & INTERNET_FLAG_MAKE_PERSISTENT)
                                ? STICKY_CACHE_ENTRY:0
                        );

         DEBUG_PRINT(GOPHER,
                     INFO,
                     ("Cache write EntryType = %x\r\n", dwEntryType
                     ));

        return (((INTERNET_CONNECT_HANDLE_OBJECT *)hGopher)->EndCacheWrite(
                    &ftExpiryTime,
                    &ftLastModTime,
                    &ftPostCheck,
                    dwEntryType,
                    0,
                    NULL,
                    lpszFileExtension
                    ));
    }
    return (ERROR_SUCCESS);
}


PRIVATE
BOOL
FIsGopherExpired(
    HINTERNET hGopher,
    LPCACHE_ENTRY_INFO lpCEI
    )
{
    DEBUG_ENTER((DBG_GOPHER,
                 Bool,
                 "FIsGopherExpired",
                 "%#x, %#x",
                 hGopher,
                 lpCEI
                 ));

    FILETIME ft;
    BOOL fExpired = TRUE;

    GetCurrentGmtTime(&ft);
    if (CheckExpired(   hGopher,
                        &fExpired,
                        lpCEI,
                        dwdwGopherDefaultExpiryDelta) != ERROR_SUCCESS) {
        fExpired = TRUE;
    }

    DEBUG_LEAVE(fExpired);

    return (fExpired);
}
