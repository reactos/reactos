/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Tests for the NtQueryInformationToken API
 * COPYRIGHT:       Copyright 2022 George Bișoc <george.bisoc@reactos.org>
 */

#include "precomp.h"

static
HANDLE
OpenCurrentToken(VOID)
{
    BOOL Success;
    HANDLE Token;

    Success = OpenProcessToken(GetCurrentProcess(),
                               TOKEN_READ | TOKEN_QUERY_SOURCE | TOKEN_DUPLICATE,
                               &Token);
    if (!Success)
    {
        ok(FALSE, "OpenProcessToken() has failed to get the process' token (error code: %lu)!\n", GetLastError());
        return NULL;
    }

    return Token;
}

static
VOID
QueryTokenUserTests(
    _In_ HANDLE Token)
{
    NTSTATUS Status;
    PTOKEN_USER UserToken;
    ULONG BufferLength;
    UNICODE_STRING SidString;

    /*
     * Query the exact buffer length to hold
     * our stuff, STATUS_BUFFER_TOO_SMALL must
     * be expected here.
     */
    Status = NtQueryInformationToken(Token,
                                     TokenUser,
                                     NULL,
                                     0,
                                     &BufferLength);
    ok_ntstatus(Status, STATUS_BUFFER_TOO_SMALL);

    /* Allocate the buffer based on the size we got */
    UserToken = RtlAllocateHeap(RtlGetProcessHeap(), 0, BufferLength);
    if (!UserToken)
    {
        ok(FALSE, "Failed to allocate from heap for token user (required buffer length %lu)!\n", BufferLength);
        return;
    }

    /* Now do the actual query */
    Status = NtQueryInformationToken(Token,
                                     TokenUser,
                                     UserToken,
                                     BufferLength,
                                     &BufferLength);
    ok_ntstatus(Status, STATUS_SUCCESS);

    RtlConvertSidToUnicodeString(&SidString, UserToken->User.Sid, TRUE);
    trace("=============== TokenUser ===============\n");
    trace("The SID of current token user is: %s\n", wine_dbgstr_w(SidString.Buffer));
    trace("=========================================\n\n");
    RtlFreeUnicodeString(&SidString);

    RtlFreeHeap(RtlGetProcessHeap(), 0, UserToken);
}

static
VOID
QueryTokenGroupsTests(
    _In_ HANDLE Token)
{
    NTSTATUS Status;
    PTOKEN_GROUPS Groups;
    ULONG BufferLength;

    /*
     * Query the exact buffer length to hold
     * our stuff, STATUS_BUFFER_TOO_SMALL must
     * be expected here.
     */
    Status = NtQueryInformationToken(Token,
                                     TokenGroups,
                                     NULL,
                                     0,
                                     &BufferLength);
    ok_ntstatus(Status, STATUS_BUFFER_TOO_SMALL);

    /* Allocate the buffer based on the size we got */
    Groups = RtlAllocateHeap(RtlGetProcessHeap(), 0, BufferLength);
    if (!Groups)
    {
        ok(FALSE, "Failed to allocate from heap for token groups (required buffer length %lu)!\n", BufferLength);
        return;
    }

    /*
     * Now do the actual query and validate the
     * number of groups.
     */
    Status = NtQueryInformationToken(Token,
                                     TokenGroups,
                                     Groups,
                                     BufferLength,
                                     &BufferLength);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(Groups->GroupCount == 10, "The number of groups must be 10 (current number %lu)!\n", Groups->GroupCount);

    RtlFreeHeap(RtlGetProcessHeap(), 0, Groups);
}

