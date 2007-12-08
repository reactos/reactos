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

extern ULONG CmiTimer;

static NTSTATUS
CmiGetLinkTarget(PCMHIVE RegistryHive,
                 PCM_KEY_NODE KeyCell,
                 PUNICODE_STRING TargetPath);

/* FUNCTONS *****************************************************************/

NTSTATUS
NTAPI
CmpCreateHandle(PVOID ObjectBody,
                ACCESS_MASK GrantedAccess,
                ULONG HandleAttributes,
                PHANDLE HandleReturn)
/*
 * FUNCTION: Add a handle referencing an object
 * ARGUMENTS:
 *         obj = Object body that the handle should refer to
 * RETURNS: The created handle
 * NOTE: The handle is valid only in the context of the current process
 */
{
    HANDLE_TABLE_ENTRY NewEntry;
    PEPROCESS CurrentProcess;
    PVOID HandleTable;
    POBJECT_HEADER ObjectHeader;
    HANDLE Handle;
    KAPC_STATE ApcState;
    BOOLEAN AttachedToProcess = FALSE;
    
    PAGED_CODE();
    
    DPRINT("CmpCreateHandle(obj %p)\n",ObjectBody);
    
    ASSERT(ObjectBody);
    
    CurrentProcess = PsGetCurrentProcess();
    
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(ObjectBody);
    
    /* check that this is a valid kernel pointer */
    //ASSERT((ULONG_PTR)ObjectHeader & EX_HANDLE_ENTRY_LOCKED);
    
    if (GrantedAccess & MAXIMUM_ALLOWED)
    {
        GrantedAccess &= ~MAXIMUM_ALLOWED;
        GrantedAccess |= GENERIC_ALL;
    }
    
    if (GrantedAccess & GENERIC_ACCESS)
    {
        RtlMapGenericMask(&GrantedAccess,
                          &ObjectHeader->Type->TypeInfo.GenericMapping);
    }
    
    NewEntry.Object = ObjectHeader;
    if(HandleAttributes & OBJ_INHERIT)
        NewEntry.ObAttributes |= OBJ_INHERIT;
    else
        NewEntry.ObAttributes &= ~OBJ_INHERIT;
    NewEntry.GrantedAccess = GrantedAccess;
    
    if ((HandleAttributes & OBJ_KERNEL_HANDLE) &&
        ExGetPreviousMode() == KernelMode)
    {
        HandleTable = ObpKernelHandleTable;
        if (PsGetCurrentProcess() != PsInitialSystemProcess)
        {
            KeStackAttachProcess(&PsInitialSystemProcess->Pcb,
                                 &ApcState);
            AttachedToProcess = TRUE;
        }
    }
    else
    {
        HandleTable = PsGetCurrentProcess()->ObjectTable;
    }
    
    Handle = ExCreateHandle(HandleTable,
                            &NewEntry);
    
    if (AttachedToProcess)
    {
        KeUnstackDetachProcess(&ApcState);
    }
    
    if(Handle != NULL)
    {
        if (HandleAttributes & OBJ_KERNEL_HANDLE)
        {
            /* mark the handle value */
            Handle = ObMarkHandleAsKernelHandle(Handle);
        }
        
        InterlockedIncrement(&ObjectHeader->HandleCount);
        ObReferenceObject(ObjectBody);
        
        *HandleReturn = Handle;
        
        return STATUS_SUCCESS;
    }
    
    return STATUS_UNSUCCESSFUL;
}

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

