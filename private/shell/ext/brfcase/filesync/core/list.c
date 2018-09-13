/*
 * list.c - List ADT module.
 */

/*

Motivation
----------

   Unfortunately, C7 doesn't fully support templates.  As a result, to link ADT
structures together, we could either embed linked list pointers in those
structures, or create a separate linked list ADT in which each node contains a
pointer to the associated structure.

   If we embed linked list pointers in other ADT structures, we lose the linked
list ADT barrier, and with it, the ability to easily change the linked list
implementation.  However, we no longer need to store an extra pointer to the
data associated with each linked node.

   If we create a separate linked list ADT, we are forced to store a pointer to
the structure associated with the node.  However, we retain the ability to
alter the linked list ADT in the future.

   Let's support the abstraction barrier, and create a separate linked list
ADT.

   In the object synchronization engine, the linked list ADT is used to store
lists of links, link handlers, and strings.


Architecture
------------

   The nodes in each doubly-linked list of nodes are allocated by
AllocateMemory().  A caller-supplied DWORD is stored in each list node.  NULL
is used as a sentinel pointer value for the head and tail of a list.

   A list handle is a pointer to a LIST allocated by AllocateMemory().  A list
node handle is a pointer to a list node.


         head            node            node            tail
        (LIST)          (NODE)          (NODE)          (NODE)
      ÚÄÄÄÄÄÄÄÄÄ¿     ÚÄÄÄÄÄÄÄÄÄ¿     ÚÄÄÄÄÄÄÄÄÄ¿     ÚÄÄÄÄÄÄÄÄÄ¿
      ³pnodeNext³ --> ³pnodeNext³ --> ³pnodeNext³ --> ³pnodeNext³ --0
      ³         ³     ³         ³     ³         ³     ³         ³
  0-- ³pnodePrev³ <-- ³pnodePrev³ <-- ³pnodePrev³ <-- ³pnodePrev³
      ³         ³     ³         ³     ³         ³     ³         ³
      ³  XXXXXX ³     ³   pcv   ³     ³   pcv   ³     ³   pcv   ³
      ÃÄÄÄÄÄÄÄÄÄ´     ÀÄÄÄÄÄÄÄÄÄÙ     ÀÄÄÄÄÄÄÄÄÄÙ     ÀÄÄÄÄÄÄÄÄÄÙ
      ³         ³
      ³   ....  ³


   pnodeNext is non-NULL for all list nodes except the tail.  pnodeNext in
the head is only NULL for an empty list.  pnodePrev is non-NULL for all list
nodes.  pnodePrev in the head is always NULL.  Be careful not to use pnodePrev
from the first list node as another list node!

*/


/* Headers
 **********/

#include "project.h"
#pragma hdrstop


/* Macros
 *********/

/* Add nodes to list in sorted order? */

#define ADD_NODES_IN_SORTED_ORDER(plist)  IS_FLAG_SET((plist)->dwFlags, LIST_FL_SORTED_ADD)


/* Types
 ********/

/* list node types */

typedef struct _node
{
   struct _node *pnodeNext;      /* next node in list */
   struct _node *pnodePrev;      /* previous node in list */
   PCVOID pcv;                   /* node data */
}
NODE;
DECLARE_STANDARD_TYPES(NODE);

/* list flags */

typedef enum _listflags
{
   /* Insert nodes in sorted order. */

   LIST_FL_SORTED_ADD      = 0x0001,

   /* flag combinations */

   ALL_LIST_FLAGS          = LIST_FL_SORTED_ADD
}
LISTFLAGS;

/*
 * A LIST is just a special node at the head of a list.  N.b., the _node
 * structure MUST appear first in the _list structure because a pointer to a
 * list is sometimes used as a pointer to a node.
 */

typedef struct _list
{
   NODE node;

   DWORD dwFlags;
}
LIST;
DECLARE_STANDARD_TYPES(LIST);

/* SearchForNode() return codes */

typedef enum _addnodeaction
{
   ANA_FOUND,
   ANA_INSERT_BEFORE_NODE,
   ANA_INSERT_AFTER_NODE,
   ANA_INSERT_AT_HEAD
}
ADDNODEACTION;
DECLARE_STANDARD_TYPES(ADDNODEACTION);