static
VOID
QueryTokenPrivilegesTests(
    _In_ HANDLE Token)
{
    NTSTATUS Status;
    PTOKEN_PRIVILEGES Privileges;
    ULONG BufferLength;

    /*
     * Query the exact buffer length to hold
     * our stuff, STATUS_BUFFER_TOO_SMALL must
     * be expected here.
     */
    Status = NtQueryInformationToken(Token,
                                     TokenPrivileges,
                                     NULL,
                                     0,
                                     &BufferLength);
    ok_ntstatus(Status, STATUS_BUFFER_TOO_SMALL);

    /* Allocate the buffer based on the size we got */
    Privileges = RtlAllocateHeap(RtlGetProcessHeap(), 0, BufferLength);
    if (!Privileges)
    {
        ok(FALSE, "Failed to allocate from heap for token privileges (required buffer length %lu)!\n", BufferLength);
        return;
    }

    /*
     * Now do the actual query and validate the
     * number of privileges.
     */
    Status = NtQueryInformationToken(Token,
                                     TokenPrivileges,
                                     Privileges,
                                     BufferLength,
                                     &BufferLength);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(Privileges->PrivilegeCount == 20, "The number of privileges must be 20 (current number %lu)!\n", Privileges->PrivilegeCount);

    RtlFreeHeap(RtlGetProcessHeap(), 0, Privileges);
}

static
VOID
QueryTokenOwnerTests(
    _In_ HANDLE Token)
{
    NTSTATUS Status;
    PTOKEN_OWNER Owner;
    ULONG BufferLength;
    UNICODE_STRING SidString;

    /*
     * Query the exact buffer length to hold
     * our stuff, STATUS_BUFFER_TOO_SMALL must
     * be expected here.
     */
    Status = NtQueryInformationToken(Token,
                                     TokenOwner,
                                     NULL,
                                     0,
                                     &BufferLength);
    ok_ntstatus(Status, STATUS_BUFFER_TOO_SMALL);

    /* Allocate the buffer based on the size we got */
    Owner = RtlAllocateHeap(RtlGetProcessHeap(), 0, BufferLength);
    if (!Owner)
    {
        ok(FALSE, "Failed to allocate from heap for token owner (required buffer length %lu)!\n", BufferLength);
        return;
    }

    /*
     * Now do the actual query and validate the
     * token owner (must be the local admin).
     */
    Status = NtQueryInformationToken(Token,
                                     TokenOwner,
                                     Owner,
                                     BufferLength,
                                     &BufferLength);
    ok_ntstatus(Status, STATUS_SUCCESS);

    RtlConvertSidToUnicodeString(&SidString, Owner->Owner, TRUE);
    ok_wstr(SidString.Buffer, L"S-1-5-32-544");
    RtlFreeUnicodeString(&SidString);

    RtlFreeHeap(RtlGetProcessHeap(), 0, Owner);
}

static
VOID
QueryTokenPrimaryGroupTests(
    _In_ HANDLE Token)
{
    NTSTATUS Status;
    PTOKEN_PRIMARY_GROUP PrimaryGroup;
    ULONG BufferLength;
    UNICODE_STRING SidString;

    /*
     * Query the exact buffer length to hold
     * our stuff, STATUS_BUFFER_TOO_SMALL must
     * be expected here.
     */
    Status = NtQueryInformationToken(Token,
                                     TokenPrimaryGroup,
                                     NULL,
                                     0,
                                     &BufferLength);
    ok_ntstatus(Status, STATUS_BUFFER_TOO_SMALL);

    /* Allocate the buffer based on the size we got */
    PrimaryGroup = RtlAllocateHeap(RtlGetProcessHeap(), 0, BufferLength);
    if (!PrimaryGroup)
    {
        ok(FALSE, "Failed to allocate from heap for token primary group (required buffer length %lu)!\n", BufferLength);
        return;
    }

    /* Now do the actual query */
    Status = NtQueryInformationToken(Token,
                                     TokenPrimaryGroup,
                                     PrimaryGroup,
                                     BufferLength,
                                     &BufferLength);
    ok_ntstatus(Status, STATUS_SUCCESS);

    RtlConvertSidToUnicodeString(&SidString, PrimaryGroup->PrimaryGroup, TRUE);
    trace("=============== TokenPrimaryGroup ===============\n");
    trace("The primary group SID of current token is: %s\n", wine_dbgstr_w(SidString.Buffer));
    trace("=========================================\n\n");
    RtlFreeUnicodeString(&SidString);

    RtlFreeHeap(RtlGetProcessHeap(), 0, PrimaryGroup);
}

