#pragma once

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:28110) // disable "Drivers must protect floating point hardware state" warning
#endif

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


/* FIXME: Do not use the fpu in kernel on x86, use FLOATOBJ_Xxx api instead
 * Performs a world-to-viewport transformation on the specified point (which
 * is in floating point format).
 */
static __inline void INTERNAL_LPTODP_FLOAT(DC *dc, FLOAT_POINT *point)
{
    FLOAT x, y;
    XFORM xformWorld2Vport;

    MatrixS2XForm(&xformWorld2Vport, &dc->pdcattr->mxWorldToDevice);

    /* Perform the transformation */
    x = point->x;
    y = point->y;
    point->x = x * xformWorld2Vport.eM11 +
               y * xformWorld2Vport.eM21 +
	          xformWorld2Vport.eDx;

    point->y = x * xformWorld2Vport.eM12 +
               y * xformWorld2Vport.eM22 +
	          xformWorld2Vport.eDy;
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

#define XDPTOLP(pdcattr,tx) \
    (MulDiv(((tx)-(pdcattr)->ptlViewportOrg.x), (pdcattr)->szlWindowExt.cx, (pdcattr)->szlViewportExt.cx) + (pdcattr)->ptlWindowOrg.x)
#define YDPTOLP(pdcattr,ty) \
    (MulDiv(((ty)-(pdcattr)->ptlViewportOrg.y), (pdcattr)->szlWindowExt.cy, (pdcattr)->szlViewportExt.cy) + (pdcattr)->ptlWindowOrg.y)
#define XLPTODP(pdcattr,tx) \
    (MulDiv(((tx)-(pdcattr)->ptlWindowOrg.x), (pdcattr)->szlViewportExt.cx, (pdcattr)->szlWindowExt.cx) + (pdcattr)->ptlViewportOrg.x)
#define YLPTODP(pdcattr,ty) \
    (MulDiv(((ty)-(pdcattr)->ptlWindowOrg.y), (pdcattr)->szlViewportExt.cy, (pdcattr)->szlWindowExt.cy) + (pdcattr)->ptlViewportOrg.y)

  /* Device <-> logical size conversion */

#define XDSTOLS(pdcattr,tx) \
    MulDiv((tx), (pdcattr)->szlWindowExt.cx, (pdcattr)->szlViewportExt.cx)
#define YDSTOLS(DC_Attr,ty) \
    MulDiv((ty), (pdcattr)->szlWindowExt.cy, (pdcattr)->szlViewportExt.cy)
#define XLSTODS(pdcattr,tx) \
    MulDiv((tx), (pdcattr)->szlViewportExt.cx, (pdcattr)->szlWindowExt.cx)
#define YLSTODS(pdcattr,ty) \
    MulDiv((ty), (pdcattr)->szlViewportExt.cy, (pdcattr)->szlWindowExt.cy)

#ifdef _MSC_VER
#pragma warning(pop)
#endif
