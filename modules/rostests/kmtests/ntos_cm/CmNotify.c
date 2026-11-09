/*
 * PROJECT:     ReactOS Kernel-Mode tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for registry change notification
 * COPYRIGHT:   Copyright 2026 Mohammad Amin Mollazadeh <madamin@pm.me>
 */

#include <kmt_test.h>

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
    int i;
    /* Registry key handles and values */
    HANDLE KeyHandle = NULL;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING ValueName;
    DWORD32 Value1 = 0x12345678;
    DWORD32 Value2 = 0x87654321;
    /* Thread/Event/WorkQueueItem-related objects and data */
    LARGE_INTEGER WaitTimeout;
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

    /* Set a wait timeout of 1 microsecond
     * -1 means relative time, unit is 100 nanoseconds */
    WaitTimeout.QuadPart = -10;

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
        goto Cleanup;
    }
    /* Listen for notification */
    Status = ZwNotifyChangeKey(KeyHandle, EventHandle, NULL, NULL, &IoStatusBlock, REG_NOTIFY_CHANGE_LAST_SET, FALSE, NULL, 0, TRUE);
    ok_eq_hex(Status, STATUS_PENDING);
    /* Check event state */
    Status = KeWaitForSingleObject(EventObject, Executive, KernelMode, FALSE, &WaitTimeout);
    ok_eq_hex(Status, STATUS_TIMEOUT);
    /* Make change to the registry key */
    Status = ZwSetValueKey(KeyHandle, &ValueName, 0, REG_DWORD, &Value2, sizeof(Value2));
    ok_eq_hex(Status, STATUS_SUCCESS);
    /* Verify that the event is signaled */
    for (i = 0; i < 10; i++)
    {
        Status = KeWaitForSingleObject(EventObject, Executive, KernelMode, FALSE, &WaitTimeout);
        if (Status != STATUS_TIMEOUT)
            break;
    }
    ok_eq_hex(Status, STATUS_WAIT_0);
    /* cleanup */
    KeClearEvent(EventObject);
    IoStatusBlock.Status = 0xdeadbeef;

    /* Test pending notification mode */
    Status = ZwSetValueKey(KeyHandle, &ValueName, 0, REG_DWORD, &Value1, sizeof(Value1));
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = ZwNotifyChangeKey(KeyHandle, EventHandle, NULL, NULL, &IoStatusBlock, REG_NOTIFY_CHANGE_LAST_SET, FALSE, NULL, 0, TRUE);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = KeWaitForSingleObject(EventObject, Executive, KernelMode, FALSE, &WaitTimeout);
    ok_eq_hex(Status, STATUS_WAIT_0);
    /* cleanup */
    ObDereferenceObject(EventObject);
    EventObject = NULL;
    ZwClose(EventHandle);
    EventHandle = NULL;
    IoStatusBlock.Status = 0xdeadbeef;

    /* WorkQueueItem-based asynchronous wait mode */

    Status = ZwCreateEvent(&EventHandle, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);
    if (NT_SUCCESS(Status))
    {
        Status = ObReferenceObjectByHandle(EventHandle, EVENT_ALL_ACCESS, NULL, KernelMode, (PVOID*)&EventObject, NULL);
    }
    if (!NT_SUCCESS(Status))
    {
        skip(FALSE, "Failed to create event");
        goto Cleanup;
    }
    /* Initialize Work Queue Item */
    ExInitializeWorkItem(&WorkQueueItem, ZwNotifyChangeKey_ItemWorker, EventObject);
    /* Listen for notification */
    Status = ZwNotifyChangeKey(KeyHandle, NULL, (PVOID)&WorkQueueItem, (PVOID)DelayedWorkQueue, &IoStatusBlock, REG_NOTIFY_CHANGE_LAST_SET, FALSE, NULL, 0, TRUE);
    ok_eq_hex(Status, STATUS_PENDING);
    /* Check event state */
    Status = KeWaitForSingleObject(EventObject, Executive, KernelMode, FALSE, &WaitTimeout);
    ok_eq_hex(Status, STATUS_TIMEOUT);
    /* Make change to the registry key */
    Status = ZwSetValueKey(KeyHandle, &ValueName, 0, REG_DWORD, &Value2, sizeof(Value2));
    ok_eq_hex(Status, STATUS_SUCCESS);
    /* Verify that the event is signaled */
    for (i = 0; i < 10; i++)
    {
        Status = KeWaitForSingleObject(EventObject, Executive, KernelMode, FALSE, &WaitTimeout);
        if (Status != STATUS_TIMEOUT)
            break;
    }
    ok_eq_hex(Status, STATUS_WAIT_0);

Cleanup:
    if (EventObject)
    {
        ObDereferenceObject(EventObject);
        EventObject = NULL;
    }
    if (EventHandle)
    {
        ObCloseHandle(EventHandle, KernelMode);
        EventHandle = NULL;
    }
    /* Cleanup keys created for the test */
    ZwDeleteKey(KeyHandle);
    ObCloseHandle(KeyHandle, KernelMode);
    KeyHandle = NULL;
}
