/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Tests for the NtFilterToken API
 * COPYRIGHT:       Copyright 2021 George Bișoc <george.bisoc@reactos.org>
 */

#include "precomp.h"

static
HANDLE
GetTokenProcess(VOID)
{
    BOOL Success;
    HANDLE Token;

    Success = OpenProcessToken(GetCurrentProcess(),
                               TOKEN_DUPLICATE | TOKEN_QUERY,
                               &Token);
    if (!Success)
    {
        skip("GetTokenProcess() has failed to get the process' token (error code: %lu)!\n", GetLastError());
        return NULL;
    }

    return Token;
}

START_TEST(NtFilterToken)
{
    NTSTATUS Status;
    HANDLE FilteredToken, Token;
    TOKEN_PRIVILEGES Priv;
    LUID PrivLuid;
    ULONG Size;
    PTOKEN_STATISTICS TokenStats;

    /* We don't give a token */
    Status = NtFilterToken(NULL,
                           0,
                           NULL,
                           NULL,
                           NULL,
                           &FilteredToken);
    ok_hex(Status, STATUS_INVALID_HANDLE);

    /* Get the token from process now */
    Token = GetTokenProcess();

    /* We don't give any privileges to delete */
    Status = NtFilterToken(Token,
                           0,
                           NULL,
                           NULL,
                           NULL,
                           &FilteredToken);
    ok_hex(Status, STATUS_SUCCESS);

    /* Query the total size to hold the statistics */
    Status = NtQueryInformationToken(Token, TokenStatistics, NULL, 0, &Size);
    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_TOO_SMALL)
    {
        skip("Failed to query the total size for token statistics structure! (Status -> 0x%lx)\n", Status);
        return;
    }

    /* Total size queried, time to allocate our buffer based on that size */
    TokenStats = RtlAllocateHeap(RtlGetProcessHeap(), 0, Size);
    if (TokenStats == NULL)
    {
        skip("Failed to allocate our token statistics buffer!\n");
        return;
    }

    /* Time to query our token statistics, prior disabling token's privileges */
    Status = NtQueryInformationToken(Token, TokenStatistics, TokenStats, Size, &Size);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to query the token statistics! (Status -> 0x%lx)\n", Status);
        return;
    }

    trace("Number of privileges before token filtering -- %lu\n\n", TokenStats->PrivilegeCount);

    /* Disable the privileges and make the token a safer inert one */
    Status = NtFilterToken(Token,
                           DISABLE_MAX_PRIVILEGE | SANDBOX_INERT,
                           NULL,
                           NULL,
                           NULL,
                           &FilteredToken);
    ok_hex(Status, STATUS_SUCCESS);

    /* We've disabled privileges, query the stats again */
    Status = NtQueryInformationToken(FilteredToken, TokenStatistics, TokenStats, Size, &Size);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to query the token statistics! (Status -> 0x%lx)\n", Status);
        return;
    }

    trace("Number of privileges after token filtering (privileges disabled with DISABLE_MAX_PRIVILEGE) -- %lu\n\n", TokenStats->PrivilegeCount);

    /* Close the filtered token and do another test */
    CloseHandle(FilteredToken);

    /* Fill in a privilege to delete */
    Priv.PrivilegeCount = 1;

    ConvertPrivLongToLuid(SE_BACKUP_PRIVILEGE, &PrivLuid);
    Priv.Privileges[0].Luid = PrivLuid;
    Priv.Privileges[0].Attributes = 0;

    /* Delete the privileges */
    Status = NtFilterToken(Token,
                           0,
                           NULL,
                           &Priv,
                           NULL,
                           &FilteredToken);
    ok_hex(Status, STATUS_SUCCESS);

    /* We've deleted a privilege, query the stats again */
    Status = NtQueryInformationToken(FilteredToken, TokenStatistics, TokenStats, Size, &Size);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to query the token statistics! (Status -> 0x%lx)\n", Status);
        return;
    }

    trace("Number of privileges after token filtering (manually deleted privilege) -- %lu\n\n", TokenStats->PrivilegeCount);

    /* We're done */
    RtlFreeHeap(RtlGetProcessHeap(), 0, TokenStats);
    CloseHandle(Token);
    CloseHandle(FilteredToken);
}
