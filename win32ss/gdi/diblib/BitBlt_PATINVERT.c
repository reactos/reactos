
#include "DibLib.h"

#define __USES_SOURCE 0
#define __USES_PATTERN 1
#define __USES_DEST 1
#define __USES_MASK 0

#define __FUNCTIONNAME BitBlt_PATINVERT

#define _DibDoRop(pBltData, M, D, S, P) ROP_PATINVERT(D,S,P)

#include "DibLib_AllDstBPP.h"

#undef __FUNCTIONNAME
#define __FUNCTIONNAME BitBlt_PATINVERT_Solid
#define __USES_SOLID_BRUSH 1
#include "DibLib_AllDstBPP.h"

VOID
FASTCALL
Dib_BitBlt_PATINVERT(PBLTDATA pBltData)
{
    /* Check for solid brush */
    if (pBltData->ulSolidColor != 0xFFFFFFFF)
    {
        /* Use the solid version of PATCOPY! */
        gapfnBitBlt_PATINVERT_Solid[pBltData->siDst.iFormat](pBltData);
    }
    else
    {
        /* Use the pattern version */
        gapfnBitBlt_PATINVERT[pBltData->siDst.iFormat](pBltData);
    }
}