/***************************** Private Functions *****************************/

/* Module Prototypes
 ********************/

PRIVATE_CODE ADDNODEACTION SearchForNode(HLIST, COMPARESORTEDNODESPROC, PCVOID, PHNODE);

#ifdef VSTF

PRIVATE_CODE BOOL IsValidPCLIST(PCLIST);
PRIVATE_CODE BOOL IsValidPCNODE(PCNODE);

#endif

#ifdef DEBUG

PRIVATE_CODE BOOL IsValidPCNEWLIST(PCNEWLIST);
PRIVATE_CODE BOOL IsValidADDNODEACTION(ADDNODEACTION);
PRIVATE_CODE HLIST GetList(HNODE);

#endif

#if defined(DEBUG) || defined(VSTF)

PRIVATE_CODE BOOL IsListInSortedOrder(PCLIST, COMPARESORTEDNODESPROC);

#endif


/*
** SearchForNode()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE ADDNODEACTION SearchForNode(HLIST hlist,
                                         COMPARESORTEDNODESPROC csnp,
                                         PCVOID pcv, PHNODE phnode)
{
   ADDNODEACTION ana;
   ULONG ulcNodes;

   /* pcv may be any value */

   ASSERT(IS_VALID_HANDLE(hlist, LIST));
   ASSERT(IS_VALID_CODE_PTR(csnp, COMPARESORTEDNODESPROC));
   ASSERT(IS_VALID_WRITE_PTR(phnode, HNODE));

   ASSERT(ADD_NODES_IN_SORTED_ORDER((PCLIST)hlist));
   ASSERT(IsListInSortedOrder((PCLIST)hlist, csnp));

   /* Yes.  Are there any nodes in this list? */

   ulcNodes = GetNodeCount(hlist);

   ASSERT(ulcNodes < LONG_MAX);

   if (ulcNodes > 0)
   {
      LONG lLow = 0;
      LONG lMiddle = 0;
      LONG lHigh = ulcNodes - 1;
      LONG lCurrent = 0;
      int nCmpResult = 0;

      /* Yes.  Search for target. */

      EVAL(GetFirstNode(hlist, phnode));

      while (lLow <= lHigh)
      {
         lMiddle = (lLow + lHigh) / 2;

         /* Which way should we seek in the list to get the lMiddle node? */

         if (lCurrent < lMiddle)
         {
            /* Forward from the current node. */

            while (lCurrent < lMiddle)
            {
               EVAL(GetNextNode(*phnode, phnode));
               lCurrent++;
            }
         }
         else if (lCurrent > lMiddle)
         {
            /* Backward from the current node. */

            while (lCurrent > lMiddle)
            {
               EVAL(GetPrevNode(*phnode, phnode));
               lCurrent--;
            }
         }

         nCmpResult = (*csnp)(pcv, GetNodeData(*phnode));

         if (nCmpResult < 0)
            lHigh = lMiddle - 1;
         else if (nCmpResult > 0)
            lLow = lMiddle + 1;
         else
            /* Found a match at *phnode. */
            break;
      }

      /*
       * If (nCmpResult >  0), insert after *phnode.
       *
       * If (nCmpResult <  0), insert before *phnode.
       *
       * If (nCmpResult == 0), string found at *phnode.
       */

      if (nCmpResult > 0)
         ana = ANA_INSERT_AFTER_NODE;
      else if (nCmpResult < 0)
         ana = ANA_INSERT_BEFORE_NODE;
      else
         ana = ANA_FOUND;
   }
   else
   {
      /* No.  Insert the target as the only node in the list. */

      *phnode = NULL;
      ana = ANA_INSERT_AT_HEAD;
   }

   ASSERT(EVAL(IsValidADDNODEACTION(ana)) &&
          (ana == ANA_INSERT_AT_HEAD ||
           IS_VALID_HANDLE(*phnode, NODE)));

   return(ana);
}


#ifdef VSTF

