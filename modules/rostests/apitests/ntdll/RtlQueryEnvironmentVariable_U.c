/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for RtlQueryEnvironmentVariable_U
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"

START_TEST(RtlQueryEnvironmentVariable_U)
{
    static WCHAR TestEnvironment[] = L"TestVar=TestVal\0Foo=4\0EmptyVar\0Bar=8\0\0";
    WCHAR Buffer[32];
    UNICODE_STRING NameString, ValueString;
    NTSTATUS Status;

    /* Query the TestVar environment variable length */
    RtlInitUnicodeString(&NameString, L"TestVar");
    RtlInitEmptyUnicodeString(&ValueString, NULL, 0);
    Status = RtlQueryEnvironmentVariable_U(TestEnvironment, &NameString, &ValueString);
    ok_ntstatus(Status, STATUS_BUFFER_TOO_SMALL);
    ok_eq_pointer(ValueString.Buffer, NULL);
    ok_eq_ulong(ValueString.Length, 7 * sizeof(WCHAR));
    ok_eq_ulong(ValueString.MaximumLength, 0);

    /* Test passing an empty sized buffer */
    memset(Buffer, 0xAB, sizeof(Buffer));
    RtlInitEmptyUnicodeString(&ValueString, Buffer, 0);
    Status = RtlQueryEnvironmentVariable_U(TestEnvironment, &NameString, &ValueString);
    ok_ntstatus(Status, STATUS_BUFFER_TOO_SMALL);
    ok_eq_pointer(ValueString.Buffer, Buffer);
    ok_eq_ulong(ValueString.Length, 7 * sizeof(WCHAR));
    ok_eq_ulong(ValueString.MaximumLength, 0);
    ok_eq_wchar(Buffer[0], 0xABAB);

    /* Test a buffer size that is too small */
    memset(Buffer, 0xAB, sizeof(Buffer));
    RtlInitEmptyUnicodeString(&ValueString, Buffer, 2 * sizeof(WCHAR));
    Status = RtlQueryEnvironmentVariable_U(TestEnvironment, &NameString, &ValueString);
    ok_ntstatus(Status, STATUS_BUFFER_TOO_SMALL);
    ok_eq_pointer(ValueString.Buffer, Buffer);
    ok_eq_ulong(ValueString.Length, 7 * sizeof(WCHAR));
    ok_eq_ulong(ValueString.MaximumLength, 2 * sizeof(WCHAR));
    ok(Buffer[0] == UNICODE_NULL || broken(/* Windows 2003 */ Buffer[0] == 0xABAB), "Buffer[0] = 0x%x\n", Buffer[0]);
    ok_eq_wchar(Buffer[1], 0xABAB);

    /* Test a buffer size that doesn't fit the terminating NULL char */
    memset(Buffer, 0xAB, sizeof(Buffer));
    RtlInitEmptyUnicodeString(&ValueString, Buffer, 7 * sizeof(WCHAR));
    Status = RtlQueryEnvironmentVariable_U(TestEnvironment, &NameString, &ValueString);
    ok(Status == STATUS_BUFFER_TOO_SMALL || broken(/* Windows 2003 */ Status == STATUS_SUCCESS), "Status = 0x%lx\n", Status);
    ok_eq_pointer(ValueString.Buffer, Buffer);
    ok_eq_ulong(ValueString.Length, 7 * sizeof(WCHAR));
    ok_eq_ulong(ValueString.MaximumLength, 7 * sizeof(WCHAR));
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        ok_eq_wchar(Buffer[0], UNICODE_NULL);
        ok_eq_wchar(Buffer[1], 0xABAB);
    }

    /* Test a buffer size that is just large enough */
    memset(Buffer, 0xAB, sizeof(Buffer));
    RtlInitEmptyUnicodeString(&ValueString, Buffer, 8 * sizeof(WCHAR));
    Status = RtlQueryEnvironmentVariable_U(TestEnvironment, &NameString, &ValueString);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_eq_pointer(ValueString.Buffer, Buffer);
    ok_eq_ulong(ValueString.Length, 7 * sizeof(WCHAR));
    ok_eq_ulong(ValueString.MaximumLength, 8 * sizeof(WCHAR));
    ok_eq_wstr(Buffer, L"TestVal");
    ok_eq_wchar(Buffer[7], UNICODE_NULL);
    ok_eq_wchar(Buffer[8], 0xABAB);

    /* Test a buffer size that is larger than needed */
    memset(Buffer, 0xAB, sizeof(Buffer));
    RtlInitEmptyUnicodeString(&ValueString, Buffer, sizeof(Buffer));
    Status = RtlQueryEnvironmentVariable_U(TestEnvironment, &NameString, &ValueString);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_eq_pointer(ValueString.Buffer, Buffer);
    ok_eq_ulong(ValueString.Length, 7 * sizeof(WCHAR));
    ok_eq_ulong(ValueString.MaximumLength, sizeof(Buffer));
    ok_eq_wstr(Buffer, L"TestVal");
    ok_eq_wchar(Buffer[7], UNICODE_NULL);
    ok_eq_wchar(Buffer[8], 0xABAB);

    /* Test a too big variable Length */
    memset(Buffer, 0xAB, sizeof(Buffer));
    NameString.Length += sizeof(WCHAR);
    RtlInitEmptyUnicodeString(&ValueString, Buffer, sizeof(Buffer));
    Status = RtlQueryEnvironmentVariable_U(TestEnvironment, &NameString, &ValueString);
    ok_ntstatus(Status, STATUS_VARIABLE_NOT_FOUND);
    ok_eq_wchar(Buffer[0], 0xABAB);

    /* Query the EmptyVar environment variable */
    memset(Buffer, 0xAB, sizeof(Buffer));
    RtlInitUnicodeString(&NameString, L"EmptyVar");
    RtlInitEmptyUnicodeString(&ValueString, Buffer, sizeof(Buffer));
    Status = RtlQueryEnvironmentVariable_U(TestEnvironment, &NameString, &ValueString);
    ok_ntstatus(Status, STATUS_VARIABLE_NOT_FOUND);
    ok_eq_pointer(ValueString.Buffer, Buffer);
    ok_eq_ulong(ValueString.Length, 0);
    ok_eq_ulong(ValueString.MaximumLength, sizeof(Buffer));
    ok_eq_wchar(Buffer[0], 0xABAB);

    /* Query the Bar environment variable */
    memset(Buffer, 0xAB, sizeof(Buffer));
    RtlInitUnicodeString(&NameString, L"Bar");
    RtlInitEmptyUnicodeString(&ValueString, Buffer, sizeof(Buffer));
    Status = RtlQueryEnvironmentVariable_U(TestEnvironment, &NameString, &ValueString);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_eq_pointer(ValueString.Buffer, Buffer);
    ok_eq_ulong(ValueString.Length, 1 * sizeof(WCHAR));
    ok_eq_ulong(ValueString.MaximumLength, sizeof(Buffer));
    ok_eq_wstr(Buffer, L"8");
    ok_eq_wchar(Buffer[1], UNICODE_NULL);
    ok_eq_wchar(Buffer[2], 0xABAB);

    /* Test NULL Variable name */
    memset(Buffer, 0xAB, sizeof(Buffer));
    RtlInitEmptyUnicodeString(&NameString, NULL, 0);
    RtlInitEmptyUnicodeString(&ValueString, Buffer, sizeof(Buffer));
    Status = RtlQueryEnvironmentVariable_U(TestEnvironment, &NameString, &ValueString);
    ok_ntstatus(Status, STATUS_VARIABLE_NOT_FOUND);
    ok_eq_pointer(ValueString.Buffer, Buffer);
    ok_eq_ulong(ValueString.Length, 0);
    ok_eq_ulong(ValueString.MaximumLength, sizeof(Buffer));
    ok_eq_wchar(Buffer[0], 0xABAB);

}
