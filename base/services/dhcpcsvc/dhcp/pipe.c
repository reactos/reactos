/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             subsys/system/dhcp/pipe.c
 * PURPOSE:          DHCP client pipe
 * PROGRAMMER:       arty
 */

#include <rosdhcp.h>

#define NDEBUG
#include <reactos/debug.h>

#define COMM_PIPE_OUTPUT_BUFFER sizeof(COMM_DHCP_REQ)
#define COMM_PIPE_INPUT_BUFFER sizeof(COMM_DHCP_REPLY)
#define COMM_PIPE_DEFAULT_TIMEOUT 1000

DWORD PipeSend( HANDLE CommPipe, COMM_DHCP_REPLY *Reply ) {
    DWORD Written = 0;
    OVERLAPPED Overlapped = {0};
    BOOL Success =
        WriteFile( CommPipe,
                   Reply,
                   sizeof(*Reply),
                   &Written,
                   &Overlapped);
    if (!Success)
    {
        WaitForSingleObject(CommPipe, INFINITE);
        Success = GetOverlappedResult(CommPipe,
                                      &Overlapped,
                                      &Written,
                                      TRUE);
    }

    return Success ? Written : -1;
}

DWORD WINAPI PipeThreadProc( LPVOID Parameter ) {
    DWORD BytesRead;
    COMM_DHCP_REQ Req;
    COMM_DHCP_REPLY Reply;
    BOOL Result, Connected;
    HANDLE Events[2];
    HANDLE CommPipe;
    OVERLAPPED Overlapped = {0};
    DWORD dwError;

    DPRINT("PipeThreadProc(%p)\n", Parameter);

    CommPipe = CreateNamedPipeW
        ( DHCP_PIPE_NAME,
          PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED,
          PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
          1,
          COMM_PIPE_OUTPUT_BUFFER,
          COMM_PIPE_INPUT_BUFFER,
          COMM_PIPE_DEFAULT_TIMEOUT,
          NULL );
    if (CommPipe == INVALID_HANDLE_VALUE)
    {
        DbgPrint("DHCP: Could not create named pipe\n");
        return FALSE;
    }

    Events[0] = (HANDLE)Parameter;
    Events[1] = CommPipe;

    while( TRUE )
    {
        Connected = ConnectNamedPipe(CommPipe, &Overlapped);
        if (!Connected)
        {
            dwError = GetLastError();
            if (dwError == ERROR_IO_PENDING)
            {
                dwError = WaitForMultipleObjects(2, Events, FALSE, INFINITE);
                DPRINT("WaitForMultipleObjects() returned %lu\n", dwError);
                if (dwError == WAIT_OBJECT_0 + 1)
                {
                    Connected = GetOverlappedResult(CommPipe,
                                                    &Overlapped,
                                                    &BytesRead,
                                                    TRUE);
                }
                else if (dwError == WAIT_OBJECT_0)
                {
                    CancelIo(CommPipe);
                    CloseHandle(CommPipe);
                    CommPipe = INVALID_HANDLE_VALUE;
                    break;
                }
            }
        }

        if (!Connected) {
            DbgPrint("DHCP: Could not connect named pipe\n");
            CloseHandle( CommPipe );
            CommPipe = INVALID_HANDLE_VALUE;
            break;
        }

        Result = ReadFile(CommPipe, &Req, sizeof(Req), &BytesRead, &Overlapped);
        if (!Result)
        {
            dwError = GetLastError();
            if (dwError == ERROR_IO_PENDING)
            {
                dwError = WaitForMultipleObjects(2, Events, FALSE, INFINITE);
                DPRINT("WaitForMultipleObjects() returned %lu\n", dwError);
                if (dwError == WAIT_OBJECT_0 + 1)
                {
                    Result = GetOverlappedResult(CommPipe,
                                                 &Overlapped,
                                                 &BytesRead,
                                                 TRUE);
                }
                else if (dwError == WAIT_OBJECT_0)
                {
                    CancelIo(CommPipe);
                    DisconnectNamedPipe( CommPipe );
                    CloseHandle(CommPipe);
                    CommPipe = INVALID_HANDLE_VALUE;
                    break;
                }
            }
        }

        if( Result ) {
            switch( Req.Type ) {
            case DhcpReqQueryHWInfo:
                DSQueryHWInfo( PipeSend, CommPipe, &Req );
                break;

            case DhcpReqLeaseIpAddress:
                DSLeaseIpAddress( PipeSend, CommPipe, &Req );
                break;

            case DhcpReqReleaseIpAddress:
                DSReleaseIpAddressLease( PipeSend, CommPipe, &Req );
                break;

            case DhcpReqRenewIpAddress:
                DSRenewIpAddressLease( PipeSend, CommPipe, &Req );
                break;

            case DhcpReqStaticRefreshParams:
                DSStaticRefreshParams( PipeSend, CommPipe, &Req );
                break;

            case DhcpReqGetAdapterInfo:
                DSGetAdapterInfo( PipeSend, CommPipe, &Req );
                break;

            default:
                DPRINT1("Unrecognized request type %d\n", Req.Type);
                ZeroMemory( &Reply, sizeof( COMM_DHCP_REPLY ) );
                Reply.Reply = 0;
                PipeSend(CommPipe, &Reply );
                break;
            }
        }
        DisconnectNamedPipe( CommPipe );
    }

    DPRINT("Pipe thread stopped!\n");

    return TRUE;
}

HANDLE PipeInit(HANDLE hStopEvent)
{
    return CreateThread( NULL, 0, PipeThreadProc, (LPVOID)hStopEvent, 0, NULL);
}
