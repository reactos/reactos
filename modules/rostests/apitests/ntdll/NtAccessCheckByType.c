/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Tests for the NtAccessCheckByType API
 * COPYRIGHT:       Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

#include "precomp.h"

static GENERIC_MAPPING RegMapping = {KEY_READ, KEY_WRITE, KEY_EXECUTE, KEY_ALL_ACCESS};
static GUID ObjectType = {0x12345678, 0x1234, 0x5678, {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}};
static GUID ChildObjectType = {0x23456789, 0x2345, 0x6786, {0x2, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99}};
static SID_IDENTIFIER_AUTHORITY WorldAuthority = {SECURITY_WORLD_SID_AUTHORITY};
static SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};

static
HANDLE
GetTokenProcess(
    _In_ BOOLEAN WantImpersonateLevel,
    _In_ BOOLEAN WantImpersonateType)
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
    Sqos.ImpersonationLevel = WantImpersonateLevel ? SecurityImpersonation : SecurityAnonymous;
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
                              WantImpersonateType ? TokenImpersonation : TokenPrimary,
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
ParamsValidationTests(VOID)
{
    NTSTATUS Status;
    NTSTATUS AccessStatus;
    ACCESS_MASK GrantedAccess;
    PPRIVILEGE_SET PrivilegeSet = NULL;
    ULONG PrivilegeSetLength;
    HANDLE Token = NULL;
    SECURITY_DESCRIPTOR Sd;
    OBJECT_TYPE_LIST ObjTypeList[2];

    /* Everything is NULL */
    Status = NtAccessCheckByType(NULL,
                                 NULL,
                                 NULL,
                                 0,
                                 NULL,
                                 0,
                                 NULL,
                                 NULL,
                                 0,
                                 NULL,
                                 NULL);
    ok_hex(Status, STATUS_ACCESS_VIOLATION);

    /* Allocate a buffer for privileges set */
    PrivilegeSetLength = FIELD_OFFSET(PRIVILEGE_SET, Privilege[16]);
    PrivilegeSet = RtlAllocateHeap(RtlGetProcessHeap(), 0, PrivilegeSetLength);
    if (PrivilegeSet == NULL)
    {
        skip("Failed to allocate PrivilegeSet, skipping tests\n");
        goto Quit;
    }

    /* No token given */
    Status = NtAccessCheckByType(NULL,
                                 NULL,
                                 NULL,
                                 MAXIMUM_ALLOWED,
                                 NULL,
                                 0,
                                 &RegMapping,
                                 PrivilegeSet,
                                 &PrivilegeSetLength,
                                 &GrantedAccess,
                                 &AccessStatus);
    ok_hex(Status, STATUS_INVALID_HANDLE);

    /* Get a token now */
    Token = GetTokenProcess(FALSE, FALSE);
    if (Token == NULL)
    {
        skip("Failed to get token, skipping tests\n");
        goto Quit;
    }

    /* Token given but it's not an impersonation one */
    Status = NtAccessCheckByType(NULL,
                                 NULL,
                                 Token,
                                 MAXIMUM_ALLOWED,
                                 NULL,
                                 0,
                                 &RegMapping,
                                 PrivilegeSet,
                                 &PrivilegeSetLength,
                                 &GrantedAccess,
                                 &AccessStatus);
    ok_hex(Status, STATUS_NO_IMPERSONATION_TOKEN);

    /* Get a token but this time with an invalid impersonation level */
    NtClose(Token);
    Token = GetTokenProcess(FALSE, TRUE);
    if (Token == NULL)
    {
        skip("Failed to get token, skipping tests\n");
        goto Quit;
    }

    /* Token given but it doesn't have the right impersonation level */
    Status = NtAccessCheckByType(NULL,
                                 NULL,
                                 Token,
                                 MAXIMUM_ALLOWED,
                                 NULL,
                                 0,
                                 &RegMapping,
                                 PrivilegeSet,
                                 &PrivilegeSetLength,
                                 &GrantedAccess,
                                 &AccessStatus);
    ok_hex(Status, STATUS_BAD_IMPERSONATION_LEVEL);

    /* A generic access right is given */
    Status = NtAccessCheckByType(NULL,
                                 NULL,
                                 Token,
                                 GENERIC_WRITE,
                                 NULL,
                                 0,
                                 &RegMapping,
                                 PrivilegeSet,
                                 &PrivilegeSetLength,
                                 &GrantedAccess,
                                 &AccessStatus);
    ok_hex(Status, STATUS_GENERIC_NOT_MAPPED);

    /* Close the token, get a valid one that we want */
    NtClose(Token);
    Token = GetTokenProcess(TRUE, TRUE);
    if (Token == NULL)
    {
        skip("Failed to get token, skipping tests\n");
        goto Quit;
    }

    /* No security descriptor is given */
    Status = NtAccessCheckByType(NULL,
                                 NULL,
                                 Token,
                                 MAXIMUM_ALLOWED,
                                 NULL,
                                 0,
                                 &RegMapping,
                                 PrivilegeSet,
                                 &PrivilegeSetLength,
                                 &GrantedAccess,
                                 &AccessStatus);
    ok_hex(Status, STATUS_INVALID_SECURITY_DESCR);

    /* The function expects a privilege set */
    Status = NtAccessCheckByType(NULL,
                                 NULL,
                                 Token,
                                 MAXIMUM_ALLOWED,
                                 NULL,
                                 0,
                                 &RegMapping,
                                 NULL,
                                 0,
                                 &GrantedAccess,
                                 &AccessStatus);
    ok_hex(Status, STATUS_ACCESS_VIOLATION);

    /* Create a security descriptor */
    Status = RtlCreateSecurityDescriptor(&Sd, SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create a security descriptor, skipping tests\n");
        goto Quit;
    }

    /* This descriptor will have no group, owner and DACL */
    RtlSetGroupSecurityDescriptor(&Sd, NULL, FALSE);
    RtlSetOwnerSecurityDescriptor(&Sd, NULL, FALSE);
    RtlSetDaclSecurityDescriptor(&Sd, FALSE, NULL, FALSE);

    /* Give the invalid descriptor */
    Status = NtAccessCheckByType(&Sd,
                                 NULL,
                                 Token,
                                 MAXIMUM_ALLOWED,
                                 NULL,
                                 0,
                                 &RegMapping,
                                 PrivilegeSet,
                                 &PrivilegeSetLength,
                                 &GrantedAccess,
                                 &AccessStatus);
    ok_hex(Status, STATUS_INVALID_SECURITY_DESCR);

    /* Give a bogus principal SID */
    Status = NtAccessCheckByType(&Sd,
                                 (PSID)1,
                                 Token,
                                 MAXIMUM_ALLOWED,
                                 NULL,
                                 0,
                                 &RegMapping,
                                 PrivilegeSet,
                                 &PrivilegeSetLength,
                                 &GrantedAccess,
                                 &AccessStatus);

    /*
     * On newer versions of Windows such as 10 the principal SID is validated
     * after the security descriptor so it will always trigger an invalid
     * descriptor case. On Windows Server 2003 the principal SID is captured
     * before the security descriptor. ReactOS currently follows the Windows
     * 10's behavior.
     */
    ok(Status == STATUS_ACCESS_VIOLATION || Status == STATUS_INVALID_SECURITY_DESCR,
       "STATUS_ACCESS_VIOLATION or STATUS_INVALID_SECURITY_DESCR expected, got 0x%lx\n", Status);

    /* The first object element is not the root */
    ObjTypeList[0].Level = ACCESS_PROPERTY_SET_GUID;
    ObjTypeList[0].Sbz = 0;
    ObjTypeList[0].ObjectType = &ObjectType;

    /* Give a bogus list */
    Status = NtAccessCheckByType(&Sd,
                                 (PSID)1,
                                 Token,
                                 MAXIMUM_ALLOWED,
                                 ObjTypeList,
                                 RTL_NUMBER_OF(ObjTypeList),
                                 &RegMapping,
                                 PrivilegeSet,
                                 &PrivilegeSetLength,
                                 &GrantedAccess,
                                 &AccessStatus);
    ok_hex(Status, STATUS_INVALID_PARAMETER);

    /* This list has two roots */
    ObjTypeList[0].Level = ACCESS_OBJECT_GUID;
    ObjTypeList[0].Sbz = 0;
    ObjTypeList[0].ObjectType = &ObjectType;

    ObjTypeList[1].Level = ACCESS_OBJECT_GUID;
    ObjTypeList[1].Sbz = 0;
    ObjTypeList[1].ObjectType = &ChildObjectType;

    /* Give a bogus list */
    Status = NtAccessCheckByType(&Sd,
                                 (PSID)1,
                                 Token,
                                 MAXIMUM_ALLOWED,
                                 ObjTypeList,
                                 RTL_NUMBER_OF(ObjTypeList),
                                 &RegMapping,
                                 PrivilegeSet,
                                 &PrivilegeSetLength,
                                 &GrantedAccess,
                                 &AccessStatus);
    ok_hex(Status, STATUS_INVALID_PARAMETER);

    /* This list has an object whose level is invalid */
    ObjTypeList[0].Level = 0xa;
    ObjTypeList[0].Sbz = 0;
    ObjTypeList[0].ObjectType = &ObjectType;

    /* Give a bogus list */
    Status = NtAccessCheckByType(&Sd,
                                 (PSID)1,
                                 Token,
                                 MAXIMUM_ALLOWED,
                                 ObjTypeList,
                                 RTL_NUMBER_OF(ObjTypeList),
                                 &RegMapping,
                                 PrivilegeSet,
                                 &PrivilegeSetLength,
                                 &GrantedAccess,
                                 &AccessStatus);
    ok_hex(Status, STATUS_INVALID_PARAMETER);

    /* This list doesn't have consistent object levels */
    ObjTypeList[0].Level = ACCESS_OBJECT_GUID;
    ObjTypeList[0].Sbz = 0;
    ObjTypeList[0].ObjectType = &ObjectType;

    ObjTypeList[1].Level = ACCESS_PROPERTY_GUID;
    ObjTypeList[1].Sbz = 0;
    ObjTypeList[1].ObjectType = &ChildObjectType;

    /* Give a bogus list */
    Status = NtAccessCheckByType(&Sd,
                                 (PSID)1,
                                 Token,
                                 MAXIMUM_ALLOWED,
                                 ObjTypeList,
                                 RTL_NUMBER_OF(ObjTypeList),
                                 &RegMapping,
                                 PrivilegeSet,
                                 &PrivilegeSetLength,
                                 &GrantedAccess,
                                 &AccessStatus);
    ok_hex(Status, STATUS_INVALID_PARAMETER);

Quit:
    if (Token)
    {
        NtClose(Token);
    }

    if (PrivilegeSet)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, PrivilegeSet);
    }
}

