/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    addrsup.c

Abstract:

    This module contains the routine to manipulate the virtual address
    descriptor tree.

Author:

    Lou Perazzoli (loup) 19-May-1989

    Ripped off and modified from timersup.c
    The support for siblings was removed and a routine to locate
    the corresponding virtual address descriptor for a given address
    was added.

Environment:

    Kernel mode only, working set mutex held, APCs disabled.

Revision History:

--*/

#include "mi.h"

#if (_MSC_VER >= 800)
#pragma warning(disable:4010)           /* Allow pretty pictures without the noise */
#endif

VOID
MiReorderTree (
    IN PMMADDRESS_NODE Node,
    IN OUT PMMADDRESS_NODE *Root
    );


VOID
MiReorderTree (
    IN PMMADDRESS_NODE Node,
    IN OUT PMMADDRESS_NODE *Root
    )

/*++

Routine Description:

    This function reorders the Node tree by applying various splay functions
    to the tree. This is a local function that is called by the insert Node
    routine.

Arguments:

    Node - Supplies a pointer to a virtual address descriptor.

Return Value:

    None.

--*/

{
    PMMADDRESS_NODE GrandParent;
    PMMADDRESS_NODE Parent;
    PMMADDRESS_NODE SplayNode;

    //
    // Reorder Node tree to make it as balanced as possible with as little
    // work as possible.
    //

    SplayNode = Node;

    while (SplayNode != *Root) {

        Parent = SplayNode->Parent;
        if (Parent == *Root) {

            //
            // Splay node's parent is the root of the tree. Rotate the tree
            // left or right depending on whether the splay node is the left
            // of right child of its parent.
            //
            // Pictorially:
            //
            //            Right                 Left
            //
            //          P        X          P          X
            //         / \      / \        / \        / \
            //        X   C -> A   P      C   X  ->  P   A
            //       / \          / \        / \    / \
            //      A   B        B   C      B   A  C   B
            //

            *Root = SplayNode;
            SplayNode->Parent = (PMMADDRESS_NODE)NULL;
            Parent->Parent = SplayNode;
            if (SplayNode == Parent->LeftChild) {

                //
                // Splay node is the left child of its parent. Rotate tree
                // right.
                //

                Parent->LeftChild = SplayNode->RightChild;
                if (SplayNode->RightChild) {
                    SplayNode->RightChild->Parent = Parent;
                }
                SplayNode->RightChild = Parent;
            } else {

                //
                // Splay node is the right child of its parent. Rotate tree
                // left.
                //

                Parent->RightChild = SplayNode->LeftChild;
                if (SplayNode->LeftChild) {
                    SplayNode->LeftChild->Parent = Parent;
                }
                SplayNode->LeftChild = Parent;
            }
            break;
        } else {
            GrandParent = Parent->Parent;
            if ((SplayNode == Parent->LeftChild) &&
               (Parent == GrandParent->LeftChild)) {

                //
                // Both the splay node and the parent node are left children
                // of their parents. Rotate tree right and make the parent
                // the root of the new subtree.
                //
                // Pictorially:
                //
                //        G                 P
                //       / \              /   \
                //      P   D            X     G
                //     / \       ->     / \   / \
                //    X   C            A   B C   D
                //   / \
                //  A   B
                //

                if (GrandParent == *Root) {
                    *Root = Parent;
                    Parent->Parent = (PMMADDRESS_NODE)NULL;
                } else {
                    Parent->Parent = GrandParent->Parent;
                    if (GrandParent == GrandParent->Parent->LeftChild) {
                        GrandParent->Parent->LeftChild = Parent;
                    } else {
                        GrandParent->Parent->RightChild = Parent;
                    }
                }
                GrandParent->LeftChild = Parent->RightChild;
                if (Parent->RightChild) {
                    Parent->RightChild->Parent = GrandParent;
                }
                GrandParent->Parent = Parent;
                Parent->RightChild = GrandParent;
                SplayNode = Parent;
            } else if ((SplayNode == Parent->RightChild) &&
                      (Parent == GrandParent->RightChild)) {

                //
                // Both the splay node and the parent node are right children
                // of their parents. Rotate tree left and make the parent
                // the root of the new subtree.
                //
                // Pictorially:
                //
                //        G                 P
                //       / \              /   \
                //      D   P            G     X
                //         / \   ->     / \   / \
                //        C   X        D   C B   A
                //           / \
                //          A   B
                //

                if (GrandParent == *Root) {
                    *Root = Parent;
                    Parent->Parent = (PMMADDRESS_NODE)NULL;
                } else {
                    Parent->Parent = GrandParent->Parent;
                    if (GrandParent == GrandParent->Parent->LeftChild) {
                        GrandParent->Parent->LeftChild = Parent;
                    } else {
                        GrandParent->Parent->RightChild = Parent;
                    }
                }
                GrandParent->RightChild = Parent->LeftChild;
                if (Parent->LeftChild) {
                    Parent->LeftChild->Parent = GrandParent;
                }
                GrandParent->Parent = Parent;
                Parent->LeftChild = GrandParent;
                SplayNode = Parent;
            } else if ((SplayNode == Parent->LeftChild) &&
                      (Parent == GrandParent->RightChild)) {

                //
                // Splay node is the left child of its parent and parent is
                // the right child of its parent. Rotate tree left and make
                // splay node the root of the new subtree.
                //
                // Pictorially:
                //
                //      G                 X
                //     / \              /   \
                //    A   P            G     P
                //       / \    ->    / \   / \
                //      X   D        A   B C   D
                //     / \
                //    B   C
                //

                if (GrandParent == *Root) {
                    *Root = SplayNode;
                    SplayNode->Parent = (PMMADDRESS_NODE)NULL;
                } else {
                    SplayNode->Parent = GrandParent->Parent;
                    if (GrandParent == GrandParent->Parent->LeftChild) {
                        GrandParent->Parent->LeftChild = SplayNode;
                    } else {
                        GrandParent->Parent->RightChild = SplayNode;
                    }
                }
                Parent->LeftChild = SplayNode->RightChild;
                if (SplayNode->RightChild) {
                    SplayNode->RightChild->Parent = Parent;
                }
                GrandParent->RightChild = SplayNode->LeftChild;
                if (SplayNode->LeftChild) {
                    SplayNode->LeftChild->Parent = GrandParent;
                }
                Parent->Parent = SplayNode;
                GrandParent->Parent = SplayNode;
                SplayNode->LeftChild = GrandParent;
                SplayNode->RightChild = Parent;
            } else {

                //
                // Splay node is the right child of its parent and parent is
                // the left child of its parent. Rotate tree right and make
                // splay node the root of the new subtree.
                //
                // Pictorially:
                //
                //       G                 X
                //      / \              /   \
                //     P   A            P     G
                //    / \        ->    / \   / \
                //   D   X            D   C B   A
                //      / \
                //     C   B
                //

                if (GrandParent == *Root) {
                    *Root = SplayNode;
                    SplayNode->Parent = (PMMADDRESS_NODE)NULL;
                } else {
                    SplayNode->Parent = GrandParent->Parent;
                    if (GrandParent == GrandParent->Parent->LeftChild) {
                        GrandParent->Parent->LeftChild = SplayNode;
                    } else {
                        GrandParent->Parent->RightChild = SplayNode;
                    }
                }
                Parent->RightChild = SplayNode->LeftChild;
                if (SplayNode->LeftChild) {
                    SplayNode->LeftChild->Parent = Parent;
                }
                GrandParent->LeftChild = SplayNode->RightChild;
                if (SplayNode->RightChild) {
                    SplayNode->RightChild->Parent = GrandParent;
                }
                Parent->Parent = SplayNode;
                GrandParent->Parent = SplayNode;
                SplayNode->LeftChild = Parent;
                SplayNode->RightChild = GrandParent;
            }
        }
    }
    return;
}

