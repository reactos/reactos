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
/* $Id: gradient.c,v 1.3 2004/02/09 16:37:59 weiden Exp $
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

const LONG LINC[2] = {-1, 1};

#define VERTEX(n) (pVertex + gt->n)
#define COMPAREVERTEX(a, b) ((a)->x == (b)->x && (a)->y == (b)->y)

#define VCMPCLR(a,b,c,color) (a->color != b->color || a->color != c->color)
#define VCMPCLRS(a,b,c) \
  !(!VCMPCLR(a,b,c,Red) || !VCMPCLR(a,b,c,Green) || !VCMPCLR(a,b,c,Blue))

#define MOVERECT(r,x,y) \
  r.left += x; r.right += x; \
  r.top += y; r.bottom += y
  
  
/* Horizontal/Vertical gradients */
#define HVINITCOL(Col, id) \
  c[id] = v1->Col >> 8; \
  dc[id] = abs((v2->Col >> 8) - c[id]); \
  ec[id] = -(dy >> 1); \
  ic[id] = LINC[(v2->Col >> 8) > c[id]]
#define HVSTEPCOL(id) \
  ec[id] += dc[id]; \
  while(ec[id] > 0) \
  { \
    c[id] += ic[id]; \
    ec[id] -= dy; \
  }

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
    IN BOOL Horizontal)
{
  SURFOBJ *OutputObj;
  SURFGDI *OutputGDI;
  TRIVERTEX *v1, *v2;
  RECT rcGradient, rcSG;
  RECT_ENUM RectEnum;
  BOOL EnumMore;
  ULONG i;
  POINTL Translate;
  INTENG_ENTER_LEAVE EnterLeave;
  LONG y, dy, c[3], dc[3], ec[3], ic[3];
  
  v1 = (pVertex + gRect->UpperLeft);
  v2 = (pVertex + gRect->LowerRight);
  
  rcGradient.left = min(v1->x, v2->x);
  rcGradient.right = max(v1->x, v2->x);
  rcGradient.top = min(v1->y, v2->y);
  rcGradient.bottom = max(v1->y, v2->y);
  rcSG = rcGradient;
  MOVERECT(rcSG, pptlDitherOrg->x, pptlDitherOrg->y);
  
  if(Horizontal)
  {
    dy = abs(rcGradient.right - rcGradient.left);
  }
  else
  {
    dy = abs(rcGradient.bottom - rcGradient.top);
  }
  
  if(!IntEngEnter(&EnterLeave, psoDest, &rcSG, FALSE, &Translate, &OutputObj))
  {
    return FALSE;
  }
  OutputGDI = AccessInternalObjectFromUserObject(OutputObj);
  
  if((v1->Red != v2->Red || v1->Green != v2->Green || v1->Blue != v2->Blue) && dy > 1)
  {
    CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_RIGHTDOWN, 0);
    do
    {
      RECT FillRect;
      ULONG Color;
      
      if(Horizontal)
      {
        EnumMore = CLIPOBJ_bEnum(pco, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
        for (i = 0; i < RectEnum.c && RectEnum.arcl[i].top <= rcSG.bottom; i++)
        {
          if(NtGdiIntersectRect(&FillRect, &RectEnum.arcl[i], &rcSG))
          {
            HVINITCOL(Red, 0);
            HVINITCOL(Green, 1);
            HVINITCOL(Blue, 2);
            
            for(y = rcSG.left; y < FillRect.right; y++)
            {
              if(y >= FillRect.left)
              {
                Color = XLATEOBJ_iXlate(pxlo, RGB(c[0], c[1], c[2]));
                OutputGDI->DIB_VLine(OutputObj, y, FillRect.top, FillRect.bottom, Color);
              }
              HVSTEPCOL(0);
              HVSTEPCOL(1);
              HVSTEPCOL(2);
            }
          }
        }
        
        continue;
      }
      
      /* vertical */
      EnumMore = CLIPOBJ_bEnum(pco, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
      for (i = 0; i < RectEnum.c && RectEnum.arcl[i].top <= rcSG.bottom; i++)
      {
        if(NtGdiIntersectRect(&FillRect, &RectEnum.arcl[i], &rcSG))
        {
          HVINITCOL(Red, 0);
          HVINITCOL(Green, 1);
          HVINITCOL(Blue, 2);
          
          for(y = rcSG.top; y < FillRect.bottom; y++)
          {
            if(y >= FillRect.top)
            {
              Color = XLATEOBJ_iXlate(pxlo, RGB(c[0], c[1], c[2]));
              OutputGDI->DIB_HLine(OutputObj, FillRect.left, FillRect.right, y, Color);
            }
            HVSTEPCOL(0);
            HVSTEPCOL(1);
            HVSTEPCOL(2);
          }
        }
      }
      
    } while(EnumMore);

    return IntEngLeave(&EnterLeave);
  }
  
  /* rectangle has only one color, no calculation required */
  CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_RIGHTDOWN, 0);
  do
  {
    RECT FillRect;
    ULONG Color = XLATEOBJ_iXlate(pxlo, RGB(v1->Red, v1->Green, v1->Blue));
    
    EnumMore = CLIPOBJ_bEnum(pco, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
    for (i = 0; i < RectEnum.c && RectEnum.arcl[i].top <= rcSG.bottom; i++)
    {
      if(NtGdiIntersectRect(&FillRect, &RectEnum.arcl[i], &rcSG))
      {
        for(; FillRect.top < FillRect.bottom; FillRect.top++)
        {
          OutputGDI->DIB_HLine(OutputObj, FillRect.left, FillRect.right, FillRect.top, Color);
        }
      }
    }
  } while(EnumMore);
  
  return IntEngLeave(&EnterLeave);
}

#define NLINES 3
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
  ULONG i;
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
                                   pptlDitherOrg, (ulMode == GRADIENT_FILL_RECT_H)))
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
