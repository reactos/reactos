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
    /* Call NTGDI (hack... like most of gdi32..sigh) */
    return NtGdiExtCreatePen(dwPenStyle,
                             dwWidth,
                             lplb->lbStyle,
                             lplb->lbColor,
                             lplb->lbHatch,
                             0,
                             dwStyleCount,
                             (PULONG)lpStyle,
                             0,
                             FALSE,
                             NULL);
}

/*
 * @implemented
 */
HBRUSH STDCALL
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
HBRUSH STDCALL
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
STDCALL
CreateHatchBrush(INT fnStyle,
                 COLORREF clrref)
{
    return NtGdiCreateHatchBrushInternal(fnStyle, clrref, FALSE);
}

/*
 * @implemented
 */
HBRUSH
STDCALL
CreatePatternBrush(HBITMAP hbmp)
{
    return NtGdiCreatePatternBrushInternal(hbmp, FALSE, FALSE);
}

/*
 * @implemented
 */
HBRUSH
STDCALL
CreateSolidBrush(IN COLORREF crColor)
{
    /* Call Server-Side API */
    return NtGdiCreateSolidBrush(crColor, NULL);
}

/*
 * @implemented
 */
HBRUSH STDCALL
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