PMMADDRESS_NODE
FASTCALL
MiGetNextNode (
    IN PMMADDRESS_NODE Node
    )

/*++

Routine Description:

    This function locates the virtual address descriptor which contains
    the address range which logically follows the specified address range.

Arguments:

    Node - Supplies a pointer to a virtual address descriptor.

Return Value:

    Returns a pointer to the virtual address descriptor containing the
    next address range, NULL if none.

--*/

{
    PMMADDRESS_NODE Next;
    PMMADDRESS_NODE Parent;
    PMMADDRESS_NODE Left;

    Next = Node;

    if (Next->RightChild == (PMMADDRESS_NODE)NULL) {

        while ((Parent = Next->Parent) != (PMMADDRESS_NODE)NULL) {

            //
            // Locate the first ancestor of this node of which this
            // node is the left child of and return that node as the
            // next element.
            //

            if (Parent->LeftChild == Next) {
                return Parent;
            }

            Next = Parent;

        }

        return (PMMADDRESS_NODE)NULL;
    }

    //
    // A right child exists, locate the left most child of that right child.
    //

    Next = Next->RightChild;

    while ((Left = Next->LeftChild) != (PMMADDRESS_NODE)NULL) {
        Next = Left;
    }
    return Next;

}

PMMADDRESS_NODE
FASTCALL
MiGetPreviousNode (
    IN PMMADDRESS_NODE Node
    )

