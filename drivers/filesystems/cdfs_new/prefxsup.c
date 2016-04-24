/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    PrefxSup.c

Abstract:

    This module implements the Cdfs Prefix support routines


--*/

#include "CdProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_PREFXSUP)

//
//  Local support routines.
//

PNAME_LINK
CdFindNameLink (
    IN PIRP_CONTEXT IrpContext,
    IN PRTL_SPLAY_LINKS *RootNode,
    IN PUNICODE_STRING Name
    );

BOOLEAN
CdInsertNameLink (
    IN PIRP_CONTEXT IrpContext,
    IN PRTL_SPLAY_LINKS *RootNode,
    IN PNAME_LINK NameLink
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdFindNameLink)
#pragma alloc_text(PAGE, CdFindPrefix)
#pragma alloc_text(PAGE, CdInsertNameLink)
#pragma alloc_text(PAGE, CdInsertPrefix)
#pragma alloc_text(PAGE, CdRemovePrefix)
#endif


VOID
CdInsertPrefix (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PCD_NAME Name,
    IN BOOLEAN IgnoreCase,
    IN BOOLEAN ShortNameMatch,
    IN PFCB ParentFcb
    )

/*++

Routine Description:

    This routine inserts the names in the given Lcb into the links for the
    parent.

Arguments:

    Fcb - This is the Fcb whose name is being inserted into the tree.

    Name - This is the name for the component.  The IgnoreCase flag tells
        us which entry this belongs to.

    IgnoreCase - Indicates if we should insert the case-insensitive name.

    ShortNameMatch - Indicates if this is the short name.

    ParentFcb - This is the ParentFcb.  The prefix tree is attached to this.

Return Value:

    None.

--*/

{
    ULONG PrefixFlags;
    PNAME_LINK NameLink;
    PPREFIX_ENTRY PrefixEntry;
    PRTL_SPLAY_LINKS *TreeRoot;

    PWCHAR NameBuffer;

    PAGED_CODE();

    //
    //  Check if we need to allocate a prefix entry for the short name.
    //  If we can't allocate one then fail quietly.  We don't have to
    //  insert the name.
    //

    PrefixEntry = &Fcb->FileNamePrefix;

    if (ShortNameMatch) {

        if (Fcb->ShortNamePrefix == NULL) {

            Fcb->ShortNamePrefix = ExAllocatePoolWithTag( CdPagedPool,
                                                          sizeof( PREFIX_ENTRY ),
                                                          TAG_PREFIX_ENTRY );

            if (Fcb->ShortNamePrefix == NULL) { return; }

            RtlZeroMemory( Fcb->ShortNamePrefix, sizeof( PREFIX_ENTRY ));
        }

        PrefixEntry = Fcb->ShortNamePrefix;
    }

    //
    //  Capture the local variables for the separate cases.
    //

    if (IgnoreCase) {

        PrefixFlags = PREFIX_FLAG_IGNORE_CASE_IN_TREE;
        NameLink = &PrefixEntry->IgnoreCaseName;
        TreeRoot = &ParentFcb->IgnoreCaseRoot;

    } else {

        PrefixFlags = PREFIX_FLAG_EXACT_CASE_IN_TREE;
        NameLink = &PrefixEntry->ExactCaseName;
        TreeRoot = &ParentFcb->ExactCaseRoot;
    }

    //
    //  If neither name is in the tree then check whether we have a buffer for this
    //  name
    //

    if (!FlagOn( PrefixEntry->PrefixFlags,
                 PREFIX_FLAG_EXACT_CASE_IN_TREE | PREFIX_FLAG_IGNORE_CASE_IN_TREE )) {

        //
        //  Allocate a new buffer if the embedded buffer is too small.
        //

        NameBuffer = PrefixEntry->FileNameBuffer;

        if (Name->FileName.Length > BYTE_COUNT_EMBEDDED_NAME) {

            NameBuffer = ExAllocatePoolWithTag( CdPagedPool,
                                                Name->FileName.Length * 2,
                                                TAG_PREFIX_NAME );

            //
            //  Exit if no name buffer.
            //

            if (NameBuffer == NULL) { return; }
        }

        //
        //  Split the buffer and fill in the separate components.
        //

        PrefixEntry->ExactCaseName.FileName.Buffer = NameBuffer;
        PrefixEntry->IgnoreCaseName.FileName.Buffer = Add2Ptr( NameBuffer,
                                                               Name->FileName.Length,
                                                               PWCHAR );

        PrefixEntry->IgnoreCaseName.FileName.MaximumLength =
        PrefixEntry->IgnoreCaseName.FileName.Length =
        PrefixEntry->ExactCaseName.FileName.MaximumLength =
        PrefixEntry->ExactCaseName.FileName.Length = Name->FileName.Length;
    }

    //
    //  Only insert the name if not already present.
    //

    if (!FlagOn( PrefixEntry->PrefixFlags, PrefixFlags )) {

        //
        //  Initialize the name in the prefix entry.
        //

        RtlCopyMemory( NameLink->FileName.Buffer,
                       Name->FileName.Buffer,
                       Name->FileName.Length );

        CdInsertNameLink( IrpContext,
                          TreeRoot,
                          NameLink );

        PrefixEntry->Fcb = Fcb;
        SetFlag( PrefixEntry->PrefixFlags, PrefixFlags );
    }

    return;
}


