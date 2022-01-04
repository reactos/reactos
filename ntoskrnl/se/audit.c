/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Security auditing functions
 * COPYRIGHT:       Copyright Eric Kohl
 *                  Copyright Timo Kreuzer <timo.kreuzer@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define SEP_PRIVILEGE_SET_MAX_COUNT 60

UNICODE_STRING SeSubsystemName = RTL_CONSTANT_STRING(L"Security");

/* PRIVATE FUNCTIONS***********************************************************/

/**
 * @unimplemented
 * @brief
 * Peforms a detailed security auditing with an access token.
 *
 * @param[in] Token
 * A valid token object.
 *
 * @return
 * To be added...
 */
BOOLEAN
NTAPI
SeDetailedAuditingWithToken(
    _In_ PTOKEN Token)
{
    /* FIXME */
    return FALSE;
}

/**
 * @unimplemented
 * @brief
 * Peforms a security auditing against a process that is about to
 * be created.
 *
 * @param[in] Process
 * An object that points to a process which is in process of
 * creation.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
SeAuditProcessCreate(
    _In_ PEPROCESS Process)
{
    /* FIXME */
}

/**
 * @unimplemented
 * @brief
 * Peforms a security auditing against a process that is about to
 * be terminated.
 *
 * @param[in] Process
 * An object that points to a process which is in process of
 * termination.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
SeAuditProcessExit(
    _In_ PEPROCESS Process)
{
    /* FIXME */
}

/**
 * @brief
 * Initializes a process audit name and returns it to the caller.
 *
 * @param[in] FileObject
 * File object that points to a name to be queried.
 *
 * @param[in] DoAudit
 * If set to TRUE, the function will perform various security
 * auditing onto the audit name.
 *
 * @param[out] AuditInfo
 * The returned audit info data.
 *
 * @return
 * Returns STATUS_SUCCESS if process audit name initialization
 * has completed successfully. STATUS_NO_MEMORY is returned if
 * pool allocation for object name info has failed. A failure
 * NTSTATUS code is returned otherwise.
 */
NTSTATUS
NTAPI
SeInitializeProcessAuditName(
    _In_ PFILE_OBJECT FileObject,
    _In_ BOOLEAN DoAudit,
    _Out_ POBJECT_NAME_INFORMATION *AuditInfo)
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

/**
 * @brief
 * Finds the process image name of a specific process.
 *
 * @param[in] Process
 * Process object submitted by the caller, where the image name
 * is to be located.
 *
 * @param[out] ProcessImageName
 * An output Unicode string structure with the located process
 * image name.
 *
 * @return
 * Returns STATUS_SUCCESS if process image name has been located
 * successfully. STATUS_NO_MEMORY is returned if pool allocation
 * for the image name has failed. A failure NTSTATUS code is
 * returned otherwise.
 */
NTSTATUS
NTAPI
SeLocateProcessImageName(
    _In_ PEPROCESS Process,
    _Out_ PUNICODE_STRING *ProcessImageName)
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

/**
 * @brief
 * Closes an audit alarm event of an object.
 *
 * @param[in] SubsystemName
 * A Unicode string pointing to the name of the subsystem where auditing
 * alarm event has to be closed.
 *
 * @param[in] HandleId
 * A handle to an ID where such ID represents the identification of the
 * object where audit alarm is to be closed.
 *
 * @param[in] Sid
 * A SID that represents the user who attempted to close the audit
 * alarm.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
SepAdtCloseObjectAuditAlarm(
    _In_ PUNICODE_STRING SubsystemName,
    _In_ PVOID HandleId,
    _In_ PSID Sid)
{
    UNIMPLEMENTED;
}

/**
 * @brief
 * Performs an audit alarm to a privileged service request.
 * This is a worker function.
 *
 * @param[in] SubjectContext
 * A security subject context used for the auditing process.
 *
 * @param[in] SubsystemName
 * A Unicode string that represents the name of a subsystem that
 * actuated the procedure of alarm auditing of a privileged
 * service.
 *
 * @param[in] ServiceName
 * A Unicode string that represents the name of a privileged
 * service request for auditing.
 *
 * @param[in] Token
 * An access token.
 *
 * @param[in] PrimaryToken
 * A primary access token.
 *
 * @param[in] Privileges
 * An array set of privileges used to check if the privileged
 * service does actually have all the required set of privileges
 * for security access.
 *
 * @param[in] AccessGranted
 * When auditing is done, the function will return TRUE to the caller
 * if access is granted, FALSE otherwise.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
SepAdtPrivilegedServiceAuditAlarm(
    _In_ PSECURITY_SUBJECT_CONTEXT SubjectContext,
    _In_opt_ PUNICODE_STRING SubsystemName,
    _In_opt_ PUNICODE_STRING ServiceName,
    _In_ PTOKEN Token,
    _In_ PTOKEN PrimaryToken,
    _In_ PPRIVILEGE_SET Privileges,
    _In_ BOOLEAN AccessGranted)
{
    DPRINT("SepAdtPrivilegedServiceAuditAlarm is unimplemented\n");
}

/**
 * @brief
 * Performs an audit alarm to a privileged service request.
 *
 * @param[in] ServiceName
 * A Unicode string that represents the name of a privileged
 * service request for auditing.
 *
 * @param[in] SubjectContext
 * A security subject context used for the auditing process.
 *
 * @param[in] PrivilegeSet
 * An array set of privileges used to check if the privileged
 * service does actually have all the required set of privileges
 * for security access.
 *
 * @param[in] AccessGranted
 * When auditing is done, the function will return TRUE to the caller
 * if access is granted, FALSE otherwise.
 *
 * @return
 * Nothing.
 */
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

