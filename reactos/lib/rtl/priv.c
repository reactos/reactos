/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * FILE:              lib/rtl/priv.c
 * PURPOSE:           Security related functions and Security Objects
 * PROGRAMMER:        Eric Kohl
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
 * @unimplemented
 */
NTSTATUS
NTAPI
RtlAcquirePrivilege(IN PULONG Privilege,
                    IN ULONG NumPriv,
                    IN ULONG Flags,
                    OUT PVOID *ReturnedState)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
NTAPI
RtlReleasePrivilege(IN PVOID ReturnedState)
{
    UNIMPLEMENTED;
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
