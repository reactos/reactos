/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    lpcp.h

Abstract:

    Private include file for the LPC subcomponent of the NTOS project

Author:

    Steve Wood (stevewo) 15-May-1989

Revision History:


!!!!!!  WARNING : this file is included in ..\ob\obref.c

--*/

#include "ntos.h"
#include <zwapi.h>

//
//  define LPCP_TRACE_SERVER_THREADS symbol to track the server threads
//  on a LPCP_SERVER_COMMUNICATION port
//
//  #define LPCP_TRACE_SERVER_THREADS
//

//
//  Global Mutex to guard the following fields:
//
//      ETHREAD.LpcReplyMsg
//      LPCP_PORT_QUEUE.ReceiveHead
//
//  Mutex is never held longer than is necessary to modify or read the field.
//  Contains additional fields to track the owner of the lock and to allows recursive locks
//

typedef struct _LPC_MUTEX {

    FAST_MUTEX  Lock;

    //
    //  field that holds the thread that owns the lock
    //

    PETHREAD    Owner;

    //
    //  Number of locks acquired
    //

    ULONG       Count;

} LPC_MUTEX, *PLPC_MUTEX;


extern LPC_MUTEX LpcpLock;

LPCP_PORT_ZONE LpcpZone;

ULONG LpcpNextMessageId;

ULONG LpcpNextCallbackId;

#define LpcpGenerateMessageId() \
    LpcpNextMessageId++;    if (LpcpNextMessageId == 0) LpcpNextMessageId = 1;

#define LpcpGenerateCallbackId() \
    LpcpNextCallbackId++;    if (LpcpNextCallbackId == 0) LpcpNextCallbackId = 1;

#if DEVL
ULONG LpcpTotalNumberOfMessages;
#endif

//
//  Global macrodefinitions to acquire and release the LPC_MUTEX
//  in order to track the owner and allow recursive calls
//

#define LpcpInitializeLpcpLock()                             \
{                                                            \
    ExInitializeFastMutex( &LpcpLock.Lock );                 \
    LpcpLock.Owner = NULL;                                   \
    LpcpLock.Count = 0;                                      \
}

#define LpcpAcquireLpcpLock()                                       \
{                                                                   \
    if ( LpcpLock.Owner == PsGetCurrentThread() ) {                 \
                                                                    \
        ASSERT ( LpcpLock.Count >= 1 );                             \
        LpcpLock.Count += 1;                                        \
                                                                    \
    } else {                                                        \
                                                                    \
        ExAcquireFastMutex( &LpcpLock.Lock );                       \
        LpcpLock.Owner = PsGetCurrentThread();                      \
        LpcpLock.Count = 1;                                         \
    }                                                               \
}

#define LpcpReleaseLpcpLock()                                               \
{                                                                           \
    ASSERT( LpcpLock.Owner == PsGetCurrentThread() );                       \
    ASSERT( LpcpLock.Count >= 1 );                                          \
                                                                            \
    LpcpLock.Count -= 1;                                                    \
                                                                            \
    if ( LpcpLock.Count == 0 ) {                                            \
                                                                            \
        LpcpLock.Owner = NULL;                                              \
        ExReleaseFastMutex( &LpcpLock.Lock );                               \
    }                                                                       \
}


#ifdef LPCP_TRACE_SERVER_THREADS

//
//  LPCP_PORT_EXTRA_DATA structure and manipulation functions
//

#define LPCP_SERVER_THREAD_ARRAY_SIZE   16

typedef struct _LPCP_PORT_EXTRA_DATA {

    ULONG ThreadCount;

    PLPCP_PORT_OBJECT PortObject;

    PVOID Threads[LPCP_SERVER_THREAD_ARRAY_SIZE];

} LPCP_PORT_EXTRA_DATA, *PLPCP_PORT_EXTRA_DATA;

VOID LpcpPortExtraDataCreate ( PLPCP_PORT_OBJECT PortObject );

VOID LpcpPortExtraDataDestroy (PLPCP_PORT_OBJECT PortObject);

VOID LpcpSaveThread (PLPCP_PORT_OBJECT PortObject);

#else

#define LpcpPortExtraDataCreate(x)

#define LpcpPortExtraDataDestroy(x)

#define LpcpSaveThread(x)

