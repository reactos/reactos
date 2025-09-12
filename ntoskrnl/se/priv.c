/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Security privileges support
 * COPYRIGHT:   Copyright Alex Ionescu <alex@relsoft.net>
 *              Copyright Timo Kreuzer <timo.kreuzer@reactos.org>
 *              Copyright Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

#define SE_MAXIMUM_PRIVILEGE_LIMIT 0x3C

#define CONST_LUID(x1, x2) {x1, x2}
const LUID SeCreateTokenPrivilege = CONST_LUID(SE_CREATE_TOKEN_PRIVILEGE, 0);
const LUID SeAssignPrimaryTokenPrivilege = CONST_LUID(SE_ASSIGNPRIMARYTOKEN_PRIVILEGE, 0);
const LUID SeLockMemoryPrivilege = CONST_LUID(SE_LOCK_MEMORY_PRIVILEGE, 0);
const LUID SeIncreaseQuotaPrivilege = CONST_LUID(SE_INCREASE_QUOTA_PRIVILEGE, 0);
const LUID SeUnsolicitedInputPrivilege = CONST_LUID(6, 0);
const LUID SeTcbPrivilege = CONST_LUID(SE_TCB_PRIVILEGE, 0);
const LUID SeSecurityPrivilege = CONST_LUID(SE_SECURITY_PRIVILEGE, 0);
const LUID SeTakeOwnershipPrivilege = CONST_LUID(SE_TAKE_OWNERSHIP_PRIVILEGE, 0);
const LUID SeLoadDriverPrivilege = CONST_LUID(SE_LOAD_DRIVER_PRIVILEGE, 0);
const LUID SeSystemProfilePrivilege = CONST_LUID(SE_SYSTEM_PROFILE_PRIVILEGE, 0);
const LUID SeSystemtimePrivilege = CONST_LUID(SE_SYSTEMTIME_PRIVILEGE, 0);
const LUID SeProfileSingleProcessPrivilege = CONST_LUID(SE_PROF_SINGLE_PROCESS_PRIVILEGE, 0);
const LUID SeIncreaseBasePriorityPrivilege = CONST_LUID(SE_INC_BASE_PRIORITY_PRIVILEGE, 0);
const LUID SeCreatePagefilePrivilege = CONST_LUID(SE_CREATE_PAGEFILE_PRIVILEGE, 0);
const LUID SeCreatePermanentPrivilege = CONST_LUID(SE_CREATE_PERMANENT_PRIVILEGE, 0);
const LUID SeBackupPrivilege = CONST_LUID(SE_BACKUP_PRIVILEGE, 0);
const LUID SeRestorePrivilege = CONST_LUID(SE_RESTORE_PRIVILEGE, 0);
const LUID SeShutdownPrivilege = CONST_LUID(SE_SHUTDOWN_PRIVILEGE, 0);
const LUID SeDebugPrivilege = CONST_LUID(SE_DEBUG_PRIVILEGE, 0);
const LUID SeAuditPrivilege = CONST_LUID(SE_AUDIT_PRIVILEGE, 0);
const LUID SeSystemEnvironmentPrivilege = CONST_LUID(SE_SYSTEM_ENVIRONMENT_PRIVILEGE, 0);
const LUID SeChangeNotifyPrivilege = CONST_LUID(SE_CHANGE_NOTIFY_PRIVILEGE, 0);
const LUID SeRemoteShutdownPrivilege = CONST_LUID(SE_REMOTE_SHUTDOWN_PRIVILEGE, 0);
const LUID SeUndockPrivilege = CONST_LUID(SE_UNDOCK_PRIVILEGE, 0);
const LUID SeSyncAgentPrivilege = CONST_LUID(SE_SYNC_AGENT_PRIVILEGE, 0);
const LUID SeEnableDelegationPrivilege = CONST_LUID(SE_ENABLE_DELEGATION_PRIVILEGE, 0);
const LUID SeManageVolumePrivilege = CONST_LUID(SE_MANAGE_VOLUME_PRIVILEGE, 0);
const LUID SeImpersonatePrivilege = CONST_LUID(SE_IMPERSONATE_PRIVILEGE, 0);
const LUID SeCreateGlobalPrivilege = CONST_LUID(SE_CREATE_GLOBAL_PRIVILEGE, 0);
const LUID SeTrustedCredmanPrivilege = CONST_LUID(SE_TRUSTED_CREDMAN_ACCESS_PRIVILEGE, 0);
const LUID SeRelabelPrivilege = CONST_LUID(SE_RELABEL_PRIVILEGE, 0);
const LUID SeIncreaseWorkingSetPrivilege = CONST_LUID(SE_INC_WORKING_SET_PRIVILEGE, 0);
const LUID SeTimeZonePrivilege = CONST_LUID(SE_TIME_ZONE_PRIVILEGE, 0);
const LUID SeCreateSymbolicLinkPrivilege = CONST_LUID(SE_CREATE_SYMBOLIC_LINK_PRIVILEGE, 0);


