/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * FILE:              lib/rtl/priv.c
 * PURPOSE:           Security related functions and Security Objects
 * PROGRAMMER:        Eric Kohl
 *                    Pierre Schweitzer (pierre@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlpOpenThreadToken(IN ACCESS_MASK DesiredAccess,
                    OUT PHANDLE TokenHandle)
{
    NTSTATUS Status;

    Status = ZwOpenThreadToken(NtCurrentThread(), DesiredAccess,
                               TRUE, TokenHandle);
    if (!NT_SUCCESS(Status))
    {
        Status = ZwOpenThreadToken(NtCurrentThread(), DesiredAccess,
                                   FALSE, TokenHandle);
    }

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlImpersonateSelf(IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel)
{
    HANDLE ProcessToken;
    HANDLE ImpersonationToken;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjAttr;
    SECURITY_QUALITY_OF_SERVICE Sqos;

    PAGED_CODE_RTL();

    Status = ZwOpenProcessToken(NtCurrentProcess(),
                                TOKEN_DUPLICATE,
                                &ProcessToken);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenProcessToken() failed (Status %lx)\n", Status);
        return Status;
    }

    Sqos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    Sqos.ImpersonationLevel = ImpersonationLevel;
    Sqos.ContextTrackingMode = 0;
    Sqos.EffectiveOnly = FALSE;

    InitializeObjectAttributes(&ObjAttr,
                               NULL,
                               0,
                               NULL,
                               NULL);

    ObjAttr.SecurityQualityOfService = &Sqos;

    Status = ZwDuplicateToken(ProcessToken,
                              TOKEN_IMPERSONATE,
                              &ObjAttr,
                              Sqos.EffectiveOnly, /* why both here _and_ in Sqos? */
                              TokenImpersonation,
                              &ImpersonationToken);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtDuplicateToken() failed (Status %lx)\n", Status);
        NtClose(ProcessToken);
        return Status;
    }

    Status = ZwSetInformationThread(NtCurrentThread(),
                                    ThreadImpersonationToken,
                                    &ImpersonationToken,
                                    sizeof(HANDLE));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetInformationThread() failed (Status %lx)\n", Status);
    }

    ZwClose(ImpersonationToken);
    ZwClose(ProcessToken);

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlAcquirePrivilege(IN PULONG Privilege,
                    IN ULONG NumPriv,
                    IN ULONG Flags,
                    OUT PVOID *ReturnedState)
{
    PRTL_ACQUIRE_STATE State;
    NTSTATUS Status, IntStatus;
    ULONG ReturnLength, i, OldSize;
    SECURITY_QUALITY_OF_SERVICE Sqos;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ImpersonationToken = 0, ProcessToken;

    DPRINT("RtlAcquirePrivilege(%p, %u, %u, %p)\n", Privilege, NumPriv, Flags, ReturnedState);

    /* Validate flags */
    if (Flags & ~(RTL_ACQUIRE_PRIVILEGE_PROCESS | RTL_ACQUIRE_PRIVILEGE_IMPERSONATE))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* If user wants to acquire privileges for the process, we have to impersonate him */
    if (Flags & RTL_ACQUIRE_PRIVILEGE_PROCESS)
    {
        Flags |= RTL_ACQUIRE_PRIVILEGE_IMPERSONATE;
    }

    /* Allocate enough memory to hold: old privileges (fixed buffer size, might not be enough)
     *                                 new privileges (big enough, after old privileges memory area)
     */
    State = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(RTL_ACQUIRE_STATE) + sizeof(TOKEN_PRIVILEGES) +
                                                    (NumPriv - ANYSIZE_ARRAY) * sizeof(LUID_AND_ATTRIBUTES));
    if (!State)
    {
        return STATUS_NO_MEMORY;
    }

    /* Only zero a bit of the memory (will be faster that way) */
    State->Token = 0;
    State->OldImpersonationToken = 0;
    State->Flags = 0;
    State->OldPrivileges = NULL;

    /* Check whether we have already an active impersonation */
    if (NtCurrentTeb()->IsImpersonating)
    {
        /* Check whether we want to impersonate */
        if (Flags & RTL_ACQUIRE_PRIVILEGE_IMPERSONATE)
        {
            /* That's all fine, just get the token.
             * We need access for: adjust (obvious...) but also
             *                     query, to be able to query old privileges
             */
            Status = RtlpOpenThreadToken(TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &State->Token);
            if (!NT_SUCCESS(Status))
            {
                RtlFreeHeap(RtlGetProcessHeap(), 0, State);
                return Status;
            }
        }
        else
        {
            /* Otherwise, we have to temporary disable active impersonation.
             * Get previous impersonation token to save it
             */
            Status = RtlpOpenThreadToken(TOKEN_IMPERSONATE, &State->OldImpersonationToken);
            if (!NT_SUCCESS(Status))
            {
                RtlFreeHeap(RtlGetProcessHeap(), 0, State);
                return Status;
            }

            /* Remember the fact we had an active impersonation */
            State->Flags |= RTL_ACQUIRE_PRIVILEGE_IMPERSONATE;

            /* Revert impersonation (ie, give 0 as handle) */
            Status = ZwSetInformationThread(NtCurrentThread(),
                                            ThreadImpersonationToken,
                                            &ImpersonationToken,
                                            sizeof(HANDLE));
        }
    }

    /* If we have no token yet (which is likely) */
    if (!State->Token)
    {
        /* If we are asked to use process, then do */
        if (Flags & RTL_ACQUIRE_PRIVILEGE_PROCESS)
        {
            Status = ZwOpenProcessToken(NtCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                        &State->Token);
            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }
        }
        else
        {
            /* Otherwise, we have to impersonate.
             * Open token for duplication
             */
            Status = ZwOpenProcessToken(NtCurrentProcess(), TOKEN_DUPLICATE, &ProcessToken);

            InitializeObjectAttributes(&ObjectAttributes,
                                       NULL,
                                       0,
                                       NULL,
                                       NULL);

            ObjectAttributes.SecurityQualityOfService = &Sqos;
            Sqos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
            Sqos.ImpersonationLevel = SecurityDelegation;
            Sqos.ContextTrackingMode = 1;
            Sqos.EffectiveOnly = FALSE;

            /* Duplicate */
            Status = ZwDuplicateToken(ProcessToken,
                                      TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY | TOKEN_IMPERSONATE,
                                      &ObjectAttributes,
                                      FALSE,
                                      TokenImpersonation,
                                      &ImpersonationToken);
            if (!NT_SUCCESS(Status))
            {
                ZwClose(ProcessToken);
                goto Cleanup;
            }

            /* Assign our duplicated token to current thread */
            Status = ZwSetInformationThread(NtCurrentThread(),
                                            ThreadImpersonationToken,
                                            &ImpersonationToken,
                                            sizeof(HANDLE));
            if (!NT_SUCCESS(Status))
            {
                ZwClose(ImpersonationToken);
                ZwClose(ProcessToken);
                goto Cleanup;
            }

            /* Save said token and the fact we have impersonated */
            State->Token = ImpersonationToken;
            State->Flags |= RTL_ACQUIRE_PRIVILEGE_IMPERSONATE;

            ZwClose(ProcessToken);
        }
    }

    /* Properly set the privileges pointers:
     * OldPrivileges points to the static memory in struct (= OldPrivBuffer)
     * NewPrivileges points to the dynamic memory after OldPrivBuffer
     * There's NO overflow risks (OldPrivileges is always used with its size)
     */
    State->OldPrivileges = (PTOKEN_PRIVILEGES)State->OldPrivBuffer;
    State->NewPrivileges = (PTOKEN_PRIVILEGES)(State->OldPrivBuffer + (sizeof(State->OldPrivBuffer) / sizeof(State->OldPrivBuffer[0])));

    /* Assign all the privileges to be acquired */
    State->NewPrivileges->PrivilegeCount = NumPriv;
    for (i = 0; i < NumPriv; ++i)
    {
        State->NewPrivileges->Privileges[i].Luid.LowPart = Privilege[i];
        State->NewPrivileges->Privileges[i].Luid.HighPart = 0;
        State->NewPrivileges->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED;
    }

    /* Start privileges adjustements */
    OldSize = sizeof(State->OldPrivBuffer);
    do
    {
        ReturnLength = sizeof(State->OldPrivBuffer);
        Status = ZwAdjustPrivilegesToken(State->Token, FALSE, State->NewPrivileges,
                                         OldSize, State->OldPrivileges, &ReturnLength);
        /* This is returned when OldPrivileges buffer is too small */
        if (Status == STATUS_BUFFER_TOO_SMALL)
        {
            /* Try to allocate a new one, big enough to hold data */
            State->OldPrivileges = RtlAllocateHeap(RtlGetProcessHeap(), 0, ReturnLength);
            if (State->OldPrivileges)
            {
                DPRINT("Allocated old privileges: %p\n", State->OldPrivileges);
                OldSize = ReturnLength;
                continue;
            }
            else
            {
                /* If we failed, properly set status: we failed because of the lack of memory */
                Status = STATUS_NO_MEMORY;
            }
        }

        /* If we failed to assign at least one privilege */
        if (Status == STATUS_NOT_ALL_ASSIGNED)
        {
            /* If there was actually only one privilege to acquire, use more accurate status */
            if (NumPriv == 1)
            {
                Status = STATUS_PRIVILEGE_NOT_HELD;
            }
        }

        /* Fail if needed, otherwise return our state to caller */
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
        else
        {
            *ReturnedState = State;
            break;
        }
    } while (TRUE);

    DPRINT("RtlAcquirePrivilege succeed!\n");

    return Status;

