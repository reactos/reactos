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
/* $Id: polyfill.c,v 1.4 2003/06/25 16:55:33 gvg Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Various Polygon Filling routines for Polygon()
 * FILE:              subsys/win32k/objects/polyfill.c
 * PROGRAMER:         Mark Tempel
 * REVISION HISTORY:
 *                 21/2/2003: Created
 */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/fillshap.h>
#include <win32k/dc.h>
#include <win32k/pen.h>
#include <include/object.h>
#include <include/paint.h>

#define NDEBUG
#include <win32k/debug1.h>

#define PFILL_EDGE_ALLOC_TAG 0x45465044

/*
** This struct is used for book keeping during polygon filling routines.
*/
typedef struct _tagPFILL_EDGE
{
    /*Basic line information*/
	int FromX;
	int FromY;
	int ToX;
	int ToY;
	int dx;
	int dy;
	int MinX;
	int MaxX;
	int MinY;
	int MaxY;
    
    /*Active Edge List information*/
    int XIntercept;
    int ErrorTerm;
    int ErrorTermAdjUp;
    int ErrorTermAdjDown;
    int XPerY;
    int Direction;

    /* The next edge in the Edge List*/
	struct _tagPFILL_EDGE * pNext;
} PFILL_EDGE, *PPFILL_EDGE;

typedef PPFILL_EDGE	PFILL_EDGE_LIST;

static void DEBUG_PRINT_EDGELIST(PFILL_EDGE_LIST list)
{
    PPFILL_EDGE pThis = list;
    if (0 == list)
    {
        DPRINT("List is NULL\n");
        return;
    }
    
    while(0 != pThis)
    {
        DPRINT("EDGE: (%d, %d) to (%d, %d)\n", pThis->FromX, pThis->FromY, pThis->ToX, pThis->ToY);
        pThis = pThis->pNext;
    }
}

/*
**  Hide memory clean up.
*/
static void FASTCALL POLYGONFILL_DestroyEdge(PPFILL_EDGE pEdge)
{
    if (0 != pEdge)
        EngFreeMem(pEdge);
}

/*
** Clean up a list.
*/
static void FASTCALL POLYGONFILL_DestroyEdgeList(PFILL_EDGE_LIST list)
{
    PPFILL_EDGE pThis = 0;
    PPFILL_EDGE pNext = 0;

    pThis = list;
    while (0 != pThis)
    {  
        //DPRINT("Destroying Edge\n");
        pNext = pThis->pNext;
        POLYGONFILL_DestroyEdge(pThis);
        pThis = pNext;
    } 
    
}

