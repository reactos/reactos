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
           INT i, SaveL = pRgnData->rdh.rcBound.left;
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
                 CombineRgn(hRgn, hRgnex, *phRgn, RGN_COPY); 
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
STDCALL
MirrorRgnDC(HDC hdc, HRGN hRgn, HRGN *phRgn)
{
  if (!GdiIsHandleValid((HGDIOBJ) hdc) ||
      (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)) return 0;

  return MirrorRgnByWidth(hRgn, NtGdiGetDeviceWidth(hdc), phRgn);
}

/* FUNCTIONS *****************************************************************/

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
CreateRectRgn(int x1, int y1, int x2,int y2)
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
int
STDCALL
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
STDCALL
GetMetaRgn(HDC hdc,
           HRGN hrgn)
{
    return NtGdiGetRandomRgn(hdc, hrgn, METARGN);
}

/*
 * @implemented
 */
BOOL
STDCALL
MirrorRgn(HWND hwnd, HRGN hrgn)
{
  RECT Rect;
  GetWindowRect(hwnd, &Rect);
  return MirrorRgnByWidth(hrgn, Rect.right - Rect.left, NULL);
}

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
BOOL
STDCALL
SetRectRgn(HRGN hrgn,
           int nLeftRect,
           int nTopRect,
           int nRightRect,
           int nBottomRect)
{
#if 0
  PRGN_ATTR Rgn_Attr;

  if (!GdiGetHandleUserData((HGDIOBJ) hrgn, GDI_OBJECT_TYPE_REGION, (PVOID) &Rgn_Attr)) 
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

/*
 * @implemented
 */
int
STDCALL
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


