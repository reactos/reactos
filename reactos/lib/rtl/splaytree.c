/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Splay-Tree implementation
 * FILE:              lib/rtl/splaytree.c
 * PROGRAMMER:        
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

/*
* @unimplemented
*/
PRTL_SPLAY_LINKS
NTAPI
RtlDelete (
	PRTL_SPLAY_LINKS Links
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
VOID
NTAPI
RtlDeleteNoSplay (
	PRTL_SPLAY_LINKS Links,
	PRTL_SPLAY_LINKS *Root
	)
{
	UNIMPLEMENTED;
}


/*
* @unimplemented
*/
PRTL_SPLAY_LINKS
NTAPI
RtlRealPredecessor (
	PRTL_SPLAY_LINKS Links
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
PRTL_SPLAY_LINKS
NTAPI
RtlRealSuccessor (
	PRTL_SPLAY_LINKS Links
	)
{
	UNIMPLEMENTED;
	return 0;
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
                        RtlLeftChild(RtlParent(G)) = N;
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
                        RtlLeftChild(RtlParent(G)) = N;
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
                        RtlLeftChild(RtlParent(G)) = N;
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
                        RtlLeftChild(RtlParent(G)) = N;
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
