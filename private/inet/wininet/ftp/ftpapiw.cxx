/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    ftpapiw.cxx

Abstract:

    Wide-character versions of Windows Internet Client DLL FTP APIs and
    Internet subordinate functions

    Contents:
        FtpFindFirstFileW
        FtpGetFileW
        FtpPutFileW
        FtpDeleteFileW
        FtpRenameFileW
        FtpOpenFileW
        FtpCreateDirectoryW
        FtpRemoveDirectoryW
        FtpSetCurrentDirectoryW
        FtpGetCurrentDirectoryW
        FtpCommandW

Author:

    Heath Hunnicutt [t-heathh] 13-Jul-1994

Environment:

    Win32(s) user-level DLL

Revision History:

    09-Mar-1995 rfirth
        moved from unicode.c

    21-Jul-1994 t-heathh
        Created

--*/

#include <wininetp.h>
#include "ftpapih.h"
#include <w95wraps.h>

#define DEFAULT_TRANSFER_BUFFER_LENGTH  (4 K)

#define ALLOWED_FTP_FLAGS               (INTERNET_FLAGS_MASK \
                                        | FTP_TRANSFER_TYPE_MASK \
                                        )

//
// functions
//


BOOL
TransformFtpFindDataToW(LPWIN32_FIND_DATAA pfdA, LPWIN32_FIND_DATAW pfdW)
{
    pfdW->dwFileAttributes = pfdA->dwFileAttributes;
    pfdW->ftCreationTime = pfdA->ftCreationTime;
    pfdW->ftLastAccessTime = pfdA->ftLastAccessTime;
    pfdW->ftLastWriteTime = pfdA->ftLastWriteTime;
    pfdW->nFileSizeHigh = pfdA->nFileSizeHigh;
    pfdW->nFileSizeLow = pfdA->nFileSizeLow;
    pfdW->dwReserved0 = pfdA->dwReserved0;
    pfdW->dwReserved1 = pfdA->dwReserved1;
    MultiByteToWideChar(CP_ACP, 0, pfdA->cFileName, -1, pfdW->cFileName, MAX_PATH);
    MultiByteToWideChar(CP_ACP, 0, pfdA->cAlternateFileName, -1, pfdW->cAlternateFileName, 14);

    return TRUE;
}


