/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Security client support routines
 * COPYRIGHT:       Copyright Alex Ionescu <alex@relsoft.net>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

/**
 * @brief
 * Creates a client security context based upon an access token.
 *
 * @param[in] Token
 * A valid token object.
 *
 * @param[in] ClientSecurityQos
 * The Quality of Service (QoS) of a client security context.
 *
 * @param[in] ServerIsRemote
 * If the client is a remote server (TRUE), the function will retrieve the
 * control information of an access token, that is, we're doing delegation
 * and that the server isn't local.
 *
 * @param[in] TokenType
 * Type of token.
 *
 * @param[in] ThreadEffectiveOnly
 * If set to TRUE, the client wants that the current thread wants to modify
 * (enable or disable) privileges and groups.
 *
 * @param[in] ImpersonationLevel
 * Security impersonation level filled in the QoS context.
 *
 * @param[out] ClientContext
 * The returned security client context.
 *
 * @return
 * Returns STATUS_SUCCESS if client security creation has completed successfully.
 * STATUS_INVALID_PARAMETER is returned if one or more of the parameters are bogus.
 * STATUS_BAD_IMPERSONATION_LEVEL is returned if the current impersonation level
 * within QoS context doesn't meet with the conditions required. A failure
 * NTSTATUS code is returned otherwise.
 */
