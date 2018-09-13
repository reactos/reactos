/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Subject.c

Abstract:

    This Module implements services related to subject security context.
    These services are part of the services provided by the Reference Monitor
    component.

    FOR PERFORMANCE SAKE, THIS MODULE IS AWARE OF INTERNAL TOKEN OBJECT
    FORMATS.

Author:

    Jim Kelly       (JimK)      2-Aug-1990

Environment:

    Kernel Mode

Revision History:

--*/

#include "sep.h"
#include "seopaque.h"
#include "tokenp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,SeCaptureSubjectContext)
#pragma alloc_text(PAGE,SeLockSubjectContext)
#pragma alloc_text(PAGE,SeUnlockSubjectContext)
#pragma alloc_text(PAGE,SeReleaseSubjectContext)
#pragma alloc_text(PAGE,SepGetDefaultsSubjectContext)
#pragma alloc_text(PAGE,SepValidOwnerSubjectContext)
#endif


VOID
SeCaptureSubjectContext (
    OUT PSECURITY_SUBJECT_CONTEXT SubjectContext
    )

/*++

Routine Description:

    This routine takes a snapshot of the calling thread's security
    context (locking tokens as necessary to do so).  This function
    is intended to support the object manager and other components
    that utilize the reference monitor's access validation,
    privilege test, and audit generation services.

    A subject's security context should be captured before initiating
    access validation and should be released after audit messages
    are generated.  This is necessary to provide a consistent security
    context to all those services.

    After calling access validation, privilege test, and audit generation
    services, the captured context should be released as soon as possible
    using the SeReleaseSubjectContext() service.

Arguments:

    SubjectContext - Points to a SECURITY_SUBJECT_CONTEXT data structure
        to be filled in with a snapshot of the calling thread's security
        profile.

Return Value:

    none.

--*/

{

    PEPROCESS CurrentProcess;

    //PVOID Objects[2];

    BOOLEAN IgnoreCopyOnOpen;
    BOOLEAN IgnoreEffectiveOnly;

    PAGED_CODE();

    CurrentProcess = PsGetCurrentProcess();
    SubjectContext->ProcessAuditId = PsProcessAuditId( CurrentProcess );

    //
    // Get pointers to primary and impersonation tokens
    //

    SubjectContext->ClientToken = PsReferenceImpersonationToken(
                                      PsGetCurrentThread(),
                                      &IgnoreCopyOnOpen,
                                      &IgnoreEffectiveOnly,
                                      &(SubjectContext->ImpersonationLevel)
                                      );

    SubjectContext->PrimaryToken = PsReferencePrimaryToken(CurrentProcess);

    return;

}



VOID
SeLockSubjectContext(
    IN PSECURITY_SUBJECT_CONTEXT SubjectContext
    )

/*++

Routine Description:

    Acquires READ LOCKS on the primary and impersonation tokens
    in the passed SubjectContext.

    This call must be undone by a call to SeUnlockSubjectContext().

    No one outside of the SE component should need to acquire a
    write lock to a token.  Therefore there is no public interface
    to do this.

Arguments:

    SubjectContext - Points to a SECURITY_SUBJECT_CONTEXT data structure
        which points to a primary token and an optional impersonation token.

Return Value:

    None

--*/

{
    PAGED_CODE();

    SepAcquireTokenReadLock((PTOKEN)(SubjectContext->PrimaryToken));

    if (ARGUMENT_PRESENT(SubjectContext->ClientToken)) {

        SepAcquireTokenReadLock((PTOKEN)(SubjectContext->ClientToken));
    }

    return;
}



VOID
SeUnlockSubjectContext(
    IN PSECURITY_SUBJECT_CONTEXT SubjectContext
    )

/*++

Routine Description:

    Releases the read locks on the token(s) in the passed SubjectContext.

Arguments:

    SubjectContext - Points to a SECURITY_SUBJECT_CONTEXT data structure
        which points to a primary token and an optional impersonation token.

Return Value:

    None

--*/

{
    PAGED_CODE();

    SepReleaseTokenReadLock((PTOKEN)(SubjectContext->PrimaryToken));

    if (ARGUMENT_PRESENT(SubjectContext->ClientToken)) {

        SepReleaseTokenReadLock((PTOKEN)(SubjectContext->ClientToken));
    }


}



