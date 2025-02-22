/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Test for NtQueryObject
 * COPYRIGHT:   Copyright 2024 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include "precomp.h"

/* Flags combination allowing all the read, write and delete share modes.
 * Currently similar to FILE_SHARE_VALID_FLAGS. */
#define FILE_SHARE_ALL \
    (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE)

/* Adapted from kmtests/ntos_ob/ObQuery.c!ObjectNameInformationTests().
 * Please sync both tests in case you add or remove new features. */
START_TEST(NtQueryObject)
{
    ULONG g_OsVersion =
        SharedUserData->NtMajorVersion << 8 | SharedUserData->NtMinorVersion;

    NTSTATUS Status;
    HANDLE DeviceHandle;
    UNICODE_STRING DeviceName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;

    ULONG BufferSize1, BufferSize2, BufferSize3;
    struct { OBJECT_NAME_INFORMATION; WCHAR Buffer[MAX_PATH]; } ObjectNameBuffer;
    PUNICODE_STRING ObjectName = &ObjectNameBuffer.Name;

    /* Test the drive containing SystemRoot */
    WCHAR NtDeviceName[] = L"\\DosDevices\\?:";
    NtDeviceName[sizeof("\\DosDevices\\")-1] = SharedUserData->NtSystemRoot[0];

    /* Open a handle to the device */
    RtlInitUnicodeString(&DeviceName, NtDeviceName);
    InitializeObjectAttributes(&ObjectAttributes,
                               &DeviceName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenFile(&DeviceHandle,
                        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_ALL,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    ok_ntstatus(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("Device '%S': Opening failed\n", NtDeviceName);
        return;
    }

    /* Invoke ObjectNameInformation that retrieves the canonical device name */
    Status = NtQueryObject(DeviceHandle,
                           ObjectNameInformation,
                           &ObjectNameBuffer,
                           0,
                           &BufferSize1);
    ok_ntstatus(Status, STATUS_INFO_LENGTH_MISMATCH);

    Status = NtQueryObject(DeviceHandle,
                           ObjectNameInformation,
                           &ObjectNameBuffer,
                           sizeof(OBJECT_NAME_INFORMATION),
                           &BufferSize2);
    ok_ntstatus(Status, STATUS_BUFFER_OVERFLOW);

    Status = NtQueryObject(DeviceHandle,
                           ObjectNameInformation,
                           &ObjectNameBuffer,
                           sizeof(ObjectNameBuffer),
                           &BufferSize3);
    ok_ntstatus(Status, STATUS_SUCCESS);

    NtClose(DeviceHandle);

    /* Compare the returned buffer sizes */

    /* The returned size behaviour changed (when NtQueryObject()'s
     * input Length is zero) between Windows <= 2003 and Vista+ */
    if (g_OsVersion < _WIN32_WINNT_VISTA)
        ok_eq_ulong(BufferSize1, (ULONG)sizeof(OBJECT_NAME_INFORMATION));
    else
        ok_eq_ulong(BufferSize1, (ULONG)sizeof(OBJECT_NAME_INFORMATION) + ObjectName->MaximumLength);

    ok_eq_ulong(BufferSize2, BufferSize3);
    ok_eq_ulong(BufferSize3, (ULONG)sizeof(OBJECT_NAME_INFORMATION) + ObjectName->MaximumLength);

    /* Test the name buffer */
    ok(ObjectName->Length > 0, "ObjectName->Length == %hu, expected > 0\n", ObjectName->Length);
    ok_eq_uint(ObjectName->MaximumLength, ObjectName->Length + sizeof(WCHAR));
    ok(ObjectName->Buffer[ObjectName->Length / sizeof(WCHAR)] == UNICODE_NULL,
       "UNICODE_NULL not found at end of ObjectName->Buffer\n");
    if (ObjectName->Buffer[ObjectName->Length / sizeof(WCHAR)] != UNICODE_NULL)
    {
        skip("ObjectName->Buffer string length check skipped\n");
        return;
    }
    /* Verify that ObjectName->Length doesn't count extra NUL-terminators */
    {
    SIZE_T strLen = wcslen(ObjectName->Buffer) * sizeof(WCHAR);
    ok_eq_size(strLen, (SIZE_T)ObjectName->Length);
    }
}
