/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    readhtml.h

Abstract:

    Prototypes, etc. for readhtml.h

Author:

    Richard L Firth (rfirth) 26-Jun-1995

Revision History:

    26-Jun-1995 rfirth
        Created

--*/

#if defined(__cplusplus)
extern "C" {
#endif

//
// prototypes
//

BOOL
ReadHtmlUrlData(
    IN HINTERNET hInternet,
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength,
    OUT LPDWORD lpdwBytesReturned
    );

DWORD
QueryHtmlDataAvailable(
    IN HINTERNET hInternet,
    OUT LPDWORD lpdwNumberOfBytesAvailable
    );

#if defined(__cplusplus)
}
#endif
