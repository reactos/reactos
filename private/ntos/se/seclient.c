
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    seclient.c

Abstract:

   This module implements routines providing client impersonation to
   communication session layers (such as LPC Ports).

        WARNING: The following notes apply to the use of these services:

        (1)  No synchronization of operations to a security context block are
             performed by these services.  The caller of these services must
             ensure that use of an individual security context block is
             serialized to prevent simultaneous, incompatible updates.

        (2)  Any or all of these services may create, open, or operate on a
             token object.  This may result in a mutex being acquired at
             MUTEXT_LEVEL_SE_TOKEN level.  The caller must ensure that no
             mutexes are held at levels that conflict with this action.


Author:

    Jim Kelly (JimK) 1-August-1990

Environment:

    Kernel mode only.

Revision History:

--*/


#include "sep.h"
#include "seopaque.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,SeCreateClientSecurity)
#pragma alloc_text(PAGE,SeUpdateClientSecurity)
#pragma alloc_text(PAGE,SeImpersonateClient)
#pragma alloc_text(PAGE,SeImpersonateClientEx)
#pragma alloc_text(PAGE,SeCreateClientSecurityFromSubjectContext)
#endif


////////////////////////////////////////////////////////////////////////
//                                                                    //
//           Routines                                                 //
//                                                                    //
////////////////////////////////////////////////////////////////////////


NTSTATUS
SepCreateClientSecurity(
    IN PACCESS_TOKEN Token,
    IN PSECURITY_QUALITY_OF_SERVICE ClientSecurityQos,
    IN BOOLEAN ServerIsRemote,
    TOKEN_TYPE TokenType,
    BOOLEAN ThreadEffectiveOnly,
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    OUT PSECURITY_CLIENT_CONTEXT ClientContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PACCESS_TOKEN DuplicateToken;

    PAGED_CODE();


    //
    // Make sure the client is not trying to abuse use of a
    // client of its own by attempting an invalid impersonation.
    // Also set the ClientContext->DirectAccessEffectiveOnly flag
    // appropriately if the impersonation is legitimate.  The
    // DirectAccessEffectiveOnly flag value will end up being ignored
    // if STATIC mode is requested, but this is the most efficient
    // place to calculate it, and we are optimizing for DYNAMIC mode.
    //

    if (TokenType == TokenImpersonation) {

        if ( ClientSecurityQos->ImpersonationLevel > ImpersonationLevel) {

            PsDereferenceImpersonationToken( Token );
            return STATUS_BAD_IMPERSONATION_LEVEL;

        }


        if ( SepBadImpersonationLevel(ImpersonationLevel,ServerIsRemote)) {

            PsDereferenceImpersonationToken( Token );
            return STATUS_BAD_IMPERSONATION_LEVEL;

        } else {

            //
            // TokenType is TokenImpersonation and the impersonation is legit.
            // Set the DirectAccessEffectiveOnly flag to be the minimum of
            // the current thread value and the caller specified value.
            //

            ClientContext->DirectAccessEffectiveOnly =
                ( (ThreadEffectiveOnly || (ClientSecurityQos->EffectiveOnly)) ?
                  TRUE : FALSE );
        }

    } else {

        //
        // TokenType is TokenPrimary.  In this case, the client specified
        // EffectiveOnly value is always used.
        //

        ClientContext->DirectAccessEffectiveOnly =
            ClientSecurityQos->EffectiveOnly;
    }



    //
    // Copy the token if necessary (i.e., static tracking requested)
    //

    if (ClientSecurityQos->ContextTrackingMode == SECURITY_STATIC_TRACKING) {

        ClientContext->DirectlyAccessClientToken = FALSE;

        Status = SeCopyClientToken(
                     Token,
                     ClientSecurityQos->ImpersonationLevel,
                     KernelMode,
                     &DuplicateToken
                     );


        if ( NT_SUCCESS(Status) ) {
            ObDeleteCapturedInsertInfo(DuplicateToken);
            }
        //
        // No longer need the pointer to the client's token
        //

        if (TokenType == TokenPrimary) {
            PsDereferencePrimaryToken( Token );
        } else {
            PsDereferenceImpersonationToken( Token );
        }

        Token = DuplicateToken;


        //
        // If there was an error, we're done.
        //
        if (!NT_SUCCESS(Status)) {
            return Status;
        }

    } else {

        ClientContext->DirectlyAccessClientToken = TRUE;

        if (ServerIsRemote) {
            //
            // Get a copy of the client token's control information
            // so that we can tell if it changes in the future.
            //

            SeGetTokenControlInformation( Token,
                                          &ClientContext->ClientTokenControl
                                          );

        }

    }



    ClientContext->SecurityQos.Length =
        (ULONG)sizeof(SECURITY_QUALITY_OF_SERVICE);

    ClientContext->SecurityQos.ImpersonationLevel =
        ClientSecurityQos->ImpersonationLevel;

    ClientContext->SecurityQos.ContextTrackingMode =
        ClientSecurityQos->ContextTrackingMode;

    ClientContext->SecurityQos.EffectiveOnly =
        ClientSecurityQos->EffectiveOnly;

    ClientContext->ServerIsRemote = ServerIsRemote;

    ClientContext->ClientToken = Token;

    return STATUS_SUCCESS;

}

