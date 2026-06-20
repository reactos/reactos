/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for NtNotifyChangeMultipleKeys
 * COPYRIGHT:   Copyright 2026 Mohammad Amin Mollazadeh <madamin@pm.me>
 */

#include "precomp.h"
#include "winreg.h"

/* Registry watcher thread for testing synchronous mode */
typedef struct _WATCH_THREAD_STATE
{
    NTSTATUS Status;
    HANDLE KeyHandle;
    PIO_STATUS_BLOCK IoStatusBlock;
} WATCH_THREAD_STATE, *PWATCH_THREAD_STATE;

DWORD WINAPI NtNotifyChangeMultipleKeys_WatchThread(LPVOID lpParameter)
{
    PWATCH_THREAD_STATE State = (PWATCH_THREAD_STATE)lpParameter;
    
    State->Status = NtNotifyChangeMultipleKeys(State->KeyHandle, 0, NULL, NULL, NULL, NULL, State->IoStatusBlock, REG_NOTIFY_CHANGE_LAST_SET, FALSE, NULL, 0, FALSE);

    return 0;
}

/* APC Routine for testing asynchronous mode */
VOID WINAPI NtNotifyChangeMultipleKeys_ApcRoutine(PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, ULONG Reserved)
{
    UNREFERENCED_PARAMETER(IoStatusBlock);
    UNREFERENCED_PARAMETER(Reserved);

    BOOLEAN* ApcRan = (BOOLEAN*)ApcContext;
    *ApcRan = TRUE;
}

