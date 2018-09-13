/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    seurtl.c

Abstract:

    This Module implements many security rtl routines defined in nturtl.h

Author:

    Robert Reichel  (RobertRe)  1-Mar-1991

Environment:

    Pure Runtime Library Routine
    User mode callable only

Revision History:

--*/


#include <ntos.h>
#include <nturtl.h>
#include <ntlsa.h>      // needed for RtlGetPrimaryDomain
#include "seopaque.h"
#include "sertlp.h"
#include "ldrp.h"





///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//    Exported Procedures                                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


#if WHEN_LSAUDLL_MOVED_TO_NTDLL
NTSTATUS
RtlGetPrimaryDomain(
    IN  ULONG            SidLength,
    OUT PBOOLEAN         PrimaryDomainPresent,
    OUT PUNICODE_STRING  PrimaryDomainName,
    OUT PUSHORT          RequiredNameLength,
    OUT PSID             PrimaryDomainSid OPTIONAL,
    OUT PULONG           RequiredSidLength
    )

/*++

Routine Description:

    This procedure opens the LSA policy object and retrieves
    the primary domain information for this machine.

Arguments:

    SidLength - Specifies the length of the PrimaryDomainSid
        parameter.

    PrimaryDomainPresent - Receives a boolean value indicating
        whether this machine has a primary domain or not. TRUE
        indicates the machine does have a primary domain. FALSE
        indicates the machine does not.

    PrimaryDomainName - Points to the unicode string to receive
        the primary domain name.  This parameter will only be
        used if there is a primary domain.

    RequiredNameLength - Recevies the length of the primary
        domain name (in bytes).  This parameter will only be
        used if there is a primary domain.

    PrimaryDomainSid - This optional parameter, if present,
        points to a buffer to receive the primary domain's
        SID.  This parameter will only be used if there is a
        primary domain.

    RequiredSidLength - Recevies the length of the primary
        domain SID (in bytes).  This parameter will only be
        used if there is a primary domain.


Return Value:

    STATUS_SUCCESS - The requested information has been retrieved.

    STATUS_BUFFER_TOO_SMALL - One of the return buffers was not
        large enough to receive the corresponding information.
        The RequiredNameLength and RequiredSidLength parameter
        values have been set to indicate the needed length.

    Other status values as may be returned by:

        LsaOpenPolicy()
        LsaQueryInformationPolicy()
        RtlCopySid()


--*/




{
    NTSTATUS Status, IgnoreStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE LsaHandle;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomainInfo;


    //
    // Set up the Security Quality Of Service
    //

    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
    SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQualityOfService.EffectiveOnly = FALSE;

    //
    // Set up the object attributes to open the Lsa policy object
    //

    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               0L,
                               (HANDLE)NULL,
                               NULL);
    ObjectAttributes.SecurityQualityOfService = &SecurityQualityOfService;

    //
    // Open the local LSA policy object
    //

    Status = LsaOpenPolicy( NULL,
                            &ObjectAttributes,
                            POLICY_VIEW_LOCAL_INFORMATION,
                            &LsaHandle
                          );
    if (NT_SUCCESS(Status)) {

        //
        // Get the primary domain info
        //
        Status = LsaQueryInformationPolicy(LsaHandle,
                                           PolicyPrimaryDomainInformation,
                                           (PVOID *)&PrimaryDomainInfo);
        IgnoreStatus = LsaClose(LsaHandle);
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }

    if (NT_SUCCESS(Status)) {

        //
        // Is there a primary domain?
        //

        if (PrimaryDomainInfo->Sid != NULL) {

            //
            // Yes
            //

            (*PrimaryDomainPresent) = TRUE;
            (*RequiredNameLength) = PrimaryDomainInfo->Name.Length;
            (*RequiredSidLength)  = RtlLengthSid(PrimaryDomainInfo->Sid);



            //
            // Copy the name
            //

            if (PrimaryDomainName->MaximumLength >=
                PrimaryDomainInfo->Name.Length) {
                RtlCopyUnicodeString(
                    PrimaryDomainName,
                    &PrimaryDomainInfo->Name
                    );
            } else {
                Status = STATUS_BUFFER_TOO_SMALL;
            }


            //
            // Copy the SID (if appropriate)
            //

            if (PrimaryDomainSid != NULL && NT_SUCCESS(Status)) {

                Status = RtlCopySid(SidLength,
                                    PrimaryDomainSid,
                                    PrimaryDomainInfo->Sid
                                    );
            }
        } else {

            (*PrimaryDomainPresent) = FALSE;
        }

        //
        // We're finished with the buffer returned by LSA
        //

        IgnoreStatus = LsaFreeMemory(PrimaryDomainInfo);
        ASSERT(NT_SUCCESS(IgnoreStatus));

    }


    return(Status);
}
#endif //WHEN_LSAUDLL_MOVED_TO_NTDLL


NTSTATUS
RtlNewSecurityObjectEx (
    IN PSECURITY_DESCRIPTOR ParentDescriptor OPTIONAL,
    IN PSECURITY_DESCRIPTOR CreatorDescriptor OPTIONAL,
    OUT PSECURITY_DESCRIPTOR * NewDescriptor,
    IN GUID *ObjectType OPTIONAL,
    IN BOOLEAN IsDirectoryObject,
    IN ULONG AutoInheritFlags,
    IN HANDLE Token OPTIONAL,
    IN PGENERIC_MAPPING GenericMapping
    )
/*++

Routine Description:

    See RtlpNewSecurityObject.

                              - - WARNING - -

    This service is for use by protected subsystems that project their own
    type of object.  This service is explicitly not for use by the
    executive for executive objects and must not be called from kernel
    mode.

Arguments:

    See RtlpNewSecurityObject.

Return Value:

    See RtlpNewSecurityObject.

--*/
{

    //
    // Simple call the newer RtlpNewSecurityObject
    //

    return RtlpNewSecurityObject (
                ParentDescriptor,
                CreatorDescriptor,
                NewDescriptor,
                ObjectType,
                IsDirectoryObject,
                AutoInheritFlags,
                Token,
                GenericMapping );

}




NTSTATUS
RtlNewSecurityObject (
    IN PSECURITY_DESCRIPTOR ParentDescriptor OPTIONAL,
    IN PSECURITY_DESCRIPTOR CreatorDescriptor OPTIONAL,
    OUT PSECURITY_DESCRIPTOR * NewDescriptor,
    IN BOOLEAN IsDirectoryObject,
    IN HANDLE Token,
    IN PGENERIC_MAPPING GenericMapping
    )
