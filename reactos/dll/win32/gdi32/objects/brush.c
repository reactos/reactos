#include "precomp.h"

#define NDEBUG
#include <debug.h>



/*
 * @implemented
 */
HPEN
APIENTRY
ExtCreatePen(DWORD dwPenStyle,
             DWORD dwWidth,
             CONST LOGBRUSH *lplb,
             DWORD dwStyleCount,
             CONST DWORD *lpStyle)
{
   PVOID lpPackedDIB = NULL;
   HPEN hPen = NULL;
   PBITMAPINFO pConvertedInfo = NULL;
   UINT ConvertedInfoSize = 0, lbStyle;
   BOOL Hit = FALSE;

   if ((dwPenStyle & PS_STYLE_MASK) == PS_USERSTYLE)
   {
      if(!lpStyle)
      {
         SetLastError(ERROR_INVALID_PARAMETER);
         return 0;
      }
   } // This is an enhancement and prevents a call to kernel space.
   else if ((dwPenStyle & PS_STYLE_MASK) == PS_INSIDEFRAME &&
            (dwPenStyle & PS_TYPE_MASK) != PS_GEOMETRIC)
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return 0;
   }
   else if ((dwPenStyle & PS_STYLE_MASK) == PS_ALTERNATE &&
            (dwPenStyle & PS_TYPE_MASK) != PS_COSMETIC)
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return 0;
   }
   else
   {
      if (dwStyleCount || lpStyle)
      {
         SetLastError(ERROR_INVALID_PARAMETER);
         return 0;
      }
   }

   lbStyle = lplb->lbStyle;

   if (lplb->lbStyle > BS_HATCHED)   
   {
      if (lplb->lbStyle == BS_PATTERN)
      {
         pConvertedInfo = (PBITMAPINFO)lplb->lbHatch;
         if (!pConvertedInfo) return 0;
      }
      else
      {
         if ((lplb->lbStyle == BS_DIBPATTERN) || (lplb->lbStyle == BS_DIBPATTERNPT))
         {
            if (lplb->lbStyle == BS_DIBPATTERN)
            {
               lbStyle = BS_DIBPATTERNPT;
               lpPackedDIB = GlobalLock((HGLOBAL)lplb->lbHatch);
               if (lpPackedDIB == NULL) return 0;
            }
            pConvertedInfo = ConvertBitmapInfo((PBITMAPINFO)lpPackedDIB,
                                                          lplb->lbColor,
                                                     &ConvertedInfoSize,
                                                                   TRUE);
            Hit = TRUE; // We converted DIB.
         }
         else
            pConvertedInfo = (PBITMAPINFO)lpStyle;
      }
   }
   else
     pConvertedInfo = (PBITMAPINFO)lplb->lbHatch;
   

   hPen = NtGdiExtCreatePen(dwPenStyle,
                               dwWidth,
                               lbStyle,
                         lplb->lbColor,
                         lplb->lbHatch,
             (ULONG_PTR)pConvertedInfo,
                          dwStyleCount,
                       (PULONG)lpStyle,
                     ConvertedInfoSize,
                                 FALSE,
                                  NULL);


   if (lplb->lbStyle == BS_DIBPATTERN) GlobalUnlock((HGLOBAL)lplb->lbHatch);

   if (Hit)
   {
      if ((PBITMAPINFO)lpPackedDIB != pConvertedInfo)
         RtlFreeHeap(RtlGetProcessHeap(), 0, pConvertedInfo);
   }
   return hPen;
}

/*
 * @implemented
 */
HBRUSH WINAPI
CreateDIBPatternBrush(
   HGLOBAL hglbDIBPacked,
   UINT fuColorSpec)
{
   PVOID lpPackedDIB;
   HBRUSH hBrush = NULL;
   PBITMAPINFO pConvertedInfo;
   UINT ConvertedInfoSize;

   lpPackedDIB = GlobalLock(hglbDIBPacked);
   if (lpPackedDIB == NULL)
      return 0;

   pConvertedInfo = ConvertBitmapInfo((PBITMAPINFO)lpPackedDIB, fuColorSpec,
                                      &ConvertedInfoSize, TRUE);
   if (pConvertedInfo)
   {
      hBrush = NtGdiCreateDIBBrush(pConvertedInfo, fuColorSpec,
                                   ConvertedInfoSize, FALSE, FALSE, lpPackedDIB);
      if ((PBITMAPINFO)lpPackedDIB != pConvertedInfo)
         RtlFreeHeap(RtlGetProcessHeap(), 0, pConvertedInfo);
   }

   GlobalUnlock(hglbDIBPacked);

   return hBrush;
}

