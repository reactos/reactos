////////////////////////////////////////////////////////////////////////////////
//
// propmisc.c
//
// Misc. Property set rotuines
//
// Change history:
//
// Date         Who             What
// --------------------------------------------------------------------------
// 07/30/94     B. Wentz        Created file
//
////////////////////////////////////////////////////////////////////////////////

#include "priv.h"
#pragma hdrstop



////////////////////////////////////////////////////////////////////////////////
//
// LpllCreateList
//
// Purpose:
//  Create a linked list with dwc elements.
//
////////////////////////////////////////////////////////////////////////////////
LPLLIST
LpllCreate
  (LPLLIST *lplpll,                     // Head
   LPLLCACHE lpllcache,                 // Cache
   DWORD dwc,                           // Number of nodes to create
   DWORD cbNode)                        // Size of each node
{
  LPLLIST lpllT;
  LPLLIST lpllCur;

  if ((lplpll == NULL) ||
      (dwc < 1))
    return NULL;

  lpllCur = *lplpll = PvMemAlloc(cbNode);
  if (lpllCur == NULL)
  {
    return NULL;
  }
  FillBuf (lpllCur, 0, cbNode);

  lpllCur->lpllistPrev = NULL;
  dwc--;

  while (dwc)
  {
    lpllT = PvMemAlloc(cbNode);
    if (lpllT == NULL)
      goto Fail;

    FillBuf (lpllT, 0, cbNode);
    lpllT->lpllistPrev = lpllCur;
    lpllCur->lpllistNext = lpllT;
    lpllCur = lpllT;
    dwc--;
  } // while

  lpllCur->lpllistNext = NULL;
  lpllcache->lpllist = NULL;

  return *lplpll;

Fail :

  lpllCur = *lplpll;
  while (lpllCur != NULL)
  {
    lpllT = lpllCur;
    lpllCur = lpllCur->lpllistNext;
    VFreeMemP(lpllT, cbNode);
  }

  return NULL;

} // LpllCreateList


////////////////////////////////////////////////////////////////////////////////
//
// LpllGetNode
//
// Purpose:
//  Gets a node from the list
//
////////////////////////////////////////////////////////////////////////////////
LPLLIST
LpllGetNode
  (LPLLIST lpllist,             // The head
   LPLLCACHE lpllcache,         // The cache
   DWORD idw)                   // Index of node to get -- 1 based
{
  DWORD i = idw;

  if ((lpllcache->lpllist != NULL) && (lpllcache->idw == idw))
  {
    return lpllcache->lpllist;
  }

  while ((i > 1) && (lpllist != NULL))
  {
    lpllist = lpllist->lpllistNext;
    i--;
  }

  if (lpllist != NULL)
  {
    lpllcache->idw = idw;
    lpllcache->lpllist = lpllist;
  }

  return lpllist;

} // LpllGetNode


////////////////////////////////////////////////////////////////////////////////
//
// LpllDeleteNode
//
// Purpose:
//  Delete a node from the list
//
////////////////////////////////////////////////////////////////////////////////
LPLLIST
LpllDeleteNode
  (LPLLIST lpllist,                     // Head
   LPLLCACHE lpllcache,                 // Cache
   DWORD idw,                           // Index -- 1 based
   DWORD cbNode,                        // Size of node
   void (*lpfnFreeNode)(LPLLIST)     // Free a node
   )
{
  LPLLIST lpT;
  LPLLIST lpNext;

  BOOL fHead;

    // Check the cache
  if ((lpllcache->lpllist != NULL) && (lpllcache->idw == idw))
  {
    lpT = lpllcache->lpllist;
    lpllcache->lpllist = NULL;
  }
  else
  {
    lpT = LpllGetNode (lpllist, lpllcache, idw);
  }

  if (lpT == NULL)      // We couldn't find the node
  {
         return lpT;
  }

  fHead = lpT->lpllistPrev == NULL;

    // Delete the node from the list
  if (lpT->lpllistPrev != NULL)
  {
    lpT->lpllistPrev->lpllistNext = lpT->lpllistNext;
  }

  if ((lpNext = lpT->lpllistNext) != NULL)
  {
    lpT->lpllistNext->lpllistPrev = lpT->lpllistPrev;
  }

    // Delete the string & node
  (*lpfnFreeNode) (lpT);
  VFreeMemP(lpT, cbNode);

    // If the head was deleted, return the new one.
  return (fHead ? lpNext : lpllist);

} // LpllDeleteNode


////////////////////////////////////////////////////////////////////////////////
//
// LpllInsertNode
//
// Purpose:
//  Insert a node into the list
//
////////////////////////////////////////////////////////////////////////////////
LPLLIST PASCAL
LpllInsertNode
  (LPLLIST lpllist,                     // Head
   LPLLCACHE lpllcache,                 // Cache
   DWORD idw,                           // index
   DWORD cbNode)                        // Size of node
{
  LPLLIST lpT;
  LPLLIST lpOrigHead;
  BOOL fHead;
  DWORD idwOrig;

  if ((lpOrigHead = lpllist) == NULL)
  {
    return NULL;
  }

  idwOrig = idw;

  idw--;
  Assert ((idw >= 0));
  while ((idw) && (lpllist->lpllistNext != NULL))
  {
    lpllist = lpllist->lpllistNext;
    idw--;
  }

  lpT = PvMemAlloc(cbNode);
  if (lpT == NULL)
  {
    return NULL;
  }
  FillBuf (lpT, 0, cbNode);

    // If the new node was to be inserted at the end, idw would not
    // be zero, the while loop would have fallen out because Next was NULL
  if (idw)
  {
    fHead = FALSE;
    lpT->lpllistPrev = lpllist;
    lpT->lpllistNext = NULL;
    lpllist->lpllistNext = lpT;
  }
  else
  {
    fHead = (lpllist == lpOrigHead);

    lpT->lpllistPrev = lpllist->lpllistPrev;
    if (lpllist->lpllistPrev != NULL)
    {
      lpllist->lpllistPrev->lpllistNext = lpT;
    }
    lpT->lpllistNext = lpllist;
    lpllist->lpllistPrev = lpT;
  }

    // Update the cache
  lpllcache->idw = idwOrig;
  lpllcache->lpllist = lpT;

  return (fHead ? lpT : lpOrigHead);

} // LpllInsertNode

