/*
 * PROJECT:         ReactOS Windows-Compatible Session Manager
 * LICENSE:         BSD 2-Clause License
 * FILE:            base/system/smss/smss.c
 * PURPOSE:         Main SMSS Code
 * PROGRAMMERS:     Alex Ionescu
 */

/* INCLUDES *******************************************************************/

#include "smss.h"
#define NDEBUG
#include "debug.h"

/* GLOBALS ********************************************************************/

typedef struct _SMP_PRIVILEGE_STATE
{
    HANDLE TokenHandle;
    PTOKEN_PRIVILEGES OldPrivileges;
    PTOKEN_PRIVILEGES NewPrivileges;
    UCHAR OldBuffer[1024];
    TOKEN_PRIVILEGES NewBuffer;
} SMP_PRIVILEGE_STATE, *PSMP_PRIVILEGE_STATE;

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
SmpAcquirePrivilege(IN ULONG Privilege,
                    OUT PVOID *PrivilegeState)
{
    PSMP_PRIVILEGE_STATE State;
    ULONG Size;
    NTSTATUS Status;

    /* Assume failure */
    *PrivilegeState = NULL;
    
    /* Acquire the state structure to hold everything we need */
    State = RtlAllocateHeap(RtlGetProcessHeap(),
                            0,
                            sizeof(SMP_PRIVILEGE_STATE) +
                            sizeof(TOKEN_PRIVILEGES) +
                            sizeof(LUID_AND_ATTRIBUTES));
    if (!State) return STATUS_NO_MEMORY;

    /* Open our token */
    Status = NtOpenProcessToken(NtCurrentProcess(),
                                TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                &State->TokenHandle);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        RtlFreeHeap(RtlGetProcessHeap(), 0, State);
        return Status;
    }

    /* Set one privilege in the enabled state */
    State->NewPrivileges = &State->NewBuffer;
    State->OldPrivileges = (PTOKEN_PRIVILEGES)&State->OldBuffer;
    State->NewPrivileges->PrivilegeCount = 1;
    State->NewPrivileges->Privileges[0].Luid = RtlConvertUlongToLuid(Privilege);
    State->NewPrivileges->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    /* Adjust the privileges in the token */
    Size = sizeof(State->OldBuffer);
    Status = NtAdjustPrivilegesToken(State->TokenHandle,
                                     FALSE,
                                     State->NewPrivileges,
                                     Size,
                                     State->OldPrivileges,
                                     &Size);
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        /* Our static buffer is not big enough, allocate a bigger one */
        State->OldPrivileges = RtlAllocateHeap(RtlGetProcessHeap(), 0, Size);
        if (!State->OldPrivileges)
        {
            /* Out of memory, fail */
            Status = STATUS_NO_MEMORY;
            goto Quickie;
        }

        /* Now try again */
        Status = NtAdjustPrivilegesToken(State->TokenHandle,
                                         FALSE,
                                         State->NewPrivileges,
                                         Size,
                                         State->OldPrivileges,
                                         &Size);
    }

    /* Normalize failure code and check for success */
    if (Status == STATUS_NOT_ALL_ASSIGNED) Status = STATUS_PRIVILEGE_NOT_HELD;
    if (NT_SUCCESS(Status))
    {
        /* We got the privilege, return */
        *PrivilegeState = State;
        return STATUS_SUCCESS;
    }

Quickie:
    /* Check if we used a dynamic buffer */
    if (State->OldPrivileges != (PTOKEN_PRIVILEGES)&State->OldBuffer)
    {
        /* Free it */
        RtlFreeHeap(RtlGetProcessHeap(), 0, State->OldPrivileges);
    }

    /* Close the token handle and free the state structure */
    NtClose(State->TokenHandle);
    RtlFreeHeap(RtlGetProcessHeap(), 0, State);
    return Status;
}

VOID
NTAPI
SmpReleasePrivilege(IN PVOID PrivState)
{
    PSMP_PRIVILEGE_STATE State = (PSMP_PRIVILEGE_STATE)PrivState;
    
    /* Adjust the privileges in the token */
    NtAdjustPrivilegesToken(State->TokenHandle,
                            FALSE,
                            State->OldPrivileges,
                            0,
                            NULL,
                            NULL);

    /* Check if we used a dynamic buffer */
    if (State->OldPrivileges != (PTOKEN_PRIVILEGES)&State->OldBuffer)
    {
        /* Free it */
        RtlFreeHeap(RtlGetProcessHeap(), 0, State->OldPrivileges);
    }

    /* Close the token handle and free the state structure */
    NtClose(State->TokenHandle);
    RtlFreeHeap(RtlGetProcessHeap(), 0, State);
}