/**
 * @brief
 * Captures a list of object types.
 *
 * @param[in] ObjectTypeList
 * An existing list of object types.
 *
 * @param[in] ObjectTypeListLength
 * The length size of the list.
 *
 * @param[in] PreviousMode
 * Processor access level mode.
 *
 * @param[out] CapturedObjectTypeList
 * The captured list of object types.
 *
 * @return
 * Returns STATUS_SUCCESS if the list of object types has been captured
 * successfully. STATUS_INVALID_PARAMETER is returned if the caller hasn't
 * supplied a buffer list of object types. STATUS_INSUFFICIENT_RESOURCES
 * is returned if pool memory allocation for the captured list has failed.
 */
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
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Releases a buffer list of object types.
 *
 * @param[in] CapturedObjectTypeList
 * A list of object types to free.
 *
 * @param[in] PreviousMode
 * Processor access level mode.
 *
 * @return
 * Nothing.
 */
static
VOID
SeReleaseObjectTypeList(
    _In_  _Post_invalid_ POBJECT_TYPE_LIST CapturedObjectTypeList,
    _In_ KPROCESSOR_MODE PreviousMode)
{
    if ((PreviousMode != KernelMode) && (CapturedObjectTypeList != NULL))
        ExFreePoolWithTag(CapturedObjectTypeList, TAG_SEPA);
}

/**
 * @unimplemented
 * @brief
 * Worker function that serves as the main heart and brain of the whole
 * concept and implementation of auditing in the kernel.
 *
 * @param[in] SubsystemName
 * A Unicode string that represents the name of a subsystem that
 * actuates the auditing process.
 *
 * @param[in] HandleId
 * A handle to an ID used to identify an object where auditing
 * is to be done.
 *
 * @param[in] SubjectContext
 * Security subject context.
 *
 * @param[in] ObjectTypeName
 * A Unicode string that represents the name of an object type.
 *
 * @param[in] ObjectName
 * The name of the object.
 *
 * @param[in] SecurityDescriptor
 * A security descriptor with internal security information details
 * for audit.
 *
 * @param[in] PrincipalSelfSid
 * A principal self user SID.
 *
 * @param[in] DesiredAccess
 * The desired access rights masks requested by the caller.
 *
 * @param[in] AuditType
 * Type of audit to start. This parameter influences how an audit
 * should be done.
 *
 * @param[in] HaveAuditPrivilege
 * If set to TRUE, the security subject context has the audit privilege thus
 * it is allowed the ability to perform the audit.
 *
 * @param[in] ObjectTypeList
 * A list of object types.
 *
 * @param[in] ObjectTypeListLength
 * The length size of the list.
 *
 * @param[in] GenericMapping
 * The generic mapping table of access rights used whilst performing auditing
 * sequence procedure.
 *
 * @param[out] GrantedAccessList
 * This parameter is used to return to the caller a list of actual granted access
 * rights masks that the audited object has.
 *
 * @param[out] AccessStatusList
 * This parameter is used to return to the caller a list of status return codes.
 * The function may actually return a single NTSTATUS code if the calling thread
 * sets UseResultList parameter to FALSE.
 *
 * @param[out] GenerateOnClose
 * Returns TRUE if the function has generated a list of granted access rights and
 * status codes on termination, FALSE otherwise.
 *
 * @param[in] UseResultList
 * If set to TRUE, the caller wants that the function should only return a single
 * NTSTATUS code.
 *
 * @return
 * Returns STATUS_SUCCESS if the function has completed the whole internal
 * auditing procedure mechanism with success.
 */
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