/*++

Routine Description:

    This function locates the virtual address descriptor which contains
    the address range which logically precedes the specified virtual
    address descriptor.

Arguments:

    Node - Supplies a pointer to a virtual address descriptor.

Return Value:

    Returns a pointer to the virtual address descriptor containing the
    next address range, NULL if none.

--*/

{
    PMMADDRESS_NODE Previous;

    Previous = Node;

    if (Previous->LeftChild == (PMMADDRESS_NODE)NULL) {


        while (Previous->Parent != (PMMADDRESS_NODE)NULL) {

            //
            // Locate the first ancestor of this node of which this
            // node is the right child of and return that node as the
            // Previous element.
            //

            if (Previous->Parent->RightChild == Previous) {
                return Previous->Parent;
            }

            Previous = Previous->Parent;

        }
        return (PMMADDRESS_NODE)NULL;
    }

    //
    // A left child exists, locate the right most child of that left child.
    //

    Previous = Previous->LeftChild;
    while (Previous->RightChild != (PMMADDRESS_NODE)NULL) {
        Previous = Previous->RightChild;
    }
    return Previous;
}

PMMADDRESS_NODE
FASTCALL
MiGetFirstNode (
    IN PMMADDRESS_NODE Root
    )

/*++

Routine Description:

    This function locates the virtual address descriptor which contains
    the address range which logically is first within the address space.

Arguments:

    None.

Return Value:

    Returns a pointer to the virtual address descriptor containing the
    first address range, NULL if none.

--*/

{
    PMMADDRESS_NODE First;

    First = Root;

    if (First == (PMMADDRESS_NODE)NULL) {
        return (PMMADDRESS_NODE)NULL;
    }

    while (First->LeftChild != (PMMADDRESS_NODE)NULL) {
        First = First->LeftChild;
    }

    return First;
}

VOID
FASTCALL
MiInsertNode (
    IN PMMADDRESS_NODE Node,
    IN OUT PMMADDRESS_NODE *Root
    )

/*++

Routine Description:

    This function inserts a virtual address descriptor into the tree and
    reorders the splay tree as appropriate.

Arguments:

    Node - Supplies a pointer to a virtual address descriptor


Return Value:

    None.

--*/

{
    ULONG Level = 0;
    PMMADDRESS_NODE Parent;

    //
    // Initialize virtual address descriptor child links.
    //

    Node->LeftChild = (PMMADDRESS_NODE)NULL;
    Node->RightChild = (PMMADDRESS_NODE)NULL;

    //
    // If the tree is empty, then establish this virtual address descriptor
    // as the root of the tree.
    // Otherwise descend the tree to find the correct place to
    // insert the descriptor.
    //

    Parent = *Root;
    if (!Parent) {
        *Root = Node;
        Node->Parent = (PMMADDRESS_NODE)NULL;
    } else {

        for (;;) {

            Level += 1;
            if (Level == 15) {
                MiReorderTree(Parent, Root);
            }

            //
            // If the starting address for this virtual address descriptor
            // is less than the parent starting address, then
            // follow the left child link. Else follow the right child link.
            //

            if (Node->StartingVpn < Parent->StartingVpn) {

                //
                // Starting address of the virtual address descriptor is less
                // than the parent starting virtual address.
                // Follow left child link if not null. Otherwise
                // insert the descriptor as the left child of the parent and
                // reorder the tree.
                //

                if (Parent->LeftChild) {
                    Parent = Parent->LeftChild;
                } else {
                    Parent->LeftChild = Node;
                    Node->Parent = Parent;
                    // MiReorderTree(Node, Root);
                    break;
                }
            } else {

                //
                // Starting address of the virtual address descriptor is greater
                // than the parent starting virtual address.
                // Follow right child link if not null. Otherwise
                // insert the descriptor as the right child of the parent and
                // reorder the tree.
                //

                if (Parent->RightChild) {
                    Parent = Parent->RightChild;
                } else {
                    Parent->RightChild = Node;
                    Node->Parent = Parent;
                    // MiReorderTree(Node, Root);
                    break;
                }
            }
        }
    }
    return;
}

VOID
FASTCALL
MiRemoveNode (
    IN PMMADDRESS_NODE Node,
    IN OUT PMMADDRESS_NODE *Root
    )

