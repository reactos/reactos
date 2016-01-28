/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/ob/oblink.c
* PURPOSE:         Implements symbolic links
* PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
*                  David Welch (welch@mcmail.com)
*/

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

POBJECT_TYPE ObSymbolicLinkType = NULL;

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
ObpDeleteSymbolicLinkName(IN POBJECT_SYMBOLIC_LINK SymbolicLink)
{
    POBJECT_HEADER ObjectHeader;
    POBJECT_HEADER_NAME_INFO ObjectNameInfo;

    /* FIXME: Need to support Device maps */

    /* Get header data */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(SymbolicLink);
    ObjectNameInfo = ObpReferenceNameInfo(ObjectHeader);

    /* Check if we are not actually in a directory with a device map */
    if (!(ObjectNameInfo) ||
        !(ObjectNameInfo->Directory) /*||
        !(ObjectNameInfo->Directory->DeviceMap)*/)
    {
        ObpDereferenceNameInfo(ObjectNameInfo);
        return;
    }

    /* Check if it's a DOS drive letter, and remove the entry from drive map if needed */
    if (SymbolicLink->DosDeviceDriveIndex != 0 &&
        ObjectNameInfo->Name.Length == 2 * sizeof(WCHAR) &&
        ObjectNameInfo->Name.Buffer[1] == L':' &&
        ( (ObjectNameInfo->Name.Buffer[0] >= L'A' &&
            ObjectNameInfo->Name.Buffer[0] <= L'Z') ||
          (ObjectNameInfo->Name.Buffer[0] >= L'a' &&
            ObjectNameInfo->Name.Buffer[0] <= L'z') ))
    {
        /* Remove the drive entry */
        KeAcquireGuardedMutex(&ObpDeviceMapLock);
        ObSystemDeviceMap->DriveType[SymbolicLink->DosDeviceDriveIndex-1] =
            DOSDEVICE_DRIVE_UNKNOWN;
        ObSystemDeviceMap->DriveMap &=
            ~(1 << (SymbolicLink->DosDeviceDriveIndex-1));
        KeReleaseGuardedMutex(&ObpDeviceMapLock);

        /* Reset the drive index, valid drive index starts from 1 */
        SymbolicLink->DosDeviceDriveIndex = 0;
    }

    ObpDereferenceNameInfo(ObjectNameInfo);
}

