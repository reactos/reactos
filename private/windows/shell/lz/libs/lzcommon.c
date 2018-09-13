/*
** lzcommon.c - Routines common to LZ compression / expansion.
**
** Author:  DavidDi
*/


// Headers
///////////

#include "common.h"
#include "buffers.h"
#include "lzcommon.h"

/*
** bool LZInitTree(void);
**
** Initializes trees used in LZ compression.
**
** Arguments:  none
**
** Returns:    true/false
**
** Globals:    RightChild[] and Parent[] arrays reset to NIL to begin
**             encoding.
*/
BOOL LZInitTree(PLZINFO pLZI)
{
   INT i;

   /*
   ** For i = 0 to RING_BUF_LEN - 1, rightChild[i] and leftChild[i] will be the
   ** right and left children of node i.  These nodes need not be initialized.
   ** Also, parent[i] is the parent of node i.  These are initialized to
   ** NIL (= N), which stands for 'not used.'
   ** For i = 0 to 255, rightChild[RING_BUF_LEN + i + 1] is the root of the tree
   ** for strings that begin with character i.  These are initialized to NIL.
   ** n.b., there are 256 trees.
   */

   if (!pLZI->rightChild) {
      if (!(pLZI->rightChild = (INT*)LocalAlloc(LPTR, (RING_BUF_LEN + 257) * sizeof(INT)))) {
         return(FALSE);
      }
   }

   if (!pLZI->leftChild) {
      if (!(pLZI->leftChild = (INT*)LocalAlloc(LPTR, (RING_BUF_LEN + 1) * sizeof(INT)))) {
         return(FALSE);
      }
   }

   if (!pLZI->parent) {
      if (!(pLZI->parent = (INT*)LocalAlloc(LPTR, (RING_BUF_LEN + 1) * sizeof(INT)))) {
         return(FALSE);
      }
   }

   for (i = RING_BUF_LEN + 1; i <= RING_BUF_LEN + 256; i++)
      pLZI->rightChild[i] = NIL;

   for (i = 0; i < RING_BUF_LEN; i++)
      pLZI->parent[i] = NIL;

   return(TRUE);
}

VOID
LZFreeTree(PLZINFO pLZI)
{
   // Sanity check
   if (!pLZI) {
      return;
   }

   if (pLZI->rightChild) {
      LocalFree((HLOCAL)pLZI->rightChild);
      pLZI->rightChild = NULL;
   }

   if (pLZI->leftChild) {
      LocalFree((HLOCAL)pLZI->leftChild);
      pLZI->leftChild = NULL;
   }

   if (pLZI->parent) {
      LocalFree((HLOCAL)pLZI->parent);
      pLZI->parent = NULL;
   }
}

