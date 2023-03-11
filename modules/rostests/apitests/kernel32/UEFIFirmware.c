/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for UEFI Firmware functions
 * COPYRIGHT:   Copyright 2023 Ratin Gao <ratin@knsoft.org>
 */

#include "precomp.h"

#include <ndk/psfuncs.h>
#include <ndk/setypes.h>
#include <ndk/sefuncs.h>
#include <ndk/obfuncs.h>

#define _A2W(quote) __A2W(quote)
#define __A2W(quote) L##quote

#define EFI_TEST_GUID_STRING "{8768B7AC-F82F-4120-B093-30DFA27DA3B5}"
#define EFI_TEST_VARIABLE_NAME "RosUefiVarTest"

#define EFI_DUMMY_NAMESPACE_GUID_STRING "{00000000-0000-0000-0000-000000000000}"
#define EFI_DUMMY_VARIABLE_NAME ""

static ULONG RandomSeed;
static DWORD EfiVariableValue;

static VOID test_GetFirmwareType(BOOL bIsUEFI)
{
#if (_WIN32_WINNT >= 0x0602)
    BOOL bResult;
    FIRMWARE_TYPE FirmwareType = FirmwareTypeUnknown, FirmwareExpect;

    /* Test GetFirmwareType, should return FirmwareTypeBios or FirmwareTypeUefi */
    bResult = GetFirmwareType(&FirmwareType);

    ok(bResult,
       "GetFirmwareType failed with error: 0x%08lX\n",
       GetLastError());

    if (!bResult)
        return;

    FirmwareExpect = (bIsUEFI ? FirmwareTypeUefi : FirmwareTypeBios);
    ok(FirmwareType == FirmwareExpect,
       "FirmwareType is %d, but %d is expected.\n",
       FirmwareType, FirmwareExpect);
#else
    skip("This test can be run only when compiled for NT >= 6.2.\n");
#endif
}

