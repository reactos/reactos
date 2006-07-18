/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ps/security.c
 * PURPOSE:         Process Manager: Process/Thread Security
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Eric Kohl
 *                  Thomas Weidenmueller (w3seek@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

/* FIXME: Turn into Macro */
VOID
NTAPI
PspLockProcessSecurityShared(IN PEPROCESS Process)
{
    /* Enter a Critical Region */
    KeEnterCriticalRegion();

    /* Lock the Process */
    //ExAcquirePushLockShared(&Process->ProcessLock);
}

/* FIXME: Turn into Macro */
VOID
NTAPI
PspUnlockProcessSecurityShared(IN PEPROCESS Process)
{
    /* Unlock the Process */
    //ExReleasePushLockShared(&Process->ProcessLock);

    /* Leave Critical Region */
    KeLeaveCriticalRegion();
}

VOID
NTAPI
PspDeleteProcessSecurity(IN PEPROCESS Process)
{
    /* Check if we have a token */
    if (Process->Token.Object)
    {
        /* Deassign it */
        SeDeassignPrimaryToken(Process);
        Process->Token.Object = NULL;
    }
}

VOID
NTAPI
PspDeleteThreadSecurity(IN PETHREAD Thread)
{
    /* Check if we have active impersonation info */
    if (Thread->ActiveImpersonationInfo)
    {
        /* Dereference its token */
        ObDereferenceObject(Thread->ImpersonationInfo->Token);
    }

    /* Check if we have impersonation info */
    if (Thread->ImpersonationInfo)
    {
        /* Free it */
        ExFreePool(Thread->ImpersonationInfo);
        Thread->ActiveImpersonationInfo = FALSE;
        Thread->ImpersonationInfo = NULL;
    }
}

NTSTATUS
NTAPI
PspInitializeProcessSecurity(IN PEPROCESS Process,
                             IN PEPROCESS Parent OPTIONAL)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PTOKEN NewToken, ParentToken;

    /* If we have a parent, then duplicate the Token */
    if (Parent)
    {
        /* Get the Parent Token */
        ParentToken = PsReferencePrimaryToken(Parent);

        /* Duplicate it */
        Status = SeSubProcessToken(ParentToken, &NewToken, TRUE, 0);

        /* Dereference the Parent */
        ObFastDereferenceObject(&Parent->Token, ParentToken);

        /* Set the new Token */
        ObInitializeFastReference(&Process->Token, NewToken);
    }
    else
    {
#ifdef SCHED_REWRITE
        PTOKEN BootToken;

        /* No parent, this is the Initial System Process. Assign Boot Token */
        BootToken = SepCreateSystemProcessToken();
        BootToken->TokenInUse = TRUE;
        Process->Token = BootToken;
        ObReferenceObject(BootToken);
#else
        DPRINT1("PspInitializeProcessSecurity called with no parent.\n");
#endif
    }

    /* Return to caller */
    return Status;
}