NTSTATUS
NTAPI
ObpParseSymbolicLinkToIoDeviceObject(IN POBJECT_DIRECTORY SymbolicLinkDirectory,
                                     IN OUT POBJECT_DIRECTORY *Directory,
                                     IN OUT PUNICODE_STRING TargetPath,
                                     IN OUT POBP_LOOKUP_CONTEXT Context,
                                     OUT PVOID *Object)
{
    UNICODE_STRING Name;
    BOOLEAN ManualUnlock;

    if (! TargetPath || ! Object || ! Context || ! Directory ||
        ! SymbolicLinkDirectory)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* FIXME: Need to support Device maps */

    /* Try walking the target path and open each part of the path */
    while (TargetPath->Length)
    {
        /* Strip '\' if present at the beginning of the target path */
        if (TargetPath->Length >= sizeof(OBJ_NAME_PATH_SEPARATOR)&&
            TargetPath->Buffer[0] == OBJ_NAME_PATH_SEPARATOR)
        {
            TargetPath->Buffer++;
            TargetPath->Length -= sizeof(OBJ_NAME_PATH_SEPARATOR);
        }

        /* Remember the current component of the target path */
        Name = *TargetPath;

        /* Move forward to the next component of the target path */
        while (TargetPath->Length >= sizeof(OBJ_NAME_PATH_SEPARATOR))
        {
            if (TargetPath->Buffer[0] != OBJ_NAME_PATH_SEPARATOR)
            {
                TargetPath->Buffer++;
                TargetPath->Length -= sizeof(OBJ_NAME_PATH_SEPARATOR);
            }
            else
                break;
        }

        Name.Length -= TargetPath->Length;

        /* Finished processing the entire path, stop */
        if (! Name.Length)
            break;

        /*
         * Make sure a deadlock does not occur as an exclusive lock on a pushlock
         * would have already taken one in ObpLookupObjectName() on the parent
         * directory where the symlink is being created [ObInsertObject()].
         * Prevent recursive locking by faking lock state in the lookup context
         * when the current directory is same as the parent directory where
         * the symlink is being created. If the lock state is not faked,
         * ObpLookupEntryDirectory() will try to get a recursive lock on the
         * pushlock and hang. For e.g. this happens when a substed drive is pointed to
         * another substed drive.
         */
        if (*Directory == SymbolicLinkDirectory && ! Context->DirectoryLocked)
        {
            /* Fake lock state so that ObpLookupEntryDirectory() doesn't attempt a lock */
            ManualUnlock = TRUE;
            Context->DirectoryLocked = TRUE;
        }
        else
            ManualUnlock = FALSE;

        *Object = ObpLookupEntryDirectory(*Directory,
                                          &Name,
                                          0,
                                          FALSE,
                                          Context);

        /* Locking was faked, undo it now */
        if (*Directory == SymbolicLinkDirectory && ManualUnlock)
            Context->DirectoryLocked = FALSE;

        /* Lookup failed, stop */
        if (! *Object)
            break;

        if (OBJECT_TO_OBJECT_HEADER(*Object)->Type == ObDirectoryType)
        {
            /* Make this current directory, and continue search */
            *Directory = (POBJECT_DIRECTORY)*Object;
        }
        else if (OBJECT_TO_OBJECT_HEADER(*Object)->Type == ObSymbolicLinkType &&
            (((POBJECT_SYMBOLIC_LINK)*Object)->DosDeviceDriveIndex == 0))
        {
            /* Symlink points to another initialized symlink, ask caller to reparse */
            *Directory = ObpRootDirectoryObject;
            TargetPath = &((POBJECT_SYMBOLIC_LINK)*Object)->LinkTarget;
            return STATUS_REPARSE_OBJECT;
        }
        else
        {
            /* Neither directory or symlink, stop */
            break;
        }
    }

    /* Return a valid object, only if object type is IoDeviceObject */
    if (*Object &&
        OBJECT_TO_OBJECT_HEADER(*Object)->Type != IoDeviceObjectType)
    {
        *Object = NULL;
    }
    return STATUS_SUCCESS;
}

