/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    userver.c

Abstract:

    User Mode Server Test program for the LPC subcomponent of the NTOS project

Author:

    Steve Wood (stevewo) 28-Aug-1989

Revision History:

--*/

#include "ulpc.h"

#define MAX_REQUEST_THREADS 9
#define MAX_CONNECTIONS 4

HANDLE ServerConnectionPortHandle;
HANDLE ServerThreadHandles[ MAX_REQUEST_THREADS ];
DWORD  ServerThreadClientIds[ MAX_REQUEST_THREADS ];

HANDLE ServerClientPortHandles[ MAX_CONNECTIONS ];
ULONG CountServerClientPortHandles = 0;
ULONG CountClosedServerClientPortHandles = 0;

BOOLEAN TestCallBacks;

VOID
ServerHandleConnectionRequest(
    IN PTLPC_PORTMSG Msg
    )
{
    BOOLEAN AcceptConnection;
    LPSTR ConnectionInformation;
    ULONG ConnectionInformationLength;
    NTSTATUS Status;
    PORT_VIEW ServerView;
    REMOTE_PORT_VIEW ClientView;
    ULONG i;
    PULONG p;

    ConnectionInformation = (LPSTR)&Msg->Data[ 0 ];
    ConnectionInformationLength = Msg->h.u1.s1.DataLength;
    AcceptConnection = FALSE;
    fprintf( stderr, "\nConnection Request Received from CLIENT_ID 0x%08lx.0x%08lx:\n",
              Msg->h.ClientId.UniqueProcess,
              Msg->h.ClientId.UniqueThread
            );
    fprintf( stderr, "    MessageId: %ld\n",
              Msg->h.MessageId
            );
    fprintf( stderr, "    ClientViewSize: 0x%08lx\n",
              Msg->h.ClientViewSize
            );
    fprintf( stderr, "    ConnectionInfo: (%ld) '%.*s'\n",
              ConnectionInformationLength,
              ConnectionInformationLength,
              (PSZ)&ConnectionInformation[0]
            );

    ClientView.Length = sizeof( ClientView );
    ClientView.ViewSize = 0;
    ClientView.ViewBase = 0;
    if (CountServerClientPortHandles >= MAX_CONNECTIONS) {
        AcceptConnection = FALSE;
        }
    else {
        AcceptConnection = TRUE;
        }

    if (AcceptConnection) {
        LARGE_INTEGER MaximumSize;

        fprintf( stderr, "Creating Port Memory Section" );
        MaximumSize.QuadPart = 0x4000;
        Status = NtCreateSection( &ServerView.SectionHandle,
                                  SECTION_MAP_READ | SECTION_MAP_WRITE,
                                  NULL,
                                  &MaximumSize,
                                  PAGE_READWRITE,
                                  SEC_COMMIT,
                                  NULL
                                );

        if (ShowHandleOrStatus( Status, ServerView.SectionHandle )) {
            ServerView.Length = sizeof( ServerView );
            ServerView.SectionOffset = 0;
            ServerView.ViewSize = 0x4000;
            ServerView.ViewBase = 0;
            ServerView.ViewRemoteBase = 0;
            }
        else {
            AcceptConnection = FALSE;
            }
        }

    fprintf( stderr, "Server calling NtAcceptConnectPort( AcceptConnection = %ld )",
              AcceptConnection
            );

    if (AcceptConnection) {
        strcpy( ConnectionInformation, "Server Accepting Connection" );
        }
    else {
        strcpy( ConnectionInformation, "Server Rejecting Connection" );
        }

    Msg->h.u1.s1.DataLength = strlen( ConnectionInformation ) + 1;
    Msg->h.u1.s1.TotalLength = Msg->h.u1.s1.DataLength + sizeof( Msg->h );
    Status = NtAcceptConnectPort( &ServerClientPortHandles[ CountServerClientPortHandles ],
                                  (PVOID)(CountServerClientPortHandles+1),
                                  &Msg->h,
                                  AcceptConnection,
                                  &ServerView,
                                  &ClientView
                                );

    if (ShowHandleOrStatus( Status, ServerClientPortHandles[ CountServerClientPortHandles ] )) {
        fprintf( stderr, "    ServerView: Base=%lx, Size=%lx, RemoteBase: %lx\n",
                  ServerView.ViewBase,
                  ServerView.ViewSize,
                  ServerView.ViewRemoteBase
                );
        fprintf( stderr, "    ClientView: Base=%lx, Size=%lx\n",
                  ClientView.ViewBase,
                  ClientView.ViewSize
                );

        ClientMemoryBase = ServerView.ViewBase;
        ClientMemorySize = ServerView.ViewSize;
        ServerMemoryBase = ServerView.ViewRemoteBase;
        ServerMemoryDelta = (ULONG)ServerMemoryBase -
                            (ULONG)ClientMemoryBase;
        p = (PULONG)(ClientView.ViewBase);
        i =ClientView.ViewSize;
        while (i) {
            *p = (ULONG)p;
            fprintf( stderr, "Server setting ClientView[ %lx ] = %lx\n",
                      p,
                      *p
                    );
            p += (0x1000/sizeof(ULONG));
            i -= 0x1000;
            }

        p = (PULONG)(ServerView.ViewBase);
        i = ServerView.ViewSize;
        while (i) {
            *p = (ULONG)p - ServerMemoryDelta;
            fprintf( stderr, "Server setting ServerView[ %lx ] = %lx\n",
                      p,
                      *p
                    );
            p += (0x1000/sizeof(ULONG));
            i -= 0x1000;
            }
        Status = NtCompleteConnectPort( ServerClientPortHandles[ CountServerClientPortHandles ] );
        CountServerClientPortHandles++;
        }

    return;
}

