/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite, Registry change notification
 * PROGRAMMER:      Mohammad Amin Mollazadeh <madamin@pm.me>
 */

#include <kmt_test.h>

#define ok_ntstatus(svar, sexpected) ok_eq_hex(svar, sexpected)

typedef struct _WATCH_THREAD_DATA
{
    NTSTATUS Status;
    HANDLE KeyHandle;
} WATCH_THREAD_DATA, *PWATCH_THREAD_DATA;

typedef struct _WORK_QUEUE_CONTEXT
{
    HANDLE EventHandle;
    PKEVENT EventObject;
} WORK_QUEUE_CONTEXT, *PWORK_QUEUE_CONTEXT;

_Function_class_(KSTART_ROUTINE)
VOID NTAPI ZwNotifyChangeKey_WatchThread(_In_ PVOID lpParameter)
{
    NTSTATUS Status;
    PWATCH_THREAD_DATA WatchThreadData = (PWATCH_THREAD_DATA)lpParameter;
    IO_STATUS_BLOCK IoStatusBlock;
    
    Status = ZwNotifyChangeKey(WatchThreadData->KeyHandle, NULL, NULL, NULL, &IoStatusBlock, REG_NOTIFY_CHANGE_LAST_SET, FALSE, NULL, 0, FALSE);
    WatchThreadData->Status = Status;
}

_Function_class_(WORKER_THREAD_ROUTINE)
VOID NTAPI ZwNotifyChangeKey_ItemWorker(_In_ PVOID pParameter)
{
    PWORK_QUEUE_CONTEXT ctx = (PWORK_QUEUE_CONTEXT)pParameter;

    /* Signal that we received a notification */
    KeSetEvent(ctx->EventObject, 1, FALSE);
}

