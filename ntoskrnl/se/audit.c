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

UNICODE_STRING SeSubsystemName = RTL_CONSTANT_STRING(L"Security");

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
    _In_ BOOLEAN AccessGranted)
{
    DPRINT("SepAdtPrivilegedServiceAuditAlarm is unimplemented\n");
}

VOID
NTAPI
SePrivilegedServiceAuditAlarm(
    _In_opt_ PUNICODE_STRING ServiceName,
    _In_ PSECURITY_SUBJECT_CONTEXT SubjectContext,
    _In_ PPRIVILEGE_SET PrivilegeSet,
    _In_ BOOLEAN AccessGranted)
{
    PTOKEN EffectiveToken;
    PSID UserSid;
    PAGED_CODE();

    /* Get the effective token */
    if (SubjectContext->ClientToken != NULL)
        EffectiveToken = SubjectContext->ClientToken;
    else
        EffectiveToken = SubjectContext->PrimaryToken;

    /* Get the user SID */
    UserSid = EffectiveToken->UserAndGroups->Sid;

    /* Check if this is the local system SID */
    if (RtlEqualSid(UserSid, SeLocalSystemSid))
    {
        /* Nothing to do */
        return;
    }

    /* Check if this is the network service or local service SID */
    if (RtlEqualSid(UserSid, SeExports->SeNetworkServiceSid) ||
        RtlEqualSid(UserSid, SeExports->SeLocalServiceSid))
    {
        // FIXME: should continue for a certain set of privileges
        return;
    }

    /* Call the worker function */
    SepAdtPrivilegedServiceAuditAlarm(SubjectContext,
                                      &SeSubsystemName,
                                      ServiceName,
                                      SubjectContext->ClientToken,
                                      SubjectContext->PrimaryToken,
                                      PrivilegeSet,
                                      AccessGranted);

}


