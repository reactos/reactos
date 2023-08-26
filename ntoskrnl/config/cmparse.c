/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmparse.c
 * PURPOSE:         Configuration Manager - Object Manager Parse Interface
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
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

    ASSERT(RemainingName->Length % sizeof(WCHAR) == 0);

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
    while ((RemainingName->Length) &&
           (*RemainingName->Buffer == OBJ_NAME_PATH_SEPARATOR))
    {
        /* Skip it */
        RemainingName->Buffer++;
        RemainingName->Length -= sizeof(WCHAR);
        RemainingName->MaximumLength -= sizeof(WCHAR);
    }

    /* Start loop at where the current buffer is */
    NextName->Buffer = RemainingName->Buffer;
    while ((RemainingName->Length) &&
           (*RemainingName->Buffer != OBJ_NAME_PATH_SEPARATOR))
    {
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
                         (PVOID*)&LinkName,
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
    PCM_KEY_CONTROL_BLOCK Kcb;
    PSECURITY_DESCRIPTOR NewDescriptor;

    /* Get the storage type */
    StorageType = Stable;
    if (ParseContext->CreateOptions & REG_OPTION_VOLATILE) StorageType = Volatile;

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
    KeyBody->Type = CM_KEY_BODY_TYPE;
    KeyBody->KeyControlBlock = NULL;
    KeyBody->KcbLocked = FALSE;

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
    KeyNode->Flags = Flags;
    KeQuerySystemTime(&KeyNode->LastWriteTime);
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
    EnlistKeyBodyWithKCB(KeyBody, CMP_ENLIST_KCB_LOCKED_EXCLUSIVE);

    /* Assign security */
    Status = SeAssignSecurity(ParentDescriptor,
                              AccessState->SecurityDescriptor,
                              &NewDescriptor,
                              TRUE,
                              &AccessState->SubjectSecurityContext,
                              &CmpKeyObjectType->TypeInfo.GenericMapping,
                              CmpKeyObjectType->TypeInfo.PoolType);
    if (NT_SUCCESS(Status))
    {
        /*
         * FIXME: We must acquire a security lock when assigning
         * a security descriptor to this hive but since the
         * CmpAssignSecurityDescriptor function does nothing
         * (we lack the necessary security management implementations
         * anyway), do not do anything for now.
         */
        Status = CmpAssignSecurityDescriptor(Kcb, NewDescriptor);
    }

    /* Now that the security descriptor is copied in the hive, we can free the original */
    SeDeassignSecurity(&NewDescriptor);

    if (NT_SUCCESS(Status))
    {
        /* Send notification to registered callbacks */
        CmpReportNotify(Kcb, Hive, Kcb->KeyCell, REG_NOTIFY_CHANGE_NAME);
    }

Quickie:
    /* Check if we got here because of failure */
    if (!NT_SUCCESS(Status))
    {
        /* Free any cells we might've allocated */
        if (ParseContext->Class.Length > 0) HvFreeCell(Hive, ClassCell);
        HvFreeCell(Hive, *KeyCell);
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

    /* Make sure the KCB is locked and lock the flusher */
    CMP_ASSERT_KCB_LOCK(ParentKcb);
    CmpLockHiveFlusherShared((PCMHIVE)Hive);

    /* Bail out on read-only KCBs */
    if (ParentKcb->ExtFlags & CM_KCB_READ_ONLY_KEY)
    {
        Status = STATUS_ACCESS_DENIED;
        goto Exit;
    }

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
        //ASSERT(FALSE);
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
            /* Free the created child */
            CmpFreeKeyByCell(Hive, KeyCell, FALSE);

            /* Purge out this KCB */
            KeyBody->KeyControlBlock->Delete = TRUE;
            CmpRemoveKeyControlBlock(KeyBody->KeyControlBlock);

            /* And cleanup the key body object */
            ObDereferenceObjectDeferDelete(*Object);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        /* Get the key node */
        KeyNode = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
        if (!KeyNode)
        {
            /* Fail, this shouldn't happen */
            CmpFreeKeyByCell(Hive, KeyCell, TRUE); // Subkey linked above

            /* Purge out this KCB */
            KeyBody->KeyControlBlock->Delete = TRUE;
            CmpRemoveKeyControlBlock(KeyBody->KeyControlBlock);

            /* And cleanup the key body object */
            ObDereferenceObjectDeferDelete(*Object);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        /* Clean up information on this subkey */
        CmpCleanUpSubKeyInfo(KeyBody->KeyControlBlock->ParentKcb);

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

        /* Check if we need to update class length maximum */
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
                CmpFreeKeyByCell(Hive, KeyCell, TRUE); // Subkey linked above

                /* Purge out this KCB */
                KeyBody->KeyControlBlock->Delete = TRUE;
                CmpRemoveKeyControlBlock(KeyBody->KeyControlBlock);

                /* And cleanup the key body object */
                ObDereferenceObjectDeferDelete(*Object);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Exit;
            }

            /* Update the flags */
            CellData->u.KeyNode.Flags |= KEY_SYM_LINK;
            KeyBody->KeyControlBlock->Flags = CellData->u.KeyNode.Flags;
            HvReleaseCell(Hive, KeyCell);
        }
    }

