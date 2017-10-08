/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test for Zw*SymbolicLinkObject
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

static
VOID
TestQueryLink(
    _In_ HANDLE LinkHandle,
    _In_ PCUNICODE_STRING ExpectedTarget)
{
    NTSTATUS Status;
    WCHAR QueriedTargetBuffer[32];
    UNICODE_STRING QueriedTarget;
    ULONG ResultLength;
    PULONG pResultLength;
    ULONG i;

    for (i = 0; i < 2; i++)
    {
        if (i == 0)
        {
            pResultLength = &ResultLength;
        }
        else
        {
            pResultLength = NULL;
        }

        /* Query with NULL Buffer just gives us the length */
        RtlInitEmptyUnicodeString(&QueriedTarget, NULL, 0);
        if (pResultLength) ResultLength = 0x55555555;
        Status = ZwQuerySymbolicLinkObject(LinkHandle,
                                           &QueriedTarget,
                                           pResultLength);
        ok_eq_hex(Status, STATUS_BUFFER_TOO_SMALL);
        ok_eq_uint(QueriedTarget.Length, 0);
        ok_eq_uint(QueriedTarget.MaximumLength, 0);
        ok_eq_pointer(QueriedTarget.Buffer, NULL);
        if (pResultLength) ok_eq_ulong(ResultLength, ExpectedTarget->MaximumLength);

        /* Query with Length-1 buffer */
        RtlInitEmptyUnicodeString(&QueriedTarget,
                                  QueriedTargetBuffer,
                                  ExpectedTarget->Length - 1);
        RtlFillMemory(&QueriedTargetBuffer, sizeof(QueriedTargetBuffer), 0x55);
        if (pResultLength) ResultLength = 0x55555555;
        Status = ZwQuerySymbolicLinkObject(LinkHandle,
                                           &QueriedTarget,
                                           pResultLength);
        ok_eq_hex(Status, STATUS_BUFFER_TOO_SMALL);
        ok_eq_uint(QueriedTarget.Length, 0);
        ok_eq_uint(QueriedTarget.MaximumLength, ExpectedTarget->Length - 1);
        ok_eq_pointer(QueriedTarget.Buffer, QueriedTargetBuffer);
        ok_eq_uint(QueriedTarget.Buffer[0], 0x5555);
        if (pResultLength) ok_eq_ulong(ResultLength, ExpectedTarget->MaximumLength);

        /* Query with Length buffer */
        RtlInitEmptyUnicodeString(&QueriedTarget,
                                  QueriedTargetBuffer,
                                  ExpectedTarget->Length);
        RtlFillMemory(&QueriedTargetBuffer, sizeof(QueriedTargetBuffer), 0x55);
        if (pResultLength) ResultLength = 0x55555555;
        Status = ZwQuerySymbolicLinkObject(LinkHandle,
                                           &QueriedTarget,
                                           pResultLength);
        ok_eq_uint(QueriedTarget.MaximumLength, ExpectedTarget->Length);
        ok_eq_pointer(QueriedTarget.Buffer, QueriedTargetBuffer);
        if (pResultLength &&
            QueriedTarget.MaximumLength < ExpectedTarget->MaximumLength)
        {
            ok_eq_hex(Status, STATUS_BUFFER_TOO_SMALL);
            ok_eq_uint(QueriedTarget.Length, 0);
            ok_eq_uint(QueriedTarget.Buffer[0], 0x5555);
            if (pResultLength) ok_eq_ulong(ResultLength, ExpectedTarget->MaximumLength);
        }
        else
        {
            ok_eq_hex(Status, STATUS_SUCCESS);
            ok_eq_uint(QueriedTarget.Length, ExpectedTarget->Length);
            ok(RtlEqualUnicodeString(&QueriedTarget, ExpectedTarget, FALSE), "%wZ != %wZ\n", &QueriedTarget, ExpectedTarget);
            ok_eq_uint(QueriedTarget.Buffer[QueriedTarget.Length / sizeof(WCHAR)], 0x5555);
        }

        /* Query with Length+1 buffer */
        RtlInitEmptyUnicodeString(&QueriedTarget,
                                  QueriedTargetBuffer,
                                  ExpectedTarget->Length + 1);
        RtlFillMemory(&QueriedTargetBuffer, sizeof(QueriedTargetBuffer), 0x55);
        if (pResultLength) ResultLength = 0x55555555;
        Status = ZwQuerySymbolicLinkObject(LinkHandle,
                                           &QueriedTarget,
                                           pResultLength);
        ok_eq_uint(QueriedTarget.MaximumLength, ExpectedTarget->Length + 1);
        ok_eq_pointer(QueriedTarget.Buffer, QueriedTargetBuffer);
        if (pResultLength &&
            QueriedTarget.MaximumLength < ExpectedTarget->MaximumLength)
        {
            ok_eq_hex(Status, STATUS_BUFFER_TOO_SMALL);
            ok_eq_uint(QueriedTarget.Length, 0);
            ok_eq_uint(QueriedTarget.Buffer[0], 0x5555);
            if (pResultLength) ok_eq_ulong(ResultLength, ExpectedTarget->MaximumLength);
        }
        else
        {
            ok_eq_hex(Status, STATUS_SUCCESS);
            ok_eq_uint(QueriedTarget.Length, ExpectedTarget->Length);
            ok(RtlEqualUnicodeString(&QueriedTarget, ExpectedTarget, FALSE), "%wZ != %wZ\n", &QueriedTarget, ExpectedTarget);
            ok_eq_uint(QueriedTarget.Buffer[QueriedTarget.Length / sizeof(WCHAR)], 0x5555);
        }

        /* Query with Length+2 buffer */
        RtlInitEmptyUnicodeString(&QueriedTarget,
                                  QueriedTargetBuffer,
                                  ExpectedTarget->Length + sizeof(WCHAR));
        RtlFillMemory(&QueriedTargetBuffer, sizeof(QueriedTargetBuffer), 0x55);
        if (pResultLength) ResultLength = 0x55555555;
        Status = ZwQuerySymbolicLinkObject(LinkHandle,
                                           &QueriedTarget,
                                           pResultLength);
        ok_eq_hex(Status, STATUS_SUCCESS);
        ok_eq_uint(QueriedTarget.Length, ExpectedTarget->Length);
        ok_eq_uint(QueriedTarget.MaximumLength, ExpectedTarget->Length + sizeof(WCHAR));
        ok_eq_pointer(QueriedTarget.Buffer, QueriedTargetBuffer);
        ok(RtlEqualUnicodeString(&QueriedTarget, ExpectedTarget, FALSE), "%wZ != %wZ\n", &QueriedTarget, ExpectedTarget);
        if (pResultLength)
        {
            if (ExpectedTarget->MaximumLength >= ExpectedTarget->Length + sizeof(UNICODE_NULL))
                ok_eq_uint(QueriedTarget.Buffer[QueriedTarget.Length / sizeof(WCHAR)], 0);
            ok_eq_ulong(ResultLength, ExpectedTarget->MaximumLength);
        }
        else
        {
            ok_eq_uint(QueriedTarget.Buffer[QueriedTarget.Length / sizeof(WCHAR)], 0x5555);
        }

        /* Query with full-sized buffer */
        RtlInitEmptyUnicodeString(&QueriedTarget,
                                  QueriedTargetBuffer,
                                  sizeof(QueriedTargetBuffer));
        RtlFillMemory(&QueriedTargetBuffer, sizeof(QueriedTargetBuffer), 0x55);
        if (pResultLength) ResultLength = 0x55555555;
        Status = ZwQuerySymbolicLinkObject(LinkHandle,
                                           &QueriedTarget,
                                           pResultLength);
        ok_eq_hex(Status, STATUS_SUCCESS);
        ok_eq_uint(QueriedTarget.Length, ExpectedTarget->Length);
        ok_eq_uint(QueriedTarget.MaximumLength, sizeof(QueriedTargetBuffer));
        ok_eq_pointer(QueriedTarget.Buffer, QueriedTargetBuffer);
        ok(RtlEqualUnicodeString(&QueriedTarget, ExpectedTarget, FALSE), "%wZ != %wZ\n", &QueriedTarget, ExpectedTarget);
        if (pResultLength)
        {
            if (ExpectedTarget->MaximumLength >= ExpectedTarget->Length + sizeof(UNICODE_NULL))
                ok_eq_uint(QueriedTarget.Buffer[QueriedTarget.Length / sizeof(WCHAR)], 0);
            ok_eq_ulong(ResultLength, ExpectedTarget->MaximumLength);
        }
        else
        {
            ok_eq_uint(QueriedTarget.Buffer[QueriedTarget.Length / sizeof(WCHAR)], 0x5555);
        }
    }
}

