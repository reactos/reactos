/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    rtlassig.c

Abstract:

    This Module implements many security rtl routines defined in ntseapi.h

Author:

    Jim Kelly       (JimK)     23-Mar-1990
    Robert Reichel  (RobertRe)  1-Mar-1991

Environment:

    Pure Runtime Library Routine

Revision History:

--*/


#include "ntrtlp.h"
#include "seopaque.h"
#include "sertlp.h"

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE,RtlSelfRelativeToAbsoluteSD)
#pragma alloc_text(PAGE,RtlMakeSelfRelativeSD)
#pragma alloc_text(PAGE,RtlpQuerySecurityDescriptor)
#pragma alloc_text(PAGE,RtlAbsoluteToSelfRelativeSD)
#pragma alloc_text(PAGE,RtlSelfRelativeToAbsoluteSD2)
#endif


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//    Exported Procedures                                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////



NTSTATUS
RtlSelfRelativeToAbsoluteSD(
    IN OUT PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
    OUT PSECURITY_DESCRIPTOR AbsoluteSecurityDescriptor,
    IN OUT PULONG AbsoluteSecurityDescriptorSize,
    IN OUT PACL Dacl,
    IN OUT PULONG DaclSize,
    IN OUT PACL Sacl,
    IN OUT PULONG SaclSize,
    IN OUT PSID Owner,
    IN OUT PULONG OwnerSize,
    IN OUT PSID PrimaryGroup,
    IN OUT PULONG PrimaryGroupSize
    )

/*++

Routine Description:

    Converts a security descriptor from self-relative format to absolute
    format

Arguments:

    SecurityDescriptor - Supplies a pointer to a security descriptor in
        Self-Relative format

    AbsoluteSecurityDescriptor - A pointer to a buffer in which will be
        placed the main body of the Absolute format security descriptor.

    Dacl - Supplies a pointer to a buffer that will contain the Dacl of the
        output descriptor.  This pointer will be referenced by, not copied
        into, the output descriptor.

    DaclSize - Supplies the size of the buffer pointed to by Dacl.  In case
        of error, it will return the minimum size necessary to contain the
        Dacl.

    Sacl - Supplies a pointer to a buffer that will contain the Sacl of the
        output descriptor.  This pointer will be referenced by, not copied
        into, the output descriptor.

    SaclSize - Supplies the size of the buffer pointed to by Sacl.  In case
        of error, it will return the minimum size necessary to contain the
        Sacl.

    Owner - Supplies a pointer to a buffer that will contain the Owner of
        the output descriptor.  This pointer will be referenced by, not
        copied into, the output descriptor.

    OwnerSize - Supplies the size of the buffer pointed to by Owner.  In
        case of error, it will return the minimum size necessary to contain
        the Owner.

    PrimaryGroup - Supplies a pointer to a buffer that will contain the
        PrimaryGroup of the output descriptor.  This pointer will be
        referenced by, not copied into, the output descriptor.

    PrimaryGroupSize - Supplies the size of the buffer pointed to by
        PrimaryGroup.  In case of error, it will return the minimum size
        necessary to contain the PrimaryGroup.


Return Value:

    STATUS_SUCCESS - Success

    STATUS_BUFFER_TOO_SMALL - One of the buffers passed was too small.

    STATUS_INVALID_OWNER - There was not a valid owner in the passed
        security descriptor.

--*/

