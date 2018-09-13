/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    PrefxSup.c

Abstract:

    This module implements the Udfs Prefix support routines

Author:

    Dan Lovinger    [DanLo]     8-Oct-1996

Revision History:

--*/

#include "UdfProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (UDFS_BUG_CHECK_PREFXSUP)

//
//  The local debug trace level
//

#define Dbg                              (UDFS_DEBUG_LEVEL_READ)

//
//  Local support routines.
//

PLCB
UdfFindNameLink (
    IN PIRP_CONTEXT IrpContext,
    IN PRTL_SPLAY_LINKS *RootNode,
    IN PUNICODE_STRING Name
    );

BOOLEAN
UdfInsertNameLink (
    IN PIRP_CONTEXT IrpContext,
    IN PRTL_SPLAY_LINKS *RootNode,
    IN PLCB NameLink
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, UdfFindNameLink)
#pragma alloc_text(PAGE, UdfFindPrefix)
#pragma alloc_text(PAGE, UdfInitializeLcbFromDirContext)
#pragma alloc_text(PAGE, UdfInsertNameLink)
#pragma alloc_text(PAGE, UdfInsertPrefix)
#pragma alloc_text(PAGE, UdfRemovePrefix)
#endif


PLCB
UdfInsertPrefix (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PUNICODE_STRING Name,
    IN BOOLEAN ShortNameMatch,
    IN BOOLEAN IgnoreCase,
    IN PFCB ParentFcb
    )

/*++

Routine Description:

    This routine inserts an Lcb linking the two Fcbs together.

Arguments:

    Fcb - This is the Fcb whose name is being inserted into the tree.

    Name - This is the name for the component.
    
    ShortNameMatch - Indicates whether this name was found on an explicit 8.3 search

    IgnoreCase - Indicates if we should insert into the case-insensitive tree.

    ParentFcb - This is the ParentFcb.  The prefix tree is attached to this.

Return Value:

    PLCB - the Lcb inserted.

--*/

{
    PLCB Lcb;
    PRTL_SPLAY_LINKS *TreeRoot;
    PLIST_ENTRY ListLinks;
    ULONG Flags;

    PWCHAR NameBuffer;

    PAGED_CODE();

    //
    //  Check inputs.
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    ASSERT_EXCLUSIVE_FCB( Fcb );
    ASSERT_EXCLUSIVE_FCB( ParentFcb );
    ASSERT_FCB_INDEX( ParentFcb );

    //
    //  It must be the case that an index Fcb is only referenced by a single index.  Now
    //  we walk the child's Lcb queue to insure that if any prefixes have already been
    //  inserted, they all refer to the index Fcb we are linking to.  This is the only way
    //  we can detect directory cross-linkage.
    //

    if (SafeNodeType( Fcb ) == UDFS_NTC_FCB_INDEX) {

        for (ListLinks = Fcb->ParentLcbQueue.Flink;
             ListLinks != &Fcb->ParentLcbQueue;
             ListLinks = ListLinks->Flink) {

            Lcb = CONTAINING_RECORD( ListLinks, LCB, ChildFcbLinks );

            if (Lcb->ParentFcb != ParentFcb) {

                UdfRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR );
            }
        }
    }
    
    //
    //  Capture the separate cases.
    //

    if (IgnoreCase) {

        TreeRoot = &ParentFcb->IgnoreCaseRoot;
        Flags = LCB_FLAG_IGNORE_CASE;

    } else {

        TreeRoot = &ParentFcb->ExactCaseRoot;
        Flags = 0;
    }

    if (ShortNameMatch) {

        SetFlag( Flags, LCB_FLAG_SHORT_NAME );
    }

    //
    //  Allocate space for the Lcb.
    //

    if ( sizeof( LCB ) + Name->Length > SIZEOF_LOOKASIDE_LCB ) {
    
        Lcb = FsRtlAllocatePoolWithTag( UdfPagedPool,
                                        sizeof( LCB ) + Name->Length,
                                        TAG_LCB );

        SetFlag( Flags, LCB_FLAG_POOL_ALLOCATED );

    } else {

        Lcb = ExAllocateFromPagedLookasideList( &UdfLcbLookasideList );
    }

    //
    //  Set the type and size.
    //

    Lcb->NodeTypeCode = UDFS_NTC_LCB;
    Lcb->NodeByteSize = sizeof( LCB ) + Name->Length;

    //
    //  Initialize the name-based file attributes.
    //
    
    Lcb->FileAttributes = 0;
    
    //
    //  Set up the filename in the Lcb.
    //

    Lcb->FileName.Length =
    Lcb->FileName.MaximumLength = Name->Length;

    Lcb->FileName.Buffer = Add2Ptr( Lcb, sizeof( LCB ), PWCHAR );

    RtlCopyMemory( Lcb->FileName.Buffer,
                   Name->Buffer,
                   Name->Length );
    
    //
    //  Insert the Lcb into the prefix tree.
    //
    
    Lcb->Flags = Flags;
    
    if (!UdfInsertNameLink( IrpContext,
                            TreeRoot,
                            Lcb )) {

        //
        //  This will very rarely occur.
        //

        UdfFreePool( &Lcb );

        Lcb = UdfFindNameLink( IrpContext,
                               TreeRoot,
                               Name );

        if (Lcb == NULL) {

            //
            //  Even worse.
            //

            UdfRaiseStatus( IrpContext, STATUS_DRIVER_INTERNAL_ERROR );
        }

        return Lcb;
    }

    //
    //  Link the Fcbs together through the Lcb.
    //

    Lcb->ParentFcb = ParentFcb;
    Lcb->ChildFcb = Fcb;

    InsertHeadList( &ParentFcb->ChildLcbQueue, &Lcb->ParentFcbLinks );
    InsertHeadList( &Fcb->ParentLcbQueue, &Lcb->ChildFcbLinks );

    //
    //  Initialize the reference count.
    //

    Lcb->Reference = 0;
    
    return Lcb;
}


