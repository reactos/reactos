/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Security manager infrastructure
 * COPYRIGHT:       Copyright Timo Kreuzer <timo.kreuzer@reactos.org>
 *                  Copyright Eric Kohl
 *                  Copyright Aleksey Bragin
 *                  Copyright Alex Ionescu <alex@relsoft.net>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PTOKEN SeAnonymousLogonToken = NULL;
PTOKEN SeAnonymousLogonTokenNoEveryone = NULL;
PSE_EXPORTS SeExports = NULL;
SE_EXPORTS SepExports;
ULONG SidInTokenCalls = 0;

extern ULONG ExpInitializationPhase;
extern ERESOURCE SepSubjectContextLock;

/* PRIVATE FUNCTIONS **********************************************************/

/**
 * @brief
 * Initializes all the security exports upon initialization phase of
 * the module.
 *
 * @return
 * Returns TRUE.
 */
static
CODE_SEG("INIT")
BOOLEAN
SepInitExports(VOID)
{
    SepExports.SeCreateTokenPrivilege = SeCreateTokenPrivilege;
    SepExports.SeAssignPrimaryTokenPrivilege = SeAssignPrimaryTokenPrivilege;
    SepExports.SeLockMemoryPrivilege = SeLockMemoryPrivilege;
    SepExports.SeIncreaseQuotaPrivilege = SeIncreaseQuotaPrivilege;
    SepExports.SeUnsolicitedInputPrivilege = SeUnsolicitedInputPrivilege;
    SepExports.SeTcbPrivilege = SeTcbPrivilege;
    SepExports.SeSecurityPrivilege = SeSecurityPrivilege;
    SepExports.SeTakeOwnershipPrivilege = SeTakeOwnershipPrivilege;
    SepExports.SeLoadDriverPrivilege = SeLoadDriverPrivilege;
    SepExports.SeCreatePagefilePrivilege = SeCreatePagefilePrivilege;
    SepExports.SeIncreaseBasePriorityPrivilege = SeIncreaseBasePriorityPrivilege;
    SepExports.SeSystemProfilePrivilege = SeSystemProfilePrivilege;
    SepExports.SeSystemtimePrivilege = SeSystemtimePrivilege;
    SepExports.SeProfileSingleProcessPrivilege = SeProfileSingleProcessPrivilege;
    SepExports.SeCreatePermanentPrivilege = SeCreatePermanentPrivilege;
    SepExports.SeBackupPrivilege = SeBackupPrivilege;
    SepExports.SeRestorePrivilege = SeRestorePrivilege;
    SepExports.SeShutdownPrivilege = SeShutdownPrivilege;
    SepExports.SeDebugPrivilege = SeDebugPrivilege;
    SepExports.SeAuditPrivilege = SeAuditPrivilege;
    SepExports.SeSystemEnvironmentPrivilege = SeSystemEnvironmentPrivilege;
    SepExports.SeChangeNotifyPrivilege = SeChangeNotifyPrivilege;
    SepExports.SeRemoteShutdownPrivilege = SeRemoteShutdownPrivilege;

    SepExports.SeNullSid = SeNullSid;
    SepExports.SeWorldSid = SeWorldSid;
    SepExports.SeLocalSid = SeLocalSid;
    SepExports.SeCreatorOwnerSid = SeCreatorOwnerSid;
    SepExports.SeCreatorGroupSid = SeCreatorGroupSid;
    SepExports.SeNtAuthoritySid = SeNtAuthoritySid;
    SepExports.SeDialupSid = SeDialupSid;
    SepExports.SeNetworkSid = SeNetworkSid;
    SepExports.SeBatchSid = SeBatchSid;
    SepExports.SeInteractiveSid = SeInteractiveSid;
    SepExports.SeLocalSystemSid = SeLocalSystemSid;
    SepExports.SeAliasAdminsSid = SeAliasAdminsSid;
    SepExports.SeAliasUsersSid = SeAliasUsersSid;
    SepExports.SeAliasGuestsSid = SeAliasGuestsSid;
    SepExports.SeAliasPowerUsersSid = SeAliasPowerUsersSid;
    SepExports.SeAliasAccountOpsSid = SeAliasAccountOpsSid;
    SepExports.SeAliasSystemOpsSid = SeAliasSystemOpsSid;
    SepExports.SeAliasPrintOpsSid = SeAliasPrintOpsSid;
    SepExports.SeAliasBackupOpsSid = SeAliasBackupOpsSid;
    SepExports.SeAuthenticatedUsersSid = SeAuthenticatedUsersSid;
    SepExports.SeRestrictedSid = SeRestrictedSid;
    SepExports.SeAnonymousLogonSid = SeAnonymousLogonSid;
    SepExports.SeLocalServiceSid = SeLocalServiceSid;
    SepExports.SeNetworkServiceSid = SeNetworkServiceSid;

    SepExports.SeUndockPrivilege = SeUndockPrivilege;
    SepExports.SeSyncAgentPrivilege = SeSyncAgentPrivilege;
    SepExports.SeEnableDelegationPrivilege = SeEnableDelegationPrivilege;
    SepExports.SeManageVolumePrivilege = SeManageVolumePrivilege;
    SepExports.SeImpersonatePrivilege = SeImpersonatePrivilege;
    SepExports.SeCreateGlobalPrivilege = SeCreateGlobalPrivilege;

    SeExports = &SepExports;
    return TRUE;
}

