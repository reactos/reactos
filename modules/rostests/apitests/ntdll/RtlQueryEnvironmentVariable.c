/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for RtlQueryEnvironmentVariable
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"

typedef
NTSTATUS
NTAPI
FN_RtlQueryEnvironmentVariable(
    _In_opt_ PVOID Environment,
    _In_reads_(NameLength) PWSTR Name,
    _In_ SIZE_T NameLength,
    _Out_writes_(ValueLength) PWSTR Value,
    _In_ SIZE_T ValueLength,
    _Out_ PSIZE_T ReturnLength);

static FN_RtlQueryEnvironmentVariable* pRtlQueryEnvironmentVariable;

START_TEST(RtlQueryEnvironmentVariable)
{
    static WCHAR TestEnv[] = L"TestVar=TestVal\0Foo=4\0EmptyVar\0Bar=8\0\0";
    WCHAR Buffer[32];
    SIZE_T ReturnLength;
    NTSTATUS Status;

    HINSTANCE hinst = GetModuleHandleA("ntdll.dll");
    pRtlQueryEnvironmentVariable = (FN_RtlQueryEnvironmentVariable*)
        GetProcAddress(hinst, "RtlQueryEnvironmentVariable");
    if (pRtlQueryEnvironmentVariable == NULL)
    {
        ok(GetNTVersion() < _WIN32_WINNT_VISTA, "RtlQueryEnvironmentVariable not available on NT6+\n");
        skip("RtlQueryEnvironmentVariable is not available\n");
        return;
    }

    /* Query the TestVar environment variable length */
    Status = pRtlQueryEnvironmentVariable(TestEnv, L"TestVar", 7, NULL, 0, &ReturnLength);
    ok_ntstatus(Status, STATUS_BUFFER_TOO_SMALL);
    ok_eq_size(ReturnLength, 8);

    /* Test a buffer size that is too small */
    memset(Buffer, 0xAB, sizeof(Buffer));
    Status = pRtlQueryEnvironmentVariable(TestEnv, L"TestVar", 7, Buffer, 2, &ReturnLength);
    ok_ntstatus(Status, STATUS_BUFFER_TOO_SMALL);
    ok_eq_size(ReturnLength, 8);
    ok_eq_wchar(Buffer[0], UNICODE_NULL);
    ok_eq_wchar(Buffer[1], 0xABAB);

    /* Test a buffer size that doesn't fit the terminating NULL char */
    memset(Buffer, 0xAB, sizeof(Buffer));
    Status = pRtlQueryEnvironmentVariable(TestEnv, L"TestVar", 7, Buffer, 7, &ReturnLength);
    ok_ntstatus(Status, STATUS_BUFFER_TOO_SMALL);
    ok_eq_size(ReturnLength, 8);
    ok_eq_wchar(Buffer[0], UNICODE_NULL);
    ok_eq_wchar(Buffer[1], 0xABAB);

    /* Test a buffer size that is just large enough */
    memset(Buffer, 0xAB, sizeof(Buffer));
    Status = pRtlQueryEnvironmentVariable(TestEnv, L"TestVar", 7, Buffer, 8, &ReturnLength);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_eq_size(ReturnLength, 7);
    ok_eq_wstr(Buffer, L"TestVal");
    ok_eq_wchar(Buffer[7], UNICODE_NULL);
    ok_eq_wchar(Buffer[8], 0xABAB);

    /* Test a buffer size that is larger than needed */
    memset(Buffer, 0xAB, sizeof(Buffer));
    Status = pRtlQueryEnvironmentVariable(TestEnv, L"TestVar", 7, Buffer, ARRAYSIZE(Buffer), &ReturnLength);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_eq_size(ReturnLength, 7);
    ok_eq_wstr(Buffer, L"TestVal");
    ok_eq_wchar(Buffer[7], UNICODE_NULL);
    ok_eq_wchar(Buffer[8], 0xABAB);

    /* Test a too big variable Length */
    memset(Buffer, 0xAB, sizeof(Buffer));
    Status = pRtlQueryEnvironmentVariable(TestEnv, L"TestVar", 8, Buffer, 8, &ReturnLength);
    ok_ntstatus(Status, STATUS_VARIABLE_NOT_FOUND);
    ok_eq_wchar(Buffer[0], 0xABAB);

    /* Query the EmptyVar environment variable */
    memset(Buffer, 0xAB, sizeof(Buffer));
    Status = pRtlQueryEnvironmentVariable(TestEnv, L"EmptyVar", 8, Buffer, ARRAYSIZE(Buffer), &ReturnLength);
    ok_ntstatus(Status, STATUS_VARIABLE_NOT_FOUND);
    ok_eq_wchar(Buffer[0], 0xABAB);

    /* Query the Bar environment variable */
    memset(Buffer, 0xAB, sizeof(Buffer));
    Status = pRtlQueryEnvironmentVariable(TestEnv, L"Bar", 3, Buffer, 8, &ReturnLength);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_eq_wchar(Buffer[0], L'8');
    ok_eq_wchar(Buffer[1], UNICODE_NULL);
    ok_eq_wchar(Buffer[2], 0xABAB);
}
