/*
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/cm/regobj.c
 * PURPOSE:         Registry object manipulation routines.
 *
 * PROGRAMMERS:     Hartmut Birr
 *                  Alex Ionescu
 *                  Rex Jolliff
 *                  Eric Kohl
 *                  Casper Hornstrup
*/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#include "cm.h"

extern LIST_ENTRY CmiKeyObjectListHead;
extern ULONG CmiTimer;

static NTSTATUS
CmiGetLinkTarget(PEREGISTRY_HIVE RegistryHive,
                 PCM_KEY_NODE KeyCell,
                 PUNICODE_STRING TargetPath);

/* FUNCTONS *****************************************************************/

PVOID
NTAPI
CmpLookupEntryDirectory(IN POBJECT_DIRECTORY Directory,
                        IN PUNICODE_STRING Name,
                        IN ULONG Attributes,
                        IN UCHAR SearchShadow,
                        IN POBP_LOOKUP_CONTEXT Context)
{
    BOOLEAN CaseInsensitive = FALSE;
    POBJECT_HEADER_NAME_INFO HeaderNameInfo;
    ULONG HashValue;
    ULONG HashIndex;
    LONG TotalChars;
    WCHAR CurrentChar;
    POBJECT_DIRECTORY_ENTRY *AllocatedEntry;
    POBJECT_DIRECTORY_ENTRY *LookupBucket;
    POBJECT_DIRECTORY_ENTRY CurrentEntry;
    PVOID FoundObject = NULL;
    PWSTR Buffer;
    PAGED_CODE();

    /* Always disable this until we have DOS Device Maps */
    SearchShadow = FALSE;

    /* Fail if we don't have a directory or name */
    if (!(Directory) || !(Name)) goto Quickie;

    /* Get name information */
    TotalChars = Name->Length / sizeof(WCHAR);
    Buffer = Name->Buffer;

    /* Fail if the name is empty */
    if (!(Buffer) || !(TotalChars)) goto Quickie;

    /* Set up case-sensitivity */
    if (Attributes & OBJ_CASE_INSENSITIVE) CaseInsensitive = TRUE;

    /* Create the Hash */
    for (HashValue = 0; TotalChars; TotalChars--)
    {
        /* Go to the next Character */
        CurrentChar = *Buffer++;

        /* Prepare the Hash */
        HashValue += (HashValue << 1) + (HashValue >> 1);

        /* Create the rest based on the name */
        if (CurrentChar < 'a') HashValue += CurrentChar;
        else if (CurrentChar > 'z') HashValue += RtlUpcaseUnicodeChar(CurrentChar);
        else HashValue += (CurrentChar - ('a'-'A'));
    }

    /* Merge it with our number of hash buckets */
    HashIndex = HashValue % 37;

    /* Save the result */
    Context->HashValue = HashValue;
    Context->HashIndex = (USHORT)HashIndex;

    /* Get the root entry and set it as our lookup bucket */
    AllocatedEntry = &Directory->HashBuckets[HashIndex];
    LookupBucket = AllocatedEntry;

    /* Start looping */
    while ((CurrentEntry = *AllocatedEntry))
    {
        /* Do the hashes match? */
        if (CurrentEntry->HashValue == HashValue)
        {
            /* Make sure that it has a name */
            ASSERT(OBJECT_TO_OBJECT_HEADER(CurrentEntry->Object)->NameInfoOffset != 0);

            /* Get the name information */
            HeaderNameInfo = OBJECT_HEADER_TO_NAME_INFO(OBJECT_TO_OBJECT_HEADER(CurrentEntry->Object));

            /* Do the names match? */
            if ((Name->Length == HeaderNameInfo->Name.Length) &&
                (RtlEqualUnicodeString(Name, &HeaderNameInfo->Name, CaseInsensitive)))
            {
                break;
            }
        }

        /* Move to the next entry */
        AllocatedEntry = &CurrentEntry->ChainLink;
    }

    /* Check if we still have an entry */
    if (CurrentEntry)
    {
        /* Set this entry as the first, to speed up incoming insertion */
        if (AllocatedEntry != LookupBucket)
        {
            /* Set the Current Entry */
            *AllocatedEntry = CurrentEntry->ChainLink;

            /* Link to the old Hash Entry */
            CurrentEntry->ChainLink = *LookupBucket;

            /* Set the new Hash Entry */
            *LookupBucket = CurrentEntry;
        }

        /* Save the found object */
        FoundObject = CurrentEntry->Object;
        if (!FoundObject) goto Quickie;
    }

Quickie:
    /* Return the object we found */
    Context->Object = FoundObject;
    return FoundObject;
}

