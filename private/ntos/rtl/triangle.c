/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Triangle.c

Abstract:

    This module implements the general splay utilities for a two link
    triangular splay structure.

Author:

    Gary Kimura     [GaryKi]    28-May-1989

Environment:

    Pure utility routine

Revision History:

--*/

#include <nt.h>
#include "triangle.h"


//
//  There are three type of swap macros.  The first two (are really the same)
//  are used to swap pointer and ulongs.  The last macro is used to swap refs
//  but it does not swap the ref type flags.
//

#define SwapPointers(Ptr1, Ptr2) {      \
    PVOID _SWAP_POINTER_TEMP;           \
    _SWAP_POINTER_TEMP = (PVOID)(Ptr1); \
    (Ptr1) = (Ptr2);                    \
    (Ptr2) = _SWAP_POINTER_TEMP;        \
    }

#define SwapUlongs(Ptr1, Ptr2) {        \
    ULONG _SWAP_POINTER_TEMP;           \
    _SWAP_POINTER_TEMP = (ULONG)(Ptr1); \
    (Ptr1) = (Ptr2);                    \
    (Ptr2) = _SWAP_POINTER_TEMP;        \
    }

#define SwapRefsButKeepFlags(Ref1, Ref2) {                            \
    ULONG _SWAP_ULONG_TEMP;                                           \
    _SWAP_ULONG_TEMP = (ULONG)(Ref1);                                 \
    (Ref1) = ((Ref2)           & 0xfffffffc) | ((Ref1) & 0x00000003); \
    (Ref2) = (_SWAP_ULONG_TEMP & 0xfffffffc) | ((Ref2) & 0x00000003); \
    }

//
//  The macro SetRefViaPointer takes a pointer to a ref and checks to see if
//  it is a valid pointer.  If it is a valid pointer it copies in the ref
//  a ulong, but does not overwrite the ref flags already in the ref.
//

#define SetRefViaPointer(Ref, Ulong) { \
    if (Ref != NULL) { \
        (*(Ref)) = (((ULONG)(Ulong)) & 0xfffffffc) | ((ULONG)(*(Ref)) & 0x00000003); \
    } \
}


//
//  The following five procedures are local to triangle.c and are used to
//  help manipluate the splay links.  The first two procedures take a pointer
//  to a splay link and returns the address of the ref that points back to the
//  input link, via either the parent or child.  They return NULL if there is
//  not a back pointer.  The result of these two procedures is often used in
//  the code with the SetRefViaPointer macro.  The third procedure is used
//  to swap the position to two splay links in the tree (i.e., the links swap
//  position, but everyone else stays stationary).  This is a general procedure
//  that can will swap any two nodes, irregardless of their relative positions
//  in the tree.  The last two procedures do a single rotation about a
//  tree node.  They either rotate left or rotate right and assume that the
//  appropriate child exists (i.e., for rotate left a right child exists and
//  for rotate right a left child exists).
//

PULONG
TriAddressOfBackRefViaParent (
    IN PTRI_SPLAY_LINKS Links
    );

PULONG
TriAddressOfBackRefViaChild (
    IN PTRI_SPLAY_LINKS Links
    );

VOID
TriSwapSplayLinks (
    IN PTRI_SPLAY_LINKS Link1,
    IN PTRI_SPLAY_LINKS Link2
    );

VOID
TriRotateRight (
   IN PTRI_SPLAY_LINKS Links
   );

VOID
TriRotateLeft (
    IN PTRI_SPLAY_LINKS Links
    );

PTRI_SPLAY_LINKS
TriSplay (
    IN PTRI_SPLAY_LINKS Links
    )

/*++

Routine Description:

    This Splay function takes as input a pointer to a splay link in a tree
    and splays the tree.  Its function return value is a pointer to the
    root of the splayed tree.

Arguments:

    Links - Supplies the pointer to a splay link in a tree

Return Values:

    PRTI_SPLAY_LINKS - Returns a pointer to the root of the splayed tree

--*/

