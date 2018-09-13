#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

DWORD Size;
LPSTR WriteData = "Hello World\n";

VOID
ClientThread(
    LPVOID ThreadParameter
    )
{
    DWORD n;
    HANDLE C1;
    LPSTR l;

    printf("Create client...\n");
    C1 = CreateFile("\\\\.\\Pipe\\cw\\testpipe",
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,       // Security Attributes
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
            NULL
            );

    if ( C1 == INVALID_HANDLE_VALUE ) {
        DWORD Status;
        printf("CreateFile returned Error %lx\n", Status = GetLastError() );
        }

    printf("Client writing...\n");

    assert( TRUE == WriteFile(C1,WriteData,Size, &n, NULL) );
    assert( n==Size );
    printf("Client done Writing...\n");

    // Get WriteData back from the server
    l = LocalAlloc(LMEM_ZEROINIT,Size);
    assert(l);
    printf("Client reading\n");

    assert( TRUE == ReadFile(C1,l,Size, &n, NULL));

    printf("Client reading Done %s\n",l);
    Sleep(10000);
    printf("Client done Sleeping...\n");
}


int
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    DWORD n;
    LPSTR l;
    HANDLE Thread;
    HANDLE S1,S2,S3;    //  Server handles
    DWORD ThreadId;
    DWORD Dummy;
    OVERLAPPED S1Overlapped;

    S1 = CreateNamedPipe("\\\\.\\Pipe\\cw\\testpipe",
            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
            PIPE_WAIT | PIPE_READMODE_MESSAGE| PIPE_TYPE_MESSAGE,
            2,
            1024,
            1024,
            0,
            NULL);

    assert(S1 != INVALID_HANDLE_VALUE);

    S2 = CreateNamedPipe("\\\\.\\Pipe\\cw\\testpipe",
            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
            PIPE_WAIT | PIPE_READMODE_MESSAGE| PIPE_TYPE_MESSAGE,
            992, //IGNORED
            0,   //IGNORED
            0,   //IGNORED
            0,   //IGNORED
            NULL);

    assert(S2 != INVALID_HANDLE_VALUE);

    S3 = CreateNamedPipe("\\\\.\\Pipe\\cw\\testpipe",
            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
            PIPE_WAIT | PIPE_READMODE_MESSAGE| PIPE_TYPE_MESSAGE,
            992,
            0,
            0,
            0,
            NULL);

    assert( S3 == INVALID_HANDLE_VALUE );   //Should fail - Used all instances

    S1Overlapped.hEvent = NULL;
    assert( FALSE == ConnectNamedPipe( S1, &S1Overlapped ));
    assert( ERROR_IO_PENDING == GetLastError());

    assert( FALSE == GetOverlappedResult( S1, &S1Overlapped, &Dummy, FALSE ));
    assert( ERROR_IO_INCOMPLETE == GetLastError());

    Size = strlen(WriteData)+1;
    l = LocalAlloc(LMEM_ZEROINIT,Size);
    assert(l);

    Thread = CreateThread(NULL,0L,ClientThread,(LPVOID)99,0,&ThreadId);
    assert(Thread);

    printf("Waiting for connection\n");
    assert( TRUE == GetOverlappedResult( S1, &S1Overlapped, &Dummy, TRUE ));

    printf("Connected, Server now Reading\n");
    if ( FALSE == ReadFile(S1,l,Size, &n, &S1Overlapped) ) {
        DWORD Status;
        printf("Server ReadFile returned Error %lx\n", Status = GetLastError() );

        if ( Status == ERROR_IO_PENDING ) {
            printf("Server got ERROR_IO_PENDING ok,Waiting for data\n");
            assert( TRUE == GetOverlappedResult( S1, &S1Overlapped, &Dummy, TRUE ));
            }

        }
    printf("Server Reading Done %s\n",l);

    printf("Server Writing\n");
    if ( FALSE == WriteFile(S1,l,Size, &n, &S1Overlapped) ) {
        DWORD Status;
        printf("Server WriteFile returned Error %lx\n", Status = GetLastError() );

        if ( Status == ERROR_IO_PENDING ) {
            printf("Server got ERROR_IO_PENDING ok,Waiting for transfer complete\n");
            assert( TRUE == GetOverlappedResult( S1, &S1Overlapped, &Dummy, TRUE ));
            }

        }
    printf("Server Writing Done %s\n",l);
}
