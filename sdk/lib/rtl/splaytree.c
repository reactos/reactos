/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Splay-Tree implementation
 * FILE:              lib/rtl/splaytree.c
 * PROGRAMMER:        Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

//#define VERIFY_SWAP_SPLAY_LINKS

/* FUNCTIONS ***************************************************************/

static
VOID
FixupChildLinks(PRTL_SPLAY_LINKS Links, BOOLEAN Root, BOOLEAN LeftChild)
{
    if (RtlLeftChild(Links))
    {
        RtlInsertAsLeftChild(Links, RtlLeftChild(Links));
    }

    if (RtlRightChild(Links))
    {
        RtlInsertAsRightChild(Links, RtlRightChild(Links));
    }

    if (!Root)
    {
        if (LeftChild)
        {
            RtlInsertAsLeftChild(RtlParent(Links), Links);
        }
        else
        {
            RtlInsertAsRightChild(RtlParent(Links), Links);
        }
    }
}

/*

Given the tree:
   D
 B   F
A C E G

Swap(Q,S):

Q S   Q.P Q.L Q.R S.P S.L S.R
A C   S.P S.L S.R Q.P Q.L Q.R
B A   S   S.L S.R Q.P Q   Q.R
B C   S   S.L S.R Q.P Q.L Q
D A   S.P S.L S.R S   Q.L Q.R
D B   S   S.L S.R S   Q   Q.R
D F   S   S.L S.R S   Q.L Q

When Q is the immediate parent of S,
  Set Q's parent to S, and the proper child ptr of S to Q
When Q is the root,
  Set S's parent to S

*/

static
VOID
SwapSplayLinks(PRTL_SPLAY_LINKS LinkA,
               PRTL_SPLAY_LINKS LinkB)
{
    if (RtlParent(LinkA) == LinkB || RtlIsRoot(LinkB))
    {
        PRTL_SPLAY_LINKS Tmp = LinkA;
        LinkA = LinkB;
        LinkB = Tmp;
    }

    {
        RTL_SPLAY_LINKS Ta = *LinkA, Tb = *LinkB;
        BOOLEAN RootA = RtlIsRoot(LinkA),
                LeftA = RtlIsLeftChild(LinkA),
                LeftB = RtlIsLeftChild(LinkB);

        *LinkB = Ta; *LinkA = Tb;

        // A was parent of B is a special case: A->Parent is now B
        if (RtlParent(&Tb) == LinkA)
        {
            if (!RootA)
            {
                if (LeftA)
                {
                    RtlInsertAsLeftChild(RtlParent(&Ta), LinkB);
                }
                else
                {
                    RtlInsertAsRightChild(RtlParent(&Ta), LinkB);
                }
            }

            if (LeftB)
            {
                RtlInsertAsLeftChild(LinkB, LinkA);
            }
            else
            {
                RtlInsertAsRightChild(LinkB, LinkA);
            }
        }

        FixupChildLinks(LinkA, FALSE, LeftB);
        FixupChildLinks(LinkB, RootA, LeftA);

        // A was root is a special case: B->Parent is now B
        if (RootA)
            RtlParent(LinkB) = LinkB;

#ifdef VERIFY_SWAP_SPLAY_LINKS
        // Verify the distinct cases of node swap
        if (RootA)
        {
            if (RtlParent(&Tb) == LinkA)
            {
                // LinkA = D, LinkB = B
                // D B   S   S.L S.R S   Q   Q.R
                ASSERT(RtlParent(LinkA) == LinkB);
                ASSERT(RtlLeftChild(LinkA) == RtlLeftChild(&Tb));
                ASSERT(RtlRightChild(LinkA) == RtlRightChild(&Tb));
                ASSERT(RtlParent(LinkB) == LinkB);
                ASSERT(RtlLeftChild(LinkB) == (LeftB ? LinkA : RtlLeftChild(&Ta)));
                ASSERT(RtlRightChild(LinkB) == (LeftB ? RtlRightChild(&Ta) : LinkA));
            }
            else
            {
                // LinkA = D, LinkB = A
                // D A   S.P S.L S.R S   Q.L Q.R
                ASSERT(RtlParent(LinkA) == RtlParent(&Tb));
                ASSERT(RtlLeftChild(LinkA) == RtlLeftChild(&Tb));
                ASSERT(RtlRightChild(LinkA) == RtlRightChild(&Tb));
                ASSERT(RtlParent(LinkB) == LinkB);
                ASSERT(RtlLeftChild(LinkB) == RtlLeftChild(&Ta));
                ASSERT(RtlRightChild(LinkB) == RtlRightChild(&Ta));
            }
        }
        else
        {
            if (RtlParent(&Tb) == LinkA)
            {
                // LinkA = B, LinkB = A
                // B A   S   S.L S.R Q.P Q   Q.R
                ASSERT(RtlParent(LinkA) == LinkB);
                ASSERT(RtlLeftChild(LinkA) == RtlLeftChild(&Tb));
                ASSERT(RtlRightChild(LinkA) == RtlRightChild(&Tb));
                ASSERT(RtlParent(LinkB) == RtlParent(&Ta));
                ASSERT(RtlLeftChild(LinkB) == (LeftB ? LinkA : RtlLeftChild(&Ta)));
                ASSERT(RtlRightChild(LinkB) == (LeftB ? RtlRightChild(&Ta) : LinkA));
            }
            else
            {
                // LinkA = A, LinkB = C
                // A C   S.P S.L S.R Q.P Q.L Q.R
                ASSERT(!memcmp(LinkA, &Tb, sizeof(Tb)));
                ASSERT(!memcmp(LinkB, &Ta, sizeof(Ta)));
            }
        }
#endif
    }
}

