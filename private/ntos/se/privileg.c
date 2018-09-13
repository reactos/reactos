/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Privileg.c

Abstract:

    This Module implements the privilege check procedures.

Author:

    Robert Reichel      (robertre)     26-Nov-90

Environment:

    Kernel Mode

Revision History:

--*/

#include "tokenp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtPrivilegeCheck)
#pragma alloc_text(PAGE,SeCheckPrivilegedObject)
#pragma alloc_text(PAGE,SepPrivilegeCheck)
#pragma alloc_text(PAGE,SePrivilegeCheck)
#pragma alloc_text(PAGE,SeSinglePrivilegeCheck)
#endif


BOOLEAN
SepPrivilegeCheck(
    IN PTOKEN Token,
    IN OUT PLUID_AND_ATTRIBUTES RequiredPrivileges,
    IN ULONG RequiredPrivilegeCount,
    IN ULONG PrivilegeSetControl,
    IN KPROCESSOR_MODE PreviousMode
    )
/*++

Routine Description:

    Worker routine for SePrivilegeCheck

Arguments:

    Token - The user's effective token.

    RequiredPrivileges - A privilege set describing the required
        privileges.  The UsedForAccess bits will be set in any privilege
        that is actually used (usually all of them).

    RequiredPrivilegeCount - How many privileges are in the
        RequiredPrivileges set.

    PrivilegeSetControl - Describes how many privileges are required.

    PreviousMode - The previous processor mode.

Return Value:

    Returns TRUE if requested privileges are granted, FALSE otherwise.

--*/

{
    PLUID_AND_ATTRIBUTES CurrentRequiredPrivilege;
    PLUID_AND_ATTRIBUTES CurrentTokenPrivilege;

    BOOLEAN RequiredAll;

    ULONG TokenPrivilegeCount;
    ULONG MatchCount = 0;

    ULONG i;
    ULONG j;

    PAGED_CODE();

    //
    //   Take care of kernel callers first
    //

    if (PreviousMode == KernelMode) {

         return(TRUE);

    }

    SepAcquireTokenReadLock( Token );

    TokenPrivilegeCount = Token->PrivilegeCount;

    //
    //   Save whether we require ALL of them or ANY
    //

    RequiredAll = (BOOLEAN)(PrivilegeSetControl & PRIVILEGE_SET_ALL_NECESSARY);

    for ( i = 0 , CurrentRequiredPrivilege = RequiredPrivileges ;
          i < RequiredPrivilegeCount ;
          i++, CurrentRequiredPrivilege++ ) {

         for ( j = 0, CurrentTokenPrivilege = Token->Privileges;
               j < TokenPrivilegeCount ;
               j++, CurrentTokenPrivilege++ ) {

              if ((CurrentTokenPrivilege->Attributes & SE_PRIVILEGE_ENABLED) &&
                   (RtlEqualLuid(&CurrentTokenPrivilege->Luid,
                                 &CurrentRequiredPrivilege->Luid))
                 ) {

                       CurrentRequiredPrivilege->Attributes |=
                                                SE_PRIVILEGE_USED_FOR_ACCESS;
                       MatchCount++;
                       break;     // start looking for next one
              }

         }

    }

    SepReleaseTokenReadLock( Token );

    //
    //   If we wanted ANY and didn't get any, return failure.
    //

    if (!RequiredAll && (MatchCount == 0)) {

         return (FALSE);

    }

    //
    // If we wanted ALL and didn't get all, return failure.
    //

    if (RequiredAll && (MatchCount != RequiredPrivilegeCount)) {

         return(FALSE);
    }

    return(TRUE);

}




BOOLEAN
SePrivilegeCheck(
    IN OUT PPRIVILEGE_SET RequiredPrivileges,
    IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
    IN KPROCESSOR_MODE AccessMode
    )
/*++

Routine Description:

    This routine checks to see if the token contains the specified
    privileges.

Arguments:

    RequiredPrivileges - Points to a set of privileges.  The subject's
        security context is to be checked to see which of the specified
        privileges are present.  The results will be indicated in the
        attributes associated with each privilege.  Note that
        flags in this parameter indicate whether all the privileges listed
        are needed, or any of the privileges.

    SubjectSecurityContext - A pointer to the subject's captured security
        context.

    AccessMode - Indicates the access mode to use for access check.  One of
        UserMode or KernelMode.  If the mode is kernel, then all privileges
        will be marked as being possessed by the subject, and successful
        completion status is returned.


Return Value:

    BOOLEAN - TRUE if all specified privileges are held by the subject,
    otherwise FALSE.


--*/