{
    ULONG NewDaclSize;
    ULONG NewSaclSize;
    ULONG NewBodySize;
    ULONG NewOwnerSize;
    ULONG NewGroupSize;

    PSID NewOwner;
    PSID NewGroup;
    PACL NewDacl;
    PACL NewSacl;

    //
    // typecast security descriptors so we don't have to cast all over the place.
    //

    PISECURITY_DESCRIPTOR OutSD =
        AbsoluteSecurityDescriptor;

    PISECURITY_DESCRIPTOR InSD =
            (PISECURITY_DESCRIPTOR)SelfRelativeSecurityDescriptor;


    RTL_PAGED_CODE();

    if ( !RtlpAreControlBitsSet( InSD, SE_SELF_RELATIVE) ) {
        return( STATUS_BAD_DESCRIPTOR_FORMAT );
    }

    NewBodySize = sizeof(SECURITY_DESCRIPTOR);

    RtlpQuerySecurityDescriptor(
        InSD,
        &NewOwner,
        &NewOwnerSize,
        &NewGroup,
        &NewGroupSize,
        &NewDacl,
        &NewDaclSize,
        &NewSacl,
        &NewSaclSize
        );

    if ( (NewBodySize  > *AbsoluteSecurityDescriptorSize) ||
         (NewOwnerSize > *OwnerSize )                     ||
         (NewDaclSize  > *DaclSize )                      ||
         (NewSaclSize  > *SaclSize )                      ||
         (NewGroupSize > *PrimaryGroupSize ) ) {

         *AbsoluteSecurityDescriptorSize = sizeof(SECURITY_DESCRIPTOR);
         *PrimaryGroupSize               = NewGroupSize;
         *OwnerSize                      = NewOwnerSize;
         *SaclSize                       = NewSaclSize;
         *DaclSize                       = NewDaclSize;

         return( STATUS_BUFFER_TOO_SMALL );
    }


    RtlMoveMemory( OutSD,
                   InSD,
                   sizeof(SECURITY_DESCRIPTOR_RELATIVE) );

    OutSD->Owner = NULL;
    OutSD->Group = NULL;
    OutSD->Sacl  = NULL;
    OutSD->Dacl  = NULL;

    RtlpClearControlBits( OutSD, SE_SELF_RELATIVE );

    if (NewOwner != NULL) {
        RtlMoveMemory( Owner, NewOwner, SeLengthSid( NewOwner ));
        OutSD->Owner = Owner;
    }

    if (NewGroup != NULL) {
        RtlMoveMemory( PrimaryGroup, NewGroup, SeLengthSid( NewGroup ));
        OutSD->Group = PrimaryGroup;
    }

    if (NewSacl != NULL) {
        RtlMoveMemory( Sacl, NewSacl, NewSacl->AclSize );
        OutSD->Sacl  = Sacl;
    }

    if (NewDacl != NULL) {
        RtlMoveMemory( Dacl, NewDacl, NewDacl->AclSize );
        OutSD->Dacl  = Dacl;
    }

    return( STATUS_SUCCESS );
}




NTSTATUS
RtlMakeSelfRelativeSD(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN OUT PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
    IN OUT PULONG BufferLength
    )

/*++

Routine Description:

    Makes a copy of a security descriptor.  The produced copy will be in self-relative
    form.

    The security descriptor to be copied may be in either absolute or self-relative
    form.

Arguments:

    SecurityDescriptor - Pointer to a security descriptor.  This descriptor will not
        be modified.

    SelfRelativeSecurityDescriptor - Pointer to a buffer that will contain
        the returned self-relative security descriptor.

    BufferLength - Supplies the length of the buffer.  If the supplied
        buffer is not large enough to hold the self-relative security
        descriptor, an error will be returned, and this field will return
        the minimum size required.


Return Value:

    STATUS_BUFFER_TOO_SMALL - The supplied buffer was too small to contain
        the resultant security descriptor.


--*/