static
NTSTATUS
SeCaptureObjectTypeList(
    _In_reads_opt_(ObjectTypeListLength) POBJECT_TYPE_LIST ObjectTypeList,
    _In_ ULONG ObjectTypeListLength,
    _In_ KPROCESSOR_MODE PreviousMode,
    _Out_ POBJECT_TYPE_LIST *CapturedObjectTypeList)
{
    SIZE_T Size;

    if (PreviousMode == KernelMode)
    {
        return STATUS_NOT_IMPLEMENTED;
    }

    if (ObjectTypeListLength == 0)
    {
        *CapturedObjectTypeList = NULL;
        return STATUS_SUCCESS;
    }

    if (ObjectTypeList == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Calculate the list size and check for integer overflow */
    Size = ObjectTypeListLength * sizeof(OBJECT_TYPE_LIST);
    if (Size == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Allocate a new list */
    *CapturedObjectTypeList = ExAllocatePoolWithTag(PagedPool, Size, TAG_SEPA);
    if (*CapturedObjectTypeList == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    _SEH2_TRY
    {
        ProbeForRead(ObjectTypeList, Size, sizeof(ULONG));
        RtlCopyMemory(*CapturedObjectTypeList, ObjectTypeList, Size);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ExFreePoolWithTag(*CapturedObjectTypeList, TAG_SEPA);
        *CapturedObjectTypeList = NULL;
        return _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return STATUS_SUCCESS;
}

static
VOID
SeReleaseObjectTypeList(
    _In_  _Post_invalid_ POBJECT_TYPE_LIST CapturedObjectTypeList,
    _In_ KPROCESSOR_MODE PreviousMode)
{
    if ((PreviousMode != KernelMode) && (CapturedObjectTypeList != NULL))
        ExFreePoolWithTag(CapturedObjectTypeList, TAG_SEPA);
}

_Must_inspect_result_
static
NTSTATUS
SepAccessCheckAndAuditAlarmWorker(
    _In_ PUNICODE_STRING SubsystemName,
    _In_opt_ PVOID HandleId,
    _In_ PSECURITY_SUBJECT_CONTEXT SubjectContext,
    _In_ PUNICODE_STRING ObjectTypeName,
    _In_ PUNICODE_STRING ObjectName,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_opt_ PSID PrincipalSelfSid,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ AUDIT_EVENT_TYPE AuditType,
    _In_ BOOLEAN HaveAuditPrivilege,
    _In_reads_opt_(ObjectTypeListLength) POBJECT_TYPE_LIST ObjectTypeList,
    _In_ ULONG ObjectTypeListLength,
    _In_ PGENERIC_MAPPING GenericMapping,
    _Out_writes_(ObjectTypeListLength) PACCESS_MASK GrantedAccessList,
    _Out_writes_(ObjectTypeListLength) PNTSTATUS AccessStatusList,
    _Out_ PBOOLEAN GenerateOnClose,
    _In_ BOOLEAN UseResultList)
{
    ULONG ResultListLength, i;

    /* Get the length of the result list */
    ResultListLength = UseResultList ? ObjectTypeListLength : 1;

    /// FIXME: we should do some real work here...
    UNIMPLEMENTED;

    /// HACK: we just pretend all access is granted!
    for (i = 0; i < ResultListLength; i++)
    {
        GrantedAccessList[i] = DesiredAccess;
        AccessStatusList[i] = STATUS_SUCCESS;
    }

    *GenerateOnClose = FALSE;

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
NTAPI
SepAccessCheckAndAuditAlarm(
    _In_ PUNICODE_STRING SubsystemName,
    _In_opt_ PVOID HandleId,
    _In_ PHANDLE ClientTokenHandle,
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
    PTOKEN SubjectContextToken, ClientToken;
    BOOLEAN AllocatedResultLists;
    BOOLEAN HaveAuditPrivilege;
    PSECURITY_DESCRIPTOR CapturedSecurityDescriptor;
    UNICODE_STRING CapturedSubsystemName, CapturedObjectTypeName, CapturedObjectName;
    ACCESS_MASK GrantedAccess, *SafeGrantedAccessList;
    NTSTATUS AccessStatus, *SafeAccessStatusList;
    PSID CapturedPrincipalSelfSid;
    POBJECT_TYPE_LIST CapturedObjectTypeList;
    ULONG i;
    BOOLEAN LocalGenerateOnClose;
    NTSTATUS Status;
    PAGED_CODE();

    /* Only user mode is supported! */
    ASSERT(ExGetPreviousMode() != KernelMode);

    /* Start clean */
    AllocatedResultLists = FALSE;
    ClientToken = NULL;
    CapturedSecurityDescriptor = NULL;
    CapturedSubsystemName.Buffer = NULL;
    CapturedObjectTypeName.Buffer = NULL;
    CapturedObjectName.Buffer = NULL;
    CapturedPrincipalSelfSid = NULL;
    CapturedObjectTypeList = NULL;

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
    if (ClientTokenHandle == NULL)
    {
        /* Check if we have a token in the subject context */
        if (SubjectContext.ClientToken == NULL)
        {
            Status = STATUS_NO_IMPERSONATION_TOKEN;
            DPRINT1("No token\n");
            goto Cleanup;
        }

        /* Check if we have a valid impersonation level */
        if (SubjectContext.ImpersonationLevel < SecurityIdentification)
        {
            Status = STATUS_BAD_IMPERSONATION_LEVEL;
            DPRINT1("Invalid impersonation level 0x%lx\n",
                    SubjectContext.ImpersonationLevel);
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
            DPRINT1("Invalud ResultListLength: 0x%lx\n", ResultListLength);
            goto Cleanup;
        }

        /* Allocate a safe buffer from paged pool */
        SafeGrantedAccessList = ExAllocatePoolWithTag(PagedPool,
                                                      2 * ResultListLength * sizeof(ULONG),
                                                      TAG_SEPA);
        if (SafeGrantedAccessList == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            DPRINT1("Failed to allocate access lists\n");
            goto Cleanup;
        }

        SafeAccessStatusList = (PNTSTATUS)&SafeGrantedAccessList[ResultListLength];
        AllocatedResultLists = TRUE;
    }
    else
    {
        /* List length is 1 */
        ResultListLength = 1;
        SafeGrantedAccessList = &GrantedAccess;
        SafeAccessStatusList = &AccessStatus;
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
        DPRINT1("Exception while probing parameters: 0x%lx\n", Status);
        goto Cleanup;
    }
    _SEH2_END;

    /* Do we have a client token? */
    if (ClientTokenHandle != NULL)
    {
        /* Reference the client token */
        Status = ObReferenceObjectByHandle(*ClientTokenHandle,
                                           TOKEN_QUERY,
                                           SeTokenObjectType,
                                           UserMode,
                                           (PVOID*)&ClientToken,
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to reference token handle %p: %lx\n",
                    *ClientTokenHandle, Status);
            goto Cleanup;
        }

        SubjectContextToken = SubjectContext.ClientToken;
        SubjectContext.ClientToken = ClientToken;
    }

    /* Check for audit privilege */
    HaveAuditPrivilege = SeCheckAuditPrivilege(&SubjectContext, UserMode);
    if (!HaveAuditPrivilege && !(Flags & AUDIT_ALLOW_NO_PRIVILEGE))
    {
        DPRINT1("Caller does not have SeAuditPrivilege\n");
        Status = STATUS_PRIVILEGE_NOT_HELD;
        goto Cleanup;
    }

    /* Generic access must already be mapped to non-generic access types! */
    if (DesiredAccess & (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL))
    {
        DPRINT1("Generic access rights requested: 0x%lx\n", DesiredAccess);
        Status = STATUS_GENERIC_NOT_MAPPED;
        goto Cleanup;
    }

    /* Capture the security descriptor */
    Status = SeCaptureSecurityDescriptor(SecurityDescriptor,
                                         UserMode,
                                         PagedPool,
                                         FALSE,
                                         &CapturedSecurityDescriptor);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to capture security descriptor!\n");
        goto Cleanup;
    }

    /* Validate the Security descriptor */
    if ((SepGetOwnerFromDescriptor(CapturedSecurityDescriptor) == NULL) ||
        (SepGetGroupFromDescriptor(CapturedSecurityDescriptor) == NULL))
    {
        Status = STATUS_INVALID_SECURITY_DESCR;
        DPRINT1("Invalid security descriptor\n");
        goto Cleanup;
    }

    /* Probe and capture the subsystem name */
    Status = ProbeAndCaptureUnicodeString(&CapturedSubsystemName,
                                          UserMode,
                                          SubsystemName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to capture subsystem name!\n");
        goto Cleanup;
    }

    /* Probe and capture the object type name */
    Status = ProbeAndCaptureUnicodeString(&CapturedObjectTypeName,
                                          UserMode,
                                          ObjectTypeName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to capture object type name!\n");
        goto Cleanup;
    }

    /* Probe and capture the object name */
    Status = ProbeAndCaptureUnicodeString(&CapturedObjectName,
                                          UserMode,
                                          ObjectName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to capture object name!\n");
        goto Cleanup;
    }

    /* Check if we have a PrincipalSelfSid */
    if (PrincipalSelfSid != NULL)
    {
        /* Capture it */
        Status = SepCaptureSid(PrincipalSelfSid,
                               UserMode,
                               PagedPool,
                               FALSE,
                               &CapturedPrincipalSelfSid);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to capture PrincipalSelfSid!\n");
            goto Cleanup;
        }
    }

    /* Capture the object type list */
    Status = SeCaptureObjectTypeList(ObjectTypeList,
                                     ObjectTypeListLength,
                                     UserMode,
                                     &CapturedObjectTypeList);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to capture object type list!\n");
        goto Cleanup;
    }

    /* Call the worker routine with the captured buffers */
    SepAccessCheckAndAuditAlarmWorker(&CapturedSubsystemName,
                                      HandleId,
                                      &SubjectContext,
                                      &CapturedObjectTypeName,
                                      &CapturedObjectName,
                                      CapturedSecurityDescriptor,
                                      CapturedPrincipalSelfSid,
                                      DesiredAccess,
                                      AuditType,
                                      HaveAuditPrivilege,
                                      CapturedObjectTypeList,
                                      ObjectTypeListLength,
                                      &LocalGenericMapping,
                                      SafeGrantedAccessList,
                                      SafeAccessStatusList,
                                      &LocalGenerateOnClose,
                                      UseResultList);

    /* Enter SEH to copy the data back to user mode */
    _SEH2_TRY
    {
        /* Loop all result entries (only 1 when no list was requested) */
        ASSERT(UseResultList || (ResultListLength == 1));
        for (i = 0; i < ResultListLength; i++)
        {
            AccessStatusList[i] = SafeAccessStatusList[i];
            GrantedAccessList[i] = SafeGrantedAccessList[i];
        }

        *GenerateOnClose = LocalGenerateOnClose;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
        DPRINT1("Exception while copying back data: 0x%lx\n", Status);
    }
    _SEH2_END;

Cleanup:

    if (CapturedObjectTypeList != NULL)
        SeReleaseObjectTypeList(CapturedObjectTypeList, UserMode);

    if (CapturedPrincipalSelfSid != NULL)
        SepReleaseSid(CapturedPrincipalSelfSid, UserMode, FALSE);

    if (CapturedObjectName.Buffer != NULL)
        ReleaseCapturedUnicodeString(&CapturedObjectName, UserMode);

    if (CapturedObjectTypeName.Buffer != NULL)
        ReleaseCapturedUnicodeString(&CapturedObjectTypeName, UserMode);

    if (CapturedSubsystemName.Buffer != NULL)
        ReleaseCapturedUnicodeString(&CapturedSubsystemName, UserMode);

    if (CapturedSecurityDescriptor != NULL)
        SeReleaseSecurityDescriptor(CapturedSecurityDescriptor, UserMode, FALSE);

    if (ClientToken != NULL)
    {
        ObDereferenceObject(ClientToken);
        SubjectContext.ClientToken = SubjectContextToken;
    }

    if (AllocatedResultLists)
        ExFreePoolWithTag(SafeGrantedAccessList, TAG_SEPA);

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
    SECURITY_SUBJECT_CONTEXT SubjectContext;
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

    /* Capture the security subject context */
    SeCaptureSubjectContext(&SubjectContext);

    /* Check for audit privilege */
    if (!SeCheckAuditPrivilege(&SubjectContext, PreviousMode))
    {
        DPRINT1("Caller does not have SeAuditPrivilege\n");
        Status = STATUS_PRIVILEGE_NOT_HELD;
        goto Cleanup;
    }

    /* Probe and capture the subsystem name */
    Status = ProbeAndCaptureUnicodeString(&CapturedSubsystemName,
                                          PreviousMode,
                                          SubsystemName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to capture subsystem name!\n");
        goto Cleanup;
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

    Status = STATUS_SUCCESS;

Cleanup:

    /* Release the security subject context */
    SeReleaseSubjectContext(&SubjectContext);

    return Status;
}


NTSTATUS NTAPI
NtDeleteObjectAuditAlarm(IN PUNICODE_STRING SubsystemName,
                         IN PVOID HandleId,
                         IN BOOLEAN GenerateOnClose)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
SepOpenObjectAuditAlarm(
    _In_ PSECURITY_SUBJECT_CONTEXT SubjectContext,
    _In_ PUNICODE_STRING SubsystemName,
    _In_opt_ PVOID HandleId,
    _In_ PUNICODE_STRING ObjectTypeName,
    _In_ PUNICODE_STRING ObjectName,
    _In_opt_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ PTOKEN ClientToken,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ACCESS_MASK GrantedAccess,
    _In_opt_ PPRIVILEGE_SET Privileges,
    _In_ BOOLEAN ObjectCreation,
    _In_ BOOLEAN AccessGranted,
    _Out_ PBOOLEAN GenerateOnClose)
{
    DBG_UNREFERENCED_PARAMETER(SubjectContext);
    DBG_UNREFERENCED_PARAMETER(SubsystemName);
    DBG_UNREFERENCED_PARAMETER(HandleId);
    DBG_UNREFERENCED_PARAMETER(ObjectTypeName);
    DBG_UNREFERENCED_PARAMETER(ObjectName);
    DBG_UNREFERENCED_PARAMETER(SecurityDescriptor);
    DBG_UNREFERENCED_PARAMETER(ClientToken);
    DBG_UNREFERENCED_PARAMETER(DesiredAccess);
    DBG_UNREFERENCED_PARAMETER(GrantedAccess);
    DBG_UNREFERENCED_PARAMETER(Privileges);
    DBG_UNREFERENCED_PARAMETER(ObjectCreation);
    DBG_UNREFERENCED_PARAMETER(AccessGranted);
    UNIMPLEMENTED;
    *GenerateOnClose = FALSE;
}

__kernel_entry
NTSTATUS
NTAPI
NtOpenObjectAuditAlarm(
    _In_ PUNICODE_STRING SubsystemName,
    _In_opt_ PVOID HandleId,
    _In_ PUNICODE_STRING ObjectTypeName,
    _In_ PUNICODE_STRING ObjectName,
    _In_opt_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ HANDLE ClientTokenHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ACCESS_MASK GrantedAccess,
    _In_opt_ PPRIVILEGE_SET PrivilegeSet,
    _In_ BOOLEAN ObjectCreation,
    _In_ BOOLEAN AccessGranted,
    _Out_ PBOOLEAN GenerateOnClose)
{
    PTOKEN ClientToken;
    PSECURITY_DESCRIPTOR CapturedSecurityDescriptor;
    UNICODE_STRING CapturedSubsystemName, CapturedObjectTypeName, CapturedObjectName;
    ULONG PrivilegeCount, PrivilegeSetSize;
    volatile PPRIVILEGE_SET CapturedPrivilegeSet;
    BOOLEAN LocalGenerateOnClose;
    PVOID CapturedHandleId;
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    NTSTATUS Status;
    PAGED_CODE();

    /* Only user mode is supported! */
    ASSERT(ExGetPreviousMode() != KernelMode);

    /* Start clean */
    ClientToken = NULL;
    CapturedSecurityDescriptor = NULL;
    CapturedPrivilegeSet = NULL;
    CapturedSubsystemName.Buffer = NULL;
    CapturedObjectTypeName.Buffer = NULL;
    CapturedObjectName.Buffer = NULL;

    /* Reference the client token */
    Status = ObReferenceObjectByHandle(ClientTokenHandle,
                                       TOKEN_QUERY,
                                       SeTokenObjectType,
                                       UserMode,
                                       (PVOID*)&ClientToken,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to reference token handle %p: %lx\n",
                ClientTokenHandle, Status);
        return Status;
    }

    /* Capture the security subject context */
    SeCaptureSubjectContext(&SubjectContext);

    /* Validate the token's impersonation level */
    if ((ClientToken->TokenType == TokenImpersonation) &&
        (ClientToken->ImpersonationLevel < SecurityIdentification))
    {
        DPRINT1("Invalid impersonation level (%u)\n", ClientToken->ImpersonationLevel);
        Status = STATUS_BAD_IMPERSONATION_LEVEL;
        goto Cleanup;
    }

    /* Check for audit privilege */
    if (!SeCheckAuditPrivilege(&SubjectContext, UserMode))
    {
        DPRINT1("Caller does not have SeAuditPrivilege\n");
        Status = STATUS_PRIVILEGE_NOT_HELD;
        goto Cleanup;
    }

    /* Check for NULL SecurityDescriptor */
    if (SecurityDescriptor == NULL)
    {
        /* Nothing to do */
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    /* Capture the security descriptor */
    Status = SeCaptureSecurityDescriptor(SecurityDescriptor,
                                         UserMode,
                                         PagedPool,
                                         FALSE,
                                         &CapturedSecurityDescriptor);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to capture security descriptor!\n");
        goto Cleanup;
    }

    _SEH2_TRY
    {
        /* Check if we have a privilege set */
        if (PrivilegeSet != NULL)
        {
            /* Probe the basic privilege set structure */
            ProbeForRead(PrivilegeSet, sizeof(PRIVILEGE_SET), sizeof(ULONG));

            /* Validate privilege count */
            PrivilegeCount = PrivilegeSet->PrivilegeCount;
            if (PrivilegeCount > SEP_PRIVILEGE_SET_MAX_COUNT)
            {
                Status = STATUS_INVALID_PARAMETER;
                goto Cleanup;
            }

            /* Calculate the size of the PrivilegeSet structure */
            PrivilegeSetSize = FIELD_OFFSET(PRIVILEGE_SET, Privilege[PrivilegeCount]);

            /* Probe the whole structure */
            ProbeForRead(PrivilegeSet, PrivilegeSetSize, sizeof(ULONG));

            /* Allocate a temp buffer */
            CapturedPrivilegeSet = ExAllocatePoolWithTag(PagedPool,
                                                         PrivilegeSetSize,
                                                         TAG_PRIVILEGE_SET);
            if (CapturedPrivilegeSet == NULL)
            {
                DPRINT1("Failed to allocate %u bytes\n", PrivilegeSetSize);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }

            /* Copy the privileges */
            RtlCopyMemory(CapturedPrivilegeSet, PrivilegeSet, PrivilegeSetSize);
        }

        if (HandleId != NULL)
        {
            ProbeForRead(HandleId, sizeof(PVOID), sizeof(PVOID));
            CapturedHandleId = *(PVOID*)HandleId;
        }

        ProbeForWrite(GenerateOnClose, sizeof(BOOLEAN), sizeof(BOOLEAN));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
        DPRINT1("Exception while probing parameters: 0x%lx\n", Status);
        goto Cleanup;
    }
    _SEH2_END;

    /* Probe and capture the subsystem name */
    Status = ProbeAndCaptureUnicodeString(&CapturedSubsystemName,
                                          UserMode,
                                          SubsystemName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to capture subsystem name!\n");
        goto Cleanup;
    }

    /* Probe and capture the object type name */
    Status = ProbeAndCaptureUnicodeString(&CapturedObjectTypeName,
                                          UserMode,
                                          ObjectTypeName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to capture object type name!\n");
        goto Cleanup;
    }

    /* Probe and capture the object name */
    Status = ProbeAndCaptureUnicodeString(&CapturedObjectName,
                                          UserMode,
                                          ObjectName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to capture object name!\n");
        goto Cleanup;
    }

    /* Call the internal function */
    SepOpenObjectAuditAlarm(&SubjectContext,
                            &CapturedSubsystemName,
                            CapturedHandleId,
                            &CapturedObjectTypeName,
                            &CapturedObjectName,
                            CapturedSecurityDescriptor,
                            ClientToken,
                            DesiredAccess,
                            GrantedAccess,
                            CapturedPrivilegeSet,
                            ObjectCreation,
                            AccessGranted,
                            &LocalGenerateOnClose);

    Status = STATUS_SUCCESS;

    /* Enter SEH to copy the data back to user mode */
    _SEH2_TRY
    {
        *GenerateOnClose = LocalGenerateOnClose;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
        DPRINT1("Exception while copying back data: 0x%lx\n", Status);
    }
    _SEH2_END;

Cleanup:

    if (CapturedObjectName.Buffer != NULL)
        ReleaseCapturedUnicodeString(&CapturedObjectName, UserMode);

    if (CapturedObjectTypeName.Buffer != NULL)
        ReleaseCapturedUnicodeString(&CapturedObjectTypeName, UserMode);

    if (CapturedSubsystemName.Buffer != NULL)
        ReleaseCapturedUnicodeString(&CapturedSubsystemName, UserMode);

    if (CapturedSecurityDescriptor != NULL)
        SeReleaseSecurityDescriptor(CapturedSecurityDescriptor, UserMode, FALSE);

    if (CapturedPrivilegeSet != NULL)
        ExFreePoolWithTag(CapturedPrivilegeSet, TAG_PRIVILEGE_SET);

    /* Release the security subject context */
    SeReleaseSubjectContext(&SubjectContext);

    ObDereferenceObject(ClientToken);

    return Status;
}


__kernel_entry
NTSTATUS
NTAPI
NtPrivilegedServiceAuditAlarm(
    _In_opt_ PUNICODE_STRING SubsystemName,
    _In_opt_ PUNICODE_STRING ServiceName,
    _In_ HANDLE ClientTokenHandle,
    _In_ PPRIVILEGE_SET Privileges,
    _In_ BOOLEAN AccessGranted )
{
    KPROCESSOR_MODE PreviousMode;
    PTOKEN ClientToken;
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

    CapturedSubsystemName.Buffer = NULL;
    CapturedServiceName.Buffer = NULL;

    /* Reference the client token */
    Status = ObReferenceObjectByHandle(ClientTokenHandle,
                                       TOKEN_QUERY,
                                       SeTokenObjectType,
                                       PreviousMode,
                                       (PVOID*)&ClientToken,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to reference client token: 0x%lx\n", Status);
        return Status;
    }

    /* Validate the token's impersonation level */
    if ((ClientToken->TokenType == TokenImpersonation) &&
        (ClientToken->ImpersonationLevel < SecurityIdentification))
    {
        DPRINT1("Invalid impersonation level (%u)\n", ClientToken->ImpersonationLevel);
        ObDereferenceObject(ClientToken);
        return STATUS_BAD_IMPERSONATION_LEVEL;
    }

    /* Capture the security subject context */
    SeCaptureSubjectContext(&SubjectContext);

    /* Check for audit privilege */
    if (!SeCheckAuditPrivilege(&SubjectContext, PreviousMode))
    {
        DPRINT1("Caller does not have SeAuditPrivilege\n");
        Status = STATUS_PRIVILEGE_NOT_HELD;
        goto Cleanup;
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
                                                   TAG_PRIVILEGE_SET);
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

    /* Call the internal function */
    SepAdtPrivilegedServiceAuditAlarm(&SubjectContext,
                                      SubsystemName ? &CapturedSubsystemName : NULL,
                                      ServiceName ? &CapturedServiceName : NULL,
                                      ClientToken,
                                      SubjectContext.PrimaryToken,
                                      CapturedPrivileges,
                                      AccessGranted);

    Status = STATUS_SUCCESS;

Cleanup:
    /* Cleanup resources */
    if (CapturedSubsystemName.Buffer != NULL)
        ReleaseCapturedUnicodeString(&CapturedSubsystemName, PreviousMode);

    if (CapturedServiceName.Buffer != NULL)
        ReleaseCapturedUnicodeString(&CapturedServiceName, PreviousMode);

    if (CapturedPrivileges != NULL)
        ExFreePoolWithTag(CapturedPrivileges, TAG_PRIVILEGE_SET);

    /* Release the security subject context */
    SeReleaseSubjectContext(&SubjectContext);

    ObDereferenceObject(ClientToken);

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
