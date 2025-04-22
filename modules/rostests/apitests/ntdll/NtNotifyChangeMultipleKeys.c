/*
 * PROJECT:          ReactOS api tests
 * LICENSE:          See COPYING in the top level directory
 * PURPOSE:          Test for NtNotifyChangeMultipleKeys
 * PROGRAMMER:       Mohammad Amin Mollazadeh (madamin@pm.me)
 */

#include "precomp.h"
#include "winreg.h"

typedef struct _WATCH_THREAD_DATA
{
    HANDLE KeyHandle;
} WATCH_THREAD_DATA, *PWATCH_THREAD_DATA;

DWORD WINAPI NtNotifyChangeMultipleKeys_WatchThread(LPVOID lpParameter)
{
    NTSTATUS Status;
    PWATCH_THREAD_DATA WatchThreadData = (PWATCH_THREAD_DATA)lpParameter;
    IO_STATUS_BLOCK IoStatusBlock;
    
    Status = NtNotifyChangeMultipleKeys(WatchThreadData->KeyHandle, 0, NULL, NULL, NULL, NULL, &IoStatusBlock, REG_NOTIFY_CHANGE_LAST_SET, FALSE, NULL, 0, FALSE);
    ok_ntstatus(Status, STATUS_NOTIFY_ENUM_DIR);

    return 0;
}

START_TEST(NtNotifyChangeMultipleKeys)
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
    DWORD ValueData1 = 0x12345678;
    DWORD ValueData2 = 0x87654321;

    RtlInitUnicodeString(&ValueName, L"TestValue");

    /* Thread */
    HANDLE WatchThreadHandle;
    WATCH_THREAD_DATA WatchThreadData;

    /* Event */
    HANDLE EventHandle;

    /* Create a registry key to watch */

    InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    Status = NtCreateKey(&KeyHandle, KEY_READ, &ObjectAttributes, 0, NULL, REG_OPTION_NON_VOLATILE, NULL);
    ok_ntstatus(Status, STATUS_SUCCESS);

    CloseHandle(KeyHandle);

    Status = NtCreateKey(&KeyHandle, KEY_SET_VALUE | KEY_NOTIFY | DELETE, &ObjectAttributes, 0, NULL, REG_OPTION_NON_VOLATILE, NULL);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* 
     * Test synchronous mode, single registry key, not watching subtree 
     */
    
    /* Create a thread */
    WatchThreadData.KeyHandle = KeyHandle;
    WatchThreadHandle = CreateThread(NULL, 0, NtNotifyChangeMultipleKeys_WatchThread, &WatchThreadData, 0, NULL);
    ok(WatchThreadHandle != NULL, "Failed to create thread\n");

    /* Verify the thread is still running */
    Status = WaitForSingleObject(WatchThreadHandle, 0);
    ok_ntstatus(Status, WAIT_TIMEOUT);

    /* Make change to the registry key */
    Status = NtSetValueKey(KeyHandle, &ValueName, 0, REG_DWORD, &ValueData1, sizeof(ValueData1));
    ok_ntstatus(Status, STATUS_SUCCESS);
    /* Give system some time to process the change and notify our watch thread */
    SleepEx(100, TRUE);
    Status = NtSetValueKey(KeyHandle, &ValueName, 0, REG_DWORD, &ValueData2, sizeof(ValueData2));
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Verify that the thread is notified */
    Status = WaitForSingleObjectEx(WatchThreadHandle, 100, TRUE);
    ok_ntstatus(Status, WAIT_OBJECT_0);

    /*
     * Test asynchronous mode, events, single registry key, not watching subtree
     */
    
    /* Create event */
    EventHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
    ok(EventHandle != NULL, "Failed to create event\n");

    /* Start watching for changes */
    Status = NtNotifyChangeMultipleKeys(KeyHandle, 0, NULL, EventHandle, NULL, NULL, &IoStatusBlock, REG_NOTIFY_CHANGE_LAST_SET, FALSE, NULL, 0, TRUE);
    ok_ntstatus(Status, STATUS_PENDING);

    /* Check event state */
    Status = WaitForSingleObject(EventHandle, 0);
    ok_ntstatus(Status, WAIT_TIMEOUT);

    /* Make change to the registry key */
    Status = NtSetValueKey(KeyHandle, &ValueName, 0, REG_DWORD, &ValueData1, sizeof(ValueData1));
    ok_ntstatus(Status, STATUS_SUCCESS);
    SleepEx(100, TRUE);

    /* Verify that the event is signaled */
    Status = WaitForSingleObjectEx(EventHandle, 100, TRUE);
    ok_ntstatus(Status, WAIT_OBJECT_0);

    /* Check event state */
    Status = WaitForSingleObject(EventHandle, 0);
    ok_ntstatus(Status, WAIT_TIMEOUT);

    /* Close event */
    CloseHandle(EventHandle);

    /*
     * Test asynchronous mode, events, multiple registry keys, and watching subtree
     */

    /* TODO: implement */

    /*
     * Test asynchronous mode with APC routine
     */

    /* TODO: implement */

    /*
     * Cleanup
     */

    NtDeleteKey(KeyHandle);
    CloseHandle(KeyHandle);
    CloseHandle(WatchThreadHandle);
}