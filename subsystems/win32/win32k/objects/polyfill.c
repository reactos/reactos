/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
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
/* $Id$
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Various Polygon Filling routines for Polygon()
 * FILE:              subsys/win32k/objects/polyfill.c
 * PROGRAMER:         Mark Tempel
 * REVISION HISTORY:
 *                 21/2/2003: Created
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

INT __cdecl abs(INT nm);

#define FILL_EDGE_ALLOC_TAG 0x45465044

/*
** This struct is used for book keeping during polygon filling routines.
*/
typedef struct _tagFILL_EDGE
{
  /*Basic line information*/
  int FromX;
  int FromY;
  int ToX;
  int ToY;
  int dx;
  int dy;
  int absdx, absdy;
  int x, y;
  int xmajor;

  /*Active Edge List information*/
  int XIntercept[2];
  int Error;
  int ErrorMax;
  int XDirection, YDirection;

  /* The next edge in the active Edge List*/
  struct _tagFILL_EDGE * pNext;
} FILL_EDGE;

typedef struct _FILL_EDGE_LIST
{
  int Count;
  FILL_EDGE** Edges;
} FILL_EDGE_LIST;

#if 0
static
void
DEBUG_PRINT_ACTIVE_EDGELIST ( FILL_EDGE* list )
{
  FILL_EDGE* pThis = list;
  if (0 == list)
  {
    DPRINT1("List is NULL\n");
    return;
  }

  while(0 != pThis)
  {
    //DPRINT1("EDGE: (%d, %d) to (%d, %d)\n", pThis->FromX, pThis->FromY, pThis->ToX, pThis->ToY);
    DPRINT1("EDGE: [%d,%d]\n", pThis->XIntercept[0], pThis->XIntercept[1] );
    pThis = pThis->pNext;
  }
}
#else
#define DEBUG_PRINT_ACTIVE_EDGELIST(x)
#endif

/*
**  Hide memory clean up.
*/
static
void
FASTCALL
POLYGONFILL_DestroyEdgeList(FILL_EDGE_LIST* list)
{
  int i;
  if ( list )
  {
    if ( list->Edges )
    {
      for ( i = 0; i < list->Count; i++ )
      {
	if ( list->Edges[i] )
	  EngFreeMem ( list->Edges[i] );
      }
      EngFreeMem ( list->Edges );
    }
    EngFreeMem ( list );
  }
}

/*
** This makes and initiaizes an Edge struct for a line between two points.
*/
static
FILL_EDGE*
FASTCALL
POLYGONFILL_MakeEdge(POINT From, POINT To)
{
  FILL_EDGE* rc = (FILL_EDGE*)EngAllocMem(FL_ZERO_MEMORY, sizeof(FILL_EDGE), FILL_EDGE_ALLOC_TAG);

  if (0 == rc)
    return NULL;

  //DPRINT1("Making Edge: (%d, %d) to (%d, %d)\n", From.x, From.y, To.x, To.y);
  //Now Fill the struct.
  if ( To.y < From.y )
  {
    rc->FromX = To.x;
    rc->FromY = To.y;
    rc->ToX = From.x;
    rc->ToY = From.y;
    rc->YDirection = -1;

    // lines that go up get walked backwards, so need to be offset
    // by -1 in order to make the walk identically on a pixel-level
    rc->Error = -1;
  }
  else
  {
    rc->FromX = From.x;
    rc->FromY = From.y;
    rc->ToX = To.x;
    rc->ToY = To.y;
    rc->YDirection = 1;

    rc->Error = 0;
  }

  rc->x = rc->FromX;
  rc->y = rc->FromY;
  rc->dx   = rc->ToX - rc->FromX;
  rc->dy   = rc->ToY - rc->FromY;
  rc->absdx = abs(rc->dx);
  rc->absdy = abs(rc->dy);

  rc->xmajor = rc->absdx > rc->absdy;

  rc->ErrorMax = max(rc->absdx,rc->absdy);

  rc->Error += rc->ErrorMax / 2;

  rc->XDirection = (rc->dx < 0)?(-1):(1);

  rc->pNext = 0;

  //DPRINT("MakeEdge (%i,%i)->(%i,%i) d=(%i,%i) dir=(%i,%i) err=%i max=%i\n",
  //  From.x, From.y, To.x, To.y, rc->dx, rc->dy, rc->XDirection, rc->YDirection, rc->Error, rc->ErrorMax );

  return rc;
}
/*
** My Edge comparison routine.
** This is for scan converting polygon fill.
** First sort by MinY, then Minx, then slope.
**
** This comparison will help us determine which
** lines will become active first when scanning from
** top (min y) to bottom (max y).
**
** Return Value Meaning
** Negative integer element1 < element2
** Zero element1 = element2
** Positive integer element1 > element2
*/
static
INT
FASTCALL
FILL_EDGE_Compare(FILL_EDGE* Edge1, FILL_EDGE* Edge2)
{
  int e1 = Edge1->XIntercept[0] + Edge1->XIntercept[1];
  int e2 = Edge2->XIntercept[0] + Edge2->XIntercept[1];

  return e1 - e2;
}


