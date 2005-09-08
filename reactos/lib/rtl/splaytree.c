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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
RtlSplay (
	PRTL_SPLAY_LINKS Links
	)
{
	UNIMPLEMENTED;
	return 0;
}


/*
* @implemented
*/
PRTL_SPLAY_LINKS STDCALL
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
PRTL_SPLAY_LINKS STDCALL
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