/*++

Routine Description:

    See RtlpNewSecurityObject.

                              - - WARNING - -

    This service is for use by protected subsystems that project their own
    type of object.  This service is explicitly not for use by the
    executive for executive objects and must not be called from kernel
    mode.

Arguments:

    See RtlpNewSecurityObject.

Return Value:

    See RtlpNewSecurityObject.

--*/
{

    //
    // Simple call the newer RtlpNewSecurityObject
    //

    return RtlpNewSecurityObject (
                ParentDescriptor,
                CreatorDescriptor,
                NewDescriptor,
                NULL,   // No ObjectType
                IsDirectoryObject,
                0,      // No Automatic inheritance
                Token,
                GenericMapping );

}



NTSTATUS
RtlSetSecurityObject (
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR ModificationDescriptor,
    IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    IN PGENERIC_MAPPING GenericMapping,
    IN HANDLE Token OPTIONAL
    )
/*++

Routine Description:

    See RtlpSetSecurityObject.

Arguments:

    See RtlpSetSecurityObject.

Return Value:

    See RtlpSetSecurityObject.

--*/

{

    //
    // Simply call RtlpSetSecurityObject specifying no auto inheritance.
    //

    return RtlpSetSecurityObject( NULL,
                                  SecurityInformation,
                                  ModificationDescriptor,
                                  ObjectsSecurityDescriptor,
                                  0,   // No AutoInheritance
                                  PagedPool,
                                  GenericMapping,
                                  Token );
}



NTSTATUS
RtlSetSecurityObjectEx (
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR ModificationDescriptor,
    IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    IN ULONG AutoInheritFlags,
    IN PGENERIC_MAPPING GenericMapping,
    IN HANDLE Token OPTIONAL
    )
/*++

Routine Description:

    See RtlpSetSecurityObject.

Arguments:

    See RtlpSetSecurityObject.

Return Value:

    See RtlpSetSecurityObject.

--*/

{

    //
    // Simply call RtlpSetSecurityObject specifying no auto inheritance.
    //

    return RtlpSetSecurityObject( NULL,
                                  SecurityInformation,
                                  ModificationDescriptor,
                                  ObjectsSecurityDescriptor,
                                  AutoInheritFlags,
                                  PagedPool,
                                  GenericMapping,
                                  Token );
}





NTSTATUS
RtlQuerySecurityObject (
    IN PSECURITY_DESCRIPTOR ObjectDescriptor,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR ResultantDescriptor,
    IN ULONG DescriptorLength,
    OUT PULONG ReturnLength
    )

/*++

Routine Description:

    Query information from a protected server object's existing security
    descriptor.

    This procedure, called only from user mode, is used to retrieve
    information from a security descriptor on an existing protected
    server's object.  All access checking is expected to be done before
    calling this routine.  This includes checking for READ_CONTROL, and
    privilege to read a system ACL as appropriate.

                          - - WARNING - -

    This service is for use by protected subsystems that project their own
    type of object.  This service is explicitly not for use by the
    executive for executive objects and must not be called from kernel
    mode.


Arguments:

    ObjectDescriptor - Points to a pointer to a security descriptor to be
        queried.

    SecurityInformation - Identifies the security information being
        requested.

    ResultantDescriptor - Points to buffer to receive the resultant
        security descriptor.  The resultant security descriptor will
        contain all information requested by the SecurityInformation
        parameter.

    DescriptorLength - Is an unsigned integer which indicates the length,
        in bytes, of the buffer provided to receive the resultant
        descriptor.

    ReturnLength - Receives an unsigned integer indicating the actual
        number of bytes needed in the ResultantDescriptor to store the
        requested information.  If the value returned is greater than the
        value passed via the DescriptorLength parameter, then
        STATUS_BUFFER_TOO_SMALL is returned and no information is returned.


Return Value:

    STATUS_SUCCESS - The operation was successful.

    STATUS_BUFFER_TOO_SMALL - The buffer provided to receive the requested
        information was not large enough to hold the information.  No
        information has been returned.

    STATUS_BAD_DESCRIPTOR_FORMAT - Indicates the provided object's security
        descriptor was not in self-relative format.

--*/

{

    PSID Group;
    PSID Owner;
    PACL Dacl;
    PACL Sacl;

    ULONG GroupSize = 0;
    ULONG DaclSize = 0;
    ULONG SaclSize = 0;
    ULONG OwnerSize = 0;

    PCHAR Field;
    PCHAR Base;


    PISECURITY_DESCRIPTOR IObjectDescriptor;
    PISECURITY_DESCRIPTOR_RELATIVE IResultantDescriptor;


    IResultantDescriptor = (PISECURITY_DESCRIPTOR_RELATIVE)ResultantDescriptor;
    IObjectDescriptor = (PISECURITY_DESCRIPTOR)ObjectDescriptor;

    //
    // For each item specified in the SecurityInformation, extract it
    // and get it to the point where it can be copied into a new
    // descriptor.
    //

    if (SecurityInformation & GROUP_SECURITY_INFORMATION) {

        Group = RtlpGroupAddrSecurityDescriptor(IObjectDescriptor);
        GroupSize = LongAlignSize(SeLengthSid(Group));
    }

    if (SecurityInformation & DACL_SECURITY_INFORMATION) {

        Dacl = RtlpDaclAddrSecurityDescriptor( IObjectDescriptor );

        if (Dacl != NULL) {
            DaclSize = LongAlignSize(Dacl->AclSize);
        }
    }

    if (SecurityInformation & SACL_SECURITY_INFORMATION) {

        Sacl = RtlpSaclAddrSecurityDescriptor( IObjectDescriptor );

        if (Sacl != NULL) {
            SaclSize = LongAlignSize(Sacl->AclSize);
        }

    }

    if (SecurityInformation & OWNER_SECURITY_INFORMATION) {

        Owner = RtlpOwnerAddrSecurityDescriptor ( IObjectDescriptor );
        OwnerSize = LongAlignSize(SeLengthSid(Owner));
    }

    *ReturnLength = sizeof( SECURITY_DESCRIPTOR_RELATIVE ) +
                    GroupSize +
                    DaclSize  +
                    SaclSize  +
                    OwnerSize;

    if (*ReturnLength > DescriptorLength) {
        return( STATUS_BUFFER_TOO_SMALL );
    }

    RtlCreateSecurityDescriptor(
        IResultantDescriptor,
        SECURITY_DESCRIPTOR_REVISION
        );

    RtlpSetControlBits( IResultantDescriptor, SE_SELF_RELATIVE );

    Base = (PCHAR)(IResultantDescriptor);
    Field =  Base + (ULONG)sizeof(SECURITY_DESCRIPTOR_RELATIVE);

    if (SecurityInformation & SACL_SECURITY_INFORMATION) {

        if (SaclSize > 0) {
            RtlMoveMemory( Field, Sacl, SaclSize );
            IResultantDescriptor->Sacl = RtlPointerToOffset(Base,Field);
            Field += SaclSize;
        }

        RtlpPropagateControlBits(
            IResultantDescriptor,
            IObjectDescriptor,
            SE_SACL_PRESENT | SE_SACL_DEFAULTED
            );
    }

    if (SecurityInformation & DACL_SECURITY_INFORMATION) {

        if (DaclSize > 0) {
            RtlMoveMemory( Field, Dacl, DaclSize );
            IResultantDescriptor->Dacl = RtlPointerToOffset(Base,Field);
            Field += DaclSize;
        }

        RtlpPropagateControlBits(
            IResultantDescriptor,
            IObjectDescriptor,
            SE_DACL_PRESENT | SE_DACL_DEFAULTED
            );
    }

    if (SecurityInformation & OWNER_SECURITY_INFORMATION) {

        if (OwnerSize > 0) {
            RtlMoveMemory( Field, Owner, OwnerSize );
            IResultantDescriptor->Owner = RtlPointerToOffset(Base,Field);
            Field += OwnerSize;
        }

        RtlpPropagateControlBits(
            IResultantDescriptor,
            IObjectDescriptor,
            SE_OWNER_DEFAULTED
            );

    }

    if (SecurityInformation & GROUP_SECURITY_INFORMATION) {

        if (GroupSize > 0) {
            RtlMoveMemory( Field, Group, GroupSize );
            IResultantDescriptor->Group = RtlPointerToOffset(Base,Field);
        }

        RtlpPropagateControlBits(
            IResultantDescriptor,
            IObjectDescriptor,
            SE_GROUP_DEFAULTED
            );
    }

    return( STATUS_SUCCESS );

}





