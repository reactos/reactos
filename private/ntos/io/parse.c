/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    parse.c

Abstract:

    This module contains the code to implement the device object parse routine.

Author:

    Darryl E. Havens (darrylh) 15-May-1988

Environment:

    Kernel mode

Revision History:


--*/

#include "iop.h"

//
// Define macro to round up the size of a name for buffer optimization.
//

#define RoundNameSize( Length ) ( \
    (Length < 64 - 8) ? 64 - 8 :  \
    (Length < 128 - 8) ? 128 - 8 :\
    (Length < 256 - 8) ? 256 - 8 : Length )

#define IO_MAX_REMOUNT_REPARSE_ATTEMPTS 32

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IopParseFile)
#pragma alloc_text(PAGE, IopParseDevice)
#pragma alloc_text(PAGE, IopQueryName)
#pragma alloc_text(PAGE, IopCheckBackupRestorePrivilege)
#endif

NTSTATUS
IopCheckDeviceAndDriver(
    POPEN_PACKET op,
    PDEVICE_OBJECT parseDeviceObject
    )
{
    NTSTATUS status;
    KIRQL irql;

    //
    // Make sure that the device and its driver are really there and they are
    // going to stay there.  The object itself cannot go away just yet because
    // the object management system has performed a reference which bumps the
    // count of the number of reasons why the object must stick around.
    // However, the driver could be attempting to unload itself, so perform
    // this check.  If the driver is being unloaded, then set the final status
    // of the operation to "No such device" and return with a NULL file object
    // pointer.
    //
    // Note that it is possible to "open" an exclusive device more than once
    // provided that the caller is performing a relative open.  This feature
    // is how users "allocate" a device, and then use it to perform operations.
    //

    ExAcquireFastLock( &IopDatabaseLock, &irql );

    if (parseDeviceObject->DeviceObjectExtension->ExtensionFlags &
            (DOE_UNLOAD_PENDING | DOE_DELETE_PENDING | DOE_REMOVE_PENDING | DOE_REMOVE_PROCESSED | DOE_START_PENDING) ||
        parseDeviceObject->Flags & DO_DEVICE_INITIALIZING) {

        status = STATUS_NO_SUCH_DEVICE;

    } else if (parseDeviceObject->Flags & DO_EXCLUSIVE &&
               parseDeviceObject->ReferenceCount != 0 &&
               op->RelatedFileObject == NULL &&
               !(op->Options & IO_ATTACH_DEVICE)) {

        status = STATUS_ACCESS_DENIED;

    } else {

        parseDeviceObject->ReferenceCount++;
        status = STATUS_SUCCESS;

    }

    ExReleaseFastLock( &IopDatabaseLock, irql );

    return status;
}

PVPB
IopCheckVpbMounted(
    IN POPEN_PACKET op,
    IN PDEVICE_OBJECT parseDeviceObject,
    IN OUT PUNICODE_STRING RemainingName,
    OUT PNTSTATUS status
    )
{
    PVPB vpb;
    KIRQL irql;
    BOOLEAN alertable;

    //
    // Loop here until the VPB_MOUNTED test can be passed while holding the
    // VPB spinlock.  After the mount succeeds, it is still necessary to acquire
    // the spinlock to check that the VPB (which may be different from the one
    // before the mount) is still mounted.  If it is, then its reference count
    // is incremented before releasing the spinlock.
    //

    ExAcquireFastLock( &IopVpbSpinLock, &irql );

    alertable = (op->CreateOptions & FILE_SYNCHRONOUS_IO_ALERT) ? TRUE : FALSE;
    while (!(parseDeviceObject->Vpb->Flags & VPB_MOUNTED)) {

        ExReleaseFastLock( &IopVpbSpinLock, irql );

        //
        // Try to mount the volume, allowing only RAW to perform the mount if
        // this is a DASD open.
        //

        *status = IopMountVolume( parseDeviceObject,
                                 (BOOLEAN) (!RemainingName->Length && !op->RelatedFileObject),
                                 FALSE,
                 alertable );
        //
        // If the mount operation was unsuccessful, adjust the reference
        // count for the device and return now.
        //

        if (!NT_SUCCESS( *status ) || *status == STATUS_USER_APC || *status == STATUS_ALERTED) {

            IopDecrementDeviceObjectRef( parseDeviceObject, FALSE );

            if (!NT_SUCCESS( *status )) {
                return NULL;
            } else {
                *status = STATUS_WRONG_VOLUME;
                return NULL;
            }
        }

        ExAcquireFastLock( &IopVpbSpinLock, &irql );
    }

    //
    // Synchronize here with the file system to make sure that volumes do not
    // go away while en route to the FS.
    //

    vpb = parseDeviceObject->Vpb;

    //
    //  Check here that the VPB is not locked.
    //

    if (vpb->Flags & VPB_LOCKED) {

        *status = STATUS_ACCESS_DENIED;
        vpb = NULL;

    } else {

        vpb->ReferenceCount += 1;
    }

    ExReleaseFastLock( &IopVpbSpinLock, irql );

    return vpb;
}

VOID
IopDereferenceVpbAndFree(
    IN PVPB Vpb
    )
{
    KIRQL irql;
    PVPB vpb = (PVPB) NULL;

    ExAcquireFastLock( &IopVpbSpinLock, &irql );
    Vpb->ReferenceCount--;
    if ((Vpb->ReferenceCount == 0) &&
        (Vpb->RealDevice->Vpb != Vpb) &&
        !(Vpb->Flags & VPB_PERSISTENT)) {
        vpb = Vpb;
    }
    ExReleaseFastLock( &IopVpbSpinLock, irql );
    if (vpb) {
        ExFreePool( vpb );
    }
}


NTSTATUS
IopParseDevice(
    IN PVOID ParseObject,
    IN PVOID ObjectType,
    IN PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE AccessMode,
    IN ULONG Attributes,
    IN OUT PUNICODE_STRING CompleteName,
    IN OUT PUNICODE_STRING RemainingName,
    IN OUT PVOID Context OPTIONAL,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
    OUT PVOID *Object
    )

/*++

Routine Description:

    This routine interfaces to the NT Object Manager.  It is invoked when
    the object system is given the name of an entity to create or open and the
    name translates to a device object.  This routine is specified as the parse
    routine for all device objects.

    In the normal case of an NtCreateFile, the user specifies either the name
    of a device or of a file.  In the former situation, this routine is invoked
    with a pointer to the device and a null ("") string.  For this case, the
    routine simply allocates an IRP, fills it in, and passes it to the driver
    for the device.  The driver will then perform whatever rudimentary functions
    are necessary and will return a status code indicating whether an error was
    incurred.  This status code is remembered in the Open Packet (OP).

    In the latter situation, the name string to be opened/created is non-null.
    That is, it contains the remainder of the pathname to the file that is to
    be opened or created.  For this case, the routine allocates an IRP, fills
    it in, and passes it to the driver for the device.  The driver may then
    need to take further action or it may complete the request immediately.  If
    it needs to perform some work asynchronously, then it can queue the request
    and return a status of STATUS_PENDING.  This allows this routine and its
    caller to return to the user so that he can continue.  Otherwise, the open/
    create is basically finished.

    If the driver supports symbolic links, then it is also possible for the
    driver to return a new name.  This name will be returned to the Object
    Manager as a new name to look up.  The parsing will then begin again from
    the start.

    It is also the responsibility of this routine to create a file object for
    the file, if the name specifies a file.  The file object's address is
    returned to the NtCreateFile service through the OP.

Arguments:

    ParseObject - Pointer to the device object the name translated into.

    ObjectType - Type of the object being opened.

    AccessState - Running security access state information for operation.

    AccessMode - Access mode of the original caller.

    Attributes - Attributes to be applied to the object.

    CompleteName - Complete name of the object.

    RemainingName - Remaining name of the object.

    Context - Pointer to an Open Packet (OP) from NtCreateFile service.

    SecurityQos - Optional security quality of service indicator.

    Object - The address of a variable to receive the created file object, if
        any.

Return Value:

    The function return value is one of the following:

        a)  Success - This indicates that the function succeeded and the object
            parameter contains the address of the created file object.

        b)  Error - This indicates that the file was not found or created and
            no file object was created.

        c)  Reparse - This indicates that the remaining name string has been
            replaced by a new name that is to be parsed.

--*/

