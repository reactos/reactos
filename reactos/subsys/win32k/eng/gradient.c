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
/* $Id: gradient.c,v 1.10 2004/07/03 13:55:35 navaraf Exp $
 * 
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Gradient Functions
 * FILE:              subsys/win32k/eng/gradient.c
 * PROGRAMER:         Thomas Weidenmueller
 * REVISION HISTORY:
 *                 3/7/1999: Created
 */
#include <w32k.h>

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
  TRIVERTEX *v1, *v2;
  RECTL rcGradient, rcSG;
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
          if(NtGdiIntersectRect(&FillRect, (PRECT)&RectEnum.arcl[i], (PRECT)&rcSG))
          {
            HVINITCOL(Red, 0);
            HVINITCOL(Green, 1);
            HVINITCOL(Blue, 2);
            
            for(y = rcSG.left; y < FillRect.right; y++)
            {
              if(y >= FillRect.left)
              {
                Color = XLATEOBJ_iXlate(pxlo, RGB(c[0], c[1], c[2]));
                DibFunctionsForBitmapFormat[OutputObj->iBitmapFormat].DIB_VLine(
                  OutputObj, y, FillRect.top, FillRect.bottom, Color);
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
        if(NtGdiIntersectRect(&FillRect, (PRECT)&RectEnum.arcl[i], (PRECT)&rcSG))
        {
          HVINITCOL(Red, 0);
          HVINITCOL(Green, 1);
          HVINITCOL(Blue, 2);
          
          for(y = rcSG.top; y < FillRect.bottom; y++)
          {
            if(y >= FillRect.top)
            {
              Color = XLATEOBJ_iXlate(pxlo, RGB(c[0], c[1], c[2]));
              DibFunctionsForBitmapFormat[OutputObj->iBitmapFormat].DIB_HLine(
                OutputObj, FillRect.left, FillRect.right, y, Color);
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
      if(NtGdiIntersectRect(&FillRect, (PRECT)&RectEnum.arcl[i], (PRECT)&rcSG))
      {
        for(; FillRect.top < FillRect.bottom; FillRect.top++)
        {
          DibFunctionsForBitmapFormat[OutputObj->iBitmapFormat].DIB_HLine(
            OutputObj, FillRect.left, FillRect.right, FillRect.top, Color);
        }
      }
    }
  } while(EnumMore);
  
  return IntEngLeave(&EnterLeave);
}

/* Fill triangle with solid color */
#define S_FILLLINE(linefrom,lineto) \
  if(sx[lineto] < sx[linefrom]) \
    DibFunctionsForBitmapFormat[OutputObj->iBitmapFormat].DIB_HLine(OutputObj, max(sx[lineto], FillRect.left), min(sx[linefrom], FillRect.right), sy, Color); \
  else \
    DibFunctionsForBitmapFormat[OutputObj->iBitmapFormat].DIB_HLine(OutputObj, max(sx[linefrom], FillRect.left), min(sx[lineto], FillRect.right), sy, Color);
#define S_DOLINE(a,b,line) \
  ex[line] += dx[line]; \
  while(ex[line] > 0 && x[line] != destx[line]) \
  { \
    x[line] += incx[line]; \
    sx[line] += incx[line]; \
    ex[line] -= dy[line]; \
  } 
#define S_GOLINE(a,b,line) \
  if(y >= a->y && y <= b->y) \
  {
#define S_ENDLINE(a,b,line) \
  }
#define S_INITLINE(a,b,line) \
  x[line] = a->x; \
  sx[line] =  a->x + pptlDitherOrg->x; \
  dx[line] = abs(b->x - a->x); \
  dy[line] = abs(b->y - a->y); \
  incx[line] = LINC[b->x > a->x]; \
  ex[line] = -(dy[line]>>1); \
  destx[line] = b->x

/* Fill triangle with gradient */
#define INITCOL(a,b,line,col,id) \
  c[line][id] = a->col >> 8; \
  dc[line][id] = abs((b->col >> 8) - c[line][id]); \
  ec[line][id] = -(dy[line]>>1); \
  ic[line][id] = LINC[(b->col >> 8) > c[line][id]]
#define STEPCOL(a,b,line,col,id) \
  ec[line][id] += dc[line][id]; \
  while(ec[line][id] > 0) \
  { \
    c[line][id] += ic[line][id]; \
    ec[line][id] -= dy[line]; \
  }
#define FINITCOL(linefrom,lineto,colid) \
  gc[colid] = c[linefrom][colid]; \
  gd[colid] = abs(c[lineto][colid] - gc[colid]); \
  ge[colid] = -(gx >> 1); \
  gi[colid] = LINC[c[lineto][colid] > gc[colid]]
#define FDOCOL(linefrom,lineto,colid) \
  ge[colid] += gd[colid]; \
  while(ge[colid] > 0) \
  { \
    gc[colid] += gi[colid]; \
    ge[colid] -= gx; \
  }
#define FILLLINE(linefrom,lineto) \
  gx = abs(sx[lineto] - sx[linefrom]); \
  gxi = LINC[sx[linefrom] < sx[lineto]]; \
  FINITCOL(linefrom, lineto, 0); \
  FINITCOL(linefrom, lineto, 1); \
  FINITCOL(linefrom, lineto, 2); \
  for(g = sx[linefrom]; g != sx[lineto]; g += gxi) \
  { \
    if(InY && g >= FillRect.left && g < FillRect.right) \
    { \
      Color = XLATEOBJ_iXlate(pxlo, RGB(gc[0], gc[1], gc[2])); \
      DibFunctionsForBitmapFormat[OutputObj->iBitmapFormat].DIB_PutPixel(OutputObj, g, sy, Color); \
    } \
    FDOCOL(linefrom, lineto, 0); \
    FDOCOL(linefrom, lineto, 1); \
    FDOCOL(linefrom, lineto, 2); \
  }
#define DOLINE(a,b,line) \
  STEPCOL(a, b, line, Red, 0); \
  STEPCOL(a, b, line, Green, 1); \
  STEPCOL(a, b, line, Blue, 2); \
  ex[line] += dx[line]; \
  while(ex[line] > 0 && x[line] != destx[line]) \
  { \
    x[line] += incx[line]; \
    sx[line] += incx[line]; \
    ex[line] -= dy[line]; \
  } 
#define GOLINE(a,b,line) \
  if(y >= a->y && y <= b->y) \
  {
#define ENDLINE(a,b,line) \
  }
#define INITLINE(a,b,line) \
  x[line] = a->x; \
  sx[line] =  a->x + pptlDitherOrg->x; \
  dx[line] = abs(b->x - a->x); \
  dy[line] = abs(b->y - a->y); \
  incx[line] = LINC[b->x > a->x]; \
  ex[line] = -(dy[line]>>1); \
  destx[line] = b->x
#define DOINIT(a, b, line) \
  INITLINE(a, b, line); \
  INITCOL(a, b, line, Red, 0); \
  INITCOL(a, b, line, Green, 1); \
  INITCOL(a, b, line, Blue, 2);
#define SMALLER(a,b)     (a->y < b->y) || (a->y == b->y && a->x < b->x)
#define SWAP(a,b,c)  c = a;\
                     a = b;\
                     a = c
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
  SURFOBJ *OutputObj;
  PTRIVERTEX v1, v2, v3;
  RECT_ENUM RectEnum;
  BOOL EnumMore;
  ULONG i;
  POINTL Translate;
  INTENG_ENTER_LEAVE EnterLeave;
  RECTL FillRect;
  ULONG Color;
  
  BOOL sx[NLINES];
  LONG x[NLINES], dx[NLINES], dy[NLINES], incx[NLINES], ex[NLINES], destx[NLINES];
  LONG c[NLINES][3], dc[NLINES][3], ec[NLINES][3], ic[NLINES][3]; /* colors on lines */
  LONG g, gx, gxi, gc[3], gd[3], ge[3], gi[3]; /* colors in triangle */
  LONG sy, y, bt;
  
  v1 = (pVertex + gTriangle->Vertex1);
  v2 = (pVertex + gTriangle->Vertex2);
  v3 = (pVertex + gTriangle->Vertex3);
  
  /* bubble sort */
  if(SMALLER(v2,v1))
  {
    TRIVERTEX *t;
    SWAP(v1,v2,t);
  }
  if(SMALLER(v3,v2))
  {
    TRIVERTEX *t;
    SWAP(v2,v3,t);
    if(SMALLER(v2,v1))
    {
      SWAP(v1,v2,t);
    }
  }
  
  DbgPrint("Triangle: (%i,%i) (%i,%i) (%i,%i)\n", v1->x, v1->y, v2->x, v2->y, v3->x, v3->y);
  
  if(!IntEngEnter(&EnterLeave, psoDest, &FillRect, FALSE, &Translate, &OutputObj))
  {
    return FALSE;
  }
  
  if(VCMPCLRS(v1, v2, v3))
  {
    CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_RIGHTDOWN, 0);
    do
    {
      EnumMore = CLIPOBJ_bEnum(pco, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
      for (i = 0; i < RectEnum.c && RectEnum.arcl[i].top <= prclExtents->bottom; i++)
      {
        if(NtGdiIntersectRect((PRECT)&FillRect, (PRECT)&RectEnum.arcl[i], (PRECT)prclExtents))
        {
          BOOL InY;
          
          DOINIT(v1, v3, 0);
          DOINIT(v1, v2, 1);
          DOINIT(v2, v3, 2);
          
          y = v1->y;
          sy = v1->y + pptlDitherOrg->y;
          bt = min(v3->y + pptlDitherOrg->y, FillRect.bottom);
          
          while(sy < bt)
          {
            InY = !(sy < FillRect.top || sy >= FillRect.bottom);
            GOLINE(v1, v3, 0);
            DOLINE(v1, v3, 0);
            ENDLINE(v1, v3, 0);
            
            GOLINE(v1, v2, 1);
            DOLINE(v1, v2, 1);
            FILLLINE(0, 1);
            ENDLINE(v1, v2, 1);
            
            GOLINE(v2, v3, 2);
            DOLINE(v2, v3, 2);
            FILLLINE(0, 2);
            ENDLINE(23, v3, 2);
            
            y++;
            sy++;
          }
        }
      }
    } while(EnumMore);
    
    return IntEngLeave(&EnterLeave);
  }
  
  /* fill triangle with one solid color */
  
  Color = XLATEOBJ_iXlate(pxlo, RGB(v1->Red >> 8, v1->Green >> 8, v1->Blue >> 8));
  CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_RIGHTDOWN, 0);
  do
  {
    EnumMore = CLIPOBJ_bEnum(pco, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
    for (i = 0; i < RectEnum.c && RectEnum.arcl[i].top <= prclExtents->bottom; i++)
    {
      if(NtGdiIntersectRect((PRECT)&FillRect, (PRECT)&RectEnum.arcl[i], (PRECT)prclExtents))
      {
        S_INITLINE(v1, v3, 0);
        S_INITLINE(v1, v2, 1);
        S_INITLINE(v2, v3, 2);
        
        y = v1->y;
        sy = v1->y + pptlDitherOrg->y;
        bt = min(v3->y + pptlDitherOrg->y, FillRect.bottom);
        
        while(sy < bt)
        {
          S_GOLINE(v1, v3, 0);
          S_DOLINE(v1, v3, 0);
          S_ENDLINE(v1, v3, 0);
          
          S_GOLINE(v1, v2, 1);
          S_DOLINE(v1, v2, 1);
          S_FILLLINE(0, 1);
          S_ENDLINE(v1, v2, 1);
          
          S_GOLINE(v2, v3, 2);
          S_DOLINE(v2, v3, 2);
          S_FILLLINE(0, 2);
          S_ENDLINE(23, v3, 2);
          
          y++;
          sy++;
        }
      }
    }
  } while(EnumMore);
  
  return IntEngLeave(&EnterLeave);
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
    IN BITMAPOBJ  *pboDest,
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
  SURFOBJ *psoDest = &pboDest->SurfObj;
  
  MouseSafetyOnDrawStart(psoDest, pco->rclBounds.left, pco->rclBounds.top, 
                         pco->rclBounds.right, pco->rclBounds.bottom);
  if((psoDest->iType != STYPE_BITMAP) && (pboDest->flHooks & HOOK_GRADIENTFILL))
  {
    Ret = GDIDEVFUNCS(psoDest).GradientFill(
      psoDest, pco, pxlo, pVertex, nVertex, pMesh, nMesh, 
      prclExtents, pptlDitherOrg, ulMode);
    MouseSafetyOnDrawEnd(psoDest);
    return Ret;
  }
  Ret = EngGradientFill(psoDest, pco, pxlo, pVertex, nVertex, pMesh, nMesh, prclExtents, 
                        pptlDitherOrg, ulMode);
  if(Ret)
  {
    /* Dummy BitBlt to let driver know that something has changed.
       0x00AA0029 is the Rop for D (no-op) */
    if(pboDest->flHooks & HOOK_BITBLT)
    {
      GDIDEVFUNCS(psoDest).BitBlt(
                      psoDest, NULL, NULL, pco, pxlo,
                      prclExtents, pptlDitherOrg, NULL, NULL, NULL, ROP_NOOP);
      MouseSafetyOnDrawEnd(psoDest);
      return TRUE;
    }
  }
  MouseSafetyOnDrawEnd(psoDest);
  return Ret;
}
