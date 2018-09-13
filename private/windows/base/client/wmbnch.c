#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <string.h>
#include <memory.h>
#include <process.h>

#define WAIT_MULTIPLE_ITERATIONS 10000
HANDLE WaitHandles[64];

//
// Define local types.
//

typedef struct _PERFINFO {
    LARGE_INTEGER StartTime;
    LARGE_INTEGER StopTime;
    ULONG ContextSwitches;
    ULONG FirstLevelFills;
    ULONG SecondLevelFills;
    ULONG SystemCalls;
    PCHAR Title;
    ULONG Iterations;
} PERFINFO, *PPERFINFO;

VOID
FinishBenchMark (
    IN PPERFINFO PerfInfo
    )

{

    ULONG ContextSwitches;
    LARGE_INTEGER Duration;
    ULONG FirstLevelFills;
    ULONG Length;
    ULONG Performance;
    ULONG SecondLevelFills;
    NTSTATUS Status;
    ULONG SystemCalls;
    SYSTEM_PERFORMANCE_INFORMATION SystemInfo;


    //
    // Print results and announce end of test.
    //

    NtQuerySystemTime((PLARGE_INTEGER)&PerfInfo->StopTime);
    Status = NtQuerySystemInformation(SystemPerformanceInformation,
                                      (PVOID)&SystemInfo,
                                      sizeof(SYSTEM_PERFORMANCE_INFORMATION),
                                      NULL);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to query performance information, status = %lx\n", Status);
        return;
    }

    Duration = RtlLargeIntegerSubtract(PerfInfo->StopTime, PerfInfo->StartTime);
    Length = Duration.LowPart / 10000;
    printf("        Test time in milliseconds %d\n", Length);
    printf("        Number of iterations      %d\n", PerfInfo->Iterations);

    Performance = PerfInfo->Iterations * 1000 / Length;
    printf("        Iterations per second     %d\n", Performance);

    ContextSwitches = SystemInfo.ContextSwitches - PerfInfo->ContextSwitches;
    FirstLevelFills = SystemInfo.FirstLevelTbFills - PerfInfo->FirstLevelFills;
    SecondLevelFills = SystemInfo.SecondLevelTbFills - PerfInfo->SecondLevelFills;
    SystemCalls = SystemInfo.SystemCalls - PerfInfo->SystemCalls;
    printf("        First Level TB Fills      %d\n", FirstLevelFills);
    printf("        Second Level TB Fills     %d\n", SecondLevelFills);
    printf("        Total Context Switches    %d\n", ContextSwitches);
    printf("        Number of System Calls    %d\n", SystemCalls);

    printf("*** End of Test ***\n\n");
    return;
}

VOID
StartBenchMark (
    IN PCHAR Title,
    IN ULONG Iterations,
    IN PPERFINFO PerfInfo
    )

{

    NTSTATUS Status;
    SYSTEM_PERFORMANCE_INFORMATION SystemInfo;

    //
    // Announce start of test and the number of iterations.
    //

    printf("*** Start of test ***\n    %s\n", Title);
    PerfInfo->Title = Title;
    PerfInfo->Iterations = Iterations;
    NtQuerySystemTime((PLARGE_INTEGER)&PerfInfo->StartTime);
    Status = NtQuerySystemInformation(SystemPerformanceInformation,
                                      (PVOID)&SystemInfo,
                                      sizeof(SYSTEM_PERFORMANCE_INFORMATION),
                                      NULL);

    if (NT_SUCCESS(Status) == FALSE) {
        printf("Failed to query performance information, status = %lx\n", Status);
        return;
    }

    PerfInfo->ContextSwitches = SystemInfo.ContextSwitches;
    PerfInfo->FirstLevelFills = SystemInfo.FirstLevelTbFills;
    PerfInfo->SecondLevelFills = SystemInfo.SecondLevelTbFills;
    PerfInfo->SystemCalls = SystemInfo.SystemCalls;
    return;
}


VOID
WaitMultipleTest()
{
    PERFINFO PerfInfo;
    int i;

    StartBenchMark(
        "Wait Single Object",
        WAIT_MULTIPLE_ITERATIONS,
        &PerfInfo
        );

    for ( i=0;i<WAIT_MULTIPLE_ITERATIONS;i++) {
        WaitForSingleObject(WaitHandles[0],INFINITE);   // Wait Single Object
        }

    FinishBenchMark(&PerfInfo);

    StartBenchMark(
        "Wait Multiple Test 1 Object",
        WAIT_MULTIPLE_ITERATIONS,
        &PerfInfo
        );

    for ( i=0;i<WAIT_MULTIPLE_ITERATIONS;i++) {
        WaitForMultipleObjects(1,WaitHandles,FALSE,INFINITE);   // Wait Any, 1 Object
        }

    FinishBenchMark(&PerfInfo);

    StartBenchMark(
        "Wait Multiple Test 8 Objects",
        WAIT_MULTIPLE_ITERATIONS,
        &PerfInfo
        );

    for ( i=0;i<WAIT_MULTIPLE_ITERATIONS;i++) {
        WaitForMultipleObjects(8,WaitHandles,FALSE,INFINITE);   // Wait Any, 8 Objects
        }

    FinishBenchMark(&PerfInfo);

    StartBenchMark(
        "Wait Multiple Test 16 Objects",
        WAIT_MULTIPLE_ITERATIONS,
        &PerfInfo
        );

    for ( i=0;i<WAIT_MULTIPLE_ITERATIONS;i++) {
        WaitForMultipleObjects(16,WaitHandles,FALSE,INFINITE);   // Wait Any, 16 Objects
        }

    FinishBenchMark(&PerfInfo);

    StartBenchMark(
        "Wait Multiple Test 32 Objects",
        WAIT_MULTIPLE_ITERATIONS,
        &PerfInfo
        );

    for ( i=0;i<WAIT_MULTIPLE_ITERATIONS;i++) {
        WaitForMultipleObjects(32,WaitHandles,FALSE,INFINITE);   // Wait Any, 32 Objects
        }

    FinishBenchMark(&PerfInfo);

    StartBenchMark(
        "Wait Multiple Test 64 Objects",
        WAIT_MULTIPLE_ITERATIONS,
        &PerfInfo
        );

    for ( i=0;i<WAIT_MULTIPLE_ITERATIONS;i++) {
        WaitForMultipleObjects(64,WaitHandles,FALSE,INFINITE);   // Wait Any, 64 Objects
        }

    FinishBenchMark(&PerfInfo);
}

DWORD
_cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{

    int i;

    for(i=0;i<64;i++){
        WaitHandles[i] = CreateEvent(NULL,TRUE,TRUE,NULL);
        }
    WaitMultipleTest();
    return 0;
}
