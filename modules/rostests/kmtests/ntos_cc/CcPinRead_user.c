/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite CcPinRead test user-mode part
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 */

#include <kmt_test.h>

#define IOCTL_START_TEST  1
#define IOCTL_FINISH_TEST 2

START_TEST(CcPinRead)
{
    DWORD Ret;
    ULONG TestId;

    KmtLoadDriver(L"CcPinRead", FALSE);
    KmtOpenDriver();

    /* 3 tests for offset
     * 1 test for BCB
     * 1 test for pinning access
     * 1 test for length/offset
     * 1 test for read/write size
     */
    for (TestId = 0; TestId < 7; ++TestId)
    {
        Ret = KmtSendUlongToDriver(IOCTL_START_TEST, TestId);
        ok(Ret == ERROR_SUCCESS, "KmtSendUlongToDriver failed: %lx\n", Ret);
        Ret = KmtSendUlongToDriver(IOCTL_FINISH_TEST, TestId);
        ok(Ret == ERROR_SUCCESS, "KmtSendUlongToDriver failed: %lx\n", Ret);
    }

    KmtCloseDriver();
    KmtUnloadDriver();
}
