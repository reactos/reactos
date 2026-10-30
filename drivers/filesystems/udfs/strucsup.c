#include "udffs.h"

// The Bug check file id for this module

#define BugCheckFileId                   (UDFS_BUG_CHECK_STRUCSUP)

typedef struct _FCB_TABLE_ELEMENT {

    FILE_ID FileId;
    PFCB Fcb;

} FCB_TABLE_ELEMENT, *PFCB_TABLE_ELEMENT;

#define UDFInsertFcbTable(IC,F) {                                    \
     ASSERT_LOCKED_VCB( (F)->Vcb );                                  \
     FCB_TABLE_ELEMENT _Key;                                         \
     _Key.Fcb = (F);                                                 \
     _Key.FileId = (F)->FileId;                                      \
     RtlInsertElementGenericTable( &(F)->Vcb->FcbTable,              \
                                   &_Key,                            \
                                   sizeof( FCB_TABLE_ELEMENT ),      \
                                   NULL );                           \
}

#define UDFDeleteFcbTable(IC,F) {                                    \
     ASSERT_LOCKED_VCB( (F)->Vcb );                                  \
     FCB_TABLE_ELEMENT _Key;                                         \
     _Key.FileId = (F)->FileId;                                      \
     RtlDeleteElementGenericTable( &(F)->Vcb->FcbTable, &_Key );     \
}

//
// Public wrapper for inserting FCB into FcbTable.
// Called from create.cpp after FCB is fully initialized.
// VCB must be locked by caller.
//
VOID
UDFInsertFcbIntoTable(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFCB Fcb
    )
{
    UNREFERENCED_PARAMETER(IrpContext);

    UDFInsertFcbTable(IrpContext, Fcb);
    SetFlag(Fcb->FcbState, FCB_STATE_IN_FCB_TABLE);
}

static
inline
PFCB_NONPAGED
UDFAllocateFcbNonpaged(
)
{
    return (PFCB_NONPAGED)ExAllocateFromNPagedLookasideList(&UdfData.UDFNonPagedFcbLookasideList);
}

static
inline
PFCB
UDFAllocateFcbIndex(
)
{
    return (PFCB)ExAllocateFromPagedLookasideList(&UdfData.UDFFcbIndexLookasideList);
}

static
inline
PFCB
UDFAllocateFcbData(
)
{
    return (PFCB)ExAllocateFromPagedLookasideList(&UdfData.UDFFcbDataLookasideList);
}

static
inline
PFCB
UDFAllocateFcb(
)
{
    return (PFCB)ExAllocatePoolWithTag(NonPagedPool, sizeof(struct FCB), TAG_FCB);
}

static
inline
VOID
UDFDeallocateFcbNonpaged(
    PFCB_NONPAGED FcbNonpaged
    )
{
    ExFreeToNPagedLookasideList(&UdfData.UDFNonPagedFcbLookasideList, FcbNonpaged);
}

static
inline
VOID
UDFDeallocateFcbIndex(
    PFCB Fcb
    )
{
    ExFreeToPagedLookasideList(&UdfData.UDFFcbIndexLookasideList, Fcb);
}

static
inline
VOID
UDFDeallocateFcbData(
    PFCB Fcb
    )
{
    ExFreeToPagedLookasideList(&UdfData.UDFFcbDataLookasideList, Fcb);
}

