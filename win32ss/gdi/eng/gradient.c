/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Gradient Functions
 * FILE:              win32ss/gdi/eng/gradient.c
 * PROGRAMER:         Thomas Weidenmueller
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

/* MACROS *********************************************************************/

const LONG LINC[2] = {-1, 1};

#define VERTEX(n) (pVertex + gt->n)
#define COMPAREVERTEX(a, b) ((a)->x == (b)->x && (a)->y == (b)->y)

/* Check if all three vectors have the same color, either R, G, or B */
#define VCMPCLR(a, b, c, color) (a->color == b->color && a->color == c->color)
/* Check if all three vectors have the same colors for R, G, and B, then
 * NOT the result because we want to check for not using solid color logic */
#define VCMPCLRS(a, b, c) \
  !(VCMPCLR(a, b, c, Red) && VCMPCLR(a, b, c, Green) && VCMPCLR(a, b, c, Blue))

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

BOOL
FASTCALL
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
    SURFOBJ *psoOutput;
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
    RECTL_vOffsetRect(&rcSG, pptlDitherOrg->x, pptlDitherOrg->y);

    if(Horizontal)
    {
        dy = abs(rcGradient.right - rcGradient.left);
    }
    else
    {
        dy = abs(rcGradient.bottom - rcGradient.top);
    }

    if(!IntEngEnter(&EnterLeave, psoDest, &rcSG, FALSE, &Translate, &psoOutput))
    {
        return FALSE;
    }

    if((v1->Red != v2->Red || v1->Green != v2->Green || v1->Blue != v2->Blue) && dy > 1)
    {
        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_RIGHTDOWN, 0);
        do
        {
            RECTL FillRect;
            ULONG Color;

            if (Horizontal)
            {
                EnumMore = CLIPOBJ_bEnum(pco, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
                for (i = 0; i < RectEnum.c && RectEnum.arcl[i].top <= rcSG.bottom; i++)
                {
                    if (RECTL_bIntersectRect(&FillRect, &RectEnum.arcl[i], &rcSG))
                    {
                        HVINITCOL(Red, 0);
                        HVINITCOL(Green, 1);
                        HVINITCOL(Blue, 2);

                        for (y = rcSG.left; y < FillRect.right; y++)
                        {
                            if (y >= FillRect.left)
                            {
                                Color = XLATEOBJ_iXlate(pxlo, RGB(c[0], c[1], c[2]));
                                DibFunctionsForBitmapFormat[psoOutput->iBitmapFormat].DIB_VLine(
                                    psoOutput, y + Translate.x, FillRect.top + Translate.y, FillRect.bottom + Translate.y, Color);
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
                if (RECTL_bIntersectRect(&FillRect, &RectEnum.arcl[i], &rcSG))
                {
                    HVINITCOL(Red, 0);
                    HVINITCOL(Green, 1);
                    HVINITCOL(Blue, 2);

                    for (y = rcSG.top; y < FillRect.bottom; y++)
                    {
                        if (y >= FillRect.top)
                        {
                            Color = XLATEOBJ_iXlate(pxlo, RGB(c[0], c[1], c[2]));
                            DibFunctionsForBitmapFormat[psoOutput->iBitmapFormat].DIB_HLine(psoOutput,
                                                                                            FillRect.left + Translate.x,
                                                                                            FillRect.right + Translate.x,
                                                                                            y + Translate.y,
                                                                                            Color);
                        }
                        HVSTEPCOL(0);
                        HVSTEPCOL(1);
                        HVSTEPCOL(2);
                    }
                }
            }

        }
        while (EnumMore);

        return IntEngLeave(&EnterLeave);
    }

    /* rectangle has only one color, no calculation required */
    CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_RIGHTDOWN, 0);
    do
    {
        RECTL FillRect;
        ULONG Color = XLATEOBJ_iXlate(pxlo, RGB(v1->Red >> 8, v1->Green >> 8, v1->Blue >> 8));

        EnumMore = CLIPOBJ_bEnum(pco, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
        for (i = 0; i < RectEnum.c && RectEnum.arcl[i].top <= rcSG.bottom; i++)
        {
            if (RECTL_bIntersectRect(&FillRect, &RectEnum.arcl[i], &rcSG))
            {
                for (; FillRect.top < FillRect.bottom; FillRect.top++)
                {
                    DibFunctionsForBitmapFormat[psoOutput->iBitmapFormat].DIB_HLine(psoOutput,
                                                                                    FillRect.left + Translate.x,
                                                                                    FillRect.right + Translate.x,
                                                                                    FillRect.top + Translate.y,
                                                                                    Color);
                }
            }
        }
    }
    while (EnumMore);

    return IntEngLeave(&EnterLeave);
}

/* Fill triangle with solid color */
#define S_FILLLINE(linefrom,lineto) \
  if(sx[lineto] < sx[linefrom]) \
    DibFunctionsForBitmapFormat[psoOutput->iBitmapFormat].DIB_HLine(psoOutput, max(sx[lineto], FillRect.left), min(sx[linefrom], FillRect.right), sy, Color); \
  else \
    DibFunctionsForBitmapFormat[psoOutput->iBitmapFormat].DIB_HLine(psoOutput, max(sx[linefrom], FillRect.left), min(sx[lineto], FillRect.right), sy, Color);

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
  if(dy[line] != 0) \
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
  if (gx != 0) \
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
  g_end = sx[lineto] + gxi; \
  for(g = sx[linefrom]; g != g_end; g += gxi) \
  { \
    if(InY && g >= FillRect.left && g < FillRect.right) \
    { \
      Color = XLATEOBJ_iXlate(pxlo, RGB(gc[0], gc[1], gc[2])); \
      DibFunctionsForBitmapFormat[psoOutput->iBitmapFormat].DIB_PutPixel(psoOutput, g, sy, Color); \
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
  sx[line] = a->x + pptlDitherOrg->x - 1; \
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
                     b = c

#define NLINES 3

BOOL
FASTCALL
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
    SURFOBJ *psoOutput;
    PTRIVERTEX v1, v2, v3;
    RECT_ENUM RectEnum;
    BOOL EnumMore;
    ULONG i;
    POINTL Translate;
    INTENG_ENTER_LEAVE EnterLeave;
    RECTL FillRect = { 0, 0, 0, 0 };
    ULONG Color;

    BOOL sx[NLINES];
    LONG x[NLINES], dx[NLINES], dy[NLINES], incx[NLINES], ex[NLINES], destx[NLINES];
    LONG c[NLINES][3], dc[NLINES][3], ec[NLINES][3], ic[NLINES][3]; /* colors on lines */
    LONG g, gx, gxi, gc[3], gd[3], ge[3], gi[3]; /* colors in triangle */
    LONG sy, y, bt, g_end;

    v1 = (pVertex + gTriangle->Vertex1);
    v2 = (pVertex + gTriangle->Vertex2);
    v3 = (pVertex + gTriangle->Vertex3);

    /* bubble sort */
    if (SMALLER(v2, v1))
    {
        TRIVERTEX *t;
        SWAP(v1, v2, t);
    }

    if (SMALLER(v3, v2))
    {
        TRIVERTEX *t;
        SWAP(v2, v3, t);
        if (SMALLER(v2, v1))
        {
            SWAP(v1, v2, t);
        }
    }

    DPRINT("Triangle: (%i,%i) (%i,%i) (%i,%i)\n", v1->x, v1->y, v2->x, v2->y, v3->x, v3->y);

    if (!IntEngEnter(&EnterLeave, psoDest, &FillRect, FALSE, &Translate, &psoOutput))
    {
        return FALSE;
    }

    if (VCMPCLRS(v1, v2, v3))
    {
      CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_RIGHTDOWN, 0);
      do
      {
        EnumMore = CLIPOBJ_bEnum(pco, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
        for (i = 0; i < RectEnum.c && RectEnum.arcl[i].top <= prclExtents->bottom; i++)
        {
          if (RECTL_bIntersectRect(&FillRect, &RectEnum.arcl[i], prclExtents))
          {
            BOOL InY;

            DOINIT(v1, v3, 0);
            DOINIT(v1, v2, 1);
            DOINIT(v2, v3, 2);

            y = v1->y;
            sy = v1->y + pptlDitherOrg->y;
            bt = min(v3->y + pptlDitherOrg->y, FillRect.bottom);

            while (sy < bt)
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
              FILLLINE(0, 2);
              DOLINE(v2, v3, 2);
              FILLLINE(0, 2);
              ENDLINE(23, v3, 2);

              y++;
              sy++;
            }
          }
        }
      } while (EnumMore);

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
        if (RECTL_bIntersectRect(&FillRect, &RectEnum.arcl[i], prclExtents))
        {
          S_INITLINE(v1, v3, 0);
          S_INITLINE(v1, v2, 1);
          S_INITLINE(v2, v3, 2);

          y = v1->y;
          sy = v1->y + pptlDitherOrg->y;
          bt = min(v3->y + pptlDitherOrg->y, FillRect.bottom);

          while (sy < bt)
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
    } while (EnumMore);

    return IntEngLeave(&EnterLeave);
}


static
BOOL
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


BOOL
APIENTRY
EngGradientFill(
    _Inout_ SURFOBJ *psoDest,
    _In_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_ TRIVERTEX *pVertex,
    _In_ ULONG nVertex,
    _In_ PVOID pMesh,
    _In_ ULONG nMesh,
    _In_ RECTL *prclExtents,
    _In_ POINTL *pptlDitherOrg,
    _In_ ULONG ulMode)
{
    ULONG i;
    BOOL ret = FALSE;

    /* Check for NULL clip object */
    if (pco == NULL)
    {
        /* Use the trivial one instead */
        pco = (CLIPOBJ *)&gxcoTrivial;//.coClient;
    }

    switch(ulMode)
    {
        case GRADIENT_FILL_RECT_H:
        case GRADIENT_FILL_RECT_V:
        {
            PGRADIENT_RECT gr = (PGRADIENT_RECT)pMesh;
            for (i = 0; i < nMesh; i++, gr++)
            {
                if (!IntEngGradientFillRect(psoDest,
                                            pco,
                                            pxlo,
                                            pVertex,
                                            nVertex,
                                            gr,
                                            prclExtents,
                                            pptlDitherOrg,
                                            (ulMode == GRADIENT_FILL_RECT_H)))
                {
                    break;
                }
            }
            ret = TRUE;
            break;
        }
        case GRADIENT_FILL_TRIANGLE:
        {
            PGRADIENT_TRIANGLE gt = (PGRADIENT_TRIANGLE)pMesh;
            for (i = 0; i < nMesh; i++, gt++)
            {
                if (IntEngIsNULLTriangle(pVertex, gt))
                {
                    /* skip empty triangles */
                    continue;
                }
                if (!IntEngGradientFillTriangle(psoDest,
                                                pco,
                                                pxlo,
                                                pVertex,
                                                nVertex,
                                                gt,
                                                prclExtents,
                                                pptlDitherOrg))
                {
                    break;
                }
            }
            ret = TRUE;
            break;
        }
    }

    return ret;
}

BOOL
APIENTRY
IntEngGradientFill(
    IN SURFOBJ *psoDest,
    IN CLIPOBJ *pco,
    IN XLATEOBJ *pxlo,
    IN TRIVERTEX *pVertex,
    IN ULONG nVertex,
    IN PVOID pMesh,
    IN ULONG nMesh,
    IN RECTL *prclExtents,
    IN POINTL *pptlDitherOrg,
    IN ULONG ulMode)
{
    BOOL Ret;
    SURFACE *psurf;
    ASSERT(psoDest);

    psurf = CONTAINING_RECORD(psoDest, SURFACE, SurfObj);
    ASSERT(psurf);

    if (psurf->flags & HOOK_GRADIENTFILL)
    {
        Ret = GDIDEVFUNCS(psoDest).GradientFill(psoDest,
                                                pco,
                                                pxlo,
                                                pVertex,
                                                nVertex,
                                                pMesh,
                                                nMesh,
                                                prclExtents,
                                                pptlDitherOrg,
                                                ulMode);
    }
    else
    {
        Ret = EngGradientFill(psoDest,
                              pco,
                              pxlo,
                              pVertex,
                              nVertex,
                              pMesh,
                              nMesh,
                              prclExtents,
                              pptlDitherOrg,
                              ulMode);
    }

    return Ret;
}
