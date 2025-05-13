/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite, Registry change notification
 * PROGRAMMER:      Mohammad Amin Mollazadeh <madamin@pm.me>
 */

#include <kmt_test.h>

/* Helper functions */

#define ok_ntstatus(svar, sexpected) ok_eq_hex(svar, sexpected)
#define CLEANUP(state) ZwNotifyChangeKey_Cleanup(state)
#define CHECK_ERROR(state, Status) if (!NT_SUCCESS(Status)) { ok(FALSE, "Internal error, Status=0x%lx\n", Status); CLEANUP(state); return Status; }
#define TEST_ERROR(state, Status, Expected)  ok_ntstatus(Status, Expected); if (Status != Expected) { CLEANUP(state); return Status; }
#define INIT_TEST(state, Status) Status = ZwNotifyChangeKey_InitializePerTest(state); CHECK_ERROR(state, Status)
#define FINALIZE_TEST(state) CLEANUP(state); return STATUS_SUCCESS
#define RUN_TEST(state, Status, TestName) Status = ZwNotifyChangeKey_TEST_##TestName(state); ok(Status == STATUS_SUCCESS, "Subtest "#TestName" failed.\n")
#define START_SUBTEST(TestName) NTSTATUS NTAPI ZwNotifyChangeKey_TEST_##TestName(PWATCH_REG_TEST_STATE state)

typedef struct _WATCH_REG_TEST_STATE
{
    NTSTATUS Status;
    HANDLE KeyHandle;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING ValueName;
    /* Thread */
    LARGE_INTEGER WaitTimeout;
    HANDLE WatchThreadHandle;
    PKTHREAD WatchThreadObject;
    /* Event */
    HANDLE EventHandle;
    PKEVENT EventObject;
    /* Work queue item */
    WORK_QUEUE_ITEM WorkQueueItem;

} WATCH_REG_TEST_STATE, *PWATCH_REG_TEST_STATE;

VOID NTAPI ZwNotifyChangeKey_Cleanup(PWATCH_REG_TEST_STATE state)
{
    if (state->EventObject)
    {
        ObDereferenceObject(state->EventObject);
        state->EventObject = NULL;
    }
    if (state->EventHandle)
    {
        ObCloseHandle(state->EventHandle, KernelMode);
        state->EventHandle = NULL;
    }
    if (state->WatchThreadObject)
    {
        ObDereferenceObject(state->WatchThreadObject);
        state->WatchThreadObject = NULL;
    }
    if (state->WatchThreadHandle)
    {
        ObCloseHandle(state->WatchThreadHandle, KernelMode);
        state->WatchThreadHandle = NULL;
    }
    if (state->KeyHandle)
    {
        ObCloseHandle(state->KeyHandle, KernelMode);
        state->KeyHandle = NULL;
    }
}

