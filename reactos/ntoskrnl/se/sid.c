/* $Id: sid.c,v 1.8 2002/02/20 20:15:38 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Security manager
 * FILE:              ntoskrnl/se/sid.c
 * PROGRAMER:         David Welch <welch@cwcom.net>
 * REVISION HISTORY:
 *                 26/07/98: Added stubs for security functions
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/se.h>

#include <internal/debug.h>

#define TAG_SID    TAG('S', 'I', 'D', 'T')


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


BOOLEAN
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
  SeNullSid = ExAllocatePoolWithTag(NonPagedPool,
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
  SeWorldSid = ExAllocatePoolWithTag(NonPagedPool,
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
  SeLocalSid = ExAllocatePoolWithTag(NonPagedPool,
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
  SeCreatorOwnerSid = ExAllocatePoolWithTag(NonPagedPool,
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
  SeCreatorGroupSid = ExAllocatePoolWithTag(NonPagedPool,
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
  SeCreatorOwnerServerSid = ExAllocatePoolWithTag(NonPagedPool,
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
  SeCreatorGroupServerSid = ExAllocatePoolWithTag(NonPagedPool,
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
  SeNtAuthoritySid = ExAllocatePoolWithTag(NonPagedPool,
					   SidLength0,
					   TAG_SID);
  if (SeNtAuthoritySid == NULL)
    return(FALSE);

  RtlInitializeSid(SeNtAuthoritySid,
		   &SeNtSidAuthority,
		   0);

  /* create DialupSid */
  SeDialupSid = ExAllocatePoolWithTag(NonPagedPool,
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
  SeNetworkSid = ExAllocatePoolWithTag(NonPagedPool,
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
  SeBatchSid = ExAllocatePoolWithTag(NonPagedPool,
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
  SeInteractiveSid = ExAllocatePoolWithTag(NonPagedPool,
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
  SeServiceSid = ExAllocatePoolWithTag(NonPagedPool,
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
  SeAnonymousLogonSid = ExAllocatePoolWithTag(NonPagedPool,
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
  SePrincipalSelfSid = ExAllocatePoolWithTag(NonPagedPool,
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
  SeLocalSystemSid = ExAllocatePoolWithTag(NonPagedPool,
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
  SeAuthenticatedUserSid = ExAllocatePoolWithTag(NonPagedPool,
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
  SeRestrictedCodeSid = ExAllocatePoolWithTag(NonPagedPool,
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
  SeAliasAdminsSid = ExAllocatePoolWithTag(NonPagedPool,
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
  SeAliasUsersSid = ExAllocatePoolWithTag(NonPagedPool,
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
  SeAliasGuestsSid = ExAllocatePoolWithTag(NonPagedPool,
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
  SeAliasPowerUsersSid = ExAllocatePoolWithTag(NonPagedPool,
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
  SeAliasAccountOpsSid = ExAllocatePoolWithTag(NonPagedPool,
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
  SeAliasSystemOpsSid = ExAllocatePoolWithTag(NonPagedPool,
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
  SeAliasPrintOpsSid = ExAllocatePoolWithTag(NonPagedPool,
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
  SeAliasBackupOpsSid = ExAllocatePoolWithTag(NonPagedPool,
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


BOOLEAN STDCALL
RtlValidSid(PSID Sid)
{
   if ((Sid->Revision & 0xf) != 1)
     {
	return(FALSE);
     }
   if (Sid->SubAuthorityCount > 15)
     {
	return(FALSE);
     }
   return(TRUE);
}


ULONG STDCALL
RtlLengthRequiredSid(UCHAR SubAuthorityCount)
{
  return(sizeof(SID) + (SubAuthorityCount - 1) * sizeof(ULONG));
}


NTSTATUS STDCALL
RtlInitializeSid(PSID Sid,
		 PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
		 UCHAR SubAuthorityCount)
{
  Sid->Revision = 1;
  Sid->SubAuthorityCount = SubAuthorityCount;
  RtlCopyMemory(&Sid->IdentifierAuthority,
		IdentifierAuthority,
		sizeof(SID_IDENTIFIER_AUTHORITY));
  return(STATUS_SUCCESS);
}


PULONG STDCALL
RtlSubAuthoritySid(PSID Sid,
		   ULONG SubAuthority)
{
  return(&Sid->SubAuthority[SubAuthority]);
}


PUCHAR STDCALL
RtlSubAuthorityCountSid (PSID Sid)
{
  return(&Sid->SubAuthorityCount);
}


BOOLEAN STDCALL
RtlEqualSid(PSID Sid1,
	    PSID Sid2)
{
   if (Sid1->Revision != Sid2->Revision)
     {
	return(FALSE);
     }
   if ((*RtlSubAuthorityCountSid(Sid1)) !=
       (*RtlSubAuthorityCountSid(Sid2)))
     {
	return(FALSE);
     }
   if (memcmp(Sid1, Sid2, RtlLengthSid(Sid1) != 0))
     {
	return(FALSE);
     }
   return(TRUE);
}


ULONG STDCALL
RtlLengthSid(PSID Sid)
{
  return(sizeof(SID) + (Sid->SubAuthorityCount-1)*4);
}


NTSTATUS STDCALL
RtlCopySid(ULONG BufferLength,
	   PSID Dest,
	   PSID Src)
{
   if (BufferLength < RtlLengthSid(Src))
     {
	return(STATUS_UNSUCCESSFUL);
     }
   memmove(Dest, Src, RtlLengthSid(Src));
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
RtlConvertSidToUnicodeString(PUNICODE_STRING String,
			     PSID Sid,
			     BOOLEAN AllocateString)
{
   WCHAR Buffer[256];
   PWSTR Ptr;
   ULONG Length;
   ULONG i;

   if (!RtlValidSid(Sid))
     return STATUS_INVALID_SID;

   Ptr = Buffer;
   Ptr += swprintf (Ptr,
		    L"S-%u-",
		    Sid->Revision);

   if(!Sid->IdentifierAuthority.Value[0] &&
      !Sid->IdentifierAuthority.Value[1])
      {
	Ptr += swprintf(Ptr,
			L"%u",
			(ULONG)Sid->IdentifierAuthority.Value[2] << 24 |
			(ULONG)Sid->IdentifierAuthority.Value[3] << 16 |
			(ULONG)Sid->IdentifierAuthority.Value[4] << 8 |
			(ULONG)Sid->IdentifierAuthority.Value[5]);
     }
   else
     {
	Ptr += swprintf(Ptr,
			L"0x%02hx%02hx%02hx%02hx%02hx%02hx",
			Sid->IdentifierAuthority.Value[0],
			Sid->IdentifierAuthority.Value[1],
			Sid->IdentifierAuthority.Value[2],
			Sid->IdentifierAuthority.Value[3],
			Sid->IdentifierAuthority.Value[4],
			Sid->IdentifierAuthority.Value[5]);
     }

   for (i = 0; i < Sid->SubAuthorityCount; i++)
     {
	Ptr += swprintf(Ptr,
			L"-%u",
			Sid->SubAuthority[i]);
     }

   Length = (Ptr - Buffer) * sizeof(WCHAR);

   if (AllocateString)
     {
	String->Buffer = ExAllocatePool(NonPagedPool,
					Length + sizeof(WCHAR));
	if (String->Buffer == NULL)
	  return STATUS_NO_MEMORY;

	String->MaximumLength = Length + sizeof(WCHAR);
     }
   else
     {
	if (Length > String->MaximumLength)
	  return STATUS_BUFFER_TOO_SMALL;
     }
   String->Length = Length;
   memmove(String->Buffer,
	   Buffer,
	   Length);
   if (Length < String->MaximumLength)
     String->Buffer[Length] = 0;

   return STATUS_SUCCESS;
}

/* EOF */
