#ifndef __WIN32K_FLOAT_H
#define __WIN32K_FLOAT_H

#include <reactos/win32k/win32kdc.h>
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

#define XDPTOLP(dc,tx) \
    (MulDiv(((tx)-(dc)->Dc_Attr.ptlViewportOrg.x), (dc)->Dc_Attr.szlWindowExt.cx, (dc)->Dc_Attr.szlViewportExt.cx) + (dc)->Dc_Attr.ptlWindowOrg.x)
#define YDPTOLP(dc,ty) \
    (MulDiv(((ty)-(dc)->Dc_Attr.ptlViewportOrg.y), (dc)->Dc_Attr.szlWindowExt.cy, (dc)->Dc_Attr.szlViewportExt.cy) + (dc)->Dc_Attr.ptlWindowOrg.y)
#define XLPTODP(dc,tx) \
    (MulDiv(((tx)-(dc)->Dc_Attr.ptlWindowOrg.x), (dc)->Dc_Attr.szlViewportExt.cx, (dc)->Dc_Attr.szlWindowExt.cx) + (dc)->Dc_Attr.ptlViewportOrg.x)
#define YLPTODP(dc,ty) \
    (MulDiv(((ty)-(dc)->Dc_Attr.ptlWindowOrg.y), (dc)->Dc_Attr.szlViewportExt.cy, (dc)->Dc_Attr.szlWindowExt.cy) + (dc)->Dc_Attr.ptlViewportOrg.y)

  /* Device <-> logical size conversion */

#define XDSTOLS(dc,tx) \
    MulDiv((tx), (dc)->Dc_Attr.szlWindowExt.cx, (dc)->Dc_Attr.szlViewportExt.cx)
#define YDSTOLS(dc,ty) \
    MulDiv((ty), (dc)->Dc_Attr.szlWindowExt.cy, (dc)->Dc_Attr.szlViewportExt.cy)
#define XLSTODS(dc,tx) \
    MulDiv((tx), (dc)->Dc_Attr.szlViewportExt.cx, (dc)->Dc_Attr.szlWindowExt.cx)
#define YLSTODS(dc,ty) \
    MulDiv((ty), (dc)->Dc_Attr.szlViewportExt.cy, (dc)->Dc_Attr.szlWindowExt.cy)

#endif

VOID FASTCALL XForm2MatrixS( MATRIX_S *, PXFORM);
VOID FASTCALL MatrixS2XForm( PXFORM, MATRIX_S *);

