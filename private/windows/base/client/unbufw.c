#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

//
// Unbuffered write test
//

#define TEST_REPEAT_COUNT   10

//
// File is 20mb
//

#define BUFFER_SIZE         (64*1024)
#define NUMBER_OF_WRITES    (320*1)

int BufferSize;
int NumberOfWrites;

char *Buffer;

BOOL fSequentialNew;
BOOL fBufferdIo;
BOOL fRaw;
BOOL fWrite = TRUE;
LPSTR FileName;

VOID
ParseSwitch(
    CHAR chSwitch,
    int *pArgc,
    char **pArgv[]
    );


VOID
ShowUsage(
    VOID
    );

int
__cdecl
main(
    int argc,
    char *argv[],
    char *envp
    )
{
    DWORD Start[TEST_REPEAT_COUNT];
    DWORD End[TEST_REPEAT_COUNT];
    DWORD Diff;
    double fDiff, fSec, fKb, fSumKbs;
    int TestNumber;
    HANDLE hFile, hPort;
    OVERLAPPED ov;
    LPOVERLAPPED ov2;
    int WriteCount;
    DWORD n,n2,key;
    BOOL b;
    int PendingIoCount;
    DWORD Version;
    DWORD FileFlags;
    char chChar, *pchChar;

    Version = GetVersion() >> 16;

    FileName = "unbufw.dat";
    fSequentialNew = FALSE;
    fBufferdIo = FALSE;
    fRaw = FALSE;

    BufferSize = BUFFER_SIZE;
    NumberOfWrites = ( (20*(1024*1024)) / BufferSize);

    while (--argc) {
        pchChar = *++argv;
        if (*pchChar == '/' || *pchChar == '-') {
            while (chChar = *++pchChar) {
                ParseSwitch( chChar, &argc, &argv );
                }
            }
        }




    if ( argc > 1 ) {
        FileName = argv[1];
        }
    else {
        }

    fSumKbs = 0.0;

    Buffer = VirtualAlloc(NULL,BUFFER_SIZE,MEM_COMMIT,PAGE_READWRITE);
    if ( !Buffer ) {
        printf("Error allocating buffer %d\n",GetLastError());
        return 99;
        }

    FileFlags = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED;

    if ( !fBufferdIo ) {
        FileFlags |= FILE_FLAG_NO_BUFFERING;
        }

    DeleteFile(FileName);

    for (TestNumber = 0; TestNumber < TEST_REPEAT_COUNT; TestNumber++ ) {


        hFile = CreateFile(
                    FileName,
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    fRaw ? OPEN_EXISTING : OPEN_ALWAYS,
                    FileFlags,
                    NULL
                    );
        if ( hFile == INVALID_HANDLE_VALUE ) {
            printf("Error opening file %s %d\n",FileName,GetLastError());
            return 99;
            }

        hPort = CreateIoCompletionPort(
                    hFile,
                    NULL,
                    (DWORD)hFile,
                    0
                    );
        if ( !hPort ) {
            printf("Error creating completion port %d\n",FileName,GetLastError());
            return 99;
            }

        ZeroMemory(&ov,sizeof(ov));

        if ( TestNumber == 0 && fSequentialNew == FALSE) {

            Start[TestNumber] = GetTickCount();

            //
            // Make sure the file is written out to the end...
            //
            ov.Offset = (NUMBER_OF_WRITES * BUFFER_SIZE) - 1024;

            b = WriteFile(hFile,Buffer,1024,&n,&ov);
            if ( !b && GetLastError() != ERROR_IO_PENDING ) {
                printf("Error in pre-write %d\n",GetLastError());
                return 99;
                }
            b = GetQueuedCompletionStatus(
                    hPort,
                    &n2,
                    &key,
                    &ov2,
                    (DWORD)-1
                    );
            if ( !b ) {
                printf("Error picking up completion pre-write %d\n",GetLastError());
                return 99;
                }

            End[TestNumber] = GetTickCount();

            Diff = End[TestNumber] - Start[TestNumber];

            fDiff = Diff;
            fSec = fDiff/1000.0;
            fKb = ( (NumberOfWrites * BufferSize) / 1024 );

            printf("First Write %2dMb Written in %3.3fs I/O Rate %4.3f Kb/S\n\n",
                (NumberOfWrites * BufferSize) / ( 1024 * 1024),
                fSec,
                fKb / fSec
                );
            }

        ov.Offset = 0;

        PendingIoCount = 0;

        Start[TestNumber] = GetTickCount();

        //
        // Issue the writes
        //

        for (WriteCount = 0; WriteCount < NumberOfWrites; WriteCount++){
reissuewrite:
            if ( fWrite ) {
                b = WriteFile(hFile,Buffer,BufferSize,&n,&ov);
                }
            else {
                b = ReadFile(hFile,Buffer,BufferSize,&n,&ov);
                }
            if ( !b && GetLastError() != ERROR_IO_PENDING ) {

                //
                // we reached our limit on outstanding I/Os
                //

                if ( GetLastError() == ERROR_INVALID_USER_BUFFER ||
                     GetLastError() == ERROR_NOT_ENOUGH_QUOTA ||
                     GetLastError() == ERROR_NOT_ENOUGH_MEMORY ) {

                    //
                    // wait for an outstanding I/O to complete and then go again
                    //
                    b = GetQueuedCompletionStatus(
                            hPort,
                            &n2,
                            &key,
                            &ov2,
                            (DWORD)-1
                            );
                    if ( !b ) {
                        printf("Error picking up completion write %d\n",GetLastError());
                        return 99;
                        }

                    PendingIoCount--;
                    goto reissuewrite;
                    }
                else {
                    printf("Error in write %d (pending count = %d)\n",GetLastError(),PendingIoCount);
                    return 99;
                    }
                }
            PendingIoCount++;
            ov.Offset += BufferSize;
            }

        //
        // Pick up the I/O completion
        //

        for (WriteCount = 0; WriteCount < PendingIoCount; WriteCount++){
            b = GetQueuedCompletionStatus(
                    hPort,
                    &n2,
                    &key,
                    &ov2,
                    (DWORD)-1
                    );
            if ( !b ) {
                printf("Error picking up completion write %d\n",GetLastError());
                return 99;
                }
            }

        End[TestNumber] = GetTickCount();

        CloseHandle(hFile);

        if ( Version > 613 ) {
            CloseHandle(hPort);
            }

        //
        // Dump the results
        //

        Diff = End[TestNumber] - Start[TestNumber];

        fDiff = Diff;
        fSec = fDiff/1000.0;
        fKb = ( (NumberOfWrites * BufferSize) / 1024 );

        printf("Test %2d %2dMb Written in %3.3fs I/O Rate %4.3f Kb/S\n",
            TestNumber,
            (NumberOfWrites * BufferSize) / ( 1024 * 1024),
            fSec,
            fKb / fSec
            );

        fSumKbs += (fKb / fSec);

        if ( fSequentialNew ) {
            DeleteFile(FileName);
            }

        }

    DeleteFile(FileName);

    //
    // Average
    //

    printf("\n Average Throughput %4.3f\n\n",
            fSumKbs/TEST_REPEAT_COUNT
            );
}


