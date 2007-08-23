#include "precomp.h"


/*
 * @implemented
 */
int STDCALL
SelectClipRgn(
        HDC     hdc,
        HRGN    hrgn
)
{
    return NtGdiExtSelectClipRgn(hdc, hrgn, RGN_COPY);
}


/*
 * @implemented
 */
int
STDCALL
GetClipRgn(
        HDC     hdc,
        HRGN    hrgn
        )
{
    return NtGdiGetRandomRgn(hdc, hrgn, 1);
}


HRGN
WINAPI 
CreatePolygonRgn( const POINT* lppt, int cPoints, int fnPolyFillMode)
{
    /* FIXME  NtGdiPolyPolyDraw */
#if 0
    return NtGdiPolyPolyDraw(fnPolyFillMode,lppt,cPoints,1,6);
#else
    return CreatePolyPolygonRgn(lppt, (const INT*)&cPoints, 1, fnPolyFillMode);
#endif
}


HRGN
WINAPI
CreatePolyPolygonRgn( const POINT* lppt,
                      const INT* lpPolyCounts,
                      int nCount,
                      int fnPolyFillMode)
{
    /* FIXME NtGdiPolyPolyDraw  */
#if 0
  return (HRGN) NtGdiPolyPolyDraw(  (HDC) fnPolyFillMode,
                                 (PPOINT) lppt,
                                 (PULONG) lpPolyCounts,
                                  (ULONG) nCount,
                                          GdiPolyPolyRgn );
#else
    return NtGdiCreatePolyPolygonRgn( (PPOINT)lppt, (PINT)lpPolyCounts, nCount, fnPolyFillMode);
#endif

}

HRGN
WINAPI
CreateEllipticRgnIndirect(
   const RECT *prc
)
{
    /* Notes if prc is NULL it will crash on All Windows NT I checked 2000/XP/VISTA */
    return NtGdiCreateEllipticRgn(prc->left, prc->top, prc->right, prc->bottom);

}

HRGN    
WINAPI 
CreateRectRgn(int x1, int y1, int x2,int y2)
{
    /* FIXME Some part need be done in user mode */
    return NtGdiCreateRectRgn(x1,y1,x2,y2);
}


HRGN
WINAPI
CreateRectRgnIndirect(
    const RECT *prc
)
{
    /* Notes if prc is NULL it will crash on All Windows NT I checked 2000/XP/VISTA */
    return CreateRectRgn(prc->left, prc->top, prc->right, prc->bottom);

}
