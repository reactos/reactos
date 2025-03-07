/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Kernel mode tests for object information querying
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 *              Copyright 2024 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include <kmt_test.h>

#define OBJ_WINSTA_DIRECTORY_NAME_INFO_SIZE (sizeof(UNICODE_STRING) + sizeof(L"\\Windows"))
#define OBJ_DIRECTORY_TYPE_INFO_SIZE (sizeof(OBJECT_TYPE_INFORMATION) + sizeof(L"Directory"))

static
VOID
ObjectBasicInformationTests(VOID)
{
    NTSTATUS Status;
    HANDLE WinStaDirHandle;
    OBJECT_BASIC_INFORMATION BasicInfo;
    ULONG ReturnLength;
    OBJECT_ATTRIBUTES ObjectAttributes;
    static UNICODE_STRING WinStaDir = RTL_CONSTANT_STRING(L"\\Windows");

    /* We must be in PASSIVE_LEVEL to do all of this stuff */
    ok_irql(PASSIVE_LEVEL);

    /* Create a path to \Windows directory */
    InitializeObjectAttributes(&ObjectAttributes,
                               &WinStaDir,
                               OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwOpenDirectoryObject(&WinStaDirHandle,
                                   DIRECTORY_QUERY | DIRECTORY_TRAVERSE,
                                   &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        ok(FALSE, "Failed to open \\Windows directory (Status 0x%lx)\n", Status);
        return;
    }

    /* Give 0 as information length, this must fail */
    Status = ZwQueryObject(WinStaDirHandle,
                           ObjectBasicInformation,
                           &BasicInfo,
                           0,
                           &ReturnLength);
    ok_eq_hex(Status, STATUS_INFO_LENGTH_MISMATCH);

    /* Do a proper query now */
    Status = ZwQueryObject(WinStaDirHandle,
                           ObjectBasicInformation,
                           &BasicInfo,
                           sizeof(BasicInfo),
                           &ReturnLength);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* \Windows is currently used */
    ok(BasicInfo.HandleCount != 0, "\\Windows is in use but HandleCount is 0!\n");
    ok(BasicInfo.PointerCount != 0, "\\Windows is in use but PointerCount is 0!\n");

    ok_eq_ulong(BasicInfo.NameInfoSize, OBJ_WINSTA_DIRECTORY_NAME_INFO_SIZE);
    ok_eq_ulong(BasicInfo.TypeInfoSize, OBJ_DIRECTORY_TYPE_INFO_SIZE);

    ZwClose(WinStaDirHandle);
}

/* Flags combination allowing all the read, write and delete share modes.
 * Currently similar to FILE_SHARE_VALID_FLAGS. */
#define FILE_SHARE_ALL \
    (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE)

#define ok_ntstatus ok_eq_hex

/* Adapted from apitests/ntdll/NtQueryObject.c!START_TEST(NtQueryObject).
 * Please sync both tests in case you add or remove new features. */
static
VOID
ObjectNameInformationTests(VOID)
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

    /* We must be in PASSIVE_LEVEL to do all of this stuff */
    ok_irql(PASSIVE_LEVEL);

    /* Open a handle to the device */
    RtlInitUnicodeString(&DeviceName, NtDeviceName);
    InitializeObjectAttributes(&ObjectAttributes,
                               &DeviceName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwOpenFile(&DeviceHandle,
                        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_ALL,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    ok_ntstatus(Status, STATUS_SUCCESS);
    if (skip(NT_SUCCESS(Status), "Device '%wZ': Opening failed\n", &DeviceName))
        return;

    /* Invoke ObjectNameInformation that retrieves the canonical device name */
    Status = ZwQueryObject(DeviceHandle,
                           ObjectNameInformation,
                           &ObjectNameBuffer,
                           0,
                           &BufferSize1);
    ok_ntstatus(Status, STATUS_INFO_LENGTH_MISMATCH);

    Status = ZwQueryObject(DeviceHandle,
                           ObjectNameInformation,
                           &ObjectNameBuffer,
                           sizeof(OBJECT_NAME_INFORMATION),
                           &BufferSize2);
    ok_ntstatus(Status, STATUS_BUFFER_OVERFLOW);

    Status = ZwQueryObject(DeviceHandle,
                           ObjectNameInformation,
                           &ObjectNameBuffer,
                           sizeof(ObjectNameBuffer),
                           &BufferSize3);
    ok_ntstatus(Status, STATUS_SUCCESS);

    ZwClose(DeviceHandle);

    /* Compare the returned buffer sizes */

    /* The returned size behaviour changed (when ZwQueryObject()'s
     * input Length is zero) between Windows <= 2003 and Vista+ */
    if (g_OsVersion < _WIN32_WINNT_VISTA)
        ok_eq_ulong(BufferSize1, sizeof(OBJECT_NAME_INFORMATION));
    else
        ok_eq_ulong(BufferSize1, sizeof(OBJECT_NAME_INFORMATION) + ObjectName->MaximumLength);

    ok_eq_ulong(BufferSize2, BufferSize3);
    ok_eq_ulong(BufferSize3, sizeof(OBJECT_NAME_INFORMATION) + ObjectName->MaximumLength);

    /* Test the name buffer */
    ok(ObjectName->Length > 0, "ObjectName->Length == %hu, expected > 0\n", ObjectName->Length);
    ok_eq_uint(ObjectName->MaximumLength, ObjectName->Length + sizeof(WCHAR));
    ok(ObjectName->Buffer[ObjectName->Length / sizeof(WCHAR)] == UNICODE_NULL,
       "UNICODE_NULL not found at end of ObjectName->Buffer\n");
    if (skip(ObjectName->Buffer[ObjectName->Length / sizeof(WCHAR)] == UNICODE_NULL,
        "ObjectName->Buffer string length check skipped\n"))
    {
        return;
    }
    /* Verify that ObjectName->Length doesn't count extra NUL-terminators */
    {
    SIZE_T strLen = wcslen(ObjectName->Buffer) * sizeof(WCHAR);
    ok_eq_size(strLen, (SIZE_T)ObjectName->Length);
    }
}

START_TEST(ObQuery)
{
    ObjectBasicInformationTests();
    ObjectNameInformationTests();
}
