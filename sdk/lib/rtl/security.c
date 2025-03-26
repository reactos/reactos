/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * FILE:              lib/rtl/security.c
 * PURPOSE:           Security related functions and Security Objects
 * PROGRAMMER:        Eric Kohl
 */

/* INCLUDES *******************************************************************/

#include <rtl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

NTSTATUS
NTAPI
RtlpSetSecurityObject(IN PVOID Object OPTIONAL,
                      IN SECURITY_INFORMATION SecurityInformation,
                      IN PSECURITY_DESCRIPTOR ModificationDescriptor,
                      IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
                      IN ULONG AutoInheritFlags,
                      IN ULONG PoolType,
                      IN PGENERIC_MAPPING GenericMapping,
                      IN HANDLE Token OPTIONAL)
{
    PISECURITY_DESCRIPTOR_RELATIVE pNewSd = NULL;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    PACL pDacl = NULL;
    PACL pSacl = NULL;
    BOOLEAN Defaulted;
    BOOLEAN Present;
    ULONG ulOwnerSidSize = 0, ulGroupSidSize = 0;
    ULONG ulDaclSize = 0, ulSaclSize = 0;
    ULONG ulNewSdSize;
    SECURITY_DESCRIPTOR_CONTROL Control = SE_SELF_RELATIVE;
    PUCHAR pDest;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("RtlpSetSecurityObject()\n");

    /* Change the Owner SID */
    if (SecurityInformation & OWNER_SECURITY_INFORMATION)
    {
        Status = RtlGetOwnerSecurityDescriptor(ModificationDescriptor, &pOwnerSid, &Defaulted);
        if (!NT_SUCCESS(Status))
            return Status;
    }
    else
    {
        Status = RtlGetOwnerSecurityDescriptor(*ObjectsSecurityDescriptor, &pOwnerSid, &Defaulted);
        if (!NT_SUCCESS(Status))
            return Status;
    }

    if (pOwnerSid == NULL || !RtlValidSid(pOwnerSid))
        return STATUS_INVALID_OWNER;

    ulOwnerSidSize = RtlLengthSid(pOwnerSid);

    /* Change the Group SID */
    if (SecurityInformation & GROUP_SECURITY_INFORMATION)
    {
        Status = RtlGetGroupSecurityDescriptor(ModificationDescriptor, &pGroupSid, &Defaulted);
        if (!NT_SUCCESS(Status))
            return Status;
    }
    else
    {
        Status = RtlGetGroupSecurityDescriptor(*ObjectsSecurityDescriptor, &pGroupSid, &Defaulted);
        if (!NT_SUCCESS(Status))
            return Status;
    }

    if (pGroupSid == NULL || !RtlValidSid(pGroupSid))
        return STATUS_INVALID_PRIMARY_GROUP;

    ulGroupSidSize = ROUND_UP(RtlLengthSid(pGroupSid), sizeof(ULONG));

    /* Change the DACL */
    if (SecurityInformation & DACL_SECURITY_INFORMATION)
    {
        Status = RtlGetDaclSecurityDescriptor(ModificationDescriptor, &Present, &pDacl, &Defaulted);
        if (!NT_SUCCESS(Status))
            return Status;

        Control |= SE_DACL_PRESENT;
    }
    else
    {
        Status = RtlGetDaclSecurityDescriptor(*ObjectsSecurityDescriptor, &Present, &pDacl, &Defaulted);
        if (!NT_SUCCESS(Status))
            return Status;

        if (Present)
            Control |= SE_DACL_PRESENT;

        if (Defaulted)
            Control |= SE_DACL_DEFAULTED;
    }

    if (pDacl != NULL)
        ulDaclSize = pDacl->AclSize;

    /* Change the SACL */
    if (SecurityInformation & SACL_SECURITY_INFORMATION)
    {
        Status = RtlGetSaclSecurityDescriptor(ModificationDescriptor, &Present, &pSacl, &Defaulted);
        if (!NT_SUCCESS(Status))
            return Status;

        Control |= SE_SACL_PRESENT;
    }
    else
    {
        Status = RtlGetSaclSecurityDescriptor(*ObjectsSecurityDescriptor, &Present, &pSacl, &Defaulted);
        if (!NT_SUCCESS(Status))
            return Status;

        if (Present)
            Control |= SE_SACL_PRESENT;

        if (Defaulted)
            Control |= SE_SACL_DEFAULTED;
    }

    if (pSacl != NULL)
        ulSaclSize = pSacl->AclSize;

    /* Calculate the size of the new security descriptor */
    ulNewSdSize = sizeof(SECURITY_DESCRIPTOR_RELATIVE) +
                  ROUND_UP(ulOwnerSidSize, sizeof(ULONG)) +
                  ROUND_UP(ulGroupSidSize, sizeof(ULONG)) +
                  ROUND_UP(ulDaclSize, sizeof(ULONG)) +
                  ROUND_UP(ulSaclSize, sizeof(ULONG));

    /* Allocate the new security descriptor */
    pNewSd = RtlAllocateHeap(RtlGetProcessHeap(), 0, ulNewSdSize);
    if (pNewSd == NULL)
    {
        Status = STATUS_NO_MEMORY;
        DPRINT1("New security descriptor allocation failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    /* Initialize the new security descriptor */
    Status = RtlCreateSecurityDescriptorRelative(pNewSd, SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("New security descriptor creation failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    /* Set the security descriptor control flags */
    pNewSd->Control = Control;

    pDest = (PUCHAR)((ULONG_PTR)pNewSd + sizeof(SECURITY_DESCRIPTOR_RELATIVE));

    /* Copy the SACL */
    if (pSacl != NULL)
    {
        RtlCopyMemory(pDest, pSacl, ulSaclSize);
        pNewSd->Sacl = (ULONG_PTR)pDest - (ULONG_PTR)pNewSd;
        pDest = pDest + ROUND_UP(ulSaclSize, sizeof(ULONG));
    }

    /* Copy the DACL */
    if (pDacl != NULL)
    {
        RtlCopyMemory(pDest, pDacl, ulDaclSize);
        pNewSd->Dacl = (ULONG_PTR)pDest - (ULONG_PTR)pNewSd;
        pDest = pDest + ROUND_UP(ulDaclSize, sizeof(ULONG));
    }

    /* Copy the Owner SID */
    RtlCopyMemory(pDest, pOwnerSid, ulOwnerSidSize);
    pNewSd->Owner = (ULONG_PTR)pDest - (ULONG_PTR)pNewSd;
    pDest = pDest + ROUND_UP(ulOwnerSidSize, sizeof(ULONG));

    /* Copy the Group SID */
    RtlCopyMemory(pDest, pGroupSid, ulGroupSidSize);
    pNewSd->Group = (ULONG_PTR)pDest - (ULONG_PTR)pNewSd;

    /* Free the old security descriptor */
    RtlFreeHeap(RtlGetProcessHeap(), 0, (PVOID)*ObjectsSecurityDescriptor);

    /* Return the new security descriptor */
    *ObjectsSecurityDescriptor = (PSECURITY_DESCRIPTOR)pNewSd;

done:
    if (!NT_SUCCESS(Status))
    {
        if (pNewSd != NULL)
            RtlFreeHeap(RtlGetProcessHeap(), 0, pNewSd);
    }

    return Status;
}

NTSTATUS
NTAPI
RtlpNewSecurityObject(IN PSECURITY_DESCRIPTOR ParentDescriptor,
                      IN PSECURITY_DESCRIPTOR CreatorDescriptor,
                      OUT PSECURITY_DESCRIPTOR *NewDescriptor,
                      IN LPGUID *ObjectTypes,
                      IN ULONG GuidCount,
                      IN BOOLEAN IsDirectoryObject,
                      IN ULONG AutoInheritFlags,
                      IN HANDLE Token,
                      IN PGENERIC_MAPPING GenericMapping)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
RtlpConvertToAutoInheritSecurityObject(IN PSECURITY_DESCRIPTOR ParentDescriptor,
                                       IN PSECURITY_DESCRIPTOR CreatorDescriptor,
                                       OUT PSECURITY_DESCRIPTOR *NewDescriptor,
                                       IN LPGUID ObjectType,
                                       IN BOOLEAN IsDirectoryObject,
                                       IN PGENERIC_MAPPING GenericMapping)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlDefaultNpAcl(OUT PACL *pAcl)
{
    NTSTATUS Status;
    HANDLE TokenHandle;
    PTOKEN_OWNER OwnerSid;
    ULONG ReturnLength = 0;
    ULONG AclSize;
    SID_IDENTIFIER_AUTHORITY NtAuthority    = {SECURITY_NT_AUTHORITY};
    SID_IDENTIFIER_AUTHORITY WorldAuthority = {SECURITY_WORLD_SID_AUTHORITY};

    C_ASSERT(sizeof(ACE) == FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart));

    /*
     * Temporary buffer large enough to hold a maximum of two SIDs.
     * An alternative is to call RtlAllocateAndInitializeSid many times...
     */
    UCHAR SidBuffer[FIELD_OFFSET(SID, SubAuthority)
                    + 2*RTL_FIELD_SIZE(SID, SubAuthority)];
    PSID Sid = (PSID)&SidBuffer;

    ASSERT(RtlLengthRequiredSid(2) == sizeof(SidBuffer));

    /* Initialize the user ACL pointer */
    *pAcl = NULL;

    /*
     * Try to retrieve the SID of the current owner. For that,
     * we first attempt to get the current thread level token.
     */
    Status = NtOpenThreadToken(NtCurrentThread(),
                               TOKEN_QUERY,
                               TRUE,
                               &TokenHandle);
    if (Status == STATUS_NO_TOKEN)
    {
        /*
         * No thread level token, so use the process level token.
         * This is the common case since the only time a thread
         * has a token is when it is impersonating.
         */
        Status = NtOpenProcessToken(NtCurrentProcess(),
                                    TOKEN_QUERY,
                                    &TokenHandle);
    }
    /* Fail if we didn't succeed in retrieving a handle to the token */
    if (!NT_SUCCESS(Status)) return Status;

    /*
     * Retrieve the owner SID from the token.
     */

    /* Query the needed size... */
    Status = NtQueryInformationToken(TokenHandle,
                                     TokenOwner,
                                     NULL, 0,
                                     &ReturnLength);
    /* ... so that we must fail with STATUS_BUFFER_TOO_SMALL error */
    if (Status != STATUS_BUFFER_TOO_SMALL) goto Cleanup1;

    /* Allocate space for the owner SID */
    OwnerSid = RtlAllocateHeap(RtlGetProcessHeap(), 0, ReturnLength);
    if (OwnerSid == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto Cleanup1;
    }

    /* Retrieve the owner SID; we must succeed */
    Status = NtQueryInformationToken(TokenHandle,
                                     TokenOwner,
                                     OwnerSid,
                                     ReturnLength,
                                     &ReturnLength);
    if (!NT_SUCCESS(Status)) goto Cleanup2;

    /*
     * Allocate one ACL with 5 ACEs.
     */
    AclSize = sizeof(ACL) +                     // Header
              5 * sizeof(ACE) +                 // 5 ACEs:
              RtlLengthRequiredSid(1) +         // LocalSystem
              RtlLengthRequiredSid(2) +         // Administrators
              RtlLengthRequiredSid(1) +         // Anonymous
              RtlLengthRequiredSid(1) +         // World
              RtlLengthSid(OwnerSid->Owner);    // Owner

    *pAcl = RtlAllocateHeap(RtlGetProcessHeap(), 0, AclSize);
    if (*pAcl == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto Cleanup2;
    }

    /*
     * Build the ACL and add the five ACEs.
     */
    Status = RtlCreateAcl(*pAcl, AclSize, ACL_REVISION2);
    ASSERT(NT_SUCCESS(Status));

    /* Local System SID - Generic All */
    Status = RtlInitializeSid(Sid, &NtAuthority, 1);
    ASSERT(NT_SUCCESS(Status));
    *RtlSubAuthoritySid(Sid, 0) = SECURITY_LOCAL_SYSTEM_RID;
    Status = RtlAddAccessAllowedAce(*pAcl, ACL_REVISION2, GENERIC_ALL, Sid);
    ASSERT(NT_SUCCESS(Status));

    /* Administrators SID - Generic All */
    Status = RtlInitializeSid(Sid, &NtAuthority, 2);
    ASSERT(NT_SUCCESS(Status));
    *RtlSubAuthoritySid(Sid, 0) = SECURITY_BUILTIN_DOMAIN_RID;
    *RtlSubAuthoritySid(Sid, 1) = DOMAIN_ALIAS_RID_ADMINS;
    Status = RtlAddAccessAllowedAce(*pAcl, ACL_REVISION2, GENERIC_ALL, Sid);
    ASSERT(NT_SUCCESS(Status));

    /* Owner SID - Generic All */
    RtlAddAccessAllowedAce(*pAcl, ACL_REVISION2, GENERIC_ALL, OwnerSid->Owner);
    ASSERT(NT_SUCCESS(Status));

    /* Anonymous SID - Generic Read */
    Status = RtlInitializeSid(Sid, &NtAuthority, 1);
    ASSERT(NT_SUCCESS(Status));
    *RtlSubAuthoritySid(Sid, 0) = SECURITY_ANONYMOUS_LOGON_RID;
    Status = RtlAddAccessAllowedAce(*pAcl, ACL_REVISION2, GENERIC_READ, Sid);
    ASSERT(NT_SUCCESS(Status));

    /* World SID - Generic Read */
    Status = RtlInitializeSid(Sid, &WorldAuthority, 1);
    ASSERT(NT_SUCCESS(Status));
    *RtlSubAuthoritySid(Sid, 0) = SECURITY_WORLD_RID;
    Status = RtlAddAccessAllowedAce(*pAcl, ACL_REVISION2, GENERIC_READ, Sid);
    ASSERT(NT_SUCCESS(Status));

    /* If some problem happened, cleanup everything */
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, *pAcl);
        *pAcl = NULL;
    }

Cleanup2:
    /* Get rid of the owner SID */
    RtlFreeHeap(RtlGetProcessHeap(), 0, OwnerSid);

Cleanup1:
    /* Close the token handle */
    NtClose(TokenHandle);

    /* Done */
    return Status;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
RtlCreateAndSetSD(IN PVOID AceData,
                  IN ULONG AceCount,
                  IN PSID OwnerSid OPTIONAL,
                  IN PSID GroupSid OPTIONAL,
                  OUT PSECURITY_DESCRIPTOR *NewDescriptor)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlDeleteSecurityObject(IN PSECURITY_DESCRIPTOR *ObjectDescriptor)
{
    DPRINT1("RtlDeleteSecurityObject(%p)\n", ObjectDescriptor);

    /* Free the object from the heap */
    RtlFreeHeap(RtlGetProcessHeap(), 0, *ObjectDescriptor);
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlNewSecurityObject(IN PSECURITY_DESCRIPTOR ParentDescriptor,
                     IN PSECURITY_DESCRIPTOR CreatorDescriptor,
                     OUT PSECURITY_DESCRIPTOR *NewDescriptor,
                     IN BOOLEAN IsDirectoryObject,
                     IN HANDLE Token,
                     IN PGENERIC_MAPPING GenericMapping)
{
    DPRINT1("RtlNewSecurityObject(%p)\n", ParentDescriptor);

    /* Call the internal API */
    return RtlpNewSecurityObject(ParentDescriptor,
                                 CreatorDescriptor,
                                 NewDescriptor,
                                 NULL,
                                 0,
                                 IsDirectoryObject,
                                 0,
                                 Token,
                                 GenericMapping);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlNewSecurityObjectEx(IN PSECURITY_DESCRIPTOR ParentDescriptor,
                       IN PSECURITY_DESCRIPTOR CreatorDescriptor,
                       OUT PSECURITY_DESCRIPTOR *NewDescriptor,
                       IN LPGUID ObjectType,
                       IN BOOLEAN IsDirectoryObject,
                       IN ULONG AutoInheritFlags,
                       IN HANDLE Token,
                       IN PGENERIC_MAPPING GenericMapping)
{
    DPRINT1("RtlNewSecurityObjectEx(%p)\n", ParentDescriptor);

    /* Call the internal API */
    return RtlpNewSecurityObject(ParentDescriptor,
                                 CreatorDescriptor,
                                 NewDescriptor,
                                 ObjectType ? &ObjectType : NULL,
                                 ObjectType ? 1 : 0,
                                 IsDirectoryObject,
                                 AutoInheritFlags,
                                 Token,
                                 GenericMapping);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlNewSecurityObjectWithMultipleInheritance(IN PSECURITY_DESCRIPTOR ParentDescriptor,
                                            IN PSECURITY_DESCRIPTOR CreatorDescriptor,
                                            OUT PSECURITY_DESCRIPTOR *NewDescriptor,
                                            IN LPGUID *ObjectTypes,
                                            IN ULONG GuidCount,
                                            IN BOOLEAN IsDirectoryObject,
                                            IN ULONG AutoInheritFlags,
                                            IN HANDLE Token,
                                            IN PGENERIC_MAPPING GenericMapping)
{
    DPRINT1("RtlNewSecurityObjectWithMultipleInheritance(%p)\n", ParentDescriptor);

    /* Call the internal API */
    return RtlpNewSecurityObject(ParentDescriptor,
                                 CreatorDescriptor,
                                 NewDescriptor,
                                 ObjectTypes,
                                 GuidCount,
                                 IsDirectoryObject,
                                 AutoInheritFlags,
                                 Token,
                                 GenericMapping);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlNewInstanceSecurityObject(IN BOOLEAN ParentDescriptorChanged,
                             IN BOOLEAN CreatorDescriptorChanged,
                             IN PLUID OldClientTokenModifiedId,
                             OUT PLUID NewClientTokenModifiedId,
                             IN PSECURITY_DESCRIPTOR ParentDescriptor,
                             IN PSECURITY_DESCRIPTOR CreatorDescriptor,
                             OUT PSECURITY_DESCRIPTOR *NewDescriptor,
                             IN BOOLEAN IsDirectoryObject,
                             IN HANDLE Token,
                             IN PGENERIC_MAPPING GenericMapping)
{
    TOKEN_STATISTICS TokenStats;
    ULONG Size;
    NTSTATUS Status;
    DPRINT1("RtlNewInstanceSecurityObject(%p)\n", ParentDescriptor);

    /* Query the token statistics */
    Status = NtQueryInformationToken(Token,
                                     TokenStatistics,
                                     &TokenStats,
                                     sizeof(TokenStats),
                                     &Size);
    if (!NT_SUCCESS(Status)) return Status;

    /* Return the LUID */
    *NewClientTokenModifiedId = TokenStats.ModifiedId;

    /* Check if the LUID changed */
    if (RtlEqualLuid(NewClientTokenModifiedId, OldClientTokenModifiedId))
    {
        /* Did nothing change? */
        if (!(ParentDescriptorChanged) && !(CreatorDescriptorChanged))
        {
            /* There's no new descriptor, we're done */
            *NewDescriptor = NULL;
            return STATUS_SUCCESS;
        }
    }

    /* Call the standard API */
    return RtlNewSecurityObject(ParentDescriptor,
                                CreatorDescriptor,
                                NewDescriptor,
                                IsDirectoryObject,
                                Token,
                                GenericMapping);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlCreateUserSecurityObject(IN PVOID AceData,
                            IN ULONG AceCount,
                            IN PSID OwnerSid,
                            IN PSID GroupSid,
                            IN BOOLEAN IsDirectoryObject,
                            IN PGENERIC_MAPPING GenericMapping,
                            OUT PSECURITY_DESCRIPTOR *NewDescriptor)
{
    NTSTATUS Status;
    PSECURITY_DESCRIPTOR Sd;
    HANDLE TokenHandle;
    DPRINT1("RtlCreateUserSecurityObject(%p)\n", AceData);

    /* Create the security descriptor based on the ACE Data */
    Status = RtlCreateAndSetSD(AceData,
                               AceCount,
                               OwnerSid,
                               GroupSid,
                               &Sd);
    if (!NT_SUCCESS(Status)) return Status;

    /* Open the process token */
    Status = NtOpenProcessToken(NtCurrentProcess(), TOKEN_QUERY, &TokenHandle);
    if (!NT_SUCCESS(Status)) goto Quickie;

    /* Create the security object */
    Status = RtlNewSecurityObject(NULL,
                                  Sd,
                                  NewDescriptor,
                                  IsDirectoryObject,
                                  TokenHandle,
                                  GenericMapping);

    /* We're done, close the token handle */
    NtClose(TokenHandle);

Quickie:
    /* Free the SD and return status */
    RtlFreeHeap(RtlGetProcessHeap(), 0, Sd);
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlNewSecurityGrantedAccess(IN ACCESS_MASK DesiredAccess,
                            OUT PPRIVILEGE_SET Privileges,
                            IN OUT PULONG Length,
                            IN HANDLE Token,
                            IN PGENERIC_MAPPING GenericMapping,
                            OUT PACCESS_MASK RemainingDesiredAccess)
{
    NTSTATUS Status;
    BOOLEAN Granted, CallerToken;
    TOKEN_STATISTICS TokenStats;
    ULONG Size;
    DPRINT1("RtlNewSecurityGrantedAccess(%lx)\n", DesiredAccess);

    /* Has the caller passed a token? */
    if (!Token)
    {
        /* Remember that we'll have to close the handle */
        CallerToken = FALSE;

        /* Nope, open it */
        Status = NtOpenThreadToken(NtCurrentThread(), TOKEN_QUERY, TRUE, &Token);
        if (!NT_SUCCESS(Status)) return Status;
    }
    else
    {
        /* Yep, use it */
        CallerToken = TRUE;
    }

    /* Get information on the token */
    Status = NtQueryInformationToken(Token,
                                     TokenStatistics,
                                     &TokenStats,
                                     sizeof(TokenStats),
                                     &Size);
    ASSERT(NT_SUCCESS(Status));

    /* Windows doesn't do anything with the token statistics! */

    /* Map the access and return it back decoded */
    RtlMapGenericMask(&DesiredAccess, GenericMapping);
    *RemainingDesiredAccess = DesiredAccess;

    /* Check if one of the rights requested was the SACL right */
    if (DesiredAccess & ACCESS_SYSTEM_SECURITY)
    {
        /* Pretend that it's allowed FIXME: Do privilege check */
        DPRINT1("Missing privilege check for SE_SECURITY_PRIVILEGE");
        Granted = TRUE;
        *RemainingDesiredAccess &= ~ACCESS_SYSTEM_SECURITY;
    }
    else
    {
        /* Nothing to grant */
        Granted = FALSE;
    }

    /* If the caller did not pass in a token, close the handle to ours */
    if (!CallerToken) NtClose(Token);

    /* We need space to return only 1 privilege -- already part of the struct */
    Size = sizeof(PRIVILEGE_SET);
    if (Size > *Length)
    {
        /* Tell the caller how much space we need and fail */
        *Length = Size;
        return STATUS_BUFFER_TOO_SMALL;
    }

    /* Check if the SACL right was granted */
    RtlZeroMemory(Privileges, Size);
    if (Granted)
    {
        /* Yes, return it in the structure */
        Privileges->PrivilegeCount = 1;
        Privileges->Privilege[0].Luid.LowPart = SE_SECURITY_PRIVILEGE;
        Privileges->Privilege[0].Luid.HighPart = 0;
        Privileges->Privilege[0].Attributes = SE_PRIVILEGE_USED_FOR_ACCESS;
    }

    /* All done */
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
RtlQuerySecurityObject(IN PSECURITY_DESCRIPTOR ObjectDescriptor,
                       IN SECURITY_INFORMATION SecurityInformation,
                       OUT PSECURITY_DESCRIPTOR ResultantDescriptor,
                       IN ULONG DescriptorLength,
                       OUT PULONG ReturnLength)
{
    NTSTATUS Status;
    SECURITY_DESCRIPTOR desc;
    BOOLEAN defaulted, present;
    PACL pacl;
    PSID psid;

    Status = RtlCreateSecurityDescriptor(&desc, SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status)) return Status;

    if (SecurityInformation & OWNER_SECURITY_INFORMATION)
    {
        Status = RtlGetOwnerSecurityDescriptor(ObjectDescriptor, &psid, &defaulted);
        if (!NT_SUCCESS(Status)) return Status;
        Status = RtlSetOwnerSecurityDescriptor(&desc, psid, defaulted);
        if (!NT_SUCCESS(Status)) return Status;
    }

    if (SecurityInformation & GROUP_SECURITY_INFORMATION)
    {
        Status = RtlGetGroupSecurityDescriptor(ObjectDescriptor, &psid, &defaulted);
        if (!NT_SUCCESS(Status)) return Status;
        Status = RtlSetGroupSecurityDescriptor(&desc, psid, defaulted);
        if (!NT_SUCCESS(Status)) return Status;
    }

    if (SecurityInformation & DACL_SECURITY_INFORMATION)
    {
        Status = RtlGetDaclSecurityDescriptor(ObjectDescriptor, &present, &pacl, &defaulted);
        if (!NT_SUCCESS(Status)) return Status;
        Status = RtlSetDaclSecurityDescriptor(&desc, present, pacl, defaulted);
        if (!NT_SUCCESS(Status)) return Status;
    }

    if (SecurityInformation & SACL_SECURITY_INFORMATION)
    {
        Status = RtlGetSaclSecurityDescriptor(ObjectDescriptor, &present, &pacl, &defaulted);
        if (!NT_SUCCESS(Status)) return Status;
        Status = RtlSetSaclSecurityDescriptor(&desc, present, pacl, defaulted);
        if (!NT_SUCCESS(Status)) return Status;
    }

    *ReturnLength = DescriptorLength;
    return RtlAbsoluteToSelfRelativeSD(&desc, ResultantDescriptor, ReturnLength);
}


/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlSetSecurityObject(IN SECURITY_INFORMATION SecurityInformation,
                     IN PSECURITY_DESCRIPTOR ModificationDescriptor,
                     IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
                     IN PGENERIC_MAPPING GenericMapping,
                     IN HANDLE Token OPTIONAL)
{
    /* Call the internal API */
    return RtlpSetSecurityObject(NULL,
                                 SecurityInformation,
                                 ModificationDescriptor,
                                 ObjectsSecurityDescriptor,
                                 0,
                                 PagedPool,
                                 GenericMapping,
                                 Token);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlSetSecurityObjectEx(IN SECURITY_INFORMATION SecurityInformation,
                       IN PSECURITY_DESCRIPTOR ModificationDescriptor,
                       IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
                       IN ULONG AutoInheritFlags,
                       IN PGENERIC_MAPPING GenericMapping,
                       IN HANDLE Token OPTIONAL)
{
    /* Call the internal API */
    return RtlpSetSecurityObject(NULL,
                                 SecurityInformation,
                                 ModificationDescriptor,
                                 ObjectsSecurityDescriptor,
                                 AutoInheritFlags,
                                 PagedPool,
                                 GenericMapping,
                                 Token);

}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlConvertToAutoInheritSecurityObject(IN PSECURITY_DESCRIPTOR ParentDescriptor,
                                      IN PSECURITY_DESCRIPTOR CreatorDescriptor,
                                      OUT PSECURITY_DESCRIPTOR *NewDescriptor,
                                      IN LPGUID ObjectType,
                                      IN BOOLEAN IsDirectoryObject,
                                      IN PGENERIC_MAPPING GenericMapping)
{
    /* Call the internal API */
    return RtlpConvertToAutoInheritSecurityObject(ParentDescriptor,
                                                  CreatorDescriptor,
                                                  NewDescriptor,
                                                  ObjectType,
                                                  IsDirectoryObject,
                                                  GenericMapping);
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
RtlRegisterSecureMemoryCacheCallback(IN PRTL_SECURE_MEMORY_CACHE_CALLBACK Callback)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
RtlFlushSecureMemoryCache(IN PVOID MemoryCache,
                          IN OPTIONAL SIZE_T MemoryLength)
{
    UNIMPLEMENTED;
    return FALSE;
}
