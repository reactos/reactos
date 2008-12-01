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

/* FUNCTIONS ******************************************************************/

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
        SeAnonymousLogonSid == NULL)
    {
        /* FIXME: We're leaking memory here. */
        return(FALSE);
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
    
    return(TRUE);
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
    NTSTATUS Status = STATUS_SUCCESS;
    
    PAGED_CODE();
    
    if(AccessMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForRead(Sid,
                         FIELD_OFFSET(SID,
                                      SubAuthority),
                         sizeof(UCHAR));
            SidSize = RtlLengthRequiredSid(Sid->SubAuthorityCount);
            ProbeForRead(Sid,
                         SidSize,
                         sizeof(UCHAR));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
        
        if(NT_SUCCESS(Status))
        {
            /* allocate a SID and copy it */
            NewSid = ExAllocatePool(PoolType,
                                    SidSize);
            if(NewSid != NULL)
            {
                _SEH2_TRY
                {
                    RtlCopyMemory(NewSid,
                                  Sid,
                                  SidSize);
                    
                    *CapturedSid = NewSid;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    ExFreePool(NewSid);
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;
            }
            else
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }
    else if(!CaptureIfKernel)
    {
        *CapturedSid = InputSid;
        return STATUS_SUCCESS;
    }
    else
    {
        SidSize = RtlLengthRequiredSid(Sid->SubAuthorityCount);
        
        /* allocate a SID and copy it */
        NewSid = ExAllocatePool(PoolType,
                                SidSize);
        if(NewSid != NULL)
        {
            RtlCopyMemory(NewSid,
                          Sid,
                          SidSize);
            
            *CapturedSid = NewSid;
        }
        else
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    
    return Status;
}

VOID
NTAPI
SepReleaseSid(IN PSID CapturedSid,
              IN KPROCESSOR_MODE AccessMode,
              IN BOOLEAN CaptureIfKernel)
{
    PAGED_CODE();
    
    if(CapturedSid != NULL &&
       (AccessMode != KernelMode ||
        (AccessMode == KernelMode && CaptureIfKernel)))
    {
        ExFreePool(CapturedSid);
    }
}

/* EOF */
