
#include "DibLib.h"

#define __USES_SOURCE 1
#define __USES_PATTERN 0
#define __USES_DEST 0
#define __USES_MASK 0

#define __FUNCTIONNAME BitBlt_SRCCOPY

#define _DibDoRop(pBltData, M, D, S, P) ROP_SRCCOPY(D,S,P)

#include "diblib_allsrcbpp.h"

VOID
FASTCALL
Dib_BitBlt_SRCCOPY(PBLTDATA pBltData)
{
    // TODO: XLATEless same-surface variants
    gapfnBitBlt_SRCCOPY[pBltData->siDst.iFormat][pBltData->siSrc.iFormat](pBltData);
}

