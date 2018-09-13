/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    sockets.h

Abstract:

    Contains manifests, macros, types and prototypes for sockets.c

Author:

    Richard L Firth (rfirth) 11-Oct-1994

Revision History:

    11-Oct-1994 rfirth
        Created

--*/

#if defined(__cplusplus)
extern "C" {
#endif

//
// prototypes
//

DWORD
GopherConnect(
    IN LPVIEW_INFO ViewInfo
    );

DWORD
GopherDisconnect(
    IN LPVIEW_INFO ViewInfo,
    IN BOOL AbortConnection
    );

DWORD
GopherSendRequest(
    IN LPVIEW_INFO ViewInfo
    );

DWORD
GopherReceiveResponse(
    IN LPVIEW_INFO ViewInfo,
    OUT LPDWORD BytesReceived
    );

#if defined(__cplusplus)
}
#endif