NTSTATUS
NTAPI
CmFindObject(POBJECT_CREATE_INFORMATION ObjectCreateInfo,
             PUNICODE_STRING ObjectName,
             PVOID* ReturnedObject,
             PUNICODE_STRING RemainingPath,
             POBJECT_TYPE ObjectType,
             IN PACCESS_STATE AccessState,
             IN PVOID ParseContext)
{
    PVOID NextObject;
    PVOID CurrentObject;
    PVOID RootObject;
    POBJECT_HEADER CurrentHeader;
    NTSTATUS Status;
    PWSTR current;
    UNICODE_STRING PathString;
    ULONG Attributes;
    UNICODE_STRING CurrentUs;
    OBP_LOOKUP_CONTEXT Context;

    PAGED_CODE();

    DPRINT("CmFindObject(ObjectCreateInfo %x, ReturnedObject %x, "
        "RemainingPath %x)\n",ObjectCreateInfo,ReturnedObject,RemainingPath);

    RtlInitUnicodeString (RemainingPath, NULL);

    if (ObjectCreateInfo->RootDirectory == NULL)
    {
        ObReferenceObjectByPointer(ObpRootDirectoryObject,
            DIRECTORY_TRAVERSE,
            CmpKeyObjectType,
            ObjectCreateInfo->ProbeMode);
        CurrentObject = ObpRootDirectoryObject;
    }
    else
    {
        Status = ObReferenceObjectByHandle(ObjectCreateInfo->RootDirectory,
            0,
            NULL,
            ObjectCreateInfo->ProbeMode,
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

    if (ObjectCreateInfo->RootDirectory == NULL &&
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
    Attributes = ObjectCreateInfo->Attributes;
    if (ObjectType == ObSymbolicLinkType)
        Attributes |= OBJ_OPENLINK;
    Attributes |= OBJ_CASE_INSENSITIVE; // hello! My name is ReactOS CM and I'm brain-dead!

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
            ObpInitializeDirectoryLookup(&Context);
            Context.DirectoryLocked = TRUE;
            Context.Directory = CurrentObject;
            FoundObject = CmpLookupEntryDirectory(CurrentObject, &StartUs, Attributes, FALSE, &Context);
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
                CmpKeyObjectType,
                KernelMode);
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
            ExGetPreviousMode(), // fixme: should be a parameter, since caller decides.
            Attributes,
            &PathString,
            &CurrentUs,
            ParseContext,
            NULL, // fixme: where do we get this from? captured OBP?
            &NextObject);
        current = CurrentUs.Buffer;
        if (Status == STATUS_REPARSE)
        {
            /* reparse the object path */
            NextObject = ObpRootDirectoryObject;
            current = PathString.Buffer;

            ObReferenceObjectByPointer(NextObject,
                DIRECTORY_TRAVERSE,
                CmpKeyObjectType,
                ObjectCreateInfo->ProbeMode);
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
        RtlCreateUnicodeString(RemainingPath, current);
    }

    RtlFreeUnicodeString (&PathString);
    *ReturnedObject = CurrentObject;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CmpParseKey(IN PVOID ParsedObject,
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
    HCELL_INDEX BlockOffset;
    PKEY_OBJECT FoundObject;
    PKEY_OBJECT ParsedKey;
    PCM_KEY_NODE SubKeyCell;
    NTSTATUS Status;
    PWSTR StartPtr;
    PWSTR EndPtr;
    ULONG Length;
    UNICODE_STRING LinkPath;
    UNICODE_STRING TargetPath;
    UNICODE_STRING KeyName;
    PWSTR *Path = &RemainingName->Buffer;

    ParsedKey = ParsedObject;

    VERIFY_KEY_OBJECT(ParsedKey);

    *NextObject = NULL;

    if ((*Path) == NULL)
    {
        DPRINT("*Path is NULL\n");
        return STATUS_UNSUCCESSFUL;
    }

    DPRINT("Path '%S'\n", *Path);

    /* Extract relevant path name */
    StartPtr = *Path;
    if (*StartPtr == L'\\')
        StartPtr++;

    EndPtr = wcschr(StartPtr, L'\\');
    if (EndPtr != NULL)
        Length = ((PCHAR)EndPtr - (PCHAR)StartPtr) / sizeof(WCHAR);
    else
        Length = wcslen(StartPtr);

    KeyName.Length = (USHORT)Length * sizeof(WCHAR);
    KeyName.MaximumLength = (USHORT)KeyName.Length + sizeof(WCHAR);
    KeyName.Buffer = ExAllocatePool(NonPagedPool,
                                    KeyName.MaximumLength);
    RtlCopyMemory(KeyName.Buffer,
                  StartPtr,
                  KeyName.Length);
    KeyName.Buffer[KeyName.Length / sizeof(WCHAR)] = 0;

    /* Acquire hive lock */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&CmpRegistryLock, TRUE);

    Status = CmiScanKeyList(ParsedKey,
                            &KeyName,
                            Attributes,
                            &FoundObject);
    if (!NT_SUCCESS(Status))
    {
        ExReleaseResourceLite(&CmpRegistryLock);
        KeLeaveCriticalRegion();
        RtlFreeUnicodeString(&KeyName);
        return Status;
    }
    if (FoundObject == NULL)
    {
        Status = CmiScanForSubKey(ParsedKey->RegistryHive,
                                  ParsedKey->KeyCell,
                                  &SubKeyCell,
                                  &BlockOffset,
                                  &KeyName,
                                  0,
                                  Attributes);
        if (!NT_SUCCESS(Status))
        {
            ExReleaseResourceLite(&CmpRegistryLock);
            KeLeaveCriticalRegion();
            RtlFreeUnicodeString(&KeyName);
            return(STATUS_UNSUCCESSFUL);
        }

        if ((SubKeyCell->Flags & REG_KEY_LINK_CELL) &&
            !((Attributes & OBJ_OPENLINK) && (EndPtr == NULL)))
        {
            RtlInitUnicodeString(&LinkPath, NULL);
            Status = CmiGetLinkTarget(ParsedKey->RegistryHive,
                                      SubKeyCell,
                                      &LinkPath);
            if (NT_SUCCESS(Status))
            {
                ExReleaseResourceLite(&CmpRegistryLock);
                KeLeaveCriticalRegion();

                DPRINT("LinkPath '%wZ'\n", &LinkPath);

                /* build new FullPath for reparsing */
                TargetPath.MaximumLength = LinkPath.MaximumLength;
                if (EndPtr != NULL)
                {
                    TargetPath.MaximumLength += (wcslen(EndPtr) * sizeof(WCHAR));
                }
                TargetPath.Length = TargetPath.MaximumLength - sizeof(WCHAR);
                TargetPath.Buffer = ExAllocatePool(NonPagedPool,
                                                   TargetPath.MaximumLength);
                wcscpy(TargetPath.Buffer, LinkPath.Buffer);
                if (EndPtr != NULL)
                {
                    wcscat(TargetPath.Buffer, EndPtr);
                }

                RtlFreeUnicodeString(FullPath);
                RtlFreeUnicodeString(&LinkPath);
                FullPath->Length = TargetPath.Length;
                FullPath->MaximumLength = TargetPath.MaximumLength;
                FullPath->Buffer = TargetPath.Buffer;

                DPRINT("FullPath '%wZ'\n", FullPath);

                /* reinitialize Path for reparsing */
                *Path = FullPath->Buffer;

                *NextObject = NULL;

                RtlFreeUnicodeString(&KeyName);
                return(STATUS_REPARSE);
            }
        }

        /* Create new key object and put into linked list */
        DPRINT("CmpParseKey: %S\n", *Path);
        Status = ObCreateObject(KernelMode,
                                CmpKeyObjectType,
                                NULL,
                                KernelMode,
                                NULL,
                                sizeof(KEY_OBJECT),
                                0,
                                0,
                                (PVOID*)&FoundObject);
        if (!NT_SUCCESS(Status))
        {
            ExReleaseResourceLite(&CmpRegistryLock);
            KeLeaveCriticalRegion();
            RtlFreeUnicodeString(&KeyName);
            return(Status);
        }
#if 0
        DPRINT("Inserting Key into Object Tree\n");
        Status = ObInsertObject((PVOID)FoundObject,
                                NULL,
                                KEY_ALL_ACCESS,
                                0,
                                NULL,
                                NULL);
        DPRINT("Status %x\n", Status);
#else
        /* Free the create information */
        ObpFreeAndReleaseCapturedAttributes(OBJECT_TO_OBJECT_HEADER(FoundObject)->ObjectCreateInfo);
        OBJECT_TO_OBJECT_HEADER(FoundObject)->ObjectCreateInfo = NULL;
#endif

        /* Add the keep-alive reference */
        ObReferenceObject(FoundObject);

        FoundObject->Flags = 0;
        FoundObject->KeyCell = SubKeyCell;
        FoundObject->KeyCellOffset = BlockOffset;
        FoundObject->RegistryHive = ParsedKey->RegistryHive;
        InsertTailList(&CmiKeyObjectListHead, &FoundObject->ListEntry);
        RtlpCreateUnicodeString(&FoundObject->Name, KeyName.Buffer, NonPagedPool);
        CmiAddKeyToList(ParsedKey, FoundObject);
        DPRINT("Created object 0x%p\n", FoundObject);
    }
    else
    {
        if ((FoundObject->KeyCell->Flags & REG_KEY_LINK_CELL) &&
            !((Attributes & OBJ_OPENLINK) && (EndPtr == NULL)))
        {
            DPRINT("Found link\n");

            RtlInitUnicodeString(&LinkPath, NULL);
            Status = CmiGetLinkTarget(FoundObject->RegistryHive,
                                      FoundObject->KeyCell,
                                      &LinkPath);
            if (NT_SUCCESS(Status))
            {
                DPRINT("LinkPath '%wZ'\n", &LinkPath);

                ExReleaseResourceLite(&CmpRegistryLock);
                KeLeaveCriticalRegion();

                ObDereferenceObject(FoundObject);

                /* build new FullPath for reparsing */
                TargetPath.MaximumLength = LinkPath.MaximumLength;
                if (EndPtr != NULL)
                {
                    TargetPath.MaximumLength += (wcslen(EndPtr) * sizeof(WCHAR));
                }
                TargetPath.Length = TargetPath.MaximumLength - sizeof(WCHAR);
                TargetPath.Buffer = ExAllocatePool(NonPagedPool,
                                                   TargetPath.MaximumLength);
                wcscpy(TargetPath.Buffer, LinkPath.Buffer);
                if (EndPtr != NULL)
                {
                    wcscat(TargetPath.Buffer, EndPtr);
                }

                RtlFreeUnicodeString(FullPath);
                RtlFreeUnicodeString(&LinkPath);
                FullPath->Length = TargetPath.Length;
                FullPath->MaximumLength = TargetPath.MaximumLength;
                FullPath->Buffer = TargetPath.Buffer;

                DPRINT("FullPath '%wZ'\n", FullPath);

                /* reinitialize Path for reparsing */
                *Path = FullPath->Buffer;

                *NextObject = NULL;

                RtlFreeUnicodeString(&KeyName);
                return(STATUS_REPARSE);
            }
        }
    }

    RemoveEntryList(&FoundObject->ListEntry);
    InsertHeadList(&CmiKeyObjectListHead, &FoundObject->ListEntry);
    FoundObject->TimeStamp = CmiTimer;

    ExReleaseResourceLite(&CmpRegistryLock);
    KeLeaveCriticalRegion();

    DPRINT("CmpParseKey: %wZ\n", &FoundObject->Name);

    *Path = EndPtr;

    VERIFY_KEY_OBJECT(FoundObject);

    *NextObject = FoundObject;

    RtlFreeUnicodeString(&KeyName);

    return(STATUS_SUCCESS);
}

VOID
NTAPI
CmpDeleteKeyObject(PVOID DeletedObject)
{
    PKEY_OBJECT ParentKeyObject;
    PKEY_OBJECT KeyObject;
    REG_KEY_HANDLE_CLOSE_INFORMATION KeyHandleCloseInfo;
    REG_POST_OPERATION_INFORMATION PostOperationInfo;
    NTSTATUS Status;

    DPRINT("Delete key object (%p)\n", DeletedObject);

    KeyObject = (PKEY_OBJECT) DeletedObject;
    ParentKeyObject = KeyObject->ParentKey;

    ObReferenceObject (ParentKeyObject);

    PostOperationInfo.Object = (PVOID)KeyObject;
    KeyHandleCloseInfo.Object = (PVOID)KeyObject;
    Status = CmiCallRegisteredCallbacks(RegNtPreKeyHandleClose, &KeyHandleCloseInfo);
    if (!NT_SUCCESS(Status))
    {
        PostOperationInfo.Status = Status;
        CmiCallRegisteredCallbacks(RegNtPostKeyHandleClose, &PostOperationInfo);
        ObDereferenceObject (ParentKeyObject);
        return;
    }

    /* Acquire hive lock */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&CmpRegistryLock, TRUE);

    RemoveEntryList(&KeyObject->ListEntry);
    RtlFreeUnicodeString(&KeyObject->Name);

    ASSERT((KeyObject->Flags & KO_MARKED_FOR_DELETE) == FALSE);

    ObDereferenceObject (ParentKeyObject);

    if (KeyObject->SizeOfSubKeys)
    {
        ExFreePool(KeyObject->SubKeys);
    }

    ExReleaseResourceLite(&CmpRegistryLock);
    KeLeaveCriticalRegion();
    PostOperationInfo.Status = STATUS_SUCCESS;
    CmiCallRegisteredCallbacks(RegNtPostKeyHandleClose, &PostOperationInfo);
}