/*
** This makes and initiaizes an Edge struct for a line between two points.
*/
static PPFILL_EDGE FASTCALL POLYGONFILL_MakeEdge(POINT From, POINT To)
{
    PPFILL_EDGE rc = (PPFILL_EDGE)EngAllocMem(FL_ZERO_MEMORY, sizeof(PFILL_EDGE), PFILL_EDGE_ALLOC_TAG);

    if (0 != rc)
    {
        //DPRINT("Making Edge: (%d, %d) to (%d, %d)\n", From.x, From.y, To.x, To.y);
        //Now Fill the struct.
	    rc->FromX = From.x;
	    rc->FromY = From.y;
	    rc->ToX = To.x;
	    rc->ToY = To.y;
	    
	    rc->dx   = To.x - From.x;
	    rc->dy   = To.y - From.y;
    	rc->MinX = MIN(To.x, From.x);
	    rc->MaxX = MAX(To.x, From.x);
	    rc->MinY = MIN(To.y, From.y);
	    rc->MaxY = MAX(To.y, From.y);

        if (rc->MinY == To.y)
            rc->XIntercept = To.x;
        else
            rc->XIntercept = From.x;

        rc->ErrorTermAdjDown = rc->dy;
        rc->Direction   = (rc->dx < 0)?(-1):(1);
        
        if (rc->dx >= 0) /*edge goes l to r*/
        {
            rc->ErrorTerm   = 0;
        }
        else/*edge goes r to l*/
        {
            rc->ErrorTerm = -rc->dy +1;
        }

        /*Now which part of the slope is greater?*/
        if (rc->dy == 0)
        {
            rc->XPerY = 0;
            rc->ErrorTermAdjUp = 0;
        }
        else if (rc->dy >= rc->dx)
        {
            rc->XPerY = 0;
            rc->ErrorTermAdjUp = rc->dx;
        }
        else 
        {
            rc->XPerY = rc->dx / rc->dy;
            rc->ErrorTermAdjUp = rc->dx % rc->dy;
        }
        
        rc->pNext = 0;
    }
    return rc;
}
/*
** My Edge comparison routine.
** This is for scan converting polygon fill.
** Fist sort by MinY, then Minx, then slope.
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
static INT FASTCALL PFILL_EDGE_Compare(PPFILL_EDGE Edge1, PPFILL_EDGE Edge2)
{
    //DPRINT("In PFILL_EDGE_Compare()\n");
	if (Edge1->MinY == Edge2->MinY)
    {
        //DPRINT("In PFILL_EDGE_Compare() MinYs are equal\n");
        if (Edge1->MinX == Edge2->MinX)
        {
            if (0 == Edge2->dx || 0 == Edge1->dx)
            {
                return Edge1->dx - Edge2->dx;
            }
            else
            {
                return (Edge1->dy/Edge1->dx) - (Edge2->dy/Edge2->dx);
            }
        }
        else
        {
            return Edge1->MinX - Edge2->MinX;
        }
    }
    //DPRINT("In PFILL_EDGE_Compare() returning: %d\n",Edge1->MinY - Edge2->MinY);
    return Edge1->MinY - Edge2->MinY;
    
}


/*
** Insert an edge into a list keeping the list in order.
*/
static void FASTCALL POLYGONFILL_ListInsert(PFILL_EDGE_LIST *list, PPFILL_EDGE NewEdge)
{
    PPFILL_EDGE pThis;
    if (0 != list && 0 != NewEdge)
    {
        pThis = *list;
        //DPRINT("In POLYGONFILL_ListInsert()\n");
        /*
        ** First lets check to see if we have a new smallest value.
        */
        if (0 < PFILL_EDGE_Compare(pThis, NewEdge))
        {
            NewEdge->pNext = pThis;
            *list = NewEdge;
            return;
        }
        /*
        ** Ok, now scan to the next spot to put this item.
        */
        while (0 > PFILL_EDGE_Compare(pThis, NewEdge))
        {   
            if (0 == pThis->pNext)
                break;

            pThis = pThis->pNext;
        }
        
        NewEdge->pNext = pThis->pNext;
        pThis->pNext = NewEdge;
        //DEBUG_PRINT_EDGELIST(*list);
    }
}

/*
** Create a list of edges for a list of points.
*/
static PFILL_EDGE_LIST FASTCALL POLYGONFILL_MakeEdgeList(PPOINT Points, int Count)
{
	int CurPt = 0;
	int SeqNum = 0;
    PPFILL_EDGE rc = 0;
    PPFILL_EDGE NextEdge = 0;
	
	if (0 != Points && 2 <= Count)
	{
        //Establish the list with the first two points.
		rc = POLYGONFILL_MakeEdge(Points[0],Points[1]); 
		if (0 == rc) return rc;

		for (CurPt = 1; CurPt < Count; ++CurPt,++SeqNum)
		{		
		    if (CurPt == Count - 1)
            {
                NextEdge = POLYGONFILL_MakeEdge(Points[CurPt],Points[0]);
            }
            else
            {
                NextEdge = POLYGONFILL_MakeEdge(Points[CurPt],Points[CurPt + 1]);
            }
            if (0 != NextEdge)
            {
                POLYGONFILL_ListInsert(&rc, NextEdge);
            }
            else
            {
                DPRINT("Out Of MEMORY!! NextEdge = 0\n");
            }
		}
	}
	return rc;
}


/*
** This slow routine uses the data stored in the edge list to 
** calculate the x intercepts for each line in the edge list
** for scanline Scanline.
**TODO: Get rid of this floating point arithmetic
*/
static void FASTCALL POLYGONFILL_UpdateScanline(PPFILL_EDGE pEdge, int Scanline)
{
    
    int Coord = 0;
    float XCoord = 0;
    if (0 == pEdge->dy) return;

    XCoord = (Scanline*pEdge->dx - pEdge->FromY*pEdge->dx)/pEdge->dy + pEdge->FromX;
    Coord = XCoord + 0.5;

    //DPRINT("Line (%d, %d) to (%d, %d) intersects scanline %d at %d\n",
    //        pEdge->FromX, pEdge->FromY, pEdge->ToX, pEdge->ToY, Scanline, Coord);
    pEdge->XIntercept = Coord;


    /*pEdge->XIntercept += pEdge->XPerY;

    if ((pEdge->ErrorTerm += pEdge->ErrorTermAdjUp) > 0)
    {
        pEdge->XIntercept += pEdge->Direction;
        pEdge->ErrorTerm -= pEdge->ErrorTermAdjDown;
    }*/
}

