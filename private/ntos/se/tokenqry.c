/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Tokenqry.c

Abstract:

    This module implements the QUERY function for the executive
    token object.

Author:

    Jim Kelly (JimK) 15-June-1990


Revision History:

--*/

#include "sep.h"
#include "tokenp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtQueryInformationToken)
#pragma alloc_text(PAGE,SeQueryAuthenticationIdToken)
#pragma alloc_text(PAGE,SeQueryInformationToken)
#endif


NTSTATUS
NtQueryInformationToken (
    IN HANDLE TokenHandle,
    IN TOKEN_INFORMATION_CLASS TokenInformationClass,
    OUT PVOID TokenInformation,
    IN ULONG TokenInformationLength,
    OUT PULONG ReturnLength
    )

/*++


Routine Description:

    Retrieve information about a specified token.

Arguments:

    TokenHandle - Provides a handle to the token to operate on.

    TokenInformationClass - The token information class about which
        to retrieve information.

    TokenInformation - The buffer to receive the requested class of
        information.  The buffer must be aligned on at least a
        longword boundary.  The actual structures returned are
        dependent upon the information class requested, as defined in
        the TokenInformationClass parameter description.

        TokenInformation Format By Information Class:

           TokenUser => TOKEN_USER data structure.  TOKEN_QUERY
           access is needed to retrieve this information about a
           token.

           TokenGroups => TOKEN_GROUPS data structure.  TOKEN_QUERY
           access is needed to retrieve this information about a
           token.

           TokenPrivileges => TOKEN_PRIVILEGES data structure.
           TOKEN_QUERY access is needed to retrieve this information
           about a token.

           TokenOwner => TOKEN_OWNER data structure.  TOKEN_QUERY
           access is needed to retrieve this information about a
           token.

           TokenPrimaryGroup => TOKEN_PRIMARY_GROUP data structure.
           TOKEN_QUERY access is needed to retrieve this information
           about a token.

           TokenDefaultDacl => TOKEN_DEFAULT_DACL data structure.
           TOKEN_QUERY access is needed to retrieve this information
           about a token.

           TokenSource => TOKEN_SOURCE data structure.
           TOKEN_QUERY_SOURCE access is needed to retrieve this
           information about a token.

           TokenType => TOKEN_TYPE data structure.
           TOKEN_QUERY access is needed to retrieve this information
           about a token.

           TokenStatistics => TOKEN_STATISTICS data structure.
           TOKEN_QUERY access is needed to retrieve this
           information about a token.

           TokenGroups => TOKEN_GROUPS data structure.  TOKEN_QUERY
           access is needed to retrieve this information about a
           token.

    TokenInformationLength - Indicates the length, in bytes, of the
        TokenInformation buffer.

    ReturnLength - This OUT parameter receives the actual length of
        the requested information.  If this value is larger than that
        provided by the TokenInformationLength parameter, then the
        buffer provided to receive the requested information is not
        large enough to hold that data and no data is returned.

        If the queried class is TokenDefaultDacl and there is no
        default Dacl established for the token, then the return
        length will be returned as zero, and no data will be returned.

Return Value:

    STATUS_SUCCESS - Indicates the operation was successful.

    STATUS_BUFFER_TOO_SMALL - if the requested information did not
        fit in the provided output buffer.  In this case, the
        ReturnLength OUT parameter contains the number of bytes
        actually needed to store the requested information.

--*/
{

    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    PTOKEN Token;

    ULONG RequiredLength;
    ULONG Index;

    PTOKEN_TYPE LocalType;
    PTOKEN_USER LocalUser;
    PTOKEN_GROUPS LocalGroups;
    PTOKEN_PRIVILEGES LocalPrivileges;
    PTOKEN_OWNER LocalOwner;
    PTOKEN_PRIMARY_GROUP LocalPrimaryGroup;
    PTOKEN_DEFAULT_DACL LocalDefaultDacl;
    PTOKEN_SOURCE LocalSource;
    PSECURITY_IMPERSONATION_LEVEL LocalImpersonationLevel;
    PTOKEN_STATISTICS LocalStatistics;

    PSID PSid;
    PACL PAcl;

    PVOID Ignore;
    ULONG SessionId;

    PAGED_CODE();

    //
    // Get previous processor mode and probe output argument if necessary.
    //

    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {
        try {

            ProbeForWrite(
                TokenInformation,
                TokenInformationLength,
                sizeof(ULONG)
                );

            ProbeForWriteUlong(ReturnLength);

        } except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
    }

    //
    // Case on information class.
    //

    switch ( TokenInformationClass ) {

    case TokenUser:

        LocalUser = (PTOKEN_USER)TokenInformation;

        Status = ObReferenceObjectByHandle(
                 TokenHandle,           // Handle
                 TOKEN_QUERY,           // DesiredAccess
                 SepTokenObjectType,    // ObjectType
                 PreviousMode,          // AccessMode
                 (PVOID *)&Token,       // Object
                 NULL                   // GrantedAccess
                 );

        if ( !NT_SUCCESS(Status) ) {
            return Status;
        }

        //
        //  Gain exclusive access to the token.
        //

        SepAcquireTokenReadLock( Token );



        //
        // Return the length required now in case not enough buffer
        // was provided by the caller and we have to return an error.
        //

        RequiredLength = SeLengthSid( Token->UserAndGroups[0].Sid) +
                         (ULONG)sizeof( TOKEN_USER );

        try {

            *ReturnLength = RequiredLength;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }

        if ( TokenInformationLength < RequiredLength ) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return STATUS_BUFFER_TOO_SMALL;

        }

        //
        // Return the user SID
        //

        try {

            //
            //  Put SID immediately following TOKEN_USER data structure
            //
            PSid = (PSID)( (ULONG_PTR)LocalUser + (ULONG)sizeof(TOKEN_USER) );

            RtlCopySidAndAttributesArray(
                1,
                Token->UserAndGroups,
                RequiredLength,
                &(LocalUser->User),
                PSid,
                ((PSID *)&Ignore),
                ((PULONG)&Ignore)
                );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }


        SepReleaseTokenReadLock( Token );
        ObDereferenceObject( Token );
        return STATUS_SUCCESS;

    case TokenGroups:

        LocalGroups = (PTOKEN_GROUPS)TokenInformation;

        Status = ObReferenceObjectByHandle(
                 TokenHandle,           // Handle
                 TOKEN_QUERY,           // DesiredAccess
                 SepTokenObjectType,    // ObjectType
                 PreviousMode,          // AccessMode
                 (PVOID *)&Token,       // Object
                 NULL                   // GrantedAccess
                 );

        if ( !NT_SUCCESS(Status) ) {
            return Status;
        }

        //
        //  Gain exclusive access to the token.
        //

        SepAcquireTokenReadLock( Token );

        //
        // Figure out how much space is needed to return the group SIDs.
        // That's the size of TOKEN_GROUPS (without any array entries)
        // plus the size of an SID_AND_ATTRIBUTES times the number of groups.
        // The number of groups is Token->UserAndGroups-1 (since the count
        // includes the user ID).  Then the lengths of each individual group
        // must be added.
        //

        RequiredLength = (ULONG)sizeof(TOKEN_GROUPS) +
                         ((Token->UserAndGroupCount - ANYSIZE_ARRAY - 1) *
                         ((ULONG)sizeof(SID_AND_ATTRIBUTES)) );

        Index = 1;
        while (Index < Token->UserAndGroupCount) {

            RequiredLength += SeLengthSid( Token->UserAndGroups[Index].Sid );

            Index += 1;

        } // endwhile

        //
        // Return the length required now in case not enough buffer
        // was provided by the caller and we have to return an error.
        //

        try {

            *ReturnLength = RequiredLength;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }

        if ( TokenInformationLength < RequiredLength ) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return STATUS_BUFFER_TOO_SMALL;

        }

        //
        // Now copy the groups.
        //

        try {

            LocalGroups->GroupCount = Token->UserAndGroupCount - 1;

            PSid = (PSID)( (ULONG_PTR)LocalGroups +
                           (ULONG)sizeof(TOKEN_GROUPS) +
                           (   (Token->UserAndGroupCount - ANYSIZE_ARRAY - 1) *
                               (ULONG)sizeof(SID_AND_ATTRIBUTES) )
                         );

            RtlCopySidAndAttributesArray(
                (ULONG)(Token->UserAndGroupCount - 1),
                &(Token->UserAndGroups[1]),
                RequiredLength,
                LocalGroups->Groups,
                PSid,
                ((PSID *)&Ignore),
                ((PULONG)&Ignore)
                );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }


        SepReleaseTokenReadLock( Token );
        ObDereferenceObject( Token );
        return STATUS_SUCCESS;

    case TokenRestrictedSids:

        LocalGroups = (PTOKEN_GROUPS)TokenInformation;

        Status = ObReferenceObjectByHandle(
                 TokenHandle,           // Handle
                 TOKEN_QUERY,           // DesiredAccess
                 SepTokenObjectType,    // ObjectType
                 PreviousMode,          // AccessMode
                 (PVOID *)&Token,       // Object
                 NULL                   // GrantedAccess
                 );

        if ( !NT_SUCCESS(Status) ) {
            return Status;
        }

        //
        //  Gain exclusive access to the token.
        //

        SepAcquireTokenReadLock( Token );

        //
        // Figure out how much space is needed to return the group SIDs.
        // That's the size of TOKEN_GROUPS (without any array entries)
        // plus the size of an SID_AND_ATTRIBUTES times the number of groups.
        // The number of groups is Token->UserAndGroups-1 (since the count
        // includes the user ID).  Then the lengths of each individual group
        // must be added.
        //

        RequiredLength = (ULONG)sizeof(TOKEN_GROUPS) +
                         ((Token->RestrictedSidCount) *
                         ((ULONG)sizeof(SID_AND_ATTRIBUTES)) -
                         ANYSIZE_ARRAY * sizeof(SID_AND_ATTRIBUTES) );

        Index = 0;
        while (Index < Token->RestrictedSidCount) {

            RequiredLength += SeLengthSid( Token->RestrictedSids[Index].Sid );

            Index += 1;

        } // endwhile

        //
        // Return the length required now in case not enough buffer
        // was provided by the caller and we have to return an error.
        //

        try {

            *ReturnLength = RequiredLength;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }

        if ( TokenInformationLength < RequiredLength ) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return STATUS_BUFFER_TOO_SMALL;

        }

        //
        // Now copy the groups.
        //

        try {

            LocalGroups->GroupCount = Token->RestrictedSidCount;

            PSid = (PSID)( (ULONG_PTR)LocalGroups +
                           (ULONG)sizeof(TOKEN_GROUPS) +
                           (   (Token->RestrictedSidCount ) *
                               (ULONG)sizeof(SID_AND_ATTRIBUTES) -
                               ANYSIZE_ARRAY * sizeof(SID_AND_ATTRIBUTES) )
                         );

            RtlCopySidAndAttributesArray(
                (ULONG)(Token->RestrictedSidCount),
                Token->RestrictedSids,
                RequiredLength,
                LocalGroups->Groups,
                PSid,
                ((PSID *)&Ignore),
                ((PULONG)&Ignore)
                );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }


        SepReleaseTokenReadLock( Token );
        ObDereferenceObject( Token );
        return STATUS_SUCCESS;

    case TokenPrivileges:

        LocalPrivileges = (PTOKEN_PRIVILEGES)TokenInformation;

        Status = ObReferenceObjectByHandle(
                 TokenHandle,           // Handle
                 TOKEN_QUERY,           // DesiredAccess
                 SepTokenObjectType,    // ObjectType
                 PreviousMode,          // AccessMode
                 (PVOID *)&Token,       // Object
                 NULL                   // GrantedAccess
                 );

        if ( !NT_SUCCESS(Status) ) {
            return Status;
        }

        //
        //  Gain exclusive access to the token to prevent changes
        //  from occuring to the privileges.
        //

        SepAcquireTokenReadLock( Token );


        //
        // Return the length required now in case not enough buffer
        // was provided by the caller and we have to return an error.
        //

        RequiredLength = (ULONG)sizeof(TOKEN_PRIVILEGES) +
                         ((Token->PrivilegeCount - ANYSIZE_ARRAY) *
                         ((ULONG)sizeof(LUID_AND_ATTRIBUTES)) );


        try {

            *ReturnLength = RequiredLength;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }

        if ( TokenInformationLength < RequiredLength ) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return STATUS_BUFFER_TOO_SMALL;

        }

        //
        // Return the token privileges.
        //

        try {

            LocalPrivileges->PrivilegeCount = Token->PrivilegeCount;

            RtlCopyLuidAndAttributesArray(
                Token->PrivilegeCount,
                Token->Privileges,
                LocalPrivileges->Privileges
                );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }


        SepReleaseTokenReadLock( Token );
        ObDereferenceObject( Token );
        return STATUS_SUCCESS;

    case TokenOwner:

        LocalOwner = (PTOKEN_OWNER)TokenInformation;

        Status = ObReferenceObjectByHandle(
                 TokenHandle,           // Handle
                 TOKEN_QUERY,           // DesiredAccess
                 SepTokenObjectType,    // ObjectType
                 PreviousMode,          // AccessMode
                 (PVOID *)&Token,       // Object
                 NULL                   // GrantedAccess
                 );

        if ( !NT_SUCCESS(Status) ) {
            return Status;
        }

        //
        //  Gain exclusive access to the token to prevent changes
        //  from occuring to the owner.
        //

        SepAcquireTokenReadLock( Token );

        //
        // Return the length required now in case not enough buffer
        // was provided by the caller and we have to return an error.
        //

        PSid = Token->UserAndGroups[Token->DefaultOwnerIndex].Sid;
        RequiredLength = (ULONG)sizeof(TOKEN_OWNER) +
                         SeLengthSid( PSid );

        try {

            *ReturnLength = RequiredLength;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }

        if ( TokenInformationLength < RequiredLength ) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return STATUS_BUFFER_TOO_SMALL;

        }

        //
        // Return the owner SID
        //

        PSid = (PSID)((ULONG_PTR)LocalOwner +
                      (ULONG)sizeof(TOKEN_OWNER));

        try {

            LocalOwner->Owner = PSid;

            Status = RtlCopySid(
                         (RequiredLength - (ULONG)sizeof(TOKEN_OWNER)),
                         PSid,
                         Token->UserAndGroups[Token->DefaultOwnerIndex].Sid
                         );

            ASSERT( NT_SUCCESS(Status) );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }


        SepReleaseTokenReadLock( Token );
        ObDereferenceObject( Token );
        return STATUS_SUCCESS;

    case TokenPrimaryGroup:

        LocalPrimaryGroup = (PTOKEN_PRIMARY_GROUP)TokenInformation;

        Status = ObReferenceObjectByHandle(
                 TokenHandle,           // Handle
                 TOKEN_QUERY,           // DesiredAccess
                 SepTokenObjectType,    // ObjectType
                 PreviousMode,          // AccessMode
                 (PVOID *)&Token,       // Object
                 NULL                   // GrantedAccess
                 );

        if ( !NT_SUCCESS(Status) ) {
            return Status;
        }

        //
        //  Gain exclusive access to the token to prevent changes
        //  from occuring to the owner.
        //

        SepAcquireTokenReadLock( Token );

        //
        // Return the length required now in case not enough buffer
        // was provided by the caller and we have to return an error.
        //

        RequiredLength = (ULONG)sizeof(TOKEN_PRIMARY_GROUP) +
                         SeLengthSid( Token->PrimaryGroup );

        try {

            *ReturnLength = RequiredLength;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }

        if ( TokenInformationLength < RequiredLength ) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return STATUS_BUFFER_TOO_SMALL;

        }

        //
        // Return the primary group SID
        //

        PSid = (PSID)((ULONG_PTR)LocalPrimaryGroup +
                      (ULONG)sizeof(TOKEN_PRIMARY_GROUP));

        try {

            LocalPrimaryGroup->PrimaryGroup = PSid;

            Status = RtlCopySid( (RequiredLength - (ULONG)sizeof(TOKEN_PRIMARY_GROUP)),
                                 PSid,
                                 Token->PrimaryGroup
                                 );

            ASSERT( NT_SUCCESS(Status) );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }


        SepReleaseTokenReadLock( Token );
        ObDereferenceObject( Token );
        return STATUS_SUCCESS;

    case TokenDefaultDacl:

        LocalDefaultDacl = (PTOKEN_DEFAULT_DACL)TokenInformation;

        Status = ObReferenceObjectByHandle(
                 TokenHandle,           // Handle
                 TOKEN_QUERY,           // DesiredAccess
                 SepTokenObjectType,    // ObjectType
                 PreviousMode,          // AccessMode
                 (PVOID *)&Token,       // Object
                 NULL                   // GrantedAccess
                 );

        if ( !NT_SUCCESS(Status) ) {
            return Status;
        }

        //
        //  Gain exclusive access to the token to prevent changes
        //  from occuring to the owner.
        //

        SepAcquireTokenReadLock( Token );


        //
        // Return the length required now in case not enough buffer
        // was provided by the caller and we have to return an error.
        //

        RequiredLength = (ULONG)sizeof(TOKEN_DEFAULT_DACL);

        if (ARGUMENT_PRESENT(Token->DefaultDacl)) {

            RequiredLength += Token->DefaultDacl->AclSize;

        }

        try {

            *ReturnLength = RequiredLength;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }

        if ( TokenInformationLength < RequiredLength ) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return STATUS_BUFFER_TOO_SMALL;

        }

        //
        // Return the default Dacl
        //

        PAcl = (PACL)((ULONG_PTR)LocalDefaultDacl +
                      (ULONG)sizeof(TOKEN_DEFAULT_DACL));

        try {

            if (ARGUMENT_PRESENT(Token->DefaultDacl)) {

                LocalDefaultDacl->DefaultDacl = PAcl;

                RtlCopyMemory( (PVOID)PAcl,
                               (PVOID)Token->DefaultDacl,
                               Token->DefaultDacl->AclSize
                               );
            } else {

                LocalDefaultDacl->DefaultDacl = NULL;

            }

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }


        SepReleaseTokenReadLock( Token );
        ObDereferenceObject( Token );
        return STATUS_SUCCESS;



    case TokenSource:

        LocalSource = (PTOKEN_SOURCE)TokenInformation;

        Status = ObReferenceObjectByHandle(
                 TokenHandle,           // Handle
                 TOKEN_QUERY_SOURCE,    // DesiredAccess
                 SepTokenObjectType,    // ObjectType
                 PreviousMode,          // AccessMode
                 (PVOID *)&Token,       // Object
                 NULL                   // GrantedAccess
                 );

        if ( !NT_SUCCESS(Status) ) {
            return Status;
        }

        //
        // The type of a token can not be changed, so
        // exclusive access to the token is not necessary.
        //

        //
        // Return the length required now in case not enough buffer
        // was provided by the caller and we have to return an error.
        //

        RequiredLength = (ULONG) sizeof(TOKEN_SOURCE);

        try {

            *ReturnLength = RequiredLength;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            ObDereferenceObject( Token );
            return GetExceptionCode();
        }

        if ( TokenInformationLength < RequiredLength ) {

            ObDereferenceObject( Token );
            return STATUS_BUFFER_TOO_SMALL;

        }


        //
        // Return the token source
        //

        try {

            (*LocalSource) = Token->TokenSource;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            ObDereferenceObject( Token );
            return GetExceptionCode();
        }


        ObDereferenceObject( Token );
        return STATUS_SUCCESS;

    case TokenType:

        LocalType = (PTOKEN_TYPE)TokenInformation;

        Status = ObReferenceObjectByHandle(
                 TokenHandle,           // Handle
                 TOKEN_QUERY,           // DesiredAccess
                 SepTokenObjectType,    // ObjectType
                 PreviousMode,          // AccessMode
                 (PVOID *)&Token,       // Object
                 NULL                   // GrantedAccess
                 );

        if ( !NT_SUCCESS(Status) ) {
            return Status;
        }

        //
        // The type of a token can not be changed, so
        // exclusive access to the token is not necessary.
        //

        //
        // Return the length required now in case not enough buffer
        // was provided by the caller and we have to return an error.
        //

        RequiredLength = (ULONG) sizeof(TOKEN_TYPE);

        try {

            *ReturnLength = RequiredLength;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            ObDereferenceObject( Token );
            return GetExceptionCode();
        }

        if ( TokenInformationLength < RequiredLength ) {

            ObDereferenceObject( Token );
            return STATUS_BUFFER_TOO_SMALL;

        }


        //
        // Return the token type
        //

        try {

            (*LocalType) = Token->TokenType;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            ObDereferenceObject( Token );
            return GetExceptionCode();
        }


        ObDereferenceObject( Token );
        return STATUS_SUCCESS;


    case TokenImpersonationLevel:

        LocalImpersonationLevel = (PSECURITY_IMPERSONATION_LEVEL)TokenInformation;

        Status = ObReferenceObjectByHandle(
                 TokenHandle,           // Handle
                 TOKEN_QUERY,           // DesiredAccess
                 SepTokenObjectType,    // ObjectType
                 PreviousMode,          // AccessMode
                 (PVOID *)&Token,       // Object
                 NULL                   // GrantedAccess
                 );

        if ( !NT_SUCCESS(Status) ) {
            return Status;
        }

        //
        // The impersonation level of a token can not be changed, so
        // exclusive access to the token is not necessary.
        //

        //
        //  Make sure the token is an appropriate type to be retrieving
        //  the impersonation level from.
        //

        if (Token->TokenType != TokenImpersonation) {

            ObDereferenceObject( Token );
            return STATUS_INVALID_INFO_CLASS;

        }

        //
        // Return the length required now in case not enough buffer
        // was provided by the caller and we have to return an error.
        //

        RequiredLength = (ULONG) sizeof(SECURITY_IMPERSONATION_LEVEL);

        try {

            *ReturnLength = RequiredLength;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            ObDereferenceObject( Token );
            return GetExceptionCode();
        }

        if ( TokenInformationLength < RequiredLength ) {

            ObDereferenceObject( Token );
            return STATUS_BUFFER_TOO_SMALL;

        }


        //
        // Return the impersonation level
        //

        try {

            (*LocalImpersonationLevel) = Token->ImpersonationLevel;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            ObDereferenceObject( Token );
            return GetExceptionCode();
        }


        ObDereferenceObject( Token );
        return STATUS_SUCCESS;


    case TokenStatistics:

        LocalStatistics = (PTOKEN_STATISTICS)TokenInformation;

        Status = ObReferenceObjectByHandle(
                 TokenHandle,           // Handle
                 TOKEN_QUERY,           // DesiredAccess
                 SepTokenObjectType,    // ObjectType
                 PreviousMode,          // AccessMode
                 (PVOID *)&Token,       // Object
                 NULL                   // GrantedAccess
                 );

        if ( !NT_SUCCESS(Status) ) {
            return Status;
        }

        //
        //  Gain exclusive access to the token.
        //

        SepAcquireTokenReadLock( Token );



        //
        // Return the length required now in case not enough buffer
        // was provided by the caller and we have to return an error.
        //

        RequiredLength = (ULONG)sizeof( TOKEN_STATISTICS );

        try {

            *ReturnLength = RequiredLength;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }

        if ( TokenInformationLength < RequiredLength ) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return STATUS_BUFFER_TOO_SMALL;

        }

        //
        // Return the statistics
        //

        try {

            LocalStatistics->TokenId            = Token->TokenId;
            LocalStatistics->AuthenticationId   = Token->AuthenticationId;
            LocalStatistics->ExpirationTime     = Token->ExpirationTime;
            LocalStatistics->TokenType          = Token->TokenType;
            LocalStatistics->ImpersonationLevel = Token->ImpersonationLevel;
            LocalStatistics->DynamicCharged     = Token->DynamicCharged;
            LocalStatistics->DynamicAvailable   = Token->DynamicAvailable;
            LocalStatistics->GroupCount         = Token->UserAndGroupCount-1;
            LocalStatistics->PrivilegeCount     = Token->PrivilegeCount;
            LocalStatistics->ModifiedId         = Token->ModifiedId;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }


        SepReleaseTokenReadLock( Token );
        ObDereferenceObject( Token );
        return STATUS_SUCCESS;

    case TokenSessionId:

        if ( TokenInformationLength != sizeof(ULONG) )
            return( STATUS_INFO_LENGTH_MISMATCH );

        Status = ObReferenceObjectByHandle(
                 TokenHandle,           // Handle
                 TOKEN_QUERY,           // DesiredAccess
                 SepTokenObjectType,    // ObjectType
                 PreviousMode,          // AccessMode
                 (PVOID *)&Token,       // Object
                 NULL                   // GrantedAccess
                 );

        if ( !NT_SUCCESS(Status) ) {
            return Status;
        }

        //
        // Get SessionId for the token
        //
        SeQuerySessionIdToken( (PACCESS_TOKEN)Token,
                               &SessionId);

        try {

            *(PULONG)TokenInformation = SessionId;
            *ReturnLength = sizeof(ULONG);

        } except(EXCEPTION_EXECUTE_HANDLER) {
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }

        ObDereferenceObject( Token );
        return( STATUS_SUCCESS );

    default:

        return STATUS_INVALID_INFO_CLASS;
    }
}


