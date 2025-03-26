/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Kernel mode tests for logon session manipulation API
 * COPYRIGHT:   Copyright 2021 George Bișoc <george.bisoc@reactos.org>
 */

#include <kmt_test.h>
#include <ntifs.h>

#define IMAGINARY_LOGON {0x001, 0}

static
VOID
LogonMarkTermination(VOID)
{
    NTSTATUS Status;
    LUID SystemLogonID = SYSTEM_LUID;
    LUID NonExistentLogonID = IMAGINARY_LOGON;

    /* Mark the system authentication logon for termination */
    Status = SeMarkLogonSessionForTerminationNotification(&SystemLogonID);
    ok_irql(PASSIVE_LEVEL);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* We give a non existent logon */
    Status = SeMarkLogonSessionForTerminationNotification(&NonExistentLogonID);
    ok_irql(PASSIVE_LEVEL);
    ok_eq_hex(Status, STATUS_NOT_FOUND);
}

START_TEST(SeLogonSession)
{
    LogonMarkTermination();
}
