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

           pTeb->GdiTebBatch.Offset += sizeof(GDIBSOBJECT);
           pTeb->GdiBatchCount++;
           if (pTeb->GdiBatchCount >= GDI_BatchLimit) NtGdiFlush();
           return TRUE;
        }
     }
  }
#endif
  return NtGdiDeleteObjectApp((HGDIOBJ) hRgn);
}

/*
 * @implemented
 */
HRGN
STDCALL
ExtCreateRegion(
	CONST XFORM *	lpXform,
	DWORD		nCount,
	CONST RGNDATA *	lpRgnData
	)
{
   if (lpRgnData)
   {
     if ((!lpXform) && (lpRgnData->rdh.nCount == 1))
     {
         PRECT pRect = (PRECT)&lpRgnData->Buffer[0];
         return CreateRectRgn(pRect->left, pRect->top, pRect->right, pRect->bottom);
     }
     return NtGdiExtCreateRegion((LPXFORM) lpXform, nCount,(LPRGNDATA) lpRgnData);
   }
   SetLastError(ERROR_INVALID_PARAMETER);
   return NULL;
}


/*
 * @implemented
 */
BOOL
STDCALL
SetRectRgn(HRGN hrgn,
           int nLeftRect,
           int nTopRect,
           int nRightRect,
           int nBottomRect)
{
#if 0
  PREGION_ATTR Rgn_Attr;

  if (!(GdiGetHandleUserData((HGDIOBJ) hrgn, (PVOID) &Rgn_Attr)) ||
       (GDI_HANDLE_GET_TYPE(hrgn) != GDI_OBJECT_TYPE_REGION)) 
#endif
     return NtGdiSetRectRgn(hrgn, nLeftRect, nTopRect, nRightRect, nBottomRect);
#if 0
  if ((nLeftRect == nRightRect) || (nTopRect == nBottomRect))
  {
     Rgn_Attr->flFlags |= DIRTY_RGNATTR;
     Rgn_Attr->dwType = RGNATTR_INIT;
     Rgn_Attr->rcBound.left = Rgn_Attr->rcBound.top =
     Rgn_Attr->rcBound.right = Rgn_Attr->rcBound.bottom = 0;
     return TRUE;
  }

  Rgn_Attr->rcBound.left   = nLeftRect;
  Rgn_Attr->rcBound.top    = nTopRect;
  Rgn_Attr->rcBound.right  = nRightRect;
  Rgn_Attr->rcBound.bottom = nBottomRect;

  if(nLeftRect > nRightRect)
  {
     Rgn_Attr->rcBound.left   = nRightRect;
     Rgn_Attr->rcBound.right  = nLeftRect;
  }
  if(nTopRect > nBottomRect)
  {
     Rgn_Attr->rcBound.top    = nBottomRect;
     Rgn_Attr->rcBound.bottom = nTopRect;
  }

  Rgn_Attr->flFlags |= DIRTY_RGNATTR;
  Rgn_Attr->dwType = RGNATTR_SET;
  return TRUE;
#endif
}

