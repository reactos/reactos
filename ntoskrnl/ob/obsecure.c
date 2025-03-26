/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/ob/obsecure.c
* PURPOSE:         SRM Interface of the Object Manager
* PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
*                  Eric Kohl
*/

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

NTSTATUS
NTAPI
ObAssignObjectSecurityDescriptor(IN PVOID Object,
                                 IN PSECURITY_DESCRIPTOR SecurityDescriptor OPTIONAL,
                                 IN POOL_TYPE PoolType)
{
    POBJECT_HEADER ObjectHeader;
    NTSTATUS Status;
    PSECURITY_DESCRIPTOR NewSd;
    PEX_FAST_REF FastRef;
    PAGED_CODE();

    /* Get the object header */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
    FastRef = (PEX_FAST_REF)&ObjectHeader->SecurityDescriptor;
    if (!SecurityDescriptor)
    {
        /* Nothing to assign */
        ExInitializeFastReference(FastRef, NULL);
        return STATUS_SUCCESS;
    }

    /* Add it to our internal cache */
    Status = ObLogSecurityDescriptor(SecurityDescriptor,
                                     &NewSd,
                                     MAX_FAST_REFS + 1);
    if (NT_SUCCESS(Status))
    {
        /* Free the old copy */
        ExFreePoolWithTag(SecurityDescriptor, TAG_SD);

        /* Set the new pointer */
        ASSERT(NewSd);
        ExInitializeFastReference(FastRef, NewSd);
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
ObDeassignSecurity(IN OUT PSECURITY_DESCRIPTOR *SecurityDescriptor)
{
    EX_FAST_REF FastRef;
    ULONG Count;
    PSECURITY_DESCRIPTOR OldSecurityDescriptor;

    /* Get the fast reference and capture it */
    FastRef = *(PEX_FAST_REF)SecurityDescriptor;

    /* Don't free again later */
    *SecurityDescriptor = NULL;

    /* Get the descriptor and reference count */
    OldSecurityDescriptor = ExGetObjectFastReference(FastRef);
    Count = ExGetCountFastReference(FastRef);

    /* Dereference the descriptor */
    ObDereferenceSecurityDescriptor(OldSecurityDescriptor, Count + 1);

    /* All done */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ObQuerySecurityDescriptorInfo(IN PVOID Object,
                              IN PSECURITY_INFORMATION SecurityInformation,
                              OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
                              IN OUT PULONG Length,
                              IN PSECURITY_DESCRIPTOR *OutputSecurityDescriptor)
{
    POBJECT_HEADER ObjectHeader;
    NTSTATUS Status;
    PSECURITY_DESCRIPTOR ObjectSd;
    PAGED_CODE();

    /* Get the object header */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);

    /* Get the SD */
    ObjectSd = ObpReferenceSecurityDescriptor(ObjectHeader);

    /* Query the information */
    Status = SeQuerySecurityDescriptorInfo(SecurityInformation,
                                           SecurityDescriptor,
                                           Length,
                                           &ObjectSd);

    /* Check if we have an object SD and dereference it, if so */
    if (ObjectSd) ObDereferenceSecurityDescriptor(ObjectSd, 1);

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
ObSetSecurityDescriptorInfo(IN PVOID Object,
                            IN PSECURITY_INFORMATION SecurityInformation,
                            IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
                            IN OUT PSECURITY_DESCRIPTOR *OutputSecurityDescriptor,
                            IN POOL_TYPE PoolType,
                            IN PGENERIC_MAPPING GenericMapping)
{
    NTSTATUS Status;
    POBJECT_HEADER ObjectHeader;
    PSECURITY_DESCRIPTOR OldDescriptor, NewDescriptor, CachedDescriptor;
    PEX_FAST_REF FastRef;
    EX_FAST_REF OldValue;
    ULONG Count;
    PAGED_CODE();

    /* Get the object header */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
    while (TRUE)
    {
        /* Reference the old descriptor */
        OldDescriptor = ObpReferenceSecurityDescriptor(ObjectHeader);
        NewDescriptor = OldDescriptor;

        /* Set the SD information */
        Status = SeSetSecurityDescriptorInfo(Object,
                                             SecurityInformation,
                                             SecurityDescriptor,
                                             &NewDescriptor,
                                             PoolType,
                                             GenericMapping);
        if (!NT_SUCCESS(Status))
        {
            /* We failed, dereference the old one */
            if (OldDescriptor) ObDereferenceSecurityDescriptor(OldDescriptor, 1);
            break;
        }

        /* Now add this to the cache */
        Status = ObLogSecurityDescriptor(NewDescriptor,
                                         &CachedDescriptor,
                                         MAX_FAST_REFS + 1);

        /* Let go of our uncached copy */
        ExFreePool(NewDescriptor);

        /* Check for success */
        if (!NT_SUCCESS(Status))
        {
            /* We failed, dereference the old one */
            ObDereferenceSecurityDescriptor(OldDescriptor, 1);
            break;
        }

        /* Do the swap */
        FastRef = (PEX_FAST_REF)OutputSecurityDescriptor;
        OldValue = ExCompareSwapFastReference(FastRef,
                                              CachedDescriptor,
                                              OldDescriptor);

        /* Make sure the swap worked */
        if (ExGetObjectFastReference(OldValue) == OldDescriptor)
        {
            /* Flush waiters */
            ObpAcquireObjectLock(ObjectHeader);
            ObpReleaseObjectLock(ObjectHeader);

            /* And dereference the old one */
            Count = ExGetCountFastReference(OldValue);
            ObDereferenceSecurityDescriptor(OldDescriptor, Count + 2);
            break;
        }
        else
        {
            /* Someone changed it behind our back -- try again */
            ObDereferenceSecurityDescriptor(OldDescriptor, 1);
            ObDereferenceSecurityDescriptor(CachedDescriptor,
                                            MAX_FAST_REFS + 1);
        }
    }

    /* Return status */
    return Status;
}

BOOLEAN
NTAPI
ObCheckCreateObjectAccess(IN PVOID Object,
                          IN ACCESS_MASK CreateAccess,
                          IN PACCESS_STATE AccessState,
                          IN PUNICODE_STRING ComponentName,
                          IN BOOLEAN LockHeld,
                          IN KPROCESSOR_MODE AccessMode,
                          OUT PNTSTATUS AccessStatus)
{
    POBJECT_HEADER ObjectHeader;
    POBJECT_TYPE ObjectType;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    BOOLEAN SdAllocated;
    BOOLEAN Result = TRUE;
    ACCESS_MASK GrantedAccess = 0;
    PPRIVILEGE_SET Privileges = NULL;
    NTSTATUS Status;
    PAGED_CODE();

    /* Get the header and type */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
    ObjectType = ObjectHeader->Type;

    /* Get the security descriptor */
    Status = ObGetObjectSecurity(Object, &SecurityDescriptor, &SdAllocated);
    if (!NT_SUCCESS(Status))
    {
        /* We failed */
        *AccessStatus = Status;
        return FALSE;
    }

    /* Lock the security context */
    SeLockSubjectContext(&AccessState->SubjectSecurityContext);

    /* Check if we have an SD */
    if (SecurityDescriptor)
    {
        /* Now do the entire access check */
        Result = SeAccessCheck(SecurityDescriptor,
                               &AccessState->SubjectSecurityContext,
                               TRUE,
                               CreateAccess,
                               0,
                               &Privileges,
                               &ObjectType->TypeInfo.GenericMapping,
                               AccessMode,
                               &GrantedAccess,
                               AccessStatus);
        if (Privileges)
        {
            /* We got privileges, append them to the access state and free them */
            Status = SeAppendPrivileges(AccessState, Privileges);
            SeFreePrivileges(Privileges);
        }
    }

    /* We're done, unlock the context and release security */
    SeUnlockSubjectContext(&AccessState->SubjectSecurityContext);
    ObReleaseObjectSecurity(SecurityDescriptor, SdAllocated);
    return Result;
}

BOOLEAN
NTAPI
ObpCheckTraverseAccess(IN PVOID Object,
                       IN ACCESS_MASK TraverseAccess,
                       IN PACCESS_STATE AccessState OPTIONAL,
                       IN BOOLEAN LockHeld,
                       IN KPROCESSOR_MODE AccessMode,
                       OUT PNTSTATUS AccessStatus)
{
    POBJECT_HEADER ObjectHeader;
    POBJECT_TYPE ObjectType;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    BOOLEAN SdAllocated;
    BOOLEAN Result;
    ACCESS_MASK GrantedAccess = 0;
    PPRIVILEGE_SET Privileges = NULL;
    NTSTATUS Status;
    PAGED_CODE();

    /* Get the header and type */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
    ObjectType = ObjectHeader->Type;

    /* Get the security descriptor */
    Status = ObGetObjectSecurity(Object, &SecurityDescriptor, &SdAllocated);
    if (!NT_SUCCESS(Status))
    {
        /* We failed */
        *AccessStatus = Status;
        return FALSE;
    }

    /* First try to perform a fast traverse check
     * If it fails, then the entire access check will
     * have to be done.
     */
    Result = SeFastTraverseCheck(SecurityDescriptor,
                                 AccessState,
                                 FILE_WRITE_DATA,
                                 AccessMode);
    if (Result)
    {
        ObReleaseObjectSecurity(SecurityDescriptor, SdAllocated);
        return TRUE;
    }

    /* Lock the security context */
    SeLockSubjectContext(&AccessState->SubjectSecurityContext);

    /* Now do the entire access check */
    Result = SeAccessCheck(SecurityDescriptor,
                           &AccessState->SubjectSecurityContext,
                           TRUE,
                           TraverseAccess,
                           0,
                           &Privileges,
                           &ObjectType->TypeInfo.GenericMapping,
                           AccessMode,
                           &GrantedAccess,
                           AccessStatus);
    if (Privileges)
    {
        /* We got privileges, append them to the access state and free them */
        Status = SeAppendPrivileges(AccessState, Privileges);
        SeFreePrivileges(Privileges);
    }

    /* We're done, unlock the context and release security */
    SeUnlockSubjectContext(&AccessState->SubjectSecurityContext);
    ObReleaseObjectSecurity(SecurityDescriptor, SdAllocated);
    return Result;
}

BOOLEAN
NTAPI
ObpCheckObjectReference(IN PVOID Object,
                        IN OUT PACCESS_STATE AccessState,
                        IN BOOLEAN LockHeld,
                        IN KPROCESSOR_MODE AccessMode,
                        OUT PNTSTATUS AccessStatus)
{
    POBJECT_HEADER ObjectHeader;
    POBJECT_TYPE ObjectType;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    BOOLEAN SdAllocated;
    BOOLEAN Result;
    ACCESS_MASK GrantedAccess = 0;
    PPRIVILEGE_SET Privileges = NULL;
    NTSTATUS Status;
    PAGED_CODE();

    /* Get the header and type */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
    ObjectType = ObjectHeader->Type;

    /* Get the security descriptor */
    Status = ObGetObjectSecurity(Object, &SecurityDescriptor, &SdAllocated);
    if (!NT_SUCCESS(Status))
    {
        /* We failed */
        *AccessStatus = Status;
        return FALSE;
    }

    /* Lock the security context */
    SeLockSubjectContext(&AccessState->SubjectSecurityContext);

    /* Now do the entire access check */
    Result = SeAccessCheck(SecurityDescriptor,
                           &AccessState->SubjectSecurityContext,
                           TRUE,
                           AccessState->RemainingDesiredAccess,
                           AccessState->PreviouslyGrantedAccess,
                           &Privileges,
                           &ObjectType->TypeInfo.GenericMapping,
                           AccessMode,
                           &GrantedAccess,
                           AccessStatus);
    if (Result)
    {
        /* Update the access state */
        AccessState->RemainingDesiredAccess &= ~GrantedAccess;
        AccessState->PreviouslyGrantedAccess |= GrantedAccess;
    }

    /* Check if we have an SD */
    if (SecurityDescriptor)
    {
        /* Do audit alarm */
#if 0
        SeObjectReferenceAuditAlarm(&AccessState->OperationID,
                                    Object,
                                    SecurityDescriptor,
                                    &AccessState->SubjectSecurityContext,
                                    AccessState->RemainingDesiredAccess |
                                    AccessState->PreviouslyGrantedAccess,
                                    ((PAUX_ACCESS_DATA)(AccessState->AuxData))->
                                    PrivilegeSet,
                                    Result,
                                    AccessMode);
#endif
    }

    /* We're done, unlock the context and release security */
    SeUnlockSubjectContext(&AccessState->SubjectSecurityContext);
    ObReleaseObjectSecurity(SecurityDescriptor, SdAllocated);
    return Result;
}

/*++
* @name ObCheckObjectAccess
*
*     The ObCheckObjectAccess routine <FILLMEIN>
*
* @param Object
*        <FILLMEIN>
*
* @param AccessState
*        <FILLMEIN>
*
* @param LockHeld
*        <FILLMEIN>
*
* @param AccessMode
*        <FILLMEIN>
*
* @param ReturnedStatus
*        <FILLMEIN>
*
* @return TRUE if access was granted, FALSE otherwise.
*
* @remarks None.
*
*--*/
BOOLEAN
NTAPI
ObCheckObjectAccess(IN PVOID Object,
                    IN OUT PACCESS_STATE AccessState,
                    IN BOOLEAN LockHeld,
                    IN KPROCESSOR_MODE AccessMode,
                    OUT PNTSTATUS ReturnedStatus)
{
    POBJECT_HEADER ObjectHeader;
    POBJECT_TYPE ObjectType;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    BOOLEAN SdAllocated;
    NTSTATUS Status;
    BOOLEAN Result;
    ACCESS_MASK GrantedAccess;
    PPRIVILEGE_SET Privileges = NULL;
    PAGED_CODE();

    /* Get the object header and type */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
    ObjectType = ObjectHeader->Type;

    /* Get security information */
    Status = ObGetObjectSecurity(Object, &SecurityDescriptor, &SdAllocated);
    if (!NT_SUCCESS(Status))
    {
        /* Return failure */
        *ReturnedStatus = Status;
        return FALSE;
    }
    else if (!SecurityDescriptor)
    {
        /* Otherwise, if we don't actually have an SD, return success */
        *ReturnedStatus = Status;
        return TRUE;
    }

    /* Lock the security context */
    SeLockSubjectContext(&AccessState->SubjectSecurityContext);

    /* Now do the entire access check */
    Result = SeAccessCheck(SecurityDescriptor,
                           &AccessState->SubjectSecurityContext,
                           TRUE,
                           AccessState->RemainingDesiredAccess,
                           AccessState->PreviouslyGrantedAccess,
                           &Privileges,
                           &ObjectType->TypeInfo.GenericMapping,
                           AccessMode,
                           &GrantedAccess,
                           ReturnedStatus);
    if (Privileges)
    {
        /* We got privileges, append them to the access state and free them */
        Status = SeAppendPrivileges(AccessState, Privileges);
        SeFreePrivileges(Privileges);
    }

    /* Check if access was granted */
    if (Result)
    {
        /* Update the access state */
        AccessState->RemainingDesiredAccess &= ~(GrantedAccess |
                                                 MAXIMUM_ALLOWED);
        AccessState->PreviouslyGrantedAccess |= GrantedAccess;
    }

    /* Do audit alarm */
    SeOpenObjectAuditAlarm(&ObjectType->Name,
                           Object,
                           NULL,
                           SecurityDescriptor,
                           AccessState,
                           FALSE,
                           Result,
                           AccessMode,
                           &AccessState->GenerateOnClose);

    /* We're done, unlock the context and release security */
    SeUnlockSubjectContext(&AccessState->SubjectSecurityContext);
    ObReleaseObjectSecurity(SecurityDescriptor, SdAllocated);
    return Result;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*++
* @name ObAssignSecurity
* @implemented NT4
*
*     The ObAssignSecurity routine <FILLMEIN>
*
* @param AccessState
*        <FILLMEIN>
*
* @param SecurityDescriptor
*        <FILLMEIN>
*
* @param Object
*        <FILLMEIN>
*
* @param Type
*        <FILLMEIN>
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
ObAssignSecurity(IN PACCESS_STATE AccessState,
                 IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                 IN PVOID Object,
                 IN POBJECT_TYPE Type)
{
    PSECURITY_DESCRIPTOR NewDescriptor;
    NTSTATUS Status;
    KIRQL CalloutIrql;
    PAGED_CODE();

    /* Build the new security descriptor */
    Status = SeAssignSecurity(SecurityDescriptor,
                              AccessState->SecurityDescriptor,
                              &NewDescriptor,
                              (Type == ObpDirectoryObjectType),
                              &AccessState->SubjectSecurityContext,
                              &Type->TypeInfo.GenericMapping,
                              PagedPool);
    if (!NT_SUCCESS(Status)) return Status;

    /* Call the security method */
    ObpCalloutStart(&CalloutIrql);
    Status = Type->TypeInfo.SecurityProcedure(Object,
                                              AssignSecurityDescriptor,
                                              NULL,
                                              NewDescriptor,
                                              NULL,
                                              NULL,
                                              PagedPool,
                                              &Type->TypeInfo.GenericMapping);
    ObpCalloutEnd(CalloutIrql, "Security", Type, Object);

    /* Check for failure and deassign security if so */
    if (!NT_SUCCESS(Status)) SeDeassignSecurity(&NewDescriptor);

    /* Return to caller */
    return Status;
}

/*++
* @name ObGetObjectSecurity
* @implemented NT4
*
*     The ObGetObjectSecurity routine <FILLMEIN>
*
* @param Object
*        <FILLMEIN>
*
* @param SecurityDescriptor
*        <FILLMEIN>
*
* @param MemoryAllocated
*        <FILLMEIN>
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
ObGetObjectSecurity(IN PVOID Object,
                    OUT PSECURITY_DESCRIPTOR *SecurityDescriptor,
                    OUT PBOOLEAN MemoryAllocated)
{
    POBJECT_HEADER Header;
    POBJECT_TYPE Type;
    ULONG Length = 0;
    NTSTATUS Status;
    SECURITY_INFORMATION SecurityInformation;
    KIRQL CalloutIrql;
    PAGED_CODE();

    /* Get the object header and type */
    Header = OBJECT_TO_OBJECT_HEADER(Object);
    Type = Header->Type;

    /* Tell the caller that we didn't have to allocate anything yet */
    *MemoryAllocated = FALSE;

    /* Check if the object uses default security */
    if (Type->TypeInfo.SecurityProcedure == SeDefaultObjectMethod)
    {
        /* Reference the descriptor */
        *SecurityDescriptor = ObpReferenceSecurityDescriptor(Header);
        return STATUS_SUCCESS;
    }

    /* Set mask to query */
    SecurityInformation =  OWNER_SECURITY_INFORMATION |
                           GROUP_SECURITY_INFORMATION |
                           DACL_SECURITY_INFORMATION |
                           SACL_SECURITY_INFORMATION;

    /* Get the security descriptor size */
    ObpCalloutStart(&CalloutIrql);
    Status = Type->TypeInfo.SecurityProcedure(Object,
                                              QuerySecurityDescriptor,
                                              &SecurityInformation,
                                              *SecurityDescriptor,
                                              &Length,
                                              &Header->SecurityDescriptor,
                                              Type->TypeInfo.PoolType,
                                              &Type->TypeInfo.GenericMapping);
    ObpCalloutEnd(CalloutIrql, "Security", Type, Object);

    /* Check for failure */
    if (Status != STATUS_BUFFER_TOO_SMALL) return Status;

    /* Allocate security descriptor */
    *SecurityDescriptor = ExAllocatePoolWithTag(PagedPool,
                                                Length,
                                                TAG_SEC_QUERY);
    if (!(*SecurityDescriptor)) return STATUS_INSUFFICIENT_RESOURCES;
    *MemoryAllocated = TRUE;

    /* Query security descriptor */
    ObpCalloutStart(&CalloutIrql);
    Status = Type->TypeInfo.SecurityProcedure(Object,
                                              QuerySecurityDescriptor,
                                              &SecurityInformation,
                                              *SecurityDescriptor,
                                              &Length,
                                              &Header->SecurityDescriptor,
                                              Type->TypeInfo.PoolType,
                                              &Type->TypeInfo.GenericMapping);
    ObpCalloutEnd(CalloutIrql, "Security", Type, Object);

    /* Check for failure */
    if (!NT_SUCCESS(Status))
    {
        /* Free the descriptor and tell the caller we failed */
        ExFreePoolWithTag(*SecurityDescriptor, TAG_SEC_QUERY);
        *MemoryAllocated = FALSE;
    }

    /* Return status */
    return Status;
}

/*++
* @name ObReleaseObjectSecurity
* @implemented NT4
*
*     The ObReleaseObjectSecurity routine <FILLMEIN>
*
* @param SecurityDescriptor
*        <FILLMEIN>
*
* @param MemoryAllocated
*        <FILLMEIN>
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
VOID
NTAPI
ObReleaseObjectSecurity(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                        IN BOOLEAN MemoryAllocated)
{
    PAGED_CODE();

    /* Nothing to do in this case */
    if (!SecurityDescriptor) return;

    /* Check if we had allocated it from memory */
    if (MemoryAllocated)
    {
        /* Free it */
        ExFreePoolWithTag(SecurityDescriptor, TAG_SEC_QUERY);
    }
    else
    {
        /* Otherwise this means we used an internal descriptor */
        ObDereferenceSecurityDescriptor(SecurityDescriptor, 1);
    }
}

/*++
* @name ObSetSecurityObjectByPointer
* @implemented NT5.1
*
*     The ObSetSecurityObjectByPointer routine <FILLMEIN>
*
* @param SecurityDescriptor
*        <FILLMEIN>
*
* @param MemoryAllocated
*        <FILLMEIN>
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
ObSetSecurityObjectByPointer(IN PVOID Object,
                             IN SECURITY_INFORMATION SecurityInformation,
                             IN PSECURITY_DESCRIPTOR SecurityDescriptor)
{
    POBJECT_TYPE Type;
    POBJECT_HEADER Header;
    PAGED_CODE();

    /* Get the header and type */
    Header = OBJECT_TO_OBJECT_HEADER(Object);
    Type = Header->Type;

    /* Sanity check */
    ASSERT(SecurityDescriptor);

    /* Call the security procedure */
    return Type->TypeInfo.SecurityProcedure(Object,
                                            SetSecurityDescriptor,
                                            &SecurityInformation,
                                            SecurityDescriptor,
                                            NULL,
                                            &Header->SecurityDescriptor,
                                            Type->TypeInfo.PoolType,
                                            &Type->TypeInfo.GenericMapping);
}

/*++
* @name NtQuerySecurityObject
* @implemented NT4
*
*     The NtQuerySecurityObject routine <FILLMEIN>
*
* @param Handle
*        <FILLMEIN>
*
* @param SecurityInformation
*        <FILLMEIN>
*
* @param SecurityDescriptor
*        <FILLMEIN>
*
* @param Length
*        <FILLMEIN>
*
* @param ResultLength
*        <FILLMEIN>
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
NtQuerySecurityObject(IN HANDLE Handle,
                      IN SECURITY_INFORMATION SecurityInformation,
                      OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
                      IN ULONG Length,
                      OUT PULONG ResultLength)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PVOID Object;
    POBJECT_HEADER Header;
    POBJECT_TYPE Type;
    ACCESS_MASK DesiredAccess;
    NTSTATUS Status;
    PAGED_CODE();

    /* Check if we came from user mode */
    if (PreviousMode != KernelMode)
    {
        /* Enter SEH */
        _SEH2_TRY
        {
            /* Probe the SD and the length pointer */
            ProbeForWrite(SecurityDescriptor, Length, sizeof(ULONG));
            ProbeForWriteUlong(ResultLength);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Get the required access rights for the operation */
    SeQuerySecurityAccessMask(SecurityInformation, &DesiredAccess);

    /* Reference the object */
    Status = ObReferenceObjectByHandle(Handle,
                                       DesiredAccess,
                                       NULL,
                                       PreviousMode,
                                       &Object,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Get the Object Header and Type */
    Header = OBJECT_TO_OBJECT_HEADER(Object);
    Type = Header->Type;

    /* Call the security procedure's query function */
    Status = Type->TypeInfo.SecurityProcedure(Object,
                                              QuerySecurityDescriptor,
                                              &SecurityInformation,
                                              SecurityDescriptor,
                                              &Length,
                                              &Header->SecurityDescriptor,
                                              Type->TypeInfo.PoolType,
                                              &Type->TypeInfo.GenericMapping);

    /* Dereference the object */
    ObDereferenceObject(Object);

    /* Protect write with SEH */
    _SEH2_TRY
    {
        /* Return the needed length */
        *ResultLength = Length;
    }
    _SEH2_EXCEPT(ExSystemExceptionFilter())
    {
        /* Get the exception code */
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    /* Return status */
    return Status;
}

/*++
* @name NtSetSecurityObject
* @implemented NT4
*
*     The NtSetSecurityObject routine <FILLMEIN>
*
* @param Handle
*        <FILLMEIN>
*
* @param SecurityInformation
*        <FILLMEIN>
*
* @param SecurityDescriptor
*        <FILLMEIN>
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
NtSetSecurityObject(IN HANDLE Handle,
                    IN SECURITY_INFORMATION SecurityInformation,
                    IN PSECURITY_DESCRIPTOR SecurityDescriptor)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PVOID Object;
    SECURITY_DESCRIPTOR_RELATIVE *CapturedDescriptor;
    ACCESS_MASK DesiredAccess = 0;
    NTSTATUS Status;
    PAGED_CODE();

    /* Make sure the caller doesn't pass a NULL security descriptor! */
    if (!SecurityDescriptor) return STATUS_ACCESS_VIOLATION;

    /* Set the required access rights for the operation */
    SeSetSecurityAccessMask(SecurityInformation, &DesiredAccess);

    /* Reference the object */
    Status = ObReferenceObjectByHandle(Handle,
                                       DesiredAccess,
                                       NULL,
                                       PreviousMode,
                                       &Object,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Capture and make a copy of the security descriptor */
        Status = SeCaptureSecurityDescriptor(SecurityDescriptor,
                                             PreviousMode,
                                             PagedPool,
                                             TRUE,
                                             (PSECURITY_DESCRIPTOR*)
                                             &CapturedDescriptor);
        if (!NT_SUCCESS(Status))
        {
            /* Fail */
            ObDereferenceObject(Object);
            return Status;
        }

        /* Sanity check */
        ASSERT(CapturedDescriptor->Control & SE_SELF_RELATIVE);

        /*
         * Make sure the security descriptor passed by the caller
         * is valid for the operation we're about to perform
         */
        if (((SecurityInformation & OWNER_SECURITY_INFORMATION) &&
             !(CapturedDescriptor->Owner)) ||
            ((SecurityInformation & GROUP_SECURITY_INFORMATION) &&
             !(CapturedDescriptor->Group)))
        {
            /* Set the failure status */
            Status = STATUS_INVALID_SECURITY_DESCR;
        }
        else
        {
            /* Set security */
            Status = ObSetSecurityObjectByPointer(Object,
                                                  SecurityInformation,
                                                  CapturedDescriptor);
        }

        /* Release the descriptor and return status */
        SeReleaseSecurityDescriptor((PSECURITY_DESCRIPTOR)CapturedDescriptor,
                                    PreviousMode,
                                    TRUE);

        /* Now we can dereference the object */
        ObDereferenceObject(Object);
    }

    return Status;
}

/*++
* @name ObQueryObjectAuditingByHandle
* @implemented NT5
*
*     The ObDereferenceSecurityDescriptor routine <FILLMEIN>
*
* @param SecurityDescriptor
*        <FILLMEIN>
*
* @param Count
*        <FILLMEIN>
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
ObQueryObjectAuditingByHandle(IN HANDLE Handle,
                              OUT PBOOLEAN GenerateOnClose)
{
    PHANDLE_TABLE_ENTRY HandleEntry;
    PVOID HandleTable;
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Check if we're dealing with a kernel handle */
    if (ObpIsKernelHandle(Handle, ExGetPreviousMode()))
    {
        /* Use the kernel table and convert the handle */
        HandleTable = ObpKernelHandleTable;
        Handle = ObKernelHandleToHandle(Handle);
    }
    else
    {
        /* Use the process's handle table */
        HandleTable = PsGetCurrentProcess()->ObjectTable;
    }

    /* Enter a critical region while we touch the handle table */
    KeEnterCriticalRegion();

    /* Map the handle */
    HandleEntry = ExMapHandleToPointer(HandleTable, Handle);
    if(HandleEntry)
    {
        /* Check if the flag is set */
        *GenerateOnClose = HandleEntry->ObAttributes & OBJ_AUDIT_OBJECT_CLOSE;

        /* Unlock the entry */
        ExUnlockHandleTableEntry(HandleTable, HandleEntry);
    }
    else
    {
        /* Otherwise, fail */
        Status = STATUS_INVALID_HANDLE;
    }

    /* Leave the critical region and return the status */
    KeLeaveCriticalRegion();
    return Status;
}

/* EOF */
