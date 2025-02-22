/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Test for NtUnloadDriver
 * COPYRIGHT:   Copyright 2019 Thomas Faber (thomas.faber@reactos.org)
 */

#include "precomp.h"

START_TEST(NtUnloadDriver)
{
    NTSTATUS Status;
    BOOLEAN OldPrivilege, OldPrivilege2;
    UNICODE_STRING ServiceName;
    PWCHAR Buffer = NULL;

    Status = RtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE, FALSE, FALSE, &OldPrivilege);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to drop driver load privilege\n");
        return;
    }

    Status = NtUnloadDriver(NULL);
    ok_hex(Status, STATUS_PRIVILEGE_NOT_HELD);

    Status = RtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE, TRUE, FALSE, &OldPrivilege2);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to acquire driver load privilege\n");
        goto Exit;
    }

    Status = NtUnloadDriver(NULL);
    ok_hex(Status, STATUS_ACCESS_VIOLATION);

    RtlInitEmptyUnicodeString(&ServiceName, NULL, 0);
    Status = NtUnloadDriver(&ServiceName);
    ok_hex(Status, STATUS_INVALID_PARAMETER);

    Buffer = AllocateGuarded(0x10000);
    if (!Buffer)
    {
        skip("Failed to allocate memory\n");
        goto Exit;
    }

    RtlFillMemoryUlong(Buffer, 0x10000, 'A' << 16 | 'A');

    ServiceName.Buffer = Buffer;
    ServiceName.Length = 0xFFFF;
    ServiceName.MaximumLength = MAXUSHORT;
    Status = NtUnloadDriver(&ServiceName);
    ok_hex(Status, STATUS_OBJECT_NAME_INVALID);

    ServiceName.Buffer = Buffer;
    ServiceName.Length = 0xFFFE;
    ServiceName.MaximumLength = MAXUSHORT;
    Status = NtUnloadDriver(&ServiceName);
    ok_hex(Status, STATUS_OBJECT_NAME_INVALID);

    ServiceName.Buffer = Buffer;
    ServiceName.Length = 0xFFFD;
    ServiceName.MaximumLength = MAXUSHORT;
    Status = NtUnloadDriver(&ServiceName);
    ok_hex(Status, STATUS_OBJECT_NAME_INVALID);

    ServiceName.Buffer = Buffer;
    ServiceName.Length = 0xFFFC;
    ServiceName.MaximumLength = MAXUSHORT;
    Status = NtUnloadDriver(&ServiceName);
    ok_hex(Status, STATUS_OBJECT_PATH_SYNTAX_BAD);

    ServiceName.Buffer = Buffer;
    ServiceName.Length = 0x1000;
    ServiceName.MaximumLength = MAXUSHORT;
    Status = NtUnloadDriver(&ServiceName);
    ok_hex(Status, STATUS_OBJECT_PATH_SYNTAX_BAD);

    ServiceName.Buffer = Buffer;
    ServiceName.Length = 1;
    ServiceName.MaximumLength = MAXUSHORT;
    Status = NtUnloadDriver(&ServiceName);
    ok_hex(Status, STATUS_OBJECT_NAME_INVALID);

    Buffer[0xFFFC / sizeof(WCHAR)] = L'\\';
    ServiceName.Buffer = Buffer;
    ServiceName.Length = 0xFFFC;
    ServiceName.MaximumLength = MAXUSHORT;
    Status = NtUnloadDriver(&ServiceName);
    ok_hex(Status, STATUS_OBJECT_PATH_SYNTAX_BAD);

    Buffer[0xFFFC / sizeof(WCHAR) - 1] = L'\\';
    ServiceName.Buffer = Buffer;
    ServiceName.Length = 0xFFFC;
    ServiceName.MaximumLength = MAXUSHORT;
    Status = NtUnloadDriver(&ServiceName);
    ok_hex(Status, STATUS_OBJECT_PATH_SYNTAX_BAD);

Exit:
    if (Buffer != NULL)
    {
        FreeGuarded(Buffer);
    }

    Status = RtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE, OldPrivilege, FALSE, &OldPrivilege2);
    ok_hex(Status, STATUS_SUCCESS);
}
