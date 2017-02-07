
#include "DibLib.h"

#define __USES_SOURCE 0
#define __USES_PATTERN 1
#define __USES_DEST 1
#define __USES_MASK 1

#define __FUNCTIONNAME MaskPatPaint

#define _DibDoRop(pBltData, M, D, S, P) pBltData->apfnDoRop[M](D,0,P)

#include "DibLib_AllDstBPP.h"

#undef __FUNCTIONNAME
#define __FUNCTIONNAME MaskPatPaint_Solid
#define __USES_SOLID_BRUSH 1
#include "DibLib_AllDstBPP.h"

VOID
FASTCALL
Dib_MaskPatPaint(PBLTDATA pBltData)
{
    /* Check for solid brush */
    if (pBltData->ulSolidColor != 0xFFFFFFFF)
    {
        /* Use the solid version of PATCOPY! */
        gapfnMaskPatPaint_Solid[pBltData->siDst.iFormat](pBltData);
    }
    else
    {
        /* Use the pattern version */
        gapfnMaskPatPaint[pBltData->siDst.iFormat](pBltData);
    }
}



