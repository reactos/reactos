/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ob/security.c
 * PURPOSE:         Security manager
 *
 * PROGRAMERS:      No programmer listed.
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
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

  PAGED_CODE();

  /* Build the new security descriptor */
  Status = SeAssignSecurity(SecurityDescriptor,
			    AccessState->SecurityDescriptor,
			    &NewDescriptor,
			    (Type == ObDirectoryType),
			    &AccessState->SubjectSecurityContext,
			    &Type->TypeInfo.GenericMapping,
			    PagedPool);
  if (!NT_SUCCESS(Status))
    return Status;

      /* Call the security method */
      Status = Type->TypeInfo.SecurityProcedure(Object,
			      AssignSecurityDescriptor,
			      0,
			      NewDescriptor,
			      NULL,
                  NULL,
                  NonPagedPool,
                  NULL);

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

  PAGED_CODE();

  Header = BODY_TO_HEADER(Object);
  if (Header->Type == NULL)
    return STATUS_UNSUCCESSFUL;

  if (Header->Type->TypeInfo.SecurityProcedure == NULL)
    {
      ObpReferenceCachedSecurityDescriptor(Header->SecurityDescriptor);
      *SecurityDescriptor = Header->SecurityDescriptor;
      *MemoryAllocated = FALSE;
      return STATUS_SUCCESS;
    }

  /* Get the security descriptor size */
  Length = 0;
  Status = Header->Type->TypeInfo.SecurityProcedure(Object,
					QuerySecurityDescriptor,
					OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
					DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION,
					NULL,
					&Length,
                    NULL,
                    NonPagedPool,
                    NULL);
  if (Status != STATUS_BUFFER_TOO_SMALL)
    return Status;

  /* Allocate security descriptor */
  *SecurityDescriptor = ExAllocatePool(NonPagedPool,
				       Length);
  if (*SecurityDescriptor == NULL)
    return STATUS_INSUFFICIENT_RESOURCES;

  /* Query security descriptor */
  Status = Header->Type->TypeInfo.SecurityProcedure(Object,
					QuerySecurityDescriptor,
					OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
					DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION,
					*SecurityDescriptor,
					&Length,
                    NULL,
                    NonPagedPool,
                    NULL);
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
  PAGED_CODE();

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

  PAGED_CODE();

  DPRINT("NtQuerySecurityObject() called\n");

  Status = ObReferenceObjectByHandle(Handle,
				     (SecurityInformation & SACL_SECURITY_INFORMATION) ? ACCESS_SYSTEM_SECURITY : 0,
				     NULL,
				     KeGetPreviousMode(),
				     &Object,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ObReferenceObjectByHandle() failed (Status %lx)\n", Status);
      return Status;
    }

  Header = BODY_TO_HEADER(Object);
  if (Header->Type == NULL)
    {
      DPRINT1("Invalid object type\n");
      ObDereferenceObject(Object);
      return STATUS_UNSUCCESSFUL;
    }

      *ResultLength = Length;
      Status = Header->Type->TypeInfo.SecurityProcedure(Object,
					    QuerySecurityDescriptor,
					    SecurityInformation,
					    SecurityDescriptor,
					    ResultLength,
                        NULL,
                        NonPagedPool,
                        NULL);

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
  POBJECT_HEADER Header;
  PVOID Object;
  NTSTATUS Status;

  PAGED_CODE();

  DPRINT("NtSetSecurityObject() called\n");

  Status = ObReferenceObjectByHandle(Handle,
				     (SecurityInformation & SACL_SECURITY_INFORMATION) ? ACCESS_SYSTEM_SECURITY : 0,
				     NULL,
				     KeGetPreviousMode(),
				     &Object,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ObReferenceObjectByHandle() failed (Status %lx)\n", Status);
      return Status;
    }

  Header = BODY_TO_HEADER(Object);
  if (Header->Type == NULL)
    {
      DPRINT1("Invalid object type\n");
      ObDereferenceObject(Object);
      return STATUS_UNSUCCESSFUL;
    }

      Status = Header->Type->TypeInfo.SecurityProcedure(Object,
					    SetSecurityDescriptor,
					    SecurityInformation,
					    SecurityDescriptor,
					    NULL,
                        NULL,
                        NonPagedPool,
                        NULL);

  ObDereferenceObject(Object);

  return Status;
}

/* EOF */
