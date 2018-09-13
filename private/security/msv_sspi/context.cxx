/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    context.cxx

Abstract:

    API and support routines for handling security contexts.

Author:

    Cliff Van Dyke (CliffV) 13-Jul-1993

Revision History:
    ChandanS 03-Aug-1996  Stolen from net\svcdlls\ntlmssp\common\context.c

--*/


//
// Common include files.
//

#include <global.h>
#include <align.h>      // ALIGN_WCHAR, etc
extern "C"
{
#include <crypt.h>      // Encryption constants and routine
#include <rc4.h>        // RC4 encryption types and functions
#include <md5.h>
}

// Globals for manipulating Context Lists while in Lsa Mode

CRITICAL_SECTION SspContextCritSect;
LIST_ENTRY       SspContextList;

//
// Performance counters
//

#define TRACK_REQUESTS
#ifdef TRACK_REQUESTS
LONG ServerAuthentications;
LONG ClientAuthentications;
#define INC_CLIENT_AUTH() (InterlockedIncrement(&ClientAuthentications))
#define INC_SERVER_AUTH() (InterlockedIncrement(&ServerAuthentications))
#else
#define INC_CLIENT_AUTH()
#define INC_SERVER_AUTH()
#endif


PSSP_CONTEXT
SspContextReferenceContext(
    IN ULONG_PTR ContextHandle,
    IN BOOLEAN RemoveContext
    )

/*++

Routine Description:

    This routine checks to see if the Context is for the specified
    Client Connection, and references the Context if it is valid.

    The caller may optionally request that the Context be
    removed from the list of valid Contexts - preventing future
    requests from finding this Context.

Arguments:

    ContextHandle - Points to the ContextHandle of the Context
        to be referenced.

    RemoveContext - This boolean value indicates whether the caller
        wants the Context to be removed from the list
        of Contexts.  TRUE indicates the Context is to be removed.
        FALSE indicates the Context is not to be removed.


Return Value:

    NULL - the Context was not found.

    Otherwise - returns a pointer to the referenced Context.

--*/

{
    PLIST_ENTRY ListEntry;
    PSSP_CONTEXT Context;

    SspPrint(( SSP_API_MORE, "Entering SspContextReferenceContext\n" ));


#if 0

    //
    // check that usermode/kernel mode caller matches.
    //

    SECPKG_CALL_INFO CallInfo;
    BOOLEAN KernelCaller;

    if( LsaFunctions->GetCallInfo(&CallInfo) ) {
        KernelCaller = ((CallInfo.Attributes & SECPKG_CALL_KERNEL_MODE) != 0);
    }

#endif



    //
    // Acquire exclusive access to the Context list
    //

    EnterCriticalSection( &SspContextCritSect );

    //
    // Now walk the list of Contexts looking for a match.
    //

    for ( ListEntry = SspContextList.Flink;
          ListEntry != &SspContextList;
          ListEntry = ListEntry->Flink ) {

        Context = CONTAINING_RECORD( ListEntry, SSP_CONTEXT, Next );

        //
        // Found a match ... reference this Context
        // (if the Context is being removed, we would increment
        // and then decrement the reference, so don't bother doing
        // either - since they cancel each other out).
        //

        if ( Context == (PSSP_CONTEXT) ContextHandle)
        {

#if 0
            ASSERT( (KernelCaller == Context->KernelClient) );
#endif
            if (!RemoveContext)
            {
                //
                // Timeout this context if caller is not trying to remove it.
                // We only timeout contexts that are being setup, not
                // fully authenticated contexts.
                //

                if (SspTimeHasElapsed( Context->StartTime, Context->Interval))
                {
                    if ((Context->State != AuthenticatedState) &&
                        (Context->State != AuthenticateSentState) &&
                        (Context->State != PassedToServiceState))
                    {
                        SspPrint(( SSP_CRITICAL, "Context 0x%lx has timed out.\n",
                                    ContextHandle ));

                        LeaveCriticalSection( &SspContextCritSect );
                        return NULL;
                    }
                }

                Context->References += 1;
            }
            else
            {
                RemoveEntryList( &Context->Next );
                SspPrint(( SSP_API_MORE, "Delinked Context 0x%lx\n",
                           Context ));
            }

            SspPrint(( SSP_LEAK_TRACK, "SspContextReferenceContext for Context = 0x%x, RemoveContext = %d, ReferenceCount = %d\n", Context, RemoveContext, Context->References));
            LeaveCriticalSection( &SspContextCritSect );
            SspPrint(( SSP_API_MORE, "Leaving SspContextReferenceContext\n" ));
            return Context;
        }

    }

    //
    // No match found
    //

    SspPrint(( SSP_API_MORE, "Tried to reference unknown Context 0x%lx\n",
               ContextHandle ));

    LeaveCriticalSection( &SspContextCritSect );
    return NULL;

}

VOID
SspContextDereferenceContext(
    PSSP_CONTEXT Context
    )

/*++

Routine Description:

    This routine decrements the specified Context's reference count.
    If the reference count drops to zero, then the Context is deleted

Arguments:

    Context - Points to the Context to be dereferenced.


Return Value:

    None.

--*/

{

    ULONG References;

    SspPrint(( SSP_API_MORE, "Entering SspContextDereferenceContext\n" ));
    //
    // Decrement the reference count
    //

    EnterCriticalSection( &SspContextCritSect );
    ASSERT( Context->References >= 1 );
    References = -- Context->References;
    SspPrint(( SSP_LEAK_TRACK, "SspContextDereferenceContext for Context = 0x%x, ReferenceCount = %d\n", Context, Context->References));
    LeaveCriticalSection( &SspContextCritSect );

    //
    // If the count dropped to zero, then run-down the Context
    //

    if (References == 0) {

        SspPrint(( SSP_API_MORE, "Deleting Context 0x%lx\n",
                   Context ));

        if ( Context->DomainName.Buffer != NULL ) {
            (VOID) NtLmFree( Context->DomainName.Buffer );
        }
        if ( Context->UserName.Buffer != NULL ) {
            (VOID) NtLmFree( Context->UserName.Buffer );
        }
        if ( Context->Password.Buffer != NULL ) {
            // note: Password.Length may contain run-encoding hint, so size may be illegal.
            ZeroMemory( Context->Password.Buffer, Context->Password.MaximumLength );
            (VOID) NtLmFree( Context->Password.Buffer );
        }
        if ( Context->TokenHandle != NULL ) {
            NTSTATUS IgnoreStatus;
            IgnoreStatus = NtClose( Context->TokenHandle );
            ASSERT( NT_SUCCESS(IgnoreStatus) );
        }
        if (Context->Credential != NULL) {
            SspCredentialDereferenceCredential( Context->Credential );
        }

        ZeroMemory( Context, sizeof(SSP_CONTEXT) );
        (VOID) NtLmFree( Context );

    }

    return;

}

PSSP_CONTEXT
SspContextAllocateContext(
    )

/*++

Routine Description:

    This routine allocates the security context block, initializes it and
    links it onto the specified credential.

Arguments: None

Return Value:

    NULL -- Not enough memory to allocate context.

    otherwise -- pointer to allocated and referenced context.

--*/

{

    SspPrint(( SSP_API_MORE, "Entering SspContextAllocateContext\n" ));
    PSSP_CONTEXT Context;
    SECPKG_CALL_INFO CallInfo;


    //
    // Allocate a Context block and initialize it.
    //

    Context = (PSSP_CONTEXT)NtLmAllocate(sizeof(SSP_CONTEXT) );

    if ( Context == NULL ) {
        SspPrint(( SSP_CRITICAL, "SspContextAllocateContext: Error allocating Context.\n" ));
        return NULL;
    }

    ZeroMemory( Context, sizeof(SSP_CONTEXT) );


    if( LsaFunctions->GetCallInfo(&CallInfo) ) {
        Context->ClientProcessID = CallInfo.ProcessId;
        Context->KernelClient = ((CallInfo.Attributes & SECPKG_CALL_KERNEL_MODE) != 0);
    }

    //
    // The reference count is set to 2.  1 to indicate it is on the
    // valid Context list, and one for the our own reference.
    //

    Context->References = 2;
    Context->State = IdleState;

    //
    // Timeout this context.
    //

    (VOID) NtQuerySystemTime( &Context->StartTime );
    Context->Interval = NTLMSSP_MAX_LIFETIME;

    //
    // Add it to the list of valid Context handles.
    //

    EnterCriticalSection( &SspContextCritSect );
    InsertHeadList( &SspContextList, &Context->Next );
    SspPrint(( SSP_LEAK_TRACK, "SspContextAllocateContext for Context = 0x%x, ReferenceCount = %d\n", Context, Context->References));
    LeaveCriticalSection( &SspContextCritSect );

    SspPrint(( SSP_API_MORE, "Added Context 0x%lx\n", Context ));

    SspPrint(( SSP_API_MORE, "Leaving SspContextAllocateContext\n" ));
    return Context;

}

NTSTATUS
SspContextGetMessage(
    IN PVOID InputMessage,
    IN ULONG InputMessageSize,
    IN NTLM_MESSAGE_TYPE ExpectedMessageType,
    OUT PVOID* OutputMessage
    )

/*++

Routine Description:

    This routine copies the InputMessage into the local address space.
    This routine then validates the message header.

Arguments:

    InputMessage - Address of the message in the client process.

    InputMessageSize - Size of the message (in bytes).

    ExpectedMessageType - The type of message the should be in the message
        header.

    OutputMessage - Returns a pointer to an allocated buffer that contains
        the message.  The buffer should be freed using NtLmFree.


Return Value:

    STATUS_SUCCESS - Call completed successfully

    SEC_E_INVALID_TOKEN -- Message improperly formatted
    SEC_E_INSUFFICIENT_MEMORY -- Not enough memory to allocate message

--*/

{

    NTSTATUS Status = STATUS_SUCCESS;
    PNEGOTIATE_MESSAGE TypicalMessage = NULL;

    //
    // Allocate a local buffer for the message.
    //

    ASSERT( NTLMSP_MAX_TOKEN_SIZE >= NTLMSSP_MAX_MESSAGE_SIZE );
    if ( InputMessageSize > NTLMSSP_MAX_MESSAGE_SIZE ) {
        Status = SEC_E_INVALID_TOKEN;
        SspPrint(( SSP_CRITICAL, "SspContextGetMessage, invalid input message size.\n" ));
        goto Cleanup;
    }

    TypicalMessage = (PNEGOTIATE_MESSAGE)NtLmAllocate(InputMessageSize );

    if ( TypicalMessage == NULL ) {
        Status = STATUS_NO_MEMORY;
        SspPrint(( SSP_CRITICAL, "SspContextGetMessage: Error allocating TypicalMessage.\n" ));
        goto Cleanup;
    }

    //
    // Copy the message into the buffer
    //

    RtlCopyMemory( TypicalMessage,
                   InputMessage,
                   InputMessageSize );

    //
    // Validate the message header.
    //

    if ( strncmp( (const char *)TypicalMessage->Signature,
                  NTLMSSP_SIGNATURE,
                  sizeof(NTLMSSP_SIGNATURE)) != 0 ||
         TypicalMessage->MessageType != ExpectedMessageType ) {

        (VOID) NtLmFree( TypicalMessage );
        TypicalMessage = NULL;
        Status = SEC_E_INVALID_TOKEN;
        SspPrint(( SSP_CRITICAL, "SspContextGetMessage, Bogus Message.\n" ));
        goto Cleanup;

    }

Cleanup:

    *OutputMessage = TypicalMessage;

    return Status;

}

VOID
SspContextCopyString(
    IN PVOID MessageBuffer,
    OUT PSTRING32 OutString,
    IN PSTRING InString,
    IN OUT PCHAR *Where
    )

/*++

Routine Description:

    This routine copies the InString into the MessageBuffer at Where.
    It then updates OutString to be a descriptor for the copied string.  The
    descriptor 'address' is an offset from the MessageBuffer.

    Where is updated to point to the next available space in the MessageBuffer.

    The caller is responsible for any alignment requirements and for ensuring
    there is room in the buffer for the string.

Arguments:

    MessageBuffer - Specifies the base address of the buffer being copied into.

    OutString - Returns a descriptor for the copied string.  The descriptor
        is relative to the begining of the buffer.

    InString - Specifies the string to copy.

    Where - On input, points to where the string is to be copied.
        On output, points to the first byte after the string.

Return Value:

    None.

--*/

{
    //
    // Copy the data to the Buffer.
    //

    if ( InString->Buffer != NULL ) {
        RtlCopyMemory( *Where, InString->Buffer, InString->Length );
    }

    //
    // Build a descriptor to the newly copied data.
    //

    OutString->Length = OutString->MaximumLength = InString->Length;
    OutString->Buffer = (ULONG)(*Where - ((PCHAR)MessageBuffer));


    //
    // Update Where to point past the copied data.
    //

    *Where += InString->Length;

}

VOID
SspContextCopyStringAbsolute(
    IN PVOID MessageBuffer,
    OUT PSTRING OutString,
    IN PSTRING InString,
    IN OUT PCHAR *Where
    )

/*++

Routine Description:

    This routine copies the InString into the MessageBuffer at Where.
    It then updates OutString to be a descriptor for the copied string.

    Where is updated to point to the next available space in the MessageBuffer.

    The caller is responsible for any alignment requirements and for ensuring
    there is room in the buffer for the string.

Arguments:

    MessageBuffer - Specifies the base address of the buffer being copied into.

    OutString - Returns a descriptor for the copied string.  The descriptor
        is relative to the begining of the buffer.

    InString - Specifies the string to copy.

    Where - On input, points to where the string is to be copied.
        On output, points to the first byte after the string.

Return Value:

    None.

--*/

{
    //
    // Copy the data to the Buffer.
    //

    if ( InString->Buffer != NULL ) {
        RtlCopyMemory( *Where, InString->Buffer, InString->Length );
    }

    //
    // Build a descriptor to the newly copied data.
    //

    OutString->Length = OutString->MaximumLength = InString->Length;
    OutString->Buffer = *Where;

    //
    // Update Where to point past the copied data.
    //

    *Where += InString->Length;

}

BOOLEAN
SspConvertRelativeToAbsolute (
    IN PVOID MessageBase,
    IN ULONG MessageSize,
    IN PSTRING32 StringToRelocate,
    IN PSTRING OutputString,
    IN BOOLEAN AlignToWchar,
    IN BOOLEAN AllowNullString
    )

/*++

Routine Description:

    Convert a Relative string desriptor to be absolute.
    Perform all boudary condition testing.

Arguments:

    MessageBase - a pointer to the base of the buffer that the string
        is relative to.  The MaximumLength field of the descriptor is
        forced to be the same as the Length field.

    MessageSize - Size of the message buffer (in bytes).

    StringToRelocate - A pointer to the string descriptor to make absolute.

    AlignToWchar - If TRUE the passed in StringToRelocate must describe
        a buffer that is WCHAR aligned.  If not, an error is returned.

    AllowNullString - If TRUE, the passed in StringToRelocate may be
        a zero length string.

Return Value:

    TRUE - The string descriptor is valid and was properly relocated.

--*/

{
    ULONG_PTR Offset;

    //
    // If the buffer is allowed to be null,
    //  check that special case.
    //

    if ( AllowNullString ) {
        if ( StringToRelocate->Length == 0 ) {
            OutputString->MaximumLength = OutputString->Length = StringToRelocate->Length;
            OutputString->Buffer = NULL;
            return TRUE;
        }
    }

    //
    // Ensure the string in entirely within the message.
    //

    Offset = (ULONG_PTR) StringToRelocate->Buffer;

    if ( Offset >= MessageSize ||
         Offset + StringToRelocate->Length > MessageSize ) {
        return FALSE;
    }

    //
    // Ensure the buffer is properly aligned.
    //

    if ( AlignToWchar ) {
        if ( !COUNT_IS_ALIGNED( Offset, ALIGN_WCHAR) ||
             !COUNT_IS_ALIGNED( StringToRelocate->Length, ALIGN_WCHAR) ) {
            return FALSE;
        }
    }

    //
    // Finally make the pointer absolute.
    //

    OutputString->Buffer = (((PCHAR)MessageBase) + Offset);
    OutputString->MaximumLength = OutputString->Length = StringToRelocate->Length ;

    return TRUE;

}


TimeStamp
SspContextGetTimeStamp(
    IN PSSP_CONTEXT Context,
    IN BOOLEAN GetExpirationTime
    )
/*++

Routine Description:

    Get the Start time or Expiration time for the specified context.

Arguments:

    Context - Pointer to the context to query

    GetExpirationTime - If TRUE return the expiration time.
        Otherwise, return the start time for the context.

Return Value:

    Returns the requested time as a local time.

--*/

{
    NTSTATUS Status;
    LARGE_INTEGER SystemTime;
    LARGE_INTEGER LocalTime;
    TimeStamp LocalTimeStamp;

    //
    // Get the requested time in NT system time format.
    //

    SystemTime = Context->StartTime;

    if ( GetExpirationTime ) {
        LARGE_INTEGER Interval;

        //
        // If the time is infinite, return that
        //

        if ( Context->Interval == INFINITE ) {
            return NtLmGlobalForever;
        }

        //
        // Compute the ending time in NT System Time.
        //

        Interval.QuadPart = Int32x32To64( (LONG) Context->Interval, 10000 );
        SystemTime.QuadPart = Interval.QuadPart + SystemTime.QuadPart;
    }

    //
    // Convert the time to local time
    //

    Status = RtlSystemTimeToLocalTime( &SystemTime, &LocalTime );

    if ( !NT_SUCCESS(Status) ) {
        return NtLmGlobalForever;
    }

    LocalTimeStamp.HighPart = LocalTime.HighPart;
    LocalTimeStamp.LowPart = LocalTime.LowPart;

    return LocalTimeStamp;

}

VOID
SspContextSetTimeStamp(
    IN PSSP_CONTEXT Context,
    IN LARGE_INTEGER ExpirationTime
    )
/*++

Routine Description:

    Set the Expiration time for the specified context.

Arguments:

    Context - Pointer to the context to change

    ExpirationTime - Expiration time to set

Return Value:

    NONE.

--*/

{

    LARGE_INTEGER BaseGetTickMagicDivisor = { 0xe219652c, 0xd1b71758 };
    CCHAR BaseGetTickMagicShiftCount = 13;

    LARGE_INTEGER TimeRemaining;
    LARGE_INTEGER MillisecondsRemaining;

    //
    // If the expiration time is infinite,
    //  so is the interval
    //

    if ( ExpirationTime.HighPart == 0x7FFFFFFF &&
         ExpirationTime.LowPart == 0xFFFFFFFF ) {
        Context->Interval = INFINITE;

    //
    // Handle non-infinite expiration times
    //

    } else {

        //
        // Compute the time remaining before the expiration time
        //

        TimeRemaining.QuadPart = ExpirationTime.QuadPart -
                                 Context->StartTime.QuadPart;

        //
        // If the time has already expired,
        //  indicate so.
        //

        if ( TimeRemaining.QuadPart < 0 ) {

            Context->Interval = 0;

        //
        // If the time hasn't expired, compute the number of milliseconds
        //  remaining.
        //

        } else {

            MillisecondsRemaining = RtlExtendedMagicDivide(
                                        TimeRemaining,
                                        BaseGetTickMagicDivisor,
                                        BaseGetTickMagicShiftCount );

            if ( MillisecondsRemaining.HighPart == 0 &&
                 MillisecondsRemaining.LowPart < 0x7fffffff ) {

                Context->Interval = MillisecondsRemaining.LowPart;

            } else {

                Context->Interval = INFINITE;
            }
        }

    }

}


BOOL
SsprCheckMinimumSecurity(
    IN ULONG NegotiateFlags,
    IN ULONG MinimumSecurityFlags
    )
/*++
Routine Description:

    Check that minimum security requirements have been met.

Arguments:

    NegotiateFlags: requested security features
    MinimumSecurityFlags: minimum required features

Return Value:
    TRUE    if minimum requirements met
    FALSE   otherwise

Notes:
    The MinimumSecurityFlags can contain features that only apply if
    a key is needed when doing signing or sealing. These have to be removed
    if SIGN or SEAL is not in the NegotiateFlags.

--*/
{
    ULONG EffFlags;     // flags in effect

    EffFlags = MinimumSecurityFlags;


    if( (NegotiateFlags & NTLMSSP_NEGOTIATE_SIGN) == 0 )
        EffFlags &= ~(NTLMSSP_NEGOTIATE_SIGN);


    if( (NegotiateFlags & NTLMSSP_NEGOTIATE_SEAL) == 0 )
        EffFlags &= ~(NTLMSSP_NEGOTIATE_SEAL);


    //
    // if SIGN or SEAL is not negotiated, then remove all key related
    //  requirements, since they're not relevant when a key isn't needed
    //

    if ((NegotiateFlags & (NTLMSSP_NEGOTIATE_SIGN | NTLMSSP_NEGOTIATE_SEAL)) == 0)
    {
        EffFlags &= ~(
                NTLMSSP_NEGOTIATE_128 |
                NTLMSSP_NEGOTIATE_56 |
                NTLMSSP_NEGOTIATE_KEY_EXCH
                );
    } else if ((NegotiateFlags & NTLMSSP_NEGOTIATE_SEAL) == 0) {

        //
        // If SIGN is negotiated, but SEAL isn't, then remove flags
        //  that aren't relevant to encryption
        //

        EffFlags &= ~( NTLMSSP_NEGOTIATE_KEY_EXCH );
    }

    //
    // FYI: flags that can be usefully spec'd even without SIGN or SEAL:
    //      NTLM2 -- forces stronger authentication
    //  All other flags should never be set.... and are nuked in initcomn
    //

    return ((NegotiateFlags & EffFlags) == EffFlags);
}


