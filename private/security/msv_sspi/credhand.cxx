/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    credhand.cxx

Abstract:

    API and support routines for handling credential handles.

Author:


    Cliff Van Dyke (CliffV) 26-Jun-1993
Revision History:
    ChandanS 03-Aug-1996  Stolen from net\svcdlls\ntlmssp\common\credhand.c

--*/


//
// Common include files.
//

#include <global.h>
#include <align.h>      // ALIGN_WCHAR

extern "C"
{

#include <nlp.h>

}

//
// Crit Sect to protect various globals in this module.
//

CRITICAL_SECTION SspCredentialCritSect;

LIST_ENTRY SspCredentialList;

// This is the definition of a null session string.
// Change this if the definition changes

#define IsNullSessionString(x) (((x)->Length == 0) &&    \
                          ((x)->Buffer != NULL))

BOOLEAN
AlterRtlEqualUnicodeString(
    IN PUNICODE_STRING String1,
    IN PUNICODE_STRING String2,
    IN BOOLEAN CaseInSensitive
    )
/*++
    This is here to catch cases that RtlEqualUnicodeString does not.
    For e.g, if String1 is (NULL,0,0) and String2 is ("",0,2),
    RtlEqualUnicodeString returned TRUE but we really want it to return FALSE
--*/
{
    BOOL fRet = RtlEqualUnicodeString(String1, String2, CaseInSensitive);

    if (fRet && (IsNullSessionString(String1) != IsNullSessionString(String2)))
    {
        fRet = FALSE;
    }
    return (fRet != 0);
}

BOOL
EnableAllPrivileges(
    IN  HANDLE ClientTokenHandle
    );


NTSTATUS
SspCredentialReferenceCredential(
    IN ULONG_PTR CredentialHandle,
    IN BOOLEAN DereferenceCredential,
    IN BOOLEAN ForceRemoveCredential,
    OUT PSSP_CREDENTIAL * UserCredential
    )

/*++

Routine Description:

    This routine checks to see if the Credential is from a currently
    active client, and references the Credential if it is valid.

    The caller may optionally request that the client's Credential be
    removed from the list of valid Credentials - preventing future
    requests from finding this Credential.

    For a client's Credential to be valid, the Credential value
    must be on our list of active Credentials.


Arguments:

    CredentialHandle - Points to the CredentialHandle of the Credential
        to be referenced.

    DereferenceCredential - This boolean value indicates that that a call
        a single instance of this credential handle should be freed. If there
        are multiple instances, they should still continue to work.

    ForceRemoveCredential - This boolean value indicates whether the caller
        wants the logon process's Credential to be removed from the list
        of Credentials.  TRUE indicates the Credential is to be removed.
        FALSE indicates the Credential is not to be removed.


Return Value:

    NULL - the Credential was not found.

    Otherwise - returns a pointer to the referenced credential.

--*/