/*++

Routine Description:

    This function removes a virtual address descriptor from the tree and
    reorders the splay tree as appropriate.

Arguments:

    Node - Supplies a pointer to a virtual address descriptor.

Return Value:

    None.

--*/

{

    PMMADDRESS_NODE LeftChild;
    PMMADDRESS_NODE RightChild;
    PMMADDRESS_NODE SplayNode;


    LeftChild = Node->LeftChild;
    RightChild = Node->RightChild;


    //
    // If the Node is the root of the tree, then establish new root. Else
    // isolate splay case and perform splay tree transformation.
    //

    if (Node == *Root) {

        //
        // This Node is the root of the tree. There are four cases to
        // handle:
        //
        //  1. the descriptor has no children
        //  2. the descriptor has a left child but no right child
        //  3. the descriptor has a right child but no left child
        //  4. the descriptor has both a right child and a left child
        //

        if (LeftChild) {
            if (RightChild) {

                //
                // The descriptor has both a left child and a right child.
                //

                if (LeftChild->RightChild) {

                    //
                    // The left child has a right child. Make the right most
                    // descendent of the right child of the left child the
                    // new root of the tree.
                    //
                    // Pictorially:
                    //
                    //      R          R
                    //      |          |
                    //      X          Z
                    //     / \        / \
                    //    A   B  ->  A   B
                    //     \          \
                    //      .          .
                    //       \
                    //        Z
                    //

                    SplayNode = LeftChild->RightChild;
                    while (SplayNode->RightChild) {
                        SplayNode = SplayNode->RightChild;
                    }
                    *Root = SplayNode;
                    SplayNode->Parent->RightChild = SplayNode->LeftChild;
                    if (SplayNode->LeftChild) {
                        SplayNode->LeftChild->Parent = SplayNode->Parent;
                    }
                    SplayNode->Parent = (PMMADDRESS_NODE)NULL;
                    LeftChild->Parent = SplayNode;
                    RightChild->Parent = SplayNode;
                    SplayNode->LeftChild = LeftChild;
                    SplayNode->RightChild = RightChild;
                } else if (RightChild->LeftChild) {

                    //
                    // The right child has a left child. Make the left most
                    // descendent of the left child of the right child the
                    // new root of the tree.
                    //
                    // Pictorially:
                    //
                    //      R          R
                    //      |          |
                    //      X          Z
                    //     / \        / \
                    //    A   B  ->  A   B
                    //       /          /
                    //      .          .
                    //     /
                    //    Z
                    //

                    SplayNode = RightChild->LeftChild;
                    while (SplayNode->LeftChild) {
                        SplayNode = SplayNode->LeftChild;
                    }
                    *Root = SplayNode;
                    SplayNode->Parent->LeftChild = SplayNode->RightChild;
                    if (SplayNode->RightChild) {
                        SplayNode->RightChild->Parent = SplayNode->Parent;
                    }
                    SplayNode->Parent = (PMMADDRESS_NODE)NULL;
                    LeftChild->Parent = SplayNode;
                    RightChild->Parent = SplayNode;
                    SplayNode->LeftChild = LeftChild;
                    SplayNode->RightChild = RightChild;
                } else {

                    //
                    // The left child of the descriptor does not have a right child,
                    // and the right child of the descriptor does not have a left
                    // child. Make the left child of the descriptor the new root of
                    // the tree.
                    //
                    // Pictorially:
                    //
                    //      R          R
                    //      |          |
                    //      X          A
                    //     / \        / \
                    //    A   B  ->  .   B
                    //   /          /
                    //  .
                    //

                    *Root = LeftChild;
                    LeftChild->Parent = (PMMADDRESS_NODE)NULL;
                    LeftChild->RightChild = RightChild;
                    LeftChild->RightChild->Parent = LeftChild;
                }
            } else {

                //
                // The descriptor has a left child, but does not have a right child.
                // Make the left child the new root of the tree.
                //
                // Pictorially:
                //
                //       R      R
                //       |      |
                //       X  ->  A
                //      /
                //     A
                //

                *Root = LeftChild;
                LeftChild->Parent = (PMMADDRESS_NODE)NULL;
            }
        } else if (RightChild) {

            //
            // The descriptor has a right child, but does not have a left child.
            // Make the right child the new root of the tree.
            //
            // Pictorially:
            //
            //       R         R
            //       |         |
            //       X    ->   A
            //        \
            //         A
            //

            *Root = RightChild;
            RightChild->Parent = (PMMADDRESS_NODE)NULL;
            while (RightChild->LeftChild) {
                RightChild = RightChild->LeftChild;
            }
        } else {

            //
            // The descriptor has neither a left child nor a right child. The
            // tree will be empty after removing the descriptor.
            //
            // Pictorially:
            //
            //      R      R
            //      |  ->
            //      X
            //

            *Root = NULL;
        }
    } else if (LeftChild) {
        if (RightChild) {

            //
            // The descriptor has both a left child and a right child.
            //

            if (LeftChild->RightChild) {

                //
                // The left child has a right child. Make the right most
                // descendent of the right child of the left child the new
                // root of the subtree.
                //
                // Pictorially:
                //
                //        P      P
                //       /        \
                //      X          X
                //     / \        / \
                //    A   B  or  A   B
                //     \          \
                //      .          .
                //       \          \
                //        Z          Z
                //
                //           |
                //           v
                //
                //        P      P
                //       /        \
                //      Z          Z
                //     / \        / \
                //    A   B  or  A   B
                //     \          \
                //      .          .
                //

                SplayNode = LeftChild->RightChild;
                while (SplayNode->RightChild) {
                    SplayNode = SplayNode->RightChild;
                }
                SplayNode->Parent->RightChild = SplayNode->LeftChild;
                if (SplayNode->LeftChild) {
                    SplayNode->LeftChild->Parent = SplayNode->Parent;
                }
                SplayNode->Parent = Node->Parent;
                if (Node == Node->Parent->LeftChild) {
                    Node->Parent->LeftChild = SplayNode;
                } else {
                    Node->Parent->RightChild = SplayNode;
                }
                LeftChild->Parent = SplayNode;
                RightChild->Parent = SplayNode;
                SplayNode->LeftChild = LeftChild;
                SplayNode->RightChild = RightChild;
            } else if (RightChild->LeftChild) {

                //
                // The right child has a left child. Make the left most
                // descendent of the left child of the right child the
                // new root of the subtree.
                //
                // Pictorially:
                //
                //        P      P
                //       /        \
                //      X          X
                //     / \        / \
                //    A   B  or  A   B
                //       /          /
                //      .          .
                //     /          /
                //    Z          Z
                //
                //           |
                //           v
                //
                //        P      P
                //       /        \
                //      Z          Z
                //     / \        / \
                //    A   B  or  A   B
                //       /          /
                //      .          .
                //

                SplayNode = RightChild->LeftChild;
                while (SplayNode->LeftChild) {
                    SplayNode = SplayNode->LeftChild;
                }
                SplayNode->Parent->LeftChild = SplayNode->RightChild;
                if (SplayNode->RightChild) {
                    SplayNode->RightChild->Parent = SplayNode->Parent;
                }
                SplayNode->Parent = Node->Parent;
                if (Node == Node->Parent->LeftChild) {
                    Node->Parent->LeftChild = SplayNode;
                } else {
                    Node->Parent->RightChild = SplayNode;
                }
                LeftChild->Parent = SplayNode;
                RightChild->Parent = SplayNode;
                SplayNode->LeftChild = LeftChild;
                SplayNode->RightChild = RightChild;
            } else {

                //
                // The left child of the descriptor does not have a right child,
                // and the right child of the descriptor does node have a left
                // child. Make the left child of the descriptor the new root of
                // the subtree.
                //
                // Pictorially:
                //
                //        P      P
                //       /        \
                //      X          X
                //     / \        / \
                //    A   B  or  A   B
                //   /          /
                //  .          .
                //
                //           |
                //           v
                //
                //        P      P
                //       /        \
                //      A          A
                //     / \        / \
                //    .   B  or  .   B
                //   /          /
                //

                SplayNode = LeftChild;
                SplayNode->Parent = Node->Parent;
                if (Node == Node->Parent->LeftChild) {
                    Node->Parent->LeftChild = SplayNode;
                } else {
                    Node->Parent->RightChild = SplayNode;
                }
                SplayNode->RightChild = RightChild;
                RightChild->Parent = SplayNode;
            }
        } else {

            //
            // The descriptor has a left child, but does not have a right child.
            // Make the left child the new root of the subtree.
            //
            // Pictorially:
            //
            //        P   P
            //       /     \
            //      X   or  X
            //     /       /
            //    A       A
            //
            //          |
            //          v
            //
            //        P   P
            //       /     \
            //      A       A
            //

            LeftChild->Parent = Node->Parent;
            if (Node == Node->Parent->LeftChild) {
                Node->Parent->LeftChild = LeftChild;
            } else {
                Node->Parent->RightChild = LeftChild;
            }
        }
    } else if (RightChild) {

        //
        // descriptor has a right child, but does not have a left child. Make
        // the right child the new root of the subtree.
        //
        // Pictorially:
        //
        //        P   P
        //       /     \
        //      X   or  X
        //       \       \
        //        A       A
        //
        //          |
        //          v
        //
        //        P   P
        //       /     \
        //      A       A
        //

        RightChild->Parent = Node->Parent;
        if (Node == Node->Parent->LeftChild) {
            Node->Parent->LeftChild = RightChild;
        } else {
            Node->Parent->RightChild = RightChild;
        }
    } else {

        //
        // The descriptor has neither a left child nor a right child. Delete
        // the descriptor from the tree and adjust its parent right or left
        // link.
        //
        // Pictorially:
        //
        //        P   P
        //       /     \
        //      X   or  X
        //
        //          |
        //          v
        //
        //        P   P
        //

        if (Node == Node->Parent->LeftChild) {
            Node->Parent->LeftChild = (PMMADDRESS_NODE)NULL;
        } else {
            Node->Parent->RightChild = (PMMADDRESS_NODE)NULL;
        }
    }
    return;
}