/* PRIVATE FUNCTIONS **********************************************************/

/**
 * @brief
 * Initializes the privileges during the startup phase of the security
 * manager module. This function serves as a placeholder as it currently
 * does nothing.
 *
 * @return
 * Nothing.
 */
CODE_SEG("INIT")
VOID
NTAPI
SepInitPrivileges(VOID)
{

}

/**
 * @brief
 * Checks the privileges pointed by Privileges array argument if they exist and
 * match with the privileges from an access token.
 *
 * @param[in] Token
 * An access token where privileges are to be checked.
 *
 * @param[in] Privileges
 * An array of privileges with attributes used as checking indicator for
 * the function.
 *
 * @param[in] PrivilegeCount
 * The total number count of privileges in the array.
 *
 * @param[in] PrivilegeControl
 * Privilege control bit mask to determine if we should check all the
 * privileges based on the number count of privileges or not.
 *
 * @param[in] PreviousMode
 * Processor level access mode.
 *
 * @return
 * Returns TRUE if the required privileges exist and that they do match.
 * Otherwise the functions returns FALSE.
 */
BOOLEAN
NTAPI
SepPrivilegeCheck(
    _In_ PTOKEN Token,
    _In_ PLUID_AND_ATTRIBUTES Privileges,
    _In_ ULONG PrivilegeCount,
    _In_ ULONG PrivilegeControl,
    _In_ KPROCESSOR_MODE PreviousMode)
{
    ULONG i;
    ULONG j;
    ULONG Required;

    DPRINT("SepPrivilegeCheck() called\n");

    PAGED_CODE();

    if (PreviousMode == KernelMode)
        return TRUE;

    /* Get the number of privileges that are required to match */
    Required = (PrivilegeControl & PRIVILEGE_SET_ALL_NECESSARY) ? PrivilegeCount : 1;

    /* Acquire a shared token lock */
    SepAcquireTokenLockShared(Token);

    /* Loop all requested privileges until we found the required ones */
    for (i = 0; i < PrivilegeCount; i++)
    {
        /* Loop the privileges of the token */
        for (j = 0; j < Token->PrivilegeCount; j++)
        {
            /* Check if the LUIDs match */
            if (RtlEqualLuid(&Token->Privileges[j].Luid, &Privileges[i].Luid))
            {
                DPRINT("Found privilege. Attributes: %lx\n",
                       Token->Privileges[j].Attributes);

                /* Check if the privilege is enabled */
                if (Token->Privileges[j].Attributes & SE_PRIVILEGE_ENABLED)
                {
                    Privileges[i].Attributes |= SE_PRIVILEGE_USED_FOR_ACCESS;
                    Required--;

                    /* Check if we have found all privileges */
                    if (Required == 0)
                    {
                        /* We're done! */
                        SepReleaseTokenLock(Token);
                        return TRUE;
                    }
                }

                /* Leave the inner loop */
                break;
            }
        }
    }

    /* Release the token lock */
    SepReleaseTokenLock(Token);

    /* When we reached this point, we did not find all privileges */
    ASSERT(Required > 0);
    return FALSE;
}