/*
 * @implemented
 */
PRTL_SPLAY_LINKS
NTAPI
RtlDelete(PRTL_SPLAY_LINKS Links)
{
    PRTL_SPLAY_LINKS N, P, C, SP;
    N = Links;

    /* Check if we have two children */
    if (RtlLeftChild(N) && RtlRightChild(N))
    {
        /* Get the predecessor */
        SP = RtlSubtreePredecessor(N);

        /* Swap it with N, this will guarantee that N will only have a child */
        SwapSplayLinks(SP, N);
    }

    /* Check if we have no children */
    if (!RtlLeftChild(N) && !RtlRightChild(N))
    {
        /* If we are also the root, then the tree is gone */
        if (RtlIsRoot(N)) return NULL;

        /* Get our parent */
        P = RtlParent(N);

        /* Find out who is referencing us and delete the reference */
        if (RtlIsLeftChild(N))
        {
            /* N was a left child, so erase its parent's left child link */
            RtlLeftChild(P) = NULL;
        }
        else
        {
            /* N was a right child, so erase its parent's right child link */
            RtlRightChild(P) = NULL;
        }

        /* And finally splay the parent */
        return RtlSplay(P);
    }

    /* If we got here, we have a child (not two: we swapped above!) */
    if (RtlLeftChild(N))
    {
        /* We have a left child, so get it */
        C = RtlLeftChild(N);
    }
    else
    {
        /* We have a right child, get it instead */
        C = RtlRightChild(N);
    }

    /* Check if we are the root entry */
    if (RtlIsRoot(N))
    {
        /* Our child is now root, return it */
        RtlParent(C) = C;
        return C;
    }

    /* Get our parent */
    P = RtlParent(N);

    /* Find out who is referencing us and link to our child instead */
    if (RtlIsLeftChild(N))
    {
        /* N was a left child, so set its parent's left child as our child */
        RtlLeftChild(P) = C;
    }
    else
    {
        /* N was a right child, so set its parent's right child as our child */
        RtlRightChild(P) = C;
    }

    /* Finally, inherit our parent and splay the parent */
    RtlParent(C) = P;
    return RtlSplay(P);
}

/*
 * @implemented
 */
VOID
NTAPI
RtlDeleteNoSplay(PRTL_SPLAY_LINKS Links,
                 PRTL_SPLAY_LINKS *Root)
{
    PRTL_SPLAY_LINKS N, P, C, SP;
    N = Links;

    /* Check if we have two children */
    if (RtlLeftChild(N) && RtlRightChild(N))
    {
        /* Get the predecessor */
        SP = RtlSubtreePredecessor(N);

        /* If we are the root, the new root will be our predecessor after swapping */
        if (RtlIsRoot(N)) *Root = SP;

        /* Swap the predecessor with N, this will guarantee that N will only have a child */
        SwapSplayLinks(SP, N);
    }

    /* Check if we have no children */
    if (!RtlLeftChild(N) && !RtlRightChild(N))
    {
        /* If we are also the root, then the tree is gone */
        if (RtlIsRoot(N))
        {
            *Root = NULL;
            return;
        }

        /* Get our parent */
        P = RtlParent(N);

        /* Find out who is referencing us and delete the reference */
        if (RtlIsLeftChild(N))
        {
            /* N was a left child, so erase its parent's left child link */
            RtlLeftChild(P) = NULL;
        }
        else
        {
            /* N was a right child, so erase its parent's right child link */
            RtlRightChild(P) = NULL;
        }

        /* We are done */
        return;
    }

    /* If we got here, we have a child (not two: we swapped above!) */
    if (RtlLeftChild(N))
    {
        /* We have a left child, so get it */
        C = RtlLeftChild(N);
    }
    else
    {
        /* We have a right child, get it instead */
        C = RtlRightChild(N);
    }

    /* Check if we are the root entry */
    if (RtlIsRoot(N))
    {
        /* Our child is now root, return it */
        RtlParent(C) = C;
        *Root = C;
        return;
    }

    /* Get our parent */
    P = RtlParent(N);

    /* Find out who is referencing us and link to our child instead */
    if (RtlIsLeftChild(N))
    {
        /* N was a left child, so set its parent's left child as our child */
        RtlLeftChild(P) = C;
    }
    else
    {
        /* N was a right child, so set its parent's right child as our child */
        RtlRightChild(P) = C;
    }

    /* Finally, inherit our parent and we are done */
    RtlParent(C) = P;
    return;
}

