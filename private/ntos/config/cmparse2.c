/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmparse2.c

Abstract:

    This module contains parse routines for the configuration manager, particularly
    the registry.

Author:

    Bryan M. Willman (bryanwi) 10-Sep-1991

Revision History:

--*/

#include    "cmp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpDoCreate)
#pragma alloc_text(PAGE,CmpDoCreateChild)
#endif

extern  PCM_KEY_CONTROL_BLOCK CmpKeyControlBlockRoot;


NTSTATUS
CmpDoCreate(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN PACCESS_STATE AccessState,
    IN PUNICODE_STRING Name,
    IN KPROCESSOR_MODE AccessMode,
    IN PCM_PARSE_CONTEXT Context,
    IN PCM_KEY_CONTROL_BLOCK ParentKcb,
    OUT PVOID *Object
    )
/*++

Routine Description:

    Performs the first step in the creation of a registry key.  This
    routine checks to make sure the caller has the proper access to
    create a key here, and allocates space for the child in the parent
    cell.  It then calls CmpDoCreateChild to initialize the key and
    create the key object.

    This two phase creation allows us to share the child creation code
    with the creation of link nodes.

Arguments:

    Hive - supplies a pointer to the hive control structure for the hive

    Cell - supplies index of node to create child under.

    AccessState - Running security access state information for operation.

    Name - supplies pointer to a UNICODE string which is the name of
            the child to be created.

    AccessMode - Access mode of the original caller.

    Context - pointer to CM_PARSE_CONTEXT structure passed through
                the object manager

    BaseName - Name of object create is relative to

    KeyName - Relative name (to BaseName)

    Object - The address of a variable to receive the created key object, if
             any.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;
    PCELL_DATA pparent;
    PCELL_DATA pdata;
    HCELL_INDEX KeyCell;
    ULONG       ParentType;
    ACCESS_MASK AdditionalAccess;
    BOOLEAN CreateAccess;
    PCM_KEY_BODY KeyBody;

    CMLOG(CML_FLOW, CMS_PARSE) {
        KdPrint(("CmpDoCreate:\n"));
    }

    if (ARGUMENT_PRESENT(Context)) {

        if (Context->CreateOptions & REG_OPTION_BACKUP_RESTORE) {

            //
            // we're supposed to be opening for a backup/restore
            // operation, but this is a new create, which makes
            // no sense, so fail.
            //
            return STATUS_OBJECT_NAME_NOT_FOUND;
        }

        //
        // Operation is a create, so set Disposition
        //
        Context->Disposition = REG_CREATED_NEW_KEY;
    }

    //
    // Check to make sure the caller can create a sub-key here.
    //
    pparent = HvGetCell(Hive, Cell);
    pdata = HvGetCell(Hive, pparent->u.KeyNode.Security);
    ParentType = HvGetCellType(Cell);

    if ( (ParentType == Volatile) &&
         ((Context->CreateOptions & REG_OPTION_VOLATILE) == 0) )
    {
        //
        // Trying to create stable child under volatile parent, report error
        //
        return STATUS_CHILD_MUST_BE_VOLATILE;
    }

    if (pparent->u.KeyNode.Flags &   KEY_SYM_LINK)
    {
        //
        // Disallow attempts to create anything under a symbolic link
        //
        return STATUS_ACCESS_DENIED;
    }

    AdditionalAccess = (Context->CreateOptions & REG_OPTION_CREATE_LINK) ? KEY_CREATE_LINK : 0;

    //
    // The FullName is not used in the routine CmpCheckCreateAccess,
    // 
    CreateAccess = CmpCheckCreateAccess(NULL,
                                        &(pdata->u.KeySecurity.Descriptor),
                                        AccessState,
                                        AccessMode,
                                        AdditionalAccess,
                                        &status);

    if (CreateAccess) {

        //
        // Security check passed, so we can go ahead and create
        // the sub-key.
        //
        if ( !(Context->CreateOptions & REG_OPTION_VOLATILE) &&
             !HvMarkCellDirty(Hive, Cell)) {

            return STATUS_NO_LOG_SPACE;
        }

        //
        // Create and initialize the new sub-key
        //
        status = CmpDoCreateChild( Hive,
                                   Cell,
                                   &(pdata->u.KeySecurity.Descriptor),
                                   AccessState,
                                   Name,
                                   AccessMode,
                                   Context,
                                   ParentKcb,
                                   0,
                                   &KeyCell,
                                   Object );

        if (NT_SUCCESS(status)) {

            //
            // Child successfully created, add to parent's list.
            //
            if (! CmpAddSubKey(Hive, Cell, KeyCell)) {

                //
                // Unable to add child, so free it
                //
                CmpFreeKeyByCell(Hive, KeyCell, FALSE);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // Update max keyname and class name length fields
            //
            if (pparent->u.KeyNode.MaxNameLen < Name->Length) {
                pparent->u.KeyNode.MaxNameLen = Name->Length;
            }

            if (pparent->u.KeyNode.MaxClassLen < Context->Class.Length) {
                pparent->u.KeyNode.MaxClassLen = Context->Class.Length;
            }

            KeyBody = (PCM_KEY_BODY)(*Object);

            //
            // A new key is created, invalid the subkey info of the parent KCB.
            //
            ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

            CmpCleanUpSubKeyInfo (KeyBody->KeyControlBlock->ParentKcb);

            if (Context->CreateOptions & REG_OPTION_CREATE_LINK) {
                pdata = HvGetCell(Hive, KeyCell);
                pdata->u.KeyNode.Flags |= KEY_SYM_LINK;
                KeyBody->KeyControlBlock->Flags = pdata->u.KeyNode.Flags;

            }
        }
    }
    return status;
}


NTSTATUS
CmpDoCreateChild(
    IN PHHIVE Hive,
    IN HCELL_INDEX ParentCell,
    IN PSECURITY_DESCRIPTOR ParentDescriptor OPTIONAL,
    IN PACCESS_STATE AccessState,
    IN PUNICODE_STRING Name,
    IN KPROCESSOR_MODE AccessMode,
    IN PCM_PARSE_CONTEXT Context,
    IN PCM_KEY_CONTROL_BLOCK ParentKcb,
    IN USHORT Flags,
    OUT PHCELL_INDEX KeyCell,
    OUT PVOID *Object
    )

/*++

Routine Description:

    Creates a new sub-key.  This is called by CmpDoCreate to create child
    sub-keys and CmpCreateLinkNode to create root sub-keys.

Arguments:

    Hive - supplies a pointer to the hive control structure for the hive

    ParentCell - supplies cell index of parent cell

    ParentDescriptor - Supplies security descriptor of parent key, for use
           in inheriting ACLs.

    AccessState - Running security access state information for operation.

    Name - Supplies pointer to a UNICODE string which is the name of the
           child to be created.

    AccessMode - Access mode of the original caller.

    Context - Supplies pointer to CM_PARSE_CONTEXT structure passed through
           the object manager.

    BaseName - Name of object create is relative to

    KeyName - Relative name (to BaseName)

    Flags - Supplies any flags to be set in the newly created node

    KeyCell - Receives the cell index of the newly created sub-key, if any.

    Object - Receives a pointer to the created key object, if any.

Return Value:

    STATUS_SUCCESS - sub-key successfully created.  New object is returned in
            Object, and the new cell's cell index is returned in KeyCell.

    !STATUS_SUCCESS - appropriate error message.

--*/

