/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Tests for the NtQuerySystemEnvironmentValueEx
 * COPYRIGHT:       Copyright 2020 George Bișoc <george.bisoc@reactos.org>
 */

#include "precomp.h"

START_TEST(NtQuerySystemEnvironmentValueEx)
{
    NTSTATUS Status;

    /* Do nothing, the function isn't implemented in Windows Server 2003 (Service Pack 2) */
    Status = NtQuerySystemEnvironmentValueEx(NULL, NULL, NULL, NULL, NULL);
    ok_hex(Status, STATUS_NOT_IMPLEMENTED);
}
