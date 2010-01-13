/* $Id: $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             subsys/system/dhcp/pipe.c
 * PURPOSE:          DHCP client pipe
 * PROGRAMMER:       arty
 */

#include <rosdhcp.h>

#define NDEBUG
#include <reactos/debug.h>

static HANDLE CommPipe = INVALID_HANDLE_VALUE, CommThread;
DWORD CommThrId;

#define COMM_PIPE_OUTPUT_BUFFER sizeof(COMM_DHCP_REQ)
#define COMM_PIPE_INPUT_BUFFER sizeof(COMM_DHCP_REPLY)
#define COMM_PIPE_DEFAULT_TIMEOUT 1000

DWORD PipeSend( COMM_DHCP_REPLY *Reply ) {
    DWORD Written = 0;
    BOOL Success =
        WriteFile( CommPipe,
                   Reply,
                   sizeof(*Reply),
                   &Written,
                   NULL );
    return Success ? Written : -1;
}

DWORD WINAPI PipeThreadProc( LPVOID Parameter ) {
    DWORD BytesRead, BytesWritten;
    COMM_DHCP_REQ Req;
    COMM_DHCP_REPLY Reply;
    BOOL Result, Connected;

    while( TRUE ) {
        Connected = ConnectNamedPipe( CommPipe, NULL ) ?
            TRUE : GetLastError() == ERROR_PIPE_CONNECTED;

        if (!Connected) {
            DbgPrint("DHCP: Could not connect named pipe\n");
            CloseHandle( CommPipe );
            CommPipe = INVALID_HANDLE_VALUE;
            break;
        }

        Result = ReadFile( CommPipe, &Req, sizeof(Req), &BytesRead, NULL );
        if( Result ) {
            switch( Req.Type ) {
            case DhcpReqQueryHWInfo:
                BytesWritten = DSQueryHWInfo( PipeSend, &Req );
                break;

            case DhcpReqLeaseIpAddress:
                BytesWritten = DSLeaseIpAddress( PipeSend, &Req );
                break;

            case DhcpReqReleaseIpAddress:
                BytesWritten = DSReleaseIpAddressLease( PipeSend, &Req );
                break;

            case DhcpReqRenewIpAddress:
                BytesWritten = DSRenewIpAddressLease( PipeSend, &Req );
                break;

            case DhcpReqStaticRefreshParams:
                BytesWritten = DSStaticRefreshParams( PipeSend, &Req );
                break;

            case DhcpReqGetAdapterInfo:
                BytesWritten = DSGetAdapterInfo( PipeSend, &Req );
                break;

            default:
                DPRINT1("Unrecognized request type %d\n", Req.Type);
                ZeroMemory( &Reply, sizeof( COMM_DHCP_REPLY ) );
                Reply.Reply = 0;
                BytesWritten = PipeSend( &Reply );
                break;
            }
        }
        DisconnectNamedPipe( CommPipe );
    }

    return TRUE;
}

HANDLE PipeInit() {
    CommPipe = CreateNamedPipeW
        ( DHCP_PIPE_NAME,
          PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE,
          PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
          1,
          COMM_PIPE_OUTPUT_BUFFER,
          COMM_PIPE_INPUT_BUFFER,
          COMM_PIPE_DEFAULT_TIMEOUT,
          NULL );

    if( CommPipe == INVALID_HANDLE_VALUE ) {
        DbgPrint("DHCP: Could not create named pipe\n");
        return CommPipe;
    }

    CommThread = CreateThread( NULL, 0, PipeThreadProc, NULL, 0, &CommThrId );

    if( !CommThread ) {
        CloseHandle( CommPipe );
        CommPipe = INVALID_HANDLE_VALUE;
    }

    return CommPipe;
}

VOID PipeDestroy() {
    CloseHandle( CommPipe );
    CommPipe = INVALID_HANDLE_VALUE;
}