{
    PLIST_ENTRY ListEntry;
    PSSP_CREDENTIAL Credential = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    SECPKG_CLIENT_INFO ClientInfo;

    ULONG DereferenceCount = 1;


    //
    // Make sure that nobody tries to force removal without also
    // trying to dereference the credential.
    //

    ASSERT(!(ForceRemoveCredential && !DereferenceCredential));

    *UserCredential = NULL ;

    Status = LsaFunctions->GetClientInfo(&ClientInfo);

    if(!NT_SUCCESS(Status))
    {
        SECPKG_CALL_INFO CallInfo;

        //
        // this call can fail during a cleanup call.  so ignore that for now,
        // and check for cleanup disposition.
        //

        if(LsaFunctions->GetCallInfo(&CallInfo)) {

            if ((CallInfo.Attributes & SECPKG_CALL_CLEANUP) != 0)
            {
                Status = STATUS_SUCCESS;
                RtlZeroMemory(
                    &ClientInfo,
                    sizeof(SECPKG_CLIENT_INFO)
                    );
                ClientInfo.HasTcbPrivilege = TRUE;
                ClientInfo.ProcessID = CallInfo.ProcessId;

                DereferenceCount = CallInfo.CallCount;
            }
        }

        if( !NT_SUCCESS( Status ) )
        {
            SspPrint(( SSP_CRITICAL, "SspCredentialReferenceCredential: GetClientInfo returned 0x%lx\n", Status));
            return( Status );
        }

    }


    //
    // Acquire exclusive access to the Credential list
    //

    EnterCriticalSection( &SspCredentialCritSect );


    //
    // Now walk the list of Credentials looking for a match.
    //

    for ( ListEntry = SspCredentialList.Flink;
          ListEntry != &SspCredentialList;
          ListEntry = ListEntry->Flink ) {

        Credential = CONTAINING_RECORD( ListEntry, SSP_CREDENTIAL, Next );


        //
        // Found a match ... reference this Credential
        // (if the Credential is being removed, we would increment
        // and then decrement the reference, so don't bother doing
        // either - since they cancel each other out).
        //

        if ( Credential == (PSSP_CREDENTIAL) CredentialHandle
            ) {

            // Make sure we have the privilege of accessing
            // this handle

            if (!ClientInfo.HasTcbPrivilege &&
               (Credential->ClientProcessID != ClientInfo.ProcessID)
               )
            {
                break;
            }

            if (!DereferenceCredential) {
                Credential->References += 1;
            } else {

                ASSERT( DereferenceCount > 0 );

                //
                // we've eliminated the two separate ref counters.
                // therefore, we let SspCredentialDereferenceCredential
                // take care of reducing the refcount.
                //

                DereferenceCount--;

                //
                // Decremenent the credential references, indicating
                // that a call to free

                ASSERT( DereferenceCount <= Credential->References );
                Credential->References -= DereferenceCount;

                ASSERT( Credential->References > 0 );

            }

            LeaveCriticalSection( &SspCredentialCritSect );

            *UserCredential = Credential ;

            return STATUS_SUCCESS ;

        }

    }


    //
    // No match found
    //
    SspPrint(( SSP_API_MORE, "Tried to reference unknown Credential 0x%lx\n",
               CredentialHandle ));

    LeaveCriticalSection( &SspCredentialCritSect );

    return STATUS_INVALID_HANDLE ;

}

NTSTATUS
SspGetToken (
    OUT PHANDLE ReturnedTokenHandle)
{
    HANDLE TokenHandle = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE TmpHandle = NULL;
    SECURITY_IMPERSONATION_LEVEL SecImpLevel = SecurityImpersonation;

    Status = LsaFunctions->ImpersonateClient();

    if (!NT_SUCCESS (Status))
    {
            goto Cleanup;
    }

    // get the token
    // note: there MUST be an impersonation token.
    // LsaFunctions->ImpersonateClient will call ImpersonateSelf() when necessary

    Status = NtOpenThreadToken(NtCurrentThread(),
                               TOKEN_DUPLICATE |
                               TOKEN_QUERY |
                               WRITE_DAC,
                               TRUE,
                               &TokenHandle);

    if (!NT_SUCCESS (Status))
    {
            goto Cleanup;
    }

    Status = SspDuplicateToken(TokenHandle,
                               SecImpLevel,
                               &TmpHandle);

    if (!NT_SUCCESS (Status))
    {
            goto Cleanup;
    }

Cleanup:

    if (ReturnedTokenHandle != NULL)
    {
        if (!NT_SUCCESS (Status))
        {
            *ReturnedTokenHandle = NULL;
        }
        else
        {
            *ReturnedTokenHandle = TmpHandle;
            TmpHandle = NULL;
        }
    }

    if (TokenHandle != NULL)
    {
        NtClose(TokenHandle);
    }

    if (TmpHandle != NULL)
    {
        NtClose(TmpHandle);
    }

    // ignore return value, we may not have impersonated successfully..
    RevertToSelf();

    return Status;
}


NTSTATUS
SspCredentialGetPassword(
    IN PSSP_CREDENTIAL Credential,
    OUT PUNICODE_STRING Password
    )
