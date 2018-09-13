/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    inet.h

Abstract:

    Contains types, prototypes, manifests, macros, etc., for internet support
    functions

Author:

    Richard L Firth (rfirth) 20-Mar-1995

Revision History:

    20-Mar-1995
        Created

--*/

//
// types
//

typedef struct {
    LPSTR HostName;
    INTERNET_PORT Port;
    LPSTR UserName;
    LPSTR Password;
} GOPHER_DEFAULT_CONNECT_INFO, *LPGOPHER_DEFAULT_CONNECT_INFO;
