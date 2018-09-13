/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    Async.h

Abstract:

    Global definitions for the WinSock asynchronous processing thread.

Author:

    Keith Moore (keithmo)        18-Jun-1992

Revision History:

--*/


#ifndef _ASYNC_H_
#define _ASYNC_H_


//
// Context block.
//

typedef struct _WINSOCK_CONTEXT_BLOCK {

    LIST_ENTRY AsyncThreadQueueListEntry;
    HANDLE TaskHandle;
    DWORD OpCode;

    union {

        struct {
            HWND hWnd;
            unsigned int wMsg;
            PCHAR Filter;
            int Length;
            int Type;
            PCHAR Buffer;
            int BufferLength;
        } AsyncGetHost;

        struct {
            HWND hWnd;
            unsigned int wMsg;
            PCHAR Filter;
            PCHAR Buffer;
            int BufferLength;
        } AsyncGetProto;

        struct {
            HWND hWnd;
            unsigned int wMsg;
            PCHAR Filter;
            PCHAR Protocol;
            PCHAR Buffer;
            int BufferLength;
        } AsyncGetServ;

    } Overlay;

} WINSOCK_CONTEXT_BLOCK, *PWINSOCK_CONTEXT_BLOCK;

//
// Opcodes for processing by the winsock asynchronous processing
// thread.
//

#define WS_OPCODE_GET_HOST_BY_ADDR    0x01
#define WS_OPCODE_GET_HOST_BY_NAME    0x02
#define WS_OPCODE_GET_PROTO_BY_NUMBER 0x03
#define WS_OPCODE_GET_PROTO_BY_NAME   0x04
#define WS_OPCODE_GET_SERV_BY_PORT    0x05
#define WS_OPCODE_GET_SERV_BY_NAME    0x06
#define WS_OPCODE_TERMINATE           0x07


//
// Initialization/termination functions.
//

BOOL
SockAsyncGlobalInitialize(
    VOID
    );

VOID
SockAsyncGlobalTerminate(
    VOID
    );

BOOL
SockCheckAndInitAsyncThread(
    VOID
    );

VOID
SockTerminateAsyncThread(
    VOID
    );


//
// Work queue functions.
//

PWINSOCK_CONTEXT_BLOCK
SockAllocateContextBlock(
    DWORD AdditionalSpace
    );

VOID
SockFreeContextBlock(
    IN PWINSOCK_CONTEXT_BLOCK ContextBlock
    );

VOID
SockQueueRequestToAsyncThread(
    IN PWINSOCK_CONTEXT_BLOCK ContextBlock
    );

INT
SockCancelAsyncRequest(
    IN HANDLE TaskHandle
    );


#endif  // _ASYNC_H_