PFCB_NONPAGED
UDFCreateFcbNonpaged(
    _In_ PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine is called to create and initialize the non-paged portion
    of an Fcb.

Arguments:

Return Value:

    PFCB_NONPAGED - Pointer to the created nonpaged Fcb.  NULL if not created.

--*/

{
    PFCB_NONPAGED FcbNonpaged;

    PAGED_CODE();
    
    UNREFERENCED_PARAMETER(IrpContext);
    
    //  Allocate the non-paged pool and initialize the various
    //  synchronization objects.

    FcbNonpaged = UDFAllocateFcbNonpaged();

    RtlZeroMemory(FcbNonpaged, sizeof(struct FCB_NONPAGED));

    FcbNonpaged->NodeTypeCode = UDF_NODE_TYPE_FCB_NONPAGED;
    FcbNonpaged->NodeByteSize = sizeof(struct FCB_NONPAGED);

    ExInitializeResourceLite(&FcbNonpaged->FcbPagingIoResource);
    ExInitializeResourceLite(&FcbNonpaged->FcbResource);
    ExInitializeFastMutex(&FcbNonpaged->FcbMutex);
    ExInitializeFastMutex(&FcbNonpaged->AdvancedFcbHeaderMutex);
    ExInitializeFastMutex(&FcbNonpaged->FcbFastMutex);

    return FcbNonpaged;
}

VOID
UDFDeleteFcbNonpaged (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFCB_NONPAGED FcbNonpaged
    )

/*++

Routine Description:

    This routine is called to cleanup the non-paged portion of an Fcb.

Arguments:

    FcbNonpaged - Structure to clean up.

Return Value:

    None

--*/

{
    PAGED_CODE();
    
    UNREFERENCED_PARAMETER(IrpContext);
    
    ExDeleteResourceLite(&FcbNonpaged->FcbResource);
    ExDeleteResourceLite(&FcbNonpaged->FcbPagingIoResource);

    UDFDeallocateFcbNonpaged(FcbNonpaged);

    return;
}

VOID
UDFDeleteFcb(
    _In_opt_ PIRP_CONTEXT IrpContext,
    _In_ PFCB Fcb
    )

/*++

Routine Description:

    This routine is called to cleanup and deallocate an Fcb.  We know there
    are no references remaining.  We cleanup any auxilary structures and
    deallocate this Fcb.

    NOTE: Caller should remove FCB from FcbTable before calling this routine
    (typically done by UDFTeardownStructures while holding VCB lock).
    FCB should NOT be in FcbTable when this is called - either it was removed
    by TeardownStructures, or it was never inserted (error path in create).

Arguments:

    IrpContext - Optional IrpContext (unused but kept for API consistency).

    Fcb - This is the Fcb to deallocate.

Return Value:

    None

--*/

{
    PVCB Vcb = NULL;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(IrpContext);

    UDFPrint(("UDFDeleteFcb: %x\n", Fcb));

    ASSERT_FCB(Fcb);

    // Sanity check the counts.

    NT_ASSERT( Fcb->FcbCleanup == 0 );
    NT_ASSERT( Fcb->FcbReference == 0 );

    // FCB must NOT be in FcbTable - either TeardownStructures removed it,
    // or it was never inserted (error path before UDFInsertFcbIntoTable).
    NT_ASSERT(!FlagOn(Fcb->FcbState, FCB_STATE_IN_FCB_TABLE));
    NT_ASSERT(!Fcb->MetadataCloseQueued);

    // LCB queues must be empty - all LCBs should be removed during teardown
    NT_ASSERT(IsListEmpty(&Fcb->ParentLcbQueue));
    NT_ASSERT(IsListEmpty(&Fcb->ChildLcbQueue));

    // NOTE: FCB no longer stores FCBName - all names are stored in LCB.
    // No FCBName cleanup needed.

    // Release any Filter Context structures associated with this FCB.
    // Only if FCB was fully initialized (Header.Resource is set by
    // UDFInitializeFCB).

    if (Fcb->Header.Resource) {
        FsRtlTeardownPerStreamContexts(&Fcb->Header);
    }

    // Delete non-paged portion (resources + dealloc).

    UDFDeleteFcbNonpaged(IrpContext, Fcb->FcbNonpaged);

    // Now do the type specific structures.

    switch (Fcb->Header.NodeTypeCode) {

    case UDF_NODE_TYPE_INDEX:

        if (Fcb == Fcb->Vcb->RootIndexFcb) {

            Vcb = Fcb->Vcb;
            Vcb->RootIndexFcb = NULL;
        }

        UDFDeallocateFcbIndex(Fcb);
        break;

    case UDF_NODE_TYPE_DATA:

        if (Fcb->FileLock != NULL) {

            FsRtlFreeFileLock( Fcb->FileLock );
        }

        if (Fcb == Fcb->Vcb->VolumeDasdFcb) {

            Vcb = Fcb->Vcb;
            Vcb->VolumeDasdFcb = NULL;
        }

        UDFDeallocateFcbData(Fcb);
        break;
    }

    // Decrement the Vcb reference count if this is a system
    // Fcb.

    if (Vcb != NULL) {

        InterlockedDecrement( (LONG*)&Vcb->VcbReference );
        InterlockedDecrement( (LONG*)&Vcb->VcbUserReference );
    }

    return;
}

/*
    This routine walks through the tree to RootDir & kills all unreferenced
    structures using LCB-based parent traversal.

    StartingFcb must be acquired exclusively by caller.
    This function will acquire locks for parent FCBs as needed.

    The algorithm:
    1. If FcbReference != 0, break (FCB still in use)
    2. Walk ParentLcbQueue to find LCBs with Reference == 0
    3. For each such LCB: remove it and decrement parent's FcbReference/FileInfo->RefCount
    4. If parent's FcbReference goes to 0, recursively tear down parent
    5. Delete the FCB when all LCBs are processed
 */
_Requires_lock_held_(_Global_critical_region_)
VOID
UDFTeardownStructures(
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PFCB StartingFcb,
    _In_ BOOLEAN Recursive,      // TRUE if this is a recursive call (for hard links)
    _Out_ PBOOLEAN RemovedStartingFcb
    )
{
    PVCB Vcb = StartingFcb->Vcb;
    PFCB CurrentFcb = StartingFcb;
    PFCB ParentFcb = NULL;
    PLCB Lcb;
    PLIST_ENTRY ListLinks;

    BOOLEAN Delete = FALSE;
    BOOLEAN AcquiredCurrentFcb = FALSE;
    BOOLEAN Abort = FALSE;
    BOOLEAN Removed;

    AdPrint(("UDFTeardownStructures, StartingFcb %p %s\n",
             StartingFcb, Recursive ? "Recursive" : "Flat"));

    ASSERT_EXCLUSIVE_FCB(StartingFcb);

    *RemovedStartingFcb = FALSE;

    //
    // If this is not an intentionally recursive call we need to check if this
    // is a layered close and we're already in another instance of teardown.
    //
    if (!Recursive) {
        if (FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_IN_TEARDOWN)) {
            return;
        }
        SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_IN_TEARDOWN);
    }

    _SEH2_TRY {

        //
        // Loop until we find an Fcb we can't remove.
        //
        do {
            /* A deferred parent delete still needs its FE on disk before
             * the last prefix and FCB-table owner are detached. */
            BOOLEAN FinalFlushPending = CurrentFcb->FileInfo &&
                (Delete ? !CurrentFcb->FcbCleanup :
                          !(CurrentFcb->FcbState & UDF_FCB_DELETED));

            //
            // If the reference count is non-zero then break.
            //
            if (CurrentFcb->FcbReference != 0) {
                break;
            }

            //
            // It looks like we have a candidate for removal here.  We
            // will need to walk the list of prefixes (LCBs) and delete them
            // from their parents.  If it turns out that we have multiple
            // parents of this Fcb (hard links), we are going to recursively
            // teardown on each of these.
            //
            for (ListLinks = CurrentFcb->ParentLcbQueue.Flink;
                 ListLinks != &CurrentFcb->ParentLcbQueue; ) {

                Lcb = CONTAINING_RECORD(ListLinks, struct LCB, ChildFcbLinks);

                ASSERT(Lcb->NodeIdentifier.NodeTypeCode == UDF_NODE_TYPE_LCB);

                //
                // We advance the pointer now because we will be toasting this guy,
                // invalidating whatever is here.
                //
                ListLinks = ListLinks->Flink;

                //
                // We may have multiple parents through hard links.  If the previous parent we
                // dealt with is not the parent of this new Lcb, lets do some work.
                //
                if (ParentFcb != Lcb->ParentFcb) {

                    //
                    // We need to deal with the previous parent.  It may now be the case that
                    // we deleted the last child reference and it wants to go away at this point.
                    //
                    if (ParentFcb) {
                        //
                        // It should never be the case that we have to recurse more than one level on
                        // any teardown since no cross-linkage of directories is possible.
                        //
                        ASSERT(!Recursive);

                        UDFTeardownStructures(IrpContext, ParentFcb, TRUE, &Removed);

                        if (!Removed) {
                            UDFReleaseFcb(IrpContext, ParentFcb);
                        }
                    }

                    //
                    // Get this new parent Fcb to work on.
                    //
                    ParentFcb = Lcb->ParentFcb;
                    UDFAcquireFcbExclusive(IrpContext, ParentFcb, FALSE);
                }

                /* The old late flush ran with this parent FCB acquired.
                 * Preserve that lock ordering, but write before detaching
                 * the first name link so a failure still has a retry owner.
                 * VcbMutex is not held during the metadata I/O. */
                if (FinalFlushPending) {
                    if (!NT_SUCCESS(UDFFlushFile__(IrpContext, Vcb,
                                                   CurrentFcb->FileInfo, 0))) {
                        UDFReleaseFcb(IrpContext, ParentFcb);
                        ParentFcb = NULL;
                        Abort = TRUE;
                        break;
                    }
                    FinalFlushPending = FALSE;
                }

                //
                // Lock the Vcb so we can look at references.
                //
                UDFLockVcb(IrpContext, Vcb);

                //
                // Now check that the reference counts on the Lcb are zero.
                //
                if (Lcb->Reference != 0) {
                    //
                    // A create is interested in getting in here, so we should
                    // stop right now.
                    //
                    UDFUnlockVcb(IrpContext, Vcb);
                    UDFReleaseFcb(IrpContext, ParentFcb);
                    ParentFcb = NULL;
                    Abort = TRUE;
                    break;
                }

                //
                // Now remove this prefix and drop the references to the parent.
                //
                ASSERT(Lcb->ChildFcb == CurrentFcb);
                ASSERT(Lcb->ParentFcb == ParentFcb);

                AdPrint(("UDFTeardownStructures, removing Lcb %p P %p <-> C %p\n",
                         Lcb, ParentFcb, CurrentFcb));

                //
                // Remove LCB from queues (this frees the LCB)
                //
                UDFRemovePrefix(IrpContext, Lcb);

                //
                // Decrement parent's references,
                // parent refs were incremented in UDFAcquirePrefix when LCB was created)
                //
                if (ParentFcb->FileInfo) {
                    UDFCloseFile__(IrpContext, Vcb, ParentFcb->FileInfo);
                }
                InterlockedDecrement((PLONG)&ParentFcb->FcbReference);

                UDFUnlockVcb(IrpContext, Vcb);
            }

            //
            // Now really leave if we have to.
            //
            if (Abort) {
                break;
            }

            /* Root/internal FCBs may have no parent LCB.  Their old final
             * flush also ran without a parent lock. */
            if (FinalFlushPending &&
                !NT_SUCCESS(UDFFlushFile__(IrpContext, Vcb,
                                           CurrentFcb->FileInfo, 0))) {
                break;
            }

            //
            // Now make the final check.
            // Lock ordering: FcbTableMutex (outer) before VcbMutex (inner).
            //
            UDFLockFcbTable(IrpContext, Vcb);
            UDFLockVcb(IrpContext, Vcb);

            if (CurrentFcb->FcbReference != 0) {
                //
                // Nope, nothing more to do.  Stop right now.
                //
                UDFUnlockVcb(IrpContext, Vcb);
                UDFUnlockFcbTable(IrpContext, Vcb);

                if (ParentFcb != NULL) {
                    UDFReleaseFcb(IrpContext, ParentFcb);
                }
                break;
            }

            //
            // This Fcb is toast.  Remove it from the Fcb Table as appropriate and delete.
            // Must happen under the same lock hold where FcbReference==0 was verified,
            // because create.cpp can increment FcbReference under VcbMutex without FCB exclusive.
            //
            if (FlagOn(CurrentFcb->FcbState, FCB_STATE_IN_FCB_TABLE)) {
                UDFDeleteFcbTable(IrpContext, CurrentFcb);
                ClearFlag(CurrentFcb->FcbState, FCB_STATE_IN_FCB_TABLE);
            }

            BOOLEAN ShouldDelete = !CurrentFcb->FcbCleanup;
            UDFUnlockVcb(IrpContext, Vcb);
            UDFUnlockFcbTable(IrpContext, Vcb);

            if (ShouldDelete && CurrentFcb->FileInfo) {

                // no more references... current file/dir MUST DIE!!!
                if (Delete) {
                    UDFReferenceFile__(CurrentFcb->FileInfo);
                    UDFUnlinkFile__(IrpContext, Vcb, CurrentFcb->FileInfo, TRUE);
                    UDFCloseFile__(IrpContext, Vcb, CurrentFcb->FileInfo);
                    CurrentFcb->FcbState |= UDF_FCB_DELETED;
                    Delete = FALSE;
                }
                else if (CurrentFcb->FcbState & UDF_FCB_DELETED) {
                    // File is already deleted - clear Modified flags without flushing to disk.
                    // The deletion was already written in cleanup.cpp via UDFUnlinkFile__.
                    // Any pending modifications are irrelevant for deleted files.
                    PUDF_FILE_INFO FileInfo = CurrentFcb->FileInfo;
                    PUDF_DATALOC_INFO Dloc = FileInfo->Dloc;
                    if (Dloc) {
                        Dloc->FE_Flags &= ~UDF_FE_FLAG_FE_MODIFIED;
                        Dloc->DataLoc.Modified = FALSE;
                        Dloc->DataLoc.Flags &= ~EXTENT_FLAG_PREALLOCATED;
                        Dloc->AllocLoc.Modified = FALSE;
                        Dloc->FELoc.Modified = FALSE;
                    }
                    // Also clear FI_Modified flag in parent's DirIndex
                    if (FileInfo->ParentFile && FileInfo->ParentFile->Dloc) {
                        PDIR_INDEX_ITEM DirNdx = UDFDirIndex(
                            FileInfo->ParentFile->Dloc->DirIndex,
                            FileInfo->Index);
                        if (DirNdx) {
                            DirNdx->FI_Flags &= ~UDF_FI_FLAG_FI_MODIFIED;
                        }
                    }
                }

                // check if we should try to delete Parent for the next time
                if (CurrentFcb->FcbState & UDF_FCB_DELETE_PARENT) {
                    Delete = TRUE;
                }

                // remove references to OS-specific structures
                // to let UDF_INFO release FI & Co
                CurrentFcb->FileInfo->Fcb = NULL;
                if (CurrentFcb->FileInfo->Dloc) {
                    CurrentFcb->FileInfo->Dloc->CommonFcb = NULL;
                }

                // FILEINFO and DLOC have separate lifetimes.  Cleanup can
                // detach this FileInfo while retaining a Dloc shared by other
                // links.  That still releases this FCB; restoring its already
                // detached FileInfo would leave a NULL Dloc for a later close.
                ULONG CleanupResult = UDFCleanUpFile__(Vcb, CurrentFcb->FileInfo);
                if (CleanupResult & UDF_FREE_FILEINFO) {

                    AdPrint(("UDFTeardownStructures, deleting Fcb %p\n", CurrentFcb));

                    // Release the exclusive FCB lock before deleting the FCB.
                    UDFReleaseFcb(IrpContext, CurrentFcb);

                    // Save FileInfo before freeing FCB (avoid use-after-free)
                    PUDF_FILE_INFO FileInfoToFree = CurrentFcb->FileInfo;
                    CurrentFcb->ParentFcb = NULL;
                    UDFDeleteFcb(IrpContext, CurrentFcb);
                    MyFreePool__(FileInfoToFree);

                    // Move to the parent Fcb.
                    CurrentFcb = ParentFcb;
                    ParentFcb = NULL;
                    AcquiredCurrentFcb = TRUE;

                } else {
                    // Stop cleaning up - restore pointers
                    CurrentFcb->FileInfo->Fcb = CurrentFcb;
                    if (CurrentFcb->FileInfo->Dloc) {
                        CurrentFcb->FileInfo->Dloc->CommonFcb = CurrentFcb;
                    }

                    UDFReleaseFcb(IrpContext, CurrentFcb);
                    CurrentFcb = ParentFcb;
                    ParentFcb = NULL;
                    AcquiredCurrentFcb = TRUE;
                }
            } else {
                // Cannot delete - release and move to parent
                if (CurrentFcb != StartingFcb || AcquiredCurrentFcb) {
                    UDFReleaseFcb(IrpContext, CurrentFcb);
                }
                CurrentFcb = ParentFcb;
                ParentFcb = NULL;
                AcquiredCurrentFcb = TRUE;
            }

        } while (CurrentFcb != NULL);

    } _SEH2_FINALLY {

        //
        // Release the current Fcb if we have acquired it.
        //
        if (AcquiredCurrentFcb && (CurrentFcb != NULL)) {
            UDFReleaseFcb(IrpContext, CurrentFcb);
        }

        //
        // Clear the teardown flag.
        //
        if (!Recursive) {
            ClearFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_IN_TEARDOWN);
        }

    } _SEH2_END;

    *RemovedStartingFcb = (CurrentFcb != StartingFcb);

    AdPrint(("UDFTeardownStructures, RemovedStartingFcb -> %c\n",
             *RemovedStartingFcb ? 'T' : 'F'));

} // end UDFTeardownStructures()

