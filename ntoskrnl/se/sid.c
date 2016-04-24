/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/se/sid.c
 * PURPOSE:         Security manager
 *
 * PROGRAMMERS:     David Welch <welch@cwcom.net>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define TAG_SID_AND_ATTRIBUTES 'aSeS'

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, SepInitSecurityIDs)
#endif

/* GLOBALS ********************************************************************/

SID_IDENTIFIER_AUTHORITY SeNullSidAuthority = {SECURITY_NULL_SID_AUTHORITY};
SID_IDENTIFIER_AUTHORITY SeWorldSidAuthority = {SECURITY_WORLD_SID_AUTHORITY};
SID_IDENTIFIER_AUTHORITY SeLocalSidAuthority = {SECURITY_LOCAL_SID_AUTHORITY};
SID_IDENTIFIER_AUTHORITY SeCreatorSidAuthority = {SECURITY_CREATOR_SID_AUTHORITY};
SID_IDENTIFIER_AUTHORITY SeNtSidAuthority = {SECURITY_NT_AUTHORITY};

PSID SeNullSid = NULL;
PSID SeWorldSid = NULL;
PSID SeLocalSid = NULL;
PSID SeCreatorOwnerSid = NULL;
PSID SeCreatorGroupSid = NULL;
PSID SeCreatorOwnerServerSid = NULL;
PSID SeCreatorGroupServerSid = NULL;
PSID SeNtAuthoritySid = NULL;
PSID SeDialupSid = NULL;
PSID SeNetworkSid = NULL;
PSID SeBatchSid = NULL;
PSID SeInteractiveSid = NULL;
PSID SeServiceSid = NULL;
PSID SePrincipalSelfSid = NULL;
PSID SeLocalSystemSid = NULL;
PSID SeAuthenticatedUserSid = NULL;
PSID SeRestrictedCodeSid = NULL;
PSID SeAliasAdminsSid = NULL;
PSID SeAliasUsersSid = NULL;
PSID SeAliasGuestsSid = NULL;
PSID SeAliasPowerUsersSid = NULL;
PSID SeAliasAccountOpsSid = NULL;
PSID SeAliasSystemOpsSid = NULL;
PSID SeAliasPrintOpsSid = NULL;
PSID SeAliasBackupOpsSid = NULL;
PSID SeAuthenticatedUsersSid = NULL;
PSID SeRestrictedSid = NULL;
PSID SeAnonymousLogonSid = NULL;
PSID SeLocalServiceSid = NULL;
PSID SeNetworkServiceSid = NULL;

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
FreeInitializedSids(VOID)
{
    if (SeNullSid) ExFreePoolWithTag(SeNullSid, TAG_SID);
    if (SeWorldSid) ExFreePoolWithTag(SeWorldSid, TAG_SID);
    if (SeLocalSid) ExFreePoolWithTag(SeLocalSid, TAG_SID);
    if (SeCreatorOwnerSid) ExFreePoolWithTag(SeCreatorOwnerSid, TAG_SID);
    if (SeCreatorGroupSid) ExFreePoolWithTag(SeCreatorGroupSid, TAG_SID);
    if (SeCreatorOwnerServerSid) ExFreePoolWithTag(SeCreatorOwnerServerSid, TAG_SID);
    if (SeCreatorGroupServerSid) ExFreePoolWithTag(SeCreatorGroupServerSid, TAG_SID);
    if (SeNtAuthoritySid) ExFreePoolWithTag(SeNtAuthoritySid, TAG_SID);
    if (SeDialupSid) ExFreePoolWithTag(SeDialupSid, TAG_SID);
    if (SeNetworkSid) ExFreePoolWithTag(SeNetworkSid, TAG_SID);
    if (SeBatchSid) ExFreePoolWithTag(SeBatchSid, TAG_SID);
    if (SeInteractiveSid) ExFreePoolWithTag(SeInteractiveSid, TAG_SID);
    if (SeServiceSid) ExFreePoolWithTag(SeServiceSid, TAG_SID);
    if (SePrincipalSelfSid) ExFreePoolWithTag(SePrincipalSelfSid, TAG_SID);
    if (SeLocalSystemSid) ExFreePoolWithTag(SeLocalSystemSid, TAG_SID);
    if (SeAuthenticatedUserSid) ExFreePoolWithTag(SeAuthenticatedUserSid, TAG_SID);
    if (SeRestrictedCodeSid) ExFreePoolWithTag(SeRestrictedCodeSid, TAG_SID);
    if (SeAliasAdminsSid) ExFreePoolWithTag(SeAliasAdminsSid, TAG_SID);
    if (SeAliasUsersSid) ExFreePoolWithTag(SeAliasUsersSid, TAG_SID);
    if (SeAliasGuestsSid) ExFreePoolWithTag(SeAliasGuestsSid, TAG_SID);
    if (SeAliasPowerUsersSid) ExFreePoolWithTag(SeAliasPowerUsersSid, TAG_SID);
    if (SeAliasAccountOpsSid) ExFreePoolWithTag(SeAliasAccountOpsSid, TAG_SID);
    if (SeAliasSystemOpsSid) ExFreePoolWithTag(SeAliasSystemOpsSid, TAG_SID);
    if (SeAliasPrintOpsSid) ExFreePoolWithTag(SeAliasPrintOpsSid, TAG_SID);
    if (SeAliasBackupOpsSid) ExFreePoolWithTag(SeAliasBackupOpsSid, TAG_SID);
    if (SeAuthenticatedUsersSid) ExFreePoolWithTag(SeAuthenticatedUsersSid, TAG_SID);
    if (SeRestrictedSid) ExFreePoolWithTag(SeRestrictedSid, TAG_SID);
    if (SeAnonymousLogonSid) ExFreePoolWithTag(SeAnonymousLogonSid, TAG_SID);
}