VOID
UdfRemovePrefix (
    IN PIRP_CONTEXT IrpContext,
    IN PLCB Lcb
    )

/*++

Routine Description:

    This routine is called to remove a given prefix of an Fcb.

Arguments:

    Lcb - the prefix being removed.

Return Value:

    None

--*/

{
    PAGED_CODE();

    //
    //  Check inputs.
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_LCB( Lcb );

    //
    //  Check the acquisition of the two Fcbs.
    //

    ASSERT_EXCLUSIVE_FCB_OR_VCB( Lcb->ParentFcb );
    ASSERT_EXCLUSIVE_FCB_OR_VCB( Lcb->ChildFcb );

    //
    //  Now remove the linkage and delete the Lcb.
    //
    
    RemoveEntryList( &Lcb->ParentFcbLinks );
    RemoveEntryList( &Lcb->ChildFcbLinks );

    if (FlagOn( Lcb->Flags, LCB_FLAG_IGNORE_CASE )) {

        Lcb->ParentFcb->IgnoreCaseRoot = RtlDelete( &Lcb->Links );
    
    } else {

        Lcb->ParentFcb->ExactCaseRoot = RtlDelete( &Lcb->Links );
    }

    if (FlagOn( Lcb->Flags, LCB_FLAG_POOL_ALLOCATED )) {

        ExFreePool( Lcb );

    } else {

        ExFreeToPagedLookasideList( &UdfLcbLookasideList, Lcb );
    }
    
    return;
}


PLCB
UdfFindPrefix (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFCB *CurrentFcb,
    IN OUT PUNICODE_STRING RemainingName,
    IN BOOLEAN IgnoreCase
    )

/*++

Routine Description:

    This routine begins from the given CurrentFcb and walks through all of
    components of the name looking for the longest match in the prefix
    splay trees.  The search is relative to the starting Fcb so the
    full name may not begin with a '\'.  On return this routine will
    update Current Fcb with the lowest point it has travelled in the
    tree.  It will also hold only that resource on return and it must
    hold that resource.

Arguments:

    CurrentFcb - Address to store the lowest Fcb we find on this search.
        On return we will have acquired this Fcb.  On entry this is the
        Fcb to examine.
        
    RemainingName - Supplies a buffer to store the exact case of the name being
        searched for.  Initially will contain the upcase name based on the
        IgnoreCase flag.

    IgnoreCase - Indicates if we are doing a case-insensitive compare.

Return Value:

    The Lcb used to find the current Fcb, NULL if we didn't find any prefix
    Fcbs.

--*/