PFCB
UDFLookupFcbTable (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PVCB Vcb,
    _In_ FILE_ID FileId
    )

/*++

Routine Description:

    This routine will look through the Fcb table looking for a matching
    entry.

Arguments:

    Vcb - Vcb for this volume.

    FileId - This is the key value to use for the search.

Return Value:

    PFCB - A pointer to the matching entry or NULL otherwise.

--*/

{
    FCB_TABLE_ELEMENT Key;
    PFCB_TABLE_ELEMENT Hit;
    PFCB ReturnFcb = NULL;

    PAGED_CODE();

    Key.FileId = FileId;

    Hit = (PFCB_TABLE_ELEMENT)RtlLookupElementGenericTable(&Vcb->FcbTable, &Key);

    if (Hit != NULL) {

        ReturnFcb = Hit->Fcb;
    }

    return ReturnFcb;

    UNREFERENCED_PARAMETER( IrpContext );
}

PFCB
UDFGetNextFcb  (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PVCB Vcb,
    _In_ PVOID *RestartKey
    )

/*++

Routine Description:

    This routine will enumerate through all of the Fcb's in the Fcb table.

Arguments:

    Vcb - Vcb for this volume.

    RestartKey - This value is used by the table package to maintain
        its position in the enumeration.  It is initialized to NULL
        for the first search.

Return Value:

    PFCB - A pointer to the next fcb or NULL if the enumeration is
        completed

--*/

