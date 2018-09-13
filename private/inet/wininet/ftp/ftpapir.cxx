/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    ftpapir.cxx

Abstract:

    Contains the remote-side FTP API worker functions. In each case, the API
    proper validates the arguments. The worker functions contained herein just
    perform the requested operation with the supplied arguments.

    These functions are the remote side of the RPC interface. If the DLL is
    the abstract0 version (no RPC) then the A forms of the functions simply
    call the w functions

    Contents:
        wFtpFindFirstFile
        wFtpDeleteFile
        wFtpRenameFile
        wFtpOpenFile
        wFtpCreateDirectory
        wFtpRemoveDirectory
        wFtpSetCurrentDirectory
        wFtpGetCurrentDirectory
        wFtpCommand
        wFtpFindNextFile
        wFtpFindClose
        wFtpConnect
        wFtpMakeConnection
        wFtpDisconnect
        wFtpReadFile
        wFtpWriteFile
        wFtpQueryDataAvailable
        wFtpCloseFile
        wFtpFindServerType
        wFtpGetFileSize

Author:

    Heath Hunnicutt [t-heathh] 13-Jul-1994

Environment:

    Win32(s) user-level DLL

Revision History:

    09-Mar-1995 rfirth
        Created new file/worker functions from functions contained in
        findfile.c, ftphelp.c

--*/

#include <wininetp.h>
#include "ftpapih.h"

//
// private macros
//

#define CASE_OF(constant)   case constant: return # constant

//
// private debug functions
//

#if INET_DEBUG

PRIVATE
DEBUG_FUNCTION
LPSTR
InternetMapFtpServerType(
    IN FTP_SERVER_TYPE ServerType
    );

#else

#define InternetMapFtpServerType(x) (VOID)(x)

#endif // INET_DEBUG

//
// external functions
//

extern
DWORD
InbLocalEndCacheWrite(
    IN HINTERNET hFtpFile,
    IN LPSTR lpszFileExtension,
    IN BOOL fNormal
    );

//
// functions
//


