/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ulpc.h

Abstract:

    User Mode Test header file for common definitions shared by userver.c
    and uclient.c

Author:

    Steve Wood (stevewo) 28-Aug-1989

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PORT_NAME L"\\RPC Control\\LpcTestPort"

UNICODE_STRING PortName;

char * LpcMsgTypes[] = {
    "** INVALID **",
    "LPC_REQUEST",
    "LPC_REPLY",
    "LPC_DATAGRAM",
    "LPC_LOST_REPLY",
    "LPC_PORT_CLOSED",
    "LPC_CLIENT_DIED",
    "LPC_EXCEPTION",
    "LPC_DEBUG_EVENT",
    "LPC_ERROR_EVENT",
    "LPC_CONNECTION_REQUEST",
    NULL
};

SECURITY_QUALITY_OF_SERVICE DynamicQos = {
    SecurityImpersonation,
    SECURITY_DYNAMIC_TRACKING,
    TRUE
    };

#define TLPC_MAX_MSG_DATA_LENGTH 16

typedef struct _TLPC_PORTMSG {
    PORT_MESSAGE h;
    ULONG Data[ TLPC_MAX_MSG_DATA_LENGTH ];
} TLPC_PORTMSG, *PTLPC_PORTMSG;

PCH   ClientMemoryBase = 0;
ULONG ClientMemorySize = 0;
PCH   ServerMemoryBase = 0;
ULONG ServerMemoryDelta = 0;

typedef struct _PAGE {
    CHAR Data[ 4096 ];
} PAGE, *PPAGE;

PPORT_MESSAGE
InitTlpcMsg(
    PTLPC_PORTMSG Msg,
    PVOID Context,
    ULONG MsgLength
    )
{
    ULONG i;
    ULONG ClientIndex;
    ULONG cbData = MsgLength % (TLPC_MAX_MSG_DATA_LENGTH * sizeof( ULONG ));
    PULONG ClientMemoryPtr;

    Msg->h.u1.Length = ((sizeof( Msg->h ) + cbData) << 16) | cbData;
    Msg->h.u2.ZeroInit = 0;
    ClientIndex = (ULONG)Context & 0xF;
    ClientIndex -= 1;
    if (cbData) {
        Msg->Data[ 0 ] = (ULONG)Context;
        ClientMemoryPtr = (PULONG)(ClientMemoryBase + (ClientIndex * 0x1000));
        for (i=1; i<(cbData/sizeof(ULONG)); i++) {
            *ClientMemoryPtr = (ULONG)Context;
            Msg->Data[ i ] = (ULONG)ClientMemoryPtr + ServerMemoryDelta;
            ClientMemoryPtr++;
            }
        }

    return( (PPORT_MESSAGE)Msg );
}

BOOLEAN
CheckTlpcMsg(
    NTSTATUS Status,
    PTLPC_PORTMSG Msg
    )
{
    ULONG i;
    ULONG ClientIndex;
    ULONG cbData = Msg->h.u1.s1.DataLength;
    ULONG Context;
    PULONG ServerMemoryPtr;
    PULONG ClientMemoryPtr;
    ULONG ExpectedContext;
    BOOLEAN Result;

    if (!NT_SUCCESS( Status )) {
        fprintf( stderr, " - FAILED.  Status == %X\n", Status );
        return( FALSE );
        }

    if (Msg->h.u2.s2.Type == LPC_CONNECTION_REQUEST) {
        fprintf( stderr, " connection request" );
        }
    else
    if (cbData) {
        Context = Msg->Data[ 0 ];
        ClientIndex = Context & 0xF;
        ClientIndex -= 1;
        ClientMemoryPtr = (PULONG)(ClientMemoryBase + (ClientIndex * 0x1000));
        for (i=1; i<(cbData/sizeof( ULONG )); i++) {
            if (Msg->h.u2.s2.Type == LPC_REPLY) {
                if (Msg->Data[ i ] != ((ULONG)ClientMemoryPtr + ServerMemoryDelta) ||
                    *ClientMemoryPtr != (ULONG)Context
                   ) {
                    fprintf( stderr, " incorrectly\n" );
                    fprintf( stderr, "    Msg->Data[ %ld ] == %lx != %lx || %lx -> %lx != %lx\n",
                             i, Msg->Data[ i ], (ULONG)ClientMemoryPtr + ServerMemoryDelta,
                             ClientMemoryPtr, *ClientMemoryPtr, Context
                           );
                    return( FALSE );
                    }

                ClientMemoryPtr++;
                }
            else {
                ServerMemoryPtr = (PULONG)(Msg->Data[ i ]);
                try {
                    ExpectedContext = *ServerMemoryPtr;
                    Result = (ExpectedContext != Context) ? FALSE : TRUE;
                    }
                except( EXCEPTION_EXECUTE_HANDLER ) {
                    ExpectedContext = 0xFEFEFEFE;
                    Result = FALSE;
                    }

                if (!Result) {
                    fprintf( stderr, " incorrectly\n" );
                    fprintf( stderr, "    Msg->Data[ %ld ] == %lx -> %lx != %lx\n",
                             i, Msg->Data[ i ], ExpectedContext, Context
                           );
                    return( FALSE );
                }
                }
            }
        }

    fprintf( stderr, " correctly\n" );
    return( TRUE );
}


