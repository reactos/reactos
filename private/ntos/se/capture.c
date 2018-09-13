/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Capture.c

Abstract:

    This Module implements the security data structure capturing routines.
    There are corresponding Release routines for the data structures that
    are captured into allocated pool.

Author:

    Gary Kimura     (GaryKi)    9-Nov-1989
    Jim Kelly       (JimK)      1-Feb-1990

Environment:

    Kernel Mode

Revision History:

--*/

#include "sep.h"
#include <sertlp.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,SeCaptureSecurityDescriptor)
#pragma alloc_text(PAGE,SeReleaseSecurityDescriptor)
#pragma alloc_text(PAGE,SeCaptureSecurityQos)
#pragma alloc_text(PAGE,SeCaptureSid)
#pragma alloc_text(PAGE,SeReleaseSid)
#pragma alloc_text(PAGE,SeCaptureAcl)
#pragma alloc_text(PAGE,SeReleaseAcl)
#pragma alloc_text(PAGE,SeCaptureLuidAndAttributesArray)
#pragma alloc_text(PAGE,SeReleaseLuidAndAttributesArray)
#pragma alloc_text(PAGE,SeCaptureSidAndAttributesArray)
#pragma alloc_text(PAGE,SeReleaseSidAndAttributesArray)
#pragma alloc_text(PAGE,SeComputeQuotaInformationSize)
#pragma alloc_text(PAGE,SepCopyProxyData)
#pragma alloc_text(PAGE,SepProbeAndCaptureQosData)
#pragma alloc_text(PAGE,SepFreeProxyData)
#endif

#define LongAligned( ptr )  (LongAlignPtr(ptr) == (ptr))


NTSTATUS
SeCaptureSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR InputSecurityDescriptor,
    IN KPROCESSOR_MODE RequestorMode,
    IN POOL_TYPE PoolType,
    IN BOOLEAN ForceCapture,
    OUT PSECURITY_DESCRIPTOR *OutputSecurityDescriptor
    )

/*++

Routine Description:

    This routine probes and captures a copy of the security descriptor based
    upon the following tests.

    if the requestor mode is not kernel mode then

        probe and capture the input descriptor
        (the captured descriptor is self-relative)

    if the requstor mode is kernel mode then

        if force capture is true then

            do not probe the input descriptor, but do capture it.
            (the captured descriptor is self-relative)

        else

            do nothing
            (the input descriptor is expected to be self-relative)

Arguments:

    InputSecurityDescriptor - Supplies the security descriptor to capture.
    This parameter is assumed to have been provided by the mode specified
    in RequestorMode.

    RequestorMode - Specifies the caller's access mode.

    PoolType - Specifies which pool type to allocate the captured
        descriptor from

    ForceCapture - Specifies whether the input descriptor should always be
        captured

    OutputSecurityDescriptor - Supplies the address of a pointer to the
        output security descriptor.  The captured descriptor will be
        self-relative format.

Return Value:

    STATUS_SUCCESS if the operation is successful.

    STATUS_INVALID_SID - An SID within the security descriptor is not
        a valid SID.

    STATUS_INVALID_ACL - An ACL within the security descriptor is not
        a valid ACL.

    STATUS_UNKNOWN_REVISION - The revision level of the security descriptor
        is not one known to this revision of the capture routine.
--*/

