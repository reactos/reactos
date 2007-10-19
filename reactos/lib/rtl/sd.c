/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Security descriptor functions
 * FILE:              lib/rtl/sd.c
 * PROGRAMER:         David Welch <welch@cwcom.net>
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/


static VOID
RtlpQuerySecurityDescriptorPointers(IN PISECURITY_DESCRIPTOR SecurityDescriptor,
                                    OUT PSID *Owner  OPTIONAL,
                                    OUT PSID *Group  OPTIONAL,
                                    OUT PACL *Sacl  OPTIONAL,
                                    OUT PACL *Dacl  OPTIONAL)
{
  if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
  {
    PISECURITY_DESCRIPTOR_RELATIVE RelSD = (PISECURITY_DESCRIPTOR_RELATIVE)SecurityDescriptor;
    if(Owner != NULL)
    {
      *Owner = ((RelSD->Owner != 0) ? (PSID)((ULONG_PTR)RelSD + RelSD->Owner) : NULL);
    }
    if(Group != NULL)
    {
      *Group = ((RelSD->Group != 0) ? (PSID)((ULONG_PTR)RelSD + RelSD->Group) : NULL);
    }
    if(Sacl != NULL)
    {
      *Sacl = (((RelSD->Control & SE_SACL_PRESENT) && (RelSD->Sacl != 0)) ?
              (PSID)((ULONG_PTR)RelSD + RelSD->Sacl) : NULL);
    }
    if(Dacl != NULL)
    {
      *Dacl = (((RelSD->Control & SE_DACL_PRESENT) && (RelSD->Dacl != 0)) ?
              (PSID)((ULONG_PTR)RelSD + RelSD->Dacl) : NULL);
    }
  }
  else
  {
    if(Owner != NULL)
    {
      *Owner = SecurityDescriptor->Owner;
    }
    if(Group != NULL)
    {
      *Group = SecurityDescriptor->Group;
    }
    if(Sacl != NULL)
    {
      *Sacl = ((SecurityDescriptor->Control & SE_SACL_PRESENT) ? SecurityDescriptor->Sacl : NULL);
    }
    if(Dacl != NULL)
    {
      *Dacl = ((SecurityDescriptor->Control & SE_DACL_PRESENT) ? SecurityDescriptor->Dacl : NULL);
    }
  }
}

static VOID
RtlpQuerySecurityDescriptor(PISECURITY_DESCRIPTOR SecurityDescriptor,
                            PSID* Owner,
                            PULONG OwnerLength,
                            PSID* Group,
                            PULONG GroupLength,
                            PACL* Dacl,
                            PULONG DaclLength,
                            PACL* Sacl,
                            PULONG SaclLength)
{
   RtlpQuerySecurityDescriptorPointers(SecurityDescriptor,
                                       Owner,
                                       Group,
                                       Sacl,
                                       Dacl);

   if (Owner != NULL)
   {
      *OwnerLength = ((*Owner != NULL) ? ROUND_UP(RtlLengthSid(*Owner), 4) : 0);
   }

   if (Group != NULL)
   {
      *GroupLength = ((*Group != NULL) ? ROUND_UP(RtlLengthSid(*Group), 4) : 0);
   }

   if (Dacl != NULL)
   {
      *DaclLength = ((*Dacl != NULL) ? ROUND_UP((*Dacl)->AclSize, 4) : 0);
   }

   if (Sacl != NULL)
   {
      *SaclLength = ((*Sacl != NULL) ? ROUND_UP((*Sacl)->AclSize, 4) : 0);
   }
}

/*
 * @implemented
 */