/*
** This routine removes an edge from the global edge list and inserts it into
** the active edge list (preserving the order).
** An edge is considered Active if the current scanline intersects it.
**
** Note: once an edge is no longer active, it is deleted.
*/
static void FASTCALL POLYGONFILL_AECInsertInOrder(PFILL_EDGE_LIST *list, PPFILL_EDGE pEdge)
{
    BOOL Done = FALSE;
    PPFILL_EDGE pThis = 0;
    PPFILL_EDGE pPrev = 0;
    pThis = *list;
    while(0 != pThis && !Done)
    {
        /*pEdge goes before pThis*/
        if (pThis->XIntercept > pEdge->XIntercept)
        {
            if (*list == pThis)
            {
                *list = pEdge;
            }
            else
            {
                pPrev->pNext = pEdge;
            }
            pEdge->pNext = pThis;
            Done = TRUE;
        }
        pPrev = pThis;
        pThis = pThis->pNext;
    }
}

/*
** This routine reorders the Active Edge collection (list) after all
** the now inactive edges have been removed.
*/
static void FASTCALL POLYGONFILL_AECReorder(PFILL_EDGE_LIST *AEC)
{
    PPFILL_EDGE pThis = 0;
    PPFILL_EDGE pPrev = 0;
    PPFILL_EDGE pTarg = 0; 
    pThis = *AEC;
    
    while (0 != pThis)
    {
        /*We are at the end of the list*/
        if (0 == pThis->pNext)
        {
            return;
        }

        /*If the next item is out of order, pull it from the list and
        re-insert it, and don't advance pThis.*/
        if (pThis->XIntercept > pThis->pNext->XIntercept)
        {
            pTarg = pThis->pNext;
            pThis->pNext = pTarg->pNext;
            pTarg->pNext = 0;
            POLYGONFILL_AECInsertInOrder(AEC, pTarg);
        }
        else/*In order, advance pThis*/
        {
            pPrev = pThis;
            pThis = pThis->pNext;
        }
    }
    
}
/*
** This method updates the Active edge collection for the scanline Scanline.
*/
static void STDCALL POLYGONFILL_UpdateActiveEdges(int Scanline, PFILL_EDGE_LIST *GEC, PFILL_EDGE_LIST *AEC)
{
    PPFILL_EDGE pThis = 0;
    PPFILL_EDGE pAECLast = 0;
    PPFILL_EDGE pPrev = 0;
    DPRINT("In POLYGONFILL_UpdateActiveEdges() Scanline: %d\n", Scanline);
    /*First scan through GEC and look for any edges that have become active*/
    pThis = *GEC;
    while (0 != pThis && pThis->MinY <= Scanline)
    {
        //DPRINT("Moving Edge to AEC\n");
        /*Remove the edge from GEC and put it into AEC*/
        if (pThis->MinY <= Scanline)
        {
            /*Always peel off the front of the GEC*/
            *GEC = pThis->pNext;
            
            /*Now put this edge at the end of AEC*/
            if (0 == *AEC)
            {
                *AEC = pThis;
                pThis->pNext = 0;
                pAECLast = pThis;
            }
            else if(0 == pAECLast)
            {
                pAECLast = *AEC;
                while(0 != pAECLast->pNext)
                {
                    pAECLast = pAECLast->pNext;
                }
             
                pAECLast->pNext = pThis;             
                pThis->pNext = 0;
                pAECLast = pThis;
            }
            else
            {
                pAECLast->pNext = pThis;
                pThis->pNext = 0;
                pAECLast = pThis;

            }
        }
        
        pThis = *GEC;
    }
    /*Now remove any edges in the AEC that are no longer active and Update the XIntercept in the AEC*/
    pThis = *AEC;
    while (0 != pThis)
    {
        /*First check to see if this item is deleted*/
        if (pThis->MaxY <= Scanline)
        {
            //DPRINT("Removing Edge from AEC\n");
            if (0 == pPrev)/*First element in the list*/
            {
                *AEC = pThis->pNext;
            }
            else
            {
                pPrev->pNext = pThis->pNext;
            }
            POLYGONFILL_DestroyEdge(pThis);
        }
        else/*Otherwise, update the scanline*/
        {
            POLYGONFILL_UpdateScanline(pThis, Scanline);
            pPrev = pThis; 
        }
        /*List Upkeep*/
        if (0 == pPrev)/*First element in the list*/
        {
            pThis = *AEC;
        }
        else
        {
            pThis = pPrev->pNext;
        }
    }
    /*Last re Xintercept order the AEC*/
    POLYGONFILL_AECReorder(AEC);

}