/*
 * @implemented
 */
HBRUSH WINAPI
CreateDIBPatternBrushPt(
   CONST VOID *lpPackedDIB,
   UINT fuColorSpec)
{
   HBRUSH hBrush = NULL;
   PBITMAPINFO pConvertedInfo;
   UINT ConvertedInfoSize;

   if (lpPackedDIB == NULL)
      return 0;

   pConvertedInfo = ConvertBitmapInfo((PBITMAPINFO)lpPackedDIB, fuColorSpec,
                                      &ConvertedInfoSize, TRUE);
   if (pConvertedInfo)
   {
      hBrush = NtGdiCreateDIBBrush(pConvertedInfo, fuColorSpec,
                                   ConvertedInfoSize, FALSE, FALSE, (PVOID)lpPackedDIB);
      if ((PBITMAPINFO)lpPackedDIB != pConvertedInfo)
         RtlFreeHeap(RtlGetProcessHeap(), 0, pConvertedInfo);
   }

   return hBrush;
}

/*
 * @implemented
 */
HBRUSH
WINAPI
CreateHatchBrush(INT fnStyle,
                 COLORREF clrref)
{
    return NtGdiCreateHatchBrushInternal(fnStyle, clrref, FALSE);
}

/*
 * @implemented
 */
HBRUSH
WINAPI
CreatePatternBrush(HBITMAP hbmp)
{
    return NtGdiCreatePatternBrushInternal(hbmp, FALSE, FALSE);
}

/*
 * @implemented
 */
HBRUSH
WINAPI
CreateSolidBrush(IN COLORREF crColor)
{
    /* Call Server-Side API */
    return NtGdiCreateSolidBrush(crColor, NULL);
}

/*
 * @implemented
 */
HBRUSH WINAPI
CreateBrushIndirect(
   CONST LOGBRUSH *LogBrush)
{
   HBRUSH hBrush;

   switch (LogBrush->lbStyle)
   {
      case BS_DIBPATTERN8X8:
      case BS_DIBPATTERN:
         hBrush = CreateDIBPatternBrush((HGLOBAL)LogBrush->lbHatch,
                                        LogBrush->lbColor);
         break;

      case BS_DIBPATTERNPT:
         hBrush = CreateDIBPatternBrushPt((PVOID)LogBrush->lbHatch,
                                          LogBrush->lbColor);
         break;

      case BS_PATTERN:
         hBrush = NtGdiCreatePatternBrushInternal((HBITMAP)LogBrush->lbHatch,
                                                  FALSE,
                                                  FALSE);
         break;

      case BS_PATTERN8X8:
         hBrush = NtGdiCreatePatternBrushInternal((HBITMAP)LogBrush->lbHatch,
                                                  FALSE,
                                                  TRUE);
         break;

      case BS_SOLID:
         hBrush = NtGdiCreateSolidBrush(LogBrush->lbColor, 0);
         break;

      case BS_HATCHED:
         hBrush = NtGdiCreateHatchBrushInternal(LogBrush->lbHatch,
                                                LogBrush->lbColor,
                                                FALSE);
         break;

      case BS_NULL:
         hBrush = NtGdiGetStockObject(NULL_BRUSH);
         break;

      default:
         SetLastError(ERROR_INVALID_PARAMETER);
         hBrush = NULL;
         break;
   }

   return hBrush;
}

BOOL
WINAPI
PatBlt(HDC hdc,
       int nXLeft,
       int nYLeft,
       int nWidth,
       int nHeight,
       DWORD dwRop)
{
    /* FIXME some part need be done in user mode */
    return NtGdiPatBlt( hdc,  nXLeft,  nYLeft,  nWidth,  nHeight,  dwRop);
}

BOOL
WINAPI
PolyPatBlt(IN HDC hdc,
           IN DWORD rop4,
           IN PPOLYPATBLT pPoly,
           IN DWORD Count,
           IN DWORD Mode)
{
    /* FIXME some part need be done in user mode */
    return NtGdiPolyPatBlt(hdc, rop4, pPoly,Count,Mode);
}