{
    ULONG NewDaclSize;
    ULONG NewSaclSize;
    ULONG NewOwnerSize;
    ULONG NewGroupSize;

    ULONG AllocationSize;

    PSID NewOwner;
    PSID NewGroup;
    PACL NewDacl;
    PACL NewSacl;

    PCHAR Field;
    PCHAR Base;


    //
    // Convert security descriptors to new data type so we don't
    // have to cast all over the place.
    //

    PISECURITY_DESCRIPTOR_RELATIVE IResultantDescriptor =
            (PISECURITY_DESCRIPTOR_RELATIVE)SelfRelativeSecurityDescriptor;

    PISECURITY_DESCRIPTOR IPassedSecurityDescriptor =
            (PISECURITY_DESCRIPTOR)SecurityDescriptor;


    RtlpQuerySecurityDescriptor(
        IPassedSecurityDescriptor,
        &NewOwner,
        &NewOwnerSize,
        &NewGroup,
        &NewGroupSize,
        &NewDacl,
        &NewDaclSize,
        &NewSacl,
        &NewSaclSize
        );

    RTL_PAGED_CODE();

    AllocationSize = sizeof(SECURITY_DESCRIPTOR_RELATIVE) +
                     NewOwnerSize +
                     NewGroupSize +
                     NewDaclSize  +
                     NewSaclSize  ;

    if (AllocationSize > *BufferLength) {
        *BufferLength = AllocationSize;
        return( STATUS_BUFFER_TOO_SMALL );
    }

    RtlZeroMemory( IResultantDescriptor, AllocationSize );

    RtlCopyMemory( IResultantDescriptor,
                   IPassedSecurityDescriptor,
                   FIELD_OFFSET( SECURITY_DESCRIPTOR_RELATIVE, Owner ));


    Base = (PCHAR)(IResultantDescriptor);
    Field =  Base + (ULONG)sizeof(SECURITY_DESCRIPTOR_RELATIVE);

    if (NewSaclSize > 0) {
        RtlCopyMemory( Field, NewSacl, NewSaclSize );
        IResultantDescriptor->Sacl = RtlPointerToOffset(Base,Field);
        Field += NewSaclSize;
    } else {
        IResultantDescriptor->Sacl = 0;
    }


    if (NewDaclSize > 0) {
        RtlCopyMemory( Field, NewDacl, NewDaclSize );
        IResultantDescriptor->Dacl = RtlPointerToOffset(Base,Field);
        Field += NewDaclSize;
    } else {
        IResultantDescriptor->Dacl = 0;
    }



    if (NewOwnerSize > 0) {
        RtlCopyMemory( Field, NewOwner, NewOwnerSize );
        IResultantDescriptor->Owner = RtlPointerToOffset(Base,Field);
        Field += NewOwnerSize;
    }


    if (NewGroupSize > 0) {
        RtlCopyMemory( Field, NewGroup, NewGroupSize );
        IResultantDescriptor->Group = RtlPointerToOffset(Base,Field);
    }

    RtlpSetControlBits( IResultantDescriptor, SE_SELF_RELATIVE );

    return( STATUS_SUCCESS );

}


NTSTATUS
RtlAbsoluteToSelfRelativeSD(
    IN PSECURITY_DESCRIPTOR AbsoluteSecurityDescriptor,
    IN OUT PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
    IN OUT PULONG BufferLength
    )

/*++

Routine Description:

    Converts a security descriptor in absolute form to one in self-relative
    form.

Arguments:

    AbsoluteSecurityDescriptor - Pointer to an absolute format security
        descriptor.  This descriptor will not be modified.

    SelfRelativeSecurityDescriptor - Pointer to a buffer that will contain
        the returned self-relative security descriptor.

    BufferLength - Supplies the length of the buffer.  If the supplied
        buffer is not large enough to hold the self-relative security
        descriptor, an error will be returned, and this field will return
        the minimum size required.


Return Value:

    STATUS_BUFFER_TOO_SMALL - The supplied buffer was too small to contain
        the resultant security descriptor.

    STATUS_BAD_DESCRIPTOR_FORMAT - The supplied security descriptor was not
        in absolute form.

--*/

{
    NTSTATUS NtStatus;

    PISECURITY_DESCRIPTOR IAbsoluteSecurityDescriptor =
            (PISECURITY_DESCRIPTOR)AbsoluteSecurityDescriptor;


    RTL_PAGED_CODE();

    //
    // Make sure the passed SD is absolute format, and then call
    // RtlMakeSelfRelativeSD() to do all the work.
    //

    if ( RtlpAreControlBitsSet( IAbsoluteSecurityDescriptor, SE_SELF_RELATIVE) ) {
        return( STATUS_BAD_DESCRIPTOR_FORMAT );
    }

    NtStatus = RtlMakeSelfRelativeSD(
                   AbsoluteSecurityDescriptor,
                   SelfRelativeSecurityDescriptor,
                   BufferLength
                   );

    return( NtStatus );

}
VOID
RtlpQuerySecurityDescriptor(
    IN PISECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PSID *Owner,
    OUT PULONG OwnerSize,
    OUT PSID *PrimaryGroup,
    OUT PULONG PrimaryGroupSize,
    OUT PACL *Dacl,
    OUT PULONG DaclSize,
    OUT PACL *Sacl,
    OUT PULONG SaclSize
    )
