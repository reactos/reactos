/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for RtlValidateUnicodeString
 * COPYRIGHT:   Copyright 2020 Hermes Belusca-Maito
 */

#include "precomp.h"

START_TEST(RtlValidateUnicodeString)
{
    NTSTATUS Status;
    UNICODE_STRING String;
    UNICODE_STRING ValidString = RTL_CONSTANT_STRING(L"My Wild String!");

    /* Start with a valid Unicode string */
    String = ValidString;

    /* Non-zero flags are unsupported! */
    Status = RtlValidateUnicodeString(0, &String);
    ok(Status == STATUS_SUCCESS, "Status = 0x%lx\n", Status);
    Status = RtlValidateUnicodeString(1, &String);
    ok(Status == STATUS_INVALID_PARAMETER, "Status = 0x%lx\n", Status);
    Status = RtlValidateUnicodeString(0xdeadbeef, &String);
    ok(Status == STATUS_INVALID_PARAMETER, "Status = 0x%lx\n", Status);

    /* Empty string is allowed. Test also the flags. */
    RtlInitEmptyUnicodeString(&String, NULL, 0);
    Status = RtlValidateUnicodeString(0, &String);
    ok(Status == STATUS_SUCCESS, "Status = 0x%lx\n", Status);
    Status = RtlValidateUnicodeString(1, &String);
    ok(Status == STATUS_INVALID_PARAMETER, "Status = 0x%lx\n", Status);
    Status = RtlValidateUnicodeString(0xdeadbeef, &String);
    ok(Status == STATUS_INVALID_PARAMETER, "Status = 0x%lx\n", Status);

    // With a non-NULL but empty buffer, and zero lengths.
    RtlInitEmptyUnicodeString(&String, L"", 0);
    Status = RtlValidateUnicodeString(0, &String);
    ok(Status == STATUS_SUCCESS, "Status = 0x%lx\n", Status);
    Status = RtlValidateUnicodeString(1, &String);
    ok(Status == STATUS_INVALID_PARAMETER, "Status = 0x%lx\n", Status);
    Status = RtlValidateUnicodeString(0xdeadbeef, &String);
    ok(Status == STATUS_INVALID_PARAMETER, "Status = 0x%lx\n", Status);

    // With a non-NULL but empty buffer, and zero Length, non-zero MaximumLength.
    RtlInitEmptyUnicodeString(&String, L"", sizeof(WCHAR));
    String.Length = 0;
    Status = RtlValidateUnicodeString(0, &String);
    ok(Status == STATUS_SUCCESS, "Status = 0x%lx\n", Status);
    Status = RtlValidateUnicodeString(1, &String);
    ok(Status == STATUS_INVALID_PARAMETER, "Status = 0x%lx\n", Status);
    Status = RtlValidateUnicodeString(0xdeadbeef, &String);
    ok(Status == STATUS_INVALID_PARAMETER, "Status = 0x%lx\n", Status);

    /* NULL pointer is also allowed! Test also the flags. */
    Status = RtlValidateUnicodeString(0, NULL);
    ok(Status == STATUS_SUCCESS, "Status = 0x%lx\n", Status);
    Status = RtlValidateUnicodeString(1, NULL);
    ok(Status == STATUS_INVALID_PARAMETER, "Status = 0x%lx\n", Status);
    Status = RtlValidateUnicodeString(0xdeadbeef, NULL);
    ok(Status == STATUS_INVALID_PARAMETER, "Status = 0x%lx\n", Status);


    /*
     * Now test invalid strings.
     */

    // NULL buffer but non-zero lengths.
    String = ValidString;
    String.Buffer = NULL;
    Status = RtlValidateUnicodeString(0, &String);
    ok(Status == STATUS_INVALID_PARAMETER, "Status = 0x%lx\n", Status);

    // NULL buffer, zero Length, non-zero MaximumLength.
    String = ValidString;
    String.Buffer = NULL;
    String.Length = 0;
    Status = RtlValidateUnicodeString(0, &String);
    ok(Status == STATUS_INVALID_PARAMETER, "Status = 0x%lx\n", Status);

    // NULL buffer, non-zero Length, zero MaximumLength
    // (tests also the case Length > MaximumLength that must fail).
    String = ValidString;
    String.Buffer = NULL;
    String.MaximumLength = 0;
    Status = RtlValidateUnicodeString(0, &String);
    ok(Status == STATUS_INVALID_PARAMETER, "Status = 0x%lx\n", Status);

    // Non-NULL buffer, non-zero Length, zero MaximumLength.
    String = ValidString;
    String.MaximumLength = 0;
    Status = RtlValidateUnicodeString(0, &String);
    ok(Status == STATUS_INVALID_PARAMETER, "Status = 0x%lx\n", Status);

    /* Non-NULL buffer, odd lengths */

    String = ValidString;
    String.Length--; // Length was already >= 2 so it remains > 0.
    Status = RtlValidateUnicodeString(0, &String);
    ok(Status == STATUS_INVALID_PARAMETER, "Status = 0x%lx\n", Status);

    String.MaximumLength--; // MaximumLength was already >= 2 so it remains > 0.
    Status = RtlValidateUnicodeString(0, &String);
    ok(Status == STATUS_INVALID_PARAMETER, "Status = 0x%lx\n", Status);
}
