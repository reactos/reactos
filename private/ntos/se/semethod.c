/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Semethod.c

Abstract:

    This Module implements the SeDefaultObjectMethod procedure.  This
    procedure and SeAssignSecurity are the only two procedures that will
    place a security descriptor on an object.  Therefore they must understand
    and agree on how a descriptor is allocated from pool so that they can
    deallocate and reallocate pool as necessary. Any security descriptor
    that is attached to an object by these procedures has the following
    pool allocation plan.

    1. if the objects security descriptor is null then there is no pool
       allocated

    2. otherwise there is at least one pool allocation for the security
       descriptor header.  if it's acl fields are null then there are no
       other pool allocations (this should never happen).

    3. There is a separate pool allocation for each acl in the descriptor.
       So a maximum of three pool allocations can occur for each attached
       security descriptor.

    4  Everytime an acl is replace in a descriptor we see if we can use
       the old acl and if so then we try and keep the acl size as large
       as possible.

    Note that this is different from the algorithm used to capture
    a security descriptor (which puts everything in one pool allocation).
    Also note that this can be easily optimized at a later time (if necessary)
    to use only one allocation.



Author:

    Gary Kimura     (GaryKi)    9-Nov-1989
    Jim Kelly       (JimK)     10-May-1990

Environment:

    Kernel Mode

Revision History:


--*/

#include "sep.h"
#include "sertlp.h"

NTSTATUS
SepDefaultDeleteMethod (
    IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor
    );



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,SeSetSecurityAccessMask)
#pragma alloc_text(PAGE,SeQuerySecurityAccessMask)
#pragma alloc_text(PAGE,SeDefaultObjectMethod)
#pragma alloc_text(PAGE,SeSetSecurityDescriptorInfo)
#pragma alloc_text(PAGE,SeQuerySecurityDescriptorInfo)
#pragma alloc_text(PAGE,SepDefaultDeleteMethod)
#endif




VOID
SeSetSecurityAccessMask(
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PACCESS_MASK DesiredAccess
    )

/*++

Routine Description:

    This routine builds an access mask representing the accesses necessary
    to set the object security information specified in the SecurityInformation
    parameter.  While it is not difficult to determine this information,
    the use of a single routine to generate it will ensure minimal impact
    when the security information associated with an object is extended in
    the future (to include mandatory access control information).

Arguments:

    SecurityInformation - Identifies the object's security information to be
        modified.

    DesiredAccess - Points to an access mask to be set to represent the
        accesses necessary to modify the information specified in the
        SecurityInformation parameter.

Return Value:

    None.

--*/

{

    PAGED_CODE();

    //
    // Figure out accesses needed to perform the indicated operation(s).
    //

    (*DesiredAccess) = 0;

    if ((SecurityInformation & OWNER_SECURITY_INFORMATION) ||
        (SecurityInformation & GROUP_SECURITY_INFORMATION)   ) {
        (*DesiredAccess) |= WRITE_OWNER;
    }

    if (SecurityInformation & DACL_SECURITY_INFORMATION) {
        (*DesiredAccess) |= WRITE_DAC;
    }

    if (SecurityInformation & SACL_SECURITY_INFORMATION) {
        (*DesiredAccess) |= ACCESS_SYSTEM_SECURITY;
    }

    return;

}


VOID
SeQuerySecurityAccessMask(
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PACCESS_MASK DesiredAccess
    )