/**
 * @brief
 * Checks only single privilege based upon the privilege pointed by a LUID and
 * if it matches with the one from an access token.
 *
 * @param[in] PrivilegeValue
 * The privilege to be checked.
 *
 * @param[in] Token
 * An access token where its privilege is to be checked against the one
 * provided by the caller.
 *
 * @param[in] PreviousMode
 * Processor level access mode.
 *
 * @return
 * Returns TRUE if the required privilege exists and that it matches
 * with the one from the access token, FALSE otherwise.
 */
BOOLEAN
NTAPI
SepSinglePrivilegeCheck(
    _In_ LUID PrivilegeValue,
    _In_ PTOKEN Token,
    _In_ KPROCESSOR_MODE PreviousMode)
{
    LUID_AND_ATTRIBUTES Privilege;
    PAGED_CODE();
    ASSERT(!RtlEqualLuid(&PrivilegeValue, &SeTcbPrivilege));

    Privilege.Luid = PrivilegeValue;
    Privilege.Attributes = SE_PRIVILEGE_ENABLED;
    return SepPrivilegeCheck(Token,
                             &Privilege,
                             1,
                             PRIVILEGE_SET_ALL_NECESSARY,
                             PreviousMode);
}

/**
 * @brief
 * Checks the security policy and returns a set of privileges
 * based upon the said security policy context.
 *
 * @param[in,out] DesiredAccess
 * The desired access right mask.
 *
 * @param[in,out] GrantedAccess
 * The granted access rights masks. The rights are granted depending
 * on the desired access rights requested by the calling thread.
 *
 * @param[in] SubjectContext
 * Security subject context. If the caller supplies one, the access token
 * supplied by the caller will be assigned to one of client or primary tokens
 * of the subject context in question.
 *
 * @param[in] Token
 * An access token.
 *
 * @param[out] OutPrivilegeSet
 * An array set of privileges to be reported to the caller, if the actual
 * calling thread wants such set of privileges in the first place.
 *
 * @param[in] PreviousMode
 * Processor level access mode.
 *
 * @return
 * Returns STATUS_PRIVILEGE_NOT_HELD if the respective operations have succeeded
 * without problems. STATUS_PRIVILEGE_NOT_HELD is returned if the access token
 * doesn't have SeSecurityPrivilege privilege to warrant ACCESS_SYSTEM_SECURITY
 * access right. STATUS_INSUFFICIENT_RESOURCES is returned if we failed
 * to allocate block of memory pool for the array set of privileges.
 */