Exit:
    /* Release the flusher lock and return status */
    CmpUnlockHiveFlusher((PCMHIVE)Hive);
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
          IN PULONG KcbsLocked,
          IN PUNICODE_STRING KeyName,
          OUT PVOID *Object)
{
    NTSTATUS Status;
    BOOLEAN LockKcb = FALSE;
    BOOLEAN IsLockShared = FALSE;
    PCM_KEY_BODY KeyBody = NULL;
    PCM_KEY_CONTROL_BLOCK Kcb = NULL;

    /* Make sure the hive isn't locked */
    if ((Hive->HiveFlags & HIVE_IS_UNLOADING) &&
        (((PCMHIVE)Hive)->CreatorOwner != KeGetCurrentThread()))
    {
        /* It is, don't touch it */
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    /* Check if we have a context */
    if (Context)
    {
        /* Check if this is a link create (which shouldn't be an open) */
        if (Context->CreateLink)
        {
            return STATUS_ACCESS_DENIED;
        }

        /* Check if this is symlink create attempt */
        if (Context->CreateOptions & REG_OPTION_CREATE_LINK)
        {
            /* Key already exists */
            return STATUS_OBJECT_NAME_COLLISION;
        }

        /* Set the disposition */
        Context->Disposition = REG_OPENED_EXISTING_KEY;
    }

    /* Lock the KCB on creation if asked */
    if (ControlFlags & CMP_CREATE_KCB_KCB_LOCKED)
    {
        LockKcb = TRUE;
    }

    /* Check if caller doesn't want to create a KCB */
    if (ControlFlags & CMP_OPEN_KCB_NO_CREATE)
    {
        /*
         * The caller doesn't want to create a KCB. This means the KCB
         * is already in cache and other threads may take use of it
         * so it has to be locked in share mode.
         */
        IsLockShared = TRUE;

        /* Check if this is a symlink */
        if (((*CachedKcb)->Flags & KEY_SYM_LINK) && !(Attributes & OBJ_OPENLINK))
        {
            /* Is this symlink found? */
            if ((*CachedKcb)->ExtFlags & CM_KCB_SYM_LINK_FOUND)
            {
                /* Get the real KCB, is this deleted? */
                Kcb = (*CachedKcb)->ValueCache.RealKcb;
                if (Kcb->Delete)
                {
                    /*
                     * The real KCB is gone, do a reparse. We used to lock the KCB in
                     * shared mode as others may have taken use of it but since we
                     * must do a reparse of the key the only thing that matter is us.
                     * Lock the KCB exclusively so nobody is going to mess with the KCB.
                     */
                    DPRINT1("The real KCB is deleted, attempt a reparse\n");
                    CmpUnLockKcbArray(KcbsLocked);
                    CmpAcquireKcbLockExclusiveByIndex(GET_HASH_INDEX((*CachedKcb)->ConvKey));
                    CmpCleanUpKcbValueCache(*CachedKcb);
                    KcbsLocked[0] = 1;
                    KcbsLocked[1] = GET_HASH_INDEX((*CachedKcb)->ConvKey);
                    return STATUS_REPARSE;
                }

                /*
                 * The symlink has been found. As in the similar case above,
                 * the KCB of the symlink exclusively, we don't want anybody
                 * to mess it up.
                 */
                CmpUnLockKcbArray(KcbsLocked);
                CmpAcquireKcbLockExclusiveByIndex(GET_HASH_INDEX((*CachedKcb)->ConvKey));
                KcbsLocked[0] = 1;
                KcbsLocked[1] = GET_HASH_INDEX((*CachedKcb)->ConvKey);
            }
            else
            {
                /* We must do a reparse */
                DPRINT("The symlink is not found, attempt a reparse\n");
                return STATUS_REPARSE;
            }
        }
        else
        {
            /* This is not a symlink, just give the cached KCB already */
            Kcb = *CachedKcb;
        }

        /* The caller wants to open a cached KCB */
        if (!CmpReferenceKeyControlBlock(Kcb))
        {
            /* Return failure code */
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        /*
         * The caller wants to create a new KCB. Unlike the code path above, here
         * we must check if the lock is exclusively held because in the scenario
         * where the caller doesn't want to create a KCB is because it is already
         * in the cache and it must have a shared lock instead.
         */
        ASSERT(CmpIsKcbLockedExclusive(*CachedKcb));

        /* Check if this is a symlink */
        if ((Node->Flags & KEY_SYM_LINK) && !(Attributes & OBJ_OPENLINK))
        {
            /* Create the KCB for the symlink */
            Kcb = CmpCreateKeyControlBlock(Hive,
                                           Cell,
                                           Node,
                                           *CachedKcb,
                                           LockKcb ? CMP_LOCK_HASHES_FOR_KCB : 0,
                                           KeyName);
            if (!Kcb)
            {
                /* Return failure */
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            /* Make sure it's also locked, and set the pointer */
            ASSERT(CmpIsKcbLockedExclusive(Kcb));
            *CachedKcb = Kcb;

            /* Return reparse required */
            return STATUS_REPARSE;
        }

        /* Create the KCB */
        Kcb = CmpCreateKeyControlBlock(Hive,
                                       Cell,
                                       Node,
                                       *CachedKcb,
                                       LockKcb ? CMP_LOCK_HASHES_FOR_KCB : 0,
                                       KeyName);
        if (!Kcb)
        {
            /* Return failure */
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Make sure it's also locked, and set the pointer */
        ASSERT(CmpIsKcbLockedExclusive(Kcb));
        *CachedKcb = Kcb;
    }

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
        KeyBody->Type = CM_KEY_BODY_TYPE;
        KeyBody->ProcessID = PsGetCurrentProcessId();
        KeyBody->NotifyBlock = NULL;

        /* Link to the KCB */
        EnlistKeyBodyWithKCB(KeyBody, IsLockShared ? CMP_ENLIST_KCB_LOCKED_SHARED : CMP_ENLIST_KCB_LOCKED_EXCLUSIVE);

        /*
         * We are already holding a lock against the KCB that is assigned
         * to this key body. This is to prevent a potential deadlock on
         * CmpSecurityMethod as ObCheckObjectAccess will invoke the Object
         * Manager to call that method, of which CmpSecurityMethod would
         * attempt to acquire a lock again.
         */
        KeyBody->KcbLocked = TRUE;

        if (!ObCheckObjectAccess(*Object,
                                 AccessState,
                                 FALSE,
                                 AccessMode,
                                 &Status))
        {
            /* Access check failed */
            ObDereferenceObject(*Object);
        }

        /*
         * We are done, the lock we are holding will be released
         * once the registry parsing is done.
         */
        KeyBody->KcbLocked = FALSE;
    }
    else
    {
        /* Failed, dereference the KCB */
        CmpDereferenceKeyControlBlockWithLock(Kcb, FALSE);
    }

    /* Return status */
    return Status;
}

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
                  IN PULONG KcbsLocked,
                  OUT PVOID *Object)
{
    NTSTATUS Status;
    HCELL_INDEX KeyCell, LinkCell, ChildCell;
    PCM_KEY_BODY KeyBody;
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

    /* Make sure the KCB is locked and lock the flusher */
    CMP_ASSERT_KCB_LOCK(ParentKcb);
    CmpLockHiveFlusherShared((PCMHIVE)Hive);
    CmpLockHiveFlusherShared((PCMHIVE)Context->ChildHive.KeyHive);

    /* Bail out on read-only KCBs */
    if (ParentKcb->ExtFlags & CM_KCB_READ_ONLY_KEY)
    {
        Status = STATUS_ACCESS_DENIED;
        goto Exit;
    }

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
                           CMP_CREATE_KCB_KCB_LOCKED,
                           &Kcb,
                           KcbsLocked,
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
        KeyNode = (PCM_KEY_NODE)HvGetCell(Context->ChildHive.KeyHive, ChildCell);
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
        KeyNode = (PCM_KEY_NODE)HvGetCell(Hive, LinkCell);
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
        KeyNode = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
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

        /* Clean up information on this subkey */
        CmpCleanUpSubKeyInfo(KeyBody->KeyControlBlock->ParentKcb);

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

        /* Check if we need to update class length maximum */
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
    CmpUnlockHiveFlusher((PCMHIVE)Context->ChildHive.KeyHive);
    CmpUnlockHiveFlusher((PCMHIVE)Hive);
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
        HvReleaseCell(*ReleaseHive, *ReleaseCell);
    }

    /* Get the link references */
    *Hive = (*KeyNode)->ChildHiveReference.KeyHive;
    *Cell = (*KeyNode)->ChildHiveReference.KeyCell;

    /* Get the new node */
    *KeyNode = (PCM_KEY_NODE)HvGetCell(*Hive, *Cell);
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

/**
 * @brief
 * Computes the hashes of each subkey in key path name
 * and stores them in a hash stack for cache lookup.
 *
 * @param[in] RemainingName
 * A Unicode string structure consisting of the remaining
 * registry key path name.
 *
 * @param[in] ConvKey
 * The hash convkey of the current KCB to be supplied.
 *
 * @param[in,out] HashCacheStack
 * An array stack. This function uses this array to store
 * all the computed hashes of a key pathname.
 *
 * @param[out] TotalSubKeys
 * The number of total subkeys that have been found, returned
 * by this function to the caller. If no subkey levels are found
 * the function returns 0.
 *
 * @return
 * Returns the number of remaining subkey levels to caller.
 * If no subkey levels are found then this function returns 0.
 */
static
ULONG
CmpComputeHashValue(
    _In_ PUNICODE_STRING RemainingName,
    _In_ ULONG ConvKey,
    _Inout_ PCM_HASH_CACHE_STACK HashCacheStack,
    _Out_ PULONG TotalSubKeys)
{
    ULONG CopyConvKey;
    ULONG SubkeysInTotal;
    ULONG RemainingSubkeysInTotal;
    PWCHAR RemainingNameBuffer;
    USHORT RemainingNameLength;
    USHORT KeyNameLength;

    /* Don't compute the hashes on a NULL remaining name */
    RemainingNameBuffer = RemainingName->Buffer;
    RemainingNameLength = RemainingName->Length;
    if (RemainingNameLength == 0)
    {
        *TotalSubKeys = 0;
        return 0;
    }

    /* Skip any leading separator */
    while (RemainingNameLength >= sizeof(WCHAR) &&
           *RemainingNameBuffer == OBJ_NAME_PATH_SEPARATOR)
    {
        RemainingNameBuffer++;
        RemainingNameLength -= sizeof(WCHAR);
    }

    /* Now set up the hash stack entries and compute the hashes */
    SubkeysInTotal = 0;
    RemainingSubkeysInTotal = 0;
    KeyNameLength = 0;
    CopyConvKey = ConvKey;
    HashCacheStack[RemainingSubkeysInTotal].NameOfKey.Buffer = RemainingNameBuffer;
    while (RemainingNameLength > 0)
    {
        /* Is this character a separator? */
        if (*RemainingNameBuffer != OBJ_NAME_PATH_SEPARATOR)
        {
            /* It's not, add it to the hash */
            CopyConvKey = COMPUTE_HASH_CHAR(CopyConvKey, *RemainingNameBuffer);

            /* Go to the next character (add up the length of the character as well) */
            RemainingNameBuffer++;
            KeyNameLength += sizeof(WCHAR);
            RemainingNameLength -= sizeof(WCHAR);

            /*
             * We are at the end of the key name path. Take into account
             * the last character and if we still have space in the hash
             * stack, add it up in the remaining list.
             */
            if (RemainingNameLength == 0)
            {
                if (RemainingSubkeysInTotal < CMP_SUBKEY_LEVELS_DEPTH_LIMIT)
                {
                    HashCacheStack[RemainingSubkeysInTotal].NameOfKey.Length = KeyNameLength;
                    HashCacheStack[RemainingSubkeysInTotal].NameOfKey.MaximumLength = KeyNameLength;
                    HashCacheStack[RemainingSubkeysInTotal].ConvKey = CopyConvKey;
                    RemainingSubkeysInTotal++;
                }

                SubkeysInTotal++;
            }
        }
        else
        {
            /* Skip any leading separator */
            while (RemainingNameLength >= sizeof(WCHAR) &&
                   *RemainingNameBuffer == OBJ_NAME_PATH_SEPARATOR)
            {
                RemainingNameBuffer++;
                RemainingNameLength -= sizeof(WCHAR);
            }

            /*
             * It would be possible that a malformed key pathname may be passed
             * to the registry parser such as a path with only separators like
             * "\\\\" for example. This would trick the function into believing
             * the key path has subkeys albeit that is not the case.
             */
            ASSERT(RemainingNameLength != 0);

            /* Take into account this subkey */
            SubkeysInTotal++;

            /* And add it up to the hash stack */
            if (RemainingSubkeysInTotal < CMP_SUBKEY_LEVELS_DEPTH_LIMIT)
            {
                HashCacheStack[RemainingSubkeysInTotal].NameOfKey.Length = KeyNameLength;
                HashCacheStack[RemainingSubkeysInTotal].NameOfKey.MaximumLength = KeyNameLength;
                HashCacheStack[RemainingSubkeysInTotal].ConvKey = CopyConvKey;

                RemainingSubkeysInTotal++;
                KeyNameLength = 0;

                /*
                 * Precaution check -- we have added up a remaining
                 * subkey above but we must ensure we still have space
                 * to hold up the new subkey for which we will compute
                 * the hashes, so that we don't blow up the hash stack.
                 */
                if (RemainingSubkeysInTotal < CMP_SUBKEY_LEVELS_DEPTH_LIMIT)
                {
                    HashCacheStack[RemainingSubkeysInTotal].NameOfKey.Buffer = RemainingNameBuffer;
                }
            }
        }
    }

    *TotalSubKeys = SubkeysInTotal;
    return RemainingSubkeysInTotal;
}

/**
 * @brief
 * Compares each subkey's hash and name with those
 * captured in the hash cache stack.
 *
 * @param[in] HashCacheStack
 * A pointer to a hash cache stack array filled with
 * subkey hashes and names for comparison.
 *
 * @param[in] CurrentKcb
 * A pointer to the currently given KCB.
 *
 * @param[in] RemainingSubkeys
 * The remaining subkey levels to be supplied.
 *
 * @param[out] ParentKcb
 * A pointer to the parent KCB returned to the caller.
 * This parameter points to the parent of the current
 * KCB if all the subkeys match, otherwise it points
 * to the actual current KCB.
 *
 * @return
 * Returns TRUE if all the subkey levels match, otherwise
 * FALSE is returned.
 */
static
BOOLEAN
CmpCompareSubkeys(
    _In_ PCM_HASH_CACHE_STACK HashCacheStack,
    _In_ PCM_KEY_CONTROL_BLOCK CurrentKcb,
    _In_ ULONG RemainingSubkeys,
    _Out_ PCM_KEY_CONTROL_BLOCK *ParentKcb)
{
    LONG HashStackIndex;
    LONG Result;
    PCM_NAME_CONTROL_BLOCK NameBlock;
    UNICODE_STRING CurrentNameBlock;

    ASSERT(CurrentKcb != NULL);

    /* Loop each hash and check that they match */
    HashStackIndex = RemainingSubkeys;
    while (HashStackIndex >= 0)
    {
        /* Does the subkey hash match? */
        if (CurrentKcb->ConvKey != HashCacheStack[HashStackIndex].ConvKey)
        {
            *ParentKcb = CurrentKcb;
            return FALSE;
        }

        /* Compare the subkey string, is the name compressed? */
        NameBlock = CurrentKcb->NameBlock;
        if (NameBlock->Compressed)
        {
            Result = CmpCompareCompressedName(&HashCacheStack[HashStackIndex].NameOfKey,
                                              NameBlock->Name,
                                              NameBlock->NameLength);
        }
        else
        {
            CurrentNameBlock.Buffer = NameBlock->Name;
            CurrentNameBlock.Length = NameBlock->NameLength;
            CurrentNameBlock.MaximumLength = NameBlock->NameLength;

            Result = RtlCompareUnicodeString(&HashCacheStack[HashStackIndex].NameOfKey,
                                             &CurrentNameBlock,
                                             TRUE);
        }

        /* Do the subkey names match? */
        if (Result)
        {
            *ParentKcb = CurrentKcb;
            return FALSE;
        }

        /* Go to the next subkey hash */
        HashStackIndex--;
    }

    /* All the subkeys match */
    *ParentKcb = CurrentKcb->ParentKcb;
    return TRUE;
}

/**
 * @brief
 * Removes the subkeys on a remaining key pathname.
 *
 * @param[in] HashCacheStack
 * A pointer to a hash cache stack array filled with
 * subkey hashes and names.
 *
 * @param[in] RemainingSubkeys
 * The remaining subkey levels to be supplied.
 *
 * @param[in,out] RemainingName
 * A Unicode string structure consisting of the remaining
 * registry key path name, where the subkeys of such path
 * are to be removed.
 */
static
VOID
CmpRemoveSubkeysInRemainingName(
    _In_ PCM_HASH_CACHE_STACK HashCacheStack,
    _In_ ULONG RemainingSubkeys,
    _Inout_ PUNICODE_STRING RemainingName)
{
    ULONG HashStackIndex = 0;

    /* Skip any leading separator on matching name */
    while (RemainingName->Length >= sizeof(WCHAR) &&
           RemainingName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR)
    {
        RemainingName->Buffer++;
        RemainingName->Length -= sizeof(WCHAR);
    }

    /* Skip the subkeys as well */
    while (HashStackIndex <= RemainingSubkeys)
    {
        RemainingName->Buffer += HashCacheStack[HashStackIndex].NameOfKey.Length / sizeof(WCHAR);
        RemainingName->Length -= HashCacheStack[HashStackIndex].NameOfKey.Length;

        /* Skip any leading separator */
        while (RemainingName->Length >= sizeof(WCHAR) &&
               RemainingName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR)
        {
            RemainingName->Buffer++;
            RemainingName->Length -= sizeof(WCHAR);
        }

        /* Go to the next hash */
        HashStackIndex++;
    }
}

/**
 * @brief
 * Looks up in the pool cache for key pathname that matches
 * with one in the said cache and returns a KCB pointing
 * to that name. This function performs locking of KCBs
 * during cache lookup.
 *
 * @param[in] HashCacheStack
 * A pointer to a hash cache stack array filled with
 * subkey hashes and names.
 *
 * @param[in] LockKcbsExclusive
 * If set to TRUE, the KCBs are locked exclusively by the
 * calling thread, otherwise they are locked in shared mode.
 * See Remarks for further information.
 *
 * @param[in] TotalRemainingSubkeys
 * The total remaining subkey levels to be supplied.
 *
 * @param[in,out] RemainingName
 * A Unicode string structure consisting of the remaining
 * registry key path name. The remaining name is updated
 * by the function if a key pathname is found in cache.
 *
 * @param[in,out] OuterStackArray
 * A pointer to an array that lives on the caller's stack.
 * The expected size of the array is up to 32 elements,
 * which is the imposed limit by CMP_HASH_STACK_LIMIT.
 * This limit also corresponds to the maximum depth of
 * subkey levels.
 *
 * @param[in,out] Kcb
 * A pointer to a KCB, this KCB is changed if the key pathname
 * is found in cache.
 *
 * @param[out] Hive
 * A pointer to a hive, this hive is changed if the key pathname
 * is found in cache.
 *
 * @param[out] Cell
 * A pointer to a cell, this cell is changed if the key pathname
 * is found in cache.
 *
 * @param[out] MatchRemainSubkeyLevel
 * A pointer to match subkey levels returned by the function.
 * If no match levels are found, this is 0.
 *
 * @return
 * Returns STATUS_SUCCESS if cache lookup has completed successfully.
 * STATUS_OBJECT_NAME_NOT_FOUND is returned if the current KCB of
 * the key pathname has been deleted. STATUS_RETRY is returned if
 * at least the current KCB or its parent have been deleted
 * and a cache lookup must be retried again. STATUS_UNSUCCESSFUL is
 * returned if a KCB is referenced too many times.
 *
 * @remarks
 * The function attempts to do a cache lookup with a shared lock
 * on KCBs so that other threads can simultaneously get access
 * to these KCBs. When the captured KCB is being deleted on us
 * we have to retry a lookup with exclusive look so that no other
 * threads will mess with the KCBs and perform appropriate actions
 * if a KCB is deleted.
 */
static
NTSTATUS
CmpLookInCache(
    _In_ PCM_HASH_CACHE_STACK HashCacheStack,
    _In_ BOOLEAN LockKcbsExclusive,
    _In_ ULONG TotalRemainingSubkeys,
    _Inout_ PUNICODE_STRING RemainingName,
    _Inout_ PULONG OuterStackArray,
    _Inout_ PCM_KEY_CONTROL_BLOCK *Kcb,
    _Out_ PHHIVE *Hive,
    _Out_ PHCELL_INDEX Cell,
    _Out_ PULONG MatchRemainSubkeyLevel)
{
    LONG RemainingSubkeys;
    ULONG TotalLevels;
    BOOLEAN SubkeysMatch;
    PCM_KEY_CONTROL_BLOCK CurrentKcb, ParentKcb;
    PCM_KEY_HASH HashEntry = NULL;
    BOOLEAN KeyFoundInCache = FALSE;
    PULONG LockedKcbs = NULL;

    /* Reference the KCB */
    if (!CmpReferenceKeyControlBlock(*Kcb))
    {
        /* This key is opened too many times, bail out */
        DPRINT1("Could not reference the KCB, too many references (KCB 0x%p)\n", Kcb);
        return STATUS_UNSUCCESSFUL;
    }

    /* Prepare to lock the KCBs */
    LockedKcbs = CmpBuildAndLockKcbArray(HashCacheStack,
                                         LockKcbsExclusive ? CMP_LOCK_KCB_ARRAY_EXCLUSIVE : CMP_LOCK_KCB_ARRAY_SHARED,
                                         *Kcb,
                                         OuterStackArray,
                                         TotalRemainingSubkeys,
                                         0);
    NT_ASSERT(LockedKcbs);

    /* Lookup in the cache */
    RemainingSubkeys = TotalRemainingSubkeys - 1;
    TotalLevels = TotalRemainingSubkeys + (*Kcb)->TotalLevels + 1;
    while (RemainingSubkeys >= 0)
    {
        /* Get the hash entry from the cache */
        HashEntry = GET_HASH_ENTRY(CmpCacheTable, HashCacheStack[RemainingSubkeys].ConvKey)->Entry;

        /* Take one level down as we are processing this hash entry */
        TotalLevels--;

        while (HashEntry != NULL)
        {
            /* Validate this hash and obtain the current KCB */
            ASSERT_VALID_HASH(HashEntry);
            CurrentKcb = CONTAINING_RECORD(HashEntry, CM_KEY_CONTROL_BLOCK, KeyHash);

            /* Does this KCB have matching levels? */
            if (TotalLevels == CurrentKcb->TotalLevels)
            {
                /*
                 * We have matching subkey levels but don't directly assume we have
                 * a matching key path in cache. Start comparing each subkey.
                 */
                SubkeysMatch = CmpCompareSubkeys(HashCacheStack,
                                                 CurrentKcb,
                                                 RemainingSubkeys,
                                                 &ParentKcb);
                if (SubkeysMatch)
                {
                    /* All subkeys match, now check if the base KCB matches with parent */
                    if (*Kcb == ParentKcb)
                    {
                        /* Is the KCB marked as deleted? */
                        if (CurrentKcb->Delete ||
                            CurrentKcb->ParentKcb->Delete)
                        {
                            /*
                             * Either the current or its parent KCB is marked
                             * but we had a shared lock so probably a naughty
                             * thread was deleting it. Retry doing a cache
                             * lookup again with exclusive lock.
                             */
                            if (!LockKcbsExclusive)
                            {
                                CmpUnLockKcbArray(LockedKcbs);
                                CmpDereferenceKeyControlBlock(*Kcb);
                                DPRINT1("The current KCB or its parent is deleted, retrying looking in the cache\n");
                                return STATUS_RETRY;
                            }

                            /* We're under an exclusive lock, is the KCB deleted yet? */
                            if (CurrentKcb->Delete)
                            {
                                /* The KCB is gone, the key should no longer belong in the cache */
                                CmpRemoveKeyControlBlock(CurrentKcb);
                                CmpUnLockKcbArray(LockedKcbs);
                                CmpDereferenceKeyControlBlock(*Kcb);
                                DPRINT1("The current KCB is deleted (KCB 0x%p)\n", CurrentKcb);
                                return STATUS_OBJECT_NAME_NOT_FOUND;
                            }

                            /*
                             * The parent is deleted so it must be that somebody created
                             * a fake key. Assert ourselves that is the case.
                             */
                            ASSERT(CurrentKcb->ExtFlags & CM_KCB_KEY_NON_EXIST);

                            /* Remove this KCB out of cache if someone still uses it */
                            if (CurrentKcb->RefCount != 0)
                            {
                                CurrentKcb->Delete = TRUE;
                                CmpRemoveKeyControlBlock(CurrentKcb);
                            }
                            else
                            {
                                /* Otherwise expunge it */
                                CmpRemoveFromDelayedClose(CurrentKcb);
                                CmpCleanUpKcbCacheWithLock(CurrentKcb, FALSE);
                            }

                            /* Stop looking for next hashes as the KCB is kaput */
                            break;
                        }

                        /* We finally found the key in cache, acknowledge it */
                        KeyFoundInCache = TRUE;

                        /* Remove the subkeys in the remaining name and stop looking in the cache */
                        CmpRemoveSubkeysInRemainingName(HashCacheStack, RemainingSubkeys, RemainingName);
                        break;
                    }
                }
            }

            /* Go to the next hash */
            HashEntry = HashEntry->NextHash;
        }

        /* Stop looking in cache if we found the matching key */
        if (KeyFoundInCache)
        {
            DPRINT("Key found in cache, stop looking\n");
            break;
        }

        /* Keep looking in the cache until we run out of remaining subkeys */
        RemainingSubkeys--;
    }

    /* Return the matching subkey levels */
    *MatchRemainSubkeyLevel = RemainingSubkeys + 1;

    /* We have to update the KCB if the key was found in cache */
    if (KeyFoundInCache)
    {
        /*
         * Before we change the KCB we must dereference the prior
         * KCB that we no longer need it.
         */
        CmpDereferenceKeyControlBlock(*Kcb);
        *Kcb = CurrentKcb;

        /* Reference the new KCB now */
        if (!CmpReferenceKeyControlBlock(*Kcb))
        {
            /* This key is opened too many times, bail out */
            DPRINT1("Could not reference the KCB, too many references (KCB 0x%p)\n", Kcb);
            return STATUS_UNSUCCESSFUL;
        }

        /* Update hive and cell data from current KCB */
        *Hive = CurrentKcb->KeyHive;
        *Cell = CurrentKcb->KeyCell;
    }

    /* Unlock the KCBs */
    CmpUnLockKcbArray(LockedKcbs);
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Builds a hash stack cache and looks up in the
 * pool cache for a matching key pathname.
 *
 * @param[in] ParseObject
 * A pointer to a parse object, acting as a key
 * body. This parameter is unused.
 *
 * @param[in,out] Kcb
 * A pointer to a KCB. This KCB is used by the
 * registry parser after hash stack and cache
 * lookup are done. This KCB might change if the
 * key is found to be cached in the cache pool.
 *
 * @param[in] Current
 * The current remaining key pathname.
 *
 * @param[out] Hive
 * A pointer to a registry hive, returned by the caller.
 *
 * @param[out] Cell
 * A pointer to a hive cell, returned by the caller.
 *
 * @param[out] TotalRemainingSubkeys
 * A pointer to a number of total remaining subkey levels,
 * returned by the caller. This can be 0 if no subkey levels
 * have been found.
 *
 * @param[out] MatchRemainSubkeyLevel
 * A pointer to a number of remaining subkey levels that match,
 * returned by the caller. This can be 0 if no matching levels
 * are found.
 *
 * @param[out] TotalSubkeys
 * A pointer to a number of total subkeys. This can be 0 if no
 * subkey levels are found. By definition, both MatchRemainSubkeyLevel
 * and TotalRemainingSubkeys are 0 as well.
 *
 * @param[in,out] OuterStackArray
 * A pointer to an array that lives on the caller's stack.
 * The expected size of the array is up to 32 elements,
 * which is the imposed limit by CMP_HASH_STACK_LIMIT.
 * This limit also corresponds to the maximum depth of
 * subkey levels.
 *
 * @param[out] LockedKcbs
 * A pointer to an array of locked KCBs, returned by the caller.
 *
 * @return
 * Returns STATUS_SUCCESS if all the operations have succeeded without
 * problems. STATUS_NAME_TOO_LONG is returned if the key pathname has
 * too many subkey levels (more than 32 levels deep). A failure NTSTATUS
 * code is returned otherwise. Refer to CmpLookInCache documentation
 * for more information about other returned status codes.
 * STATUS_UNSUCCESSFUL is returned if a KCB is referenced too many times.
 */
NTSTATUS
NTAPI
CmpBuildHashStackAndLookupCache(
    _In_ PCM_KEY_BODY ParseObject,
    _Inout_ PCM_KEY_CONTROL_BLOCK *Kcb,
    _In_ PUNICODE_STRING Current,
    _Out_ PHHIVE *Hive,
    _Out_ PHCELL_INDEX Cell,
    _Out_ PULONG TotalRemainingSubkeys,
    _Out_ PULONG MatchRemainSubkeyLevel,
    _Out_ PULONG TotalSubkeys,
    _Inout_ PULONG OuterStackArray,
    _Out_ PULONG *LockedKcbs)
{
    NTSTATUS Status;
    ULONG ConvKey;
    ULONG SubkeysInTotal, RemainingSubkeysInTotal, MatchRemainingSubkeys;
    CM_HASH_CACHE_STACK HashCacheStack[CMP_SUBKEY_LEVELS_DEPTH_LIMIT];

    /* Make sure it's not a dead KCB */
    ASSERT((*Kcb)->RefCount > 0);

    /* Lock the registry */
    CmpLockRegistry();

    /* Calculate hash values for every subkey this key path has */
    ConvKey = (*Kcb)->ConvKey;
    RemainingSubkeysInTotal = CmpComputeHashValue(Current,
                                                  ConvKey,
                                                  HashCacheStack,
                                                  &SubkeysInTotal);

    /* This key path has too many subkeys */
    if (SubkeysInTotal > CMP_SUBKEY_LEVELS_DEPTH_LIMIT)
    {
        DPRINT1("The key path has too many subkeys - %lu\n", SubkeysInTotal);
        *LockedKcbs = NULL;
        return STATUS_NAME_TOO_LONG;
    }

    /* Return hive and cell data */
    *Hive = (*Kcb)->KeyHive;
    *Cell = (*Kcb)->KeyCell;

    /* Do we have any subkeys? */
    if (!RemainingSubkeysInTotal && !SubkeysInTotal)
    {
        /*
         * We don't have any subkeys nor remaining levels, the
         * KCB points to the actual key. Lock it.
         */
        if (!CmpReferenceKeyControlBlock(*Kcb))
        {
            /* This key is opened too many times, bail out */
            DPRINT1("Could not reference the KCB, too many references (KCB 0x%p)\n", Kcb);
            return STATUS_UNSUCCESSFUL;
        }

        CmpAcquireKcbLockSharedByIndex(GET_HASH_INDEX(ConvKey));

        /* Add this KCB in the array of locked KCBs */
        OuterStackArray[0] = 1;
        OuterStackArray[1] = GET_HASH_INDEX(ConvKey);
        *LockedKcbs = OuterStackArray;

        /* And return all the subkey level counters */
        *TotalRemainingSubkeys = RemainingSubkeysInTotal;
        *MatchRemainSubkeyLevel = 0;
        *TotalSubkeys = SubkeysInTotal;
        return STATUS_SUCCESS;
    }

    /* Lookup in the cache */
    Status = CmpLookInCache(HashCacheStack,
                            FALSE,
                            RemainingSubkeysInTotal,
                            Current,
                            OuterStackArray,
                            Kcb,
                            Hive,
                            Cell,
                            &MatchRemainingSubkeys);
    if (!NT_SUCCESS(Status))
    {
        /* Bail out if cache lookup failed for other reasons */
        if (Status != STATUS_RETRY)
        {
            DPRINT1("CmpLookInCache() failed (Status 0x%lx)\n", Status);
            *LockedKcbs = NULL;
            return Status;
        }

        /* Retry looking in the cache but with KCBs locked exclusively */
        Status = CmpLookInCache(HashCacheStack,
                                TRUE,
                                RemainingSubkeysInTotal,
                                Current,
                                OuterStackArray,
                                Kcb,
                                Hive,
                                Cell,
                                &MatchRemainingSubkeys);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("CmpLookInCache() failed after retry (Status 0x%lx)\n", Status);
            *LockedKcbs = NULL;
            return Status;
        }
    }

    /*
     * Check if we have a full match of remaining levels.
     *
     * FIXME: It is possible we can catch a fake key from the cache
     * when we did the lookup, in such case we should not do any
     * locking as such KCB does not point to any real information.
     * Currently ReactOS doesn't create fake KCBs so we are good
     * for now.
     */
    if (RemainingSubkeysInTotal == MatchRemainingSubkeys)
    {
        /*
         * Just simply lock this KCB as it points to the full
         * subkey levels in cache.
         */
        CmpAcquireKcbLockSharedByIndex(GET_HASH_INDEX((*Kcb)->ConvKey));
        OuterStackArray[0] = 1;
        OuterStackArray[1] = GET_HASH_INDEX((*Kcb)->ConvKey);
        *LockedKcbs = OuterStackArray;
    }
    else
    {
        /*
         * We only have a partial match so other subkey levels
         * have each KCB. Simply just lock them.
         */
        *LockedKcbs = CmpBuildAndLockKcbArray(HashCacheStack,
                                              CMP_LOCK_KCB_ARRAY_EXCLUSIVE,
                                              *Kcb,
                                              OuterStackArray,
                                              RemainingSubkeysInTotal,
                                              MatchRemainingSubkeys);
        NT_ASSERT(*LockedKcbs);
    }

    /* Return all the subkey level counters */
    *TotalRemainingSubkeys = RemainingSubkeysInTotal;
    *MatchRemainSubkeyLevel = MatchRemainingSubkeys;
    *TotalSubkeys = SubkeysInTotal;
    return Status;
}

NTSTATUS
NTAPI
CmpParseKey(IN PVOID ParseObject,
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
    ULONG LockedKcbArray[CMP_KCBS_IN_ARRAY_LIMIT];
    PULONG LockedKcbs;
    BOOLEAN IsKeyCached = FALSE;
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

    /* Sanity check */
    ASSERT(Kcb != NULL);

    /* Fail if the key was marked as deleted */
    if (Kcb->Delete)
        return STATUS_KEY_DELETED;

    /* Lookup in the cache */
    Status = CmpBuildHashStackAndLookupCache(ParseObject,
                                             &Kcb,
                                             &Current,
                                             &Hive,
                                             &Cell,
                                             &TotalRemainingSubkeys,
                                             &MatchRemainSubkeyLevel,
                                             &TotalSubkeys,
                                             LockedKcbArray,
                                             &LockedKcbs);
    CMP_ASSERT_REGISTRY_LOCK();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to look in cache, stop parsing (Status 0x%lx)\n", Status);
        ParentKcb = NULL;
        goto Quickie;
    }

    /* This is now the parent */
    ParentKcb = Kcb;

    /* Sanity check */
    ASSERT(ParentKcb != NULL);

    /* Don't do anything if we're being deleted */
    if (Kcb->Delete)
    {
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto Quickie;
    }

    /* Check if everything was found cached */
    if (!TotalRemainingSubkeys)
    {
        /*
         * We don't have any remaining subkey levels so we're good
         * that we have an already perfect candidate for a KCB, just
         * do the open directly.
         */
        DPRINT("No remaining subkeys, the KCB points to the actual key\n");
        IsKeyCached = TRUE;
        goto KeyCachedOpenNow;
    }

    /* Check if we have a matching level */
    if (MatchRemainSubkeyLevel)
    {
        /*
         * We have a matching level, check if that matches
         * with the total levels of subkeys. Do the open directly
         * if that is the case, because the whole subkeys levels
         * is cached.
         */
        if (MatchRemainSubkeyLevel == TotalSubkeys)
        {
            DPRINT("We have a full matching level, open the key now\n");
            IsKeyCached = TRUE;
            goto KeyCachedOpenNow;
        }

        /*
         * We only have a partial match, make sure we did not
         * get mismatched hive data.
         */
        ASSERT(Hive == Kcb->KeyHive);
        ASSERT(Cell == Kcb->KeyCell);
    }

    /*
     * FIXME: Currently the registry parser doesn't check for fake
     * KCBs. CmpCreateKeyControlBlock does have the necessary implementation
     * to create such fake keys but we don't create these fake keys anywhere.
     * When we will do, we must improve the registry parser routine to handle
     * fake keys a bit differently here.
     */

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

        /* CmpGetSymbolicLink doesn't want a lock */
        CmpUnLockKcbArray(LockedKcbs);
        LockedKcbs = NULL;

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
    if (!Node)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Quickie;
    }

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
                    ASSERT(Node);

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
                            if (!Node)
                            {
                                /* Fail */
                                Status = STATUS_INSUFFICIENT_RESOURCES;
                                break;
                            }
                        }

