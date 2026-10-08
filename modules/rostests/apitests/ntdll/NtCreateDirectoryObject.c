/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Test for NtCreateDirectoryObject
 * COPYRIGHT:   Copyright 2026 Tomas Srnka <tomas.srnka@e2b.dev>
 */

#include "precomp.h"

#define HANDLE_PATTERN ((HANDLE)(ULONG_PTR)0x12345678)

/* On failure the output handle must not carry anything the caller did
 * not put there: it is either untouched or NULL. */
static
VOID
CheckFailedCreate(
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ NTSTATUS ExpectedStatus,
    _In_ PCSTR Description)
{
    NTSTATUS Status;
    HANDLE Handle;
    ULONG i;

    for (i = 0; i < 8; i++)
    {
        Handle = HANDLE_PATTERN;
        Status = NtCreateDirectoryObject(&Handle, DIRECTORY_ALL_ACCESS, ObjectAttributes);
        ok(Status == ExpectedStatus, "%s [%lu]: Status = 0x%lx, expected 0x%lx\n",
           Description, i, Status, ExpectedStatus);
        ok(Handle == HANDLE_PATTERN || Handle == NULL,
           "%s [%lu]: Handle = %p, expected %p or NULL\n",
           Description, i, Handle, HANDLE_PATTERN);
        if (NT_SUCCESS(Status) && Handle && Handle != HANDLE_PATTERN)
            NtClose(Handle);
    }
}

START_TEST(NtCreateDirectoryObject)
{
    NTSTATUS Status;
    HANDLE Parent, Child, Handle;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;

    /* An unnamed directory keeps the test out of the global namespace */
    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);
    Status = NtCreateDirectoryObject(&Parent, DIRECTORY_ALL_ACCESS, &ObjectAttributes);
    ok_ntstatus(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("Cannot create the parent directory\n");
        return;
    }

    RtlInitUnicodeString(&Name, L"Child");
    InitializeObjectAttributes(&ObjectAttributes, &Name, 0, Parent, NULL);
    Child = HANDLE_PATTERN;
    Status = NtCreateDirectoryObject(&Child, DIRECTORY_ALL_ACCESS, &ObjectAttributes);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(Child != NULL && Child != HANDLE_PATTERN, "Child = %p\n", Child);
    if (!NT_SUCCESS(Status))
    {
        skip("Cannot create the child directory\n");
        NtClose(Parent);
        return;
    }

    /* The name exists: creating it again must fail without returning a handle */
    CheckFailedCreate(&ObjectAttributes, STATUS_OBJECT_NAME_COLLISION, "Child");

    /* The root directory always exists */
    RtlInitUnicodeString(&Name, L"\\");
    InitializeObjectAttributes(&ObjectAttributes, &Name, 0, NULL, NULL);
    CheckFailedCreate(&ObjectAttributes, STATUS_OBJECT_NAME_COLLISION, "Root");

    /* With OBJ_OPENIF the existing directory is opened and its handle returned */
    RtlInitUnicodeString(&Name, L"Child");
    InitializeObjectAttributes(&ObjectAttributes, &Name, OBJ_OPENIF, Parent, NULL);
    Handle = HANDLE_PATTERN;
    Status = NtCreateDirectoryObject(&Handle, DIRECTORY_ALL_ACCESS, &ObjectAttributes);
    ok_ntstatus(Status, STATUS_OBJECT_NAME_EXISTS);
    ok(Handle != NULL && Handle != HANDLE_PATTERN, "Handle = %p\n", Handle);
    if (NT_SUCCESS(Status) && Handle != NULL && Handle != HANDLE_PATTERN)
    {
        Status = NtClose(Handle);
        ok_ntstatus(Status, STATUS_SUCCESS);
    }

    Status = NtClose(Child);
    ok_ntstatus(Status, STATUS_SUCCESS);
    Status = NtClose(Parent);
    ok_ntstatus(Status, STATUS_SUCCESS);
}
