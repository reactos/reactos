/*
 * PROJECT:     ReactOS Kernel-Mode tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for registry change notification
 * COPYRIGHT:   Copyright 2026 Mohammad Amin Mollazadeh <madamin@pm.me>
 */

#include <kmt_test.h>

/* Timeout definition for use with KeDelayExecutionThread function */
#define TIMEOUT_MICROSECONDS 10 /* The unit used by KeDelayExecutionThread is 100 nanoseconds, multiplying by 10 makes it 1 microsecond */
#define RELATIVE_TIMEOUT(x) (-1 * (x)) /* KeDelayExecutionThread uses negative values to indicate relative time */

#define SYNC_THREAD_WAIT_TIMEOUT RELATIVE_TIMEOUT(100 * TIMEOUT_MICROSECONDS)

/* Thread to watch for registry changes for synchronous mode test */
typedef struct _WATCH_THREAD_STATE
{
    NTSTATUS Status;
    HANDLE KeyHandle;
    IO_STATUS_BLOCK IoStatusBlock;
} WATCH_THREAD_STATE, *PWATCH_THREAD_STATE;

_Function_class_(KSTART_ROUTINE)
VOID NTAPI ZwNotifyChangeKey_WatchThread(_In_ PVOID lpParameter)
{
    PWATCH_THREAD_STATE State = (PWATCH_THREAD_STATE)lpParameter;
    State->Status = ZwNotifyChangeKey(State->KeyHandle, NULL, NULL, NULL, &State->IoStatusBlock, REG_NOTIFY_CHANGE_LAST_SET, FALSE, NULL, 0, FALSE);
}

/* WorkerQueueItem for asynchronous mode test */
_Function_class_(WORKER_THREAD_ROUTINE)
VOID NTAPI ZwNotifyChangeKey_ItemWorker(_In_ PVOID pParameter)
{
    PKEVENT event = (PKEVENT)pParameter;
    KeSetEvent(event, 1, FALSE);
}

