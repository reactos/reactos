/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Client/Server Runtime SubSystem
 * FILE:            include/reactos/subsys/csr/csrmsg.h
 * PURPOSE:         Public definitions for communication
 *                  between CSR Clients and Servers
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
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
    CsrpIdentifyAlertable,
    CsrpSetPriorityClass,

    CsrpMaxApiNumber
} CSRSRV_API_NUMBER, *PCSRSRV_API_NUMBER;


/*
typedef union _CSR_API_NUMBER
{
    WORD Index;
    WORD Subsystem;
} CSR_API_NUMBER, *PCSR_API_NUMBER;
*/
typedef ULONG CSR_API_NUMBER;

#define CSR_CREATE_API_NUMBER(ServerId, ApiId) \
    (CSR_API_NUMBER)(((ServerId) << 16) | (ApiId))

#define CSR_API_NUMBER_TO_SERVER_ID(ApiNumber) \
    (ULONG)((ULONG)(ApiNumber) >> 16)

#define CSR_API_NUMBER_TO_API_ID(ApiNumber) \
    (ULONG)((ULONG)(ApiNumber) & 0xFFFF)


typedef struct _CSR_CONNECTION_INFO
{
    ULONG Version;
    ULONG Unknown;
    HANDLE ObjectDirectory;
    PVOID SharedSectionBase;
    PVOID SharedSectionHeap;
    PVOID SharedSectionData;
    ULONG DebugFlags;
    ULONG Unknown2[3];
    HANDLE ProcessId;
} CSR_CONNECTION_INFO, *PCSR_CONNECTION_INFO;

typedef struct _CSR_IDENTIFY_ALTERTABLE_THREAD
{
    CLIENT_ID Cid;
} CSR_IDENTIFY_ALTERTABLE_THREAD, *PCSR_IDENTIFY_ALTERTABLE_THREAD;

typedef struct _CSR_SET_PRIORITY_CLASS
{
    HANDLE hProcess;
    ULONG PriorityClass;
} CSR_SET_PRIORITY_CLASS, *PCSR_SET_PRIORITY_CLASS;

typedef struct
{
    HANDLE  UniqueThread;
    CLIENT_ID Cid;
} CSRSS_IDENTIFY_ALERTABLE_THREAD, *PCSRSS_IDENTIFY_ALERTABLE_THREAD;

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
    ULONG_PTR BufferEnd;
    ULONG_PTR PointerArray[1];
} CSR_CAPTURE_BUFFER, *PCSR_CAPTURE_BUFFER;


#include "csrss.h" // remove it when the data structures are not used anymore.

/* Keep in sync with definition below. */
// #define CSRSS_HEADER_SIZE (sizeof(PORT_MESSAGE) + sizeof(ULONG) + sizeof(NTSTATUS))

typedef struct _CSR_API_MESSAGE
{
    PORT_MESSAGE Header;
    union
    {
        CSR_CONNECTION_INFO ConnectionInfo; // Uniquely used in csrss/csrsrv for internal signaling (opening a new connection).
        struct
        {
            PCSR_CAPTURE_BUFFER CsrCaptureData;
            CSR_API_NUMBER ApiNumber;
            ULONG Status; // ReturnValue; // NTSTATUS Status
            ULONG Reserved;
            union
            {
                CSR_CLIENT_CONNECT CsrClientConnect;

                CSR_SET_PRIORITY_CLASS SetPriorityClass;
                CSR_IDENTIFY_ALTERTABLE_THREAD IdentifyAlertableThread;

            /*** win32csr thingies to remove. ***/
#if 1
                CSRSS_CREATE_DESKTOP CreateDesktopRequest;
                CSRSS_SHOW_DESKTOP ShowDesktopRequest;
                CSRSS_HIDE_DESKTOP HideDesktopRequest;
#endif
            /************************************/
            } Data;
        };
    };
} CSR_API_MESSAGE, *PCSR_API_MESSAGE;

#endif // _CSRMSG_H

/* EOF */
