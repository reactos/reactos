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
 * FILE:            stree.c
 * PURPOSE:         Splay tree support
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * NOTES:           Based on a splay tree implementation by
 *                  Daniel Stenberg <Daniel.Stenberg@sth.frontec.se>
 *                  http://www.contactor.se/~dast/stuff/dsplay-1.2.tar.gz
 * UPDATE HISTORY:
 *      15-03-2002  CSH  Created
 */
#include <ddk/ntddk.h>
#include <internal/ex.h>

#define NDEBUG
#include <internal/debug.h>

/* DATA **********************************************************************/

#define WEIGHT 1
#undef UNIQUE_KEYS

typedef struct _SPLAY_TREE_NODE
{
  /* Children on this branch that has smaller keys than this node */
  struct _SPLAY_TREE_NODE  * SmallerChild;

  /* Children on this branch that has larger keys than this node */
  struct _SPLAY_TREE_NODE  * LargerChild;

  /* Points to a node with identical key */
  struct _SPLAY_TREE_NODE  * Same;

  /* Key of this node */
  PVOID  Key;

  /* Value of this node */
  PVOID  Value;

  /* The number of nodes rooted here */
  LONG  Weight;
} SPLAY_TREE_NODE, *PSPLAY_TREE_NODE;

typedef struct _TRAVERSE_CONTEXT {
  PTRAVERSE_ROUTINE Routine;
  PVOID Context;
} TRAVERSE_CONTEXT, *PTRAVERSE_CONTEXT;

#define SPLAY_INDEX 0
#define SEARCH_INDEX 1
#define INSERT_INDEX 2
#define REMOVE_INDEX 3

typedef PSPLAY_TREE_NODE (*PSPLAY_TREE_SPLAY)(PSPLAY_TREE Tree,
  PVOID Key,
  PSPLAY_TREE_NODE Node);

typedef BOOLEAN (*PSPLAY_TREE_SEARCH)(PSPLAY_TREE Tree,
  PVOID Key,
  PSPLAY_TREE_NODE Node,
  PSPLAY_TREE_NODE * ReturnNode);

typedef PSPLAY_TREE_NODE (*PSPLAY_TREE_INSERT)(PSPLAY_TREE Tree,
  PVOID Key,
  PSPLAY_TREE_NODE Node,
  PSPLAY_TREE_NODE New);

typedef PSPLAY_TREE_NODE (*PSPLAY_TREE_REMOVE)(PSPLAY_TREE Tree,
  PVOID Key,
  PSPLAY_TREE_NODE Node,
  PSPLAY_TREE_NODE * RemovedNode);

/* FUNCTIONS ****************************************************************/

#define ExpSplayTreeRootNode(Tree)(((PSPLAY_TREE) (Tree))->RootNode)
#define ExpSplayTreeNodeKey(Node)((Node)->Key)
#define ExpSplayTreeNodeValue(Node)((Node)->Value)
#define ExpSplayTreeSmallerChildNode(Node)((Node)->SmallerChild)
#define ExpSplayTreeLargerChildNode(Node)((Node)->LargerChild)
#define ExpSplayTreeNodeEqual(Equality)((Equality) == 0)
#define ExpSplayTreeNodeLess(Equality)((Equality) < 0)
#define ExpSplayTreeNodeMore(Equality)((Equality) > 0)
#define ExpSplayTreeNodeSame(Node)((Node)->Same)
#define ExpSplayTreeNodeWeight(Node)((Node)->Weight)
#define ExpSplayTreeNodeGetWeight(Node)(((Node) == NULL) ? 0 : ((Node)->Weight))
#define ExpSplayTreeNodeSetWeight(Node, _Weight)((Node)->Weight = (_Weight))

#define KEY_NOTUSED (PVOID)-1


/*
 * Lock the splay tree 
 */