/**
 * @brief
 * Performs security auditing, if the specific object can be granted
 * security access or not.
 *
 * @param[in] SubsystemName
 * A Unicode string that represents the name of a subsystem that
 * actuates the auditing process.
 *
 * @param[in] HandleId
 * A handle to an ID used to identify an object where auditing
 * is to be done.
 *
 * @param[in] SubjectContext
 * Security subject context.
 *
 * @param[in] ObjectTypeName
 * A Unicode string that represents the name of an object type.
 *
 * @param[in] ObjectName
 * The name of the object.
 *
 * @param[in] SecurityDescriptor
 * A security descriptor with internal security information details
 * for audit.
 *
 * @param[in] PrincipalSelfSid
 * A principal self user SID.
 *
 * @param[in] DesiredAccess
 * The desired access rights masks requested by the caller.
 *
 * @param[in] AuditType
 * Type of audit to start. This parameter influences how an audit
 * should be done.
 *
 * @param[in] Flags
 * Flag bitmask parameter.
 *
 * @param[in] HaveAuditPrivilege
 * If set to TRUE, the security subject context has the audit privilege thus
 * it is allowed the ability to perform the audit.
 *
 * @param[in] ObjectTypeList
 * A list of object types.
 *
 * @param[in] ObjectTypeListLength
 * The length size of the list.
 *
 * @param[in] GenericMapping
 * The generic mapping table of access rights used whilst performing auditing
 * sequence procedure.
 *
 * @param[out] GrantedAccessList
 * This parameter is used to return to the caller a list of actual granted access
 * rights masks that the audited object has.
 *
 * @param[out] AccessStatusList
 * This parameter is used to return to the caller a list of status return codes.
 * The function may actually return a single NTSTATUS code if the calling thread
 * sets UseResultList parameter to FALSE.
 *
 * @param[out] GenerateOnClose
 * Returns TRUE if the function has generated a list of granted access rights and
 * status codes on termination, FALSE otherwise.
 *
 * @param[in] UseResultList
 * If set to TRUE, the caller wants that the function should only return a single
 * NTSTATUS code.
 *
 * @return
 * Returns STATUS_SUCCESS if the function has completed the whole internal
 * auditing procedure mechanism with success. STATUS_INVALID_PARAMETER is
 * returned if one of the parameters do not satisfy the general requirements
 * by the function. STATUS_INSUFFICIENT_RESOURCES is returned if pool memory
 * allocation has failed. STATUS_PRIVILEGE_NOT_HELD is returned if the current
 * security subject context does not have the required audit privilege to actually
 * perform auditing in the first place. STATUS_INVALID_SECURITY_DESCR is returned
 * if the security descriptor provided by the caller is not valid, that is, such
 * descriptor doesn't belong to the main user (owner) and current group.
 * STATUS_GENERIC_NOT_MAPPED is returned if the access rights masks aren't actually
 * mapped. A failure NTSTATUS code is returned otherwise.
 */
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
            DPRINT1("Invalid ResultListLength: 0x%lx\n", ResultListLength);
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
        _SEH2_YIELD(goto Cleanup);
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
    Status = SepAccessCheckAndAuditAlarmWorker(&CapturedSubsystemName,
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
    if (!NT_SUCCESS(Status))
        goto Cleanup;

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

/**
 * @unimplemented
 * @brief
 * Performs an audit against a hard link creation.
 *
 * @param[in] FileName
 * A Unicode string that points to the name of the file.
 *
 * @param[in] LinkName
 * A Unicode string that points to a link.
 *
 * @param[out] bSuccess
 * If TRUE, the function has successfully audited
 * the hard link and security access can be granted,
 * FALSE otherwise.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
SeAuditHardLinkCreation(
    _In_ PUNICODE_STRING FileName,
    _In_ PUNICODE_STRING LinkName,
    _In_ BOOLEAN bSuccess)
{
    UNIMPLEMENTED;
}

/**
 * @unimplemented
 * @brief
 * Determines whether auditing against file events is being
 * done or not.
 *
 * @param[in] AccessGranted
 * If set to TRUE, the access attempt is deemed as successful
 * otherwise set it to FALSE.
 *
 * @param[in] SecurityDescriptor
 * A security descriptor.
 *
 * @return
 * Returns TRUE if auditing is being currently done, FALSE otherwise.
 */
BOOLEAN
NTAPI
SeAuditingFileEvents(
    _In_ BOOLEAN AccessGranted,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor)
{
    UNIMPLEMENTED;
    return FALSE;
}

/**
 * @unimplemented
 * @brief
 * Determines whether auditing against file events with subject context
 * is being done or not.
 *
 * @param[in] AccessGranted
 * If set to TRUE, the access attempt is deemed as successful
 * otherwise set it to FALSE.
 *
 * @param[in] SecurityDescriptor
 * A security descriptor.
 *
 * @param[in] SubjectSecurityContext
 * If specified, the function will check if security auditing is currently
 * being done with this context.
 *
 * @return
 * Returns TRUE if auditing is being currently done, FALSE otherwise.
 */
BOOLEAN
NTAPI
SeAuditingFileEventsWithContext(
    _In_ BOOLEAN AccessGranted,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_opt_ PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext)
{
    UNIMPLEMENTED_ONCE;
    return FALSE;
}

/**
 * @unimplemented
 * @brief
 * Determines whether auditing against hard links events is being
 * done or not.
 *
 * @param[in] AccessGranted
 * If set to TRUE, the access attempt is deemed as successful
 * otherwise set it to FALSE.
 *
 * @param[in] SecurityDescriptor
 * A security descriptor.
 *
 * @return
 * Returns TRUE if auditing is being currently done, FALSE otherwise.
 */
BOOLEAN
NTAPI
SeAuditingHardLinkEvents(
    _In_ BOOLEAN AccessGranted,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor)
{
    UNIMPLEMENTED;
    return FALSE;
}

/**
 * @unimplemented
 * @brief
 * Determines whether auditing against hard links events with subject context
 * is being done or not.
 *
 * @param[in] AccessGranted
 * If set to TRUE, the access attempt is deemed as successful
 * otherwise set it to FALSE.
 *
 * @param[in] SecurityDescriptor
 * A security descriptor.
 *
 * @param[in] SubjectSecurityContext
 * If specified, the function will check if security auditing is currently
 * being done with this context.
 *
 * @return
 * Returns TRUE if auditing is being currently done, FALSE otherwise.
 */
BOOLEAN
NTAPI
SeAuditingHardLinkEventsWithContext(
    _In_ BOOLEAN AccessGranted,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_opt_ PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext)
{
    UNIMPLEMENTED;
    return FALSE;
}

/**
 * @unimplemented
 * @brief
 * Determines whether auditing against files or global events with
 * subject context is being done or not.
 *
 * @param[in] AccessGranted
 * If set to TRUE, the access attempt is deemed as successful
 * otherwise set it to FALSE.
 *
 * @param[in] SecurityDescriptor
 * A security descriptor.
 *
 * @param[in] SubjectSecurityContext
 * If specified, the function will check if security auditing is currently
 * being done with this context.
 *
 * @return
 * Returns TRUE if auditing is being currently done, FALSE otherwise.
 */
BOOLEAN
NTAPI
SeAuditingFileOrGlobalEvents(
    _In_ BOOLEAN AccessGranted,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext)
{
    UNIMPLEMENTED;
    return FALSE;
}

/**
 * @unimplemented
 * @brief
 * Closes an alarm audit of an object.
 *
 * @param[in] Object
 * An arbitrary pointer data that points to the object.
 *
 * @param[in] Handle
 * A handle of the said object.
 *
 * @param[in] PerformAction
 * Set this to TRUE to perform any auxiliary action, otherwise
 * set to FALSE.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
SeCloseObjectAuditAlarm(
    _In_ PVOID Object,
    _In_ HANDLE Handle,
    _In_ BOOLEAN PerformAction)
{
    UNIMPLEMENTED;
}

/**
 * @unimplemented
 * @brief
 * Deletes an alarm audit of an object.
 *
 * @param[in] Object
 * An arbitrary pointer data that points to the object.
 *
 * @param[in] Handle
 * A handle of the said object.
 *
 * @return
 * Nothing.
 */
VOID NTAPI
SeDeleteObjectAuditAlarm(
    _In_ PVOID Object,
    _In_ HANDLE Handle)
{
    UNIMPLEMENTED;
}

/**
 * @unimplemented
 * @brief
 * Creates an audit with alarm notification of an object
 * that is being opened.
 *
 * @param[in] ObjectTypeName
 * A Unicode string that points to the object type name.
 *
 * @param[in] Object
 * If specified, the function will use this parameter to
 * directly open the object.
 *
 * @param[in] AbsoluteObjectName
 * If specified, the function will use this parameter to
 * directly open the object through the absolute name
 * of the object.
 *
 * @param[in] SecurityDescriptor
 * A security descriptor.
 *
 * @param[in] AccessState
 * An access state right mask when opening the object.
 *
 * @param[in] ObjectCreated
 * Set this to TRUE if the object has been fully created,
 * FALSE otherwise.
 *
 * @param[in] AccessGranted
 * Set this to TRUE if access was deemed as granted.
 *
 * @param[in] AccessMode
 * Processor level access mode.
 *
 * @param[out] GenerateOnClose
 * A boolean flag returned to the caller once audit generation procedure
 * finishes.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
SeOpenObjectAuditAlarm(
    _In_ PUNICODE_STRING ObjectTypeName,
    _In_opt_ PVOID Object,
    _In_opt_ PUNICODE_STRING AbsoluteObjectName,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ PACCESS_STATE AccessState,
    _In_ BOOLEAN ObjectCreated,
    _In_ BOOLEAN AccessGranted,
    _In_ KPROCESSOR_MODE AccessMode,
    _Out_ PBOOLEAN GenerateOnClose)
{
    PAGED_CODE();

    /* Audits aren't done on kernel-mode access */
    if (AccessMode == KernelMode) return;

    /* Otherwise, unimplemented! */
    //UNIMPLEMENTED;
    return;
}