NTSTATUS
NTAPI
SePrivilegePolicyCheck(
    _Inout_ PACCESS_MASK DesiredAccess,
    _Inout_ PACCESS_MASK GrantedAccess,
    _In_ PSECURITY_SUBJECT_CONTEXT SubjectContext,
    _In_ PTOKEN Token,
    _Out_opt_ PPRIVILEGE_SET *OutPrivilegeSet,
    _In_ KPROCESSOR_MODE PreviousMode)
{
    SIZE_T PrivilegeSize;
    PPRIVILEGE_SET PrivilegeSet;
    ULONG PrivilegeCount = 0, Index = 0;
    ACCESS_MASK AccessMask = 0;
    PAGED_CODE();

    /* Check if we have a security subject context */
    if (SubjectContext != NULL)
    {
        /* Check if there is a client impersonation token */
        if (SubjectContext->ClientToken != NULL)
            Token = SubjectContext->ClientToken;
        else
            Token = SubjectContext->PrimaryToken;
    }

    /* Check if the caller wants ACCESS_SYSTEM_SECURITY access */
    if (*DesiredAccess & ACCESS_SYSTEM_SECURITY)
    {
        /* Do the privilege check */
        if (SepSinglePrivilegeCheck(SeSecurityPrivilege, Token, PreviousMode))
        {
            /* Remember this access flag */
            AccessMask |= ACCESS_SYSTEM_SECURITY;
            PrivilegeCount++;
        }
        else
        {
            return STATUS_PRIVILEGE_NOT_HELD;
        }
    }

    /* Check if the caller wants WRITE_OWNER access */
    if (*DesiredAccess & WRITE_OWNER)
    {
        /* Do the privilege check */
        if (SepSinglePrivilegeCheck(SeTakeOwnershipPrivilege, Token, PreviousMode))
        {
            /* Remember this access flag */
            AccessMask |= WRITE_OWNER;
            PrivilegeCount++;
        }
    }

    /* Update the access masks */
    *GrantedAccess |= AccessMask;
    *DesiredAccess &= ~AccessMask;

    /* Does the caller want a privilege set? */
    if (OutPrivilegeSet != NULL)
    {
        /* Do we have any privileges to report? */
        if (PrivilegeCount > 0)
        {
            /* Calculate size and allocate the structure */
            PrivilegeSize = FIELD_OFFSET(PRIVILEGE_SET, Privilege[PrivilegeCount]);
            PrivilegeSet = ExAllocatePoolWithTag(PagedPool, PrivilegeSize, TAG_PRIVILEGE_SET);
            *OutPrivilegeSet = PrivilegeSet;
            if (PrivilegeSet == NULL)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            PrivilegeSet->PrivilegeCount = PrivilegeCount;
            PrivilegeSet->Control = 0;

            if (AccessMask & WRITE_OWNER)
            {
                PrivilegeSet->Privilege[Index].Luid = SeTakeOwnershipPrivilege;
                PrivilegeSet->Privilege[Index].Attributes = SE_PRIVILEGE_USED_FOR_ACCESS;
                Index++;
            }

            if (AccessMask & ACCESS_SYSTEM_SECURITY)
            {
                PrivilegeSet->Privilege[Index].Luid = SeSecurityPrivilege;
                PrivilegeSet->Privilege[Index].Attributes = SE_PRIVILEGE_USED_FOR_ACCESS;
            }
        }
        else
        {
            /* No privileges, no structure */
            *OutPrivilegeSet = NULL;
        }
    }

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Checks a single privilege and performs an audit
 * against a privileged service based on a security subject
 * context.
 *
 * @param[in] DesiredAccess
 * Security subject context used for privileged service
 * auditing.
 *
 * @param[in] PreviousMode
 * Processor level access mode.
 *
 * @return
 * Returns TRUE if service auditing and privilege checking
 * tests have succeeded, FALSE otherwise.
 */
BOOLEAN
NTAPI
SeCheckAuditPrivilege(
    _In_ PSECURITY_SUBJECT_CONTEXT SubjectContext,
    _In_ KPROCESSOR_MODE PreviousMode)
{
    PRIVILEGE_SET PrivilegeSet;
    BOOLEAN Result;
    PAGED_CODE();

    /* Initialize the privilege set with the single privilege */
    PrivilegeSet.PrivilegeCount = 1;
    PrivilegeSet.Control = PRIVILEGE_SET_ALL_NECESSARY;
    PrivilegeSet.Privilege[0].Luid = SeAuditPrivilege;
    PrivilegeSet.Privilege[0].Attributes = 0;

    /* Check against the primary token! */
    Result = SepPrivilegeCheck(SubjectContext->PrimaryToken,
                               &PrivilegeSet.Privilege[0],
                               1,
                               PRIVILEGE_SET_ALL_NECESSARY,
                               PreviousMode);

    if (PreviousMode != KernelMode)
    {
        SePrivilegedServiceAuditAlarm(NULL,
                                      SubjectContext,
                                      &PrivilegeSet,
                                      Result);
    }

    return Result;
}

/**
 * @brief
 * Captures a LUID with attributes structure. This function is mainly
 * tied in the context of privileges.
 *
 * @param[in] Src
 * Source of a valid LUID with attributes structure.
 *
 * @param[in] PrivilegeCount
 * Count number of privileges to be captured.
 *
 * @param[in] PreviousMode
 * Processor level access mode.
 *
 * @param[in] AllocatedMem
 * If specified, the function will use this allocated block memory
 * buffer for the captured LUID and attributes structure. Otherwise
 * the function will automatically allocate some buffer for it.
 *
 * @param[in] AllocatedLength
 * The length of the buffer, pointed by AllocatedMem.
 *
 * @param[in] PoolType
 * Pool type of the memory allocation.
 *
 * @param[in] CaptureIfKernel
 * If set to TRUE, the capturing is done in the kernel itself.
 * FALSE if the capturing is done in a kernel mode driver instead.
 *
 * @param[out] Dest
 * The captured LUID with attributes buffer.
 *
 * @param[in,out] Length
 * The length of the captured privileges count.
 *
 * @return
 * Returns STATUS_SUCCESS if the LUID and attributes array
 * has been captured successfully. STATUS_INSUFFICIENT_RESOURCES is returned
 * if memory pool allocation for the captured buffer has failed.
 * STATUS_BUFFER_TOO_SMALL is returned if the buffer size is less than the
 * required size. STATUS_INVALID_PARAMETER is returned if the caller has
 * submitted a privilege count that exceeds that maximum threshold the
 * kernel can permit, for the purpose to avoid an integer overflow.
 */
NTSTATUS
NTAPI
SeCaptureLuidAndAttributesArray(
    _In_ PLUID_AND_ATTRIBUTES Src,
    _In_ ULONG PrivilegeCount,
    _In_ KPROCESSOR_MODE PreviousMode,
    _In_opt_ PLUID_AND_ATTRIBUTES AllocatedMem,
    _In_opt_ ULONG AllocatedLength,
    _In_ POOL_TYPE PoolType,
    _In_ BOOLEAN CaptureIfKernel,
    _Out_ PLUID_AND_ATTRIBUTES *Dest,
    _Inout_ PULONG Length)
{
    ULONG BufferSize;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    if (PrivilegeCount == 0)
    {
        *Dest = 0;
        *Length = 0;
        return STATUS_SUCCESS;
    }

    if (PrivilegeCount > SE_MAXIMUM_PRIVILEGE_LIMIT)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (PreviousMode == KernelMode && !CaptureIfKernel)
    {
        *Dest = Src;
        return STATUS_SUCCESS;
    }

    BufferSize = PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES);
    *Length = ROUND_UP(BufferSize, 4); /* round up to a 4 byte alignment */

    /* probe the buffer */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForRead(Src,
                         BufferSize,
                         sizeof(ULONG));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* allocate enough memory or check if the provided buffer is
     large enough to hold the array */
    if (AllocatedMem != NULL)
    {
        if (AllocatedLength < BufferSize)
        {
            return STATUS_BUFFER_TOO_SMALL;
        }

        *Dest = AllocatedMem;
    }
    else
    {
        *Dest = ExAllocatePoolWithTag(PoolType,
                                      BufferSize,
                                      TAG_LUID);
        if (*Dest == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    /* copy the array to the buffer */
    _SEH2_TRY
    {
        RtlCopyMemory(*Dest,
                      Src,
                      BufferSize);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status) && AllocatedMem == NULL)
    {
        ExFreePoolWithTag(*Dest, TAG_LUID);
    }

    return Status;
}

/**
 * @brief
 * Releases a LUID with attributes structure.
 *
 * @param[in] Privilege
 * Array of a LUID and attributes that represents a privilege.
 *
 * @param[in] PreviousMode
 * Processor level access mode.
 *
 * @param[in] CaptureIfKernel
 * If set to TRUE, the releasing is done in the kernel itself.
 * FALSE if the releasing is done in a kernel mode driver instead.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
SeReleaseLuidAndAttributesArray(
    _In_ PLUID_AND_ATTRIBUTES Privilege,
    _In_ KPROCESSOR_MODE PreviousMode,
    _In_ BOOLEAN CaptureIfKernel)
{
    PAGED_CODE();

    if (Privilege != NULL &&
        (PreviousMode != KernelMode || CaptureIfKernel))
    {
        ExFreePoolWithTag(Privilege, TAG_LUID);
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief
 * Appends additional privileges.
 *
 * @param[in] AccessState
 * Access request to append.
 *
 * @param[in] Privileges
 * Set of new privileges to append.
 *
 * @return
 * Returns STATUS_SUCCESS if the privileges have been successfully
 * appended. Otherwise STATUS_INSUFFICIENT_RESOURCES is returned,
 * indicating that pool allocation has failed for the buffer to hold
 * the new set of privileges.
 */
NTSTATUS
NTAPI
SeAppendPrivileges(
    _Inout_ PACCESS_STATE AccessState,
    _In_ PPRIVILEGE_SET Privileges)
{
    PAUX_ACCESS_DATA AuxData;
    ULONG OldPrivilegeSetSize;
    ULONG NewPrivilegeSetSize;
    PPRIVILEGE_SET PrivilegeSet;

    PAGED_CODE();

    /* Get the Auxiliary Data */
    AuxData = AccessState->AuxData;

    /* Calculate the size of the old privilege set */
    OldPrivilegeSetSize = sizeof(PRIVILEGE_SET) +
                          (AuxData->PrivilegesUsed->PrivilegeCount - 1) * sizeof(LUID_AND_ATTRIBUTES);

    if (AuxData->PrivilegesUsed->PrivilegeCount +
        Privileges->PrivilegeCount > INITIAL_PRIVILEGE_COUNT)
    {
        /* Calculate the size of the new privilege set */
        NewPrivilegeSetSize = OldPrivilegeSetSize +
                              Privileges->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES);

        /* Allocate a new privilege set */
        PrivilegeSet = ExAllocatePoolWithTag(PagedPool,
                                             NewPrivilegeSetSize,
                                             TAG_PRIVILEGE_SET);
        if (PrivilegeSet == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;

        /* Copy original privileges from the acess state */
        RtlCopyMemory(PrivilegeSet,
                      AuxData->PrivilegesUsed,
                      OldPrivilegeSetSize);

        /* Append privileges from the privilege set*/
        RtlCopyMemory((PVOID)((ULONG_PTR)PrivilegeSet + OldPrivilegeSetSize),
                      (PVOID)((ULONG_PTR)Privileges + sizeof(PRIVILEGE_SET) - sizeof(LUID_AND_ATTRIBUTES)),
                      Privileges->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES));

        /* Adjust the number of privileges in the new privilege set */
        PrivilegeSet->PrivilegeCount += Privileges->PrivilegeCount;

        /* Free the old privilege set if it was allocated */
        if (AccessState->PrivilegesAllocated != FALSE)
            ExFreePoolWithTag(AuxData->PrivilegesUsed, TAG_PRIVILEGE_SET);

        /* Now we are using an allocated privilege set */
        AccessState->PrivilegesAllocated = TRUE;

        /* Assign the new privileges to the access state */
        AuxData->PrivilegesUsed = PrivilegeSet;
    }
    else
    {
        /* Append privileges */
        RtlCopyMemory((PVOID)((ULONG_PTR)AuxData->PrivilegesUsed + OldPrivilegeSetSize),
                      (PVOID)((ULONG_PTR)Privileges + sizeof(PRIVILEGE_SET) - sizeof(LUID_AND_ATTRIBUTES)),
                      Privileges->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES));

        /* Adjust the number of privileges in the target privilege set */
        AuxData->PrivilegesUsed->PrivilegeCount += Privileges->PrivilegeCount;
    }

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Frees a set of privileges.
 *
 * @param[in] Privileges
 * Set of privileges array to be freed.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