/*
** Insert an edge into a list keeping the list in order.
*/
static
void
FASTCALL
POLYGONFILL_ActiveListInsert(FILL_EDGE** activehead, FILL_EDGE* NewEdge )
{
  FILL_EDGE *pPrev, *pThis;
  //DPRINT1("In POLYGONFILL_ActiveListInsert()\n");
  ASSERT ( activehead && NewEdge );
  if ( !*activehead )
  {
    NewEdge->pNext = NULL;
    *activehead = NewEdge;
    return;
  }
  /*
  ** First lets check to see if we have a new smallest value.
  */
  if (FILL_EDGE_Compare(NewEdge, *activehead) <= 0)
  {
    NewEdge->pNext = *activehead;
    *activehead = NewEdge;
    return;
  }
  /*
  ** Ok, now scan to the next spot to put this item.
  */
  pThis = *activehead;
  pPrev = NULL;
  while ( pThis && FILL_EDGE_Compare(pThis, NewEdge) < 0 )
  {
    pPrev = pThis;
    pThis = pThis->pNext;
  }

  ASSERT(pPrev);
  NewEdge->pNext = pPrev->pNext;
  pPrev->pNext = NewEdge;
  //DEBUG_PRINT_ACTIVE_EDGELIST(*activehead);
}

/*
** Create a list of edges for a list of points.
*/
static
FILL_EDGE_LIST*
FASTCALL
POLYGONFILL_MakeEdgeList(PPOINT Points, int Count)
{
  int CurPt = 0;
  FILL_EDGE_LIST* list = 0;
  FILL_EDGE* e = 0;

  if ( 0 == Points || 2 > Count )
    return 0;

  list = (FILL_EDGE_LIST*)EngAllocMem(FL_ZERO_MEMORY, sizeof(FILL_EDGE_LIST), FILL_EDGE_ALLOC_TAG);
  if ( 0 == list )
    goto fail;
  list->Count = 0;
  list->Edges = (FILL_EDGE**)EngAllocMem(FL_ZERO_MEMORY, Count*sizeof(FILL_EDGE*), FILL_EDGE_ALLOC_TAG);
  if ( !list->Edges )
    goto fail;

  memset ( list->Edges, 0, Count * sizeof(FILL_EDGE*) );

  for ( CurPt = 1; CurPt < Count; ++CurPt )
  {
    e = POLYGONFILL_MakeEdge ( Points[CurPt-1], Points[CurPt] );
    if ( !e )
      goto fail;

    // if a straight horizontal line - who cares?
    if ( !e->absdy )
      EngFreeMem ( e );
    else
      list->Edges[list->Count++] = e;
  }
  e = POLYGONFILL_MakeEdge ( Points[CurPt-1], Points[0] );
  if ( !e )
    goto fail;

  if ( !e->absdy )
    EngFreeMem ( e );
  else
    list->Edges[list->Count++] = e;
  return list;

fail:

  DPRINT1("Out Of MEMORY!!\n");
  POLYGONFILL_DestroyEdgeList ( list );
  return 0;
}