INTERNETAPI
HINTERNET
WINAPI
FtpFindFirstFileW(
    IN HINTERNET hFtpSession,
    IN LPCWSTR lpszSearchFile,
    OUT LPWIN32_FIND_DATAW pffdOutput,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    hFtpSession     -
    lpszSearchFile  -
    pffdOutput      -
    dwFlags         -
    dwContext       -

Return Value:

    HINTERNET

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Handle,
                     "FtpFindFirstFileW",
                     "%#x, %.80wq, %#x, %#x, %#x",
                     hFtpSession,
                     lpszSearchFile,
                     pffdOutput,
                     dwFlags,
                     dwContext
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    MEMORYPACKET mpSearchFile;
    HANDLE hInternet = NULL;
    WIN32_FIND_DATAA fdA;
    
    if (!pffdOutput)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    if (lpszSearchFile)
    {
        ALLOC_MB(lpszSearchFile,0,mpSearchFile);
        if (!mpSearchFile.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(lpszSearchFile,mpSearchFile);
    }
    hInternet = FtpFindFirstFileA(hFtpSession,mpSearchFile.psStr,&fdA,dwFlags,dwContext);
    if (hInternet)
    {
        TransformFtpFindDataToW(&fdA, pffdOutput);
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


INTERNETAPI
BOOL
WINAPI
FtpGetFileW(
    IN HINTERNET hFtpSession,
    IN LPCWSTR lpszRemoteFile,
    IN LPCWSTR lpszNewFile,
    IN BOOL fFailIfExists,
    IN DWORD dwFlagsAndAttributes,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    hFtpSession             -
    lpszRemoteFile          -
    lpszNewFile             -
    fFailIfExists           -
    dwFlagsAndAttributes    -
    dwFlags                 -
    dwContext               -

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "FtpGetFileW",
                     "%#x, %wq, %wq, %B, %#x, %#x, %#x",
                     hFtpSession,
                     lpszRemoteFile,
                     lpszNewFile,
                     fFailIfExists,
                     dwFlagsAndAttributes,
                     dwFlags,
                     dwContext
                     ));

    LPINTERNET_THREAD_INFO lpThreadInfo;
    DWORD error;
    BOOL success;
    DWORD nestingLevel = 0;
    HINTERNET hSessionMapped = NULL;
    HINTERNET hFileMapped = NULL;
    BOOL fDeref = TRUE;

    if (!GlobalDataInitialized) {
        error = ERROR_INTERNET_NOT_INITIALIZED;
        goto done;
    }

    //
    // we need the INTERNET_THREAD_INFO
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
    // map the connect handle
    //

    error = MapHandleToAddress(hFtpSession, (LPVOID *)&hSessionMapped, FALSE);
    if ((error != ERROR_SUCCESS) && (hSessionMapped == NULL)) {
        goto quit;
    }

    //
    // set the context and handle info and clear last error variables
    //

    _InternetSetObjectHandle(lpThreadInfo, hFtpSession, hSessionMapped);
    _InternetSetContext(lpThreadInfo, dwContext);
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

    error = RIsHandleLocal(hSessionMapped,
                           &isLocal,
                           &isAsync,
                           TypeFtpConnectHandle
                           );
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // if we're in the async thread context then we've already validated the
    // parameters, so skip this stage
    //

    if (!lpThreadInfo->IsAsyncWorkerThread
    || (lpThreadInfo->NestedRequests > 1)) {

        //
        // validate parameters
        //

        if (IsBadStringPtrW(lpszRemoteFile, INTERNET_MAX_PATH_LENGTH + 1)
        || (*lpszRemoteFile == L'\0')
        || IsBadStringPtrW(lpszNewFile, INTERNET_MAX_PATH_LENGTH + 1)
        || (*lpszNewFile == L'\0')
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
        // if we're performing async I/O AND this we are not in the context of
        // an async worker thread AND the app supplied a non-zero context value
        // then we can make an async request
        //

        if (!lpThreadInfo->IsAsyncWorkerThread
        && isAsync
        && (dwContext != INTERNET_NO_CALLBACK)) {

            //
            // create an asynchronous request block (ARB) and copy the parameters
            //

            // MakeAsyncRequest
            CFsm_FtpGetFile * pFsm;

            pFsm = new CFsm_FtpGetFile(hFtpSession, dwContext, lpszRemoteFile, lpszNewFile, fFailIfExists,
                                            dwFlagsAndAttributes, dwFlags);
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

    //
    // initialize variables in case of early exit
    //

    HANDLE hLocalFile;
    HINTERNET hRemoteFile;
    LPBYTE transferBuffer;

    hLocalFile = INVALID_HANDLE_VALUE;
    hRemoteFile = NULL;
    transferBuffer = NULL;

    //
    // first off, allocate a buffer for the transfer(s) below. The caller can
    // now influence the buffer size used by setting the appropriate option
    // for this handle. If we fail to get the value then revert to the default
    // size of 4K. We want to reduce the number of RPC calls we make
    //

    DWORD optionLength;
    DWORD transferBufferLength;

    optionLength = sizeof(transferBufferLength);
    if (!InternetQueryOption(hFtpSession,
                             INTERNET_OPTION_READ_BUFFER_SIZE,
                             (LPVOID)&transferBufferLength,
                             &optionLength
                             )) {
        transferBufferLength = DEFAULT_TRANSFER_BUFFER_LENGTH;
    }
    transferBuffer = (LPBYTE)ALLOCATE_FIXED_MEMORY(transferBufferLength);
    if (transferBuffer == NULL) {
        goto last_error_exit;
    }

    //
    // open/create local file
    //

    hLocalFile = CreateFileW(lpszNewFile,
                            GENERIC_WRITE,
                            0,
                            NULL,
                            fFailIfExists ? CREATE_NEW : CREATE_ALWAYS,
                            dwFlagsAndAttributes | FILE_FLAG_SEQUENTIAL_SCAN,
                            NULL // need NULL for Win95, handle to file with attributes to copy
                            );

    if (hLocalFile == INVALID_HANDLE_VALUE) {
        goto last_error_exit;
    }

    //
    // open file at FTP server
    //

    hRemoteFile = FtpOpenFileW(hFtpSession,
                               lpszRemoteFile,
                               GENERIC_READ,
                               dwFlags,
                               dwContext
                               );
    if (hRemoteFile == NULL) {
        goto last_error_exit;
    }

    //
    // since we are going under the API, map the file handle
    //

    error = MapHandleToAddress(hRemoteFile, (LPVOID *)&hFileMapped, FALSE);
    if ((error != ERROR_SUCCESS) && (hFileMapped == NULL)) {
        goto cleanup;
    }

    //
    // transfer remote file to local (i.e. download it)
    //

    do {

        DWORD bytesRead;

        //
        // let app invalidate out
        //

        if (((HANDLE_OBJECT *)hFileMapped)->IsInvalidated()
        || ((HANDLE_OBJECT *)hSessionMapped)->IsInvalidated()) {
            error = ERROR_INTERNET_OPERATION_CANCELLED;
            goto cleanup;
        }

        success = FtpReadFile(hFileMapped,
                              transferBuffer,
                              transferBufferLength,
                              &bytesRead
                              );
        if (success) {
            if (bytesRead != 0) {

                DWORD bytesWritten;

                success = WriteFile(hLocalFile,
                                    transferBuffer,
                                    bytesRead,
                                    &bytesWritten,
                                    NULL
                                    );
            } else {

                //
                // done
                //

                error = ERROR_SUCCESS;
                goto cleanup;
            }
        }
    } while (success);

last_error_exit:

    error = GetLastError();

cleanup:

    if (transferBuffer != NULL) {
        FREE_MEMORY((HLOCAL)transferBuffer);
    }

    //
    // close local file
    //

    if (hLocalFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hLocalFile);

        //
        // if we failed, but created the local file then delete it
        //

        if ((error != ERROR_SUCCESS) && (hLocalFile != INVALID_HANDLE_VALUE)) {
            DeleteFileW(lpszNewFile);
        }
    }

    //
    // close remote file
    //

    if (hRemoteFile != NULL) {
        _InternetCloseHandle(hRemoteFile);
    }

    //
    // BUGBUG [arthurbi] SetContext should not be called from here,
    //   InternetCloseHandle should not reset the context as its
    //   doing right now, and this needs to be investigated more,
    //   as for now we're workaround this problem by calling SetContext
    //

    _InternetSetContext(lpThreadInfo, dwContext);

quit:

    if (hFileMapped != NULL) {
        INET_ASSERT(fDeref);
        DereferenceObject((LPVOID)hFileMapped);
    }

    if (hSessionMapped != NULL && fDeref) {
        DereferenceObject((LPVOID)hSessionMapped);
    }

    _InternetDecNestingCount(nestingLevel);;

done:

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
FtpPutFileW(
    IN HINTERNET hFtpSession,
    IN LPCWSTR lpszLocalFile,
    IN LPCWSTR lpszNewRemoteFile,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    hFtpSession         -
    lpszLocalFile       -
    lpszNewRemoteFile   -
    dwFlags             -
    dwContext           -

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "FtpPutFileW",
                     "%#x, %wq, %wq, %#x, %#x",
                     hFtpSession,
                     lpszLocalFile,
                     lpszNewRemoteFile,
                     dwFlags,
                     dwContext
                     ));

    LPINTERNET_THREAD_INFO lpThreadInfo;
    DWORD error;
    BOOL success;
    DWORD nestingLevel = 0;
    HINTERNET hSessionMapped = NULL;
    HINTERNET hFileMapped = NULL;
    BOOL fDeref = TRUE;

    if (!GlobalDataInitialized) {
        error = ERROR_INTERNET_NOT_INITIALIZED;
        goto done;
    }

    //
    // we need the INTERNET_THREAD_INFO
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
    // map the connect handle
    //

    error = MapHandleToAddress(hFtpSession, (LPVOID *)&hSessionMapped, FALSE);
    if ((error != ERROR_SUCCESS) && (hSessionMapped == NULL)) {
        goto quit;
    }

    //
    // set the context and handle info and clear last error variables
    //

    _InternetSetObjectHandle(lpThreadInfo, hFtpSession, hSessionMapped);
    _InternetSetContext(lpThreadInfo, dwContext);
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

    error = RIsHandleLocal(hSessionMapped,
                           &isLocal,
                           &isAsync,
                           TypeFtpConnectHandle
                           );
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // if we're in the async thread context then we've already validated the
    // parameters, so skip this stage
    //

    if (!lpThreadInfo->IsAsyncWorkerThread
    || (lpThreadInfo->NestedRequests > 1)) {

        //
        // validate parameters
        //

        if (IsBadStringPtrW(lpszNewRemoteFile, INTERNET_MAX_PATH_LENGTH + 1)
        || (*lpszNewRemoteFile == L'\0')
        || IsBadStringPtrW(lpszLocalFile, INTERNET_MAX_PATH_LENGTH + 1)
        || (*lpszLocalFile == L'\0')
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

        // in offline mode modifications are disallowed
        // someday we will do internet briefcase but not today

        if ((((INTERNET_CONNECT_HANDLE_OBJECT *)hSessionMapped)->
                GetInternetOpenFlags() | dwFlags) &
                INTERNET_FLAG_OFFLINE) {
            error = ERROR_WRITE_PROTECT;
            goto quit;
        }

        //
        // if we're performing async I/O AND this we are not in the context of
        // an async worker thread AND the app supplied a non-zero context value
        // then we can make an async request
        //

        if (!lpThreadInfo->IsAsyncWorkerThread
        && isAsync
        && (dwContext != INTERNET_NO_CALLBACK)) {

            // MakeAsyncRequest
            CFsm_FtpPutFile * pFsm;

            pFsm = new CFsm_FtpPutFile(hFtpSession, dwContext, lpszLocalFile, lpszNewRemoteFile, dwFlags);
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

    //
    // initialize variables in case of early exit
    //

    HANDLE hLocalFile;
    HINTERNET hRemoteFile;
    LPBYTE transferBuffer;

    hLocalFile = INVALID_HANDLE_VALUE;
    hRemoteFile = NULL;
    transferBuffer = NULL;

    //
    // first off, allocate a buffer for the transfer(s) below. The caller can
    // now influence the buffer size used by setting the appropriate option
    // for this handle. If we fail to get the value then revert to the default
    // size of 4K. We want to reduce the number of RPC calls we make
    //

    DWORD optionLength;
    DWORD transferBufferLength;

    optionLength = sizeof(transferBufferLength);
    if (!InternetQueryOption(hFtpSession,
                             INTERNET_OPTION_WRITE_BUFFER_SIZE,
                             (LPVOID)&transferBufferLength,
                             &optionLength
                             )) {
        transferBufferLength = DEFAULT_TRANSFER_BUFFER_LENGTH;
    }
    transferBuffer = (LPBYTE)ALLOCATE_FIXED_MEMORY(transferBufferLength);
    if (transferBuffer == NULL) {
        goto last_error_exit;
    }

    //
    // open local file
    //

    hLocalFile = CreateFileW(lpszLocalFile,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            FILE_FLAG_SEQUENTIAL_SCAN,
                            NULL // need NULL for Win95, handle to file with attributes to copy
                            );
    if (hLocalFile == INVALID_HANDLE_VALUE) {
        goto last_error_exit;
    }

    //
    // open file at FTP server
    //

    hRemoteFile = FtpOpenFileW(hFtpSession,
                               lpszNewRemoteFile,
                               GENERIC_WRITE,
                               dwFlags,
                               dwContext
                               );
    if (hRemoteFile == NULL) {
        goto last_error_exit;
    }

    //
    // since we are going under the API, map the file handle
    //

    error = MapHandleToAddress(hRemoteFile, (LPVOID *)&hFileMapped, FALSE);
    if ((error != ERROR_SUCCESS) && (hFileMapped == NULL)) {
        goto cleanup;
    }

    //
    // transfer local file to remote (i.e. upload it)
    //

    do {

        DWORD bytesRead;

        //
        // let app invalidate out
        //

        if (((HANDLE_OBJECT *)hFileMapped)->IsInvalidated()
        || ((HANDLE_OBJECT *)hSessionMapped)->IsInvalidated()) {
            error = ERROR_INTERNET_OPERATION_CANCELLED;
            goto cleanup;
        }

        success = ReadFile(hLocalFile,
                           transferBuffer,
                           transferBufferLength,
                           &bytesRead,
                           NULL
                           );
        if (success) {
            if (bytesRead != 0) {

                DWORD bytesWritten;

                success = FtpWriteFile(hFileMapped,
                                       transferBuffer,
                                       bytesRead,
                                       &bytesWritten
                                       );
            } else {

                //
                // ensure last error is really no error
                //

                error = ERROR_SUCCESS;

                goto cleanup;
            }
        }
    } while (success);

last_error_exit:

    error = GetLastError();

cleanup:

    if (transferBuffer != NULL) {
        FREE_MEMORY((HLOCAL)transferBuffer);
    }

    //
    // close local file
    //

    if (hLocalFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hLocalFile);
    }

    //
    // close remote file
    //

    if (hRemoteFile != NULL) {
        _InternetCloseHandle(hRemoteFile);
    }

    //
    // BUGBUG [arthurbi] SetContext should not be called from here,
    //   InternetCloseHandle should not reset the context as its
    //   doing right now, and this needs to be investigated more,
    //   as for now we're workaround this problem by calling SetContext
    //

    _InternetSetContext(lpThreadInfo, dwContext);

quit:

    if (hFileMapped != NULL) {
        DereferenceObject((LPVOID)hFileMapped);
    }

    if (hSessionMapped != NULL && fDeref) {
        DereferenceObject((LPVOID)hSessionMapped);
    }

    _InternetDecNestingCount(nestingLevel);;

done:

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
FtpGetFileEx(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszRemoteFile,
    IN LPCWSTR lpszNewFile,
    IN BOOL fFailIfExists,
    IN DWORD dwFlagsAndAttributes,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "FtpGetFileEx",
                     "%#x, %sq, %wq, %B, %#x, %#x, %#x",
                     hFtpSession,
                     lpszRemoteFile,
                     lpszNewFile,
                     fFailIfExists,
                     dwFlagsAndAttributes,
                     dwFlags,
                     dwContext
                     ));

    DWORD cc, dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    PWSTR pwszRemoteFile = NULL;

    cc = MultiByteToWideChar(CP_ACP, 0, lpszRemoteFile, -1, NULL, 0);
    pwszRemoteFile = (LPWSTR)ALLOCATE_FIXED_MEMORY((cc*sizeof(WCHAR)));
    if (!pwszRemoteFile)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    MultiByteToWideChar(CP_ACP, 0, lpszRemoteFile, -1, pwszRemoteFile, cc);

    fResult = FtpGetFileW(hFtpSession,pwszRemoteFile,lpszNewFile,fFailIfExists,
        dwFlagsAndAttributes,dwFlags,dwContext);

cleanup: 
    if (pwszRemoteFile)
    {
        FREE_MEMORY(pwszRemoteFile);
    }
    
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;

}


INTERNETAPI
BOOL
WINAPI
FtpPutFileEx(
    IN HINTERNET hFtpSession,
    IN LPCWSTR lpszLocalFile,
    IN LPCSTR lpszNewRemoteFile,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "FtpPutFileEx",
                     "%#x, %wq, %sq, %#x, %#x",
                     hFtpSession,
                     lpszLocalFile,
                     lpszNewRemoteFile,
                     dwFlags,
                     dwContext
                     ));

    DWORD cc, dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    PWSTR pwszRemoteFile = NULL;

    cc = MultiByteToWideChar(CP_ACP, 0, lpszNewRemoteFile, -1, NULL, 0);
    pwszRemoteFile = (LPWSTR)ALLOCATE_FIXED_MEMORY((cc*sizeof(WCHAR)));
    if (!pwszRemoteFile)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    MultiByteToWideChar(CP_ACP, 0, lpszNewRemoteFile, -1, pwszRemoteFile, cc);

    fResult = FtpPutFileW(hFtpSession,lpszLocalFile,pwszRemoteFile,dwFlags,dwContext);

cleanup: 
    if (pwszRemoteFile)
    {
        FREE_MEMORY(pwszRemoteFile);
    }
    
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;

}


INTERNETAPI
BOOL
WINAPI
FtpDeleteFileW(
    IN HINTERNET hFtpSession,
    IN LPCWSTR lpszFileName
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    hFtpSession     -
    lpszFileName    -

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "FtpDeleteFileW",
                     "%#x, %wq",
                     hFtpSession,
                     lpszFileName
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    MEMORYPACKET mpFile;

    if (!lpszFileName)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    ALLOC_MB(lpszFileName,0,mpFile);
    if (!mpFile.psStr)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    UNICODE_TO_ANSI(lpszFileName,mpFile);
    fResult = FtpDeleteFileA(hFtpSession,mpFile.psStr);

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}


INTERNETAPI
BOOL
WINAPI
FtpRenameFileW(
    IN HINTERNET hFtpSession,
    IN LPCWSTR lpszExisting,
    IN LPCWSTR lpszNew
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    hFtpSession     -
    lpszExisting    -
    lpszNew         -

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "FtpRenameFileW",
                     "%#x, %wq, %wq",
                     hFtpSession,
                     lpszExisting,
                     lpszNew
                     ));


    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    MEMORYPACKET mpExisting, mpNew;

    if (!(lpszExisting && lpszNew))
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    ALLOC_MB(lpszExisting,0,mpExisting);
    ALLOC_MB(lpszNew,0,mpNew);
    if (!(mpExisting.psStr && mpNew.psStr))
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    UNICODE_TO_ANSI(lpszExisting,mpExisting);
    UNICODE_TO_ANSI(lpszNew,mpNew);
    fResult = FtpRenameFileA(hFtpSession,mpExisting.psStr,mpNew.psStr);

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}