static
VOID
QueryTokenDefaultDaclTests(
    _In_ HANDLE Token)
{
    NTSTATUS Status;
    PTOKEN_DEFAULT_DACL Dacl;
    ULONG BufferLength;

    /*
     * Query the exact buffer length to hold
     * our stuff, STATUS_BUFFER_TOO_SMALL must
     * be expected here.
     */
    Status = NtQueryInformationToken(Token,
                                     TokenDefaultDacl,
                                     NULL,
                                     0,
                                     &BufferLength);
    ok_ntstatus(Status, STATUS_BUFFER_TOO_SMALL);

    /* Allocate the buffer based on the size we got */
    Dacl = RtlAllocateHeap(RtlGetProcessHeap(), 0, BufferLength);
    if (!Dacl)
    {
        ok(FALSE, "Failed to allocate from heap for token default DACL (required buffer length %lu)!\n", BufferLength);
        return;
    }

    /*
     * Now do the actual query and validate the
     * ACL revision and number count of ACEs.
     */
    Status = NtQueryInformationToken(Token,
                                     TokenDefaultDacl,
                                     Dacl,
                                     BufferLength,
                                     &BufferLength);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(Dacl->DefaultDacl->AclRevision == 2, "The ACL revision of token default DACL must be 2 (current revision %u)!\n", Dacl->DefaultDacl->AclRevision);
    ok(Dacl->DefaultDacl->AceCount == 2, "The ACL's ACE count must be 2 (current ACE count %u)!\n", Dacl->DefaultDacl->AceCount);

    RtlFreeHeap(RtlGetProcessHeap(), 0, Dacl);
}

static
VOID
QueryTokenSourceTests(
    _In_ HANDLE Token)
{
    NTSTATUS Status;
    PTOKEN_SOURCE Source;
    ULONG BufferLength;
    CHAR SourceName[8];

    /*
     * Query the exact buffer length to hold
     * our stuff, STATUS_BUFFER_TOO_SMALL must
     * be expected here.
     */
    Status = NtQueryInformationToken(Token,
                                     TokenSource,
                                     NULL,
                                     0,
                                     &BufferLength);
    ok_ntstatus(Status, STATUS_BUFFER_TOO_SMALL);

    /* Allocate the buffer based on the size we got */
    Source = RtlAllocateHeap(RtlGetProcessHeap(), 0, BufferLength);
    if (!Source)
    {
        ok(FALSE, "Failed to allocate from heap for token source (required buffer length %lu)!\n", BufferLength);
        return;
    }

    /* Now do the actual query */
    Status = NtQueryInformationToken(Token,
                                     TokenSource,
                                     Source,
                                     BufferLength,
                                     &BufferLength);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /*
     * Subtract the source name from the queried buffer
     * and compare it. The source name in question must be
     * "User32" as the primary token of the current calling
     * process is generated when the user has successfully
     * logged in and he's into the desktop.
     */
    SourceName[0] = Source->SourceName[0];
    SourceName[1] = Source->SourceName[1];
    SourceName[2] = Source->SourceName[2];
    SourceName[3] = Source->SourceName[3];
    SourceName[4] = Source->SourceName[4];
    SourceName[5] = Source->SourceName[5];
    SourceName[6] = '\0';
    ok_str(SourceName, "User32");

    RtlFreeHeap(RtlGetProcessHeap(), 0, Source);
}

static
VOID
QueryTokenTypeTests(
    _In_ HANDLE Token)
{
    NTSTATUS Status;
    TOKEN_TYPE Type;
    ULONG BufferLength;

    /*
     * Query the token type. The token of the
     * current calling process must be primary
     * since we aren't impersonating the security
     * context of a client.
     */
    Status = NtQueryInformationToken(Token,
                                     TokenType,
                                     &Type,
                                     sizeof(TOKEN_TYPE),
                                     &BufferLength);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(Type == TokenPrimary, "The current token is not primary!\n");
}