PMMADDRESS_NODE
FASTCALL
MiLocateAddressInTree (
    IN ULONG_PTR Vpn,
    IN PMMADDRESS_NODE *Root
    )

/*++

Routine Description:

    The function locates the virtual address descriptor which describes
    a given address.

Arguments:

    Vpn - Supplies the virtual page number to locate a descriptor
                     for.

Return Value:

    Returns a pointer to the virtual address descriptor which contains
    the supplied virtual address or NULL if none was located.

--*/

{

    PMMADDRESS_NODE Parent;
    ULONG Level = 0;

    Parent = *Root;

    for (;;) {

        if (Parent == (PMMADDRESS_NODE)NULL) {
            return (PMMADDRESS_NODE)NULL;
        }

        if (Level == 20) {

            //
            // There are 20 nodes above this point, reorder the
            // tree with this node as the root.
            //

            MiReorderTree(Parent, Root);
        }

        if (Vpn < Parent->StartingVpn) {
            Parent = Parent->LeftChild;
            Level += 1;

        } else if (Vpn > Parent->EndingVpn) {
            Parent = Parent->RightChild;
            Level += 1;

        } else {

            //
            // The address is within the start and end range.
            //

            return Parent;
        }
    }
}

PMMADDRESS_NODE
MiCheckForConflictingNode (
    IN ULONG_PTR StartVpn,
    IN ULONG_PTR EndVpn,
    IN PMMADDRESS_NODE Root
    )