START_TEST(ZwNotifyChangeKey)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    
    /* Key creation */
    HANDLE KeyHandle;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;

    RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\SOFTWARE\\TestKey");

    /* Key value */
    UNICODE_STRING ValueName;
    DWORD32 ValueData1 = 0x12345678;
    DWORD32 ValueData2 = 0x87654321;

    RtlInitUnicodeString(&ValueName, L"TestValue");

    /* Thread */
    LARGE_INTEGER timeout;
    HANDLE WatchThreadHandle;
    PKTHREAD WatchThreadObject;
    WATCH_THREAD_DATA WatchThreadData;

    /* Event */
    HANDLE EventHandle;
    PKEVENT EventObject;

    /* Work queue item */
    WORK_QUEUE_ITEM WorkQueueItem;
    WORK_QUEUE_CONTEXT WorkQueueContext;

    /* Create a registry key to watch */

    InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    Status = ZwCreateKey(&KeyHandle, KEY_READ, &ObjectAttributes, 0, NULL, REG_OPTION_NON_VOLATILE, NULL);
    ok_ntstatus(Status, STATUS_SUCCESS);

    ObCloseHandle(KeyHandle, KernelMode);

    Status = ZwOpenKey(&KeyHandle, KEY_SET_VALUE | KEY_NOTIFY | DELETE, &ObjectAttributes);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* 
     * Test synchronous mode, single registry key, not watching subtree 
     */
    
    /* Create a thread */
    WatchThreadData.KeyHandle = KeyHandle;
    Status = PsCreateSystemThread(&WatchThreadHandle, THREAD_ALL_ACCESS, NULL, NULL, NULL, ZwNotifyChangeKey_WatchThread, &WatchThreadData);
    ok_ntstatus(Status, STATUS_SUCCESS);

    Status = ObReferenceObjectByHandle(WatchThreadHandle, THREAD_ALL_ACCESS, NULL, KernelMode, (PVOID*)&WatchThreadObject, NULL);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Verify the thread is still running */
    timeout.QuadPart = -1 * 100 * 1000 * 10;
    Status = KeWaitForSingleObject(WatchThreadObject, Executive, KernelMode, FALSE, &timeout);
    ok_ntstatus(Status, STATUS_TIMEOUT);

    /* Make change to the registry key */
    Status = ZwSetValueKey(KeyHandle, &ValueName, 0, REG_DWORD, &ValueData1, sizeof(ValueData1));
    ok_ntstatus(Status, STATUS_SUCCESS);
    /* Give system some time to process the change and notify our watch thread */
    KeDelayExecutionThread(KernelMode, FALSE, &timeout);
    Status = ZwSetValueKey(KeyHandle, &ValueName, 0, REG_DWORD, &ValueData2, sizeof(ValueData2));
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Verify that the thread is notified */
    Status = KeWaitForSingleObject(WatchThreadObject, Executive, KernelMode, FALSE, &timeout);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Verify thread's return value */
    ok_ntstatus(WatchThreadData.Status, STATUS_NOTIFY_ENUM_DIR);

    ObDereferenceObject(WatchThreadObject);
    ObCloseHandle(WatchThreadHandle, KernelMode);
    ObCloseHandle(KeyHandle, KernelMode);

    /*
     * Test asynchronous mode, events, single registry key, not watching subtree
     */

    Status = ZwOpenKey(&KeyHandle, KEY_SET_VALUE | KEY_NOTIFY | DELETE, &ObjectAttributes);
    ok_ntstatus(Status, STATUS_SUCCESS);
    
    /* Create event */
    Status = ZwCreateEvent(&EventHandle, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);
    ok_ntstatus(Status, STATUS_SUCCESS);

    Status = ObReferenceObjectByHandle(EventHandle, EVENT_ALL_ACCESS, NULL, KernelMode, (PVOID*)&EventObject, NULL);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Start watching for changes */
    Status = ZwNotifyChangeKey(KeyHandle, EventHandle, NULL, NULL, &IoStatusBlock, REG_NOTIFY_CHANGE_LAST_SET, FALSE, NULL, 0, TRUE);
    ok_ntstatus(Status, STATUS_PENDING);

    /* Check event state */
    Status = KeWaitForSingleObject(EventObject, Executive, KernelMode, FALSE, &timeout);
    ok_ntstatus(Status, STATUS_TIMEOUT);

    /* Make change to the registry key */
    Status = ZwSetValueKey(KeyHandle, &ValueName, 0, REG_DWORD, &ValueData1, sizeof(ValueData1));
    ok_ntstatus(Status, STATUS_SUCCESS);
    KeDelayExecutionThread(KernelMode, FALSE, &timeout);

    /* Verify that the event is signaled */
    Status = KeWaitForSingleObject(EventObject, Executive, KernelMode, FALSE, &timeout);
    ok_ntstatus(Status, STATUS_WAIT_0);

    /* Close event */
    ObDereferenceObject(EventObject);
    ObCloseHandle(EventHandle, KernelMode);
    ObCloseHandle(KeyHandle, KernelMode);

    /*
     * Test asynchronous mode, events, multiple registry keys, and watching subtree
     */

    /* TODO: implement */

    /*
     * Test asynchronous mode with WorkQueueItem
     */

    Status = ZwOpenKey(&KeyHandle, KEY_SET_VALUE | KEY_NOTIFY | DELETE, &ObjectAttributes);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Create event */
    Status = ZwCreateEvent(&EventHandle, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);
    ok_ntstatus(Status, STATUS_SUCCESS);

    Status = ObReferenceObjectByHandle(EventHandle, EVENT_ALL_ACCESS, NULL, KernelMode, (PVOID*)&EventObject, NULL);
    ok_ntstatus(Status, STATUS_SUCCESS);

    WorkQueueContext.EventHandle = EventHandle;
    WorkQueueContext.EventObject= EventObject;
    
    /* Initialize Work Queue Item */
    ExInitializeWorkItem(&WorkQueueItem, ZwNotifyChangeKey_ItemWorker, &WorkQueueContext);

    /* Listen for notification */
    Status = ZwNotifyChangeKey(KeyHandle, NULL, (PVOID)&WorkQueueItem, (PVOID)DelayedWorkQueue, &IoStatusBlock, REG_NOTIFY_CHANGE_LAST_SET, TRUE, NULL, 0, TRUE);
    ok_ntstatus(Status, STATUS_PENDING);

    /* Check event state */
    Status = KeWaitForSingleObject(EventObject, Executive, KernelMode, FALSE, &timeout);
    ok_ntstatus(Status, STATUS_TIMEOUT);

    /* Make change to the registry key */
    Status = ZwSetValueKey(KeyHandle, &ValueName, 0, REG_DWORD, &ValueData1, sizeof(ValueData1));
    ok_ntstatus(Status, STATUS_SUCCESS);
    /* Give system some time to process the change and notify our watch thread */
    KeDelayExecutionThread(KernelMode, FALSE, &timeout);
    Status = ZwSetValueKey(KeyHandle, &ValueName, 0, REG_DWORD, &ValueData2, sizeof(ValueData2));
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Verify that the event is signaled */
    Status = KeWaitForSingleObject(EventObject, Executive, KernelMode, FALSE, &timeout);
    ok_ntstatus(Status, STATUS_WAIT_0);

    ObDereferenceObject(EventObject);
    ObCloseHandle(EventHandle, KernelMode);

    /*
     * Cleanup
     */

    ZwDeleteKey(KeyHandle);
    ObCloseHandle(KeyHandle, KernelMode);
}
