
#include "DibLib.h"

#define __USES_SOURCE 0
#define __USES_PATTERN 0
#define __USES_DEST 1
#define __USES_MASK 1

#define __FUNCTIONNAME MaskPaint

#define _DibDoRop(pBltData, M, D, S, P) pBltData->apfnDoRop[M](D,0,0)

#include "DibLib_AllDstBPP.h"

VOID
FASTCALL
Dib_MaskPaint(PBLTDATA pBltData)
{
    // TODO: XLATEless same-surface variants
    gapfnMaskPaint[pBltData->siDst.iFormat](pBltData);
}




