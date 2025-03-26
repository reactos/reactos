/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Tests for the NtSetDefaultLocale API
 * COPYRIGHT:       Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

#include "precomp.h"

START_TEST(NtSetDefaultLocale)
{
    NTSTATUS Status;

    Status = NtSetDefaultLocale(TRUE, 0xffffffff);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);

    Status = NtSetDefaultLocale(TRUE, 0xfffffffe);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);

    Status = NtSetDefaultLocale(TRUE, 0x7fffffff);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);

    Status = NtSetDefaultLocale(TRUE, 0x7ffffffe);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);

    Status = NtSetDefaultLocale(TRUE, 0x80000000);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);

    Status = NtSetDefaultLocale(TRUE, 0x80000001);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);

    Status = NtSetDefaultLocale(TRUE, 0x10000);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);

    Status = NtSetDefaultLocale(TRUE, 1);
    ok_ntstatus(Status, STATUS_OBJECT_NAME_NOT_FOUND);

    Status = NtSetDefaultLocale(TRUE, 0x0C);
    ok_ntstatus(Status, STATUS_OBJECT_NAME_NOT_FOUND);

    Status = NtSetDefaultLocale(TRUE, 0x1000);
    ok_ntstatus(Status, STATUS_OBJECT_NAME_NOT_FOUND);
}
