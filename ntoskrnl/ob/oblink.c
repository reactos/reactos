/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/ob/oblink.c
* PURPOSE:         Implements symbolic links
* PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
*                  David Welch (welch@mcmail.com)
*                  Pierre Schweitzer
*/

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

POBJECT_TYPE ObpSymbolicLinkObjectType = NULL;

/* PRIVATE FUNCTIONS *********************************************************/

VOID
ObpProcessDosDeviceSymbolicLink(IN POBJECT_SYMBOLIC_LINK SymbolicLink,
                                IN BOOLEAN DeleteLink)
{
    PDEVICE_MAP DeviceMap;
    UNICODE_STRING TargetPath, LocalTarget;
    POBJECT_DIRECTORY NameDirectory, DirectoryObject;
    ULONG MaxReparse;
    OBP_LOOKUP_CONTEXT LookupContext;
    ULONG DriveType;
    POBJECT_HEADER ObjectHeader;
    POBJECT_HEADER_NAME_INFO ObjectNameInfo;
    BOOLEAN DirectoryLocked;
    PVOID Object;

    /*
     * To prevent endless reparsing, setting an upper limit on the
     * number of reparses.
     */
    MaxReparse = 32;
    NameDirectory = NULL;

    /* Get header data */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(SymbolicLink);
    ObjectNameInfo = OBJECT_HEADER_TO_NAME_INFO(ObjectHeader);

    /* Check if we have a directory in our symlink to use */
    if (ObjectNameInfo != NULL)
    {
        NameDirectory = ObjectNameInfo->Directory;
    }

    /* Initialize lookup context */
    ObpInitializeLookupContext(&LookupContext);

    /*
     * If we have to create the link, locate the IoDeviceObject if any
     * this symbolic link points to.
     */
    if (SymbolicLink->LinkTargetObject != NULL || !DeleteLink)
    {
        /* Start the search from the root */
        DirectoryObject = ObpRootDirectoryObject;

        /* Keep track of our progress while parsing the name */
        LocalTarget = SymbolicLink->LinkTarget;

        /* If LUID mappings are enabled, use system map */
        if (ObpLUIDDeviceMapsEnabled != 0)
        {
            DeviceMap = ObSystemDeviceMap;
        }
        /* Otherwise, use the one in the process */
        else
        {
            DeviceMap = PsGetCurrentProcess()->DeviceMap;
        }

ReparseTargetPath:
        /*
         * If we have a device map active, check whether we have a drive
         * letter prefixed with ??, if so, chomp it
         */
        if (DeviceMap != NULL)
        {
            if (!((ULONG_PTR)(LocalTarget.Buffer) & 7))
            {
                if (DeviceMap->DosDevicesDirectory != NULL)
                {
                    if (LocalTarget.Length >= ObpDosDevicesShortName.Length &&
                        (*(PULONGLONG)LocalTarget.Buffer ==
                         ObpDosDevicesShortNamePrefix.Alignment.QuadPart))
                    {
                        DirectoryObject = DeviceMap->DosDevicesDirectory;

                        LocalTarget.Length -= ObpDosDevicesShortName.Length;
                        LocalTarget.Buffer += (ObpDosDevicesShortName.Length / sizeof(WCHAR));
                    }
                }
            }
        }

        /* Try walking the target path and open each part of the path */
        while (TRUE)
        {
            if (LocalTarget.Buffer[0] == OBJ_NAME_PATH_SEPARATOR)
            {
                ++LocalTarget.Buffer;
                LocalTarget.Length -= sizeof(WCHAR);
            }

            /* Remember the current component of the target path */
            TargetPath = LocalTarget;

            /* Move forward to the next component of the target path */
            if (LocalTarget.Length != 0)
            {
                do
                {
                    if (LocalTarget.Buffer[0] == OBJ_NAME_PATH_SEPARATOR)
                    {
                        break;
                    }

                    ++LocalTarget.Buffer;
                    LocalTarget.Length -= sizeof(WCHAR);
                }
                while (LocalTarget.Length != 0);
            }

            TargetPath.Length -= LocalTarget.Length;

            /*
             * Finished processing the entire path, stop
             * That's a failure case, we quit here
             */
            if (TargetPath.Length == 0)
            {
                ObpReleaseLookupContext(&LookupContext);
                return;
            }


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
            if (DirectoryObject == NameDirectory)
            {
                DirectoryLocked = LookupContext.DirectoryLocked;
                LookupContext.DirectoryLocked = TRUE;
            }
            else
            {
                DirectoryLocked = FALSE;
            }

            Object = ObpLookupEntryDirectory(DirectoryObject,
                                             &TargetPath,
                                             0,
                                             FALSE,
                                             &LookupContext);

            /* Locking was faked, undo it now */
            if (DirectoryObject == NameDirectory)
            {
                LookupContext.DirectoryLocked = DirectoryLocked;
            }

            /* Lookup failed, stop */
            if (Object == NULL)
            {
                break;
            }

            /* If we don't have a directory object, we'll have to handle the object */
            if (OBJECT_TO_OBJECT_HEADER(Object)->Type != ObpDirectoryObjectType)
            {
                /* If that's not a symbolic link, stop here, nothing to do */
                if (OBJECT_TO_OBJECT_HEADER(Object)->Type != ObpSymbolicLinkObjectType ||
                    (((POBJECT_SYMBOLIC_LINK)Object)->DosDeviceDriveIndex != 0))
                {
                    break;
                }

                /* We're out of reparse attempts */
                if (MaxReparse == 0)
                {
                    Object = NULL;
                    break;
                }

                --MaxReparse;

                /* Symlink points to another initialized symlink, ask caller to reparse */
                DirectoryObject = ObpRootDirectoryObject;

                LocalTarget = ((POBJECT_SYMBOLIC_LINK)Object)->LinkTarget;

                goto ReparseTargetPath;
            }

            /* Make this current directory, and continue search */
            DirectoryObject = Object;
        }
    }

    DeviceMap = NULL;
    /* That's a drive letter, find a suitable device map */
    if (SymbolicLink->DosDeviceDriveIndex != 0)
    {
        ObjectHeader = OBJECT_TO_OBJECT_HEADER(SymbolicLink);
        ObjectNameInfo = ObpReferenceNameInfo(ObjectHeader);
        if (ObjectNameInfo != NULL)
        {
            if (ObjectNameInfo->Directory != NULL)
            {
                DeviceMap = ObjectNameInfo->Directory->DeviceMap;
            }

            ObpDereferenceNameInfo(ObjectNameInfo);
        }
    }

    /* If we were asked to delete the symlink */
    if (DeleteLink)
    {
        /* Zero its target */
        RtlInitUnicodeString(&SymbolicLink->LinkTargetRemaining, NULL);

        /* If we had a target objected, dereference it */
        if (SymbolicLink->LinkTargetObject != NULL)
        {
            ObDereferenceObject(SymbolicLink->LinkTargetObject);
            SymbolicLink->LinkTargetObject = NULL;
        }

        /* If it was a drive letter */
        if (DeviceMap != NULL)
        {
            /* Acquire the device map lock */
            KeAcquireGuardedMutex(&ObpDeviceMapLock);

            /* Remove the drive entry */
            DeviceMap->DriveType[SymbolicLink->DosDeviceDriveIndex - 1] =
                DOSDEVICE_DRIVE_UNKNOWN;
            DeviceMap->DriveMap &=
                ~(1 << (SymbolicLink->DosDeviceDriveIndex - 1));

            /* Release the device map lock */
            KeReleaseGuardedMutex(&ObpDeviceMapLock);

            /* Reset the drive index, valid drive index starts from 1 */
            SymbolicLink->DosDeviceDriveIndex = 0;
        }
    }
    else
    {
        DriveType = DOSDEVICE_DRIVE_CALCULATE;

        /* If we have a drive letter and a pointer device object */
        if (Object != NULL && SymbolicLink->DosDeviceDriveIndex != 0 &&
            OBJECT_TO_OBJECT_HEADER(Object)->Type == IoDeviceObjectType)
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
        if (DeviceMap != NULL)
        {
            /* Acquire the device map lock */
            KeAcquireGuardedMutex(&ObpDeviceMapLock);

            DeviceMap->DriveType[SymbolicLink->DosDeviceDriveIndex - 1] =
                (UCHAR)DriveType;
            DeviceMap->DriveMap |=
                1 << (SymbolicLink->DosDeviceDriveIndex - 1);

            /* Release the device map lock */
            KeReleaseGuardedMutex(&ObpDeviceMapLock);
        }
    }

    /* Cleanup */
    ObpReleaseLookupContext(&LookupContext);
}

