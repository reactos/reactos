/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998 - 2004 ReactOS Team
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
/* $Id: gradient.c,v 1.1 2004/02/08 21:37:52 weiden Exp $
 * 
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Gradient Functions
 * FILE:              subsys/win32k/eng/gradient.c
 * PROGRAMER:         Thomas Weidenmueller
 * REVISION HISTORY:
 *                 3/7/1999: Created
 */

#include <ddk/winddi.h>
#include <ddk/ntddmou.h>
#include <include/object.h>
#include <include/paint.h>
#include <include/surface.h>
#include <include/rect.h>

#include "objects.h"
#include <include/mouse.h>
#include "../dib/dib.h"

#include "brush.h"
#include "clip.h"

//#define NDEBUG
#include <win32k/debug1.h>

/* MACROS *********************************************************************/

#define VERTEX(n) (pVertex + gt->n)
#define COMPAREVERTEX(a, b) ((a)->x == (b)->x && (a)->y == (b)->y)

/* FUNCTIONS ******************************************************************/

BOOL FASTCALL
IntEngGradientFillRect(
    IN SURFOBJ  *psoDest,
    IN CLIPOBJ  *pco,
    IN XLATEOBJ  *pxlo,
    IN TRIVERTEX  *pVertex,
    IN ULONG  nVertex,
    IN PGRADIENT_RECT gRect,
    IN RECTL  *prclExtents,
    IN POINTL  *pptlDitherOrg,
    IN BOOL Vertical)
{
  /*
  RECT_ENUM RectEnum;
  BOOL EnumMore;
  ULONG i;
  
  CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_RIGHTDOWN, 0);
  do
  {
    EnumMore = CLIPOBJ_bEnum(pco, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
    for (i = 0; i < RectEnum.c && RectEnum.arcl[i].top <= y1; i++)
    {
    
    }
  } while(EnumMore);
  */
  return FALSE;
}

#define NLINES 3
#define VCMPCLR(a,b,c,color) (a->color != b->color || a->color != c->color)
#define VCMPCLRS(a,b,c) \
  !(!VCMPCLR(a,b,c,Red) || !VCMPCLR(a,b,c,Green) || !VCMPCLR(a,b,c,Blue))

BOOL FASTCALL
IntEngGradientFillTriangle(
    IN SURFOBJ  *psoDest,
    IN CLIPOBJ  *pco,
    IN XLATEOBJ  *pxlo,
    IN TRIVERTEX  *pVertex,
    IN ULONG  nVertex,
    IN PGRADIENT_TRIANGLE gTriangle,
    IN RECTL  *prclExtents,
    IN POINTL  *pptlDitherOrg)
{
  PTRIVERTEX v1, v2, v3;
  RECT_ENUM RectEnum;
  BOOL EnumMore;
  POINTL Translate;
  INTENG_ENTER_LEAVE EnterLeave;
  
  v1 = (pVertex + gTriangle->Vertex1);
  v2 = (pVertex + gTriangle->Vertex2);
  v3 = (pVertex + gTriangle->Vertex3);
  
  /* sort */
  if(v1->y > v2->y)
  {
    TRIVERTEX *t;
    t = v1;
    v1 = v2;
    v2 = t;
  }
  if(v2->y > v3->y)
  {
    TRIVERTEX *t;
    t = v2;
    v2 = v3;
    v3 = t;
    if(v1->y > v2->y)
    {
      t = v1;
      v1 = v2;
      v2 = t;
    }
  }
  
  DbgPrint("Triangle: (%i,%i) (%i,%i) (%i,%i)\n", v1->x, v1->y, v2->x, v2->y, v3->x, v3->y);
  
  if(VCMPCLRS(v1, v2, v3))
  {
    CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_RIGHTDOWN, 0);
    do
    {
      LONG i;
      RECT FillRect;
      SURFOBJ *OutputObj;
      SURFGDI *OutputGDI;
      
      EnumMore = CLIPOBJ_bEnum(pco, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
      for (i = 0; i < RectEnum.c; i++)
      {
        if(NtGdiIntersectRect(&FillRect, &RectEnum.arcl[i], prclExtents))
        {
          if(!IntEngEnter(&EnterLeave, psoDest, &FillRect, FALSE, &Translate, &OutputObj))
          {
            return FALSE;
          }
          OutputGDI = AccessInternalObjectFromUserObject(OutputObj);
          
          /* FIXME - Render gradient triangle */
          
          if(!IntEngLeave(&EnterLeave))
          {
            return FALSE;
          }
        }
      }
    } while(EnumMore);
    
    return TRUE;
  }
  
  /* FIXME - fill triangle with one solid color */
  
  return FALSE;
}


