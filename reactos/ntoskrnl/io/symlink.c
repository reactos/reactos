/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/symlink.c
 * PURPOSE:         Implements symbolic links
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>


/* FUNCTIONS ****************************************************************/

/**********************************************************************
 * NAME							EXPORTED
 *	IoCreateSymbolicLink
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * @implemented
 */
NTSTATUS STDCALL
IoCreateSymbolicLink(PUNICODE_STRING SymbolicLinkName,
		     PUNICODE_STRING DeviceName)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  HANDLE Handle;
  NTSTATUS Status;

  ASSERT_IRQL(PASSIVE_LEVEL);

  DPRINT("IoCreateSymbolicLink(SymbolicLinkName %wZ, DeviceName %wZ)\n",
	 SymbolicLinkName,
	 DeviceName);

  InitializeObjectAttributes(&ObjectAttributes,
			     SymbolicLinkName,
			     OBJ_PERMANENT,
			     NULL,
			     SePublicDefaultSd);

  Status = ZwCreateSymbolicLinkObject(&Handle,
				      SYMBOLIC_LINK_ALL_ACCESS,
				      &ObjectAttributes,
				      DeviceName);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ZwCreateSymbolicLinkObject() failed (Status %lx)\n", Status);
      return(Status);
    }

  ZwClose(Handle);

  return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							EXPORTED
 *	IoCreateUnprotectedSymbolicLink
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * @implemented
 */
NTSTATUS STDCALL
IoCreateUnprotectedSymbolicLink(PUNICODE_STRING SymbolicLinkName,
				PUNICODE_STRING DeviceName)
{
  SECURITY_DESCRIPTOR SecurityDescriptor;
  OBJECT_ATTRIBUTES ObjectAttributes;
  HANDLE Handle;
  NTSTATUS Status;

  ASSERT_IRQL(PASSIVE_LEVEL);

  DPRINT("IoCreateUnprotectedSymbolicLink(SymbolicLinkName %wZ, DeviceName %wZ)\n",
	 SymbolicLinkName,
	 DeviceName);

  Status = RtlCreateSecurityDescriptor(&SecurityDescriptor,
				       SECURITY_DESCRIPTOR_REVISION);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("RtlCreateSecurityDescriptor() failed (Status %lx)\n", Status);
      return(Status);
    }

  Status = RtlSetDaclSecurityDescriptor(&SecurityDescriptor,
					TRUE,
					NULL,
					TRUE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("RtlSetDaclSecurityDescriptor() failed (Status %lx)\n", Status);
      return(Status);
    }

  InitializeObjectAttributes(&ObjectAttributes,
			     SymbolicLinkName,
			     OBJ_PERMANENT,
			     NULL,
			     &SecurityDescriptor);

  Status = ZwCreateSymbolicLinkObject(&Handle,
				      SYMBOLIC_LINK_ALL_ACCESS,
				      &ObjectAttributes,
				      DeviceName);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ZwCreateSymbolicLinkObject() failed (Status %lx)\n", Status);
      return(Status);
    }

  ZwClose(Handle);

  return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							EXPORTED
 *	IoDeleteSymbolicLink
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * @implemented
 */
NTSTATUS STDCALL
IoDeleteSymbolicLink(PUNICODE_STRING SymbolicLinkName)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  HANDLE Handle;
  NTSTATUS Status;

  ASSERT_IRQL(PASSIVE_LEVEL);

  DPRINT("IoDeleteSymbolicLink (SymbolicLinkName %S)\n",
	 SymbolicLinkName->Buffer);

  InitializeObjectAttributes(&ObjectAttributes,
			     SymbolicLinkName,
			     OBJ_OPENLINK,
			     NULL,
			     NULL);

  Status = ZwOpenSymbolicLinkObject(&Handle,
				    SYMBOLIC_LINK_ALL_ACCESS,
				    &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    return(Status);

  Status = ZwMakeTemporaryObject(Handle);
  ZwClose(Handle);

  return(Status);
}

/* EOF */