{
    SECURITY_DESCRIPTOR Captured;
    SECURITY_DESCRIPTOR_RELATIVE *PIOutputSecurityDescriptor;
    PCHAR DescriptorOffset;

    ULONG SaclSize;
    ULONG NewSaclSize;

    ULONG DaclSize;
    ULONG NewDaclSize;

    ULONG OwnerSubAuthorityCount;
    ULONG OwnerSize;
    ULONG NewOwnerSize;

    ULONG GroupSubAuthorityCount;
    ULONG GroupSize;
    ULONG NewGroupSize;

    ULONG Size;

    PAGED_CODE();

    //
    //  if the security descriptor is null then there is really nothing to
    //  capture
    //

    if (InputSecurityDescriptor == NULL) {

        (*OutputSecurityDescriptor) = NULL;

        return STATUS_SUCCESS;

    }

    //
    //  check if the requestors mode is kernel mode and we are not
    //  to force a capture
    //

    if ((RequestorMode == KernelMode) && (ForceCapture == FALSE)) {

        //
        //  Yes it is so we don't need to do any work and can simply
        //  return a pointer to the input descriptor
        //

        (*OutputSecurityDescriptor) = InputSecurityDescriptor;

        return STATUS_SUCCESS;

    }


    //
    //  We need to probe and capture the descriptor.
    //  To do this we need to probe the main security descriptor record
    //  first.
    //

    if (RequestorMode != KernelMode) {

        //
        // Capture of UserMode SecurityDescriptor.
        //

        try {

            //
            // Probe the main record of the input SecurityDescriptor
            //

            ProbeForRead( InputSecurityDescriptor,
                          sizeof(SECURITY_DESCRIPTOR_RELATIVE),
                          sizeof(ULONG) );

            //
            //  Capture the SecurityDescriptor main record.
            //

            RtlCopyMemory( (&Captured),
                          InputSecurityDescriptor,
                          sizeof(SECURITY_DESCRIPTOR_RELATIVE) );

            //
            // Verify the alignment is correct for absolute case. This is
            // only needed when pointer are 64 bits.
            //

            if (!(Captured.Control & SE_SELF_RELATIVE)) {

               if ((ULONG_PTR) InputSecurityDescriptor & (sizeof(ULONG_PTR) - 1)) {
                   ExRaiseDatatypeMisalignment();
               }
            }


        } except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

    } else {

        //
        //  Force capture of kernel mode SecurityDescriptor.
        //
        //  Capture the SecurityDescriptor main record.
        //  It doesn't need probing because requestor mode is kernel.
        //

        RtlCopyMemory( (&Captured),
                      InputSecurityDescriptor,
                      sizeof(SECURITY_DESCRIPTOR_RELATIVE) );

    }

    //
    // Make sure it is a revision we recognize
    //

    if (Captured.Revision != SECURITY_DESCRIPTOR_REVISION) {
       return STATUS_UNKNOWN_REVISION;
    }


    //
    // In case the input security descriptor is self-relative, change the
    // captured main record to appear as an absolute form so we can use
    // common code for both cases below.
    //
    // Note that the fields of Captured are left pointing to user
    // space addresses.  Treat them carefully.
    //

    try {

        Captured.Owner = RtlpOwnerAddrSecurityDescriptor(
            (SECURITY_DESCRIPTOR *)InputSecurityDescriptor
            );
        Captured.Group = RtlpGroupAddrSecurityDescriptor(
            (SECURITY_DESCRIPTOR *)InputSecurityDescriptor
            );
        Captured.Sacl  = RtlpSaclAddrSecurityDescriptor (
            (SECURITY_DESCRIPTOR *)InputSecurityDescriptor
            );
        Captured.Dacl  = RtlpDaclAddrSecurityDescriptor (
            (SECURITY_DESCRIPTOR *)InputSecurityDescriptor
            );
        Captured.Control &= ~SE_SELF_RELATIVE;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }



    //
    //  Indicate the size we are going to need to allocate for the captured
    //  acls
    //

    SaclSize = 0;
    DaclSize = 0;

    NewSaclSize = 0;
    NewDaclSize = 0;
    NewGroupSize = 0;
    NewOwnerSize = 0;

    //
    //  Probe (if necessary) and capture each of the components of a
    //  SECURITY_DESCRIPTOR.
    //

    //
    //  System ACL first
    //

    if ((Captured.Control & SE_SACL_PRESENT) &&
        (Captured.Sacl != NULL) ) {

        if (RequestorMode != KernelMode) {

            try {
                SaclSize = ProbeAndReadUshort( &(Captured.Sacl->AclSize) );
                ProbeForRead( Captured.Sacl,
                              SaclSize,
                              sizeof(ULONG) );
            } except(EXCEPTION_EXECUTE_HANDLER) {
                return GetExceptionCode();
            }

        } else {

            SaclSize = Captured.Sacl->AclSize;

        }

        NewSaclSize = (ULONG)LongAlignSize( SaclSize );

    } else {
        //
        // Force the SACL to null if the bit is off
        //
        Captured.Sacl = NULL;
    }

    //
    //  Discretionary ACL
    //

    if ((Captured.Control & SE_DACL_PRESENT) &&
        (Captured.Dacl != NULL) ) {

        if (RequestorMode != KernelMode) {

            try {
                DaclSize = ProbeAndReadUshort( &(Captured.Dacl->AclSize) );
                ProbeForRead( Captured.Dacl,
                              DaclSize,
                              sizeof(ULONG) );
            } except(EXCEPTION_EXECUTE_HANDLER) {
                return GetExceptionCode();
            }

        } else {

            DaclSize = Captured.Dacl->AclSize;

        }

        NewDaclSize = (ULONG)LongAlignSize( DaclSize );

    } else {
        //
        // Force the DACL to null if it is not present
        //
        Captured.Dacl = NULL;
    }

    //
    //  Owner SID
    //

    if (Captured.Owner != NULL)  {

        if (RequestorMode != KernelMode) {

            try {
                OwnerSubAuthorityCount =
                    ProbeAndReadUchar( &(((SID *)(Captured.Owner))->SubAuthorityCount) );
                OwnerSize = RtlLengthRequiredSid( OwnerSubAuthorityCount );
                ProbeForRead( Captured.Owner,
                              OwnerSize,
                              sizeof(ULONG) );
            } except(EXCEPTION_EXECUTE_HANDLER) {
                return GetExceptionCode();
            }

        } else {

            OwnerSubAuthorityCount = ((SID *)(Captured.Owner))->SubAuthorityCount;
            OwnerSize = RtlLengthRequiredSid( OwnerSubAuthorityCount );

        }

        NewOwnerSize = (ULONG)LongAlignSize( OwnerSize );

    }

    //
    //  Group SID
    //

    if (Captured.Group != NULL)  {

        if (RequestorMode != KernelMode) {

            try {
                GroupSubAuthorityCount =
                    ProbeAndReadUchar( &(((SID *)(Captured.Group))->SubAuthorityCount) );
                GroupSize = RtlLengthRequiredSid( GroupSubAuthorityCount );
                ProbeForRead( Captured.Group,
                              GroupSize,
                              sizeof(ULONG) );
            } except(EXCEPTION_EXECUTE_HANDLER) {
                return GetExceptionCode();
            }

        } else {

            GroupSubAuthorityCount = ((SID *)(Captured.Group))->SubAuthorityCount;
            GroupSize = RtlLengthRequiredSid( GroupSubAuthorityCount );

        }

        NewGroupSize = (ULONG)LongAlignSize( GroupSize );

    }



    //
    //  Now allocate enough pool to hold the descriptor
    //

    Size = sizeof(SECURITY_DESCRIPTOR_RELATIVE) +
           NewSaclSize +
           NewDaclSize +
           NewOwnerSize +
           NewGroupSize;

    (PIOutputSecurityDescriptor) = (SECURITY_DESCRIPTOR_RELATIVE *)ExAllocatePoolWithTag( PoolType,
                                                                                 Size,
                                                                                 'cSeS' );

    if ( PIOutputSecurityDescriptor == NULL ) {
        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    (*OutputSecurityDescriptor) = (PSECURITY_DESCRIPTOR)PIOutputSecurityDescriptor;
    DescriptorOffset = (PCHAR)(PIOutputSecurityDescriptor);


    //
    //  Copy the main security descriptor record over
    //

    RtlCopyMemory( DescriptorOffset,
                  &Captured,
                  sizeof(SECURITY_DESCRIPTOR_RELATIVE) );
    DescriptorOffset += sizeof(SECURITY_DESCRIPTOR_RELATIVE);

    //
    // Indicate the output descriptor is self-relative
    //

    PIOutputSecurityDescriptor->Control |= SE_SELF_RELATIVE;

    //
    //  If there is a System Acl, copy it over and set
    //  the output descriptor's offset to point to the newly captured copy.
    //

    if ((Captured.Control & SE_SACL_PRESENT) && (Captured.Sacl != NULL)) {


        try {
            RtlCopyMemory( DescriptorOffset,
                          Captured.Sacl,
                          SaclSize );


        } except(EXCEPTION_EXECUTE_HANDLER) {
            ExFreePool( PIOutputSecurityDescriptor );
            return GetExceptionCode();
        }

        if ((RequestorMode != KernelMode) &&
            (!SepCheckAcl( (PACL) DescriptorOffset, SaclSize )) ) {

            ExFreePool( PIOutputSecurityDescriptor );
            return STATUS_INVALID_ACL;
        }

        //
        // Change pointer to offset
        //

        PIOutputSecurityDescriptor->Sacl =
            RtlPointerToOffset( PIOutputSecurityDescriptor,
                                DescriptorOffset,
                                );

        ((PACL) DescriptorOffset)->AclSize = (USHORT) NewSaclSize;
        DescriptorOffset += NewSaclSize;
    } else {
        PIOutputSecurityDescriptor->Sacl = 0;
    }

    //
    //  If there is a Discretionary Acl, copy it over and set
    //  the output descriptor's offset to point to the newly captured copy.
    //

    if ((Captured.Control & SE_DACL_PRESENT) && (Captured.Dacl != NULL)) {


        try {
            RtlCopyMemory( DescriptorOffset,
                          Captured.Dacl,
                          DaclSize );
        } except(EXCEPTION_EXECUTE_HANDLER) {
            ExFreePool( PIOutputSecurityDescriptor );
            return GetExceptionCode();
        }

        if ((RequestorMode != KernelMode) &&
            (!SepCheckAcl( (PACL) DescriptorOffset, DaclSize )) ) {

            ExFreePool( PIOutputSecurityDescriptor );
            return STATUS_INVALID_ACL;
        }

        //
        // Change pointer to offset
        //

        PIOutputSecurityDescriptor->Dacl =
                   RtlPointerToOffset(
                        PIOutputSecurityDescriptor,
                        DescriptorOffset
                        );

        ((PACL) DescriptorOffset)->AclSize = (USHORT) NewDaclSize;
        DescriptorOffset += NewDaclSize;
    } else {
        PIOutputSecurityDescriptor->Dacl = 0;
    }

    //
    //  If there is an Owner SID, copy it over and set
    //  the output descriptor's offset to point to the newly captured copy.
    //

    if (Captured.Owner != NULL) {


        try {
            RtlCopyMemory( DescriptorOffset,
                          Captured.Owner,
                          OwnerSize );
            ((SID *) (DescriptorOffset))->SubAuthorityCount = (UCHAR) OwnerSubAuthorityCount;

        } except(EXCEPTION_EXECUTE_HANDLER) {
            ExFreePool( PIOutputSecurityDescriptor );
            return GetExceptionCode();
        }

        if ((RequestorMode != KernelMode) &&
            (!RtlValidSid( (PSID) DescriptorOffset )) ) {

            ExFreePool( PIOutputSecurityDescriptor );
            return STATUS_INVALID_SID;
        }

        //
        // Change pointer to offset
        //

        PIOutputSecurityDescriptor->Owner =
                    RtlPointerToOffset(
                        PIOutputSecurityDescriptor,
                        DescriptorOffset
                        );

        DescriptorOffset += NewOwnerSize;

    } else {
        PIOutputSecurityDescriptor->Owner = 0;
    }

    //
    //  If there is a group SID, copy it over and set
    //  the output descriptor's offset to point to the newly captured copy.
    //

    if (Captured.Group != NULL) {


        try {
            RtlCopyMemory( DescriptorOffset,
                          Captured.Group,
                          GroupSize );

            ((SID *) DescriptorOffset)->SubAuthorityCount = (UCHAR) GroupSubAuthorityCount;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            ExFreePool( PIOutputSecurityDescriptor );
            return GetExceptionCode();
        }

        if ((RequestorMode != KernelMode) &&
            (!RtlValidSid( (PSID) DescriptorOffset )) ) {

            ExFreePool( PIOutputSecurityDescriptor );
            return STATUS_INVALID_SID;
        }

        //
        // Change pointer to offset
        //

        PIOutputSecurityDescriptor->Group =
                    RtlPointerToOffset(
                        PIOutputSecurityDescriptor,
                        DescriptorOffset
                        );

        DescriptorOffset += NewGroupSize;
    } else {
        PIOutputSecurityDescriptor->Group = 0;
    }

    //
    //  And return to our caller
    //

    return STATUS_SUCCESS;

}


VOID
SeReleaseSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR CapturedSecurityDescriptor,
    IN KPROCESSOR_MODE RequestorMode,
    IN BOOLEAN ForceCapture
    )

