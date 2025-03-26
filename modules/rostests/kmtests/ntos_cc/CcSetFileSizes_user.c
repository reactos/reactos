/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite CcSetFileSizes test user-mode part
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 */

#include <kmt_test.h>

#define IOCTL_START_TEST  1
#define IOCTL_FINISH_TEST 2

START_TEST(CcSetFileSizes)
{
    DWORD Ret;
    ULONG TestId;

    Ret = KmtLoadAndOpenDriver(L"CcSetFileSizes", FALSE);
    ok_eq_int(Ret, ERROR_SUCCESS);
    if (Ret)
        return;

    /* 0: mapped data - only FS
     * 1: copy read - only FS
     * 2: mapped data - FS & AS
     * 3: copy read - FS & AS
     * 4: dirty VACB - only FS
     * 5: dirty VACB - FS & AS
     * 6: CcSetFileSizes with mapped data at tail of file
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
