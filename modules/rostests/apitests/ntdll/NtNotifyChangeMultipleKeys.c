/*
 * PROJECT:          ReactOS api tests
 * LICENSE:          See COPYING in the top level directory
 * PURPOSE:          Test for NtNotifyChangeMultipleKeys
 * PROGRAMMER:       Mohammad Amin Mollazadeh (madamin@pm.me)
 */

#include "precomp.h"
#include "winreg.h"

/* Test state holder */

typedef struct _CHANGE_NOTIFY_TEST_STATE
{
    /* Registry key handles */
    HANDLE KeyHandle;
    HANDLE SubKeyHandle;

    /* Registry key object attributes */
    UNICODE_STRING KeyName;
    UNICODE_STRING SubKeyName;
    UNICODE_STRING SecondaryKeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    OBJECT_ATTRIBUTES SubKeyObjectAttributes;
    OBJECT_ATTRIBUTES SecondaryObjectAttributes;

    /* Watcher thread */
    HANDLE ThreadHandle;

    /* Notification event handle */
    HANDLE EventHandle;

    /* IoStatusBlock for NtNotifyChangeMultipleKeys */
    IO_STATUS_BLOCK IoStatusBlock;

    /* A flag to check if ApcRoutine is ran */
    BOOLEAN ApcRan;
} CHANGE_NOTIFY_TEST_STATE, *PCHANGE_NOTIFY_TEST_STATE;

#define TState CHANGE_NOTIFY_TEST_STATE
#define PState PCHANGE_NOTIFY_TEST_STATE

/* Registry watcher thread for testing synchronous mode */

DWORD WINAPI NtNotifyChangeMultipleKeys_WatchThread(LPVOID lpParameter)
{
    NTSTATUS Status;
    HANDLE KeyHandle = (HANDLE)lpParameter;
    IO_STATUS_BLOCK IoStatusBlock;
    
    Status = NtNotifyChangeMultipleKeys(KeyHandle, 0, NULL, NULL, NULL, NULL, &IoStatusBlock, REG_NOTIFY_CHANGE_LAST_SET, FALSE, NULL, 0, FALSE);
    ok_ntstatus(Status, STATUS_NOTIFY_ENUM_DIR);

    return 0;
}

/* APC Routine for testing asynchronous mode */

VOID WINAPI NtNotifyChangeMultipleKeys_ApcRoutine(PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, ULONG Reserved)
{
    UNREFERENCED_PARAMETER(IoStatusBlock);
    UNREFERENCED_PARAMETER(Reserved);

    PState State = (PState)ApcContext;
    State->ApcRan = TRUE;
}

/* Helper functions */

#define CLEANUP(state) NtNotifyChangeMultipleKeys_Cleanup(state)
#define CHECK_ERROR(state, Status) if (!NT_SUCCESS(Status)) { ok(FALSE, "%d: Internal error, Status=0x%x\n", __LINE__, (unsigned int)Status); CLEANUP(state); return Status; }
#define TEST_ERROR(state, Status, Expected)  ok_ntstatus(Status, Expected); if (Status != Expected) { CLEANUP(state); return Status; }
#define INIT_TEST(state, Status) Status = NtNotifyChangeMultipleKeys_InitializePerTest(state); CHECK_ERROR(state, Status)
#define FINALIZE_TEST(state) CLEANUP(state); return STATUS_SUCCESS
#define RUN_TEST(state, Status, TestName) Status = NtNotifyChangeMultipleKeys_TEST_##TestName(state); ok(Status == STATUS_SUCCESS, "Subtest "#TestName" failed.\n")
#define START_SUBTEST(TestName) NTSTATUS WINAPI NtNotifyChangeMultipleKeys_TEST_##TestName(PState state)

VOID WINAPI NtNotifyChangeMultipleKeys_Cleanup(PState state)
{
    if (state->EventHandle)
    {
        CloseHandle(state->EventHandle);
        state->EventHandle = NULL;
    }
    if (state->ThreadHandle)
    {
        CloseHandle(state->ThreadHandle);
        state->ThreadHandle = NULL;
    }
    if (state->SubKeyHandle)
    {
        CloseHandle(state->SubKeyHandle);
        state->SubKeyHandle = NULL;
    }
    if (state->KeyHandle)
    {
        CloseHandle(state->KeyHandle);
        state->KeyHandle = NULL;
    }
}

