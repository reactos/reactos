/*****************************************************************************
*                                                                            *
*  LL.C                                                                      *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1989.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Program Description: Linked list module                                   *
*                                                                            *
******************************************************************************
*                                                                            *
*  Revision History:  Created 4/15/89 by Robert Bunney                       *
*                                                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Known Bugs: None                                                          *
*                                                                            *
*                                                                            *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

#define NEAR near
#define PRIVATE static
#define publicsw
#define H_ASSERT
#define H_MEM

#include <windows.h>
#include <port1632.h>

#define ALLOC(x)    GlobalAlloc(GMEM_MOVEABLE, (LONG)x)
#define FREE(x)     GlobalFree(x)
#define LOCK(x)     GlobalLock(x)
#define UNLOCK(x)   GlobalUnlock(x)
#define ASSERT(x)
#define MEMMOVE(dest, src, cb) QvCopy(dest, src, cb)


#include "ll.h"

/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

typedef struct
  {
   HANDLE hNext;
   HANDLE hData;
  } LLN;                                 /* L inked L ist N ode              */
typedef LLN FAR *PLLN;


/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

PRIVATE HLLN NEAR APIENTRY HLLNAlloc(VOID);
VOID FAR * FAR cdecl QvCopy(VOID FAR *, VOID FAR *, LONG);

/*******************
**
** Name:       LLCreate
**
** Purpose:    Creates a link list
**
** Arguments:  None.
**
** Returns:    Link list.  nilLL is returned if an error occurred.
**
*******************/

LL FAR APIENTRY LLCreate(VOID)
  {
  return HLLNAlloc();
  }

/*******************
**
** Name:       InsertLL
**
** Purpose:    Inserts a new node at the head of the linked list
**
** Arguments:  ll     - link list
**             vpData - pointer to data to be associated with
**             c      - count of the bytes pointed to by vpData
**
** Returns:    TRUE iff insertion is successful.
**
*******************/

BOOL FAR APIENTRY InsertLL(ll, qvData, c)
LL ll;
VOID FAR *qvData;
LONG c;
  {
  HLLN   hlln;                          /* Handle for the new node          */
  PLLN   pllnCur;                       /* Head node for linked list        */
  PLLN   pllnNew;                       /* New node                         */
  HANDLE h;                             /* Handle to the object data        */
  VOID FAR *qv;                         /* Pointer to data block            */

  ASSERT(c > 0L);
                                        /* Check and lock to get header node*/
  if ((ll == nilHAND) || ((pllnCur = (PLLN)LOCK((HANDLE)ll)) == NULL))
    return FALSE;
  if ((h = (HANDLE)ALLOC(c)) == nilHAND)       /* Get handle for data              */
    {UNLOCK((HANDLE)ll); return FALSE;}
  if ((qv = LOCK(h)) == NULL)           /* Get pointer to data              */
    {UNLOCK((HANDLE)ll); FREE(h); return FALSE;}
  if ((hlln = HLLNAlloc()) == NULL)     /* Get handle to new node           */
    {UNLOCK((HANDLE)ll); FREE(h); return FALSE;}
  if ((pllnNew = (PLLN)LOCK((HANDLE)hlln)) == NULL)/* Get pointer to new node          */
    {UNLOCK((HANDLE)ll); UNLOCK(h); FREE(h); UNLOCK(hlln); FREE(hlln); return FALSE;}

  MEMMOVE(qv, qvData, c);               /* Copy data                        */
  UNLOCK(h);
  pllnNew->hData = h;                   /* Insert at head of list           */
  pllnNew->hNext = pllnCur->hNext;
  pllnCur->hNext = hlln;
  UNLOCK((HANDLE)ll);
  UNLOCK(hlln);
  return TRUE;
  }


/*******************
**
** Name:       WalkLL
**
** Purpose:    Mechanism for walking the nodes in the linked list
**
** Arguments:  ll   - linked list
**             hlln - handle to a linked list node
**
** Returns:    a handle to a link list node or NIL_HLLN if at the
**             end of the list (or an error).
**
** Notes:      To get the first node, pass NIL_HLLN as the hlln - further
**             calls should use the HLLN returned by this function.
**
*******************/

HLLN FAR APIENTRY WalkLL(ll, hlln)
LL ll;
HLLN hlln;
  {
  PLLN plln;                            /* node in linked list              */
  HLLN hllnT;

  if (hlln == nilHLLN)                  /* First time called                */
    hlln = ll;

  if ((hlln == nilHAND) || ((plln = (PLLN)LOCK((HANDLE)hlln)) == NULL))
    return nilHAND;
  hllnT = plln->hNext;
  UNLOCK(hlln);
  return hllnT;
  }


/*******************
**
** Name:       QVLockHLLN
**
** Purpose:    Locks a LL node returning a pointer to the data
**
** Arguments:  hlln - handle to a linked list node
**
** Returns:    pointer to data or NULL if an error occurred
**
*******************/

VOID FAR * FAR QVLockHLLN(hlln)
HLLN hlln;
  {
  PLLN plln;
  VOID FAR * qv;
                                        /* Lock node                        */
  if ((hlln == nilHAND) || ((plln = (PLLN)LOCK((HANDLE)hlln)) == NULL))
    return NULL;

  qv = LOCK(plln->hData);               /* Get pointer to data              */
  UNLOCK(hlln);
  return qv;
  }


/*******************
**
** Name:       QVUnlockHLLN
**
** Purpose:    Unlocks a LL node
**
** Arguments:  hlln - handle to a link list node
**
** Returns:    TRUE iff the handle is successfully locked.
**
*******************/

VOID FAR UnlockHLLN(hlln)
  HLLN hlln;
  {
  PLLN plln;                            /* Pointer to the node              */

  if ((hlln == nilHAND) || ((plln = (PLLN)LOCK((HANDLE)hlln)) == NULL))
    return;

  UNLOCK(plln->hData);
  UNLOCK(hlln);
  return;
  }


/*******************
**
** Name:       HLLNAlloc
**
** Purpose:    Allocates a link list node
**
** Arguments:  Nothing.
**
** Returns:    A new new node with hNext and hData set to nilHAND
**
**
** Method:
**
*******************/

PRIVATE HLLN NEAR APIENTRY HLLNAlloc(VOID)
  {
  HANDLE h;
  PLLN plln;
  if ((h = ALLOC(sizeof(LLN))) != nilHAND)
    {
    if ((plln = (PLLN) LOCK(h)) != NULL)
      {
      plln->hNext = nilHAND;
      plln->hData = nilHAND;
      UNLOCK(h);
      }
    else
      {
      FREE(h);
      h = nilHAND;
      }
    }
  return (LL)h;
  }


/*******************
**
** Name:       DestroyLL
**
** Purpose:    Deletes a LL and all of its contents
**
** Arguments:  ll - linked list
**
** Returns:    Nothing.
**
*******************/

VOID FAR APIENTRY DestroyLL(ll)
LL ll;
  {
  HLLN hllnNow = ll;
  HLLN hllnNext;
  PLLN plln;

  do
    {
    hllnNext = WalkLL(ll, hllnNow);
    plln = (PLLN)LOCK(hllnNow);
    ASSERT(plln);
    FREE(plln->hData);
    UNLOCK(hllnNow);
    FREE(hllnNow);
    hllnNow = hllnNext;
    } while (hllnNow != nilHLLN);
  }
