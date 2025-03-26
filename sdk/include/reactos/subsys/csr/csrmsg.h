/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Client/Server Runtime SubSystem
 * FILE:            include/reactos/subsys/csr/csrmsg.h
 * PURPOSE:         Public definitions for communication
 *                  between CSR Clients and Servers
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _CSRMSG_H
#define _CSRMSG_H

#pragma once


#define CSR_PORT_NAME L"ApiPort" // CSR_API_PORT_NAME


#define CSRSRV_SERVERDLL_INDEX      0
#define CSRSRV_FIRST_API_NUMBER     0

typedef enum _CSRSRV_API_NUMBER
{
    CsrpClientConnect = CSRSRV_FIRST_API_NUMBER,
    CsrpThreadConnect,
    CsrpProfileControl,
    CsrpIdentifyAlertableThread,
    CsrpSetPriorityClass,

    CsrpMaxApiNumber
} CSRSRV_API_NUMBER, *PCSRSRV_API_NUMBER;


typedef ULONG CSR_API_NUMBER;

#define CSR_CREATE_API_NUMBER(ServerId, ApiId) \
    (CSR_API_NUMBER)(((ServerId) << 16) | (ApiId))

#define CSR_API_NUMBER_TO_SERVER_ID(ApiNumber) \
    (ULONG)((ULONG)(ApiNumber) >> 16)

#define CSR_API_NUMBER_TO_API_ID(ApiNumber) \
    (ULONG)((ULONG)(ApiNumber) & 0xFFFF)


typedef struct _CSR_API_CONNECTINFO
{
    HANDLE ObjectDirectory; // Unused on Windows >= 2k3
    PVOID  SharedSectionBase;
    PVOID  SharedStaticServerData;
    PVOID  SharedSectionHeap;
    ULONG  DebugFlags;
    ULONG  SizeOfPebData;
    ULONG  SizeOfTebData;
    ULONG  NumberOfServerDllNames;
    HANDLE ServerProcessId;
} CSR_API_CONNECTINFO, *PCSR_API_CONNECTINFO;

#if defined(_M_IX86)
C_ASSERT(sizeof(CSR_API_CONNECTINFO) == 0x24);
#endif

// We must have a size at most equal to the maximum acceptable LPC data size.
C_ASSERT(sizeof(CSR_API_CONNECTINFO) <= LPC_MAX_DATA_LENGTH);


#if (NTDDI_VERSION < NTDDI_WS03)

typedef struct _CSR_IDENTIFY_ALERTABLE_THREAD
{
    CLIENT_ID Cid;
} CSR_IDENTIFY_ALERTABLE_THREAD, *PCSR_IDENTIFY_ALERTABLE_THREAD;

typedef struct _CSR_SET_PRIORITY_CLASS
{
    HANDLE hProcess;
    ULONG PriorityClass;
} CSR_SET_PRIORITY_CLASS, *PCSR_SET_PRIORITY_CLASS;

#endif // (NTDDI_VERSION < NTDDI_WS03)

typedef struct _CSR_CLIENT_CONNECT
{
    ULONG ServerId;
    PVOID ConnectionInfo;
    ULONG ConnectionInfoSize;
} CSR_CLIENT_CONNECT, *PCSR_CLIENT_CONNECT;

typedef struct _CSR_CAPTURE_BUFFER
{
    ULONG Size;
    struct _CSR_CAPTURE_BUFFER *PreviousCaptureBuffer;
    ULONG PointerCount;
    PVOID BufferEnd;
    ULONG_PTR PointerOffsetsArray[ANYSIZE_ARRAY];
} CSR_CAPTURE_BUFFER, *PCSR_CAPTURE_BUFFER;


typedef struct _CSR_API_MESSAGE
{
    PORT_MESSAGE Header;
    union
    {
        CSR_API_CONNECTINFO ConnectionInfo; // Uniquely used in CSRSRV for internal signaling (opening a new connection).
        struct
        {
            PCSR_CAPTURE_BUFFER CsrCaptureData;
            CSR_API_NUMBER ApiNumber;
            NTSTATUS Status; // ReturnValue;
            ULONG Reserved;
            union
            {
                CSR_CLIENT_CONNECT CsrClientConnect;
#if (NTDDI_VERSION < NTDDI_WS03)
                CSR_SET_PRIORITY_CLASS SetPriorityClass;
                CSR_IDENTIFY_ALERTABLE_THREAD IdentifyAlertableThread;
#endif
                //
                // This padding is used to make the CSR_API_MESSAGE structure
                // large enough to hold full other API_MESSAGE-type structures
                // used by other servers. These latter structures's sizes must
                // be checked against the size of CSR_API_MESSAGE by using the
                // CHECK_API_MSG_SIZE macro defined below.
                //
                // This is required because LPC will use this generic structure
                // for communicating all the different servers' messages, and
                // thus we avoid possible buffer overflows with this method.
                // The problems there are, that we have to manually adjust the
                // size of the padding to hope that all the servers' messaging
                // structures will hold in it, or, that we have to be careful
                // to not define too big messaging structures for the servers.
                //
                // Finally, the overall message structure size must be at most
                // equal to the maximum acceptable LPC message size.
                //
                ULONG_PTR ApiMessageData[39];
            } Data;
        };
    };
} CSR_API_MESSAGE, *PCSR_API_MESSAGE;

// We must have a size at most equal to the maximum acceptable LPC message size.
C_ASSERT(sizeof(CSR_API_MESSAGE) <= LPC_MAX_MESSAGE_LENGTH);

// Macro to check that the total size of servers' message structures
// are at most equal to the size of the CSR_API_MESSAGE structure.
#define CHECK_API_MSG_SIZE(type) C_ASSERT(sizeof(type) <= sizeof(CSR_API_MESSAGE))

#endif // _CSRMSG_H

/* EOF */