static
VOID
QueryTokenImpersonationTests(
    _In_ HANDLE Token)
{
    NTSTATUS Status;
    SECURITY_IMPERSONATION_LEVEL Level;
    ULONG BufferLength;
    HANDLE DupToken;
    OBJECT_ATTRIBUTES ObjectAttributes;

    /*
     * Windows throws STATUS_INVALID_INFO_CLASS here
     * because one cannot simply query the impersonation
     * level of a primary token.
     */
    Status = NtQueryInformationToken(Token,
                                     TokenImpersonationLevel,
                                     &Level,
                                     sizeof(SECURITY_IMPERSONATION_LEVEL),
                                     &BufferLength);
    ok_ntstatus(Status, STATUS_INVALID_INFO_CLASS);

    /*
     * Initialize the object attribute and duplicate
     * the token into an actual impersonation one.
     */
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               0,
                               NULL,
                               NULL);

    Status = NtDuplicateToken(Token,
                              TOKEN_QUERY,
                              &ObjectAttributes,
                              FALSE,
                              TokenImpersonation,
                              &DupToken);
    if (!NT_SUCCESS(Status))
    {
        ok(FALSE, "Failed to duplicate token (Status code %lx)!\n", Status);
        return;
    }

    /* Now do the actual query */
    Status = NtQueryInformationToken(DupToken,
                                     TokenImpersonationLevel,
                                     &Level,
                                     sizeof(SECURITY_IMPERSONATION_LEVEL),
                                     &BufferLength);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(Level == SecurityAnonymous, "The current token impersonation level is not anonymous!\n");
    NtClose(DupToken);
}

static
VOID
QueryTokenStatisticsTests(
    _In_ HANDLE Token)
{
    NTSTATUS Status;
    PTOKEN_STATISTICS Statistics;
    ULONG BufferLength;

    /*
     * Query the exact buffer length to hold
     * our stuff, STATUS_BUFFER_TOO_SMALL must
     * be expected here.
     */
    Status = NtQueryInformationToken(Token,
                                     TokenStatistics,
                                     NULL,
                                     0,
                                     &BufferLength);
    ok_ntstatus(Status, STATUS_BUFFER_TOO_SMALL);

    /* Allocate the buffer based on the size we got */
    Statistics = RtlAllocateHeap(RtlGetProcessHeap(), 0, BufferLength);
    if (!Statistics)
    {
        ok(FALSE, "Failed to allocate from heap for token statistics (required buffer length %lu)!\n", BufferLength);
        return;
    }

    /* Do the actual query */
    Status = NtQueryInformationToken(Token,
                                     TokenStatistics,
                                     Statistics,
                                     BufferLength,
                                     &BufferLength);
    ok_ntstatus(Status, STATUS_SUCCESS);

    trace("=============== TokenStatistics ===============\n");
    trace("Token ID: %lu %lu\n", Statistics->TokenId.LowPart, Statistics->TokenId.HighPart);
    trace("Authentication ID: %lu %lu\n", Statistics->AuthenticationId.LowPart, Statistics->AuthenticationId.HighPart);
    trace("Dynamic Charged: %lu\n", Statistics->DynamicCharged);
    trace("Dynamic Available: %lu\n", Statistics->DynamicAvailable);
    trace("Modified ID: %lu %lu\n", Statistics->ModifiedId.LowPart, Statistics->ModifiedId.HighPart);
    trace("=========================================\n\n");

    RtlFreeHeap(RtlGetProcessHeap(), 0, Statistics);
}

