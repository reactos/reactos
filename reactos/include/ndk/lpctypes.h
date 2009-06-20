/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    lpctypes.h

Abstract:

    Type definitions for the Loader.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _LPCTYPES_H
#define _LPCTYPES_H

//
// Dependencies
//
#include <umtypes.h>
//#include <pstypes.h>

//
// Internal helper macro
//
#define N_ROUND_UP(x,s) \
    (((ULONG)(x)+(s)-1) & ~((ULONG)(s)-1))

//
// Maximum message size that can be sent through an LPC Port without a section
//
#ifdef NTOS_MODE_USER
#ifdef _WIN64
#define PORT_MAXIMUM_MESSAGE_LENGTH 512
#else
#define PORT_MAXIMUM_MESSAGE_LENGTH 256
#endif
#endif

//
// Port Object Access Masks
//
#define PORT_CONNECT                    0x1
#define PORT_ALL_ACCESS                 0x1

//
// Port Object Flags
//
#define LPCP_CONNECTION_PORT            0x00000001
#define LPCP_UNCONNECTED_PORT           0x00000002
#define LPCP_COMMUNICATION_PORT         0x00000003
#define LPCP_CLIENT_PORT                0x00000004
#define LPCP_PORT_TYPE_MASK             0x0000000F
#define LPCP_PORT_DELETED               0x10000000
#define LPCP_WAITABLE_PORT              0x20000000
#define LPCP_NAME_DELETED               0x40000000
#define LPCP_SECURITY_DYNAMIC           0x80000000

//
// LPC Message Types
//
typedef enum _LPC_TYPE
{
    LPC_NEW_MESSAGE,
    LPC_REQUEST,
    LPC_REPLY,
    LPC_DATAGRAM,
    LPC_LOST_REPLY,
    LPC_PORT_CLOSED,
    LPC_CLIENT_DIED,
    LPC_EXCEPTION,
    LPC_DEBUG_EVENT,
    LPC_ERROR_EVENT,
    LPC_CONNECTION_REQUEST,
    LPC_CONNECTION_REFUSED,
    LPC_MAXIMUM
} LPC_TYPE;

//
// Information Classes for NtQueryInformationPort
//
typedef enum _PORT_INFORMATION_CLASS
{
    PortNoInformation
} PORT_INFORMATION_CLASS;

#ifdef NTOS_MODE_USER

//
// Portable LPC Types for 32/64-bit compatibility
//
#ifdef USE_LPC6432
#define LPC_CLIENT_ID CLIENT_ID64
#define LPC_SIZE_T ULONGLONG
#define LPC_PVOID ULONGLONG
#define LPC_HANDLE ULONGLONG
#else
#define LPC_CLIENT_ID CLIENT_ID
#define LPC_SIZE_T SIZE_T
#define LPC_PVOID PVOID
#define LPC_HANDLE HANDLE
#endif

//
// LPC Port Message
//
typedef struct _PORT_MESSAGE
{
    union
    {
        struct
        {
            CSHORT DataLength;
            CSHORT TotalLength;
        } s1;
        ULONG Length;
    } u1;
    union
    {
        struct
        {
            CSHORT Type;
            CSHORT DataInfoOffset;
        } s2;
        ULONG ZeroInit;
    } u2;
    union
    {
        LPC_CLIENT_ID ClientId;
        double DoNotUseThisField;
    };
    ULONG MessageId;
    union
    {
        LPC_SIZE_T ClientViewSize;
        ULONG CallbackId;
    };
} PORT_MESSAGE, *PPORT_MESSAGE;

//
// Local and Remove Port Views
//
typedef struct _PORT_VIEW
{
    ULONG Length;
    LPC_HANDLE SectionHandle;
    ULONG SectionOffset;
    LPC_SIZE_T ViewSize;
    LPC_PVOID ViewBase;
    LPC_PVOID ViewRemoteBase;
} PORT_VIEW, *PPORT_VIEW;

typedef struct _REMOTE_PORT_VIEW
{
    ULONG Length;
    LPC_SIZE_T ViewSize;
    LPC_PVOID ViewBase;
} REMOTE_PORT_VIEW, *PREMOTE_PORT_VIEW;

