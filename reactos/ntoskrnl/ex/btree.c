/*
 *  ReactOS kernel
 *  Copyright (C) 1998-2002 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * PROJECT:         ReactOS kernel
 * FILE:            btree.c
 * PURPOSE:         Binary tree support
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      15-03-2002  CSH  Created
 */
#include <ddk/ntddk.h>
#include <internal/ex.h>

#define NDEBUG
#include <internal/debug.h>

/* DATA **********************************************************************/

typedef struct _BINARY_TREE_NODE
{
  struct _BINARY_TREE_NODE  * Parent;
  struct _BINARY_TREE_NODE  * LeftChild;
  struct _BINARY_TREE_NODE  * RightChild;
  PVOID  Key;
  PVOID  Value;
} BINARY_TREE_NODE, *PBINARY_TREE_NODE;

/* FUNCTIONS ****************************************************************/

#define ExpBinaryTreeRootNode(Tree)(((PBINARY_TREE) (Tree))->RootNode)
#define ExpIsExternalBinaryTreeNode(Node)(((Node)->LeftChild == NULL) && ((Node)->RightChild == NULL))
#define ExpIsInternalBinaryTreeNode(Node)(!ExpIsExternalBinaryTreeNode(Node))
#define ExpBinaryTreeNodeKey(Node)((Node)->Key)
#define ExpBinaryTreeNodeValue(Node)((Node)->Value)
#define ExpBinaryTreeParentNode(Node)((Node)->Parent)
#define ExpBinaryTreeLeftChildNode(Node)((Node)->LeftChild)
#define ExpBinaryTreeRightChildNode(Node)((Node)->RightChild)
#define ExpBinaryTreeNodeEqual(Equality)((Equality) == 0)
#define ExpBinaryTreeNodeLess(Equality)((Equality) < 0)
#define ExpBinaryTreeNodeMore(Equality)((Equality) > 0)


/*
 * Lock the binary tree 
 */
inline VOID
ExpLockBinaryTree(PBINARY_TREE Tree,
 PKIRQL OldIrql)
{
	if (Tree->UseNonPagedPool)
	  {
      KeAcquireSpinLock(&Tree->Lock.NonPaged, OldIrql);
	  }
	else
		{
      ExAcquireFastMutex(&Tree->Lock.Paged);
		}
}


/*
 * Unlock the binary tree 
 */
inline VOID
ExpUnlockBinaryTree(PBINARY_TREE Tree,
  PKIRQL OldIrql)
{
	if (Tree->UseNonPagedPool)
	  {
      KeReleaseSpinLock(&Tree->Lock.NonPaged, *OldIrql);
	  }
	else
		{
      ExReleaseFastMutex(&Tree->Lock.Paged);
		}
}


/*
 * Allocate resources for a new node and initialize it.
 */
inline PBINARY_TREE_NODE
ExpCreateBinaryTreeNode(PBINARY_TREE Tree,
  PBINARY_TREE_NODE Parent,
  PVOID Value)
{
  PBINARY_TREE_NODE Node;

	if (Tree->UseNonPagedPool)
	  {
      Node = (PBINARY_TREE_NODE) ExAllocateFromNPagedLookasideList(&Tree->List.NonPaged);	    
	  }
	else
		{
      Node = (PBINARY_TREE_NODE) ExAllocateFromPagedLookasideList(&Tree->List.Paged);
		}

  if (Node)
		{
	    ExpBinaryTreeParentNode(Node)     = Parent;
      ExpBinaryTreeLeftChildNode(Node)  = NULL;
      ExpBinaryTreeRightChildNode(Node) = NULL;
      ExpBinaryTreeNodeValue(Node)      = Value;
		}

  return Node;
}


/*
 * Release resources for the node.
 */
inline VOID
ExpDestroyBinaryTreeNode(PBINARY_TREE Tree,
  PBINARY_TREE_NODE  Node)
{
	if (Tree->UseNonPagedPool)
	  {
      ExFreeToNPagedLookasideList(&Tree->List.NonPaged, Node);
	  }
	else
		{
      ExFreeToPagedLookasideList(&Tree->List.Paged, Node);
		}
}


