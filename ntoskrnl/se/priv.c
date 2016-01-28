/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/se/priv.c
 * PURPOSE:         Security manager
 *
 * PROGRAMMERS:     No programmer listed.
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, SepInitPrivileges)
#endif

/* GLOBALS ********************************************************************/

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

VOID
INIT_FUNCTION
NTAPI
SepInitPrivileges(VOID)
{

}


BOOLEAN
NTAPI
SepPrivilegeCheck(PTOKEN Token,
                  PLUID_AND_ATTRIBUTES Privileges,
                  ULONG PrivilegeCount,
                  ULONG PrivilegeControl,
                  KPROCESSOR_MODE PreviousMode)
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

NTSTATUS
NTAPI
SepSinglePrivilegeCheck(
    LUID PrivilegeValue,
    PTOKEN Token,
    KPROCESSOR_MODE PreviousMode)
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

NTSTATUS
NTAPI
SeCaptureLuidAndAttributesArray(PLUID_AND_ATTRIBUTES Src,
                                ULONG PrivilegeCount,
                                KPROCESSOR_MODE PreviousMode,
                                PLUID_AND_ATTRIBUTES AllocatedMem,
                                ULONG AllocatedLength,
                                POOL_TYPE PoolType,
                                BOOLEAN CaptureIfKernel,
                                PLUID_AND_ATTRIBUTES *Dest,
                                PULONG Length)
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

    if (PreviousMode == KernelMode && !CaptureIfKernel)
    {
        *Dest = Src;
        return STATUS_SUCCESS;
    }

    /* FIXME - check PrivilegeCount for a valid number so we don't
     cause an integer overflow or exhaust system resources! */

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

VOID
NTAPI
SeReleaseLuidAndAttributesArray(PLUID_AND_ATTRIBUTES Privilege,
                                KPROCESSOR_MODE PreviousMode,
                                BOOLEAN CaptureIfKernel)
{
    PAGED_CODE();

    if (Privilege != NULL &&
        (PreviousMode != KernelMode || CaptureIfKernel))
    {
        ExFreePoolWithTag(Privilege, TAG_LUID);
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
SeAppendPrivileges(IN OUT PACCESS_STATE AccessState,
                   IN PPRIVILEGE_SET Privileges)
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
                          (AuxData->PrivilegeSet->PrivilegeCount - 1) * sizeof(LUID_AND_ATTRIBUTES);

    if (AuxData->PrivilegeSet->PrivilegeCount +
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
                      AuxData->PrivilegeSet,
                      OldPrivilegeSetSize);

        /* Append privileges from the privilege set*/
        RtlCopyMemory((PVOID)((ULONG_PTR)PrivilegeSet + OldPrivilegeSetSize),
                      (PVOID)((ULONG_PTR)Privileges + sizeof(PRIVILEGE_SET) - sizeof(LUID_AND_ATTRIBUTES)),
                      Privileges->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES));

        /* Adjust the number of privileges in the new privilege set */
        PrivilegeSet->PrivilegeCount += Privileges->PrivilegeCount;

        /* Free the old privilege set if it was allocated */
        if (AccessState->PrivilegesAllocated != FALSE)
            ExFreePoolWithTag(AuxData->PrivilegeSet, TAG_PRIVILEGE_SET);

        /* Now we are using an allocated privilege set */
        AccessState->PrivilegesAllocated = TRUE;

        /* Assign the new privileges to the access state */
        AuxData->PrivilegeSet = PrivilegeSet;
    }
    else
    {
        /* Append privileges */
        RtlCopyMemory((PVOID)((ULONG_PTR)AuxData->PrivilegeSet + OldPrivilegeSetSize),
                      (PVOID)((ULONG_PTR)Privileges + sizeof(PRIVILEGE_SET) - sizeof(LUID_AND_ATTRIBUTES)),
                      Privileges->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES));

        /* Adjust the number of privileges in the target privilege set */
        AuxData->PrivilegeSet->PrivilegeCount += Privileges->PrivilegeCount;
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
NTAPI
SeFreePrivileges(IN PPRIVILEGE_SET Privileges)
{
    PAGED_CODE();
    ExFreePoolWithTag(Privileges, TAG_PRIVILEGE_SET);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
SePrivilegeCheck(PPRIVILEGE_SET Privileges,
                 PSECURITY_SUBJECT_CONTEXT SubjectContext,
                 KPROCESSOR_MODE PreviousMode)
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

/*
 * @implemented
 */
BOOLEAN
NTAPI
SeSinglePrivilegeCheck(IN LUID PrivilegeValue,
                       IN KPROCESSOR_MODE PreviousMode)
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

BOOLEAN
NTAPI
SeCheckPrivilegedObject(IN LUID PrivilegeValue,
                        IN HANDLE ObjectHandle,
                        IN ACCESS_MASK DesiredAccess,
                        IN KPROCESSOR_MODE PreviousMode)
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

NTSTATUS
NTAPI
NtPrivilegeCheck(IN HANDLE ClientToken,
                 IN PPRIVILEGE_SET RequiredPrivileges,
                 OUT PBOOLEAN Result)
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