NTSTATUS WINAPI NtNotifyChangeMultipleKeys_Initialize(PState state)
{
    NTSTATUS Status;

    HANDLE SecondaryKeyHandle;

    state->KeyHandle = NULL;
    state->SubKeyHandle = NULL;
    state->ThreadHandle = NULL;
    state->EventHandle = NULL;

    /* Setup object attributes */
    RtlInitUnicodeString(&state->KeyName, L"\\Registry\\Machine\\SOFTWARE\\TestKey");
    InitializeObjectAttributes(&state->ObjectAttributes, &state->KeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    RtlInitUnicodeString(&state->SubKeyName, L"\\Registry\\Machine\\SOFTWARE\\TestKey\\TestSubKey");
    InitializeObjectAttributes(&state->SubKeyObjectAttributes, &state->SubKeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    RtlInitUnicodeString(&state->SecondaryKeyName, L"\\Registry\\User\\.DEFAULT\\SOFTWARE\\TestKey");
    InitializeObjectAttributes(&state->SecondaryObjectAttributes, &state->SecondaryKeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    /* Create registry keys */
    Status = NtCreateKey(&state->KeyHandle, KEY_ALL_ACCESS, &state->ObjectAttributes, 0, NULL, REG_OPTION_NON_VOLATILE, NULL);
    CHECK_ERROR(state, Status);

    Status = NtCreateKey(&state->SubKeyHandle, KEY_ALL_ACCESS, &state->SubKeyObjectAttributes, 0, NULL, REG_OPTION_NON_VOLATILE, NULL);
    CHECK_ERROR(state, Status);

    Status = NtCreateKey(&SecondaryKeyHandle, KEY_ALL_ACCESS, &state->SecondaryObjectAttributes, 0, NULL, REG_OPTION_NON_VOLATILE, NULL);
    CHECK_ERROR(state, Status);
    CloseHandle(SecondaryKeyHandle);

    FINALIZE_TEST(state);
}

NTSTATUS WINAPI NtNotifyChangeMultipleKeys_CleanupTestKeys(PState state)
{
    NTSTATUS Status;
    HANDLE SecondaryKey;

    Status = NtOpenKey(&state->KeyHandle, DELETE, &state->ObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        NtDeleteKey(state->KeyHandle);
        CloseHandle(state->KeyHandle);
    }

    Status = NtOpenKey(&SecondaryKey, DELETE, &state->SecondaryObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        NtDeleteKey(SecondaryKey);   
        CloseHandle(SecondaryKey);
    }

    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtNotifyChangeMultipleKeys_InitializePerTest(PState state)
{
    NTSTATUS Status;

    Status = NtOpenKey(&state->KeyHandle, KEY_SET_VALUE | KEY_NOTIFY | DELETE, &state->ObjectAttributes);
    CHECK_ERROR(state, Status);

    Status = NtOpenKey(&state->SubKeyHandle, KEY_SET_VALUE | KEY_NOTIFY | DELETE, &state->SubKeyObjectAttributes);
    CHECK_ERROR(state, Status);

    return STATUS_SUCCESS;
}

/* Tests */

START_SUBTEST(Synchronous)
{
    NTSTATUS Status;

    UNICODE_STRING ValueName;
    RtlInitUnicodeString(&ValueName, L"TestValue");

    DWORD ValueData1 = 0x12345678;
    DWORD ValueData2 = 0x87654321;

    INIT_TEST(state, Status);
    
    /* Create a thread */
    state->ThreadHandle = CreateThread(NULL, 0, NtNotifyChangeMultipleKeys_WatchThread, state->KeyHandle, 0, NULL);
    if (!state->ThreadHandle)
    {
        CLEANUP(state);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Verify the thread is still running */
    Status = WaitForSingleObject(state->ThreadHandle, 100);
    TEST_ERROR(state, Status, WAIT_TIMEOUT);

    /* Make change to the registry key */
    Status = NtSetValueKey(state->KeyHandle, &ValueName, 0, REG_DWORD, &ValueData1, sizeof(ValueData1));
    CHECK_ERROR(state, Status);
    /* Give system some time to process the change and notify our watch thread */
    SleepEx(100, TRUE);
    Status = NtSetValueKey(state->KeyHandle, &ValueName, 0, REG_DWORD, &ValueData2, sizeof(ValueData2));
    CHECK_ERROR(state, Status);

    /* Verify that the thread is notified */
    Status = WaitForSingleObjectEx(state->ThreadHandle, 100, TRUE);
    TEST_ERROR(state, Status, WAIT_OBJECT_0);

    FINALIZE_TEST(state);
}

START_SUBTEST(Asynchronous)
{
    NTSTATUS Status;

    UNICODE_STRING ValueName;
    RtlInitUnicodeString(&ValueName, L"AsyncTestValue");

    DWORD ValueData1 = 0x12345678;
    DWORD ValueData2 = 0x87654321;

    INIT_TEST(state, Status);

    /* Create event */
    state->EventHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
    ok(state->EventHandle != NULL, "Failed to create event\n");

    /* Start watching for changes */
    Status = NtNotifyChangeMultipleKeys(state->KeyHandle, 0, NULL, state->EventHandle, NULL, NULL, &state->IoStatusBlock, REG_NOTIFY_CHANGE_LAST_SET, FALSE, NULL, 0, TRUE);
    TEST_ERROR(state, Status, STATUS_PENDING);

    /* Check event state */
    Status = WaitForSingleObject(state->EventHandle, 0);
    TEST_ERROR(state, Status, WAIT_TIMEOUT);

    /* Make change to the registry key */
    Status = NtSetValueKey(state->KeyHandle, &ValueName, 0, REG_DWORD, &ValueData1, sizeof(ValueData1));
    TEST_ERROR(state, Status, STATUS_SUCCESS);
    SleepEx(100, TRUE);
    Status = NtSetValueKey(state->KeyHandle, &ValueName, 0, REG_DWORD, &ValueData2, sizeof(ValueData2));
    TEST_ERROR(state, Status, STATUS_SUCCESS);

    /* Verify that the event is signaled */
    Status = WaitForSingleObjectEx(state->EventHandle, 100, TRUE);
    TEST_ERROR(state, Status, WAIT_OBJECT_0);

    FINALIZE_TEST(state);
}

START_SUBTEST(WatchSubtree)
{
    NTSTATUS Status;

    UNICODE_STRING ValueName;
    RtlInitUnicodeString(&ValueName, L"SubtreeTestValue");

    DWORD ValueData1 = 0x12345678;
    DWORD ValueData2 = 0x87654321;

    INIT_TEST(state, Status);

    /* Create event */
    state->EventHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
    ok(state->EventHandle != NULL, "Failed to create event\n");

    /* Start watching for changes */
    Status = NtNotifyChangeMultipleKeys(state->KeyHandle, 0, NULL, state->EventHandle, NULL, NULL, &state->IoStatusBlock, REG_NOTIFY_CHANGE_LAST_SET, TRUE, NULL, 0, TRUE);
    TEST_ERROR(state, Status, STATUS_PENDING);

    /* Check event state */
    Status = WaitForSingleObject(state->EventHandle, 0);
    TEST_ERROR(state, Status, WAIT_TIMEOUT);

    /* Make change to the subkey */
    Status = NtSetValueKey(state->SubKeyHandle, &ValueName, 0, REG_DWORD, &ValueData1, sizeof(ValueData1));
    TEST_ERROR(state, Status, STATUS_SUCCESS);
    SleepEx(100, TRUE);
    Status = NtSetValueKey(state->SubKeyHandle, &ValueName, 0, REG_DWORD, &ValueData2, sizeof(ValueData2));
    TEST_ERROR(state, Status, STATUS_SUCCESS);

    /* Verify that the event is signaled */
    Status = WaitForSingleObjectEx(state->EventHandle, 100, TRUE);
    TEST_ERROR(state, Status, WAIT_OBJECT_0);

    FINALIZE_TEST(state);
}

START_SUBTEST(WatchSecondaryKey)
{
    NTSTATUS Status;

    UNICODE_STRING ValueName;
    RtlInitUnicodeString(&ValueName, L"SecondaryKeyTestValue");

    DWORD ValueData1 = 0x12345678;
    DWORD ValueData2 = 0x87654321;

    HANDLE SecondaryKey;

    INIT_TEST(state, Status);

    /* Create event */
    state->EventHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
    ok(state->EventHandle != NULL, "Failed to create event\n");

    /* Start watching for changes */
    OBJECT_ATTRIBUTES SubordinateObjects[1];
    InitializeObjectAttributes(&SubordinateObjects[0], &state->SecondaryKeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtNotifyChangeMultipleKeys(state->KeyHandle,
                                        _countof(SubordinateObjects),
                                        SubordinateObjects,
                                        state->EventHandle,
                                        NULL,
                                        NULL,
                                        &state->IoStatusBlock,
                                        REG_NOTIFY_CHANGE_LAST_SET,
                                        TRUE,
                                        NULL,
                                        0,
                                        TRUE);
    TEST_ERROR(state, Status, STATUS_PENDING);

    /* Check event state */
    Status = WaitForSingleObject(state->EventHandle, 0);
    TEST_ERROR(state, Status, WAIT_TIMEOUT);

    /* Make change to the secondary key */
    Status = NtOpenKey(&SecondaryKey, KEY_SET_VALUE | KEY_NOTIFY | DELETE, &state->SecondaryObjectAttributes);
    CHECK_ERROR(state, Status);
    Status = NtSetValueKey(SecondaryKey, &ValueName, 0, REG_DWORD, &ValueData1, sizeof(ValueData1));
    TEST_ERROR(state, Status, STATUS_SUCCESS);
    Status = NtSetValueKey(SecondaryKey, &ValueName, 0, REG_DWORD, &ValueData2, sizeof(ValueData2));
    TEST_ERROR(state, Status, STATUS_SUCCESS);
    CloseHandle(SecondaryKey);
    SleepEx(100, TRUE);

    /* Verify that the event is signaled */
    Status = WaitForSingleObjectEx(state->EventHandle, 100, TRUE);
    TEST_ERROR(state, Status, WAIT_OBJECT_0);

    FINALIZE_TEST(state);
}

START_SUBTEST(ApcRoutine)
{
    NTSTATUS Status;

    UNICODE_STRING ValueName;
    RtlInitUnicodeString(&ValueName, L"SecondaryKeyTestValue");

    DWORD ValueData1 = 0x12345678;
    DWORD ValueData2 = 0x87654321;

    INIT_TEST(state, Status);

    state->ApcRan = FALSE;

    /* Start watching for changes */
    Status = NtNotifyChangeMultipleKeys(state->KeyHandle, 0, NULL, NULL, NtNotifyChangeMultipleKeys_ApcRoutine, state, &state->IoStatusBlock, REG_NOTIFY_CHANGE_LAST_SET, FALSE, NULL, 0, TRUE);
    TEST_ERROR(state, Status, STATUS_PENDING);

    /* Make change to the registry key */
    Status = NtSetValueKey(state->KeyHandle, &ValueName, 0, REG_DWORD, &ValueData1, sizeof(ValueData1));
    TEST_ERROR(state, Status, STATUS_SUCCESS);
    SleepEx(100, TRUE);
    Status = NtSetValueKey(state->KeyHandle, &ValueName, 0, REG_DWORD, &ValueData2, sizeof(ValueData2));
    TEST_ERROR(state, Status, STATUS_SUCCESS);

    ok(state->ApcRan == TRUE, "The APC routine did not ran.\n");
    if (!state->ApcRan)
    {
        CLEANUP(state);
        return STATUS_UNSUCCESSFUL;
    }

    FINALIZE_TEST(state);
}

/* Main */

START_TEST(NtNotifyChangeMultipleKeys)
{
    NTSTATUS Status;
    TState State;
    
    Status = NtNotifyChangeMultipleKeys_Initialize(&State);
    ASSERT(NT_SUCCESS(Status));

    RUN_TEST(&State, Status, Synchronous);
    RUN_TEST(&State, Status, Asynchronous);
    RUN_TEST(&State, Status, WatchSubtree);
    RUN_TEST(&State, Status, WatchSecondaryKey);
    RUN_TEST(&State, Status, ApcRoutine);

    NtNotifyChangeMultipleKeys_CleanupTestKeys(&State);
}