{
    UNICODE_STRING LocalRemainingName;
    UNICODE_STRING FinalName;

    PLCB NameLink;
    PLCB CurrentLcb = NULL;

    PAGED_CODE();

    //
    //  Check inputs.
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( *CurrentFcb );
    ASSERT_EXCLUSIVE_FCB( *CurrentFcb );

    //
    //  Make a local copy of the input strings.
    //

    LocalRemainingName = *RemainingName;

    //
    //  Loop until we find the longest matching prefix.
    //

    while (TRUE) {

        //
        //  If there are no characters left or we are not at an IndexFcb then
        //  return immediately.
        //

        if ((LocalRemainingName.Length == 0) ||
            (SafeNodeType( *CurrentFcb ) != UDFS_NTC_FCB_INDEX)) {

            return CurrentLcb;
        }

        //
        //  Split off the next component from the name.
        //

        UdfDissectName( IrpContext,
                        &LocalRemainingName,
                        &FinalName );

        //
        //  Check if this name is in the splay tree for this Scb.
        //

        if (IgnoreCase) {

            NameLink = UdfFindNameLink( IrpContext,
                                        &(*CurrentFcb)->IgnoreCaseRoot,
                                        &FinalName );

        } else {

            NameLink = UdfFindNameLink( IrpContext,
                                        &(*CurrentFcb)->ExactCaseRoot,
                                        &FinalName );
        }

        //
        //  If we didn't find a match then exit.
        //

        if (NameLink == NULL) { 

            break;
        }

        CurrentLcb = NameLink;

        //
        //  If this is a case-insensitive match then copy the exact case of the name into
        //  the input buffer.
        //

        if (IgnoreCase) {

            RtlCopyMemory( FinalName.Buffer,
                           NameLink->FileName.Buffer,
                           NameLink->FileName.Length );
        }

        //
        //  Update the caller's remaining name string to reflect the fact that we found
        //  a match.
        //

        *RemainingName = LocalRemainingName;

        //
        //  Move down to the next component in the tree.  Acquire without waiting.
        //  If this fails then lock the Fcb to reference this Fcb and then drop
        //  the parent and acquire the child.
        //

        ASSERT( NameLink->ParentFcb == *CurrentFcb );

        if (!UdfAcquireFcbExclusive( IrpContext, NameLink->ChildFcb, TRUE )) {

            //
            //  If we can't wait then raise CANT_WAIT.
            //

            if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT )) {

                UdfRaiseStatus( IrpContext, STATUS_CANT_WAIT );
            }

            UdfLockVcb( IrpContext, IrpContext->Vcb );
            NameLink->ChildFcb->FcbReference += 1;
            NameLink->Reference += 1;
            UdfUnlockVcb( IrpContext, IrpContext->Vcb );

            UdfReleaseFcb( IrpContext, *CurrentFcb );
            UdfAcquireFcbExclusive( IrpContext, NameLink->ChildFcb, FALSE );

            UdfLockVcb( IrpContext, IrpContext->Vcb );
            NameLink->ChildFcb->FcbReference -= 1;
            NameLink->Reference -= 1;
            UdfUnlockVcb( IrpContext, IrpContext->Vcb );

        } else {

            UdfReleaseFcb( IrpContext, *CurrentFcb );
        }

        *CurrentFcb = NameLink->ChildFcb;
    }

    return CurrentLcb;
}



VOID            
UdfInitializeLcbFromDirContext (
    IN PIRP_CONTEXT IrpContext,
    IN PLCB Lcb,
    IN PDIR_ENUM_CONTEXT DirContext
    )

/*++

Routine Description:

    This routine performs common initialization of Lcbs from found directory
    entries.

Arguments:

    Lcb - the Lcb to initialize.
    
    DirContext - the directory enumeration context, enumerated to the FID associated
        with this Lcb.
    
Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    //  Check inputs.
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_LCB( Lcb );

    ASSERT( DirContext->Fid != NULL );

    //
    //  This is falling down trivial now.  Simply update the hidden flag in the Lcb.
    //

    if (FlagOn( DirContext->Fid->Flags, NSR_FID_F_HIDDEN )) {

        SetFlag( Lcb->FileAttributes, FILE_ATTRIBUTE_HIDDEN );
    }
}


//
//  Local support routine
//

PLCB
UdfFindNameLink (
    IN PIRP_CONTEXT IrpContext,
    IN PRTL_SPLAY_LINKS *RootNode,
    IN PUNICODE_STRING Name
    )

/*++

Routine Description:

    This routine searches through a splay link tree looking for a match for the
    input name.  If we find the corresponding name we will rebalance the
    tree.

Arguments:

    RootNode - Supplies the parent to search.

    Name - This is the name to search for.  Note if we are doing a case
        insensitive search the name would have been upcased already.

Return Value:

    PLCB - The name link found or NULL if there is no match.

--*/

