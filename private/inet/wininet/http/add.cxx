/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    add.cxx

Abstract:

    This file contains the implementation of the HttpAddRequestHeadersA API.

    The following functions are exported by this module:

        HttpAddRequestHeadersA
        HttpAddRequestHeadersW

Author:

    Keith Moore (keithmo) 16-Nov-1994

Revision History:

    Modified to make HttpAddRequestHeadersA remotable. madana (2/8/95)

--*/

#include <wininetp.h>
#include "httpp.h"

//
// private manifests
//

#define VALID_ADD_FLAGS (HTTP_ADDREQ_FLAG_ADD_IF_NEW \
                        | HTTP_ADDREQ_FLAG_ADD \
                        | HTTP_ADDREQ_FLAG_COALESCE_WITH_COMMA \
                        | HTTP_ADDREQ_FLAG_REPLACE \
                        | HTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON \
                        )

//
// functions
//


INTERNETAPI
BOOL
WINAPI
HttpAddRequestHeadersA(
    IN HINTERNET hRequest,
    IN LPCSTR lpszHeaders,
    IN DWORD dwHeadersLength,
    IN DWORD dwModifiers
    )

/*++

Routine Description:

    Appends additional header(s) to an HTTP request handle

Arguments:

    hRequest        - An open HTTP request handle returned by HttpOpenRequest()

    lpszHeaders     - The headers to append to the request. Each header must be
                      terminated by a CR/LF pair.

    dwHeadersLength - The length (in characters) of the headers. If this is -1L
                      then lpszHeaders is assumed to be zero terminated (ASCIIZ)

    dwModifiers     - flags controlling operation. Can be one or more of:

                        HTTP_ADDREQ_FLAG_ADD_IF_NEW
                            - add the header, but only if it does not already
                              exist. Index must be zero

                        HTTP_ADDREQ_FLAG_ADD
                            - if HTTP_ADDREQ_FLAG_REPLACE is set, but the header
                              is not found and this flag is set then the header
                              is added, so long as there is a valid header-value

                        HTTP_ADDREQ_FLAG_COALESCE
                        HTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON
                        HTTP_ADDREQ_FLAG_COALESCE_WITH_COMMA
                            - concatenate headers of same name. E.g. if we
                              already have "Accept: text/html" then adding
                              "Accept: text/*" will create
                              "Accept: text/html, text/*"

                        HTTP_ADDREQ_FLAG_REPLACE
                            - replaces the named header. Only one header can be
                              supplied. If header-value is empty then the header
                              is removed

Return Value:

    Success - TRUE
                The header was appended successfully

    Failure - FALSE
                The operation failed. Error status is available by calling
                GetLastError()

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "HttpAddRequestHeadersA",
                     "%#x, %.80q, %d, %#x",
                     hRequest,
                     lpszHeaders,
                     dwHeadersLength,
                     dwModifiers
                     ));

    DWORD error;
    HINTERNET hRequestMapped = NULL;
    DWORD nestingLevel = 0;

    if (!GlobalDataInitialized) {
        error = ERROR_INTERNET_NOT_INITIALIZED;
        goto done;
    }

    //
    // get the thread info
    //

    LPINTERNET_THREAD_INFO lpThreadInfo;

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL) {
        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto done;
    }

    //
    // map the handle
    //

    error = MapHandleToAddress(hRequest, (LPVOID *)&hRequestMapped, FALSE);
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    _InternetIncNestingCount();
    nestingLevel = 1;

    //
    // validate handle
    //

    BOOL isLocal;
    BOOL isAsync;

    error = RIsHandleLocal(hRequestMapped,
                           &isLocal,
                           &isAsync,
                           TypeHttpRequestHandle
                           );
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // validate parameters
    //

    if ((lpszHeaders == NULL)
    || (*lpszHeaders == '\0')
    || (dwHeadersLength == 0)
    || (dwModifiers & (HTTP_ADDREQ_FLAGS_MASK & ~VALID_ADD_FLAGS))) {
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }

    INET_ASSERT(error == ERROR_SUCCESS);

    //
    // BUGBUG - we should determine whether the app is trying to give us a bogus
    //          header, and whether the header conforms to the format:
    //
    //                          "<header>[:[ <value>]]"
    //

    __try {
        if (dwHeadersLength == (DWORD)-1) {
            dwHeadersLength = (DWORD)lstrlen(lpszHeaders);
        } else {

            //
            // perform our own IsBadStringPtr() - don't have to call another
            // function or register extra exception handlers
            //

            for (DWORD i = 0; i < dwHeadersLength; ++i) {

                char ch = (volatile char)lpszHeaders[i];
            }
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }
    ENDEXCEPT
    if (error == ERROR_SUCCESS) {
        error = wHttpAddRequestHeaders(hRequestMapped,
                                       lpszHeaders,
                                       dwHeadersLength,
                                       dwModifiers
                                       );
    }

quit:

    _InternetDecNestingCount(nestingLevel);

done:

    if (error != ERROR_SUCCESS) {

        DEBUG_ERROR(HTTP, error);

        SetLastError(error);
    }

    if (hRequestMapped != NULL) {
        DereferenceObject((LPVOID)hRequestMapped);
    }

    DEBUG_LEAVE_API(error == ERROR_SUCCESS);

    return error == ERROR_SUCCESS;
}


INTERNETAPI
BOOL
WINAPI
HttpAddRequestHeadersW(
    IN HINTERNET hRequest,
    IN LPCWSTR lpszHeaders,
    IN DWORD dwHeadersLength,
    IN DWORD dwModifiers
    )

/*++

Routine Description:

    Appends additional header(s) to an HTTP request handle.

Arguments:

    hHttpRequest - An open HTTP request handle returned by HttpOpenRequest().

    lpszHeaders - The headers to append to the request. Each header must be
        terminated by a CR/LF pair.

    dwHeadersLength - The length (in characters) of the headers. If this is
        -1L, then lpszHeaders is assumed to be zero terminated (ASCIIZ).

    dwModifiers     -

Return Value:

    TRUE - The header was appended successfully.

    FALSE - The operation failed. Error status is available by calling
        GetLastError().

Comments:

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "HttpAddRequestHeadersW",
                     "%#x, %.80wq, %d, %#x",
                     hRequest,
                     lpszHeaders,
                     dwHeadersLength,
                     dwModifiers
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;

    if (!lpszHeaders)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        MEMORYPACKET mpHeaders;
        ALLOC_MB(lpszHeaders, (dwHeadersLength==-1L ? 0 : dwHeadersLength), mpHeaders);
        if (mpHeaders.psStr)
        {
            UNICODE_TO_ANSI(lpszHeaders, mpHeaders);
            fResult = HttpAddRequestHeadersA(hRequest, mpHeaders.psStr, mpHeaders.dwSize, dwModifiers);
        }
        else
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(HTTP, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}


PUBLIC
DWORD
wHttpAddRequestHeaders(
    IN HINTERNET hRequest,
    IN LPCSTR lpszHeaders,
    IN DWORD dwHeadersLength,
    IN DWORD dwModifiers
    )

/*++

Routine Description:

    Worker function to append additional header(s) to an HTTP request handle

Arguents:

    hRequest        - handle of HTTP request

    lpszHeaders     - pointer to buffer containing one or more headers

    dwHeadersLength - length of lpszHeaders. Cannot be -1 at this stage

    dwModifiers     - flags controlling operation

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_PARAMETER
                    The header string(s) was bad after all

                  ERROR_INTERNET_INCORRECT_HANDLE_STATE
                    We can't add headers to this object at this time

                  ERROR_HTTP_HEADER_NOT_FOUND
                    We were asked to replace a header, but couldn't find it

                  ERROR_HTTP_HEADER_ALREADY_EXISTS
                    We were asked to add a header, only if one of the same name
                    doesn't already exist. It does

--*/