SeFreePrivileges(
    _In_ PPRIVILEGE_SET Privileges)
{
    PAGED_CODE();
    ExFreePoolWithTag(Privileges, TAG_PRIVILEGE_SET);
}

/**
 * @brief
 * Checks if a set of privileges exist and match within a
 * security subject context.
 *
 * @param[in] Privileges
 * A set of privileges where the check must be performed
 * against the subject context.
 *
 * @param[in] SubjectContext
 * A subject security context.
 *
 * @param[in] PreviousMode
 * Processor level access mode.
 *
 * @return
 * Returns TRUE if all the privileges do exist and match
 * with the ones specified by the caller and subject
 * context, FALSE otherwise.
 */
BOOLEAN
NTAPI
SePrivilegeCheck(
    _In_ PPRIVILEGE_SET Privileges,
    _In_ PSECURITY_SUBJECT_CONTEXT SubjectContext,
    _In_ KPROCESSOR_MODE PreviousMode)
{
    PACCESS_TOKEN Token = NULL;

    PAGED_CODE();

    if (SubjectContext->ClientToken == NULL)
    {
        Token = SubjectContext->PrimaryToken;
    }
    else
    {
        Token = SubjectContext->ClientToken;
        if (SubjectContext->ImpersonationLevel < 2)
        {
            return FALSE;
        }
    }

    return SepPrivilegeCheck(Token,
                             Privileges->Privilege,
                             Privileges->PrivilegeCount,
                             Privileges->Control,
                             PreviousMode);
}

