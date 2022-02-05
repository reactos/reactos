/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Security Identifier (SID) implementation support and handling
 * COPYRIGHT:       Copyright David Welch <welch@cwcom.net>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

#define SE_MAXIMUM_GROUP_LIMIT 0x1000

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

typedef struct _SID_VALIDATE
{
    UCHAR SubAuthorityCount;
    PISID ProbeSid;
} SID_VALIDATE, *PSID_VALIDATE;

/* FUNCTIONS ******************************************************************/

/**
 * @brief
 * Frees all the known initialized SIDs in the system from the memory.
 *
 * @return
 * Nothing.
 */
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

/**
 * @brief
 * Initializes all the SIDs known in the system.
 *
 * @return
 * Returns TRUE if all the SIDs have been initialized,
 * FALSE otherwise.
 */
CODE_SEG("INIT")
BOOLEAN
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

/**
 * @brief
 * Captures a SID.
 *
 * @param[in] InputSid
 * A valid security identifier to be captured.
 *
 * @param[in] AccessMode
 * Processor level access mode.
 *
 * @param[in] PoolType
 * Pool memory type for the captured SID to assign upon
 * allocation.
 *
 * @param[in] CaptureIfKernel
 * If set to TRUE, the capturing is done within the kernel.
 * Otherwise the capturing is done in a kernel mode driver.
 *
 * @param[out] CapturedSid
 * The captured security identifier, returned to the caller.
 *
 * @return
 * Returns STATUS_SUCCESS if the SID was captured. STATUS_INSUFFICIENT_RESOURCES
 * is returned if memory pool allocation for the captured SID has failed.
 */
NTSTATUS
NTAPI
SepCaptureSid(
    _In_ PSID InputSid,
    _In_ KPROCESSOR_MODE AccessMode,
    _In_ POOL_TYPE PoolType,
    _In_ BOOLEAN CaptureIfKernel,
    _Out_ PSID *CapturedSid)
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

        /* Allocate a SID and copy it */
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

        /* Allocate a SID and copy it */
        NewSid = ExAllocatePoolWithTag(PoolType, SidSize, TAG_SID);
        if (NewSid == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;

        RtlCopyMemory(NewSid, Sid, SidSize);

        *CapturedSid = NewSid;
    }

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Releases a captured SID.
 *
 * @param[in] CapturedSid
 * The captured SID to be released.
 *
 * @param[in] AccessMode
 * Processor level access mode.
 *
 * @param[in] CaptureIfKernel
 * If set to TRUE, the releasing is done within the kernel.
 * Otherwise the releasing is done in a kernel mode driver.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
SepReleaseSid(
    _In_ PSID CapturedSid,
    _In_ KPROCESSOR_MODE AccessMode,
    _In_ BOOLEAN CaptureIfKernel)
{
    PAGED_CODE();

    if (CapturedSid != NULL &&
        (AccessMode != KernelMode ||
         (AccessMode == KernelMode && CaptureIfKernel)))
    {
        ExFreePoolWithTag(CapturedSid, TAG_SID);
    }
}

/**
 * @brief
 * Captures a security identifier from a
 * given access control entry. This identifier
 * is valid for the whole of its lifetime.
 *
 * @param[in] AceType
 * The type of an access control entry. This
 * type that is given by the calling thread
 * must coincide with the actual ACE that is
 * given in the second parameter otherwise this
 * can potentially lead to UNDEFINED behavior!
 *
 * @param[in] Ace
 * A pointer to an access control entry, which
 * can be obtained from a DACL.
 *
 * @return
 * Returns a pointer to a security identifier (SID),
 * otherwise NULL is returned if an unsupported ACE
 * type was given to the function.
 */
PSID
NTAPI
SepGetSidFromAce(
    _In_ UCHAR AceType,
    _In_ PACE Ace)
{
    PSID Sid;
    PAGED_CODE();

    /* Sanity check */
    ASSERT(Ace);

    /* Initialize the SID */
    Sid = NULL;

    /* Obtain the SID based upon ACE type */
    switch (AceType)
    {
        case ACCESS_DENIED_ACE_TYPE:
        {
            Sid = (PSID)&((PACCESS_DENIED_ACE)Ace)->SidStart;
            break;
        }

        case ACCESS_ALLOWED_ACE_TYPE:
        {
            Sid = (PSID)&((PACCESS_ALLOWED_ACE)Ace)->SidStart;
            break;
        }

        case ACCESS_DENIED_OBJECT_ACE_TYPE:
        {
            Sid = (PSID)&((PACCESS_DENIED_OBJECT_ACE)Ace)->SidStart;
            break;
        }

        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
        {
            Sid = (PSID)&((PACCESS_ALLOWED_OBJECT_ACE)Ace)->SidStart;
            break;
        }

        default:
            break;
    }

    return Sid;
}

