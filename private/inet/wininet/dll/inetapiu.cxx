/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    inetapiu.cxx

Abstract:

    Contains WinInet API utility & sub-API functions

    Contents:
        wInternetQueryDataAvailable

Author:

    Richard L Firth (rfirth) 16-Feb-1996

Environment:

    Win32 user-level

Revision History:

    16-Feb-1996 rfirth
        Created

--*/

#include <wininetp.h>
#include "inetapiu.h"

DWORD
InbLocalEndCacheWrite(
    IN HINTERNET hFtpFile,
    IN LPSTR    lpszFileExtension,
    IN BOOL fNormal
    );

DWORD
InbGopherLocalEndCacheWrite(
    IN HINTERNET hGopherFile,
    IN LPSTR    lpszFileExtension,
    IN BOOL fNormal
    );

//
// functions
//


BOOL
wInternetQueryDataAvailable(
    IN LPVOID hFileMapped,
    OUT LPDWORD lpdwNumberOfBytesAvailable,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    Part 2 of InternetQueryDataAvailabe. This function is called by the async
    worker thread in order to resume InternetQueryDataAvailable(), and by the
    app as the worker part of the API, post validation

    We can query available data for handle types that return data, either from
    a socket, or from a cache file:

        - HTTP request
        - FTP file
        - FTP find
        - FTP find HTML
        - gopher file
        - gopher find
        - gopher find HTML

Arguments:

    hFileMapped                 - the mapped HINTERNET

    lpdwNumberOfBytesAvailable  - where the number of bytes is returned

    dwFlags                     - flags controlling operation

    dwContext                   - context value for callbacks

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_INET,
                Bool,
                "wInternetQueryDataAvailable",
                "%#x, %#x, %#x, %#x",
                hFileMapped,
                lpdwNumberOfBytesAvailable,
                dwFlags,
                dwContext
                ));

    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();
    DWORD error;
    HINTERNET_HANDLE_TYPE handleType;

    INET_ASSERT(hFileMapped);

    //
    // as usual, grab the per-thread info block
    //

    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    //
    // if this is the async worker thread then set the context, handle, and
    // last-error info in the per-thread data block before we go any further
    // (we already did this on the sync path)
    //

    if (lpThreadInfo->IsAsyncWorkerThread) {
        _InternetSetContext(lpThreadInfo,
                            ((INTERNET_HANDLE_OBJECT *)hFileMapped)->GetContext()
                            );
        _InternetSetObjectHandle(lpThreadInfo,
                                 ((HANDLE_OBJECT *)hFileMapped)->GetPseudoHandle(),
                                 hFileMapped
                                 );
        _InternetClearLastError(lpThreadInfo);

        //
        // we should only be here in async mode if there was no data immediately
        // available
        //

        INET_ASSERT(!((INTERNET_HANDLE_OBJECT *)hFileMapped)->IsDataAvailable());

    }

    //
    // get the local handle for FTP & gopher
    //

    HINTERNET hLocal;

    error = RGetLocalHandle(hFileMapped, &hLocal);
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // we copy the number of bytes available to a local variable first, and
    // only update the caller's variable if we succeed
    //

    DWORD bytesAvailable;

    //
    // get the current data available, based on the handle type
    //

    switch (handleType = ((HANDLE_OBJECT *)hFileMapped)->GetHandleType()) {
    case TypeHttpRequestHandle:
        error = ((HTTP_REQUEST_HANDLE_OBJECT *)hFileMapped)
                    ->QueryDataAvailable(&bytesAvailable);
        break;

    case TypeFtpFileHandle:
    case TypeFtpFindHandle:
        error = wFtpQueryDataAvailable(hLocal, &bytesAvailable);
        break;

    case TypeGopherFileHandle:
    case TypeGopherFindHandle:
        error = wGopherQueryDataAvailable(hLocal, &bytesAvailable);
        break;

    case TypeFtpFindHandleHtml:
    case TypeGopherFindHandleHtml:
        error = QueryHtmlDataAvailable(hFileMapped, &bytesAvailable);
        if (((INTERNET_CONNECT_HANDLE_OBJECT *)hFileMapped)->
                    IsCacheWriteInProgress()) {
            DWORD errorCache = error;

            if ((errorCache == ERROR_SUCCESS) && (bytesAvailable == 0)) {
                errorCache = ERROR_NO_MORE_FILES;
            }
            if (errorCache != ERROR_SUCCESS) {
                if (handleType == TypeFtpFindHandleHtml) {
                    InbLocalEndCacheWrite(  hFileMapped,
                                            "htm",
                                            (errorCache == ERROR_NO_MORE_FILES)
                                         );
                }
                else {
                    InbGopherLocalEndCacheWrite(  hFileMapped,
                                                "htm",
                                                (errorCache == ERROR_NO_MORE_FILES)
                                         );
                }
            }
        }
        break;

#ifdef EXTENDED_ERROR_HTML

    case TypeFtpFileHandleHtml:
        error = QueryHtmlDataAvailable(hFileMapped, &bytesAvailable);
        break;

#endif

    default:
        error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        break;
    }

quit:

    BOOL success;

    if (error == ERROR_SUCCESS) {
        ((INTERNET_HANDLE_OBJECT *)hFileMapped)->SetAvailableDataLength(bytesAvailable);
        *lpdwNumberOfBytesAvailable = bytesAvailable;
        success = TRUE;

        DEBUG_PRINT(INET,
                    INFO,
                    ("%d bytes available\n",
                    bytesAvailable
                    ));

        DEBUG_PRINT_API(API,
                        INFO,
                        ("*lpdwNumberOfBytesAvailable (%#x) = %d\n",
                        lpdwNumberOfBytesAvailable,
                        bytesAvailable
                        ));

    } else {
        success = FALSE;

        DEBUG_ERROR(INET, error);

    }

    SetLastError(error);

    DEBUG_LEAVE(success);

    return success;
}
