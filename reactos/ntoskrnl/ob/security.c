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

NTSTATUS STDCALL
ObAssignSecurity(IN PACCESS_STATE AccessState,
		 IN PSECURITY_DESCRIPTOR SecurityDescriptor,
		 IN PVOID Object,
		 IN POBJECT_TYPE Type)
{
  UNIMPLEMENTED;
}


NTSTATUS STDCALL
ObGetObjectSecurity(IN PVOID Object,
		    OUT PSECURITY_DESCRIPTOR *SecurityDescriptor,
		    OUT PBOOLEAN MemoryAllocated)
{
  UNIMPLEMENTED;
}


VOID STDCALL
ObReleaseObjectSecurity(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
			IN BOOLEAN MemoryAllocated)
{
  UNIMPLEMENTED;
}


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
      return(Status);
    }

  Header = BODY_TO_HEADER(Object);
  if (Header->ObjectType != NULL &&
      Header->ObjectType->Security != NULL)
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
      Status = STATUS_NOT_IMPLEMENTED;
    }

  ObDereferenceObject(Object);

  return(Status);
}


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