/*
** IsValidPCLIST()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCLIST(PCLIST pcl)
{
   BOOL bResult = FALSE;

   if (IS_VALID_READ_PTR(pcl, CLIST) &&
       FLAGS_ARE_VALID(pcl->dwFlags, ALL_LIST_FLAGS) &&
       EVAL(! pcl->node.pnodePrev))
   {
      PNODE pnode;

      for (pnode = pcl->node.pnodeNext;
           pnode && IS_VALID_STRUCT_PTR(pnode, CNODE);
           pnode = pnode->pnodeNext)
         ;

      bResult = (! pnode);
   }

   return(bResult);
}


/*
** IsValidPCNODE()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCNODE(PCNODE pcn)
{
   /*
    * All valid nodes must have a valid pnodePrev pointer.  The first node's
    * pnodePrev pointer points at the list head.  A node's pnodeNext pointer
    * may be a valid pointer or NULL.
    */

   return(IS_VALID_READ_PTR(pcn, CNODE) &&
          EVAL(IS_VALID_READ_PTR(pcn->pnodePrev, CNODE) &&
               pcn->pnodePrev->pnodeNext == pcn) &&
          EVAL(! pcn->pnodeNext ||
               (IS_VALID_READ_PTR(pcn->pnodeNext, CNODE) &&
                pcn->pnodeNext->pnodePrev == pcn)));
}

#endif


#ifdef DEBUG

/*
** IsValidPCNEWLIST()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCNEWLIST(PCNEWLIST pcnl)
{
   return(IS_VALID_READ_PTR(pcnl, CNEWLIST) &&
          FLAGS_ARE_VALID(pcnl->dwFlags, ALL_NL_FLAGS));
}


/*
** IsValidADDNODEACTION()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidADDNODEACTION(ADDNODEACTION ana)
{
   BOOL bResult;

   switch (ana)
   {
      case ANA_FOUND:
      case ANA_INSERT_BEFORE_NODE:
      case ANA_INSERT_AFTER_NODE:
      case ANA_INSERT_AT_HEAD:
         bResult = TRUE;
         break;

      default:
         bResult = FALSE;
         ERROR_OUT((TEXT("IsValidADDNODEACTION(): Invalid ADDNODEACTION %d."),
                    ana));
         break;
   }

   return(bResult);
}


/*
** GetList()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE HLIST GetList(HNODE hnode)
{
   PCNODE pcnode;

   ASSERT(IS_VALID_HANDLE(hnode, NODE));

   ASSERT(((PCNODE)hnode)->pnodePrev);

   for (pcnode = (PCNODE)hnode; pcnode->pnodePrev; pcnode = pcnode->pnodePrev)
      ;

   return((HLIST)pcnode);
}

#endif


#if defined(DEBUG) || defined(VSTF)

/*
** IsListInSortedOrder()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsListInSortedOrder(PCLIST pclist, COMPARESORTEDNODESPROC csnp)
{
   BOOL bResult = TRUE;
   PNODE pnode;

   /* Don't validate pclist here. */

   ASSERT(ADD_NODES_IN_SORTED_ORDER(pclist));
   ASSERT(IS_VALID_CODE_PTR(csnp, COMPARESORTEDNODESPROC));

   pnode = pclist->node.pnodeNext;

   while (pnode)
   {
      PNODE pnodeNext;

      pnodeNext = pnode->pnodeNext;

      if (pnodeNext)
      {
         if ( (*csnp)(pnode->pcv, pnodeNext->pcv) == CR_FIRST_LARGER)
         {
            bResult = FALSE;
            ERROR_OUT((TEXT("IsListInSortedOrder(): Node [%ld] %#lx > following node [%ld] %#lx."),
                       pnode,
                       pnode->pcv,
                       pnodeNext,
                       pnodeNext->pcv));
            break;
         }

         pnode = pnodeNext;
      }
      else
         break;
   }

   return(bResult);
}

#endif


/****************************** Public Functions *****************************/