NTSTATUS NTAPI
RtlCreateSecurityDescriptor(OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
                            IN ULONG Revision)
{
   PISECURITY_DESCRIPTOR pSD = (PISECURITY_DESCRIPTOR)SecurityDescriptor;

   PAGED_CODE_RTL();

   if (Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   pSD->Revision = Revision;
   pSD->Sbz1 = 0;
   pSD->Control = 0;
   pSD->Owner = NULL;
   pSD->Group = NULL;
   pSD->Sacl = NULL;
   pSD->Dacl = NULL;

   return STATUS_SUCCESS;
}


NTSTATUS NTAPI
RtlCreateSecurityDescriptorRelative (OUT PISECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
			             IN ULONG Revision)
{
   PAGED_CODE_RTL();

   if (Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   SecurityDescriptor->Revision = Revision;
   SecurityDescriptor->Sbz1 = 0;
   SecurityDescriptor->Control = SE_SELF_RELATIVE;
   SecurityDescriptor->Owner = 0;
   SecurityDescriptor->Group = 0;
   SecurityDescriptor->Sacl = 0;
   SecurityDescriptor->Dacl = 0;

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
ULONG NTAPI
RtlLengthSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor)
{
   PSID Owner, Group;
   PACL Sacl, Dacl;
   ULONG Length = sizeof(SECURITY_DESCRIPTOR);

   PAGED_CODE_RTL();

   RtlpQuerySecurityDescriptorPointers((PISECURITY_DESCRIPTOR)SecurityDescriptor,
                                       &Owner,
                                       &Group,
                                       &Sacl,
                                       &Dacl);

   if (Owner != NULL)
   {
      Length += ROUND_UP(RtlLengthSid(Owner), 4);
   }

   if (Group != NULL)
   {
      Length += ROUND_UP(RtlLengthSid(Group), 4);
   }

   if (Dacl != NULL)
   {
      Length += ROUND_UP(Dacl->AclSize, 4);
   }

   if (Sacl != NULL)
   {
      Length += ROUND_UP(Sacl->AclSize, 4);
   }

   return Length;
}


/*
 * @implemented
 */
NTSTATUS NTAPI
RtlGetDaclSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                             OUT PBOOLEAN DaclPresent,
                             OUT PACL* Dacl,
                             OUT PBOOLEAN DaclDefaulted)
{
   PISECURITY_DESCRIPTOR pSD = (PISECURITY_DESCRIPTOR)SecurityDescriptor;

   PAGED_CODE_RTL();

   if (pSD->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   if (!(pSD->Control & SE_DACL_PRESENT))
   {
      *DaclPresent = FALSE;
      return STATUS_SUCCESS;
   }
   *DaclPresent = TRUE;

   RtlpQuerySecurityDescriptorPointers(pSD,
                                       NULL,
                                       NULL,
                                       NULL,
                                       Dacl);

   *DaclDefaulted = ((pSD->Control & SE_DACL_DEFAULTED) ? TRUE : FALSE);

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS NTAPI
RtlSetDaclSecurityDescriptor(IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
                             IN BOOLEAN DaclPresent,
                             IN PACL Dacl,
                             IN BOOLEAN DaclDefaulted)
{
   PISECURITY_DESCRIPTOR pSD = (PISECURITY_DESCRIPTOR)SecurityDescriptor;

   PAGED_CODE_RTL();

   if (pSD->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   if (pSD->Control & SE_SELF_RELATIVE)
   {
      return STATUS_BAD_DESCRIPTOR_FORMAT;
   }

   if (!DaclPresent)
   {
      pSD->Control = pSD->Control & ~(SE_DACL_PRESENT);
      return STATUS_SUCCESS;
   }

   pSD->Dacl = Dacl;
   pSD->Control |= SE_DACL_PRESENT;
   pSD->Control &= ~(SE_DACL_DEFAULTED);

   if (DaclDefaulted)
   {
      pSD->Control |= SE_DACL_DEFAULTED;
   }

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
BOOLEAN NTAPI
RtlValidSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor)
{
   PISECURITY_DESCRIPTOR pSD = (PISECURITY_DESCRIPTOR)SecurityDescriptor;
   PSID Owner, Group;
   PACL Sacl, Dacl;

   PAGED_CODE_RTL();

   if (pSD->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return FALSE;
   }

   RtlpQuerySecurityDescriptorPointers(pSD,
                                       &Owner,
                                       &Group,
                                       &Sacl,
                                       &Dacl);

   if ((Owner != NULL && !RtlValidSid(Owner)) ||
       (Group != NULL && !RtlValidSid(Group)) ||
       (Sacl != NULL && !RtlValidAcl(Sacl)) ||
       (Dacl != NULL && !RtlValidAcl(Dacl)))
   {
      return FALSE;
   }

   return TRUE;
}


/*
 * @implemented
 */
NTSTATUS NTAPI
RtlSetOwnerSecurityDescriptor(IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
                              IN PSID Owner,
                              IN BOOLEAN OwnerDefaulted)
{
   PISECURITY_DESCRIPTOR pSD = (PISECURITY_DESCRIPTOR)SecurityDescriptor;

   PAGED_CODE_RTL();

   if (pSD->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   if (pSD->Control & SE_SELF_RELATIVE)
   {
      return STATUS_BAD_DESCRIPTOR_FORMAT;
   }

   pSD->Owner = Owner;
   pSD->Control &= ~(SE_OWNER_DEFAULTED);

   if (OwnerDefaulted)
   {
      pSD->Control |= SE_OWNER_DEFAULTED;
   }

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS NTAPI
RtlGetOwnerSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                              OUT PSID* Owner,
                              OUT PBOOLEAN OwnerDefaulted)
{
   PISECURITY_DESCRIPTOR pSD = (PISECURITY_DESCRIPTOR)SecurityDescriptor;

   PAGED_CODE_RTL();

   if (pSD->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   RtlpQuerySecurityDescriptorPointers(pSD,
                                       Owner,
                                       NULL,
                                       NULL,
                                       NULL);

   *OwnerDefaulted = ((pSD->Control & SE_OWNER_DEFAULTED) ? TRUE : FALSE);

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS NTAPI
RtlSetGroupSecurityDescriptor(IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
                              IN PSID Group,
                              IN BOOLEAN GroupDefaulted)
{
   PISECURITY_DESCRIPTOR pSD = (PISECURITY_DESCRIPTOR)SecurityDescriptor;

   PAGED_CODE_RTL();

   if (pSD->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   if (pSD->Control & SE_SELF_RELATIVE)
   {
      return STATUS_BAD_DESCRIPTOR_FORMAT;
   }

   pSD->Group = Group;
   pSD->Control &= ~(SE_GROUP_DEFAULTED);
   if (GroupDefaulted)
   {
      pSD->Control |= SE_GROUP_DEFAULTED;
   }

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS NTAPI
RtlGetGroupSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                              OUT PSID* Group,
                              OUT PBOOLEAN GroupDefaulted)
{
   PISECURITY_DESCRIPTOR pSD = (PISECURITY_DESCRIPTOR)SecurityDescriptor;

   PAGED_CODE_RTL();

   if (pSD->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   RtlpQuerySecurityDescriptorPointers(pSD,
                                       NULL,
                                       Group,
                                       NULL,
                                       NULL);

   *GroupDefaulted = ((pSD->Control & SE_GROUP_DEFAULTED) ? TRUE : FALSE);

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS NTAPI
RtlMakeSelfRelativeSD(IN PSECURITY_DESCRIPTOR AbsoluteSD,
		      OUT PSECURITY_DESCRIPTOR SelfRelativeSD,
		      IN OUT PULONG BufferLength)
{
   PSID Owner;
   PSID Group;
   PACL Sacl;
   PACL Dacl;
   ULONG OwnerLength;
   ULONG GroupLength;
   ULONG SaclLength;
   ULONG DaclLength;
   ULONG TotalLength;
   ULONG_PTR Current;
   PISECURITY_DESCRIPTOR pAbsSD = (PISECURITY_DESCRIPTOR)AbsoluteSD;
   PISECURITY_DESCRIPTOR_RELATIVE pRelSD = (PISECURITY_DESCRIPTOR_RELATIVE)SelfRelativeSD;

   PAGED_CODE_RTL();

   RtlpQuerySecurityDescriptor(pAbsSD,
                               &Owner,
                               &OwnerLength,
                               &Group,
                               &GroupLength,
                               &Dacl,
                               &DaclLength,
                               &Sacl,
                               &SaclLength);

   TotalLength = sizeof(SECURITY_DESCRIPTOR_RELATIVE) + OwnerLength + GroupLength + SaclLength + DaclLength;
   if (*BufferLength < TotalLength)
   {
      *BufferLength = TotalLength;
      return STATUS_BUFFER_TOO_SMALL;
   }

   RtlZeroMemory(pRelSD,
                 TotalLength);

   pRelSD->Revision = pAbsSD->Revision;
   pRelSD->Sbz1 = pAbsSD->Sbz1;
   pRelSD->Control = pAbsSD->Control | SE_SELF_RELATIVE;

   Current = (ULONG_PTR)(pRelSD + 1);

   if (SaclLength != 0)
   {
      RtlCopyMemory((PVOID)Current,
                    Sacl,
                    SaclLength);
      pRelSD->Sacl = (ULONG)((ULONG_PTR)Current - (ULONG_PTR)pRelSD);
      Current += SaclLength;
   }

   if (DaclLength != 0)
   {
      RtlCopyMemory((PVOID)Current,
                    Dacl,
                    DaclLength);
      pRelSD->Dacl = (ULONG)((ULONG_PTR)Current - (ULONG_PTR)pRelSD);
      Current += DaclLength;
   }

   if (OwnerLength != 0)
   {
      RtlCopyMemory((PVOID)Current,
                    Owner,
                    OwnerLength);
      pRelSD->Owner = (ULONG)((ULONG_PTR)Current - (ULONG_PTR)pRelSD);
      Current += OwnerLength;
   }

   if (GroupLength != 0)
   {
      RtlCopyMemory((PVOID)Current,
                    Group,
                    GroupLength);
      pRelSD->Group = (ULONG)((ULONG_PTR)Current - (ULONG_PTR)pRelSD);
   }

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS NTAPI
RtlAbsoluteToSelfRelativeSD(IN PSECURITY_DESCRIPTOR AbsoluteSecurityDescriptor,
                            IN OUT PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
                            IN PULONG BufferLength)
{
   PISECURITY_DESCRIPTOR pAbsSD = (PISECURITY_DESCRIPTOR)AbsoluteSecurityDescriptor;

   PAGED_CODE_RTL();

   if (pAbsSD->Control & SE_SELF_RELATIVE)
   {
      return STATUS_BAD_DESCRIPTOR_FORMAT;
   }

   return RtlMakeSelfRelativeSD(AbsoluteSecurityDescriptor,
                                SelfRelativeSecurityDescriptor,
                                BufferLength);
}


/*
 * @implemented
 */
NTSTATUS NTAPI
RtlGetControlSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                                OUT PSECURITY_DESCRIPTOR_CONTROL Control,
                                OUT PULONG Revision)
{
   PISECURITY_DESCRIPTOR pSD = (PISECURITY_DESCRIPTOR)SecurityDescriptor;

   PAGED_CODE_RTL();

   *Revision = pSD->Revision;

   if (pSD->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   *Control = pSD->Control;

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS NTAPI
RtlSetControlSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                                IN SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest,
                                IN SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet)
{
   PISECURITY_DESCRIPTOR pSD = (PISECURITY_DESCRIPTOR)SecurityDescriptor;

   PAGED_CODE_RTL();

   if (pSD->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   /* Zero the 'bits of interest' */
   pSD->Control &= ~ControlBitsOfInterest;

   /* Set the 'bits to set' */
   pSD->Control |= (ControlBitsToSet & ControlBitsOfInterest);

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS NTAPI
RtlGetSaclSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                             OUT PBOOLEAN SaclPresent,
                             OUT PACL *Sacl,
                             OUT PBOOLEAN SaclDefaulted)
{
   PISECURITY_DESCRIPTOR pSD = (PISECURITY_DESCRIPTOR)SecurityDescriptor;

   PAGED_CODE_RTL();

   if (pSD->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   if (!(pSD->Control & SE_SACL_PRESENT))
   {
      *SaclPresent = FALSE;
      return STATUS_SUCCESS;
   }
   *SaclPresent = TRUE;

   RtlpQuerySecurityDescriptorPointers(pSD,
                                       NULL,
                                       NULL,
                                       Sacl,
                                       NULL);

   *SaclDefaulted = ((pSD->Control & SE_SACL_DEFAULTED) ? TRUE : FALSE);

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS NTAPI
RtlSetSaclSecurityDescriptor(IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
                             IN BOOLEAN SaclPresent,
                             IN PACL Sacl,
                             IN BOOLEAN SaclDefaulted)
{
   PISECURITY_DESCRIPTOR pSD = (PISECURITY_DESCRIPTOR)SecurityDescriptor;

   PAGED_CODE_RTL();

   if (pSD->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   if (pSD->Control & SE_SELF_RELATIVE)
   {
      return STATUS_BAD_DESCRIPTOR_FORMAT;
   }

   if (!SaclPresent)
   {
      pSD->Control &= ~(SE_SACL_PRESENT);
      return STATUS_SUCCESS;
   }

   pSD->Sacl = Sacl;
   pSD->Control |= SE_SACL_PRESENT;
   pSD->Control &= ~(SE_SACL_DEFAULTED);

   if (SaclDefaulted)
   {
      pSD->Control |= SE_SACL_DEFAULTED;
   }

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS NTAPI
RtlSelfRelativeToAbsoluteSD(IN PSECURITY_DESCRIPTOR SelfRelativeSD,
                            OUT PSECURITY_DESCRIPTOR AbsoluteSD,
                            IN PULONG AbsoluteSDSize,
                            IN PACL Dacl,
                            IN PULONG DaclSize,
                            IN PACL Sacl,
                            IN PULONG SaclSize,
                            IN PSID Owner,
                            IN PULONG OwnerSize,
                            IN PSID PrimaryGroup,
                            IN PULONG PrimaryGroupSize)
{
   PISECURITY_DESCRIPTOR pAbsSD = (PISECURITY_DESCRIPTOR)AbsoluteSD;
   PISECURITY_DESCRIPTOR pRelSD = (PISECURITY_DESCRIPTOR)SelfRelativeSD;
   ULONG OwnerLength;
   ULONG GroupLength;
   ULONG DaclLength;
   ULONG SaclLength;
   PSID pOwner;
   PSID pGroup;
   PACL pDacl;
   PACL pSacl;

   PAGED_CODE_RTL();

   if (pRelSD->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   if (!(pRelSD->Control & SE_SELF_RELATIVE))
   {
      return STATUS_BAD_DESCRIPTOR_FORMAT;
   }

   RtlpQuerySecurityDescriptor (pRelSD,
                                &pOwner,
                                &OwnerLength,
                                &pGroup,
                                &GroupLength,
                                &pDacl,
                                &DaclLength,
                                &pSacl,
                                &SaclLength);

   if (OwnerLength > *OwnerSize ||
       GroupLength > *PrimaryGroupSize ||
       DaclLength > *DaclSize ||
       SaclLength > *SaclSize)
   {
      *OwnerSize = OwnerLength;
      *PrimaryGroupSize = GroupLength;
      *DaclSize = DaclLength;
      *SaclSize = SaclLength;
      return STATUS_BUFFER_TOO_SMALL;
   }

   RtlCopyMemory (Owner, pOwner, OwnerLength);
   RtlCopyMemory (PrimaryGroup, pGroup, GroupLength);
   RtlCopyMemory (Dacl, pDacl, DaclLength);
   RtlCopyMemory (Sacl, pSacl, SaclLength);

   pAbsSD->Revision = pRelSD->Revision;
   pAbsSD->Sbz1 = pRelSD->Sbz1;
   pAbsSD->Control = pRelSD->Control & ~SE_SELF_RELATIVE;
   pAbsSD->Owner = Owner;
   pAbsSD->Group = PrimaryGroup;
   pAbsSD->Dacl = Dacl;
   pAbsSD->Sacl = Sacl;

   *OwnerSize = OwnerLength;
   *PrimaryGroupSize = GroupLength;
   *DaclSize = DaclLength;
   *SaclSize = SaclLength;

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS NTAPI
RtlSelfRelativeToAbsoluteSD2(IN OUT PSECURITY_DESCRIPTOR SelfRelativeSD,
                             OUT PULONG BufferSize)
{
    PISECURITY_DESCRIPTOR pAbsSD = (PISECURITY_DESCRIPTOR)SelfRelativeSD;
    PISECURITY_DESCRIPTOR_RELATIVE pRelSD = (PISECURITY_DESCRIPTOR_RELATIVE)SelfRelativeSD;
#ifdef _WIN64
    PVOID DataStart, DataEnd;
    ULONG DataSize;
    LONG MoveDelta;
    ULONG OwnerLength;
    ULONG GroupLength;
    ULONG DaclLength;
    ULONG SaclLength;
#endif
    PSID pOwner;
    PSID pGroup;
    PACL pDacl;
    PACL pSacl;

    PAGED_CODE_RTL();

    if (SelfRelativeSD == NULL)
    {
        return STATUS_INVALID_PARAMETER_1;
    }
    if (BufferSize == NULL)
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    if (pRelSD->Revision != SECURITY_DESCRIPTOR_REVISION1)
    {
        return STATUS_UNKNOWN_REVISION;
    }
    if (!(pRelSD->Control & SE_SELF_RELATIVE))
    {
        return STATUS_BAD_DESCRIPTOR_FORMAT;
    }

#ifdef _WIN64

    RtlpQuerySecurityDescriptor((PISECURITY_DESCRIPTOR)pRelSD,
                                &pOwner,
                                &OwnerLength,
                                &pGroup,
                                &GroupLength,
                                &pDacl,
                                &DaclLength,
                                &pSacl,
                                &SaclLength);

    /* calculate the start and end of the data area, we simply just move the
       data by the difference between the size of the relative and absolute
       security descriptor structure */
    DataStart = pOwner;
    DataEnd = (PVOID)((ULONG_PTR)pOwner + OwnerLength);
    if (pGroup != NULL)
    {
        if (((ULONG_PTR)pGroup < (ULONG_PTR)DataStart) || DataStart == NULL)
            DataStart = pGroup;
        if (((ULONG_PTR)pGroup + GroupLength > (ULONG_PTR)DataEnd) || DataEnd == NULL)
            DataEnd = (PVOID)((ULONG_PTR)pGroup + GroupLength);
    }
    if (pDacl != NULL)
    {
        if (((ULONG_PTR)pDacl < (ULONG_PTR)DataStart) || DataStart == NULL)
            DataStart = pDacl;
        if (((ULONG_PTR)pDacl + DaclLength > (ULONG_PTR)DataEnd) || DataEnd == NULL)
            DataEnd = (PVOID)((ULONG_PTR)pDacl + DaclLength);
    }
    if (pSacl != NULL)
    {
        if (((ULONG_PTR)pSacl < (ULONG_PTR)DataStart) || DataStart == NULL)
            DataStart = pSacl;
        if (((ULONG_PTR)pSacl + SaclLength > (ULONG_PTR)DataEnd) || DataEnd == NULL)
            DataEnd = (PVOID)((ULONG_PTR)pSacl + SaclLength);
    }

    ASSERT((ULONG_PTR)DataEnd >= (ULONG_PTR)DataStart);

    DataSize = (ULONG)((ULONG_PTR)DataEnd - (ULONG_PTR)DataStart);

    if (*BufferSize < sizeof(SECURITY_DESCRIPTOR) + DataSize)
    {
        *BufferSize = sizeof(SECURITY_DESCRIPTOR) + DataSize;
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (DataSize != 0)
    {
        /* if DataSize != 0 ther must be at least one SID or ACL in the security
           descriptor! Also the data area must be located somewhere after the
           end of the SECURITY_DESCRIPTOR_RELATIVE structure */
        ASSERT(DataStart != NULL);
        ASSERT((ULONG_PTR)DataStart >= (ULONG_PTR)(pRelSD + 1));

        /* it's time to move the data */
        RtlMoveMemory((PVOID)(pAbsSD + 1),
                      DataStart,
                      DataSize);

        MoveDelta = (LONG)((LONG_PTR)(pAbsSD + 1) - (LONG_PTR)DataStart);

        /* adjust the pointers if neccessary */
        if (pOwner != NULL)
            pAbsSD->Owner = (PSID)((LONG_PTR)pOwner + MoveDelta);
        else
            pAbsSD->Owner = NULL;

        if (pGroup != NULL)
            pAbsSD->Group = (PSID)((LONG_PTR)pGroup + MoveDelta);
        else
            pAbsSD->Group = NULL;

        if (pSacl != NULL)
            pAbsSD->Sacl = (PACL)((LONG_PTR)pSacl + MoveDelta);
        else
            pAbsSD->Sacl = NULL;

        if (pDacl != NULL)
            pAbsSD->Dacl = (PACL)((LONG_PTR)pDacl + MoveDelta);
        else
            pAbsSD->Dacl = NULL;
    }
    else
    {
        /* all pointers must be NULL! */
        ASSERT(pOwner == NULL);
        ASSERT(pGroup == NULL);
        ASSERT(pSacl == NULL);
        ASSERT(pDacl == NULL);

        pAbsSD->Owner = NULL;
        pAbsSD->Group = NULL;
        pAbsSD->Sacl = NULL;
        pAbsSD->Dacl = NULL;
    }

    /* clear the self-relative flag */
    pAbsSD->Control &= ~SE_SELF_RELATIVE;

#else

    RtlpQuerySecurityDescriptorPointers((PISECURITY_DESCRIPTOR)pRelSD,
                                        &pOwner,
                                        &pGroup,
                                        &pSacl,
                                        &pDacl);

    /* clear the self-relative flag and simply convert the offsets to pointers */
    pAbsSD->Control &= ~SE_SELF_RELATIVE;
    pAbsSD->Owner = pOwner;
    pAbsSD->Group = pGroup;
    pAbsSD->Sacl = pSacl;
    pAbsSD->Dacl = pDacl;

#endif

    return STATUS_SUCCESS;
}


/*
 * @implemented
 */
BOOLEAN NTAPI
RtlValidRelativeSecurityDescriptor(IN PISECURITY_DESCRIPTOR SecurityDescriptorInput,
                                   IN ULONG SecurityDescriptorLength,
                                   IN SECURITY_INFORMATION RequiredInformation)
{
   PISECURITY_DESCRIPTOR pSD = (PISECURITY_DESCRIPTOR)SecurityDescriptorInput;

   PAGED_CODE_RTL();

   if (SecurityDescriptorLength < sizeof(SECURITY_DESCRIPTOR_RELATIVE) ||
       SecurityDescriptorInput->Revision != SECURITY_DESCRIPTOR_REVISION1 ||
       !(pSD->Control & SE_SELF_RELATIVE))
   {
      return FALSE;
   }

   if (pSD->Owner != 0)
   {
      PSID Owner = (PSID)((ULONG_PTR)pSD->Owner + (ULONG_PTR)pSD);
      if (!RtlValidSid(Owner))
      {
         return FALSE;
      }
   }
   else if (RequiredInformation & OWNER_SECURITY_INFORMATION)
   {
      return FALSE;
   }

   if (pSD->Group != 0)
   {
      PSID Group = (PSID)((ULONG_PTR)pSD->Group + (ULONG_PTR)pSD);
      if (!RtlValidSid(Group))
      {
         return FALSE;
      }
   }
   else if (RequiredInformation & GROUP_SECURITY_INFORMATION)
   {
      return FALSE;
   }

   if (pSD->Control & SE_DACL_PRESENT)
   {
      if (pSD->Dacl != 0 &&
          !RtlValidAcl((PACL)((ULONG_PTR)pSD->Dacl + (ULONG_PTR)pSD)))
      {
         return FALSE;
      }
   }
   else if (RequiredInformation & DACL_SECURITY_INFORMATION)
   {
      return FALSE;
   }

   if (pSD->Control & SE_SACL_PRESENT)
   {
      if (pSD->Sacl != 0 &&
          !RtlValidAcl((PACL)((ULONG_PTR)pSD->Sacl + (ULONG_PTR)pSD)))
      {
         return FALSE;
      }
   }
   else if (RequiredInformation & SACL_SECURITY_INFORMATION)
   {
      return FALSE;
   }

   return TRUE;
}


/*
 * @implemented
 */
BOOLEAN NTAPI
RtlGetSecurityDescriptorRMControl(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                                  OUT PUCHAR RMControl)
{
   PISECURITY_DESCRIPTOR pSD = (PISECURITY_DESCRIPTOR)SecurityDescriptor;

   PAGED_CODE_RTL();

   if (!(pSD->Control & SE_RM_CONTROL_VALID))
   {
      *RMControl = 0;
      return FALSE;
   }

   *RMControl = pSD->Sbz1;

   return TRUE;
}


/*
 * @implemented
 */
VOID NTAPI
RtlSetSecurityDescriptorRMControl(IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
                                  IN PUCHAR RMControl)
{
   PISECURITY_DESCRIPTOR pSD = (PISECURITY_DESCRIPTOR)SecurityDescriptor;

   PAGED_CODE_RTL();

   if (RMControl == NULL)
   {
      pSD->Control &= ~SE_RM_CONTROL_VALID;
      pSD->Sbz1 = 0;
   }
   else
   {
      pSD->Control |= SE_RM_CONTROL_VALID;
      pSD->Sbz1 = *RMControl;
   }
}


/*
 * @implemented
 */
NTSTATUS NTAPI
RtlSetAttributesSecurityDescriptor(IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
                                   IN SECURITY_DESCRIPTOR_CONTROL Control,
                                   OUT PULONG Revision)
{
   PISECURITY_DESCRIPTOR pSD = (PISECURITY_DESCRIPTOR)SecurityDescriptor;

   PAGED_CODE_RTL();

   *Revision = pSD->Revision;

   if (pSD->Revision != SECURITY_DESCRIPTOR_REVISION1)
      return STATUS_UNKNOWN_REVISION;

   Control &=
      ~(SE_OWNER_DEFAULTED | SE_GROUP_DEFAULTED | SE_DACL_PRESENT |
        SE_DACL_DEFAULTED | SE_SACL_PRESENT | SE_SACL_DEFAULTED |
        SE_RM_CONTROL_VALID | SE_SELF_RELATIVE);

   return RtlSetControlSecurityDescriptor(SecurityDescriptor,
                                          Control,
                                          Control);
}

/* EOF */