/*
 * @implemented
 */
PRTL_SPLAY_LINKS
NTAPI
RtlRealPredecessor(PRTL_SPLAY_LINKS Links)
{
    PRTL_SPLAY_LINKS Child;

    /* Get the left child */
    Child = RtlLeftChild(Links);
    if (Child)
    {
        /* Get right-most child */
        while (RtlRightChild(Child)) Child = RtlRightChild(Child);
        return Child;
    }

    /* We don't have a left child, keep looping until we find our parent */
    Child = Links;
    while (RtlIsLeftChild(Child)) Child = RtlParent(Child);

    /* The parent should be a right child, return the real predecessor */
    if (RtlIsRightChild(Child)) return RtlParent(Child);

    /* The parent isn't a right child, so no real precessor for us */
    return NULL;
}

/*
 * @implemented
 */
PRTL_SPLAY_LINKS
NTAPI
RtlRealSuccessor(PRTL_SPLAY_LINKS Links)
{
    PRTL_SPLAY_LINKS Child;

    /* Get the right child */
    Child = RtlRightChild(Links);
    if (Child)
    {
        /* Get left-most child */
        while (RtlLeftChild(Child)) Child = RtlLeftChild(Child);
        return Child;
    }

    /* We don't have a right child, keep looping until we find our parent */
    Child = Links;
    while (RtlIsRightChild(Child)) Child = RtlParent(Child);

    /* The parent should be a left child, return the real successor */
    if (RtlIsLeftChild(Child)) return RtlParent(Child);

    /* The parent isn't a right child, so no real successor for us */
    return NULL;
}

/*
 * @implemented
 */