/*
** This slow routine uses the data stored in the edge list to
** calculate the x intercepts for each line in the edge list
** for scanline Scanline.
**TODO: Get rid of this floating point arithmetic
*/
static
void
FASTCALL
POLYGONFILL_UpdateScanline(FILL_EDGE* pEdge, int Scanline)
{
  if ( 0 == pEdge->dy )
    return;

  ASSERT ( pEdge->FromY <= Scanline && pEdge->ToY > Scanline );

  if ( pEdge->xmajor )
  {
    int steps;

    ASSERT ( pEdge->y == Scanline );

    // now shoot to end of scanline collision
    steps = (pEdge->ErrorMax-pEdge->Error-1)/pEdge->absdy;
    if ( steps )
    {
      // record first collision with scanline
      int x1 = pEdge->x;
      pEdge->x += steps * pEdge->XDirection;
      pEdge->Error += steps * pEdge->absdy;
      ASSERT ( pEdge->Error < pEdge->ErrorMax );
      pEdge->XIntercept[0] = min(x1,pEdge->x);
      pEdge->XIntercept[1] = max(x1,pEdge->x);
    }
    else
    {
      pEdge->XIntercept[0] = pEdge->x;
      pEdge->XIntercept[1] = pEdge->x;
    }

    // we should require exactly 1 step to step onto next scanline...
    ASSERT ( (pEdge->ErrorMax-pEdge->Error-1) / pEdge->absdy == 0 );
    pEdge->x += pEdge->XDirection;
    pEdge->Error += pEdge->absdy;
    ASSERT ( pEdge->Error >= pEdge->ErrorMax );

    // now step onto next scanline...
    pEdge->Error -= pEdge->absdx;
    pEdge->y++;
  }
  else // then this is a y-major line
  {
    pEdge->XIntercept[0] = pEdge->x;
    pEdge->XIntercept[1] = pEdge->x;

    pEdge->Error += pEdge->absdx;
    pEdge->y++;

    if ( pEdge->Error >= pEdge->ErrorMax )
    {
      pEdge->Error -= pEdge->ErrorMax;
      pEdge->x += pEdge->XDirection;
      ASSERT ( pEdge->Error < pEdge->ErrorMax );
    }
  }

  //DPRINT("Line (%d, %d) to (%d, %d) intersects scanline %d at (%d,%d)\n",
  //        pEdge->FromX, pEdge->FromY, pEdge->ToX, pEdge->ToY, Scanline, pEdge->XIntercept[0], pEdge->XIntercept[1] );
}

/*
** This method updates the Active edge collection for the scanline Scanline.
*/
static
void
APIENTRY
POLYGONFILL_BuildActiveList ( int Scanline, FILL_EDGE_LIST* list, FILL_EDGE** ActiveHead )
{
  int i;

  ASSERT ( list && ActiveHead );
  *ActiveHead = 0;
  for ( i = 0; i < list->Count; i++ )
  {
    FILL_EDGE* pEdge = list->Edges[i];
    ASSERT(pEdge);
    if ( pEdge->FromY <= Scanline && pEdge->ToY > Scanline )
    {
      POLYGONFILL_UpdateScanline ( pEdge, Scanline );
      POLYGONFILL_ActiveListInsert ( ActiveHead, pEdge );
    }
  }
}