BOOLEAN
INIT_FUNCTION
NTAPI
SepInitSecurityIDs(VOID)
{
    ULONG SidLength0;
    ULONG SidLength1;
    ULONG SidLength2;
    PULONG SubAuthority;

    SidLength0 = RtlLengthRequiredSid(0);
    SidLength1 = RtlLengthRequiredSid(1);
    SidLength2 = RtlLengthRequiredSid(2);

    /* create NullSid */
    SeNullSid = ExAllocatePoolWithTag(PagedPool, SidLength1, TAG_SID);
    SeWorldSid = ExAllocatePoolWithTag(PagedPool, SidLength1, TAG_SID);
    SeLocalSid = ExAllocatePoolWithTag(PagedPool, SidLength1, TAG_SID);
    SeCreatorOwnerSid = ExAllocatePoolWithTag(PagedPool, SidLength1, TAG_SID);
    SeCreatorGroupSid = ExAllocatePoolWithTag(PagedPool, SidLength1, TAG_SID);
    SeCreatorOwnerServerSid = ExAllocatePoolWithTag(PagedPool, SidLength1, TAG_SID);
    SeCreatorGroupServerSid = ExAllocatePoolWithTag(PagedPool, SidLength1, TAG_SID);
    SeNtAuthoritySid = ExAllocatePoolWithTag(PagedPool, SidLength0, TAG_SID);
    SeDialupSid = ExAllocatePoolWithTag(PagedPool, SidLength1, TAG_SID);
    SeNetworkSid = ExAllocatePoolWithTag(PagedPool, SidLength1, TAG_SID);
    SeBatchSid = ExAllocatePoolWithTag(PagedPool, SidLength1, TAG_SID);
    SeInteractiveSid = ExAllocatePoolWithTag(PagedPool, SidLength1, TAG_SID);
    SeServiceSid = ExAllocatePoolWithTag(PagedPool, SidLength1, TAG_SID);
    SePrincipalSelfSid = ExAllocatePoolWithTag(PagedPool, SidLength1, TAG_SID);
    SeLocalSystemSid = ExAllocatePoolWithTag(PagedPool, SidLength1, TAG_SID);
    SeAuthenticatedUserSid = ExAllocatePoolWithTag(PagedPool, SidLength1, TAG_SID);
    SeRestrictedCodeSid = ExAllocatePoolWithTag(PagedPool, SidLength1, TAG_SID);
    SeAliasAdminsSid = ExAllocatePoolWithTag(PagedPool, SidLength2, TAG_SID);
    SeAliasUsersSid = ExAllocatePoolWithTag(PagedPool, SidLength2, TAG_SID);
    SeAliasGuestsSid = ExAllocatePoolWithTag(PagedPool, SidLength2, TAG_SID);
    SeAliasPowerUsersSid = ExAllocatePoolWithTag(PagedPool, SidLength2, TAG_SID);
    SeAliasAccountOpsSid = ExAllocatePoolWithTag(PagedPool, SidLength2, TAG_SID);
    SeAliasSystemOpsSid = ExAllocatePoolWithTag(PagedPool, SidLength2, TAG_SID);
    SeAliasPrintOpsSid = ExAllocatePoolWithTag(PagedPool, SidLength2, TAG_SID);
    SeAliasBackupOpsSid = ExAllocatePoolWithTag(PagedPool, SidLength2, TAG_SID);
    SeAuthenticatedUsersSid = ExAllocatePoolWithTag(PagedPool, SidLength1, TAG_SID);
    SeRestrictedSid = ExAllocatePoolWithTag(PagedPool, SidLength1, TAG_SID);
    SeAnonymousLogonSid = ExAllocatePoolWithTag(PagedPool, SidLength1, TAG_SID);
    SeLocalServiceSid = ExAllocatePoolWithTag(PagedPool, SidLength1, TAG_SID);
    SeNetworkServiceSid = ExAllocatePoolWithTag(PagedPool, SidLength1, TAG_SID);

    if (SeNullSid == NULL || SeWorldSid == NULL ||
        SeLocalSid == NULL || SeCreatorOwnerSid == NULL ||
        SeCreatorGroupSid == NULL || SeCreatorOwnerServerSid == NULL ||
        SeCreatorGroupServerSid == NULL || SeNtAuthoritySid == NULL ||
        SeDialupSid == NULL || SeNetworkSid == NULL || SeBatchSid == NULL ||
        SeInteractiveSid == NULL || SeServiceSid == NULL ||
        SePrincipalSelfSid == NULL || SeLocalSystemSid == NULL ||
        SeAuthenticatedUserSid == NULL || SeRestrictedCodeSid == NULL ||
        SeAliasAdminsSid == NULL || SeAliasUsersSid == NULL ||
        SeAliasGuestsSid == NULL || SeAliasPowerUsersSid == NULL ||
        SeAliasAccountOpsSid == NULL || SeAliasSystemOpsSid == NULL ||
        SeAliasPrintOpsSid == NULL || SeAliasBackupOpsSid == NULL ||
        SeAuthenticatedUsersSid == NULL || SeRestrictedSid == NULL ||
        SeAnonymousLogonSid == NULL || SeLocalServiceSid == NULL ||
        SeNetworkServiceSid == NULL)
    {
        FreeInitializedSids();
        return FALSE;
    }

    RtlInitializeSid(SeNullSid, &SeNullSidAuthority, 1);
    RtlInitializeSid(SeWorldSid, &SeWorldSidAuthority, 1);
    RtlInitializeSid(SeLocalSid, &SeLocalSidAuthority, 1);
    RtlInitializeSid(SeCreatorOwnerSid, &SeCreatorSidAuthority, 1);
    RtlInitializeSid(SeCreatorGroupSid, &SeCreatorSidAuthority, 1);
    RtlInitializeSid(SeCreatorOwnerServerSid, &SeCreatorSidAuthority, 1);
    RtlInitializeSid(SeCreatorGroupServerSid, &SeCreatorSidAuthority, 1);
    RtlInitializeSid(SeNtAuthoritySid, &SeNtSidAuthority, 0);
    RtlInitializeSid(SeDialupSid, &SeNtSidAuthority, 1);
    RtlInitializeSid(SeNetworkSid, &SeNtSidAuthority, 1);
    RtlInitializeSid(SeBatchSid, &SeNtSidAuthority, 1);
    RtlInitializeSid(SeInteractiveSid, &SeNtSidAuthority, 1);
    RtlInitializeSid(SeServiceSid, &SeNtSidAuthority, 1);
    RtlInitializeSid(SePrincipalSelfSid, &SeNtSidAuthority, 1);
    RtlInitializeSid(SeLocalSystemSid, &SeNtSidAuthority, 1);
    RtlInitializeSid(SeAuthenticatedUserSid, &SeNtSidAuthority, 1);
    RtlInitializeSid(SeRestrictedCodeSid, &SeNtSidAuthority, 1);
    RtlInitializeSid(SeAliasAdminsSid, &SeNtSidAuthority, 2);
    RtlInitializeSid(SeAliasUsersSid, &SeNtSidAuthority, 2);
    RtlInitializeSid(SeAliasGuestsSid, &SeNtSidAuthority, 2);
    RtlInitializeSid(SeAliasPowerUsersSid, &SeNtSidAuthority, 2);
    RtlInitializeSid(SeAliasAccountOpsSid, &SeNtSidAuthority, 2);
    RtlInitializeSid(SeAliasSystemOpsSid, &SeNtSidAuthority, 2);
    RtlInitializeSid(SeAliasPrintOpsSid, &SeNtSidAuthority, 2);
    RtlInitializeSid(SeAliasBackupOpsSid, &SeNtSidAuthority, 2);
    RtlInitializeSid(SeAuthenticatedUsersSid, &SeNtSidAuthority, 1);
    RtlInitializeSid(SeRestrictedSid, &SeNtSidAuthority, 1);
    RtlInitializeSid(SeAnonymousLogonSid, &SeNtSidAuthority, 1);
    RtlInitializeSid(SeLocalServiceSid, &SeNtSidAuthority, 1);
    RtlInitializeSid(SeNetworkServiceSid, &SeNtSidAuthority, 1);

    SubAuthority = RtlSubAuthoritySid(SeNullSid, 0);
    *SubAuthority = SECURITY_NULL_RID;
    SubAuthority = RtlSubAuthoritySid(SeWorldSid, 0);
    *SubAuthority = SECURITY_WORLD_RID;
    SubAuthority = RtlSubAuthoritySid(SeLocalSid, 0);
    *SubAuthority = SECURITY_LOCAL_RID;
    SubAuthority = RtlSubAuthoritySid(SeCreatorOwnerSid, 0);
    *SubAuthority = SECURITY_CREATOR_OWNER_RID;
    SubAuthority = RtlSubAuthoritySid(SeCreatorGroupSid, 0);
    *SubAuthority = SECURITY_CREATOR_GROUP_RID;
    SubAuthority = RtlSubAuthoritySid(SeCreatorOwnerServerSid, 0);
    *SubAuthority = SECURITY_CREATOR_OWNER_SERVER_RID;
    SubAuthority = RtlSubAuthoritySid(SeCreatorGroupServerSid, 0);
    *SubAuthority = SECURITY_CREATOR_GROUP_SERVER_RID;
    SubAuthority = RtlSubAuthoritySid(SeDialupSid, 0);
    *SubAuthority = SECURITY_DIALUP_RID;
    SubAuthority = RtlSubAuthoritySid(SeNetworkSid, 0);
    *SubAuthority = SECURITY_NETWORK_RID;
    SubAuthority = RtlSubAuthoritySid(SeBatchSid, 0);
    *SubAuthority = SECURITY_BATCH_RID;
    SubAuthority = RtlSubAuthoritySid(SeInteractiveSid, 0);
    *SubAuthority = SECURITY_INTERACTIVE_RID;
    SubAuthority = RtlSubAuthoritySid(SeServiceSid, 0);
    *SubAuthority = SECURITY_SERVICE_RID;
    SubAuthority = RtlSubAuthoritySid(SePrincipalSelfSid, 0);
    *SubAuthority = SECURITY_PRINCIPAL_SELF_RID;
    SubAuthority = RtlSubAuthoritySid(SeLocalSystemSid, 0);
    *SubAuthority = SECURITY_LOCAL_SYSTEM_RID;
    SubAuthority = RtlSubAuthoritySid(SeAuthenticatedUserSid, 0);
    *SubAuthority = SECURITY_AUTHENTICATED_USER_RID;
    SubAuthority = RtlSubAuthoritySid(SeRestrictedCodeSid, 0);
    *SubAuthority = SECURITY_RESTRICTED_CODE_RID;
    SubAuthority = RtlSubAuthoritySid(SeAliasAdminsSid, 0);
    *SubAuthority = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthority = RtlSubAuthoritySid(SeAliasAdminsSid, 1);
    *SubAuthority = DOMAIN_ALIAS_RID_ADMINS;
    SubAuthority = RtlSubAuthoritySid(SeAliasUsersSid, 0);
    *SubAuthority = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthority = RtlSubAuthoritySid(SeAliasUsersSid, 1);
    *SubAuthority = DOMAIN_ALIAS_RID_USERS;
    SubAuthority = RtlSubAuthoritySid(SeAliasGuestsSid, 0);
    *SubAuthority = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthority = RtlSubAuthoritySid(SeAliasGuestsSid, 1);
    *SubAuthority = DOMAIN_ALIAS_RID_GUESTS;
    SubAuthority = RtlSubAuthoritySid(SeAliasPowerUsersSid, 0);
    *SubAuthority = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthority = RtlSubAuthoritySid(SeAliasPowerUsersSid, 1);
    *SubAuthority = DOMAIN_ALIAS_RID_POWER_USERS;
    SubAuthority = RtlSubAuthoritySid(SeAliasAccountOpsSid, 0);
    *SubAuthority = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthority = RtlSubAuthoritySid(SeAliasAccountOpsSid, 1);
    *SubAuthority = DOMAIN_ALIAS_RID_ACCOUNT_OPS;
    SubAuthority = RtlSubAuthoritySid(SeAliasSystemOpsSid, 0);
    *SubAuthority = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthority = RtlSubAuthoritySid(SeAliasSystemOpsSid, 1);
    *SubAuthority = DOMAIN_ALIAS_RID_SYSTEM_OPS;
    SubAuthority = RtlSubAuthoritySid(SeAliasPrintOpsSid, 0);
    *SubAuthority = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthority = RtlSubAuthoritySid(SeAliasPrintOpsSid, 1);
    *SubAuthority = DOMAIN_ALIAS_RID_PRINT_OPS;
    SubAuthority = RtlSubAuthoritySid(SeAliasBackupOpsSid, 0);
    *SubAuthority = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthority = RtlSubAuthoritySid(SeAliasBackupOpsSid, 1);
    *SubAuthority = DOMAIN_ALIAS_RID_BACKUP_OPS;
    SubAuthority = RtlSubAuthoritySid(SeAuthenticatedUsersSid, 0);
    *SubAuthority = SECURITY_AUTHENTICATED_USER_RID;
    SubAuthority = RtlSubAuthoritySid(SeRestrictedSid, 0);
    *SubAuthority = SECURITY_RESTRICTED_CODE_RID;
    SubAuthority = RtlSubAuthoritySid(SeAnonymousLogonSid, 0);
    *SubAuthority = SECURITY_ANONYMOUS_LOGON_RID;
    SubAuthority = RtlSubAuthoritySid(SeLocalServiceSid, 0);
    *SubAuthority = SECURITY_LOCAL_SERVICE_RID;
    SubAuthority = RtlSubAuthoritySid(SeNetworkServiceSid, 0);
    *SubAuthority = SECURITY_NETWORK_SERVICE_RID;

    return TRUE;
}

