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
  NTSTATUS Status;

  Status = ObReferenceObjectByHandle(Handle,
				     0,
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
    return STATUS_UNSUCCESSFUL;

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
      if (Header->SecurityDescriptor != NULL)
	{
	  /* FIXME: Use SecurityInformation */
	  *ResultLength = RtlLengthSecurityDescriptor(Header->SecurityDescriptor);
	  if (Length >= *ResultLength)
	    {
	      RtlCopyMemory(SecurityDescriptor,
			    Header->SecurityDescriptor,
			    *ResultLength);

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
 * @unimplemented
 */
NTSTATUS STDCALL
NtSetSecurityObject(IN HANDLE Handle,
		    IN SECURITY_INFORMATION SecurityInformation,
		    IN PSECURITY_DESCRIPTOR SecurityDescriptor)
{
  POBJECT_HEADER Header;
  PVOID Object;
  NTSTATUS Status;

  Status = ObReferenceObjectByHandle(Handle,
				     0,
				     NULL,
				     KeGetPreviousMode(),
				     &Object,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  Header = BODY_TO_HEADER(Object);
  if (Header->ObjectType != NULL &&
      Header->ObjectType->Security != NULL)
    {
      Status = Header->ObjectType->Security(Object,
					    SetSecurityDescriptor,
					    SecurityInformation,
					    SecurityDescriptor,
					    NULL);
    }
  else
    {
      Status = STATUS_NOT_IMPLEMENTED;
    }

  ObDereferenceObject(Object);

  return(Status);
}

/* EOF */