NTSTATUS
SeCreateClientSecurity (
    IN PETHREAD ClientThread,
    IN PSECURITY_QUALITY_OF_SERVICE ClientSecurityQos,
    IN BOOLEAN ServerIsRemote,
    OUT PSECURITY_CLIENT_CONTEXT ClientContext
    )

/*++

Routine Description:

    This service initializes a context block to represent a client's
    security context.  This may simply result in a reference to the
    client's token, or may cause the client's token to be duplicated,
    depending upon the security quality of service information specified.

                               NOTE

        The code in this routine is optimized for DYNAMIC context
        tracking.  This is only mode in which direct access to a
        caller's token is allowed, and the mode expected to be used
        most often.  STATIC context tracking always requires the
        caller's token to be copied.


Arguments:

    ClientThread - Points to the client's thread.  This is used to
        locate the client's security context (token).

    ClientSecurityQos - Points to the security quality of service
        parameters specified by the client for this communication
        session.

    ServerIsRemote - Provides an indication as to whether the session
        this context block is being used for is an inter-system
        session or intra-system session.  This is reconciled with the
        impersonation level of the client thread's token (in case the
        client has a client of his own that didn't specify delegation).

    ClientContext - Points to the client security context block to be
        initialized.


Return Value:

    STATUS_SUCCESS - The service completed successfully.

    STATUS_BAD_IMPERSONATION_LEVEL - The client is currently
        impersonating either an Anonymous or Identification level
        token, which can not be passed on for use by another server.
        This status may also be returned if the security context
        block is for an inter-system communication session and the
        client thread is impersonating a client of its own using
        other than delegation impersonation level.


--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PACCESS_TOKEN Token;
    TOKEN_TYPE TokenType;
    BOOLEAN ThreadEffectiveOnly;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    PACCESS_TOKEN DuplicateToken;

    PAGED_CODE();

    //
    // Gain access to the client thread's effective token
    //

    Token = PsReferenceEffectiveToken(
                ClientThread,
                &TokenType,
                &ThreadEffectiveOnly,
                &ImpersonationLevel
                );


    Status = SepCreateClientSecurity(
                Token,
                ClientSecurityQos,
                ServerIsRemote,
                TokenType,
                ThreadEffectiveOnly,
                ImpersonationLevel,
                ClientContext );

    return Status ;
}



#if SAVE_FOR_PRODUCT_2




NTSTATUS
SeUpdateClientSecurity(
    IN PETHREAD ClientThread,
    IN OUT PSECURITY_CLIENT_CONTEXT ClientContext,
    OUT PBOOLEAN ChangesMade,
    OUT PBOOLEAN NewToken
    )

/*++

Routine Description:

    This service is used to update a client security context block
    based upon the client's current security context and the security
    quality of service parameters specified when the security block
    was created.  If the SecurityContextTracking specified when the
    context block was created indicated static tracking, then no
    change will be made to the context block.  Otherwise, a change may
    be made.


    An indication of whether any changes were made is returned to the
    caller.  This may be used by communication session layers
    providing remote communications to decide whether or not to send
    an updated security context to the remote server's node.  It may
    also be used by a server session layer to decide whether or not to
    inform a server that a previously obtained handle to a token no
    longer represents the current security context.


Arguments:

    ClientThread - Points to the client's thread.  This is used to
        locate the security context to synchronize with.

    ClientContext - Points to client security context block to be
        updated.

    ChangesMade - Receives an indication as to whether any changes to
        the client's security context had been made since the last
        time the security context block was synchronized.  This will
        always be FALSE if static security tracking is in effect.

    NewToken - Receives an indication as to whether the same token
        is used to represent the client's current context, or whether
        the context now points to a new token.  If the client's token
        is directly referenced, then this indicates the client changed
        tokens (and the new one is now referenced).  If the client's token
        isn't directly referenced, then this indicates it was necessary
        to delete one token and create another one.  This will always be
        FALSE if static security tracking is in effect.


Return Value:

    STATUS_SUCCESS - The service completed successfully.

    STATUS_BAD_IMPERSONATION_LEVEL - The client is currently
        impersonating either an Anonymous or Identification level
        token, which can not be passed on for use by another server.
        This status may also be returned if the security context
        block is for an inter-system communication session and the
        client thread is impersonating a client of its own using
        other than delegation impersonation level.


--*/