{
    //
    // dwHeadersLength cannot be -1 or 0 at this stage. Nor can lpszHeaders be
    // NULL
    //

    INET_ASSERT(lpszHeaders != NULL);
    INET_ASSERT(dwHeadersLength != (DWORD)-1);
    INET_ASSERT(dwHeadersLength != 0);

    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "wHttpAddRequestHeaders",
                 "%#x, %#x [%.80q], %d, %#x",
                 hRequest,
                 lpszHeaders,
                 lpszHeaders,
                 dwHeadersLength,
                 dwModifiers
                 ));

    //
    // get the underlying object and check that we can add headers
    //

    HTTP_REQUEST_HANDLE_OBJECT * pRequest;

    pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)hRequest;

    DWORD error;

    if (!IS_VALID_HTTP_STATE(pRequest, ADD, TRUE)) {
        error = ERROR_INTERNET_INCORRECT_HANDLE_STATE;
        goto quit;
    }

    DWORD offset;
    LPSTR header;

    offset = 0;
    header = (LPSTR)lpszHeaders;

    do {

        //
        // first time: ignore any empty strings; subsequent time: clean off any
        // trailing line termination
        //

        while ((offset < dwHeadersLength)
        && ((lpszHeaders[offset] == '\r') || (lpszHeaders[offset] == '\n'))) {
            ++offset;
        }
        if (offset == dwHeadersLength) {

            //
            // even if app tried adding empty line(s), we return success
            //

            error = ERROR_SUCCESS;
            break;
        }

        DWORD length;
        DWORD nameLength;
        DWORD valueLength;
        LPSTR value;
        BOOL done;

        nameLength = 0;
        valueLength = 0;
        value = NULL;

        //
        // break the header into header-name, header-value pairs. Exclude CR-LF
        // from the header-value (if present)
        //

        for (length = 0, header = (LPSTR)&lpszHeaders[offset];
            offset < dwHeadersLength;
            ++length, ++offset) {

            char ch = header[length];

            if ((ch == '\r') || (ch == '\n')) {

                //
                // end of this particular header
                //

                break;
            } else if (ch == ':') {
                if (nameLength == 0) {

                    //
                    // found end of header name
                    //

                    nameLength = length;
                    value = &header[length];
                }
            }
        }
        if (length == 0) {

            //
            // empty string
            //

            continue;
        } else if (nameLength == 0) {

            //
            // entry consists of just header-name (e.g. "Accept[\r\n]")
            //

            nameLength = length;
        } else {

            //
            // find the start of the header-value
            //

            valueLength = (DWORD) (header + length - value);

            //
            // N.B. We are allowing any mixture of ':' and ' ' between header
            // name and value, but this is probably not a big deal...
            //

            while ((*value == ':') || (*value == ' ') && (valueLength != 0)) {
                ++value;
                --valueLength;
            }
        }
        if (dwModifiers
            & (HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD_IF_NEW)) {

            //
            // replace or remove the header
            //

            error = pRequest->ReplaceRequestHeader(
                                header,
                                nameLength,
                                value,
                                valueLength,
                                dwModifiers & HTTP_ADDREQ_INDEX_MASK,
                                dwModifiers & HTTP_ADDREQ_FLAGS_MASK
                                );
        } else if (valueLength != 0) {

            //
            // add a single, unterminated header string to the request headers.
            // Since these headers came from the app, we don't trust it to get
            // the header termination right (number & type of line terminators)
            // so we add it ourselves
            //

            error = pRequest->AddRequestHeader(
                                header,
                                nameLength,
                                value,
                                valueLength,
                                dwModifiers & HTTP_ADDREQ_INDEX_MASK,
                                dwModifiers & HTTP_ADDREQ_FLAGS_MASK
                                );
        } else {

            //
            // BUGBUG - we are adding headers, but the header-value is not
            //          present. This is a somewhat tricky situation because we
            //          we may have already added some headers, resulting in
            //          the app not really knowing which headers were good and
            //          which failed; additionally, one or more of the headers
            //          may have been added, increasing the apps confusion. The
            //          best way to handle this (if necessary) is to check the
            //          header name/value pairs w.r.t. the dwModifiers flags.
            //          HOWEVER, even then we can get into a state down here
            //          where we add a couple of headers, then fail...
            //

            error = ERROR_INVALID_PARAMETER;
        }
    } while (error == ERROR_SUCCESS);

quit:

    DEBUG_LEAVE(error);

    return error;
}
