/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Security manager
 * FILE:              ntoskrnl/ob/security.c
 * PROGRAMER:         ?
 * REVISION HISTORY:
 *                 26/07/98: Added stubs for security functions
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ob.h>

#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

/*
 * @implemented
 */
NTSTATUS STDCALL
ObAssignSecurity(IN PACCESS_STATE AccessState,
		 IN PSECURITY_DESCRIPTOR SecurityDescriptor,
		 IN PVOID Object,
		 IN POBJECT_TYPE Type)
{
  PSECURITY_DESCRIPTOR NewDescriptor;
  NTSTATUS Status;

  /* Build the new security descriptor */
  Status = SeAssignSecurity(SecurityDescriptor,
			    AccessState->SecurityDescriptor,
			    &NewDescriptor,
			    (Type == ObDirectoryType),
			    &AccessState->SubjectSecurityContext,
			    Type->Mapping,
			    PagedPool);
  if (!NT_SUCCESS(Status))
    return Status;

  if (Type->Security != NULL)
    {
      /* Call the security method */
      Status = Type->Security(Object,
			      AssignSecurityDescriptor,
			      0,
			      NewDescriptor,
			      NULL);
    }
  else
    {
      /* Assign the security descriptor to the object header */
      Status = ObpAddSecurityDescriptor(NewDescriptor,
					&(BODY_TO_HEADER(Object)->SecurityDescriptor));
    }

  /* Release the new security descriptor */
  SeDeassignSecurity(&NewDescriptor);

  return Status;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
ObGetObjectSecurity(IN PVOID Object,
		    OUT PSECURITY_DESCRIPTOR *SecurityDescriptor,
		    OUT PBOOLEAN MemoryAllocated)
{
  POBJECT_HEADER Header;
  ULONG Length;
  NTSTATUS Status;

  Header = BODY_TO_HEADER(Object);
  if (Header->ObjectType == NULL)
    return STATUS_UNSUCCESSFUL;

  if (Header->ObjectType->Security == NULL)
    {
      ObpReferenceCachedSecurityDescriptor(Header->SecurityDescriptor);
      *SecurityDescriptor = Header->SecurityDescriptor;
      *MemoryAllocated = FALSE;
      return STATUS_SUCCESS;
    }

  /* Get the security descriptor size */
  Length = 0;
  Status = Header->ObjectType->Security(Object,
					QuerySecurityDescriptor,
					OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
					DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION,
					NULL,
					&Length);
  if (Status != STATUS_BUFFER_TOO_SMALL)
    return Status;

  /* Allocate security descriptor */
  *SecurityDescriptor = ExAllocatePool(NonPagedPool,
				       Length);
  if (*SecurityDescriptor == NULL)
    return STATUS_INSUFFICIENT_RESOURCES;

  /* Query security descriptor */
  Status = Header->ObjectType->Security(Object,
					QuerySecurityDescriptor,
					OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
					DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION,
					*SecurityDescriptor,
					&Length);
  if (!NT_SUCCESS(Status))
    {
      ExFreePool(*SecurityDescriptor);
      return Status;
    }

  *MemoryAllocated = TRUE;

  return STATUS_SUCCESS;
}


/*
 * @implemented
 */
VOID STDCALL
ObReleaseObjectSecurity(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
			IN BOOLEAN MemoryAllocated)
{
  if (SecurityDescriptor == NULL)
    return;

  if (MemoryAllocated)
    {
      ExFreePool(SecurityDescriptor);
    }
  else
    {
      ObpDereferenceCachedSecurityDescriptor(SecurityDescriptor);
    }
}


/*
 * @implemented
 */
NTSTATUS STDCALL
NtQuerySecurityObject(IN HANDLE Handle,
		      IN SECURITY_INFORMATION SecurityInformation,
		      OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
		      IN ULONG Length,
		      OUT PULONG ResultLength)
{
  POBJECT_HEADER Header;
  PVOID Object;
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
  NTSTATUS Status;

  Status = ObReferenceObjectByHandle(Handle,
				     (SecurityInformation & SACL_SECURITY_INFORMATION) ? ACCESS_SYSTEM_SECURITY : 0,
				     NULL,
				     KeGetPreviousMode(),
				     &Object,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  Header = BODY_TO_HEADER(Object);
  if (Header->ObjectType == NULL)
    {
      ObDereferenceObject(Object);
      return STATUS_UNSUCCESSFUL;
    }

  if (Header->ObjectType->Security != NULL)
    {
      Status = Header->ObjectType->Security(Object,
					    QuerySecurityDescriptor,
					    SecurityInformation,
					    SecurityDescriptor,
					    &Length);
      *ResultLength = Length;
    }
  else
    {
      ObjectSd = Header->SecurityDescriptor;

      if (ObjectSd != NULL)
	{
	  Control = SE_SELF_RELATIVE;
	  if ((SecurityInformation & OWNER_SECURITY_INFORMATION) &&
	      (ObjectSd->Owner != NULL))
	    {
	      Owner = (PSID)((ULONG_PTR)ObjectSd->Owner + (ULONG_PTR)ObjectSd);
	      OwnerLength = ROUND_UP(RtlLengthSid(Owner), 4);
	      Control |= (ObjectSd->Control & SE_OWNER_DEFAULTED);
	    }

	  if ((SecurityInformation & GROUP_SECURITY_INFORMATION) &&
	      (ObjectSd->Group != NULL))
	    {
	      Group = (PSID)((ULONG_PTR)ObjectSd->Group + (ULONG_PTR)ObjectSd);
	      GroupLength = ROUND_UP(RtlLengthSid(Group), 4);
	      Control |= (ObjectSd->Control & SE_GROUP_DEFAULTED);
	    }

	  if ((SecurityInformation & DACL_SECURITY_INFORMATION) &&
	      (ObjectSd->Control & SE_DACL_PRESENT))
	    {
	      if (ObjectSd->Dacl != NULL)
		{
		  Dacl = (PACL)((ULONG_PTR)ObjectSd->Dacl + (ULONG_PTR)ObjectSd);
		  DaclLength = ROUND_UP((ULONG)Dacl->AclSize, 4);
		}
	      Control |= (ObjectSd->Control & (SE_DACL_DEFAULTED | SE_DACL_PRESENT));
	    }

	  if ((SecurityInformation & SACL_SECURITY_INFORMATION) &&
	      (ObjectSd->Control & SE_SACL_PRESENT))
	    {
	      if (ObjectSd->Sacl != NULL)
		{
		  Sacl = (PACL)((ULONG_PTR)ObjectSd->Sacl + (ULONG_PTR)ObjectSd);
		  SaclLength = ROUND_UP(Sacl->AclSize, 4);
		}
	      Control |= (ObjectSd->Control & (SE_SACL_DEFAULTED | SE_SACL_PRESENT));
	    }

	  *ResultLength = OwnerLength + GroupLength +
			  DaclLength + SaclLength + sizeof(SECURITY_DESCRIPTOR);
	  if (Length >= *ResultLength)
	    {
	      RtlCreateSecurityDescriptor(SecurityDescriptor,
					  SECURITY_DESCRIPTOR_REVISION1);
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

	      Status = STATUS_SUCCESS;
	    }
	  else
	    {
	      Status = STATUS_BUFFER_TOO_SMALL;
	    }
	}
      else
	{
	  *ResultLength = 0;
	  Status = STATUS_UNSUCCESSFUL;
	}
    }

  ObDereferenceObject(Object);

  return Status;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
NtSetSecurityObject(IN HANDLE Handle,
		    IN SECURITY_INFORMATION SecurityInformation,
		    IN PSECURITY_DESCRIPTOR SecurityDescriptor)
{
  PSECURITY_DESCRIPTOR ObjectSd;
  PSECURITY_DESCRIPTOR NewSd;
  POBJECT_HEADER Header;
  PVOID Object;
  PSID Owner;
  PSID Group;
  PACL Dacl;
  PACL Sacl;
  ULONG OwnerLength = 0;
  ULONG GroupLength = 0;
  ULONG DaclLength = 0;
  ULONG SaclLength = 0;
  ULONG Control = 0;
  ULONG_PTR Current;
  NTSTATUS Status;

  Status = ObReferenceObjectByHandle(Handle,
				     (SecurityInformation & SACL_SECURITY_INFORMATION) ? ACCESS_SYSTEM_SECURITY : 0,
				     NULL,
				     KeGetPreviousMode(),
				     &Object,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  Header = BODY_TO_HEADER(Object);
  if (Header->ObjectType != NULL)
    {
      ObDereferenceObject(Object);
      return STATUS_UNSUCCESSFUL;
    }

  if (Header->ObjectType->Security != NULL)
    {
      Status = Header->ObjectType->Security(Object,
					    SetSecurityDescriptor,
					    SecurityInformation,
					    SecurityDescriptor,
					    NULL);
    }
  else
    {
      ObjectSd = Header->SecurityDescriptor;

      /* Get owner and owner size */
      if (SecurityInformation & OWNER_SECURITY_INFORMATION)
	{
	  if (SecurityDescriptor->Owner != NULL)
	    {
	      Owner = (PSID)((ULONG_PTR)SecurityDescriptor->Owner + (ULONG_PTR)SecurityDescriptor);
	      OwnerLength = ROUND_UP(RtlLengthSid(Owner), 4);
	    }
	  Control |= (SecurityDescriptor->Control & SE_OWNER_DEFAULTED);
	}
      else
	{
	  if (ObjectSd->Owner != NULL)
	    {
	      Owner = (PSID)((ULONG_PTR)ObjectSd->Owner + (ULONG_PTR)ObjectSd);
	      OwnerLength = ROUND_UP(RtlLengthSid(Owner), 4);
	    }
	  Control |= (ObjectSd->Control & SE_OWNER_DEFAULTED);
	}

      /* Get group and group size */
      if (SecurityInformation & GROUP_SECURITY_INFORMATION)
	{
	  if (SecurityDescriptor->Group != NULL)
	    {
	      Group = (PSID)((ULONG_PTR)SecurityDescriptor->Group + (ULONG_PTR)SecurityDescriptor);
	      GroupLength = ROUND_UP(RtlLengthSid(Group), 4);
	    }
	  Control |= (SecurityDescriptor->Control & SE_GROUP_DEFAULTED);
	}
      else
	{
	  if (ObjectSd->Group != NULL)
	    {
	      Group = (PSID)((ULONG_PTR)ObjectSd->Group + (ULONG_PTR)ObjectSd);
	      GroupLength = ROUND_UP(RtlLengthSid(Group), 4);
	    }
	  Control |= (ObjectSd->Control & SE_GROUP_DEFAULTED);
	}

      /* Get DACL and DACL size */
      if (SecurityInformation & DACL_SECURITY_INFORMATION)
	{
	  if ((SecurityDescriptor->Control & SE_DACL_PRESENT) &&
	      (SecurityDescriptor->Dacl != NULL))
	    {
	      Dacl = (PACL)((ULONG_PTR)SecurityDescriptor->Dacl + (ULONG_PTR)SecurityDescriptor);
	      DaclLength = ROUND_UP((ULONG)Dacl->AclSize, 4);
	    }
	  Control |= (SecurityDescriptor->Control & (SE_DACL_DEFAULTED | SE_DACL_PRESENT));
	}
      else
	{
	  if ((ObjectSd->Control & SE_DACL_PRESENT) &&
	      (ObjectSd->Dacl != NULL))
	    {
	      Dacl = (PACL)((ULONG_PTR)ObjectSd->Dacl + (ULONG_PTR)ObjectSd);
	      DaclLength = ROUND_UP((ULONG)Dacl->AclSize, 4);
	    }
	  Control |= (ObjectSd->Control & (SE_DACL_DEFAULTED | SE_DACL_PRESENT));
	}

      /* Get SACL and SACL size */
      if (SecurityInformation & SACL_SECURITY_INFORMATION)
	{
	  if ((SecurityDescriptor->Control & SE_SACL_PRESENT) &&
	      (SecurityDescriptor->Sacl != NULL))
	    {
	      Sacl = (PACL)((ULONG_PTR)SecurityDescriptor->Sacl + (ULONG_PTR)SecurityDescriptor);
	      SaclLength = ROUND_UP((ULONG)Sacl->AclSize, 4);
	    }
	  Control |= (SecurityDescriptor->Control & (SE_SACL_DEFAULTED | SE_SACL_PRESENT));
	}
      else
	{
	  if ((ObjectSd->Control & SE_SACL_PRESENT) &&
	      (ObjectSd->Sacl != NULL))
	    {
	      Sacl = (PACL)((ULONG_PTR)ObjectSd->Sacl + (ULONG_PTR)ObjectSd);
	      SaclLength = ROUND_UP((ULONG)Sacl->AclSize, 4);
	    }
	  Control |= (ObjectSd->Control & (SE_SACL_DEFAULTED | SE_SACL_PRESENT));
	}

      NewSd = ExAllocatePool(NonPagedPool,
			     sizeof(SECURITY_DESCRIPTOR) + OwnerLength + GroupLength +
			     DaclLength + SaclLength);
      if (NewSd == NULL)
	{
	  ObDereferenceObject(Object);
	  return STATUS_INSUFFICIENT_RESOURCES;
	}

      RtlCreateSecurityDescriptor(NewSd,
				  SECURITY_DESCRIPTOR_REVISION1);
      NewSd->Control = Control;

      Current = (ULONG_PTR)NewSd + sizeof(SECURITY_DESCRIPTOR);

      if (OwnerLength != 0)
	{
	  RtlCopyMemory((PVOID)Current,
			Owner,
			OwnerLength);
	  NewSd->Owner = (PSID)(Current - (ULONG_PTR)NewSd);
	  Current += OwnerLength;
	}

      if (GroupLength != 0)
	{
	  RtlCopyMemory((PVOID)Current,
			Group,
			GroupLength);
	  NewSd->Group = (PSID)(Current - (ULONG_PTR)NewSd);
	  Current += GroupLength;
	}

      if (DaclLength != 0)
	{
	  RtlCopyMemory((PVOID)Current,
			Dacl,
			DaclLength);
	  NewSd->Dacl = (PACL)(Current - (ULONG_PTR)NewSd);
	  Current += DaclLength;
	}

      if (SaclLength != 0)
	{
	  RtlCopyMemory((PVOID)Current,
			Sacl,
			SaclLength);
	  NewSd->Sacl = (PACL)(Current - (ULONG_PTR)NewSd);
	  Current += SaclLength;
	}

      /* Add the new SD */
      Status = ObpAddSecurityDescriptor(NewSd,
					&Header->SecurityDescriptor);
      if (NT_SUCCESS(Status))
	{
	  /* Remove the old security descriptor */
	  ObpRemoveSecurityDescriptor(ObjectSd);
	}
      else
	{
	  /* Restore the old security descriptor */
	  Header->SecurityDescriptor = ObjectSd;
	}

      ExFreePool(NewSd);
    }

  ObDereferenceObject(Object);

  return Status;
}

/* EOF */