{
    PFCB Fcb;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( IrpContext );
    
    Fcb = (PFCB) RtlEnumerateGenericTableWithoutSplaying( &Vcb->FcbTable, RestartKey );

    if (Fcb != NULL) {

        Fcb = ((PFCB_TABLE_ELEMENT)(Fcb))->Fcb;
    }

    return Fcb;
}

/*************************************************************************
*
* Function: UDFCreateFcb()
*
* Description:
*   Allocate a new FCB structure to represent an open on-disk object.
*   Also initialize the FCB structure to NULL.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: A pointer to the FCB structure OR NULL.
*
*************************************************************************/

PFCB
UDFCreateFcb(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ FILE_ID FileId,
    _In_ NODE_TYPE_CODE NodeTypeCode,
    _Out_opt_ PBOOLEAN FcbExisted
    )

/*++

Routine Description:

    This routine is called to find the Fcb for the given FileId.  We will
    look this up first in the Fcb table and if not found we will create
    an Fcb.  We don't initialize it or insert it into the FcbTable in this
    routine.

    This routine is called while the Vcb is locked.

Arguments:

    FileId - This is the Id for the target Fcb.

    NodeTypeCode - Node type for this Fcb if we need to create.

    FcbExisted - If specified, we store whether the Fcb existed.

Return Value:

    PFCB - The Fcb found in the table or created if needed.

--*/

{
    PFCB NewFcb;
    BOOLEAN LocalFcbExisted;

    PAGED_CODE();

    _SEH2_TRY {

        // Use the local boolean if one was not passed in.

        if (!ARGUMENT_PRESENT(FcbExisted)) {

            FcbExisted = &LocalFcbExisted;
        }

        // Maybe this is already in the table.

        NewFcb = UDFLookupFcbTable(IrpContext, IrpContext->Vcb, FileId);

        // If not then create the Fcb is requested by our caller.

        if (NewFcb == NULL) {

            // Allocate and initialize the structure depending on the
            // type code.

            switch (NodeTypeCode) {

            case UDF_NODE_TYPE_INDEX:

                NewFcb = UDFAllocateFcbIndex();

                RtlZeroMemory(NewFcb, SIZEOF_FCB_INDEX);

                NewFcb->NodeIdentifier.NodeByteSize = SIZEOF_FCB_INDEX;

                break;

            case UDF_NODE_TYPE_DATA:

                NewFcb = UDFAllocateFcbData();

                RtlZeroMemory(NewFcb, SIZEOF_FCB_DATA);

                NewFcb->NodeIdentifier.NodeByteSize = SIZEOF_FCB_DATA;

                break;

            default:

#pragma prefast( suppress: __WARNING_USE_OTHER_FUNCTION, "This is a bug." )   
                UDFBugCheck(0, 0, 0);
            }

            // Now do the common initialization.

            NewFcb->NodeIdentifier.NodeTypeCode = NodeTypeCode;

            NewFcb->Vcb = IrpContext->Vcb;
            NewFcb->FileId = FileId;

            // A failed root-directory open can delete this FCB before
            // UDFInitializeFCB.  Its teardown requires valid empty queues.
            InitializeListHead(&NewFcb->ParentLcbQueue);
            InitializeListHead(&NewFcb->ChildLcbQueue);

            //  Now create the non-paged section object.

            NewFcb->FcbNonpaged = UDFCreateFcbNonpaged(IrpContext);

            *FcbExisted = FALSE;

        } else {

            *FcbExisted = TRUE;
        }

    } _SEH2_FINALLY{

        if (_SEH2_AbnormalTermination()) {

            if (NewFcb && NewFcb->FcbNonpaged) {

                UDFDeleteFcbNonpaged(IrpContext, NewFcb->FcbNonpaged);
            }

            UDFFreePool((PVOID*)&NewFcb);
        }

    } _SEH2_END;

    return NewFcb;
}

/*************************************************************************
*
* Function: UDFInitializeFCB()
*
* Description:
*   Initialize a new FCB structure and also the sent-in file object
*   (if supplied)
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
NTSTATUS
UDFInitializeFCB(
    IN PFCB             Fcb,            // FCB structure to be initialized
    IN PVCB             Vcb,            // logical volume (VCB) pointer
    IN ULONG            Flags)          // is this a file/directory, etc.
{
    ASSERT_LOCKED_VCB(Vcb);

    // Fill NT required Fcb part

    ASSERT(!Fcb->Header.Resource);
    Fcb->Header.Resource = &Fcb->FcbNonpaged->FcbResource;
    Fcb->Header.PagingIoResource = &Fcb->FcbNonpaged->FcbPagingIoResource;
    InitializeListHead(&Fcb->EofListHead);
    FsRtlSetupAdvancedHeader(&Fcb->Header, &Fcb->FcbNonpaged->AdvancedFcbHeaderMutex);
    Fcb->FileLock = NULL;

    Fcb->FcbState = Flags;

    // NOTE: FCB is NOT added to FcbTable here.
    // Caller is responsible for calling UDFInsertFcbIntoTable after
    // FCB is fully initialized.

    // initialize the various list heads
    InitializeListHead(&Fcb->ParentLcbQueue);
    InitializeListHead(&Fcb->ChildLcbQueue);

    // Splay tree roots for fast child lookup by name
    Fcb->ExactCaseRoot = NULL;
    Fcb->IgnoreCaseRoot = NULL;
    Fcb->ShortNameRoot = NULL;

    Fcb->FcbReference = 0;
    Fcb->FcbCleanup = 0;

    // Initialize file name cache synchronization
    Fcb->FcbLockThread = NULL;
    Fcb->FcbLockCount = 0;

    Fcb->Vcb = Vcb;

    return STATUS_SUCCESS;
} // end UDFInitializeFCB()

RTL_GENERIC_COMPARE_RESULTS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
UDFFcbTableCompare (
    _In_ PRTL_GENERIC_TABLE FcbTable,
    _In_ PVOID Fid1,
    _In_ PVOID Fid2
    )

/*++

Routine Description:

    This routine is the Cdfs compare routine called by the generic table package.
    If will compare the two File Id values and return a comparison result.

Arguments:

    FcbTable - This is the table being searched.

    Fid1 - First key value.

    Fid2 - Second key value.

Return Value:

    RTL_GENERIC_COMPARE_RESULTS - The results of comparing the two
        input structures

--*/

{
    FILE_ID Id1, Id2;
    PAGED_CODE();

    Id1 = *((FILE_ID UNALIGNED *) Fid1);
    Id2 = *((FILE_ID UNALIGNED *) Fid2);

    if (Id1.QuadPart < Id2.QuadPart) {

        return GenericLessThan;

    } else if (Id1.QuadPart > Id2.QuadPart) {

        return GenericGreaterThan;

    } else {

        return GenericEqual;
    }

    UNREFERENCED_PARAMETER( FcbTable );
}

PVOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
UDFAllocateFcbTable (
    _In_ PRTL_GENERIC_TABLE FcbTable,
    _In_ CLONG ByteSize
    )

/*++

Routine Description:

    This is a generic table support routine to allocate memory

Arguments:

    FcbTable - Supplies the generic table being used

    ByteSize - Supplies the number of bytes to allocate

Return Value:

    PVOID - Returns a pointer to the allocated data

--*/

{
    PAGED_CODE();
    
    UNREFERENCED_PARAMETER(FcbTable);

    return FsRtlAllocatePoolWithTag(PagedPool, ByteSize, TAG_FCB_TABLE);
}

VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
UDFDeallocateFcbTable (
    _In_ PRTL_GENERIC_TABLE FcbTable,
    _In_ __drv_freesMem(Mem) _Post_invalid_ PVOID Buffer
    )
/*++

Routine Description:

    This is a generic table support routine that deallocates memory

Arguments:

    FcbTable - Supplies the generic table being used

    Buffer - Supplies the buffer being deallocated

Return Value:

    None.

--*/

{
    PAGED_CODE();

    UDFFreePool(&Buffer);

    UNREFERENCED_PARAMETER( FcbTable );
}

/*************************************************************************
*
* Function: UDFInitializeVCB()
*
* Description:
*   Perform the initialization for a VCB structure.
*
* Expected Interrupt Level (for execution) :
*
*   IRQL PASSIVE_LEVEL
*
* Return Value: status
*
*************************************************************************/
VOID
UDFInitializeVCB(
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PVCB Vcb,
    _In_ PDEVICE_OBJECT TargetDeviceObject,
    _In_ PVPB Vpb,
    _In_ PDISK_GEOMETRY DiskGeometry,
    _In_ ULONG MediaChangeCount
    )
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(IrpContext);

    // We start by first zeroing out all of the VCB, this will guarantee
    // that any stale data is wiped clean.

    RtlZeroMemory(Vcb, sizeof(struct VCB));

    // Set the proper node type code and node byte size.

    Vcb->NodeIdentifier.NodeTypeCode = UDF_NODE_TYPE_VCB;
    Vcb->NodeIdentifier.NodeByteSize = sizeof(struct VCB);

    // Initialize the notify sync mutex. FsRtlNotifyInitializeSync can raise.

    FsRtlNotifyInitializeSync(&Vcb->NotifySync);

    _SEH2_TRY {

        ExInitializeResourceLite(&Vcb->VcbResource);
        ExInitializeResourceLite(&Vcb->BitMapResource1);
        ExInitializeResourceLite(&Vcb->MetadataGrowResource);

        ExInitializeResourceLite(&Vcb->DlocResource);
        ExInitializeResourceLite(&Vcb->DlocResource2);
        ExInitializeResourceLite(&Vcb->FlushResource);
        ExInitializeResourceLite(&Vcb->PreallocResource);
        ExInitializeFastMutex(&Vcb->VcbMutex);
        ExInitializeFastMutex(&Vcb->FcbTableMutex);

        // Initialize the generic Fcb Table.

        RtlInitializeGenericTable(&Vcb->FcbTable,
                                  (PRTL_GENERIC_COMPARE_ROUTINE)UDFFcbTableCompare,
                                  (PRTL_GENERIC_ALLOCATE_ROUTINE)UDFAllocateFcbTable,
                                  (PRTL_GENERIC_FREE_ROUTINE)UDFDeallocateFcbTable,
                                  NULL);

        // Pick up a VPB right now so we know we can pull this filesystem stack
        // off of the storage stack on demand.  This can raise - if it does,  
        // uninitialize the notify structures before returning.

        Vcb->SwapVpb = (PVPB)FsRtlAllocatePoolWithTag(NonPagedPoolNx, sizeof( VPB), TAG_VPB);

        RtlZeroMemory(Vcb->SwapVpb, sizeof(VPB));

        // We know the target device object.
        // Note that this is not neccessarily a pointer to the actual
        // physical/virtual device on which the logical volume should
        // be mounted. This is actually a pointer to either the actual
        // (real) device or to any device object that may have been
        // attached to it. Any IRPs that we send down should be sent to this
        // device object. However, the "real" physical/virtual device object
        // on which we perform our mount operation can be determined from the
        // RealDevice field in the VPB sent to us.
        Vcb->TargetDeviceObject = TargetDeviceObject;

        // We also have the VPB pointer. This was obtained from the
        // Parameters.MountVolume.Vpb field in the current I/O stack location
        // for the mount IRP.
        Vcb->Vpb = Vpb;

        //  Set the removable media flag based on the real device's
        //  characteristics
        if (Vpb->RealDevice->Characteristics & FILE_REMOVABLE_MEDIA) {

            Vcb->VcbState |= VCB_STATE_REMOVABLE_MEDIA;
        }

        // Initialize the list anchor (head) for some lists in this VCB.
        InitializeListHead(&Vcb->NextNotifyIRP);

        // Intilize FCB for this VCB

        // Refererence the Vcb for two reasons.  The first is a reference
        // that prevents the Vcb from going away on the last close unless
        // dismount has already occurred.  The second is to make sure
        // we don't go into the dismount path on any error during mount
        // until we get to the Mount cleanup.

        Vcb->VcbResidualReference = UDFS_BASE_RESIDUAL_REFERENCE;
        Vcb->VcbResidualUserReference = UDFS_BASE_RESIDUAL_USER_REFERENCE;

        Vcb->VcbReference = 1 + Vcb->VcbResidualReference;

        // Create a stream file object for this volume.
        //Vcb->PtrStreamFileObject = IoCreateStreamFileObject(NULL,
        //                                            Vcb->Vpb->RealDevice);
        //ASSERT(Vcb->PtrStreamFileObject);

        // Initialize some important fields in the newly created file object.
        //Vcb->PtrStreamFileObject->FsContext = (PVOID)Vcb;
        //Vcb->PtrStreamFileObject->FsContext2 = NULL;
        //Vcb->PtrStreamFileObject->SectionObjectPointer = &(Vcb->SectionObject);

        //Vcb->PtrStreamFileObject->Vpb = PtrVPB;

        // Insert this Vcb record on the CdData.VcbQueue.

        ASSERT_EXCLUSIVE_CDDATA;
        InsertTailList(&(UdfData.VcbQueue), &(Vcb->VcbLinks));

        // Initialize caching for the stream file object.
        //CcInitializeCacheMap(Vcb->PtrStreamFileObject, (PCC_FILE_SIZES)(&(Vcb->AllocationSize)),
        //                            TRUE,       // We will use pinned access.
        //                            &(UDFGlobalData.CacheMgrCallBacks), Vcb);

        Vcb->SectorSize = DiskGeometry->BytesPerSector;
        Vcb->SectorShift = UDFHighBit(DiskGeometry->BytesPerSector);
        Vcb->MediaChangeCount = MediaChangeCount;

    } _SEH2_FINALLY {

        if (_SEH2_AbnormalTermination()) {

            FsRtlNotifyUninitializeSync(&Vcb->NotifySync);
        }
    } _SEH2_END;
} // end UDFInitializeVCB()