NTSTATUS
RtlDeleteSecurityObject (
    IN OUT PSECURITY_DESCRIPTOR * ObjectDescriptor
    )


/*++

Routine Description:

    Delete a protected server object's security descriptor.

    This procedure, called only from user mode, is used to delete a
    security descriptor associated with a protected server's object.  This
    routine will normally be called by a protected server during object
    deletion.

                                  - - WARNING - -

    This service is for use by protected subsystems that project their own
    type of object.  This service is explicitly not for use by the
    executive for executive objects and must not be called from kernel
    mode.


Arguments:

    ObjectDescriptor - Points to a pointer to a security descriptor to be
        deleted.


Return Value:

    STATUS_SUCCESS - The operation was successful.

--*/

{
    RtlFreeHeap( RtlProcessHeap(), 0, (PVOID)*ObjectDescriptor );

    return( STATUS_SUCCESS );

}




NTSTATUS
RtlNewInstanceSecurityObject(
    IN BOOLEAN ParentDescriptorChanged,
    IN BOOLEAN CreatorDescriptorChanged,
    IN PLUID OldClientTokenModifiedId,
    OUT PLUID NewClientTokenModifiedId,
    IN PSECURITY_DESCRIPTOR ParentDescriptor OPTIONAL,
    IN PSECURITY_DESCRIPTOR CreatorDescriptor OPTIONAL,
    OUT PSECURITY_DESCRIPTOR * NewDescriptor,
    IN BOOLEAN IsDirectoryObject,
    IN HANDLE Token,
    IN PGENERIC_MAPPING GenericMapping
    )

/*++

Routine Description:

    If the return status is STATUS_SUCCESS and the NewSecurity return
    value is NULL, then the security desscriptor of the original
    instance of the object is valid for this instance as well.

Arguments:

    ParentDescriptorChanged - Supplies a flag indicating whether the
        parent security descriptor has changed since the last time
        this set of parameters was used.

    CreatorDescriptorChanged - Supplies a flag indicating whether the
        creator security descriptor has changed since the last time
        this set of parameters was used.

    OldClientTokenModifiedId - Supplies the ModifiedId from the passed
        token that was in effect when this call was last made with
        these parameters.  If the current ModifiedId is different from
        the one passed in here, the security descriptor must be
        rebuilt.

    NewClientTokenModifiedId - Returns the current ModifiedId from the
        passed token.

    ParentDescriptor - Supplies the Security Descriptor for the parent
        directory under which a new object is being created.  If there is
        no parent directory, then this argument is specified as NULL.

    CreatorDescriptor - (Optionally) Points to a security descriptor
        presented by the creator of the object.  If the creator of the
        object did not explicitly pass security information for the new
        object, then a null pointer should be passed.

    NewDescriptor - Points to a pointer that is to be made to point to the
        newly allocated self-relative security descriptor.

    IsDirectoryObject - Specifies if the new object is going to be a
        directory object.  A value of TRUE indicates the object is a
        container of other objects.

    Token - Supplies the token for the client on whose behalf the
        object is being created.  If it is an impersonation token,
        then it must be at SecurityIdentification level or higher.  If
        it is not an impersonation token, the operation proceeds
        normally.

        A client token is used to retrieve default security
        information for the new object, such as default owner, primary
        group, and discretionary access control.  The token must be
        open for TOKEN_QUERY access.

    GenericMapping - Supplies a pointer to a generic mapping array denoting
        the mapping between each generic right to specific rights.

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{

    TOKEN_STATISTICS ClientTokenStatistics;
    ULONG ReturnLength;
    NTSTATUS Status;



    //
    // Get the current token modified LUID
    //


    Status = NtQueryInformationToken(
                 Token,                        // Handle
                 TokenStatistics,              // TokenInformationClass
                 &ClientTokenStatistics,       // TokenInformation
                 sizeof(TOKEN_STATISTICS),     // TokenInformationLength
                 &ReturnLength                 // ReturnLength
                 );

    if ( !NT_SUCCESS( Status )) {
        return( Status );
    }

    *NewClientTokenModifiedId = ClientTokenStatistics.ModifiedId;

    if ( RtlEqualLuid(NewClientTokenModifiedId, OldClientTokenModifiedId) ) {

        if ( !(ParentDescriptorChanged || CreatorDescriptorChanged) ) {

            //
            // The old security descriptor is valid for this new instance
            // of the object type as well.  Pass back success and NULL for
            // the NewDescriptor.
            //

            *NewDescriptor = NULL;
            return( STATUS_SUCCESS );

        }
    }

    //
    // Something has changed, take the long route and build a new
    // descriptor
    //

    return( RtlNewSecurityObject( ParentDescriptor,
                                  CreatorDescriptor,
                                  NewDescriptor,
                                  IsDirectoryObject,
                                  Token,
                                  GenericMapping
                                  ));
}




NTSTATUS
RtlNewSecurityGrantedAccess(
    IN ACCESS_MASK DesiredAccess,
    OUT PPRIVILEGE_SET Privileges,
    IN OUT PULONG Length,
    IN HANDLE Token OPTIONAL,
    IN PGENERIC_MAPPING GenericMapping,
    OUT PACCESS_MASK RemainingDesiredAccess
    )

/*++

Routine Description:

    This routine implements privilege policy by examining the bits in
    a DesiredAccess mask and adjusting them based on privilege checks.

    Currently, a request for ACCESS_SYSTEM_SECURITY may only be satisfied
    by the caller having SeSecurityPrivilege.

    Note that this routine is only to be called when an object is being
    created.  When an object is being opened, it is expected that
    NtAccessCheck will be called, and that routine will implement
    another policy for substituting privileges for DACL access.

Arguments:

    DesiredAccess - Supplies the user's desired access mask

    Privileges - Supplies a pointer to an empty buffer in which will
        be returned a privilege set describing any privileges that were
        used to gain access.

        Note that this is not an optional parameter, that is, enough
        room for a single privilege must always be passed.

    Length - Supplies the length of the Privileges parameter in bytes.
        If the supplies length is not adequate to store the entire
        privilege set, this field will return the minimum length required.

    Token - (optionally) Supplies the token for the client on whose
        behalf the object is being accessed.  If this value is
        specified as null, then the token on the thread is opened and
        examined to see if it is an impersonation token.  If it is,
        then it must be at SecurityIdentification level or higher.  If
        it is not an impersonation token, the operation proceeds
        normally.

    GenericMapping - Supplies the generic mapping associated with this
        object type.

    RemainingDesiredAccess - Returns the DesiredAccess mask after any bits
        have been masked off.  If no access types could be granted, this
        mask will be identical to the one passed in.

Return Value:

    STATUS_SUCCESS - The operation completed successfully.

    STATUS_BUFFER_TOO_SMALL - The passed buffer was not large enough
        to contain the information being returned.

    STATUS_BAD_IMPERSONATION_LEVEL - The caller or passed token was
        impersonating, but not at a high enough level.


--*/