static
VOID
QueryTokenPrivilegesAndGroupsTests(
    _In_ HANDLE Token)
{
    NTSTATUS Status;
    PTOKEN_GROUPS_AND_PRIVILEGES PrivsAndGroups;
    TOKEN_GROUPS SidToRestrict;
    HANDLE FilteredToken;
    PSID WorldSid;
    ULONG BufferLength;
    static SID_IDENTIFIER_AUTHORITY WorldAuthority = {SECURITY_WORLD_SID_AUTHORITY};

    /*
     * Create a World SID and filter the token
     * by adding a restricted SID.
     */
    Status = RtlAllocateAndInitializeSid(&WorldAuthority,
                                         1,
                                         SECURITY_WORLD_RID,
                                         0, 0, 0, 0, 0, 0, 0,
                                         &WorldSid);
    if (!NT_SUCCESS(Status))
    {
        ok(FALSE, "Failed to allocate World SID (Status code %lx)!\n", Status);
        return;
    }

    SidToRestrict.GroupCount = 1;
    SidToRestrict.Groups[0].Attributes = 0;
    SidToRestrict.Groups[0].Sid = WorldSid;

    Status = NtFilterToken(Token,
                           0,
                           NULL,
                           NULL,
                           &SidToRestrict,
                           &FilteredToken);
    if (!NT_SUCCESS(Status))
    {
        ok(FALSE, "Failed to filter the current token (Status code %lx)!\n", Status);
        RtlFreeHeap(RtlGetProcessHeap(), 0, WorldSid);
        return;
    }

    /*
     * Query the exact buffer length to hold
     * our stuff, STATUS_BUFFER_TOO_SMALL must
     * be expected here.
     */
    Status = NtQueryInformationToken(FilteredToken,
                                     TokenGroupsAndPrivileges,
                                     NULL,
                                     0,
                                     &BufferLength);
    ok_ntstatus(Status, STATUS_BUFFER_TOO_SMALL);

    /* Allocate the buffer based on the size we got */
    PrivsAndGroups = RtlAllocateHeap(RtlGetProcessHeap(), 0, BufferLength);
    if (!PrivsAndGroups)
    {
        ok(FALSE, "Failed to allocate from heap for token privileges and groups (required buffer length %lu)!\n", BufferLength);
        RtlFreeHeap(RtlGetProcessHeap(), 0, WorldSid);
        NtClose(FilteredToken);
        return;
    }

    /* Do the actual query */
    Status = NtQueryInformationToken(FilteredToken,
                                     TokenGroupsAndPrivileges,
                                     PrivsAndGroups,
                                     BufferLength,
                                     &BufferLength);
    ok_ntstatus(Status, STATUS_SUCCESS);

    trace("=============== TokenGroupsAndPrivileges ===============\n");
    trace("SID count: %lu\n", PrivsAndGroups->SidCount);
    trace("SID length: %lu\n", PrivsAndGroups->SidLength);
    trace("Restricted SID count: %lu\n", PrivsAndGroups->RestrictedSidCount);
    trace("Restricted SID length: %lu\n", PrivsAndGroups->RestrictedSidLength);
    trace("Privilege count: %lu\n", PrivsAndGroups->PrivilegeCount);
    trace("Privilege length: %lu\n", PrivsAndGroups->PrivilegeLength);
    trace("Authentication ID: %lu %lu\n", PrivsAndGroups->AuthenticationId.LowPart, PrivsAndGroups->AuthenticationId.HighPart);
    trace("=========================================\n\n");

    RtlFreeHeap(RtlGetProcessHeap(), 0, PrivsAndGroups);
    RtlFreeHeap(RtlGetProcessHeap(), 0, WorldSid);
    NtClose(FilteredToken);
}