/**
 * @brief
 * Checks if a single privilege is present in the context
 * of the calling thread.
 *
 * @param[in] PrivilegeValue
 * The specific privilege to be checked.
 *
 * @param[in] PreviousMode
 * Processor level access mode.
 *
 * @return
 * Returns TRUE if the privilege is present, FALSE
 * otherwise.
 */
BOOLEAN
NTAPI
SeSinglePrivilegeCheck(
    _In_ LUID PrivilegeValue,
    _In_ KPROCESSOR_MODE PreviousMode)
{
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    PRIVILEGE_SET Priv;
    BOOLEAN Result;

    PAGED_CODE();

    SeCaptureSubjectContext(&SubjectContext);

    Priv.PrivilegeCount = 1;
    Priv.Control = PRIVILEGE_SET_ALL_NECESSARY;
    Priv.Privilege[0].Luid = PrivilegeValue;
    Priv.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;

    Result = SePrivilegeCheck(&Priv,
                              &SubjectContext,
                              PreviousMode);

    if (PreviousMode != KernelMode)
    {
        SePrivilegedServiceAuditAlarm(NULL,
                                      &SubjectContext,
                                      &Priv,
                                      Result);

    }

    SeReleaseSubjectContext(&SubjectContext);

    return Result;
}

/**
 * @brief
 * Checks a privileged object if such object has
 * the specific privilege submitted by the caller.
 *
 * @param[in] PrivilegeValue
 * A privilege to be checked against the one from
 * the object.
 *
 * @param[in] ObjectHandle
 * A handle to any kind of object.
 *
 * @param[in] DesiredAccess
 * Desired access right mask requested by the caller.
 *
 * @param[in] PreviousMode
 * Processor level access mode.
 *
 * @return
 * Returns TRUE if the privilege is present, FALSE
 * otherwise.
 */
