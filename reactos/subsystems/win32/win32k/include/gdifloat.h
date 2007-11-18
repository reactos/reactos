#ifndef __WIN32K_FLOAT_H
#define __WIN32K_FLOAT_H


#include <reactos/win32k/ntgdityp.h>
#include <reactos/win32k/ntgdihdl.h>
#include "dc.h"
#include "math.h"
#include <ft2build.h>
#include FT_FREETYPE_H

typedef struct tagFLOAT_POINT
{
   FLOAT x, y;
} FLOAT_POINT;

/* Rounds a floating point number to integer. The world-to-viewport
 * transformation process is done in floating point internally. This function
 * is then used to round these coordinates to integer values.
 */
static __inline INT GDI_ROUND(FLOAT val)
{
   return (int)floor(val + 0.5);
}

/* Performs a world-to-viewport transformation on the specified point (which
 * is in floating point format).
 */
static __inline void INTERNAL_LPTODP_FLOAT(DC *dc, FLOAT_POINT *point)
{
    FLOAT x, y;

    /* Perform the transformation */
    x = point->x;
    y = point->y;
    point->x = x * dc->w.xformWorld2Vport.eM11 +
               y * dc->w.xformWorld2Vport.eM21 +
	       dc->w.xformWorld2Vport.eDx;
    point->y = x * dc->w.xformWorld2Vport.eM12 +
               y * dc->w.xformWorld2Vport.eM22 +
	       dc->w.xformWorld2Vport.eDy;
}

/* Performs a viewport-to-world transformation on the specified point (which
 * is in integer format). Returns TRUE if successful, else FALSE.
 */
#if 0
static __inline BOOL INTERNAL_DPTOLP(DC *dc, LPPOINT point)
{
    FLOAT_POINT floatPoint;

    /* Perform operation with floating point */
    floatPoint.x=(FLOAT)point->x;
    floatPoint.y=(FLOAT)point->y;
    if (!INTERNAL_DPTOLP_FLOAT(dc, &floatPoint))
        return FALSE;

    /* Round to integers */
    point->x = GDI_ROUND(floatPoint.x);
    point->y = GDI_ROUND(floatPoint.y);

    return TRUE;
}

/* Performs a world-to-viewport transformation on the specified point (which
 * is in integer format).
 */
static __inline void INTERNAL_LPTODP(DC *dc, LPPOINT point)
{
    FLOAT_POINT floatPoint;

    /* Perform operation with floating point */
    floatPoint.x=(FLOAT)point->x;
    floatPoint.y=(FLOAT)point->y;
    INTERNAL_LPTODP_FLOAT(dc, &floatPoint);

    /* Round to integers */
    point->x = GDI_ROUND(floatPoint.x);
    point->y = GDI_ROUND(floatPoint.y);
}

#endif

#define MulDiv( x, y, z ) EngMulDiv( x, y, z )

#define XDPTOLP(Dc_Attr,tx) \
    (MulDiv(((tx)-(Dc_Attr)->ptlViewportOrg.x), (Dc_Attr)->szlWindowExt.cx, (Dc_Attr)->szlViewportExt.cx) + (Dc_Attr)->ptlWindowOrg.x)
#define YDPTOLP(Dc_Attr,ty) \
    (MulDiv(((ty)-(Dc_Attr)->ptlViewportOrg.y), (Dc_Attr)->szlWindowExt.cy, (Dc_Attr)->szlViewportExt.cy) + (Dc_Attr)->ptlWindowOrg.y)
#define XLPTODP(Dc_Attr,tx) \
    (MulDiv(((tx)-(Dc_Attr)->ptlWindowOrg.x), (Dc_Attr)->szlViewportExt.cx, (Dc_Attr)->szlWindowExt.cx) + (Dc_Attr)->ptlViewportOrg.x)
#define YLPTODP(Dc_Attr,ty) \
    (MulDiv(((ty)-(Dc_Attr)->ptlWindowOrg.y), (Dc_Attr)->szlViewportExt.cy, (Dc_Attr)->szlWindowExt.cy) + (Dc_Attr)->ptlViewportOrg.y)

  /* Device <-> logical size conversion */

#define XDSTOLS(Dc_Attr,tx) \
    MulDiv((tx), (Dc_Attr)->szlWindowExt.cx, (Dc_Attr)->szlViewportExt.cx)
#define YDSTOLS(DC_Attr,ty) \
    MulDiv((ty), (Dc_Attr)->szlWindowExt.cy, (Dc_Attr)->szlViewportExt.cy)
#define XLSTODS(Dc_Attr,tx) \
    MulDiv((tx), (Dc_Attr)->szlViewportExt.cx, (Dc_Attr)->szlWindowExt.cx)
#define YLSTODS(Dc_Attr,ty) \
    MulDiv((ty), (Dc_Attr)->szlViewportExt.cy, (Dc_Attr)->szlWindowExt.cy)

VOID FASTCALL XForm2MatrixS( MATRIX_S *, PXFORM);
VOID FASTCALL MatrixS2XForm( PXFORM, MATRIX_S *);

#endif

