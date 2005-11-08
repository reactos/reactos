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
* @unimplemented
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
	UNIMPLEMENTED;
	return 0;
}


/*
* @implemented
*/
PRTL_SPLAY_LINKS NTAPI
RtlSubtreePredecessor (IN PRTL_SPLAY_LINKS Links)
{
   PRTL_SPLAY_LINKS Child;

   Child = Links->RightChild;
   if (Child == NULL)
      return NULL;

   if (Child->LeftChild == NULL)
      return Child;

   /* Get left-most child */
   while (Child->LeftChild != NULL)
      Child = Child->LeftChild;

   return Child;
}

/*
* @implemented
*/
PRTL_SPLAY_LINKS NTAPI
RtlSubtreeSuccessor (IN PRTL_SPLAY_LINKS Links)
{
   PRTL_SPLAY_LINKS Child;

   Child = Links->LeftChild;
   if (Child == NULL)
      return NULL;

   if (Child->RightChild == NULL)
      return Child;

   /* Get right-most child */
   while (Child->RightChild != NULL)
      Child = Child->RightChild;

   return Child;
}

/* EOF */
