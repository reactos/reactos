/* $Id$
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Security descriptor functions
 * FILE:              lib/rtl/sd.c
 * PROGRAMER:         David Welch <welch@cwcom.net>
 * REVISION HISTORY:
 *                 26/07/98: Added stubs for security functions
 */

/* INCLUDES *****************************************************************/

#include "rtl.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/


static VOID
RtlpQuerySecurityDescriptorPointers(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                                    OUT PSID *Owner  OPTIONAL,
                                    OUT PSID *Group  OPTIONAL,
                                    OUT PACL *Sacl  OPTIONAL,
                                    OUT PACL *Dacl  OPTIONAL)
{
  if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
  {
    PSECURITY_DESCRIPTOR_RELATIVE RelSD = (PSECURITY_DESCRIPTOR_RELATIVE)SecurityDescriptor;
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
RtlpQuerySecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
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
NTSTATUS STDCALL
RtlCreateSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
                            ULONG Revision)
{
   PAGED_CODE_RTL();

   if (Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   SecurityDescriptor->Revision = Revision;
   SecurityDescriptor->Sbz1 = 0;
   SecurityDescriptor->Control = 0;
   SecurityDescriptor->Owner = NULL;
   SecurityDescriptor->Group = NULL;
   SecurityDescriptor->Sacl = NULL;
   SecurityDescriptor->Dacl = NULL;

   return STATUS_SUCCESS;
}


NTSTATUS STDCALL
RtlCreateSecurityDescriptorRelative (PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
			             ULONG Revision)
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
ULONG STDCALL
RtlLengthSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor)
{
   PSID Owner, Group;
   PACL Sacl, Dacl;
   ULONG Length = sizeof(SECURITY_DESCRIPTOR);

   PAGED_CODE_RTL();

   RtlpQuerySecurityDescriptorPointers(SecurityDescriptor,
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
NTSTATUS STDCALL
RtlGetDaclSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
                             PBOOLEAN DaclPresent,
                             PACL* Dacl,
                             PBOOLEAN DaclDefaulted)
{
   PAGED_CODE_RTL();

   if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   if (!(SecurityDescriptor->Control & SE_DACL_PRESENT))
   {
      *DaclPresent = FALSE;
      return STATUS_SUCCESS;
   }
   *DaclPresent = TRUE;

   RtlpQuerySecurityDescriptorPointers(SecurityDescriptor,
                                       NULL,
                                       NULL,
                                       NULL,
                                       Dacl);

   *DaclDefaulted = ((SecurityDescriptor->Control & SE_DACL_DEFAULTED) ? TRUE : FALSE);

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlSetDaclSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
                             BOOLEAN DaclPresent,
                             PACL Dacl,
                             BOOLEAN DaclDefaulted)
{
   PAGED_CODE_RTL();

   if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
   {
      return STATUS_BAD_DESCRIPTOR_FORMAT;
   }

   if (!DaclPresent)
   {
      SecurityDescriptor->Control = SecurityDescriptor->Control & ~(SE_DACL_PRESENT);
      return STATUS_SUCCESS;
   }

   SecurityDescriptor->Control = SecurityDescriptor->Control | SE_DACL_PRESENT;
   SecurityDescriptor->Dacl = Dacl;
   SecurityDescriptor->Control = SecurityDescriptor->Control & ~(SE_DACL_DEFAULTED);

   if (DaclDefaulted)
   {
      SecurityDescriptor->Control = SecurityDescriptor->Control | SE_DACL_DEFAULTED;
   }

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
BOOLEAN STDCALL
RtlValidSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor)
{
   PSID Owner, Group;
   PACL Sacl, Dacl;

   PAGED_CODE_RTL();

   if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return FALSE;
   }

   RtlpQuerySecurityDescriptorPointers(SecurityDescriptor,
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
NTSTATUS STDCALL
RtlSetOwnerSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
                              PSID Owner,
                              BOOLEAN OwnerDefaulted)
{
   PAGED_CODE_RTL();

   if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
   {
      return STATUS_BAD_DESCRIPTOR_FORMAT;
   }

   SecurityDescriptor->Owner = Owner;
   SecurityDescriptor->Control = SecurityDescriptor->Control & ~(SE_OWNER_DEFAULTED);

   if (OwnerDefaulted)
   {
      SecurityDescriptor->Control = SecurityDescriptor->Control | SE_OWNER_DEFAULTED;
   }

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlGetOwnerSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
                              PSID* Owner,
                              PBOOLEAN OwnerDefaulted)
{
   PAGED_CODE_RTL();

   if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   RtlpQuerySecurityDescriptorPointers(SecurityDescriptor,
                                       Owner,
                                       NULL,
                                       NULL,
                                       NULL);

   *OwnerDefaulted = ((SecurityDescriptor->Control & SE_OWNER_DEFAULTED) ? TRUE : FALSE);

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlSetGroupSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
                              PSID Group,
                              BOOLEAN GroupDefaulted)
{
   PAGED_CODE_RTL();

   if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
   {
      return STATUS_BAD_DESCRIPTOR_FORMAT;
   }

   SecurityDescriptor->Group = Group;
   SecurityDescriptor->Control = SecurityDescriptor->Control & ~(SE_GROUP_DEFAULTED);
   if (GroupDefaulted)
   {
      SecurityDescriptor->Control = SecurityDescriptor->Control | SE_GROUP_DEFAULTED;
   }

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlGetGroupSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
                              PSID* Group,
                              PBOOLEAN GroupDefaulted)
{
   PAGED_CODE_RTL();

   if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   RtlpQuerySecurityDescriptorPointers(SecurityDescriptor,
                                       NULL,
                                       Group,
                                       NULL,
                                       NULL);

   *GroupDefaulted = ((SecurityDescriptor->Control & SE_GROUP_DEFAULTED) ? TRUE : FALSE);

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlMakeSelfRelativeSD(PSECURITY_DESCRIPTOR AbsSD,
		      PSECURITY_DESCRIPTOR_RELATIVE RelSD,
		      PULONG BufferLength)
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

   PAGED_CODE_RTL();

   RtlpQuerySecurityDescriptor(AbsSD,
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
      return STATUS_BUFFER_TOO_SMALL;
   }

   RtlZeroMemory(RelSD,
                 TotalLength);

   RelSD->Revision = AbsSD->Revision;
   RelSD->Sbz1 = AbsSD->Sbz1;
   RelSD->Control = AbsSD->Control | SE_SELF_RELATIVE;

   Current = (ULONG_PTR)(RelSD + 1);

   if (SaclLength != 0)
   {
      RtlCopyMemory((PVOID)Current,
                    Sacl,
                    SaclLength);
      RelSD->Sacl = (ULONG)((ULONG_PTR)Current - (ULONG_PTR)RelSD);
      Current += SaclLength;
   }

   if (DaclLength != 0)
   {
      RtlCopyMemory((PVOID)Current,
                    Dacl,
                    DaclLength);
      RelSD->Dacl = (ULONG)((ULONG_PTR)Current - (ULONG_PTR)RelSD);
      Current += DaclLength;
   }

   if (OwnerLength != 0)
   {
      RtlCopyMemory((PVOID)Current,
                    Owner,
                    OwnerLength);
      RelSD->Owner = (ULONG)((ULONG_PTR)Current - (ULONG_PTR)RelSD);
      Current += OwnerLength;
   }

   if (GroupLength != 0)
   {
      RtlCopyMemory((PVOID)Current,
                    Group,
                    GroupLength);
      RelSD->Group = (ULONG)((ULONG_PTR)Current - (ULONG_PTR)RelSD);
   }

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlAbsoluteToSelfRelativeSD(PSECURITY_DESCRIPTOR AbsSD,
                            PSECURITY_DESCRIPTOR_RELATIVE RelSD,
                            PULONG BufferLength)
{
   PAGED_CODE_RTL();

   if (AbsSD->Control & SE_SELF_RELATIVE)
   {
      return STATUS_BAD_DESCRIPTOR_FORMAT;
   }

   return RtlMakeSelfRelativeSD(AbsSD, RelSD, BufferLength);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlGetControlSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
                                PSECURITY_DESCRIPTOR_CONTROL Control,
                                PULONG Revision)
{
   PAGED_CODE_RTL();

   *Revision = SecurityDescriptor->Revision;

   if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   *Control = SecurityDescriptor->Control;

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlSetControlSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                                IN SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest,
                                IN SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet)
{
  PAGED_CODE_RTL();

  if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION1)
  {
    return STATUS_UNKNOWN_REVISION;
  }

  /* Zero the 'bits of interest' */
  SecurityDescriptor->Control &= ~ControlBitsOfInterest;

  /* Set the 'bits to set' */
  SecurityDescriptor->Control |= (ControlBitsToSet & ControlBitsOfInterest);

  return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlGetSaclSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
                             PBOOLEAN SaclPresent,
                             PACL *Sacl,
                             PBOOLEAN SaclDefaulted)
{
   PAGED_CODE_RTL();

   if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   if (!(SecurityDescriptor->Control & SE_SACL_PRESENT))
   {
      *SaclPresent = FALSE;
      return STATUS_SUCCESS;
   }
   *SaclPresent = TRUE;

   RtlpQuerySecurityDescriptorPointers(SecurityDescriptor,
                                       NULL,
                                       NULL,
                                       Sacl,
                                       NULL);

   *SaclDefaulted = ((SecurityDescriptor->Control & SE_SACL_DEFAULTED) ? TRUE : FALSE);

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlSetSaclSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
                             BOOLEAN SaclPresent,
                             PACL Sacl,
                             BOOLEAN SaclDefaulted)
{
   PAGED_CODE_RTL();

   if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
   {
      return STATUS_BAD_DESCRIPTOR_FORMAT;
   }

   if (!SaclPresent)
   {
      SecurityDescriptor->Control = SecurityDescriptor->Control & ~(SE_SACL_PRESENT);
      return STATUS_SUCCESS;
   }

   SecurityDescriptor->Control = SecurityDescriptor->Control | SE_SACL_PRESENT;
   SecurityDescriptor->Sacl = Sacl;
   SecurityDescriptor->Control = SecurityDescriptor->Control & ~(SE_SACL_DEFAULTED);

   if (SaclDefaulted)
   {
      SecurityDescriptor->Control = SecurityDescriptor->Control | SE_SACL_DEFAULTED;
   }

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlSelfRelativeToAbsoluteSD(PSECURITY_DESCRIPTOR_RELATIVE RelSD,
                            PSECURITY_DESCRIPTOR AbsSD,
                            PDWORD AbsSDSize,
                            PACL Dacl,
                            PDWORD DaclSize,
                            PACL Sacl,
                            PDWORD SaclSize,
                            PSID Owner,
                            PDWORD OwnerSize,
                            PSID Group,
                            PDWORD GroupSize)
{
   ULONG OwnerLength;
   ULONG GroupLength;
   ULONG DaclLength;
   ULONG SaclLength;
   PSID pOwner;
   PSID pGroup;
   PACL pDacl;
   PACL pSacl;

   PAGED_CODE_RTL();

   if (RelSD->Revision != SECURITY_DESCRIPTOR_REVISION1)
   {
      return STATUS_UNKNOWN_REVISION;
   }

   if (!(RelSD->Control & SE_SELF_RELATIVE))
   {
      return STATUS_BAD_DESCRIPTOR_FORMAT;
   }

   RtlpQuerySecurityDescriptor ((PSECURITY_DESCRIPTOR)RelSD,
                                &pOwner,
                                &OwnerLength,
                                &pGroup,
                                &GroupLength,
                                &pDacl,
                                &DaclLength,
                                &pSacl,
                                &SaclLength);

   if (OwnerLength > *OwnerSize ||
       GroupLength > *GroupSize ||
       DaclLength > *DaclSize ||
       SaclLength > *SaclSize)
   {
      return STATUS_BUFFER_TOO_SMALL;
   }

   RtlCopyMemory (Owner, pOwner, OwnerLength);
   RtlCopyMemory (Group, pGroup, GroupLength);
   RtlCopyMemory (Dacl, pDacl, DaclLength);
   RtlCopyMemory (Sacl, pSacl, SaclLength);

   AbsSD->Revision = RelSD->Revision;
   AbsSD->Sbz1 = RelSD->Sbz1;
   AbsSD->Control = RelSD->Control & ~SE_SELF_RELATIVE;
   AbsSD->Owner = Owner;
   AbsSD->Group = Group;
   AbsSD->Dacl = Dacl;
   AbsSD->Sacl = Sacl;

   *OwnerSize = OwnerLength;
   *GroupSize = GroupLength;
   *DaclSize = DaclLength;
   *SaclSize = SaclLength;

   return STATUS_SUCCESS;
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
RtlSelfRelativeToAbsoluteSD2(PSECURITY_DESCRIPTOR_RELATIVE SelfRelativeSecurityDescriptor,
                             PULONG BufferSize)
{
   UNIMPLEMENTED;
   return STATUS_NOT_IMPLEMENTED;
}


/*
 * @implemented
 */
BOOLEAN STDCALL
RtlValidRelativeSecurityDescriptor(IN PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptorInput,
                                   IN ULONG SecurityDescriptorLength,
                                   IN SECURITY_INFORMATION RequiredInformation)
{
   PAGED_CODE_RTL();

   if (SecurityDescriptorLength < sizeof(SECURITY_DESCRIPTOR_RELATIVE) ||
       SecurityDescriptorInput->Revision != SECURITY_DESCRIPTOR_REVISION1 ||
       !(SecurityDescriptorInput->Control & SE_SELF_RELATIVE))
   {
      return FALSE;
   }

   if (SecurityDescriptorInput->Owner != 0)
   {
      PSID Owner = (PSID)((ULONG_PTR)SecurityDescriptorInput->Owner + (ULONG_PTR)SecurityDescriptorInput);
      if (!RtlValidSid(Owner))
      {
         return FALSE;
      }
   }
   else if (RequiredInformation & OWNER_SECURITY_INFORMATION)
   {
      return FALSE;
   }

   if (SecurityDescriptorInput->Group != 0)
   {
      PSID Group = (PSID)((ULONG_PTR)SecurityDescriptorInput->Group + (ULONG_PTR)SecurityDescriptorInput);
      if (!RtlValidSid(Group))
      {
         return FALSE;
      }
   }
   else if (RequiredInformation & GROUP_SECURITY_INFORMATION)
   {
      return FALSE;
   }

   if (SecurityDescriptorInput->Control & SE_DACL_PRESENT)
   {
      if (SecurityDescriptorInput->Dacl != 0 &&
          !RtlValidAcl((PACL)((ULONG_PTR)SecurityDescriptorInput->Dacl + (ULONG_PTR)SecurityDescriptorInput)))
      {
         return FALSE;
      }
   }
   else if (RequiredInformation & DACL_SECURITY_INFORMATION)
   {
      return FALSE;
   }

   if (SecurityDescriptorInput->Control & SE_SACL_PRESENT)
   {
      if (SecurityDescriptorInput->Sacl != 0 &&
          !RtlValidAcl((PACL)((ULONG_PTR)SecurityDescriptorInput->Sacl + (ULONG_PTR)SecurityDescriptorInput)))
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
BOOLEAN STDCALL
RtlGetSecurityDescriptorRMControl(PSECURITY_DESCRIPTOR SecurityDescriptor,
                                  PUCHAR RMControl)
{
  PAGED_CODE_RTL();

  if (!(SecurityDescriptor->Control & SE_RM_CONTROL_VALID))
  {
    *RMControl = 0;
    return FALSE;
  }

  *RMControl = SecurityDescriptor->Sbz1;

  return TRUE;
}


/*
 * @implemented
 */
VOID STDCALL
RtlSetSecurityDescriptorRMControl(PSECURITY_DESCRIPTOR SecurityDescriptor,
                                  PUCHAR RMControl)
{
  PAGED_CODE_RTL();

  if (RMControl == NULL)
  {
    SecurityDescriptor->Control &= ~SE_RM_CONTROL_VALID;
    SecurityDescriptor->Sbz1 = 0;
  }
  else
  {
    SecurityDescriptor->Control |= SE_RM_CONTROL_VALID;
    SecurityDescriptor->Sbz1 = *RMControl;
  }
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlSetAttributesSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                                   IN SECURITY_DESCRIPTOR_CONTROL Control,
                                   OUT PULONG Revision)
{
  PAGED_CODE_RTL();

  *Revision = SecurityDescriptor->Revision;

  if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION1)
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
