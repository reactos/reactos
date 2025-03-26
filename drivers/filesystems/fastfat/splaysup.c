/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    PrefxSup.c

Abstract:

    This module implements the Fat Name lookup Suport routines


--*/

#include "fatprocs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (FAT_BUG_CHECK_SPLAYSUP)

//
//  The debug trace level for this module
//

#define Dbg                              (DEBUG_TRACE_SPLAYSUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FatInsertName)
#pragma alloc_text(PAGE, FatRemoveNames)
#pragma alloc_text(PAGE, FatFindFcb)
#pragma alloc_text(PAGE, FatCompareNames)
#endif


VOID
FatInsertName (
    IN PIRP_CONTEXT IrpContext,
    IN PRTL_SPLAY_LINKS *RootNode,
    IN PFILE_NAME_NODE Name
    )

/*++

Routine Description:

    This routine will insert a name in the splay tree pointed to
    by RootNode.

    The name must not already exist in the splay tree.

Arguments:

    RootNode - Supplies a pointer to the table.

    Name - Contains the New name to enter.

Return Value:

    None.

--*/

{
    COMPARISON Comparison;
    PFILE_NAME_NODE Node;

    PAGED_CODE();

    RtlInitializeSplayLinks(&Name->Links);

Restart:

    //
    //  If we are the first entry in the tree, just become the root.
    //

    if (*RootNode == NULL) {

        *RootNode = &Name->Links;

        return;
    }

    Node = CONTAINING_RECORD( *RootNode, FILE_NAME_NODE, Links );

    while (TRUE) {

        //
        //  Compare the prefix in the tree with the prefix we want
        //  to insert.  Note that Oem here doesn't mean anything.
        //

        Comparison = CompareNames(&Node->Name.Oem, &Name->Name.Oem);

        //
        //  We should never find the name in the table already.
        //

        if (Comparison == IsEqual) {

            //
            //  Almost. If the removable media was taken to another machine and
            //  back, and we have something like:
            //
            //  Old: abcdef~1  /  abcdefxyz
            //  New: abcdef~1  /  abcdefxyzxyz
            //
            //  but a handle was kept open to abcdefxyz so we couldn't purge
            //  away the Fcb in the verify path ... opening abcdefxyzxyz will
            //  try to insert a duplicate shortname. Bang!
            //
            //  Invalidate it and the horse it came in on.  This new one wins.
            //  The old one is gone.  Only if the old one is in normal state
            //  do we really have a problem.
            //

            if (Node->Fcb->FcbState == FcbGood) {

#ifdef _MSC_VER
#pragma prefast( suppress:28159, "things are seriously wrong if we get here" )
#endif
                FatBugCheck( (ULONG_PTR)*RootNode, (ULONG_PTR)Name, (ULONG_PTR)Node );
            }

            //
            //  Note, once we zap the prefix links we need to restart our walk
            //  of the tree. Note that we aren't properly synchronized to
            //  recursively mark bad.
            //

            FatMarkFcbCondition( IrpContext, Node->Fcb, FcbBad, FALSE );
            FatRemoveNames( IrpContext, Node->Fcb );

            goto Restart;
        }

        //
        //  If the tree prefix is greater than the new prefix then
        //  we go down the left subtree
        //

        if (Comparison == IsGreaterThan) {

            //
            //  We want to go down the left subtree, first check to see
            //  if we have a left subtree
            //

            if (RtlLeftChild(&Node->Links) == NULL) {

                //
                //  there isn't a left child so we insert ourselves as the
                //  new left child
                //

                RtlInsertAsLeftChild(&Node->Links, &Name->Links);

                //
                //  and exit the while loop
                //

                break;

            } else {

                //
                //  there is a left child so simply go down that path, and
                //  go back to the top of the loop
                //

                Node = CONTAINING_RECORD( RtlLeftChild(&Node->Links),
                                          FILE_NAME_NODE,
                                          Links );
            }

        } else {

            //
            //  The tree prefix is either less than or a proper prefix
            //  of the new string.  We treat both cases a less than when
            //  we do insert.  So we want to go down the right subtree,
            //  first check to see if we have a right subtree
            //

            if (RtlRightChild(&Node->Links) == NULL) {

                //
                //  These isn't a right child so we insert ourselves as the
                //  new right child
                //

                RtlInsertAsRightChild(&Node->Links, &Name->Links);

                //
                //  and exit the while loop
                //

                break;

            } else {

                //
                //  there is a right child so simply go down that path, and
                //  go back to the top of the loop
                //

                Node = CONTAINING_RECORD( RtlRightChild(&Node->Links),
                                          FILE_NAME_NODE,
                                          Links );
            }

        }
    }

    return;
}