SECURITY_STATUS
SsprMakeSessionKey(
    IN  PSSP_CONTEXT Context,
    IN  PSTRING LmChallengeResponse,
    IN  UCHAR NtUserSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH], // from the DC or GetChalResp
    IN  UCHAR LanmanSessionKey[MSV1_0_LANMAN_SESSION_KEY_LENGTH],     // from the DC of GetChalResp
    IN  PSTRING DatagramSessionKey
    )

/*++
// SsprMakeSessionKey
//  on entry:
//      if KEY_EXCH has been negotiated, then
//          either Context->SessionKey has a random number to be encrypted
//          to be sent to the server, or it has the encrypted session key
//          received from the client
//          if client, DatagramSessionKey must point to STRING set up to hold 16 byte key,
//              but with 0 length.
//      else Context->SessionKey and DatagramSessionKey are irrelevant on entry
//  on exit:
//      Context->SessionKey has the session key to be used for the rest of the session
//      if (DatagramSessionKey != NULL) then if KEY_EXCH then it has the encrypted session key
//      to send to the server, else it is zero length
//
--*/

{
    NTSTATUS Status;
    UCHAR LocalSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];

    // if we don't need to make any keys, just return
// RDR/SRV expect session key but don't ask for it! work-around this...
//    if ((Context->NegotiateFlags & (NTLMSSP_NEGOTIATE_SIGN| NTLMSSP_NEGOTIATE_SEAL)) == 0)
//        return(SEC_E_OK);

    if ((Context->NegotiateFlags & (NTLMSSP_NEGOTIATE_SIGN| NTLMSSP_NEGOTIATE_SEAL)) == 0)
    {

        RtlCopyMemory(
            Context->SessionKey,
            NtUserSessionKey,
            sizeof(LocalSessionKey)
            );

        return SEC_E_OK;
    }

    if (Context->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2)
    {
        //
        // when using NTLM2, LSA gets passed flags that cause
        //  it to make good session keys -- nothing for us to do
        //

        RtlCopyMemory(
            LocalSessionKey,
            NtUserSessionKey,
            sizeof(LocalSessionKey)
            );
    }
    else if( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_LM_KEY )
    {
        LM_OWF_PASSWORD LmKey;
        LM_RESPONSE LmResponseKey;

        BYTE TemporaryResponse[ LM_RESPONSE_LENGTH ];

        RtlZeroMemory(
            LocalSessionKey,
            MSV1_0_USER_SESSION_KEY_LENGTH
            );

        if (LmChallengeResponse->Length > LM_RESPONSE_LENGTH)
            return(SEC_E_UNSUPPORTED_FUNCTION);

        ZeroMemory( TemporaryResponse, sizeof(TemporaryResponse) );
        CopyMemory( TemporaryResponse, LmChallengeResponse->Buffer, LmChallengeResponse->Length );

        //
        // The LM session key is made by taking the LM sesion key
        // given to us by the LSA, extending it to LM_OWF_LENGTH
        // with our salt, and then producing a new challenge-response
        // with it and the original challenge response.  The key is
        // made from the first 8 bytes of the key.
        //

        RtlCopyMemory(  &LmKey,
                        LanmanSessionKey,
                        MSV1_0_LANMAN_SESSION_KEY_LENGTH );

        memset( (PUCHAR)(&LmKey) + MSV1_0_LANMAN_SESSION_KEY_LENGTH,
                NTLMSSP_KEY_SALT,
                LM_OWF_PASSWORD_LENGTH - MSV1_0_LANMAN_SESSION_KEY_LENGTH );

        Status = RtlCalculateLmResponse(
                    (PLM_CHALLENGE) TemporaryResponse,
                    &LmKey,
                    &LmResponseKey
                    );

        ZeroMemory( TemporaryResponse, sizeof(TemporaryResponse) );
        if (!NT_SUCCESS(Status))
            return(SspNtStatusToSecStatus(Status, SEC_E_NO_CREDENTIALS));

        RtlCopyMemory(
            LocalSessionKey,
            &LmResponseKey,
            MSV1_0_USER_SESSION_KEY_LENGTH
            );
    } else {

        RtlCopyMemory(
            LocalSessionKey,
            NtUserSessionKey,
            MSV1_0_USER_SESSION_KEY_LENGTH
            );
    }


    //
    // If we aren't doing key exchange, store the session key in the
    // context.  Otherwise encrypt the session key to send to the
    // server.
    //

    if (Context->NegotiateFlags & NTLMSSP_NEGOTIATE_KEY_EXCH) {

        struct RC4_KEYSTRUCT Rc4Key;

        //
        // make a key schedule from the temp key to form key exchange key
        //

        rc4_key(
            &Rc4Key,
            MSV1_0_USER_SESSION_KEY_LENGTH,
            LocalSessionKey
            );

        if (DatagramSessionKey == NULL)
        {
            //
            // decrypt what's in Context->SessionKey, leave it there
            //

            rc4(
                &Rc4Key,
                MSV1_0_USER_SESSION_KEY_LENGTH,
                Context->SessionKey
                );
        } else {

            //
            // set the proper length so client will send something (length was 0)
            //

            DatagramSessionKey->Length =
                DatagramSessionKey->MaximumLength =
                    MSV1_0_USER_SESSION_KEY_LENGTH;

            //
            // copy randomly generated key to buffer to send to server
            //

            RtlCopyMemory(
                DatagramSessionKey->Buffer,
                Context->SessionKey,
                MSV1_0_USER_SESSION_KEY_LENGTH
                );

            //
            // encrypt it with the key exchange key
            //

            rc4(
                &Rc4Key,
                MSV1_0_USER_SESSION_KEY_LENGTH,
                (unsigned char*)DatagramSessionKey->Buffer
                );
        }


    } else {

        //
        // just make the temp key into the real one
        //

        RtlCopyMemory(
            Context->SessionKey,
            LocalSessionKey,
            MSV1_0_USER_SESSION_KEY_LENGTH
            );

    }
    return(SEC_E_OK);
}



NTSTATUS
SsprHandleFirstCall(
    IN LSA_SEC_HANDLE CredentialHandle,
    IN OUT PLSA_SEC_HANDLE ContextHandle,
    IN ULONG ContextReqFlags,
    IN ULONG InputTokenSize,
    IN PVOID InputToken,
    IN OUT PULONG OutputTokenSize,
    OUT PVOID *OutputToken,
    OUT PULONG ContextAttributes,
    OUT PTimeStamp ExpirationTime,
    OUT PUCHAR SessionKey,
    OUT PULONG NegotiateFlags
    )

/*++

Routine Description:

    Handle the First Call part of InitializeSecurityContext.

Arguments:

    All arguments same as for InitializeSecurityContext

Return Value:

    STATUS_SUCCESS -- All OK
    SEC_I_CONTINUE_NEEDED -- Caller should call again later

    SEC_E_INVALID_HANDLE -- Credential/Context Handle is invalid
    SEC_E_BUFFER_TOO_SMALL -- Buffer for output token isn't big enough
    SEC_E_INSUFFICIENT_MEMORY -- Not enough memory

--*/

{

    SspPrint(( SSP_API_MORE, "Entering SsprHandleFirstCall\n" ));
    NTSTATUS Status = STATUS_SUCCESS;
    PSSP_CONTEXT Context      = NULL;
    PSSP_CREDENTIAL Credential = NULL;

    PNEGOTIATE_MESSAGE NegotiateMessage = NULL;
    ULONG NegotiateMessageSize = 0;
    PCHAR Where = NULL;

    ULONG NegotiateFlagsKeyStrength;

    STRING NtLmLocalOemComputerNameString;
    STRING NtLmLocalOemPrimaryDomainNameString;

    INC_CLIENT_AUTH();

    //
    // Initialization
    //

    *ContextAttributes = 0;
    *NegotiateFlags = 0;

    RtlInitString( &NtLmLocalOemComputerNameString, NULL );
    RtlInitString( &NtLmLocalOemPrimaryDomainNameString, NULL );

    //
    // Get a pointer to the credential
    //

    Status = SspCredentialReferenceCredential(
                    CredentialHandle,
                    FALSE,
                    FALSE,
                    &Credential );

    if ( !NT_SUCCESS( Status ) )
    {
        SspPrint(( SSP_CRITICAL, "SsprHandleFirstCall: invalid credential handle.\n" ));
        goto Cleanup;
    }

    if ( (Credential->CredentialUseFlags & SECPKG_CRED_OUTBOUND) == 0 ) {
        Status = SEC_E_INVALID_CREDENTIAL_USE;
        SspPrint(( SSP_CRITICAL, "SsprHandleFirstCall: invalid credential use.\n" ));
        goto Cleanup;
    }

    //
    // Allocate a new context
    //

    Context = SspContextAllocateContext( );

    if ( Context == NULL) {
        Status = STATUS_NO_MEMORY;
        SspPrint(( SSP_CRITICAL, "SsprHandleFirstCall: SspContextAllocateContext returned NULL\n"));
        goto Cleanup;
    }

    //
    // Build a handle to the newly created context.
    //

    *ContextHandle = (LSA_SEC_HANDLE) Context;

    //
    // We don't support any options.
    //
    // Complain about those that require we do something.
    //

    if ( (ContextReqFlags & ISC_REQ_PROMPT_FOR_CREDS) != 0 ) {

        Status = SEC_E_INVALID_CONTEXT_REQ;
        SspPrint(( SSP_CRITICAL,
                   "SsprHandleFirstCall: invalid ContextReqFlags 0x%lx.\n",
                   ContextReqFlags ));
        goto Cleanup;
    }

    //
    // Capture the default credentials from the credential structure.
    //

    if ( Credential->DomainName.Buffer != NULL ) {
        Status = NtLmDuplicateUnicodeString(
                        &Context->DomainName,
                        &Credential->DomainName
                        );
        if (!NT_SUCCESS(Status)) {
            SspPrint(( SSP_CRITICAL,
            "SsprHandleFirstCall: NtLmDuplicateUnicodeString (DomainName) returned %d\n",Status));
            goto Cleanup;
        }
    }
    if ( Credential->UserName.Buffer != NULL ) {
        Status = NtLmDuplicateUnicodeString(
                        &Context->UserName,
                        &Credential->UserName
                        );
        if (!NT_SUCCESS(Status)) {
            SspPrint(( SSP_CRITICAL,
            "SsprHandleFirstCall: NtLmDuplicateUnicodeString (UserName) returned %d\n", Status ));
            goto Cleanup;
        }
    }

    Status = SspCredentialGetPassword(
                    Credential,
                    &Context->Password
                    );

    if (!NT_SUCCESS(Status)) {
        SspPrint(( SSP_CRITICAL,
        "SsprHandleFirstCall: SspCredentialGetPassword returned %d\n", Status ));
        goto Cleanup;
    }

    //
    // Compute the negotiate flags
    //


    //
    // Supported key strength(s)
    //

    NegotiateFlagsKeyStrength = NTLMSSP_NEGOTIATE_56;

    if( NtLmSecPkg.MachineState & SECPKG_STATE_STRONG_ENCRYPTION_PERMITTED )
    {
        NegotiateFlagsKeyStrength |= NTLMSSP_NEGOTIATE_128;
    }


    Context->NegotiateFlags = NTLMSSP_NEGOTIATE_UNICODE |
                              NTLMSSP_NEGOTIATE_OEM |
                              NTLMSSP_NEGOTIATE_NTLM |
                              ((NtLmGlobalLmProtocolSupported != 0)
                               ? NTLMSSP_NEGOTIATE_NTLM2 : 0 ) |
                              NTLMSSP_REQUEST_TARGET |
                              NTLMSSP_NEGOTIATE_ALWAYS_SIGN |
                              NegotiateFlagsKeyStrength;


    //
    // If the caller specified INTEGRITY, SEQUENCE_DETECT or REPLAY_DETECT,
    // that means they want to use the MakeSignature/VerifySignature
    // calls.  Add this to the negotiate.
    //

    if (ContextReqFlags &
        (ISC_REQ_INTEGRITY | ISC_REQ_SEQUENCE_DETECT | ISC_REQ_REPLAY_DETECT))
    {
        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_SIGN |
                                   NTLMSSP_NEGOTIATE_KEY_EXCH |
                                   NTLMSSP_NEGOTIATE_LM_KEY ;
    }


    if ((ContextReqFlags & ISC_REQ_INTEGRITY) != 0)
    {
        *ContextAttributes |= ISC_RET_INTEGRITY;
        Context->ContextFlags |= ISC_RET_INTEGRITY;
    }


    if ((ContextReqFlags & ISC_REQ_SEQUENCE_DETECT) != 0)
    {
        *ContextAttributes |= ISC_RET_SEQUENCE_DETECT;
        Context->ContextFlags |= ISC_RET_SEQUENCE_DETECT;
    }

    if ((ContextReqFlags & ISC_REQ_REPLAY_DETECT) != 0)
    {
        *ContextAttributes |= ISC_RET_REPLAY_DETECT;
        Context->ContextFlags |= ISC_RET_REPLAY_DETECT;
    }

    if ( (ContextReqFlags & ISC_REQ_NULL_SESSION ) != 0) {

        *ContextAttributes |= ISC_RET_NULL_SESSION;
        Context->ContextFlags |= ISC_RET_NULL_SESSION;
    }

    if ( (ContextReqFlags & ISC_REQ_CONNECTION ) != 0) {

        *ContextAttributes |= ISC_RET_CONNECTION;
        Context->ContextFlags |= ISC_RET_CONNECTION;
    }



    if ((ContextReqFlags & ISC_REQ_CONFIDENTIALITY) != 0) {
        if (NtLmGlobalEncryptionEnabled) {
            Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_SEAL |
                                       NTLMSSP_NEGOTIATE_LM_KEY |
                                       NTLMSSP_NEGOTIATE_KEY_EXCH ;

            *ContextAttributes |= ISC_RET_CONFIDENTIALITY;
            Context->ContextFlags |= ISC_RET_CONFIDENTIALITY;
        } else {
            Status = STATUS_NOT_SUPPORTED;
            SspPrint(( SSP_CRITICAL,
            "SsprHandleFirstCall: NtLmGlobalEncryptionEnabled is FALSE\n"));
            goto Cleanup;
        }
    }

    //
    // Check if the caller wants identify level
    //

    if ((ContextReqFlags & ISC_REQ_IDENTIFY)!= 0)  {
        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_IDENTIFY;
        *ContextAttributes |= ISC_RET_IDENTIFY;
        Context->ContextFlags |= ISC_RET_IDENTIFY;
    }

    IF_DEBUG( USE_OEM ) {
        Context->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_UNICODE;
    }

    if ( ((ContextReqFlags & ISC_REQ_MUTUAL_AUTH) != 0 ) &&
         (NtLmGlobalMutualAuthLevel < 2 ) ) {

        *ContextAttributes |= ISC_RET_MUTUAL_AUTH ;

        if ( NtLmGlobalMutualAuthLevel == 0 )
        {
            Context->ContextFlags |= ISC_RET_MUTUAL_AUTH ;
        }

    }
    //
    // For connection oriented security, we send a negotiate message to
    // the server.  For datagram, we get back the server's
    // capabilities in the challenge message.
    //

    if ((ContextReqFlags & ISC_REQ_DATAGRAM) == 0) {

        BOOLEAN CheckForLocal;

        if ( (Credential->DomainName.Buffer == NULL &&
              Credential->UserName.Buffer == NULL &&
              Credential->Password.Buffer == NULL ) &&
             (Credential->ClientTokenHandle != NULL) )
        {
            CheckForLocal = TRUE;
        } else {
            CheckForLocal = FALSE;
        }


        if( CheckForLocal ) {

            //
            // snap up a copy of the globals so we can just take the critsect once.
            // the old way took the critsect twice, once to read sizes, second time
            // to grab buffers - bad news if the global got bigger in between.
            //

            EnterCriticalSection (&NtLmGlobalCritSect);

            if( NtLmGlobalOemComputerNameString.Buffer == NULL ||
                NtLmGlobalOemPrimaryDomainNameString.Buffer == NULL ) {

                //
                // user has picked a computer name or domain name
                // that failed to convert to OEM.  disable the loopback
                // detection.
                // Sometime beyond Win2k, Negotiate package should have
                // a general, robust scheme for detecting loopback.
                //

                CheckForLocal = FALSE;

            } else {

                Status = NtLmDuplicateString(
                                        &NtLmLocalOemComputerNameString,
                                        &NtLmGlobalOemComputerNameString
                                        );

                if( NT_SUCCESS(Status) ) {
                    Status = NtLmDuplicateString(
                                            &NtLmLocalOemPrimaryDomainNameString,
                                            &NtLmGlobalOemPrimaryDomainNameString
                                            );
                }

            }

            LeaveCriticalSection (&NtLmGlobalCritSect);


            if (!NT_SUCCESS(Status)) {
                SspPrint(( SSP_CRITICAL,
                "SsprHandleFirstCall: NtLmDuplicateUnicodeString (GlobalOemComputerName or GlobalOemPrimaryDomainName) returned %d\n", Status ));
                goto Cleanup;
            }
        }


        //
        // Allocate a Negotiate message
        //

        NegotiateMessageSize = sizeof(*NegotiateMessage) +
                               NtLmLocalOemComputerNameString.Length +
                               NtLmLocalOemPrimaryDomainNameString.Length;

        if ((ContextReqFlags & ISC_REQ_ALLOCATE_MEMORY) == 0)
        {
            if ( NegotiateMessageSize > *OutputTokenSize ) {
                Status = SEC_E_BUFFER_TOO_SMALL;
                SspPrint(( SSP_CRITICAL,
                "SsprHandleFirstCall: OutputTokenSize is %d\n", *OutputTokenSize));
                goto Cleanup;
            }
        }

        NegotiateMessage = (PNEGOTIATE_MESSAGE)
                           NtLmAllocate( NegotiateMessageSize );

        if ( NegotiateMessage == NULL) {
            Status = STATUS_NO_MEMORY;
            SspPrint(( SSP_CRITICAL, "SsprHandleFirstCall: Error allocating NegotiateMessage.\n"));
            goto Cleanup;
        }

        //
        // If this is the first call,
        //  build a Negotiate message.
        //

        strcpy( (char *) NegotiateMessage->Signature, NTLMSSP_SIGNATURE );
        NegotiateMessage->MessageType = NtLmNegotiate;
        NegotiateMessage->NegotiateFlags = Context->NegotiateFlags;

        IF_DEBUG( REQUEST_TARGET ) {
            NegotiateMessage->NegotiateFlags |= NTLMSSP_REQUEST_TARGET;
        }

        //
        // Copy the DomainName and ComputerName into the negotiate message so
        // the other side can determine if this is a call from the local system.
        //
        // Pass the names in the OEM character set since the character set
        // hasn't been negotiated yet.
        //
        // Skip passing the workstation name if credentials were specified. This
        // ensures the other side doesn't fall into the case that this is the
        // local system.  We wan't to ensure the new credentials are
        // authenticated.
        //

        Where = (PCHAR)(NegotiateMessage+1);

        if ( CheckForLocal ) {

            SspContextCopyString( NegotiateMessage,
                                  &NegotiateMessage->OemWorkstationName,
                                  &NtLmLocalOemComputerNameString,
                                  &Where );

            NegotiateMessage->NegotiateFlags |=
                              NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED;


            //
            // OEM_DOMAIN_SUPPLIED used to always be supplied - but the
            // only case it is ever used is when NTLMSSP_NEGOTIATE_LOCAL_CALL
            // is set.
            //

            SspContextCopyString( NegotiateMessage,
                                  &NegotiateMessage->OemDomainName,
                                  &NtLmLocalOemPrimaryDomainNameString,
                                  &Where );

            NegotiateMessage->NegotiateFlags |=
                                  NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED;

        }

        if ((ContextReqFlags & ISC_REQ_ALLOCATE_MEMORY) == 0)
        {
            RtlCopyMemory( *OutputToken,
                       NegotiateMessage,
                       NegotiateMessageSize );

        }
        else
        {
            *OutputToken = NegotiateMessage;
            NegotiateMessage = NULL;
            *ContextAttributes |= ISC_RET_ALLOCATED_MEMORY;
        }

        *OutputTokenSize = NegotiateMessageSize;

    }

    //
    // Save a reference to the credential in the context.
    //

    Context->Credential = Credential;
    Credential = NULL;

    //
    // Check for a caller requesting datagram security.
    //

    if ((ContextReqFlags & ISC_REQ_DATAGRAM) != 0) {
        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_DATAGRAM;
        Context->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_NT_ONLY;
        Context->ContextFlags |= ISC_RET_DATAGRAM;
        *ContextAttributes |= ISC_RET_DATAGRAM;

        // If datagram security is required, then we don't send back a token

        *OutputTokenSize = 0;



        //
        // Generate a session key for this context if sign or seal was
        // requested.
        //

        if ((Context->NegotiateFlags & (NTLMSSP_NEGOTIATE_SIGN |
                                       NTLMSSP_NEGOTIATE_SEAL)) != 0) {

            Status = SspGenerateRandomBits(
                                Context->SessionKey,
                                MSV1_0_USER_SESSION_KEY_LENGTH
                                );

            if( !NT_SUCCESS( Status ) ) {
                SspPrint(( SSP_CRITICAL,
                "SsprHandleFirstCall: SspGenerateRandomBits failed\n"));
                goto Cleanup;
            }
        }
        RtlCopyMemory(
            SessionKey,
            Context->SessionKey,
            MSV1_0_USER_SESSION_KEY_LENGTH
            );


        //
        // Unless client wants to force its use,
        // Turn off strong crypt, because we can't negotiate it.
        //

        if (!NtLmGlobalDatagramUse128BitEncryption) {
            Context->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_128;
        }

        //
        // likewise for 56bit.  note that package init handles turning
        // off 56bit if 128bit is configured for datagram.
        //

        if(!NtLmGlobalDatagramUse56BitEncryption) {
            Context->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_56;
        }

        //
        //  Unless client wants to require NTLM2, can't use its
        //  message processing features because we start using
        //  MD5 sigs, full duplex mode, and datagram rekey before
        //  we know if the server supports NTLM2.
        //

        if (!NtLmGlobalRequireNtlm2) {
            Context->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_NTLM2;
        }

        //
        // done fiddling with the negotiate flags, output them.
        //

        *NegotiateFlags = Context->NegotiateFlags;

        //
        // send back the negotiate flags to control signing and sealing
        //

        *NegotiateFlags |= NTLMSSP_APP_SEQ;

    }

    if( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_KEY_EXCH )
    {
        Status = SspGenerateRandomBits(
                            Context->SessionKey,
                            MSV1_0_USER_SESSION_KEY_LENGTH
                            );

        if( !NT_SUCCESS( Status ) ) {
            SspPrint(( SSP_CRITICAL,
            "SsprHandleFirstCall: SspGenerateRandomBits failed\n"));
            goto Cleanup;
        }

        RtlCopyMemory(
            SessionKey,
            Context->SessionKey,
            MSV1_0_USER_SESSION_KEY_LENGTH
            );
    }

    //
    // Return output parameters to the caller.
    //

    *ExpirationTime = SspContextGetTimeStamp( Context, TRUE );

    Status = SEC_I_CONTINUE_NEEDED;
    Context->State = NegotiateSentState;


    SspPrint(( SSP_NEGOTIATE_FLAGS,
        "SsprHandleFirstCall: NegotiateFlags = %lx\n", Context->NegotiateFlags));


    //
    // Check that caller asked for minimum security required.
    //

    if (!SsprCheckMinimumSecurity(
                        Context->NegotiateFlags,
                        NtLmGlobalMinimumClientSecurity)) {

        Status = SEC_E_UNSUPPORTED_FUNCTION;

        SspPrint(( SSP_CRITICAL,
                  "SsprHandleFirstCall: "
                  "Caller didn't request minimum security requirements (caller=0x%lx wanted=0x%lx).\n",
                    Context->NegotiateFlags, NtLmGlobalMinimumClientSecurity ));
        goto Cleanup;
    }


    //
    // Free and locally used resources.
    //
Cleanup:

    if ( Context != NULL ) {

        //
        // If we failed,
        //  deallocate the context we allocated above.
        //
        // Delinking is a side effect of referencing, so do that.
        //

        if ( !NT_SUCCESS(Status) ) {

            PSSP_CONTEXT LocalContext;
            LocalContext = SspContextReferenceContext( *ContextHandle, TRUE );

            ASSERT( LocalContext != NULL );
            if ( LocalContext != NULL ) {
                SspContextDereferenceContext( LocalContext );
            }
        }

        // Always dereference it.

        SspContextDereferenceContext( Context );
    }

    if ( NegotiateMessage != NULL ) {
        (VOID) NtLmFree( NegotiateMessage );
    }

    if ( Credential != NULL ) {
        SspCredentialDereferenceCredential( Credential );
    }

    if ( NtLmLocalOemComputerNameString.Buffer != NULL ) {
        (VOID) NtLmFree( NtLmLocalOemComputerNameString.Buffer );
    }

    if ( NtLmLocalOemPrimaryDomainNameString.Buffer != NULL ) {
        (VOID) NtLmFree( NtLmLocalOemPrimaryDomainNameString.Buffer );
    }

    SspPrint(( SSP_API_MORE, "Leaving SsprHandleFirstCall: 0x%lx\n", Status ));
    return Status;

    UNREFERENCED_PARAMETER( InputToken );
    UNREFERENCED_PARAMETER( InputTokenSize );

}