{
    FSRTL_COMPARISON_RESULT Comparison;
    PLCB Node;
    PRTL_SPLAY_LINKS Links;

    PAGED_CODE();

    Links = *RootNode;

    while (Links != NULL) {

        Node = CONTAINING_RECORD( Links, LCB, Links );

        //
        //  Compare the prefix in the tree with the full name
        //

        Comparison = UdfFullCompareNames( IrpContext, &Node->FileName, Name );

        //
        //  See if they don't match
        //

        if (Comparison == GreaterThan) {

            //
            //  The prefix is greater than the full name
            //  so we go down the left child
            //

            Links = RtlLeftChild( Links );

            //
            //  And continue searching down this tree
            //

        } else if (Comparison == LessThan) {

            //
            //  The prefix is less than the full name
            //  so we go down the right child
            //

            Links = RtlRightChild( Links );

            //
            //  And continue searching down this tree
            //

        } else {

            //
            //  We found it.
            //
            //  Splay the tree and save the new root.
            //

            *RootNode = RtlSplay( Links );

            return Node;
        }
    }

    //
    //  We didn't find the Link.
    //

    return NULL;
}


//
//  Local support routine
//

BOOLEAN
UdfInsertNameLink (
    IN PIRP_CONTEXT IrpContext,
    IN PRTL_SPLAY_LINKS *RootNode,
    IN PLCB NameLink
    )

/*++

Routine Description:

    This routine will insert a name in the splay tree pointed to
    by RootNode.

Arguments:

    RootNode - Supplies a pointer to the table.

    NameLink - Contains the new link to enter.

Return Value:

    BOOLEAN - TRUE if the name is inserted, FALSE otherwise.

--*/

{
    FSRTL_COMPARISON_RESULT Comparison;
    PLCB Node;

    PAGED_CODE();

    //
    //  Check inputs.
    //

    ASSERT_IRP_CONTEXT( IrpContext );

    RtlInitializeSplayLinks( &NameLink->Links );

    //
    //  If we are the first entry in the tree, just become the root.
    //

    if (*RootNode == NULL) {

        *RootNode = &NameLink->Links;

        return TRUE;
    }

    Node = CONTAINING_RECORD( *RootNode, LCB, Links );

    while (TRUE) {

        //
        //  Compare the prefix in the tree with the prefix we want
        //  to insert.
        //

        Comparison = UdfFullCompareNames( IrpContext, &Node->FileName, &NameLink->FileName );

        //
        //  If we found the entry, return immediately.
        //

        if (Comparison == EqualTo) { return FALSE; }

        //
        //  If the tree prefix is greater than the new prefix then
        //  we go down the left subtree
        //

        if (Comparison == GreaterThan) {

            //
            //  We want to go down the left subtree, first check to see
            //  if we have a left subtree
            //

            if (RtlLeftChild( &Node->Links ) == NULL) {

                //
                //  there isn't a left child so we insert ourselves as the
                //  new left child
                //

                RtlInsertAsLeftChild( &Node->Links, &NameLink->Links );

                //
                //  and exit the while loop
                //

                break;

            } else {

                //
                //  there is a left child so simply go down that path, and
                //  go back to the top of the loop
                //

                Node = CONTAINING_RECORD( RtlLeftChild( &Node->Links ),
                                          LCB,
                                          Links );
            }

        } else {

            //
            //  The tree prefix is either less than or a proper prefix
            //  of the new string.  We treat both cases as less than when
            //  we do insert.  So we want to go down the right subtree,
            //  first check to see if we have a right subtree
            //

            if (RtlRightChild( &Node->Links ) == NULL) {

                //
                //  These isn't a right child so we insert ourselves as the
                //  new right child
                //

                RtlInsertAsRightChild( &Node->Links, &NameLink->Links );

                //
                //  and exit the while loop
                //

                break;

            } else {

                //
                //  there is a right child so simply go down that path, and
                //  go back to the top of the loop
                //

                Node = CONTAINING_RECORD( RtlRightChild( &Node->Links ),
                                          LCB,
                                          Links );
            }
        }
    }

    return TRUE;
}