{
    BOOLEAN Status;

    PAGED_CODE();

    //
    // If we're impersonating a client, we have to be at impersonation level
    // of SecurityImpersonation or above.
    //

    if ( (SubjectSecurityContext->ClientToken != NULL) &&
         (SubjectSecurityContext->ImpersonationLevel < SecurityImpersonation)
       ) {

           return(FALSE);
    }

    //
    // SepPrivilegeCheck locks the passed token for read access
    //

    Status = SepPrivilegeCheck(
                 EffectiveToken( SubjectSecurityContext ),
                 RequiredPrivileges->Privilege,
                 RequiredPrivileges->PrivilegeCount,
                 RequiredPrivileges->Control,
                 AccessMode
                 );

    return(Status);
}



NTSTATUS
NtPrivilegeCheck(
    IN HANDLE ClientToken,
    IN OUT PPRIVILEGE_SET RequiredPrivileges,
    OUT PBOOLEAN Result
    )

/*++

Routine Description:

    This routine tests the caller's client's security context to see if it
    contains the specified privileges.

    This API requires the caller have SeTcbPrivilege privilege.  The test
    for this privilege is always against the primary token of the calling
    process, not the impersonation token of the thread.


Arguments:

    ClientToken - A handle to a token object representing a client
        attempting access.  This handle must be obtained from a
        communication session layer, such as from an LPC Port or Local
        Named Pipe, to prevent possible security policy violations.

    RequiredPrivileges - Points to a set of privileges.  The client's
        security context is to be checked to see which of the specified
        privileges are present.  The results will be indicated in the
        attributes associated with each privilege.  Note that
        flags in this parameter indicate whether all the privileges listed
        are needed, or any of the privileges.

    Result - Receives a boolean flag indicating whether the client has all
        the specified privileges or not.  A value of TRUE indicates the
        client has all the specified privileges.  Otherwise a value of
        FALSE is returned.



Return Value:

    STATUS_SUCCESS - Indicates the call completed successfully.

    STATUS_PRIVILEGE_NOT_HELD - Indicates the caller does not have
        sufficient privilege to use this privileged system service.

--*/