NTSTATUS
SsprHandleNegotiateMessage(
    IN ULONG_PTR CredentialHandle,
    IN OUT PULONG_PTR ContextHandle,
    IN ULONG ContextReqFlags,
    IN ULONG InputTokenSize,
    IN PVOID InputToken,
    IN OUT PULONG OutputTokenSize,
    OUT PVOID *OutputToken,
    OUT PULONG ContextAttributes,
    OUT PTimeStamp ExpirationTime
    )

/*++

Routine Description:

    Handle the Negotiate message part of AcceptSecurityContext.

Arguments:

    All arguments same as for AcceptSecurityContext

Return Value:

    STATUS_SUCCESS - Message handled
    SEC_I_CONTINUE_NEEDED -- Caller should call again later

    SEC_E_INVALID_TOKEN -- Token improperly formatted
    SEC_E_INVALID_HANDLE -- Credential/Context Handle is invalid
    SEC_E_BUFFER_TOO_SMALL -- Buffer for output token isn't big enough
    SEC_E_INSUFFICIENT_MEMORY -- Not enough memory

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PSSP_CONTEXT Context = NULL;
    PSSP_CREDENTIAL Credential = NULL;
    STRING TargetName;
    ULONG TargetFlags = 0;

    PNEGOTIATE_MESSAGE NegotiateMessage = NULL;

    PCHALLENGE_MESSAGE ChallengeMessage = NULL;
    ULONG ChallengeMessageSize = 0;
    PCHAR Where = NULL;

    ULONG NegotiateFlagsKeyStrength;

    UNICODE_STRING NtLmLocalUnicodeTargetName;
    UNICODE_STRING TargetInfo;
    STRING NtLmLocalOemTargetName;
    STRING OemWorkstationName;
    STRING OemDomainName;

    SspPrint(( SSP_API_MORE, "Entering SsprNegotiateMessage\n" ));
    //
    // Initialization
    //

    *ContextAttributes = 0;

    RtlInitString( &TargetName, NULL );

    RtlInitUnicodeString( &NtLmLocalUnicodeTargetName, NULL );
    RtlInitString( &NtLmLocalOemTargetName, NULL );

    RtlInitUnicodeString( &TargetInfo, NULL );

    //
    // Get a pointer to the credential
    //

    Status = SspCredentialReferenceCredential(
                    CredentialHandle,
                    FALSE,
                    FALSE,
                    &Credential );

    if ( !NT_SUCCESS( Status ) ) {
        SspPrint(( SSP_CRITICAL,
                "SsprHandleNegotiateMessage: invalid credential handle.\n" ));
        goto Cleanup;
    }

    if ( (Credential->CredentialUseFlags & SECPKG_CRED_INBOUND) == 0 ) {
        Status = SEC_E_INVALID_CREDENTIAL_USE;
        SspPrint(( SSP_CRITICAL,
            "SsprHandleNegotiateMessage: invalid credential use.\n" ));
        goto Cleanup;
    }

    //
    // Allocate a new context
    //

    Context = SspContextAllocateContext( );

    if ( Context == NULL ) {
        Status = STATUS_NO_MEMORY;
        SspPrint(( SSP_CRITICAL,
            "SsprHandleNegotiateMessage: SspContextAllocateContext() returned NULL.\n" ));
        goto Cleanup;
    }

    //
    // Build a handle to the newly created context.
    //

    *ContextHandle = (ULONG_PTR) Context;


    if ( (ContextReqFlags & ASC_REQ_IDENTIFY) != 0 ) {

        *ContextAttributes |= ASC_RET_IDENTIFY;
        Context->ContextFlags |= ASC_RET_IDENTIFY;
    }

    if ( (ContextReqFlags & ASC_REQ_DATAGRAM) != 0 ) {

        *ContextAttributes |= ASC_RET_DATAGRAM;
        Context->ContextFlags |= ASC_RET_DATAGRAM;
    }

    if ( (ContextReqFlags & ASC_REQ_CONNECTION) != 0 ) {

        *ContextAttributes |= ASC_RET_CONNECTION;
        Context->ContextFlags |= ASC_RET_CONNECTION;
    }

    if ( (ContextReqFlags & ASC_REQ_INTEGRITY) != 0 ) {

        *ContextAttributes |= ASC_RET_INTEGRITY;
        Context->ContextFlags |= ASC_RET_INTEGRITY;
    }

    if ( (ContextReqFlags & ASC_REQ_REPLAY_DETECT) != 0){

        *ContextAttributes |= ASC_RET_REPLAY_DETECT;
        Context->ContextFlags |= ASC_RET_REPLAY_DETECT;
    }

    if ( (ContextReqFlags & ASC_REQ_SEQUENCE_DETECT ) != 0) {

        *ContextAttributes |= ASC_RET_SEQUENCE_DETECT;
        Context->ContextFlags |= ASC_RET_SEQUENCE_DETECT;
    }

    // Nothing to return, we might need this on the next server side call.
    if ( (ContextReqFlags & ASC_REQ_ALLOW_NULL_SESSION ) != 0) {

        Context->ContextFlags |= ASC_REQ_ALLOW_NULL_SESSION;
    }

    if ( (ContextReqFlags & ASC_REQ_ALLOW_NON_USER_LOGONS ) != 0) {

        *ContextAttributes |= ASC_RET_ALLOW_NON_USER_LOGONS;
        Context->ContextFlags |= ASC_RET_ALLOW_NON_USER_LOGONS;
    }

    if ( ContextReqFlags & ASC_REQ_CONFIDENTIALITY ) {

        if (NtLmGlobalEncryptionEnabled) {
            *ContextAttributes |= ASC_RET_CONFIDENTIALITY;
            Context->ContextFlags |= ASC_RET_CONFIDENTIALITY;
        } else {
            Status = STATUS_NOT_SUPPORTED;
            SspPrint(( SSP_CRITICAL,
                "SsprHandleNegotiateMessage: invalid ContextReqFlags 0x%lx\n", ContextReqFlags ));
            goto Cleanup;
        }
    }



    //
    // Supported key strength(s)
    //

    NegotiateFlagsKeyStrength = NTLMSSP_NEGOTIATE_56;

    if( NtLmSecPkg.MachineState & SECPKG_STATE_STRONG_ENCRYPTION_PERMITTED )
    {
        NegotiateFlagsKeyStrength |= NTLMSSP_NEGOTIATE_128;
    }


    //
    // Get the NegotiateMessage.  If we are re-establishing a datagram
    // context then there may not be one.
    //

    if ( InputTokenSize >= sizeof(OLD_NEGOTIATE_MESSAGE) ) {

        Status = SspContextGetMessage( InputToken,
                                          InputTokenSize,
                                          NtLmNegotiate,
                                          (PVOID *)&NegotiateMessage );

        if ( !NT_SUCCESS(Status) ) {
            SspPrint(( SSP_CRITICAL,
                      "SsprHandleNegotiateMessage: "
                      "NegotiateMessage GetMessage returns 0x%lx\n",
                      Status ));
            goto Cleanup;
        }

        //
        // Compute the TargetName to return in the ChallengeMessage.
        //

        if ( NegotiateMessage->NegotiateFlags & NTLMSSP_REQUEST_TARGET ||
             NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2 ) {

            EnterCriticalSection (&NtLmGlobalCritSect);
            if ( NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_UNICODE) {
                Status = NtLmDuplicateUnicodeString( &NtLmLocalUnicodeTargetName, &NtLmGlobalUnicodeTargetName );
                TargetName = *((PSTRING)&NtLmLocalUnicodeTargetName);
            } else {
                Status = NtLmDuplicateString( &NtLmLocalOemTargetName, &NtLmGlobalOemTargetName );
                TargetName = NtLmLocalOemTargetName;
            }

            //
            // if client is NTLM2-aware, send it target info AV pairs
            //

            if(NT_SUCCESS(Status))
            {
                Status = NtLmDuplicateUnicodeString( &TargetInfo, &NtLmGlobalNtLm3TargetInfo );
            }

            TargetFlags = NtLmGlobalTargetFlags;
            LeaveCriticalSection (&NtLmGlobalCritSect);

            TargetFlags |= NTLMSSP_REQUEST_TARGET | NTLMSSP_NEGOTIATE_TARGET_INFO;

            if(!NT_SUCCESS(Status)) {
                SspPrint(( SSP_CRITICAL,
                          "SsprHandleNegotiateMessage: "
                          "failed to duplicate UnicodeTaretName or OemTargetName error 0x%lx\n",
                          Status ));

                goto Cleanup;
            }

        } else {
            TargetFlags = 0;
        }


        //
        // Allocate a Challenge message
        //

        ChallengeMessageSize = sizeof(*ChallengeMessage) +
                                TargetName.Length +
                                TargetInfo.Length ;

        if ((ContextReqFlags & ASC_REQ_ALLOCATE_MEMORY) == 0)
        {
            if ( ChallengeMessageSize > *OutputTokenSize ) {
                SspPrint(( SSP_CRITICAL,
                    "SsprHandleNegotiateMessage: invalid ChallengeMessageSize\n"));
                Status = SEC_E_BUFFER_TOO_SMALL;
                goto Cleanup;
            }
        }

        ChallengeMessage = (PCHALLENGE_MESSAGE)
                           NtLmAllocate( ChallengeMessageSize );

        if ( ChallengeMessage == NULL ) {
            SspPrint(( SSP_CRITICAL,
                "SsprHandleNegotiateMessage: Error allocating ChallengeMessage.\n" ));
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        ChallengeMessage->NegotiateFlags = 0;

        //
        // Check that both sides can use the same authentication model.  For
        // compatibility with beta 1 and 2 (builds 612 and 683), no requested
        // authentication type is assumed to be NTLM.  If NetWare is explicitly
        // asked for, it is assumed that NTLM would have been also, so if it
        // wasn't, return an error.
        //

        if ( (NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NETWARE) &&
             ((NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM) == 0) &&
             ((NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2) == 0)
            ) {
            Status = STATUS_NOT_SUPPORTED;
            SspPrint(( SSP_CRITICAL,
                      "SsprHandleNegotiateMessage: "
                      "NegotiateMessage asked for Netware only.\n" ));
            goto Cleanup;
        } else {
            ChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_NTLM;
        }




        //
        // if client can do NTLM2, nuke LM_KEY
        //

        if (NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2) {
            NegotiateMessage->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_LM_KEY;

            ChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_NTLM2;
        } else if (NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_LM_KEY) {
            ChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_LM_KEY;
        }


        //
        // If the client wants to always sign messages, so be it.
        //

        if (NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_ALWAYS_SIGN ) {
            ChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_ALWAYS_SIGN;

            // BUGBUG: check when this is set, and update ContextAttributes accordingly
        }

        //
        // If the caller wants identify level, so be it.
        //

        if (NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_IDENTIFY ) {
            ChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_IDENTIFY;

            *ContextAttributes |= ASC_RET_IDENTIFY;
            Context->ContextFlags |= ASC_RET_IDENTIFY;

        }


        //
        // Determine if the caller wants OEM or UNICODE
        //
        // Prefer UNICODE if caller allows both.
        //

        if ( NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_UNICODE ) {
            ChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_UNICODE;
        } else if ( NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_OEM ){
            ChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_OEM;
        } else {
            Status = SEC_E_INVALID_TOKEN;
            SspPrint(( SSP_CRITICAL,
                      "SsprHandleNegotiateMessage: "
                      "NegotiateMessage bad NegotiateFlags 0x%lx\n",
                      NegotiateMessage->NegotiateFlags ));
            goto Cleanup;
        }

        //
        // Client wants Sign capability, OK.
        //
        if (NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_SIGN) {
            ChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_SIGN;

            *ContextAttributes |= (ASC_RET_SEQUENCE_DETECT | ASC_RET_REPLAY_DETECT);
            Context->ContextFlags |= (ASC_RET_SEQUENCE_DETECT | ASC_RET_REPLAY_DETECT);

        }

        //
        // Client wants Seal, OK.
        //

        if (NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_SEAL)
        {
            ChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_SEAL;

            *ContextAttributes |= ASC_RET_CONFIDENTIALITY;
            Context->ContextFlags |= ASC_RET_CONFIDENTIALITY;
        }

        if(NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_KEY_EXCH)
        {
            ChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_KEY_EXCH;

        }

        if( (NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_56) &&
            (NegotiateFlagsKeyStrength & NTLMSSP_NEGOTIATE_56) )
        {
            ChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_56;
        }

        if( (NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_128) &&
            (NegotiateFlagsKeyStrength & NTLMSSP_NEGOTIATE_128) )
        {
            ChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_128;
        }


        //
        // If the client supplied the Domain Name and User Name,
        //  and did not request datagram, see if the client is running
        //  on this local machine.
        //

        if ( ( (NegotiateMessage->NegotiateFlags &
                NTLMSSP_NEGOTIATE_DATAGRAM) == 0) &&
             ( (NegotiateMessage->NegotiateFlags &
               (NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED|
                NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED)) ==
               (NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED|
                NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED) ) ) {

            //
            // The client must pass the new negotiate message if they pass
            // these flags
            //

            if (InputTokenSize < sizeof(NEGOTIATE_MESSAGE)) {
                Status = SEC_E_INVALID_TOKEN;
                SspPrint(( SSP_CRITICAL,
                    "SsprHandleNegotiateMessage: invalid InputTokenSize.\n" ));
                goto Cleanup;
            }

            //
            // Convert the names to absolute references so we
            // can compare them
            //

            if ( !SspConvertRelativeToAbsolute(
                NegotiateMessage,
                InputTokenSize,
                &NegotiateMessage->OemDomainName,
                &OemDomainName,
                FALSE,     // No special alignment
                FALSE ) ) { // NULL not OK

                Status = SEC_E_INVALID_TOKEN;
                SspPrint(( SSP_CRITICAL,
                    "SsprHandleNegotiateMessage: Error from SspConvertRelativeToAbsolute.\n" ));
                goto Cleanup;
            }

            if ( !SspConvertRelativeToAbsolute(
                NegotiateMessage,
                InputTokenSize,
                &NegotiateMessage->OemWorkstationName,
                &OemWorkstationName,
                FALSE,     // No special alignment
                FALSE ) ) { // NULL not OK

                Status = SEC_E_INVALID_TOKEN;
                SspPrint(( SSP_CRITICAL,
                    "SsprHandleNegotiateMessage: Error from SspConvertRelativeToAbsolute.\n" ));
                goto Cleanup;
            }

            //
            // If both strings match,
            // this is a local call.
            // The strings have already been uppercased.
            //

            EnterCriticalSection (&NtLmGlobalCritSect);
            if ( RtlEqualString( &OemWorkstationName,
                                 &NtLmGlobalOemComputerNameString,
                                 FALSE ) &&
                RtlEqualString( &OemDomainName,
                                 &NtLmGlobalOemPrimaryDomainNameString,
                                 FALSE ) ) {

                ChallengeMessage->NegotiateFlags |=
                    NTLMSSP_NEGOTIATE_LOCAL_CALL;
                SspPrint(( SSP_MISC,
                    "SsprHandleNegotiateMessage: Local Call.\n" ));

                ChallengeMessage->ServerContextHandle = (ULONG64)*ContextHandle;

            }
            LeaveCriticalSection (&NtLmGlobalCritSect);
        }

        //
        // Check if datagram is being negotiated
        //

        if ( (NegotiateMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_DATAGRAM) ==
                NTLMSSP_NEGOTIATE_DATAGRAM) {
            ChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_DATAGRAM;
        }

    } else {

        //
        // No negotiate message.  We need to check if the caller is asking
        // for datagram.
        //

        if ((ContextReqFlags & ASC_REQ_DATAGRAM) == 0 ) {
            SspPrint(( SSP_CRITICAL,
                      "SsprHandleNegotiateMessage: "
                      "NegotiateMessage size wrong %ld\n",
                      InputTokenSize ));
            Status = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }

        //
        // Allocate a Challenge message
        //

        //
        // always send target info -- new for NTLM3!
        //
        TargetFlags = NTLMSSP_NEGOTIATE_TARGET_INFO;

        ChallengeMessageSize = sizeof(*ChallengeMessage) + TargetInfo.Length;

        if ((ContextReqFlags & ASC_REQ_ALLOCATE_MEMORY) == 0)
        {
            if ( ChallengeMessageSize > *OutputTokenSize ) {
                Status = SEC_E_BUFFER_TOO_SMALL;
                SspPrint(( SSP_CRITICAL,
                    "SsprHandleNegotiateMessage: invalid ChallengeMessageSize.\n" ));
                goto Cleanup;
            }
        }

        ChallengeMessage = (PCHALLENGE_MESSAGE)
                           NtLmAllocate(ChallengeMessageSize );

        if ( ChallengeMessage == NULL ) {
            Status = STATUS_NO_MEMORY;
            SspPrint(( SSP_CRITICAL,
                "SsprHandleNegotiateMessage: Error allocating ChallengeMessage.\n" ));
            goto Cleanup;
        }

        //
        // Record in the context that we are doing datagram.  We will tell
        // the client everything we can negotiate and let it decide what
        // to negotiate.
        //

        ChallengeMessage->NegotiateFlags = NTLMSSP_NEGOTIATE_DATAGRAM |
                                            NTLMSSP_NEGOTIATE_UNICODE |
                                            NTLMSSP_NEGOTIATE_OEM |
                                            NTLMSSP_NEGOTIATE_SIGN |
                                            NTLMSSP_NEGOTIATE_LM_KEY |
                                            NTLMSSP_NEGOTIATE_NTLM |
                                            NTLMSSP_NEGOTIATE_ALWAYS_SIGN |
                                            NTLMSSP_NEGOTIATE_IDENTIFY |
                                            NTLMSSP_NEGOTIATE_NTLM2 |
                                            NTLMSSP_NEGOTIATE_KEY_EXCH |
                                            NegotiateFlagsKeyStrength
                                            ;

        if (NtLmGlobalEncryptionEnabled) {
            ChallengeMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_SEAL;
        }

    }

    //
    // Build the Challenge Message
    //

    strcpy( (char *) ChallengeMessage->Signature, NTLMSSP_SIGNATURE );
    ChallengeMessage->MessageType = NtLmChallenge;
    Status = SspGenerateRandomBits( (UCHAR*)ChallengeMessage->Challenge,
                                    MSV1_0_CHALLENGE_LENGTH );

    if( !NT_SUCCESS( Status ) ) {
        SspPrint(( SSP_CRITICAL,
        "SsprHandleNegotiateMessage: SspGenerateRandomBits failed\n"));
        goto Cleanup;
    }

    Where = (PCHAR)(ChallengeMessage+1);

    SspContextCopyString( ChallengeMessage,
                          &ChallengeMessage->TargetName,
                          &TargetName,
                          &Where );

    SspContextCopyString( ChallengeMessage,
                          &ChallengeMessage->TargetInfo,
                          (PSTRING)&TargetInfo,
                          &Where );

    ChallengeMessage->NegotiateFlags |= TargetFlags;

    //
    // Save the Challenge and Negotiate Flags in the Context so it
    // is available when the authenticate message comes in.
    //

    RtlCopyMemory( Context->Challenge,
                   ChallengeMessage->Challenge,
                   sizeof( Context->Challenge ) );

    Context->NegotiateFlags = ChallengeMessage->NegotiateFlags;

    //
    // Check that client asked for minimum security required.
    //

    if (!SsprCheckMinimumSecurity(
                    Context->NegotiateFlags,
                    NtLmGlobalMinimumServerSecurity)) {

        Status = SEC_E_UNSUPPORTED_FUNCTION;
        SspPrint(( SSP_CRITICAL,
                  "SsprHandleNegotiateMessage: "
                  "NegotiateMessage didn't support minimum security requirements. (caller=0x%lx wanted=0x%lx\n",
                   Context->NegotiateFlags, NtLmGlobalMinimumServerSecurity ));
        goto Cleanup;
    }


    if ((ContextReqFlags & ASC_REQ_ALLOCATE_MEMORY) == 0)
    {
        RtlCopyMemory( *OutputToken,
                   ChallengeMessage,
                   ChallengeMessageSize );

    }
    else
    {
        *OutputToken = ChallengeMessage;
        ChallengeMessage = NULL;
        *ContextAttributes |= ASC_RET_ALLOCATED_MEMORY;
    }

    *OutputTokenSize = ChallengeMessageSize;

    //
    // Return output parameters to the caller.
    //

    *ExpirationTime = SspContextGetTimeStamp( Context, TRUE );
    Context->State = ChallengeSentState;

    Status = SEC_I_CONTINUE_NEEDED;

    //
    // Free and locally used resources.
    //

Cleanup:

    if ( Context != NULL ) {

        //
        // If we failed, deallocate the context we allocated above.
        // Delinking is a side effect of referencing, so do that.
        //

        if ( !NT_SUCCESS(Status) ) {
            PSSP_CONTEXT LocalContext;
            LocalContext = SspContextReferenceContext( *ContextHandle,
                                                       TRUE );

            ASSERT( LocalContext != NULL );
            if ( LocalContext != NULL ) {
                SspContextDereferenceContext( LocalContext );
            }
        }

        // Always dereference it.

        SspContextDereferenceContext( Context );
    }

    if ( NegotiateMessage != NULL ) {
        (VOID) NtLmFree( NegotiateMessage );
    }

    if ( ChallengeMessage != NULL ) {
        (VOID) NtLmFree( ChallengeMessage );
    }

    if ( Credential != NULL ) {
        SspCredentialDereferenceCredential( Credential );
    }

    if ( NtLmLocalUnicodeTargetName.Buffer != NULL ) {
        (VOID) NtLmFree( NtLmLocalUnicodeTargetName.Buffer );
    }

    if ( NtLmLocalOemTargetName.Buffer != NULL ) {
        (VOID) NtLmFree( NtLmLocalOemTargetName.Buffer );
    }

    if (TargetInfo.Buffer != NULL ) {
        (VOID) NtLmFree( TargetInfo.Buffer );
    }


    SspPrint(( SSP_API_MORE, "Leaving SsprHandleNegotiateMessage: 0x%lx\n", Status ));
    return Status;

}

NTSTATUS
SsprHandleChallengeMessage(
    IN LSA_SEC_HANDLE CredentialHandle,
    IN OUT PLSA_SEC_HANDLE ContextHandle,
    IN HANDLE ClientTokenHandle,
    IN PLUID LogonId,
    IN ULONG ContextReqFlags,
    IN ULONG InputTokenSize,
    IN PVOID InputToken,
    IN ULONG SecondInputTokenSize,
    IN PVOID SecondInputToken,
    IN OUT PULONG OutputTokenSize,
    OUT PVOID *OutputToken,
    IN OUT PULONG SecondOutputTokenSize,
    OUT PVOID *SecondOutputToken,
    OUT PULONG ContextAttributes,
    OUT PTimeStamp ExpirationTime,
    OUT PUCHAR SessionKey,
    OUT PULONG NegotiateFlags
    )

/*++

Routine Description:

    Handle the Challenge message part of InitializeSecurityContext.

Arguments:

    ClientTokenHandle - Optionally passes in a handle to an impersonation
        token of the client.  This impersonation token will be passed directly
        to the server if the server is running on the same machine.  In that
        case, this routine will NULL the ClientTokenHandle letting the caller
        know that it need not close the handle.  The server will close the
        handle when it's done with it.

    LogonId -- LogonId of the calling process.

    DomainName,UserName,Password - Passed in credentials to be used for this
        context.

    DomainNameSize,userNameSize,PasswordSize - length in characters of the
        credentials to be used for this context.

    SessionKey - Session key to use for this context

    NegotiateFlags - Flags negotiated for this context

    All other arguments same as for InitializeSecurityContext

Return Value:

    STATUS_SUCCESS - Message handled
    SEC_I_CONTINUE_NEEDED -- Caller should call again later

    SEC_E_INVALID_TOKEN -- Token improperly formatted
    SEC_E_INVALID_HANDLE -- Credential/Context Handle is invalid
    SEC_E_BUFFER_TOO_SMALL -- Buffer for output token isn't big enough
    SEC_E_NO_CREDENTIALS -- There are no credentials for this client
    SEC_E_INSUFFICIENT_MEMORY -- Not enough memory

--*/