/**
 * @unimplemented
 * @brief
 * Creates an audit with alarm notification of an object
 * that is being opened for deletion.
 *
 * @param[in] ObjectTypeName
 * A Unicode string that points to the object type name.
 *
 * @param[in] Object
 * If specified, the function will use this parameter to
 * directly open the object.
 *
 * @param[in] AbsoluteObjectName
 * If specified, the function will use this parameter to
 * directly open the object through the absolute name
 * of the object.
 *
 * @param[in] SecurityDescriptor
 * A security descriptor.
 *
 * @param[in] AccessState
 * An access state right mask when opening the object.
 *
 * @param[in] ObjectCreated
 * Set this to TRUE if the object has been fully created,
 * FALSE otherwise.
 *
 * @param[in] AccessGranted
 * Set this to TRUE if access was deemed as granted.
 *
 * @param[in] AccessMode
 * Processor level access mode.
 *
 * @param[out] GenerateOnClose
 * A boolean flag returned to the caller once audit generation procedure
 * finishes.
 *
 * @return
 * Nothing.
 */
VOID NTAPI
SeOpenObjectForDeleteAuditAlarm(
    _In_ PUNICODE_STRING ObjectTypeName,
    _In_opt_ PVOID Object,
    _In_opt_ PUNICODE_STRING AbsoluteObjectName,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ PACCESS_STATE AccessState,
    _In_ BOOLEAN ObjectCreated,
    _In_ BOOLEAN AccessGranted,
    _In_ KPROCESSOR_MODE AccessMode,
    _Out_ PBOOLEAN GenerateOnClose)
{
    UNIMPLEMENTED;
}