Cleanup:
    /* If we allocated our own buffer for old privileges, release it */
    if (State->OldPrivileges && (PVOID)State->OldPrivBuffer != (PVOID)State->OldPrivileges)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, State->OldPrivileges);
    }

    /* Do we have to restore previously active impersonation? */
    if (State->Flags & RTL_ACQUIRE_PRIVILEGE_IMPERSONATE)
    {
        IntStatus = ZwSetInformationThread(NtCurrentThread(), ThreadImpersonationToken,
                                           &State->OldImpersonationToken, sizeof(HANDLE));
        /* If this ever happens, we're in a really bad situation... */
        if (!NT_SUCCESS(IntStatus))
        {
            RtlRaiseStatus(IntStatus);
        }
    }

    /* Release token */
    if (State->Token)
    {
        ZwClose(State->Token);
    }

    /* And free our state buffer */
    RtlFreeHeap(RtlGetProcessHeap(), 0, State);

    DPRINT("RtlAcquirePrivilege() failed with status: %lx\n", Status);

    return Status;
}

/*
 * @implemented
 */
VOID
NTAPI
RtlReleasePrivilege(IN PVOID ReturnedState)
{
    NTSTATUS Status;
    PRTL_ACQUIRE_STATE State = (PRTL_ACQUIRE_STATE)ReturnedState;

    DPRINT("RtlReleasePrivilege(%p)\n", ReturnedState);

    /* If we had an active impersonation before we acquired privileges
     * Or if we have impersonated, quit it
     */
    if (State->Flags & RTL_ACQUIRE_PRIVILEGE_IMPERSONATE)
    {
        /* Restore it for the current thread */
        Status = ZwSetInformationThread(NtCurrentThread(), ThreadImpersonationToken,
                                        &State->OldImpersonationToken, sizeof(HANDLE));
        if (!NT_SUCCESS(Status))
        {
            RtlRaiseStatus(Status);
        }

        /* And close the token if needed */
        if (State->OldImpersonationToken)
            ZwClose(State->OldImpersonationToken);
    }
    else
    {
        /* Otherwise, restore old state */
        Status = ZwAdjustPrivilegesToken(State->Token, FALSE,
                                         State->OldPrivileges, 0, NULL, NULL);
        if (!NT_SUCCESS(Status))
        {
            RtlRaiseStatus(Status);
        }
    }

    /* If we used a different buffer for old privileges, just free it */
    if ((PVOID)State->OldPrivBuffer != (PVOID)State->OldPrivileges)
    {
        DPRINT("Releasing old privileges: %p\n", State->OldPrivileges);
        RtlFreeHeap(RtlGetProcessHeap(), 0, State->OldPrivileges);
    }

    /* Release token and free state */
    ZwClose(State->Token);
    RtlFreeHeap(RtlGetProcessHeap(), 0, State);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlAdjustPrivilege(IN ULONG Privilege,
                   IN BOOLEAN Enable,
                   IN BOOLEAN CurrentThread,
                   OUT PBOOLEAN Enabled)
{
    TOKEN_PRIVILEGES NewState;
    TOKEN_PRIVILEGES OldState;
    ULONG ReturnLength;
    HANDLE TokenHandle;
    NTSTATUS Status;

    PAGED_CODE_RTL();

    DPRINT("RtlAdjustPrivilege() called\n");

    if (CurrentThread)
    {
        Status = ZwOpenThreadToken(NtCurrentThread(),
                                   TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                   FALSE,
                                   &TokenHandle);
    }
    else
    {
        Status = ZwOpenProcessToken(NtCurrentProcess(),
                                    TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                    &TokenHandle);
    }

    if (!NT_SUCCESS (Status))
    {
        DPRINT1("Retrieving token handle failed (Status %lx)\n", Status);
        return Status;
    }

    OldState.PrivilegeCount = 1;

    NewState.PrivilegeCount = 1;
    NewState.Privileges[0].Luid.LowPart = Privilege;
    NewState.Privileges[0].Luid.HighPart = 0;
    NewState.Privileges[0].Attributes = (Enable) ? SE_PRIVILEGE_ENABLED : 0;

    Status = ZwAdjustPrivilegesToken(TokenHandle,
                                     FALSE,
                                     &NewState,
                                     sizeof(TOKEN_PRIVILEGES),
                                     &OldState,
                                     &ReturnLength);
    ZwClose (TokenHandle);
    if (Status == STATUS_NOT_ALL_ASSIGNED)
    {
        DPRINT1("Failed to assign all privileges\n");
       return STATUS_PRIVILEGE_NOT_HELD;
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtAdjustPrivilegesToken() failed (Status %lx)\n", Status);
        return Status;
    }

    if (OldState.PrivilegeCount == 0)
    {
        *Enabled = Enable;
    }
    else
    {
        *Enabled = (OldState.Privileges[0].Attributes & SE_PRIVILEGE_ENABLED);
    }

    DPRINT("RtlAdjustPrivilege() done\n");

    return STATUS_SUCCESS;
}
