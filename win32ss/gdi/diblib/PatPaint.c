
#include "DibLib.h"

#define __USES_SOURCE 0
#define __USES_PATTERN 1
#define __USES_DEST 1
#define __USES_MASK 0

#define __FUNCTIONNAME PatPaint

#define _DibDoRop(pBltData, M, D, S, P) pBltData->apfnDoRop[0](D,0,P)

#include "DibLib_AllDstBPP.h"

#undef __FUNCTIONNAME
#define __FUNCTIONNAME PatPaint_Solid
#define __USES_SOLID_BRUSH 1
#include "DibLib_AllDstBPP.h"

VOID
FASTCALL
Dib_PatPaint(PBLTDATA pBltData)
{
    /* Check for solid brush */
    if (pBltData->ulSolidColor != 0xFFFFFFFF)
    {
        /* Use the solid version of PATCOPY! */
        gapfnPatPaint_Solid[pBltData->siDst.iFormat](pBltData);
    }
    else
    {
        /* Use the pattern version */
        gapfnPatPaint[pBltData->siDst.iFormat](pBltData);
    }
}



