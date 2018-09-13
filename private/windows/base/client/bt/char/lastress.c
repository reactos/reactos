#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <string.h>

typedef DWORD ERESOURCE;

#include "\nt\private\ntos\inc\heap.h"

#define ITERATIONS 1000
#define SIZES 10

DWORD Sizes[SIZES] = {
                      100,
                      10,
                      128,
                      256,
                      256,
                      100,
                      32,
                      64,
                      64,
                      100
                     };

VOID
LaThread(PVOID h)
{
    int i,j;
    PVOID p;

    WaitForSingleObject((HANDLE)h,INFINITE);

    for(i=0;i<ITERATIONS;i++){
        for(j=0;j<SIZES;j++) {
            if ( j & 1 ) {
                p = LocalAlloc(LMEM_FIXED,Sizes[j]);
                }
            else {
                p = LocalAlloc(LMEM_ZEROINIT,Sizes[j]);
                }
            if ( p ) {
                LocalFree(p);
                }
            else {
                printf("Alloc In Thread Failed\n");
                ExitProcess(1);
                }
            }
        }
}

VOID
StartTest(
    DWORD SpinCount,
    DWORD ThreadCount
    )
{
    PHANDLE Threads,p;
    HANDLE hStartEvent;
    DWORD Id;
    DWORD i;
    PHEAP ProcessHeap;
    PRTL_CRITICAL_SECTION HeapLock;
    DWORD Start,End;

    ProcessHeap = (PHEAP)NtCurrentPeb()->ProcessHeap;
    HeapLock = &ProcessHeap->LockVariable->Lock.CriticalSection;
    if ( HeapLock ) {
        HeapLock->DebugInfo->ContentionCount = 0;
        if ( SpinCount != 0xffffffff) {
            HeapLock->SpinCount = SpinCount;
            }
        }
    hStartEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
    if ( !hStartEvent ) {
        printf("CreateEvent Failed %d\n",GetLastError());
        ExitProcess(1);
        }

    if ( ThreadCount < 2 ) {
        ThreadCount = 2;
        }
    if ( ThreadCount > 64 ) {
        ThreadCount = 64;
        }

    Threads = LocalAlloc(LMEM_ZEROINIT,sizeof(HANDLE)*ThreadCount);
    if ( !Threads ) {
        printf("Alloc Threads Failed\n");
        ExitProcess(1);
        }
    for(i=0,p=Threads;i<ThreadCount;i++,p++){

        *p = CreateThread(
                NULL,
                0,
                (LPTHREAD_START_ROUTINE)LaThread,
                (PVOID)hStartEvent,
                0,
                &Id
                );

        if ( !*p ) {
            printf("CreateThread Failed %d\n",GetLastError());
            ExitProcess(1);
            }
        }
    Start = GetTickCount();
    SetEvent(hStartEvent);

    if ( WaitForMultipleObjects(ThreadCount,Threads,TRUE,INFINITE) == WAIT_FAILED ) {
        printf("WaitMultiple Failed %d\n",GetLastError());
        ExitProcess(1);
        }
    End = GetTickCount();

    for(i=0,p=Threads;i<ThreadCount;i++,p++){
        CloseHandle(*p);
        }
    CloseHandle(hStartEvent);

    if ( !HeapLock ) {
        HeapLock = &ProcessHeap->LockVariable->Lock.CriticalSection;
        if ( !HeapLock ) {
            printf("No Heap Lock found\n");
            ExitProcess(1);
            }
        }
    printf("Heap Contention Threads %2d SpinCount %5d -> Contention %7d TickCount %d\n",ThreadCount,SpinCount,HeapLock->DebugInfo->ContentionCount,End-Start);

}

DWORD
_cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{

    StartTest(0xffffffff,2);
    StartTest(0,2);
    StartTest(1000,2);
    StartTest(3000,2);
    StartTest(4000,2);
    StartTest(5000,2);
    StartTest(10000,2);

    printf("\n");

    StartTest(0,4);
    StartTest(1000,4);
    StartTest(3000,4);
    StartTest(4000,4);
    StartTest(5000,4);
    StartTest(10000,4);

    printf("\n");

    StartTest(0,5);
    StartTest(1000,5);
    StartTest(3000,5);
    StartTest(4000,5);
    StartTest(5000,5);
    StartTest(10000,5);

    printf("\n");

    StartTest(0,6);
    StartTest(1000,6);
    StartTest(3000,6);
    StartTest(4000,6);
    StartTest(5000,6);
    StartTest(10000,6);

    printf("\n");

    StartTest(0,8);
    StartTest(1000,8);
    StartTest(3000,8);
    StartTest(4000,8);
    StartTest(5000,8);
    StartTest(10000,8);

}