/*
** This method fills the portion of the polygon that intersects with the scanline
** Scanline.
*/
static void STDCALL POLYGONFILL_FillScanLine(int ScanLine, PFILL_EDGE_LIST ActiveEdges, SURFOBJ *SurfObj, PBRUSHOBJ BrushObj, MIX RopMode)
{
  BOOL OnOdd = TRUE;
  RECTL BoundRect;
  int XInterceptOdd,XInterceptEven,ret;
  PPFILL_EDGE pThis = ActiveEdges;
    
  while (NULL != pThis)
    {
      if (OnOdd)
	{
	  XInterceptOdd = pThis->XIntercept;
	  OnOdd = FALSE;
	}
      else
	{
	  BoundRect.top = ScanLine - 1;
	  BoundRect.bottom = ScanLine + 1;
	  BoundRect.left = XInterceptOdd - 2;
	  BoundRect.right = XInterceptEven;

	  XInterceptEven = pThis->XIntercept;
	  DPRINT("Fill Line (%d, %d) to (%d, %d)\n",XInterceptOdd - 1, ScanLine, XInterceptEven - 1, ScanLine);
	  ret = EngLineTo(SurfObj,
	                  NULL, /* ClipObj */
	                  BrushObj,
		          XInterceptOdd - 1, 
		          ScanLine, 
	                  XInterceptEven - 1, 
	                  ScanLine,
                          &BoundRect, /* Bounding rectangle */
                          RopMode); /* MIX */
	  OnOdd = TRUE;
	}
      pThis = pThis->pNext;
    }
}

//ALTERNATE Selects alternate mode (fills the area between odd-numbered and even-numbered 
//polygon sides on each scan line). 
//When the fill mode is ALTERNATE, GDI fills the area between odd-numbered and 
//even-numbered polygon sides on each scan line. That is, GDI fills the area between the 
//first and second side, between the third and fourth side, and so on. 
BOOL STDCALL FillPolygon_ALTERNATE(SURFOBJ *SurfObj, PBRUSHOBJ BrushObj, MIX RopMode, CONST PPOINT Points, int Count, RECTL BoundRect)
{
  PFILL_EDGE_LIST list = 0;
  PFILL_EDGE_LIST ActiveEdges = 0;
  int ScanLine;

  DPRINT("FillPolygon_ALTERNATE\n");
	
  /* Create Edge List. */
  list = POLYGONFILL_MakeEdgeList(Points, Count);
  /* DEBUG_PRINT_EDGELIST(list); */
  if (NULL == list)
    {
      return FALSE;
    }

  /* For each Scanline from BoundRect.bottom to BoundRect.top, 
   * determine line segments to draw
   */
  for (ScanLine = BoundRect.top + 1; ScanLine < BoundRect.bottom ; ++ScanLine)
    {
      POLYGONFILL_UpdateActiveEdges(ScanLine, &list, &ActiveEdges);
      /* DEBUG_PRINT_EDGELIST(ActiveEdges); */
      POLYGONFILL_FillScanLine(ScanLine, ActiveEdges, SurfObj, BrushObj, RopMode);
    }
    
  /* Free Edge List. If any are left. */
  POLYGONFILL_DestroyEdgeList(list);

  return TRUE;
}

//WINDING Selects winding mode (fills any region with a nonzero winding value). 
//When the fill mode is WINDING, GDI fills any region that has a nonzero winding value. 
//This value is defined as the number of times a pen used to draw the polygon would go around the region. 
//The direction of each edge of the polygon is important. 
BOOL STDCALL FillPolygon_WINDING(SURFOBJ *SurfObj, PBRUSHOBJ BrushObj,MIX RopMode, CONST PPOINT Points, int Count, RECTL BoundRect)
{
  DPRINT("FillPolygon_WINDING\n");

  return FALSE;
}
/* EOF */
