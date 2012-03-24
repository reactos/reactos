
#include "DibLib.h"

#define __USES_SOURCE 1
#define __USES_PATTERN 1
#define __USES_DEST 1
#define __USES_MASK 0

#define __FUNCTIONNAME BitBlt

#define _DibDoRop(pBltData, M, D, S, P) pBltData->apfnDoRop[0](D,S,P)

#include "DibLib_AllSrcBPP.h"

#undef __FUNCTIONNAME
#define __FUNCTIONNAME BitBlt_Solid
#define __USES_SOLID_BRUSH 1
#include "DibLib_AllSrcBPP.h"

VOID
FASTCALL
Dib_BitBlt(PBLTDATA pBltData)
{
    /* Check for solid brush */
    if (pBltData->ulSolidColor != 0xFFFFFFFF)
    {
        /* Use the solid version of PATCOPY! */
        gapfnBitBlt_Solid[pBltData->siDst.iFormat][pBltData->siSrc.iFormat](pBltData);
    }
    else
    {
        /* Use the pattern version */
        gapfnBitBlt[pBltData->siDst.iFormat][pBltData->siSrc.iFormat](pBltData);
    }
}