{
    ULONG clean=0;
    ULONG alloc=0;
    NTSTATUS Status = STATUS_SUCCESS;
    PCM_KEY_BODY KeyBody;
    HCELL_INDEX ClassCell=HCELL_NIL;
    PCM_KEY_NODE KeyNode;
    PCELL_DATA CellData;
    PCM_KEY_CONTROL_BLOCK kcb;
    PCM_KEY_CONTROL_BLOCK fkcb;
    LONG found;
    ULONG StorageType;
    PSECURITY_DESCRIPTOR NewDescriptor = NULL;
    LARGE_INTEGER systemtime;

    CMLOG(CML_FLOW, CMS_PARSE) {
        KdPrint(("CmpDoCreateChild:\n"));
    }
    try {
        //
        // Get allocation type
        //
        StorageType = Stable;
        if (Context->CreateOptions & REG_OPTION_VOLATILE) {
            StorageType = Volatile;
        }

        //
        // Allocate child cell
        //
        *KeyCell = HvAllocateCell(
                        Hive,
                        CmpHKeyNodeSize(Hive, Name),
                        StorageType
                        );
        if (*KeyCell == HCELL_NIL) {
			Status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
        }
        alloc = 1;
        KeyNode = (PCM_KEY_NODE)HvGetCell(Hive, *KeyCell);

        //
        // Allocate cell for class name
        //
        if (Context->Class.Length > 0) {
            ClassCell = HvAllocateCell(Hive, Context->Class.Length, StorageType);
            if (ClassCell == HCELL_NIL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
				__leave;
            }
        }
        alloc = 2;
        //
        // Allocate the object manager object
        //
        Status = ObCreateObject(AccessMode,
                                CmpKeyObjectType,
                                NULL,
                                AccessMode,
                                NULL,
                                sizeof(CM_KEY_BODY),
                                0,
                                0,
                                Object);

        if (NT_SUCCESS(Status)) {

            KeyBody = (PCM_KEY_BODY)(*Object);

            //
            // We have managed to allocate all of the objects we need to,
            // so initialize them
            //

            //
            // Mark the object as uninitialized (in case we get an error too soon)
            //
            KeyBody->Type = KEY_BODY_TYPE;
            KeyBody->KeyControlBlock = NULL;

            //
            // Fill in the class name
            //
            if (Context->Class.Length > 0) {

                CellData = HvGetCell(Hive, ClassCell);

                try {

                    RtlMoveMemory(
                        &(CellData->u.KeyString[0]),
                        Context->Class.Buffer,
                        Context->Class.Length
                        );

                } except(EXCEPTION_EXECUTE_HANDLER) {
                    ObDereferenceObject(*Object);
                    return GetExceptionCode();
                }
            }

            //
            // Fill in the new key itself
            //
            KeyNode->Signature = CM_KEY_NODE_SIGNATURE;
            KeyNode->Flags = Flags;

            KeQuerySystemTime(&systemtime);
            KeyNode->LastWriteTime = systemtime;

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

            KeyNode->NameLength = CmpCopyName(Hive,
                                              KeyNode->Name,
                                              Name);
            if (KeyNode->NameLength < Name->Length) {
                KeyNode->Flags |= KEY_COMP_NAME;
            }

            if (Context->CreateOptions & REG_OPTION_PREDEF_HANDLE) {
                KeyNode->ValueList.Count = (ULONG)((ULONG_PTR)Context->PredefinedHandle);
                KeyNode->Flags |= KEY_PREDEF_HANDLE;
            }

            //
            // Create kcb here so all data are filled in.
            //
            // Allocate a key control block
            //
            kcb = CmpCreateKeyControlBlock(Hive, *KeyCell, KeyNode, ParentKcb, FALSE, Name);
            if (kcb == NULL) {
                ObDereferenceObject(*Object);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            ASSERT(kcb->RefCount == 1);
            alloc = 3;

            //
            // Fill in CM specific fields in the object
            //
            KeyBody->Type = KEY_BODY_TYPE;
            KeyBody->KeyControlBlock = kcb;
            KeyBody->NotifyBlock = NULL;
            KeyBody->Process = PsGetCurrentProcess();
            ENLIST_KEYBODY_IN_KEYBODY_LIST(KeyBody);
            //
            // Assign a security descriptor to the object.  Note that since
            // registry keys are container objects, and ObAssignSecurity
            // assumes that the only container object in the world is
            // the ObpDirectoryObjectType, we have to call SeAssignSecurity
            // directly in order to get the right inheritance.
            //

            Status = SeAssignSecurity(ParentDescriptor,
                                      AccessState->SecurityDescriptor,
                                      &NewDescriptor,
                                      TRUE,             // container object
                                      &AccessState->SubjectSecurityContext,
                                      &CmpKeyObjectType->TypeInfo.GenericMapping,
                                      CmpKeyObjectType->TypeInfo.PoolType);
            if (NT_SUCCESS(Status)) {
                Status = CmpSecurityMethod(*Object,
                                           AssignSecurityDescriptor,
                                           NULL,
                                           NewDescriptor,
                                           NULL,
                                           NULL,
                                           CmpKeyObjectType->TypeInfo.PoolType,
                                           &CmpKeyObjectType->TypeInfo.GenericMapping);
            }

            //
            // Since the security descriptor now lives in the hive,
            // free the in-memory copy
            //
            SeDeassignSecurity( &NewDescriptor );

            if (!NT_SUCCESS(Status)) {

                //
                // Note that the dereference will clean up the kcb, so
                // make sure and decrement the allocation count here.
                //
                // Also mark the kcb as deleted so it does not get 
                // inappropriately cached.
                //
                ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
                kcb->Delete = TRUE;
                CmpRemoveKeyControlBlock(kcb);
                ObDereferenceObject(*Object);
                alloc = 2;

            } else {
                CmpReportNotify(
                        kcb,
                        kcb->KeyHive,
                        kcb->KeyCell,
                        REG_NOTIFY_CHANGE_NAME
                        );
            }
        }

    } finally {

        if (!NT_SUCCESS(Status)) {

            //
            // Clean up allocations
            //
            switch (alloc) {
            case 3:
                //
                // Mark KCB as deleted so it does not get inadvertently added to 
                // the delayed close list. That would have fairly disastrous effects
                // as the KCB points to storage we are about to free.
                //
                ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
                kcb->Delete = TRUE;
                CmpRemoveKeyControlBlock(kcb);
                CmpDereferenceKeyControlBlockWithLock(kcb);
                // DELIBERATE FALL

            case 2:
                if (Context->Class.Length > 0) {
                    HvFreeCell(Hive, ClassCell);
                }
                // DELIBERATE FALL

            case 1:
                HvFreeCell(Hive, *KeyCell);
                // DELIBERATE FALL
            }
        }
    }

    return(Status);
}
