/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    uclient.c

Abstract:

    User Mode Client Test program for the LPC subcomponent of the NTOS project

Author:

    Steve Wood (stevewo) 28-Aug-1989

Revision History:

--*/

#include "ulpc.h"

#define MAX_CLIENT_PROCESSES 9
#define MAX_CLIENT_THREADS 9

CHAR ProcessName[ 32 ];
HANDLE PortHandle = NULL;

HANDLE ClientThreadHandles[ MAX_CLIENT_THREADS ] = {NULL};
DWORD  ClientThreadClientIds[ MAX_CLIENT_THREADS ];

DWORD
ClientThread(
    LPVOID Context
    )
{
    CHAR ThreadName[ 64 ];
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG MsgLength;
    PTEB Teb = NtCurrentTeb();

    Teb->ActiveRpcHandle = NULL;

    strcpy( ThreadName, "Client Thread Id: " );
    RtlIntegerToChar( (ULONG)Teb->ClientId.UniqueProcess, 16, 9,
                    ThreadName + strlen( ThreadName )
                  );
    strcat( ThreadName, "." );
    RtlIntegerToChar( (ULONG)Teb->ClientId.UniqueThread,  16, 9,
                    ThreadName + strlen( ThreadName )
                  );

    EnterThread( ThreadName, (ULONG)Context );

    MsgLength = 0;
    while (NT_SUCCESS( Status )) {
        Status = SendRequest( 1,
                              ThreadName,
                              PortHandle,
                              Context,
                              MsgLength,
                              NULL,
                              FALSE
                            );
        MsgLength += sizeof( ULONG );
        if (MsgLength >= (TLPC_MAX_MSG_DATA_LENGTH * sizeof( ULONG ))) {
            break;
            }
        }

    if (PortHandle != NULL) {
        if (!CloseHandle( PortHandle )) {
            fprintf( stderr, "CloseHandle( 0x%lx ) failed - %u\n", PortHandle, GetLastError() );
            }
        else {
            PortHandle = NULL;
            }
        }

    fprintf( stderr, "%s %s\n",
              NT_SUCCESS( Status ) ? "Exiting" : "ABORTING",
              ThreadName
            );

    return RtlNtStatusToDosError( Status );
}


VOID
Usage( VOID )
{
    fprintf( stderr, "usage: UCLIENT ClientNumber [#threads]\n" );
    ExitProcess( 1 );
}