/*
** This method fills the portion of the polygon that intersects with the scanline
** Scanline.
*/
static
void
APIENTRY
POLYGONFILL_FillScanLineAlternate(
  PDC dc,
  int ScanLine,
  FILL_EDGE* ActiveHead,
  SURFACE *psurf,
  BRUSHOBJ *BrushObj,
  MIX RopMode )
{
  FILL_EDGE *pLeft, *pRight;

  if ( !ActiveHead )
    return;

  pLeft = ActiveHead;
  pRight = pLeft->pNext;
  ASSERT(pRight);

  while ( NULL != pRight )
  {
    int x1 = pLeft->XIntercept[0];
    int x2 = pRight->XIntercept[1];
    if ( x2 > x1 )
    {
      RECTL BoundRect;
      BoundRect.top = ScanLine;
      BoundRect.bottom = ScanLine + 1;
      BoundRect.left = x1;
      BoundRect.right = x2;

      //DPRINT("Fill Line (%d, %d) to (%d, %d)\n",x1, ScanLine, x2, ScanLine);
      IntEngLineTo(&psurf->SurfObj,
                   dc->rosdc.CombinedClip,
                   BrushObj,
                   x1,
                   ScanLine,
                   x2,
                   ScanLine,
                   &BoundRect, // Bounding rectangle
                   RopMode); // MIX
    }
    pLeft = pRight->pNext;
    pRight = pLeft ? pLeft->pNext : NULL;
  }
}

static
void
APIENTRY
POLYGONFILL_FillScanLineWinding(
  PDC dc,
  int ScanLine,
  FILL_EDGE* ActiveHead,
  SURFACE *psurf,
  BRUSHOBJ *BrushObj,
  MIX RopMode )
{
  FILL_EDGE *pLeft, *pRight;
  int x1, x2, winding = 0;
  RECTL BoundRect;

  if ( !ActiveHead )
    return;

  BoundRect.top = ScanLine;
  BoundRect.bottom = ScanLine + 1;

  pLeft = ActiveHead;
  winding = pLeft->YDirection;
  pRight = pLeft->pNext;
  ASSERT(pRight);

  // setup first line...
  x1 = pLeft->XIntercept[0];
  x2 = pRight->XIntercept[1];

  pLeft = pRight;
  pRight = pLeft->pNext;
  winding += pLeft->YDirection;

  while ( NULL != pRight )
  {
    int newx1 = pLeft->XIntercept[0];
    int newx2 = pRight->XIntercept[1];
    if ( winding )
    {
      // check and see if this new line touches the previous...
      if ( (newx1 >= x1 && newx1 <= x2)
	|| (newx2 >= x1 && newx2 <= x2)
	|| (x1 >= newx1 && x1 <= newx2)
	|| (x2 >= newx2 && x2 <= newx2)
	)
      {
	// yup, just tack it on to our existing line
	x1 = min(x1,newx1);
	x2 = max(x2,newx2);
      }
      else
      {
	// nope - render the old line..
	BoundRect.left = x1;
	BoundRect.right = x2;

	//DPRINT("Fill Line (%d, %d) to (%d, %d)\n",x1, ScanLine, x2, ScanLine);
	IntEngLineTo(&psurf->SurfObj,
                     dc->rosdc.CombinedClip,
                     BrushObj,
                     x1,
                     ScanLine,
                     x2,
                     ScanLine,
                     &BoundRect, // Bounding rectangle
                     RopMode); // MIX

	x1 = newx1;
	x2 = newx2;
      }
    }
    pLeft = pRight;
    pRight = pLeft->pNext;
    winding += pLeft->YDirection;
  }
  // there will always be a line left-over, render it now...
  BoundRect.left = x1;
  BoundRect.right = x2;

  //DPRINT("Fill Line (%d, %d) to (%d, %d)\n",x1, ScanLine, x2, ScanLine);
  IntEngLineTo(&psurf->SurfObj,
               dc->rosdc.CombinedClip,
               BrushObj,
               x1,
               ScanLine,
               x2,
               ScanLine,
               &BoundRect, // Bounding rectangle
               RopMode); // MIX
}

//When the fill mode is ALTERNATE, GDI fills the area between odd-numbered and
//even-numbered polygon sides on each scan line. That is, GDI fills the area between the
//first and second side, between the third and fourth side, and so on.

//WINDING Selects winding mode (fills any region with a nonzero winding value).
//When the fill mode is WINDING, GDI fills any region that has a nonzero winding value.
//This value is defined as the number of times a pen used to draw the polygon would go around the region.
//The direction of each edge of the polygon is important.

