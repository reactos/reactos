//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        ctxtapi.cxx
//
// Contents:    Context APIs for the NtLm security package
//              Main entry points into this dll:
//                SpDeleteContext
//                SpInitLsaModeContext
//                SpApplyControlToken
//                SpAcceptLsaModeContext
//
// History:     ChandanS 26-Jul-1996   Stolen from kerberos\client2\ctxtapi.cxx
//
//------------------------------------------------------------------------
#define NTLM_CTXTAPI
#include <global.h>




//+-------------------------------------------------------------------------
//
//  Function:   SpDeleteContext
//
//  Synopsis:   Deletes an NtLm context
//
//  Effects:
//
//  Arguments:  ContextHandle - The context to delete
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS or STATUS_INVALID_HANDLE
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
SpDeleteContext(
    IN ULONG_PTR ContextHandle
    )
/*++

Routine Description:

    Deletes the local data structures associated with the specified
    security context.

    This API terminates a context on the local machine.

Arguments:

    ContextHandle - Handle to the context to delete


Return Value:

    STATUS_SUCCESS - Call completed successfully

    SEC_E_NO_SPM -- Security Support Provider is not running
    SEC_E_INVALID_HANDLE -- Credential/Context Handle is invalid

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG_PTR TempContextHandle = ContextHandle;
    SspPrint((SSP_API, "Entering SpDeleteContext for 0x%x\n", ContextHandle));
    // BUGBUG Check args specific to NTLM

    Status = SsprDeleteSecurityContext(
                    TempContextHandle );

    SspPrint((SSP_API, "Leaving SpDeleteContext for 0x%x\n", ContextHandle));
    return (SspNtStatusToSecStatus(Status, SEC_E_INTERNAL_ERROR));

}


//+-------------------------------------------------------------------------
//
//  Function:   SpInitLsaModeContext
//
//  Synopsis:   NtLm implementation of InitializeSecurityContext
//              while in Lsa mode. If we return TRUE in *MappedContext,
//              secur32 will call SpInitUserModeContext with
//              the returned context handle and ContextData
//              as input. Fill in whatever info needed for
//              the user mode apis
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes: This function can be called in various ways:
//         1. Generic users of ntlm make the first call to
//            InitializeSecurityContext and we return a NEGOTIATE_MESSAGE
//         2. The rdr makes the first call to InitializeSecurityContext
//            with no contextHandle but info is passed in through a
//            CHALLENGE_MESSAGE (& possibly an NTLM_CHALLENGE_MESSAGE),
//            we return an AUTHENTICATE_MESSAGE and an
//            NTLM_INITIALIZE_RESPONSE
//         3. Generic users of NTLM make the second call to
//            InitializeSecurityContext, passing in a CHALLENGE_MESSAGE
//            and we return an AUTHENTICATE_MESSAGE
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
SpInitLsaModeContext(
    IN OPTIONAL ULONG_PTR CredentialHandle,
    IN OPTIONAL ULONG_PTR OldContextHandle,
    IN OPTIONAL PUNICODE_STRING TargetName,
    IN ULONG ContextReqFlags,
    IN ULONG TargetDataRep,
    IN PSecBufferDesc InputBuffers,
    OUT PULONG_PTR NewContextHandle,
    IN OUT PSecBufferDesc OutputBuffers,
    OUT PULONG ContextAttributes,
    OUT PTimeStamp ExpirationTime,
    OUT PBOOLEAN MappedContext,
    OUT PSecBuffer ContextData
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    SecBuffer TempTokens[4];
    PSecBuffer FirstInputToken;
    PSecBuffer SecondInputToken;
    PSecBuffer FirstOutputToken;
    PSecBuffer SecondOutputToken;

    ULONG_PTR OriginalContextHandle = NULL;
    ULONG_PTR TempContextHandle = NULL;
    ULONG NegotiateFlags = 0;
    UCHAR SessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];

    SspPrint((SSP_API, "Entering SpInitLsaModeContext for Old:0x%x, New:0x%x\n", OldContextHandle, *NewContextHandle));

    // BUGBUG Check args specific to NTLM



    RtlZeroMemory(
        TempTokens,
        sizeof(TempTokens)
        );

    FirstInputToken = &TempTokens[0];
    SecondInputToken = &TempTokens[1];
    FirstOutputToken = &TempTokens[2];
    SecondOutputToken = &TempTokens[3];


    *MappedContext = FALSE;

    ASSERT(ContextData);

    ContextData->pvBuffer = NULL;
    ContextData->cbBuffer = 0;

    RtlZeroMemory(SessionKey,
                  MSV1_0_USER_SESSION_KEY_LENGTH);

#ifdef notdef  // ? RPC passes 0x10 or 0 here depending on attitude
    if ( TargetDataRep != SECURITY_NATIVE_DREP ) {
        Status = STATUS_INVALID_PARAMETER;
        SspPrint((SSP_CRITICAL, "SpInitLsaModeContext, TargetdataRep is 0x%lx\n", TargetDataRep));
        goto Cleanup;
    }
#else // notdef
    UNREFERENCED_PARAMETER( TargetDataRep );
#endif // notdef

    //
    // Extract tokens from the SecBuffers
    //

    if ( !SspGetTokenBuffer( InputBuffers,
                             0,   // get the first SECBUFFER_TOKEN
                             &FirstInputToken,
                             TRUE
                             ) ) {
        Status = SEC_E_INVALID_TOKEN;
        SspPrint((SSP_CRITICAL, "SpInitLsaModeContext, SspGetTokenBuffer (FirstInputToken) returns %d\n", Status));
        goto Cleanup;
    }

    //
    //  If we are using supplied credentials, gte the second SECBUFFER_TOKEN
    //

    if (ContextReqFlags & ISC_REQ_USE_SUPPLIED_CREDS)
    {
        if ( !SspGetTokenBuffer( InputBuffers,
                             1,   // get the second SECBUFFER_TOKEN
                             &SecondInputToken,
                             TRUE
                             ) ) {
            Status = SEC_E_INVALID_TOKEN;
            SspPrint((SSP_CRITICAL, "SpInitLsaModeContext, SspGetTokenBuffer (SecondInputToken) returns %d\n", Status));
            goto Cleanup;
        }
    }

    if ( !SspGetTokenBuffer( OutputBuffers,
                             0,   // get the first SECBUFFER_TOKEN
                             &FirstOutputToken,
                             FALSE
                             ) ) {
        Status = SEC_E_INVALID_TOKEN;
        SspPrint((SSP_CRITICAL, "SpInitLsaModeContext, SspGetTokenBuffer (FirstOutputToken) returns %d\n", Status));
        goto Cleanup;
    }

    if ( !SspGetTokenBuffer( OutputBuffers,
                             1,   // get the second SECBUFFER_TOKEN
                             &SecondOutputToken,
                             FALSE
                             ) ) {
        Status = SEC_E_INVALID_TOKEN;
        SspPrint((SSP_CRITICAL, "SpInitLsaModeContext, SspGetTokenBuffer (SecondOutputToken) returns %d\n", Status));
        goto Cleanup;
    }

    //
    // Save the old context handle, in case someone changes it
    //

    TempContextHandle = OldContextHandle;
    OriginalContextHandle = OldContextHandle;

    //
    // If no previous context was passed
    // and if no legitimate input token existed, this is the first call
    //

    if ((OriginalContextHandle == 0 ) &&
        (FirstInputToken->cbBuffer == 0))
    {

        if ( !ARGUMENT_PRESENT( CredentialHandle ) ) {
            Status = STATUS_INVALID_HANDLE;
            SspPrint((SSP_CRITICAL, "SpInitLsaModeContext, No CredentialHandle\n"));
            goto Cleanup;
        }

        *NewContextHandle = 0;

        Status = SsprHandleFirstCall(
                        CredentialHandle,
                        NewContextHandle,
                        ContextReqFlags,
                        FirstInputToken->cbBuffer,
                        FirstInputToken->pvBuffer,
                        &FirstOutputToken->cbBuffer,
                        &FirstOutputToken->pvBuffer,
                        ContextAttributes,
                        ExpirationTime,
                        SessionKey,
                        &NegotiateFlags );

        TempContextHandle = *NewContextHandle;
    //
    // If context was passed in, continue where we left off.
    // Or if the redir's passing in stuff in the InputBuffers,
    // skip the first call and get on with the second
    //

    } else {

        *NewContextHandle = OldContextHandle;

        Status = SsprHandleChallengeMessage(
                        CredentialHandle,
                        &TempContextHandle,
                        NULL,   // No client token
                        NULL,   // No logon ID
                        ContextReqFlags,
                        FirstInputToken->cbBuffer,
                        FirstInputToken->pvBuffer,
                        SecondInputToken->cbBuffer,
                        SecondInputToken->pvBuffer,
                        &FirstOutputToken->cbBuffer,
                        &FirstOutputToken->pvBuffer,
                        &SecondOutputToken->cbBuffer,
                        &SecondOutputToken->pvBuffer,
                        ContextAttributes,
                        ExpirationTime,
                        SessionKey,
                        &NegotiateFlags
                        );
    }

    //
    // If the original handle is zero, set it to be the TempContextHandle.
    // This is for the datagram case, where we map the context after the
    // first call to initialize.
    //

    if (OriginalContextHandle == 0) {

        OriginalContextHandle = TempContextHandle;
        *NewContextHandle = OriginalContextHandle;
    }
    //
    // Only map the context if this is the real authentication, not a re-auth
    // or if this was datagram.
    //

    if (((Status == SEC_I_CONTINUE_NEEDED) &&
         ((*ContextAttributes & ISC_RET_DATAGRAM) != 0)) ||
        ((Status == SEC_E_OK) &&
         ((*ContextAttributes & (SSP_RET_REAUTHENTICATION | ISC_RET_DATAGRAM)) == 0))) {

        NTSTATUS TempStatus;

        TempStatus = SspMapContext(
                        &OriginalContextHandle,
                        SessionKey,
                        NegotiateFlags,
                        NULL,               // no token handle for clients
                        NULL,               // no password expiry for clients
                        0,                  // no userflags
                        ContextData
                        );

        if (!NT_SUCCESS(TempStatus)) {
            Status = TempStatus;
            SspPrint((SSP_CRITICAL, "SpInitLsaModeContext, SspMapContext returns %d\n", Status));
            goto Cleanup;
        }

        SspPrint((SSP_SESSION_KEYS, "Init sessionkey %lx %lx %lx %lx\n",
                ((DWORD*)SessionKey)[0],
                ((DWORD*)SessionKey)[1],
                ((DWORD*)SessionKey)[2],
                ((DWORD*)SessionKey)[3]
                ));


        //
        // Yes, do load msv1_0.dll in the client's process
        // and ContextData will contain info to be passed on
        // to the InitializeSecurityContext counterpart that
        // runs in the client's process

        *MappedContext = TRUE;
    }

    //
    // Make sure this bit isn't sent to the caller
    //

    *ContextAttributes &= ~SSP_RET_REAUTHENTICATION;


Cleanup:

    // Send the output stuff
    // pvBuffer is reassigned in case ISC-REQ_ALLOCATE_MEMORY has ben defined

//    if (OutputBuffers != NULL && OutputBuffers->cBuffers > 0 && OutputBuffers->pBuffers != NULL)
//    {
//        OutputBuffers->pBuffers->cbBuffer =  FirstOutputToken.cbBuffer;
//        OutputBuffers->pBuffers->pvBuffer =  FirstOutputToken.pvBuffer;
//    }

    // Ignore TargetName.  By convention, it is being passed as \\server\ipc$.
    // This implementation makes no use of that information.  Perhaps, I'll
    // display it if I prompt for credentials.
    //
    UNREFERENCED_PARAMETER(TargetName);


    SspPrint((SSP_API, "Leaving  SpInitLsaModeContext for Old:0x%x, New:0x%x\n", OldContextHandle, *NewContextHandle));
    return (SspNtStatusToSecStatus(Status, SEC_E_INTERNAL_ERROR));
}



NTSTATUS NTAPI
SpApplyControlToken(
    IN ULONG_PTR ContextHandle,
    IN PSecBufferDesc ControlToken
    )
{
    SspPrint((SSP_API, "Entering SpApplyControlToken\n"));
    UNREFERENCED_PARAMETER(ContextHandle);
    UNREFERENCED_PARAMETER(ControlToken);
    SspPrint((SSP_API, "Leaving  SpApplyControlToken\n"));
    return(SEC_E_UNSUPPORTED_FUNCTION);
}



//+-------------------------------------------------------------------------
//
//  Function:   SpAcceptLsaModeContext
//
//  Synopsis:   NtLm implementation of AcceptSecurityContext call.
//              This routine accepts an AP request message from a client
//              and verifies that it is a valid ticket. If mutual
//              authentication is desired an AP reply is generated to
//              send back to the client.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------




NTSTATUS NTAPI
SpAcceptLsaModeContext(
    IN OPTIONAL ULONG_PTR CredentialHandle,
    IN OPTIONAL ULONG_PTR OldContextHandle,
    IN PSecBufferDesc InputBuffers,
    IN ULONG ContextReqFlags,
    IN ULONG TargetDataRep,
    OUT PULONG_PTR NewContextHandle,
    OUT PSecBufferDesc OutputBuffers,
    OUT PULONG ContextAttributes,
    OUT PTimeStamp ExpirationTime,
    OUT PBOOLEAN MappedContext,
    OUT PSecBuffer ContextData
    )
/*++

Routine Description:

    Allows a remotely initiated security context between the application
    and a remote peer to be established.  To complete the establishment of
    context one or more reply tokens may be required from remote peer.

    This function is the server counterpart to the
    InitializeSecurityContext API.  The ContextAttributes is a bit mask
    representing various context level functions viz.  delegation, mutual
    authentication, confidentiality, replay detection and sequence
    detection.  This API is used by the server side.  When a request comes
    in, the server uses the ContextReqFlags parameter to specify what
    it requires of the session.  In this fashion, a server can specify that
    clients must be capable of using a confidential or integrity checked
    session, and fail clients that can't meet that demand.  Alternatively,
    a server can require nothing, and whatever the client can provide or
    requires is returned in the pfContextAttributes parameter.  For a
    package that supports 3 leg mutual authentication, the calling sequence
    would be: Client provides a token, server calls Accept the first time,
    generating a reply token.  The client uses this in a second call to
    InitializeSecurityContext, and generates a final token.  This token is
    then used in the final call to Accept to complete the session.  Another
    example would be the LAN Manager/NT authentication style.  The client
    connects to negotiate a protocol.  The server calls Accept to set up a
    context and generate a challenge to the client.  The client calls
    InitializeSecurityContext and creates a response.  The server then
    calls Accept the final time to allow the package to verify the response
    is appropriate for the challenge.

Arguments:

   CredentialHandle - Handle to the credentials to be used to
       create the context.

   OldContextHandle - Handle to the partially formed context, if this is
       a second call (see above) or NULL if this is the first call.

   InputToken - Pointer to the input token.  In the first call this
       token can either be NULL or may contain security package specific
       information.

   ContextReqFlags - Requirements of the context, package specific.

      #define ASC_REQ_DELEGATE         0x00000001
      #define ASC_REQ_MUTUAL_AUTH      0x00000002
      #define ASC_REQ_REPLAY_DETECT    0x00000004
      #define ASC_REQ_SEQUENCE_DETECT  0x00000008
      #define ASC_REQ_CONFIDENTIALITY  0x00000010
      #define ASC_REQ_ALLOCATE_MEMORY 0x00000100
      #define ASC_REQ_USE_DCE_STYLE    0x00000200

   TargetDataRep - Long indicating the data representation (byte ordering, etc)
        on the target.  The constant SECURITY_NATIVE_DREP may be supplied
        by the transport indicating that the native format is in use.

   NewContextHandle - New context handle.  If this is a second call, this
       can be the same as OldContextHandle.

   OutputToken - Buffer to receive the output token.

   ContextAttributes -Attributes of the context established.

        #define ASC_RET_DELEGATE          0x00000001
        #define ASC_RET_MUTUAL_AUTH       0x00000002
        #define ASC_RET_REPLAY_DETECT     0x00000004
        #define ASC_RET_SEQUENCE_DETECT   0x00000008
        #define ASC_RET_CONFIDENTIALITY   0x00000010
        #define ASC_RET_ALLOCATED_BUFFERS 0x00000100
        #define ASC_RET_USED_DCE_STYLE    0x00000200

   ExpirationTime - Expiration time of the context.

Return Value:

    STATUS_SUCCESS - Message handled
    SEC_I_CONTINUE_NEEDED -- Caller should call again later

    SEC_E_NO_SPM -- Security Support Provider is not running
    SEC_E_INVALID_TOKEN -- Token improperly formatted
    SEC_E_INVALID_HANDLE -- Credential/Context Handle is invalid
    SEC_E_BUFFER_TOO_SMALL -- Buffer for output token isn't big enough
    SEC_E_LOGON_DENIED -- User is no allowed to logon to this server
    SEC_E_INSUFFICIENT_MEMORY -- Not enough memory

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;


    SecBuffer TempTokens[3];
    PSecBuffer FirstInputToken;
    PSecBuffer SecondInputToken;
    PSecBuffer FirstOutputToken;

    ULONG NegotiateFlags = 0;
    UCHAR SessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];
    HANDLE TokenHandle = NULL;
    NTSTATUS SubStatus = STATUS_SUCCESS;

    TimeStamp PasswordExpiry;
    ULONG UserFlags;

    SspPrint((SSP_API, "Entering SpAcceptLsaModeContext for Old:0x%x, New:0x%x\n", OldContextHandle, *NewContextHandle));

    FirstInputToken = &TempTokens[0];
    SecondInputToken = &TempTokens[1];
    FirstOutputToken = &TempTokens[2];

    RtlZeroMemory(
        TempTokens,
        sizeof(TempTokens)
        );



    // BUGBUG Check args specific to NTLM
    RtlZeroMemory(SessionKey,
                  MSV1_0_USER_SESSION_KEY_LENGTH);

    *MappedContext = FALSE;

    //
    // Validate the arguments
    //

#ifdef notdef  // ? RPC passes 0x10 here
    if ( TargetDataRep != SECURITY_NATIVE_DREP ) {
        Status = STATUS_INVALID_PARAMETER;
        SspPrint((SSP_CRITICAL, "SpAcceptLsaModeContext, TargetdataRep is 0x%lx\n", TargetDataRep));
        goto Cleanup;
    }
#else // notdef
    UNREFERENCED_PARAMETER( TargetDataRep );
#endif // notdef


    if ( !SspGetTokenBuffer( InputBuffers,
                             0,   // get the first SECBUFFER_TOKEN
                             &FirstInputToken,
                             TRUE ) ) {
        Status = SEC_E_INVALID_TOKEN;
        SspPrint((SSP_CRITICAL, "SpAcceptLsaModeContext, SspGetTokenBuffer (FirstInputToken) returns %d\n", Status));
        goto Cleanup;
    }

    if ( !SspGetTokenBuffer( InputBuffers,
                             1,   // get the second SECBUFFER_TOKEN
                             &SecondInputToken,
                             TRUE ) ) {
        Status = SEC_E_INVALID_TOKEN;
        SspPrint((SSP_CRITICAL, "SpAcceptLsaModeContext, SspGetTokenBuffer (SecondInputToken) returns %d\n", Status));
        goto Cleanup;
    }

    if ( !SspGetTokenBuffer( OutputBuffers,
                             0,   // get the first SECBUFFER_TOKEN
                             &FirstOutputToken,
                             FALSE ) ) {
        Status = SEC_E_INVALID_TOKEN;
        SspPrint((SSP_CRITICAL, "SpAcceptLsaModeContext, SspGetTokenBuffer (FirstOutputToken) returns %d\n", Status));
        goto Cleanup;
    }

    //
    // If no previous context was passed in this is the first call.
    //

    if ( (OldContextHandle  == 0 ) &&
         (SecondInputToken->cbBuffer == 0)) {

        if ( !ARGUMENT_PRESENT( CredentialHandle ) ) {
            Status = SEC_E_INVALID_HANDLE;
            SspPrint((SSP_CRITICAL, "SpAcceptLsaModeContext, No CredentialHandle\n"));
            goto Cleanup;
        }

        Status = SsprHandleNegotiateMessage(
                        CredentialHandle,
                        NewContextHandle,
                        ContextReqFlags,
                        FirstInputToken->cbBuffer,
                        FirstInputToken->pvBuffer,
                        &FirstOutputToken->cbBuffer,
                        &FirstOutputToken->pvBuffer,
                        ContextAttributes,
                        ExpirationTime );

    //
    // If context was passed in, continue where we left off.
    //

    } else {

        *NewContextHandle = OldContextHandle;

        Status = SsprHandleAuthenticateMessage(
                        CredentialHandle,
                        NewContextHandle,
                        ContextReqFlags,
                        FirstInputToken->cbBuffer,
                        FirstInputToken->pvBuffer,
                        SecondInputToken->cbBuffer,
                        SecondInputToken->pvBuffer,
                        &FirstOutputToken->cbBuffer,
                        &FirstOutputToken->pvBuffer,
                        ContextAttributes,
                        ExpirationTime,
                        SessionKey,
                        &NegotiateFlags,
                        &TokenHandle,
                        &SubStatus,
                        &PasswordExpiry,
                        &UserFlags
                        );

        //
        // for errors such as PASSWORD_EXPIRED, return the SubStatus, which
        // is more descriptive than the generic error code.
        //

        if( Status == STATUS_ACCOUNT_RESTRICTION )
            Status = SubStatus;
    }

    if ((Status == SEC_E_OK) &&
        !(*ContextAttributes & SSP_RET_REAUTHENTICATION)) {
        Status = SspMapContext(
                        NewContextHandle,
                        SessionKey,
                        NegotiateFlags,
                        TokenHandle,
                        &PasswordExpiry,
                        UserFlags,
                        ContextData
                        );

        if (!NT_SUCCESS(Status)) {
            SspPrint((SSP_CRITICAL, "SpAcceptLsaModeContext, SspMapContext returns %d\n", Status));
            goto Cleanup;
        }

        SspPrint((SSP_SESSION_KEYS, "Accept sessionkey %lx %lx %lx %lx\n",
                ((DWORD*)SessionKey)[0],
                ((DWORD*)SessionKey)[1],
                ((DWORD*)SessionKey)[2],
                ((DWORD*)SessionKey)[3]
                ));

        *MappedContext = TRUE;


    } else {

        //
        // Make sure this bit isn't sent to the caller
        //

        *ContextAttributes &= ~SSP_RET_REAUTHENTICATION;
    }

Cleanup:

    if (TokenHandle != NULL) {
        NtClose(TokenHandle);
    }

    SetLastError(RtlNtStatusToDosError(SubStatus));

#if DBG
    if (NT_SUCCESS(Status))
    {
        ULONG CallerID = 0;

        if (CredentialHandle != 0)
        {
            CallerID = ((PSSP_CREDENTIAL)CredentialHandle)->ClientProcessID;
        }
        else if ((*NewContextHandle != 0) && (((PSSP_CONTEXT)(*NewContextHandle))->Credential != NULL))
        {
            CallerID = ((PSSP_CONTEXT)*NewContextHandle)->Credential->ClientProcessID;
        }
        SspPrint((SSP_LEAK_TRACK, "SpAcceptLsaModeContext Context = 0x%x, PID = %d\n", *NewContextHandle, CallerID));
    }
#endif

    SspPrint((SSP_API, "Leaving SpAcceptLsaModeContext for Old:0x%x, New:0x%x\n", OldContextHandle, *NewContextHandle));

    return (SspNtStatusToSecStatus(Status, SEC_E_INTERNAL_ERROR));

}