/*++

Routine Description:

    Returns the pieces of a security descriptor structure.

Arguments:


    SecurityDescriptor - Provides the security descriptor of interest.

    Owner - Returns a pointer to the owner information contained in the
        security descriptor.

    OwnerSize - Returns the size of the owner information.

    PrimaryGroup -  Returns a pointer to the primary group information.

    PrimaryGroupSize - Returns the size of the primary group information.

    Dacl - Returns a pointer to the Dacl.

    DaclSize - Returns the size of the Dacl.

    Sacl - Returns a pointer to the Sacl.

    SaclSize - Returns the size of the Sacl.

Return Value:

    None.

--*/
{

    RTL_PAGED_CODE();

    *Owner = RtlpOwnerAddrSecurityDescriptor( SecurityDescriptor );

    if (*Owner != NULL) {
        *OwnerSize = LongAlignSize(SeLengthSid(*Owner));
    } else {
        *OwnerSize = 0;
    }

    *Dacl = RtlpDaclAddrSecurityDescriptor ( SecurityDescriptor );

    if (*Dacl !=NULL) {
        *DaclSize = LongAlignSize((*Dacl)->AclSize);
    } else {
        *DaclSize = 0;
    }

    *PrimaryGroup = RtlpGroupAddrSecurityDescriptor( SecurityDescriptor );

    if (*PrimaryGroup != NULL) {
        *PrimaryGroupSize = LongAlignSize(SeLengthSid(*PrimaryGroup));
    } else {
         *PrimaryGroupSize = 0;
    }

    *Sacl = RtlpSaclAddrSecurityDescriptor( SecurityDescriptor );

    if (*Sacl != NULL) {
        *SaclSize = LongAlignSize((*Sacl)->AclSize);
    } else {
        *SaclSize = 0;
    }

}



NTSTATUS
RtlSelfRelativeToAbsoluteSD2(
    IN OUT PSECURITY_DESCRIPTOR pSelfRelativeSecurityDescriptor,
    IN OUT PULONG               pBufferSize
    )

/*++

Routine Description:

    Converts a security descriptor from self-relative format to absolute
    format using the memory allocated for the SelfRelativeSecurityDescriptor

Arguments:

    pSecurityDescriptor - Supplies a pointer to a security descriptor in
        Self-Relative format. If success, we return a absolute security
        descriptor where this pointer pointings.

    pBufferSize - Supplies a pointer to the size of the
        buffer.

Return Value:

    STATUS_SUCCESS - Success

    STATUS_BAD_DESCRIPTOR_FORMAT - The passed descriptor is not a self-relative
       security descriptor.

    STATUS_BUFFER_TOO_SMALL - The passed buffer is too small.

    STATUS_INVALID_OWNER - There was not a valid owner in the passed
        security descriptor.

Notes: Despite some attempts to make this code as portable as possible and the 
       utilization of C_ASSERT or ASSERT to detect the respect of these assumptions, 
       this code is still making several assumptions about the format of the absolute 
       and self-relative descriptors and their relationships: in terms of packing, 
       fields definitions and locations in their respective structures. 
       In particular, this code assumes that the only differences are due to differences 
       in the types of the structure members and in the behaviour of the security descriptor
       query API.
       At this time, the only structure members that get read/updated are Owner, Group,
       Dacl and Sacl. If more members are added or displaced in the definitions of these
       structures, this code may have to be modified.

--*/