{
    SECURITY_STATUS SecStatus = STATUS_SUCCESS;
    PSSP_CONTEXT Context = NULL;
    PCHALLENGE_MESSAGE ChallengeMessage = NULL;
    PNTLM_CHALLENGE_MESSAGE NtLmChallengeMessage = NULL;
    PAUTHENTICATE_MESSAGE AuthenticateMessage = NULL;
    PNTLM_INITIALIZE_RESPONSE NtLmInitializeResponse = NULL;
    PMSV1_0_GETCHALLENRESP_RESPONSE ChallengeResponseMessage = NULL;
    STRING UserName;
    STRING DomainName;
    STRING Workstation;
    STRING LmChallengeResponse;
    STRING NtChallengeResponse;
    STRING DatagramSessionKey;
    BOOLEAN DoUnicode = TRUE;

    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS ProtocolStatus;

    // BUGBUG -- server can overflow this buffer!
    LPBYTE GetChallengeResponseBuffer[
        sizeof(MSV1_0_GETCHALLENRESP_REQUEST) +
        TARGET_INFO_LEN +
        (PWLEN+UNLEN+DNLEN+CNLEN+DNLEN+5) * sizeof(WCHAR) ];

    PMSV1_0_GETCHALLENRESP_REQUEST GetChallengeResponse;
    ULONG GetChallengeResponseSize;

    UNICODE_STRING RevealedPassword;

    ULONG ChallengeResponseSize;
    ULONG AuthenticateMessageSize;
    PCHAR Where;
    UCHAR LocalSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];
    UCHAR DatagramKey[MSV1_0_USER_SESSION_KEY_LENGTH];
    PLUID ClientLogonId = NULL;
    BOOLEAN UseSuppliedCreds = FALSE;
    PSSP_CREDENTIAL Credential = NULL;
    BOOLEAN fCallFromRedir = FALSE;

    SspPrint((SSP_API_MORE, "Entering SsprHandleChallengeMessage\n"));

    INC_SERVER_AUTH();

    //
    // Initialization
    //

    *ContextAttributes = 0;
    *NegotiateFlags = 0;
    UserName.Buffer = NULL;
    DomainName.Buffer = NULL;
    RevealedPassword.Buffer = NULL;
    GetChallengeResponse =
        (PMSV1_0_GETCHALLENRESP_REQUEST) GetChallengeResponseBuffer;

    if (*ContextHandle == NULL)
    {
        // This is possibly an old style redir call (for 4.0 and before)
        // so, alloc the context and replace the creds if new ones exists

        fCallFromRedir = TRUE;

        SspPrint((SSP_API_MORE, "SsprHandleChallengeMessage: *ContextHandle is NULL (old-style RDR)\n"));

        if ((ContextReqFlags & ISC_REQ_USE_SUPPLIED_CREDS) != 0)
        {
            UseSuppliedCreds = TRUE;
        }

        // This is a  superflous check since we alloc only if the caller
        // has asked us too. This is to make sure that the redir always asks us to alloc

        if (!(ContextReqFlags & ISC_REQ_ALLOCATE_MEMORY))
        {
            SecStatus = STATUS_NOT_SUPPORTED;
            goto Cleanup;
        }

        SecStatus = SspCredentialReferenceCredential(
                                          CredentialHandle,
                                          FALSE,
                                          FALSE,
                                          &Credential );

        if ( !NT_SUCCESS( SecStatus ) )
        {
            SspPrint(( SSP_CRITICAL,
                "SsprHandleChallengeMessage: SspCredentialReferenceCredential returns %x.\n", SecStatus ));
            goto Cleanup;
        }

        //
        // Allocate a new context
        //

        Context = SspContextAllocateContext();

        if (Context == NULL)
        {
            SspPrint(( SSP_CRITICAL,
                "SsprHandleChallengeMessage: SspContextAllocateContext returns NULL.\n" ));
            SecStatus = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        // We've just added a context, we don't nornally add and then
        // reference it.

        SspContextDereferenceContext( Context );

        *ContextHandle = (LSA_SEC_HANDLE) Context;

        //
        // Capture the default credentials from the credential structure.
        //

        if ( Credential->DomainName.Buffer != NULL ) {
            Status = NtLmDuplicateUnicodeString(
                        &Context->DomainName,
                        &Credential->DomainName
                        );
            if (!NT_SUCCESS(Status)) {
                SspPrint(( SSP_CRITICAL,
                "SsprHandleChallengeMessage: NtLmDuplicateUnicodeString (DomainName) returned %d\n",Status));
                SecStatus = SspNtStatusToSecStatus( Status,
                                                    SEC_E_INSUFFICIENT_MEMORY);
                goto Cleanup;
            }
        }
        if ( Credential->UserName.Buffer != NULL ) {
            Status = NtLmDuplicateUnicodeString(
                        &Context->UserName,
                        &Credential->UserName
                        );
            if (!NT_SUCCESS(Status)) {
                SspPrint(( SSP_CRITICAL,
                "SsprHandleChallengeMessage: NtLmDuplicateUnicodeString (UserName) returned %d\n", Status ));
                SecStatus = SspNtStatusToSecStatus( Status,
                                                    SEC_E_INSUFFICIENT_MEMORY);
                goto Cleanup;
            }
        }

        SecStatus = SspCredentialGetPassword(
                    Credential,
                    &Context->Password
                    );

        if (!NT_SUCCESS(SecStatus)) {
            SspPrint(( SSP_CRITICAL,
            "SsprHandleChallengeMessage: SspCredentialGetPassword returned %d\n", SecStatus ));
            goto Cleanup;
        }

        // Assign the Credential

        Context->Credential = Credential;
        Credential = NULL;

        //
        // fake it
        //

        Context->NegotiateFlags = NTLMSSP_NEGOTIATE_UNICODE |
                                  NTLMSSP_NEGOTIATE_OEM |
                                  NTLMSSP_REQUEST_TARGET |
                                  NTLMSSP_REQUEST_INIT_RESPONSE |
                                  NTLMSSP_TARGET_TYPE_SERVER ;


        *ExpirationTime = SspContextGetTimeStamp(Context, TRUE);

        Context->State = NegotiateSentState;

        // If creds are passed in by the RDR, then replace the ones in the context
        if (UseSuppliedCreds)
        {
            if (SecondInputTokenSize < sizeof(NTLM_CHALLENGE_MESSAGE))
            {
                SspPrint(( SSP_CRITICAL,
                "SsprHandleChallengeMessage: Invalid SecondInputTokensize.\n" ));
                SecStatus = SEC_E_INVALID_TOKEN;
                goto Cleanup;
            }

            NtLmChallengeMessage = (PNTLM_CHALLENGE_MESSAGE) NtLmAllocate(SecondInputTokenSize);
            if (NtLmChallengeMessage == NULL)
            {
                SspPrint(( SSP_CRITICAL,
                "SsprHandleChallengeMessage: Error while allocating NtLmChallengeMessage\n" ));
                SecStatus = STATUS_NO_MEMORY;
                goto Cleanup;
            }

            RtlCopyMemory(NtLmChallengeMessage,
                          SecondInputToken,
                          SecondInputTokenSize);

            if ((NtLmChallengeMessage->Password.Length == 0) &&
                (NtLmChallengeMessage->UserName.Length == 0) &&
                (NtLmChallengeMessage->DomainName.Length == 0))
            {
                // This could only be a null session request

                if (Context->Password.Buffer != NULL)
                {
                    // free it first
                    NtLmFree (Context->Password.Buffer);
                }

                Context->Password.Buffer =  (LPWSTR) NtLmAllocate(sizeof(WCHAR));
                if (Context->Password.Buffer == NULL)
                {
                    SspPrint(( SSP_CRITICAL,
                        "SsprHandleChallengeMessage: NtLmAllocate(Password) returns NULL.\n"));
                    SecStatus = SEC_E_INSUFFICIENT_MEMORY;
                    goto Cleanup;
                }
                Context->Password.Length = 0;
                Context->Password.MaximumLength = sizeof(WCHAR);
                *(Context->Password.Buffer) = L'\0';
                SspHidePassword(&Context->Password);

                if (Context->UserName.Buffer != NULL)
                {
                    // free it first
                    NtLmFree (Context->UserName.Buffer);
                }

                Context->UserName.Buffer =  (LPWSTR) NtLmAllocate(sizeof(WCHAR));
                if (Context->UserName.Buffer == NULL)
                {
                    SspPrint(( SSP_CRITICAL,
                        "SsprHandleChallengeMessage: NtLmAllocate(UserName) returns NULL.\n"));
                    SecStatus = SEC_E_INSUFFICIENT_MEMORY;
                    goto Cleanup;
                }
                Context->UserName.Length = 0;
                Context->UserName.MaximumLength = sizeof(WCHAR);
                *(Context->UserName.Buffer) = L'\0';

                if (Context->DomainName.Buffer != NULL)
                {
                    // free it first
                    NtLmFree (Context->DomainName.Buffer);
                }

                Context->DomainName.Buffer =  (LPWSTR) NtLmAllocate(sizeof(WCHAR));
                if (Context->DomainName.Buffer == NULL)
                {
                    SspPrint(( SSP_CRITICAL,
                        "SsprHandleChallengeMessage: NtLmAllocate(DomainName) returns NULL.\n"));
                    SecStatus = SEC_E_INSUFFICIENT_MEMORY;
                    goto Cleanup;
                }
                Context->DomainName.Length = 0;
                Context->DomainName.MaximumLength = sizeof(WCHAR);
                *(Context->DomainName.Buffer) = L'\0';
            }
            else
            {
                ULONG_PTR BufferTail = (ULONG_PTR)NtLmChallengeMessage + SecondInputTokenSize;
                UNICODE_STRING AbsoluteString;

                if (NtLmChallengeMessage->Password.Buffer != 0)
                {
                    AbsoluteString.Buffer = (LPWSTR)((PUCHAR)NtLmChallengeMessage + NtLmChallengeMessage->Password.Buffer);

                    //
                    // verify buffer not out of range.
                    //

                    if( ( (ULONG_PTR)AbsoluteString.Buffer > BufferTail ) ||
                        ( (ULONG_PTR)((PUCHAR)AbsoluteString.Buffer + NtLmChallengeMessage->Password.Length) > BufferTail ) ||
                        ( (ULONG_PTR)AbsoluteString.Buffer < (ULONG_PTR)NtLmChallengeMessage )
                        )
                    {
                        SecStatus = SEC_E_NO_CREDENTIALS;
                        SspPrint(( SSP_CRITICAL,
                        "SsprHandleChallengeMessage: Buffer overflow (Password).\n" ));
                        goto Cleanup;

                    }

                    if (Context->Password.Buffer != NULL)
                    {
                        // free it first
                        NtLmFree (Context->Password.Buffer);
                    }

                    AbsoluteString.Length = AbsoluteString.MaximumLength = NtLmChallengeMessage->Password.Length;
                    SecStatus = NtLmDuplicateUnicodeString(&Context->Password,
                                                       &AbsoluteString);

                    if (!NT_SUCCESS(SecStatus))
                    {
                        SspPrint(( SSP_CRITICAL,
                        "SsprHandleChallengeMessage: NtLmDuplicateUnicodeString(Password) returns 0x%lx.\n",SecStatus ));
                        goto Cleanup;
                    }

                    SspHidePassword(&Context->Password);
                }

                if (NtLmChallengeMessage->UserName.Length != 0)
                {
                    AbsoluteString.Buffer = (LPWSTR)((PUCHAR)NtLmChallengeMessage + NtLmChallengeMessage->UserName.Buffer);

                    //
                    // verify buffer not out of range.
                    //

                    if( ( (ULONG_PTR)AbsoluteString.Buffer > BufferTail ) ||
                        ( (ULONG_PTR)((PUCHAR)AbsoluteString.Buffer + NtLmChallengeMessage->UserName.Length) > BufferTail ) ||
                        ( (ULONG_PTR)AbsoluteString.Buffer < (ULONG_PTR)NtLmChallengeMessage )
                        )
                    {
                        SecStatus = SEC_E_NO_CREDENTIALS;
                        SspPrint(( SSP_CRITICAL,
                        "SsprHandleChallengeMessage: Buffer overflow (UserName).\n" ));
                        goto Cleanup;

                    }

                    if (Context->UserName.Buffer != NULL)
                    {
                        // free it first
                        NtLmFree (Context->UserName.Buffer);
                    }

                    AbsoluteString.Length = AbsoluteString.MaximumLength = NtLmChallengeMessage->UserName.Length;
                    SecStatus = NtLmDuplicateUnicodeString(&Context->UserName,
                                                       &AbsoluteString);
                    if (!NT_SUCCESS(SecStatus))
                    {
                        SspPrint(( SSP_CRITICAL,
                        "SsprHandleChallengeMessage: NtLmDuplicateUnicodeString(UserName) returns 0x%lx.\n",SecStatus ));
                        goto Cleanup;
                    }
                }

                if (NtLmChallengeMessage->DomainName.Length != 0)
                {
                    AbsoluteString.Buffer = (LPWSTR)((PUCHAR)NtLmChallengeMessage + NtLmChallengeMessage->DomainName.Buffer);

                    //
                    // verify buffer not out of range.
                    //

                    if( ( (ULONG_PTR)AbsoluteString.Buffer > BufferTail ) ||
                        ( (ULONG_PTR)((PUCHAR)AbsoluteString.Buffer + NtLmChallengeMessage->DomainName.Length) > BufferTail ) ||
                        ( (ULONG_PTR)AbsoluteString.Buffer < (ULONG_PTR)NtLmChallengeMessage )
                        )
                    {
                        SecStatus = SEC_E_NO_CREDENTIALS;
                        SspPrint(( SSP_CRITICAL,
                        "SsprHandleChallengeMessage: Buffer overflow (DomainName).\n" ));
                        goto Cleanup;

                    }

                    if (Context->DomainName.Buffer != NULL)
                    {
                        // free it first
                        NtLmFree (Context->DomainName.Buffer);
                    }

                    AbsoluteString.Length = AbsoluteString.MaximumLength = NtLmChallengeMessage->DomainName.Length;
                    SecStatus = NtLmDuplicateUnicodeString(&Context->DomainName,
                                                       &AbsoluteString);
                    if (!NT_SUCCESS(SecStatus))
                    {
                        SspPrint(( SSP_CRITICAL,
                        "SsprHandleChallengeMessage: NtLmDuplicateUnicodeString(DomainName) returns 0x%lx.\n",SecStatus ));
                        goto Cleanup;
                    }
                }
            }


            if (NtLmChallengeMessage)
            {
                NtLmFree (NtLmChallengeMessage);
            }

        } // end of special casing if credentials are supplied in the first init call

    } // end of special casing for the old style redir


    //
    // Find the currently existing context.
    //

    Context = SspContextReferenceContext( *ContextHandle, FALSE );

    if ( Context == NULL ) {
        SspPrint(( SSP_CRITICAL,
                "SsprHandleChallengeMessage: invalid context handle.\n" ));
        SecStatus = STATUS_INVALID_HANDLE;
        goto Cleanup;
    }

    else {

        //
        // bug 321061: passing Accept handle to Init causes AV.
        //

        if( Context->Credential == NULL ) {
            SspPrint(( SSP_CRITICAL,
                    "SsprHandleChallengeMessage: invalid context handle, missing credential.\n" ));

            ASSERT( Context->Credential );
            SecStatus = STATUS_INVALID_HANDLE;
            goto Cleanup;
        }

        //
        // If this is not reauthentication (or is datagram reauthentication)
        // pull the context out of the associated credential.
        //

        if ((Context->State != AuthenticateSentState) ||
           (Context->NegotiateFlags & NTLMSSP_NEGOTIATE_DATAGRAM) != 0) {
            ClientLogonId = &Context->Credential->LogonId;
            ClientTokenHandle = Context->Credential->ClientTokenHandle;
        }
    }

    //
    // If we have already sent the authenticate message, then this must be
    // RPC calling Initialize a third time to re-authenticate a connection.
    // This happens when a new interface is called over an existing
    // connection.  What we do here is build a NULL authenticate message
    // that the server will recognize and also ignore.
    //

    //
    // That being said, if we are doing datagram style authentication then
    // the story is different.  The server may have dropped this security
    // context and then the client sent another packet over.  The server
    // will then be trying to restore the context, so we need to build
    // another authenticate message.
    //

    if ( Context->State == AuthenticateSentState ) {
        AUTHENTICATE_MESSAGE NullMessage;

        if (((Context->NegotiateFlags & NTLMSSP_NEGOTIATE_DATAGRAM) ==
                NTLMSSP_NEGOTIATE_DATAGRAM) &&
            (InputTokenSize != 0) &&
            (InputToken != NULL) ) {

            //
            // we are doing a reauthentication for datagram, so let this
            // through.  We don't want the security.dll remapping this
            // context.
            //

            *ContextAttributes |= SSP_RET_REAUTHENTICATION;

        } else {

            //
            // To make sure this is the intended meaning of the call, check
            // that the input token is NULL.
            //

            if ( (InputTokenSize != 0) || (InputToken != NULL) ) {

                SecStatus = SEC_E_INVALID_TOKEN;
                SspPrint(( SSP_CRITICAL,
                "SsprHandleChallengeMessage: (re-auth) invalid InputTokenSize.\n" ));
                goto Cleanup;
            }

            if ( (*OutputTokenSize < sizeof(NullMessage))  &&
                 ((ContextReqFlags & ISC_REQ_ALLOCATE_MEMORY) == 0))
            {
                SecStatus = SEC_E_BUFFER_TOO_SMALL;
                SspPrint(( SSP_CRITICAL,
                "SsprHandleChallengeMessage: invalid OutputTokenSize.\n" ));
            }
            else {

                strcpy( (char *)NullMessage.Signature, NTLMSSP_SIGNATURE );
                NullMessage.MessageType = NtLmAuthenticate;
                RtlZeroMemory(
                          &NullMessage.LmChallengeResponse,5*sizeof(STRING));
                if ((ContextReqFlags & ISC_REQ_ALLOCATE_MEMORY) == 0)
                {
                    RtlCopyMemory( *OutputToken,
                               &NullMessage,
                               sizeof(NullMessage));
                }
                else
                {
                    PAUTHENTICATE_MESSAGE NewNullMessage = (PAUTHENTICATE_MESSAGE)
                                            NtLmAllocate(sizeof(NullMessage));
                    if ( NewNullMessage == NULL)
                    {
                        SecStatus = STATUS_NO_MEMORY;
                        SspPrint(( SSP_CRITICAL,
                        "SsprHandleChallengeMessage: Error allocating NewNullMessage.\n" ));
                        goto Cleanup;
                    }

                    *OutputToken = NewNullMessage;
                    NewNullMessage = NULL;
                    *ContextAttributes |= ISC_RET_ALLOCATED_MEMORY;
                }
                *OutputTokenSize = sizeof(NullMessage);
            }

            *ContextAttributes |= SSP_RET_REAUTHENTICATION;
            goto Cleanup;

        }

    } else if ( Context->State != NegotiateSentState ) {
        SspPrint(( SSP_CRITICAL,
                  "SsprHandleChallengeMessage: "
                  "Context not in NegotiateSentState\n" ));
        SecStatus = SEC_E_OUT_OF_SEQUENCE;
        goto Cleanup;
    }

    //
    // We don't support any options.
    // Complain about those that require we do something.
    //

    if ( (ContextReqFlags & ISC_REQ_PROMPT_FOR_CREDS) != 0 ){

        SspPrint(( SSP_CRITICAL,
                 "SsprHandleChallengeMessage: invalid ContextReqFlags 0x%lx.\n",
                 ContextReqFlags ));
        SecStatus = SEC_E_INVALID_CONTEXT_REQ;
        goto Cleanup;
    }

    //
    // Ignore the Credential Handle.
    //
    // Since this is the second call,
    // the credential is implied by the Context.
    // We could double check that the Credential Handle is either NULL or
    // correct.  However, our implementation doesn't maintain a close
    // association between the two (actually no association) so checking
    // would require a lot of overhead.
    //

    UNREFERENCED_PARAMETER( CredentialHandle );

    //
    // Get the ChallengeMessage.
    //

    if ( InputTokenSize < sizeof(OLD_CHALLENGE_MESSAGE) ) {
        SspPrint(( SSP_CRITICAL,
                  "SsprHandleChallengeMessage: "
                  "ChallengeMessage size wrong %ld\n",
                  InputTokenSize ));
        SecStatus = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    SecStatus = SspContextGetMessage( InputToken,
                                      InputTokenSize,
                                      NtLmChallenge,
                                      (PVOID *)&ChallengeMessage );

    if ( !NT_SUCCESS(SecStatus) ) {
        SspPrint(( SSP_CRITICAL,
                  "SsprHandleChallengeMessage: "
                  "ChallengeMessage GetMessage returns 0x%lx\n",
                  SecStatus ));
        goto Cleanup;
    }



    if( ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_DATAGRAM ) {

        //
        // take out any flags we didn't ask for -- self defense from bogus combinations
        //

        ChallengeMessage->NegotiateFlags &=
            ( Context->NegotiateFlags |
                NTLMSSP_NEGOTIATE_TARGET_INFO |
                NTLMSSP_TARGET_TYPE_SERVER |
                NTLMSSP_TARGET_TYPE_DOMAIN |
                NTLMSSP_NEGOTIATE_LOCAL_CALL );
    }


    //
    // Determine if the caller wants OEM or UNICODE
    //

    if ( ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_UNICODE ) {
        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_UNICODE;
        Context->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_OEM;
        DoUnicode = TRUE;
    } else if ( ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_OEM ){
        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_OEM;
        Context->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_UNICODE;
        DoUnicode = FALSE;
    } else {
        SspPrint(( SSP_CRITICAL,
                  "SsprHandleChallengeMessage: "
                  "ChallengeMessage bad NegotiateFlags 0x%lx\n",
                  ChallengeMessage->NegotiateFlags ));
        SecStatus = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    //
    // Copy other interesting negotiate flags into the context
    //


    if( ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_TARGET_INFO ) {
        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_TARGET_INFO;
    } else {
        Context->NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_TARGET_INFO);
    }

    //
    // if got NTLM2, don't use LM_KEY
    //

    if (ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2) {

        if( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_LM_KEY ) {
            SspPrint(( SSP_NEGOTIATE_FLAGS,
                      "SsprHandleChallengeMessage: "
                      "Server support NTLM2 caused LM_KEY to be disabled.\n" ));
        }

        ChallengeMessage->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_LM_KEY;
        Context->NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_LM_KEY);
    } else {
        if( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2 ) {
            SspPrint(( SSP_NEGOTIATE_FLAGS,
                      "SsprHandleChallengeMessage: "
                      "Server didn't support NTLM2 and client did.\n" ));
        }

        Context->NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_NTLM2);
    }

    if ((ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM) == 0) {
        if( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM ) {
            SspPrint(( SSP_NEGOTIATE_FLAGS,
                      "SsprHandleChallengeMessage: "
                      "Server didn't support NTLM and client did.\n" ));
        }

        Context->NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_NTLM);
    }


    if ((ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_KEY_EXCH) == 0) {
        if( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_KEY_EXCH ) {
            SspPrint(( SSP_NEGOTIATE_FLAGS,
                      "SsprHandleChallengeMessage: "
                      "Server didn't support KEY_EXCH and client did.\n" ));
        }

        Context->NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_KEY_EXCH);
    }


    if ((ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_LM_KEY) == 0) {
        if( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_LM_KEY ) {
            SspPrint(( SSP_NEGOTIATE_FLAGS,
                      "SsprHandleChallengeMessage: "
                      "Server didn't support LM_KEY and client did.\n" ));
        }

        Context->NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_LM_KEY);
    }


    //
    // make sure KEY_EXCH is always set if DATAGRAM negotiated and we need a key
    //  this is for local internal use; its now safe because we've got the bits
    //  to go on the wire copied...
    //

    if ((Context->NegotiateFlags & NTLMSSP_NEGOTIATE_DATAGRAM) &&
        (Context->NegotiateFlags & (NTLMSSP_NEGOTIATE_SIGN |NTLMSSP_NEGOTIATE_SEAL)))
    {
        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_KEY_EXCH;
    }


    //
    // allow negotiate of certain options such as sign/seal when server
    // asked for it, but client didn't.
    //