BOOLEAN
NTAPI
SeCheckPrivilegedObject(
    _In_ LUID PrivilegeValue,
    _In_ HANDLE ObjectHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ KPROCESSOR_MODE PreviousMode)
{
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    PRIVILEGE_SET Priv;
    BOOLEAN Result;

    PAGED_CODE();

    SeCaptureSubjectContext(&SubjectContext);

    Priv.PrivilegeCount = 1;
    Priv.Control = PRIVILEGE_SET_ALL_NECESSARY;
    Priv.Privilege[0].Luid = PrivilegeValue;
    Priv.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;

    Result = SePrivilegeCheck(&Priv, &SubjectContext, PreviousMode);
    if (PreviousMode != KernelMode)
    {
#if 0
        SePrivilegeObjectAuditAlarm(ObjectHandle,
                                    &SubjectContext,
                                    DesiredAccess,
                                    &PrivilegeValue,
                                    Result,
                                    PreviousMode);
#endif
    }

    SeReleaseSubjectContext(&SubjectContext);

    return Result;
}

/* SYSTEM CALLS ***************************************************************/

/**
 * @brief
 * Checks a client access token if it has the required set of
 * privileges.
 *
 * @param[in] ClientToken
 * A handle to an access client token.
 *
 * @param[in] RequiredPrivileges
 * A set of required privileges to be checked against the privileges
 * of the access token.
 *
 * @param[out] Result
 * The result, as a boolean value. If TRUE, the token has all the required
 * privileges, FALSE otherwise.
 *
 * @return
 * Returns STATUS_SUCCESS if the function has completed successfully.
 * STATUS_INVALID_PARAMETER is returned if the set array of required
 * privileges has a bogus number of privileges, that is, the array
 * has a count of privileges that exceeds the maximum threshold
 * (or in other words, an integer overflow). A failure NTSTATUS code
 * is returned otherwise.
 */