int
_cdecl
main(
    int argc,
    char *argv[]
    )
{
    NTSTATUS Status;
    DWORD rc;
    PORT_VIEW ClientView;
    REMOTE_PORT_VIEW ServerView;
    ULONG ClientNumber;
    ULONG NumberOfThreads;
    ULONG MaxMessageLength;
    ULONG ConnectionInformationLength;
    UCHAR ConnectionInformation[ 64 ];
    ULONG i;
    PULONG p;
    PTEB Teb = NtCurrentTeb();
    LARGE_INTEGER MaximumSize;

    Status = STATUS_SUCCESS;

    fprintf( stderr, "Entering UCLIENT User Mode LPC Test Program\n" );

    if (argc < 2) {
        Usage();
        }

    ClientNumber = atoi( argv[ 1 ] );
    if (argc < 3) {
        NumberOfThreads = 1;
        }
    else {
        NumberOfThreads = atoi( argv[ 2 ] );
        }

    if (ClientNumber > MAX_CLIENT_PROCESSES ||
        NumberOfThreads > MAX_CLIENT_THREADS
       ) {
        Usage();
        }

    sprintf( ProcessName, "Client Process %08x", Teb->ClientId.UniqueProcess );
    strcpy( ConnectionInformation, ProcessName );
    ConnectionInformationLength = strlen( ProcessName ) + 1;

    RtlInitUnicodeString( &PortName, PORT_NAME );
    fprintf( stderr, "Creating Port Memory Section" );

    MaximumSize.QuadPart = 0x4000 * NumberOfThreads;
    Status = NtCreateSection( &ClientView.SectionHandle,
                              SECTION_MAP_READ | SECTION_MAP_WRITE,
                              NULL,
                              &MaximumSize,
                              PAGE_READWRITE,
                              SEC_COMMIT,
                              NULL
                            );

    if (ShowHandleOrStatus( Status, ClientView.SectionHandle )) {
        ClientView.Length = sizeof( ClientView );
        ClientView.SectionOffset = 0;
        ClientView.ViewSize = 0x2000;
        ClientView.ViewBase = 0;
        ClientView.ViewRemoteBase = 0;
        ServerView.Length = sizeof( ServerView );
        ServerView.ViewSize = 0;
        ServerView.ViewBase = 0;

        fprintf( stderr, "%s calling NtConnectPort( %wZ )\n", ProcessName, &PortName );
        Status = NtConnectPort( &PortHandle,
                                &PortName,
                                &DynamicQos,
                                &ClientView,
                                &ServerView,
                                (PULONG)&MaxMessageLength,
                                (PVOID)ConnectionInformation,
                                (PULONG)&ConnectionInformationLength
                              );



        if (ShowHandleOrStatus( Status, PortHandle )) {
            fprintf( stderr, "    MaxMessageLength: %ld\n", MaxMessageLength );
            fprintf( stderr, "    ConnectionInfo: (%ld) '%.*s'\n",
                      ConnectionInformationLength,
                      ConnectionInformationLength,
                      (PSZ)&ConnectionInformation[0]
                    );
            fprintf( stderr, "    ClientView: Base=%lx, Size=%lx, RemoteBase: %lx\n",
                      ClientView.ViewBase,
                      ClientView.ViewSize,
                      ClientView.ViewRemoteBase
                    );
            fprintf( stderr, "    ServerView: Base=%lx, Size=%lx\n",
                      ServerView.ViewBase,
                      ServerView.ViewSize
                    );
            ClientMemoryBase = ClientView.ViewBase;
            ClientMemorySize = ClientView.ViewSize;
            ServerMemoryBase = ClientView.ViewRemoteBase;
            ServerMemoryDelta = (ULONG)ServerMemoryBase -
                                (ULONG)ClientMemoryBase;

            p = (PULONG)ClientMemoryBase;
            i = ClientMemorySize;
            while (i) {
                fprintf( stderr, "ClientView[%lx] == %lx (%lx)\n",
                          p,
                          *p,
                          *p - ServerMemoryDelta
                        );
                p += (0x1000/sizeof( ULONG ));
                i -= 0x1000;
                }
            p = (PULONG)ServerView.ViewBase;
            i = ServerView.ViewSize;
            while (i) {
                fprintf( stderr, "ServerView[%lx] == %lx\n", p, *p );
                p += (0x1000/sizeof( ULONG ));
                i -= 0x1000;
                }
            }
        }

    rc = RtlNtStatusToDosError( Status );
    if (rc == NO_ERROR) {
        ClientThreadHandles[ 0 ] = GetCurrentThread();
        ClientThreadClientIds[ 0 ] = GetCurrentThreadId();
        for (i=1; i< NumberOfThreads; i++) {
            fprintf( stderr, "Creating %s, Thread %ld\n", ProcessName, i+1 );
            rc = NO_ERROR;
            ClientThreadHandles[ i ] = CreateThread( NULL,
                                                     0,
                                                     (LPTHREAD_START_ROUTINE)ClientThread,
                                                     (LPVOID)((ClientNumber << 4) | (i+1)),
                                                     CREATE_SUSPENDED,
                                                     &ClientThreadClientIds[ i ]
                                                   );
            if (ClientThreadHandles[ i ] == NULL) {
                rc = GetLastError();
                break;
                }
            }

        if (rc == NO_ERROR) {
            for (i=1; i<NumberOfThreads; i++) {
                ResumeThread( ClientThreadHandles[ i ] );
                }

            ClientThread( (LPVOID)((ClientNumber << 4) | 1) );
            }
        }

    if (rc == NO_ERROR) {
        }
    else {
        fprintf( stderr, "UCLIENT: Initialization Failed - %u\n", rc );
        ExitProcess( rc );
        }

    return( rc );
}