/**
 * @unimplemented
 * @brief
 * Raises an audit with alarm notification message
 * when an object tries to acquire this privilege.
 *
 * @param[in] Handle
 * A handle to an object.
 *
 * @param[in] SubjectContext
 * The security subject context for auditing.
 *
 * @param[in] DesiredAccess
 * The desired right access masks requested by the caller.
 *
 * @param[in] Privileges
 * An array set of privileges for auditing.
 *
 * @param[out] AccessGranted
 * When the auditing procedure routine ends, it returns TRUE to the
 * caller if the object has the required privileges for access,
 * FALSE otherwise.
 *
 * @param[in] CurrentMode
 * Processor level access mode.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
SePrivilegeObjectAuditAlarm(
    _In_ HANDLE Handle,
    _In_ PSECURITY_SUBJECT_CONTEXT SubjectContext,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PPRIVILEGE_SET Privileges,
    _In_ BOOLEAN AccessGranted,
    _In_ KPROCESSOR_MODE CurrentMode)
{
    UNIMPLEMENTED;
}

/* SYSTEM CALLS ***************************************************************/

/**
 * @brief
 * Raises an alarm audit message when an object is about
 * to be closed.
 *
 * @param[in] SubsystemName
 * A Unicode string that points to the name of the subsystem.
 *
 * @param[in] HandleId
 * A handle of an ID used for identification instance for auditing.
 *
 * @param[in] GenerateOnClose
 * A boolean value previously created by the "open" equivalent of this
 * function. If the caller explicitly sets this to FALSE, the function
 * assumes that the object is not opened.
 *
 * @return
 * Returns STATUS_SUCCESS if all the operations have completed successfully.
 * STATUS_PRIVILEGE_NOT_HELD is returned if the security subject context
 * does not have the audit privilege to actually begin auditing procedures
 * in the first place.
 */
