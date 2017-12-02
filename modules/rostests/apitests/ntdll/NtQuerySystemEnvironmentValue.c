/*
 * PROJECT:         ReactOS API Tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for the NtQuerySystemEnvironmentValue.
 * PROGRAMMER:      Hermès BÉLUSCA - MAÏTO <hermes.belusca@sfr.fr>
 */

#include "precomp.h"

// Arbitrary-defined constants
#define MIN_BUFFER_LENGTH 4L
#define MAX_BUFFER_LENGTH 2048L

#define COUNT_OF(x) (sizeof((x))/sizeof((x)[0]))

typedef struct _TEST_CASE
{
    NTSTATUS        Result;
    UNICODE_STRING  VariableName;
    BOOLEAN         AdjustPrivileges;
    ULONG           ValueBufferLength;
    ULONG           MinimalExpectedReturnedLength;
    ULONG           MaximalExpectedReturnedLength;
} TEST_CASE, *PTEST_CASE;

static TEST_CASE TestCases[] =
{
    //
    // Non-existent variable name.
    //
    {STATUS_PRIVILEGE_NOT_HELD, RTL_CONSTANT_STRING(L"NonExistent"),   FALSE, 0, 0, 0},
    {STATUS_PRIVILEGE_NOT_HELD, RTL_CONSTANT_STRING(L"NonExistent"),   FALSE, MIN_BUFFER_LENGTH, 0, 0},
    {STATUS_PRIVILEGE_NOT_HELD, RTL_CONSTANT_STRING(L"NonExistent"),   FALSE, MAX_BUFFER_LENGTH, 0, 0},
    {STATUS_UNSUCCESSFUL      , RTL_CONSTANT_STRING(L"NonExistent"),   TRUE , 0, 0, 0},
    {STATUS_UNSUCCESSFUL      , RTL_CONSTANT_STRING(L"NonExistent"),   TRUE , MIN_BUFFER_LENGTH, 0, 0},
    {STATUS_UNSUCCESSFUL      , RTL_CONSTANT_STRING(L"NonExistent"),   TRUE , MAX_BUFFER_LENGTH, 0, 0},

    //
    // Existent variable name.
    //
    {STATUS_PRIVILEGE_NOT_HELD, RTL_CONSTANT_STRING(L"LastKnownGood"), FALSE, 0, 0, 0},
    {STATUS_PRIVILEGE_NOT_HELD, RTL_CONSTANT_STRING(L"LastKnownGood"), FALSE, MIN_BUFFER_LENGTH, 0, 0},
    {STATUS_PRIVILEGE_NOT_HELD, RTL_CONSTANT_STRING(L"LastKnownGood"), FALSE, MAX_BUFFER_LENGTH, 0, 0},
    {STATUS_BUFFER_OVERFLOW   , RTL_CONSTANT_STRING(L"LastKnownGood"), TRUE , 0                , MIN_BUFFER_LENGTH, MAX_BUFFER_LENGTH},
    {STATUS_BUFFER_OVERFLOW   , RTL_CONSTANT_STRING(L"LastKnownGood"), TRUE , MIN_BUFFER_LENGTH, MIN_BUFFER_LENGTH, MAX_BUFFER_LENGTH},
    {STATUS_SUCCESS           , RTL_CONSTANT_STRING(L"LastKnownGood"), TRUE , MAX_BUFFER_LENGTH, MIN_BUFFER_LENGTH, MAX_BUFFER_LENGTH},
};

static void Test_API(IN ULONG TestNumber,
                     IN PTEST_CASE TestCase)
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN  WasEnabled = FALSE;
    WCHAR    ValueBuffer[MAX_BUFFER_LENGTH / sizeof(WCHAR)];
    ULONG    ReturnedLength = 0;

    //
    // Adjust the privileges if asked for (we need to
    // have already administrator privileges to do so).
    //
    if (TestCase->AdjustPrivileges)
    {
        Status = RtlAdjustPrivilege(SE_SYSTEM_ENVIRONMENT_PRIVILEGE,
                                    TRUE,
                                    FALSE,
                                    &WasEnabled);
        ok(NT_SUCCESS(Status), "RtlAdjustPrivilege(%lu) failed : 0x%08lx\n", TestNumber, Status);
    }

    //
    // Get the system environment value and set the privilege back.
    //
    Status = NtQuerySystemEnvironmentValue(&TestCase->VariableName,
                                           ValueBuffer,
                                           TestCase->ValueBufferLength,
                                           &ReturnedLength);

    if (TestCase->AdjustPrivileges)
    {
        RtlAdjustPrivilege(SE_SYSTEM_ENVIRONMENT_PRIVILEGE,
                           WasEnabled,
                           FALSE,
                           &WasEnabled);
    }

    //
    // Now check the results.
    //
    ok(Status == TestCase->Result,
       "NtQuerySystemEnvironmentValue(%lu) failed : returned 0x%08lx, expected 0x%08lx\n",
       TestNumber,
       Status,
       TestCase->Result);

    ok( ((TestCase->MinimalExpectedReturnedLength <= ReturnedLength) && (ReturnedLength <= TestCase->MaximalExpectedReturnedLength)),
        "NtQuerySystemEnvironmentValue(%lu) failed : returned length %lu, expected between %lu and %lu\n",
        TestNumber,
        ReturnedLength,
        TestCase->MinimalExpectedReturnedLength,
        TestCase->MaximalExpectedReturnedLength);
}

START_TEST(NtQuerySystemEnvironmentValue)
{
    ULONG i;

    for (i = 0 ; i < COUNT_OF(TestCases) ; ++i)
    {
        Test_API(i, &TestCases[i]);
    }
}

/* EOF */
