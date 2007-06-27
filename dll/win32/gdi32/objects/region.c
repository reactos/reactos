#include "precomp.h"


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
	HRGN rgn = NtGdiGetClipRgn(hdc);
	if(rgn)
	{
		if(NtGdiCombineRgn(hrgn, rgn, 0, RGN_COPY) != ERROR) return 1;
		else
			return -1;
	}
	else	return 0;
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


