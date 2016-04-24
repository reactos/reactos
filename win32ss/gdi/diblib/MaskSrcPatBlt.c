
#include "DibLib.h"

#define __USES_SOURCE 1
#define __USES_PATTERN 1
#define __USES_DEST 1
#define __USES_MASK 1

#define __FUNCTIONNAME MaskSrcPatBlt

#define _DibDoRop(pBltData, M, D, S, P) pBltData->apfnDoRop[M](D,S,P)

#include "DibLib_AllSrcBPP.h"

#undef __FUNCTIONNAME
#define __FUNCTIONNAME MaskSrcPatBlt_Solid
#define __USES_SOLID_BRUSH 1
#include "DibLib_AllSrcBPP.h"

VOID
FASTCALL
Dib_MaskSrcPatBlt(PBLTDATA pBltData)
{
    gapfnMaskSrcPatBlt[pBltData->siDst.iFormat][pBltData->siSrc.iFormat](pBltData);
}


