/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    security.c

Abstract:

    This module implements the security related portions of the process
    structure.


Author:

    Mark Lucovsky (markl) 25-Apr-1989
    Jim Kelly (JimK) 2-August-1990

Revision History:

--*/

#include "psp.h"


PACCESS_TOKEN
PsReferencePrimaryToken(
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This function returns a pointer to the primary token of a process.
    The reference count of that primary token is incremented to protect
    the pointer returned.

    When the pointer is no longer needed, it should be freed using
    PsDereferencePrimaryToken().


Arguments:

    Process - Supplies the address of the process whose primary token
        is to be referenced.

Return Value:

    A pointer to the specified process's primary token.

--*/

{
    PACCESS_TOKEN
        Token;

    ASSERT( Process->Pcb.Header.Type == ProcessObject );

    //
    //  For performance sake, we may want to change this to use
    //  an executive interlocked add routine in the future.
    //


    //
    //  Lock the process security fields.
    //

    PsLockProcessSecurityFields();

    //
    //  Grab the current token pointer value
    //

    Token = Process->Token;

    //
    //  Increment the reference count of the primary token to protect our
    //  pointer.
    //

    ObReferenceObject(Token);

    //
    //  Release the process security fields
    //

    PsFreeProcessSecurityFields();

    return Token;

}

PACCESS_TOKEN
PsReferenceImpersonationToken(
    IN PETHREAD Thread,
    OUT PBOOLEAN CopyOnOpen,
    OUT PBOOLEAN EffectiveOnly,
    OUT PSECURITY_IMPERSONATION_LEVEL ImpersonationLevel
    )

/*++

Routine Description:

    This function returns a pointer to the impersonation token of a thread.
    The reference count of that impersonation token is incremented to protect
    the pointer returned.

    If the thread is not currently impersonating a client, then a null pointer
    is returned.

    If the thread is impersonating a client, then information about the
    means of impersonation are also returned (ImpersonationLevel).

    If a non-null value is returned, then PsDereferenceImpersonationToken()
    must be called to decrement the token's reference count when the pointer
    is no longer needed.


Arguments:

    Thread - Supplies the address of the thread whose impersonation token
        is to be referenced.

    CopyOnOpen - The current value of the Thread->ImpersonationInfo->CopyOnOpen field.

    EffectiveOnly - The current value of the Thread->ImpersonationInfo->EffectiveOnly field.

    ImpersonationLevel - The current value of the Thread->ImpersonationInfo->ImpersonationLevel
        field.

Return Value:

    A pointer to the specified thread's impersonation token.

    If the thread is not currently impersonating a client, then NULL is
    returned.

--*/

{
    PACCESS_TOKEN
        Token;

    ASSERT( Thread->Tcb.Header.Type == ThreadObject );
    //
    // before going through the lock overhead just look to see if it is
    // null. There is no race.  Grabbing the lock is not needed until
    // we decide to use the token at which point we re check to see it
    // it is null.
    // This check saves about 300 instructions.
    //

    if ( !Thread->ActiveImpersonationInfo ) {
        return NULL;
    }


    //
    //  Lock the process security fields.
    //

    PsLockProcessSecurityFields();

    //
    //  Grab the current token pointer value
    //


    if ( Thread->ActiveImpersonationInfo ) {

        //
        //  Return the thread's impersonation level, etc.
        //

        Token = Thread->ImpersonationInfo->Token;
        (*ImpersonationLevel) = Thread->ImpersonationInfo->ImpersonationLevel;
        (*CopyOnOpen) = Thread->ImpersonationInfo->CopyOnOpen;
        (*EffectiveOnly) = Thread->ImpersonationInfo->EffectiveOnly;


        //
        //  Increment the reference count of the token to protect our
        //  pointer.
        //

        ObReferenceObject(Token);

    } else {
        Token = NULL;
    }


    //
    //  Release the security fields.
    //

    PsFreeProcessSecurityFields();

    return Token;

}

PACCESS_TOKEN
PsReferenceEffectiveToken(
    IN PETHREAD Thread,
    OUT PTOKEN_TYPE TokenType,
    OUT PBOOLEAN EffectiveOnly,
    OUT PSECURITY_IMPERSONATION_LEVEL ImpersonationLevel
    )

/*++

Routine Description:

    This function returns a pointer to the effective token of a thread.  The
    effective token of a thread is the thread's impersonation token if it has
    one.  Otherwise, it is the primary token of the thread's process.

    The reference count of the effective token is incremented to protect
    the pointer returned.

    If the thread is impersonating a client, then the impersonation level
    is also returned.

    Either PsDereferenceImpersonationToken() (for an impersonation token) or
    PsDereferencePrimaryToken() (for a primary token) must be called to
    decrement the token's reference count when the pointer is no longer
    needed.


Arguments:

    Thread - Supplies the address of the thread whose effective token
        is to be referenced.

    TokenType - Receives the type of the effective token.  If the thread
        is currently impersonating a client, then this will be
        TokenImpersonation.  Othwerwise, it will be TokenPrimary.

    EffectiveOnly - If the token type is TokenImpersonation, then this
        receives the value of the client thread's Thread->Client->EffectiveOnly field.
        Otherwise, it is set to FALSE.

    ImpersonationLevel - The current value of the Thread->Client->ImpersonationLevel
        field for an impersonation token and is not set for a primary token.

Return Value:

    A pointer to the specified thread's effective token.

--*/

{
    PACCESS_TOKEN
        Token;

    ASSERT( Thread->Tcb.Header.Type == ThreadObject );

    //
    //  Lock the process security fields.
    //

    PsLockProcessSecurityFields();

    //
    //  Grab the current impersonation token pointer value
    //


    if ( Thread->ActiveImpersonationInfo ) {

        Token = Thread->ImpersonationInfo->Token;

        //
        //  Return the thread's impersonation level, etc.
        //

        (*TokenType) = TokenImpersonation;
        (*EffectiveOnly) = Thread->ImpersonationInfo->EffectiveOnly;
        (*ImpersonationLevel) = Thread->ImpersonationInfo->ImpersonationLevel;



    } else {

        //
        // Get the thread's primary token if it wasn't impersonating a client.
        //

        Token = THREAD_TO_PROCESS(Thread)->Token;


        //
        //  Only the TokenType and CopyOnOpen OUT parameters are
        //  returned for a primary token.
        //

        (*TokenType) = TokenPrimary;
        (*EffectiveOnly) = FALSE;

    }

    //
    //  Increment the reference count of the token to protect our
    //  pointer.
    //

    ObReferenceObject(Token);

    //
    //  Release the security fields.
    //

    PsFreeProcessSecurityFields();


    return Token;

}

NTSTATUS
PsOpenTokenOfThread(
    IN HANDLE ThreadHandle,
    IN BOOLEAN OpenAsSelf,
    OUT PACCESS_TOKEN *Token,
    OUT PBOOLEAN CopyOnOpen,
    OUT PBOOLEAN EffectiveOnly,
    OUT PSECURITY_IMPERSONATION_LEVEL ImpersonationLevel
    )

/*++

Routine Description:

    This function does the thread specific processing of
    an NtOpenThreadToken() service.

    The service validates that the handle has appropriate access
    to reference the thread.  If so, it goes on to increment
    the reference count of the token object to prevent it from
    going away while the rest of the NtOpenThreadToken() request
    is processed.

    NOTE: If this call completes successfully, the caller is responsible
          for decrementing the reference count of the target token.
          This must be done using PsDereferenceImpersonationToken().

Arguments:

    ThreadHandle - Supplies a handle to a thread object.

    OpenAsSelf - Is a boolean value indicating whether the access should
        be made using the calling thread's current security context, which
        may be that of a client (if impersonating), or using the caller's
        process-level security context.  A value of FALSE indicates the
        caller's current context should be used un-modified.  A value of
        TRUE indicates the request should be fulfilled using the process
        level security context.

    Token - If successful, receives a pointer to the thread's token
        object.

    CopyOnOpen - The current value of the Thread->Client->CopyOnOpen field.

    EffectiveOnly - The current value of the Thread->Client->EffectiveOnly field.

    ImpersonationLevel - The current value of the Thread->Client->ImpersonationLevel
        field.

Return Value:

    STATUS_SUCCESS - Indicates the call completed successfully.

    STATUS_NO_TOKEN - Indicates the referenced thread is not currently
        impersonating a client.

    STATUS_CANT_OPEN_ANONYMOUS - Indicates the client requested anonymous
        impersonation level.  An anonymous token can not be openned.

    status may also be any value returned by an attemp the reference
    the thread object for THREAD_QUERY_INFORMATION access.

--*/

{

    NTSTATUS
        Status;

    PETHREAD
        Thread;

    KPROCESSOR_MODE
        PreviousMode;

    SE_IMPERSONATION_STATE
        DisabledImpersonationState;

    BOOLEAN
        RestoreImpersonationState = FALSE;

    PreviousMode = KeGetPreviousMode();



    //
    // Disable impersonation if necessary
    //

    if (OpenAsSelf) {
         RestoreImpersonationState = PsDisableImpersonation(
                                         PsGetCurrentThread(),
                                         &DisabledImpersonationState
                                         );
    }

    //
    //  Make sure the handle grants the appropriate access to the specified
    //  thread.
    //

    Status = ObReferenceObjectByHandle(
                 ThreadHandle,
                 THREAD_QUERY_INFORMATION,
                 PsThreadType,
                 PreviousMode,
                 (PVOID *)&Thread,
                 NULL
                 );




    if (RestoreImpersonationState) {
        PsRestoreImpersonation(
            PsGetCurrentThread(),
            &DisabledImpersonationState
            );
    }

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    //  Reference the impersonation token, if there is one
    //

    (*Token) = PsReferenceImpersonationToken( Thread,
                                              CopyOnOpen,
                                              EffectiveOnly,
                                              ImpersonationLevel
                                              );


    //
    //  dereference the target thread.
    //

    ObDereferenceObject( Thread );

    //
    // Make sure there is a token
    //

    if ((*Token) == NULL) {
        return STATUS_NO_TOKEN;
    }

    //
    //  Make sure the ImpersonationLevel is high enough to allow
    //  the token to be openned.
    //

    if ((*ImpersonationLevel) <= SecurityAnonymous) {
        PsDereferenceImpersonationToken( (*Token) );
        (*Token) = NULL;
        return STATUS_CANT_OPEN_ANONYMOUS;
    }


    return STATUS_SUCCESS;

}


NTSTATUS
PsOpenTokenOfProcess(
    IN HANDLE ProcessHandle,
    OUT PACCESS_TOKEN *Token
    )

/*++

Routine Description:

    This function does the process specific processing of
    an NtOpenProcessToken() service.

    The service validates that the handle has appropriate access
    to referenced process.  If so, it goes on to reference the
    primary token object to prevent it from going away while the
    rest of the NtOpenProcessToken() request is processed.

    NOTE: If this call completes successfully, the caller is responsible
          for decrementing the reference count of the target token.
          This must be done using the PsDereferencePrimaryToken() API.

Arguments:

    ProcessHandle - Supplies a handle to a process object whose primary
        token is to be opened.

    Token - If successful, receives a pointer to the process's token
        object.

Return Value:

    STATUS_SUCCESS - Indicates the call completed successfully.

    status may also be any value returned by an attemp the reference
    the process object for PROCESS_QUERY_INFORMATION access.

--*/

{

    NTSTATUS
        Status;

    PEPROCESS
        Process;

    KPROCESSOR_MODE
        PreviousMode;


    PreviousMode = KeGetPreviousMode();

    //
    //  Make sure the handle grants the appropriate access to the specified
    //  process.
    //

    Status = ObReferenceObjectByHandle(
                 ProcessHandle,
                 PROCESS_QUERY_INFORMATION,
                 PsProcessType,
                 PreviousMode,
                 (PVOID *)&Process,
                 NULL
                 );

    if (!NT_SUCCESS(Status)) {

        return Status;

    }

    //
    //  Reference the primary token
    //  (This takes care of gaining exlusive access to the process
    //   security fields for us)
    //

    (*Token) = PsReferencePrimaryToken( Process );



    //
    // Done with the process object
    //

    ObDereferenceObject( Process );

    return STATUS_SUCCESS;


}

NTSTATUS
PsOpenTokenOfJobObject(
    IN HANDLE JobObject,
    OUT PACCESS_TOKEN * Token
    )

/*++

Routine Description:

    This function does the ps/job specific work for NtOpenJobObjectToken.


Arguments:

    JobObject - Supplies a handle to a job object whose limit token
        token is to be opened.

    Token - If successful, receives a pointer to the process's token
        object.

Return Value:

    STATUS_SUCCESS - Indicates the call completed successfully.

    STATUS_NO_TOKEN - indicates the job object does  not have a token


--*/
{
    NTSTATUS Status ;
    PEJOB Job ;
    KPROCESSOR_MODE PreviousMode ;

    PreviousMode = KeGetPreviousMode();

    Status = ObReferenceObjectByHandle(
                    JobObject,
                    JOB_OBJECT_QUERY,
                    PsJobType,
                    PreviousMode,
                    &Job,
                    NULL );

    if ( NT_SUCCESS( Status ) )
    {
        if ( Job->Token )
        {
            ObReferenceObject( Job->Token );

            *Token = Job->Token ;

        }
        else
        {
            Status = STATUS_NO_TOKEN ;
        }

    }

    return Status ;
}


NTSTATUS
PsImpersonateClient(
    IN PETHREAD Thread,
    IN PACCESS_TOKEN Token,
    IN BOOLEAN CopyOnOpen,
    IN BOOLEAN EffectiveOnly,
    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel
    )

/*++

Routine Description:

    This routine sets up the specified thread so that it is impersonating
    the specified client.  This will result in the reference count of the
    token representing the client being incremented to reflect the new
    reference.

    If the thread is currently impersonating a client, that token will be
    dereferenced.



Arguments:

    Thread - points to the thread which is going to impersonate a client.

    Token - Points to the token to be assigned as the impersonation token.
        This does NOT have to be a TokenImpersonation type token.  This
        allows direct reference of client process's primary tokens.

    CopyOnOpen - If TRUE, indicates the token is considered to be private
        by the assigner and should be copied if opened.  For example, a
        session layer may be using a token to represent a client's context.
        If the session is trying to synchronize the context of the client,
        then user mode code should not be given direct access to the session
        layer's token.

        Basically, session layers should always specify TRUE for this, while
        tokens assigned by the server itself (handle based) should specify
        FALSE.


    EffectiveOnly - Is a boolean value to be assigned as the
        Thread->ImpersonationInfo->EffectiveOnly field value for the
        impersonation.  A value of FALSE indicates the server is allowed
        to enable currently disabled groups and privileges.

    ImpersonationLevel - Is the impersonation level that the server is allowed
        to access the token with.


Return Value:

    STATUS_SUCCESS - Indicates the call completed successfully.


--*/

{

    PPS_IMPERSONATION_INFORMATION
        NewClient;

    PACCESS_TOKEN
        OldToken;

    PACCESS_TOKEN NewerToken ;
    NTSTATUS Status ;
    PPS_JOB_TOKEN_FILTER Filter ;
    BOOLEAN DontRefToken = FALSE ;

    ASSERT( Thread->Tcb.Header.Type == ThreadObject );


    //
    //  Lock the process security fields
    //

    PsLockProcessSecurityFields();


    if (!ARGUMENT_PRESENT(Token)) {

        //
        // This is a request to revert to self.
        // Clean up any client information.
        //

        if ( Thread->ActiveImpersonationInfo ) {

            OldToken = Thread->ImpersonationInfo->Token;
            Thread->ActiveImpersonationInfo = FALSE;
        } else {
            OldToken = NULL;
        }

    } else {

        //
        // Check if we're allowed to impersonate based on the job
        // restrictions:
        //

        Status = STATUS_SUCCESS ;

        if ( Thread->ThreadsProcess->Job ) {

            if ( ( Thread->ThreadsProcess->Job->SecurityLimitFlags & JOB_OBJECT_SECURITY_NO_ADMIN ) &&
                 ( SeTokenIsAdmin( Token ) ) ) {

                Status = STATUS_ACCESS_DENIED ;

            }

            if ( ( Thread->ThreadsProcess->Job->SecurityLimitFlags & JOB_OBJECT_SECURITY_RESTRICTED_TOKEN ) &&
                 ( !SeTokenIsRestricted( Token ) ) )
            {
                Status = STATUS_ACCESS_DENIED ;
            }

            if ( Thread->ThreadsProcess->Job->Filter )
            {
                //
                // Filter installed.  Need to create a restricted token
                // dynamically.
                //
                Filter = Thread->ThreadsProcess->Job->Filter ;

                Status = SeFastFilterToken(
                                Token,
                                KernelMode,
                                0,
                                Filter->CapturedGroupCount,
                                Filter->CapturedGroups,
                                Filter->CapturedPrivilegeCount,
                                Filter->CapturedPrivileges,
                                Filter->CapturedSidCount,
                                Filter->CapturedSids,
                                Filter->CapturedSidsLength,
                                &NewerToken );

                if ( NT_SUCCESS( Status ) )
                {
                    DontRefToken = TRUE ;

                    Token = NewerToken ;
                }

            }
        }

        if ( !NT_SUCCESS( Status ) )
        {
            PsFreeProcessSecurityFields();

            return Status ;
        }

        //
        // If we are already impersonating someone,
        // use the already allocated block.  This avoids
        // an alloc and a free.
        //

        if ( Thread->ActiveImpersonationInfo ) {

            //
            // capture the old token pointer.
            // We'll dereference it after unlocking the security fields.
            //

            OldToken = Thread->ImpersonationInfo->Token;
            NewClient = Thread->ImpersonationInfo;

        } else {

            OldToken = NULL;

            if ( Thread->ImpersonationInfo ) {
                NewClient = Thread->ImpersonationInfo;
            } else {

                //
                // Allocate and set up the Client block
                //

                NewClient = (PPS_IMPERSONATION_INFORMATION)ExAllocatePoolWithTag(
                                PagedPool,
                                sizeof(PS_IMPERSONATION_INFORMATION),
                                'mIsP');

                if (NewClient == NULL) {

                    PsFreeProcessSecurityFields();

                    return STATUS_NO_MEMORY ;

                }

                Thread->ImpersonationInfo = NewClient;
            }
        }

        NewClient->ImpersonationLevel = ImpersonationLevel;
        NewClient->EffectiveOnly = EffectiveOnly;
        NewClient->CopyOnOpen = CopyOnOpen;
        NewClient->Token = Token;
        Thread->ActiveImpersonationInfo = TRUE;

        if ( !DontRefToken )
        {
            ObReferenceObject(NewClient->Token);
        }
    }

    //
    //  Release the security fields
    //

    PsFreeProcessSecurityFields();


    //
    // Free the old client token, if necessary.
    //

    if (ARGUMENT_PRESENT( OldToken )) {

        PsDereferenceImpersonationToken( OldToken );
    }


    return STATUS_SUCCESS ;

}


BOOLEAN
PsDisableImpersonation(
    IN PETHREAD Thread,
    IN PSE_IMPERSONATION_STATE ImpersonationState
    )

/*++

Routine Description:

    This routine temporarily disables the impersonation of a thread.
    The impersonation state is saved for quick replacement later.  The
    impersonation token is left referenced and a pointer to it is held
    in the IMPERSONATION_STATE data structure.

    PsRestoreImpersonation() must be used after this routine is called.



Arguments:

    Thread - points to the thread whose impersonation (if any) is to
        be temporarily disabled.

    ImpersonationState - receives the current impersonation information,
        including a pointer to the impersonation token.


Return Value:

    TRUE - Indicates the impersonation state has been saved and the
        impersonation has been temporarily disabled.

    FALSE - Indicates the specified thread was not impersonating a client.
       No action has been taken.

--*/

{


    PPS_IMPERSONATION_INFORMATION
        OldClient;

    ASSERT( Thread->Tcb.Header.Type == ThreadObject );

    //
    //  Lock the process security fields
    //

    PsLockProcessSecurityFields();

    //
    //  Capture the impersonation information (if there is any).
    //

    if ( Thread->ActiveImpersonationInfo ) {

        OldClient = Thread->ImpersonationInfo;
        ImpersonationState->Level         = OldClient->ImpersonationLevel;
        ImpersonationState->EffectiveOnly = OldClient->EffectiveOnly;
        ImpersonationState->CopyOnOpen    = OldClient->CopyOnOpen;
        ImpersonationState->Token         = OldClient->Token;
    } else {

        //
        // Not impersonating.  Just make up some values.
        // The NULL for the token indicates we aren't impersonating.
        //

        OldClient = NULL;
        ImpersonationState->Level         = SecurityAnonymous;
        ImpersonationState->EffectiveOnly = FALSE;
        ImpersonationState->CopyOnOpen    = FALSE;
        ImpersonationState->Token         = NULL;
    }

    //
    // Clear the Client field to indicate the thread is not impersonating.
    //

    Thread->ActiveImpersonationInfo = FALSE;


    //
    //  Release the security fields
    //

    PsFreeProcessSecurityFields();


    if ( OldClient ) {

        return TRUE;

    } else {
        return FALSE;
    }

}


VOID
PsRestoreImpersonation(
    IN PETHREAD Thread,
    IN PSE_IMPERSONATION_STATE ImpersonationState
    )

/*++

Routine Description:

    This routine restores an impersonation that has been temporarily disabled
    using PsDisableImpersonation().

    Notice that if this routine finds the thread is already impersonating
    (again), then restoring the temporarily disabled impersonation will cause
    the current impersonation to be abandoned.



Arguments:

    Thread - points to the thread whose impersonation is to be restored.

    ImpersontionState - receives the current impersontion information,
        including a pointer ot the impersonation token.


Return Value:

    TRUE - Indicates the impersonation state has been saved and the
        impersonation has been temporarily disabled.

    FALSE - Indicates the specified thread was not impersonating a client.
       No action has been taken.

--*/

{


    //
    // The processing for restore is identical to that for Impersonation,
    // except that the token's reference count will be incremented (and
    // so we need to do one dereference).

    PsImpersonateClient(
        Thread,
        ImpersonationState->Token,
        ImpersonationState->CopyOnOpen,
        ImpersonationState->EffectiveOnly,
        ImpersonationState->Level
        );


    ObDereferenceObject( ImpersonationState->Token );

    return;

}


VOID
PsRevertToSelf( )

/*++

Routine Description:

    This routine causes the calling thread to discontinue
    impersonating a client.  If the thread is not currently
    impersonating a client, no action is taken.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PETHREAD
        Thread = PsGetCurrentThread();

    PACCESS_TOKEN OldToken;

    //
    //  Lock the process security fields
    //

    PsLockProcessSecurityFields();

    //
    //  See if the thread is impersonating a client
    //  and dereference that token if so.
    //

    if ( Thread->ActiveImpersonationInfo ) {

        Thread->ActiveImpersonationInfo = FALSE;
        OldToken = Thread->ImpersonationInfo->Token;
    } else {
        OldToken = NULL;
    }


    //
    //  Release the security fields
    //

    PsFreeProcessSecurityFields();


    //
    // Free the old client info...
    //

    if ( OldToken ) {
        ObDereferenceObject( OldToken );
    }

    return;
}


NTSTATUS
PspInitializeProcessSecurity(
    IN PEPROCESS Parent OPTIONAL,
    IN PEPROCESS Child
    )

/*++

Routine Description:

    This function initializes a new process's security fields, including
    the assignment of a new primary token.

    The child process is assumed to not yet have been inserted into
    an object table.

    NOTE: IT IS EXPECTED THAT THIS SERVICE WILL BE CALLED WITH A NULL
          PARENT PROCESS POINTER EXACTLY ONCE - FOR THE INITIAL SYSTEM
          PROCESS.


Arguments:

    Parent - An optional pointer to the process being used as the parent
        of the new process.  If this value is NULL, then the process is
        assumed to be the initial system process, and the boot token is
        assigned rather than a duplicate of the parent process's primary
        token.

    Child - Supplies the address of the process being initialized.  This
        process does not yet require security field contention protection.
        In particular, the security fields may be accessed without first
        acquiring the process security fields lock.



Return Value:


--*/

{
    NTSTATUS
        Status;


    //
    // Assign the primary token
    //

    if (ARGUMENT_PRESENT(Parent)) {

        //
        // create the primary token
        // This is a duplicate of the parent's token.
        //

        Status = SeSubProcessToken(
                     Parent,
                     &Child->Token
                     );

        if (!NT_SUCCESS(Status)) {
            Child->Token = NULL;       // Needed to ensure proper deletion
        }

    } else {

        //
        //  Reference and assign the boot token
        //
        //  The use of a single boot access token assumes there is
        //  exactly one parentless process in the system - the initial
        //  process.  If this ever changes, this code will need to change
        //  to match the new condition (so that a token doesn't end up
        //  being shared by multiple processes.
        //

        Child->Token = NULL;
        SeAssignPrimaryToken( Child, PspBootAccessToken );
        Status = STATUS_SUCCESS;


    }

    return Status;

}

VOID
PspDeleteProcessSecurity(
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This function cleans up a process's security fields as part of process
    deletion.  It is assumed no other references to the process can occur
    during or after a call to this routine.  This enables us to reference
    the process security fields without acquiring the lock protecting those
    fields.

    NOTE: It may be desirable to add auditing capability to this routine
          at some point.


Arguments:

    Process - A pointer to the process being deleted.


Return Value:

    None.

--*/

{




    //
    // If we are deleting a process that didn't successfully complete
    // process initialization, then there may be no token associated
    // with it yet.
    //

    if (ARGUMENT_PRESENT(Process->Token)) {
        SeDeassignPrimaryToken( Process );
        Process->Token = NULL;
    }

    return;
}


NTSTATUS
PspAssignPrimaryToken(
    IN PEPROCESS Process,
    IN HANDLE Token OPTIONAL,
    IN PACCESS_TOKEN TokenPointer OPTIONAL
    )

/*++

Routine Description:

    This function performs the security portions of primary token assignment.
    It is expected that the proper access to the process and thread objects,
    as well as necessary privilege, has already been established.

    A primary token can only be replaced if the process has no threads, or
    has one thread.  This is because the thread objects point to the primary
    token and must have those pointers updated when the primary token is
    changed.  This is only expected to be necessary at logon time, when
    the process is in its infancy and either has zero threads or maybe one
    inactive thread.

    If the assignment is successful, the old token is dereferenced and the
    new one is referenced.



Arguments:

    Process - A pointer to the process whose primary token is being
        replaced.

    Token - The handle value of the token to be assigned as the primary
        token.


Return Value:

    STATUS_SUCCESS - Indicates the primary token has been successfully
        replaced.

    STATUS_BAD_TOKEN_TYPE - Indicates the token is not of type TokenPrimary.

    STATUS_TOKEN_IN_USE - Indicates the token is already in use by
        another process.

    Other status may be returned when attempting to reference the token
    object.

--*/

{
    NTSTATUS
        Status;

    PACCESS_TOKEN
        NewToken,
        OldToken;

    KPROCESSOR_MODE
        PreviousMode;


    if ( TokenPointer == NULL )
    {
        PreviousMode = KeGetPreviousMode();

        //
        // Reference the specified token, and make sure it can be assigned
        // as a primary token.
        //

        Status = ObReferenceObjectByHandle (
                    Token,
                    TOKEN_ASSIGN_PRIMARY,
                    SeTokenObjectType(),
                    PreviousMode,
                    (PVOID *)&NewToken,
                    NULL
                    );

        if ( !NT_SUCCESS(Status) ) {
            return Status;
        }
    }
    else
    {
        NewToken = TokenPointer ;
    }


    //
    //  Lock the process security fields.
    //

    PsLockProcessSecurityFields();





    //
    // This routine makes sure the NewToken is suitable for assignment
    // as a primary token.
    //

    Status = SeExchangePrimaryToken( Process, NewToken, &OldToken );


    //
    //  Release the process security fields
    //

    PsFreeProcessSecurityFields();

    //
    // Free the old token (we don't need it).
    // This can't be done while the security fields are locked.
    //

    if (NT_SUCCESS( Status )) {

        ObDereferenceObject( OldToken );
    }

    //
    // Undo the handle reference
    //

    if ( TokenPointer == NULL )
    {
        ObDereferenceObject( NewToken );
    }


    return Status;
}


VOID
PspInitializeThreadSecurity(
    IN PEPROCESS Process,
    IN PETHREAD Thread
    )

/*++

Routine Description:

    This function initializes a new thread's security fields.


Arguments:

    Process - Points to the process the thread belongs to.

    Thread - Points to the thread object being initialized.


Return Value:

    None.

--*/

{


    //
    // Initially not impersonating anyone.
    //

    Thread->ImpersonationInfo = NULL;
    Thread->ActiveImpersonationInfo = FALSE;

    return;

}


VOID
PspDeleteThreadSecurity(
    IN PETHREAD Thread
    )

/*++

Routine Description:

    This function cleans up a thread's security fields as part of thread
    deletion.  It is assumed no other references to the thread can occur
    during or after a call to this routine, so no locking is necessary
    to access the thread security fields.


Arguments:

    Thread - A pointer to the thread being deleted.


Return Value:

    None.

--*/

{

    //
    // clean-up client information, if there is any.
    //

    if ( Thread->ActiveImpersonationInfo ) {
        ObDereferenceObject( Thread->ImpersonationInfo->Token );
    }

    if ( Thread->ImpersonationInfo ) {
        ExFreePool( Thread->ImpersonationInfo );
        Thread->ActiveImpersonationInfo = FALSE;
        Thread->ImpersonationInfo = NULL;
    }

    return;

}


NTSTATUS
PsAssignImpersonationToken(
    IN PETHREAD Thread,
    IN HANDLE Token
    )

/*++

Routine Description:

    This function performs the security portions of establishing an
    impersonation token.  This routine is expected to be used only in
    the case where the subject has asked for impersonation explicitly
    providing an impersonation token.  Other services are provided for
    use by communication session layers that need to establish an
    impersonation on a server's behalf.

    It is expected that the proper access to the thread object has already
    been established.

    The following rules apply:

         1) The caller must have TOKEN_IMPERSONATE access to the token
            for any action to be taken.

         2) If the token may NOT be used for impersonation (e.g., not an
            impersonation token) no action is taken.

         3) Otherwise, any existing impersonation token is dereferenced and
            the new token is established as the impersonation token.



Arguments:

    Thread - A pointer to the thread whose impersonation token is being
        set.

    Token - The handle value of the token to be assigned as the impersonation
        token.  If this value is NULL, then current impersonation (if any)
        is terminated and no new impersonation is established.


Return Value:

    STATUS_SUCCESS - Indicates the primary token has been successfully
        replaced.

    STATUS_BAD_TOKEN_TYPE - Indicates the token is not of type
        TokenImpersonation.

    Other status may be returned when attempting to reference the token
    object.

--*/

{
    NTSTATUS
        Status;

    PACCESS_TOKEN
        NewToken, NewerToken ;

    KPROCESSOR_MODE
        PreviousMode;

    SECURITY_IMPERSONATION_LEVEL
        ImpersonationLevel;

    PPS_JOB_TOKEN_FILTER Filter ;

    PEPROCESS ThreadProcess;
    BOOLEAN  AttachedToProcess = FALSE;

    PreviousMode = KeGetPreviousMode();

    if (!ARGUMENT_PRESENT(Token)) {

        NewToken = NULL;

        //
        // Don't care what value ImpersonationLevel has, it won't
        // be used.
        //

    } else {

        //
        // Reference the specified token for TOKEN_IMPERSONATE access
        //

        Status = ObReferenceObjectByHandle (
                    Token,
                    TOKEN_IMPERSONATE,
                    SeTokenObjectType(),
                    PreviousMode,
                    (PVOID *)&NewToken,
                    NULL
                    );

        if ( !NT_SUCCESS(Status) ) {
            return Status;
        }

        //
        // Make sure the token is an impersonation token.
        //

        if (SeTokenType( NewToken ) != TokenImpersonation ) {
            ObDereferenceObject( NewToken );
            return STATUS_BAD_TOKEN_TYPE;
        }

        if ( Thread->ThreadsProcess->Job ) {

            if ( ( Thread->ThreadsProcess->Job->SecurityLimitFlags & JOB_OBJECT_SECURITY_NO_ADMIN ) &&
                 ( SeTokenIsAdmin( NewToken ) ) ) {

                ObDereferenceObject( NewToken );

                return STATUS_ACCESS_DENIED ;

            }

            if ( ( Thread->ThreadsProcess->Job->SecurityLimitFlags & JOB_OBJECT_SECURITY_RESTRICTED_TOKEN ) &&
                 ( !SeTokenIsRestricted( NewToken ) ) )
            {
                ObDereferenceObject( NewToken );

                return STATUS_ACCESS_DENIED ;
            }

            if ( Thread->ThreadsProcess->Job->Filter )
            {
                //
                // Filter installed.  Need to create a restricted token
                // dynamically.
                //
                Filter = Thread->ThreadsProcess->Job->Filter ;

                Status = SeFastFilterToken(
                                NewToken,
                                PreviousMode,
                                0,
                                Filter->CapturedGroupCount,
                                Filter->CapturedGroups,
                                Filter->CapturedPrivilegeCount,
                                Filter->CapturedPrivileges,
                                Filter->CapturedSidCount,
                                Filter->CapturedSids,
                                Filter->CapturedSidsLength,
                                &NewerToken );

                ObDereferenceObject( NewToken );

                if ( NT_SUCCESS( Status ) )
                {
                    NewToken = NewerToken ;
                }
                else
                {
                    return Status ;
                }


            }
        }

        ImpersonationLevel = SeTokenImpersonationLevel( NewToken );
    }

    //
    // The rest can be done by PsImpersonateClient.
    //
    // PsImpersonateClient will reference the passed token
    // on success.
    //

    Status = PsImpersonateClient(
                 Thread,
                 NewToken,
                 FALSE,          // CopyOnOpen
                 FALSE,          //EffectiveOnly
                 ImpersonationLevel
                 );


    //
    // dereference the passed token, if there is one.
    //
    // Note that if PsImpersonateClient failed, this will
    // be the final dereference of NewToken/NewerToken,
    // and it will be freed.
    //

    if (ARGUMENT_PRESENT(NewToken)) {
        ObDereferenceObject( NewToken );
    }

    if (NT_SUCCESS( Status )) {

        //
        //   Indicate that 'Thread' has started to do impersonation.
        //   This info is useful for GetUserDefaultLCID()
        //

        if (!IS_SYSTEM_THREAD(Thread)) {

            ThreadProcess = THREAD_TO_PROCESS(Thread);

            if ( PsGetCurrentProcess() != ThreadProcess ) {
                KeAttachProcess( &ThreadProcess->Pcb );
                AttachedToProcess = TRUE;
            }

            if (ARGUMENT_PRESENT(NewToken)) {
                ((PTEB)(Thread->Tcb.Teb))->ImpersonationLocale = (LCID)-1;
                ((PTEB)(Thread->Tcb.Teb))->IsImpersonating = 1;
            } else {
                ((PTEB)(Thread->Tcb.Teb))->ImpersonationLocale = (LCID) 0;
                ((PTEB)(Thread->Tcb.Teb))->IsImpersonating = 0;
            }

            if (AttachedToProcess) {
                KeDetachProcess();
            }
        }
    }

    return Status;
}
