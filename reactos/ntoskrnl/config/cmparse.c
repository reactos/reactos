/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmparse.c
 * PURPOSE:         Configuration Manager - Object Manager Parse Interface
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
#include "cm.h"
#define NDEBUG
#include "debug.h"

/* GLOBALS *******************************************************************/

/* FUNCTIONS *****************************************************************/

BOOLEAN
NTAPI
CmpGetNextName(IN OUT PUNICODE_STRING RemainingName,
               OUT PUNICODE_STRING NextName,
               OUT PBOOLEAN LastName)
{
    BOOLEAN NameValid = TRUE;

    /* Check if there's nothing left in the name */
    if (!(RemainingName->Buffer) ||
        (!RemainingName->Length) ||
        !(*RemainingName->Buffer))
    {
        /* Clear the next name and set this as last */
        *LastName = TRUE;
        NextName->Buffer = NULL;
        NextName->Length = 0;
        return TRUE;
    }

    /* Check if we have a path separator */
    if (*RemainingName->Buffer == OBJ_NAME_PATH_SEPARATOR)
    {
        /* Skip it */
        RemainingName->Buffer++;
        RemainingName->Length -= sizeof(WCHAR);
        RemainingName->MaximumLength -= sizeof(WCHAR);
    }

    /* Start loop at where the current buffer is */
    NextName->Buffer = RemainingName->Buffer;
    while (TRUE)
    {
        /* Break out if we ran out or hit a path separator */
        if (!(RemainingName->Length) ||
            (*RemainingName->Buffer == OBJ_NAME_PATH_SEPARATOR))
        {
            break;
        }

        /* Move to the next character */
        RemainingName->Buffer++;
        RemainingName->Length -= sizeof(WCHAR);
        RemainingName->MaximumLength -= sizeof(WCHAR);
    }

    /* See how many chars we parsed and validate the length */
    NextName->Length = (USHORT)((ULONG_PTR)RemainingName->Buffer -
                                (ULONG_PTR)NextName->Buffer);
    if (NextName->Length > 512) NameValid = FALSE;
    NextName->MaximumLength = NextName->Length;

    /* If there's nothing left, we're last */
    *LastName = !RemainingName->Length;
    return NameValid;
}

BOOLEAN
NTAPI
CmpGetSymbolicLink(IN PHHIVE Hive,
                   IN OUT PUNICODE_STRING ObjectName,
                   IN OUT PCM_KEY_CONTROL_BLOCK SymbolicKcb,
                   IN PUNICODE_STRING RemainingName OPTIONAL)
{
    HCELL_INDEX LinkCell = HCELL_NIL;
    PCM_KEY_VALUE LinkValue = NULL;
    PWSTR LinkName = NULL;
    BOOLEAN LinkNameAllocated = FALSE;
    PWSTR NewBuffer;
    ULONG Length = 0;
    ULONG ValueLength = 0;
    BOOLEAN Result = FALSE;
    HCELL_INDEX CellToRelease = HCELL_NIL;
    PCM_KEY_NODE Node;
    UNICODE_STRING NewObjectName;

    /* Make sure we're not being deleted */
    if (SymbolicKcb->Delete) return FALSE;

    /* Get the key node */
    Node = (PCM_KEY_NODE)HvGetCell(SymbolicKcb->KeyHive, SymbolicKcb->KeyCell);
    if (!Node) goto Exit;

    /* Find the symbolic link key */
    LinkCell = CmpFindValueByName(Hive, Node, &CmSymbolicLinkValueName);
    HvReleaseCell(SymbolicKcb->KeyHive, SymbolicKcb->KeyCell);
    if (LinkCell == HCELL_NIL) goto Exit;

    /* Get the value cell */
    LinkValue = (PCM_KEY_VALUE)HvGetCell(Hive, LinkCell);
    if (!LinkValue) goto Exit;

    /* Make sure it's a registry link */
    if (LinkValue->Type != REG_LINK) goto Exit;

    /* Now read the value data */
    if (!CmpGetValueData(Hive,
                         LinkValue,
                         &ValueLength,
                         (PVOID)&LinkName,
                         &LinkNameAllocated,
                         &CellToRelease))
    {
        /* Fail */
        goto Exit;
    }

    /* Get the length */
    Length = ValueLength + sizeof(WCHAR);

    /* Make sure we start with a slash */
    if (*LinkName != OBJ_NAME_PATH_SEPARATOR) goto Exit;

    /* Add the remaining name if needed */
    if (RemainingName) Length += RemainingName->Length + sizeof(WCHAR);
    
    /* Check for overflow */
    if (Length > 0xFFFF) goto Exit;

    /* Check if we need a new buffer */
	if (Length > ObjectName->MaximumLength)
    {
        /* We do -- allocate one */
        NewBuffer = ExAllocatePoolWithTag(PagedPool, Length, TAG_CM);
        if (!NewBuffer) goto Exit;
        
        /* Setup the new string and copy the symbolic target */
        NewObjectName.Buffer = NewBuffer;
        NewObjectName.MaximumLength = (USHORT)Length;
        NewObjectName.Length = (USHORT)ValueLength;
        RtlCopyMemory(NewBuffer, LinkName, ValueLength);

        /* Check if we need to add anything else */
        if (RemainingName)
        {
            /* Add the remaining name */
            NewBuffer[ValueLength / sizeof(WCHAR)] = OBJ_NAME_PATH_SEPARATOR;
            NewObjectName.Length += sizeof(WCHAR);
            RtlAppendUnicodeStringToString(&NewObjectName, RemainingName);
        }

        /* Free the old buffer */
        ExFreePool(ObjectName->Buffer);
        *ObjectName = NewObjectName;
    }
    else
    {
        /* The old name is large enough -- update the length */
        ObjectName->Length = (USHORT)ValueLength;
        if (RemainingName)
        {
            /* Copy the remaining name inside */
            RtlMoveMemory(&ObjectName->Buffer[(ValueLength / sizeof(WCHAR)) + 1],
                          RemainingName->Buffer,
                          RemainingName->Length);

            /* Add the slash and update the length */
            ObjectName->Buffer[ValueLength / sizeof(WCHAR)] = OBJ_NAME_PATH_SEPARATOR;
            ObjectName->Length += RemainingName->Length + sizeof(WCHAR);
        }

        /* Copy the symbolic link target name */
        RtlCopyMemory(ObjectName->Buffer, LinkName, ValueLength);
    }

    /* Null-terminate the whole thing */
    ObjectName->Buffer[ObjectName->Length / sizeof(WCHAR)] = UNICODE_NULL;
    Result = TRUE;

Exit:
    /* Free the link name */
    if (LinkNameAllocated) ExFreePool(LinkName);

    /* Check if we had a value cell */
    if (LinkValue)
    {
        /* Release it */
        ASSERT(LinkCell != HCELL_NIL);
        HvReleaseCell(Hive, LinkCell);
    }

    /* Check if we had an active cell and release it, then return the result */
    if (CellToRelease != HCELL_NIL) HvReleaseCell(Hive, CellToRelease);
    return Result;
}

