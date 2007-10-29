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
                 IN PKEY_OBJECT Parent,
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
                                   Parent->KeyControlBlock,
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
    KeyBody->KeyCell = KeyNode;
    KeyBody->KeyCellOffset = *KeyCell;
    KeyBody->Flags = 0;
    KeyBody->SubKeyCounts = 0;
    KeyBody->SubKeys = NULL;
    KeyBody->SizeOfSubKeys = 0;
    KeyBody->ParentKey = Parent;
    KeyBody->RegistryHive = KeyBody->ParentKey->RegistryHive;
    InsertTailList(&CmiKeyObjectListHead, &KeyBody->ListEntry);

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
            IN PKEY_OBJECT Parent,
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
    #define KO_MARKED_FOR_DELETE 0x00000001
    if (Parent->Flags & KO_MARKED_FOR_DELETE)
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
    ASSERT(Cell == Parent->KeyCellOffset);

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
    if (Parent->Flags & KEY_SYM_LINK)
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
                              Parent,
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
        ASSERT(KeyBody->ParentKey->KeyCellOffset == Cell);
        ASSERT(&KeyBody->ParentKey->RegistryHive->Hive == Hive);
        ASSERT(KeyBody->ParentKey == Parent);

        /* Update the timestamp */
        KeQuerySystemTime(&TimeStamp);
        KeyNode->LastWriteTime = TimeStamp;

        /* Check if we need to update name maximum */
        if (KeyNode->MaxNameLen < Name->Length)
        {
            /* Do it */
            KeyNode->MaxNameLen = Name->Length;
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
