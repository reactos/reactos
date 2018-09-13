/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    tfile.c

Abstract:

    Test program for Win32 Base File API calls

Author:

    Mark Lucovsky (markl) 26-Sep-1990

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <assert.h>
#include <stdio.h>
#include <windows.h>
#include <string.h>
#include <memory.h>

#define xassert ASSERT

typedef struct _aio {
    LIST_ENTRY Links;
    HANDLE ReadHandle;
    HANDLE WriteHandle;
    LPVOID Buffer;
    OVERLAPPED Overlapped;
} AIO, *PAIO;

HANDLE IoReadsDoneEvent;
HANDLE IoWritesDoneEvent;
HANDLE IoWorkerListLock;
HANDLE IoWorkerListSemaphore;
LIST_ENTRY IoRequestList;
ULONG IoCount;
ULONG IoReadCount;
ULONG IoWriteCount;
#define BSIZE 2048


VOID
WriteIoComplete(
    DWORD dwErrorCode,
    DWORD dwNumberOfBytesTransfered,
    LPOVERLAPPED lpOverlapped
    )
{
    PAIO paio;

    //
    // If an I/O error occured, display the error and then exit
    //

    if ( dwErrorCode ) {
        printf("FATAL I/O Error %ld I/O Context %lx.%lx\n",
            dwErrorCode,
            lpOverlapped,
            lpOverlapped->hEvent
            );
        ExitProcess(dwErrorCode);
        }
    paio = (PAIO)CONTAINING_RECORD(lpOverlapped,AIO,Overlapped);
    if ( InterlockedDecrement(&IoWriteCount) == 0 ) {
        SetEvent(IoWritesDoneEvent);
        }
    LocalFree(paio->Buffer);
    LocalFree(paio);
}

VOID
ReadIoComplete(
    DWORD dwErrorCode,
    DWORD dwNumberOfBytesTransfered,
    LPOVERLAPPED lpOverlapped
    )
{
    PAIO paio;
    BOOL IoOperationStatus;

    //
    // If an I/O error occured, display the error and then exit
    //

    if ( dwErrorCode ) {
        printf("FATAL I/O Error %ld I/O Context %lx.%lx\n",
            dwErrorCode,
            lpOverlapped,
            lpOverlapped->hEvent
            );
        ExitProcess(dwErrorCode);
        }
    paio = (PAIO)CONTAINING_RECORD(lpOverlapped,AIO,Overlapped);
//printf("%s",paio->Buffer);

    IoOperationStatus = WriteFileEx(
                            paio->WriteHandle,
                            paio->Buffer,
                            dwNumberOfBytesTransfered,
                            &paio->Overlapped,
                            WriteIoComplete
                            );

    //
    // Test to see if I/O was queued successfully
    //

    if ( !IoOperationStatus ) {
        if ( GetLastError() != ERROR_HANDLE_EOF ) {
            printf("FATAL I/O Error %ld\n",
                GetLastError()
                );
            ExitProcess(1);
            }
        }

    if ( InterlockedDecrement(&IoReadCount) == 0 ) {
        SetEvent(IoReadsDoneEvent);
        }
}

VOID
IoWorkerThread(
    PVOID Unused
    )
{
    HANDLE HandleVector[2];
    DWORD CompletionStatus;
    PAIO paio;
    BOOL IoOperationStatus;

    HandleVector[0] = IoWorkerListLock;
    HandleVector[1] = IoWorkerListSemaphore;

    for(;;){

        //
        // Do an alertable wait on the handle vector. Both objects
        // becoming signaled at the same time means there is an
        // I/O request in the queue, and the caller has exclusive
        // access to the queue
        //


        CompletionStatus = WaitForMultipleObjectsEx(
                                2,
                                HandleVector,
                                TRUE,
                                0xffffffff,
                                TRUE
                                );

        //
        // If the wait failed, error out
        //

        if ( CompletionStatus == 0xffffffff ) {
            printf("FATAL WAIT ERROR %ld\n",GetLastError());
            ExitProcess(1);
            }

        //
        // If an I/O completion occured, then wait for another
        // I/O request or I/O completion
        //

        if ( CompletionStatus != WAIT_IO_COMPLETION ) {

            //
            // Wait was satisfied. We now have exclusive ownership of the
            // I/O request queue, and there is something on the queue.
            // Note that to insert something on the queue, the inserter
            // gets the list lock (mutex), inserts an entry, signals the
            // list semaphore, and finally releases the list lock
            //

            paio = (PAIO)RemoveHeadList(&IoRequestList);

            ReleaseMutex(IoWorkerListLock);

            IoOperationStatus = ReadFileEx(
                                    paio->ReadHandle,
                                    paio->Buffer,
                                    paio->Overlapped.Internal,
                                    &paio->Overlapped,
                                    ReadIoComplete
                                    );

            //
            // Test to see if I/O was queued successfully
            //

            if ( !IoOperationStatus ) {
                if ( GetLastError() != ERROR_HANDLE_EOF ) {
                    printf("FATAL I/O Error %ld\n",
                        GetLastError()
                        );
                    ExitProcess(1);
                    }
                }

            //
            // The I/O was successfully queued. Go back into the alertable
            // wait waiting for I/O completion, or for more I/O requests
            //

            }
        }

}

