/*++

Copyright (c) 1987-1999  Microsoft Corporation

Module Name:

    msvsam.c

Abstract:

    Sam account validation interface.

    These routines are shared by the MSV authentication package and
    the Netlogon service.

Author:

    Cliff Van Dyke (cliffv) 15-Jan-1992

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:
    Chandana Surlu         21-Jul-96      Stolen from \\kernel\razzle3\src\security\msv1_0\msvsam.c

--*/

#include <global.h>
#undef EXTERN

#include "msp.h"
#include "nlp.h"
#include <stddef.h>     // offsetof()
#include <msaudite.h>   // SE_AUDITID_xxx



///////////////////////////////////////////////////////////////////////
//                                                                   //
// SubAuth package zero helper routine                               //
//                                                                   //
///////////////////////////////////////////////////////////////////////


NTSTATUS
Msv1_0SubAuthenticationRoutineZero(
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PVOID LogonInformation,
    IN ULONG Flags,
    IN PUSER_ALL_INFORMATION UserAll,
    OUT PULONG WhichFields,
    OUT PULONG UserFlags,
    OUT PBOOLEAN Authoritative,
    OUT PLARGE_INTEGER LogoffTime,
    OUT PLARGE_INTEGER KickoffTime
    );


BOOLEAN
MsvpLm3ValidateResponse (
    IN PNT_OWF_PASSWORD pNtOwfPassword,
    IN PUNICODE_STRING pUserName,
    IN PUNICODE_STRING pLogonDomainName,
    IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    IN PMSV1_0_LM3_RESPONSE pLm3Response
    )
{
    UCHAR Response[MSV1_0_NTLM3_RESPONSE_LENGTH];
    ULONG i;

    // compute the response again

    MsvpLm3Response (
                pNtOwfPassword,
                pUserName,
                pLogonDomainName,
                ChallengeToClient,
                pLm3Response,
                Response
                );

    // compare with what we were passed
    i = (ULONG)RtlCompareMemory(
                pLm3Response->Response,
                Response,
                MSV1_0_NTLM3_RESPONSE_LENGTH);

    return (i == MSV1_0_NTLM3_RESPONSE_LENGTH);
}


BOOLEAN
MsvpNtlm3ValidateResponse (
    IN PNT_OWF_PASSWORD pNtOwfPassword,
    IN PUNICODE_STRING pUserName,
    IN PUNICODE_STRING pLogonDomainName,
    IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    IN PMSV1_0_NTLM3_RESPONSE pNtlm3Response,
    IN ULONG Ntlm3ResponseLength,
    OUT PUSER_SESSION_KEY UserSessionKey,
    OUT PLM_SESSION_KEY LmSessionKey
    )
{
    UCHAR Response[MSV1_0_NTLM3_RESPONSE_LENGTH];
    ULONG i;
    LARGE_INTEGER Time;
    NTSTATUS Status;
    const LONGLONG TicksPerSecond = 10*1000*1000;    // 100 ns ticks per second

    // check version numbers

    if (pNtlm3Response->RespType > 1 ||
        pNtlm3Response->HiRespType < 1)
        return FALSE;

    // check that the timestamp isn't too old
    Status = NtQuerySystemTime ( &Time );
    ASSERT( NT_SUCCESS(Status) );

#ifndef USE_CONSTANT_CHALLENGE
    // make sure time hasn't expired
    // don't forget that client's clock could be behind ours
    if (Time.QuadPart > (LONGLONG)pNtlm3Response->TimeStamp) {
        if (Time.QuadPart - (LONGLONG)pNtlm3Response->TimeStamp >
            (MSV1_0_MAX_NTLM3_LIFE*TicksPerSecond))
            return FALSE;
    } else if ((LONGLONG)pNtlm3Response->TimeStamp - Time.QuadPart >
        (MSV1_0_MAX_NTLM3_LIFE*TicksPerSecond)) {
        return FALSE;
    }
#endif

    // compute the response itself

    MsvpNtlm3Response (
                pNtOwfPassword,
                pUserName,
                pLogonDomainName,
                (Ntlm3ResponseLength-sizeof(MSV1_0_NTLM3_RESPONSE)),
                ChallengeToClient,
                pNtlm3Response,
                Response,
                UserSessionKey,
                LmSessionKey
                );

    // compare with what we were passed
    i = (ULONG)RtlCompareMemory(
                pNtlm3Response->Response,
                Response,
                (size_t)MSV1_0_NTLM3_RESPONSE_LENGTH);

    return (i == MSV1_0_NTLM3_RESPONSE_LENGTH);
}