BOOL
APIENTRY
FillPolygon(
  PDC dc,
  SURFACE *psurf,
  BRUSHOBJ *BrushObj,
  MIX RopMode,
  CONST PPOINT Points,
  int Count,
  RECTL BoundRect )
{
  FILL_EDGE_LIST *list = 0;
  FILL_EDGE *ActiveHead = 0;
  int ScanLine;
  PDC_ATTR pdcattr = dc->pdcattr;
  void
  (APIENTRY *FillScanLine)(
    PDC dc,
    int ScanLine,
    FILL_EDGE* ActiveHead,
    SURFACE *psurf,
    BRUSHOBJ *BrushObj,
    MIX RopMode );

  //DPRINT("FillPolygon\n");

  /* Create Edge List. */
  list = POLYGONFILL_MakeEdgeList(Points, Count);
  /* DEBUG_PRINT_EDGELIST(list); */
  if (NULL == list)
    return FALSE;

  if ( WINDING == pdcattr->jFillMode )
    FillScanLine = POLYGONFILL_FillScanLineWinding;
  else /* default */
    FillScanLine = POLYGONFILL_FillScanLineAlternate;

  /* For each Scanline from BoundRect.bottom to BoundRect.top,
   * determine line segments to draw
   */
  for ( ScanLine = BoundRect.top; ScanLine < BoundRect.bottom; ++ScanLine )
  {
    POLYGONFILL_BuildActiveList(ScanLine, list, &ActiveHead);
    //DEBUG_PRINT_ACTIVE_EDGELIST(ActiveHead);
    FillScanLine ( dc, ScanLine, ActiveHead, psurf, BrushObj, RopMode );
  }

  /* Free Edge List. If any are left. */
  POLYGONFILL_DestroyEdgeList(list);

  return TRUE;
}

BOOL FASTCALL
IntFillPolygon(
    PDC dc,
    SURFACE *psurf,
    BRUSHOBJ *BrushObj,
    CONST PPOINT Points,
    int Count,
    RECTL DestRect, 
    POINTL *BrushOrigin)
{
    FILL_EDGE_LIST *list = 0;
    FILL_EDGE *ActiveHead = 0;
    FILL_EDGE *pLeft, *pRight;
    int ScanLine;
  
    //DPRINT("IntFillPolygon\n");

    /* Create Edge List. */
    list = POLYGONFILL_MakeEdgeList(Points, Count);
    /* DEBUG_PRINT_EDGELIST(list); */
    if (NULL == list)
        return FALSE;

    /* For each Scanline from DestRect.top to DestRect.bottom, determine line segments to draw */
    for ( ScanLine = DestRect.top; ScanLine < DestRect.bottom; ++ScanLine )
    {
        POLYGONFILL_BuildActiveList(ScanLine, list, &ActiveHead);
        //DEBUG_PRINT_ACTIVE_EDGELIST(ActiveHead);

        if ( !ActiveHead )
          return FALSE;

        pLeft = ActiveHead;
        pRight = pLeft->pNext;
        ASSERT(pRight);

        while ( NULL != pRight )
        {
            int x1 = pLeft->XIntercept[0];
            int x2 = pRight->XIntercept[1];
            if ( x2 > x1 )
            {
                RECTL LineRect;
                LineRect.top = ScanLine;
                LineRect.bottom = ScanLine + 1;
                LineRect.left = x1;
                LineRect.right = x2;

                IntEngBitBlt(&psurf->SurfObj,
                                 NULL,
                                 NULL,
                                 dc->rosdc.CombinedClip,
                                 NULL,
                                 &LineRect,
                                 NULL,
                                 NULL,
                                 BrushObj,
                                 BrushOrigin,
                                 ROP3_TO_ROP4(PATCOPY));
            }
            pLeft = pRight->pNext;
            pRight = pLeft ? pLeft->pNext : NULL;
        }   
    }

    /* Free Edge List. If any are left. */
    POLYGONFILL_DestroyEdgeList(list);

    return TRUE;
}

/* EOF */