INTERNETAPI
HINTERNET
WINAPI
FtpOpenFileW(
    IN HINTERNET hFtpSession,
    IN LPCWSTR lpszFileName,
    IN DWORD dwAccess,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    hFtpSession     -
    lpszFileName    -
    dwAccess        -
    dwFlags         -
    dwContext       -

Return Value:

    HINTERNET

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Handle,
                     "FtpOpenFileW",
                     "%#x, %wq, %#x, %#x, %#x",
                     hFtpSession,
                     lpszFileName,
                     dwAccess,
                     dwFlags,
                     dwContext
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    HINTERNET hInternet = NULL;
    MEMORYPACKET mpFile;

    if (!lpszFileName)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    ALLOC_MB(lpszFileName,0,mpFile);
    if (!mpFile.psStr)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    UNICODE_TO_ANSI(lpszFileName,mpFile);
    hInternet = FtpOpenFileA(hFtpSession,mpFile.psStr,dwAccess,dwFlags,dwContext);

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);

    }
    DEBUG_LEAVE_API(hInternet);
    return hInternet;
}


INTERNETAPI
BOOL
WINAPI
FtpCreateDirectoryW(
    IN HINTERNET hFtpSession,
    IN LPCWSTR pwszDir
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    hFtpSession -
    pwszDir     -

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "FtpCreateDirectoryW",
                     "%#x, %wq",
                     hFtpSession,
                     pwszDir
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    MEMORYPACKET mpDir;

    if (!pwszDir)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    ALLOC_MB(pwszDir,0,mpDir);
    if (!mpDir.psStr)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    UNICODE_TO_ANSI(pwszDir,mpDir);
    fResult = FtpCreateDirectoryA(hFtpSession,mpDir.psStr);

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}


