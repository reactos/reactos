/*++ BUILD Version: 0002    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    lpc.h

Abstract:

    This module contains the public data structures and procedure
    prototypes for the Local Inter-Process Communication (LPC)
    sub-component of NTOS.

Author:

    Steve Wood (stevewo) 15-May-1989

Revision History:

--*/

#ifndef _LPC_
#define _LPC_

//
// System Initialization procedure for Lpc subcomponent of NTOS
//

BOOLEAN
LpcInitSystem( VOID );

VOID
LpcExitThread(
    PETHREAD Thread
    );

VOID
LpcDumpThread(
    PETHREAD Thread,
    IN POB_DUMP_CONTROL Control OPTIONAL
    );

NTKERNELAPI
NTSTATUS
LpcRequestPort(
    IN PVOID PortAddress,
    IN PPORT_MESSAGE RequestMessage
    );

NTSTATUS
LpcRequestWaitReplyPort(
    IN PVOID PortAddress,
    IN PPORT_MESSAGE RequestMessage,
    OUT PPORT_MESSAGE ReplyMessage
    );


//
// The following are global counters used by the LPC component to indicate
// the amount of LPC calls being performed in the system.
//

extern ULONG LpcCallOperationCount;
extern ULONG LpcCallBackOperationCount;
extern ULONG LpcDatagramOperationCount;

//
// Nonpagable portion of a port queue
//
typedef struct _LPCP_NONPAGED_PORT_QUEUE {
    KSEMAPHORE Semaphore;       // Counting semaphore that is incremented
                                // whenever a message is put in receive queue
    struct _LPCP_PORT_OBJECT *BackPointer;
} LPCP_NONPAGED_PORT_QUEUE, *PLPCP_NONPAGED_PORT_QUEUE;

typedef struct _LPCP_PORT_QUEUE {
    PLPCP_NONPAGED_PORT_QUEUE NonPagedPortQueue;
    PKSEMAPHORE Semaphore;      // Counting semaphore that is incremented
                                // whenever a message is put in receive queue
    LIST_ENTRY ReceiveHead;     // list of messages to receive
} LPCP_PORT_QUEUE, *PLPCP_PORT_QUEUE;

#define LPCP_ZONE_ALIGNMENT 16
#define LPCP_ZONE_ALIGNMENT_MASK ~(LPCP_ZONE_ALIGNMENT-1)

//
// This allows ~96 outstanding messages
//

#define LPCP_ZONE_MAX_POOL_USAGE (8*PAGE_SIZE)
typedef struct _LPCP_PORT_ZONE {
    KEVENT FreeEvent;           // Autoclearing event that is whenever the
                                // zone free list goes from empty to non-empty
    ULONG MaxPoolUsage;
    ULONG GrowSize;
    ZONE_HEADER Zone;
} LPCP_PORT_ZONE, *PLPCP_PORT_ZONE;

//
// Data Types and Constants
//

typedef struct _LPCP_PORT_OBJECT {
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
    LIST_ENTRY LpcReplyChainHead;           // Only in _COMMUNICATION ports
    LIST_ENTRY LpcDataInfoChainHead;        // Only in _COMMUNICATION ports
    PEPROCESS ServerProcess;                // Only in SERVER_CONNECTION ports
    ULONG Reserved;
    KEVENT WaitEvent ;
} LPCP_PORT_OBJECT, *PLPCP_PORT_OBJECT;

//
// Valid values for Flags field
//

#define PORT_TYPE                           0x0000000F
#define SERVER_CONNECTION_PORT              0x00000001
#define UNCONNECTED_COMMUNICATION_PORT      0x00000002
#define SERVER_COMMUNICATION_PORT           0x00000003
#define CLIENT_COMMUNICATION_PORT           0x00000004
#define PORT_WAITABLE                       0x20000000
#define PORT_NAME_DELETED                   0x40000000
#define PORT_DYNAMIC_SECURITY               0x80000000
#define PORT_DELETED                        0x10000000

typedef struct _LPCP_MESSAGE {
    union {
        LIST_ENTRY Entry;
        struct {
            SINGLE_LIST_ENTRY FreeEntry;
            ULONG Reserved0;
        };
    };
    USHORT Reserved1;
    USHORT ZoneIndex;
    ULONG Reserved;
    PETHREAD RepliedToThread;               // Filled in when reply is sent so recipient
                                            // of reply can dereference it.
    PVOID PortContext;                      // Captured from senders communication port.
    PORT_MESSAGE Request;
} LPCP_MESSAGE, *PLPCP_MESSAGE;

#if DEVL
//
// This bit set in the ZoneIndex field to mark allocated messages.
//

#define LPCP_ZONE_MESSAGE_ALLOCATED (USHORT)0x8000
#endif

//
// This data is placed at the beginning of the Request data for an
// LPC_CONNECTION_REQUEST message.
//

typedef struct _LPCP_CONNECTION_MESSAGE {
    PORT_VIEW ClientView;
    PLPCP_PORT_OBJECT ClientPort;
    PVOID SectionToMap;
    REMOTE_PORT_VIEW ServerView;
} LPCP_CONNECTION_MESSAGE, *PLPCP_CONNECTION_MESSAGE;


#endif  // _LPC_