{

#define COPY_ATTRIBUTES( n, b, s ) {                                    \
        (n)->CreationTime.QuadPart = (b)->CreationTime.QuadPart;        \
        (n)->LastAccessTime.QuadPart = (b)->LastAccessTime.QuadPart;    \
        (n)->LastWriteTime.QuadPart = (b)->LastWriteTime.QuadPart;      \
        (n)->ChangeTime.QuadPart = (b)->ChangeTime.QuadPart;            \
        (n)->AllocationSize.QuadPart = (s)->AllocationSize.QuadPart;    \
        (n)->EndOfFile.QuadPart = (s)->EndOfFile.QuadPart;              \
        (n)->FileAttributes = (b)->FileAttributes; }

    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    POPEN_PACKET op;
    PFILE_OBJECT fileObject;
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatus;
    IO_SECURITY_CONTEXT securityContext;
    PDEVICE_OBJECT deviceObject;
    PDEVICE_OBJECT parseDeviceObject;
    BOOLEAN directDeviceOpen;
    PVPB vpb;
    ACCESS_MASK desiredAccess;
    PDUMMY_FILE_OBJECT localFileObject;
    BOOLEAN realFileObjectRequired;
    KPROCESSOR_MODE modeForPrivilegeCheck;
    ULONG retryCount = 0;
    BOOLEAN  relativeVolumeOpen = FALSE;     // True if opening a filesystem volume

    PAGED_CODE();

