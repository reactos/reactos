/* $Id: $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             subsys/system/dhcp/pipe.c
 * PURPOSE:          DHCP client pipe
 * PROGRAMMER:       arty
 */

#include <rosdhcp.h>

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
    BOOL Result;
    HANDLE Connection;

    while( (Connection = ConnectNamedPipe( CommPipe, NULL )) ) {
        Result = ReadFile( Connection, &Req, sizeof(Req), &BytesRead, NULL );
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
            }
        }
        CloseHandle( Connection );
    }
}

HANDLE PipeInit() {
    CommPipe = CreateNamedPipe
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