{
    NTSTATUS Status;
    PACCESS_TOKEN Token;
    TOKEN_TYPE TokenType;
    BOOLEAN ThreadEffectiveOnly;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    PACCESS_TOKEN DuplicateToken;
    TOKEN_CONTROL TokenControl;

    PAGED_CODE();

    if (ClientContext->SecurityQos.ContextTrackingMode ==
        SECURITY_STATIC_TRACKING) {

        (*NewToken) = FALSE;
        (*ChangesMade) = FALSE;
        return STATUS_SUCCESS;

    }


    //////////////////////////////////////////////
    //                                          //
    // Optimize for the directly accessed token //
    //                                          //
    //////////////////////////////////////////////



    //
    // Gain access to the client thread's effective token
    //

    Token = PsReferenceEffectiveToken(
                ClientThread,
                &TokenType,
                &ThreadEffectiveOnly,
                &ImpersonationLevel
                );



    //
    //  See if the token is the same.
    //


    SeGetTokenControlInformation( Token, &TokenControl );

    if ( SeSameToken( &TokenControl,
                      &ClientContext->ClientTokenControl) ) {

        (*NewToken = FALSE);


        //
        // Same token.
        // Is it unmodified?
        //

        if ( (TokenControl.ModifiedId.HighPart ==
              ClientContext->ClientTokenControl.ModifiedId.HighPart) &&
             (TokenControl.ModifiedId.LowPart  ==
              ClientContext->ClientTokenControl.ModifiedId.LowPart) )   {

            //
            // Yup.  No changes necessary.
            //

            if (TokenType == TokenPrimary) {
                PsDereferencePrimaryToken( Token );
            } else {
                PsDereferenceImpersonationToken( Token );
            }

            (*ChangesMade) = FALSE;
            return STATUS_SUCCESS;

        } else {

            //
            // Same token, but it has been modified.
            // If we are directly accessing the token, then we can
            // just indicate it has changed and return.  Otherwise
            // we have to actually update our copy of the token.
            //

            (*ChangesMade) = TRUE;
            if (ClientContext->DirectlyAccessClientToken) {

                if (TokenType == TokenPrimary) {
                    PsDereferencePrimaryToken( Token );
                } else {
                    PsDereferenceImpersonationToken( Token );
                }

                //
                // Save the new modified count and whether or not
                // the token is for effective use only
                //

                ClientContext->ClientTokenControl.ModifiedId =
                    TokenControl.ModifiedId;
                ClientContext->DirectAccessEffectiveOnly =
                ( (ThreadEffectiveOnly || (ClientContext->SecurityQos.EffectiveOnly)) ?
                  TRUE : FALSE );

                return STATUS_SUCCESS;
            } else {

                //
                //  There is a possibility for a fair performance gain here
                //  by just updating the existing token to match its origin.
                //  However, it isn't clear that this case is ever really
                //  used, so the effort and complexity is avoided at this time.
                //  If it is found that this case is used, then this code
                //  can be added.
                //
                //  Instead, we just fall through to the case of completely
                //  different tokens below.
                //
            }
        }
    }


    //
    // Not the same token, or the same token has changed.
    // In either case, we're going to create a new copy of the token
    // and dump the old copy.
    //
    // Make sure the current impersonation situation is legitimate.
    //

    (*NewToken) = TRUE;
    (*ChangesMade) = TRUE;
    if (TokenType == TokenImpersonation) {
        if ( SepBadImpersonationLevel(ImpersonationLevel,
                                      ClientContext->ServerIsRemote)) {

            PsDereferenceImpersonationToken( Token );
            return STATUS_BAD_IMPERSONATION_LEVEL;
        }
    }


    //
    // Copy the token
    //



    Status = SeCopyClientToken(
                 Token,
                 ClientContext->SecurityQos.ImpersonationLevel,
                 KernelMode,
                 &DuplicateToken
                 );


    //
    // No longer need the pointer to the client's effective token
    //

    if (TokenType == TokenPrimary) {
        PsDereferencePrimaryToken( Token );
    } else {
        PsDereferenceImpersonationToken( Token );
    }



    //
    // If there was an error, we're done.
    //
    if (!NT_SUCCESS(Status)) {
        return Status;
    }


    //
    // Otherwise, replace the current token with the new one.
    //

    Token = ClientContext->ClientToken;
    ClientContext->ClientToken = DuplicateToken;
    ClientContext->DirectlyAccessClientToken = FALSE;

    if (SeTokenType( Token ) == TokenPrimary) {
        PsDereferencePrimaryToken( Token );
    } else {
        PsDereferenceImpersonationToken( Token );
    }


    //
    // Get a copy of the current token's control information
    // so that we can tell if it changes in the future.
    //

    SeGetTokenControlInformation( DuplicateToken,
                                  &ClientContext->ClientTokenControl
                                  );


    return STATUS_SUCCESS;

}


