/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    ftpapia.cxx

Abstract:

    ANSI versions of Windows Internet Client DLL FTP APIs

    Contents:
        FtpFindFirstFileA
        FtpGetFileA
        FtpPutFileA
        FtpDeleteFileA
        FtpRenameFileA
        FtpOpenFileA
        FtpCreateDirectoryA
        FtpRemoveDirectoryA
        FtpSetCurrentDirectoryA
        FtpGetCurrentDirectoryA
        FtpCommandA
        FtpGetFileSize
        FtpGetSystemNameA
        FtpFindNextFileA
        FtpReadFile
        FtpWriteFile
        pFtpGetUrlString

Author:

    Heath Hunnicutt [t-heathh] 13-Jul-1994

Environment:

    Win32 user-level DLL

Revision History:

    09-Mar-1995 rfirth
        moved from findfile.c, ftphelp.c

    13-Jul-1994 t-heathh
        Created

--*/

#include <wininetp.h>
#include "ftpapih.h"

//
// manifests
//

#define DEFAULT_TRANSFER_BUFFER_LENGTH  (4 K)

#define ALLOWED_FTP_FLAGS               (INTERNET_FLAGS_MASK \
                                        | FTP_TRANSFER_TYPE_MASK \
                                        )

//
// private prototypes
//

PRIVATE
BOOL
FBeginCacheReadProcessing(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszFileName,
    IN DWORD dwAccess,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext,
    IN BOOL  fIsHtmlFind
    );

PRIVATE
BOOL
FFtpCanReadFromCache(
    IN HINTERNET hFtpSession
    );

PRIVATE
BOOL
FBeginCacheWriteProcessing(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszFileName,
    IN DWORD dwAccess,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext,
    IN BOOL  fIsHtmlFind
    );

PRIVATE
BOOL
FFtpCanWriteToCache(
    HINTERNET   hFtpSession
    );

DWORD
InbLocalEndCacheWrite(
    IN HINTERNET hFtpFile,
    IN LPSTR    lpszFileExtension,
    IN BOOL fNormal
    );

PRIVATE
BOOL
FGetCWDFromCache(
    HINTERNET   hFtpSession,
    LPSTR       lpBuff,
    LPDWORD     lpdwBuffSize
    );

PRIVATE
BOOL
FIsFtpExpired(
    HINTERNET   handle,
    LPCACHE_ENTRY_INFO  lpCEI
    );

VOID
LocalSetObjectName(
    HINTERNET hFtpMapped,
    LPSTR   lpszFileName
    );

PRIVATE
BOOL
IsSearchFileDirectory(
    LPCSTR   lpszFileDirName
);

//
// functions
//


INTERNETAPI
HINTERNET
WINAPI
FtpFindFirstFileA(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszSearchFile OPTIONAL,
    OUT LPWIN32_FIND_DATA lpFindFileData OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    Download the remote site's directory listing and parse it into
    WIN32_FIND_DATA structures that we can pass back to the app.

    If the FTP session is currently involved in a data transfer, such as
    a FtpOpenFile()....FtpCloseFile() series of calls, this function will
    fail.

Arguments:

    hFtpSession     - Handle to an FTP session, as returned from FtpOpen()

    lpszSearchFile  - Pointer to a string containing a file specification
                      that constrains the search.  (e.g., "*.txt"). A NULL
                      pointer is treated the same as an empty string

    lpFindFileData  - Pointer to a buffer that will contain WIN32_FIND_DATA
                      information when this call succeeds. If this parameter
                      is NULL, then we can still return success, but all find
                      information will be returned via InternetFindNextFile()

    dwFlags         - controlling caching, etc.

    dwContext       - app-supplied context value for call-backs

Return Value:

    HINTERNET
        Success - new find handle

        Failure - NULL. Call GetLastError() for more information:

                    ERROR_INVALID_HANDLE
                        The session handle is not recognized

                    ERROR_FTP_TRANSFER_IN_PROGRESS
                        The data connection is already in use

                    ERROR_NO_MORE_FILES
                        The end of the directory listing has been reached

                    ERROR_INTERNET_EXTENDED_ERROR
                        Call InternetGetLastResponseInfo() for the text

                    ERROR_INTERNET_INTERNAL_ERROR
                        Something bad happened

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Handle,
                     "FtpFindFirstFileA",
                     "%#x, %.80q, %#x, %#x, %#x",
                     hFtpSession,
                     lpszSearchFile,
                     lpFindFileData,
                     dwFlags,
                     dwContext
                     ));


    HINTERNET hFind = InternalFtpFindFirstFileA(hFtpSession,
                                                lpszSearchFile,
                                                lpFindFileData,
                                                dwFlags,
                                                dwContext,
                                                FALSE   // not a CACHE_ONLY request
                                                );

    DEBUG_LEAVE_API(hFind);

    return hFind;
}


HINTERNET
InternalFtpFindFirstFileA(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszSearchFile OPTIONAL,
    OUT LPWIN32_FIND_DATA lpFindFileData OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext,
    IN BOOL fCacheOnly,
    IN BOOL fAllowEmpty
    )

/*++

Routine Description:

    Download the remote site's directory listing and parse it into
    WIN32_FIND_DATA structures that we can pass back to the app.

    If the FTP session is currently involved in a data transfer, such as
    a FtpOpenFile()....FtpCloseFile() series of calls, this function will
    fail.

Arguments:

    hFtpSession     - Handle to an FTP session, as returned from FtpOpen()

    lpszSearchFile  - Pointer to a string containing a file specification
                      that constrains the search.  (e.g., "*.txt"). A NULL
                      pointer is treated the same as an empty string

    lpFindFileData  - Pointer to a buffer that will contain WIN32_FIND_DATA
                      information when this call succeeds. If this parameter
                      is NULL, then we can still return success, but all find
                      information will be returned via InternetFindNextFile()

    dwFlags         - controlling caching, etc.

    dwContext       - app-supplied context value for call-backs,

    fCacheOnly      - don't go remote if didn't find in the cache

    fAllowEmpty     - return handle even if no files found

Return Value:

    HINTERNET
        Success - new find handle

        Failure - NULL. Call GetLastError() for more information:

                    ERROR_INVALID_HANDLE
                        The session handle is not recognized

                    ERROR_FTP_TRANSFER_IN_PROGRESS
                        The data connection is already in use

                    ERROR_NO_MORE_FILES
                        The end of the directory listing has been reached

                    ERROR_INTERNET_EXTENDED_ERROR
                        Call InternetGetLastResponseInfo() for the text

                    ERROR_INTERNET_INTERNAL_ERROR
                        Something bad happened

--*/

{
    DEBUG_ENTER((DBG_FTP,
                 Handle,
                 "InternalFtpFindFirstFileA",
                 "%#x, %.80q, %#x, %#x, %#x, %B, %B",
                 hFtpSession,
                 lpszSearchFile,
                 lpFindFileData,
                 dwFlags,
                 dwContext,
                 fCacheOnly,
                 fAllowEmpty
                 ));

    HINTERNET findHandle = NULL;
    HINTERNET hConnectMapped = NULL;
    BOOL isLocal;
    BOOL isAsync;
    LPINTERNET_THREAD_INFO lpThreadInfo;
    DWORD error;
    BOOL bIsWorker = FALSE;
    BOOL bNonNestedAsync = FALSE;

    if (!GlobalDataInitialized) {
        error = ERROR_INTERNET_NOT_INITIALIZED;
        goto done;
    }

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto done;
    }

    _InternetIncNestingCount();
    bIsWorker = lpThreadInfo->IsAsyncWorkerThread;
    bNonNestedAsync = bIsWorker && (lpThreadInfo->NestedRequests == 1);

    //
    // if this is the async part of the request and this function is not nested
    // then hFtpSession is actually the mapped address of the find handle
    //

    if (bNonNestedAsync) {
        findHandle = hFtpSession;
        hConnectMapped = ((FTP_FIND_HANDLE_OBJECT *)findHandle)->GetParent();
    } else {
        error = MapHandleToAddress(hFtpSession, (LPVOID *)&hConnectMapped, FALSE);
        if ((error != ERROR_SUCCESS) && (hConnectMapped == NULL)) {
            goto quit;
        }
        error = RIsHandleLocal(hConnectMapped,
                               &isLocal,
                               &isAsync,
                               TypeFtpConnectHandle
                               );
        if (error != ERROR_SUCCESS) {
            goto quit;
        }

        //
        // validate parameters
        //

        if ((ARGUMENT_PRESENT(lpFindFileData)
            && IsBadWritePtr(lpFindFileData, sizeof(*lpFindFileData)))
        || (ARGUMENT_PRESENT(lpszSearchFile)
            && IsBadStringPtr(lpszSearchFile, INTERNET_MAX_PATH_LENGTH + 1))) {
            error = ERROR_INVALID_PARAMETER;
            goto quit;
        }

        //
        // convert NULL search argument to empty string
        //

        if (!ARGUMENT_PRESENT(lpszSearchFile)) {
            lpszSearchFile = "";
        }

        //
        // set the context and handle info and clear last error variables
        //

        _InternetSetObjectHandle(lpThreadInfo, hFtpSession, hConnectMapped);
        _InternetSetContext(lpThreadInfo, dwContext);
        _InternetClearLastError(lpThreadInfo);

        //
        // create the handle object now. This can be used to cancel the async
        // operation, or the sync operation if InternetCloseHandle() is called
        // from a different thread
        //

        INET_ASSERT(findHandle == NULL);

        error = RMakeFtpFindObjectHandle(hConnectMapped,
                                         &findHandle,
                                         (CLOSE_HANDLE_FUNC)wFtpFindClose,
                                         dwContext
                                         );
        if (error != ERROR_SUCCESS) {

            INET_ASSERT(findHandle == NULL);

            goto quit;
        }

        //
        // add another reference: we need this to protect the handle against
        // closure in callbacks and across the async thread transition
        //

        ((HANDLE_OBJECT *)findHandle)->Reference();

        //
        // this new handle will be used in callbacks
        //

        _InternetSetObjectHandle(lpThreadInfo,
                                 ((HANDLE_OBJECT *)findHandle)->GetPseudoHandle(),
                                 findHandle
                                 );
    }

    //
    // check to see if the data is in the cache. Do it here so that we don't
    // waste any time going async if we already have the data locally
    //

    if (IsSearchFileDirectory(lpszSearchFile)
    && FBeginCacheReadProcessing(findHandle,
                                 lpszSearchFile,
                                 GENERIC_READ,
                                 dwFlags,
                                 dwContext,
                                 ((INTERNET_CONNECT_HANDLE_OBJECT *)hConnectMapped)
                                            ->IsHtmlFind()))// doing html finds

    {

        error = ERROR_SUCCESS;

        if (lpFindFileData) {

            DWORD dwBytes = sizeof(WIN32_FIND_DATA);

            error = ((INTERNET_CONNECT_HANDLE_OBJECT *)findHandle)->ReadCache(
                        (LPBYTE)lpFindFileData,
                        sizeof(WIN32_FIND_DATA),
                        &dwBytes
                        );
        }
        if (error == ERROR_SUCCESS) {
            goto quit;
        } else {
            ((INTERNET_CONNECT_HANDLE_OBJECT *)findHandle)->EndCacheRetrieval();
        }
    }
    if (!((INTERNET_CONNECT_HANDLE_OBJECT *)findHandle)->IsCacheReadInProgress()) {

        // if this is a cacheonly request or we are in OFFLINE mode
        // then fail

        if (fCacheOnly ||
            ((((INTERNET_CONNECT_HANDLE_OBJECT *)findHandle)->
                GetInternetOpenFlags() | dwFlags) & INTERNET_FLAG_OFFLINE)) {
            error = ERROR_PATH_NOT_FOUND;
            goto quit;
        }
    }

    //
    // the data wasn't in the cache. If the app requested async operation and
    // we are in the app's synchronous thread context then queue the request to
    // the async scheduler
    //

    if (!bIsWorker && isAsync && (dwContext != INTERNET_NO_CALLBACK)) {

        CFsm_FtpFindFirstFile * pFsm = new CFsm_FtpFindFirstFile(
            lpszSearchFile,
            lpFindFileData,
            dwFlags,
            dwContext
            );

        if (pFsm != NULL &&
            pFsm->GetError() == ERROR_SUCCESS)
        {
            if (error == ERROR_SUCCESS) {
                error = pFsm->QueueWorkItem();
                if (error == ERROR_IO_PENDING) {
                    hConnectMapped = NULL;
                    findHandle = NULL;
                }
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
    // if we're here then we're on the synchronous path: either async I/O was
    // not requested, or we failed to make the request asynchronous
    //

    HINTERNET protocolFtpHandle;

    error = RGetLocalHandle(hConnectMapped, &protocolFtpHandle);
    if (error == ERROR_SUCCESS) {

        HINTERNET protocolFindHandle;

        error = wFtpFindFirstFile(protocolFtpHandle,
                                  lpszSearchFile,
                                  lpFindFileData,
                                  &protocolFindHandle
                                  );

        if (error == ERROR_NO_MORE_FILES && fAllowEmpty) {

            //
            // The directory is empty.  Allow a handle to be returned,
            // but mark it empty so FtpFindNextFile doesn't complain.
            //

            ((FTP_FIND_HANDLE_OBJECT *) findHandle)->SetIsEmpty();
            error = ERROR_SUCCESS;
        }

        if (error == ERROR_SUCCESS) {
            ((FTP_FIND_HANDLE_OBJECT *)findHandle)->SetFindHandle(
                                                        protocolFindHandle
                                                        );
        }
    }

    //
    // if we succeeded in getting the data, add it to the cache
    //

    if (error == ERROR_SUCCESS) {

        //
        // don't worry about errors if cache write fails
        //

        if (IsSearchFileDirectory(lpszSearchFile)  && FBeginCacheWriteProcessing(findHandle,
                                       lpszSearchFile,
                                       GENERIC_READ,
                                       dwFlags,
                                       dwContext,
                                       ((INTERNET_CONNECT_HANDLE_OBJECT *)hConnectMapped)
                                            ->IsHtmlFind()// doing html finds
                                       )) {
            if (!((INTERNET_CONNECT_HANDLE_OBJECT *)hConnectMapped)->IsHtmlFind()
                     && lpFindFileData) {

                DWORD dwBytes = sizeof(WIN32_FIND_DATA);
                DWORD errorCache;

                errorCache = ((INTERNET_CONNECT_HANDLE_OBJECT *)findHandle)->
                                WriteCache((LPBYTE)lpFindFileData,
                                           sizeof(WIN32_FIND_DATA)
                                           );
                if (errorCache != ERROR_SUCCESS) {
                    InbLocalEndCacheWrite(findHandle,
                                          NULL,
                                          (errorCache == ERROR_NO_MORE_FILES)
                                          );
                }
            }
        }
    }

quit:

    _InternetDecNestingCount(1);

    if ((!bNonNestedAsync
         || ((error != ERROR_SUCCESS) && (error != ERROR_IO_PENDING)))
        && (findHandle != NULL)) {

        //
        // balance the extra reference we added to protect against closure in
        // callbacks and across the async thread transition. If non-nested
        // async request, this will be accomplished after REQUEST_COMPLETE
        // callback
        //

        if (((HANDLE_OBJECT *)findHandle)->Dereference()) {
            error = ERROR_INTERNET_OPERATION_CANCELLED;
        }
    }

done:

    if (error == ERROR_SUCCESS) {

        //
        // success - return generated pseudo-handle
        //

        findHandle = ((HANDLE_OBJECT *)findHandle)->GetPseudoHandle();
    } else {
        if (bNonNestedAsync) {
            if (((HANDLE_OBJECT *)findHandle)->IsInvalidated()) {
                error = ERROR_INTERNET_OPERATION_CANCELLED;
            }
        }
        if ((error != ERROR_IO_PENDING) && (findHandle != NULL)) {
            _InternetCloseHandle(((HANDLE_OBJECT *)findHandle)->GetPseudoHandle());
            if (bNonNestedAsync) {

                //
                // this handle deref'd at async completion
                //

                hConnectMapped = NULL;
            }

            if (lpThreadInfo) {
                _InternetSetContext(lpThreadInfo, dwContext);
            }
        }
        findHandle = NULL;
    }
    if (hConnectMapped != NULL) {
        DereferenceObject((LPVOID)hConnectMapped);
    }
    if (error != ERROR_SUCCESS) {

        DEBUG_ERROR(API, error);

        SetLastError(error);
    }

    DEBUG_LEAVE(findHandle);

    return findHandle;
}


INTERNETAPI
BOOL
WINAPI
FtpGetFileA(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszRemoteFile,
    IN LPCSTR lpszNewFile,
    IN BOOL fFailIfExists,
    IN DWORD dwFlagsAndAttributes,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    This is a 'wrapper' function that opens/creates a local file and calls other
    FTP APIs to copy a file from an FTP server to the local file.

    This API does not get remoted, although APIs called herein do

Arguments:

    hFtpSession             - identifies the FTP server where the file resides

    lpszRemoteFile          - name of the file on the server to get

    lpszNewFile             - name of the local file to create

    fFailIfExists           - TRUE if we should not overwrite an existing file

    dwFlagsAndAttributes    - various flags

    dwFlags                 - how to transfer the file: as ASCII text or binary
                              and open options

    dwContext               - app-supplied context value for call-backs

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Use GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "FtpGetFileA",
                     "%#x, %q, %q, %B, %#x, %#x, %#x",
                     hFtpSession,
                     lpszRemoteFile,
                     lpszNewFile,
                     fFailIfExists,
                     dwFlagsAndAttributes,
                     dwFlags,
                     dwContext
                     ));

    BOOL fSuccess = FALSE;
    DWORD error = ERROR_SUCCESS;

    PWSTR pwszRemoteFile = NULL, pwszNewFile = NULL;
    DWORD cc;
    
    if (IsBadStringPtr(lpszRemoteFile, INTERNET_MAX_PATH_LENGTH + 1)
        || (*lpszRemoteFile == '\0')
        || IsBadStringPtr(lpszNewFile, INTERNET_MAX_PATH_LENGTH + 1)
        || (*lpszNewFile == '\0'))
    {
        error = ERROR_INVALID_PARAMETER;
        goto done;
    }

    cc = MultiByteToWideChar(CP_ACP, 0, lpszRemoteFile, -1, NULL, 0);
    pwszRemoteFile = (PWSTR)ALLOCATE_FIXED_MEMORY((cc*sizeof(WCHAR)));
    if (!pwszRemoteFile)
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }
    MultiByteToWideChar(CP_ACP, 0, lpszRemoteFile, -1, pwszRemoteFile, cc);

    cc = MultiByteToWideChar(CP_ACP, 0, lpszNewFile, -1, NULL, 0);
    pwszNewFile = (PWSTR)ALLOCATE_FIXED_MEMORY((cc*sizeof(WCHAR)));
    if (!pwszNewFile)
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto done;        
    }
    MultiByteToWideChar(CP_ACP, 0, lpszNewFile, -1, pwszNewFile, cc);

    fSuccess = FtpGetFileW(
                        hFtpSession,
                        pwszRemoteFile,
                        pwszNewFile,
                        fFailIfExists,
                        dwFlagsAndAttributes,
                        dwFlags,
                        dwContext);