static
VOID
AccessGrantedNoDaclTests(VOID)
{
    NTSTATUS Status;
    NTSTATUS AccessStatus;
    ACCESS_MASK GrantedAccess;
    PPRIVILEGE_SET PrivilegeSet = NULL;
    ULONG PrivilegeSetLength;
    HANDLE Token = NULL;
    SECURITY_DESCRIPTOR Sd;
    OBJECT_TYPE_LIST ObjTypeList[2];
    PSID AdminSid = NULL, UsersSid = NULL;

    /* Allocate all the stuff we need */
    PrivilegeSetLength = FIELD_OFFSET(PRIVILEGE_SET, Privilege[16]);
    PrivilegeSet = RtlAllocateHeap(RtlGetProcessHeap(), 0, PrivilegeSetLength);
    if (PrivilegeSet == NULL)
    {
        skip("Failed to allocate PrivilegeSet, skipping tests\n");
        return;
    }

    Status = RtlAllocateAndInitializeSid(&NtAuthority,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_ADMINS,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         &AdminSid);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create Admins SID, skipping tests\n");
        goto Quit;
    }

    Status = RtlAllocateAndInitializeSid(&NtAuthority,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_USERS,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         &UsersSid);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create User SID, skipping tests\n");
        goto Quit;
    }

    Token = GetTokenProcess(TRUE, TRUE);
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

    /* Setup the descriptor with no DACL */
    RtlSetGroupSecurityDescriptor(&Sd, UsersSid, FALSE);
    RtlSetOwnerSecurityDescriptor(&Sd, AdminSid, FALSE);
    RtlSetDaclSecurityDescriptor(&Sd, FALSE, NULL, FALSE);

    /* Setup the object type list */
    ObjTypeList[0].Level = ACCESS_OBJECT_GUID;
    ObjTypeList[0].Sbz = 0;
    ObjTypeList[0].ObjectType = &ObjectType;

    ObjTypeList[1].Level = ACCESS_PROPERTY_SET_GUID;
    ObjTypeList[1].Sbz = 0;
    ObjTypeList[1].ObjectType = &ChildObjectType;

    /*
     * Evaluate the access -- a SD with no DACL is always a granted
     * access as no ACL is enforced to protect the object.
     */
    Status = NtAccessCheckByType(&Sd,
                                 NULL,
                                 Token,
                                 KEY_READ,
                                 ObjTypeList,
                                 RTL_NUMBER_OF(ObjTypeList),
                                 &RegMapping,
                                 PrivilegeSet,
                                 &PrivilegeSetLength,
                                 &GrantedAccess,
                                 &AccessStatus);
    ok_hex(Status, STATUS_SUCCESS);
    ok(GrantedAccess == KEY_READ, "Expected KEY_READ as granted right but got 0x%08lx\n", GrantedAccess);
    ok(AccessStatus == STATUS_SUCCESS, "Expected a success status but got 0x%08lx\n", AccessStatus);
    ok(PrivilegeSet != NULL, "PrivilegeSet is NULL when it mustn't be!\n");
    ok(PrivilegeSetLength != 0, "PrivilegeSetLength mustn't be 0!\n");

    /* Evaluate access for MAXIMUM_ALLOWED as well */
    Status = NtAccessCheckByType(&Sd,
                                 NULL,
                                 Token,
                                 MAXIMUM_ALLOWED,
                                 ObjTypeList,
                                 RTL_NUMBER_OF(ObjTypeList),
                                 &RegMapping,
                                 PrivilegeSet,
                                 &PrivilegeSetLength,
                                 &GrantedAccess,
                                 &AccessStatus);
    ok_hex(Status, STATUS_SUCCESS);
    ok(GrantedAccess == KEY_ALL_ACCESS, "Expected KEY_ALL_ACCESS as granted right but got 0x%08lx\n", GrantedAccess);
    ok(AccessStatus == STATUS_SUCCESS, "Expected a success status but got 0x%08lx\n", AccessStatus);
    ok(PrivilegeSet != NULL, "PrivilegeSet is NULL when it mustn't be!\n");
    ok(PrivilegeSetLength != 0, "PrivilegeSetLength mustn't be 0!\n");

    /*
     * Evaluate access but with no object type list.
     * NtAccessCheckByType will be treated as a normal NtAccessCheck.
     */
    Status = NtAccessCheckByType(&Sd,
                                 NULL,
                                 Token,
                                 KEY_READ,
                                 NULL,
                                 0,
                                 &RegMapping,
                                 PrivilegeSet,
                                 &PrivilegeSetLength,
                                 &GrantedAccess,
                                 &AccessStatus);
    ok_hex(Status, STATUS_SUCCESS);
    ok(GrantedAccess == KEY_READ, "Expected KEY_READ as granted right but got 0x%08lx\n", GrantedAccess);
    ok(AccessStatus == STATUS_SUCCESS, "Expected a success status but got 0x%08lx\n", AccessStatus);
    ok(PrivilegeSet != NULL, "PrivilegeSet is NULL when it mustn't be!\n");
    ok(PrivilegeSetLength != 0, "PrivilegeSetLength mustn't be 0!\n");