{
    PTRI_SPLAY_LINKS Parent;
    PTRI_SPLAY_LINKS GrandParent;

    //
    //  While Links is not the root we test and rotate until it is the root.
    //

    while (!TriIsRoot(Links)) {

        //
        //  Get Parent and then check if we don't have a grandparent.
        //

        Parent = TriParent(Links);

        if (TriIsRoot(Parent)) {

            //
            //  No grandparent so check for single rotation
            //

            if (TriIsLeftChild(Links)) {

                //
                //  do the following single rotation
                //
                //          Parent           Links
                //           /        ==>        \
                //      Links                     Parent
                //

                TriRotateRight(Parent);

            } else { // TriIsRightChild(Links)

                //
                //  do the following single rotation
                //
                //
                //      Parent                    Links
                //          \        ==>          /
                //           Links          Parent
                //

                TriRotateLeft(Parent);

            }

        } else { // !TriIsRoot(Parent)

            //
            //  Get grandparent and check for the four double rotation
            //  cases
            //

            GrandParent = TriParent(Parent);

            if (TriIsLeftChild(Links)) {

                if (TriIsLeftChild(Parent)) {

                    //
                    //  do the following double rotation
                    //
                    //          GP         L
                    //         /            \
                    //        P      ==>     P
                    //       /                \
                    //      L                  GP
                    //

                    TriRotateRight(GrandParent);
                    TriRotateRight(Parent);

                } else { // TriIsRightChild(Parent)

                    //
                    //  do the following double rotation
                    //
                    //      GP                L
                    //        \              / \
                    //         P    ==>    GP   P
                    //        /
                    //       L
                    //

                    TriRotateRight(Parent);
                    TriRotateLeft(GrandParent);

                }

            } else { // TriIsRightChild(Links);

                if (TriIsLeftChild(Parent)) {

                    //
                    //  do the following double rotation
                    //
                    //        GP             L
                    //       /              / \
                    //      P       ==>    P   GP
                    //       \
                    //        L
                    //

                    TriRotateLeft(Parent);
                    TriRotateRight(GrandParent);

                } else { // TriIsRightChild(Parent)

                    //
                    //  do the following double rotation
                    //
                    //      GP                   L
                    //        \                 /
                    //         P      ==>      P
                    //          \             /
                    //           L          GP
                    //

                    TriRotateLeft(GrandParent);
                    TriRotateLeft(Parent);

                }

            }

        }

    }

    return Links;

}


PTRI_SPLAY_LINKS
TriDelete (
    IN PTRI_SPLAY_LINKS Links
    )

/*++

Routine Description:

    This Delete function takes as input a pointer to a splay link in a tree
    and deletes that node from the tree.  Its function return value is a
    pointer to the root the tree.  If the tree is now empty, the return
    value is NULL.

Arguments:

    Links - Supplies the pointer to a splay link in a tree

Return Values:

    PRTI_SPLAY_LINKS - Returns a pointer to the root of the splayed tree

--*/

{
    PTRI_SPLAY_LINKS Predecessor;
    PTRI_SPLAY_LINKS Parent;
    PTRI_SPLAY_LINKS Child;

    PULONG ParentChildRef;

    //
    //  First check to see if Links as two children.  If it does then swap
    //  Links with its subtree predecessor.  Now we are guaranteed that Links
    //  has at most one child.
    //

    if ((TriLeftChild(Links) != NULL) && (TriRightChild(Links) != NULL)) {

        //
        //  get the predecessor, and swap their position in the tree
        //

        Predecessor = TriSubtreePredecessor(Links);
        TriSwapSplayLinks(Predecessor, Links);

    }

    //
    //  If Links has no children then delete links by checking if it is
    //  already the root or has a parent.  If it is the root then the
    //  tree is now empty, otherwise set the appropriate parent's child
    //  pointer, and possibly sibling, and splay the parent.
    //

    if ((TriLeftChild(Links) == NULL) && (TriRightChild(Links) == NULL)) {

        //
        //  Links has no children, if it is the root then return NULL
        //

        if (TriIsRoot(Links)) {

            return NULL;

        }

        //
        //  Links has no children, check to see if links is an only child
        //

        Parent = TriParent(Links);
        if (MakeIntoPointer(Parent->Refs.Child) == Links &&
            MakeIntoPointer(Links->Refs.ParSib) == Parent) {

            //
            //  Links has no children and is an only child.  So simply make
            //  our parent have no children and splay our parent.
            //
            //          Parent              Parent
            //            |         ==>
            //          Links
            //

            Parent->Refs.Child = 0;
            return TriSplay(Parent);

        } else if (TriIsLeftChild(Links)) {

            //
            //  Links has no children and has a right sibling.  So make the
            //  parent's child Ref be the right sibling, splay the parent.
            //
            //             Parent                 Parent
            //              /  \           ==>        \
            //          Links  Sibling                Sibling
            //

            Parent->Refs.Child = MakeIntoRightChildRef(Links->Refs.ParSib);
            return TriSplay(Parent);

        } else { // TriIsRightChild(Links)

            //
            //  Links has no children and has a left sibling.  So make link's
            //  back via its parent into a parent ref of link's parent, and
            //  splay the parent.
            //
            //             Parent                    Parent
            //              /  \                     /
            //        Sibling  Links    ==>    Sibling
            //

            ParentChildRef = TriAddressOfBackRefViaParent(Links);
            *ParentChildRef = MakeIntoParentRef(Parent);
            return TriSplay(Parent);

        }

    }

    //
    //  otherwise Links has one child.  If it is the root then make the child
    //  the new root, otherwise link together the child and parent, and splay
    //  the parent.  But first remember who our child is.
    //

    if (TriLeftChild(Links) != NULL) {
        Child = TriLeftChild(Links);
    } else {
        Child = TriRightChild(Links);
    }

    //
    //  If links is the root then we make the child the root and return the
    //  child.
    //

    if (TriIsRoot(Links)) {
        Child->Refs.ParSib = MakeIntoParentRef(Child);
        return Child;
    }

    //
    //  Links is not the root, so set links's back ref via its parent to be
    //  links's child and the set the child's ParSib to be link's ParSib, and
    //  splay the parent.  This will handle the case where link is an only
    //  or has a sibling on either side.
    //

    Parent = TriParent(Links);
    ParentChildRef = TriAddressOfBackRefViaParent(Links);
    SetRefViaPointer(ParentChildRef, Child);
    Child->Refs.ParSib = Links->Refs.ParSib;

    return TriSplay(Parent);

}