done:
    if (pwszRemoteFile)
    {
        FREE_MEMORY(pwszRemoteFile);
    }
    if (pwszNewFile)
    {
        FREE_MEMORY(pwszNewFile);
    }
    
    if (error != ERROR_SUCCESS) {
        SetLastError(error);
        DEBUG_ERROR(API, error);
    }
    DEBUG_LEAVE_API(fSuccess);

    return fSuccess;
}


INTERNETAPI
BOOL
WINAPI
FtpPutFileA(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszLocalFile,
    IN LPCSTR lpszNewRemoteFile,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    This is a 'wrapper' function that opens/creates a local file and calls other
    FTP APIs to copy a file from an FTP server to the local file.

    This API does not get remoted, although APIs called herein do

    BUGBUG - this API is virtually the same as FtpGetFileA(). Check out
             possibility of commonalizing

Arguments:

    hFtpSession         - identifies the FTP server where the file resides

    lpszLocalFile       - name of the local file to upload

    lpszNewRemoteFile   - name of the file on the server to create

    dwFlags             - how to transfer the file: as ASCII text or binary and
                          open options

    dwContext           - app-supplied context value for call-backs

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Use GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "FtpPutFileA",
                     "%#x, %q, %q, %#x, %#x",
                     hFtpSession,
                     lpszLocalFile,
                     lpszNewRemoteFile,
                     dwFlags,
                     dwContext
                     ));

    BOOL fSuccess = FALSE;
    DWORD error = ERROR_SUCCESS;

    PWSTR pwszRemoteFile = NULL, pwszNewFile = NULL;
    DWORD cc;
    
    if (IsBadStringPtr(lpszNewRemoteFile, INTERNET_MAX_PATH_LENGTH + 1)
        || (*lpszNewRemoteFile == '\0')
        || IsBadStringPtr(lpszLocalFile, INTERNET_MAX_PATH_LENGTH + 1)
        || (*lpszLocalFile == '\0'))
    {
        error = ERROR_INVALID_PARAMETER;
        goto done;
    }

    cc = MultiByteToWideChar(CP_ACP, 0, lpszNewRemoteFile, -1, NULL, 0);
    pwszRemoteFile = (PWSTR)ALLOCATE_FIXED_MEMORY((cc*sizeof(WCHAR)));
    if (!pwszRemoteFile)
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }
    MultiByteToWideChar(CP_ACP, 0, lpszNewRemoteFile, -1, pwszRemoteFile, cc);

    cc = MultiByteToWideChar(CP_ACP, 0, lpszLocalFile, -1, NULL, 0);
    pwszNewFile = (PWSTR)ALLOCATE_FIXED_MEMORY((cc*sizeof(WCHAR)));
    if (!pwszNewFile)
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto done;        
    }
    MultiByteToWideChar(CP_ACP, 0, lpszLocalFile, -1, pwszNewFile, cc);

    fSuccess = FtpPutFileW(
                        hFtpSession,
                        pwszNewFile,
                        pwszRemoteFile,
                        dwFlags,
                        dwContext);

done:
    if (pwszRemoteFile)
    {
        FREE_MEMORY(pwszRemoteFile);
    }
    if (pwszNewFile)
    {
        FREE_MEMORY(pwszNewFile);
    }
    
    if (error != ERROR_SUCCESS) {
        SetLastError(error);
        DEBUG_ERROR(API, error);
    }
    DEBUG_LEAVE_API(fSuccess);

    return fSuccess;
}


INTERNETAPI
BOOL
WINAPI
FtpDeleteFileA(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszFileName
    )