Quit:
    if (Token)
    {
        NtClose(Token);
    }

    if (UsersSid)
    {
        RtlFreeSid(UsersSid);
    }

    if (AdminSid)
    {
        RtlFreeSid(AdminSid);
    }

    if (PrivilegeSet)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, PrivilegeSet);
    }
}

static
VOID
AccessGrantedTests(VOID)
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
    OBJECT_TYPE_LIST ObjTypeList[2];
    PSID EveryoneSid = NULL, AdminSid = NULL, UsersSid = NULL;

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
                                         &EveryoneSid);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create Everyone SID, skipping tests\n");
        goto Quit;
    }

    Status = RtlAllocateAndInitializeSid(&NtAuthority,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_ADMINS,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         &AdminSid);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create Admins SID, skipping tests\n");
        goto Quit;
    }

    Status = RtlAllocateAndInitializeSid(&NtAuthority,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_USERS,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         &UsersSid);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create User SID, skipping tests\n");
        goto Quit;
    }

    Token = GetTokenProcess(TRUE, TRUE);
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
               sizeof(ACCESS_ALLOWED_OBJECT_ACE) + RtlLengthSid(EveryoneSid) +
               sizeof(ACCESS_ALLOWED_OBJECT_ACE) + RtlLengthSid(AdminSid);
    Dacl = RtlAllocateHeap(RtlGetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           DaclSize);
    if (Dacl == NULL)
    {
        skip("Failed to allocate memory for DACL, skipping tests\n");
        goto Quit;
    }

    /*
     * Create an ordinary ACL with an old revision. Object types are supported
     * starting with Revision 4, so adding ACEs to it will fail.
     */
    Status = RtlCreateAcl(Dacl,
                          DaclSize,
                          ACL_REVISION);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create DACL, skipping tests\n");
        goto Quit;
    }

    Status = RtlAddAccessAllowedObjectAce(Dacl,
                                          ACL_REVISION,
                                          0,
                                          KEY_READ,
                                          &ObjectType,
                                          NULL,
                                          EveryoneSid);
    ok_hex(Status, STATUS_REVISION_MISMATCH);

    /* Free the ACL and create a proper one with correct revision */
    RtlFreeHeap(RtlGetProcessHeap(), 0, Dacl);
    Dacl = RtlAllocateHeap(RtlGetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           DaclSize);
    if (Dacl == NULL)
    {
        skip("Failed to allocate memory for DACL, skipping tests\n");
        goto Quit;
    }

    Status = RtlCreateAcl(Dacl,
                          DaclSize,
                          ACL_REVISION4);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create DACL, skipping tests\n");
        goto Quit;
    }

    /* Setup ACEs -- READ for everyone and READ/WRITE for admins */
    Status = RtlAddAccessAllowedObjectAce(Dacl,
                                          ACL_REVISION4,
                                          0,
                                          KEY_READ,
                                          &ObjectType,
                                          NULL,
                                          EveryoneSid);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to add allowed object ACE for Everyone SID, skipping tests\n");
        goto Quit;
    }

    Status = RtlAddAccessAllowedObjectAce(Dacl,
                                          ACL_REVISION4,
                                          0,
                                          KEY_READ | KEY_WRITE,
                                          &ObjectType,
                                          NULL,
                                          AdminSid);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to add allowed object ACE for Admins SID, skipping tests\n");
        goto Quit;
    }

    /* Setup the descriptor */
    RtlSetGroupSecurityDescriptor(&Sd, UsersSid, FALSE);
    RtlSetOwnerSecurityDescriptor(&Sd, AdminSid, FALSE);
    RtlSetDaclSecurityDescriptor(&Sd, TRUE, Dacl, FALSE);

    /* Setup the object type list */
    ObjTypeList[0].Level = ACCESS_OBJECT_GUID;
    ObjTypeList[0].Sbz = 0;
    ObjTypeList[0].ObjectType = &ObjectType;

    ObjTypeList[1].Level = ACCESS_PROPERTY_SET_GUID;
    ObjTypeList[1].Sbz = 0;
    ObjTypeList[1].ObjectType = &ChildObjectType;

    /* Evaluate access -- KEY_READ has to be granted */
    Status = NtAccessCheckByType(&Sd,
                                 NULL,
                                 Token,
                                 KEY_READ,
                                 ObjTypeList,
                                 RTL_NUMBER_OF(ObjTypeList),
                                 &RegMapping,
                                 PrivilegeSet,
                                 &PrivilegeSetLength,
                                 &GrantedAccess,
                                 &AccessStatus);
    ok_hex(Status, STATUS_SUCCESS);
    ok(GrantedAccess == KEY_READ, "Expected KEY_READ as granted right but got 0x%08lx\n", GrantedAccess);
    ok(AccessStatus == STATUS_SUCCESS, "Expected a success status but got 0x%08lx\n", AccessStatus);
    ok(PrivilegeSet != NULL, "PrivilegeSet is NULL when it mustn't be!\n");
    ok(PrivilegeSetLength != 0, "PrivilegeSetLength mustn't be 0!\n");

    /* Admins should be granted WRITE access */
    Status = NtAccessCheckByType(&Sd,
                                 NULL,
                                 Token,
                                 KEY_WRITE,
                                 ObjTypeList,
                                 RTL_NUMBER_OF(ObjTypeList),
                                 &RegMapping,
                                 PrivilegeSet,
                                 &PrivilegeSetLength,
                                 &GrantedAccess,
                                 &AccessStatus);
    ok_hex(Status, STATUS_SUCCESS);
    ok(GrantedAccess == KEY_WRITE, "Expected KEY_WRITE as granted right but got 0x%08lx\n", GrantedAccess);
    ok(AccessStatus == STATUS_SUCCESS, "Expected a success status but got 0x%08lx\n", AccessStatus);
    ok(PrivilegeSet != NULL, "PrivilegeSet is NULL when it mustn't be!\n");
    ok(PrivilegeSetLength != 0, "PrivilegeSetLength mustn't be 0!\n");

