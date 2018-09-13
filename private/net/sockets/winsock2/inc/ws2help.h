/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    ws2help.h

Abstract:

    Contains declarations for the interface to the OS-specific
    WinSock 2.0 helper routines.

Author:

    Keith Moore (keithmo)        19-Jun-1995

Revision History:

--*/

#ifndef _WS2HELP_H_
#define _WS2HELP_H_


#if defined __cplusplus
extern "C" {
#endif


#if !defined(_WS2HELP_)
#define WS2HELPAPI DECLSPEC_IMPORT
#else
#define WS2HELPAPI
#endif


//
//  APC functions.
//

WS2HELPAPI
DWORD
WINAPI
WahOpenApcHelper(
    OUT LPHANDLE HelperHandle
    );

WS2HELPAPI
DWORD
WINAPI
WahCloseApcHelper(
    IN HANDLE HelperHandle
    );

WS2HELPAPI
DWORD
WINAPI
WahOpenCurrentThread(
    IN  HANDLE HelperHandle,
    OUT LPWSATHREADID ThreadId
    );

WS2HELPAPI
DWORD
WINAPI
WahCloseThread(
    IN HANDLE HelperHandle,
    IN LPWSATHREADID ThreadId
    );

WS2HELPAPI
DWORD
WINAPI
WahQueueUserApc(
    IN HANDLE HelperHandle,
    IN LPWSATHREADID ThreadId,
    IN LPWSAUSERAPC ApcRoutine,
    IN ULONG_PTR ApcContext OPTIONAL
    );

//
// Context functions.
//

typedef struct _CONTEXT_TABLE FAR * LPCONTEXT_TABLE;

#define WAH_CONTEXT_FLAG_SERIALIZE  0x00000001

WS2HELPAPI
DWORD
WINAPI
WahCreateContextTable(
    LPCONTEXT_TABLE FAR * Table,
    DWORD Flags
    );

WS2HELPAPI
DWORD
WINAPI
WahDestroyContextTable(
    LPCONTEXT_TABLE Table
    );

WS2HELPAPI
DWORD
WINAPI
WahSetContext(
    LPCONTEXT_TABLE Table,
    SOCKET Socket,
    LPVOID Context
    );

WS2HELPAPI
DWORD
WINAPI
WahGetContext(
    LPCONTEXT_TABLE Table,
    SOCKET Socket,
    LPVOID FAR * Context
    );

WS2HELPAPI
DWORD
WINAPI
WahRemoveContext(
    LPCONTEXT_TABLE Table,
    SOCKET Socket
    );

WS2HELPAPI
DWORD
WINAPI
WahRemoveContextEx(
    LPCONTEXT_TABLE Table,
    SOCKET Socket,
    LPVOID Context
    );

// Handle function

WS2HELPAPI
DWORD
WINAPI
WahOpenHandleHelper(
    OUT LPHANDLE HelperHandle
    );

WS2HELPAPI
DWORD
WINAPI
WahCloseHandleHelper(
    IN HANDLE HelperHandle
    );


WS2HELPAPI
DWORD
WINAPI
WahCreateSocketHandle(
    IN HANDLE           HelperHandle,
    OUT SOCKET          *s
    );

WS2HELPAPI
DWORD
WINAPI
WahCloseSocketHandle(
    IN HANDLE           HelperHandle,
    IN SOCKET           s
    );

WS2HELPAPI
DWORD
WINAPI
WahCompleteRequest(
    IN HANDLE              HelperHandle,
    IN SOCKET              s,
    IN LPWSAOVERLAPPED     lpOverlapped,
    IN DWORD               dwError,
    IN DWORD               cbTransferred
    );

WS2HELPAPI
DWORD
WINAPI
WahEnableNonIFSHandleSupport (
    VOID
    );

WS2HELPAPI
DWORD
WINAPI
WahDisableNonIFSHandleSupport (
    VOID
    );


// Notification handle functions

WS2HELPAPI
DWORD
WINAPI
WahOpenNotificationHandleHelper(
    OUT LPHANDLE HelperHandle
    );

WS2HELPAPI
DWORD
WINAPI
WahCloseNotificationHandleHelper(
    IN HANDLE HelperHandle
    );

WS2HELPAPI
DWORD
WINAPI
WahCreateNotificationHandle(
    IN HANDLE           HelperHandle,
    OUT HANDLE          *h
    );

WS2HELPAPI
DWORD
WINAPI
WahWaitForNotification(
    IN HANDLE           HelperHandle,
    IN HANDLE           h,
    IN LPWSAOVERLAPPED  lpOverlapped,
    IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    );

WS2HELPAPI
DWORD
WINAPI
WahNotifyAllProcesses (
    IN HANDLE           HelperHandle
    );


//
// New handle -> context lookup functions
//
typedef struct _WSHANDLE_CONTEXT {
    LONG        RefCount;   // Context reference count
    HANDLE      Handle;     // Handle that corresponds to context
} WSHANDLE_CONTEXT, FAR * LPWSHANDLE_CONTEXT;

WS2HELPAPI
DWORD
WINAPI
WahCreateHandleContextTable(
    LPCONTEXT_TABLE FAR * Table
    );

WS2HELPAPI
DWORD
WINAPI
WahDestroyHandleContextTable(
    LPCONTEXT_TABLE Table
    );

WS2HELPAPI
LPWSHANDLE_CONTEXT
WINAPI
WahReferenceContextByHandle(
    LPCONTEXT_TABLE Table,
    HANDLE          Handle
    );

WS2HELPAPI
LPWSHANDLE_CONTEXT
WINAPI
WahInsertHandleContext(
    LPCONTEXT_TABLE     Table,
    LPWSHANDLE_CONTEXT  HContext
    );

WS2HELPAPI
DWORD
WINAPI
WahRemoveHandleContext(
    LPCONTEXT_TABLE     Table,
    LPWSHANDLE_CONTEXT  HContext
    );


typedef
BOOL
(WINAPI * LPFN_CONTEXT_ENUMERATOR)(
    LPVOID              EnumCtx,
    LPWSHANDLE_CONTEXT  HContext
    );

WS2HELPAPI
BOOL
WINAPI
WahEnumerateHandleContexts(
    LPCONTEXT_TABLE         Table,
    LPFN_CONTEXT_ENUMERATOR Enumerator,
    LPVOID                  EnumCtx
    );

#define WahReferenceHandleContext(_ctx)  InterlockedIncrement(&(_ctx)->RefCount)
#define WahDereferenceHandleContext(_ctx)  InterlockedDecrement(&(_ctx)->RefCount)

#if defined __cplusplus
}   // extern "C"
#endif


#endif // _WS2HELP_H_

