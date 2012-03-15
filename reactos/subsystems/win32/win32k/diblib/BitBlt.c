
#include "DibLib.h"

#define __USES_SOURCE 1
#define __USES_PATTERN 1
#define __USES_DEST 1
#define __USES_MASK 0

#define _DibDoRop(pBltData, M, D, S, P) pBltData->apfnDoRop[0](D,S,P)

#define __FUNCTIONNAME BitBlt
#include "DibLib_AllSrcBPP.h"

#undef __FUNCTIONNAME
#define __FUNCTIONNAME BitBlt_Solid
#define __USES_SOLID_BRUSH 1
#include "DibLib_AllSrcBPP.h"

VOID
FASTCALL
Dib_BitBlt(PBLTDATA pBltData)
{
    gapfnBitBlt[pBltData->siDst.iFormat][pBltData->siSrc.iFormat](pBltData);
}



