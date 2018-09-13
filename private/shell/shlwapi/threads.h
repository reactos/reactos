/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    threads.h

Abstract:

    Win32 version of NT wait/timer/thread pool functions

Author:

    Richard L Firth (rfirth) 27-Feb-1998

Notes:

    Original code from NT5/gurdeep

Revision History:

    27-Feb-1998 rfirth
        Created

--*/

//
// manifests
//

#define TPS_IO_WORKER_SIGNATURE     0x49737054  // 'TpsI'
#define TPS_WORKER_SIGNATURE        0x4B737054  // 'TpsK'
#define TPS_TIMER_SIGNATURE         0x54737054  // 'TpsT'
#define TPS_WAITER_SIGNATURE        0x57577054  // 'TpsW'

#define MAX_WAITS   64

//
// global data
//

EXTERN_C BOOL g_bDllTerminating;
extern BOOL g_bTpsTerminating;
extern DWORD g_ActiveRequests;

//
// prototypes for internal functions
//

VOID
TerminateTimers(
    VOID
    );

VOID
TerminateWaiters(
    VOID
    );

VOID
TerminateWorkers(
    VOID
    );

//
// Prototypes for thread pool private functions
//

DWORD
StartThread(
    IN LPTHREAD_START_ROUTINE pfnFunction,
    OUT PHANDLE phThread,
    IN BOOL fSynchronize
    );

DWORD
TpsEnter(
    VOID
    );

#define TpsLeave() \
    InterlockedDecrement((LPLONG)&g_ActiveRequests)

VOID
QueueNullFunc(
    IN HANDLE hThread
    );

#define THREAD_TYPE_WORKER      1
#define THREAD_TYPE_IO_WORKER   2
