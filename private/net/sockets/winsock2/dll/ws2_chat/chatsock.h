/*++

Copyright (c) 1995 Intel Corp

Module Name:

    chatsock.h

Abstract:

    Header file containing constants, data structure definitions, and
    function prototypes for socket.c.

Author:

    Dan Chou & Michael Grafton

--*/

#ifndef CHTSOCK_H
#define CHTSOCK_H

#include "nowarn.h"  /* turn off benign warnings */
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_   /* Prevent inclusion of winsock.h in windows.h */
#endif
#include <windows.h>
#include "nowarn.h"  /* some warnings may have been turned back on */
#include <winsock2.h>


// 
// Manifest Constants
//

#define WSA_WAIT_EVENT_1  WSA_WAIT_EVENT_0+1
#define WSA_WAIT_EVENT_2  WSA_WAIT_EVENT_0+2




//
// Typedefs
//

typedef struct _CALLBACK_INFO {
    BOOL  DoneYet;   // has the overlapped I/O completed yet?
    LPSTR Buffer;    // the buffer we gave to WSASend
} CALLBACK_INFO, *PCALLBACK_INFO;




// 
// Function Prototypes -- Exported Functions
//

BOOL
InitWS2(void);

BOOL
FindProtocols(void);

BOOL
ListenAll(void);

void
HandleAcceptMessage(
    IN HWND   ConnectionWindow,
    IN SOCKET Socket,
    IN LPARAM LParam);

void 
HandleConnectMessage(
    IN HWND   ConnectionWindow,
    IN LPARAM LongParam);

BOOL
IsSendable(
    char Character);

PCONNDATA
GetConnData(
    IN HWND ConnectionWindow);

BOOL
MakeConnection(
    IN HWND WindowHandle);

BOOL
FillInFamilies(
    IN HWND  DialogWindow,
    IN DWORD FamilyLB);


LPWSAPROTOCOL_INFO
GetProtoFromIndex(
    IN int LBIndex);

void
CleanUpSockets(void);

BOOL
GetAddressString(
    OUT char            *String,
    IN  LPVOID          SockAddr,
    IN  int             SockAddrLen,
    IN  LPWSAPROTOCOL_INFO ProtocolInfo);

BOOL
UseProtocol(
    IN LPWSAPROTOCOL_INFO Proto);

void
CleanupConnection(
    IN PCONNDATA ConnData);

#endif // CHTSOCK_H
