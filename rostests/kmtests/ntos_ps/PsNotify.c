/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Process Notification Routines test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

//#define NDEBUG
#include <debug.h>

static
VOID
NTAPI
CreateProcessNotifyRoutine(
    IN HANDLE ParentId,
    IN HANDLE ProcessId,
    IN BOOLEAN Create)
{
    ok_irql(PASSIVE_LEVEL);
    if (!Create)
        ok_eq_pointer(ProcessId, PsGetCurrentProcessId());
    else
        ok(ProcessId != PsGetCurrentProcessId(),
           "ProcessId %p equals current\n", ProcessId);
    DPRINT("%s(%p, %p, %d)\n", __FUNCTION__, ParentId, ProcessId, Create);
}

static
VOID
TestCreateProcessNotify(VOID)
{
    NTSTATUS Status;

    Status = PsSetCreateProcessNotifyRoutine(NULL, TRUE);
    ok_eq_hex(Status, STATUS_PROCEDURE_NOT_FOUND);

    Status = PsSetCreateProcessNotifyRoutine(NULL, FALSE);
    ok_eq_hex(Status, STATUS_SUCCESS);

    Status = PsSetCreateProcessNotifyRoutine(NULL, TRUE);
    ok_eq_hex(Status, STATUS_SUCCESS);

    Status = PsSetCreateProcessNotifyRoutine(NULL, TRUE);
    ok_eq_hex(Status, STATUS_PROCEDURE_NOT_FOUND);

    Status = PsSetCreateProcessNotifyRoutine(CreateProcessNotifyRoutine, TRUE);
    ok_eq_hex(Status, STATUS_PROCEDURE_NOT_FOUND);

    Status = PsSetCreateProcessNotifyRoutine(CreateProcessNotifyRoutine, FALSE);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* TODO: test whether the callback is notified on process creation */

    Status = PsSetCreateProcessNotifyRoutine(CreateProcessNotifyRoutine, TRUE);
    ok_eq_hex(Status, STATUS_SUCCESS);

    Status = PsSetCreateProcessNotifyRoutine(CreateProcessNotifyRoutine, TRUE);
    ok_eq_hex(Status, STATUS_PROCEDURE_NOT_FOUND);

    /* TODO: register the same routine twice */
    /* TODO: register more than the maximum number of notifications */
}

static
VOID
NTAPI
CreateThreadNotifyRoutine(
    IN HANDLE ProcessId,
    IN HANDLE ThreadId,
    IN BOOLEAN Create)
{
    ok_irql(PASSIVE_LEVEL);
    if (!Create)
    {
        ok_eq_pointer(ProcessId, PsGetCurrentProcessId());
        ok_eq_pointer(ThreadId, PsGetCurrentThreadId());
    }
    else
    {
        ok(ThreadId != PsGetCurrentThreadId(),
           "ThreadId %p equals current\n", ThreadId);
    }
    DPRINT("%s(%p, %p, %d)\n", __FUNCTION__, ProcessId, ThreadId, Create);
}

static
VOID
TestCreateThreadNotify(VOID)
{
    NTSTATUS Status;

    Status = PsRemoveCreateThreadNotifyRoutine(NULL);
    ok_eq_hex(Status, STATUS_PROCEDURE_NOT_FOUND);

    Status = PsSetCreateThreadNotifyRoutine(NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);

    Status = PsRemoveCreateThreadNotifyRoutine(NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);

    Status = PsRemoveCreateThreadNotifyRoutine(NULL);
    ok_eq_hex(Status, STATUS_PROCEDURE_NOT_FOUND);

    Status = PsRemoveCreateThreadNotifyRoutine(CreateThreadNotifyRoutine);
    ok_eq_hex(Status, STATUS_PROCEDURE_NOT_FOUND);

    Status = PsSetCreateThreadNotifyRoutine(CreateThreadNotifyRoutine);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* TODO: test whether the callback is notified on thread creation */

    Status = PsRemoveCreateThreadNotifyRoutine(CreateThreadNotifyRoutine);
    ok_eq_hex(Status, STATUS_SUCCESS);

    Status = PsRemoveCreateThreadNotifyRoutine(CreateThreadNotifyRoutine);
    ok_eq_hex(Status, STATUS_PROCEDURE_NOT_FOUND);

    /* TODO: register the same routine twice */
    /* TODO: register more than the maximum number of notifications */
}

static
VOID
NTAPI
LoadImageNotifyRoutine(
    IN PUNICODE_STRING FullImageName OPTIONAL,
    IN HANDLE ProcessId,
    IN PIMAGE_INFO ImageInfo)
{
    ok_irql(PASSIVE_LEVEL);
    DPRINT("%s('%wZ', %p, %p)\n", __FUNCTION__, FullImageName, ProcessId, ImageInfo);
}

static
VOID
TestLoadImageNotify(VOID)
{
    NTSTATUS Status;

    Status = PsRemoveLoadImageNotifyRoutine(NULL);
    ok_eq_hex(Status, STATUS_PROCEDURE_NOT_FOUND);

    Status = PsSetLoadImageNotifyRoutine(NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);

    Status = PsRemoveLoadImageNotifyRoutine(NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);

    Status = PsRemoveLoadImageNotifyRoutine(NULL);
    ok_eq_hex(Status, STATUS_PROCEDURE_NOT_FOUND);

    Status = PsRemoveLoadImageNotifyRoutine(LoadImageNotifyRoutine);
    ok_eq_hex(Status, STATUS_PROCEDURE_NOT_FOUND);

    Status = PsSetLoadImageNotifyRoutine(LoadImageNotifyRoutine);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* TODO: test whether the callback is notified on image load */

    Status = PsRemoveLoadImageNotifyRoutine(LoadImageNotifyRoutine);
    ok_eq_hex(Status, STATUS_SUCCESS);

    Status = PsRemoveLoadImageNotifyRoutine(LoadImageNotifyRoutine);
    ok_eq_hex(Status, STATUS_PROCEDURE_NOT_FOUND);

    /* TODO: register the same routine twice */
    /* TODO: register more than the maximum number of notifications */
}

START_TEST(PsNotify)
{
    TestCreateProcessNotify();
    TestCreateThreadNotify();
    TestLoadImageNotify();
}