/*
** CreateList()
**
** Creates a new list.
**
** Arguments:     void
**
** Returns:       Handle to new list, or NULL if unsuccessful.
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL CreateList(PCNEWLIST pcnl, PHLIST phlist)
{
   PLIST plist;

   ASSERT(IS_VALID_STRUCT_PTR(pcnl, CNEWLIST));
   ASSERT(IS_VALID_WRITE_PTR(phlist, HLIST));

   /* Try to allocate new list structure. */

   *phlist = NULL;

   if (AllocateMemory(sizeof(*plist), &plist))
   {
      /* List allocated successfully.  Initialize list fields. */

      plist->node.pnodeNext = NULL;
      plist->node.pnodePrev = NULL;
      plist->node.pcv = NULL;

      plist->dwFlags = 0;

      if (IS_FLAG_SET(pcnl->dwFlags, NL_FL_SORTED_ADD))
      {
         SET_FLAG(plist->dwFlags, LIST_FL_SORTED_ADD);
      }

      *phlist = (HLIST)plist;

      ASSERT(IS_VALID_HANDLE(*phlist, LIST));
   }

   return(*phlist != NULL);
}


/*
** DestroyList()
**
** Deletes a list.
**
** Arguments:     hlist - handle to list to be deleted
**
** Returns:       void
**
** Side Effects:  none
*/
PUBLIC_CODE void DestroyList(HLIST hlist)
{
   ASSERT(IS_VALID_HANDLE(hlist, LIST));

   DeleteAllNodes(hlist);

   /* Delete list. */

   FreeMemory((PLIST)hlist);

   return;
}


#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

/*
** AddNode()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL AddNode(HLIST hlist, COMPARESORTEDNODESPROC csnp, PCVOID pcv, PHNODE phnode)
{
   BOOL bResult;

   ASSERT(IS_VALID_HANDLE(hlist, LIST));

   if (ADD_NODES_IN_SORTED_ORDER((PCLIST)hlist))
   {
      ADDNODEACTION ana;

      ana = SearchForNode(hlist, csnp, pcv, phnode);

      ASSERT(ana != ANA_FOUND);

      switch (ana)
      {
         case ANA_INSERT_BEFORE_NODE:
            bResult = InsertNodeBefore(*phnode, csnp, pcv, phnode);
            break;

         case ANA_INSERT_AFTER_NODE:
            bResult = InsertNodeAfter(*phnode, csnp, pcv, phnode);
            break;

         default:
            ASSERT(ana == ANA_INSERT_AT_HEAD);
            bResult = InsertNodeAtFront(hlist, csnp, pcv, phnode);
            break;
      }
   }
   else
      bResult = InsertNodeAtFront(hlist, csnp, pcv, phnode);

   ASSERT(! bResult ||
          IS_VALID_HANDLE(*phnode, NODE));

   return(bResult);
}


/*
** InsertNodeAtFront()
**
** Inserts a node at the front of a list.
**
** Arguments:     hlist - handle to list that node is to be inserted at head of
**                pcv - data to be stored in node
**
** Returns:       Handle to new node, or NULL if unsuccessful.
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL InsertNodeAtFront(HLIST hlist, COMPARESORTEDNODESPROC csnp, PCVOID pcv, PHNODE phnode)
{
   BOOL bResult;
   PNODE pnode;

   ASSERT(IS_VALID_HANDLE(hlist, LIST));
   ASSERT(IS_VALID_WRITE_PTR(phnode, HNODE));

#ifdef DEBUG

   /* Make sure the correct index was given for insertion. */

   if (ADD_NODES_IN_SORTED_ORDER((PCLIST)hlist))
   {
      HNODE hnodeNew;
      ADDNODEACTION anaNew;

      anaNew = SearchForNode(hlist, csnp, pcv, &hnodeNew);

      ASSERT(anaNew != ANA_FOUND);
      ASSERT(anaNew == ANA_INSERT_AT_HEAD ||
             (anaNew == ANA_INSERT_BEFORE_NODE &&
              hnodeNew == (HNODE)(((PCLIST)hlist)->node.pnodeNext)));
   }

#endif

   bResult = AllocateMemory(sizeof(*pnode), &pnode);

   if (bResult)
   {
      /* Add new node to front of list. */

      pnode->pnodePrev = (PNODE)hlist;
      pnode->pnodeNext = ((PLIST)hlist)->node.pnodeNext;
      pnode->pcv = pcv;

      ((PLIST)hlist)->node.pnodeNext = pnode;

      /* Any more nodes in list? */

      if (pnode->pnodeNext)
         pnode->pnodeNext->pnodePrev = pnode;

      *phnode = (HNODE)pnode;
   }

   ASSERT(! bResult ||
          IS_VALID_HANDLE(*phnode, NODE));

   return(bResult);
}