static
VOID
QueryTokenRestrictedSidsTest(
    _In_ HANDLE Token)
{
    NTSTATUS Status;
    PTOKEN_GROUPS RestrictedGroups;
    TOKEN_GROUPS SidToRestrict;
    ULONG BufferLength;
    HANDLE FilteredToken;
    PSID WorldSid;
    static SID_IDENTIFIER_AUTHORITY WorldAuthority = {SECURITY_WORLD_SID_AUTHORITY};

    /*
     * Query the exact buffer length to hold
     * our stuff, STATUS_BUFFER_TOO_SMALL must
     * be expected here.
     */
    Status = NtQueryInformationToken(Token,
                                     TokenRestrictedSids,
                                     NULL,
                                     0,
                                     &BufferLength);
    ok_ntstatus(Status, STATUS_BUFFER_TOO_SMALL);

    /* Allocate the buffer based on the size we got */
    RestrictedGroups = RtlAllocateHeap(RtlGetProcessHeap(), 0, BufferLength);
    if (!RestrictedGroups)
    {
        ok(FALSE, "Failed to allocate from heap for restricted SIDs (required buffer length %lu)!\n", BufferLength);
        return;
    }

    /*
     * Query the number of restricted SIDs. Originally the token
     * doesn't have any restricted SIDs inserted.
     */
    Status = NtQueryInformationToken(Token,
                                     TokenRestrictedSids,
                                     RestrictedGroups,
                                     BufferLength,
                                     &BufferLength);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(RestrictedGroups->GroupCount == 0, "There mustn't be any restricted SIDs before filtering (number of restricted SIDs %lu)!\n", RestrictedGroups->GroupCount);

    RtlFreeHeap(RtlGetProcessHeap(), 0, RestrictedGroups);
    RestrictedGroups = NULL;

    Status = RtlAllocateAndInitializeSid(&WorldAuthority,
                                         1,
                                         SECURITY_WORLD_RID,
                                         0, 0, 0, 0, 0, 0, 0,
                                         &WorldSid);
    if (!NT_SUCCESS(Status))
    {
        ok(FALSE, "Failed to allocate World SID (Status code %lx)!\n", Status);
        return;
    }

    SidToRestrict.GroupCount = 1;
    SidToRestrict.Groups[0].Attributes = 0;
    SidToRestrict.Groups[0].Sid = WorldSid;

    Status = NtFilterToken(Token,
                           0,
                           NULL,
                           NULL,
                           &SidToRestrict,
                           &FilteredToken);
    if (!NT_SUCCESS(Status))
    {
        ok(FALSE, "Failed to filter the current token (Status code %lx)!\n", Status);
        RtlFreeHeap(RtlGetProcessHeap(), 0, WorldSid);
        return;
    }

    Status = NtQueryInformationToken(FilteredToken,
                                     TokenRestrictedSids,
                                     NULL,
                                     0,
                                     &BufferLength);
    ok_ntstatus(Status, STATUS_BUFFER_TOO_SMALL);

    RestrictedGroups = RtlAllocateHeap(RtlGetProcessHeap(), 0, BufferLength);
    if (!RestrictedGroups)
    {
        ok(FALSE, "Failed to allocate from heap for restricted SIDs (required buffer length %lu)!\n", BufferLength);
        RtlFreeHeap(RtlGetProcessHeap(), 0, WorldSid);
        return;
    }

    /*
     * Do a query again, this time we must have a
     * restricted SID inserted into the token.
     */
    Status = NtQueryInformationToken(FilteredToken,
                                     TokenRestrictedSids,
                                     RestrictedGroups,
                                     BufferLength,
                                     &BufferLength);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(RestrictedGroups->GroupCount == 1, "There must be only one restricted SID added in token (number of restricted SIDs %lu)!\n", RestrictedGroups->GroupCount);

    RtlFreeHeap(RtlGetProcessHeap(), 0, RestrictedGroups);
    RtlFreeHeap(RtlGetProcessHeap(), 0, WorldSid);
    NtClose(FilteredToken);
}

static
VOID
QueryTokenSessionIdTests(
    _In_ HANDLE Token)
{
    NTSTATUS Status;
    ULONG SessionId;
    ULONG BufferLength;

    /*
     * Query the session ID. Generally the current
     * process token is not under any terminal service
     * so the ID must be 0.
     */
    Status = NtQueryInformationToken(Token,
                                     TokenSessionId,
                                     &SessionId,
                                     sizeof(ULONG),
                                     &BufferLength);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(SessionId == 0, "The session ID of current token must be 0 (current session %lu)!\n", SessionId);
}