VOID
NTAPI
ObpCreateSymbolicLinkName(IN POBJECT_SYMBOLIC_LINK SymbolicLink)
{
    POBJECT_HEADER ObjectHeader;
    POBJECT_HEADER_NAME_INFO ObjectNameInfo;
    PVOID Object = NULL;
    POBJECT_DIRECTORY Directory;
    UNICODE_STRING TargetPath;
    NTSTATUS Status;
    ULONG DriveType = DOSDEVICE_DRIVE_CALCULATE;
    ULONG ReparseCnt;
    const ULONG MaxReparseAttempts = 20;
    OBP_LOOKUP_CONTEXT Context;

    /* FIXME: Need to support Device maps */

    /* Get header data */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(SymbolicLink);
    ObjectNameInfo = ObpReferenceNameInfo(ObjectHeader);

    /* Check if we are not actually in a directory with a device map */
    if (!(ObjectNameInfo) ||
        !(ObjectNameInfo->Directory) /*||
        !(ObjectNameInfo->Directory->DeviceMap)*/)
    {
        ObpDereferenceNameInfo(ObjectNameInfo);
        return;
    }

    /* Check if it's a DOS drive letter, and set the drive index accordingly */
    if (ObjectNameInfo->Name.Length == 2 * sizeof(WCHAR) &&
        ObjectNameInfo->Name.Buffer[1] == L':' &&
        ( (ObjectNameInfo->Name.Buffer[0] >= L'A' &&
            ObjectNameInfo->Name.Buffer[0] <= L'Z') ||
          (ObjectNameInfo->Name.Buffer[0] >= L'a' &&
            ObjectNameInfo->Name.Buffer[0] <= L'z') ))
    {
        SymbolicLink->DosDeviceDriveIndex =
            RtlUpcaseUnicodeChar(ObjectNameInfo->Name.Buffer[0]) - L'A';
        /* The Drive index start from 1 */
        SymbolicLink->DosDeviceDriveIndex++;

        /* Initialize lookup context */
        ObpInitializeLookupContext(&Context);

        /* Start the search from the root */
        Directory = ObpRootDirectoryObject;
        TargetPath = SymbolicLink->LinkTarget;

        /*
         * Locate the IoDeviceObject if any this symbolic link points to.
         * To prevent endless reparsing, setting an upper limit on the 
         * number of reparses.
         */
        Status = STATUS_REPARSE_OBJECT;
        ReparseCnt = 0;
        while (Status == STATUS_REPARSE_OBJECT &&
               ReparseCnt < MaxReparseAttempts)
        {
            Status =
                ObpParseSymbolicLinkToIoDeviceObject(ObjectNameInfo->Directory,
                                                     &Directory,
                                                     &TargetPath,
                                                     &Context,
                                                     &Object);
            if (Status == STATUS_REPARSE_OBJECT)
                ReparseCnt++;
        }

        /* Cleanup lookup context */
        ObpReleaseLookupContext(&Context);

        /* Error, or max resparse attemtps exceeded */
        if (! NT_SUCCESS(Status) || ReparseCnt >= MaxReparseAttempts)
        {
            /* Cleanup */
            ObpDereferenceNameInfo(ObjectNameInfo);
            return;
        }

        if (Object)
        {
            /* Calculate the drive type */
            switch(((PDEVICE_OBJECT)Object)->DeviceType)
            {
            case FILE_DEVICE_VIRTUAL_DISK:
                DriveType = DOSDEVICE_DRIVE_RAMDISK;
                break;
            case FILE_DEVICE_CD_ROM:
            case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
                DriveType = DOSDEVICE_DRIVE_CDROM;
                break;
            case FILE_DEVICE_DISK:
            case FILE_DEVICE_DISK_FILE_SYSTEM:
            case FILE_DEVICE_FILE_SYSTEM:
                if (((PDEVICE_OBJECT)Object)->Characteristics & FILE_REMOVABLE_MEDIA)
                    DriveType = DOSDEVICE_DRIVE_REMOVABLE;
                else
                    DriveType = DOSDEVICE_DRIVE_FIXED;
                break;
            case FILE_DEVICE_NETWORK:
            case FILE_DEVICE_NETWORK_FILE_SYSTEM:
                DriveType = DOSDEVICE_DRIVE_REMOTE;
                break;
            default:
                DPRINT1("Device Type %lu for %wZ is not known or unhandled\n",
                        ((PDEVICE_OBJECT)Object)->DeviceType,
                        &SymbolicLink->LinkTarget);
                DriveType = DOSDEVICE_DRIVE_UNKNOWN;
            }
        }

        /* Add a new drive entry */
        KeAcquireGuardedMutex(&ObpDeviceMapLock);
        ObSystemDeviceMap->DriveType[SymbolicLink->DosDeviceDriveIndex-1] =
            (UCHAR)DriveType;
        ObSystemDeviceMap->DriveMap |=
            1 << (SymbolicLink->DosDeviceDriveIndex-1);
        KeReleaseGuardedMutex(&ObpDeviceMapLock);
    }

    /* Cleanup */
    ObpDereferenceNameInfo(ObjectNameInfo);
}

/*++
* @name ObpDeleteSymbolicLink
*
*     The ObpDeleteSymbolicLink routine <FILLMEIN>
*
* @param ObjectBody
*        <FILLMEIN>
*
* @return None.
*
* @remarks None.
*
*--*/
VOID
NTAPI
ObpDeleteSymbolicLink(PVOID ObjectBody)
{
    POBJECT_SYMBOLIC_LINK SymlinkObject = (POBJECT_SYMBOLIC_LINK)ObjectBody;

    /* Make sure that the symbolic link has a name */
    if (SymlinkObject->LinkTarget.Buffer)
    {
        /* Free the name */
        ExFreePool(SymlinkObject->LinkTarget.Buffer);
        SymlinkObject->LinkTarget.Buffer = NULL;
    }
}

