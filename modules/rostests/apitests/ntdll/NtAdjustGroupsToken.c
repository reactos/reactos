/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Tests for the NtAdjustGroupsToken API
 * COPYRIGHT:       Copyright 2021 George Bișoc <george.bisoc@reactos.org>
 */

#include "precomp.h"

static
HANDLE
GetProcessToken(
    _In_ DWORD Access)
{
    BOOL Success;
    HANDLE Token;

    Success = OpenProcessToken(GetCurrentProcess(), Access, &Token);
    if (!Success)
    {
        skip("Failed to open the process' token (error code: %lu)!\n", GetLastError());
        return NULL;
    }

    return Token;
}

START_TEST(NtAdjustGroupsToken)
{
    HANDLE TokenHandle;
    NTSTATUS Status;

    /* Get the token from current process but with incorrect rights */
    TokenHandle = GetProcessToken(TOKEN_DUPLICATE);

    /* We give an invalid handle */
    Status = NtAdjustGroupsToken(NULL,
                                 TRUE,
                                 NULL,
                                 0,
                                 NULL,
                                 NULL);
    ok_hex(Status, STATUS_INVALID_HANDLE);

    /* We're trying to adjust the token's groups with wrong rights */
    Status = NtAdjustGroupsToken(TokenHandle,
                                 TRUE,
                                 NULL,
                                 0,
                                 NULL,
                                 NULL);
    ok_hex(Status, STATUS_ACCESS_DENIED);

    /* Close our handle and open a new one with right access right */
    CloseHandle(TokenHandle);
    TokenHandle = GetProcessToken(TOKEN_ADJUST_GROUPS);

    /* We don't give a list of groups to be adjusted in token */
    Status = NtAdjustGroupsToken(TokenHandle,
                                 FALSE,
                                 NULL,
                                 0,
                                 NULL,
                                 NULL);
    ok_hex(Status, STATUS_INVALID_PARAMETER);

    /* Reset the groups of an access token to default */
    Status = NtAdjustGroupsToken(TokenHandle,
                                 TRUE,
                                 NULL,
                                 0,
                                 NULL,
                                 NULL);
    ok_hex(Status, STATUS_SUCCESS);

    CloseHandle(TokenHandle);
}
