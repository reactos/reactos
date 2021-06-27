/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Test for RtlpApplyLengthFunction
 * COPYRIGHT:   Copyright 2021 Mark Jansen <mark.jansen@reactos.org>
 */

#include "precomp.h"
#include <ntstrsafe.h>


NTSTATUS
NTAPI
RtlpApplyLengthFunction(IN ULONG Flags,
    IN ULONG Type,
    IN PVOID UnicodeStringOrUnicodeStringBuffer,
    IN NTSTATUS(NTAPI*LengthFunction)(ULONG, PUNICODE_STRING, PULONG));


NTSTATUS NTAPI LengthFunctionFail(ULONG Unknown, PUNICODE_STRING String, PULONG Length)
{
    ok_int(*Length, 0);
    /* Show that this is ignored when an error is returned */
    *Length = 3;
    return STATUS_INVALID_ACCOUNT_NAME;
}

NTSTATUS NTAPI LengthFunctionOk(ULONG Unknown, PUNICODE_STRING String, PULONG Length)
{
    ok_int(*Length, 0);
    *Length = 4;
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI LengthFunctionReturn1(ULONG Unknown, PUNICODE_STRING String, PULONG Length)
{
    ok_int(*Length, 0);
    *Length = 4;
    return (NTSTATUS)1;
}

NTSTATUS NTAPI LengthFunctionCopyLen(ULONG Unknown, PUNICODE_STRING String, PULONG Length)
{
    /* Use Buffer as length, to show that the function does not interpret the contents at all */
    *Length = (ULONG)(ULONG_PTR)String->Buffer;
    return STATUS_SUCCESS;
}


START_TEST(RtlpApplyLengthFunction)
{
    NTSTATUS Status;
    /* Show that RtlpApplyLengthFunction does not interpret anything in the UNICODE_STRING */
    UNICODE_STRING String = { 1, 2, (PWSTR)3 };
    RTL_UNICODE_STRING_BUFFER Buffer;
    WCHAR StaticBuffer[10] = { 0 };
    ULONG n;

    Status = RtlpApplyLengthFunction(0, 0, NULL, LengthFunctionFail);
    ok_int(String.Length, 1);
    ok_hex(Status, STATUS_INVALID_PARAMETER);

    Status = RtlpApplyLengthFunction(0, 0, &String, LengthFunctionFail);
    ok_int(String.Length, 1);
    ok_hex(Status, STATUS_INVALID_PARAMETER);

    /* Show that no flag is accepted */
    for (n = 0; n < 32; ++n)
    {
        Status = RtlpApplyLengthFunction((1 << n), sizeof(String), &String, LengthFunctionFail);
        ok_int(String.Length, 1);
        ok_hex(Status, STATUS_INVALID_PARAMETER);
    }

    Status = RtlpApplyLengthFunction(0, sizeof(String), &String, NULL);
    ok_int(String.Length, 1);
    ok_hex(Status, STATUS_INVALID_PARAMETER);

    /* Still Length 1 when the function returns an error */
    Status = RtlpApplyLengthFunction(0, sizeof(String), &String, LengthFunctionFail);
    ok_int(String.Length, 1);
    ok_hex(Status, STATUS_INVALID_ACCOUNT_NAME);

    Status = RtlpApplyLengthFunction(0, sizeof(String), &String, LengthFunctionOk);
    ok_int(String.Length, 8);   /* Value returned from LengthFunction is multiplied by sizeof(WCHAR) */
    ok_hex(Status, STATUS_SUCCESS);

    String.Length = 1;
    Status = RtlpApplyLengthFunction(0, sizeof(String), &String, LengthFunctionReturn1);
    ok_int(String.Length, 8);
    ok_hex(Status, STATUS_SUCCESS); /* Returns STATUS_SUCCESS regardless of success code from the function */

    /* Show max length */
    String.Buffer = (PWCHAR)UNICODE_STRING_MAX_CHARS;
    String.Length = 2;
    Status = RtlpApplyLengthFunction(0, sizeof(String), &String, LengthFunctionCopyLen);
    ok_int(String.Length, UNICODE_STRING_MAX_CHARS * sizeof(WCHAR));
    ok_hex(Status, STATUS_SUCCESS);

    String.Buffer = (PWCHAR)(UNICODE_STRING_MAX_CHARS + 1);
    String.Length = 2;
    Status = RtlpApplyLengthFunction(0, sizeof(String), &String, LengthFunctionCopyLen);
    ok_int(String.Length, 2);       /* Unchanged */
    ok_hex(Status, STATUS_NAME_TOO_LONG);

    /* Now try it with the RTL_UNICODE_STRING_BUFFER, this works fine on 2k3 but not on Win10!! */
    RtlInitBuffer(&Buffer.ByteBuffer, (PUCHAR)StaticBuffer, sizeof(StaticBuffer));
    /* In this case the Buffer is modified, so we should have a valid UNICODE_STRING! */
    Buffer.String.Length = 5;
    Buffer.String.MaximumLength = Buffer.ByteBuffer.StaticSize;
    Buffer.String.Buffer = (PWSTR)Buffer.ByteBuffer.Buffer;
    wcscpy(StaticBuffer, L"123456789");

    /* Show that no flag is accepted */
    for (n = 0; n < 32; ++n)
    {
        Status = RtlpApplyLengthFunction((1 << n), sizeof(Buffer), &Buffer, LengthFunctionFail);
        ok_int(Buffer.String.Length, 5);
        ok_hex(Status, STATUS_INVALID_PARAMETER);
        ok_wstr(StaticBuffer, L"123456789");
    }

    /* Still Length 1 when the function returns an error */
    Status = RtlpApplyLengthFunction(0, sizeof(Buffer), &Buffer, LengthFunctionFail);
    ok_int(Buffer.String.Length, 5);
    ok_hex(Status, STATUS_INVALID_ACCOUNT_NAME);
    ok_wstr(StaticBuffer, L"123456789");

    Status = RtlpApplyLengthFunction(0, sizeof(Buffer), &Buffer, LengthFunctionOk);
    ok_int(Buffer.String.Length, 8);   /* Value returned from LengthFunction is multiplied by sizeof(WCHAR) */
    ok_hex(Status, STATUS_SUCCESS);
    ok_wstr(StaticBuffer, L"1234");     /* Buffer is truncated */
    ok_wstr(StaticBuffer + 5, L"6789"); /* Rest is not overwritten*/

    Buffer.String.Length = 1;
    wcscpy(StaticBuffer, L"123456789");
    Status = RtlpApplyLengthFunction(0, sizeof(Buffer), &Buffer, LengthFunctionReturn1);
    ok_int(Buffer.String.Length, 8);
    ok_hex(Status, STATUS_SUCCESS); /* Returns STATUS_SUCCESS regardless of success code from the function */
    ok_wstr(StaticBuffer, L"1234");     /* Buffer is truncated */
    ok_wstr(StaticBuffer + 5, L"6789"); /* Rest is not overwritten*/
}