START_TEST(NtNotifyChangeMultipleKeys)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES SubordinateObjects[1];
    WATCH_THREAD_STATE WatchThreadState;
    BOOLEAN ApcRan = FALSE;
    /* Registry key object attributes */
    UNICODE_STRING KeyName, SubKeyName, SecondaryKeyName, ThirdKeyName;
    UNICODE_STRING ValueName;
    OBJECT_ATTRIBUTES ObjectAttributes, SubKeyObjectAttributes, SecondaryObjectAttributes, ThirdObjectAttributes;
    DWORD Value1 = 0x12345678, Value2 = 0x87654321;
    /* handles */
    HANDLE KeyHandle = NULL, SubKeyHandle = NULL, SecondaryKeyHandle = NULL;
    HANDLE WatchThreadHandle = NULL, EventHandle = NULL;
    
    RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\SOFTWARE\\TestKey");
    RtlInitUnicodeString(&SubKeyName, L"\\Registry\\Machine\\SOFTWARE\\TestKey\\TestSubKey");
    RtlInitUnicodeString(&SecondaryKeyName, L"\\Registry\\User\\.DEFAULT\\SOFTWARE\\TestKey");
    RtlInitUnicodeString(&ThirdKeyName, L"\\Registry\\Machine\\Software\\Microsoft\\Windows");
    RtlInitUnicodeString(&ValueName, L"TestValue");
    InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    InitializeObjectAttributes(&SubKeyObjectAttributes, &SubKeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    InitializeObjectAttributes(&SecondaryObjectAttributes, &SecondaryKeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    InitializeObjectAttributes(&ThirdObjectAttributes, &ThirdKeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    /* Create event */
    EventHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (EventHandle == NULL)
    {
        skip(FALSE, "Failed to create event");
        return;
    }
    /* Create registry keys */
    Status = NtCreateKey(&KeyHandle, KEY_ALL_ACCESS, &ObjectAttributes, 0, NULL, REG_OPTION_NON_VOLATILE, NULL);
    if (!NT_SUCCESS(Status))
    {
        skip(FALSE, "Failed to create registry key");
        goto Cleanup;
    }
    NtSetValueKey(KeyHandle, &ValueName, 0, REG_DWORD, &Value1, sizeof(Value1));
    
    /* Synchronous mode */

    /* Create a thread */
    WatchThreadState.KeyHandle = KeyHandle;
    WatchThreadState.IoStatusBlock = &IoStatusBlock;
    WatchThreadHandle = CreateThread(NULL, 0, NtNotifyChangeMultipleKeys_WatchThread, &WatchThreadState, 0, NULL);
    if (WatchThreadHandle)
    {
        /* Verify the thread is still running */
        Status = WaitForSingleObject(WatchThreadHandle, 100);
        ok_ntstatus(Status, WAIT_TIMEOUT);
        /* Make change to the registry key */
        Status = NtSetValueKey(KeyHandle, &ValueName, 0, REG_DWORD, &Value2, sizeof(Value2));
        ok_ntstatus(Status, STATUS_SUCCESS);
        /* Verify that the thread is notified */
        Status = WaitForSingleObject(WatchThreadHandle, 100);
        ok_ntstatus(Status, WAIT_OBJECT_0);
        /* Verify the status code */
        ok_ntstatus(WatchThreadState.Status, STATUS_NOTIFY_ENUM_DIR);
        ok_ntstatus(WatchThreadState.IoStatusBlock->Status, STATUS_NOTIFY_ENUM_DIR);
        /* cleanup */
        CloseHandle(WatchThreadHandle);
        WatchThreadHandle = NULL;
        WatchThreadState.Status = 0xdeadbeef;
        WatchThreadState.IoStatusBlock->Status = 0xdeadbeef;
    }
    else
    {
        skip(FALSE, "Failed to create watch thread");
    }
    /* Watch again, but this time close the handle without making any change */
    WatchThreadHandle = CreateThread(NULL, 0, NtNotifyChangeMultipleKeys_WatchThread, &WatchThreadState, 0, NULL);
    if (WatchThreadHandle)
    {
        Status = WaitForSingleObject(WatchThreadHandle, 100);
        ok_ntstatus(Status, WAIT_TIMEOUT);
        NtClose(KeyHandle);
        KeyHandle = NULL;
        Status = WaitForSingleObject(WatchThreadHandle, 100);
        ok_ntstatus(Status, WAIT_OBJECT_0);
        ok_ntstatus(WatchThreadState.Status, STATUS_NOTIFY_CLEANUP);
        ok_ntstatus(WatchThreadState.IoStatusBlock->Status, STATUS_NOTIFY_CLEANUP);
        /* cleanup */
        CloseHandle(WatchThreadHandle);
        WatchThreadHandle = NULL;
    }
    else
    {
        skip(FALSE, "Failed to create watch thread");
    }

    /* Event-based asynchronous mode */

    if (!KeyHandle)
    {
        Status = NtOpenKey(&KeyHandle, KEY_ALL_ACCESS, &ObjectAttributes);
        if (!NT_SUCCESS(Status))
        {
            skip(FALSE, "Failed to open registry key");
            goto Cleanup;
        }
    }
    /* Start watching for changes */
    Status = NtNotifyChangeMultipleKeys(KeyHandle, 0, NULL, EventHandle, NULL, NULL, &IoStatusBlock, REG_NOTIFY_CHANGE_LAST_SET, FALSE, NULL, 0, TRUE);
    ok_ntstatus(Status, STATUS_PENDING);
    /* Check event state */
    Status = WaitForSingleObject(EventHandle, 0);
    ok_ntstatus(Status, WAIT_TIMEOUT);
    /* Make change to the registry key */
    Status = NtSetValueKey(KeyHandle, &ValueName, 0, REG_DWORD, &Value1, sizeof(Value1));
    ok_ntstatus(Status, STATUS_SUCCESS);
    /* Verify that the event is signaled */
    NtTestAlert();
    Status = WaitForSingleObject(EventHandle, 100);
    ok_ntstatus(Status, WAIT_OBJECT_0);
    /* Verify the status code */
    ok_ntstatus(IoStatusBlock.Status, STATUS_NOTIFY_ENUM_DIR);

    /* Watch again, but this time close the handle without making any change */
    Status = NtNotifyChangeMultipleKeys(KeyHandle, 0, NULL, EventHandle, NULL, NULL, &IoStatusBlock, REG_NOTIFY_CHANGE_LAST_SET, FALSE, NULL, 0, TRUE);
    ok_ntstatus(Status, STATUS_PENDING);
    Status = WaitForSingleObject(EventHandle, 0);
    ok_ntstatus(Status, WAIT_TIMEOUT);
    NtClose(KeyHandle);
    KeyHandle = NULL;
    NtTestAlert();
    Status = WaitForSingleObject(EventHandle, 100);
    ok_ntstatus(Status, WAIT_OBJECT_0);
    ok_ntstatus(IoStatusBlock.Status, STATUS_NOTIFY_CLEANUP);

    /* Watching subtree */

    if (!KeyHandle)
    {
        Status = NtOpenKey(&KeyHandle, KEY_ALL_ACCESS, &ObjectAttributes);
        if (!NT_SUCCESS(Status))
        {
            skip(FALSE, "Failed to open registry key");
            goto Cleanup;
        }
    }
    Status = NtCreateKey(&SubKeyHandle, KEY_ALL_ACCESS, &SubKeyObjectAttributes, 0, NULL, REG_OPTION_NON_VOLATILE, NULL);
    if (NT_SUCCESS(Status))
    {
        /* Start watching for changes */
        Status = NtNotifyChangeMultipleKeys(KeyHandle, 0, NULL, EventHandle, NULL, NULL, &IoStatusBlock, REG_NOTIFY_CHANGE_LAST_SET, TRUE, NULL, 0, TRUE);
        ok_ntstatus(Status, STATUS_PENDING);
        /* Check event state */
        Status = WaitForSingleObject(EventHandle, 0);
        ok_ntstatus(Status, WAIT_TIMEOUT);
        /* Make change to the subkey */
        Status = NtSetValueKey(SubKeyHandle, &ValueName, 0, REG_DWORD, &Value1, sizeof(Value1));
        ok_ntstatus(Status, STATUS_SUCCESS);
        /* Verify that the event is signaled */
        NtTestAlert();
        Status = WaitForSingleObject(EventHandle, 100);
        ok_ntstatus(Status, WAIT_OBJECT_0);
    }
    else
    {
        skip(FALSE, "Failed to create subkey");
    }

    /* Watching multiple keys */

    Status = NtCreateKey(&SecondaryKeyHandle, KEY_ALL_ACCESS, &SecondaryObjectAttributes, 0, NULL, REG_OPTION_NON_VOLATILE, NULL);
    if (NT_SUCCESS(Status))
    {
        CloseHandle(SecondaryKeyHandle);
        InitializeObjectAttributes(&SubordinateObjects[0], &SecondaryKeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);
        Status = NtNotifyChangeMultipleKeys(KeyHandle,
                                            _countof(SubordinateObjects),
                                            SubordinateObjects,
                                            EventHandle,
                                            NULL,
                                            NULL,
                                            &IoStatusBlock,
                                            REG_NOTIFY_CHANGE_LAST_SET,
                                            TRUE,
                                            NULL,
                                            0,
                                            TRUE);
        ok_ntstatus(Status, STATUS_PENDING);
        /* Check event state */
        Status = WaitForSingleObject(EventHandle, 0);
        ok_ntstatus(Status, WAIT_TIMEOUT);
        /* Make change to the secondary key */
        Status = NtOpenKey(&SecondaryKeyHandle, KEY_SET_VALUE | KEY_NOTIFY | DELETE, &SecondaryObjectAttributes);
        if (NT_SUCCESS(Status))
        {
            Status = NtSetValueKey(SecondaryKeyHandle, &ValueName, 0, REG_DWORD, &Value1, sizeof(Value1));
            ok_ntstatus(Status, STATUS_SUCCESS);
            CloseHandle(SecondaryKeyHandle);
        }
        /* Verify that the event is signaled */
        NtTestAlert();
        Status = WaitForSingleObject(EventHandle, 100);
        ok_ntstatus(Status, WAIT_OBJECT_0);
    }
    else
    {
        skip(FALSE, "Failed to create secondary key");
    }
    /* Test if the function fails if the given subordinate key is same with master key */
    InitializeObjectAttributes(&SubordinateObjects[0], &KeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtNotifyChangeMultipleKeys(KeyHandle,
                                        _countof(SubordinateObjects),
                                        SubordinateObjects,
                                        EventHandle,
                                        NULL,
                                        NULL,
                                        &IoStatusBlock,
                                        REG_NOTIFY_CHANGE_LAST_SET,
                                        TRUE,
                                        NULL,
                                        0,
                                        TRUE);
    /* The status is STATUS_INVALID_PARAMETER on Windows Server 2003 and STATUS_INVALID_OBJECT_NAME on Windows 10 */
    ok(!NT_SUCCESS(Status), "NtNotifyChangeMultipleKeys succeeded unexpectedly.\n");
    /* Test if the function fails if the the given subordinate key is in the same hive as the master key */
    InitializeObjectAttributes(&SubordinateObjects[0], &ThirdKeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtNotifyChangeMultipleKeys(KeyHandle,
                                        _countof(SubordinateObjects),
                                        SubordinateObjects,
                                        EventHandle,
                                        NULL,
                                        NULL,
                                        &IoStatusBlock,
                                        REG_NOTIFY_CHANGE_LAST_SET,
                                        TRUE,
                                        NULL,
                                        0,
                                        TRUE);
    ok(!NT_SUCCESS(Status), "NtNotifyChangeMultipleKeys succeeded unexpectedly.\n");
    
    /* APC-based asynchronous mode */
    Status = NtNotifyChangeMultipleKeys(KeyHandle, 0, NULL, NULL, NtNotifyChangeMultipleKeys_ApcRoutine, &ApcRan, &IoStatusBlock, REG_NOTIFY_CHANGE_LAST_SET, FALSE, NULL, 0, TRUE);
    ok_ntstatus(Status, STATUS_PENDING);
    /* Make change to the registry key */
    Status = NtSetValueKey(KeyHandle, &ValueName, 0, REG_DWORD, &Value2, sizeof(Value2));
    ok_ntstatus(Status, STATUS_SUCCESS);
    /* Force system to run queued APC routines */
    NtTestAlert();
    ok(ApcRan == TRUE, "The APC routine did not ran.\n");

Cleanup:
    if (WatchThreadHandle)
    {
        CloseHandle(WatchThreadHandle);
    }
    if (SubKeyHandle)
    {
        NtDeleteKey(SubKeyHandle);
        CloseHandle(SubKeyHandle);
    }
    if (!KeyHandle)
    {
        Status = NtOpenKey(&KeyHandle, DELETE, &ObjectAttributes);
        if (!NT_SUCCESS(Status))
        {
            KeyHandle = NULL;
        }
    }
    if (KeyHandle)
    {
        NtDeleteKey(KeyHandle);
        CloseHandle(KeyHandle);
    }
    Status = NtOpenKey(&SecondaryKeyHandle, DELETE, &SecondaryObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        NtDeleteKey(SecondaryKeyHandle);   
        CloseHandle(SecondaryKeyHandle);
    }
}