{
    BOOLEAN BStatus;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    PLUID_AND_ATTRIBUTES CapturedPrivileges = NULL;
    PTOKEN Token;
    ULONG CapturedPrivilegeCount;
    ULONG CapturedPrivilegesLength;
    ULONG ParameterLength;
    ULONG PrivilegeSetControl;

    PAGED_CODE();

    PreviousMode = KeGetPreviousMode();

    Status = ObReferenceObjectByHandle(
         ClientToken,             // Handle
         TOKEN_QUERY,             // DesiredAccess
         SepTokenObjectType,      // ObjectType
         PreviousMode,            // AccessMode
         (PVOID *)&Token,         // Object
         NULL                     // GrantedAccess
         );

    if ( !NT_SUCCESS(Status) ) {
         return Status;

    }

    //
    // If the passed token is an impersonation token, make sure
    // it is at SecurityIdentification or above.
    //

    if (Token->TokenType == TokenImpersonation) {

        if (Token->ImpersonationLevel < SecurityIdentification) {

            ObDereferenceObject( (PVOID)Token );

            return( STATUS_BAD_IMPERSONATION_LEVEL );

        }
    }

    try  {

         //
         // Capture passed Privilege Set
         //

         ProbeForWrite(
             RequiredPrivileges,
             sizeof(PRIVILEGE_SET),
             sizeof(ULONG)
             );

         CapturedPrivilegeCount = RequiredPrivileges->PrivilegeCount;

         if (!IsValidElementCount(CapturedPrivilegeCount, LUID_AND_ATTRIBUTES)) {
             Status = STATUS_INVALID_PARAMETER;
             leave;
         }
         ParameterLength = (ULONG)sizeof(PRIVILEGE_SET) +
                           ((CapturedPrivilegeCount - ANYSIZE_ARRAY) *
                             (ULONG)sizeof(LUID_AND_ATTRIBUTES)  );

         ProbeForWrite(
             RequiredPrivileges,
             ParameterLength,
             sizeof(ULONG)
             );


         ProbeForWriteBoolean(Result);

         PrivilegeSetControl = RequiredPrivileges->Control;


    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    if (!NT_SUCCESS(Status)) {
        ObDereferenceObject( (PVOID)Token );
        return Status;

    }

    Status = SeCaptureLuidAndAttributesArray(
                    (RequiredPrivileges->Privilege),
                    CapturedPrivilegeCount,
                    UserMode,
                    NULL, 0,
                    PagedPool,
                    TRUE,
                    &CapturedPrivileges,
                    &CapturedPrivilegesLength
                    );

    if (!NT_SUCCESS(Status)) {

        ObDereferenceObject( (PVOID)Token );
        return Status;
    }

    BStatus = SepPrivilegeCheck(
                  Token,                   // Token,
                  CapturedPrivileges,      // RequiredPrivileges,
                  CapturedPrivilegeCount,  // RequiredPrivilegeCount,
                  PrivilegeSetControl,     // PrivilegeSetControl
                  PreviousMode             // PreviousMode
                  );

    ObDereferenceObject( Token );


    try {

        //
        // copy the modified privileges buffer back to user
        //

        RtlMoveMemory(
            RequiredPrivileges->Privilege,
            CapturedPrivileges,
            CapturedPrivilegesLength
            );

        *Result = BStatus;

        } except (EXCEPTION_EXECUTE_HANDLER) {

            SeReleaseLuidAndAttributesArray(
               CapturedPrivileges,
               PreviousMode,
               TRUE
               );

            return(GetExceptionCode());

        }

    SeReleaseLuidAndAttributesArray(
        CapturedPrivileges,
        PreviousMode,
        TRUE
        );

    return( STATUS_SUCCESS );
}



BOOLEAN
SeSinglePrivilegeCheck(
    LUID PrivilegeValue,
    KPROCESSOR_MODE PreviousMode
    )

/*++

Routine Description:

    This function will check for the passed privilege value in the
    current context.

Arguments:

    PrivilegeValue - The value of the privilege being checked.


Return Value:

    TRUE - The current subject has the desired privilege.

    FALSE - The current subject does not have the desired privilege.
--*/

{
    BOOLEAN AccessGranted;
    PRIVILEGE_SET RequiredPrivileges;
    SECURITY_SUBJECT_CONTEXT SubjectSecurityContext;

    PAGED_CODE();

    //
    // Make sure the caller has the privilege to make this
    // call.
    //

    RequiredPrivileges.PrivilegeCount = 1;
    RequiredPrivileges.Control = PRIVILEGE_SET_ALL_NECESSARY;
    RequiredPrivileges.Privilege[0].Luid = PrivilegeValue;
    RequiredPrivileges.Privilege[0].Attributes = 0;

    SeCaptureSubjectContext( &SubjectSecurityContext );

    AccessGranted = SePrivilegeCheck(
                        &RequiredPrivileges,
                        &SubjectSecurityContext,
                        PreviousMode
                        );

    if ( PreviousMode != KernelMode ) {

        SePrivilegedServiceAuditAlarm (
            NULL,                              // BUGWARNING need service name
            &SubjectSecurityContext,
            &RequiredPrivileges,
            AccessGranted
            );
    }


    SeReleaseSubjectContext( &SubjectSecurityContext );

    return( AccessGranted );

}


BOOLEAN
SeCheckPrivilegedObject(
    LUID PrivilegeValue,
    HANDLE ObjectHandle,
    ACCESS_MASK DesiredAccess,
    KPROCESSOR_MODE PreviousMode
    )

/*++

Routine Description:

    This function will check for the passed privilege value in the
    current context, and generate audits as appropriate.

Arguments:

    PrivilegeValue - The value of the privilege being checked.

    Object - Specifies a pointer to the object being accessed.

    ObjectHandle - Specifies the object handle being used.

    DesiredAccess - The desired access mask, if any

    PreviousMode - The previous processor mode


Return Value:

    TRUE - The current subject has the desired privilege.

    FALSE - The current subject does not have the desired privilege.
--*/

{
    BOOLEAN AccessGranted;
    PRIVILEGE_SET RequiredPrivileges;
    SECURITY_SUBJECT_CONTEXT SubjectSecurityContext;

    PAGED_CODE();

    //
    // Make sure the caller has the privilege to make this
    // call.
    //

    RequiredPrivileges.PrivilegeCount = 1;
    RequiredPrivileges.Control = PRIVILEGE_SET_ALL_NECESSARY;
    RequiredPrivileges.Privilege[0].Luid = PrivilegeValue;
    RequiredPrivileges.Privilege[0].Attributes = 0;

    SeCaptureSubjectContext( &SubjectSecurityContext );

    AccessGranted = SePrivilegeCheck(
                        &RequiredPrivileges,
                        &SubjectSecurityContext,
                        PreviousMode
                        );

    if ( PreviousMode != KernelMode ) {

        SePrivilegeObjectAuditAlarm(
            ObjectHandle,
            &SubjectSecurityContext,
            DesiredAccess,
            &RequiredPrivileges,
            AccessGranted,
            PreviousMode
            );

    }


    SeReleaseSubjectContext( &SubjectSecurityContext );

    return( AccessGranted );

}