/* Main */
START_TEST(ZwNotifyChangeKey)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    /* Registry key handles and values */
    HANDLE KeyHandle = NULL;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING ValueName;
    DWORD32 Value1 = 0x12345678;
    DWORD32 Value2 = 0x87654321;
    /* Thread/Event/WorkQueueItem-related objects and data */
    LARGE_INTEGER WaitTimeout;
    WATCH_THREAD_STATE WatchThreadState;
    HANDLE WatchThreadHandle = NULL;
    PKTHREAD WatchThreadObject = NULL;
    HANDLE EventHandle = NULL;
    PKEVENT EventObject = NULL;
    WORK_QUEUE_ITEM WorkQueueItem;

    /* Create registry keys */
    RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\SOFTWARE\\TestKey");
    InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    RtlInitUnicodeString(&ValueName, L"TestValue");
    Status = ZwCreateKey(&KeyHandle, KEY_ALL_ACCESS, &ObjectAttributes, 0, NULL, REG_OPTION_NON_VOLATILE, NULL);
    if (!NT_SUCCESS(Status))
    {
        skip(FALSE, "Failed to create registry key");
        return;
    }
    Status = ZwSetValueKey(KeyHandle, &ValueName, 0, REG_DWORD, &Value1, sizeof(Value1));
    if (!NT_SUCCESS(Status))
    {
        skip(FALSE, "Failed to set default value for registry key");
        ZwDeleteKey(KeyHandle);
        ObCloseHandle(KeyHandle, KernelMode);
        return;
    }

    /* Synchronous wait test */

    /* Create a thread */
    WatchThreadState.KeyHandle = KeyHandle;
    WatchThreadState.IoStatusBlock.Status = 0xdeadbeef;
    Status = PsCreateSystemThread(&WatchThreadHandle, THREAD_ALL_ACCESS, NULL, NULL, NULL, ZwNotifyChangeKey_WatchThread, &WatchThreadState);
    if (NT_SUCCESS(Status))
    {
        Status = ObReferenceObjectByHandle(WatchThreadHandle, THREAD_ALL_ACCESS, NULL, KernelMode, (PVOID*)&WatchThreadObject, NULL);
    }
    if (NT_SUCCESS(Status))
    {
        /* Verify the thread is still running */
        WaitTimeout.QuadPart = SYNC_THREAD_WAIT_TIMEOUT;
        Status = KeWaitForSingleObject(WatchThreadObject, Executive, KernelMode, FALSE, &WaitTimeout);
        ok_eq_hex(Status, STATUS_TIMEOUT);
        /* Make change to the registry key */
        Status = ZwSetValueKey(KeyHandle, &ValueName, 0, REG_DWORD, &Value2, sizeof(Value2));
        ok_eq_hex(Status, STATUS_SUCCESS);
        /* Give system some time to process the change and notify our watch thread */
        KeDelayExecutionThread(KernelMode, FALSE, &WaitTimeout);
        /* Verify that the thread is notified */
        Status = KeWaitForSingleObject(WatchThreadObject, Executive, KernelMode, FALSE, &WaitTimeout);
        ok_eq_hex(Status, STATUS_WAIT_0);
        /* Verify thread's return value */
        ok_eq_hex(WatchThreadState.Status, STATUS_NOTIFY_ENUM_DIR);
        ok_eq_hex(WatchThreadState.IoStatusBlock.Status, STATUS_NOTIFY_ENUM_DIR);
        /* cleanup */
        ObDereferenceObject(WatchThreadObject);
        WatchThreadObject = NULL;
        ObCloseHandle(WatchThreadHandle, KernelMode);
        WatchThreadHandle = NULL;
        IoStatusBlock.Status = 0xdeadbeef;
    }
    else
    {
        skip(FALSE, "Failed to create thread");

        if (WatchThreadHandle)
        {
            ObCloseHandle(WatchThreadHandle, KernelMode);
        }
    }

    /* Event-based asynchronous wait mode */

    /* Initialize event */
    Status = ZwCreateEvent(&EventHandle, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);
    if (NT_SUCCESS(Status))
    {
        Status = ObReferenceObjectByHandle(EventHandle, EVENT_ALL_ACCESS, NULL, KernelMode, (PVOID*)&EventObject, NULL);
    }
    if (!NT_SUCCESS(Status))
    {
        skip(FALSE, "Failed to create event");
        /* All remaining tests depend on a KEVENT object */
        goto Cleanup;
    }
    /* Listen for notification */
    Status = ZwNotifyChangeKey(KeyHandle, EventHandle, NULL, NULL, &IoStatusBlock, REG_NOTIFY_CHANGE_LAST_SET, FALSE, NULL, 0, TRUE);
    ok_eq_hex(Status, STATUS_PENDING);
    /* Check event state */
    Status = KeWaitForSingleObject(EventObject, Executive, KernelMode, FALSE, &WaitTimeout);
    ok_eq_hex(Status, STATUS_TIMEOUT);
    /* Make change to the registry key */
    Status = ZwSetValueKey(KeyHandle, &ValueName, 0, REG_DWORD, &Value1, sizeof(Value1));
    ok_eq_hex(Status, STATUS_SUCCESS);
    /* Verify that the event is signaled */
    Status = KeWaitForSingleObject(EventObject, Executive, KernelMode, FALSE, &WaitTimeout);
    ok_eq_hex(Status, STATUS_WAIT_0);
    /* cleanup */
    KeClearEvent(EventObject);
    IoStatusBlock.Status = 0xdeadbeef;

    /* Test pending notification mode */
    Status = ZwSetValueKey(KeyHandle, &ValueName, 0, REG_DWORD, &Value2, sizeof(Value2));
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = ZwNotifyChangeKey(KeyHandle, EventHandle, NULL, NULL, &IoStatusBlock, REG_NOTIFY_CHANGE_LAST_SET, FALSE, NULL, 0, TRUE);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = KeWaitForSingleObject(EventObject, Executive, KernelMode, FALSE, &WaitTimeout);
    ok_eq_hex(Status, STATUS_WAIT_0);
    /* cleanup */
    KeClearEvent(EventObject);
    IoStatusBlock.Status = 0xdeadbeef;

    /* WorkQueueItem-based asynchronous wait mode */

    /* Initialize Work Queue Item */
    ExInitializeWorkItem(&WorkQueueItem, ZwNotifyChangeKey_ItemWorker, EventObject);
    /* Listen for notification */
    Status = ZwNotifyChangeKey(KeyHandle, NULL, (PVOID)&WorkQueueItem, (PVOID)DelayedWorkQueue, &IoStatusBlock, REG_NOTIFY_CHANGE_LAST_SET, FALSE, NULL, 0, TRUE);
    ok_eq_hex(Status, STATUS_PENDING);
    /* Check event state */
    Status = KeWaitForSingleObject(EventObject, Executive, KernelMode, FALSE, &WaitTimeout);
    ok_eq_hex(Status, STATUS_TIMEOUT);
    /* Make change to the registry key */
    Status = ZwSetValueKey(KeyHandle, &ValueName, 0, REG_DWORD, &Value1, sizeof(Value1));
    ok_eq_hex(Status, STATUS_SUCCESS);
    /* Verify that the event is signaled */
    Status = KeWaitForSingleObject(EventObject, Executive, KernelMode, FALSE, &WaitTimeout);
    ok_eq_hex(Status, STATUS_WAIT_0);

Cleanup:
    if (EventObject)
    {
        ObDereferenceObject(EventObject);
    }
    if (EventHandle)
    {
        ObCloseHandle(EventHandle, KernelMode);
    }
    /* Cleanup keys created for the test */
    ZwDeleteKey(KeyHandle);
    ObCloseHandle(KeyHandle, KernelMode);
}
