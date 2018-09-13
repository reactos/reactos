//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       lpcsvr.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-12-97   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __LPCSVR_H__
#define __LPCSVR_H__

struct _LPCSVR_MESSAGE ;

typedef struct _LPCSVR_SERVER {
    LARGE_INTEGER Timeout ;         // Default Timeout
    RTL_CRITICAL_SECTION Lock ;     // Lock
    LPCSVR_INITIALIZE Init ;        // Callback functions
    LIST_ENTRY ContextList ;        // List of active contexts
    ULONG ContextCount ;            // Count of contexts
    HANDLE Port ;                   // Server Port
    HANDLE WaitHandle ;             // Thread pool handle
    ULONG Flags ;                   // Flags (below)
    ULONG MessageSize ;             // Size for a message
    struct _LPCSVR_MESSAGE * MessagePool ; // List of message buffers
    ULONG MessagePoolSize ;         // Number of messages
    ULONG MessagePoolLimit ;        // max # messages
    ULONG ReceiveThreads ;          // Number of threads with Recieve pending
    HANDLE ShutdownEvent ;          // Event to signal during shutdown
} LPCSVR_SERVER, * PLPCSVR_SERVER ;

#define LPCSVR_WAITABLE         0x00000001
#define LPCSVR_SHUTDOWN_PENDING 0x00000002
#define LPCSVR_SYNCHRONOUS      0x00000004


typedef struct _LPCSVR_CONTEXT {
    LIST_ENTRY List ;
    PLPCSVR_SERVER Server ;
    HANDLE CommPort ;
    LONG RefCount ;
    UCHAR PrivateContext[ 4 ];
} LPCSVR_CONTEXT, * PLPCSVR_CONTEXT ;

typedef struct _LPCSVR_MESSAGE {
    union {
        struct _LPCSVR_MESSAGE * Next ;
        struct _LPCSVR_CONTEXT * Context ;
    } Header ;

    PORT_MESSAGE Message ;

} LPCSVR_MESSAGE, * PLPCSVR_MESSAGE ;



#endif