KeyCachedOpenNow:
                        /* Do the open */
                        Status = CmpDoOpen(Hive,
                                           Cell,
                                           Node,
                                           AccessState,
                                           AccessMode,
                                           Attributes,
                                           ParseContext,
                                           IsKeyCached ? CMP_OPEN_KCB_NO_CREATE : CMP_CREATE_KCB_KCB_LOCKED,
                                           &Kcb,
                                           LockedKcbs,
                                           &NextName,
                                           Object);
                        if (Status == STATUS_REPARSE)
                        {
                            /* CmpGetSymbolicLink doesn't want a lock */
                            CmpUnLockKcbArray(LockedKcbs);
                            LockedKcbs = NULL;

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
                        if (!Node)
                        {
                            /* Fail */
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            break;
                        }
                    }

                    /* Create a KCB for this key */
                    Kcb = CmpCreateKeyControlBlock(Hive,
                                                   Cell,
                                                   Node,
                                                   ParentKcb,
                                                   CMP_LOCK_HASHES_FOR_KCB,
                                                   &NextName);
                    if (!Kcb)
                    {
                        /* Fail */
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        break;
                    }

                    /* Dereference the parent and set the new one */
                    CmpDereferenceKeyControlBlockWithLock(ParentKcb, FALSE);
                    ParentKcb = Kcb;
                }
                else
                {
                    /* Check if this was the last key for a create */
                    if ((Last) && (ParseContext))
                    {
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
                                                       LockedKcbs,
                                                       Object);
                        }
                        else if (Hive == &CmiVolatileHive->Hive && CmpNoVolatileCreates)
                        {
                            /* Creating keys in the master hive is not allowed */
                            Status = STATUS_INVALID_PARAMETER;
                        }
                        else
                        {
                            /* Do the create */
                            Status = CmpDoCreate(Hive,
                                                 Cell,
                                                 AccessState,
                                                 &NextName,
                                                 AccessMode,
                                                 ParseContext,
                                                 ParentKcb,
                                                 Object);
                        }

                        /* Check for reparse (in this case, someone beat us) */
                        if (Status == STATUS_REPARSE) break;

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

                /* CmpGetSymbolicLink doesn't want a lock */
                CmpUnLockKcbArray(LockedKcbs);
                LockedKcbs = NULL;

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
                if (!Node)
                {
                    /* Fail */
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }
            }

            /* Do the open */
            Status = CmpDoOpen(Hive,
                               Cell,
                               Node,
                               AccessState,
                               AccessMode,
                               Attributes,
                               ParseContext,
                               CMP_OPEN_KCB_NO_CREATE,
                               &Kcb,
                               LockedKcbs,
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

Quickie:
    /* Unlock all the KCBs */
    if (LockedKcbs != NULL)
    {
        CmpUnLockKcbArray(LockedKcbs);
    }

    /* Dereference the parent if it exists */
    if (ParentKcb)
        CmpDereferenceKeyControlBlock(ParentKcb);

    /* Unlock the registry */
    CmpUnlockRegistry();
    return Status;
}
