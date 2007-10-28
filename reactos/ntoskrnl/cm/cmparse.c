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
                 IN PCM_PARSE_CONTEXT Context,
                 IN PCM_KEY_CONTROL_BLOCK ParentKcb,
                 IN USHORT Flags,
                 OUT PHCELL_INDEX KeyCell,
                 OUT PVOID *Object)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCM_KEY_CONTROL_BLOCK kcb;
    HCELL_INDEX ClassCell = HCELL_NIL;
    PCM_KEY_NODE KeyNode;
    PCELL_DATA CellData;
    ULONG StorageType;
    LARGE_INTEGER SystemTime;
    BOOLEAN Hack = FALSE;
    PCM_KEY_BODY KeyBody;

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
    if (Context->CreateOptions & REG_OPTION_VOLATILE) StorageType = Volatile;

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
    if (Context->Class.Length > 0)
    {
        /* Allocate a class cell */
        ClassCell = HvAllocateCell(Hive,
                                   Context->Class.Length,
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
    KeyBody = (PCM_KEY_BODY)(*Object);

    /* Check if we had a class */
    if (Context->Class.Length > 0)
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
                      Context->Class.Buffer,
                      Context->Class.Length);
    }

    /* Fill out the key node */
    KeyNode->Signature = CM_KEY_NODE_SIGNATURE;
    KeyNode->Flags = Flags;
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
    KeyNode->ClassLength = Context->Class.Length;
    KeyNode->MaxValueDataLen = 0;
    KeyNode->MaxNameLen = 0;
    KeyNode->MaxValueNameLen = 0;
    KeyNode->MaxClassLen = 0;
    KeyNode->NameLength = CmpCopyName(Hive, KeyNode->Name, Name);
    if (KeyNode->NameLength < Name->Length) KeyNode->Flags |= KEY_COMP_NAME;

    /* Now fill out the Cm object */
    kcb = CmpCreateKeyControlBlock(Hive,
                                   *KeyCell,
                                   KeyNode,
                                   ParentKcb,
                                   CMP_LOCK_HASHES_FOR_KCB, Name);
    ASSERT(kcb->RefCount == 1);
    KeyBody->Type = KEY_BODY_TYPE;
    KeyBody->KeyControlBlock = kcb;
    KeyBody->NotifyBlock = NULL;
    KeyBody->ProcessID = PsGetCurrentProcessId();
    EnlistKeyBodyWithKCB(KeyBody, 2);

Quickie:
    /* Check if we got here because of failure */
    if (!NT_SUCCESS(Status))
    {
        /* Free any cells we might've allocated */
        if (Context->Class.Length > 0) HvFreeCell(Hive, ClassCell);
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
            IN PCM_PARSE_CONTEXT Context,
            IN PCM_KEY_CONTROL_BLOCK ParentKcb,
            IN PCMHIVE OriginatingHive OPTIONAL,
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

    /* Acquire the flusher lock */
    //ExAcquirePushLockShared((PVOID)&((PCMHIVE)Hive)->FlusherLock);

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
    if ((ParentType == Volatile) && !(Context->CreateOptions & REG_OPTION_VOLATILE))
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
                              Context,
                              ParentKcb,
                              0,
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
        if (KeyNode->MaxClassLen < Context->Class.Length)
        {
            /* Update it */
            KeyNode->MaxClassLen = Context->Class.Length;
        }

        /* Check if we're creating a symbolic link */
        if (Context->CreateOptions & REG_OPTION_CREATE_LINK)
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
    //ExReleasePushLock((PVOID)&((PCMHIVE)Hive)->FlusherLock);
    return Status;
}
