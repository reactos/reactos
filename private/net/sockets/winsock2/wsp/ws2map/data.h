/*++
Copyright (c) 1996 Microsoft Corporation

Module Name:
    data.h

Abstract:
    This module contains global variable declarations for the Winsock 2  to Winsock 1.1 Mapper Service Provider.

Author:
    Keith Moore (keithmo) 29-May-1996
*/


#ifndef _DATA_H_
#define _DATA_H_

// This DLL's module handle. We need this to add an artificial reference to the DLL so that it doesn't go away while a worker thread is running.
extern HMODULE SockModuleHandle;

extern LIST_ENTRY SockGlobalSocketListHead;// Linked list of all sockets created by this provider.
extern CRITICAL_SECTION SockGlobalLock;// Critical section protecting global variables.
extern DWORD SockTlsSlot;// TLS slot for per-thread data.

// Flags so we know when we're shutting down.
extern BOOL SockProcessTerminating;
extern BOOL SockTerminating;

extern LONG SockWspStartupCount;// A count of the number of times the client has called WSPStartup().

// Our procedure table, and WS2_32.DLL's upcall table.
extern WSPPROC_TABLE SockProcTable;
extern WSPUPCALLTABLE SockUpcallTable;

// Hooker management.
extern LIST_ENTRY SockHookerListHead;
extern HKEY SockHookerRegistryKey;

extern QOS SockDefaultQos;// QOS stuff.


#if DBG
extern ULONG SockDebugFlags;// Debug flags.
#endif  // DBG

#endif // _DATA_H_