/**
 * @brief
 * Handles the phase 0 procedure of the SRM initialization.
 *
 * @return
 * Returns TRUE if the phase 0 initialization has succeeded and that
 * we can proceed further with next initialization phase, FALSE
 * otherwise.
 */
CODE_SEG("INIT")
BOOLEAN
NTAPI
SepInitializationPhase0(VOID)
{
    PAGED_CODE();

    if (!ExLuidInitialization()) return FALSE;
    if (!SepInitSecurityIDs()) return FALSE;
    if (!SepInitDACLs()) return FALSE;
    if (!SepInitSDs()) return FALSE;
    SepInitPrivileges();
    if (!SepInitExports()) return FALSE;

    /* Initialize the subject context lock */
    ExInitializeResource(&SepSubjectContextLock);

    /* Initialize token objects */
    SepInitializeTokenImplementation();

    /* Initialize logon sessions */
    if (!SeRmInitPhase0()) return FALSE;

    /* Clear impersonation info for the idle thread */
    PsGetCurrentThread()->ImpersonationInfo = NULL;
    PspClearCrossThreadFlag(PsGetCurrentThread(),
                            CT_ACTIVE_IMPERSONATION_INFO_BIT);

    /* Initialize the boot token */
    ObInitializeFastReference(&PsGetCurrentProcess()->Token, NULL);
    ObInitializeFastReference(&PsGetCurrentProcess()->Token,
                              SepCreateSystemProcessToken());

    /* Initialise the anonymous logon tokens */
    SeAnonymousLogonToken = SepCreateSystemAnonymousLogonToken();
    if (!SeAnonymousLogonToken)
        return FALSE;

    SeAnonymousLogonTokenNoEveryone = SepCreateSystemAnonymousLogonTokenNoEveryone();
    if (!SeAnonymousLogonTokenNoEveryone)
        return FALSE;

    return TRUE;
}

/**
 * @brief
 * Handles the phase 1 procedure of the SRM initialization.
 *
 * @return
 * Returns TRUE if the phase 1 initialization has succeeded, FALSE
 * otherwise.
 */