{
    ULONG_PTR   ptr;
    PSID        owner;
    PSID        group;
    PACL        dacl;
    PACL        sacl;
    ULONG       daclSize;
    ULONG       saclSize;
    ULONG       newBodySize;
    ULONG       ownerSize;
    ULONG       groupSize;
    ULONG       newBufferSize;
    LONG        deltaSize;

//
// Typecast security descriptors so we don't have to cast all over the place.
//

    PISECURITY_DESCRIPTOR          psd  = (PISECURITY_DESCRIPTOR)         pSelfRelativeSecurityDescriptor;
    PISECURITY_DESCRIPTOR_RELATIVE psdr = (PISECURITY_DESCRIPTOR_RELATIVE)pSelfRelativeSecurityDescriptor;

//
// This code uses several assumptions about the absolute and self-relative formats of 
// security descriptors and the way they are packing in memory. 
// See Routine Description Notes.
//

    C_ASSERT( sizeof( SECURITY_DESCRIPTOR ) >= sizeof( SECURITY_DESCRIPTOR_RELATIVE ) ); 
    C_ASSERT( sizeof( psd->Control ) == sizeof( psdr->Control ) );
    C_ASSERT( FIELD_OFFSET( SECURITY_DESCRIPTOR, Control ) == FIELD_OFFSET( SECURITY_DESCRIPTOR_RELATIVE, Control ) );
    
    RTL_PAGED_CODE();

//
// Parameters check point
//

    if ( psd == (PISECURITY_DESCRIPTOR)0 ) {
        return( STATUS_INVALID_PARAMETER_1 );        
    }
    if ( pBufferSize == (PULONG)0 )   {
        return( STATUS_INVALID_PARAMETER_2 );       
    }

    //
    // If the passed security descriptor is not self-relative, we return
    // an format error.
    //

    if ( !RtlpAreControlBitsSet( psd, SE_SELF_RELATIVE) ) {
        return( STATUS_BAD_DESCRIPTOR_FORMAT );
    }

//
// Update local variables by querying the self-relative descriptor.
//
// Note that the returned size values are long-aligned.
//

    RtlpQuerySecurityDescriptor(
        psd,
        &owner,
        &ownerSize,
        &group,
        &groupSize,
        &dacl,
        &daclSize,
        &sacl,
        &saclSize
        );

//
// Identical formats check:
//

    //
    // Determine the delta in size between the two formats of security descriptors
    //

    deltaSize = sizeof( SECURITY_DESCRIPTOR ) - sizeof( SECURITY_DESCRIPTOR_RELATIVE ); 

    //
    // If identical format: 
    //      - clear the SELF_RELATIVE flag
    //      - update absolute descriptor members
    //      - return SUCCESS.
    //

    if ( deltaSize == 0 )   {
       
        RtlpClearControlBits( psd, SE_SELF_RELATIVE );

        //
        // Only the following fields are updated.
        //

        ASSERT( sizeof( psd->Owner ) == sizeof( psdr->Owner ) );
        ASSERT( sizeof( psd->Group ) == sizeof( psdr->Group ) );
        ASSERT( sizeof( psd->Sacl  ) == sizeof( psdr->Sacl  ) );
        ASSERT( sizeof( psd->Dacl  ) == sizeof( psdr->Dacl  ) );

        psd->Owner = owner;
        psd->Group = group;
        psd->Sacl  = sacl;
        psd->Dacl  = dacl;
    
        return( STATUS_SUCCESS );

    }

//
// Determine the required size for the absolute format:
//

#define ULONG_PTR_SDEND( _Adr ) ( (ULONG_PTR)(_Adr) + (ULONG_PTR)(_Adr##Size) )

    ptr = owner > group ? ULONG_PTR_SDEND( owner ) : ULONG_PTR_SDEND( group );
    ptr = ptr > (ULONG_PTR)dacl ? ptr : ULONG_PTR_SDEND( dacl );
    ptr = ptr > (ULONG_PTR)sacl ? ptr : ULONG_PTR_SDEND( sacl );
   
    newBufferSize = sizeof( SECURITY_DESCRIPTOR );
    if ( ptr )   {

#define ULONG_ROUND_UP( x, y )   ((ULONG)(x) + ((y)-1) & ~((y)-1))

        newBufferSize += ULONG_ROUND_UP( (ULONG_PTR)ptr - (ULONG_PTR)(psdr + 1), sizeof(PVOID) );
    }

    //
    // If the specified buffer size is not big enough, let the caller know abour 
    // the minimum size and return STATUS_BUFFER_TOO_SMALL.
    //

    if ( newBufferSize > *pBufferSize )  {
        *pBufferSize = newBufferSize;
        return( STATUS_BUFFER_TOO_SMALL );
    }

//
// Update absolute security descriptor:
//

    //
    // Move the members of self-relative security descriptor in their 
    // absolute format locations.
    //

    if ( ptr )   {
       RtlMoveMemory( (PVOID)(psd + 1), (PVOID)(psdr + 1), newBufferSize - sizeof( SECURITY_DESCRIPTOR) );      
    }

    //
    // Clear the self-relative flag
    //

    RtlpClearControlBits( psd, SE_SELF_RELATIVE );

    //
    // Only the following fields are updated.
    //

    psd->Owner = (PSID)( owner ? (ULONG_PTR)owner + deltaSize : 0 );
    psd->Group = (PSID)( group ? (ULONG_PTR)group + deltaSize : 0 );
    psd->Sacl  = (PACL)( sacl  ? (ULONG_PTR)sacl  + deltaSize : 0 );
    psd->Dacl  = (PACL)( dacl  ? (ULONG_PTR)dacl  + deltaSize : 0 );
    
    return( STATUS_SUCCESS );

} // RtlSelfRelativeToAbsoluteSD2()