#if 0
////
    if( ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_SEAL ) {
        if( (Context->NegotiateFlags & NTLMSSP_NEGOTIATE_SEAL) == 0 ) {
            SspPrint(( SSP_API_MORE,
                  "SsprHandleChallengeMessage: client didn't request SEAL but server did, adding SEAL.\n"));
        }

        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_SEAL;
    }

    if( ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_SIGN ) {
        if( (Context->NegotiateFlags & NTLMSSP_NEGOTIATE_SIGN) == 0 ) {
            SspPrint(( SSP_API_MORE,
                  "SsprHandleChallengeMessage: client didn't request SIGN but server did, adding SIGN.\n"));
        }

        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_SIGN;
    }
////
#endif


    //
    // if server didn't support certain crypto strengths, insure they
    // are disabled.
    //

    if( (ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_56) == 0 ) {
        if( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_56 ) {
            SspPrint(( SSP_NEGOTIATE_FLAGS,
                  "SsprHandleChallengeMessage: Client supported 56, but server didn't.\n"));
        }

        Context->NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_56);
    }


    if( (ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_128) == 0 ) {

        if( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_128 ) {
            SspPrint(( SSP_NEGOTIATE_FLAGS,
                  "SsprHandleChallengeMessage: Client supported 128, but server didn't.\n"));
        }

        Context->NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_128);
    }



    //
    // Check that server gave minimum security required.
    // not done for legacy down-level case, as, NegotiateFlags are
    // constructed from incomplete information.
    //

    if( !fCallFromRedir )
    {
        if (!SsprCheckMinimumSecurity(
                    Context->NegotiateFlags,
                    NtLmGlobalMinimumClientSecurity)) {

            SecStatus = SEC_E_UNSUPPORTED_FUNCTION;
            SspPrint(( SSP_CRITICAL,
                      "SsprHandleChallengeMessage: "
                      "ChallengeMessage didn't support minimum security requirements.\n" ));
            goto Cleanup;
        }
    }



    if (ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_ALWAYS_SIGN ) {
        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_ALWAYS_SIGN;
    } else {
        Context->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_ALWAYS_SIGN;
    }

    //
    // Determine that the caller negotiated to NTLM or nothing, but not
    // NetWare.
    //

    if ( (ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NETWARE) &&
        ((ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM) == 0) &&
        ((ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2) == 0)
        ) {
        SecStatus = STATUS_NOT_SUPPORTED;
        SspPrint(( SSP_CRITICAL,
                  "SsprHandleChallengeMessage: "
                  "ChallengeMessage asked for Netware only.\n" ));
        goto Cleanup;
    }

    //
    // Check if we negotiated for identify level
    //

    if (Context->NegotiateFlags & NTLMSSP_NEGOTIATE_IDENTIFY) {
        if (ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_IDENTIFY) {

            Context->ContextFlags |= ISC_REQ_IDENTIFY;
            *ContextAttributes |= ISC_RET_IDENTIFY;
        } else {
            SecStatus = STATUS_NOT_SUPPORTED;
            SspPrint(( SSP_CRITICAL,
                  "SsprHandleChallengeMessage: "
                  "ChallengeMessage bad NegotiateFlags 0x%lx\n",
                  ChallengeMessage->NegotiateFlags ));
            goto Cleanup;
        }

    }

    //
    // If the server is running on this same machine,
    //  just duplicate our caller's token and use it.
    //

    if ( ChallengeMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_LOCAL_CALL ) {
        ULONG_PTR ServerContextHandle;
        PSSP_CONTEXT ServerContext;
        SECURITY_IMPERSONATION_LEVEL ImpersonationLevel = SecurityImpersonation;
        static const UCHAR FixedSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH] = {
                    'S', 'y', 's', 't', 'e', 'm', 'L', 'i',
                    'b', 'r', 'a', 'r', 'y', 'D', 'T', 'C'
                    };

        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_LOCAL_CALL;

        SspPrint(( SSP_MISC,
                  "SsprHandleChallengeMessage: Local Call.\n"));

        //
        // Require the new challenge message if we are going to access the
        // server context handle
        //

        if ( InputTokenSize < sizeof(CHALLENGE_MESSAGE) ) {
            SecStatus = SEC_E_INVALID_TOKEN;
            SspPrint(( SSP_CRITICAL,
                  "SsprHandleChallengeMessage: invalid InputTokenSize.\n"));
            goto Cleanup;
        }

        //
        // Open the server's context here within this process.
        //

        ServerContextHandle = (ULONG_PTR)(ChallengeMessage->ServerContextHandle);

        ServerContext = SspContextReferenceContext(
                            ServerContextHandle,
                            FALSE );

        if ( ServerContext == NULL ) {
            //
            // This means the server has lied about this being a local call or
            //  the server process has exitted.
            //
            SspPrint(( SSP_CRITICAL,
                      "SsprHandleChallengeMessage: "
                      "ChallengeMessage bad ServerContextHandle 0x%p\n",
                      ChallengeMessage->ServerContextHandle));
            SecStatus = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }

        if ((Context->NegotiateFlags & NTLMSSP_NEGOTIATE_IDENTIFY) != 0) {
            ImpersonationLevel = SecurityIdentification;
        }

        SecStatus = SspDuplicateToken(
                        ClientTokenHandle,
                        ImpersonationLevel,
                        &ServerContext->TokenHandle
                        );

        SspContextDereferenceContext( ServerContext );

        if (!NT_SUCCESS(SecStatus)) {
            SspPrint(( SSP_CRITICAL,
                      "SsprHandleChallengeMessage: "
                      "Could not duplicate client token 0x%lx\n",
                      SecStatus ));
            goto Cleanup;
        }



        //
        // give local call a hard-coded session key so calls into RDR
        // to fetch a session key succeed (eg: RtlGetUserSessionKeyClient)
        //

        RtlCopyMemory(Context->SessionKey, FixedSessionKey, MSV1_0_USER_SESSION_KEY_LENGTH);

        //
        // Don't pass any credentials in the authenticate message.
        //

        RtlInitString( &DomainName, NULL );
        RtlInitString( &UserName, NULL );
        RtlInitString( &Workstation, NULL );
        RtlInitString( &NtChallengeResponse, NULL );
        RtlInitString( &LmChallengeResponse, NULL );
        RtlInitString( &DatagramSessionKey, NULL );

    //
    // If the server is running on a diffent machine,
    //  determine the caller's DomainName, UserName and ChallengeResponse
    //  to pass back in the AuthenicateMessage.
    //

    } else {

        //
        // Build the GetChallengeResponse message to pass to the LSA.
        //

        PCHAR MarshalPtr = (PCHAR)(GetChallengeResponse+1);     // marshalling pointer
        ULONG MarshalBytes;
        UNICODE_STRING TargetInfo;
        UNICODE_STRING TargetName;

        ZeroMemory( GetChallengeResponse, sizeof(*GetChallengeResponse) );
        GetChallengeResponseSize = sizeof(*GetChallengeResponse);
        GetChallengeResponse->MessageType = MsV1_0Lm20GetChallengeResponse;
        GetChallengeResponse->ParameterControl = 0;


        //
        // if caller specifically asked for non nt session key, give it to them
        //

        if ( (ChallengeMessage->NegotiateFlags & NTLMSSP_REQUEST_NON_NT_SESSION_KEY ) ||
             (Context->NegotiateFlags & NTLMSSP_NEGOTIATE_LM_KEY))
        {
            GetChallengeResponse->ParameterControl |= RETURN_NON_NT_USER_SESSION_KEY;
        }

        GetChallengeResponse->ParameterControl |= GCR_NTLM3_PARMS;

        SspContextCopyStringAbsolute( GetChallengeResponse,
                          (PSTRING)&GetChallengeResponse->LogonDomainName,
                          (PSTRING)&Context->DomainName,
                          &MarshalPtr);


        SspContextCopyStringAbsolute( GetChallengeResponse,
                          (PSTRING)&GetChallengeResponse->UserName,
                          (PSTRING)&Context->UserName,
                          &MarshalPtr);

        //
        // if TargetInfo present, use it, otherwise construct it from target name
        //

        if (Context->NegotiateFlags & NTLMSSP_NEGOTIATE_TARGET_INFO) {
            if ( InputTokenSize < sizeof(CHALLENGE_MESSAGE) ) {
                SspPrint(( SSP_CRITICAL,
                          "SspHandleChallengeMessage: "
                          "ChallengeMessage size wrong when target info flag on %ld\n",
                          InputTokenSize ));
                SecStatus = SEC_E_INVALID_TOKEN;
                goto Cleanup;
            }

            //
            // validate and relocate the target info
            //

            if (!SspConvertRelativeToAbsolute (
                ChallengeMessage,
                InputTokenSize,
                &ChallengeMessage->TargetInfo,
                (PSTRING)&TargetInfo,
                DoUnicode,
                TRUE    // NULL target info OK
                ))
            {
                SspPrint(( SSP_CRITICAL,
                          "SspHandleChallengeMessage: "
                          "ChallengeMessage.TargetInfo size wrong %ld\n",
                          InputTokenSize ));
                SecStatus = SEC_E_INVALID_TOKEN;
                goto Cleanup;
            }

            //
            // MSV needs the server name to be 'in' the passed in buffer.
            //

            SspContextCopyStringAbsolute( GetChallengeResponse,
                          (PSTRING)&GetChallengeResponse->ServerName,
                          (PSTRING)&TargetInfo,
                          &MarshalPtr);


            GetChallengeResponseSize += GetChallengeResponse->ServerName.Length;

            //
            // tell GCR that its an AV list.
            //

            GetChallengeResponse->ParameterControl |= GCR_TARGET_INFO;

        } else {

            //
            // validate and relocate the target name
            //

            if (!SspConvertRelativeToAbsolute (
                                ChallengeMessage,
                                InputTokenSize,
                                &ChallengeMessage->TargetName,
                                (PSTRING)&TargetName,
                                DoUnicode,
                                TRUE    // NULL targetname OK
                                ))
            {
                SspPrint(( SSP_CRITICAL,
                          "SspHandleChallengeMessage: "
                          "ChallengeMessage.TargetName size wrong %ld\n",
                          InputTokenSize ));
                SecStatus = SEC_E_INVALID_TOKEN;
                goto Cleanup;
            }

            //
            // convert TargetName to Unicode if needed
            //

            if ( !DoUnicode ) {
                UNICODE_STRING TempString;

                Status = RtlOemStringToUnicodeString(
                            &TempString,
                            (PSTRING)&TargetName,
                            TRUE);

                if ( !NT_SUCCESS(Status) ) {
                    SecStatus = SspNtStatusToSecStatus( Status,
                                                        SEC_E_INSUFFICIENT_MEMORY );
                    goto Cleanup;
                }

                TargetName = TempString;
            }

            //
            // MSV needs the server name to be 'in' the passed in buffer.
            //

            SspContextCopyStringAbsolute( GetChallengeResponse,
                          (PSTRING)&GetChallengeResponse->ServerName,
                          (PSTRING)&TargetName,
                          &MarshalPtr);


            if( !DoUnicode )
            {
                RtlFreeUnicodeString( &TargetName );
            }

        }


        GetChallengeResponseSize += GetChallengeResponse->ServerName.Length +
                                    GetChallengeResponse->LogonDomainName.Length +
                                    GetChallengeResponse->UserName.Length;


        //
        // Check for null session. This is the case if the caller supplies
        // an empty username, domainname, and password.
        //

        //
        // duplicate the hidden password string, then reveal it into
        // new buffer.  This avoids thread race conditions during hide/reveal
        // and also allows us to avoid re-hiding the material.
        //

        SecStatus = NtLmDuplicateUnicodeString( &RevealedPassword, &Context->Password );
        if(!NT_SUCCESS( SecStatus ) ) {
                SspPrint(( SSP_CRITICAL,
                "SsprHandleChallengeMessage: NtLmDuplicateUnicodeString returned %d\n",Status));
            goto Cleanup;
        }

        SspRevealPassword(&RevealedPassword);

        if (((Context->ContextFlags & ISC_RET_NULL_SESSION) != 0) ||
            (((Context->DomainName.Length == 0) && (Context->DomainName.Buffer != NULL)) &&
            ((Context->UserName.Length == 0) && (Context->UserName.Buffer != NULL)) &&
            ((Context->Password.Length == 0) && (Context->Password.Buffer != NULL)))) {


#define NULL_SESSION_REQUESTED RETURN_RESERVED_PARAMETER

            GetChallengeResponse->ParameterControl |= NULL_SESSION_REQUESTED |
                                                    USE_PRIMARY_PASSWORD;

        } else {

            //
            // We aren't doing a null session intentionally. MSV may choose
            // to do a null session if we have no credentials available.
            //

            if ( Context->DomainName.Buffer == NULL ) {
                GetChallengeResponse->ParameterControl |=
                                  RETURN_PRIMARY_LOGON_DOMAINNAME;
            }
            if ( Context->UserName.Buffer == NULL ) {
                GetChallengeResponse->ParameterControl |= RETURN_PRIMARY_USERNAME;
            }

            //
            // The password may be a zero length password
            //

            GetChallengeResponse->Password = RevealedPassword;
            if ( Context->Password.Buffer == NULL ) {
                SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
                ULONG TokenInformationSize = sizeof(SECURITY_IMPERSONATION_LEVEL);

                GetChallengeResponse->ParameterControl |= USE_PRIMARY_PASSWORD;

                //
                // Check to make sure the client's impersonation level is
                // less than or equal to what they are asking for on the
                // server. We only do this if we actually have their token.
                //

                if (ClientTokenHandle != NULL) {
                    Status = NtQueryInformationToken(
                                ClientTokenHandle,
                                TokenImpersonationLevel,
                                (PVOID) &ImpersonationLevel,
                                TokenInformationSize,
                                &TokenInformationSize
                                );

                    //
                    // If the token is a primary token the return code will be
                    // STATUS_INVALID_INFO_CLASS because the token is forced to be
                    // equivalent to SecurityImpersonation.
                    //

                    if (NT_SUCCESS(Status)) {
                        if ((ImpersonationLevel ==  SecurityIdentification) &&
                            ((Context->ContextFlags &
                              NTLMSSP_NEGOTIATE_IDENTIFY) == 0)) {
                            SecStatus = SEC_E_NO_CREDENTIALS;
                            SspPrint(( SSP_CRITICAL,
                                "SsprHandleChallengeMessage: Error 1, 0x%lx.\n", SecStatus));
                            goto Cleanup;
                        } else if (ImpersonationLevel != SecurityImpersonation) {
                            SecStatus = SEC_E_NO_CREDENTIALS;
                            SspPrint(( SSP_CRITICAL,
                                "SsprHandleChallengeMessage: Error 2, 0x%lx.\n", SecStatus));
                        }
                    } else if (Status != STATUS_INVALID_INFO_CLASS) {
                        SecStatus = SspNtStatusToSecStatus(
                                     Status,
                                     SEC_E_NO_CREDENTIALS
                                     );
                        SspPrint(( SSP_CRITICAL,
                                "SsprHandleChallengeMessage: Error 3, 0x%lx.\n", SecStatus));
                        goto Cleanup;
                    }
                }

            } else {
                //
                // MSV needs the password to be 'in' the passed in buffer.
                //

                RtlCopyMemory(MarshalPtr,
                              GetChallengeResponse->Password.Buffer,
                              GetChallengeResponse->Password.Length + sizeof(WCHAR));
                GetChallengeResponse->Password.Buffer =
                                           (LPWSTR)(MarshalPtr);
                GetChallengeResponseSize += GetChallengeResponse->Password.Length +
                                            sizeof(WCHAR);
            }
        }

        //
        // scrub the cleartext password now to avoid pagefile exposure
        // during lengthy processing.
        //

        if( RevealedPassword.Buffer != NULL ) {
            ZeroMemory( RevealedPassword.Buffer, RevealedPassword.Length );
            NtLmFree( RevealedPassword.Buffer );
            RevealedPassword.Buffer = NULL;
        }


        GetChallengeResponse->LogonId = *ClientLogonId;

        RtlCopyMemory( &GetChallengeResponse->ChallengeToClient,
                       ChallengeMessage->Challenge,
                       MSV1_0_CHALLENGE_LENGTH );

        //
        // if NTLM2 negotiated, then ask MSV1_0 to mix my challenge with the server's...
        //

        if (Context->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2)
        {
            GetChallengeResponse->ParameterControl |= GENERATE_CLIENT_CHALLENGE;
        }

        //
        // Get the DomainName, UserName, and ChallengeResponse from the MSV
        //

        Status = LsaApCallPackage(
                    (PLSA_CLIENT_REQUEST)(-1),
                    GetChallengeResponse,
                    GetChallengeResponse,
                    GetChallengeResponseSize,
                    (PVOID *)&ChallengeResponseMessage,
                    &ChallengeResponseSize,
                    &ProtocolStatus );

        if ( !NT_SUCCESS(Status) ) {
            SspPrint(( SSP_CRITICAL,
            "SsprHandleChallengeMessage: "
            "ChallengeMessage LsaCall to get ChallengeResponse returns 0x%lx\n",
              Status ));
            SecStatus = SspNtStatusToSecStatus( Status, SEC_E_NO_CREDENTIALS);
            goto Cleanup;
        }

        if ( !NT_SUCCESS(ProtocolStatus) ) {
            Status = ProtocolStatus;
            SspPrint(( SSP_CRITICAL,
              "SsprHandleChallengeMessage: ChallengeMessage LsaCall "
              "to get ChallengeResponse returns ProtocolStatus 0x%lx\n",
              Status ));
            SecStatus = SspNtStatusToSecStatus( Status, SEC_E_NO_CREDENTIALS);
            goto Cleanup;
        }

        //
        // Check to see if we are doing a null session
        //

        if ((ChallengeResponseMessage->CaseSensitiveChallengeResponse.Length == 0) &&
            (ChallengeResponseMessage->CaseInsensitiveChallengeResponse.Length == 1)) {

            *ContextAttributes |= ISC_RET_NULL_SESSION;
            Context->ContextFlags |= ISC_RET_NULL_SESSION;
            Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_NULL_SESSION;
        } else {

            //
            // Normalize things by copying the default domain name and user name
            // into the ChallengeResponseMessage structure.
            //

            if ( Context->DomainName.Buffer != NULL ) {
                ChallengeResponseMessage->LogonDomainName = Context->DomainName;
            }
            if ( Context->UserName.Buffer != NULL ) {
                ChallengeResponseMessage->UserName = Context->UserName;
            }
        }

        //
        // Convert the domainname/user name to the right character set.
        //

        if ( DoUnicode ) {
            DomainName = *(PSTRING)&ChallengeResponseMessage->LogonDomainName;
            UserName = *(PSTRING)&ChallengeResponseMessage->UserName;
            Workstation =  *(PSTRING)&NtLmGlobalUnicodeComputerNameString;
        } else {
            Status = RtlUpcaseUnicodeStringToOemString(
                        &DomainName,
                        &ChallengeResponseMessage->LogonDomainName,
                        TRUE);

            if ( !NT_SUCCESS(Status) ) {
                SspPrint(( SSP_CRITICAL,
                            "SsprHandleChallengeMessage: RtlUpcaseUnicodeToOemString (DomainName) returned 0x%lx.\n", Status));
                SecStatus = SspNtStatusToSecStatus( Status,
                                                    SEC_E_INSUFFICIENT_MEMORY);
                goto Cleanup;
            }

            Status = RtlUpcaseUnicodeStringToOemString(
                        &UserName,
                        &ChallengeResponseMessage->UserName,
                        TRUE);

            if ( !NT_SUCCESS(Status) ) {
                SspPrint(( SSP_CRITICAL,
                            "SsprHandleChallengeMessage: RtlUpcaseUnicodeToOemString (UserName) returned 0x%lx.\n", Status));
                SecStatus = SspNtStatusToSecStatus( Status,
                                                    SEC_E_INSUFFICIENT_MEMORY);
                goto Cleanup;
            }
            Workstation =  NtLmGlobalOemComputerNameString;

        }

        //
        // Save the ChallengeResponses
        //

        LmChallengeResponse =
            ChallengeResponseMessage->CaseInsensitiveChallengeResponse;
        NtChallengeResponse =
            ChallengeResponseMessage->CaseSensitiveChallengeResponse;


        //
        // prepare to send encrypted randomly generated session key
        //

        DatagramSessionKey.Buffer = (CHAR*)DatagramKey;
        DatagramSessionKey.Length =
          DatagramSessionKey.MaximumLength = 0;

        //
        // Generate the session key, or encrypt the previosly generated random one,
        // from various bits of info. Fill in session key if needed.
        //

        SecStatus = SsprMakeSessionKey(
                            Context,
                            &LmChallengeResponse,
                            ChallengeResponseMessage->UserSessionKey,
                            ChallengeResponseMessage->LanmanSessionKey,
                            &DatagramSessionKey
                            );

        if (SecStatus != SEC_E_OK)
        {
            SspPrint(( SSP_CRITICAL,
                        "SsprHandleChallengeMessage: SsprMakeSessionKey\n"));
            goto Cleanup;
        }

    }

    //
    // If the caller specified SEQUENCE_DETECT or REPLAY_DETECT,
    // that means they want to use the MakeSignature/VerifySignature
    // calls.  Add this to the returned attributes and the context
    // negotiate flags.
    //

    if ((Context->NegotiateFlags &
         ChallengeMessage->NegotiateFlags &
         NTLMSSP_NEGOTIATE_SIGN) ||
        (ContextReqFlags & ISC_REQ_REPLAY_DETECT)) {

        Context->ContextFlags |= ISC_RET_REPLAY_DETECT;
        *ContextAttributes |= ISC_RET_REPLAY_DETECT;
        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_SIGN;
    }

    if ((Context->NegotiateFlags &
         ChallengeMessage->NegotiateFlags &
         NTLMSSP_NEGOTIATE_SIGN) ||
        (ContextReqFlags & ISC_REQ_SEQUENCE_DETECT)) {

        Context->ContextFlags |= ISC_RET_SEQUENCE_DETECT;
        *ContextAttributes |= ISC_RET_SEQUENCE_DETECT;
        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_SIGN;
    }

    if ((Context->NegotiateFlags &
         ChallengeMessage->NegotiateFlags &
         NTLMSSP_NEGOTIATE_SIGN) ||
        (ContextReqFlags & ISC_REQ_INTEGRITY)) {

        Context->ContextFlags |= ISC_RET_INTEGRITY;
        *ContextAttributes |= ISC_RET_INTEGRITY;
        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_SIGN;
    }

    if ((Context->NegotiateFlags &
         ChallengeMessage->NegotiateFlags &
         NTLMSSP_NEGOTIATE_SEAL) ||
        (ContextReqFlags & ISC_REQ_CONFIDENTIALITY)) {
        if (NtLmGlobalEncryptionEnabled) {
            Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_SEAL;
            Context->ContextFlags |= ISC_REQ_CONFIDENTIALITY;
            *ContextAttributes |= ISC_REQ_CONFIDENTIALITY;
        } else {
            SecStatus = STATUS_NOT_SUPPORTED;
            SspPrint(( SSP_CRITICAL,
                        "SsprHandleChallengeMessage: NtLmGlobalEncryption not enabled.\n"));
            goto Cleanup;
        }
    }

    if ((Context->NegotiateFlags &
         ChallengeMessage->NegotiateFlags &
         NTLMSSP_NEGOTIATE_DATAGRAM) ==
        NTLMSSP_NEGOTIATE_DATAGRAM ) {
        *ContextAttributes |= ISC_RET_DATAGRAM;
        Context->ContextFlags |= ISC_RET_DATAGRAM;
        Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_DATAGRAM;
    }

    //
    // Slip in the hacky mutual auth override here:
    //

    if ( ((ContextReqFlags & ISC_REQ_MUTUAL_AUTH) != 0 ) &&
         (NtLmGlobalMutualAuthLevel < 2 ) ) {

        *ContextAttributes |= ISC_RET_MUTUAL_AUTH ;

        if ( NtLmGlobalMutualAuthLevel == 0 )
        {
            Context->ContextFlags |= ISC_RET_MUTUAL_AUTH ;
        }

    }

    //
    // Allocate an authenticate message
    //

    AuthenticateMessageSize =
        sizeof(*AuthenticateMessage) +
        LmChallengeResponse.Length +
        NtChallengeResponse.Length +
        DomainName.Length +
        UserName.Length +
        Workstation.Length +
        DatagramSessionKey.Length;

    if ((ContextReqFlags & ISC_REQ_ALLOCATE_MEMORY) == 0)
    {
        if ( AuthenticateMessageSize > *OutputTokenSize ) {
            SspPrint(( SSP_CRITICAL,
                "SsprHandleChallengeMessage: OutputTokenSize is 0x%lx.\n", *OutputTokenSize));
            SecStatus = SEC_E_BUFFER_TOO_SMALL;
            goto Cleanup;
        }
    }

    AuthenticateMessage = (PAUTHENTICATE_MESSAGE)
                          NtLmAllocate(AuthenticateMessageSize );

    if ( AuthenticateMessage == NULL ) {
        SecStatus = STATUS_NO_MEMORY;
        SspPrint(( SSP_CRITICAL,
            "SsprHandleChallengeMessage: Error allocating AuthenticateMessage.\n" ));
        goto Cleanup;
    }

    //
    // Build the authenticate message
    //

    strcpy( (char *)AuthenticateMessage->Signature, NTLMSSP_SIGNATURE );
    AuthenticateMessage->MessageType = NtLmAuthenticate;

    Where = (PCHAR)(AuthenticateMessage+1);

    //
    // Copy the strings needing 2 byte alignment.
    //

    SspContextCopyString( AuthenticateMessage,
                          &AuthenticateMessage->DomainName,
                          &DomainName,
                          &Where );

    SspContextCopyString( AuthenticateMessage,
                          &AuthenticateMessage->UserName,
                          &UserName,
                          &Where );

    SspContextCopyString( AuthenticateMessage,
                          &AuthenticateMessage->Workstation,
                          &Workstation,
                          &Where );

    //
    // Copy the strings not needing special alignment.
    //
    SspContextCopyString( AuthenticateMessage,
                          &AuthenticateMessage->LmChallengeResponse,
                          &LmChallengeResponse,
                          &Where );

    SspContextCopyString( AuthenticateMessage,
                          &AuthenticateMessage->NtChallengeResponse,
                          &NtChallengeResponse,
                          &Where );

    SspContextCopyString( AuthenticateMessage,
                          &AuthenticateMessage->SessionKey,
                          &DatagramSessionKey,
                          &Where );

    AuthenticateMessage->NegotiateFlags = Context->NegotiateFlags;


    SspPrint(( SSP_NEGOTIATE_FLAGS,
          "SsprHandleChallengeMessage: ChallengeFlags: %lx AuthenticateFlags: %lx\n",
            ChallengeMessage->NegotiateFlags, AuthenticateMessage->NegotiateFlags ));


    //
    // Copy the AuthenticateMessage to the caller's address space.
    //

    if ((ContextReqFlags & ISC_REQ_ALLOCATE_MEMORY) == 0)
    {
        RtlCopyMemory( *OutputToken,
                   AuthenticateMessage,
                   AuthenticateMessageSize );
    }
    else
    {
        *OutputToken = AuthenticateMessage;
        AuthenticateMessage = NULL;
        *ContextAttributes |= ISC_RET_ALLOCATED_MEMORY;
    }

    *OutputTokenSize = AuthenticateMessageSize;

    // we need to send a second token back for the rdr
    if (fCallFromRedir)
    {
        NtLmInitializeResponse = (PNTLM_INITIALIZE_RESPONSE)
                                 NtLmAllocate(sizeof(NTLM_INITIALIZE_RESPONSE));

        if ( NtLmInitializeResponse == NULL ) {
            SecStatus = STATUS_NO_MEMORY;
            SspPrint(( SSP_CRITICAL,
                "SsprHandleChallengeMessage: Error allocating NtLmInitializeResponse.\n" ));
            goto Cleanup;
        }
        RtlCopyMemory(
            NtLmInitializeResponse->UserSessionKey,
            ChallengeResponseMessage->UserSessionKey,
            MSV1_0_USER_SESSION_KEY_LENGTH
            );

        RtlCopyMemory(
            NtLmInitializeResponse->LanmanSessionKey,
            ChallengeResponseMessage->LanmanSessionKey,
            MSV1_0_LANMAN_SESSION_KEY_LENGTH
            );
        *SecondOutputToken = NtLmInitializeResponse;
        NtLmInitializeResponse = NULL;
        *SecondOutputTokenSize = sizeof(NTLM_INITIALIZE_RESPONSE);
    }

    SspPrint((SSP_API_MORE,"Client session key = %p\n",Context->SessionKey));

    //
    // Return output parameters to the caller.
    //

    *ExpirationTime = SspContextGetTimeStamp( Context, TRUE );

    SecStatus = STATUS_SUCCESS;

    //
    // Free and locally used resources.
    //
