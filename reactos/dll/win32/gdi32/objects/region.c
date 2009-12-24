#include "precomp.h"

#define INRECT(r, x, y) \
      ( ( ((r).right >  x)) && \
      ( ((r).left <= x)) && \
      ( ((r).bottom >  y)) && \
      ( ((r).top <= y)) )

static
INT
FASTCALL
ComplexityFromRects( PRECT prc1, PRECT prc2)
{
  if ( prc2->left >= prc1->left )
  {
     if ( ( prc1->right >= prc2->right) &&
          ( prc1->top <= prc2->top ) &&
          ( prc1->bottom <= prc2->bottom ) )      
        return SIMPLEREGION;

     if ( prc2->left > prc1->left )
     {
        if ( ( prc1->left >= prc2->right ) ||
             ( prc1->right <= prc2->left ) ||
             ( prc1->top >= prc2->bottom ) ||
             ( prc1->bottom <= prc2->top ) )
           return COMPLEXREGION;
     }
  }

  if ( ( prc2->right < prc1->right ) ||
       ( prc2->top > prc1->top ) ||
       ( prc2->bottom < prc1->bottom ) )
  {
     if ( ( prc1->left >= prc2->right ) ||
          ( prc1->right <= prc2->left ) ||
          ( prc1->top >= prc2->bottom ) ||
          ( prc1->bottom <= prc2->top ) )
        return COMPLEXREGION;
  }
  else
  {
    return NULLREGION;
  }

  return ERROR;
}

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
  PRGN_ATTR pRgn_Attr;
  HRGN hrgn;
  int x, y;

 /* Normalize points */
  x = x1;
  if ( x1 > x2 )
  {
    x1 = x2;
    x2 = x;
  }

  y = y1;
  if ( y1 > y2 )
  {
    y1 = y2;
    y2 = y;
  }

  if ( (UINT)x1 < 0x80000000 ||
       (UINT)y1 < 0x80000000 ||
       (UINT)x2 > 0x7FFFFFFF ||
       (UINT)y2 > 0x7FFFFFFF )
  {
     SetLastError(ERROR_INVALID_PARAMETER);
     return NULL;
  }
//// Remove when Brush/Pen/Rgn Attr is ready!
  return NtGdiCreateRectRgn(x1,y1,x2,y2);
////
  hrgn = hGetPEBHandle(hctRegionHandle, 0);

  if (!hrgn)
     hrgn = NtGdiCreateRectRgn(0, 0, 1, 1);

  if (!hrgn)
     return hrgn;

  if (!GdiGetHandleUserData((HGDIOBJ) hrgn, GDI_OBJECT_TYPE_REGION, (PVOID) &pRgn_Attr))
  {
     DeleteRegion(hrgn);
     return NULL;
  }

  if (( x1 == x2) || (y1 == y2))
  {
     pRgn_Attr->Flags = NULLREGION;
     pRgn_Attr->Rect.left = pRgn_Attr->Rect.top =
     pRgn_Attr->Rect.right = pRgn_Attr->Rect.bottom = 0;
  }
  else
  {
     pRgn_Attr->Flags = SIMPLEREGION;
     pRgn_Attr->Rect.left   = x1;
     pRgn_Attr->Rect.top    = y1;
     pRgn_Attr->Rect.right  = x2;
     pRgn_Attr->Rect.bottom = y2;
  }

  pRgn_Attr->AttrFlags = (ATTR_RGN_DIRTY|ATTR_RGN_VALID);

  return hrgn;
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
#if 0
// Handle something other than a normal dc object.
  if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
  {
    if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_ExcludeClipRect( hdc, xLeft, yTop, xRight, yBottom);
    else
    {
      PLDC pLDC = GdiGetLDC(hdc);
      if ( pLDC )
      {
         if (pLDC->iType != LDC_EMFLDC || EMFDRV_ExcludeClipRect( hdc, xLeft, yTop, xRight, yBottom))
             return NtGdiExcludeClipRect(hdc, xLeft, yTop, xRight, yBottom);
      }
      else
        SetLastError(ERROR_INVALID_HANDLE);
      return ERROR;
    }
  }
