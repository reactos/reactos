/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Tests for the NtCompareTokens API
 * COPYRIGHT:       Copyright 2021 George Bișoc <george.bisoc@reactos.org>
 */

#include "precomp.h"

static
HANDLE
GetTokenFromCurrentProcess(VOID)
{
    BOOL Success;
    HANDLE Token;

    Success = OpenProcessToken(GetCurrentProcess(),
                               TOKEN_READ | TOKEN_ADJUST_PRIVILEGES | TOKEN_DUPLICATE,
                               &Token);
    if (!Success)
    {
        skip("OpenProcessToken() has failed to get the process' token (error code: %lu)!\n", GetLastError());
        return NULL;
    }

    return Token;
}

static
HANDLE
GetDuplicateToken(_In_ HANDLE Token)
{
    BOOL Success;
    HANDLE ReturnedToken;

    Success = DuplicateToken(Token, SecurityIdentification, &ReturnedToken);
    if (!Success)
    {
        skip("DuplicateToken() has failed to get the process' token (error code: %lu)!\n", GetLastError());
        return NULL;
    }

    return ReturnedToken;
}

static
VOID
DisableTokenPrivileges(_In_ HANDLE Token)
{
    BOOL Success;

    Success = AdjustTokenPrivileges(Token, TRUE, NULL, 0, NULL, NULL);
    if (!Success)
    {
        skip("AdjustTokenPrivileges() has failed to disable the privileges (error code: %lu)!\n", GetLastError());
        return;
    }
}

START_TEST(NtCompareTokens)
{
    NTSTATUS Status;
    HANDLE ProcessToken = NULL;
    HANDLE DuplicatedToken = NULL;
    BOOLEAN IsEqual = FALSE;

    /* Obtain some tokens from current process */
    ProcessToken = GetTokenFromCurrentProcess();
    DuplicatedToken = GetDuplicateToken(ProcessToken);

    /*
     * Give invalid token handles and don't output
     * the returned value in the last parameter.
     */
    Status = NtCompareTokens(NULL, NULL, NULL);
    ok_hex(Status, STATUS_ACCESS_VIOLATION);

    /*
     * Token handles are valid but don't output
     * the returned value.
     */
    Status = NtCompareTokens(ProcessToken, ProcessToken, NULL);
    ok_hex(Status, STATUS_ACCESS_VIOLATION);

    /* The tokens are the same */
    Status = NtCompareTokens(ProcessToken, ProcessToken, &IsEqual);
    ok_hex(Status, STATUS_SUCCESS);
    ok(IsEqual == TRUE, "Equal tokens expected but they aren't (current value: %u)!\n", IsEqual);

    /* A token is duplicated with equal SIDs and privileges */
    Status = NtCompareTokens(ProcessToken, DuplicatedToken, &IsEqual);
    ok_hex(Status, STATUS_SUCCESS);
    ok(IsEqual == TRUE, "Equal tokens expected but they aren't (current value: %u)!\n", IsEqual);

    /* Disable all the privileges for token. */
    DisableTokenPrivileges(ProcessToken);

    /*
     * The main token has privileges disabled but the
     * duplicated one has them enabled still.
     */
    Status = NtCompareTokens(ProcessToken, DuplicatedToken, &IsEqual);
    ok_hex(Status, STATUS_SUCCESS);
    ok(IsEqual == FALSE, "Tokens mustn't be equal (current value: %u)!\n", IsEqual);

    /* We finished our tests, close the tokens */
    CloseHandle(ProcessToken);
    CloseHandle(DuplicatedToken);
}