DWORD
ServerThread(
    LPVOID Context
    )
{
    NTSTATUS Status;
    CHAR ThreadName[ 64 ];
    TLPC_PORTMSG Msg;
    PTLPC_PORTMSG ReplyMsg;
    HANDLE ReplyPortHandle;
    ULONG PortContext;
    PTEB Teb = NtCurrentTeb();

    Teb->ActiveRpcHandle = NULL;

    strcpy( ThreadName, "Server Thread Id: " );
    RtlIntegerToChar( (ULONG)Teb->ClientId.UniqueProcess, 16, 9,
                    ThreadName + strlen( ThreadName )
                  );
    strcat( ThreadName, "." );
    RtlIntegerToChar( (ULONG)Teb->ClientId.UniqueThread, 16, 9,
                    ThreadName + strlen( ThreadName )
                  );

    EnterThread( ThreadName, (ULONG)Context );

    ReplyMsg = NULL;
    ReplyPortHandle = ServerConnectionPortHandle;
    while (TRUE) {
        fprintf( stderr, "%s waiting for message...\n", ThreadName );
        Status = NtReplyWaitReceivePort( ReplyPortHandle,
                                         (PVOID)&PortContext,
                                         (PPORT_MESSAGE)ReplyMsg,
                                         (PPORT_MESSAGE)&Msg
                                       );

        ReplyMsg = NULL;
        ReplyPortHandle = ServerConnectionPortHandle;
        fprintf( stderr, "%s Receive (%s)  Id: %u", ThreadName, LpcMsgTypes[ Msg.h.u2.s2.Type ], Msg.h.MessageId );
        PortContext -= 1;
        if (!NT_SUCCESS( Status )) {
            fprintf( stderr, " (Status == %08x)\n", Status );
            }
        else
        if (Msg.h.u2.s2.Type == LPC_CONNECTION_REQUEST) {
            ServerHandleConnectionRequest( &Msg );
            continue;
            }
        else
        if (PortContext >= CountServerClientPortHandles) {
            fprintf( stderr, "*** Invalid PortContext (%lx) received\n",
                     PortContext
                   );
            }
        else
        if (Msg.h.u2.s2.Type == LPC_PORT_CLOSED ||
            Msg.h.u2.s2.Type == LPC_CLIENT_DIED
           ) {
            fprintf( stderr, " - disconnect for client %08x\n", PortContext );
            CloseHandle( ServerClientPortHandles[ (ULONG)PortContext ] );
            CountClosedServerClientPortHandles += 1;
            if (CountClosedServerClientPortHandles == CountServerClientPortHandles) {
                break;
                }
            }
        else
        if (Msg.h.u2.s2.Type == LPC_REQUEST) {
            CheckTlpcMsg( Status, &Msg );
            ReplyMsg = &Msg;
            ReplyPortHandle = ServerClientPortHandles[ PortContext ];
            if (TestCallBacks && (Msg.h.u1.s1.DataLength > 30)) {
                Status = SendRequest( 1,
                                      ThreadName,
                                      ReplyPortHandle,
                                      Context,
                                      Msg.h.u1.s1.DataLength >> 1,
                                      ReplyMsg,
                                      TRUE
                                    );
                }
            }
        }

    fprintf( stderr, "Exiting %s\n", ThreadName );

    return RtlNtStatusToDosError( Status );
}