#endif
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
      return ERROR;
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
         return ERROR;
      }
      if (pLDC->iType == LDC_EMFLDC && !EMFDRV_OffsetClipRgn( hdc, nXOffset, nYOffset ))
         return ERROR;
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
  PRGN_ATTR pRgn_Attr;
  int nLeftRect, nTopRect, nRightRect, nBottomRect;

  if (!GdiGetHandleUserData((HGDIOBJ) hrgn, GDI_OBJECT_TYPE_REGION, (PVOID) &pRgn_Attr))
     return NtGdiOffsetRgn(hrgn,nXOffset,nYOffset);

  if ( pRgn_Attr->Flags == NULLREGION)
     return pRgn_Attr->Flags;

  if ( pRgn_Attr->Flags != SIMPLEREGION)
     return NtGdiOffsetRgn(hrgn,nXOffset,nYOffset);

  nLeftRect   = pRgn_Attr->Rect.left;
  nTopRect    = pRgn_Attr->Rect.top;
  nRightRect  = pRgn_Attr->Rect.right;
  nBottomRect = pRgn_Attr->Rect.bottom;

  if (nLeftRect < nRightRect)
  {
     if (nTopRect < nBottomRect)
     {
        nLeftRect   = nXOffset + nLeftRect;
        nTopRect    = nYOffset + nTopRect;
        nRightRect  = nXOffset + nRightRect;
        nBottomRect = nYOffset + nBottomRect;

        /* Mask and bit test. */
        if ( ( nLeftRect   & 0xF8000000 &&
              (nLeftRect   & 0xF8000000) != 0x80000000 ) ||
             ( nTopRect    & 0xF8000000 &&
              (nTopRect    & 0xF8000000) != 0x80000000 ) ||
             ( nRightRect  & 0xF8000000 &&
              (nRightRect  & 0xF8000000) != 0x80000000 ) ||
             ( nBottomRect & 0xF8000000 &&
              (nBottomRect & 0xF8000000) != 0x80000000 ) )
        {
           return ERROR;
        }
        else
        {
           pRgn_Attr->Rect.top    = nTopRect;
           pRgn_Attr->Rect.left   = nLeftRect;
           pRgn_Attr->Rect.right  = nRightRect;
           pRgn_Attr->Rect.bottom = nBottomRect;
           pRgn_Attr->AttrFlags |= ATTR_RGN_DIRTY;
        }
     }
  }
  return pRgn_Attr->Flags;
}

/*
 * @implemented
 */
BOOL
WINAPI
PtInRegion(IN HRGN hrgn,
           int x,
           int y)
{
  PRGN_ATTR pRgn_Attr;

  if (!GdiGetHandleUserData((HGDIOBJ) hrgn, GDI_OBJECT_TYPE_REGION, (PVOID) &pRgn_Attr))
     return NtGdiPtInRegion(hrgn,x,y);

  if ( pRgn_Attr->Flags == NULLREGION)
     return FALSE;

  if ( pRgn_Attr->Flags != SIMPLEREGION)
     return NtGdiPtInRegion(hrgn,x,y);

  return INRECT( pRgn_Attr->Rect, x, y);
}

/*
 * @implemented
 */
BOOL
WINAPI
RectInRegion(HRGN hrgn,
             LPCRECT prcl)
{
  PRGN_ATTR pRgn_Attr;
  RECT rc;

  if (!GdiGetHandleUserData((HGDIOBJ) hrgn, GDI_OBJECT_TYPE_REGION, (PVOID) &pRgn_Attr))
     return NtGdiRectInRegion(hrgn, (LPRECT) prcl);

  if ( pRgn_Attr->Flags == NULLREGION)
     return FALSE;

  if ( pRgn_Attr->Flags != SIMPLEREGION)
     return NtGdiRectInRegion(hrgn, (LPRECT) prcl);

 /* swap the coordinates to make right >= left and bottom >= top */
 /* (region building rectangles are normalized the same way) */
   if ( prcl->top > prcl->bottom)
   {
      rc.top = prcl->bottom;
      rc.bottom = prcl->top;
   }
   else
   {
     rc.top = prcl->top;
     rc.bottom = prcl->bottom;
   }
   if ( prcl->right < prcl->left)
   {
      rc.right = prcl->left;
      rc.left = prcl->right;
   }
   else
   {
      rc.right = prcl->right;
      rc.left = prcl->left;
   }

   if ( ComplexityFromRects( (PRECT)&pRgn_Attr->Rect, &rc) != COMPLEXREGION )
      return TRUE;

   return FALSE;
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
 return ERROR;
}