inline VOID
ExpLockSplayTree(PSPLAY_TREE Tree,
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
 * Unlock the splay tree 
 */
inline VOID
ExpUnlockSplayTree(PSPLAY_TREE Tree,
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
inline PSPLAY_TREE_NODE
ExpCreateSplayTreeNode(PSPLAY_TREE Tree,
  PVOID Value)
{
  PSPLAY_TREE_NODE Node;

	if (Tree->UseNonPagedPool)
	  {
      Node = (PSPLAY_TREE_NODE) ExAllocateFromNPagedLookasideList(&Tree->List.NonPaged);
	  }
	else
		{
      Node = (PSPLAY_TREE_NODE) ExAllocateFromPagedLookasideList(&Tree->List.Paged);
		}

  if (Node)
		{
      ExpSplayTreeSmallerChildNode(Node) = NULL;
      ExpSplayTreeLargerChildNode(Node)  = NULL;
      ExpSplayTreeNodeValue(Node)        = Value;
		}

  return Node;
}

/*
 * Release resources for the node.
 */
inline VOID
ExpDestroySplayTreeNode(PSPLAY_TREE Tree,
  PSPLAY_TREE_NODE Node)
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
 * Splay using the key 'Key' (which may or may not be in the tree). The starting
 * root is Node.
 * The lock for the tree must be acquired when this routine is called.
 * This routine does not maintain weight information.
 */
PSPLAY_TREE_NODE
ExpSplayTreeNoWeight(PSPLAY_TREE Tree,
  PVOID Key,
  PSPLAY_TREE_NODE Node)
{
  PSPLAY_TREE_NODE l;
  PSPLAY_TREE_NODE r;
  PSPLAY_TREE_NODE y;
  LONG ChildEquality;
  SPLAY_TREE_NODE N;
  LONG Equality;

  if (Node == NULL)
    return Node;

  ExpSplayTreeSmallerChildNode(&N) = NULL;
  ExpSplayTreeLargerChildNode(&N)  = NULL;
  l = &N;
  r = &N;

  for (;;)
    {
      Equality = (*Tree->Compare)(Key, ExpSplayTreeNodeKey(Node));

      if (ExpSplayTreeNodeLess(Equality))
        {
          if (ExpSplayTreeSmallerChildNode(Node) == NULL)
	          break;

          ChildEquality = (*Tree->Compare)(Key, ExpSplayTreeNodeKey(ExpSplayTreeSmallerChildNode(Node)));
          if (ExpSplayTreeNodeLess(ChildEquality))
            {
              y = ExpSplayTreeSmallerChildNode(Node);     /* Rotate smaller */
              ExpSplayTreeSmallerChildNode(Node) = ExpSplayTreeLargerChildNode(y);
              ExpSplayTreeLargerChildNode(y) = Node;

			        Node = y;
			        if (ExpSplayTreeSmallerChildNode(Node) == NULL)
			          break;
		        }

		      ExpSplayTreeSmallerChildNode(r) = Node;           /* Link smaller */
		      r = Node;
		      Node = ExpSplayTreeSmallerChildNode(Node);
		    }
		  else if (ExpSplayTreeNodeMore(Equality))
		    {
		      if (ExpSplayTreeLargerChildNode(Node) == NULL)
			      break;

		      ChildEquality = (*Tree->Compare)(Key, ExpSplayTreeNodeKey(ExpSplayTreeLargerChildNode(Node)));
		      if (ExpSplayTreeNodeMore(ChildEquality))
		        {
			        y = ExpSplayTreeLargerChildNode(Node);        /* Rotate larger */
			        ExpSplayTreeLargerChildNode(Node) = ExpSplayTreeSmallerChildNode(y);
			        ExpSplayTreeSmallerChildNode(y) = Node;
		
			        Node = y;
			        if (ExpSplayTreeLargerChildNode(Node) == NULL)
			          break;
		        }

		      ExpSplayTreeLargerChildNode(l) = Node;            /* Link larger */
		      l = Node;
		      Node = ExpSplayTreeLargerChildNode(Node);		
		    }
		  else
		    {
		      break;
		    }
    }

  ExpSplayTreeLargerChildNode(l)  = NULL;
  ExpSplayTreeSmallerChildNode(r) = NULL;

  ExpSplayTreeLargerChildNode(l)     = ExpSplayTreeSmallerChildNode(Node);  /* Assemble */
  ExpSplayTreeSmallerChildNode(r)    = ExpSplayTreeLargerChildNode(Node);
  ExpSplayTreeSmallerChildNode(Node) = ExpSplayTreeLargerChildNode(&N);
  ExpSplayTreeLargerChildNode(Node)  = ExpSplayTreeSmallerChildNode(&N);

  return Node;
}


/*
 * Splay using the key 'Key' (which may or may not be in the tree). The starting
 * root is Node.
 * The lock for the tree must be acquired when this routine is called.
 * This routine maintains weight information.
 */
PSPLAY_TREE_NODE
ExpSplayTreeWeight(PSPLAY_TREE Tree,
  PVOID Key,
  PSPLAY_TREE_NODE Node)
{
  PSPLAY_TREE_NODE l;
  PSPLAY_TREE_NODE r;
  PSPLAY_TREE_NODE y;
  LONG ChildEquality;
  SPLAY_TREE_NODE N;
  LONG Equality;
#ifdef WEIGHT
  LONG RootWeight;
  LONG Weight1;
  LONG Weight2;
#endif

  if (Node == NULL)
    return Node;

  ExpSplayTreeSmallerChildNode(&N) = NULL;
  ExpSplayTreeLargerChildNode(&N)  = NULL;
  l = &N;
  r = &N;

#ifdef WEIGHT
  RootWeight = ExpSplayTreeNodeGetWeight(Node);
  Weight1 = 0;
  Weight2 = 0;
#endif

  for (;;)
    {
      Equality = (*Tree->Compare)(Key, ExpSplayTreeNodeKey(Node));

      if (ExpSplayTreeNodeLess(Equality))
        {
          if (ExpSplayTreeSmallerChildNode(Node) == NULL)
	          break;

          ChildEquality = (*Tree->Compare)(Key, ExpSplayTreeNodeKey(ExpSplayTreeSmallerChildNode(Node)));
          if (ExpSplayTreeNodeLess(ChildEquality))
            {
              y = ExpSplayTreeSmallerChildNode(Node);     /* Rotate smaller */
              ExpSplayTreeSmallerChildNode(Node) = ExpSplayTreeLargerChildNode(y);
              ExpSplayTreeLargerChildNode(y) = Node;

#ifdef WEIGHT
              ExpSplayTreeNodeSetWeight(Node, ExpSplayTreeNodeGetWeight(ExpSplayTreeSmallerChildNode(Node))
                + ExpSplayTreeNodeGetWeight(ExpSplayTreeLargerChildNode(Node)) + 1);
#endif

			        Node = y;
			        if (ExpSplayTreeSmallerChildNode(Node) == NULL)
			          break;
		        }
		
		      ExpSplayTreeSmallerChildNode(r) = Node;           /* Link smaller */
		      r = Node;
		      Node = ExpSplayTreeSmallerChildNode(Node);
		
#ifdef WEIGHT
		      Weight2 += 1 + ExpSplayTreeNodeGetWeight(ExpSplayTreeLargerChildNode(r));
#endif
		    }
		  else if (ExpSplayTreeNodeMore(Equality))
		    {
		      if (ExpSplayTreeLargerChildNode(Node) == NULL)
			      break;
		
		      ChildEquality = (*Tree->Compare)(Key, ExpSplayTreeNodeKey(ExpSplayTreeLargerChildNode(Node)));
		      if (ExpSplayTreeNodeMore(ChildEquality))
		        {
			        y = ExpSplayTreeLargerChildNode(Node);        /* Rotate larger */
			        ExpSplayTreeLargerChildNode(Node) = ExpSplayTreeSmallerChildNode(y);
			        ExpSplayTreeSmallerChildNode(y) = Node;
		
#ifdef WEIGHT
			        ExpSplayTreeNodeSetWeight(Node, ExpSplayTreeNodeGetWeight(ExpSplayTreeSmallerChildNode(Node))
		            + ExpSplayTreeNodeGetWeight(ExpSplayTreeLargerChildNode(Node)) + 1);
#endif
		
			        Node = y;
			        if (ExpSplayTreeLargerChildNode(Node) == NULL)
			          break;
		        }
		
		      ExpSplayTreeLargerChildNode(l) = Node;            /* Link larger */
		      l = Node;
		      Node = ExpSplayTreeLargerChildNode(Node);
		
#ifdef WEIGHT
		      Weight1 += 1 + ExpSplayTreeNodeGetWeight(ExpSplayTreeSmallerChildNode(l));
#endif
		
		    }
		  else
		    {
		      break;
		    }
    }

#ifdef WEIGHT
  Weight1 += ExpSplayTreeNodeGetWeight(ExpSplayTreeSmallerChildNode(Node)); /* Now Weight1 and Weight2 are the weights of */
  Weight2 += ExpSplayTreeNodeGetWeight(ExpSplayTreeLargerChildNode(Node));  /* The 'smaller' and 'larger' trees we just built. */
  ExpSplayTreeNodeSetWeight(Node, Weight1 + Weight2 + 1);
#endif

  ExpSplayTreeLargerChildNode(l)  = NULL;
  ExpSplayTreeSmallerChildNode(r) = NULL;

#ifdef WEIGHT
  /* The following two loops correct the weight fields of the right path from
   * the left child of the root and the right path from the left child of the
   * root.
   */
  for (y = ExpSplayTreeLargerChildNode(&N); y != NULL; y = ExpSplayTreeLargerChildNode(y)) {
    ExpSplayTreeNodeSetWeight(y, Weight1);
    Weight1 -= 1 + ExpSplayTreeNodeGetWeight(ExpSplayTreeSmallerChildNode(y));
  }
  for (y = ExpSplayTreeSmallerChildNode(&N); y != NULL; y = ExpSplayTreeSmallerChildNode(y)) {
    ExpSplayTreeNodeSetWeight(y, Weight2);
    Weight2 -= 1 + ExpSplayTreeNodeGetWeight(ExpSplayTreeLargerChildNode(y));
  }
#endif

  ExpSplayTreeLargerChildNode(l)     = ExpSplayTreeSmallerChildNode(Node);  /* Assemble */
  ExpSplayTreeSmallerChildNode(r)    = ExpSplayTreeLargerChildNode(Node);
  ExpSplayTreeSmallerChildNode(Node) = ExpSplayTreeLargerChildNode(&N);
  ExpSplayTreeLargerChildNode(Node)  = ExpSplayTreeSmallerChildNode(&N);

  return Node;
}


inline PSPLAY_TREE_NODE
ExpSplayTree(PSPLAY_TREE Tree,
  PVOID Key,
  PSPLAY_TREE_NODE Node)
{
  return (*((PSPLAY_TREE_SPLAY)Tree->Reserved[SPLAY_INDEX]))(Tree, Key, Node);
}


/*
 * The lock for the tree must be acquired when this routine is called.
 * This routine does not maintain weight information.
 */
BOOLEAN
ExpSearchSplayTreeNoWeight(PSPLAY_TREE Tree,
  PVOID Key,
  PSPLAY_TREE_NODE Node,
  PSPLAY_TREE_NODE * ReturnNode)
{
  LONG Equality;

  if (Node == NULL)
    return FALSE;

  Node = ExpSplayTree(Tree, Key, Node);

  Equality = (*Tree->Compare)(Key, ExpSplayTreeNodeKey(Node));
  if (ExpSplayTreeNodeEqual(Equality))
    {
      /* Found the key */

      *ReturnNode = Node;
		  return TRUE;
	  }
	else
	  {
	    *ReturnNode = NULL;                 /* No match */
	    return FALSE;                       /* It wasn't there */
	  }
}


/*
 * The lock for the tree must be acquired when this routine is called.
 * This routine maintains weight information.
 */
BOOLEAN
ExpSearchSplayTreeWeight(PSPLAY_TREE Tree,
  PVOID Key,
  PSPLAY_TREE_NODE Node,
  PSPLAY_TREE_NODE * ReturnNode)
{
  PSPLAY_TREE_NODE x = NULL;
  LONG Equality;
#ifdef WEIGHT
  LONG tweight;
#endif

  if (Node == NULL)
    return FALSE;

#ifdef WEIGHT
  tweight = ExpSplayTreeNodeGetWeight(Node);
#endif

  Node = ExpSplayTree(Tree, Key, Node);

  Equality = (*Tree->Compare)(Key, ExpSplayTreeNodeKey(Node));
  if (ExpSplayTreeNodeEqual(Equality))
    {
      /* Found the key */

#ifdef WEIGHT
		  if (x != NULL)
		    {
		      ExpSplayTreeNodeSetWeight(x, tweight - 1);
		    }
#endif

      *ReturnNode = Node;
		  return TRUE;
	  }
	else
	  {
	    *ReturnNode = NULL;                 /* No match */
	    return FALSE;                       /* It wasn't there */
	  }
}


inline BOOLEAN
ExpSearchSplayTree(PSPLAY_TREE Tree,
  PVOID Key,
  PSPLAY_TREE_NODE Node,
  PSPLAY_TREE_NODE * ReturnNode)
{
  return (*((PSPLAY_TREE_SEARCH)Tree->Reserved[SEARCH_INDEX]))(Tree, Key, Node, ReturnNode);
}


/*
 * The lock for the tree must be acquired when this routine is called.
 * This routine does not maintain weight information.
 */
PSPLAY_TREE_NODE
ExpInsertSplayTreeNoWeight(PSPLAY_TREE Tree,
  PVOID Key,
  PSPLAY_TREE_NODE Node,
  PSPLAY_TREE_NODE New)
{
  if (New == NULL)
    return Node;

  if (Node != NULL)
    {
      LONG Equality;

      Node = ExpSplayTree(Tree, Key, Node);

      Equality = (*Tree->Compare)(Key, ExpSplayTreeNodeKey(Node));
      if (ExpSplayTreeNodeEqual(Equality))
        {

#ifdef UNIQUE_KEYS

      /* This is how to prevent the same node key getting added twice */
      return NULL; 

#else

      /* It already exists one of this size */

      ExpSplayTreeNodeSame(New) = Node;
      ExpSplayTreeNodeKey(New) = Key;
      ExpSplayTreeSmallerChildNode(New) = ExpSplayTreeSmallerChildNode(Node);
      ExpSplayTreeLargerChildNode(New) = ExpSplayTreeLargerChildNode(Node);

      ExpSplayTreeSmallerChildNode(Node) = New;
      ExpSplayTreeNodeKey(Node) = KEY_NOTUSED;

      /* New root node */
      return New;

#endif

    }
  }

  if (Node == NULL)
    {
      ExpSplayTreeSmallerChildNode(New) = NULL;
      ExpSplayTreeLargerChildNode(New)  = NULL;
    }
  else if (ExpSplayTreeNodeLess((*Tree->Compare)(Key, ExpSplayTreeNodeKey(Node))))
    {
      ExpSplayTreeSmallerChildNode(New)  = ExpSplayTreeSmallerChildNode(Node);
      ExpSplayTreeLargerChildNode(New)   = Node;
      ExpSplayTreeSmallerChildNode(Node) = NULL;
    }
  else
    {
      ExpSplayTreeLargerChildNode(New)  = ExpSplayTreeLargerChildNode(Node);
      ExpSplayTreeSmallerChildNode(New) = Node;
      ExpSplayTreeLargerChildNode(Node) = NULL;
    }

  ExpSplayTreeNodeKey(New) = Key;

#ifndef UNIQUE_KEYS
  /* No identical nodes (yet) */
  ExpSplayTreeNodeSame(New) = NULL;
#endif

  return New;
}


/*
 * The lock for the tree must be acquired when this routine is called.
 * This routine maintains weight information.
 */
PSPLAY_TREE_NODE
ExpInsertSplayTreeWeight(PSPLAY_TREE Tree,
  PVOID Key,
  PSPLAY_TREE_NODE Node,
  PSPLAY_TREE_NODE New)
{
  if (New == NULL)
    return Node;

  if (Node != NULL)
    {
      LONG Equality;

      Node = ExpSplayTree(Tree, Key, Node);

      Equality = (*Tree->Compare)(Key, ExpSplayTreeNodeKey(Node));
      if (ExpSplayTreeNodeEqual(Equality))
        {

#ifdef UNIQUE_KEYS

      /* This is how to prevent the same node key getting added twice */
      return NULL; 

#else

      /* It already exists one of this size */

      ExpSplayTreeNodeSame(New) = Node;
      ExpSplayTreeNodeKey(New) = Key;
      ExpSplayTreeSmallerChildNode(New) = ExpSplayTreeSmallerChildNode(Node);
      ExpSplayTreeLargerChildNode(New) = ExpSplayTreeLargerChildNode(Node);

#ifdef WEIGHT
      ExpSplayTreeNodeSetWeight(New, ExpSplayTreeNodeGetWeight(Node));
#endif

      ExpSplayTreeSmallerChildNode(Node) = New;
      ExpSplayTreeNodeKey(Node) = KEY_NOTUSED;

      /* New root node */
      return New;

#endif

    }
  }

  if (Node == NULL)
    {
      ExpSplayTreeSmallerChildNode(New) = NULL;
      ExpSplayTreeLargerChildNode(New)  = NULL;
    }
  else if (ExpSplayTreeNodeLess((*Tree->Compare)(Key, ExpSplayTreeNodeKey(Node))))
    {
      ExpSplayTreeSmallerChildNode(New)  = ExpSplayTreeSmallerChildNode(Node);
      ExpSplayTreeLargerChildNode(New)   = Node;
      ExpSplayTreeSmallerChildNode(Node) = NULL;

#ifdef WEIGHT
      ExpSplayTreeNodeSetWeight(Node, 1 + ExpSplayTreeNodeGetWeight(ExpSplayTreeLargerChildNode(Node)));
#endif

    }
  else
    {
      ExpSplayTreeLargerChildNode(New)  = ExpSplayTreeLargerChildNode(Node);
      ExpSplayTreeSmallerChildNode(New) = Node;
      ExpSplayTreeLargerChildNode(Node) = NULL;

#ifdef WEIGHT
      ExpSplayTreeNodeSetWeight(Node, 1 + ExpSplayTreeNodeGetWeight(ExpSplayTreeSmallerChildNode(Node)));
#endif

    }

  ExpSplayTreeNodeKey(New) = Key;

#ifdef WEIGHT
  ExpSplayTreeNodeSetWeight(New, 1 + ExpSplayTreeNodeGetWeight(ExpSplayTreeSmallerChildNode(New))
    + ExpSplayTreeNodeGetWeight(ExpSplayTreeLargerChildNode(New)));
#endif

#ifndef UNIQUE_KEYS
  /* No identical nodes (yet) */
  ExpSplayTreeNodeSame(New) = NULL;
#endif

  return New;
}


inline PSPLAY_TREE_NODE
ExpInsertSplayTree(PSPLAY_TREE Tree,
  PVOID Key,
  PSPLAY_TREE_NODE Node,
  PSPLAY_TREE_NODE New)
{
  return (*((PSPLAY_TREE_INSERT)Tree->Reserved[INSERT_INDEX]))(Tree, Key, Node, New);
}


/*
 * Deletes the node with key 'Key' from the tree if it's there.
 * Return a pointer to the resulting tree.
 * The lock for the tree must be acquired when this routine is called.
 * This routine does not maintain weight information.
 */
PSPLAY_TREE_NODE
ExpRemoveSplayTreeNoWeight(PSPLAY_TREE Tree,
  PVOID Key,
  PSPLAY_TREE_NODE Node,
  PSPLAY_TREE_NODE * RemovedNode)
{
  PSPLAY_TREE_NODE x;
  LONG Equality;

  if (Node == NULL)
    return NULL;

  Node = ExpSplayTree(Tree, Key, Node);

  Equality = (*Tree->Compare)(Key, ExpSplayTreeNodeKey(Node));
  if (ExpSplayTreeNodeEqual(Equality))
    {
      /* Found the key */

#ifndef UNIQUE_KEYS
	    /* FIRST! Check if there is a list with identical sizes */
      x = ExpSplayTreeNodeSame(Node);
	    if (x)
	      {
	        /* There is several, pick one from the list */

	        /* 'x' is the new root node */

	        ExpSplayTreeNodeKey(x)          = ExpSplayTreeNodeKey(Node);
	        ExpSplayTreeLargerChildNode(x)  = ExpSplayTreeLargerChildNode(Node);
	        ExpSplayTreeSmallerChildNode(x) = ExpSplayTreeSmallerChildNode(Node);

          *RemovedNode = Node;
          return x;
        }
#endif

		  if (ExpSplayTreeSmallerChildNode(Node) == NULL)
		    {
		      x = ExpSplayTreeLargerChildNode(Node);
		    }
		  else
		    {
		      x = ExpSplayTree(Tree, Key, ExpSplayTreeSmallerChildNode(Node));
		      ExpSplayTreeLargerChildNode(x) = ExpSplayTreeLargerChildNode(Node);
		    }
		  *RemovedNode = Node;
		
		  return x;
	  }
	else
	  {
	    *RemovedNode = NULL;                /* No match */
	    return Node;                        /* It wasn't there */
	  }
}


/*
 * Deletes the node with key 'Key' from the tree if it's there.
 * Return a pointer to the resulting tree.
 * The lock for the tree must be acquired when this routine is called.
 * This routine maintains weight information.
 */
PSPLAY_TREE_NODE
ExpRemoveSplayTreeWeight(PSPLAY_TREE Tree,
  PVOID Key,
  PSPLAY_TREE_NODE Node,
  PSPLAY_TREE_NODE * RemovedNode)
{
  PSPLAY_TREE_NODE x;
  LONG Equality;

#ifdef WEIGHT
  LONG tweight;
#endif

  if (Node == NULL)
    return NULL;

#ifdef WEIGHT
  tweight = ExpSplayTreeNodeGetWeight(Node);
#endif

  Node = ExpSplayTree(Tree, Key, Node);

  Equality = (*Tree->Compare)(Key, ExpSplayTreeNodeKey(Node));
  if (ExpSplayTreeNodeEqual(Equality))
    {
      /* Found the key */

#ifndef UNIQUE_KEYS
	    /* FIRST! Check if there is a list with identical sizes */
      x = ExpSplayTreeNodeSame(Node);
	    if (x)
	      {
	        /* There is several, pick one from the list */
	
	        /* 'x' is the new root node */
	
	        ExpSplayTreeNodeKey(x)          = ExpSplayTreeNodeKey(Node);
	        ExpSplayTreeLargerChildNode(x)  = ExpSplayTreeLargerChildNode(Node);
	        ExpSplayTreeSmallerChildNode(x) = ExpSplayTreeSmallerChildNode(Node);

#ifdef WEIGHT
          ExpSplayTreeNodeSetWeight(x, ExpSplayTreeNodeGetWeight(Node));
#endif

          *RemovedNode = Node;
          return x;
        }
#endif

		  if (ExpSplayTreeSmallerChildNode(Node) == NULL)
		    {
		      x = ExpSplayTreeLargerChildNode(Node);
		    }
		  else
		    {
		      x = ExpSplayTree(Tree, Key, ExpSplayTreeSmallerChildNode(Node));
		      ExpSplayTreeLargerChildNode(x) = ExpSplayTreeLargerChildNode(Node);
		    }
		  *RemovedNode = Node;
		
#ifdef WEIGHT
		  if (x != NULL)
		    {
		      ExpSplayTreeNodeSetWeight(x, tweight - 1);
		    }
#endif
		
		  return x;
	  }
	else
	  {
	    *RemovedNode = NULL;                /* No match */
	    return Node;                        /* It wasn't there */
	  }
}


inline PSPLAY_TREE_NODE
ExpRemoveSplayTree(PSPLAY_TREE Tree,
  PVOID Key,
  PSPLAY_TREE_NODE Node,
  PSPLAY_TREE_NODE * RemovedNode)
{
  return (*((PSPLAY_TREE_REMOVE)Tree->Reserved[REMOVE_INDEX]))(Tree, Key, Node, RemovedNode);
}


/*
 * The lock for the tree must be acquired when this routine is called.
 */
ULONG
ExpPrintSplayTree(PSPLAY_TREE Tree,
  PSPLAY_TREE_NODE Node,
  ULONG Distance)
{
  PSPLAY_TREE_NODE n;
  ULONG d = 0;
  ULONG i;

  if (Node == NULL)
    return 0;

  Distance += ExpPrintSplayTree(Tree, ExpSplayTreeLargerChildNode(Node), Distance + 1);

  for (i = 0; i < Distance; i++)
    {
      DbgPrint("  ");
    }

  if (Tree->Weighted)
    {
      DbgPrint("%d(%d)[%d]", ExpSplayTreeNodeKey(Node), ExpSplayTreeNodeGetWeight(Node), i);
    }
  else
   {
     DbgPrint("%d[%d]", ExpSplayTreeNodeKey(Node), i);
   }

  for (n = ExpSplayTreeNodeSame(Node); n; n = ExpSplayTreeNodeSame(n))
    {
      d += i;

      DbgPrint(" [+]");
    }

  d += i;

  d += ExpPrintSplayTree(Tree, ExpSplayTreeSmallerChildNode(Node), Distance + 1);

  return d;
}


#define MAX_DIFF 4

#ifdef WEIGHT

/*
 * The lock for the tree must be acquired when this routine is called.
 * Returns the new root of the tree.
 * Use of this routine could improve performance, or it might not.
 * FIXME: Do some performance tests
 */
PSPLAY_TREE_NODE
ExpSplayTreeMaxTreeWeight(PSPLAY_TREE Tree,
  PSPLAY_TREE_NODE Node)
{
  PSPLAY_TREE_NODE First = Node;
  LONG Diff;

  do
    {
      Diff = ExpSplayTreeNodeGetWeight(ExpSplayTreeSmallerChildNode(Node))
        - ExpSplayTreeNodeGetWeight(ExpSplayTreeLargerChildNode(Node));

      if (Diff >= MAX_DIFF)
        {
          Node = ExpSplayTreeSmallerChildNode(Node);
        }
      else if (Diff <= -MAX_DIFF)
        {
          Node = ExpSplayTreeLargerChildNode(Node);
        }
      else
        break;
    } while (abs(Diff) >= MAX_DIFF);

  if (Node != First)
    return ExpSplayTree(Tree, ExpSplayTreeNodeKey(Node), First);
  else
    return First;
}

#endif


/*
 * Traverse a splay tree using preorder traversal method.
 * Returns FALSE, if the traversal was terminated prematurely or
 * TRUE if the callback routine did not request that the traversal
 * be terminated prematurely.
 * The lock for the tree must be acquired when this routine is called.
 */
BOOLEAN
ExpTraverseSplayTreePreorder(PTRAVERSE_CONTEXT Context,
  PSPLAY_TREE_NODE Node)
{
  PSPLAY_TREE_NODE n; 

  if (Node == NULL)
    return TRUE;

  /* Call the traversal routine */
  if (!(*Context->Routine)(Context->Context,
    ExpSplayTreeNodeKey(Node),
    ExpSplayTreeNodeValue(Node)))
    {
      return FALSE;
    }

	for (n = ExpSplayTreeNodeSame(Node); n; n = ExpSplayTreeNodeSame(n))
		{
		  /* Call the traversal routine */
		  if (!(*Context->Routine)(Context->Context,
		    ExpSplayTreeNodeKey(n),
		    ExpSplayTreeNodeValue(n)))
		    {
		      return FALSE;
		    }
		}

  /* Traverse 'smaller' subtree */
  ExpTraverseSplayTreePreorder(Context, ExpSplayTreeSmallerChildNode(Node));

  /* Traverse 'larger' subtree */
  ExpTraverseSplayTreePreorder(Context, ExpSplayTreeLargerChildNode(Node));

  return TRUE;
}


/*
 * Traverse a splay tree using inorder traversal method.
 * Returns FALSE, if the traversal was terminated prematurely or
 * TRUE if the callback routine did not request that the traversal
 * be terminated prematurely.
 * The lock for the tree must be acquired when this routine is called.
 */
BOOLEAN
ExpTraverseSplayTreeInorder(PTRAVERSE_CONTEXT Context,
  PSPLAY_TREE_NODE Node)
{
  PSPLAY_TREE_NODE n;

  if (Node == NULL)
    return TRUE;

  /* Traverse 'smaller' subtree */
  ExpTraverseSplayTreeInorder(Context, ExpSplayTreeSmallerChildNode(Node));

  /* Call the traversal routine */
  if (!(*Context->Routine)(Context->Context,
    ExpSplayTreeNodeKey(Node),
    ExpSplayTreeNodeValue(Node)))
    {
      return FALSE;
    }

	for (n = ExpSplayTreeNodeSame(Node); n; n = ExpSplayTreeNodeSame(n))
		{
		  /* Call the traversal routine */
		  if (!(*Context->Routine)(Context->Context,
		    ExpSplayTreeNodeKey(n),
		    ExpSplayTreeNodeValue(n)))
		    {
		      return FALSE;
		    }
		}

  /* Traverse right subtree */
  ExpTraverseSplayTreeInorder(Context, ExpSplayTreeLargerChildNode(Node));

  return TRUE;
}


/*
 * Traverse a splay tree using postorder traversal method.
 * Returns FALSE, if the traversal was terminated prematurely or
 * TRUE if the callback routine did not request that the traversal
 * be terminated prematurely.
 * The lock for the tree must be acquired when this routine is called.
 */
BOOLEAN
ExpTraverseSplayTreePostorder(PTRAVERSE_CONTEXT Context,
  PSPLAY_TREE_NODE Node)
{
  PSPLAY_TREE_NODE n;

  if (Node == NULL)
    return TRUE;

  /* Traverse 'smaller' subtree */
  ExpTraverseSplayTreePostorder(Context, ExpSplayTreeSmallerChildNode(Node));

  /* Traverse 'larger' subtree */
  ExpTraverseSplayTreePostorder(Context, ExpSplayTreeLargerChildNode(Node));

  /* Call the traversal routine */
  if (!(*Context->Routine)(Context->Context,
    ExpSplayTreeNodeKey(Node),
    ExpSplayTreeNodeValue(Node)))
    {
      return FALSE;
    }

	for (n = ExpSplayTreeNodeSame(Node); n; n = ExpSplayTreeNodeSame(n))
		{
		  /* Call the traversal routine */
		  if (!(*Context->Routine)(Context->Context,
		    ExpSplayTreeNodeKey(n),
		    ExpSplayTreeNodeValue(n)))
		    {
		      return FALSE;
		    }
		}

  return TRUE;
}


/*
 * Default key compare function. Compares the integer values of the two keys.
 */
LONG STDCALL
ExpSplayTreeDefaultCompare(IN PVOID  Key1,
  IN PVOID  Key2)
{
  if (Key1 == Key2)
    return 0;

  return (((LONG_PTR) Key1 < (LONG_PTR) Key2) ? -1 : 1);
}


/*
 * Initializes a splay tree.
 */
BOOLEAN STDCALL
ExInitializeSplayTree(IN PSPLAY_TREE  Tree,
  IN PKEY_COMPARATOR  Compare,
  IN BOOLEAN  Weighted,
  IN BOOLEAN  UseNonPagedPool)
{
  RtlZeroMemory(Tree, sizeof(SPLAY_TREE));

  Tree->Compare = (Compare == NULL)
    ? ExpSplayTreeDefaultCompare : Compare;

  Tree->Weighted = Weighted;

  if (Weighted)
    {
      Tree->Reserved[SPLAY_INDEX]  = (PVOID) ExpSplayTreeWeight;
      Tree->Reserved[SEARCH_INDEX] = (PVOID) ExpSearchSplayTreeWeight;
      Tree->Reserved[INSERT_INDEX] = (PVOID) ExpInsertSplayTreeWeight;
      Tree->Reserved[REMOVE_INDEX] = (PVOID) ExpRemoveSplayTreeWeight;
    }
	else
		{
      Tree->Reserved[SPLAY_INDEX]  = (PVOID) ExpSplayTreeNoWeight;
      Tree->Reserved[SEARCH_INDEX] = (PVOID) ExpSearchSplayTreeNoWeight;
      Tree->Reserved[INSERT_INDEX] = (PVOID) ExpInsertSplayTreeNoWeight;
      Tree->Reserved[REMOVE_INDEX] = (PVOID) ExpRemoveSplayTreeNoWeight;
		}

  Tree->UseNonPagedPool = UseNonPagedPool;

  if (UseNonPagedPool)
    {
		  ExInitializeNPagedLookasideList(
		    &Tree->List.NonPaged,           /* Lookaside list */
		    NULL,                           /* Allocate routine */
		    NULL,                           /* Free routine */
		    0,                              /* Flags */
		    sizeof(SPLAY_TREE_NODE),        /* Size of each entry */
		    TAG('E','X','S','T'),           /* Tag */
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
		    sizeof(SPLAY_TREE_NODE),       /* Size of each entry */
		    TAG('E','X','S','T'),           /* Tag */
		    0);                             /* Depth */

      ExInitializeFastMutex(&Tree->Lock.Paged);
		}

  return TRUE;
}


/*
 * Release resources used by a splay tree.
 */
VOID STDCALL
ExDeleteSplayTree(IN PSPLAY_TREE  Tree)
{
  PSPLAY_TREE_NODE RemovedNode;
  PSPLAY_TREE_NODE Node;

  /* Remove all nodes */
  Node = ExpSplayTreeRootNode(Tree);
  while (Node != NULL)
    {
      Node = ExpRemoveSplayTree(Tree, Node->Key, Node, &RemovedNode);

      if (RemovedNode != NULL)
	  	  {
          ExpDestroySplayTreeNode(Tree, RemovedNode);
        }
    }

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
 * Insert a value in a splay tree.
 */
VOID STDCALL
ExInsertSplayTree(IN PSPLAY_TREE  Tree,
  IN PVOID  Key,
  IN PVOID  Value)
{
  PSPLAY_TREE_NODE Node;
  PSPLAY_TREE_NODE NewNode;
  KIRQL OldIrql;

  /* FIXME: Use SEH for error reporting */

  NewNode = ExpCreateSplayTreeNode(Tree, Value);

  ExpLockSplayTree(Tree, &OldIrql);
  Node = ExpInsertSplayTree(Tree, Key, ExpSplayTreeRootNode(Tree), NewNode);
  ExpSplayTreeRootNode(Tree) = Node;
  ExpUnlockSplayTree(Tree, &OldIrql);
}


/*
 * Search for a value associated with a given key in a splay tree.
 */
BOOLEAN STDCALL
ExSearchSplayTree(IN PSPLAY_TREE  Tree,
  IN PVOID  Key,
  OUT PVOID  * Value)
{
  PSPLAY_TREE_NODE Node;
  BOOLEAN Status;
  KIRQL OldIrql;

  ExpLockSplayTree(Tree, &OldIrql);
  Status = ExpSearchSplayTree(Tree, Key, ExpSplayTreeRootNode(Tree), &Node);

  if (Status)
    {
      ExpSplayTreeRootNode(Tree) = Node;
      *Value = ExpSplayTreeNodeValue(Node);
      ExpUnlockSplayTree(Tree, &OldIrql);
		  return TRUE;
	  }
	else
	  {
      ExpUnlockSplayTree(Tree, &OldIrql);
	    return FALSE;
	  }
}


/*
 * Remove a value associated with a given key from a splay tree.
 */
BOOLEAN STDCALL
ExRemoveSplayTree(IN PSPLAY_TREE  Tree,
  IN PVOID  Key,
  IN PVOID  * Value)
{
  PSPLAY_TREE_NODE RemovedNode;
  PSPLAY_TREE_NODE Node;
  KIRQL OldIrql;

  ExpLockSplayTree(Tree, &OldIrql);
  Node = ExpRemoveSplayTree(Tree, Key, ExpSplayTreeRootNode(Tree), &RemovedNode);
  ExpSplayTreeRootNode(Tree) = Node;
  ExpUnlockSplayTree(Tree, &OldIrql);

  if (RemovedNode != NULL)
		{
      *Value = ExpSplayTreeNodeValue(RemovedNode);
      ExpDestroySplayTreeNode(Tree, RemovedNode);
      return TRUE;
		}
	else
		{
      return FALSE;
		}
}


/*
 * Return the weight of the root node in the splay tree.
 * The returned value is the number of nodes in the tree.
 */
BOOLEAN STDCALL
ExWeightOfSplayTree(IN PSPLAY_TREE  Tree,
  OUT PULONG  Weight)
{
  KIRQL OldIrql;

  ExpLockSplayTree(Tree, &OldIrql);

	if (!Tree->Weighted)
		{
      ExpUnlockSplayTree(Tree, &OldIrql);
      return FALSE;	
 		}

  *Weight = ExpSplayTreeNodeWeight(ExpSplayTreeRootNode(Tree));
  ExpUnlockSplayTree(Tree, &OldIrql);

  return TRUE;
}


/*
 * Traverse a splay tree using either preorder, inorder or postorder
 * traversal method.
 * Returns FALSE, if the traversal was terminated prematurely or
 * TRUE if the callback routine did not request that the traversal
 * be terminated prematurely.
 */
BOOLEAN STDCALL
ExTraverseSplayTree(IN PSPLAY_TREE  Tree,
  IN TRAVERSE_METHOD  Method,
  IN PTRAVERSE_ROUTINE  Routine,
  IN PVOID  Context)
{
  TRAVERSE_CONTEXT tc;
  BOOLEAN Status;
  KIRQL OldIrql;

  tc.Routine = Routine;
  tc.Context = Context;

  ExpLockSplayTree(Tree, &OldIrql);

  if (ExpSplayTreeRootNode(Tree) == NULL)
		{
      ExpUnlockSplayTree(Tree, &OldIrql);
      return TRUE;
		}

  switch (Method)
    {
      case TraverseMethodPreorder:
        Status = ExpTraverseSplayTreePreorder(&tc, ExpSplayTreeRootNode(Tree));
        break;

      case TraverseMethodInorder:
        Status = ExpTraverseSplayTreeInorder(&tc, ExpSplayTreeRootNode(Tree));
        break;

      case TraverseMethodPostorder:
        Status = ExpTraverseSplayTreePostorder(&tc, ExpSplayTreeRootNode(Tree));
        break;

      default:
        Status = FALSE;
        break;
    }

  ExpUnlockSplayTree(Tree, &OldIrql);

  return Status;
}

/* EOF */