NTSTATUS
NTAPI
PspAssignPrimaryToken(IN PEPROCESS Process,
                      IN HANDLE TokenHandle)
{
    PACCESS_TOKEN Token, OldToken;
    NTSTATUS Status;

    /* Reference the Token */
    Status = ObReferenceObjectByHandle(TokenHandle,
                                       TOKEN_ASSIGN_PRIMARY,
                                       SepTokenObjectType,
                                       KeGetPreviousMode(),
                                       (PVOID*)&Token,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Exchange them */
    Status = SeExchangePrimaryToken(Process, Token, &OldToken);

    /* Derefernece Tokens and Return */
    ObDereferenceObject(Token);
    return Status;
}

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtOpenProcessToken(IN HANDLE ProcessHandle,
                   IN ACCESS_MASK DesiredAccess,
                   OUT PHANDLE TokenHandle)
{
    /* Call the newer API */
    return NtOpenProcessTokenEx(ProcessHandle,
                                DesiredAccess,
                                0,
                                TokenHandle);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtOpenProcessTokenEx(IN HANDLE ProcessHandle,
                     IN ACCESS_MASK DesiredAccess,
                     IN ULONG HandleAttributes,
                     OUT PHANDLE TokenHandle)
{
    PACCESS_TOKEN Token;
    HANDLE hToken;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Check if caller was user-mode */
    if (PreviousMode != KernelMode)
    {
        /* Enter SEH for probing */
        _SEH_TRY
        {
            /* Probe the token handle */
            ProbeForWriteHandle(TokenHandle);
        }
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* Fail on exception */
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Open the process token */
    Status = PsOpenTokenOfProcess(ProcessHandle, &Token);
    if(NT_SUCCESS(Status))
    {
        /* Reference it by handle and dereference the pointer */
        Status = ObOpenObjectByPointer(Token,
                                       0,
                                       NULL,
                                       DesiredAccess,
                                       SepTokenObjectType,
                                       PreviousMode,
                                       &hToken);
        ObDereferenceObject(Token);

        /* Make sure we got a handle */
        if(NT_SUCCESS(Status))
        {
            /* Enter SEH for write */
            _SEH_TRY
            {
                /* Return the handle */
                *TokenHandle = hToken;
            }
            _SEH_HANDLE
            {
                /* Get exception code */
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
        }
    }

    /* Return status */
    return Status;
}

/*
 * @implemented
 */
PACCESS_TOKEN
NTAPI
PsReferencePrimaryToken(PEPROCESS Process)
{
    PACCESS_TOKEN Token;

    /* Fast Reference the Token */
    Token = ObFastReferenceObject(&Process->Token);

    /* Check if we got the Token or if we got locked */
    if (!Token)
    {
        /* Lock the Process */
        PspLockProcessSecurityShared(Process);

        /* Do a Locked Fast Reference */
        Token = ObFastReferenceObjectLocked(&Process->Token);

        /* Unlock the Process */
        PspUnlockProcessSecurityShared(Process);
    }

    /* Return the Token */
    return Token;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
PsOpenTokenOfProcess(IN HANDLE ProcessHandle,
                     OUT PACCESS_TOKEN* Token)
{
    PEPROCESS Process;
    NTSTATUS Status;

    /* Get the Token */
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_QUERY_INFORMATION,
                                       PsProcessType,
                                       ExGetPreviousMode(),
                                       (PVOID*)&Process,
                                       NULL);
    if(NT_SUCCESS(Status))
    {
        /* Reference the token and dereference the process */
        *Token = PsReferencePrimaryToken(Process);
        ObDereferenceObject(Process);
    }

    /* Return */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
PsAssignImpersonationToken(IN PETHREAD Thread,
                           IN HANDLE TokenHandle)
{
    PACCESS_TOKEN Token;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    NTSTATUS Status;

    /* Check if we were given a handle */
    if (TokenHandle)
    {
        /* Get the token object */
        Status = ObReferenceObjectByHandle(TokenHandle,
                                           TOKEN_IMPERSONATE,
                                           SepTokenObjectType,
                                           KeGetPreviousMode(),
                                           (PVOID*)&Token,
                                           NULL);
        if (!NT_SUCCESS(Status)) return(Status);

        /* Get the impersionation level */
        ImpersonationLevel = SeTokenImpersonationLevel(Token);
    }
    else
    {
        /* Otherwise, clear values */
        Token = NULL;
        ImpersonationLevel = 0;
    }

    /* Call the impersonation API */
    Status = PsImpersonateClient(Thread,
                                 Token,
                                 FALSE,
                                 FALSE,
                                 ImpersonationLevel);

    /* Dereference the token and return status */
    if (Token) ObDereferenceObject(Token);
    return Status;
}

/*
 * @implemented
 */
VOID
NTAPI
PsRevertToSelf(VOID)
{
    /* Call the per-thread API */
    PsRevertThreadToSelf(PsGetCurrentThread());
}

/*
 * @implemented
 */
VOID
NTAPI
PsRevertThreadToSelf(IN PETHREAD Thread)
{
    /* Make sure we had impersonation information */
    if (Thread->ActiveImpersonationInfo)
    {
        /* Dereference the impersonation token and set it as false */
        ObDereferenceObject (Thread->ImpersonationInfo->Token);
        Thread->ActiveImpersonationInfo = FALSE;
    }
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
PsImpersonateClient(IN PETHREAD Thread,
                    IN PACCESS_TOKEN Token,
                    IN BOOLEAN CopyOnOpen,
                    IN BOOLEAN EffectiveOnly,
                    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel)
{
    PPS_IMPERSONATION_INFORMATION Impersonation;

    /* Check if we don't have a token */
    if (!Token)
    {
        /* Make sure we're impersonating */
        if (Thread->ActiveImpersonationInfo)
        {
            /* Disable impersonation and check for token */
            Thread->ActiveImpersonationInfo = FALSE;
            if (Thread->ImpersonationInfo->Token)
            {
                /* Dereference it */
                ObDereferenceObject(Thread->ImpersonationInfo->Token);
            }
        }

        /* Return success */
        return STATUS_SUCCESS;
    }

    /* Check if we have active impersonation */
    if (Thread->ActiveImpersonationInfo)
    {
        /* Reuse the block and reference the token */
        Impersonation = Thread->ImpersonationInfo;
        if (Impersonation->Token) ObDereferenceObject(Impersonation->Token);
    }
    else if (Thread->ImpersonationInfo)
    {
        /* It's not active, but we can still reuse the block */
        Impersonation = Thread->ImpersonationInfo;
    }
    else
    {
        /* We need to allocate a new one */
        Impersonation = ExAllocatePoolWithTag(PagedPool,
                                              sizeof(*Impersonation),
                                              TAG_PS_IMPERSONATION);
        if (!Impersonation) return STATUS_INSUFFICIENT_RESOURCES;

        /* Update the pointer */
        Thread->ImpersonationInfo = Impersonation;
    }

    /* Now fill it out */
    Impersonation->ImpersonationLevel = ImpersonationLevel;
    Impersonation->CopyOnOpen = CopyOnOpen;
    Impersonation->EffectiveOnly = EffectiveOnly;
    Impersonation->Token = Token;
    Thread->ActiveImpersonationInfo = TRUE;

    /* Reference the token and return success */
    ObReferenceObject(Impersonation->Token);
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
PACCESS_TOKEN
NTAPI
PsReferenceEffectiveToken(IN PETHREAD Thread,
                          OUT IN PTOKEN_TYPE TokenType,
                          OUT PBOOLEAN EffectiveOnly,
                          OUT PSECURITY_IMPERSONATION_LEVEL Level)
{
    PEPROCESS Process;
    PACCESS_TOKEN Token;

    /* Check if we don't have impersonation info */
    if (!Thread->ActiveImpersonationInfo)
    {
        Process = Thread->ThreadsProcess;
        *TokenType = TokenPrimary;
        *EffectiveOnly = FALSE;

        /* Fast Reference the Token */
        Token = ObFastReferenceObject(&Process->Token);

        /* Check if we got the Token or if we got locked */
        if (!Token)
        {
            /* Lock the Process */
            PspLockProcessSecurityShared(Process);

            /* Do a Locked Fast Reference */
            Token = ObFastReferenceObjectLocked(&Process->Token);

            /* Unlock the Process */
            PspUnlockProcessSecurityShared(Process);
        }
    }
    else
    {
        /* Get the token */
        Token = Thread->ImpersonationInfo->Token;
        ObReferenceObject(Token);

        /* Return data to caller */
        *TokenType = TokenImpersonation;
        *EffectiveOnly = Thread->ImpersonationInfo->EffectiveOnly;
        *Level = Thread->ImpersonationInfo->ImpersonationLevel;
    }

    /* Return the token */
    return Token;
}

/*
 * @implemented
 */
PACCESS_TOKEN
NTAPI
PsReferenceImpersonationToken(IN PETHREAD Thread,
                              OUT PBOOLEAN CopyOnOpen,
                              OUT PBOOLEAN EffectiveOnly,
                              OUT PSECURITY_IMPERSONATION_LEVEL ImpersonationLevel)
{
    /* If we don't have impersonation info, just quit */
    if (!Thread->ActiveImpersonationInfo) return NULL;

    /* Return data from caller */
    *ImpersonationLevel = Thread->ImpersonationInfo->ImpersonationLevel;
    *CopyOnOpen = Thread->ImpersonationInfo->CopyOnOpen;
    *EffectiveOnly = Thread->ImpersonationInfo->EffectiveOnly;

    /* Reference the token and return it */
    ObReferenceObject(Thread->ImpersonationInfo->Token);
    return Thread->ImpersonationInfo->Token;
}

#undef PsDereferenceImpersonationToken
/*
 * @implemented
 */
VOID
NTAPI
PsDereferenceImpersonationToken(IN PACCESS_TOKEN ImpersonationToken)
{
    /* If we got a token, dereference it */
    if (ImpersonationToken) ObDereferenceObject(ImpersonationToken);
}

#undef PsDereferencePrimaryToken
/*
 * @implemented
 */
VOID
NTAPI
PsDereferencePrimaryToken(IN PACCESS_TOKEN PrimaryToken)
{
    /* Dereference the token*/
    ObDereferenceObject(PrimaryToken);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
PsDisableImpersonation(IN PETHREAD Thread,
                       IN PSE_IMPERSONATION_STATE ImpersonationState)
{
    PPS_IMPERSONATION_INFORMATION Impersonation;

    /* Check if we don't have impersonation */
    if (!Thread->ActiveImpersonationInfo)
    {
        /* Clear everything */
        ImpersonationState->Token = NULL;
        ImpersonationState->CopyOnOpen = FALSE;
        ImpersonationState->EffectiveOnly = FALSE;
        ImpersonationState->Level = SecurityAnonymous;
        return FALSE;
    }

    /* Copy the old state */
    Impersonation = Thread->ImpersonationInfo;
    ImpersonationState->Token = Impersonation->Token;
    ImpersonationState->CopyOnOpen = Impersonation->CopyOnOpen;
    ImpersonationState->EffectiveOnly = Impersonation->EffectiveOnly;
    ImpersonationState->Level = Impersonation->ImpersonationLevel;

    /* Disable impersonation and return true */
    Thread->ActiveImpersonationInfo = FALSE;
    return TRUE;
}

/*
 * @implemented
 */
VOID
NTAPI
PsRestoreImpersonation(IN PETHREAD Thread,
                       IN PSE_IMPERSONATION_STATE ImpersonationState)
{
    /* Call the impersonation API */
    PsImpersonateClient(Thread,
                        ImpersonationState->Token,
                        ImpersonationState->CopyOnOpen,
                        ImpersonationState->EffectiveOnly,
                        ImpersonationState->Level);

    /* Dereference the token */
    ObDereferenceObject(ImpersonationState->Token);
}

NTSTATUS
NTAPI
NtImpersonateThread(IN HANDLE ThreadHandle,
                    IN HANDLE ThreadToImpersonateHandle,
                    IN PSECURITY_QUALITY_OF_SERVICE SecurityQualityOfService)
{
    SECURITY_QUALITY_OF_SERVICE SafeServiceQoS;
    SECURITY_CLIENT_CONTEXT ClientContext;
    PETHREAD Thread;
    PETHREAD ThreadToImpersonate;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Check if call came from user mode */
    if (PreviousMode != KernelMode)
    {
        /* Enter SEH for probing */
        _SEH_TRY
        {
            /* Probe QoS */
            ProbeForRead(SecurityQualityOfService,
                         sizeof(SECURITY_QUALITY_OF_SERVICE),
                         sizeof(ULONG));

            /* Capture it */
            SafeServiceQoS = *SecurityQualityOfService;
            SecurityQualityOfService = &SafeServiceQoS;
        }
        _SEH_HANDLE
        {
            /* Get exception status */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* Fail on exception */
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Reference the thread */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_IMPERSONATE,
                                       PsThreadType,
                                       PreviousMode,
                                       (PVOID*)&Thread,
                                       NULL);
    if(NT_SUCCESS(Status))
    {
        /* Reference the impersonating thead */
        Status = ObReferenceObjectByHandle(ThreadToImpersonateHandle,
                                           THREAD_DIRECT_IMPERSONATION,
                                           PsThreadType,
                                           PreviousMode,
                                           (PVOID*)&ThreadToImpersonate,
                                           NULL);
        if(NT_SUCCESS(Status))
        {
            /* Create a client security context */
            Status = SeCreateClientSecurity(ThreadToImpersonate,
                                            SecurityQualityOfService,
                                            0,
                                            &ClientContext);
            if(NT_SUCCESS(Status))
            {
                /* Do the impersonation */
                SeImpersonateClient(&ClientContext, Thread);
                if(ClientContext.ClientToken)
                {
                    /* Dereference the client token if we had one */
                    ObDereferenceObject(ClientContext.ClientToken);
                }
            }

            /* Dereference the thread to impersonate */
            ObDereferenceObject(ThreadToImpersonate);
        }

        /* Dereference the main thread */
        ObDereferenceObject(Thread);
    }

    /* Return status */
    return Status;
}
/* EOF */