VOID
Usage( VOID )
{
    fprintf( stderr, "usage: USERVER #threads\n" );
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
    ULONG i, NumberOfThreads;
    OBJECT_ATTRIBUTES ObjectAttributes;

    Status = STATUS_SUCCESS;

    fprintf( stderr, "Entering USERVER User Mode LPC Test Program\n" );

    TestCallBacks = FALSE;
    if (argc < 2) {
        NumberOfThreads = 1;
        }
    else {
        NumberOfThreads = atoi( argv[ 1 ] );
        if (NumberOfThreads >= MAX_REQUEST_THREADS) {
            Usage();
            }

        if (argc > 2) {
            TestCallBacks = TRUE;
            }
        }

    RtlInitUnicodeString( &PortName, PORT_NAME );
    fprintf( stderr, "Creating %wZ connection port", (PUNICODE_STRING)&PortName );
    InitializeObjectAttributes( &ObjectAttributes, &PortName, 0, NULL, NULL );
    Status = NtCreatePort( &ServerConnectionPortHandle,
                           &ObjectAttributes,
                           40,
                           sizeof( TLPC_PORTMSG ),
                           sizeof( TLPC_PORTMSG ) * 32
                         );
    ShowHandleOrStatus( Status, ServerConnectionPortHandle );
    rc = RtlNtStatusToDosError( Status );
    if (rc == NO_ERROR) {
        ServerThreadHandles[ 0 ] = GetCurrentThread();
        ServerThreadClientIds[ 0 ] = GetCurrentThreadId();
        for (i=1; i<NumberOfThreads; i++) {
            fprintf( stderr, "Creating Server Request Thread %ld\n", i+1 );
            rc = NO_ERROR;
            ServerThreadHandles[ i ] = CreateThread( NULL,
                                                     0,
                                                     (LPTHREAD_START_ROUTINE)ServerThread,
                                                     (LPVOID)(i+1),
                                                     CREATE_SUSPENDED,
                                                     &ServerThreadClientIds[ i ]
                                                   );
            if (ServerThreadHandles[ i ] == NULL) {
                rc = GetLastError();
                break;
                }
            }

        if (rc == NO_ERROR) {
            for (i=1; i<NumberOfThreads; i++) {
                ResumeThread( ServerThreadHandles[ i ] );
                }

            ServerThread( 0 );
            }
        }

    if (rc != NO_ERROR) {
        fprintf( stderr, "USERVER: Initialization Failed - %u\n", rc );
        }

    ExitProcess( rc );
    return( rc );
}
