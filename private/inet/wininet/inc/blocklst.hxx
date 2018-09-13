/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    blocklst.hxx

Abstract:

    Contains types, prototypes, manifests for blocklst.cxx

Author:

    Arthur L Bierer (arthurbi) 18-Feb-1998

Revision History:

    18-Feb-1998 arthurbi
        Created

--*/


DWORD
BlockThreadOnEvent(
    IN DWORD_PTR eventId,
    IN DWORD dwTimeout,
    IN BOOL bReleaseLock
    );

DWORD
SignalThreadOnEvent(
    IN DWORD_PTR dwEventId,
    IN DWORD dwNumberOfWaiters,
    IN DWORD dwReturnCode
    );

VOID
AcquireBlockedRequestQueue(
    VOID
    );

VOID
ReleaseBlockedRequestQueue(
    VOID
    );


//
// AR_TYPE - Asynchronous Request Type designator. Used as index into array of
// ARB sizes, hence must start at 0
//

typedef enum {
    AR_INTERNET_CONNECT = 0,            // 0
    AR_INTERNET_OPEN_URL,               // 1
    AR_INTERNET_READ_FILE,              // 2
    AR_INTERNET_WRITE_FILE,             // 3
    AR_INTERNET_QUERY_DATA_AVAILABLE,   // 4
    AR_INTERNET_FIND_NEXT_FILE,         // 5
    AR_FTP_FIND_FIRST_FILE,             // 6
    AR_FTP_GET_FILE,                    // 7
    AR_FTP_PUT_FILE,                    // 8
    AR_FTP_DELETE_FILE,                 // 9
    AR_FTP_RENAME_FILE,                 // 10
    AR_FTP_OPEN_FILE,                   // 11
    AR_FTP_CREATE_DIRECTORY,            // 12
    AR_FTP_REMOVE_DIRECTORY,            // 13
    AR_FTP_SET_CURRENT_DIRECTORY,       // 14
    AR_FTP_GET_CURRENT_DIRECTORY,       // 15
    AR_GOPHER_FIND_FIRST_FILE,          // 16
    AR_GOPHER_OPEN_FILE,                // 17
    AR_GOPHER_GET_ATTRIBUTE,            // 18
    AR_HTTP_SEND_REQUEST,               // 19
    AR_HTTP_BEGIN_SEND_REQUEST,         // 20
    AR_HTTP_END_SEND_REQUEST,           // 21
    AR_READ_PREFETCH,                   // 22
    AR_SYNC_EVENT,                      // 23
    AR_TIMER_EVENT,                     // 24
    AR_HTTP_REQUEST1,                   // 25
    AR_FILE_IO,                         // 26
    AR_INTERNET_READ_FILE_EX,           // 27
    AR_MAX_REQUEST_TYPE
} AR_TYPE;



