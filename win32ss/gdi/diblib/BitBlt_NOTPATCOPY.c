
#include "DibLib.h"

#define __USES_SOURCE 0
#define __USES_PATTERN 1
#define __USES_DEST 0
#define __USES_MASK 0

#define __FUNCTIONNAME BitBlt_NOTPATCOPY

#define _DibDoRop(pBltData, M, D, S, P) (~(P))

#include "DibLib_AllDstBPP.h"

extern PFN_DIBFUNCTION gapfnBitBlt_PATCOPY_Solid[];

VOID
FASTCALL
Dib_BitBlt_NOTPATCOPY(PBLTDATA pBltData)
{
    /* Check for solid brush */
    if (pBltData->ulSolidColor != 0xFFFFFFFF)
    {
        /* Prepare inverted colot */
        pBltData->ulSolidColor = ~pBltData->ulSolidColor;

        /* Use the solid version of PATCOPY! */
        gapfnBitBlt_PATCOPY_Solid[pBltData->siDst.iFormat](pBltData);
    }
    else
    {
        /* Use the pattern version */
        gapfnBitBlt_NOTPATCOPY[pBltData->siDst.iFormat](pBltData);
    }
}

