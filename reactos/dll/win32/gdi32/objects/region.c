#include "precomp.h"

static
VOID
FASTCALL
SortRects(PRECT pRect, INT nCount)
{
  INT i = 0, a = 0, b = 0, c, s;
  RECT sRect;

  if (nCount > 0)
  {
     i = 1; // set index point
     c = nCount; // set inverse count
     do
     {
        s = i; // set sort count
        if ( i < nCount )
        {
           a = i - 1;
           b = i;
           do
           {
              if(pRect[b].top != pRect[i].bottom) break;
              if(pRect[b].left < pRect[a].left)
              {
                 sRect = pRect[a];
                 pRect[a] = pRect[b];
                 pRect[b] = sRect;
              }
              ++s;
              ++b;
           } while ( s < nCount );
        }
        ++i;
     } while ( c-- != 1 );
  }
}

/*
 * I thought it was okay to have this in DeleteObject but~ Speed. (jt)
 */
BOOL
FASTCALL
DeleteRegion( HRGN hRgn )
{
#if 0
  PRGN_ATTR Rgn_Attr;

  if ((GdiGetHandleUserData((HGDIOBJ) hRgn, GDI_OBJECT_TYPE_REGION, (PVOID) &Rgn_Attr)) &&
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

INT
FASTCALL
MirrorRgnByWidth(HRGN hRgn, INT Width, HRGN *phRgn)
{
  INT cRgnDSize, Ret = 0;
  PRGNDATA pRgnData;
  
  cRgnDSize = NtGdiGetRegionData(hRgn, 0, NULL);

  if (cRgnDSize)
  {
     pRgnData = LocalAlloc(LMEM_FIXED, cRgnDSize * sizeof(LONG));
     if (pRgnData)
     {
        if ( GetRegionData(hRgn, cRgnDSize, pRgnData) )
        {
           HRGN hRgnex;
           UINT i;
           INT SaveL = pRgnData->rdh.rcBound.left;
           pRgnData->rdh.rcBound.left = Width - pRgnData->rdh.rcBound.right;
           pRgnData->rdh.rcBound.right = Width - SaveL;
           if (pRgnData->rdh.nCount > 0)
           {
              PRECT pRect = (PRECT)&pRgnData->Buffer;
              for (i = 0; i < pRgnData->rdh.nCount; i++)
              {
                  SaveL = pRect[i].left;
                  pRect[i].left = Width - pRect[i].right;
                  pRect[i].right = Width - SaveL;
              }
           }
           SortRects((PRECT)&pRgnData->Buffer, pRgnData->rdh.nCount);
           hRgnex = ExtCreateRegion(NULL, cRgnDSize , pRgnData);
           if (hRgnex)
           {
              if (phRgn) phRgn = (HRGN *)hRgnex;
              else
              {
                 CombineRgn(hRgn, hRgnex, 0, RGN_COPY); 
                 DeleteObject(hRgnex);
              }
              Ret = 1;
           }
        }
        LocalFree(pRgnData);
     }
  }
  return Ret;
}

INT
WINAPI
MirrorRgnDC(HDC hdc, HRGN hRgn, HRGN *phRgn)
{
  if (!GdiIsHandleValid((HGDIOBJ) hdc) ||
      (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)) return 0;

  return MirrorRgnByWidth(hRgn, NtGdiGetDeviceWidth(hdc), phRgn);
}

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
INT
WINAPI
CombineRgn(HRGN  hDest,
           HRGN  hSrc1,
           HRGN  hSrc2,
           INT  CombineMode)
{
    /* FIXME some part should be done in user mode */
    return NtGdiCombineRgn(hDest, hSrc1, hSrc2, CombineMode);
}

/*
 * @implemented
 */
HRGN
WINAPI
CreatePolygonRgn( const POINT * lppt, int cPoints, int fnPolyFillMode)
{
    return (HRGN) NtGdiPolyPolyDraw( (HDC) fnPolyFillMode, (PPOINT) lppt, (PULONG) &cPoints, 1, GdiPolyPolyRgn);
}

/*
 * @implemented
 */
HRGN
WINAPI
CreatePolyPolygonRgn( const POINT* lppt,
                      const INT* lpPolyCounts,
                      int nCount,
                      int fnPolyFillMode)
{
    return (HRGN) NtGdiPolyPolyDraw(  (HDC) fnPolyFillMode, (PPOINT) lppt, (PULONG) lpPolyCounts, (ULONG) nCount, GdiPolyPolyRgn );
}

/*
 * @implemented
 */
HRGN
WINAPI
CreateEllipticRgnIndirect(
   const RECT *prc
)
{
    /* Notes if prc is NULL it will crash on All Windows NT I checked 2000/XP/VISTA */
    return NtGdiCreateEllipticRgn(prc->left, prc->top, prc->right, prc->bottom);

}

/*
 * @implemented
 */
HRGN
WINAPI
CreateRectRgn(int x1, int y1, int x2, int y2)
{
    /* FIXME Some part need be done in user mode */
    return NtGdiCreateRectRgn(x1,y1,x2,y2);
}

/*
 * @implemented
 */
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
 * @implemented
 */
INT
WINAPI
ExcludeClipRect(IN HDC hdc, IN INT xLeft, IN INT yTop, IN INT xRight, IN INT yBottom)
{
    /* FIXME some part need be done on user mode size */
    return NtGdiExcludeClipRect(hdc, xLeft, yTop, xRight, yBottom);
}

/*
 * @implemented
 */
HRGN
WINAPI
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
INT
WINAPI
ExtSelectClipRgn( IN HDC hdc, IN HRGN hrgn, IN INT iMode)
{
    /* FIXME some part need be done on user mode size */
    return NtGdiExtSelectClipRgn(hdc,hrgn, iMode);
}

/*
 * @implemented
 */
int
WINAPI
GetClipRgn(
        HDC     hdc,
        HRGN    hrgn
        )
{
  INT Ret = NtGdiGetRandomRgn(hdc, hrgn, CLIPRGN);
//  if (Ret)
//  {
//     if(GetLayout(hdc) & LAYOUT_RTL) MirrorRgnDC(hdc,(HRGN)Ret, NULL);
//  }
  return Ret;
}

/*
 * @implemented
 */
int
WINAPI
GetMetaRgn(HDC hdc,
           HRGN hrgn)
{
    return NtGdiGetRandomRgn(hdc, hrgn, METARGN);
}

/*
 * @implemented
 *
 */
DWORD
WINAPI
GetRegionData(HRGN hrgn,
              DWORD nCount,
              LPRGNDATA lpRgnData)
{
    if (!lpRgnData)
    {
        nCount = 0;
    }

    return NtGdiGetRegionData(hrgn,nCount,lpRgnData);
}

/*
 * @implemented
 *
 */
INT
WINAPI
GetRgnBox(HRGN hrgn,
          LPRECT prcOut)
{
  PRGN_ATTR Rgn_Attr;

  if (!GdiGetHandleUserData((HGDIOBJ) hrgn, GDI_OBJECT_TYPE_REGION, (PVOID) &Rgn_Attr))
     return NtGdiGetRgnBox(hrgn, prcOut);

  if (Rgn_Attr->Flags == NULLREGION)
  {
     prcOut->left   = 0;
     prcOut->top    = 0;
     prcOut->right  = 0;
     prcOut->bottom = 0;
  }
  else
  {
     if (Rgn_Attr->Flags != SIMPLEREGION)
        return NtGdiGetRgnBox(hrgn, prcOut);
     /* WARNING! prcOut is never checked newbies! */
     RtlCopyMemory( prcOut, &Rgn_Attr->Rect, sizeof(RECT));
  }
  return Rgn_Attr->Flags;
}

/*
 * @implemented
 */
INT
WINAPI
IntersectClipRect(HDC hdc,
                  int nLeftRect,
                  int nTopRect,
                  int nRightRect,
                  int nBottomRect)
{
#if 0
// Handle something other than a normal dc object.
  if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
  {
    if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_IntersectClipRect( hdc, nLeftRect, nTopRect, nRightRect, nBottomRect);
    else
    {
      PLDC pLDC = GdiGetLDC(hdc);
      if ( pLDC )
      {
         if (pLDC->iType != LDC_EMFLDC || EMFDRV_IntersectClipRect( hdc, nLeftRect, nTopRect, nRightRect, nBottomRect))
             return NtGdiIntersectClipRect(hdc, nLeftRect, nTopRect, nRightRect, nBottomRect);
      }
      else
        SetLastError(ERROR_INVALID_HANDLE);
      return 0;
    }
  }
#endif
    return NtGdiIntersectClipRect(hdc, nLeftRect, nTopRect, nRightRect, nBottomRect);
}

/*
 * @implemented
 */
BOOL
WINAPI
MirrorRgn(HWND hwnd, HRGN hrgn)
{
  RECT Rect;
  GetWindowRect(hwnd, &Rect);
  return MirrorRgnByWidth(hrgn, Rect.right - Rect.left, NULL);
}

/*
 * @implemented
 */
INT
WINAPI
OffsetClipRgn(HDC hdc,
              int nXOffset,
              int nYOffset)
{
#if 0
// Handle something other than a normal dc object.
  if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
  {
    if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_OffsetClipRgn( hdc, nXOffset, nYOffset );
    else
    {
      PLDC pLDC = GdiGetLDC(hdc);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return 0;
      }
      if (pLDC->iType == LDC_EMFLDC && !EMFDRV_OffsetClipRgn( hdc, nXOffset, nYOffset ))
         return 0;
      return NtGdiOffsetClipRgn( hdc,  nXOffset,  nYOffset);
    }
  }
#endif
  return NtGdiOffsetClipRgn( hdc,  nXOffset,  nYOffset);
}

/*
 * @implemented
 *
 */
INT
WINAPI
OffsetRgn( HRGN hrgn,
          int nXOffset,
          int nYOffset)
{
    /* FIXME some part are done in user mode */
    return NtGdiOffsetRgn(hrgn,nXOffset,nYOffset);
}

/*
 * @unimplemented
 */
BOOL
WINAPI
PtInRegion(IN HRGN hrgn,
           int x,
           int y)
{
    /* FIXME some stuff at user mode need be fixed */
    return NtGdiPtInRegion(hrgn,x,y);
}

/*
 * @unimplemented
 */
BOOL
WINAPI
RectInRegion(HRGN hrgn,
             LPCRECT prcl)
{
    /* FIXME some stuff at user mode need be fixed */
    return NtGdiRectInRegion(hrgn, (LPRECT) prcl);
}

/*
 * @implemented
 */
int WINAPI
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
BOOL
WINAPI
SetRectRgn(HRGN hrgn,
           int nLeftRect,
           int nTopRect,
           int nRightRect,
           int nBottomRect)
{
  PRGN_ATTR Rgn_Attr;

  if (!GdiGetHandleUserData((HGDIOBJ) hrgn, GDI_OBJECT_TYPE_REGION, (PVOID) &Rgn_Attr)) 
     return NtGdiSetRectRgn(hrgn, nLeftRect, nTopRect, nRightRect, nBottomRect);

  if ((nLeftRect == nRightRect) || (nTopRect == nBottomRect))
  {
     Rgn_Attr->AttrFlags |= ATTR_RGN_DIRTY;
     Rgn_Attr->Flags = NULLREGION;
     Rgn_Attr->Rect.left = Rgn_Attr->Rect.top =
     Rgn_Attr->Rect.right = Rgn_Attr->Rect.bottom = 0;
     return TRUE;
  }

  Rgn_Attr->Rect.left   = nLeftRect;
  Rgn_Attr->Rect.top    = nTopRect;
  Rgn_Attr->Rect.right  = nRightRect;
  Rgn_Attr->Rect.bottom = nBottomRect;

  if(nLeftRect > nRightRect)
  {
     Rgn_Attr->Rect.left   = nRightRect;
     Rgn_Attr->Rect.right  = nLeftRect;
  }
  if(nTopRect > nBottomRect)
  {
     Rgn_Attr->Rect.top    = nBottomRect;
     Rgn_Attr->Rect.bottom = nTopRect;
  }

  Rgn_Attr->AttrFlags |= ATTR_RGN_DIRTY ;
  Rgn_Attr->Flags = SIMPLEREGION;
  return TRUE;
}

/*
 * @implemented
 */
int
WINAPI
SetMetaRgn( HDC hDC )
{
 if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_DC)
    return NtGdiSetMetaRgn(hDC);
#if 0
 PLDC pLDC = GdiGetLDC(hDC);
 if ( pLDC && GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_METADC )
 {
    if (pLDC->iType == LDC_EMFLDC || EMFDRV_SetMetaRgn(hDC))
    {
       return NtGdiSetMetaRgn(hDC);
    }
    else
       SetLastError(ERROR_INVALID_HANDLE);
 }
#endif
 return 0;
}