Quit:
    if (Dacl)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, Dacl);
    }

    if (Token)
    {
        NtClose(Token);
    }

    if (UsersSid)
    {
        RtlFreeSid(UsersSid);
    }

    if (AdminSid)
    {
        RtlFreeSid(AdminSid);
    }

    if (EveryoneSid)
    {
        RtlFreeSid(EveryoneSid);
    }

    if (PrivilegeSet)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, PrivilegeSet);
    }
}

static
VOID
AccessGrantedMultipleObjectsTests(VOID)
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
    OBJECT_TYPE_LIST ObjTypeList[6];
    PSID EveryoneSid = NULL, AdminSid = NULL, UsersSid = NULL;
    GUID ChildObjectType2 = {0x34578901, 0x3456, 0x7896, {0x3, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00}};
    GUID ChildObjectType3 = {0x45678901, 0x4567, 0x1122, {0x4, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x01}};
    GUID ChildObjectType4 = {0x56788901, 0x1111, 0x2222, {0x5, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x02}};
    GUID ChildObjectType5 = {0x67901234, 0x2222, 0x3333, {0x4, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x03}};

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
                                         &EveryoneSid);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create Everyone SID, skipping tests\n");
        goto Quit;
    }

    Status = RtlAllocateAndInitializeSid(&NtAuthority,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_ADMINS,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         &AdminSid);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create Admins SID, skipping tests\n");
        goto Quit;
    }

    Status = RtlAllocateAndInitializeSid(&NtAuthority,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_USERS,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         &UsersSid);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create User SID, skipping tests\n");
        goto Quit;
    }

    Token = GetTokenProcess(TRUE, TRUE);
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
               sizeof(ACCESS_ALLOWED_OBJECT_ACE) + RtlLengthSid(AdminSid) +
               sizeof(ACCESS_ALLOWED_OBJECT_ACE) + RtlLengthSid(EveryoneSid) +
               sizeof(ACCESS_ALLOWED_OBJECT_ACE) + RtlLengthSid(EveryoneSid) +
               sizeof(ACCESS_ALLOWED_OBJECT_ACE) + RtlLengthSid(EveryoneSid) +
               sizeof(ACCESS_ALLOWED_OBJECT_ACE) + RtlLengthSid(EveryoneSid) +
               sizeof(ACCESS_ALLOWED_OBJECT_ACE) + RtlLengthSid(EveryoneSid);
    Dacl = RtlAllocateHeap(RtlGetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           DaclSize);
    if (Dacl == NULL)
    {
        skip("Failed to allocate memory for DACL, skipping tests\n");
        goto Quit;
    }

    Status = RtlCreateAcl(Dacl,
                          DaclSize,
                          ACL_REVISION4);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create DACL, skipping tests\n");
        goto Quit;
    }

    /* Setup some rights for these objects, admins are granted everything */
    Status = RtlAddAccessAllowedObjectAce(Dacl,
                                          ACL_REVISION4,
                                          0,
                                          KEY_ALL_ACCESS,
                                          &ObjectType,
                                          NULL,
                                          AdminSid);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to add allowed object ACE for Admins SID, skipping tests\n");
        goto Quit;
    }

    Status = RtlAddAccessAllowedObjectAce(Dacl,
                                          ACL_REVISION4,
                                          0,
                                          KEY_READ,
                                          &ChildObjectType,
                                          NULL,
                                          EveryoneSid);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to add allowed object ACE for Everyone SID, skipping tests\n");
        goto Quit;
    }

    Status = RtlAddAccessAllowedObjectAce(Dacl,
                                          ACL_REVISION4,
                                          0,
                                          KEY_WRITE,
                                          &ChildObjectType2,
                                          NULL,
                                          EveryoneSid);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to add allowed object ACE for Everyone SID, skipping tests\n");
        goto Quit;
    }

    Status = RtlAddAccessAllowedObjectAce(Dacl,
                                          ACL_REVISION4,
                                          0,
                                          KEY_EXECUTE,
                                          &ChildObjectType3,
                                          NULL,
                                          EveryoneSid);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to add allowed object ACE for Everyone SID, skipping tests\n");
        goto Quit;
    }

    Status = RtlAddAccessAllowedObjectAce(Dacl,
                                          ACL_REVISION4,
                                          0,
                                          KEY_SET_VALUE,
                                          &ChildObjectType4,
                                          NULL,
                                          EveryoneSid);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to add allowed object ACE for Everyone SID, skipping tests\n");
        goto Quit;
    }

    Status = RtlAddAccessAllowedObjectAce(Dacl,
                                          ACL_REVISION4,
                                          0,
                                          KEY_QUERY_VALUE,
                                          &ChildObjectType5,
                                          NULL,
                                          EveryoneSid);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to add allowed object ACE for Everyone SID, skipping tests\n");
        goto Quit;
    }

    /* Setup the descriptor */
    RtlSetGroupSecurityDescriptor(&Sd, UsersSid, FALSE);
    RtlSetOwnerSecurityDescriptor(&Sd, AdminSid, FALSE);
    RtlSetDaclSecurityDescriptor(&Sd, TRUE, Dacl, FALSE);

    /* Setup the object type list */
    ObjTypeList[0].Level = ACCESS_OBJECT_GUID;
    ObjTypeList[0].Sbz = 0;
    ObjTypeList[0].ObjectType = &ObjectType;

    ObjTypeList[1].Level = ACCESS_PROPERTY_SET_GUID;
    ObjTypeList[1].Sbz = 0;
    ObjTypeList[1].ObjectType = &ChildObjectType;

    ObjTypeList[2].Level = ACCESS_PROPERTY_GUID;
    ObjTypeList[2].Sbz = 0;
    ObjTypeList[2].ObjectType = &ChildObjectType2;

    ObjTypeList[3].Level = ACCESS_PROPERTY_GUID;
    ObjTypeList[3].Sbz = 0;
    ObjTypeList[3].ObjectType = &ChildObjectType3;

    ObjTypeList[4].Level = ACCESS_PROPERTY_SET_GUID;
    ObjTypeList[4].Sbz = 0;
    ObjTypeList[4].ObjectType = &ChildObjectType4;

    ObjTypeList[5].Level = ACCESS_PROPERTY_GUID;
    ObjTypeList[5].Sbz = 0;
    ObjTypeList[5].ObjectType = &ChildObjectType5;

    /* Evaluate access for each object case */
    Status = NtAccessCheckByType(&Sd,
                                 NULL,
                                 Token,
                                 MAXIMUM_ALLOWED,
                                 ObjTypeList,
                                 RTL_NUMBER_OF(ObjTypeList),
                                 &RegMapping,
                                 PrivilegeSet,
                                 &PrivilegeSetLength,
                                 &GrantedAccess,
                                 &AccessStatus);
    ok_hex(Status, STATUS_SUCCESS);
    ok(GrantedAccess == KEY_ALL_ACCESS, "Expected KEY_ALL_ACCESS as granted right but got 0x%08lx\n", GrantedAccess);
    ok(AccessStatus == STATUS_SUCCESS, "Expected a success status but got 0x%08lx\n", AccessStatus);

    Status = NtAccessCheckByType(&Sd,
                                 NULL,
                                 Token,
                                 KEY_READ,
                                 ObjTypeList,
                                 RTL_NUMBER_OF(ObjTypeList),
                                 &RegMapping,
                                 PrivilegeSet,
                                 &PrivilegeSetLength,
                                 &GrantedAccess,
                                 &AccessStatus);
    ok_hex(Status, STATUS_SUCCESS);
    ok(GrantedAccess == KEY_READ, "Expected KEY_READ as granted right but got 0x%08lx\n", GrantedAccess);
    ok(AccessStatus == STATUS_SUCCESS, "Expected a success status but got 0x%08lx\n", AccessStatus);

    Status = NtAccessCheckByType(&Sd,
                                 NULL,
                                 Token,
                                 KEY_WRITE,
                                 ObjTypeList,
                                 RTL_NUMBER_OF(ObjTypeList),
                                 &RegMapping,
                                 PrivilegeSet,
                                 &PrivilegeSetLength,
                                 &GrantedAccess,
                                 &AccessStatus);
    ok_hex(Status, STATUS_SUCCESS);
    ok(GrantedAccess == KEY_WRITE, "Expected KEY_WRITE as granted right but got 0x%08lx\n", GrantedAccess);
    ok(AccessStatus == STATUS_SUCCESS, "Expected a success status but got 0x%08lx\n", AccessStatus);

    Status = NtAccessCheckByType(&Sd,
                                 NULL,
                                 Token,
                                 KEY_EXECUTE,
                                 ObjTypeList,
                                 RTL_NUMBER_OF(ObjTypeList),
                                 &RegMapping,
                                 PrivilegeSet,
                                 &PrivilegeSetLength,
                                 &GrantedAccess,
                                 &AccessStatus);
    ok_hex(Status, STATUS_SUCCESS);
    ok(GrantedAccess == KEY_EXECUTE, "Expected KEY_EXECUTE as granted right but got 0x%08lx\n", GrantedAccess);
    ok(AccessStatus == STATUS_SUCCESS, "Expected a success status but got 0x%08lx\n", AccessStatus);

    Status = NtAccessCheckByType(&Sd,
                                 NULL,
                                 Token,
                                 KEY_SET_VALUE,
                                 ObjTypeList,
                                 RTL_NUMBER_OF(ObjTypeList),
                                 &RegMapping,
                                 PrivilegeSet,
                                 &PrivilegeSetLength,
                                 &GrantedAccess,
                                 &AccessStatus);
    ok_hex(Status, STATUS_SUCCESS);
    ok(GrantedAccess == KEY_SET_VALUE, "Expected KEY_SET_VALUE as granted right but got 0x%08lx\n", GrantedAccess);
    ok(AccessStatus == STATUS_SUCCESS, "Expected a success status but got 0x%08lx\n", AccessStatus);

    Status = NtAccessCheckByType(&Sd,
                                 NULL,
                                 Token,
                                 KEY_QUERY_VALUE,
                                 ObjTypeList,
                                 RTL_NUMBER_OF(ObjTypeList),
                                 &RegMapping,
                                 PrivilegeSet,
                                 &PrivilegeSetLength,
                                 &GrantedAccess,
                                 &AccessStatus);
    ok_hex(Status, STATUS_SUCCESS);
    ok(GrantedAccess == KEY_QUERY_VALUE, "Expected KEY_QUERY_VALUE as granted right but got 0x%08lx\n", GrantedAccess);
    ok(AccessStatus == STATUS_SUCCESS, "Expected a success status but got 0x%08lx\n", AccessStatus);