/*++

Routine Description:

    This routine builds an access mask representing the accesses necessary
    to query the object security information specified in the
    SecurityInformation parameter.  While it is not difficult to determine
    this information, the use of a single routine to generate it will ensure
    minimal impact when the security information associated with an object is
    extended in the future (to include mandatory access control information).

Arguments:

    SecurityInformation - Identifies the object's security information to be
        queried.

    DesiredAccess - Points to an access mask to be set to represent the
        accesses necessary to query the information specified in the
        SecurityInformation parameter.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    // Figure out accesses needed to perform the indicated operation(s).
    //

    (*DesiredAccess) = 0;

    if ((SecurityInformation & OWNER_SECURITY_INFORMATION) ||
        (SecurityInformation & GROUP_SECURITY_INFORMATION) ||
        (SecurityInformation & DACL_SECURITY_INFORMATION)) {
        (*DesiredAccess) |= READ_CONTROL;
    }

    if ((SecurityInformation & SACL_SECURITY_INFORMATION)) {
        (*DesiredAccess) |= ACCESS_SYSTEM_SECURITY;
    }

    return;

}



NTSTATUS
SeDefaultObjectMethod (
    IN PVOID Object,
    IN SECURITY_OPERATION_CODE OperationCode,
    IN PSECURITY_INFORMATION SecurityInformation,
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN OUT PULONG CapturedLength,
    IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    IN POOL_TYPE PoolType,
    IN PGENERIC_MAPPING GenericMapping
    )

/*++

Routine Description:

    This is the default security method for objects.  It is responsible
    for either retrieving, setting, and deleting the security descriptor of
    an object.  It is not used to assign the original security descriptor
    to an object (use SeAssignSecurity for that purpose).


    IT IS ASSUMED THAT THE OBJECT MANAGER HAS ALREADY DONE THE ACCESS
    VALIDATIONS NECESSARY TO ALLOW THE REQUESTED OPERATIONS TO BE PERFORMED.

Arguments:

    Object - Supplies a pointer to the object being used.

    OperationCode - Indicates if the operation is for setting, querying, or
        deleting the object's security descriptor.

    SecurityInformation - Indicates which security information is being
        queried or set.  This argument is ignored for the delete operation.

    SecurityDescriptor - The meaning of this parameter depends on the
        OperationCode:

        QuerySecurityDescriptor - For the query operation this supplies the
            buffer to copy the descriptor into.  The security descriptor is
            assumed to have been probed up to the size passed in in Length.
            Since it still points into user space, it must always be
            accessed in a try clause in case it should suddenly disappear.

        SetSecurityDescriptor - For a set operation this supplies the
            security descriptor to copy into the object.  The security
            descriptor must be captured before this routine is called.

        DeleteSecurityDescriptor - It is ignored when deleting a security
            descriptor.

        AssignSecurityDescriptor - For assign operations this is the
            security descriptor that will be assigned to the object.
            It is assumed to be in kernel space, and is therefore not
            probed or captured.

    CapturedLength - For the query operation this specifies the length, in
        bytes, of the security descriptor buffer, and upon return contains
        the number of bytes needed to store the descriptor.  If the length
        needed is greater than the length supplied the operation will fail.
        It is ignored in the set and delete operation.

        This parameter is assumed to be captured and probed as appropriate.

    ObjectsSecurityDescriptor - For the Set operation this supplies the address
        of a pointer to the object's current security descriptor.  This routine
        will either modify the security descriptor in place or allocate a new
        security descriptor and use this variable to indicate its new location.
        For the query operation it simply supplies the security descriptor
        being queried.  The caller is responsible for freeing the old security
        descriptor.

    PoolType - For the set operation this specifies the pool type to use if
        a new security descriptor needs to be allocated.  It is ignored
        in the query and delete operation.

        the mapping of generic to specific/standard access types for the object
        being accessed.  This mapping structure is expected to be safe to
        access (i.e., captured if necessary) prior to be passed to this routine.

Return Value:

    NTSTATUS - STATUS_SUCCESS if the operation is successful and an
        appropriate error status otherwise.

--*/

{
    PAGED_CODE();

    //
    // If the object's security descriptor is null, then object is not
    // one that has security information associated with it.  Return
    // an error.
    //

    //
    //  Make sure the common parts of our input are proper
    //

    ASSERT( (OperationCode == SetSecurityDescriptor) ||
            (OperationCode == QuerySecurityDescriptor) ||
            (OperationCode == AssignSecurityDescriptor) ||
            (OperationCode == DeleteSecurityDescriptor) );

    //
    //  This routine simply cases off of the operation code to decide
    //  which support routine to call
    //

    switch (OperationCode) {

        case SetSecurityDescriptor:

        ASSERT( (PoolType == PagedPool) || (PoolType == NonPagedPool) );

        return ObSetSecurityDescriptorInfo( Object,
                                            SecurityInformation,
                                            SecurityDescriptor,
                                            ObjectsSecurityDescriptor,
                                            PoolType,
                                            GenericMapping
                                            );



    case QuerySecurityDescriptor:

        //
        //  check the rest of our input and call the default query security
        //  method
        //

        ASSERT( CapturedLength != NULL );

        return ObQuerySecurityDescriptorInfo( SecurityInformation,
                                              SecurityDescriptor,
                                              CapturedLength,
                                              ObjectsSecurityDescriptor );

    case DeleteSecurityDescriptor:

        //
        //  call the default delete security method
        //

        return SepDefaultDeleteMethod( ObjectsSecurityDescriptor );

    case AssignSecurityDescriptor:

        ObAssignObjectSecurityDescriptor( Object, SecurityDescriptor, PoolType );
        return( STATUS_SUCCESS );

    default:

        //
        //  Bugcheck on any other operation code,  We won't get here if
        //  the earlier asserts are still checked.
        //

        KeBugCheck( SECURITY_SYSTEM );
        return( STATUS_INVALID_PARAMETER );

    }

}




