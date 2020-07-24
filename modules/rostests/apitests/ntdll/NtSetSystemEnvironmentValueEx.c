/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Tests for the NtSetSystemEnvironmentValueEx
 * COPYRIGHT:       Copyright 2020 George Bișoc <george.bisoc@reactos.org>
 */

#include "precomp.h"

START_TEST(NtSetSystemEnvironmentValueEx)
{
    NTSTATUS Status;

    /* Do nothing, the function isn't implemented in Windows Server 2003 (Service Pack 2) */
    Status = NtSetSystemEnvironmentValueEx(NULL, NULL, NULL, NULL, NULL);
    ok_hex(Status, STATUS_NOT_IMPLEMENTED);
}
