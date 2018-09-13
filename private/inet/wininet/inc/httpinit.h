/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    httpinit.h

Abstract:

    Prototypes for httpinit.c

Author:

    Richard L Firth (rfirth) 09-Jun-1995

Revision History:

    09-Jun-1995 rfirth
        Created

--*/

#if defined(__cplusplus)
extern "C" {
#endif

BOOL
HttpInitialize(
    VOID
    );

BOOL
HttpTerminate(
    VOID
    );

#if defined(__cplusplus)
}
#endif