/*
 * Replaces a child node of a node with a new node.
 * The lock for the tree must be acquired when this routine is called.
 */
inline VOID
ExpBinaryTreeReplaceChildNode(PBINARY_TREE_NODE Child,
  PBINARY_TREE_NODE NewChild)
{
  if (ExpBinaryTreeLeftChildNode(ExpBinaryTreeParentNode(Child)) == Child)
    {
      ExpBinaryTreeLeftChildNode(ExpBinaryTreeParentNode(Child)) = NewChild;
    }
	else
		{
      ExpBinaryTreeRightChildNode(ExpBinaryTreeParentNode(Child)) = NewChild;
		}
}


/*
 * Returns the sibling node of a node.
 * The lock for the tree must be acquired when this routine is called.
 */
inline PBINARY_TREE_NODE
ExpSiblingBinaryTreeNode(PBINARY_TREE Tree,
  PBINARY_TREE_NODE Node)
{
  if (Node == ExpBinaryTreeRootNode(Tree))
		{
      return NULL;
		}
  else
    {
      if (ExpBinaryTreeLeftChildNode(ExpBinaryTreeParentNode(Node)) == Node)
		    {          
          return ExpBinaryTreeRightChildNode(ExpBinaryTreeParentNode(Node));
        }
			else
			  {
	        return ExpBinaryTreeLeftChildNode(ExpBinaryTreeParentNode(Node));
        }
		}
}


/*
 * Expands an external node to an internal node.
 * The lock for the tree must be acquired when this routine is called.
 */
VOID
ExpExpandExternalBinaryTreeNode(PBINARY_TREE Tree,
  PBINARY_TREE_NODE Node)
{
  ExpBinaryTreeLeftChildNode(Node) = ExpCreateBinaryTreeNode(Tree, Node, NULL);

  if (!ExpBinaryTreeLeftChildNode(Node))
		{
      /* FIXME: Throw exception */
      DbgPrint("ExpCreateBinaryTreeNode() failed\n");
		}

  ExpBinaryTreeRightChildNode(Node) = ExpCreateBinaryTreeNode(Tree, Node, NULL);

  if (!ExpBinaryTreeRightChildNode(Node))
		{
      ExpDestroyBinaryTreeNode(Tree, ExpBinaryTreeLeftChildNode(Node));
      /* FIXME: Throw exception */
      DbgPrint("ExpCreateBinaryTreeNode() failed\n");
		}
}


/*
 * Searches a tree for a node with the specified key. If a node with the
 * specified key is not found, the external node where it should be is
 * returned.
 * The lock for the tree must be acquired when this routine is called.
 */
inline PBINARY_TREE_NODE
ExpSearchBinaryTree(PBINARY_TREE  Tree,
  PVOID  Key,
  PBINARY_TREE_NODE  Node)
{
  LONG Equality;

  /* FIXME: Possibly do this iteratively due to the small kernel-mode stack */

  if (ExpIsExternalBinaryTreeNode(Node))
    {
      return Node;
    }

  Equality = (*Tree->Compare)(Key, ExpBinaryTreeNodeKey(Node));

  if (ExpBinaryTreeNodeEqual(Equality))
    {
      return Node;
    }

  if (ExpBinaryTreeNodeLess(Equality))
    {
      return ExpSearchBinaryTree(Tree, Key, ExpBinaryTreeLeftChildNode(Node));
    }

/*  if (ExpBinaryTreeNodeMore(Equality)) */
    {
      return ExpSearchBinaryTree(Tree, Key, ExpBinaryTreeRightChildNode(Node));
    }
}


/*
 * Removes an external node and it's parent node from the tree.
 * The lock for the tree must be acquired when this routine is called.
 */
