/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:         Kernel-Mode Test Suite CcGetFlushedValidData test user-mode part
 * PROGRAMMER:      Alex Mendoza <05alex.mendozaa@gmail.com>
 */

#include <kmt_test.h>

#define IOCTL_START_TEST 1
#define IOCTL_FINISH_TEST 2

START_TEST(CcGetFlushedValidData)
{
    DWORD Ret;
    ULONG TestId;

    Ret = KmtLoadAndOpenDriver(L"CcGetFlushedValidData", FALSE);
    ok_eq_int(Ret, ERROR_SUCCESS);
    if (Ret)
        return;

    /* 0: uncached SectionObjectPointer -> MAXLONGLONG
     * 1: freshly cached, nothing dirty -> full ValidDataLength
     * 2: first page dirty -> 0
     * 3: page in second VACB dirty -> offset of that VACB
     * 4: dirty then flushed -> full ValidDataLength again
     */
    for (TestId = 0; TestId < 5; ++TestId)
    {
        Ret = KmtSendUlongToDriver(IOCTL_START_TEST, TestId);
        ok(Ret == ERROR_SUCCESS, "KmtSendUlongToDriver failed: %lx\n", Ret);
        Ret = KmtSendUlongToDriver(IOCTL_FINISH_TEST, TestId);
        ok(Ret == ERROR_SUCCESS, "KmtSendUlongToDriver failed: %lx\n", Ret);
    }

    KmtCloseDriver();
    KmtUnloadDriver();
}