/*
 * @implemented
 *
 */
int
WINAPI
GetROP2(HDC hdc)
{
  PDC_ATTR Dc_Attr;
  if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return 0;
  return Dc_Attr->jROP2;
}

/*
 * @implemented
 */
int
WINAPI
SetROP2(HDC hdc,
        int fnDrawMode)
{
  PDC_ATTR Dc_Attr;
  INT Old_ROP2;
  
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_SetROP2( hdc, fnDrawMode);
    else
    {
      PLDC pLDC = GdiGetLDC(hdc);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_SetROP2(( hdc, fnDrawMode);
      }
      return FALSE;
    }
 }
#endif
 if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return FALSE;

 if (NtCurrentTeb()->GdiTebBatch.HDC == hdc)
 {
    if (Dc_Attr->ulDirty_ & DC_MODE_DIRTY)
    {
       NtGdiFlush();
       Dc_Attr->ulDirty_ &= ~DC_MODE_DIRTY;
    }
 }

 Old_ROP2 = Dc_Attr->jROP2;
 Dc_Attr->jROP2 = fnDrawMode;

 return Old_ROP2;
}

/*
 * @implemented
 *
 */
BOOL
WINAPI
GetBrushOrgEx(HDC hdc,LPPOINT pt)
{
  PDC_ATTR Dc_Attr;

  if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return FALSE;
  if (pt)
  {
     pt->x = Dc_Attr->ptlBrushOrigin.x;
     pt->y = Dc_Attr->ptlBrushOrigin.y;
  }
  return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetBrushOrgEx(HDC hdc,
              int nXOrg,
              int nYOrg,
              LPPOINT lppt)
{
  PDC_ATTR Dc_Attr;
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
 {
    PLDC pLDC = GdiGetLDC(hdc);
    if ( (pLDC == NULL) || (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC))
    {
       SetLastError(ERROR_INVALID_HANDLE);
       return FALSE;
    }
    if (pLDC->iType == LDC_EMFLDC)
    {
      return EMFDRV_SetBrushOrg(hdc, nXOrg, nYOrg); // ReactOS only.
    }
    return FALSE;
 }
#endif
 if (GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr))
 {
    PTEB pTeb = NtCurrentTeb();
    if (lppt)
    {
       lppt->x = Dc_Attr->ptlBrushOrigin.x;
       lppt->y = Dc_Attr->ptlBrushOrigin.y;
    }
    if ((nXOrg == Dc_Attr->ptlBrushOrigin.x) && (nYOrg == Dc_Attr->ptlBrushOrigin.y))
       return TRUE;

    if(((pTeb->GdiTebBatch.HDC == NULL) || (pTeb->GdiTebBatch.HDC == hdc)) &&
       ((pTeb->GdiTebBatch.Offset + sizeof(GDIBSSETBRHORG)) <= GDIBATCHBUFSIZE) &&
       (!(Dc_Attr->ulDirty_ & DC_DIBSECTION)) )
    {
       PGDIBSSETBRHORG pgSBO = (PGDIBSSETBRHORG)(&pTeb->GdiTebBatch.Buffer[0] +
                                                      pTeb->GdiTebBatch.Offset);

       Dc_Attr->ptlBrushOrigin.x = nXOrg;
       Dc_Attr->ptlBrushOrigin.y = nYOrg;

       pgSBO->gbHdr.Cmd = GdiBCSetBrushOrg;
       pgSBO->gbHdr.Size = sizeof(GDIBSSETBRHORG);
       pgSBO->ptlBrushOrigin = Dc_Attr->ptlBrushOrigin;
       
       pTeb->GdiTebBatch.Offset += sizeof(GDIBSSETBRHORG);
       pTeb->GdiTebBatch.HDC = hdc;
       pTeb->GdiBatchCount++;
       DPRINT("Loading the Flush!! COUNT-> %d\n", pTeb->GdiBatchCount);

       if (pTeb->GdiBatchCount >= GDI_BatchLimit)
       {
       DPRINT("Call GdiFlush!!\n");
       NtGdiFlush();
       DPRINT("Exit GdiFlush!!\n");
       }
       return TRUE;
    }
 }
 return NtGdiSetBrushOrg(hdc,nXOrg,nYOrg,lppt);
}