VOID
SeReleaseSubjectContext (
    IN PSECURITY_SUBJECT_CONTEXT SubjectContext
    )

/*++


Routine Description:

    This routine releases a subject security context previously captured by
    SeCaptureSubjectContext().

Arguments:

    SubjectContext - Points to a SECURITY_SUBJECT_CONTEXT data structure
        containing a subject's previously captured security context.

Return Value:

    none.

--*/

{
    PAGED_CODE();

    PsDereferencePrimaryToken( SubjectContext->PrimaryToken );
    SubjectContext->PrimaryToken = NULL;


    PsDereferenceImpersonationToken( SubjectContext->ClientToken );
    SubjectContext->ClientToken = NULL;

    return;

}

VOID
SepGetDefaultsSubjectContext(
    IN PSECURITY_SUBJECT_CONTEXT SubjectContext,
    OUT PSID *Owner,
    OUT PSID *Group,
    OUT PSID *ServerOwner,
    OUT PSID *ServerGroup,
    OUT PACL *Dacl
    )
/*++

Routine Description:

    This routine retrieves pointers to the default owner, primary group,
    and, if present, discretionary ACL of the provided subject security
    context.

Arguments:

    SubjectContext - Points to the subject security context whose default
        values are to be retrieved.

    Owner - Receives a pointer to the subject's default owner SID.  This
        value will always be returned as a non-zero pointer.  That is,
        a subject's security context must contain a owner SID.

    Group - Receives a pointer to the subject's default primary group SID.
        This value will always be returned as a non-zero pointer.  That is,
        a subject's security context must contain a primary group.

    Dacl - Receives a pointer to the subject's default discretionary ACL,
        if one is define for the subject.  Note that a subject security context
        does not have to include a default discretionary ACL.  In this case,
        this value will be returned as NULL.




Return Value:

    none.

--*/

{
    PTOKEN EffectiveToken;
    PTOKEN PrimaryToken;

    PAGED_CODE();

    if (ARGUMENT_PRESENT(SubjectContext->ClientToken)) {
        EffectiveToken = (PTOKEN)SubjectContext->ClientToken;
    } else {
        EffectiveToken = (PTOKEN)SubjectContext->PrimaryToken;
    }

    (*Owner) = EffectiveToken->UserAndGroups[EffectiveToken->DefaultOwnerIndex].Sid;

    (*Group) = EffectiveToken->PrimaryGroup;

    (*Dacl)  = EffectiveToken->DefaultDacl;

    PrimaryToken = (PTOKEN)SubjectContext->PrimaryToken;

    *ServerOwner = PrimaryToken->UserAndGroups[PrimaryToken->DefaultOwnerIndex].Sid;

    *ServerGroup = PrimaryToken->PrimaryGroup;

    return;
}


BOOLEAN
SepIdAssignableAsGroup(
    IN PACCESS_TOKEN AToken,
    IN PSID Group
    )
/*++

Routine Description:

    This routine checks to see whether the provided SID is one that
    may be assigned to be the default primary group in a token.

    The current criteria is that the passed SID be a group in the
    token, with no other restrictions.

Arguments:

    Token - Points to the token to be examined.

    Group - Points to the SID to be checked.

Return Value:

    TRUE - SID passed by be assigned as the default primary group in a token.

    FALSE - Passed SID may not be so assigned.

--*/

{
    ULONG Index;
    BOOLEAN Found = FALSE;
    PTOKEN Token;

    PAGED_CODE();

    Token = (PTOKEN)AToken;

    //
    // Let's make it invalid to assign a NULL primary group,
    // but we may need to revisit this.
    //

    if (Group == NULL) {
        return( FALSE );
    }

    SepAcquireTokenReadLock( Token );

    //
    //  Walk through the list of user and group IDs looking
    //  for a match to the specified SID.
    //

    Index = 0;
    while (Index < Token->UserAndGroupCount) {

        Found = RtlEqualSid(
                    Group,
                    Token->UserAndGroups[Index].Sid
                    );

        if ( Found ) {
            break;
        }

        Index += 1;
    }

    SepReleaseTokenReadLock( Token );

    return Found;
}