NTSTATUS
SeQueryAuthenticationIdToken(
    IN PACCESS_TOKEN Token,
    OUT PLUID AuthenticationId
    )

/*++


Routine Description:

    Retrieve authentication ID out of the token.

Arguments:

    Token - Referenced pointer to a token.

    AutenticationId - Receives the token's authentication ID.

Return Value:

    STATUS_SUCCESS - Indicates the operation was successful.

    This is the only expected status.

--*/
{
    PAGED_CODE();

    SepAcquireTokenReadLock( ((PTOKEN)Token) );
    (*AuthenticationId) = ((PTOKEN)Token)->AuthenticationId;
    SepReleaseTokenReadLock( ((PTOKEN)Token) );
    return(STATUS_SUCCESS);
}



NTSTATUS
SeQueryInformationToken (
    IN PACCESS_TOKEN AccessToken,
    IN TOKEN_INFORMATION_CLASS TokenInformationClass,
    OUT PVOID *TokenInformation
    )

/*++


Routine Description:

    Retrieve information about a specified token.

Arguments:

    TokenHandle - Provides a handle to the token to operate on.

    TokenInformationClass - The token information class about which
        to retrieve information.

    TokenInformation - Receives a pointer to the requested information.
        The actual structures returned are dependent upon the information
        class requested, as defined in the TokenInformationClass parameter
        description.

        TokenInformation Format By Information Class:

           TokenUser => TOKEN_USER data structure.  TOKEN_QUERY
           access is needed to retrieve this information about a
           token.

           TokenGroups => TOKEN_GROUPS data structure.  TOKEN_QUERY
           access is needed to retrieve this information about a
           token.

           TokenPrivileges => TOKEN_PRIVILEGES data structure.
           TOKEN_QUERY access is needed to retrieve this information
           about a token.

           TokenOwner => TOKEN_OWNER data structure.  TOKEN_QUERY
           access is needed to retrieve this information about a
           token.

           TokenPrimaryGroup => TOKEN_PRIMARY_GROUP data structure.
           TOKEN_QUERY access is needed to retrieve this information
           about a token.

           TokenDefaultDacl => TOKEN_DEFAULT_DACL data structure.
           TOKEN_QUERY access is needed to retrieve this information
           about a token.

           TokenSource => TOKEN_SOURCE data structure.
           TOKEN_QUERY_SOURCE access is needed to retrieve this
           information about a token.

           TokenType => TOKEN_TYPE data structure.
           TOKEN_QUERY access is needed to retrieve this information
           about a token.

           TokenStatistics => TOKEN_STATISTICS data structure.
           TOKEN_QUERY access is needed to retrieve this
           information about a token.

Return Value:

    STATUS_SUCCESS - Indicates the operation was successful.

--*/
{

    NTSTATUS Status;

    ULONG RequiredLength;
    ULONG Index;

    PSID PSid;
    PACL PAcl;

    PVOID Ignore;
    PTOKEN Token = (PTOKEN)AccessToken;

    PAGED_CODE();

    //
    // Case on information class.
    //

    switch ( TokenInformationClass ) {

        case TokenUser:
            {
                PTOKEN_USER LocalUser;

                //
                //  Gain exclusive access to the token.
                //

                SepAcquireTokenReadLock( Token );

                //
                // Return the length required now in case not enough buffer
                // was provided by the caller and we have to return an error.
                //

                RequiredLength = SeLengthSid( Token->UserAndGroups[0].Sid) +
                                 (ULONG)sizeof( TOKEN_USER );

                LocalUser = ExAllocatePool( PagedPool, RequiredLength );

                if (LocalUser == NULL) {
                    SepReleaseTokenReadLock( Token );
                    return( STATUS_INSUFFICIENT_RESOURCES );
                }

                //
                // Return the user SID
                //
                //  Put SID immediately following TOKEN_USER data structure
                //

                PSid = (PSID)( (ULONG_PTR)LocalUser + (ULONG)sizeof(TOKEN_USER) );

                RtlCopySidAndAttributesArray(
                    1,
                    Token->UserAndGroups,
                    RequiredLength,
                    &(LocalUser->User),
                    PSid,
                    ((PSID *)&Ignore),
                    ((PULONG)&Ignore)
                    );

                SepReleaseTokenReadLock( Token );
                *TokenInformation = LocalUser;
                return STATUS_SUCCESS;
            }


        case TokenGroups:
            {
                PTOKEN_GROUPS LocalGroups;

                //
                //  Gain exclusive access to the token.
                //

                SepAcquireTokenReadLock( Token );

                //
                // Figure out how much space is needed to return the group SIDs.
                // That's the size of TOKEN_GROUPS (without any array entries)
                // plus the size of an SID_AND_ATTRIBUTES times the number of groups.
                // The number of groups is Token->UserAndGroups-1 (since the count
                // includes the user ID).  Then the lengths of each individual group
                // must be added.
                //

                RequiredLength = (ULONG)sizeof(TOKEN_GROUPS) +
                                 ((Token->UserAndGroupCount - ANYSIZE_ARRAY - 1) *
                                 ((ULONG)sizeof(SID_AND_ATTRIBUTES)) );

                Index = 1;
                while (Index < Token->UserAndGroupCount) {

                    RequiredLength += SeLengthSid( Token->UserAndGroups[Index].Sid );

                    Index += 1;

                } // endwhile

                LocalGroups = ExAllocatePool( PagedPool, RequiredLength );

                if (LocalGroups == NULL) {
                    SepReleaseTokenReadLock( Token );
                    return( STATUS_INSUFFICIENT_RESOURCES );
                }

                //
                // Now copy the groups.
                //

                LocalGroups->GroupCount = Token->UserAndGroupCount - 1;

                PSid = (PSID)( (ULONG_PTR)LocalGroups +
                               (ULONG)sizeof(TOKEN_GROUPS) +
                               (   (Token->UserAndGroupCount - ANYSIZE_ARRAY - 1) *
                                   (ULONG)sizeof(SID_AND_ATTRIBUTES) )
                             );

                RtlCopySidAndAttributesArray(
                    (ULONG)(Token->UserAndGroupCount - 1),
                    &(Token->UserAndGroups[1]),
                    RequiredLength,
                    LocalGroups->Groups,
                    PSid,
                    ((PSID *)&Ignore),
                    ((PULONG)&Ignore)
                    );

                SepReleaseTokenReadLock( Token );
                *TokenInformation = LocalGroups;
                return STATUS_SUCCESS;
            }


        case TokenPrivileges:
            {
                PTOKEN_PRIVILEGES LocalPrivileges;

                //
                //  Gain exclusive access to the token to prevent changes
                //  from occuring to the privileges.
                //

                SepAcquireTokenReadLock( Token );

                //
                // Return the length required now in case not enough buffer
                // was provided by the caller and we have to return an error.
                //

                RequiredLength = (ULONG)sizeof(TOKEN_PRIVILEGES) +
                                 ((Token->PrivilegeCount - ANYSIZE_ARRAY) *
                                 ((ULONG)sizeof(LUID_AND_ATTRIBUTES)) );

                LocalPrivileges = ExAllocatePool( PagedPool, RequiredLength );

                if (LocalPrivileges == NULL) {
                    SepReleaseTokenReadLock( Token );
                    return( STATUS_INSUFFICIENT_RESOURCES );
                }

                //
                // Return the token privileges.
                //

                LocalPrivileges->PrivilegeCount = Token->PrivilegeCount;

                RtlCopyLuidAndAttributesArray(
                    Token->PrivilegeCount,
                    Token->Privileges,
                    LocalPrivileges->Privileges
                    );

                SepReleaseTokenReadLock( Token );
                *TokenInformation = LocalPrivileges;
                return STATUS_SUCCESS;
            }


        case TokenOwner:
            {
                PTOKEN_OWNER LocalOwner;

                //
                //  Gain exclusive access to the token to prevent changes
                //  from occuring to the owner.
                //

                SepAcquireTokenReadLock( Token );

                //
                // Return the length required now in case not enough buffer
                // was provided by the caller and we have to return an error.
                //

                PSid = Token->UserAndGroups[Token->DefaultOwnerIndex].Sid;
                RequiredLength = (ULONG)sizeof(TOKEN_OWNER) +
                                 SeLengthSid( PSid );

                LocalOwner = ExAllocatePool( PagedPool, RequiredLength );

                if (LocalOwner == NULL) {
                    SepReleaseTokenReadLock( Token );
                    return( STATUS_INSUFFICIENT_RESOURCES );
                }

                //
                // Return the owner SID
                //

                PSid = (PSID)((ULONG_PTR)LocalOwner +
                              (ULONG)sizeof(TOKEN_OWNER));

                LocalOwner->Owner = PSid;

                Status = RtlCopySid(
                             (RequiredLength - (ULONG)sizeof(TOKEN_OWNER)),
                             PSid,
                             Token->UserAndGroups[Token->DefaultOwnerIndex].Sid
                             );

                ASSERT( NT_SUCCESS(Status) );

                SepReleaseTokenReadLock( Token );
                *TokenInformation = LocalOwner;
                return STATUS_SUCCESS;
            }


        case TokenPrimaryGroup:
            {
                PTOKEN_PRIMARY_GROUP LocalPrimaryGroup;

                //
                //  Gain exclusive access to the token to prevent changes
                //  from occuring to the owner.
                //

                SepAcquireTokenReadLock( Token );

                //
                // Return the length required now in case not enough buffer
                // was provided by the caller and we have to return an error.
                //

                RequiredLength = (ULONG)sizeof(TOKEN_PRIMARY_GROUP) +
                                 SeLengthSid( Token->PrimaryGroup );

                LocalPrimaryGroup = ExAllocatePool( PagedPool, RequiredLength );

                if (LocalPrimaryGroup == NULL) {
                    SepReleaseTokenReadLock( Token );
                    return( STATUS_INSUFFICIENT_RESOURCES );
                }

                //
                // Return the primary group SID
                //

                PSid = (PSID)((ULONG_PTR)LocalPrimaryGroup +
                              (ULONG)sizeof(TOKEN_PRIMARY_GROUP));

                LocalPrimaryGroup->PrimaryGroup = PSid;

                Status = RtlCopySid( (RequiredLength - (ULONG)sizeof(TOKEN_PRIMARY_GROUP)),
                                     PSid,
                                     Token->PrimaryGroup
                                     );

                ASSERT( NT_SUCCESS(Status) );

                SepReleaseTokenReadLock( Token );
                *TokenInformation = LocalPrimaryGroup;
                return STATUS_SUCCESS;
            }


        case TokenDefaultDacl:
            {
                PTOKEN_DEFAULT_DACL LocalDefaultDacl;

                //
                //  Gain exclusive access to the token to prevent changes
                //  from occuring to the owner.
                //

                SepAcquireTokenReadLock( Token );

                //
                // Return the length required now in case not enough buffer
                // was provided by the caller and we have to return an error.
                //

                RequiredLength = (ULONG)sizeof(TOKEN_DEFAULT_DACL);

                if (ARGUMENT_PRESENT(Token->DefaultDacl)) {

                    RequiredLength += Token->DefaultDacl->AclSize;
                }

                LocalDefaultDacl = ExAllocatePool( PagedPool, RequiredLength );

                if (LocalDefaultDacl == NULL) {
                    SepReleaseTokenReadLock( Token );
                    return( STATUS_INSUFFICIENT_RESOURCES );
                }

                //
                // Return the default Dacl
                //

                PAcl = (PACL)((ULONG_PTR)LocalDefaultDacl +
                              (ULONG)sizeof(TOKEN_DEFAULT_DACL));

                if (ARGUMENT_PRESENT(Token->DefaultDacl)) {

                    LocalDefaultDacl->DefaultDacl = PAcl;

                    RtlCopyMemory( (PVOID)PAcl,
                                   (PVOID)Token->DefaultDacl,
                                   Token->DefaultDacl->AclSize
                                   );
                } else {

                    LocalDefaultDacl->DefaultDacl = NULL;
                }

                SepReleaseTokenReadLock( Token );
                *TokenInformation = LocalDefaultDacl;
                return STATUS_SUCCESS;
            }


        case TokenSource:
            {
                PTOKEN_SOURCE LocalSource;

                //
                // The type of a token can not be changed, so
                // exclusive access to the token is not necessary.
                //

                //
                // Return the length required now in case not enough buffer
                // was provided by the caller and we have to return an error.
                //

                RequiredLength = (ULONG) sizeof(TOKEN_SOURCE);

                LocalSource = ExAllocatePool( PagedPool, RequiredLength );

                if (LocalSource == NULL) {
                    return( STATUS_INSUFFICIENT_RESOURCES );
                }

                //
                // Return the token source
                //

                (*LocalSource) = Token->TokenSource;
                *TokenInformation = LocalSource;

                return STATUS_SUCCESS;
            }


        case TokenType:
            {
                PTOKEN_TYPE LocalType;

                //
                // The type of a token can not be changed, so
                // exclusive access to the token is not necessary.
                //

                //
                // Return the length required now in case not enough buffer
                // was provided by the caller and we have to return an error.
                //

                RequiredLength = (ULONG) sizeof(TOKEN_TYPE);

                LocalType = ExAllocatePool( PagedPool, RequiredLength );

                if (LocalType == NULL) {
                    return( STATUS_INSUFFICIENT_RESOURCES );
                }

                //
                // Return the token type
                //

                (*LocalType) = Token->TokenType;
                *TokenInformation = LocalType;
                return STATUS_SUCCESS;
            }


        case TokenImpersonationLevel:
            {
                PSECURITY_IMPERSONATION_LEVEL LocalImpersonationLevel;

                //
                // The impersonation level of a token can not be changed, so
                // exclusive access to the token is not necessary.
                //

                //
                //  Make sure the token is an appropriate type to be retrieving
                //  the impersonation level from.
                //

                if (Token->TokenType != TokenImpersonation) {

                    return STATUS_INVALID_INFO_CLASS;
                }

                //
                // Return the length required now in case not enough buffer
                // was provided by the caller and we have to return an error.
                //

                RequiredLength = (ULONG) sizeof(SECURITY_IMPERSONATION_LEVEL);

                LocalImpersonationLevel = ExAllocatePool( PagedPool, RequiredLength );

                if (LocalImpersonationLevel == NULL) {
                    return( STATUS_INSUFFICIENT_RESOURCES );
                }

                //
                // Return the impersonation level
                //

                (*LocalImpersonationLevel) = Token->ImpersonationLevel;
                *TokenInformation = LocalImpersonationLevel;
                return STATUS_SUCCESS;
            }


        case TokenStatistics:
            {
                PTOKEN_STATISTICS LocalStatistics;

                //
                //  Gain exclusive access to the token.
                //

                SepAcquireTokenReadLock( Token );

                //
                // Return the length required now in case not enough buffer
                // was provided by the caller and we have to return an error.
                //

                RequiredLength = (ULONG)sizeof( TOKEN_STATISTICS );

                LocalStatistics = ExAllocatePool( PagedPool, RequiredLength );

                if (LocalStatistics == NULL) {
                    SepReleaseTokenReadLock( Token );
                    return( STATUS_INSUFFICIENT_RESOURCES );
                }

                //
                // Return the statistics
                //

                LocalStatistics->TokenId            = Token->TokenId;
                LocalStatistics->AuthenticationId   = Token->AuthenticationId;
                LocalStatistics->ExpirationTime     = Token->ExpirationTime;
                LocalStatistics->TokenType          = Token->TokenType;
                LocalStatistics->ImpersonationLevel = Token->ImpersonationLevel;
                LocalStatistics->DynamicCharged     = Token->DynamicCharged;
                LocalStatistics->DynamicAvailable   = Token->DynamicAvailable;
                LocalStatistics->GroupCount         = Token->UserAndGroupCount-1;
                LocalStatistics->PrivilegeCount     = Token->PrivilegeCount;
                LocalStatistics->ModifiedId         = Token->ModifiedId;

                SepReleaseTokenReadLock( Token );
                *TokenInformation = LocalStatistics;
                return STATUS_SUCCESS;
            }

    case TokenSessionId:

        /*
         * Get SessionId for the token
         */
        SeQuerySessionIdToken( (PACCESS_TOKEN)Token,
                             (PULONG)TokenInformation );

        return( STATUS_SUCCESS );

    default:

        return STATUS_INVALID_INFO_CLASS;
    }
}



NTSTATUS
SeQuerySessionIdToken(
    PACCESS_TOKEN Token,
    PULONG SessionId
    )

/*++


Routine Description:

    Gets the SessionId from the specified token object.

Arguments:

    Token (input)
      Opaque kernel ACCESS_TOKEN pointer
    SessionId (output)
      pointer to location to return SessionId

Return Value:

    STATUS_SUCCESS - no error

--*/
{

    /*
     * Get the SessionId.
     */
    SepAcquireTokenReadLock( ((PTOKEN)Token) );
    (*SessionId) = ((PTOKEN)Token)->SessionId;
    SepReleaseTokenReadLock( ((PTOKEN)Token) );
    return( STATUS_SUCCESS );
}