/*++

Routine Description:

    This routine releases a previously captured security descriptor.
    Only

Arguments:

    CapturedSecurityDescriptor - Supplies the security descriptor to release.

    RequestorMode - The processor mode specified when the descriptor was
        captured.

    ForceCapture - The ForceCapture value specified when the descriptor was
        captured.

Return Value:

    None.

--*/

{
    //
    // We only have something to deallocate if the requestor was user
    // mode or kernel mode requesting ForceCapture.
    //

    PAGED_CODE();

    if ( ((RequestorMode == KernelMode) && (ForceCapture == TRUE)) ||
          (RequestorMode == UserMode ) ) {
        if ( CapturedSecurityDescriptor ) {
            ExFreePool(CapturedSecurityDescriptor);
            }
    }

    return;

}


NTSTATUS
SepCopyProxyData (
    OUT PSECURITY_TOKEN_PROXY_DATA * DestProxyData,
    IN PSECURITY_TOKEN_PROXY_DATA SourceProxyData
    )

/*++

Routine Description:

    This routine copies a token proxy data structure from one token to another.

Arguments:

    DestProxyData - Receives a pointer to a new proxy data structure.

    SourceProxyData - Supplies a pointer to an already existing proxy data structure.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES on failure.

--*/

{

    PAGED_CODE();

    *DestProxyData = ExAllocatePoolWithTag( PagedPool, sizeof( SECURITY_TOKEN_PROXY_DATA ), 'dPoT' );

    if (*DestProxyData == NULL) {
        return( STATUS_INSUFFICIENT_RESOURCES );
    }



    (*DestProxyData)->PathInfo.Buffer = ExAllocatePoolWithTag( PagedPool, SourceProxyData->PathInfo.Length, 'dPoT' );

    if ((*DestProxyData)->PathInfo.Buffer == NULL) {
        ExFreePool( *DestProxyData );
        *DestProxyData = NULL;
        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    (*DestProxyData)->Length = SourceProxyData->Length;
    (*DestProxyData)->ProxyClass = SourceProxyData->ProxyClass;
    (*DestProxyData)->PathInfo.MaximumLength =
        (*DestProxyData)->PathInfo.Length = SourceProxyData->PathInfo.Length;
    (*DestProxyData)->ContainerMask = SourceProxyData->ContainerMask;
    (*DestProxyData)->ObjectMask = SourceProxyData->ObjectMask;

    RtlCopyUnicodeString( &(*DestProxyData)->PathInfo, &SourceProxyData->PathInfo );

    return( STATUS_SUCCESS );
}

VOID
SepFreeProxyData (
    IN PSECURITY_TOKEN_PROXY_DATA ProxyData
    )

/*++

Routine Description:

    This routine frees a SECURITY_TOKEN_PROXY_DATA structure and all sub structures.

Arguments:

    ProxyData - Supplies a pointer to an existing proxy data structure.

Return Value:

    None.

--*/
{
    PAGED_CODE();

    if (ProxyData != NULL) {

        if (ProxyData->PathInfo.Buffer != NULL) {
            ExFreePool( ProxyData->PathInfo.Buffer );
        }

        ExFreePool( ProxyData );
    }
}




NTSTATUS
SepProbeAndCaptureQosData(
    IN PSECURITY_ADVANCED_QUALITY_OF_SERVICE CapturedSecurityQos
    )

/*++

Routine Description:

    This routine probes and captures the imbedded structures in a
    Security Quality of Service structure.

    This routine assumes that it is being called under an existing
    try-except clause.

Arguments:

    CapturedSecurityQos - Points to the captured body of a QOS
        structure.  The pointers in this structure are presumed
        not to be probed or captured at this point.

Return Value:

    STATUS_SUCCESS indicates no exceptions were encountered.

    Any access violations encountered will be returned.

--*/
{
    NTSTATUS Status;
    PSECURITY_TOKEN_PROXY_DATA CapturedProxyData;
    PSECURITY_TOKEN_AUDIT_DATA CapturedAuditData;
    SECURITY_TOKEN_PROXY_DATA StackProxyData;
    PAGED_CODE();

    CapturedProxyData = CapturedSecurityQos->ProxyData;
    CapturedSecurityQos->ProxyData = NULL;
    CapturedAuditData = CapturedSecurityQos->AuditData;
    CapturedSecurityQos->AuditData = NULL;

    if (ARGUMENT_PRESENT( CapturedProxyData )) {

        //
        // Make sure the body of the proxy data is ok to read.
        //

        ProbeForRead(
            CapturedProxyData,
            sizeof(SECURITY_TOKEN_PROXY_DATA),
            sizeof(ULONG)
            );

        StackProxyData = *CapturedProxyData;

        if (StackProxyData.Length != sizeof( SECURITY_TOKEN_PROXY_DATA )) {
            return( STATUS_INVALID_PARAMETER );
        }


        //
        // Probe the passed pathinfo buffer
        //

        ProbeForRead(
            StackProxyData.PathInfo.Buffer,
            StackProxyData.PathInfo.Length,
            sizeof( UCHAR )
            );

        Status = SepCopyProxyData( &CapturedSecurityQos->ProxyData, &StackProxyData );

        if (!NT_SUCCESS(Status)) {

            if (CapturedSecurityQos->ProxyData != NULL) {
                SepFreeProxyData( CapturedSecurityQos->ProxyData );
                CapturedSecurityQos->ProxyData = NULL;
            }

            return( Status );
        }

    }

    if (ARGUMENT_PRESENT( CapturedAuditData )) {

        PSECURITY_TOKEN_AUDIT_DATA LocalAuditData;

        //
        // Probe the audit data structure and make sure it looks ok
        //

        ProbeForRead(
            CapturedAuditData,
            sizeof( SECURITY_TOKEN_AUDIT_DATA ),
            sizeof( ULONG )
            );


        LocalAuditData = ExAllocatePool( PagedPool, sizeof( SECURITY_TOKEN_AUDIT_DATA ));

        if (LocalAuditData == NULL) {

            //
            // Cleanup any proxy data we may have allocated.
            //

            SepFreeProxyData( CapturedSecurityQos->ProxyData );
            CapturedSecurityQos->ProxyData = NULL;

            return( STATUS_INSUFFICIENT_RESOURCES );

        }

        //
        // Copy the data to the local buffer. Note: we do this in this
        // order so that if the final assignment fails the caller will
        // still be able to free the allocated pool.
        //

        CapturedSecurityQos->AuditData = LocalAuditData;

        *CapturedSecurityQos->AuditData = *CapturedAuditData;

        if ( LocalAuditData->Length != sizeof( SECURITY_TOKEN_AUDIT_DATA ) ) {
            SepFreeProxyData( CapturedSecurityQos->ProxyData );
            CapturedSecurityQos->ProxyData = NULL;
            ExFreePool(CapturedSecurityQos->AuditData);
            CapturedSecurityQos->AuditData = NULL;
            return( STATUS_INVALID_PARAMETER );
        }
    }

    return( STATUS_SUCCESS );

}


VOID
SeFreeCapturedSecurityQos(
    IN PVOID SecurityQos
    )

/*++

Routine Description:

    This routine frees the data associated with a captured SecurityQos
    structure.  It does not free the body of the structure, just whatever
    its internal fields point to.

Arguments:

    SecurityQos - Points to a captured security QOS structure.

Return Value:

    None.

--*/

{
    PSECURITY_ADVANCED_QUALITY_OF_SERVICE IAdvancedSecurityQos;

    PAGED_CODE();

    IAdvancedSecurityQos = (PSECURITY_ADVANCED_QUALITY_OF_SERVICE)SecurityQos;

    if (IAdvancedSecurityQos->Length == sizeof( SECURITY_ADVANCED_QUALITY_OF_SERVICE )) {

        if (IAdvancedSecurityQos->AuditData != NULL) {
            ExFreePool( IAdvancedSecurityQos->AuditData );
        }

        SepFreeProxyData( IAdvancedSecurityQos->ProxyData );
    }

    return;
}


NTSTATUS
SeCaptureSecurityQos (
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN KPROCESSOR_MODE RequestorMode,
    OUT PBOOLEAN SecurityQosPresent,
    OUT PSECURITY_ADVANCED_QUALITY_OF_SERVICE CapturedSecurityQos
)
/*++

Routine Description:

    This routine probes and captures a copy of any security quality
    of service parameters that might have been provided via the
    ObjectAttributes argument.

Arguments:

    ObjectAttributes - The object attributes from which the QOS
        information is to be retrieved.

    RequestorMode - Indicates the processor mode by which the access
        is being requested.

    SecurityQosPresent - Receives a boolean value indicating whether
        or not the optional security QOS information was available
        and copied.

    CapturedSecurityQos - Receives the security QOS information if available.

Return Value:

    STATUS_SUCCESS indicates no exceptions were encountered.

    Any access violations encountered will be returned.

--*/

{

    PSECURITY_QUALITY_OF_SERVICE LocalSecurityQos;
    ULONG LocalQosLength;
    PSECURITY_ADVANCED_QUALITY_OF_SERVICE LocalAdvancedSecurityQos;
    NTSTATUS Status;
    BOOLEAN CapturedQos;

    PAGED_CODE();

    CapturedQos =  FALSE;
    //
    //  Set default return
    //

    (*SecurityQosPresent) = FALSE;

    //
    //  check if the requestors mode is kernel mode
    //

    if (RequestorMode != KernelMode) {
        try {

            if ( ARGUMENT_PRESENT(ObjectAttributes) ) {

                ProbeForRead( ObjectAttributes,
                              sizeof(OBJECT_ATTRIBUTES),
                              sizeof(ULONG)
                              );

                LocalSecurityQos =
                    (PSECURITY_QUALITY_OF_SERVICE)ObjectAttributes->SecurityQualityOfService;

                if ( ARGUMENT_PRESENT(LocalSecurityQos) ) {

                    ProbeForRead(
                        LocalSecurityQos,
                        sizeof(SECURITY_QUALITY_OF_SERVICE),
                        sizeof(ULONG)
                        );

                    LocalQosLength = LocalSecurityQos->Length;

                    //
                    // Check the length and see if this is a QOS or Advanced QOS
                    // structure.
                    //

                    if (LocalQosLength == sizeof( SECURITY_QUALITY_OF_SERVICE )) {

                        //
                        // It's a downlevel QOS, copy what's there and leave.
                        //

                        (*SecurityQosPresent) = TRUE;
                        RtlMoveMemory( CapturedSecurityQos, LocalSecurityQos, sizeof( SECURITY_QUALITY_OF_SERVICE ));
                        CapturedSecurityQos->ProxyData = NULL;
                        CapturedSecurityQos->AuditData = NULL;
                        CapturedSecurityQos->Length = LocalQosLength;

                    } else {

                        if (LocalQosLength == sizeof( SECURITY_ADVANCED_QUALITY_OF_SERVICE )) {

                            LocalAdvancedSecurityQos =
                                (PSECURITY_ADVANCED_QUALITY_OF_SERVICE)ObjectAttributes->SecurityQualityOfService;

                                ProbeForRead(
                                    LocalAdvancedSecurityQos,
                                    sizeof(SECURITY_ADVANCED_QUALITY_OF_SERVICE),
                                    sizeof(ULONG)
                                    );

                            (*SecurityQosPresent) = TRUE;
                            *CapturedSecurityQos = *LocalAdvancedSecurityQos;
                            CapturedSecurityQos->Length = LocalQosLength;

                            //
                            // Capture the proxy and audit data, if necessary.
                            //

                            if ( ARGUMENT_PRESENT(CapturedSecurityQos->ProxyData) || ARGUMENT_PRESENT( CapturedSecurityQos->AuditData ) ) {

                                CapturedQos = TRUE;
                                Status = SepProbeAndCaptureQosData( CapturedSecurityQos );

                                if (!NT_SUCCESS( Status )) {

                                    return( Status );
                                }
                            }

                        } else {

                            return( STATUS_INVALID_PARAMETER );
                        }
                    }

                } // end_if


            } // end_if

        } except(EXCEPTION_EXECUTE_HANDLER) {


            //
            // If we captured any proxy data, we need to free it now.
            //

            if ( CapturedQos ) {

                SepFreeProxyData( CapturedSecurityQos->ProxyData );

                if ( CapturedSecurityQos->AuditData != NULL ) {
                    ExFreePool( CapturedSecurityQos->AuditData );
                }
            }

            return GetExceptionCode();
        } // end_try


    } else {

        if ( ARGUMENT_PRESENT(ObjectAttributes) ) {
            if ( ARGUMENT_PRESENT(ObjectAttributes->SecurityQualityOfService) ) {
                (*SecurityQosPresent) = TRUE;

                if (((PSECURITY_QUALITY_OF_SERVICE)(ObjectAttributes->SecurityQualityOfService))->Length == sizeof( SECURITY_QUALITY_OF_SERVICE )) {

                    RtlMoveMemory( CapturedSecurityQos, ObjectAttributes->SecurityQualityOfService, sizeof( SECURITY_QUALITY_OF_SERVICE ));
                    CapturedSecurityQos->ProxyData = NULL;
                    CapturedSecurityQos->AuditData = NULL;

                } else {

                    (*CapturedSecurityQos) =
                        (*(SECURITY_ADVANCED_QUALITY_OF_SERVICE *)(ObjectAttributes->SecurityQualityOfService));
                }


            } // end_if
        } // end_if

    } // end_if

    return STATUS_SUCCESS;
}

NTSTATUS
SeCaptureSid (
    IN PSID InputSid,
    IN KPROCESSOR_MODE RequestorMode,
    IN PVOID CaptureBuffer OPTIONAL,
    IN ULONG CaptureBufferLength,
    IN POOL_TYPE PoolType,
    IN BOOLEAN ForceCapture,
    OUT PSID *CapturedSid
)
/*++

Routine Description:

    This routine probes and captures a copy of the specified SID.
    The SID is either captured into a provided buffer, or pool
    allocated to receive the SID.


    if the requestor mode is not kernel mode then

        probe and capture the input SID

    if the requstor mode is kernel mode then

        if force capture is true then

            do not probe the input SID, but do capture it

        else

            return address of original, but don't copy

Arguments:

    InputSid - Supplies the SID to capture.  This parameter is assumed
        to have been provided by the mode specified in RequestorMode.

    RequestorMode - Specifies the caller's access mode.

    CaptureBuffer - Specifies a buffer into which the SID is to be
        captured.  If this parameter is not provided, pool will be allocated
        to hold the captured data.

    CaptureBufferLength - Indicates the length, in bytes, of the capture
        buffer.

    PoolType - Specifies which pool type to allocate to capture the
        SID into.  This parameter is ignored if CaptureBuffer is provided.

    ForceCapture - Specifies whether the SID should be captured even if
        requestor mode is kernel.

    CapturedSid - Supplies the address of a pointer to an SID.
        The pointer will be set to point to the captured (or uncaptured) SID.

    AlignedSidSize - Supplies the address of a ULONG to receive the length
        of the SID rounded up to the next longword boundary.

Return Value:

    STATUS_SUCCESS indicates the capture was successful.

    STATUS_BUFFER_TOO_SMALL - indicates the buffer provided to capture the SID
        into wasn't large enough to hold the SID.

    Any access violations encountered will be returned.

--*/

{



    ULONG GetSidSubAuthorityCount;
    ULONG SidSize;

    PAGED_CODE();

    //
    //  check if the requestors mode is kernel mode and we are not
    //  to force a capture.
    //

    if ((RequestorMode == KernelMode) && (ForceCapture == FALSE)) {

        //
        //  We don't need to do any work and can simply
        //  return a pointer to the input SID
        //

        (*CapturedSid) = InputSid;

        return STATUS_SUCCESS;
    }


    //
    // Get the length needed to hold the SID
    //

    if (RequestorMode != KernelMode) {

        try {
            GetSidSubAuthorityCount =
                ProbeAndReadUchar( &(((SID *)(InputSid))->SubAuthorityCount) );
            SidSize = RtlLengthRequiredSid( GetSidSubAuthorityCount );
            ProbeForRead( InputSid,
                          SidSize,
                          sizeof(ULONG) );
        } except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

    } else {

        GetSidSubAuthorityCount = ((SID *)(InputSid))->SubAuthorityCount;
        SidSize = RtlLengthRequiredSid( GetSidSubAuthorityCount );

    }


    //
    // If a buffer was provided, compare lengths.
    // Otherwise, allocate a buffer.
    //

    if (ARGUMENT_PRESENT(CaptureBuffer)) {

        if (SidSize > CaptureBufferLength) {
            return STATUS_BUFFER_TOO_SMALL;
        } else {

            (*CapturedSid) = CaptureBuffer;
        }

    } else {

        (*CapturedSid) = (PSID)ExAllocatePoolWithTag(PoolType, SidSize, 'iSeS');

        if ( *CapturedSid == NULL ) {
            return( STATUS_INSUFFICIENT_RESOURCES );
        }

    }

    //
    // Now copy the SID and validate it
    //

    try {

        RtlMoveMemory( (*CapturedSid), InputSid, SidSize );
        ((SID *)(*CapturedSid))->SubAuthorityCount = (UCHAR) GetSidSubAuthorityCount;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        if (!ARGUMENT_PRESENT(CaptureBuffer)) {
            ExFreePool( (*CapturedSid) );
            *CapturedSid = NULL;
        }

        return GetExceptionCode();
    }

    if ((!RtlValidSid( (*CapturedSid) )) ) {

        if (!ARGUMENT_PRESENT(CaptureBuffer)) {
            ExFreePool( (*CapturedSid) );
            *CapturedSid = NULL;
        }

        return STATUS_INVALID_SID;
    }

    return STATUS_SUCCESS;

}


VOID
SeReleaseSid (
    IN PSID CapturedSid,
    IN KPROCESSOR_MODE RequestorMode,
    IN BOOLEAN ForceCapture
    )

/*++

Routine Description:

    This routine releases a previously captured SID.

    This routine should NOT be called if the SID was captured into a
    provided CaptureBuffer (see SeCaptureSid).

Arguments:

    CapturedSid - Supplies the SID to release.

    RequestorMode - The processor mode specified when the SID was captured.

    ForceCapture - The ForceCapture value specified when the SID was
        captured.

Return Value:

    None.

--*/

{
    //
    // We only have something to deallocate if the requestor was user
    // mode or kernel mode requesting ForceCapture.
    //

    PAGED_CODE();

    if ( ((RequestorMode == KernelMode) && (ForceCapture == TRUE)) ||
          (RequestorMode == UserMode ) ) {

        ExFreePool(CapturedSid);

    }

    return;

}

NTSTATUS
SeCaptureAcl (
    IN PACL InputAcl,
    IN KPROCESSOR_MODE RequestorMode,
    IN PVOID CaptureBuffer OPTIONAL,
    IN ULONG CaptureBufferLength,
    IN POOL_TYPE PoolType,
    IN BOOLEAN ForceCapture,
    OUT PACL *CapturedAcl,
    OUT PULONG AlignedAclSize
    )

/*++

Routine Description:

    This routine probes and captures a copy of the specified ACL.
    The ACL is either captured into a provided buffer, or pool
    allocated to receive the ACL.

    Any ACL captured will have its structure validated.


    if the requestor mode is not kernel mode then

        probe and capture the input ACL

    if the requstor mode is kernel mode then

        if force capture is true then

            do not probe the input ACL, but do capture it

        else

            return address of original, but don't copy

Arguments:

    InputAcl - Supplies the ACL to capture.  This parameter is assumed
        to have been provided by the mode specified in RequestorMode.

    RequestorMode - Specifies the caller's access mode.

    CaptureBuffer - Specifies a buffer into which the ACL is to be
        captured.  If this parameter is not provided, pool will be allocated
        to hold the captured data.

    CaptureBufferLength - Indicates the length, in bytes, of the capture
        buffer.

    PoolType - Specifies which pool type to allocate to capture the
        ACL into.  This parameter is ignored if CaptureBuffer is provided.

    ForceCapture - Specifies whether the ACL should be captured even if
        requestor mode is kernel.

    CapturedAcl - Supplies the address of a pointer to an ACL.
        The pointer will be set to point to the captured (or uncaptured) ACL.

    AlignedAclSize - Supplies the address of a ULONG to receive the length
        of the ACL rounded up to the next longword boundary.

Return Value:

    STATUS_SUCCESS indicates the capture was successful.

    STATUS_BUFFER_TOO_SMALL - indicates the buffer provided to capture the ACL
        into wasn't large enough to hold the ACL.

    Any access violations encountered will be returned.

--*/

{

    ULONG AclSize;

    PAGED_CODE();

    //
    //  check if the requestors mode is kernel mode and we are not
    //  to force a capture.
    //

    if ((RequestorMode == KernelMode) && (ForceCapture == FALSE)) {

        //
        //  We don't need to do any work and can simply
        //  return a pointer to the input ACL
        //

        (*CapturedAcl) = InputAcl;

        return STATUS_SUCCESS;
    }


    //
    // Get the length needed to hold the ACL
    //

    if (RequestorMode != KernelMode) {

        try {

            AclSize = ProbeAndReadUshort( &(InputAcl->AclSize) );

            ProbeForRead( InputAcl,
                          AclSize,
                          sizeof(ULONG) );

        } except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

    } else {

        AclSize = InputAcl->AclSize;

    }

    //
    // If the passed pointer is non-null, it has better at least
    // point to a well formed ACL
    //

    if (AclSize < sizeof(ACL)) {
        return( STATUS_INVALID_ACL );
    }

    (*AlignedAclSize) = (ULONG)LongAlignSize( AclSize );


    //
    // If a buffer was provided, compare lengths.
    // Otherwise, allocate a buffer.
    //

    if (ARGUMENT_PRESENT(CaptureBuffer)) {

        if (AclSize > CaptureBufferLength) {
            return STATUS_BUFFER_TOO_SMALL;
        } else {

            (*CapturedAcl) = CaptureBuffer;
        }

    } else {

        (*CapturedAcl) = (PACL)ExAllocatePoolWithTag(PoolType, AclSize, 'cAeS');

        if ( *CapturedAcl == NULL ) {
            return( STATUS_INSUFFICIENT_RESOURCES );
        }

    }

    //
    // Now copy the ACL and validate it
    //

    try {

        RtlMoveMemory( (*CapturedAcl), InputAcl, AclSize );

    } except(EXCEPTION_EXECUTE_HANDLER) {
        if (!ARGUMENT_PRESENT(CaptureBuffer)) {
            ExFreePool( (*CapturedAcl) );
        }

        *CapturedAcl = NULL;
        return GetExceptionCode();
    }

    if ( (!SepCheckAcl( (*CapturedAcl), AclSize )) ) {

        if (!ARGUMENT_PRESENT(CaptureBuffer)) {
            ExFreePool( (*CapturedAcl) );
        }

        *CapturedAcl = NULL;
        return STATUS_INVALID_ACL;
    }

    return STATUS_SUCCESS;

}


VOID
SeReleaseAcl (
    IN PACL CapturedAcl,
    IN KPROCESSOR_MODE RequestorMode,
    IN BOOLEAN ForceCapture
    )

/*++

Routine Description:

    This routine releases a previously captured ACL.

    This routine should NOT be called if the ACL was captured into a
    provided CaptureBuffer (see SeCaptureAcl).

Arguments:

    CapturedAcl - Supplies the ACL to release.

    RequestorMode - The processor mode specified when the ACL was captured.

    ForceCapture - The ForceCapture value specified when the ACL was
        captured.

Return Value:

    None.

--*/

{
    //
    // We only have something to deallocate if the requestor was user
    // mode or kernel mode requesting ForceCapture.
    //

    PAGED_CODE();

    if ( ((RequestorMode == KernelMode) && (ForceCapture == TRUE)) ||
          (RequestorMode == UserMode ) ) {

        ExFreePool(CapturedAcl);

    }

}

NTSTATUS
SeCaptureLuidAndAttributesArray (
    IN PLUID_AND_ATTRIBUTES InputArray,
    IN ULONG ArrayCount,
    IN KPROCESSOR_MODE RequestorMode,
    IN PVOID CaptureBuffer OPTIONAL,
    IN ULONG CaptureBufferLength,
    IN POOL_TYPE PoolType,
    IN BOOLEAN ForceCapture,
    OUT PLUID_AND_ATTRIBUTES *CapturedArray,
    OUT PULONG AlignedArraySize
    )

/*++

Routine Description:

    This routine probes and captures a copy of the specified
    LUID_AND_ATTRIBUTES array.

    The array is either captured into a provided buffer, or pool
    allocated to receive the array.


    if the requestor mode is not kernel mode then

        probe and capture the input array

    if the requstor mode is kernel mode then

        if force capture is true then

            do not probe the input array, but do capture it

        else

            return address of original, but don't copy

Arguments:

    InputArray - Supplies the array to capture.  This parameter is assumed
        to have been provided by the mode specified in RequestorMode.

    ArrayCount - Indicates the number of elements in the array to capture.

    RequestorMode - Specifies the caller's access mode.

    CaptureBuffer - Specifies a buffer into which the array is to be
        captured.  If this parameter is not provided, pool will be allocated
        to hold the captured data.

    CaptureBufferLength - Indicates the length, in bytes, of the capture
        buffer.

    PoolType - Specifies which pool type to allocate to capture the
        array into.  This parameter is ignored if CaptureBuffer is provided.

    ForceCapture - Specifies whether the array should be captured even if
        requestor mode is kernel.

    CapturedArray - Supplies the address of a pointer to an array.
        The pointer will be set to point to the captured (or uncaptured) array.

    AlignedArraySize - Supplies the address of a ULONG to receive the length
        of the array rounded up to the next longword boundary.

Return Value:

    STATUS_SUCCESS indicates the capture was successful.

    STATUS_BUFFER_TOO_SMALL - indicates the buffer provided to capture the array
        into wasn't large enough to hold the array.

    Any access violations encountered will be returned.

--*/

{

    ULONG ArraySize;

    PAGED_CODE();

    //
    // Make sure the array isn't empty
    //

    if (ArrayCount == 0) {
        (*CapturedArray) = NULL;
        (*AlignedArraySize) = 0;
        return STATUS_SUCCESS;
    }

    //
    // If there are too many LUIDs, return failure
    //

    if (ArrayCount > SEP_MAX_PRIVILEGE_COUNT) {
        return(STATUS_INVALID_PARAMETER);
    }

    //
    //  check if the requestors mode is kernel mode and we are not
    //  to force a capture.
    //

    if ((RequestorMode == KernelMode) && (ForceCapture == FALSE)) {

        //
        //  We don't need to do any work and can simply
        //  return a pointer to the input array
        //

        (*CapturedArray) = InputArray;

        return STATUS_SUCCESS;
    }


    //
    // Get the length needed to hold the array
    //

    ArraySize = ArrayCount * (ULONG)sizeof(LUID_AND_ATTRIBUTES);
    (*AlignedArraySize) = (ULONG)LongAlignSize( ArraySize );

    if (RequestorMode != KernelMode) {

        try {


            ProbeForRead( InputArray,
                          ArraySize,
                          sizeof(ULONG) );

        } except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

    }



    //
    // If a buffer was provided, compare lengths.
    // Otherwise, allocate a buffer.
    //

    if (ARGUMENT_PRESENT(CaptureBuffer)) {

        if (ArraySize > CaptureBufferLength) {
            return STATUS_BUFFER_TOO_SMALL;
        } else {

            (*CapturedArray) = CaptureBuffer;
        }

    } else {

        (*CapturedArray) =
            (PLUID_AND_ATTRIBUTES)ExAllocatePoolWithTag(PoolType, ArraySize, 'uLeS');

        if ( *CapturedArray == NULL ) {
            return( STATUS_INSUFFICIENT_RESOURCES );
        }

    }

    //
    // Now copy the array
    //

    try {

        RtlMoveMemory( (*CapturedArray), InputArray, ArraySize );

    } except(EXCEPTION_EXECUTE_HANDLER) {
        if (!ARGUMENT_PRESENT(CaptureBuffer)) {
            ExFreePool( (*CapturedArray) );
        }

        return GetExceptionCode();
    }

    return STATUS_SUCCESS;

}


VOID
SeReleaseLuidAndAttributesArray (
    IN PLUID_AND_ATTRIBUTES CapturedArray,
    IN KPROCESSOR_MODE RequestorMode,
    IN BOOLEAN ForceCapture
    )

/*++

Routine Description:

    This routine releases a previously captured array of LUID_AND_ATTRIBUTES.

    This routine should NOT be called if the array was captured into a
    provided CaptureBuffer (see SeCaptureLuidAndAttributesArray).

Arguments:

    CapturedArray - Supplies the array to release.

    RequestorMode - The processor mode specified when the array was captured.

    ForceCapture - The ForceCapture value specified when the array was
        captured.

Return Value:

    None.

--*/

{
    //
    // We only have something to deallocate if the requestor was user
    // mode or kernel mode requesting ForceCapture.
    //

    PAGED_CODE();

    if ( ((RequestorMode == KernelMode) && (ForceCapture == TRUE)) ||
          (RequestorMode == UserMode )) {
        //
        // the capture routine returns success with a null pointer for zero elements.
        //
        if (CapturedArray != NULL)
           ExFreePool(CapturedArray);

    }

    return;

}

NTSTATUS
SeCaptureSidAndAttributesArray (
    IN PSID_AND_ATTRIBUTES InputArray,
    IN ULONG ArrayCount,
    IN KPROCESSOR_MODE RequestorMode,
    IN PVOID CaptureBuffer OPTIONAL,
    IN ULONG CaptureBufferLength,
    IN POOL_TYPE PoolType,
    IN BOOLEAN ForceCapture,
    OUT PSID_AND_ATTRIBUTES *CapturedArray,
    OUT PULONG AlignedArraySize
    )

/*++

Routine Description:

    This routine probes and captures a copy of the specified
    SID_AND_ATTRIBUTES array, along with the SID values pointed
    to.

    The array is either captured into a provided buffer, or pool
    allocated to receive the array.

    The format of the captured information is an array of SID_AND_ATTRIBUTES
    data structures followed by the SID values.  THIS MAY NOT BE THE CASE
    FOR KERNEL MODE UNLESS A FORCE CAPTURE IS SPECIFIED.


    if the requestor mode is not kernel mode then

        probe and capture the input array

    if the requstor mode is kernel mode then

        if force capture is true then

            do not probe the input array, but do capture it

        else

            return address of original, but don't copy

Arguments:

    InputArray - Supplies the array to capture.  This parameter is assumed
        to have been provided by the mode specified in RequestorMode.

    ArrayCount - Indicates the number of elements in the array to capture.

    RequestorMode - Specifies the caller's access mode.

    CaptureBuffer - Specifies a buffer into which the array is to be
        captured.  If this parameter is not provided, pool will be allocated
        to hold the captured data.

    CaptureBufferLength - Indicates the length, in bytes, of the capture
        buffer.

    PoolType - Specifies which pool type to allocate to capture the
        array into.  This parameter is ignored if CaptureBuffer is provided.

    ForceCapture - Specifies whether the array should be captured even if
        requestor mode is kernel.

    CapturedArray - Supplies the address of a pointer to an array.
        The pointer will be set to point to the captured (or uncaptured) array.

    AlignedArraySize - Supplies the address of a ULONG to receive the length
        of the array rounded up to the next longword boundary.

Return Value:

    STATUS_SUCCESS indicates the capture was successful.

    STATUS_BUFFER_TOO_SMALL - indicates the buffer provided to capture the array
        into wasn't large enough to hold the array.

    Any access violations encountered will be returned.

--*/

{

typedef struct _TEMP_ARRAY_ELEMENT {
    PISID  Sid;
    ULONG SidLength;
} TEMP_ARRAY_ELEMENT;


    TEMP_ARRAY_ELEMENT *TempArray;

    NTSTATUS CompletionStatus = STATUS_SUCCESS;

    ULONG ArraySize;
    ULONG AlignedLengthRequired;

    ULONG NextIndex;

    PSID_AND_ATTRIBUTES NextElement;
    PVOID NextBufferLocation;

    ULONG GetSidSubAuthorityCount;
    ULONG SidSize;
    ULONG AlignedSidSize;

    PAGED_CODE();

    //
    // Make sure the array isn't empty
    //

    if (ArrayCount == 0) {
        (*CapturedArray) = NULL;
        (*AlignedArraySize) = 0;
        return STATUS_SUCCESS;
    }

    //
    // Check there aren't too many SIDs
    //

    if (ArrayCount > SEP_MAX_GROUP_COUNT) {
        return(STATUS_INVALID_PARAMETER);
    }
    //
    //  check if the requestor's mode is kernel mode and we are not
    //  to force a capture.
    //

    if ((RequestorMode == KernelMode) && (ForceCapture == FALSE)) {

        //
        //  We don't need to do any work and can simply
        //  return a pointer to the input array
        //

        (*CapturedArray) = InputArray;

        return STATUS_SUCCESS;
    }


    //
    // ---------- For RequestorMode == UserMode ----------------------
    //
    // the algorithm for capturing an SID_AND_ATTRIBUTES array is somewhat
    // convoluted to avoid problems that could occur if the data is
    // being changed while being captured.
    //
    // The algorithm uses two loops.
    //
    //    Allocate a temporary buffer to house the fixed length data.
    //
    //    1st loop:
    //          For each SID:
    //              Capture the Pointers to the SID and the length of the SID.
    //
    //    Allocate a buffer large enough to hold all of the data.
    //
    //    2nd loop:
    //          For each SID:
    //               Capture the Attributes.
    //               Capture the SID.
    //               Set the pointer to the SID.
    //
    //    Deallocate temporary buffer.
    //
    // ------------ For RequestorMode == KernelMode --------------------
    //
    // There is no need to capture the length and address of the SIDs
    // in the first loop (since the kernel can be trusted not to change
    // them while they are being copied.)  So for kernel mode, the first
    // loop just adds up the length needed.  Kernel mode, thus, avoids
    // having to allocate a temporary buffer.
    //

    //
    // Get the length needed to hold the array elements.
    //

    ArraySize = ArrayCount * (ULONG)sizeof(SID_AND_ATTRIBUTES);
    AlignedLengthRequired = (ULONG)LongAlignSize( ArraySize );

    if (RequestorMode != KernelMode) {

        //
        // Allocate a temporary array to capture the array elements into
        //

        TempArray =
            (TEMP_ARRAY_ELEMENT *)ExAllocatePoolWithTag(PoolType, AlignedLengthRequired, 'aTeS');

        if ( TempArray == NULL ) {
            return( STATUS_INSUFFICIENT_RESOURCES );
        }


        try {

            //
            // Make sure we can read each SID_AND_ATTRIBUTE
            //

            ProbeForRead( InputArray,
                          ArraySize,
                          sizeof(ULONG) );

            //
            // Probe and capture the length and address of each SID
            //

            NextIndex = 0;
            while (NextIndex < ArrayCount) {
                PSID TempSid;

                TempSid = InputArray[NextIndex].Sid;
                GetSidSubAuthorityCount =
                    ProbeAndReadUchar( &((PISID)TempSid)->SubAuthorityCount);

                if (GetSidSubAuthorityCount > SID_MAX_SUB_AUTHORITIES) {
                    CompletionStatus = STATUS_INVALID_SID;
                    break;
                }

                TempArray[NextIndex].Sid = ((PISID)(TempSid));
                TempArray[NextIndex].SidLength =
                    RtlLengthRequiredSid( GetSidSubAuthorityCount );

                ProbeForRead( TempArray[NextIndex].Sid,
                              TempArray[NextIndex].SidLength,
                              sizeof(ULONG) );

                AlignedLengthRequired +=
                    (ULONG)LongAlignSize( TempArray[NextIndex].SidLength );

                NextIndex += 1;

            }  //end while

        } except(EXCEPTION_EXECUTE_HANDLER) {

            ExFreePool( TempArray );
            return GetExceptionCode();
        }

        if (!NT_SUCCESS(CompletionStatus)) {
            ExFreePool( TempArray );
            return(CompletionStatus);
        }

    } else {

        //
        // No need to capture anything.
        // But, we do need to add up the lengths of the SIDs
        // so we can allocate a buffer (or check the size of one provided).
        //

        NextIndex = 0;

        while (NextIndex < ArrayCount) {

            GetSidSubAuthorityCount =
                ((PISID)(InputArray[NextIndex].Sid))->SubAuthorityCount;

            AlignedLengthRequired +=
                (ULONG)LongAlignSize(RtlLengthRequiredSid(GetSidSubAuthorityCount));

            NextIndex += 1;

        }  //end while

    }


    //
    // Now we know how much memory we need.
    // Return this value in the output parameter.
    //

    (*AlignedArraySize) = AlignedLengthRequired;

    //
    // If a buffer was provided, make sure it is long enough.
    // Otherwise, allocate a buffer.
    //

    if (ARGUMENT_PRESENT(CaptureBuffer)) {

        if (AlignedLengthRequired > CaptureBufferLength) {

            if (RequestorMode != KernelMode) {
                ExFreePool( TempArray );
            }

            return STATUS_BUFFER_TOO_SMALL;

        } else {

            (*CapturedArray) = CaptureBuffer;
        }

    } else {

        (*CapturedArray) =
            (PSID_AND_ATTRIBUTES)ExAllocatePoolWithTag(PoolType, AlignedLengthRequired, 'aSeS');

        if ( *CapturedArray == NULL ) {
                if (RequestorMode != KernelMode) {
                    ExFreePool( TempArray );
                }
            return( STATUS_INSUFFICIENT_RESOURCES );
        }
    }


    //
    // Now copy everything.
    // This is done by copying all the SID_AND_ATTRIBUTES and then
    // copying each individual SID.
    //
    // All SIDs have already been probed for READ access.  We just
    // need to copy them.
    //
    //

    if (RequestorMode != KernelMode) {
        try {

            //
            //  Copy the SID_AND_ATTRIBUTES array elements
            //  This really only sets the attributes, since we
            //  over-write the SID pointer field later on.
            //

            NextBufferLocation = (*CapturedArray);
            RtlMoveMemory( NextBufferLocation, InputArray, ArraySize );
            NextBufferLocation = (PVOID)((ULONG_PTR)NextBufferLocation +
                                         (ULONG)LongAlignSize(ArraySize) );

            //
            //  Now go through and copy each referenced SID.
            //  Validate each SID as it is copied.
            //

            NextIndex = 0;
            NextElement = (*CapturedArray);
            while (  (NextIndex < ArrayCount) &&
                     (CompletionStatus == STATUS_SUCCESS) ) {


                RtlMoveMemory( NextBufferLocation,
                    TempArray[NextIndex].Sid,
                    TempArray[NextIndex].SidLength );


                NextElement[NextIndex].Sid = (PSID)NextBufferLocation;
                NextBufferLocation =
                    (PVOID)((ULONG_PTR)NextBufferLocation +
                            (ULONG)LongAlignSize(TempArray[NextIndex].SidLength));

                //
                // Verify the sid is valid and its length didn't change
                //

                if (!RtlValidSid(NextElement[NextIndex].Sid) ) {
                    CompletionStatus = STATUS_INVALID_SID;
                } else if (RtlLengthSid(NextElement[NextIndex].Sid) != TempArray[NextIndex].SidLength) {
                    CompletionStatus = STATUS_INVALID_SID;
                }


                NextIndex += 1;

            }  //end while


        } except(EXCEPTION_EXECUTE_HANDLER) {

            if (!ARGUMENT_PRESENT(CaptureBuffer)) {
                ExFreePool( (*CapturedArray) );
            }

            ExFreePool( TempArray );

            return GetExceptionCode();
        }
    } else {

        //
        // Requestor mode is kernel mode -
        // don't need protection, probing, and validating
        //

        //
        //  Copy the SID_AND_ATTRIBUTES array elements
        //  This really only sets the attributes, since we
        //  over-write the SID pointer field later on.
        //

        NextBufferLocation = (*CapturedArray);
        RtlMoveMemory( NextBufferLocation, InputArray, ArraySize );
        NextBufferLocation = (PVOID)( (ULONG_PTR)NextBufferLocation +
                                      (ULONG)LongAlignSize(ArraySize));

        //
        //  Now go through and copy each referenced SID
        //

        NextIndex = 0;
        NextElement = (*CapturedArray);
        while (NextIndex < ArrayCount) {

            GetSidSubAuthorityCount =
                ((PISID)(NextElement[NextIndex].Sid))->SubAuthorityCount;

            RtlMoveMemory(
                NextBufferLocation,
                NextElement[NextIndex].Sid,
                RtlLengthRequiredSid(GetSidSubAuthorityCount) );
                SidSize = RtlLengthRequiredSid( GetSidSubAuthorityCount );
                AlignedSidSize = (ULONG)LongAlignSize(SidSize);

            NextElement[NextIndex].Sid = (PSID)NextBufferLocation;

            NextIndex += 1;
            NextBufferLocation = (PVOID)((ULONG_PTR)NextBufferLocation +
                                                   AlignedSidSize);

        }  //end while

    }

    if (RequestorMode != KernelMode) {
        ExFreePool( TempArray );
    }

    if (!ARGUMENT_PRESENT(CaptureBuffer) && !NT_SUCCESS(CompletionStatus)) {
        ExFreePool( (*CapturedArray) );
        *CapturedArray = NULL ;
    }

    return CompletionStatus;
}


VOID
SeReleaseSidAndAttributesArray (
    IN PSID_AND_ATTRIBUTES CapturedArray,
    IN KPROCESSOR_MODE RequestorMode,
    IN BOOLEAN ForceCapture
    )

/*++

Routine Description:

    This routine releases a previously captured array of SID_AND_ATTRIBUTES.

    This routine should NOT be called if the array was captured into a
    provided CaptureBuffer (see SeCaptureSidAndAttributesArray).

Arguments:

    CapturedArray - Supplies the array to release.

    RequestorMode - The processor mode specified when the array was captured.

    ForceCapture - The ForceCapture value specified when the array was
        captured.

Return Value:

    None.

--*/

{
    //
    // We only have something to deallocate if the requestor was user
    // mode or kernel mode requesting ForceCapture.
    //

    PAGED_CODE();

    if ( ((RequestorMode == KernelMode) && (ForceCapture == TRUE)) ||
          (RequestorMode == UserMode ) ) {

        ExFreePool(CapturedArray);

    }

    return;

}




NTSTATUS
SeComputeQuotaInformationSize(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PULONG Size
    )

/*++

Routine Description:

    This routine computes the size of the Group and DACL for the
    passed security descriptor.

    This quantity will later be used in calculating the amount
    of quota to charge for this object.

Arguments:

    SecurityDescriptor - Supplies a pointer to the security descriptor
        to be examined.

    Size - Returns the size in bytes of the sum of the Group and Dacl
        fields of the security descriptor.

Return Value:

    STATUS_SUCCESS - The operation was successful.

    STATUS_INVALID_REVISION - The passed security descriptor was of
        an unknown revision.

--*/

{
    PISECURITY_DESCRIPTOR ISecurityDescriptor;

    PSID Group;
    PACL Dacl;

    PAGED_CODE();

    ISecurityDescriptor = (PISECURITY_DESCRIPTOR)SecurityDescriptor;
    *Size = 0;

    if (ISecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION) {
        return( STATUS_UNKNOWN_REVISION );
    }

    Group = RtlpGroupAddrSecurityDescriptor( ISecurityDescriptor );

    Dacl = RtlpDaclAddrSecurityDescriptor( ISecurityDescriptor );

    if (Group != NULL) {
        *Size += (ULONG)LongAlignSize(SeLengthSid( Group ));
    }

    if (Dacl != NULL) {
        *Size += (ULONG)LongAlignSize(Dacl->AclSize);
    }

    return( STATUS_SUCCESS );
}


BOOLEAN
SeValidSecurityDescriptor(
    IN ULONG Length,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    )

/*++

Routine Description:

    Validates a security descriptor for structural correctness.  The idea is to make
    sure that the security descriptor may be passed to other kernel callers, without
    fear that they're going to choke while manipulating it.

    This routine does not enforce policy (e.g., ACL/ACE revision information).  It is
    entirely possible for a security descriptor to be approved by this routine, only
    to be later found to be invalid by some later routine.

    This routine is designed to be used by callers who have a security descriptor in
    kernel memory.  Callers wishing to validate a security descriptor passed from user
    mode should call RtlValidSecurityDescriptor.

Arguments:

    Length - Length in bytes of passed Security Descriptor.

    SecurityDescriptor - Points to the Security Descriptor (in kernel memory) to be
        validatated.

Return Value:

    TRUE - The passed security descriptor is correctly structured
    FALSE - The passed security descriptor is badly formed

--*/

{
    PISECURITY_DESCRIPTOR_RELATIVE ISecurityDescriptor =
        (PISECURITY_DESCRIPTOR_RELATIVE)SecurityDescriptor;
    PISID OwnerSid;
    PISID GroupSid;
    PACE_HEADER Ace;
    PISID Sid;
    PISID Sid2;
    PACL Dacl;
    PACL Sacl;
    ULONG i;

    if (Length < sizeof(SECURITY_DESCRIPTOR_RELATIVE)) {
        return(FALSE);
    }

    //
    // Check the revision information.
    //

    if (ISecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION) {
        return(FALSE);
    }

    //
    // Make sure the passed SecurityDescriptor is in self-relative form
    //

    if (!(ISecurityDescriptor->Control & SE_SELF_RELATIVE)) {
        return(FALSE);
    }

    //
    // Check the owner.  A valid SecurityDescriptor must have an owner.
    // It must also be long aligned.
    //

    if ((ISecurityDescriptor->Owner == 0) ||
        (!LongAligned((PVOID)(ULONG_PTR)(ULONG)ISecurityDescriptor->Owner)) ||
        (ISecurityDescriptor->Owner > Length) ||
        (Length - ISecurityDescriptor->Owner < sizeof(SID))) {

        return(FALSE);
    }

    //
    // It is safe to reference the owner's SubAuthorityCount, compute the
    // expected length of the SID
    //

    OwnerSid = (PSID)RtlOffsetToPointer( ISecurityDescriptor, ISecurityDescriptor->Owner );

    if (OwnerSid->Revision != SID_REVISION) {
        return(FALSE);
    }

    if (OwnerSid->SubAuthorityCount > SID_MAX_SUB_AUTHORITIES) {
        return(FALSE);
    }

    if (Length - ISecurityDescriptor->Owner < (ULONG) SeLengthSid(OwnerSid)) {
        return(FALSE);
    }

    //
    // The owner appears to be a structurally valid SID that lies within
    // the bounds of the security descriptor.  Do the same for the Group
    // if there is one.
    //

    if (ISecurityDescriptor->Group != 0) {

        //
        // Check alignment
        //

        if (!LongAligned( (PVOID)(ULONG_PTR)(ULONG)ISecurityDescriptor->Group)) {
            return(FALSE);
        }

        if (ISecurityDescriptor->Group > Length) {
            return(FALSE);
        }

        if (Length - ISecurityDescriptor->Group < sizeof (SID)) {
            return(FALSE);
        }

        //
        // It is safe to reference the Group's SubAuthorityCount, compute the
        // expected length of the SID
        //

        GroupSid = (PSID)RtlOffsetToPointer( ISecurityDescriptor, ISecurityDescriptor->Group );

        if (GroupSid->Revision != SID_REVISION) {
            return(FALSE);
        }

        if (GroupSid->SubAuthorityCount > SID_MAX_SUB_AUTHORITIES) {
            return(FALSE);
        }

        if (Length - ISecurityDescriptor->Group < (ULONG) SeLengthSid(GroupSid)) {
            return(FALSE);
        }
    }

    //
    // Validate the DACL.  A structurally valid SecurityDescriptor may not necessarily
    // have a DACL.
    //

    if (ISecurityDescriptor->Dacl != 0) {

        //
        // Check alignment
        //

        if (!LongAligned( (PVOID)(ULONG_PTR)(ULONG)ISecurityDescriptor->Dacl)) {
            return(FALSE);
        }

        //
        // Make sure the DACL structure is within the bounds of the security descriptor.
        //

        if ((ISecurityDescriptor->Dacl > Length) ||
            (Length - ISecurityDescriptor->Dacl < sizeof(ACL))) {
            return(FALSE);
        }

        Dacl = (PACL) RtlOffsetToPointer( ISecurityDescriptor, ISecurityDescriptor->Dacl );


        //
        // Make sure the DACL length fits within the bounds of the security descriptor.
        //

        if (Length - ISecurityDescriptor->Dacl < Dacl->AclSize) {
            return(FALSE);
        }

        //
        // Make sure the ACL is structurally valid.
        //

        if (!RtlValidAcl( Dacl )) {
            return(FALSE);
        }
    }

    //
    // Validate the SACL.  A structurally valid SecurityDescriptor may not
    // have a SACL.
    //

    if (ISecurityDescriptor->Sacl != 0) {

        //
        // Check alignment
        //

        if (!LongAligned( (PVOID)(ULONG_PTR)(ULONG)ISecurityDescriptor->Sacl)) {
            return(FALSE);
        }

        //
        // Make sure the SACL structure is within the bounds of the security descriptor.
        //

        if ((ISecurityDescriptor->Sacl > Length) ||
            (Length - ISecurityDescriptor->Sacl < sizeof(ACL))) {
            return(FALSE);
        }

        //
        // Make sure the Sacl structure is within the bounds of the security descriptor.
        //

        Sacl = (PACL)RtlOffsetToPointer( ISecurityDescriptor, ISecurityDescriptor->Sacl );


        if (Length - ISecurityDescriptor->Sacl < Sacl->AclSize) {
            return(FALSE);
        }

        //
        // Make sure the ACL is structurally valid.
        //

        if (!RtlValidAcl( Sacl )) {
            return(FALSE);
        }
    }

    return(TRUE);
}

