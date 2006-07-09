/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ob/namespce.c
 * PURPOSE:         Manages all functions related to the Object Manager name-
 *                  space, such as finding objects or querying their names.
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Eric Kohl
 *                  Thomas Weidenmueller (w3seek@reactos.org)
 */

/* INCLUDES ******************************************************************/

#define NTDDI_VERSION NTDDI_WINXP
#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

POBJECT_DIRECTORY NameSpaceRoot = NULL;
POBJECT_DIRECTORY ObpTypeDirectoryObject = NULL;

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
ObDereferenceDeviceMap(IN PEPROCESS Process)
{
    //KIRQL OldIrql;
    PDEVICE_MAP DeviceMap = Process->DeviceMap;

    /* FIXME: We don't use Process Devicemaps yet */
    if (DeviceMap)
    {
        /* FIXME: Acquire the DeviceMap Spinlock */
        // KeAcquireSpinLock(DeviceMap->Lock, &OldIrql);

        /* Delete the device map link and dereference it */
        Process->DeviceMap = NULL;
        if (--DeviceMap->ReferenceCount)
        {
            /* Nobody is referencing it anymore, unlink the DOS directory */
            DeviceMap->DosDevicesDirectory->DeviceMap = NULL;

            /* FIXME: Release the DeviceMap Spinlock */
            // KeReleasepinLock(DeviceMap->Lock, OldIrql);

            /* Dereference the DOS Devices Directory and free the Device Map */
            ObDereferenceObject(DeviceMap->DosDevicesDirectory);
            ExFreePool(DeviceMap);
        }
        else
        {
            /* FIXME: Release the DeviceMap Spinlock */
            // KeReleasepinLock(DeviceMap->Lock, OldIrql);
        }
    }
}

VOID
NTAPI
ObInheritDeviceMap(IN PEPROCESS Parent,
                   IN PEPROCESS Process)
{
    /* FIXME: Devicemap Support */
}

/*++
* @name ObpDeleteNameCheck
*
*     The ObpDeleteNameCheck routine checks if a named object should be
*     removed from the object directory namespace.
*
* @param Object
*        Pointer to the object to check for possible removal.
*
* @return None.
*
* @remarks An object is removed if the following 4 criteria are met:
*          1) The object has 0 handles open
*          2) The object is in the directory namespace and has a name
*          3) The object is not permanent
*
*--*/
VOID
NTAPI
ObpDeleteNameCheck(IN PVOID Object)
{
    POBJECT_HEADER ObjectHeader;
    OBP_LOOKUP_CONTEXT Context;
    POBJECT_HEADER_NAME_INFO ObjectNameInfo;
    POBJECT_TYPE ObjectType;
    PVOID Directory = NULL;

    /* Get object structures */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
    ObjectNameInfo = OBJECT_HEADER_TO_NAME_INFO(ObjectHeader);
    ObjectType = ObjectHeader->Type;

    /*
     * Check if the handle count is 0, if the object is named,
     * and if the object isn't a permanent object.
     */
    if (!(ObjectHeader->HandleCount) &&
         (ObjectNameInfo) &&
         (ObjectNameInfo->Name.Length) &&
         !(ObjectHeader->Flags & OB_FLAG_PERMANENT))
    {
        /* Make sure it's still inserted */
        Context.Directory = ObjectNameInfo->Directory;
        Context.DirectoryLocked = TRUE;
        Object = ObpLookupEntryDirectory(ObjectNameInfo->Directory,
                                         &ObjectNameInfo->Name,
                                         0,
                                         FALSE,
                                         &Context);
        if ((Object) && !(ObjectHeader->HandleCount))
        {
            /* First delete it from the directory */
            ObpDeleteEntryDirectory(&Context);

            /* Now check if we have a security callback */
            if (ObjectType->TypeInfo.SecurityRequired)
            {
                /* Call it */
                ObjectType->TypeInfo.SecurityProcedure(Object,
                                                       DeleteSecurityDescriptor,
                                                       0,
                                                       NULL,
                                                       NULL,
                                                       &ObjectHeader->
                                                       SecurityDescriptor,
                                                       ObjectType->
                                                       TypeInfo.PoolType,
                                                       NULL);
            }

            /* Free the name */
            ExFreePool(ObjectNameInfo->Name.Buffer);
            RtlInitEmptyUnicodeString(&ObjectNameInfo->Name, NULL, 0);

            /* Clear the current directory and de-reference it */
            Directory = ObjectNameInfo->Directory;
            ObjectNameInfo->Directory = NULL;
        }

        /* Check if we were inserted in a directory */
        if (Directory)
        {
            /* We were, so dereference the directory and the object as well */
            ObDereferenceObject(Directory);
            ObDereferenceObject(Object);
        }
    }
}