BOOLEAN
MsvpPasswordValidate (
    IN BOOLEAN UasCompatibilityRequired,
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PVOID LogonInformation,
    IN PUSER_INTERNAL1_INFORMATION Passwords,
    OUT PULONG UserFlags,
    OUT PUSER_SESSION_KEY UserSessionKey,
    OUT PLM_SESSION_KEY LmSessionKey
)
/*++

Routine Description:

    Process an interactive, network, or session logon.  It calls
    SamIUserValidation, validates the passed in credentials, updates the logon
    statistics and packages the result for return to the caller.

    This routine is called directly from the MSV Authentication package
    on any system where LanMan is not installed.  This routine is called
    from the Netlogon Service otherwise.

Arguments:

    UasCompatibilityRequired -- True, if UAS compatibility is required.

    LogonLevel -- Specifies the level of information given in
        LogonInformation.

    LogonInformation -- Specifies the description for the user
        logging on.  The LogonDomainName field should be ignored.
        The caller is responsible for validating this field.

    Passwords -- Specifies the passwords for the user account.

    UserFlags -- Returns flags identifying how the password was validated.
        Returns LOGON_NOENCRYPTION if the password wasn't encrypted
        Returns LOGON_USED_LM_PASSWORD if the LM password from SAM was used.

    UserSessionKey -- Returns the NT User session key for this network logon
        session.

    LmSessionKey -- Returns the LM compatible session key for this network
        logon session.

Return Value:

    TRUE -- Password validation is successful
    FALSE -- Password validation failed

--*/
{
    NTSTATUS Status;

    PNETLOGON_LOGON_IDENTITY_INFO LogonInfo;
    PNETLOGON_INTERACTIVE_INFO LogonInteractiveInfo;
    PNETLOGON_NETWORK_INFO LogonNetworkInfo;
    BOOLEAN AlreadyValidated = FALSE;
    UNICODE_STRING NullUnicodeString;

    ULONG NtLmProtocolSupported;

    //
    // Initialization.
    //

    LogonInfo = (PNETLOGON_LOGON_IDENTITY_INFO) LogonInformation;
    *UserFlags = LOGON_NTLMV2_ENABLED;
    RtlZeroMemory( UserSessionKey, sizeof(*UserSessionKey) );
    RtlZeroMemory( LmSessionKey, sizeof(*LmSessionKey) );
    RtlInitUnicodeString( &NullUnicodeString, NULL );

    //
    // Ensure the OWF password is always defined
    //

    if ( !Passwords->NtPasswordPresent ){
        RtlCopyMemory( &Passwords->NtOwfPassword,
                       &NlpNullNtOwfPassword,
                       sizeof(Passwords->NtOwfPassword) );
    }

    if ( !Passwords->LmPasswordPresent ){
        RtlCopyMemory( &Passwords->LmOwfPassword,
                       &NlpNullLmOwfPassword,
                       sizeof(Passwords->LmOwfPassword) );
    }

    //
    // Handle interactive/service validation.
    //
    // Simply compare the OWF password passed in with the one from the
    // SAM database.
    //

    switch ( LogonLevel ) {
    case NetlogonInteractiveInformation:
    case NetlogonServiceInformation:

        ASSERT( offsetof( NETLOGON_INTERACTIVE_INFO, LmOwfPassword)
            ==  offsetof( NETLOGON_SERVICE_INFO, LmOwfPassword) );
        ASSERT( offsetof( NETLOGON_INTERACTIVE_INFO, NtOwfPassword)
            ==  offsetof( NETLOGON_SERVICE_INFO, NtOwfPassword) );

        LogonInteractiveInfo =
            (PNETLOGON_INTERACTIVE_INFO) LogonInformation;

        //
        // If we're in UasCompatibilityMode,
        //  and we don't have the NT password in SAM (but do have LM password),
        //  validate against the LM version of the password.
        //

        if ( UasCompatibilityRequired &&
             !Passwords->NtPasswordPresent &&
             Passwords->LmPasswordPresent ) {

            if ( RtlCompareMemory( &Passwords->LmOwfPassword,
                                   &LogonInteractiveInfo->LmOwfPassword,
                                   LM_OWF_PASSWORD_LENGTH ) !=
                                   LM_OWF_PASSWORD_LENGTH ) {

                return FALSE;
            }
            *UserFlags |= LOGON_USED_LM_PASSWORD;

        //
        // In all other circumstances, use the NT version of the password.
        //  This enforces case sensitivity.
        //

        } else {

            if ( RtlCompareMemory( &Passwords->NtOwfPassword,
                                   &LogonInteractiveInfo->NtOwfPassword,
                                   NT_OWF_PASSWORD_LENGTH ) !=
                                   NT_OWF_PASSWORD_LENGTH ) {

                return FALSE;
            }
        }

        break;


    //
    // Handle network logon validation.
    //

    case NetlogonNetworkInformation:


        //
        // First, assume the passed password information is a challenge
        // response.
        //


        LogonNetworkInfo =
            (PNETLOGON_NETWORK_INFO) LogonInformation;

        // If the NT response is an NTLM3 response, do NTLM3 or NTLM3 with LM OWF
        //  if the length is > NT_RESPONSE_LENGTH, then it's an NTLM3 response

        if (LogonNetworkInfo->NtChallengeResponse.Length > NT_RESPONSE_LENGTH) {

            AlreadyValidated =  MsvpNtlm3ValidateResponse (
                &Passwords->NtOwfPassword,
                &LogonNetworkInfo->Identity.UserName,
                &LogonNetworkInfo->Identity.LogonDomainName,
                (PUCHAR)&LogonNetworkInfo->LmChallenge,
                (PMSV1_0_NTLM3_RESPONSE) LogonNetworkInfo->NtChallengeResponse.Buffer,
                LogonNetworkInfo->NtChallengeResponse.Length,
                UserSessionKey,
                LmSessionKey
                );

            //
            // because a Subauth may have been used, we will only return failure
            // here if we know the request was NTLMv2.
            //

            if( AlreadyValidated ||
                (LogonNetworkInfo->Identity.ParameterControl & MSV1_0_USE_CLIENT_CHALLENGE) ) {
                return AlreadyValidated;
            }
        }

        //
        // check the LM3 response based on NT OWF hash next
        //  this will be recieved from Win9x server with NTLMv2 client
        //

        if (LogonNetworkInfo->LmChallengeResponse.Length == NT_RESPONSE_LENGTH) {

            AlreadyValidated = MsvpLm3ValidateResponse (
                &Passwords->NtOwfPassword,
                &LogonNetworkInfo->Identity.UserName,
                &LogonNetworkInfo->Identity.LogonDomainName,
                (PUCHAR)&LogonNetworkInfo->LmChallenge,
                (PMSV1_0_LM3_RESPONSE) LogonNetworkInfo->LmChallengeResponse.Buffer
                );

            if (AlreadyValidated)
                return TRUE;
        }

        NtLmProtocolSupported = NtLmGlobalLmProtocolSupported;

        // if we're requiring all clients (Win9x and NT) to have been upgraded, fail out now
        if (NtLmProtocolSupported >= RefuseNtlm)
            return FALSE;

        // if that fails, check the NTLM response if there is one of the
        //  appropriate size in either NT response or LM response
        if (!AlreadyValidated &&
            (Passwords->NtPasswordPresent || (!Passwords->NtPasswordPresent && !Passwords->LmPasswordPresent)) &&
            (LogonNetworkInfo->NtChallengeResponse.Length == NT_RESPONSE_LENGTH ||
            LogonNetworkInfo->LmChallengeResponse.Length == NT_RESPONSE_LENGTH)) {

            NT_RESPONSE NtResponse;

            //
            // Compute what the response should be.
            //

            Status = RtlCalculateNtResponse(
                        &LogonNetworkInfo->LmChallenge,
                        &Passwords->NtOwfPassword,
                        &NtResponse );

            if ( NT_SUCCESS(Status) ) {

                //
                // If the responses match, the passwords are valid.
                //  Try the NT response first, then the LM response
                //

                if ( RtlCompareMemory(
                      LogonNetworkInfo->
                        NtChallengeResponse.Buffer,
                      &NtResponse,
                      LogonNetworkInfo->NtChallengeResponse.Length ) ==
                      NT_RESPONSE_LENGTH ) {

                    AlreadyValidated = TRUE;

                } else if ( RtlCompareMemory(
                      LogonNetworkInfo->
                        LmChallengeResponse.Buffer,
                      &NtResponse,
                      LogonNetworkInfo->LmChallengeResponse.Length ) ==
                      NT_RESPONSE_LENGTH ) {

                    AlreadyValidated = TRUE;
                }
            }
        }

        // if we're requiring all Win9x clients to have been upgraded, fail out now
        if (!AlreadyValidated && NtLmProtocolSupported >= RefuseLm)
            return FALSE;

        //
        //  if the LM response is the right size
        //  validate against the LM version of the response
        //  this applies also when both NTOWF and LMOWF are not present in SAM.
        //

        if (!AlreadyValidated &&
            ( LogonNetworkInfo->LmChallengeResponse.Length == LM_RESPONSE_LENGTH ) &&
            ( (Passwords->LmPasswordPresent) || (!Passwords->LmPasswordPresent && !Passwords->NtPasswordPresent) ) &&
            ( LogonNetworkInfo->NtChallengeResponse.Length < NT_RESPONSE_LENGTH )
            ) {

            LM_RESPONSE LmResponse;

            //
            // Compute what the response should be.
            //

            Status = RtlCalculateLmResponse(
                        &LogonNetworkInfo->LmChallenge,
                        &Passwords->LmOwfPassword,
                        &LmResponse );

            if ( NT_SUCCESS(Status) ) {

                //
                // If the responses match, the passwords are valid.
                //

                if ( RtlCompareMemory(
                      LogonNetworkInfo->
                        LmChallengeResponse.Buffer,
                      &LmResponse,
                      LM_RESPONSE_LENGTH ) ==
                      LM_RESPONSE_LENGTH ) {

                    AlreadyValidated = TRUE;
                    *UserFlags |= LOGON_USED_LM_PASSWORD;
                }
            }
        }

        //
        // If we haven't already validated this user,
        //  Validate a Cleartext password on a Network logon request.
        //

        if ( !AlreadyValidated ) {

            // If Cleartext passwords are not allowed,
            //  indicate the password doesn't match.
            //

            if((LogonInfo->ParameterControl & CLEARTEXT_PASSWORD_ALLOWED) == 0){
                return FALSE;
            }


            //
            // Compute the OWF password for the specified Cleartext password and
            // compare that to the OWF password retrieved from SAM.
            //

            //
            // If we're in UasCompatibilityMode,
            //  and we don't have the NT password in SAM or
            //      we don't have the NT password supplied by the caller.
            //  validate against the LM version of the password.
            //
            // if neither password are present, we validate against
            // the empty computed LMOWF.
            //

            if ( UasCompatibilityRequired &&
                 ((Passwords->LmPasswordPresent) || (!Passwords->LmPasswordPresent && !Passwords->NtPasswordPresent)) &&
                 (!Passwords->NtPasswordPresent ||
                 LogonNetworkInfo->NtChallengeResponse.Length == 0 ) ) {

                LM_OWF_PASSWORD LmOwfPassword;
                CHAR LmPassword[LM20_PWLEN+1];
                USHORT i;


                //
                // Compute the LmOwfPassword for the cleartext password passed in.
                //  (Enforce length restrictions on LanMan compatible passwords.)
                //

                if ( LogonNetworkInfo->LmChallengeResponse.Length >
                    sizeof(LmPassword) ) {
                    return FALSE;
                }

                RtlZeroMemory( &LmPassword, sizeof(LmPassword) );

                for (i = 0; i < LogonNetworkInfo->LmChallengeResponse.Length; i++) {
                    LmPassword[i] =
                      RtlUpperChar(LogonNetworkInfo->LmChallengeResponse.Buffer[i]);
                }

                (VOID) RtlCalculateLmOwfPassword( LmPassword, &LmOwfPassword );

                if ( RtlCompareMemory( &Passwords->LmOwfPassword,
                                       &LmOwfPassword,
                                       LM_OWF_PASSWORD_LENGTH ) !=
                                       LM_OWF_PASSWORD_LENGTH ) {

                    //
                    // Try the case preserved clear text password, too.
                    //  (I know of no client that does this,
                    //  but it is compatible with the LM 2.x server.)
                    //

                    RtlZeroMemory( &LmPassword, sizeof(LmPassword) );
                    RtlCopyMemory(
                        &LmPassword,
                        LogonNetworkInfo->LmChallengeResponse.Buffer,
                        LogonNetworkInfo->LmChallengeResponse.Length);

                    (VOID) RtlCalculateLmOwfPassword( LmPassword,
                                                      &LmOwfPassword );

                    if ( RtlCompareMemory( &Passwords->LmOwfPassword,
                                           &LmOwfPassword,
                                           LM_OWF_PASSWORD_LENGTH ) !=
                                           LM_OWF_PASSWORD_LENGTH ) {

                        return FALSE;
                    }

                }

                *UserFlags |= LOGON_USED_LM_PASSWORD;


            //
            // In all other circumstances, use the NT version of the password.
            //  This enforces case sensitivity.
            //

            } else {
                NT_OWF_PASSWORD NtOwfPassword;


                //
                // Compute the NtOwfPassword for the cleartext password passed in.
                //

                Status = RtlCalculateNtOwfPassword(
                             (PUNICODE_STRING)
                                &LogonNetworkInfo->NtChallengeResponse,
                             &NtOwfPassword );

                if ( RtlCompareMemory( &Passwords->NtOwfPassword,
                                       &NtOwfPassword,
                                       NT_OWF_PASSWORD_LENGTH ) !=
                                       NT_OWF_PASSWORD_LENGTH ) {

                    return FALSE;
                }
            }

            *UserFlags |= LOGON_NOENCRYPTION;
        }

        //
        // ASSERT: the network logon has been authenticated
        //
        //  Compute the session keys.

        //
        // If the client negotiated a non-NT protocol,
        //  use the lanman session key as the UserSessionKey.
        //

        if ( LogonNetworkInfo->NtChallengeResponse.Length == 0 ) {

            ASSERT( sizeof(*UserSessionKey) >= sizeof(*LmSessionKey) );

            RtlCopyMemory( UserSessionKey,
                           &Passwords->LmOwfPassword,
                           sizeof(*LmSessionKey) );

        } else {

            //
            // Return the NT UserSessionKey unless this is an account
            //  that doesn't have the NT version of the password.
            //  (A null password counts as a password).
            //

            if ( Passwords->NtPasswordPresent || !Passwords->LmPasswordPresent){

                Status = RtlCalculateUserSessionKeyNt(
                            (PNT_RESPONSE) NULL,    // Argument not used
                            &Passwords->NtOwfPassword,
                            UserSessionKey );

                ASSERT( NT_SUCCESS(Status) );
            }
        }

        //
        // Return the LM SessionKey unless this is an account
        //  that doesn't have the LM version of the password.
        //  (A null password counts as a password).
        //

        if ( Passwords->LmPasswordPresent || !Passwords->NtPasswordPresent ) {
            RtlCopyMemory( LmSessionKey,
                           &Passwords->LmOwfPassword,
                           sizeof(*LmSessionKey) );
        }

        break;

    //
    // Any other LogonLevel is an internal error.
    //
    default:
        return FALSE;

    }

    return TRUE;
}


