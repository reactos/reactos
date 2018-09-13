/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Splay.c

Abstract:

    This module implements the general splay utilities

Author:

    Gary Kimura     [GaryKi]    23-May-1989

Environment:

    Pure utility routine

Revision History:

--*/

#include <nt.h>

#include <ntrtl.h>

#define SwapPointers(Ptr1, Ptr2) {      \
    PVOID _SWAP_POINTER_TEMP;           \
    _SWAP_POINTER_TEMP = (PVOID)(Ptr1); \
    (Ptr1) = (Ptr2);                    \
    (Ptr2) = _SWAP_POINTER_TEMP; \
    }

#define ParentsChildPointerAddress(Links) ( \
    RtlIsLeftChild(Links) ?                 \
        &(((Links)->Parent)->LeftChild)     \
    :                                       \
        &(((Links)->Parent)->RightChild)    \
    )

VOID
SwapSplayLinks (
    IN PRTL_SPLAY_LINKS Link1,
    IN PRTL_SPLAY_LINKS Link2
    );


PRTL_SPLAY_LINKS
RtlSplay (
    IN PRTL_SPLAY_LINKS Links
    )

/*++

Routine Description:

    The Splay function takes as input a pointer to a splay link in a tree
    and splays the tree.  Its function return value is a pointer to the
    root of the splayed tree.

Arguments:

    Links - Supplies a pointer to a splay link in a tree.

Return Value:

    PRTL_SPLAY_LINKS - returns a pointer to the root of the splayed tree.

--*/

{
    PRTL_SPLAY_LINKS L;
    PRTL_SPLAY_LINKS P;
    PRTL_SPLAY_LINKS G;

    //
    //  while links is not the root we need to keep rotating it toward
    //  the root
    //

    L = Links;

    while (!RtlIsRoot(L)) {

        P = RtlParent(L);
        G = RtlParent(P);

        if (RtlIsLeftChild(L)) {

            if (RtlIsRoot(P)) {

                /*
                  we have the following case

                          P           L
                         / \         / \
                        L   c  ==>  a   P
                       / \             / \
                      a   b           b   c
                */

                //
                //  Connect P & b
                //

                P->LeftChild = L->RightChild;
                if (P->LeftChild != NULL) {P->LeftChild->Parent = P;}

                //
                //  Connect L & P
                //

                L->RightChild = P;
                P->Parent = L;

                //
                //  Make L the root
                //

                L->Parent = L;

            } else if (RtlIsLeftChild(P)) {

                /*
                  we have the following case

                          |           |
                          G           L
                         / \         / \
                        P   d  ==>  a   P
                       / \             / \
                      L   c           b   G
                     / \                 / \
                    a   b               c   d
                */

                //
                //  Connect P & b
                //

                P->LeftChild = L->RightChild;
                if (P->LeftChild != NULL) {P->LeftChild->Parent = P;}

                //
                //  Connect G & c
                //

                G->LeftChild = P->RightChild;
                if (G->LeftChild != NULL) {G->LeftChild->Parent = G;}

                //
                //  Connect L & Great GrandParent
                //

                if (RtlIsRoot(G)) {
                    L->Parent = L;
                } else {
                    L->Parent = G->Parent;
                    *(ParentsChildPointerAddress(G)) = L;
                }

                //
                //  Connect L & P
                //

                L->RightChild = P;
                P->Parent = L;

                //
                //  Connect P & G
                //

                P->RightChild = G;
                G->Parent = P;

            } else { // RtlIsRightChild(Parent)

                /*
                  we have the following case

                        |                |
                        G                L
                       / \             /   \
                      a   P           G     P
                         / \         / \   / \
                        L   d  ==>  a   b c   d
                       / \
                      b   c
                */

                //
                //  Connect G & b
                //

                G->RightChild = L->LeftChild;
                if (G->RightChild != NULL) {G->RightChild->Parent = G;}

                //
                //  Connect P & c
                //

                P->LeftChild = L->RightChild;
                if (P->LeftChild != NULL) {P->LeftChild->Parent = P;}

                //
                //  Connect L & Great GrandParent
                //

                if (RtlIsRoot(G)) {
                    L->Parent = L;
                } else {
                    L->Parent = G->Parent;
                    *(ParentsChildPointerAddress(G)) = L;
                }

                //
                //  Connect L & G
                //

                L->LeftChild = G;
                G->Parent = L;

                //
                //  Connect L & P
                //

                L->RightChild = P;
                P->Parent = L;

            }

        } else { // RtlIsRightChild(L)

            if (RtlIsRoot(P)) {

                /*
                  we have the following case

                        P               L
                       / \             / \
                      a   L           P   c
                         / \         / \
                        b   c  ==>  a   b
                */

                //
                //  Connect P & b
                //

                P->RightChild = L->LeftChild;
                if (P->RightChild != NULL) {P->RightChild->Parent = P;}

                //
                //  Connect P & L
                //

                L->LeftChild = P;
                P->Parent = L;

                //
                //  Make L the root
                //

                L->Parent = L;

            } else if (RtlIsRightChild(P)) {

                /*
                  we have the following case

                      |                   |
                      G                   L
                     / \                 / \
                    a   P               P   d
                       / \             / \
                      b   L           G   c
                         / \         / \
                        c   d  ==>  a   b
                */

                //
                //  Connect G & b
                //

                G->RightChild = P->LeftChild;
                if (G->RightChild != NULL) {G->RightChild->Parent = G;}

                //
                //  Connect P & c
                //

                P->RightChild = L->LeftChild;
                if (P->RightChild != NULL) {P->RightChild->Parent = P;}

                //
                //  Connect L & Great GrandParent
                //

                if (RtlIsRoot(G)) {
                    L->Parent = L;
                } else {
                    L->Parent = G->Parent;
                    *(ParentsChildPointerAddress(G)) = L;
                }

                //
                //  Connect L & P
                //

                L->LeftChild = P;
                P->Parent = L;

                //
                //  Connect P & G
                //

                P->LeftChild = G;
                G->Parent = P;

            } else { // RtlIsLeftChild(P)

                /*
                  we have the following case

                          |              |
                          G              L
                         / \           /   \
                        P   d         P     G
                       / \           / \   / \
                      a   L    ==>  a   b c   d
                         / \
                        b   c
                */

                //
                //  Connect P & b
                //

                P->RightChild = L->LeftChild;
                if (P->RightChild != NULL) {P->RightChild->Parent = P;}

                //
                //  Connect G & c
                //

                G->LeftChild = L->RightChild;
                if (G->LeftChild != NULL) {G->LeftChild->Parent = G;}

                //
                //  Connect L & Great GrandParent
                //

                if (RtlIsRoot(G)) {
                    L->Parent = L;
                } else {
                    L->Parent = G->Parent;
                    *(ParentsChildPointerAddress(G)) = L;
                }

                //
                //  Connect L & P
                //

                L->LeftChild = P;
                P->Parent = L;

                //
                //  Connect L & G
                //

                L->RightChild = G;
                G->Parent = L;

            }
        }
    }

    return L;
}