NTSTATUS
NTAPI
ObFindObject(IN HANDLE RootHandle,
             IN PUNICODE_STRING ObjectName,
             IN ULONG Attributes,
             IN KPROCESSOR_MODE AccessMode,
             IN PVOID *ReturnedObject,
             IN POBJECT_TYPE ObjectType,
             IN POBP_LOOKUP_CONTEXT Context,
             IN PACCESS_STATE AccessState,
             IN PSECURITY_QUALITY_OF_SERVICE SecurityQos,
             IN PVOID ParseContext,
             OUT PVOID ExpectedObject)
{
    PVOID RootDirectory;
    PVOID CurrentDirectory = NULL;
    PVOID CurrentObject = NULL;
    POBJECT_HEADER CurrentHeader;
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID NewName;
    POBJECT_HEADER_NAME_INFO ObjectNameInfo;
    UNICODE_STRING RemainingPath, PartName;
    BOOLEAN InsideRoot = FALSE;
    OB_PARSE_METHOD ParseRoutine;
    PAGED_CODE();

    /* Assume failure */
    OBTRACE(OB_NAMESPACE_DEBUG,
            "%s - Finding Object: %wZ. Expecting: %p\n",
            __FUNCTION__,
            ObjectName,
            ExpectedObject);
    *ReturnedObject = NULL;

    /* Check if we got a Root Directory */
    if (RootHandle)
    {
        /* We did. Reference it */
        Status = ObReferenceObjectByHandle(RootHandle,
                                           0,
                                           NULL,
                                           AccessMode,
                                           &RootDirectory,
                                           NULL);
        if (!NT_SUCCESS(Status)) return Status;

        /* Get the header */
        CurrentHeader = OBJECT_TO_OBJECT_HEADER(RootDirectory);

        /* The name cannot start with a separator, unless this is a file */
        if ((ObjectName->Buffer) &&
            (ObjectName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR) &&
            (CurrentHeader->Type != IoFileObjectType))
        {
            /* The syntax is bad, so fail this request */
            ObDereferenceObject(RootDirectory);
            return STATUS_OBJECT_PATH_SYNTAX_BAD;
        }

        /* Don't parse a Directory */
        if (CurrentHeader->Type != ObDirectoryType)
        {
            /* Make sure the Object Type has a parse routine */
            ParseRoutine = CurrentHeader->Type->TypeInfo.ParseProcedure;
            if (!ParseRoutine)
            {
                /* We can't parse a name if we don't have a parse routine */
                ObDereferenceObject(RootDirectory);
                return STATUS_INVALID_HANDLE;
            }

            /* Now parse */
            while (TRUE)
            {
                /* Start with the full name */
                RemainingPath = *ObjectName;

                /* Call the Parse Procedure */
                Status = ParseRoutine(RootDirectory,
                                      ObjectType,
                                      AccessState,
                                      AccessMode,
                                      Attributes,
                                      ObjectName,
                                      &RemainingPath,
                                      ParseContext,
                                      SecurityQos,
                                      &CurrentObject);

                /* Check for success or failure, so not reparse */
                if ((Status != STATUS_REPARSE) &&
                    (Status != STATUS_REPARSE_OBJECT))
                {
                    /* Check for failure */
                    if (!NT_SUCCESS(Status))
                    {
                        /* Parse routine might not have cleared this, do it */
                        CurrentObject = NULL;
                    }
                    else if (!CurrentObject)
                    {
                        /* Modify status to reflect failure inside Ob */
                        Status = STATUS_OBJECT_NAME_NOT_FOUND;
                    }

                    /* We're done, return the status and object */
                    *ReturnedObject = CurrentObject;
                    ObDereferenceObject(RootDirectory);
                    return Status;
                }
                else if ((!ObjectName->Length) ||
                         (!ObjectName->Buffer) ||
                         (ObjectName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR))
                {
                    /* Reparsed to the root directory, so start over */
                    ObDereferenceObject(RootDirectory);
                    RootDirectory = NameSpaceRoot;

                    /* Don't use this anymore, since we're starting at root */
                    RootHandle = NULL;
                    break;
                }
            }
        }
        else if (!(ObjectName->Length) || !(ObjectName->Buffer))
        {
            /* Just return the Root Directory if we didn't get a name*/
            Status = ObReferenceObjectByPointer(RootDirectory,
                                                0,
                                                ObjectType,
                                                AccessMode);
            if (NT_SUCCESS(Status)) *ReturnedObject = RootDirectory;
            ObDereferenceObject(RootDirectory);
            return Status;
        }
    }
    else
    {
        /* We did not get a Root Directory, so use the root */
        RootDirectory = NameSpaceRoot;

        /* It must start with a path separator */
        if (!(ObjectName->Length) ||
            !(ObjectName->Buffer) ||
            (ObjectName->Buffer[0] != OBJ_NAME_PATH_SEPARATOR))
        {
            /* This name is invalid, so fail */
            return STATUS_OBJECT_PATH_SYNTAX_BAD;
        }

        /* Check if the name is only the path separator */
        if (ObjectName->Length == sizeof(OBJ_NAME_PATH_SEPARATOR))
        {
            /* So the caller only wants the root directory; do we have one? */
            if (!RootDirectory)
            {
                /* This must be the first time we're creating it... right? */
                if (ExpectedObject)
                {
                    /* Yes, so return it to ObInsert so that it can create it */
                    Status = ObReferenceObjectByPointer(ExpectedObject,
                                                        0,
                                                        ObjectType,
                                                        AccessMode);
                    if (NT_SUCCESS(Status)) *ReturnedObject = ExpectedObject;
                    return Status;
                }
                else
                {
                    /* This should never really happen */
                    ASSERT(FALSE);
                    return STATUS_INVALID_PARAMETER;
                }
            }
            else
            {
                /* We do have the root directory, so just return it */
                Status = ObReferenceObjectByPointer(RootDirectory,
                                                    0,
                                                    ObjectType,
                                                    AccessMode);
                if (NT_SUCCESS(Status)) *ReturnedObject = RootDirectory;
                return Status;
            }
        }
    }

    /* Save the name */
ReparseNewDir:
    RemainingPath = *ObjectName;

    /* Reparse */
    while (TRUE)
    {
        /* Check if we should use the Root Directory */
        if (!InsideRoot)
        {
            /* Yes, use the root directory and remember that */
            CurrentDirectory = RootDirectory;
            InsideRoot = TRUE;
        }

        /* Check if the name starts with a path separator */
        if ((RemainingPath.Length) &&
            (RemainingPath.Buffer[0] == OBJ_NAME_PATH_SEPARATOR))
        {
            /* Skip the path separator */
            RemainingPath.Buffer++;
            RemainingPath.Length -= sizeof(OBJ_NAME_PATH_SEPARATOR);
        }

        /* Find the next Part Name */
        PartName = RemainingPath;
        while (RemainingPath.Length)
        {
            /* Break if we found the \ ending */
            if (RemainingPath.Buffer[0] == OBJ_NAME_PATH_SEPARATOR) break;

            /* Move on */
            RemainingPath.Buffer++;
            RemainingPath.Length -= sizeof(OBJ_NAME_PATH_SEPARATOR);
        }

        /* Get its size and make sure it's valid */
        if (!(PartName.Length -= RemainingPath.Length))
        {
            Status = STATUS_OBJECT_NAME_INVALID;
            break;
        }

        /* Do the look up */
        Context->DirectoryLocked = TRUE;
        Context->Directory = CurrentDirectory;
        CurrentObject = ObpLookupEntryDirectory(CurrentDirectory,
                                                &PartName,
                                                Attributes,
                                                FALSE,
                                                Context);
        if (!CurrentObject)
        {
            /* We didn't find it... do we still have a path? */
            if (RemainingPath.Length)
            {
                /* Then tell the caller the path wasn't found */
                Status = STATUS_OBJECT_PATH_NOT_FOUND;
                break;
            }
            else if (!ExpectedObject)
            {
                /* Otherwise, we have a path, but the name isn't valid */
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
                break;
            }

            /* Reference newly to be inserted object */
            ObReferenceObject(ExpectedObject);
            CurrentHeader = OBJECT_TO_OBJECT_HEADER(ExpectedObject);

            /* Create Object Name */
            NewName = ExAllocatePoolWithTag(NonPagedPool,
                                            PartName.MaximumLength,
                                            OB_NAME_TAG);
            ObjectNameInfo = OBJECT_HEADER_TO_NAME_INFO(CurrentHeader);

            /* Copy the Name */
            RtlMoveMemory(NewName, PartName.Buffer, PartName.MaximumLength);

            /* Free old name */
            if (ObjectNameInfo->Name.Buffer) ExFreePool(ObjectNameInfo->Name.Buffer);

            /* Write new one */
            ObjectNameInfo->Name.Buffer = NewName;
            ObjectNameInfo->Name.Length = PartName.Length;
            ObjectNameInfo->Name.MaximumLength = PartName.MaximumLength;

            /* Rereference the Directory and insert */
            ObReferenceObject(CurrentDirectory);
            ObpInsertEntryDirectory(CurrentDirectory, Context, CurrentHeader);

            /* Return Status and the Expected Object */
            Status = STATUS_SUCCESS;
            CurrentObject = ExpectedObject;

            /* Get out of here */
            break;
        }

Reparse:
        /* We found it, so now get its header */
        CurrentHeader = OBJECT_TO_OBJECT_HEADER(CurrentObject);

        /* 
         * Check for a parse Procedure, but don't bother to parse for an insert
         * unless it's a Symbolic Link, in which case we MUST parse
         */
        ParseRoutine = CurrentHeader->Type->TypeInfo.ParseProcedure;
        if (ParseRoutine &&
            (!ExpectedObject || ParseRoutine == ObpParseSymbolicLink))
        {
            /* Use the Root Directory next time */
            InsideRoot = FALSE;

            /* Call the Parse Procedure */
            Status = ParseRoutine(CurrentObject,
                                  ObjectType,
                                  AccessState,
                                  AccessMode,
                                  Attributes,
                                  ObjectName,
                                  &RemainingPath,
                                  ParseContext,
                                  SecurityQos,
                                  &CurrentObject);

            /* Check if we have to reparse */
            if ((Status == STATUS_REPARSE) ||
                (Status == STATUS_REPARSE_OBJECT))
            {
                /* Start over from root if we got sent back there */
                if ((Status == STATUS_REPARSE_OBJECT) ||
                    (ObjectName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR))
                {
                    /* Check if we got a root directory */
                    if (RootHandle)
                    {
                        /* Stop using it, because we have a new directory now */
                        ObDereferenceObject(RootDirectory);
                        RootHandle = NULL;
                    }

                    /* Start at Root */
                    RootDirectory = NameSpaceRoot;

                    /* Check for reparse status */
                    if (Status == STATUS_REPARSE_OBJECT)
                    {
                        /* Did we actually get an object to which to reparse? */
                        if (!CurrentObject)
                        {
                            /* We didn't, so set a failure status */
                            Status = STATUS_OBJECT_NAME_NOT_FOUND;
                        }
                        else
                        {
                            /* We did, so we're free to parse the new object */
                            InsideRoot = TRUE;
                            goto Reparse;
                        }
                    }

                    /* Restart the search */
                    goto ReparseNewDir;
                }
                else if (RootDirectory == NameSpaceRoot)
                {
                    /* We got STATUS_REPARSE but are at the Root Directory */
                    CurrentObject = NULL;
                    Status = STATUS_OBJECT_NAME_NOT_FOUND;
                }
            }
            else if (!NT_SUCCESS(Status))
            {
                /* Total failure */
                CurrentObject = NULL;
            }
            else if (!CurrentObject)
            {
                /* We didn't reparse but we didn't find the Object Either */
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
            }

            /* Break out of the loop */
            break;
        }
        else
        {
            /* No parse routine...do we still have a remaining name? */
            if (!RemainingPath.Length)
            {
                /* Are we creating an object? */
                if (!ExpectedObject)
                {
                    /* We don't... reference the Object */
                    Status = ObReferenceObjectByPointer(CurrentObject,
                                                        0,
                                                        ObjectType,
                                                        AccessMode);
                    if (!NT_SUCCESS(Status)) CurrentObject = NULL;
                }

                /* And get out of the reparse loop */
                break;
            }
            else
            {
                /* We still have a name; check if this is a directory object */
                if (CurrentHeader->Type == ObDirectoryType)
                {
                    /* Restart from this directory */
                    CurrentDirectory = CurrentObject;
                }
                else
                {
                    /* We still have a name, but no parse routine for it */
                    Status = STATUS_OBJECT_TYPE_MISMATCH;
                    CurrentObject = NULL;
                    break;
                }
            }
        }
    }

    /* Write what we found, and if it's null, check if we got success */
    if (!(*ReturnedObject = CurrentObject) && (NT_SUCCESS(Status)))
    {
        /* Nothing found... but we have success. Correct the status code */
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
    }

    /* Check if we had a root directory */
    if (RootHandle)
    {
        /* Dereference it */
        ObDereferenceObject(RootDirectory);
    }

    /* Return status to caller */
    OBTRACE(OB_NAMESPACE_DEBUG,
            "%s - Found Object: %p. Expected: %p\n",
            __FUNCTION__,
            *ReturnedObject,
            ExpectedObject);
    return Status;
}

