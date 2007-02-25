/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/se/sd.c
 * PURPOSE:         Security manager
 *
 * PROGRAMMERS:     David Welch <welch@cwcom.net>
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, SepInitSDs)
#endif


/* GLOBALS ******************************************************************/

PSECURITY_DESCRIPTOR SePublicDefaultSd = NULL;
PSECURITY_DESCRIPTOR SePublicDefaultUnrestrictedSd = NULL;
PSECURITY_DESCRIPTOR SePublicOpenSd = NULL;
PSECURITY_DESCRIPTOR SePublicOpenUnrestrictedSd = NULL;
PSECURITY_DESCRIPTOR SeSystemDefaultSd = NULL;
PSECURITY_DESCRIPTOR SeUnrestrictedSd = NULL;

/* FUNCTIONS ***************************************************************/

BOOLEAN
INIT_FUNCTION
NTAPI
SepInitSDs(VOID)
{
  /* Create PublicDefaultSd */
  SePublicDefaultSd = ExAllocatePoolWithTag(PagedPool,
				     sizeof(SECURITY_DESCRIPTOR), TAG_SD);
  if (SePublicDefaultSd == NULL)
    return FALSE;

  RtlCreateSecurityDescriptor(SePublicDefaultSd,
			      SECURITY_DESCRIPTOR_REVISION);
  RtlSetDaclSecurityDescriptor(SePublicDefaultSd,
			       TRUE,
			       SePublicDefaultDacl,
			       FALSE);

  /* Create PublicDefaultUnrestrictedSd */
  SePublicDefaultUnrestrictedSd = ExAllocatePoolWithTag(PagedPool,
						 sizeof(SECURITY_DESCRIPTOR), TAG_SD);
  if (SePublicDefaultUnrestrictedSd == NULL)
    return FALSE;

  RtlCreateSecurityDescriptor(SePublicDefaultUnrestrictedSd,
			      SECURITY_DESCRIPTOR_REVISION);
  RtlSetDaclSecurityDescriptor(SePublicDefaultUnrestrictedSd,
			       TRUE,
			       SePublicDefaultUnrestrictedDacl,
			       FALSE);

  /* Create PublicOpenSd */
  SePublicOpenSd = ExAllocatePoolWithTag(PagedPool,
				  sizeof(SECURITY_DESCRIPTOR), TAG_SD);
  if (SePublicOpenSd == NULL)
    return FALSE;

  RtlCreateSecurityDescriptor(SePublicOpenSd,
			      SECURITY_DESCRIPTOR_REVISION);
  RtlSetDaclSecurityDescriptor(SePublicOpenSd,
			       TRUE,
			       SePublicOpenDacl,
			       FALSE);

  /* Create PublicOpenUnrestrictedSd */
  SePublicOpenUnrestrictedSd = ExAllocatePoolWithTag(PagedPool,
					      sizeof(SECURITY_DESCRIPTOR), TAG_SD);
  if (SePublicOpenUnrestrictedSd == NULL)
    return FALSE;

  RtlCreateSecurityDescriptor(SePublicOpenUnrestrictedSd,
			      SECURITY_DESCRIPTOR_REVISION);
  RtlSetDaclSecurityDescriptor(SePublicOpenUnrestrictedSd,
			       TRUE,
			       SePublicOpenUnrestrictedDacl,
			       FALSE);

  /* Create SystemDefaultSd */
  SeSystemDefaultSd = ExAllocatePoolWithTag(PagedPool,
				     sizeof(SECURITY_DESCRIPTOR), TAG_SD);
  if (SeSystemDefaultSd == NULL)
    return FALSE;

  RtlCreateSecurityDescriptor(SeSystemDefaultSd,
			      SECURITY_DESCRIPTOR_REVISION);
  RtlSetDaclSecurityDescriptor(SeSystemDefaultSd,
			       TRUE,
			       SeSystemDefaultDacl,
			       FALSE);

  /* Create UnrestrictedSd */
  SeUnrestrictedSd = ExAllocatePoolWithTag(PagedPool,
				    sizeof(SECURITY_DESCRIPTOR), TAG_SD);
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

NTSTATUS
NTAPI
SeSetWorldSecurityDescriptor(SECURITY_INFORMATION SecurityInformation,
                             PISECURITY_DESCRIPTOR SecurityDescriptor,
                             PULONG BufferLength)
{
  ULONG_PTR Current;
  ULONG SidSize;
  ULONG SdSize;
  NTSTATUS Status;
  PISECURITY_DESCRIPTOR_RELATIVE SdRel = (PISECURITY_DESCRIPTOR_RELATIVE)SecurityDescriptor;

  DPRINT("SeSetWorldSecurityDescriptor() called\n");

  if (SecurityInformation == 0)
    {
      return STATUS_ACCESS_DENIED;
    }

  /* calculate the minimum size of the buffer */
  SidSize = RtlLengthSid(SeWorldSid);
  SdSize = sizeof(SECURITY_DESCRIPTOR_RELATIVE);
  if (SecurityInformation & OWNER_SECURITY_INFORMATION)
      SdSize += SidSize;
  if (SecurityInformation & GROUP_SECURITY_INFORMATION)
      SdSize += SidSize;
  if (SecurityInformation & DACL_SECURITY_INFORMATION)
    {
      SdSize += sizeof(ACL) + sizeof(ACE) + SidSize;
    }

  if (*BufferLength < SdSize)
    {
      *BufferLength = SdSize;
      return STATUS_BUFFER_TOO_SMALL;
    }

  *BufferLength = SdSize;

  Status = RtlCreateSecurityDescriptorRelative(SdRel,
                                               SECURITY_DESCRIPTOR_REVISION);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  Current = (ULONG_PTR)(SdRel + 1);

  if (SecurityInformation & OWNER_SECURITY_INFORMATION)
    {
      RtlCopyMemory((PVOID)Current,
                    SeWorldSid,
                    SidSize);
      SdRel->Owner = (ULONG)((ULONG_PTR)Current - (ULONG_PTR)SdRel);
      Current += SidSize;
    }

  if (SecurityInformation & GROUP_SECURITY_INFORMATION)
    {
      RtlCopyMemory((PVOID)Current,
                    SeWorldSid,
                    SidSize);
      SdRel->Group = (ULONG)((ULONG_PTR)Current - (ULONG_PTR)SdRel);
      Current += SidSize;
    }

  if (SecurityInformation & DACL_SECURITY_INFORMATION)
    {
      PACL Dacl = (PACL)Current;
      SdRel->Control |= SE_DACL_PRESENT;

      Status = RtlCreateAcl(Dacl,
                            sizeof(ACL) + sizeof(ACE) + SidSize,
                            ACL_REVISION);
      if (!NT_SUCCESS(Status))
          return Status;

      Status = RtlAddAccessAllowedAce(Dacl,
                                      ACL_REVISION,
                                      GENERIC_ALL,
                                      SeWorldSid);
      if (!NT_SUCCESS(Status))
          return Status;

      SdRel->Dacl = (ULONG)((ULONG_PTR)Current - (ULONG_PTR)SdRel);
    }

  if (SecurityInformation & SACL_SECURITY_INFORMATION)
    {
      /* FIXME - SdRel->Control |= SE_SACL_PRESENT; */
    }

  return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
SepCaptureSecurityQualityOfService(IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
                                   IN KPROCESSOR_MODE AccessMode,
                                   IN POOL_TYPE PoolType,
                                   IN BOOLEAN CaptureIfKernel,
                                   OUT PSECURITY_QUALITY_OF_SERVICE *CapturedSecurityQualityOfService,
                                   OUT PBOOLEAN Present)
{
  PSECURITY_QUALITY_OF_SERVICE CapturedQos;
  NTSTATUS Status = STATUS_SUCCESS;

  PAGED_CODE();

  ASSERT(CapturedSecurityQualityOfService);
  ASSERT(Present);

  if(ObjectAttributes != NULL)
  {
    if(AccessMode != KernelMode)
    {
      SECURITY_QUALITY_OF_SERVICE SafeQos;

      _SEH_TRY
      {
        ProbeForRead(ObjectAttributes,
                     sizeof(ObjectAttributes),
                     sizeof(ULONG));
        if(ObjectAttributes->Length == sizeof(OBJECT_ATTRIBUTES))
        {
          if(ObjectAttributes->SecurityQualityOfService != NULL)
          {
            ProbeForRead(ObjectAttributes->SecurityQualityOfService,
                         sizeof(SECURITY_QUALITY_OF_SERVICE),
                         sizeof(ULONG));

            if(((PSECURITY_QUALITY_OF_SERVICE)ObjectAttributes->SecurityQualityOfService)->Length ==
               sizeof(SECURITY_QUALITY_OF_SERVICE))
            {
              /* don't allocate memory here because ExAllocate should bugcheck
                 the system if it's buggy, SEH would catch that! So make a local
                 copy of the qos structure.*/
              RtlCopyMemory(&SafeQos,
                            ObjectAttributes->SecurityQualityOfService,
                            sizeof(SECURITY_QUALITY_OF_SERVICE));
              *Present = TRUE;
            }
            else
            {
              Status = STATUS_INVALID_PARAMETER;
            }
          }
          else
          {
            *CapturedSecurityQualityOfService = NULL;
            *Present = FALSE;
          }
        }
        else
        {
          Status = STATUS_INVALID_PARAMETER;
        }
      }
      _SEH_HANDLE
      {
        Status = _SEH_GetExceptionCode();
      }
      _SEH_END;

      if(NT_SUCCESS(Status))
      {
        if(*Present)
        {
          CapturedQos = ExAllocatePool(PoolType,
                                       sizeof(SECURITY_QUALITY_OF_SERVICE));
          if(CapturedQos != NULL)
          {
            RtlCopyMemory(CapturedQos,
                          &SafeQos,
                          sizeof(SECURITY_QUALITY_OF_SERVICE));
            *CapturedSecurityQualityOfService = CapturedQos;
          }
          else
          {
            Status = STATUS_INSUFFICIENT_RESOURCES;
          }
        }
        else
        {
          *CapturedSecurityQualityOfService = NULL;
        }
      }
    }
    else
    {
      if(ObjectAttributes->Length == sizeof(OBJECT_ATTRIBUTES))
      {
        if(CaptureIfKernel)
        {
          if(ObjectAttributes->SecurityQualityOfService != NULL)
          {
            if(((PSECURITY_QUALITY_OF_SERVICE)ObjectAttributes->SecurityQualityOfService)->Length ==
               sizeof(SECURITY_QUALITY_OF_SERVICE))
            {
              CapturedQos = ExAllocatePool(PoolType,
                                           sizeof(SECURITY_QUALITY_OF_SERVICE));
              if(CapturedQos != NULL)
              {
                RtlCopyMemory(CapturedQos,
                              ObjectAttributes->SecurityQualityOfService,
                              sizeof(SECURITY_QUALITY_OF_SERVICE));
                *CapturedSecurityQualityOfService = CapturedQos;
                *Present = TRUE;
              }
              else
              {
                Status = STATUS_INSUFFICIENT_RESOURCES;
              }
            }
            else
            {
              Status = STATUS_INVALID_PARAMETER;
            }
          }
          else
          {
            *CapturedSecurityQualityOfService = NULL;
            *Present = FALSE;
          }
        }
        else
        {
          *CapturedSecurityQualityOfService = (PSECURITY_QUALITY_OF_SERVICE)ObjectAttributes->SecurityQualityOfService;
          *Present = (ObjectAttributes->SecurityQualityOfService != NULL);
        }
      }
      else
      {
        Status = STATUS_INVALID_PARAMETER;
      }
    }
  }
  else
  {
    *CapturedSecurityQualityOfService = NULL;
    *Present = FALSE;
  }

  return Status;
}


VOID
NTAPI
SepReleaseSecurityQualityOfService(IN PSECURITY_QUALITY_OF_SERVICE CapturedSecurityQualityOfService  OPTIONAL,
                                   IN KPROCESSOR_MODE AccessMode,
                                   IN BOOLEAN CaptureIfKernel)
{
  PAGED_CODE();

  if(CapturedSecurityQualityOfService != NULL &&
     (AccessMode != KernelMode || CaptureIfKernel))
  {
    ExFreePool(CapturedSecurityQualityOfService);
  }
}


/*
 * @implemented
 */
NTSTATUS
STDCALL
SeCaptureSecurityDescriptor(
	IN PSECURITY_DESCRIPTOR _OriginalSecurityDescriptor,
	IN KPROCESSOR_MODE CurrentMode,
	IN POOL_TYPE PoolType,
	IN BOOLEAN CaptureIfKernel,
	OUT PSECURITY_DESCRIPTOR *CapturedSecurityDescriptor
	)
{
  PISECURITY_DESCRIPTOR OriginalSecurityDescriptor = _OriginalSecurityDescriptor;
  SECURITY_DESCRIPTOR DescriptorCopy;
  PISECURITY_DESCRIPTOR NewDescriptor;
  ULONG OwnerSAC = 0, GroupSAC = 0;
  ULONG OwnerSize = 0, GroupSize = 0;
  ULONG SaclSize = 0, DaclSize = 0;
  ULONG DescriptorSize = 0;
  NTSTATUS Status = STATUS_SUCCESS;

  if(OriginalSecurityDescriptor != NULL)
  {
    if(CurrentMode != KernelMode)
    {
      RtlZeroMemory(&DescriptorCopy, sizeof(DescriptorCopy));

      _SEH_TRY
      {
        /* first only probe and copy until the control field of the descriptor
           to determine whether it's a self-relative descriptor */
        DescriptorSize = FIELD_OFFSET(SECURITY_DESCRIPTOR,
                                      Owner);
        ProbeForRead(OriginalSecurityDescriptor,
                     DescriptorSize,
                     sizeof(ULONG));

        if(OriginalSecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION1)
        {
          Status = STATUS_UNKNOWN_REVISION;
          _SEH_LEAVE;
        }

        /* make a copy on the stack */
        DescriptorCopy.Revision = OriginalSecurityDescriptor->Revision;
        DescriptorCopy.Sbz1 = OriginalSecurityDescriptor->Sbz1;
        DescriptorCopy.Control = OriginalSecurityDescriptor->Control;
        DescriptorSize = ((DescriptorCopy.Control & SE_SELF_RELATIVE) ?
                          sizeof(SECURITY_DESCRIPTOR_RELATIVE) : sizeof(SECURITY_DESCRIPTOR));

        /* probe and copy the entire security descriptor structure. The SIDs
           and ACLs will be probed and copied later though */
        ProbeForRead(OriginalSecurityDescriptor,
                     DescriptorSize,
                     sizeof(ULONG));
        if(DescriptorCopy.Control & SE_SELF_RELATIVE)
        {
          PISECURITY_DESCRIPTOR_RELATIVE RelSD = (PISECURITY_DESCRIPTOR_RELATIVE)OriginalSecurityDescriptor;

          DescriptorCopy.Owner = (PSID)RelSD->Owner;
          DescriptorCopy.Group = (PSID)RelSD->Group;
          DescriptorCopy.Sacl = (PACL)RelSD->Sacl;
          DescriptorCopy.Dacl = (PACL)RelSD->Dacl;
        }
        else
        {
          DescriptorCopy.Owner = OriginalSecurityDescriptor->Owner;
          DescriptorCopy.Group = OriginalSecurityDescriptor->Group;
          DescriptorCopy.Sacl = OriginalSecurityDescriptor->Sacl;
          DescriptorCopy.Dacl = OriginalSecurityDescriptor->Dacl;
        }
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
    else if(!CaptureIfKernel)
    {
      if(OriginalSecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION1)
      {
        return STATUS_UNKNOWN_REVISION;
      }

      *CapturedSecurityDescriptor = OriginalSecurityDescriptor;
      return STATUS_SUCCESS;
    }
    else
    {
      if(OriginalSecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION1)
      {
        return STATUS_UNKNOWN_REVISION;
      }

      /* make a copy on the stack */
      DescriptorCopy.Revision = OriginalSecurityDescriptor->Revision;
      DescriptorCopy.Sbz1 = OriginalSecurityDescriptor->Sbz1;
      DescriptorCopy.Control = OriginalSecurityDescriptor->Control;
      DescriptorSize = ((DescriptorCopy.Control & SE_SELF_RELATIVE) ?
                        sizeof(SECURITY_DESCRIPTOR_RELATIVE) : sizeof(SECURITY_DESCRIPTOR));
      if(DescriptorCopy.Control & SE_SELF_RELATIVE)
      {
        PISECURITY_DESCRIPTOR_RELATIVE RelSD = (PISECURITY_DESCRIPTOR_RELATIVE)OriginalSecurityDescriptor;

        DescriptorCopy.Owner = (PSID)RelSD->Owner;
        DescriptorCopy.Group = (PSID)RelSD->Group;
        DescriptorCopy.Sacl = (PACL)RelSD->Sacl;
        DescriptorCopy.Dacl = (PACL)RelSD->Dacl;
      }
      else
      {
        DescriptorCopy.Owner = OriginalSecurityDescriptor->Owner;
        DescriptorCopy.Group = OriginalSecurityDescriptor->Group;
        DescriptorCopy.Sacl = OriginalSecurityDescriptor->Sacl;
        DescriptorCopy.Dacl = OriginalSecurityDescriptor->Dacl;
      }
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
          SidType##SAC = ProbeForReadUchar(&SidType->SubAuthorityCount);       \
          SidType##Size = RtlLengthRequiredSid(SidType##SAC);                  \
          DescriptorSize += ROUND_UP(SidType##Size, sizeof(ULONG));            \
          ProbeForRead(SidType,                                                \
                       SidType##Size,                                          \
                       sizeof(ULONG));                                         \
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
        DescriptorSize += ROUND_UP(SidType##Size, sizeof(ULONG));              \
      }                                                                        \
    }                                                                          \
    } while(0)

    DetermineSIDSize(Owner);
    DetermineSIDSize(Group);

#undef DetermineSIDSize

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
          AclType##Size = ProbeForReadUshort(&AclType->AclSize);               \
          DescriptorSize += ROUND_UP(AclType##Size, sizeof(ULONG));            \
          ProbeForRead(AclType,                                                \
                       AclType##Size,                                          \
                       sizeof(ULONG));                                         \
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
        DescriptorSize += ROUND_UP(AclType##Size, sizeof(ULONG));              \
      }                                                                        \
    }                                                                          \
    else                                                                       \
    {                                                                          \
      DescriptorCopy.AclType = NULL;                                           \
    }                                                                          \
    } while(0)

    DetermineACLSize(Sacl, SACL);
    DetermineACLSize(Dacl, DACL);

#undef DetermineACLSize

    /* allocate enough memory to store a complete copy of a self-relative
       security descriptor */
    NewDescriptor = ExAllocatePool(PoolType,
                                   DescriptorSize);
    if(NewDescriptor != NULL)
    {
      ULONG_PTR Offset = sizeof(SECURITY_DESCRIPTOR);

      RtlZeroMemory(NewDescriptor, DescriptorSize);
      NewDescriptor->Revision = DescriptorCopy.Revision;
      NewDescriptor->Sbz1 = DescriptorCopy.Sbz1;
      NewDescriptor->Control = DescriptorCopy.Control | SE_SELF_RELATIVE;

      _SEH_TRY
      {
        /* setup the offsets and copy the SIDs and ACLs to the new
           self-relative security descriptor. Probing the pointers is not
           neccessary anymore as we did that when collecting the sizes!
           Make sure to validate the SIDs and ACLs *again* as they could have
           been modified in the meanwhile! */
#define CopySID(Type)                                                          \
        do {                                                                   \
        if(DescriptorCopy.Type != NULL)                                        \
        {                                                                      \
          NewDescriptor->Type = (PVOID)Offset;                                 \
          RtlCopyMemory((PVOID)((ULONG_PTR)NewDescriptor +                     \
                                (ULONG_PTR)NewDescriptor->Type),               \
                        DescriptorCopy.Type,                                   \
                        Type##Size);                                           \
          if (!RtlValidSid((PSID)((ULONG_PTR)NewDescriptor +                   \
                                  (ULONG_PTR)NewDescriptor->Type)))            \
          {                                                                    \
            RtlRaiseStatus(STATUS_INVALID_SID);                                \
          }                                                                    \
          Offset += ROUND_UP(Type##Size, sizeof(ULONG));                       \
        }                                                                      \
        } while(0)

        CopySID(Owner);
        CopySID(Group);

#undef CopySID

#define CopyACL(Type)                                                          \
        do {                                                                   \
        if(DescriptorCopy.Type != NULL)                                        \
        {                                                                      \
          NewDescriptor->Type = (PVOID)Offset;                                 \
          RtlCopyMemory((PVOID)((ULONG_PTR)NewDescriptor +                     \
                                (ULONG_PTR)NewDescriptor->Type),               \
                        DescriptorCopy.Type,                                   \
                        Type##Size);                                           \
          if (!RtlValidAcl((PACL)((ULONG_PTR)NewDescriptor +                   \
                                  (ULONG_PTR)NewDescriptor->Type)))            \
          {                                                                    \
            RtlRaiseStatus(STATUS_INVALID_ACL);                                \
          }                                                                    \
          Offset += ROUND_UP(Type##Size, sizeof(ULONG));                       \
        }                                                                      \
        } while(0)

        CopyACL(Sacl);
        CopyACL(Dacl);

#undef CopyACL
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
  PISECURITY_DESCRIPTOR ObjectSd;
  PISECURITY_DESCRIPTOR_RELATIVE RelSD;
  PSID Owner = NULL;
  PSID Group = NULL;
  PACL Dacl = NULL;
  PACL Sacl = NULL;
  ULONG OwnerLength = 0;
  ULONG GroupLength = 0;
  ULONG DaclLength = 0;
  ULONG SaclLength = 0;
  ULONG Control = 0;
  ULONG_PTR Current;
  ULONG SdLength;

  RelSD = (PISECURITY_DESCRIPTOR_RELATIVE)SecurityDescriptor;

  if (*ObjectsSecurityDescriptor == NULL)
    {
      if (*Length < sizeof(SECURITY_DESCRIPTOR_RELATIVE))
	{
	  *Length = sizeof(SECURITY_DESCRIPTOR_RELATIVE);
	  return STATUS_BUFFER_TOO_SMALL;
	}

      *Length = sizeof(SECURITY_DESCRIPTOR_RELATIVE);
      RtlCreateSecurityDescriptorRelative(RelSD,
				          SECURITY_DESCRIPTOR_REVISION);
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
	     SaclLength + sizeof(SECURITY_DESCRIPTOR_RELATIVE);
  if (*Length < SdLength)
    {
      *Length = SdLength;
      return STATUS_BUFFER_TOO_SMALL;
    }

  /* Build the new security descrtiptor */
  RtlCreateSecurityDescriptorRelative(RelSD,
			              SECURITY_DESCRIPTOR_REVISION);
  RelSD->Control = (USHORT)Control;

  Current = (ULONG_PTR)(RelSD + 1);

  if (OwnerLength != 0)
    {
      RtlCopyMemory((PVOID)Current,
		    Owner,
		    OwnerLength);
      RelSD->Owner = (ULONG)(Current - (ULONG_PTR)SecurityDescriptor);
      Current += OwnerLength;
    }

  if (GroupLength != 0)
    {
      RtlCopyMemory((PVOID)Current,
		    Group,
		    GroupLength);
      RelSD->Group = (ULONG)(Current - (ULONG_PTR)SecurityDescriptor);
      Current += GroupLength;
    }

  if (DaclLength != 0)
    {
      RtlCopyMemory((PVOID)Current,
		    Dacl,
		    DaclLength);
      RelSD->Dacl = (ULONG)(Current - (ULONG_PTR)SecurityDescriptor);
      Current += DaclLength;
    }

  if (SaclLength != 0)
    {
      RtlCopyMemory((PVOID)Current,
		    Sacl,
		    SaclLength);
      RelSD->Sacl = (ULONG)(Current - (ULONG_PTR)SecurityDescriptor);
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
  PAGED_CODE();

  /* WARNING! You need to call this function with the same value for CurrentMode
              and CaptureIfKernelMode that you previously passed to
              SeCaptureSecurityDescriptor() in order to avoid memory leaks! */
  if(CapturedSecurityDescriptor != NULL &&
     (CurrentMode != KernelMode ||
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
			  IN PSECURITY_DESCRIPTOR _SecurityDescriptor)
{
  ULONG SdLength;
  PISID Sid;
  PACL Acl;
  PISECURITY_DESCRIPTOR SecurityDescriptor = _SecurityDescriptor;

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
