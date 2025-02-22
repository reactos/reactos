/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for DupicateTokenEx
 * PROGRAMMER:      Jérôme Gardou <jerome.gardou@reactos.org>
 */

#include "precomp.h"

#define ok_luid_equal(Luid, Expected)                                                                             \
    ok(RtlEqualLuid((Luid), (Expected)), "Got wrong LUID %08lx%08lx, expected (%08lx%08lx).\n", \
        (Luid)->HighPart, (Luid)->LowPart, (Expected)->HighPart, (Expected)->LowPart)
#define ok_luid_notequal(Luid, Comparand)                                                                             \
    ok(!RtlEqualLuid((Luid), (Comparand)), "LUID is %08lx%08lx and should not be.\n", \
        (Luid)->HighPart, (Luid)->LowPart)

START_TEST(DuplicateTokenEx)
{
    HANDLE ProcessToken, TokenDup;
    TOKEN_STATISTICS ProcessTokenStats, TokenDupStats;
    BOOL Result;
    DWORD ReturnLength;

    /* Get the current process token */
    Result = OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE | TOKEN_QUERY, &ProcessToken);
    ok(Result, "OpenProcessToken failed. GLE: %lu.\n", GetLastError());
    /* And its statistics */
    Result = GetTokenInformation(ProcessToken,
        TokenStatistics,
        &ProcessTokenStats,
        sizeof(ProcessTokenStats),
        &ReturnLength);
    ok(Result, "GetTokenInformation failed. GLE: %lu.\n", GetLastError());
    ok_size_t(ReturnLength, sizeof(ProcessTokenStats));

    /* Duplicate it as primary token with the same access rights. */
    Result = DuplicateTokenEx(ProcessToken, 0, NULL, SecurityImpersonation, TokenPrimary, &TokenDup);
    ok(Result, "DuplicateTokenEx failed. GLE: %lu.\n", GetLastError());
    /* Get the stats */
    Result = GetTokenInformation(TokenDup,
        TokenStatistics,
        &TokenDupStats,
        sizeof(ProcessTokenStats),
        &ReturnLength);
    ok(Result, "GetTokenInformation failed. GLE: %lu.\n", GetLastError());
    ok_size_t(ReturnLength, sizeof(ProcessTokenStats));
    /* And test them */
    ok_luid_notequal(&TokenDupStats.TokenId, &ProcessTokenStats.TokenId);
    ok_luid_equal(&TokenDupStats.AuthenticationId, &ProcessTokenStats.AuthenticationId);
    ok(TokenDupStats.TokenType == TokenPrimary, "Duplicate token type is %d.\n", TokenDupStats.TokenType);
    ok(TokenDupStats.ImpersonationLevel == SecurityImpersonation,
        "Duplicate token impersonation level is %d.\n", TokenDupStats.ImpersonationLevel);
    ok_dec(TokenDupStats.DynamicCharged, ProcessTokenStats.DynamicCharged);
    ok_dec(TokenDupStats.DynamicAvailable, ProcessTokenStats.DynamicAvailable);
    ok_dec(TokenDupStats.GroupCount, ProcessTokenStats.GroupCount);
    ok_dec(TokenDupStats.PrivilegeCount, ProcessTokenStats.PrivilegeCount);
    ok_luid_equal(&TokenDupStats.ModifiedId, &ProcessTokenStats.ModifiedId);

    CloseHandle(ProcessToken);
    CloseHandle(TokenDup);
}