VOID
CdRemovePrefix (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine is called to remove all of the previx entries of a
    given Fcb from its parent Fcb.

Arguments:

    Fcb - Fcb whose entries are to be removed.

Return Value:

    None

--*/

{
    PAGED_CODE();

    //
    //  Start with the short name prefix entry.
    //

    if (Fcb->ShortNamePrefix != NULL) {

        if (FlagOn( Fcb->ShortNamePrefix->PrefixFlags, PREFIX_FLAG_IGNORE_CASE_IN_TREE )) {

            Fcb->ParentFcb->IgnoreCaseRoot = RtlDelete( &Fcb->ShortNamePrefix->IgnoreCaseName.Links );
        }

        if (FlagOn( Fcb->ShortNamePrefix->PrefixFlags, PREFIX_FLAG_EXACT_CASE_IN_TREE )) {

            Fcb->ParentFcb->ExactCaseRoot = RtlDelete( &Fcb->ShortNamePrefix->ExactCaseName.Links );
        }

        ClearFlag( Fcb->ShortNamePrefix->PrefixFlags,
                   PREFIX_FLAG_IGNORE_CASE_IN_TREE | PREFIX_FLAG_EXACT_CASE_IN_TREE );
    }

    //
    //  Now do the long name prefix entries.
    //

    if (FlagOn( Fcb->FileNamePrefix.PrefixFlags, PREFIX_FLAG_IGNORE_CASE_IN_TREE )) {

        Fcb->ParentFcb->IgnoreCaseRoot = RtlDelete( &Fcb->FileNamePrefix.IgnoreCaseName.Links );
    }

    if (FlagOn( Fcb->FileNamePrefix.PrefixFlags, PREFIX_FLAG_EXACT_CASE_IN_TREE )) {

        Fcb->ParentFcb->ExactCaseRoot = RtlDelete( &Fcb->FileNamePrefix.ExactCaseName.Links );
    }

    ClearFlag( Fcb->FileNamePrefix.PrefixFlags,
               PREFIX_FLAG_IGNORE_CASE_IN_TREE | PREFIX_FLAG_EXACT_CASE_IN_TREE );

    //
    //  Deallocate any buffer we may have allocated.
    //

    if ((Fcb->FileNamePrefix.ExactCaseName.FileName.Buffer != (PWCHAR) &Fcb->FileNamePrefix.FileNameBuffer) &&
        (Fcb->FileNamePrefix.ExactCaseName.FileName.Buffer != NULL)) {

        CdFreePool( &Fcb->FileNamePrefix.ExactCaseName.FileName.Buffer );
        Fcb->FileNamePrefix.ExactCaseName.FileName.Buffer = NULL;
    }

    return;
}


VOID
CdFindPrefix (
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

    None

--*/

{
    UNICODE_STRING LocalRemainingName;

    UNICODE_STRING FinalName;

    PNAME_LINK NameLink;
    PPREFIX_ENTRY PrefixEntry;

    PAGED_CODE();

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
            (SafeNodeType( *CurrentFcb ) != CDFS_NTC_FCB_INDEX)) {

            return;
        }

        //
        //  Split off the next component from the name.
        //

        CdDissectName( IrpContext,
                       &LocalRemainingName,
                       &FinalName );

        //
        //  Check if this name is in the splay tree for this Scb.
        //

        if (IgnoreCase) {

            NameLink = CdFindNameLink( IrpContext,
                                       &(*CurrentFcb)->IgnoreCaseRoot,
                                       &FinalName );

            //
            //  Get the prefix entry from this NameLink.  Don't access any
            //  fields within it until we verify we have a name link.
            //

            PrefixEntry = (PPREFIX_ENTRY) CONTAINING_RECORD( NameLink,
                                                             PREFIX_ENTRY,
                                                             IgnoreCaseName );

        } else {

            NameLink = CdFindNameLink( IrpContext,
                                       &(*CurrentFcb)->ExactCaseRoot,
                                       &FinalName );

            PrefixEntry = (PPREFIX_ENTRY) CONTAINING_RECORD( NameLink,
                                                             PREFIX_ENTRY,
                                                             ExactCaseName );
        }

        //
        //  If we didn't find a match then exit.
        //

        if (NameLink == NULL) { return; }

        //
        //  If this is a case-insensitive match then copy the exact case of the name into
        //  the input buffer.
        //

        if (IgnoreCase) {

            RtlCopyMemory( FinalName.Buffer,
                           PrefixEntry->ExactCaseName.FileName.Buffer,
                           PrefixEntry->ExactCaseName.FileName.Length );
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

        if (!CdAcquireFcbExclusive( IrpContext, PrefixEntry->Fcb, TRUE )) {

            //
            //  If we can't wait then raise CANT_WAIT.
            //

            if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT )) {

                CdRaiseStatus( IrpContext, STATUS_CANT_WAIT );
            }

            CdLockVcb( IrpContext, IrpContext->Vcb );
            PrefixEntry->Fcb->FcbReference += 1;
            CdUnlockVcb( IrpContext, IrpContext->Vcb );

            CdReleaseFcb( IrpContext, *CurrentFcb );
            CdAcquireFcbExclusive( IrpContext, PrefixEntry->Fcb, FALSE );

            CdLockVcb( IrpContext, IrpContext->Vcb );
            PrefixEntry->Fcb->FcbReference -= 1;
            CdUnlockVcb( IrpContext, IrpContext->Vcb );

        } else {

            CdReleaseFcb( IrpContext, *CurrentFcb );
        }

        *CurrentFcb = PrefixEntry->Fcb;
    }
}