/*++

Routine Description:

    This routine copies the password out of credential. It requires locking
    the credential list because other threads may be hiding/revealing the
    password and we need exclusive access to do that.

    NOTE: Locking is no longer required, because the caller is expected
    to NtLmDuplicateUnicodeString() the cipher text Password prior to
    passing it to this routine.  This change allows the following advantages:

    1. Avoid taking Credential list lock.
    2. Avoid having to avoid having to Re-hide the password after reveal.
    3. Avoid having to take locks elsewhere associated with hiding/revealing.


Arguments:

    Credential - Credential record to retrieve the password from.

    Password - UNICODE_STRING to store the password in.


Return Value:

    STATUS_NO_MEMORY - there was not enough memory to copy
        the password.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    if ( Credential->Password.Buffer != NULL ) {
        Status = NtLmDuplicateUnicodeString(
                        Password,
                        &Credential->Password
                        );
    } else {
        RtlInitUnicodeString(
            Password,
            NULL
            );
    }

    return(Status);
}


PSSP_CREDENTIAL
SspCredentialLookupSupplementalCredential(
    IN PLUID LogonId,
    IN ULONG CredentialUseFlags,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING DomainName,
    IN PUNICODE_STRING Password
    )

/*++

Routine Description:

    This routine walks the list of credentials for this client looking
    for one that has the same supplemental credentials as those passed
    in.  If it is found, its reference count is increased and a pointer
    to it is returned.


Arguments:

    UserName - User name to match.

    DomainName - Domain name to match.

    Password - Password to match.


Return Value:

    NULL - the Credential was not found.

    Otherwise - returns a pointer to the referenced credential.

--*/

{
    SspPrint((SSP_API_MORE, "Entering SspCredentialLookupSupplementalCredential\n"));
    NTSTATUS Status;
    PLIST_ENTRY ListEntry;
    PSSP_CREDENTIAL Credential = NULL;
    PLIST_ENTRY ListHead;
    SECPKG_CLIENT_INFO ClientInfo;
    SECPKG_CALL_INFO CallInfo ;


    Status = LsaFunctions->GetClientInfo(&ClientInfo);

    if (!NT_SUCCESS(Status)) {
        SspPrint(( SSP_CRITICAL, "SspCredentialLookupSupplementalCredential: GetClientInfo returned 0x%lx\n", Status));
        return(NULL);
    }

    if ( !LsaFunctions->GetCallInfo( &CallInfo ) )
    {
        SspPrint(( SSP_CRITICAL, "SspCredentialLookupSupplementalCredential: GetCallInfo returned FALSE\n" ));
        return NULL ;
    }


    //
    // Acquire exclusive access to the Credential list
    //

    EnterCriticalSection( &SspCredentialCritSect );

    ListHead = &SspCredentialList;

    //
    // Now walk the list of Credentials looking for a match.
    //

    for ( ListEntry = ListHead->Flink;
          ListEntry != ListHead;
          ListEntry = ListEntry->Flink ) {

        Credential = CONTAINING_RECORD( ListEntry, SSP_CREDENTIAL, Next );

        //
        // We are only looking for outbound credentials.
        //

        if ((Credential->CredentialUseFlags & SECPKG_CRED_OUTBOUND) == 0) {
            continue;
        }

        //
        // We only want credentials from the same caller
        //
        if (Credential->ClientProcessID != ClientInfo.ProcessID) {
            continue;
        }

        //
        // if the caller is from kernel mode, only return creds
        // granted to kernel mode
        //

        if ( ( CallInfo.Attributes & SECPKG_CALL_KERNEL_MODE ) != 0 )
        {
            if ( !Credential->KernelClient )
            {
                continue;
            }
        }

        //
        // Check for a match
        //

        // The credential use check was added because null session
        // credentials were being returned when default credentials
        // were being asked. This happened becuase RtlEqualUnicodeString
        // for NULL,0,0 and "",0,2 is TRUE

        if ( (CredentialUseFlags == Credential->CredentialUseFlags) &&
            AlterRtlEqualUnicodeString(
                UserName,
                &Credential->UserName,
                FALSE
                ) &&
            AlterRtlEqualUnicodeString(
                DomainName,
                &Credential->DomainName,
                FALSE
                ) &&
            RtlEqualLuid(
                LogonId,
                &Credential->LogonId
                )) {

            UNICODE_STRING RevealedPassword;
            BOOLEAN fMatch;

            Status = NtLmDuplicateUnicodeString( &RevealedPassword, &Credential->Password );
            if(!NT_SUCCESS( Status ) ) {
                SspPrint(( SSP_CRITICAL,
                "SspCredentialLookupSupplementalCredential: NtLmDuplicateUnicodeString returned %d\n",Status));
                // an allocation failure is catastrophic, bail out now.
                break;
            }

            SspRevealPassword(&RevealedPassword);

            fMatch = AlterRtlEqualUnicodeString(
                    Password,
                    &RevealedPassword,
                    FALSE
                    );

            if( RevealedPassword.Buffer != NULL ) {
                ZeroMemory( RevealedPassword.Buffer, RevealedPassword.Length );
                NtLmFree( RevealedPassword.Buffer );
            }

            if( fMatch ) {

                //
                // Found a match - reference the credential
                //


                //
                // Reference the credential and indicate that
                // it is in use as two different handles to the caller
                // (who may call FreeCredentialsHandle twice)
                //

                Credential->References++;

                LeaveCriticalSection( &SspCredentialCritSect );
                SspPrint((SSP_API_MORE, "Leaving SspCredentialLookupSupplementalCredential\n"));
                return Credential;

            }

        }

    }


    //
    // No match found
    //
    SspPrint(( SSP_API_MORE, "Tried to reference unknown Credential\n" ));

    LeaveCriticalSection( &SspCredentialCritSect );
    SspPrint((SSP_API_MORE, "Leaving SspCredentialLookupSupplementalCredential\n"));
    return NULL;

}