VOID
FatRemoveNames (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine will remove the short name and any long names associated
    with the files from their repsective splay tree.

Arguments:

    Name - Supplies the Fcb to process.

Return Value:

    None.

--*/

{
    PDCB Parent;
    PRTL_SPLAY_LINKS NewRoot;

    PAGED_CODE();
    UNREFERENCED_PARAMETER( IrpContext );

    Parent = Fcb->ParentDcb;

    //
    //  We used to assert this condition, but it really isn't good.  If
    //  someone rapidly renames a directory multiple times and we can't
    //  flush the lower fcbs fast enough (that didn't go away synch.)
    //  well, well hit some of them again.
    //
    //  NT_ASSERT( FlagOn( Fcb->FcbState, FCB_STATE_NAMES_IN_SPLAY_TREE ));
    //

    if (FlagOn( Fcb->FcbState, FCB_STATE_NAMES_IN_SPLAY_TREE )) {

        //
        //  Delete the node short name.
        //

        NewRoot = RtlDelete(&Fcb->ShortName.Links);

        Parent->Specific.Dcb.RootOemNode = NewRoot;

        //
        //  Now check for the presence of long name and delete it.
        //

        if (FlagOn( Fcb->FcbState, FCB_STATE_HAS_OEM_LONG_NAME )) {

            NewRoot = RtlDelete(&Fcb->LongName.Oem.Links);

            Parent->Specific.Dcb.RootOemNode = NewRoot;

            RtlFreeOemString( &Fcb->LongName.Oem.Name.Oem );

            ClearFlag( Fcb->FcbState, FCB_STATE_HAS_OEM_LONG_NAME );
        }

        if (FlagOn( Fcb->FcbState, FCB_STATE_HAS_UNICODE_LONG_NAME )) {

            NewRoot = RtlDelete(&Fcb->LongName.Unicode.Links);

            Parent->Specific.Dcb.RootUnicodeNode = NewRoot;

            RtlFreeUnicodeString( &Fcb->LongName.Unicode.Name.Unicode );

            ClearFlag( Fcb->FcbState, FCB_STATE_HAS_UNICODE_LONG_NAME );
        }

        ClearFlag( Fcb->FcbState, FCB_STATE_NAMES_IN_SPLAY_TREE );
    }

    return;
}


PFCB
FatFindFcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PRTL_SPLAY_LINKS *RootNode,
    IN PSTRING Name,
    OUT PBOOLEAN FileNameDos OPTIONAL
    )

/*++

Routine Description:

    This routine searches either the Oem or Unicode splay tree looking
    for an Fcb with the specified name.  In the case the Fcb is found,
    rebalance the tree.

Arguments:

    RootNode - Supplies the parent to search.

    Name - If present, search the Oem tree.

    UnicodeName - If present, search the Unicode tree.

Return Value:

    PFCB - The Fcb, or NULL if none was found.

--*/

{
    COMPARISON Comparison;
    PFILE_NAME_NODE Node;
    PRTL_SPLAY_LINKS Links;

    PAGED_CODE();
    UNREFERENCED_PARAMETER( IrpContext );

    Links = *RootNode;

    while (Links != NULL) {

        Node = CONTAINING_RECORD(Links, FILE_NAME_NODE, Links);

        //
        //  Compare the prefix in the tree with the full name
        //

        Comparison = CompareNames(&Node->Name.Oem, Name);

        //
        //  See if they don't match
        //

        if (Comparison == IsGreaterThan) {

            //
            //  The prefix is greater than the full name
            //  so we go down the left child
            //

            Links = RtlLeftChild(Links);

            //
            //  And continue searching down this tree
            //

        } else if (Comparison == IsLessThan) {

            //
            //  The prefix is less than the full name
            //  so we go down the right child
            //

            Links = RtlRightChild(Links);

            //
            //  And continue searching down this tree
            //

        } else {

            //
            //  We found it.
            //
            //  Splay the tree and save the new root.
            //

            *RootNode = RtlSplay(Links);

            //
            //  Tell the caller what kind of name we hit
            //

            if (FileNameDos) {

                *FileNameDos = Node->FileNameDos;
            }

            return Node->Fcb;
        }
    }

    //
    //  We didn't find the Fcb.
    //

    return NULL;
}


//
//  Local support routine
//

COMPARISON
FatCompareNames (
    IN PSTRING NameA,
    IN PSTRING NameB
    )

/*++

Routine Description:

    This function compares two names as fast as possible.  Note that since
    this comparison is case sensitive, I neither know nor case if the names
    are UNICODE or OEM.  All that is important is that the result is
    deterministic.

Arguments:

    NameA & NameB - The names to compare.

Return Value:

    COMPARISON - returns

        IsLessThan    if NameA < NameB lexicalgraphically,
        IsGreaterThan if NameA > NameB lexicalgraphically,
        IsEqual       if NameA is equal to NameB

--*/

{
    ULONG i;
    ULONG MinLength;

    PAGED_CODE();

    //
    //  Figure out the minimum of the two lengths
    //

    MinLength = NameA->Length < NameB->Length ? NameA->Length :
                                                NameB->Length;

    //
    //  Loop through looking at all of the characters in both strings
    //  testing for equalilty, less than, and greater than
    //

    i = (ULONG)RtlCompareMemory( NameA->Buffer, NameB->Buffer, MinLength );


    if (i < MinLength) {

        return NameA->Buffer[i] < NameB->Buffer[i] ? IsLessThan :
                                                     IsGreaterThan;
    }

    if (NameA->Length < NameB->Length) {

        return IsLessThan;
    }

    if (NameA->Length > NameB->Length) {

        return IsGreaterThan;
    }

    return IsEqual;
}