PRTL_SPLAY_LINKS
RtlDelete (
    IN PRTL_SPLAY_LINKS Links
    )

/*++

Routine Description:

    The Delete function takes as input a pointer to a splay link in a tree
    and deletes that node from the tree.  Its function return value is a
    pointer to the root of the tree.  If the tree is now empty, the return
    value is NULL.

Arguments:

    Links - Supplies a pointer to a splay link in a tree.

Return Value:

    PRTL_SPLAY_LINKS - returns a pointer to the root of the tree.

--*/

{
    PRTL_SPLAY_LINKS Predecessor;
    PRTL_SPLAY_LINKS Parent;
    PRTL_SPLAY_LINKS Child;

    PRTL_SPLAY_LINKS *ParentChildPtr;

    //
    //  First check to see if Links as two children.  If it does then swap
    //  Links with its subtree predecessor.  Now we are guaranteed that Links
    //  has at most one child.
    //

    if ((RtlLeftChild(Links) != NULL) && (RtlRightChild(Links) != NULL)) {

        //
        //  get the predecessor, and swap their position in the tree
        //

        Predecessor = RtlSubtreePredecessor(Links);
        SwapSplayLinks(Predecessor, Links);

    }

    //
    //  If Links has no children then delete links by checking if it is
    //  already the root or has a parent.  If it is the root then the
    //  tree is now empty, otherwise it set the appropriate parent's child
    //  pointer (i.e., the one to links) to NULL, and splay the parent.
    //

    if ((RtlLeftChild(Links) == NULL) && (RtlRightChild(Links) == NULL)) {

        //
        //  Links has no children, if it is the root then return NULL
        //

        if (RtlIsRoot(Links)) {

            return NULL;
        }

        //
        //  Links as not children and is not the root, so to the parent's
        //  child pointer to NULL and splay the parent.
        //

        Parent = RtlParent(Links);

        ParentChildPtr = ParentsChildPointerAddress(Links);
        *ParentChildPtr = NULL;

        return RtlSplay(Parent);

    }

    //
    //  otherwise Links has one child.  If it is the root then make the child
    //  the new root, otherwise link together the child and parent, and splay
    //  the parent.  But first remember who our child is.
    //

    if (RtlLeftChild(Links) != NULL) {
        Child = RtlLeftChild(Links);
    } else {
        Child = RtlRightChild(Links);
    }

    //
    //  If links is the root then we make the child the root and return the
    //  child.
    //

    if (RtlIsRoot(Links)) {
        Child->Parent = Child;
        return Child;
    }

    //
    //  Links is not the root, so set link's parent child pointer to be
    //  the child and the set child's parent to be link's parent, and splay
    //  the parent.
    //

    ParentChildPtr = ParentsChildPointerAddress(Links);
    *ParentChildPtr = Child;
    Child->Parent = Links->Parent;

    return RtlSplay(RtlParent(Child));

}


