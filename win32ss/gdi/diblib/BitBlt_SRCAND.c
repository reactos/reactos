
#include "DibLib.h"

#define __USES_SOURCE 1
#define __USES_PATTERN 0
#define __USES_DEST 1
#define __USES_MASK 0

#define __FUNCTIONNAME BitBlt_SRCAND

#define _DibDoRop(pBltData, M, D, S, P) ROP_SRCAND(D,S,P)

#include "DibLib_AllSrcBPP.h"

VOID
FASTCALL
Dib_BitBlt_SRCAND(PBLTDATA pBltData)
{
    // TODO: XLATEless same-surface variants
    gapfnBitBlt_SRCAND[pBltData->siDst.iFormat][pBltData->siSrc.iFormat](pBltData);
}

