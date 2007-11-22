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

NTSTATUS
NTAPI
CmpDoCreateChild(IN PHHIVE Hive,
                 IN HCELL_INDEX ParentCell,
                 IN PSECURITY_DESCRIPTOR ParentDescriptor OPTIONAL,
                 IN PACCESS_STATE AccessState,
                 IN PUNICODE_STRING Name,
                 IN KPROCESSOR_MODE AccessMode,
                 IN PUNICODE_STRING Class,
                 IN PCM_KEY_CONTROL_BLOCK ParentKcb,
                 IN ULONG CreateOptions,
                 OUT PHCELL_INDEX KeyCell,
                 OUT PVOID *Object)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKEY_OBJECT KeyBody;
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
    if (CreateOptions & REG_OPTION_VOLATILE) StorageType = Volatile;

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
    if (Class->Length > 0)
    {
        /* Allocate a class cell */
        ClassCell = HvAllocateCell(Hive, Class->Length, StorageType, HCELL_NIL);
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
                            sizeof(KEY_OBJECT),
                            0,
                            0,
                            Object);
    if (!NT_SUCCESS(Status)) goto Quickie;
    KeyBody = (PKEY_OBJECT)(*Object);

    /* Check if we had a class */
    if (Class->Length > 0)
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
                      Class->Buffer,
                      Class->Length);
    }

    /* Fill out the key node */
    KeyNode->Signature = CM_KEY_NODE_SIGNATURE;
    KeyNode->Flags = CreateOptions;
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
    KeyNode->ClassLength = Class->Length;
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
                                   0,
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
    KeyBody->KeyControlBlock = Kcb;
    KeyBody->SubKeyCounts = 0;
    KeyBody->SubKeys = NULL;
    KeyBody->SizeOfSubKeys = 0;
    InsertTailList(&CmiKeyObjectListHead, &KeyBody->KeyBodyList);

Quickie:
    /* Check if we got here because of failure */
    if (!NT_SUCCESS(Status))
    {
        /* Free any cells we might've allocated */
        if (Class->Length > 0) HvFreeCell(Hive, ClassCell);
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
            IN PUNICODE_STRING Class OPTIONAL,
            IN ULONG CreateOptions,
            IN PCM_KEY_CONTROL_BLOCK ParentKcb,
            IN PCMHIVE OriginatingHive OPTIONAL,
            OUT PVOID *Object)
{
    NTSTATUS Status;
    PCELL_DATA CellData;
    HCELL_INDEX KeyCell;
    ULONG ParentType;
    PKEY_OBJECT KeyBody;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    LARGE_INTEGER TimeStamp;
    PCM_KEY_NODE KeyNode;
    UNICODE_STRING LocalClass = {0};
    if (!Class) Class = &LocalClass;

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
    if ((ParentType == Volatile) && !(CreateOptions & REG_OPTION_VOLATILE))
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
                              Class,
                              ParentKcb,
                              CreateOptions,
                              &KeyCell,
                              Object);
    if (NT_SUCCESS(Status))
    {
        /* Get the key body */
        KeyBody = (PKEY_OBJECT)(*Object);

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
        //ASSERT(KeyBody->KeyControlBlock->ParentKcb->KcbMaxNameLen == KeyNode->MaxNameLen);

        /* Update the timestamp */
        KeQuerySystemTime(&TimeStamp);
        KeyNode->LastWriteTime = TimeStamp;

        /* Check if we need to update name maximum */
        if (KeyNode->MaxNameLen < Name->Length)
        {
            /* Do it */
            KeyNode->MaxNameLen = Name->Length;
            KeyBody->KeyControlBlock->ParentKcb->KcbMaxNameLen = Name->Length;
        }

        /* Check if we need toupdate class length maximum */
        if (KeyNode->MaxClassLen < Class->Length)
        {
            /* Update it */
            KeyNode->MaxClassLen = Class->Length;
        }

        /* Check if we're creating a symbolic link */
        if (CreateOptions & REG_OPTION_CREATE_LINK)
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
    PKEY_OBJECT KeyBody = NULL;
    PCM_KEY_CONTROL_BLOCK Kcb = NULL;

    /* Make sure the hive isn't locked */
    if ((Hive->HiveFlags & HIVE_IS_UNLOADING) &&
        (((PCMHIVE)Hive)->CreatorOwner != KeGetCurrentThread()))
    {
        /* It is, don't touch it */
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    /* If we have a KCB, make sure it's locked */
    ASSERT(CmpIsKcbLockedExclusive(*CachedKcb));

    /* Create the KCB */
    Kcb = CmpCreateKeyControlBlock(Hive, Cell, Node, *CachedKcb, 0, KeyName);
    if (!Kcb) return STATUS_INSUFFICIENT_RESOURCES;

    /* Make sure it's also locked, and set the pointer */
    ASSERT(CmpIsKcbLockedExclusive(Kcb));
    *CachedKcb = Kcb;

    /* Allocate the key object */
    Status = ObCreateObject(AccessMode,
                            CmpKeyObjectType,
                            NULL,
                            AccessMode,
                            NULL,
                            sizeof(KEY_OBJECT),
                            0,
                            0,
                            Object);
    if (NT_SUCCESS(Status))
    {
        /* Get the key body and fill it out */
        KeyBody = (PKEY_OBJECT)(*Object);       
        KeyBody->KeyControlBlock = Kcb;
        KeyBody->SubKeyCounts = 0;
        KeyBody->SubKeys = NULL;
        KeyBody->SizeOfSubKeys = 0;
        InsertTailList(&CmiKeyObjectListHead, &KeyBody->KeyBodyList);
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
    PKEY_OBJECT KeyBody;
    LARGE_INTEGER TimeStamp;
    PCM_KEY_NODE KeyNode;
    PCM_KEY_CONTROL_BLOCK Kcb = ParentKcb;
    
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
                           NULL,
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
                                  &Context->Class,
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
        
        /* Set the parent adn flags */
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
        //KeyNode->ChildHiveReference.KeyHive = Context->ChildHive.KeyHive;
        //KeyNode->ChildHiveReference.KeyCell = ChildCell;
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
        KeyBody = (PKEY_OBJECT)*Object;

        /* Sanity checks */
        ASSERT(KeyBody->KeyControlBlock->ParentKcb->KeyCell == Cell);
        ASSERT(KeyBody->KeyControlBlock->ParentKcb->KeyHive == Hive);
        //ASSERT(KeyBody->KeyControlBlock->ParentKcb->KcbMaxNameLen == KeyNode->MaxNameLen);
        
        /* Update the timestamp */
        KeQuerySystemTime(&TimeStamp);
        KeyNode->LastWriteTime = TimeStamp;
        
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
    }
    
Exit:
    /* Release the flusher locks and return status */
    ExReleasePushLock((PVOID)&((PCMHIVE)Context->ChildHive.KeyHive)->FlusherLock);
    ExReleasePushLock((PVOID)&((PCMHIVE)Hive)->FlusherLock);
    return Status;
}
