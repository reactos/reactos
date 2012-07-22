
#include "DibLib.h"

#define __USES_SOURCE 0
#define __USES_PATTERN 1
#define __USES_DEST 0
#define __USES_MASK 1

#define __FUNCTIONNAME MaskPatBlt

#define _DibDoRop(pBltData, M, D, S, P) pBltData->apfnDoRop[M](0,0,P)

#include "DibLib_AllDstBPP.h"

#undef __FUNCTIONNAME
#define __FUNCTIONNAME MaskPatBlt_Solid
#define __USES_SOLID_BRUSH 1
#include "DibLib_AllDstBPP.h"

VOID
FASTCALL
Dib_MaskPatBlt(PBLTDATA pBltData)
{
    gapfnMaskPatBlt[pBltData->siDst.iFormat](pBltData);
}

