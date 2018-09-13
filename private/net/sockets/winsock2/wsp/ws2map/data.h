/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    data.h

Abstract:

    This module contains global variable declarations for the Winsock 2
    to Winsock 1.1 Mapper Service Provider.

Author:

    Keith Moore (keithmo) 29-May-1996

Revision History:

--*/


#ifndef _DATA_H_
#define _DATA_H_


//
// This DLL's module handle. We need this to add an artificial reference
// to the DLL so that it doesn't go away while a worker thread is running.
//

extern HMODULE SockModuleHandle;


//
// Linked list of all sockets created by this provider.
//

extern LIST_ENTRY SockGlobalSocketListHead;


//
// Critical section protecting global variables.
//

extern CRITICAL_SECTION SockGlobalLock;


//
// TLS slot for per-thread data.
//

extern DWORD SockTlsSlot;


//
// Flags so we know when we're shutting down.
//

extern BOOL SockProcessTerminating;
extern BOOL SockTerminating;


//
// A count of the number of times the client has called WSPStartup().
//

extern LONG SockWspStartupCount;


//
// Our procedure table, and WS2_32.DLL's upcall table.
//

extern WSPPROC_TABLE SockProcTable;
extern WSPUPCALLTABLE SockUpcallTable;


//
// Hooker management.
//

extern LIST_ENTRY SockHookerListHead;
extern HKEY SockHookerRegistryKey;


//
// QOS stuff.
//

extern QOS SockDefaultQos;


#if DBG

//
// Debug flags.
//

extern ULONG SockDebugFlags;

#endif  // DBG


#endif // _DATA_H_