/*
** InsertNodeBefore()
**
** Inserts a new node in a list before a given node.
**
** Arguments:     hnode - handle to node that new node is to be inserted before
**                pcv - data to be stored in node
**
** Returns:       Handle to new node, or NULL if unsuccessful.
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL InsertNodeBefore(HNODE hnode, COMPARESORTEDNODESPROC csnp, PCVOID pcv, PHNODE phnode)
{
   BOOL bResult;
   PNODE pnode;

   ASSERT(IS_VALID_HANDLE(hnode, NODE));
   ASSERT(IS_VALID_WRITE_PTR(phnode, HNODE));

#ifdef DEBUG

   {
      HLIST hlistParent;

      /* Make sure the correct index was given for insertion. */

      hlistParent = GetList(hnode);

      if (ADD_NODES_IN_SORTED_ORDER((PCLIST)hlistParent))
      {
         HNODE hnodeNew;
         ADDNODEACTION anaNew;

         anaNew = SearchForNode(hlistParent, csnp, pcv, &hnodeNew);

         ASSERT(anaNew != ANA_FOUND);
         ASSERT((anaNew == ANA_INSERT_BEFORE_NODE &&
                 hnodeNew == hnode) ||
                (anaNew == ANA_INSERT_AFTER_NODE &&
                 hnodeNew == (HNODE)(((PCNODE)hnode)->pnodePrev)) ||
                (anaNew == ANA_INSERT_AT_HEAD &&
                 hnode == (HNODE)(((PCLIST)hlistParent)->node.pnodeNext)));
      }
   }

#endif

   bResult = AllocateMemory(sizeof(*pnode), &pnode);

   if (bResult)
   {
      /* Insert new node before given node. */

      pnode->pnodePrev = ((PNODE)hnode)->pnodePrev;
      pnode->pnodeNext = (PNODE)hnode;
      pnode->pcv = pcv;

      ((PNODE)hnode)->pnodePrev->pnodeNext = pnode;

      ((PNODE)hnode)->pnodePrev = pnode;

      *phnode = (HNODE)pnode;
   }

   ASSERT(! bResult ||
          IS_VALID_HANDLE(*phnode, NODE));

   return(bResult);
}


/*
** InsertNodeAfter()
**
** Inserts a new node in a list after a given node.
**
** Arguments:     hnode - handle to node that new node is to be inserted after
**                pcv - data to be stored in node
**
** Returns:       Handle to new node, or NULL if unsuccessful.
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL InsertNodeAfter(HNODE hnode, COMPARESORTEDNODESPROC csnp, PCVOID pcv, PHNODE phnode)
{
   BOOL bResult;
   PNODE pnode;

   ASSERT(IS_VALID_HANDLE(hnode, NODE));
   ASSERT(IS_VALID_WRITE_PTR(phnode, HNODE));

#ifdef DEBUG

   /* Make sure the correct index was given for insertion. */

   {
      HLIST hlistParent;

      /* Make sure the correct index was given for insertion. */

      hlistParent = GetList(hnode);

      if (ADD_NODES_IN_SORTED_ORDER((PCLIST)hlistParent))
      {
         HNODE hnodeNew;
         ADDNODEACTION anaNew;

         anaNew = SearchForNode(hlistParent, csnp, pcv, &hnodeNew);

         ASSERT(anaNew != ANA_FOUND);
         ASSERT((anaNew == ANA_INSERT_AFTER_NODE &&
                 hnodeNew == hnode) ||
                (anaNew == ANA_INSERT_BEFORE_NODE &&
                 hnodeNew == (HNODE)(((PCNODE)hnode)->pnodeNext)));
      }
   }

#endif

   bResult = AllocateMemory(sizeof(*pnode), &pnode);

   if (bResult)
   {
      /* Insert new node after given node. */

      pnode->pnodePrev = (PNODE)hnode;
      pnode->pnodeNext = ((PNODE)hnode)->pnodeNext;
      pnode->pcv = pcv;

      /* Are we inserting after the tail of the list? */

      if (((PNODE)hnode)->pnodeNext)
         /* No. */
         ((PNODE)hnode)->pnodeNext->pnodePrev = pnode;

      ((PNODE)hnode)->pnodeNext = pnode;

      *phnode = (HNODE)pnode;
   }

   ASSERT(! bResult ||
          IS_VALID_HANDLE(*phnode, NODE));

   return(bResult);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