VOID
ParseSwitch(
    CHAR chSwitch,
    int *pArgc,
    char **pArgv[]
    )
{
    int bs;
    switch (toupper( chSwitch )) {

        case '?':
            ShowUsage();
            break;

        case 'F':
            if (!--(*pArgc)) {
                ShowUsage();
                }
            (*pArgv)++;
            FileName =  *(*pArgv);
            break;

        case 'K':
            if (!--(*pArgc)) {
                ShowUsage();
                }
            (*pArgv)++;
            bs = strtoul( *(*pArgv), NULL, 10 );
            bs *= 1024;
            if ( bs > BUFFER_SIZE ) {
                ShowUsage();
                }

            BufferSize = bs;
            NumberOfWrites = ( (20*(1024*1024)) / BufferSize);

            break;


        case 'S':
            fSequentialNew = TRUE;
            break;

        case 'R':
            fRaw = TRUE;
            break;

        case 'B':
            fBufferdIo = TRUE;
            break;

        case 'X':
            fWrite = FALSE;
            break;


        default:
            fprintf( stderr, "UNBUFW: Invalid switch - /%c\n", chSwitch );
            ShowUsage();
            break;

        }
}


VOID
ShowUsage(
    VOID
    )
{
    fputs( "usage: UNBUFW [switches]\n"
           "              [-f filename] supplies the output filename\n"
           "              [-s] write sequentially without pre-allocating the file\n"
           "              [-r] use raw I/O\n"
           "              [-b] use buffered I/O instead of unbuffered I/O\n"
           "              [-x] do reads instead of writes\n"
           "              [-k write-size] use write-size k as write-size (64 is max)\n",
           stderr );

    exit( 1 );
}