{
    PRIVILEGE_SET RequiredPrivilege;
    BOOLEAN Result = FALSE;
    NTSTATUS Status;
    ULONG PrivilegeCount = 0;
    HANDLE ThreadToken;
    BOOLEAN TokenPassed;
    TOKEN_STATISTICS ThreadTokenStatistics;
    ULONG ReturnLength;
    ULONG SizeRequired;
    ULONG PrivilegeNumber = 0;


    //
    //  If the caller hasn't passed a token, call the kernel and get
    //  his impersonation token.  This call will fail if the caller is
    //  not impersonating a client, so if the caller is not
    //  impersonating someone, he'd better have passed in an explicit
    //  token.
    //

    if (!ARGUMENT_PRESENT( Token )) {

        Status = NtOpenThreadToken(
                     NtCurrentThread(),
                     TOKEN_QUERY,
                     TRUE,
                     &ThreadToken
                     );

        TokenPassed = FALSE;

        if (!NT_SUCCESS( Status )) {
            return( Status );
        }

    } else {

        ThreadToken = Token;
        TokenPassed = TRUE;
    }

    Status = NtQueryInformationToken(
                 ThreadToken,                  // Handle
                 TokenStatistics,              // TokenInformationClass
                 &ThreadTokenStatistics,       // TokenInformation
                 sizeof(TOKEN_STATISTICS),     // TokenInformationLength
                 &ReturnLength                 // ReturnLength
                 );

    ASSERT( NT_SUCCESS(Status) );

    RtlMapGenericMask(
        &DesiredAccess,
        GenericMapping
        );

    *RemainingDesiredAccess = DesiredAccess;

    if ( DesiredAccess & ACCESS_SYSTEM_SECURITY ) {

        RequiredPrivilege.PrivilegeCount = 1;
        RequiredPrivilege.Control = PRIVILEGE_SET_ALL_NECESSARY;
        RequiredPrivilege.Privilege[0].Luid = RtlConvertLongToLuid(SE_SECURITY_PRIVILEGE);
        RequiredPrivilege.Privilege[0].Attributes = 0;

        //
        // NtPrivilegeCheck will make sure we are impersonating
        // properly.
        //

        Status = NtPrivilegeCheck(
                     ThreadToken,
                     &RequiredPrivilege,
                     &Result
                     );

        if ( (!NT_SUCCESS ( Status )) || (!Result) ) {

            if (!TokenPassed) {
                NtClose( ThreadToken );
            }

            if ( !NT_SUCCESS( Status )) {
                return( Status );
            }

            if ( !Result ) {
                return( STATUS_PRIVILEGE_NOT_HELD );
            }

        }

        //
        // We have the required privilege, turn off the bit in
        // copy of the input mask and remember that we need to return
        // this privilege.
        //

        *RemainingDesiredAccess &= ~ACCESS_SYSTEM_SECURITY;
    }

    if (!TokenPassed) {
        NtClose( ThreadToken );
    }

    SizeRequired = sizeof(PRIVILEGE_SET);

    if ( SizeRequired > *Length ) {
        *Length = SizeRequired;
        return( STATUS_BUFFER_TOO_SMALL );
    }

    if (Result) {

        Privileges->PrivilegeCount = 1;
        Privileges->Control = 0;
        Privileges->Privilege[PrivilegeNumber].Luid = RtlConvertLongToLuid(SE_SECURITY_PRIVILEGE);
        Privileges->Privilege[PrivilegeNumber].Attributes = SE_PRIVILEGE_USED_FOR_ACCESS;

    } else {

        Privileges->PrivilegeCount = 0;
        Privileges->Control = 0;
        Privileges->Privilege[PrivilegeNumber].Luid = RtlConvertLongToLuid(0);
        Privileges->Privilege[PrivilegeNumber].Attributes = 0;

    }

    return( STATUS_SUCCESS );

}



NTSTATUS
RtlCopySecurityDescriptor(
    IN PSECURITY_DESCRIPTOR InputSecurityDescriptor,
    OUT PSECURITY_DESCRIPTOR *OutputSecurityDescriptor
    )

/*++

Routine Description:

    This routine will copy a self-relative security descriptor from
    any memory into the correct type of memory required by security
    descriptor Rtl routines.

    This allows security descriptors to be kept in whatever kind of
    storage is most convenient for the current application.  A security
    descriptor should be copied via this routine and the copy passed
    into any Rtl routine that in any way modify the security descriptor
    (eg RtlSetSecurityObject).

    The storage allocated by this routine must be freed by
    RtlDeleteSecurityObject.

Arguments:

    InputSecurityDescriptor - contains the source security descriptor

    OutputSecurityDescriptor - returns a copy of the security descriptor
        in the correct kind of memory.


Return Value:

    STATUS_NO_MEMORY - There was not enough memory available to the current
        process to complete this operation.

--*/

{

    PACL Dacl;
    PACL Sacl;

    PSID Owner;
    PSID PrimaryGroup;

    ULONG DaclSize;
    ULONG OwnerSize;
    ULONG PrimaryGroupSize;
    ULONG SaclSize;
    ULONG TotalSize;

    PISECURITY_DESCRIPTOR ISecurityDescriptor =
                            (PISECURITY_DESCRIPTOR)InputSecurityDescriptor;


    RtlpQuerySecurityDescriptor(
        ISecurityDescriptor,
        &Owner,
        &OwnerSize,
        &PrimaryGroup,
        &PrimaryGroupSize,
        &Dacl,
        &DaclSize,
        &Sacl,
        &SaclSize
        );

    TotalSize = sizeof(SECURITY_DESCRIPTOR) +
                OwnerSize +
                PrimaryGroupSize +
                DaclSize +
                SaclSize;

    *OutputSecurityDescriptor = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( SE_TAG ), TotalSize );

    if ( *OutputSecurityDescriptor == NULL ) {
        return( STATUS_NO_MEMORY );
    }

    RtlMoveMemory( *OutputSecurityDescriptor,
                   ISecurityDescriptor,
                   TotalSize
                   );

    return( STATUS_SUCCESS );

}