//
// LPC Kernel-Mode Message Structures defined for size only
//
typedef struct _LPCP_MESSAGE
{
    UCHAR Data[0x14];
    PORT_MESSAGE Request;
} LPCP_MESSAGE;

typedef struct _LPCP_CONNECTION_MESSAGE
{
    UCHAR Data[0x2C];
} LPCP_CONNECTION_MESSAGE;

#else

//
// LPC Paged and Non-Paged Port Queues
//
typedef struct _LPCP_NONPAGED_PORT_QUEUE
{
    KSEMAPHORE Semaphore;
    struct _LPCP_PORT_OBJECT *BackPointer;
} LPCP_NONPAGED_PORT_QUEUE, *PLPCP_NONPAGED_PORT_QUEUE;

typedef struct _LPCP_PORT_QUEUE
{
    PLPCP_NONPAGED_PORT_QUEUE NonPagedPortQueue;
    PKSEMAPHORE Semaphore;
    LIST_ENTRY ReceiveHead;
} LPCP_PORT_QUEUE, *PLPCP_PORT_QUEUE;

//
// LPC Port Object
//
typedef struct _LPCP_PORT_OBJECT
{
    struct _LPCP_PORT_OBJECT *ConnectionPort;
    struct _LPCP_PORT_OBJECT *ConnectedPort;
    LPCP_PORT_QUEUE MsgQueue;
    CLIENT_ID Creator;
    PVOID ClientSectionBase;
    PVOID ServerSectionBase;
    PVOID PortContext;
    PETHREAD ClientThread;
    SECURITY_QUALITY_OF_SERVICE SecurityQos;
    SECURITY_CLIENT_CONTEXT StaticSecurity;
    LIST_ENTRY LpcReplyChainHead;
    LIST_ENTRY LpcDataInfoChainHead;
    PEPROCESS ServerProcess;
    PEPROCESS MappingProcess;
    ULONG MaxMessageLength;
    ULONG MaxConnectionInfoLength;
    ULONG Flags;
    KEVENT WaitEvent;
} LPCP_PORT_OBJECT, *PLPCP_PORT_OBJECT;

//
// LPC Kernel-Mode Message Structures
//
typedef struct _LPCP_MESSAGE
{
    union
    {
        LIST_ENTRY Entry;
        struct
        {
            SINGLE_LIST_ENTRY FreeEntry;
            ULONG Reserved0;
        };
    };
    PLPCP_PORT_OBJECT SenderPort;
    PETHREAD RepliedToThread;
    PVOID PortContext;
    PORT_MESSAGE Request;
} LPCP_MESSAGE, *PLPCP_MESSAGE;

typedef struct _LPCP_CONNECTION_MESSAGE
{
    PORT_VIEW ClientView;
    PLPCP_PORT_OBJECT ClientPort;
    PVOID SectionToMap;
    REMOTE_PORT_VIEW ServerView;
} LPCP_CONNECTION_MESSAGE, *PLPCP_CONNECTION_MESSAGE;

#endif

//
// Client Died LPC Message
//
typedef struct _CLIENT_DIED_MSG
{
    PORT_MESSAGE h;
    LARGE_INTEGER CreateTime;
} CLIENT_DIED_MSG, *PCLIENT_DIED_MSG;

//
// Maximum total Kernel-Mode LPC Message Structure Size
//
#define LPCP_MAX_MESSAGE_SIZE \
    N_ROUND_UP(PORT_MAXIMUM_MESSAGE_LENGTH + \
    sizeof(LPCP_MESSAGE) + \
    sizeof(LPCP_CONNECTION_MESSAGE), 16)

//
// Maximum actual LPC Message Length
//
#define LPC_MAX_MESSAGE_LENGTH \
    (LPCP_MAX_MESSAGE_SIZE - \
    FIELD_OFFSET(LPCP_MESSAGE, Request))

//
// Maximum actual size of LPC Message Data
//
#define LPC_MAX_DATA_LENGTH \
    (LPC_MAX_MESSAGE_LENGTH - \
    sizeof(PORT_MESSAGE) - \
    sizeof(LPCP_CONNECTION_MESSAGE))

#endif // _LPCTYPES_H