Quit:
    if (Dacl)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, Dacl);
    }

    if (Token)
    {
        NtClose(Token);
    }

    if (UsersSid)
    {
        RtlFreeSid(UsersSid);
    }

    if (AdminSid)
    {
        RtlFreeSid(AdminSid);
    }

    if (EveryoneSid)
    {
        RtlFreeSid(EveryoneSid);
    }

    if (PrivilegeSet)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, PrivilegeSet);
    }
}

static
VOID
DenyAccessTests(VOID)
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
    OBJECT_TYPE_LIST ObjTypeList[2];
    PSID EveryoneSid = NULL, AdminSid = NULL, UsersSid = NULL;

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
                                         &EveryoneSid);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create Everyone SID, skipping tests\n");
        goto Quit;
    }

    Status = RtlAllocateAndInitializeSid(&NtAuthority,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_ADMINS,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         &AdminSid);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create Admins SID, skipping tests\n");
        goto Quit;
    }

    Status = RtlAllocateAndInitializeSid(&NtAuthority,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_USERS,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         &UsersSid);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create User SID, skipping tests\n");
        goto Quit;
    }

    Token = GetTokenProcess(TRUE, TRUE);
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
               sizeof(ACCESS_DENIED_OBJECT_ACE) + RtlLengthSid(AdminSid) +
               sizeof(ACCESS_DENIED_OBJECT_ACE) + RtlLengthSid(EveryoneSid) +
               sizeof(ACCESS_ALLOWED_OBJECT_ACE) + RtlLengthSid(EveryoneSid);
    Dacl = RtlAllocateHeap(RtlGetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           DaclSize);
    if (Dacl == NULL)
    {
        skip("Failed to allocate memory for DACL, skipping tests\n");
        goto Quit;
    }

    Status = RtlCreateAcl(Dacl,
                          DaclSize,
                          ACL_REVISION4);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create DACL, skipping tests\n");
        goto Quit;
    }

    /*
     * Admins can't query values from keys and everyone else is denied writing to
     * keys to child sub-object but can enumerate subkeys from the object itself.
     */
    Status = RtlAddAccessDeniedObjectAce(Dacl,
                                         ACL_REVISION4,
                                         0,
                                         KEY_QUERY_VALUE,
                                         &ObjectType,
                                         NULL,
                                         AdminSid);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to add deny object ACE for Admins SID, skipping tests\n");
        goto Quit;
    }

    Status = RtlAddAccessDeniedObjectAce(Dacl,
                                         ACL_REVISION4,
                                         0,
                                         KEY_WRITE,
                                         &ChildObjectType,
                                         NULL,
                                         EveryoneSid);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to add deny object ACE for Everyone SID, skipping tests\n");
        goto Quit;
    }

    Status = RtlAddAccessAllowedObjectAce(Dacl,
                                          ACL_REVISION4,
                                          0,
                                          KEY_ENUMERATE_SUB_KEYS,
                                          &ObjectType,
                                          NULL,
                                          EveryoneSid);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to add allowed object ACE for Everyone SID, skipping tests\n");
        goto Quit;
    }

    /* Setup the descriptor */
    RtlSetGroupSecurityDescriptor(&Sd, UsersSid, FALSE);
    RtlSetOwnerSecurityDescriptor(&Sd, AdminSid, FALSE);
    RtlSetDaclSecurityDescriptor(&Sd, TRUE, Dacl, FALSE);

    /* Setup the object type list */
    ObjTypeList[0].Level = ACCESS_OBJECT_GUID;
    ObjTypeList[0].Sbz = 0;
    ObjTypeList[0].ObjectType = &ObjectType;

    ObjTypeList[1].Level = ACCESS_PROPERTY_SET_GUID;
    ObjTypeList[1].Sbz = 0;
    ObjTypeList[1].ObjectType = &ChildObjectType;

    /* Evaluate access -- they should be denied */
    Status = NtAccessCheckByType(&Sd,
                                 NULL,
                                 Token,
                                 KEY_QUERY_VALUE,
                                 ObjTypeList,
                                 RTL_NUMBER_OF(ObjTypeList),
                                 &RegMapping,
                                 PrivilegeSet,
                                 &PrivilegeSetLength,
                                 &GrantedAccess,
                                 &AccessStatus);
    ok_hex(Status, STATUS_SUCCESS);
    ok(GrantedAccess == 0, "Expected no access rights but got 0x%08lx\n", GrantedAccess);
    ok(AccessStatus == STATUS_ACCESS_DENIED, "Expected access denied as status but got 0x%08lx\n", AccessStatus);

    Status = NtAccessCheckByType(&Sd,
                                 NULL,
                                 Token,
                                 KEY_WRITE,
                                 ObjTypeList,
                                 RTL_NUMBER_OF(ObjTypeList),
                                 &RegMapping,
                                 PrivilegeSet,
                                 &PrivilegeSetLength,
                                 &GrantedAccess,
                                 &AccessStatus);
    ok_hex(Status, STATUS_SUCCESS);
    ok(GrantedAccess == 0, "Expected no access rights but got 0x%08lx\n", GrantedAccess);
    ok(AccessStatus == STATUS_ACCESS_DENIED, "Expected access denied as status but got 0x%08lx\n", AccessStatus);

    /* Everyone else should be granted only enumerate subkey access */
    Status = NtAccessCheckByType(&Sd,
                                 NULL,
                                 Token,
                                 KEY_ENUMERATE_SUB_KEYS,
                                 ObjTypeList,
                                 RTL_NUMBER_OF(ObjTypeList),
                                 &RegMapping,
                                 PrivilegeSet,
                                 &PrivilegeSetLength,
                                 &GrantedAccess,
                                 &AccessStatus);
    ok_hex(Status, STATUS_SUCCESS);
    ok(GrantedAccess == KEY_ENUMERATE_SUB_KEYS, "Expected KEY_ENUMERATE_SUB_KEYS as granted right but got 0x%08lx\n", GrantedAccess);
    ok(AccessStatus == STATUS_SUCCESS, "Expected a success status but got 0x%08lx\n", AccessStatus);

Quit:
    if (Dacl)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, Dacl);
    }

    if (Token)
    {
        NtClose(Token);
    }

    if (UsersSid)
    {
        RtlFreeSid(UsersSid);
    }

    if (AdminSid)
    {
        RtlFreeSid(AdminSid);
    }

    if (EveryoneSid)
    {
        RtlFreeSid(EveryoneSid);
    }

    if (PrivilegeSet)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, PrivilegeSet);
    }
}

START_TEST(NtAccessCheckByType)
{
    ParamsValidationTests();
    AccessGrantedNoDaclTests();
    AccessGrantedTests();
    AccessGrantedMultipleObjectsTests();
    DenyAccessTests();
}
