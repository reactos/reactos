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
        skip("OpenProcessToken() has failed to get the process' token (error code: %lu)!\n", GetLastError());
        return NULL;
    }

    return Token;
}

static
VOID
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
        skip("LookupPrivilegeValueW() has failed to locate the privilege value (error code: %lu)!\n", GetLastError());
        return;
    }

    TokenPriv.PrivilegeCount = 1;
    TokenPriv.Privileges[0].Luid = PrivLuid;
    TokenPriv.Privileges[0].Attributes = 0;

    Success = AdjustTokenPrivileges(Token,
                                    FALSE,
                                    &TokenPriv,
                                    0,
                                    NULL,
                                    NULL);
    if (!Success)
    {
        skip("AdjustTokenPrivileges() has failed to adjust privileges of token (error code: %lu)!\n", GetLastError());
        return;
    }
}

static
VOID
DuplicateTokenAsEffective(VOID)
{
    NTSTATUS Status;
    ULONG Size;
    HANDLE TokenHandle;
    HANDLE DuplicatedTokenHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PTOKEN_STATISTICS TokenStats;

    /* Initialize the object attributes for token duplication */
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               0,
                               NULL,
                               NULL);

    /* Get the token from process and begin the tests */
    TokenHandle = OpenTokenFromProcess();

    /* We give a bogus invalid handle */
    Status = NtDuplicateToken(NULL,
                              0,
                              NULL,
                              TRUE,
                              TokenPrimary,
                              NULL);
    ok_hex(Status, STATUS_ACCESS_VIOLATION);

    /*
     * Disable a privilege, the impersonation privilege for example.
     * Why we're doing this is because such privilege is enabled
     * by default and we'd want to know what the kernel does
     * at the moment of removing disabled privileges during making
     * the token effective, with this potential privilege being
     * disabled by ourselves.
     */
    DisablePrivilege(TokenHandle, L"SeImpersonatePrivilege");

    /* Query the total size of the token statistics structure */
    Status = NtQueryInformationToken(TokenHandle, TokenStatistics, NULL, 0, &Size);
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

    /* Time to query our token statistics, prior duplicating the token as effective */
    Status = NtQueryInformationToken(TokenHandle, TokenStatistics, TokenStats, Size, &Size);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to query the token statistics! (Status -> 0x%lx)\n", Status);
        return;
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
    ok_hex(Status, STATUS_SUCCESS);

    /*
     * Query the token statistics again, but now this time of
     * the duplicated effective token. On this moment this token
     * should have the disabled privileges (including the one we
     * disabled ourselves) removed as well as the disabled groups
     * that the duplicated token includes, whatever that is.
     */
    Status = NtQueryInformationToken(DuplicatedTokenHandle, TokenStatistics, TokenStats, Size, &Size);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to query the token statistics! (Status -> 0x%lx)\n", Status);
        return;
    }

    trace("Number of privileges of effective only token -- %lu\n", TokenStats->PrivilegeCount);
    trace("Number of groups of effective only token -- %lu\n", TokenStats->GroupCount);

    /*
     * We finished our tests, free the memory
     * block and close the handles now.
     */
    RtlFreeHeap(RtlGetProcessHeap(), 0, TokenStats);
    CloseHandle(TokenHandle),
    CloseHandle(DuplicatedTokenHandle);
}

START_TEST(NtDuplicateToken)
{
    DuplicateTokenAsEffective();
}