INTERNETAPI
BOOL
WINAPI
FtpRemoveDirectoryW(
    IN HINTERNET hFtpSession,
    IN LPCWSTR pwszDir
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    hFtpSession -
    pwszDir     -

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "FtpRemoveDirectoryW",
                     "%#x, %wq",
                     hFtpSession,
                     pwszDir
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    MEMORYPACKET mpDir;

    if (!pwszDir)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    ALLOC_MB(pwszDir,0,mpDir);
    if (!mpDir.psStr)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    UNICODE_TO_ANSI(pwszDir,mpDir);
    fResult = FtpRemoveDirectoryA(hFtpSession,mpDir.psStr);

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}


INTERNETAPI
BOOL
WINAPI
FtpSetCurrentDirectoryW(
    IN HINTERNET hFtpSession,
    IN LPCWSTR pwszDir
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    hFtpSession -
    pwszDir     -

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "FtpSetCurrentDirectoryW",
                     "%#x, %wq",
                     hFtpSession,
                     pwszDir
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    MEMORYPACKET mpDir;

    if (!pwszDir)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    ALLOC_MB(pwszDir,0,mpDir);
    if (!mpDir.psStr)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    UNICODE_TO_ANSI(pwszDir,mpDir);
    fResult = FtpSetCurrentDirectoryA(hFtpSession,mpDir.psStr);

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}