/*
** void LZInsertNode(int nodeToInsert, BOOL bDoArithmeticInsert);
**
** Inserts a new tree into the forest.  Inserts string of length
** cbMaxMatchLen, rgbyteRingBuf[r..r + cbMaxMatchLen - 1], into one of the trees
** (rgbyteRingBuf[r]'th tree).
**
** Arguments:  nodeToInsert        - start of string in ring buffer to insert
**                                   (also, associated tree root)
**             bDoArithmeticInsert - flag for performing regular LZ node
**                                   insertion or arithmetic encoding node
**                                   insertion
**
** Returns:    void
**
** Globals:    cbCurMatch - set to length of longest match
**             iCurMatch  - set to start index of longest matching string in
**                          ring buffer
**
** N.b., if cbCurMatch == cbMaxMatchLen, we remove the old node in favor of
** the new one, since the old node will be deleted sooner.
*/
VOID LZInsertNode(INT nodeToInsert, BOOL bDoArithmeticInsert, PLZINFO pLZI)
{
   INT  i, p, cmp, temp;
   BYTE FAR *key;

   // Sanity check
   if (!pLZI) {
      return;
   }

   cmp = 1;

   key = pLZI->rgbyteRingBuf + nodeToInsert;
   p = RING_BUF_LEN + 1 + key[0];

   pLZI->rightChild[nodeToInsert] = pLZI->leftChild[nodeToInsert] = NIL;
   pLZI->cbCurMatch = 0;

   FOREVER
   {
      if (cmp >= 0)
      {
         if (pLZI->rightChild[p] != NIL)
            p = pLZI->rightChild[p];
         else
         {
            pLZI->rightChild[p] = nodeToInsert;
            pLZI->parent[nodeToInsert] = p;
            return;
         }
      }
      else
      {
         if (pLZI->leftChild[p] != NIL)
            p = pLZI->leftChild[p];
         else
         {
            pLZI->leftChild[p] = nodeToInsert;
            pLZI->parent[nodeToInsert] = p;
            return;
         }
      }

      for (i = 1; i < pLZI->cbMaxMatchLen; i++)
         if ((cmp = key[i] - pLZI->rgbyteRingBuf[p + i]) != 0)
            break;

      if (bDoArithmeticInsert == TRUE)
      {
         // Do node insertion for arithmetic encoding.
         if (i > MAX_LITERAL_LEN)
         {
            if (i > pLZI->cbCurMatch)
            {
               pLZI->iCurMatch = (nodeToInsert - p) & (RING_BUF_LEN - 1);
               if ((pLZI->cbCurMatch = i) >= pLZI->cbMaxMatchLen)
                  break;
            }
            else if (i == pLZI->cbCurMatch)
            {
               if ((temp = (nodeToInsert - p) & (RING_BUF_LEN - 1)) < pLZI->iCurMatch)
                  pLZI->iCurMatch = temp;
            }
         }
      }
      else
      {
         // Do node insertion for LZ.
         if (i > pLZI->cbCurMatch)
         {
            pLZI->iCurMatch = p;
            if ((pLZI->cbCurMatch = i) >= pLZI->cbMaxMatchLen)
               break;
         }
      }
   }

   pLZI->parent[nodeToInsert] = pLZI->parent[p];
   pLZI->leftChild[nodeToInsert] = pLZI->leftChild[p];
   pLZI->rightChild[nodeToInsert] = pLZI->rightChild[p];

   pLZI->parent[pLZI->leftChild[p]] = nodeToInsert;
   pLZI->parent[pLZI->rightChild[p]] = nodeToInsert;

   if (pLZI->rightChild[pLZI->parent[p]] == p)
      pLZI->rightChild[pLZI->parent[p]] = nodeToInsert;
   else
      pLZI->leftChild[pLZI->parent[p]] = nodeToInsert;

   // Remove p.
   pLZI->parent[p] = NIL;

   return;
}


/*
** void LZDeleteNode(int nodeToDelete);
**
** Delete a tree from the forest.
**
** Arguments:  nodeToDelete - tree to delete from forest
**
** Returns:    void
**
** Globals:    Parent[], RightChild[], and LeftChild[] updated to reflect the
**             deletion of nodeToDelete.
*/
VOID LZDeleteNode(INT nodeToDelete, PLZINFO pLZI)
{
   INT  q;

   // Sanity check
   if (!pLZI) {
      return;
   }

   if (pLZI->parent[nodeToDelete] == NIL)
      // Tree nodeToDelete is not in the forest.
      return;

   if (pLZI->rightChild[nodeToDelete] == NIL)
      q = pLZI->leftChild[nodeToDelete];
   else if (pLZI->leftChild[nodeToDelete] == NIL)
      q = pLZI->rightChild[nodeToDelete];
   else
   {
      q = pLZI->leftChild[nodeToDelete];
      if (pLZI->rightChild[q] != NIL)
      {
         do
         {
            q = pLZI->rightChild[q];
         } while (pLZI->rightChild[q] != NIL);

         pLZI->rightChild[pLZI->parent[q]] = pLZI->leftChild[q];
         pLZI->parent[pLZI->leftChild[q]] = pLZI->parent[q];
         pLZI->leftChild[q] = pLZI->leftChild[nodeToDelete];
         pLZI->parent[pLZI->leftChild[nodeToDelete]] = q;
      }
      pLZI->rightChild[q] = pLZI->rightChild[nodeToDelete];
      pLZI->parent[pLZI->rightChild[nodeToDelete]] = q;
   }
   pLZI->parent[q] = pLZI->parent[nodeToDelete];

   if (pLZI->rightChild[pLZI->parent[nodeToDelete]] == nodeToDelete)
      pLZI->rightChild[pLZI->parent[nodeToDelete]] = q;
   else
      pLZI->leftChild[pLZI->parent[nodeToDelete]] = q;

   // Remove nodeToDelete.
   pLZI->parent[nodeToDelete] = NIL;

   return;
}

