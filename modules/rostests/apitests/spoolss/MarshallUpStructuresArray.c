/*
 * PROJECT:     ReactOS Spooler Router API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for MarshallUpStructuresArray
 * COPYRIGHT:   Copyright 2018 Colin Finck (colin@reactos.org)
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <marshalling/marshalling.h>

#define INVALID_POINTER ((PVOID)(ULONG_PTR)0xdeadbeefdeadbeefULL)

START_TEST(MarshallUpStructuresArray)
{
    // Setting cElements to zero should yield success.
    SetLastError(0xDEADBEEF);
    ok(MarshallUpStructuresArray(0, NULL, 0, NULL, 0, FALSE), "MarshallUpStructuresArray returns FALSE!\n");
    ok(GetLastError() == 0xDEADBEEF, "GetLastError returns %lu!\n", GetLastError());

    // Setting cElements non-zero should fail with ERROR_INVALID_PARAMETER.
    SetLastError(0xDEADBEEF);
    ok(!MarshallUpStructuresArray(0, NULL, 1, NULL, 0, FALSE), "MarshallUpStructuresArray returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError returns %lu!\n", GetLastError());

    // This is triggered by both pStructuresArray and pInfo.
    SetLastError(0xDEADBEEF);
    ok(!MarshallUpStructuresArray(0, INVALID_POINTER, 1, NULL, 0, FALSE), "MarshallUpStructuresArray returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError returns %lu!\n", GetLastError());

    SetLastError(0xDEADBEEF);
    ok(!MarshallUpStructuresArray(0, NULL, 1, (const MARSHALLING_INFO*)INVALID_POINTER, 0, FALSE), "MarshallUpStructuresArray returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError returns %lu!\n", GetLastError());

    // More testing is conducted in the MarshallDownStructuresArray test.
}