PTRI_SPLAY_LINKS
TriSubtreeSuccessor (
    IN PTRI_SPLAY_LINKS Links
    )

/*++

Routine Description:

    This SubTreeSuccessor function takes as input a pointer to a splay link
    in a tree and returns a pointer to the successor of the input node of
    the subtree rooted at the input node.  If there is not a successor, the
    return value is NULL.

Arguments:

    Links - Supplies the pointer to a splay link in a tree

Return Values:

    PRTI_SPLAY_LINKS - Returns a pointer to the successor in the subtree

--*/

{
    PTRI_SPLAY_LINKS Ptr;

    //
    //  check to see if there is a right subtree to the input link
    //  if there is then the subtree successor is the left most node in
    //  the right subtree.  That is find and return P in the following diagram
    //
    //              Links
    //                 \
    //                  .
    //                 .
    //                .
    //               /
    //              P
    //               \
    //

    if ((Ptr = TriRightChild(Links)) != NULL) {

        while (TriLeftChild(Ptr) != NULL) {
            Ptr = TriLeftChild(Ptr);
        }

        return Ptr;

    }

    //
    //  Otherwise we do not have a subtree successor so we simply return NULL
    //

    return NULL;

}


PTRI_SPLAY_LINKS
TriSubtreePredecessor (
    IN PTRI_SPLAY_LINKS Links
    )

/*++

Routine Description:

    This SubTreePredecessor function takes as input a pointer to a splay link
    in a tree and returns a pointer to the predecessor of the input node of
    the subtree rooted at the input node.  If there is not a predecessor,
    the return value is NULL.

Arguments:

    Links - Supplies the pointer to a splay link in a tree

Return Values:

    PRTI_SPLAY_LINKS - Returns a pointer to the predecessor in the subtree

--*/

{
    PTRI_SPLAY_LINKS Ptr;

    //
    //  check to see if there is a left subtree to the input link
    //  if there is then the subtree predecessor is the right most node in
    //  the left subtree.  That is find and return P in the following diagram
    //
    //              Links
    //               /
    //              .
    //               .
    //                .
    //                 P
    //                /
    //

    if ((Ptr = TriLeftChild(Links)) != NULL) {

        while (TriRightChild(Ptr) != NULL) {
            Ptr = TriRightChild(Ptr);
        }

        return Ptr;

    }

    //
    //  Otherwise we do not have a subtree predecessor so we simply return NULL
    //

    return NULL;

}


PTRI_SPLAY_LINKS
TriRealSuccessor (
    IN PTRI_SPLAY_LINKS Links
    )

/*++

Routine Description:

    This RealSuccess function takes as input a pointer to a splay link in a
    tree and returns a pointer to the successor of the input node within the
    entire tire.  If there is not a successor, the return value is NULL.

Arguments:

    Links - Supplies the pointer to a splay link in a tree

Return Values:

    PRTI_SPLAY_LINKS - Returns a pointer to the successor in the entire tree

--*/

{
    PTRI_SPLAY_LINKS Ptr;

    //
    //  first check to see if there is a right subtree to the input link
    //  if there is then the real successor is the left most node in
    //  the right subtree.  That is find and return P in the following diagram
    //
    //              Links
    //                 \
    //                  .
    //                 .
    //                .
    //               /
    //              P
    //               \
    //

    if ((Ptr = TriRightChild(Links)) != NULL) {

        while (TriLeftChild(Ptr) != NULL) {
            Ptr = TriLeftChild(Ptr);
        }

        return Ptr;

    }

    //
    //  we do not have a right child so check to see if have a parent and if
    //  so find the first ancestor that we are a left decendent of. That
    //  is find and return P in the following diagram
    //
    //                   P
    //                  /
    //                 .
    //                  .
    //                   .
    //                  Links
    //

    Ptr = Links;
    while (!TriIsLeftChild(Ptr) && !TriIsRoot(Ptr)) {  // (TriIsRightChild(Ptr)) {
        Ptr = TriParent(Ptr);
    }

    if (TriIsLeftChild(Ptr)) {
        return TriParent(Ptr);
    }

    //
    //  Otherwise we do not have a real successor so we simply return NULL
    //

    return NULL;

}