NTSTATUS
NTAPI
CmpQueryKeyName(PVOID ObjectBody,
                IN BOOLEAN HasName,
                POBJECT_NAME_INFORMATION ObjectNameInfo,
                ULONG Length,
                PULONG ReturnLength,
                IN KPROCESSOR_MODE PreviousMode)
{
    PKEY_OBJECT KeyObject;
    NTSTATUS Status;

    DPRINT ("CmpQueryKeyName() called\n");

    KeyObject = (PKEY_OBJECT)ObjectBody;

    if (KeyObject->ParentKey != KeyObject)
    {
        Status = ObQueryNameString (KeyObject->ParentKey,
                                    ObjectNameInfo,
                                    Length,
                                    ReturnLength);
    }
    else
    {
        /* KeyObject is the root key */
        Status = ObQueryNameString (OBJECT_HEADER_TO_NAME_INFO(OBJECT_TO_OBJECT_HEADER(KeyObject))->Directory,
                                    ObjectNameInfo,
                                    Length,
                                    ReturnLength);
    }

    if (!NT_SUCCESS(Status) && Status != STATUS_INFO_LENGTH_MISMATCH)
    {
        return Status;
    }
    (*ReturnLength) += sizeof(WCHAR) + KeyObject->Name.Length;

    if (Status == STATUS_INFO_LENGTH_MISMATCH || *ReturnLength > Length)
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    if (ObjectNameInfo->Name.Buffer == NULL)
    {
        ObjectNameInfo->Name.Buffer = (PWCHAR)(ObjectNameInfo + 1);
        ObjectNameInfo->Name.Length = 0;
        ObjectNameInfo->Name.MaximumLength = (USHORT)Length - sizeof(OBJECT_NAME_INFORMATION);
    }

    DPRINT ("Parent path: %wZ\n", ObjectNameInfo->Name);

    Status = RtlAppendUnicodeToString (&ObjectNameInfo->Name,
                                       L"\\");
    if (!NT_SUCCESS (Status))
        return Status;

    Status = RtlAppendUnicodeStringToString (&ObjectNameInfo->Name,
                                             &KeyObject->Name);
    if (NT_SUCCESS (Status))
    {
        DPRINT ("Total path: %wZ\n", &ObjectNameInfo->Name);
    }

    return Status;
}