/*++

Routine Description:

    The function determines if any addresses between a given starting and
    ending address is contained within a virtual address descriptor.

Arguments:

    StartVpn - Supplies the virtual address to locate a containing
                      descriptor.

    EndVpn - Supplies the virtual address to locate a containing
                      descriptor.

Return Value:

    Returns a pointer to the first conflicting virtual address descriptor
    if one is found, otherwise a NULL value is returned.

--*/

{
    PMMADDRESS_NODE Node;

    Node = Root;

    for (;;) {

        if (Node == (PMMADDRESS_NODE)NULL) {
            return (PMMADDRESS_NODE)NULL;
        }

        if (StartVpn > Node->EndingVpn) {
            Node = Node->RightChild;

        } else if (EndVpn < Node->StartingVpn) {
            Node = Node->LeftChild;

        } else {

            //
            // The starting address is less than or equal to the end VA
            // and the ending address is greater than or equal to the
            // start va.  Return this node.
            //

            return Node;
        }
    }
}

PVOID
MiFindEmptyAddressRangeInTree (
    IN SIZE_T SizeOfRange,
    IN ULONG_PTR Alignment,
    IN PMMADDRESS_NODE Root,
    OUT PMMADDRESS_NODE *PreviousVad
    )

/*++

Routine Description:

    The function examines the virtual address descriptors to locate
    an unused range of the specified size and returns the starting
    address of the range.

Arguments:

    SizeOfRange - Supplies the size in bytes of the range to locate.

    Alignment - Supplies the alignment for the address.  Must be
                 a power of 2 and greater than the page_size.

    Root - Supplies the root of the tree to search through.

    PreviousVad - Supplies the Vad which is before this the found
                  address range.

Return Value:

    Returns the starting address of a suitable range.

--*/