INTERNETAPI
BOOL
WINAPI
FtpGetCurrentDirectoryW(
    IN HINTERNET hFtpSession,
    OUT LPWSTR lpszCurrentDirectory,
    IN OUT LPDWORD lpdwCurrentDirectory
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    hFtpSession             -
    lpszCurrentDirectory    -
    lpdwCurrentDirectory    -

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "FtpGetCurrentDirectoryW",
                     "%#x, %#x, %#x [%d]",
                     hFtpSession,
                     lpszCurrentDirectory,
                     lpdwCurrentDirectory,
                     lpdwCurrentDirectory ? *lpdwCurrentDirectory : 0
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    MEMORYPACKET mpDir;

    if (!lpdwCurrentDirectory)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    if (lpszCurrentDirectory)
    {
        mpDir.dwSize = mpDir.dwAlloc = *lpdwCurrentDirectory*sizeof(CHAR);
        mpDir.psStr = (LPSTR)ALLOC_BYTES(mpDir.dwAlloc);
        if (!mpDir.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }
    fResult = FtpGetCurrentDirectoryA(hFtpSession,mpDir.psStr,&mpDir.dwSize);
    if (fResult) 
    {
        *lpdwCurrentDirectory = (MultiByteToWideChar(CP_ACP, 0, mpDir.psStr, -1, 
                    lpszCurrentDirectory, *lpdwCurrentDirectory/sizeof(WCHAR))+1)*sizeof(WCHAR);
        if (*lpdwCurrentDirectory<=mpDir.dwAlloc)
        {
            MultiByteToWideChar(CP_ACP, 0, mpDir.psStr, -1, 
                    lpszCurrentDirectory, *lpdwCurrentDirectory);
        }
        else
        {
            dwErr = ERROR_INSUFFICIENT_BUFFER;
            fResult = FALSE;
        }
    }
    else
    {
        if (GetLastError()==ERROR_INSUFFICIENT_BUFFER)
        {
            *lpdwCurrentDirectory = mpDir.dwSize*sizeof(WCHAR);
        }
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


INTERNETAPI
BOOL
WINAPI
FtpCommandW(
    IN HINTERNET hFtpSession,
    IN BOOL fExpectResponse,
    IN DWORD dwFlags,
    IN LPCWSTR lpszCommand,
    IN DWORD_PTR dwContext,
    OUT HINTERNET *phFtpCommand OPTIONAL
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    hFtpSession     -
    fExpectResponse -
    dwFlags         -
    lpszCommand     -
    dwContext       -
    phFtpCommand    -

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "FtpCommandW",
                     "%#x, %B, %#x, %wq, %#x, %x",
                     hFtpSession,
                     fExpectResponse,
                     dwFlags,
                     lpszCommand,
                     dwContext,
                     phFtpCommand
                     ));

    MEMORYPACKET mpCommand;
    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    if (!lpszCommand)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    ALLOC_MB(lpszCommand, 0, mpCommand);
    if (!mpCommand.psStr)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    UNICODE_TO_ANSI(lpszCommand, mpCommand);

    fResult = FtpCommandA(hFtpSession, fExpectResponse, dwFlags, mpCommand.psStr, dwContext, phFtpCommand);

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}