VOID
CmiAddKeyToList(PKEY_OBJECT ParentKey,
                PKEY_OBJECT NewKey)
{
    DPRINT("ParentKey %.08x\n", ParentKey);

    if (ParentKey->SizeOfSubKeys <= ParentKey->SubKeyCounts)
    {
        PKEY_OBJECT *tmpSubKeys = ExAllocatePool(NonPagedPool,
            (ParentKey->SubKeyCounts + 1) * sizeof(ULONG));

        if (ParentKey->SubKeyCounts > 0)
        {
            RtlCopyMemory (tmpSubKeys,
                           ParentKey->SubKeys,
                           ParentKey->SubKeyCounts * sizeof(ULONG));
        }

        if (ParentKey->SubKeys)
            ExFreePool(ParentKey->SubKeys);

        ParentKey->SubKeys = tmpSubKeys;
        ParentKey->SizeOfSubKeys = ParentKey->SubKeyCounts + 1;
    }

    /* FIXME: Please maintain the list in alphabetic order */
    /*      to allow a dichotomic search */
    ParentKey->SubKeys[ParentKey->SubKeyCounts++] = NewKey;

    DPRINT("Reference parent key: 0x%p\n", ParentKey);

    ObReferenceObjectByPointer(ParentKey,
                               STANDARD_RIGHTS_REQUIRED,
                               CmpKeyObjectType,
                               KernelMode);
    NewKey->ParentKey = ParentKey;
}