BOOLEAN
MsvpEqualSidPrefix(
    IN PSID DomainSid,
    IN PSID GroupSid
    )
/*++

Routine Description:

    This routine checks to see if the specified group sid came from the
    specified domain by verifying that the domain portion of the group sid
    is equal to the domain sid.

Arguments:

    DomainSid - Sid of the domain for comparison.

    GroupSid - Sid of the group for comparison

Returns:

    TRUE - The group sid came from the specified domain.
    FALSE - The group sid did not come from the specified domain.

--*/
{
    PISID LocalGroupSid = (PISID) GroupSid;
    PISID LocalDomainSid = (PISID) DomainSid;
    if ((LocalGroupSid->SubAuthorityCount == LocalDomainSid->SubAuthorityCount + 1) &&
        RtlEqualMemory(
            RtlIdentifierAuthoritySid(LocalDomainSid),
            RtlIdentifierAuthoritySid(LocalGroupSid),
            RtlLengthRequiredSid(
                LocalDomainSid->SubAuthorityCount
                ) - FIELD_OFFSET(SID,IdentifierAuthority)
            )) {
        return(TRUE);
    }
    return(FALSE);
}

NTSTATUS
MsvpFilterGroupMembership(
    IN PSID_AND_ATTRIBUTES_LIST CompleteMembership,
    IN PSID LogonDomainId,
    OUT PSAMPR_GET_GROUPS_BUFFER LocalMembership,
    OUT PSID_AND_ATTRIBUTES_LIST GlobalMembership,
    OUT PULONG GlobalMembershipSize
    )
