/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Process Quota kernel mode tests suite
 * COPYRIGHT:   Copyright 2020 George Bișoc (george.bisoc@reactos.org)
 */

#include <kmt_test.h>

static
VOID
TestChargeProcessQuota(VOID)
{
    NTSTATUS Status;

    /* We give an invalid EPROCESS */
    KmtStartSeh()
        PsChargePoolQuota(NULL, -1, 0);
    KmtEndSeh(STATUS_ACCESS_VIOLATION);

    Status = PsChargeProcessNonPagedPoolQuota(PsGetCurrentProcess(), 200);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_irql(PASSIVE_LEVEL);

    Status = PsChargeProcessNonPagedPoolQuota(PsGetCurrentProcess(), -1);
    ok_eq_hex(Status, STATUS_QUOTA_EXCEEDED);
    ok_irql(PASSIVE_LEVEL);

    Status = PsChargeProcessPagedPoolQuota(PsGetCurrentProcess(), 200);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_irql(PASSIVE_LEVEL);

    Status = PsChargeProcessPagedPoolQuota(PsGetCurrentProcess(), -1);
    ok_eq_hex(Status, STATUS_QUOTA_EXCEEDED);
    ok_irql(PASSIVE_LEVEL);

    Status = PsChargeProcessPoolQuota(PsGetCurrentProcess(), PagedPool, 200);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_irql(PASSIVE_LEVEL);

    Status = PsChargeProcessPoolQuota(PsGetCurrentProcess(), NonPagedPool, 200);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_irql(PASSIVE_LEVEL);
}

START_TEST(PsQuota)
{
    TestChargeProcessQuota();
}
