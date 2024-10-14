/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Tests for the NtAdjustPrivilegesToken API
 * COPYRIGHT:       Copyright 2021 George Bișoc <george.bisoc@reactos.org>
 */

#include "precomp.h"

static
BOOL
IsPrivilegeEnabled(
    _In_ HANDLE TokenHandle,
    _In_ ULONG Privilege)
{
    PRIVILEGE_SET PrivSet;
    BOOL Result, Success;
    LUID Priv;

    ConvertPrivLongToLuid(Privilege, &Priv);

    PrivSet.PrivilegeCount = 1;
    PrivSet.Control = PRIVILEGE_SET_ALL_NECESSARY;
    PrivSet.Privilege[0].Luid = Priv;
    PrivSet.Privilege[0].Attributes = 0;

    Success = PrivilegeCheck(TokenHandle, &PrivSet, &Result);
    if (!Success)
    {
        skip("Failed to check the privilege (error code: %lu)\n", GetLastError());
        return FALSE;
    }

    return Result;
}

static
VOID
AdjustEnableDefaultPriv(VOID)
{
    NTSTATUS Status;
    HANDLE Token;
    TOKEN_PRIVILEGES Priv;
    BOOL Success, IsEnabled;
    LUID PrivLuid;

    Success = OpenProcessToken(GetCurrentProcess(),
                               TOKEN_READ | TOKEN_ADJUST_PRIVILEGES,
                               &Token);
    if (!Success)
    {
        skip("OpenProcessToken() has failed to get the process' token (error code: %lu)!\n", GetLastError());
        return;
    }

    Success = LookupPrivilegeValueW(NULL, L"SeImpersonatePrivilege", &PrivLuid);
    if (!Success)
    {
        skip("LookupPrivilegeValueW() has failed to locate the privilege value (error code: %lu)!\n", GetLastError());
        return;
    }

    Priv.PrivilegeCount = 1;
    Priv.Privileges[0].Luid = PrivLuid;
    Priv.Privileges[0].Attributes = 0;

    IsEnabled = IsPrivilegeEnabled(Token, SE_IMPERSONATE_PRIVILEGE);
    trace("The privilege is %s!\n", IsEnabled ? "enabled" : "disabled");

    Status = NtAdjustPrivilegesToken(Token,
                                     FALSE,
                                     &Priv,
                                     0,
                                     NULL,
                                     NULL);
    ok_hex(Status, STATUS_SUCCESS);

    IsEnabled = IsPrivilegeEnabled(Token, SE_IMPERSONATE_PRIVILEGE);
    trace("The privilege is %s!\n", IsEnabled ? "enabled" : "disabled");

    CloseHandle(Token);
}

START_TEST(NtAdjustPrivilegesToken)
{
    AdjustEnableDefaultPriv();
}