PTRI_SPLAY_LINKS
TriRealPredecessor (
    IN PTRI_SPLAY_LINKS Links
    )

/*++

Routine Description:

    This RealPredecessor function takes as input a pointer to a splay link in
    a tree and returns a pointer to the predecessor of the input node within
    the entire tree.  If there is not a predecessor, the return value is NULL.

Arguments:

    Links - Supplies the pointer to a splay link in a tree

Return Values:

    PRTI_SPLAY_LINKS - Returns a pointer to the predecessor in the entire tree

--*/

{
    PTRI_SPLAY_LINKS Ptr;

    //
    //  first check to see if there is a left subtree to the input link
    //  if there is then the real predecessor is the right most node in
    //  the left subtree.  That is find and return P in the following diagram
    //
    //              Links
    //               /
    //              .
    //               .
    //                .
    //                 P
    //                /
    //

    if ((Ptr = TriLeftChild(Links)) != NULL) {

        while (TriRightChild(Ptr) != NULL) {
            Ptr = TriRightChild(Ptr);
        }

        return Ptr;

    }

    //
    //  we do not have a left child so check to see if have a parent and if
    //  so find the first ancestor that we are a right decendent of. That
    //  is find and return P in the following diagram
    //
    //                   P
    //                    \
    //                     .
    //                    .
    //                   .
    //                Links
    //

    Ptr = Links;
    while (TriIsLeftChild(Ptr)) {
        Ptr = TriParent(Ptr);
    }

    if (!TriIsLeftChild(Ptr) && !TriIsRoot(Ptr)) { // (TriIsRightChild(Ptr)) {
        return TriParent(Ptr);
    }

    //
    //  Otherwise we do not have a real predecessor so we simply return NULL
    //

    return NULL;

}


PULONG
TriAddressOfBackRefViaParent (
    IN PTRI_SPLAY_LINKS Links
    )

{
    PTRI_SPLAY_LINKS Ptr;

    //
    //  If Links is the root then we do not have a back pointer via our parent
    //  so return NULL
    //

    if (TriIsRoot(Links)) {

        return NULL;

    }

    //
    //  We are not the root so find our parent and if our parent directly points
    //  to us we return the address of our parent's reference to us.  Otherwise
    //  (we must be a right child with a sibling) so return the address of
    //  our sibling's ParSib reference to us.
    //

    Ptr = TriParent(Links);
    if (MakeIntoPointer(Ptr->Refs.Child) == Links) {
        return &(Ptr->Refs.Child);
    } else {
        return &(MakeIntoPointer(Ptr->Refs.Child)->Refs.ParSib);
    }

}


PULONG
TriAddressOfBackRefViaChild (
    IN PTRI_SPLAY_LINKS Links
    )

{
    PTRI_SPLAY_LINKS Ptr;

    //
    //  Make Ptr be the same reference as found in our child field.
    //

    Ptr = MakeIntoPointer(Links->Refs.Child);

    //
    //  If our child pointer is null then we don't have a back pointer
    //  via our child so return NULL.
    //

    if (Ptr == NULL) {
        return NULL;

    //
    //  if our child directly reference's us (then we only have one child)
    //  return the address of the ParSib of our only child.
    //

    } else if (MakeIntoPointer(Ptr->Refs.ParSib) == Links) {
        return &(Ptr->Refs.ParSib);

    //
    //  otherwise we have two children so return the address of the ParSib
    //  of the second child.
    //

    } else {
        return &(MakeIntoPointer(Ptr->Refs.ParSib)->Refs.ParSib);

    }

}


VOID
TriSwapSplayLinks (
    IN PTRI_SPLAY_LINKS Link1,
    IN PTRI_SPLAY_LINKS Link2
    )