PRTL_SPLAY_LINKS
NTAPI
RtlSplay(PRTL_SPLAY_LINKS Links)
{
    /*
     * Implementation Notes (http://en.wikipedia.org/wiki/Splay_tree):
     *
     * To do a splay, we carry out a sequence of rotations,
     * each of which moves the target node N closer to the root.
     *
     * Each particular step depends on only two factors:
     *  - Whether N is the left or right child of its parent node, P,
     *  - Whether P is the left or right child of its parent, G (for grandparent node).
     *
     * Thus, there are four cases:
     *  - Case 1: N is the left child of P and P is the left child of G.
     *            In this case we perform a double right rotation, so that
     *            P becomes N's right child, and G becomes P's right child.
     *
     *  - Case 2: N is the right child of P and P is the right child of G.
     *            In this case we perform a double left rotation, so that
     *            P becomes N's left child, and G becomes P's left child.
     *
     *  - Case 3: N is the left child of P and P is the right child of G.
     *            In this case we perform a rotation so that
     *            G becomes N's left child, and P becomes N's right child.
     *
     *  - Case 4: N is the right child of P and P is the left child of G.
     *            In this case we perform a rotation so that
     *            P becomes N's left child, and G becomes N's right child.
     *
     * Finally, if N doesn't have a grandparent node, we simply perform a
     * left or right rotation to move it to the root.
     *
     * By performing a splay on the node of interest after every operation,
     * we keep recently accessed nodes near the root and keep the tree
     * roughly balanced, so that we achieve the desired amortized time bounds.
     */
    PRTL_SPLAY_LINKS N, P, G;

    /* N is the item we'll be playing with */
    N = Links;

    /* Let the algorithm run until N becomes the root entry */
    while (!RtlIsRoot(N))
    {
        /* Now get the parent and grand-parent */
        P = RtlParent(N);
        G = RtlParent(P);

        /* Case 1 & 3: N is left child of P */
        if (RtlIsLeftChild(N))
        {
            /* Case 1: P is the left child of G */
            if (RtlIsLeftChild(P))
            {
                /*
                 * N's right-child becomes P's left child and
                 * P's right-child becomes G's left child.
                 */
                RtlLeftChild(P) = RtlRightChild(N);
                RtlLeftChild(G) = RtlRightChild(P);

                /*
                 * If they exist, update their parent pointers too,
                 * since they've changed trees.
                 */
                if (RtlLeftChild(P)) RtlParent(RtlLeftChild(P)) = P;
                if (RtlLeftChild(G)) RtlParent(RtlLeftChild(G)) = G;

                /*
                 * Now we'll shove N all the way to the top.
                 * Check if G is the root first.
                 */
                if (RtlIsRoot(G))
                {
                    /* G doesn't have a parent, so N will become the root! */
                    RtlParent(N) = N;
                }
                else
                {
                    /* G has a parent, so inherit it since we take G's place */
                    RtlParent(N) = RtlParent(G);

                    /*
                     * Now find out who was referencing G and have it reference
                     * N instead, since we're taking G's place.
                     */
                    if (RtlIsLeftChild(G))
                    {
                        /*
                         * G was a left child, so change its parent's left
                         * child link to point to N now.
                         */
                        RtlLeftChild(RtlParent(G)) = N;
                    }
                    else
                    {
                        /*
                         * G was a right child, so change its parent's right
                         * child link to point to N now.
                         */
                        RtlRightChild(RtlParent(G)) = N;
                    }
                }

                /* Now N is on top, so P has become its child. */
                RtlRightChild(N) = P;
                RtlParent(P) = N;

                /* N is on top, P is its child, so G is grandchild. */
                RtlRightChild(P) = G;
                RtlParent(G) = P;
            }
            /* Case 3: P is the right child of G */
            else if (RtlIsRightChild(P))
            {
                /*
                 * N's left-child becomes G's right child and
                 * N's right-child becomes P's left child.
                 */
                RtlRightChild(G) = RtlLeftChild(N);
                RtlLeftChild(P) = RtlRightChild(N);

                /*
                 * If they exist, update their parent pointers too,
                 * since they've changed trees.
                 */
                if (RtlRightChild(G)) RtlParent(RtlRightChild(G)) = G;
                if (RtlLeftChild(P)) RtlParent(RtlLeftChild(P)) = P;

                /*
                 * Now we'll shove N all the way to the top.
                 * Check if G is the root first.
                 */
                if (RtlIsRoot(G))
                {
                    /* G doesn't have a parent, so N will become the root! */
                    RtlParent(N) = N;
                }
                else
                {
                    /* G has a parent, so inherit it since we take G's place */
                    RtlParent(N) = RtlParent(G);

                    /*
                     * Now find out who was referencing G and have it reference
                     * N instead, since we're taking G's place.
                     */
                    if (RtlIsLeftChild(G))
                    {
                        /*
                         * G was a left child, so change its parent's left
                         * child link to point to N now.
                         */
                        RtlLeftChild(RtlParent(G)) = N;
                    }
                    else
                    {
                        /*
                         * G was a right child, so change its parent's right
                         * child link to point to N now.
                         */
                        RtlRightChild(RtlParent(G)) = N;
                    }
                }

                /* Now N is on top, so G has become its left child. */
                RtlLeftChild(N) = G;
                RtlParent(G) = N;

                /* N is on top, G is its left child, so P is right child. */
                RtlRightChild(N) = P;
                RtlParent(P) = N;
            }
            /* "Finally" case: N doesn't have a grandparent => P is root */
            else
            {
                /* P's left-child becomes N's right child */
                RtlLeftChild(P) = RtlRightChild(N);

                /* If it exists, update its parent pointer too */
                if (RtlLeftChild(P)) RtlParent(RtlLeftChild(P)) = P;

                /* Now make N the root, no need to worry about references */
                N->Parent = N;

                /* And make P its right child */
                N->RightChild = P;
                P->Parent = N;
            }
        }
        /* Case 2 & 4: N is right child of P */
        else
        {
            /* Case 2: P is the right child of G */
            if (RtlIsRightChild(P))
            {
                /*
                 * P's left-child becomes G's right child and
                 * N's left-child becomes P's right child.
                 */
                RtlRightChild(G) = RtlLeftChild(P);
                RtlRightChild(P) = RtlLeftChild(N);

                /*
                 * If they exist, update their parent pointers too,
                 * since they've changed trees.
                 */
                if (RtlRightChild(G)) RtlParent(RtlRightChild(G)) = G;
                if (RtlRightChild(P)) RtlParent(RtlRightChild(P)) = P;

                /*
                 * Now we'll shove N all the way to the top.
                 * Check if G is the root first.
                 */
                if (RtlIsRoot(G))
                {
                    /* G doesn't have a parent, so N will become the root! */
                    RtlParent(N) = N;
                }
                else
                {
                    /* G has a parent, so inherit it since we take G's place */
                    RtlParent(N) = RtlParent(G);

                    /*
                     * Now find out who was referencing G and have it reference
                     * N instead, since we're taking G's place.
                     */
                    if (RtlIsLeftChild(G))
                    {
                        /*
                         * G was a left child, so change its parent's left
                         * child link to point to N now.
                         */
                        RtlLeftChild(RtlParent(G)) = N;
                    }
                    else
                    {
                        /*
                         * G was a right child, so change its parent's right
                         * child link to point to N now.
                         */
                        RtlRightChild(RtlParent(G)) = N;
                    }
                }

                /* Now N is on top, so P has become its child. */
                RtlLeftChild(N) = P;
                RtlParent(P) = N;

                /* N is on top, P is its child, so G is grandchild. */
                RtlLeftChild(P) = G;
                RtlParent(G) = P;
            }
            /* Case 4: P is the left child of G */
            else if (RtlIsLeftChild(P))
            {
                /*
                 * N's left-child becomes G's right child and
                 * N's right-child becomes P's left child.
                 */
                RtlRightChild(P) = RtlLeftChild(N);
                RtlLeftChild(G) = RtlRightChild(N);

                /*
                 * If they exist, update their parent pointers too,
                 * since they've changed trees.
                 */
                if (RtlRightChild(P)) RtlParent(RtlRightChild(P)) = P;
                if (RtlLeftChild(G)) RtlParent(RtlLeftChild(G)) = G;

                /*
                 * Now we'll shove N all the way to the top.
                 * Check if G is the root first.
                 */
                if (RtlIsRoot(G))
                {
                    /* G doesn't have a parent, so N will become the root! */
                    RtlParent(N) = N;
                }
                else
                {
                    /* G has a parent, so inherit it since we take G's place */
                    RtlParent(N) = RtlParent(G);

                    /*
                     * Now find out who was referencing G and have it reference
                     * N instead, since we're taking G's place.
                     */
                    if (RtlIsLeftChild(G))
                    {
                        /*
                         * G was a left child, so change its parent's left
                         * child link to point to N now.
                         */
                        RtlLeftChild(RtlParent(G)) = N;
                    }
                    else
                    {
                        /*
                         * G was a right child, so change its parent's right
                         * child link to point to N now.
                         */
                        RtlRightChild(RtlParent(G)) = N;
                    }
                }

                /* Now N is on top, so P has become its left child. */
                RtlLeftChild(N) = P;
                RtlParent(G) = N;

                /* N is on top, P is its left child, so G is right child. */
                RtlRightChild(N) = G;
                RtlParent(P) = N;
            }
            /* "Finally" case: N doesn't have a grandparent => P is root */
            else
            {
                /* P's right-child becomes N's left child */
                RtlRightChild(P) = RtlLeftChild(N);

                /* If it exists, update its parent pointer too */
                if (RtlRightChild(P)) RtlParent(RtlRightChild(P)) = P;

                /* Now make N the root, no need to worry about references */
                N->Parent = N;

                /* And make P its left child */
                N->LeftChild = P;
                P->Parent = N;
            }
        }
    }

    /* Return the root entry */
    ASSERT(RtlIsRoot(N));
    return N;
}

/*
 * @implemented
 */
PRTL_SPLAY_LINKS
NTAPI
RtlSubtreePredecessor(IN PRTL_SPLAY_LINKS Links)
{
    PRTL_SPLAY_LINKS Child;

    /* Get the left child */
    Child = RtlLeftChild(Links);
    if (!Child) return NULL;

    /* Get right-most child */
    while (RtlRightChild(Child)) Child = RtlRightChild(Child);

    /* Return it */
    return Child;
}

/*
 * @implemented
 */
PRTL_SPLAY_LINKS
NTAPI
RtlSubtreeSuccessor(IN PRTL_SPLAY_LINKS Links)
{
    PRTL_SPLAY_LINKS Child;

    /* Get the right child */
    Child = RtlRightChild(Links);
    if (!Child) return NULL;

    /* Get left-most child */
    while (RtlLeftChild(Child)) Child = RtlLeftChild(Child);

    /* Return it */
    return Child;
}

/* EOF */