/**
 * @brief
 * Captures a SID with attributes.
 *
 * @param[in] SrcSidAndAttributes
 * Source of the SID with attributes to be captured.
 *
 * @param[in] AttributeCount
 * The number count of attributes, in total.
 *
 * @param[in] PreviousMode
 * Processor access level mode.
 *
 * @param[in] AllocatedMem
 * The allocated memory buffer for the captured SID. If the caller
 * supplies no allocated block of memory then the function will
 * allocate some buffer block of memory for the captured SID
 * automatically.
 *
 * @param[in] AllocatedLength
 * The length of the buffer that points to the allocated memory,
 * in bytes.
 *
 * @param[in] PoolType
 * The pool type for the captured SID and attributes to assign.
 *
 * @param[in] CaptureIfKernel
 * If set to TRUE, the capturing is done within the kernel.
 * Otherwise the capturing is done in a kernel mode driver.
 *
 * @param[out] CapturedSidAndAttributes
 * The captured SID and attributes.
 *
 * @param[out] ResultLength
 * The length of the captured SID and attributes, in bytes.
 *
 * @return
 * Returns STATUS_SUCCESS if SID and attributes capturing
 * has been completed successfully. STATUS_INVALID_PARAMETER
 * is returned if the count of attributes exceeds the maximum
 * threshold that the kernel can permit, with the purpose of
 * avoiding integer overflows. STATUS_INSUFFICIENT_RESOURCES
 * is returned if memory pool allocation for the captured SID has failed.
 * STATUS_BUFFER_TOO_SMALL is returned if the length of the allocated
 * buffer is less than the required size. STATUS_INVALID_SID is returned
 * if a SID doesn't meet certain requirements to be considered a valid
 * SID by the security manager. A failure NTSTATUS code is returned
 * otherwise.
 *
 * @remarks
 * A security identifier (SID) is a variable-length data structure that
 * can change in size over time, depending on the factors that influence
 * this effect in question. An attacker can take advantage of this fact
 * and can potentially modify certain properties of a SID making it
 * different in size than it was originally before. This is what we'd
 * call, a TOCTOU vulnerability.
 *
 * For this reason, the logic of copying the SIDs and their attributes
 * into a new buffer goes like this: first, allocate a buffer array
 * that just holds the lengths and subauthority count of each SID.
 * Such information is copied in the first loop. Then in a second loop,
 * iterate over the array with SID provided and copy them into the final
 * array. The moment we're doing this, validate the lengths of each SID
 * basing upon the captured lengths we've got before. In this way we
 * ensure that the SIDs have remained intact. The validation checks are
 * done in user mode as a general rule that we just cannot trust UM and
 * whatever data is coming from it.
 */
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
    ULONG TempArrayValidate, TempLengthValidate;
    PSID_AND_ATTRIBUTES SidAndAttributes;
    _SEH2_VOLATILE PSID_VALIDATE ValidateArray;
    PUCHAR CurrentDest;
    PISID Sid;
    NTSTATUS Status;
    PAGED_CODE();

    ValidateArray = NULL;
    SidAndAttributes = NULL;
    *CapturedSidAndAttributes = NULL;
    *ResultLength = 0;

    if (AttributeCount == 0)
    {
        return STATUS_SUCCESS;
    }

    if (AttributeCount > SE_MAXIMUM_GROUP_LIMIT)
    {
        DPRINT1("SeCaptureSidAndAttributesArray(): Maximum group limit exceeded!\n");
        return STATUS_INVALID_PARAMETER;
    }

    if ((PreviousMode == KernelMode) && !CaptureIfKernel)
    {
        *CapturedSidAndAttributes = SrcSidAndAttributes;
        return STATUS_SUCCESS;
    }

    ArraySize = AttributeCount * sizeof(SID_AND_ATTRIBUTES);
    RequiredLength = ALIGN_UP_BY(ArraySize, sizeof(ULONG));

    if (PreviousMode != KernelMode)
    {
        /* Check for user mode data */
        _SEH2_TRY
        {
            /* First probe the whole array */
            ProbeForRead(SrcSidAndAttributes, ArraySize, sizeof(ULONG));

            /* We're in user mode, set up the size for the temporary array */
            TempArrayValidate = AttributeCount * sizeof(SID_VALIDATE);
            TempLengthValidate = ALIGN_UP_BY(TempArrayValidate, sizeof(ULONG));

            /*
             * Allocate a buffer for the array that we're going to
             * temporarily hold the subauthority count and the SID
             * elements. We'll be going to use this array to perform
             * validation checks later.
             */
            ValidateArray = ExAllocatePoolWithTag(PoolType | POOL_RAISE_IF_ALLOCATION_FAILURE,
                                                  TempLengthValidate,
                                                  TAG_SID_VALIDATE);

            /* Loop the array elements */
            for (i = 0; i < AttributeCount; i++)
            {
                /* Get the SID and probe the minimal structure */
                Sid = SrcSidAndAttributes[i].Sid;
                ProbeForRead(Sid, sizeof(*Sid), sizeof(ULONG));

                /*
                 * Capture the subauthority count and hold it
                 * into the temporary array for later validation.
                 * This way we ensure that the said count of each
                 * SID has remained the same.
                 */
                ValidateArray[i].SubAuthorityCount = Sid->SubAuthorityCount;

                /* Capture the SID */
                ValidateArray[i].ProbeSid = Sid;

                /* Calculate the SID length and probe the full SID */
                SidLength = RtlLengthRequiredSid(ValidateArray[i].SubAuthorityCount);
                ProbeForRead(ValidateArray[i].ProbeSid, SidLength, sizeof(ULONG));

                /* Add the aligned length to the required length */
                RequiredLength += ALIGN_UP_BY(SidLength, sizeof(ULONG));
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
            _SEH2_YIELD(goto Cleanup);
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
            DPRINT1("SeCaptureSidAndAttributesArray(): Failed to allocate memory for SID and attributes array (requested size -> %lu)!\n", RequiredLength);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
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
        DPRINT1("SeCaptureSidAndAttributesArray(): The provided buffer is small (expected size -> %lu || current size -> %lu)!\n", RequiredLength, AllocatedLength);
        Status = STATUS_BUFFER_TOO_SMALL;
        goto Cleanup;
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
                /*
                 * Get the SID length from the subauthority
                 * count we've captured before.
                 */
                SidLength = RtlLengthRequiredSid(ValidateArray[i].SubAuthorityCount);

                /* Copy attributes */
                SidAndAttributes[i].Attributes = SrcSidAndAttributes[i].Attributes;

                /* Copy the SID to the current destination address */
                SidAndAttributes[i].Sid = (PSID)CurrentDest;
                RtlCopyMemory(CurrentDest, ValidateArray[i].ProbeSid, SidLength);

                /* Obtain the SID we've captured before for validation */
                Sid = SidAndAttributes[i].Sid;

                /* Validate that the subauthority count hasn't changed */
                if (ValidateArray[i].SubAuthorityCount !=
                    Sid->SubAuthorityCount)
                {
                    /* It's changed, bail out */
                    DPRINT1("SeCaptureSidAndAttributesArray(): The subauthority counts have changed (captured count -> %u || current count -> %u)\n",
                            ValidateArray[i].SubAuthorityCount, Sid->SubAuthorityCount);
                    Status = STATUS_INVALID_SID;
                    goto Cleanup;
                }

                /* Validate that the SID length is the same */
                if (SidLength != RtlLengthSid(Sid))
                {
                    /* They're no longer the same, bail out */
                    DPRINT1("SeCaptureSidAndAttributesArray(): The SID lengths have changed (captured length -> %lu || current length -> %lu)\n",
                            SidLength, RtlLengthSid(Sid));
                    Status = STATUS_INVALID_SID;
                    goto Cleanup;
                }

                /* Check that the SID is valid */
                if (!RtlValidSid(Sid))
                {
                    DPRINT1("SeCaptureSidAndAttributesArray(): The SID is not valid!\n");
                    Status = STATUS_INVALID_SID;
                    goto Cleanup;
                }

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

Cleanup:
    /* Check for failure */
    if (!NT_SUCCESS(Status))
    {
        /* Check if we allocated a new array */
        if ((SidAndAttributes != AllocatedMem) && (SidAndAttributes != NULL))
        {
            /* Free the array */
            ExFreePoolWithTag(SidAndAttributes, TAG_SID_AND_ATTRIBUTES);
        }

        /* Set returned address to NULL */
        *CapturedSidAndAttributes = NULL;
    }

    /* Free the temporary validation array */
    if ((PreviousMode != KernelMode) && (ValidateArray != NULL))
    {
        ExFreePoolWithTag(ValidateArray, TAG_SID_VALIDATE);
    }

    return Status;
}

/**
 * @brief
 * Releases a captured SID with attributes.
 *
 * @param[in] CapturedSidAndAttributes
 * The captured SID with attributes to be released.
 *
 * @param[in] AccessMode
 * Processor access level mode.
 *
 * @param[in] CaptureIfKernel
 * If set to TRUE, the releasing is done within the kernel.
 * Otherwise the releasing is done in a kernel mode driver.
 *
 * @return
 * Nothing.
 */
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