static
VOID
QueryTokenIsSandboxInert(
    _In_ HANDLE Token)
{
    NTSTATUS Status;
    ULONG IsTokenInert;
    ULONG BufferLength;
    HANDLE FilteredToken;

    /*
     * Query the sandbox inert token information,
     * it must not be inert.
     */
    Status = NtQueryInformationToken(Token,
                                     TokenSandBoxInert,
                                     &IsTokenInert,
                                     sizeof(ULONG),
                                     &BufferLength);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(IsTokenInert == FALSE, "The token must not be a sandbox inert one!\n");

    /*
     * Try to turn the token into an inert
     * one by filtering it.
     */
    Status = NtFilterToken(Token,
                           SANDBOX_INERT,
                           NULL,
                           NULL,
                           NULL,
                           &FilteredToken);
    if (!NT_SUCCESS(Status))
    {
        ok(FALSE, "Failed to filter the current token (Status code %lx)!\n", Status);
        return;
    }

    /*
     * Now do a query again, this time
     * the token should be inert.
     */
    Status = NtQueryInformationToken(FilteredToken,
                                     TokenSandBoxInert,
                                     &IsTokenInert,
                                     sizeof(ULONG),
                                     &BufferLength);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(IsTokenInert == TRUE, "The token must be a sandbox inert one after filtering!\n");

    NtClose(FilteredToken);
}

static
VOID
QueryTokenOriginTests(
    _In_ HANDLE Token)
{
    NTSTATUS Status;
    TOKEN_ORIGIN Origin;
    ULONG BufferLength;

    /* Query the token origin */
    Status = NtQueryInformationToken(Token,
                                     TokenOrigin,
                                     &Origin,
                                     sizeof(TOKEN_ORIGIN),
                                     &BufferLength);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(Origin.OriginatingLogonSession.LowPart == 0x3e7, "The LowPart field of the originating logon session must be SYSTEM_LUID (current value %lu)!\n",
       Origin.OriginatingLogonSession.LowPart);
    ok(Origin.OriginatingLogonSession.HighPart == 0x0, "The HighPart field of the logon session must be 0 (current value %lu)!\n",
       Origin.OriginatingLogonSession.HighPart);
}

START_TEST(NtQueryInformationToken)
{
    NTSTATUS Status;
    HANDLE Token;
    PVOID Dummy;
    ULONG DummyReturnLength;

    /* ReturnLength is NULL */
    Status = NtQueryInformationToken(NULL,
                                     TokenUser,
                                     NULL,
                                     0,
                                     NULL);
    ok_ntstatus(Status, STATUS_ACCESS_VIOLATION);

    /* We don't give any token here */
    Status = NtQueryInformationToken(NULL,
                                     TokenUser,
                                     &Dummy,
                                     0,
                                     &DummyReturnLength);
    ok_ntstatus(Status, STATUS_INVALID_HANDLE);

    Token = OpenCurrentToken();

    /* Class 0 is unused on Windows */
    Status = NtQueryInformationToken(Token,
                                     0,
                                     &Dummy,
                                     0,
                                     &DummyReturnLength);
    ok_ntstatus(Status, STATUS_INVALID_INFO_CLASS);

    /* We give a bogus info class */
    Status = NtQueryInformationToken(Token,
                                     0xa0a,
                                     &Dummy,
                                     0,
                                     &DummyReturnLength);
    ok_ntstatus(Status, STATUS_INVALID_INFO_CLASS);

    /* Now perform tests for each class */
    QueryTokenUserTests(Token);
    QueryTokenGroupsTests(Token);
    QueryTokenPrivilegesTests(Token);
    QueryTokenOwnerTests(Token);
    QueryTokenPrimaryGroupTests(Token);
    QueryTokenDefaultDaclTests(Token);
    QueryTokenSourceTests(Token);
    QueryTokenTypeTests(Token);
    QueryTokenImpersonationTests(Token);
    QueryTokenStatisticsTests(Token);
    QueryTokenPrivilegesAndGroupsTests(Token);
    QueryTokenRestrictedSidsTest(Token);
    QueryTokenSessionIdTests(Token);
    QueryTokenIsSandboxInert(Token);
    QueryTokenOriginTests(Token);

    NtClose(Token);
}