reparse_loop:

    //
    // Assume failure by setting the returned object pointer to NULL.
    //

    *Object = (PVOID) NULL;

    //
    // Get the address of the Open Packet (OP).
    //

    op = Context;

    //
    // Ensure that this routine is actually being invoked because someone is
    // attempting to open a device or a file through NtCreateFile.  This code
    // must be invoked from there (as opposed to some other random object
    // create or open routine).
    //

    if (op == NULL ||
        op->Type != IO_TYPE_OPEN_PACKET ||
        op->Size != sizeof( OPEN_PACKET )) {

        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    //
    // Obtain a pointer to the parse object as a device object, which is the
    // actual type of the object anyway.
    //

    parseDeviceObject = (PDEVICE_OBJECT) ParseObject;

    //
    // If this is a relative open, then get the device on which the file
    // is really being opened from the related file object and use that for
    // the remainder of this function and for all operations performed on
    // the file object that is about to be created.
    //

    if (op->RelatedFileObject) {
        parseDeviceObject = op->RelatedFileObject->DeviceObject;
    }

    //
    // Make sure that the device and its driver are really there and they are
    // going to stay there.  The object itself cannot go away just yet because
    // the object management system has performed a reference which bumps the
    // count of the number of reasons why the object must stick around.
    // However, the driver could be attempting to unload itself, so perform
    // this check.  If the driver is being unloaded, then set the final status
    // of the operation to "No such device" and return with a NULL file object
    // pointer.
    //
    // Note that it is possible to "open" an exclusive device more than once
    // provided that the caller is performing a relative open.  This feature
    // is how users "allocate" a device, and then use it to perform operations.
    //

    status = IopCheckDeviceAndDriver( op, parseDeviceObject );

    if (!NT_SUCCESS(status)) {
        return op->FinalStatus = status;
    }

    //
    // Since ObOpenObjectByName is called without being passed
    // any object type information, we need to map the generic
    // bits in the DesiredAccess mask here.  We also need to save
    // the object's generic mapping in the access state structure
    // here, because this is the earliest opportunity we have
    // to do so.
    //

    RtlMapGenericMask( &AccessState->RemainingDesiredAccess,
                       &IoFileObjectType->TypeInfo.GenericMapping );

    RtlMapGenericMask( &AccessState->OriginalDesiredAccess,
                       &IoFileObjectType->TypeInfo.GenericMapping );

    SeSetAccessStateGenericMapping( AccessState, &IoFileObjectType->TypeInfo.GenericMapping );

    desiredAccess = AccessState->RemainingDesiredAccess;

    //
    // Compute the previous mode to be passed in to the privilege check
    //

    if (AccessMode != KernelMode || op->Options & IO_FORCE_ACCESS_CHECK) {
        modeForPrivilegeCheck = UserMode;
    } else {
        modeForPrivilegeCheck = KernelMode;
    }

    IopCheckBackupRestorePrivilege( AccessState,
                                    &op->CreateOptions,
                                    modeForPrivilegeCheck,
                                    op->Disposition
                                    );

    //
    // If this is not the first time through here for this object, and the
    // object itself is being opened, then the desired access must also
    // include the previously granted access from the last pass.  Likewise,
    // if the privileges have been checked already, then this is another
    // pass through for a file, so add in the previously granted access.
    //

    if ((op->Override && !RemainingName->Length) ||
        AccessState->Flags & SE_BACKUP_PRIVILEGES_CHECKED) {
        desiredAccess |= AccessState->PreviouslyGrantedAccess;
    }

    //
    // If its a filesystem volume open and we are doing a relative open to it
    // then do the access check. Note that relative opens can be nested and we propagate
    // the fact that the relative open is for a volume using the FO_VOLUME_OPEN flag.
    //

    if (op->RelatedFileObject) {
        if ((op->RelatedFileObject->Flags & FO_VOLUME_OPEN) && RemainingName->Length == 0) {
            relativeVolumeOpen = TRUE;
        }
    }

    //
    // Now determine what type of security check should be made.  This is
    // based on whether the remaining name string is null.  If it is null,
    // then the device itself is being opened, so a full security check is
    // performed.  Otherwise, only a check to ensure that the caller can
    // traverse the device object is made.  Note that these checks are only
    // made if the caller's mode is user, or if access checking is being
    // forced.  Note also that if an access check was already made on the
    // device itself, and this code is being executed again because of a
    // reparse, then the access check need not be made the second time
    // around.
    //


    if ((AccessMode != KernelMode || op->Options & IO_FORCE_ACCESS_CHECK) &&
        (!op->RelatedFileObject || relativeVolumeOpen) &&
        !op->Override) {

        BOOLEAN subjectContextLocked = FALSE;
        BOOLEAN accessGranted;
        ACCESS_MASK grantedAccess;

        //
        // The caller's mode is either user or access checking is being
        // forced.  Perform the appropriate access check on the device
        // object.
        //

        if (!RemainingName->Length) {

            UNICODE_STRING nameString;
            PPRIVILEGE_SET privileges = NULL;

            //
            // The device itself is being opened.  Make a full security check
            // to ensure that the caller has the appropriate access.
            //

            KeEnterCriticalRegion( );
            ExAcquireResourceShared( &IopSecurityResource, TRUE );

            SeLockSubjectContext( &AccessState->SubjectSecurityContext );
            subjectContextLocked = TRUE;

            accessGranted = SeAccessCheck( parseDeviceObject->SecurityDescriptor,
                                           &AccessState->SubjectSecurityContext,
                                           subjectContextLocked,
                                           desiredAccess,
                                           0,
                                           &privileges,
                                           &IoFileObjectType->TypeInfo.GenericMapping,
                                           UserMode,
                                           &grantedAccess,
                                           &status );

            if (privileges) {
                (VOID) SeAppendPrivileges( AccessState,
                                           privileges );
                SeFreePrivileges( privileges );
            }

            if (accessGranted) {
                AccessState->PreviouslyGrantedAccess |= grantedAccess;
                AccessState->RemainingDesiredAccess &= ~( grantedAccess | MAXIMUM_ALLOWED );
                op->Override = TRUE;
            }

            nameString.Length = 8;
            nameString.MaximumLength = 8;
            nameString.Buffer = L"File";

            SeOpenObjectAuditAlarm( &nameString,
                                    parseDeviceObject,
                                    CompleteName,
                                    parseDeviceObject->SecurityDescriptor,
                                    AccessState,
                                    FALSE,
                                    accessGranted,
                                    UserMode,
                                    &AccessState->GenerateOnClose );

            ExReleaseResource( &IopSecurityResource );
            KeLeaveCriticalRegion();

        } else {

            //
            // The device is not being opened, rather, a file on the device
            // is being opened or created.  Therefore, only perform a check
            // here for traverse access to the device.
            //

            //
            // First determine if we have to perform traverse checking at all.
            // Traverse checking only needs to be done if the device being
            // traversed is a disk, or if the caller does not already have
            // traverse checking privilege.  Note that the former case is so
            // that an administrator can turn off access to the "system
            // partition", or someone would be able to install a trojan horse
            // into the system by simply replacing one of the files there with
            // something of their own.
            //

            if (!(AccessState->Flags & TOKEN_HAS_TRAVERSE_PRIVILEGE) ||
                parseDeviceObject->DeviceType == FILE_DEVICE_DISK ||
                parseDeviceObject->DeviceType == FILE_DEVICE_CD_ROM ) {

                KeEnterCriticalRegion( );
                ExAcquireResourceShared( &IopSecurityResource, TRUE );

                //
                // If the token is restricted we need to do the full
                // access check.
                //

                if ((AccessState->Flags & TOKEN_IS_RESTRICTED) == 0) {
                    accessGranted = SeFastTraverseCheck( parseDeviceObject->SecurityDescriptor,
                                                         FILE_TRAVERSE,
                                                         UserMode );
                } else {
                    accessGranted = FALSE;
                }

                if (!accessGranted) {

                    PPRIVILEGE_SET privileges = NULL;

                    //
                    // The caller was not granted traverse access through the
                    // normal fast path lookup.  Perform a full-blown access
                    // check to determine whether some other ACE allows traverse
                    // access.
                    //

                    SeLockSubjectContext( &AccessState->SubjectSecurityContext );

                    subjectContextLocked = TRUE;

                    accessGranted = SeAccessCheck( parseDeviceObject->SecurityDescriptor,
                                                   &AccessState->SubjectSecurityContext,
                                                   subjectContextLocked,
                                                   FILE_TRAVERSE,
                                                   0,
                                                   &privileges,
                                                   &IoFileObjectType->TypeInfo.GenericMapping,
                                                   UserMode,
                                                   &grantedAccess,
                                                   &status );

                    if (privileges) {

                        (VOID) SeAppendPrivileges( AccessState,
                                                   privileges );
                        SeFreePrivileges( privileges );
                    }

                }

                //
                // Perform the traverse audit alarm if necessary.
                //

                SeTraverseAuditAlarm( &AccessState->OperationID,
                                      parseDeviceObject,
                                      parseDeviceObject->SecurityDescriptor,
                                      &AccessState->SubjectSecurityContext,
                                      subjectContextLocked,
                                      FILE_TRAVERSE,
                                      (PPRIVILEGE_SET) NULL,
                                      accessGranted,
                                      UserMode );
                ExReleaseResource( &IopSecurityResource );
                KeLeaveCriticalRegion();

            } else {

                    accessGranted = TRUE;
            }
        }

        //
        // Unlock the subject's security context so that it can be changed,
        // if it was locked.
        //

        if (subjectContextLocked) {
            SeUnlockSubjectContext( &AccessState->SubjectSecurityContext );
        }

        //
        // Finally, determine whether or not access was granted to the device.
        // If not, clean everything up and get out now without even invoking
        // the device driver.
        //

        if (!accessGranted) {

            IopDecrementDeviceObjectRef( parseDeviceObject, FALSE );
            return STATUS_ACCESS_DENIED;
        }

    }

    realFileObjectRequired = !(op->QueryOnly || op->DeleteOnly);

    if (RemainingName->Length == 0 &&
        op->RelatedFileObject == NULL &&
        ((desiredAccess & ~(SYNCHRONIZE |
                            FILE_READ_ATTRIBUTES |
                            READ_CONTROL |
                            ACCESS_SYSTEM_SECURITY |
                            WRITE_OWNER |
                            WRITE_DAC)) == 0) &&
        realFileObjectRequired) {

        //
        // If the name of the object being opened is just the name of the
        // device itself, and there is no related file object, and the caller
        // is opening the device for only read attributes access, then this
        // device will not be mounted.  This allows applications to obtain
        // attributes about the device without actually mounting it.
        //
        // Note that if this *is* a direct device open, then the normal path
        // through the I/O system and drivers may never be used, even if
        // the device appears to be mounted.  This is because the user may
        // remove the media from the drive (even though it is mounted), and
        // now attempting to determine what type of drive it is will still
        // fail, this time very hard, because a whole mount process is now
        // required, thus defeating this feature.
        //

        directDeviceOpen = TRUE;

    } else {

        //
        // Otherwise, this is a normal open of a file, directory, device, or
        // volume.
        //

        directDeviceOpen = FALSE;
    }

    //
    // There are now five different cases.  These are as follows:
    //
    //    1)  This is a relative open, in which case we want to send the
    //        request to then same device that opened the relative file object.
    //
    //    2)  The VPB pointer in the device object is NULL.  This means that
    //        this device does not support a file system.  This includes
    //        devices such as terminals, etc.
    //
    //    3)  The VPB pointer in the device object is not NULL and:
    //
    //        a)  The VPB is "blank".  That is, the VPB has never been filled
    //            in, which means that the device has never been mounted.
    //
    //        b)  The VPB is non-blank, but the verify flag on the device is
    //            set, indicating that the door to the drive may have been
    //            opened and the media may therefore have been changed.
    //
    //        c)  The VPB is non-blank and the verify flag is not set.
    //
    //        Both of the latter are not explicitly checked for, as #c is
    //        the normal case, and #b is the responsibility of the file
    //        system to check.
    //

    //
    //  If this is a file system that supports volumes, vpbRefCount will
    //  be filled in to point to the reference count in the Vpb.  Error
    //  exits paths later on key off this value to see if they should
    //  decrement the ref count.  Note that a direct device open does not
    //  make it to the file system, so no increment is needed, and no
    //  decrement will be performed in objsup.c IopDeleteFile().
    //

    vpb = NULL;

    //
    // If the related open was a direct device open then we should go through the full mount 
    // path for this open as this may not be a direct device open.
    //
    if (op->RelatedFileObject && (!(op->RelatedFileObject->Flags & FO_DIRECT_DEVICE_OPEN))) {

        deviceObject = (PDEVICE_OBJECT)ParseObject;

        if (op->RelatedFileObject->Vpb) {

            vpb = op->RelatedFileObject->Vpb;

            //
            // Synchronize here with the file system to make sure that
            // volumes don't go away while en route to the FS.
            //

            ExInterlockedAddUlong( &vpb->ReferenceCount, 1, &IopVpbSpinLock );
        }

    } else {

        deviceObject = parseDeviceObject;

        if (parseDeviceObject->Vpb && !directDeviceOpen) {
            vpb = IopCheckVpbMounted( op,
                                      parseDeviceObject,
                                      RemainingName,
                                      &status );
            if ( !vpb ) {
                return status;
            }

            //
            // Set the address of the device object associated with the VPB.
            //

            deviceObject = vpb->DeviceObject;
        }

        //
        // Walk the attached device list.
        //

        if (deviceObject->AttachedDevice) {
            deviceObject = IoGetAttachedDevice( deviceObject );
        }
    }

    //
    //  If the driver says that the IO manager should do the access checks, lets do it here.
    //  We do the check against the parse device object as that device object has a name
    //  and we can set an ACL against it.
    //  We only worry about related opens of devices as the other case is taken care of in the
    //  filesystem.
    //
    if ((deviceObject->Characteristics & FILE_DEVICE_SECURE_OPEN) &&
        (op->RelatedFileObject || RemainingName->Length) &&  (!relativeVolumeOpen)) {

        BOOLEAN subjectContextLocked = FALSE;
        BOOLEAN accessGranted;
        ACCESS_MASK grantedAccess;
        UNICODE_STRING nameString;
        PPRIVILEGE_SET privileges = NULL;

        //
        // If the device wants to ensure secure opens then lets check the two
        // cases which were skipped earlier. These cases are if its a relative
        // open or if there are trailing names.
        //

        KeEnterCriticalRegion( );
        ExAcquireResourceShared( &IopSecurityResource, TRUE );

        SeLockSubjectContext( &AccessState->SubjectSecurityContext );
        subjectContextLocked = TRUE;

        accessGranted = SeAccessCheck( parseDeviceObject->SecurityDescriptor,
                                       &AccessState->SubjectSecurityContext,
                                       subjectContextLocked,
                                       desiredAccess,
                                       0,
                                       &privileges,
                                       &IoFileObjectType->TypeInfo.GenericMapping,
                                       UserMode,
                                       &grantedAccess,
                                       &status );

        if (privileges) {
            (VOID) SeAppendPrivileges( AccessState,
                                       privileges );
            SeFreePrivileges( privileges );
        }

        if (accessGranted) {
            AccessState->PreviouslyGrantedAccess |= grantedAccess;
            AccessState->RemainingDesiredAccess &= ~( grantedAccess | MAXIMUM_ALLOWED );
        }

        nameString.Length = 8;
        nameString.MaximumLength = 8;
        nameString.Buffer = L"File";

        SeOpenObjectAuditAlarm( &nameString,
                                deviceObject,
                                CompleteName,
                                parseDeviceObject->SecurityDescriptor,
                                AccessState,
                                FALSE,
                                accessGranted,
                                UserMode,
                                &AccessState->GenerateOnClose );

        SeUnlockSubjectContext( &AccessState->SubjectSecurityContext );
        ExReleaseResource( &IopSecurityResource );
        KeLeaveCriticalRegion();

        if (!accessGranted) {
            IopDecrementDeviceObjectRef( parseDeviceObject, FALSE );

            if (vpb) {
                IopDereferenceVpbAndFree(vpb);
            }
            return STATUS_ACCESS_DENIED;
        }
    }

    //
    // Allocate and fill in the I/O Request Packet (IRP) to use in interfacing
    // to the driver.  The allocation is done using an exception handler in
    // case the caller does not have enough quota to allocate the packet.
    //

    irp = IopAllocateIrp( deviceObject->StackSize, TRUE );
    if (!irp) {

        //
        // An IRP could not be allocated.  Cleanup and return an appropriate
        // error status code.
        //

        IopDecrementDeviceObjectRef( parseDeviceObject, FALSE );

        if (vpb) {
            IopDereferenceVpbAndFree(vpb);
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->RequestorMode = AccessMode;
    irp->Flags = IRP_CREATE_OPERATION | IRP_SYNCHRONOUS_API | IRP_DEFER_IO_COMPLETION;

    securityContext.SecurityQos = SecurityQos;
    securityContext.AccessState = AccessState;
    securityContext.DesiredAccess = desiredAccess;
    securityContext.FullCreateOptions = op->CreateOptions;

    //
    // Get a pointer to the stack location for the first driver.  This is where
    // the original function codes and parameters are passed.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->Control = 0;

    if (op->CreateFileType == CreateFileTypeNone) {

        //
        // This is a normal file open or create function.
        //

        irpSp->MajorFunction = IRP_MJ_CREATE;
        irpSp->Parameters.Create.EaLength = op->EaLength;
        irpSp->Flags = (UCHAR) op->Options;
        if (!(Attributes & OBJ_CASE_INSENSITIVE)) {
            irpSp->Flags |= SL_CASE_SENSITIVE;
        }

    } else if (op->CreateFileType == CreateFileTypeNamedPipe) {

        //
        // A named pipe is being created.
        //

        irpSp->MajorFunction = IRP_MJ_CREATE_NAMED_PIPE;
        irpSp->Parameters.CreatePipe.Parameters = op->ExtraCreateParameters;

    } else {

        //
        // A mailslot is being created.
        //

        irpSp->MajorFunction = IRP_MJ_CREATE_MAILSLOT;
        irpSp->Parameters.CreateMailslot.Parameters = op->ExtraCreateParameters;
    }

    //
    // Also fill in the NtCreateFile service's caller's parameters.
    //

    irp->Overlay.AllocationSize = op->AllocationSize;
    irp->AssociatedIrp.SystemBuffer = op->EaBuffer;
    irpSp->Parameters.Create.Options = (op->Disposition << 24) | (op->CreateOptions & 0x00ffffff);
    irpSp->Parameters.Create.FileAttributes = op->FileAttributes;
    irpSp->Parameters.Create.ShareAccess = op->ShareAccess;
    irpSp->Parameters.Create.SecurityContext = &securityContext;

    //
    // Fill in local parameters so this routine can determine when the I/O is
    // finished, and the normal I/O completion code will not get any errors.
    //

    irp->UserIosb = &ioStatus;
    irp->MdlAddress = (PMDL) NULL;
    irp->PendingReturned = FALSE;
    irp->Cancel = FALSE;
    irp->UserEvent = (PKEVENT) NULL;
    irp->CancelRoutine = (PDRIVER_CANCEL) NULL;
    irp->Tail.Overlay.AuxiliaryBuffer = (PVOID) NULL;

    //
    // Allocate and initialize the file object that will be used in dealing
    // with the device for the remainder of this session with the user.  How
    // the file object is allocated is based on whether or not a real file
    // object is actually required.  It is not required for the query and
    // delete only operations.
    //

    if (realFileObjectRequired) {

        OBJECT_ATTRIBUTES objectAttributes;

        //
        // A real, full-blown file object is actually required.
        //

        InitializeObjectAttributes( &objectAttributes,
                                    (PUNICODE_STRING) NULL,
                                    Attributes,
                                    (HANDLE) NULL,
                                    (PSECURITY_DESCRIPTOR) NULL
                                  );

        status = ObCreateObject( KernelMode,
                                 IoFileObjectType,
                                 &objectAttributes,
                                 AccessMode,
                                 (PVOID) NULL,
                                 (ULONG) sizeof( FILE_OBJECT ),
                                 0,
                                 0,
                                 (PVOID *) &fileObject );

        if (!NT_SUCCESS( status )) {
            IoFreeIrp( irp );

            IopDecrementDeviceObjectRef( parseDeviceObject, FALSE );

            if (vpb) {
               IopDereferenceVpbAndFree(vpb);
            }
            return op->FinalStatus = status;
        }

        RtlZeroMemory( fileObject, sizeof( FILE_OBJECT ) );
        fileObject->Type = IO_TYPE_FILE;
        fileObject->Size = sizeof( FILE_OBJECT );
        fileObject->RelatedFileObject = op->RelatedFileObject;
        if (op->CreateOptions & (FILE_SYNCHRONOUS_IO_ALERT | FILE_SYNCHRONOUS_IO_NONALERT)) {
            fileObject->Flags = FO_SYNCHRONOUS_IO;
            if (op->CreateOptions & FILE_SYNCHRONOUS_IO_ALERT) {
                fileObject->Flags |= FO_ALERTABLE_IO;
            }
        }

        //
        // Now fill in the file object as best is possible at this point and set
        // a pointer to it in the IRP so everyone else can find it.
        //

        if (fileObject->Flags & FO_SYNCHRONOUS_IO) {
            KeInitializeEvent( &fileObject->Lock, SynchronizationEvent, FALSE );
            fileObject->Waiters = 0;
            fileObject->CurrentByteOffset.QuadPart = 0;
        }
        if (op->CreateOptions & FILE_NO_INTERMEDIATE_BUFFERING) {
            fileObject->Flags |= FO_NO_INTERMEDIATE_BUFFERING;
        }
        if (op->CreateOptions & FILE_WRITE_THROUGH) {
            fileObject->Flags |= FO_WRITE_THROUGH;
        }
        if (op->CreateOptions & FILE_SEQUENTIAL_ONLY) {
            fileObject->Flags |= FO_SEQUENTIAL_ONLY;
        }
        if (op->CreateOptions & FILE_RANDOM_ACCESS) {
            fileObject->Flags |= FO_RANDOM_ACCESS;
        }

    } else {

        //
        // This is either a quick delete or query operation.  For these cases,
        // it is possible to optimize the Object Manager out of the picture by
        // simply putting together something that "looks" like a file object,
        // and then operating on it.
        //

        localFileObject = op->LocalFileObject;
        RtlZeroMemory( localFileObject, sizeof( DUMMY_FILE_OBJECT ) );
        fileObject = (PFILE_OBJECT) &localFileObject->ObjectHeader.Body;
        localFileObject->ObjectHeader.Type = IoFileObjectType;
        localFileObject->ObjectHeader.PointerCount = 1;
    }

    if (directDeviceOpen) {
        fileObject->Flags |= FO_DIRECT_DEVICE_OPEN;
    }
    if (!(Attributes & OBJ_CASE_INSENSITIVE)) {
        fileObject->Flags |= FO_OPENED_CASE_SENSITIVE;
    }

    fileObject->Type = IO_TYPE_FILE;
    fileObject->Size = sizeof( FILE_OBJECT );
    fileObject->RelatedFileObject = op->RelatedFileObject;
    fileObject->DeviceObject = parseDeviceObject;

    irp->Tail.Overlay.OriginalFileObject = fileObject;
    irpSp->FileObject = fileObject;

    //
    // Allocate a file name string buffer which is large enough to contain
    // the entire remaining name string and initialize the maximum length.
    //

    if (RemainingName->Length) {
        fileObject->FileName.MaximumLength = RoundNameSize( RemainingName->Length );
        fileObject->FileName.Buffer = ExAllocatePoolWithTag( PagedPool,
                                                             fileObject->FileName.MaximumLength,
                                                             'mNoI' );
        if (!fileObject->FileName.Buffer) {
            IoFreeIrp( irp );

            IopDecrementDeviceObjectRef( parseDeviceObject, FALSE );

            if (vpb) {
               IopDereferenceVpbAndFree(vpb);
            }
            fileObject->DeviceObject = (PDEVICE_OBJECT) NULL;
            if (realFileObjectRequired) {
                ObDereferenceObject( fileObject );
            }
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    //
    // Now copy the name string into the file object from the remaining name
    // that is being reparsed.  If the driver decides to reparse, then it must
    // replace this name.
    //

    RtlCopyUnicodeString( &fileObject->FileName, RemainingName );

    //
    // Before invoking the driver's open routine, check to see whether or not
    // this is a fast network attributes query and, if so, and the driver
    // implements the function, attempt to call it here.
    //

    if (op->QueryOnly) {
        PFAST_IO_DISPATCH fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;
        BOOLEAN result;

        if (fastIoDispatch &&
            fastIoDispatch->SizeOfFastIoDispatch > FIELD_OFFSET( FAST_IO_DISPATCH, FastIoQueryOpen ) &&
            fastIoDispatch->FastIoQueryOpen) {

            IoSetNextIrpStackLocation( irp );
            irpSp->DeviceObject = deviceObject;
            result = (fastIoDispatch->FastIoQueryOpen)( irp,
                                                        op->NetworkInformation,
                                                        deviceObject );
            if (result) {
                op->FinalStatus = irp->IoStatus.Status;
                op->Information = irp->IoStatus.Information;

                //
                // The operation worked, so simply dereference and free the
                // resources acquired up to this point.
                //

                if ((op->FinalStatus == STATUS_REPARSE) &&
                    irp->Tail.Overlay.AuxiliaryBuffer) {
                    ASSERT( op->Information > IO_REPARSE_TAG_RESERVED_ONE );
                    ExFreePool( irp->Tail.Overlay.AuxiliaryBuffer );
                    irp->Tail.Overlay.AuxiliaryBuffer = NULL;
                    op->RelatedFileObject = (PFILE_OBJECT) NULL;
                }

                if (fileObject->FileName.Length) {
                    ExFreePool( fileObject->FileName.Buffer );
                }

                IopDecrementDeviceObjectRef( parseDeviceObject, FALSE );

                if (vpb) {
                    IopDereferenceVpbAndFree(vpb);
                }

#if DBG
                irp->CurrentLocation = irp->StackCount + 2;
#endif // DBG

                IoFreeIrp( irp );

                //
                // Finally, indicate that the parse routine was actually
                // invoked and that the information returned herein can be
                // used.
                //

                op->ParseCheck = OPEN_PACKET_PATTERN;
                status = STATUS_SUCCESS;

                if (!op->FullAttributes) {
                    try {
                        op->BasicInformation->FileAttributes = op->NetworkInformation->FileAttributes;
                    } except(EXCEPTION_EXECUTE_HANDLER) {
                        status = GetExceptionCode();
                    }
                }

                return status;

            } else {

                //
                // The fast I/O operation did not work, so take the longer
                // route.
                //

                irp->Tail.Overlay.CurrentStackLocation++;
                irp->CurrentLocation++;
            }
        }
    }

    //
    // Finally, initialize the file object's event to the Not Signaled state
    // and remember that a file object was created.
    //

    KeInitializeEvent( &fileObject->Event, NotificationEvent, FALSE );
    op->FileObject = fileObject;

    //
    // Insert the packet at the head of the IRP list for the thread.
    //

    IopQueueThreadIrp( irp );

    //
    // Now invoke the driver itself to open the file.
    //

    status = IoCallDriver( deviceObject, irp );

    //
    // One of four things may have happened when the driver was invoked:
    //
    //    1.  The I/O operation is pending (Status == STATUS_PENDING).  This can
    //        occur on devices which need to perform some sort of device
    //        manipulation (such as opening a file for a file system).
    //
    //    2.  The driver returned an error (Status < 0). This occurs when either
    //        a supplied parameter was in error, or the device or file system
    //        incurred or discovered an error.
    //
    //    3.  The operation ended in a reparse (Status == STATUS_REPARSE).  This
    //        occurs when a file system opens the file, only to discover that it
    //        represents a symbolic link.
    //
    //    4.  The operation is complete and was successful (Status ==
    //        STATUS_SUCCESS).  Note that for this case the only action is to
    //        return a pointer to the file object.
    //

    if (status == STATUS_PENDING) {

        (VOID) KeWaitForSingleObject( &fileObject->Event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER) NULL );
        status = ioStatus.Status;

    } else {

        //
        // The I/O operation was completed without returning a status of
        // pending.  This means that at this point, the IRP has not been
        // fully completed.  Complete it now.
        //

        PKNORMAL_ROUTINE normalRoutine;
        PVOID normalContext;
        KIRQL irql;

        ASSERT( !irp->PendingReturned );
        ASSERT( !irp->MdlAddress );

        //
        // In the case of name junctions do the transmogrify work.
        //

        if (irp->IoStatus.Status == STATUS_REPARSE &&
            irp->IoStatus.Information == IO_REPARSE_TAG_MOUNT_POINT ) {

            PREPARSE_DATA_BUFFER reparseBuffer = NULL;

            ASSERT ( irp->Tail.Overlay.AuxiliaryBuffer != NULL );

            reparseBuffer = (PREPARSE_DATA_BUFFER) irp->Tail.Overlay.AuxiliaryBuffer;

            ASSERT( reparseBuffer->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT );
            ASSERT( reparseBuffer->ReparseDataLength < MAXIMUM_REPARSE_DATA_BUFFER_SIZE );
            ASSERT( reparseBuffer->Reserved < MAXIMUM_REPARSE_DATA_BUFFER_SIZE );

            IopDoNameTransmogrify( irp,
                                   fileObject,
                                   reparseBuffer );
        }

        //
        // Now finish up the request.
        //

        KeRaiseIrql( APC_LEVEL, &irql );

        //
        // Note that normally the system would simply call IopCompleteRequest
        // here to complete the packet.  However, because this is a create
        // operation, several assumptions can be made that make it much faster
        // to perform the couple of operations that completing the request
        // would perform.  These include:  copying the I/O status block,
        // dequeueing the IRP and freeing it, and setting the file object's
        // event to the signalled state.  The latter is done here by hand,
        // since it is known that it is not possible for any thread to be
        // waiting on the event.
        //

        ioStatus = irp->IoStatus;
        status = ioStatus.Status;

        fileObject->Event.Header.SignalState = 1;

        IopDequeueThreadIrp( irp );

        //
        // The SystemBuffer is in some cases used by the driver, and
        // needs to be freed if present.
        //

        if ((irp->Flags & IRP_BUFFERED_IO) && (irp->Flags & IRP_DEALLOCATE_BUFFER)) {
            ExFreePool(irp->AssociatedIrp.SystemBuffer);
        }

        IoFreeIrp( irp );

        KeLowerIrql( irql );
    }

    //
    // Copy the information field of the I/O status block back to the
    // original caller in case it is required.
    //

    op->Information = ioStatus.Information;

    if (!NT_SUCCESS( status )) {
        int openCancelled;

        //
        // The operation ended in an error.  Kill the file object, dereference
        // the device object, and return a null pointer.
        //

        if (fileObject->FileName.Length) {
            ExFreePool( fileObject->FileName.Buffer );
            fileObject->FileName.Length = 0;
        }

        fileObject->DeviceObject = (PDEVICE_OBJECT) NULL;

        openCancelled = (fileObject->Flags & FO_FILE_OPEN_CANCELLED);

        if (realFileObjectRequired) {
            ObDereferenceObject( fileObject );
        }
        op->FileObject = (PFILE_OBJECT) NULL;

        IopDecrementDeviceObjectRef( parseDeviceObject, FALSE );

        if ((!openCancelled) && (vpb )) {
            IopDereferenceVpbAndFree(vpb);
        }

        return op->FinalStatus = status;

    } else if (status == STATUS_REPARSE) {

        //
        // The operation resulted in a reparse.  This means that the file
        // name in the file object is the new name to be looked up. Replace
        // the complete name string with the new name and return STATUS_REPARSE
        // so the object manager knows to start over again.  Note, however,
        // that the file name buffer in the file object itself is kept intact
        // so that it can be reused when coming back here again.
        //
        // A reparse status may also have been returned from the file system if
        // the volume that was in a drive needed to have been verified, but
        // the verification failed, and a new volume was mounted.  In this
        // case, everything starts over again using the new volume.
        //

        ASSERT( IO_REPARSE == IO_REPARSE_TAG_RESERVED_ZERO );

        if ((ioStatus.Information == IO_REPARSE) ||
            (ioStatus.Information == IO_REPARSE_TAG_MOUNT_POINT)) {

            //
            // If the complete name buffer isn't large enough, reallocate it.
            //

            if (CompleteName->MaximumLength < fileObject->FileName.Length) {

                PVOID buffer;

                buffer = ExAllocatePoolWithTag( PagedPool,
                                                fileObject->FileName.Length,
                                                'cFoI' );
                if (!buffer) {
                    return op->FinalStatus = STATUS_INSUFFICIENT_RESOURCES;
                } else {
                    if (CompleteName->Buffer) {
                        ExFreePool( CompleteName->Buffer );
                    }
                    CompleteName->Buffer = buffer;
                    CompleteName->MaximumLength = fileObject->FileName.Length;
                }
            }

            RtlCopyUnicodeString( CompleteName, &fileObject->FileName );

            //
            // For NTFS directory junction points we NULL the RelatedFileObject.
            // If the prior call was a relative open, the subsequent one will
            // not be.
            //

            if (ioStatus.Information == IO_REPARSE_TAG_MOUNT_POINT) {

                op->RelatedFileObject = (PFILE_OBJECT) NULL;
            }
        }

        //
        // Kill the file object, dereference the device object, and return a
        // null pointer.
        //

        if (fileObject->FileName.Length) {
            ExFreePool( fileObject->FileName.Buffer );
            fileObject->FileName.Length = 0;
        }

        fileObject->DeviceObject = (PDEVICE_OBJECT) NULL;

        if (realFileObjectRequired) {
            ObDereferenceObject( fileObject );
        }
        op->FileObject = (PFILE_OBJECT) NULL;

        IopDecrementDeviceObjectRef( parseDeviceObject, FALSE );

        if (vpb) {
            IopDereferenceVpbAndFree(vpb);
        }

        ASSERT( IO_REMOUNT == IO_REPARSE_TAG_RESERVED_ONE );

        if (ioStatus.Information == IO_REPARSE_TAG_RESERVED_ONE) {

            //
            // If we are reparsing to verify a volume, restart the reparse
            // by attempting to parse the device once again.  Note that it
            // would be best to simply recurse, but it's not possible since
            // there is a limited amount of stack available to kernel mode
            // and a limit needs to be enforced for the number of times that
            // verify reparse can occur.
            //

            if (++retryCount > IO_MAX_REMOUNT_REPARSE_ATTEMPTS) {

                return STATUS_UNSUCCESSFUL;
            }
            goto reparse_loop;

        } else {

            //
            // Really reparsing a symbolic link, so go back to the object
            // manager so it can begin the parse from the top.
            //

            op->RelatedFileObject = (PFILE_OBJECT) NULL;
            return STATUS_REPARSE;
        }

    } else {

        //
        // The operation was successful. The first thing to do is to see if
        // the device that processed the open also opened the file. If
        // not, we need to adjust the vpb reference counts. Then, if this is
        // not a query or a delete, but rather a normal open/create, return
        // the address of the FileObject to the caller and set the
        // information returned in the original requestor's I/O status block.
        // Also set the value of the parse check field in the open packet to
        // a value which will let the caller know that this routine was
        // successful in creating the file object. Finally, return the status
        // of the operation to the caller.
        //

        PDEVICE_OBJECT deviceObjectThatOpenedFile;

        deviceObjectThatOpenedFile = IoGetRelatedDeviceObject(fileObject);
        if (deviceObject != deviceObjectThatOpenedFile) {
            //
            // The device that opened the related file is not the one
            // that opened this file. So, readjust the vpb reference
            // counts.
            if (vpb) {
                IopDereferenceVpbAndFree(vpb);
            }
            vpb = fileObject->Vpb;
            if (vpb) {
                ExInterlockedAddUlong(
                    &vpb->ReferenceCount, 1, &IopVpbSpinLock );
            }
        }

        if (realFileObjectRequired) {

            *Object = fileObject;
            op->ParseCheck = OPEN_PACKET_PATTERN;

            //
            // Add a reference so the file object cannot go away before
            // the create routine gets chance to flag the object for handle
            // create.
            //

            ObReferenceObject( fileObject );

            //
            // If the filename length is zero and its not a relative open or
            // its a relative open to a volume open then set the volume open flag.
            // Also set it only for filesystem device object volume.
            //
            if ((!fileObject->RelatedFileObject || fileObject->RelatedFileObject->Flags & FO_VOLUME_OPEN) &&
                (!fileObject->FileName.Length)) {
                switch (deviceObjectThatOpenedFile->DeviceType) {
                case FILE_DEVICE_DISK_FILE_SYSTEM:
                case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
                case FILE_DEVICE_TAPE_FILE_SYSTEM:
                case FILE_DEVICE_FILE_SYSTEM:

                    fileObject->Flags |= FO_VOLUME_OPEN;
                    break;

                default:
                    break;
                }
            }

            return op->FinalStatus = ioStatus.Status;

        } else {

            //
            // This is either a quick query or delete operation.  Determine
            // which it is and quickly perform the operation.
            //

            if (op->QueryOnly) {
                PFAST_IO_DISPATCH fastIoDispatch;
                BOOLEAN queryResult = FALSE;

                fastIoDispatch = deviceObjectThatOpenedFile->DriverObject->FastIoDispatch;

                if (!op->FullAttributes) {
                    PFILE_BASIC_INFORMATION basicInfo = NULL;

                    //
                    // This is a simple FAT file attribute query.  Attempt to
                    // obtain the basic information about the file.
                    //

                    try {

                        if (fastIoDispatch && fastIoDispatch->FastIoQueryBasicInfo) {
                            queryResult = fastIoDispatch->FastIoQueryBasicInfo(
                                            fileObject,
                                            TRUE,
                                            op->BasicInformation,
                                            &ioStatus,
                                            deviceObjectThatOpenedFile
                                            );
                        }
                        if (!queryResult) {
                            ULONG returnedLength;

                            basicInfo = ExAllocatePool( NonPagedPool,
                                                        sizeof( FILE_BASIC_INFORMATION ) );
                            if (basicInfo) {
                                status = IoQueryFileInformation(
                                            fileObject,
                                            FileBasicInformation,
                                            sizeof( FILE_BASIC_INFORMATION ),
                                            basicInfo,
                                            &returnedLength
                                            );
                                if (NT_SUCCESS( status )) {
                                    RtlCopyMemory( op->BasicInformation,
                                                   basicInfo,
                                                   returnedLength );
                                }
                                ExFreePool( basicInfo );
                            } else {
                                status = STATUS_INSUFFICIENT_RESOURCES;
                            }
                        } else {
                            status = ioStatus.Status;
                        }
                    } except(EXCEPTION_EXECUTE_HANDLER) {
                        if (basicInfo) {
                            ExFreePool( basicInfo );
                        }
                        status = GetExceptionCode();
                    }

                } else {

                    //
                    // This is a full attribute query.  Attempt to obtain the
                    // full network attributes for the file.  This includes
                    // both the basic and standard information about the
                    // file.  Try the fast path first, if it exists.
                    //

                    if (fastIoDispatch &&
                        fastIoDispatch->SizeOfFastIoDispatch > FIELD_OFFSET( FAST_IO_DISPATCH, FastIoQueryNetworkOpenInfo ) &&
                        fastIoDispatch->FastIoQueryNetworkOpenInfo) {
                        queryResult = fastIoDispatch->FastIoQueryNetworkOpenInfo(
                                        fileObject,
                                        TRUE,
                                        op->NetworkInformation,
                                        &ioStatus,
                                        deviceObjectThatOpenedFile
                                        );
                    }
                    if (!queryResult) {
                        ULONG returnedLength;

                        //
                        // Either the fast dispatch routine did not exist, or
                        // it simply wasn't callable at this time.  Attempt to
                        // obtain all of the information at once via an IRP-
                        // based call.
                        //

                        status = IoQueryFileInformation(
                                    fileObject,
                                    FileNetworkOpenInformation,
                                    sizeof( FILE_NETWORK_OPEN_INFORMATION ),
                                    op->NetworkInformation,
                                    &returnedLength
                                    );

                        if (!NT_SUCCESS( status )) {
                            if (status == STATUS_INVALID_PARAMETER ||
                                status == STATUS_NOT_IMPLEMENTED) {
                                FILE_BASIC_INFORMATION basicInfo;
                                FILE_STANDARD_INFORMATION stdInfo;

                                //
                                // The IRP-based call did not work either, so
                                // simply try to obtain the information by
                                // doing IRP-based queries for the basic and
                                // standard information and piecing together
                                // the results into the caller's buffer.  Note
                                // that it might be possible to perform fast
                                // I/O operations to get the data, but it
                                // might also fail because of the above.  So
                                // simply query the information the long way.
                                //

                                status = IoQueryFileInformation(
                                            fileObject,
                                            FileBasicInformation,
                                            sizeof( FILE_BASIC_INFORMATION ),
                                            &basicInfo,
                                            &returnedLength
                                            );
                                if (NT_SUCCESS( status )) {
                                    status = IoQueryFileInformation(
                                                fileObject,
                                                FileStandardInformation,
                                                sizeof( FILE_STANDARD_INFORMATION ),
                                                &stdInfo,
                                                &returnedLength
                                                );
                                    if (NT_SUCCESS( status )) {
                                        COPY_ATTRIBUTES( op->NetworkInformation,
                                                         &basicInfo,
                                                         &stdInfo );
                                    }
                                }
                            }
                        }
                    }
                }

            } else {

                //
                // There is nothing to do for a quick delete since the caller
                // set the FILE_DELETE_ON_CLOSE CreateOption so it is already
                // set in the file system.
                //

                NOTHING;

            }

            op->ParseCheck = OPEN_PACKET_PATTERN;
            if (realFileObjectRequired) {
                ObDereferenceObject( fileObject );
            } else {
                IopDeleteFile( fileObject );
            }
            op->FileObject = (PFILE_OBJECT) NULL;

            op->FinalStatus = status;

            return status;
        }
    }
}

NTSTATUS
IopParseFile(
    IN PVOID ParseObject,
    IN PVOID ObjectType,
    IN PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE AccessMode,
    IN ULONG Attributes,
    IN OUT PUNICODE_STRING CompleteName,
    IN OUT PUNICODE_STRING RemainingName,
    IN OUT PVOID Context OPTIONAL,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
    OUT PVOID *Object
    )

/*++

Routine Description:

    This routine interfaces to the NT Object Manager.  It is invoked when
    the object system is given the name of an entity to create or open and is
    also given a handle to a directory file object that the operation is to be
    performed relative to.  This routine is specified as the parse routine for
    all file objects.

    This routine simply invokes the parse routine for the appropriate device
    that is associated with the file object.  It is the responsibility of that
    routine to perform the operation.

Arguments:

    ParseObject - Pointer to the file object that the name is to be opened or
        created relative to.

    ObjectType - Type of the object being opened.

    AccessState - Running security access state information for operation.

    AccessMode - Access mode of the original caller.

    Attributes - Attributes to be applied to the object.

    CompleteName - Complete name of the object.

    RemainingName - Remaining name of the object.

    Context - Pointer to an Open Packet (OP) from NtCreateFile service.

    SecurityQos - Supplies a pointer to the captured QOS information
        if available.

    Object - The address of a variable to receive the created file object, if
        any.

Return Value:

    The function return value is one of the following:

        a)  Success - This indicates that the function succeeded and the object
            parameter contains the address of the created file object.

        b)  Error - This indicates that the file was not found or created and
            no file object was created.

        c)  Reparse - This indicates that the remaining name string has been
            replaced by a new name that is to be parsed.

--*/

{
    PDEVICE_OBJECT deviceObject;
    POPEN_PACKET op;

    PAGED_CODE();

    //
    // Get the address of the Open Packet (OP).
    //

    op = (POPEN_PACKET) Context;

    //
    // Ensure that this routine is actually being invoked because someone is
    // attempting to open a device or a file through NtCreateFile.  This code
    // must be invoked from there (as opposed to some other random object
    // create or open routine).
    //

    if (op == NULL ||
        op->Type != IO_TYPE_OPEN_PACKET ||
        op->Size != sizeof( OPEN_PACKET )) {
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    //
    // Get a pointer to the device object for this file.
    //

    deviceObject = IoGetRelatedDeviceObject( (PFILE_OBJECT) ParseObject );

    //
    // Pass the related file object to the device object parse routine.
    //

    op->RelatedFileObject = (PFILE_OBJECT) ParseObject;

    //
    // Open or create the specified file.
    //

    return IopParseDevice( deviceObject,
                           ObjectType,
                           AccessState,
                           AccessMode,
                           Attributes,
                           CompleteName,
                           RemainingName,
                           Context,
                           SecurityQos,
                           Object );
}

NTSTATUS
IopQueryName(
    IN PVOID Object,
    IN BOOLEAN HasObjectName,
    OUT POBJECT_NAME_INFORMATION ObjectNameInfo,
    IN ULONG Length,
    OUT PULONG ReturnLength
    )

/*++

Routine Description:

    This function implements the query name procedure for the Object Manager
    for querying the names of file objects.

Arguments:

    Object - Pointer to the file object whose name is to be retrieved.

    HasObjectName - Indicates whether or not the object has a name.

    ObjectNameInfo - Buffer in which to return the name.

    Length - Specifies the length of the output buffer, in bytes.

    ReturnLength - Specifies the number of bytes actually returned in the
        output buffer.

Return Value:

    The function return value is the final status of the query operation.

--*/

{
    NTSTATUS status;
    ULONG lengthNeeded;
    PFILE_OBJECT fileObject;
    PUCHAR buffer;
    PWSTR p;
    POBJECT_NAME_INFORMATION deviceNameInfo;
    PFILE_NAME_INFORMATION fileNameInfo;
    ULONG length;

    UNREFERENCED_PARAMETER( HasObjectName );

    PAGED_CODE();

    ASSERT( FIELD_OFFSET( FILE_NAME_INFORMATION, FileName ) < sizeof( OBJECT_NAME_INFORMATION ) );

    //
    // Ensure that the size of the output buffer is at least the minimum
    // size required to include the basic object name information structure.
    //

    if (Length < sizeof( OBJECT_NAME_INFORMATION )) {
        return STATUS_INFO_LENGTH_MISMATCH;
        }

    //
    // Begin by allocating a buffer in which to build the name of the file.
    //

    buffer = ExAllocatePoolWithTag( PagedPool, Length, '  oI' );
    if (!buffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
        }

    try {

        //
        // Query the name of the device on which the file is open.
        //

        fileObject = (PFILE_OBJECT) Object;
        deviceNameInfo = (POBJECT_NAME_INFORMATION) buffer;

        status = ObQueryNameString( (PVOID) fileObject->DeviceObject,
                                    deviceNameInfo,
                                    Length,
                                    &lengthNeeded );
        if (!NT_SUCCESS( status )) {
            return status;
            }

        //
        // Ensure that there is enough room in the output buffer to return the
        // name and copy it.
        //

        RtlCopyMemory( ObjectNameInfo,
                       deviceNameInfo,
                       lengthNeeded > Length ? Length : lengthNeeded );
        p = (PWSTR) (ObjectNameInfo + 1);
        ObjectNameInfo->Name.Buffer = p;
        p = (PWSTR) ((PCHAR) p + deviceNameInfo->Name.Length);

        //
        // If the buffer is already full, then return.
        //

        if (lengthNeeded > Length) {
            return STATUS_BUFFER_OVERFLOW;
            }

        //
        // Reset the state for the buffer to obtain the filename portion of the
        // name and calculate the remaining length of the caller's buffer.  Note
        // that in the following calculations, there are two assumptions and
        // and dependencies:
        //
        //     1)  The above query of the device name's returned length needed
        //         include a NULL character which will be included at the end
        //         of the entire name.  This is included in the calculations
        //         although it does not appear to be included.
        //
        //     2)  The sizeof the object name information buffer is assumed
        //         (and guaranteed because it can never change) to be larger
        //         than the filename offset in a file name information buffer.
        //         Therefore it is known that the new length of the "buffer"
        //         variable can be set to the remaining length plus at least 4.
        //

        fileNameInfo = (PFILE_NAME_INFORMATION) buffer;
        length = Length - lengthNeeded;

        length += FIELD_OFFSET( FILE_NAME_INFORMATION, FileName );

        if (KeGetPreviousMode() == UserMode ||
            !(fileObject->Flags & FO_SYNCHRONOUS_IO)) {

            //
            // Query the name of the file based using an intermediary buffer.
            //

            status = IoQueryFileInformation( fileObject,
                                             FileNameInformation,
                                             length,
                                             (PVOID) fileNameInfo,
                                             &lengthNeeded );
            }
        else {

            //
            // This is a kernel mode request for a file that was opened for
            // synchronous I/O.  A special function that does not obtain the
            // file object lock is required, otherwise the request may deadlock
            // since the lock is probably already owned.
            //

            status = IopGetFileName( fileObject,
                                     length,
                                     fileNameInfo,
                                     &lengthNeeded );
            }

        //
        // If an error occurred attempting to obtain the filename return now.  Note
        // that buffer overflow is a warning, not an error.
        //

        if (NT_ERROR( status )) {
            if (status == STATUS_INVALID_PARAMETER ||
                status == STATUS_INVALID_DEVICE_REQUEST ||
                status == STATUS_NOT_IMPLEMENTED ||
                status == STATUS_INVALID_INFO_CLASS) {
                lengthNeeded = FIELD_OFFSET( FILE_NAME_INFORMATION, FileName );
                fileNameInfo->FileNameLength = 0;
                fileNameInfo->FileName[0] = OBJ_NAME_PATH_SEPARATOR;
                status = STATUS_SUCCESS;
                }
            else {
                return status;
                }
            }

        //
        // Set the remaining length of the caller's buffer as well as the total
        // length needed to contain the entire name of the file.
        //

        length = lengthNeeded - FIELD_OFFSET( FILE_NAME_INFORMATION, FileName );
        lengthNeeded = (ULONG)((PUCHAR) p - (PUCHAR) ObjectNameInfo) + fileNameInfo->FileNameLength;

        //
        // Attempt to copy the name of the file into the output buffer.  Note
        // that if the file name does not begin w/a '\', then it is not volume
        // relative, so the name of the file cannot be expressed as the
        // concatenation of the name of the device and the file.  Therefore an
        // error is returned.
        //
        // The only example of this situation known at this time is when one
        // opens a directory by file ID, and then opens a file relative to that
        // directory.  When attempting to query the path, if the caller did not
        // have traverse access to open the directory, then the only name that
        // can be returned is the path name to the file from the directory, but
        // the volume-relative name cannot be returned.  Therefore, the file
        // system returns only the name of the directory and the path to the
        // file, but this is not volume-relative so the only recourse is to
        // return an error.
        //
        // Note that if the caller were to call NtQueryInformationFile and
        // request FileNameInformation, then the name above named will be
        // successfully returned from the file system.
        //

        if (fileNameInfo->FileName[0] != OBJ_NAME_PATH_SEPARATOR) {
            return STATUS_OBJECT_PATH_INVALID;
            }

        RtlMoveMemory( p,
                       fileNameInfo->FileName,
                       length );
        p = (PWSTR) ((PCH) p + length);
        *p = '\0';
        lengthNeeded += sizeof( WCHAR );

        *ReturnLength = lengthNeeded;

        length = (ULONG)((PUCHAR) p - (PUCHAR) ObjectNameInfo);
        ObjectNameInfo->Name.Length = (USHORT) (length - sizeof( *ObjectNameInfo ));
        ObjectNameInfo->Name.MaximumLength =  (USHORT) ((length - sizeof( *ObjectNameInfo )) + sizeof( WCHAR ));
        }

    finally {

        //
        // Finally, free the temporary buffer.
        //

        ExFreePool( buffer );
        }

    return status;
}

VOID
IopCheckBackupRestorePrivilege(
    IN PACCESS_STATE AccessState,
    IN OUT PULONG CreateOptions,
    IN KPROCESSOR_MODE PreviousMode,
    IN ULONG Disposition
    )

/*++

Routine Description:

    This funcion will determine if the caller is asking for any accesses
    that may be satisfied by Backup or Restore privileges, and if so,
    perform the privilge checks.  If the privilege checks succeed, then
    the appropriate bits will be moved out of the RemainingDesiredAccess
    field in the AccessState structure and placed into the PreviouslyGrantedAccess
    field.

    Note that access is not denied if the caller does not have either or
    both of the privileges, since he may be granted the desired access
    via the security descriptor on the object.

    This routine will also set a flag in the AccessState structure so that
    it will not perform these privilege checks again in case we come through
    this way again due to a reparse.

Arguments:

    AccessState - The AccessState containing the current state of this access
        attempt.

    CreateOptions - The CreateOptions field from the OPEN_PACKET structure for
        this open attempt.

    PreviousMode - The processor mode to be used in checking parameters.

    Disposition - The create disposition for this request.

Return Value:

    None.

--*/

{
    ACCESS_MASK desiredAccess;
    ACCESS_MASK readAccess;
    ACCESS_MASK writeAccess;
    PRIVILEGE_SET requiredPrivileges;
    BOOLEAN accessGranted;
    BOOLEAN keepBackupIntent = FALSE;
    BOOLEAN ForceRestoreCheck = FALSE;

    PAGED_CODE();

    //
    // Check to determine whether or not this check has already been made.
    // If so, simply return back to the caller.
    //

    if (AccessState->Flags & SE_BACKUP_PRIVILEGES_CHECKED) {
        return;
    }

    if (*CreateOptions & FILE_OPEN_FOR_BACKUP_INTENT) {
        AccessState->Flags |= SE_BACKUP_PRIVILEGES_CHECKED;

        readAccess = READ_CONTROL | ACCESS_SYSTEM_SECURITY | FILE_GENERIC_READ | FILE_TRAVERSE;
        writeAccess = WRITE_DAC | WRITE_OWNER | ACCESS_SYSTEM_SECURITY | FILE_GENERIC_WRITE | FILE_ADD_FILE | FILE_ADD_SUBDIRECTORY | DELETE;

        desiredAccess = AccessState->RemainingDesiredAccess;

        //
        // If the caller has requested MAXIMUM_ALLOWED, then make it appear as
        // if the request was for everything permitted by Backup and Restore,
        // and then grant everything that can actually be granted.
        //

        if (desiredAccess & MAXIMUM_ALLOWED) {
            desiredAccess |= ( readAccess | writeAccess );
        }

        //
        // If the disposition says that we're opening the file, check for both backup
        // and restore privilege, depending on what's in the desired access.
        //
        // If the disposition says that we're creating or trying to overwrite the file,
        // then all we need to do is to check for restore privilege, and if it's there,
        // grant every possible access.
        //

        if ( Disposition & FILE_OPEN ) {

            //
            // If the request was for any of the bits in the read access mask, then
            // assume that this is a backup operation, and check for the Backup
            // privielege.  If the caller has it, then grant the intersection of
            // the desired access and read access masks.
            //

            if (readAccess & desiredAccess) {

                requiredPrivileges.PrivilegeCount = 1;
                requiredPrivileges.Control = PRIVILEGE_SET_ALL_NECESSARY;
                requiredPrivileges.Privilege[0].Luid = SeBackupPrivilege;
                requiredPrivileges.Privilege[0].Attributes = 0;

                accessGranted = SePrivilegeCheck( &requiredPrivileges,
                                                  &AccessState->SubjectSecurityContext,
                                                  PreviousMode );

                if (accessGranted) {

                    //
                    // The caller has Backup privilege, so grant the appropriate
                    // accesses.
                    //

                    keepBackupIntent = TRUE;
                    (VOID) SeAppendPrivileges( AccessState, &requiredPrivileges );
                    AccessState->PreviouslyGrantedAccess |= ( desiredAccess & readAccess );
                    AccessState->RemainingDesiredAccess &= ~readAccess;
                    desiredAccess &= ~readAccess;
                    AccessState->Flags |= TOKEN_HAS_BACKUP_PRIVILEGE;
                }
            }

        } else {

            ForceRestoreCheck = TRUE;
        }

        //
        // If the request was for any of the bits in the write access mask, then
        // assume that this is a restore operation, so check for the Restore
        // privilege.  If the caller has it, then grant the intersection of
        // the desired access and write access masks.
        //

        if ((writeAccess & desiredAccess) || ForceRestoreCheck) {

            requiredPrivileges.PrivilegeCount = 1;
            requiredPrivileges.Control = PRIVILEGE_SET_ALL_NECESSARY;
            requiredPrivileges.Privilege[0].Luid = SeRestorePrivilege;
            requiredPrivileges.Privilege[0].Attributes = 0;

            accessGranted = SePrivilegeCheck( &requiredPrivileges,
                                              &AccessState->SubjectSecurityContext,
                                              PreviousMode );

            if (accessGranted) {

                //
                // The caller has Restore privilege, so grant the appropriate
                // accesses.
                //

                keepBackupIntent = TRUE;
                (VOID) SeAppendPrivileges( AccessState, &requiredPrivileges );
                AccessState->PreviouslyGrantedAccess |= (desiredAccess & writeAccess);
                AccessState->RemainingDesiredAccess &= ~writeAccess;
                AccessState->Flags |= TOKEN_HAS_RESTORE_PRIVILEGE;
            }
        }

        //
        // If either of the access types was granted because the caller had
        // backup or restore privilege, then the backup intent flag is kept.
        // Otherwise, it is cleared so that it is not passed onto the driver
        // so that it is not incorrectly propogated anywhere else, since this
        // caller does not actually have the privilege enabled.
        //

        if (!keepBackupIntent) {
            *CreateOptions &= ~FILE_OPEN_FOR_BACKUP_INTENT;
        }
    }
}