CODE_SEG("INIT")
BOOLEAN
NTAPI
SepInitializationPhase1(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING Name;
    HANDLE SecurityHandle;
    HANDLE EventHandle;
    NTSTATUS Status;
    SECURITY_DESCRIPTOR SecurityDescriptor;
    PACL Dacl;
    ULONG DaclLength;

    PAGED_CODE();

    /* Insert the system token into the tree */
    Status = ObInsertObject((PVOID)(PsGetCurrentProcess()->Token.Value &
                                    ~MAX_FAST_REFS),
                            NULL,
                            0,
                            0,
                            NULL,
                            NULL);
    ASSERT(NT_SUCCESS(Status));

    /* Create a security descriptor for the directory */
    RtlCreateSecurityDescriptor(&SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);

    /* Setup the ACL */
    DaclLength = sizeof(ACL) + 3 * sizeof(ACCESS_ALLOWED_ACE) +
                 RtlLengthSid(SeLocalSystemSid) +
                 RtlLengthSid(SeAliasAdminsSid) +
                 RtlLengthSid(SeWorldSid);
    Dacl = ExAllocatePoolWithTag(NonPagedPool, DaclLength, TAG_SE);
    if (Dacl == NULL)
    {
        return FALSE;
    }

    Status = RtlCreateAcl(Dacl, DaclLength, ACL_REVISION);
    ASSERT(NT_SUCCESS(Status));

    /* Grant full access to SYSTEM */
    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    DIRECTORY_ALL_ACCESS,
                                    SeLocalSystemSid);
    ASSERT(NT_SUCCESS(Status));

    /* Allow admins to traverse and query */
    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    READ_CONTROL | DIRECTORY_TRAVERSE | DIRECTORY_QUERY,
                                    SeAliasAdminsSid);
    ASSERT(NT_SUCCESS(Status));

    /* Allow anyone to traverse */
    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    DIRECTORY_TRAVERSE,
                                    SeWorldSid);
    ASSERT(NT_SUCCESS(Status));

    /* And link ACL and SD */
    Status = RtlSetDaclSecurityDescriptor(&SecurityDescriptor, TRUE, Dacl, FALSE);
    ASSERT(NT_SUCCESS(Status));

    /* Create '\Security' directory */
    RtlInitUnicodeString(&Name, L"\\Security");
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_PERMANENT | OBJ_CASE_INSENSITIVE,
                               0,
                               &SecurityDescriptor);

    Status = ZwCreateDirectoryObject(&SecurityHandle,
                                     DIRECTORY_ALL_ACCESS,
                                     &ObjectAttributes);
    ASSERT(NT_SUCCESS(Status));

    /* Free the DACL */
    ExFreePoolWithTag(Dacl, TAG_SE);

    /* Create 'LSA_AUTHENTICATION_INITIALIZED' event */
    RtlInitUnicodeString(&Name, L"LSA_AUTHENTICATION_INITIALIZED");
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_PERMANENT | OBJ_CASE_INSENSITIVE,
                               SecurityHandle,
                               SePublicDefaultSd);

    Status = ZwCreateEvent(&EventHandle,
                           GENERIC_WRITE,
                           &ObjectAttributes,
                           NotificationEvent,
                           FALSE);
    ASSERT(NT_SUCCESS(Status));

    Status = ZwClose(EventHandle);
    ASSERT(NT_SUCCESS(Status));

    Status = ZwClose(SecurityHandle);
    ASSERT(NT_SUCCESS(Status));

    return TRUE;
}

/**
 * @brief
 * Main security manager initialization function.
 *
 * @return
 * Returns a boolean value according to the phase initialization
 * routine that handles it. If TRUE, the routine deems the initialization
 * phase as complete, FALSE otherwise.
 */
CODE_SEG("INIT")
BOOLEAN
NTAPI
SeInitSystem(VOID)
{
    /* Check the initialization phase */
    switch (ExpInitializationPhase)
    {
        case 0:

            /* Do Phase 0 */
            return SepInitializationPhase0();

        case 1:

            /* Do Phase 1 */
            return SepInitializationPhase1();

        default:

            /* Don't know any other phase! Bugcheck! */
            KeBugCheckEx(UNEXPECTED_INITIALIZATION_CALL,
                         0,
                         ExpInitializationPhase,
                         0,
                         0);
            return FALSE;
    }
}

