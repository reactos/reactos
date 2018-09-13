#include "stdio.h"
#include "string.h"
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"

#define SPD_PROCESS_ITERATIONS 15

//
// Define local types.
//

typedef struct _PERFINFO {
    LARGE_INTEGER StartTime;
    LARGE_INTEGER StopTime;
    ULONG ContextSwitches;
    ULONG InterruptCount;
    ULONG FirstLevelFills;
    ULONG SecondLevelFills;
    ULONG SystemCalls;
    PCHAR Title;
    ULONG Iterations;
} PERFINFO, *PPERFINFO;

VOID
SuspendedProcessTest(
    VOID
    );

VOID
FinishBenchMark (
    IN PPERFINFO PerfInfo
    );

VOID
StartBenchMark (
    IN PCHAR Title,
    IN ULONG Iterations,
    IN PPERFINFO PerfInfo
    );

VOID
main(
    int argc,
    char *argv[]
    )

{
    if ( !_strcmpi("just exit",GetCommandLine()) ) {
        return;
        }
    SuspendedProcessTest();
    return;
}

VOID
SuspendedProcessTest (
    VOID
    )

{

    PERFINFO PerfInfo;
    STARTUPINFO si;
    PROCESS_INFORMATION pi[SPD_PROCESS_ITERATIONS];
    BOOL b;
    int Index;
    CHAR Buffer[256];
    KPRIORITY Base;

    GetModuleFileName(0,Buffer,256);

    RtlZeroMemory(&si,sizeof(si));
    si.cb = sizeof(si);

    Base = 13;
    NtSetInformationProcess(
        NtCurrentProcess(),
        ProcessBasePriority,
        (PVOID) &Base,
        sizeof(Base)
        );
//    SetPriorityClass(GetCurrentProcess(),HIGH_PRIORITY_CLASS);
    StartBenchMark("Suspended Process Creation Benchmark)",
                   SPD_PROCESS_ITERATIONS,
                   &PerfInfo);

    for (Index = 0; Index < SPD_PROCESS_ITERATIONS; Index += 1) {
        b = CreateProcess(
                Buffer,
                "just exit",
                NULL,
                NULL,
                TRUE,
                CREATE_SUSPENDED,
                NULL,
                NULL,
                &si,
                &pi[Index]
                );
        if ( !b ) {
            printf("failed %ld\n",Index);
            }
        }
    //
    // Print out performance statistics.
    //

    FinishBenchMark(&PerfInfo);
//    SetPriorityClass(GetCurrentProcess(),NORMAL_PRIORITY_CLASS);

    StartBenchMark("Process Startup/Exit Benchmark)",
                   SPD_PROCESS_ITERATIONS,
                   &PerfInfo);
    for (Index = 0; Index < SPD_PROCESS_ITERATIONS; Index += 1) {
        ResumeThread(pi[Index].hThread);
        CloseHandle(pi[Index].hThread);
        WaitForSingleObject(pi[Index].hProcess,-1);
        CloseHandle(pi[Index].hProcess);
        }
    FinishBenchMark(&PerfInfo);

    //
    // End of event1 context switch test.
    //

    return;
}

VOID
FinishBenchMark (
    IN PPERFINFO PerfInfo
    )

{

    ULONG ContextSwitches;
    LARGE_INTEGER Duration;
    ULONG FirstLevelFills;
    ULONG InterruptCount;
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
    InterruptCount = SystemInfo.InterruptCount - PerfInfo->InterruptCount;
    SecondLevelFills = SystemInfo.SecondLevelTbFills - PerfInfo->SecondLevelFills;
    SystemCalls = SystemInfo.SystemCalls - PerfInfo->SystemCalls;
    printf("        First Level TB Fills      %d\n", FirstLevelFills);
    printf("        Second Level TB Fills     %d\n", SecondLevelFills);
    printf("        Number of Interrupts      %d\n", InterruptCount);
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
    PerfInfo->InterruptCount = SystemInfo.InterruptCount;
    PerfInfo->SecondLevelFills = SystemInfo.SecondLevelTbFills;
    PerfInfo->SystemCalls = SystemInfo.SystemCalls;
    return;
}
