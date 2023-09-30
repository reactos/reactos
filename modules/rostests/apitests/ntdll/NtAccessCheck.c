/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Tests for the NtAccessCheck API
 * COPYRIGHT:       Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

#include "precomp.h"

static
HANDLE
GetToken(VOID)
{
    NTSTATUS Status;
    HANDLE Token;
    HANDLE DuplicatedToken;
    OBJECT_ATTRIBUTES ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE Sqos;

    Status = NtOpenProcessToken(NtCurrentProcess(),
                                TOKEN_QUERY | TOKEN_DUPLICATE,
                                &Token);
    if (!NT_SUCCESS(Status))
    {
        trace("Failed to get current process token (Status 0x%08lx)\n", Status);
        return NULL;
    }

    Sqos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    Sqos.ImpersonationLevel = SecurityImpersonation;
    Sqos.ContextTrackingMode = 0;
    Sqos.EffectiveOnly = FALSE;

    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               0,
                               NULL,
                               NULL);
    ObjectAttributes.SecurityQualityOfService = &Sqos;

    Status = NtDuplicateToken(Token,
                              TOKEN_QUERY | TOKEN_DUPLICATE,
                              &ObjectAttributes,
                              FALSE,
                              TokenImpersonation,
                              &DuplicatedToken);
    if (!NT_SUCCESS(Status))
    {
        trace("Failed to duplicate token (Status 0x%08lx)\n", Status);
        NtClose(Token);
        return NULL;
    }

    return DuplicatedToken;
}

static
VOID
AccessCheckEmptyMappingTest(VOID)
{
    NTSTATUS Status;
    NTSTATUS AccessStatus;
    ACCESS_MASK GrantedAccess;
    PPRIVILEGE_SET PrivilegeSet = NULL;
    ULONG PrivilegeSetLength;
    HANDLE Token = NULL;
    PACL Dacl = NULL;
    ULONG DaclSize;
    SECURITY_DESCRIPTOR Sd;
    PSID WorldSid = NULL;
    static SID_IDENTIFIER_AUTHORITY WorldAuthority = {SECURITY_WORLD_SID_AUTHORITY};
    static GENERIC_MAPPING EmptyMapping = {0, 0, 0, 0};

    /* Allocate all the stuff we need */
    PrivilegeSetLength = FIELD_OFFSET(PRIVILEGE_SET, Privilege[16]);
    PrivilegeSet = RtlAllocateHeap(RtlGetProcessHeap(), 0, PrivilegeSetLength);
    if (PrivilegeSet == NULL)
    {
        skip("Failed to allocate PrivilegeSet, skipping tests\n");
        return;
    }

    Status = RtlAllocateAndInitializeSid(&WorldAuthority,
                                         1,
                                         SECURITY_WORLD_RID,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         &WorldSid);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create World SID, skipping tests\n");
        goto Quit;
    }

    Token = GetToken();
    if (Token == NULL)
    {
        skip("Failed to get token, skipping tests\n");
        goto Quit;
    }

    Status = RtlCreateSecurityDescriptor(&Sd, SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create a security descriptor, skipping tests\n");
        goto Quit;
    }

    DaclSize = sizeof(ACL) +
               sizeof(ACCESS_ALLOWED_OBJECT_ACE) + RtlLengthSid(WorldSid);
    Dacl = RtlAllocateHeap(RtlGetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           DaclSize);
    if (Dacl == NULL)
    {
        skip("Failed to allocate memory for DACL, skipping tests\n");
        goto Quit;
    }

    /* Setup a ACL and give full access to everyone */
    Status = RtlCreateAcl(Dacl,
                          DaclSize,
                          ACL_REVISION);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create DACL, skipping tests\n");
        goto Quit;
    }

    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    GENERIC_ALL,
                                    WorldSid);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to add allowed ACE for World SID, skipping tests\n");
        goto Quit;
    }

    /* Setup the descriptor */
    RtlSetGroupSecurityDescriptor(&Sd, WorldSid, FALSE);
    RtlSetOwnerSecurityDescriptor(&Sd, WorldSid, FALSE);
    RtlSetDaclSecurityDescriptor(&Sd, TRUE, Dacl, FALSE);

    /* Do an access check with empty mapping */
    Status = NtAccessCheck(&Sd,
                           Token,
                           MAXIMUM_ALLOWED,
                           &EmptyMapping,
                           PrivilegeSet,
                           &PrivilegeSetLength,
                           &GrantedAccess,
                           &AccessStatus);
    ok_hex(Status, STATUS_SUCCESS);
    ok(AccessStatus == STATUS_SUCCESS, "Expected a success status but got 0x%08lx\n", AccessStatus);
    trace("GrantedAccess == 0x%08lx\n", GrantedAccess);

Quit:
    if (Dacl)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, Dacl);
    }

    if (Token)
    {
        NtClose(Token);
    }

    if (WorldSid)
    {
        RtlFreeSid(WorldSid);
    }

    if (PrivilegeSet)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, PrivilegeSet);
    }
}

START_TEST(NtAccessCheck)
{
    AccessCheckEmptyMappingTest();
}
