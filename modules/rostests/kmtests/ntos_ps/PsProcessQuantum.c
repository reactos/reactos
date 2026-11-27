/*
 * PROJECT:     ReactOS kernel tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Process foreground/background quantum tests
 * COPYRIGHT:   Copyright 2026 Gleb Surikov <glebs.surikovs@gmail.com>
 */

#include <kmt_test.h>

static VOID NTAPI
QuantumWorker(PVOID Parameter)
{
    KeWaitForSingleObject(Parameter, Executive, KernelMode, FALSE, NULL);
    PsTerminateSystemThread(STATUS_SUCCESS);
}

static NTSTATUS
SetProcessClass(
    HANDLE Process,
    UCHAR Class,
    BOOLEAN Foreground
)
{
    PROCESS_PRIORITY_CLASS PriorityClass;
    PriorityClass.Foreground = Foreground;
    PriorityClass.PriorityClass = Class;
    return ZwSetInformationProcess(Process,
                                   ProcessPriorityClass,
                                   &PriorityClass,
                                   sizeof(PriorityClass));
}

static VOID
TestForegroundApi(VOID)
{
    PROCESS_PRIORITY_CLASS OriginalClass;
    PROCESS_FOREGROUND_BACKGROUND Foreground;
    PROCESS_BASIC_INFORMATION BeforeProcess, AfterProcess;
    THREAD_BASIC_INFORMATION BeforeThread, AfterThread;
    NTSTATUS Status;
    ULONG Index;

    Status = ZwQueryInformationProcess(NtCurrentProcess(),
                                       ProcessPriorityClass,
                                       &OriginalClass,
                                       sizeof(OriginalClass),
                                       NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status)) return;

    Status = ZwQueryInformationProcess(NtCurrentProcess(),
                                       ProcessBasicInformation,
                                       &BeforeProcess,
                                       sizeof(BeforeProcess),
                                       NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status)) return;

    Status = ZwQueryInformationThread(NtCurrentThread(),
                                      ThreadBasicInformation,
                                      &BeforeThread,
                                      sizeof(BeforeThread),
                                      NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status)) return;

    for (Index = 0; Index < 2; Index += 1)
    {
        Foreground.Foreground = Index == 0;
        Status = ZwSetInformationProcess(NtCurrentProcess(),
                                         ProcessForegroundInformation,
                                         &Foreground,
                                         sizeof(Foreground));
        ok_eq_hex(Status, STATUS_SUCCESS);

        Status = ZwQueryInformationProcess(NtCurrentProcess(),
                                           ProcessBasicInformation,
                                           &AfterProcess,
                                           sizeof(AfterProcess),
                                           NULL);
        ok_eq_hex(Status, STATUS_SUCCESS);
        if (NT_SUCCESS(Status))
            ok_eq_long(AfterProcess.BasePriority, BeforeProcess.BasePriority);

        Status = ZwQueryInformationThread(NtCurrentThread(),
                                          ThreadBasicInformation,
                                          &AfterThread,
                                          sizeof(AfterThread),
                                          NULL);
        ok_eq_hex(Status, STATUS_SUCCESS);
        if (NT_SUCCESS(Status))
            ok_eq_long(AfterThread.BasePriority, BeforeThread.BasePriority);
    }

    Status = ZwSetInformationProcess(NtCurrentProcess(),
                                     ProcessPriorityClass,
                                     &OriginalClass,
                                     sizeof(OriginalClass));
    ok_eq_hex(Status, STATUS_SUCCESS);
}

