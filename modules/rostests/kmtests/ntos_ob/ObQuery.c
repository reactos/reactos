/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Kernel mode tests for object information querying
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
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

START_TEST(ObQuery)
{
    ObjectBasicInformationTests();
}