NTSTATUS
RtlpInitializeAllowedAce(
    IN  PACCESS_ALLOWED_ACE AllowedAce,
    IN  USHORT AceSize,
    IN  UCHAR InheritFlags,
    IN  UCHAR AceFlags,
    IN  ACCESS_MASK Mask,
    IN  PSID AllowedSid
    )
/*++

Routine Description:

    This function assigns the specified ACE values into an allowed type ACE.

Arguments:

    AllowedAce - Supplies a pointer to the ACE that is initialized.

    AceSize - Supplies the size of the ACE in bytes.

    InheritFlags - Supplies ACE inherit flags.

    AceFlags - Supplies ACE type specific control flags.

    Mask - Supplies the allowed access masks.

    AllowedSid - Supplies the pointer to the SID of user/group which is allowed
        the specified access.

Return Value:

    Returns status from RtlCopySid.

--*/
{
    AllowedAce->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    AllowedAce->Header.AceSize = AceSize;
    AllowedAce->Header.AceFlags = AceFlags | InheritFlags;

    AllowedAce->Mask = Mask;

    return RtlCopySid(
               RtlLengthSid(AllowedSid),
               &(AllowedAce->SidStart),
               AllowedSid
               );
}


NTSTATUS
RtlpInitializeDeniedAce(
    IN  PACCESS_DENIED_ACE DeniedAce,
    IN  USHORT AceSize,
    IN  UCHAR InheritFlags,
    IN  UCHAR AceFlags,
    IN  ACCESS_MASK Mask,
    IN  PSID DeniedSid
    )
/*++

Routine Description:

    This function assigns the specified ACE values into a denied type ACE.

Arguments:

    DeniedAce - Supplies a pointer to the ACE that is initialized.

    AceSize - Supplies the size of the ACE in bytes.

    InheritFlags - Supplies ACE inherit flags.

    AceFlags - Supplies ACE type specific control flags.

    Mask - Supplies the denied access masks.

    AllowedSid - Supplies the pointer to the SID of user/group which is denied
        the specified access.

Return Value:

    Returns status from RtlCopySid.

--*/
{
    DeniedAce->Header.AceType = ACCESS_DENIED_ACE_TYPE;
    DeniedAce->Header.AceSize = AceSize;
    DeniedAce->Header.AceFlags = AceFlags | InheritFlags;

    DeniedAce->Mask = Mask;

    return RtlCopySid(
               RtlLengthSid(DeniedSid),
               &(DeniedAce->SidStart),
               DeniedSid
               );
}


NTSTATUS
RtlpInitializeAuditAce(
    IN  PACCESS_ALLOWED_ACE AuditAce,
    IN  USHORT AceSize,
    IN  UCHAR InheritFlags,
    IN  UCHAR AceFlags,
    IN  ACCESS_MASK Mask,
    IN  PSID AuditSid
    )
/*++

Routine Description:

    This function assigns the specified ACE values into an audit type ACE.

Arguments:

    AuditAce - Supplies a pointer to the ACE that is initialized.

    AceSize - Supplies the size of the ACE in bytes.

    InheritFlags - Supplies ACE inherit flags.

    AceFlags - Supplies ACE type specific control flags.

    Mask - Supplies the allowed access masks.

    AuditSid - Supplies the pointer to the SID of user/group which is to be
        audited.

Return Value:

    Returns status from RtlCopySid.

--*/
{
    AuditAce->Header.AceType = SYSTEM_AUDIT_ACE_TYPE;
    AuditAce->Header.AceSize = AceSize;
    AuditAce->Header.AceFlags = AceFlags | InheritFlags;

    AuditAce->Mask = Mask;

    return RtlCopySid(
               RtlLengthSid(AuditSid),
               &(AuditAce->SidStart),
               AuditSid
               );
}

NTSTATUS
RtlCreateAndSetSD(
    IN  PRTL_ACE_DATA AceData,
    IN  ULONG AceCount,
    IN  PSID OwnerSid OPTIONAL,
    IN  PSID GroupSid OPTIONAL,
    OUT PSECURITY_DESCRIPTOR *NewDescriptor
    )