VOID
ExpRemoveAboveExternalBinaryTreeNode(PBINARY_TREE Tree,
  PBINARY_TREE_NODE Node)
{
  assertmsg(ExpIsExternalBinaryTreeNode(Node), ("Node is not external"));

  if (Node == ExpBinaryTreeRootNode(Tree))
		{
      return;
		}
  else
		{
      PBINARY_TREE_NODE GrandParent;
	    PBINARY_TREE_NODE NewChild;

      GrandParent = ExpBinaryTreeParentNode(ExpBinaryTreeParentNode(Node));
	    NewChild = ExpSiblingBinaryTreeNode(Tree, Node);

      if (GrandParent != NULL)
        {
          ExpBinaryTreeReplaceChildNode(ExpBinaryTreeParentNode(Node), NewChild);
        }

	    ExpDestroyBinaryTreeNode(Tree, ExpBinaryTreeParentNode(Node));
	    ExpDestroyBinaryTreeNode(Tree, Node);
		}
}


/*
 * Release resources used by nodes of a binary (sub)tree.
 */
VOID
ExpDeleteBinaryTree(PBINARY_TREE Tree,
  PBINARY_TREE_NODE Node)
{
  /* FIXME: Possibly do this iteratively due to the small kernel-mode stack */

  if (ExpIsInternalBinaryTreeNode(Node))
    {
      ExpDeleteBinaryTree(Tree, ExpBinaryTreeLeftChildNode(Node));
      ExpDeleteBinaryTree(Tree, ExpBinaryTreeRightChildNode(Node));
    }

  ExpDestroyBinaryTreeNode(Tree, Node);
}


/*
 * Default key compare function. Compares the integer values of the two keys.
 */
LONG STDCALL
ExpBinaryTreeDefaultCompare(PVOID  Key1,
  PVOID  Key2)
{
  if (Key1 == Key2)
    return 0;

  return (((LONG_PTR) Key1 < (LONG_PTR) Key2) ? -1 : 1);
}


/*
 * Initializes a binary tree.
 */
BOOLEAN STDCALL
ExInitializeBinaryTree(IN PBINARY_TREE  Tree,
  IN PKEY_COMPARATOR  Compare,
  IN BOOLEAN  UseNonPagedPool)
{
  RtlZeroMemory(Tree, sizeof(BINARY_TREE));

  Tree->Compare = (Compare == NULL)
    ? ExpBinaryTreeDefaultCompare : Compare;

  Tree->UseNonPagedPool = UseNonPagedPool;

  if (UseNonPagedPool)
    {
		  ExInitializeNPagedLookasideList(
		    &Tree->List.NonPaged,           /* Lookaside list */
		    NULL,                           /* Allocate routine */
		    NULL,                           /* Free routine */
		    0,                              /* Flags */
		    sizeof(BINARY_TREE_NODE),       /* Size of each entry */
		    TAG('E','X','B','T'),           /* Tag */
		    0);                             /* Depth */

      KeInitializeSpinLock(&Tree->Lock.NonPaged);
		}
		else
		{
		  ExInitializePagedLookasideList(
		    &Tree->List.Paged,              /* Lookaside list */
		    NULL,                           /* Allocate routine */
		    NULL,                           /* Free routine */
		    0,                              /* Flags */
		    sizeof(BINARY_TREE_NODE),       /* Size of each entry */
		    TAG('E','X','B','T'),           /* Tag */
		    0);                             /* Depth */

      ExInitializeFastMutex(&Tree->Lock.Paged);
		}

  ExpBinaryTreeRootNode(Tree) = ExpCreateBinaryTreeNode(Tree, NULL, NULL);

  if (ExpBinaryTreeRootNode(Tree) == NULL)
		{
		  if (UseNonPagedPool)
		    {
          ExDeleteNPagedLookasideList(&Tree->List.NonPaged);
			  }
			else
				{
          ExDeletePagedLookasideList(&Tree->List.Paged);
				}
      return FALSE;
		}
  else
		{
      return TRUE;
		}
}


/*
 * Release resources used by a binary tree.
 */