VOID
SspCredentialDereferenceCredential(
    IN PSSP_CREDENTIAL Credential
    )

/*++

Routine Description:

    This routine decrements the specified Credential's reference count.
    If the reference count drops to zero, then the Credential is deleted

Arguments:

    Credential - Points to the Credential to be dereferenced.


Return Value:

    None.

--*/

{
    ULONG References;

    //
    // Decrement the reference count
    //

    EnterCriticalSection( &SspCredentialCritSect );
    ASSERT( Credential->References >= 1 );

    References = -- Credential->References;

    //
    // If the count dropped to zero, then run-down the Credential
    //

    if ( References == 0 ) {

        LPWSTR DomainName = Credential->DomainName.Buffer;
        LPWSTR UserName = Credential->UserName.Buffer;
        LPWSTR Password = Credential->Password.Buffer;
        // note: Password.Length may contain run-encoding hint, so size may be illegal.
        DWORD cbPassword = Credential->Password.MaximumLength;
        HANDLE ClientTokenHandle = Credential->ClientTokenHandle;

        SspPrint(( SSP_API_MORE, "Deleting Credential 0x%lx\n",
                   Credential ));

        if (!Credential->Unlinked) {
            RemoveEntryList( &Credential->Next );
        }

        ZeroMemory( Credential, sizeof(SSP_CREDENTIAL) );

        LeaveCriticalSection( &SspCredentialCritSect );

        if ( DomainName ) {
            (VOID) NtLmFree( DomainName );
        }
        if ( UserName ) {
            (VOID) NtLmFree( UserName );
        }
        if ( Password ) {
            ZeroMemory( Password, cbPassword );
            (VOID) NtLmFree( Password );
        }

        if ( ClientTokenHandle ) {
            (VOID) NtClose( ClientTokenHandle );
        }

        (VOID) NtLmFree( Credential );

        return;
    }

    LeaveCriticalSection( &SspCredentialCritSect );

    return;

}

