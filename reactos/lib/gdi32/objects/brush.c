#include "precomp.h"

#define NDEBUG
#include <debug.h>

/*
 * @implemented
 */
BOOL
STDCALL
FixBrushOrgEx(
   HDC hDC,
   INT nXOrg,
   INT nYOrg,
   LPPOINT lpPoint)
{
   return FALSE;
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
                                   ConvertedInfoSize, lpPackedDIB);
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
                                   ConvertedInfoSize, lpPackedDIB);
      if ((PBITMAPINFO)lpPackedDIB != pConvertedInfo)
         RtlFreeHeap(RtlGetProcessHeap(), 0, pConvertedInfo);
   }

   return hBrush;
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
      case BS_PATTERN8X8:
         hBrush = NtGdiCreatePatternBrush((HBITMAP)LogBrush->lbHatch);
         break;

      case BS_SOLID:
         hBrush = NtGdiCreateSolidBrush(LogBrush->lbColor);
         break;

      case BS_HATCHED:
         hBrush = NtGdiCreateHatchBrush(LogBrush->lbHatch, LogBrush->lbColor);
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

