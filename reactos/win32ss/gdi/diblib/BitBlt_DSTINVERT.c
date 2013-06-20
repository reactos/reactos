
#include "DibLib.h"

#define __USES_SOURCE 0
#define __USES_PATTERN 0
#define __USES_DEST 1
#define __USES_MASK 0

#define _DibDoRop(pBltData, M, D, S, P) ROP_DSTINVERT(D,S,P)

#define __FUNCTIONNAME BitBlt_DSTINVERT
#include "DibLib_AllDstBPP.h"

VOID
FASTCALL
Dib_BitBlt_DSTINVERT(PBLTDATA pBltData)
{
    gapfnBitBlt_DSTINVERT[pBltData->siDst.iFormat](pBltData);
}