BOOLEAN
SsprCheckMachineLogon(
    IN  OUT PLUID pLogonId,
    IN      PUNICODE_STRING UserName,
    IN      PUNICODE_STRING DomainName,
    IN      PUNICODE_STRING Password
    )
/*++

Routine Description:

    This routine determines if the input credential matches a special
    machine account logon over-ride.

Return Value:

    TRUE - the intput credential was the special machine account logon over-ride.
           the pLogonId is updated to utilize the machine credential.

--*/
{
    UNICODE_STRING MachineAccountName;
    static LUID LogonIdSystem = SYSTEM_LUID ;
    BOOLEAN fMachineLogon = FALSE;

    MachineAccountName.Buffer = NULL;


    //
    // check if caller was system, and requested machine credential
    // eg: user=computername$, domain=NULL, password=NULL
    //

    if( !RtlEqualLuid( pLogonId, &LogonIdSystem ) )
    {
        return FALSE;
    }


    if( UserName->Buffer == NULL )
    {
        return FALSE;
    }

    if( DomainName->Buffer != NULL )
    {
        return FALSE;
    }

    if( Password->Buffer != NULL )
    {
        return FALSE;
    }


    EnterCriticalSection (&NtLmGlobalCritSect);

    MachineAccountName.Length = NtLmGlobalUnicodeComputerNameString.Length + sizeof(WCHAR);

    if( MachineAccountName.Length == UserName->Length )
    {
        MachineAccountName.MaximumLength = MachineAccountName.Length;
        MachineAccountName.Buffer = (PWSTR)NtLmAllocate( MachineAccountName.Length );

        if( MachineAccountName.Buffer != NULL )
        {
            RtlCopyMemory(  MachineAccountName.Buffer,
                            NtLmGlobalUnicodeComputerNameString.Buffer,
                            NtLmGlobalUnicodeComputerNameString.Length
                            );

            MachineAccountName.Buffer[ (MachineAccountName.Length / sizeof(WCHAR)) - 1 ] = L'$';
        }
    }

    LeaveCriticalSection (&NtLmGlobalCritSect);


    if( MachineAccountName.Buffer == NULL )
    {
        goto Cleanup;
    }

    if( RtlEqualUnicodeString( &MachineAccountName, UserName, TRUE ) )
    {
        //
        // yes, it's a machine account logon request, update the
        // requested LogonId to match our mapped logon session.
        //

        *pLogonId = NtLmGlobalLuidMachineLogon;
        fMachineLogon = TRUE;
    }


Cleanup:

    if( MachineAccountName.Buffer )
    {
        NtLmFree( MachineAccountName.Buffer );
    }

    return fMachineLogon;
}


NTSTATUS
SsprAcquireCredentialHandle(
    IN PHANDLE TokenHandle,
    IN PLUID LogonId,
    IN ULONG ClientProcessID,
    IN ULONG CredentialUseFlags,
    OUT PLSA_SEC_HANDLE CredentialHandle,
    OUT PTimeStamp Lifetime,
    IN OPTIONAL PUNICODE_STRING DomainName,
    IN OPTIONAL PUNICODE_STRING UserName,
    IN OPTIONAL PUNICODE_STRING Password
    )

/*++

Routine Description:

    This API allows applications to acquire a handle to pre-existing
    credentials associated with the user on whose behalf the call is made
    i.e. under the identity this application is running.  These pre-existing
    credentials have been established through a system logon not described
    here.  Note that this is different from "login to the network" and does
    not imply gathering of credentials.


    This API returns a handle to the credentials of a principal (user, client)
    as used by a specific security package.  This handle can then be used
    in subsequent calls to the Context APIs.  This API will not let a
    process obtain a handle to credentials that are not related to the
    process; i.e. we won't allow a process to grab the credentials of
    another user logged into the same machine.  There is no way for us
    to determine if a process is a trojan horse or not, if it is executed
    by the user.

Arguments:

    CredentialUseFlags - Flags indicating the way with which these
        credentials will be used.

        #define     CRED_INBOUND        0x00000001
        #define     CRED_OUTBOUND       0x00000002
        #define     CRED_BOTH           0x00000003

        The credentials created with CRED_INBOUND option can only be used
        for (validating incoming calls and can not be used for making accesses.

    CredentialHandle - Returned credential handle.

    Lifetime - Time that these credentials expire. The value returned in
        this field depends on the security package.

    DomainName, DomainNameSize, UserName, UserNameSize, Password, PasswordSize -
        Optional credentials for this user.

Return Value:

    STATUS_SUCCESS -- Call completed successfully

    SEC_E_PRINCIPAL_UNKNOWN -- No such principal
    SEC_E_NOT_OWNER -- caller does not own the specified credentials
    STATUS_NO_MEMORY -- Not enough memory

--*/

