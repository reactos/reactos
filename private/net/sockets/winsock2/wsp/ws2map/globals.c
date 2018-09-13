/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    globals.c

Abstract:

    This module contains global variable definitions for the Winsock 2
    to Winsock 1.1 Mapper Service Provider.

Author:

    Keith Moore (keithmo) 29-May-1996

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


//
// This DLL's module handle. We need this to add an artificial reference
// to the DLL so that it doesn't go away while a worker thread is running.
//

HMODULE SockModuleHandle = NULL;


//
// Linked list of all sockets created by this provider.
//

LIST_ENTRY SockGlobalSocketListHead = { NULL };


//
// Critical section protecting global variables.
//

CRITICAL_SECTION SockGlobalLock = { NULL };


//
// TLS slot for per-thread data.
//

DWORD SockTlsSlot = TLS_OUT_OF_INDEXES;


//
// Flags so we know when we're shutting down.
//

BOOL SockProcessTerminating = FALSE;
BOOL SockTerminating = FALSE;


//
// A count of the number of times the client has called WSPStartup().
//

LONG SockWspStartupCount = 0;


//
// Our procedure table, and WS2_32.DLL's upcall table.
//

WSPPROC_TABLE SockProcTable = {

        &WSPAccept,
        &WSPAddressToString,
        &WSPAsyncSelect,
        &WSPBind,
        &WSPCancelBlockingCall,
        &WSPCleanup,
        &WSPCloseSocket,
        &WSPConnect,
        &WSPDuplicateSocket,
        &WSPEnumNetworkEvents,
        &WSPEventSelect,
        &WSPGetOverlappedResult,
        &WSPGetPeerName,
        &WSPGetSockName,
        &WSPGetSockOpt,
        &WSPGetQOSByName,
        &WSPIoctl,
        &WSPJoinLeaf,
        &WSPListen,
        &WSPRecv,
        &WSPRecvDisconnect,
        &WSPRecvFrom,
        &WSPSelect,
        &WSPSend,
        &WSPSendDisconnect,
        &WSPSendTo,
        &WSPSetSockOpt,
        &WSPShutdown,
        &WSPSocket,
        &WSPStringToAddress

    };

WSPUPCALLTABLE SockUpcallTable = { NULL };


//
// Hooker management.
//

LIST_ENTRY SockHookerListHead = { NULL };
HKEY SockHookerRegistryKey = NULL;


//
// QOS stuff.
//

QOS SockDefaultQos =
        {
            {                           // SendingFlowspec
                (ULONG) -1,             // TokenRate
                (ULONG) -1,             // TokenBucketSize
                (ULONG) -1,             // PeakBandwidth
                (ULONG) -1,             // Latency
                (ULONG) -1,             // DelayVariation
                SERVICETYPE_BESTEFFORT, // ServiceType
                (ULONG) -1,             // MaxSduSize
                (ULONG) -1              // MinimumPolicedSize
            },

            {                           // SendingFlowspec
                (ULONG) -1,             // TokenRate
                (ULONG) -1,             // TokenBucketSize
                (ULONG) -1,             // PeakBandwidth
                (ULONG) -1,             // Latency
                (ULONG) -1,             // DelayVariation
                SERVICETYPE_BESTEFFORT, // ServiceType
                (ULONG) -1,             // MaxSduSize
                (ULONG) -1              // MinimumPolicedSize
            },
        };


//
// Debug-specific data.
//

#if DBG

//
// Flags controlling debug output.
//

ULONG SockDebugFlags = 0;

#endif  // DBG