NTSTATUS
NTAPI
CmpDoCreateChild(IN PHHIVE Hive,
                 IN HCELL_INDEX ParentCell,
                 IN PSECURITY_DESCRIPTOR ParentDescriptor OPTIONAL,
                 IN PACCESS_STATE AccessState,
                 IN PUNICODE_STRING Name,
                 IN KPROCESSOR_MODE AccessMode,
                 IN PCM_PARSE_CONTEXT ParseContext,
                 IN PCM_KEY_CONTROL_BLOCK ParentKcb,
                 IN ULONG Flags,
                 OUT PHCELL_INDEX KeyCell,
                 OUT PVOID *Object)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCM_KEY_BODY KeyBody;
    HCELL_INDEX ClassCell = HCELL_NIL;
    PCM_KEY_NODE KeyNode;
    PCELL_DATA CellData;
    ULONG StorageType;
    LARGE_INTEGER SystemTime;
    BOOLEAN Hack = FALSE;
    PCM_KEY_CONTROL_BLOCK Kcb;

    /* ReactOS Hack */
    if (Name->Buffer[0] == OBJ_NAME_PATH_SEPARATOR)
    {
        /* Skip initial path separator */
        Name->Buffer++;
        Name->Length -= sizeof(OBJ_NAME_PATH_SEPARATOR);
        Name->MaximumLength -= sizeof(OBJ_NAME_PATH_SEPARATOR);
        Hack = TRUE;
    }

    /* Get the storage type */
    StorageType = Stable;
    if (Flags & REG_OPTION_VOLATILE) StorageType = Volatile;

    /* Allocate the child */
    *KeyCell = HvAllocateCell(Hive,
                              FIELD_OFFSET(CM_KEY_NODE, Name) +
                              CmpNameSize(Hive, Name),
                              StorageType,
                              HCELL_NIL);
    if (*KeyCell == HCELL_NIL)
    {
        /* Fail */
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Quickie;
    }

    /* Get the key node */
    KeyNode = (PCM_KEY_NODE)HvGetCell(Hive, *KeyCell);
    if (!KeyNode)
    {
        /* Fail, this should never happen */
        ASSERT(FALSE);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Quickie;
    }

    /* Release the cell */
    HvReleaseCell(Hive, *KeyCell);

    /* Check if we have a class name */
    if (ParseContext->Class.Length > 0)
    {
        /* Allocate a class cell */
        ClassCell = HvAllocateCell(Hive,
                                   ParseContext->Class.Length,
                                   StorageType,
                                   HCELL_NIL);
        if (ClassCell == HCELL_NIL)
        {
            /* Fail */
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Quickie;
        }
    }

    /* Allocate the Cm Object */
    Status = ObCreateObject(AccessMode,
                            CmpKeyObjectType,
                            NULL,
                            AccessMode,
                            NULL,
                            sizeof(CM_KEY_BODY),
                            0,
                            0,
                            Object);
    if (!NT_SUCCESS(Status)) goto Quickie;
    
    /* Setup the key body */
    KeyBody = (PCM_KEY_BODY)(*Object);
    KeyBody->Type = TAG('k', 'y', '0', '2');
    KeyBody->KeyControlBlock = NULL;

    /* Check if we had a class */
    if (ParseContext->Class.Length > 0)
    {
        /* Get the class cell */
        CellData = HvGetCell(Hive, ClassCell);
        if (!CellData)
        {
            /* Fail, this should never happen */
            ASSERT(FALSE);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            ObDereferenceObject(*Object);
            goto Quickie;
        }

        /* Release the cell */
        HvReleaseCell(Hive, ClassCell);

        /* Copy the class data */
        RtlCopyMemory(&CellData->u.KeyString[0],
                      ParseContext->Class.Buffer,
                      ParseContext->Class.Length);
    }

    /* Fill out the key node */
    KeyNode->Signature = CM_KEY_NODE_SIGNATURE;
    KeyNode->Flags = Flags &~ REG_OPTION_CREATE_LINK;
    KeQuerySystemTime(&SystemTime);
    KeyNode->LastWriteTime = SystemTime;
    KeyNode->Spare = 0;
    KeyNode->Parent = ParentCell;
    KeyNode->SubKeyCounts[Stable] = 0;
    KeyNode->SubKeyCounts[Volatile] = 0;
    KeyNode->SubKeyLists[Stable] = HCELL_NIL;
    KeyNode->SubKeyLists[Volatile] = HCELL_NIL;
    KeyNode->ValueList.Count = 0;
    KeyNode->ValueList.List = HCELL_NIL;
    KeyNode->Security = HCELL_NIL;
    KeyNode->Class = ClassCell;
    KeyNode->ClassLength = ParseContext->Class.Length;
    KeyNode->MaxValueDataLen = 0;
    KeyNode->MaxNameLen = 0;
    KeyNode->MaxValueNameLen = 0;
    KeyNode->MaxClassLen = 0;
    KeyNode->NameLength = CmpCopyName(Hive, KeyNode->Name, Name);
    if (KeyNode->NameLength < Name->Length) KeyNode->Flags |= KEY_COMP_NAME;
    
    /* Create the KCB */
    Kcb = CmpCreateKeyControlBlock(Hive,
                                   *KeyCell,
                                   KeyNode,
                                   ParentKcb,
                                   CMP_LOCK_HASHES_FOR_KCB,
                                   Name);
    if (!Kcb)
    {
        /* Fail */
        ObDereferenceObjectDeferDelete(*Object);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Quickie;
    }
    
    /* Sanity check */
    ASSERT(Kcb->RefCount == 1);

    /* Now fill out the Cm object */
    KeyBody->NotifyBlock = NULL;
    KeyBody->ProcessID = PsGetCurrentProcessId();
    KeyBody->KeyControlBlock = Kcb;
    
    /* Link it with the KCB */
    EnlistKeyBodyWithKCB(KeyBody, 0);