/*++

Routine Description:

    Deletes the named file at the FTP server

Arguments:

    hFtpSession     - identifies FTP server where file is to be deleted

    lpszFileName    - name of file to delete

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Use GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "FtpDeleteFileA",
                     "%#x, %q",
                     hFtpSession,
                     lpszFileName
                     ));

    DWORD error;
    LPINTERNET_THREAD_INFO lpThreadInfo;
    DWORD nestingLevel = 0;
    HINTERNET hMapped = NULL;
    BOOL fDeref = TRUE;

    if (!GlobalDataInitialized) {
        error = ERROR_INTERNET_NOT_INITIALIZED;
        goto done;
    }

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto done;
    }

    _InternetIncNestingCount();
    nestingLevel = 1;

    //
    // map the handle
    //

    error = MapHandleToAddress(hFtpSession, (LPVOID *)&hMapped, FALSE);
    if ((error != ERROR_SUCCESS) && (hMapped == NULL)) {
        goto quit;
    }

    //
    // set the context and handle info and clear last error variables
    //

    _InternetSetObjectHandle(lpThreadInfo, hFtpSession, hMapped);
    _InternetSetContext(lpThreadInfo,
                        ((INTERNET_HANDLE_OBJECT *)hMapped)->GetContext()
                        );
    _InternetClearLastError(lpThreadInfo);

    //
    // quit now if the handle is invalid
    //

    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // validate handle
    //

    BOOL isLocal;
    BOOL isAsync;

    error = RIsHandleLocal(hMapped,
                           &isLocal,
                           &isAsync,
                           TypeFtpConnectHandle
                           );
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // perform sync work
    //

    if (!lpThreadInfo->IsAsyncWorkerThread
    || (lpThreadInfo->NestedRequests > 1)) {

        //
        // validate parameters
        //

        if (IsBadStringPtr(lpszFileName, INTERNET_MAX_PATH_LENGTH + 1)
        || (*lpszFileName == '\0')) {
            error = ERROR_INVALID_PARAMETER;
            goto quit;
        }

        // in offline mode modifications are disallowed
        // someday we will do internet briefcase but not today

        // BUGBUG there is hole in this API, there is no dwFlags
        // so there is no way to know whether these operations are
        // happening online or offline
        if (((INTERNET_CONNECT_HANDLE_OBJECT *)hMapped)->GetInternetOpenFlags() &
                INTERNET_FLAG_OFFLINE) {
            error = ERROR_WRITE_PROTECT;
            goto quit;
        }

        if (!lpThreadInfo->IsAsyncWorkerThread && isAsync) {

            // MakeAsyncRequest
            CFsm_FtpDeleteFile * pFsm;

            pFsm = new CFsm_FtpDeleteFile(hFtpSession, lpszFileName);
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
    }

    LocalSetObjectName(hMapped, (LPSTR)lpszFileName);

    HINTERNET ftpHandle;

    error = RGetLocalHandle(hMapped, &ftpHandle);
    if (error == ERROR_SUCCESS) {

        error = wFtpDeleteFile(ftpHandle, lpszFileName);

        if (error == ERROR_SUCCESS) {

            ((INTERNET_CONNECT_HANDLE_OBJECT *)hMapped)->ExpireDependents();

        }
    }

quit:

    if ((hMapped != NULL) && fDeref) {
        DereferenceObject((LPVOID)hMapped);
    }
    _InternetDecNestingCount(nestingLevel);;

done:

    BOOL success;

    if (error != ERROR_SUCCESS) {
        SetLastError(error);
        success = FALSE;

        DEBUG_ERROR(API, error);

    } else {
        success = TRUE;
    }

    DEBUG_LEAVE_API(success);

    return success;
}


INTERNETAPI
BOOL
WINAPI
FtpRenameFileA(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszExisting,
    IN LPCSTR lpszNew
    )

/*++

Routine Description:

    Renames a file on an FTP server

Arguments:

    hFtpSession     - identifies FTP server where file is to be renamed

    lpszExisting    - current file name

    lpszNew         - new file name

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Use GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "FtpRenameFileA",
                     "%#x, %q, %q",
                     hFtpSession,
                     lpszExisting,
                     lpszNew
                     ));

    DWORD error;
    LPINTERNET_THREAD_INFO lpThreadInfo;
    DWORD nestingLevel = 0;
    HINTERNET hMapped = NULL;
    BOOL fDeref = TRUE;

    if (!GlobalDataInitialized) {
        error = ERROR_INTERNET_NOT_INITIALIZED;
        goto done;
    }

    //
    // get the thread info block
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
    // map the handle
    //

    error = MapHandleToAddress(hFtpSession, (LPVOID *)&hMapped, FALSE);
    if ((error != ERROR_SUCCESS) && (hMapped == NULL)) {
        goto quit;
    }

    //
    // set the context and handle info and clear last error variables
    //

    _InternetSetObjectHandle(lpThreadInfo, hFtpSession, hMapped);
    _InternetSetContext(lpThreadInfo,
                        ((INTERNET_HANDLE_OBJECT *)hMapped)->GetContext()
                        );
    _InternetClearLastError(lpThreadInfo);

    //
    // quit now if the handle is invalid
    //

    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // validate the handle
    //

    BOOL isLocal;
    BOOL isAsync;

    error = RIsHandleLocal(hMapped,
                           &isLocal,
                           &isAsync,
                           TypeFtpConnectHandle
                           );
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // perform sync work
    //

    if (!lpThreadInfo->IsAsyncWorkerThread
    || (lpThreadInfo->NestedRequests > 1))
    {

        //
        // validate parameters
        //

        if (IsBadStringPtr(lpszExisting, INTERNET_MAX_PATH_LENGTH + 1)
        || (*lpszExisting == '\0')
        || IsBadStringPtr(lpszNew, INTERNET_MAX_PATH_LENGTH + 1)
        || (*lpszNew == '\0')) {
            error = ERROR_INVALID_PARAMETER;
            goto quit;
        }

        // in offline mode modifications are disallowed
        // someday we will do internet briefcase but not today

        // BUGBUG there is hole in this API, there is no dwFlags
        // so there is no way to know whether these operations are
        // happening online or offline
        if (((INTERNET_CONNECT_HANDLE_OBJECT *)hMapped)->GetInternetOpenFlags() &
                INTERNET_FLAG_OFFLINE) {
            error = ERROR_WRITE_PROTECT;
            goto quit;
        }

        if (!lpThreadInfo->IsAsyncWorkerThread && isAsync) {

            // MakeAsyncRequest
            CFsm_FtpRenameFile * pFsm;

            pFsm = new CFsm_FtpRenameFile(hFtpSession, lpszExisting, lpszNew);
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
    }

    HINTERNET ftpHandle;

    LocalSetObjectName(hMapped, (LPSTR)lpszExisting);

    error = RGetLocalHandle(hMapped, &ftpHandle);
    if (error == ERROR_SUCCESS) {
        error = wFtpRenameFile(ftpHandle,
                               lpszExisting,
                               lpszNew
                               );

        if (error == ERROR_SUCCESS) {

            ((INTERNET_CONNECT_HANDLE_OBJECT *)hMapped)->ExpireDependents();

        }
    }

quit:

    if ((hMapped != NULL) && fDeref) {
        DereferenceObject((LPVOID)hMapped);
    }

    _InternetDecNestingCount(nestingLevel);;

done:

    BOOL success;

    if (error != ERROR_SUCCESS) {
        SetLastError(error);
        success = FALSE;

        DEBUG_ERROR(API, error);

    } else {
        success = TRUE;
    }

    DEBUG_LEAVE_API(success);

    return success;
}


INTERNETAPI
HINTERNET
WINAPI
FtpOpenFileA(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszFileName,
    IN DWORD dwAccess,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    Sets up the FTP session to read or write a file at the FTP server

Arguments:

    hFtpSession     - InternetConnect handle identifying FTP server

    lpszFileName    - name of file to open

    dwAccess        - how to access file - for read or write. Can be one of:
                        - GENERIC_READ
                        - GENERIC_WRITE

    dwFlags         - how to transfer file - ASCII text, or binary and open
                      options. Can be any or all of the following:
                        - INTERNET_FLAG_RELOAD

                        - INTERNET_FLAG_RAW_DATA (passed through by
                          InternetOpenUrl(), meaningless here)

                        - INTERNET_FLAG_EXISTING_CONNECT (passed through by
                          InternetOpenUrl(), meaningless here)

                        - FTP_TRANSFER_TYPE_XXX

    dwContext       - app-supplied context value for call-backs

Return Value:

    HINTERNET
        Success - handle of FTP file object

        Failure - NULL. Use GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Handle,
                     "FtpOpenFileA",
                     "%#x, %q, %#x, %#x, %#x",
                     hFtpSession,
                     lpszFileName,
                     dwAccess,
                     dwFlags,
                     dwContext
                     ));

    HINTERNET hFile = InternalFtpOpenFileA(hFtpSession,
                                           lpszFileName,
                                           dwAccess,
                                           dwFlags,
                                           dwContext,
                                           FALSE   // this is not a cachonly request
                                           );

    DEBUG_LEAVE_API(hFile);

    return hFile;
}


HINTERNET
InternalFtpOpenFileA(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszFileName,
    IN DWORD dwAccess,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext,
    IN BOOL fCacheOnly
    )

/*++

Routine Description:

    Sets up the FTP session to read or write a file at the FTP server

Arguments:

    hFtpSession     - InternetConnect handle identifying FTP server

    lpszFileName    - name of file to open

    dwAccess        - how to access file - for read or write. Can be one of:
                        - GENERIC_READ
                        - GENERIC_WRITE

    dwFlags         - how to transfer file - ASCII text, or binary and open
                      options. Can be any or all of the following:
                        - INTERNET_FLAG_RELOAD

                        - INTERNET_FLAG_RAW_DATA (passed through by
                          InternetOpenUrl(), meaningless here)

                        - INTERNET_FLAG_EXISTING_CONNECT (passed through by
                          InternetOpenUrl(), meaningless here)

                        - FTP_TRANSFER_TYPE_XXX

    dwContext       - app-supplied context value for call-backs

    fCacheOnly      - TRUE if this operation must be satisfied from cache

Return Value:

    HINTERNET
        Success - handle of FTP file object

        Failure - NULL. Use GetLastError() for more info

--*/