{
    SspPrint((SSP_API_MORE, "Entering SsprAcquireCredentialHandle\n"));

    NTSTATUS Status = STATUS_SUCCESS;
    PSSP_CREDENTIAL Credential = NULL;
    SECPKG_CALL_INFO CallInfo ;

    if ( !LsaFunctions->GetCallInfo( &CallInfo ) )
    {
        return STATUS_UNSUCCESSFUL ;
    }

    //
    // If this is an outbound credential, and supplemental credentials
    // were supplied, look to see if we have already
    // created one with this set of credentials. Note - this leaves
    // the credential referenced, so if we fail further down we need to
    // dereference the credential.
    //

    if ((CredentialUseFlags & SECPKG_CRED_OUTBOUND) != 0) {

        //
        // check if machine account logon over-ride.
        //

        SsprCheckMachineLogon(
                        LogonId,
                        UserName,
                        DomainName,
                        Password
                        );

        Credential = SspCredentialLookupSupplementalCredential(
                        LogonId,
                        CredentialUseFlags,
                        UserName,
                        DomainName,
                        Password
                        );

        //
        // If we're using a common set of data, free the captured stuff
        //

        if ( Credential )
        {
            if ( (UserName) && (UserName->Buffer) )
            {
                NtLmFree( UserName->Buffer );
                UserName->Buffer = NULL ;
            }

            if ( ( DomainName ) && (DomainName->Buffer) )
            {
                NtLmFree( DomainName->Buffer );
                DomainName->Buffer = NULL ;
            }

            if ( ( Password ) && ( Password->Buffer ) )
            {
                ZeroMemory( Password->Buffer, Password->Length );
                NtLmFree( Password->Buffer );
                Password->Buffer = NULL ;
            }

        }
    }

    //
    // If we didn't just find a credential, create one now.
    //

    if (Credential == NULL) {

        //
        // Allocate a credential block and initialize it.
        //

        Credential = (PSSP_CREDENTIAL)NtLmAllocate(sizeof(SSP_CREDENTIAL) );

        if ( Credential == NULL ) {
            Status = STATUS_NO_MEMORY;
            SspPrint((SSP_CRITICAL, "Error from NtLmAllocate 0x%lx\n", Status));
            goto Cleanup;
        }

        Credential->References = 1;
        Credential->ClientProcessID = ClientProcessID;
        Credential->CredentialUseFlags = CredentialUseFlags;
        Credential->Unlinked = FALSE;

        if ( ( CallInfo.Attributes & SECPKG_CALL_KERNEL_MODE ) != 0 )
        {
            Credential->KernelClient = TRUE ;
        }
        else
        {
            Credential->KernelClient = FALSE ;
        }

        //
        // Stick the token and logon ID in the credential
        //

        Credential->ClientTokenHandle = *TokenHandle;

        //
        // If the supplied access token is NULL, and the supplied LogonId
        // specifies the SYSTEM account, then we need to pickup a copy of
        // the SYSTEM access token.  This is required so subsequent calls
        // so usage of that credential can be treated as Local.  Otherwise,
        // subsequent calls in this scenario get treated as Anonymous
        // (no accesstoken, null creds)
        // See SsprHandleFirstCall() where it checks for NULL ClientTokenHandle
        // and NULL buffers for logic behind local calls.
        //

        if( Credential->ClientTokenHandle == NULL ) {

            LUID LogonIdSystem = SYSTEM_LUID ;

            //
            // do this only for SYSTEM originated calls when no creds specified.
            //

            if( RtlEqualLuid( LogonId, &LogonIdSystem ) &&
                NtLmGlobalAccessTokenSystem &&
                DomainName && DomainName->Buffer == NULL &&
                UserName && UserName->Buffer == NULL &&
                Password && Password->Buffer == NULL
                ) {

                Status = SspDuplicateToken(
                                NtLmGlobalAccessTokenSystem,
                                SecurityImpersonation,
                                &(Credential->ClientTokenHandle)
                                );
            }
        }


        //
        // for loopback, enable all privileges in token to be consistent
        // with kerberos and ntlm network logon.
        //

        if( (Credential->ClientTokenHandle != NULL) &&
            !(CredentialUseFlags & NTLM_CRED_NULLSESSION) &&
            (DomainName == NULL || DomainName->Length == 0) &&
            (UserName == NULL || UserName->Length == 0) &&
            (Password == NULL || Password->Length == 0)
            )
        {
            if(!EnableAllPrivileges( Credential->ClientTokenHandle )) {
                SspPrint((SSP_CRITICAL, "Failed to EnableAllPrivileges for loopback logon %lx\n", Credential->ClientTokenHandle));
            }
        }

        //
        // so we don't close it in SpAcquireCredentialsHandle
        //

        if( NT_SUCCESS( Status ) )
            *TokenHandle = NULL;

        Credential->LogonId = *LogonId;

        //
        // Stick the supplemental credentials into the credential.
        //

        if (ARGUMENT_PRESENT(DomainName))
        {
            Credential->DomainName = *DomainName;
        }

        if (ARGUMENT_PRESENT(UserName))
        {
            Credential->UserName = *UserName;
        }

        if (ARGUMENT_PRESENT(Password))
        {
            SspHidePassword(Password);
            Credential->Password = *Password;
        }

        //
        // Add it to the list of valid credential handles.
        //

        EnterCriticalSection( &SspCredentialCritSect );
        InsertHeadList( &SspCredentialList, &Credential->Next );
        LeaveCriticalSection( &SspCredentialCritSect );

        SspPrint((SSP_API_MORE, "Added Credential 0x%lx\n", Credential ));

        //
        // Don't bother dereferencing because we already set the
        // reference count to 1.
        //
    }

    //
    // Return output parameters to the caller.
    //

    *CredentialHandle = (LSA_SEC_HANDLE) Credential;

    *Lifetime = NtLmGlobalForever;

Cleanup:

    if ( !NT_SUCCESS(Status) ) {

        if ( Credential != NULL ) {
            (VOID)NtLmFree( Credential );
        }

    }


    SspPrint((SSP_API_MORE, "Leaving SsprAcquireCredentialHandle\n"));

    return Status;
}


