/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/se/audit.c
 * PURPOSE:         Audit functions
 *
 * PROGRAMMERS:     Eric Kohl
 *                  Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define SEP_PRIVILEGE_SET_MAX_COUNT 60

/* PRIVATE FUNCTIONS***********************************************************/

BOOLEAN
NTAPI
SeDetailedAuditingWithToken(IN PTOKEN Token)
{
    /* FIXME */
    return FALSE;
}

VOID
NTAPI
SeAuditProcessCreate(IN PEPROCESS Process)
{
    /* FIXME */
}

VOID
NTAPI
SeAuditProcessExit(IN PEPROCESS Process)
{
    /* FIXME */
}

NTSTATUS
NTAPI
SeInitializeProcessAuditName(IN PFILE_OBJECT FileObject,
                             IN BOOLEAN DoAudit,
                             OUT POBJECT_NAME_INFORMATION *AuditInfo)
{
    OBJECT_NAME_INFORMATION LocalNameInfo;
    POBJECT_NAME_INFORMATION ObjectNameInfo = NULL;
    ULONG ReturnLength = 8;
    NTSTATUS Status;

    PAGED_CODE();
    ASSERT(AuditInfo);

    /* Check if we should do auditing */
    if (DoAudit)
    {
        /* FIXME: TODO */
    }

    /* Now query the name */
    Status = ObQueryNameString(FileObject,
                               &LocalNameInfo,
                               sizeof(LocalNameInfo),
                               &ReturnLength);
    if (((Status == STATUS_BUFFER_OVERFLOW) ||
         (Status == STATUS_BUFFER_TOO_SMALL) ||
         (Status == STATUS_INFO_LENGTH_MISMATCH)) &&
        (ReturnLength != sizeof(LocalNameInfo)))
    {
        /* Allocate required size */
        ObjectNameInfo = ExAllocatePoolWithTag(NonPagedPool,
                                               ReturnLength,
                                               TAG_SEPA);
        if (ObjectNameInfo)
        {
            /* Query the name again */
            Status = ObQueryNameString(FileObject,
                                       ObjectNameInfo,
                                       ReturnLength,
                                       &ReturnLength);
        }
    }

    /* Check if we got here due to failure */
    if ((ObjectNameInfo) &&
        (!(NT_SUCCESS(Status)) || (ReturnLength == sizeof(LocalNameInfo))))
    {
        /* First, free any buffer we might've allocated */
        ASSERT(FALSE);
        if (ObjectNameInfo) ExFreePool(ObjectNameInfo);

        /* Now allocate a temporary one */
        ReturnLength = sizeof(OBJECT_NAME_INFORMATION);
        ObjectNameInfo = ExAllocatePoolWithTag(NonPagedPool,
                                               sizeof(OBJECT_NAME_INFORMATION),
                                               TAG_SEPA);
        if (ObjectNameInfo)
        {
            /* Clear it */
            RtlZeroMemory(ObjectNameInfo, ReturnLength);
            Status = STATUS_SUCCESS;
        }
    }

    /* Check if memory allocation failed */
    if (!ObjectNameInfo) Status = STATUS_NO_MEMORY;

    /* Return the audit name */
    *AuditInfo = ObjectNameInfo;

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
SeLocateProcessImageName(IN PEPROCESS Process,
                         OUT PUNICODE_STRING *ProcessImageName)
{
    POBJECT_NAME_INFORMATION AuditName;
    PUNICODE_STRING ImageName;
    PFILE_OBJECT FileObject;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    /* Assume failure */
    *ProcessImageName = NULL;

    /* Check if we have audit info */
    AuditName = Process->SeAuditProcessCreationInfo.ImageFileName;
    if (!AuditName)
    {
        /* Get the file object */
        Status = PsReferenceProcessFilePointer(Process, &FileObject);
        if (!NT_SUCCESS(Status)) return Status;

        /* Initialize the audit structure */
        Status = SeInitializeProcessAuditName(FileObject, TRUE, &AuditName);
        if (NT_SUCCESS(Status))
        {
            /* Set it */
            if (InterlockedCompareExchangePointer((PVOID*)&Process->
                                                  SeAuditProcessCreationInfo.ImageFileName,
                                                  AuditName,
                                                  NULL))
            {
                /* Someone beat us to it, deallocate our copy */
                ExFreePool(AuditName);
            }
        }

        /* Dereference the file object */
        ObDereferenceObject(FileObject);
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Get audit info again, now we have it for sure */
    AuditName = Process->SeAuditProcessCreationInfo.ImageFileName;

    /* Allocate the output string */
    ImageName = ExAllocatePoolWithTag(NonPagedPool,
                                      AuditName->Name.MaximumLength +
                                      sizeof(UNICODE_STRING),
                                      TAG_SEPA);
    if (!ImageName) return STATUS_NO_MEMORY;

    /* Make a copy of it */
    RtlCopyMemory(ImageName,
                  &AuditName->Name,
                  AuditName->Name.MaximumLength + sizeof(UNICODE_STRING));

    /* Fix up the buffer */
    ImageName->Buffer = (PWSTR)(ImageName + 1);

    /* Return it */
    *ProcessImageName = ImageName;

    /* Return status */
    return Status;
}

VOID
NTAPI
SepAdtCloseObjectAuditAlarm(
    PUNICODE_STRING SubsystemName,
    PVOID HandleId,
    PSID Sid)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
SepAdtPrivilegedServiceAuditAlarm(
    PSECURITY_SUBJECT_CONTEXT SubjectContext,
    _In_opt_ PUNICODE_STRING SubsystemName,
    _In_opt_ PUNICODE_STRING ServiceName,
    _In_ PTOKEN Token,
    _In_ PTOKEN PrimaryToken,
    _In_ PPRIVILEGE_SET Privileges,
    _In_ BOOLEAN AccessGranted )
{
    UNIMPLEMENTED;
}

_Must_inspect_result_
NTSTATUS
NTAPI
SepAccessCheckAndAuditAlarm(
    _In_ PUNICODE_STRING SubsystemName,
    _In_opt_ PVOID HandleId,
    _In_ PHANDLE ClientToken,
    _In_ PUNICODE_STRING ObjectTypeName,
    _In_ PUNICODE_STRING ObjectName,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_opt_ PSID PrincipalSelfSid,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ AUDIT_EVENT_TYPE AuditType,
    _In_ ULONG Flags,
    _In_reads_opt_(ObjectTypeListLength) POBJECT_TYPE_LIST ObjectTypeList,
    _In_ ULONG ObjectTypeListLength,
    _In_ PGENERIC_MAPPING GenericMapping,
    _Out_writes_(ObjectTypeListLength) PACCESS_MASK GrantedAccessList,
    _Out_writes_(ObjectTypeListLength) PNTSTATUS AccessStatusList,
    _Out_ PBOOLEAN GenerateOnClose,
    _In_ BOOLEAN UseResultList)
{
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    ULONG ResultListLength;
    GENERIC_MAPPING LocalGenericMapping;
    NTSTATUS Status;
    PAGED_CODE();

    DBG_UNREFERENCED_LOCAL_VARIABLE(LocalGenericMapping);

    /* Only user mode is supported! */
    ASSERT(ExGetPreviousMode() != KernelMode);

    /* Validate AuditType */
    if ((AuditType != AuditEventObjectAccess) &&
        (AuditType != AuditEventDirectoryServiceAccess))
    {
        DPRINT1("Invalid audit type: %u\n", AuditType);
        return STATUS_INVALID_PARAMETER;
    }

    /* Capture the security subject context */
    SeCaptureSubjectContext(&SubjectContext);

    /* Did the caller pass a token handle? */
    if (ClientToken == NULL)
    {
        /* Check if we have a token in the subject context */
        if (SubjectContext.ClientToken == NULL)
        {
            Status = STATUS_NO_IMPERSONATION_TOKEN;
            goto Cleanup;
        }

        /* Check if we have a valid impersonation level */
        if (SubjectContext.ImpersonationLevel < SecurityIdentification)
        {
            Status = STATUS_BAD_IMPERSONATION_LEVEL;
            goto Cleanup;
        }
    }

    /* Are we using a result list? */
    if (UseResultList)
    {
        /* The list length equals the object type list length */
        ResultListLength = ObjectTypeListLength;
        if ((ResultListLength == 0) || (ResultListLength > 0x1000))
        {
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }
    }
    else
    {
        /* List length is 1 */
        ResultListLength = 1;
    }

    _SEH2_TRY
    {
        /* Probe output buffers */
        ProbeForWrite(AccessStatusList,
                      ResultListLength * sizeof(*AccessStatusList),
                      sizeof(*AccessStatusList));
        ProbeForWrite(GrantedAccessList,
                      ResultListLength * sizeof(*GrantedAccessList),
                      sizeof(*GrantedAccessList));

        /* Probe generic mapping and make a local copy */
        ProbeForRead(GenericMapping, sizeof(*GenericMapping), sizeof(ULONG));
        LocalGenericMapping = * GenericMapping;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
        goto Cleanup;
    }
    _SEH2_END;


    UNIMPLEMENTED;

    /* For now pretend everything else is ok */
    Status = STATUS_SUCCESS;

Cleanup:

    /* Release the security subject context */
    SeReleaseSubjectContext(&SubjectContext);

    return Status;
}


/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @unimplemented
 */
VOID
NTAPI
SeAuditHardLinkCreation(IN PUNICODE_STRING FileName,
                        IN PUNICODE_STRING LinkName,
                        IN BOOLEAN bSuccess)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
SeAuditingFileEvents(IN BOOLEAN AccessGranted,
                     IN PSECURITY_DESCRIPTOR SecurityDescriptor)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
SeAuditingFileEventsWithContext(IN BOOLEAN AccessGranted,
                                IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                                IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext OPTIONAL)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
SeAuditingHardLinkEvents(IN BOOLEAN AccessGranted,
                         IN PSECURITY_DESCRIPTOR SecurityDescriptor)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
SeAuditingHardLinkEventsWithContext(IN BOOLEAN AccessGranted,
                                    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                                    IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext OPTIONAL)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
SeAuditingFileOrGlobalEvents(IN BOOLEAN AccessGranted,
                             IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                             IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
VOID
NTAPI
SeCloseObjectAuditAlarm(IN PVOID Object,
                        IN HANDLE Handle,
                        IN BOOLEAN PerformAction)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID NTAPI
SeDeleteObjectAuditAlarm(IN PVOID Object,
                         IN HANDLE Handle)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
NTAPI
SeOpenObjectAuditAlarm(IN PUNICODE_STRING ObjectTypeName,
                       IN PVOID Object OPTIONAL,
                       IN PUNICODE_STRING AbsoluteObjectName OPTIONAL,
                       IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                       IN PACCESS_STATE AccessState,
                       IN BOOLEAN ObjectCreated,
                       IN BOOLEAN AccessGranted,
                       IN KPROCESSOR_MODE AccessMode,
                       OUT PBOOLEAN GenerateOnClose)
{
    PAGED_CODE();

    /* Audits aren't done on kernel-mode access */
    if (AccessMode == KernelMode) return;

    /* Otherwise, unimplemented! */
    //UNIMPLEMENTED;
    return;
}

/*
 * @unimplemented
 */
VOID NTAPI
SeOpenObjectForDeleteAuditAlarm(IN PUNICODE_STRING ObjectTypeName,
                                IN PVOID Object OPTIONAL,
                                IN PUNICODE_STRING AbsoluteObjectName OPTIONAL,
                                IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                                IN PACCESS_STATE AccessState,
                                IN BOOLEAN ObjectCreated,
                                IN BOOLEAN AccessGranted,
                                IN KPROCESSOR_MODE AccessMode,
                                OUT PBOOLEAN GenerateOnClose)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
NTAPI
SePrivilegeObjectAuditAlarm(IN HANDLE Handle,
                            IN PSECURITY_SUBJECT_CONTEXT SubjectContext,
                            IN ACCESS_MASK DesiredAccess,
                            IN PPRIVILEGE_SET Privileges,
                            IN BOOLEAN AccessGranted,
                            IN KPROCESSOR_MODE CurrentMode)
{
    UNIMPLEMENTED;
}

/* SYSTEM CALLS ***************************************************************/

NTSTATUS
NTAPI
NtCloseObjectAuditAlarm(
    PUNICODE_STRING SubsystemName,
    PVOID HandleId,
    BOOLEAN GenerateOnClose)
{
    UNICODE_STRING CapturedSubsystemName;
    KPROCESSOR_MODE PreviousMode;
    BOOLEAN UseImpersonationToken;
    PETHREAD CurrentThread;
    BOOLEAN CopyOnOpen, EffectiveOnly;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    NTSTATUS Status;
    PTOKEN Token;
    PAGED_CODE();

    /* Get the previous mode (only user mode is supported!) */
    PreviousMode = ExGetPreviousMode();
    ASSERT(PreviousMode != KernelMode);

    /* Do we even need to do anything? */
    if (!GenerateOnClose)
    {
        /* Nothing to do, return success */
        return STATUS_SUCCESS;
    }

    /* Validate privilege */
    if (!SeSinglePrivilegeCheck(SeAuditPrivilege, PreviousMode))
    {
        DPRINT1("Caller does not have SeAuditPrivilege\n");
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    /* Probe and capture the subsystem name */
    Status = ProbeAndCaptureUnicodeString(&CapturedSubsystemName,
                                          PreviousMode,
                                          SubsystemName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to capture subsystem name!\n");
        return Status;
    }

    /* Get the current thread and check if it's impersonating */
    CurrentThread = PsGetCurrentThread();
    if (PsIsThreadImpersonating(CurrentThread))
    {
        /* Get the impersonation token */
        Token = PsReferenceImpersonationToken(CurrentThread,
                                              &CopyOnOpen,
                                              &EffectiveOnly,
                                              &ImpersonationLevel);
        UseImpersonationToken = TRUE;
    }
    else
    {
        /* Get the primary token */
        Token = PsReferencePrimaryToken(PsGetCurrentProcess());
        UseImpersonationToken = FALSE;
    }

    /* Call the internal function */
    SepAdtCloseObjectAuditAlarm(&CapturedSubsystemName,
                                HandleId,
                                Token->UserAndGroups->Sid);

    /* Release the captured subsystem name */
    ReleaseCapturedUnicodeString(&CapturedSubsystemName, PreviousMode);

    /* Check what token we used */
    if (UseImpersonationToken)
    {
        /* Release impersonation token */
        PsDereferenceImpersonationToken(Token);
    }
    else
    {
        /* Release primary token */
        PsDereferencePrimaryToken(Token);
    }

    return STATUS_SUCCESS;
}


NTSTATUS NTAPI
NtDeleteObjectAuditAlarm(IN PUNICODE_STRING SubsystemName,
                         IN PVOID HandleId,
                         IN BOOLEAN GenerateOnClose)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS NTAPI
NtOpenObjectAuditAlarm(IN PUNICODE_STRING SubsystemName,
                       IN PVOID HandleId,
                       IN PUNICODE_STRING ObjectTypeName,
                       IN PUNICODE_STRING ObjectName,
                       IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                       IN HANDLE ClientToken,
                       IN ULONG DesiredAccess,
                       IN ULONG GrantedAccess,
                       IN PPRIVILEGE_SET Privileges,
                       IN BOOLEAN ObjectCreation,
                       IN BOOLEAN AccessGranted,
                       OUT PBOOLEAN GenerateOnClose)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


__kernel_entry
NTSTATUS
NTAPI
NtPrivilegedServiceAuditAlarm(
    _In_opt_ PUNICODE_STRING SubsystemName,
    _In_opt_ PUNICODE_STRING ServiceName,
    _In_ HANDLE ClientToken,
    _In_ PPRIVILEGE_SET Privileges,
    _In_ BOOLEAN AccessGranted )
{
    KPROCESSOR_MODE PreviousMode;
    PTOKEN Token;
    volatile PPRIVILEGE_SET CapturedPrivileges = NULL;
    UNICODE_STRING CapturedSubsystemName;
    UNICODE_STRING CapturedServiceName;
    ULONG PrivilegeCount, PrivilegesSize;
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    NTSTATUS Status;
    PAGED_CODE();

    /* Get the previous mode (only user mode is supported!) */
    PreviousMode = ExGetPreviousMode();
    ASSERT(PreviousMode != KernelMode);

    /* Reference the client token */
    Status = ObReferenceObjectByHandle(ClientToken,
                                       TOKEN_QUERY,
                                       SeTokenObjectType,
                                       PreviousMode,
                                       (PVOID*)&Token,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to reference client token: 0x%lx\n", Status);
        return Status;
    }

    /* Validate the token's impersonation level */
    if ((Token->TokenType == TokenImpersonation) &&
        (Token->ImpersonationLevel < SecurityIdentification))
    {
        DPRINT1("Invalid impersonation level (%u)\n", Token->ImpersonationLevel);
        ObfDereferenceObject(Token);
        return STATUS_BAD_IMPERSONATION_LEVEL;
    }

    /* Validate privilege */
    if (!SeSinglePrivilegeCheck(SeAuditPrivilege, PreviousMode))
    {
        DPRINT1("Caller does not have SeAuditPrivilege\n");
        ObfDereferenceObject(Token);
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    /* Do we have a subsystem name? */
    if (SubsystemName != NULL)
    {
        /* Probe and capture the subsystem name */
        Status = ProbeAndCaptureUnicodeString(&CapturedSubsystemName,
                                              PreviousMode,
                                              SubsystemName);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to capture subsystem name!\n");
            goto Cleanup;
        }
    }

    /* Do we have a service name? */
    if (ServiceName != NULL)
    {
        /* Probe and capture the service name */
        Status = ProbeAndCaptureUnicodeString(&CapturedServiceName,
                                              PreviousMode,
                                              ServiceName);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to capture service name!\n");
            goto Cleanup;
        }
    }

    _SEH2_TRY
    {
        /* Probe the basic privilege set structure */
        ProbeForRead(Privileges, sizeof(PRIVILEGE_SET), sizeof(ULONG));

        /* Validate privilege count */
        PrivilegeCount = Privileges->PrivilegeCount;
        if (PrivilegeCount > SEP_PRIVILEGE_SET_MAX_COUNT)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        /* Calculate the size of the Privileges structure */
        PrivilegesSize = FIELD_OFFSET(PRIVILEGE_SET, Privilege[PrivilegeCount]);

        /* Probe the whole structure */
        ProbeForRead(Privileges, PrivilegesSize, sizeof(ULONG));

        /* Allocate a temp buffer */
        CapturedPrivileges = ExAllocatePoolWithTag(PagedPool,
                                                   PrivilegesSize,
                                                   'rPeS');
        if (CapturedPrivileges == NULL)
        {
            DPRINT1("Failed to allocate %u bytes\n", PrivilegesSize);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        /* Copy the privileges */
        RtlCopyMemory(CapturedPrivileges, Privileges, PrivilegesSize);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
        DPRINT1("Got exception 0x%lx\n", Status);
        goto Cleanup;
    }
    _SEH2_END;

    /* Capture the security subject context */
    SeCaptureSubjectContext(&SubjectContext);

    /* Call the internal function */
    SepAdtPrivilegedServiceAuditAlarm(&SubjectContext,
                                      &CapturedSubsystemName,
                                      &CapturedServiceName,
                                      Token,
                                      SubjectContext.PrimaryToken,
                                      CapturedPrivileges,
                                      AccessGranted);

    /* Release the security subject context */
    SeReleaseSubjectContext(&SubjectContext);

    Status = STATUS_SUCCESS;

Cleanup:
    /* Cleanup resources */
    if (SubsystemName != NULL)
        ReleaseCapturedUnicodeString(&CapturedSubsystemName, PreviousMode);
    if (ServiceName != NULL)
        ReleaseCapturedUnicodeString(&CapturedServiceName, PreviousMode);
    if (CapturedPrivileges != NULL)
        ExFreePoolWithTag(CapturedPrivileges, 0);
    ObDereferenceObject(Token);

    return Status;
}


NTSTATUS NTAPI
NtPrivilegeObjectAuditAlarm(IN PUNICODE_STRING SubsystemName,
                            IN PVOID HandleId,
                            IN HANDLE ClientToken,
                            IN ULONG DesiredAccess,
                            IN PPRIVILEGE_SET Privileges,
                            IN BOOLEAN AccessGranted)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


_Must_inspect_result_
__kernel_entry
NTSTATUS
NTAPI
NtAccessCheckAndAuditAlarm(
    _In_ PUNICODE_STRING SubsystemName,
    _In_opt_ PVOID HandleId,
    _In_ PUNICODE_STRING ObjectTypeName,
    _In_ PUNICODE_STRING ObjectName,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PGENERIC_MAPPING GenericMapping,
    _In_ BOOLEAN ObjectCreation,
    _Out_ PACCESS_MASK GrantedAccess,
    _Out_ PNTSTATUS AccessStatus,
    _Out_ PBOOLEAN GenerateOnClose)
{
    /* Call the internal function */
    return SepAccessCheckAndAuditAlarm(SubsystemName,
                                       HandleId,
                                       NULL,
                                       ObjectTypeName,
                                       ObjectName,
                                       SecurityDescriptor,
                                       NULL,
                                       DesiredAccess,
                                       AuditEventObjectAccess,
                                       0,
                                       NULL,
                                       0,
                                       GenericMapping,
                                       GrantedAccess,
                                       AccessStatus,
                                       GenerateOnClose,
                                       FALSE);
}

_Must_inspect_result_
__kernel_entry
NTSTATUS
NTAPI
NtAccessCheckByTypeAndAuditAlarm(
    _In_ PUNICODE_STRING SubsystemName,
    _In_opt_ PVOID HandleId,
    _In_ PUNICODE_STRING ObjectTypeName,
    _In_ PUNICODE_STRING ObjectName,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_opt_ PSID PrincipalSelfSid,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ AUDIT_EVENT_TYPE AuditType,
    _In_ ULONG Flags,
    _In_reads_opt_(ObjectTypeLength) POBJECT_TYPE_LIST ObjectTypeList,
    _In_ ULONG ObjectTypeLength,
    _In_ PGENERIC_MAPPING GenericMapping,
    _In_ BOOLEAN ObjectCreation,
    _Out_ PACCESS_MASK GrantedAccess,
    _Out_ PNTSTATUS AccessStatus,
    _Out_ PBOOLEAN GenerateOnClose)
{
    /* Call the internal function */
    return SepAccessCheckAndAuditAlarm(SubsystemName,
                                       HandleId,
                                       NULL,
                                       ObjectTypeName,
                                       ObjectName,
                                       SecurityDescriptor,
                                       PrincipalSelfSid,
                                       DesiredAccess,
                                       AuditType,
                                       Flags,
                                       ObjectTypeList,
                                       ObjectTypeLength,
                                       GenericMapping,
                                       GrantedAccess,
                                       AccessStatus,
                                       GenerateOnClose,
                                       FALSE);
}

_Must_inspect_result_
__kernel_entry
NTSTATUS
NTAPI
NtAccessCheckByTypeResultListAndAuditAlarm(
    _In_ PUNICODE_STRING SubsystemName,
    _In_opt_ PVOID HandleId,
    _In_ PUNICODE_STRING ObjectTypeName,
    _In_ PUNICODE_STRING ObjectName,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_opt_ PSID PrincipalSelfSid,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ AUDIT_EVENT_TYPE AuditType,
    _In_ ULONG Flags,
    _In_reads_opt_(ObjectTypeListLength) POBJECT_TYPE_LIST ObjectTypeList,
    _In_ ULONG ObjectTypeListLength,
    _In_ PGENERIC_MAPPING GenericMapping,
    _In_ BOOLEAN ObjectCreation,
    _Out_writes_(ObjectTypeListLength) PACCESS_MASK GrantedAccessList,
    _Out_writes_(ObjectTypeListLength) PNTSTATUS AccessStatusList,
    _Out_ PBOOLEAN GenerateOnClose)
{
    /* Call the internal function */
    return SepAccessCheckAndAuditAlarm(SubsystemName,
                                       HandleId,
                                       NULL,
                                       ObjectTypeName,
                                       ObjectName,
                                       SecurityDescriptor,
                                       PrincipalSelfSid,
                                       DesiredAccess,
                                       AuditType,
                                       Flags,
                                       ObjectTypeList,
                                       ObjectTypeListLength,
                                       GenericMapping,
                                       GrantedAccessList,
                                       AccessStatusList,
                                       GenerateOnClose,
                                       TRUE);
}

_Must_inspect_result_
__kernel_entry
NTSTATUS
NTAPI
NtAccessCheckByTypeResultListAndAuditAlarmByHandle(
    _In_ PUNICODE_STRING SubsystemName,
    _In_opt_ PVOID HandleId,
    _In_ HANDLE ClientToken,
    _In_ PUNICODE_STRING ObjectTypeName,
    _In_ PUNICODE_STRING ObjectName,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_opt_ PSID PrincipalSelfSid,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ AUDIT_EVENT_TYPE AuditType,
    _In_ ULONG Flags,
    _In_reads_opt_(ObjectTypeListLength) POBJECT_TYPE_LIST ObjectTypeList,
    _In_ ULONG ObjectTypeListLength,
    _In_ PGENERIC_MAPPING GenericMapping,
    _In_ BOOLEAN ObjectCreation,
    _Out_writes_(ObjectTypeListLength) PACCESS_MASK GrantedAccessList,
    _Out_writes_(ObjectTypeListLength) PNTSTATUS AccessStatusList,
    _Out_ PBOOLEAN GenerateOnClose)
{
    UNREFERENCED_PARAMETER(ObjectCreation);

    /* Call the internal function */
    return SepAccessCheckAndAuditAlarm(SubsystemName,
                                       HandleId,
                                       &ClientToken,
                                       ObjectTypeName,
                                       ObjectName,
                                       SecurityDescriptor,
                                       PrincipalSelfSid,
                                       DesiredAccess,
                                       AuditType,
                                       Flags,
                                       ObjectTypeList,
                                       ObjectTypeListLength,
                                       GenericMapping,
                                       GrantedAccessList,
                                       AccessStatusList,
                                       GenerateOnClose,
                                       TRUE);
}

/* EOF */