NTSTATUS
NTAPI
NtCloseObjectAuditAlarm(
    _In_ PUNICODE_STRING SubsystemName,
    _In_ PVOID HandleId,
    _In_ BOOLEAN GenerateOnClose)
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

/**
 * @unimplemented
 * @brief
 * Raises an alarm audit message when an object is about
 * to be deleted.
 *
 * @param[in] SubsystemName
 * A Unicode string that points to the name of the subsystem.
 *
 * @param[in] HandleId
 * A handle of an ID used for identification instance for auditing.
 *
 * @param[in] GenerateOnClose
 * A boolean value previously created by the "open" equivalent of this
 * function. If the caller explicitly sets this to FALSE, the function
 * assumes that the object is not opened.
 *
 * @return
 * To be added...
 */
NTSTATUS NTAPI
NtDeleteObjectAuditAlarm(
    _In_ PUNICODE_STRING SubsystemName,
    _In_ PVOID HandleId,
    _In_ BOOLEAN GenerateOnClose)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/**
 * @unimplemented
 * @brief
 * Raises an alarm audit message when an object is about
 * to be opened.
 *
 * @param[in] SubjectContext
 * A security subject context for auditing.
 *
 * @param[in] SubsystemName
 * A Unicode string that points to a name of the subsystem.
 *
 * @param[in] HandleId
 * A handle to an ID used for identification instance for auditing.
 *
 * @param[in] ObjectTypeName
 * A Unicode string that points to an object type name.
 *
 * @param[in] ObjectName
 * The name of the object.
 *
 * @param[in] SecurityDescriptor
 * A security descriptor.
 *
 * @param[in] ClientToken
 * A client access token, representing the client we want to impersonate.
 *
 * @param[in] DesiredAccess
 * The desired access rights masks requested by the caller.
 *
 * @param[in] GrantedAccess
 * The granted access mask rights.
 *
 * @param[in] Privileges
 * If specified, the function will use this set of privileges to audit.
 *
 * @param[in] ObjectCreation
 * Set this to TRUE if the object has just been created.
 *
 * @param[in] AccessGranted
 * Set this to TRUE if the access attempt was deemed as granted.
 *
 * @param[out] GenerateOnClose
 * A boolean flag returned to the caller once audit generation procedure
 * finishes.
 *
 * @return
 * Nothing.
 */
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

/**
 * @brief
 * Raises an alarm audit message when an object is about
 * to be opened.
 *
 * @param[in] SubsystemName
 * A Unicode string that points to a name of the subsystem.
 *
 * @param[in] HandleId
 * A handle to an ID used for identification instance for auditing.
 *
 * @param[in] ObjectTypeName
 * A Unicode string that points to an object type name.
 *
 * @param[in] ObjectName
 * The name of the object.
 *
 * @param[in] SecurityDescriptor
 * A security descriptor.
 *
 * @param[in] ClientTokenHandle
 * A handle to a client access token.
 *
 * @param[in] DesiredAccess
 * The desired access rights masks requested by the caller.
 *
 * @param[in] GrantedAccess
 * The granted access mask rights.
 *
 * @param[in] PrivilegeSet
 * If specified, the function will use this set of privileges to audit.
 *
 * @param[in] ObjectCreation
 * Set this to TRUE if the object has just been created.
 *
 * @param[in] AccessGranted
 * Set this to TRUE if the access attempt was deemed as granted.
 *
 * @param[out] GenerateOnClose
 * A boolean flag returned to the caller once audit generation procedure
 * finishes.
 *
 * @return
 * Returns STATUS_SUCCESS if all the operations have been completed successfully.
 * STATUS_PRIVILEGE_NOT_HELD is returned if the given subject context does not
 * hold the required audit privilege to actually begin auditing in the first place.
 * STATUS_BAD_IMPERSONATION_LEVEL is returned if the security impersonation level
 * of the client token is not on par with the impersonation level that alllows
 * impersonation. STATUS_INVALID_PARAMETER is returned if the caller has
 * submitted a bogus set of privileges as such array set exceeds the maximum
 * count of privileges that the kernel can accept. A failure NTSTATUS code
 * is returned otherwise.
 */
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
                _SEH2_YIELD(goto Cleanup);
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
                _SEH2_YIELD(goto Cleanup);
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
        _SEH2_YIELD(goto Cleanup);
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