//
//  Local support routine
//

PNAME_LINK
CdFindNameLink (
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

    PNAME_LINK - The name link found or NULL if there is no match.

--*/

{
    FSRTL_COMPARISON_RESULT Comparison;
    PNAME_LINK Node;
    PRTL_SPLAY_LINKS Links;

    PAGED_CODE();

    Links = *RootNode;

    while (Links != NULL) {

        Node = CONTAINING_RECORD( Links, NAME_LINK, Links );

        //
        //  Compare the prefix in the tree with the full name
        //

        Comparison = CdFullCompareNames( IrpContext, &Node->FileName, Name );

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
CdInsertNameLink (
    IN PIRP_CONTEXT IrpContext,
    IN PRTL_SPLAY_LINKS *RootNode,
    IN PNAME_LINK NameLink
    )

/*++

Routine Description:

    This routine will insert a name in the splay tree pointed to
    by RootNode.

    The name could already exist in this tree for a case-insensitive tree.
    In that case we simply return FALSE and do nothing.

Arguments:

    RootNode - Supplies a pointer to the table.

    NameLink - Contains the new link to enter.

Return Value:

    BOOLEAN - TRUE if the name is inserted, FALSE otherwise.

--*/

{
    FSRTL_COMPARISON_RESULT Comparison;
    PNAME_LINK Node;

    PAGED_CODE();

    RtlInitializeSplayLinks( &NameLink->Links );

    //
    //  If we are the first entry in the tree, just become the root.
    //

    if (*RootNode == NULL) {

        *RootNode = &NameLink->Links;

        return TRUE;
    }

    Node = CONTAINING_RECORD( *RootNode, NAME_LINK, Links );

    while (TRUE) {

        //
        //  Compare the prefix in the tree with the prefix we want
        //  to insert.
        //

        Comparison = CdFullCompareNames( IrpContext, &Node->FileName, &NameLink->FileName );

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
                                          NAME_LINK,
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
                                          NAME_LINK,
                                          Links );
            }
        }
    }

    return TRUE;
}