/*
** DeleteNode()
**
** Removes a node from a list.
**
** Arguments:     hnode - handle to node to be removed
**
** Returns:       void
**
** Side Effects:  none
*/
PUBLIC_CODE void DeleteNode(HNODE hnode)
{
   ASSERT(IS_VALID_HANDLE(hnode, NODE));

   /*
    * There is always a previous node for normal list nodes.  Even the head
    * list node is preceded by the list's leading LIST node.
    */

   ((PNODE)hnode)->pnodePrev->pnodeNext = ((PNODE)hnode)->pnodeNext;

   /* Any more nodes in list? */

   if (((PNODE)hnode)->pnodeNext)
      ((PNODE)hnode)->pnodeNext->pnodePrev = ((PNODE)hnode)->pnodePrev;

   FreeMemory((PNODE)hnode);

   return;
}


/*
** DeleteAllNodes()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void DeleteAllNodes(HLIST hlist)
{
   PNODE pnodePrev;
   PNODE pnode;

   ASSERT(IS_VALID_HANDLE(hlist, LIST));

   /* Walk list, starting with first node after head, deleting each node. */

   pnodePrev = ((PLIST)hlist)->node.pnodeNext;

   /*
    * Deleting the tail node in the loop forces us to add an extra
    * comparison to the body of the loop.  Trade speed for size here.
    */

   while (pnodePrev)
   {
      pnode = pnodePrev->pnodeNext;

      FreeMemory(pnodePrev);

      pnodePrev = pnode;

      if (pnode)
         pnode = pnode->pnodeNext;
   }

   ((PLIST)hlist)->node.pnodeNext = NULL;

   return;
}


/*
** GetNodeData()
**
** Gets the data stored in a node.
**
** Arguments:     hnode - handle to node whose data is to be returned
**
** Returns:       Pointer to node's data.
**
** Side Effects:  none
*/
PUBLIC_CODE PVOID GetNodeData(HNODE hnode)
{
   ASSERT(IS_VALID_HANDLE(hnode, NODE));

   return((PVOID)(((PNODE)hnode)->pcv));
}


/*
** SetNodeData()
**
** Sets the data stored in a node.
**
** Arguments:     hnode - handle to node whose data is to be set
**                pcv - node data
**
** Returns:       void
**
** Side Effects:  none
*/
PUBLIC_CODE void SetNodeData(HNODE hnode, PCVOID pcv)
{
   ASSERT(IS_VALID_HANDLE(hnode, NODE));

   ((PNODE)hnode)->pcv = pcv;

   return;
}


/*
** GetNodeCount()
**
** Counts the number of nodes in a list.
**
** Arguments:     hlist - handle to list whose nodes are to be counted
**
** Returns:       Number of nodes in list.
**
** Side Effects:  none
**
** N.b., this is an O(n) operation since we don't explicitly keep track of the
** number of nodes in a list.
*/
PUBLIC_CODE ULONG GetNodeCount(HLIST hlist)
{
   PNODE pnode;
   ULONG ulcNodes;

   ASSERT(IS_VALID_HANDLE(hlist, LIST));

   ulcNodes = 0;

   for (pnode = ((PLIST)hlist)->node.pnodeNext;
        pnode;
        pnode = pnode->pnodeNext)
   {
      ASSERT(ulcNodes < ULONG_MAX);
      ulcNodes++;
   }

   return(ulcNodes);
}