/* Preconditions: Must be called with CmpRegistryLock held. */
NTSTATUS
CmiScanKeyList(PCM_KEY_CONTROL_BLOCK Parent,
               PCUNICODE_STRING KeyName,
               ULONG Attributes,
               PCM_KEY_BODY* ReturnedObject)
{
    PCM_KEY_BODY CurKey = NULL;
    PCM_NAME_CONTROL_BLOCK Ncb;
    PLIST_ENTRY NextEntry;
    PWCHAR p, pp;
    ULONG i;
    *ReturnedObject = NULL;
    
    /* Loop child keys in the KCB */
    NextEntry = Parent->KeyBodyListHead.Flink;
    while (NextEntry != &Parent->KeyBodyListHead)
    {
        /* Get the current ReactOS Key Object */
        CurKey = CONTAINING_RECORD(NextEntry, CM_KEY_BODY, KeyBodyList);
        
        /* Get the NCB */
        Ncb = CurKey->KeyControlBlock->NameBlock;
        
        /* Check if the key is compressed */
        if (Ncb->Compressed)
        {
            /* Do a compressed compare */
            if (!CmpCompareCompressedName(KeyName,
                                          Ncb->Name,
                                          Ncb->NameLength))
            {
                /* We got it */
                break;
            }
        }
        else
        {
            /* Do a manual compare */
            p = KeyName->Buffer;
            pp = Ncb->Name;
            for (i = 0; i < Ncb->NameLength; i += sizeof(WCHAR))
            {
                /* Compare the character */
                if (RtlUpcaseUnicodeChar(*p) != RtlUpcaseUnicodeChar(*pp))
                {
                    /* Failed */
                    break;
                }
                
                /* Next chars */
                p++;
                pp++;
            }
            
            /* Did we find it? */
            if (i == Ncb->NameLength - 1)
            {
                /* We found it, break out */
                break;
            }
        }
        
        /* Go go the next entry */
        NextEntry = NextEntry->Flink;
    }
    
    /* Check if we got here and found something */
    if (NextEntry != &Parent->KeyBodyListHead)
    {
        /* Refernece the object and return it */
        ObReferenceObject(CurKey);
        *ReturnedObject = CurKey;
    }
    
    /* Return success */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CmpParseKey2(IN PVOID ParseObject,
             IN PVOID ObjectType,
             IN OUT PACCESS_STATE AccessState,
             IN KPROCESSOR_MODE AccessMode,
             IN ULONG Attributes,
             IN OUT PUNICODE_STRING CompleteName,
             IN OUT PUNICODE_STRING RemainingName,
             IN OUT PVOID Context OPTIONAL,
             IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
             OUT PVOID *Object);

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
    PCM_KEY_BODY FoundObject;
    PCM_KEY_BODY ParsedKey;
    PCM_KEY_NODE SubKeyCell;
    NTSTATUS Status;
    PWSTR StartPtr;
    PWSTR EndPtr;
    ULONG Length;
    UNICODE_STRING LinkPath;
    UNICODE_STRING TargetPath;
    UNICODE_STRING KeyName;
    PWSTR *Path = &RemainingName->Buffer;
    PCM_KEY_CONTROL_BLOCK ParentKcb = NULL, Kcb;
    PCM_KEY_NODE Node;
    
    /* Detect new-style parse */
    if (Context)
    {
        /* Call proper parse routine */
        return CmpParseKey2(ParsedObject,
                            ObjectType,
                            AccessState,
                            AccessMode,
                            Attributes,
                            FullPath,
                            RemainingName,
                            Context,
                            SecurityQos,
                            NextObject);
    }
    
    ParsedKey = ParsedObject;

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

    Status = CmiScanKeyList(ParsedKey->KeyControlBlock,
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

    ParentKcb = ParsedKey->KeyControlBlock;

    if (FoundObject == NULL)
    {
        /* Search for the subkey */
        Node = (PCM_KEY_NODE)HvGetCell(ParsedKey->KeyControlBlock->KeyHive,
                                       ParsedKey->KeyControlBlock->KeyCell);
        
        BlockOffset = CmpFindSubKeyByName(ParsedKey->KeyControlBlock->KeyHive,
                                          Node,
                                          &KeyName);
        if (BlockOffset == HCELL_NIL)
        {
            ExReleaseResourceLite(&CmpRegistryLock);
            KeLeaveCriticalRegion();
            RtlFreeUnicodeString(&KeyName);
            return(STATUS_UNSUCCESSFUL);
        }
        
        /* Get the node */
        SubKeyCell = (PCM_KEY_NODE)HvGetCell(ParsedKey->KeyControlBlock->KeyHive, BlockOffset);

        if ((SubKeyCell->Flags & KEY_SYM_LINK) &&
            !((Attributes & OBJ_OPENLINK) && (EndPtr == NULL)))
        {
            RtlInitUnicodeString(&LinkPath, NULL);
            Status = CmiGetLinkTarget((PCMHIVE)ParsedKey->KeyControlBlock->KeyHive,
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
                                sizeof(CM_KEY_BODY),
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
        
        /* Create the KCB */
        Kcb = CmpCreateKeyControlBlock(ParsedKey->KeyControlBlock->KeyHive,
                                       BlockOffset,
                                       SubKeyCell,
                                       ParentKcb,
                                       0,
                                       &KeyName);
        if (!Kcb)
        {
            ExReleaseResourceLite(&CmpRegistryLock);
            KeLeaveCriticalRegion();
            RtlFreeUnicodeString(&KeyName);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
                                  
        FoundObject->KeyControlBlock = Kcb;
        ASSERT(FoundObject->KeyControlBlock->KeyHive == ParsedKey->KeyControlBlock->KeyHive);
        InsertTailList(&ParsedKey->KeyControlBlock->KeyBodyListHead, &FoundObject->KeyBodyList);
        DPRINT("Created object 0x%p\n", FoundObject);
    }
    else
    {
        Node = (PCM_KEY_NODE)HvGetCell(FoundObject->KeyControlBlock->KeyHive,
                                       FoundObject->KeyControlBlock->KeyCell);
        
        if ((Node->Flags & KEY_SYM_LINK) &&
            !((Attributes & OBJ_OPENLINK) && (EndPtr == NULL)))
        {
            DPRINT("Found link\n");

            RtlInitUnicodeString(&LinkPath, NULL);
            Status = CmiGetLinkTarget((PCMHIVE)FoundObject->KeyControlBlock->KeyHive,
                                      Node,
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

    ExReleaseResourceLite(&CmpRegistryLock);
    KeLeaveCriticalRegion();

    *Path = EndPtr;


    *NextObject = FoundObject;

    RtlFreeUnicodeString(&KeyName);

    return(STATUS_SUCCESS);
}

static NTSTATUS
CmiGetLinkTarget(PCMHIVE RegistryHive,
                 PCM_KEY_NODE KeyCell,
                 PUNICODE_STRING TargetPath)
{
    UNICODE_STRING LinkName = RTL_CONSTANT_STRING(L"SymbolicLinkValue");
    PCM_KEY_VALUE ValueCell;
    PVOID DataCell;
    HCELL_INDEX Cell;

    DPRINT("CmiGetLinkTarget() called\n");

    /* Find the cell */
    Cell = CmpFindValueByName(&RegistryHive->Hive, KeyCell, &LinkName);
    if (Cell == HCELL_NIL)
    {
        DPRINT1("CmiScanKeyForValue() failed\n");
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }
    
    /* Get the cell data */
    ValueCell = (PCM_KEY_VALUE)HvGetCell(&RegistryHive->Hive, Cell);
    if (ValueCell->Type != REG_LINK)
    {
        DPRINT1("Type != REG_LINK\n!");
        return(STATUS_UNSUCCESSFUL);
    }

    if (TargetPath->Buffer == NULL && TargetPath->MaximumLength == 0)
    {
        TargetPath->Length = 0;
        TargetPath->MaximumLength = (USHORT)ValueCell->DataLength + sizeof(WCHAR);
        TargetPath->Buffer = ExAllocatePool(NonPagedPool,
                                            TargetPath->MaximumLength);
    }

    TargetPath->Length = min((USHORT)TargetPath->MaximumLength - sizeof(WCHAR),
                         (USHORT)ValueCell->DataLength);

    if (ValueCell->DataLength > 0)
    {
        DataCell = HvGetCell (&RegistryHive->Hive, ValueCell->Data);
        RtlCopyMemory(TargetPath->Buffer,
                      DataCell,
                      TargetPath->Length);
        TargetPath->Buffer[TargetPath->Length / sizeof(WCHAR)] = 0;
    }
    else
    {
        RtlCopyMemory(TargetPath->Buffer,
                      &ValueCell->Data,
                      TargetPath->Length);
        TargetPath->Buffer[TargetPath->Length / sizeof(WCHAR)] = 0;
    }

    DPRINT("TargetPath '%wZ'\n", TargetPath);

    return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
NtCreateKey(OUT PHANDLE KeyHandle,
            IN ACCESS_MASK DesiredAccess,
            IN POBJECT_ATTRIBUTES ObjectAttributes,
            IN ULONG TitleIndex,
            IN PUNICODE_STRING Class,
            IN ULONG CreateOptions,
            OUT PULONG Disposition)
{
    UNICODE_STRING RemainingPath = {0}, ReturnedPath = {0};
    ULONG LocalDisposition;
    PCM_KEY_BODY KeyObject, Parent;
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING ObjectName;
    OBJECT_CREATE_INFORMATION ObjectCreateInfo;
    ULONG i;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    HANDLE hKey;
    PCM_KEY_NODE Node, ParentNode;
    CM_PARSE_CONTEXT ParseContext = {0};
    PAGED_CODE();
    
    /* Setup the parse context */
    ParseContext.CreateOperation = TRUE;
    ParseContext.CreateOptions = CreateOptions;
    if (Class) ParseContext.Class = *Class;
    
    /* Capture all the info */
    Status = ObpCaptureObjectAttributes(ObjectAttributes,
                                        PreviousMode,
                                        FALSE,
                                        &ObjectCreateInfo,
                                        &ObjectName);
    if (!NT_SUCCESS(Status)) return Status;
    
    /* Find the key object */
    Status = CmFindObject(&ObjectCreateInfo,
                          &ObjectName,
                          (PVOID*)&Parent,
                          &ReturnedPath,
                          CmpKeyObjectType,
                          NULL,
                          NULL);
    if (!NT_SUCCESS(Status)) goto Cleanup;
    
    /* Check if we found the entire path */
    RemainingPath = ReturnedPath;
    if (!RemainingPath.Length)
    {
        /* Check if the parent has been deleted */
        if (Parent->KeyControlBlock->Delete)
        {
            /* Fail */
            DPRINT1("Object marked for delete!\n");
            Status = STATUS_UNSUCCESSFUL;
            goto Cleanup;
        }
        
        /* Create a new handle to the parent */
        Status = CmpCreateHandle(Parent,
                                 DesiredAccess,
                                 ObjectCreateInfo.Attributes,
                                 &hKey);
        if (!NT_SUCCESS(Status)) goto Cleanup;
        
        /* Tell the caller we did this */
        LocalDisposition = REG_OPENED_EXISTING_KEY;
        goto SuccessReturn;
    }
    
    /* Loop every leading slash */
    while ((RemainingPath.Length) &&
           (*RemainingPath.Buffer == OBJ_NAME_PATH_SEPARATOR))
    {
        /* And remove it */
        RemainingPath.Length -= sizeof(WCHAR);
        RemainingPath.MaximumLength -= sizeof(WCHAR);
        RemainingPath.Buffer++;
    }
    
    /* Loop every terminating slash */
    while ((RemainingPath.Length) && 
           (RemainingPath.Buffer[(RemainingPath.Length / sizeof(WCHAR)) - 1] ==
            OBJ_NAME_PATH_SEPARATOR))
    {
        /* And remove it */
        RemainingPath.Length -= sizeof(WCHAR);
        RemainingPath.MaximumLength -= sizeof(WCHAR);
    }
    
    /* Now loop the entire path */
    for (i = 0; i < RemainingPath.Length / sizeof(WCHAR); i++)
    {
        /* And check if we found slahes */
        if (RemainingPath.Buffer[i] == OBJ_NAME_PATH_SEPARATOR)
        {
            /* We don't create trees -- parent key doesn't exist, so fail */
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
            goto Cleanup;
        }
    }
    
    /* Now check if we're left with no name by this point */
    if (!(RemainingPath.Length) || (RemainingPath.Buffer[0] == UNICODE_NULL))
    {
        /* Then fail since we can't do anything */
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto Cleanup;
    }
    
    /* Lock the registry */
    CmpLockRegistry();
    
    /* Create the key */
    Status = CmpDoCreate(Parent->KeyControlBlock->KeyHive,
                         Parent->KeyControlBlock->KeyCell,
                         NULL,
                         &RemainingPath,
                         KernelMode,
                         &ParseContext,
                         Parent->KeyControlBlock,
                         (PVOID*)&KeyObject);
    if (!NT_SUCCESS(Status)) goto Cleanup;
    
    /* If we got here, this is a new key */
    LocalDisposition = REG_CREATED_NEW_KEY;
    
    /* Get the parent node and the child node */
    ParentNode = (PCM_KEY_NODE)HvGetCell(KeyObject->KeyControlBlock->ParentKcb->KeyHive,
                                         KeyObject->KeyControlBlock->ParentKcb->KeyCell);
    Node = (PCM_KEY_NODE)HvGetCell(KeyObject->KeyControlBlock->KeyHive,
                                   KeyObject->KeyControlBlock->KeyCell);
    
    /* Inherit some information */
    Node->Parent = KeyObject->KeyControlBlock->ParentKcb->KeyCell;
    Node->Security = ParentNode->Security;
    KeyObject->KeyControlBlock->ValueCache.ValueList = Node->ValueList.List;
    KeyObject->KeyControlBlock->ValueCache.Count = Node->ValueList.Count;
    
    /* Link child to parent */
    InsertTailList(&Parent->KeyControlBlock->KeyBodyListHead, &KeyObject->KeyBodyList);
    
    /* Create the actual handle to the object */
    Status = CmpCreateHandle(KeyObject,
                             DesiredAccess,
                             ObjectCreateInfo.Attributes,
                             &hKey);
    
    /* Free the create information */
    ObpFreeAndReleaseCapturedAttributes(OBJECT_TO_OBJECT_HEADER(KeyObject)->ObjectCreateInfo);
    OBJECT_TO_OBJECT_HEADER(KeyObject)->ObjectCreateInfo = NULL;
    if (!NT_SUCCESS(Status)) goto Cleanup;
    
    /* Add the keep-alive reference */
    ObReferenceObject(KeyObject);
    
    /* Unlock registry */
    CmpUnlockRegistry();
    
    /* Force a lazy flush */
    CmpLazyFlush();
    
SuccessReturn:
    /* Return data to user */
    *KeyHandle = hKey;
    if (Disposition) *Disposition = LocalDisposition;
    
Cleanup:
    /* Cleanup */
    ObpReleaseCapturedAttributes(&ObjectCreateInfo);
    if (ObjectName.Buffer) ObpFreeObjectNameBuffer(&ObjectName);
    RtlFreeUnicodeString(&ReturnedPath);
    if (Parent) ObDereferenceObject(Parent);
    return Status;
}

/* EOF */