VOID
RtlDeleteNoSplay (
    IN PRTL_SPLAY_LINKS Links,
    IN OUT PRTL_SPLAY_LINKS *Root
    )

/*++

Routine Description:

    The Delete function takes as input a pointer to a splay link in a tree,
    a pointer to the callers pointer to the tree and deletes that node from
    the tree.  The caller's pointer is updated upon return.  If the tree is
    now empty, the value is NULL.

    Unfortunately, the original RtlDelete() always splays and this is not
    always a desireable side-effect.

Arguments:

    Links - Supplies a pointer to a splay link in a tree.

    Root - Pointer to the callers pointer to the root 

Return Value:

    None

--*/

{
    PRTL_SPLAY_LINKS Predecessor;
    PRTL_SPLAY_LINKS Parent;
    PRTL_SPLAY_LINKS Child;

    PRTL_SPLAY_LINKS *ParentChildPtr;

    //
    //  First check to see if Links as two children.  If it does then swap
    //  Links with its subtree predecessor.  Now we are guaranteed that Links
    //  has at most one child.
    //

    if ((RtlLeftChild(Links) != NULL) && (RtlRightChild(Links) != NULL)) {

        //
        //  get the predecessor, and swap their position in the tree
        //

        Predecessor = RtlSubtreePredecessor(Links);

        if (RtlIsRoot(Links)) {

            //
            //  If we're switching with the root of the tree, fix the
            //  caller's root pointer
            //

            *Root = Predecessor;
        }

        SwapSplayLinks(Predecessor, Links);

    }

    //
    //  If Links has no children then delete links by checking if it is
    //  already the root or has a parent.  If it is the root then the
    //  tree is now empty, otherwise it set the appropriate parent's child
    //  pointer (i.e., the one to links) to NULL.
    //

    if ((RtlLeftChild(Links) == NULL) && (RtlRightChild(Links) == NULL)) {

        //
        //  Links has no children, if it is the root then set root to NULL
        //

        if (RtlIsRoot(Links)) {

            *Root = NULL;

            return;
        }

        //
        //  Links as not children and is not the root, so to the parent's
        //  child pointer to NULL.
        //

        ParentChildPtr = ParentsChildPointerAddress(Links);
        *ParentChildPtr = NULL;

        return;
    }

    //
    //  otherwise Links has one child.  If it is the root then make the child
    //  the new root, otherwise link together the child and parent. But first
    //  remember who our child is.
    //

    if (RtlLeftChild(Links) != NULL) {
        Child = RtlLeftChild(Links);
    } else {
        Child = RtlRightChild(Links);
    }

    //
    //  If links is the root then we make the child the root and return the
    //  child.
    //

    if (RtlIsRoot(Links)) {
        Child->Parent = Child;

        *Root = Child;

        return;
    }

    //
    //  Links is not the root, so set link's parent child pointer to be
    //  the child and the set child's parent to be link's parent.
    //

    ParentChildPtr = ParentsChildPointerAddress(Links);
    *ParentChildPtr = Child;
    Child->Parent = Links->Parent;

    return;
}


PRTL_SPLAY_LINKS
RtlSubtreeSuccessor (
    IN PRTL_SPLAY_LINKS Links
    )

/*++

Routine Description:

    The SubtreeSuccessor function takes as input a pointer to a splay link
    in a tree and returns a pointer to the successor of the input node of
    the substree rooted at the input node.  If there is not a successor, the
    return value is NULL.

Arguments:

    Links - Supplies a pointer to a splay link in a tree.

Return Value:

    PRTL_SPLAY_LINKS - returns a pointer to the successor in the subtree

--*/

