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
 * @implemented
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
  SECURITY_DESCRIPTOR DescriptorCopy;
  PSECURITY_DESCRIPTOR NewDescriptor;
  ULONG OwnerSAC = 0, GroupSAC = 0;
  ULONG OwnerSize = 0, GroupSize = 0;
  ULONG SaclSize = 0, DaclSize = 0;
  ULONG DescriptorSize;
  NTSTATUS Status = STATUS_SUCCESS;
  
  if(OriginalSecurityDescriptor != NULL)
  {
    if(CurrentMode != KernelMode)
    {
      _SEH_TRY
      {
        ProbeForRead(OriginalSecurityDescriptor,
                     sizeof(SECURITY_DESCRIPTOR),
                     sizeof(ULONG));

        /* make a copy on the stack */
        DescriptorCopy = *OriginalSecurityDescriptor;
      }
      _SEH_HANDLE
      {
        Status = _SEH_GetExceptionCode();
      }
      _SEH_END;
      
      if(!NT_SUCCESS(Status))
      {
        return Status;
      }
    }
    else
    {
      /* make a copy on the stack */
      DescriptorCopy = *OriginalSecurityDescriptor;
    }
    
    if(CurrentMode == KernelMode && !CaptureIfKernel)
    {
      *CapturedSecurityDescriptor = OriginalSecurityDescriptor;
      return STATUS_SUCCESS;
    }
    
    if(DescriptorCopy.Revision != SECURITY_DESCRIPTOR_REVISION1)
    {
      return STATUS_UNKNOWN_REVISION;
    }
    
    if(DescriptorCopy.Control & SE_SELF_RELATIVE)
    {
      /* in case we're dealing with a self-relative descriptor, do a basic convert
         to an absolute descriptor. We do this so we can simply access the data
         using the pointers without calculating them again. */
      DescriptorCopy.Control &= ~SE_SELF_RELATIVE;
      if(DescriptorCopy.Owner != NULL)
      {
        DescriptorCopy.Owner = (PSID)((ULONG_PTR)OriginalSecurityDescriptor + (ULONG_PTR)DescriptorCopy.Owner);
      }
      if(DescriptorCopy.Group != NULL)
      {
        DescriptorCopy.Group = (PSID)((ULONG_PTR)OriginalSecurityDescriptor + (ULONG_PTR)DescriptorCopy.Group);
      }
      if(DescriptorCopy.Dacl != NULL)
      {
        DescriptorCopy.Dacl = (PACL)((ULONG_PTR)OriginalSecurityDescriptor + (ULONG_PTR)DescriptorCopy.Dacl);
      }
      if(DescriptorCopy.Sacl != NULL)
      {
        DescriptorCopy.Sacl = (PACL)((ULONG_PTR)OriginalSecurityDescriptor + (ULONG_PTR)DescriptorCopy.Sacl);
      }
    }
    
    /* determine the size of the SIDs */
#define DetermineSIDSize(SidType)                                              \
    do {                                                                       \
    if(DescriptorCopy.SidType != NULL)                                         \
    {                                                                          \
      SID *SidType = (SID*)DescriptorCopy.SidType;                             \
                                                                               \
      if(CurrentMode != KernelMode)                                            \
      {                                                                        \
        /* securely access the buffers! */                                     \
        _SEH_TRY                                                               \
        {                                                                      \
          ProbeForRead(&SidType->SubAuthorityCount,                            \
                       sizeof(SidType->SubAuthorityCount),                     \
                       1);                                                     \
          SidType##SAC = SidType->SubAuthorityCount;                           \
          SidType##Size = RtlLengthRequiredSid(SidType##SAC);                  \
          ProbeForRead(SidType,                                                \
                       SidType##Size,                                          \
                       sizeof(ULONG));                                         \
          if(!RtlValidSid(SidType))                                            \
          {                                                                    \
            Status = STATUS_INVALID_SID;                                       \
          }                                                                    \
        }                                                                      \
        _SEH_HANDLE                                                            \
        {                                                                      \
          Status = _SEH_GetExceptionCode();                                    \
        }                                                                      \
        _SEH_END;                                                              \
                                                                               \
        if(!NT_SUCCESS(Status))                                                \
        {                                                                      \
          return Status;                                                       \
        }                                                                      \
      }                                                                        \
      else                                                                     \
      {                                                                        \
        SidType##SAC = SidType->SubAuthorityCount;                             \
        SidType##Size = RtlLengthRequiredSid(SidType##SAC);                    \
      }                                                                        \
    }                                                                          \
    } while(0)
    
    DetermineSIDSize(Owner);
    DetermineSIDSize(Group);
    
    /* determine the size of the ACLs */
#define DetermineACLSize(AclType, AclFlag)                                     \
    do {                                                                       \
    if((DescriptorCopy.Control & SE_##AclFlag##_PRESENT) &&                    \
       DescriptorCopy.AclType != NULL)                                         \
    {                                                                          \
      PACL AclType = (PACL)DescriptorCopy.AclType;                             \
                                                                               \
      if(CurrentMode != KernelMode)                                            \
      {                                                                        \
        /* securely access the buffers! */                                     \
        _SEH_TRY                                                               \
        {                                                                      \
          ProbeForRead(&AclType->AclSize,                                      \
                       sizeof(AclType->AclSize),                               \
                       1);                                                     \
          AclType##Size = AclType->AclSize;                                    \
          ProbeForRead(AclType,                                                \
                       AclType##Size,                                          \
                       sizeof(ULONG));                                         \
          if(!RtlValidAcl(AclType))                                            \
          {                                                                    \
            Status = STATUS_INVALID_ACL;                                       \
          }                                                                    \
        }                                                                      \
        _SEH_HANDLE                                                            \
        {                                                                      \
          Status = _SEH_GetExceptionCode();                                    \
        }                                                                      \
        _SEH_END;                                                              \
                                                                               \
        if(!NT_SUCCESS(Status))                                                \
        {                                                                      \
          return Status;                                                       \
        }                                                                      \
      }                                                                        \
      else                                                                     \
      {                                                                        \
        AclType##Size = AclType->AclSize;                                      \
      }                                                                        \
    }                                                                          \
    else                                                                       \
    {                                                                          \
      DescriptorCopy.AclType = NULL;                                           \
    }                                                                          \
    } while(0)
    
    DetermineACLSize(Sacl, SACL);
    DetermineACLSize(Dacl, DACL);
    
    /* allocate enough memory to store a complete copy of a self-relative
       security descriptor */
    DescriptorSize = sizeof(SECURITY_DESCRIPTOR) +
                     ROUND_UP(OwnerSize, sizeof(ULONG)) +
                     ROUND_UP(GroupSize, sizeof(ULONG)) +
                     ROUND_UP(SaclSize, sizeof(ULONG)) +
                     ROUND_UP(DaclSize, sizeof(ULONG));

    NewDescriptor = ExAllocatePool(PagedPool,
                                   DescriptorSize);
    if(NewDescriptor != NULL)
    {
      ULONG_PTR Offset = sizeof(SECURITY_DESCRIPTOR);
      
      NewDescriptor->Revision = DescriptorCopy.Revision;
      NewDescriptor->Sbz1 = DescriptorCopy.Sbz1;
      NewDescriptor->Control = DescriptorCopy.Control | SE_SELF_RELATIVE;
      
      /* setup the offsets to the SIDs and ACLs */
      NewDescriptor->Owner = (PVOID)Offset;
      Offset += ROUND_UP(OwnerSize, sizeof(ULONG));
      NewDescriptor->Group = (PVOID)Offset;
      Offset += ROUND_UP(GroupSize, sizeof(ULONG));
      NewDescriptor->Sacl = (PVOID)Offset;
      Offset += ROUND_UP(SaclSize, sizeof(ULONG));
      NewDescriptor->Dacl = (PVOID)Offset;
      
      _SEH_TRY
      {
        /* copy the SIDs and ACLs to the new self-relative security descriptor */
        RtlCopyMemory((PVOID)((ULONG_PTR)NewDescriptor + (ULONG_PTR)NewDescriptor->Owner),
                      DescriptorCopy.Owner,
                      OwnerSize);
        RtlCopyMemory((PVOID)((ULONG_PTR)NewDescriptor + (ULONG_PTR)NewDescriptor->Group),
                      DescriptorCopy.Group,
                      GroupSize);
        RtlCopyMemory((PVOID)((ULONG_PTR)NewDescriptor + (ULONG_PTR)NewDescriptor->Sacl),
                      DescriptorCopy.Sacl,
                      SaclSize);
        RtlCopyMemory((PVOID)((ULONG_PTR)NewDescriptor + (ULONG_PTR)NewDescriptor->Dacl),
                      DescriptorCopy.Dacl,
                      DaclSize);
      }
      _SEH_HANDLE
      {
        Status = _SEH_GetExceptionCode();
      }
      _SEH_END;
      
      if(NT_SUCCESS(Status))
      {
        /* we're finally done! copy the pointer to the captured descriptor to
           to the caller */
        *CapturedSecurityDescriptor = NewDescriptor;
        return STATUS_SUCCESS;
      }
      else
      {
        /* we failed to copy the data to the new descriptor */
        ExFreePool(NewDescriptor);
      }
    }
    else
    {
      Status = STATUS_INSUFFICIENT_RESOURCES;
    }
  }
  else
  {
    /* nothing to do... */
    *CapturedSecurityDescriptor = NULL;
  }
  
  return Status;
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
 * @implemented
 */
NTSTATUS
STDCALL
SeReleaseSecurityDescriptor(
	IN PSECURITY_DESCRIPTOR CapturedSecurityDescriptor,
	IN KPROCESSOR_MODE CurrentMode,
	IN BOOLEAN CaptureIfKernelMode
	)
{
  /* WARNING! You need to call this function with the same value for CurrentMode
              and CaptureIfKernelMode that you previously passed to
              SeCaptureSecurityDescriptor() in order to avoid memory leaks! */
  if(CapturedSecurityDescriptor != NULL &&
     (CurrentMode == UserMode ||
      (CurrentMode == KernelMode && CaptureIfKernelMode)))
  {
    /* only delete the descriptor when SeCaptureSecurityDescriptor() allocated one! */
    ExFreePool(CapturedSecurityDescriptor);
  }

  return STATUS_SUCCESS;
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