{
    PMMADDRESS_NODE Node;
    PMMADDRESS_NODE NextNode;
    ULONG_PTR AlignmentVpn;

    AlignmentVpn = Alignment >> PAGE_SHIFT;

    //
    // Locate the Node with the lowest starting address.
    //

    SizeOfRange = (SizeOfRange + (PAGE_SIZE - 1)) >> PAGE_SHIFT;
    ASSERT (SizeOfRange != 0);

    Node = Root;

    if (Node == (PMMADDRESS_NODE)NULL) {
        return MM_LOWEST_USER_ADDRESS;
    }
    while (Node->LeftChild != (PMMADDRESS_NODE)NULL) {
        Node = Node->LeftChild;
    }

    //
    // Check to see if a range exists between the lowest address VAD
    // and lowest user address.
    //

    if (Node->StartingVpn > MI_VA_TO_VPN (MM_LOWEST_USER_ADDRESS)) {
        if ( SizeOfRange <
            (Node->StartingVpn - MI_VA_TO_VPN (MM_LOWEST_USER_ADDRESS))) {

            *PreviousVad = NULL;
            return MM_LOWEST_USER_ADDRESS;
        }
    }

    for (;;) {

        NextNode = MiGetNextNode (Node);

        if (NextNode != (PMMADDRESS_NODE)NULL) {

            if (SizeOfRange <=
                ((ULONG_PTR)NextNode->StartingVpn -
                                MI_ROUND_TO_SIZE(1 + Node->EndingVpn,
                                                 AlignmentVpn))) {

                //
                // Check to ensure that the ending address aligned upwards
                // is not greater than the starting address.
                //

                if ((ULONG_PTR)NextNode->StartingVpn >
                        MI_ROUND_TO_SIZE(1 + Node->EndingVpn,
                                         AlignmentVpn)) {

                    *PreviousVad = Node;
                    return (PMMADDRESS_NODE)MI_ROUND_TO_SIZE(
                                   (ULONG_PTR)MI_VPN_TO_VA_ENDING(Node->EndingVpn),
                                          Alignment);
                }
            }

        } else {

            //
            // No more descriptors, check to see if this fits into the remainder
            // of the address space.
            //

            if ((((ULONG_PTR)Node->EndingVpn + MI_VA_TO_VPN(X64K)) <
                    MI_VA_TO_VPN (MM_HIGHEST_VAD_ADDRESS))
                        &&
                (SizeOfRange <=
                    ((ULONG_PTR)MM_HIGHEST_VAD_ADDRESS -
                         (ULONG_PTR)MI_ROUND_TO_SIZE(
                         (ULONG_PTR)MI_VPN_TO_VA(Node->EndingVpn), Alignment)))) {

                *PreviousVad = Node;
                return (PMMADDRESS_NODE)MI_ROUND_TO_SIZE(
                                    (ULONG_PTR)MI_VPN_TO_VA_ENDING(Node->EndingVpn),
                                    Alignment);
            } else {
                ExRaiseStatus (STATUS_NO_MEMORY);
            }
        }
        Node = NextNode;
    }
}

PVOID
MiFindEmptyAddressRangeDownTree (
    IN SIZE_T SizeOfRange,
    IN PVOID HighestAddressToEndAt,
    IN ULONG_PTR Alignment,
    IN PMMADDRESS_NODE Root
    )

/*++

Routine Description:

    The function examines the virtual address descriptors to locate
    an unused range of the specified size and returns the starting
    address of the range.  The function examines from the high
    addresses down and ensures that starting address is less than
    the specified address.

Arguments:

    SizeOfRange - Supplies the size in bytes of the range to locate.

    HighestAddressToEndAt - Supplies the virtual address that limits
                            the value of the ending address.  The ending
                            address of the located range must be less
                            than this address.

    Alignment - Supplies the alignment for the address.  Must be
                 a power of 2 and greater than the page_size.

    Root - Supplies the root of the tree to search through.

Return Value:

    Returns the starting address of a suitable range.

--*/