NTSTATUS
SsprFreeCredentialHandle(
    IN ULONG_PTR CredentialHandle
    )

/*++

Routine Description:

    This API is used to notify the security system that the credentials are
    no longer needed and allows the application to free the handle acquired
    in the call described above. When all references to this credential
    set has been removed then the credentials may themselves be removed.

Arguments:


    CredentialHandle - Credential Handle obtained through
        AcquireCredentialHandle.

Return Value:


    STATUS_SUCCESS -- Call completed successfully

    SEC_E_NO_SPM -- Security Support Provider is not running
    STATUS_INVALID_HANDLE -- Credential Handle is invalid


--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PSSP_CREDENTIAL Credential;

    SspPrint(( SSP_API_MORE, "SspFreeCredentialHandle Entered\n" ));

    //
    // Find the referenced credential and delink it.
    //

    Status = SspCredentialReferenceCredential(
                            CredentialHandle,
                            TRUE,       // remove the instance of the credential
                            FALSE,
                            &Credential );

    if ( !NT_SUCCESS( Status ) )
    {
        goto Cleanup ;
    }

    //
    // Dereferencing the Credential will remove the client's reference
    // to it, causing it to be rundown if nobody else is using it.
    //

    SspCredentialDereferenceCredential( Credential );

    //
    // Free and locally used resources.
    //
Cleanup:


    SspPrint(( SSP_API_MORE, "SspFreeCredentialHandle returns 0x%lx\n", Status ));
    return Status;
}




NTSTATUS
SspCredentialInitialize(
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
    // Initialize the Credential list to be empty.
    //

    InitializeCriticalSection(&SspCredentialCritSect);
    InitializeListHead( &SspCredentialList );

    return STATUS_SUCCESS;

}




VOID
SspCredentialTerminate(
    VOID
    )

/*++

Routine Description:

    This function cleans up any dangling credentials.

Arguments:

    None.

Return Value:

    Status of the operation.

--*/