NTSTATUS
SeSetSecurityDescriptorInfo (
    IN PVOID Object OPTIONAL,
    IN PSECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR ModificationDescriptor,
    IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    IN POOL_TYPE PoolType,
    IN PGENERIC_MAPPING GenericMapping
    )

/*++

Routine Description:

    This routine will set an object's security descriptor.  The input
    security descriptor must be previously captured.

Arguments:

    Object - Optionally supplies the object whose security is
        being adjusted.  This is used to update security quota
        information.

    SecurityInformation - Indicates which security information is
        to be applied to the object.  The value(s) to be assigned are
        passed in the SecurityDescriptor parameter.

    ModificationDescriptor - Supplies the input security descriptor to be
        applied to the object.  The caller of this routine is expected
        to probe and capture the passed security descriptor before calling
        and release it after calling.

    ObjectsSecurityDescriptor - Supplies the address of a pointer to
        the objects security descriptor that is going to be altered by
        this procedure.  This structure must be deallocated by the caller.

    PoolType - Specifies the type of pool to allocate for the objects
        security descriptor.

    GenericMapping - This argument provides the mapping of generic to
        specific/standard access types for the object being accessed.
        This mapping structure is expected to be safe to access
        (i.e., captured if necessary) prior to be passed to this routine.

Return Value:

    NTSTATUS - STATUS_SUCCESS if successful and an appropriate error
        value otherwise.

--*/

{



    //
    // Make sure the object already has a security descriptor.
    // Objects that 'may' have security descriptors 'must' have security
    // descriptors.  If this one doesn't already have one, then we can't
    // assign one to it.
    //

    if ((*ObjectsSecurityDescriptor) == NULL) {
        return(STATUS_NO_SECURITY_ON_OBJECT);
    }


    //
    // Pass this call to the common Rtlp routine.
    //

    return RtlpSetSecurityObject (
                    Object,
                    *SecurityInformation,
                    ModificationDescriptor,
                    ObjectsSecurityDescriptor,
                    0,  // No Auto Inheritance
                    PoolType,
                    GenericMapping,
                    NULL ); // No Token


}




NTSTATUS
SeSetSecurityDescriptorInfoEx (
    IN PVOID Object OPTIONAL,
    IN PSECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR ModificationDescriptor,
    IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    IN ULONG AutoInheritFlags,
    IN POOL_TYPE PoolType,
    IN PGENERIC_MAPPING GenericMapping
    )

/*++

Routine Description:

    This routine will set an object's security descriptor.  The input
    security descriptor must be previously captured.

Arguments:

    Object - Optionally supplies the object whose security is
        being adjusted.  This is used to update security quota
        information.

    SecurityInformation - Indicates which security information is
        to be applied to the object.  The value(s) to be assigned are
        passed in the SecurityDescriptor parameter.

    ModificationDescriptor - Supplies the input security descriptor to be
        applied to the object.  The caller of this routine is expected
        to probe and capture the passed security descriptor before calling
        and release it after calling.

    ObjectsSecurityDescriptor - Supplies the address of a pointer to
        the objects security descriptor that is going to be altered by
        this procedure.  This structure must be deallocated by the caller.

    AutoInheritFlags - Controls automatic inheritance of ACES.
        Valid values are a bits mask of the logical OR of
        one or more of the following bits:

        SEF_DACL_AUTO_INHERIT - If set, inherited ACEs from the
            DACL in the ObjectsSecurityDescriptor are preserved and inherited ACEs from
            the ModificationDescriptor are ignored. Inherited ACEs are not supposed
            to be modified; so preserving them across this call is appropriate.
            If a protected server does not itself implement auto inheritance, it should
            not set this bit.  The caller of the protected server may implement
            auto inheritance and my indeed be modifying inherited ACEs.

        SEF_SACL_AUTO_INHERIT - If set, inherited ACEs from the
            SACL in the ObjectsSecurityDescriptor are preserved and inherited ACEs from
            the ModificationDescriptor are ignored. Inherited ACEs are not supposed
            to be modified; so preserving them across this call is appropriate.
            If a protected server does not itself implement auto inheritance, it should
            not set this bit.  The caller of the protected server may implement
            auto inheritance and my indeed be modifying inherited ACEs.

    PoolType - Specifies the type of pool to allocate for the objects
        security descriptor.

    GenericMapping - This argument provides the mapping of generic to
        specific/standard access types for the object being accessed.
        This mapping structure is expected to be safe to access
        (i.e., captured if necessary) prior to be passed to this routine.

Return Value:

    NTSTATUS - STATUS_SUCCESS if successful and an appropriate error
        value otherwise.

--*/