{
    PRTL_SPLAY_LINKS Ptr;

    /*
      check to see if there is a right subtree to the input link
      if there is then the subtree successor is the left most node in
      the right subtree.  That is find and return P in the following diagram

                  Links
                     \
                      .
                     .
                    .
                   /
                  P
                   \
    */

    if ((Ptr = RtlRightChild(Links)) != NULL) {

        while (RtlLeftChild(Ptr) != NULL) {
            Ptr = RtlLeftChild(Ptr);
        }

        return Ptr;

    }

    //
    //  otherwise we are do not have a subtree successor so we simply return
    //  NULL
    //

    return NULL;

}


PRTL_SPLAY_LINKS
RtlSubtreePredecessor (
    IN PRTL_SPLAY_LINKS Links
    )

/*++

Routine Description:

    The SubtreePredecessor function takes as input a pointer to a splay link
    in a tree and returns a pointer to the predecessor of the input node of
    the substree rooted at the input node.  If there is not a predecessor,
    the return value is NULL.

Arguments:

    Links - Supplies a pointer to a splay link in a tree.

Return Value:

    PRTL_SPLAY_LINKS - returns a pointer to the predecessor in the subtree

--*/

{
    PRTL_SPLAY_LINKS Ptr;

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

    if ((Ptr = RtlLeftChild(Links)) != NULL) {

        while (RtlRightChild(Ptr) != NULL) {
            Ptr = RtlRightChild(Ptr);
        }

        return Ptr;

    }

    //
    //  otherwise we are do not have a subtree predecessor so we simply return
    //  NULL
    //

    return NULL;

}


PRTL_SPLAY_LINKS
RtlRealSuccessor (
    IN PRTL_SPLAY_LINKS Links
    )

/*++

Routine Description:

    The RealSuccessor function takes as input a pointer to a splay link
    in a tree and returns a pointer to the successor of the input node within
    the entire tree.  If there is not a successor, the return value is NULL.

Arguments:

    Links - Supplies a pointer to a splay link in a tree.

Return Value:

    PRTL_SPLAY_LINKS - returns a pointer to the successor in the entire tree

--*/

{
    PRTL_SPLAY_LINKS Ptr;

    /*
      first check to see if there is a right subtree to the input link
      if there is then the real successor is the left most node in
      the right subtree.  That is find and return P in the following diagram

                  Links
                     \
                      .
                     .
                    .
                   /
                  P
                   \
    */

    if ((Ptr = RtlRightChild(Links)) != NULL) {

        while (RtlLeftChild(Ptr) != NULL) {
            Ptr = RtlLeftChild(Ptr);
        }

        return Ptr;

    }

    /*
      we do not have a right child so check to see if have a parent and if
      so find the first ancestor that we are a left decendent of. That
      is find and return P in the following diagram

                       P
                      /
                     .
                      .
                       .
                      Links
    */

    Ptr = Links;
    while (RtlIsRightChild(Ptr)) {
        Ptr = RtlParent(Ptr);
    }

    if (RtlIsLeftChild(Ptr)) {
        return RtlParent(Ptr);
    }

    //
    //  otherwise we are do not have a real successor so we simply return
    //  NULL
    //

    return NULL;

}


PRTL_SPLAY_LINKS
RtlRealPredecessor (
    IN PRTL_SPLAY_LINKS Links
    )

/*++

Routine Description:

    The RealPredecessor function takes as input a pointer to a splay link
    in a tree and returns a pointer to the predecessor of the input node
    within the entire tree.  If there is not a predecessor, the return value
    is NULL.

Arguments:

    Links - Supplies a pointer to a splay link in a tree.

Return Value:

    PRTL_SPLAY_LINKS - returns a pointer to the predecessor in the entire tree

--*/

{
    PRTL_SPLAY_LINKS Ptr;

    /*
      first check to see if there is a left subtree to the input link
      if there is then the real predecessor is the right most node in
      the left subtree.  That is find and return P in the following diagram

                  Links
                   /
                  .
                   .
                    .
                     P
                    /
    */

    if ((Ptr = RtlLeftChild(Links)) != NULL) {

        while (RtlRightChild(Ptr) != NULL) {
            Ptr = RtlRightChild(Ptr);
        }

        return Ptr;

    }

    /*
      we do not have a left child so check to see if have a parent and if
      so find the first ancestor that we are a right decendent of. That
      is find and return P in the following diagram

                       P
                        \
                         .
                        .
                       .
                    Links
    */

    Ptr = Links;
    while (RtlIsLeftChild(Ptr)) {
        Ptr = RtlParent(Ptr);
    }

    if (RtlIsRightChild(Ptr)) {
        return RtlParent(Ptr);
    }

    //
    //  otherwise we are do not have a real predecessor so we simply return
    //  NULL
    //

    return NULL;

}