/**
 * @brief
 * Raises an alarm audit message when a caller attempts to request
 * a privileged service call.
 *
 * @param[in] SubsystemName
 * A Unicode string that points to a name of the subsystem.
 *
 * @param[in] ServiceName
 * A Unicode string that points to a name of the privileged service.
 *
 * @param[in] ClientTokenHandle
 * A handle to a client access token.
 *
 * @param[in] Privileges
 * An array set of privileges.
 *
 * @param[in] AccessGranted
 * Set this to TRUE if the access attempt was deemed as granted.
 *
 * @return
 * Returns STATUS_SUCCESS if all the operations have been completed successfully.
 * STATUS_PRIVILEGE_NOT_HELD is returned if the given subject context does not
 * hold the required audit privilege to actually begin auditing in the first place.
 * STATUS_BAD_IMPERSONATION_LEVEL is returned if the security impersonation level
 * of the client token is not on par with the impersonation level that alllows
 * impersonation. STATUS_INVALID_PARAMETER is returned if the caller has
 * submitted a bogus set of privileges as such array set exceeds the maximum
 * count of privileges that the kernel can accept. A failure NTSTATUS code
 * is returned otherwise.
 */
__kernel_entry
NTSTATUS
NTAPI
NtPrivilegedServiceAuditAlarm(
    _In_opt_ PUNICODE_STRING SubsystemName,
    _In_opt_ PUNICODE_STRING ServiceName,
    _In_ HANDLE ClientTokenHandle,
    _In_ PPRIVILEGE_SET Privileges,
    _In_ BOOLEAN AccessGranted)
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
            _SEH2_YIELD(goto Cleanup);
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
            _SEH2_YIELD(goto Cleanup);
        }

        /* Copy the privileges */
        RtlCopyMemory(CapturedPrivileges, Privileges, PrivilegesSize);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
        DPRINT1("Got exception 0x%lx\n", Status);
        _SEH2_YIELD(goto Cleanup);
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

/**
 * @brief
 * Raises an alarm audit message when a caller attempts to access a
 * privileged object.
 *
 * @param[in] SubsystemName
 * A Unicode string that points to a name of the subsystem.
 *
 * @param[in] HandleId
 * A handle to an ID that is used as identification instance for auditing.
 *
 * @param[in] ClientToken
 * A handle to a client access token.
 *
 * @param[in] DesiredAccess
 * A handle to a client access token.
 *
 * @param[in] Privileges
 * An array set of privileges.
 *
 * @param[in] AccessGranted
 * Set this to TRUE if the access attempt was deemed as granted.
 *
 * @return
 * To be added...
 */
NTSTATUS NTAPI
NtPrivilegeObjectAuditAlarm(
    _In_ PUNICODE_STRING SubsystemName,
    _In_ PVOID HandleId,
    _In_ HANDLE ClientToken,
    _In_ ULONG DesiredAccess,
    _In_ PPRIVILEGE_SET Privileges,
    _In_ BOOLEAN AccessGranted)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief
 * Raises an alarm audit message when a caller attempts to access an
 * object and determine if the access can be made.
 *
 * @param[in] SubsystemName
 * A Unicode string that points to a name of the subsystem.
 *
 * @param[in] HandleId
 * A handle to an ID that is used as identification instance for auditing.
 *
 * @param[in] ObjectTypeName
 * The name of the object type.
 *
 * @param[in] ObjectName
 * The object name.
 *
 * @param[in] SecurityDescriptor
 * A security descriptor.
 *
 * @param[in] DesiredAccess
 * The desired access rights masks requested by the caller.
 *
 * @param[in] GenericMapping
 * The generic mapping of access mask rights.
 *
 * @param[in] ObjectCreation
 * Set this to TRUE if the object has just been created.
 *
 * @param[out] GrantedAccess
 * Returns the granted access rights.
 *
 * @param[out] AccessStatus
 * Returns a NTSTATUS status code indicating whether access check
 * can be granted or not.
 *
 * @param[out] GenerateOnClose
 * Returns TRUE if the function has generated a list of granted access rights and
 * status codes on termination, FALSE otherwise.
 *
 * @return
 * See SepAccessCheckAndAuditAlarm.
 */
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