#endif // LPCP_TRACE_SERVER_THREADS


//
//  Internal Entry Points defined in lpcclose.c
//

VOID
LpcpClosePort (
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG ProcessHandleCount,
    IN ULONG SystemHandleCount
    );

VOID
LpcpDeletePort (
    IN PVOID Object
    );


//
//  Entry points defined in lpcqueue.c
//

NTSTATUS
LpcpInitializePortQueue (
    IN PLPCP_PORT_OBJECT Port
    );

VOID
LpcpDestroyPortQueue (
    IN PLPCP_PORT_OBJECT Port,
    IN BOOLEAN CleanupAndDestroy
    );

NTSTATUS
LpcpInitializePortZone (
    IN ULONG MaxEntrySize,
    IN ULONG SegmentSize,
    IN ULONG MaxPoolUsage
    );

NTSTATUS
LpcpExtendPortZone (
    VOID
    );

PLPCP_MESSAGE
FASTCALL
LpcpAllocateFromPortZone (
    ULONG Size
    );

VOID
FASTCALL
LpcpFreeToPortZone (
    IN PLPCP_MESSAGE Msg,
    IN BOOLEAN MutexOwned
    );

VOID
LpcpSaveDataInfoMessage (
    IN PLPCP_PORT_OBJECT Port,
    PLPCP_MESSAGE Msg
    );

VOID
LpcpFreeDataInfoMessage (
    IN PLPCP_PORT_OBJECT Port,
    IN ULONG MessageId,
    IN ULONG CallbackId
    );

PLPCP_MESSAGE
LpcpFindDataInfoMessage (
    IN PLPCP_PORT_OBJECT Port,
    IN ULONG MessageId,
    IN ULONG CallbackId
    );


//
//  Entry points defined in lpcquery.c
//


//
//  Entry points defined in lpcmove.s and lpcmove.asm
//

VOID
LpcpMoveMessage (
    OUT PPORT_MESSAGE DstMsg,
    IN PPORT_MESSAGE SrcMsg,
    IN PVOID SrcMsgData,
    IN ULONG MsgType OPTIONAL,
    IN PCLIENT_ID ClientId OPTIONAL
    );


//
//  Internal Entry Points defined in lpcpriv.c
//

VOID
LpcpFreePortClientSecurity (
    IN PLPCP_PORT_OBJECT Port
    );


//
//  Macro Procedures used by RequestWaitReply, Reply, ReplyWaitReceive,
//  and ReplyWaitReply services
//

#define LpcpGetDynamicClientSecurity(Thread,Port,DynamicSecurity) \
    SeCreateClientSecurity((Thread),&(Port)->SecurityQos,FALSE,(DynamicSecurity))

#define LpcpFreeDynamicClientSecurity(DynamicSecurity) \
    SeDeleteClientSecurity( DynamicSecurity )

#define LpcpReferencePortObject(PortHandle,PortAccess,PreviousMode,PortObject) \
    ObReferenceObjectByHandle((PortHandle),(PortAccess),LpcPortObjectType,(PreviousMode),(PVOID *)(PortObject),NULL)


//
//  Entry Points defined in lpcinit.c
//

char *
LpcpGetCreatorName (
    PLPCP_PORT_OBJECT PortObject
    );

#if DBG
#define ENABLE_LPC_TRACING 1
#else
#define ENABLE_LPC_TRACING 0
#endif

#if ENABLE_LPC_TRACING
BOOLEAN LpcpStopOnReplyMismatch;
BOOLEAN LpcpTraceMessages;

char *LpcpMessageTypeName[];

char *
LpcpGetCreatorName (
    PLPCP_PORT_OBJECT PortObject
    );

#define LpcpPrint( _x_ ) {                              \
    DbgPrint( "LPC[ %02x.%02x ]: ",                     \
              PsGetCurrentThread()->Cid.UniqueProcess,  \
              PsGetCurrentThread()->Cid.UniqueThread ); \
    DbgPrint _x_ ;                                      \
}

#define LpcpTrace( _x_ ) if (LpcpTraceMessages) { LpcpPrint( _x_ ); }

#else

#define LpcpPrint( _x_ )
#define LpcpTrace( _x_ )

#endif // ENABLE_LPC_TRACING
