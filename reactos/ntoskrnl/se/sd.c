/* $Id: sd.c,v 1.16 2004/07/26 12:44:40 ekohl Exp $
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

#include <ddk/ntddk.h>
#include <internal/se.h>

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
    return(FALSE);

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
    return(FALSE);

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
    return(FALSE);

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
    return(FALSE);

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
    return(FALSE);

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
    return(FALSE);

  RtlCreateSecurityDescriptor(SeUnrestrictedSd,
			      SECURITY_DESCRIPTOR_REVISION);
  RtlSetDaclSecurityDescriptor(SeUnrestrictedSd,
			       TRUE,
			       SeUnrestrictedDacl,
			       FALSE);

  return(TRUE);
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
SeQuerySecurityDescriptorInfo(IN PSECURITY_INFORMATION SecurityInformation,
			      OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
			      IN OUT PULONG Length,
			      IN PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor)
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
 * @implemented
 */
BOOLEAN STDCALL
SeValidSecurityDescriptor(IN ULONG Length,
			  IN PSECURITY_DESCRIPTOR SecurityDescriptor)
{
  ULONG SdLength;
  PSID Sid;
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

  Sid = (PSID)((ULONG_PTR)SecurityDescriptor + (ULONG_PTR)SecurityDescriptor->Owner);
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