BOOLEAN
ShowHandleOrStatus(
    NTSTATUS Status,
    HANDLE Handle
    )
{
    if (NT_SUCCESS( Status )) {
        fprintf( stderr, " - Handle = 0x%lx\n", Handle );
        return( TRUE );
        }
    else {
        fprintf( stderr, " - *** FAILED *** Status == %X\n", Status );
        return( FALSE );
        }
}


BOOLEAN
ShowStatus(
    NTSTATUS Status
    )
{
    if (NT_SUCCESS( Status )) {
        fprintf( stderr, " - success\n" );
        return( TRUE );
        }
    else {
        fprintf( stderr, " - *** FAILED *** Status == %X\n", Status );
        return( FALSE );
        }
}

PCH EnterString = ">>>>>>>>>>";
PCH InnerString = "||||||||||";
PCH LeaveString = "<<<<<<<<<<";

NTSTATUS
SendRequest(
    ULONG Level,
    PSZ ThreadName,
    HANDLE PortHandle,
    PVOID Context,
    ULONG MsgLength,
    PTLPC_PORTMSG CallBackTarget,
    BOOLEAN ServerCallingClient
    )
{
    NTSTATUS Status;
    TLPC_PORTMSG Request, Reply;
    PTEB Teb = NtCurrentTeb();

    fprintf( stderr, "%.*sEnter SendRequest, %lx.%lx",
             Level, EnterString,
             Teb->ClientId.UniqueProcess,
             Teb->ClientId.UniqueThread
           );

    InitTlpcMsg( &Request, Context, MsgLength );
    if (CallBackTarget == NULL) {
        fprintf( stderr, " - Request");
        }
    else {
        Request.h.u2.s2.Type = LPC_REQUEST;
        Request.h.ClientId = CallBackTarget->h.ClientId;
        Request.h.MessageId = CallBackTarget->h.MessageId;
        fprintf( stderr, " - Callback to %lx.%lx, ID: %ld",
                 Request.h.ClientId.UniqueProcess,
                 Request.h.ClientId.UniqueThread,
                 Request.h.MessageId
               );
        }

    fprintf( stderr, " (%ld bytes)...\n", Request.h.u1.s1.DataLength );
    Status = NtRequestWaitReplyPort( PortHandle,
                                     (PPORT_MESSAGE)&Request,
                                     (PPORT_MESSAGE)&Reply
                                   );
    fprintf( stderr, "%.*s %lx.%lx, ID: %u received ",
             Level, InnerString,
             Teb->ClientId.UniqueProcess,
             Teb->ClientId.UniqueThread,
             Reply.h.MessageId
           );

    if (Reply.h.u2.s2.Type == LPC_REPLY) {
        if (!CheckTlpcMsg( Status, &Reply )) {
            Status = STATUS_UNSUCCESSFUL;
            fprintf( stderr, "SendRequest got invalid reply message at %x\n", &Reply );
            DbgBreakPoint();
            }
        }
    else {
        fprintf( stderr, "callback from %lx.%lx, ID: %ld",
                 Reply.h.ClientId.UniqueProcess,
                 Reply.h.ClientId.UniqueThread,
                 Reply.h.MessageId
               );
        if (!CheckTlpcMsg( Status, &Reply )) {
            Status = STATUS_UNSUCCESSFUL;
            fprintf( stderr, "SendRequest got invalid callback message at %x\n", &Reply );
            DbgBreakPoint();
            }
        else {
            MsgLength = Reply.h.u1.s1.DataLength / 2;
            if (MsgLength) {
                Status = SendRequest( Level+1,
                                      ThreadName,
                                      PortHandle,
                                      Context,
                                      MsgLength,
                                      &Reply,
                                      ServerCallingClient
                                    );
                }

            if (!ServerCallingClient || Level > 1) {
                fprintf( stderr, "%.*s %lx.%lx sending ",
                         Level, InnerString,
                         Teb->ClientId.UniqueProcess,
                         Teb->ClientId.UniqueThread
                       );
                fprintf( stderr, " callback (%u) reply to %lx.%lx, ID: %u (%ld bytes)...\n",
                         Level,
                         Reply.h.ClientId.UniqueProcess,
                         Reply.h.ClientId.UniqueThread,
                         Reply.h.MessageId,
                         Reply.h.u1.s1.DataLength
                       );
                if (Level > 1) {
                    Status = NtReplyWaitReplyPort( PortHandle,
                                                   (PPORT_MESSAGE)&Reply
                                                 );
                    }
                }
            }
        }

    fprintf( stderr, "%.*sLeave SendRequest, %lx.%lx - Status == %X\n",
             Level, LeaveString,
             Teb->ClientId.UniqueProcess,
             Teb->ClientId.UniqueThread,
             Status
           );
    return( Status );
}

VOID
EnterThread(
    PSZ ThreadName,
    ULONG Context
    )
{
    fprintf( stderr, "Entering %s thread, Context = 0x%lx\n",
             ThreadName,
             Context
           );
}