Cleanup:

    if( RevealedPassword.Buffer != NULL ) {
        ZeroMemory( RevealedPassword.Buffer, RevealedPassword.Length );
        NtLmFree( RevealedPassword.Buffer );
    }

    if ( Context != NULL ) {

        Context->LastStatus = SecStatus;
        Context->DownLevel = fCallFromRedir;

        //
        // Don't allow this context to be used again.
        //

        if ( NT_SUCCESS(SecStatus) ) {
            Context->State = AuthenticateSentState;
        } else if ( SecStatus == SEC_I_CALL_NTLMSSP_SERVICE ) {
            Context->State = PassedToServiceState;
        } else {
            Context->State = IdleState;
        }

        RtlCopyMemory(
            SessionKey,
            Context->SessionKey,
            MSV1_0_USER_SESSION_KEY_LENGTH );

        *NegotiateFlags = Context->NegotiateFlags;

        // If we just created the context (because rdr may be talking to
        // a pre NT 5.0 server,, we need to dereference it again.

        if (fCallFromRedir && !NT_SUCCESS(SecStatus))
        {
            PSSP_CONTEXT LocalContext;
            LocalContext = SspContextReferenceContext( *ContextHandle, TRUE);
            ASSERT(LocalContext != NULL);
            if (LocalContext != NULL)
            {
                SspContextDereferenceContext( LocalContext );
            }
        }

        SspContextDereferenceContext( Context );

    }

    if ( ChallengeMessage != NULL ) {
        (VOID) NtLmFree( ChallengeMessage );
    }

    if ( AuthenticateMessage != NULL ) {
        (VOID) NtLmFree( AuthenticateMessage );
    }

    if ( ChallengeResponseMessage != NULL ) {
        (VOID) LsaFunctions->FreeLsaHeap( ChallengeResponseMessage );
    }

    if ( !DoUnicode ) {
        if ( DomainName.Buffer != NULL) {
            RtlFreeOemString( &DomainName );
        }
        if ( UserName.Buffer != NULL) {
            RtlFreeOemString( &UserName );
        }
    }

    SspPrint(( SSP_API_MORE, "Leaving SsprHandleChallengeMessage: 0x%lx\n", SecStatus ));
    return SecStatus;

}

NTSTATUS
SsprHandleAuthenticateMessage(
    IN LSA_SEC_HANDLE CredentialHandle,
    IN OUT PLSA_SEC_HANDLE ContextHandle,
    IN ULONG ContextReqFlags,
    IN ULONG InputTokenSize,
    IN PVOID InputToken,
    IN ULONG SecondInputTokenSize,
    IN PVOID SecondInputToken,
    IN OUT PULONG OutputTokenSize,
    OUT PVOID *OutputToken,
    OUT PULONG ContextAttributes,
    OUT PTimeStamp ExpirationTime,
    OUT PUCHAR SessionKey,
    OUT PULONG NegotiateFlags,
    OUT PHANDLE TokenHandle,
    OUT PNTSTATUS ApiSubStatus,
    OUT PTimeStamp PasswordExpiry,
    OUT PULONG UserFlags
    )

/*++

Routine Description:

    Handle the authenticate message part of AcceptSecurityContext.

Arguments:

    SessionKey - The session key for the context, used for signing and sealing

    NegotiateFlags - The flags negotiated for the context, used for sign & seal

    ApiSubStatus - Returns the substatus for why the logon failed.

    PasswordExpiry - Contains the time that the authenticated user's password
        expires, or 0x7fffffff ffffffff for local callers.

    UserFlags - UserFlags returned in LogonProfile.

    All other arguments same as for AcceptSecurityContext


Return Value:

    STATUS_SUCCESS - Message handled

    SEC_E_INVALID_TOKEN -- Token improperly formatted
    SEC_E_INVALID_HANDLE -- Credential/Context Handle is invalid
    SEC_E_BUFFER_TOO_SMALL -- Buffer for output token isn't big enough
    SEC_E_LOGON_DENIED -- User is no allowed to logon to this server
    SEC_E_INSUFFICIENT_MEMORY -- Not enough memory

--*/