START_TEST(ObSymbolicLink)
{
    NTSTATUS Status;
    HANDLE LinkHandle;
    HANDLE LinkHandle2;
    UNICODE_STRING LinkName = RTL_CONSTANT_STRING(L"\\Device\\ObSymbolicLinkKmtestNamedPipe");
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING DefaultLinkTarget = RTL_CONSTANT_STRING(L"\\Device\\NamedPipe");
    UNICODE_STRING LinkTarget = DefaultLinkTarget;

    InitializeObjectAttributes(&ObjectAttributes,
                               &LinkName,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    LinkHandle = KmtInvalidPointer;
    Status = ZwOpenSymbolicLinkObject(&LinkHandle,
                                      SYMBOLIC_LINK_QUERY,
                                      &ObjectAttributes);
    ok_eq_hex(Status, STATUS_OBJECT_NAME_NOT_FOUND);
    ok_eq_pointer(LinkHandle, NULL);
    if (NT_SUCCESS(Status)) ObCloseHandle(LinkHandle, KernelMode);

    /* Create it */
    LinkHandle = KmtInvalidPointer;
    Status = ZwCreateSymbolicLinkObject(&LinkHandle,
                                        SYMBOLIC_LINK_QUERY,
                                        &ObjectAttributes,
                                        &LinkTarget);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok(LinkHandle != NULL && LinkHandle != KmtInvalidPointer, "LinkHandle = %p\n", LinkHandle);
    if (skip(NT_SUCCESS(Status), "Failed to create link\n"))
    {
        return;
    }

    /* Now we should be able to open it */
    LinkHandle2 = KmtInvalidPointer;
    Status = ZwOpenSymbolicLinkObject(&LinkHandle2,
                                      SYMBOLIC_LINK_QUERY,
                                      &ObjectAttributes);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok(LinkHandle2 != NULL && LinkHandle2 != KmtInvalidPointer, "LinkHandle = %p\n", LinkHandle2);
    if (!skip(NT_SUCCESS(Status), "Failed to open symlink\n"))
    {
        TestQueryLink(LinkHandle2, &LinkTarget);
        ObCloseHandle(LinkHandle2, KernelMode);
    }

    /* Close it */
    ObCloseHandle(LinkHandle, KernelMode);

    /* Open fails again */
    LinkHandle = KmtInvalidPointer;
    Status = ZwOpenSymbolicLinkObject(&LinkHandle,
                                      SYMBOLIC_LINK_QUERY,
                                      &ObjectAttributes);
    ok_eq_hex(Status, STATUS_OBJECT_NAME_NOT_FOUND);
    ok_eq_pointer(LinkHandle, NULL);
    if (NT_SUCCESS(Status)) ObCloseHandle(LinkHandle, KernelMode);

    /* Create with broken LinkTarget */
    LinkHandle = KmtInvalidPointer;
    LinkTarget.Buffer = NULL;
    Status = ZwCreateSymbolicLinkObject(&LinkHandle,
                                        SYMBOLIC_LINK_QUERY,
                                        &ObjectAttributes,
                                        &LinkTarget);
    ok_eq_hex(Status, STATUS_ACCESS_VIOLATION);
    ok(LinkHandle == KmtInvalidPointer, "LinkHandle = %p\n", LinkHandle);
    if (NT_SUCCESS(Status)) ObCloseHandle(LinkHandle, KernelMode);

    /* Since it failed, open should also fail */
    LinkHandle = KmtInvalidPointer;
    Status = ZwOpenSymbolicLinkObject(&LinkHandle,
                                      SYMBOLIC_LINK_QUERY,
                                      &ObjectAttributes);
    ok_eq_hex(Status, STATUS_OBJECT_NAME_NOT_FOUND);
    ok_eq_pointer(LinkHandle, NULL);
    if (NT_SUCCESS(Status)) ObCloseHandle(LinkHandle, KernelMode);

    /* Create with valid, but not null-terminated LinkTarget */
    LinkTarget = DefaultLinkTarget;
    LinkTarget.MaximumLength = LinkTarget.Length;
    LinkHandle = KmtInvalidPointer;
    Status = ZwCreateSymbolicLinkObject(&LinkHandle,
                                        SYMBOLIC_LINK_QUERY,
                                        &ObjectAttributes,
                                        &LinkTarget);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok(LinkHandle != NULL && LinkHandle != KmtInvalidPointer, "LinkHandle = %p\n", LinkHandle);
    if (skip(NT_SUCCESS(Status), "Failed to create link\n"))
    {
        return;
    }

    /* Now we should be able to open it */
    LinkHandle2 = KmtInvalidPointer;
    Status = ZwOpenSymbolicLinkObject(&LinkHandle2,
                                      SYMBOLIC_LINK_QUERY,
                                      &ObjectAttributes);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok(LinkHandle2 != NULL && LinkHandle2 != KmtInvalidPointer, "LinkHandle = %p\n", LinkHandle2);
    if (!skip(NT_SUCCESS(Status), "Failed to open symlink\n"))
    {
        TestQueryLink(LinkHandle2, &LinkTarget);
        ObCloseHandle(LinkHandle2, KernelMode);
    }

    /* Close it */
    ObCloseHandle(LinkHandle, KernelMode);
}
