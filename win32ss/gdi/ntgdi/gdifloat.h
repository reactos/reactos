#pragma once

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:28110) // disable "Drivers must protect floating point hardware state" warning
#endif

static __inline INT GDI_ROUND(FLOAT val)
{
    return (int)floor(val + 0.5);
}


/*
 * Performs a world-to-viewport transformation on the specified point,
 * which is in integer format.
 */
static
inline
VOID
INTERNAL_LPTODP(DC *dc, LPPOINT point, UINT Count)
{
    MATRIX* WorldToDevice = &dc->pdcattr->mxWorldToDevice;

    while (Count--)
    {
        FLOATOBJ x, y;
        FLOATOBJ tmp;

        /* x = x * mxWorldToDevice.efM11 + y * mxWorldToDevice.efM21 + mxWorldToDevice.efDx; */
        FLOATOBJ_SetLong(&x, point[Count].x);
        FLOATOBJ_Mul(&x, &WorldToDevice->efM11);
        tmp = WorldToDevice->efM21;
        FLOATOBJ_MulLong(&tmp, point[Count].y);
        FLOATOBJ_Add(&x, &tmp);
        FLOATOBJ_Add(&x, &WorldToDevice->efDx);

        /* y = x * mxWorldToDevice.efM12 + y * mxWorldToDevice.efM22 + mxWorldToDevice.efDy; */
        FLOATOBJ_SetLong(&y, point[Count].y);
        FLOATOBJ_Mul(&y, &WorldToDevice->efM22);
        tmp = WorldToDevice->efM12;
        FLOATOBJ_MulLong(&tmp, point[Count].x);
        FLOATOBJ_Add(&y, &tmp);
        FLOATOBJ_Add(&y, &WorldToDevice->efDy);

        point[Count].x = FLOATOBJ_GetLong(&x);
        point[Count].y = FLOATOBJ_GetLong(&y);
    }
}

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
