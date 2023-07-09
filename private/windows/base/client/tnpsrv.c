/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    tnpsrv.c

Abstract:

    This program creates a single instance of the pipe \cw\testpipe,
    awaits for a connection. While a client wants to talk it will echo
    data back to the client. When the client closes the pipe tnpsrv will
    wait for another client.

Author:

    Colin Watson (ColinW) 19-March-1991

Revision History:

--*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

int
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{

    HANDLE S1;
    DWORD Size;
    DWORD Dummy;
    CHAR Data[1024];

    S1 = CreateNamedPipe("\\\\.\\Pipe\\cw\\testpipe",
            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
            PIPE_WAIT | PIPE_READMODE_MESSAGE| PIPE_TYPE_MESSAGE,
            1,  // One instance only
            sizeof(Data),
            sizeof(Data),
            0,
            NULL);

    assert(S1 != INVALID_HANDLE_VALUE);

    while (1) {

        printf("Waiting for connection\n");
        if ( FALSE == ConnectNamedPipe( S1, NULL )) {
            printf("Server ReadFile returned Error %lx\n", GetLastError() );
            break;
            }

        while (1) {

            printf("Server now Reading\n");
            if ( FALSE == ReadFile(S1,Data, sizeof(Data), &Size, NULL) ) {
                printf("Server ReadFile returned Error %lx\n", GetLastError() );
                break;
                }

            printf("Server Reading Done %s\n",Data);

            printf("Server Writing\n");
            if ( FALSE == WriteFile(S1, Data, Size, &Dummy, NULL) ) {
                printf("Server WriteFile returned Error %lx\n", GetLastError() );
                break;
                }

            printf("Server Writing Done\n");
            }

        if ( FALSE == DisconnectNamedPipe( S1 ) ) {
            printf("Server WriteFile returned Error %lx\n", GetLastError() );
            break;
            }
        }

    CloseHandle(S1);

}
