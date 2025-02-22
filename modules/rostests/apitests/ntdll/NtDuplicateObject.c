/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Test for NtDuplicateObject
 * COPYRIGHT:   Copyright 2019 Thomas Faber (thomas.faber@reactos.org)
 */

#include "precomp.h"

#define OBJ_PROTECT_CLOSE 0x01

START_TEST(NtDuplicateObject)
{
    NTSTATUS Status;
    HANDLE Handle;

    Handle = NULL;
    Status = NtDuplicateObject(NtCurrentProcess(),
                               NtCurrentProcess(),
                               NtCurrentProcess(),
                               &Handle,
                               GENERIC_ALL,
                               OBJ_PROTECT_CLOSE,
                               0);
    ok_hex(Status, STATUS_SUCCESS);
    ok(Handle != NULL && Handle != NtCurrentProcess(),
        "Handle = %p\n", Handle);
    Status = NtClose(Handle);
    ok_hex(Status, STATUS_HANDLE_NOT_CLOSABLE);
}