NTSTATUS
NTAPI
SepCreateClientSecurity(
    _In_ PACCESS_TOKEN Token,
    _In_ PSECURITY_QUALITY_OF_SERVICE ClientSecurityQos,
    _In_ BOOLEAN ServerIsRemote,
    _In_ TOKEN_TYPE TokenType,
    _In_ BOOLEAN ThreadEffectiveOnly,
    _In_ SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    _Out_ PSECURITY_CLIENT_CONTEXT ClientContext)
{
    NTSTATUS Status;
    PACCESS_TOKEN NewToken;
    PAGED_CODE();

    /* Check for bogus impersonation level */
    if (!VALID_IMPERSONATION_LEVEL(ClientSecurityQos->ImpersonationLevel))
    {
        /* Fail the call */
        return STATUS_INVALID_PARAMETER;
    }

    /* Check what kind of token this is */
    if (TokenType != TokenImpersonation)
    {
        /* On a primary token, if we do direct access, copy the flag from the QOS */
        ClientContext->DirectAccessEffectiveOnly = ClientSecurityQos->EffectiveOnly;
    }
    else
    {
        /* This is an impersonation token, is the level ok? */
        if (ClientSecurityQos->ImpersonationLevel > ImpersonationLevel)
        {
            /* Nope, fail */
            return STATUS_BAD_IMPERSONATION_LEVEL;
        }

        /* Is the level too low, or are we doing something other than delegation remotely */
        if ((ImpersonationLevel == SecurityAnonymous) ||
            (ImpersonationLevel == SecurityIdentification) ||
            ((ServerIsRemote) && (ImpersonationLevel != SecurityDelegation)))
        {
            /* Fail the call */
            return STATUS_BAD_IMPERSONATION_LEVEL;
        }

        /* Pick either the thread setting or the QOS setting */
        ClientContext->DirectAccessEffectiveOnly =
            ((ThreadEffectiveOnly) || (ClientSecurityQos->EffectiveOnly)) ? TRUE : FALSE;
    }

    /* Is this static tracking */
    if (ClientSecurityQos->ContextTrackingMode == SECURITY_STATIC_TRACKING)
    {
        /* Do not use direct access and make a copy */
        ClientContext->DirectlyAccessClientToken = FALSE;
        Status = SeCopyClientToken(Token,
                                   ClientSecurityQos->ImpersonationLevel,
                                   KernelMode,
                                   &NewToken);
        if (!NT_SUCCESS(Status))
            return Status;
    }
    else
    {
        /* Use direct access and check if this is local */
        ClientContext->DirectlyAccessClientToken = TRUE;
        if (ServerIsRemote)
        {
            /* We are doing delegation, so make a copy of the control data */
            SeGetTokenControlInformation(Token,
                                         &ClientContext->ClientTokenControl);
        }

        /* Keep the same token */
        NewToken = Token;
    }

    /* Fill out the context and return success */
    ClientContext->SecurityQos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    ClientContext->SecurityQos.ImpersonationLevel = ClientSecurityQos->ImpersonationLevel;
    ClientContext->SecurityQos.ContextTrackingMode = ClientSecurityQos->ContextTrackingMode;
    ClientContext->SecurityQos.EffectiveOnly = ClientSecurityQos->EffectiveOnly;
    ClientContext->ServerIsRemote = ServerIsRemote;
    ClientContext->ClientToken = NewToken;
    return STATUS_SUCCESS;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief
 * Creates a client security context.
 *
 * @param[in] Thread
 * Thread object of the client where impersonation has to begin.
 *
 * @param[in] Qos
 * Quality of service to specify what kind of impersonation to be done.
 *
 * @param[in] RemoteClient
 * If set to TRUE, the client that we're going to impersonate is remote.
 *
 * @param[out] ClientContext
 * The returned security client context.
 *
 * @return
 * See SepCreateClientSecurity.
 */
NTSTATUS
NTAPI
SeCreateClientSecurity(
    _In_ PETHREAD Thread,
    _In_ PSECURITY_QUALITY_OF_SERVICE Qos,
    _In_ BOOLEAN RemoteClient,
    _Out_ PSECURITY_CLIENT_CONTEXT ClientContext)
{
    TOKEN_TYPE TokenType;
    BOOLEAN ThreadEffectiveOnly;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    PACCESS_TOKEN Token;
    NTSTATUS Status;
    PAGED_CODE();

    /* Reference the correct token */
    Token = PsReferenceEffectiveToken(Thread,
                                      &TokenType,
                                      &ThreadEffectiveOnly,
                                      &ImpersonationLevel);

    /* Create client security from it */
    Status = SepCreateClientSecurity(Token,
                                     Qos,
                                     RemoteClient,
                                     TokenType,
                                     ThreadEffectiveOnly,
                                     ImpersonationLevel,
                                     ClientContext);

    /* Check if we failed or static tracking was used */
    if (!(NT_SUCCESS(Status)) || (Qos->ContextTrackingMode == SECURITY_STATIC_TRACKING))
    {
        /* Dereference our copy since it's not being used */
        ObDereferenceObject(Token);
    }

    /* Return status */
    return Status;
}

/**
 * @brief
 * Creates a client security context based upon the captured security
 * subject context.
 *
 * @param[in] SubjectContext
 * The captured subject context where client security is to be created
 * from.
 *
 * @param[in] ClientSecurityQos
 * Quality of service to specify what kind of impersonation to be done.
 *
 * @param[in] ServerIsRemote
 * If set to TRUE, the client that we're going to impersonate is remote.
 *
 * @param[out] ClientContext
 * The returned security client context.
 *
 * @return
 * See SepCreateClientSecurity.
 */
NTSTATUS
NTAPI
SeCreateClientSecurityFromSubjectContext(
    _In_ PSECURITY_SUBJECT_CONTEXT SubjectContext,
    _In_ PSECURITY_QUALITY_OF_SERVICE ClientSecurityQos,
    _In_ BOOLEAN ServerIsRemote,
    _Out_ PSECURITY_CLIENT_CONTEXT ClientContext)
{
    PACCESS_TOKEN Token;
    NTSTATUS Status;
    PAGED_CODE();

    /* Get the right token and reference it */
    Token = SeQuerySubjectContextToken(SubjectContext);
    ObReferenceObject(Token);

    /* Create the context */
    Status = SepCreateClientSecurity(Token,
                                     ClientSecurityQos,
                                     ServerIsRemote,
                                     SubjectContext->ClientToken ?
                                     TokenImpersonation : TokenPrimary,
                                     FALSE,
                                     SubjectContext->ImpersonationLevel,
                                     ClientContext);

    /* Check if we failed or static tracking was used */
    if (!(NT_SUCCESS(Status)) ||
        (ClientSecurityQos->ContextTrackingMode == SECURITY_STATIC_TRACKING))
    {
        /* Dereference our copy since it's not being used */
        ObDereferenceObject(Token);
    }

    /* Return status */
    return Status;
}

/**
 * @brief
 * Extended function that impersonates a client.
 *
 * @param[in] ClientContext
 * A valid client context.
 *
 * @param[in] ServerThread
 * The thread where impersonation is to be done.
 *
 * @return
 * STATUS_SUCCESS is returned if the calling thread successfully impersonates
 * the client. A failure NTSTATUS code is returned otherwise.
 */
NTSTATUS
NTAPI
SeImpersonateClientEx(
    _In_ PSECURITY_CLIENT_CONTEXT ClientContext,
    _In_opt_ PETHREAD ServerThread)
{
    BOOLEAN EffectiveOnly;
    PAGED_CODE();

    /* Check if direct access is requested */
    if (!ClientContext->DirectlyAccessClientToken)
    {
        /* No, so get the flag from QOS */
        EffectiveOnly = ClientContext->SecurityQos.EffectiveOnly;
    }
    else
    {
        /* Yes, so see if direct access should be effective only */
        EffectiveOnly = ClientContext->DirectAccessEffectiveOnly;
    }

    /* Use the current thread if one was not passed */
    if (!ServerThread) ServerThread = PsGetCurrentThread();

    /* Call the lower layer routine */
    return PsImpersonateClient(ServerThread,
                               ClientContext->ClientToken,
                               TRUE,
                               EffectiveOnly,
                               ClientContext->SecurityQos.ImpersonationLevel);
}

/**
 * @brief
 * Impersonates a client user.
 *
 * @param[in] ClientContext
 * A valid client context.
 *
 * @param[in] ServerThread
 * The thread where impersonation is to be done.
 * *
 * @return
 * Nothing.
 */
VOID
NTAPI
SeImpersonateClient(
    _In_ PSECURITY_CLIENT_CONTEXT ClientContext,
    _In_opt_ PETHREAD ServerThread)
{
    PAGED_CODE();

    /* Call the new API */
    SeImpersonateClientEx(ClientContext, ServerThread);
}

/* EOF */
