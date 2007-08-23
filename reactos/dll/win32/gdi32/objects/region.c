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
CreatePolygonRgn( const POINT* Point, int Count, int Mode)
{
  return CreatePolyPolygonRgn(Point, (const INT*)&Count, 1, Mode);
}


HRGN
WINAPI
CreatePolyPolygonRgn( const POINT* Point,
                      const INT* Count,
                      int inPolygons,
                      int Mode)
{
  return (HRGN) NtGdiPolyPolyDraw(  (HDC) Mode,
                                 (PPOINT) Point,
                                 (PULONG) Count,
                                  (ULONG) inPolygons,
                                          GdiPolyPolyRgn );
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
CreateRectRgnIndirect(
    const RECT *prc
)
{
    if (prc)
    {
        return NtGdiCreateRectRgn(prc->left,
                                  prc->top,
                                  prc->right,
                                  prc->bottom);
    }
    return NULL;
}
