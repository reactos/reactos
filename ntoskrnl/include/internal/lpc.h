/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/include/internal/lpc.h
* PURPOSE:         Internal header for the Local Procedure Call
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*/

//
// Define this if you want debugging support
//
#define _LPC_DEBUG_                                         0x00

//
// These define the Debug Masks Supported
//
#define LPC_CREATE_DEBUG                                    0x01
#define LPC_CLOSE_DEBUG                                     0x02
#define LPC_CONNECT_DEBUG                                   0x04
#define LPC_LISTEN_DEBUG                                    0x08
#define LPC_REPLY_DEBUG                                     0x10
#define LPC_COMPLETE_DEBUG                                  0x20
#define LPC_SEND_DEBUG                                      0x40

//
// Debug/Tracing support
//
#if _LPC_DEBUG_
#ifdef NEW_DEBUG_SYSTEM_IMPLEMENTED // enable when Debug Filters are implemented
#define LPCTRACE(x, ...)                                    \
    {                                                       \
        DbgPrintEx("%s [%.16s] - ",                         \
                   __FUNCTION__,                            \
                   PsGetCurrentProcess()->ImageFileName);   \
        DbgPrintEx(__VA_ARGS__);                            \
    }
#else
#define LPCTRACE(x, ...)                                    \
    if (x & LpcpTraceLevel)                                 \
    {                                                       \
        DbgPrint("%s [%.16s:%lx] - ",                       \
                 __FUNCTION__,                              \
                 PsGetCurrentProcess()->ImageFileName,      \
                 PsGetCurrentThreadId());                   \
        DbgPrint(__VA_ARGS__);                              \
    }
#endif
#else
#define LPCTRACE(x, fmt, ...) DPRINT(fmt, ##__VA_ARGS__)
#endif

//
// LPC Port/Message Flags
//
#define LPCP_THREAD_FLAG_IS_PORT                            1
#define LPCP_THREAD_FLAG_NO_IMPERSONATION                   2
#define LPCP_THREAD_FLAGS                                   (LPCP_THREAD_FLAG_IS_PORT | \
                                                             LPCP_THREAD_FLAG_NO_IMPERSONATION)

//
// LPC Locking Flags
//
#define LPCP_LOCK_HELD      1
#define LPCP_LOCK_RELEASE   2


typedef struct _LPCP_DATA_INFO
{
    ULONG NumberOfEntries;
    struct
    {
        PVOID BaseAddress;
        ULONG DataLength;
    } Entries[1];
} LPCP_DATA_INFO, *PLPCP_DATA_INFO;


//
// Internal Port Management
//
VOID
NTAPI
LpcpClosePort(
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG ProcessHandleCount,
    IN ULONG SystemHandleCount
);

VOID
NTAPI
LpcpDeletePort(
    IN PVOID ObjectBody
);

NTSTATUS
NTAPI
LpcpInitializePortQueue(
    IN PLPCP_PORT_OBJECT Port
);

VOID
NTAPI
LpcpFreeToPortZone(
    IN PLPCP_MESSAGE Message,
    IN ULONG LockFlags
);

VOID
NTAPI
LpcpMoveMessage(
    IN PPORT_MESSAGE Destination,
    IN PPORT_MESSAGE Origin,
    IN PVOID Data,
    IN ULONG MessageType,
    IN PCLIENT_ID ClientId
);

VOID
NTAPI
LpcpSaveDataInfoMessage(
    IN PLPCP_PORT_OBJECT Port,
    IN PLPCP_MESSAGE Message,
    IN ULONG LockFlags
);

//
// Module-external utlity functions
//
VOID
NTAPI
LpcExitThread(
    IN PETHREAD Thread
);

//
// Initialization functions
//
BOOLEAN
NTAPI
LpcInitSystem(
    VOID
);

BOOLEAN
NTAPI
LpcpValidateClientPort(
    PETHREAD ClientThread,
    PLPCP_PORT_OBJECT Port);


//
// Global data inside the Process Manager
//
extern POBJECT_TYPE LpcPortObjectType;
extern ULONG LpcpNextMessageId, LpcpNextCallbackId;
extern KGUARDED_MUTEX LpcpLock;
extern PAGED_LOOKASIDE_LIST LpcpMessagesLookaside;
extern ULONG LpcpMaxMessageSize;
extern ULONG LpcpTraceLevel;

//
// Inlined Functions
//
#include "lpc_x.h"