/*++

Routine Description:

    This routine separates the complete transitive group membership into
    portions from this domain and portions from others.

Arguments:

    CompleteMembership - The complete transitive membership.

    LogonDomainId - SID of the logon domain, used for compressing group
        membership.

    LocalMembership - Receives a list of rids corresponding to groups in this
        domain. The list should be freed with MIDL_user_free.

    GlobalMembership - Recevies a list of sids corresponding to groups in
        other domain. The list, but not the sids, should be free with
        MIDL_user_free.

    GlobalMembershipSize - Size, in bytes, of the sids in the global membership
        and the size of the SID_AND_ATTRIBUTES structures.

Returns:

    STATUS_SUCCESS on success
    STATUS_INSUFFICIENT_RESOURCES on for memory allocation failures.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG LocalCount = 0;
    ULONG GlobalCount = 0;
    ULONG GlobalSize = 0;
    ULONG Index;

    LocalMembership->MembershipCount = 0;
    LocalMembership->Groups = NULL;
    GlobalMembership->Count = 0;
    GlobalMembership->SidAndAttributes = NULL;


    //
    // Define a flag so we don't have to do the comparison twice.
    //

#define MSVP_LOCAL_GROUP_ATTR 0x20000000

    for (Index = 0; Index < CompleteMembership->Count ; Index++ ) {
        ASSERT((CompleteMembership->SidAndAttributes[Index].Attributes & MSVP_LOCAL_GROUP_ATTR) == 0);

        if (MsvpEqualSidPrefix(
            LogonDomainId,
            CompleteMembership->SidAndAttributes[Index].Sid
            )) {

            CompleteMembership->SidAndAttributes[Index].Attributes |= MSVP_LOCAL_GROUP_ATTR;
            LocalCount++;
        } else {

            GlobalCount++;
            GlobalSize += sizeof(SID_AND_ATTRIBUTES) + RtlLengthSid(CompleteMembership->SidAndAttributes[Index].Sid);
        }
    }

    //
    // Allocate the arrays for the output
    //

    if (LocalCount != 0)
    {
        LocalMembership->Groups = (PGROUP_MEMBERSHIP) MIDL_user_allocate(LocalCount * sizeof(GROUP_MEMBERSHIP));
        if (LocalMembership->Groups == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        LocalMembership->MembershipCount = LocalCount;
    }
    if (GlobalCount != 0)
    {
        GlobalMembership->SidAndAttributes = (PSID_AND_ATTRIBUTES) MIDL_user_allocate(GlobalCount * sizeof(SID_AND_ATTRIBUTES));
        if (GlobalMembership->SidAndAttributes == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        GlobalMembership->Count = GlobalCount;
    }

    //
    // Loop through again copy the rid or sid into the respective array
    //

    LocalCount = 0;
    GlobalCount = 0;
    for (Index = 0; Index < CompleteMembership->Count ; Index++ ) {

        if ((CompleteMembership->SidAndAttributes[Index].Attributes & MSVP_LOCAL_GROUP_ATTR) != 0) {

            LocalMembership->Groups[LocalCount].Attributes = CompleteMembership->SidAndAttributes[Index].Attributes & ~MSVP_LOCAL_GROUP_ATTR;
            LocalMembership->Groups[LocalCount].RelativeId =
                        *RtlSubAuthoritySid(
                            CompleteMembership->SidAndAttributes[Index].Sid,
                            *RtlSubAuthorityCountSid(
                                CompleteMembership->SidAndAttributes[Index].Sid
                                ) - 1
                            );
            LocalCount++;
        } else {
            GlobalMembership->SidAndAttributes[GlobalCount] = CompleteMembership->SidAndAttributes[Index];
            GlobalCount++;
        }
    }
    *GlobalMembershipSize = GlobalSize;
Cleanup:
    if (!NT_SUCCESS(Status)) {
        if (LocalMembership->Groups != NULL)
        {
            MIDL_user_free(LocalMembership->Groups);
            LocalMembership->Groups = NULL;
        }
        if (GlobalMembership->SidAndAttributes != NULL)
        {
            MIDL_user_free(GlobalMembership->SidAndAttributes);
            GlobalMembership->SidAndAttributes = NULL;
        }
    }
    return(Status);
}


NTSTATUS
MsvpSamValidate (
    IN SAMPR_HANDLE DomainHandle,
    IN BOOLEAN UasCompatibilityRequired,
    IN NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType,
    IN PUNICODE_STRING LogonServer,
    IN PUNICODE_STRING LogonDomainName,
    IN PSID LogonDomainId,
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PVOID LogonInformation,
    IN ULONG GuestRelativeId,
    IN NETLOGON_VALIDATION_INFO_CLASS ValidationLevel,
    OUT PVOID * ValidationInformation,
    OUT PBOOLEAN Authoritative,
    OUT PBOOLEAN BadPasswordCountZeroed
)
/*++

Routine Description:

    Process an interactive, network, or session logon.  It calls
    SamIUserValidation, validates the passed in credentials, updates the logon
    statistics and packages the result for return to the caller.

    This routine is called by MsvSamValidate.

Arguments:

    DomainHandle -- Specifies a handle to the SamDomain to use to
        validate the request.

    UasCompatibilityRequired -- TRUE iff UasCompatibilityMode is on.

    SecureChannelType -- The secure channel type this request was made on.

    LogonServer -- Specifies the server name of the caller.

    LogonDomainName -- Specifies the domain of the caller.

    LogonDomainId  -- Specifies the DomainId of the domain of the caller.

    LogonLevel -- Specifies the level of information given in
        LogonInformation.

    LogonInformation -- Specifies the description for the user
        logging on.  The LogonDomainName field should be ignored.
        The caller is responsible for validating this field.

    GuestRelativeId - If non-zero, specifies the relative ID of the account
        to validate against.

    ValidationLevel -- Specifies the level of information returned in
        ValidationInformation.  Must be NetlogonValidationSamInfo or
        NetlogonValidationSamInfo2.

    ValidationInformation -- Returns the requested validation
        information.  This buffer must be freed user MIDL_user_free.
        This information is only return on STATUS_SUCCESS.

    Authoritative -- Returns whether the status returned is an
        authoritative status which should be returned to the original
        caller.  If not, this logon request may be tried again on another
        domain controller.  This parameter is returned regardless of the
        status code.

    BadPasswordCountZeroed - Returns TRUE iff we zeroed the BadPasswordCount
        field of this user.

Return Value:

    STATUS_SUCCESS: if there was no error.
    STATUS_INVALID_INFO_CLASS: LogonLevel or ValidationLevel are invalid.
    STATUS_NO_SUCH_USER: The specified user has no account.
    STATUS_WRONG_PASSWORD: The password was invalid.

    Other return codes from SamIUserValidation

--*/
{
    NTSTATUS Status;
    NTSTATUS SubAuthExStatus = STATUS_SUCCESS;

    PNETLOGON_LOGON_IDENTITY_INFO LogonInfo;

    SAMPR_HANDLE UserHandle = NULL;
    ULONG RelativeId = GuestRelativeId;
    ULONG SamFlags;
    PSID LocalSidUser = NULL;

    PSAMPR_USER_INFO_BUFFER UserAllInfo = NULL;
    PSAMPR_USER_ALL_INFORMATION UserAll = NULL;
    SAMPR_GET_GROUPS_BUFFER GroupsBuffer;
    ULONG UserFlags = 0;
    USER_SESSION_KEY UserSessionKey;
    LM_SESSION_KEY LmSessionKey;
    ULONG WhichFields = 0;
    UNICODE_STRING LocalUserName;

    UNICODE_STRING LocalWorkstation;
    ULONG UserAccountControl;
    LARGE_INTEGER LogonTime;
    LARGE_INTEGER LogoffTime;
    LARGE_INTEGER KickoffTime;

    LARGE_INTEGER AccountExpires;
    LARGE_INTEGER PasswordMustChange;
    LARGE_INTEGER PasswordLastSet;


    PNETLOGON_VALIDATION_SAM_INFO2 ValidationSam = NULL;
    ULONG ValidationSamSize;
    PUCHAR Where;
    ULONG Index;

    SAMPR_RETURNED_USTRING_ARRAY NameArray;
    SAMPR_ULONG_ARRAY UseArray;
    SID_AND_ATTRIBUTES_LIST GroupMembership;
    SID_AND_ATTRIBUTES_LIST GlobalGroupMembership;
    ULONG GlobalMembershipSize = 0;

    MSV1_0_VALIDATION_INFO SubAuthValidationInformation;
    BOOLEAN fSubAuthEx = FALSE;

    BOOLEAN fMachineAccount;

    ULONG ActionsPerformed = 0;

    LogonInfo = (PNETLOGON_LOGON_IDENTITY_INFO) LogonInformation;


    //
    // check if caller requested that logon only target specified domain.
    //

    if( LogonInfo->ParameterControl & MSV1_0_TRY_SPECIFIED_DOMAIN_ONLY &&
        LogonInfo->LogonDomainName.Length ) {

        //
        // common case is a match for LogonDomainName, so avoid taking locks
        // until mis-match occurs.
        //

        if(!RtlEqualDomainName( &LogonInfo->LogonDomainName, LogonDomainName )) {

            WCHAR LocalTarget[ DNS_MAX_NAME_LENGTH + 1 ];
            WCHAR SpecifiedTarget[ DNS_MAX_NAME_LENGTH + 1 ];
            ULONG cchLocalTarget = 0;
            ULONG cchSpecifiedTarget = 0;

            //
            // pickup the local target name, based on whether this computer is
            // a domain controller.
            //

            EnterCriticalSection(&NtLmGlobalCritSect);

            if( NlpWorkstation ) {

                if( (NtLmGlobalUnicodeDnsComputerNameString.Length + sizeof(WCHAR)) <=
                    sizeof( LocalTarget ) ) {

                    RtlCopyMemory(
                                LocalTarget,
                                NtLmGlobalUnicodeDnsComputerName,
                                NtLmGlobalUnicodeDnsComputerNameString.Length
                                );

                    cchLocalTarget = (NtLmGlobalUnicodeDnsComputerNameString.Length) /
                                    sizeof(WCHAR);
                }

            } else {
                if( (NtLmGlobalUnicodeDnsDomainNameString.Length + sizeof(WCHAR)) <=
                    sizeof( LocalTarget ) ) {

                    RtlCopyMemory(
                                LocalTarget,
                                NtLmGlobalUnicodeDnsDomainName,
                                NtLmGlobalUnicodeDnsDomainNameString.Length
                                );

                    cchLocalTarget = (NtLmGlobalUnicodeDnsDomainNameString.Length) /
                                    sizeof(WCHAR);
                }

            }

            LeaveCriticalSection(&NtLmGlobalCritSect);

            //
            // pull out target name.
            //

            if( (LogonInfo->LogonDomainName.Length + sizeof(WCHAR)) <= sizeof( SpecifiedTarget ) ) {

                cchSpecifiedTarget = (LogonInfo->LogonDomainName.Length) / sizeof(WCHAR);

                RtlCopyMemory(
                                SpecifiedTarget,
                                LogonInfo->LogonDomainName.Buffer,
                                LogonInfo->LogonDomainName.Length
                                );
            }

            if ( cchLocalTarget && cchSpecifiedTarget ) {

                LocalTarget[ cchLocalTarget ] = L'\0';
                SpecifiedTarget[ cchSpecifiedTarget ] = L'\0';

                if(!DnsNameCompare_W( LocalTarget, SpecifiedTarget ) ) {
                    *Authoritative = FALSE;
                    return STATUS_NO_SUCH_USER;
                }
            }
        }
    }


    //
    // Initialization.
    //

    RtlZeroMemory(
        &SubAuthValidationInformation,
        sizeof(MSV1_0_VALIDATION_INFO));

    SubAuthValidationInformation.Authoritative = TRUE;
    SubAuthValidationInformation.WhichFields = 0;
    NameArray.Count = 0;
    NameArray.Element = NULL;
    UseArray.Count = 0;
    UseArray.Element = NULL;
    *BadPasswordCountZeroed = FALSE;
    GroupMembership.Count = 0;
    GroupMembership.SidAndAttributes = NULL;
    GlobalGroupMembership.Count = 0;
    GlobalGroupMembership.SidAndAttributes = NULL;
    GroupsBuffer.MembershipCount = 0;
    GroupsBuffer.Groups = NULL;


    (VOID) NtQuerySystemTime( &LogonTime );


    //
    // Determine what account types are valid.
    //
    // Normal user accounts are always allowed.
    //

    UserAccountControl = USER_NORMAL_ACCOUNT;

    *Authoritative = TRUE;

    switch ( LogonLevel ) {
    case NetlogonInteractiveInformation:
    case NetlogonServiceInformation:

        break;

    case NetlogonNetworkInformation:
        //
        // Local user (Temp Duplicate) accounts are only used on the machine
        // being directly logged onto.
        // (Nor are interactive or service logons allowed to them.)
        //

        if ( SecureChannelType == MsvApSecureChannel ) {
            UserAccountControl |= USER_TEMP_DUPLICATE_ACCOUNT;
        }

        //
        // Machine accounts can be accessed on network connections.
        //

        UserAccountControl |= USER_INTERDOMAIN_TRUST_ACCOUNT |
                              USER_WORKSTATION_TRUST_ACCOUNT |
                              USER_SERVER_TRUST_ACCOUNT;

        break;

    default:
        *Authoritative = TRUE;
        return STATUS_INVALID_INFO_CLASS;
    }


    //
    // Check the ValidationLevel
    //

    switch (ValidationLevel) {
    case NetlogonValidationSamInfo:
    case NetlogonValidationSamInfo2:
        break;

    default:

        *Authoritative = TRUE;
        return STATUS_INVALID_INFO_CLASS;
    }




    //
    // Convert the user name to a RelativeId.
    //

    if ( RelativeId != 0 ) {

        UCHAR cDomainSubAuthorities;
        UCHAR SubAuthIndex;
        ULONG cbLocalSidUser;
        PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority;

        //
        // build a Sid out of the DomainId and the supplied Rid.
        //

        cDomainSubAuthorities = *RtlSubAuthorityCountSid( LogonDomainId );
        pIdentifierAuthority = RtlIdentifierAuthoritySid( LogonDomainId );

        cbLocalSidUser = RtlLengthRequiredSid( (ULONG)(cDomainSubAuthorities + 1) );
        LocalSidUser = MIDL_user_allocate( cbLocalSidUser );
        if (LocalSidUser == NULL) {
            *Authoritative = FALSE;
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        Status = RtlInitializeSid(LocalSidUser, pIdentifierAuthority, (UCHAR)((DWORD)cDomainSubAuthorities+1));
        if(!NT_SUCCESS(Status)) {
            *Authoritative = FALSE;
            goto Cleanup;
        }

        //
        // loop copying subauthorities.
        //

        for( SubAuthIndex = 0 ; SubAuthIndex < cDomainSubAuthorities ; SubAuthIndex++ )
        {
            *RtlSubAuthoritySid( LocalSidUser, (ULONG)SubAuthIndex ) =
            *RtlSubAuthoritySid( LogonDomainId, (ULONG)SubAuthIndex );
        }

        //
        // append relative ID.
        //

        *RtlSubAuthoritySid(LocalSidUser, cDomainSubAuthorities) = RelativeId;

        LocalUserName.Buffer = LocalSidUser;
        LocalUserName.Length = (USHORT)cbLocalSidUser;
        LocalUserName.MaximumLength = (USHORT)cbLocalSidUser;

        SamFlags = SAM_OPEN_BY_SID;
    } else {
        LocalUserName = LogonInfo->UserName;
        SamFlags = 0;
    }



    //
    // Open the user account.
    //

    Status = I_SamIGetUserLogonInformation(
                DomainHandle,
                SamFlags,
                &LocalUserName,
                &UserAllInfo,
                &GroupMembership,
                &UserHandle
                );


    if ( !NT_SUCCESS(Status) ) {
        UserHandle = NULL;
        *Authoritative = FALSE;
        Status = STATUS_NO_SUCH_USER;
        goto Cleanup;
    }



    UserAll = &UserAllInfo->All;

    //
    // pickup RelativeId from looked up information.
    //

    RelativeId = UserAll->UserId;

    //
    // If the account type isn't allowed,
    //  Treat this as though the User Account doesn't exist.
    //
    // SubAuthentication packages can be more specific than this test but
    // they can't become less restrictive.
    //

    if ( (UserAccountControl & UserAll->UserAccountControl) == 0 ) {
        *Authoritative = FALSE;
        Status = STATUS_NO_SUCH_USER;
        goto Cleanup;
    }

    //
    // determine if machine account, if so, certain failures are treated
    // as Authoritative, to prevent fallback to guest and returning incorrect
    // error codes.
    //

    if ( (UserAll->UserAccountControl & USER_MACHINE_ACCOUNT_MASK) != 0 ) {
        fMachineAccount = TRUE;
    } else {
        fMachineAccount = FALSE;
    }


    //
    // If there is a SubAuthentication DLL,
    //  call it to do all the authentication work.
    //

    if ( (LogonInfo->ParameterControl & MSV1_0_SUBAUTHENTICATION_DLL) &&
         (!(LogonInfo->ParameterControl & MSV1_0_SUBAUTHENTICATION_DLL_EX))) {

        ULONG LocalUserFlags = 0;
        ULONG Flags = 0;

        //
        // Ensure the account isn't locked out.
        // We did this regardless before NT 5.0. Now, we do it when either
        // No SubAuth package is specified or
        // New SubAuth package asks us to do account lockout test
        // But, for those who call with old SubAuth packages, they will expect
        // us to do the dirty work.
        //

        if (RelativeId != DOMAIN_USER_RID_ADMIN) {
            if ( UserAll->UserAccountControl & USER_ACCOUNT_AUTO_LOCKED ) {

                //
                // Since the UI strongly encourages admins to disable user
                // accounts rather than delete them.  Treat disabled acccount as
                // non-authoritative allowing the search to continue for other
                // accounts by the same name.
                //

                if ( UserAll->UserAccountControl & USER_ACCOUNT_DISABLED ) {
                    *Authoritative = fMachineAccount;
                } else {
                    *Authoritative = TRUE;
                }

                Status = STATUS_ACCOUNT_LOCKED_OUT;
                goto Cleanup;

            }
        }

        if ( SecureChannelType != MsvApSecureChannel ) {
            Flags |= MSV1_0_PASSTHRU;
        }
        if ( GuestRelativeId != 0 ) {
            Flags |= MSV1_0_GUEST_LOGON;
        }

        Status = Msv1_0SubAuthenticationRoutine(
                    LogonLevel,
                    LogonInformation,
                    Flags,
                    (PUSER_ALL_INFORMATION) UserAll,
                    &WhichFields,
                    &LocalUserFlags,
                    Authoritative,
                    &LogoffTime,
                    &KickoffTime );

        if ( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }

        //
        // Sanity check what the SubAuthentication package returned
        //
        if ( (WhichFields & ~USER_ALL_PARAMETERS) != 0 ) {
            Status = STATUS_INTERNAL_ERROR;
            *Authoritative = TRUE;
            goto Cleanup;
        }

        UserFlags |= LocalUserFlags;



    } else { // we may still have an NT 5.0 SubAuth dll

        //
        // If there is an NT 5.0 SubAuthentication DLL,
        // call it to do all the authentication work.
        //

        if ( (LogonInfo->ParameterControl & MSV1_0_SUBAUTHENTICATION_DLL_EX))
        {
            ULONG LocalUserFlags = 0;
            ULONG Flags = 0;

            if ( SecureChannelType != MsvApSecureChannel ) {
                Flags |= MSV1_0_PASSTHRU;
            }
            if ( GuestRelativeId != 0 ) {
                Flags |= MSV1_0_GUEST_LOGON;
            }

            Status = Msv1_0SubAuthenticationRoutineEx(
                        LogonLevel,
                        LogonInformation,
                        Flags,
                        (PUSER_ALL_INFORMATION) UserAll,
                        (SAM_HANDLE)UserHandle,
                        &SubAuthValidationInformation,
                        &ActionsPerformed );

            *Authoritative = SubAuthValidationInformation.Authoritative;

            if ( !NT_SUCCESS(Status) ) {
                goto Cleanup;
            }

            // We need to do this because even if any of the following checks
            // fail, ARAP stills wants the returned blobs from teh subauth
            // package to be returned to the caller.

            fSubAuthEx = TRUE;

        }

        //
        // Ensure the account isn't locked out.
        //

        if ((ActionsPerformed & MSV1_0_SUBAUTH_LOCKOUT) == 0)
        {
            if (RelativeId != DOMAIN_USER_RID_ADMIN) {
                if ( UserAll->UserAccountControl & USER_ACCOUNT_AUTO_LOCKED ) {

                     //
                     // Since the UI strongly encourages admins to disable user
                     // accounts rather than delete them.  Treat disabled acccount as
                     // non-authoritative allowing the search to continue for other
                     // accounts by the same name.
                     //

                     if ( UserAll->UserAccountControl & USER_ACCOUNT_DISABLED ) {
                         *Authoritative = fMachineAccount;
                     } else {
                         *Authoritative = TRUE;
                     }

                    Status = STATUS_ACCOUNT_LOCKED_OUT;
                    goto Cleanup;
                }
            }
        }

        //
        // Check the password if there's no subauth or if subauth did
        // not already check password.
        //

        if ((ActionsPerformed & MSV1_0_SUBAUTH_PASSWORD) == 0)
        {
            if ( SecureChannelType != NullSecureChannel ) {
                USER_INTERNAL1_INFORMATION Passwords;

                //
                // Copy the password info to the right structure.
                //

                Passwords.NtPasswordPresent = UserAll->NtPasswordPresent;
                if ( UserAll->NtPasswordPresent ) {
                    Passwords.NtOwfPassword =
                        *((PNT_OWF_PASSWORD)(UserAll->NtOwfPassword.Buffer));
                }

                Passwords.LmPasswordPresent = UserAll->LmPasswordPresent;
                if ( UserAll->LmPasswordPresent ) {
                    Passwords.LmOwfPassword =
                        *((PLM_OWF_PASSWORD)(UserAll->LmOwfPassword.Buffer));
                }


                //
                // If the password specified doesn't match the SAM password,
                //    then we've got a password mismatch.
                //

                if ( ! MsvpPasswordValidate (
                            UasCompatibilityRequired,
                            LogonLevel,
                            LogonInformation,
                            &Passwords,
                            &UserFlags,
                            &UserSessionKey,
                            &LmSessionKey ) ) {

                    //
                    // If this is a guest logon and the guest account has no password,
                    //  let the user log on.
                    //
                    // This special case check is after the MsvpPasswordValidate to
                    // give MsvpPasswordValidate every opportunity to compute the
                    // correct values for UserSessionKey and LmSessionKey.
                    //

                    if ( GuestRelativeId != 0 &&
                         !UserAll->NtPasswordPresent &&
                         !UserAll->LmPasswordPresent ) {

                        RtlZeroMemory( &UserSessionKey, sizeof(UserSessionKey) );
                        RtlZeroMemory( &LmSessionKey, sizeof(LmSessionKey) );


                    //
                    // The password mismatched.  We treat STATUS_WRONG_PASSWORD as
                    // an authoritative response.  Our caller may choose to do otherwise.
                    //

                    } else {

                        Status = STATUS_WRONG_PASSWORD;

                        //
                        // Since the UI strongly encourages admins to disable user
                        // accounts rather than delete them.  Treat disabled acccount as
                        // non-authoritative allowing the search to continue for other
                        // accounts by the same name.
                        //
                        if ( UserAll->UserAccountControl & USER_ACCOUNT_DISABLED ) {
                            *Authoritative = fMachineAccount;
                        } else {
                            *Authoritative = TRUE;
                        }

                        goto Cleanup;
                    }
                }
            }
        }

        //
        // Prevent some things from effecting the Administrator user
        //

        if (RelativeId != DOMAIN_USER_RID_ADMIN) {

            //
            // Check if the account is disabled if there's no subauth or if
            // subauth has not already checked.
            //


            if ((ActionsPerformed & MSV1_0_SUBAUTH_ACCOUNT_DISABLED) == 0)
            {
                if ( UserAll->UserAccountControl & USER_ACCOUNT_DISABLED ) {
                    //
                    // Since the UI strongly encourages admins to disable user
                    // accounts rather than delete them.  Treat disabled acccount as
                    // non-authoritative allowing the search to continue for other
                    // accounts by the same name.
                    //

                    *Authoritative = fMachineAccount;
                    Status = STATUS_ACCOUNT_DISABLED;
                    goto Cleanup;
                }
            }

            //
            // Check if the account has expired if there's no subauth or if
            // subauth has not already checked.
            //

            if ((ActionsPerformed & MSV1_0_SUBAUTH_ACCOUNT_EXPIRY) == 0)
            {
                OLD_TO_NEW_LARGE_INTEGER( UserAll->AccountExpires, AccountExpires );

                if ( AccountExpires.QuadPart != 0 &&
                    LogonTime.QuadPart >= AccountExpires.QuadPart ) {
                    *Authoritative = TRUE;
                    Status = STATUS_ACCOUNT_EXPIRED;
                    goto Cleanup;
                }
            }


            //
            // The password is valid, check to see if the password is expired.
            //  (SAM will have appropriately set PasswordMustChange to reflect
            //  USER_DONT_EXPIRE_PASSWORD)
            //
            // Don't check if the password is expired if we didn't check the password.
            // BUGBUG What happens when a Subauth does the password checking
            // but requests us to do password expiry ??
            //

            if ((ActionsPerformed & MSV1_0_SUBAUTH_PASSWORD_EXPIRY) == 0)
            {

              OLD_TO_NEW_LARGE_INTEGER( UserAll->PasswordMustChange, PasswordMustChange );
              OLD_TO_NEW_LARGE_INTEGER( UserAll->PasswordLastSet, PasswordLastSet );

              if ( SecureChannelType != NullSecureChannel ) {
                if ( LogonTime.QuadPart >= PasswordMustChange.QuadPart ) {

                    if ( PasswordLastSet.QuadPart == 0 ) {
                        Status = STATUS_PASSWORD_MUST_CHANGE;
                    } else {
                        Status = STATUS_PASSWORD_EXPIRED;
                    }
                    *Authoritative = TRUE;
                    goto Cleanup;
                }
              }
            }
        }

        //
        // Validate the workstation the user logged on from.
        //
        // Ditch leading \\ on workstation name before passing it to SAM.
        //

        LocalWorkstation = LogonInfo->Workstation;
        if ( LocalWorkstation.Length > 0 &&
             LocalWorkstation.Buffer[0] == L'\\' &&
             LocalWorkstation.Buffer[1] == L'\\' ) {
            LocalWorkstation.Buffer += 2;
            LocalWorkstation.Length -= 2*sizeof(WCHAR);
            LocalWorkstation.MaximumLength -= 2*sizeof(WCHAR);
        }


        //
        // Check if SAM found some more specific reason to not allow logon.
        //

        Status = I_SamIAccountRestrictions(
                    UserHandle,
                    &LocalWorkstation,
                    (PUNICODE_STRING) &UserAll->WorkStations,
                    (PLOGON_HOURS) &UserAll->LogonHours,
                    &LogoffTime,
                    &KickoffTime );

        if ( !NT_SUCCESS(Status) ) {
            *Authoritative = TRUE;
            goto Cleanup;
        }


        //
        // If there is a SubAuthentication package zero, call it
        //

        if (NlpSubAuthZeroExists) {
            ULONG LocalUserFlags = 0;
            ULONG Flags = 0;

            if ( SecureChannelType != MsvApSecureChannel ) {
                Flags |= MSV1_0_PASSTHRU;
            }
            if ( GuestRelativeId != 0 ) {
                Flags |= MSV1_0_GUEST_LOGON;
            }


            Status = Msv1_0SubAuthenticationRoutineZero(
                        LogonLevel,
                        LogonInformation,
                        Flags,
                        (PUSER_ALL_INFORMATION) UserAll,
                        &WhichFields,
                        &LocalUserFlags,
                        Authoritative,
                        &LogoffTime,
                        &KickoffTime );

            if ( !NT_SUCCESS(Status) ) {
                goto Cleanup;
            }

            //
            // Sanity check what the SubAuthentication package returned
            //

            if ( (WhichFields & ~USER_ALL_PARAMETERS) != 0 ) {
                Status = STATUS_INTERNAL_ERROR;
                *Authoritative = TRUE;
                goto Cleanup;
            }

            UserFlags |= LocalUserFlags;

        }


    }


    //
    // If the account is a machine account,
    //  let the caller know he got the password right.
    //  (But don't let him actually log on).
    //

    // But, for NT 5.0, we must allow accounts with account control
    // USER_WORKSTATION_TRUST_ACCOUNT for remote boot clients who
    // will logon with their machine accounts

    if ( (UserAll->UserAccountControl & USER_MACHINE_ACCOUNT_MASK) != 0 ) {
        if (UserAll->UserAccountControl & USER_INTERDOMAIN_TRUST_ACCOUNT) {
            Status = STATUS_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT;
        } else if (UserAll->UserAccountControl &
                   USER_WORKSTATION_TRUST_ACCOUNT) {
            if ( (LogonInfo->ParameterControl & MSV1_0_ALLOW_WORKSTATION_TRUST_ACCOUNT) == 0) {
                Status = STATUS_NOLOGON_WORKSTATION_TRUST_ACCOUNT;
            } else {

                UNICODE_STRING MachineAccountName;
                NTSTATUS TempStatus;


                //
                // if the password was correct, and it happened to match
                // the machine name, dis-allow it regardless.
                //


                //
                // Compute the lower case user name.
                //

                TempStatus = RtlDowncaseUnicodeString( &MachineAccountName,
                                                   (PUNICODE_STRING)&UserAll->UserName,
                                                   TRUE );


                if( NT_SUCCESS( TempStatus ) )
                {
                    USHORT LastChar = MachineAccountName.Length / sizeof(WCHAR);

                    if( LastChar )
                    {
                        if( MachineAccountName.Buffer[LastChar-1] == L'$' )
                        {
                            MachineAccountName.Length -= sizeof(WCHAR);
                        }

                        if( LastChar > LM20_PWLEN )
                        {
                            MachineAccountName.Length = LM20_PWLEN * sizeof(WCHAR);
                        }
                    }


                    if ( UserAll->NtPasswordPresent ) {

                        NT_OWF_PASSWORD NtOwfMachineName;
                        NT_OWF_PASSWORD *pOwf;

                        pOwf = ((PNT_OWF_PASSWORD)(UserAll->NtOwfPassword.Buffer));

                        RtlCalculateNtOwfPassword(
                                     &MachineAccountName,
                                     &NtOwfMachineName );

                        if ( RtlCompareMemory( pOwf,
                                               &NtOwfMachineName,
                                               NT_OWF_PASSWORD_LENGTH ) ==
                                               NT_OWF_PASSWORD_LENGTH ) {

                            Status = STATUS_NOLOGON_WORKSTATION_TRUST_ACCOUNT;
                        }
                    }


                    RtlFreeUnicodeString( &MachineAccountName );
                }

            }
        } else if (UserAll->UserAccountControl & USER_SERVER_TRUST_ACCOUNT) {
            if ( (LogonInfo->ParameterControl & MSV1_0_ALLOW_SERVER_TRUST_ACCOUNT) == 0) {
                Status = STATUS_NOLOGON_SERVER_TRUST_ACCOUNT;
            } else {

                //
                // it's a server trust account.
                // treat same way as workstation trust account.

                UNICODE_STRING MachineAccountName;
                NTSTATUS TempStatus;

                // NOTE: some code here is duplicated from above.
                // this will be merged when a new Rtl has been added for
                // computing initial machine password from machine acct name.
                //

                //
                // if the password was correct, and it happened to match
                // the machine name, dis-allow it regardless.
                //


                //
                // Compute the lower case user name.
                //

                TempStatus = RtlDowncaseUnicodeString( &MachineAccountName,
                                                   (PUNICODE_STRING)&UserAll->UserName,
                                                   TRUE );


                if( NT_SUCCESS( TempStatus ) )
                {
                    USHORT LastChar = MachineAccountName.Length / sizeof(WCHAR);

                    if( LastChar )
                    {
                        if( MachineAccountName.Buffer[LastChar-1] == L'$' )
                        {
                            MachineAccountName.Length -= sizeof(WCHAR);
                        }

                        if( LastChar > LM20_PWLEN )
                        {
                            MachineAccountName.Length = LM20_PWLEN * sizeof(WCHAR);
                        }
                    }


                    if ( UserAll->NtPasswordPresent ) {

                        NT_OWF_PASSWORD NtOwfMachineName;
                        NT_OWF_PASSWORD *pOwf;

                        pOwf = ((PNT_OWF_PASSWORD)(UserAll->NtOwfPassword.Buffer));

                        RtlCalculateNtOwfPassword(
                                     &MachineAccountName,
                                     &NtOwfMachineName );

                        if ( RtlCompareMemory( pOwf,
                                               &NtOwfMachineName,
                                               NT_OWF_PASSWORD_LENGTH ) ==
                                               NT_OWF_PASSWORD_LENGTH ) {

                            Status = STATUS_NOLOGON_SERVER_TRUST_ACCOUNT;
                        }
                    }


                    RtlFreeUnicodeString( &MachineAccountName );
                }

                //
                // Let the client know that this was
                // a server trust account
                //

                UserFlags |= LOGON_SERVER_TRUST_ACCOUNT;
            }

        } else {
            Status = STATUS_NO_SUCH_USER;
        }
        if (!NT_SUCCESS(Status)) {
            *Authoritative = TRUE;
            goto Cleanup;
        }
    }

    //
    // Filter the groups into global groups (from other domains) and local
    // groups (from this domain).
    //

    Status = MsvpFilterGroupMembership(
                &GroupMembership,
                LogonDomainId,
                &GroupsBuffer,
                &GlobalGroupMembership,
                &GlobalMembershipSize
                );


    if ( !NT_SUCCESS(Status) ) {
        *Authoritative = FALSE;
        goto Cleanup;
    }

Cleanup:

    if (NT_SUCCESS(Status) || fSubAuthEx)
    {
        //
        // Allocate a return buffer for validation information.
        //  (Return less information for a network logon)
        //  (Return UserParameters for a MNS logon)
        //

        ValidationSamSize = sizeof( NETLOGON_VALIDATION_SAM_INFO2 ) +
                GroupsBuffer.MembershipCount * sizeof(GROUP_MEMBERSHIP) +
                LogonDomainName->Length + sizeof(WCHAR) +
                LogonServer->Length + sizeof(WCHAR) +
                RtlLengthSid( LogonDomainId );

        if ( LogonLevel != NetlogonNetworkInformation ) {
            ValidationSamSize +=
                UserAll->UserName.Length + sizeof(WCHAR) +
                UserAll->FullName.Length + sizeof(WCHAR) +
                UserAll->ScriptPath.Length + sizeof(WCHAR)+
                UserAll->ProfilePath.Length + sizeof(WCHAR) +
                UserAll->HomeDirectory.Length + sizeof(WCHAR) +
                UserAll->HomeDirectoryDrive.Length + sizeof(WCHAR);
        }

        //
        // for network logon of the guest account, return the account
        // name so the correct name ends up in the session list.
        //

        if ( LogonLevel == NetlogonNetworkInformation &&
             GuestRelativeId != 0 ) {

            ValidationSamSize +=
                UserAll->UserName.Length + sizeof(WCHAR) ;
        }

        if ( LogonInfo->ParameterControl & MSV1_0_RETURN_USER_PARAMETERS ) {
            ValidationSamSize +=
                UserAll->Parameters.Length + sizeof(WCHAR);
        } else if ( LogonInfo->ParameterControl & MSV1_0_RETURN_PROFILE_PATH ) {
            ValidationSamSize +=
                UserAll->ProfilePath.Length + sizeof(WCHAR);
        }

        //
        // If the caller can handle extra groups, let them have the groups from
        // other domains.
        //

        if (ValidationLevel == NetlogonValidationSamInfo2) {
            ValidationSamSize += GlobalMembershipSize;
        }

        ValidationSamSize = ROUND_UP_COUNT( ValidationSamSize, sizeof(WCHAR) );

        ValidationSam = MIDL_user_allocate( ValidationSamSize );

        if ( ValidationSam == NULL ) {
            *Authoritative = FALSE;
            fSubAuthEx = FALSE; // avoid nasty loop condition
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        //
        // Default unused fields (and ExpansionRoom) to zero.
        //

        RtlZeroMemory( ValidationSam, ValidationSamSize );

        //
        // Copy the scalars to the validation buffer.
        //

        Where = (PUCHAR) (ValidationSam + 1);

        if ((SubAuthValidationInformation.WhichFields & MSV1_0_VALIDATION_LOGOFF_TIME) != 0) {
            NEW_TO_OLD_LARGE_INTEGER( SubAuthValidationInformation.LogoffTime, ValidationSam->LogoffTime );
        }
        else {
            NEW_TO_OLD_LARGE_INTEGER( LogoffTime, ValidationSam->LogoffTime );
        }

        if ((SubAuthValidationInformation.WhichFields & MSV1_0_VALIDATION_KICKOFF_TIME) != 0) {
            NEW_TO_OLD_LARGE_INTEGER( SubAuthValidationInformation.KickoffTime, ValidationSam->KickOffTime );
        }
        else {
            NEW_TO_OLD_LARGE_INTEGER( KickoffTime, ValidationSam->KickOffTime );
        }

        if ((SubAuthValidationInformation.WhichFields & MSV1_0_VALIDATION_SESSION_KEY) != 0) {
                ValidationSam->UserSessionKey = SubAuthValidationInformation.SessionKey;
        }
        else {
            ValidationSam->UserSessionKey = UserSessionKey;
        }

        if ((SubAuthValidationInformation.WhichFields & MSV1_0_VALIDATION_USER_FLAGS) != 0) {
            ValidationSam->UserFlags = SubAuthValidationInformation.UserFlags;
        }
        else {
            ValidationSam->UserFlags = UserFlags;
        }

        if ((SubAuthValidationInformation.WhichFields & MSV1_0_VALIDATION_USER_ID) != 0) {
            ValidationSam->UserId = SubAuthValidationInformation.UserId;
        }
        else {
            ValidationSam->UserId = UserAll->UserId;
        }
        ValidationSam->LogonTime = UserAll->LastLogon;
        ValidationSam->PasswordLastSet = UserAll->PasswordLastSet;
        ValidationSam->PasswordCanChange = UserAll->PasswordCanChange;
        ValidationSam->PasswordMustChange = UserAll->PasswordMustChange;

        ValidationSam->LogonCount = UserAll->LogonCount;
        ValidationSam->BadPasswordCount = UserAll->BadPasswordCount;
        ValidationSam->PrimaryGroupId = UserAll->PrimaryGroupId;
        ValidationSam->GroupCount = GroupsBuffer.MembershipCount;
        ASSERT( SAMINFO_LM_SESSION_KEY_SIZE == sizeof(LmSessionKey) );
        RtlCopyMemory( &ValidationSam->ExpansionRoom[SAMINFO_LM_SESSION_KEY],
                   &LmSessionKey,
                   SAMINFO_LM_SESSION_KEY_SIZE );
        ValidationSam->ExpansionRoom[SAMINFO_USER_ACCOUNT_CONTROL] = UserAll->UserAccountControl;

        // Save any status for subuath users not returned by the subauth package

        if (fSubAuthEx)
        {
            ValidationSam->ExpansionRoom[SAMINFO_SUBAUTH_STATUS] = Status;
        }


        //
        // Copy ULONG aligned data to the validation buffer.
        //

        RtlCopyMemory(
            Where,
            GroupsBuffer.Groups,
            GroupsBuffer.MembershipCount * sizeof(GROUP_MEMBERSHIP) );

        ValidationSam->GroupIds = (PGROUP_MEMBERSHIP) Where;
        Where += GroupsBuffer.MembershipCount * sizeof(GROUP_MEMBERSHIP);


        RtlCopyMemory(
            Where,
            LogonDomainId,
            RtlLengthSid( LogonDomainId ) );

        ValidationSam->LogonDomainId = (PSID) Where;
        Where += RtlLengthSid( LogonDomainId );

        //
        // If the client asked for extra information, return that
        // we support it
        //

        if (ValidationLevel == NetlogonValidationSamInfo2) {
            ValidationSam->UserFlags |= LOGON_EXTRA_SIDS;
            if (GlobalMembershipSize != 0) {
                ULONG SidLength;

                ValidationSam->SidCount = GlobalGroupMembership.Count;
                ValidationSam->ExtraSids = (PNETLOGON_SID_AND_ATTRIBUTES) Where;
                Where += ValidationSam->SidCount * sizeof(NETLOGON_SID_AND_ATTRIBUTES);

                //
                // Copy all the extra sids into the buffer
                //

                for (Index = 0; Index < ValidationSam->SidCount ; Index++ ) {
                    ValidationSam->ExtraSids[Index].Attributes = GlobalGroupMembership.SidAndAttributes[Index].Attributes;
                    ValidationSam->ExtraSids[Index].Sid = Where;
                    SidLength = RtlLengthSid(GlobalGroupMembership.SidAndAttributes[Index].Sid);
                    RtlCopyMemory(
                        ValidationSam->ExtraSids[Index].Sid,
                        GlobalGroupMembership.SidAndAttributes[Index].Sid,
                        SidLength
                        );

                    Where += SidLength;

                }

            }
        }

        //
        // Copy WCHAR aligned data to the validation buffer.
        //  (Return less information for a network logon)
        //

        Where = ROUND_UP_POINTER( Where, sizeof(WCHAR) );

        if ( LogonLevel != NetlogonNetworkInformation ) {

            NlpPutString( &ValidationSam->EffectiveName,
                          (PUNICODE_STRING)&UserAll->UserName,
                          &Where );

            NlpPutString( &ValidationSam->FullName,
                          (PUNICODE_STRING)&UserAll->FullName,
                          &Where );

            NlpPutString( &ValidationSam->LogonScript,
                          (PUNICODE_STRING)&UserAll->ScriptPath,
                          &Where );

            NlpPutString( &ValidationSam->ProfilePath,
                          (PUNICODE_STRING)&UserAll->ProfilePath,
                          &Where );

            NlpPutString( &ValidationSam->HomeDirectory,
                          (PUNICODE_STRING)&UserAll->HomeDirectory,
                          &Where );

            NlpPutString( &ValidationSam->HomeDirectoryDrive,
                          (PUNICODE_STRING)&UserAll->HomeDirectoryDrive,
                          &Where );

        }

        if ( LogonLevel == NetlogonNetworkInformation &&
             GuestRelativeId != 0 ) {

            NlpPutString( &ValidationSam->EffectiveName,
                          (PUNICODE_STRING)&UserAll->UserName,
                          &Where );
        }

        if ((SubAuthValidationInformation.WhichFields & MSV1_0_VALIDATION_LOGON_SERVER) != 0) {
            NlpPutString( &ValidationSam->LogonServer,
                          &SubAuthValidationInformation.LogonServer,
                          &Where );

        }
        else {
            NlpPutString( &ValidationSam->LogonServer,
                          LogonServer,
                          &Where );

        }
        if ((SubAuthValidationInformation.WhichFields & MSV1_0_VALIDATION_LOGON_DOMAIN) != 0) {
            NlpPutString( &ValidationSam->LogonDomainName,
                          &SubAuthValidationInformation.LogonDomainName,
                          &Where );

        }
        else {
            NlpPutString( &ValidationSam->LogonDomainName,
                          LogonDomainName,
                          &Where );

        }


        //
        // Kludge: Pass back UserParameters in HomeDirectoryDrive since we
        // can't change the NETLOGON_VALIDATION_SAM_INFO2 structure between
        // releases NT 1.0 and NT 1.0A. HomeDirectoryDrive was NULL for release 1.0A
        // so we'll use that field.
        //

        if ( LogonInfo->ParameterControl & MSV1_0_RETURN_USER_PARAMETERS ) {
            NlpPutString( &ValidationSam->HomeDirectoryDrive,
                          (PUNICODE_STRING)&UserAll->Parameters,
                          &Where );
        } else if ( LogonInfo->ParameterControl & MSV1_0_RETURN_PROFILE_PATH ) {
            NlpPutString( &ValidationSam->HomeDirectoryDrive,
                          (PUNICODE_STRING)&UserAll->ProfilePath,
                          &Where );
            ValidationSam->UserFlags |= LOGON_PROFILE_PATH_RETURNED;
        }


        *Authoritative = TRUE;

        //
        // For SubAuthEx, we save away the original Status to make decisions
        // later on about additional processing to perform.
        //

        if( fSubAuthEx ) {
            SubAuthExStatus = Status;
        }

        Status = STATUS_SUCCESS;

    }

    //
    // Cleanup up before returning.
    //

    //
    // If the User Parameters have been changed,
    //  write them back to SAM.
    //

    if ( NT_SUCCESS(Status) &&
        (WhichFields & USER_ALL_PARAMETERS) ) {
        SAMPR_USER_INFO_BUFFER UserInfo;

        UserInfo.Parameters.Parameters = UserAll->Parameters;

        Status = I_SamrSetInformationUser(
                        UserHandle,
                        UserParametersInformation,
                        &UserInfo );

    }


    //
    // Update the logon statistics.
    //

    if ( NT_SUCCESS( SubAuthExStatus ) &&
        (NT_SUCCESS(Status) || Status == STATUS_WRONG_PASSWORD) ) {

        SAMPR_USER_INFO_BUFFER UserInfo;

        if ( NT_SUCCESS( Status ) ) {
            if ( LogonLevel == NetlogonInteractiveInformation ) {
                UserInfo.Internal2.StatisticsToApply =
                    USER_LOGON_INTER_SUCCESS_LOGON;
            } else {

                //
                // On network logons,
                //  only update the statistics on 'success' if explicitly asked,
                //  or the Bad Password count will be zeroed.
                //

                if ( (LogonInfo->ParameterControl & MSV1_0_UPDATE_LOGON_STATISTICS) ||
                     UserAll->BadPasswordCount != 0 ) {

                    UserInfo.Internal2.StatisticsToApply =
                        USER_LOGON_NET_SUCCESS_LOGON;
                } else {
                    UserInfo.Internal2.StatisticsToApply = 0;
                }
            }

            // Tell the caller we zeroed the bad password count
            if ( UserAll->BadPasswordCount != 0 ) {
                *BadPasswordCountZeroed = TRUE;
            }

        } else {
            UserInfo.Internal2.StatisticsToApply =
                USER_LOGON_BAD_PASSWORD | USER_LOGON_BAD_PASSWORD_WKSTA;
            ((PUSER_INTERNAL2A_INFORMATION) &UserInfo.Internal2)->Workstation = LogonInfo->Workstation;
        }

        if ( UserInfo.Internal2.StatisticsToApply != 0 ) {
            NTSTATUS LogonStatus;

            UserInfo.Internal2.StatisticsToApply |= USER_LOGON_TYPE_NTLM;

            LogonStatus = I_SamrSetInformationUser(
                            UserHandle,
                            UserInternal2Information,
                            &UserInfo );

        }

    }


    //
    // Audit this logon. We don't audit failures for the guest account because
    // they are so frequent.
    //

    if (GuestRelativeId == 0 || NT_SUCCESS(Status)) {
        NTSTATUS AuditStatus;

        AuditStatus = Status;

        //
        // if there was a possibly un-successful SubAuthEx status, use it
        //

        if( NT_SUCCESS( AuditStatus ) && fSubAuthEx ) {

            AuditStatus = SubAuthExStatus;
        }

        I_LsaIAuditAccountLogon(
            NT_SUCCESS(AuditStatus) ? SE_AUDITID_ACCOUNT_LOGON_SUCCESS : SE_AUDITID_ACCOUNT_LOGON_FAILURE,
            (BOOLEAN) NT_SUCCESS(AuditStatus),
            &NlpMsv1_0PackageName,
            &LocalUserName,
            &LogonInfo->Workstation,
            AuditStatus
            );
    }

    //
    // Return the validation buffer to the caller.
    //

    if ( !NT_SUCCESS(Status) ) {
        if (ValidationSam != NULL) {
            MIDL_user_free( ValidationSam );
            ValidationSam = NULL;
        }
    }

    *ValidationInformation = ValidationSam;

    //
    // Free locally used resources.
    //

    I_SamIFree_SAMPR_RETURNED_USTRING_ARRAY( &NameArray );
    I_SamIFree_SAMPR_ULONG_ARRAY( &UseArray );

    if ( UserAllInfo != NULL ) {
        I_SamIFree_SAMPR_USER_INFO_BUFFER( UserAllInfo, UserAllInformation );
    }

    if (GroupMembership.SidAndAttributes != NULL)
    {
        I_SamIFreeSidAndAttributesList(&GroupMembership);
    }

    if ( GroupsBuffer.Groups != NULL ) {
        MIDL_user_free(GroupsBuffer.Groups);
    }

    if ( GlobalGroupMembership.SidAndAttributes != NULL ) {
        MIDL_user_free(GlobalGroupMembership.SidAndAttributes);
    }

    if ( UserHandle != NULL ) {
        I_SamrCloseHandle( &UserHandle );
    }

    if (SubAuthValidationInformation.LogonDomainName.Buffer != NULL) {
        MIDL_user_free(SubAuthValidationInformation.LogonDomainName.Buffer);
    }
    if (SubAuthValidationInformation.LogonServer.Buffer != NULL) {
        MIDL_user_free(SubAuthValidationInformation.LogonServer.Buffer);
    }

    if (LocalSidUser != NULL) {
        MIDL_user_free(LocalSidUser);
    }

    return Status;
}


NTSTATUS
MsvSamValidate (
    IN SAM_HANDLE DomainHandle,
    IN BOOLEAN UasCompatibilityRequired,
    IN NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType,
    IN PUNICODE_STRING LogonServer,
    IN PUNICODE_STRING LogonDomainName,
    IN PSID LogonDomainId,
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PVOID LogonInformation,
    IN NETLOGON_VALIDATION_INFO_CLASS ValidationLevel,
    OUT PVOID * ValidationInformation,
    OUT PBOOLEAN Authoritative,
    OUT PBOOLEAN BadPasswordCountZeroed,
    IN DWORD AccountsToTry
)
/*++

Routine Description:

    Process an interactive, network, or session logon.  It calls
    SamIUserValidation, validates the passed in credentials, updates the logon
    statistics and packages the result for return to the caller.

    This routine is called directly from the MSV Authentication package
    if the account is defined locally.  This routine is called
    from the Netlogon Service otherwise.

Arguments:

    DomainHandle -- Specifies a handle to the SamDomain to use to
        validate the request.

    UasCompatibilityRequired -- TRUE iff UasCompatibilityRequired is on.

    SecureChannelType -- The secure channel type this request was made on.

    LogonServer -- Specifies the server name of the caller.

    LogonDomainName -- Specifies the domain of the caller.

    LogonDomainId  -- Specifies the DomainId of the domain of the caller.

    LogonLevel -- Specifies the level of information given in
        LogonInformation.

    LogonInformation -- Specifies the description for the user
        logging on.  The LogonDomainName field should be ignored.
        The caller is responsible for validating this field.

    ValidationLevel -- Specifies the level of information returned in
        ValidationInformation.  Must be NetlogonValidationSamInfo or
        NetlogonValidationSamInfo2.

    ValidationInformation -- Returns the requested validation
        information.  This buffer must be freed user MIDL_user_free.

    Authoritative -- Returns whether the status returned is an
        authoritative status which should be returned to the original
        caller.  If not, this logon request may be tried again on another
        domain controller.  This parameter is returned regardless of the
        status code.

    BadPasswordCountZeroed - Returns TRUE iff we zeroed the BadPasswordCount
        field of this user.

    AccountsToTry -- Specifies whether the username specified in
        LogonInformation is to be used to logon, whether to guest account
        is to be used to logon, or both serially.

Return Value:

    STATUS_SUCCESS: if there was no error.
    STATUS_INVALID_INFO_CLASS: LogonLevel or ValidationLevel are invalid.
    STATUS_NO_SUCH_USER: The specified user has no account.
    STATUS_WRONG_PASSWORD: The password was invalid.

    Other return codes from SamIUserValidation

--*/
{
    NTSTATUS Status;
    NTSTATUS GuestStatus;
    PNETLOGON_LOGON_IDENTITY_INFO LogonInfo;


    //
    // Validate the specified user.
    //
    *BadPasswordCountZeroed = FALSE;

    if ( AccountsToTry & MSVSAM_SPECIFIED ) {

        //
        // Keep track of the total number of logons attempted.
        //


        I_SamIIncrementPerformanceCounter(
            MsvLogonCounter
            );
        InterlockedIncrement(&NlpLogonAttemptCount);


        Status = MsvpSamValidate( (SAMPR_HANDLE) DomainHandle,
                                  UasCompatibilityRequired,
                                  SecureChannelType,
                                  LogonServer,
                                  LogonDomainName,
                                  LogonDomainId,
                                  LogonLevel,
                                  LogonInformation,
                                  0,
                                  ValidationLevel,
                                  ValidationInformation,
                                  Authoritative,
                                  BadPasswordCountZeroed );

        //
        // If the SAM database authoritatively handled this logon attempt,
        //  just return.
        //

        if ( *Authoritative ) {
            return Status;
        }

    //
    // If the caller only wants to log on as guest,
    //  Pretend the first validation simply didn't find the user.
    //
    } else {
        *Authoritative = FALSE;
        Status = STATUS_NO_SUCH_USER;

    }

    //
    // If guest accounts are not allowed,
    //  return now.
    //

    LogonInfo = (PNETLOGON_LOGON_IDENTITY_INFO) LogonInformation;

    if ( LogonLevel != NetlogonNetworkInformation ||
        SecureChannelType != MsvApSecureChannel ||
        ( LogonInfo->ParameterControl & MSV1_0_DONT_TRY_GUEST_ACCOUNT ) ||
        (AccountsToTry & MSVSAM_GUEST) == 0 ) {

        return Status;
    }

    //
    // Try the Guest Account.
    //

    GuestStatus = MsvpSamValidate( (SAMPR_HANDLE) DomainHandle,
                                   UasCompatibilityRequired,
                                   SecureChannelType,
                                   LogonServer,
                                   LogonDomainName,
                                   LogonDomainId,
                                   LogonLevel,
                                   LogonInformation,
                                   DOMAIN_USER_RID_GUEST,
                                   ValidationLevel,
                                   ValidationInformation,
                                   Authoritative,
                                   BadPasswordCountZeroed );

    if ( NT_SUCCESS(GuestStatus) ) {
        PNETLOGON_VALIDATION_SAM_INFO2 ValidationInfo;

        ASSERT ((ValidationLevel == NetlogonValidationSamInfo) ||
                (ValidationLevel == NetlogonValidationSamInfo2) );
        ValidationInfo =
            (PNETLOGON_VALIDATION_SAM_INFO2) *ValidationInformation;
        ValidationInfo->UserFlags |= LOGON_GUEST;
        return GuestStatus;
    }

    //
    // Failed Guest logon attempts are never authoritative and the status from
    // the original logon attempt is more significant than the Guest logon
    // status.
    //
    *Authoritative = FALSE;
    return Status;

}


NTSTATUS
MsvSamLogoff (
    IN SAM_HANDLE DomainHandle,
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PVOID LogonInformation
)
/*++

Routine Description:

    Process an interactive, network, or session logoff.  It simply updates
    the logon statistics for the user account.

    This routine is called directly from the MSV Authentication package
    if the user was logged on not using the Netlogon service.  This routine
    is called from the Netlogon Service otherwise.

Arguments:

    DomainHandle -- Specifies a handle to the SamDomain containing
        the user to logoff.

    LogonLevel -- Specifies the level of information given in
        LogonInformation.

    LogonInformation -- Specifies the description for the user
        logging on.  The LogonDomainName field should be ignored.
        The caller is responsible for validating this field.

Return Value:

    STATUS_SUCCESS: if there was no error.
    STATUS_INVALID_INFO_CLASS: LogonLevel or ValidationLevel are invalid.
    STATUS_NO_SUCH_USER: The specified user has no account.
    STATUS_WRONG_PASSWORD: The password was invalid.

    Other return codes from SamIUserValidation

--*/
{
    return(STATUS_SUCCESS);
    UNREFERENCED_PARAMETER( DomainHandle );
    UNREFERENCED_PARAMETER( LogonLevel );
    UNREFERENCED_PARAMETER( LogonInformation );
}


ULONG
MsvGetLogonAttemptCount (
    VOID
)
/*++

Routine Description:

    Return the number of logon attempts since the last reboot.

Arguments:

    NONE

Return Value:

    Returns the number of logon attempts since the last reboot.

--*/
{

    //
    // Keep track of the total number of logons attempted.
    //

    return NlpLogonAttemptCount;
}

