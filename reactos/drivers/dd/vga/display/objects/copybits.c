#include "../vgaddi.h"
#include "../vgavideo/vgavideo.h"

#define DBG
#include <debug.h>

BOOL STDCALL
DrvCopyBits(OUT PSURFOBJ  DestObj,  
            IN  PSURFOBJ  SourceObj,  
            IN  PCLIPOBJ  ClipObj,  
            IN  PXLATEOBJ XLateObj,  
            IN  PRECTL    DestRectL,  
            IN  PPOINTL   SrcPointL)
{
  BOOL Done = FALSE;

  if (STYPE_BITMAP == DestObj->iType && BMF_4BPP == DestObj->iBitmapFormat &&
      STYPE_DEVICE == SourceObj->iType)
    {
    /* Screen to 4 BPP DIB */
    DIB_BltFromVGA(SrcPointL->x, SrcPointL->y,
                   DestRectL->right - DestRectL->left,
                   DestRectL->bottom - DestRectL->top,
                   DestObj->pvScan0, DestObj->lDelta);
    Done = TRUE;
    }
  else if (STYPE_DEVICE == DestObj->iType &&
           STYPE_BITMAP == SourceObj->iType && BMF_4BPP == SourceObj->iBitmapFormat)
    {
    /* 4 BPP DIB to Screen */
    DIB_BltToVGA(DestRectL->left, DestRectL->top,
                 DestRectL->right - DestRectL->left,
                 DestRectL->bottom - DestRectL->top,
                 SourceObj->pvScan0, SourceObj->lDelta);
    Done = TRUE;
    }

  return Done;
}
