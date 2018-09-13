/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    gfrapir.cxx

Abstract:

    Contains the remote-side gopher API worker functions. In each case, the API
    proper validates the arguments. The worker functions contained herein just
    perform the requested operation with the supplied arguments.

    These functions are the remote side of the RPC interface. If the DLL is
    the abstract0 version (no RPC) then the A forms of the functions simply
    call the w functions

    Contents:
        wGopherFindFirst
        wGopherFindNext
        wGopherFindClose
        wGopherOpenFile
        wGopherReadFile
        wGopherQueryDataAvailable
        wGopherCloseHandle
        wGopherGetAttribute
        wGopherConnect
        wGopherDisconnect
        (GetView)

Author:

    Richard L Firth (rfirth) 14-Oct-1994

Environment:

    Win32 DLL

Revision History:

    14-Oct-1994 rfirth
        Created

--*/

#include <wininetp.h>
#include "gfrapih.h"

//
// manifests
//

#define DEFAULT_REQUEST_BUFFER_LENGTH   (MAX_GOPHER_SELECTOR_TEXT + GOPHER_REQUEST_TERMINATOR_LENGTH + 1)

//
// private data
//

char szQuery[] = "query "; // prepend to CSO searches

//
// private prototypes
//

PRIVATE
DWORD
GetView(
    IN LPSESSION_INFO SessionInfo,
    IN VIEW_TYPE ViewType,
    IN LPSTR Request,
    IN BOOL RequestIsGopherPlus,
    IN DWORD ResponseFlags,
    OUT LPVIEW_INFO* pViewInfo
    );

extern
DWORD
InbGopherLocalEndCacheWrite(
    IN HINTERNET hGopherFile,
    IN LPSTR    lpszFileExtension,
    IN BOOL fNormal
    );

//
// functions
//


DWORD
wGopherFindFirst(
    IN LPCSTR lpszLocator,
    IN LPCSTR lpszSearchString OPTIONAL,
    OUT LPGOPHER_FIND_DATA lpBuffer OPTIONAL,
    OUT LPHINTERNET lpHandle
    )

/*++

Routine Description:

    Connects to the gopher server, sends a request to get directory information,
    gets the response and converts the gopher descriptor strings to
    GOPHER_FIND_DATA structures

Arguments:

    lpszLocator         - pointer to descriptor of information to get

    lpszSearchString    - pointer to strings to search for if Locator is search
                          server. This argument MUST be present if Locator is
                          an search server

    lpBuffer            - pointer to user-allocated buffer in which to return
                          info

    lpHandle            - pointer to returned handle if ERROR_SUCCESS returned

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY
                  WSA error

--*/