NTSTATUS
NTAPI
NtPrivilegeCheck(
    _In_ HANDLE ClientToken,
    _In_ PPRIVILEGE_SET RequiredPrivileges,
    _Out_ PBOOLEAN Result)
{
    PLUID_AND_ATTRIBUTES Privileges;
    PTOKEN Token;
    ULONG PrivilegeCount = 0;
    ULONG PrivilegeControl = 0;
    ULONG Length;
    BOOLEAN CheckResult;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    PAGED_CODE();

    PreviousMode = KeGetPreviousMode();

    /* probe the buffers */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForWrite(RequiredPrivileges,
                          FIELD_OFFSET(PRIVILEGE_SET,
                                       Privilege),
                          sizeof(ULONG));

            PrivilegeCount = RequiredPrivileges->PrivilegeCount;
            PrivilegeControl = RequiredPrivileges->Control;

            /* Check PrivilegeCount to avoid an integer overflow! */
            if (FIELD_OFFSET(PRIVILEGE_SET,
                             Privilege[PrivilegeCount]) /
                sizeof(RequiredPrivileges->Privilege[0]) != PrivilegeCount)
            {
                _SEH2_YIELD(return STATUS_INVALID_PARAMETER);
            }

            /* probe all of the array */
            ProbeForWrite(RequiredPrivileges,
                          FIELD_OFFSET(PRIVILEGE_SET,
                                       Privilege[PrivilegeCount]),
                          sizeof(ULONG));

            ProbeForWriteBoolean(Result);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        PrivilegeCount = RequiredPrivileges->PrivilegeCount;
        PrivilegeControl = RequiredPrivileges->Control;
    }

    /* reference the token and make sure we're
     not doing an anonymous impersonation */
    Status = ObReferenceObjectByHandle(ClientToken,
                                       TOKEN_QUERY,
                                       SeTokenObjectType,
                                       PreviousMode,
                                       (PVOID*)&Token,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (Token->TokenType == TokenImpersonation &&
        Token->ImpersonationLevel < SecurityIdentification)
    {
        ObDereferenceObject(Token);
        return STATUS_BAD_IMPERSONATION_LEVEL;
    }

    /* capture the privileges */
    Status = SeCaptureLuidAndAttributesArray(RequiredPrivileges->Privilege,
                                             PrivilegeCount,
                                             PreviousMode,
                                             NULL,
                                             0,
                                             PagedPool,
                                             TRUE,
                                             &Privileges,
                                             &Length);
    if (!NT_SUCCESS(Status))
    {
        ObDereferenceObject (Token);
        return Status;
    }

    CheckResult = SepPrivilegeCheck(Token,
                                    Privileges,
                                    PrivilegeCount,
                                    PrivilegeControl,
                                    PreviousMode);

    ObDereferenceObject(Token);

    /* return the array */
    _SEH2_TRY
    {
        RtlCopyMemory(RequiredPrivileges->Privilege,
                      Privileges,
                      PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES));
        *Result = CheckResult;
        Status = STATUS_SUCCESS;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    SeReleaseLuidAndAttributesArray(Privileges,
                                    PreviousMode,
                                    TRUE);

    return Status;
}

/* EOF */