/* PUBLIC FUNCTIONS *********************************************************/

NTSTATUS
NTAPI
ObQueryNameString(IN  PVOID Object,
                  OUT POBJECT_NAME_INFORMATION ObjectNameInfo,
                  IN  ULONG Length,
                  OUT PULONG ReturnLength)
{
    POBJECT_HEADER_NAME_INFO LocalInfo;
    POBJECT_HEADER ObjectHeader;
    POBJECT_DIRECTORY ParentDirectory;
    ULONG NameSize;
    PWCH ObjectName;

    /* Get the Kernel Meta-Structures */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
    LocalInfo = OBJECT_HEADER_TO_NAME_INFO(ObjectHeader);

    /* Check if a Query Name Procedure is available */
    if (ObjectHeader->Type->TypeInfo.QueryNameProcedure)
    {
        /* Call the procedure */
        return ObjectHeader->Type->TypeInfo.QueryNameProcedure(Object,
                                                               TRUE, //fixme
                                                               ObjectNameInfo,
                                                               Length,
                                                               ReturnLength,
                                                               KernelMode);
    }

    /* Check if the object doesn't even have a name */
    if (!LocalInfo || !LocalInfo->Name.Buffer)
    {
        /* We're returning the name structure */
        *ReturnLength = sizeof(OBJECT_NAME_INFORMATION);

        /* Check if we were given enough space */
        if (*ReturnLength > Length) return STATUS_INFO_LENGTH_MISMATCH;

        /* Return an empty buffer */
        RtlInitEmptyUnicodeString(&ObjectNameInfo->Name, NULL, 0);
        return STATUS_SUCCESS;
    }

    /*
     * Find the size needed for the name. We won't do
     * this during the Name Creation loop because we want
     * to let the caller know that the buffer isn't big
     * enough right at the beginning, not work our way through
     * and find out at the end
     */
    if (Object == NameSpaceRoot)
    {
        /* Size of the '\' string */
        NameSize = sizeof(OBJ_NAME_PATH_SEPARATOR);
    }
    else
    {
        /* Get the Object Directory and add name of Object */
        ParentDirectory = LocalInfo->Directory;
        NameSize = sizeof(OBJ_NAME_PATH_SEPARATOR) + LocalInfo->Name.Length;

        /* Loop inside the directory to get the top-most one (meaning root) */
        while ((ParentDirectory != NameSpaceRoot) && (ParentDirectory))
        {
            /* Get the Name Information */
            LocalInfo = OBJECT_HEADER_TO_NAME_INFO(
                            OBJECT_TO_OBJECT_HEADER(ParentDirectory));

            /* Add the size of the Directory Name */
            if (LocalInfo && LocalInfo->Directory)
            {
                /* Size of the '\' string + Directory Name */
                NameSize += sizeof(OBJ_NAME_PATH_SEPARATOR) +
                            LocalInfo->Name.Length;

                /* Move to next parent Directory */
                ParentDirectory = LocalInfo->Directory;
            }
            else
            {
                /* Directory with no name. We append "...\" */
                NameSize += sizeof(L"...") + sizeof(OBJ_NAME_PATH_SEPARATOR);
                break;
            }
        }
    }

    /* Finally, add the name of the structure and the null char */
    *ReturnLength = NameSize +
                    sizeof(OBJECT_NAME_INFORMATION) +
                    sizeof(UNICODE_NULL);

    /* Check if we were given enough space */
    if (*ReturnLength > Length) return STATUS_INFO_LENGTH_MISMATCH;

    /*
     * Now we will actually create the name. We work backwards because
     * it's easier to start off from the Name we have and walk up the
     * parent directories. We use the same logic as Name Length calculation.
     */
    LocalInfo = OBJECT_HEADER_TO_NAME_INFO(ObjectHeader);
    ObjectName = (PWCH)((ULONG_PTR)ObjectNameInfo + *ReturnLength);
    *--ObjectName = UNICODE_NULL;

    /* Check if the object is actually the Root directory */
    if (Object == NameSpaceRoot)
    {
        /* This is already the Root Directory, return "\\" */
        *--ObjectName = OBJ_NAME_PATH_SEPARATOR;
        ObjectNameInfo->Name.Length = (USHORT)NameSize;
        ObjectNameInfo->Name.MaximumLength = (USHORT)(NameSize +
                                                      sizeof(UNICODE_NULL));
        ObjectNameInfo->Name.Buffer = ObjectName;
        return STATUS_SUCCESS;
    }
    else
    {
        /* Start by adding the Object's Name */
        ObjectName = (PWCH)((ULONG_PTR)ObjectName -
                            LocalInfo->Name.Length);
        RtlMoveMemory(ObjectName,
                      LocalInfo->Name.Buffer,
                      LocalInfo->Name.Length);

        /* Now parse the Parent directories until we reach the top */
        ParentDirectory = LocalInfo->Directory;
        while ((ParentDirectory != NameSpaceRoot) && (ParentDirectory))
        {
            /* Get the name information */
            LocalInfo = OBJECT_HEADER_TO_NAME_INFO(
                            OBJECT_TO_OBJECT_HEADER(ParentDirectory));

            /* Add the "\" */
            *(--ObjectName) = OBJ_NAME_PATH_SEPARATOR;

            /* Add the Parent Directory's Name */
            if (LocalInfo && LocalInfo->Name.Buffer)
            {
                /* Add the name */
                ObjectName = (PWCH)((ULONG_PTR)ObjectName -
                                    LocalInfo->Name.Length);
                RtlMoveMemory(ObjectName,
                              LocalInfo->Name.Buffer,
                              LocalInfo->Name.Length);

                /* Move to next parent */
                ParentDirectory = LocalInfo->Directory;
            }
            else
            {
                /* Directory without a name, we add "..." */
                DPRINT("Nameless Directory\n");
                ObjectName -= sizeof(L"...");
                ObjectName = L"...";
                break;
            }
        }

        /* Add Root Directory Name */
        *(--ObjectName) = OBJ_NAME_PATH_SEPARATOR;
        ObjectNameInfo->Name.Length = (USHORT)NameSize;
        ObjectNameInfo->Name.MaximumLength = (USHORT)(NameSize +
                                                      sizeof(UNICODE_NULL));
        ObjectNameInfo->Name.Buffer = ObjectName;
    }

    /* Return success */
    return STATUS_SUCCESS;
}

VOID
NTAPI
ObQueryDeviceMapInformation(IN PEPROCESS Process,
                            IN PPROCESS_DEVICEMAP_INFORMATION DeviceMapInfo)
{
    //KIRQL OldIrql ;

    /*
    * FIXME: This is an ugly hack for now, to always return the System Device Map
    * instead of returning the Process Device Map. Not important yet since we don't use it
    */

    /* FIXME: Acquire the DeviceMap Spinlock */
    // KeAcquireSpinLock(DeviceMap->Lock, &OldIrql);

    /* Make a copy */
    DeviceMapInfo->Query.DriveMap = ObSystemDeviceMap->DriveMap;
    RtlMoveMemory(DeviceMapInfo->Query.DriveType,
                  ObSystemDeviceMap->DriveType,
                  sizeof(ObSystemDeviceMap->DriveType));

    /* FIXME: Release the DeviceMap Spinlock */
    // KeReleasepinLock(DeviceMap->Lock, OldIrql);
}

/* EOF */