START_TEST(PsProcessQuantum)
{
    OBJECT_ATTRIBUTES Attributes;
    HANDLE ProcessHandle = NULL, Threads[2] = {NULL, NULL};
    PEPROCESS Process = PsGetCurrentProcess();
    PETHREAD Thread[2] = {NULL, NULL};
    PKPROCESS Pcb = (PKPROCESS)Process;
    KEVENT Stop;
    NTSTATUS Status;
    PROCESS_FOREGROUND_BACKGROUND Foreground;
    UCHAR ForegroundQuantum, BackgroundQuantum, OriginalClass, SavedQuantum;
    BOOLEAN OriginalForeground;
    KPRIORITY BasePriority, BoostedPriority;
    ULONG BasePriorityInfo;
    ULONG Index, Attempt;
    LARGE_INTEGER Delay;

    /* Currently, Vista runs API level checks, but quantum checks use NT 5.2 layouts */
    if (GetNTVersion() != _WIN32_WINNT_WS03)
    {
        TestForegroundApi();

        skip(0, "Quantum checks require the NT 5.2 process/thread layout\n");
        return;
    }

    /* Check the normal console process that submitted this test */
    OriginalClass = Process->PriorityClass;
    OriginalForeground = Process->Vm.Flags.MemoryPriority == MEMORY_PRIORITY_FOREGROUND;
    KeInitializeEvent(&Stop, NotificationEvent, FALSE);
    InitializeObjectAttributes(&Attributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
    Status = ObOpenObjectByPointer(Process,
                                   OBJ_KERNEL_HANDLE,
                                   NULL,
                                   PROCESS_ALL_ACCESS,
                                   *PsProcessType,
                                   KernelMode,
                                   &ProcessHandle);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status)) return;

    TestForegroundApi();

    Status = PsCreateSystemThread(&Threads[0],
                                  THREAD_ALL_ACCESS,
                                  &Attributes,
                                  ProcessHandle,
                                  NULL,
                                  QuantumWorker,
                                  &Stop);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status)) goto Cleanup;
    Status = ObReferenceObjectByHandle(Threads[0],
                                       0,
                                       *PsThreadType,
                                       KernelMode,
                                       (PVOID *)&Thread[0],
                                       NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status)) goto Cleanup;

    Delay.QuadPart = -10000;
    for (Attempt = 0;
         Attempt < 1000 && Thread[0]->Tcb.State != Waiting;
         Attempt += 1)
    {
        KeDelayExecutionThread(KernelMode, FALSE, &Delay);
    }
    ok_eq_uint(Thread[0]->Tcb.State, Waiting);
    if (Thread[0]->Tcb.State != Waiting) goto Cleanup;

    /* Establish each policy's quantum */
    Status = SetProcessClass(ProcessHandle, PROCESS_PRIORITY_CLASS_BELOW_NORMAL, TRUE);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status)) goto Cleanup;
    ForegroundQuantum = Pcb->QuantumReset;
    ok_eq_uint(Thread[0]->Tcb.QuantumReset, ForegroundQuantum);
    Status = SetProcessClass(ProcessHandle, PROCESS_PRIORITY_CLASS_NORMAL, FALSE);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status)) goto Cleanup;
    BackgroundQuantum = Pcb->QuantumReset;
    ok_eq_uint(Thread[0]->Tcb.QuantumReset, BackgroundQuantum);
    trace("FG quantum=%u BG quantum=%u\n", ForegroundQuantum, BackgroundQuantum);
    if (ForegroundQuantum == BackgroundQuantum)
        skip(0, "The configured policy uses the same FG/BG quantum\n");

    BasePriority = Thread[0]->Tcb.BasePriority;
    BoostedPriority = BasePriority + 1;
    KeSetPriorityThread(&Thread[0]->Tcb, BoostedPriority);

    /* A quantum only change */
    Foreground.Foreground = TRUE;
    Status = ZwSetInformationProcess(ProcessHandle,
                                     ProcessForegroundInformation,
                                     &Foreground,
                                     sizeof(Foreground));
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(Pcb->QuantumReset, ForegroundQuantum);
    ok_eq_uint(Thread[0]->Tcb.QuantumReset, ForegroundQuantum);
    ok_eq_int(Thread[0]->Tcb.BasePriority, BasePriority);
    ok_eq_int(Thread[0]->Tcb.Priority, BoostedPriority);

    /*
     * ProcessBasePriority passes zero for the quantum
     * Keep the foreground flag while setting the same base priority
     */
    SavedQuantum = Pcb->QuantumReset;
    BasePriorityInfo = (ULONG)BasePriority | 0x80000000;
    Status = ZwSetInformationProcess(ProcessHandle,
                                     ProcessBasePriority,
                                     &BasePriorityInfo,
                                     sizeof(BasePriorityInfo));
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(Pcb->QuantumReset, SavedQuantum);
    ok_eq_uint(Thread[0]->Tcb.QuantumReset, SavedQuantum);
    ok_eq_int(Thread[0]->Tcb.Priority, BoostedPriority);

    /* Threads created after the change must inherit the new quantum */
    Status = PsCreateSystemThread(&Threads[1],
                                  THREAD_ALL_ACCESS,
                                  &Attributes,
                                  ProcessHandle,
                                  NULL,
                                  QuantumWorker,
                                  &Stop);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status)) goto Cleanup;

    Status = ObReferenceObjectByHandle(Threads[1],
                                       0,
                                       *PsThreadType,
                                       KernelMode,
                                       (PVOID *)&Thread[1],
                                       NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status)) goto Cleanup;
    ok_eq_uint(Thread[1]->Tcb.QuantumReset, ForegroundQuantum);

    Foreground.Foreground = FALSE;
    Status = ZwSetInformationProcess(ProcessHandle,
                                     ProcessForegroundInformation,
                                     &Foreground,
                                     sizeof(Foreground));
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(Pcb->QuantumReset, BackgroundQuantum);
    ok_eq_uint(Thread[0]->Tcb.QuantumReset, BackgroundQuantum);
    ok_eq_uint(Thread[1]->Tcb.QuantumReset, BackgroundQuantum);
    ok_eq_int(Thread[0]->Tcb.Priority, BoostedPriority);

Cleanup:
    KeSetEvent(&Stop, IO_NO_INCREMENT, FALSE);
    for (Index = 0; Index < RTL_NUMBER_OF(Threads); Index += 1)
    {
        if (Threads[Index])
        {
            Status = ZwWaitForSingleObject(Threads[Index], FALSE, NULL);
            ok_eq_hex(Status, STATUS_SUCCESS);
            ZwClose(Threads[Index]);
        }
        if (Thread[Index])
        {
            ObDereferenceObject(Thread[Index]);
        }
    }
    Status = SetProcessClass(ProcessHandle, OriginalClass, OriginalForeground);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ZwClose(ProcessHandle);
}