VOID
NTAPI
ObpDeleteSymbolicLinkName(IN POBJECT_SYMBOLIC_LINK SymbolicLink)
{
    /* Just call the helper */
    ObpProcessDosDeviceSymbolicLink(SymbolicLink, TRUE);
}

VOID
NTAPI
ObpCreateSymbolicLinkName(IN POBJECT_SYMBOLIC_LINK SymbolicLink)
{
    WCHAR UpperDrive;
    POBJECT_HEADER ObjectHeader;
    POBJECT_HEADER_NAME_INFO ObjectNameInfo;

    /* Get header data */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(SymbolicLink);
    ObjectNameInfo = ObpReferenceNameInfo(ObjectHeader);

    /* No name info, nothing to create */
    if (ObjectNameInfo == NULL)
    {
        return;
    }

    /* If we have a device map, look for creating a letter based drive */
    if (ObjectNameInfo->Directory != NULL &&
        ObjectNameInfo->Directory->DeviceMap != NULL)
    {
        /* Is it a drive letter based name? */
        if (ObjectNameInfo->Name.Length == 2 * sizeof(WCHAR))
        {
            if (ObjectNameInfo->Name.Buffer[1] == L':')
            {
                UpperDrive = RtlUpcaseUnicodeChar(ObjectNameInfo->Name.Buffer[0]);
                if (UpperDrive >= L'A' && UpperDrive <= L'Z')
                {
                    /* Compute its index (it's 1 based - 0 means no letter) */
                    SymbolicLink->DosDeviceDriveIndex = UpperDrive - (L'A' - 1);
                }
            }
        }

        /* Call the helper */
        ObpProcessDosDeviceSymbolicLink(SymbolicLink, FALSE);
    }

    /* We're done */
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
        /* No name to reparse, directly reparse the object */
        if (!SymlinkObject->LinkTargetRemaining.Length)
        {
            *NextObject = SymlinkObject->LinkTargetObject;
            return STATUS_REPARSE_OBJECT;
        }

        TempLength = SymlinkObject->LinkTargetRemaining.Length;
        /* The target and remaining names aren't empty, so check for slashes */
        if (SymlinkObject->LinkTargetRemaining.Buffer[TempLength / sizeof(WCHAR) - 1] == OBJ_NAME_PATH_SEPARATOR &&
            RemainingName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR)
        {
            /* Reduce the length by one to cut off the extra '\' */
            TempLength -= sizeof(OBJ_NAME_PATH_SEPARATOR);
        }

        /* Calculate the new length */
        LengthUsed = TempLength + RemainingName->Length;
        LengthUsed += (sizeof(WCHAR) * (RemainingName->Buffer - FullPath->Buffer));

        /* Check if it's not too much */
        if (LengthUsed > 0xFFF0)
            return STATUS_NAME_TOO_LONG;

        /* If FullPath is enough, use it */
        if (FullPath->MaximumLength > LengthUsed)
        {
            /* Update remaining length if appropriate */
            if (RemainingName->Length)
            {
                RtlMoveMemory((PVOID)((ULONG_PTR)RemainingName->Buffer + TempLength),
                              RemainingName->Buffer,
                              RemainingName->Length);
            }

            /* And move the target object name */
            RtlCopyMemory(RemainingName->Buffer,
                          SymlinkObject->LinkTargetRemaining.Buffer,
                          TempLength);

            /* Finally update the full path with what we parsed */
            FullPath->Length += SymlinkObject->LinkTargetRemaining.Length;
            RemainingName->Length += SymlinkObject->LinkTargetRemaining.Length;
            RemainingName->MaximumLength += RemainingName->Length + sizeof(WCHAR);
            FullPath->Buffer[FullPath->Length / sizeof(WCHAR)] = UNICODE_NULL;

            /* And reparse */
            *NextObject = SymlinkObject->LinkTargetObject;
            return STATUS_REPARSE_OBJECT;
        }

        /* FullPath is not enough, we'll have to reallocate */
        MaximumLength = LengthUsed + sizeof(WCHAR);
        NewTargetPath = ExAllocatePoolWithTag(NonPagedPool,
                                              MaximumLength,
                                              OB_NAME_TAG);
        if (!NewTargetPath) return STATUS_INSUFFICIENT_RESOURCES;

        /* Copy path begin */
        RtlCopyMemory(NewTargetPath,
                      FullPath->Buffer,
                      sizeof(WCHAR) * (RemainingName->Buffer - FullPath->Buffer));

        /* Copy path end (if any) */
        if (RemainingName->Length)
        {
            RtlCopyMemory((PVOID)((ULONG_PTR)&NewTargetPath[RemainingName->Buffer - FullPath->Buffer] + TempLength),
                          RemainingName->Buffer,
                          RemainingName->Length);
        }

        /* And finish path with bound object */
        RtlCopyMemory(&NewTargetPath[RemainingName->Buffer - FullPath->Buffer],
                      SymlinkObject->LinkTargetRemaining.Buffer,
                      TempLength);

        /* Free old buffer */
        ExFreePool(FullPath->Buffer);

        /* Set new buffer in FullPath */
        FullPath->Buffer = NewTargetPath;
        FullPath->MaximumLength = MaximumLength;
        FullPath->Length = LengthUsed;

        /* Update remaining with what we handled */
        RemainingName->Length = LengthUsed + (ULONG_PTR)NewTargetPath - (ULONG_PTR)&NewTargetPath[RemainingName->Buffer - FullPath->Buffer];
        RemainingName->Buffer = &NewTargetPath[RemainingName->Buffer - FullPath->Buffer];
        RemainingName->MaximumLength = RemainingName->Length + sizeof(WCHAR);

        /* Reparse! */
        *NextObject = SymlinkObject->LinkTargetObject;
        return STATUS_REPARSE_OBJECT;
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
                            ObpSymbolicLinkObjectType,
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
        SymbolicLink->LinkTarget.MaximumLength = CapturedLinkTarget.MaximumLength;
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
        _SEH2_TRY
        {
            RtlCopyMemory(SymbolicLink->LinkTarget.Buffer,
                          CapturedLinkTarget.Buffer,
                          CapturedLinkTarget.MaximumLength);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            ObDereferenceObject(SymbolicLink);
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;

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
                                ObpSymbolicLinkObjectType,
                                PreviousMode,
                                NULL,
                                DesiredAccess,
                                NULL,
                                &hLink);

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
    NTSTATUS Status;
    UNICODE_STRING SafeLinkTarget = { 0, 0, NULL };
    POBJECT_SYMBOLIC_LINK SymlinkObject;
    POBJECT_HEADER ObjectHeader;
    ULONG LengthUsed;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();

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
                                       ObpSymbolicLinkObjectType,
                                       PreviousMode,
                                       (PVOID*)&SymlinkObject,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Get the object header */
        ObjectHeader = OBJECT_TO_OBJECT_HEADER(SymlinkObject);

        /* Lock the object */
        ObpAcquireObjectLock(ObjectHeader);

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
        ObpReleaseObjectLock(ObjectHeader);
        ObDereferenceObject(SymlinkObject);
    }

    /* Return query status */
    return Status;
}

/* EOF */