{
    DEBUG_ENTER((DBG_FTP,
                 Handle,
                 "InternalFtpOpenFileA",
                 "%#x, %q, %#x, %#x, %#x, %B",
                 hFtpSession,
                 lpszFileName,
                 dwAccess,
                 dwFlags,
                 dwContext,
                 fCacheOnly
                 ));

    HINTERNET fileHandle = NULL;
    HINTERNET hConnectMapped;
    HINTERNET hObject;
    HINTERNET hObjectMapped = NULL;
    BOOL bNonNestedAsync = FALSE;

    LPINTERNET_THREAD_INFO lpThreadInfo;
    DWORD error;
    DWORD nestingLevel = 0;

    if (!GlobalDataInitialized) {
        error = ERROR_INTERNET_NOT_INITIALIZED;
        goto done;
    }

    //
    // get the thread info block
    //

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto done;
    }

    bNonNestedAsync = lpThreadInfo->IsAsyncWorkerThread
                    && (lpThreadInfo->NestedRequests == 0);
    _InternetIncNestingCount();
    nestingLevel = 1;

    //
    // if this is the async worker thread AND we haven't been called from
    // another API which is running asynchronously, then what we think is
    // hFtpSession is really the file handle object. Get the handles in the
    // right variables
    //

    if (bNonNestedAsync) {
        hObject = hFtpSession;
        error = MapHandleToAddress(hObject, (LPVOID *)&hObjectMapped, FALSE);
        if ((error != ERROR_SUCCESS) && (hObjectMapped == NULL)) {
            goto quit;
        }
        fileHandle = hObjectMapped;
        hConnectMapped = ((FTP_FILE_HANDLE_OBJECT *)fileHandle)->GetParent();
    } else {
        error = MapHandleToAddress(hFtpSession, (LPVOID *)&hConnectMapped, FALSE);
        if ((error != ERROR_SUCCESS) && (hConnectMapped == NULL)) {
            goto quit;
        }
        hObject = hFtpSession;
        hObjectMapped = hConnectMapped;
    }

    //
    // set the context and handle info and clear last error variables
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
    // validate handle
    //

    BOOL isLocal;
    BOOL isAsync;

    error = RIsHandleLocal(hConnectMapped,
                           &isLocal,
                           &isAsync,
                           TypeFtpConnectHandle
                           );
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // perform sync work
    //

    if (!lpThreadInfo->IsAsyncWorkerThread
    || (lpThreadInfo->NestedRequests > 1)) {

        //
        // validate parameters
        //

        if (IsBadStringPtr(lpszFileName, INTERNET_MAX_PATH_LENGTH + 1)
        || (*lpszFileName == '\0')

        //
        // dwAccess must be GENERIC_READ or GENERIC_WRITE, but not both, and
        // can't be zero or have undefined bits set. Comparing for equality
        // works
        //

        || ((dwAccess != GENERIC_READ) && (dwAccess != GENERIC_WRITE))

        //
        // must be a recognized transfer type
        //

        || (((dwFlags & FTP_TRANSFER_TYPE_MASK) != 0)
            ? (((dwFlags & FTP_TRANSFER_TYPE_MASK) != FTP_TRANSFER_TYPE_ASCII)
            && ((dwFlags & FTP_TRANSFER_TYPE_MASK) != FTP_TRANSFER_TYPE_BINARY))
            : FALSE)
        || ((dwFlags & ~ALLOWED_FTP_FLAGS) != 0)) {
            error = ERROR_INVALID_PARAMETER;
            goto quit;
        }

        //
        // use default transfer type if so requested
        //

        if ((dwFlags & FTP_TRANSFER_TYPE_MASK) == 0) {
            dwFlags |= FTP_TRANSFER_TYPE_BINARY;
        }

        //
        // create the handle object now. This can be used to cancel the async
        // operation, or the sync operation if InternetCloseHandle() is called
        // from a different thread
        //

        INET_ASSERT(fileHandle == NULL);

        error = RMakeFtpFileObjectHandle(hConnectMapped,
                                         &fileHandle,
                                         (CLOSE_HANDLE_FUNC)wFtpCloseFile,
                                         dwContext
                                         );
        if (error != ERROR_SUCCESS) {

            INET_ASSERT(fileHandle == NULL);

            goto quit;
        }

        //
        // add reference to keep handle alive during callbacks and across
        // async thread transition
        //

        ((HANDLE_OBJECT *)fileHandle)->Reference();

        //
        // this new handle will be used in callbacks
        //

        _InternetSetObjectHandle(lpThreadInfo,
                                 ((HANDLE_OBJECT *)fileHandle)->GetPseudoHandle(),
                                 fileHandle
                                 );
    }

    if (((FTP_FILE_HANDLE_OBJECT *)fileHandle)->SetFileName(lpszFileName) != ERROR_SUCCESS)
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }


    //
    // check to see if the data is in the cache
    //

    if (FBeginCacheReadProcessing(fileHandle,
                                  lpszFileName,
                                  dwAccess,
                                  dwFlags,
                                  dwContext, FALSE)) {
        error = ERROR_SUCCESS;
        goto quit;
    } else {
        if (fCacheOnly ||
                ((((INTERNET_CONNECT_HANDLE_OBJECT *)fileHandle)->
                    GetInternetOpenFlags() | dwFlags )& INTERNET_FLAG_OFFLINE)) {
            // if we are offline,or doing cacheonly request
            // let us give the right error and quit

            error = ERROR_FILE_NOT_FOUND;

            goto quit;
        }
    }

    if (!lpThreadInfo->IsAsyncWorkerThread
    && isAsync
    && (dwContext != INTERNET_NO_CALLBACK)) {

        // MakeAsyncRequest
        CFsm_FtpOpenFile * pFsm;

        pFsm = new CFsm_FtpOpenFile(((HANDLE_OBJECT *)fileHandle)->GetPseudoHandle(),
                                     dwContext,
                                     lpszFileName,
                                     dwAccess,
                                     dwFlags
                                     );
        if (pFsm != NULL &&
            pFsm->GetError() == ERROR_SUCCESS)
        {
            error = pFsm->QueueWorkItem();
            if (error == ERROR_IO_PENDING) {
                fileHandle = NULL;
                hObjectMapped = NULL;
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

    HINTERNET protocolFtpHandle;

    error = RGetLocalHandle(hConnectMapped, &protocolFtpHandle);
    if (error == ERROR_SUCCESS) {

        HINTERNET protocolFileHandle;

        error = wFtpOpenFile(protocolFtpHandle,
                             lpszFileName,
                             dwAccess,
                             dwFlags,
                             &protocolFileHandle
                             );
        if (error == ERROR_SUCCESS) {
            ((FTP_FILE_HANDLE_OBJECT *)fileHandle)->SetFileHandle(
                                                        protocolFileHandle
                                                        );
            // Now seems like a reasonable time to remove the entry from the
            // cache IFF the open has GENERIC_WRITE access set, i.e. the
            // caller is about to write to this url and make the cache entry
            // stale. This fixes a problem in FtpParseUrl where we bypass the expiry
            // info by forcing an offline state.

            if (dwAccess & GENERIC_WRITE) {
                DeleteUrlCacheEntryA(((FTP_FILE_HANDLE_OBJECT *)fileHandle)->GetURL());
            }
        }
    }

    if (error == ERROR_SUCCESS) {

        //
        // don't worry about errors if cache write does not begin
        //

        FBeginCacheWriteProcessing(fileHandle,
                                   lpszFileName,
                                   dwAccess,
                                   dwFlags,
                                   dwContext,
                                   FALSE        // not a find
                                   );
    }

quit:

    if (fileHandle != NULL) {

        //
        // balance the refcount we added to keep the handle alive during
        // callbacks and across the async thread transition. If this is a non-
        // nested async request then the reference will be balanced after the
        // REQUEST_COMPLETE callback
        //

        ((HANDLE_OBJECT *)fileHandle)->Dereference();
    }

    _InternetDecNestingCount(nestingLevel);;

done:

    if (bNonNestedAsync && (error == ERROR_SUCCESS)) {
        hObjectMapped = hConnectMapped;
    }
    if (hObjectMapped != NULL) {

        //
        // if we are about to deref the file handle BUT it is already invalidated
        // because the operation was cancelled, e.g., then do not perform the
        // deref - leave it for the close. Otherwise, the close will fail and
        // will not reinstate the callback parameters
        //

        if (!((hObjectMapped == fileHandle) && ((HANDLE_OBJECT *)fileHandle)->IsInvalidated())) {
            DereferenceObject((LPVOID)hObjectMapped);
        }
    }
    if (error != ERROR_SUCCESS) {

        //
        // if we are not pending an async request but we created a handle object
        // then close it
        //

        if ((error != ERROR_IO_PENDING) && (fileHandle != NULL)) {

            HANDLE_OBJECT * hConnMapped;
            HINTERNET hConn;

            hConnMapped = (HANDLE_OBJECT *)((HANDLE_OBJECT *)fileHandle)->GetParent();
            if (hConnMapped != NULL) {
                hConn = (HINTERNET)hConnMapped->GetPseudoHandle();
            }
            ((HANDLE_OBJECT *)fileHandle)->Invalidate();
            ((HANDLE_OBJECT *)fileHandle)->Dereference();
            if (hConnMapped != NULL) {
                _InternetSetObjectHandle(lpThreadInfo, hConn, hConnMapped);
                _InternetSetContext(lpThreadInfo, dwContext);
            }
        }

        //
        // error situation, or request is being processed asynchronously: return
        // a NULL handle
        //

        fileHandle = NULL;

        DEBUG_ERROR(API, error);

        SetLastError(error);
    } else {

        //
        // success - return generated pseudo-handle
        //

        fileHandle = ((HANDLE_OBJECT *)fileHandle)->GetPseudoHandle();
    }

    DEBUG_LEAVE(fileHandle);

    return fileHandle;
}


INTERNETAPI
BOOL
WINAPI
FtpCreateDirectoryA(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszDirectory
    )

/*++

Routine Description:

    Creates a directory on the FTP server

Arguments:

    hFtpSession     - identifies FTP server where directory is to be created

    lpszDirectory   - name of directory to create

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Use GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "FtpCreateDirectoryA",
                     "%#x, %q",
                     hFtpSession,
                     lpszDirectory
                     ));

    DWORD error;
    LPINTERNET_THREAD_INFO lpThreadInfo;
    DWORD nestingLevel = 0;
    HINTERNET hMapped = NULL;
    BOOL fDeref = TRUE;

    if (!GlobalDataInitialized) {
        error = ERROR_INTERNET_NOT_INITIALIZED;
        goto done;
    }

    //
    // get the thread info block
    //

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    _InternetIncNestingCount();
    nestingLevel = 1;

    //
    // map the handle
    //

    error = MapHandleToAddress(hFtpSession, (LPVOID *)&hMapped, FALSE);
    if ((error != ERROR_SUCCESS) && (hMapped == NULL)) {
        goto quit;
    }

    //
    // set the context and handle info and clear last error variables
    //

    _InternetSetObjectHandle(lpThreadInfo, hFtpSession, hMapped);
    _InternetSetContext(lpThreadInfo,
                        ((INTERNET_HANDLE_OBJECT *)hMapped)->GetContext()
                        );
    _InternetClearLastError(lpThreadInfo);

    //
    // quit now if the handle is invalid
    //

    if (error != ERROR_SUCCESS) {
        goto quit;
    }


    //
    // validate handle
    //

    BOOL isLocal;
    BOOL isAsync;

    error = RIsHandleLocal(hMapped,
                           &isLocal,
                           &isAsync,
                           TypeFtpConnectHandle
                           );
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // perform sync work
    //

    if (!lpThreadInfo->IsAsyncWorkerThread
    || (lpThreadInfo->NestedRequests > 1)) {

        //
        // validate parameters
        //

        if (IsBadStringPtr(lpszDirectory, INTERNET_MAX_PATH_LENGTH + 1)
        || (*lpszDirectory == '\0')) {
            error = ERROR_INVALID_PARAMETER;
            goto quit;
        }

        // in offline mode modifications are disallowed
        // someday we will do internet briefcase but not today

        // BUGBUG there is hole in this API, there is no dwFlags
        // so there is no way to know whether these operations are
        // happening online or offline
        if (((INTERNET_CONNECT_HANDLE_OBJECT *)hMapped)->GetInternetOpenFlags() &
                INTERNET_FLAG_OFFLINE) {
            error = ERROR_WRITE_PROTECT;
            goto quit;
        }

        if (!lpThreadInfo->IsAsyncWorkerThread && isAsync) {

            // MakeAsyncRequest
            CFsm_FtpCreateDirectory * pFsm;

            pFsm = new CFsm_FtpCreateDirectory(hFtpSession, lpszDirectory);
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
    }

    HINTERNET ftpHandle;

    LocalSetObjectName(hMapped, (LPSTR)lpszDirectory);

    error = RGetLocalHandle(hMapped, &ftpHandle);
    if (error == ERROR_SUCCESS) {
        error = wFtpCreateDirectory(ftpHandle,
                                    lpszDirectory
                                    );
        if (error == ERROR_SUCCESS) {

            ((INTERNET_CONNECT_HANDLE_OBJECT *)hMapped)->ExpireDependents();

        }
    }

quit:

    _InternetDecNestingCount(nestingLevel);;

done:

    BOOL success;

    if (error != ERROR_SUCCESS) {
        SetLastError(error);
        success = FALSE;
        DEBUG_ERROR(API, error);
    } else {
        success = TRUE;
    }

    if ((hMapped != NULL) && fDeref) {
        DereferenceObject((LPVOID)hMapped);
    }

    DEBUG_LEAVE_API(success);

    return success;
}


INTERNETAPI
BOOL
WINAPI
FtpRemoveDirectoryA(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszDirectory
    )

/*++

Routine Description:

    Removes a directory at an FTP server

Arguments:

    hFtpSession     - identifies FTP server where directory is to be removed

    lpszDirectory   - name of directory to remove

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Use GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "FtpRemoveDirectoryA",
                     "%#x, %q",
                     hFtpSession,
                     lpszDirectory
                     ));

    DWORD error;
    LPINTERNET_THREAD_INFO lpThreadInfo;
    DWORD nestingLevel = 0;
    HINTERNET hMapped = NULL;
    BOOL fDeref = TRUE;

    if (!GlobalDataInitialized) {
        error = ERROR_INTERNET_NOT_INITIALIZED;
        goto done;
    }

    //
    // get the thread info block
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
    // map the handle
    //

    error = MapHandleToAddress(hFtpSession, (LPVOID *)&hMapped, FALSE);
    if ((error != ERROR_SUCCESS) && (hMapped == NULL)) {
        goto quit;
    }

    //
    // set the context and handle info and clear last error variables
    //

    _InternetSetObjectHandle(lpThreadInfo, hFtpSession, hMapped);
    _InternetSetContext(lpThreadInfo,
                        ((INTERNET_HANDLE_OBJECT *)hMapped)->GetContext()
                        );
    _InternetClearLastError(lpThreadInfo);

    //
    // quit now if the handle is invalid
    //

    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // validate handle
    //

    BOOL isLocal;
    BOOL isAsync;

    error = RIsHandleLocal(hMapped,
                           &isLocal,
                           &isAsync,
                           TypeFtpConnectHandle
                           );
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // perform sync work
    //

    if (!lpThreadInfo->IsAsyncWorkerThread
    || (lpThreadInfo->NestedRequests > 1)) {

        //
        // validate parameters
        //

        if (IsBadStringPtr(lpszDirectory, INTERNET_MAX_PATH_LENGTH + 1)
        || (*lpszDirectory == '\0')) {
            error = ERROR_INVALID_PARAMETER;
            goto quit;
        }

        // in offline mode modifications are disallowed
        // someday we will do internet briefcase but not today

        // BUGBUG there is hole in this API, there is no dwFlags
        // so there is no way to know whether these operations are
        // happening online or offline
        if (((INTERNET_CONNECT_HANDLE_OBJECT *)hMapped)->GetInternetOpenFlags() &
                INTERNET_FLAG_OFFLINE) {
            error = ERROR_WRITE_PROTECT;
            goto quit;
        }

        if (!lpThreadInfo->IsAsyncWorkerThread && isAsync) {

            // MakeAsyncRequest
            CFsm_FtpRemoveDirectory * pFsm;

            pFsm = new CFsm_FtpRemoveDirectory(hFtpSession, lpszDirectory);
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
    }

    HINTERNET ftpHandle;

    error = RGetLocalHandle(hMapped, &ftpHandle);
    if (error == ERROR_SUCCESS) {
        error = wFtpRemoveDirectory(ftpHandle,
                                    lpszDirectory
                                    );
    }

quit:

    _InternetDecNestingCount(nestingLevel);;

done:

    BOOL success;

    if (error != ERROR_SUCCESS) {
        SetLastError(error);
        success = FALSE;

        DEBUG_ERROR(API, error);

    } else {
        success = TRUE;
    }

    if ((hMapped != NULL) && fDeref) {
        DereferenceObject((LPVOID)hMapped);
    }

    DEBUG_LEAVE_API(success);

    return success;
}


INTERNETAPI
BOOL
WINAPI
FtpSetCurrentDirectoryA(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszDirectory
    )

/*++

Routine Description:

    Sets the current directory (for this session) at an FTP server

Arguments:

    hFtpSession     - identifies FTP server at which directory is to be set

    lpszDirectory   - name of directory to make current working directory

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Use GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "FtpSetCurrentDirectoryA",
                     "%#x, %q",
                     hFtpSession,
                     lpszDirectory
                     ));

    DWORD error;
    LPINTERNET_THREAD_INFO lpThreadInfo;
    DWORD nestingLevel = 0;
    HINTERNET hMapped = NULL;
    BOOL fDeref = TRUE;

    if (!GlobalDataInitialized) {
        error = ERROR_INTERNET_NOT_INITIALIZED;
        goto done;
    }

    //
    // get the thread info block
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
    // map the handle
    //

    error = MapHandleToAddress(hFtpSession, (LPVOID *)&hMapped, FALSE);
    if ((error != ERROR_SUCCESS) && (hMapped == NULL)) {
        goto quit;
    }

    //
    // set the context and handle info and clear last error variables
    //

    _InternetSetObjectHandle(lpThreadInfo, hFtpSession, hMapped);
    _InternetSetContext(lpThreadInfo,
                        ((INTERNET_HANDLE_OBJECT *)hMapped)->GetContext()
                        );
    _InternetClearLastError(lpThreadInfo);

    //
    // quit now if the handle is invalid
    //

    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // validate handle
    //

    BOOL isLocal;
    BOOL isAsync;

    error = RIsHandleLocal(hMapped,
                           &isLocal,
                           &isAsync,
                           TypeFtpConnectHandle
                           );
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // perform sync work
    //

    INTERNET_CONNECT_HANDLE_OBJECT * pMapped;

    pMapped = (INTERNET_CONNECT_HANDLE_OBJECT *)hMapped;

    if (!lpThreadInfo->IsAsyncWorkerThread
    || (lpThreadInfo->NestedRequests > 1)) {

        //
        // validate parameters
        //

        if (IsBadStringPtr(lpszDirectory, INTERNET_MAX_PATH_LENGTH + 1)
        || (*lpszDirectory == '\0')) {
            error = ERROR_INVALID_PARAMETER;
            goto quit;
        }

        //
        // if we are not going to net at all (offline), spoof setting directory
        // at the server
        //

        if (pMapped->GetInternetOpenFlags() & INTERNET_FLAG_OFFLINE) {
            goto set_object_cwd;
        }

        //
        // we have to hit the net. This will be an asynchronous operation if the
        // app requested async
        //

        if (!lpThreadInfo->IsAsyncWorkerThread && isAsync) {

            // MakeAsyncRequest
            CFsm_FtpSetCurrentDirectory * pFsm;

            pFsm = new CFsm_FtpSetCurrentDirectory(hFtpSession, lpszDirectory);
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
    }

    HINTERNET ftpHandle;

    error = RGetLocalHandle(hMapped, &ftpHandle);
    if (error == ERROR_SUCCESS) {
        error = wFtpSetCurrentDirectory(ftpHandle, lpszDirectory);
    }

set_object_cwd:

    if (error == ERROR_SUCCESS) {
        error = pMapped->SetCurrentWorkingDirectory((LPSTR)lpszDirectory);
    }

quit:

    _InternetDecNestingCount(nestingLevel);;

done:

    BOOL success;

    if (error != ERROR_SUCCESS) {
        SetLastError(error);
        success = FALSE;

        DEBUG_ERROR(API, error);

    } else {
        success = TRUE;
    }

    if ((hMapped != NULL) && fDeref) {
        DereferenceObject((LPVOID)hMapped);
    }

    DEBUG_LEAVE_API(success);

    return success;
}


INTERNETAPI
BOOL
WINAPI
FtpGetCurrentDirectoryA(
    IN HINTERNET hFtpSession,
    OUT LPSTR lpszCurrentDirectory,
    IN OUT LPDWORD lpdwCurrentDirectory
    )

/*++

Routine Description:

    Gets the name of the current working directory for this session at the
    FTP server identified by hFtpSession

Arguments:

    hFtpSession             - identifies FTP server from which to get directory

    lpszCurrentDirectory    - buffer where name of current directory will be written

    lpdwCurrentDirectory    - IN: size of the buffer
                              OUT: number of bytes returned

Return Value:

    BOOL
        TRUE    - *lpdwCurrentDirectory contains number of characters returned

        FALSE   - Use GetLastError() to get more info. One of the following will
                  be returned:
                    ERROR_INVALID_PARAMETER

                    ERROR_INSUFFICIENT_BUFFER
                        *lpdwCurrentDirectory contains required buffer length

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "FtpGetCurrentDirectoryA",
                     "%#x, %#x, %#x [%d]",
                     hFtpSession,
                     lpszCurrentDirectory,
                     lpdwCurrentDirectory,
                     lpdwCurrentDirectory ? *lpdwCurrentDirectory : 0
                     ));

    DWORD error;
    LPINTERNET_THREAD_INFO lpThreadInfo;
    DWORD nestingLevel = 0;
    HINTERNET hMapped = NULL;
    BOOL fDeref = TRUE;

    if (!GlobalDataInitialized) {
        error = ERROR_INTERNET_NOT_INITIALIZED;
        goto done;
    }

    //
    // get the thread info block
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
    // map the handle
    //

    error = MapHandleToAddress(hFtpSession, (LPVOID *)&hMapped, FALSE);
    if ((error != ERROR_SUCCESS) && (hMapped == NULL)) {
        goto quit;
    }

    //
    // set the context and handle info and clear last error variables
    //

    _InternetSetObjectHandle(lpThreadInfo, hFtpSession, hMapped);
    _InternetSetContext(lpThreadInfo,
                        ((INTERNET_HANDLE_OBJECT *)hMapped)->GetContext()
                        );
    _InternetClearLastError(lpThreadInfo);

    //
    // quit now if the handle is invalid
    //

    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // validate handle
    //

    BOOL isLocal;
    BOOL isAsync;

    error = RIsHandleLocal(hMapped,
                           &isLocal,
                           &isAsync,
                           TypeFtpConnectHandle
                           );
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // perform sync work
    //

    if (!lpThreadInfo->IsAsyncWorkerThread
    || (lpThreadInfo->NestedRequests > 1)) {

        //
        // validate parameters
        //

        //
        // lpdwCurrentDirectory must be present (and writeable - we assume it is)
        //

        if (!ARGUMENT_PRESENT(lpdwCurrentDirectory)

        //
        // lpszCurrentDirectory may be not present, but if it is must be writeable
        // by the number of bytes specified in *lpdwCurrentDirectory
        //

        || (ARGUMENT_PRESENT(lpszCurrentDirectory)
            ? IsBadWritePtr(lpszCurrentDirectory, *lpdwCurrentDirectory) : FALSE)) {

            error = ERROR_INVALID_PARAMETER;
            goto quit;
        }
    }

    //
    // we get CWD from the cache only in disconnected state
    //

    if (FGetCWDFromCache(hMapped, lpszCurrentDirectory, lpdwCurrentDirectory)) {
        error = ERROR_SUCCESS;
        goto quit;
    }

    if (!lpThreadInfo->IsAsyncWorkerThread
        && isAsync) {

        // MakeAsyncRequest
        CFsm_FtpGetCurrentDirectory * pFsm;

        pFsm = new CFsm_FtpGetCurrentDirectory(hFtpSession, lpszCurrentDirectory, lpdwCurrentDirectory);
        if (pFsm != NULL) {
            error = pFsm->QueueWorkItem();
            if ( error == ERROR_IO_PENDING ) {
                fDeref = FALSE;
            }
        } else {
            error = ERROR_NOT_ENOUGH_MEMORY;
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

    HINTERNET ftpHandle;

    error = RGetLocalHandle(hMapped, &ftpHandle);
    if (error == ERROR_SUCCESS) {
        error = wFtpGetCurrentDirectory(
                    ftpHandle,

                    //
                    // if the caller supplied no buffer then the
                    // buffer length is 0
                    //

                    ARGUMENT_PRESENT(lpszCurrentDirectory)
                        ? *lpdwCurrentDirectory
                        : 0,
                    lpszCurrentDirectory,
                    lpdwCurrentDirectory
                    );
    }

quit:

    _InternetDecNestingCount(nestingLevel);;

done:

    BOOL success;

    if (error != ERROR_SUCCESS) {
        SetLastError(error);
        success = FALSE;

        DEBUG_ERROR(API, error);

    } else {
        success = TRUE;
    }

    if ((hMapped != NULL) && fDeref) {
        DereferenceObject((LPVOID)hMapped);
    }

    DEBUG_LEAVE_API(success);

    return success;
}


INTERNETAPI
BOOL
WINAPI
FtpCommandA(
    IN HINTERNET hFtpSession,
    IN BOOL fExpectResponse,
    IN DWORD dwFlags,
    IN LPCSTR lpszCommand,
    IN DWORD_PTR dwContext,
    OUT HINTERNET *phFtpCommand OPTIONAL
    )

/*++

Routine Description:

    Runs an arbitrary command at the identified FTP server

Arguments:

    hFtpSession     - identifies FTP server where this command is to be run

    fExpectResponse - TRUE if we expect response data

    dwTransferType  - how to receive the data - as ASCII text, or as BINARY,
                      and open options

    lpszCommand     - string describing the command to run

    dwContext       - app-supplied context value for call-backs


    phFtpCommand    - pointer to an optional handle that will be created if
                        a valid data socket is opened, fExpectResponse must
                        be set to TRUE phFtpCommand to be filled
Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Use GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "FtpCommandA",
                     "%#x, %B, %#x, %q, %#x, %#x",
                     hFtpSession,
                     fExpectResponse,
                     dwFlags,
                     lpszCommand,
                     dwContext,
                     phFtpCommand
                     ));

    DWORD error;
    HINTERNET hMapped = NULL;
    HINTERNET hCommandMapped = NULL;
    LPINTERNET_THREAD_INFO lpThreadInfo;
    BOOL fDeref = TRUE;

    if (!GlobalDataInitialized) {
        error = ERROR_INTERNET_NOT_INITIALIZED;
        goto quit;
    }

    //
    // get the thread info block
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

    error = MapHandleToAddress(hFtpSession, (LPVOID *)&hMapped, FALSE);
    if ((error != ERROR_SUCCESS) && (hMapped == NULL)) {
        goto quit;
    }

    INTERNET_CONNECT_HANDLE_OBJECT * pMapped;

    pMapped = (INTERNET_CONNECT_HANDLE_OBJECT *)hMapped;

    //
    // this is the handle we are currently working on
    //

    _InternetSetObjectHandle(lpThreadInfo,
                             hFtpSession,
                             hMapped
                             );
    _InternetSetContext(lpThreadInfo, dwContext);

    //
    // clear the per-thread object last error variables
    //

    InternetClearLastError();

    BOOL isLocal;
    BOOL isAsync;

    //
    // make RPC or local-worker function call
    //

    error = RIsHandleLocal(hMapped,
                           &isLocal,
                           &isAsync,
                           TypeFtpConnectHandle
                           );

    if ( error != ERROR_SUCCESS ) {
        goto quit;
    }

    //
    // in offline mode we can't execute commands
    //

    if ((pMapped->GetInternetOpenFlags()|dwFlags) & INTERNET_FLAG_OFFLINE) {
        error = ERROR_INTERNET_NO_DIRECT_ACCESS;
        goto quit;
    }

    //
    // validate parameters
    //

    if ( !lpThreadInfo->IsAsyncWorkerThread
         || (lpThreadInfo->NestedRequests > 1) )
    {
        //
        // BUGBUG - reasonable upper limit for command string length?
        //

        if (fExpectResponse && (phFtpCommand == NULL))
        {
            error = ERROR_INVALID_PARAMETER;
            goto quit;
        }


        if (IsBadStringPtr(lpszCommand, 1024)
        || (*lpszCommand == '\0')
        || (fExpectResponse
            && (((dwFlags & FTP_TRANSFER_TYPE_MASK) != FTP_TRANSFER_TYPE_ASCII)
            && ((dwFlags & FTP_TRANSFER_TYPE_MASK) != FTP_TRANSFER_TYPE_BINARY)))
        || ((dwFlags & ~ALLOWED_FTP_FLAGS) != 0))
        {
            error = ERROR_INVALID_PARAMETER;
            goto quit;
        }
    }

    if (!lpThreadInfo->IsAsyncWorkerThread
    && isAsync
    && (dwContext != INTERNET_NO_CALLBACK)) {

        CFsm_FtpCommand * pFsm;

        pFsm = new CFsm_FtpCommand(hFtpSession,
                                   fExpectResponse,
                                   dwFlags,
                                   lpszCommand,
                                   dwContext,
                                   phFtpCommand
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


    INET_ASSERT (error == ERROR_SUCCESS) ;

    if ( fExpectResponse )
    {

        //
        // create the handle object now. This can be used to cancel the async
        // operation, or the sync operation if InternetCloseHandle() is called
        // from a different thread
        //

        error = RMakeFtpFileObjectHandle(hMapped,
                                         &hCommandMapped,
                                         (CLOSE_HANDLE_FUNC)wFtpCloseFile,
                                         dwContext
                                         );
        if (error != ERROR_SUCCESS) {
            goto quit;
        }

        //
        // this new handle will be used in callbacks
        //

    }


    HINTERNET ftpHandle;

    error = RGetLocalHandle(hMapped, &ftpHandle);
    if (error == ERROR_SUCCESS)
    {
        //InternetSetContext(dwContext);
        error = wFtpCommand(ftpHandle,
                            fExpectResponse,
                            dwFlags,
                            lpszCommand
                            );

        if ( fExpectResponse )
        {

            //
            // FTP can only have one active operation per session, so we just return
            // this session handle as the find handle
            //

            ((FTP_FILE_HANDLE_OBJECT *)hCommandMapped)->SetFileHandle(
                                                            ftpHandle
                                                            );

            *phFtpCommand = ((HANDLE_OBJECT *)hCommandMapped)->GetPseudoHandle();
        }
    }

quit:

    BOOL success;

    if ((hMapped != NULL) && fDeref) {
        DereferenceObject((LPVOID)hMapped);
    }

    if (error != ERROR_SUCCESS) {
        SetLastError(error);
        success = FALSE;

        DEBUG_ERROR(API, error);

    } else {
        success = TRUE;
    }

    DEBUG_LEAVE_API(success);

    return success;
}


INTERNETAPI
DWORD
WINAPI
FtpGetFileSize(
    IN HINTERNET hFile,
    OUT LPDWORD lpdwFileSizeHigh OPTIONAL
    )

/*++

Routine Description:

    Same as base Win32 GetFileSize() function. Returns size of file as reported
    by server (if known). For the IIS FTP server, the file size is reported in
    the response string when we open the file. For other (Unix) server, we need
    to send a "SIZE <filename>" command

Arguments:

    hFile               - hInternet of FTP_FILE_HANDLE_OBJECT returned by
                          FtpOpenFile()

    lpdwFileSizeHigh    - optional pointer to returned high 32 bits of file size

Return Value:

    DWORD
        Success - low 32 bits of size of file associated with hFile. If
                  0xFFFFFFFF is returned then GetLastError() must be used to
                  determine if an error occurred. If GetLastError() returns
                  ERROR_SUCCESS then the file size is 4GB-1

        Failure - 0xFFFFFFFF. GetLastError() returns possible error codes:

                    ERROR_INVALID_HANDLE
                        The handle is not a valid HINTERNET

                    ERROR_INTERNET_INCORRECT_HANDLE_TYPE
                        The handle is a valid HINTERNET, but does not describe
                        a FTP FILE handle object

                    ERROR_INVALID_PARAMETER
                        lpdwFileSizeHigh is an invalid pointer

                    ERROR_INTERNET_OVERFLOW
                        The server reported that the file size was >0xFFFFFFFF,
                        but lpdwFileSizeHigh was NULL

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Dword,
                     "FtpGetFileSize",
                     "%#x, %#x",
                     hFile,
                     lpdwFileSizeHigh
                     ));

    DWORD error = ERROR_SUCCESS;
    DWORD dwSizeLow = 0;
    DWORD dwSizeHigh = 0;
    LPINTERNET_THREAD_INFO lpThreadInfo;
    DWORD nestingLevel = 0;
    FTP_FILE_HANDLE_OBJECT * pFileMapped = NULL;
    INTERNET_CONNECT_HANDLE_OBJECT * pConnectMapped = NULL;

    HINTERNET hMapped;

    BOOL fFile = TRUE;

    BOOL isAsync;
    BOOL isLocal;
    BOOL fDeref = TRUE;
    BOOL fDerefSession = FALSE;


    if (!GlobalDataInitialized) {
        error = ERROR_INTERNET_NOT_INITIALIZED;
        goto quit;
    }

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    //
    // validate parameters
    //

    if (lpdwFileSizeHigh != NULL) {
        if(IsBadWritePtr(lpdwFileSizeHigh, sizeof(*lpdwFileSizeHigh))) {
            error = ERROR_INVALID_PARAMETER;
            goto quit;
        }
        *lpdwFileSizeHigh = 0;
    }

    error = MapHandleToAddress(hFile, (LPVOID *)&pFileMapped, FALSE);
    if ((error != ERROR_SUCCESS) && (pFileMapped == NULL)) {
        goto quit;
    }

    hMapped = pFileMapped;

    _InternetSetObjectHandle(lpThreadInfo, hFile, pFileMapped);
    _InternetSetContext(lpThreadInfo,
                        ((FTP_FILE_HANDLE_OBJECT *)pFileMapped)->GetContext()
                        );
    _InternetClearLastError(lpThreadInfo);

    error = RIsHandleLocal(hMapped, // mapped file handle
                           &isLocal,
                           &isAsync,
                           TypeFtpFileHandle
                           );
    if (error != ERROR_SUCCESS) {

#if 0
        error = RIsHandleLocal((HINTERNET) hMapped,
                               &isLocal,
                               &isAsync,
                               TypeFtpConnectHandle
                               );

        if ( error != ERROR_SUCESS ) {
            goto quit;
        }

        //else ...
        fFile = FALSE;
        pConnectMapped = (INTERNET_CONNECT_HANDLE_OBJECT *) pFileMapped;
        hMapped = (HINTERNET) pConnectMapped;
        pFileMapped = NULL;
#else
        goto quit;
#endif
    }

    //
    // If we're reading from the cache we need to find the file size via the cache api.
    //

    //
    // BUGBUG [arthurbi] - Need to test the cached code path, as it may be that we're
    //  not properly reading from the cached handle
    //

    if (fFile && pFileMapped->IsCacheReadInProgress())
    {
        CACHE_ENTRY_INFO ceiCacheInfo;
        DWORD dwBuffSize = sizeof(ceiCacheInfo);

        error = pFileMapped->CacheGetUrlInfo(
                        &ceiCacheInfo,
                        &dwBuffSize
                        );

        if ( error != ERROR_SUCCESS )
        {
            goto quit;
        }

        dwSizeLow =  ceiCacheInfo.dwSizeLow;
        dwSizeHigh = ceiCacheInfo.dwSizeHigh;

        goto quit;
    }

    //
    // if we already know the size of the file from a 150 response, return it
    // immediately, else we need to send a "SIZE" request to the server to get
    // it
    //

    LPFTP_SESSION_INFO lpSessionInfo;
    HINTERNET hHandleMapped;
    HINTERNET hFtpSession;

    //
    // find the FTP_SESSION_INFO and ensure it is set up to receive data
    //

    error = RGetLocalHandle(hMapped, &hFtpSession);
    if (error == ERROR_SUCCESS) {
        if (!FindFtpSession( hFtpSession, &lpSessionInfo)) {
            error = ERROR_INVALID_HANDLE;
            goto quit;
        }
    }
    else
    {
        goto quit;
    }

    fDerefSession = TRUE;

#if 0
    if (!fFile)
    {

        if (!lpThreadInfo->IsAsyncWorkerThread && isAsync)
        {
            CFsm_FtpGetFileSize * pFsm;

            pFsm = new CFsm_FtpGetFileSize(hFile);
            if (pFsm != NULL )
            {
                error = pFsm->QueueWorkItem();
                if ( error == ERROR_IO_PENDING ) {
                    fDeref = FALSE;
                }
            }
            else
            {
                error = ERROR_NOT_ENOUGH_MEMORY;
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

        error = wFtpGetFileSize(hMapped, lpSessionInfo, &dwSizeLow, &dwSizeHigh);
    }
    else
#endif
    {
        INET_ASSERT(fFile);

        //
        // if FFTP_KNOWN_FILE_SIZE is set then we already know the file size
        //

        if ( (lpSessionInfo->Flags & FFTP_KNOWN_FILE_SIZE) == 0 ) {
            error = ERROR_INTERNET_ITEM_NOT_FOUND;
            goto quit;
        }

        // set dwSizeLow here..
        dwSizeLow = lpSessionInfo->dwFileSizeLow;
    }

quit:

    if ( fDerefSession ) {
        DereferenceFtpSession(lpSessionInfo);
    }

    if (pFileMapped != NULL && fDeref) {
        DereferenceObject((LPVOID)pFileMapped);
    }

    if ( error != ERROR_SUCCESS )
    {
        dwSizeLow = 0xffffffff;
    }

    SetLastError(error);

    DEBUG_LEAVE_API(dwSizeLow);

    return dwSizeLow;
}


INTERNETAPI
BOOL
WINAPI
FtpGetSystemNameA(
        IN HINTERNET hSession,
        OUT LPSTR lpszBuffer,
        IN OUT LPDWORD lpdwBufferLength
        )

/*++

Routine Description:

    Returns the results of a "SYST" command sent to the FTP server to identify
    the FTP server/operating system type in use at the site. The server will
    return a string of the form "215 Windows_NT Version 4.0". The FTP status
    code substring "215 " will be stripped before the string is returned to the
    caller

Arguments:

    hSession            - a HINTERNET returned by InternetConnect() describing
                          an FTP session

    lpszBuffer          - pointer to buffer where output string will be stored

    lpdwBufferLength    - IN: the size of lpszBuffer in BYTEs
                          OUT: if successful, the number of CHARACTERs comprising
                          the string in lpszBuffer minus 1 for the string
                          terminator. If ERROR_INSUFFICIENT_BUFFER is returned,
                          the number of BYTEs required to hold the string,
                          including the string termination character

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Use GetLastError() to return the error information.
                  Possible error codes are:

                    ERROR_INVALID_HANDLE
                        The handle is not a valid HINTERNET

                    ERROR_INTERNET_INCORRECT_HANDLE_TYPE
                        The handle is a valid HINTERNET, but does not describe
                        an FTP connect handle object

                    ERROR_INVALID_PARAMETER
                        lpszBuffer or lpdwBufferLength are invalid pointers

                    ERROR_INSUFFICIENT_BUFFER
                        The buffer size given in *lpdwBufferLength is too small
                        to hold the resultant string. The required buffer length
                        is returned in *lpdwBufferLength

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "FtpGetSystemNameA",
                     "%#x, %#x, %#x [%d]",
                     hSession,
                     lpszBuffer,
                     lpdwBufferLength,
                     lpdwBufferLength ? *lpdwBufferLength : 0
                     ));

    DWORD error;
    INTERNET_CONNECT_HANDLE_OBJECT * phMapped = NULL;

    if (!GlobalDataInitialized) {
        error = ERROR_INTERNET_NOT_INITIALIZED;
        goto quit;
    }

    //
    // validate parameters
    //

    if ((lpdwBufferLength == NULL)
    || IsBadWritePtr(lpdwBufferLength, sizeof(*lpdwBufferLength))
    || IsBadWritePtr(lpszBuffer, *lpdwBufferLength)) {
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }

    error = MapHandleToAddress(hSession, (LPVOID *)&phMapped, FALSE);
    if ((error != ERROR_SUCCESS) && (phMapped == NULL)) {
        goto quit;
    }

quit:

    if (phMapped != NULL) {
        DereferenceObject((LPVOID)phMapped);
    }

    BOOL success;

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


//
// Internet subordinate functions
//

BOOL
FtpFindNextFileA(
    IN HINTERNET hFind,
    OUT LPWIN32_FIND_DATA lpFindFileData
    )

/*++

Routine Description:

    Returns the directory entry in the list returned by FtpFindFirstFile()

    Assumes:    1. We are being called from InternetFindNextFile() which has
                   already validated the parameters, set the thread variables,
                   and cleared the object last error info

Arguments:

    hFind           - find object handle, returned by FtpFindFirstFile()

    lpFindFileData  - Pointer to a buffer that will contain WIN32_FIND_DATA
                      information when this call succeeds.

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

{
    DEBUG_ENTER((DBG_FTP,
                 Bool,
                 "FtpFindNextFileA",
                 "%#x, %#x",
                 hFind,
                 lpFindFileData
                 ));

    INET_ASSERT(GlobalDataInitialized);

    DWORD error, errorCache;
    BOOL isLocal;
    BOOL isAsync, fIsHtml = FALSE;

    //
    // make RPC or local-worker function call
    //

    error = RIsHandleLocal(hFind,
                           &isLocal,
                           &isAsync,
                           TypeFtpFindHandle
                           );
    if (error != ERROR_SUCCESS) {

        //
        // if the handle is actually a HTML FTP find handle, then we allow the
        // operation. Note: we can do this because FtpFindNextFile() is not
        // exported, so a rogue app cannot call this function after opening the
        // handle via InternetOpenUrl()
        //

        error = RIsHandleLocal(hFind,
                               &isLocal,
                               &isAsync,
                               TypeFtpFindHandleHtml
                               );
        fIsHtml = (error == ERROR_SUCCESS);
    }

    FTP_FIND_HANDLE_OBJECT * pFind;

    pFind = (FTP_FIND_HANDLE_OBJECT *)hFind;

    if (error == ERROR_SUCCESS) {
        if (pFind->IsCacheReadInProgress()) {

            //we should never get here when the it is a findhtml type handle.
            // internetopenurl should skim it off the top.

            INET_ASSERT(!fIsHtml);

            DWORD   dwLen = sizeof(WIN32_FIND_DATA);
            error = pFind->ReadCache((LPBYTE)lpFindFileData,
                                                                dwLen,
                                                                &dwLen);
            if ((error == ERROR_SUCCESS) && !dwLen) {
                error = ERROR_NO_MORE_FILES;
            }
            goto quit;
        } else {

            INET_ASSERT(!(pFind->GetInternetOpenFlags() & INTERNET_FLAG_OFFLINE));
        }

        HINTERNET ftpHandle;

        error = RGetLocalHandle(hFind, &ftpHandle);
        if (error == ERROR_SUCCESS) {
            if (pFind->IsEmpty()) {
                error = ERROR_NO_MORE_FILES;
            } else {
                error = wFtpFindNextFile(ftpHandle, lpFindFileData);
            }
        }
    }

    errorCache = error;

    if (error == ERROR_SUCCESS) {
        if (((INTERNET_CONNECT_HANDLE_OBJECT *)hFind)->IsCacheWriteInProgress()) {

            // write here only if it is a native find handle
            // otherwise, internetreadurl will take care
            if (!fIsHtml) {
                errorCache = ((INTERNET_CONNECT_HANDLE_OBJECT *)hFind)->WriteCache(
                                (LPBYTE)lpFindFileData,
                                sizeof(WIN32_FIND_DATA)
                                );
            }
        }

    }
    if (errorCache != ERROR_SUCCESS) {
        if (!fIsHtml) {
            InbLocalEndCacheWrite(hFind, NULL, (errorCache == ERROR_NO_MORE_FILES));
        }
    }

quit:

    BOOL success;

    if (error != ERROR_SUCCESS) {
        success = FALSE;

        DEBUG_ERROR(API, error);

    } else {
        success = TRUE;
    }

    DEBUG_LEAVE(success);

    SetLastError(error);

    return success;
}


BOOL
FtpReadFile(
    IN HINTERNET hFile,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead
    )

/*++

Routine Description:

    Reads number of bytes from file at FTP server

    Assumes:    1. We are being called from InternetReadFile() which has
                   already validated the parameters, handled the zero byte
                   read case, set the thread variables, and cleared the object
                   last error info

Arguments:

    hFile                   - file object handle, returned by FtpOpenFile()

    lpBuffer                - pointer to user's buffer

    dwNumberOfBytesToRead   - size of user's buffer

    lpdwNumberOfBytesRead   - number of bytes copied to user's buffer on output

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

{
    DEBUG_ENTER((DBG_FTP,
                 Bool,
                 "FtpReadFile",
                 "%#x, %#x, %d, %#x",
                 hFile,
                 lpBuffer,
                 dwNumberOfBytesToRead,
                 lpdwNumberOfBytesRead
                 ));

    INET_ASSERT(GlobalDataInitialized);

    DWORD error, errorCache;
    BOOL isLocal;
    BOOL isAsync;

    FTP_FILE_HANDLE_OBJECT * pFile = (FTP_FILE_HANDLE_OBJECT *)hFile;

    //
    // make RPC or local-worker function call
    //

    error = RIsHandleLocal(hFile,
                           &isLocal,
                           &isAsync,
                           TypeFtpFileHandle
                           );
    if (error == ERROR_SUCCESS) {

        if (pFile->IsCacheReadInProgress()) {
            error = pFile->ReadCache((LPBYTE)lpBuffer,
                                     dwNumberOfBytesToRead,
                                     lpdwNumberOfBytesRead
                                     );
            if (!*lpdwNumberOfBytesRead || (error != ERROR_SUCCESS)) {

                //
                // don't do anything here so we don't barf when someone
                // does an extraneous read. The cache stream gets closed
                // when the handle is closed. bug#9086
//                ((FTP_FILE_HANDLE_OBJECT *)hFile)->EndCacheRetrieval();
                //

            }

            //
            // quit whether we succeed or we fail
            //

            goto quit;
        } else {

            INET_ASSERT(!((pFile->GetInternetOpenFlags()|pFile->GetCacheFlags())
                            & INTERNET_FLAG_OFFLINE));

        }

        HINTERNET ftpHandle;

        error = RGetLocalHandle(hFile, &ftpHandle);
        if (error == ERROR_SUCCESS) {
            error = wFtpReadFile(ftpHandle,
                                 lpBuffer,
                                 dwNumberOfBytesToRead,
                                 lpdwNumberOfBytesRead
                                 );
        }
    }

    if (error == ERROR_SUCCESS) {
        if (pFile->IsCacheWriteInProgress()) {

            if (!*lpdwNumberOfBytesRead) {

                DEBUG_PRINT(CACHE,
                            INFO,
                            ("Cache write complete\r\n"
                            ));

                errorCache = InbLocalEndCacheWrite(hFile, NULL, TRUE);

                INET_ASSERT(error == ERROR_SUCCESS);

                goto quit;
            }

            INET_ASSERT(pFile->IsCacheReadInProgress() == FALSE);

            if (pFile->WriteCache((LPBYTE)lpBuffer,
                                  *lpdwNumberOfBytesRead
                                  ) != ERROR_SUCCESS) {

                DEBUG_PRINT(CACHE,
                            ERROR,
                            ("Error in Cache write\n"
                            ));

                errorCache = InbLocalEndCacheWrite(hFile, NULL, FALSE);

                INET_ASSERT(error == ERROR_SUCCESS);

            }
        }
    }

quit:

    BOOL success;

    if (error != ERROR_SUCCESS) {
        SetLastError(error);
        success = FALSE;

        DEBUG_ERROR(API, error);

    } else {
        success = TRUE;
    }

    DEBUG_LEAVE(success);

    return success;
}


BOOL
FtpWriteFile(
    IN HINTERNET hFile,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToWrite,
    OUT LPDWORD lpdwNumberOfBytesWritten
    )

/*++

Routine Description:

    Writes a number of bytes from the user's buffer to an open file on an FTP
    server

    Assumes:    1. We are being called from InternetWriteFile() which has
                   already validated the parameters, handled the zero byte
                   read case, set the thread variables, and cleared the object
                   last error info

Arguments:

    hFile                       - file object handle, returned by FtpOpenFile()

    lpBuffer                    - pointer to user's buffer

    dwNumberOfBytesToWrite      - number of bytes to write from user's buffer

    lpdwNumberOfBytesWritten    - number of bytes actually written

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

{
    DEBUG_ENTER((DBG_FTP,
                 Bool,
                 "FtpWriteFile",
                 "%#x, %#x, %d, %#x",
                 hFile,
                 lpBuffer,
                 dwNumberOfBytesToWrite,
                 lpdwNumberOfBytesWritten
                 ));

    INET_ASSERT(GlobalDataInitialized);

    DWORD error;
    BOOL isLocal;
    BOOL isAsync;

    //
    // make RPC or local-worker function call
    //

    error = RIsHandleLocal(hFile,
                           &isLocal,
                           &isAsync,
                           TypeFtpFileHandle
                           );
    if (error == ERROR_SUCCESS) {

        HINTERNET ftpHandle;

        error = RGetLocalHandle(hFile, &ftpHandle);
        if (error == ERROR_SUCCESS) {
            error = wFtpWriteFile(ftpHandle,
                                  lpBuffer,
                                  dwNumberOfBytesToWrite,
                                  lpdwNumberOfBytesWritten
                                  );
            if (error == ERROR_SUCCESS) {

                // expire the url if it exists and it's
                // parent directory

                if( !((FTP_FILE_HANDLE_OBJECT *)hFile)->IsForcedExpirySet()){

                    ((FTP_FILE_HANDLE_OBJECT *)hFile)->ExpireDependents();

                    ((FTP_FILE_HANDLE_OBJECT *)hFile)->SetForcedExpiry(TRUE);

                    ((FTP_FILE_HANDLE_OBJECT *)hFile)->ExpireUrl();
                }
            }
        }
    }

    BOOL success;

    if (error != ERROR_SUCCESS) {
        SetLastError(error);
        success = FALSE;

        DEBUG_ERROR(API, error);

    } else {
        success = TRUE;
    }

    DEBUG_LEAVE(success);

    return success;
}


DWORD
pFtpGetUrlString(
    IN INTERNET_SCHEME SchemeType,
    IN LPSTR    lpszTargetName,
    IN LPSTR    lpszCWD,
    IN LPSTR    lpszObjectName,
    IN LPSTR    lpszExtension,
    IN DWORD    dwPort,
    OUT LPSTR   *lplpUrlName,
    OUT LPDWORD lpdwUrlLen
    )

/*++

Routine Description:

    This routine returns a LocaAlloc'ed buffer containing an FTP URL constructed
    from the TargetHost, CWD, and the ObjectName. The caller is responsible
    for freeing the memory.

Arguments:

    SchemeType      - protocol scheme (INTERNET_SCHEME_FTP)

    lpszTargetName  - name of server

    lpszCWD         - current directory at server

    lpszObjectName  - name of file. If NULL or empty then the URL is for a
                      directory

    lpszExtension   - file extension. NOT USED

    dwPort          - port at server

    lplpUrlName     - pointer to returned URL

    lpdwUrlLen      - pointer to returned URL length

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DWORD dwError, dwLen, dwT, dwTargetNameLen=0, dwObjectNameLen=0;
    char cBuff[INTERNET_MAX_URL_LENGTH], cBuff1[INTERNET_MAX_URL_LENGTH];//????
    URL_COMPONENTS sUrlComp;
    DWORD dwSav, i, ccFirst, ccTmp;

    INET_ASSERT(lpszTargetName);
    INET_ASSERT(lpszObjectName || lpszCWD);

    //
    // NULL or empty object name == directory
    //

    if ((lpszObjectName != NULL) && (*lpszObjectName == '\0')) {
        lpszObjectName = NULL;
    }

    memcpy(cBuff1, "/", sizeof("/"));
    ccFirst = 2;

    // add the current directory only if the path is a relative one
    if (lpszCWD && !(lpszObjectName && (*lpszObjectName == '/')) ) {
        if (*lpszCWD == '/') {
            ++lpszCWD;
        }
        ccTmp = lstrlen(lpszCWD);
        if (ccTmp > sizeof(cBuff1) - 2)
        {
            dwError = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
        memcpy(cBuff1 + 1, lpszCWD, ccTmp + 1);
        ccFirst += ccTmp;

        INET_ASSERT(lpszCWD[ccTmp - 1] == '/');

    }

    if (lpszObjectName) {

        if (*lpszObjectName == '/') {
            lpszObjectName++;
        }

        ccTmp = lstrlen(lpszObjectName);
        if (ccTmp > sizeof(cBuff1) - ccFirst)
        {
            dwError = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
        memcpy(cBuff1 + ccFirst - 1, lpszObjectName, ccTmp + 1);
    }

    memset(&sUrlComp, 0, sizeof(URL_COMPONENTS));

    sUrlComp.dwStructSize = sizeof(URL_COMPONENTS);
    sUrlComp.nScheme = INTERNET_SCHEME_FTP;
    sUrlComp.lpszHostName = lpszTargetName;
    sUrlComp.lpszUrlPath = cBuff1;
    sUrlComp.nPort = (INTERNET_PORT)dwPort;

    dwSav = sizeof(cBuff);

    if(!InternetCreateUrl(&sUrlComp, 0, cBuff, &dwSav)){
        dwError = GetLastError();
        goto Cleanup;
    }

    // BUGBUG, this is because InternetCreateUrl is not returning
    // the correct size

    dwSav = strlen(cBuff)+5;

    for(i=0;i<2;++i) {

        *lplpUrlName = (LPSTR)ALLOCATE_MEMORY(LPTR, dwSav);

        if (*lplpUrlName) {

            if(!InternetCanonicalizeUrl(cBuff, *lplpUrlName, &dwSav, ICU_NO_ENCODE)){

                FREE_MEMORY(*lplpUrlName);

                // general paranoia
                *lplpUrlName = NULL;

                dwError = GetLastError();

                if ((i == 1) || (dwError != ERROR_INSUFFICIENT_BUFFER)) {
                    goto Cleanup;
                }
            }
            else {

                dwError = ERROR_SUCCESS;
                *lpdwUrlLen = dwSav;
                break;

            }
        }
        else {
            SetLastError(dwError = ERROR_NOT_ENOUGH_MEMORY);
            goto Cleanup;
        }
    }



Cleanup:
    if (dwError != ERROR_SUCCESS) {

        INET_ASSERT(!*lplpUrlName);

        *lpdwUrlLen = 0;
    }

    return (dwError);
}


PRIVATE
BOOL
FBeginCacheReadProcessing(
    IN HINTERNET hFtp,
    IN LPCSTR lpszFileName,
    IN DWORD dwAccess,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext,
    IN BOOL fIsHtmlFind
    )

/*++

Routine Description:

    Sets up to read FTP data from the cache

Arguments:

    hFtp            A mapped handle to an ftp data transfer object (FtpOpenFile/FtpFindFirstFile)

    lpszFileName    ftp file to look for in the cache

    dwAccess        Accesstype eg: GENERIC_READ

    dwFlags         caching flags

    dwContext       async context

Returns:

    TRUE of started cache reading,  FALSE otherwise

Comments:

--*/

{
    DEBUG_ENTER((DBG_FTP,
                 Bool,
                 "FBeginCacheReadProcessing",
                 "%#x, %q, %d, %08x, %x, %B",
                 hFtp,
                 lpszFileName,
                 dwAccess,
                 dwFlags,
                 dwContext,
                 fIsHtmlFind
                 ));

    DWORD dwError = ERROR_SUCCESS;
    URLGEN_FUNC fn = pFtpGetUrlString;
    LPCACHE_ENTRY_INFO lpCEI = NULL;
    FTP_FILE_HANDLE_OBJECT *pFtp = (FTP_FILE_HANDLE_OBJECT *)hFtp;

    if (((INTERNET_CONNECT_HANDLE_OBJECT *)hFtp)->IsCacheReadInProgress()) {

        //
        // BUGBUG - return FALSE surely? If there is a cache read in progress
        //          then by default, its for another request?
        //

        DEBUG_LEAVE(TRUE);

        return (TRUE);
    }

    //
    // if the object name is not set then all cache methods fail
    //

    //
    // BUGBUG - do this only when we know we are reading from cache?
    //

    ((INTERNET_CONNECT_HANDLE_OBJECT *)hFtp)->SetObjectName((LPSTR)lpszFileName,
                                                            NULL,
                                                            &fn
                                                            );
    if (dwAccess & GENERIC_WRITE) {

        DEBUG_LEAVE(FALSE);

        return (FALSE);
    }

    if (!(dwFlags & INTERNET_FLAG_NO_CACHE_WRITE)) {

        //
        // set the cache flags like RELOAD etc.
        //

        ((INTERNET_CONNECT_HANDLE_OBJECT *)hFtp)->SetCacheFlags(dwFlags);
    } else {

        //
        // set flags to disable both read and write
        //

        ((INTERNET_CONNECT_HANDLE_OBJECT *)hFtp)->SetCacheFlags(
                                                    INTERNET_FLAG_NO_CACHE_WRITE
                                                    | INTERNET_FLAG_RELOAD);
    }

    if (!FFtpCanReadFromCache(hFtp)) {

        DEBUG_LEAVE(FALSE);

        return (FALSE);
    }

    DEBUG_PRINT(CACHE,
                INFO,
                ("Checking in the cache\n"
                ));

    dwError = ((INTERNET_CONNECT_HANDLE_OBJECT *)hFtp)->BeginCacheRetrieval(&lpCEI);
    if (dwError == ERROR_SUCCESS)
    {
        //
        // found it in the cache
        //

        DEBUG_PRINT(FTP,
                    INFO,
                    ("Found in the cache\n"
                    ));

        BOOL bGetFromCache = FALSE;

        if (IsOffline()
        || (((((INTERNET_HANDLE_OBJECT *)hFtp)->GetInternetOpenFlags() | dwFlags)
            & INTERNET_FLAG_OFFLINE) && !(dwFlags & INTERNET_FLAG_RELOAD))) {
            bGetFromCache = TRUE;
            DEBUG_PRINT(CACHE,
                        INFO,
                        ("Offline, loading from cache:\n \
                            dwFlags & INTERNET_FLAG_RELOAD = %s \n \
                            dwFlags & INTERNET_FLAG_OFFLIME = %s \n \
                            InternetOpenFlags & INTERNET_FLAG_OFFLIME = %s \n",
                            (dwFlags & INTERNET_FLAG_RELOAD) ? "TRUE" : "FALSE",
                            (dwFlags & INTERNET_FLAG_OFFLINE) ? "TRUE" : "FALSE",
                            (((INTERNET_HANDLE_OBJECT *)hFtp)->GetInternetOpenFlags()
                            & INTERNET_FLAG_OFFLINE) ? "TRUE" : "FALSE"
                        ));

        } else {
            if (!FIsFtpExpired(hFtp, lpCEI))
            {
                bGetFromCache = TRUE;
            }
        }

        if (! (( fIsHtmlFind  &&  lpCEI->lpszFileExtension) ||
               (!fIsHtmlFind  && !lpCEI->lpszFileExtension) ) )
        {
            DEBUG_PRINT(CACHE,
                        INFO,
                        ("Mismatched HTML-type, expiring\n"
                        ));

            bGetFromCache = FALSE;
        }


        if (bGetFromCache) {
            dwError = ERROR_SUCCESS;
            ((INTERNET_CONNECT_HANDLE_OBJECT *)hFtp)->SetFromCache();
        } else {

            DEBUG_PRINT(CACHE,
                        INFO,
                        ("Expired\n"
                        ));

            dwError = ((INTERNET_CONNECT_HANDLE_OBJECT *)hFtp)->EndCacheRetrieval();

            INET_ASSERT(dwError == ERROR_SUCCESS);

            dwError = ERROR_FILE_NOT_FOUND;
        }
    }

    //
    // cleanup
    //

    if (lpCEI != NULL) {
        lpCEI = (LPCACHE_ENTRY_INFO)FREE_MEMORY(lpCEI);

        INET_ASSERT(lpCEI == NULL);

    }

    DEBUG_LEAVE(dwError == ERROR_SUCCESS);

    return (dwError == ERROR_SUCCESS);
}


PRIVATE
BOOL
FFtpCanReadFromCache(
    HINTERNET hFtp
    )

/*++

Routine Description:

    This routine checks whether a cache read should even be started.

Arguments:

    hFtp    a mapped handle to an ftp download (FtpOpenFile/FtpFindFirstFile)

Returns:

    Windows Error Code.

Comments:

--*/

{
    DWORD dwFlags;

    dwFlags = ((INTERNET_CONNECT_HANDLE_OBJECT *)hFtp)->GetCacheFlags();

    //
    // in offline or disconnected states client always reads
    //

    if (IsOffline()
    || (((INTERNET_HANDLE_OBJECT *)hFtp)->GetInternetOpenFlags() | dwFlags)
        & INTERNET_FLAG_OFFLINE) {
        return (TRUE);
    }

    //
    // if we are asked to reload data, it is not OK to read
    //

    if (dwFlags & (INTERNET_FLAG_RELOAD | INTERNET_FLAG_RESYNCHRONIZE)) {

        DEBUG_PRINT(CACHE,
                    INFO,
                    ("no cache option\n"
                    ));

        return (FALSE);
    }

    return (TRUE);
}


PRIVATE
BOOL
FBeginCacheWriteProcessing(
    IN HINTERNET hFtp,
    IN LPCSTR lpszFileName,
    IN DWORD dwAccess,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext,
    BOOL fIsHtmlFind
    )

/*++

Routine Description:

    Sets up to start writing FTP data to the cache

Arguments:

    hFtp            A mapped handle to an ftp data transfer object (FtpOpenFile/FtpFindFirstFile)

    lpszFileName    ftp file to look for in the cache

    dwAccess        Accesstype eg: GENERIC_WRITE

    dwFlags         caching flags

    dwContext       async context

Returns:

    TRUE if started cache writing,  FALSE otherwise

Comments:

--*/

{
    DWORD dwError = ERROR_INVALID_FUNCTION;
    URLGEN_FUNC fn = pFtpGetUrlString;
    char cExt[DEFAULT_MAX_EXTENSION_LENGTH + 1];
    DWORD dwBuffLen;
    LPSTR lpszFileExtension;

    if (dwAccess & GENERIC_WRITE) {
        return (FALSE);
    }

    if (!((INTERNET_CONNECT_HANDLE_OBJECT *)hFtp)->IsCacheReadInProgress()) {

        //
        // we are not reading from the cache
        // Let us ask a routine whether we should cache this
        // stuff or not
        //

        if (FFtpCanWriteToCache(hFtp)) {

            //
            // if the object name is not set then all cache methods fail
            //

            ((INTERNET_CONNECT_HANDLE_OBJECT *)hFtp)->
                SetObjectName((LPSTR)lpszFileName,
                              NULL,
                              &fn
                              );

            //
            // set the cache flags
            //

            ((INTERNET_CONNECT_HANDLE_OBJECT *)hFtp)->SetCacheFlags(dwFlags);

            //
            // he says we can cache it.
            //

            DEBUG_PRINT(CACHE,
                        INFO,
                        ("Starting cache write\n"
                        ));

            if (!fIsHtmlFind) {
                dwBuffLen = sizeof(cExt);
                lpszFileExtension = GetFileExtensionFromUrl(((INTERNET_CONNECT_HANDLE_OBJECT *)hFtp)->GetURL(), &dwBuffLen);

                if (lpszFileExtension != NULL) {
                    memcpy(cExt, lpszFileExtension, dwBuffLen);
                    cExt[dwBuffLen] = '\0';
                    lpszFileExtension = cExt;
                }
            }
            else {
                //allways generate htm extension
                strcpy(cExt, "htm");
                lpszFileExtension = cExt;
            }

            dwError = ((INTERNET_CONNECT_HANDLE_OBJECT *)hFtp)->BeginCacheWrite(0, lpszFileExtension);

            if (dwError != ERROR_SUCCESS) {

                DEBUG_PRINT(CACHE,
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
FFtpCanWriteToCache(
    HINTERNET hFtp
    )

/*++

Routine Description:
    This routine checks whether a cache write should even be started.

Arguments:
    hFtp    a mapped handle to an ftp download (FtpOpenFile/FtpFindFirstFile)

Returns:

    TRUE if successful, FALSE otherwise

Comments:

--*/

{

    if (((INTERNET_CONNECT_HANDLE_OBJECT *)hFtp)->GetCacheFlags() & INTERNET_FLAG_DONT_CACHE) {
        return (FALSE);
    }

    return (TRUE);
}


DWORD
InbLocalEndCacheWrite(
    IN HINTERNET hFtp,
    LPSTR lpszFileExtension,
    IN BOOL fNormal
    )

/*++

Routine Description:
    This routine checks terminates cache writing if it was in progress and
    commits the entry to the cache

Arguments:
    hFtp    a mapped handle to an ftp download (FtpOpenFile/FtpFindFirstFile)
    fNormal TRUE if the termination is normal, in which case the collected data
            is entered in the cache, otherwise it is discarded
Returns:

    Windows error code

Comments:

--*/

{

    FILETIME ftLastModTime, ftExpiryTime, ftPostCheck;
    DWORD   dwEntryType;

    if (((INTERNET_CONNECT_HANDLE_OBJECT *)hFtp)->IsCacheWriteInProgress()) {

        ftLastModTime.dwLowDateTime =
        ftLastModTime.dwHighDateTime = 0;

        ftExpiryTime.dwLowDateTime =
        ftExpiryTime.dwHighDateTime = 0;

        ftPostCheck.dwLowDateTime =
        ftPostCheck.dwHighDateTime = 0;


        dwEntryType = (!fNormal)?0xffffffff:
                        ((((INTERNET_CONNECT_HANDLE_OBJECT *)hFtp)->

                            GetCacheFlags() & INTERNET_FLAG_MAKE_PERSISTENT)
                                ? STICKY_CACHE_ENTRY:0
                        );

         DEBUG_PRINT(CACHE,
                     INFO,
                     ("Cache write EntryType = %x \
                     IsPerUserItem = %d \
                     <hFtp = 0x%x> \
                     <hFtp->GetParent() = 0x%x> \
                     <hFtp->IsPerUserItem() = %d\n",
                     dwEntryType,
                     ((INTERNET_CONNECT_HANDLE_OBJECT *)hFtp)->IsPerUserItem(),
                     hFtp,
                     ((INTERNET_CONNECT_HANDLE_OBJECT *)hFtp)->GetParent(),
                     ((INTERNET_CONNECT_HANDLE_OBJECT *)hFtp)->IsPerUserItem()
                     ));

        return (((INTERNET_CONNECT_HANDLE_OBJECT *)hFtp)->
                                EndCacheWrite(  &ftExpiryTime,
                                                &ftLastModTime,
                                                &ftPostCheck,
                                                dwEntryType,
                                                0,
                                                NULL,
                                                lpszFileExtension));
    }
    return (ERROR_SUCCESS);
}


PRIVATE
BOOL
FGetCWDFromCache(
    HINTERNET   hFtpSession,
    LPSTR       lpBuff,
    LPDWORD     lpdwBuffSize
    )

/*++

Routine Description:
    This routine returns the current working directory for an FTP session

Arguments:
    hFtpSession     a mapped handle to an ftp download (FtpOpenFile/FtpFindFirstFile)

    LPSTR           buffer to return the string in

    lpdwBuffSize    IN size of lpBuff, OUT size passed back
Returns:

    TRUE if successful, FALSE otherwise

Comments:

--*/

{
    if ((((INTERNET_CONNECT_HANDLE_OBJECT *)hFtpSession)->GetInternetOpenFlags()
            & INTERNET_FLAG_OFFLINE)) {
        ((INTERNET_CONNECT_HANDLE_OBJECT *)hFtpSession)->GetCurrentWorkingDirectory(lpBuff, lpdwBuffSize);
        return (TRUE);
    }
    return (FALSE);
}


PRIVATE
BOOL
FIsFtpExpired(
    HINTERNET   hFtp,
    LPCACHE_ENTRY_INFO  lpCEI
    )

/*++

Routine Description:
    This routine checks whether an ftp item in the cache has expired

Arguments:
    hFtp    a mapped handle to an ftp download (FtpOpenFile/FtpFindFirstFile)
    lpCEI   cache entry info for the item

Returns:

    TRUE if successful, FALSE otherwise

Comments:

    Even though this is a trivial routine and can be nuked, it is good
    place holder for doing special things to check FTP expiry

--*/

{
    BOOL fExpired = FALSE;

    if (CheckExpired(   hFtp,
                        &fExpired,
                        lpCEI,
                        dwdwFtpDefaultExpiryDelta) != ERROR_SUCCESS) {
        fExpired = TRUE;
    }
    return (fExpired);
}



VOID
LocalSetObjectName(
    HINTERNET hFtpMapped,
    LPSTR   lpszFileName
    )
{
    URLGEN_FUNC fn = pFtpGetUrlString;

    ((INTERNET_CONNECT_HANDLE_OBJECT *)hFtpMapped)->
                SetObjectName((LPSTR)lpszFileName,
                              NULL,
                              &fn
                              );
}

/*++

Routine Description:
    This routine checks whether a string given to FtpFindFirstFile
    is a directory or a file

Arguments:

    lpszSearchFile string passed in to either FtpFindFirstFile

Returns:

    TRUE if successful, FALSE otherwise

Comments:

    This routine should not be used anywhere other thatn FtpFindFirstFile
    functions



--*/
BOOL IsSearchFileDirectory(
    LPCSTR   lpszSearchFile
)
{
    DWORD dwLen;

    if (!lpszSearchFile) {

        return (TRUE);

    }

    dwLen = lstrlen(lpszSearchFile);

    if (!dwLen) {

        return (TRUE);

    }


    return (lpszSearchFile[dwLen-1] == '/');

}
