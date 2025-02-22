/*
 * PROJECT:         ReactOS API Tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for NtSaveKey
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#include "precomp.h"

static
NTSTATUS
OpenRegistryKeyHandle(PHANDLE KeyHandle,
                      ACCESS_MASK AccessMask,
                      PWCHAR RegistryPath)
{
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES Attributes;

    RtlInitUnicodeString(&KeyName, RegistryPath);
    InitializeObjectAttributes(&Attributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    return NtOpenKey(KeyHandle, AccessMask, &Attributes);
}

START_TEST(NtSaveKey)
{
    NTSTATUS Status;
    HANDLE KeyHandle;
    HANDLE FileHandle;
    BOOLEAN PrivilegeEnabled = FALSE;
    BOOLEAN OldPrivilegeStatus;

    /* Make sure we don't have backup privileges initially, otherwise WHS testbot fails */
    Status = RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE, FALSE, FALSE, &PrivilegeEnabled);
    ok(Status == STATUS_SUCCESS, "RtlAdjustPrivilege returned %lx\n", Status);

    /* Open the file */
    FileHandle = CreateFileW(L"saved_key.dat",
                             GENERIC_READ | GENERIC_WRITE,
                             0,
                             NULL,
                             CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
                             NULL);
    if (FileHandle == INVALID_HANDLE_VALUE)
    {
        skip("CreateFileW failed with error: %lu\n", GetLastError());
        return;
    }

    /* Try saving HKEY_LOCAL_MACHINE\Hardware */
    Status = OpenRegistryKeyHandle(&KeyHandle, KEY_READ, L"\\Registry\\Machine\\Hardware");
    if (!NT_SUCCESS(Status))
    {
        skip("NtOpenKey failed with status: 0x%08lX\n", Status);
        NtClose(FileHandle);
        return;
    }

    Status = NtSaveKey(KeyHandle, FileHandle);
    ok_ntstatus(Status, STATUS_PRIVILEGE_NOT_HELD);

    NtClose(KeyHandle);

    /* Set the SeBackupPrivilege */
    Status = RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE,
                                TRUE,
                                FALSE,
                                &OldPrivilegeStatus);
    if (!NT_SUCCESS(Status))
    {
        skip("RtlAdjustPrivilege failed with status: 0x%08lX\n", (ULONG)Status);
        NtClose(FileHandle);
        return;
    }

    /* Try saving HKEY_LOCAL_MACHINE\Hardware again */
    Status = OpenRegistryKeyHandle(&KeyHandle, KEY_READ, L"\\Registry\\Machine\\Hardware");
    if (!NT_SUCCESS(Status))
    {
        skip("NtOpenKey failed with status: 0x%08lX\n", Status);
        goto Cleanup;
    }

    Status = NtSaveKey(KeyHandle, FileHandle);
    ok_ntstatus(Status, STATUS_SUCCESS);

    NtClose(KeyHandle);

    /* Try saving HKEY_LOCAL_MACHINE */
    Status = OpenRegistryKeyHandle(&KeyHandle, KEY_READ, L"\\Registry\\Machine");
    if (!NT_SUCCESS(Status))
    {
        skip("NtOpenKey failed with status: 0x%08lX\n", Status);
        goto Cleanup;
    }

    Status = NtSaveKey(KeyHandle, FileHandle);
    ok_ntstatus(Status, STATUS_ACCESS_DENIED);

    NtClose(KeyHandle);

    /* Try saving HKEY_USERS */
    Status = OpenRegistryKeyHandle(&KeyHandle, KEY_READ, L"\\Registry\\User");
    if (!NT_SUCCESS(Status))
    {
        skip("NtOpenKey failed with status: 0x%08lX\n", Status);
        goto Cleanup;
    }

    Status = NtSaveKey(KeyHandle, FileHandle);
    ok_ntstatus(Status, STATUS_ACCESS_DENIED);

    NtClose(KeyHandle);

Cleanup:

    /* Restore the SeBackupPrivilege */
    RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE,
                       OldPrivilegeStatus,
                       FALSE,
                       &OldPrivilegeStatus);

    /* Close the file handle */
    NtClose(FileHandle);
}
