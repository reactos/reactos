/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmse.c
 * PURPOSE:         Configuration Manager - Security Subsystem Interface
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
#define NDEBUG
#include "debug.h"

/* GLOBALS *******************************************************************/

/* FUNCTIONS *****************************************************************/

PSECURITY_DESCRIPTOR
NTAPI
CmpHiveRootSecurityDescriptor(VOID)
{
    NTSTATUS Status;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    PACL Acl, AclCopy;
    PSID Sid[4];
    SID_IDENTIFIER_AUTHORITY WorldAuthority = {SECURITY_WORLD_SID_AUTHORITY};
    SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};
    ULONG AceLength, AclLength, SidLength;
    PACE_HEADER AceHeader;
    ULONG i;
    PAGED_CODE();

    /* Phase 1: Allocate SIDs */
    SidLength = RtlLengthRequiredSid(1);
    Sid[0] = ExAllocatePoolWithTag(PagedPool, SidLength, TAG_CM);
    Sid[1] = ExAllocatePoolWithTag(PagedPool, SidLength, TAG_CM);
    Sid[2] = ExAllocatePoolWithTag(PagedPool, SidLength, TAG_CM);
    SidLength = RtlLengthRequiredSid(2);
    Sid[3] = ExAllocatePoolWithTag(PagedPool, SidLength, TAG_CM);

    /* Make sure all SIDs were allocated */
    if (!(Sid[0]) || !(Sid[1]) || !(Sid[2]) || !(Sid[3]))
    {
        /* Bugcheck */
        KeBugCheckEx(REGISTRY_ERROR, 11, 1, 0, 0);
    }

    /* Phase 2: Initialize all SIDs */
    Status = RtlInitializeSid(Sid[0], &WorldAuthority, 1);
    Status |= RtlInitializeSid(Sid[1], &NtAuthority, 1);
    Status |= RtlInitializeSid(Sid[2], &NtAuthority, 1);
    Status |= RtlInitializeSid(Sid[3], &NtAuthority, 2);
    if (!NT_SUCCESS(Status)) KeBugCheckEx(REGISTRY_ERROR, 11, 2, 0, 0);

    /* Phase 2: Setup SID Sub Authorities */
    *RtlSubAuthoritySid(Sid[0], 0) = SECURITY_WORLD_RID;
    *RtlSubAuthoritySid(Sid[1], 0) = SECURITY_RESTRICTED_CODE_RID;
    *RtlSubAuthoritySid(Sid[2], 0) = SECURITY_LOCAL_SYSTEM_RID;
    *RtlSubAuthoritySid(Sid[3], 0) = SECURITY_BUILTIN_DOMAIN_RID;
    *RtlSubAuthoritySid(Sid[3], 1) = DOMAIN_ALIAS_RID_ADMINS;

    /* Make sure all SIDs are valid */
    ASSERT(RtlValidSid(Sid[0]));
    ASSERT(RtlValidSid(Sid[1]));
    ASSERT(RtlValidSid(Sid[2]));
    ASSERT(RtlValidSid(Sid[3]));

    /* Phase 3: Calculate ACL Length */
    AclLength = sizeof(ACL);
    for (i = 0; i < 4; i++)
    {
        /* This is what MSDN says to do */
        AceLength = FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart);
        AceLength += SeLengthSid(Sid[i]);
        AclLength += AceLength;
    }

    /* Phase 3: Allocate the ACL */
    Acl = ExAllocatePoolWithTag(PagedPool, AclLength, TAG_CM);
    if (!Acl) KeBugCheckEx(REGISTRY_ERROR, 11, 3, 0, 0);

    /* Phase 4: Create the ACL */
    Status = RtlCreateAcl(Acl, AclLength, ACL_REVISION);
    if (!NT_SUCCESS(Status)) KeBugCheckEx(REGISTRY_ERROR, 11, 4, Status, 0);

    /* Phase 5: Build the ACL */
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION, KEY_ALL_ACCESS, Sid[0]);
    Status |= RtlAddAccessAllowedAce(Acl, ACL_REVISION, KEY_ALL_ACCESS, Sid[1]);
    Status |= RtlAddAccessAllowedAce(Acl, ACL_REVISION, KEY_READ, Sid[2]);
    Status |= RtlAddAccessAllowedAce(Acl, ACL_REVISION, KEY_READ, Sid[3]);
    if (!NT_SUCCESS(Status)) KeBugCheckEx(REGISTRY_ERROR, 11, 5, Status, 0);

    /* Phase 5: Make the ACEs inheritable */
    Status = RtlGetAce(Acl, 0,( PVOID*)&AceHeader);
    ASSERT(NT_SUCCESS(Status));
    AceHeader->AceFlags |= CONTAINER_INHERIT_ACE;
    Status = RtlGetAce(Acl, 1, (PVOID*)&AceHeader);
    ASSERT(NT_SUCCESS(Status));
    AceHeader->AceFlags |= CONTAINER_INHERIT_ACE;
    Status = RtlGetAce(Acl, 2, (PVOID*)&AceHeader);
    ASSERT(NT_SUCCESS(Status));
    AceHeader->AceFlags |= CONTAINER_INHERIT_ACE;
    Status = RtlGetAce(Acl, 3, (PVOID*)&AceHeader);
    ASSERT(NT_SUCCESS(Status));
    AceHeader->AceFlags |= CONTAINER_INHERIT_ACE;

    /* Phase 6: Allocate the security descriptor and make space for the ACL */
    SecurityDescriptor = ExAllocatePoolWithTag(PagedPool,
                                               sizeof(SECURITY_DESCRIPTOR) +
                                               AclLength,
                                               TAG_CM);
    if (!SecurityDescriptor) KeBugCheckEx(REGISTRY_ERROR, 11, 6, 0, 0);

    /* Phase 6: Make a copy of the ACL */
    AclCopy = (PACL)((PISECURITY_DESCRIPTOR)SecurityDescriptor + 1);
    RtlCopyMemory(AclCopy, Acl, AclLength);

    /* Phase 7: Create the security descriptor */
    Status = RtlCreateSecurityDescriptor(SecurityDescriptor,
                                         SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status)) KeBugCheckEx(REGISTRY_ERROR, 11, 7, Status, 0);

    /* Phase 8: Set the ACL as a DACL */
    Status = RtlSetDaclSecurityDescriptor(SecurityDescriptor,
                                          TRUE,
                                          AclCopy,
                                          FALSE);
    if (!NT_SUCCESS(Status)) KeBugCheckEx(REGISTRY_ERROR, 11, 8, Status, 0);

    /* Free the SIDs and original ACL */
    for (i = 0; i < 4; i++) ExFreePoolWithTag(Sid[i], TAG_CM);
    ExFreePoolWithTag(Acl, TAG_CM);

    /* Return the security descriptor */
    return SecurityDescriptor;
}

NTSTATUS
NTAPI
CmpSecurityMethod(IN PVOID ObjectBody,
                  IN SECURITY_OPERATION_CODE OperationCode,
                  IN PSECURITY_INFORMATION SecurityInformation,
                  IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
                  IN OUT PULONG BufferLength,
                  IN OUT PSECURITY_DESCRIPTOR *OldSecurityDescriptor,
                  IN POOL_TYPE PoolType,
                  IN PGENERIC_MAPPING GenericMapping)
{
    /* HACK */
    return STATUS_SUCCESS;
}
