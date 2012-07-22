
#include "DibLib.h"

#define __USES_SOURCE 0
#define __USES_PATTERN 1
#define __USES_DEST 0
#define __USES_MASK 0

#define __FUNCTIONNAME BitBlt_PATCOPY

#define _DibDoRop(pBltData, M, D, S, P) ROP_PATCOPY(D,S,P)

#include "DibLib_AllDstBPP.h"

#undef __FUNCTIONNAME
#define __FUNCTIONNAME BitBlt_PATCOPY_Solid
#define __USES_SOLID_BRUSH 1
#include "DibLib_AllDstBPP.h"

VOID
FASTCALL
Dib_BitBlt_PATCOPY(PBLTDATA pBltData)
{
    /* Check for solid brush */
    if (pBltData->ulSolidColor != 0xFFFFFFFF)
    {
        /* Use the solid version of PATCOPY! */
        gapfnBitBlt_PATCOPY_Solid[pBltData->siDst.iFormat](pBltData);
    }
    else
    {
        /* Use the pattern version */
        gapfnBitBlt_PATCOPY[pBltData->siDst.iFormat](pBltData);
    }
}

