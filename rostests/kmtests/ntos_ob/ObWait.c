/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite *WaitForMultipleObjects
 * PROGRAMMER:      Pierre Schweitzer <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

static
VOID
NTAPI
KernelModeTest(IN PVOID Context)
{
    NTSTATUS Status;

    Status = ZwWaitForMultipleObjects(2, (void **)0x42424242, WaitAll, FALSE, NULL);
    ok_eq_hex(Status, STATUS_ACCESS_VIOLATION);
}

START_TEST(ZwWaitForMultipleObjects)
{
    PKTHREAD ThreadHandle;

    /* We've to be in kernel mode, so spawn a thread */
    ThreadHandle = KmtStartThread(KernelModeTest, NULL);
    KmtFinishThread(ThreadHandle, NULL);
}
