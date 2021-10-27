/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Tests for the NtDuplicateToken API
 * COPYRIGHT:       Copyright 2021 George Bișoc <george.bisoc@reactos.org>
 */

#include "precomp.h"

static
HANDLE
OpenTokenFromProcess(VOID)
{
    BOOL Success;
    HANDLE Token;

    Success = OpenProcessToken(GetCurrentProcess(),
                               TOKEN_READ | TOKEN_ADJUST_PRIVILEGES | TOKEN_ADJUST_GROUPS | TOKEN_DUPLICATE | TOKEN_QUERY,
                               &Token);
    if (!Success)
    {
        ok(FALSE, "Failed to get the process' token (error code: %lu)\n", GetLastError());
        return NULL;
    }

    return Token;
}

static
BOOL
DisablePrivilege(
    _In_ HANDLE Token,
    _In_ LPCWSTR PrivilegeName)
{
    TOKEN_PRIVILEGES TokenPriv;
    LUID PrivLuid;
    BOOL Success;

    Success = LookupPrivilegeValueW(NULL, PrivilegeName, &PrivLuid);
    if (!Success)
    {
        ok(FALSE, "Failed to locate the privilege value (error code: %lu)\n", GetLastError());
        return FALSE;
    }

    TokenPriv.PrivilegeCount = 1;
    TokenPriv.Privileges[0].Luid = PrivLuid;
    TokenPriv.Privileges[0].Attributes = 0;

    SetLastError(0xdeadbeef);
    Success = AdjustTokenPrivileges(Token,
                                    FALSE,
                                    &TokenPriv,
                                    0,
                                    NULL,
                                    NULL);
    if (!Success || GetLastError() != ERROR_SUCCESS)
    {
        ok(FALSE, "Failed to adjust privileges of token (error code: %lu)\n", GetLastError());
        return FALSE;
    }

    return TRUE;
}

static
VOID
DuplicateTokenAsEffective(VOID)
{
    NTSTATUS Status;
    ULONG Size;
    HANDLE TokenHandle;
    HANDLE DuplicatedTokenHandle = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PTOKEN_STATISTICS TokenStats = NULL;

    /* We give a bogus invalid handle */
    Status = NtDuplicateToken(NULL,
                              0,
                              NULL,
                              TRUE,
                              TokenPrimary,
                              NULL);
    ok_ntstatus(Status, STATUS_ACCESS_VIOLATION);

    /* Initialize the object attributes for token duplication */
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               0,
                               NULL,
                               NULL);

    /* Get the token from process and begin the tests */
    TokenHandle = OpenTokenFromProcess();
    if (TokenHandle == NULL)
    {
        skip("TokenHandle is NULL\n");
        return;
    }

    /*
     * Disable a privilege, the impersonation privilege for example.
     * Why we're doing this is because such privilege is enabled
     * by default and we'd want to know what the kernel does
     * at the moment of removing disabled privileges during making
     * the token effective, with this potential privilege being
     * disabled by ourselves.
     */
    if (!DisablePrivilege(TokenHandle, L"SeImpersonatePrivilege"))
    {
        skip("DisablePrivilege() failed\n");
        goto cleanup;
    }

    /* Query the total size of the token statistics structure */
    Status = NtQueryInformationToken(TokenHandle, TokenStatistics, NULL, 0, &Size);
    ok_ntstatus(Status, STATUS_BUFFER_TOO_SMALL);
    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        if (NT_SUCCESS(Status))
            skip("NtQueryInformationToken() succeeded unexpectedly\n");
        else
            skip("Failed to query the total size for token statistics structure\n");
        goto cleanup;
    }

    /* Total size queried, time to allocate our buffer based on that size */
    TokenStats = RtlAllocateHeap(RtlGetProcessHeap(), 0, Size);
    ok(TokenStats != NULL, "Failed to allocate our token statistics buffer\n");
    if (TokenStats == NULL)
    {
        skip("TokenStats is NULL\n");
        goto cleanup;
    }

    /* Time to query our token statistics, prior duplicating the token as effective */
    Status = NtQueryInformationToken(TokenHandle, TokenStatistics, TokenStats, Size, &Size);
    ok_ntstatus(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to query the token statistics\n");
        goto cleanup;
    }

    trace("Number of privileges of regular token -- %lu\n", TokenStats->PrivilegeCount);
    trace("Number of groups of regular token -- %lu\n", TokenStats->GroupCount);

    /* Duplicate the token as effective only */
    Status = NtDuplicateToken(TokenHandle,
                              0,
                              &ObjectAttributes,
                              TRUE,
                              TokenPrimary,
                              &DuplicatedTokenHandle);
    ok_ntstatus(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("NtDuplicateToken() failed\n");
        goto cleanup;
    }

    /*
     * Query the token statistics again, but now this time of
     * the duplicated effective token. On this moment this token
     * should have the disabled privileges (including the one we
     * disabled ourselves) removed as well as the disabled groups
     * that the duplicated token includes, whatever that is.
     */
    Status = NtQueryInformationToken(DuplicatedTokenHandle, TokenStatistics, TokenStats, Size, &Size);
    ok_ntstatus(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to query the token statistics\n");
        goto cleanup;
    }

    trace("Number of privileges of effective only token -- %lu\n", TokenStats->PrivilegeCount);
    trace("Number of groups of effective only token -- %lu\n", TokenStats->GroupCount);

    // TODO: Actually check that counts are lower than before.

cleanup:
    /*
     * We finished our tests, free the memory
     * block and close the handles now.
     */
    if (DuplicatedTokenHandle)
        CloseHandle(DuplicatedTokenHandle);
    if (TokenStats)
        RtlFreeHeap(RtlGetProcessHeap(), 0, TokenStats);
    CloseHandle(TokenHandle);
}

START_TEST(NtDuplicateToken)
{
    DuplicateTokenAsEffective();
}