{
    NTSTATUS Status ;

    //
    // Drop any lingering Credentials
    //

    EnterCriticalSection( &SspCredentialCritSect );
    while ( !IsListEmpty( &SspCredentialList ) ) {
        ULONG_PTR CredentialHandle;
        PSSP_CREDENTIAL Credential;

        CredentialHandle =
            (LSA_SEC_HANDLE) CONTAINING_RECORD( SspCredentialList.Flink,
                                      SSP_CREDENTIAL,
                                      Next );

        //
        // note: we don't need to release and re-enter critsec because
        // the same thread can Enter and Leave a critical section multiple
        // times without dead-locking.
        //

        Status = SspCredentialReferenceCredential(
                                CredentialHandle,
                                TRUE,
                                TRUE,
                                &Credential );            // Remove Credential

        if ( Credential != NULL ) {
            SspCredentialDereferenceCredential(Credential);
        }

    }
    LeaveCriticalSection( &SspCredentialCritSect );


    //
    // Delete the critical section
    //

    DeleteCriticalSection(&SspCredentialCritSect);

    return;

}

BOOL
EnableAllPrivileges(
    IN  HANDLE ClientTokenHandle
    )
{
    PTOKEN_PRIVILEGES pPrivileges;
    BYTE FastBuffer[ 512 ];
    PBYTE SlowBuffer = NULL;
    DWORD cbPrivileges;
    BOOL fSuccess;

    pPrivileges = (PTOKEN_PRIVILEGES)FastBuffer;
    cbPrivileges = sizeof( FastBuffer );

    fSuccess = GetTokenInformation(
                ClientTokenHandle,
                TokenPrivileges,
                pPrivileges,
                cbPrivileges,
                &cbPrivileges
                );

    if( !fSuccess ) {

        if( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
            return FALSE;

        SlowBuffer = (PBYTE)NtLmAllocate( cbPrivileges );
        if( SlowBuffer == NULL )
            return FALSE;

        pPrivileges = (PTOKEN_PRIVILEGES)SlowBuffer;

        fSuccess = GetTokenInformation(
                        ClientTokenHandle,
                        TokenPrivileges,
                        pPrivileges,
                        cbPrivileges,
                        &cbPrivileges
                        );
    }


    if( fSuccess && pPrivileges->PrivilegeCount != 0 ) {
        DWORD indexPrivilege;

        for( indexPrivilege = 0 ;
             indexPrivilege < pPrivileges->PrivilegeCount ;
             indexPrivilege ++ )
        {
            pPrivileges->Privileges[ indexPrivilege ].Attributes |=
                SE_PRIVILEGE_ENABLED;
        }

        fSuccess = AdjustTokenPrivileges(
                        ClientTokenHandle,
                        FALSE,
                        pPrivileges,
                        0,
                        NULL,
                        NULL
                        );

        if( fSuccess && GetLastError() != ERROR_SUCCESS )
            fSuccess = FALSE;
    }

    if( SlowBuffer )
        NtLmFree( SlowBuffer );

    return fSuccess;

}