/*++
* @name ObpParseSymbolicLink
*
*     The ObpParseSymbolicLink routine <FILLMEIN>
*
* @param Object
*        <FILLMEIN>
*
* @param NextObject
*        <FILLMEIN>
*
* @param FullPath
*        <FILLMEIN>
*
* @param RemainingPath
*        <FILLMEIN>
*
* @param Attributes
*        <FILLMEIN>
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
ObpParseSymbolicLink(IN PVOID ParsedObject,
                     IN PVOID ObjectType,
                     IN OUT PACCESS_STATE AccessState,
                     IN KPROCESSOR_MODE AccessMode,
                     IN ULONG Attributes,
                     IN OUT PUNICODE_STRING FullPath,
                     IN OUT PUNICODE_STRING RemainingName,
                     IN OUT PVOID Context OPTIONAL,
                     IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
                     OUT PVOID *NextObject)
{
    POBJECT_SYMBOLIC_LINK SymlinkObject = (POBJECT_SYMBOLIC_LINK)ParsedObject;
    PUNICODE_STRING TargetPath;
    PWSTR NewTargetPath;
    ULONG LengthUsed, MaximumLength, TempLength;
    NTSTATUS Status;
    PAGED_CODE();

    /* Assume failure */
    *NextObject = NULL;

    /* Check if we're out of name to parse */
    if (!RemainingName->Length)
    {
        /* Check if we got an object type */
        if (ObjectType)
        {
            /* Reference the object only */
            Status = ObReferenceObjectByPointer(ParsedObject,
                                                0,
                                                ObjectType,
                                                AccessMode);
            if (NT_SUCCESS(Status))
            {
                /* Return it */
                *NextObject = ParsedObject;
            }

            if ((NT_SUCCESS(Status)) || (Status != STATUS_OBJECT_TYPE_MISMATCH))
            {
                /* Fail */
                return Status;
            }
        }
    }
    else if (RemainingName->Buffer[0] != OBJ_NAME_PATH_SEPARATOR)
    {
        /* Symbolic links must start with a backslash */
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    /* Check if this symlink is bound to a specific object */
    if (SymlinkObject->LinkTargetObject)
    {
        UNIMPLEMENTED;
    }

    /* Set the target path and length */
    TargetPath = &SymlinkObject->LinkTarget;
    TempLength = TargetPath->Length;

    /*
     * Strip off the extra trailing '\', if we don't do this we will end up
     * adding a extra '\' between TargetPath and RemainingName
     * causing caller's like ObpLookupObjectName() to fail.
     */
    if (TempLength && RemainingName->Length)
    {
        /* The target and remaining names aren't empty, so check for slashes */
        if ((TargetPath->Buffer[TempLength / sizeof(WCHAR) - 1] ==
            OBJ_NAME_PATH_SEPARATOR) &&
            (RemainingName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR))
        {
            /* Reduce the length by one to cut off the extra '\' */
            TempLength -= sizeof(OBJ_NAME_PATH_SEPARATOR);
        }
    }

    /* Calculate the new length */
    LengthUsed = TempLength + RemainingName->Length;

    /* Check if it's not too much */
    if (LengthUsed > 0xFFF0)
        return STATUS_NAME_TOO_LONG;

    /* Optimization: check if the new name is shorter */
    if (FullPath->MaximumLength <= LengthUsed)
    {
        /* It's not, allocate a new one */
        MaximumLength = LengthUsed + sizeof(WCHAR);
        NewTargetPath = ExAllocatePoolWithTag(NonPagedPool,
                                              MaximumLength,
                                              OB_NAME_TAG);
        if (!NewTargetPath) return STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        /* It is! Reuse the name... */
        MaximumLength = FullPath->MaximumLength;
        NewTargetPath = FullPath->Buffer;
    }

    /* Make sure we have a length */
    if (RemainingName->Length)
    {
        /* Copy the new path */
        RtlMoveMemory((PVOID)((ULONG_PTR)NewTargetPath + TempLength),
                      RemainingName->Buffer,
                      RemainingName->Length);
    }

    /* Copy the target path and null-terminate it */
    RtlCopyMemory(NewTargetPath, TargetPath->Buffer, TempLength);
    NewTargetPath[LengthUsed / sizeof(WCHAR)] = UNICODE_NULL;

    /* If the optimization didn't work, free the old buffer */
    if (NewTargetPath != FullPath->Buffer) ExFreePool(FullPath->Buffer);

    /* Update the path values */
    FullPath->Length = (USHORT)LengthUsed;
    FullPath->MaximumLength = (USHORT)MaximumLength;
    FullPath->Buffer = NewTargetPath;

    /* Tell the parse routine to start reparsing */
    return STATUS_REPARSE;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*++
* @name NtCreateSymbolicLinkObject
* @implemented NT4
*
*     The NtCreateSymbolicLinkObject opens or creates a symbolic link object.
*
* @param LinkHandle
*        Variable which receives the symlink handle.
*
* @param DesiredAccess
*        Desired access to the symlink.
*
* @param ObjectAttributes
*        Structure describing the symlink.
*
* @param LinkTarget
*        Unicode string defining the symlink's target
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
NtCreateSymbolicLinkObject(OUT PHANDLE LinkHandle,
                           IN ACCESS_MASK DesiredAccess,
                           IN POBJECT_ATTRIBUTES ObjectAttributes,
                           IN PUNICODE_STRING LinkTarget)
{
    HANDLE hLink;
    POBJECT_SYMBOLIC_LINK SymbolicLink;
    UNICODE_STRING CapturedLinkTarget;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    PAGED_CODE();

    /* Check if we need to probe parameters */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            /* Probe the target */
            CapturedLinkTarget = ProbeForReadUnicodeString(LinkTarget);
            ProbeForRead(CapturedLinkTarget.Buffer,
                         CapturedLinkTarget.MaximumLength,
                         sizeof(WCHAR));

            /* Probe the return handle */
            ProbeForWriteHandle(LinkHandle);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        /* No need to capture */
        CapturedLinkTarget = *LinkTarget;
    }

    /* Check if the maximum length is odd */
    if (CapturedLinkTarget.MaximumLength % sizeof(WCHAR))
    {
        /* Round it down */
        CapturedLinkTarget.MaximumLength =
            (USHORT)ALIGN_DOWN(CapturedLinkTarget.MaximumLength, WCHAR);
    }

    /* Fail if the length is odd, or if the maximum is smaller or 0 */
    if ((CapturedLinkTarget.Length % sizeof(WCHAR)) ||
        (CapturedLinkTarget.MaximumLength < CapturedLinkTarget.Length) ||
        !(CapturedLinkTarget.MaximumLength))
    {
        /* This message is displayed on the debugger in Windows */
        DbgPrint("OB: Invalid symbolic link target - %wZ\n",
                 &CapturedLinkTarget);
        return STATUS_INVALID_PARAMETER;
    }

    /* Create the object */
    Status = ObCreateObject(PreviousMode,
                            ObSymbolicLinkType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(OBJECT_SYMBOLIC_LINK),
                            0,
                            0,
                            (PVOID*)&SymbolicLink);
    if (NT_SUCCESS(Status))
    {
        /* Success! Fill in the creation time immediately */
        KeQuerySystemTime(&SymbolicLink->CreationTime);

        /* Setup the target name */
        SymbolicLink->LinkTarget.Length = CapturedLinkTarget.Length;
        SymbolicLink->LinkTarget.MaximumLength = CapturedLinkTarget.Length +
                                                 sizeof(WCHAR);
        SymbolicLink->LinkTarget.Buffer =
            ExAllocatePoolWithTag(PagedPool,
                                  CapturedLinkTarget.MaximumLength,
                                  TAG_SYMLINK_TARGET);
        if (!SymbolicLink->LinkTarget.Buffer)
        {
            /* Dereference the symbolic link object and fail */
            ObDereferenceObject(SymbolicLink);
            return STATUS_NO_MEMORY;
        }

        /* Copy it */
        RtlCopyMemory(SymbolicLink->LinkTarget.Buffer,
                      CapturedLinkTarget.Buffer,
                      CapturedLinkTarget.MaximumLength);

        /* Initialize the remaining name, dos drive index and target object */
        SymbolicLink->LinkTargetObject = NULL;
        SymbolicLink->DosDeviceDriveIndex = 0;
        RtlInitUnicodeString(&SymbolicLink->LinkTargetRemaining, NULL);

        /* Insert it into the object tree */
        Status = ObInsertObject(SymbolicLink,
                                NULL,
                                DesiredAccess,
                                0,
                                NULL,
                                &hLink);
        if (NT_SUCCESS(Status))
        {
            _SEH2_TRY
            {
                /* Return the handle to caller */
                *LinkHandle = hLink;
            }
            _SEH2_EXCEPT(ExSystemExceptionFilter())
            {
                /* Get exception code */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
        }
    }

    /* Return status to caller */
    return Status;
}

/*++
* @name NtOpenSymbolicLinkObject
* @implemented NT4
*
*     The NtOpenSymbolicLinkObject opens a symbolic link object.
*
* @param LinkHandle
*        Variable which receives the symlink handle.
*
* @param DesiredAccess
*        Desired access to the symlink.
*
* @param ObjectAttributes
*        Structure describing the symlink.
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
NtOpenSymbolicLinkObject(OUT PHANDLE LinkHandle,
                         IN ACCESS_MASK DesiredAccess,
                         IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    HANDLE hLink;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    PAGED_CODE();

    /* Check if we need to probe parameters */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            /* Probe the return handle */
            ProbeForWriteHandle(LinkHandle);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Open the object */
    Status = ObOpenObjectByName(ObjectAttributes,
                                ObSymbolicLinkType,
                                PreviousMode,
                                NULL,
                                DesiredAccess,
                                NULL,
                                &hLink);
    if (NT_SUCCESS(Status))
    {
        _SEH2_TRY
        {
            /* Return the handle to caller */
            *LinkHandle = hLink;
        }
        _SEH2_EXCEPT(ExSystemExceptionFilter())
        {
            /* Get exception code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    /* Return status to caller */
    return Status;
}

/*++
* @name NtQuerySymbolicLinkObject
* @implemented NT4
*
*     The NtQuerySymbolicLinkObject queries a symbolic link object.
*
* @param LinkHandle
*        Symlink handle to query
*
* @param LinkTarget
*        Unicode string defining the symlink's target
*
* @param ResultLength
*        Caller supplied storage for the number of bytes written (or NULL).
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
NtQuerySymbolicLinkObject(IN HANDLE LinkHandle,
                          OUT PUNICODE_STRING LinkTarget,
                          OUT PULONG ResultLength OPTIONAL)
{
    UNICODE_STRING SafeLinkTarget = { 0, 0, NULL };
    POBJECT_SYMBOLIC_LINK SymlinkObject;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    ULONG LengthUsed;
    PAGED_CODE();

    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            /* Probe the unicode string for read and write */
            ProbeForWriteUnicodeString(LinkTarget);

            /* Probe the unicode string's buffer for write */
            SafeLinkTarget = *LinkTarget;
            ProbeForWrite(SafeLinkTarget.Buffer,
                          SafeLinkTarget.MaximumLength,
                          sizeof(WCHAR));

            /* Probe the return length */
            if (ResultLength) ProbeForWriteUlong(ResultLength);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        /* No need to probe */
        SafeLinkTarget = *LinkTarget;
    }

    /* Reference the object */
    Status = ObReferenceObjectByHandle(LinkHandle,
                                       SYMBOLIC_LINK_QUERY,
                                       ObSymbolicLinkType,
                                       PreviousMode,
                                       (PVOID *)&SymlinkObject,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Lock the object */
        ObpAcquireObjectLock(OBJECT_TO_OBJECT_HEADER(SymlinkObject));

        /*
         * So here's the thing: If you specify a return length, then the
         * implementation will use the maximum length. If you don't, then
         * it will use the length.
         */
        LengthUsed = ResultLength ? SymlinkObject->LinkTarget.MaximumLength :
                                    SymlinkObject->LinkTarget.Length;

        /* Enter SEH so we can safely copy */
        _SEH2_TRY
        {
            /* Make sure our buffer will fit */
            if (LengthUsed <= SafeLinkTarget.MaximumLength)
            {
                /* Copy the buffer */
                RtlCopyMemory(SafeLinkTarget.Buffer,
                              SymlinkObject->LinkTarget.Buffer,
                              LengthUsed);

                /* Copy the new length */
                LinkTarget->Length = SymlinkObject->LinkTarget.Length;
            }
            else
            {
                /* Otherwise set the failure status */
                Status = STATUS_BUFFER_TOO_SMALL;
            }

            /* In both cases, check if the required length was requested */
            if (ResultLength)
            {
                /* Then return it */
                *ResultLength = SymlinkObject->LinkTarget.MaximumLength;
            }
        }
        _SEH2_EXCEPT(ExSystemExceptionFilter())
        {
            /* Get the error code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        /* Unlock and dereference the object */
        ObpReleaseObjectLock(OBJECT_TO_OBJECT_HEADER(SymlinkObject));
        ObDereferenceObject(SymlinkObject);
    }

    /* Return query status */
    return Status;
}

/* EOF */