/* Preconditions: Must be called with CmpRegistryLock held. */
NTSTATUS
CmiScanKeyList(PKEY_OBJECT Parent,
               CONST UNICODE_STRING* KeyName,
               ULONG Attributes,
               PKEY_OBJECT* ReturnedObject)
{
    PKEY_OBJECT CurKey = NULL;
    ULONG Index;

    DPRINT("Scanning key list for: %wZ (Parent: %wZ)\n",
        KeyName, &Parent->Name);

    /* FIXME: if list maintained in alphabetic order, use dichotomic search */
    /* (a binary search) */
    for (Index=0; Index < Parent->SubKeyCounts; Index++)
    {
        CurKey = Parent->SubKeys[Index];
        if (Attributes & OBJ_CASE_INSENSITIVE)
        {
            DPRINT("Comparing %wZ and %wZ\n", KeyName, &CurKey->Name);
            if ((KeyName->Length == CurKey->Name.Length)
                && (_wcsicmp(KeyName->Buffer, CurKey->Name.Buffer) == 0))
            {
                break;
            }
        }
        else
        {
            if ((KeyName->Length == CurKey->Name.Length)
                && (wcscmp(KeyName->Buffer, CurKey->Name.Buffer) == 0))
            {
                break;
            }
        }
    }

    if (Index < Parent->SubKeyCounts)
    {
        if (CurKey->Flags & KO_MARKED_FOR_DELETE)
        {
            CHECKPOINT;
            *ReturnedObject = NULL;
            return STATUS_UNSUCCESSFUL;
        }
        ObReferenceObject(CurKey);
        *ReturnedObject = CurKey;
    }
    else
    {
        *ReturnedObject = NULL;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS
CmiGetLinkTarget(PEREGISTRY_HIVE RegistryHive,
                 PCM_KEY_NODE KeyCell,
                 PUNICODE_STRING TargetPath)
{
    UNICODE_STRING LinkName = RTL_CONSTANT_STRING(L"SymbolicLinkValue");
    PCM_KEY_VALUE ValueCell;
    PVOID DataCell;
    NTSTATUS Status;

    DPRINT("CmiGetLinkTarget() called\n");

    /* Get Value block of interest */
    Status = CmiScanKeyForValue(RegistryHive,
                                KeyCell,
                                &LinkName,
                                &ValueCell,
                                NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CmiScanKeyForValue() failed (Status %lx)\n", Status);
        return(Status);
    }

    if (ValueCell->DataType != REG_LINK)
    {
        DPRINT1("Type != REG_LINK\n!");
        return(STATUS_UNSUCCESSFUL);
    }

    if (TargetPath->Buffer == NULL && TargetPath->MaximumLength == 0)
    {
        TargetPath->Length = 0;
        TargetPath->MaximumLength = (USHORT)ValueCell->DataSize + sizeof(WCHAR);
        TargetPath->Buffer = ExAllocatePool(NonPagedPool,
                                            TargetPath->MaximumLength);
    }

    TargetPath->Length = min((USHORT)TargetPath->MaximumLength - sizeof(WCHAR),
                         (USHORT)ValueCell->DataSize);

    if (ValueCell->DataSize > 0)
    {
        DataCell = HvGetCell (&RegistryHive->Hive, ValueCell->DataOffset);
        RtlCopyMemory(TargetPath->Buffer,
                      DataCell,
                      TargetPath->Length);
        TargetPath->Buffer[TargetPath->Length / sizeof(WCHAR)] = 0;
    }
    else
    {
        RtlCopyMemory(TargetPath->Buffer,
                      &ValueCell->DataOffset,
                      TargetPath->Length);
        TargetPath->Buffer[TargetPath->Length / sizeof(WCHAR)] = 0;
    }

    DPRINT("TargetPath '%wZ'\n", TargetPath);

    return(STATUS_SUCCESS);
}

/* EOF */