DWORD
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{

    HANDLE iFile,oFile;
    DWORD FileSize;
    PAIO paio;
    DWORD WaitStatus;
    DWORD Offset;
    HANDLE CompletionHandles[2];
    HANDLE Thread;
    DWORD ThreadId;

    if ( argc < 2 ) {
        printf("Usage: trd source-file destination-file\n");
        }

    iFile = CreateFile(
                argv[1],
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_FLAG_OVERLAPPED,
                NULL
                );
    if ( iFile == INVALID_HANDLE_VALUE ) {
        printf("OPEN %s failed %ld\n",argv[1],GetLastError());
        ExitProcess(1);
        }

    oFile = CreateFile(
                argv[2],
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ,
                NULL,
                CREATE_ALWAYS,
                FILE_FLAG_OVERLAPPED,
                NULL
                );
    if ( oFile == INVALID_HANDLE_VALUE ) {
        printf("OPEN %s failed %ld\n",argv[2],GetLastError());
        ExitProcess(1);
        }

    IoReadsDoneEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
    IoWritesDoneEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
    IoWorkerListLock = CreateMutex(NULL,FALSE,NULL);;
    IoWorkerListSemaphore = CreateSemaphore(NULL,0,0x7fffffff,NULL);
    xassert(IoReadsDoneEvent);
    xassert(IoWritesDoneEvent);
    xassert(IoWorkerListLock);
    xassert(IoWorkerListSemaphore);
    InitializeListHead(&IoRequestList);

    Thread = CreateThread(NULL,0L,IoWorkerThread,0,0,&ThreadId);
    xassert(Thread);

    Offset = 0;
    FileSize = GetFileSize(iFile,NULL);

    WaitStatus = WaitForSingleObject(IoWorkerListLock,-1);
    xassert(WaitStatus == 0);

    while(FileSize >= BSIZE) {
        FileSize -= BSIZE;
        paio = LocalAlloc(LMEM_ZEROINIT,sizeof(*paio));
        xassert(paio);
        paio->Buffer = LocalAlloc(LMEM_ZEROINIT,BSIZE+1);
        xassert(paio->Buffer);
        paio->ReadHandle = iFile;
        paio->WriteHandle = oFile;
        paio->Overlapped.Internal = BSIZE;
        paio->Overlapped.Offset = Offset;
        Offset += BSIZE;
        IoCount++;
        if ( IoCount & 1 ) {
            InsertTailList(&IoRequestList,&paio->Links);
            }
        else {
            InsertHeadList(&IoRequestList,&paio->Links);
            }
        ReleaseSemaphore(IoWorkerListSemaphore,1,NULL);
        }
    if ( FileSize != 0 ) {
        paio = LocalAlloc(LMEM_ZEROINIT,sizeof(*paio));
        xassert(paio);
        paio->Buffer = LocalAlloc(LMEM_ZEROINIT,FileSize+1);
        xassert(paio->Buffer);
        paio->ReadHandle = iFile;
        paio->WriteHandle = oFile;
	paio->Overlapped.interlockInternal = FileSize;
        paio->Overlapped.Offset = Offset;
        IoCount++;
        if ( IoCount & 1 ) {
            InsertTailList(&IoRequestList,&paio->Links);
            }
        else {
            InsertHeadList(&IoRequestList,&paio->Links);
            }
        ReleaseSemaphore(IoWorkerListSemaphore,1,NULL);
        }
    IoReadCount = IoCount;
    IoWriteCount = IoCount;
    ReleaseMutex(IoWorkerListLock);

    CompletionHandles[0] = IoReadsDoneEvent;
    CompletionHandles[1] = IoWritesDoneEvent;

    WaitStatus = WaitForMultipleObjects(2,CompletionHandles,TRUE,0xffffffff);
    xassert(WaitStatus != 0xffffffff);
    CloseHandle(iFile);
    CloseHandle(oFile);
    return 1;
}
