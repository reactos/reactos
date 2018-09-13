/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    icasync.hxx

Abstract:

    Contains types, prototypes, manifests for async thread

Author:

    Richard L Firth (rfirth) 04-Mar-1998

Revision History:

    04-Mar-1998 rfirth
        Created

--*/

//
// manifests
//

#define TP_NO_TIMEOUT               0xffffffff
#define TP_NO_PRIORITY_CHANGE       (-1)

//
// prototypes
//

#if defined(__cplusplus)
extern "C" {
#endif

DWORD
InitializeAsyncSupport(
    VOID
    );

VOID
TerminateAsyncSupport(
    VOID
    );

//DWORD
//QueueWorkItem(
//    IN CFsm * pWorkItem
//    );
//
DWORD
QueueSocketWorkItem(
    IN CFsm * WorkItem,
    IN SOCKET Socket
    );

DWORD
BlockWorkItem(
    IN CFsm * WorkItem,
    IN DWORD_PTR dwBlockId,
    IN DWORD dwTimeout = TP_NO_TIMEOUT
    );

DWORD
CheckForBlockedWorkItems(
    IN DWORD dwCount,
    IN DWORD_PTR dwBlockId
    );

DWORD
UnblockWorkItems(
    IN DWORD dwCount,
    IN DWORD_PTR dwBlockId,
    IN DWORD dwError,
    IN LONG lPriority = TP_NO_PRIORITY_CHANGE
    );

#if defined(__cplusplus)
}
#endif
