/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/lpctypes.h
 * PURPOSE:         Definitions for Local Procedure Call Types not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _LPCTYPES_H
#define _LPCTYPES_H

/* DEPENDENCIES **************************************************************/

/* EXPORTED DATA *************************************************************/

/* ENUMERATIONS **************************************************************/

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

/* TYPES *********************************************************************/

/* 
 * Native Structures in IFS. Duplicated here for user-mode.
 * Also duplicated if the IFS is not present. Until the WDK is
 * released, we should not force the usage of a 100$ kit.
 */
#if defined(NTOS_MODE_USER) || !(defined(_NTIFS_))

#if defined(USE_LPC6432)
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

typedef struct _LPCP_MESSAGE
{
    UCHAR Data[0x14];
    PORT_MESSAGE Request;
} LPCP_MESSAGE;

typedef struct _LPCP_CONNECTION_MESSAGE
{
    UCHAR Data[0x2C];
} LPCP_CONNECTION_MESSAGE;

/* Kernel-Mode Structures */
#else

typedef struct _LPCP_NONPAGED_PORT_QUEUE
{
    KSEMAPHORE Semaphore;
    struct _LPCP_PORT_OBJECT *BackPointer;
} LPCP_NONPAGED_PORT_QUEUE, *PLPCP_NONPAGED_PORT_QUEUE;

typedef struct _LPCP_PORT_QUEUE
{
    PLPCP_NONPAGED_PORT_QUEUE NonPagedPortQueue;
    KSEMAPHORE Semaphore;
    LIST_ENTRY ReceiveHead;
} LPCP_PORT_QUEUE, *PLPCP_PORT_QUEUE;

#ifdef _NTIFS_
typedef struct _LPCP_PORT_OBJECT
{
    ULONG Length;
    ULONG Flags;
    struct _LPCP_PORT_OBJECT *ConnectionPort;
    struct _LPCP_PORT_OBJECT *ConnectedPort;
    LPCP_PORT_QUEUE MsgQueue;
    CLIENT_ID Creator;
    PVOID ClientSectionBase;
    PVOID ServerSectionBase;
    PVOID PortContext;
    ULONG MaxMessageLength;
    ULONG MaxConnectionInfoLength;
    PETHREAD ClientThread;
    SECURITY_QUALITY_OF_SERVICE SecurityQos;
    SECURITY_CLIENT_CONTEXT StaticSecurity;
    LIST_ENTRY LpcReplyChainHead;
    LIST_ENTRY LpcDataInfoChainHead;
} LPCP_PORT_OBJECT, *PLPCP_PORT_OBJECT;

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

#endif

/* CONSTANTS *****************************************************************/

#define PORT_MAXIMUM_MESSAGE_LENGTH 256

#define LPCP_MAX_MESSAGE_SIZE \
    ROUND_UP(PORT_MAXIMUM_MESSAGE_LENGTH + \
    sizeof(LPCP_MESSAGE) + \
    sizeof(LPCP_CONNECTION_MESSAGE), 16)

#define LPC_MAX_MESSAGE_LENGTH \
    (LPCP_MAX_MESSAGE_SIZE - \
    FIELD_OFFSET(LPCP_MESSAGE, Request))

#define LPC_MAX_DATA_LENGTH \
    (LPC_MAX_MESSAGE_LENGTH - \
    sizeof(PORT_MESSAGE) - \
    sizeof(LPCP_CONNECTION_MESSAGE))

#endif