{
    DEBUG_ENTER((DBG_GOPHER,
                Dword,
                "wGopherFindFirst",
                "%q, %q, %#x, %#x",
                lpszLocator,
                lpszSearchString,
                lpBuffer,
                lpHandle
                ));

    DWORD gopherType;
    LPSTR requestPtr;
    DWORD requestLen;
    char hostName[MAX_GOPHER_HOST_NAME + 1];
    DWORD hostNameLen;
    DWORD port;
    LPSTR gopherPlus;
    LPSESSION_INFO sessionInfo;
    DWORD error;
    HINTERNET findHandle;
    LPVIEW_INFO viewInfo;
    DWORD newRequestLength;
    DWORD searchStringsLength;

    //
    // initialise variables in case of early exit (via goto)
    //

    sessionInfo = NULL;

    //
    // grab a buffer for the request string
    //

    requestLen = DEFAULT_REQUEST_BUFFER_LENGTH;
    requestPtr = (LPSTR)ResizeBuffer(NULL, requestLen, FALSE);
    if (requestPtr == NULL) {

        DEBUG_LEAVE(ERROR_NOT_ENOUGH_MEMORY);

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // pull the individual fields out of the locator. Not interested in display
    // string
    //

    hostNameLen = sizeof(hostName);
    if (!CrackLocator(lpszLocator,
                      &gopherType,
                      NULL,         // DisplayString
                      NULL,         // DisplayStringLength
                      requestPtr,   // SelectorString
                      &requestLen,  // SelectorStringLength
                      hostName,
                      &hostNameLen,
                      &port,
                      &gopherPlus
                      )) {
        error = ERROR_GOPHER_INVALID_LOCATOR;
        goto quit;
    }

    //
    // find the session 'object'. If we don't have one describing the requested
    // gopher server then create one
    //

    sessionInfo = FindOrCreateSession(hostName, port, &error);
    if (sessionInfo == NULL) {
        goto quit;
    }

    //
    // if the request is gopher+ or plain gopher but we know the server is
    // gopher+ then we automatically promote the request to be gopher+. It
    // potentially makes life easier for this DLL (the server could tell us
    // the exact length of the response or be more discerning about errors)
    // and potentially gives more information to the app. Either way, the app
    // doesn't lose by this
    //

    if (gopherPlus || IsGopherPlusSession(sessionInfo)) {
        gopherType |= GOPHER_TYPE_GOPHER_PLUS;
    }

    //
    // calculate the length of the extra strings we have to add to the selector
    //

    newRequestLength = requestLen;

    if (  IS_GOPHER_SEARCH_SERVER(gopherType)) {

        INET_ASSERT(lpszSearchString != NULL);
        INET_ASSERT(*lpszSearchString != '\0');

        //
        // add search strings length
        //

        searchStringsLength = strlen(lpszSearchString);
        newRequestLength += searchStringsLength;

        if (IS_GOPHER_INDEX_SERVER(gopherType)) {
            newRequestLength++; // for tab
        } else {
            newRequestLength += sizeof(szQuery) - 1;
        }

    } else {
        searchStringsLength = 0;
    }

    //
    // gopher+ requests have "\t+" or "\t$" at the end of the request
    //

    if (IS_GOPHER_PLUS(gopherType)) {
        newRequestLength += 2;
    }

    //
    // all requests terminated by "\r\n". Add 1 for string terminator
    //

    newRequestLength += sizeof(GOPHER_REQUEST_TERMINATOR);

    //
    // grow the buffer if necessary
    //

    if (newRequestLength > DEFAULT_REQUEST_BUFFER_LENGTH) {
        requestPtr = (LPSTR)ResizeBuffer((HLOCAL)requestPtr, newRequestLength, FALSE);
        if (requestPtr == NULL) {
            error = ERROR_NOT_ENOUGH_MEMORY;
            goto quit;
        }
    }

    //
    // add the additional strings
    //

    if (searchStringsLength != 0) {

        if (IS_GOPHER_INDEX_SERVER(gopherType)) {
            requestPtr[requestLen++] = GOPHER_FIELD_SEPARATOR;
        } else {
            memcpy (requestPtr + requestLen, szQuery, sizeof(szQuery) - 1);
            requestLen += sizeof(szQuery) - 1;
        }
        memcpy(&requestPtr[requestLen], lpszSearchString, searchStringsLength);
        requestLen += searchStringsLength;
    }
    if (IS_GOPHER_PLUS(gopherType)) {
        requestPtr[requestLen++] = GOPHER_FIELD_SEPARATOR;
        requestPtr[requestLen++] =
            IS_GOPHER_SEARCH_SERVER(gopherType)? '+' : '$';
    }
    memcpy(&requestPtr[requestLen],
           GOPHER_REQUEST_TERMINATOR,
           sizeof(GOPHER_REQUEST_TERMINATOR)    // don't scrub the '\0' in this case
           );

    //
    // selector munged; get the directory listing
    //

    error = GetView(sessionInfo,
                    ViewTypeFind,
                    requestPtr,
                    IS_GOPHER_PLUS(gopherType) ? TRUE : FALSE,
                    BI_DOT_AT_END,
                    &viewInfo
                    );

    //
    // if no error was reported then we can return a directory entry
    //

    if (error == ERROR_SUCCESS) {

        //
        // if the caller supplied an output buffer then convert the first
        // directory entry to the API buffer format, else the caller wants
        // all gopher directory information returned by InternetFindNextFile()
        //

        if (ARGUMENT_PRESENT(lpBuffer)) {
            error = GetDirEntry(viewInfo, lpBuffer);
        }
        if (error == ERROR_SUCCESS) {
            findHandle = viewInfo->Handle;
        } else {
            DereferenceView(viewInfo);
        }
    }

quit:

    //
    // dereference the session. If we have an active VIEW_INFO then the
    // reference count will still be > 0
    //

    if (sessionInfo != NULL) {
        DereferenceSession(sessionInfo);
    }

    //
    // if we allocated a new request buffer for a large search request then
    // free it
    //

    if (requestPtr != NULL) {
        requestPtr = (LPSTR)ResizeBuffer((HLOCAL)requestPtr, 0, FALSE);

        INET_ASSERT(requestPtr == NULL);

    }

    //
    // set the handle value - ignored by caller if we don't return ERROR_SUCCESS
    //

    *lpHandle = findHandle;

    DEBUG_LEAVE(error);

    return error;
}


DWORD
wGopherFindNext(
    IN HINTERNET hFind,
    OUT LPGOPHER_FIND_DATA lpBuffer
    )

/*++

Routine Description:

    Remote side of GopherFindNext API. All parameters have been validated by the
    time this function is called, so we know that Buffer is large enough to
    hold all the returned data

Arguments:

    hFind       - handle of FIND_DATA, created by GopherFindFirstFile()

    lpBuffer    - pointer to user-allocated buffer for GOPHER_FIND_DATA

Return Value:

    DWORD
        Success - ERROR_SUCCESS
                    Buffer contains next GOPHER_FIND_DATA structure

        Failure - ERROR_INVALID_HANDLE
                    Can't find the VIEW_INFO corresponding to hFind

                  ERROR_NO_MORE_FILES
                    We have reached the end of the directory info

--*/

{
    DEBUG_ENTER((DBG_GOPHER,
                Dword,
                "wGopherFindNext",
                "%#x, %#x",
                hFind,
                lpBuffer
                ));

    LPVIEW_INFO viewInfo;
    DWORD error;

    //
    // locate the VIEW_INFO corresponding to hFind and the SESSION_INFO
    // that owns it
    //

    viewInfo = FindViewByHandle(hFind, ViewTypeFind);
    if (viewInfo != NULL) {

        //
        // just read out the next directory entry
        //

        error = GetDirEntry(viewInfo, lpBuffer);

        //
        // if the Find has been closed or this was the last entry, the following
        // dereference will cause the VIEW_INFO to be deleted, and if there are no
        // more requests outstanding on the SESSION_INFO then it too will be
        // deleted (via a dereference)
        //

        DereferenceView(viewInfo);
    } else {
        error = ERROR_INVALID_HANDLE;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
wGopherFindClose(
    IN HINTERNET hFind
    )

/*++

Routine Description:

    Causes the VIEW_INFO described by hFind to be removed from the SESSION_INFO
    and freed. If there are no other links to the data buffer then it too is
    deallocated

Arguments:

    hFind   - handle describing the VIEW_INFO to terminate

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_HANDLE

--*/

{
    DEBUG_ENTER((DBG_GOPHER,
                Dword,
                "wGopherFindClose",
                "%#x",
                hFind
                ));

    DWORD error;

    //
    // atomically find and dereference the VIEW_INFO given the handle
    //

    error = DereferenceViewByHandle(hFind, ViewTypeFind);

    DEBUG_LEAVE(error);

    return error;
}


DWORD
wGopherOpenFile(
    IN LPCSTR lpszLocator,
    IN LPCSTR lpszView OPTIONAL,
    OUT LPHINTERNET lpHandle
    )

/*++

Routine Description:

    'Opens' a gopher file by copying it locally and returning a handle to the
    buffer

Arguments:

    lpszLocator - pointer to locator describing file to open

    lpszView    - pointer to view name - identifies type of file to retrieve

    lpHandle    - pointer to returned handle if ERROR_SUCCESS returned

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY
                  WSA error

--*/

{
    DEBUG_ENTER((DBG_GOPHER,
                Dword,
                "wGopherOpenFile",
                "%q, %q, %#x",
                lpszLocator,
                lpszView,
                lpHandle
                ));

    DWORD error;
    DWORD gopherType;
    LPSTR requestPtr;
    DWORD requestLen;
    char hostName[MAX_GOPHER_HOST_NAME + 1];
    DWORD hostNameLen;
    DWORD port;
    LPSTR gopherPlus;
    LPSESSION_INFO sessionInfo;
    LPVIEW_INFO viewInfo;
    HINTERNET fileHandle;
    DWORD viewLen;

    //
    // initialise variables in case of early exit (via goto)
    //

    sessionInfo = NULL;
    fileHandle = NULL;

    //
    // grab a buffer for the request string
    //

    requestLen = MAX_GOPHER_SELECTOR_TEXT + 1;
    requestPtr = (LPSTR)ResizeBuffer((HLOCAL)NULL, requestLen, FALSE);
    if (requestPtr == NULL) {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    //
    // pull the individual fields out of the locator. Not interested in display
    // string
    //

    hostNameLen = sizeof(hostName);
    if (!CrackLocator(lpszLocator,
                      &gopherType,
                      NULL,         // DisplayString
                      NULL,         // DisplayStringLength
                      requestPtr,   // SelectorString
                      &requestLen,  // SelectorStringLength
                      hostName,
                      &hostNameLen,
                      &port,
                      &gopherPlus
                      )) {
        error = ERROR_GOPHER_INVALID_LOCATOR;
        goto quit;
    }

    //
    // find the session 'object'. If we don't have one describing the requested
    // gopher server then create one
    //

    sessionInfo = FindOrCreateSession(hostName, port, &error);
    if (sessionInfo == NULL) {
        goto quit;
    }

    //
    // (see GopherFindFirstFile()). If gopher+ is requested or available then
    // make this a gopher+ request
    //

    if (gopherPlus || IsGopherPlusSession(sessionInfo)) {
        gopherType |= GOPHER_TYPE_GOPHER_PLUS;

        //
        // we at least need some space for "\t+"
        //

        viewLen = sizeof(GOPHER_PLUS_INDICATOR) - 1;

        //
        // get the amount of space required for the alternate view, if supplied
        //

        if (ARGUMENT_PRESENT(lpszView)) {

            //
            // the extra +1 here is for the '+' between the selector and the
            // alternate view string: the '\0' is handled by
            // sizeof(GOPHER_REQUEST_TERMINATOR)
            //

            viewLen += strlen(lpszView);
        }
    } else {

        //
        // the caller may have supplied an alternate view for a gopher0 even
        // though it is meaningless. Ensure that it is not used
        //

        lpszView = NULL;
        viewLen = 0;
    }

    //
    // grow the buffer if it is not large enough to hold the view etc. Note, if
    // this is true, then we have bust one of our internal limits, which is
    // unexpected to say the least. But this way, we can allow apps to present
    // completely bogus (so we think) parameters, and at least give them a try
    //

    if ((requestLen + viewLen + sizeof(GOPHER_REQUEST_TERMINATOR))
    > (MAX_GOPHER_SELECTOR_TEXT + 1)) {
        requestPtr = (LPSTR)ResizeBuffer((HLOCAL)requestPtr,
                                         requestLen
                                         + viewLen
                                         + sizeof(GOPHER_REQUEST_TERMINATOR),
                                         FALSE
                                         );
        if (requestPtr == NULL) {
            error = ERROR_NOT_ENOUGH_MEMORY;
            goto quit;
        }
    }

    //
    // if this is a gopher plus request, then add the "\t+". If there is an
    // alternate view then it will be appended to the plus, else we just add
    // the line terminator
    //

    if (IS_GOPHER_PLUS(gopherType)) {
        memcpy(&requestPtr[requestLen],
               GOPHER_PLUS_INDICATOR,
               sizeof(GOPHER_PLUS_INDICATOR) - 1
               );
        requestLen += sizeof(GOPHER_PLUS_INDICATOR) - 1;
    }

    //
    // add the alternate view information, if any was supplied
    //

    if (ARGUMENT_PRESENT(lpszView)) {
        memcpy(&requestPtr[requestLen], lpszView, viewLen);
        requestLen += viewLen;
    }

    //
    // in gopher0 and gopher+ cases we must terminate the selector by CR-LF
    //

    memcpy(&requestPtr[requestLen],
           GOPHER_REQUEST_TERMINATOR,
           sizeof(GOPHER_REQUEST_TERMINATOR)
           );

    //
    // selector munged; get the file
    //

    error = GetView(sessionInfo,
                    ViewTypeFile,
                    requestPtr,
                    (gopherPlus != NULL)
                        ? TRUE
                        : FALSE,
                    IS_DOT_TERMINATED_REQUEST(gopherType)
                        ? BI_DOT_AT_END
                        : 0,
                    &viewInfo
                    );
    if (error == ERROR_SUCCESS) {
        fileHandle = viewInfo->Handle;
    }

quit:

    //
    // free the request buffer
    //

    if (requestPtr != NULL) {
        requestPtr = (LPSTR)ResizeBuffer((HLOCAL)requestPtr, 0, FALSE);

        INET_ASSERT(requestPtr == NULL);

    }

    //
    // dereference the session - this may cause it to be deleted
    //

    if (sessionInfo != NULL) {
        DereferenceSession(sessionInfo);
    }

    //
    // set the handle value - ignored by caller if we don't return ERROR_SUCCESS
    //

    *lpHandle = fileHandle;

    DEBUG_LEAVE(error);

    return error;
}


DWORD
wGopherReadFile(
    IN HINTERNET hFile,
    OUT LPBYTE lpBuffer,
    IN DWORD dwBufferLength,
    OUT LPDWORD lpdwBytesReturned
    )

/*++

Routine Description:

    Reads the next dwBufferLength bytes (or as much as is remaining) from the
    file identified by hFile and writes to lpBuffer

Arguments:

    hFile               - identifies file

    lpBuffer            - place to return file data

    dwBufferLength      - length of Buffer

    lpdwBytesReturned   - amount of data written to Buffer

Return Value:

    DWORD
        Success - ERROR_SUCCESS
                    *lpdwBytesReturned written to lpBuffer

        Failure - ERROR_INVALID_HANDLE
                    Couldn't find the VIEW_INFO corresponding to hFile

--*/

{
    DEBUG_ENTER((DBG_GOPHER,
                Dword,
                "wGopherReadFile",
                "%#x, %#x, %d, %#x",
                hFile,
                lpBuffer,
                dwBufferLength,
                lpdwBytesReturned
                ));

    LPVIEW_INFO viewInfo;
    DWORD error;

    viewInfo = FindViewByHandle(hFile, ViewTypeFile);
    if (viewInfo != NULL) {

        LPBUFFER_INFO bufferInfo;

        INET_ASSERT(viewInfo->BufferInfo != NULL);

        bufferInfo = viewInfo->BufferInfo;

        bufferInfo->Buffer = lpBuffer;
        bufferInfo->BufferLength = dwBufferLength;
        error = ReadData(viewInfo, lpdwBytesReturned);
        DereferenceView(viewInfo);
    } else {
        error = ERROR_INVALID_HANDLE;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
wGopherQueryDataAvailable(
    IN HINTERNET hFile,
    OUT LPDWORD lpdwNumberOfBytesAvailable
    )

/*++

Routine Description:

    Determines the amount of data available to be read on the socket

Arguments:

    hFile                       - identifies gopher file

    lpdwNumberOfBytesAvailable  - where number of available bytes returned

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_HANDLE

--*/

{
    DEBUG_ENTER((DBG_GOPHER,
                Dword,
                "wGopherQueryDataAvailable",
                "%#x, %#x",
                hFile,
                lpdwNumberOfBytesAvailable
                ));

    DWORD bytesAvailable = 0;
    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();
    DWORD error;

    HINTERNET_HANDLE_TYPE handleType;

    error = RGetHandleType(lpThreadInfo->hObjectMapped, &handleType);

    if (error != ERROR_SUCCESS) {
        return (error);
    }

    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    LPVIEW_INFO viewInfo;
    VIEW_TYPE viewType;

    //
    // assume this is a directory list first
    //

    if ((viewInfo = FindViewByHandle(hFile, ViewTypeFind)) != NULL) {
        viewType = ViewTypeFind;
        error = ERROR_SUCCESS;
        if (viewInfo->ViewOffset < viewInfo->BufferInfo->BufferLength) {
            bytesAvailable = sizeof(GOPHER_FIND_DATA);
        }
    } else if ((viewInfo = FindViewByHandle(hFile, ViewTypeFile)) != NULL) {
        viewType = ViewTypeFile;
        error = ERROR_SUCCESS;
    } else {
        error = ERROR_INVALID_HANDLE;
    }
    if ((error == ERROR_SUCCESS) && (bytesAvailable == 0)) {

        INET_ASSERT(viewInfo->BufferInfo != NULL);

        ICSocket *socket = viewInfo->BufferInfo->Socket;

        if (socket->IsValid()) {
            error = socket->DataAvailable(&bytesAvailable);
        }
        if ((error == ERROR_SUCCESS)
        && (viewType == ViewTypeFind)
        && (bytesAvailable != 0)) {
            bytesAvailable = sizeof(GOPHER_FIND_DATA);
        }
    } else {
        error = ERROR_SUCCESS;
    }

    if (viewInfo != NULL) {
        DereferenceView(viewInfo);
    }

    *lpdwNumberOfBytesAvailable = bytesAvailable;

quit:

    if ((error == ERROR_SUCCESS) && !bytesAvailable) {
        InbGopherLocalEndCacheWrite(lpThreadInfo->hObjectMapped,
                                    ((handleType==TypeFtpFindHandleHtml)
                                    ?"htm":NULL),
                                    TRUE);
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
wGopherCloseHandle(
    IN HINTERNET hFile
    )

/*++

Routine Description:

    Causes the VIEW_INFO described by hFile to be removed from the
    SESSION_INFO and freed. If there are no other links to the data buffer
    then it too is deallocated

Arguments:

    hFile   - handle describing the VIEW_INFO to terminate

Return Value:

    BOOL
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_HANDLE

--*/

{
    DEBUG_ENTER((DBG_GOPHER,
                Dword,
                "wGopherCloseHandle",
                "%#x",
                hFile
                ));

    DWORD error;

    //
    // atomically find and dereference the VIEW_INFO given the handle
    //

    error = DereferenceViewByHandle(hFile, ViewTypeFile);

    DEBUG_LEAVE(error);

    return error;
}

#if defined(GOPHER_ATTRIBUTE_SUPPORT)


DWORD
wGopherGetAttribute(
    IN LPCSTR lpszLocator,
    IN LPCSTR lpszAttribute,
    OUT LPBYTE lpBuffer,
    IN OUT LPDWORD lpdwBufferLength
    )

/*++

Routine Description:

    Retrieves the requested attribute

Arguments:

    lpszLocator         - descriptor of item for which attribute information will
                          be retrieved

    lpszAttribute       - the attribute name, e.g. +VIEWS

    lpBuffer            - to receive attributes

    lpdwBufferLength    - IN: length of lpBuffer
                          OUT: number of bytes returned in lpBuffer

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_GOPHER_ATTRIBUTE_NOT_FOUND
                  ERROR_GOPHER_NOT_GOPHER_PLUS
                  ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DEBUG_ENTER((DBG_GOPHER,
                Dword,
                "wGopherGetAttribute",
                "%q, %q, %#x, %#x [%d]",
                lpszLocator,
                lpszAttribute,
                lpBuffer,
                lpdwBufferLength
                ));

    DWORD error;

    //
    // the locator we are requested to get attributes for may have come from a
    // server other than that identified in the locator. We will search any
    // VIEW_INFO buffers we have looking for the locator. In the worst case
    // we won't find it and will have to send a request to the server
    //

    error = SearchSessionsForAttribute((LPSTR)lpszLocator,
                                       (LPSTR)lpszAttribute,
                                       lpBuffer,
                                       lpdwBufferLength
                                       );
    if (error == ERROR_GOPHER_ATTRIBUTE_NOT_FOUND) {

        char request[MAX_GOPHER_SELECTOR_TEXT + 1];
        DWORD requestLen;
        char hostName[MAX_GOPHER_HOST_NAME + 1];
        DWORD hostNameLen;
        DWORD port;
        LPSTR gopherPlus;
        LPSESSION_INFO sessionInfo;
        DWORD requestType;

        //
        // its the worst case - we don't (or no longer) have the requested
        // locator/attributes. We must request them again from the server
        //

        //
        // pull the individual fields out of the locator. Not interested in display
        // string
        //

        requestLen = sizeof(request);
        hostNameLen = sizeof(hostName);
        if (!CrackLocator(lpszLocator,
                          &requestType,
                          NULL,         // DisplayString
                          NULL,         // DisplayStringLength
                          request,      // SelectorString
                          &requestLen,  // SelectorStringLength
                          hostName,
                          &hostNameLen,
                          &port,
                          &gopherPlus
                          )) {
            error = ERROR_GOPHER_INVALID_LOCATOR;
            goto quit;
        }

        //
        // if we already have a session to the server identified in the locator
        // then we will check if we already have the information stored in a
        // VIEW_INFO. If not then we must send the request for the attribute info
        //

        sessionInfo = FindOrCreateSession(hostName, port, &error);
        if (sessionInfo != NULL) {

            //
            // BUGBUG - IsGopherPlusSession needs to perform discovery if
            //          unknown
            //

//            if (IsGopherPlusSession(sessionInfo)) {
            if (TRUE) {

                LPSTR attributeRequest;

                //
                // convert the request to a request for attributes that the
                // gopher server understands
                //

                attributeRequest = MakeAttributeRequest(request,
                                                        (LPSTR)lpszAttribute
                                                        );
                if (attributeRequest != NULL) {

                    LPVIEW_INFO viewInfo;

                    error = GetView(sessionInfo,
                                    ViewTypeFind,
                                    attributeRequest,
                                    TRUE,   // RequestIsGopherPlus
                                    0,
                                    &viewInfo
                                    );

                    //
                    // done with attribute request buffer (created by
                    // MakeAttributeRequest)
                    //

                    DEL(attributeRequest);

                    //
                    // copy everything that came back to the caller's buffer
                    // if there's enough space
                    //

                    if (error == ERROR_SUCCESS) {

                        DWORD amountToCopy;

                        INET_ASSERT(viewInfo->BufferInfo->Flags & BI_RECEIVE_COMPLETE);

                        AcquireBufferLock(viewInfo->BufferInfo);
                        amountToCopy = viewInfo->BufferInfo->BufferLength;

                        //
                        // if the buffer contains dot-terminated info, then
                        // account for the dot
                        //

                        if ((viewInfo->BufferInfo->Flags & BI_DOT_AT_END)

                        //
                        // this *SHOULD* always be true, but just in case we
                        // have an anomalous situation, we don't want to
                        // return a negative value (i.e. a large DWORD value)
                        //

                        && (amountToCopy > GOPHER_DOT_TERMINATOR_LENGTH)) {
                            amountToCopy -= GOPHER_DOT_TERMINATOR_LENGTH;
                        }
                        if (amountToCopy <= *lpdwBufferLength) {

                            LPBYTE attributeBuffer;

                            attributeBuffer = viewInfo->BufferInfo->Buffer;

                            INET_ASSERT(attributeBuffer != NULL);

                            memcpy(lpBuffer, attributeBuffer, amountToCopy);
                        }

                        //
                        // whether we copied the data or not, indicate to the
                        // caller how much data is available
                        //

                        *lpdwBufferLength = amountToCopy;

                        //
                        // we are done with the buffer. Unlock the BUFFER_INFO
                        // and the VIEW_INFO. Both will probably be destroyed
                        //

                        ReleaseBufferLock(viewInfo->BufferInfo);
                        DereferenceView(viewInfo);
                    }
                } else {
                    error = ERROR_NOT_ENOUGH_MEMORY;
                }
            } else {
                error = ERROR_GOPHER_NOT_GOPHER_PLUS;
            }

            //
            // dereference the session, possibly destroying it
            //

            DereferenceSession(sessionInfo);
        }
    }

quit:

    DEBUG_LEAVE(error);

    return error;
}

#endif // defined(GOPHER_ATTRIBUTE_SUPPORT)

//
//DWORD
//wGopherConnect(
//    IN LPCSTR lpszServerName,
//    IN INTERNET_PORT nServerPort,
//    IN LPCSTR lpszUsername,
//    IN LPCSTR lpszPassword,
//    IN DWORD dwService,
//    IN DWORD dwFlags,
//    OUT LPHINTERNET lpConnectHandle
//    )
//
///*++
//
//Routine Description:
//
//    Creates a default gopher connection information 'object' from the
//    parameters supplied in InternetConnect()
//
//Arguments:
//
//    lpszServerName  - pointer to default gopher server
//
//    nServerPort     - default configured gopher port (0 for use default of 70)
//
//    lpszUsername    - our user's name
//
//    lpszPassword    - and password
//
//    dwService       - INTERNET_SERVICE_GOPHER
//
//    dwFlags         - unused
//
//    lpConnectHandle - where we return the pointer to the 'object'
//
//Return Value:
//
//    DWORD
//        Success - ERROR_SUCCESS
//
//        Failure - ERROR_NOT_ENOUGH_MEMORY
//
//--*/
//
//{
//    LPGOPHER_DEFAULT_CONNECT_INFO info;
//    DWORD error;
//
//    DEBUG_ENTER((DBG_GOPHER,
//                Dword,
//                "wGopherConnect",
//                "%q, %d, %q, %q, %d, %#x, %#x",
//                lpszServerName,
//                nServerPort,
//                lpszUsername,
//                lpszPassword,
//                dwService,
//                dwFlags,
//                lpConnectHandle
//                ));
//
//    UNREFERENCED_PARAMETER(dwService);
//    UNREFERENCED_PARAMETER(dwFlags);
//
//    info = NEW(GOPHER_DEFAULT_CONNECT_INFO);
//    if (info != NULL) {
//        error = ERROR_SUCCESS;
//        if (lpszServerName != NULL) {
//            info->HostName = NEW_STRING((LPSTR)lpszServerName);
//            if (info->HostName == NULL) {
//                error = ERROR_NOT_ENOUGH_MEMORY;
//            }
//        }
//        info->Port = nServerPort;
//        if ((lpszUsername != NULL) && (error == ERROR_SUCCESS)) {
//            info->UserName = NEW_STRING((LPSTR)lpszUsername);
//            if (info->UserName == NULL) {
//                error = ERROR_NOT_ENOUGH_MEMORY;
//            }
//        }
//        if ((lpszPassword != NULL) && (error == ERROR_NOT_ENOUGH_MEMORY)) {
//            info->Password = NEW_STRING((LPSTR)lpszPassword);
//            if (info->Password == NULL) {
//                error = ERROR_NOT_ENOUGH_MEMORY;
//            }
//        }
//    } else {
//        error = ERROR_NOT_ENOUGH_MEMORY;
//    }
//    if (error == ERROR_SUCCESS) {
//        *lpConnectHandle = (HINTERNET)info;
//    } else {
//        if (info != NULL) {
//            if (info->HostName != NULL) {
//                DEL_STRING(info->HostName);
//            }
//            if (info->UserName != NULL) {
//                DEL_STRING(info->UserName);
//            }
//            if (info->Password != NULL) {
//                DEL_STRING(info->Password);
//            }
//            DEL(info);
//        }
//    }
//
//    DEBUG_LEAVE(error);
//
//    return error;
//}
//
//
//DWORD
//wGopherDisconnect(
//    IN HINTERNET hInternet
//    )
//
///*++
//
//Routine Description:
//
//    Undoes the work of wGopherConnect
//
//Arguments:
//
//    hInternet   - handle to object created by wGopherConnect. Actually just a
//                  pointer to GOPHER_DEFAULT_CONNECT_INFO structure
//
//Return Value:
//
//    DWORD
//        Success - ERROR_SUCCESS
//
//        Failure - ERROR_INTERNET_INTERNAL_ERROR
//                    hInternet does not identify a GOPHER_DEFAULT_CONNECT_INFO
//
//--*/
//
//{
//    LPGOPHER_DEFAULT_CONNECT_INFO info;
//    DWORD error;
//
//    DEBUG_ENTER((DBG_GOPHER,
//                Dword,
//                "wGopherDisconnect",
//                "%#x",
//                hInternet
//                ));
//
//    //
//    // BUGBUG - not expecting bogus pointer?!
//    //
//
//    info = (LPGOPHER_DEFAULT_CONNECT_INFO)hInternet;
//    if (info != NULL) {
//        if (info->HostName != NULL) {
//            DEL_STRING(info->HostName);
//        }
//        if (info->UserName != NULL) {
//            DEL_STRING(info->UserName);
//        }
//        if (info->Password != NULL) {
//            DEL_STRING(info->Password);
//        }
//        DEL(info);
//        error = ERROR_SUCCESS;
//    } else {
//        error = ERROR_INTERNET_INTERNAL_ERROR;
//    }
//
//    DEBUG_LEAVE(error);
//
//    return error;
//}

//
// private functions
//


PRIVATE
DWORD
GetView(
    IN LPSESSION_INFO SessionInfo,
    IN VIEW_TYPE ViewType,
    IN LPSTR Request,
    IN BOOL RequestIsGopherPlus,
    IN DWORD ResponseFlags,
    OUT LPVIEW_INFO* pViewInfo
    )

/*++

Routine Description:

    Creates a 'view object'. Sends a request to the gopher server and receives
    the response. The response data is the view. Multiple simultaneous
    requests for the same data from the same server are serialized. It is
    possible that when this function terminates, the entire response may not
    have been received, but we have enough to return to the caller. In this
    case, a background thread may be actively receiving the remainder of the
    response

Arguments:

    SessionInfo         - pointer to SESSION_INFO describing the gopher server

    ViewType            - type of view being requested, File or Find

    Request             - gopher request string

    RequestIsGopherPlus - TRUE if this is a gopher+ request

    ResponseFlags       - bit-mask describing expected response buffer

    pViewInfo           - returned view info

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DEBUG_ENTER((DBG_GOPHER,
                Dword,
                "GetView",
                "%#x, %s, %q, %B, %#x, %#x",
                SessionInfo,
                (ViewType == ViewTypeFile) ? "File" : "Find",
                Request,
                RequestIsGopherPlus,
                ResponseFlags,
                pViewInfo
                ));

    DWORD error;
    LPVIEW_INFO viewInfo;
    BOOL viewCloned;

    //
    // find the view info that contains the results of sending Request to
    // the server identified by the session info. If this succeeds then we
    // will have added another reference to the session info
    //

    viewInfo = CreateView(SessionInfo, ViewType, Request, &error, &viewCloned);
    if (viewInfo != NULL) {

        LPBUFFER_INFO bufferInfo;

        //
        // if this is a gopher+ request then mark it in the VIEW_INFO
        //

        if (RequestIsGopherPlus) {
            viewInfo->Flags |= VI_GOPHER_PLUS;
        }

        bufferInfo = viewInfo->BufferInfo;

        INET_ASSERT(bufferInfo != NULL);

        //
        // set the expected response type (dot-terminated or not.) Only really
        // useful if request is gopher0 file
        //

        bufferInfo->Flags |= ResponseFlags;

        if (!viewCloned) {

            BOOL receiveComplete;

            //
            // if this is a directory view then we need to communicate the fact
            // that ReceiveResponse needs to allocate a buffer
            //

            if (ViewType == ViewTypeFind) {
                bufferInfo->Flags |= BI_BUFFER_RESPONSE;
            }

            //
            // the view was created. We must make the request to the gopher
            // server
            //

            error = GopherTransaction(viewInfo);

            //
            // if there were multiple simultaneous requests for the same data
            // then we must signal the request event to restart those other
            // threads. If no other threads are waiting then we can dispense
            // with the request event
            //

//            AcquireViewLock(SessionInfo, ViewType);

//            if (bufferInfo->RequestWaiters != 0) {

                //
                // the request has completed, maybe with an error. In both
                // cases, we signal the event in the BUFFER_INFO to allow
                // any concurrent requesters of the same information to
                // continue
                //

//                SetEvent(bufferInfo->RequestEvent);
//            } else {

                //
                // no other concurrent waiters. The system can have an event
                // back
                //

//                CloseHandle(bufferInfo->RequestEvent);
//                bufferInfo->RequestEvent = NULL;
//            }

//            ReleaseViewLock(SessionInfo, ViewType);

        } else {

            //
            // we no longer allow file views to be cloned - each request for a
            // file must now make a separate connection to the server
            //

            INET_ASSERT(ViewType != ViewTypeFile);

            //
            // the view was cloned. If we made the request at the same time
            // another thread was making the same request to the server then
            // wait for that other thread to signal the request event. If the
            // request event handle is NULL then we don't have to wait
            //

//            if (bufferInfo->RequestEvent != NULL) {
//                WAIT_FOR_SINGLE_OBJECT(bufferInfo->RequestEvent, error);
//            } else {
//                error = WAIT_OBJECT_0;
//            }

            //
            // if RequestEvent existed when this clone was generated then we
            // decrement the number of waiters, now that we have access to the
            // data.
            // If the number of requesters goes to zero, we grab the view lock
            // for this list. If the number of requesters is still zero then
            // we close the event handle.
            // The event handle was only required for a special purpose - to
            // hold off multiple simultaneous requesters of the same data from
            // the same server. Once the data has been retrieved, there is no
            // need to keep the event
            //

//            if (bufferInfo->RequestWaiters != 0) {
//                if (InterlockedDecrement(&bufferInfo->RequestWaiters) == 0) {

//                    AcquireViewLock(SessionInfo, ViewType);

//                    if (bufferInfo->RequestWaiters == 0) {
//                        CloseHandle(bufferInfo->RequestEvent);
//                        bufferInfo->RequestEvent = NULL;
//                    }

//                    ReleaseViewLock(SessionInfo, ViewType);
//                }
//            }
        }

        //
        // if an error occurred then dereferencing the view should destroy it
        //

        if (error != ERROR_SUCCESS) {
            viewInfo = DereferenceView(viewInfo);
        }
    }

    *pViewInfo = viewInfo;

    DEBUG_LEAVE(error);

    return error;
}