{
    PULONG Parent1ChildRef;
    PULONG Parent2ChildRef;

    PULONG Child1ParSibRef;
    PULONG Child2ParSibRef;

    //
    //  We have the following situation
    //
    //
    //         Parent1            Parent2
    //            |                  |
    //            |                  |
    //          Link1              Link2
    //           / \                / \
    //          /   \              /   \
    //        LC1   RC1          LC2   RC2
    //
    //  where one of the links can possibly be the root and one of the links
    //  can possibly be a direct child of the other, or can be connected
    //  via their sibling pointers.  Without loss of generality we'll make
    //  link2 be the possible and root and link1 be the possible child, or
    //  link2 have a parsib pointer to link1
    //

    if ((TriIsRoot(Link1)) ||
        (TriParent(Link2) == Link1) ||
        (MakeIntoPointer(Link1->Refs.ParSib) == Link2)) {

        SwapPointers(Link1, Link2);

    }

    //
    //  The cases we need to handle are
    //
    //  1. Link1 is not a child of link2, link2 is not the root, and they are not siblings
    //  2. Link1 is not a child of link2, link2 is not the root, and they are     siblings
    //
    //  3. Link1 is not a child of link2, link2 is     the root
    //
    //  4. Link1 is an only child of link2, and link2 is not the root
    //  5. Link1 is an only child of link2, and link2 is     the root
    //
    //  6. Link1 is a left child of link2 (has a sibling), and link2 is not the root
    //  7. Link1 is a left child of link2 (has a sibling), and link2 is     the root
    //
    //  8. Link1 is a right child of link2 (has a sibling), and link2 is not the root
    //  9. Link1 is a right child of link2 (has a sibling), and link2 is     the root
    //
    //  Each case will be handled separately
    //

    if (TriParent(Link1) != Link2) {

        if (!TriIsRoot(Link2)) {

            if (MakeIntoPointer(Link2->Refs.ParSib) != Link1) {

                //
                //  Case 1 - Link1 is not a child of link2,
                //           Link2 is not the root, and
                //           they are not siblings
                //

                Parent1ChildRef = TriAddressOfBackRefViaParent(Link1);
                Child1ParSibRef = TriAddressOfBackRefViaChild(Link1);
                Parent2ChildRef = TriAddressOfBackRefViaParent(Link2);
                Child2ParSibRef = TriAddressOfBackRefViaChild(Link2);
                SwapUlongs(Link1->Refs.Child, Link2->Refs.Child);
                SwapUlongs(Link1->Refs.ParSib, Link2->Refs.ParSib);
                SetRefViaPointer(Parent1ChildRef, Link2);
                SetRefViaPointer(Parent2ChildRef, Link1);
                SetRefViaPointer(Child1ParSibRef, Link2);
                SetRefViaPointer(Child2ParSibRef, Link1);

            } else {

                //
                //  Case 2 - Link1 is not a child of link2,
                //           Link2 is not the root, and
                //           they are siblings
                //

                Child1ParSibRef = TriAddressOfBackRefViaChild(Link1);
                Parent2ChildRef = TriAddressOfBackRefViaParent(Link2);
                Child2ParSibRef = TriAddressOfBackRefViaChild(Link2);
                SwapUlongs(Link1->Refs.Child, Link2->Refs.Child);
                SetRefViaPointer(Child1ParSibRef, Link2);
                SetRefViaPointer(Child2ParSibRef, Link1);
                *Parent2ChildRef = MakeIntoLeftChildRef(Link1);
                Link2->Refs.ParSib = Link1->Refs.ParSib;
                Link1->Refs.ParSib = MakeIntoSiblingRef(Link2);

            }

        } else {

            //
            //  Case 3 - Link1 is not a child of link2, and
            //           Link2 is the root
            //

            Parent1ChildRef = TriAddressOfBackRefViaParent(Link1);
            Child1ParSibRef = TriAddressOfBackRefViaChild(Link1);
            Child2ParSibRef = TriAddressOfBackRefViaChild(Link2);
            SwapUlongs(Link1->Refs.Child, Link2->Refs.Child);
            Link2->Refs.ParSib = Link1->Refs.ParSib;
            Link1->Refs.ParSib = MakeIntoParentRef(Link1);
            SetRefViaPointer(Child1ParSibRef, Link2);
            SetRefViaPointer(Child2ParSibRef, Link1);
            SetRefViaPointer(Parent1ChildRef, Link2);

        }

    } else { // TriParent(Link1) == Link2

        if (MakeIntoPointer(Link2->Refs.Child) == Link1 &&
            MakeIntoPointer(Link1->Refs.ParSib) == Link2) { // Link1 is an only child

            if (!TriIsRoot(Link2)) {

                //
                //  Case 4 - Link1 is an only child of link2, and
                //           Link2 is not the root
                //

                Child1ParSibRef = TriAddressOfBackRefViaChild(Link1);
                Parent2ChildRef = TriAddressOfBackRefViaParent(Link2);
                SetRefViaPointer(Child1ParSibRef, Link2);
                SetRefViaPointer(Parent2ChildRef, Link1);
                Link1->Refs.ParSib = Link2->Refs.ParSib;
                Link2->Refs.ParSib = MakeIntoParentRef(Link1);
                SwapRefsButKeepFlags(Link1->Refs.Child, Link2->Refs.Child);
                SetRefViaPointer(&Link1->Refs.Child, Link2);

            } else {

                //
                //  Case 5 - Link1 is an only child of link2, and
                //           Link2 is the root
                //

                Child1ParSibRef = TriAddressOfBackRefViaChild(Link1);
                SetRefViaPointer(Child1ParSibRef, Link2);
                Link1->Refs.ParSib = MakeIntoParentRef(Link1);
                Link2->Refs.ParSib = MakeIntoParentRef(Link1);
                SwapRefsButKeepFlags(Link1->Refs.Child, Link2->Refs.Child);
                SetRefViaPointer(&Link1->Refs.Child, Link2);

            }

        } else if (TriIsLeftChild(Link1)) {  // and link1 has a sibling

            if (!TriIsRoot(Link2)) {

                //
                //  Case 6 - Link1 is a left child of link2 (has a sibling), and
                //           Link2 is not the root
                //

                Child1ParSibRef = TriAddressOfBackRefViaChild(Link1);
                Parent2ChildRef = TriAddressOfBackRefViaParent(Link2);
                Child2ParSibRef = TriAddressOfBackRefViaChild(Link2);
                SetRefViaPointer(Child1ParSibRef, Link2);
                SetRefViaPointer(Parent2ChildRef, Link1);
                SetRefViaPointer(Child2ParSibRef, Link1);
                Link2->Refs.Child = Link1->Refs.Child;
                Link1->Refs.Child = MakeIntoLeftChildRef(Link2);
                SwapUlongs(Link1->Refs.ParSib, Link2->Refs.ParSib);

            } else {

                //
                //  Case 7 - Link1 is a left child of link2 (has a sibling), and
                //           Link2 is the root
                //

                Child1ParSibRef = TriAddressOfBackRefViaChild(Link1);
                Child2ParSibRef = TriAddressOfBackRefViaChild(Link2);
                SetRefViaPointer(Child1ParSibRef, Link2);
                SetRefViaPointer(Child2ParSibRef, Link1);
                Link2->Refs.Child = Link1->Refs.Child;
                Link1->Refs.Child = MakeIntoLeftChildRef(Link2);
                Link2->Refs.ParSib = Link1->Refs.ParSib;
                Link1->Refs.ParSib = MakeIntoParentRef(Link1);

            }

        } else { // TriIsRightChild(Link1) and Link1 has a sibling

            if (!TriIsRoot(Link2)) {

                //
                //  Case 8 - Link1 is a right child of link2 (has a sibling), and
                //           Link2 is not the root
                //

                Parent1ChildRef = TriAddressOfBackRefViaParent(Link1);
                Child1ParSibRef = TriAddressOfBackRefViaChild(Link1);
                Parent2ChildRef = TriAddressOfBackRefViaParent(Link2);
                SetRefViaPointer(Parent1ChildRef, Link2);
                SetRefViaPointer(Child1ParSibRef, Link2);
                SetRefViaPointer(Parent2ChildRef, Link1);
                SwapUlongs(Link1->Refs.Child, Link2->Refs.Child);
                Link1->Refs.ParSib = Link2->Refs.ParSib;
                Link2->Refs.ParSib = MakeIntoParentRef(Link1);

            } else {

                //
                //  Case 9 - Link1 is a right child of link2 (has a sibling), and
                //           Link2 is the root
                //

                Parent1ChildRef = TriAddressOfBackRefViaParent(Link1);
                Child1ParSibRef = TriAddressOfBackRefViaChild(Link1);
                SetRefViaPointer(Parent1ChildRef, Link2);
                SetRefViaPointer(Child1ParSibRef, Link2);
                SwapUlongs(Link1->Refs.Child, Link2->Refs.Child);
                Link1->Refs.ParSib = MakeIntoParentRef(Link1);
                Link1->Refs.ParSib = MakeIntoParentRef(Link1);

            }

        }

    }

}


