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

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

POBJECT_DIRECTORY NameSpaceRoot = NULL;
POBJECT_DIRECTORY ObpTypeDirectoryObject = NULL;

/* PRIVATE FUNCTIONS *********************************************************/

NTSTATUS
NTAPI
ObFindObject(IN HANDLE RootHandle,
             IN PUNICODE_STRING ObjectName,
             IN ULONG Attributes,
             IN KPROCESSOR_MODE PreviousMode,
             IN PVOID *ReturnedObject,
             IN POBJECT_TYPE ObjectType,
             IN POBP_LOOKUP_CONTEXT Context,
             IN PACCESS_STATE AccessState,
             IN PSECURITY_QUALITY_OF_SERVICE SecurityQos,
             IN PVOID ParseContext,
             IN PVOID Insert)
{
    PVOID NextObject;
    PVOID CurrentObject;
    PVOID RootObject;
    POBJECT_HEADER CurrentHeader;
    NTSTATUS Status = STATUS_SUCCESS;
    PWSTR current;
    UNICODE_STRING PathString;
    UNICODE_STRING CurrentUs;
    UNICODE_STRING Path;
    PUNICODE_STRING RemainingPath = &Path;

    PAGED_CODE();

    DPRINT("ObFindObject(ObjectCreateInfo %x, ReturnedObject %x, "
        "RemainingPath %x)\n",ObjectCreateInfo,ReturnedObject,RemainingPath);

    RtlInitUnicodeString (RemainingPath, NULL);

    if (RootHandle == NULL)
    {
        ObReferenceObjectByPointer(NameSpaceRoot,
            DIRECTORY_TRAVERSE,
            NULL,
            PreviousMode);
        CurrentObject = NameSpaceRoot;
    }
    else
    {
        Status = ObReferenceObjectByHandle(RootHandle,
            0,
            NULL,
            PreviousMode,
            &CurrentObject,
            NULL);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    if (ObjectName->Length == 0 ||
        ObjectName->Buffer[0] == UNICODE_NULL)
    {
        *ReturnedObject = CurrentObject;
        return STATUS_SUCCESS;
    }

    if (RootHandle == NULL &&
        ObjectName->Buffer[0] != L'\\')
    {
        ObDereferenceObject (CurrentObject);
        DPRINT1("failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    /* Create a zero-terminated copy of the object name */
    PathString.Length = ObjectName->Length;
    PathString.MaximumLength = ObjectName->Length + sizeof(WCHAR);
    PathString.Buffer = ExAllocatePool (NonPagedPool,
        PathString.MaximumLength);
    if (PathString.Buffer == NULL)
    {
        ObDereferenceObject (CurrentObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory (PathString.Buffer,
        ObjectName->Buffer,
        ObjectName->Length);
    PathString.Buffer[PathString.Length / sizeof(WCHAR)] = UNICODE_NULL;

    current = PathString.Buffer;

    RootObject = CurrentObject;
    if (ObjectType == ObSymbolicLinkType)
        Attributes |= OBJ_OPENLINK;

    while (TRUE)
    {
        CurrentHeader = OBJECT_TO_OBJECT_HEADER(CurrentObject);

        /* Loop as long as we're dealing with a directory */
        while (CurrentHeader->Type == ObDirectoryType)
        {
            PWSTR Start, End;
            PVOID FoundObject;
            UNICODE_STRING StartUs;
            NextObject = NULL;

            if (!current) goto Next;

            Start = current;
            if (*Start == L'\\') Start++;

            End = wcschr(Start, L'\\');
            if (End != NULL) *End = 0;

            RtlInitUnicodeString(&StartUs, Start);
            Context->DirectoryLocked = TRUE;
            Context->Directory = CurrentObject;
            FoundObject = ObpLookupEntryDirectory(CurrentObject, &StartUs, Attributes, FALSE, Context);
            if (FoundObject == NULL)
            {
                if (End != NULL)
                {
                    *End = L'\\';
                }
                 goto Next;
            }

            ObReferenceObjectByPointer(FoundObject,
                STANDARD_RIGHTS_REQUIRED,
                NULL,
                UserMode);
            if (End != NULL)
            {
                *End = L'\\';
                current = End;
            }
            else
            {
                current = NULL;
            }

            NextObject = FoundObject;

Next:
            if (NextObject == NULL)
            {
                break;
            }
            ObDereferenceObject(CurrentObject);
            CurrentObject = NextObject;
            CurrentHeader = OBJECT_TO_OBJECT_HEADER(CurrentObject);
        }

        if (CurrentHeader->Type->TypeInfo.ParseProcedure == NULL)
        {
            DPRINT("Current object can't parse\n");
            break;
        }

        RtlInitUnicodeString(&CurrentUs, current);
        Status = CurrentHeader->Type->TypeInfo.ParseProcedure(CurrentObject,
            CurrentHeader->Type,
            AccessState,
            PreviousMode,
            Attributes,
            &PathString,
            &CurrentUs,
            ParseContext,
            SecurityQos,
            &NextObject);
        current = CurrentUs.Buffer;
        if (Status == STATUS_REPARSE)
        {
            /* reparse the object path */
            NextObject = NameSpaceRoot;
            current = PathString.Buffer;

            ObReferenceObjectByPointer(NextObject,
                DIRECTORY_TRAVERSE,
                NULL,
                PreviousMode);
        }


        if (NextObject == NULL)
        {
            break;
        }
        ObDereferenceObject(CurrentObject);
        CurrentObject = NextObject;
    }

    if (current)
    {
        RtlpCreateUnicodeString (RemainingPath, current, NonPagedPool);
    }

    RtlFreeUnicodeString (&PathString);
    *ReturnedObject = CurrentObject;

    /*
     * Icky hack: put the code that was in ObInsertObject here so that
     * we can get rid of the "RemainingPath" stuff, which shouldn't
     * be exposed outside of here.
     * Also makes the interface closer to NT parsing, and will make the
     * eventual changes easier to deal with
     */
    if (Insert)
    {
        PVOID FoundObject = NULL;
        POBJECT_HEADER Header = OBJECT_TO_OBJECT_HEADER(Insert);
        POBJECT_HEADER FoundHeader = NULL;
        BOOLEAN ObjectAttached = FALSE;
        FoundObject = *ReturnedObject;
        if (FoundObject)
        {
            FoundHeader = OBJECT_TO_OBJECT_HEADER(FoundObject);
        }

        if (FoundHeader && RemainingPath->Buffer == NULL)
        {
            DPRINT("Object exists\n");
            ObDereferenceObject(FoundObject);
            return STATUS_OBJECT_NAME_COLLISION;
        }

        if (FoundHeader && FoundHeader->Type == ObDirectoryType &&
            RemainingPath->Buffer)
        {
            /* The name was changed so let's update it */
            PVOID NewName;
            PWSTR BufferPos = RemainingPath->Buffer;
            ULONG Delta = 0;
            POBJECT_HEADER_NAME_INFO ObjectNameInfo;

            ObjectNameInfo = OBJECT_HEADER_TO_NAME_INFO(Header);

            if (BufferPos[0] == L'\\')
            {
                BufferPos++;
                Delta = sizeof(WCHAR);
            }
            NewName = ExAllocatePool(NonPagedPool, RemainingPath->MaximumLength - Delta);
            RtlMoveMemory(NewName, BufferPos, RemainingPath->MaximumLength - Delta);
            if (ObjectNameInfo->Name.Buffer) ExFreePool(ObjectNameInfo->Name.Buffer);
            ObjectNameInfo->Name.Buffer = NewName;
            ObjectNameInfo->Name.Length = RemainingPath->Length - Delta;
            ObjectNameInfo->Name.MaximumLength = RemainingPath->MaximumLength - Delta;
            ObpInsertEntryDirectory(FoundObject, Context, Header);
            ObjectAttached = TRUE;
        }

        if ((Header->Type == IoFileObjectType) ||
            (Header->Type->TypeInfo.OpenProcedure != NULL))
        {    
            DPRINT("About to call Open Routine\n");
            if (Header->Type == IoFileObjectType)
            {
                /* TEMPORARY HACK. DO NOT TOUCH -- Alex */
                DPRINT("Calling IopCreateFile: %x\n", FoundObject);
                Status = IopCreateFile(&Header->Body,
                    FoundObject,
                    RemainingPath->Buffer,            
                    NULL);
                DPRINT("Called IopCreateFile: %x\n", Status);

            }
            else if (Header->Type->TypeInfo.OpenProcedure != NULL)
            {
                DPRINT("Calling %x\n", Header->Type->TypeInfo.OpenProcedure);
                Status = Header->Type->TypeInfo.OpenProcedure(ObCreateHandle,
                    NULL,
                    &Header->Body,
                    0,
                    0);
            }

            if (!NT_SUCCESS(Status))
            {
                DPRINT("Create Failed\n");
                if (ObjectAttached == TRUE)
                {
                    ObpDeleteEntryDirectory(Context);
                }
                if (FoundObject)
                {
                    ObDereferenceObject(FoundObject);
                }
                RtlFreeUnicodeString(RemainingPath);
                return Status;
            }
        }
        RtlFreeUnicodeString(RemainingPath);
    }
    else
    {
        /* ROS Hack */
        DPRINT("REmaining path: %wZ\n", RemainingPath);
        if (RemainingPath->Buffer != NULL)
        {
            if (wcschr(RemainingPath->Buffer + 1, L'\\') == NULL)
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
            else
                Status =STATUS_OBJECT_PATH_NOT_FOUND;
        }
    }

    return Status;
}

/* PUBLIC FUNCTIONS *********************************************************/

NTSTATUS 
STDCALL
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
                                                               ReturnLength);
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