{
    PMMADDRESS_NODE Node;
    PMMADDRESS_NODE PreviousNode;
    ULONG_PTR AlignedEndingVa;
    PVOID OptimalStart;
    ULONG_PTR OptimalStartVpn;
    ULONG_PTR HighestVpn;
    ULONG_PTR AlignmentVpn;

    SizeOfRange = MI_ROUND_TO_SIZE (SizeOfRange, PAGE_SIZE);

    ASSERT (HighestAddressToEndAt != NULL);
    ASSERT (HighestAddressToEndAt <= (PVOID)((ULONG_PTR)MM_HIGHEST_VAD_ADDRESS + 1));

    HighestVpn = MI_VA_TO_VPN (HighestAddressToEndAt);

    //
    // Locate the Node with the highest starting address.
    //

    OptimalStart = (PVOID)(MI_ALIGN_TO_SIZE(
                           (((ULONG_PTR)HighestAddressToEndAt + 1) - SizeOfRange),
                           Alignment));
    Node = Root;

    if (Node == (PMMADDRESS_NODE)NULL) {

        //
        // The tree is empty, any range is okay.
        //

        return (PMMADDRESS_NODE)(OptimalStart);
    }

    //
    // See if an empty slot exists to hold this range, locate the largest
    // element in the tree.
    //

    while (Node->RightChild != (PMMADDRESS_NODE)NULL) {
        Node = Node->RightChild;
    }

    //
    // Check to see if a range exists between the highest address VAD
    // and the highest address to end at.
    //

    AlignedEndingVa = (ULONG_PTR)MI_ROUND_TO_SIZE ((ULONG_PTR)MI_VPN_TO_VA_ENDING (Node->EndingVpn),
                                               Alignment);

    if (AlignedEndingVa < (ULONG_PTR)HighestAddressToEndAt) {

        if ( SizeOfRange < ((ULONG_PTR)HighestAddressToEndAt - AlignedEndingVa)) {

            return (PMMADDRESS_NODE)(MI_ALIGN_TO_SIZE(
                                  ((ULONG_PTR)HighestAddressToEndAt - SizeOfRange),
                                  Alignment));
        }
    }

    //
    // Walk the tree backwards looking for a fit.
    //

    OptimalStartVpn = MI_VA_TO_VPN (OptimalStart);
    AlignmentVpn = MI_VA_TO_VPN (Alignment);

    for (;;) {

        PreviousNode = MiGetPreviousNode (Node);

        if (PreviousNode != (PMMADDRESS_NODE)NULL) {

            //
            // Is the ending Va below the top of the address to end at.
            //

            if (PreviousNode->EndingVpn < OptimalStartVpn) {
                if ((SizeOfRange >> PAGE_SHIFT) <=
                    ((ULONG_PTR)Node->StartingVpn -
                    (ULONG_PTR)MI_ROUND_TO_SIZE(1 + PreviousNode->EndingVpn,
                                            AlignmentVpn))) {

                    //
                    // See if the optimal start will fit between these
                    // two VADs.
                    //

                    if ((OptimalStartVpn > PreviousNode->EndingVpn) &&
                        (HighestVpn < Node->StartingVpn)) {
                        return (PMMADDRESS_NODE)(OptimalStart);
                    }

                    //
                    // Check to ensure that the ending address aligned upwards
                    // is not greater than the starting address.
                    //

                    if ((ULONG_PTR)Node->StartingVpn >
                            (ULONG_PTR)MI_ROUND_TO_SIZE(1 + PreviousNode->EndingVpn,
                                                    AlignmentVpn)) {

                        return (PMMADDRESS_NODE)MI_ALIGN_TO_SIZE(
                                            (ULONG_PTR)MI_VPN_TO_VA (Node->StartingVpn) - SizeOfRange,
                                            Alignment);
                    }
                }
            }
        } else {

            //
            // No more descriptors, check to see if this fits into the remainder
            // of the address space.
            //

            if (Node->StartingVpn > MI_VA_TO_VPN (MM_LOWEST_USER_ADDRESS)) {
                if ((SizeOfRange >> PAGE_SHIFT) <=
                    ((ULONG_PTR)Node->StartingVpn - MI_VA_TO_VPN (MM_LOWEST_USER_ADDRESS))) {

                    //
                    // See if the optimal start will fit between these
                    // two VADs.
                    //

                    if (HighestVpn < Node->StartingVpn) {
                        return (PMMADDRESS_NODE)(OptimalStart);
                    }

                    return (PMMADDRESS_NODE)MI_ALIGN_TO_SIZE(
                                  (ULONG_PTR)MI_VPN_TO_VA (Node->StartingVpn) - SizeOfRange,
                                  Alignment);
                }
            } else {
                ExRaiseStatus (STATUS_NO_MEMORY);
            }
        }
        Node = PreviousNode;
    }
}
#if DBG
VOID
NodeTreeWalk (
    PMMADDRESS_NODE Start
    )

{
    if (Start == (PMMADDRESS_NODE)NULL) {
        return;
    }

    NodeTreeWalk(Start->LeftChild);

    DbgPrint("Node at 0x%lx start 0x%lx  end 0x%lx \n",
                    (ULONG_PTR)Start, MI_VPN_TO_VA(Start->StartingVpn),
                    (ULONG_PTR)MI_VPN_TO_VA (Start->EndingVpn) | (PAGE_SIZE - 1));


    NodeTreeWalk(Start->RightChild);
    return;
}
#endif //DBG
