/* $Id$
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Security manager
 * FILE:              kernel/se/sd.c
 * PROGRAMER:         David Welch <welch@cwcom.net>
 * REVISION HISTORY:
 *                 26/07/98: Added stubs for security functions
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

PSECURITY_DESCRIPTOR SePublicDefaultSd = NULL;
PSECURITY_DESCRIPTOR SePublicDefaultUnrestrictedSd = NULL;
PSECURITY_DESCRIPTOR SePublicOpenSd = NULL;
PSECURITY_DESCRIPTOR SePublicOpenUnrestrictedSd = NULL;
PSECURITY_DESCRIPTOR SeSystemDefaultSd = NULL;
PSECURITY_DESCRIPTOR SeUnrestrictedSd = NULL;

/* FUNCTIONS ***************************************************************/

BOOLEAN INIT_FUNCTION
SepInitSDs(VOID)
{
  /* Create PublicDefaultSd */
  SePublicDefaultSd = ExAllocatePool(NonPagedPool,
				     sizeof(SECURITY_DESCRIPTOR));
  if (SePublicDefaultSd == NULL)
    return FALSE;

  RtlCreateSecurityDescriptor(SePublicDefaultSd,
			      SECURITY_DESCRIPTOR_REVISION);
  RtlSetDaclSecurityDescriptor(SePublicDefaultSd,
			       TRUE,
			       SePublicDefaultDacl,
			       FALSE);

  /* Create PublicDefaultUnrestrictedSd */
  SePublicDefaultUnrestrictedSd = ExAllocatePool(NonPagedPool,
						 sizeof(SECURITY_DESCRIPTOR));
  if (SePublicDefaultUnrestrictedSd == NULL)
    return FALSE;

  RtlCreateSecurityDescriptor(SePublicDefaultUnrestrictedSd,
			      SECURITY_DESCRIPTOR_REVISION);
  RtlSetDaclSecurityDescriptor(SePublicDefaultUnrestrictedSd,
			       TRUE,
			       SePublicDefaultUnrestrictedDacl,
			       FALSE);

  /* Create PublicOpenSd */
  SePublicOpenSd = ExAllocatePool(NonPagedPool,
				  sizeof(SECURITY_DESCRIPTOR));
  if (SePublicOpenSd == NULL)
    return FALSE;

  RtlCreateSecurityDescriptor(SePublicOpenSd,
			      SECURITY_DESCRIPTOR_REVISION);
  RtlSetDaclSecurityDescriptor(SePublicOpenSd,
			       TRUE,
			       SePublicOpenDacl,
			       FALSE);

  /* Create PublicOpenUnrestrictedSd */
  SePublicOpenUnrestrictedSd = ExAllocatePool(NonPagedPool,
					      sizeof(SECURITY_DESCRIPTOR));
  if (SePublicOpenUnrestrictedSd == NULL)
    return FALSE;

  RtlCreateSecurityDescriptor(SePublicOpenUnrestrictedSd,
			      SECURITY_DESCRIPTOR_REVISION);
  RtlSetDaclSecurityDescriptor(SePublicOpenUnrestrictedSd,
			       TRUE,
			       SePublicOpenUnrestrictedDacl,
			       FALSE);

  /* Create SystemDefaultSd */
  SeSystemDefaultSd = ExAllocatePool(NonPagedPool,
				     sizeof(SECURITY_DESCRIPTOR));
  if (SeSystemDefaultSd == NULL)
    return FALSE;

  RtlCreateSecurityDescriptor(SeSystemDefaultSd,
			      SECURITY_DESCRIPTOR_REVISION);
  RtlSetDaclSecurityDescriptor(SeSystemDefaultSd,
			       TRUE,
			       SeSystemDefaultDacl,
			       FALSE);

  /* Create UnrestrictedSd */
  SeUnrestrictedSd = ExAllocatePool(NonPagedPool,
				    sizeof(SECURITY_DESCRIPTOR));
  if (SeUnrestrictedSd == NULL)
    return FALSE;

  RtlCreateSecurityDescriptor(SeUnrestrictedSd,
			      SECURITY_DESCRIPTOR_REVISION);
  RtlSetDaclSecurityDescriptor(SeUnrestrictedSd,
			       TRUE,
			       SeUnrestrictedDacl,
			       FALSE);

  return TRUE;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
SeCaptureSecurityDescriptor(
	IN PSECURITY_DESCRIPTOR OriginalSecurityDescriptor,
	IN KPROCESSOR_MODE CurrentMode,
	IN POOL_TYPE PoolType,
	IN BOOLEAN CaptureIfKernel,
	OUT PSECURITY_DESCRIPTOR *CapturedSecurityDescriptor
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS STDCALL
SeQuerySecurityDescriptorInfo(IN PSECURITY_INFORMATION SecurityInformation,
			      IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
			      IN OUT PULONG Length,
			      IN PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor OPTIONAL)
{
  PSECURITY_DESCRIPTOR ObjectSd;
  PSID Owner = 0;
  PSID Group = 0;
  PACL Dacl = 0;
  PACL Sacl = 0;
  ULONG OwnerLength = 0;
  ULONG GroupLength = 0;
  ULONG DaclLength = 0;
  ULONG SaclLength = 0;
  ULONG Control = 0;
  ULONG_PTR Current;
  ULONG SdLength;

  if (*ObjectsSecurityDescriptor == NULL)
    {
      if (*Length < sizeof(SECURITY_DESCRIPTOR))
	{
	  *Length = sizeof(SECURITY_DESCRIPTOR);
	  return STATUS_BUFFER_TOO_SMALL;
	}

      *Length = sizeof(SECURITY_DESCRIPTOR);
      RtlCreateSecurityDescriptor(SecurityDescriptor,
				  SECURITY_DESCRIPTOR_REVISION);
      SecurityDescriptor->Control |= SE_SELF_RELATIVE;
      return STATUS_SUCCESS;
    }

  ObjectSd = *ObjectsSecurityDescriptor;

  /* Calculate the required security descriptor length */
  Control = SE_SELF_RELATIVE;
  if ((*SecurityInformation & OWNER_SECURITY_INFORMATION) &&
      (ObjectSd->Owner != NULL))
    {
      Owner = (PSID)((ULONG_PTR)ObjectSd->Owner + (ULONG_PTR)ObjectSd);
      OwnerLength = ROUND_UP(RtlLengthSid(Owner), 4);
      Control |= (ObjectSd->Control & SE_OWNER_DEFAULTED);
    }

  if ((*SecurityInformation & GROUP_SECURITY_INFORMATION) &&
      (ObjectSd->Group != NULL))
    {
      Group = (PSID)((ULONG_PTR)ObjectSd->Group + (ULONG_PTR)ObjectSd);
      GroupLength = ROUND_UP(RtlLengthSid(Group), 4);
      Control |= (ObjectSd->Control & SE_GROUP_DEFAULTED);
    }

  if ((*SecurityInformation & DACL_SECURITY_INFORMATION) &&
      (ObjectSd->Control & SE_DACL_PRESENT))
    {
      if (ObjectSd->Dacl != NULL)
	{
	  Dacl = (PACL)((ULONG_PTR)ObjectSd->Dacl + (ULONG_PTR)ObjectSd);
	  DaclLength = ROUND_UP((ULONG)Dacl->AclSize, 4);
	}
      Control |= (ObjectSd->Control & (SE_DACL_DEFAULTED | SE_DACL_PRESENT));
    }

  if ((*SecurityInformation & SACL_SECURITY_INFORMATION) &&
      (ObjectSd->Control & SE_SACL_PRESENT))
    {
      if (ObjectSd->Sacl != NULL)
	{
	  Sacl = (PACL)((ULONG_PTR)ObjectSd->Sacl + (ULONG_PTR)ObjectSd);
	  SaclLength = ROUND_UP(Sacl->AclSize, 4);
	}
      Control |= (ObjectSd->Control & (SE_SACL_DEFAULTED | SE_SACL_PRESENT));
    }

  SdLength = OwnerLength + GroupLength + DaclLength +
	     SaclLength + sizeof(SECURITY_DESCRIPTOR);
  if (*Length < sizeof(SECURITY_DESCRIPTOR))
    {
      *Length = SdLength;
      return STATUS_BUFFER_TOO_SMALL;
    }

  /* Build the new security descrtiptor */
  RtlCreateSecurityDescriptor(SecurityDescriptor,
			      SECURITY_DESCRIPTOR_REVISION);
  SecurityDescriptor->Control = Control;

  Current = (ULONG_PTR)SecurityDescriptor + sizeof(SECURITY_DESCRIPTOR);

  if (OwnerLength != 0)
    {
      RtlCopyMemory((PVOID)Current,
		    Owner,
		    OwnerLength);
      SecurityDescriptor->Owner = (PSID)(Current - (ULONG_PTR)SecurityDescriptor);
      Current += OwnerLength;
    }

  if (GroupLength != 0)
    {
      RtlCopyMemory((PVOID)Current,
		    Group,
		    GroupLength);
      SecurityDescriptor->Group = (PSID)(Current - (ULONG_PTR)SecurityDescriptor);
      Current += GroupLength;
    }

  if (DaclLength != 0)
    {
      RtlCopyMemory((PVOID)Current,
		    Dacl,
		    DaclLength);
      SecurityDescriptor->Dacl = (PACL)(Current - (ULONG_PTR)SecurityDescriptor);
      Current += DaclLength;
    }

  if (SaclLength != 0)
    {
      RtlCopyMemory((PVOID)Current,
		    Sacl,
		    SaclLength);
      SecurityDescriptor->Sacl = (PACL)(Current - (ULONG_PTR)SecurityDescriptor);
      Current += SaclLength;
    }

  *Length = SdLength;

  return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
SeReleaseSecurityDescriptor(
	IN PSECURITY_DESCRIPTOR CapturedSecurityDescriptor,
	IN KPROCESSOR_MODE CurrentMode,
	IN BOOLEAN CaptureIfKernelMode
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
SeSetSecurityDescriptorInfo(IN PVOID Object OPTIONAL,
			    IN PSECURITY_INFORMATION SecurityInformation,
			    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
			    IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
			    IN POOL_TYPE PoolType,
			    IN PGENERIC_MAPPING GenericMapping)
{
  UNIMPLEMENTED;
  return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
SeSetSecurityDescriptorInfoEx(
	IN PVOID Object OPTIONAL,
	IN PSECURITY_INFORMATION SecurityInformation,
	IN PSECURITY_DESCRIPTOR ModificationDescriptor,
	IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
	IN ULONG AutoInheritFlags,
	IN POOL_TYPE PoolType,
	IN PGENERIC_MAPPING GenericMapping
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}


/*
 * @implemented
 */
BOOLEAN STDCALL
SeValidSecurityDescriptor(IN ULONG Length,
			  IN PSECURITY_DESCRIPTOR SecurityDescriptor)
{
  ULONG SdLength;
  PISID Sid;
  PACL Acl;

  if (Length < SECURITY_DESCRIPTOR_MIN_LENGTH)
    {
      DPRINT1("Invalid Security Descriptor revision\n");
      return FALSE;
    }

  if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION1)
    {
      DPRINT1("Invalid Security Descriptor revision\n");
      return FALSE;
    }

  if (!(SecurityDescriptor->Control & SE_SELF_RELATIVE))
    {
      DPRINT1("No self-relative Security Descriptor\n");
      return FALSE;
    }

  SdLength = sizeof(SECURITY_DESCRIPTOR);

  /* Check Owner SID */
  if (SecurityDescriptor->Owner == NULL)
    {
      DPRINT1("No Owner SID\n");
      return FALSE;
    }

  if ((ULONG_PTR)SecurityDescriptor->Owner % sizeof(ULONG))
    {
      DPRINT1("Invalid Owner SID alignment\n");
      return FALSE;
    }

  Sid = (PISID)((ULONG_PTR)SecurityDescriptor + (ULONG_PTR)SecurityDescriptor->Owner);
  if (Sid->Revision != SID_REVISION)
    {
      DPRINT1("Invalid Owner SID revision\n");
      return FALSE;
    }

  SdLength += (sizeof(SID) + (Sid->SubAuthorityCount - 1) * sizeof(ULONG));
  if (Length < SdLength)
    {
      DPRINT1("Invalid Owner SID size\n");
      return FALSE;
    }

  /* Check Group SID */
  if (SecurityDescriptor->Group != NULL)
    {
      if ((ULONG_PTR)SecurityDescriptor->Group % sizeof(ULONG))
	{
	  DPRINT1("Invalid Group SID alignment\n");
	  return FALSE;
	}

      Sid = (PSID)((ULONG_PTR)SecurityDescriptor + (ULONG_PTR)SecurityDescriptor->Group);
      if (Sid->Revision != SID_REVISION)
	{
	  DPRINT1("Invalid Group SID revision\n");
	  return FALSE;
	}

      SdLength += (sizeof(SID) + (Sid->SubAuthorityCount - 1) * sizeof(ULONG));
      if (Length < SdLength)
	{
	  DPRINT1("Invalid Group SID size\n");
	  return FALSE;
	}
    }

  /* Check DACL */
  if (SecurityDescriptor->Dacl != NULL)
    {
      if ((ULONG_PTR)SecurityDescriptor->Dacl % sizeof(ULONG))
	{
	  DPRINT1("Invalid DACL alignment\n");
	  return FALSE;
	}

      Acl = (PACL)((ULONG_PTR)SecurityDescriptor + (ULONG_PTR)SecurityDescriptor->Dacl);
      if ((Acl->AclRevision < MIN_ACL_REVISION) &&
	  (Acl->AclRevision > MAX_ACL_REVISION))
	{
	  DPRINT1("Invalid DACL revision\n");
	  return FALSE;
	}

      SdLength += Acl->AclSize;
      if (Length < SdLength)
	{
	  DPRINT1("Invalid DACL size\n");
	  return FALSE;
	}
    }

  /* Check SACL */
  if (SecurityDescriptor->Sacl != NULL)
    {
      if ((ULONG_PTR)SecurityDescriptor->Sacl % sizeof(ULONG))
	{
	  DPRINT1("Invalid SACL alignment\n");
	  return FALSE;
	}

      Acl = (PACL)((ULONG_PTR)SecurityDescriptor + (ULONG_PTR)SecurityDescriptor->Sacl);
      if ((Acl->AclRevision < MIN_ACL_REVISION) ||
	  (Acl->AclRevision > MAX_ACL_REVISION))
	{
	  DPRINT1("Invalid SACL revision\n");
	  return FALSE;
	}

      SdLength += Acl->AclSize;
      if (Length < SdLength)
	{
	  DPRINT1("Invalid SACL size\n");
	  return FALSE;
	}
    }

  return TRUE;
}

/* EOF */