{



    //
    // Make sure the object already has a security descriptor.
    // Objects that 'may' have security descriptors 'must' have security
    // descriptors.  If this one doesn't already have one, then we can't
    // assign one to it.
    //

    if ((*ObjectsSecurityDescriptor) == NULL) {
        return(STATUS_NO_SECURITY_ON_OBJECT);
    }


    //
    // Pass this call to the common Rtlp routine.
    //

    return RtlpSetSecurityObject (
                    Object,
                    *SecurityInformation,
                    ModificationDescriptor,
                    ObjectsSecurityDescriptor,
                    AutoInheritFlags,
                    PoolType,
                    GenericMapping,
                    NULL ); // No Token


}



NTSTATUS
SeQuerySecurityDescriptorInfo (
    IN PSECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN OUT PULONG Length,
    IN PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor
    )

/*++

Routine Description:

    This routine will extract the desired information from the
    passed security descriptor and return the information in
    the passed buffer as a security descriptor in self-relative
    format.

Arguments:

    SecurityInformation - Specifies what information is being queried.

    SecurityDescriptor - Supplies the buffer to output the requested
        information into.

        This buffer has been probed only to the size indicated by
        the Length parameter.  Since it still points into user space,
        it must always be accessed in a try clause.

    Length - Supplies the address of a variable containing the length of
        the security descriptor buffer.  Upon return this variable will
        contain the length needed to store the requested information.

    ObjectsSecurityDescriptor - Supplies the address of a pointer to
        the objects security descriptor.  The passed security descriptor
        must be in self-relative format.

Return Value:

    NTSTATUS - STATUS_SUCCESS if successful and an appropriate error value
        otherwise

--*/

