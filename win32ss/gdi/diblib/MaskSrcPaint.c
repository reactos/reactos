
#include "DibLib.h"

#define __USES_SOURCE 1
#define __USES_PATTERN 0
#define __USES_DEST 1
#define __USES_MASK 1

#define __FUNCTIONNAME MaskSrcPaint

#define _DibDoRop(pBltData, M, D, S, P) pBltData->apfnDoRop[M](D,S,0)

#include "DibLib_AllSrcBPP.h"

VOID
FASTCALL
Dib_MaskSrcPaint(PBLTDATA pBltData)
{
    gapfnMaskSrcPaint[pBltData->siDst.iFormat][pBltData->siSrc.iFormat](pBltData);
}