NTSTATUS
UDFCompleteMount(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )
{
    NTSTATUS Status;
    UNICODE_STRING LocalPath;
    ULONG LastSector = 0;
    BOOLEAN UnlockVcb = FALSE;
    FILE_ID FileId;
    RtlZeroMemory(&FileId, sizeof(FILE_ID));

    PAGED_CODE();

    UDFPrint(("UDFCompleteMount:\n"));

    // Use a try-finally to facilitate cleanup.

    _SEH2_TRY {

        Vcb->ZBuffer = (PCHAR)DbgAllocatePoolWithTag(NonPagedPool, max(Vcb->SectorSize, PAGE_SIZE), 'zNWD');

        if (!Vcb->ZBuffer) {

            try_return(Status = STATUS_INSUFFICIENT_RESOURCES);
        }

        RtlZeroMemory(Vcb->ZBuffer, Vcb->SectorSize);

        // Create the root index and reference it in the Vcb.

        Vcb->RootIndexFcb = UDFCreateFcb(IrpContext, FileId, UDF_NODE_TYPE_INDEX, NULL);

        if (!Vcb->RootIndexFcb) {

            try_return(Status = STATUS_INSUFFICIENT_RESOURCES);
        }

        //  Create the File id by hand for this Fcb.

        Vcb->RootIndexFcb->FileId = UdfGetFidFromLbAddr(Vcb->RootLbAddr);
        SetFlag(Vcb->RootIndexFcb->FileId.HighPart, FID_DIR_MASK);

        Vcb->RootIndexFcb->FileInfo = (PUDF_FILE_INFO)MyAllocatePool__(NonPagedPool,sizeof(UDF_FILE_INFO));

        if (!Vcb->RootIndexFcb->FileInfo) {

            UDFDeleteFcb(IrpContext, Vcb->RootIndexFcb);
            Vcb->RootIndexFcb = NULL;
            try_return(Status = STATUS_INSUFFICIENT_RESOURCES);
        }

        UDFPrint(("UDFCompleteMount: open Root Dir\n"));
        // Open Root Directory
        Status = UDFOpenRootFile__(IrpContext, Vcb, &Vcb->RootLbAddr, Vcb->RootIndexFcb->FileInfo);

        if (!NT_SUCCESS(Status)) {

            UDFCleanUpFile__(Vcb, Vcb->RootIndexFcb->FileInfo);
            MyFreePool__(Vcb->RootIndexFcb->FileInfo);
            UDFDeleteFcb(IrpContext, Vcb->RootIndexFcb);
            Vcb->RootIndexFcb = NULL;
            try_return(Status);
        }

        Vcb->RootIndexFcb->FileInfo->Fcb = Vcb->RootIndexFcb;

        if (!Vcb->RootIndexFcb->FileInfo->Dloc->CommonFcb) {
            Vcb->RootIndexFcb->FileInfo->Dloc->CommonFcb = Vcb->RootIndexFcb;
        }

        UDFLockVcb(IrpContext, Vcb);
        UnlockVcb = TRUE;

        Status = UDFInitializeFCB(Vcb->RootIndexFcb, Vcb, UDF_FCB_ROOT_DIRECTORY | UDF_FCB_DIRECTORY);

        if (!NT_SUCCESS(Status)) {

            // if we get here, no resources are inited
            Vcb->RootIndexFcb->FcbCleanup = 0;
            Vcb->RootIndexFcb->FcbReference = 0;

            UDFCleanUpFile__(Vcb, Vcb->RootIndexFcb->FileInfo);
            MyFreePool__(Vcb->RootIndexFcb->FileInfo);

            // FCB was not inserted into table (UDFInitializeFCB failed
            // before UDFInsertFcbIntoTable was called)
            UDFUnlockVcb(IrpContext, Vcb);
            UnlockVcb = FALSE;

            UDFDeleteFcb(IrpContext, Vcb->RootIndexFcb);
            Vcb->RootIndexFcb = NULL;
            try_return(Status);
        }

        // Insert into FcbTable after successful initialization
        UDFInsertFcbIntoTable(IrpContext, Vcb->RootIndexFcb);

        // this is a part of UDF_RESIDUAL_REFERENCE
        InterlockedIncrement((PLONG)&Vcb->VcbReference);
        Vcb->RootIndexFcb->FcbCleanup = 1;
        Vcb->RootIndexFcb->FcbReference = 1;

        UDFGetFileXTime(Vcb->RootIndexFcb->FileInfo,
                      &(Vcb->RootIndexFcb->CreationTime.QuadPart),
                      &(Vcb->RootIndexFcb->LastAccessTime.QuadPart),
                      &(Vcb->RootIndexFcb->ChangeTime.QuadPart),
                      &(Vcb->RootIndexFcb->LastWriteTime.QuadPart) );

        if (Vcb->SysStreamLbAddr.logicalBlockNum) {
            Vcb->SysSDirFileInfo = (PUDF_FILE_INFO)MyAllocatePool__(NonPagedPool,sizeof(UDF_FILE_INFO));
            if (!Vcb->SysSDirFileInfo) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto unwind_1;
            }
            // Open System SDir Directory
            Status = UDFOpenRootFile__(IrpContext, Vcb, &Vcb->SysStreamLbAddr, Vcb->SysSDirFileInfo);
            if (!NT_SUCCESS(Status)) {
                UDFCleanUpFile__(Vcb, Vcb->SysSDirFileInfo);
                MyFreePool__(Vcb->SysSDirFileInfo);
                Vcb->SysSDirFileInfo = NULL;
                goto unwind_1;
            } else {
                Vcb->SysSDirFileInfo->Dloc->DataLoc.Flags |= EXTENT_FLAG_VERIFY;
            }
        }

        // Open Unallocatable space stream
        // Generally, it should be placed in SystemStreamDirectory, but some
        // stupid apps think that RootDirectory is much better place.... :((
        RtlInitUnicodeString(&LocalPath, UDF_FN_NON_ALLOCATABLE);
        Status = UDFOpenFile__(IrpContext, Vcb, FALSE, TRUE, &LocalPath, Vcb->RootIndexFcb->FileInfo, &Vcb->NonAllocFileInfo, NULL);

        if (!NT_SUCCESS(Status) && (Status != STATUS_OBJECT_NAME_NOT_FOUND)) {

    //unwind_2:
            UDFCleanUpFile__(Vcb, Vcb->NonAllocFileInfo);
            Vcb->NonAllocFileInfo = NULL;
            // this was a part of UDF_RESIDUAL_REFERENCE
            InterlockedDecrement((PLONG)&Vcb->VcbReference);
    unwind_1:

            // UDFCloseResidual() will clean up everything

            try_return(Status);
        }

        /* process Non-allocatable */
        if (NT_SUCCESS(Status)) {
            UDFMarkSpaceAsXXX(Vcb, Vcb->NonAllocFileInfo->Dloc, Vcb->NonAllocFileInfo->Dloc->DataLoc.Mapping, AS_USED); // used
            UDFDirIndex(UDFGetDirIndexByFileInfo(Vcb->NonAllocFileInfo), Vcb->NonAllocFileInfo->Index)->FI_Flags |= UDF_FI_FLAG_FI_INTERNAL;
        } else {
            /* try to read Non-allocatable from alternate locations */
            RtlInitUnicodeString(&LocalPath, UDF_FN_NON_ALLOCATABLE_2);
            Status = UDFOpenFile__(IrpContext, Vcb, FALSE, TRUE, &LocalPath, Vcb->RootIndexFcb->FileInfo, &(Vcb->NonAllocFileInfo), NULL);
            if (!NT_SUCCESS(Status) && (Status != STATUS_OBJECT_NAME_NOT_FOUND)) {
                goto unwind_1;
            }
            if (NT_SUCCESS(Status)) {
                UDFMarkSpaceAsXXX(Vcb, Vcb->NonAllocFileInfo->Dloc, Vcb->NonAllocFileInfo->Dloc->DataLoc.Mapping, AS_USED); // used
                UDFDirIndex(UDFGetDirIndexByFileInfo(Vcb->NonAllocFileInfo), Vcb->NonAllocFileInfo->Index)->FI_Flags |= UDF_FI_FLAG_FI_INTERNAL;
            } else
            if (Vcb->SysSDirFileInfo) {
                RtlInitUnicodeString(&LocalPath, UDF_SN_NON_ALLOCATABLE);
                Status = UDFOpenFile__(IrpContext, Vcb, FALSE, TRUE, &LocalPath, Vcb->SysSDirFileInfo , &(Vcb->NonAllocFileInfo), NULL);
                if (!NT_SUCCESS(Status) && (Status != STATUS_OBJECT_NAME_NOT_FOUND)) {
                    goto unwind_1;
                }
                if (NT_SUCCESS(Status)) {
                    UDFMarkSpaceAsXXX(Vcb, Vcb->NonAllocFileInfo->Dloc, Vcb->NonAllocFileInfo->Dloc->DataLoc.Mapping, AS_USED); // used
    //                    UDFDirIndex(UDFGetDirIndexByFileInfo(Vcb->NonAllocFileInfo), Vcb->NonAllocFileInfo->Index)->FI_Flags |= UDF_FI_FLAG_FI_INTERNAL;
                } else {
                    Status = STATUS_SUCCESS;
                }
            } else {
                Status = STATUS_SUCCESS;
            }
        }

        /* Read SN UID mapping */
        if (Vcb->SysSDirFileInfo) {

            RtlInitUnicodeString(&LocalPath, UDF_SN_UID_MAPPING);

            Status = UDFOpenFile__(IrpContext, Vcb, FALSE, TRUE, &LocalPath, Vcb->SysSDirFileInfo , &Vcb->UniqueIDMapFileInfo, NULL);

            if (NT_SUCCESS(Status)) {

            } else if  (Status == STATUS_OBJECT_NAME_NOT_FOUND) {

                Vcb->UniqueIDMapFileInfo = NULL;

            } else {

                goto unwind_1;
            }
            Status = STATUS_SUCCESS;
        }

        Status = STATUS_SUCCESS;

        // clear Modified flags. It was not real modify, just
        // bitmap construction
        Vcb->BitmapModified = FALSE;
        //Vcb->Modified = FALSE;
        UDFPreClrModified(Vcb);
        UDFClrModified(Vcb);
        // this is a part of UDF_RESIDUAL_REFERENCE
        InterlockedIncrement((PLONG)&Vcb->VcbReference);

        // Start initializing the fields contained in the Header.

        // DisAllow fast-IO for now.
    //    RootFcb->Header->IsFastIoPossible = FastIoIsNotPossible;
        Vcb->RootIndexFcb->Header.IsFastIoPossible = FastIoIsPossible;

        // Initialize the MainResource and PagingIoResource pointers in
        // the CommonFCBHeader structure to point to the ERESOURCE structures we
        // have allocated and already initialized above.
    //    RootFcb->Header.Resource = &RootFcb->MainResource;
    //    RootFcb->Header.PagingIoResource = &RootFcb->PagingIoResource;

        // Initialize the file size values here.
        Vcb->RootIndexFcb->Header.AllocationSize.QuadPart = 0;
        Vcb->RootIndexFcb->Header.FileSize.QuadPart = 0;

        // The following will disable ValidDataLength support.
    //    RootFcb->Header.ValidDataLength.QuadPart = 0x7FFFFFFFFFFFFFFFI64;
        Vcb->RootIndexFcb->Header.ValidDataLength.QuadPart = 0;

        if (!NT_SUCCESS(Status))
            try_return(Status);

        ASSERT(!Vcb->Modified);

        UDFUnlockVcb(IrpContext, Vcb);
        UnlockVcb = FALSE;

        //  Now do the volume dasd Fcb.  Create this and reference it in the Vcb.

        UDFLockVcb(IrpContext, Vcb);
        UnlockVcb = TRUE;

        Vcb->VolumeDasdFcb = UDFCreateFcb(IrpContext, FileId, UDF_NODE_TYPE_DATA, NULL);

        UDFIncrementReferenceCounts(IrpContext, Vcb->VolumeDasdFcb, 1, 1);
        UDFUnlockVcb(IrpContext, Vcb);
        UnlockVcb = FALSE;

        // Iterate through all partitions in the Pcb structure to find the highest sector number (LastSector)
        // occupied by any physical partition. If the end of the current partition exceeds the current LastSector,
        // update LastSector. This determines the upper boundary of space used on the device.

        for (USHORT RefPartNum = 0; RefPartNum < Vcb->PartitionMaps; RefPartNum++) {

            if ((Vcb->Partitions[RefPartNum].PartitionType == UDF_TYPE1_MAP15 ||
                Vcb->Partitions[RefPartNum].PartitionType == UDF_SPARABLE_MAP15) &&
                Vcb->Partitions[RefPartNum].PartitionRoot +
                Vcb->Partitions[RefPartNum].PartitionLen > LastSector) {

                LastSector = Vcb->Partitions[RefPartNum].PartitionRoot +
                             Vcb->Partitions[RefPartNum].PartitionLen;
            }
        }

        if ((Vcb->SessionEndLba + 1 > LastSector) 
            && !Vcb->CDR_Mode
        ) {
            LastSector = Vcb->SessionEndLba + 1;
        }

        Vcb->VolumeDasdFcb->Header.FileSize.QuadPart = LlBytesFromSectors(Vcb, LastSector);

        Vcb->VolumeDasdFcb->Header.AllocationSize.QuadPart =
        Vcb->VolumeDasdFcb->Header.ValidDataLength.QuadPart = Vcb->VolumeDasdFcb->Header.FileSize.QuadPart;

        // Point to the resource.

        Vcb->VolumeDasdFcb->Header.Resource = &Vcb->VolumeDasdFcb->FcbNonpaged->FcbResource;
        Vcb->VolumeDasdFcb->Header.PagingIoResource = &Vcb->VolumeDasdFcb->FcbNonpaged->FcbPagingIoResource;

        // TODO: use VolumeDasdFcb ?????

        FsRtlSetupAdvancedHeader(&Vcb->VolumeDasdFcb->Header, &Vcb->VolumeDasdFcb->FcbNonpaged->AdvancedFcbHeaderMutex);

        // Mark the Fcb as initialized.

        SetFlag(Vcb->VolumeDasdFcb->FcbState, FCB_STATE_INITIALIZED);

    try_exit:  NOTHING;
    } _SEH2_FINALLY {

        if (_SEH2_AbnormalTermination()) {

            UDFFreePool((PVOID*)&Vcb->ZBuffer);

            // Vcb->VolumeDasdFcb
        }

        if (UnlockVcb) {
            UDFUnlockVcb(IrpContext, Vcb);
        }
    } _SEH2_END;

    return Status;
} // end UDFCompleteMount()