/*
** IsListEmpty()
**
** Determines whether or not a list is empty.
**
** Arguments:     hlist - handle to list to be checked
**
** Returns:       TRUE if list is empty, or FALSE if not.
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsListEmpty(HLIST hlist)
{
   ASSERT(IS_VALID_HANDLE(hlist, LIST));

   return(((PLIST)hlist)->node.pnodeNext == NULL);
}


/*

   To walk a list:
   ---------------

   {
      BOOL bContinue;
      HNODE hnode;

      for (bContinue = GetFirstNode(hlist, &hnode);
           bContinue;
           bContinue = GetNextNode(hnode, &hnode))
         DoSomethingWithNode(hnode);
   }

   or:
   ---

   {
      HNODE hnode;

      if (GetFirstNode(hlist, &hnode))
      {
         do
         {
            DoSomethingWithNode(hnode);
         }
         while (GetNextNode(hnode, &hnode));
      }
   }

   To compare nodes by adjacent pairs:
   -----------------------------------

   {
      HNODE hnodePrev;

      if (GetFirstNode(hlist, &hnodePrev))
      {
         PFOO pfooPrev;
         HNODE hnodeNext;

         pfooPrev = GetNodeData(hnodePrev);

         while (GetNextNode(hnodePrev, &hnodeNext))
         {
            PFOO pfooNext;

            pfooNext = GetNodeData(hnodeNext);

            CompareFoos(pfooPrev, pfooNext);

            hnodePrev = hnodeNext;
            pfooPrev = pfooNext;
         }
      }
   }

   To destroy nodes in a list:
   ---------------------------

   {
      BOOL bContinue;
      HNODE hnodePrev;

      bContinue = GetFirstNode(hlist, &hnodePrev);

      while (bContinue)
      {
         HNODE hnodeNext;

         bContinue = GetNextNode(hnodePrev, &hnodeNext);

         DeleteNode(hnodePrev);

         hnodePrev = hnodeNext;
      }
   }

*/


/*
** GetFirstNode()
**
** Gets the head node in a list.
**
** Arguments:     hlist - handle to list whose head node is to be retrieved
**
** Returns:       Handle to head list node, or NULL if list is empty.
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL GetFirstNode(HLIST hlist, PHNODE phnode)
{
   ASSERT(IS_VALID_HANDLE(hlist, LIST));
   ASSERT(IS_VALID_WRITE_PTR(phnode, HNODE));

   *phnode = (HNODE)(((PLIST)hlist)->node.pnodeNext);

   ASSERT(! *phnode || IS_VALID_HANDLE(*phnode, NODE));

   return(*phnode != NULL);
}


/*
** GetNextNode()
**
** Gets the next node in a list.
**
** Arguments:     hnode - handle to current node
**                phnode - pointer to HNODE to be filled in with handle to next
**                         node in list, *phnode is only valid if GetNextNode()
**                         returns TRUE
**
** Returns:       TRUE if there is another node in the list, or FALSE if there
**                are no more nodes in the list.
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL GetNextNode(HNODE hnode, PHNODE phnode)
{
   ASSERT(IS_VALID_HANDLE(hnode, NODE));
   ASSERT(IS_VALID_WRITE_PTR(phnode, HNODE));

   *phnode = (HNODE)(((PNODE)hnode)->pnodeNext);

   ASSERT(! *phnode || IS_VALID_HANDLE(*phnode, NODE));

   return(*phnode != NULL);
}


/*
** GetPrevNode()
**
** Gets the previous node in a list.
**
** Arguments:     hnode - handle to current node
**
** Returns:       Handle to previous node in list, or NULL if there are no
**                previous nodes in the list.
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL GetPrevNode(HNODE hnode, PHNODE phnode)
{
   ASSERT(IS_VALID_HANDLE(hnode, NODE));
   ASSERT(IS_VALID_WRITE_PTR(phnode, HNODE));

   /* Is this the first node in the list? */

   if (((PNODE)hnode)->pnodePrev->pnodePrev)
   {
      *phnode = (HNODE)(((PNODE)hnode)->pnodePrev);
      ASSERT(IS_VALID_HANDLE(*phnode, NODE));
   }
   else
      *phnode = NULL;

   return(*phnode != NULL);
}