{
    ULONG BufferLength;

    ULONG Size;
    ULONG OwnerLength;
    ULONG GroupLength;
    ULONG DaclLength;
    ULONG SaclLength;
    PUCHAR NextFree;
    SECURITY_DESCRIPTOR IObjectSecurity;

    //
    // Note that IObjectSecurity is not a pointer to a pointer
    // like ObjectsSecurityDescriptor is.
    //

    SECURITY_DESCRIPTOR_RELATIVE *ISecurityDescriptor = SecurityDescriptor;

    PAGED_CODE();

    //
    //  We will be accessing user memory throughout this routine,
    //  therefore do everything in a try-except clause.
    //

    try {

        BufferLength = *Length;

        //
        //  Check if the object's descriptor is null, and if it is then
        //  we only need to return a blank security descriptor record
        //

        if (*ObjectsSecurityDescriptor == NULL) {

            *Length = sizeof(SECURITY_DESCRIPTOR_RELATIVE);

            //
            //  Now make sure it's large enough for the security descriptor
            //  record
            //

            if (BufferLength < sizeof(SECURITY_DESCRIPTOR_RELATIVE)) {

                return STATUS_BUFFER_TOO_SMALL;

            }

            //
            //  It's large enough to make a blank security descriptor record
            //
            //  Note that this parameter has been probed for write by the
            //  object manager, however, we still have to be careful when
            //  writing to it.
            //

            //
            // We do not have to probe this here, because the object
            // manager has probed it for length=BufferLength, which we
            // know at this point is at least as large as a security
            // descriptor.
            //

            RtlCreateSecurityDescriptorRelative( SecurityDescriptor,
                                                 SECURITY_DESCRIPTOR_REVISION );

            //
            // Mark it as self-relative
            //

            RtlpSetControlBits( ISecurityDescriptor, SE_SELF_RELATIVE );

            //
            //  And return to our caller
            //

            return STATUS_SUCCESS;

        }

        //
        // Create an absolute format SD on the stack pointing into
        // user space to simplify the following code
        //

        RtlCopyMemory( (&IObjectSecurity),
                      *ObjectsSecurityDescriptor,
                      sizeof(SECURITY_DESCRIPTOR_RELATIVE) );

        IObjectSecurity.Owner = RtlpOwnerAddrSecurityDescriptor(
                    (SECURITY_DESCRIPTOR *) *ObjectsSecurityDescriptor );
        IObjectSecurity.Group = RtlpGroupAddrSecurityDescriptor(
                    (SECURITY_DESCRIPTOR *) *ObjectsSecurityDescriptor );
        IObjectSecurity.Dacl = RtlpDaclAddrSecurityDescriptor(
                    (SECURITY_DESCRIPTOR *) *ObjectsSecurityDescriptor );
        IObjectSecurity.Sacl = RtlpSaclAddrSecurityDescriptor(
                    (SECURITY_DESCRIPTOR *) *ObjectsSecurityDescriptor );

        IObjectSecurity.Control &= ~SE_SELF_RELATIVE;

        //
        //  This is not a blank descriptor so we need to determine the size
        //  needed to store the requested information.  It is the size of the
        //  descriptor record plus the size of each requested component.
        //

        Size = sizeof(SECURITY_DESCRIPTOR_RELATIVE);

        if ( (((*SecurityInformation) & OWNER_SECURITY_INFORMATION)) &&
             (IObjectSecurity.Owner != NULL) ) {

            OwnerLength = SeLengthSid( IObjectSecurity.Owner );
            Size += (ULONG)LongAlignSize(OwnerLength);

        }

        if ( (((*SecurityInformation) & GROUP_SECURITY_INFORMATION)) &&
             (IObjectSecurity.Group != NULL) ) {

            GroupLength = SeLengthSid( IObjectSecurity.Group );
            Size += (ULONG)LongAlignSize(GroupLength);

        }

        if ( (((*SecurityInformation) & DACL_SECURITY_INFORMATION)) &&
             (IObjectSecurity.Control & SE_DACL_PRESENT) &&
             (IObjectSecurity.Dacl != NULL) ) {


            DaclLength = (ULONG)LongAlignSize((IObjectSecurity.Dacl)->AclSize);
            Size += DaclLength;

        }

        if ( (((*SecurityInformation) & SACL_SECURITY_INFORMATION)) &&
             (IObjectSecurity.Control & SE_SACL_PRESENT) &&
             (IObjectSecurity.Sacl != NULL) ) {

            SaclLength = (ULONG)LongAlignSize((IObjectSecurity.Sacl)->AclSize);
            Size += SaclLength;

        }

        //
        //  Tell the caller how much space this will require
        //  (whether we actually fit or not)
        //

        *Length = Size;

        //
        //  Now make sure the size is less than or equal to the length
        //  we were passed
        //

        if (Size > BufferLength) {

            return STATUS_BUFFER_TOO_SMALL;

        }

        //
        //  The length is fine.
        //
        //  Fill in the length and flags part of the security descriptor.
        //  The real addresses of each acl will be filled in later when we
        //  copy the ACLs over.
        //
        //  Note that we only set a flag in the descriptor if the information
        //  was requested, which is a simple copy of the requested information
        //  input variable
        //
        //  The output buffer has already been probed to the passed size,
        //  so we can just write to it.
        //

        RtlCreateSecurityDescriptorRelative( SecurityDescriptor,
                                             SECURITY_DESCRIPTOR_REVISION );

        //
        // Mark the returned Security Descriptor as being in
        // self-relative format
        //

        RtlpSetControlBits( ISecurityDescriptor, SE_SELF_RELATIVE );

        //
        //  NextFree is used to point to the next free spot in the
        //  returned security descriptor.
        //

        NextFree = LongAlignPtr((PUCHAR)SecurityDescriptor +
                                        sizeof(SECURITY_DESCRIPTOR_RELATIVE));

        //
        //  Copy the Owner SID if necessary and update the NextFree pointer,
        //  keeping it longword aligned.
        //

        if ( ((*SecurityInformation) & OWNER_SECURITY_INFORMATION) &&
             ((IObjectSecurity.Owner) != NULL) ) {

                RtlMoveMemory( NextFree,
                               IObjectSecurity.Owner,
                               OwnerLength );

                ISecurityDescriptor->Owner = (ULONG)((PUCHAR)NextFree - (PUCHAR)SecurityDescriptor);

                RtlpPropagateControlBits(
                    ISecurityDescriptor,
                    &IObjectSecurity,
                    SE_OWNER_DEFAULTED
                    );

                NextFree += (ULONG)LongAlignSize(OwnerLength);

        }


        //
        //  Copy the Group SID if necessary and update the NextFree pointer,
        //  keeping it longword aligned.
        //

        if ( ((*SecurityInformation) & GROUP_SECURITY_INFORMATION) &&
             (IObjectSecurity.Group != NULL) ) {

                RtlMoveMemory( NextFree,
                               IObjectSecurity.Group,
                               GroupLength );

                ISecurityDescriptor->Group = (ULONG)((PUCHAR)NextFree - (PUCHAR)SecurityDescriptor);

                RtlpPropagateControlBits(
                    ISecurityDescriptor,
                    &IObjectSecurity,
                    SE_GROUP_DEFAULTED
                    );

                NextFree += (ULONG)LongAlignSize(GroupLength);

        }


        //
        //  Set discretionary acl information if requested.
        //  If not set in object's security,
        //  then everything is already set properly.
        //

        if ( (*SecurityInformation) & DACL_SECURITY_INFORMATION) {

            RtlpPropagateControlBits(
                ISecurityDescriptor,
                &IObjectSecurity,
                SE_DACL_PRESENT | SE_DACL_DEFAULTED | SE_DACL_PROTECTED | SE_DACL_AUTO_INHERITED
                );

            //
            // Copy the acl if non-null  and update the NextFree pointer,
            // keeping it longword aligned.
            //

            if ( (IObjectSecurity.Control & SE_DACL_PRESENT) != 0 &&
                 IObjectSecurity.Dacl != NULL) {

                RtlMoveMemory( NextFree,
                               IObjectSecurity.Dacl,
                               (IObjectSecurity.Dacl)->AclSize );

                ISecurityDescriptor->Dacl = (ULONG)((PUCHAR)NextFree - (PUCHAR)SecurityDescriptor);

                NextFree += DaclLength;

            }
        }


        //
        //  Set system acl information if requested.
        //  If not set in object's security,
        //  then everything is already set properly.
        //

        if ( (*SecurityInformation) & SACL_SECURITY_INFORMATION) {

            RtlpPropagateControlBits(
                ISecurityDescriptor,
                &IObjectSecurity,
                SE_SACL_PRESENT | SE_SACL_DEFAULTED | SE_SACL_PROTECTED | SE_SACL_AUTO_INHERITED
                );

            //
            // Copy the acl if non-null  and update the NextFree pointer,
            // keeping it longword aligned.
            //
            if ( (IObjectSecurity.Control & SE_SACL_PRESENT) != 0 &&
                 IObjectSecurity.Sacl != NULL) {

                RtlMoveMemory( NextFree,
                               IObjectSecurity.Sacl,
                               (IObjectSecurity.Sacl)->AclSize );

                ISecurityDescriptor->Sacl = (ULONG)((PUCHAR)NextFree - (PUCHAR)SecurityDescriptor);

            }
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        return(GetExceptionCode());
    }

    return STATUS_SUCCESS;

}


NTSTATUS
SepDefaultDeleteMethod (
    IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor
    )

/*++

Routine Description:

    This is a private procedure to delete the security descriptor for
    an object.  It cleans up any pool allocations that have occured
    as part of the descriptor.

Arguments:

    ObjectsSecurityDescriptor - Supplies the address of a pointer
        to the security descriptor being deleted.

Return Value:

    NTSTATUS - STATUS_SUCCESS

--*/

{
    PAGED_CODE();

    return (ObDeassignSecurity ( ObjectsSecurityDescriptor ));
}