VOID
SwapSplayLinks (
    IN PRTL_SPLAY_LINKS Link1,
    IN PRTL_SPLAY_LINKS Link2
    )

{
    PRTL_SPLAY_LINKS *Parent1ChildPtr;
    PRTL_SPLAY_LINKS *Parent2ChildPtr;

    /*
      We have the following situation


             Parent1            Parent2
                |                  |
                |                  |
              Link1              Link2
               / \                / \
              /   \              /   \
            LC1   RC1          LC2   RC2

      where one of the links can possibly be the root and one of the links
      can possibly be a direct child of the other.  Without loss of
      generality we'll make link2 be the possible and root and link1 be
      the possible child.
    */

    if ((RtlIsRoot(Link1)) || (RtlParent(Link2) == Link1)) {
        SwapPointers(Link1, Link2);
    }

    //
    //  The four cases we need to handle are
    //
    //  1. Link1 is not a child of link2 and link2 is not the root
    //  2. Link1 is not a child of link2 and link2 is     the root
    //  3. Link1 is     a child of link2 and link2 is not the root
    //  4. Link1 is     a child of link2 and link2 is     the root
    //
    //
    //  Each case will be handled separately
    //

    if (RtlParent(Link1) != Link2) {

        if (!RtlIsRoot(Link2)) {

            //
            //  Case 1 the initial steps are:
            //
            //  1. get both parent child pointers
            //  2. swap the parent child pointers
            //  3. swap the parent pointers
            //

            Parent1ChildPtr = ParentsChildPointerAddress(Link1);
            Parent2ChildPtr = ParentsChildPointerAddress(Link2);

            SwapPointers(*Parent1ChildPtr, *Parent2ChildPtr);

            SwapPointers(Link1->Parent, Link2->Parent);

        } else {

            //
            //  Case 2 the initial steps are:
            //
            //  1. Set link1's parent child pointer to link2
            //  2. Set parent pointer of link2 to link1's parent
            //  3. Set parent pointer of link1 to be itself
            //

            Parent1ChildPtr = ParentsChildPointerAddress(Link1);
            *Parent1ChildPtr = Link2;

            Link2->Parent = Link1->Parent;

            Link1->Parent = Link1;

        }

        //
        //  Case 1 and 2 common steps are:
        //
        //  1. swap the child pointers
        //

        SwapPointers(Link1->LeftChild, Link2->LeftChild);
        SwapPointers(Link1->RightChild, Link2->RightChild);

    } else { // RtlParent(Link1) == Link2

        if (!RtlIsRoot(Link2)) {

            //
            //  Case 3 the initial steps are:
            //
            //  1. Set Link2's parent child pointer to link1
            //  2. Set Link1's parent pointer to parent of link2
            //

            Parent2ChildPtr = ParentsChildPointerAddress(Link2);
            *Parent2ChildPtr = Link1;

            Link1->Parent = Link2->Parent;

        } else {

            //
            //  Case 4 the initial steps are:
            //
            //  1. Set Link1's parent pointer to be link1
            //

            Link1->Parent = Link1;

        }

        //
        //  Case 3 and 4 common steps are:
        //
        //  1. Swap the child pointers
        //  2. if link1 was a left child (i.e., RtlLeftChild(Link1) == Link1)
        //     then set left child of link1 to link2
        //     else set right child of link1 to link2
        //

        SwapPointers(Link1->LeftChild, Link2->LeftChild);
        SwapPointers(Link1->RightChild, Link2->RightChild);

        if (Link1->LeftChild == Link1) {
            Link1->LeftChild = Link2;
        } else {
            Link1->RightChild = Link2;
        }

    }

    //
    //  Case 1, 2, 3, 4 common (and final) steps are:
    //
    //  1. Fix the parent pointers of the left and right children
    //

    if (Link1->LeftChild  != NULL) {Link1->LeftChild->Parent  = Link1;}
    if (Link1->RightChild != NULL) {Link1->RightChild->Parent = Link1;}
    if (Link2->LeftChild  != NULL) {Link2->LeftChild->Parent  = Link2;}
    if (Link2->RightChild != NULL) {Link2->RightChild->Parent = Link2;}

}
