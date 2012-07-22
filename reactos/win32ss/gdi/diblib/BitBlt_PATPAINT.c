
#include "DibLib.h"

#define __USES_SOURCE 1
#define __USES_PATTERN 1
#define __USES_DEST 1
#define __USES_MASK 0

#define __FUNCTIONNAME BitBlt_PATPAINT

#define _DibDoRop(pBltData, M, D, S, P) ROP_PATPAINT(D,S,P)

#include "DibLib_AllSrcBPP.h"

#undef __FUNCTIONNAME
#define __FUNCTIONNAME BitBlt_PATPAINT_Solid
#define __USES_SOLID_BRUSH 1
#include "DibLib_AllSrcBPP.h"

VOID
FASTCALL
Dib_BitBlt_PATPAINT(PBLTDATA pBltData)
{
    /* Check for solid brush */
    if (pBltData->ulSolidColor != 0xFFFFFFFF)
    {
        /* Use the solid version of PATCOPY! */
        gapfnBitBlt_PATPAINT_Solid[pBltData->siDst.iFormat][pBltData->siSrc.iFormat](pBltData);
    }
    else
    {
        /* Use the pattern version */
        gapfnBitBlt_PATPAINT[pBltData->siDst.iFormat][pBltData->siSrc.iFormat](pBltData);
    }
}