/**
 * @brief
 * Raises an alarm audit message when a caller attempts to access an
 * object and determine if the access can be made by type.
 *
 * @param[in] SubsystemName
 * A Unicode string that points to a name of the subsystem.
 *
 * @param[in] HandleId
 * A handle to an ID that is used as identification instance for auditing.
 *
 * @param[in] ObjectTypeName
 * The name of the object type.
 *
 * @param[in] ObjectName
 * The object name.
 *
 * @param[in] SecurityDescriptor
 * A security descriptor.
 *
 * @param[in] PrincipalSelfSid
 * A principal self user SID.
 *
 * @param[in] DesiredAccess
 * The desired access rights masks requested by the caller.
 *
 * @param[in] AuditType
 * Type of audit to start, influencing how the audit should
 * be done.
 *
 * @param[in] Flags
 * Flag bitmask, used to check if auditing can be done
 * without privileges.
 *
 * @param[in] ObjectTypeList
 * A list of object types.
 *
 * @param[in] ObjectTypeLength
 * The length size of the list.
 *
 * @param[in] GenericMapping
 * The generic mapping of access mask rights.
 *
 * @param[in] ObjectCreation
 * Set this to TRUE if the object has just been created.
 *
 * @param[out] GrantedAccess
 * Returns the granted access rights.
 *
 * @param[out] AccessStatus
 * Returns a NTSTATUS status code indicating whether access check
 * can be granted or not.
 *
 * @param[out] GenerateOnClose
 * Returns TRUE if the function has generated a list of granted access rights and
 * status codes on termination, FALSE otherwise.
 *
 * @return
 * See SepAccessCheckAndAuditAlarm.
 */
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

/**
 * @brief
 * Raises an alarm audit message when a caller attempts to access an
 * object and determine if the access can be made by given type result.
 *
 * @param[in] SubsystemName
 * A Unicode string that points to a name of the subsystem.
 *
 * @param[in] HandleId
 * A handle to an ID that is used as identification instance for auditing.
 *
 * @param[in] ObjectTypeName
 * The name of the object type.
 *
 * @param[in] ObjectName
 * The object name.
 *
 * @param[in] SecurityDescriptor
 * A security descriptor.
 *
 * @param[in] PrincipalSelfSid
 * A principal self user SID.
 *
 * @param[in] DesiredAccess
 * The desired access rights masks requested by the caller.
 *
 * @param[in] AuditType
 * Type of audit to start, influencing how the audit should
 * be done.
 *
 * @param[in] Flags
 * Flag bitmask, used to check if auditing can be done
 * without privileges.
 *
 * @param[in] ObjectTypeList
 * A list of object types.
 *
 * @param[in] ObjectTypeLength
 * The length size of the list.
 *
 * @param[in] GenericMapping
 * The generic mapping of access mask rights.
 *
 * @param[in] ObjectCreation
 * Set this to TRUE if the object has just been created.
 *
 * @param[out] GrantedAccessList
 * Returns the granted access rights.
 *
 * @param[out] AccessStatusList
 * Returns a NTSTATUS status code indicating whether access check
 * can be granted or not.
 *
 * @param[out] GenerateOnClose
 * Returns TRUE if the function has generated a list of granted access rights and
 * status codes on termination, FALSE otherwise.
 *
 * @return
 * See SepAccessCheckAndAuditAlarm.
 */
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

/**
 * @brief
 * Raises an alarm audit message when a caller attempts to access an
 * object and determine if the access can be made by given type result
 * and a token handle.
 *
 * @param[in] SubsystemName
 * A Unicode string that points to a name of the subsystem.
 *
 * @param[in] HandleId
 * A handle to an ID that is used as identification instance for auditing.
 *
 * @param[in] ClientToken
 * A handle to a client access token.
 *
 * @param[in] ObjectTypeName
 * The name of the object type.
 *
 * @param[in] ObjectName
 * The object name.
 *
 * @param[in] SecurityDescriptor
 * A security descriptor.
 *
 * @param[in] PrincipalSelfSid
 * A principal self user SID.
 *
 * @param[in] DesiredAccess
 * The desired access rights masks requested by the caller.
 *
 * @param[in] AuditType
 * Type of audit to start, influencing how the audit should
 * be done.
 *
 * @param[in] Flags
 * Flag bitmask, used to check if auditing can be done
 * without privileges.
 *
 * @param[in] ObjectTypeList
 * A list of object types.
 *
 * @param[in] ObjectTypeLength
 * The length size of the list.
 *
 * @param[in] GenericMapping
 * The generic mapping of access mask rights.
 *
 * @param[in] ObjectCreation
 * Set this to TRUE if the object has just been created.
 *
 * @param[out] GrantedAccessList
 * Returns the granted access rights.
 *
 * @param[out] AccessStatusList
 * Returns a NTSTATUS status code indicating whether access check
 * can be granted or not.
 *
 * @param[out] GenerateOnClose
 * Returns TRUE if the function has generated a list of granted access rights and
 * status codes on termination, FALSE otherwise.
 *
 * @return
 * See SepAccessCheckAndAuditAlarm.
 */
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