BOOLEAN
SepValidOwnerSubjectContext(
    IN PSECURITY_SUBJECT_CONTEXT SubjectContext,
    IN PSID Owner,
    IN BOOLEAN ServerObject
    )
/*++

Routine Description:

    This routine checks to see whether the provided SID is one the subject
    is authorized to assign as the owner of objects.  It will also check to
    see if the caller has SeRestorePrivilege, if so, the request is granted.

Arguments:

    SubjectContext - Points to the subject's security context.

    Owner - Points to the SID to be checked.



Return Value:

    none.

--*/

{

    ULONG Index;
    BOOLEAN Found;
    PTOKEN EffectiveToken;
    BOOLEAN Rc = FALSE;

    PAGED_CODE();

    //
    // It is invalid to assign a NULL owner, regardless of
    // whether you have SeRestorePrivilege or not.
    //

    if (Owner == NULL) {
        return( FALSE );
    }

    //
    // Allowable owners come from the primary if it's a server object.
    //

    if (!ServerObject && ARGUMENT_PRESENT(SubjectContext->ClientToken)) {
        EffectiveToken = (PTOKEN)SubjectContext->ClientToken;
    } else {
        EffectiveToken = (PTOKEN)SubjectContext->PrimaryToken;
    }


    //
    // If we're impersonating, make sure we're at TokenImpersonation
    // or above.  This prevents someone from setting the owner of an
    // object when impersonating at less Identify or Anonymous.
    //

    if (EffectiveToken->TokenType == TokenImpersonation) {

        if (EffectiveToken->ImpersonationLevel < SecurityImpersonation) {

            return( FALSE );

        }
    }

    SepAcquireTokenReadLock( EffectiveToken );

    //
    //  Walk through the list of user and group IDs looking
    //  for a match to the specified SID.  If one is found,
    //  make sure it may be assigned as an owner.
    //
    //  This code is similar to that performed to set the default
    //  owner of a token (NtSetInformationToken).
    //

    Index = 0;
    while (Index < EffectiveToken->UserAndGroupCount) {


        Found = RtlEqualSid(
                    Owner,
                    EffectiveToken->UserAndGroups[Index].Sid
                    );

        if ( Found ) {

            //
            // We may return success if the Sid is one that may be assigned
            // as an owner, or if the caller has SeRestorePrivilege
            //

            if ( SepIdAssignableAsOwner(EffectiveToken,Index) ) {

                SepReleaseTokenReadLock( EffectiveToken );
                Rc = TRUE;
                goto exit;

            } else {

                //
                // Rc is already set to FALSE, just exit.
                //

                SepReleaseTokenReadLock( EffectiveToken );
                goto exit;

            } //endif assignable


        }  //endif Found


        Index += 1;

    } //endwhile


    SepReleaseTokenReadLock( EffectiveToken );

exit:

    //
    // If we are going to fail this call, check for Restore privilege,
    // and succeed if he has it.
    //

    //
    // We should really have gotten PreviousMode from the caller, but we
    // didn't, so hard wire it to be user-mode here.
    //

    if ( Rc == FALSE ) {
        Rc = SeSinglePrivilegeCheck( SeRestorePrivilege, UserMode );
    }

    return Rc;
}



NTSTATUS
SeQueryAuthenticationIdSubjectContext(
    IN PSECURITY_SUBJECT_CONTEXT SubjectContext,
    OUT PLUID AuthenticationId
    )
/*++

    Routine Description:

        This routine returns the authentication ID for the effective token
        in a subject context

    Parameters:

        SubjectContext - The subject context to get the ID from

        AuthenticationId - Receives the authentication ID from the token

    Return Value:

        Errors from SeQueryAuthenticationidToken.

--*/
{
    NTSTATUS Status;

    SeLockSubjectContext( SubjectContext );


    Status = SeQueryAuthenticationIdToken(
                EffectiveToken(SubjectContext),
                AuthenticationId
                );

    SeUnlockSubjectContext( SubjectContext );

    return( Status );


}
