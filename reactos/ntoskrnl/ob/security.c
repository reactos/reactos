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

#include <ntoskrnl.h>

#define NDEBUG
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
  IN SECURITY_INFORMATION  SecurityInformation,
  OUT PSECURITY_DESCRIPTOR  SecurityDescriptor,
  IN ULONG  SecurityDescriptorLength,
  OUT PULONG  ReturnLength)
{
   NTSTATUS Status;
   PVOID Object;
   OBJECT_HANDLE_INFORMATION HandleInfo;
   POBJECT_HEADER Header;
   
   Status = ObReferenceObjectByHandle(Handle,
				      0,
				      NULL,
				      KeGetPreviousMode(),
				      &Object,
				      &HandleInfo);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
  
   Header = BODY_TO_HEADER(Object);
   if (Header->ObjectType != NULL &&
       Header->ObjectType->Security != NULL)
     {
	Status = Header->ObjectType->Security(Object,
					      SecurityInformation,
					      SecurityDescriptor,
					      &SecurityDescriptorLength);
	*ReturnLength = SecurityDescriptorLength;
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
  UNIMPLEMENTED;
}

/* EOF */