VOID
TriRotateRight (
    IN PTRI_SPLAY_LINKS Links
    )

{
    BOOLEAN IsRoot;
    PULONG ParentChildRef;
    ULONG SavedParSibRef;
    PTRI_SPLAY_LINKS LeftChild;
    PTRI_SPLAY_LINKS a,b,c;

    //
    //  We perform the following rotation
    //
    //               -Links-       -LeftChild-
    //                 / \           /     \
    //        LeftChild   c   ==>   a       Links
    //         /     \                       / \
    //        a       b                     b   c
    //
    //  where Links is a possible root and a,b, and c are all optional.
    //  We will consider each combination of optional children individually
    //  and handle the case of the root when we set T's parsib pointer and
    //  the backpointer to T.
    //

    //
    //  First remember if we are the root and if not also remember our
    //  back ref via our parent.
    //

    if (TriIsRoot(Links)) {
        IsRoot = TRUE;
    } else {
        IsRoot = FALSE;
        ParentChildRef = TriAddressOfBackRefViaParent(Links);
        SavedParSibRef = Links->Refs.ParSib;
    }

    //
    //  Now we set LeftChild, a, b, and c, and then later check for the
    //  different combinations.  In the diagrams only those links that
    //  need to change are shown in the after part.
    //

    LeftChild = TriLeftChild(Links);
    a = TriLeftChild(LeftChild);
    b = TriRightChild(LeftChild);
    c = TriRightChild(Links);

    if        ((a != NULL) && (b != NULL) && (c != NULL)) {

        //
        //  Handle the following case
        //
        //              Links            LeftChild
        //               / \     ==>            \
        //      LeftChild   c            a ----- Links
        //       /     \                          /
        //      a       b                        b - c
        //

        a->Refs.ParSib = MakeIntoSiblingRef(Links);
        b->Refs.ParSib = MakeIntoSiblingRef(c);
        Links->Refs.Child = MakeIntoLeftChildRef(b);
        Links->Refs.ParSib = MakeIntoParentRef(LeftChild);

    } else if ((a != NULL) && (b != NULL) && (c == NULL)) {

        //
        //  Handle the following case
        //
        //              Links            LeftChild
        //               /       ==>            \
        //      LeftChild                a ----- Links
        //       /     \                          /
        //      a       b                        b --
        //

        a->Refs.ParSib = MakeIntoSiblingRef(Links);
        b->Refs.ParSib = MakeIntoParentRef(Links);
        Links->Refs.Child = MakeIntoLeftChildRef(b);
        Links->Refs.ParSib = MakeIntoParentRef(LeftChild);

    } else if ((a != NULL) && (b == NULL) && (c != NULL)) {

        //
        //  Handle the following case
        //
        //              Links            LeftChild
        //               / \     ==>            \
        //      LeftChild   c            a ----- Links
        //       /                                /
        //      a                                    c
        //

        a->Refs.ParSib = MakeIntoSiblingRef(Links);
        Links->Refs.Child = MakeIntoRightChildRef(c);
        Links->Refs.ParSib = MakeIntoParentRef(LeftChild);

    } else if ((a != NULL) && (b == NULL) && (c == NULL)) {

        //
        //  Handle the following case
        //
        //              Links            LeftChild
        //               /       ==>            \
        //      LeftChild                a ----- Links
        //       /                                /
        //      a
        //

        a->Refs.ParSib = MakeIntoSiblingRef(Links);
        Links->Refs.Child = 0L;
        Links->Refs.ParSib = MakeIntoParentRef(LeftChild);

    } else if ((a == NULL) && (b != NULL) && (c != NULL)) {

        //
        //  Handle the following case
        //
        //              Links            LeftChild
        //               / \     ==>      /     \
        //      LeftChild   c                    Links
        //             \                          /
        //              b                        b - c
        //

        b->Refs.ParSib = MakeIntoSiblingRef(c);
        Links->Refs.Child = MakeIntoLeftChildRef(b);
        Links->Refs.ParSib = MakeIntoParentRef(LeftChild);
        LeftChild->Refs.Child = MakeIntoRightChildRef(Links);

    } else if ((a == NULL) && (b != NULL) && (c == NULL)) {

        //
        //  Handle the following case
        //
        //              Links            LeftChild
        //               /       ==>      /     \
        //      LeftChild                        Links
        //             \                          /
        //              b                        b -
        //

        b->Refs.ParSib = MakeIntoParentRef(Links);
        Links->Refs.Child = MakeIntoLeftChildRef(b);
        Links->Refs.ParSib = MakeIntoParentRef(LeftChild);
        LeftChild->Refs.Child = MakeIntoRightChildRef(Links);

    } else if ((a == NULL) && (b == NULL) && (c != NULL)) {

        //
        //  Handle the following case
        //
        //              Links            LeftChild
        //               / \     ==>      /     \
        //      LeftChild   c                    Links
        //                                        /
        //                                           c
        //

        Links->Refs.Child = MakeIntoRightChildRef(c);
        Links->Refs.ParSib = MakeIntoParentRef(LeftChild);
        LeftChild->Refs.Child = MakeIntoRightChildRef(Links);

    } else if ((a == NULL) && (b == NULL) && (c == NULL)) {

        //
        //  Handle the following case
        //
        //              Links            LeftChild
        //               /       ==>      /     \
        //      LeftChild                        Links
        //                                        /
        //

        Links->Refs.Child = 0L;
        Links->Refs.ParSib = MakeIntoParentRef(LeftChild);
        LeftChild->Refs.Child = MakeIntoRightChildRef(Links);

    }

    if (IsRoot) {
        LeftChild->Refs.ParSib = MakeIntoParentRef(LeftChild);
    } else {
        LeftChild->Refs.ParSib = SavedParSibRef;
        SetRefViaPointer(ParentChildRef, LeftChild);
    }

}


