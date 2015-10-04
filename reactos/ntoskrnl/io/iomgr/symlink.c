/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/iomgr/symlink.c
 * PURPOSE:         I/O Wrappers for Symbolic Links
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoCreateSymbolicLink(IN PUNICODE_STRING SymbolicLinkName,
                     IN PUNICODE_STRING DeviceName)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;
    NTSTATUS Status;
    PAGED_CODE();

    /* Initialize the object attributes and create the link */
    InitializeObjectAttributes(&ObjectAttributes,
                               SymbolicLinkName,
                               OBJ_PERMANENT | OBJ_CASE_INSENSITIVE,
                               NULL,
                               SePublicDefaultSd);
    Status = ZwCreateSymbolicLinkObject(&Handle,
                                        SYMBOLIC_LINK_ALL_ACCESS,
                                        &ObjectAttributes,
                                        DeviceName);
    if (NT_SUCCESS(Status)) ZwClose(Handle);

    /* Return status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoCreateUnprotectedSymbolicLink(IN PUNICODE_STRING SymbolicLinkName,
                                IN PUNICODE_STRING DeviceName)
{
    SECURITY_DESCRIPTOR SecurityDescriptor;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;
    NTSTATUS Status;
    PAGED_CODE();

    /* Create an SD */
    Status = RtlCreateSecurityDescriptor(&SecurityDescriptor,
                                         SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status)) return Status;

    /* Set the DACL */
    Status = RtlSetDaclSecurityDescriptor(&SecurityDescriptor,
                                          TRUE,
                                          NULL,
                                          TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    /* Initialize the object attributes and create the link */
    InitializeObjectAttributes(&ObjectAttributes,
                               SymbolicLinkName,
                               OBJ_PERMANENT | OBJ_CASE_INSENSITIVE,
                               NULL,
                               &SecurityDescriptor);
    Status = ZwCreateSymbolicLinkObject(&Handle,
                                        SYMBOLIC_LINK_ALL_ACCESS,
                                        &ObjectAttributes,
                                        DeviceName);
    if (NT_SUCCESS(Status)) ZwClose(Handle);

    /* Return status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoDeleteSymbolicLink(IN PUNICODE_STRING SymbolicLinkName)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;
    NTSTATUS Status;
    PAGED_CODE();

    /* Initialize the object attributes and open the link */
    InitializeObjectAttributes(&ObjectAttributes,
                               SymbolicLinkName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = ZwOpenSymbolicLinkObject(&Handle, DELETE, &ObjectAttributes);
    if (!NT_SUCCESS(Status)) return Status;

    /* Make the link temporary and close its handle */
    Status = ZwMakeTemporaryObject(Handle);
    if (NT_SUCCESS(Status)) ZwClose(Handle);

    /* Return status */
    return Status;
}

/* EOF */