START_TEST(UEFIFirmware)
{
    BOOL bResult, bResultTemp, bIsUEFI;
    DWORD dwError, dwErrorTemp, dwLength, dwLengthTemp, dwValue;
    HANDLE hToken;
    TOKEN_PRIVILEGES Privilege;
    NTSTATUS Status;
    ULONG ReturnLength;

    /*
     * Check whether this test runs on legacy BIOS-based or UEFI system
     * by calling GetFirmwareEnvironmentVariable with dummy name and GUID.
     * It should fail with ERROR_INVALID_FUNCTION on the former and
     * fail with another error on the latter.
     */
    dwLength = GetFirmwareEnvironmentVariableW(_A2W(EFI_DUMMY_VARIABLE_NAME),
                                               _A2W(EFI_DUMMY_NAMESPACE_GUID_STRING),
                                               NULL,
                                               0);
    dwError = GetLastError();
    ok(dwLength == 0, "dwLength = %lu, expected 0\n", dwLength);

    bIsUEFI = (dwLength == 0 && dwError != ERROR_INVALID_FUNCTION);
    test_GetFirmwareType(bIsUEFI);
    if (!bIsUEFI)
    {
        skip("Skipping tests that require UEFI environment.\n");
        return;
    }

    /* Test ANSI function too */
    dwLengthTemp = GetFirmwareEnvironmentVariableA(EFI_DUMMY_VARIABLE_NAME,
                                                   EFI_DUMMY_NAMESPACE_GUID_STRING,
                                                   NULL,
                                                   0);
    dwErrorTemp = GetLastError();
    ok(dwLengthTemp == dwLength && dwErrorTemp == dwError,
       "dwLength = %lu, LastError = %lu, expected bResult = %lu, LastError = %lu\n",
       dwLengthTemp,
       dwErrorTemp,
       dwLength,
       dwError);

    /* Generate a random variable value to be used in this test */
    RandomSeed = GetTickCount();
    EfiVariableValue = RtlRandom(&RandomSeed);

    /* Try to set firmware variable, should fail with ERROR_PRIVILEGE_NOT_HELD,
     * because no SE_SYSTEM_ENVIRONMENT_NAME privilege enabled by default. */
    bResult = SetFirmwareEnvironmentVariableW(_A2W(EFI_TEST_VARIABLE_NAME),
                                              _A2W(EFI_TEST_GUID_STRING),
                                              &EfiVariableValue,
                                              sizeof(EfiVariableValue));
    dwError = GetLastError();
    ok(!bResult && dwError == ERROR_PRIVILEGE_NOT_HELD,
       "bResult = %d, LastError = %lu, expected bResult = 0, LastError = ERROR_PRIVILEGE_NOT_HELD\n",
       bResult,
       dwError);

    /* Test ANSI function too */
    bResultTemp = SetFirmwareEnvironmentVariableA(EFI_TEST_VARIABLE_NAME,
                                                  EFI_TEST_GUID_STRING,
                                                  &EfiVariableValue,
                                                  sizeof(EfiVariableValue));
    dwErrorTemp = GetLastError();
    ok(bResultTemp == bResult && dwErrorTemp == dwError,
       "bResult = %d, LastError = %lu, expected bResult = %d, LastError = %lu\n",
       bResultTemp,
       dwErrorTemp,
       bResult,
       dwError);

    /* Enable SE_SYSTEM_ENVIRONMENT_NAME privilege required by the following tests */
    bResult = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken);
    if (!bResult)
    {
        skip("OpenProcessToken failed with error: 0x%08lX, aborting.\n", GetLastError());
        return;
    }
    Privilege.PrivilegeCount = 1;
    Privilege.Privileges[0].Luid = RtlConvertUlongToLuid(SE_SYSTEM_ENVIRONMENT_PRIVILEGE);
    Privilege.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    Status = NtAdjustPrivilegesToken(hToken, FALSE, &Privilege, sizeof(Privilege), NULL, &ReturnLength);
    if (Status != STATUS_SUCCESS)
    {
        skip("NtAdjustPrivilegesToken failed with status: 0x%08lX, aborting.\n", Status);
        NtClose(hToken);
        return;
    }

    /* Set our test variable to UEFI firmware */
    bResult = SetFirmwareEnvironmentVariableW(_A2W(EFI_TEST_VARIABLE_NAME),
                                              _A2W(EFI_TEST_GUID_STRING),
                                              &EfiVariableValue,
                                              sizeof(EfiVariableValue));
    ok(bResult,
       "SetFirmwareEnvironmentVariableW failed with error: 0x%08lX\n",
       GetLastError());
    if (bResult)
    {
        /* Get the variable back and verify */
        dwLength = GetFirmwareEnvironmentVariableW(_A2W(EFI_TEST_VARIABLE_NAME),
                                                   _A2W(EFI_TEST_GUID_STRING),
                                                   &dwValue,
                                                   sizeof(dwValue));
        ok(dwLength,
           "GetFirmwareEnvironmentVariableW failed with error: 0x%08lX\n",
           GetLastError());
        if (dwLength)
        {
            ok(dwLength == sizeof(EfiVariableValue) && dwValue == EfiVariableValue,
               "Retrieved variable different from what we set, "
               "dwLength = %lu, dwValue = %lu, expected dwLength = %u, dwValue = %lu",
               dwLength,
               dwValue,
               sizeof(EfiVariableValue),
               EfiVariableValue);
        }
    }

    /* Change variable value and test ANSI function */
    EfiVariableValue = RtlRandom(&RandomSeed);
    bResult = SetFirmwareEnvironmentVariableA(EFI_TEST_VARIABLE_NAME,
                                              EFI_TEST_GUID_STRING,
                                              &EfiVariableValue,
                                              sizeof(EfiVariableValue));
    ok(bResult,
       "SetFirmwareEnvironmentVariableA failed with error: 0x%08lX\n",
       GetLastError());
    if (bResult)
    {
        /* Get the variable back and verify */
        dwLength = GetFirmwareEnvironmentVariableA(EFI_TEST_VARIABLE_NAME,
                                                   EFI_TEST_GUID_STRING,
                                                   &dwValue,
                                                   sizeof(dwValue));
        ok(dwLength,
           "GetFirmwareEnvironmentVariableA failed with error: 0x%08lX\n",
           GetLastError());
        if (dwLength)
        {
            ok(dwLength == sizeof(EfiVariableValue) && dwValue == EfiVariableValue,
               "Retrieved variable different from what we set, "
               "dwLength = %lu, dwValue = %lu, expected dwLength = %u, dwValue = %lu",
               dwLength,
               dwValue,
               sizeof(EfiVariableValue),
               EfiVariableValue);
        }
    }

    /* Delete the variable */
    bResult = SetFirmwareEnvironmentVariableW(_A2W(EFI_TEST_VARIABLE_NAME),
                                              _A2W(EFI_TEST_GUID_STRING),
                                              NULL,
                                              0);
    ok(bResult,
       "SetFirmwareEnvironmentVariableW failed with error: 0x%08lX\n",
       GetLastError());
    if (bResult)
    {
        dwLength = GetFirmwareEnvironmentVariableW(_A2W(EFI_TEST_VARIABLE_NAME),
                                                   _A2W(EFI_TEST_GUID_STRING),
                                                   &dwValue,
                                                   sizeof(dwValue));
        ok(dwLength == 0, "SetFirmwareEnvironmentVariableW didn't delete the variable!\n");
    }

    /* Restore variable and test ANSI function */
    bResult = SetFirmwareEnvironmentVariableW(_A2W(EFI_TEST_VARIABLE_NAME),
                                              _A2W(EFI_TEST_GUID_STRING),
                                              &EfiVariableValue,
                                              sizeof(EfiVariableValue));
    if (!bResult)
    {
        skip("SetFirmwareEnvironmentVariableW failed to restore variable with error: 0x%08lX\n",
             GetLastError());
        goto _exit;
    }
    bResult = SetFirmwareEnvironmentVariableA(EFI_TEST_VARIABLE_NAME,
                                              EFI_TEST_GUID_STRING,
                                              NULL,
                                              0);
    ok(bResult,
       "SetFirmwareEnvironmentVariableA failed with error: 0x%08lX\n",
       GetLastError());
    if (bResult)
    {
        dwLength = GetFirmwareEnvironmentVariableA(EFI_TEST_VARIABLE_NAME,
                                                   EFI_TEST_GUID_STRING,
                                                   &dwValue,
                                                   sizeof(dwValue));
        ok(dwLength == 0, "SetFirmwareEnvironmentVariableA didn't delete the variable!\n");
    }

_exit:
    /* Restore the privilege */
    Privilege.Privileges[0].Attributes = 0;
    Status = NtAdjustPrivilegesToken(hToken, FALSE, &Privilege, sizeof(Privilege), NULL, &ReturnLength);
    ok(Status == STATUS_SUCCESS, "NtAdjustPrivilegesToken failed with status: 0x%08lX\n", Status);
    NtClose(hToken);
}