/*
** AppendList()
**
** Appends one list on to another, leaving the source list empty.
**
** Arguments:     hlistDest - handle to destination list to append to
**                hlistSrc - handle to source list to truncate
**
** Returns:       void
**
** Side Effects:  none
**
** N.b., all HNODEs from both lists remain valid.
*/
PUBLIC_CODE void AppendList(HLIST hlistDest, HLIST hlistSrc)
{
   PNODE pnode;

   ASSERT(IS_VALID_HANDLE(hlistDest, LIST));
   ASSERT(IS_VALID_HANDLE(hlistSrc, LIST));

   if (hlistSrc != hlistDest)
   {
      /* Find last node in destination list to append to. */

      /*
       * N.b., start with the actual LIST node here, not the first node in the
       * list, in case the list is empty.
       */

      for (pnode = &((PLIST)hlistDest)->node;
           pnode->pnodeNext;
           pnode = pnode->pnodeNext)
         ;

      /* Append the source list to the last node in the destination list. */

      pnode->pnodeNext = ((PLIST)hlistSrc)->node.pnodeNext;

      if (pnode->pnodeNext)
         pnode->pnodeNext->pnodePrev = pnode;

      ((PLIST)hlistSrc)->node.pnodeNext = NULL;
   }
   else
      WARNING_OUT((TEXT("AppendList(): Source list same as destination list (%#lx)."),
                   hlistDest));

   return;
}


/*
** SearchSortedList()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL SearchSortedList(HLIST hlist, COMPARESORTEDNODESPROC csnp,
                                  PCVOID pcv, PHNODE phnode)
{
   BOOL bResult;

   /* pcv may be any value */

   ASSERT(IS_VALID_HANDLE(hlist, LIST));
   ASSERT(IS_VALID_CODE_PTR(csnp, COMPARESORTEDNODESPROC));
   ASSERT(IS_VALID_WRITE_PTR(phnode, HNODE));

   ASSERT(ADD_NODES_IN_SORTED_ORDER((PCLIST)hlist));

   bResult = (SearchForNode(hlist, csnp, pcv, phnode) == ANA_FOUND);

   ASSERT(! bResult ||
          IS_VALID_HANDLE(*phnode, NODE));

   return(bResult);
}


/*
** SearchUnsortedList()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL SearchUnsortedList(HLIST hlist, COMPAREUNSORTEDNODESPROC cunp,
                                    PCVOID pcv, PHNODE phn)
{
   PNODE pnode;

   ASSERT(IS_VALID_HANDLE(hlist, LIST));
   ASSERT(IS_VALID_CODE_PTR(cunp, COMPAREUNSORTEDNODESPROC));
   ASSERT(IS_VALID_WRITE_PTR(phn, HNODE));

   *phn = NULL;

   for (pnode = ((PLIST)hlist)->node.pnodeNext;
        pnode;
        pnode = pnode->pnodeNext)
   {
      if ((*cunp)(pcv, pnode->pcv) == CR_EQUAL)
      {
         *phn = (HNODE)pnode;
         break;
      }
   }

   return(*phn != NULL);
}


/*
** WalkList()
**
** Walks a list, calling a callback function with each list node's data and
** caller supplied data.
**
** Arguments:     hlist - handle to list to be searched
**                wlp - callback function to be called with each list node's
**                      data, called as:
**
**                         bContinue = (*wlwdp)(pv, pvRefData);
**
**                      wlp should return TRUE to continue the walk, or FALSE
**                      to halt the walk
**                pvRefData - data to pass to callback function
**
** Returns:       FALSE if callback function aborted the walk.  TRUE if the
**                walk completed.
**
** N.b., the callback function is allowed to delete the node it is passed.
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL WalkList(HLIST hlist, WALKLIST wlp, PVOID pvRefData)
{
   BOOL bResult = TRUE;
   PNODE pnode;

   ASSERT(IS_VALID_HANDLE(hlist, LIST));
   ASSERT(IS_VALID_CODE_PTR(wlp, WALKLISTPROC));

   pnode = ((PLIST)hlist)->node.pnodeNext;

   while (pnode)
   {
      PNODE pnodeNext;

      pnodeNext = pnode->pnodeNext;

      if ((*wlp)((PVOID)(pnode->pcv), pvRefData))
         pnode = pnodeNext;
      else
      {
         bResult = FALSE;
         break;
      }
   }

   return(bResult);
}


#if defined(DEBUG) || defined(VSTF)

/*
** IsValidHLIST()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidHLIST(HLIST hlist)
{
   return(IS_VALID_STRUCT_PTR((PLIST)hlist, CLIST));
}


/*
** IsValidHNODE()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidHNODE(HNODE hnode)
{
   return(IS_VALID_STRUCT_PTR((PNODE)hnode, CNODE));
}

#endif