/*************************************************************************
*
* Function: UDFDeallocateCcb()
*
* Description:
*   Deallocate a previously allocated structure.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
VOID
UDFDeallocateCcb(
    PCCB Ccb
    )
{
    ASSERT_CCB(Ccb);

    ExFreeToPagedLookasideList(&UdfData.CcbLookasideList, Ccb);

} // end UDFDeallocateCcb()

/*
  Function: UDFDeleteCcb()

  Description:
    Cleanup and deallocate a previously allocated structure.

  Expected Interrupt Level (for execution) :

   IRQL_PASSIVE_LEVEL

  Return Value: None

*/
VOID
UDFDeleteCcb(
    PCCB Ccb
)
{
    if (Ccb->SearchExpression.Buffer != NULL) {

        UDFFreePool((PVOID*)&Ccb->SearchExpression.Buffer);
    }

    UDFDeallocateCcb(Ccb);
} // end UDFDeleteCcb()

/*
  Function: UDFCreateBitmapStream()

  Description:
    Creates an internal stream FCB and FileObject for the free space bitmap.
    This allows bitmap data to be cached via CcPinRead/CcSetDirtyPinnedData.
    The MCB maps stream offsets to physical disk sectors where the bitmap resides.

  Arguments:
    Vcb         - Volume control block
    BitmapPsn   - Physical sector number where bitmap starts on disk
    BitmapLength - Total length of bitmap data on disk in bytes
                   (including SPACE_BITMAP_DESC header)

  Return Value:
    NTSTATUS
*/
NTSTATUS
UDFCreateBitmapStream(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG BitmapPsn,
    IN ULONG BitmapLength
    )
{
    NTSTATUS Status;
    PFCB Fcb;
    PFILE_OBJECT FileObject;
    PFCB_NONPAGED FcbNonpaged;
    ULONG BitmapSectors;
    LARGE_INTEGER FileSize;

    PAGED_CODE();
    UNREFERENCED_PARAMETER(IrpContext);

    UDFPrint(("UDFCreateBitmapStream: PSN=%x, Length=%x\n", BitmapPsn, BitmapLength));

    // Allocate the FCB

    Fcb = UDFAllocateFcb();
    if (!Fcb) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(Fcb, sizeof(struct FCB));

    // Initialize nonpaged data (inline in VCB to avoid extra allocation)

    FcbNonpaged = &Vcb->BitmapNonpaged;
    RtlZeroMemory(FcbNonpaged, sizeof(struct FCB_NONPAGED));

    FcbNonpaged->NodeTypeCode = UDF_NODE_TYPE_FCB_NONPAGED;
    FcbNonpaged->NodeByteSize = sizeof(struct FCB_NONPAGED);

    ExInitializeResourceLite(&FcbNonpaged->FcbResource);
    ExInitializeResourceLite(&FcbNonpaged->FcbPagingIoResource);
    ExInitializeFastMutex(&FcbNonpaged->FcbMutex);
    ExInitializeFastMutex(&FcbNonpaged->AdvancedFcbHeaderMutex);
    ExInitializeFastMutex(&FcbNonpaged->FcbFastMutex);

    // Initialize the FCB header

    Fcb->NodeIdentifier.NodeTypeCode = UDF_NODE_TYPE_DATA;
    Fcb->NodeIdentifier.NodeByteSize = sizeof(struct FCB);
    Fcb->FcbNonpaged = FcbNonpaged;
    Fcb->Vcb = Vcb;

    Fcb->Header.Resource = &FcbNonpaged->FcbResource;
    Fcb->Header.PagingIoResource = &FcbNonpaged->FcbPagingIoResource;
    Fcb->Header.IsFastIoPossible = FastIoIsNotPossible;

    InitializeListHead(&Fcb->EofListHead);
    FsRtlSetupAdvancedHeader(&Fcb->Header, &FcbNonpaged->AdvancedFcbHeaderMutex);

    InitializeListHead(&Fcb->ParentLcbQueue);
    InitializeListHead(&Fcb->ChildLcbQueue);

    // Set file sizes: bitmap data on disk (with header)

    BitmapSectors = (BitmapLength + Vcb->SectorSize - 1) >> Vcb->SectorShift;
    FileSize.QuadPart = (LONGLONG)BitmapSectors << Vcb->SectorShift;

    Fcb->Header.AllocationSize.QuadPart = FileSize.QuadPart;
    Fcb->Header.FileSize.QuadPart = BitmapLength;
    Fcb->Header.ValidDataLength.QuadPart = BitmapLength;

    // Initialize the MCB: maps VBN (stream sectors) -> PSN (disk sectors)

    FsRtlInitializeLargeMcb(&Vcb->BitmapMcb, PagedPool);

    _SEH2_TRY {
        FsRtlAddLargeMcbEntry(&Vcb->BitmapMcb,
                              0,                    // Vbn: offset 0 in stream
                              (LONGLONG)BitmapPsn,  // Lbn: physical sector on disk
                              (LONGLONG)BitmapSectors);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        Status = _SEH2_GetExceptionCode();
        FsRtlUninitializeLargeMcb(&Vcb->BitmapMcb);
        ExDeleteResourceLite(&FcbNonpaged->FcbResource);
        ExDeleteResourceLite(&FcbNonpaged->FcbPagingIoResource);
        ExFreePoolWithTag(Fcb, TAG_FCB);
        return Status;
    } _SEH2_END;

    // Create the internal stream FileObject

    FileObject = IoCreateStreamFileObjectLite(NULL, Vcb->Vpb->RealDevice);
    if (!FileObject) {
        FsRtlUninitializeLargeMcb(&Vcb->BitmapMcb);
        ExDeleteResourceLite(&FcbNonpaged->FcbResource);
        ExDeleteResourceLite(&FcbNonpaged->FcbPagingIoResource);
        ExFreePoolWithTag(Fcb, TAG_FCB);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    FileObject->ReadAccess = TRUE;
    FileObject->WriteAccess = FALSE;
    FileObject->DeleteAccess = FALSE;

    FileObject->FsContext = Fcb;
    FileObject->FsContext2 = (PVOID)(ULONG_PTR)StreamFileOpen;
    FileObject->SectionObjectPointer = &FcbNonpaged->SegmentObject;
    FileObject->Vpb = Vcb->Vpb;

    // Initialize Cache Manager with pin access

    CcInitializeCacheMap(FileObject,
                         (PCC_FILE_SIZES)&Fcb->Header.AllocationSize,
                         TRUE,                           // PinAccess
                         &UdfData.CacheMgrCallBacks,
                         Fcb);

    /* Physical allocation bits can describe FE/FID owners that are still
     * waiting for the volume checkpoint. Keep dirty bitmap BCBs out of the
     * lazy writer; UDFUpdatePartDesc explicitly flushes them after
     * UDFUmount__ has made the namespace durable. */
    CcSetAdditionalCacheAttributes(FileObject, FALSE, TRUE);

    // Store in VCB

    Vcb->BitmapFcb = Fcb;
    Vcb->BitmapStreamFileObject = FileObject;
    Vcb->BitmapBcb = NULL;

    UDFPrint(("UDFCreateBitmapStream: OK, Sectors=%x, FileSize=%I64x\n",
        BitmapSectors, FileSize.QuadPart));

    return STATUS_SUCCESS;
} // end UDFCreateBitmapStream()

/*
  Function: UDFDeleteBitmapStream()

  Description:
    Tears down the bitmap cache stream created by UDFCreateBitmapStream.
    Purges cache, uninitializes cache map, and releases all resources.

  Arguments:
    Vcb - Volume control block

  Return Value:
    None
*/
VOID
UDFDeleteBitmapStream(
    IN PVCB Vcb
    )
{
    PAGED_CODE();

    if (!Vcb->BitmapFcb) {
        return;
    }

    UDFPrint(("UDFDeleteBitmapStream\n"));

    // Purge cache and uninitialize cache map

    CcPurgeCacheSection(&Vcb->BitmapNonpaged.SegmentObject, NULL, 0, FALSE);
    CcUninitializeCacheMap(Vcb->BitmapStreamFileObject, NULL, NULL);

    // Dereference the FileObject

    ObDereferenceObject(Vcb->BitmapStreamFileObject);
    Vcb->BitmapStreamFileObject = NULL;

    // Cleanup MCB

    FsRtlUninitializeLargeMcb(&Vcb->BitmapMcb);

    // Cleanup resources (nonpaged is inline in VCB, don't free struct)

    ExDeleteResourceLite(&Vcb->BitmapNonpaged.FcbResource);
    ExDeleteResourceLite(&Vcb->BitmapNonpaged.FcbPagingIoResource);

    // Free the FCB

    ExFreePoolWithTag(Vcb->BitmapFcb, TAG_FCB);
    Vcb->BitmapFcb = NULL;

} // end UDFDeleteBitmapStream()