VOID
TriRotateLeft (
    IN PTRI_SPLAY_LINKS Links
    )

{
    BOOLEAN IsRoot;
    PULONG ParentChildRef;
    ULONG SavedParSibRef;
    PTRI_SPLAY_LINKS RightChild;
    PTRI_SPLAY_LINKS a,b,c;

    //
    //  We perform the following rotation
    //
    //      -Links-                   -RightChild-
    //        / \                       /      \
    //       a   RightChild   ==>   Links       c
    //            /      \           / \
    //           b        c         a   b
    //
    //  where Links is a possible root and a,b, and c are all optional.
    //  We will consider each combination of optional children individually
    //  and handle the case of the root when we set T's parsib pointer and
    //  the backpointer to T.
    //

    //
    //  First remember if we are the root and if not also remember our
    //  back ref via our parent.
    //

    if (TriIsRoot(Links)) {
        IsRoot = TRUE;
    } else {
        IsRoot = FALSE;
        ParentChildRef = TriAddressOfBackRefViaParent(Links);
        SavedParSibRef = Links->Refs.ParSib;
    }

    //
    //  Now we set RightChild, a, b, and c, and then later check for the
    //  different combinations.  In the diagrams only those links that
    //  need to change are shown in the after part.
    //

    RightChild = TriRightChild(Links);
    a = TriLeftChild(Links);
    b = TriLeftChild(RightChild);
    c = TriRightChild(RightChild);

    if        ((a != NULL) && (b != NULL) && (c != NULL)) {

        //
        //  Handle the following case
        //
        //       Links                     RightChild
        //        / \                       /
        //       a   RightChild   ==>   Links ----- c
        //            /      \             \
        //           b        c         a - b
        //

        a->Refs.ParSib = MakeIntoSiblingRef(b);
        b->Refs.ParSib = MakeIntoParentRef(Links);
        Links->Refs.ParSib = MakeIntoSiblingRef(c);
        RightChild->Refs.Child = MakeIntoLeftChildRef(Links);

    } else if ((a != NULL) && (b != NULL) && (c == NULL)) {

        //
        //  Handle the following case
        //
        //       Links                     RightChild
        //        / \                       /
        //       a   RightChild   ==>   Links -----
        //            /                    \
        //           b                  a - b
        //

        a->Refs.ParSib = MakeIntoSiblingRef(b);
        b->Refs.ParSib = MakeIntoParentRef(Links);
        Links->Refs.ParSib = MakeIntoParentRef(RightChild);
        RightChild->Refs.Child = MakeIntoLeftChildRef(Links);

    } else if ((a != NULL) && (b == NULL) && (c != NULL)) {

        //
        //  Handle the following case
        //
        //       Links                     RightChild
        //        / \                       /
        //       a   RightChild   ==>   Links ----- c
        //                   \
        //                    c         a -
        //

        a->Refs.ParSib = MakeIntoParentRef(Links);
        Links->Refs.ParSib = MakeIntoSiblingRef(c);
        RightChild->Refs.Child = MakeIntoLeftChildRef(Links);

    } else if ((a != NULL) && (b == NULL) && (c == NULL)) {

        //
        //  Handle the following case
        //
        //       Links                     RightChild
        //        / \                       /
        //       a   RightChild   ==>   Links -----
        //
        //                              a -
        //

        a->Refs.ParSib = MakeIntoParentRef(Links);
        Links->Refs.ParSib = MakeIntoParentRef(RightChild);
        RightChild->Refs.Child = MakeIntoLeftChildRef(Links);

    } else if ((a == NULL) && (b != NULL) && (c != NULL)) {

        //
        //  Handle the following case
        //
        //       Links                     RightChild
        //          \                       /
        //           RightChild   ==>   Links ----- c
        //            /      \           / \
        //           b        c             b
        //

        b->Refs.ParSib = MakeIntoParentRef(Links);
        Links->Refs.Child = MakeIntoRightChildRef(b);
        Links->Refs.ParSib = MakeIntoSiblingRef(c);
        RightChild->Refs.Child = MakeIntoLeftChildRef(Links);

    } else if ((a == NULL) && (b != NULL) && (c == NULL)) {

        //
        //  Handle the following case
        //
        //       Links                     RightChild
        //          \                       /
        //           RightChild   ==>   Links -----
        //            /                  / \
        //           b                      b
        //

        b->Refs.ParSib = MakeIntoParentRef(Links);
        Links->Refs.Child = MakeIntoRightChildRef(b);
        Links->Refs.ParSib = MakeIntoParentRef(RightChild);
        RightChild->Refs.Child = MakeIntoLeftChildRef(Links);

    } else if ((a == NULL) && (b == NULL) && (c != NULL)) {

        //
        //  Handle the following case
        //
        //       Links                     RightChild
        //          \                       /
        //           RightChild   ==>   Links ----- c
        //                   \           /
        //                    c
        //

        Links->Refs.Child = 0L;
        Links->Refs.ParSib = MakeIntoSiblingRef(c);
        RightChild->Refs.Child = MakeIntoLeftChildRef(Links);

    } else if ((a == NULL) && (b == NULL) && (c == NULL)) {

        //
        //  Handle the following case
        //
        //       Links                     RightChild
        //          \                       /
        //           RightChild   ==>   Links -----
        //                               /
        //
        //

        Links->Refs.Child = 0L;
        Links->Refs.ParSib = MakeIntoParentRef(RightChild);
        RightChild->Refs.Child = MakeIntoLeftChildRef(Links);

    }

    if (IsRoot) {
        RightChild->Refs.ParSib = MakeIntoParentRef(RightChild);
    } else {
        RightChild->Refs.ParSib = SavedParSibRef;
        SetRefViaPointer(ParentChildRef, RightChild);
    }

}
