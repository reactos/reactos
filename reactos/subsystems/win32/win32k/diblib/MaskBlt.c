
#include "DibLib.h"

#define __USES_SOURCE 1
#define __USES_PATTERN 1
#define __USES_DEST 1
#define __USES_MASK 1

#define __FUNCTIONNAME MaskBlt

#define _DibDoRop(pBltData, M, D, S, P) pBltData->apfnDoRop[M](D,S,P)

#include "diblib_allsrcbpp.h"

#undef __FUNCTIONNAME
#define __FUNCTIONNAME MaskBlt_Solid
#define __USES_SOLID_BRUSH 1
#include "diblib_allsrcbpp.h"

VOID
FASTCALL
Dib_MaskBlt(PBLTDATA pBltData)
{
    gapfnMaskBlt[pBltData->siDst.iFormat][pBltData->siSrc.iFormat](pBltData);
}