NTSTATUS
NTAPI
SepCaptureSid(IN PSID InputSid,
              IN KPROCESSOR_MODE AccessMode,
              IN POOL_TYPE PoolType,
              IN BOOLEAN CaptureIfKernel,
              OUT PSID *CapturedSid)
{
    ULONG SidSize = 0;
    PISID NewSid, Sid = (PISID)InputSid;

    PAGED_CODE();

    if (AccessMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForRead(Sid, FIELD_OFFSET(SID, SubAuthority), sizeof(UCHAR));
            SidSize = RtlLengthRequiredSid(Sid->SubAuthorityCount);
            ProbeForRead(Sid, SidSize, sizeof(UCHAR));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;

        /* allocate a SID and copy it */
        NewSid = ExAllocatePoolWithTag(PoolType, SidSize, TAG_SID);
        if (!NewSid)
            return STATUS_INSUFFICIENT_RESOURCES;

        _SEH2_TRY
        {
            RtlCopyMemory(NewSid, Sid, SidSize);

            *CapturedSid = NewSid;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Free the SID and return the exception code */
            ExFreePoolWithTag(NewSid, TAG_SID);
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else if (!CaptureIfKernel)
    {
        *CapturedSid = InputSid;
    }
    else
    {
        SidSize = RtlLengthRequiredSid(Sid->SubAuthorityCount);

        /* allocate a SID and copy it */
        NewSid = ExAllocatePoolWithTag(PoolType, SidSize, TAG_SID);
        if (NewSid == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;

        RtlCopyMemory(NewSid, Sid, SidSize);

        *CapturedSid = NewSid;
    }

    return STATUS_SUCCESS;
}

VOID
NTAPI
SepReleaseSid(IN PSID CapturedSid,
              IN KPROCESSOR_MODE AccessMode,
              IN BOOLEAN CaptureIfKernel)
{
    PAGED_CODE();

    if (CapturedSid != NULL &&
        (AccessMode != KernelMode ||
         (AccessMode == KernelMode && CaptureIfKernel)))
    {
        ExFreePoolWithTag(CapturedSid, TAG_SID);
    }
}

NTSTATUS
NTAPI
SeCaptureSidAndAttributesArray(
    _In_ PSID_AND_ATTRIBUTES SrcSidAndAttributes,
    _In_ ULONG AttributeCount,
    _In_ KPROCESSOR_MODE PreviousMode,
    _In_opt_ PVOID AllocatedMem,
    _In_ ULONG AllocatedLength,
    _In_ POOL_TYPE PoolType,
    _In_ BOOLEAN CaptureIfKernel,
    _Out_ PSID_AND_ATTRIBUTES *CapturedSidAndAttributes,
    _Out_ PULONG ResultLength)
{
    ULONG ArraySize, RequiredLength, SidLength, i;
    PSID_AND_ATTRIBUTES SidAndAttributes;
    PUCHAR CurrentDest;
    PISID Sid;
    NTSTATUS Status;
    PAGED_CODE();

    *CapturedSidAndAttributes = NULL;
    *ResultLength = 0;

    if (AttributeCount == 0)
    {
        return STATUS_SUCCESS;
    }

    if (AttributeCount > 0x1000)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ((PreviousMode == KernelMode) && !CaptureIfKernel)
    {
        *CapturedSidAndAttributes = SrcSidAndAttributes;
        return STATUS_SUCCESS;
    }

    ArraySize = AttributeCount * sizeof(SID_AND_ATTRIBUTES);
    RequiredLength = ALIGN_UP_BY(ArraySize, sizeof(ULONG));

    /* Check for user mode data */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            /* First probe the whole array */
            ProbeForRead(SrcSidAndAttributes, ArraySize, sizeof(ULONG));

            /* Loop the array elements */
            for (i = 0; i < AttributeCount; i++)
            {
                /* Get the SID and probe the minimal structure */
                Sid = SrcSidAndAttributes[i].Sid;
                ProbeForRead(Sid, sizeof(*Sid), sizeof(ULONG));

                /* Verify that the SID is valid */
                if (((Sid->Revision & 0xF) != SID_REVISION) ||
                    (Sid->SubAuthorityCount > SID_MAX_SUB_AUTHORITIES))
                {
                    return STATUS_INVALID_SID;
                }

                /* Calculate the SID length and probe the full SID */
                SidLength = RtlLengthRequiredSid(Sid->SubAuthorityCount);
                ProbeForRead(Sid, SidLength, sizeof(ULONG));

                /* Add the aligned length to the required length */
                RequiredLength += ALIGN_UP_BY(SidLength, sizeof(ULONG));
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            return _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }
    else
    {
        /* Loop the array elements */
        for (i = 0; i < AttributeCount; i++)
        {
            /* Get the SID and it's length */
            Sid = SrcSidAndAttributes[i].Sid;
            SidLength = RtlLengthRequiredSid(Sid->SubAuthorityCount);

            /* Add the aligned length to the required length */
            RequiredLength += ALIGN_UP_BY(SidLength, sizeof(ULONG));
        }
    }

    /* Assume success */
    Status = STATUS_SUCCESS;
    *ResultLength = RequiredLength;

    /* Check if we have no buffer */
    if (AllocatedMem == NULL)
    {
        /* Allocate a new buffer */
        SidAndAttributes = ExAllocatePoolWithTag(PoolType,
                                                 RequiredLength,
                                                 TAG_SID_AND_ATTRIBUTES);
        if (SidAndAttributes == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    /* Otherwise check if the buffer is large enough */
    else if (AllocatedLength >= RequiredLength)
    {
        /* Buffer is large enough, use it */
        SidAndAttributes = AllocatedMem;
    }
    else
    {
        /* Buffer is too small, fail */
        return STATUS_BUFFER_TOO_SMALL;
    }

    *CapturedSidAndAttributes = SidAndAttributes;

    /* Check again for user mode */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            /* The rest of the data starts after the array */
            CurrentDest = (PUCHAR)SidAndAttributes;
            CurrentDest += ALIGN_UP_BY(ArraySize, sizeof(ULONG));

            /* Loop the array elements */
            for (i = 0; i < AttributeCount; i++)
            {
                /* Get the SID and it's length */
                Sid = SrcSidAndAttributes[i].Sid;
                SidLength = RtlLengthRequiredSid(Sid->SubAuthorityCount);

                /* Copy attributes */
                SidAndAttributes[i].Attributes = SrcSidAndAttributes[i].Attributes;

                /* Copy the SID to the current destination address */
                SidAndAttributes[i].Sid = (PSID)CurrentDest;
                RtlCopyMemory(CurrentDest, SrcSidAndAttributes[i].Sid, SidLength);

                /* Sanity checks */
                ASSERT(RtlLengthSid(SidAndAttributes[i].Sid) == SidLength);
                ASSERT(RtlValidSid(SidAndAttributes[i].Sid));

                /* Update the current destination address */
                CurrentDest += ALIGN_UP_BY(SidLength, sizeof(ULONG));
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }
    else
    {
        /* The rest of the data starts after the array */
        CurrentDest = (PUCHAR)SidAndAttributes;
        CurrentDest += ALIGN_UP_BY(ArraySize, sizeof(ULONG));

        /* Loop the array elements */
        for (i = 0; i < AttributeCount; i++)
        {
            /* Get the SID and it's length */
            Sid = SrcSidAndAttributes[i].Sid;
            SidLength = RtlLengthRequiredSid(Sid->SubAuthorityCount);

            /* Copy attributes */
            SidAndAttributes[i].Attributes = SrcSidAndAttributes[i].Attributes;

            /* Copy the SID to the current destination address */
            SidAndAttributes[i].Sid = (PSID)CurrentDest;
            RtlCopyMemory(CurrentDest, SrcSidAndAttributes[i].Sid, SidLength);

            /* Update the current destination address */
            CurrentDest += ALIGN_UP_BY(SidLength, sizeof(ULONG));
        }
    }

    /* Check for failure */
    if (!NT_SUCCESS(Status))
    {
        /* Check if we allocated a new array */
        if (SidAndAttributes != AllocatedMem)
        {
            /* Free the array */
            ExFreePoolWithTag(SidAndAttributes, TAG_SID_AND_ATTRIBUTES);
        }

        /* Set returned address to NULL */
        *CapturedSidAndAttributes = NULL ;
    }

    return Status;
}

VOID
NTAPI
SeReleaseSidAndAttributesArray(
    _In_ _Post_invalid_ PSID_AND_ATTRIBUTES CapturedSidAndAttributes,
    _In_ KPROCESSOR_MODE AccessMode,
    _In_ BOOLEAN CaptureIfKernel)
{
    PAGED_CODE();

    if ((CapturedSidAndAttributes != NULL) &&
        ((AccessMode != KernelMode) || CaptureIfKernel))
    {
        ExFreePoolWithTag(CapturedSidAndAttributes, TAG_SID_AND_ATTRIBUTES);
    }
}


/* EOF */
