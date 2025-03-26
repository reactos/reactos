/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Kernel mode tests for process quotas
 * COPYRIGHT:   Copyright 2021 George Bișoc <george.bisoc@reactos.org>
 */

#include <kmt_test.h>

START_TEST(PsQuota)
{
    NTSTATUS Status;
    VM_COUNTERS VmCounters;
    QUOTA_LIMITS QuotaLimits;
    SIZE_T NonPagedUsage, PagedUsage;
    PEPROCESS Process = PsGetCurrentProcess();

    /* Guard the quota operations in a guarded region */
    KeEnterGuardedRegion();

    /* Report the current process' quota limits */
    Status = ZwQueryInformationProcess(NtCurrentProcess(),
                                       ProcessQuotaLimits,
                                       &QuotaLimits,
                                       sizeof(QuotaLimits),
                                       NULL);
    if (skip(NT_SUCCESS(Status), "Failed to query quota limits -- %lx\n", Status))
    {
        return;
    }

    trace("Process paged pool quota limit -- %lu\n", QuotaLimits.PagedPoolLimit);
    trace("Process non paged pool quota limit -- %lu\n", QuotaLimits.NonPagedPoolLimit);
    trace("Process page file quota limit -- %lu\n\n", QuotaLimits.PagefileLimit);

    /* Query the quota usage */
    Status = ZwQueryInformationProcess(NtCurrentProcess(),
                                       ProcessVmCounters,
                                       &VmCounters,
                                       sizeof(VmCounters),
                                       NULL);
    if (skip(NT_SUCCESS(Status), "Failed to query quota usage -- %lx\n", Status))
    {
        return;
    }

    /* Test that quotas usage are within limits */
    ok(VmCounters.QuotaNonPagedPoolUsage < QuotaLimits.NonPagedPoolLimit, "Non paged quota over limits (usage -> %lu || limit -> %lu)\n",
       VmCounters.QuotaNonPagedPoolUsage, QuotaLimits.NonPagedPoolLimit);
    ok(VmCounters.QuotaPagedPoolUsage < QuotaLimits.PagedPoolLimit, "Paged quota over limits (usage -> %lu || limit -> %lu)\n",
       VmCounters.QuotaPagedPoolUsage, QuotaLimits.PagedPoolLimit);

    /* Cache the quota usage pools for later checks  */
    NonPagedUsage = VmCounters.QuotaNonPagedPoolUsage;
    PagedUsage = VmCounters.QuotaPagedPoolUsage;

    /* Charge some paged and non paged quotas */
    Status = PsChargeProcessNonPagedPoolQuota(Process, 0x200);
    ok_irql(PASSIVE_LEVEL);
    ok_eq_hex(Status, STATUS_SUCCESS);

    Status = PsChargeProcessPagedPoolQuota(Process, 0x500);
    ok_irql(PASSIVE_LEVEL);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Query the quota usage again */
    Status = ZwQueryInformationProcess(NtCurrentProcess(),
                                       ProcessVmCounters,
                                       &VmCounters,
                                       sizeof(VmCounters),
                                       NULL);
    if (skip(NT_SUCCESS(Status), "Failed to query quota usage -- %lx\n", Status))
    {
        return;
    }


    /* Test again the usage that's within limits */
    ok(VmCounters.QuotaNonPagedPoolUsage < QuotaLimits.NonPagedPoolLimit, "Non paged quota over limits (usage -> %lu || limit -> %lu)\n",
       VmCounters.QuotaNonPagedPoolUsage, QuotaLimits.NonPagedPoolLimit);
    ok(VmCounters.QuotaPagedPoolUsage < QuotaLimits.PagedPoolLimit, "Paged quota over limits (usage -> %lu || limit -> %lu)\n",
       VmCounters.QuotaPagedPoolUsage, QuotaLimits.PagedPoolLimit);

    /*
     * Make sure the results are consistent, that nobody else
     * is charging quotas other than us.
     */
    ok_eq_size(VmCounters.QuotaNonPagedPoolUsage, NonPagedUsage + 0x200);
    ok_eq_size(VmCounters.QuotaPagedPoolUsage, PagedUsage + 0x500);

    /* Report the quota usage */
    trace("=== QUOTA USAGE AFTER CHARGE ===\n\n");
    trace("Process paged pool quota usage -- %lu\n", VmCounters.QuotaPagedPoolUsage);
    trace("Process paged pool quota peak -- %lu\n", VmCounters.QuotaPeakPagedPoolUsage);
    trace("Process non paged pool quota usage -- %lu\n", VmCounters.QuotaNonPagedPoolUsage);
    trace("Process non paged pool quota peak -- %lu\n", VmCounters.QuotaPeakNonPagedPoolUsage);
    trace("Process page file quota usage -- %lu\n", VmCounters.PagefileUsage);
    trace("Process page file quota peak -- %lu\n\n", VmCounters.PeakPagefileUsage);

    /* Return the quotas we've charged up */
    PsReturnProcessNonPagedPoolQuota(Process, 0x200);
    PsReturnProcessPagedPoolQuota(Process, 0x500);

    /* Query the quota usage again */
    Status = ZwQueryInformationProcess(NtCurrentProcess(),
                                       ProcessVmCounters,
                                       &VmCounters,
                                       sizeof(VmCounters),
                                       NULL);
    if (skip(NT_SUCCESS(Status), "Failed to query quota usage -- %lx\n", Status))
    {
        return;
    }

    /*
     * Check that nobody else has returned quotas
     * but only us.
     */
    ok_eq_size(VmCounters.QuotaNonPagedPoolUsage, NonPagedUsage);
    ok_eq_size(VmCounters.QuotaPagedPoolUsage, PagedUsage);

    /* Report the usage again */
    trace("=== QUOTA USAGE AFTER RETURN ===\n\n");
    trace("Process paged pool quota usage -- %lu\n", VmCounters.QuotaPagedPoolUsage);
    trace("Process paged pool quota peak -- %lu\n", VmCounters.QuotaPeakPagedPoolUsage);
    trace("Process non paged pool quota usage -- %lu\n", VmCounters.QuotaNonPagedPoolUsage);
    trace("Process non paged pool quota peak -- %lu\n", VmCounters.QuotaPeakNonPagedPoolUsage);
    trace("Process page file quota usage -- %lu\n", VmCounters.PagefileUsage);
    trace("Process page file quota peak -- %lu\n\n", VmCounters.PeakPagefileUsage);

    /* We're done, leave the region */
    KeLeaveGuardedRegion();
}