VOID STDCALL
ExDeleteBinaryTree(IN PBINARY_TREE  Tree)
{
  /* Remove all nodes */
  ExpDeleteBinaryTree(Tree, ExpBinaryTreeRootNode(Tree));

  if (Tree->UseNonPagedPool)
    {
      ExDeleteNPagedLookasideList(&Tree->List.NonPaged);
	  }
	else
		{
      ExDeletePagedLookasideList(&Tree->List.Paged);
		}
}


/*
 * Insert a value in a binary tree.
 */
VOID STDCALL
ExInsertBinaryTree(IN PBINARY_TREE  Tree,
  IN PVOID  Key,
  IN PVOID  Value)
{
  PBINARY_TREE_NODE Node;
  KIRQL OldIrql;

  /* FIXME: Use SEH for error reporting */

  ExpLockBinaryTree(Tree, &OldIrql);
  Node = ExpBinaryTreeRootNode(Tree);
  do
    {
      Node = ExpSearchBinaryTree(Tree, Key, Node);

		  if (ExpIsExternalBinaryTreeNode(Node))
		    {
          break;
		    }
			else
				{
          Node = ExpBinaryTreeRightChildNode(Node);
				}
    } while (TRUE);
  ExpExpandExternalBinaryTreeNode(Tree, Node);
  ExpBinaryTreeNodeKey(Node)   = Key;
  ExpBinaryTreeNodeValue(Node) = Value;
  ExpUnlockBinaryTree(Tree, &OldIrql);
}


/*
 * Search for a value associated with a given key in a binary tree.
 */
BOOLEAN STDCALL
ExSearchBinaryTree(IN PBINARY_TREE  Tree,
  IN PVOID  Key,
  OUT PVOID  * Value)
{
  PBINARY_TREE_NODE Node;
  KIRQL OldIrql;

  ExpLockBinaryTree(Tree, &OldIrql);
  Node = ExpSearchBinaryTree(Tree, Key, ExpBinaryTreeRootNode(Tree));

  if (ExpIsInternalBinaryTreeNode(Node))
    {
	    *Value = ExpBinaryTreeNodeValue(Node);
      ExpUnlockBinaryTree(Tree, &OldIrql);
	    return TRUE;
	  }
	else
		{
      ExpUnlockBinaryTree(Tree, &OldIrql);
      return FALSE;
		}
}


/*
 * Remove a value associated with a given key from a binary tree.
 */
BOOLEAN STDCALL
ExRemoveBinaryTree(IN PBINARY_TREE  Tree,
  IN PVOID  Key,
  IN PVOID  * Value)
{
  PBINARY_TREE_NODE Node;
  KIRQL OldIrql;

  ExpLockBinaryTree(Tree, &OldIrql);

  Node = ExpSearchBinaryTree(Tree, Key, ExpBinaryTreeRootNode(Tree));

  if (ExpIsExternalBinaryTreeNode(Node))
		{
      ExpUnlockBinaryTree(Tree, &OldIrql);
      return FALSE;
		}
	else
		{
      *Value = ExpBinaryTreeNodeValue(Node);
		  if (ExpIsExternalBinaryTreeNode(ExpBinaryTreeLeftChildNode(Node)))
				{
          Node = ExpBinaryTreeLeftChildNode(Node);
				}
      else if (ExpIsExternalBinaryTreeNode(ExpBinaryTreeRightChildNode(Node)))
				{
          Node = ExpBinaryTreeRightChildNode(Node);
				}
      else
        {
          // Node has internal children
          PBINARY_TREE_NODE SwapNode;

          SwapNode = Node;
          Node = ExpBinaryTreeRightChildNode(SwapNode);
          do
            {
              Node = ExpBinaryTreeLeftChildNode(Node);
            } while (ExpIsInternalBinaryTreeNode(Node));
        }

      ExpRemoveAboveExternalBinaryTreeNode(Tree, Node);
      ExpUnlockBinaryTree(Tree, &OldIrql);
      return TRUE;
		}
}

/* EOF */
