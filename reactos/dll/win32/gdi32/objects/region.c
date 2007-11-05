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
    return ExtSelectClipRgn(hdc, hrgn, RGN_COPY);
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
CreatePolygonRgn( const POINT * lppt, int cPoints, int fnPolyFillMode)
{
    return (HRGN) NtGdiPolyPolyDraw( (HDC) fnPolyFillMode, (PPOINT) lppt, (PULONG) &cPoints, 1, GdiPolyPolyRgn);
}


HRGN
WINAPI
CreatePolyPolygonRgn( const POINT* lppt,
                      const INT* lpPolyCounts,
                      int nCount,
                      int fnPolyFillMode)
{
    return (HRGN) NtGdiPolyPolyDraw(  (HDC) fnPolyFillMode, (PPOINT) lppt, (PULONG) lpPolyCounts, (ULONG) nCount, GdiPolyPolyRgn );
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

/*
 * I thought it was okay to have this in DeleteObject but~ Speed. (jt)
 */
BOOL
FASTCALL
DeleteRegion( HRGN hRgn )
{
#if 0
  PREGION_ATTR Rgn_Attr;

  if ((GdiGetHandleUserData((HGDIOBJ) hRgn, (PVOID) &Rgn_Attr)) &&
      ( Rgn_Attr != NULL ))
  {
     PTEB pTeb = NtCurrentTeb();
     if (pTeb->Win32ThreadInfo != NULL)
     {
        if ((pTeb->GdiTebBatch.Offset + sizeof(GDIBSOBJECT)) <= GDIBATCHBUFSIZE)
        {
           PGDIBSOBJECT pgO = (PGDIBSOBJECT)(&pTeb->GdiTebBatch.Buffer[0] +
                                                      pTeb->GdiTebBatch.Offset);
           pgO->gbHdr.Cmd = GdiBCDelRgn;
           pgO->gbHdr.Size = sizeof(GDIBSOBJECT);

           pgO->hgdiobj = (HGDIOBJ)hRgn;

           pTeb->GdiTebBatch.Offset += sizeof(GDIBSSETBRHORG);
           pTeb->GdiBatchCount++;
           if (pTeb->GdiBatchCount >= GDI_BatchLimit) NtGdiFlush();
           return TRUE;
        }
     }
  }
#endif
  return NtGdiDeleteObjectApp((HGDIOBJ) hRgn);
}

