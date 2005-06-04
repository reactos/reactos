/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/se/sid.c
 * PURPOSE:         Security manager
 *
 * PROGRAMMERS:     David Welch <welch@cwcom.net>
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

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
PSID SeAnonymousLogonSid = NULL;
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


/* FUNCTIONS ****************************************************************/


BOOLEAN INIT_FUNCTION
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
  SeNullSid = ExAllocatePoolWithTag(PagedPool,
				    SidLength1,
				    TAG_SID);
  if (SeNullSid == NULL)
    return(FALSE);

  RtlInitializeSid(SeNullSid,
		   &SeNullSidAuthority,
		   1);
  SubAuthority = RtlSubAuthoritySid(SeNullSid,
				    0);
  *SubAuthority = SECURITY_NULL_RID;

  /* create WorldSid */
  SeWorldSid = ExAllocatePoolWithTag(PagedPool,
				     SidLength1,
				     TAG_SID);
  if (SeWorldSid == NULL)
    return(FALSE);

  RtlInitializeSid(SeWorldSid,
		   &SeWorldSidAuthority,
		   1);
  SubAuthority = RtlSubAuthoritySid(SeWorldSid,
				    0);
  *SubAuthority = SECURITY_WORLD_RID;

  /* create LocalSid */
  SeLocalSid = ExAllocatePoolWithTag(PagedPool,
				     SidLength1,
				     TAG_SID);
  if (SeLocalSid == NULL)
    return(FALSE);

  RtlInitializeSid(SeLocalSid,
		   &SeLocalSidAuthority,
		   1);
  SubAuthority = RtlSubAuthoritySid(SeLocalSid,
				    0);
  *SubAuthority = SECURITY_LOCAL_RID;

  /* create CreatorOwnerSid */
  SeCreatorOwnerSid = ExAllocatePoolWithTag(PagedPool,
					    SidLength1,
					    TAG_SID);
  if (SeCreatorOwnerSid == NULL)
    return(FALSE);

  RtlInitializeSid(SeCreatorOwnerSid,
		   &SeCreatorSidAuthority,
		   1);
  SubAuthority = RtlSubAuthoritySid(SeCreatorOwnerSid,
				    0);
  *SubAuthority = SECURITY_CREATOR_OWNER_RID;

  /* create CreatorGroupSid */
  SeCreatorGroupSid = ExAllocatePoolWithTag(PagedPool,
					    SidLength1,
					    TAG_SID);
  if (SeCreatorGroupSid == NULL)
    return(FALSE);

  RtlInitializeSid(SeCreatorGroupSid,
		   &SeCreatorSidAuthority,
		   1);
  SubAuthority = RtlSubAuthoritySid(SeCreatorGroupSid,
				    0);
  *SubAuthority = SECURITY_CREATOR_GROUP_RID;

  /* create CreatorOwnerServerSid */
  SeCreatorOwnerServerSid = ExAllocatePoolWithTag(PagedPool,
						  SidLength1,
						  TAG_SID);
  if (SeCreatorOwnerServerSid == NULL)
    return(FALSE);

  RtlInitializeSid(SeCreatorOwnerServerSid,
		   &SeCreatorSidAuthority,
		   1);
  SubAuthority = RtlSubAuthoritySid(SeCreatorOwnerServerSid,
				    0);
  *SubAuthority = SECURITY_CREATOR_OWNER_SERVER_RID;

  /* create CreatorGroupServerSid */
  SeCreatorGroupServerSid = ExAllocatePoolWithTag(PagedPool,
						  SidLength1,
						  TAG_SID);
  if (SeCreatorGroupServerSid == NULL)
    return(FALSE);

  RtlInitializeSid(SeCreatorGroupServerSid,
		   &SeCreatorSidAuthority,
		   1);
  SubAuthority = RtlSubAuthoritySid(SeCreatorGroupServerSid,
				    0);
  *SubAuthority = SECURITY_CREATOR_GROUP_SERVER_RID;


  /* create NtAuthoritySid */
  SeNtAuthoritySid = ExAllocatePoolWithTag(PagedPool,
					   SidLength0,
					   TAG_SID);
  if (SeNtAuthoritySid == NULL)
    return(FALSE);

  RtlInitializeSid(SeNtAuthoritySid,
		   &SeNtSidAuthority,
		   0);

  /* create DialupSid */
  SeDialupSid = ExAllocatePoolWithTag(PagedPool,
				      SidLength1,
				      TAG_SID);
  if (SeDialupSid == NULL)
    return(FALSE);

  RtlInitializeSid(SeDialupSid,
		   &SeNtSidAuthority,
		   1);
  SubAuthority = RtlSubAuthoritySid(SeDialupSid,
				    0);
  *SubAuthority = SECURITY_DIALUP_RID;

  /* create NetworkSid */
  SeNetworkSid = ExAllocatePoolWithTag(PagedPool,
				       SidLength1,
				       TAG_SID);
  if (SeNetworkSid == NULL)
    return(FALSE);

  RtlInitializeSid(SeNetworkSid,
		   &SeNtSidAuthority,
		   1);
  SubAuthority = RtlSubAuthoritySid(SeNetworkSid,
				    0);
  *SubAuthority = SECURITY_NETWORK_RID;

  /* create BatchSid */
  SeBatchSid = ExAllocatePoolWithTag(PagedPool,
				     SidLength1,
				     TAG_SID);
  if (SeBatchSid == NULL)
    return(FALSE);

  RtlInitializeSid(SeBatchSid,
		   &SeNtSidAuthority,
		   1);
  SubAuthority = RtlSubAuthoritySid(SeBatchSid,
				    0);
  *SubAuthority = SECURITY_BATCH_RID;

  /* create InteractiveSid */
  SeInteractiveSid = ExAllocatePoolWithTag(PagedPool,
					   SidLength1,
					   TAG_SID);
  if (SeInteractiveSid == NULL)
    return(FALSE);

  RtlInitializeSid(SeInteractiveSid,
		   &SeNtSidAuthority,
		   1);
  SubAuthority = RtlSubAuthoritySid(SeInteractiveSid,
				    0);
  *SubAuthority = SECURITY_INTERACTIVE_RID;

  /* create ServiceSid */
  SeServiceSid = ExAllocatePoolWithTag(PagedPool,
				       SidLength1,
				       TAG_SID);
  if (SeServiceSid == NULL)
    return(FALSE);

  RtlInitializeSid(SeServiceSid,
		   &SeNtSidAuthority,
		   1);
  SubAuthority = RtlSubAuthoritySid(SeServiceSid,
				    0);
  *SubAuthority = SECURITY_SERVICE_RID;

  /* create AnonymousLogonSid */
  SeAnonymousLogonSid = ExAllocatePoolWithTag(PagedPool,
					      SidLength1,
					      TAG_SID);
  if (SeAnonymousLogonSid == NULL)
    return(FALSE);

  RtlInitializeSid(SeAnonymousLogonSid,
		   &SeNtSidAuthority,
		   1);
  SubAuthority = RtlSubAuthoritySid(SeAnonymousLogonSid,
				    0);
  *SubAuthority = SECURITY_ANONYMOUS_LOGON_RID;

  /* create PrincipalSelfSid */
  SePrincipalSelfSid = ExAllocatePoolWithTag(PagedPool,
					     SidLength1,
					     TAG_SID);
  if (SePrincipalSelfSid == NULL)
    return(FALSE);

  RtlInitializeSid(SePrincipalSelfSid,
		   &SeNtSidAuthority,
		   1);
  SubAuthority = RtlSubAuthoritySid(SePrincipalSelfSid,
				    0);
  *SubAuthority = SECURITY_PRINCIPAL_SELF_RID;

  /* create LocalSystemSid */
  SeLocalSystemSid = ExAllocatePoolWithTag(PagedPool,
					   SidLength1,
					   TAG_SID);
  if (SeLocalSystemSid == NULL)
    return(FALSE);

  RtlInitializeSid(SeLocalSystemSid,
		   &SeNtSidAuthority,
		   1);
  SubAuthority = RtlSubAuthoritySid(SeLocalSystemSid,
				    0);
  *SubAuthority = SECURITY_LOCAL_SYSTEM_RID;

  /* create AuthenticatedUserSid */
  SeAuthenticatedUserSid = ExAllocatePoolWithTag(PagedPool,
						 SidLength1,
						 TAG_SID);
  if (SeAuthenticatedUserSid == NULL)
    return(FALSE);

  RtlInitializeSid(SeAuthenticatedUserSid,
		   &SeNtSidAuthority,
		   1);
  SubAuthority = RtlSubAuthoritySid(SeAuthenticatedUserSid,
				    0);
  *SubAuthority = SECURITY_AUTHENTICATED_USER_RID;

  /* create RestrictedCodeSid */
  SeRestrictedCodeSid = ExAllocatePoolWithTag(PagedPool,
					      SidLength1,
					      TAG_SID);
  if (SeRestrictedCodeSid == NULL)
    return(FALSE);

  RtlInitializeSid(SeRestrictedCodeSid,
		   &SeNtSidAuthority,
		   1);
  SubAuthority = RtlSubAuthoritySid(SeRestrictedCodeSid,
				    0);
  *SubAuthority = SECURITY_RESTRICTED_CODE_RID;

  /* create AliasAdminsSid */
  SeAliasAdminsSid = ExAllocatePoolWithTag(PagedPool,
					   SidLength2,
					   TAG_SID);
  if (SeAliasAdminsSid == NULL)
    return(FALSE);

  RtlInitializeSid(SeAliasAdminsSid,
		   &SeNtSidAuthority,
		   2);
  SubAuthority = RtlSubAuthoritySid(SeAliasAdminsSid,
				    0);
  *SubAuthority = SECURITY_BUILTIN_DOMAIN_RID;

  SubAuthority = RtlSubAuthoritySid(SeAliasAdminsSid,
				    1);
  *SubAuthority = DOMAIN_ALIAS_RID_ADMINS;

  /* create AliasUsersSid */
  SeAliasUsersSid = ExAllocatePoolWithTag(PagedPool,
					  SidLength2,
					  TAG_SID);
  if (SeAliasUsersSid == NULL)
    return(FALSE);

  RtlInitializeSid(SeAliasUsersSid,
		   &SeNtSidAuthority,
		   2);
  SubAuthority = RtlSubAuthoritySid(SeAliasUsersSid,
				    0);
  *SubAuthority = SECURITY_BUILTIN_DOMAIN_RID;

  SubAuthority = RtlSubAuthoritySid(SeAliasUsersSid,
				    1);
  *SubAuthority = DOMAIN_ALIAS_RID_USERS;

  /* create AliasGuestsSid */
  SeAliasGuestsSid = ExAllocatePoolWithTag(PagedPool,
					   SidLength2,
					   TAG_SID);
  if (SeAliasGuestsSid == NULL)
    return(FALSE);

  RtlInitializeSid(SeAliasGuestsSid,
		   &SeNtSidAuthority,
		   2);
  SubAuthority = RtlSubAuthoritySid(SeAliasGuestsSid,
				    0);
  *SubAuthority = SECURITY_BUILTIN_DOMAIN_RID;

  SubAuthority = RtlSubAuthoritySid(SeAliasGuestsSid,
				    1);
  *SubAuthority = DOMAIN_ALIAS_RID_GUESTS;

  /* create AliasPowerUsersSid */
  SeAliasPowerUsersSid = ExAllocatePoolWithTag(PagedPool,
					       SidLength2,
					       TAG_SID);
  if (SeAliasPowerUsersSid == NULL)
    return(FALSE);

  RtlInitializeSid(SeAliasPowerUsersSid,
		   &SeNtSidAuthority,
		   2);
  SubAuthority = RtlSubAuthoritySid(SeAliasPowerUsersSid,
				    0);
  *SubAuthority = SECURITY_BUILTIN_DOMAIN_RID;

  SubAuthority = RtlSubAuthoritySid(SeAliasPowerUsersSid,
				    1);
  *SubAuthority = DOMAIN_ALIAS_RID_POWER_USERS;

  /* create AliasAccountOpsSid */
  SeAliasAccountOpsSid = ExAllocatePoolWithTag(PagedPool,
					       SidLength2,
					       TAG_SID);
  if (SeAliasAccountOpsSid == NULL)
    return(FALSE);

  RtlInitializeSid(SeAliasAccountOpsSid,
		   &SeNtSidAuthority,
		   2);
  SubAuthority = RtlSubAuthoritySid(SeAliasAccountOpsSid,
				    0);
  *SubAuthority = SECURITY_BUILTIN_DOMAIN_RID;

  SubAuthority = RtlSubAuthoritySid(SeAliasAccountOpsSid,
				    1);
  *SubAuthority = DOMAIN_ALIAS_RID_ACCOUNT_OPS;

  /* create AliasSystemOpsSid */
  SeAliasSystemOpsSid = ExAllocatePoolWithTag(PagedPool,
					      SidLength2,
					      TAG_SID);
  if (SeAliasSystemOpsSid == NULL)
    return(FALSE);

  RtlInitializeSid(SeAliasSystemOpsSid,
		   &SeNtSidAuthority,
		   2);
  SubAuthority = RtlSubAuthoritySid(SeAliasSystemOpsSid,
				    0);
  *SubAuthority = SECURITY_BUILTIN_DOMAIN_RID;

  SubAuthority = RtlSubAuthoritySid(SeAliasSystemOpsSid,
				    1);
  *SubAuthority = DOMAIN_ALIAS_RID_SYSTEM_OPS;

  /* create AliasPrintOpsSid */
  SeAliasPrintOpsSid = ExAllocatePoolWithTag(PagedPool,
					     SidLength2,
					     TAG_SID);
  if (SeAliasPrintOpsSid == NULL)
    return(FALSE);

  RtlInitializeSid(SeAliasPrintOpsSid,
		   &SeNtSidAuthority,
		   2);
  SubAuthority = RtlSubAuthoritySid(SeAliasPrintOpsSid,
				    0);
  *SubAuthority = SECURITY_BUILTIN_DOMAIN_RID;

  SubAuthority = RtlSubAuthoritySid(SeAliasPrintOpsSid,
				    1);
  *SubAuthority = DOMAIN_ALIAS_RID_PRINT_OPS;

  /* create AliasBackupOpsSid */
  SeAliasBackupOpsSid = ExAllocatePoolWithTag(PagedPool,
					      SidLength2,
					      TAG_SID);
  if (SeAliasBackupOpsSid == NULL)
    return(FALSE);

  RtlInitializeSid(SeAliasBackupOpsSid,
		   &SeNtSidAuthority,
		   2);
  SubAuthority = RtlSubAuthoritySid(SeAliasBackupOpsSid,
				    0);
  *SubAuthority = SECURITY_BUILTIN_DOMAIN_RID;

  SubAuthority = RtlSubAuthoritySid(SeAliasBackupOpsSid,
				    1);
  *SubAuthority = DOMAIN_ALIAS_RID_BACKUP_OPS;

  return(TRUE);
}

NTSTATUS
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
    _SEH_TRY
    {
      ProbeForRead(Sid,
                   sizeof(*Sid) - sizeof(Sid->SubAuthority),
                   sizeof(UCHAR));
      SidSize = RtlLengthRequiredSid(Sid->SubAuthorityCount);
      ProbeForRead(Sid,
                   SidSize,
                   sizeof(UCHAR));
    }
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    if(NT_SUCCESS(Status))
    {
      /* allocate a SID and copy it */
      NewSid = ExAllocatePool(PoolType,
                              SidSize);
      if(NewSid != NULL)
      {
        _SEH_TRY
        {
          RtlCopyMemory(NewSid,
                        Sid,
                        SidSize);

          *CapturedSid = NewSid;
        }
        _SEH_HANDLE
        {
          ExFreePool(NewSid);
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
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
SepReleaseSid(IN PSID CapturedSid,
              IN KPROCESSOR_MODE AccessMode,
              IN BOOLEAN CaptureIfKernel)
{
  PAGED_CODE();

  if(CapturedSid != NULL &&
     (AccessMode == UserMode ||
      (AccessMode == KernelMode && CaptureIfKernel)))
  {
    ExFreePool(CapturedSid);
  }
}

/* EOF */