{
    SECURITY_STATUS SecStatus = STATUS_SUCCESS;
    NTSTATUS Status = STATUS_SUCCESS;
    PSSP_CONTEXT Context = NULL;

    PNEGOTIATE_MESSAGE NegotiateMessage = NULL;
    PAUTHENTICATE_MESSAGE AuthenticateMessage = NULL;
    PNTLM_AUTHENTICATE_MESSAGE NtLmAuthenticateMessage = NULL;
    PNTLM_ACCEPT_RESPONSE NtLmAcceptResponse = NULL;
    ULONG MsvLogonMessageSize = 0;
    PMSV1_0_LM20_LOGON MsvLogonMessage = NULL;
    ULONG MsvSubAuthLogonMessageSize = 0;
    PMSV1_0_SUBAUTH_LOGON MsvSubAuthLogonMessage = NULL;
    ULONG LogonProfileMessageSize;
    PMSV1_0_LM20_LOGON_PROFILE LogonProfileMessage = NULL;

    BOOLEAN DoUnicode = FALSE;
    STRING DomainName2;
    STRING UserName2;
    STRING Workstation2;
    STRING SessionKeyString;
    UNICODE_STRING DomainName;
    UNICODE_STRING UserName;
    UNICODE_STRING Workstation;
    LARGE_INTEGER KickOffTime;

    LUID LogonId = {0};
    HANDLE LocalTokenHandle = NULL;
    BOOLEAN LocalTokenHandleOpenned = FALSE;
    TOKEN_SOURCE SourceContext;
    QUOTA_LIMITS Quotas;
    NTSTATUS SubStatus;
    STRING OriginName;
    PCHAR Where;
    UCHAR LocalSessionKey[LM_RESPONSE_LENGTH];
    PSSP_CREDENTIAL Credential = NULL;
    BOOLEAN fCallFromSrv = FALSE;
    PUNICODE_STRING AccountName = NULL;
    PUNICODE_STRING AuthenticatingAuthority = NULL;
    PUNICODE_STRING WorkstationName = NULL;
    STRING NtChallengeResponse;
    STRING LmChallengeResponse;
    BOOL fSubAuth = FALSE;
    ULONG SubAuthPackageId = 0;
    PSID AuditSid = NULL ;
    BOOLEAN fAvoidGuestAudit = FALSE;

    ASSERT(LM_RESPONSE_LENGTH >= MSV1_0_USER_SESSION_KEY_LENGTH);

    SspPrint(( SSP_API_MORE, "Entering SsprHandleAuthenticateMessage\n"));
    //
    // Initialization
    //

    *ContextAttributes = 0;
    RtlInitUnicodeString(
        &DomainName,
        NULL
        );
    RtlInitUnicodeString(
        &UserName,
        NULL
        );
    RtlInitUnicodeString(
        &Workstation,
        NULL
        );
    *ApiSubStatus = STATUS_SUCCESS;
    PasswordExpiry->LowPart = 0xffffffff;
    PasswordExpiry->HighPart = 0x7fffffff;
    *UserFlags = 0;

    if (*ContextHandle == NULL)
    {
        // This is possibly an old style srv call (for 4.0 and before)
        // so, alloc the context and replace the creds if new ones exists

        fCallFromSrv = TRUE;

        SspPrint((SSP_API_MORE, "SsprHandleAuthenticateMessage: *ContextHandle is NULL (old style SRV)\n"));


        SecStatus = SspCredentialReferenceCredential(
                                          CredentialHandle,
                                          FALSE,
                                          FALSE,
                                          &Credential );

        if ( !NT_SUCCESS( SecStatus ) )
        {
            SspPrint(( SSP_CRITICAL,
                "SsprHandleAuthenticateMessage: SspCredentialReferenceCredential returns %x.\n", SecStatus ));
            goto Cleanup;
        }

        // check the validity of the NtlmAuthenticateMessage

        if (SecondInputTokenSize < sizeof(NTLM_AUTHENTICATE_MESSAGE))
        {
            SspPrint(( SSP_CRITICAL,
                "SsprHandleAuthenticateMessage: NtlmAuthenticateMessage size if bogus.\n" ));
            SecStatus = SEC_E_INVALID_TOKEN;
            goto Cleanup;

        }

        // This is a  superflous check since we alloc only if the caller
        // has asked us too. This is to make sure that the srv always allocs

        if (ContextReqFlags  & ISC_REQ_ALLOCATE_MEMORY)
        {
            SspPrint(( SSP_CRITICAL,
                "SsprHandleAuthenticateMessage: ContextReqFlags has ISC_REQ_ALLOCATE_MEMORY.\n" ));
            SecStatus = STATUS_NOT_SUPPORTED;
            goto Cleanup;
        }

        if (*OutputTokenSize < sizeof(NTLM_ACCEPT_RESPONSE))
        {
            SspPrint(( SSP_CRITICAL,
                "SsprHandleAuthenticateMessage: NtlmAcceptResponse size if bogus.\n" ));
            SecStatus = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }

        //
        // Allocate a new context
        //

        Context = SspContextAllocateContext();

        if (Context == NULL)
        {
            SspPrint(( SSP_CRITICAL,
                "SsprHandleChallengeMessage: SspContextAllocateContext returns NULL.\n" ));
            SecStatus = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        // We've just added a context, we don't nornally add and then
        // reference it.

        SspContextDereferenceContext( Context );

        *ContextHandle = (LSA_SEC_HANDLE) Context;

        // Assign the Credential

        Context->Credential = Credential;
        Credential = NULL;

        NtLmAuthenticateMessage = (PNTLM_AUTHENTICATE_MESSAGE) SecondInputToken;
        if (NtLmAuthenticateMessage == NULL)
        {
            SspPrint(( SSP_CRITICAL,
                "SsprHandleAuthenticateMessage: Error while assigning NtLmAuthenticateMessage\n" ));
            SecStatus = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        // copy challenge from NTLM_AUTHENTICATE_MESSAGE
        RtlCopyMemory(Context->Challenge,
                      NtLmAuthenticateMessage->ChallengeToClient,
                      MSV1_0_CHALLENGE_LENGTH);

        if (NtLmAuthenticateMessage->ParameterControl & MSV1_0_SUBAUTHENTICATION_FLAGS)
        {
            fSubAuth = TRUE;
            SubAuthPackageId = (NtLmAuthenticateMessage->ParameterControl >>
                                MSV1_0_SUBAUTHENTICATION_DLL_SHIFT)
                                ;
        }
        Context->State = ChallengeSentState;
        Context->NegotiateFlags = NTLMSSP_NEGOTIATE_UNICODE ;

        //
        // The server may request this option with a <= 4.0 client, in
        // which case HandleNegotiateMessage, which normally sets
        // this flag, won't have been called.
        //

        if ( (ContextReqFlags & ASC_REQ_ALLOW_NON_USER_LOGONS ) != 0) {

            *ContextAttributes |= ASC_RET_ALLOW_NON_USER_LOGONS;
            Context->ContextFlags |= ASC_RET_ALLOW_NON_USER_LOGONS;
        }


    }

    //
    // Find the currently existing context.
    //

    Context = SspContextReferenceContext( *ContextHandle, FALSE );

    if ( Context == NULL ) {

        SecStatus = STATUS_INVALID_HANDLE;
        SspPrint(( SSP_CRITICAL,
                  "SsprHandleAuthenticateMessage: Error from SspContextReferenceContext.\n" ));

        goto Cleanup;
    }


    if ( ( Context->State != ChallengeSentState) &&
         ( Context->State != AuthenticatedState) ) {
        SspPrint(( SSP_CRITICAL,
                  "SsprHandleAuthenticateMessage: "
                  "Context not in ChallengeSentState\n" ));
        SecStatus = SEC_E_OUT_OF_SEQUENCE;
        goto Cleanup;
    }

    //
    // Ignore the Credential Handle.
    //
    // Since this is the second call,
    // the credential is implied by the Context.
    // We could double check that the Credential Handle is either NULL or
    // correct.  However, our implementation doesn't maintain a close
    // association between the two (actually no association) so checking
    // would require a lot of overhead.
    //

    UNREFERENCED_PARAMETER( CredentialHandle );

    //
    // Get the AuthenticateMessage.
    //

    if ( InputTokenSize < sizeof(OLD_AUTHENTICATE_MESSAGE) ) {
        SspPrint(( SSP_CRITICAL,
                  "SsprHandleAuthenticateMessage: "
                  "AuthenticateMessage size wrong %ld\n",
                  InputTokenSize ));
        SecStatus = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    SecStatus = SspContextGetMessage( InputToken,
                                      InputTokenSize,
                                      NtLmAuthenticate,
                                      (PVOID *)&AuthenticateMessage );

    if ( !NT_SUCCESS(SecStatus) ) {
        SspPrint(( SSP_CRITICAL,
                  "SsprHandleAuthenticateMessage: "
                  "AuthenticateMessage GetMessage returns 0x%lx\n",
                  SecStatus ));
        goto Cleanup;
    }

    if (fCallFromSrv)
    {
        // Copy the Context Negotiate Flags from what's sent in
        Context->NegotiateFlags |= AuthenticateMessage->NegotiateFlags;
    }
    //
    // If the call comes and we have already authenticated, then it is
    // probably RPC trying to reauthenticate, which happens when someone
    // calls two interfaces on the same connection.  In this case we don't
    // have to do anything - we just return success and let them get on
    // with it.  We do want to check that the input token is all zeros,
    // though.
    //

    if ( Context->State == AuthenticatedState ) {
        AUTHENTICATE_MESSAGE NullMessage;

        *OutputTokenSize = 0;

        //
        // Check that all the fields are null.  There are 5 strings
        // in the Authenticate message that have to be set to zero.
        //

        RtlZeroMemory(&NullMessage.LmChallengeResponse,5*sizeof(STRING32));

        if (memcmp(&AuthenticateMessage->LmChallengeResponse,
                   &NullMessage.LmChallengeResponse,
                   sizeof(STRING32) * 5) ) {
            SecStatus = SEC_E_INVALID_TOKEN;
            SspPrint(( SSP_CRITICAL,
                  "SsprHandleAuthenticateMessage: "
                  "AuthenticateMessage->LmChallengeResponse is not zeroed\n"));
        }
        else
        {
            *ContextAttributes = SSP_RET_REAUTHENTICATION;
            SecStatus = STATUS_SUCCESS;
        }
        goto Cleanup;
    }

    //
    // If we are re-establishing a datagram context, get the negotiate flags
    // out of this message.
    //

    if ((Context->NegotiateFlags & NTLMSSP_NEGOTIATE_DATAGRAM) != 0) {

        if ((InputTokenSize < sizeof(AUTHENTICATE_MESSAGE)) ||
            ((AuthenticateMessage->NegotiateFlags &
              NTLMSSP_NEGOTIATE_DATAGRAM) == 0) ) {
            SecStatus = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }

        Context->NegotiateFlags = AuthenticateMessage->NegotiateFlags;

        //
        // always do key exchange with datagram if we need a key (for SIGN or SEAL)
        //

        if (Context->NegotiateFlags & (NTLMSSP_NEGOTIATE_SIGN | NTLMSSP_NEGOTIATE_SEAL))
        {
            Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_KEY_EXCH;
        }

        //
        // if got NTLM2, don't use LM_KEY
        //

        if ( (Context->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2) != 0 )
        {
            if( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_LM_KEY ) {
                SspPrint(( SSP_NEGOTIATE_FLAGS,
                      "SsprHandleAuthenticateMessage: "
                      "AuthenticateMessage (datagram) NTLM2 caused LM_KEY to be disabled.\n" ));
            }

            Context->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_LM_KEY;
        }

    }


    //
    // Check that client asked for minimum security required.
    // not done for legacy down-level case, as, NegotiateFlags are
    // constructed from incomplete information.
    //

    if( !fCallFromSrv )
    {
        if (!SsprCheckMinimumSecurity(
                    AuthenticateMessage->NegotiateFlags,
                    NtLmGlobalMinimumServerSecurity)) {

            SecStatus = SEC_E_UNSUPPORTED_FUNCTION;
            SspPrint(( SSP_CRITICAL,
                      "SsprHandleAuthenticateMessage: "
                      "client didn't support minimum security requirements.\n" ));
            goto Cleanup;
        }

    }

    //
    // Convert relative pointers to absolute.
    //

    DoUnicode = ( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_UNICODE ) != 0;

    if (!SspConvertRelativeToAbsolute(AuthenticateMessage,
                                      InputTokenSize,
                                      &AuthenticateMessage->LmChallengeResponse,
                                      (PSTRING) &LmChallengeResponse,
                                      FALSE,     // No special alignment
                                      TRUE ) ) { // NULL OK
        SecStatus = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    if (!SspConvertRelativeToAbsolute(AuthenticateMessage,
                                      InputTokenSize,
                                      &AuthenticateMessage->NtChallengeResponse,
                                      (PSTRING) &NtChallengeResponse,
                                      FALSE,     // No special alignment
                                      TRUE ) ) { // NULL OK
        SecStatus = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    if (!SspConvertRelativeToAbsolute(AuthenticateMessage,
                                      InputTokenSize,
                                      &AuthenticateMessage->DomainName,
                                      &DomainName2,
                                      DoUnicode, // Unicode alignment
                                      TRUE ) ) { // NULL OK
        SecStatus = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    if ( !SspConvertRelativeToAbsolute( AuthenticateMessage,
                                        InputTokenSize,
                                        &AuthenticateMessage->UserName,
                                        &UserName2,
                                        DoUnicode, // Unicode alignment
#ifdef notdef

        //
        // Allow null sessions.  The server should guard against them if
        // it doesn't want them.
        //
                                        FALSE )) { // User name cannot be NULL

#endif // notdef
                                        TRUE ) ) { // NULL OK
        SecStatus = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    if ( !SspConvertRelativeToAbsolute( AuthenticateMessage,
                                        InputTokenSize,
                                        &AuthenticateMessage->Workstation,
                                        &Workstation2,
                                        DoUnicode, // Unicode alignment
                                        TRUE ) ) { // NULL OK
        SecStatus = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    //
    // If this is datagram, get the session key
    //

///    if ((Context->NegotiateFlags & NTLMSSP_NEGOTIATE_DATAGRAM) != 0) {
    if ((Context->NegotiateFlags & NTLMSSP_NEGOTIATE_KEY_EXCH) != 0) {

        if ( !SspConvertRelativeToAbsolute( AuthenticateMessage,
                                            InputTokenSize,
                                            &AuthenticateMessage->SessionKey,
                                            &SessionKeyString,
                                            FALSE, // No special alignment
                                            TRUE) ) { // NULL OK
            SecStatus = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }

        //
        // It should be NULL if this is a local call
        //

        if (((Context->NegotiateFlags & NTLMSSP_NEGOTIATE_LOCAL_CALL) == 0) &&
            (SessionKeyString.Buffer == NULL)) {
            SecStatus = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }

        if(Context->NegotiateFlags & NTLMSSP_NEGOTIATE_LOCAL_CALL) {
            static const UCHAR FixedSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH] = {
                        'S', 'y', 's', 't', 'e', 'm', 'L', 'i',
                        'b', 'r', 'a', 'r', 'y', 'D', 'T', 'C'
                        };

            RtlCopyMemory(Context->SessionKey, FixedSessionKey, MSV1_0_USER_SESSION_KEY_LENGTH);
        }

    }

    //
    // Convert the domainname/user name/workstation to the right character set.
    //

    if ( DoUnicode ) {

        DomainName = *((PUNICODE_STRING) &DomainName2);
        UserName = *((PUNICODE_STRING) &UserName2);
        Workstation = *((PUNICODE_STRING) &Workstation2);

    } else {

        SspPrint(( SSP_API_MORE, "SsprHandleAuthenticateMessage: Not doing Unicode\n"));
        Status = RtlOemStringToUnicodeString(
                    &DomainName,
                    &DomainName2,
                    TRUE);

        if ( !NT_SUCCESS(Status) ) {
            SecStatus = SspNtStatusToSecStatus( Status,
                                                SEC_E_INSUFFICIENT_MEMORY );
            goto Cleanup;
        }

        Status = RtlOemStringToUnicodeString(
                    &UserName,
                    &UserName2,
                    TRUE);

        if ( !NT_SUCCESS(Status) ) {
            SecStatus = SspNtStatusToSecStatus( Status,
                                                SEC_E_INSUFFICIENT_MEMORY );
            goto Cleanup;
        }

        Status = RtlOemStringToUnicodeString(
                    &Workstation,
                    &Workstation2,
                    TRUE);

        if ( !NT_SUCCESS(Status) ) {
            SecStatus = SspNtStatusToSecStatus( Status,
                                                SEC_E_INSUFFICIENT_MEMORY );
            goto Cleanup;
        }

    }

    //
    // If the client is on the same machine as we are, just
    // use the token the client has already placed in our context structure,
    //

    if ( (Context->NegotiateFlags & NTLMSSP_NEGOTIATE_LOCAL_CALL ) &&
         Context->TokenHandle != NULL &&
         DomainName.Length == 0 &&
         UserName.Length == 0 &&
         Workstation.Length == 0 &&
         AuthenticateMessage->NtChallengeResponse.Length == 0 &&
         AuthenticateMessage->LmChallengeResponse.Length == 0 ) {

        static const UCHAR FixedSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH] = {
                    'S', 'y', 's', 't', 'e', 'm', 'L', 'i',
                    'b', 'r', 'a', 'r', 'y', 'D', 'T', 'C'
                    };

        LocalTokenHandle = Context->TokenHandle;
        Context->TokenHandle = NULL;

        KickOffTime.HighPart = 0x7FFFFFFF;
        KickOffTime.LowPart = 0xFFFFFFFF;

        RtlCopyMemory(Context->SessionKey, FixedSessionKey, MSV1_0_USER_SESSION_KEY_LENGTH);
        SspPrint(( SSP_MISC, "SsprHandleAuthenticateMessage: Local Call\n"));

    //
    // If the client is on a different machine than we are,
    //  use LsaLogonUser to create a token for the client.
    //

    } else {

        //
        //  Store user name and domain name
        //

        SecStatus = NtLmDuplicateUnicodeString(
                          &Context->UserName,
                          &UserName);
        if (!NT_SUCCESS(SecStatus)) {
            goto Cleanup;
        }

        SecStatus = NtLmDuplicateUnicodeString(
                          &Context->DomainName,
                          &DomainName);
        if (!NT_SUCCESS(SecStatus)) {
            goto Cleanup;
        }


        //
        // Allocate an MSV1_0 network logon message
        //

        if (!fSubAuth)
        {

            MsvLogonMessageSize =
                sizeof(*MsvLogonMessage) +
                DomainName.Length +
                UserName.Length +
                Workstation.Length +
                AuthenticateMessage->NtChallengeResponse.Length +
                AuthenticateMessage->LmChallengeResponse.Length;

            MsvLogonMessage = (PMSV1_0_LM20_LOGON)
                          NtLmAllocate(MsvLogonMessageSize );

            if ( MsvLogonMessage == NULL ) {
                SecStatus = STATUS_NO_MEMORY;
                SspPrint(( SSP_CRITICAL,
                  "SsprHandleAuthenticateMessage: Error allocating MsvLogonMessage"));
                goto Cleanup;
            }

            //
            // Build the MSV1_0 network logon message to pass to the LSA.
            //

            MsvLogonMessage->MessageType = MsV1_0NetworkLogon;

            Where = (PCHAR)(MsvLogonMessage+1);

            SspContextCopyStringAbsolute( MsvLogonMessage,
                              (PSTRING)&MsvLogonMessage->LogonDomainName,
                              (PSTRING)&DomainName,
                              &Where );

            SspContextCopyStringAbsolute( MsvLogonMessage,
                              (PSTRING)&MsvLogonMessage->UserName,
                              (PSTRING)&UserName,
                              &Where );

            SspContextCopyStringAbsolute( MsvLogonMessage,
                              (PSTRING)&MsvLogonMessage->Workstation,
                              (PSTRING)&Workstation,
                              &Where );

            RtlCopyMemory( MsvLogonMessage->ChallengeToClient,
                       Context->Challenge,
                       sizeof( MsvLogonMessage->ChallengeToClient ) );

            SspContextCopyStringAbsolute( MsvLogonMessage,
                              &MsvLogonMessage->CaseSensitiveChallengeResponse,
                              &NtChallengeResponse,
                              &Where );

            SspContextCopyStringAbsolute(MsvLogonMessage,
                             &MsvLogonMessage->CaseInsensitiveChallengeResponse,
                             &LmChallengeResponse,
                             &Where );

            MsvLogonMessage->ParameterControl = MSV1_0_ALLOW_SERVER_TRUST_ACCOUNT;

            // This is required by the pre 4.0 server
            if (fCallFromSrv)
            {
                MsvLogonMessage->ParameterControl = MSV1_0_CLEARTEXT_PASSWORD_ALLOWED | NtLmAuthenticateMessage->ParameterControl;
            }

            if ( (Context->ContextFlags & ASC_RET_ALLOW_NON_USER_LOGONS ) != 0) {
                MsvLogonMessage->ParameterControl |= MSV1_0_ALLOW_WORKSTATION_TRUST_ACCOUNT;
            }

            //
            // Get the profile path for EFS
            //

            MsvLogonMessage->ParameterControl |= MSV1_0_RETURN_PROFILE_PATH;

            //
            // By passing in the RETURN_PASSWORD_EXPIRY flag, the password
            // expiration time is returned in the logoff time
            //

            MsvLogonMessage->ParameterControl |= MSV1_0_RETURN_PASSWORD_EXPIRY;

        }
        else
        {

            MsvSubAuthLogonMessageSize =
                sizeof(*MsvSubAuthLogonMessage) +
                DomainName.Length +
                UserName.Length +
                Workstation.Length +
                AuthenticateMessage->NtChallengeResponse.Length +
                AuthenticateMessage->LmChallengeResponse.Length;

            MsvSubAuthLogonMessage = (PMSV1_0_SUBAUTH_LOGON)
                          NtLmAllocate(MsvSubAuthLogonMessageSize );

            if ( MsvSubAuthLogonMessage == NULL ) {
                SecStatus = STATUS_NO_MEMORY;
                SspPrint(( SSP_CRITICAL,
                  "SsprHandleAuthenticateMessage: Error allocating MsvSubAuthLogonMessage"));
                goto Cleanup;
            }

            //
            // Build the MSV1_0 subauth logon message to pass to the LSA.
            //

            MsvSubAuthLogonMessage->MessageType = MsV1_0SubAuthLogon;

            Where = (PCHAR)(MsvSubAuthLogonMessage+1);

            SspContextCopyStringAbsolute( MsvSubAuthLogonMessage,
                              (PSTRING)&MsvSubAuthLogonMessage->LogonDomainName,
                              (PSTRING)&DomainName,
                              &Where );

            SspContextCopyStringAbsolute( MsvSubAuthLogonMessage,
                              (PSTRING)&MsvSubAuthLogonMessage->UserName,
                              (PSTRING)&UserName,
                              &Where );

            SspContextCopyStringAbsolute( MsvSubAuthLogonMessage,
                              (PSTRING)&MsvSubAuthLogonMessage->Workstation,
                              (PSTRING)&Workstation,
                              &Where );

            RtlCopyMemory( MsvSubAuthLogonMessage->ChallengeToClient,
                       Context->Challenge,
                       sizeof( MsvSubAuthLogonMessage->ChallengeToClient ) );

            SspContextCopyStringAbsolute( MsvSubAuthLogonMessage,
                              &MsvSubAuthLogonMessage->AuthenticationInfo1,
                              &LmChallengeResponse,
                              &Where );

            SspContextCopyStringAbsolute(MsvSubAuthLogonMessage,
                             &MsvSubAuthLogonMessage->AuthenticationInfo2,
                             &NtChallengeResponse,
                             &Where );

            MsvSubAuthLogonMessage->ParameterControl = MSV1_0_ALLOW_SERVER_TRUST_ACCOUNT;

            MsvSubAuthLogonMessage->SubAuthPackageId = SubAuthPackageId;

            // This is required by the pre 4.0 server
            if (fCallFromSrv)
            {
                MsvSubAuthLogonMessage->ParameterControl = MSV1_0_CLEARTEXT_PASSWORD_ALLOWED | NtLmAuthenticateMessage->ParameterControl;
            }

            if ( (Context->ContextFlags & ASC_RET_ALLOW_NON_USER_LOGONS ) != 0) {
                MsvSubAuthLogonMessage->ParameterControl |= MSV1_0_ALLOW_WORKSTATION_TRUST_ACCOUNT;
            }

            //
            // By passing in the RETURN_PASSWORD_EXPIRY flag, the password
            // expiration time is returned in the logoff time
            //

            MsvSubAuthLogonMessage->ParameterControl |= MSV1_0_RETURN_PASSWORD_EXPIRY;
        }


        //
        // if NTLM2 is negotiated, then mix my challenge with the client's...
        // But, special case for null sessions. since we already negotiated
        // NTLM2, but the LmChallengeResponse field is actually used
        // here. REVIEW -- maybe don't negotiate NTLM2 if NULL session
        //

        if((Context->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM2)  &&
           (AuthenticateMessage->LmChallengeResponse.Length >= MSV1_0_CHALLENGE_LENGTH))
        {
            MsvLogonMessage->ParameterControl |= MSV1_0_USE_CLIENT_CHALLENGE;
        }


        //
        // Log this user on.
        //

        // No origin (could use F(workstaion))

        RtlInitString( &OriginName, NULL );

        strncpy( SourceContext.SourceName,
                 "NtLmSsp ",
                 sizeof(SourceContext.SourceName) );

        RtlZeroMemory( &SourceContext.SourceIdentifier,
                       sizeof(SourceContext.SourceIdentifier) );

        {
            PVOID TokenInformation;
            LSA_TOKEN_INFORMATION_TYPE TokenInformationType;
            LSA_TOKEN_INFORMATION_TYPE OriginalTokenType;
            SECPKG_PRIMARY_CRED PrimaryCredentials;
            PSECPKG_SUPPLEMENTAL_CRED_ARRAY Credentials = NULL;
            PPRIVILEGE_SET PrivilegesAssigned = NULL;

            RtlZeroMemory(
                &PrimaryCredentials,
                sizeof(SECPKG_PRIMARY_CRED)
                );

            if (!fSubAuth)
            {
                Status = LsaApLogonUserEx2(
                    (PLSA_CLIENT_REQUEST) (-1),
                    Network,
                    MsvLogonMessage,
                    MsvLogonMessage,
                    MsvLogonMessageSize,
                    (PVOID *) &LogonProfileMessage,
                    &LogonProfileMessageSize,
                    &LogonId,
                    &SubStatus,
                    &TokenInformationType,
                    &TokenInformation,
                    &AccountName,
                    &AuthenticatingAuthority,
                    &WorkstationName,
                    &PrimaryCredentials,
                    &Credentials
                    );
            }
            else
            {
                Status = LsaApLogonUserEx2(
                    (PLSA_CLIENT_REQUEST) (-1),
                    Network,
                    MsvSubAuthLogonMessage,
                    MsvSubAuthLogonMessage,
                    MsvSubAuthLogonMessageSize,
                    (PVOID *) &LogonProfileMessage,
                    &LogonProfileMessageSize,
                    &LogonId,
                    &SubStatus,
                    &TokenInformationType,
                    &TokenInformation,
                    &AccountName,
                    &AuthenticatingAuthority,
                    &WorkstationName,
                    &PrimaryCredentials,
                    &Credentials
                    );
            }

        if ( !NT_SUCCESS(Status) ) {
            SspPrint(( SSP_CRITICAL,
                      "SsprHandleAuthenticateMessage: "
                      "LsaApLogonUserEx2 returns 0x%lx for context 0x%x\n",
                      Status, Context ));
            SecStatus = SspNtStatusToSecStatus( Status, SEC_E_LOGON_DENIED );
            if (Status == STATUS_PASSWORD_MUST_CHANGE) {
                *ApiSubStatus = Status;
            }
            else if (Status == STATUS_ACCOUNT_RESTRICTION) {
                *ApiSubStatus = SubStatus;
            } else {
                *ApiSubStatus = Status;
            }

            goto Cleanup;
        }

        if ( !NT_SUCCESS(SubStatus) ) {
            SspPrint(( SSP_CRITICAL,
                      "SsprHandleAuthenticateMessage: "
                      "LsaApLogonUserEx2 returns SubStatus of 0x%lx\n",
                      SubStatus ));
            SecStatus = SspNtStatusToSecStatus( SubStatus, SEC_E_LOGON_DENIED );
            goto Cleanup;
        }

        //
        // Check if this was a null session. The TokenInformationType will
        // be LsaTokenInformationNull if it is. If so, we may need to fail
        // the logon.
        //

        if (TokenInformationType == LsaTokenInformationNull)
        {

//
// RESTRICT_NULL_SESSIONS deemed too risky because legacy behavior of package
// allows null sessions from SYSTEM.
//

#ifdef RESTRICT_NULL_SESSIONS
            if ((Context->ContextFlags & ASC_REQ_ALLOW_NULL_SESSION) == 0) {
                SspPrint(( SSP_CRITICAL,
                           "SsprHandleAuthenticateMessage: "
                           "Null session logon attempted but not allowed\n" ));
                SecStatus = SEC_E_LOGON_DENIED;
                goto Cleanup;
            }
#endif
            *ContextAttributes |= ASC_RET_NULL_SESSION;
            Context->ContextFlags |= ASC_RET_NULL_SESSION;
            Context->NegotiateFlags |= NTLMSSP_NEGOTIATE_NULL_SESSION;
        }
        else
        {
            PLSA_TOKEN_INFORMATION_V2 TokenInfoV2 ;

            TokenInfoV2 = (PLSA_TOKEN_INFORMATION_V2) TokenInformation ;
            AuditSid = (PSID) NtLmAllocate( RtlLengthSid( TokenInfoV2->User.User.Sid ) );

            if ( AuditSid )
            {

                RtlCopyMemory( AuditSid,
                               TokenInfoV2->User.User.Sid,
                               RtlLengthSid( TokenInfoV2->User.User.Sid ) );

            }
        }

        Status = LsaFunctions->CreateToken(
                    &LogonId,
                    &SourceContext,
                    Network,
                    (((Context->NegotiateFlags & NTLMSSP_NEGOTIATE_IDENTIFY) != 0) ?
                        SecurityIdentification : SecurityImpersonation),
                    TokenInformationType,
                    TokenInformation,
                    NULL,
                    AccountName,
                    AuthenticatingAuthority,
                    WorkstationName,
                    ((LogonProfileMessage->UserFlags & LOGON_PROFILE_PATH_RETURNED) != 0) ? &LogonProfileMessage->UserParameters : NULL,
                    &LocalTokenHandle,
                    &SubStatus);

        }  // end of block for LsaLogonUser

        if ( !NT_SUCCESS(Status) ) {
            SspPrint(( SSP_CRITICAL,
                      "SsprHandleAuthenticateMessage: "
                      "CreateToken returns 0x%lx\n",
                      Status ));
            SecStatus = Status;
            goto Cleanup;
        }

        if ( !NT_SUCCESS(SubStatus) ) {
            SspPrint(( SSP_CRITICAL,
                      "SsprHandleAuthenticateMessage: "
                      "CreateToken returns SubStatus of 0x%lx\n",
                      SubStatus ));
            SecStatus = SubStatus;
            goto Cleanup;
        }

        LocalTokenHandleOpenned = TRUE;

        //
        // Don't allow cleartext password on the logon.
        // Except if called from Downlevel

        if (!fCallFromSrv)
        {
            if ( LogonProfileMessage->UserFlags & LOGON_NOENCRYPTION ) {
                SspPrint(( SSP_CRITICAL,
                      "SsprHandleAuthenticateMessage: "
                      "LsaLogonUser used cleartext password\n" ));
                SecStatus = SEC_E_LOGON_DENIED;
                goto Cleanup;

            }
        }

        //
        // If we did a guest logon, set the substatus to be STATUS_NO_SUCH_USER
        //

        if ( LogonProfileMessage->UserFlags & LOGON_GUEST ) {
            fAvoidGuestAudit = TRUE;
            *ApiSubStatus = STATUS_NO_SUCH_USER;

            //
            // If caller required Sign/Seal, fail them here
            //

            if (
//                (Context->NegotiateFlags & NTLMSSP_NEGOTIATE_SIGN) ||
                (Context->NegotiateFlags & NTLMSSP_NEGOTIATE_SEAL))
            {
                SspPrint(( SSP_CRITICAL,
                     "SsprHandleAuthenticateMessage: "
                      "LsaLogonUser logged user as a guest but sign seal is requested\n" ));
                SecStatus = SEC_E_LOGON_DENIED;
                goto Cleanup;
            }
        }

        //
        // Save important information about the caller.
        //

        KickOffTime = LogonProfileMessage->KickOffTime;

        //
        // By passing in the RETURN_PASSWORD_EXPIRY flag, the password
        // expiration time is returned in the logoff time
        //

        *PasswordExpiry = LogonProfileMessage->LogoffTime;
        *UserFlags = LogonProfileMessage->UserFlags;

        //
        // set the session key to what the client sent us (if anything)
        //

        if (Context->NegotiateFlags & NTLMSSP_NEGOTIATE_KEY_EXCH &&
            AuthenticateMessage->SessionKey.Length == MSV1_0_USER_SESSION_KEY_LENGTH)
        {
            RtlCopyMemory(
                Context->SessionKey,
                SessionKeyString.Buffer,
                MSV1_0_USER_SESSION_KEY_LENGTH
                );
        }

        //
        // Generate the session key, or decrypt the generated random one sent to
        // us by the client, from various bits of info
        //

        // BUGBUG - should doublecheck LsaLogonUserEx2() actually did a NTLMv2
        // logon before assuming that is the session key type.

        SecStatus = SsprMakeSessionKey(
                            Context,
                            &LmChallengeResponse,
                            LogonProfileMessage->UserSessionKey,
                            LogonProfileMessage->LanmanSessionKey,
                            NULL
                            );

        if ( !NT_SUCCESS(SecStatus) ) {
            SspPrint(( SSP_CRITICAL,
                  "SsprHandleAuthenticateMessage: "
                  "SsprMakeSessionKey failed.\n" ));
            goto Cleanup;
        }



    }

    //
    // Copy the logon domain name returned by the LSA if it is different.
    // from the one the caller passed in. This may happen with temp duplicate
    // accounts and local account
    //

    if ((LogonProfileMessage != NULL) &&
        (LogonProfileMessage->LogonDomainName.Length != 0) &&
        !RtlEqualUnicodeString(
                    &Context->DomainName,
                    &LogonProfileMessage->LogonDomainName,
                    TRUE               // case insensitive
                    )) {
        //
        // erase the old domain name
        //

        if (Context->DomainName.Buffer != NULL) {
            NtLmFree(Context->DomainName.Buffer);
            Context->DomainName.Buffer = NULL;
        }

        SecStatus = NtLmDuplicateUnicodeString(
                        &Context->DomainName,
                        &LogonProfileMessage->LogonDomainName
                        );

        if (!NT_SUCCESS(SecStatus)) {
            goto Cleanup;
        }

    }

    //
    // Allow the context to live until kickoff time.
    //

    SspContextSetTimeStamp( Context, KickOffTime );

    //
    // Return output parameters to the caller.
    //

    *ExpirationTime = SspContextGetTimeStamp( Context, TRUE );

    //
    // Return output token
    //

    if (fCallFromSrv)
    {
        NtLmAcceptResponse = (PNTLM_ACCEPT_RESPONSE) *OutputToken;
        if (NtLmAcceptResponse == NULL)
        {
            SecStatus = SEC_E_INSUFFICIENT_MEMORY;
            goto Cleanup;
        }

        LUID UNALIGNED * TempLogonId = (LUID UNALIGNED *) &NtLmAcceptResponse->LogonId;
        *TempLogonId = LogonId;
        NtLmAcceptResponse->UserFlags = LogonProfileMessage->UserFlags;

        RtlCopyMemory(
            NtLmAcceptResponse->UserSessionKey,
            LogonProfileMessage->UserSessionKey,
            MSV1_0_USER_SESSION_KEY_LENGTH
            );

        RtlCopyMemory(
            NtLmAcceptResponse->LanmanSessionKey,
            LogonProfileMessage->LanmanSessionKey,
            MSV1_0_LANMAN_SESSION_KEY_LENGTH
            );

        LARGE_INTEGER UNALIGNED *TempKickoffTime = (LARGE_INTEGER UNALIGNED *) &NtLmAcceptResponse->KickoffTime;
        *TempKickoffTime = LogonProfileMessage->KickOffTime;

    }
    else
    {
        *OutputTokenSize = 0;
    }


    //
    // We only support replay and sequence detect options
    //

    if ( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_SIGN ) {
        *ContextAttributes |= ASC_RET_REPLAY_DETECT |
                              ASC_RET_SEQUENCE_DETECT |
                              ASC_RET_INTEGRITY;
    }

    if ( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_SEAL ) {
        *ContextAttributes |= ASC_RET_CONFIDENTIALITY;
    }

    if( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_IDENTIFY ) {
        *ContextAttributes |= ASC_RET_IDENTIFY;
    }

    if( Context->NegotiateFlags & NTLMSSP_NEGOTIATE_DATAGRAM ) {
        *ContextAttributes |= ASC_RET_DATAGRAM;
    }

    if ( ContextReqFlags & ASC_REQ_REPLAY_DETECT ) {
        *ContextAttributes |= ASC_RET_REPLAY_DETECT;
    }

    if ( ContextReqFlags & ASC_REQ_SEQUENCE_DETECT ) {
        *ContextAttributes |= ASC_RET_SEQUENCE_DETECT;
    }

    if ( ContextReqFlags & ASC_REQ_ALLOW_NON_USER_LOGONS ) {
        *ContextAttributes |= ASC_RET_ALLOW_NON_USER_LOGONS;
    }


    SecStatus = STATUS_SUCCESS;

    //
    // Free and locally used resources.
    //

Cleanup:

    //
    // Audit this logon
    //

    if (NT_SUCCESS(SecStatus)) {

        //
        // If we don't have an account name, this was a local connection
        // and we didn't build a new token, so don't bother auditing.
        // also, don't bother auditing logons that fellback to guest.
        //

        if ( (AccountName != NULL) &&
             (AccountName->Length != 0) &&
              !fAvoidGuestAudit ) {

            LsaFunctions->AuditLogon(
                STATUS_SUCCESS,
                STATUS_SUCCESS,
                AccountName,
                AuthenticatingAuthority,
                WorkstationName,
                AuditSid,
                Network,
                &SourceContext,
                &LogonId
                );
        }
    } else {
        LsaFunctions->AuditLogon(
            !NT_SUCCESS(Status) ? Status : SecStatus,
            SubStatus,
            &UserName,
            &DomainName,
            &Workstation,
            NULL,
            Network,
            &SourceContext,
            &LogonId
            );

    }

    if ( Context != NULL ) {

        Context->Server = TRUE;
        Context->LastStatus = SecStatus;
        Context->DownLevel = fCallFromSrv;


        //
        // Don't allow this context to be used again.
        //

        if ( NT_SUCCESS(SecStatus) ) {
            Context->State = AuthenticatedState;

            if ( LocalTokenHandle ) {
                *TokenHandle = LocalTokenHandle;
            }

            LocalTokenHandle = NULL;

            RtlCopyMemory(
                SessionKey,
                Context->SessionKey,
                MSV1_0_USER_SESSION_KEY_LENGTH );

            *NegotiateFlags = Context->NegotiateFlags;

            //
            // if caller wants only INTEGRITY, then wants application
            // supplied sequence numbers...
            //

            if ((Context->ContextFlags &
                (ASC_REQ_INTEGRITY | ASC_REQ_REPLAY_DETECT | ASC_REQ_SEQUENCE_DETECT)) ==
                ASC_REQ_INTEGRITY)
            {
                *NegotiateFlags |= NTLMSSP_APP_SEQ;
            }

        } else {
            Context->State = IdleState;
        }

        // If we just created this context, then we need to dereference it
        // once more with feeling

        if (fCallFromSrv && !NT_SUCCESS(SecStatus))
        {
            PSSP_CONTEXT LocalContext;
            LocalContext = SspContextReferenceContext (*ContextHandle, TRUE);
            ASSERT (LocalContext != NULL);
            if (LocalContext != NULL)
            {
                SspContextDereferenceContext( LocalContext );
            }

        }
        SspContextDereferenceContext( Context );

    }

    if ( NegotiateMessage != NULL ) {
        (VOID) NtLmFree( NegotiateMessage );
    }

    if ( AuthenticateMessage != NULL ) {
        (VOID) NtLmFree( AuthenticateMessage );
    }

    if ( MsvLogonMessage != NULL ) {
        (VOID) NtLmFree( MsvLogonMessage );
    }

    if ( MsvSubAuthLogonMessage != NULL ) {
        (VOID) NtLmFree( MsvSubAuthLogonMessage );
    }


    if ( LogonProfileMessage != NULL ) {
        (VOID) LsaFunctions->FreeLsaHeap( LogonProfileMessage );
    }

    if ( LocalTokenHandle != NULL && LocalTokenHandleOpenned ) {
        (VOID) NtClose( LocalTokenHandle );
    }

    if ( !DoUnicode ) {
        if ( DomainName.Buffer != NULL) {
            RtlFreeUnicodeString( &DomainName );
        }
        if ( UserName.Buffer != NULL) {
            RtlFreeUnicodeString( &UserName );
        }
        if ( Workstation.Buffer != NULL) {
            RtlFreeUnicodeString( &Workstation );
        }
    }

    if (AccountName != NULL) {
        if (AccountName->Buffer != NULL) {
            LsaFunctions->FreeLsaHeap(AccountName->Buffer);
        }
        LsaFunctions->FreeLsaHeap(AccountName);
    }
    if (AuthenticatingAuthority != NULL) {
        if (AuthenticatingAuthority->Buffer != NULL) {
            LsaFunctions->FreeLsaHeap(AuthenticatingAuthority->Buffer);
        }
        LsaFunctions->FreeLsaHeap(AuthenticatingAuthority);
    }
    if (WorkstationName != NULL) {
        if (WorkstationName->Buffer != NULL) {
            LsaFunctions->FreeLsaHeap(WorkstationName->Buffer);
        }
        LsaFunctions->FreeLsaHeap(WorkstationName);
    }

    if ( AuditSid )
    {
        NtLmFree( AuditSid );
    }

    //
    // Set a flag telling RPC not to destroy the connection yet
    //

    if (!NT_SUCCESS(SecStatus)) {
        *ContextAttributes |= ASC_RET_THIRD_LEG_FAILED;
    }


    SspPrint(( SSP_API_MORE, "Leaving SsprHandleAutheticateMessage: 0x%lx\n", SecStatus ));
    return SecStatus;
}

NTSTATUS
SsprDeleteSecurityContext (
    IN OUT ULONG_PTR ContextHandle
    )

/*++

Routine Description:

    Deletes the local data structures associated with the specified
    security context and generates a token which is passed to a remote peer
    so it too can remove the corresponding security context.

    This API terminates a context on the local machine, and optionally
    provides a token to be sent to the other machine.  The OutputToken
    generated by this call is to be sent to the remote peer (initiator or
    acceptor).  If the context was created with the I _REQ_ALLOCATE_MEMORY
    flag, then the package will allocate a buffer for the output token.
    Otherwise, it is the responsibility of the caller.

Arguments:

    ContextHandle - Handle to the context to delete

Return Value:

    STATUS_SUCCESS - Call completed successfully

    SEC_E_NO_SPM -- Security Support Provider is not running
    SEC_E_INVALID_HANDLE -- Credential/Context Handle is invalid

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PSSP_CONTEXT Context = NULL;

    //
    // Initialization
    //

    SspPrint(( SSP_API_MORE, "SspDeleteSecurityContext Entered\n" ));

    //
    // Find the currently existing context (and delink it).
    //

    Context = SspContextReferenceContext( ContextHandle, TRUE );

    //
    // If the context is a server context, return SEC_I_CALL_NTLMSSP_SERVICE.
    // If there is also a context here, delete that first.
    //

    if ( Context == NULL ) {

        Status = STATUS_INVALID_HANDLE;

    } else {

        SspContextDereferenceContext( Context );
    }

    SspPrint(( SSP_API_MORE, "SspDeleteSecurityContext returns 0x%lx\n", Status));
    return Status;
}

NTSTATUS
SspContextInitialize(
    VOID
    )

/*++

Routine Description:

    This function initializes this module.

Arguments:

    None.

Return Value:

    Status of the operation.

--*/

{

    //
    // Initialize the Context list to be empty.
    //

    InitializeCriticalSection(&SspContextCritSect);
    InitializeListHead( &SspContextList );

    return STATUS_SUCCESS;

}

VOID
SspContextTerminate(
    VOID
    )

/*++

Routine Description:

    This function cleans up any dangling Contexts.

Arguments:

    None.

Return Value:

    Status of the operation.

--*/
{
    //
    // Drop any lingering Contexts
    //

    EnterCriticalSection( &SspContextCritSect );

    while ( !IsListEmpty( &SspContextList ) ) {
        ULONG_PTR ContextHandle;
        PSSP_CONTEXT Context;

        ContextHandle =
            (LSA_SEC_HANDLE) CONTAINING_RECORD( SspContextList.Flink,
                                      SSP_CONTEXT,
                                      Next );

        LeaveCriticalSection( &SspContextCritSect );

        // Remove Context

        Context = SspContextReferenceContext( ContextHandle, TRUE);

        if ( Context != NULL ) {
            SspContextDereferenceContext(Context);
        }

        EnterCriticalSection( &SspContextCritSect );
    }
    LeaveCriticalSection( &SspContextCritSect );

    //
    // Delete the critical section
    //

    DeleteCriticalSection(&SspContextCritSect);

    return;
}