DWORD
wFtpFindFirstFile(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszFilespec,
    OUT LPWIN32_FIND_DATA lpFindFileData OPTIONAL,
    OUT LPHINTERNET lphInternet
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

    lpszFilespec    - Pointer to a string containing a file specification
                      to find. May be empty, but not NULL

    lpFindFileData  - Pointer to a buffer that will contain WIN32_FIND_DATA
                      information when this call succeeds.
                      If this parameter is not supplied, then any find data
                      will be returned via InternetFindNextFile()

    lphInternet     - place to return open find handle

Return Value:

    DWORD
        Success - ERROR_SUCCESS
                    *lphInternet contains new find handle

        Failure - ERROR_INVALID_HANDLE
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
                Dword,
                "wFtpFindFirstFile",
                "%#x, %q, %#x, %#x",
                hFtpSession,
                lpszFilespec,
                lpFindFileData,
                lphInternet
                ));

    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();
    LPSTR lpBuffer = NULL;
    DWORD error;

    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    LPFTP_SESSION_INFO lpSessionInfo;

    if (!FindFtpSession(hFtpSession, &lpSessionInfo)) {
        error = ERROR_INVALID_HANDLE;
        goto quit;
    }

    //
    // acquire the session lock while we check and optionally set the active
    // find flag
    //

    AcquireFtpSessionLock(lpSessionInfo);

    if (!(lpSessionInfo->Flags & FFTP_FIND_ACTIVE)) {
        lpSessionInfo->Flags |= FFTP_FIND_ACTIVE;
        error = ERROR_SUCCESS;
    } else {
        error = ERROR_FTP_TRANSFER_IN_PROGRESS;
    }

    ReleaseFtpSessionLock(lpSessionInfo);

    //
    // if we already have a directory listing on this connection, then we can
    // not allow another one, until the current listing is cleared out by the
    // app calling InternetCloseHandle()
    //

    if (error != ERROR_SUCCESS) {
        goto deref_exit;
    }

    //
    // the filespec may have a path component. We assume that any wild-cards
    // will only be in the filename part. We use the path part in the directory
    // request and the filename part when parsing the directory output
    //

    char pathBuf[INTERNET_MAX_PATH_LENGTH + 1];
    LPSTR lpszPathPart;
    LPSTR lpszFilePart;
    BOOL isWild;
    DWORD dwFilePartLength;

    lpszFilePart = (LPSTR)lpszFilespec;
    lpszPathPart = NULL;
    dwFilePartLength = lstrlen(lpszFilePart);

    if (*lpszFilePart != '\0') {

        LPSTR pathSeparator;

        pathSeparator = _memrchr(lpszFilePart, '\\', dwFilePartLength);
        if (pathSeparator == NULL) {
            pathSeparator = _memrchr(lpszFilePart, '/', dwFilePartLength);
        }
        if (pathSeparator != NULL) {

            int len = (int) (pathSeparator - lpszFilePart) + 1;

            if (len < sizeof(pathBuf)) {
                memcpy(pathBuf, lpszFilePart, len);
                pathBuf[len] = '\0';
                lpszPathPart = pathBuf;
                lpszFilePart = pathSeparator + 1;

                DEBUG_PRINT(FTP,
                            INFO,
                            ("lpszPathPart = %q, lpszFilePart = %q\n",
                            lpszPathPart,
                            lpszFilePart
                            ));

            }
        }

        //
        // determine whether the caller is asking for a fuzzy file match, or
        // (typically) the request is for the contents of a directory
        //

        isWild = IsFilespecWild(lpszFilePart);
    } else {

        //
        // empty string - not asking for wildcard search
        //

        isWild = FALSE;
    }

    //
    // and ask the FTP server for the directory listing
    //

    FTP_RESPONSE_CODE rcResponse;

    error = Command(lpSessionInfo,
                    TRUE,
                    FTP_TRANSFER_TYPE_ASCII,
                    &rcResponse,
                    ((lpszPathPart == NULL) && (isWild || (*lpszFilePart == '\0')))
                        ? "LIST"
                        : "LIST %s",
                    (lpszPathPart == NULL)
                        ? lpszFilePart
                        : isWild
                            ? lpszPathPart
                            : lpszFilespec
                    );

    //
    // quit early if we failed to send the command, or the server didn't
    // understand it
    //

    if (error != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // presumably, the server has sent us a directory listing. Receive it
    //

    DWORD bufferLength;
    DWORD bufferLeft;
    DWORD bytesReceived;
    BOOL eof;

    bufferLength = 0;
    bufferLeft = 0;
    bytesReceived = 0;

    error = lpSessionInfo->socketData->Receive((LPVOID *)&lpBuffer,
                                               &bufferLength,
                                               &bufferLeft,
                                               &bytesReceived,
                                               0,
                                               SF_EXPAND
                                               | SF_COMPRESS
                                               | SF_RECEIVE_ALL
                                               | SF_INDICATE,
                                               &eof
                                               );

    //
    // we are done with the data connection
    //

    lpSessionInfo->socketData->Close();

    //
    // quit now if we had an error while receiving
    //

    if (error != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // if the previous response was preliminary then get the final response from
    // the FTP server
    //

    if (rcResponse.Major != FTP_RESPONSE_COMPLETE) {
        error = GetReply(lpSessionInfo, &rcResponse);
        if (error != ERROR_SUCCESS) {
            goto cleanup;
        }

        //
        // check response for failure
        //

        if (rcResponse.Major != FTP_RESPONSE_COMPLETE) {

            //
            // <-- Return "command failed" error code
            //

            error = ERROR_INTERNET_EXTENDED_ERROR;
            goto cleanup;
        }
    }

    if (bytesReceived == 0) {

        DEBUG_PRINT(WORKER,
                    ERROR,
                    ("ReceiveData() returns 0 bytes\n"
                    ));

        error = ERROR_NO_MORE_FILES;
        goto cleanup;
    }

    //
    // trap bad servers which return a not-found message in the data stream. We
    // only do this if we are not performing a wild-card search (because the
    // wild-card match will fail to match anything if the target file or path
    // cannot be found)
    //

    LPSTR lpszSearch;
    DWORD dwSearch;

    lpszSearch = (lpszPathPart == NULL) ? lpszFilePart : (LPSTR)lpszFilespec;
    dwSearch = lstrlen(lpszSearch);

    if (!isWild && (bytesReceived > dwSearch)) {
        if (!_strnicmp(lpBuffer, lpszSearch, dwSearch)
        && (lpBuffer[dwSearch] == ':')) {

            static char testChars[] = {'\r', '\n', '\0'};
            LPSTR lpStartOfString = lpBuffer + dwSearch + 1;
            LPSTR lpEndOfString;

            for (int i = 0; i < ARRAY_ELEMENTS(testChars); ++i) {
                lpEndOfString = strchr(lpStartOfString, testChars[i]);
                if (lpEndOfString != NULL) {
                    break;
                }
            }

            //
            // we should have found at least one of the target characters
            //

            INET_ASSERT(lpEndOfString != NULL);

            if (lpEndOfString != NULL) {

                int lengthToTest = (int) (lpEndOfString - lpStartOfString);

                //
                // BUGBUG - internationalization?
                //

                if (strnistr(lpStartOfString, "not found", lengthToTest)
                || strnistr(lpStartOfString, "cannot find", lengthToTest)) {
                    error = ERROR_NO_MORE_FILES;
                    goto cleanup;
                }
            } else {
                error = ERROR_INTERNET_INTERNAL_ERROR;
                goto cleanup;
            }
        }
    }

    INET_ASSERT(lpBuffer != NULL);
    INET_ASSERT((int)bytesReceived > 0);

    error = ParseDirList(lpBuffer,
                         bytesReceived,
                         isWild ? (LPSTR)lpszFilePart : NULL,
                         &lpSessionInfo->FindFileList
                         );

    //
    // ParseDirList() may have failed
    //

    if (error != ERROR_SUCCESS) {
        goto cleanup;
    }

    //
    // if there's nothing in the list then no files matching the caller's
    // specification were found
    //

    if (IsListEmpty(&lpSessionInfo->FindFileList)) {
        error = ERROR_NO_MORE_FILES;
    } else {

        //
        // if the caller supplied an output buffer then return the first entry
        // and remove it from the list
        //

        if (ARGUMENT_PRESENT(lpFindFileData)) {

            PLIST_ENTRY pEntry;

            pEntry = RemoveHeadList(&lpSessionInfo->FindFileList);
            CopyMemory(lpFindFileData,
                       (LPWIN32_FIND_DATA)(pEntry + 1),
                       sizeof(*lpFindFileData)
                       );
            FREE_MEMORY(pEntry);
        }

        //
        // FTP can only have one active operation per session, so we just return
        // this session handle as the find handle
        //

        *lphInternet = hFtpSession;
        error = ERROR_SUCCESS;
    }

cleanup:

    if (lpSessionInfo->socketData->IsValid()) {
        lpSessionInfo->socketData->SetLinger(TRUE, 0);
        lpSessionInfo->socketData->Close();
    }

    if (lpBuffer != NULL) {
        (void)FREE_MEMORY((HLOCAL)lpBuffer);
    }

    //
    // if we failed then reset the active find flag. We set it, so we know it
    // is safe to reset without acquiring the session lock
    //

    if (error != ERROR_SUCCESS) {
        lpSessionInfo->Flags &= ~FFTP_FIND_ACTIVE;
    }

deref_exit:

    DereferenceFtpSession(lpSessionInfo);

quit:

    DEBUG_LEAVE(error);

    return error;
}


DWORD
wFtpDeleteFile(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszFileName
    )

/*++

Routine Description:

    Deletes a file at an FTP server

Arguments:

    hFtpSession     - identifies the FTP server

    lpszFileName    - name of file to delete

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_FTP,
                Dword,
                "wFtpDeleteFile",
                "%#x, %q",
                hFtpSession,
                lpszFileName
                ));

    LPFTP_SESSION_INFO lpSessionInfo;
    DWORD error;

    if (FindFtpSession(hFtpSession, &lpSessionInfo)) {

        FTP_RESPONSE_CODE rcResponse;

        error = Command(lpSessionInfo,
                        FALSE,
                        FTP_TRANSFER_TYPE_UNKNOWN,
                        &rcResponse,
                        "DELE %s",
                        lpszFileName
                        );

        if ((error == ERROR_SUCCESS)
        && (rcResponse.Major != FTP_RESPONSE_COMPLETE)) {
            error = ERROR_INTERNET_EXTENDED_ERROR;
        }
        DereferenceFtpSession(lpSessionInfo);
    } else {
        error = ERROR_INVALID_HANDLE;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
wFtpRenameFile(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszExisting,
    IN LPCSTR lpszNew
    )

/*++

Routine Description:

    Renames a file at an FTP server

Arguments:

    hFtpSession     - identifies FTP server

    lpszExisting    - current file name

    lpszNew         - new file name

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_FTP,
                Dword,
                "wFtpRenameFile",
                "%#x, %q, %q",
                hFtpSession,
                lpszExisting,
                lpszNew
                ));

    LPFTP_SESSION_INFO lpSessionInfo;
    DWORD error;

    if (FindFtpSession(hFtpSession, &lpSessionInfo)) {

        FTP_RESPONSE_CODE rcResponse;

        error = Command(lpSessionInfo,
                        FALSE,
                        FTP_TRANSFER_TYPE_UNKNOWN,
                        &rcResponse,
                        "RNFR %s",
                        lpszExisting
                        );

        if ((error == ERROR_SUCCESS)
        && (rcResponse.Major != FTP_RESPONSE_CONTINUE)) {
            error = ERROR_INTERNET_EXTENDED_ERROR;
        }
        if (error == ERROR_SUCCESS) {
            error = Command(lpSessionInfo,
                            FALSE,
                            FTP_TRANSFER_TYPE_UNKNOWN,
                            &rcResponse,
                            "RNTO %s",
                            lpszNew
                            );
            if ((error == ERROR_SUCCESS)
            && (rcResponse.Major != FTP_RESPONSE_COMPLETE)) {
                error = ERROR_INTERNET_EXTENDED_ERROR;
            }
        }
        DereferenceFtpSession(lpSessionInfo);
    } else {
        error = ERROR_INVALID_HANDLE;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
wFtpOpenFile(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszFileName,
    IN DWORD dwAccess,
    IN DWORD dwFlags,
    OUT LPHINTERNET lphInternet
    )

/*++

Routine Description:

    Initiates the connection to read or write a file at the FTP server

Arguments:

    hFtpSession     - identifies FTP server

    lpszFileName    - name of file to open

    dwAccess        - access mode - GENERIC_READ or GENERIC_WRITE

    dwFlags         - flags controlling how to transfer the data

    lphInternet     - where to return the open file handle

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_FTP,
                Dword,
                "wFtpOpenFile",
                "%#x, %q, %#x, %#x, %#x",
                hFtpSession,
                lpszFileName,
                dwAccess,
                dwFlags,
                lphInternet
                ));

    LPFTP_SESSION_INFO lpSessionInfo;
    DWORD error;

    if (FindFtpSession(hFtpSession, &lpSessionInfo)) {

        //
        // control session must be established
        //

        if (! lpSessionInfo->socketControl->IsValid()) {
            error = ERROR_FTP_DROPPED;
        } else if ((lpSessionInfo->socketData->IsValid())
        || (lpSessionInfo->Flags & FFTP_FILE_ACTIVE)) {

            //
            // there is a (file) transfer in progress if the socket is valid,
            // or we are awaiting a call to InternetCloseHandle() before we can
            // open another file (FFTP_FILE_ACTIVE is set. This stops another
            // thread from closing our socket handle)
            //

            error = ERROR_FTP_TRANSFER_IN_PROGRESS;
        } else {

            FTP_RESPONSE_CODE rcResponse;

            INET_ASSERT(!lpSessionInfo->socketData->IsValid());

            //
            // Clear the session's "known size bit" before we download the next file,
            //  this is to make sure we don't read an extranous size value off it.
            //

            lpSessionInfo->Flags &= ~(FFTP_KNOWN_FILE_SIZE);

            //
            // send the connection set-up commands, and issue either the send
            // or the receive command
            //
            // Either "RETR filename" or "STOR filename"
            //

            error = NegotiateDataConnection(lpSessionInfo,
                                            dwFlags,
                                            &rcResponse,
                                            (dwAccess & GENERIC_READ)
                                                 ? "RETR %s"
                                                 : "STOR %s",
                                            lpszFileName
                                            );

            if (error == ERROR_SUCCESS) {

                //
                // Check response for failure
                //

                if ((rcResponse.Major != FTP_RESPONSE_PRELIMINARY)
                && (rcResponse.Major != FTP_RESPONSE_COMPLETE)) {

                    ICSocket * socketData;

                    //
                    // BUGBUG - RLF - don't know if this is what's intended
                    //          here, but the code just used to check
                    //          socketData != INVALID_SOCKET. Since socketData
                    //          was getting set to INVALID_SOCKET at the top
                    //          of this routine, this branch would never be
                    //          taken
                    //

                    socketData = lpSessionInfo->socketData;
                    if (socketData->IsValid()) {
                        ResetSocket(socketData);
                    }
                    error = ERROR_INTERNET_EXTENDED_ERROR;
                } else {

                    lpSessionInfo->dwTransferAccess = dwAccess;

                    //
                    // Some FTP servers will send us back both the preliminary
                    // response and the complete response so quickly that we
                    // will never see the preliminary.
                    //
                    // In order for FtpCloseFile() to know that the completion
                    // response has been received, we store the response
                    // structure in the Session Info.
                    //
                    // The response structure only needs to be stored between
                    // API calls in this situation, it is not generally
                    // referred to.
                    //

                    SetSessionLastResponseCode(lpSessionInfo, &rcResponse);

                    //
                    // set the abort flag if the file was opened for read - this
                    // lets the server know it can clean up the session if we
                    // close early
                    //

                    if (dwAccess & GENERIC_READ) {
                        lpSessionInfo->Flags |= FFTP_ABORT_TRANSFER;
                    }

                    //
                    // FTP can only have one active operation per session, so
                    // we just return this session handle as the find handle
                    //

                    *lphInternet = hFtpSession;

                    //
                    // this session has an active file operation
                    //

                    lpSessionInfo->Flags |= FFTP_FILE_ACTIVE;

                    //
                    // N.B. error == ERROR_SUCCESS from above test after call
                    // to NegotiateDataConnection
                    //

                    INET_ASSERT(error == ERROR_SUCCESS);
                }
            }
        }
        DereferenceFtpSession(lpSessionInfo);
    } else {
        error = ERROR_INVALID_HANDLE;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
wFtpCreateDirectory(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszDirectory
    )

/*++

Routine Description:

    Creates a directory at the FTP server

Arguments:

    hFtpSession     - identifies the FTP server

    lpszDirectory   - directory to create

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_FTP,
                Dword,
                "wFtpCreateDirectory",
                "%#x, %q",
                hFtpSession,
                lpszDirectory
                ));

    LPFTP_SESSION_INFO lpSessionInfo;
    DWORD error;

    if (FindFtpSession(hFtpSession, &lpSessionInfo)) {

        FTP_RESPONSE_CODE rcResponse;

        error = Command(lpSessionInfo,
                        FALSE,
                        FTP_TRANSFER_TYPE_UNKNOWN,
                        &rcResponse,
                        "MKD %s",
                        lpszDirectory
                        );
        if ((error == ERROR_SUCCESS)
        && (rcResponse.Major != FTP_RESPONSE_COMPLETE)) {
            error = ERROR_INTERNET_EXTENDED_ERROR;
        }
        DereferenceFtpSession(lpSessionInfo);
    } else {
        error = ERROR_INVALID_HANDLE;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
wFtpRemoveDirectory(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszDirectory
    )

/*++

Routine Description:

    Removes the named directory at the FTP server

Arguments:

    hFtpSession     - identifies the FTP server

    lpszDirectory   - directory to remove

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_FTP,
                Dword,
                "wFtpRemoveDirectory",
                "%#x, %q",
                hFtpSession,
                lpszDirectory
                ));

    LPFTP_SESSION_INFO lpSessionInfo;
    DWORD error;

    if (FindFtpSession(hFtpSession, &lpSessionInfo)) {

        FTP_RESPONSE_CODE rcResponse;

        error = Command(lpSessionInfo,
                        FALSE,
                        FTP_TRANSFER_TYPE_UNKNOWN,
                        &rcResponse,
                        "RMD %s",
                        lpszDirectory
                        );
        if ((error == ERROR_SUCCESS)
        && (rcResponse.Major != FTP_RESPONSE_COMPLETE)) {
            error = ERROR_INTERNET_EXTENDED_ERROR;
        }
        DereferenceFtpSession(lpSessionInfo);
    } else {
        error = ERROR_INVALID_HANDLE;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
wFtpSetCurrentDirectory(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszDirectory
    )

/*++

Routine Description:

    Sets the current directory for this FTP server session

Arguments:

    hFtpSession     - identifies the FTP server/session

    lpszDirectory   - name of directory to set

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_FTP,
                Dword,
                "wFtpSetCurrentDirectory",
                "%#x, %q",
                hFtpSession,
                lpszDirectory
                ));

    LPFTP_SESSION_INFO lpSessionInfo;
    DWORD error;

    if (FindFtpSession(hFtpSession, &lpSessionInfo)) {

        FTP_RESPONSE_CODE rcResponse;

        error = Command(lpSessionInfo,
                        FALSE,
                        FTP_TRANSFER_TYPE_UNKNOWN,
                        &rcResponse,
                        "CWD %s",
                        lpszDirectory
                        );
        if ((error == ERROR_SUCCESS)
        && (rcResponse.Major != FTP_RESPONSE_COMPLETE)) {
            error = ERROR_INTERNET_EXTENDED_ERROR;
        }
        DereferenceFtpSession(lpSessionInfo);
    } else {
        error = ERROR_INVALID_HANDLE;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
wFtpGetCurrentDirectory(
    IN HINTERNET hFtpSession,
    IN DWORD cchCurrentDirectory,
    OUT LPSTR lpszCurrentDirectory,
    OUT LPDWORD lpdwBytesReturned
    )

/*++

Routine Description:

    Gets the current working directory at the FTP server for this session

Arguments:

    hFtpSession             - identifies FTP server

    cchCurrentDirectory     - number of characters in lpszCurrentDirectory

    lpszCurrentDirectory    - buffer where current directory string is written

    lpdwBytesReturned       - number of characters in output string NOT including
                              terminating NUL

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_HANDLE

                  ERROR_INSUFFICIENT_BUFFER
                    The buffer in lpszCurrentDirectory is not large enough to
                    hold the directory string. *lpdwBytesReturned will have
                    the required size

--*/

{
    DEBUG_ENTER((DBG_FTP,
                Dword,
                "wFtpGetCurrentDirectory",
                "%#x, %d, %#x, %#x",
                hFtpSession,
                cchCurrentDirectory,
                lpszCurrentDirectory,
                lpdwBytesReturned
                ));

    LPFTP_SESSION_INFO lpSessionInfo;
    DWORD cchCopied;
    DWORD error;

    if (FindFtpSession(hFtpSession, &lpSessionInfo)) {

        FTP_RESPONSE_CODE rcResponse;

        error = Command(lpSessionInfo,
                        FALSE,
                        FTP_TRANSFER_TYPE_UNKNOWN,
                        &rcResponse,
                        "PWD"
                        );
        if ((error == ERROR_SUCCESS)
        && (rcResponse.Major != FTP_RESPONSE_COMPLETE)) {
            error = ERROR_INTERNET_EXTENDED_ERROR;
        }
        if (error == ERROR_SUCCESS) {

            LPSTR pchResponse;

            //
            // parse the returned directory name out of the response text
            //

            pchResponse = InternetLockErrorText();
            if (pchResponse != NULL) {
                pchResponse = strstr(pchResponse, "257 ");
                if (pchResponse != NULL) {
                    pchResponse = strchr(pchResponse, '\"');
                    if (pchResponse != NULL) {

                        int idx;

                        ++pchResponse;
                        for (idx = 0, cchCopied = 0; pchResponse[idx] != '\0'; idx++) {
                            if (pchResponse[idx] == '\"') {
                                if (pchResponse[idx + 1] == '\"') {
                                    continue;
                                }
                                break;
                            }
                            if (cchCopied < cchCurrentDirectory) {
                                lpszCurrentDirectory[cchCopied] = pchResponse[idx];
                            }
                            cchCopied++;
                        }
                        if (cchCopied < cchCurrentDirectory) {
                            lpszCurrentDirectory[cchCopied] = '\0';
                            error = ERROR_SUCCESS;
                        } else {
                            error = ERROR_INSUFFICIENT_BUFFER;
                            ++cchCopied;
                        }
                    } else {
                        error = ERROR_INTERNET_EXTENDED_ERROR;
                    }
                }
                //InternetUnlockErrorText();
            }
        }
        DereferenceFtpSession(lpSessionInfo);
    } else {
        error = ERROR_INVALID_HANDLE;
    }

    if ((error == ERROR_SUCCESS) || (error == ERROR_INSUFFICIENT_BUFFER)) {
        *lpdwBytesReturned = cchCopied;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
wFtpCommand(
    IN HINTERNET hFtpSession,
    IN BOOL fExpectResponse,
    IN DWORD dwFlags,
    IN LPCSTR lpszCommand
    )

/*++

Routine Description:

    Runs arbitrary command at an FTP server. Direct connect over Internet

Arguments:

    hFtpSession     - identifies the FTP server

    fExpectResponse - TRUE if we expect a response from the server

    dwFlags         - type of response - ASCII text or BINARY data

    lpszCommand     - pointer to string describing command to run

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_FTP,
                Dword,
                "wFtpCommand",
                "%#x, %#x, %#x, %q",
                hFtpSession,
                fExpectResponse,
                dwFlags,
                lpszCommand
                ));

    LPFTP_SESSION_INFO lpSessionInfo;
    DWORD error;

    //
    // Look up the given handle.
    //

    if (FindFtpSession(hFtpSession, &lpSessionInfo)) {

        FTP_RESPONSE_CODE rcResponse;

        //
        // Issue the command.
        //

        error = Command(lpSessionInfo,
                        fExpectResponse,
                        dwFlags,
                        &rcResponse,
                        lpszCommand
                        );
        if (fExpectResponse && (error == ERROR_SUCCESS)) {
            
            INET_ASSERT(lpSessionInfo->socketData->IsValid());

            lpSessionInfo->dwTransferAccess |= (GENERIC_READ|GENERIC_WRITE);
            
        }
#if DBG
        else {

            INET_ASSERT(! lpSessionInfo->socketData->IsValid());

        }

        if (error == ERROR_SUCCESS) {

            INET_ASSERT(lpSessionInfo->socketControl->IsValid());

        }

#endif

        DereferenceFtpSession(lpSessionInfo);
    } else {
        error = ERROR_INVALID_HANDLE;
    }

    DEBUG_LEAVE(error);

    return error;
}


//
// Internet subordinate functions
//

DWORD
wFtpFindNextFile(
    IN HINTERNET hFtpSession,
    OUT LPWIN32_FIND_DATA lpFindFileData
    )

/*++

Routine Description:

    Returns the next file found from a call to FtpFindFirstFile().

Arguments:

    hFtpSession     - Handle to an FTP session, as returned from FtpConnect()

    lpFindFileData  - Pointer to a buffer that will contain WIN32_FIND_DATA
                      information when this call succeeds.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NO_MORE_FILES
                    The end of the file list has been reached.

                  ERROR_INVALID_HANDLE
                    Can't find session that knows about hFind
--*/

{
    DEBUG_ENTER((DBG_FTP,
                Dword,
                "wFtpFindNextFile",
                "%#x, %#x",
                hFtpSession,
                lpFindFileData
                ));

    LPFTP_SESSION_INFO lpSessionInfo;
    DWORD error;

    if (FindFtpSession(hFtpSession, &lpSessionInfo)) {

        //
        // ISSUE this code is cut & paste from find first - they should both call a
        // fn instead
        //

        if (!IsListEmpty(&lpSessionInfo->FindFileList)) {

            PLIST_ENTRY pEntry;

            //
            // Enumerate the first entry and advance pointers
            //

            pEntry = RemoveHeadList(&lpSessionInfo->FindFileList);

            INET_ASSERT(pEntry != NULL);

            CopyMemory(lpFindFileData,
                       (LPWIN32_FIND_DATA)(pEntry + 1),
                       sizeof(WIN32_FIND_DATA)
                       );
            FREE_MEMORY(pEntry);
            error = ERROR_SUCCESS;
        } else {
            error = ERROR_NO_MORE_FILES;
        }
        DereferenceFtpSession(lpSessionInfo);
    } else {
        error = ERROR_INVALID_HANDLE;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
wFtpFindClose(
    IN HINTERNET hFtpSession
    )

/*++

Routine Description:

    Frees the WIN32_FIND_DATA structures in the directory list for this session

Arguments:

    hFtpSession - handle of an FTP session, created by InternetConnect

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_HANDLE

--*/

{
    DEBUG_ENTER((DBG_FTP,
                Dword,
                "wFtpFindClose",
                "%#x",
                hFtpSession
                ));

    LPFTP_SESSION_INFO lpSessionInfo;
    DWORD error;

    if (FindFtpSession(hFtpSession, &lpSessionInfo)) {
        ClearFindList(&lpSessionInfo->FindFileList);

        //
        // this session no longer has an active directory listing
        //

        lpSessionInfo->Flags &= ~FFTP_FIND_ACTIVE;
        DereferenceFtpSession(lpSessionInfo);
        error = ERROR_SUCCESS;
    } else {
        error = ERROR_INVALID_HANDLE;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
wFtpConnect(
    IN LPCSTR lpszServerName,
    IN INTERNET_PORT nServerPort,
    IN LPCSTR lpszUsername,
    IN LPCSTR lpszPassword,
    IN DWORD dwService,
    IN DWORD dwFlags,
    OUT LPHINTERNET lphInternet
    )

/*++

Routine Description:

    Creates a new FTP session object

Arguments:

    lpszServerName  - pointer to string identifying FTP server

    nServerPort     - port number to connect to

    lpszUsername    - pointer to string identifying user name to log on as

    lpszPassword    - pointer to string identifying password to use with user name

    dwService       - service type parameter (unused)

    dwFlags         - session flags. Currently only INTERNET_FLAG_PASSIVE
                      is defined

    lphInternet     - returned handle of created FTP session

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY
                    Ran out of memory while creating the session object

                  ERROR_INTERNET_OUT_OF_HANDLES
                    Ran out of handles while creating the session object

                  ERROR_INTERNET_SHUTDOWN
                    The DLL is being unloaded

--*/

{
    INET_ASSERT(lpszUsername != NULL);
    INET_ASSERT(lpszPassword != NULL);

    DEBUG_ENTER((DBG_FTP,
                Dword,
                "wFtpConnect",
                "%q, %d, %q, %q, %d, %#x, %#x",
                lpszServerName,
                nServerPort,
                lpszUsername,
                lpszPassword,
                dwService,
                dwFlags,
                lphInternet
                ));

    DWORD error;
    LPFTP_SESSION_INFO sessionInfo;

    UNREFERENCED_PARAMETER(lpszUsername);
    UNREFERENCED_PARAMETER(lpszPassword);
    UNREFERENCED_PARAMETER(dwService);

    //
    // create a new FTP session object
    //

    error = CreateFtpSession((LPSTR)lpszServerName,
                             nServerPort,

                             //
                             // if INTERNET_FLAG_PASSIVE then create a passive
                             // session object
                             //

                             (dwFlags & INTERNET_FLAG_PASSIVE)
                                ? FFTP_PASSIVE_MODE
                                : 0,
                             &sessionInfo
                             );
    if (error == ERROR_SUCCESS) {

        //
        // return the FTP_SESSION_INFO handle
        //

        *lphInternet = sessionInfo->Handle;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
wFtpMakeConnection(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszUsername,
    IN LPCSTR lpszPassword
    )

/*++

Routine Description:

    Connect with and log into an FTP server.

    This function is cancellable

Arguments:

    hFtpSession - handle of an FTP session, created by InternetConnect

    pszUsername - pointer to string identifying user name to log on as

    pszPassword - pointer to string identifying password to use with user name

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INTERNET_INCORRECT_USER_NAME
                    The server didn't like the user name

                  ERROR_INTERNET_INCORRECT_PASSWORD
                    The server didn't like the password

                  ERROR_INTERNET_LOGIN_FAILURE
                    The server rejected the login request

                  ERROR_FTP_DROPPED
                    The connection has been closed

                  ERROR_FTP_TRANSFER_IN_PROGRESS
                    There is already a transfer in progress on this connection

                  ERROR_INTERNET_NAME_NOT_RESOLVED
                    Couldn't resolve the server name

                  WSA error
                    Couldn't connect to the server, or problems while
                    communicating with it

--*/

{
    DEBUG_ENTER((DBG_FTP,
                Dword,
                "wFtpMakeConnection",
                "%#x, %q, %q",
                hFtpSession,
                lpszUsername,
                lpszPassword
                ));

    LPFTP_SESSION_INFO sessionInfo;
    DWORD error;

    if (FindFtpSession(hFtpSession, &sessionInfo)) {

        //
        // resolve the FTP server's host name and connect to the server
        //

        error = FtpOpenServer(sessionInfo);
        if (error == ERROR_SUCCESS) {

            FTP_RESPONSE_CODE rcResponse;

            //
            // set send and receive timeouts on the control channel socket.
            // Ignore any errors
            //

            sessionInfo->socketControl->SetTimeout(
                        SEND_TIMEOUT,
                        GetTimeoutValue(INTERNET_OPTION_CONTROL_SEND_TIMEOUT)
                        );

            sessionInfo->socketControl->SetTimeout(
                        RECEIVE_TIMEOUT,
                        GetTimeoutValue(INTERNET_OPTION_CONTROL_RECEIVE_TIMEOUT)
                        );

            //
            // check greeting and store in per-thread response text buffer
            //

            error = GetReply(sessionInfo, &rcResponse);
            if (error == ERROR_SUCCESS) {

                //
                // check that the server sent us an affirmative response
                //

                if (rcResponse.Major == FTP_RESPONSE_COMPLETE) {

                    //
                    // send the user name
                    //

                    error = Command(sessionInfo,
                                    FALSE,
                                    FTP_TRANSFER_TYPE_UNKNOWN,
                                    &rcResponse,
                                    "USER %s",
                                    lpszUsername
                                    );

                    //
                    // BUGBUG - is it possible to get success from Command(),
                    //          but an error from the server - e.g. 332, need
                    //          account for login?
                    //

                    if (error == ERROR_SUCCESS) {

                        //
                        // send the password if required
                        //

                        if (rcResponse.Major == FTP_RESPONSE_CONTINUE) {
                            error = Command(sessionInfo,
                                            FALSE,
                                            FTP_TRANSFER_TYPE_UNKNOWN,
                                            &rcResponse,
                                            "PASS %s",
                                            lpszPassword
                                            );

                            //
                            // if we failed to send the password, or the password
                            // was rejected, or we are attempting to log on as
                            // "anonymous" and it turns out that the server does
                            // not allow anonymous logon, then return a password
                            // error. The caller can still check the response
                            // from the server
                            //

                            if (((error == ERROR_SUCCESS)
                                && (rcResponse.Major != FTP_RESPONSE_COMPLETE))
                            || (error == ERROR_INTERNET_EXTENDED_ERROR)) {
                                if (stricmp(lpszUsername, "anonymous") == 0) {
                                    error = ERROR_INTERNET_LOGIN_FAILURE;
                                } else {
                                    error = ERROR_INTERNET_INCORRECT_PASSWORD;
                                }
                            }
                        } else if (rcResponse.Major != FTP_RESPONSE_COMPLETE) {
                            error = ERROR_INTERNET_INCORRECT_USER_NAME;
                        }

                        //
                        // get the server type
                        //

                        //if (error == ERROR_SUCCESS) {
                        //    error = wFtpFindServerType(hFtpSession);
                        //}
                    }
                } else {
                    error = ERROR_INTERNET_LOGIN_FAILURE;
                }
            }
        }

        //
        // success or fail: unlock the session object
        //

        DereferenceFtpSession(sessionInfo);

        //
        // if we failed to login then let wFtpDisconnect() clean up - it will
        // also send a "QUIT" to the server (if we have a control connection)
        // which will ensure a clean exit
        //

        if (error != ERROR_SUCCESS) {

            //
            // if we experience an error during disconnect, we will just ignore
            // it and return the error generated during our failed login attempt
            //

            (void)wFtpDisconnect(hFtpSession, CF_EXPEDITED_CLOSE);
        }
    } else {
        error = ERROR_INVALID_HANDLE;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
wFtpDisconnect(
    IN HINTERNET hFtpSession,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Closes the connection, issues the quit command, etc.,

Arguments:

    hFtpSession - FTP session created by wFtpConnect

    dwFlags     - controlling operation. Can be:

                    CF_EXPEDITED_CLOSE  - Don't send QUIT to the server, just
                                          close the control connection

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_HANDLE

--*/

{
    DEBUG_ENTER((DBG_FTP,
                Dword,
                "wFtpDisconnect",
                "%#x, %#x",
                hFtpSession,
                dwFlags
                ));

    LPFTP_SESSION_INFO lpSessionInfo;
    DWORD error;
    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();

    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    if (FindFtpSession(hFtpSession, &lpSessionInfo)) {

        ICSocket * socketControl;
        ICSocket * socketData;

        socketControl = lpSessionInfo->socketControl;
        socketData = lpSessionInfo->socketData;

        //
        // kill any active data transfer
        //

        if (socketData->IsValid()) {

            //
            // set the non-blocking state depending on whether we are called in
            // an app thread context, or in the async scheduler thread context
            //

            //socketData->SetNonBlockingMode(lpThreadInfo->IsAsyncWorkerThread);

            if (dwFlags & CF_EXPEDITED_CLOSE) {
                error = socketData->Close();
            } else {
                error = wFtpCloseFile(hFtpSession);
                if (error != ERROR_SUCCESS) {

                    DEBUG_PRINT(WORKER,
                                ERROR,
                                ("wFtpCloseFile() returns %d\n",
                                error
                                ));

                }
            }
        }

        INET_ASSERT(!lpSessionInfo->socketData->IsValid());

        //
        // perform graceful close to the server if we have a control connection
        //

        if (socketControl->IsValid()) {

            //
            // set the non-blocking state depending on whether we are called in
            // an app thread context, or in the async scheduler thread context
            //

            //socketControl->SetNonBlockingMode(lpThreadInfo->IsAsyncWorkerThread);

            if (!(dwFlags & CF_EXPEDITED_CLOSE)) {

                FTP_RESPONSE_CODE rcResponse;

                Command(lpSessionInfo,
                        FALSE,
                        FTP_TRANSFER_TYPE_UNKNOWN,
                        &rcResponse,
                        "QUIT"
                        );
            }
            lpSessionInfo->socketControl->Disconnect(SF_INDICATE);
        }

        //
        // finally kill the FTP_SESSION_INFO structure
        //

        TerminateFtpSession(lpSessionInfo);
        error = ERROR_SUCCESS;
    } else {
        error = ERROR_INVALID_HANDLE;
    }

quit:

    DEBUG_LEAVE(error);

    return error;
}


DWORD
wFtpReadFile(
    IN HINTERNET hFtpSession,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead
    )

/*++

Routine Description:

    Reads data from the FTP server. We use the data channel

Arguments:

    hFtpSession             - handle identifying FTP session

    lpBuffer                - pointer to buffer for received data

    dwNumberOfBytesToRead   - size of lpBuffer in bytes

    lpdwNumberOfBytesRead   - returned number of bytes received

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_HANDLE
                    Couldn't find hFtpSession

                  ERROR_ACCESS_DENIED
                    This session doesn't have read access (?)

                  ERROR_FTP_DROPPED
                    The data channel has been closed

--*/

{
    DEBUG_ENTER((DBG_FTP,
                Dword,
                "wFtpReadFile",
                "%#x, %#x, %d, %#x",
                hFtpSession,
                lpBuffer,
                dwNumberOfBytesToRead,
                lpdwNumberOfBytesRead
                ));

    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();
    DWORD error;

    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    LPFTP_SESSION_INFO lpSessionInfo;
    ICSocket * socketData;
    BOOL eof;
    DWORD bytesReceived;

    //
    // initialize variables in case we quit early (i.e. via goto)
    //

    bytesReceived = 0;

    //
    // find the FTP_SESSION_INFO and ensure it is set up to receive data
    //

    if (!FindFtpSession(hFtpSession, &lpSessionInfo)) {
        error = ERROR_INVALID_HANDLE;
        goto quit;
    }

    //
    // if FFTP_EOF is set then we already reached the end the file
    //

    if (lpSessionInfo->Flags & FFTP_EOF) {
        error = ERROR_SUCCESS;
        goto unlock_and_quit;
    }

    //
    // get the data socket. If it has become INVALID_SOCKET then the server
    // closed the connection
    //

    socketData = lpSessionInfo->socketData;
    if (!socketData->IsValid()) {
        error = ERROR_FTP_DROPPED;
        goto unlock_and_quit;
    }

    if (!(lpSessionInfo->dwTransferAccess & GENERIC_READ)) {
        error = ERROR_ACCESS_DENIED;
        goto unlock_and_quit;
    }

    //
    // read until we fill the users buffer, get an error, or get to EOF
    //

    DWORD bufferRemaining;

    bufferRemaining = dwNumberOfBytesToRead;
    error = socketData->Receive(
                          &lpBuffer,
                          &dwNumberOfBytesToRead,   // lpdwBufferLength
                          &bufferRemaining,         // lpdwBufferRemaining
                          &bytesReceived,           // lpdwBytesReceived
                          0,                        // dwExtraSpace
                          SF_RECEIVE_ALL
                          | SF_INDICATE,
                          &eof
                          );
    if (error == ERROR_SUCCESS) {

        //
        // if we got to EOF then the server will have closed the data
        // connection. We need to close the socket at our end. If this is
        // a passive connection then we initiate session termination
        //

        if (eof) {
            (void)socketData->Close();

            INET_ASSERT(lpSessionInfo->socketData == socketData);

            //
            // reset the abort flag - we no longer have to send and ABOR command
            // when we close the handle
            //

            lpSessionInfo->Flags &= ~FFTP_ABORT_TRANSFER;

            //
            // set EOF in the FTP_SESSION_INFO flags so we know next time
            // we call this function that the session is not dropped, but
            // that we already reached the end of the data
            //

            lpSessionInfo->Flags |= FFTP_EOF;
        }
    }

    //
    // BUGBUG - in error case we should probably close the socket, set
    //          INVALID_SOCKET in the FTP_SESSION_INFO, etc.
    //

unlock_and_quit:

    //
    // update the output parameters if we succeeded
    //

    if (error == ERROR_SUCCESS) {
        *lpdwNumberOfBytesRead = bytesReceived;
    }

    DereferenceFtpSession(lpSessionInfo);

quit:

    DEBUG_LEAVE(error);

    return error;
}


DWORD
wFtpWriteFile(
    IN HINTERNET hFtpSession,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToWrite,
    OUT LPDWORD lpdwNumberOfBytesWritten
    )

/*++

Routine Description:

    Writes data to the FTP server. We use the data channel

Arguments:

    hFtpSession                 - handle identifying FTP session

    lpBuffer                    - pointer to buffer containing data to write

    dwNumberOfBytesToWrite      - size of lpBuffer in bytes

    lpdwNumberOfBytesWritten    - returned number of bytes sent

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_HANDLE
                    Couldn't find hFtpSession

                  ERROR_ACCESS_DENIED
                    This session doesn't have write access (?)

                  ERROR_FTP_DROPPED
                    The data channel has been closed

--*/

{
    DEBUG_ENTER((DBG_FTP,
                Dword,
                "wFtpWriteFile",
                "%#x, %#x, %d, %#x",
                hFtpSession,
                lpBuffer,
                dwNumberOfBytesToWrite,
                lpdwNumberOfBytesWritten
                ));

    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();
    DWORD error;

    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    LPFTP_SESSION_INFO lpSessionInfo;
    ICSocket * socketData;
    int nSent;

    //
    // find the FTP_SESSION_INFO and ensure it is set up to send data
    //

    if (!FindFtpSession(hFtpSession, &lpSessionInfo)) {
        error = ERROR_INVALID_HANDLE;
        goto quit;
    }

    socketData = lpSessionInfo->socketData;
    if (! socketData->IsValid()) {
        error = ERROR_FTP_DROPPED;
        goto unlock_and_quit;
    }

    if (!(lpSessionInfo->dwTransferAccess & GENERIC_WRITE)) {
        error = ERROR_ACCESS_DENIED;
        goto unlock_and_quit;
    }

    error = socketData->Send(lpBuffer, dwNumberOfBytesToWrite, SF_INDICATE);
    if (error == ERROR_SUCCESS) {
        *lpdwNumberOfBytesWritten = dwNumberOfBytesToWrite;
    } else {

        //
        // we had a failure. We should check the control socket for any error
        // info from the server
        //

        //FTP_RESPONSE_CODE response;
        //
        //(void)GetReply(lpSessionInfo, &response);
    }

unlock_and_quit:

    DereferenceFtpSession(lpSessionInfo);

quit:

    DEBUG_LEAVE(error);

    return error;
}


DWORD
wFtpQueryDataAvailable(
    IN HINTERNET hFtpSession,
    OUT LPDWORD lpdwNumberOfBytesAvailable
    )

/*++

Routine Description:

    Determines amount of data available to be received on a data (file) socket

Arguments:

    hFtpSession                 - identifies FTP session

    lpdwNumberOfBytesAvailable  - returned number of bytes available

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_HANDLE

--*/

{
    DEBUG_ENTER((DBG_FTP,
                Dword,
                "wFtpQueryDataAvailable",
                "%#x, %#x",
                hFtpSession,
                lpdwNumberOfBytesAvailable
                ));

    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();
    DWORD error;

    HINTERNET_HANDLE_TYPE handleType;

    error = RGetHandleType(lpThreadInfo->hObjectMapped, &handleType);

    if (error != ERROR_SUCCESS) {
        return (error);
    }

    *lpdwNumberOfBytesAvailable = 0;

    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    LPFTP_SESSION_INFO lpSessionInfo;

    if (FindFtpSession(hFtpSession, &lpSessionInfo)) {

        //
        // if we are currently performing a directory list then return the size
        // of a dir list entry
        //

        if (lpSessionInfo->Flags & FFTP_FIND_ACTIVE) {
            *lpdwNumberOfBytesAvailable = !IsListEmpty(&lpSessionInfo->FindFileList)
                                        ? sizeof(WIN32_FIND_DATA) : 0;
        } else {

            //
            // otherwise, if we are receiving data, find out how much
            //

            ICSocket * socketData;

            socketData = lpSessionInfo->socketData;
            if (socketData->IsValid()) {
                error = socketData->DataAvailable(lpdwNumberOfBytesAvailable);
            } else {

                //
                // there is no data connection
                //

                *lpdwNumberOfBytesAvailable = 0;
                error = ERROR_SUCCESS;
            }
        }
        DereferenceFtpSession(lpSessionInfo);
    } else {
        error = ERROR_INVALID_HANDLE;
    }

quit:

    if ((error == ERROR_SUCCESS) && (*lpdwNumberOfBytesAvailable == 0)) {

        InbLocalEndCacheWrite(lpThreadInfo->hObjectMapped,
                                            ((handleType==TypeFtpFindHandleHtml)
                                            ?"htm":NULL),
                                            TRUE);
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
wFtpCloseFile(
    IN HINTERNET hFtpSession
    )

/*++

Routine Description:

    Terminates the connection used for file transfer. The connection may already
    be closed (by the server during a READ, or by the client during a WRITE) in
    which case we just need to receive the confirmation (226) on the control
    socket. If the connection is still open, or the abort flag is set for this
    connection, then this is an abnormal termination, and we need to send an
    ABORt command

Arguments:

    hFtpSession - Identifies the session on which to terminate file transfer

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_HANDLE
                    Couldn't find the FTP_SESSION_INFO corresponding to
                    hFtpSession

--*/

{
    DEBUG_ENTER((DBG_FTP,
                Dword,
                "wFtpCloseFile",
                "%#x",
                hFtpSession
                ));

    LPFTP_SESSION_INFO lpSessionInfo;
    DWORD error;

    if (FindFtpSession(hFtpSession, &lpSessionInfo)) {

        BOOL getResponse;
        ICSocket * socketData;
        FTP_RESPONSE_CODE rcResponse;

        socketData = lpSessionInfo->socketData;
        if (socketData->IsValid()) {

            //
            // if we are performing a read/write operation and the transfer
            // isn't complete then abort the connection
            //

            if (lpSessionInfo->Flags & FFTP_ABORT_TRANSFER) {
                AbortTransfer(lpSessionInfo);
                ResetSocket(lpSessionInfo->socketData);
            } else {

                //
                // in all other cases - completed READ, complete or incomplete
                // WRITE - just close the socket
                //

                lpSessionInfo->socketData->Close();
            }
        } else if (lpSessionInfo->Flags & FFTP_ABORT_TRANSFER) {

            //
            // we have no data socket, but the abort transfer flag is set. We
            // are probably closing a file we opened for read without having
            // read any data. In this case we send an abort anyway
            //

            AbortTransfer(lpSessionInfo);
        }

        //
        // get the server response - we expect either 226 to a good transfer,
        // or 426 for an aborted transfer...
        //

        GetSessionLastResponseCode(lpSessionInfo, &rcResponse);
        if (rcResponse.Major == FTP_RESPONSE_PRELIMINARY) {
            error = GetReply(lpSessionInfo, &rcResponse);
            if ((error == ERROR_SUCCESS)
            && (rcResponse.Major != FTP_RESPONSE_COMPLETE)) {
                error = ERROR_INTERNET_EXTENDED_ERROR;
            }
        } else {
            error = ERROR_SUCCESS;
        }

        //
        // reset the ABORT, FILE_ACTIVE and EOF flags
        //

        lpSessionInfo->Flags &= ~(FFTP_ABORT_TRANSFER
                                  | FFTP_EOF
                                  | FFTP_FILE_ACTIVE
                                  );
        DereferenceFtpSession(lpSessionInfo);
    } else {
        error = ERROR_INVALID_HANDLE;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
wFtpFindServerType(
    IN HINTERNET hFtpSession
    )

/*++

Routine Description:

    Determines the type of server we are talking to (NT or Unix)

Arguments:

    hFtpSession - identifies FTP_SESSION_INFO. The structure ServerType field
                  will be updated with the discovered info

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_HANDLE
                    Couldn't find the FTP_SESSION_INFO corresponding to
                    hFtpSession

--*/

{
    DEBUG_ENTER((DBG_FTP,
                Dword,
                "wFtpFindServerType",
                "%#x",
                hFtpSession
                ));

    LPFTP_SESSION_INFO lpSessionInfo;
    DWORD error;

    if (FindFtpSession(hFtpSession, &lpSessionInfo)) {

        FTP_RESPONSE_CODE rcResponse;

        error = Command(lpSessionInfo,
                        FALSE,
                        FTP_TRANSFER_TYPE_UNKNOWN,
                        &rcResponse,
                        "SYST"
                        );
        if (error == ERROR_SUCCESS) {

            LPSTR lpszResponse = InternetLockErrorText();

            if (lpszResponse != NULL) {

                FTP_SERVER_TYPE serverType = FTP_SERVER_TYPE_UNKNOWN;

                //
                // "215 " must be first token in response text
                //

                lpszResponse = strstr(lpszResponse, "215 ");
                if (lpszResponse != NULL) {

                    //
                    // check for existence of "Windows_NT" or "Unix" (case
                    // insensitive comparison)
                    //

                    //
                    // BUGBUG - find out from MuraliK/TerryK the values these
                    //          ids can have
                    //

                    static struct {
                        LPCSTR lpszSystemName;
                        FTP_SERVER_TYPE ServerType;
                    } FtpServerTypes[] = {
                        "Windows_NT",   FTP_SERVER_TYPE_NT,
                        "Unix",         FTP_SERVER_TYPE_UNIX
                    };

                    DWORD textLength = strlen(lpszResponse);

                    for (int i = 0; i < ARRAY_ELEMENTS(FtpServerTypes); ++i) {
                        if (strnistr(lpszResponse,
                                     (LPSTR)FtpServerTypes[i].lpszSystemName,
                                     textLength
                                     ) != NULL) {

                            serverType = FtpServerTypes[i].ServerType;

                            DEBUG_PRINT(FTP,
                                        INFO,
                                        ("serverType = %s (%d)\n",
                                        InternetMapFtpServerType(serverType),
                                        serverType
                                        ));

                            break;
                        }
                    }
                }
                lpSessionInfo->ServerType = serverType;
                //InternetUnlockErrorText();
            }
        }
        DereferenceFtpSession(lpSessionInfo);
    } else {
        error = ERROR_INVALID_HANDLE;
    }

    DEBUG_LEAVE(error);

    return error;
}



#if 0

//
// We don't use this today, because FtpGetFileSize does not support
//   issuing backround commands through the FTP Control socket while
//   the user is doing an FTP download (with FtpOpenFile)
//


DWORD
wFtpGetFileSize(
    IN  HINTERNET hMappedFtpSession,
    IN  LPFTP_SESSION_INFO lpSessionInfo,
    OUT LPDWORD lpdwFileSizeLow,
    OUT LPDWORD lpdwFileSizeHigh
    )

/*++

Routine Description:

    Finds size of a file at server

Arguments:

    hFtpSession         - identifies mapped FTP handle obj

    lpSessionInfo       - LPFTP_SESSION_INFO structure ptr.

    lpdwFileSizeLow     - pointer to low dword of file size

    lpdwFileSizeHigh    - optional output pointer to high dword of file size

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_HANDLE
                    Couldn't find the FTP_SESSION_INFO corresponding to
                    hFtpSession

--*/

{
    DEBUG_ENTER((DBG_FTP,
                Dword,
                "wFtpGetFileSize",
                "%#x, %#x, %#x, %#x",
                hMappedFtpSession,
                lpSessionInfo,
                lpdwFileSizeLow,
                lpdwFileSizeHigh
                ));


    DWORD error = ERROR_INTERNET_INTERNAL_ERROR;
    FTP_FILE_HANDLE_OBJECT * pFileMapped = (FTP_FILE_HANDLE_OBJECT *) hMappedFtpSession;

    *lpdwFileSizeLow  = 0;
    *lpdwFileSizeHigh = 0;
                       
    if (lpSessionInfo) {

        FTP_RESPONSE_CODE rcResponse;

        error = Command(lpSessionInfo,
                        FALSE,
                        FTP_TRANSFER_TYPE_UNKNOWN,
                        &rcResponse,
                        "SIZE %s",
                        pFileMapped->GetFileName()
                        );
        if (error == ERROR_SUCCESS) {

            LPSTR lpszResponse = InternetLockErrorText();

            if (lpszResponse != NULL) {

                FTP_SERVER_TYPE serverType = FTP_SERVER_TYPE_UNKNOWN;

                //
                // "213 " must be first token in response text of file size
                //

                lpszResponse = strstr(lpszResponse, "213 ");
                if (lpszResponse != NULL) {
                    *lpdwFileSizeLow = atoi(lpszResponse);                    
                    error = ERROR_SUCCESS;
                }
            }
        }        
    } else {
        error = ERROR_INVALID_HANDLE;
    }

    DEBUG_LEAVE(error);

    return error;
}
#endif

//
// private debug functions
//

#if INET_DEBUG

PRIVATE
DEBUG_FUNCTION
LPSTR
InternetMapFtpServerType(
    IN FTP_SERVER_TYPE ServerType
    )
{
    switch (ServerType) {
    CASE_OF(FTP_SERVER_TYPE_UNKNOWN);
    CASE_OF(FTP_SERVER_TYPE_NT);
    CASE_OF(FTP_SERVER_TYPE_UNIX);
    }
    return "?";
}

#endif // INET_DEBUG