#endif




VOID
SeImpersonateClient(
    IN PSECURITY_CLIENT_CONTEXT ClientContext,
    IN PETHREAD ServerThread OPTIONAL
    )
/*++

Routine Description:

    This service is used to cause the calling thread to impersonate a
    client.  The client security context in ClientContext is assumed to
    be up to date.


Arguments:

    ClientContext - Points to client security context block.

    ServerThread - (Optional) Specifies the thread which is to be made to
        impersonate the client.  If not specified, the calling thread is
        used.


Return Value:

    None.


--*/


{

    PAGED_CODE();

#if DBG
    DbgPrint("SE:  Obsolete call:  SeImpersonateClient\n");
#endif

    (VOID) SeImpersonateClientEx( ClientContext, ServerThread );
}


NTSTATUS
SeImpersonateClientEx(
    IN PSECURITY_CLIENT_CONTEXT ClientContext,
    IN PETHREAD ServerThread OPTIONAL
    )
/*++

Routine Description:

    This service is used to cause the calling thread to impersonate a
    client.  The client security context in ClientContext is assumed to
    be up to date.


Arguments:

    ClientContext - Points to client security context block.

    ServerThread - (Optional) Specifies the thread which is to be made to
        impersonate the client.  If not specified, the calling thread is
        used.


Return Value:

    None.


--*/


{

    BOOLEAN EffectiveValueToUse;
    PETHREAD Thread;
    NTSTATUS Status ;

    PAGED_CODE();

    if (ClientContext->DirectlyAccessClientToken) {
        EffectiveValueToUse = ClientContext->DirectAccessEffectiveOnly;
    } else {
        EffectiveValueToUse = ClientContext->SecurityQos.EffectiveOnly;
    }



    //
    // if a ServerThread wasn't specified, then default to the current
    // thread.
    //

    if (!ARGUMENT_PRESENT(ServerThread)) {
        Thread = PsGetCurrentThread();
    } else {
        Thread = ServerThread;
    }



    //
    // Assign the context to the calling thread
    //

    Status = PsImpersonateClient( Thread,
                         ClientContext->ClientToken,
                         TRUE,
                         EffectiveValueToUse,
                         ClientContext->SecurityQos.ImpersonationLevel
                         );

    return Status ;

}


NTSTATUS
SeCreateClientSecurityFromSubjectContext (
    IN PSECURITY_SUBJECT_CONTEXT SubjectContext,
    IN PSECURITY_QUALITY_OF_SERVICE ClientSecurityQos,
    IN BOOLEAN ServerIsRemote,
    OUT PSECURITY_CLIENT_CONTEXT ClientContext
    )                              
/*++

Routine Description:

    This service initializes a context block to represent a client's
    security context.  This may simply result in a reference to the
    client's token, or may cause the client's token to be duplicated,
    depending upon the security quality of service information specified.

                               NOTE

        The code in this routine is optimized for DYNAMIC context
        tracking.  This is only mode in which direct access to a
        caller's token is allowed, and the mode expected to be used
        most often.  STATIC context tracking always requires the
        caller's token to be copied.


Arguments:

    SubjectContext - Points to the SubjectContext that should serve
        as the basis for this client context.

    ClientSecurityQos - Points to the security quality of service
        parameters specified by the client for this communication
        session.

    ServerIsRemote - Provides an indication as to whether the session
        this context block is being used for is an inter-system
        session or intra-system session.  This is reconciled with the
        impersonation level of the client thread's token (in case the
        client has a client of his own that didn't specify delegation).

    ClientContext - Points to the client security context block to be
        initialized.


Return Value:

    STATUS_SUCCESS - The service completed successfully.

    STATUS_BAD_IMPERSONATION_LEVEL - The client is currently
        impersonating either an Anonymous or Identification level
        token, which can not be passed on for use by another server.
        This status may also be returned if the security context
        block is for an inter-system communication session and the
        client thread is impersonating a client of its own using
        other than delegation impersonation level.


--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PACCESS_TOKEN Token;
    TOKEN_TYPE Type;
    BOOLEAN ThreadEffectiveOnly;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    PACCESS_TOKEN DuplicateToken;

    PAGED_CODE();

    Token = SeQuerySubjectContextToken(
                SubjectContext
                );

    ObReferenceObject( Token );

    if ( SubjectContext->ClientToken )
    {
        Type = TokenImpersonation ;
    }
    else 
    {
        Type = TokenPrimary ;
    }

    Status = SepCreateClientSecurity(
                Token,
                ClientSecurityQos,
                ServerIsRemote,
                Type,
                FALSE,
                SubjectContext->ImpersonationLevel,
                ClientContext
                );

    
    return Status ;
}