Quickie:
    /* Check if we got here because of failure */
    if (!NT_SUCCESS(Status))
    {
        /* Free any cells we might've allocated */
        if (ParseContext->Class.Length > 0) HvFreeCell(Hive, ClassCell);
        HvFreeCell(Hive, *KeyCell);
    }

    /* Check if we applied ReactOS hack */
    if (Hack)
    {
        /* Restore name */
        Name->Buffer--;
        Name->Length += sizeof(OBJ_NAME_PATH_SEPARATOR);
        Name->MaximumLength += sizeof(OBJ_NAME_PATH_SEPARATOR);
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
CmpDoCreate(IN PHHIVE Hive,
            IN HCELL_INDEX Cell,
            IN PACCESS_STATE AccessState,
            IN PUNICODE_STRING Name,
            IN KPROCESSOR_MODE AccessMode,
            IN PCM_PARSE_CONTEXT ParseContext,
            IN PCM_KEY_CONTROL_BLOCK ParentKcb,
            OUT PVOID *Object)
{
    NTSTATUS Status;
    PCELL_DATA CellData;
    HCELL_INDEX KeyCell;
    ULONG ParentType;
    PCM_KEY_BODY KeyBody;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    LARGE_INTEGER TimeStamp;
    PCM_KEY_NODE KeyNode;

    /* Sanity check */
#if 0
    ASSERT((CmpIsKcbLockedExclusive(ParentKcb) == TRUE) ||
           (CmpTestRegistryLockExclusive() == TRUE));
#endif

    /* Acquire the flusher lock */
    ExAcquirePushLockShared((PVOID)&((PCMHIVE)Hive)->FlusherLock);

    /* Check if the parent is being deleted */
    if (ParentKcb->Delete)
    {
        /* It has, quit */
        ASSERT(FALSE);
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto Exit;
    }

    /* Get the parent node */
    KeyNode = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
    if (!KeyNode)
    {
        /* Fail */
        ASSERT(FALSE);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    /* Make sure nobody added us yet */
    if (CmpFindSubKeyByName(Hive, KeyNode, Name) != HCELL_NIL)
    {
        /* Fail */
        ASSERT(FALSE);
        Status = STATUS_REPARSE;
        goto Exit;
    }

    /* Sanity check */
    ASSERT(Cell == ParentKcb->KeyCell);

    /* Get the parent type */
    ParentType = HvGetCellType(Cell);
    if ((ParentType == Volatile) &&
        !(ParseContext->CreateOptions & REG_OPTION_VOLATILE))
    {
        /* Children of volatile parents must also be volatile */
        ASSERT(FALSE);
        Status = STATUS_CHILD_MUST_BE_VOLATILE;
        goto Exit;
    }

    /* Don't allow children under symlinks */
    if (ParentKcb->Flags & KEY_SYM_LINK)
    {
        /* Fail */
        ASSERT(FALSE);
        Status = STATUS_ACCESS_DENIED;
        goto Exit;
    }

    /* Make the cell dirty for now */
    HvMarkCellDirty(Hive, Cell, FALSE);

    /* Do the actual create operation */
    Status = CmpDoCreateChild(Hive,
                              Cell,
                              SecurityDescriptor,
                              AccessState,
                              Name,
                              AccessMode,
                              ParseContext,
                              ParentKcb,
                              ParseContext->CreateOptions, // WRONG!
                              &KeyCell,
                              Object);
    if (NT_SUCCESS(Status))
    {
        /* Get the key body */
        KeyBody = (PCM_KEY_BODY)(*Object);

        /* Now add the subkey */
        if (!CmpAddSubKey(Hive, Cell, KeyCell))
        {
            /* Failure! We don't handle this yet! */
            ASSERT(FALSE);
        }

        /* Get the key node */
        KeyNode = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
        if (!KeyNode)
        {
            /* Fail, this shouldn't happen */
            ASSERT(FALSE);
        }

        /* Sanity checks */
        ASSERT(KeyBody->KeyControlBlock->ParentKcb->KeyCell == Cell);
        ASSERT(KeyBody->KeyControlBlock->ParentKcb->KeyHive == Hive);
        ASSERT(KeyBody->KeyControlBlock->ParentKcb == ParentKcb);
        ASSERT(KeyBody->KeyControlBlock->ParentKcb->KcbMaxNameLen == KeyNode->MaxNameLen);

        /* Update the timestamp */
        KeQuerySystemTime(&TimeStamp);
        KeyNode->LastWriteTime = TimeStamp;
        KeyBody->KeyControlBlock->ParentKcb->KcbLastWriteTime = TimeStamp;

        /* Check if we need to update name maximum */
        if (KeyNode->MaxNameLen < Name->Length)
        {
            /* Do it */
            KeyNode->MaxNameLen = Name->Length;
            KeyBody->KeyControlBlock->ParentKcb->KcbMaxNameLen = Name->Length;
        }

        /* Check if we need toupdate class length maximum */
        if (KeyNode->MaxClassLen < ParseContext->Class.Length)
        {
            /* Update it */
            KeyNode->MaxClassLen = ParseContext->Class.Length;
        }

        /* Check if we're creating a symbolic link */
        if (ParseContext->CreateOptions & REG_OPTION_CREATE_LINK)
        {
            /* Get the cell data */
            CellData = HvGetCell(Hive, KeyCell);
            if (!CellData)
            {
                /* This shouldn't happen */
                ASSERT(FALSE);
            }

            /* Update the flags */
            CellData->u.KeyNode.Flags |= KEY_SYM_LINK;
            KeyBody->KeyControlBlock->Flags = CellData->u.KeyNode.Flags;
            HvReleaseCell(Hive, KeyCell);
        }
    }

Exit:
    /* Release the flusher lock and return status */
    ExReleasePushLock((PVOID)&((PCMHIVE)Hive)->FlusherLock);
    return Status;
}

NTSTATUS
NTAPI
CmpDoOpen(IN PHHIVE Hive,
          IN HCELL_INDEX Cell,
          IN PCM_KEY_NODE Node,
          IN PACCESS_STATE AccessState,
          IN KPROCESSOR_MODE AccessMode,
          IN ULONG Attributes,
          IN PCM_PARSE_CONTEXT Context OPTIONAL,
          IN ULONG ControlFlags,
          IN OUT PCM_KEY_CONTROL_BLOCK *CachedKcb,
          IN PUNICODE_STRING KeyName,
          OUT PVOID *Object)
{
    NTSTATUS Status;
    PCM_KEY_BODY KeyBody = NULL;
    PCM_KEY_CONTROL_BLOCK Kcb = NULL;

    /* Make sure the hive isn't locked */
    if ((Hive->HiveFlags & HIVE_IS_UNLOADING) &&
        (((PCMHIVE)Hive)->CreatorOwner != KeGetCurrentThread()))
    {
        /* It is, don't touch it */
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }
    
    /* Do this in the registry lock */
    CmpLockRegistry();

    /* If we have a KCB, make sure it's locked */
    //ASSERT(CmpIsKcbLockedExclusive(*CachedKcb));

    /* Create the KCB. FIXME: Use lock flag */
    Kcb = CmpCreateKeyControlBlock(Hive,
                                   Cell,
                                   Node,
                                   *CachedKcb,
                                   0,
                                   KeyName);
    if (!Kcb) return STATUS_INSUFFICIENT_RESOURCES;

    /* Make sure it's also locked, and set the pointer */
    //ASSERT(CmpIsKcbLockedExclusive(Kcb));
    *CachedKcb = Kcb;

    /* Release the registry lock */
    CmpUnlockRegistry();

    /* Allocate the key object */
    Status = ObCreateObject(AccessMode,
                            CmpKeyObjectType,
                            NULL,
                            AccessMode,
                            NULL,
                            sizeof(CM_KEY_BODY),
                            0,
                            0,
                            Object);
    if (NT_SUCCESS(Status))
    {
        /* Get the key body and fill it out */
        KeyBody = (PCM_KEY_BODY)(*Object);       
        KeyBody->KeyControlBlock = Kcb;
        KeyBody->Type = TAG('k', 'y', '0', '2');
        KeyBody->ProcessID = PsGetCurrentProcessId();
        KeyBody->NotifyBlock = NULL;
        
        /* Link to the KCB */
        EnlistKeyBodyWithKCB(KeyBody, 0);
    }
    else
    {
        /* Failed, dereference the KCB */
        CmpDereferenceKeyControlBlockWithLock(Kcb, FALSE);
    }

    /* Return status */
    return Status;
}

/* Remove calls to CmCreateRootNode once this is used! */
NTSTATUS
NTAPI
CmpCreateLinkNode(IN PHHIVE Hive,
                  IN HCELL_INDEX Cell,
                  IN PACCESS_STATE AccessState,
                  IN UNICODE_STRING Name,
                  IN KPROCESSOR_MODE AccessMode,
                  IN ULONG CreateOptions,
                  IN PCM_PARSE_CONTEXT Context,
                  IN PCM_KEY_CONTROL_BLOCK ParentKcb,
                  OUT PVOID *Object)
{
    NTSTATUS Status;
    HCELL_INDEX KeyCell, LinkCell, ChildCell;
    PCM_KEY_BODY KeyBody;
    LARGE_INTEGER TimeStamp;
    PCM_KEY_NODE KeyNode;
    PCM_KEY_CONTROL_BLOCK Kcb = ParentKcb;
#if 0
    CMP_ASSERT_REGISTRY_LOCK();
#endif

    /* Link nodes only allowed on the master */
    if (Hive != &CmiVolatileHive->Hive)
    {
        /* Fail */
        DPRINT1("Invalid link node attempt\n");
        return STATUS_ACCESS_DENIED;
    }
    
    /* Acquire the flusher locks */
    ExAcquirePushLockShared((PVOID)&((PCMHIVE)Hive)->FlusherLock);
    ExAcquirePushLockShared((PVOID)&((PCMHIVE)Context->ChildHive.KeyHive)->FlusherLock);

    /* Check if the parent is being deleted */
    if (ParentKcb->Delete)
    {
        /* It is, quit */
        ASSERT(FALSE);
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto Exit;
    }
    
    /* Allocate a link node */
    LinkCell = HvAllocateCell(Hive,
                              FIELD_OFFSET(CM_KEY_NODE, Name) +
                              CmpNameSize(Hive, &Name),
                              Stable,
                              HCELL_NIL);
    if (LinkCell == HCELL_NIL)
    {
        /* Fail */
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }
    
    /* Get the key cell */
    KeyCell = Context->ChildHive.KeyCell;
    if (KeyCell != HCELL_NIL)
    {
        /* Hive exists! */
        ChildCell = KeyCell;
        
        /* Get the node data */
        KeyNode = (PCM_KEY_NODE)HvGetCell(Context->ChildHive.KeyHive, ChildCell);
        if (!KeyNode)
        {
            /* Fail */
            ASSERT(FALSE);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }
        
        /* Fill out the data */
        KeyNode->Parent = LinkCell;
        KeyNode->Flags |= KEY_HIVE_ENTRY | KEY_NO_DELETE;
        HvReleaseCell(Context->ChildHive.KeyHive, ChildCell);
        
        /* Now open the key cell */
        KeyNode = (PCM_KEY_NODE)HvGetCell(Context->ChildHive.KeyHive, KeyCell);
        if (!KeyNode)
        {
            /* Fail */
            ASSERT(FALSE);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }
        
        /* Open the parent */
        Status = CmpDoOpen(Context->ChildHive.KeyHive,
                           KeyCell,
                           KeyNode,
                           AccessState,
                           AccessMode,
                           CreateOptions,
                           Context,
                           0,
                           &Kcb,
                           &Name,
                           Object);
        HvReleaseCell(Context->ChildHive.KeyHive, KeyCell);
    }
    else
    {
        /* Do the actual create operation */
        Status = CmpDoCreateChild(Context->ChildHive.KeyHive,
                                  Cell,
                                  NULL,
                                  AccessState,
                                  &Name,
                                  AccessMode,
                                  Context,
                                  ParentKcb,
                                  KEY_HIVE_ENTRY | KEY_NO_DELETE,
                                  &ChildCell,
                                  Object);
        if (NT_SUCCESS(Status))
        {
            /* Setup root pointer */
            Context->ChildHive.KeyHive->BaseBlock->RootCell = ChildCell;
        }
    }
    
    /* Check if open or create suceeded */
    if (NT_SUCCESS(Status))
    {
        /* Mark the cell dirty */
        HvMarkCellDirty(Context->ChildHive.KeyHive, ChildCell, FALSE);
        
        /* Get the key node */
        KeyNode = HvGetCell(Context->ChildHive.KeyHive, ChildCell);
        if (!KeyNode)
        {
            /* Fail */
            ASSERT(FALSE);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }
        
        /* Release it */
        HvReleaseCell(Context->ChildHive.KeyHive, ChildCell);
        
        /* Set the parent and flags */
        KeyNode->Parent = LinkCell;
        KeyNode->Flags |= KEY_HIVE_ENTRY | KEY_NO_DELETE;
        
        /* Get the link node */
        KeyNode = HvGetCell(Hive, LinkCell);
        if (!KeyNode)
        {
            /* Fail */
            ASSERT(FALSE);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }
        
        /* Set it up */
        KeyNode->Signature = CM_LINK_NODE_SIGNATURE;
        KeyNode->Flags = KEY_HIVE_EXIT | KEY_NO_DELETE;
        KeyNode->Parent = Cell;
        KeyNode->NameLength = CmpCopyName(Hive, KeyNode->Name, &Name);
        if (KeyNode->NameLength < Name.Length) KeyNode->Flags |= KEY_COMP_NAME;
        KeQuerySystemTime(&TimeStamp);
        KeyNode->LastWriteTime = TimeStamp;
        
        /* Clear out the rest */
        KeyNode->SubKeyCounts[Stable] = 0;
        KeyNode->SubKeyCounts[Volatile] = 0;
        KeyNode->SubKeyLists[Stable] = HCELL_NIL;
        KeyNode->SubKeyLists[Volatile] = HCELL_NIL;
        KeyNode->ValueList.Count = 0;
        KeyNode->ValueList.List = HCELL_NIL;
        KeyNode->ClassLength = 0;
        
        /* Reference the root node */
        KeyNode->ChildHiveReference.KeyHive = Context->ChildHive.KeyHive;
        KeyNode->ChildHiveReference.KeyCell = ChildCell;
        HvReleaseCell(Hive, LinkCell);
        
        /* Get the parent node */
        KeyNode = HvGetCell(Hive, Cell);
        if (!KeyNode)
        {
            /* Fail */
            ASSERT(FALSE);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;  
        }
        
        /* Now add the subkey */
        if (!CmpAddSubKey(Hive, Cell, LinkCell))
        {
            /* Failure! We don't handle this yet! */
            ASSERT(FALSE);
        }
        
        /* Get the key body */
        KeyBody = (PCM_KEY_BODY)*Object;

        /* Sanity checks */
        ASSERT(KeyBody->KeyControlBlock->ParentKcb->KeyCell == Cell);
        ASSERT(KeyBody->KeyControlBlock->ParentKcb->KeyHive == Hive);
        ASSERT(KeyBody->KeyControlBlock->ParentKcb->KcbMaxNameLen == KeyNode->MaxNameLen);
        
        /* Update the timestamp */
        KeQuerySystemTime(&TimeStamp);
        KeyNode->LastWriteTime = TimeStamp;
        KeyBody->KeyControlBlock->ParentKcb->KcbLastWriteTime = TimeStamp;
        
        /* Check if we need to update name maximum */
        if (KeyNode->MaxNameLen < Name.Length)
        {
            /* Do it */
            KeyNode->MaxNameLen = Name.Length;
            KeyBody->KeyControlBlock->ParentKcb->KcbMaxNameLen = Name.Length;
        }
        
        /* Check if we need toupdate class length maximum */
        if (KeyNode->MaxClassLen < Context->Class.Length)
        {
            /* Update it */
            KeyNode->MaxClassLen = Context->Class.Length;
        }
        
        /* Release the cell */
        HvReleaseCell(Hive, Cell);
    }
    else
    {
        /* Release the link cell */
        HvReleaseCell(Hive, LinkCell);
    }
    
Exit:
    /* Release the flusher locks and return status */
    ExReleasePushLock((PVOID)&((PCMHIVE)Context->ChildHive.KeyHive)->FlusherLock);
    ExReleasePushLock((PVOID)&((PCMHIVE)Hive)->FlusherLock);
    return Status;
}

VOID
NTAPI
CmpHandleExitNode(IN OUT PHHIVE *Hive,
                  IN OUT HCELL_INDEX *Cell,
                  IN OUT PCM_KEY_NODE *KeyNode,
                  IN OUT PHHIVE *ReleaseHive,
                  IN OUT HCELL_INDEX *ReleaseCell)
{
    /* Check if we have anything to release */
    if (*ReleaseCell != HCELL_NIL)
    {
        /* Release it */
        ASSERT(*ReleaseHive != NULL);
        HvReleaseCell((*ReleaseHive), *ReleaseCell);
    }
    
    /* Get the link references */
    *Hive = (*KeyNode)->ChildHiveReference.KeyHive;
    *Cell = (*KeyNode)->ChildHiveReference.KeyCell;
    
    /* Get the new node */
    *KeyNode = (PCM_KEY_NODE)HvGetCell((*Hive), *Cell);
    if (*KeyNode)
    {
        /* Set the new release values */
        *ReleaseCell = *Cell;
        *ReleaseHive = *Hive;
    }
    else
    {
        /* Nothing to release */
        *ReleaseCell = HCELL_NIL;
        *ReleaseHive = NULL;
    }
}

NTSTATUS
NTAPI
CmpBuildHashStackAndLookupCache(IN PCM_KEY_BODY ParseObject,
                                IN OUT PCM_KEY_CONTROL_BLOCK *Kcb,
                                IN PUNICODE_STRING Current,
                                OUT PHHIVE *Hive,
                                OUT HCELL_INDEX *Cell,
                                OUT PULONG TotalRemainingSubkeys,
                                OUT PULONG MatchRemainSubkeyLevel,
                                OUT PULONG TotalSubkeys,
                                OUT PULONG OuterStackArray,
                                OUT PULONG *LockedKcbs)
{
    ULONG HashKeyCopy;
    
    /* We don't lock anything for now */
    *LockedKcbs = NULL;

    /* Make a copy of the hash key */
    HashKeyCopy = (*Kcb)->ConvKey;

    /* Calculate hash values */
    *TotalRemainingSubkeys = 0xBAADF00D;
    
    /* Lock the registry */
    CmpLockRegistry();
    
    /* Return hive and cell data */
    *Hive = (*Kcb)->KeyHive;
    *Cell = (*Kcb)->KeyCell;
    
    /* Return success for now */
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
             OUT PVOID *Object)
{
    NTSTATUS Status;
    PCM_KEY_CONTROL_BLOCK Kcb, ParentKcb;
    PHHIVE Hive = NULL;
    PCM_KEY_NODE Node = NULL;
    HCELL_INDEX Cell = HCELL_NIL, NextCell;
    PHHIVE HiveToRelease = NULL;
    HCELL_INDEX CellToRelease = HCELL_NIL;
    UNICODE_STRING Current, NextName;
    PCM_PARSE_CONTEXT ParseContext = Context;
    ULONG TotalRemainingSubkeys = 0, MatchRemainSubkeyLevel = 0, TotalSubkeys = 0;
    PULONG LockedKcbs = NULL;
    BOOLEAN Result, Last;
    PAGED_CODE();

    /* Loop path separators at the end */
    while ((RemainingName->Length) &&
           (RemainingName->Buffer[(RemainingName->Length / sizeof(WCHAR)) - 1] ==
            OBJ_NAME_PATH_SEPARATOR))
    {
        /* Remove path separator */
        RemainingName->Length -= sizeof(WCHAR);
    }

    /* Fail if this isn't a key object */
    if (ObjectType != CmpKeyObjectType) return STATUS_OBJECT_TYPE_MISMATCH;
    
    /* Copy the remaining name */
    Current = *RemainingName;
    
    /* Check if this is a create */
    if (!(ParseContext) || !(ParseContext->CreateOperation))
    {
        /* It isn't, so no context */
        ParseContext = NULL;
    }
    
    /* Grab the KCB */
    Kcb = ((PCM_KEY_BODY)ParseObject)->KeyControlBlock;

    /* Lookup in the cache */
    Status = CmpBuildHashStackAndLookupCache(ParseObject,
                                             &Kcb,
                                             &Current,
                                             &Hive,
                                             &Cell,
                                             &TotalRemainingSubkeys,
                                             &MatchRemainSubkeyLevel,
                                             &TotalSubkeys,
                                             NULL,
                                             &LockedKcbs);
    
    /* This is now the parent */
    ParentKcb = Kcb;
    
    /* Check if everything was found cached */
    if (!TotalRemainingSubkeys) ASSERTMSG("Caching not implemented", FALSE);
    
    /* Don't do anything if we're being deleted */
    if (Kcb->Delete) return STATUS_OBJECT_NAME_NOT_FOUND;
    
    /* Check if this is a symlink */
    if (Kcb->Flags & KEY_SYM_LINK)
    {
        /* Get the next name */
        Result = CmpGetNextName(&Current, &NextName, &Last);
        Current.Buffer = NextName.Buffer;

        /* Validate the current name string length */
        if (Current.Length + NextName.Length > MAXUSHORT)
        {
            /* too long */
            Status = STATUS_NAME_TOO_LONG;
            goto Quickie;
        }
        Current.Length += NextName.Length;

        /* Validate the current name string maximum length */
        if (Current.MaximumLength + NextName.MaximumLength > MAXUSHORT)
        {
            /* too long */
            Status = STATUS_NAME_TOO_LONG;
            goto Quickie;
        }
        Current.MaximumLength += NextName.MaximumLength;
        
        /* Parse the symlink */
        if (CmpGetSymbolicLink(Hive,
                               CompleteName,
                               Kcb,
                               &Current))
        {
            /* Symlink parse succeeded */
            Status = STATUS_REPARSE;
        }
        else
        {
            /* Couldn't find symlink */
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
        }

        /* We're done */
        goto Quickie;
    }
    
    /* Get the key node */
    Node = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
    if (!Node) return STATUS_INSUFFICIENT_RESOURCES;
    
    /* Start parsing */
    Status = STATUS_NOT_IMPLEMENTED;
    while (TRUE)
    {
        /* Get the next component */
        Result = CmpGetNextName(&Current, &NextName, &Last); 
        if ((Result) && (NextName.Length))
        {
            /* See if this is a sym link */
            if (!(Kcb->Flags & KEY_SYM_LINK))
            {
                /* Find the subkey */
                NextCell = CmpFindSubKeyByName(Hive, Node, &NextName);
                if (NextCell != HCELL_NIL)
                {
                    /* Get the new node */
                    Cell = NextCell;
                    Node = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
                    if (!Node) ASSERT(FALSE);
                    
                    /* Check if this was the last key */
                    if (Last)
                    {
                        /* Is this an exit node */
                        if (Node->Flags & KEY_HIVE_EXIT)
                        {
                            /* Handle it */
                            CmpHandleExitNode(&Hive,
                                              &Cell,
                                              &Node,
                                              &HiveToRelease,
                                              &CellToRelease);
                            if (!Node) ASSERT(FALSE);
                        }
                        
                        /* Do the open */
                        Status = CmpDoOpen(Hive,
                                           Cell,
                                           Node,
                                           AccessState,
                                           AccessMode,
                                           Attributes,
                                           ParseContext,
                                           0,
                                           &Kcb,
                                           &NextName,
                                           Object);
                        if (Status == STATUS_REPARSE)
                        {
                            /* Parse the symlink */
                            if (!CmpGetSymbolicLink(Hive,
                                                    CompleteName,
                                                    Kcb,
                                                    NULL))
                            {
                                /* Symlink parse failed */
                                Status = STATUS_OBJECT_NAME_NOT_FOUND;
                            }
                        }
                        
                        /* We are done */
                        break;
                    }
                    
                    /* Is this an exit node */
                    if (Node->Flags & KEY_HIVE_EXIT)
                    {
                        /* Handle it */
                        CmpHandleExitNode(&Hive,
                                          &Cell,
                                          &Node,
                                          &HiveToRelease,
                                          &CellToRelease);
                        if (!Node) ASSERT(FALSE);
                    }

                    /* Create a KCB for this key */
                    Kcb = CmpCreateKeyControlBlock(Hive,
                                                   Cell,
                                                   Node,
                                                   ParentKcb,
                                                   0,
                                                   &NextName);
                    if (!Kcb) ASSERT(FALSE);
                    
                    /* Dereference the parent and set the new one */
                    //CmpDereferenceKeyControlBlock(ParentKcb);
                    ParentKcb = Kcb;
                }
                else
                {
                    /* Check if this was the last key for a create */
                    if ((Last) && (ParseContext))
                    {
                        PCM_KEY_BODY KeyBody;

                        /* Check if we're doing a link node */
                        if (ParseContext->CreateLink)
                        {
                            /* The only thing we should see */
                            Status = CmpCreateLinkNode(Hive,
                                                       Cell,
                                                       AccessState,
                                                       NextName,
                                                       AccessMode,
                                                       Attributes,
                                                       ParseContext,
                                                       ParentKcb,
                                                       Object);
                        }
                        else
                        {
                            /* Create: should not see this (yet) */
                            DPRINT1("Unexpected: Creating new child\n");
                            while (TRUE);
                        }
                        
                        /* Check for reparse (in this case, someone beat us) */
                        if (Status == STATUS_REPARSE) break;
                        
                        /* ReactOS Hack: Link this key to the parent */
                        KeyBody = (PCM_KEY_BODY)*Object;
                        InsertTailList(&ParentKcb->KeyBodyListHead,
                                       &KeyBody->KeyBodyList);

                        /* Update disposition */
                        ParseContext->Disposition = REG_CREATED_NEW_KEY;
                        break;
                    }
                    else
                    {
                        /* Key not found */
                        Status = STATUS_OBJECT_NAME_NOT_FOUND;
                        break;
                    }
                }
            }
            else
            {
                /* Save the next name */
                Current.Buffer = NextName.Buffer;
                
                /* Validate the current name string length */
                if (Current.Length + NextName.Length > MAXUSHORT)
                {
                    /* too long */
                    Status = STATUS_NAME_TOO_LONG;
                    break;
                }
                Current.Length += NextName.Length;
                
                /* Validate the current name string maximum length */
                if (Current.MaximumLength + NextName.MaximumLength > MAXUSHORT)
                {
                    /* too long */
                    Status = STATUS_NAME_TOO_LONG;
                    break;
                }
                Current.MaximumLength += NextName.MaximumLength;
                
                /* Parse the symlink */
                if (CmpGetSymbolicLink(Hive,
                                       CompleteName,
                                       Kcb,
                                       &Current))
                {
                    /* Symlink parse succeeded */
                    Status = STATUS_REPARSE;
                }
                else
                {
                    /* Couldn't find symlink */
                    Status = STATUS_OBJECT_NAME_NOT_FOUND;
                }

                /* We're done */
                break;
            }
        }
        else if ((Result) && (Last))
        {
            /* Opening the root. Is this an exit node? */
            if (Node->Flags & KEY_HIVE_EXIT)
            {
                /* Handle it */
                CmpHandleExitNode(&Hive,
                                  &Cell,
                                  &Node,
                                  &HiveToRelease,
                                  &CellToRelease);
                if (!Node) ASSERT(FALSE);
            }
            
            /* FIXME: This hack seems required? */
            RtlInitUnicodeString(&NextName, L"\\REGISTRY");
            
            /* Do the open */
            Status = CmpDoOpen(Hive,
                               Cell,
                               Node,
                               AccessState,
                               AccessMode,
                               Attributes,
                               ParseContext,
                               0,
                               &Kcb,
                               &NextName,
                               Object);
            if (Status == STATUS_REPARSE)
            {
                /* Nothing to do */
            }
            
            /* We're done */
            break;
        }
        else
        {
            /* Bogus */
            Status = STATUS_INVALID_PARAMETER;
            break;
        }
    }

    /* Dereference the parent if it exists */
Quickie:
    //if (ParentKcb) CmpDereferenceKeyControlBlock(ParentKcb);
    
    /* Unlock the registry */
    CmpUnlockRegistry();
    return Status;
}