NTSTATUS NTAPI ZwNotifyChangeKey_Initialize(PWATCH_REG_TEST_STATE state)
{
    NTSTATUS Status;

    state->WaitTimeout.QuadPart = -1 * 100 * 1000 * 10;

    state->KeyHandle = NULL;
    state->WatchThreadHandle = NULL;
    state->WatchThreadObject = NULL;
    state->EventHandle = NULL;
    state->EventObject = NULL;

    /* Setup object attributes */
    RtlInitUnicodeString(&state->KeyName, L"\\Registry\\Machine\\SOFTWARE\\TestKey");
    InitializeObjectAttributes(&state->ObjectAttributes, &state->KeyName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    RtlInitUnicodeString(&state->ValueName, L"TestValue");

    /* Create registry keys */
    Status = ZwCreateKey(&state->KeyHandle, KEY_ALL_ACCESS, &state->ObjectAttributes, 0, NULL, REG_OPTION_NON_VOLATILE, NULL);
    CHECK_ERROR(state, Status);

    FINALIZE_TEST(state);
}

NTSTATUS NTAPI ZwNotifyChangeKey_CleanupTestKeys(PWATCH_REG_TEST_STATE state)
{
    NTSTATUS Status;

    Status = ZwOpenKey(&state->KeyHandle, DELETE, &state->ObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        ZwDeleteKey(state->KeyHandle);
        ObCloseHandle(state->KeyHandle, KernelMode);
    }

    return Status;
}

NTSTATUS NTAPI ZwNotifyChangeKey_InitializePerTest(PWATCH_REG_TEST_STATE state)
{
    NTSTATUS Status;

    Status = ZwOpenKey(&state->KeyHandle, KEY_SET_VALUE | KEY_NOTIFY | DELETE, &state->ObjectAttributes);
    CHECK_ERROR(state, Status);

    DWORD32 DefaultValue = 0x12345678;
    Status = ZwSetValueKey(state->KeyHandle, &state->ValueName, 0, REG_DWORD, &DefaultValue, sizeof(DefaultValue));
    CHECK_ERROR(state, Status);

    state->IoStatusBlock.Status = 0xdeadbeef;

    return STATUS_SUCCESS;
}

/* Sync test watch thread */

_Function_class_(KSTART_ROUTINE)
VOID NTAPI ZwNotifyChangeKey_WatchThread(_In_ PVOID lpParameter)
{
    PWATCH_REG_TEST_STATE State = (PWATCH_REG_TEST_STATE)lpParameter;
    
    State->Status = ZwNotifyChangeKey(State->KeyHandle, NULL, NULL, NULL, &State->IoStatusBlock, REG_NOTIFY_CHANGE_LAST_SET, FALSE, NULL, 0, FALSE);
}

/* Async test queue worker item */

_Function_class_(WORKER_THREAD_ROUTINE)
VOID NTAPI ZwNotifyChangeKey_ItemWorker(_In_ PVOID pParameter)
{
    PWATCH_REG_TEST_STATE ctx = (PWATCH_REG_TEST_STATE)pParameter;

    /* Signal that we received a notification */
    KeSetEvent(ctx->EventObject, 1, FALSE);
}

/* Tests */

START_SUBTEST(Synchronous)
{
    NTSTATUS Status;
    DWORD32 ValueData = 0x87654321;

    INIT_TEST(state, Status);

    /* Create a thread */
    Status = PsCreateSystemThread(&state->WatchThreadHandle, THREAD_ALL_ACCESS, NULL, NULL, NULL, ZwNotifyChangeKey_WatchThread, state);
    CHECK_ERROR(state, Status);
    Status = ObReferenceObjectByHandle(state->WatchThreadHandle, THREAD_ALL_ACCESS, NULL, KernelMode, (PVOID*)&state->WatchThreadObject, NULL);
    CHECK_ERROR(state, Status);

    /* Verify the thread is still running */
    Status = KeWaitForSingleObject(state->WatchThreadObject, Executive, KernelMode, FALSE, &state->WaitTimeout);
    if (Status != STATUS_TIMEOUT)
    {
        ok(FALSE, "Thread terminated soon unexpectedly (0x%lx, 0x%lx)\n", state->Status, Status);
        CLEANUP(state);
        return STATUS_UNSUCCESSFUL;
    }

    /* Make change to the registry key */
    Status = ZwSetValueKey(state->KeyHandle, &state->ValueName, 0, REG_DWORD, &ValueData, sizeof(ValueData));
    CHECK_ERROR(state, Status);
    /* Give system some time to process the change and notify our watch thread */
    KeDelayExecutionThread(KernelMode, FALSE, &state->WaitTimeout);
    /* Verify that the thread is notified */
    Status = KeWaitForSingleObject(state->WatchThreadObject, Executive, KernelMode, FALSE, &state->WaitTimeout);
    ok_ntstatus(Status, STATUS_WAIT_0);

    /* Verify thread's return value */
    ok_ntstatus(state->Status, STATUS_NOTIFY_ENUM_DIR);
    ok_ntstatus(state->IoStatusBlock.Status, STATUS_NOTIFY_ENUM_DIR);

    FINALIZE_TEST(state);
}

START_SUBTEST(AsynchronousEvent)
{
    NTSTATUS Status;
    DWORD32 ValueData = 0x87654321;

    INIT_TEST(state, Status);

    /* Initialize event */
    Status = ZwCreateEvent(&state->EventHandle, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);
    CHECK_ERROR(state, Status);
    Status = ObReferenceObjectByHandle(state->EventHandle, EVENT_ALL_ACCESS, NULL, KernelMode, (PVOID*)&state->EventObject, NULL);
    CHECK_ERROR(state, Status);

    /* Listen for notification */
    Status = ZwNotifyChangeKey(state->KeyHandle, state->EventHandle, NULL, NULL, &state->IoStatusBlock, REG_NOTIFY_CHANGE_LAST_SET, FALSE, NULL, 0, TRUE);
    ok_ntstatus(Status, STATUS_PENDING);

    /* Check event state */
    Status = KeWaitForSingleObject(state->EventObject, Executive, KernelMode, FALSE, &state->WaitTimeout);
    if (Status != STATUS_TIMEOUT)
    {
        CLEANUP(state);
        return STATUS_UNSUCCESSFUL;
    }

    /* Make change to the registry key */
    Status = ZwSetValueKey(state->KeyHandle, &state->ValueName, 0, REG_DWORD, &ValueData, sizeof(ValueData));
    CHECK_ERROR(state, Status);

    /* Verify that the event is signaled */
    Status = KeWaitForSingleObject(state->EventObject, Executive, KernelMode, FALSE, &state->WaitTimeout);
    ok_ntstatus(Status, STATUS_WAIT_0);
    /* Verify status */
    // ok_ntstatus(state->IoStatusBlock.Status, STATUS_NOTIFY_ENUM_DIR);

    FINALIZE_TEST(state);
}

START_SUBTEST(AsynchronousWqi)
{
    NTSTATUS Status;
    DWORD32 ValueData = 0x87654321;

    INIT_TEST(state, Status);

    /* Initialize event */
    Status = ZwCreateEvent(&state->EventHandle, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);
    CHECK_ERROR(state, Status);
    Status = ObReferenceObjectByHandle(state->EventHandle, EVENT_ALL_ACCESS, NULL, KernelMode, (PVOID*)&state->EventObject, NULL);
    CHECK_ERROR(state, Status);

    /* Initialize Work Queue Item */
    ExInitializeWorkItem(&state->WorkQueueItem, ZwNotifyChangeKey_ItemWorker, state);

    /* Listen for notification */
    Status = ZwNotifyChangeKey(state->KeyHandle, NULL, (PVOID)&state->WorkQueueItem, (PVOID)DelayedWorkQueue, &state->IoStatusBlock, REG_NOTIFY_CHANGE_LAST_SET, FALSE, NULL, 0, TRUE);
    ok_ntstatus(Status, STATUS_PENDING);

    /* Check event state */
    Status = KeWaitForSingleObject(state->EventObject, Executive, KernelMode, FALSE, &state->WaitTimeout);
    if (Status != STATUS_TIMEOUT)
    {
        CLEANUP(state);
        return STATUS_UNSUCCESSFUL;
    }

    /* Make change to the registry key */
    Status = ZwSetValueKey(state->KeyHandle, &state->ValueName, 0, REG_DWORD, &ValueData, sizeof(ValueData));
    CHECK_ERROR(state, Status);

    /* Verify that the event is signaled */
    Status = KeWaitForSingleObject(state->EventObject, Executive, KernelMode, FALSE, &state->WaitTimeout);
    ok_ntstatus(Status, STATUS_WAIT_0);
    /* Verify status */
    // ok_ntstatus(state->IoStatusBlock.Status, STATUS_NOTIFY_ENUM_DIR);

    FINALIZE_TEST(state);
}


/* Main */

START_TEST(ZwNotifyChangeKey)
{
    NTSTATUS Status;
    WATCH_REG_TEST_STATE State;

    ZwNotifyChangeKey_Initialize(&State);
    
    /* See also: modules/rostests/apitests/ntdll/NtNotifyChangeMultipleKeys */

    RUN_TEST(&State, Status, Synchronous);
    RUN_TEST(&State, Status, AsynchronousEvent);
    RUN_TEST(&State, Status, AsynchronousWqi);
    //RUN_SUBTEST(AsynchronousSUAF);
    
    ZwNotifyChangeKey_CleanupTestKeys(&State);
}