/**
 * @brief
 * Internal function that is responsible for querying, deleting, assigning and
 * setting a security descriptor for an object in the NT kernel. It is the default
 * security method for objects regarding the security context of objects.
 *
 * @param[in] Object
 * The object that has the default security method, which the function has been
 * called upon.
 *
 * @param[in] OperationType
 * Operation type to perform to that object.
 *
 * @param[in] SecurityInformation
 * Auxiliary security information of the object.
 *
 * @param[in,out] SecurityDescriptor
 * A security descriptor. This SD is used accordingly to the operation type
 * requested by the caller.
 *
 * @param[in,out] ReturnLength
 * The length size of the queried security descriptor, in bytes.
 *
 * @param[in,out] OldSecurityDescriptor
 * The old SD that belonged to the object, in case we're either deleting
 * or replacing it.
 *
 * @param[in] PoolType
 * Pool type allocation for the security descriptor.
 *
 * @param[in] GenericMapping
 * The generic mapping of access rights masks for the object.
 *
 * @return
 * Returns STATUS_SUCCESS if the specific operation tasked has been
 * completed. Otherwise a failure NTSTATUS code is returned.
 */
NTSTATUS
NTAPI
SeDefaultObjectMethod(
    _In_ PVOID Object,
    _In_ SECURITY_OPERATION_CODE OperationType,
    _In_ PSECURITY_INFORMATION SecurityInformation,
    _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Inout_opt_ PULONG ReturnLength,
    _Inout_ PSECURITY_DESCRIPTOR *OldSecurityDescriptor,
    _In_ POOL_TYPE PoolType,
    _In_ PGENERIC_MAPPING GenericMapping)
{
    PAGED_CODE();

    /* Select the operation type */
    switch (OperationType)
    {
            /* Setting a new descriptor */
        case SetSecurityDescriptor:

            /* Sanity check */
            ASSERT((PoolType == PagedPool) || (PoolType == NonPagedPool));

            /* Set the information */
            return ObSetSecurityDescriptorInfo(Object,
                                               SecurityInformation,
                                               SecurityDescriptor,
                                               OldSecurityDescriptor,
                                               PoolType,
                                               GenericMapping);

        case QuerySecurityDescriptor:

            /* Query the information */
            return ObQuerySecurityDescriptorInfo(Object,
                                                 SecurityInformation,
                                                 SecurityDescriptor,
                                                 ReturnLength,
                                                 OldSecurityDescriptor);

        case DeleteSecurityDescriptor:

            /* De-assign it */
            return ObDeassignSecurity(OldSecurityDescriptor);

        case AssignSecurityDescriptor:

            /* Assign it */
            ObAssignObjectSecurityDescriptor(Object, SecurityDescriptor, PoolType);
            return STATUS_SUCCESS;

        default:

            /* Bug check */
            KeBugCheckEx(SECURITY_SYSTEM, 0, STATUS_INVALID_PARAMETER, 0, 0);
    }

    /* Should never reach here */
    ASSERT(FALSE);
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Queries the access mask from a security information context.
 *
 * @param[in] SecurityInformation
 * The security information context where the access mask is to be
 * gathered.
 *
 * @param[out] DesiredAccess
 * The queried access mask right.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
SeQuerySecurityAccessMask(
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_ PACCESS_MASK DesiredAccess)
{
    *DesiredAccess = 0;

    if (SecurityInformation & (OWNER_SECURITY_INFORMATION |
                               GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION))
    {
        *DesiredAccess |= READ_CONTROL;
    }

    if (SecurityInformation & SACL_SECURITY_INFORMATION)
    {
        *DesiredAccess |= ACCESS_SYSTEM_SECURITY;
    }
}

/**
 * @brief
 * Sets the access mask for a security information context.
 *
 * @param[in] SecurityInformation
 * The security information context to apply a new access right.
 *
 * @param[out] DesiredAccess
 * The returned access mask right.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
SeSetSecurityAccessMask(
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_ PACCESS_MASK DesiredAccess)
{
    *DesiredAccess = 0;

    if (SecurityInformation & (OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION))
    {
        *DesiredAccess |= WRITE_OWNER;
    }

    if (SecurityInformation & DACL_SECURITY_INFORMATION)
    {
        *DesiredAccess |= WRITE_DAC;
    }

    if (SecurityInformation & SACL_SECURITY_INFORMATION)
    {
        *DesiredAccess |= ACCESS_SYSTEM_SECURITY;
    }
}

/**
 * @unimplemented
 * @brief
 * Report a security event to the security manager.
 *
 * @param[in] Flags
 * Flags that influence how the event should be reported.
 *
 * @param[in] SourceName
 * A Unicode string that represents the source name of the event.
 *
 * @param[in] UserSid
 * The SID that represents a user that initiated the reporting.
 *
 * @param[in] AuditParameters
 * An array of parameters for auditing purposes. This is used
 * for reporting the event which the security manager will take
 * care subsequently of doing eventual security auditing.
 *
 * @return
 * Returns STATUS_SUCCESS if the security event has been reported.
 * STATUS_INVALID_PARAMETER is returned if one of the parameters
 * do not satisfy the requirements expected by the function.
 */
NTSTATUS
NTAPI
SeReportSecurityEvent(
    _In_ ULONG Flags,
    _In_ PUNICODE_STRING SourceName,
    _In_opt_ PSID UserSid,
    _In_ PSE_ADT_PARAMETER_ARRAY AuditParameters)
{
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    PTOKEN EffectiveToken;
    PISID Sid;
    NTSTATUS Status;

    /* Validate parameters */
    if ((Flags != 0) ||
        (SourceName == NULL) ||
        (SourceName->Buffer == NULL) ||
        (SourceName->Length == 0) ||
        (AuditParameters == NULL) ||
        (AuditParameters->ParameterCount > SE_MAX_AUDIT_PARAMETERS - 4))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Validate the source name */
    Status = RtlValidateUnicodeString(0, SourceName);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Check if we have a user SID */
    if (UserSid != NULL)
    {
        /* Validate it */
        if (!RtlValidSid(UserSid))
        {
            return STATUS_INVALID_PARAMETER;
        }

        /* Use the user SID */
        Sid = UserSid;
    }
    else
    {
        /* No user SID, capture the security subject context */
        SeCaptureSubjectContext(&SubjectContext);

        /* Extract the effective token */
        EffectiveToken = SubjectContext.ClientToken ?
            SubjectContext.ClientToken : SubjectContext.PrimaryToken;

        /* Use the user-and-groups SID */
        Sid = EffectiveToken->UserAndGroups->Sid;
    }

    UNIMPLEMENTED;

    /* Check if we captured the subject context */
    if (Sid != UserSid)
    {
        /* Release it */
        SeReleaseSubjectContext(&SubjectContext);
    }

    /* Return success */
    return STATUS_SUCCESS;
}

/**
 * @unimplemented
 * @brief
 * Sets an array of audit parameters for later security auditing use.
 *
 * @param[in,out] AuditParameters
 * An array of audit parameters to be set.
 *
 * @param[in] Type
 * The type of audit parameters to be set.
 *
 * @param[in] Index
 * Index number that represents an instance of an audit parameters.
 * Such index must be within the maximum range of audit parameters.
 *
 * @param[in] Data
 * An arbitrary buffer data that is bounds to what kind of audit parameter
 * type must be set.
 *
 * @return
 * To be added...
 */
_Const_
NTSTATUS
NTAPI
SeSetAuditParameter(
    _Inout_ PSE_ADT_PARAMETER_ARRAY AuditParameters,
    _In_ SE_ADT_PARAMETER_TYPE Type,
    _In_range_(<, SE_MAX_AUDIT_PARAMETERS) ULONG Index,
    _In_reads_(_Inexpressible_("depends on SE_ADT_PARAMETER_TYPE")) PVOID Data)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

/* EOF */