BOOL FASTCALL STATIC
IntEngIsNULLTriangle(TRIVERTEX  *pVertex, GRADIENT_TRIANGLE *gt)
{
  if(COMPAREVERTEX(VERTEX(Vertex1), VERTEX(Vertex2)))
    return TRUE;
  if(COMPAREVERTEX(VERTEX(Vertex1), VERTEX(Vertex3)))
    return TRUE;
  if(COMPAREVERTEX(VERTEX(Vertex2), VERTEX(Vertex3)))
    return TRUE;
  return FALSE;
}


BOOL STDCALL
EngGradientFill(
    IN SURFOBJ  *psoDest,
    IN CLIPOBJ  *pco,
    IN XLATEOBJ  *pxlo,
    IN TRIVERTEX  *pVertex,
    IN ULONG  nVertex,
    IN PVOID  pMesh,
    IN ULONG  nMesh,
    IN RECTL  *prclExtents,
    IN POINTL  *pptlDitherOrg,
    IN ULONG  ulMode)
{
  ULONG i;
  
  switch(ulMode)
  {
    case GRADIENT_FILL_RECT_H:
    case GRADIENT_FILL_RECT_V:
    {
      PGRADIENT_RECT gr = (PGRADIENT_RECT)pMesh;
      for(i = 0; i < nMesh; i++, gr++)
      {
        if(!IntEngGradientFillRect(psoDest, pco, pxlo, pVertex, nVertex, gr, prclExtents,
                                   pptlDitherOrg, (ulMode == GRADIENT_FILL_RECT_V)))
        {
          return FALSE;
        }
      }
      return TRUE;
    }
    case GRADIENT_FILL_TRIANGLE:
    {
      PGRADIENT_TRIANGLE gt = (PGRADIENT_TRIANGLE)pMesh;
      for(i = 0; i < nMesh; i++, gt++)
      {
        if(IntEngIsNULLTriangle(pVertex, gt))
        {
          /* skip empty triangles */
          continue;
        }
        if(!IntEngGradientFillTriangle(psoDest, pco, pxlo, pVertex, nVertex, gt, prclExtents,
                                       pptlDitherOrg))
        {
          return FALSE;
        }
      }
      return TRUE;
    }
  }
  return FALSE;
}

BOOL STDCALL
IntEngGradientFill(
    IN SURFOBJ  *psoDest,
    IN CLIPOBJ  *pco,
    IN XLATEOBJ  *pxlo,
    IN TRIVERTEX  *pVertex,
    IN ULONG  nVertex,
    IN PVOID  pMesh,
    IN ULONG  nMesh,
    IN RECTL  *prclExtents,
    IN POINTL  *pptlDitherOrg,
    IN ULONG  ulMode)
{
  BOOL Ret;
  SURFGDI *SurfGDI;
  
  SurfGDI = (SURFGDI*)AccessInternalObjectFromUserObject(psoDest);
  MouseSafetyOnDrawStart(psoDest, SurfGDI, pco->rclBounds.left, pco->rclBounds.top, 
                         pco->rclBounds.right, pco->rclBounds.bottom);
  if((psoDest->iType != STYPE_BITMAP) && SurfGDI->GradientFill)
  {
    ExAcquireFastMutex(SurfGDI->DriverLock);
    Ret = SurfGDI->GradientFill(psoDest, pco, pxlo, pVertex, nVertex, pMesh, nMesh, 
                                prclExtents, pptlDitherOrg, ulMode);
    ExReleaseFastMutex(SurfGDI->DriverLock);
    MouseSafetyOnDrawEnd(psoDest, SurfGDI);
    return Ret;
  }
  Ret = EngGradientFill(psoDest, pco, pxlo, pVertex, nVertex, pMesh, nMesh, prclExtents, 
                        pptlDitherOrg, ulMode);
  if(Ret)
  {
    /* Dummy BitBlt to let driver know that something has changed.
       0x00AA0029 is the Rop for D (no-op) */
    if(SurfGDI->BitBlt)
    {
      ExAcquireFastMutex(SurfGDI->DriverLock);
      SurfGDI->BitBlt(psoDest, NULL, NULL, pco, pxlo,
                      prclExtents, pptlDitherOrg, NULL, NULL, NULL, 0x00AA0029);
      ExReleaseFastMutex(SurfGDI->DriverLock);
      MouseSafetyOnDrawEnd(psoDest, SurfGDI);
      return TRUE;
    }
    EngBitBlt(psoDest, NULL, NULL, pco, pxlo,
              prclExtents, pptlDitherOrg, NULL, NULL, NULL, 0x00AA0029);
  }
  MouseSafetyOnDrawEnd(psoDest, SurfGDI);
  return Ret;
}
