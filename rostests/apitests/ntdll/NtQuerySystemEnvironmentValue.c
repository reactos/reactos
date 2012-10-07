/*
 * PROJECT:         ReactOS API Tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for the NtQuerySystemEnvironmentValue.
 * PROGRAMMER:      Hermès BÉLUSCA - MAÏTO <hermes.belusca@sfr.fr>
 */

#define WIN32_NO_STATUS
#include <stdio.h>
#include <wine/test.h>
#include <ndk/ntndk.h>

// Arbitrary-defined constants
#define MIN_BUFFER_LENGTH 4L
#define MAX_BUFFER_LENGTH 2048L

#define COUNT_OF(x) (sizeof((x))/sizeof((x)[0]))

static struct TEST_CASES
{
    NTSTATUS        Result;
    UNICODE_STRING  VariableName;
    BOOLEAN         AdjustPrivileges;
    ULONG           ValueBufferLength;
    ULONG           MinimalExpectedReturnedLength;
    ULONG           MaximalExpectedReturnedLength;
} TestCases[] =
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

static NTSTATUS Test_API(IN  BOOLEAN AdjustPrivileges,
                         IN  PUNICODE_STRING VariableName,
                         OUT PWSTR ValueBuffer,
                         IN  ULONG ValueBufferLength,
                         IN  OUT PULONG ReturnLength OPTIONAL)
{
    NTSTATUS Status;
    BOOLEAN  WasEnabled;

    //
    // Adjust the privileges if asked for (we need to
    // have already administrator privileges to do so).
    //
    if (AdjustPrivileges)
    {
        Status = RtlAdjustPrivilege(SE_SYSTEM_ENVIRONMENT_PRIVILEGE,
                                    TRUE,
                                    FALSE,
                                    &WasEnabled);
        ok(NT_SUCCESS(Status), "RtlAdjustPrivilege failed : 0x%08lx\n", Status);
    }

    //
    // Get the system environment value and set the privilege back.
    //
    Status = NtQuerySystemEnvironmentValue(VariableName,
                                           ValueBuffer,
                                           ValueBufferLength,
                                           ReturnLength);

    if (AdjustPrivileges)
    {
        RtlAdjustPrivilege(SE_SYSTEM_ENVIRONMENT_PRIVILEGE,
                           WasEnabled,
                           FALSE,
                           &WasEnabled);
    }

    return Status;
}

START_TEST(NtQuerySystemEnvironmentValue)
{
    NTSTATUS Status;
    WCHAR ValueBuffer[MAX_BUFFER_LENGTH / sizeof(WCHAR)];
    ULONG ReturnedLength = 0;
    ULONG i;

    for (i = 0 ; i < COUNT_OF(TestCases) ; ++i)
    {
        Status = Test_API(TestCases[i].AdjustPrivileges,
                          &TestCases[i].VariableName,
                          ValueBuffer,
                          TestCases[i].ValueBufferLength,
                          &ReturnedLength);

        ok(Status == TestCases[i].Result,
           "NtQuerySystemEnvironmentValue failed, returned 0x%08lx, expected 0x%08lx\n",
           Status,
           TestCases[i].Result);

        ok( ((TestCases[i].MinimalExpectedReturnedLength <= ReturnedLength) && (ReturnedLength <= TestCases[i].MaximalExpectedReturnedLength)),
            "Returned length %lu, expected between %lu and %lu\n",
            ReturnedLength,
            TestCases[i].MinimalExpectedReturnedLength,
            TestCases[i].MaximalExpectedReturnedLength);
    }
}

/* EOF */
