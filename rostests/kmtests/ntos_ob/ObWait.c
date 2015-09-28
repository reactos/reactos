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
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ThreadHandle;
    PVOID ThreadObject = NULL;

    /* We've to be in kernel mode, so spawn a thread */
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = PsCreateSystemThread(&ThreadHandle,
                                  SYNCHRONIZE,
                                  &ObjectAttributes,
                                  NULL,
                                  NULL,
                                  KernelModeTest,
                                  NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (Status == STATUS_SUCCESS)
    {
        /* Then, just wait on our thread to finish */
        Status = ObReferenceObjectByHandle(ThreadHandle,
                                           SYNCHRONIZE,
                                           *PsThreadType,
                                           KernelMode,
                                           &ThreadObject,
                                           NULL);
        ObCloseHandle(ThreadHandle, KernelMode);

        Status = KeWaitForSingleObject(ThreadObject,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       NULL);
        ok_eq_hex(Status, STATUS_SUCCESS);
        ObDereferenceObject(ThreadObject);
    }
}
