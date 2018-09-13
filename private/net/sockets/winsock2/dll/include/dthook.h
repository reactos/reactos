/*++

  Copyright (c) 1995 Intel Corp

  Module Name:

    dthook.h

  Abstract:

    Header file containing definitions, function prototypes, and other
    stuff for the Debug/Trace hooks in WinSock 2.

  Author:

    Michael A. Grafton

--*/

#ifndef DTHOOK_H
#define DTHOOK_H

#include "warnoff.h"
#include <windows.h>
#include "dt_dll.h"


//
// Function Declarations
//

LPFNWSANOTIFY
GetPreApiNotifyFP(void);

LPFNWSANOTIFY
GetPostApiNotifyFP(void);

void
DTHookInitialize(void);

void
DTHookShutdown(void);



#ifdef DEBUG_TRACING

#define PREAPINOTIFY(x) \
    ( GetPreApiNotifyFP()  ? ( (*(GetPreApiNotifyFP())) x ) : FALSE)
#define POSTAPINOTIFY(x) \
    if ( GetPostApiNotifyFP() ) { \
         (VOID) ( (*(GetPostApiNotifyFP())) x ); \
    } else

#else

#define PREAPINOTIFY(x) FALSE
#define POSTAPINOTIFY(x)

#endif  // DEBUG_TRACING


#ifdef DEBUG_TRACING
// In  this  case we need function prototypes for the DTHOOK_ prefaced versions
// of  all  the upcall functions.  Alas, the task of keeping these identical to
// the normal WPU function prototypes is an error-prone manual process.

#ifdef __cplusplus
extern "C" {
#endif

BOOL WSPAPI DTHOOK_WPUCloseEvent( WSAEVENT hEvent,
                           LPINT lpErrno );

int WSPAPI DTHOOK_WPUCloseSocketHandle( SOCKET s,
                                 LPINT lpErrno );

WSAEVENT WSPAPI DTHOOK_WPUCreateEvent( LPINT lpErrno );

SOCKET WSPAPI DTHOOK_WPUCreateSocketHandle( DWORD dwCatalogEntryId,
                                     DWORD_PTR dwContext,
                                     LPINT lpErrno );

int WSPAPI DTHOOK_WPUFDIsSet ( SOCKET s,
                        fd_set FAR * set );

int WSPAPI DTHOOK_WPUGetProviderPath( LPGUID lpProviderId,
                               WCHAR FAR * lpszProviderDllPath,
                               LPINT lpProviderDllPathLen,
                               LPINT lpErrno );

SOCKET WSPAPI DTHOOK_WPUModifyIFSHandle( DWORD dwCatalogEntryId,
                                  SOCKET ProposedHandle,
                                  LPINT lpErrno );

BOOL WSPAPI DTHOOK_WPUPostMessage( HWND hWnd,
                            UINT Msg,
                            WPARAM wParam,
                            LPARAM lParam );

int WSPAPI DTHOOK_WPUQueryBlockingCallback( DWORD dwCatalogEntryId,
                                     LPBLOCKINGCALLBACK FAR * lplpfnCallback,
                                     PDWORD_PTR lpdwContext,
                                     LPINT lpErrno );

int WSPAPI DTHOOK_WPUQuerySocketHandleContext( SOCKET s,
                                        PDWORD_PTR lpContext,
                                        LPINT lpErrno );

int WSPAPI DTHOOK_WPUQueueApc( LPWSATHREADID lpThreadId,
                        LPWSAUSERAPC lpfnUserApc,
                        DWORD_PTR dwContext,
                        LPINT lpErrno );

BOOL WSPAPI DTHOOK_WPUResetEvent( WSAEVENT hEvent,
                           LPINT lpErrno );

BOOL WSPAPI DTHOOK_WPUSetEvent( WSAEVENT hEvent,
                         LPINT lpErrno );

#ifdef __cplusplus
}
#endif

#endif  // DEBUG_TRACING


#endif  // DTHOOK_H