/*++

Routine Description:

    This function creates an absolute security descriptor containing
    the supplied ACE information.

    A sample usage of this function:

        //
        // Order matters!  These ACEs are inserted into the DACL in the
        // following order.  Security access is granted or denied based on
        // the order of the ACEs in the DACL.
        //

        RTL_ACE_DATA AceData[4] = {
            {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
                   GENERIC_ALL,                  &LocalAdminSid},

            {ACCESS_DENIED_ACE_TYPE,  0, 0,
                   GENERIC_ALL,                  &NetworkSid},

            {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
                   WKSTA_CONFIG_GUEST_INFO_GET |
                   WKSTA_CONFIG_USER_INFO_GET,   &DomainUsersSid},

            {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
                   WKSTA_CONFIG_GUEST_INFO_GET,  &DomainGuestsSid}
            };

        PSECURITY_DESCRIPTOR WkstaSecurityDescriptor;


        return RtlCreateAndSetSD(
                   AceData,
                   4,
                   LocalSystemSid,
                   LocalSystemSid,
                   &WkstaSecurityDescriptor
                   );

Arguments:

    AceData - Supplies the structure of information that describes the DACL.

    AceCount - Supplies the number of entries in AceData structure.

    OwnerSid - Supplies the pointer to the SID of the security descriptor
        owner.  If not specified, a security descriptor with no owner
        will be created.

    GroupSid - Supplies the pointer to the SID of the security descriptor
        primary group.  If not specified, a security descriptor with no primary
        group will be created.

    NewDescriptor - Returns a pointer to the absolute security descriptor
        allocated using RtlAllocateHeap.

Return Value:

    STATUS_SUCCESS - if successful
    STATUS_NO_MEMORY - if cannot allocate memory for DACL, ACEs, and
        security descriptor.

    Any other status codes returned from the security Rtl routines.

    NOTE : the user security object created by calling this function may be
                freed up by calling RtlDeleteSecurityObject().

--*/
{

    NTSTATUS ntstatus = STATUS_SUCCESS;
    ULONG i;

    //
    // Pointer to memory dynamically allocated by this routine to hold
    // the absolute security descriptor, the DACL, the SACL, and all the ACEs.
    //
    // +---------------------------------------------------------------+
    // |                     Security Descriptor                       |
    // +-------------------------------+-------+---------------+-------+
    // |          DACL                 | ACE 1 |   .  .  .     | ACE n |
    // +-------------------------------+-------+---------------+-------+
    // |          SACL                 | ACE 1 |   .  .  .     | ACE n |
    // +-------------------------------+-------+---------------+-------+
    //

    PSECURITY_DESCRIPTOR AbsoluteSd = NULL;
    PACL Dacl = NULL;   // Pointer to the DACL portion of above buffer
    PACL Sacl = NULL;   // Pointer to the SACL portion of above buffer

    ULONG DaclSize = sizeof(ACL);
    ULONG SaclSize = sizeof(ACL);
    ULONG MaxAceSize = 0;
    PVOID MaxAce = NULL;

    PCHAR CurrentAvailable;
    ULONG Size;

    PVOID HeapHandle = RtlProcessHeap();


    ASSERT( AceCount > 0 );

    //
    // Compute the total size of the DACL and SACL ACEs and the maximum
    // size of any ACE.
    //

    for (i = 0; i < AceCount; i++) {
        ULONG AceSize;

        AceSize = RtlLengthSid(*(AceData[i].Sid));

        switch (AceData[i].AceType) {
        case ACCESS_ALLOWED_ACE_TYPE:
            AceSize += sizeof(ACCESS_ALLOWED_ACE);
            DaclSize += AceSize;
            break;

        case ACCESS_DENIED_ACE_TYPE:
            AceSize += sizeof(ACCESS_DENIED_ACE);
            DaclSize += AceSize;
            break;

        case SYSTEM_AUDIT_ACE_TYPE:
            AceSize += sizeof(SYSTEM_AUDIT_ACE);
            SaclSize += AceSize;
            break;

        default:
            return STATUS_INVALID_PARAMETER;
        }

        MaxAceSize = MaxAceSize > AceSize ? MaxAceSize : AceSize;
    }

    //
    // Allocate a chunk of memory large enough for the security descriptor,
    // the DACL, the SACL and all ACEs.
    //
    // A security descriptor is of opaque data type but
    // SECURITY_DESCRIPTOR_MIN_LENGTH is the right size.
    //

    Size = SECURITY_DESCRIPTOR_MIN_LENGTH;
    if ( DaclSize != sizeof(ACL) ) {
        Size += DaclSize;
    }
    if ( SaclSize != sizeof(ACL) ) {
        Size += SaclSize;
    }

    if ((AbsoluteSd = RtlAllocateHeap(
                          HeapHandle, MAKE_TAG( SE_TAG ),
                          Size
                          )) == NULL) {
        ntstatus = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    //
    // Initialize the Dacl and Sacl
    //

    CurrentAvailable = (PCHAR)AbsoluteSd + SECURITY_DESCRIPTOR_MIN_LENGTH;

    if ( DaclSize != sizeof(ACL) ) {
        Dacl = (PACL)CurrentAvailable;
        CurrentAvailable += DaclSize;

        ntstatus = RtlCreateAcl( Dacl, DaclSize, ACL_REVISION );

        if ( !NT_SUCCESS(ntstatus) ) {
            goto Cleanup;
        }
    }

    if ( SaclSize != sizeof(ACL) ) {
        Sacl = (PACL)CurrentAvailable;
        CurrentAvailable += SaclSize;

        ntstatus = RtlCreateAcl( Sacl, SaclSize, ACL_REVISION );

        if ( !NT_SUCCESS(ntstatus) ) {
            goto Cleanup;
        }
    }

    //
    // Allocate a temporary buffer big enough for the biggest ACE.
    //

    if ((MaxAce = RtlAllocateHeap(
                      HeapHandle, MAKE_TAG( SE_TAG ),
                      MaxAceSize
                      )) == NULL ) {
        ntstatus = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    //
    // Initialize each ACE, and append it into the end of the DACL or SACL.
    //

    for (i = 0; i < AceCount; i++) {
        ULONG AceSize;
        PACL CurrentAcl;

        AceSize = RtlLengthSid(*(AceData[i].Sid));

        switch (AceData[i].AceType) {
        case ACCESS_ALLOWED_ACE_TYPE:

            AceSize += sizeof(ACCESS_ALLOWED_ACE);
            CurrentAcl = Dacl;
            ntstatus = RtlpInitializeAllowedAce(
                           MaxAce,
                           (USHORT) AceSize,
                           AceData[i].InheritFlags,
                           AceData[i].AceFlags,
                           AceData[i].Mask,
                           *(AceData[i].Sid)
                           );
            break;

        case ACCESS_DENIED_ACE_TYPE:
            AceSize += sizeof(ACCESS_DENIED_ACE);
            CurrentAcl = Dacl;
            ntstatus = RtlpInitializeDeniedAce(
                           MaxAce,
                           (USHORT) AceSize,
                           AceData[i].InheritFlags,
                           AceData[i].AceFlags,
                           AceData[i].Mask,
                           *(AceData[i].Sid)
                           );
            break;

        case SYSTEM_AUDIT_ACE_TYPE:
            AceSize += sizeof(SYSTEM_AUDIT_ACE);
            CurrentAcl = Sacl;
            ntstatus = RtlpInitializeAuditAce(
                           MaxAce,
                           (USHORT) AceSize,
                           AceData[i].InheritFlags,
                           AceData[i].AceFlags,
                           AceData[i].Mask,
                           *(AceData[i].Sid)
                           );
            break;
        }

        if ( !NT_SUCCESS( ntstatus ) ) {
            goto Cleanup;
        }

        //
        // Append the initialized ACE to the end of DACL or SACL
        //

        if (! NT_SUCCESS (ntstatus = RtlAddAce(
                                         CurrentAcl,
                                         ACL_REVISION,
                                         MAXULONG,
                                         MaxAce,
                                         AceSize
                                         ))) {
            goto Cleanup;
        }
    }

    //
    // Create the security descriptor with absolute pointers to SIDs
    // and ACLs.
    //
    // Owner = OwnerSid
    // Group = GroupSid
    // Dacl  = Dacl
    // Sacl  = Sacl
    //

    if (! NT_SUCCESS(ntstatus = RtlCreateSecurityDescriptor(
                                    AbsoluteSd,
                                    SECURITY_DESCRIPTOR_REVISION
                                    ))) {
        goto Cleanup;
    }

    if (! NT_SUCCESS(ntstatus = RtlSetOwnerSecurityDescriptor(
                                    AbsoluteSd,
                                    OwnerSid,
                                    FALSE
                                    ))) {
        goto Cleanup;
    }

    if (! NT_SUCCESS(ntstatus = RtlSetGroupSecurityDescriptor(
                                    AbsoluteSd,
                                    GroupSid,
                                    FALSE
                                    ))) {
        goto Cleanup;
    }

    if (! NT_SUCCESS(ntstatus = RtlSetDaclSecurityDescriptor(
                                    AbsoluteSd,
                                    TRUE,
                                    Dacl,
                                    FALSE
                                    ))) {
        goto Cleanup;
    }

    if (! NT_SUCCESS(ntstatus = RtlSetSaclSecurityDescriptor(
                                    AbsoluteSd,
                                    FALSE,
                                    Sacl,
                                    FALSE
                                    ))) {
        goto Cleanup;
    }

    //
    // Done
    //

    ntstatus = STATUS_SUCCESS;

    //
    // Clean up
    //

Cleanup:
    //
    // Either return the security descriptor to the caller or delete it
    //

    if ( NT_SUCCESS( ntstatus ) ) {
        *NewDescriptor = AbsoluteSd;
    } else if ( AbsoluteSd != NULL ) {
        (void) RtlFreeHeap(HeapHandle, 0, AbsoluteSd);
    }

    //
    // Delete the temporary ACE
    //

    if ( MaxAce != NULL ) {
        (void) RtlFreeHeap(HeapHandle, 0, MaxAce);
    }
    return ntstatus;
}


NTSTATUS
RtlCreateUserSecurityObject(
    IN  PRTL_ACE_DATA AceData,
    IN  ULONG AceCount,
    IN  PSID OwnerSid,
    IN  PSID GroupSid,
    IN  BOOLEAN IsDirectoryObject,
    IN  PGENERIC_MAPPING GenericMapping,
    OUT PSECURITY_DESCRIPTOR *NewDescriptor
    )
/*++

Routine Description:

    This function creates the DACL for the security descriptor based on
    on the ACE information specified, and creates the security descriptor
    which becomes the user-mode security object.

    A sample usage of this function:

        //
        // Structure that describes the mapping of Generic access rights to
        // object specific access rights for the ConfigurationInfo object.
        //

        GENERIC_MAPPING WsConfigInfoMapping = {
            STANDARD_RIGHTS_READ            |      // Generic read
                WKSTA_CONFIG_GUEST_INFO_GET |
                WKSTA_CONFIG_USER_INFO_GET  |
                WKSTA_CONFIG_ADMIN_INFO_GET,
            STANDARD_RIGHTS_WRITE |                // Generic write
                WKSTA_CONFIG_INFO_SET,
            STANDARD_RIGHTS_EXECUTE,               // Generic execute
            WKSTA_CONFIG_ALL_ACCESS                // Generic all
            };

        //
        // Order matters!  These ACEs are inserted into the DACL in the
        // following order.  Security access is granted or denied based on
        // the order of the ACEs in the DACL.
        //

        RTL_ACE_DATA AceData[4] = {
            {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
                   GENERIC_ALL,                  &LocalAdminSid},

            {ACCESS_DENIED_ACE_TYPE,  0, 0,
                   GENERIC_ALL,                  &NetworkSid},

            {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
                   WKSTA_CONFIG_GUEST_INFO_GET |
                   WKSTA_CONFIG_USER_INFO_GET,   &DomainUsersSid},

            {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
                   WKSTA_CONFIG_GUEST_INFO_GET,  &DomainGuestsSid}
            };

        PSECURITY_DESCRIPTOR WkstaSecurityObject;


        return RtlCreateUserSecurityObject(
                   AceData,
                   4,
                   LocalSystemSid,
                   LocalSystemSid,
                   FALSE,
                   &WsConfigInfoMapping,
                   &WkstaSecurityObject
                   );

Arguments:

    AceData - Supplies the structure of information that describes the DACL.

    AceCount - Supplies the number of entries in AceData structure.

    OwnerSid - Supplies the pointer to the SID of the security descriptor
        owner.

    GroupSid - Supplies the pointer to the SID of the security descriptor
        primary group.

    IsDirectoryObject - Supplies the flag which indicates whether the
        user-mode object is a directory object.

    GenericMapping - Supplies the pointer to a generic mapping array denoting
        the mapping between each generic right to specific rights.

    NewDescriptor - Returns a pointer to the self-relative security descriptor
        which represents the user-mode object.

Return Value:

    STATUS_SUCCESS - if successful
    STATUS_NO_MEMORY - if cannot allocate memory for DACL, ACEs, and
        security descriptor.

    Any other status codes returned from the security Rtl routines.

    NOTE : the user security object created by calling this function may be
                freed up by calling RtlDeleteSecurityObject().

--*/
{

    NTSTATUS ntstatus;
    PSECURITY_DESCRIPTOR AbsoluteSd;
    HANDLE TokenHandle;
    PVOID HeapHandle = RtlProcessHeap();

    ntstatus = RtlCreateAndSetSD(
                   AceData,
                   AceCount,
                   OwnerSid,
                   GroupSid,
                   &AbsoluteSd
                   );

    if (! NT_SUCCESS(ntstatus)) {
        return ntstatus;
    }

    ntstatus = NtOpenProcessToken(
                   NtCurrentProcess(),
                   TOKEN_QUERY,
                   &TokenHandle
                   );

    if (! NT_SUCCESS(ntstatus)) {
        (void) RtlFreeHeap(HeapHandle, 0, AbsoluteSd);
        return ntstatus;
    }

    //
    // Create the security object (a user-mode object is really a pseudo-
    // object represented by a security descriptor that have relative
    // pointers to SIDs and ACLs).  This routine allocates the memory to
    // hold the relative security descriptor so the memory allocated for the
    // DACL, ACEs, and the absolute descriptor can be freed.
    //
    ntstatus = RtlNewSecurityObject(
                   NULL,                   // Parent descriptor
                   AbsoluteSd,             // Creator descriptor
                   NewDescriptor,          // Pointer to new descriptor
                   IsDirectoryObject,      // Is directory object
                   TokenHandle,            // Token
                   GenericMapping          // Generic mapping
                   );

    (void) NtClose(TokenHandle);

    //
    // Free dynamic memory before returning
    //
    (void) RtlFreeHeap(HeapHandle, 0, AbsoluteSd);
    return ntstatus;
}





NTSTATUS
RtlConvertToAutoInheritSecurityObject(
    IN PSECURITY_DESCRIPTOR ParentDescriptor OPTIONAL,
    IN PSECURITY_DESCRIPTOR CurrentSecurityDescriptor,
    OUT PSECURITY_DESCRIPTOR *NewSecurityDescriptor,
    IN GUID *ObjectType OPTIONAL,
    IN BOOLEAN IsDirectoryObject,
    IN PGENERIC_MAPPING GenericMapping
    )
/*++

Routine Description:

    This is a converts a security descriptor whose ACLs are not marked
    as AutoInherit to a security descriptor whose ACLs are marked as
    AutoInherit.

    See further detailed description on ConvertToAutoInheritPrivateObjectSecurity.

Arguments:

    ParentDescriptor - Supplies the Security Descriptor for the parent
        directory under which a object exists.  If there is
        no parent directory, then this argument is specified as NULL.

    CurrentSecurityDescriptor - Supplies a pointer to the objects security descriptor
        that is going to be altered by this procedure.

    NewSecurityDescriptor Points to a pointer that is to be made to point to the
        newly allocated self-relative security descriptor. When no
        longer needed, this descriptor must be freed using
        DestroyPrivateObjectSecurity().

    ObjectType - GUID of the object type being created.  If the object being
        created has no GUID associated with it, then this argument is
        specified as NULL.

    IsDirectoryObject - Specifies if the object is a
        directory object.  A value of TRUE indicates the object is a
        container of other objects.

    GenericMapping - Supplies a pointer to a generic mapping array denoting
        the mapping between each generic right to specific rights.

Return Value:

    STATUS_SUCCESS - The operation was successful.

    STATUS_UNKNOWN_REVISION - Indicates the source ACL is a revision that
        is unknown to this routine.  (Only revision 2 ACLs are support by this routine.)

    STATUS_INVALID_ACL - The structure of one of the ACLs in invalid.



--*/
{

    //
    // Simply call the corresponding Rtlp routine telling it which allocator
    //  to use.
    //

    return RtlpConvertToAutoInheritSecurityObject(
                            ParentDescriptor,
                            CurrentSecurityDescriptor,
                            NewSecurityDescriptor,
                            ObjectType,
                            IsDirectoryObject,
                            GenericMapping );

}


NTSTATUS
RtlDefaultNpAcl(
    OUT PACL * pAcl
    )
/*++

Routine Description:

    This routine constructs a default ACL to be applied to
    named pipe objects when the caller has not specified one.
    See NT bug 131090.

    The ACL constructed is as follows:

    Need to build an ACL that looks like the following:

     Local System : F
     Administrators: F
     Owner: F
     Everyone: R

     The owner is determined by querying the currently effective
     toke and extracting the default owner.

Arguments:

    pAcl - Receives a pointer to an ACL to apply to the named pipe
        being created.  Guaranteed to be NULL on return if an error
        occurs.

        This must be freed by calling RtlFreeHeap.

Return Value:

    NT Status.

--*/
{
    SID_IDENTIFIER_AUTHORITY    NtAuthority         = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY    CreatorSidAuthority = SECURITY_CREATOR_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY    WorldSidAuthority   = SECURITY_WORLD_SID_AUTHORITY;

    ULONG AclSize         = 0;
    NTSTATUS Status       = STATUS_SUCCESS;
    ULONG ReturnLength    = 0;
    PTOKEN_OWNER OwnerSid = NULL;

    HANDLE hToken;

    //
    // Initialize OUT parameters
    //

    *pAcl = NULL;

    //
    // Open thread token
    //

    Status = NtOpenThreadToken(
                 NtCurrentThread(),
                 TOKEN_QUERY,
                 TRUE,
                 &hToken
                 );

    if (STATUS_NO_TOKEN == Status) {

        //
        // Not impersonating, get process token
        //

        Status = NtOpenProcessToken(
                     NtCurrentProcess(),
                     TOKEN_QUERY,
                     &hToken
                     );
    }

    if (NT_SUCCESS( Status )) {

        //
        // Get the default owner
        //

        Status = NtQueryInformationToken (
                     hToken,
                     TokenOwner,
                     NULL,
                     0,
                     &ReturnLength
                     );

        if (STATUS_BUFFER_TOO_SMALL == Status) {

            OwnerSid = (PTOKEN_OWNER)RtlAllocateHeap( RtlProcessHeap(), 0, ReturnLength );

            if (OwnerSid) {

                Status = NtQueryInformationToken (
                             hToken,
                             TokenOwner,
                             OwnerSid,
                             ReturnLength,
                             &ReturnLength
                             );

                if (NT_SUCCESS( Status )) {

                    //
                    // Compute the size needed
                    //

                    UCHAR SidBuffer[16];
                    ASSERT( 16 == RtlLengthRequiredSid( 2 ));

                    AclSize += RtlLengthRequiredSid( 1 );   // LocalSystem Sid
                    AclSize += RtlLengthRequiredSid( 2 );   // Administrators
                    AclSize += RtlLengthRequiredSid( 1 );   // Everyone (World)

                    AclSize += RtlLengthSid( OwnerSid->Owner );   // Owner

                    AclSize += sizeof( ACL );               // Header
                    AclSize += 4 * (sizeof( ACCESS_ALLOWED_ACE ) - sizeof( ULONG ));

                    //
                    // Allocate the Acl out of the local process heap
                    //

                    *pAcl = (PACL)RtlAllocateHeap( RtlProcessHeap(), 0, AclSize );

                    if (*pAcl != NULL) {

                        RtlCreateAcl( *pAcl, AclSize, ACL_REVISION );

                        //
                        // Create each SID in turn and copy the resultant ACE into
                        // the new ACL
                        //

                        //
                        // Local System - Generic All
                        //

                        RtlInitializeSid( SidBuffer, &NtAuthority, 1);
                        *(RtlSubAuthoritySid( SidBuffer, 0 )) = SECURITY_LOCAL_SYSTEM_RID;
                        RtlAddAccessAllowedAce( *pAcl, ACL_REVISION, GENERIC_ALL, (PSID)SidBuffer );

                        //
                        // Admins - Generic All
                        //

                        RtlInitializeSid( SidBuffer, &NtAuthority, 2);
                        *(RtlSubAuthoritySid( SidBuffer, 0 )) = SECURITY_BUILTIN_DOMAIN_RID;
                        *(RtlSubAuthoritySid( SidBuffer, 1 )) = DOMAIN_ALIAS_RID_ADMINS;
                        RtlAddAccessAllowedAce( *pAcl, ACL_REVISION, GENERIC_ALL, (PSID)SidBuffer );

                        //
                        // Owner - Generic All
                        //

                        RtlAddAccessAllowedAce( *pAcl, ACL_REVISION, GENERIC_ALL, OwnerSid->Owner );

                        //
                        // World - Generic Read
                        //

                        RtlInitializeSid( SidBuffer, &WorldSidAuthority, 1 );
                        *(RtlSubAuthoritySid( SidBuffer, 0 )) = SECURITY_WORLD_RID;
                        RtlAddAccessAllowedAce( *pAcl, ACL_REVISION, GENERIC_READ, (PSID)SidBuffer );

                    } else {

                        Status = STATUS_NO_MEMORY;
                    }
                }

                RtlFreeHeap( RtlProcessHeap(), 0, OwnerSid );

            } else {

                Status = STATUS_NO_MEMORY;
            }
        }

        NtClose( hToken );
    }

    if (!NT_SUCCESS( Status )) {

        //
        // Something failed, clean up OUT
        // parameters.
        //

        if (*pAcl != NULL) {
            RtlFreeHeap( RtlProcessHeap(), 0, *pAcl );
            *pAcl = NULL;
        }
    }

    return( Status );
}
